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
#include "Mailbox.h"
#include "Serial.h"
#include "Timer.h"

#ifndef __APPLE__
// FAT32 has a new statement, which is not allowed in the bootloader.
// Put a stub here to make sure we don't use it
void *operator new(size_t size)
{
    bare::Serial::printf("***** operator new not allowed in bootloader!\n");
    abort();
}
#endif

void autoload(void);

static constexpr uint32_t AutoloadTimeout = 3;

int main(int argc, const char * argv[])
{
    bare::Serial::init();
        
    bare::Serial::printf("\n\nPlacid Bootloader v0.2\n\n");
    bare::Serial::printf("Autoloading in %d seconds\n", AutoloadTimeout);
    bare::Serial::printf("    (press [space] for XMODEM upload or any other key to autoload immediately)\n");
    
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
            bare::Serial::printf("\n\nStart XMODEM upload when ready...\n\n");
            if (xmodemReceive([](uint32_t addr, char byte) { PUT8(addr, byte); })) {
                BRANCHTO(ARMBASE);
                break;
            }
        } else if (c < 0x7f) {
            bare::Serial::printf("\n\nAutoloading...\n\n");
            autoload();
            break;
        }
    }
    
    bare::Serial::printf("\n\n*** Returned from loading, that should not happen. Busy looping...\n");
    while(1) { }
    return 0;
}
