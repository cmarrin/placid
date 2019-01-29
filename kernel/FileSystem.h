/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare/FAT32.h"
#include "bare/SDCard.h"

namespace placid {

    class File;

    // FileSystem
    //
    // This is essentially a wrapper around bare::FS to give that file system
    // more capabilities. bare::FS is really just a single device, so maybe 
    // we'll rename it at some point. FileSystem should eventually be able to
    // handle multiple bare::FS devices at different mount points, presenting
    // a single unified file system. But for now we just support a single
    // FS instance for the built-in FAT32 SD card.
    class FileSystem
    {
    public:
        FileSystem();
        
        // Modes are slightly different than the C standard. Read and Append are
        // the same, but Write will not overwrite an existing file. Attempting to 
        // open an existing file in OpenMode::Write will fail. This matches the
        // behavior of the 'x' mode in the 2011 C standard. OpenOption::Update
        // is the same as the '+' modifier in the C standard. There is no concept
        // of text vs binary. All files are accessed in raw binary format. Also
        // OpenMode::Append ignores seek() unless OpenOption::Update is present. 
        // In that case the seek() is accepted but is only used for read. The
        // next write operation resets the file position to the end of the file 
        // and writes there. This matches the C standard
        
        enum class OpenMode { Read, Write, Append };
        enum class OpenOption { None, Update };
         
        bare::DirectoryIterator* directoryIterator(const char* path);
        File* open(const char* name, OpenMode = OpenMode::Read, OpenOption = OpenOption::None);
        
        bare::Volume::Error create(const char* name);
        bare::Volume::Error remove(const char* name);

        const char* errorDetail(bare::Volume::Error error) const { return _fatFS.errorDetail(error); }
        bare::Volume::Error error() const { return _fatFS.error(); }
        
        static FileSystem* sharedFileSystem();
        
    private:
        bare::SDCard _sdCard;
        bare::FAT32 _fatFS;

        static FileSystem* _sharedFileSystem;
    };
    
    class File
    {
        friend class FileSystem;
        
    public:
        enum class SeekWhence { Set, Cur, End };
        
        File() { }
        ~File() { close(); }
        
        bare::Volume::Error close() { return flush(); }
      
        size_t read(char* buf, size_t size);
        size_t write(const char* buf, size_t size);
        
        uint32_t size() const { return _rawFile->size(); }
        
        bare::Volume::Error rename(const char* to) { return _rawFile->rename(to); }
    
        bool seek(off_t offset, SeekWhence);
        off_t tell() const { return _offset; }
        bool eof() const { return _offset >= static_cast<off_t>(_rawFile->size()); }
        
        bare::Volume::Error flush();
    
        bool valid() const { return _error == bare::Volume::Error::OK; }
        bare::Volume::Error error() const { return _error; }

    private:
        bool prepareBuffer(off_t offset);
        size_t io(char* buf, size_t size, bool write);

        off_t _offset = 0;
        bare::Volume::Error _error = bare::Volume::Error::OK;
        bare::RawFile* _rawFile;
        bool _bufferValid = false;
        bool _bufferNeedsWriting = false;
        bool _canWrite;
        bool _canRead;
        bool _appendOnly;
        bool _needsSizeUpate = false;
        uint32_t _bufferAddr = 0; // Block addr of the contents of the buffer, if any

        // RawFile has a requirement for 4 byte alignment
        char _buffer[bare::BlockSize] __attribute__((aligned(4)));
    };

}

