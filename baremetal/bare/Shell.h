/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <cstdint>
#include <vector>
#include "bare/String.h"

namespace bare {
	
	// Shell - Base class for a console shell
	//

	class Shell {
	public:
	    enum class State { Connect, Disconnect, NeedPrompt, ShowingPrompt, ShowHelp };
		
	    void connected();
	    void disconnected();
	    bool received(uint8_t c);
		
		virtual const char* welcomeString() const = 0;
		virtual const char* helpString() const = 0;
        virtual const char* promptString() const = 0;
	    virtual void shellSend(const char* data, uint32_t size = 0, bool raw = false) = 0;
		virtual bool executeShellCommand(const std::vector<bare::String>&) = 0;

	protected:
        enum class MessageType { Info, Error };
        void showMessage(MessageType type, const char* msg, ...);

	    void sendComplete();

	private:
	    bool executeCommand(const std::vector<bare::String>&);

	    State _state = State::Connect;
		
		static constexpr uint32_t BufferSize = 200;
		char _buffer[BufferSize + 1];
		uint32_t _bufferIndex = 0;
	};

}
