# -------------------------------------------------------------------------
# This source file is a part of Placid
# 
# For the latest info, see http://www.marrin.org/
# 
# Copyright (c) 2018, Chris Marrin
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 
#     - Redistributions of source code must retain the above copyright notice, 
#     this list of conditions and the following disclaimer.
#     
#     - Redistributions in binary form must reproduce the above copyright 
#     notice, this list of conditions and the following disclaimer in the 
#     documentation and/or other materials provided with the distribution.
#     
#     - Neither the name of the <ORGANIZATION> nor the names of its 
#     contributors may be used to endorse or promote products derived from 
#     this software without specific prior written permission.
#     
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
# POSSIBILITY OF SUCH DAMAGE.
# -------------------------------------------------------------------------

FLOATTYPE ?= FLOAT32

SRCDIR = .
INCLUDES = -I.. -I.
ARCHIVE = baremetal-$(FLOATTYPE).a

PLATFORMSRC = \
	RPi/ldivmod.S \
	RPi/idivmod.S \
	RPi/uidivmod.S \
	RPi/RPiBare.cpp \
	RPi/RPiGPIO.cpp \
	RPi/RPiInterruptManager.cpp \
	RPi/RPiMailbox.cpp \
	RPi/RPiMemory.cpp \
	RPi/RPiMemoryMin.cpp \
	RPi/RPiMutex.cpp \
	RPi/RPiSDCard.cpp \
	RPi/RPiSerial.cpp \
	RPi/RPiSPI.cpp \
	RPi/RPiTimer.cpp \
	
SRC = \
	bare.cpp \
	fpconv.cpp \
	printf-emb_tiny.c \
	FAT32.cpp \
	FAT32DirectoryIterator.cpp \
	FAT32RawFile.cpp \
	Print.cpp \
	PrintFloat.cpp \
	Serial.cpp \
	String.cpp \
	Timer.cpp \
	Volume.cpp \
	WiFiSpi.cpp \
	WiFiSpiDriver.cpp \
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
