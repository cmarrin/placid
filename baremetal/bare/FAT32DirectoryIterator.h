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
        virtual ~FAT32DirectoryIterator()
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
