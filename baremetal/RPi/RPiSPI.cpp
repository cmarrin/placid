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

#include "bare/SPI.h"

#include "bare/GPIO.h"
#include "bare/Timer.h"

#define ENABLE_DEBUG_LOG
#include "bare/Log.h"

using namespace bare;

struct SPI0 {
    static constexpr uint32_t CSPOL2       = 1 << 23;
    static constexpr uint32_t CSPOL1       = 1 << 22;
    static constexpr uint32_t CSPOL0       = 1 << 21;
    static constexpr uint32_t RXF          = 1 << 20;
    static constexpr uint32_t RXR          = 1 << 19;
    static constexpr uint32_t TXD          = 1 << 18;
    static constexpr uint32_t RXD          = 1 << 17;
    static constexpr uint32_t DONE         = 1 << 16;
    static constexpr uint32_t LEN          = 1 << 13;
    static constexpr uint32_t REN          = 1 << 12;
    static constexpr uint32_t ADCS         = 1 << 11;
    static constexpr uint32_t INTR         = 1 << 10;
    static constexpr uint32_t INTD         = 1 << 9;
    static constexpr uint32_t DMAEN        = 1 << 8;
    static constexpr uint32_t TA           = 1 << 7;
    static constexpr uint32_t CSPOL        = 1 << 6;
    static constexpr uint32_t CLEAR_RX     = 1 << 5;
    static constexpr uint32_t CLEAR_TX     = 1 << 4;
    static constexpr uint32_t CPOL         = 1 << 3;
    static constexpr uint32_t CPHA         = 1 << 2;
    static constexpr uint32_t WhichCS      = 1 << 0;
    static constexpr uint32_t WhichCSShift = 0;
    
    uint32_t CS;
    uint32_t FIFO;
    uint32_t CLK;
    uint32_t DLEN;
    uint32_t LTOH;
    uint32_t DC;
};


static constexpr uint32_t SPI0Base = 0x20204000;

inline volatile SPI0& spi()
{
    return *(reinterpret_cast<volatile SPI0*>(SPI0Base));
}

void SPI::init(EnablePolarity enablePol, ClockEdge clockEdge, ClockPolarity clockPol)
{
    DEBUG_LOG("SPI:Initializing\n");

    // Make sure CS is set as an input. Only switch it to its Alt0 mode when sending 
    GPIO::setFunction(8, GPIO::Function::Input);
    GPIO::setFunction(9, GPIO::Function::Alt0);
    GPIO::setFunction(10, GPIO::Function::Alt0);
    GPIO::setFunction(11, GPIO::Function::Alt0);
    
    spi().CS = SPI0::CLEAR_RX | SPI0::CLEAR_TX;
    spi().CLK = 0; // 250MHz / 65536 = 3814.7Hz (slowest possible transfer rate)
    
    bool csPolarity = enablePol == EnablePolarity::ActiveHigh;
    bool clockPolarity = clockPol == ClockPolarity::ActiveLow;
    bool clockPhase = clockEdge == ClockEdge::Falling;
    Timer::usleep(1000);
    uint32_t cs = clockPolarity ? SPI0::CPOL : 0;
    cs |= clockPhase ? SPI0::CPHA : 0;
    cs |= csPolarity ? SPI0::CSPOL : 0;
    cs |= csPolarity ? SPI0::CSPOL0 : 0;
    spi().CS = cs;
    
    // FIXME: Wait a bit for things to settle. Not sure if we need this
    Timer::usleep(1000); 
    DEBUG_LOG("SPI:Initialization complete\n");
}

void SPI::startTransfer()
{
    DEBUG_LOG("SPI:startTransfer\n");
    GPIO::setFunction(8, GPIO::Function::Alt0);
    spi().CS = spi().CS | SPI0::TA | SPI0::CLEAR_RX | SPI0::CLEAR_TX;
}

static inline bool wait(uint32_t csBit)
{
    for (uint32_t i = 0; i < 10000; ++i) {
        if ((spi().CS & csBit) != 0) {
            return true;
        }
    }
    return false;
}

uint32_t SPI::transferByte(uint8_t b)
{
    DEBUG_LOG("SPI:transferByte sent %#02x\n", b);
    if (!wait(SPI0::TXD)) {
        return ErrorByte;
    }
    spi().FIFO = b;
    if (!wait(SPI0::RXD)) {
        return ErrorByte;
    }
    b = spi().FIFO;
    DEBUG_LOG("SPI:transferByte received %#02x\n", b);
    return b;
}

void SPI::endTransfer()
{
    while((spi().CS & SPI0::DONE) == 0) ;
    spi().CS = spi().CS & ~SPI0::TA & ~SPI0::CLEAR_RX & ~SPI0::CLEAR_TX;
    GPIO::setFunction(8, GPIO::Function::Input);
    DEBUG_LOG("SPI:endTransfer\n");
}
