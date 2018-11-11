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

#include "Shell.h"

#include "bare/Print.h"
#include "bare/Serial.h"

using namespace placid;

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
		showMessage(MessageType::Info, (String("Commands:\n    '?' : this help message\n\n") + helpString()).c_str());
		sendComplete();
        break;
    }
}

bool Shell::received(uint8_t c)
{
#ifndef __APPLE__
    shellSend(reinterpret_cast<const char*>(&c), 1, true);
#endif

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
	
    std::vector<String> array = String(_buffer, _bufferIndex).trim().split(" ", true);
   
    bool returnValue = executeCommand(array);
	_bufferIndex = 0;
    sendComplete();
	return returnValue;
}

bool Shell::executeCommand(const std::vector<String>& array)
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
    
    String string;
    string.vprintf(msg, args);    
    shellSend(string.c_str());

    if (type == MessageType::Error) {
        shellSend("\n");
    }
}
