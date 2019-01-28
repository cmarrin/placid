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

#include "bare.h"

#include "bare/Serial.h"
#include "bare/Timer.h"

using namespace bare;

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
