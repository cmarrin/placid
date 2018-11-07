# Placid Bootloader

## Overview

This bootloader is loosly based on bootloader02 from dwelch.

It's purpose is to ease software development by eliminating the need to remove the SD card from the Raspberry Pi to update the software on it. The bootloader has the dual purpose of loading a second stage binary on the SD card or uploading one from the serial console. It is very small and reliable and should have to be changed rarely.

## How it works

The bootloader starts up, prints a message to the serial console, waits 3 seconds and then loads a second stage executable from the SD card named kernel.bin. Within that 3 seconds you can press the space bar on the serial console and the system will wait for an executable to be uploaded using the XModem or YModem protocol.

By default, the Raspberry Pi boot sequence loads an executable called **kernel.img** at location **0x8000** and then jumps to that location to begin execution. The bootloader is called **bootloader.bin** and loads itself lower in memory, at location **0x800**. That way, the second stage executable can use the original address and everything above.

The bootloader also places the stack at 0x8000. Since it grows down and the executable is around 17KB, this allows at least 10KB for the stack, which is plenty.

Note that the bootloader and the second stage executable are **.bin** files, as extracted with the **objcopy -O binary** command. They are not **.elf** or **.hex** files.

## How to run

1) In the bootloader directory, run 'make'
2) In the kernel directory, run 'make' (this is to give you something to autoload)
3) Format an SD card with a FAT32 file system in partition 0
4) Copy all the files from bin to the card, including bin/bootloader.bin (this is the binary you just made)
6) Copy ../kernel/build/kernel.bin to the card
7) Remove the card (after ejecting it) from your computer and insert it into your RPi Zero
8) Connect a serial port to your computer (see below) and open a serial shell. I use **Serial.app** for macOS
9) Power on. You will be greeted with a message on your console.
10) After 3 seconds it will autoload and show you the Placid Kernel Shell
11) If you hit [space] before 3 seconds is up it will go into X/YModem mode and you can upload any binary if your serial shell supports it (Serial does)

## How to connect a serial port

The RPi Zero connects the serial port to UART1 (the "miniUART") via pins 8 and 10 of the 40 pin header. This is because UART0 is connected to the Wifi/Bluetooth chip. I believe this is also true of the RPi 3, but all the other models connect UART0 to these pins. Since Placid currently assumes UART1, these other boards can't be used. Here is a pretty good description of how to connect everything up: https://elinux.org/RPi_Serial_Connection. I use a little FTDI board from eBay like one of these: http://www.ebay.com/sch/i.html?_nkw=ftdi.
    
The nice thing about one of these is that it can work at either 5v or 3.3v by moving the little jumper. **It is super important you set the board for 3.3v**. I recommend you don't try to use the 3.3v power output of the FTDI board. It doesn't have enough power and I was having some flakiness when I tried it. Just use a separate micro USB connector to the power input on the RPi Zero.

Connect the FTDI board to your computer. You'll find lots of articles about any drivers you'll need for your particular environment. The latest macOS and I think Windows has these drivers installed, so there shouldn't be any problems. Then you need a serial shell. I use Serial for macOS, which is pretty full featured. It's 30 bucks, which I was happy to pay to support the developer, but there are free options, too. Set the shell to 115200 baud, 8n1 and no local echo. You should then get some good serial output when you Pi comes up!

## Implementation

Trying to create an SD card reader and a FAT file system in a bare metal environment turns out to be really
difficult. There is quite a bit of code out there to do it, but most is very entwined with the working 
environment, so you end up having t odrag in a lot of extra code. I quickly blew past 0x8000 with my 
first attempt. I eventually found some code that had been very well optimized so I based my implementation on that.

With code under my belt to read raw blocks, I decided to write my own FAT32 format reader. Again, I looked
at a lot of code, but all of it was hard to use. For one thing many of them handled every variant of FAT
and were therefore huge (when's the last time you saw a FAT16 formatted disk?).

Turns out FAT32 isn't too bad as long as you limit what you're trying to do. First, I decided to only support
FAT32 using LBA disk addressing. And I'm limited to 512 byte blocks and 2 FAT copies. All of the cards I've 
tried so far meet these requirements. I can rethink that decision if I find something I can't support. the library
does both reading and writing although the bootloader only uses read. The driver also uses the FAT to traverse the chain of blocks. Even though a FAT can be many sectors long, the driver only ever loads a single 512 byte FAT sector to save RAM. This is generally not a performance issue since most FAT chains are pretty sequential or close to it, so you're not loading new sectors that often.

This is not used in the bootloader, but the driver also can allocate new clusters and link them in the FAT. It can also return clusters to the free list to support file deletion.

The code currently only supports bare 8.3 file names (not long file names) with no path specifiers. So all
files have to be in the root directory. Incoming names are converted to 8.3 so it should work pretty well in most cases.

