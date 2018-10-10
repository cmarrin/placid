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

class File;

class FS {
    friend class File;
    
public:
    enum class Error {
        OK = 0,
        Failed,
        UnsupportedDevice, 
        SDCardInitFailed,
    };
    
    struct RawIO
    {
        virtual int32_t read(char* buf, uint32_t blockAddr, uint32_t blocks) = 0;
        virtual int32_t write(const char* buf, uint32_t blockAddr, uint32_t blocks) = 0;
    };
    
    struct FileInfo {
        char name[FilenameLength]; // Passed in name converted to 8.3
        uint32_t size = 0;
        uint32_t baseBlock = 0;
    };

    struct Device
    {
        virtual uint32_t sizeInBlocks() const = 0;
        virtual FS::Error mount() = 0;
        virtual FS::Error read(char* buf, uint32_t baseBlock, uint32_t relativeBlock, uint32_t blocks) = 0;    
        virtual FS::Error write(const char* buf, uint32_t baseBlock, uint32_t relativeBlock, uint32_t blocks) = 0;    
        virtual bool find(FS::FileInfo&, const char* name) = 0;
        virtual const char* errorDetail() const = 0;
    };

    FS() { }
    
    Error mount(Device*);
    bool open(File&, const char* name, const char* mode);
    
    const char* errorDetail() { return _device->errorDetail(); }

    uint32_t sizeInBlocks() { return _device ? _device->sizeInBlocks() : 0; }
    
private:    
        
    Device* _device;
};

class File {
    friend class FS;
    
public:      
    FS::Error read(char* buf, uint32_t blockAddr, uint32_t blocks);    
    FS::Error write(const char* buf, uint32_t blockAddr, uint32_t blocks);    

    bool valid() { return _error == FS::Error::OK; }
    uint32_t size() { return _size; }
    FS::Error error() { return _error; }

private:
    FS::Error _error = FS::Error::OK;
    uint32_t _size = 0;
    uint32_t _baseBlock;
    FS::Device* _device = nullptr;
};

}

