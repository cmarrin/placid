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

std::shared_ptr<Timer> Timer::create(Handler handler)
{
    struct MakeSharedEnabler : public Timer
    {
        MakeSharedEnabler(Handler handler) : Timer(handler) { }
    };
    
    std::shared_ptr<MakeSharedEnabler> timer = std::make_shared<MakeSharedEnabler>(handler);
    TimerManager::instance().add(timer);
    return timer;
}

void Timer::TimerManager::fireTimers()
{
    int64_t currentTime = systemTime();
    for (std::shared_ptr<Timer> timer : _timers) {
        if (timer->_timeToFire <= currentTime) {
            timer->_handler(timer);
            timer->_timeToFire = timer->_repeat ? (currentTime + timer->_timeout) : DoNotFire;
        } else {
            break;
        }
    }
    TimerManager::instance().sortTimers();
    TimerManager::instance().updateTimers();
}

void Timer::TimerManager::setCurrentTime(const RealTime& t)
{
    _epochOffset = t.usSinceEpoch() - systemTime();
}

RealTime Timer::TimerManager::currentTime()
{
    return RealTime(_epochOffset + Timer::systemTime());
}
