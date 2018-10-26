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

#include "FAT32DirectoryIterator.h"
#include "util.h"
#include "Serial.h"

using namespace bare;

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
    char bootstrap[0x1be];
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

Volume::Error FAT32::rawRead(char* buf, Block block, uint32_t blocks)
{
    return _rawIO->read(buf, block, blocks);
}

Volume::Error FAT32::rawWrite(const char* buf, Block block, uint32_t blocks)
{
    return _rawIO->write(buf, block, blocks);
}

Volume::Error FAT32::mount()
{
    _error = static_cast<FAT32::Error>(Volume::Error::OK);
    
    if (_partition >= 4) {
        _error = Error::UnsupportedPartition;
        return Volume::Error::Failed;
    }
    
    if (_mounted) {
        return Volume::Error::OK;
    }
    
    // Read the MBR
    char buf[512] __attribute__((aligned(4)));
    if (rawRead(buf, 0, 1) != Volume::Error::OK) {
        _error = Error::MBRReadError;
        return Volume::Error::Failed;
    }
    
    MBR* mbr = reinterpret_cast<MBR*>(buf);

    // Do validation checks
    if (mbr->signature[0] != 0x55 || mbr->signature[1] != 0xaa) {
        _error = Error::BadMBRSignature;
        return Volume::Error::Failed;
    }
        
    uint8_t partitionType = mbr->partitions[_partition].type;
    if (partitionType != 0x0b && partitionType != 0x0c) {
        _error = Error::OnlyFAT32LBASupported;
        return Volume::Error::Failed;
    }

    // Extract the needed values
    _firstBlock = bufToUInt32(mbr->partitions[_partition].lbaStart);
    _sizeInBlocks = bufToUInt32(mbr->partitions[_partition].lbaCount);
    
    // Read the Boot Block
    if (rawRead(buf, _firstBlock, 1) != Volume::Error::OK) {
        _error = Error::BPBReadError;
        return Volume::Error::Failed;
    }
    
    // Do validation checks
    BootBlock* bootBlock = reinterpret_cast<BootBlock*>(buf);
    if (bootBlock->signature[0] != 0x55 || bootBlock->signature[1] != 0xaa) {
        _error = Error::BadBPBSignature;
        return Volume::Error::Failed;
    }
            
    if (bufToUInt16(bootBlock->bytesPerBlock) != 512) {
        _error = Error::UnsupportedBlockSize;
        return Volume::Error::Failed;
    }
    
    if (bootBlock->numberOfFATCopies != 2) {
        _error = Error::UnsupportedFATCount;
        return Volume::Error::Failed;
    }
    
    if (bufToUInt16(bootBlock->rootDirEntries) != 0) {
        _error = Error::InvalidFAT32Volume;
        return Volume::Error::Failed;
    }
    
    if (bufToUInt16(bootBlock->blocksPerFATOld) != 0 || bufToUInt16(bootBlock->blocksPerFATOld) != 0) {
        _error = Error::InvalidFAT32Volume;
        return Volume::Error::Failed;
    }
    
    // Extract the needed values
    Block reservedBlocks = bufToUInt16(bootBlock->reservedBlocks);
    
    _blocksPerFAT = bufToUInt32(bootBlock->blocksPerFAT32);
    _blocksPerCluster = bootBlock->blocksPerCluster;
    _startFATBlock = _firstBlock + reservedBlocks;
    _startDataBlock = _startFATBlock + Block(_blocksPerFAT * 2);
    _rootDirectoryStartCluster = bufToUInt32(bootBlock->rootDirectoryStartCluster);
    
    _mounted = true;
    return Volume::Error::OK;
}

bool FAT32::readFATBlock(uint32_t block)
{
    if (!_fatBufferValid || block != _currentFATBufferAddr) {
        if (_fatBufferNeedsWriting) {
            if (!writeFATBlock()) {
                return false;
            }
        }
        
        if (rawRead(_fatBuffer, block, 1) != Volume::Error::OK) {
            _error = Error::FATReadError;
            return false;
        }
        _fatBufferValid = true;
        _currentFATBufferAddr = block;
    }
    return true;
}

