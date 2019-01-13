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
    
    // SPISlave - Serial Peripheral Interface slave driver
    //
    // This slave handles status values sent and received, fixed at 32 bits
    // and data sent and received with a maximum length of 32 bytes

    class SPISlave {
    public:
        enum class ClockEdge { Rising, Falling };
        enum class ClockPolarity { ActiveHigh, ActiveLow };
        enum class EnablePolarity { ActiveLow, ActiveHigh };
        
        using DataReceivedFunction = std::function<void(uint8_t*, uint8_t)>;
        using DataSentFunction = std::function<void()>;
        using StatusReceivedFunction = std::function<void(uint32_t)>;
        using StatusSentFunction = std::function<void()>;

        void init(EnablePolarity = EnablePolarity::ActiveLow, ClockEdge = ClockEdge::Rising, ClockPolarity = ClockPolarity::ActiveHigh);
        
        void setStatus(uint32_t status);
        void setData(const uint8_t* data, uint8_t size);

        void setDataReceivedFunction(DataReceivedFunction f) { _dataReceivedFunction = f; }
        void setDataSentFunction(DataSentFunction f) { _dataSentFunction = f; }
        void setStatusReceivedFunction(StatusReceivedFunction f) { _statusReceivedFunction = f; }
        void setStatusSentFunction(StatusSentFunction f) { _statusSentFunction = f; }
        
        DataReceivedFunction dataReceivedFunction() const { return _dataReceivedFunction; }
        DataSentFunction dataSentFunction() const { return _dataSentFunction; }
        StatusReceivedFunction statusReceivedFunction() const { return _statusReceivedFunction; }
        StatusSentFunction statusSentFunction() const { return _statusSentFunction; }
        
    private:
        DataReceivedFunction _dataReceivedFunction;
        DataSentFunction _dataSentFunction;
        StatusReceivedFunction _statusReceivedFunction;
        StatusSentFunction _statusSentFunction;
    };
    
}
