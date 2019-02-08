/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "bare/Shell.h"

namespace placid {
	
	// BootShell - Shell with boot commands
	//
	// Subclass of Shell

	class BootShell : public bare::Shell {
	public:
		virtual const char* welcomeString() const override;
		virtual const char* helpString() const override;
        virtual const char* promptString() const override;
	    virtual void shellSend(const char* data, uint32_t size = 0, bool raw = false) override;
		virtual bool executeShellCommand(const std::vector<bare::String>&) override;
	
    private:
        void putFile(const char* name);
    };
	
}
