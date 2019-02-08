/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare/String.h"

namespace bare {
    
    class RealTime
    {
    public:
        enum class TimeFormat { Time, Date, DateTime };
        
        RealTime(int32_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0, uint32_t us = 0);        
        RealTime(int64_t t) : _time(t) { }
        RealTime(const RealTime& other) { _time = other._time; }
        
        String timeString(TimeFormat);
        
        int64_t usSinceEpoch() const { return _time; }
        
        uint32_t us() const { return _time % 1000000; }
        uint8_t seconds() const { return (_time / 1000000) % 60; }
        uint8_t minutes() const { return (_time / 1000000 / 60) % 60; }
        uint8_t hours() const { return (_time / 1000000 / 60 / 60) % 24; }
        uint8_t day() const;
        uint8_t month() const;
        int32_t year() const;
        uint8_t dayOfWeek() const; // 0 = Sunday
        
    private:
        int64_t _time = 0;
    };

}
