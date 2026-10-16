#include <dbg/print/kdprint.h>
#include "acpi.h"

static MADT_INFO gMadt;

MADT_INFO *HalpGetMadtInfo(void) { return &gMadt; }

static int acpi_checksum_ok(const void *p, uint32_t len)
{
    const uint8_t *b = (const uint8_t *)p;
    uint8_t sum = 0;
    for (uint32_t i = 0; i < len; i++) sum = (uint8_t)(sum + b[i]);
    return sum == 0;
}

static const ACPI_SDT_HEADER *acpi_find_in_xsdt(uint64_t XsdtPhys, const char *Sig)
{
    const ACPI_SDT_HEADER *xsdt = (const ACPI_SDT_HEADER *)(uintptr_t)XsdtPhys;
    if (xsdt->Signature[0] != 'X' || xsdt->Signature[1] != 'S' ||
        xsdt->Signature[2] != 'D' || xsdt->Signature[3] != 'T') return 0;
    if (!acpi_checksum_ok(xsdt, xsdt->Length)) {
        KdPrint("[ACPI] XSDT checksum mismatch\n");
        return 0;
    }

    uint32_t count = (xsdt->Length - sizeof(ACPI_SDT_HEADER)) / 8;
    const uint64_t *tables =
        (const uint64_t *)((const uint8_t *)xsdt + sizeof(ACPI_SDT_HEADER));

    for (uint32_t i = 0; i < count; i++) {
        const ACPI_SDT_HEADER *t =
            (const ACPI_SDT_HEADER *)(uintptr_t)tables[i];
        if (!t) continue;
        if (t->Signature[0] == Sig[0] && t->Signature[1] == Sig[1] &&
            t->Signature[2] == Sig[2] && t->Signature[3] == Sig[3])
            return t;
    }
    return 0;
}

static const ACPI_SDT_HEADER *acpi_find_in_rsdt(uint32_t RsdtPhys, const char *Sig)
{
    const ACPI_SDT_HEADER *rsdt = (const ACPI_SDT_HEADER *)(uintptr_t)RsdtPhys;
    if (rsdt->Signature[0] != 'R' || rsdt->Signature[1] != 'S' ||
        rsdt->Signature[2] != 'D' || rsdt->Signature[3] != 'T') return 0;
    if (!acpi_checksum_ok(rsdt, rsdt->Length)) {
        KdPrint("[ACPI] RSDT checksum mismatch\n");
        return 0;
    }

    uint32_t count = (rsdt->Length - sizeof(ACPI_SDT_HEADER)) / 4;
    const uint32_t *tables =
        (const uint32_t *)((const uint8_t *)rsdt + sizeof(ACPI_SDT_HEADER));

    for (uint32_t i = 0; i < count; i++) {
        const ACPI_SDT_HEADER *t =
            (const ACPI_SDT_HEADER *)(uintptr_t)tables[i];
        if (!t) continue;
        if (t->Signature[0] == Sig[0] && t->Signature[1] == Sig[1] &&
            t->Signature[2] == Sig[2] && t->Signature[3] == Sig[3])
            return t;
    }
    return 0;
}

