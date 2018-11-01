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

#include "Allocator.h"

#include "Serial.h"

using namespace placid;

static inline bool checkFreeList(Allocator::FreeChunk* list)
{
    if (list) {
        assert(list->prev == nullptr);
    }
    while(list) {
        assert(list->status() == Allocator::FreeChunk::Status::Free);
        if (list->next) {
            assert(list->next->prev == list);
        }
        list = list->next;
    }
    return true;
}

Allocator Allocator::_kernelAllocator;

#ifndef __APPLE__
extern "C" void free(void* p) { Allocator::kernelAllocator().free(p); }
extern "C" void* malloc(size_t s) { void* m; return Allocator::kernelAllocator().alloc(s, m) ? m : nullptr; }
#endif

Allocator::Allocator()
{
}

void Allocator::removeFromFreeList(FreeChunk* chunk)
{
    assert(checkFreeList(_freeList));
    if (chunk == _freeList) {
        _freeList = chunk->next;
        if (_freeList) {
            _freeList->prev = nullptr;
        }
    } else {
        chunk->prev->next = chunk->next;
        if (chunk->next) {
            chunk->next->prev = chunk->prev;
        }
    }
    assert(checkFreeList(_freeList));
}

void Allocator::splitFreeBlock(FreeChunk* chunk, size_t size)
{
    // Remove a size bytes section of the passed block from the free list
    // and leave the rest
    removeFromFreeList(chunk);
    FreeChunk* newFreeChunk = reinterpret_cast<FreeChunk*>(reinterpret_cast<uint8_t*>(chunk) + size);
    addToFreeList(newFreeChunk, chunk->size() - size);
}

void Allocator::addToFreeList(void* mem, size_t size)
{
    // FIXME: For now add to the head, but there might be better approaches
    assert(checkFreeList(_freeList));
    FreeChunk* chunk = reinterpret_cast<FreeChunk*>(mem);
    chunk->setSize(size);
    chunk->setStatus(Chunk::Status::Free);
    if (_freeList) {
        _freeList->prev = chunk;
    }
    chunk->next = _freeList;
    chunk->prev = nullptr;
    _freeList = chunk;
    assert(checkFreeList(_freeList));
}

bool Allocator::alloc(size_t size, void*& mem)
{
#ifdef __APPLE__
    if (!_useAllocator) {
        mem = ::malloc(size);
        return mem;
    }
#endif

    size = (size + sizeof(Chunk) + MinAllocSize - 1) / MinAllocSize * MinAllocSize;
    
    // Try to find a block in the free list
    FreeChunk* entry = _freeList;
    for ( ; entry; entry = entry->next) {
        if (entry->size() >= size) {
            break;
        }
    }
    
    if (entry) {
        // Should we split or just use it?
        if (size + MinSplitSize > entry->size()) {
            removeFromFreeList(entry);
        } else {
            splitFreeBlock(entry, size);
        }
    } else {
        // No free entry found, alloc a new block
        // Allocate as much as needed, in multiples of BlockSize
        size_t sizeToAlloc = (size + BlockSize - 1) / BlockSize * BlockSize;
        void* newSegment;
        if (!Memory::mapSegment(sizeToAlloc, newSegment)) {
            return false;
        }
        
        entry = reinterpret_cast<FreeChunk*>(newSegment);
        size_t newSize = BlockSize;
        if (size + MinSplitSize < BlockSize) {
            addToFreeList(reinterpret_cast<uint8_t*>(entry) + size, sizeToAlloc - size);
        } else {
            size = newSize;
        }
    }
           
    entry->setSize(size);
    entry->setStatus(Chunk::Status::InUse);
    _size += size;
    
    // return the part of the block past the header
    mem = reinterpret_cast<Chunk*>(entry) + 1;
    return true;
}

void Allocator::free(void *addr)
{
#ifdef __APPLE__
    if (!_useAllocator) {
        ::free(addr);
        return;
    }
#endif

    Chunk* chunk = reinterpret_cast<Chunk*>(addr) - 1;
    addToFreeList(chunk, chunk->size());
    _size -= chunk->size();
}

void *operator new(size_t size)
{
    void* mem;
    void* result = Allocator::kernelAllocator().alloc(size, mem) ? mem : nullptr;
    return result;
}

void *operator new[] (size_t size)
{
    void* mem;
    return Allocator::kernelAllocator().alloc(size, mem) ? mem : nullptr;
}

void operator delete(void *p) noexcept
{
    Allocator::kernelAllocator().free(p);
}

void operator delete [ ](void *p) noexcept
{
    Allocator::kernelAllocator().free(p);
}

void operator delete(void *p, size_t size) noexcept
{
    Allocator::kernelAllocator().free(p);
}

void operator delete [ ](void *p, size_t size) noexcept
{
    Allocator::kernelAllocator().free(p);
}
