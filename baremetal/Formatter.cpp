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

#include "bare.h"

#include "bare/Formatter.h"

#include <cassert>

using namespace bare;

bool Formatter::toNumber(const char*& s, uint32_t& n)
{
    n = 0;
    bool haveNumber = false;
    while (1) {
        if (*s < '0' || *s > '9') {
            return haveNumber;
        }
        n = n * 10 + *s++ - '0';
        haveNumber = true;
    }
}

enum class Signed { Yes, No };
enum class FloatType { Float, Exp, Shortest };
 
enum class Flag {
    leftJustify = 0x01,
    plus = 0x02,
    space = 0x04,
    alt = 0x08,
    zeroPad = 0x10,
};

// va_list can be either an array or a struct. This makes it impossible to pass by reference
// in a cross-platform way. Wrap it in a struct and pass that by reference so it works in
// all platforms
struct VA_LIST { va_list value; };

static inline bool isFlag(uint8_t flags, Flag flag) { return (flags & static_cast<uint8_t>(flag)) != 0; }
static inline void setFlag(uint8_t& flags, Flag flag) { flags |= static_cast<uint8_t>(flag); }

static void handleFlags(const char*& format, uint8_t& flags)
{
    while (1) {
        switch (*format) {
        case '-': setFlag(flags, Flag::leftJustify); break;
        case '+': setFlag(flags, Flag::plus); break;
        case ' ': setFlag(flags, Flag::space); break;
        case '#': setFlag(flags, Flag::alt); break;
        case '0': setFlag(flags, Flag::zeroPad); break;
        default: return;
        }
        ++format;
    }
}

static int32_t handleWidth(const char*& format, VA_LIST& va)
{
    if (*format == '*') {
        ++format;
        return va_arg(va.value, int);
    }
    
    uint32_t n;
    return Formatter::toNumber(format, n) ? static_cast<int32_t>(n) : -1;
}

enum class Length { None, H, HH, L, LL, J, Z, T };

static Length handleLength(const char*& format)
{
    Length length = Length::None;
    if (*format == 'h') {
        ++format;
        length = (*++format == 'h') ? Length::HH : Length::H;
    } else if (*format == 'l') {
        ++format;
        length = (*++format == 'l') ? Length::LL : Length::L;
    } else if (*format == 'j') {
        length = Length::J;
    } else if (*format == 'z') {
        length = Length::Z;
    } else if (*format == 't') {
        length = Length::T;
    } else {
        return length;
    }
    ++format;
    return length;
}

static uintmax_t getInteger(Length length, Signed sign, VA_LIST& va)
{
    if (sign == Signed::Yes) {
        switch(length) {
        case Length::None: return static_cast<uintmax_t>(va_arg(va.value, int));
        case Length::H: return static_cast<uintmax_t>(va_arg(va.value, int)) & 0xffff;
        case Length::HH: return static_cast<uintmax_t>(va_arg(va.value, int)) & 0xff;
        case Length::L: return static_cast<uintmax_t>(va_arg(va.value, long int));
        case Length::LL: return static_cast<uintmax_t>(va_arg(va.value, long long int));
        case Length::J: return static_cast<uintmax_t>(va_arg(va.value, intmax_t));
        case Length::Z: return static_cast<uintmax_t>(va_arg(va.value, size_t));
        case Length::T: return static_cast<uintmax_t>(va_arg(va.value, ptrdiff_t));
        }
    } else {
        switch(length) {
        case Length::None: return static_cast<uintmax_t>(va_arg(va.value, unsigned int));
        case Length::H: return static_cast<uintmax_t>(va_arg(va.value, unsigned int)) & 0xffff;
        case Length::HH: return static_cast<uintmax_t>(va_arg(va.value, unsigned int)) & 0xff;
        case Length::L: return static_cast<uintmax_t>(va_arg(va.value, unsigned long int));
        case Length::LL: return static_cast<uintmax_t>(va_arg(va.value, unsigned long long int));
        case Length::J: return static_cast<uintmax_t>(va_arg(va.value, intmax_t));
        case Length::Z: return static_cast<uintmax_t>(va_arg(va.value, size_t));
        case Length::T: return static_cast<uintmax_t>(va_arg(va.value, ptrdiff_t));
        }
    }
    return 0;
}

static char* intToString(uint64_t value, char* buf, size_t size, uint8_t base = 10, bare::Formatter::Capital cap = bare::Formatter::Capital::No)
{
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }
    
    char hexBase = (cap == bare::Formatter::Capital::Yes) ? 'A' : 'a';
    char* p = buf + size;
    *--p = '\0';
    
    while (value) {
        uint8_t digit = value % base;
        *--p = (digit > 9) ? (digit - 10 + hexBase) : (digit + '0');
        value /= base;
    }
    return p;
}

