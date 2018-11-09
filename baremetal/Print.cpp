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

#include "PrintfCore.h"
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

int32_t Print::printfCore(Printer printer, const char *format, va_list va)
{
    return PrintfCore::format(printer, format, va);
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