bool FAT32::writeFATBlock()
{
    if (!_fatBufferValid || !_fatBufferNeedsWriting) {
        return true;
    }
                
    _fatBufferNeedsWriting = false;

    // Assume 2 FAT copies
    if (rawWrite(_fatBuffer, _currentFATBufferAddr, 1) != Volume::Error::OK) {
        _error = Error::FATWriteError;
        return false;
    }

    if (rawWrite(_fatBuffer, _currentFATBufferAddr + _blocksPerFAT, 1) != Volume::Error::OK) {
        _error = Error::FATWriteError;
        return false;
    }

    return true;
}

FAT32::FATEntryType FAT32::nextClusterFATEntry(Cluster cluster, Cluster& nextCluster)
{
    uint32_t fatBlockAddr = cluster.value() * 4 / 512 + _startFATBlock.value();
    uint32_t fatBlockOffset = cluster.value() * 4 % 512;
    
    if (!readFATBlock(fatBlockAddr)) {
        return FATEntryType::Error;
    }
    
    uint32_t entry = bufToUInt32(reinterpret_cast<uint8_t*>(_fatBuffer + fatBlockOffset));
    if (entry == 0) {
        return FATEntryType::Free;
    }
    if (entry >= 0x0ffffff8) {
        return FATEntryType::End;
    }
    nextCluster = entry & 0x0fffffff;
    return FATEntryType::Normal;
}

Cluster FAT32::allocateCluster(Cluster prev)
{
    Cluster dummyCluster;
    uint32_t lastCluster = _blocksPerFAT * 512 / sizeof(uint32_t) - 1;
    
    for (uint32_t currentCluster = 2; currentCluster <= lastCluster; ++currentCluster) {
        FATEntryType type = nextClusterFATEntry(currentCluster, dummyCluster);
        if (type == FATEntryType::Free) {
            uint32_t oldNext = 0x0ffffff8; // By default make this the last cluster in the chain
            uint32_t fatBlockAddr;
            uint32_t fatBlockOffset;
           
            if (prev.value() > 0) {
                // Insert this cluster into the FAT chain
                fatBlockAddr = prev.value() * 4 / 512 + _startFATBlock.value();
                fatBlockOffset = prev.value() * 4 % 512;
                
                if (!readFATBlock(fatBlockAddr)) {
                    return 0;
                }
                
                oldNext = bufToUInt32(reinterpret_cast<uint8_t*>(_fatBuffer + fatBlockOffset));
                uint32ToBuf(currentCluster, reinterpret_cast<uint8_t*>(_fatBuffer + fatBlockOffset));
                _fatBufferNeedsWriting = true;
            }
                
            fatBlockAddr = currentCluster * 4 / 512 + _startFATBlock.value();
            fatBlockOffset = currentCluster * 4 % 512;
                
            if (!readFATBlock(fatBlockAddr)) {
                return 0;
            }
            
            uint32ToBuf(oldNext, reinterpret_cast<uint8_t*>(_fatBuffer + fatBlockOffset));

            if (!writeFATBlock()) {
                return 0;
            }
            
            return currentCluster;
        }
    }
    
    // There is no Cluster 0, so check for this on return mean we've run out of disk space
    return 0;
}

bool FAT32::freeClusters(Cluster cluster)
{
    Cluster nextCluster;
    
    while (1) {
        FATEntryType type = nextClusterFATEntry(cluster, nextCluster);
        if (type == FATEntryType::Error) {
            return false;
        }
  
        // Free this cluster
        uint32_t fatBlockOffset = cluster.value() * 4 % 512;
        uint32ToBuf(0, reinterpret_cast<uint8_t*>(_fatBuffer + fatBlockOffset));
        _fatBufferNeedsWriting = true;

        if (type == FATEntryType::Normal) {
            cluster = nextCluster;
            continue;
        }
        
        if (!writeFATBlock()) {
            return false;
        }
        return type == FATEntryType::End;
    }
}

