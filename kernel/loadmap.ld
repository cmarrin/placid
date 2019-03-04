/*-------------------------------------------------------------------------

This source file is a part of Placid

For the latest info, see http://www.marrin.org/

Copyright (c) 2018-2019, Chris Marrin
All rights reserved.

Use of this source code is governed by the MIT license that can be
found in the LICENSE file.

-------------------------------------------------------------------------*/

ENTRY(restart)

SECTIONS
{
    .init : {
        *(.init)
    }

    .text : {
        *(.text*)

        _etext = .;
    }

    .rodata : {
        *(.rodata*)
    }

    .init_array : {
        __init_start = .;

        *(.init_array*)

        __init_end = .;
    }

    .ARM.exidx : {
        __exidx_start = .;

        *(.ARM.exidx*)

        __exidx_end = .;
    }

    .data : {
        *(.data*)
    }

    .bss : {
        __bss_start = .;

        *(.bss*)
        *(COMMON)

        _end = .;
    }
}
