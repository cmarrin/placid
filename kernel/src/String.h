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

#include <cstring>
#include <algorithm>
#include <vector>

namespace placid {
	
	// String - std::string functions
	//

	class String
	{
	public:
		
		static inline std::string& trim(std::string& s)
	    {
			// Trim from the start
		    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		        return !std::isspace(ch);
		    }));

			// Trim from the end
		    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		        return !std::isspace(ch);
		    }).base(), s.end());
		
			return s;
	    }
    
	    // If skipEmpty is true, substrings of zero length are not added to the array
	    static inline std::vector<std::string> split(const std::string& s, const std::string& separator, bool skipEmpty = false)
	    {
	        std::vector<std::string> array;
	        const char* p = s.c_str();
	        while (1) {
	            const char* n = strstr(p, separator.c_str());
	            if (!n || n - p != 0 || !skipEmpty) {
	                array.push_back(std::string(p, static_cast<int32_t>(n ? (n - p) : -1)));
	            }
	            if (!n) {
	                break;
	            }
	            p = n ? (n + separator.size()) : nullptr;
	        }
	        return array;
	    }

	    static inline std::string join(const std::vector<std::string>& array, const std::string& separator)
	    {
	        std::string s;
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
	};

}
