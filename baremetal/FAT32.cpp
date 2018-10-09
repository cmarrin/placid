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

#include "FAT32.h"

#include "util.h"
#include "Serial.h"

using namespace placid;

struct PartitionEntry
{
    uint8_t status;
    uint8_t chsFirst[3];
    uint8_t type;
    uint8_t chsLast[3];
    uint8_t lbaStart[4];
    uint8_t lbaCount[4];
};

static_assert(sizeof(PartitionEntry) == 16, "Wrong PartitionEntry size");

struct MBR
{
    char bootstrap[446];
    PartitionEntry partitions[4];
    uint8_t signature[2];
};

static_assert(sizeof(MBR) == 512, "Wrong MBR size");

struct BootBlock
{
    char jump[3];
    char oemName[8];
    
    // DOS 2.0 (0x0b)
    uint8_t bytesPerBlock[2];
    uint8_t blocksPerCluster;
    uint8_t reservedBlocks[2];
    uint8_t numberOfFATCopies;
    uint8_t rootDirEntries[2];
    uint8_t numBlocksOld[2];
    uint8_t mediaDescriptor;
    uint8_t blocksPerFATOld[2];
    
    // DOS 3.31 (0x18)
    uint8_t blocksPerTrack[2];
    uint8_t headCount[2];
    uint8_t totalHiddenBlocks[4];
    uint8_t totalBlocks[4];
    
    // FAT32 extended (0x24)
    uint8_t blocksPerFAT32[4];
    uint8_t descriptionFlags[2];
    uint8_t version[2];
    uint8_t rootDirectoryStartCluster[4];
    uint8_t infoBlock[2];
    uint8_t backupBootBlock[2];
    uint8_t reserved[12];
    uint8_t logicalDriveNumber;
    uint8_t unused;
    uint8_t extendedSignature;
    uint8_t serialNumber[4];
    char volumeName[11];
    char fatName[8];
    uint8_t executableCode[420];
    uint8_t signature[2];
};

static_assert(sizeof(BootBlock) == 512, "Wrong BootBlock size");

struct FATDirEntry
{
    char name[11];
    uint8_t attr;
    uint8_t reserved1[8];
    uint8_t firstClusterHi[2];
    uint8_t reserved2[4];
    uint8_t firstClusterLo[2];
    uint8_t size[4];
};

static_assert(sizeof(FATDirEntry) == 32, "Wrong FATDirEntry size");

static inline uint32_t bufToUInt32(uint8_t* buf)
{
    return  static_cast<uint32_t>(buf[0]) + 
            static_cast<uint32_t>((buf[1] << 8)) + 
            static_cast<uint32_t>((buf[2] << 16)) + 
            static_cast<uint32_t>((buf[3] << 24));
}

static inline uint16_t bufToUInt16(uint8_t* buf)
{
    return  static_cast<uint16_t>(buf[0]) + 
            static_cast<uint16_t>((buf[1] << 8));
}

FS::Error FAT32::mount()
{
    _error = Error::OK;
    
    if (_partition >= 4) {
        _error = Error::UnsupportedPartition;
        return FS::Error::Failed;
    }
    
    if (_mounted) {
        return FS::Error::OK;
    }
    
    // Read the MBR
    char buf[512];
    if (_rawIO->read(buf, 0, 1) != 1) {
        _error = Error::MBRReadError;
        return FS::Error::Failed;
    }
    
    MBR* mbr = reinterpret_cast<MBR*>(buf);

    // Do validation checks
    if (mbr->signature[0] != 0x55 || mbr->signature[1] != 0xaa) {
        _error = Error::BadMBRSignature;
        return FS::Error::Failed;
    }
        
    uint8_t partitionType = mbr->partitions[_partition].type;
    if (partitionType != 0x0c) {
        _error = Error::OnlyFAT32LBASupported;
        return FS::Error::Failed;
    }
    
    // Exrtract the needed values
    _firstBlock = bufToUInt32(mbr->partitions[_partition].lbaStart);
    _sizeInBlocks = bufToUInt32(mbr->partitions[_partition].lbaCount);
    
    // Read the Boot Block
    if (_rawIO->read(buf, _firstBlock, 1) != 1) {
        _error = Error::BPBReadError;
        return FS::Error::Failed;
    }
    
    // Do validation checks
    BootBlock* bootBlock = reinterpret_cast<BootBlock*>(buf);
    if (bootBlock->signature[0] != 0x55 || bootBlock->signature[1] != 0xaa) {
        _error = Error::BadBPBSignature;
        return FS::Error::Failed;
    }
            
    if (bufToUInt16(bootBlock->bytesPerBlock) != 512) {
        _error = Error::UnsupportedBlockSize;
        return FS::Error::Failed;
    }
    
    if (bootBlock->numberOfFATCopies != 2) {
        _error = Error::UnsupportedFATCount;
        return FS::Error::Failed;
    }
    
    if (bufToUInt16(bootBlock->rootDirEntries) != 0) {
        _error = Error::InvalidFAT32Volume;
        return FS::Error::Failed;
    }
    
    if (bufToUInt16(bootBlock->blocksPerFATOld) != 0 || bufToUInt16(bootBlock->blocksPerFATOld) != 0) {
        _error = Error::InvalidFAT32Volume;
        return FS::Error::Failed;
    }
    
    // Exrtract the needed values
    uint16_t reservedBlocks = bufToUInt16(bootBlock->reservedBlocks);
    
    _blocksPerFAT = bufToUInt32(bootBlock->blocksPerFAT32);
    _blocksPerCluster = bootBlock->blocksPerCluster;
    _startFATBlock = _firstBlock + reservedBlocks;
    _startDataBlock = _startFATBlock + (_blocksPerFAT * 2);
    _rootDirectoryStartBlock = clusterToBlock(bufToUInt32(bootBlock->rootDirectoryStartCluster));
    
    _mounted = true;
    return FS::Error::OK;
}

