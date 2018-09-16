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

TOOLCHAIN ?= arm-none-eabi
AS = $(TOOLCHAIN)-as
CC = $(TOOLCHAIN)-gcc
CXX = $(TOOLCHAIN)-g++
LD = $(TOOLCHAIN)-ld
OBJDUMP = $(TOOLCHAIN)-objdump
OBJCOPY = $(TOOLCHAIN)-objcopy

ASFLAGS = --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6 -mfpu=vfp
CFLAGS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
LDFLAGS =  -L/usr/local/opt/gcc-arm-none-eabi/lib/gcc/arm-none-eabi/7.2.1 -lgcc
LIB1FUNCSFLAGS = 	-O2 -g -Os -mcpu=arm1176jzf-s -march=armv6 -mfpu=vfp -w -DIN_GCC -DCROSS_DIRECTORY_STRUCTURE \
					-W -Wall -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition \
					-fno-inline -g  -DIN_LIBGCC2 -D__GCC_FLOAT_NOT_NEEDED -Dinhibit_libc  -DHAVE_CC_TLS -xassembler-with-cpp 

SRCDIR = src
BUILDDIR = build

SRC =	start.S \
		crc16.c \
		dlmalloc.c \
		main.cpp \
		memory.cpp \
		util.cpp \
		xmodem.c \
		BootShell.cpp \
		GPIO.cpp \
		InterruptManager.cpp \
		Serial.cpp \
		Shell.cpp \
		Timer.cpp \
		
DIVOBJS :=	_udivsi3.o \
			_divsi3.o \
			_umodsi3.o \
			_modsi3.o \
			_div0.o \
			_div.o

OBJS := $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SRC))))
DIVOBJS := $(addprefix $(BUILDDIR)/, $(DIVOBJS))

all: checkdirs $(BUILDDIR)/bootloader.bin
	
clean:
	rm -f $(OBJS) $(DIVOBJS) $(BUILDDIR)/bootloader.elf $(BUILDDIR)/bootloader.bin $(BUILDDIR)/bootloader.hex $(BUILDDIR)/bootloader.list $(BUILDDIR)/bootloader.sym
	
checkdirs: $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $@
	
$(BUILDDIR)/bootloader.bin: loadmap.ld $(OBJS) $(DIVOBJS)
	$(LD) $(LDFLAGS) $(OBJS) $(DIVOBJS) -T loadmap.ld -o $(BUILDDIR)/bootloader.elf
	$(OBJDUMP) -D $(BUILDDIR)/bootloader.elf > $(BUILDDIR)/bootloader.list
	$(OBJCOPY) $(BUILDDIR)/bootloader.elf -O ihex $(BUILDDIR)/bootloader.hex
	$(OBJCOPY) --only-keep-debug $(BUILDDIR)/bootloader.elf $(BUILDDIR)/bootloader.sym
	$(OBJCOPY) $(BUILDDIR)/bootloader.elf -O binary $(BUILDDIR)/bootloader.bin
	wc -c $(BUILDDIR)/bootloader.bin

$(BUILDDIR)/%.o: $(SRCDIR)/%.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.S Makefile
	$(AS) $(ASFLAGS) -c $< -o $@

$(DIVOBJS): $(SRCDIR)/lib1funcs.asm
	$(CC) $(LIB1FUNCSFLAGS)-o $@ -DL$(basename $(notdir $@)) -c $(SRCDIR)/lib1funcs.asm
