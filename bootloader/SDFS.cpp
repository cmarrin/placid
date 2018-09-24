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

#include "SDFS.h"

#include "sdcard.h"
#include "bootutil.h"

using namespace placid;

struct PartitionEntry
{
    uint8_t status;
    uint8_t chsFirst[3];
    uint8_t type;
    uint8_t chsLast[3];
    uint8_t lbaStart[4];
    uint8_t lbaCount[4];
};

static_assert(sizeof(PartitionEntry) == 16, "Wrong PartitionEntry size");

struct MBR
{
    char bootstrap[446];
    PartitionEntry partitions[4];
    uint8_t signature[2];
};

static_assert(sizeof(MBR) == 512, "Wrong MBR size");

struct BootSector
{
    char jump[3];
    char oemName[8];
    
    // DOS 2.0 (0x0b)
    uint8_t bytesPerSector[2];
    uint8_t sectorsPerCluster;
    uint8_t reservedSectors[2];
    uint8_t numberOfFATCopies;
    uint8_t rootDirEntries[2];
    uint8_t numSectorsOld[2];
    uint8_t mediaDescriptor;
    uint8_t sectorsPerFATOld[2];
    
    // DOS 3.31 (0x18)
    uint8_t sectorsPerTrack[2];
    uint8_t headCount[2];
    uint8_t totalHiddenSectors[4];
    uint8_t totalSectors[4];
    
    // FAT32 extended (0x24)
    uint8_t sectorsPerFAT32[4];
    uint8_t descriptionFlags[2];
    uint8_t version[2];
    uint8_t rootDirectoryStartCluster[4];
    uint8_t infoSector[2];
    uint8_t backupBootSector[2];
    uint8_t reserved[12];
    uint8_t logicalDriveNumber;
    uint8_t unused;
    uint8_t extendedSignature;
    uint8_t serialNumber[4];
    char volumeName[11];
    char fatName[8];
    uint8_t executableCode[420];
    uint8_t signature[2];
};

static_assert(sizeof(BootSector) == 512, "Wrong BootSector size");

struct FATDirEntry
{
    char name[11];
    uint8_t attr;
    uint8_t reserved1[8];
    uint8_t firstClusterHi[2];
    uint8_t reserved2[4];
    uint8_t firstClusterLo[2];
    uint8_t size[4];
};

static_assert(sizeof(FATDirEntry) == 32, "Wrong FATDirEntry size");

static int32_t readRaw(char* buf, uint64_t blockAddr, uint32_t blocks)
{
    printf("*** readRaw(%d, %d)\n", static_cast<uint32_t>(blockAddr), blocks);
    int r = sdTransferBlocks(blockAddr * 512, blocks, reinterpret_cast<uint8_t*>(buf), 0);
    if (r != 0) {
        printf("*** Disk Read Error: return code=%d\n", r);
        return -r;
    }
    return static_cast<int32_t>(blocks);
}

int32_t File::read(const File& file, char* buf, uint64_t blockAddr, uint32_t blocks)
{
    return readRaw(buf, (file._baseSector + blockAddr), blocks);
}

static inline uint32_t bufToUInt32(uint8_t* buf)
{
    return  static_cast<uint32_t>(buf[0]) + 
            static_cast<uint32_t>((buf[1] << 8)) + 
            static_cast<uint32_t>((buf[2] << 16)) + 
            static_cast<uint32_t>((buf[3] << 24));
}

static inline uint16_t bufToUInt16(uint8_t* buf)
{
    return  static_cast<uint16_t>(buf[0]) + 
            static_cast<uint16_t>((buf[1] << 8));
}

SDFS::Error SDFS::mount(SDFS& fs, uint8_t device, uint8_t partition)
{
    if (device != 0) {
        return Error::UnsupportedDevice;
    }
    
    if (partition >= 4) {
        return Error::UnsupportedPartition;
    }
    
    if (fs._mounted) {
        return Error::OK;
    }
    
    sdInit();
    delay(100);
    int r = sdInitCard();
    delay(10000);
    if (r != SD_OK && r != SD_CARD_CHANGED) {
        return Error::SDCardInitFailed;
    }
    
    // Read the MBR
    char buf[512];
    if (readRaw(buf, 0, 1) != 1) {
        return Error::MBRReadError;
    }
    
    MBR* mbr = reinterpret_cast<MBR*>(buf);
    
    if (mbr->signature[0] != 0x55 || mbr->signature[1] != 0xaa) {
        return Error::BadMBRSignature;
    }
        
    // Make sure this is a type we support
    uint8_t partitionType = mbr->partitions[partition].type;
    if (partitionType != 0x0c) {
        return Error::OnlyFAT32LBASupported;
    }
    
    fs._firstSector = bufToUInt32(mbr->partitions[partition].lbaStart);
    fs._sizeInSectors = bufToUInt32(mbr->partitions[partition].lbaCount);
    
    // Read the Boot Sector
    if (readRaw(buf, fs._firstSector, 1) != 1) {
        return Error::BPBReadError;
    }
    
    BootSector* bootSector = reinterpret_cast<BootSector*>(buf);
    if (bootSector->signature[0] != 0x55 || bootSector->signature[1] != 0xaa) {
        return Error::BadBPBSignature;
    }
            
    if (bufToUInt16(bootSector->bytesPerSector) != 512) {
        return Error::UnsupportedSectorSize;
    }
    
    if (bootSector->numberOfFATCopies != 2) {
        return Error::UnsupportedFATCount;
    }
    
    if (bufToUInt16(bootSector->rootDirEntries) != 0) {
        return Error::InvalidFAT32Volume;
    }
    
    if (bufToUInt16(bootSector->sectorsPerFATOld) != 0 || bufToUInt16(bootSector->sectorsPerFATOld) != 0) {
        return Error::InvalidFAT32Volume;
    }
    
    uint16_t reservedSectors = bufToUInt16(bootSector->reservedSectors);
    
    fs._sectorsPerFAT = bufToUInt32(bootSector->sectorsPerFAT32);
    fs._sectorsPerCluster = bootSector->sectorsPerCluster;
    fs._rootDirectoryStartCluster = bufToUInt32(bootSector->rootDirectoryStartCluster);
    fs._startFATSector = fs._firstSector + reservedSectors;
    fs._startDataSector = fs._startFATSector + (fs._sectorsPerFAT * 2);
    
    fs._mounted = true;
    return Error::OK;
}

