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

#include "bare/GPIO.h"
#include "bare/InterruptManager.h"
#include "bare/Timer.h"

using namespace bare;

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

// UART1 or the "mini UART" is one of the AUX units
// and shares an interrupt with a couple of SPI devices.
// We're not using either of those, so the AUX Interrupt
// is used for UART1. It is in the main interrupt set
// at bit 29. Since this set comes after the "Basic IRQ"
// set, it is offset by 32.
// InterruptManager considers Basic interrupts starting
// at 0 and the other 64 interrupt bits starting at 32.
static constexpr uint32_t ARMTimerInterruptBit = 29 + 32;

static constexpr uint32_t UART1Base = 0x20215000;
static constexpr uint32_t RXBUFMASK = 0xFF;

volatile unsigned int _rxhead = 0;
volatile unsigned int _rxtail = 0;
volatile unsigned char _rxbuffer[RXBUFMASK + 1];

inline volatile UART1& uart()
{
	return *(reinterpret_cast<volatile UART1*>(UART1Base));
}

void Serial::init(uint32_t baudrate)
{
    baudrate = std::min(std::max(static_cast<int>(baudrate), 110), 31250000);
    
#ifndef NOINTERRUPTS
    if (interruptsSupported()) {
        disableIRQ();
	    InterruptManager::instance().enableIRQ(ARMTimerInterruptBit, false);

        _rxhead = _rxtail = 0;
        
        InterruptManager::instance().addHandler(ARMTimerInterruptBit, handleInterrupt);
    }
#endif

    uart().AUXENB = 1;
    uart().IER = 0;
    uart().CNTL = 0;
    uart().LCR = 3;
    uart().MCR = 0;
    uart().IER = interruptsSupported() ? 0x05 : 0;
    uart().IIR = 0xc6;
    uart().BAUD = 250000000 / baudrate / 8 - 1;

	GPIO::setFunction(14, GPIO::Function::Alt5);
	GPIO::setFunction(15, GPIO::Function::Alt5);

    GPIO::reg(GPIO::Register::GPPUD) = 0;
    Timer::usleep(150);
    GPIO::reg(GPIO::Register::GPPUDCLK0) = (1 << 14) | (1 << 15);
    Timer::usleep(150);
    GPIO::reg(GPIO::Register::GPPUDCLK0) = 0;

    uart().CNTL = 3;
    
#ifndef NOINTERRUPTS
    if (interruptsSupported()) {
	    InterruptManager::instance().enableIRQ(ARMTimerInterruptBit, true);
	    enableIRQ();
    }
#endif
}

Serial::Error Serial::read(uint8_t& c)
{
    if (interruptsSupported()) {
        while (1) {
            if (_rxtail != _rxhead) {
                c = _rxbuffer[_rxtail];
                _rxtail = (_rxtail + 1) & RXBUFMASK;
                break;
            }
            WFE();
        }
    } else {
        while (!rxReady()) { }
        c = static_cast<uint8_t>(uart().IO);
    }
	return Error::OK;
}

bool Serial::rxReady()
{
    if (interruptsSupported()) {
        return _rxtail != _rxhead;
    }
    return (uart().LSR & 0x01) != 0;
}

Serial::Error Serial::write(uint8_t c)
{
    while ((uart().LSR & 0x20) == 0) { }
    uart().IO = static_cast<uint32_t>(c);
    return Error::OK;
}

void Serial::handleInterrupt()
{
    while (1)
    {
        uint32_t iir = uart().IIR;
        if ((iir & 1) == 1) {
            //no more interrupts
            break;
        }
        
        if ((iir & 6) == 4) {
            //receiver holds a valid byte
            uint32_t b = uart().IO;
            _rxbuffer[_rxhead] = b & 0xFF;
            _rxhead = (_rxhead + 1) & RXBUFMASK;
        }
    }
}

void Serial::clearInput() 
{
    _rxhead = _rxtail = 0;
}
