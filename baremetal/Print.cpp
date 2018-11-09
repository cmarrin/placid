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

#include "Print.h"

#include "util.h"
#include <cassert>

using namespace bare;

//static constexpr uint16_t MaxDecimalDigits = 6;

// Decompose a double into an exponent and an integer mantissa.
// The mantissa is between 0.1 and 1 and multiplied by 100,000,000
// to fit in an int32_t. So the maximum number of digits that can
// be displayed is 9.

static void decompose(double v, int32_t& mantissa, int32_t& exponent, int32_t precision)
{
    // Get number to between 0.1 and 1
    // This is obviously slow if the number is very small or very large.
    // It could be sped up by going by multiple orders of magnitude at
    // a time when possible
    bool sign = false;
    if (v < 0) {
        sign = true;
        v = -v;
    }
    exponent = 0;
    while (v < 0.1) {
        v *= 10;
        exponent--;
    }
    while (v > 1) {
        v /= 10;
        exponent++;
    }
    mantissa = static_cast<int32_t>(v * 1e9 + 0.5);
 
    // Round mantissa according to the number of digits to display
    if (precision > 0) {
        uint16_t digits = precision + ((exponent > 0) ? exponent : 0);
        if (digits < 9) {
            digits = 8 - digits;
            while (digits--) {
                mantissa /= 10;
            }
            mantissa += 5;
            mantissa /= 10;
        }
    }
    
    if (sign) {
        mantissa = -mantissa;
    }
}

static char* mantissaToString(uint32_t mantissa, char* buf, size_t size, int32_t dp)
{
    int32_t numDigits = 1;
    for (uint32_t pow10 = 10; mantissa > pow10; pow10 *= 10, numDigits++) ;
    
    int32_t digitsToRight = numDigits - dp;
    
    char* p = buf + size;
    *--p = '\0';
    
    bool trailing = true;
    while (mantissa || digitsToRight > 0) {
        if (digitsToRight-- == 0) {
            *--p = '.';
        }
        
        uint8_t digit = mantissa % 10;
        
        // Remove trailing zeroes
        if (digitsToRight <= 0 || digit != 0 || !trailing) {
            *--p = digit + '0';
            trailing = false;
        }
        
        mantissa /= 10;
    }
    return p;
}

static uint32_t mantissaToString(uint32_t mantissa, bare::Print::Printer printer, int32_t dp)
{
    char buf[bare::Print::MaxToStringBufferSize];
    char* p = mantissaToString(mantissa, buf, bare::Print::MaxToStringBufferSize, dp);
    uint32_t size = static_cast<uint32_t>(p - buf);
    while (*p) {
        printer(*p++);
    }
    return size;
}

static char* intToString(uint64_t value, char* buf, size_t size, uint8_t base = 10, bare::Print::Capital cap = bare::Print::Capital::No)
{
    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }
    
    char hexBase = (cap == bare::Print::Capital::Yes) ? 'A' : 'a';
    char* p = buf + size;
    *--p = '\0';
    
    while (value) {
        uint8_t digit = value % base;
        *--p = (digit > 9) ? (digit - 10 + hexBase) : (digit + '0');
        value /= base;
    }
    return p;
}

static uint32_t intToString(uint64_t value, bare::Print::Printer printer, uint8_t base = 10, bare::Print::Capital cap = bare::Print::Capital::No)
{
    char buf[bare::Print::MaxToStringBufferSize];
    char* p = intToString(value, buf, bare::Print::MaxToStringBufferSize, base, cap);
    uint32_t size = static_cast<uint32_t>(p - buf);
    while (*p) {
        printer(*p++);
    }
    return size;
}

