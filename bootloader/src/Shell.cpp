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

#include "Shell.h"

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
        shellSend("\n> ");
        break;
        
		case State::ShowingPrompt:
        break;
	    
		case State::ShowHelp:
    	_state = State::NeedPrompt;
		showMessage(MessageType::Info, helpString());
		sendComplete();
        break;
    }
}

static inline void trim(char*& s, uint32_t& len)
{
	while (::isspace(*s) && len) {
		++s;
		--len;
	}
	while (len && ::isspace(s[len - 1])) {
		len--;
	}
}

// static inline uint32_t split(const char* s, uint32_t length, const char**& tokens, uint32_t*& tokenLengths, uint32_t arrayLength, char separator)
// {
// 	trim(s, length);
//
// 	uint32_t index = 0;
//
// 	while (1) {
// 		const char* n = s;
// 		for (uint32_t i = 0; i < length; ++i, ++n) {
// 			if (*n == separator) {
// 				break;
// 			}
// 		}
// 		if (!n || n - s != 0) {
// 			tokens[index] = s;
// 			tokenLengths[index] = n - s;
// 		}
// 		if (!n) {
// 			break;
// 		}
// 		length -= n - s + 1;
// 		s = n + 1;
// 		if (++index >= arrayLength) {
// 			break;
// 		}
// 	}
//
// 	return index;
// }
//
bool Shell::received(uint8_t c)
{
	if (c == '\r') {
		return true;
	}
	if (c != '\n') {
		if (_bufferIndex >= BufferSize) {
			return true;
		}
		_buffer[_bufferIndex++] = c;
		return true;
	}
	
	char* p = _buffer;
	trim(p, _bufferIndex);
	p[_bufferIndex] = '\0';
    bool returnValue = executeCommand(p);
	_bufferIndex = 0;
    sendComplete();
	return returnValue;
}

bool Shell::executeCommand(const char* s)
{
	_state = State::NeedPrompt;
	
    if (*s == '\0') {
        return true;
    }
    if (s[0] == '?') {
        _state = State::ShowHelp;
    } else {
    	showMessage(MessageType::Error, "unrecognized command: ", s);
	}
    return true;
}

void Shell::showMessage(MessageType type, const char* msg, const char* altmsg)
{
    if (type == MessageType::Error) {
        _state = State::NeedPrompt;
		shellSend("Error: ");
    }
	
	shellSend(msg);
	if (altmsg) {
		shellSend("'");
		shellSend(altmsg);
		shellSend("'");
	}
    
    if (type == MessageType::Error) {
		shellSend("\n");
    }
}
