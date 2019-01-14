/*-------------------------------------------------------------------------
This source file is a part of placid

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

#include "bare/String.h"
#include <cstdint>

namespace placid {

    //////////////////////////////////////////////////////////////////////////////
    //
    //  Class: Stream
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class Stream {
    public:
        virtual bool eof() const = 0;
        virtual int read() const = 0;
        virtual int write(uint8_t) = 0;
        virtual void flush() = 0;
        
    private:
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    //  Class: FileStream
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class FileStream : public Stream {
    //public:
    //    FileStream(FS* fs, const char* file, const char* mode = "r")
    //    {
    //        _file = fs->open(file, mode);
    //    }
    //
    //    ~FileStream()
    //    {
    //        if (_file) {
    //            delete _file;
    //        }
    //    }
    //    
    //    bool loaded()
    //    {
    //        return _file && _file->valid();
    //    }
    //    virtual bool eof() const override
    //    {
    //        return !_file || _file->eof();
    //    }
    //    virtual int read() const override
    //    {
    //        if (!_file) {
    //            return -1;
    //        }
    //        char c;
    //        if (_file->read(&c, 1) != 1) {
    //            return -1;
    //        }
    //        return static_cast<uint8_t>(c);
    //    }
    //    virtual int write(uint8_t c) override
    //    {
    //        if (!_file) {
    //            return -1;
    //        }
    //        if (_file->write(reinterpret_cast<char*>(&c), 1) != 1) {
    //            return -1;
    //        }
    //        return c;
    //    }
    //    virtual void flush() override { }
    //    
    //private:
    //    File* _file = nullptr;
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    //  Class: StringStream
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class StringStream : public Stream {
    public:
        StringStream() { }
        StringStream(const bare::String& s) : _string(s) { }
        StringStream(const char* s) : _string(s) { }
        
        virtual ~StringStream() { }
        
        bool loaded() { return true; }
        virtual bool eof() const override
        {
            return _string.size() <= _index;
        }
        virtual int read() const override
        {
            return (_index < _string.size()) ? _string[_index++] : -1;
        }
        virtual int write(uint8_t c) override
        {
            // Only allow writing to the end of the string
            if (_index != _string.size()) {
                return -1;
            }
            _string += c;
            _index++;
            return c;
        }
        virtual void flush() override { }
        
    private:
        bare::String _string;
        mutable uint32_t _index = 0;
    };

    //////////////////////////////////////////////////////////////////////////////
    //
    //  Class: VectorStream
    //
    //
    //
    //////////////////////////////////////////////////////////////////////////////

    class VectorStream : public Stream {
    public:
        VectorStream() : _index(0) { }
        VectorStream(const std::vector<uint8_t>& vector) : _vector(vector), _index(0) { }
        
        virtual ~VectorStream() { }
        
        bool loaded() { return true; }
        virtual bool eof() const override
        {
            return _vector.size() <= _index;
        }
        virtual int read() const override
        {
            return (_index < _vector.size()) ? _vector[_index++] : -1;
        }
        virtual int write(uint8_t c) override
        {
            // Only allow writing to the end of the vector
            if (_index != _vector.size()) {
                return -1;
            }
            _vector.push_back(c);
            _index++;
            return c;
        }
        virtual void flush() override { }
        
        void swap(std::vector<uint8_t>& vector) { std::swap(vector, _vector); }
        
    private:
        std::vector<uint8_t> _vector;
        mutable uint32_t _index;
    };

}
