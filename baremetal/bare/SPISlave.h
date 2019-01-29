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
