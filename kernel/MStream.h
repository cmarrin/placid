/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
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
