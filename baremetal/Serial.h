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

#include <stdarg.h>
#include <stdint.h>

namespace bare {
	
	// Serial - Raw serial driver for Raspberry Pi
	//
	// The UART which comes out on GPIO pins 8 (TX) and 10 (RX). This is the "mini UART"
	// hardware on the Raspberry Pi 3 and Zero and the "full UART" on all other Pi
	// hardware. Currently this driver only works with the "mini UART" so it is only
	// for the 3 and Zero. The code is derived from serial.c in this projet:
	//
	//		https://github.com/organix/pijFORTHos
	//
	// See that project for how to support the full UART. That code is in turn derived fron:
	//
	//		https://github.com/dwelch67/raspberrypi
	//
	// This is a static class and cannot be instantiated
	//

	class Serial {
	public:
		enum class Error { OK, Timeout, NoData, NotReady, Fail };
		
		static void init();
		
        static int32_t printf(const char* format, ...);
        static int32_t vprintf(const char* format, va_list);

		// Blocking API
		static Error read(uint8_t&);
        static bool rxReady();
		static Error write(uint8_t);
		static Error puts(const char*, uint32_t size = 0);
        static Error puts(double);
        static Error puts(int32_t);
        static Error puts(uint32_t);
        static Error puts(int64_t);
        static Error puts(uint64_t);
        
        static void clearInput() { rxhead = rxtail = 0; }

		static void handleInterrupt();

	private:
		Serial() { }
		Serial(Serial&) { }
		Serial& operator=(Serial& other) { return other; }
		
		static constexpr uint32_t RXBUFMASK = 0xFFF;
		static volatile unsigned int rxhead;
		static volatile unsigned int rxtail;
		static volatile unsigned char rxbuffer[RXBUFMASK + 1];
	};
	
}
