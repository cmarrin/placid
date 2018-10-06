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

#include "BootShell.h"

#include "Print.h"
#include "Serial.h"
#include "Timer.h"

using namespace placid;

const char* BootShell::welcomeString() const
{
	return "\n\nPlacid Kernel Shell v0.1";
}

const char* BootShell::helpString() const
{
	return  "    ls                 : list files\n"
            "    get <file>         : get file (XModem receive)\n"
            "    put <file>         : put file (XModem send)\n"
            "    rm <file>          : remove file\n"
            "    mv <src> <dst>     : rename file\n"
            "    date [<time/date>] : set/get time/date\n"
            "    heap               : show heap status\n"
            "    run <file>         : run user program\n"
            "    stop <pid>         : stop user program\n"
            "    debug [on/off]     : turn debugging on/off\n";
} 

void BootShell::shellSend(const char* data, uint32_t size, bool raw)
{
    // puts converts control characters to printable, so if we want
    // to send control we have to send raw
    if (raw) {
        while (*data) {
            Serial::write(*data++);
        }
    } else {
	    Serial::puts(data, size);
     }   
}

static String timeString()
{
    static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
    RealTime currentTime = Timer::currentTime();
    char buf[50];
    Print::snprintf(buf, 49, "%s %d/%d/%d %d:%02d:%02d", 
        days[currentTime.dayOfWeek()],
        currentTime.month(), currentTime.day(), currentTime.year(),
        currentTime.hours(), currentTime.minutes(), currentTime.seconds());
    return String(buf);
}

bool BootShell::executeShellCommand(const std::vector<String>& array)
{
    if (array[0] == "ls") {
    } else if (array[0] == "get") {
    } else if (array[0] == "put") {
    } else if (array[0] == "rm") {
    } else if (array[0] == "mv") {
    } else if (array[0] == "date") {
        if (array.size() == 1) {
            showMessage(MessageType::Info, "current time: %s\n", timeString().c_str());
        } else {
            // Set time. Format is as in the unix date command with the following command line:
            // 
            //      date "+%Y/%m/%d %T"
            //
            //      e.g., 2018/10/05 23:57:39
            std::vector<String> dateArray = array[1].split("/");
            std::vector<String> timeArray = array[2].split(":");
            RealTime t(
                    static_cast<uint32_t>(dateArray[0]),
                    static_cast<uint32_t>(dateArray[1]),
                    static_cast<uint32_t>(dateArray[2]), 
                    static_cast<uint32_t>(timeArray[0]), 
                    static_cast<uint32_t>(timeArray[1]), 
                    static_cast<uint32_t>(timeArray[2]));
            Timer::setCurrentTime(t);
            showMessage(MessageType::Info, "set current time to: %s\n", timeString().c_str());
        }
    } else if (array[0] == "heap") {
        showMessage(MessageType::Info, "heap size: %d\n", 0);
    } else if (array[0] == "run") {
        showMessage(MessageType::Info, "Program started...\n");
    } else if (array[0] == "stop") {
        showMessage(MessageType::Info, "Program stopped\n");
    } else if (array[0] == "debug") {
        showMessage(MessageType::Info, "Debug true\n");
    } else {
        return false;
    }
    return true;
}

