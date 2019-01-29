/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/SPIMaster.h"

//#define ENABLE_DEBUG_LOG
#include "bare/Log.h"
#include <algorithm>

using namespace bare;

static constexpr uint8_t WriteStatus = 0x01;
static constexpr uint8_t WriteData = 0x02;
static constexpr uint8_t ReadData = 0x03;
static constexpr uint8_t ReadStatus = 0x04;

void SPIMaster::sendStatus(uint32_t status, uint8_t statusByteCount)
{
    statusByteCount = std::max(std::min(static_cast<int>(statusByteCount), 4), 1);
    startTransfer();
    transferByte(WriteStatus);
    transferByte(status & 0xff);
    if (statusByteCount > 1) {
        transferByte(status >> 8 & 0xff);
    }
    if (statusByteCount > 2) {
        transferByte(status >> 16 & 0xff);
    }
    if (statusByteCount > 3) {
        transferByte(status >> 24 & 0xff);
    }
    endTransfer();
}

void SPIMaster::sendData(const uint8_t* data, uint8_t size)
{
    startTransfer();
    transferByte(WriteData);
    transferByte(0);
    for (int i = 0; i < size; i++) {
        transferByte(data[i]);
    }
    endTransfer();
}

uint32_t SPIMaster::receiveStatus(uint8_t statusByteCount)
{
    statusByteCount = std::max(std::min(static_cast<int>(statusByteCount), 4), 1);
    startTransfer();
    transferByte(ReadStatus);
    uint32_t status = transferByte(0);
    if (statusByteCount > 1) {
        status += static_cast<uint32_t>(transferByte(0)) << 8;
    }
    if (statusByteCount > 2) {
        status += static_cast<uint32_t>(transferByte(0)) << 16;
    }
    if (statusByteCount > 3) {
        status += static_cast<uint32_t>(transferByte(0)) << 24;
    }
    endTransfer();
    return status;
}

uint8_t SPIMaster::receiveData(uint8_t* data, uint8_t maxSize)
{
    startTransfer();
    transferByte(ReadData);
    transferByte(0);
    for (int i = 0; i < maxSize; i++) {
        data[i] = transferByte(0);
    }
    endTransfer();
    return maxSize;
}
