/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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
