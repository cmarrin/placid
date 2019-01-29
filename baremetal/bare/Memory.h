/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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

