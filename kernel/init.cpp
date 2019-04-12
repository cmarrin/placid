/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/GPIO.h"
#include "bare/Memory.h"
#include "bare/SDCard.h"
#include "bare/Serial.h"
#include "bare/Timer.h"
#include "Allocator.h"
#include "BootShell.h"
#include "FileSystem.h"
#include "bare/String.h"
#include <vector>

using namespace placid;

extern int  emb_snprintf(char *s, size_t n, const char *fmt, ...);

static constexpr uint32_t ActivityLED = 47;
static constexpr uint32_t blinkRate = 500000;

static int64_t timingTest(const char* s)
{
    // Timing test
    int64_t t = bare::Timer::systemTime();
    volatile uint32_t n = 0;
    for (int i = 0; i < 1000000; ++i) {
        n += i + 234;
    }
    
    t = bare::Timer::systemTime() - t;
    bare::Serial::printf("*** Timing test %s: %lld us\n", s, t);
    return t;
}

BootShell shell;
bare::Memory::KernelHeap<bare::Memory::DefaultKernelHeapSize, bare::Memory::DefaultPageSize> kernelHeap;

extern "C" void init()
{
    bare::initSystem();
    
    bare::Serial::init(115200);
    bare::Timer::init();
    
    bare::Serial::printf("\n\nWelcome to the Placid Kernel\n\n");
        
    bare::Float t1 = bare::Float(timingTest("Memory perf without cache"));
    bare::Memory::init(&kernelHeap);
    bare::Float t2 = bare::Float(timingTest("Memory perf with cache"));
    bare::Float speedup = t1 / t2;
    
    bare::Serial::printf("Speedup with cache: %.1gx\n", speedup.toArg());
    
    bare::Timer::setCurrentTime(bare::RealTime(2019, 1, 19, 9, 9));
    
    bare::GPIO::setFunction(ActivityLED, bare::GPIO::Function::Output);
    bare::Timer::create([](std::shared_ptr<bare::Timer>) { bare::GPIO::setPin(ActivityLED, !bare::GPIO::getPin(ActivityLED)); })->start(blinkRate, true);

	shell.connected();
}

extern "C" void inputChar(uint8_t c)
{
    shell.received(c);
}
