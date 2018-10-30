
TOOLCHAIN ?= arm-none-eabi
AR = $(TOOLCHAIN)-ar
AS = $(TOOLCHAIN)-as
CC = $(TOOLCHAIN)-gcc
CXX = $(TOOLCHAIN)-g++
LD = $(TOOLCHAIN)-ld
OBJDUMP = $(TOOLCHAIN)-objdump
OBJCOPY = $(TOOLCHAIN)-objcopy

ASFLAGS = --warn --fatal-warnings -mcpu=arm1176jzf-s -march=armv6 -mfpu=vfp
CFLAGS = $(INCLUDES) -Wall -O2 -nostdlib -nostartfiles -ffreestanding -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mhard-float -mfpu=vfp -MMD
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti

BUILDDIR = build

SRC =	util.cpp \
		xmodem.cpp \
		FAT32.cpp \
		FAT32DirectoryIterator.cpp \
		FAT32RawFile.cpp \
		GPIO.cpp \
		InterruptManager.cpp \
		Mailbox.cpp \
		Print.cpp \
		SD.cpp \
		SDCard.cpp \
		Serial.cpp \
		SPI.cpp \
		Timer.cpp \
		Volume.cpp \
		ldivmod.S \

OBJS := $(addprefix $(BUILDDIR)/, $(addsuffix .o, $(basename $(SRC))))
DEP = $(OBJS:%.o=%.d)

all : checkdirs $(BUILDDIR)/baremetal.a

-include $(DEP)

clean :
	rm -rf $(BUILDDIR)

checkdirs: $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $@

$(BUILDDIR)/baremetal.a : $(OBJS)
	$(AR) rcs -o $@ $(OBJS)

$(BUILDDIR)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: %.S Makefile
	$(AS) $(ASFLAGS) -c $< -o $@

