# Placid

## First thoughts

I started thinking about bare metal programming on the Raspberry Pi when I tried running OctoPrint (https://octoprint.org) on Model B+. This is a Linux web server designed to run a 3D printer. It works, but it takes minutes to boot into a running server. This is crazy. The Model B+ is not the newest, but it is a powerful ARM processor with 512MB of RAM. It made me harken back to a long time complaint of mine - that as software gets more complex it continues to run slower and slower with each new release. 

<examples and reasons why computers are so damn slow>

The solution was always to get more RAM or a bigger disk, or a whole new machine.

So I decided to see what I could do if I went right down to the bare metal. The performance would be much better without a doubt. But could I do anything practical with it? Could I build, for instance, a web browser, email client or word processor on top of it? I probably could if I simply ported over lots of legacy libraries, but that would kind of defeat the purpose. I’d just be putting bloated software one layer up.

So I’m starting by writing a bootloader. It will talk to the serial port. The way RPi hardware works, you simply put 3 files on a FAT partition of the SD card: start.elf, boot code.bin and kernel.img. The first two are stock files that have something to do with initializing the GPU and rest of the system before loading and executing kernel.img. The last file is simply a binary that loads at location 0x8000 and starts to run. I’ve run some canned experiments to blink the status LED and talk to the serial port on pins 14 and 15 of the GPIO connector.

Actually, the name of the file loaded and the location where it is placed can be changed, which will be important for my bootloader. Read https://github.com/cmarrin/placid/blob/master/bootloader/README.md for more info

My next step is to write a bootloader/debugger which will allow me to load my own binary images to run and control the system from the serial console.
