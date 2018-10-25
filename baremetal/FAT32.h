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

#include "Volume.h"
#include <stdint.h>

namespace bare {

// Strong typed cluster
class ClusterType;
using Cluster = Scalar<ClusterType, uint32_t>;


class FAT32 : public Volume
{
public:
    static constexpr uint32_t FilenameLength = 32;
    
    enum class Error {
        UnsupportedType = 1000, 
        UnsupportedPartition, 
        UnsupportedBlockSize,
        UnsupportedFATCount,
        BadMBRSignature,
        BadBPBSignature,
        MBRReadError,
        BPBReadError,
        FATReadError,
        FATWriteError,
        DirReadError,
        OnlyFAT32LBASupported,
        InvalidFAT32Volume,
        WrongSizeRead,
        WrongSizeWrite,
        Incomplete,
    };
    
    struct FileInfo {
        char name[FilenameLength]; // Passed in name converted to 8.3
        uint32_t size = 0;
        Cluster baseCluster = 0;
    };

    FAT32(Volume::RawIO* rawIO, uint8_t partition) : _rawIO(rawIO), _partition(partition) { }
    
    virtual uint32_t sizeInBlocks() const override { return _sizeInBlocks; }
    virtual Volume::Error mount() override;
    virtual RawFile* open(const char* name) override;
    virtual Volume::Error create(const char* name) override;
    virtual Volume::Error remove(const char* name) override;
    virtual const char* errorDetail(Volume::Error) const override;
    virtual DirectoryIterator* directoryIterator(const char* path) override;

    Volume::Error rawRead(char* buf, Block block, uint32_t blocks);    
    Volume::Error rawWrite(const char* buf, Block block, uint32_t blocks);    
    bool mounted() { return _mounted; }
    
    Cluster rootDirectoryStartCluster() const { return _rootDirectoryStartCluster; }
    uint32_t blocksPerCluster() { return _blocksPerCluster; }
    uint32_t clusterSize() { return _blocksPerCluster * 512; }
    
    Block clusterToBlock(Cluster cluster)
    {
        return _startDataBlock + Block((cluster.value() - 2) * _blocksPerCluster);
    }

    enum class FATEntryType { Normal, Free, End, Error };
    FATEntryType nextClusterFATEntry(Cluster cluster, Cluster& nextCluster);
    
    // Passing 0 as prev Cluster indicates that this is the first cluster of a file
    Cluster allocateCluster(Cluster prev = 0);
    
    static uint32_t bufToUInt32(uint8_t* buf)
    {
        return  static_cast<uint32_t>(buf[0]) + 
                static_cast<uint32_t>((buf[1] << 8)) + 
                static_cast<uint32_t>((buf[2] << 16)) + 
                static_cast<uint32_t>((buf[3] << 24));
    }

    static uint16_t bufToUInt16(uint8_t* buf)
    {
        return  static_cast<uint16_t>(buf[0]) + 
                static_cast<uint16_t>((buf[1] << 8));
    }
    
    static void uint32ToBuf(uint32_t value, uint8_t* buf)
    {
        buf[0] = static_cast<uint8_t>(value);
        buf[1] = static_cast<uint8_t>(value >> 8);
        buf[2] = static_cast<uint8_t>(value >> 16);
        buf[3] = static_cast<uint8_t>(value >> 24);
    }

    static void uint16ToBuf(uint16_t value, uint8_t* buf)
    {
        buf[0] = static_cast<uint8_t>(value);
        buf[1] = static_cast<uint8_t>(value >> 8);
    }

private:
    bool find(FileInfo&, const char* name);
    bool readFATBlock(uint32_t block);
    bool writeFATBlock();

    bool _mounted = false;
    Block _firstBlock = 0;                  // first block of this partition
    uint32_t _sizeInBlocks = 0;             // size in blocks of this partition
    uint8_t _blocksPerCluster = 0;          // cluster size in blocks
    Cluster _rootDirectoryStartCluster = 0; // first cluster of root dir
    Block _startFATBlock = 0;               // location of FAT
    uint32_t _blocksPerFAT = 0;             // size of a FAT in blocks
    Block _startDataBlock = 0;              // start of data
    
    char _fatBuffer[512] __attribute__((aligned(4)));
    uint32_t _currentFATBufferAddr = 0;
    bool _fatBufferValid = false;
    bool _fatBufferNeedsWriting = false;
    
    Volume::RawIO* _rawIO = nullptr;
    uint8_t _partition = 0;
    Error _error = static_cast<FAT32::Error>(Volume::Error::OK);
};

}

