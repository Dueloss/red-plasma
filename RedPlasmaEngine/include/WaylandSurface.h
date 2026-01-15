// /*
//  * Red Plasma Engine
//  * Copyright (C) 2026  Kim Johansson
//  *
//  * This program is free software: you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License as published by
//  * the Free Software Foundation...
//  *

//
// Created by Dueloss on 14.01.2026.
//

#ifndef REDPLASMA_WAYLANDSURFACE_H
#define REDPLASMA_WAYLANDSURFACE_H
#include "IWindowSurface.h"
#include <vulkan/vulkan_wayland.h>

namespace RedPlasma {
    class WaylandSurface : public IWindowSurface {
    public:
        WaylandSurface(void* display, void* window) : m_display(display), m_window(window) {}

        ~WaylandSurface() override = default;

        [[nodiscard]] std::vector<const char*> GetRequiredExtensions() const override {
            return {
                VK_KHR_SURFACE_EXTENSION_NAME,
                VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
            };
        }
        VkSurfaceKHR CreateSurface(VkInstance instance) override {
            VkWaylandSurfaceCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
            createInfo.display = static_cast<struct wl_display*>(m_display);
            createInfo.surface = static_cast<struct wl_surface*>(m_window);

            VkSurfaceKHR surface;
            vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface);
            return surface;
        }
        [[nodiscard]] void* GetSurfaceHandle() override { return m_window; }

        void UpdateSize(int w, int h) {
            m_width = w;
            m_height = h;
        }

        [[nodiscard]] int GetWidth() const override { return m_width; }
        [[nodiscard]] int GetHeight() const override { return m_height; }

    private:
        void* m_display;
        void* m_window;
        int m_width = 800;
        int m_height = 600;
    };
}
#endif //REDPLASMA_WAYLANDSURFACE_H