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

