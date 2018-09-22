
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

// The raspberry pi firmware at the time this was written defaults
// loading at address 0x8000.  Although this bootloader could easily
// load at 0x0000, it loads at 0x8000 so that the same binaries built
// for the SD card work with this bootloader.  Change the ARMBASE
// below to use a different location.

#include "sdcard.h"
#include "SDFS.h"
#include "bootutil.h"

using namespace placid;

#define ARMBASE 0x8000

extern "C" {

void PUT8 ( unsigned int, unsigned int );
void BRANCHTO ( unsigned int );

extern void uart_init ( void );
extern unsigned int uart_lcr ( void );
extern void uart_send ( unsigned int );
extern unsigned int uart_recv ( void );
extern void timer_init ( void );

unsigned long long __aeabi_uidivmod(unsigned int value, unsigned int divisor) {
        unsigned long long answer = 0;

		unsigned int i;
        for (i = 0; i < 32; i++) {
                if ((divisor << (31 - i)) >> (31 - i) == divisor) {
                        if (value >= divisor << (31 - i)) {
                                value -= divisor << (31 - i);
                                answer |= (unsigned long long)(1 << (31 - i));
                                if (value == 0) break;
                        } 
                }
        }

        answer |= (unsigned long long)value << 32;
        return answer;
};

unsigned int __aeabi_uidiv(unsigned int value, unsigned int divisor) {
        return (unsigned int)__aeabi_uidivmod(value, divisor);
};

}

//------------------------------------------------------------------------
unsigned char xstring[256];
//------------------------------------------------------------------------

static void autoload()
{
    // FIXME: implement
    printf("\n\nautoload...\n\n");
    printf("mounting FS\n");
    SDFS fs;
    SDFS::mount(fs, 0, 0);
    
    printf("opening hello.txt\n");
    File fp;
    bool r = SDFS::open(fs, fp, "/hello.txt", "r");
    printf("file open returned %s\n", r ? "true" : "false");
    if (!r) {
        printf("*** File open error:%d\n", File::error(fp));
    } else {
        // Read sector
        char buf[512];
        int32_t size = File::read(fp, buf, 16);
        printf("file read returned %d\n", size);
        if (size != 16) {
            printf("*** File read error:%d\n", File::error(fp));
        } else {
            buf[15] = '\0';
            printf("Read returned '%s'\n", buf);
        }
    }
    
    while(1) { }
}

extern "C" int notmain ( void )
{
    unsigned int ra;
    //unsigned int rb;
    unsigned int rx;
    unsigned int addr;
    unsigned int block;
    unsigned int state;

    unsigned int crc;

    uart_init();
    putstr("\n\nPlacid Bootloader v0.1\n\n");
    putstr("Autoloading in 5 seconds\n");
    putstr("    (press [space] to autoload immediately or [return] for XMODEM upload)\n");
    
    putstr("***** n = ");
    putu(1234);
    putstr("\n");

    timer_init();
    
    rx = timerTick();
    ra = 1000000;
    uart_send('.');

    while (1) {
        if (timerTick() - rx > ra) {
            uart_send('.');
            ra += 1000000;
            if (ra++ >= 5000000) {
                autoload();
            }
        }
            
        if ((uart_lcr() & 0x01) == 0) {
            continue;
        }
        int c = uart_recv();
        if (c == ' ') {
            // autoload
            autoload();
        } else if (c == '\n') {
            break;
        }
    }
    
    putstr("Start XMODEM upload when ready...\n\n");

//SOH 0x01
//ACK 0x06
//NAK 0x15
//EOT 0x04

//block numbers start with 1

//132 byte packet
//starts with SOH
//block number byte
//255-block number
//128 bytes of data
//checksum byte (whole packet)
//a single EOT instead of SOH when done, send an ACK on it too

    block=1;
    addr=ARMBASE;
    state=0;
    crc=0;
    rx=timerTick();
    while(1)
    {
        ra=timerTick();
        if((ra-rx)>=4000000)
        {
            uart_send(0x15);
            rx+=4000000;
        }
        if((uart_lcr()&0x01)==0) continue;
        xstring[state]=uart_recv();
        rx=timerTick();
        if(state==0)
        {
            if(xstring[state]==0x04)
            {
                uart_send(0x06);
                BRANCHTO(ARMBASE);
                break;
            }
        }
        switch(state)
        {
            case 0:
            {
                if(xstring[state]==0x01)
                {
                    crc=xstring[state];
                    state++;
                }
                else
                {
                    //state=0;
                    uart_send(0x15);
                }
                break;
            }
            case 1:
            {
                if(xstring[state]==block)
                {
                    crc+=xstring[state];
                    state++;
                }
                else
                {
                    state=0;
                    uart_send(0x15);
                }
                break;
            }
            case 2:
            {
                if(xstring[state]==(0xFF-xstring[state-1]))
                {
                    crc+=xstring[state];
                    state++;
                }
                else
                {
                    uart_send(0x15);
                    state=0;
                }
                break;
            }
            case 131:
            {
                crc&=0xFF;
                if(xstring[state]==crc)
                {
                    for(ra=0;ra<128;ra++)
                    {
                        PUT8(addr++,xstring[ra+3]);
                    }
                    uart_send(0x06);
                    block=(block+1)&0xFF;
                }
                else
                {
                    uart_send(0x15);
                }
                state=0;
                break;
            }
            default:
            {
                crc+=xstring[state];
                state++;
                break;
            }
        }
    }
    return(0);
}
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
//
// Copyright (c) 2012 David Welch dwelch@dwelch.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------
