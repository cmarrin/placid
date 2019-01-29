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

FLOATTYPE = FLOATNONE

PLATFORMDIR = RPi

LIBS = ../baremetal/$(PLATFORMDIR)/build/baremetal-$(FLOATTYPE).a
INCLUDES = -I../baremetal
BUILDDIR = build
PRODUCTDIR = bin
PRODUCT = bootloader
LOADER = loader
SRCDIR = .

SRC =	bootloader.S \
        autoload.cpp \
        nanoalloc.cpp \
        main.cpp

all : checkdirs $(PRODUCTDIR)/$(PRODUCT).bin

include ../baremetal/Common.mk

clean : cleandir cleanlibs
