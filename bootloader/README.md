# Placid Bootloader

## Overview

This bootloader is based on bootloader02 from dwelch.

The code is loaded at 0x0800 and is currently about 13k, which takes it to about 0x3b00.
But the loader file has a size of 0x5000 to allow for growth. From there to 0x8000 is the bootloader stack. 
That's about 12KB for the stack which should be plenty.

The loader does two things. If you press the space bar or let it timeout for 5 seconds, it will autoload
a file on the first partition of the SD card called kernel.bin. Note that it's not kernel.img to
distinguish it from the standard kernal binary. If you press the return key within 5 seconds it will
wait for and XModem transfer. Assuming you have a serial shell that supports it you can upload a
binary and run it.

Whether you autoload or manually load a binary, the file goes to 0x8000 just like most stock bare metal
Raspberry Pi binaries. Note that this is a .bin file, as extracted with the objcopy -O binary command. 
It's not a .elf or a .hex file.

## Implementation

Trying to create an SD card reader and a FAT file system in a bare metal environment turns out to be really
difficult. There is quite a bit of code out there to do it, but most is very entwined with the working 
environment, so you end up having t odrag in a lot of extra code. I quickly blew past 0x8000 with my 
first attempt. I finally found sdcard.c which was part of the RPiHaribote project from moizumi99. This
is a great piece of code which didn't have many dependencies, and it had some nice logging code that helped
enormously in getting it working in my environment.

With code under my belt to read raw sectors, I decided to write my own FAT32 format reader. Again, I looked
at a lot of code, but all of it was hard to use. For one thing many of them handled every variant of FAT
and were therefore huge (when's the last time you saw a FAT16 formatted disk?).

Turns out FAT32 isn't too bad as long as you limit what you're trying to do. First, I decided to only support
FAT32 using LBA disk addressing. And I'm limited to 512 byte sectors and 2 FAT copies. All of the cards I've 
tried so far meet these requirements. I can rethink that decision if I find something I can't support. I also
am only doing file reading since that's all I need for a bootloader. When I start on the actual kernel I'll
have to flesh out the implementation. I also only support bare file names with no path specifiers. So all
files have to be in the root directory. And I don't support root directories with multiple clusters, so I
can skip looking at the FAT data. This is probably fine since all the cards I've see have 32KB clusters which
holds about a thousand files. So I think it will hold me for a while. Finally, I only look at the 8.3 
filenames. I convert incoming names to 8.3 so it should work pretty well in most cases.

Because of all this the code is pretty light on memory. I never read in the FAT, so I don't need to take up
memory for that. And I read the root directory one sector at a time when doing a file name search. That 
512 byte buffer is allocated on the stack, and since I have at least 12KB to work with, there should be 
plenty of space. This would be very inefficient if it weren't for the fact that I'll mostly be searching
for one file and then loading that.
