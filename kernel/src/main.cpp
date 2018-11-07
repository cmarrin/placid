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
#include "Allocator.h"
#include "BootShell.h"
#include "FileSystem.h"
#include "GPIO.h"
#include "Memory.h"
#include "Print.h"
#include "SDCard.h"
#include "Serial.h"
#include "SPI.h"
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
    Allocator::kernelAllocator().setUseAllocator(true);
    
    bare::Serial::init();
    bare::Timer::init();
    bare::SPI::init();
    
    bare::Serial::printf("\n\nWelcome to the Placid Kernel\n\n");
        
    timingTest("Memory perf without cache");
    Memory::init();
    timingTest("Memory perf with cache");
    
    bare::Timer::setCurrentTime(bare::RealTime(2018, 10, 5, 10, 19));
    showTime();
    
    // Test file read
    bare::Serial::printf("File read test...\n");
    File* fp = FileSystem::sharedFileSystem()->open("sample.txt", FileSystem::OpenMode::Read, FileSystem::OpenOption::Update);
    if (!fp->valid()) {
        bare::Serial::printf("File read open error for '%s': %s\n", "sample.txt", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
    } else {
        char buf[26];
        fp->seek(511, File::SeekWhence::Set);
        int32_t size = fp->read(buf, 25);
        buf[25] = '\0';
        if (size < 0) {
            bare::Serial::printf("File read error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        } else {
            bare::Serial::printf("File read:'%s'\n", buf);
        }
    }
    
    // Test update
    bare::Serial::printf("File update test...\n");
    fp->seek(518, File::SeekWhence::Set);
    int32_t size = fp->write("0123456789", 10);
    if (size < 0) {
        bare::Serial::printf("File update error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
    } else {
        fp->seek(-17, File::SeekWhence::Cur);
        char buf[26];
        int32_t size = fp->read(buf, 25);
        buf[25] = '\0';
        if (size < 0) {
            bare::Serial::printf("Reading back after update error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        } else {
            bare::Serial::printf("After update read:'%s'\n", buf);
        }
    }
    
    // Repair the file
    bare::Serial::printf("File repair test...\n");
    fp->seek(518, File::SeekWhence::Set);
    size = fp->write("altogether", 10);
    if (size < 0) {
        bare::Serial::printf("File repair error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
    } else {
        fp->seek(-17, File::SeekWhence::Cur);
        char buf[26];
        int32_t size = fp->read(buf, 25);
        buf[25] = '\0';
        if (size < 0) {
            bare::Serial::printf("Reading back after repair error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        } else {
            bare::Serial::printf("After repair read:'%s'\n", buf);
        }
    }

    delete fp;
    fp = nullptr;
    
    // Test file write
    bare::Serial::printf("File write test...\n");
    FileSystem::sharedFileSystem()->remove("test.txt");
    
    fp = FileSystem::sharedFileSystem()->open("test.txt", FileSystem::OpenMode::Write);
    if (!fp->valid()) {
        bare::Serial::printf("File write open error for '%s': %s\n", "test.txt", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
    } else {
        int32_t size = fp->write("The quick brown fox", 19);
        if (size < 0) {
            bare::Serial::printf("File write error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        } else {
            bare::Serial::printf("File write successful\n");
        }
    }
    
    if (fp->close() != bare::Volume::Error::OK) {
        bare::Serial::printf("File write close error for '%s': %s\n", "test.txt", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
    }
    
    delete fp;
    fp = nullptr;
    
    FileSystem::sharedFileSystem()->remove("test.txt");

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
