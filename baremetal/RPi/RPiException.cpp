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

extern "C" {

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
                             "        PC  0x%08x   SP  0x%08x   LR  0x%08x\n"
                             "        FSR 0x%08x   FAR 0x%08x   PSR 0x%08x\n"
                             "\n"
                             "        R0-3  0x%08x 0x%08x 0x%08x 0x%08x\n"
                             "        R4-7  0x%08x 0x%08x 0x%08x 0x%08x\n"
                             "        R8-12 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n\n", 
                             currentTime.timeString(bare::RealTime::TimeFormat::DateTime).c_str(), source,
                             frame->pc, sp, lr,
                             FSR, FAR, frame->spsr,
                             frame->r0, frame->r1, frame->r2, frame->r3, 
                             frame->r4, frame->r5, frame->r6, frame->r7, 
                             frame->r8, frame->r9, frame->r10, frame->r11, frame->r12);
        abort();
    }

}
