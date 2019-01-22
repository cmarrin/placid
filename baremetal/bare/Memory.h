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

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <bitset>

namespace bare {

    class Memory
    {
    public:
        class Heap;
        
        static constexpr uint32_t DefaultKernelHeapSize = 0x80000;
        static constexpr uint32_t DefaultPageSize = 4096;
        
        static void init(Heap* kernelHeap);
        
        static void* heapStart();
        static size_t heapSize();

        static bool mapSegment(size_t size, void*& addr)
        {
            return _kernelHeap ? _kernelHeap->mapSegment(size, addr) : false;
        }

        static int32_t unmapSegment(void* addr, size_t size)
        {
            return _kernelHeap ? _kernelHeap->unmapSegment(addr, size) : -1;
        }
        
        class Heap
        {
        public:
            friend class Memory;
            
            Heap() { }
            virtual ~Heap() { }
            
            virtual bool mapSegment(size_t size, void*&) = 0;
            virtual int32_t unmapSegment(void* addr, size_t size) = 0;
            
        private:
            void* _heapStart =  nullptr;
        };
        
        // KernelHeap
        //
        // User processes use a heap which allocates virtual memory pages
        // in their own process space. The kernel needs a "real" heap,
        // one that allocates real memory for use by things like 
        // translation tables and heap segment tables.
        template<uint32_t HeapSize, uint32_t PageSize> class KernelHeap : public Heap
        {
        public:
            KernelHeap() { _pageBitmap.reset(); }
            virtual ~KernelHeap() { }

            virtual bool mapSegment(size_t size, void*& addr) override
            {
                // Find a free area in the bitmap that is big enough
                uint32_t pages = (static_cast<uint32_t>(size) + PageSize - 1) / PageSize;
                uint32_t startBit;
                if (!findFreeBits(pages, startBit)) {
                    return false;
                }
                
                // Set the bits needed
                setFreeBits(startBit, pages);
                
                addr = reinterpret_cast<uint8_t*>(_heapStart) + (startBit * PageSize);
                return true;
            }
            
            virtual int32_t unmapSegment(void* addr, size_t size) override
            {
                // FIXME: Implement
                return -1;
            }

        private:
            bool findFreeBits(uint32_t pages, uint32_t& startBit)
            {
                if (pages == 0) {
                    return false;
                }
                
                uint32_t firstBit = 0;
                bool keepTrying = true;
                for (firstBit = 0; firstBit < _pageBitmap.size(); ++firstBit) {
                    if (!_pageBitmap[firstBit]) {
                        // found a bit, see if there are enough
                        if (pages == 1) {
                            keepTrying = false;
                            break;
                        }
                        
                        keepTrying = false;
                        for (uint32_t n = 1; n < pages; ++n) {
                            if (_pageBitmap[firstBit + n]) {
                                // not enough
                                firstBit += n + 1;
                                keepTrying = true;
                                break;
                            }
                        }
                        if (!keepTrying) {
                            break;
                        }
                    }
                }
                if (keepTrying) {
                    return false;
                }
                startBit = firstBit;
                return true;
            }
            
            void setFreeBits(uint32_t startBit, uint32_t pages)
            {
                for (uint32_t i = 0; i < pages; ++i) {
                    _pageBitmap[startBit + i] = true;
                }
            }
            
            static constexpr uint32_t PageBitmapSize = HeapSize / PageSize;
            typedef std::bitset<PageBitmapSize> PageBitmap;

            PageBitmap _pageBitmap;
        };
        
        static Heap* _kernelHeap;
    };
    
}

