#include <red_plasma/engine/engine_api.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>

#include <new>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <limits>
#include <fstream>
#include <algorithm>
#include <cstdlib>

struct wl_display;
struct wl_surface;

struct rp_engine {
    rp_logger_t logger{};
    std::string last_error;

    VkInstance instance = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VkPhysicalDevice phys = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    uint32_t width = 0;
    uint32_t height = 0;

    uint32_t gfx_qfam = UINT32_MAX;
    uint32_t present_qfam = UINT32_MAX;
    VkQueue gfx_queue = VK_NULL_HANDLE;
    VkQueue present_queue = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swap_format = VK_FORMAT_UNDEFINED;
    VkExtent2D swap_extent{};
    std::vector<VkImage> swap_images;
    std::vector<VkImageView> swap_views;

    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;

    VkCommandPool cmd_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> cmd_bufs;

    VkSemaphore sem_image_avail = VK_NULL_HANDLE;
    VkSemaphore sem_render_done = VK_NULL_HANDLE;
    VkFence fence_inflight = VK_NULL_HANDLE;

    bool needs_swapchain_rebuild = false;
};

static void rp_log(const rp_logger_t& lg, int level, const char* msg) {
    if (!lg.callback) return;
    if (level < lg.min_level) return;
    lg.callback(lg.user, level, msg);
}

static rp_result_t rp_fail(rp_engine_t* e, rp_result_t code, const char* msg) {
    if (e) e->last_error = msg ? msg : "unknown error";
    return code;
}

static bool has_layer(const std::vector<VkLayerProperties>& layers, const char* name) {
    for (const auto& l : layers) if (std::strcmp(l.layerName, name) == 0) return true;
    return false;
}

static std::vector<char> read_file(const char* path) {
    if (!path) return {};
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> data((size_t)size);
    if (!f.read(data.data(), size)) return {};
    return data;
}

static void destroy_swapchain(rp_engine* e) {
    if (!e || !e->device) return;

    for (auto fb : e->framebuffers) vkDestroyFramebuffer(e->device, fb, nullptr);
    e->framebuffers.clear();

    if (e->pipeline) vkDestroyPipeline(e->device, e->pipeline, nullptr);
    e->pipeline = VK_NULL_HANDLE;

    if (e->pipeline_layout) vkDestroyPipelineLayout(e->device, e->pipeline_layout, nullptr);
    e->pipeline_layout = VK_NULL_HANDLE;

    if (e->render_pass) vkDestroyRenderPass(e->device, e->render_pass, nullptr);
    e->render_pass = VK_NULL_HANDLE;

    if (!e->cmd_bufs.empty()) {
        vkFreeCommandBuffers(e->device, e->cmd_pool, (uint32_t)e->cmd_bufs.size(), e->cmd_bufs.data());
        e->cmd_bufs.clear();
    }

    for (auto v : e->swap_views) vkDestroyImageView(e->device, v, nullptr);
    e->swap_views.clear();
    e->swap_images.clear();

    if (e->swapchain) vkDestroySwapchainKHR(e->device, e->swapchain, nullptr);
    e->swapchain = VK_NULL_HANDLE;
}

static bool pick_device_and_queues(rp_engine* e) {
    uint32_t dev_count = 0;
    if (vkEnumeratePhysicalDevices(e->instance, &dev_count, nullptr) != VK_SUCCESS || dev_count == 0)
        return false;

    std::vector<VkPhysicalDevice> devs(dev_count);
    vkEnumeratePhysicalDevices(e->instance, &dev_count, devs.data());

    for (auto dev : devs) {
        uint32_t qcount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qcount, nullptr);
        std::vector<VkQueueFamilyProperties> qprops(qcount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qcount, qprops.data());

        uint32_t gfx = UINT32_MAX, pres = UINT32_MAX;

        for (uint32_t i = 0; i < qcount; ++i) {
            if ((qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && gfx == UINT32_MAX)
                gfx = i;

            VkBool32 present = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, e->surface, &present);
            if (present && pres == UINT32_MAX)
                pres = i;
        }

        // Require swapchain extension
        uint32_t ext_count = 0;
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &ext_count, nullptr);
        std::vector<VkExtensionProperties> exts(ext_count);
        vkEnumerateDeviceExtensionProperties(dev, nullptr, &ext_count, exts.data());

        bool has_swapchain = false;
        for (auto& x : exts) {
            if (std::strcmp(x.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                has_swapchain = true;
                break;
            }
        }

        if (gfx != UINT32_MAX && pres != UINT32_MAX && has_swapchain) {
            e->phys = dev;
            e->gfx_qfam = gfx;
            e->present_qfam = pres;
            return true;
        }
    }
    return false;
}

