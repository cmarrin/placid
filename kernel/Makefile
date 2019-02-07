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
		ELFLoader.cpp \
		FileSystem.cpp \
		Process.cpp \
		Scanner.cpp \

all: checkdirs $(PRODUCTDIR)/$(PRODUCT).bin

include ../baremetal/Common.mk

clean : cleandir cleanlibs
