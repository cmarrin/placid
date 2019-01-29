/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Formatter.h"

#include <cassert>

using namespace bare;

static uint32_t mantissaToString(const char* mantissa, char* buf, int32_t digitsToLeft)
{
    const char* p = buf;
    char* trailingNonZeroDigit = nullptr;
    
    if (digitsToLeft == 0) {
        *buf++ = '0';
    }
    
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

    if (buf[-1] == '.') {
        buf[-1] = '\0';
    }
    return static_cast<uint32_t>(buf - p);
}

static uint32_t mantissaToString(const char* mantissa, bare::Formatter::Generator gen, int32_t digitsToLeft)
{
    char buf[bare::Formatter::MaxToStringBufferSize];
    uint32_t size = mantissaToString(mantissa, buf, digitsToLeft);
    for (char* p = buf; *p; ++p) {
        gen(*p);
    }
    return size;
}

static void truncateNumber(char* buf, int32_t numDigits, int32_t digitsToTruncate)
{
    if (digitsToTruncate > 0) {
        buf[numDigits - digitsToTruncate] = '\0';
    }
}

uint32_t Formatter::printString(Generator gen, Float v, int32_t precision, Capital cap, uint8_t flags)
{
    if (v == Float()) {
        gen('0');
        gen('\0');
        return 1;
    }
    
    // Round to precision
    if (precision < 0) {
        precision = 6;
    } else if (precision > 16) {
        precision = 16;
    }
    
    Float multiplier = 1;
    for (int32_t i = 0; i < precision; ++i) {
        multiplier *= 10;
    }
    
    v += Float(0.5) / multiplier;
    
    uint32_t size = 0;
    int16_t exponent;    
    char buf[20];
    v.toString(buf, exponent);

    if (v < Float()) {
        gen('-');
        size++;
    }
        
    int32_t numDigits = static_cast<int32_t>(strlen(buf));

    // Compute n as exponent if form is 1.xxxe<n>
    int16_t n = numDigits - 1 + exponent;
    
    if (n >= -4 && n <= 6) {
        // no exponent
        truncateNumber(buf, numDigits, numDigits - n - precision - 1);
        return mantissaToString(buf, gen, n + 1) + size;
    }
    
    truncateNumber(buf, numDigits, numDigits - precision - 1);
    size += mantissaToString(buf, gen, 1);
    gen((cap == bare::Formatter::Capital::Yes) ? 'E' : 'e');
    size++;

    if (n < 0) {
        gen('-');
        size++;
        n = -n;
    }
    
    size += printString(gen, static_cast<uint64_t>(n));
    gen('\0');
    return size;
}
