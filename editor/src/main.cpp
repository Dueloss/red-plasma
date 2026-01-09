#include <red_plasma/engine/engine_api.h>

#include <cstdlib>
#include <cstdio>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <GLFW/glfw3native.h>

static void log_cb(void*, int level, const char* msg)
{
    std::printf("[engine][%d] %s\n", level, msg);
}

int main()
{
    if (!glfwInit()) {
        std::fprintf(stderr, "GLFW init failed\n");
        return 1;
    }

    // Vulkan engine owns rendering; GLFW must not create OpenGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    GLFWwindow* window =
    glfwCreateWindow(1280, 720, "red_plasma editor (Wayland test)", nullptr, nullptr);

    if (!window) {
        std::fprintf(stderr, "GLFW window creation failed\n");
        glfwTerminate();
        return 1;
    }

    // ---- Wayland native handles ----
    wl_display* display = glfwGetWaylandDisplay();
    wl_surface* surface = glfwGetWaylandWindow(window);

    if (!display || !surface) {
        std::fprintf(stderr, "Failed to get Wayland handles\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    setenv("RP_TRIANGLE_VERT_SPV", RP_TRIANGLE_VERT_SPV, 1);
    setenv("RP_TRIANGLE_FRAG_SPV", RP_TRIANGLE_FRAG_SPV, 1);

    rp_engine_desc_t desc{};
    desc.logger.callback = log_cb;
    desc.logger.user = nullptr;
    desc.logger.min_level = 0;
    desc.width = 1280;
    desc.height = 720;
    desc.enable_validation = 1;
    desc.triangle_vert_spv_path = RP_TRIANGLE_VERT_SPV;
    desc.triangle_frag_spv_path = RP_TRIANGLE_FRAG_SPV;

    rp_native_window_t wnd{};
    wnd.ws = RP_WS_WAYLAND;
    wnd.handle.wayland.display = display;
    wnd.handle.wayland.surface = surface;

    rp_engine_t* engine = nullptr;
    rp_result_t r = rp_engine_create(&desc, &wnd, &engine);

    std::printf("rp_engine_create result = %d\n", (int)r);

    if (r != RP_OK) {
        std::printf("last_error: %s\n",
                    engine ? rp_engine_last_error(engine) : "engine is null");
        rp_engine_destroy(engine);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // ---- Main loop (no triangle yet, engine stub is fine) ----
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        rp_engine_render(engine);
    }

    rp_engine_wait_idle(engine);
    rp_engine_destroy(engine);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
