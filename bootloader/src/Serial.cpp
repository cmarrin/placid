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

#include "Serial.h"

#include "GPIO.h"

using namespace placid;

struct UART1 {
    uint32_t _00;
    uint32_t AUXENB; //_04;
    uint32_t _08;
    uint32_t _0c;
    uint32_t _10;
    uint32_t _14;
    uint32_t _18;
    uint32_t _1c;
    uint32_t _20;
    uint32_t _24;
    uint32_t _28;
    uint32_t _2c;
    uint32_t _30;
    uint32_t _34;
    uint32_t _38;
    uint32_t _3c;
    uint32_t IO;     //_40;
    uint32_t IER;    //_44;
    uint32_t IIR;    //_48;
    uint32_t LCR;    //_4c;
    uint32_t MCR;    //_50;
    uint32_t LSR;    //_54;
    uint32_t MSR;    //_58;
    uint32_t _5c;
    uint32_t CNTL;   //_60;
    uint32_t STAT;   //_64;
    uint32_t BAUD;   //_68;
};

static constexpr uint32_t gpioBase = 0x20200000;

enum class Register {
	GPFSEL0 = 0x00,
	GPFSEL1 = 0x04,
	GPFSEL2 = 0x08,
	GPFSEL3 = 0x0c,
	GPFSEL4 = 0x10,
	GPSET0 = 0x1c, 
	GPSET1 = 0x20, 
	GPCLR0 = 0x28,
	GPCLR1 = 0x2c,
	GPPUD = 0x94,
	GPPUDCLK0 = 0x98,
	UART1 = 0x00015000,
};

inline volatile UART1& uart()
{
	return *(reinterpret_cast<volatile UART1*>(gpioBase + static_cast<uint32_t>(Register::UART1)));
}

inline volatile uint32_t& reg(Register r)
{
	return *(reinterpret_cast<volatile uint32_t*>(gpioBase + static_cast<uint32_t>(r)));
}

// extern "C" void handleIRQ()
// {
//
// }
//
Serial::Serial()
{
    uint32_t r0;

    uart().AUXENB = 1;
    uart().IER = 0;
    uart().CNTL = 0;
    uart().LCR = 3;
    uart().MCR = 0;
    uart().IER = 0;
    uart().IIR = 0xc6;
    /* ((250,000,000 / 115200) / 8) - 1 = 270 */
    uart().BAUD = 270;

	GPIO::setFunction(14, GPIO::Function::Alt5);
	GPIO::setFunction(15, GPIO::Function::Alt5);

    reg(Register::GPPUD) = 0;
    SPIN(150);                  // wait for (at least) 150 clock cycles
    r0 = (1 << 14) | (1 << 15);
    reg(Register::GPPUDCLK0) = r0;
    SPIN(150);                  // wait for (at least) 150 clock cycles
    reg(Register::GPPUDCLK0) = 0;

    uart().CNTL = 3;
}

Serial::Error Serial::read(uint8_t& c)
{
    while (!rxReady()) {
    }
    return rx(c);
}

Serial::Error Serial::write(uint8_t c)
{
    while (!txReady()) {
    }
    return tx(c);
}

Serial::Error Serial::puts(const char* s)
{
    while (*s != '\0') {
        Error error = write(*s++);
		if (error != Error::OK) {
			return error;
		}
    }
	
	return Error::OK;
}

bool Serial::rxReady()
{
    return (uart().LSR & 0x01) != 0;
}

Serial::Error Serial::rx(uint8_t& c)
{
    c = uart().IO & 0xff;
	return Error::OK;
}

bool Serial::txReady()
{
    return (uart().LSR & 0x20) != 0;
}

Serial::Error Serial::tx(uint8_t c)
{
    uart().IO = static_cast<uint32_t>(c);
    return Error::OK;
}
