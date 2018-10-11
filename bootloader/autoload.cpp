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

#include "sdcard.h"
#include "Serial.h"
#include "FAT32.h"
#include "SDCard.h"
#include "FS.h"
#include "Timer.h"
#include "util.h"

static const char* KernelFileName = "kernel.bin";

void autoload()
{
    bare::Serial::printf("\n\nAutoloading '%s'...\n", KernelFileName);
    
    bare::SDCard sdCard;
    bare::FAT32 fatFS(&sdCard, 0);
    bare::FS fs;
    bare::FS::Error e = fs.mount(&fatFS);
    if (e != bare::FS::Error::OK) {
        bare::Serial::printf("*** error mounting:%s\n", fs.errorDetail());
        return;
    }
    
    bare::RawFile fp;
    bool r = fs.open(fp, KernelFileName, "r");
    if (!r) {
        bare::Serial::printf("*** File open error:%s\n", fs.errorDetail());
        return;
    }
    
    // FIXME: Currently only loads one cluster, limiting the file size to 32KB
    uint32_t addr = ARMBASE;
    uint32_t size = fp.size();
    
    for (uint32_t block = 0; size != 0; ++block) {
        char buf[512];
        bare::FS::Error result = fp.read(buf, block, 1);
        if (result != bare::FS::Error::OK) {
            bare::Serial::printf("*** File read error:%d\n", static_cast<uint32_t>(result));
            return;
        }
        
        uint32_t bytesToLoad = (size > 512) ? 512 : size;
        for (uint32_t i = 0; i < bytesToLoad; i++) {
            PUT8(addr++, buf[i]);
        }
        size -= bytesToLoad;
    }
    
    bare::Serial::printf("Autoload complete, executing...\n");
    bare::Timer::usleep(200000);
    BRANCHTO(ARMBASE);
}
