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

void Timer::TimerManager::start(Timer* timer, uint32_t us, bool repeat)
{
    if (!interruptsSupported()) {
        return;
    }
    
    timer->_next = _head;
    timer->_timeout = us;
    timer->_repeat = repeat;
    _head = timer;
    
    updateTimers();
}

void Timer::TimerManager::stop(Timer*)
{
    // FIXME: Implement
}

void Timer::TimerManager::fireTimers()
{
    for (Timer* timer = _head; timer; timer = timer->_next) {
        timer->_handler(timer);
    }    
}

void Timer::TimerManager::setCurrentTime(const RealTime& t)
{
    _epochOffset = t.usSinceEpoch() - systemTime();
}

RealTime Timer::TimerManager::currentTime()
{
    return RealTime(_epochOffset + Timer::systemTime());
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

String RealTime::timeString(TimeFormat format)
{
    static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
    switch (format) {
    case TimeFormat::Time:
        return bare::String::format("%d:%02d:%02d", hours(), minutes(), seconds());
    case TimeFormat::Date:
        return bare::String::format("%s %d/%d/%d", days[dayOfWeek()], month(), day(), year());
    case TimeFormat::DateTime:
        return bare::String::format("%s %d/%d/%d %d:%02d:%02d", days[dayOfWeek()], month(), day(), year(), hours(), minutes(), seconds());
    default: return "***";
    }
}
