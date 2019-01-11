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

#include "bare/SPI.h"

#include "bare/Serial.h"

using namespace bare;

static void showSim(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Serial::printf("[SPI Sim]===> ");
    Serial::vprintf(format, args);
    va_end(args);
}

void SPI::init(EnablePolarity, ClockEdge, ClockPolarity)
{
    showSim("init\n");
}

int32_t SPI::readWrite(char* readBuf, const char* writeBuf, int32_t size)
{
    showSim("readWrite %d bytes:");
    if (writeBuf) {
        for (int i = 0; i < size; ++i) {
            Serial::printf("%02x", writeBuf[i]);
        }
    }
    Serial::printf("\n");
    return size;
}

void SPI::startTransfer(uint32_t size)
{
    showSim("startTransfer(%d)\n", size);
}

uint32_t SPI::transferByte(uint8_t b)
{
    showSim("transferByte(0x%02x)\n", b);
    return AnyByte;
}

void SPI::endTransfer()
{
    showSim("endTransfer\n");
}

bool SPI::simulatedData()
{
    return true;
}
