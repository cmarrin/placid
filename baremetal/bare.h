/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare/Float.h"
#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <cstdlib>
#include <functional>

// Platform defines. Currently supported:
//
//      PLATFORM_DARWIN     - macOS or iOS (defined here ifdef __APPLE__)
//      PLATFORM_RPI        - Raspberry Pi (defined in baremetal/Makefile)
//      PLATFORM_ESP        - ESP8266 (defined if ESP8266 is defined)
#if defined(__APPLE__)
#define PLATFORM_APPLE
#elif defined(ESP8266)
#define PLATFORM_ESP
#elif !defined(PLATFORM_RPI)
static_assert(0, "Unsupported platform");
#endif

namespace bare {

#if defined(FLOAT32)
    using Float = Float32;
#elif defined(FLOAT64)
    using Float = Float64;
#elif defined(FLOATFLOAT)
    using Float = FloatFloat;
#elif defined(FLOATDOUBLE)
    using Float = FloatDouble;
#elif defined(FLOATNONE)
    using Float = FloatNone;
#else
    #error must define one of FLOAT32, FLOAT64, FLOATFLOAT, FLOATDOUBLE or FLOATNONE
#endif

    void initSystem();
    bool useAllocator();
    
    void* aligned_alloc(size_t align, size_t size);
    void aligned_free(void*);

    extern "C" {
        void disableIRQ(void);
        void enableIRQ(void);
        bool interruptsSupported(void);
        void halt();
        void reboot();
        void PUT8(uint8_t*, uint8_t);
        void BRANCHTO(uint8_t*);
        void WFE();

        void* memset(void* p, int value, size_t n);
        void* memcpy(void* dst, const void* src, size_t n);
        void* memmove(void * dst, const void* src, size_t n);
        int memcmp(const void* left, const void* right, size_t n);

        size_t strlen(const char*);
        char* strcpy(char* dst, const char* src);

        uint8_t* kernelBase();
    }
    
    static inline bool isDigit(uint8_t c)        { return c >= '0' && c <= '9'; }
    static inline bool isLCHex(uint8_t c)       { return c >= 'a' && c <= 'f'; }
    static inline bool isUCHex(uint8_t c)       { return c >= 'A' && c <= 'F'; }
    static inline bool isHex(uint8_t c)         { return isUCHex(c) || isLCHex(c); }
    static inline bool isXDigit(uint8_t c)        { return isHex(c) || isdigit(c); }
    static inline bool isOctal(uint8_t c)       { return c >= '0' && c <= '7'; }
    static inline bool isUpper(uint8_t c)        { return (c >= 'A' && c <= 'Z'); }
    static inline bool isLower(uint8_t c)        { return (c >= 'a' && c <= 'z'); }
    static inline bool isLetter(uint8_t c)        { return isUpper(c) || isLower(c); }
    static inline bool isIdFirst(uint8_t c)        { return isLetter(c) || c == '$' || c == '_'; }
    static inline bool isIdOther(uint8_t c)        { return isDigit(c) || isIdFirst(c); }
    static inline bool isSpace(uint8_t c)       { return c == ' ' || c == '\n' || c == '\r' || c == '\f' || c == '\t' || c == '\v'; }
    static inline uint8_t toLower(uint8_t c)    { return isUpper(c) ? (c - 'A' + 'a') : c; }
    static inline uint8_t toUpper(uint8_t c)    { return isLower(c) ? (c - 'a' + 'A') : c; }
}
