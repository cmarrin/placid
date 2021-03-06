/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/FAT32DirectoryIterator.h"

using namespace bare;

FAT32DirectoryIterator::FAT32DirectoryIterator(FAT32* fs, const char* path)
    : _fs(fs)
{
    // FIXME: Ignore path for now
    _file = new FAT32RawFile(_fs, _fs->rootDirectoryStartCluster(), 0);
    next();
}

DirectoryIterator& FAT32DirectoryIterator::next()
{
    while (1) {
        rawNext();
        if (!deleted() && !subdir()) {
            return *this;
        }
    }
}

void FAT32DirectoryIterator::rawNext(bool extend)
{
    _subdir = false;
    _deleted = false;
    
    while (1) {
        if (_entryIndex < 0 || ++_entryIndex >= static_cast<int32_t>(EntriesPerBlock)) {
            // get the next block
            if (_file->read(_buf, ++_blockIndex, 1) != Volume::Error::OK) {
                if (_file->error() != Volume::Error::EndOfFile || !extend) {                
                    _valid = false;
                    return;
                }
                
                // Extend the directory
                _file->insertCluster();
                
                // Clear all the directory entries
                memset(_buf, 0, sizeof(_buf));

                // Write out the new directory block
                if (_file->write(_buf, _blockIndex, 1) != Volume::Error::OK) {
                    _valid = false;
                    return;
                }
            }
            
            _entryIndex = 0;
        }
        
        FileInfoResult result = getFileInfo();
        if (result == FileInfoResult::OK) {
            _valid = true;
        } else if (result == FileInfoResult::End) {
            _valid = false;
        } else {
            // All other results are skipped
            continue;
        }
        
        return;
    }
}

FAT32DirectoryIterator::FileInfoResult FAT32DirectoryIterator::getFileInfo()
{
    FATDirEntry* entry = reinterpret_cast<FATDirEntry*>(_buf) + _entryIndex;
    if (entry->attr & 0x0f) {
        // Regular files have lower 4 bits clear. Skip other types
        return FileInfoResult::Skip;
    }
    
    if (entry->attr & 0x10) {
        // Bit 0x10 is the subdir bit.
        return FileInfoResult::SubDir;
    }

    if (static_cast<uint8_t>(entry->name[0]) == 0xe5 || entry->name[0] == 0x05) {
        // If the first char of the name is 0x05 or 0xe5 it means the file has been deleted
        return FileInfoResult::Deleted;
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
    _fileInfo.size = FAT32::bufToUInt32(entry->size);
    _fileInfo.baseCluster = (static_cast<uint32_t>(FAT32::bufToUInt16(entry->firstClusterHi)) << 16) + 
                            static_cast<uint32_t>(FAT32::bufToUInt16(entry->firstClusterLo));

    Block physicalBlock;
    if (_file->logicalToPhysicalBlock(_blockIndex, physicalBlock) != Volume::Error::OK) {
        return FileInfoResult::Skip;
    }
    _fileInfo.directoryBlock = physicalBlock;
    _fileInfo.directoryBlockIndex = _entryIndex;
    
    return FileInfoResult::OK;
}

bool FAT32DirectoryIterator::createEntry(const char* name, uint32_t size, Cluster baseCluster)
{
    FATDirEntry* entry = reinterpret_cast<FATDirEntry*>(_buf) + _entryIndex;
    
    memset(entry, 0, sizeof(FATDirEntry));
    FAT32::convertTo8dot3(entry->name, name);
    FAT32::uint32ToBuf(size, entry->size);
    FAT32::uint16ToBuf(baseCluster.value() >> 16, entry->firstClusterHi);
    FAT32::uint16ToBuf(baseCluster.value(), entry->firstClusterLo);

    return _file->write(_buf, _blockIndex, 1) == Volume::Error::OK;
}

bool FAT32DirectoryIterator::deleteEntry()
{
    FATDirEntry* entry = reinterpret_cast<FATDirEntry*>(_buf) + _entryIndex;
    entry->name[0] = 0xe5;
    return _file->write(_buf, _blockIndex, 1) == Volume::Error::OK;
}
