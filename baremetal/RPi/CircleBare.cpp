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

void bare::initSystem()
{
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

    bool interruptsSupported()
    {
        return true;
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

    void __aeabi_idiv0()
    {
        Serial::printf("Divide by zero\n");
        abort();
    }

    double __aeabi_l2d(long long a) { return intToFloat<double>(a); }
    float __aeabi_l2f(long long a) { return intToFloat<float>(a); }

    struct AbortFrame
    {
        uint32_t    sp_irq;
        uint32_t    lr_irq;
        uint32_t    r0;
        uint32_t    r1;
        uint32_t    r2;
        uint32_t    r3;
        uint32_t    r4;
        uint32_t    r5;
        uint32_t    r6;
        uint32_t    r7;
        uint32_t    r8;
        uint32_t    r9;
        uint32_t    r10;
        uint32_t    r11;
        uint32_t    r12;
        uint32_t    sp;
        uint32_t    lr;
        uint32_t    spsr;
        uint32_t    pc;
    };

    void handleException(uint32_t exception, AbortFrame* frame)
    {
        uint32_t FSR = 0;
        uint32_t FAR = 0;
        const char* source = "";
        
        switch (exception)
        {
        case EXCEPTION_DIVISION_BY_ZERO: source = "divide by zero"; break;
        case EXCEPTION_UNDEFINED_INSTRUCTION: source = "undefined instruction"; break;
        case EXCEPTION_PREFETCH_ABORT:
            asm volatile ("mrc p15, 0, %0, c5, c0,  1" : "=r" (FSR));
            asm volatile ("mrc p15, 0, %0, c6, c0,  2" : "=r" (FAR));
            source = "prefetch abort";
            break;

        case EXCEPTION_DATA_ABORT:
            asm volatile ("mrc p15, 0, %0, c5, c0,  0" : "=r" (FSR));
            asm volatile ("mrc p15, 0, %0, c6, c0,  0" : "=r" (FAR));
            source = "data abort";
            break;

        default:
            break;
        }

        assert (frame);
        uint32_t lr = frame->lr;
        uint32_t sp = frame->sp;

        // IRQ mode?
        if ((frame->spsr & 0x1F) == 0x12) {
            lr = frame->lr_irq;
            sp = frame->sp_irq;
        }
        
        RealTime currentTime = bare::Timer::currentTime();
        bare::Serial::printf("\n\n*** Panic(%s) %s:\n"
                             "        PC 0x%x\n"
                             "        FSR 0x%x\n"
                             "        FAR 0x%x\n"
                             "        SP 0x%x\n"
                             "        LR 0x%x\n"
                             "        PSR 0x%x\n\n", 
                             currentTime.timeString(bare::RealTime::TimeFormat::DateTime).c_str(), 
                             source, frame->pc, FSR, FAR, sp, lr, frame->spsr);
                        
        abort();
    }

    void handleSWI()
    {
        bare::Serial::printf("\n\n*** SWI Not Supported\n");
        abort();
    }

    void handleUnused()
    {
        bare::Serial::printf("\n\n*** Unused Vector Not Supported\n");
        abort();
    }

}
