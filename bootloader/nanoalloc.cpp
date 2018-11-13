/*-------------------------------------------------------------------------
This source file is a part of Placid

For the latest info, see http://www.marrin.org/

Copyright (c) 2018, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
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
    if (!bare::SystemIsInited) {
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
    if (!bare::SystemIsInited) {
        free(p);
    }
}

void operator delete [ ](void *p) noexcept
{
    if (!bare::SystemIsInited) {
        free(p);
    }
}

void operator delete(void *p, size_t size) noexcept
{
    if (!bare::SystemIsInited) {
        free(p);
    }
}

void operator delete [ ](void *p, size_t size) noexcept
{
    if (!bare::SystemIsInited) {
        free(p);
    }
}
