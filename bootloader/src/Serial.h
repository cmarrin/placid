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

#include "defs.h"

namespace placid {
	
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
		enum class Error { OK, NoData, NotReady, Fail };
		
		static void init();
		
		// Blocking API
		static Error read(int8_t&);
		static Error write(int8_t);
		static Error puts(const char*, uint32_t size = 0);
        static Error puts(double);
        static Error puts(int32_t);
        static Error puts(uint32_t);

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
	
	class OutputStream
	{
	public:
		OutputStream& operator << (const char* s) { Serial::puts(s); return *this; }
		OutputStream& operator << (int32_t v) { Serial::puts(v); return *this; }
		OutputStream& operator << (uint32_t v) { Serial::puts(v); return *this; }
		OutputStream& operator << (int16_t v) { Serial::puts(static_cast<int32_t>(v)); return *this; }
		OutputStream& operator << (uint16_t v) { Serial::puts(static_cast<uint32_t>(v)); return *this; }
		OutputStream& operator << (int8_t v) { Serial::puts(static_cast<int32_t>(v)); return *this; }
		OutputStream& operator << (uint8_t v) { Serial::puts(static_cast<uint32_t>(v)); return *this; }
		OutputStream& operator << (float v) { Serial::puts(static_cast<double>(v)); return *this; }
		OutputStream& operator << (double v) { Serial::puts(v); return *this; }
	};

	extern OutputStream cout;
	
	
}
