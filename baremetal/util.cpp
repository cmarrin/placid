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

#ifdef __APPLE__
#include <stdio.h>
#include <sys/time.h>
#undef vsnprintf
#undef snprintf

void uart_send ( unsigned int c)
{
    putchar(c);
}

void uart_init()
{
}

unsigned int uart_lcr()
{
    return 0;
}

unsigned int uart_recv()
{
    return 0;
}

void timer_init()
{
}

extern "C" {

void PUT8(unsigned int addr, unsigned int value)
{
    printf("PUT8:[%d] <= %d\n", addr, value);
}

void PUT32(unsigned int addr, unsigned int value)
{
    printf("PUT32:[%d] <= %d\n", addr, value);
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

}

uint64_t timerTick()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (((uint64_t) time.tv_sec) * 1000000) + ((uint64_t) time.tv_usec);
}

#else


void disableIRQ()
{
    asm volatile ("mrs r0,cpsr" : : : "r0" );
    asm volatile ("orr r0,r0,#0x80" : : : "r0" );
    asm volatile ("msr cpsr_c,r0" : : : "r0" );
    asm volatile ("bx lr" );
}

void enableIRQ()
{
    asm volatile ("mrs r0,cpsr" : : : "r0" );
    asm volatile ("bic r0,r0,#0x80" : : : "r0" );
    asm volatile ("msr cpsr_c,r0" : : : "r0" );
    asm volatile ("bx lr" );
}

void WFE()
{
    asm volatile ("wfe" );
    asm volatile ("bx lr" );
}

#define STIMER_CLO 0x20003004
#define STIMER_CHI 0x20003008

uint64_t timerTick ()
{
    return (((uint64_t) GET32(STIMER_CHI)) << 32) | (uint64_t) GET32(STIMER_CLO);
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

}
#endif

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

bool placid::toString(char* buf, double v)
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

bool placid::toString(char* buf, int32_t v)
{
    if (v < 0) {
        *buf++ = '-';
        v = -v;
    }
 
    return toString(buf, static_cast<uint32_t>(v));
}

bool placid::toString(char* buf, uint32_t v)
{
    buf = intToString(static_cast<uint32_t>(v), buf, 19, 6);
    *buf = '\0';
	return true;
}

bool placid::toString(char* buf, int64_t v)
{
    if (v < 0) {
        *buf++ = '-';
        v = -v;
    }
    
    return toString(buf, static_cast<uint64_t>(v));
}

bool placid::toString(char* buf, uint64_t v)
{
    buf = intToString(v, buf, 19, 6);
    *buf = '\0';
    return true;
}

void delay(uint32_t t)
{
    uint64_t t0 = timerTick();
    
    while(timerTick() < t0 + t) ;
}

static char* intToString(uint32_t mantissa, char* str)
{
    if (!mantissa) {
        *str++ = '0';
        return str;
    }
    
    int leadingDigit = 1;
    int16_t dp = 9;

    while (mantissa || dp > 0) {
        int32_t digit = mantissa / 100000000;
        mantissa -= digit * 100000000;
        mantissa *= 10;
        
        // If this is the leading digit and '0', skip it
        if (!leadingDigit || digit != 0) {
            *str++ = ((char) digit) + '0';
            leadingDigit = 0;
        }
        dp--;
    }
    return str;
}

void utos(char* buf, uint32_t v)
{
    buf = intToString(v, buf);
    *buf = '\0';
}

void itos(char* buf, int32_t v)
{
    if (v < 0) {
        *buf++ = '-';
        v = -v;
    }
    
    utos(buf, v);
}

void putstr(const char* s)
{
    while (*s) {
        uart_send(*s++);
    }
}

void puti(int32_t v)
{
    char buf[MaxToStringBufferSize];
    itos(buf, v);
    putstr(buf);
}

void putu(uint32_t v)
{
    char buf[MaxToStringBufferSize];
    utos(buf, v);
    putstr(buf);
}

