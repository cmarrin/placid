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

#include "stackAddrs.h"
#include <stddef.h>
#include <stdint.h>
#include <bitset>

namespace placid {

    // Virtual memory management
    //
    // Sets up MMU, allocates kernal translation tables, sets up user process
    // memory and handles page faults.
    //
    // This class may not be instantiated until the virtual memory system
    // is setup. So the init() method is static.
    //
    // Kernal occupies the first 1 MB of physical memory, so it occupies the
    // first TTB. This first TTB has a second level translation table which
    // maps 256 4KB pages. These pages are used as follows:
    //
    //      0           0x0000 - 0x0800     reserved for system use (interrupt vectors, ATAGS, etc.)
    //                  0x0800 - 0x0c00     second level translation table.
    //                  0x0c00 = 0x1000     TBD
    //
    //      1           0x1000 - 0x2000     IRQ/FIQ stack (2)
    //      2           0x2000 - 0x3000     ABORT stack (2)
    //      3           0x3000 - 0x4000     SWI stack (2)
    //      4 - 7       0x4000 - 0x8000     first level translation table
    //      8 - 128     0x8000 - 0x80000    kernel code, rodata and bss
    //      128 - 255   0x80000 - 0x100000  heap at the bottom and the SVC stack at the top (2)
    //
    //      1) The interrupt stacks occupy one 4096 byte page. If a page fault occurs for any, a 
    //      kernel panic is generated
    //
    //      2) SVC stack and heap occupy whole pages (4096 bytes) if a page fault occurs in either and
    //      the adjacent page is allocated to the other, a kernel panic is generated
    //
    static constexpr uint32_t IRQStack = _IRQStack;
    static constexpr uint32_t FIQStack = _FIQStack;
    static constexpr uint32_t AbortStack = _AbortStack;
    static constexpr uint32_t SVCStack = _SVCStack;

    static constexpr uint32_t FirstLevelTTB = 0x4000;
    static constexpr uint32_t SecondLevelTTB = 0xc00;

    static constexpr uint32_t KernelHeapStart = 0x80000;
    static constexpr uint32_t KernelHeapSize = SVCStack - KernelHeapStart;
    
    static constexpr uint32_t PageSize = 4096;

    class Memory
    {
    public:
        enum class TTType { Fault = 0, Page = 0b01, Section = 0b10 };
        
        enum class AP {
            Control = 0,            // permission determined by control reg bits S and R
            UserNoAccess = 1,
            UserReadOnly = 2,
            UserReadWrite = 3,
        };
        
        enum class Cacheable { No, Yes };
        enum class Bufferable { No, Yes };
            
        static void init();

        static bool mapSegment(size_t size, void*&);
        static int32_t unmapSegment(void* addr, size_t size);
        
    private:        
        static void enableMMU();
        static void disableMMU();
        static void invalidateTLBs();
        static void invalidateCaches();
        static void setDomains(uint32_t domains);
        static void setMMUSectionDescriptors(uint32_t ttb, uint32_t vaddr, uint32_t paddr, uint32_t num, AP, uint8_t domain, bool cacheable, bool bufferable);
        static void setMMUPageDescriptors(uint32_t ttb);
        
        template<uint32_t which> static void setTTB(uint32_t addr)
        {
            static_assert(which < 2, "which variable must be 0 or 1");
            
            addr &= 0xffffc000;

        #ifndef __APPLE__
            __asm volatile (
                "mcr p15,0,%1,c2,c0,%0\n"
            :
            : "I" (which), "r" (addr)
            : "r0" );
        #endif
        }
        
        class Heap
        {
        public:
            virtual bool mapSegment(size_t size, void*&) = 0;
            virtual int32_t unmapSegment(void* addr, size_t size) = 0;
        };
        
        // KernelHeap
        //
        // User processes use a heap which allocates virtual memory pages
        // in their own process space. The kernel needs a "real" heap,
        // one that allocates real memory for use by things like 
        // translation tables and heap segment tables.
        class KernelHeap : public Heap
        {
        public:
            KernelHeap() { _pageBitmap.reset(); }
        
            virtual bool mapSegment(size_t size, void*&) override;
            virtual int32_t unmapSegment(void* addr, size_t size) override;

        private:
            static constexpr uint32_t PageBitmapSize = (KernelHeapSize / PageSize + 7) / 8;
            typedef std::bitset<PageBitmapSize> PageBitmap;

            bool findFreeBits(uint32_t pages, uint32_t& startBit);
            void setFreeBits(uint32_t startBit, uint32_t pages);
            
            PageBitmap _pageBitmap;
        };
        
        static KernelHeap _kernelHeap;
    };
    
}

