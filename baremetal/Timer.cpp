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
#include "util.h"

#ifdef __APPLE__
#include <unistd.h>
#include <sys/time.h>
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
int64_t Timer::_epochOffset = 0;

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

volatile unsigned int icount;

void  Timer::init()
{
#ifndef __APPLE__
    // We want a 1MHz tick. System clock is 250MHz, so we divide by 250
    // Set the prescaler (bits 16-23) to one less, or 0xf9
    armTimer().control = 0x00F90000; // Set the prescaler, but keep the timer stopped
    armTimer().control = 0x00F90200; // Now start the timer with the same prescaler value
#endif
}

void Timer::handleInterrupt()
{
    if (!interruptsSupported()) {
        return;
    }
    
	if (_cb) {
		_cb->handleTimerEvent();
	}
	armTimer().clearIRQ = 0;
}

void Timer::start(TimerCallback* cb, float seconds, bool /*repeat*/)
{
    if (!interruptsSupported()) {
        return;
    }
    
	_cb = cb;
	
#ifdef __APPLE__
    // FIXME: implement
    return;
#else
	uint32_t us = static_cast<uint32_t>(seconds * 1000000);

    disableIRQ();
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

void Timer::usleep(uint32_t us)
{
    int64_t t0 = Timer::systemTime();
    while(Timer::systemTime() < t0 + us) ;
}

int64_t Timer::systemTime()
{
#ifdef __APPLE__
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((static_cast<int64_t>(time.tv_sec)) * 1000000) + (static_cast<int64_t>(time.tv_usec));
#else
    return (static_cast<int64_t>(systemTimer().counter1) << 32) | static_cast<int64_t>(systemTimer().counter0);
#endif
}

void Timer::setCurrentTime(const RealTime& t)
{
    _epochOffset = t.usSinceEpoch() - systemTime();
}

RealTime Timer::currentTime()
{
    return RealTime(_epochOffset + systemTime());
}

// This returns the number of days from 0001-01-01
static int32_t rataDie(uint32_t y, uint8_t m, uint8_t d)
{
    // Rata Die day one is 0001-01-01
    if (m < 3) {
        y -= 1;
        m += 12;
    }
    return 365 * y + y / 4 - y / 100 + y / 400 + (153 * m - 457)/5 + d - 306;
}

RealTime::RealTime(int32_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, uint32_t us)
{
    _time = static_cast<int64_t>(rataDie(year, month, day));
    _time *= 24 * 60 * 60;
    _time += hour * 60 * 60 + minute * 60 + second;
    _time *= 1000000;
}

static void toDate(int32_t& year, uint8_t& month, uint8_t& day, int64_t time)
{
    int32_t days = time / 1000000 / 60 / 60 / 24;
    int32_t z = days + 306;
    int32_t h = 100 * z - 25;
    int32_t a = h / 3652425;
    int32_t b = a - (a / 4);
    year = static_cast<int32_t>((100 * b + h) / 36525);
    int32_t c = b + z - 365 * year - year / 4;
    month = (5 * c + 456) / 153;
    
    static constexpr uint16_t daysInMonth[] = { 0, 31, 61, 92, 122, 153, 184, 214, 245, 275, 306, 337 };
    int32_t index = month - 3;
    day = c - ((index < 0 || index >= 12) ? 0 : daysInMonth[index]);
    if (month > 12) {
        year += 1;
        month -= 12;
    }
}

uint8_t RealTime::day() const
{
    int32_t year;
    uint8_t month;
    uint8_t day;
    toDate(year, month, day, _time);
    return day;
}

uint8_t RealTime::month() const
{
    int32_t year;
    uint8_t month;
    uint8_t day;
    toDate(year, month, day, _time);
    return month;
}

int32_t RealTime::year() const
{
    int32_t year;
    uint8_t month;
    uint8_t day;
    toDate(year, month, day, _time);
    return year;
}

uint8_t RealTime::dayOfWeek() const
{
    int64_t days = _time / 1000000 / 60 / 60 / 24;
    return days % 7;
}
