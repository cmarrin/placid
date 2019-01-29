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
#include "bare/Serial.h"
#include <cstdint>
#include <cstdlib>

// Simple allocator for bootloader.
//
// Allocates from a block of memory above the end of .bss and below
// 0x8000. Delete does nothing, so there is no recovery of freed memory

//#define DEBUG_NANOALLOC

static void* alloc(size_t size)
{
    if (!bare::useAllocator()) {
        return malloc(size);
    }
    
    static uint32_t* HeapPtr = nullptr;
    if (!HeapPtr) {
        HeapPtr = reinterpret_cast<uint32_t*>(bare::Memory::heapStart());
    }
    uint32_t* r = HeapPtr;
    HeapPtr += (size + sizeof(uint32_t) - 1) / sizeof(uint32_t);
    if (HeapPtr > reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(bare::Memory::heapStart()) + bare::Memory::heapSize())) {
        bare::Serial::printf("Nano Heap ran out of memory\n");
        return nullptr;
    }
    
#ifdef DEBUG_NANOALLOC
    bare::Serial::printf("Alloc %d bytes at 0x%08x, new free start 0x%08x\n", size, 
                    static_cast<uint32_t>(reinterpret_cast<uint64_t>(r)), 
                    static_cast<uint32_t>(reinterpret_cast<uint64_t>(HeapPtr)));
#endif
    return r;
}

void *operator new(size_t size)
{
    return alloc(size);
}

void *operator new[] (size_t size)
{
    return alloc(size);
}

void operator delete(void *p) noexcept
{
    if (!bare::useAllocator()) {
        free(p);
    }
}

void operator delete [ ](void *p) noexcept
{
    if (!bare::useAllocator()) {
        free(p);
    }
}

void operator delete(void *p, size_t size) noexcept
{
    if (!bare::useAllocator()) {
        free(p);
    }
}

void operator delete [ ](void *p, size_t size) noexcept
{
    if (!bare::useAllocator()) {
        free(p);
    }
}
