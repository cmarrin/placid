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

#include "Memory.h"

#include "util.h"
#include "Print.h"
#include "Timer.h"

using namespace placid;

// mmap is only ever called like this:
//      void* addr = mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
extern "C" void *mmap(void *, size_t length, int prot, int flags, int fd, size_t offset)
{
    return Memory::mapSegment(length);
}

extern "C" int munmap(void *addr, size_t length)
{
    return static_cast<int>(Memory::unmapSegment(addr, length));
}

inline volatile uint32_t& rawAddr(uint32_t r)
{
#ifdef __APPLE__
    static uint32_t _dummy = 0;
    return *(&_dummy);
#else
    return *(reinterpret_cast<volatile uint32_t*>(r));
#endif
}

void Memory::enableMMU()
{
#ifndef __APPLE__
    __asm volatile (
        "mrc p15,0,r0,c1,c0,0\n"
        "orr r0,r0,#0x05\n"
        "orr r0,r0,#0x1000\n"
        "mcr p15,0,r0,c1,c0,0\n"
    :
    :
    : "r0" );
#endif
}

void Memory::disableMMU()
{
#ifndef __APPLE__
    __asm volatile (
        "mrc p15,0,r2,c1,c0,0\n"
        "bic r2,#0x1000\n"
        "bic r2,#0x0005\n"
        "mcr p15,0,r2,c1,c0,0\n"
    :
    : 
    : "r2" );
#endif
}

void Memory::invalidateTLBs()
{
#ifndef __APPLE__
    __asm volatile (
        "mov r0,#0\n"
        "mcr p15,0,r0,c8,c7,0\n"
        "mcr p15,0,r0,c7,c10,4\n"
    :
    : 
    : "r0" );
#endif
}

void Memory::invalidateCaches()
{
#ifndef __APPLE__
    __asm volatile (
        "mov r0,#0\n"
        "mcr p15,0,r0,c7,c7,0\n"
        "mcr p15,0,r0,c7,c10,4\n"
    :
    : 
    : "r0" );
#endif
}

void Memory::setDomains(uint32_t domain)
{
#ifndef __APPLE__
    __asm volatile (
        "mcr p15, 0, %0, c3, c0, 0\n"
    :
    : "r" (domain));
#endif
}

// Translation table entries are as follows:
//
//  Bits 1:0 indicate the entry type:
//
//      00 - All accesses to this section cause a fault
//      01 - Coarse address of a 2nd level table, for page tables
//      10 - Section description
//      11 - reserved
//
//  1MB Sections
//      31:10   - Coarse page table base addr
//      9       - Unused (must be 0)
//      8:5     - Domain
//      4:2     - SBZ (don't use, set to 0)
//      1:0     - 10 indicates that this is a section description
//
//  Small Page table
//

typedef union {
    struct {
        uint32_t sectionTypeIdentifier : 2; // Must be 2 (b10)
        uint32_t bufferable : 1;
        uint32_t cacheable : 1;
        uint32_t executeNever : 1;
        uint32_t domain : 4;
        uint32_t present : 1;
        uint32_t accessPermission : 2;
        uint32_t typeExtension : 3;
        uint32_t apx : 1;
        uint32_t shared : 1;
        uint32_t notGlobal : 1;
        uint32_t mustBeZero : 1;
        uint32_t notSecure : 1;
        uint32_t baseAddress : 12;
    } section;
    struct {
        uint32_t ttType : 2;
        uint32_t : 2;
        uint32_t alwaysOne : 1;
        uint32_t domain : 4;
        uint32_t : 1;
        uint32_t baseAddress : 22;
    } page;
    uint32_t raw;
} SectionPageTable;

unsigned int mmu_section ( unsigned int vadd, unsigned int padd, unsigned int flags )
{
    uint32_t ra = vadd >> 20;
    uint32_t rb = 0x4000 | (ra << 2);
    uint32_t rc = (padd & 0xFFF00000) | 0xC00 | flags | 2;
    rawAddr(rb) = rc;
    return(0);
}

void Memory::setMMUSectionDescriptors(uint32_t ttb, uint32_t vaddr, uint32_t paddr, uint32_t num, AP ap, uint8_t domain, bool cacheable, bool bufferable)
{
#ifndef __APPLE__
    uint32_t vaBase = vaddr >> 20;
    uint32_t paBase = paddr >> 20;
    
    for (uint32_t i = 0; i < num; ++i) {
        SectionPageTable* section = reinterpret_cast<SectionPageTable*>(ttb + ((vaBase + i) << 2));
        section->raw = 0;
        section->section.baseAddress = paBase + i;
        section->section.domain = domain;
        section->section.cacheable = cacheable ? 1 : 0;
        section->section.bufferable = bufferable ? 1 : 0;
        section->section.accessPermission = static_cast<uint32_t>(ap);
        section->section.sectionTypeIdentifier = 2;
    }
#endif
}

void Memory::init()
{
    // Invalidate all memory
    memset(reinterpret_cast<void*>(FirstLevelTTB), 0, sizeof(SectionPageTable) * 4096);

    // Map the kernel memory (the first section) to the physical memory
    setMMUSectionDescriptors(FirstLevelTTB, 0x00000000, 0x00000000, 1, AP::UserNoAccess, 0, true, true);

    // Map the peripherals and make them non-cacheable
    setMMUSectionDescriptors(FirstLevelTTB, 0x20000000, 0x20000000, 256, AP::UserNoAccess, 0, false, false);

    // Init the MMU and caching
    invalidateCaches();
    invalidateTLBs();
    setDomains(0xffffffff);
    setTTB<0>(FirstLevelTTB);
    setTTB<1>(FirstLevelTTB);
    
    enableMMU();
    invalidateTLBs();
}

void* Memory::mapSegment(size_t size)
{
    // FIXME: Implement
    return nullptr;    // FIXME: Implement

}

int32_t Memory::unmapSegment(void* addr, size_t size)
{
    // FIXME: Implement
    return -1;
}

//void *operator new(size_t size)
//{
//    return malloc(size);
//}
//
//void *operator new[] (size_t size)
//{
//    return malloc(size);
//}
//
//void operator delete(void *p) noexcept
//{
//    free(p);
//}
//
//void operator delete [ ](void *p) noexcept
//{
//    free(p);
//}
//
//void operator delete(void *p, size_t size) noexcept
//{
//    free(p);
//}
//
//void operator delete [ ](void *p, size_t size) noexcept
//{
//    free(p);
//}
