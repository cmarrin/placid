/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "BootShell.h"

#include "bare/Graphics.h"
#include "bare/Serial.h"
#include "bare/WiFiSPI.h"
#include "bare/Timer.h"
#include "bare/XYModem.h"
#include "Allocator.h"
#include "Dispatcher.h"
#include "FileSystem.h"

using namespace placid;

static void testFS()
{
    // Make sure file system is mounted
    bare::Volume::Error fsError = FileSystem::sharedFileSystem()->error();
    
    if (fsError != bare::Volume::Error::OK) {
        bare::Serial::printf("File system not mounted: %s\n", FileSystem::sharedFileSystem()->errorDetail(fsError));
        bare::Serial::printf("    Skipping file system tests\n");
    } else {
        // Test file read
        bare::Serial::printf("File read test...\n");
        File* fp = FileSystem::sharedFileSystem()->open("sample.txt", FileSystem::OpenMode::Read, FileSystem::OpenOption::Update);
        if (!fp->valid()) {
            bare::Serial::printf("File read open error for '%s': %s\n", "sample.txt", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        } else {
            char buf[26];
            fp->seek(511, File::SeekWhence::Set);
            size_t size = fp->read(buf, 25);
            buf[25] = '\0';
            if (size < 0) {
                bare::Serial::printf("File read error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
            } else {
                bare::Serial::printf("File read:'%s'\n", buf);
            }
        }
        
        // Test update
        bare::Serial::printf("File update test...\n");
        fp->seek(518, File::SeekWhence::Set);
        size_t size = fp->write("0123456789", 10);
        if (size < 0) {
            bare::Serial::printf("File update error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        } else {
            fp->seek(-17, File::SeekWhence::Cur);
            char buf[26];
            size_t size = fp->read(buf, 25);
            buf[25] = '\0';
            if (size < 0) {
                bare::Serial::printf("Reading back after update error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
            } else {
                bare::Serial::printf("After update read:'%s'\n", buf);
            }
        }
        
        // Repair the file
        bare::Serial::printf("File repair test...\n");
        fp->seek(518, File::SeekWhence::Set);
        size = fp->write("altogether", 10);
        if (size < 0) {
            bare::Serial::printf("File repair error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        } else {
            fp->seek(-17, File::SeekWhence::Cur);
            char buf[26];
            size_t size = fp->read(buf, 25);
            buf[25] = '\0';
            if (size < 0) {
                bare::Serial::printf("Reading back after repair error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
            } else {
                bare::Serial::printf("After repair read:'%s'\n", buf);
            }
        }

        delete fp;
        fp = nullptr;
        
        // Test file write
        bare::Serial::printf("File write test...\n");
        FileSystem::sharedFileSystem()->remove("test.txt");
        
        fp = FileSystem::sharedFileSystem()->open("test.txt", FileSystem::OpenMode::Write);
        if (!fp->valid()) {
            bare::Serial::printf("File write open error for '%s': %s\n", "test.txt", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        } else {
            size_t size = fp->write("The quick brown fox", 19);
            if (size < 0) {
                bare::Serial::printf("File write error: %s\n", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
            } else {
                bare::Serial::printf("File write successful\n");
            }
        }
        
        if (fp->close() != bare::Volume::Error::OK) {
            bare::Serial::printf("File write close error for '%s': %s\n", "test.txt", FileSystem::sharedFileSystem()->errorDetail(fp->error()));
        }
        
        delete fp;
        fp = nullptr;
        
        FileSystem::sharedFileSystem()->remove("test.txt");
    }
    
}

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
    spi.init(8000000);
    
    for (int i = 0; i < 50; ++i) {
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
    bare::WiFiSPI wifi(&spi);
    wifi.init();

    // check for the presence of the ESP module:
    bare::WiFiSPI::Status status = wifi.status();
    if (status == bare::WiFiSPI::Status::NoShield) {
        bare::Serial::printf("WiFi module not present");
    } else {
        bare::Serial::printf("WiFiSPI status=%s\n", bare::WiFiSPI::statusDetail(status));
        
        bare::String s = wifi.firmwareVersion();
        bare::Serial::printf("WiFiSPI firmware version='%s'\n", s.c_str());
        s = wifi.protocolVersion();
        bare::Serial::printf("WiFiSPI protocol version='%s'\n", s.c_str());
        
        bare::Serial::printf("Scanning network...\n");

        wifi.startNetworkScan();
        bool succeeded = false;
        uint8_t attempts = 20;
        uint8_t networkCount = 0;
        
        while (--attempts > 0) {
            status = wifi.checkNetworkScan(networkCount);
            if (status == bare::WiFiSPI::Status::ScanCompleted) {
                succeeded = true;
                break;
            } else if (status == bare::WiFiSPI::Status::Failure) {
                bare::Serial::printf("WiFi network scan failed\n");
                break;
            } else if (status != bare::WiFiSPI::Status::Scanning) {
                bare::Serial::printf("Unexpected status from checkNetworkScan:%s\n", bare::WiFiSPI::statusDetail(status));
                break;
            }
            bare::Timer::usleep(1000000);
        }
        
        if (!succeeded) {
            if (attempts == 0) {
                bare::Serial::printf("WiFi network scan timed out\n");
            }
        } else {
            bare::Serial::printf("Scanned networks (%d):\n", networkCount);
            for (uint8_t i = 0; i < networkCount; ++i) {
                bare::String ssid;
                int32_t rssi;
                uint8_t encryptionType;
                wifi.scannedNetworkItem(i, ssid, rssi, encryptionType);
                bare::Serial::printf("    ssid=%s, rssi=%d, encr=%d\n", ssid.c_str(), rssi, encryptionType);
            }
            bare::Serial::printf("done\n");
        }
        
        bare::Serial::printf("WiFi waiting to connect...\n");
        bare::WiFiSPI::Status status = bare::WiFiSPI::Status::Idle;
        for  (int i = 0; i < 20; ++i) {
            if (status == bare::WiFiSPI::Status::Connected) {
                bare::Serial::printf("You're connected to '%s'\n", wifi.SSID().c_str());
                return;
            }

            bare::Timer::usleep(1000000);
            status = wifi.status();
            bare::Serial::printf("    Wifi status: %s\n", bare::WiFiSPI::statusDetail(status));
        }

        bare::Serial::printf("Failed to connect\n");
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
        bare::reboot();
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
            bare::RealTime currentTime = bare::Timer::currentTime();
            showMessage(MessageType::Info, "current time: %s\n", currentTime.timeString(bare::RealTime::TimeFormat::DateTime).c_str());
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
            bare::RealTime currentTime = bare::Timer::currentTime();
            showMessage(MessageType::Info, "set current time to: %s\n", currentTime.timeString(bare::RealTime::TimeFormat::DateTime).c_str());
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
        } else if (array[1] == "fs") {
            bare::Serial::printf("Filesystem test\n");
            testFS();
        } else {
            showMessage(MessageType::Error, "invalid test command\n");
        }
    } else {
        return false;
    }
    return true;
}

