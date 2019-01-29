/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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
