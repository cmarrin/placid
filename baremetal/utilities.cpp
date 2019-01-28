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

using namespace bare;

extern "C" {

    void* memset(void* dst, int value, size_t n)
    {
        if (n == 0) {
            return dst;
        }
        uint8_t* p = reinterpret_cast<uint8_t*>(dst);
        while (n--) {
            *p++ = static_cast<uint8_t>(value);
        }
        return dst;
    }

    void* memcpy(void* dst, const void* src, size_t n)
    {
        if (n == 0) {
            return dst;
        }
        uint8_t* d = reinterpret_cast<uint8_t*>(dst);
        const uint8_t* s = reinterpret_cast<const uint8_t*>(src);
        while (n--) {
            *d++ = *s++;
        }
        return dst;
    }

    void* memmove(void* dst, const void* src, size_t n)
    {
        if (n == 0) {
            return dst;
        }
        
        if (dst == src) {
            return dst;
        }

        const char* s = reinterpret_cast<const char*>(src);
        char* d = reinterpret_cast<char*>(dst);

        // If the areas don't overlap, just do memcpy
        if (s < d && d < s + n) {
            // If the areas overlap, have to copy byte by byte
            s += n;
            d += n;
            while (n-- > 0) {
                *--d = *--s;
            }
            return dst;
        }
        
        // just do memcpy
        return bare::memcpy(dst, src, n);
    }

    int memcmp(const void* left, const void* right, size_t n)
    {
        const char *s1 = reinterpret_cast<const char*>(left);
        const char *s2 = reinterpret_cast<const char*>(right);
        while (n--) {
            if (*s1 < *s2) {
                return -1;
            } else if (*s1 > *s2) {
                return 1;
            }
            s1++;
            s2++;
        }
        return 0;
    }

    size_t strlen(const char* str)
    {
        const char* s;
        for (s = str; *s; ++s) { }
        return (unsigned int)(s - str);
    }

    char* strcpy(char* dst, const char* src)
    {
        char* ret = dst;
        while (*src) {
            *dst++ = *src++;
        }
        return ret;
    }

    int strcmp(const char* s1, const char* s2)
    {
        char c1;
        char c2;
        while (true) {
            c1 = *s1++;
            c2 = *s2++;
            if (c1 != c2) {
                break;
            }
            if (c1 == '\0') {
                return 0;
            }
        }
        return c1 - c2;
    }

}
