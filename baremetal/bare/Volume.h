/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <stdint.h>

namespace bare {

    const uint32_t FilenameLength = 32;
    const uint32_t BlockSize = 512;

    class DirectoryIterator;
    class RawFile;

    // Strong types
    template<typename T, typename S>
    class Scalar
    {
    public:
        Scalar() { }
        Scalar(uint32_t v) : _value(v) { }
        
        Scalar operator +(Scalar other) { return _value + other._value; }
        Scalar operator -(Scalar other) { return _value - other._value; }
        Scalar operator *(Scalar other) { return _value * other._value; }
        Scalar operator /(Scalar other) { return _value / other._value; }
        Scalar operator %(Scalar other) { return _value % other._value; }

        S value() const { return _value; }

        bool operator==(const Scalar& other) { return _value == other._value; }
        bool operator!=(const Scalar& other) { return !operator==(other);   }
        bool operator< (const Scalar& other) { return _value < other._value;  }
        bool operator> (const Scalar& other) { return  operator< (other);   }
        bool operator<=(const Scalar& other) { return !operator> (other);   }
        bool operator>=(const Scalar& other) { return !operator< (other);   }
        
        Scalar operator++() { ++_value; return *this; }
        Scalar operator++(int) { S v = _value; ++_value; return Scalar(v); }
        Scalar operator--() { --_value; return *this; }
        Scalar operator--(int) { S v = _value; --_value; return Scalar(v); }

    private:
        S _value = 0;
    };

    class BlockType;
    using Block = Scalar<BlockType, uint32_t>;

    class Volume {
        friend class RawFile;
        
    public:
        enum class Error {
            OK = 0,
            NotMounted,
            CreationFailure,
            Failed,
            FileExists,
            ReadOnly,
            WriteOnly,
            FileNotFound,
            InternalError,
            EndOfFile,
            NotImplemented,
            UnsupportedDevice, 
        };
        
        struct RawIO
        {
            virtual Volume::Error read(char* buf, Block blockAddr, uint32_t blocks) = 0;
            virtual Volume::Error write(const char* buf, Block blockAddr, uint32_t blocks) = 0;
        };
        
        virtual uint32_t sizeInBlocks() const = 0;
        virtual Error mount() = 0;
        virtual RawFile* open(const char* name) = 0;
        virtual Error create(const char* name) = 0;
        virtual Error remove(const char* name) = 0;
        virtual bool exists(const char* name) = 0;
        virtual const char* errorDetail(Error) const;
        virtual DirectoryIterator* directoryIterator(const char* path) = 0;
        virtual Error error() const = 0;

        Volume() { }
    };

    class RawFile {
        friend class Volume;
        
    public:
        RawFile() { }
        
        virtual Volume::Error read(char* buf, Block blockAddr, uint32_t blocks) = 0;    
        virtual Volume::Error write(const char* buf, Block blockAddr, uint32_t blocks) = 0;    

        virtual Volume::Error rename(const char* to) = 0;
        
        virtual Volume::Error insertCluster() = 0;
        virtual Volume::Error updateSize() = 0;
        
        bool valid() const { return _error == Volume::Error::OK; }
        Volume::Error error() const { return _error; }
        uint32_t size() const { return _size; }
        void setSize(uint32_t size) { _size = size; }

    protected:
        Volume::Error _error = Volume::Error::OK;
        uint32_t _size = 0;
    };

    class DirectoryIterator
    {
    public:
        virtual ~DirectoryIterator() { }
        virtual DirectoryIterator& next() = 0;
        virtual const char* name() const = 0;
        virtual uint32_t size() const = 0;
        virtual operator bool() const = 0;
    };

}