static rp_result_t create_device(rp_engine* e) {
    float prio = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> qcis;
    qcis.reserve(2);

    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = e->gfx_qfam;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    qcis.push_back(qci);

    if (e->present_qfam != e->gfx_qfam) {
        VkDeviceQueueCreateInfo qci2 = qci;
        qci2.queueFamilyIndex = e->present_qfam;
        qcis.push_back(qci2);
    }

    const char* dev_exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = (uint32_t)qcis.size();
    dci.pQueueCreateInfos = qcis.data();
    dci.enabledExtensionCount = 1;
    dci.ppEnabledExtensionNames = dev_exts;

    VkResult vr = vkCreateDevice(e->phys, &dci, nullptr, &e->device);
    if (vr != VK_SUCCESS || e->device == VK_NULL_HANDLE)
        return rp_fail(e, RP_ERR_VULKAN_INIT_FAILED, "vkCreateDevice failed");

    vkGetDeviceQueue(e->device, e->gfx_qfam, 0, &e->gfx_queue);
    vkGetDeviceQueue(e->device, e->present_qfam, 0, &e->present_queue);

    return RP_OK;
}

static rp_result_t create_command_pool_and_sync(rp_engine* e) {
    VkCommandPoolCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pci.queueFamilyIndex = e->gfx_qfam;

    if (vkCreateCommandPool(e->device, &pci, nullptr, &e->cmd_pool) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateCommandPool failed");

    VkSemaphoreCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(e->device, &sci, nullptr, &e->sem_image_avail) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateSemaphore (image_avail) failed");

    if (vkCreateSemaphore(e->device, &sci, nullptr, &e->sem_render_done) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateSemaphore (render_done) failed");

    VkFenceCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(e->device, &fci, nullptr, &e->fence_inflight) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateFence failed");

    return RP_OK;
}

static VkSurfaceFormatKHR choose_format(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return f;
    }
    return formats[0];
}

static VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& modes) {
    for (auto m : modes) if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D choose_extent(const VkSurfaceCapabilitiesKHR& caps, uint32_t w, uint32_t h) {
    if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
    VkExtent2D e{};
    e.width  = std::clamp(w, caps.minImageExtent.width,  caps.maxImageExtent.width);
    e.height = std::clamp(h, caps.minImageExtent.height, caps.maxImageExtent.height);
    return e;
}

static rp_result_t create_render_pass(rp_engine* e) {
    VkAttachmentDescription color{};
    color.format = e->swap_format;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference cref{};
    cref.attachment = 0;
    cref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription sub{};
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &cref;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &color;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &sub;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;

    if (vkCreateRenderPass(e->device, &rpci, nullptr, &e->render_pass) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateRenderPass failed");

    return RP_OK;
}

static rp_result_t create_pipeline(rp_engine* e) {
    const char* vpath = std::getenv("RP_TRIANGLE_VERT_SPV");
    const char* fpath = std::getenv("RP_TRIANGLE_FRAG_SPV");
    if (!vpath || !fpath)
        return rp_fail(e, RP_ERR_INVALID_ARGUMENT, "Shader env vars not set (RP_TRIANGLE_VERT_SPV / RP_TRIANGLE_FRAG_SPV)");

    auto vert = read_file(vpath);
    auto frag = read_file(fpath);
    if (vert.empty() || frag.empty())
        return rp_fail(e, RP_ERR_INVALID_ARGUMENT, "Failed to read SPV shader files");

    VkShaderModuleCreateInfo smci{};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    VkShaderModule vmod = VK_NULL_HANDLE;
    smci.codeSize = vert.size();
    smci.pCode = reinterpret_cast<const uint32_t*>(vert.data());
    if (vkCreateShaderModule(e->device, &smci, nullptr, &vmod) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateShaderModule (vert) failed");

    VkShaderModule fmod = VK_NULL_HANDLE;
    smci.codeSize = frag.size();
    smci.pCode = reinterpret_cast<const uint32_t*>(frag.data());
    if (vkCreateShaderModule(e->device, &smci, nullptr, &fmod) != VK_SUCCESS) {
        vkDestroyShaderModule(e->device, vmod, nullptr);
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateShaderModule (frag) failed");
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vmod;
    stages[0].pName = "main";

    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fmod;
    stages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vi{};
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport vp{};
    vp.x = 0.0f; vp.y = 0.0f;
    vp.width = (float)e->swap_extent.width;
    vp.height = (float)e->swap_extent.height;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D sc{};
    sc.offset = {0,0};
    sc.extent = e->swap_extent;

    VkPipelineViewportStateCreateInfo vs{};
    vs.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vs.viewportCount = 1;
    vs.pViewports = &vp;
    vs.scissorCount = 1;
    vs.pScissors = &sc;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_BACK_BIT;
    rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rs.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cb_att{};
    cb_att.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cb{};
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.attachmentCount = 1;
    cb.pAttachments = &cb_att;

    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (vkCreatePipelineLayout(e->device, &plci, nullptr, &e->pipeline_layout) != VK_SUCCESS) {
        vkDestroyShaderModule(e->device, fmod, nullptr);
        vkDestroyShaderModule(e->device, vmod, nullptr);
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreatePipelineLayout failed");
    }

    VkGraphicsPipelineCreateInfo gpci{};
    gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.stageCount = 2;
    gpci.pStages = stages;
    gpci.pVertexInputState = &vi;
    gpci.pInputAssemblyState = &ia;
    gpci.pViewportState = &vs;
    gpci.pRasterizationState = &rs;
    gpci.pMultisampleState = &ms;
    gpci.pColorBlendState = &cb;
    gpci.layout = e->pipeline_layout;
    gpci.renderPass = e->render_pass;
    gpci.subpass = 0;

    VkResult vr = vkCreateGraphicsPipelines(e->device, VK_NULL_HANDLE, 1, &gpci, nullptr, &e->pipeline);

    vkDestroyShaderModule(e->device, fmod, nullptr);
    vkDestroyShaderModule(e->device, vmod, nullptr);

    if (vr != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateGraphicsPipelines failed");

    return RP_OK;
}

static rp_result_t record_cmds(rp_engine* e) {
    e->cmd_bufs.resize(e->swap_views.size());

    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = e->cmd_pool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = (uint32_t)e->cmd_bufs.size();

    if (vkAllocateCommandBuffers(e->device, &ai, e->cmd_bufs.data()) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkAllocateCommandBuffers failed");

    for (size_t i = 0; i < e->cmd_bufs.size(); ++i) {
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(e->cmd_bufs[i], &bi) != VK_SUCCESS)
            return rp_fail(e, RP_ERR_UNKNOWN, "vkBeginCommandBuffer failed");

        VkClearValue clear{};
        clear.color = {{0.05f, 0.05f, 0.08f, 1.0f}};

        VkRenderPassBeginInfo rbi{};
        rbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rbi.renderPass = e->render_pass;
        rbi.framebuffer = e->framebuffers[i];
        rbi.renderArea.offset = {0,0};
        rbi.renderArea.extent = e->swap_extent;
        rbi.clearValueCount = 1;
        rbi.pClearValues = &clear;

        vkCmdBeginRenderPass(e->cmd_bufs[i], &rbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(e->cmd_bufs[i], VK_PIPELINE_BIND_POINT_GRAPHICS, e->pipeline);
        vkCmdDraw(e->cmd_bufs[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(e->cmd_bufs[i]);

        if (vkEndCommandBuffer(e->cmd_bufs[i]) != VK_SUCCESS)
            return rp_fail(e, RP_ERR_UNKNOWN, "vkEndCommandBuffer failed");
    }

    return RP_OK;
}

static rp_result_t create_swapchain(rp_engine* e) {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(e->phys, e->surface, &caps);

    uint32_t fmt_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(e->phys, e->surface, &fmt_count, nullptr);
    if (fmt_count == 0) return rp_fail(e, RP_ERR_SWAPCHAIN_FAILED, "No surface formats");
    std::vector<VkSurfaceFormatKHR> formats(fmt_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(e->phys, e->surface, &fmt_count, formats.data());

    uint32_t pm_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(e->phys, e->surface, &pm_count, nullptr);
    if (pm_count == 0) return rp_fail(e, RP_ERR_SWAPCHAIN_FAILED, "No present modes");
    std::vector<VkPresentModeKHR> modes(pm_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(e->phys, e->surface, &pm_count, modes.data());

    VkSurfaceFormatKHR chosen_fmt = choose_format(formats);
    VkPresentModeKHR chosen_pm = choose_present_mode(modes);
    VkExtent2D extent = choose_extent(caps, e->width ? e->width : 1280, e->height ? e->height : 720);

    uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) image_count = caps.maxImageCount;

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = e->surface;
    sci.minImageCount = image_count;
    sci.imageFormat = chosen_fmt.format;
    sci.imageColorSpace = chosen_fmt.colorSpace;
    sci.imageExtent = extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t qfams[] = { e->gfx_qfam, e->present_qfam };
    if (e->gfx_qfam != e->present_qfam) {
        sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices = qfams;
    } else {
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = chosen_pm;
    sci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(e->device, &sci, nullptr, &e->swapchain) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_SWAPCHAIN_FAILED, "vkCreateSwapchainKHR failed");

    e->swap_format = chosen_fmt.format;
    e->swap_extent = extent;

    uint32_t img_count2 = 0;
    vkGetSwapchainImagesKHR(e->device, e->swapchain, &img_count2, nullptr);
    e->swap_images.resize(img_count2);
    vkGetSwapchainImagesKHR(e->device, e->swapchain, &img_count2, e->swap_images.data());

    e->swap_views.resize(e->swap_images.size());
    for (size_t i = 0; i < e->swap_images.size(); ++i) {
        VkImageViewCreateInfo iv{};
        iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv.image = e->swap_images[i];
        iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv.format = e->swap_format;
        iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv.subresourceRange.levelCount = 1;
        iv.subresourceRange.layerCount = 1;

        if (vkCreateImageView(e->device, &iv, nullptr, &e->swap_views[i]) != VK_SUCCESS)
            return rp_fail(e, RP_ERR_SWAPCHAIN_FAILED, "vkCreateImageView failed");
    }

    return RP_OK;
}

static rp_result_t create_framebuffers(rp_engine* e) {
    e->framebuffers.resize(e->swap_views.size());

    for (size_t i = 0; i < e->swap_views.size(); ++i) {
        VkImageView attachments[] = { e->swap_views[i] };

        VkFramebufferCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = e->render_pass;
        fci.attachmentCount = 1;
        fci.pAttachments = attachments;
        fci.width = e->swap_extent.width;
        fci.height = e->swap_extent.height;
        fci.layers = 1;

        if (vkCreateFramebuffer(e->device, &fci, nullptr, &e->framebuffers[i]) != VK_SUCCESS)
            return rp_fail(e, RP_ERR_UNKNOWN, "vkCreateFramebuffer failed");
    }
    return RP_OK;
}

static rp_result_t rebuild_swapchain(rp_engine* e) {
    vkDeviceWaitIdle(e->device);
    destroy_swapchain(e);

    auto r = create_swapchain(e);
    if (r != RP_OK) return r;

    r = create_render_pass(e);
    if (r != RP_OK) return r;

    r = create_pipeline(e);
    if (r != RP_OK) return r;

    r = create_framebuffers(e);
    if (r != RP_OK) return r;

    r = record_cmds(e);
    if (r != RP_OK) return r;

    e->needs_swapchain_rebuild = false;
    return RP_OK;
}

rp_result_t rp_engine_create(
    const rp_engine_desc_t* desc,
    const rp_native_window_t* window,
    rp_engine_t** out_engine)
{
    if (!desc || !out_engine) return RP_ERR_INVALID_ARGUMENT;
    *out_engine = nullptr;

    auto* e = new (std::nothrow) rp_engine();
    if (!e) return RP_ERR_UNKNOWN;

    e->logger = desc->logger;
    *out_engine = e;

    e->width = desc->width;
    e->height = desc->height;

    rp_log(e->logger, 1, "rp_engine_create: starting Vulkan instance creation...");

    std::vector<const char*> layers_to_enable;
    if (desc->enable_validation) {
        uint32_t layer_count = 0;
        if (vkEnumerateInstanceLayerProperties(&layer_count, nullptr) != VK_SUCCESS)
            return rp_fail(e, RP_ERR_VULKAN_INIT_FAILED, "vkEnumerateInstanceLayerProperties failed (count)");

        std::vector<VkLayerProperties> layers(layer_count);
        if (vkEnumerateInstanceLayerProperties(&layer_count, layers.data()) != VK_SUCCESS)
            return rp_fail(e, RP_ERR_VULKAN_INIT_FAILED, "vkEnumerateInstanceLayerProperties failed (data)");

        const char* kValidation = "VK_LAYER_KHRONOS_validation";
        if (has_layer(layers, kValidation)) {
            layers_to_enable.push_back(kValidation);
            rp_log(e->logger, 1, "Vulkan: enabling VK_LAYER_KHRONOS_validation");
        } else {
            rp_log(e->logger, 2, "Vulkan: validation requested but VK_LAYER_KHRONOS_validation not found");
        }
    }

    std::vector<const char*> exts_to_enable;
    exts_to_enable.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    exts_to_enable.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);

    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "red_plasma_engine";
    app.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app.pEngineName = "red_plasma";
    app.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &app;
    ci.enabledLayerCount = (uint32_t)layers_to_enable.size();
    ci.ppEnabledLayerNames = layers_to_enable.empty() ? nullptr : layers_to_enable.data();
    ci.enabledExtensionCount = (uint32_t)exts_to_enable.size();
    ci.ppEnabledExtensionNames = exts_to_enable.empty() ? nullptr : exts_to_enable.data();

    VkResult vr = vkCreateInstance(&ci, nullptr, &e->instance);
    if (vr != VK_SUCCESS || e->instance == VK_NULL_HANDLE)
        return rp_fail(e, RP_ERR_VULKAN_INIT_FAILED, "vkCreateInstance failed");

    rp_log(e->logger, 1, "Vulkan: instance created successfully");

    if (!window) return rp_fail(e, RP_ERR_INVALID_ARGUMENT, "Window is required for rendering");

    if (window->ws != RP_WS_WAYLAND)
        return rp_fail(e, RP_ERR_UNSUPPORTED_PLATFORM, "Expected Wayland window system");

    if (!window->handle.wayland.display || !window->handle.wayland.surface)
        return rp_fail(e, RP_ERR_INVALID_ARGUMENT, "Wayland display/surface is null");

    VkWaylandSurfaceCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    sci.display = static_cast<wl_display*>(window->handle.wayland.display);
    sci.surface = static_cast<wl_surface*>(window->handle.wayland.surface);

    vr = vkCreateWaylandSurfaceKHR(e->instance, &sci, nullptr, &e->surface);
    if (vr != VK_SUCCESS || e->surface == VK_NULL_HANDLE)
        return rp_fail(e, RP_ERR_SURFACE_FAILED, "vkCreateWaylandSurfaceKHR failed");

    rp_log(e->logger, 1, "Vulkan: Wayland surface created successfully");

    if (!pick_device_and_queues(e))
        return rp_fail(e, RP_ERR_VULKAN_INIT_FAILED, "No suitable Vulkan device found");

    auto r = create_device(e);
    if (r != RP_OK) return r;

    r = create_command_pool_and_sync(e);
    if (r != RP_OK) return r;

    r = rebuild_swapchain(e);
    if (r != RP_OK) return r;

    rp_log(e->logger, 1, "Vulkan: triangle pipeline ready");
    return RP_OK;
}

rp_result_t rp_engine_resize(rp_engine_t* engine, uint32_t w, uint32_t h) {
    if (!engine) return RP_ERR_INVALID_ARGUMENT;
    engine->width = w;
    engine->height = h;
    engine->needs_swapchain_rebuild = true;
    return RP_OK;
}

rp_result_t rp_engine_render(rp_engine_t* e) {
    if (!e) return RP_ERR_INVALID_ARGUMENT;
    if (!e->device || !e->swapchain) return RP_ERR_INVALID_ARGUMENT;

    if (e->needs_swapchain_rebuild) {
        auto r = rebuild_swapchain(e);
        if (r != RP_OK) return r;
    }

    vkWaitForFences(e->device, 1, &e->fence_inflight, VK_TRUE, UINT64_MAX);
    vkResetFences(e->device, 1, &e->fence_inflight);

    uint32_t image_index = 0;
    VkResult vr = vkAcquireNextImageKHR(e->device, e->swapchain, UINT64_MAX, e->sem_image_avail, VK_NULL_HANDLE, &image_index);

    if (vr == VK_ERROR_OUT_OF_DATE_KHR) {
        e->needs_swapchain_rebuild = true;
        return RP_OK;
    }
    if (vr != VK_SUCCESS && vr != VK_SUBOPTIMAL_KHR)
        return rp_fail(e, RP_ERR_SWAPCHAIN_FAILED, "vkAcquireNextImageKHR failed");

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.waitSemaphoreCount = 1;
    si.pWaitSemaphores = &e->sem_image_avail;
    si.pWaitDstStageMask = &wait_stage;
    si.commandBufferCount = 1;
    si.pCommandBuffers = &e->cmd_bufs[image_index];
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &e->sem_render_done;

    if (vkQueueSubmit(e->gfx_queue, 1, &si, e->fence_inflight) != VK_SUCCESS)
        return rp_fail(e, RP_ERR_UNKNOWN, "vkQueueSubmit failed");

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &e->sem_render_done;
    pi.swapchainCount = 1;
    pi.pSwapchains = &e->swapchain;
    pi.pImageIndices = &image_index;

    vr = vkQueuePresentKHR(e->present_queue, &pi);
    if (vr == VK_ERROR_OUT_OF_DATE_KHR || vr == VK_SUBOPTIMAL_KHR) {
        e->needs_swapchain_rebuild = true;
        return RP_OK;
    }
    if (vr != VK_SUCCESS)
        return rp_fail(e, RP_ERR_SWAPCHAIN_FAILED, "vkQueuePresentKHR failed");

    return RP_OK;
}

rp_result_t rp_engine_wait_idle(rp_engine_t* e) {
    if (!e) return RP_ERR_INVALID_ARGUMENT;
    if (e->device) vkDeviceWaitIdle(e->device);
    return RP_OK;
}

void rp_engine_destroy(rp_engine_t* e) {
    if (!e) return;

    if (e->device) vkDeviceWaitIdle(e->device);

    if (e->fence_inflight) vkDestroyFence(e->device, e->fence_inflight, nullptr);
    if (e->sem_render_done) vkDestroySemaphore(e->device, e->sem_render_done, nullptr);
    if (e->sem_image_avail) vkDestroySemaphore(e->device, e->sem_image_avail, nullptr);

    destroy_swapchain(e);

    if (e->cmd_pool) vkDestroyCommandPool(e->device, e->cmd_pool, nullptr);

    if (e->device) vkDestroyDevice(e->device, nullptr);

    if (e->surface && e->instance) vkDestroySurfaceKHR(e->instance, e->surface, nullptr);
    if (e->instance) vkDestroyInstance(e->instance, nullptr);

    delete e;
}

const char* rp_engine_last_error(const rp_engine_t* e) {
    if (!e) return "engine is null";
    return e->last_error.c_str();
}
