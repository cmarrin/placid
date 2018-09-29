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

| Translation Table Entry     | Page Table Entry     | Virtual Address             | Physical Address            | Notes                                                     |
|-------------------------    |------------------    |-------------------------    |-------------------------    |-------------------------------------------------------    |
| 0 - 2015                    |                      | 0 - 0x7e000000              | 0 - 0x7e000000              | Bare phycical memory access, Only available to kernel     |
| 2016 - 2047                 |                      | 0x7e000000 - 0x80000000     | 0x20000000 - 0x22000000     | Peripherals                                               |
|                             |                      |                             |                             |                                                           |