static inline char toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
}

static void convertTo8dot3(char* name8dot3, const char* name)
{
    // Find the dot
    int dot = 0;
    const char* p = name;
    while (*p) {
        if (*p == '.') {
            dot = static_cast<int>(p - name);
            break;
        }
        p++;
        dot++;
    }
    
    if (dot <= 8) {
        // We have the simple case
        int index = 0;
        for (int i = 0; i < 8; ++i) {
            name8dot3[i] = (index < dot) ? toupper(name[index++]) : ' ';
        }
    } else {
        // We need to add '~1'
        for (int i = 0; i < 8; ++i) {
            if (i < 6) {
                name8dot3[i] = toupper(name[i]);
            } else if (i == 6) {
                name8dot3[i] = '~';
            } else {
                name8dot3[i] = '1';
            }
        }
    }

    // Now add the extension
    if (name[dot] == '.') {
        dot++;
    }
    
    for (int i = 8; i < 11; ++i) {
        name8dot3[i] = name[dot] ? toupper(name[dot++]) : ' ';
    }
    
    name8dot3[11] = '\0';
}

bool SDFS::open(const SDFS& fs, File& file, const char* name, const char* mode)
{
    // Find file
    DirectoryEntry dir;
    SDFS::directory(fs, dir);
    if (!DirectoryEntry::find(fs, dir, name)) {
        return false;
    }
    
    file._error = 0;
    file._size = DirectoryEntry::size(dir);
    file._baseSector = SDFS::clusterToSector(fs, DirectoryEntry::firstFileCluster(dir));
    return true;
}

bool SDFS::directory(const SDFS& fs, DirectoryEntry& dir)
{
    // Get the address of the root dir
    dir._startDataSector = fs._startDataSector;
    dir._sectorsPerCluster = fs._sectorsPerCluster;
    dir._dirCluster = fs._rootDirectoryStartCluster;
    dir._nextIndex = 0;
    dir._valid = false;
    return true;
}

bool DirectoryEntry::find(const SDFS& fs, DirectoryEntry&dir, const char* name)
{
    // Convert the incoming filename to 8.3 and then compare all 11 characters
    char nameToFind[11];
    convertTo8dot3(nameToFind, name);

    uint32_t dirSector = SDFS::clusterToSector(fs, dir._dirCluster);

    char buf[512];
    static constexpr uint32_t EntriesPerSector = 512 / 32;
    
    for (int dirSectorIndex = 0; dirSectorIndex < dir._sectorsPerCluster; ++dirSectorIndex) {
        if (readRaw(buf, dirSector + dirSectorIndex, 1) != 1) {
            return false;
        }
        
        FATDirEntry* ent = reinterpret_cast<FATDirEntry*>(buf);
        
        for (uint32_t entryIndex = 0; entryIndex < EntriesPerSector; ++entryIndex) {
            if (memcmp(nameToFind, ent[entryIndex].name, 11) == 0) {
                for (int i = 0; i < 11; ++i) {
                    dir._name[i] = ent[entryIndex].name[i];
                }
                dir._name[11] = '\0';
        
                dir._size = bufToUInt32(ent[entryIndex].size);
        
                // first cluster is in this crazy hi/lo split format
                dir._firstFileCluster = (static_cast<uint32_t>(bufToUInt16(ent[entryIndex].firstClusterHi)) << 16) + static_cast<uint32_t>(bufToUInt16(ent[entryIndex].firstClusterLo));

                printf("***** found dirent: name='%s', size=%d, first=%d, attr=0x%02x\n", dir._name, dir._size, dir._firstFileCluster, ent[entryIndex].attr);

                dir._valid = true;
                return true;
            }
        }
    }
    
    dir._valid = false;
    return false;
}

