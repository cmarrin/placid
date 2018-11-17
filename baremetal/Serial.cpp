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

#include "bare/Print.h"
#include "bare/Timer.h"

using namespace bare;

int32_t Serial::printf(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    
    int32_t result = vprintf(format, va);
    va_end(va);
    return result;
}

int32_t Serial::vprintf(const char* format, va_list va)
{
    return Print::printfCore([](char c) { Serial::write(c); }, format, va);
}

Serial::Error Serial::puts(const char* s, uint32_t size)
{
    if (size == 0) {
        for (const char* p = s; *p != '\0'; ++p, ++size) ;
    }
    
    while (*s != '\0' && size > 0) {
        char c;
        c = *s++;
        size--;
        
        if (c != '\n' && c != '\r') {
            if (static_cast<uint8_t>(c) < ' ' || static_cast<uint8_t>(c) > 0x7e) {
                write('\\');
                write(((c >> 6) & 0x03) + '0');
                write(((c >> 3) & 0x07) + '0');
                write((c & 0x07) + '0');
                continue;
            }
        }

        Error error = write(c);
		if (error != Error::OK) {
			return error;
		}
    }
    
    static bool firstTime = true;
    if (firstTime) {
        clearInput();
        firstTime = false;
    }

	return Error::OK;
}
