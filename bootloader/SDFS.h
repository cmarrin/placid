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

namespace placid {

const uint32_t FilenameLength = 32;

class SDFS;

class File {
    friend class SDFS;
    
public:      
    static int32_t read(const File&, char* buf, uint64_t blockAddr, uint32_t blocks);    

    static bool valid(const File& file) { return file._error == 0; }
    static uint32_t error(const File& file) { return file._error; }

protected:
    uint32_t _error = 0;
    uint64_t _baseSector = 0;
    uint64_t _size = 0;
};

class DirectoryEntry {
    friend class SDFS;
public:
    DirectoryEntry() { }

    static const char* name(const DirectoryEntry& dir) { return dir._name; }
    static uint32_t size(const DirectoryEntry& dir) { return dir._size; }
    static uint32_t firstFileCluster(const DirectoryEntry& dir) { return dir._firstFileCluster; }
    static bool valid(const DirectoryEntry& dir) { return dir._valid; }
    
    static bool find(const SDFS&, DirectoryEntry&, const char* name);

private:
    bool _valid = false;
    char _name[FilenameLength];
    uint32_t _size = 0;
    uint32_t _firstFileCluster;
    uint32_t _dirCluster = 0;
};

class SDFS {
public:
    enum class Error {
        OK,
        UnsupportedType, 
        UnsupportedDevice, 
        UnsupportedPartition, 
        UnsupportedSectorSize,
        UnsupportedFATCount,
        BadMBRSignature,
        BadBPBSignature,
        SDCardInitFailed,
        MBRReadError,
        BPBReadError,
        RootDirReadError,
        OnlyFAT32LBASupported,
        InvalidFAT32Volume,
        Incomplete
    };
    
    SDFS() { }
    
    static Error mount(SDFS&, uint8_t device, uint8_t partition);
    static bool mounted(const SDFS& fs) { return fs._mounted; }
    static bool directory(const SDFS&, DirectoryEntry&);
    static bool open(const SDFS&, File&, const char* name, const char* mode);

    static uint32_t sizeInSectors(const SDFS& fs) { return fs._sizeInSectors; }
    static uint32_t sectorsPerCluster(const SDFS& fs) { return fs._sectorsPerCluster; }
    
    static uint32_t clusterToSector(const SDFS& fs, uint32_t cluster)
    {
        return fs._startDataSector + (cluster - 2) * fs._sectorsPerCluster;
    }
    
    private:    
    bool _mounted = false;
    uint32_t _firstSector = 0;                  // first sector of this partition
    uint32_t _sizeInSectors = 0;                // size in sectors of this partition
    uint8_t _sectorsPerCluster = 0;             // cluster size in sectors
    uint32_t _sectorsPerFAT = 0;                // size of a fat in sectors
    uint32_t _rootDirectoryStartCluster = 0;
    uint32_t _startFATSector = 0;
    uint32_t _startDataSector = 0;
};

}