bool FAT32::find(FileInfo& fileInfo, const char* name)
{
    // Convert the incoming filename to 8.3 and then compare all 11 characters
    char nameToFind[12];
    convertTo8dot3(nameToFind, name);
    
    FAT32DirectoryIterator it = FAT32DirectoryIterator(this, "/");
    for ( ; it; it.next()) {
        char testName[12];
        convertTo8dot3(testName, it.name());
        if (memcmp(nameToFind, testName, 11) == 0) {
            memcpy(&fileInfo, &it.fileInfo(), sizeof(FileInfo));
            return true;
        }
    }
    
    // No match
    return false;
}
    
DirectoryIterator* FAT32::directoryIterator(const char* path)
{
    FAT32DirectoryIterator* it = new FAT32DirectoryIterator(this, path);
    return it;
}

const char* FAT32::errorDetail(Volume::Error error) const
{
    if (error != Volume::Error::PlatformSpecificError) {
        return Volume::errorDetail(error);
    }
    
    switch(static_cast<FAT32::Error>(error)) {
    case Error::UnsupportedType:        return "unsupported type";
    case Error::UnsupportedPartition:   return "unsupported partition";
    case Error::UnsupportedBlockSize:  return "unsupported block size";
    case Error::UnsupportedFATCount:    return "unsupported FAT count";
    case Error::BadMBRSignature:        return "bad MBR signature";
    case Error::BadBPBSignature:        return "bad BPB signature";
    case Error::MBRReadError:           return "MBR read error";
    case Error::BPBReadError:           return "BPB read error";
    case Error::FATReadError:           return "FAT read error";
    case Error::FATWriteError:          return "FAT write error";
    case Error::DirReadError:           return "dir read error";
    case Error::OnlyFAT32LBASupported:  return "only FAT32 LBA supported";
    case Error::InvalidFAT32Volume:     return "invalid FAT32 volume";
    case Error::WrongSizeRead:          return "wrong size read";
    case Error::WrongSizeWrite:         return "wrong size write";
    case Error::Incomplete:             return "incomplete";
    default:                            return "***";
    }
}

RawFile* FAT32::open(const char* name)
{
    FileInfo fileInfo;
    if (!find(fileInfo, name)) {
        return nullptr;
    }
    
    return new FAT32RawFile(this, fileInfo.baseCluster, fileInfo.size, fileInfo.directoryBlock, fileInfo.directoryBlockIndex);
}

Volume::Error FAT32::create(const char* name)
{
    // Find an empty directory entry
    FAT32DirectoryIterator it = FAT32DirectoryIterator(this, "/");
    
    while (1) {
        if (it.deleted() || !it) {
            // Create an initial cluster
            Cluster cluster = allocateCluster();
            it.createEntry(name, 0, cluster);
            return Volume::Error::OK;
        }
        it.rawNext(true);
    }
}

Volume::Error FAT32::remove(const char* name)
{
    // Convert the incoming filename to 8.3 and then compare all 11 characters
    char nameToFind[12];
    convertTo8dot3(nameToFind, name);
    
    FAT32DirectoryIterator it = FAT32DirectoryIterator(this, "/");
    for ( ; it; it.next()) {
        char testName[12];
        convertTo8dot3(testName, it.name());
        if (memcmp(nameToFind, testName, 11) == 0) {
            freeClusters(it.baseCluster());
            it.deleteEntry();
            return Volume::Error::OK;
        }
    }
    
    // No match
    return Volume::Error::FileNotFound;
}

bool FAT32::exists(const char* name)
{
    FileInfo dummy;
    return find(dummy, name);
}
