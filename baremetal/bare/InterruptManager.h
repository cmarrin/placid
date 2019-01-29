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
        void enableBasicIRQ(uint32_t n, bool enable);
        
        void addHandler(Handler);
        
        void handleInterrupt();
        
	private:
        static constexpr uint32_t MaxHandlers = 4;
        Handler _handlers[MaxHandlers];
        uint8_t _handlerIndex = 0;
	};
	
}
