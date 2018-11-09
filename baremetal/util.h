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

#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdlib>
#include <functional>

static constexpr uint32_t ARMBASE = 0x8000;

void disableIRQ(void);
void enableIRQ(void);
void WFE(void);

#ifdef __cplusplus
extern "C" {
#endif
void PUT8 ( unsigned int, unsigned int );
void BRANCHTO ( unsigned int );
bool interruptsSupported(void);
void _start();

typedef struct { uint64_t rem; uint64_t quot; } ulldiv_t;

uint64_t __aeabi_uidivmod(unsigned int value, unsigned int divisor);
unsigned int __aeabi_uidiv(unsigned int value, unsigned int divisor);
int __aeabi_idiv(int value, int divisor);
ulldiv_t __aeabi_uldivmod(uint64_t value, uint64_t divisor);
lldiv_t __aeabi_ldivmod(int64_t numerator, int64_t denominator);

void* memset(void* p, int value, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
void* memmove(void * dst, const void* src, size_t n);
int memcmp(const void* left, const void* right, size_t n);
void convertTo8dot3(char* name8dot3, const char* name);

size_t strlen(const char*);
char* strcpy(char* dst, const char* src);
#ifdef __cplusplus
}
#endif

#ifdef __APPLE__
static inline void dmb() { }
static inline void dsb() { }
static inline void flushcache() { }
#else
static inline void dmb() { __asm volatile ("mcr p15, #0, %[zero], c7, c10, #5" : : [zero] "r" (0) ); }
static inline void dsb() { __asm volatile ("mcr p15, #0, %[zero], c7, c10, #4" : : [zero] "r" (0) ); }
static inline void flushcache() { __asm volatile ("mcr p15, #0, %[zero], c7, c14, #0" : : [zero] "r" (0) ); }
#endif

static inline int isspace(char c) { return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v'); }
static inline char toupper(char c) { return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c; }
static inline bool isdigit(uint8_t c)        { return c >= '0' && c <= '9'; }
static inline bool isLCHex(uint8_t c)       { return c >= 'a' && c <= 'f'; }
static inline bool isUCHex(uint8_t c)       { return c >= 'A' && c <= 'F'; }
static inline bool isHex(uint8_t c)         { return isUCHex(c) || isLCHex(c); }
static inline bool isxdigit(uint8_t c)        { return isHex(c) || isdigit(c); }
static inline bool isOctal(uint8_t c)       { return c >= '0' && c <= '7'; }
static inline bool isUpper(uint8_t c)        { return (c >= 'A' && c <= 'Z'); }
static inline bool isLower(uint8_t c)        { return (c >= 'a' && c <= 'z'); }
static inline bool isLetter(uint8_t c)        { return isUpper(c) || isLower(c); }
static inline bool isIdFirst(uint8_t c)        { return isLetter(c) || c == '$' || c == '_'; }
static inline bool isIdOther(uint8_t c)        { return isdigit(c) || isIdFirst(c); }
static inline bool isspace(uint8_t c)       { return c == ' ' || c == '\n' || c == '\r' || c == '\f' || c == '\t' || c == '\v'; }
static inline uint8_t tolower(uint8_t c)    { return isUpper(c) ? (c - 'A' + 'a') : c; }
static inline uint8_t toupper(uint8_t c)    { return isLower(c) ? (c - 'a' + 'A') : c; }
