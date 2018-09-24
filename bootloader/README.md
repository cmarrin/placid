# Placid Bootloader

## How to run

1) In the bootloader directory, run 'make'
2) In the kernel directory, run 'make' (this is to give you something to autoload)
3) Format an SD card with a FAT32 file system in partition 0
4) Copy all the files from bin to the card (config.txt, bootcode.bin, start.elf)
5) Copy bin/bootloader.bin to the card (this is the binary you just made)
6) Copy ../kernel/build/kernel.bin to the card
7) Remove the card (safely) from your computer and insert it into your RPi Zero
8) Connect a serial port to your computer (see below) and open a serial shell. I use Serial for macOS
9) Power on. You will be greeted with a message on your console.
10) After 3 seconds it will autoload and show you the Placid Kernel Shell
11) If you hit [space] before 3 seconds is up it will go into XModem mode and you can upload any binary if your serial shell supports it (Serial does)

## How to connect a serial port

The RPi Zero connects the serial port to UART1 (the "miniUART") via pins 8 and 10 of the 40 pin header. This is because UART0 is connected to the Wifi/Bluetooth chip. I believe this is also true of the RPi 3, but all the other models connect UART0 to these pins. Since Placid currently assumes UART1, these other boards can't be used. Here is a pretty good description of how to connect everything up: https://elinux.org/RPi_Serial_Connection. I use a little FTDI board from eBay like one of these:

    https://www.ebay.com/itm/FT232RL-3-3V-5-5V-FTDI-USB-to-TTL-Serial-Adapter-Module-for-Arduino-Mini-Port/222418337628?_trkparms=aid%3D555017%26algo%3DPL.CASSINI%26ao%3D1%26asc%3D20150817211623%26meid%3D22bd458248e2412b8b109ecdbd6a604e%26pid%3D100505%26rk%3D1%26rkt%3D1%26%26itm%3D222418337628&_trksid=p2045573.c100505.m3226
    
The nice thing about one of these is that it can work at either 5v or 3.3v by moving the little jumper. **It is super important you set the board for 3.3v**. I recommend you don't try to use the 3.3v power output of the FTDI board. It doesn't have enough power and I was having some flakiness when I tried it. Just use a separate micro USB connector to the power input on the RPi Zero.

Connect the FTDI board to your computer. You'll find lots of articles about any drivers you'll need for your particular environment. The latest macOS and I think Windows has these drivers installed, so there shouldn't be any problems. Then you need a serial shell. I use Serial for macOS, which is pretty full featured. It's 30 bucks, which I was happy to pay to support the developer, but there are free options, too. Set the shell to 115200 baud, 8n1 and local echo. You should then get some good serial output when you Pi come up!

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
