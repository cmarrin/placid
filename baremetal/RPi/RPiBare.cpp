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

#include "bare/Serial.h"

using namespace bare;

extern unsigned char __bss_start;
extern unsigned char _end;
extern void (*__init_start) (void);
extern void (*__init_end) (void);

extern "C" uint64_t __aeabi_idivmod(int value, int divisor);
extern "C" uint64_t __aeabi_uidivmod(int value, int divisor);

void bare::initSystem()
{
    for (unsigned char *pBSS = &__bss_start; pBSS < &_end; pBSS++)
    {
        *pBSS = 0;
    }

    // call construtors of static objects
    for (void (**pFunc) (void) = &__init_start; pFunc < &__init_end; pFunc++)
    {
        (**pFunc) ();
    }
}

bool bare::useAllocator()
{
    return true;
}

extern "C" {

    void* malloc(size_t size)
    {
        bare::Serial::printf("Attempted to malloc %d bytes\n", size);
        abort();
        return nullptr;
    }

    void free(void*)
    {
        bare::Serial::printf("Attempted to free\n");
        abort();
    }

    void disableIRQ()
    {
        __asm volatile (
            "mrs r0,cpsr\n"
            "orr r0,r0,#0x80\n"
            "msr cpsr_c,r0\n"
            "bx lr" : : : "r0"
        );
    }

    void enableIRQ()
    {
        __asm volatile (
            "mrs r0,cpsr\n"
            "bic r0,r0,#0x80\n"
            "msr cpsr_c,r0\n"
            "bx lr" : : : "r0"
        );
    }

    void WFE()
    {
        __asm volatile (
            "wfe\n"
            "bx lr"
        );
    }

    uint8_t* kernelBase() { return reinterpret_cast<uint8_t*>(0x8000); }

    int __aeabi_idiv(int value, int divisor)
    {
        return (int)__aeabi_idivmod(value, divisor);
    }

    void __aeabi_idiv0()
    {
        Serial::printf("Divide by zero\n");
        abort();
    }

}
