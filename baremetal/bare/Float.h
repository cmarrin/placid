/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
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

#include "fpconv.h"
#include <utility>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <limits>

namespace bare {

inline void mult64to128(uint64_t op1, uint64_t op2, uint64_t& hi, uint64_t& lo)
{
    uint64_t u1 = (op1 & 0xffffffff);
    uint64_t v1 = (op2 & 0xffffffff);
    uint64_t t = (u1 * v1);
    uint64_t w3 = (t & 0xffffffff);
    uint64_t k = (t >> 32);

    op1 >>= 32;
    t = (op1 * v1) + k;
    k = (t & 0xffffffff);
    uint64_t w1 = (t >> 32);

    op2 >>= 32;
    t = (u1 * op2) + k;
    k = (t >> 32);

    hi = (op1 * op2) + w1 + k;
    lo = (t << 32) + w3;
}

template<typename RawType, typename DecomposeType, typename ArgType, int32_t BinExp = 0, int32_t DecExp = 0>
class _Float
{
public:
    using value_type = RawType;
    using decompose_type = DecomposeType;
    using arg_type = ArgType;
    
private:
    static constexpr decompose_type exp(decompose_type v, decompose_type base, decompose_type n) { return n ? exp(v * base, base, n - 1) : v; }

public:
    static constexpr decompose_type BinaryExponent = BinExp;
    static constexpr decompose_type MaxIntegerBits = sizeof(value_type) * 8 - BinaryExponent;
    static constexpr decompose_type BinaryMultiplier = exp(1, 2, BinaryExponent);
    static constexpr decompose_type BinaryMask = (1L << BinaryExponent) - 1;
    static constexpr decompose_type DecimalExponent = DecExp;
    static constexpr decompose_type DecimalMultiplier = exp(1, 10, DecimalExponent);
    static constexpr decompose_type MaxDigits = (sizeof(value_type) <= 32) ? 8 : 12;
    
    // Constructors
    //
    _Float() { _value = 0; }
    _Float(const _Float& value) { _value = value._value; }
    
    explicit _Float(bool value) { _value = value ? (static_cast<value_type>(1) * BinaryMultiplier) : 0; }
    explicit _Float(int32_t value) { _value = static_cast<value_type>(value) * BinaryMultiplier; }

    _Float(double d)
    {
        union {
            double number;
            uint64_t integer;
            struct
            {
                uint64_t frac : 52;
                int16_t exp : 11;
                bool sign : 1;
            };
        } value;
        
        value.number = d;
        value.exp -= 1023;
        
        if (value.exp < -BinaryExponent || value.exp > MaxIntegerBits) {
            _value = 0;
            return;
        }
        
        uint64_t frac = value.frac;
        frac |= 0x10000000000000;
        
        int32_t shift = 52 - value.exp - BinaryExponent;
        if (shift < 0) {
            frac <<= -shift;
        } else if (shift > 0) {
            frac >>= shift - 1;
            frac += 1;
            frac >>= 1;
        }

        _value = static_cast<value_type>(frac);
        if (value.sign) {
            _value = -_value;
        }
    }
    
    // Convert Float to arg_type for passing to printf
    arg_type toArg() const { return static_cast<arg_type>(_value); }
    
    // Get the next arg from the va_list and convert it to a Float
    static _Float argToFloat(va_list va)
    {
        _Float floatValue;
        floatValue._value = static_cast<value_type>(va_arg(va, arg_type));
        return floatValue;
    }

    operator bool() const { return _value != 0; }

    const _Float& operator=(const _Float& other) { _value = other._value; return *this; }
    _Float& operator=(_Float& other) { _value = other._value; return *this; }
    
    _Float operator+(const _Float& other) const { _Float r; r._value = _value + other._value; return r; }
    _Float operator-(const _Float& other) const { _Float r; r._value = _value - other._value; return r; }

