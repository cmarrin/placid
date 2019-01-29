/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare/String.h"

namespace bare {

    class IPAddress {
    public:
        IPAddress() { memset(_addr, 0, sizeof(_addr)); }
        
        IPAddress(uint32_t addr)
        {
            _addr[0] = static_cast<uint8_t>(addr);
            _addr[1] = static_cast<uint8_t>(addr >> 8);
            _addr[2] = static_cast<uint8_t>(addr >> 16);
            _addr[3] = static_cast<uint8_t>(addr >> 24);
        }
        
        IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
        {
            _addr[0] = a;
            _addr[1] = b;
            _addr[2] = c;
            _addr[3] = d;
        }
        
        IPAddress(const String& ipString);
        
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
        
        static IPAddress myIPAddress();
        static void lookupHostName(const char* name, std::function<void (const char* name, IPAddress)>);
        
    private:    
        uint8_t _addr[4];
};

}

