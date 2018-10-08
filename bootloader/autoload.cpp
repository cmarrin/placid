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
#include "SDFS.h"
#include "Timer.h"
#include "util.h"

using namespace placid;

void autoload()
{
    SDFS fs;
    SDFS::Error e = fs.mount(0, 0);
    if (e != SDFS::Error::OK) {
        Serial::printf("*** error mounting:%d\n", static_cast<int>(e));
        return;
    }
    
    File fp;
    bool r = fs.open(fp, "kernel.bin", "r");
    if (!r) {
        Serial::printf("*** File open error:%d\n", File::error(fp));
        return;
    }
    
    // FIXME: Currently only loads one cluster, limiting the file size to 32KB
    uint32_t addr = ARMBASE;
    uint32_t size = File::size(fp);
    
    for (uint32_t sector = 0; size != 0; ++sector) {
        char buf[512];
        SDFS::Error result = File::read(fp, buf, sector, 1);
        if (result != SDFS::Error::OK) {
            Serial::printf("*** File read error:%d\n", static_cast<uint32_t>(result));
            return;
        }
        
        uint32_t bytesToLoad = (size > 512) ? 512 : size;
        for (uint32_t i = 0; i < bytesToLoad; i++) {
            PUT8(addr++, buf[i]);
        }
        size -= bytesToLoad;
    }
    
    Serial::printf("Autoload complete, executing...\n");
    Timer::usleep(200000);
    BRANCHTO(ARMBASE);
}