    _Float operator*(const _Float& other) const
    {
        _Float r;
        int64_t result = static_cast<uint64_t>(_value) * other._value >> BinaryExponent;
        r._value = static_cast<value_type>(result);
        return r;
    }
    _Float operator/(const _Float& other) const
    {
        // FIXME: Have some sort of error on divide by 0
        if (other._value == 0) {
            return _Float();
        }
        _Float r;
        int64_t result = (static_cast<int64_t>(_value) << BinaryExponent) / other._value;
        r._value = static_cast<value_type>(result);
        return r;
    }

    _Float operator%(const _Float& other) const { return *this - (*this / other).floor() * other; }
    
    _Float operator+=(const _Float& other) { *this = *this + other; return *this; }
    _Float operator-=(const _Float& other) { *this = *this - other; return *this; }
    _Float operator*=(const _Float& other) { *this = *this * other; return *this; }
    _Float operator/=(const _Float& other) { *this = *this / other; return *this; }
    _Float operator%=(const _Float& other) { *this = *this % other; return *this; }
    
    _Float floor() const { _Float r; r._value = _value >> BinaryExponent << BinaryExponent; return r; }
    operator int32_t() const { return static_cast<int32_t>(_value >> BinaryExponent); }
    operator int64_t() const { return static_cast<int64_t>(_value >> BinaryExponent); }

    // Returned string is numeric with exp determining how many digits from the right end to place the decimal point
    void toString(char* buf, int16_t& exponent) const
    {
        // Convert to base 10
        value_type intPart = _value / BinaryMultiplier;
        value_type fracPart = _value % BinaryMultiplier;
        
        value_type value = intPart * DecimalMultiplier;
        value += ((fracPart * DecimalMultiplier) + (DecimalMultiplier / 2)) / BinaryMultiplier;
        exponent = -DecimalExponent;
        
        // Generate the string backwards then reverse it
        int16_t i = 0;
        for ( ; value || i < -exponent; ++i) {
            value_type digit = value % 10;
            buf[i] = digit + '0';
            value /= 10;
        }
        for (int16_t j = 0; j < i / 2; ++j) {
            std::swap(buf[j], buf[i - j - 1]);
        }
        buf[i] = '\0';
    }

    _Float operator%(const _Float& other) { return *this - other * (*this / other).floor(); }
    
    bool operator==(const _Float& other) const { return _value == other._value; }
    bool operator!=(const _Float& other) const { return _value != other._value; }
    bool operator<(const _Float& other) const { return _value < other._value; }
    bool operator<=(const _Float& other) const { return _value <= other._value; }
    bool operator>(const _Float& other) const { return _value > other._value; }
    bool operator>=(const _Float& other) const { return _value >= other._value; }

