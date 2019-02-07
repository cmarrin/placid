/****************************************************************************
 * ARMv7M ELF loader
 * Copyright (c) 2013-2015 Martin Ribelotta 
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#ifndef LOADER_CONFIG_H_
#define LOADER_CONFIG_H_

#ifndef DOX

//#if 0
//#define LOADER_FD_T FILE *
//#define LOADER_OPEN_FOR_RD(path) fopen(path, "rb")
//#define LOADER_FD_VALID(fd) (fd != NULL)
//#define LOADER_READ(fd, buffer, size) fread(buffer, 1, size, fd)
//#define LOADER_WRITE(fd, buffer, size) fwrite(buffer, 1, size, fd)
//#define LOADER_CLOSE(fd) fclose(fd)
//#define LOADER_SEEK_FROM_START(fd, off) fseek(fd, off, SEEK_SET)
//#define LOADER_TELL(fd) ftell(fd)
//#else
//#define LOADER_FD_T int
//#define LOADER_OPEN_FOR_RD(path) open(path, O_RDONLY)
//#define LOADER_FD_VALID(fd) (fd != -1)
//#define LOADER_READ(fd, buffer, size) read(fd, buffer, size)
//#define LOADER_WRITE(fd, buffer, size) write(fd, buffer, size)
//#define LOADER_CLOSE(fd) close(fd)
//#define LOADER_SEEK_FROM_START(fd, off) (lseek(fd, off, SEEK_SET) == -1)
//#define LOADER_TELL(fd) lseek(fd, 0, SEEK_CUR)
//#endif

//#define LOADER_TELL(fd) lseek(fd, 0, SEEK_CUR)
//
//extern void *do_alloc(size_t size, size_t align, ELFSecPerm_t perm);
//
//#define LOADER_ALIGN_ALLOC(size, align, perm) do_alloc(size, align, perm)
//
//extern int is_streq(const char *s1, const char *s2);
//
//#define LOADER_FREE(ptr) free(ptr)
//#define LOADER_CLEAR(ptr, size) memset(ptr, 0, size)
//#define LOADER_STREQ(s1, s2) (is_streq(s1, s2))
//
//#define DBG(...) printf("ELF: " __VA_ARGS__)
//#define ERR(msg) do { perror("ELF: " msg); __asm__ volatile ("bkpt"); } while(0)
//#define MSG(msg) puts("ELF: " msg)

// CFM: Configuration for Placid
#include "bare.h"

#include "bare/Serial.h"
#include "FileSystem.h"

#define LOADER_FD_T placid::File*
#define LOADER_OPEN_FOR_RD(path) placid::FileSystem::sharedFileSystem()->open(path, placid::FileSystem::OpenMode::Read)
#define LOADER_FD_VALID(fd) (fd != nullptr)
#define LOADER_READ(fd, buffer, size) fd->read(reinterpret_cast<char*>(buffer), size)
#define LOADER_WRITE(fd, buffer, size) fd->write(reinterpret_cast<char*>(buffer), size)
#define LOADER_CLOSE(fd) delete fd
#define LOADER_SEEK_FROM_START(fd, off) fd->seek(off, placid::File::SeekWhence::Set)
#define LOADER_TELL(fd) fd->tell()
#define LOADER_ALIGN_ALLOC(size, align, perm) bare::aligned_alloc(align, size)
#define LOADER_FREE(ptr) bare::aligned_free(ptr)
#define LOADER_CLEAR(ptr, size) memset(ptr, 0, size)
#define LOADER_STREQ(s1, s2) (strcmp(s1, s2) == 0)

#define DBG(...) bare::Serial::printf("ELF: " __VA_ARGS__)
#define ERR(msg) do { perror("ELF: " msg); } while(0)
#define MSG(msg) bare::Serial::puts("ELF: " msg)

#else

/**
 * @defgroup Configuration Configuration macros
 *
 * This macros defines the access function to various internal function of
 * elf loader
 *
 * You need to define this macros to compile and use the elf loader library
 * in your system
 *
 * @{
 */

/**
 * File handler descriptor type macro
 *
 * This define the file handler declaration type
 */
#define LOADER_FD_T

/**
 * Open for read function macro
 *
 * This macro define the function name and convention call to open file for
 * read. This need to return #LOADER_FD_T type
 *
 * @param path Path to file for open
 */
#define LOADER_OPEN_FOR_RD(path)

/**
 * Macro for check file descriptor validity
 *
 * This macro is used for check the validity of #LOADER_FD_T after open
 * operation
 *
 * @param fd File descriptor object
 * @retval Zero if fd is unusable
 * @retval Non-zero if fd is usable
 */
#define LOADER_FD_VALID(fd)

/**
 * Macro for read buffer from file
 *
 * This macro is used when need read a block for the #LOADER_FD_T previous
 * opened.
 * @param fd File descriptor
 * @param buffer Writable buffer to store read data
 * @param size Number of bytes to read
 */
#define LOADER_READ(fd, buffer, size)

/**
 * Close file macro
 *
 * Close a file descriptor previously opened with LOADER_OPEN_FOR_RD
 *
 * @param fd File descriptor to close
 */
#define LOADER_CLOSE(fd)

/**
 * Seek read cursor of file
 *
 * Seek read cursor from begin of file
 *
 * @param fd File descriptor
 * @param off Offset from begin of file
 */
#define LOADER_SEEK_FROM_START(fd, off)

/**
 * Tell cursor of file
 *
 * Tell current read cursor of file
 *
 * @param fd File descriptor
 * @return The current read cursor position
 */
#define LOADER_TELL(fd)

/**
 * Allocate memory service
 *
 * Allocate aligned memory with required right access
 *
 * @param size Size in bytes to allocate
 * @param align Aligned size in bytes
 * @param perm Access rights of allocated block. Mask of #ELFSecPerm_t values
 * @return Void pointer to allocated memory or NULL on fail
 */
#define LOADER_ALIGN_ALLOC(size, align, perm)

/**
 * Free memory
 *
 * Free memory allocated with #LOADER_ALIGN_ALLOC
 *
 * @param ptr Pointer to allocated memory to free
 */
#define LOADER_FREE(ptr)

/**
 * Clear memory block
 *
 * Set to zero memory block
 *
 * @param ptr Pointer to memory
 * @param size Bytes to clear
 */
#define LOADER_CLEAR(ptr, size)

/**
 * Compare string
 *
 * Compare zero terminated C-strings
 *
 * @param s1 First string
 * @param s2 Second string
 * @retval Zero if non equals
 * @retval Non-zero if s1==s2
 */
#define LOADER_STREQ(s1, s2)

/**
 * Jump to code
 *
 * Execute loaded code from entry point
 *
 * @param entry Pointer to function code to execute
 */
#define LOADER_JUMP_TO(entry)

/**
 * Debug macro
 *
 * This macro is used for trace code execution and not side effect is needed
 *
 * @param ... printf style format and variadic argument list
 */
#define DBG(...)

/**
 * Error macro
 *
 * This macro is used on unrecoverable error message
 *
 * @param msg C string printable text
 */
#define ERR(msg)

/**
 * Information/Warnings message macro
 *
 * This macro is used on information or recoverable warnings mesages
 *
 * @param msg C string printable text
 */
#define MSG(msg)

/** @} */

#endif

#endif /* LOADER_CONFIG_H_ */
