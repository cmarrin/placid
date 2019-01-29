/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare/Singleton.h"
#include "bare/String.h"
#include "Process.h"
#include <memory>

namespace placid {
    
    // Dispatcher - Manages Processes

    class Dispatcher : public Singleton<Dispatcher> {
    public:
        void exec(const bare::String& name);

    private:
        std::vector<std::shared_ptr<Process>> _processes;
    };
    
}
