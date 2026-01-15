// /*
//  * Red Plasma Engine
//  * Copyright (C) 2026  Kim Johansson
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation...
//  *

//
// Created by Dueloss on 12.01.2026.
//

#include "../include/VulkanGraphicsDevice.h"

#include <algorithm>

#include "../include/IWindowSurface.h"
#include <iostream>
#include <vector>

#include "VulkanWindowSurface.h"

namespace RedPlasma {
    struct QueueFamilyIndices {
        int graphicsFamily = -1;
        [[nodiscard]] bool isComplete() const {
            return graphicsFamily >= 0;
        }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    int VulkanGraphicsDevice::InitializeDevice(IWindowSurface* surface) {
        if (!surface) {
            return -1;
        }

        auto vkSurface = static_cast<VkSurfaceKHR>(dynamic_cast<VulkanWindowSurface*>(surface)->GetSurfaceHandle());

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, m_graphicsFamilyIndex, vkSurface, &presentSupport);
        if (!presentSupport) {
            return -3;
        }

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = m_graphicsFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS) {
            return -2;
        }

        vkGetDeviceQueue(m_LogicalDevice, m_graphicsFamilyIndex, 0, &m_GraphicsQueue);

        return setupSwapchain(surface);
    }

    int VulkanGraphicsDevice::setupSwapchain(IWindowSurface* surface) {
        auto* vulkanSurface = dynamic_cast<VulkanWindowSurface*>(surface);
        auto vkSurface = static_cast<VkSurfaceKHR>(dynamic_cast<VulkanWindowSurface*>(vulkanSurface)->GetSurfaceHandle());

        VkSurfaceCapabilitiesKHR capabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, vkSurface, &capabilities) != VK_SUCCESS) {
            return -5;
        }

        VkExtent2D swapchainExtent;

        if (capabilities.currentExtent.width != UINT32_MAX) {
            swapchainExtent = capabilities.currentExtent;
        } else {
            auto width = static_cast<uint32_t>(vulkanSurface->GetWidth());
            auto height = static_cast<uint32_t>(vulkanSurface->GetHeight());

            swapchainExtent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            swapchainExtent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        }

        return 0; // Success
    }

    VulkanGraphicsDevice::VulkanGraphicsDevice() {

    }

    VulkanGraphicsDevice::~VulkanGraphicsDevice() {

    }

    int VulkanGraphicsDevice::Initialize() {

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Red Plasma Engine";
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(m_EnableExtension.size());
        createInfo.ppEnabledExtensionNames = m_EnableExtension.data();

        if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
            return -1;
        }

        uint32_t devicesCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &devicesCount, nullptr);

        if (devicesCount == 0) {
            std::cout << "No Vulkan instance found!" << std::endl;
            return -2;
        }

        std::vector<VkPhysicalDevice> devices(devicesCount);
        vkEnumeratePhysicalDevices(m_Instance, &devicesCount, devices.data());

        m_PhysicalDevice = devices[0];

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &deviceProperties);
        m_DeviceName = deviceProperties.deviceName;

        std::cout << "Vulkan instance name: " << m_DeviceName << std::endl;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

        QueueFamilyIndices indices;

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
            i++;
        }

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createDeviceInfo {};
        createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createDeviceInfo.pQueueCreateInfos = &queueCreateInfo;
        createDeviceInfo.queueCreateInfoCount = 1;
        createDeviceInfo.pEnabledFeatures = &deviceFeatures;

        if (vkCreateDevice(m_PhysicalDevice, &createDeviceInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS) {
            std::cout << "Vulkan: Failed to create logical device!" << std::endl;
            return -3;
        }

        std::cout << "Vulkan: Logical device created successfully!" << std::endl;

        vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily, 0, &m_GraphicsQueue);
        return 0;
    }

    int VulkanGraphicsDevice::Shutdown() {
        std::cout << "Vulkan:: Starting graceful shutdown..." << std::endl;
        if (m_LogicalDevice != VK_NULL_HANDLE) {
            vkDestroyDevice(m_LogicalDevice, nullptr);
            m_LogicalDevice = VK_NULL_HANDLE;
            std::cout << "Vulkan:: Destroying logical device." << std::endl;
        }

        if (m_Surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_Instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
            std::cout << "Vulkan:: Destroying Instance." << std::endl;
        }
        return 0;
    }

    int VulkanGraphicsDevice::UploadMeshData(const std::vector<Vertex> &vertices) {
        return 0;
    }

    int VulkanGraphicsDevice::DrawFrame() {
        return 0;
    }

    const char * VulkanGraphicsDevice::GetDeviceName() {
        return m_DeviceName;
    }

    int VulkanGraphicsDevice::CreateSurface(IWindowSurface* windowHandle) {
        m_Surface = windowHandle->CreateSurface(m_Instance);

        if (m_Surface == VK_NULL_HANDLE) {
            return -1;
        }
        return 0;
    }

    void VulkanGraphicsDevice::AddExtension(const std::vector<const char*> &extensions) {
        for (auto extension : extensions) {
            m_EnableExtension.push_back(extension);
        }
    }
} // RedPlasma