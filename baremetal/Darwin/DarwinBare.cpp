/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include <stdio.h>
#include <sys/time.h>
#include <unicorn/unicorn.h>

using namespace bare;

static bool inited = false;

void bare::initSystem()
{
    inited = true;
}

bool bare::useAllocator()
{
    return inited;
}

static void hook_block(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    printf(">>> Tracing basic block at 0x%lld, block size = 0x%x\n", address, size);
}

static void hook_code(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    printf(">>> Tracing instruction at 0x%lld, instruction size = 0x%x\n", address, size);
}

// memory address where emulation starts
#define ADDRESS 0x10000

void bare::runCode(void* memory, uint32_t size, uint32_t startOffset)
{
    uc_engine *uc;
    uc_err err;
    uc_hook trace1, trace2;

    int r0 = 0x1234;     // R0 register
    int r2 = 0x6789;     // R1 register
    int r3 = 0x3333;     // R2 register
    int r1;     // R1 register

    printf("Emulate ARM code\n");

    // Initialize emulator in ARM mode
    err = uc_open(UC_ARCH_ARM, UC_MODE_ARM, &uc);
    if (err) {
        printf("Failed on uc_open() with error returned: %u (%s)\n", err, uc_strerror(err));
        return;
    }

    // map 2MB memory for this emulation
    uc_mem_map(uc, ADDRESS, size, UC_PROT_ALL);

    // write machine code to be emulated to memory
    uc_mem_write(uc, ADDRESS, memory, size);

    // initialize machine registers
    uc_reg_write(uc, UC_ARM_REG_R0, &r0);
    uc_reg_write(uc, UC_ARM_REG_R2, &r2);
    uc_reg_write(uc, UC_ARM_REG_R3, &r3);

    // tracing all basic blocks with customized callback
    uc_hook_add(uc, &trace1, UC_HOOK_BLOCK, reinterpret_cast<void*>(hook_block), NULL, 1, 0);

    // tracing one instruction at ADDRESS with customized callback
    uc_hook_add(uc, &trace2, UC_HOOK_CODE, reinterpret_cast<void*>(hook_code), NULL, ADDRESS, ADDRESS);

    // emulate machine code in infinite time (last param = 0), or when
    // finishing all the code.
    err = uc_emu_start(uc, ADDRESS + startOffset, ADDRESS + size, 0, 0);
    if (err) {
        printf("Failed on uc_emu_start() with error returned: %u\n", err);
    }

    // now print out some registers
    printf(">>> Emulation done. Below is the CPU context\n");

    uc_reg_read(uc, UC_ARM_REG_R0, &r0);
    uc_reg_read(uc, UC_ARM_REG_R1, &r1);
    printf(">>> R0 = 0x%x\n", r0);
    printf(">>> R1 = 0x%x\n", r1);

    uc_close(uc);
}

// Setup a dummy area of "kernel space" to dump data to
uint8_t _dummyKernel[8 * 1024 * 1024];

extern "C" {

uint8_t* kernelBase() { return _dummyKernel; }

void PUT8(uint8_t* addr, uint8_t value)
{
    printf("PUT8:[0x%p] <= %#02x\n", addr, value);
}

void BRANCHTO(uint8_t* addr)
{
    printf("BRANCHTO: => 0x%p\n", addr);
    while (1) ;
}

bool interruptsSupported()
{
    return false;
}

void restart()
{
    printf("RESTART\n");
    while (1) ;
}

}
