/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include <Arduino.h>

#include "bare.h"

#include "bare/SPISlave.h"
#include "hspi_slave.h"

using namespace bare;

static void _onDataReceived(void *arg, uint8_t * data, uint8_t len)
{
    SPISlave* p = reinterpret_cast<SPISlave*>(arg);
    if (p->dataReceivedFunction()) {
        p->dataReceivedFunction()(data, len);
    }
}

static void _onDataSent(void *arg)
{
    SPISlave* p = reinterpret_cast<SPISlave*>(arg);
    if (p->dataSentFunction()) {
        p->dataSentFunction()();
    }
}

static void _onStatusReceived(void *arg, uint32_t status)
{
    SPISlave* p = reinterpret_cast<SPISlave*>(arg);
    if (p->statusReceivedFunction()) {
        p->statusReceivedFunction()(status);
    }
}

static void _onStatusSent(void *arg)
{
    SPISlave* p = reinterpret_cast<SPISlave*>(arg);
    if (p->statusSentFunction()) {
        p->statusSentFunction()();
    }
}

void SPISlave::init(EnablePolarity, ClockEdge, ClockPolarity)
{
    // FIXME: handle EnablePolarity, ClockEdge, ClockPolarity
    hspi_slave_begin(1, this);
    hspi_slave_onData(_onDataReceived);
    hspi_slave_onDataSent(_onDataSent);
    hspi_slave_onStatus(_onStatusReceived);
    hspi_slave_onStatusSent(_onStatusSent);
}

void SPISlave::setStatus(uint32_t status)
{
    hspi_slave_setStatus(status);
}

void SPISlave::setData(const uint8_t* data, uint8_t size)
{
    hspi_slave_setData(data, size);
}
