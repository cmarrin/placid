/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Timer.h"

#include "bare/InterruptManager.h"
#include "bare/Serial.h"

using namespace bare;

struct ARMTimer
{
	uint32_t load;
	uint32_t value;
	uint32_t control;
	uint32_t clearIRQ;
	uint32_t rawIRQ;
	uint32_t maskedIRQ;
	uint32_t reload;
	uint32_t divider;
	uint32_t freeRunningCounter;
};

// ARM Timer is in the Basic IRQ set, bit 0.
// InterruptManager considers Basic interrupts starting
// at 0 and the other 64 interrupt bits starting at 32.
static constexpr uint32_t ARMTimerInterruptBit = 0;

struct SystemTimer
{
    uint32_t control;
    uint32_t counter0;
    uint32_t counter1;
    uint32_t compare0;
    uint32_t compare1;
    uint32_t compare2;
    uint32_t compare3;
};

static constexpr uint32_t ARMTimerBase = 0x2000B400;
static constexpr uint32_t SystemTimerBase = 0x20003000;

inline volatile ARMTimer& armTimer()
{
    return *(reinterpret_cast<volatile ARMTimer*>(ARMTimerBase));
}

inline volatile SystemTimer& systemTimer()
{
    return *(reinterpret_cast<volatile SystemTimer*>(SystemTimerBase));
}

void  Timer::TimerManager::init()
{
    // We want a 1MHz tick. System clock is 250MHz, so we divide by 250
    // Set the prescaler (bits 16-23) to one less, or 0xf9
    armTimer().control = 0x00F90000; // Set the prescaler, but keep the timer stopped
    armTimer().control = 0x00F90200; // Now start the timer with the same prescaler value

    // Disable timer interrupts (until they are turned on by updateTimers) and set the handler
    InterruptManager::instance().enableIRQ(ARMTimerInterruptBit, false);
    InterruptManager::instance().addHandler(ARMTimerInterruptBit, handleInterrupt);
}

void Timer::TimerManager::updateTimers()
{
    // FIXME: For now we only support a single timer at the head of the list
    if (!_head) {
        return;
    }
    
    disableIRQ();
    InterruptManager::instance().enableIRQ(ARMTimerInterruptBit, false);

    // Reset Free running prescaler
    armTimer().control = 0x003E0000;
    
    armTimer().load = _head->_timeout - 1;
    armTimer().reload = _head->_timeout - 1;
    
    InterruptManager::instance().enableIRQ(ARMTimerInterruptBit, true);
    armTimer().clearIRQ = 0;
    
    enableIRQ();

    // 32 bit counter, timer enabled, timer interrupt enabled
    armTimer().control = 0x003E00A2;
    armTimer().clearIRQ = 0;
}

void Timer::handleInterrupt()
{
    Timer::TimerManager::instance().fireTimers();
	armTimer().clearIRQ = 0;
}

int64_t Timer::systemTime()
{
    return (static_cast<int64_t>(systemTimer().counter1) << 32) | static_cast<int64_t>(systemTimer().counter0);
}

void Timer::usleep(uint32_t us)
{
    int64_t t0 = systemTime();
    while(systemTime() < t0 + us) ;
}
