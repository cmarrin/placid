/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "elf.h"
#include "FileSystem.h"
#include <memory>

namespace placid {
    // ELFLoader
    //
    // Loads ELF file and provides executable
    //
    class ELFLoader
    {
    public:
        enum class Error {
            OK,
            FileOpen,
            BadMagic, BadClass, BadEndianess, BadABI, NotExecutable,
            BadEHeaderSize, BadSHeaderSize, BadPHeaderSize,
            InvalidSHeaderOffset, InvalidSStringOffset, StringReadFailure,
        };
        
        ELFLoader(const char* name);
    
    private:
        bool readSectionHeader(Elf32_Shdr*, uint32_t index);
        bool readSectionName(char* name, uint32_t size, uint32_t offset);
        
        uint32_t sectionOffset(uint32_t i) { return _sectionOffset + i * sizeof(Elf32_Shdr); }

        
        std::unique_ptr<File> _fp;
        
        uint32_t _entryPoint;
        uint32_t _sectionOffset;
        uint32_t _stringSectionOffset;
        uint16_t _sectionCount;
        
        Error _error;
    };

}



































///****************************************************************************
// * ARMv7M ELF loader
// * Copyright (c) 2013-2015 Martin Ribelotta
// * All rights reserved.
// *
// * Redistribution and use in source and binary forms, with or without
// * modification, are permitted provided that the following conditions
// * are met:
// * 1. Redistributions of source code must retain the above copyright
// *    notice, this list of conditions and the following disclaimer.
// * 2. Redistributions in binary form must reproduce the above copyright
// *    notice, this list of conditions and the following disclaimer in the
// *    documentation and/or other materials provided with the distribution.
// * 3. Neither the name of copyright holders nor the names of its
// *    contributors may be used to endorse or promote products derived
// *    from this software without specific prior written permission.
// *
// * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
// * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// * POSSIBILITY OF SUCH DAMAGE.
// *****************************************************************************/
//
//#ifndef LOADER_H_
//#define LOADER_H_
//
//#ifdef __cplusplus__
//extern "C" {
//#endif
//
///**
// * @defgroup elf_loader ELF Loader
// * @{
// */
//
///**
// * Protection flags of memory
// */
//typedef enum {
//  ELF_SEC_WRITE = 0x1, /*!< Enable for write */
//  ELF_SEC_READ = 0x2, /*!< Enable for read */
//  ELF_SEC_EXEC = 0x4, /*!< Enable for execution (instruction fetch) */
//} ELFSecPerm_t;
//
///**
// * Exported symbol struct
// */
//typedef struct {
//  const char *name; /*!< Name of symbol */
//  void *ptr; /*!< Pointer of symbol in memory */
//} ELFSymbol_t;
//
///**
// * Environment for execution
// */
//typedef struct {
//  const ELFSymbol_t *exported; /*!< Pointer to exported symbols array */
//  unsigned int exported_size; /*!< Elements on exported symbol array */
//} ELFEnv_t;
//
///**
// * Execute ELF file from "path" with environment "env"
// * @param path Path to file to load
// * @param env Pointer to environment struct
// * @retval 0 On successful
// * @retval -1 On fail
// * @todo Error information
// */
//extern int exec_elf(const char *path, const ELFEnv_t *env);
//
///** @} */
//
//#ifdef __cplusplus__
//}
//#endif
//
//#endif /* LOADER_H_ */
