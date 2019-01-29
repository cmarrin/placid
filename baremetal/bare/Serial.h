/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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
		
		static void init(uint32_t baudrate);
		
        static int32_t printf(const char* format, ...);
        static int32_t vprintf(const char* format, va_list);

		// Blocking API
		static Error read(uint8_t&);
        static bool rxReady();
		static Error write(uint8_t);
		static Error puts(const char*, uint32_t size = 0);
        
        static void clearInput();

        static void handleInterrupt();

	private:
		Serial() { }
		Serial(Serial&) { }
		Serial& operator=(Serial& other) { return other; }
	};
	
}
