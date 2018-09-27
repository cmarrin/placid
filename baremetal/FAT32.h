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

namespace FAT32 {

class FS {
public:
    static constexpr uint32_t FilenameLength = 32;

    typedef int32_t (*ReadRawCB)(char* buf, uint64_t sectorAddr, uint32_t sectors);
    typedef int32_t (*WriteRawCB)(const char* buf, uint64_t sectorAddr, uint32_t sectors);

    enum class Error {
        OK = 0,
        UnsupportedType = 100, 
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
        WrongSizeRead,
        WrongSizeWrite,
        NotImplemented,
        Incomplete
    };
    
    struct File {
        char _name[FilenameLength]; // Passed in name converted to 8.3
        uint32_t _size = 0;
        uint32_t _baseSector = 0;
    };

    FS() { }
    
    static Error mount(FS&, uint8_t partition, ReadRawCB, WriteRawCB);
    static bool mounted(const FS& fs) { return fs._mounted; }
    
    static bool find(const FS&, File&, const char* name);
    
    static Error read(const FS&, const File&, char* buf, uint64_t sectorAddr, uint32_t sectors);    
    static Error write(const FS&, const File&, const char* buf, uint64_t sectorAddr, uint32_t sectors);    
    
    static uint32_t sizeInSectors(const FS& fs) { return fs._sizeInSectors; }

    private:    
    static uint32_t sectorsPerCluster(const FS& fs) { return fs._sectorsPerCluster; }
    static uint32_t clusterSize(const FS& fs) { return fs._sectorsPerCluster * 512; }
    
    static uint32_t clusterToSector(const FS& fs, uint32_t cluster)
    {
        return fs._startDataSector + (cluster - 2) * fs._sectorsPerCluster;
    }
    
    bool _mounted = false;
    uint32_t _firstSector = 0;                  // first sector of this partition
    uint32_t _sizeInSectors = 0;                // size in sectors of this partition
    uint8_t _sectorsPerCluster = 0;             // cluster size in sectors
    uint32_t _rootDirectoryStartSector = 0;     // first sector of root dir
    uint32_t _startFATSector = 0;               // location of FAT
    uint32_t _sectorsPerFAT = 0;                // size of a FAT in sectors
    uint32_t _startDataSector = 0;              // start of data
    
    ReadRawCB _readRaw;
    WriteRawCB _writeRaw;
};

}

