/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Memory.h"

#include "bare/Timer.h"
#include <cstdlib>

using namespace bare;

extern uint8_t _end;
static void* KernelHeapStart = &_end;

void* Memory::heapStart()
{
    return KernelHeapStart;
}

size_t Memory::heapSize()
{
    return reinterpret_cast<uint8_t*>(bare::kernelBase()) - &_end;
}
