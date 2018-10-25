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
#include "SDCard.h"

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
      
        int32_t read(char* buf, uint32_t size);
        int32_t write(const char* buf, uint32_t size);
    
        bool seek(int32_t offset, SeekWhence);
        int32_t tell() const { return _offset; }
        bool eof() const { return _offset >= _rawFile->size(); }
        
        bare::Volume::Error flush();
    
        bool valid() const { return _error == bare::Volume::Error::OK; }
        bare::Volume::Error error() const { return _error; }

    private:
        bool prepareBuffer(uint32_t offset);
        int32_t io(char* buf, uint32_t size, bool write);

        uint32_t _offset = 0;
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

