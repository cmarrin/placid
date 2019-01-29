/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include <stdint.h>

namespace bare {

    class Mailbox
    {
    public:
        enum class Error {
            OK,
            SizeTooLarge,
            InvalidResponse,
        };
        
        enum class Channel {
            Power = 0x0,
            FB = 0x1,
            VirtualUART = 0x2,
            VCHIQ = 0x3,
            LEDs = 0x4,
            Buttons = 0x5,
            Touch = 0x6,
            Counter = 0x7,
            Tags = 0x8,
            GPU = 0x9,
        };

        enum class Command {
            // Videocore info commands
            GetFirmwareRev          = 0x00000001,   // < uint32_t rev
            GetBoardModel           = 0x00010001,   // < uint32_t model
            GetBoardRev             = 0x00010002,   // < uint32_t boardRev
            GetMACAddress           = 0x00010003,   // < uint8_t addr[6]
            GetBoardSerialNo        = 0x00010004,   // < uint32_t ser[2]
            GetARMMemory            = 0x00010005,   // < uint32_t base, uint32_t size
            GetVCMemory             = 0x00010006,   // < uint32_t base, uint32_t size
            GetClocks               = 0x00010007,   // < { uint32_t parentId, uint32_t id }+

            // Power commands
            GetPowerState           = 0x00020001,   // 
            GetTiming               = 0x00020002,   // 
            SetPowerState           = 0x00028001,   // 

            // GPIO commands
            GetGPIOState            = 0x00030041,   // 
            SetGPIOState            = 0x00038041,   // 

            // Clock commands
            GetClockState           = 0x00030001,   // 
            GetClockRate            = 0x00030002,   // 
            GetMaxClockRate         = 0x00030004,   // 
            GetMinClockRate         = 0x00030007,   // 
            GetTurbo                = 0x00030009,   // 

            SetClockState           = 0x00038001,   // 
            SetClockRate            = 0x00038002,   // 
            SetTurbo                = 0x00038009,   // 

            // Voltage commands
            GetVoltage              = 0x00030003,   // 
            GetMaxVoltage           = 0x00030005,   // 
            GetMinVoltage           = 0x00030008,   // 

            SetVoltage              = 0x00038003,   // 

            // Temperature commands
            GetTemperature          = 0x00030006,   // 
            GetMaxTemperature       = 0x0003000A,   // 

            // Memory commands
            AllocateMemory          = 0x0003000C,   // 
            LockMemory              = 0x0003000D,   // 
            UnlockMemory            = 0x0003000E,   // 
            ReleaseMemory           = 0x0003000F,   // 
                                                                   
            // Execute code commands
            ExecuteCode             = 0x00030010,   // 

            // QPU control commands
            ExecuteQPU              = 0x00030011,   // 
            EnableQPU               = 0x00030012,   // 

            // Displaymax commands
            GetDispmanxHandle       = 0x00030014,   // 
            GetEDIDBlock            = 0x00030020,   // 

            // SD Card commands
            GetSDHostClock          = 0x00030042,   // 
            SetSDHostClock          = 0x00038042,   // 

            // Framebuffer commands
            AllocateFrameBuffer     = 0x00040001,   // 
            BlankScreen             = 0x00040002,   // 
            GetPhysicalWidthHeight  = 0x00040003,   // 
            GetVirtualWidthHeight   = 0x00040004,   // 
            GetColorDepth           = 0x00040005,   // 
            GetPixelOrder           = 0x00040006,   // 
            GetAlphaMode            = 0x00040007,   // 
            GetPitch                = 0x00040008,   // 
            GetVirtualOffset        = 0x00040009,   // 
            GetOverscan             = 0x0004000A,   // 
            GetPalette              = 0x0004000B,   // 

            ReleaseFramebuffer      = 0x00048001,   // 
            SetPhysicalWidthHeight  = 0x00048003,   // 
            SetVirtualWidthHeight   = 0x00048004,   // 
            SetColorDepth           = 0x00048005,   // 
            SetPixelOrder           = 0x00048006,   // 
            SetAlphaMode            = 0x00048007,   // 
            SetVirtualOffset        = 0x00048009,   // 
            SetOverscan             = 0x0004800A,   // 
            SetPalette              = 0x0004800B,   // 
            SetVSync                = 0x0004800E,   // 
            SetBacklight            = 0x0004800F,   // 

            // VCHIQ commands
            VCHIQInit               = 0x00048010,   // 

            // Config commands
            GetCommandLine          = 0x00050001,   //  

            // Shared resource management commands
            GetDMAChannels          = 0x00060001,   // uint32_t mask

            /* Cursor commands */
            SetCursorInfo           = 0x00008010,   // 
            SetCursorState          = 0x00008011,   // 
        };
        
        enum class ClockId {
            EMMC    = 0x1, 
            UART    = 0x2,
            ARM     = 0x3,
            Core    = 0x4,
            V3D     = 0x5,
            H264    = 0x6,
            ISP     = 0x7,
            SDRAM   = 0x8,
            Pixel   = 0x9,
            PWM     = 0xA,
        };
        
        static Error getParameter(Command, uint32_t* result, uint32_t size);
        static Error tagMessage(uint32_t* responseBuf, uint8_t size, ...);
        static void printBoardParams();
    };

}
