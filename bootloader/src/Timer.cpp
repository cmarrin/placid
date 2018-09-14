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

TimerCallback* Timer::_cb = nullptr;

static constexpr uint32_t timerBase = 0x2000B400;
	
inline volatile ARMTimer& armTimer()
{
	return *(reinterpret_cast<volatile ARMTimer*>(timerBase));
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
	
	uint32_t us = static_cast<uint32_t>(seconds * 1000000);

    InterruptManager::enableBasicIRQ(1, false);

	// Reset Free running prescaler
	armTimer().control = 0x003E0000;
	
	armTimer().load = us - 1;
	armTimer().reload = us - 1;
	
    InterruptManager::enableBasicIRQ(1, true);
	armTimer().clearIRQ = 0;
	
    icount=0;
    enableIRQ();

	// 32 bit counter, timer enabled, timer interrupt enabled
	armTimer().control = 0x003E00A2;
	armTimer().clearIRQ = 0;
}
