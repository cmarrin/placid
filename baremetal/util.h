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

typedef struct { uint64_t rem; uint64_t quot; } ulldiv_t;

uint64_t __aeabi_uidivmod(unsigned int value, unsigned int divisor);
unsigned int __aeabi_uidiv(unsigned int value, unsigned int divisor);
ulldiv_t __aeabi_uldivmod(uint64_t value, uint64_t divisor);
lldiv_t __aeabi_ldivmod(int64_t numerator, int64_t denominator);

void* memset(void* p, int value, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
void* memmove(void * dst, const void* src, size_t n);
int memcmp(const void* left, const void* right, size_t n);
void convertTo8dot3(char* name8dot3, const char* name);

size_t strlen(const char*);
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

inline int isspace(char c) { return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v'); }
inline char toupper(char c) { return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c; }
