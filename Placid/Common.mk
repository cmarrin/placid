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
TOOLCHAIN ?= arm-none-eabi-

LOADADDR ?= 0x8000

AR = $(TOOLCHAIN)ar
AS = $(TOOLCHAIN)as
CC = $(TOOLCHAIN)gcc
CXX = $(TOOLCHAIN)g++
LD = $(TOOLCHAIN)ld
OBJDUMP = $(TOOLCHAIN)objdump
OBJCOPY = $(TOOLCHAIN)objcopy

INCLUDES = -I$(PLACIDDIR)

ASFLAGS = $(INCLUDES) -mcpu=arm1176jzf-s -mfpu=vfp
CFLAGS = $(INCLUDES) -D$(PLATFORM) -Wall -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp -MMD

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS += -DDEBUG -g
else 
    CFLAGS += -DNDEBUG -Os
endif

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-threadsafe-statics

OBJS := $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(notdir $(SRC)))))
DEP = $(OBJS:%.o=%.d)

debug: CXXFLAGS += -DDEBUG -g
debug: CCFLAGS += -DDEBUG -g
debug: all

clean :
	rm -rf $(BUILDDIR)
	rm -rf $(PRODUCT)

-include $(DEP)

checkdirs: $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $@

$(PRODUCT) : $(LOADER) $(OBJS)
	@echo "  LD      -Map $(BUILDDIR)/$(PRODUCT).map $(PRODUCT)"
	@$(LD) $(OBJS) $(LIBS) -T $(LOADER) --section-start=.init=$(LOADADDR) -Map $(BUILDDIR)/$(PRODUCT).map -o $(PRODUCT)
	@echo "  OBJDUMP $(BUILDDIR)/$(PRODUCT).list"
	@$(OBJDUMP) -D $(BUILDDIR)/$(PRODUCT).elf > $(BUILDDIR)/$(PRODUCT).list

$(BUILDDIR)/%.o: %.cpp
	@echo "  CPP   $@"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.c
	@echo "  CC    $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.S
	@echo "  AS    $@"
	@$(CC) $(ASFLAGS) -c $< -o $@
