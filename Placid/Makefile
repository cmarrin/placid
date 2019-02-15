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

PLACIDDIR = .
BUILDDIR = build
BAREDIR = ../baremetal
ARCHIVE = libplacid.a

INCLUDES ?= -I$(BAREDIR)

SRC =	Placid.cpp \
		start.S \

all: checkdirs $(ARCHIVE)

include Common.mk

check:
	@echo "BUILDDIR="$(BUILDDIR)
	@echo "ARCHIVE="$(ARCHIVE)
	@echo "OBJS="$(OBJS)

$(ARCHIVE) : $(OBJS)
	$(AR) rcs -o $@ $(OBJS)
