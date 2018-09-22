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

namespace placid {

const uint32_t FilenameLength = 32;

class DirectoryEntry {    
public:
    DirectoryEntry() { }

    static const char* name(const DirectoryEntry& dir) { return dir._name; }
    static uint32_t size(const DirectoryEntry& dir) { return dir._size; }
    static bool valid(const DirectoryEntry& dir) { return dir._valid; }
    
    static bool next();
    
private:
    bool _valid = false;
    char _name[FilenameLength];
    uint32_t _size = 0;
};

class File {
public:
    enum class SeekWhence { Set, Cur, End };
      
    static int32_t read(const File&, char* buf, uint32_t size);
    static int32_t write(File&, const char* buf, uint32_t size);
    
    static int32_t read(const File& file)
    {
        char buf;
        return (file.read(file, &buf, 1) <= 0) ? -1 : static_cast<int>(buf);
    }
    
    static int32_t write(File& file, uint8_t c)
    {
        return (file.write(file, reinterpret_cast<const char*>(&c), 1) <= 0) ? -1 : c;
    }

    static bool seek(File&, int32_t offset, SeekWhence whence);
    static int32_t tell(const File&);
    static bool eof(const File&);
    
    static bool valid(const File& file) { return file._error == 0; }
    static uint32_t error(const File& file) { return file._error; }

protected:
    uint32_t _error = 0;
    uint64_t _baseAddress = 0;
    uint64_t _cur = 0;
};

class SDFS {
public:
    SDFS() { }
    
    static bool mount(SDFS&, uint8_t device, uint8_t partition);
    static bool mounted(const SDFS& fs) { return fs._mounted; }
    static void unmount(SDFS& fs);
    static bool format(SDFS& fs);
    
    static bool directory(DirectoryEntry&);

    static bool open(const SDFS&, File&, const char* name, const char* mode);
    static bool remove(SDFS&, const char* name);
    static bool rename(SDFS&, const char* src, const char* dst);
    
    static uint32_t sizeInBlocks(const SDFS& fs) { return fs._sizeInBlocks; }
    static uint32_t usedInBlocks(const SDFS& fs) { return fs._usedInBlocks; }
    
    private:
    bool _mounted = false;
    uint32_t _sizeInBlocks = 0;
    uint32_t _usedInBlocks = 0;
};

}

