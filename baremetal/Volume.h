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

#include <stdint.h>

namespace bare {

const uint32_t FilenameLength = 32;
const uint32_t BlockSize = 512;

class DirectoryIterator;
class RawFile;

// Strong typed block
struct Block
{
    Block() { }
    Block(uint32_t v) : value(v) { }
    Block operator +(Block other) { return value + other.value; }
    uint32_t value = 0;
};

class Volume {
    friend class RawFile;
    
public:
    enum class Error {
        OK = 0,
        Failed,
        UnsupportedDevice, 
        FileNotFound,
    };
    
    struct RawIO
    {
        virtual int32_t read(char* buf, Block blockAddr, uint32_t blocks) = 0;
        virtual int32_t write(const char* buf, Block blockAddr, uint32_t blocks) = 0;
    };
    
    struct FileInfo {
        char name[FilenameLength]; // Passed in name converted to 8.3
        uint32_t size = 0;
        Block baseBlock = 0;
    };

    virtual uint32_t sizeInBlocks() const = 0;
    virtual Error mount() = 0;
    virtual Error read(char* buf, Block baseBlock, Block relativeBlock, uint32_t blocks) = 0;    
    virtual Error write(const char* buf, Block baseBlock, Block relativeBlock, uint32_t blocks) = 0;    
    virtual bool find(FileInfo&, const char* name) = 0;
    virtual const char* errorDetail() const = 0;
    virtual DirectoryIterator* directoryIterator(const char* path) = 0;

    Volume() { }
    
    bool open(RawFile&, const char* name);
};

class RawFile {
    friend class Volume;
    
public:
    RawFile() { }
    
    Volume::Error read(char* buf, Block blockAddr, uint32_t blocks);    
    Volume::Error write(const char* buf, Block blockAddr, uint32_t blocks);    

    bool valid() const { return _error == Volume::Error::OK; }
    uint32_t size() const { return _size; }
    Volume::Error error() const { return _error; }

private:
    Volume::Error _error = Volume::Error::OK;
    uint32_t _size = 0;
    Block _baseBlock;
    Volume* _volume = nullptr;
};

class DirectoryIterator
{
public:
    virtual DirectoryIterator& next() = 0;
    virtual const char* name() const = 0;
    virtual uint32_t size() const = 0;
    virtual operator bool() const = 0;
};

}
