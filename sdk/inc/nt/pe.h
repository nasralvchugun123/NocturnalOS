#ifndef _NT_PE_H
#define _NT_PE_H

#include "ntdefs.h"

#define IMAGE_DOS_SIGNATURE     0x5A4D
#define IMAGE_NT_SIGNATURE      0x00004550
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_MACHINE_I386  0x014c

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC 0x010B
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC 0x020B

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define IMAGE_DIRECTORY_ENTRY_EXPORT    0
#define IMAGE_DIRECTORY_ENTRY_IMPORT    1
#define IMAGE_DIRECTORY_ENTRY_BASERELOC 5

#define IMAGE_REL_BASED_ABSOLUTE 0
#define IMAGE_REL_BASED_HIGHLOW  3
#define IMAGE_REL_BASED_DIR64    10
#define IMAGE_SCN_MEM_EXECUTE   0x20000000
#define IMAGE_SCN_MEM_READ      0x40000000
#define IMAGE_SCN_MEM_WRITE     0x80000000
#define IMAGE_SCN_CNT_CODE      0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA 0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080


#define IMAGE_ORDINAL_FLAG64 0x8000000000000000ULL
#define IMAGE_DIRECTORY_ENTRY_TLS 9

typedef struct _IMAGE_TLS_DIRECTORY64 {
    ULONG64 StartAddressOfRawData;
    ULONG64 EndAddressOfRawData;
    ULONG64 AddressOfIndex;
    ULONG64 AddressOfCallBacks;
    ULONG   SizeOfZeroFill;
    ULONG   Characteristics;
} IMAGE_TLS_DIRECTORY64, *PIMAGE_TLS_DIRECTORY64;


typedef struct _IMAGE_DOS_HEADER {
    USHORT e_magic;
    USHORT e_cblp;
    USHORT e_cp;
    USHORT e_crlc;
    USHORT e_cparhdr;
    USHORT e_minalloc;
    USHORT e_maxalloc;
    USHORT e_ss;
    USHORT e_sp;
    USHORT e_csum;
    USHORT e_ip;
    USHORT e_cs;
    USHORT e_lfarlc;
    USHORT e_ovno;
    USHORT e_res[4];
    USHORT e_oemid;
    USHORT e_oeminfo;
    USHORT e_res2[10];
    LONG   e_lfanew;
} IMAGE_DOS_HEADER, *PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    USHORT Machine;
    USHORT NumberOfSections;
    ULONG  TimeDateStamp;
    ULONG  PointerToSymbolTable;
    ULONG  NumberOfSymbols;
    USHORT SizeOfOptionalHeader;
    USHORT Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
    ULONG VirtualAddress;
    ULONG Size;
} IMAGE_DATA_DIRECTORY, *PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER64 {
    USHORT Magic;
    UINT8  MajorLinkerVersion;
    UINT8  MinorLinkerVersion;
    ULONG  SizeOfCode;
    ULONG  SizeOfInitializedData;
    ULONG  SizeOfUninitializedData;
    ULONG  AddressOfEntryPoint;
    ULONG  BaseOfCode;
    ULONG64 ImageBase;
    ULONG  SectionAlignment;
    ULONG  FileAlignment;
    USHORT MajorOperatingSystemVersion;
    USHORT MinorOperatingSystemVersion;
    USHORT MajorImageVersion;
    USHORT MinorImageVersion;
    USHORT MajorSubsystemVersion;
    USHORT MinorSubsystemVersion;
    ULONG  Win32VersionValue;
    ULONG  SizeOfImage;
    ULONG  SizeOfHeaders;
    ULONG  CheckSum;
    USHORT Subsystem;
    USHORT DllCharacteristics;
    ULONG64 SizeOfStackReserve;
    ULONG64 SizeOfStackCommit;
    ULONG64 SizeOfHeapReserve;
    ULONG64 SizeOfHeapCommit;
    ULONG  LoaderFlags;
    ULONG  NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER64, *PIMAGE_OPTIONAL_HEADER64;

typedef struct _IMAGE_NT_HEADERS64 {
    ULONG Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} IMAGE_NT_HEADERS64, *PIMAGE_NT_HEADERS64;

typedef struct _IMAGE_SECTION_HEADER {
    UINT8 Name[8];
    union {
        ULONG PhysicalAddress;
        ULONG VirtualSize;
    } Misc;
    ULONG VirtualAddress;
    ULONG SizeOfRawData;
    ULONG PointerToRawData;
    ULONG PointerToRelocations;
    ULONG PointerToLinenumbers;
    USHORT NumberOfRelocations;
    USHORT NumberOfLinenumbers;
    ULONG Characteristics;
} IMAGE_SECTION_HEADER, *PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_EXPORT_DIRECTORY {
    ULONG Characteristics;
    ULONG TimeDateStamp;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG Name;
    ULONG Base;
    ULONG NumberOfFunctions;
    ULONG NumberOfNames;
    ULONG AddressOfFunctions;
    ULONG AddressOfNames;
    ULONG AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
        ULONG Characteristics;
        ULONG OriginalFirstThunk;
    };
    ULONG TimeDateStamp;
    ULONG ForwarderChain;
    ULONG Name;
    ULONG FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR, *PIMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_BASE_RELOCATION {
    ULONG VirtualAddress;
    ULONG SizeOfBlock;
} IMAGE_BASE_RELOCATION, *PIMAGE_BASE_RELOCATION;

typedef struct _IMAGE_IMPORT_BY_NAME {
    USHORT Hint;
    CHAR   Name[1];
} IMAGE_IMPORT_BY_NAME, *PIMAGE_IMPORT_BY_NAME;

#endif // _NT_PE_H 