/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Memory.h"

#include "bare/Timer.h"
#include <cstdlib>

using namespace bare;

// Virtual memory management
//
// Sets up MMU, allocates kernal translation tables, sets up user process
// memory and handles page faults.
//
// This class may not be instantiated until the virtual memory system
// is setup. So the init() method is static.
//
// Kernal occupies the first 1 MB of physical memory, so it occupies the
// first TTB. This first TTB has a second level translation table which
// maps 256 4KB pages. These pages are used as follows:
//
//      0           0x0000 - 0x0800     reserved for system use (interrupt vectors, ATAGS, etc.)
//                  0x0800 - 0x0c00     second level translation table.
//                  0x0c00 = 0x1000     FIQ stack
//
//      1           0x1000 - 0x2000     IRQ stack
//      2           0x2000 - 0x3000     ABORT stack
//      3           0x3000 - 0x4000     SWI stack
//      4 - 7       0x4000 - 0x8000     first level translation table
//      8 - ???     0x8000 - _end       kernel code, rodata and bss
//      ??? - 255   _end   - 0x100000  heap at the bottom and the SVC stack at the top (2)
//
//      1) The interrupt stacks occupy one 4096 byte page. If a page fault occurs for any, a 
//      kernel panic is generated
//
//      2) SVC stack and heap occupy whole pages (4096 bytes) if a page fault occurs in either and
//      the adjacent page is allocated to the other, a kernel panic is generated
//
static constexpr uint32_t FirstLevelTTB = 0x4000;
static constexpr uint32_t SecondLevelTTB = 0xc00;

extern uint8_t _end;

static void* KernelHeapStart = &_end;

enum class TTType { Fault = 0, Page = 0b01, Section = 0b10 };

enum class AP {
    Control = 0,            // permission determined by control reg bits S and R
    UserNoAccess = 1,
    UserReadOnly = 2,
    UserReadWrite = 3,
};

enum class Cacheable { No, Yes };
enum class Bufferable { No, Yes };
            
template<uint32_t which> static void setTTB(uint32_t addr)
{
    static_assert(which < 2, "which variable must be 0 or 1");
    
    addr &= 0xffffc000;

    __asm volatile (
        "mcr p15,0,%1,c2,c0,%0\n"
    :
    : "I" (which), "r" (addr)
    : "r0" );
}

inline volatile uint32_t& rawAddr(uint32_t r)
{
    return *(reinterpret_cast<volatile uint32_t*>(r));
}

static void enableMMU()
{
    __asm volatile (
        "mrc p15,0,r0,c1,c0,0\n"
        "orr r0,r0,#0x05\n"
        "orr r0,r0,#0x1000\n"
        "mcr p15,0,r0,c1,c0,0\n"
    :
    :
    : "r0" );
}

//static void disableMMU()
//{
//    __asm volatile (
//        "mrc p15,0,r2,c1,c0,0\n"
//        "bic r2,#0x1000\n"
//        "bic r2,#0x0005\n"
//        "mcr p15,0,r2,c1,c0,0\n"
//    :
//    : 
//    : "r2" );
//}

static void invalidateTLBs()
{
    __asm volatile (
        "mov r0,#0\n"
        "mcr p15,0,r0,c8,c7,0\n"
        "mcr p15,0,r0,c7,c10,4\n"
    :
    : 
    : "r0" );
}

static void invalidateCaches()
{
    __asm volatile (
        "mov r0,#0\n"
        "mcr p15,0,r0,c7,c7,0\n"
        "mcr p15,0,r0,c7,c10,4\n"
    :
    : 
    : "r0" );
}

static void setDomains(uint32_t domain)
{
    __asm volatile (
        "mcr p15, 0, %0, c3, c0, 0\n"
    :
    : "r" (domain));
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

static void setMMUSectionDescriptors(uint32_t ttb, uint32_t vaddr, uint32_t paddr, uint32_t num, AP ap, uint8_t domain, bool cacheable, bool bufferable)
{
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
}

Memory::Heap* Memory::_kernelHeap = nullptr;

void Memory::init(Heap* kernelHeap)
{
    _kernelHeap = kernelHeap;
    _kernelHeap->_heapStart = KernelHeapStart;

    // Invalidate all memory
    bare::memset(reinterpret_cast<void*>(FirstLevelTTB), 0, sizeof(SectionPageTable) * 4096);

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
