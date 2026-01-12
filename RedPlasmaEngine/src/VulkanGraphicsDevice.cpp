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
#include <iostream>
#include <vector>
struct QueueFamilyIndices {
  int graphicsFamily = -1;
    bool  isComplete() {
        return graphicsFamily >= 0;
    }
};
namespace RedPlasma {
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
        }
        return 0;
    }

    int VulkanGraphicsDevice::Shutdown() {
        std::cout << "Vulkan:: Starting graceful shutdown..." << std::endl;
        if (m_LogicalDevice != VK_NULL_HANDLE) {
            vkDestroyDevice(m_LogicalDevice, nullptr);
            m_LogicalDevice = VK_NULL_HANDLE;
            std::cout << "Vulkan:: Destroying logical device." << std::endl;
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
} // RedPlasma