int HalpMadtParse(uint64_t RsdpPhys, MADT_INFO *Out)
{
    if (!RsdpPhys || !Out) return -1;

    const ACPI_RSDP *rsdp = (const ACPI_RSDP *)(uintptr_t)RsdpPhys;
    if (rsdp->Signature[0] != 'R' || rsdp->Signature[1] != 'S' ||
        rsdp->Signature[2] != 'D' || rsdp->Signature[3] != ' ') {
        KdPrint("[ACPI] bad RSDP signature\n");
        return -2;
    }
    if (!acpi_checksum_ok(rsdp, 20)) {
        KdPrint("[ACPI] RSDP checksum mismatch \n");
        return -2;
    }

    KdPrintf("[ACPI] RSDP: rev=%u RSDT=0x%x XSDT=0x%llx len=%u\n",
             (unsigned)rsdp->Revision,
             (unsigned)rsdp->RsdtAddress,
             (unsigned long long)rsdp->XsdtAddress,
             (unsigned)rsdp->Length);

    const ACPI_SDT_HEADER *madt = 0;

    if (rsdp->Revision >= 2 && rsdp->XsdtAddress) {
        madt = acpi_find_in_xsdt(rsdp->XsdtAddress, "APIC");
        if (madt) KdPrint("[ACPI] MADT found via XSDT\n");
    }

    if (!madt && rsdp->RsdtAddress) {
        madt = acpi_find_in_rsdt(rsdp->RsdtAddress, "APIC");
        if (madt) KdPrint("[ACPI] MADT found via RSDT\n");
    }

    if (!madt) {
        KdPrint("[ACPI] MADT (APIC) not found in XSDT/RSDT\n");
        return -4;
    }

    KdPrintf("[ACPI] MADT len=%u rev=%u\n",
             (unsigned)madt->Length, (unsigned)madt->Revision);

    for (uint64_t i = 0; i < sizeof(MADT_INFO); i++) ((uint8_t *)Out)[i] = 0;

    const uint8_t *p   = (const uint8_t *)madt + sizeof(ACPI_MADT);
    const uint8_t *end = (const uint8_t *)madt + madt->Length;

    while (p + sizeof(ACPI_MADT_ENTRY) <= end) {
        const ACPI_MADT_ENTRY *e = (const ACPI_MADT_ENTRY *)p;
        if (e->Length < sizeof(ACPI_MADT_ENTRY)) break;

        switch (e->Type) {
        case ACPI_MADT_TYPE_LAPIC: {
            const ACPI_MADT_LAPIC *l = (const ACPI_MADT_LAPIC *)p;
            if (Out->Count < MADT_MAX_CPUS) {
                Out->Cpus[Out->Count].AcpiId = l->AcpiProcessorId;
                Out->Cpus[Out->Count].ApicId = l->ApicId;
                Out->Cpus[Out->Count].Flags  = l->Flags;
                Out->Count++;
            }
            break;
        }
        case ACPI_MADT_TYPE_IOAPIC: {
            const uint8_t *raw = p;
            if (Out->IoApicCount < MADT_MAX_IOAPICS && e->Length >= 12) {
                Out->IoApics[Out->IoApicCount].Id      = raw[2];
                Out->IoApics[Out->IoApicCount].Address = *(const uint32_t *)(raw + 4);
                Out->IoApics[Out->IoApicCount].GsiBase = *(const uint32_t *)(raw + 8);
                Out->IoApicCount++;
            }
            break;
        }
        default:
            break;
        }
        p += e->Length;
    }

    KdPrintf("[ACPI] MADT: %u CPU(s), %u IOAPIC(s)\n",
             (unsigned)Out->Count, (unsigned)Out->IoApicCount);
    for (uint32_t i = 0; i < Out->Count; i++) {
        KdPrintf("[ACPI]   cpu[%u]: apic_id=%u acpi_id=%u flags=0x%x %s\n",
                 (unsigned)i,
                 (unsigned)Out->Cpus[i].ApicId,
                 (unsigned)Out->Cpus[i].AcpiId,
                 (unsigned)Out->Cpus[i].Flags,
                 (Out->Cpus[i].Flags & ACPI_MADT_LAPIC_ENABLED)
                    ? "enabled" : "disabled");
    }

    return 0;
}

void HalpMadtInit(uint64_t RsdpPhys)
{
    if (RsdpPhys == 0) {
        KdPrint("[ACPI] no RSDP, skipping MADT\n");
        return;
    }
    if (HalpMadtParse(RsdpPhys, &gMadt) != 0) {
        KdPrint("[ACPI] MADT parse failed\n");
        return;
    }
    KdPrint("[ACPI] MADT parse ok\n");
}