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

#include "bare/Formatter.h"
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
    return Formatter::vformat([](char c) { Serial::write(c); }, format, va);
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
