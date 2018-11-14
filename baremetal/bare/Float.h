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

#include <cmath>
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
private:
    static constexpr int32_t exp(int32_t v, int32_t n) { return n ? exp(v * 10, n - 1) : v; }

public:
    class Raw
    {
        friend class _Float;

    private:
        RawType _raw;
    };
    
    typedef RawType value_type;
    typedef DecomposeType decompose_type;
    typedef ArgType arg_type;
    
    static constexpr int32_t BinaryExponent = BinExp;
    static constexpr value_type BinaryMask = (1L << BinaryExponent) - 1;
    static constexpr int32_t DecimalExponent = DecExp;
    static constexpr value_type DecimalMultiplier = exp(1, DecExp);
    static constexpr uint8_t MaxDigits = (sizeof(RawType) <= 32) ? 8 : 12;
    
    _Float() { _value._raw = 0; }
    explicit _Float(Raw value) { _value._raw = value._raw; }
    _Float(const _Float& value) { _value._raw = value._value._raw; }
    _Float(_Float& value) { _value._raw = value._value._raw; }
    explicit _Float(RawType value) { _value._raw = value; }
    explicit _Float(bool value) { _value._raw = value ? (static_cast<RawType>(1) << BinaryExponent) : 0; }

    _Float(RawType i, int32_t e)
    {
        if (i == 0) {
            _value._raw = 0;
            return;
        }
        if (e == 0) {
            _value._raw = static_cast<RawType>(static_cast<int64_t>(i) << BinaryExponent);
            return;
        }
        
        int64_t num = static_cast<int64_t>(i) << BinaryExponent;
        int32_t sign = (num < 0) ? -1 : 1;
        num *= sign;
        
        while (e > 0) {
            if (num > std::numeric_limits<value_type>::max()) {
                // FIXME: Number is over range, handle that
                _value._raw = 0;
                return;
            }
            --e;
            num *= 10;
        }
        while (e < 0) {
            if (num == 0) {
                // FIXME: Number is under range, handle that
                _value._raw = 0;
                return;
            }
            ++e;
            num /= 10;
        }
        
        if (num > std::numeric_limits<value_type>::max()) {
            // FIXME: Number is under range, handle that
            _value._raw = 0;
            return;
        }
        
        _value._raw = sign * static_cast<RawType>(num);
    }
    
    RawType raw() const { return _value._raw; 
    }
    static _Float make(uint64_t v)
    {
        Raw r;
        r._raw = *(reinterpret_cast<RawType*>(&v));
        return r;
    }
    operator Raw() const { return _value; }
    
    operator bool() const { return _value._raw != 0; }

    const _Float& operator=(const _Float& other) { _value._raw = other._value._raw; return *this; }
    _Float& operator=(_Float& other) { _value._raw = other._value._raw; return *this; }
    
    _Float operator+(const _Float& other) const { _Float r; r._value._raw = _value._raw + other._value._raw; return r; }
    _Float operator-(const _Float& other) const { _Float r; r._value._raw = _value._raw - other._value._raw; return r; }

    _Float operator*(const _Float& other) const
    {
        _Float r;
        int64_t result = static_cast<uint64_t>(_value._raw) * other._value._raw >> BinaryExponent;
        r._value._raw = static_cast<RawType>(result);
        return r;
    }
    _Float operator/(const _Float& other) const
    {
        // FIXME: Have some sort of error on divide by 0
        if (other._value._raw == 0) {
            return _Float();
        }
        _Float r;
        int64_t result = (static_cast<int64_t>(_value._raw) << BinaryExponent) / other._value._raw;
        r._value._raw = static_cast<RawType>(result);
        return r;
    }

    _Float operator%(const _Float& other) const { return *this - (*this / other).floor() * other; }
    
    _Float operator+=(const _Float& other) { *this = *this + other; return *this; }
    _Float operator-=(const _Float& other) { *this = *this - other; return *this; }
    _Float operator*=(const _Float& other) { *this = *this * other; return *this; }
    _Float operator/=(const _Float& other) { *this = *this / other; return *this; }
    _Float operator%=(const _Float& other) { *this = *this % other; return *this; }
    
    _Float floor() const { _Float r; r._value._raw = _value._raw >> BinaryExponent << BinaryExponent; return r; }
    operator int32_t() const { return static_cast<int32_t>(_value._raw >> BinaryExponent); }
    operator int64_t() const { return static_cast<int64_t>(_value._raw >> BinaryExponent); }

    void decompose(decompose_type& mantissa, int16_t& exponent) const
    {        
        if (_value._raw == 0) {
            mantissa = 0;
            exponent = 0;
            return;
        }
        _Float integerFloat = floor();
        decompose_type integerPart = static_cast<decompose_type>(integerFloat);
        _Float fractionPart = *this - integerFloat;
        decompose_type value = static_cast<decompose_type>(fractionPart._value._raw) * DecimalMultiplier;
        mantissa = ((value >> (BinaryExponent - 1)) + 1) >> 1;
        mantissa += integerPart * DecimalMultiplier;
        exponent = -DecimalExponent;
    }

    _Float operator%(const _Float& other) { return *this - other * (*this / other).floor(); }
    
    bool operator==(const _Float& other) const { return _value._raw == other._value._raw; }
    bool operator!=(const _Float& other) const { return _value._raw != other._value._raw; }
    bool operator<(const _Float& other) const { return _value._raw < other._value._raw; }
    bool operator<=(const _Float& other) const { return _value._raw <= other._value._raw; }
    bool operator>(const _Float& other) const { return _value._raw > other._value._raw; }
    bool operator>=(const _Float& other) const { return _value._raw >= other._value._raw; }

    _Float operator-() const { _Float r; r._value._raw = -_value._raw; return r; }

