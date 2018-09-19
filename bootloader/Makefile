
ARMGNU ?= arm-none-eabi

COPS = -Wall -O2 -nostdlib -nostartfiles -ffreestanding 

BUILDDIR = build

all : checkdirs gcc

gcc : bin/bootloader.bin

clean :
	rm -rf $(BUILDDIR)
	rm -f bin/bootloader.bin

checkdirs: $(BUILDDIR)

$(BUILDDIR):
	@mkdir -p $@
	
$(BUILDDIR)/vectors.o : vectors.s
	$(ARMGNU)-as vectors.s -o $(BUILDDIR)/vectors.o

$(BUILDDIR)/bootloader04.o : bootloader04.c
	$(ARMGNU)-gcc $(COPS) -c bootloader04.c -o $(BUILDDIR)/bootloader04.o

$(BUILDDIR)/periph.o : periph.c
	$(ARMGNU)-gcc $(COPS) -c periph.c -o $(BUILDDIR)/periph.o

bin/bootloader.bin : loader $(BUILDDIR)/vectors.o $(BUILDDIR)/periph.o $(BUILDDIR)/bootloader04.o 
	$(ARMGNU)-ld $(BUILDDIR)/vectors.o $(BUILDDIR)/periph.o $(BUILDDIR)/bootloader04.o -T loader -o $(BUILDDIR)/bootloader04.elf
	$(ARMGNU)-objdump -D $(BUILDDIR)/bootloader04.elf > $(BUILDDIR)/bootloader04.list
	$(ARMGNU)-objcopy $(BUILDDIR)/bootloader04.elf -O ihex $(BUILDDIR)/bootloader04.hex
	$(ARMGNU)-objcopy $(BUILDDIR)/bootloader04.elf -O binary bin/bootloader.bin
	wc -c bin/bootloader.bin
