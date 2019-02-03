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
#include <algorithm>

using namespace bare;

void InterruptManager::addHandler(uint8_t id, Handler handler)
{
    if (_handlerIndex >= MaxHandlers) {
        return;
    }
    
    _handlers[_handlerIndex++] = { id, handler };
}

void InterruptManager::handleInterrupt()
{
    for (auto it : _handlers) {
        if (interruptPending(it.id)) {
            if (it.handler) {
                it.handler();
            }
        }
    }
}
