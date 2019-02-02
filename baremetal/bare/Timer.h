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
#include <memory>

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
    
	class Timer : std::enable_shared_from_this<Timer> {
    private:
        struct TimerManager : public Singleton <TimerManager>
        {
            void init();
            
            void add(std::shared_ptr<Timer> timer) { _timers.push_back(timer); }
            void remove(std::shared_ptr<Timer> timer)
            {
                _timers.erase(std::remove_if(_timers.begin(), _timers.end(),
                    [timer](std::shared_ptr<Timer> t) { return t == timer; }), _timers.end());
                TimerManager::instance().updateTimers();
            }

            void fireTimers();
            void sortTimers()
            {
                std::sort(_timers.begin(), _timers.end(),
                    [](std::shared_ptr<Timer> a, std::shared_ptr<Timer> b)
                    {
                        return a->_timeToFire < b->_timeToFire;
                    });
            }
            
            RealTime currentTime();
            void setCurrentTime(const RealTime&);

            // Platform implementation
            void updateTimers();

            std::vector<std::shared_ptr<Timer>> _timers;
            int64_t _epochOffset = 0;
        };
        
	public:
        static constexpr int64_t DoNotFire = std::numeric_limits<int64_t>::max();
        
        using Handler = std::function<void(std::shared_ptr<Timer>)>;
        
        ~Timer() {
            TimerManager::instance().remove(shared_from_this());
            TimerManager::instance().updateTimers();
        }
        
        static std::shared_ptr<Timer> create(Handler);

        static void init() { TimerManager::instance().init(); }
        
        // FIXME: repeat is currently not implemented 
        void start(uint32_t us, bool repeat)
        {
            _timeout = us;
            _repeat = repeat;
            _timeToFire = systemTime() + _timeout;
            TimerManager::instance().sortTimers();
            TimerManager::instance().updateTimers();
        }
        void stop()
        {
            _timeout = 0;
            _timeToFire = DoNotFire;
            TimerManager::instance().sortTimers();
            TimerManager::instance().updateTimers();
        }

        // Platform implementations
        static void handleInterrupt();
        static void usleep(uint32_t us);
        static int64_t systemTime();

        static RealTime currentTime() { return TimerManager::instance().currentTime(); }
        static void setCurrentTime(const RealTime& t) { TimerManager::instance().setCurrentTime(t); }

	private:
        Timer(Handler handler) : _handler(handler) { }

        Handler _handler;
        uint32_t _timeout = 0;
        int64_t _timeToFire = DoNotFire;
        bool _repeat = false;
	};
	
}
