/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

namespace bare {

    class Mutex
    {
    public:
        Mutex();
        ~Mutex();
        
        void lock();
        bool try_lock();
        void unlock();
    
    private:
        void* _mutex = nullptr;
    };
    
}
