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

#include "sdcard.h"
#include "SDFS.h"
#include "util.h"

using namespace placid;

static constexpr uint32_t SOH = 0x01;
static constexpr uint32_t ACK = 0x06;
static constexpr uint32_t NAK = 0x15;
static constexpr uint32_t EOT = 0x04;

void xmodemReceive()
{
    // block numbers start with 1

    // 132 byte packet
    // starts with SOH
    // block number byte
    // 255-block number
    // 128 bytes of data
    // checksum byte (whole packet)
    // a single EOT instead of SOH when done, send an ACK on it too

    uint32_t block = 1;
    uint32_t addr = ARMBASE;
    uint32_t state = 0;
    uint32_t crc = 0;
    uint64_t startTime = timerTick();
    
    unsigned char xstring[256];

    while(1)
    {
        uint64_t curTime = timerTick();
        if ((curTime - startTime) >= 4000000)
        {
            uart_send(NAK);
            startTime += 4000000;
        }
        
        if((uart_lcr() & 0x01) == 0) {
            continue;
        }
        
        xstring[state] = uart_recv();
        startTime = timerTick();
        
        if (state == 0) {
            if (xstring[state] == EOT) {
                uart_send(ACK);
                BRANCHTO(ARMBASE);
                break;
            }
        }
        
        switch (state) {
        case 0:
            if (xstring[state] == SOH) {
                crc = xstring[state];
                state++;
            } else {
                uart_send(NAK);
            }
            break;
        case 1:
            if (xstring[state] == block) {
                crc += xstring[state];
                state++;
            } else {
                state = 0;
                uart_send(NAK);
            }
            break;
        case 2:
            if (xstring[state] == (0xFF - xstring[state - 1])) {
                crc += xstring[state];
                state++;
            } else {
                uart_send(NAK);
                state = 0;
            }
            break;
        case 131:
            crc &= 0xFF;
            if (xstring[state] == crc) {
                for (uint32_t i = 0; i < 128; i++) {
                    PUT8(addr++, xstring[i + 3]);
                }
                uart_send(ACK);
                block = (block + 1) & 0xFF;
            } else {
                uart_send(NAK);
            }
            state = 0;
            break;
        default:
            crc += xstring[state];
            state++;
            break;
        }
    }
}
