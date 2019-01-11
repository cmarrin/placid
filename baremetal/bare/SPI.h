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

#include "Serial.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

namespace bare {
    
    // SPI - Serial Peripheral Interface driver for SPI0 on the Raspberry Pi
    //

    class SPI {
    public:
        // Special values for transferByte(). AnyByte is send by Darwin to indicate that the returned byte
        // can be anyting you want. ErrorByte means the read had an error
        static constexpr uint32_t AnyByte = 256;
        static constexpr uint32_t ErrorByte = 257;
        
        enum class ClockEdge { Rising, Falling };
        enum class ClockPolarity { ActiveHigh, ActiveLow };
        enum class EnablePolarity { ActiveLow, ActiveHigh };
        
        static void init(EnablePolarity = EnablePolarity::ActiveLow, ClockEdge = ClockEdge::Rising, ClockPolarity = ClockPolarity::ActiveHigh);
        
        static int32_t readWrite(char* readBuf, const char* writeBuf, int32_t);

        static void startTransfer(uint32_t size);
        uint32_t transferByte(uint8_t b);
        static void endTransfer();
        
        static bool simulatedData();

    private:
    };
    
}
