#pragma once

#include <stdint.h>

class Mailbox
{
public:
    enum class Error {
        OK,
        SizeTooLarge,
    };
    
    enum class Param {
        FirmwareRev = 0x00000001,   // uint32_t rev
        BoardModel = 0x00010001,    // uint32_t model
        BoardRev = 0x00010002,      // uint32_t boardRev
        MACAddress = 0x00010003,    // uint8_t addr[6]
        BoardSerialNo = 0x00010004, // uint32_t ser[2]
        ARMMemory = 0x00010005,     // uint32_t base, uint32_t size
        VCMemory = 0x00010006,      // uint32_t base, uint32_t size
        DMAChannels = 0x00060001,   // uint32_t nchannels
    };
    
    static Error getParameter(Param, uint32_t* result, uint32_t size);
};
