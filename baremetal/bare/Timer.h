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

#pragma once

#include "bare/Singleton.h"
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
        RealTime(int32_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t minute = 0, uint8_t second = 0, uint32_t us = 0);        
        RealTime(int64_t t) : _time(t) { }
        RealTime(const RealTime& other) { _time = other._time; }
        
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
            void start(Timer*, uint32_t us, bool repeat);
            void stop(Timer*);
            
            RealTime currentTime();
            void setCurrentTime(const RealTime&);

            Timer* _head;
            int64_t _epochOffset = 0;
        };
        
	public:
        Timer() { }
        virtual ~Timer() { }
        
        virtual void handleTimerEvent() = 0;
        
        // FIXME: repeat is currently not implemented 
        void start(uint32_t us, bool repeat) { TimerManager::instance().start(this, us, repeat); }
        void stop() { TimerManager::instance().stop(this); }

        // Platform implementations
        static void init();
        static void usleep(uint32_t us);
        static int64_t systemTime();
        static void handleInterrupt();

        static RealTime currentTime() { return TimerManager::instance().currentTime(); }
        static void setCurrentTime(const RealTime& t) { TimerManager::instance().setCurrentTime(t); }

	private:
        // Platform implementations
        static void updateTimers();
        
        Timer* _next = nullptr;
        uint32_t _timeout = 0;
        bool _repeat = false;
	};
	
}
