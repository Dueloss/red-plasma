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
#include "Engine.h"
#include <iostream>

#include "VulkanGraphicsDevice.h"

namespace RedPlasma {
    Engine::Engine() {
        std::cout << "Red Plasma Engine: Initializing..." << std::endl;
        m_GraphicsDevice = new VulkanGraphicsDevice();
        if (m_GraphicsDevice->Initialize() == 0) {
            m_IsRunning = true;
        }

    }

    Engine::~Engine() {
        std::cout << "Red Plasma Engine: Shutting down..." << std::endl;
        if (m_GraphicsDevice) {
            m_GraphicsDevice->Shutdown();
            delete m_GraphicsDevice;
            m_GraphicsDevice = nullptr;
        }
    }

    void Engine::Run() {
        if (m_IsRunning) {
            std::cout << "Red Plasma Engine: Running..." << std::endl;
        }

    }
}
