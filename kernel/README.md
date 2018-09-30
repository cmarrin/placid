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

The ARM MMU is used to manage virtual memory. In the kernel translation table the memory map is as follows:

| **Translation Table Entry** | **Virtual Address**       | **Physical Address**     	| **Notes**                                               |
|-------------------------	  |-------------------------	|-------------------------	|-------------------------------------------------------	|
| `0`                      	  | `0 - 0x00100000`          | `0 - 0x00100000`          | Kernel                                                	|
| `1 - 2015`               	  | `0 - 0x7dffffff`          | `0 - 0x7dffffff`          | Bare physical memory access, Only available to kernel 	|
| `2016 - 2047`            	  | `0x7e000000 - 0x7fffffff` | `0x20000000 - 0x21ffffff` | Peripherals                                           	|
| `2048 - 4095`            	  | `0x80000000 - 0xffffffff` |                           | Unused                                                	|

**Kernel** **bare physical memory** and **peripherals** are only accessible to the kernel. Any attempt to 
access them from a user process will result in a memory protection fault and termination of the process.

### Kernel section

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

### User process memory

Each user processes has a 4GB virtual memory space. Each process has its own translation table with 4096 1MB 
section entries, taking up 16KB. Each section entry maps to a page table covering 256 small pages (4096 bytes each). 
Since translation and page tables need to reside in physical memory, allocation is simplified by always allocating
a 16KB block whether for the translation table or a set of page tables. So with each allocation 16 page tables are
created and assigned to 16 consecutive translation table entries. Initially each page is marked as inaccessible.
When an attempt is made to access that page a fault occurs. Again, to simplify allocation a 16KB block of memory 
is allocated and assigned to 4 consecutive page tables.

### Physical memory

Some Raspberry Pi boards have 512MB of physical RAM, others have 1GB. This memory is conceptually divided into 1MB
memory sections which can be mapped from virtual space using a translation table. The first 1MB physical block is
used by the kernel. It holds system space (interrupt vectors and ATAGS), several stacks, the kernel code/data, a 
kernel heap and the translation table (16KB) and a page table (4KB). The rest of the memory (511MB or 999MB) is 
split into 16K block and used to satisfy page request from user processes. On startup, these blocks are added
to a free list by linking them to one another by a pointer in the first 4 bytes of the block. When a block is 
allocated it is removed from the head of the free list and the head is pointed to the next block. When a block
is freed, it is added to the head of the list and the previous head is linked to from the freed block.


