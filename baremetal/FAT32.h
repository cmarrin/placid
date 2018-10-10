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

#include "FS.h"
#include <stdint.h>

namespace bare {

class FAT32 : public FS::Device
{
public:
    friend class FAT32DirectoryIterator;
    
    static constexpr uint32_t FilenameLength = 32;
    
    enum class Error {
        OK = 0,
        UnsupportedType = 100, 
        UnsupportedPartition, 
        UnsupportedBlockSize,
        UnsupportedFATCount,
        BadMBRSignature,
        BadBPBSignature,
        MBRReadError,
        BPBReadError,
        DirReadError,
        FileNotFound,
        OnlyFAT32LBASupported,
        InvalidFAT32Volume,
        WrongSizeRead,
        WrongSizeWrite,
        NotImplemented,
        Incomplete
    };
    
    FAT32(FS::RawIO* rawIO, uint8_t partition) : _rawIO(rawIO), _partition(partition) { }
    
    virtual FS::Error mount() override;
    virtual FS::Error read(char* buf, uint32_t baseBlock, uint32_t relativeBlock, uint32_t blocks) override;    
    virtual FS::Error write(const char* buf, uint32_t baseBlock, uint32_t relativeBlock, uint32_t blocks) override;    
    virtual bool find(FS::FileInfo&, const char* name) override;
    virtual uint32_t sizeInBlocks() const override { return _sizeInBlocks; }
    virtual const char* errorDetail() const override;
    virtual DirectoryIterator* directoryIterator(const char* path) override;

    bool mounted() { return _mounted; }
    
    uint32_t rootDirectoryStartBlock() const { return _rootDirectoryStartBlock; }
    uint32_t blocksPerCluster() { return _blocksPerCluster; }
    uint32_t clusterSize() { return _blocksPerCluster * 512; }
    
    uint32_t clusterToBlock(uint32_t cluster)
    {
        return _startDataBlock + (cluster - 2) * _blocksPerCluster;
    }
    
    
private:
    bool _mounted = false;
    uint32_t _firstBlock = 0;                  // first block of this partition
    uint32_t _sizeInBlocks = 0;                // size in blocks of this partition
    uint8_t _blocksPerCluster = 0;             // cluster size in blocks
    uint32_t _rootDirectoryStartBlock = 0;     // first block of root dir
    uint32_t _startFATBlock = 0;               // location of FAT
    uint32_t _blocksPerFAT = 0;                // size of a FAT in blocks
    uint32_t _startDataBlock = 0;              // start of data
    
    FS::RawIO* _rawIO = nullptr;
    uint8_t _partition = 0;
    Error _error = Error::OK;
};

}

