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

#include "bare/InterruptManager.h"

#include "bare/Serial.h"
#include "bare/Timer.h"

using namespace bare;

struct IRPT {
    uint32_t BasicPending;
    uint32_t IRQ1Pending; //_04;
    uint32_t IRQ2Pending;
    uint32_t FIQControl;
    uint32_t IRQ1Enable;
    uint32_t IRQ2Enable;
    uint32_t BasicEnable;
    uint32_t IRQ1Disable;
    uint32_t IRQ2Disable;
    uint32_t BasicDisable;
};

static constexpr uint32_t IRPTBase = 0x2000B200;

inline volatile IRPT& irpt()
{
#ifdef __APPLE__
    static IRPT _dummy = { IRPTBase };
    return _dummy;
#else
    return *(reinterpret_cast<volatile IRPT*>(IRPTBase));
#endif
}

extern "C" void handleIRQ()
{
    if (interruptsSupported()) {
        Serial::handleInterrupt();
	    Timer::handleInterrupt();
    }
}

void InterruptManager::enableIRQ(uint32_t n, bool enable)
{
    if (!interruptsSupported()) {
        return;
    }
    
	uint32_t r = n / 32;
	uint32_t off = n % 32;
	
	if (enable) {
		if (r == 0) {
			irpt().IRQ1Enable = 1 << off;
		} else {
			irpt().IRQ2Enable = 1 << off;
		}
	} else {
		if (r == 0) {
			irpt().IRQ1Disable = 1 << off;
		} else {
			irpt().IRQ2Disable = 1 << off;
		}
	}
}

void InterruptManager::enableBasicIRQ(uint32_t n, bool enable)
{
    if (!interruptsSupported()) {
        return;
    }
    
    if (n >= 32) {
        return;
    }
    
    if (enable) {
        irpt().BasicEnable = 1 << n;
    } else {
        irpt().BasicDisable = 1 << n;
    }
}

