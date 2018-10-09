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

#include "SD.h"
#include "FS.h"
#include <stdint.h>
#include <functional>

#define SD_OK                0
#define SD_TIMEOUT          -1
#define SD_ERROR            -2

namespace placid {

    struct Command
    {
        uint32_t code;
#ifdef DEBUG_SD
        const char* name;
#endif
        operator uint32_t() { return code; }
    };

    // SD Card interface. Subclass of SD
    class SDCard : public SD, public FS::RawIO
    {
    public:
        SDCard();
        
        virtual int32_t read(char* buf, uint32_t blockAddr, uint32_t blocks) override;
        virtual int32_t write(const char* buf, uint32_t blockAddr, uint32_t blocks) override { return -1; }

    private:
        bool checkStatusWithTimeout(std::function<bool()>, const char* error, uint32_t count = 1000);
        Error setClock(uint32_t freq, uint32_t hostVersion);
        Error sendCommand(const Command&, uint32_t arg);
        Error sendCommand(const Command&, uint32_t arg, uint32_t& response);
        Error readStatus(uint32_t mask);
        Error waitForInterrupt(uint32_t mask);
        Error setSCRValues();
        void finishFail() const { DEBUG_LOG("SDCard: EMMC init FAILED!\n"); }
        
        uint32_t _rca = 0;
        uint32_t _scr[2] { 0, 0 };
    };

}
