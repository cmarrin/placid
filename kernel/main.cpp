/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Serial.h"

extern "C" void init();
extern "C" void inputChar(uint8_t);

int main()
{
    init();

    while (1) {
        uint8_t c;
        if (bare::Serial::read(c) != bare::Serial::Error::OK) {
            bare::Serial::puts("*** Serial Read Error\n");
        } else {
            inputChar(c);
            if (c == '\r') {
                inputChar('\n');
            }
        }
    }
}