uint32_t Print::printString(Printer printer, double v, int32_t precision, Capital cap)
{
    if (v == 0) {
        printer('0');
        printer('\0');
        return 1;
    }
    
    uint32_t size = 0;
    int32_t mantissa;
    int32_t exponent;
    decompose(v, mantissa, exponent, precision);
    
    if (mantissa < 0) {
        printer('-');
        size++;
        mantissa = -mantissa;
    }
        
    if (exponent >= -3 && exponent <= 5) {
        // no exponent
        return mantissaToString(mantissa, printer, exponent) + size;
    }
    
    size += mantissaToString(mantissa, printer, 1);
    printer((cap == bare::Print::Capital::Yes) ? 'E' : 'e');
    size++;

    // Assume exp is no more than 3 digits. To move
    // it to the upper 3 digits of an int32_t we
    // multiply by 1000000 and indicate that there
    // are 3 digits
    exponent--;
    if (exponent < 0) {
        printer('-');
        size++;
        exponent = -exponent;
    }
    
    size += intToString(exponent, printer);
    printer('\0');
    return size;
}

uint32_t Print::printString(Printer printer, uint64_t v, uint8_t base, Capital cap)
{
    char buf[MaxIntegerBufferSize];
    char* p = intToString(v, buf, MaxIntegerBufferSize, base, cap);
    uint32_t size = 0;
    while (*p) {
        printer(*p++);
        ++size;
    }
    printer('\0');
    return size;
}

int32_t Print::vsnprintf(char* buffer, size_t count, const char* format, va_list va)
{
    return printfCore([&buffer, &count](char c)
    {
        if (count > 0) {
            *buffer++ = c;
            --count;
        }
    }, format, va);
}

int32_t Print::snprintf(char* buffer, size_t count, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    int32_t result = vsnprintf(buffer, count, format, va);
    va_end(va);
    return result;
}

union Flags
{
    struct
    {
        bool leftJustify : 1;
        bool plus : 1;
        bool space : 1;
        bool alt : 1;
        bool zeroPad : 1;
    };
    uint8_t value;
};

enum class Signed { Yes, No };
enum class FloatType { Float, Exp, Shortest };
 
static void handleFlags(const char*& format, Flags& flags)
{
    while (1) {
        switch (*format) {
        case '-': flags.leftJustify = true; break;
        case '+': flags.plus = true; break;
        case ' ': flags.space = true; break;
        case '#': flags.alt = true; break;
        case '0': flags.zeroPad = true; break;
        default: return;
        }
        ++format;
    }
}

static int32_t parseNumber(const char*& format)
{
    int32_t n = 0;
    bool first = true;
    while (1) {
        if (*format < '0' || *format > '9') {
            return first ? -1 : n;
        }
        n = n * 10 + *format++ - '0';
        first = false;
    }
}

static uint32_t handleWidth(const char*& format, va_list va)
{
    if (*format == '*') {
        ++format;
        return va_arg(va, int);
    }

    return parseNumber(format);
}

enum class Length { None, H, HH, L, LL, J, Z, T };

static Length handleLength(const char*& format)
{
    Length length = Length::None;
    switch (*format) {
    case 'h':
        if (*++format == 'h') {
            ++format;
            length = Length::HH;
        } else {
            length = Length::H;
        }
        break;
    case 'l':
        if (*++format == 'l') {
            ++format;
            length = Length::LL;
        } else {
            length = Length::L;
        }
        break;
    case 'j':
        length = Length::J;
        break;
    case 'z':
        length = Length::Z;
        break;
    case 't':
        length = Length::T;
        break;
    default: return length;
    }
    ++format;
    return length;
}

static uintmax_t getInteger(Length length, va_list va)
{
    switch(length) {
    case Length::None: return static_cast<uintmax_t>(va_arg(va, int));
    case Length::H: return static_cast<uintmax_t>(va_arg(va, int)) & 0xffff;
    case Length::HH: return static_cast<uintmax_t>(va_arg(va, int)) & 0xff;
    case Length::L: return static_cast<uintmax_t>(va_arg(va, long int));
    case Length::LL: return static_cast<uintmax_t>(va_arg(va, long long int));
    case Length::J: return static_cast<uintmax_t>(va_arg(va, intmax_t));
    case Length::Z: return static_cast<uintmax_t>(va_arg(va, size_t));
    case Length::T: return static_cast<uintmax_t>(va_arg(va, ptrdiff_t));
    }
    return 0;
}

