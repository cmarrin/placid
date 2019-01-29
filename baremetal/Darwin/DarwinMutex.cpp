/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Mutex.h"

#include <mutex>

using namespace bare;

Mutex::Mutex() : _mutex(new std::mutex())
{
}

Mutex::~Mutex()
{
    delete reinterpret_cast<std::mutex*>(_mutex);
}

void Mutex::lock()
{
    reinterpret_cast<std::mutex*>(_mutex)->lock();
}

void Mutex::unlock()
{
    reinterpret_cast<std::mutex*>(_mutex)->unlock();
}

bool Mutex::try_lock()
{
    return reinterpret_cast<std::mutex*>(_mutex)->try_lock();
}
