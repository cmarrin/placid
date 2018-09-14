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

#include "util.h"
#include "BootShell.h"
#include "GPIO.h"
#include "Serial.h"
#include "Timer.h"

using namespace placid;

static constexpr uint32_t ActivityLED = 47;
static constexpr float blinkRate = 3;

class LEDBlinker : public TimerCallback
{
public:
	virtual void handleTimerEvent()
	{
		GPIO::setPin(ActivityLED, !GPIO::getPin(ActivityLED));
	}
};

int main()
{
	Serial::init();
	
	// Delay for the serial port to connect after power up
	Timer::delay(1.5);

	cout << "hello";
		
	GPIO::setFunction(ActivityLED, GPIO::Function::Output);
	
	LEDBlinker blinker;
	Timer::start(&blinker, blinkRate, true);
	
	BootShell shell;
	shell.connected();

	while (1) {
        int8_t c;
        if (Serial::read(c) != Serial::Error::OK) {
            Serial::puts("*** Serial Read Error\n");
        } else {
            Serial::write(c);
            shell.received(c);
        }
	}
 
    return 0;
}
