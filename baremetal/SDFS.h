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

#pragma once

#include <stdint.h>
#include "FAT32.h"

namespace placid {

const uint32_t FilenameLength = 32;

class File;

class SDFS {
    friend class File;
    
public:
    enum class Error {
        OK = 0,
        UnsupportedDevice, 
        SDCardInitFailed,
    };
    
    SDFS() { }
    
    static Error mount(SDFS&, uint8_t device, uint8_t partition);
    static bool open(const SDFS&, File&, const char* name, const char* mode);

    static uint32_t sizeInSectors(const SDFS& fs) { return fs._fatfs.sizeInSectors(fs._fatfs); }
    
    private:    
        FAT32::FS _fatfs;
};

class File {
    friend class SDFS;
    
public:      
    static SDFS::Error read(SDFS&, File&, char* buf, uint64_t sectorAddr, uint32_t sectors);    
    static SDFS::Error write(SDFS&, File&, const char* buf, uint64_t sectorAddr, uint32_t sectors);    

    static bool valid(const File& file) { return file._error == SDFS::Error::OK; }
    static uint32_t size(const File& file) { return file._file._size; }
    static SDFS::Error error(const File& file) { return file._error; }

protected:
    FAT32::FS::File _file;
    SDFS::Error _error = SDFS::Error::OK;
};

}

