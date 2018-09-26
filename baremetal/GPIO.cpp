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

#include "GPIO.h"

#include "util.h"

using namespace placid;

static constexpr uint32_t GPIOBase = 0x20200000;
static constexpr uint32_t GPFSELOffset = 0;
static constexpr uint32_t GPSETOffset = 0x1c;
static constexpr uint32_t GPCLROffset = 0x28;
static constexpr uint32_t GPLEVOffset = 0x34;

inline volatile uint32_t& rawReg(uint32_t r)
{
#ifdef __APPLE__
    static uint32_t _dummy = GPIOBase;
    return _dummy;
#else
    return *(reinterpret_cast<volatile uint32_t*>(GPIOBase + r));
#endif
}

void GPIO::setFunction(uint32_t pin, Function f)
{
	uint32_t r = pin / 10 * 4 + GPFSELOffset;
	uint32_t off = (pin % 10) * 3;
	
	uint32_t ra = rawReg(r);
    ra &= ~(7 << off);
    ra |= static_cast<uint32_t>(f) << off;
    rawReg(r) = ra;
    delay(1);
}

void GPIO::setPin(uint32_t pin, bool on)
{
	uint32_t r = pin / 32 * 4;
	uint32_t off = pin % 32;
	rawReg(r + (on ? GPSETOffset : GPCLROffset)) = 1 << off;
}

bool GPIO::getPin(uint32_t pin)
{
	uint32_t r = pin / 32 * 4;
	uint32_t off = pin % 32;
	return (rawReg(r + GPLEVOffset) & (1 << off)) != 0;
}

void GPIO::setPull(uint32_t pin, Pull val)
{
    Register reg;
    uint32_t enbit;
    
    if (pin < 32) {
        enbit = 1 << pin;
        reg = Register::GPPUDCLK0;
    } else {
        enbit = 1 << (pin - 32);
        reg = Register::GPPUDCLK1;
    }

    GPIO::reg(Register::GPPUD) = static_cast<uint32_t>(val);
    delay(150);
    GPIO::reg(reg) = enbit;
    delay(150);
    GPIO::reg(Register::GPPUD) = 0;
    GPIO::reg(reg) = 0;
}

volatile uint32_t& GPIO::reg(Register r)
{
	return rawReg(static_cast<uint32_t>(r));
}
