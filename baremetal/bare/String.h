/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare.h"

#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <vector>

namespace bare {

    class String {
    public:
        static constexpr size_t npos = std::numeric_limits<size_t>::max();
        
        String() : _size(1), _capacity(0), _data(nullptr) { }
        String(const char* s, int32_t len = -1);
        
        String(const String& other) : _data(nullptr) { *this = other; }
        
        ~String() { delete [ ] _data; };

        String& operator=(const String& other);
        
        explicit operator uint32_t();
        
        const char& operator[](size_t i) const { assert(i >= 0 && i < _size - 1); return _data[i]; };
        char& operator[](size_t i) { assert(i >= 0 && i < _size - 1); return _data[i]; };
        size_t size() const { return _size ? (_size - 1) : 0; }
        bool empty() const { return _size <= 1; }
        void clear() { _size = 1; if (_data) _data[0] = '\0'; }
        void reserve(size_t n = 0) { ensureCapacity(n); }
        String& operator+=(uint8_t c)
        {
            ensureCapacity(_size + 1);
            _data[_size - 1] = c;
            _data[_size++] = '\0';
            return *this;
        }
        
        String& operator+=(char c)
        {
            ensureCapacity(_size + 1);
            _data[_size - 1] = c;
            _data[_size] = '\0';
            _size += 1;
            return *this;
        }
        
        String& operator+=(const char* s)
        {
            size_t len = strlen(s);
            ensureCapacity(_size + len);
            memcpy(_data + _size - 1, s, len + 1);
            _size += len;
            return *this;
        }
        
        String& operator+=(const String& s) { return *this += s.c_str(); }
        
        friend String operator +(const String& s1 , const String& s2) { String s = s1; s += s2; return s; }
        friend String operator +(const String& s1 , const char* s2) { String s = s1; s += s2; return s; }
        friend String operator +(const char* s1 , const String& s2) { String s = s1; s += s2; return s; }
        
        bool operator<(const String& other) const { return strcmp(c_str(), other.c_str()) < 0; }
        bool operator==(const String& other) const { return strcmp(c_str(), other.c_str()) == 0; }
        bool operator!=(const String& other) const { return strcmp(c_str(), other.c_str()) != 0; }

        const char* c_str() const { return _data ? _data : ""; }
        String& erase(size_t pos, size_t len);

        static String format(const char* format, ...);
        static String vformat(const char* format, va_list);
        
        String& erase(size_t pos = 0)
        {
            return erase(pos, _size - pos);
        }
        
        size_t find(const char* str, size_t pos = 0) const;

        String slice(int32_t start, int32_t end) const;
        
        String slice(int32_t start) const { return slice(start, static_cast<int32_t>(size())); }
        
        String trim() const;
        
        // If skipEmpty is true, substrings of zero length are not added to the array
        std::vector<String> split(const String& separator, bool skipEmpty = false) const;
                
        bool isMarked() const { return _marked; }
        void setMarked(bool b) { _marked = b; }
        
    private:
        void ensureCapacity(size_t size)
        {
            if (_capacity >= size) {
                return;
            }
            _capacity = _capacity ? _capacity * 2 : 1;
            if (_capacity < size) {
                _capacity = size;
            }
            char *newData = new char[_capacity];
            assert(newData);
            if (_data) {
                if (newData) {
                    memcpy(newData, _data, _size);
                } else {
                    _capacity = 0;
                    _size = 1;
                }
                delete _data;
            }
            _data = newData;
        };

        size_t _size;
        size_t _capacity;
        char *_data;
        bool _marked = true;
    };

    inline String join(const std::vector<String>& array, const String& separator)
    {
        String s;
        bool first = true;
        for (auto it : array) {
            if (first) {
                first = false;
            } else {
                s += separator;
            }
            s += it;
        }
        return s;
    }

}
