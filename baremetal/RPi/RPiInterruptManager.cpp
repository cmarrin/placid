/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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
    return *(reinterpret_cast<volatile IRPT*>(IRPTBase));
}

extern "C" void handleIRQ()
{
    if (interruptsSupported()) {
        InterruptManager::instance().handleInterrupt();
    }
}

extern "C" void handleFIQ()
{
    Serial::printf("\n\n*** FIQ not supported\n\n");
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

