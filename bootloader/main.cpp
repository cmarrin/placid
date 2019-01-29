/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Serial.h"
#include "bare/Timer.h"
#include "bare/XYModem.h"

void autoload(void);

static constexpr uint32_t AutoloadTimeout = 3;

int main()
{
    bare::initSystem();
    
    bare::Serial::init(115200);
        
    bare::Serial::printf("\n\nPlacid Bootloader v0.2\n\n");
    bare::Serial::printf("Autoloading in %d seconds\n", AutoloadTimeout);
    bare::Serial::printf("    (press [space] for X/YMODEM upload or any other key to autoload immediately)\n");
    
    bare::Timer::init();
    
    int64_t startTime = bare::Timer::systemTime();
    int64_t tickTime = 1000000;

    while (1) {
        if (bare::Timer::systemTime() - startTime > tickTime) {
            bare::Serial::write('.');
            tickTime += 1000000;
            if (tickTime++ > (AutoloadTimeout + 1) * 1000000) {
                autoload();
                break;
            }
        }
            
        if (!bare::Serial::rxReady()) {
            continue;
        }
        
        uint8_t c;
        bare::Serial::read(c);
        if (c == ' ') {
            bare::Serial::printf("\n\nStart X/YMODEM upload when ready...\n\n");
            uint8_t* addr = bare::kernelBase();
            bare::XYModem xyModem(
                [](uint8_t& c) { bare::Serial::read(c); },
                [](uint8_t c) { bare::Serial::write(c); },
                []() -> bool { return bare::Serial::rxReady(); },
                []() -> uint32_t { return static_cast<uint32_t>(bare::Timer::systemTime() / 1000); });

            if (xyModem.receive([&addr](char byte) -> bool { bare::PUT8(addr++, byte); return true; })) {
                bare::Timer::usleep(100000);
                bare::BRANCHTO(bare::kernelBase());
                break;
            }
        } else if (c < 0x7f) {
            bare::Serial::printf("\n\nAutoloading...\n\n");
            autoload();
            break;
        }
    }
    
    bare::Timer::usleep(100000);
    bare::Serial::printf("\n\n*** Returned from loading, that should not happen. Busy looping...\n");
    while(1) { }
    return 0;
}
