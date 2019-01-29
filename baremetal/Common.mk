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

PLATFORM ?= PLATFORM_RPI
PLATFORMDIR ?= RPi
BUILDDIR ?= $(PLATFORMDIR)/build
TOOLCHAIN ?= arm-none-eabi-

LOADADDR ?= 0x8000

CIRCLE ?= 0
CIRCLEHOME ?= ../../circle

AR = $(TOOLCHAIN)ar
AS = $(TOOLCHAIN)as
CC = $(TOOLCHAIN)gcc
CXX = $(TOOLCHAIN)g++
LD = $(TOOLCHAIN)ld
OBJDUMP = $(TOOLCHAIN)objdump
OBJCOPY = $(TOOLCHAIN)objcopy

ASFLAGS = $(INCLUDES) -mcpu=arm1176jzf-s -mfpu=vfp
CFLAGS = $(INCLUDES) -D$(PLATFORM) -D$(FLOATTYPE) -Wall -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp -MMD

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG -g
else 
    CFLAGS += -DNDEBUG -Os
endif

ifeq ($(CIRCLE), 1)
	LIBS += $(CIRCLEHOME)/lib/libcircle.a
	CFLAGS += -DCIRCLE
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
	cd ../baremetal; make DEBUG=$(DEBUG) PLATFORM=$(PLATFORM) FLOATTYPE=$(FLOATTYPE) PLATFORMDIR=$(PLATFORMDIR) CIRCLE=$(CIRCLE)
	
cleanlibs:
	cd ../baremetal; make clean

$(PRODUCTDIR)/$(PRODUCT).bin : $(LOADER) $(OBJS) makelibs
	@echo "  LD      -Map $(BUILDDIR)/$(PRODUCT).map $(BUILDDIR)/$(PRODUCT).elf"
	@$(LD) $(OBJS) $(LIBS) -T $(LOADER) --section-start=.init=$(LOADADDR) -Map $(BUILDDIR)/$(PRODUCT).map -o $(BUILDDIR)/$(PRODUCT).elf
	@echo "  OBJDUMP $(BUILDDIR)/$(PRODUCT).list"
	@$(OBJDUMP) -D $(BUILDDIR)/$(PRODUCT).elf > $(BUILDDIR)/$(PRODUCT).list
	@echo "  OBJDUMP $(BUILDDIR)/$(PRODUCT).hex"
	@$(OBJCOPY) $(BUILDDIR)/$(PRODUCT).elf -O ihex $(BUILDDIR)/$(PRODUCT).hex
	@echo "  OBJDUMP $(BUILDDIR)/$(PRODUCT).bin"
	@$(OBJCOPY) $(BUILDDIR)/$(PRODUCT).elf -O binary $(PRODUCTDIR)/$(PRODUCT).bin
	@wc -c $(PRODUCTDIR)/$(PRODUCT).bin

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/$(PLATFORMDIR)/%.cpp
	@echo "  CPP   $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/$(PLATFORMDIR)/%.c
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/$(PLATFORMDIR)/%.S
	@echo "  AS    $@"
	@$(CC) $(ASFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/%.cpp
	@echo "  CPP   $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/%.c
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%-$(FLOATTYPE).o: $(SRCDIR)/%.S
	@echo "  AS    $@"
	@$(CC) $(ASFLAGS) -c $< -o $@
