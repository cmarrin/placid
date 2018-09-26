/*
  This program is based on the information on 
  https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
*/

#include "Mailbox.h"

#include "util.h"

static volatile unsigned int *MAILBOX0READ = (unsigned int *) 0x2000b880;
static volatile unsigned int *MAILBOX0STATUS = (unsigned int *) 0x2000b898;
static volatile unsigned int *MAILBOX0WRITE = (unsigned int *) 0x2000b8a0;

#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000

// This code is from corn-mainline by Raspberry Alpha Omega

static uint32_t readmailbox(uint32_t channel) {
  uint32_t count = 0;
  uint32_t data;

    if ((channel & 0xFFFFFFF0) != 0) {
        printf("Channel %d is wrong\n", (int) channel);
        return -1;
    }

    /* Loop until something is received from channel
     * If nothing recieved, it eventually give up and returns 0xffffffff
     */
    while(1) {
        while (*MAILBOX0STATUS & MAILBOX_EMPTY) {
            /* Need to check if this is the right thing to do */
            flushcache();

            /* This is an arbritarily large number */
            if(count++ >(1<<25)) {
                return 0xffffffff;
            }
        }

        /* Read the data
         * Data memory barriers as we've switched peripheral
         */
        dmb();
        data = *MAILBOX0READ;
        dmb();

        if ((data & 15) == channel)
            return data & 0xFFFFFFF0;
    }

    return 0;
}

static void writemailbox(uint32_t channel, uint32_t data)
{
    if ((data & 0x0000000F) != 0) {
        return;
    }
    if ((channel & 0xFFFFFFF0) != 0) {
        return;
    }

    // Wait for mailbox to be not full
    while (*MAILBOX0STATUS & MAILBOX_FULL)     {
        // Need to check if this is the right thing to do
        flushcache();
    }

    dmb();
    *MAILBOX0WRITE = (data | channel);
}

Mailbox::Error Mailbox::getParameter(Param param, uint32_t* result, uint32_t size)
{
    static constexpr uint32_t BufferPadding = 6;
    static constexpr uint32_t MaxSize = 256 - BufferPadding;
    if (size > MaxSize) {
        return Error::SizeTooLarge;
    }
    
    uint32_t buf[MaxSize + BufferPadding] __attribute__((aligned(16)));
    memset(buf, 0, sizeof(buf));

    buf[0] = (size + BufferPadding) * sizeof(uint32_t);
    buf[1] = 0; // Request code: process request
    buf[2] = static_cast<uint32_t>(param);
    buf[3] = size * sizeof(uint32_t); // value buffer size in bytes ((1 entry)*4)
    buf[4] = 0; // 1bit request/response (0/1), 31bit value length in bytes

    uint32_t addr = static_cast<uint32_t>((reinterpret_cast<uintptr_t>(buf)) + 0x40000000);
    writemailbox(8, addr);
    uint32_t mail = readmailbox(8);
    dmb();
    
#ifdef __APPLE__
(void) mail;
#else
    uint32_t* responseBuf = reinterpret_cast<uint32_t*>(mail);
    for (uint32_t i = 0; i < size; ++i) {
        result[i] = responseBuf[5 + i];
    }
#endif

    return Error::OK;
}
