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

// Test kernel heap memory
uint8_t _kernelHeapMemory[Memory::DefaultKernelHeapSize];

static constexpr uint32_t MinHeapSize = 0x1000;

Memory::Heap* Memory::_kernelHeap = nullptr;

void* Memory::heapStart()
{
    return _kernelHeapMemory;
}

size_t Memory::heapSize()
{
    return MinHeapSize;
}

void Memory::init(Heap* kernelHeap)
{
    _kernelHeap = kernelHeap;
    _kernelHeap->_heapStart = _kernelHeapMemory;
}
