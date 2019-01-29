/*-------------------------------------------------------------------------
This source file is a part of Placid

For the latest info, see http://www.marrin.org/

Copyright (c) 2018, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

// Adapted from vectors.S from David Welch
// Copyright (c) 2012 David Welch dwelch@dwelch.com

#include "RPi/asmInterface.h"

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

    .sect    .init

.global _start
.global reboot
_start:
reboot:
    // Vector table
    ldr pc,resetHandler
    ldr pc,undefinedInstruction
    ldr pc,swi
    ldr pc,prefetchAbort
    ldr pc,dataAbort
    ldr pc,unused
    ldr pc,irq
    ldr pc,fiq
    
resetHandler:           .word reset
undefinedInstruction:   .word undefinedInstructionStub
swi:                    .word swiStub
prefetchAbort:          .word prefetchAbortStub
dataAbort:              .word dataAbortStub
unused:                 .word unusedStub
irq:                    .word irqStub
fiq:                    .word fiqStub

reset:

	// enable fpu
    mrc p15, 0, r0, c1, c0, 2
    orr r0,r0,#0x300000 ;@ single precision
    orr r0,r0,#0xC00000 ;@ double precision
    mcr p15, 0, r0, c1, c0, 2
    mov r0,#0x40000000
    fmxr fpexc,r0

    // The vector table (above) is at 0x8000, the starting
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


    // The stack addresses here are temporary and will get reset when the memory system starts up
    // Set IRQ mode with FIQ and IRQ disabled and set the IRQ stack
    mov r0,#0xD2
    msr cpsr_c,r0
    mov sp,#_IRQStack

    // Set FIQ mode with FIQ and IRQ disabled and set the FIQ stack
    mov r0,#0xD1
    msr cpsr_c,r0
    mov sp,#_FIQStack

    // Set Abort mode with FIQ and IRQ disabled and set the Abort stack
    mov r0,#0xD7
    msr cpsr_c,r0
    mov sp,#_AbortStack

    // Set supervisor mode with FIQ and IRQ disabled and set the supervisor stack
    mov r0,#0xD3
    msr cpsr_c,r0
    mov sp,#_SVCStack

    // Set supervisor mode with IRQ enabled and FIQ disabled
    ;@ SVC MODE, IRQ ENABLED, FIQ DIS
    ;@mov r0,#0x53
    ;@msr cpsr_c, r0

    bl main

stub undefinedInstructionInternal, EXCEPTION_UNDEFINED_INSTRUCTION, 4
stub prefetchAbortStub, EXCEPTION_PREFETCH_ABORT, 4
stub dataAbortStub, EXCEPTION_DATA_ABORT, 8

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
    bl handleSWI

unusedStub:
    bl handleUnused

irqStub:
    sub    lr, lr, #4
    stmfd sp!, {r0-r12, lr}
    bl handleIRQ
    ldmfd sp!, {r0-r12, pc}^

fiqStub:
    sub    lr, lr, #4
    stmfd sp!, {r0-r12, lr}
    bl handleFIQ
    ldmfd sp!, {r0-r12, pc}^

.global BRANCHTO
BRANCHTO:
    bx r0

.global PUT8
PUT8:
    strb r1,[r0]
    bx lr

.globl interruptsSupported
interruptsSupported:
    mov r0, #1
    bx lr