private:    
    Raw _value;
};

template<>
inline _Float<float, int32_t, double>::_Float(value_type i, int32_t e)
{
    float num = static_cast<float>(i);
    while (e > 0) {
        --e;
        num *= 10;
    }
    while (e < 0) {
        ++e;
        num /= 10;
    }
    _value._raw = num;
}

template<>
inline _Float<float, int32_t, double>::_Float(bool value) { _value._raw = value ? 1 : 0; }

template<>
inline void _Float<float, int32_t, double>::decompose(int32_t& mantissa, int16_t& exponent) const
{
    // FIXME: Implement correctly for fractions
    if (_value._raw == 0) {
        mantissa = 0;
        exponent = 0;
        return;
    }
    int32_t sign = (_value._raw < 0) ? -1 : 1;
    double value = _value._raw * sign;
    int32_t exp = 0;
    while (value >= 1) {
        value /= 10;
        exp++;
    }
    while (value < 0.1) {
        value *= 10;
        exp--;
    }
    mantissa = static_cast<int32_t>(sign * value * 1000000000);
    exponent = exp - 9;
}

template<>
inline _Float<float, int32_t, double> _Float<float, int32_t, double>::operator*(const _Float& other) const
{
    _Float r;
    r._value._raw = _value._raw * other._value._raw;
    return r;
}

template<>
inline _Float<float, int32_t, double> _Float<float, int32_t, double>::operator/(const _Float& other) const
{
    _Float r;
    r._value._raw = _value._raw / other._value._raw;
    return r;
}

template<>
inline _Float<float, int32_t, double> _Float<float, int32_t, double>::floor() const
{
    _Float r;
    r._value._raw = static_cast<float>(static_cast<int32_t>(_value._raw));
    return r;
}

template<>
inline _Float<double, int64_t, double>::_Float(value_type i, int32_t e)
{
    double num = static_cast<double>(i);
    while (e > 0) {
        --e;
        num *= 10;
    }
    while (e < 0) {
        ++e;
        num /= 10;
    }
    _value._raw = num;
}

