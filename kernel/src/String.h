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

#include "util.h"
#include <cstdint>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <vector>

namespace placid {

    class String {
    public:
        static constexpr size_t npos = std::numeric_limits<size_t>::max();
        
        String() : _size(1), _capacity(0), _data(nullptr) { }
        String(const char* s, int32_t len = -1) : _size(1), _capacity(0), _data(nullptr)
        {
            if (!s) {
                return;
            }
            if (len == -1) {
                len = static_cast<int32_t>(strlen(s));
            }
            ensureCapacity(len + 1);
            if (len) {
                memcpy(_data, s, len);
            }
            _size = len + 1;
            _data[_size - 1] = '\0';
        }
        
        String(const String& other) : _data(nullptr)
        {
            *this = other;
        };
        
        ~String() { delete [ ] _data; };

        String& operator=(const String& other)
        {
            if (_data) {
                delete [ ] _data;
            }
            _size = other._size;
            _capacity = other._capacity;
            if (!other._data) {
                _data = nullptr;
                return *this;
            }
            
            _data = new char[_capacity];
            assert(_data);
            if (_data) {
                memcpy(_data, other._data, _size);
            } else {
                _capacity = 0;
                _size = 1;
            }
            return *this;
        }
        
        explicit operator uint32_t();
        
        const char& operator[](size_t i) const { assert(i >= 0 && i < _size - 1); return _data[i]; };
        char& operator[](size_t i) { assert(i >= 0 && i < _size - 1); return _data[i]; };
        size_t size() const { return _size ? (_size - 1) : 0; }
        bool empty() const { return _size <= 1; }
        void clear() { _size = 1; if (_data) _data[0] = '\0'; }
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
        String& erase(size_t pos, size_t len)
        {
            if (pos >= _size - 1) {
                return *this;
            }
            if (pos + len >= _size) {
                len = _size - pos - 1;
            }
            memmove(_data + pos, _data + pos + len, _size - pos - len);
            _size -= len;
            return *this;
        }

        String& printf(const char* format, ...);
        String& vprintf(const char* format, va_list);
        
        String& erase(size_t pos = 0)
        {
            return erase(pos, _size - pos);
        }
        
        size_t find(const char* str, size_t pos = 0) const
        {
            if (str == nullptr) {
                return npos;
            }

            for(size_t i = pos; ; i++) {
                char c1 = _data[i];
                if (c1 == '\0') {
                    return npos;
                }
                
                char c2 = *str;
                if (c1 == c2) {
                    for (size_t j = i; ; j++) {
                        c2 = str[j - i];
                        if (c2 == '\0') {
                            return i;
                        }
                        c1 = _data[j];
                        if (c1 != c2) {
                            break;
                        }
                    }
                }
            }
        }

        String slice(int32_t start, int32_t end) const
        {
            int32_t sz = static_cast<int32_t>(size());
            if (start < 0) {
                start = sz + start;
            }
            if (end < 0) {
                end = sz + end;
            }
            if (end > sz) {
                end = sz;
            }
            if (start >= end) {
                return String();
            }
            return String(_data + start, end - start);
        }
        
        String slice(int32_t start) const
        {
            return slice(start, static_cast<int32_t>(size()));
        }
        
        String trim() const
        {
            if (_size < 2 || !_data) {
                return String();
            }
            size_t l = _size - 1;
            char* s = _data;
            while (isspace(s[l - 1])) {
                --l;
            }
            while (*s && isspace(*s)) {
                ++s;
                --l;
            }
            return String(s, static_cast<int32_t>(l));
        }
        
        // If skipEmpty is true, substrings of zero length are not added to the array
        std::vector<String> split(const String& separator, bool skipEmpty = false) const
        {
            std::vector<String> array;
            if (!size()) {
                return array;
            }
            
            size_t offset = 0;
            while (1) {
                size_t n = find(separator.c_str(), offset);
                bool found = n != npos;
                size_t length = (found ? n : size()) - offset;
                if (length || !skipEmpty) {
                    array.push_back(slice(static_cast<int32_t>(offset), static_cast<int32_t>(offset + length)));
                }
                
                if (!found) {
                    break;
                }
                offset = n + separator.size();
            }
            return array;
        }
                
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
