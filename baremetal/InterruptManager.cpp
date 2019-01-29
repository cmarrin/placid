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

using namespace bare;

void InterruptManager::addHandler(Handler handler)
{
    if (_handlerIndex >= MaxHandlers) {
        return;
    }
    _handlers[_handlerIndex++] = handler;
}

void InterruptManager::handleInterrupt()
{
    for (uint8_t i = 0; i < _handlerIndex; ++i) {
        _handlers[i]();
    }
}
