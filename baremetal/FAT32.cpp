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

struct FATDirEntry
{
    char name[11];
    uint8_t attr;
    uint8_t reserved1;
    uint8_t creationTime;
    uint8_t createTime[2];
    uint8_t createDate[2];
    uint8_t accessedDate[2];
    uint8_t firstClusterHi[2];
    uint8_t modificationTime[2];
    uint8_t modificationDate[2];
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

Volume::Error FAT32::rawRead(char* buf, Block block, uint32_t blocks)
{
    int32_t result = _rawIO->read(buf, block, blocks);
    _error = (result == static_cast<int32_t>(blocks)) ? Error::OK : Error::WrongSizeRead;
    return (_error == Error::OK) ? Volume::Error::OK : Volume::Error::Failed;
}

Volume::Error FAT32::rawWrite(const char* buf, Block block, uint32_t blocks)
{
    int32_t result = _rawIO->write(buf, block, blocks);
    _error = (result == static_cast<int32_t>(blocks)) ? Error::OK : Error::WrongSizeWrite;
    return (_error == Error::OK) ? Volume::Error::OK : Volume::Error::Failed;
}

class FAT32DirectoryIterator : public DirectoryIterator
{
public:
    static constexpr uint32_t EntriesPerBlock = 512 / 32;

    FAT32DirectoryIterator(FAT32* fs, const char* path)
        : _fs(fs)
    {
        // FIXME: Ignore path for now
        _startBlock = _fs->rootDirectoryStartBlock();
        next();
    }
    
    virtual DirectoryIterator& next() override
    {
        // FIXME: Handle more than one cluster
        while (1) {
            if (_entryIndex < 0 || ++_entryIndex >= static_cast<int32_t>(EntriesPerBlock)) {
                if (++_blockIndex < static_cast<int32_t>(_fs->blocksPerCluster())) {
                    if (!getBlock()) {
                        // Could not get a block
                        _valid = false;
                        return *this;
                    }
                    _entryIndex = 0;
                } else {
                    // Go to the next cluster
                    _startBlock = _fs->nextBlockFATEntry(_startBlock.value + _blockIndex - 1);
                    _blockIndex = -1;
                    _entryIndex = -1;
                    continue;
                }
            }
            FileInfoResult result = getFileInfo();
            if (result == FileInfoResult::OK) {
                _valid = true;
            } else if (result == FileInfoResult::Skip) {
                continue;
            } else {
                _valid = false;
            }
            
            return *this;
        }
    }
    
    virtual const char* name() const override { return _valid ? _fileInfo.name : ""; }
    virtual uint32_t size() const override { return _valid ? _fileInfo.size : 0; }
    Block baseBlock() const { return _valid ? _fileInfo.baseBlock : 0; }
    virtual operator bool() const override { return _valid; }
    
private:
    bool getBlock()
    {
        if (_fs->rawRead(_buf, _startBlock + _blockIndex, 1) != Volume::Error::OK) {
            return false;
        }
        return true;
    }
    
    enum class FileInfoResult { OK, Skip, End };
    FileInfoResult getFileInfo()
    {
        FATDirEntry* entry = reinterpret_cast<FATDirEntry*>(_buf) + _entryIndex;
        if (entry->attr & 0x1f || static_cast<uint8_t>(entry->name[0]) == 0xe5 || entry->name[0] == 0x05) {
            // Regular files have lower 4 bits clear.
            // Bit 0x10 is the subdir bit. Skip them for now
            // If the first char of the name is 0x05 or 0xe5 it means the file has been deleted so ignore it
            return FileInfoResult::Skip;
        }
        
        
        if (entry->name[0] == '\0') {
            // End of directory
            return FileInfoResult::End;
        }

        // Format the name
        uint32_t j = 0;
        for (uint32_t i = 0; i < 8; ++i) {
            if (entry->name[i] == ' ') {
                break;
            }
            _fileInfo.name[j++] = entry->name[i];
        }
        
        _fileInfo.name[j++] = '.';
        
        for (uint32_t i = 8; i < 11; ++i) {
            if (entry->name[i] == ' ') {
                break;
            }
            _fileInfo.name[j++] = entry->name[i];
        }
        
        _fileInfo.name[j] = '\0';
        _fileInfo.size = bufToUInt32(entry->size);
        Cluster baseCluster = (static_cast<uint32_t>(bufToUInt16(entry->firstClusterHi)) << 16) + 
                                static_cast<uint32_t>(bufToUInt16(entry->firstClusterLo));
        _fileInfo.baseBlock = _fs->clusterToBlock(baseCluster);
        return FileInfoResult::OK;
    }
    
