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

#include "sdcard.h"
#include "bootutil.h"

using namespace placid;

int32_t File::read(const File& file, char* buf, uint32_t size)
{
    // FIXME: For now we will always ask for 1 block from the start of the file and assume buf can hold 512 bytes
    int r = sdTransferBlocks(file._baseAddress, 1, reinterpret_cast<uint8_t*>(buf), 0);
    if (r != 0) {
        printf("*** Disk Read Error: return code=%d\n", r);
        return -1;
    }
    return static_cast<int32_t>(size);
}

int32_t File::write(File& file, const char* buf, uint32_t size)
{
    // FIXME: Implement
    return -1;
}

bool SDFS::mount(SDFS& fs, uint8_t device, uint8_t partition)
{
    // FIXME: For now assume device=0 and partition=0
    if (fs._mounted) {
        return true;
    }
    sdInit();
    delay(100);
    int r = sdInitCard();
    delay(10000);
    if (r != SD_OK && r != SD_CARD_CHANGED) {
        printf("*** Disk Initialize Error: return code=%d\n", r);
        return false;
    }
    
    fs._mounted = true;
    return true;
}

bool SDFS::open(const SDFS& fs, File& file, const char* name, const char* mode)
{
    // FIXME: Implement
    return false;
}
