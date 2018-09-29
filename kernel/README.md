# The Placid Kernel

## Features

- Microkernel. Drivers, system libraries all live outside kernel space
- Multiprocessing. Mutiple user processes with protected memory spaces
- Debug console on UART1
- FAT32 SD card driver for loading higher level drivers
- GPIO access
- System time tick. int64_t value. Microseconds since January 1st, 2001, 00:00:00 UCT
- Event system
- Timer. One shot and repeating. Microsecond accuracy, generates Events
- Interrupt management. Raw interrupts come into kernel, generate Events. Driver can request interrupt service
- Mailbox for communication with VideoCore and system features

## Memory management

ARM MMU is used to manage virtual memory. Memory map is as follows:

| **Translation Table Entry** | **Virtual Address**       | **Physical Address**     	| **Notes**                                                 	|
|-------------------------	  |-------------------------	|-------------------------	|-------------------------------------------------------	|
| `0`                      	  | `0 - 0x00100000`          | `0 - 0x00100000`          | Kernel                                                	|
| `1 - 2015`               	  | `0 - 0x7dffffff`          | `0 - 0x7dffffff`          | Bare phycical memory access, Only available to kernel 	|
| `2016 - 2047`            	  | `0x7e000000 - 0x7fffffff` | `0x20000000 - 0x21ffffff` | Peripherals                                           	|
| `2048 - 4095`            	  | `0x80000000 - 0xffffffff` | `0 - 0x7ffffff`           | User Process Memory                                   	|

**Kernel** **bare physical memory** and **peripherals** are only accessible to the kernel. Any attempt to 
access them from a user process will result in a memory protection fault and termination of the process.

# Kernel section

The first translation table entry is reserved for the kernel. It maps the first 1MB of physical memory 
to address `0-0x00100000`. This section has a 256 byte small page table. Each 4096 byte page is mapped
to the corresponding page of physical memory with the appropriate permission bits set:

| Page      	| Address            	| Purpose          	| Permissions                                     	    | Notes                                       	|
|-----------	|--------------------	|------------------	|-------------------------------------------------      |---------------------------------------------	|
| `0`       	| `0-0xfff`          	| Reserved         	| `rwx`                                           	    | System use (interrupt vectors, ATAGS, etc.) 	|
| `1`       	| `0x1000 - 0x1fff`  	| Interrupt stack  	| `rw-`                                           	    |                                             	|
| `2`       	| `0x2000 - 0x2fff`  	| Page fault stack 	| `rw-`                                           	    |                                             	|
| `3`       	| `0x3000 - 0x3fff`  	| Reserved stack   	| `rw-`                                           	    | For future stack use                        	|
| `4 - 7`   	| `0x4000 - 0x7fff`  	| Kernel stack     	| `rw-`                                           	    |                                             	|
| `8 - 255` 	| `0x8000 - 0xfffff` 	| Kernel code/data 	| `.code (--x), .rodata (r--), .bss (rw-), heap (rw-)` 	| Each segment takes up as many pages as needed |
