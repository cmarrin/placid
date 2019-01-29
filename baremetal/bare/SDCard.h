/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Volume.h"
#include <stdint.h>
#include <functional>

#define SD_OK                0
#define SD_TIMEOUT          -1
#define SD_ERROR            -2

namespace bare {

    struct Command
    {
        uint32_t code;
#ifdef DEBUG_SD
        const char* name;
#endif
        operator uint32_t() { return code; }
    };

    // SD Card interface. Subclass of SD
    class SDCard : public Volume::RawIO
    {
    public:
        enum class Error { OK, Timeout, Error };

        SDCard();
        
        virtual Volume::Error read(char* buf, Block blockAddr, uint32_t blocks) override;
        virtual Volume::Error write(const char* buf, Block blockAddr, uint32_t blocks) override;
    };

}
