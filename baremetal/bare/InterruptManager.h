/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Singleton.h"
#include <cstdint>
#include <cstdint>
#include <functional>

namespace bare {
	
	// InterruptManager - Raw interrupt manager for Raspberry Pi
	//
	// This is a static class. Its only purpose is to hold the raw IRQ handler
	//
	// This code was inspired bu the work here:
	//
	//		https://github.com/dwelch67/raspberrypi
	//
	class InterruptManager : public Singleton<InterruptManager> {
	public:
        using Handler = std::function<void()>;
        
        void enableIRQ(uint32_t n, bool enable);
        
        void addHandler(uint8_t id, Handler);
        
        void handleInterrupt();
        
	private:
        static constexpr uint32_t MaxHandlers = 96; // This happens to be the number of entries for RPi
        static constexpr uint32_t PendingArraySize = (MaxHandlers + 31) / 32;
        
        int32_t findFirstBit(uint32_t* pendingBits);
        void interruptsPending(uint32_t* bits);
        
        Handler _handlers[MaxHandlers];
	};
	
}