    FAT32* _fs;
    FAT32::FileInfo _fileInfo;
    Block _startBlock = 0;
    int32_t _blockIndex = -1;
    int32_t _entryIndex = -1;
    char _buf[512];
    bool _valid = true;
};

Volume::Error FAT32::mount()
{
    _error = Error::OK;
    
    if (_partition >= 4) {
        _error = Error::UnsupportedPartition;
        return Volume::Error::Failed;
    }
    
    if (_mounted) {
        return Volume::Error::OK;
    }
    
    // Read the MBR
    char buf[512];
    if (_rawIO->read(buf, 0, 1) != 1) {
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
    
    // Exrtract the needed values
    _firstBlock = bufToUInt32(mbr->partitions[_partition].lbaStart);
    _sizeInBlocks = bufToUInt32(mbr->partitions[_partition].lbaCount);
    
    // Read the Boot Block
    if (_rawIO->read(buf, _firstBlock, 1) != 1) {
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
    
    // Exrtract the needed values
    uint16_t reservedBlocks = bufToUInt16(bootBlock->reservedBlocks);
    
    _blocksPerFAT = bufToUInt32(bootBlock->blocksPerFAT32);
    _blocksPerCluster = bootBlock->blocksPerCluster;
    _startFATBlock = _firstBlock + reservedBlocks;
    _startDataBlock = _startFATBlock + (_blocksPerFAT * 2);
    _rootDirectoryStartBlock = clusterToBlock(bufToUInt32(bootBlock->rootDirectoryStartCluster));
    
    _mounted = true;
    return Volume::Error::OK;
}

uint32_t FAT32::nextBlockFATEntry(Block block)
{
    uint32_t fatBlockAddr = block.value * 4 / 512 + _startFATBlock.value;
    uint32_t fatBlockOffset = block.value * 4 % 512;
    
    if (!_fatBufferValid || fatBlockAddr != _currentFATBufferAddr) {
        if (_rawIO->read(_fatBuffer, fatBlockAddr, 1) != 1) {
            _error = Error::FATReadError;
            return 0;
        }
        _fatBufferValid = true;
        _currentFATBufferAddr = fatBlockAddr;
    }
    
    return bufToUInt32(reinterpret_cast<uint8_t*>(_fatBuffer + fatBlockOffset));    
}

bool FAT32::find(FileInfo& fileInfo, const char* name)
{
    // Convert the incoming filename to 8.3 and then compare all 11 characters
    char nameToFind[12];
    convertTo8dot3(nameToFind, name);
    
    FAT32DirectoryIterator it = FAT32DirectoryIterator(this, "/");;
    for ( ; it; it.next()) {
        char testName[12];
        convertTo8dot3(testName, it.name());
        if (memcmp(nameToFind, testName, 11) == 0) {
            memcpy(fileInfo.name, it.name(), 11);
            fileInfo.name[11] = '\0';
            fileInfo.size = it.size();
            fileInfo.baseBlock = it.baseBlock();
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
    case Error::OnlyFAT32LBASupported:  return "only FAT32 LBA supported";
    case Error::InvalidFAT32Volume:     return "invalid FAT32 volume";
    case Error::WrongSizeRead:          return "wrong size read";
    case Error::WrongSizeWrite:         return "wrong size write";
    case Error::NotImplemented:         return "not implemented";
    case Error::Incomplete:             return "incomplete";
    default:                            return "***";
    }
}

class FAT32RawFile: public RawFile
{
public:
    FAT32RawFile(Block baseBlock, uint32_t size, FAT32* fat32)
        : _baseBlock(baseBlock)
        , _size(size)
        , _fat32(fat32)
    { }
    
    virtual Volume::Error read(char* buf, Block blockAddr, uint32_t blocks) override;
    virtual Volume::Error write(const char* buf, Block blockAddr, uint32_t blocks) override;
    virtual uint32_t size() const override { return _size; }

private:
    Block _baseBlock;
    uint32_t _size = 0;
    FAT32* _fat32;
};

RawFile* FAT32::open(const char* name)
{
    FileInfo fileInfo;
    if (!find(fileInfo, name)) {
        return nullptr;
    }
    
    return new FAT32RawFile(fileInfo.baseBlock, fileInfo.size, this);
}

Volume::Error FAT32RawFile::read(char* buf, Block blockAddr, uint32_t blocks)
{
    _error = static_cast<Volume::Error>(_fat32->rawRead(buf, _baseBlock + blockAddr, blocks));
    return _error;
}

Volume::Error FAT32RawFile::write(const char* buf, Block blockAddr, uint32_t blocks)
{
    _error = static_cast<Volume::Error>(_fat32->rawWrite(buf, _baseBlock + blockAddr, blocks));
    return _error;
}








