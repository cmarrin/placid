
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
CFLAGS = $(INCLUDES) -Wall -O2 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp -MMD
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

BUILDDIR = build

SRC =	bootloader.S \
        autoload.cpp \
        nanoprintf.cpp \
        xmodem.cpp \
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
	cd ../baremetal; make -f Makefile

bin/bootloader.bin : loader $(OBJS) makelibs
	$(LD) $(OBJS) $(LIBS) -T loader -o $(BUILDDIR)/bootloader04.elf
	$(OBJDUMP) -D $(BUILDDIR)/bootloader04.elf > $(BUILDDIR)/bootloader04.list
	$(OBJCOPY) $(BUILDDIR)/bootloader04.elf -O ihex $(BUILDDIR)/bootloader04.hex
	$(OBJCOPY) $(BUILDDIR)/bootloader04.elf -O binary bin/bootloader.bin
	wc -c bin/bootloader.bin

$(BUILDDIR)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.S Makefile
	$(AS) $(ASFLAGS) -c $< -o $@
