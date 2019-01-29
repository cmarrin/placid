/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace bare {
    
    // Formatter - Formatted printer

    class Formatter {
    public:
        using Generator = std::function<void(char)>;

        enum class Flag {
            leftJustify = 0x01,
            plus = 0x02,
            space = 0x04,
            alt = 0x08,
            zeroPad = 0x10,
        };

        static bool isFlag(uint8_t flags, Flag flag) { return (flags & static_cast<uint8_t>(flag)) != 0; }
        static void setFlag(uint8_t& flags, Flag flag) { flags |= static_cast<uint8_t>(flag); }

        enum class Capital { Yes, No };
        
        //                                                 sign    digits  dp      'e'     dp      exp     '\0'
        static constexpr uint32_t MaxToStringBufferSize =    1 +     20 +   1 +     1 +     1 +     3 +      1;
        static constexpr uint32_t MaxStringSize = 256;
        static constexpr uint32_t MaxIntegerBufferSize = 24; // Big enough for a 64 bit integer in octal

        static int32_t format(Generator gen, const char* fmt, ...)
        {
            va_list va;
            va_start(va, fmt);
            int32_t result = vformat(gen, fmt, va);
            va_end(va);
            return result;
        }
        
        static int32_t vformat(Generator, const char *format, va_list);

        static uint32_t printString(Generator, Float v, int32_t precision = -1, Capital = Capital::No, uint8_t flags = 0);
        static uint32_t printString(Generator, uint64_t v, uint8_t base = 10, Capital = Capital::No, uint8_t flags = 0);
        
        static uint32_t printString(Generator gen, int32_t v) { emitSign(gen, v); return printString(gen, static_cast<uint32_t>(v)); }
        static uint32_t printString(Generator gen, uint32_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString(gen, static_cast<uint64_t>(v), base, cap); }
        static uint32_t printString(Generator gen, int64_t v) { emitSign(gen, v); return printString(gen, static_cast<uint64_t>(v)); }
        static uint32_t printString(Generator gen, int8_t v) { return printString(gen, static_cast<int32_t>(v)); }
        static uint32_t printString(Generator gen, uint8_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString(gen, static_cast<uint32_t>(v), base, cap); }
        static uint32_t printString(Generator gen, int16_t v) { return printString(gen, static_cast<int32_t>(v)); }
        static uint32_t printString(Generator gen, uint16_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString(gen, static_cast<uint32_t>(v), base, cap); }

        static uint32_t toString(char* buf, Float v) { return printString([&buf](char c) { *buf++ = c; }, v); }
        static uint32_t toString(char* buf, int32_t v) { return printString([&buf](char c) { *buf++ = c; }, v); }
        static uint32_t toString(char* buf, uint32_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString([&buf](char c) { *buf++ = c; }, v, base, cap); }
        static uint32_t toString(char* buf, int64_t v) { return printString([&buf](char c) { *buf++ = c; }, v); }
        static uint32_t toString(char* buf, uint64_t v, uint8_t base = 10, Capital cap = Capital::No) { return printString([&buf](char c) { *buf++ = c; }, v, base, cap); }
        static uint32_t toString(char* buf, int8_t v) { return toString(buf, static_cast<int32_t>(v)); }
        static uint32_t toString(char* buf, uint8_t v, uint8_t base = 10, Capital cap = Capital::No) { return toString(buf, static_cast<uint32_t>(v), base, cap); }
        static uint32_t toString(char* buf, int16_t v) { return toString(buf, static_cast<int32_t>(v)); }
        static uint32_t toString(char* buf, uint16_t v, uint8_t base = 10, Capital cap = Capital::No) { return toString(buf, static_cast<uint32_t>(v), base, cap); }

        static bool toNumber(const char*& s, uint32_t& n);
        
    private:
        template<typename T> static void emitSign(Generator gen, T& v)
        {
            if (v < 0) {
                gen('-');
                v = -v;
            }
        }
    };
    
}
