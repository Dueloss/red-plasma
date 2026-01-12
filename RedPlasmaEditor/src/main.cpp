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

#include <iostream>
#include "Engine.h"

int main() {
    std::cout << "[Editor] Red Plasma Engine: Starting..." << std::endl;

    RedPlasma::Engine engine;

    engine.Run();

    std::cout << "[Editor] Red Plasma Engine: Closing..." << std::endl;
}