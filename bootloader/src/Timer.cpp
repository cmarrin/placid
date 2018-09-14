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

#include "Timer.h"

#include "InterruptManager.h"
#include "Serial.h"

#ifdef __APPLE__
#include <unistd.h>
#endif

using namespace placid;

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

TimerCallback* Timer::_cb = nullptr;

static constexpr uint32_t ARMTimerBase = 0x2000B400;
static constexpr uint32_t SystemTimerBase = 0x20003000;
static constexpr float SystemTimerTick = 1e-6;

inline volatile ARMTimer& armTimer()
{
    return *(reinterpret_cast<volatile ARMTimer*>(ARMTimerBase));
}

inline volatile SystemTimer& systemTimer()
{
    return *(reinterpret_cast<volatile SystemTimer*>(SystemTimerBase));
}

volatile unsigned int icount;

void Timer::handleInterrupt()
{
	if (_cb) {
		_cb->handleTimerEvent();
	}
	armTimer().clearIRQ = 0;
}

void Timer::start(TimerCallback* cb, float seconds, bool /*repeat*/)
{
	_cb = cb;
	
#ifdef __APPLE__
    // FIXME: implement
    return;
#else
	uint32_t us = static_cast<uint32_t>(seconds * 1000000);

    InterruptManager::enableBasicIRQ(0, false);

	// Reset Free running prescaler
	armTimer().control = 0x003E0000;
	
	armTimer().load = us - 1;
	armTimer().reload = us - 1;
	
    InterruptManager::enableBasicIRQ(0, true);
	armTimer().clearIRQ = 0;
	
    icount=0;
    enableIRQ();

	// 32 bit counter, timer enabled, timer interrupt enabled
	armTimer().control = 0x003E00A2;
	armTimer().clearIRQ = 0;
#endif
}

void Timer::delay(float seconds)
{
    // Keep the number of ticks in a uint32_t, which gives us a limit 
    // of about 4000 seconds or 67 minutes. This way we can expand the counter into
    // a uint64_t to deal with overflow
    if (seconds > 4000) {
        seconds = 4000;
    }
    uint32_t ticks = static_cast<uint32_t>(seconds / SystemTimerTick + 0.5);
    if (ticks == 0) {
        return;
    }
    
#ifdef __APPLE__
    usleep(ticks);
#else
    uint32_t start = systemTimer().counter0;
    uint32_t end = start + ticks;
    
    // FIXME: Handle the wrapping case
    while (1) {
        uint64_t current = systemTimer().counter0;
        if (current >= end) {
            return;
        }
    }
#endif
}
