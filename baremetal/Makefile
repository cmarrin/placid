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

FLOATTYPE ?= FLOATDOUBLE
PLATFORMDIR ?= Circle

SRCDIR = .
INCLUDES = -I.. -I.
ARCHIVE = baremetal-$(FLOATTYPE).a

include $(PLATFORMDIR)/Common.mk

SRC = \
	bare.cpp \
	fpconv.cpp \
	printf-emb_tiny.c \
	utilities.cpp \
	FAT32.cpp \
	FAT32DirectoryIterator.cpp \
	FAT32RawFile.cpp \
	Formatter.cpp \
	FloatFormatter.cpp \
	InterruptManager.cpp \
	RealTime.cpp \
	Serial.cpp \
	Shell.cpp \
	SPIMaster.cpp \
	String.cpp \
	Timer.cpp \
	Volume.cpp \
	WiFiSPI.cpp \
	WiFiSPIDriver.cpp \
	XYModem.cpp \

include Common.mk

all : checkdirs $(BUILDDIR)/$(ARCHIVE)

check:
	echo "BUILDDIR="$(BUILDDIR)
	echo "ARCHIVE="$(ARCHIVE)
	echo "OBJS="$(OBJS)

clean : cleandir

$(BUILDDIR)/$(ARCHIVE) : $(OBJS)
	$(AR) rcs -o $@ $(OBJS)
