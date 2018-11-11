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

#include "bare.h"

#include "bare/Mailbox.h"
#include "bare/Serial.h"
#include "bare/Timer.h"
#include "bare/XYModem.h"

void autoload(void);

static constexpr uint32_t AutoloadTimeout = 3;

extern bool UseAllocator;

extern unsigned char __bss_start;
extern unsigned char _end;
extern void (*__init_start) (void);
extern void (*__init_end) (void);

int main(int argc, const char * argv[])
{
#ifndef __APPLE__
    for (unsigned char *pBSS = &__bss_start; pBSS < &_end; pBSS++)
    {
        *pBSS = 0;
    }

    // call construtors of static objects
    for (void (**pFunc) (void) = &__init_start; pFunc < &__init_end; pFunc++)
    {
        (**pFunc) ();
    }
#endif
    
    // This is only used on Mac. We are essentially overriding new and there
    // are system libraries on Mac which use new and we don't want them using our
    // little block of heap. So we tell them to just allocate using malloc() and
    // then we set this true and start using the nanoallocator.
    UseAllocator = true;

    bare::Serial::init();
        
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
            uint32_t addr = ARMBASE;
            bare::XYModem xyModem(
                [](uint8_t& c) { bare::Serial::read(c); },
                [](uint8_t c) { bare::Serial::write(c); },
                []() -> bool { return bare::Serial::rxReady(); },
                []() -> uint32_t { return static_cast<uint32_t>(bare::Timer::systemTime() / 1000); });

            if (xyModem.receive([&addr](char byte) -> bool { PUT8(addr++, byte); return true; })) {
                bare::Timer::usleep(100000);
                BRANCHTO(ARMBASE);
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
