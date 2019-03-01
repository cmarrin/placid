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

#include "RPi/asmInterface.h"

   .sect    .init

.global _start
.global restart
_start:
restart:
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
    ldr sp, =_AbortStack

    // Set the System stack
    mov r0, #0xdf
    msr cpsr_c,r0
    ldr sp, =_SystemStack

    bl main

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
