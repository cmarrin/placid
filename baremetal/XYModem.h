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
        
        // Whenever a byte comes in, this function is called with the byte
        using ReceiveFunction = std::function<bool(char byte)>;
        
        bool receive(ReceiveFunction);
        
    private:
        ReadFunction _readFunc;
        WriteFunction _writeFunc;
        RxReadyFunction _rxReadyFunc;
        SystemTime _systemTime;
    };
    
}
