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
#include "bare/String.h"

using namespace bare;

bool bare::receiveFile(ReceiveFunction func)
{
    Serial::printf("Enter file name: ");
    uint8_t c = '\0';
    String name;
    while (1) {
        Serial::read(c);
        if (c >= 0x20 && c <= 0x7f) {
            name += c;
        } else if (c == '\n') {
            break;
        }
    }

    FILE* f = fopen(name.c_str(), "r");
    if (!f) {
        perror("Error opening file");
        return false;
    }
    
    while (1) {
        int result = getc(f);
        if (feof(f)) {
            return true;
        }
        if (result < 0) {
            perror("Error reading character");
            return false;
        }
        func(result);
    }
    return true;
}
