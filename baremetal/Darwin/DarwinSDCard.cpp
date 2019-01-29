/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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
