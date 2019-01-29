/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Serial.h"
#include "bare/FAT32.h"
#include "bare/SDCard.h"
#include "bare/Timer.h"

static const char* KernelFileName = "kernel.bin";

void autoload()
{
    bare::Serial::printf("\n\nAutoloading '%s'...\n", KernelFileName);
    
    bare::SDCard sdCard;
    bare::FAT32 fatFS(&sdCard, 0);
    bare::Volume::Error e = fatFS.mount();
    if (e != bare::Volume::Error::OK) {
        bare::Serial::printf("*** error mounting:%s\n", fatFS.errorDetail(e));
        return;
    }
    
    bare::RawFile* fp = fatFS.open(KernelFileName);
    if (!fp) {
        bare::Serial::printf("*** File open error:%s\n", fatFS.errorDetail(fp->error()));
        return;
    }
    
    uint8_t* addr = bare::kernelBase();
    uint32_t size = fp->size();
    
    for (uint32_t block = 0; size != 0; ++block) {
        char buf[512];
        bare::Volume::Error result = fp->read(buf, block, 1);
        if (result != bare::Volume::Error::OK) {
            bare::Serial::printf("*** File read error:%d\n", static_cast<uint32_t>(result));
            return;
        }
        
        uint32_t bytesToLoad = (size > 512) ? 512 : size;
        for (uint32_t i = 0; i < bytesToLoad; i++) {
            bare::PUT8(addr++, buf[i]);
        }
        size -= bytesToLoad;
    }
    
    bare::BRANCHTO(bare::kernelBase());
}
