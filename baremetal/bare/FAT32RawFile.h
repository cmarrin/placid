/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "FAT32.h"

namespace bare {

    class FAT32RawFile: public RawFile
    {
    public:
        FAT32RawFile(FAT32* fat32, Cluster baseCluster, uint32_t size, Block dirBlock = 0, uint32_t dirIndex = 0)
            : _fat32(fat32)
            , _baseCluster(baseCluster)
            , _lastPhysicalCluster(baseCluster)
            , _directoryBlock(dirBlock)
            , _directoryBlockIndex(dirIndex)
        {
            _size = size;
        }
        
        virtual ~FAT32RawFile() { }
        
        virtual Volume::Error read(char* buf, Block blockAddr, uint32_t blocks) override;
        virtual Volume::Error write(const char* buf, Block blockAddr, uint32_t blocks) override;
        virtual Volume::Error rename(const char* to) override;
        virtual Volume::Error insertCluster() override;
        virtual Volume::Error updateSize() override;

        Volume::Error logicalToPhysicalBlock(Block logical, Block& physical);
        
    private:
        FAT32* _fat32;
        Cluster _baseCluster;
        
        Cluster _lastLogicalCluster = 0;
        Cluster _lastPhysicalCluster = 0;
        
        Block _directoryBlock;
        uint32_t _directoryBlockIndex;
    };

}

