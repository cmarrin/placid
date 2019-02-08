/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <functional>

namespace bare {

    class XYModem
    {
    public:
        // Serial I/O lambdas
        using ReadFunction = std::function<void(uint8_t& c)>;
        using WriteFunction = std::function<void(uint8_t c)>;
        using RxReadyFunction = std::function<bool()>;
        
        // System time lambda. Returns current time in ms
        // Time is relative. Values don't matter as long as
        // subsequent values increment at a rate of 1ms
        using SystemTime = std::function<uint32_t()>;

        XYModem(ReadFunction readFunc, WriteFunction writeFunc, RxReadyFunction rxReadyFunc, SystemTime systemTime)
            : _readFunc(readFunc)
            , _writeFunc(writeFunc)
            , _rxReadyFunc(rxReadyFunc)
            , _systemTime(systemTime)
        { }
        
        bool receive(ReceiveFunction);
        
    private:
        ReadFunction _readFunc;
        WriteFunction _writeFunc;
        RxReadyFunction _rxReadyFunc;
        SystemTime _systemTime;
    };
    
}
