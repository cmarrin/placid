/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/FAT32RawFile.h"

#include "bare/FAT32DirectoryIterator.h"
#include "bare/Serial.h"

using namespace bare;

Volume::Error FAT32RawFile::read(char* buf, Block logicalBlock, uint32_t blocks)
{
    Block physicalBlock;
    _error = logicalToPhysicalBlock(logicalBlock, physicalBlock);
    if (_error != Volume::Error::OK) {
        return _error;
    }

    return static_cast<Volume::Error>(_fat32->rawRead(buf, physicalBlock, blocks));
}

Volume::Error FAT32RawFile::write(const char* buf, Block logicalBlock, uint32_t blocks)
{
    Block physicalBlock;
    Volume::Error error = logicalToPhysicalBlock(logicalBlock, physicalBlock);
    if (error != Volume::Error::OK) {
        return error;
    }

    return static_cast<Volume::Error>(_fat32->rawWrite(buf, physicalBlock, blocks));
}

Volume::Error FAT32RawFile::rename(const char* to)
{
    if (_directoryBlock == 0) {
        Serial::printf("*** FAT32RawFile::rename invalid directoryBlock\n");
        return Volume::Error::InternalError;
    }
    
    // First make sure to does not exist
    if (_fat32->exists(to)) {
        _error = Volume::Error::FileExists;
        return _error;
    }
    
    char buf[BlockSize];
    Volume::Error error = _fat32->rawRead(buf, _directoryBlock, 1);
    if (error != Volume::Error::OK) {
        return error;
    }
    
    FATDirEntry* entry = reinterpret_cast<FATDirEntry*>(buf) + _directoryBlockIndex;
    FAT32::convertTo8dot3(entry->name, to);
    
    return _fat32->rawWrite(buf, _directoryBlock, 1);
}

Volume::Error FAT32RawFile::insertCluster()
{
    if (_lastPhysicalCluster == 0) {
        return Volume::Error::InternalError;
    }
    
    _fat32->allocateCluster(_lastPhysicalCluster);
    return error();
}

Volume::Error FAT32RawFile::updateSize()
{
    if (_directoryBlock == 0) {
        Serial::printf("*** FAT32RawFile::updateSize invalid directoryBlock\n");
        return Volume::Error::InternalError;
    }
    
    char buf[BlockSize];
    Volume::Error error = _fat32->rawRead(buf, _directoryBlock, 1);
    if (error != Volume::Error::OK) {
        return error;
    }
    
    FATDirEntry* entry = reinterpret_cast<FATDirEntry*>(buf) + _directoryBlockIndex;
    FAT32::uint32ToBuf(_size, entry->size);
    
    return _fat32->rawWrite(buf, _directoryBlock, 1);
}

Volume::Error FAT32RawFile::logicalToPhysicalBlock(Block logicalBlock, Block& physicalBlock)
{
    // 4 cases:
    //
    //      1) Cluster number of block matches last cluster - use same physical cluster
    //      2) Cluster number of block is one more than the last - get the next FAT entry
    //          a) If the right FAT sector is loaded and the next entry is still in this sector, get it
    //          b) Otherwise, load the right FAT sector and get the entry
    //      3) Cluster number is higher that the last but not consecutive - search the FAT from this point
    //      4) Cluster number is lower or we don't have a last entry - search from the start of the FAT
    //
    if (_fat32->blocksPerCluster() == 0) {
        return static_cast<Volume::Error>(FAT32::Error::UnsupportedBlockSize);
    }
    
    Cluster currentLogicalCluster = (logicalBlock / Block(_fat32->blocksPerCluster())).value();
    Block currentLogicalClusterBlock = logicalBlock % Block(_fat32->blocksPerCluster());
    
    if (_lastLogicalCluster != currentLogicalCluster) {
        if (currentLogicalCluster < _lastLogicalCluster) {
            _lastPhysicalCluster = _baseCluster;
            _lastLogicalCluster = 0;
        }
        
        while (_lastLogicalCluster++ < currentLogicalCluster) {
            FAT32::FATEntryType type = _fat32->nextClusterFATEntry(_lastPhysicalCluster, _lastPhysicalCluster);
            if (type == FAT32::FATEntryType::Normal) {
                continue;
            }
            
            // The only time _lastPhysicalCluster gets updated is if the return type is Normal.
            // If we have an error decrement _lastLogicalCluster to keep it consistent with
            // the existing value of _lastPhysicalCluster.
            _lastLogicalCluster--;
            if (type == FAT32::FATEntryType::End) {
                return Volume::Error::EndOfFile;
            }
            if (type == FAT32::FATEntryType::Error) {
                Serial::printf("**** Error: failed to read FAT block\n");
                return Volume::Error::Failed;
            }
            Serial::printf("**** Error: Disk inconsistency - next FAT entry is free\n");
            return Volume::Error::InternalError;
        }
    }

    physicalBlock = _fat32->clusterToBlock(_lastPhysicalCluster) + currentLogicalClusterBlock;
    _lastLogicalCluster = currentLogicalCluster;
    return Volume::Error::OK;
}

