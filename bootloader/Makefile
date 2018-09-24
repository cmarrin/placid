
TOOLCHAIN ?= arm-none-eabi
AS = $(TOOLCHAIN)-as
CC = $(TOOLCHAIN)-gcc
CXX = $(TOOLCHAIN)-g++
LD = $(TOOLCHAIN)-ld
OBJDUMP = $(TOOLCHAIN)-objdump
OBJCOPY = $(TOOLCHAIN)-objcopy

CFLAGS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding 
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

BUILDDIR = build

SRC =	vectors.S \
        bootutil.c \
        mailbox.c \
        periph.c \
        sdcard.c \
        autoload.cpp \
        xmodem.cpp \
        SDFS.cpp \
        main.cpp
		
OBJS := $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SRC))))

all : checkdirs bin/bootloader.bin

clean :
	rm -rf $(BUILDDIR)
	rm -f bin/bootloader.bin

checkdirs: $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $@

bin/bootloader.bin : loader $(OBJS)
	$(LD) $(OBJS) -T loader -o $(BUILDDIR)/bootloader04.elf
	$(OBJDUMP) -D $(BUILDDIR)/bootloader04.elf > $(BUILDDIR)/bootloader04.list
	$(OBJCOPY) $(BUILDDIR)/bootloader04.elf -O ihex $(BUILDDIR)/bootloader04.hex
	$(OBJCOPY) $(BUILDDIR)/bootloader04.elf -O binary bin/bootloader.bin
	wc -c bin/bootloader.bin

$(BUILDDIR)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.S Makefile
	$(AS) -c $< -o $@

