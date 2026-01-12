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

#ifndef REDPLASMA_IWINDOWSURFACE_H
#define REDPLASMA_IWINDOWSURFACE_H
namespace RedPlasma {
    class IWindowSurface {
    public:
        virtual ~IWindowSurface() = default;

        virtual int CreateSurface() = 0;
    };
}
#endif //REDPLASMA_IWINDOWSURFACE_H