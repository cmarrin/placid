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

#include <sys/time.h>

using namespace bare;

void  Timer::TimerManager::init()
{
    // FIXME:: Implement
}

void Timer::handleInterrupt()
{
    // FIXME:: Implement
}

void Timer::TimerManager::updateTimers()
{
    // FIXME:: Implement
}

int64_t Timer::systemTime()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((static_cast<int64_t>(time.tv_sec)) * 1000000) + (static_cast<int64_t>(time.tv_usec));
}

void Timer::usleep(uint32_t us)
{
    int64_t t0 = systemTime();
    while(systemTime() < t0 + us) ;
}