int getchar(void)
{
    return uart_recv();
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

#define StringGuard 1024

static int strcpy(char *dst, char const *src)
{
    int i;
    i = 0;
    while(*src!=0) {
        *(dst++) = *(src++);
        i++;
    } 
    *dst = 0;
    return i;
}

int strHex(char *str, uint32_t ad, int len, int fill)
{
    int i, j=0;
    int c;
    char s[StringGuard];
    int st;

    st = 0;
    for(i=0; i<8; i++) {
        if ((c = ((ad>>28) & 0x0F))>0) {
            c = (c<10) ? (c+'0') : (c+'a'-10);
            s[j++] = (char) c;
            st = 1;
        } else if (st || i==7) {
            s[j++] = '0';
        }
        ad = ad << 4;
    }
    s[j]=0;
    for(i=0; i<len-j; i++) {
        str[i] = (fill) ? '0' : ' ';
    }
    strcpy(str+i, s);
    
    return j+i;
}

int strNum(char *str, uint32_t ui, int len, int fill, int sflag)
{
    unsigned int cmp = 1;
    int i, j;
    int d;
    int l;
    char c;
    char s[StringGuard];

    cmp=1;
    l=1;
    //    printf("ui:%d, cmp:%d, l:%d\n", ui, cmp, l);
    while(cmp*10<=ui && l<10) {
        //2^32 = 4.29 * 10^9 -> Max digits =10 for 32 bit
        cmp *=10;
        l++;
        //        printf("ui:%d, cmp:%d, l:%d\n", ui, cmp, l);
    }

    j = 0;
    if (sflag) {
        s[j++]='-';
        l++; // add one space for '0'
    }
    //    printf("ui:%d, cmp:%d, j:%d, d:%d, c:%d\n", ui, cmp, j, d, c);
    while(j<l) {
        //        d = u32_div(ui, cmp);
        d = ui/cmp;
        c = (char) (d+'0');
        s[j++] = c;
        ui = ui - d*cmp;
        //cmp = u32_div(cmp, 10);
        cmp = cmp/10;
        //        Printf("ui:%d, cmp:%d, j:%d, d:%d, c:%d\n", ui, cmp, j, d, c);
    }
    s[j]=0;
    for(i=0; i<len-l; i++) {
        str[i] = (fill) ? '0' : ' ';
    }
    strcpy(str+i, s);
    
    return j+i;
}

int printf(const char *format, ...)
{
    va_list ap;
    char s[StringGuard];
    int i;

    va_start(ap, format);
    i = vsnprintf(s, StringGuard, format, ap);
    putstr(s);
    va_end(ap);
    return i;
}

int vsnprintf(char *str, size_t size, const char *format, va_list listPointer)
{
    char c;
    int i, si;
    unsigned int ui;
    int pf;
    char *s;
    int len, fill, sign_flag;
    
    i=0;
    pf = 0;
    len = 0;
    fill = 0;
    while((c=*format++)!=0 && i < static_cast<int>(size)) {
        if (pf==0) {
            // after regular character
            if (c=='%') {
                pf=1;
                len = 0;
                fill = 0;
            } else {
                str[i++]=c;
            }
        } else if (pf>0) {
            // previous character was '%'
            if (c=='x' || c=='X') {
                ui = va_arg(listPointer, unsigned);
                i += strHex(str+i, (unsigned) ui, len, fill);
                pf=0;
            } else if (c=='u' || c=='U' || c=='i' || c=='I') {
                ui = va_arg(listPointer, unsigned);
                i += strNum(str+i, (unsigned) ui, len, fill, 0);
                pf=0;
            } else if (c=='d' || c=='D') {
                si = va_arg(listPointer, int);
                if (si<0) {
                    ui = -si;
                    sign_flag = 1;
                } else {
                    ui = si;
                    sign_flag = 0;
                }
                i += strNum(str+i, (unsigned) ui, len, fill, sign_flag);
                pf=0;
            } else if (c=='s' || c=='S') {
                s = va_arg(listPointer, char *);
                i += strcpy(str+i, s);
                pf=0;
            } else if ('0'<=c && c<='9') {
                if (pf==1 && c=='0') {
                    fill = 1;
                } else {
                    len = len*10+(c-'0');
                }
                pf=2;
            } else {
                // this shouldn't happen
                str[i++]=c;
                pf=0;
            }
        }
    }
    str[i]=0;
    return (i>0) ? i : -1;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list ap;
    int i;

    va_start(ap, format);
    i = vsnprintf(str, size, format, ap);
    va_end(ap);
    return i;
}

int puts(const char* s)
{
    printf("%s", s);
    return 0;
}

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
