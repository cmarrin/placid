/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Process.h"

using namespace placid;

std::shared_ptr<Process> Process::create(const bare::String& name)
{
    return std::make_shared<Process>(name);
}

Process::Process(const bare::String& name)
{
}

void Process::run()
{
}
