/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Serial.h"
#include "bare/Timer.h"

using namespace bare;

void* bare::aligned_alloc(size_t align, size_t size)
{
    if(align < alignof(void*)) { 
        align = alignof(void*); 
    }
    
    size_t space = size + align - 1;
    void* allocated_mem = ::operator new(space + sizeof(void*));
    void* aligned_mem = static_cast<void*>(static_cast<char*>(allocated_mem) + sizeof(void*)); 
    std::align(align, size, aligned_mem, space);
    *(static_cast<void**>(aligned_mem) - 1) = allocated_mem;
    return aligned_mem;
}

void bare::aligned_free(void* ptr)
{
    ::operator delete(*(static_cast<void**>(ptr) - 1));
}

#ifdef PLATFORM_RPI

extern "C" {

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

    void* __dso_handle __attribute__ ((__weak__));
    int __aeabi_atexit(void *, void(*)(void *), void *) {
        return 1;
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

    void __cxa_pure_virtual()
    {
        Serial::printf("pure virtual call\n");
        abort();
    }

}

namespace std {
    void __throw_bad_function_call()
    {
        Serial::printf("bad function call\n");
        abort();
    }

    void __throw_length_error(const char* s)
    {
        Serial::printf("length error:%s\n", s);
        abort();
    }

    void __throw_bad_alloc()
    {
        Serial::printf("bad alloc\n");
        abort();
    }
}

#endif
