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

#ifndef REDPLASMA_IGRAPHICSDEVICE_H
#define REDPLASMA_IGRAPHICSDEVICE_H
#include <vector>

namespace RedPlasma {

    struct Vertex {
        float x, y, z;
    };

    class IGraphicsDevice {
    public:
        virtual ~IGraphicsDevice() = default;

        virtual int Initialize() = 0;
        virtual int Shutdown() = 0;

        virtual int UploadMeshData(const std::vector<Vertex>& vertices) = 0;

        virtual int DrawFrame() = 0;

        virtual const char* GetDeviceName() = 0;
    };
}
#endif //REDPLASMA_IGRAPHICSDEVICE_H