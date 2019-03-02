/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Serial.h"
#include "bare/Timer.h"
#include "asmInterface.h"
#include <cassert>

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

bool bare::interruptsSupported()
{
    return true;
}

bool bare::useAllocator()
{
    return true;
}

template<typename T>
T intToFloat(long long a)
{
    bool sign = false;
    if (a < 0) {
        sign = true;
        a = -a;
    }
    
    T f = 0;
    T multiplier = 1;
    
    while (a) {
        T digit = 0.0;
        switch(a % 10) {
        case 1: digit = 1.0; break;
        case 2: digit = 2.0; break;
        case 3: digit = 3.0; break;
        case 4: digit = 4.0; break;
        case 5: digit = 5.0; break;
        case 6: digit = 6.0; break;
        case 7: digit = 7.0; break;
        case 8: digit = 8.0; break;
        case 9: digit = 9.0; break;
        default: break;
        }
        
        f += digit * multiplier;
        a /= 10;
        multiplier *= 10;
    }
    
    return sign ? -f : f;
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

    double __aeabi_l2d(long long a) { return intToFloat<double>(a); }
    float __aeabi_l2f(long long a) { return intToFloat<float>(a); }

}
