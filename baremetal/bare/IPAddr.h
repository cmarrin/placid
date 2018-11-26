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

#include "bare/String.h"

namespace bare {

    class IPAddr {
    public:
        IPAddr() { memset(_addr, 0, sizeof(_addr)); }
        
        IPAddr(uint32_t addr)
        {
            _addr[0] = static_cast<uint8_t>(addr);
            _addr[1] = static_cast<uint8_t>(addr >> 8);
            _addr[2] = static_cast<uint8_t>(addr >> 16);
            _addr[3] = static_cast<uint8_t>(addr >> 24);
        }
        
        IPAddr(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        {
            _addr[0] = a;
            _addr[1] = b;
            _addr[2] = c;
            _addr[3] = d;
        }
        
        IPAddr(const String& ipString);
        
        operator uint32_t() const
        {
            return  static_cast<uint32_t>(_addr[0]) | 
                    (static_cast<uint32_t>(_addr[1]) << 8) |
                    (static_cast<uint32_t>(_addr[2]) << 16) |
                    (static_cast<uint32_t>(_addr[3]) << 24);
        }
    
        operator bool() const { return _addr[0] != 0 || _addr[1] != 0 || _addr[2] != 0 || _addr[3] != 0; }
        uint8_t& operator[](size_t i) { assert(i < 4); return _addr[i]; }
        const uint8_t& operator[](size_t i) const { assert(i < 4); return _addr[i]; }
        
        static IPAddr myIPAddr();
        static void lookupHostName(const char* name, std::function<void (const char* name, IPAddr)>);
        
    private:    
        uint8_t _addr[4];
};

}

