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

using namespace placid;

#ifdef __APPLE__
#include <stdio.h>
#include <sys/time.h>
#undef vsnprintf
#undef snprintf

extern "C" {

void PUT8(unsigned int addr, unsigned int value)
{
    printf("PUT8:[%d] <= %d\n", addr, value);
}

unsigned int GET32(unsigned int addr)
{
    printf("GET32:[%d]\n", addr);
    return 0;
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

extern "C" {
unsigned long long __aeabi_uidivmod(unsigned int value, unsigned int divisor) {
        unsigned long long answer = 0;

        unsigned int i;
        for (i = 0; i < 32; i++) {
                if ((divisor << (31 - i)) >> (31 - i) == divisor) {
                        if (value >= divisor << (31 - i)) {
                                value -= divisor << (31 - i);
                                answer |= (unsigned long long)(1 << (31 - i));
                                if (value == 0) break;
                        } 
                }
        }

        answer |= (unsigned long long)value << 32;
        return answer;
}

unsigned int __aeabi_uidiv(unsigned int value, unsigned int divisor) {
        return (unsigned int)__aeabi_uidivmod(value, divisor);
}

unsigned long long __aeabi_uldivmod(unsigned long long value, unsigned long long divisor) {
        unsigned long long answer = 0;

        unsigned int i;
        for (i = 0; i < 32; i++) {
                if ((divisor << (31 - i)) >> (31 - i) == divisor) {
                        if (value >= divisor << (31 - i)) {
                                value -= divisor << (31 - i);
                                answer |= (unsigned long long)(1 << (31 - i));
                                if (value == 0) break;
                        } 
                }
        }

        answer |= (unsigned long long)value << 32;
        return answer;
}

void abort()
{
    while (1) ;
}

void __aeabi_idiv0()
{
    abort();
}

}
#endif

void putch(char c)
{
    Serial::write(c);
}

void putstr(const char* s)
{
    while (*s) {
        putch(*s++);
    }
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

// strHex, strNum, printf, sprintf and vsprintf are adapted from https://github.com/moizumi99/RPiHaribote

//static int strcpy(char *dst, char const *src)
//{
//    int i;
//    i = 0;
//    while(*src!=0) {
//        *(dst++) = *(src++);
//        i++;
//    } 
//    *dst = 0;
//    return i;
//}

//int printf(const char *format, ...)
//{
//    va_list ap;
//    char s[StringGuard];
//    int i;
//
//    va_start(ap, format);
//    i = vsnprintf(s, StringGuard, format, ap);
//    putstr(s);
//    va_end(ap);
//    return i;
//}
//
//int vsnprintf(char *str, size_t size, const char *format, va_list listPointer)
//{
//    char c;
//    int i, si;
//    unsigned int ui;
//    int pf;
//    char *s;
//    int len, fill, sign_flag;
//    
//    i=0;
//    pf = 0;
//    len = 0;
//    fill = 0;
//    while((c=*format++)!=0 && i < static_cast<int>(size)) {
//        if (pf==0) {
//            // after regular character
//            if (c=='%') {
//                pf=1;
//                len = 0;
//                fill = 0;
//            } else {
//                str[i++]=c;
//            }
//        } else if (pf>0) {
//            // previous character was '%'
//            if (c=='x' || c=='X') {
//                ui = va_arg(listPointer, unsigned);
//                i += strHex(str+i, (unsigned) ui, len, fill);
//                pf=0;
//            } else if (c=='u' || c=='U' || c=='i' || c=='I') {
//                ui = va_arg(listPointer, unsigned);
//                i += strNum(str+i, (unsigned) ui, len, fill, 0);
//                pf=0;
//            } else if (c=='d' || c=='D') {
//                si = va_arg(listPointer, int);
//                if (si<0) {
//                    ui = -si;
//                    sign_flag = 1;
//                } else {
//                    ui = si;
//                    sign_flag = 0;
//                }
//                i += strNum(str+i, (unsigned) ui, len, fill, sign_flag);
//                pf=0;
//            } else if (c=='s' || c=='S') {
//                s = va_arg(listPointer, char *);
//                i += strcpy(str+i, s);
//                pf=0;
//            } else if ('0'<=c && c<='9') {
//                if (pf==1 && c=='0') {
//                    fill = 1;
//                } else {
//                    len = len*10+(c-'0');
//                }
//                pf=2;
//            } else {
//                // this shouldn't happen
//                str[i++]=c;
//                pf=0;
//            }
//        }
//    }
//    str[i]=0;
//    return (i>0) ? i : -1;
//}
//
//int snprintf(char *str, size_t size, const char *format, ...)
//{
//    va_list ap;
//    int i;
//
//    va_start(ap, format);
//    i = vsnprintf(str, size, format, ap);
//    va_end(ap);
//    return i;
//}
//
static inline char toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
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
