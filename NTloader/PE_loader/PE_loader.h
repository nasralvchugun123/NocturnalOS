
#ifndef PE_LOADER_H
#define PE_LOADER_H

#include <stdint.h>
#include <stddef.h>
#include "UEFI/uefi.h"

#pragma pack(push, 1)

typedef struct {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;    // offset 60
} IMAGE_DOS_HEADER;

typedef struct {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    uint32_t VirtualAddress;
    uint32_t Size;
} IMAGE_DATA_DIRECTORY;

typedef struct {
    uint16_t Magic;                          // 0
    uint8_t  MajorLinkerVersion;             // 2
    uint8_t  MinorLinkerVersion;             // 3
    uint32_t SizeOfCode;                     // 4
    uint32_t SizeOfInitializedData;          // 8
    uint32_t SizeOfUninitializedData;        // 12
    uint32_t AddressOfEntryPoint;            // 16
    uint32_t BaseOfCode;                     // 20
    uint64_t ImageBase;                      // 24
    uint32_t SectionAlignment;               // 32
    uint32_t FileAlignment;                  // 36
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[16];
} IMAGE_OPTIONAL_HEADER64;

#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5

typedef struct {
    uint8_t  Name[8];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct {
    uint32_t VirtualAddress;
    uint32_t SizeOfBlock;
    
} IMAGE_BASE_RELOCATION;

#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_DIR64    10
#define IMAGE_REL_BASED_HIGHLOW  3

#pragma pack(pop)

#define IMAGE_DOS_SIGNATURE 0x5A4D      // "MZ"
#define IMAGE_NT_SIGNATURE  0x00004550  // "PE\0\0"
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x20B

typedef struct {
    void    *ImageBase;    
    uint64_t EntryPoint;     
    uint32_t SizeOfImage;
} PE_LOADED_IMAGE;

int PE_LoadImage(const void *FileBuffer, size_t FileSize,
                 void *(*alloc_cb)(size_t),
                 PE_LOADED_IMAGE *Out);

#endif