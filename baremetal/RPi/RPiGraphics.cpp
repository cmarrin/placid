/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Graphics.h"
#include "RPiMailbox.h"

using namespace bare;

static constexpr uint32_t V3DBase = 0x20c00000;

struct V3D
{
    uint32_t ident0;
    uint32_t ident1;
    uint32_t ident2;
};

inline volatile V3D& v3d()
{
    return *(reinterpret_cast<volatile V3D*>(V3DBase));
}

bool Graphics::init()
{
    // Set V3D clock to 250Mhz and Enable the QPU units
    if (Mailbox::tagMessage(0, 9,
        Mailbox::Command::SetClockRate, 8, 8, Mailbox::ClockId::V3D, 250000000,
        Mailbox::Command::EnableQPU, 4, 4, 1) == Mailbox::Error::OK)
    {
        if (v3d().ident0 == 0x02443356) {
            return true;
        }
    }
    return false;
}

void Graphics::clear(uint32_t color)
{
}
     
void Graphics::drawTriangle()
{
}

void Graphics::render()
{
}
