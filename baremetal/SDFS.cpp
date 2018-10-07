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

#include "SDFS.h"

//#include "sdcard.h"
#include "util.h"
#include "Serial.h"
#include "Timer.h"

using namespace placid;

static int32_t readRaw(char* buf, uint64_t sectorAddr, uint32_t sectors)
{
//    int r = sdTransferBlocks(sectorAddr * 512, sectors, reinterpret_cast<uint8_t*>(buf), 0);
//    if (r != 0) {
//        Serial::printf("*** Disk Read Error: return code=%d\n", r);
//        return -r;
//    }
    return static_cast<int32_t>(sectors);
}

static int32_t writeRaw(const char* buf, uint64_t sectorAddr, uint32_t sectors)
{
    // FIXME: Implement
    return -1;
}

SDFS::Error SDFS::mount(SDFS& fs, uint8_t device, uint8_t partition)
{
    if (device != 0) {
        return Error::UnsupportedDevice;
    }
    
    //sdInit();
    Timer::usleep(100);
    //int r = sdInitCard();
    Timer::usleep(10000);
//    if (r != SD_OK && r != SD_CARD_CHANGED) {
//        return Error::SDCardInitFailed;
//    }
    
    return static_cast<Error>(FAT32::FS::mount(fs._fatfs, partition, readRaw, writeRaw));
}

bool SDFS::open(const SDFS& fs, File& file, const char* name, const char* mode)
{
    if (!fs._fatfs.find(fs._fatfs, file._file, name)) {
        return false;
    }
    
    return true;
}

SDFS::Error File::read(SDFS& fs, File& file, char* buf, uint64_t sectorAddr, uint32_t sectors)
{
    file._error = static_cast<SDFS::Error>(fs._fatfs.read(fs._fatfs, file._file, buf, sectorAddr, sectors));
    return file._error;
}

SDFS::Error File::write(SDFS& fs, File& file, const char* buf, uint64_t sectorAddr, uint32_t sectors)
{
    file._error = static_cast<SDFS::Error>(fs._fatfs.write(fs._fatfs, file._file, buf, sectorAddr, sectors));
    return file._error;
}
