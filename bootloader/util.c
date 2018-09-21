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

#ifdef __APPLE__
#include <stdio.h>
#include <sys/time.h>

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

unsigned int timer_tick()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    return (((unsigned int) time.tv_sec) * 1000000) + ((unsigned int) time.tv_usec);
}

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

void dummy(unsigned int unused)
{
    volatile uint32_t a = 0;
    (void) a;
    (void) unused;
}

void dmb(void)
{
}

void BRANCHTO(unsigned int addr)
{
    printf("BRANCHTO: => %d\n", addr);
}

#else
extern void uart_send ( unsigned int );
extern unsigned int uart_recv ( void );
#endif


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

int putchar(int c)
{
    uart_send(c);
    return c;
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

