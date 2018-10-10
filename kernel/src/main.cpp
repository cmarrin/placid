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

#include "util.h"
#include "BootShell.h"
#include "GPIO.h"
#include "Memory.h"
#include "Print.h"
#include "SDCard.h"
#include "Serial.h"
#include "String.h"
#include "Timer.h"
#include <vector>

using namespace placid;

static constexpr uint32_t ActivityLED = 47;
static constexpr float blinkRate = 0.5;

class LEDBlinker : public bare::TimerCallback
{
public:
	virtual void handleTimerEvent()
	{
		bare::GPIO::setPin(ActivityLED, !bare::GPIO::getPin(ActivityLED));
	}
};

static void timingTest(const char* s)
{
    // Timing test
    int64_t startTime = bare::Timer::systemTime();
    volatile uint32_t n = 0;
    for (int i = 0; i < 1000000; ++i) {
        n += i + 234;
    }
    bare::Serial::printf("*** Timing test %s: %d us\n", s, static_cast<uint32_t>(bare::Timer::systemTime() - startTime));
}

static void showTime()
{
    static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
    bare::RealTime currentTime = bare::Timer::currentTime();
    bare::Serial::printf("*** current time = %d:%d:%d %s %d/%d/%d\n",
        currentTime.hours(), currentTime.minutes(), currentTime.seconds(),
        days[currentTime.dayOfWeek()],
        currentTime.month(), currentTime.day(), currentTime.year());
}

int main()
{
    bare::Serial::init();
    bare::Timer::init();
    
    bare::Serial::printf("\n\nWelcome to the Placid Kernel\n\n");

    timingTest("Memory perf without cache");
    Memory::init();
    timingTest("Memory perf with cache");
    
    bare::Timer::setCurrentTime(bare::RealTime(2018, 10, 5, 10, 19));
    showTime();

    std::vector<String> vec = {"This", "is", "a" };
    vec.push_back("nice");
    vec[3] += " string";
    bare::Serial::printf("Vector test: %s\n", join(vec, " ").c_str());
    
    bare::GPIO::setFunction(ActivityLED, bare::GPIO::Function::Output);
    
    LEDBlinker blinker;
    bare::Timer::start(&blinker, blinkRate, true);
    
	BootShell shell;
	shell.connected();

	while (1) {
        uint8_t c;
        if (bare::Serial::read(c) != bare::Serial::Error::OK) {
            bare::Serial::puts("*** Serial Read Error\n");
        } else {
            shell.received(c);
        }
	}
 
    return 0;
}
