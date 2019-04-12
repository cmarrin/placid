/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "ELFLoader.h"

using namespace placid;

ELFLoader::ELFLoader(const char* name)
{
    _fp.reset(FileSystem::sharedFileSystem()->open(name, FileSystem::OpenMode::Read));
    if (!_fp->valid()) {
        _error = Error::FileOpen;
        return;
    }

    // Load the header
    Elf32_Ehdr elfHeader;
    if (_fp->read(reinterpret_cast<char*>(&elfHeader), sizeof(elfHeader)) != sizeof(elfHeader)) {
        _error = Error::ELFHeaderRead;
        return;
    }
    
    // Do some validity tests
    uint8_t magic[] = EI_MAGIC;
    if (elfHeader.e_ident[EI_MAG0] != magic[0] || 
        elfHeader.e_ident[EI_MAG1] != magic[1] || 
        elfHeader.e_ident[EI_MAG2] != magic[2] || 
        elfHeader.e_ident[EI_MAG3] != magic[3]) {
        _error = Error::BadMagic;
        return;
    }
    
    if (elfHeader.e_ident[EI_CLASS] != ELFCLASS32) {
        _error = Error::BadClass;
        return;
    }
    
    if (elfHeader.e_ident[EI_DATA] != ELFDATA2LSB) {
        _error = Error::BadEndianess;
        return;
    }
    
    if (elfHeader.e_type != ET_EXEC) {
        _error = Error::NotExecutable;
        return;
    }

    if (elfHeader.e_ehsize != sizeof(Elf32_Ehdr)) {
        _error = Error::BadEHeaderSize;
        return;
    }

    if (elfHeader.e_phentsize != sizeof(Elf32_Phdr)) {
        _error = Error::BadPHeaderSize;
        return;
    }

    if (elfHeader.e_shentsize != sizeof(Elf32_Shdr)) {
        _error = Error::BadSHeaderSize;
        return;
    }

    // Make sure this is ARM
    if (elfHeader.e_machine != 0x28) {
        _error = Error::BadABI;
        return;
    }
    
    // Collect info from ELF header
    _entryPoint = elfHeader.e_entry;
    _sectionCount = elfHeader.e_shnum;
    _sectionOffset = elfHeader.e_shoff;

    // Get string table offser from string section
    Elf32_Shdr sectionHeader;
    if (!readSectionHeader(&sectionHeader, elfHeader.e_shstrndx)) {
        return;
    }

    _stringSectionOffset = sectionHeader.sh_offset;
    
    // Collect info from program header
    Elf32_Phdr programHeader;
    
    if (!_fp->seek(elfHeader.e_phoff, File::SeekWhence::Set)) {
        _error = Error::ProgramHeaderRead;
        return;
    }

    if (_fp->read(reinterpret_cast<char*>(&programHeader), sizeof(programHeader)) != sizeof(programHeader)) {
        _error = Error::ProgramHeaderRead;
        return;
    }

    if (programHeader.p_type != PT_LOAD) {
        _error = Error::BadABI;
        return;
    }
    
    // Allocate the data space
    _memorySize = programHeader.p_memsz;
    _memory = std::make_unique<char[]>(_memorySize);
    
    // Collect info from sections and load
    for (uint32_t i = 0; i < _sectionCount; i++) {
        if (!readSectionHeader(&sectionHeader, i)) {
            return;
        }
        if (sectionHeader.sh_name) {
            bare::String name;
            readSectionName(name, sectionHeader.sh_name);
            collectSectionInfo(name, &sectionHeader, i);
        }
    }
}

bool ELFLoader::readSectionHeader(Elf32_Shdr* sectionHeader, uint32_t index)
{
    if (!_fp->seek(sectionOffset(index), File::SeekWhence::Set)) {
        _error = Error::InvalidSHeaderOffset;
        return false;
    }

    if (_fp->read(reinterpret_cast<char*>(sectionHeader), sizeof(Elf32_Shdr)) != sizeof(Elf32_Shdr)) {
        _error = Error::InvalidSHeaderOffset;
        return false;
    }
    
    return true;
}

bool ELFLoader::readSectionName(bare::String& name, uint32_t offset)
{
    offset = _stringSectionOffset + offset;
    if (!_fp->seek(offset, File::SeekWhence::Set)) {
        _error = Error::InvalidSStringOffset;
        return false;
    }
    
    char buf[33];
    if (_fp->read(buf, 33) == 0) {
        _error = Error::StringReadFailure;
        return false;
    }
    name = buf;
    return true;
}

bool ELFLoader::loadSection(Section& section, Elf32_Shdr* sectionHeader)
{
    if (!sectionHeader->sh_size) {
        return true;
    }
    
    if (sectionHeader->sh_addr + sectionHeader->sh_size > _memorySize) {
        _error = Error::SectionOutOfRange;
        return false;
    }
    
    char* addr = _memory.get() + sectionHeader->sh_addr;
    if (!_fp->seek(sectionHeader->sh_offset, File::SeekWhence::Set)) {
        _error = Error::InvalidSHeaderOffset;
        return false;
    }
    
    if (_fp->read(addr, sectionHeader->sh_size) != sectionHeader->sh_size) {
        _error = Error::SectionDataRead;
        return false;
    }
    
    section._data = addr;
    
    return true;
}

bool ELFLoader::collectSectionInfo(const bare::String name, Elf32_Shdr* sectionHeader, uint32_t index)
{
    if (name == ".symtab") {
        _symbolTableOffset = sectionHeader->sh_offset;
        _symbolCount = sectionHeader->sh_size / sizeof(Elf32_Sym);
    } else if (name == ".strtab") {
        _symbolTableStringOffset = sectionHeader->sh_offset;
    } else if (name == ".text") {
        _text._index = index;
        _text._size = sectionHeader->sh_size;
        return loadSection(_text, sectionHeader);
    } else if (name == ".rodata") {
        _rodata._index = index;
        _rodata._size = sectionHeader->sh_size;
        return loadSection(_rodata, sectionHeader);
    } else if (name == ".data") {
        _data._index = index;
        _data._size = sectionHeader->sh_size;
        return loadSection(_data, sectionHeader);
    } else if (name == ".bss") {
        _bss._index = index;
        _bss._size = sectionHeader->sh_size;
        return loadSection(_bss, sectionHeader);
    } else if (name == ".rel.text") {
        _text._relativeSectionIndex = index;
    } else if (name == ".rel.rodata") {
        _rodata._relativeSectionIndex = index;
    } else if (name == ".rel.data") {
        _data._relativeSectionIndex = index;
    } else {
        return false;
    }
    return true;
}