bool FAT32::find(FS::FileInfo& fileInfo, const char* name)
{
    // Convert the incoming filename to 8.3 and then compare all 11 characters
    char nameToFind[11];
    convertTo8dot3(nameToFind, name);
    
    uint32_t dirBlock = _rootDirectoryStartBlock;

    char buf[512];
    static constexpr uint32_t EntriesPerBlock = 512 / 32;
    
    // Read one block at a time
    for (uint32_t dirBlockIndex = 0; dirBlockIndex < blocksPerCluster(); ++dirBlockIndex) {
        if (_rawIO->read(buf, dirBlock + dirBlockIndex, 1) != 1) {
            _error = Error::DirReadError;
            return false;
        }
        
        FATDirEntry* ent = reinterpret_cast<FATDirEntry*>(buf);
        
        // Go through each entry in this block and look for a match
        for (uint32_t entryIndex = 0; entryIndex < EntriesPerBlock; ++entryIndex) {
            if (memcmp(nameToFind, ent[entryIndex].name, 11) == 0) {
                // A match was found, init the directory object
                for (int i = 0; i < 11; ++i) {
                    fileInfo.name[i] = ent[entryIndex].name[i];
                }
                fileInfo.name[11] = '\0';
        
                fileInfo.size = bufToUInt32(ent[entryIndex].size);
        
                // first cluster is in this crazy hi/lo split format
                uint32_t baseCluster = (static_cast<uint32_t>(bufToUInt16(ent[entryIndex].firstClusterHi)) << 16) + static_cast<uint32_t>(bufToUInt16(ent[entryIndex].firstClusterLo));
                fileInfo.baseBlock = clusterToBlock(baseCluster);
                return true;
            }
        }
    }
    
    // No match
    _error = Error::FileNotFound;
    return false;
}
    
FS::Error FAT32::read(char* buf, uint32_t baseBlock, uint32_t relativeBlock, uint32_t blocks)
{
    int32_t result = _rawIO->read(buf, (baseBlock + relativeBlock), blocks);
    _error = (result == static_cast<int32_t>(blocks)) ? Error::OK : Error::WrongSizeRead;
    return (_error == Error::OK) ? FS::Error::OK : FS::Error::Failed;
}

FS::Error FAT32::write(const char* buf, uint32_t baseBlock, uint32_t relativeBlock, uint32_t blocks)
{
    int32_t result = _rawIO->write(buf, (baseBlock + relativeBlock), blocks);
    _error = (result == static_cast<int32_t>(blocks)) ? Error::OK : Error::WrongSizeWrite;
    return (_error == Error::OK) ? FS::Error::OK : FS::Error::Failed;
}

const char* FAT32::errorDetail() const
{
    switch(_error) {
    case Error::OK:                     return "OK";
    case Error::UnsupportedType:        return "unsupported type";
    case Error::UnsupportedPartition:   return "unsupported partition";
    case Error::UnsupportedBlockSize:  return "unsupported block size";
    case Error::UnsupportedFATCount:    return "unsupported FAT count";
    case Error::BadMBRSignature:        return "bad MBR signature";
    case Error::BadBPBSignature:        return "bad BPB signature";
    case Error::MBRReadError:           return "MBR read error";
    case Error::BPBReadError:           return "BPB read error";
    case Error::DirReadError:           return "dir read error";
    case Error::FileNotFound:           return "file not found";
    case Error::OnlyFAT32LBASupported:  return "only FAT32 LBA supported";
    case Error::InvalidFAT32Volume:     return "invalid FAT32 volume";
    case Error::WrongSizeRead:          return "wrong size read";
    case Error::WrongSizeWrite:         return "wrong size write";
    case Error::NotImplemented:         return "not implemented";
    case Error::Incomplete:             return "incomplete";
    default:                            return "***";
    }
}