static int32_t outInteger(bare::Formatter::Generator gen, uintmax_t value, Signed sign, int32_t width, int32_t precision, uint8_t flags, uint8_t base, bare::Formatter::Capital cap)
{
    uint32_t size = 0;
    if (sign == Signed::Yes) {
        if (value < 0) {
            value = -value;
            gen('-');
            size = 1;
            width--;
        }
    }
    
    if (isFlag(flags, Flag::alt) && base != 10) {
        gen('0');
        size++;
        width--;
        if (base == 16) {
            gen((cap == bare::Formatter::Capital::Yes) ? 'X' : 'x');
            size++;
            width--;
        }
    }
    
    char buf[bare::Formatter::MaxIntegerBufferSize];
    char* p = intToString(static_cast<uint64_t>(value), buf, bare::Formatter::MaxIntegerBufferSize, base, cap);
    size += static_cast<uint32_t>(p - buf);

    if (isFlag(flags, Flag::zeroPad)) {
        int32_t pad = static_cast<int32_t>(width) - static_cast<int32_t>(bare::strlen(p));
        while (pad > 0) {
            gen('0');
            size++;
            pad--;
        }
    }
    
    for ( ; *p; ++p) {
        gen(*p);
    }

    return size;
}

#if !defined(FLOATNONE)
static int32_t outFloat(bare::Formatter::Generator gen, Float value, int32_t width, int32_t precision, uint8_t flags, bare::Formatter::Capital cap, FloatType type)
{
    // FIXME: Handle flags.leftJustify
    // FIXME: Handle flags.plus
    // FIXME: Handle flags.space
    // FIXME: Handle flags.alt
    // FIXME: Handle flags.zeroPad
    // FIXME: Handle width
    // FIXME: Handle precision
    return bare::Formatter::printString(gen, value, precision, cap);
}
#endif

static int32_t outString(bare::Formatter::Generator gen, const char* s, int32_t width, int32_t precision, uint8_t flags)
{
    // FIXME: Handle flags.leftJustify
    // FIXME: Handle width
    // FIXME: Handle precision
    int32_t size = 0;
    while (*s) {
        gen(*s++);
        ++size;
    }
    return size;
}

// Unsupported features:
//
//     'n' specifier - returns number of characters written so far
//     'a', 'A' specifiers - prints hex floats
//     'L' length - long double
//     'l' length for 'c' and 's' specifiers - wide characters
 
int32_t Formatter::vformat(Formatter::Generator gen, const char *format, va_list vaIn)
{
    assert(format);
    
    VA_LIST va;
    va_copy(va.value, vaIn);
    
    uint8_t flags = 0;
        
    int32_t size = 0;
    
    while (*format) {
        if (*format != '%') {
            gen(*format++);
            size++;
            continue;
        }
        
        format++;
        
        // We have a format, do the optional part
        handleFlags(format, flags);
        int32_t width = handleWidth(format, va);
        int32_t precision = -1;
        if (*format == '.') {
            precision = handleWidth(++format, va);
        }
        Length length = handleLength(format);
        
        // Handle the specifier
        switch (*format)
        {
        case 'd':
        case 'i':
            size += outInteger(gen, getInteger(length, Signed::Yes, va), Signed::Yes, width, precision, flags, 10, Formatter::Capital::No);
            break;
        case 'u':
            size += outInteger(gen, getInteger(length, Signed::No, va), Signed::No, width, precision, flags, 10, Formatter::Capital::No);
            break;
        case 'o':
            size += outInteger(gen, getInteger(length, Signed::No, va), Signed::No, width, precision, flags, 8, Formatter::Capital::No);
            break;
        case 'x':
        case 'X':
            size += outInteger(gen, getInteger(length, Signed::No, va), Signed::No, width, precision, flags, 16, (*format == 'X') ? Formatter::Capital::Yes : Formatter::Capital::No);
            break;
#if !defined(FLOATNONE)
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G': {
            Formatter::Capital cap = Formatter::Capital::No;
            FloatType type = FloatType::Shortest;
            switch(*format)
            {
            case 'f': cap = Formatter::Capital::No; type = FloatType::Float; break;
            case 'F': cap = Formatter::Capital::Yes; type = FloatType::Float; break;
            case 'e': cap = Formatter::Capital::No; type = FloatType::Exp; break;
            case 'E': cap = Formatter::Capital::Yes; type = FloatType::Exp; break;
            case 'g': cap = Formatter::Capital::No; type = FloatType::Shortest; break;
            case 'G': cap = Formatter::Capital::Yes; type = FloatType::Shortest; break;
            }

            size += outFloat(gen, Float::fromArg(va_arg(va.value, Float::arg_type)), width, precision, flags, cap, type);
            break;
        }
#endif
        case 'c':
            gen(static_cast<char>(va_arg(va.value, int)));
            size++;
            break;
        case 's':
            size += outString(gen, va_arg(va.value, const char*), width, precision, flags);
            break;
        case 'p':
            size += outInteger(gen, reinterpret_cast<int64_t>(va_arg(va.value, void*)), Signed::No, width, precision, flags, 16, Formatter::Capital::No);
            break;
        default:
            gen(*format++);
            size++;
            break;
        }
        ++format;
    }
    
    return size;
}

uint32_t Formatter::printString(Generator gen, uint64_t v, uint8_t base, Capital cap)
{
    char buf[MaxIntegerBufferSize];
    char* p = ::intToString(v, buf, MaxIntegerBufferSize, base, cap);
    uint32_t size = 0;
    while (*p) {
        gen(*p++);
        ++size;
    }
    gen('\0');
    return size;
}
