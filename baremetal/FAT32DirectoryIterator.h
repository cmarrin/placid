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
#include "FAT32RawFile.h"

namespace bare {

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

    class FAT32DirectoryIterator : public DirectoryIterator
    {
        friend class FAT32;
    
    public:
        static constexpr uint32_t EntriesPerBlock = 512 / 32;

        FAT32DirectoryIterator(FAT32* fs, const char* path);
        ~FAT32DirectoryIterator()
        {
            if (_file) {
                delete _file;
            }
        }
        
        virtual DirectoryIterator& next() override;
        
        virtual const char* name() const override { return _valid ? _fileInfo.name : ""; }
        virtual uint32_t size() const override { return _valid ? _fileInfo.size : 0; }
        Cluster baseCluster() const { return _valid ? _fileInfo.baseCluster : 0; }
        virtual operator bool() const override { return _valid; }
        
        bool subdir() const { return _subdir; }
        bool deleted() const { return _deleted; }
        
        const FAT32::FileInfo& fileInfo() const { return _fileInfo; }
        
    private:
        enum class FileInfoResult { OK, SubDir, Deleted, Skip, End };
        
        FileInfoResult getFileInfo();
        
        // If extend is true, append block when hit the end of the directory
        void rawNext(bool extend = false);

        bool createEntry(const char* name, uint32_t size, Cluster baseCluster);
        bool deleteEntry();
                
        FAT32* _fs;
        FAT32::FileInfo _fileInfo;
        FAT32RawFile* _file = nullptr;
        int32_t _blockIndex = -1;
        int32_t _entryIndex = -1;
        char _buf[512] __attribute__((aligned(4)));
        bool _valid = true;
        bool _subdir = false;
        bool _deleted = false;
    };

}
