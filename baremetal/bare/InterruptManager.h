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
        using InterruptHandler = std::function<void()>;
        using SWIHandler = std::function<uint32_t(uint32_t, uint32_t, uint32_t)>;
        
        void enableIRQ(uint32_t n, bool enable);
        
        void addInterruptHandler(uint8_t id, InterruptHandler);
        void handleInterrupt();

        void addSWIHandler(uint8_t id, SWIHandler);
        uint32_t handleSWI(uint8_t id, uint32_t arg0, uint32_t arg1, uint32_t arg2);
        
	private:
        static constexpr uint32_t MaxInterruptHandlers = 32; // This number can be increased at the cost of more HandleEntries 
        static constexpr uint32_t MaxSWIHandlers = 32; // This number can be increased at the cost of more HandleEntries 
        
        bool interruptPending(uint8_t n);
        
        struct InterruptHandlerEntry { uint8_t id; InterruptHandler handler; };
        std::array<InterruptHandlerEntry, MaxInterruptHandlers> _interruptHandlers;
        uint32_t _interruptHandlerIndex = 0;

        struct SWIHandlerEntry { uint8_t id; SWIHandler handler; };
        std::array<SWIHandlerEntry, MaxSWIHandlers> _swiHandlers;
        uint32_t _swiHandlerIndex = 0;
	};
	
}
