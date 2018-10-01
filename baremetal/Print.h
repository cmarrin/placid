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

#include <cstdarg>
#include <cstddef>
#include <cstdint>

namespace placid {
    
    // Print - Formatted printer
    //
    // Adapted from https://github.com/mpaland/printf
    // 
    // (c) Marco Paland (info@paland.com)
    // 2014-2018, PALANDesign Hannover, Germany
    // The MIT License (MIT)
    //

    class Print {
    public:
        struct Printer
        {
            virtual int32_t outChar(char c) = 0;
        };
        
        //                                                 sign    digits  dp      'e'     dp      exp     '\0'
        static constexpr uint32_t MaxToStringBufferSize =    1 +     20 +   1 +     1 +     1 +     3 +      1;
        static constexpr uint32_t MaxStringSize = 256;

        static int32_t printf(const char* format, ...);
        static int32_t snprintf(char *str, size_t n, const char *format, ...);
        static int32_t vsnprintf(char *str, size_t n, const char *format, va_list);

        static bool toString(char* buf, double v);
        static bool toString(char* buf, int32_t v);
        static bool toString(char* buf, uint32_t v, uint8_t base = 10);
        static bool toString(char* buf, int64_t v);
        static bool toString(char* buf, uint64_t v, uint8_t base = 10);
        static bool toString(char* buf, float v) { return toString(buf, static_cast<double>(v)); }
        static bool toString(char* buf, int8_t v) { return toString(buf, static_cast<int32_t>(v)); }
        static bool toString(char* buf, uint8_t v, uint8_t base = 10) { return toString(buf, static_cast<uint32_t>(v), base); }
        static bool toString(char* buf, int16_t v) { return toString(buf, static_cast<int32_t>(v)); }
        static bool toString(char* buf, uint16_t v, uint8_t base = 10) { return toString(buf, static_cast<uint32_t>(v), base); }

    private:
        Print() { }
        Print(Print&) { }
        Print& operator=(Print& other) { return other; }
        
        static int32_t vsnprintCore(Printer&, const char *format, va_list);
    };
    
}
