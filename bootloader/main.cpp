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

void autoload(void);
void xmodemReceive(void);

static constexpr uint32_t AutoloadTimeout = 3;

int main(int argc, const char * argv[])
{
    uart_init();

    printf("\n\nPlacid Bootloader v0.1\n\n");
    printf("Autoloading in %d seconds\n", AutoloadTimeout);
    printf("    (press [space] for XMODEM upload or any other key to autoload immediately)\n");
    
    timer_init();
    
    uint64_t startTime = timerTick();
    uint64_t tickTime = 1000000;

    while (1) {
        if (timerTick() - startTime > tickTime) {
            uart_send('.');
            tickTime += 1000000;
            if (tickTime++ > (AutoloadTimeout + 1) * 1000000) {
                autoload();
                break;
            }
        }
            
        if ((uart_lcr() & 0x01) == 0) {
            continue;
        }
        int c = uart_recv();
        if (c == ' ') {
            printf("\n\nStart XMODEM upload when ready...\n\n");
            xmodemReceive();
            break;
        } else if (c < 0x7f) {
            printf("\n\nAutoloading...\n\n");
            autoload();
            break;
        }
    }
    
    printf("\n\n*** Returned from loading, that should not happen. Busy looping...\n");
    while(1) { }
    return 0;
}
