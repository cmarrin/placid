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

FLOATTYPE ?= FLOAT32

SRCDIR = .
INCLUDES = -I.. -I.
ARCHIVE = baremetal-$(FLOATTYPE).a

PLATFORMSRC = \
	RPi/ldivmod.S \
	RPi/idivmod.S \
	RPi/RPiGPIO.cpp \
	RPi/RPiGraphics.cpp \
	RPi/RPiInterruptManager.cpp \
	RPi/RPiMailbox.cpp \
	RPi/RPiMemory.cpp \
	RPi/RPiMemoryMin.cpp \
	RPi/RPiMutex.cpp \
	RPi/RPiSDCard.cpp \
	RPi/RPiSerial.cpp \
	RPi/RPiSPIMaster.cpp \
	RPi/RPiTimer.cpp \

ifeq ($(CIRCLE), 0)
PLATFORMSRC += \
	RPi/uidivmod.S \
	RPi/RPiBare.cpp
else
PLATFORMSRC += \
	RPi/CircleBare.cpp
endif

SRC = \
	bare.cpp \
	fpconv.cpp \
	printf-emb_tiny.c \
	FAT32.cpp \
	FAT32DirectoryIterator.cpp \
	FAT32RawFile.cpp \
	Formatter.cpp \
	FloatFormatter.cpp \
	InterruptManager.cpp \
	Serial.cpp \
	Shell.cpp \
	SPIMaster.cpp \
	String.cpp \
	Timer.cpp \
	Volume.cpp \
	WiFiSPI.cpp \
	WiFiSPIDriver.cpp \
	XYModem.cpp \
	
ifeq ($(CIRCLE), 0)
SRC += utilities.cpp
endif

include Common.mk

all : checkdirs $(BUILDDIR)/$(ARCHIVE)

check:
	echo "BUILDDIR="$(BUILDDIR)
	echo "ARCHIVE="$(ARCHIVE)
	echo "OBJS="$(OBJS)

clean : cleandir

$(BUILDDIR)/$(ARCHIVE) : $(OBJS)
	$(AR) rcs -o $@ $(OBJS)
