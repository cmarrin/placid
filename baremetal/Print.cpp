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

#include "bare/Print.h"

#include "bare/PrintfCore.h"
#include <cassert>

using namespace bare;

static uint32_t mantissaToString(const char* mantissa, char* buf, int32_t digitsToLeft)
{
    const char* p = buf;
    char* trailingNonZeroDigit = nullptr;
    
    while (*mantissa) {
        if (digitsToLeft-- == 0) {
            *buf++ = '.';
        }
    
        if (*mantissa == '0' && digitsToLeft <= 0) {
            if (!trailingNonZeroDigit) {
                trailingNonZeroDigit = buf;
            }
        } else {
            trailingNonZeroDigit = nullptr;
        }
        *buf++ = *mantissa;
        
        ++mantissa;
    }
    
    if (trailingNonZeroDigit) {
        buf = trailingNonZeroDigit;
    }
    *buf = '\0';
    
    return static_cast<uint32_t>(buf - p);
}

static uint32_t mantissaToString(const char* mantissa, bare::Print::Printer printer, int32_t digitsToLeft)
{
    char buf[bare::Print::MaxToStringBufferSize];
    uint32_t size = mantissaToString(mantissa, buf, digitsToLeft);
    for (char* p = buf; *p; ++p) {
        printer(*p);
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

int32_t Print::printfCore(Printer printer, const char *format, va_list va)
{
    return PrintfCore::format(printer, format, va);
}

uint32_t Print::printString(Printer printer, Float v, int32_t precision, Capital cap)
{
    if (v == Float()) {
        printer('0');
        printer('\0');
        return 1;
    }
    
    uint32_t size = 0;
    int16_t exponent;    
    char buf[20];
    v.toString(buf, exponent);

    // FIXME: Round using precision
    
    if (v < Float()) {
        printer('-');
        size++;
    }
        
    int32_t numDigits = static_cast<int32_t>(strlen(buf));

    // Compute n as exponent if form is 1.xxxe<n>
    int16_t n = numDigits - 1 + exponent;
    
    if (n >= -4 && n <= 6) {
        // no exponent
        return mantissaToString(buf, printer, n + 1) + size;
    }
    
    size += mantissaToString(buf, printer, 1);
    printer((cap == bare::Print::Capital::Yes) ? 'E' : 'e');
    size++;

    if (n < 0) {
        printer('-');
        size++;
        n = -n;
    }
    
    size += intToString(n, printer);
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
    return PrintfCore::format([&buffer, &count](char c)
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
