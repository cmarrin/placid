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

// This code was adapted from:
//
//  https://github.com/bztsrc/raspi3-tutorial/tree/master/0B_readsector
//
//  Copyright (C) 2018 bzt (bztsrc@github)


#include "bare.h"

#include "bare/SDCard.h"

using namespace bare;

#include <stdio.h>

static FILE* sdCardFP = nullptr;

SDCard::SDCard()
{
    // FAT32.img is a file containing an image of a FAT32 filesystem. Use
    // that to simulate an SD card
    sdCardFP = fopen("FAT32.img", "r+");
}

Volume::Error SDCard::read(char* buf, Block blockAddr, uint32_t blocks)
{
    if (!sdCardFP) {
        return Volume::Error::InternalError;
    }
    fseek(sdCardFP, blockAddr.value() * 512, SEEK_SET);
    size_t size = fread(buf, 1, blocks * 512, sdCardFP);
    return (size == blocks * 512) ? Volume::Error::OK : Volume::Error::Failed;
}

Volume::Error SDCard::write(const char* buf, Block blockAddr, uint32_t blocks)
{
    if (!sdCardFP) {
        return Volume::Error::InternalError;
    }
    fseek(sdCardFP, blockAddr.value() * 512, SEEK_SET);
    size_t size = fwrite(buf, 1, blocks * 512, sdCardFP);
    return (size == blocks * 512) ? Volume::Error::OK : Volume::Error::Failed;
}
