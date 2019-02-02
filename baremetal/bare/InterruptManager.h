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
#include <array>

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
        static constexpr uint32_t MaxHandlers = 32; // This number can be increased at the cost of more HandleEntries 
        
        bool interruptPending(uint8_t n);
        
        struct HandlerEntry { uint8_t id; Handler handler; };
        std::array<HandlerEntry, MaxHandlers> _handlers;
        uint32_t _handlerIndex = 0;
	};
	
}
