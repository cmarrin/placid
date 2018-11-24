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

PLATFORM ?= PLATFORM_RPI
PLATFORMDIR ?= RPi
BUILDDIR ?= $(PLATFORMDIR)/build
TOOLCHAIN ?= arm-none-eabi-

AR = $(TOOLCHAIN)ar
AS = $(TOOLCHAIN)as
CC = $(TOOLCHAIN)gcc
CXX = $(TOOLCHAIN)g++
LD = $(TOOLCHAIN)ld
OBJDUMP = $(TOOLCHAIN)objdump
OBJCOPY = $(TOOLCHAIN)objcopy

ASFLAGS = -mcpu=arm1176jzf-s -mfpu=vfp
CFLAGS = $(INCLUDES) -D$(PLATFORM) -D$(FLOATTYPE) -Wall -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp -MMD

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG -g
else 
    CFLAGS += -DNDEBUG -Os
endif

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-threadsafe-statics

PRODUCTDIR ?= $(BUILDDIR)

OBJS := $(addprefix $(BUILDDIR)/, $(addsuffix -$(FLOATTYPE).o, $(basename $(notdir $(SRC) $(PLATFORMSRC)))))
DEP = $(OBJS:%.o=%.d)

debug: CXXFLAGS += -DDEBUG -g
debug: CCFLAGS += -DDEBUG -g
debug: all

-include $(DEP)

cleandir :
	rm -rf $(BUILDDIR)
	rm -rf $(PRODUCTDIR)/$(PRODUCT).bin

checkdirs: $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $@

makelibs:
	cd ../baremetal; make DEBUG=$(DEBUG) PLATFORM=$(PLATFORM) FLOATTYPE=$(FLOATTYPE) PLATFORMDIR=$(PLATFORMDIR)
	
cleanlibs:
	cd ../baremetal; make clean

$(PRODUCTDIR)/$(PRODUCT).bin : $(LOADER) $(OBJS) makelibs
	$(LD) $(OBJS) $(LIBS) -T $(LOADER) -Map $(BUILDDIR)/$(PRODUCT).map -o $(BUILDDIR)/$(PRODUCT).elf
	$(OBJDUMP) -D $(BUILDDIR)/$(PRODUCT).elf > $(BUILDDIR)/$(PRODUCT).list
	$(OBJCOPY) $(BUILDDIR)/$(PRODUCT).elf -O ihex $(BUILDDIR)/$(PRODUCT).hex
	$(OBJCOPY) $(BUILDDIR)/$(PRODUCT).elf -O binary $(PRODUCTDIR)/$(PRODUCT).bin
	wc -c $(PRODUCTDIR)/$(PRODUCT).bin

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/$(PLATFORMDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/$(PLATFORMDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/$(PLATFORMDIR)/%.S
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/%.S
	$(CC) $(ASFLAGS) -c $< -o $@
