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

#include <stdint.h>

namespace placid {
	
	// GPIO - Raw GPIO driver for Raspberry Pi
	//
	// This code was inspired bu the work here:
	//
	//		https://github.com/dwelch67/raspberrypi
	//
	// This is a static class and cannot be instantiated
	//

	class GPIO {
	public:
		enum class Register {
			GPFSEL0 = 0x00,
			GPFSEL1 = 0x04,
			GPFSEL2 = 0x08,
			GPFSEL3 = 0x0c,
			GPFSEL4 = 0x10,
			GPFSEL5 = 0x14,
			GPSET0 = 0x1c,
			GPSET1 = 0x20,
			GPCLR0 = 0x28,
			GPCLR1 = 0x2c,
			GPLEV0 = 0x34,
			GPLEV1 = 0x38,
			GPEDS0 = 0x40,
			GPEDS1 = 0x44,
			GPREN0 = 0x4c,
			GPREN1 = 0x50,
			GPFEN0 = 0x58,
			GPFEN1 = 0x5c,
			GPHEN0 = 0x64,
			GPHEN1 = 0x68,
			GPLEN0 = 0x70,
			GPLEN1 = 0x74,
			GPAREN0 = 0x7c,
			GPAREN1 = 0x80,
			GPAFEN0 = 0x88,
			GPAFEN1 = 0x8c,
			GPPUD = 0x94,
            GPPUDCLK0 = 0x98,
            GPPUDCLK1 = 0x9c,
		};

		enum class Function {
			Input = 0,
			Output = 1,
			Alt0 = 4,
			Alt1 = 5,
			Alt2 = 6,
			Alt3 = 7,
			Alt4 = 3,
			Alt5 = 2,
		};
  
        enum class Pull { None = 0b00, Down = 0b01, Up = 0b10 };
		
		// Set the alternate function select register
		static void setFunction(uint32_t pin, Function);
		
		static void setPin(uint32_t pin, bool on);
		static bool getPin(uint32_t pin);
        static void setPull(uint32_t pin, Pull);

		static volatile uint32_t& reg(Register);
	};
	
}
