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

#include <Arduino.h>

#include "bare.h"

#include "bare/Timer.h"

#include "bare/Serial.h"

#include <Ticker.h>
#include <unistd.h>
#include <sys/time.h>

using namespace bare;

static Ticker _systemTicker;
static uint32_t _lastMicros = 0;
static uint32_t _systemTimeOffset = 0;

void  Timer::init()
{
    _systemTicker.attach_ms(1000, []() {
        // Getting systemTime() kicks the overflow logic into action
        systemTime();
    });
}

void Timer::handleInterrupt()
{
}

void Timer::updateTimers()
{
}

int64_t Timer::systemTime()
{
    uint32_t t = micros();
    if (t < _lastMicros) {
        // Overflow - this logic assumes systemTime() will get called at least every twice every overflow (70 minutes)
        _systemTimeOffset++;
    }
    _lastMicros = t;
    return static_cast<int64_t>(_systemTimeOffset) << 32 + t;
}

void Timer::usleep(uint32_t us)
{
    delayMicroseconds(us);
}
