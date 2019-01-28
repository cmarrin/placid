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

#include "bare.h"

#include "bare/String.h"

#include "bare/Formatter.h"

using namespace bare;

String::String(const char* s, int32_t len) : _size(1), _capacity(0), _data(nullptr)
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

String& String::operator=(const String& other)
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

String& String::erase(size_t pos, size_t len)
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

size_t String::find(const char* str, size_t pos) const
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

String String::slice(int32_t start, int32_t end) const
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

String String::trim() const
{
    if (_size < 2 || !_data) {
        return String();
    }
    size_t l = _size - 1;
    char* s = _data;
    while (isSpace(s[l - 1])) {
        --l;
    }
    while (*s && isSpace(*s)) {
        ++s;
        --l;
    }
    return String(s, static_cast<int32_t>(l));
}

// If skipEmpty is true, substrings of zero length are not added to the array
std::vector<String> String::split(const String& separator, bool skipEmpty) const
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

String::operator uint32_t()
{
    uint32_t n;
    const char* p = c_str();
    return Formatter::toNumber(p, n) ? n : 0;
}

String String::format(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    return String::vformat(format, va);
}

String String::vformat(const char* format, va_list va)
{
    String s;
    bare::Formatter::vformat([&s](char c) { s += c; }, format, va);
    return s;
}
