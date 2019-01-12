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

static constexpr uint32_t UART1Base = 0x20215000;

volatile unsigned int Serial::rxhead = 0;
volatile unsigned int Serial::rxtail = 0;
volatile unsigned char Serial::rxbuffer[RXBUFMASK + 1];

inline volatile UART1& uart()
{
	return *(reinterpret_cast<volatile UART1*>(UART1Base));
}

void Serial::init(uint32_t baudrate)
{
    baudrate = std::min(std::max(static_cast<int>(baudrate), 110), 31250000);
    
    if (interruptsSupported()) {
        disableIRQ();
	    InterruptManager::enableIRQ(29, false);

        rxhead = rxtail = 0;
    }

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
    
    if (interruptsSupported()) {
	    InterruptManager::enableIRQ(29, true);
	    enableIRQ();
    }
}

Serial::Error Serial::read(uint8_t& c)
{
    if (interruptsSupported()) {
        while (1) {
            if (rxtail != rxhead) {
                c = rxbuffer[rxtail];
                rxtail = (rxtail + 1) & RXBUFMASK;
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
        return rxtail != rxhead;
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
    if (!interruptsSupported()) {
        return;
    }
    
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
            rxbuffer[rxhead] = b & 0xFF;
            rxhead = (rxhead + 1) & RXBUFMASK;
        }
    }
}
