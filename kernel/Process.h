/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare/String.h"
#include <memory>

namespace placid {
    
    // Process - Client program

    class Process {
    public:
        static std::shared_ptr<Process> create(const bare::String& name);

        Process(const bare::String& name);

        void run();
    
    private:
        void* _startAddr = nullptr;
        void* _memory = nullptr;
        uint32_t _size = 0;
    };

}
