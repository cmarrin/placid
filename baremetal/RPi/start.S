/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Adapted from vectors.S from David Welch
// Copyright (c) 2012 David Welch dwelch@dwelch.com

#include "RPi/MemoryMap.h"

   .sect    .init

    // Vector table
    // Vector table
    ldr pc, resetAddr
    ldr pc, undefinedInstructionAddr
    ldr pc, swiAddr
    ldr pc, prefetchAbortAddr
    ldr pc, dataAbortAddr
    ldr pc, unusedAddr
    ldr pc, irqAddr
    ldr pc, fiqAddr
    
resetAddr:                  .word restart
undefinedInstructionAddr:   .word undefinedInstructionStub
swiAddr:                    .word swiStub
prefetchAbortAddr:          .word prefetchAbortStub
dataAbortAddr:              .word dataAbortStub
unusedAddr:                 .word unusedStub
irqAddr:                    .word irqStub
fiqAddr:                    .word fiqStub

.global restart
restart:
    // The vector table (above) is at the starting
    // location of the loaded binary. The CPU expects them to
    // be at 0x0000, so we need to move them. There are actually
    // 16 words to move (the instructions moving an address into
    // the pc and the addresses themselves. So we move them in
    // 2 sets of 8.
    mov r0,#0x8000
    mov r1,#0x0000
    ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}
    ldmia r0!,{r2,r3,r4,r5,r6,r7,r8,r9}
    stmia r1!,{r2,r3,r4,r5,r6,r7,r8,r9}

    // enable fpu
    mrc p15, 0, r0, c1, c0, 2
    orr r0,r0,#0x300000 ;@ single precision
    orr r0,r0,#0xC00000 ;@ double precision
    mcr p15, 0, r0, c1, c0, 2
    mov r0,#0x40000000
    fmxr fpexc,r0

    // For all stack setting set the IRQ and FIQ disable bits
    // Set the FIQ stack
    mov r0, #0xd1
    msr cpsr_c,r0
    ldr sp, =_FIQStack

    // Set the IRQ stack
    mov r0, #0xd2
    msr cpsr_c,r0
    ldr sp, =_IRQStack

    // Set the SVC stack
    mov r0, #0xd3
    msr cpsr_c,r0
    ldr sp, =_SVCStack

    // Set the Abort stack
    mov r0, #0xd7
    msr cpsr_c,r0
    ldr sp, =_AbortStack

    // Set the Undefined stack
    mov r0, #0xdb
    msr cpsr_c,r0
    ldr sp, =_UndefinedStack

    // Set the System stack
    mov r0, #0xdf
    msr cpsr_c,r0
    ldr sp, =_SystemStack

    bl main

// Interrupt handlers
    .macro    stub name, exception, pc_offset

\name:
    sub     lr, lr, #\pc_offset /* lr: correct PC of aborted program */
    stmfd   sp!, {lr}           /* store PC onto stack */
    mrs     lr, spsr            /* lr can be overwritten now */
    stmfd   sp!, {lr}           /* store saved PSR onto stack */
    stmfd   sp, {r0-r14}^       /* store user registers r0-r14 (unbanked) */
    sub     sp, sp, #4*15       /* correct stack (not done by previous instruction */
    mov     r1, sp              /* save sp_abt or sp_und */
    cps     #0x12               /* set IRQ mode to access sp_irq and lr_irq */
    mov     r2, sp
    mov     r3, lr
    cps     #0x1F               /* our abort handler runs in system mode */
    mov     sp, r1              /* set sp_sys to stack top of abort stack */
    stmfd   sp!, {r2, r3}       /* store lr_irq and sp_irq onto stack */
    mov     r1, sp              /* r1: pointer to register frame */
    mov     r0, #\exception     /* r0: exception identifier */
    b       handleException     /* jump to ExceptionHandler (never returns) */

    .endm

stub undefinedInstructionInternal, EXCEPTION_UNDEFINED_INSTRUCTION, 4
stub prefetchAbortStub, EXCEPTION_PREFETCH_ABORT, 4
stub dataAbortStub, EXCEPTION_DATA_ABORT, 8
stub unusedStub, EXCEPTION_UNKNOWN, 0
stub fiqStub, EXCEPTION_FIQ, 0

undefinedInstructionStub:
    stmfd    sp!, {r0, lr}
    fmrx    r0, fpexc                   /* check for floating point exception */
#define VFP_FPEXC_EX    (1 << 31)
    tst    r0, #VFP_FPEXC_EX            /* if EX bit is clear in FPEXC */
    beq    undefinedInstructionInternal /* then jump to abort stub */
    bic    r0, r0, #VFP_FPEXC_EX        /* else clear EX bit */
    fmxr    fpexc, r0
    ldmfd    sp!, {r0, pc}^            /* restore registers and return */

swiStub:
    // r0 contains call id, r1-r3 contain generic params
	stmfd sp!, {r4-r5,lr}
	bl handleSWI
	ldmfd sp!, {r4, r5, pc}^

irqStub:
    sub    lr, lr, #4
    stmfd sp!, {r0-r3, r12, lr}
    bl handleIRQ
    ldmfd sp!, {r0-r3, r12, pc}^

.global BRANCHTO
BRANCHTO:
    bx r0

.global PUT8
PUT8:
    strb r1,[r0]
    bx lr
