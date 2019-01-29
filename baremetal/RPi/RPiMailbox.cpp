/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "RPiMailbox.h"

#include "bare/Serial.h"
#include <cassert>

using namespace bare;

static volatile unsigned int *MAILBOX0READ = (unsigned int *) 0x2000b880;
static volatile unsigned int *MAILBOX0STATUS = (unsigned int *) 0x2000b898;
static volatile unsigned int *MAILBOX0WRITE = (unsigned int *) 0x2000b8a0;

static constexpr uint32_t MailboxResponse = 0x80000000;
static constexpr uint32_t GPUAddressAlias = 0x40000000;

#define MAILBOX_FULL 0x80000000
#define MAILBOX_EMPTY 0x40000000

static inline void dmb() { __asm volatile ("mcr p15, #0, %[zero], c7, c10, #5" : : [zero] "r" (0) ); }
static inline void flushcache() { __asm volatile ("mcr p15, #0, %[zero], c7, c14, #0" : : [zero] "r" (0) ); }

static inline uint32_t ARMaddrToGPUaddr(void* addr)
{
    return reinterpret_cast<uint32_t>(addr) | GPUAddressAlias;
}

// This code is from corn-mainline by Raspberry Alpha Omega

static uint32_t readMailbox(Mailbox::Channel channel)
{
    uint32_t count = 0;
    uint32_t data;

    // Loop until something is received from channel
    // If nothing recieved, it eventually give up and returns 0xffffffff
    while (1) {
        while (*MAILBOX0STATUS & MAILBOX_EMPTY) {
            /* Need to check if this is the right thing to do */
            flushcache();

            /* This is an arbritarily large number */
            if(count++ > 30000000) {
                return 0xffffffff;
            }
        }

        dmb();
        data = *MAILBOX0READ;
        dmb();

        if ((data & 15) == static_cast<uint32_t>(channel)) {
            return data & 0xFFFFFFF0;
        }
    }

    return 0;
}

static void writeMailbox(Mailbox::Channel channel, uint32_t* addr)
{
    // Addr must be 16 byte aligned
    assert((reinterpret_cast<uintptr_t>(addr) & 0xf) == 0);
    
    uint32_t data = ARMaddrToGPUaddr(addr);

    // Wait for mailbox to be not full
    while (*MAILBOX0STATUS & MAILBOX_FULL)     {
        // Need to check if this is the right thing to do
        flushcache();
    }

    dmb();
    *MAILBOX0WRITE = (data | static_cast<uint32_t>(channel));
}

Mailbox::Error Mailbox::getParameter(Command cmd, uint32_t* result, uint32_t size)
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
    buf[2] = static_cast<uint32_t>(cmd);
    buf[3] = size * sizeof(uint32_t); // value buffer size in bytes ((1 entry)*4)
    buf[4] = 0; // 1bit request/response (0/1), 31bit value length in bytes

    writeMailbox(Channel::Tags, buf);
    uint32_t mail = readMailbox(Channel::Tags);
    dmb();
    
    uint32_t* responseBuf = reinterpret_cast<uint32_t*>(mail);
    for (uint32_t i = 0; i < size; ++i) {
        result[i] = responseBuf[5 + i];
    }

    return Error::OK;
}

void Mailbox::printBoardParams()
{
    uint32_t responseBuf[2];
    getParameter(Mailbox::Command::GetFirmwareRev, responseBuf, 1);
    Serial::printf("FirmwareRev: %d\n", responseBuf[0]);
    
    getParameter(Mailbox::Command::GetBoardModel, responseBuf, 1);
    Serial::printf("BoardModel: %d\n", responseBuf[0]);
    
    getParameter(Mailbox::Command::GetBoardRev, responseBuf, 1);
    Serial::printf("BoardRev: %d\n", responseBuf[0]);
    
    getParameter(Mailbox::Command::GetBoardSerialNo, responseBuf, 2);
    Serial::printf("BoardSerialNo: %d, %d\n", responseBuf[0], responseBuf[1]);
    
    getParameter(Mailbox::Command::GetARMMemory, responseBuf, 2);
    Serial::printf("ARMMemory: start=0x%08x, size=0x%08x\n", responseBuf[0], responseBuf[1]);
    
    getParameter(Mailbox::Command::GetVCMemory, responseBuf, 2);
    Serial::printf("VCMemory: start=0x%08x, size=0x%08x\n", responseBuf[0], responseBuf[1]);
    
    getParameter(Mailbox::Command::GetDMAChannels, responseBuf, 1);
    Serial::printf("DMAChannelMask: 0x%08x\n", responseBuf[0]);
}

Mailbox::Error Mailbox::tagMessage(uint32_t* responseBuf, uint8_t size, ...)
{
    uint32_t __attribute__((aligned(16))) message[32];
    va_list list;
    va_start(list, size);
    
    // Set message size
    message[0] = (size + 3) * 4;
    
    // Set zero response
    message[1] = 0;
    
    // Add args to buffer
    for (int i = 0; i < size; i++) {
        message[2 + i] = va_arg(list, uint32_t);
    }
    
    // Set end pointer
    message[size + 2] = 0;
    
    va_end(list);
    
    writeMailbox(Channel::Tags, message);
    
    // Wait for response
    readMailbox(Channel::Tags);
    
    if (message[1] == MailboxResponse) {
        if (responseBuf) {
            for (int i = 0; i < size; i++) {
                responseBuf[i] = message[2 + i];
            }
        }
        return Error::OK;
    }
    return Error::InvalidResponse;

}
