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
#include "bare/Print.h"
#include "bare/Timer.h"

#ifdef __APPLE__
#include <iostream>
#endif

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

void Serial::init()
{
#ifdef __APPLE__
    //system("stty raw");
#else
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
    /* ((250,000,000 / 115200) / 8) - 1 = 270 */
    uart().BAUD = 270;

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
#endif
}

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
    return Print::printfCore([](char c) { Serial::write(c); }, format, va);
}

Serial::Error Serial::read(uint8_t& c)
{
#ifdef __APPLE__
    c = getchar();
#else
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
#endif
	return Error::OK;
}

bool Serial::rxReady()
{
#ifdef __APPLE__
    return true;
#else
    if (interruptsSupported()) {
        return rxtail != rxhead;
    }
    return (uart().LSR & 0x01) != 0;
#endif
}

Serial::Error Serial::write(uint8_t c)
{
#ifdef __APPLE__
    std::cout << c;
    return Error::OK;
#else
    while ((uart().LSR & 0x20) == 0) { }
    uart().IO = static_cast<uint32_t>(c);
    return Error::OK;
#endif
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

#ifdef __APPLE__
        std::cout.write(&c, 1);
#else
        Error error = write(c);
		if (error != Error::OK) {
			return error;
		}
#endif
    }
    
    static bool firstTime = true;
    if (firstTime) {
        clearInput();
        firstTime = false;
    }

	return Error::OK;
}

Serial::Error Serial::puts(double v)
{
    char buf[Print::MaxToStringBufferSize];
    if(Print::toString(buf, v)) {
        puts(buf);
    }
    return Error::OK;
}

Serial::Error Serial::puts(int32_t v)
{
    char buf[Print::MaxToStringBufferSize];
    if(Print::toString(buf, v)) {
        puts(buf);
    }
    return Error::OK;
}

Serial::Error Serial::puts(uint32_t v)
{
    char buf[Print::MaxToStringBufferSize];
    if(Print::toString(buf, v)) {
        puts(buf);
    }
    return Error::OK;
}

Serial::Error Serial::puts(int64_t v)
{
    char buf[Print::MaxToStringBufferSize];
    if(Print::toString(buf, v)) {
        puts(buf);
    }
    return Error::OK;
}

Serial::Error Serial::puts(uint64_t v)
{
    char buf[Print::MaxToStringBufferSize];
    if(Print::toString(buf, v)) {
        puts(buf);
    }
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
