
#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>

#define BOOT_INFO_MAGIC   0x4E545042u   
#define BOOT_INFO_VERSION 2u

typedef struct {
    uint32_t Type;
    uint32_t Pad;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} BOOT_MEMORY_DESCRIPTOR;

#define BOOT_MEM_RESERVED          0
#define BOOT_MEM_LOADER_CODE       1
#define BOOT_MEM_LOADER_DATA       2
#define BOOT_MEM_BOOTSVC_CODE      3
#define BOOT_MEM_BOOTSVC_DATA      4
#define BOOT_MEM_RUNTIME_CODE      5
#define BOOT_MEM_RUNTIME_DATA      6
#define BOOT_MEM_CONVENTIONAL      7
#define BOOT_MEM_UNUSABLE          8
#define BOOT_MEM_ACPI_RECLAIM      9
#define BOOT_MEM_ACPI_NVS          10
#define BOOT_MEM_MMIO              11
#define BOOT_MEM_MMIO_PORT         12
#define BOOT_MEM_PAL_CODE          13
#define BOOT_MEM_PERSISTENT        14

typedef struct {
    uint32_t Magic;
    uint32_t Version;
    uint32_t Size;

    uint64_t KernelBase;
    uint64_t KernelSize;
    uint64_t EntryPoint;
    uint64_t StackTop;

    uint64_t FramebufferBase;
    uint64_t FramebufferSize;
    uint32_t FramebufferWidth;
    uint32_t FramebufferHeight;
    uint32_t FramebufferPixelsPerScanLine;
    uint32_t FramebufferFormat;

    uint64_t AcpiRsdp;

    uint64_t MemoryMap;                 
    uint64_t MemoryMapSize;             
    uint64_t MemoryMapDescriptorSize;   
    uint32_t MemoryMapDescriptorVersion;
    uint32_t Reserved2;

    uint64_t EfiSystemTable;
} BOOT_INFO;

#endif