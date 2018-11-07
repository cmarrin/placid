
TOOLCHAIN ?= arm-none-eabi
AS = $(TOOLCHAIN)-as
CC = $(TOOLCHAIN)-gcc
CXX = $(TOOLCHAIN)-g++
LD = $(TOOLCHAIN)-ld
OBJDUMP = $(TOOLCHAIN)-objdump
OBJCOPY = $(TOOLCHAIN)-objcopy

LIBS = ../baremetal/build/baremetal.a
INCLUDES = -I../baremetal

ASFLAGS = --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6 -mfpu=vfp
CFLAGS = $(INCLUDES) -Wall -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp -MMD

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG -g
else 
	CFLAGS += -DNDEBUG -Os
endif

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

BUILDDIR = build

SRC =	bootloader.S \
        autoload.cpp \
        nanoalloc.cpp \
        nanoprintf.cpp \
        main.cpp

OBJS := $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SRC))))
DEP = $(OBJS:%.o=%.d)

all : checkdirs bin/bootloader.bin

-include $(DEP)

clean :
	rm -rf $(BUILDDIR)
	rm -f bin/bootloader.bin
	cd ../baremetal; make -f Makefile clean

checkdirs: $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $@

makelibs:
	cd ../baremetal; make -f Makefile DEBUG=$(DEBUG)

bin/bootloader.bin : loader $(OBJS) makelibs
	$(LD) $(OBJS) $(LIBS) -T loader -Map $(BUILDDIR)/bootloader.map -o $(BUILDDIR)/bootloader.elf
	$(OBJDUMP) -D $(BUILDDIR)/bootloader.elf > $(BUILDDIR)/bootloader.list
	$(OBJCOPY) $(BUILDDIR)/bootloader.elf -O ihex $(BUILDDIR)/bootloader.hex
	$(OBJCOPY) $(BUILDDIR)/bootloader.elf -O binary bin/bootloader.bin
	wc -c bin/bootloader.bin

$(BUILDDIR)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.S Makefile
	$(AS) $(ASFLAGS) -c $< -o $@
