/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/GPIO.h"

#include "bare/Timer.h"

using namespace bare;

static constexpr uint32_t GPIOBase = 0x20200000;
static constexpr uint32_t GPFSELOffset = 0;
static constexpr uint32_t GPSETOffset = 0x1c;
static constexpr uint32_t GPCLROffset = 0x28;
static constexpr uint32_t GPLEVOffset = 0x34;

inline volatile uint32_t& rawReg(uint32_t r)
{
    return *(reinterpret_cast<volatile uint32_t*>(GPIOBase + r));
}

void GPIO::setFunction(uint32_t pin, Function f, Pull p)
{
    // Set function
	uint32_t r = pin / 10 * 4 + GPFSELOffset;
	uint32_t off = (pin % 10) * 3;
	
	uint32_t ra = rawReg(r);
    ra &= ~(7 << off);
    ra |= static_cast<uint32_t>(f) << off;
    rawReg(r) = ra;
    Timer::usleep(1);

    // Set pullup/pulldown
    Register reg;
    uint32_t enbit;
    
    if (pin < 32) {
        enbit = 1 << pin;
        reg = Register::GPPUDCLK0;
    } else {
        enbit = 1 << (pin - 32);
        reg = Register::GPPUDCLK1;
    }

    GPIO::reg(Register::GPPUD) = static_cast<uint32_t>(p);
    Timer::usleep(150);
    GPIO::reg(reg) = enbit;
    Timer::usleep(150);
    GPIO::reg(Register::GPPUD) = 0;
    GPIO::reg(reg) = 0;
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

volatile uint32_t& GPIO::reg(Register r)
{
	return rawReg(static_cast<uint32_t>(r));
}
