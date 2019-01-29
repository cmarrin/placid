/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Serial.h"
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

namespace bare {
    
    // SPIMaster - Serial Peripheral Interface master driver
    //

    class SPIMaster {
    public:
        // Special values for transferByte(). AnyByte is send by Darwin to indicate that the returned byte
        // can be anyting you want. ErrorByte means the read had an error
        static constexpr uint32_t AnyByte = 256;
        static constexpr uint32_t ErrorByte = 257;
        
        enum class ClockEdge { Rising, Falling };
        enum class ClockPolarity { ActiveHigh, ActiveLow };
        enum class EnablePolarity { ActiveLow, ActiveHigh };
        
        static void init(uint32_t transferRate, EnablePolarity = EnablePolarity::ActiveLow, ClockEdge = ClockEdge::Rising, ClockPolarity = ClockPolarity::ActiveHigh);
        
        static void sendStatus(uint32_t status, uint8_t statusByteCount);
        static void sendData(const uint8_t* data, uint8_t size);
        static uint32_t receiveStatus(uint8_t statusByteCount);
        static uint8_t receiveData(uint8_t* data, uint8_t maxSize);
        
        static int32_t readWrite(char* readBuf, const char* writeBuf, int32_t);

        static void startTransfer();
        static uint32_t transferByte(uint8_t b);
        static void endTransfer();
        
        static bool simulatedData();

    private:
    };
    
}
