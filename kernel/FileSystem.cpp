/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "FileSystem.h"

#include "bare/Serial.h"
#include <sys/types.h>

using namespace placid;

FileSystem* FileSystem::_sharedFileSystem = nullptr;

FileSystem* FileSystem::sharedFileSystem()
{
    if (!_sharedFileSystem) {
        _sharedFileSystem = new FileSystem();
    }
    return _sharedFileSystem;
}

FileSystem::FileSystem()
    : _fatFS(&_sdCard, 0)
{
    // FIXME: For now we just create a bare::FS for the FAT32 filesystem 
    // partition 0 of the SD card
    bare::Volume::Error e = _fatFS.mount();
    if (e != bare::Volume::Error::OK) {
        return;
    }
}

bare::DirectoryIterator* FileSystem::directoryIterator(const char* path)
{
    return _fatFS.directoryIterator(path);
}

File* FileSystem::open(const char* name, OpenMode mode, OpenOption option)
{
    if (error() != bare::Volume::Error::OK) {
        return nullptr;
    }
    
    File* fp = new File;
    fp->_error = bare::Volume::Error::OK;
    fp->_rawFile = _fatFS.open(name);
    if (!fp->_rawFile) {
        if (mode == OpenMode::Write) {
            // File does not exist, create it
            fp->_error = create(name);
            if (fp->_error != bare::Volume::Error::OK) {
                return fp;
            }
            
            fp->_rawFile = _fatFS.open(name);
            if (!fp->_rawFile) {
                fp->_error = bare::Volume::Error::InternalError;
                return fp;
            }
        } else {
            fp->_error = bare::Volume::Error::FileNotFound;
            return fp;
        }
    } else if (mode == OpenMode::Write) {
        fp->_error = bare::Volume::Error::FileExists;
        return fp;
    }

    fp->_canRead = mode == OpenMode::Read;
    fp->_canWrite = mode != OpenMode::Read;
    fp->_appendOnly = mode == OpenMode::Append;
    if (option == OpenOption::Update) {
        fp->_canWrite = true;
    }
    
    return fp;
}

bare::Volume::Error FileSystem::create(const char* name)
{
    return _fatFS.create(name);
}

bare::Volume::Error FileSystem::remove(const char* name)
{
    return _fatFS.remove(name);
}

bool File::prepareBuffer(off_t offset)
{
    if (offset / bare::BlockSize == _bufferAddr) {
        return true;
    }

    // Write out any needed data
    if (_bufferNeedsWriting) {
    _error = _rawFile->write(_buffer, _bufferAddr, 1);
    if (_error != bare::Volume::Error::OK) {
            return false;
        }
        _bufferNeedsWriting = false;
    }
    _bufferValid = false;
    return true;
}

size_t File::io(char* buf, size_t size, bool write)
{
    if (!prepareBuffer(_offset)) {
        return 0;
    }
    
    size_t sizeRemaining = size;
    uint32_t bufferAddr = static_cast<uint32_t>(_offset / bare::BlockSize);
    uint32_t bufferOffset = _offset % bare::BlockSize;
    
    if (write && !_bufferValid) {
        // We need to preload the buffer to fill the parts we're not
        // going to change
        _error = _rawFile->read(_buffer, bufferAddr, 1);
        if (_error != bare::Volume::Error::OK) {
            return 0;
        }
        
        _bufferValid = true;
        _bufferAddr = bufferAddr++;
    }

    while (1) {
        if (!_bufferValid) {
            bare::Volume::Error error = write ? 
                _rawFile->write(_buffer, bufferAddr, 1) :
                _rawFile->read(_buffer, bufferAddr, 1);
            
            if (error != bare::Volume::Error::OK) {
                return 0;
            }
            
            _bufferValid = true;
            _bufferAddr = bufferAddr++;
            if (write) {
                _bufferNeedsWriting = false;
            }
        }
        
        size_t amountInBuffer = bare::BlockSize - bufferOffset;
        size_t amountToCopy = (sizeRemaining <= amountInBuffer) ? sizeRemaining : amountInBuffer;
        
        if (write) {
            bare::memcpy(_buffer + bufferOffset, buf, amountToCopy);
            _bufferNeedsWriting = true;
        } else {
            bare::memcpy(buf, _buffer + bufferOffset, amountToCopy);
        }
        
        _offset += amountToCopy;
        buf += amountToCopy;
        bufferOffset += amountToCopy;
        sizeRemaining -= amountToCopy;
        
        if (bufferOffset >= bare::BlockSize) {
            _bufferValid = false;
            bufferOffset = 0;
        }
        
        if (sizeRemaining == 0) {
            return size;
        }
    }
}

size_t File::read(char* buf, size_t size)
{
    if (!_canRead) {
        _error = bare::Volume::Error::WriteOnly;
        return -1;
    }
    return io(buf, size, false);
}


size_t File::write(const char* buf, size_t size)
{
    if (!_canWrite) {
        _error = bare::Volume::Error::ReadOnly;
        return -1;
    }
    
    if (_offset + size > _rawFile->size()) {
        // FIXME: support > 32 bit size
        _rawFile->setSize(static_cast<uint32_t>(_offset + size));
        _needsSizeUpate = true;
    }
    return io(const_cast<char*>(buf), size, true);
}

bool File::seek(off_t offset, SeekWhence whence)
{
    if (whence == SeekWhence::Cur) {
        offset += _offset;
    } else if (whence == SeekWhence::End) {
        offset = static_cast<int32_t>(_rawFile->size()) - offset;
    }
    if (offset < 0) {
        offset = 0;
    } else if (offset > static_cast<int32_t>(_rawFile->size())) {
        offset = _rawFile->size();
    }
    _offset = offset;
    _bufferValid = false;
    return true;
}

bare::Volume::Error File::flush()
{
    _error = bare::Volume::Error::OK;
    
    if (_bufferNeedsWriting && _bufferValid) {
        bare::Volume::Error  error = _rawFile->write(_buffer, _bufferAddr, 1);
        if (error != bare::Volume::Error::OK) {
            return error;
        }
        _bufferNeedsWriting = false;
        _bufferValid = false;
    }
    
    if (_needsSizeUpate) {
        _error = _rawFile->updateSize();
        _needsSizeUpate = false;
    }
    
    return _error;
}
