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
