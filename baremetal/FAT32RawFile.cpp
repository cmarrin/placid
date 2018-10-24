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

#include "FAT32RawFile.h"

#include "Serial.h"

using namespace bare;

Volume::Error FAT32RawFile::read(char* buf, Block blockAddr, uint32_t blocks)
{
    Block offset = 0;
    Cluster physical;
    
    Volume::Error error = logicalToPhysicalBlock(_baseCluster, blockAddr, physical, offset);
    if (error != Volume::Error::OK) {
        return error;
    }

    Block baseBlock = _fat32->clusterToBlock(physical);
    _error = static_cast<Volume::Error>(_fat32->rawRead(buf, baseBlock + offset, blocks));
    return _error;
}

Volume::Error FAT32RawFile::write(const char* buf, Block blockAddr, uint32_t blocks)
{
    Block offset = 0;
    Cluster physical;
    
    Volume::Error error = logicalToPhysicalBlock(_baseCluster, blockAddr, physical, offset);
    if (error != Volume::Error::OK) {
        return error;
    }

    Block baseBlock = _fat32->clusterToBlock(physical);
    _error = static_cast<Volume::Error>(_fat32->rawWrite(buf, baseBlock + offset, blocks));
    return _error;
}

Volume::Error FAT32RawFile::logicalToPhysicalBlock(Cluster base, Block block, Cluster& physical, Block& offset)
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
    Cluster currentLogicalCluster = block.value / _fat32->blocksPerCluster();
    Block currentLogicalClusterBlock = block.value % _fat32->blocksPerCluster();
    
    if (_lastLogicalCluster.value != currentLogicalCluster.value) {
        if (currentLogicalCluster.value < _lastLogicalCluster.value) {
            _lastPhysicalCluster = _baseCluster;
            _lastLogicalCluster = 0;
        }
        
        while (_lastLogicalCluster.value++ < currentLogicalCluster.value) {
            FAT32::FATEntryType type = _fat32->nextClusterFATEntry(_lastPhysicalCluster, _lastPhysicalCluster);
            if (type == FAT32::FATEntryType::Normal) {
                continue;
            }
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

    physical = _lastPhysicalCluster;
    _lastLogicalCluster = currentLogicalCluster;
    offset = currentLogicalClusterBlock;
    return Volume::Error::OK;
}