static int32_t outInteger(bare::Print::Printer printer, uintmax_t value, Signed sign, int32_t width, int32_t precision, Flags flags, uint8_t base, bare::Print::Capital cap)
{
    uint32_t size = 0;
    if (sign == Signed::Yes) {
        if (value < 0) {
            value = -value;
            printer('-');
            size = 1;
            width--;
        }
    }
    
    if (flags.alt && base != 10) {
        printer('0');
        size++;
        width--;
        if (base == 16) {
            printer((cap == bare::Print::Capital::Yes) ? 'X' : 'x');
            size++;
            width--;
        }
    }
    
    char buf[bare::Print::MaxIntegerBufferSize];
    char* p = buf;
    size += bare::Print::printString([&p](char c) { *p++ = c; }, static_cast<uint64_t>(value), base, cap);
    
    if (flags.zeroPad) {
        int32_t pad = static_cast<int32_t>(width) - static_cast<int32_t>(strlen(buf));
        while (pad > 0) {
            printer('0');
            size++;
            pad--;
        }
    }
    
    for (const char* p = buf; *p; ++p) {
        printer(*p);
    }

    return size;
}

static int32_t outFloat(bare::Print::Printer printer, double value, int32_t width, int32_t precision, Flags flags, bare::Print::Capital cap, FloatType type)
{
    // FIXME: Handle flags.leftJustify
    // FIXME: Handle flags.plus
    // FIXME: Handle flags.space
    // FIXME: Handle flags.alt
    // FIXME: Handle flags.zeroPad
    // FIXME: Handle width
    // FIXME: Handle precision
    return bare::Print::printString(printer, value, precision, cap);
}

static int32_t outString(bare::Print::Printer printer, const char* s, int32_t width, int32_t precision, Flags flags)
{
    // FIXME: Handle flags.leftJustify
    // FIXME: Handle width
    // FIXME: Handle precision
    int32_t size = 0;
    while (*s) {
        printer(*s++);
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
 
int32_t bare::Print::printfCore(bare::Print::Printer printer, const char *format, va_list va)
{
    assert(format);
    
    Flags flags;
    flags.value = 0;
        
    int32_t size = 0;
    
    while (*format) {
        if (*format != '%') {
            printer(*format++);
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
            size += outInteger(printer, getInteger(length, va), Signed::Yes, width, precision, flags, 10, Capital::No);
            break;
        case 'u':
            size += outInteger(printer, getInteger(length, va), Signed::No, width, precision, flags, 10, Capital::No);
            break;
        case 'o':
            size += outInteger(printer, getInteger(length, va), Signed::No, width, precision, flags, 8, Capital::No);
            break;
        case 'x':
        case 'X':
            size += outInteger(printer, getInteger(length, va), Signed::No, width, precision, flags, 16, (*format == 'X') ? Capital::Yes : Capital::No);
            break;
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G': {
            Capital cap = Capital::No;
            FloatType type = FloatType::Shortest;
            switch(*format)
            {
            case 'f': cap = Capital::No; type = FloatType::Float; break;
            case 'F': cap = Capital::Yes; type = FloatType::Float; break;
            case 'e': cap = Capital::No; type = FloatType::Exp; break;
            case 'E': cap = Capital::Yes; type = FloatType::Exp; break;
            case 'g': cap = Capital::No; type = FloatType::Shortest; break;
            case 'G': cap = Capital::Yes; type = FloatType::Shortest; break;
            }
            size += outFloat(printer, va_arg(va, double), width, precision, flags, cap, type);
            break;
        }
        case 'c':
            printer(static_cast<char>(va_arg(va, int)));
            size++;
            break;
        case 's':
            size += outString(printer, va_arg(va, const char*), width, precision, flags);
            break;
        case 'p':
            size += outInteger(printer, reinterpret_cast<int64_t>(va_arg(va, void*)), Signed::No, width, precision, flags, 16, Capital::No);
            break;
        default:
            printer(*format++);
            size++;
            break;
        }
        ++format;
    }
    
    return size;
}
