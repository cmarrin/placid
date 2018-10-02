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

namespace placid {

    // Virtual memory management
    //
    // Sets up MMU, allocates kernal translation tables, sets up user process
    // memory and handles page faults.
    //
    // This class may not be instantiated until the virtual memory system
    // is setup. So the init() method is static.
    //
    class Memory
    {
    public:
        enum class AP {
            Control = 0,            // permission determined by control reg bits S and R
            UserNoAccess = 1,
            UserReadOnly = 2,
            UserReadWrite = 3,
        };
            
        static void init();
        
    private:
        static constexpr uint32_t KernelTT = 0x4000;
        
        static void enableMMU();
        static void disableMMU();
        static void invalidateTLBs();
        static void invalidateCaches();
        static void setDomains(uint32_t domains);
        static void setMMUSections(uint32_t vaddr, uint32_t paddr, uint32_t num, AP, uint8_t domain, bool cacheable, bool bufferable);
        
        template<uint32_t which> static void setTTB(uint32_t addr)
        {
            static_assert(which < 2, "which variable must be 0 or 1");
            
            addr &= 0xffffc000;
            
            _kernelTTB[which] = addr;
            
        #ifndef __APPLE__
            __asm volatile (
                "mcr p15,0,%1,c2,c0,%0\n"
            :
            : "I" (which), "r" (addr)
            : "r0" );
        #endif
        }

        static uint32_t _kernelTTB[2];
    };
    
}

