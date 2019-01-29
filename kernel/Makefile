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

FLOATTYPE = FLOAT64

PLATFORMDIR = RPi

CIRCLE = 0

LIBS = ../baremetal/$(PLATFORMDIR)/build/baremetal-$(FLOATTYPE).a
INCLUDES = -I../baremetal
BUILDDIR = build
PRODUCTDIR = $(BUILDDIR)
PRODUCT = kernel
LOADER = loadmap.ld
SRCDIR = .

SRC =	init.cpp \
        main.cpp \
		Allocator.cpp \
		BootShell.cpp \
		Dispatcher.cpp \
		FileSystem.cpp \
		Process.cpp \
		Scanner.cpp \

ifeq ($(CIRCLE), 0)
SRC +=	kernel.S
endif

all: checkdirs $(PRODUCTDIR)/$(PRODUCT).bin

include ../baremetal/Common.mk

clean : cleandir cleanlibs
