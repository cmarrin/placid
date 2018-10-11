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

#include "FS.h"

#include "SDCard.h"
#include "util.h"
#include "Serial.h"
#include "Timer.h"

using namespace bare;

FS::Error FS::mount(Device* device)
{
    if (!device) {
        return Error::UnsupportedDevice;
    }
    
    _device = device;

    return _device->mount();
}

bool FS::open(RawFile& file, const char* name, const char* mode)
{
    FileInfo fileInfo;
    if (!_device->find(fileInfo, name)) {
        return false;
    }
    
    file._baseBlock = fileInfo.baseBlock;
    file._size = fileInfo.size;
    file._device = _device;
    
    return true;
}

FS::Error RawFile::read(char* buf, uint32_t blockAddr, uint32_t blocks)
{
    _error = static_cast<FS::Error>(_device->read(buf, _baseBlock, blockAddr, blocks));
    return _error;
}

FS::Error RawFile::write(const char* buf, uint32_t blockAddr, uint32_t blocks)
{
    _error = static_cast<FS::Error>(_device->write(buf, _baseBlock, blockAddr, blocks));
    return _error;
}
