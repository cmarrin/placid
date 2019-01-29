/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Dispatcher.h"

using namespace placid;

void Dispatcher::exec(const bare::String& name)
{
    std::shared_ptr<Process> process = Process::create(name);
    _processes.push_back(process);
    process->run();
}
