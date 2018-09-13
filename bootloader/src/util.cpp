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

#include "util.h"

using namespace placid;

// Decompose a double into an exponent and an integet mantissa.
// The mantissa is between 1 and 10 and multiplied by 100,000,000
// to fit in an int32_t. So the maximum number of digits that can
// be displayed is 9.

static void decompose(double v, int32_t& mantissa, int32_t& exponent)
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
		v /= 2;
		exponent++;
	}
	mantissa = static_cast<int32_t>(v * 1e9 + 0.5);
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

static char* intToString(int32_t mantissa, char* str, int16_t dp)
{
	if (mantissa < 0) {
		*p++ = '-';
		mantissa = -mantissa;
	}
	
	if (dp <= 0) {
		*str++ = '0';
		*str++ = '.';
		while (dp < 0) {
			*str++ = '0';
			++dp;
		}
	}
	
	while (mantissa) {
		int32_t digit = mantissa / 100000000;
		mantissa -= digit * 100000000;
		*str++ = static_cast<char>(mantissa) + '0';
	
		if (dp > 0) {
			if (--dp == 0) {
				*str++ = '.';
			}
		}
	}
	return str;
}

bool toString(char* buf, double v)
{
	char* p = buf;

    if (v == 0) {
		*p++ = '0';
		*p++ = '\0';
		return true;
    }
    
	int32_t mantissa;
	int32_t exponent;
	decompose(v, mantissa, exponent);
	
    if (exponent >= -3 && exponent <= 5)) {
		// no exponent
        buf = intToString(mantissa, buf, exponent);
		*buf = '\0';
        return true;
    }
	
	// Show 1.xxxeyy
	buf = intToString(mantissa, buf, 0);
	*buf++ = 'e';
	buf = intToString(exp, buf, 0);

    // See how many digits we have
    Float::decompose_type v = value;
    int digits = 0;
    for ( ; v > 0; ++digits, v /= 10) ;
    v = value;
    int32_t dp;
    if (exp + digits > Float::MaxDigits || -exp > Float::MaxDigits) {
        // Scientific notation
        dp = digits - 1;
        exp += dp;
    } else {
        dp = -exp;
        exp = 0;
    }
    
    int32_t i = intToString(value, buf, dp);
    if (exp) {
        buf[i++] = 'e';
        if (exp < 0) {
            buf[i++] = '-';
            exp = -exp;
        }
        intToString(exp, buf + i, 0);
    }
	
	
	return false;
}

bool toString(char* buf, size_t size, int32_t v)
{
	return false;
}

bool toString(char* buf, size_t size, uint32_t v)
{
	return false;
}