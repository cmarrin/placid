/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/InterruptManager.h"

#include "bare/Serial.h"

using namespace bare;

void InterruptManager::addHandler(uint8_t id, Handler handler)
{
    if (id >= MaxHandlers) {
        return;
    }
    _handlers[id] = handler;
}

int32_t InterruptManager::findFirstBit(uint32_t* pendingBits)
{
    for (uint32_t i = 0; i < PendingArraySize; ++i) {
        int32_t bit = i * 32;
        if (pendingBits[i]) {
            for (int32_t mask = 1; pendingBits[i]; ++bit, mask <<= 1) {
                if (pendingBits[i] & mask) {
                    pendingBits[i] &= ~mask;
                    return bit;
                }
            }
        }
    }
    return -1;
}

void InterruptManager::handleInterrupt()
{
    uint32_t pendingBits[PendingArraySize];
    interruptsPending(pendingBits);
    
    while (1) {
        int32_t id = findFirstBit(pendingBits);

        if (id < 0) {
            return;
        }
        if (_handlers[id]) {
            _handlers[id]();
        }
    }
}