    _Float operator-() const { _Float r; r._value = -_value; return r; }

private:    
    value_type _value;
};

// Specializations
//
// Float and Double specialize several functions to deal with the fact that they are
// true floating point and not fixed point. Functions specialized are:
//

// float specializations

template<>
inline _Float<float, int32_t, double>::_Float(bool value) { _value = value ? 1 : 0; }

template<>
inline _Float<float, int32_t, double>::_Float(double d) { _value = static_cast<value_type>(d); }

template<>
inline void _Float<float, int32_t, double>::toString(char* buf, int16_t& exponent) const
{
    int size = fpconv_dtoa(_value, buf, exponent);
    buf[size] = '\0';
}

template<>
inline _Float<float, int32_t, double> _Float<float, int32_t, double>::operator*(const _Float& other) const
{
    _Float r;
    r._value = _value * other._value;
    return r;
}

template<>
inline _Float<float, int32_t, double> _Float<float, int32_t, double>::operator/(const _Float& other) const
{
    _Float r;
    r._value = _value / other._value;
    return r;
}

template<>
inline _Float<float, int32_t, double> _Float<float, int32_t, double>::floor() const
{
    _Float r;
    r._value = static_cast<float>(static_cast<int32_t>(_value));
    return r;
}

// double specializations

template<>
inline _Float<double, int64_t, double>::_Float(bool value) { _value = value ? 1 : 0; }

template<>
inline _Float<double, int64_t, double>::_Float(double d) { _value = d; }

template<>
inline void _Float<double, int64_t, double>::toString(char* buf, int16_t& exponent) const
{
    int size = fpconv_dtoa(_value, buf, exponent);
    buf[size] = '\0';
}

template<>
inline _Float<double, int64_t, double> _Float<double, int64_t, double>::operator*(const _Float& other) const
{
    _Float r;
    r._value = _value * other._value;
    return r;
}

template<>
inline _Float<double, int64_t, double> _Float<double, int64_t, double>::operator/(const _Float& other) const
{
    _Float r;
    r._value = _value / other._value;
    return r;
}

template<>
inline _Float<double, int64_t, double> _Float<double, int64_t, double>::floor() const
{
    _Float r;
    r._value = static_cast<double>(static_cast<int64_t>(_value));
    return r;
}

// int64_t specializations
template<>
inline _Float<int64_t, int64_t, int64_t, 30, 7> _Float<int64_t, int64_t, int64_t, 30, 7>::operator*(const _Float& other) const
{
    if (_value > -std::numeric_limits<int32_t>::max() && _value < std::numeric_limits<int32_t>::max() &&
        other._value > -std::numeric_limits<int32_t>::max() && other._value < std::numeric_limits<int32_t>::max()) {
        _Float r;
        int64_t result = static_cast<uint64_t>(_value) * other._value >> BinaryExponent;
        r._value = static_cast<int64_t>(result);
        return r;
    }
    // Do it the slow way
    bool signA = _value < 0;
    bool signB = other._value < 0;
    uint64_t a = static_cast<uint64_t>(_value);
    uint64_t b = static_cast<uint64_t>(other._value);
    if (signA) {
        a = -a;
    }
    if (signB) {
        b = -b;
    }
    mult64to128(a, b, a, b);
    if (a >= (1 << 26)) {
        // Handle out of range error
        return _Float();
    }
    b = (b >> BinaryExponent) + (a << (64 - BinaryExponent));
    if (signA ^ signB) {
        b = -b;
    }
    _Float r;
    r._value = b;
    return r;
}

template<>
inline _Float<int64_t, int64_t, int64_t, 30, 7> _Float<int64_t, int64_t, int64_t, 30, 7>::operator/(const _Float& other) const
{
    if (_value > -std::numeric_limits<int32_t>::max() && _value < std::numeric_limits<int32_t>::max()) {
        if (other._value == 0) {
            return _Float();
        }
        _Float r;
        int64_t result = (static_cast<int64_t>(_value) << BinaryExponent) / other._value;
        r._value = result;
        return r;
    }

    // do *this * 1 / other
    _Float inv;
    inv._value = (1LL << 62) / other._value;
    _Float r = inv * *this;
    r._value = ((r._value >> (62 - (30 * 2) - 1)) + 1) >> 1;
    return r;
}

// Float32     - +/- 2e6 with 3 decimal digits of precision
// Float64     - +/- 8e9 with 8 decimal digits of precision
// FloatFloat  - IEEE single precision floating point
// FloatDouble - IEEE double precision floating point

// Range is +/- 8e9 with a precision of 1e-9. When used as a Value
// the LSB is the type, so we lose a bit. That leaves us with 5e-8
// precision. That safely gives us 8 decimal digits of precision.
using Float64 = _Float<int64_t, int64_t, int64_t, 30, 7>;
using Float32 = _Float<int32_t, int32_t, int32_t, 10, 3>;
using FloatFloat = _Float<float, int32_t, double>;
using FloatDouble = _Float<double, int64_t, double>;
using FloatNone = _Float<int, int, int>;

}
