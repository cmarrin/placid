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

.global _start
_start:
    bl main
    b  .
