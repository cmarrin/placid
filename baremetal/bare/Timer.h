/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare/InterruptManager.h"
#include "bare/Singleton.h"
#include "bare/String.h"
#include <stdint.h>

namespace bare {
	
	// Timer - Raw Peripheral (as opposed to System) Timer driver for Raspberry Pi
	//
	// This code was inspired bu the work here:
	//
	//		https://github.com/dwelch67/raspberrypi
	//
	// This is a static class and cannot be instantiated. Starting a new
	// timer will cancel the current one
	//
 
    // Calendar date/time
    //
    // Value is stored in "Rata Die", in which a time of 0 is the date 0001-01-01
    //
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
    
	class Timer {
    private:
        struct TimerManager : public Singleton <TimerManager>
        {
            void init();
            
            void start(Timer*, uint32_t us, bool repeat);
            void stop(Timer*);
            
            void fireTimers();
            
            RealTime currentTime();
            void setCurrentTime(const RealTime&);

            // Platform implementation
            void updateTimers();

            Timer* _head;
            int64_t _epochOffset = 0;
        };
        
	public:
        Timer() { }
        virtual ~Timer() { }
        
        static void init()
        {
            TimerManager::instance().init();
            InterruptManager::instance().addHandler(handleInterrupt);
        }

        virtual void handleTimerEvent() = 0;
        
        // FIXME: repeat is currently not implemented 
        void start(uint32_t us, bool repeat) { TimerManager::instance().start(this, us, repeat); }
        void stop() { TimerManager::instance().stop(this); }

        // Platform implementations
        static void handleInterrupt();
        static void usleep(uint32_t us);
        static int64_t systemTime();

        static RealTime currentTime() { return TimerManager::instance().currentTime(); }
        static void setCurrentTime(const RealTime& t) { TimerManager::instance().setCurrentTime(t); }

	private:
        Timer* _next = nullptr;
        uint32_t _timeout = 0;
        bool _repeat = false;
	};
	
}
