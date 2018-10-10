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

using namespace bare;

// Decompose a double into an exponent and an integet mantissa.
// The mantissa is between 1 and 10 and multiplied by 100,000,000
// to fit in an int32_t. So the maximum number of digits that can
// be displayed is 9.

static void decompose(double v, int32_t& mantissa, int32_t& exponent, uint16_t maxDecimalPlaces)
{
    // Get number to between 0.5 and 1
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
    mantissa = static_cast<int32_t>(v * 1e19 + 0.5);
 
    // Round mantissa according to the number of digits to display
    uint16_t digits = maxDecimalPlaces + ((exponent > 0) ? exponent : 0);
    if (digits <= 19) {
        mantissa += 50 * (19 - digits);
    }
    
    if (sign) {
        mantissa = -mantissa;
    }
}

// dp defines where the decimal point goes. If it's 0
// the decimal point goes to the immediate left of the 
// mantissa (0.xxx). Negative numbers have leading 0's
// to the left of the mantissa (e.g., -3 is 0.000xxx)
// and positive numbers have that many digits to the
// left of the dp (e.g., 2 is xx.xxxx)

static char* intToString(uint64_t mantissa, char* str, int16_t dp, uint16_t maxDecimalPlaces)
{
    bool leadingDigit = true;
    bool decimalDigits = false;
    
    if (dp <= 0) {
        leadingDigit = false;
        *str++ = '0';
        *str++ = '.';
        while (dp < 0) {
            *str++ = '0';
            ++dp;
        }
        decimalDigits = true;
    }
 
    if (!mantissa) {
        *str++ = '0';
        return str;
    }
    
    while (mantissa || dp > 0) {
        int64_t digit = mantissa / 1000000000000000000;
        mantissa -= digit * 1000000000000000000;
        mantissa *= 10;
        
        if (digit > 9) {
            // The only way this should be able it to happen is
            // when we have a uint64_t that is larger than 19 digits
            // In this case we need to show an extra digit. For now
            // we assume this extra digit is '1' and we subtract
            // 10 from the digit
            *str++ = '1';
            digit -= 10;
        }
        
        if (decimalDigits) {
            if (maxDecimalPlaces-- == 0) {
                return str;
            }
        }
  
        // If this is the leading digit and '0', skip it
        if (!leadingDigit || digit != 0) {
            *str++ = static_cast<char>(digit) + '0';
            leadingDigit = false;
        }
    
        if (dp > 0) {
            if (--dp == 0 && mantissa != 0) {
                *str++ = '.';
                decimalDigits = true;
            }
        }
    }
    return str;
}

bool Print::toString(char* buf, double v)
{
    static constexpr uint16_t MaxDecimalDigits = 6;
    char* p = buf;

    if (v == 0) {
        *p++ = '0';
        *p++ = '\0';
        return true;
    }
    
    int32_t mantissa;
    int32_t exponent;
    decompose(v, mantissa, exponent, MaxDecimalDigits);
    
    if (mantissa < 0) {
        *p++ = '-';
        mantissa = -mantissa;
    }
    
 
    if (exponent >= -3 && exponent <= 5) {
        // no exponent
        buf = intToString(static_cast<uint64_t>(mantissa), p, exponent, MaxDecimalDigits);
        *buf = '\0';
        return true;
    }
    
    // Show 1.xxxeyy
    buf = intToString(static_cast<uint32_t>(mantissa), p, 1, MaxDecimalDigits);
    *buf++ = 'e';
 
    // Assume exp is no more than 3 digits. To move
    // it to the upper 3 digits of an int32_t we
    // multiply by 1000000 and indicate that there
    // are 3 digits
    exponent--;
    if (exponent < 0) {
        *buf++ = '-';
        exponent = -exponent;
    }
    buf = intToString(static_cast<uint32_t>(exponent * 1000000), p, 3, 6);
    *buf = '\0';
    return true;
}

bool Print::toString(char* buf, int32_t v)
{
    if (v < 0) {
        *buf++ = '-';
        v = -v;
    }
 
    return toString(buf, static_cast<uint32_t>(v));
}

bool Print::toString(char* buf, uint32_t v, uint8_t base)
{
    buf = intToString(static_cast<uint32_t>(v), buf, 19, 6);
    *buf = '\0';
    return true;
}

bool Print::toString(char* buf, int64_t v)
{
    if (v < 0) {
        *buf++ = '-';
        v = -v;
    }
    
    return toString(buf, static_cast<uint64_t>(v));
}

bool Print::toString(char* buf, uint64_t v, uint8_t base)
{
    buf = intToString(v, buf, 19, 6);
    *buf = '\0';
    return true;
}

class BufferPrinter : public Print::Printer
{
    public:
    BufferPrinter(char* buf, size_t count) : _buf(buf), _count(count) { }
    
    virtual int32_t outChar(char c) override
    {
        if (_count > 0) {
            *_buf++ = c;
            --_count;
            return 1;
        }
        return -1;
    }
    
    private:
    char* _buf;
    size_t _count;
};

int32_t Print::vsnprintf(char* buffer, size_t count, const char* format, va_list va)
{
    BufferPrinter p(buffer, count);
    return vsnprintCore(p, format, va);
}

int32_t Print::snprintf(char* buffer, size_t count, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    int32_t result = vsnprintf(buffer, count, format, va);
    va_end(va);
    return result;
}
