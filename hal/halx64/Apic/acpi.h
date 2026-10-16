#ifndef HAL_ACPI_H
#define HAL_ACPI_H

#include <stdint.h>

typedef struct {
    char     Signature[4];
    uint32_t Length;
    uint8_t  Revision;
    uint8_t  Checksum;
    char     OemId[6];
    char     OemTableId[8];
    uint32_t OemRevision;
    char     CreatorId[4];
    uint32_t CreatorRevision;
} __attribute__((packed)) ACPI_SDT_HEADER;

typedef struct {
    char     Signature[8];       //RSD PTR 
    uint8_t  Checksum;
    char     OemId[6];
    uint8_t  Revision;
    uint32_t RsdtAddress;
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t  ExtendedChecksum;
    uint8_t  Reserved[3];
} __attribute__((packed)) ACPI_RSDP;

typedef struct {
    ACPI_SDT_HEADER Hdr;
    uint32_t LocalApicAddress;
    uint32_t Flags;
} __attribute__((packed)) ACPI_MADT;

typedef struct {
    uint8_t Type;
    uint8_t Length;
} __attribute__((packed)) ACPI_MADT_ENTRY;

#define ACPI_MADT_TYPE_LAPIC    0
#define ACPI_MADT_TYPE_IOAPIC   1
#define ACPI_MADT_TYPE_OVERRIDE 2
#define ACPI_MADT_TYPE_NMI      4

typedef struct {
    ACPI_MADT_ENTRY Hdr;
    uint8_t  AcpiProcessorId;
    uint8_t  ApicId;
    uint32_t Flags;
} __attribute__((packed)) ACPI_MADT_LAPIC;

#define ACPI_MADT_LAPIC_ENABLED (1u << 0)

#define MADT_MAX_CPUS 64
#define MADT_MAX_IOAPICS 8

typedef struct {
    uint32_t Count;
    struct {
        uint8_t  AcpiId;
        uint8_t  ApicId;
        uint32_t Flags;
    } Cpus[MADT_MAX_CPUS];

    uint32_t IoApicCount;
    struct {
        uint8_t  Id;
        uint32_t Address;
        uint32_t GsiBase;
    } IoApics[MADT_MAX_IOAPICS];
} MADT_INFO;

int   HalpMadtParse(uint64_t RsdpPhys, MADT_INFO *Out);
void  HalpMadtInit(uint64_t RsdpPhys);
MADT_INFO *HalpGetMadtInfo(void);

#endif