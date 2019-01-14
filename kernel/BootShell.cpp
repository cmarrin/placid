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

#include "BootShell.h"

#include "bare/Graphics.h"
#include "bare/Serial.h"
#include "bare/WiFiSpi.h"
#include "bare/Timer.h"
#include "bare/XYModem.h"
#include "Allocator.h"
#include "Dispatcher.h"
#include "FileSystem.h"

using namespace placid;

static bool waitForSlaveBitSet(bare::SPIMaster* spi, uint8_t bit)
{
    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 10; ++j) {
            uint32_t status = spi->receiveStatus(1);
            if (status & bit) {
                return true;
            }
        }
        bare::Timer::usleep(1000);
    }
    return false;
}

static void testSPI()
{
    bare::SPIMaster spi;
    spi.init();
    
    for (int i = 0; i < 10; ++i) {
        // Wait for slave to be ready to receive data
        if (!waitForSlaveBitSet(&spi, 0x01)) {
            bare::Serial::printf("*** Error: timeout waiting for slave to be ready to receive data\n");
            return;
        }

        const char* str = "Are you there?";
        spi.sendData(reinterpret_cast<const uint8_t*>(str), 15);
        
        // Wait for slave to have data to send
        if (!waitForSlaveBitSet(&spi, 0x02)) {
            bare::Serial::printf("*** Error: timeout waiting for slave to have data to send\n");
            return;
        }

        uint8_t buffer[15];
        spi.receiveData(buffer, 15);
        if (strcmp("I am here.", reinterpret_cast<const char*>(buffer)) != 0) {
            bare::Serial::printf("*** Error: no match, got '%s'\n", buffer);
            return;
        }
        bare::Serial::printf("*");
    }
    
    spi.sendStatus(0x04, 1);
    bare::Serial::printf("\nDone.\n\n");
}

static void testWifi()
{
    bare::SPIMaster spi;
    spi.init();
    bare::WiFiSpi wifi(&spi);
    wifi.init();

    // check for the presence of the ESP module:
    bare::WiFiSpi::Status status = wifi.status();
    if (status == bare::WiFiSpi::Status::NoShield) {
        bare::Serial::printf("WiFi module not present");
    } else {
        bare::Serial::printf("WiFiSpi status=%#04x\n", static_cast<uint8_t>(status));
        
        bare::String fv = wifi.firmwareVersion();
        if (fv != "0.1.2") {
            bare::Serial::printf("WiFiSpi firmware version='%s', expected 0.1.2\n", fv.c_str());
        } else {
            bare::WiFiSpi::Status status = bare::WiFiSpi::Status::Idle;
            while (status != bare::WiFiSpi::Status::Connected) {
                bare::Serial::printf("Wifi status: %d\n", status);
                bare::Timer::usleep(1000000);
                status = wifi.status();
            }

            // you're connected now, so print out the data:
            bare::Serial::printf("You're connected to '%s'\n", wifi.SSID().c_str());
        }
    }
}

static void testDraw()
{
    if (!bare::Graphics::init()) {
        bare::Serial::printf("Graphics failed to init\n");
        return;
    }
    bare::Graphics::clear(0xff00ff00);
    bare::Graphics::drawTriangle();
    bare::Graphics::render();
}

const char* BootShell::welcomeString() const
{
	return "\n\nPlacid Kernel Shell v0.2";
}

const char* BootShell::helpString() const
{
	return
            "    date [<time/date>] : set/get time/date\n"
            "    debug [on/off]     : turn debugging on/off\n"
            "    heap               : show heap status\n"
            "    put <file>         : put file (X/YModem send)\n"
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

static bare::String timeString()
{
    static const char* days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
    bare::RealTime currentTime = bare::Timer::currentTime();
    return bare::String::format("%s %d/%d/%d %d:%02d:%02d",
        days[currentTime.dayOfWeek()],
        currentTime.month(), currentTime.day(), currentTime.year(),
        currentTime.hours(), currentTime.minutes(), currentTime.seconds());
}

bool BootShell::executeShellCommand(const std::vector<bare::String>& array)
{
    if (array[0] == "ls") {
        bare::DirectoryIterator* it = FileSystem::sharedFileSystem()->directoryIterator("/");
        for ( ; *it; it->next()) {
            showMessage(MessageType::Info, "%-13s %10d\n", it->name(), it->size());
        }
        delete it;
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
            delete fp;
            return true;
        }
        
        showMessage(MessageType::Info, "Start X/YModem download when ready, or press [esc] key to cancel...\n");
        showMessage(MessageType::Info, "    (Use YModem to accurately set file size)\n");
        
        bare::XYModem xyModem(
            [](uint8_t& c) { bare::Serial::read(c); },
            [](uint8_t c) { bare::Serial::write(c); },
            []() -> bool { return bare::Serial::rxReady(); },
            []() -> uint32_t { return static_cast<uint32_t>(bare::Timer::systemTime() / 1000); });

        if (!xyModem.receive([fp](char byte) -> bool
        {
            return fp->write(&byte, 1) == 1;
        })) {
            bare::Timer::usleep(100000);
            showMessage(MessageType::Error, "X/YModem upload failed\n");
            delete fp;
            bare::Volume::Error error = FileSystem::sharedFileSystem()->remove(array[1].c_str());
            if (error != bare::Volume::Error::OK) {
                showMessage(MessageType::Error, "deletion of '%s' failed: %s\n",
                    array[1].c_str(), FileSystem::sharedFileSystem()->errorDetail(error));
            }
        } else {
            bare::Timer::usleep(100000);
            showMessage(MessageType::Info, "'%s' uploaded, size=%d\n", array[1].c_str(), fp->size());
            delete fp;
        }
    } else if (array[0] == "reset") {
        bare::restart();
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
            std::vector<bare::String> dateArray = array[1].split("/");
            std::vector<bare::String> timeArray = array[2].split(":");
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
        uint32_t size = Allocator::kernelAllocator().size();
        showMessage(MessageType::Info, "heap size: %d\n", size);
    } else if (array[0] == "run") {
        if (array.size() < 2) {
            showMessage(MessageType::Error, "enter a program to run\n");
        } else {
            Dispatcher::instance().exec(array[1]);
        }
    } else if (array[0] == "stop") {
        showMessage(MessageType::Info, "Program stopped\n");
    } else if (array[0] == "debug") {
        showMessage(MessageType::Info, "Debug true\n");
    } else if (array[0] == "test") {
        if (array.size() < 2) {
            showMessage(MessageType::Error, "need a test command\n");
        } else if (array[1] == "spi") {
            bare::Serial::printf("SPI test\n");
            testSPI();
        } else if (array[1] == "wifi") {
            bare::Serial::printf("Wifi (ESP) test\n");
            testWifi();
        } else if (array[1] == "draw") {
            bare::Serial::printf("GPU Drawing test\n");
            testDraw();
        } else {
            showMessage(MessageType::Error, "invalid test command\n");
        }
    }
    return true;
}

