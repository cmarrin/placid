/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cassert>
#include "bare/Mutex.h"

namespace placid {

    // Allocator
    //
    // Use the mapSegment API from Memory to manage a malloc-style memory
    // system.
    //
    // This allocator is informed by dlmalloc, Doug Lea's excellent allocator.
    // Why write another one from scratch then? Because that code suffers from
    // a variety of issues:
    //
    //      1) It is very complex, hard to read and hard to debug code in
    //      the "bare metal" environment I'm currently working in (i.e.,
    //      I couldn't get it to work and I couldn't debug it!)
    //      
    //      2) Written in C and being portable and adaptable makes it rife
    //      with ifdefs and macros. This makes it even harder to read and
    //      debug.
    //
    //      3) Many of the adaptations this allocator is famous for are built
    //      into the code. For instance, there are several locking schemes 
    //      available which are controlled by several ifdefs and macros. Why
    //      not just have a single set of macros that provide a locking 
    //      abstraction?
    //
    //      4) Another adaptation this allocator provides is the ability to 
    //      use sbrk, mmap and even mremap. Many ifdefs and macros are used
    //      provide a wide variety of combinations. The problem is that some
    //      of these choices infect the runtime code, potentially slowing
    //      performance. Modern systems favor mmap because of its ability to
    //      return discontiguous blocks to the system. Using that as the sole
    //      abstraction for getting memory from the system will simplify the
    //      design. And a mmap style interface can be simulated using sbrk
    //      or even with a static block of memory.
    //
    // The bottom line is that I think that a rewrite of this allocator,
    // using the same concepts but with more modern programming tools, like
    // C++, would produce a more flexible and easier to use allocator.
    //
    // I don't mean to disparage dlmalloc at all. It is great work. And the fact
    // that Doug Lea has given the work the most unencumbered license possible
    // (https://creativecommons.org/publicdomain/zero/1.0/) is a great gift to
    // the open source community (I'm looking at you, Richard Stallman).
    //
    class Allocator
    {
    public:
        Allocator();
        
        bool alloc(size_t size, void*&, size_t align = 0);
        void free(void *);
        
        uint32_t size() const { return _size; }
        
        static Allocator& kernelAllocator() { return _kernelAllocator; }

        static constexpr size_t MinAllocSize = 16 * sizeof(uintptr_t) / 4;
        static constexpr size_t MinSplitSize = 32;
        static constexpr size_t BlockSize = 4096;

        class Chunk
        {
        public:
            enum class Status { Free = 0, InUse = 1 };
            
            static constexpr size_t SizeMask = ~static_cast<size_t>(0x03);
            
            void setSize(size_t size) { assert((size & 0x03) == 0); _value &= 0x03; _value |= size & SizeMask; }
            size_t size() const { return _value & SizeMask; }
            void setStatus(Status status) { _value &= SizeMask; _value |= static_cast<size_t>(status); }
            Status status() const { return static_cast<Status>(_value & 0x03); }

        private:
            size_t _value = 0;
        };

        struct FreeChunk : public Chunk
        {
            FreeChunk* next;
            FreeChunk* prev;
        };

        static_assert(sizeof(FreeChunk) < MinAllocSize, "MinAllocSize too small");
        static_assert(sizeof(FreeChunk) < MinSplitSize, "MinSplitSize too small");

    private:
        void removeFromFreeList(FreeChunk*);
        void splitFreeBlock(FreeChunk*, size_t size);
        void addToFreeList(void*, size_t size);
        
        FreeChunk* _freeList = nullptr;
        
        static Allocator _kernelAllocator;
        
        uint32_t _size = 0;
        
        bare::Mutex _mutex;
    };
    
}