template<>
inline _Float<double, int32_t, double>::_Float(bool value) { _value._raw = value ? 1 : 0; }

template<>
inline void _Float<double, int64_t, double>::decompose(int64_t& mantissa, int16_t& exponent) const
{
    // FIXME: Implement correctly for fractions
    if (_value._raw == 0) {
        mantissa = 0;
        exponent = 0;
        return;
    }
    int32_t sign = (_value._raw < 0) ? -1 : 1;
    double value = _value._raw * sign;
    int32_t exp = 0;
    while (value >= 1) {
        value /= 10;
        exp++;
    }
    while (value < 0.1) {
        value *= 10;
        exp--;
    }
    mantissa = static_cast<int32_t>(sign * value * 1000000000);
    exponent = exp - 9;
}

template<>
inline _Float<double, int64_t, double> _Float<double, int64_t, double>::operator*(const _Float& other) const
{
    _Float r;
    r._value._raw = _value._raw * other._value._raw;
    return r;
}

template<>
inline _Float<double, int64_t, double> _Float<double, int64_t, double>::operator/(const _Float& other) const
{
    _Float r;
    r._value._raw = _value._raw / other._value._raw;
    return r;
}

template<>
inline _Float<double, int64_t, double> _Float<double, int64_t, double>::floor() const
{
    _Float r;
    r._value._raw = static_cast<double>(static_cast<int64_t>(_value._raw));
    return r;
}

template<>
inline _Float<int64_t, int64_t, int64_t, 30, 7> _Float<int64_t, int64_t, int64_t, 30, 7>::operator*(const _Float& other) const
{
    if (_value._raw > -std::numeric_limits<int32_t>::max() && _value._raw < std::numeric_limits<int32_t>::max() &&
        other._value._raw > -std::numeric_limits<int32_t>::max() && other._value._raw < std::numeric_limits<int32_t>::max()) {
        _Float r;
        int64_t result = static_cast<uint64_t>(_value._raw) * other._value._raw >> BinaryExponent;
        r._value._raw = static_cast<int64_t>(result);
        return r;
    }
    // Do it the slow way
    bool signA = _value._raw < 0;
    bool signB = other._value._raw < 0;
    uint64_t a = static_cast<uint64_t>(_value._raw);
    uint64_t b = static_cast<uint64_t>(other._value._raw);
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
    r._value._raw = b;
    return r;
}

template<>
inline _Float<int64_t, int64_t, int64_t, 30, 7> _Float<int64_t, int64_t, int64_t, 30, 7>::operator/(const _Float& other) const
{
    if (_value._raw > -std::numeric_limits<int32_t>::max() && _value._raw < std::numeric_limits<int32_t>::max()) {
        if (other._value._raw == 0) {
            return _Float();
        }
        _Float r;
        int64_t result = (static_cast<int64_t>(_value._raw) << BinaryExponent) / other._value._raw;
        r._value._raw = result;
        return r;
    }

    // do *this * 1 / other
    _Float inv;
    inv._value._raw = (1LL << 62) / other._value._raw;
    _Float r = inv * *this;
    r._value._raw = ((r._value._raw >> (62 - (30 * 2) - 1)) + 1) >> 1;
    return r;
}

// Float32     - +/- 2e6 with 2 decimal digits of precision
// Float64     - +/- 8e9 with 8 decimal digits of precision
// FloatFloat  - IEEE single precision floating point
// FloatDouble - IEEE double precision floating point

// Range is +/- 8e9 with a precision of 1e-9. When used as a Value
// the LSB is the type, so we lose a bit. That leaves us with 5e-8
// precision. That safely gives us 8 decimal digits of precision.
using Float64 = _Float<int64_t, int64_t, int64_t, 30, 7>;
using Float32 = _Float<int32_t, int32_t, int32_t, 10, 2>;
using FloatFloat = _Float<float, int32_t, double>;
using FloatDouble = _Float<double, int64_t, double>;

}
