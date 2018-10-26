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

#include "FileSystem.h"
#include "Print.h"
#include "Serial.h"
#include "Timer.h"

using namespace placid;

const char* BootShell::welcomeString() const
{
	return "\n\nPlacid Kernel Shell v0.2";
}

const char* BootShell::helpString() const
{
	return
            "    date [<time/date>] : set/get time/date\n"
            "    debug [on/off]     : turn debugging on/off\n"
            "    get <file>         : get file (XModem receive)\n"
            "    heap               : show heap status\n"
            "    put <file>         : put file (XModem send)\n"
            "    ls                 : list files\n"
            "    mv <src> <dst>     : rename file\n"
            "    reset              : restart kernel\n"
            "    rm <file>          : remove file\n"
            "    run <file>         : run user program\n"
            "    stop <pid>         : stop user program\n"
    ;
}

const char* BootShell::promptString() const
{
    // FIXME: We want to display which process we're set to. For now, just kernel
    return "kernel";
}

void BootShell::shellSend(const char* data, uint32_t size, bool raw)
{
    // puts converts control characters to printable, so if we want
    // to send control we have to send raw
    if (raw) {
        while (size--) {
            bare::Serial::write(*data++);
        }
    } else {
	    bare::Serial::puts(data, size);
     }   
}

static String timeString()
{
    static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
    bare::RealTime currentTime = bare::Timer::currentTime();
    char buf[50];
    bare::Print::snprintf(buf, 49, "%s %d/%d/%d %d:%02d:%02d", 
        days[currentTime.dayOfWeek()],
        currentTime.month(), currentTime.day(), currentTime.year(),
        currentTime.hours(), currentTime.minutes(), currentTime.seconds());
    return String(buf);
}

bool BootShell::executeShellCommand(const std::vector<String>& array)
{
    if (array[0] == "ls") {
        bare::DirectoryIterator* it = FileSystem::sharedFileSystem()->directoryIterator("/");
        for ( ; *it; it->next()) {
            showMessage(MessageType::Info, "%-13s %10d\n", it->name(), it->size());
        }
    } else if (array[0] == "put") {
        if (array.size() != 2) {
            showMessage(MessageType::Error, "put requires one file name\n");
            return true;
        }
        
        File* fp = FileSystem::sharedFileSystem()->open(array[1].c_str(), FileSystem::OpenMode::Write);
        if (!fp->valid()) {
            if (fp->error() == bare::Volume::Error::FileExists) {
                showMessage(MessageType::Error, "'%s' exists. Please select a new file name\n", array[1].c_str());
            } else {
                showMessage(MessageType::Error, "open of '%s' failed: %s\n", array[1].c_str(), FileSystem::sharedFileSystem()->errorDetail(fp->error()));
            }
            return true;
        }
        
        showMessage(MessageType::Info, "Start XModem download when ready, or press [esc] key to cancel...\n");
        if (!xmodemReceive([fp](uint32_t addr, char byte) -> bool
        {
            return fp->write(&byte, 1) == 1;
        })) {
            showMessage(MessageType::Error, "XModem upload failed\n");
            delete fp;
            bare::Volume::Error error = FileSystem::sharedFileSystem()->remove(array[1].c_str());
            if (error != bare::Volume::Error::OK) {
                showMessage(MessageType::Error, "deletion of '%s' failed: %s\n",
                    array[1].c_str(), FileSystem::sharedFileSystem()->errorDetail(error));
            }
        } else {
            showMessage(MessageType::Info, "'%s' uploaded, size=%d\n", array[1].c_str(), fp->size());
            delete fp;
        }
    } else if (array[0] == "reset") {
        _start();
    } else if (array[0] == "rm") {
        if (array.size() != 2) {
            showMessage(MessageType::Error, "rm requires one file name\n");
            return true;
        }
        bare::Volume::Error error = FileSystem::sharedFileSystem()->remove(array[1].c_str());
        if (error != bare::Volume::Error::OK) {
            showMessage(MessageType::Error, "attempting to rm: %s\n", FileSystem::sharedFileSystem()->errorDetail(error));
        } else {
            showMessage(MessageType::Info, "'%s' removed\n", array[1].c_str());
        }
        return true;
    } else if (array[0] == "mv") {
        if (array.size() != 3) {
            showMessage(MessageType::Error, "mv requires from and to file names\n");
            return true;
        }
        
        File* fp = FileSystem::sharedFileSystem()->open(array[1].c_str(), FileSystem::OpenMode::Read);
        if (!fp->valid()) {
            if (fp->error() == bare::Volume::Error::FileNotFound) {
                showMessage(MessageType::Error, "from filename '%s' does not exist\n", array[1].c_str());
            } else {
                showMessage(MessageType::Error, "open of '%s' failed: %s\n", array[1].c_str(), FileSystem::sharedFileSystem()->errorDetail(fp->error()));
            }
            delete fp;
            return true;
        }

        bare::Volume::Error error = fp->rename(array[2].c_str());
        if (error == bare::Volume::Error::FileExists) {
            showMessage(MessageType::Error, "to filename '%s' exists. Please select a new file name\n", array[2].c_str());
        } else if (fp->error() != bare::Volume::Error::OK) {
            showMessage(MessageType::Error, "rename of '%s' to '%s' failed: %s\n", array[1].c_str(), array[2].c_str(), FileSystem::sharedFileSystem()->errorDetail(error));
        } else {
            showMessage(MessageType::Info, "'%s' renamed to '%s'\n", array[1].c_str(), array[2].c_str());
        }
        
        delete fp;
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
            bare::RealTime t(
                    static_cast<uint32_t>(dateArray[0]),
                    static_cast<uint32_t>(dateArray[1]),
                    static_cast<uint32_t>(dateArray[2]), 
                    static_cast<uint32_t>(timeArray[0]), 
                    static_cast<uint32_t>(timeArray[1]), 
                    static_cast<uint32_t>(timeArray[2]));
            bare::Timer::setCurrentTime(t);
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

