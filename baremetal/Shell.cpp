/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Shell.h"

#include "bare/Serial.h"

using namespace bare;

void Shell::connected()
{
    _state = State::Connect;
    sendComplete();
}

void Shell::disconnected()
{
    _state = State::Disconnect;
    sendComplete();
}

void Shell::sendComplete()
{
    switch(_state) {
		case State::Connect:
		_state = State::NeedPrompt;
        showMessage(MessageType::Info, welcomeString());
		sendComplete();
        break;
	    
		case State::Disconnect:
		break;
        
		case State::NeedPrompt:
        _state = State::ShowingPrompt;
        showMessage(MessageType::Info, "\n[%s]> ", promptString());
        break;
        
		case State::ShowingPrompt:
        break;
	    
		case State::ShowHelp:
    	_state = State::NeedPrompt;
		showMessage(MessageType::Info, (bare::String("Commands:\n    '?' : this help message\n\n") + helpString()).c_str());
		sendComplete();
        break;
    }
}

bool Shell::received(uint8_t c)
{
    shellSend(reinterpret_cast<const char*>(&c), 1, true);

	if (c == '\r') {
		return true;
	}
	if (c != '\n') {
        if (c == '\b') {
            if (_bufferIndex > 0) {
                --_bufferIndex;
                
                // Erase the character we just backspaced over (by echoing)
                shellSend(" \b", 2, true);
            }
            return true;
        }
        if (_bufferIndex >= BufferSize) {
			return true;
		}
		_buffer[_bufferIndex++] = c;
		return true;
	}
	
    std::vector<bare::String> array = bare::String(_buffer, _bufferIndex).trim().split(" ", true);
   
    bool returnValue = executeCommand(array);
	_bufferIndex = 0;
    sendComplete();
	return returnValue;
}

bool Shell::executeCommand(const std::vector<bare::String>& array)
{
	_state = State::NeedPrompt;
	
    if (array.size() == 0) {
        return true;
    }
    if (array[0] == "?") {
        _state = State::ShowHelp;
    } else if (!executeShellCommand(array)) {
        showMessage(MessageType::Error, "unrecognized command: %s", join(array, " ").c_str());
    }
    return true;
}

void Shell::showMessage(MessageType type, const char* msg, ...)
{
    if (type == MessageType::Error) {
        _state = State::NeedPrompt;
        shellSend("Error: ");
    }
    
    va_list args;
    va_start(args, msg);
    
    shellSend(String::vformat(msg, args).c_str());

    if (type == MessageType::Error) {
        shellSend("\n");
    }
}
