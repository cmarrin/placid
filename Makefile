# -------------------------------------------------------------------------
# This source file is a part of Placid
# 
# For the latest info, see http://www.marrin.org/
# 
# Copyright (c) 2018-2019, Chris Marrin
# All rights reserved.
#
# Use of this source code is governed by the MIT license that can be
# found in the LICENSE file.

baremetal: FORCE
	cd baremetal; make DEBUG=$(DEBUG)

kernel: FORCE
	cd kernel; make DEBUG=$(DEBUG)

bootloader: FORCE
	cd bootloader; make DEBUG=$(DEBUG)
	
FORCE:

clean :
	cd baremetal; make clean
	cd kernel; make clean
	cd bootloader; make clean

