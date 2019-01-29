/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <stdint.h>

namespace bare {
    
    // Graphics - Interface to graphics subsystem
    //
    // This code was inspired bu the work here:
    //
    //        https://github.com/LdB-ECM/Raspberry-Pi
    //
    // This is a static class and cannot be instantiated
    //

    class Graphics {
    public:
        static bool init();
        static void clear(uint32_t color);
        
        static void drawTriangle();
        static void render();
    };
    
}
