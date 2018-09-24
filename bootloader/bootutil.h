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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

                   //        sign    digits  dp      'e'     dp      exp     '\0'
#define MaxToStringBufferSize (1 +     20 +   1 +     1 +     1 +     3 +      1)

#ifdef __cplusplus
extern "C" {
#endif
uint64_t timerTick(void);
void delay(uint32_t t);
void PUT8 ( unsigned int, unsigned int );
void BRANCHTO ( unsigned int );
void uart_init ( void );
unsigned int uart_lcr ( void );
void uart_send ( unsigned int );
unsigned int uart_recv ( void );
void timer_init ( void );
void itos(char* buf, int32_t v);
void utos(char* buf, uint32_t v);
void putstr(const char* s);
void puti(int32_t v);
void putu(uint32_t v);
int getchar(void);
void* memset(void* p, int value, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
int memcmp(const void* left, const void* right, size_t n);
int printf(const char *format, ...);
int vsnprintf(char *str, size_t n, const char *format, va_list);
int snprintf(char *str, size_t n, const char *format, ...);
int puts(const char*); // ARM compiler seems to convert printf("...") to puts("...")
void convertTo8dot3(char* name8dot3, const char* name);
#ifdef __cplusplus
}
#endif
