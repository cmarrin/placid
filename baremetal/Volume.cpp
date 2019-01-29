/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Volume.h"

#include "bare/SDCard.h"
#include "bare/Serial.h"
#include "bare/Timer.h"

using namespace bare;

const char* Volume::errorDetail(Error error) const
{
    switch (error) {
    case Error::OK: return "OK";
    case Error::Failed: return "failed";
    case Error::FileNotFound: return "file not found";
    case Error::CreationFailure: return "creation failure";
    case Error::InternalError: return "internal error";
    case Error::FileExists: return "file exists";
    case Error::ReadOnly: return "read only";
    case Error::WriteOnly: return "write only";
    case Error::EndOfFile: return "end of file";
    case Error::UnsupportedDevice: return "unsupported device";
    case Error::NotImplemented: return "not implemented";
    default: return "*****";
    }
}

