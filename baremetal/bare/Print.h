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
#include <functional>

namespace bare {
    
    // Print - Formatted printer

    class Print {
    public:
        using Printer = std::function<void(char)>;

        enum class Capital { Yes, No };
        
        //                                                 sign    digits  dp      'e'     dp      exp     '\0'
        static constexpr uint32_t MaxToStringBufferSize =    1 +     20 +   1 +     1 +     1 +     3 +      1;
        static constexpr uint32_t MaxStringSize = 256;
        static constexpr uint32_t MaxIntegerBufferSize = 24; // Big enough for a 64 bit integer in octal

        static int32_t snprintf(char *str, size_t n, const char *format, ...);
        static int32_t vsnprintf(char *str, size_t n, const char *format, va_list);

        static int32_t printfCore(Printer, const char *format, va_list);

        static uint32_t printString(Printer, Float v, int32_t precision = -1, Capital = Capital::No);
        static uint32_t printString(Printer, uint64_t v, uint8_t base = 10, Capital = Capital::No);
        
        static uint32_t printString(Printer printer, int32_t v) { emitSign(printer, v); return printString(printer, static_cast<uint32_t>(v)); }
        static uint32_t printString(Printer printer, uint32_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString(printer, static_cast<uint64_t>(v), base, cap); }
        static uint32_t printString(Printer printer, int64_t v) { emitSign(printer, v); return printString(printer, static_cast<uint64_t>(v)); }
        static uint32_t printString(Printer printer, int8_t v) { return printString(printer, static_cast<int32_t>(v)); }
        static uint32_t printString(Printer printer, uint8_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString(printer, static_cast<uint32_t>(v), base, cap); }
        static uint32_t printString(Printer printer, int16_t v) { return printString(printer, static_cast<int32_t>(v)); }
        static uint32_t printString(Printer printer, uint16_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString(printer, static_cast<uint32_t>(v), base, cap); }

        static uint32_t toString(char* buf, Float v) { return printString([&buf](char c) { *buf++ = c; }, v); }
        static uint32_t toString(char* buf, int32_t v) { return printString([&buf](char c) { *buf++ = c; }, v); }
        static uint32_t toString(char* buf, uint32_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString([&buf](char c) { *buf++ = c; }, v, base, cap); }
        static uint32_t toString(char* buf, int64_t v) { return printString([&buf](char c) { *buf++ = c; }, v); }
        static uint32_t toString(char* buf, uint64_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString([&buf](char c) { *buf++ = c; }, v, base, cap); }
        static uint32_t toString(char* buf, int8_t v) { return toString(buf, static_cast<int32_t>(v)); }
        static uint32_t toString(char* buf, uint8_t v, uint8_t base = 10, Capital cap = Capital::No) { return toString(buf, static_cast<uint32_t>(v), base, cap); }
        static uint32_t toString(char* buf, int16_t v) { return toString(buf, static_cast<int32_t>(v)); }
        static uint32_t toString(char* buf, uint16_t v, uint8_t base = 10, Capital cap = Capital::No) { return toString(buf, static_cast<uint32_t>(v), base, cap); }

    private:
        Print() { }
        Print(Print&) { }
        Print& operator=(Print& other) { return other; }
        
        static uint32_t intToString(uint64_t value, bare::Print::Printer printer, uint8_t base = 10, bare::Print::Capital cap = bare::Print::Capital::No);
        template<typename T> static void emitSign(Printer printer, T& v)
        {
            if (v < 0) {
                printer('-');
                v = -v;
            }
        }
    };
    
}
