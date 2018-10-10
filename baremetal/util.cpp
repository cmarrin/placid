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

#include "Serial.h"
#include "Timer.h"

using namespace bare;

#ifdef __APPLE__
#include <stdio.h>
#include <sys/time.h>

extern "C" {

void PUT8(unsigned int addr, unsigned int value)
{
    printf("PUT8:[%d] <= %d\n", addr, value);
}

void BRANCHTO(unsigned int addr)
{
    printf("BRANCHTO: => %d\n", addr);
}

bool interruptsSupported()
{
    return false;
}

}

#else

void disableIRQ()
{
    __asm volatile (
        "mrs r0,cpsr\n"
        "orr r0,r0,#0x80\n"
        "msr cpsr_c,r0\n"
        "bx lr" : : : "r0"
    );
}

void enableIRQ()
{
    __asm volatile (
        "mrs r0,cpsr\n"
        "bic r0,r0,#0x80\n"
        "msr cpsr_c,r0\n"
        "bx lr" : : : "r0"
    );
}

void WFE()
{
    __asm volatile (
        "wfe\n"
        "bx lr"
    );
}
#endif

extern "C" {
uint64_t __aeabi_uidivmod(unsigned int value, unsigned int divisor) {
        uint64_t answer = 0;

        unsigned int i;
        for (i = 0; i < 32; i++) {
                if ((divisor << (31 - i)) >> (31 - i) == divisor) {
                        if (value >= divisor << (31 - i)) {
                                value -= divisor << (31 - i);
                                answer |= (uint64_t)(1 << (31 - i));
                                if (value == 0) break;
                        } 
                }
        }

        answer |= (uint64_t)value << 32;
        return answer;
}

unsigned int __aeabi_uidiv(unsigned int value, unsigned int divisor) {
        return (unsigned int)__aeabi_uidivmod(value, divisor);
}

void abort()
{
    Serial::printf("***********ABORTING**********\n");
    Timer::usleep(500000);
    while (1) ;
}

void __assert_func(const char *file, int line, const char *func, const char *what) {
    Serial::printf("Assertion failed: %s, function %s, file %s, line %d.\n", what, func, file, line);
    abort();
}

void __aeabi_idiv0()
{
    abort();
}

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
    return memcpy(dst, src, n);
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

const char* strstr(const char* s1, const char* s2)
{
    int i, j;

    if ((s1 == nullptr || s2 == nullptr)) {
        return nullptr;
    }

    for( i = 0; ; i++) {
        char c1 = s1[i];
        if (c1 == '\0') {
            return nullptr;
        }
        
        char c2 = *s2;
        if (c1 == c2) {
            for (j = i; ; j++) {
                c2 = s2[j - i];
                if (c2 == '\0') {
                    return s1 + i;
                }
                c1 = s1[j];
                if (c1 != c2) {
                    break;
                }
            }
        }
    }
}

}

namespace std {
    void __throw_bad_function_call()
    {
        abort();
    }
}

void convertTo8dot3(char* name8dot3, const char* name)
{
    // Find the dot
    int dot = 0;
    const char* p = name;
    while (*p) {
        if (*p == '.') {
            dot = static_cast<int>(p - name);
            break;
        }
        p++;
        dot++;
    }
    
    if (dot <= 8) {
        // We have the simple case
        int index = 0;
        for (int i = 0; i < 8; ++i) {
            name8dot3[i] = (index < dot) ? toupper(name[index++]) : ' ';
        }
    } else {
        // We need to add '~1'
        for (int i = 0; i < 8; ++i) {
            if (i < 6) {
                name8dot3[i] = toupper(name[i]);
            } else if (i == 6) {
                name8dot3[i] = '~';
            } else {
                name8dot3[i] = '1';
            }
        }
    }

    // Now add the extension
    if (name[dot] == '.') {
        dot++;
    }
    
    for (int i = 8; i < 11; ++i) {
        name8dot3[i] = name[dot] ? toupper(name[dot++]) : ' ';
    }
    
    name8dot3[11] = '\0';
}
