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

#include "FAT32.h"

#include "util.h"

using namespace FAT32;

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

FS::Error FS::mount(FS& fs, uint8_t partition,  ReadRawCB readRaw, WriteRawCB writeRaw)
{
    if (partition >= 4) {
        return Error::UnsupportedPartition;
    }
    
    if (fs._mounted) {
        return Error::OK;
    }
    
    fs._readRaw = readRaw;
    fs._writeRaw = writeRaw;
    
    // Read the MBR
    char buf[512];
    if (fs._readRaw(buf, 0, 1) != 1) {
        return Error::MBRReadError;
    }
    
    MBR* mbr = reinterpret_cast<MBR*>(buf);

    // Do validation checks
    if (mbr->signature[0] != 0x55 || mbr->signature[1] != 0xaa) {
        return Error::BadMBRSignature;
    }
        
    uint8_t partitionType = mbr->partitions[partition].type;
    if (partitionType != 0x0c) {
        return Error::OnlyFAT32LBASupported;
    }
    
    // Exrtract the needed values
    fs._firstSector = bufToUInt32(mbr->partitions[partition].lbaStart);
    fs._sizeInSectors = bufToUInt32(mbr->partitions[partition].lbaCount);
    
    // Read the Boot Sector
    if (fs._readRaw(buf, fs._firstSector, 1) != 1) {
        return Error::BPBReadError;
    }
    
    // Do validation checks
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
    
    // Exrtract the needed values
    uint16_t reservedSectors = bufToUInt16(bootSector->reservedSectors);
    
    fs._sectorsPerFAT = bufToUInt32(bootSector->sectorsPerFAT32);
    fs._sectorsPerCluster = bootSector->sectorsPerCluster;
    fs._startFATSector = fs._firstSector + reservedSectors;
    fs._startDataSector = fs._startFATSector + (fs._sectorsPerFAT * 2);
    fs._rootDirectoryStartSector = FS::clusterToSector(fs, bufToUInt32(bootSector->rootDirectoryStartCluster));
    
    fs._mounted = true;
    return Error::OK;
}

bool FS::find(const FS& fs, File& file, const char* name)
{
    // Convert the incoming filename to 8.3 and then compare all 11 characters
    char nameToFind[11];
    convertTo8dot3(nameToFind, name);

    
    uint32_t dirSector = fs._rootDirectoryStartSector;

    char buf[512];
    static constexpr uint32_t EntriesPerSector = 512 / 32;
    
    // Read one sector at a time
    for (uint32_t dirSectorIndex = 0; dirSectorIndex < FS::sectorsPerCluster(fs); ++dirSectorIndex) {
        if (fs._readRaw(buf, dirSector + dirSectorIndex, 1) != 1) {
            return false;
        }
        
        FATDirEntry* ent = reinterpret_cast<FATDirEntry*>(buf);
        
        // Go through each entry in this sector and look for a match
        for (uint32_t entryIndex = 0; entryIndex < EntriesPerSector; ++entryIndex) {
            if (memcmp(nameToFind, ent[entryIndex].name, 11) == 0) {
                // A match was found, init the directory object
                for (int i = 0; i < 11; ++i) {
                    file._name[i] = ent[entryIndex].name[i];
                }
                file._name[11] = '\0';
        
                file._size = bufToUInt32(ent[entryIndex].size);
        
                // first cluster is in this crazy hi/lo split format
                uint32_t baseCluster = (static_cast<uint32_t>(bufToUInt16(ent[entryIndex].firstClusterHi)) << 16) + static_cast<uint32_t>(bufToUInt16(ent[entryIndex].firstClusterLo));
                file._baseSector = clusterToSector(fs, baseCluster);
                return true;
            }
        }
    }
    
    // No match
    return false;
}
    
FS::Error FS::read(const FS& fs, const File& file, char* buf, uint64_t sectorAddr, uint32_t sectors)
{
    int32_t result = fs._readRaw(buf, (file._baseSector + sectorAddr), sectors);
    return (result == static_cast<int32_t>(sectors)) ? Error::OK : Error::WrongSizeRead;
}

FS::Error FS::write(const FS& fs, const File& file, const char* buf, uint64_t sectorAddr, uint32_t sectors)
{
    int32_t result = fs._writeRaw(buf, (file._baseSector + sectorAddr), sectors);
    return (result == static_cast<int32_t>(sectors)) ? Error::OK : Error::WrongSizeWrite;
}
