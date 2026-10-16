#include <dbg/print/kdprint.h>
#include "pci.h"
#include <rt.h>

#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

static inline void outl(uint16_t port, uint32_t val)
{
    __asm__ volatile ("outl %0, %1" :: "a"(val), "Nd"(port));
}
static inline uint32_t inl(uint16_t port)
{
    uint32_t v;
    __asm__ volatile ("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static inline uint32_t pci_addr(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t off)
{
    return (uint32_t)((1u << 31)
                    | ((uint32_t)bus << 16)
                    | ((uint32_t)dev << 11)
                    | ((uint32_t)fn  << 8)
                    | (off & 0xFC));
}

uint32_t HalpPciReadConfig(uint8_t Bus, uint8_t Dev, uint8_t Fn, uint8_t Offset, uint8_t Size)
{
    outl(PCI_CONFIG_ADDR, pci_addr(Bus, Dev, Fn, Offset));
    uint32_t v = inl(PCI_CONFIG_DATA);
    uint8_t shift = (Offset & 3) * 8;

    switch (Size) {
    case 1: return (v >> shift) & 0xFF;
    case 2: return (v >> shift) & 0xFFFF;
    default: return v;
    }
}

void HalpPciWriteConfig(uint8_t Bus, uint8_t Dev, uint8_t Fn, uint8_t Offset, uint8_t Size, uint32_t Value)
{
    outl(PCI_CONFIG_ADDR, pci_addr(Bus, Dev, Fn, Offset));

    if (Size == 4) {
        outl(PCI_CONFIG_DATA, Value);
        return;
    }
    uint32_t old = inl(PCI_CONFIG_DATA);
    uint8_t shift = (Offset & 3) * 8;
    uint32_t mask;
    if (Size == 1) mask = 0xFFu << shift;
    else           mask = 0xFFFFu << shift;
    old = (old & ~mask) | ((Value << shift) & mask);
    outl(PCI_CONFIG_DATA, old);
}

int PciReadDevice(uint8_t Bus, uint8_t Dev, uint8_t Fn, PCI_DEVICE *Out)
{
    uint16_t vendor = (uint16_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_VENDOR_ID, 2);
    if (vendor == 0xFFFF) return -1;

    Out->Bus         = Bus;
    Out->Device      = Dev;
    Out->Function    = Fn;
    Out->VendorId    = vendor;
    Out->DeviceId    = (uint16_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_DEVICE_ID, 2);
    Out->Revision    = (uint8_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_REVISION, 1);
    Out->ProgIf      = (uint8_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_PROG_IF, 1);
    Out->SubClass    = (uint8_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_SUBCLASS, 1);
    Out->ClassCode   = (uint8_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_CLASS, 1);
    Out->HeaderType  = (uint8_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_HEADER_TYPE, 1);
    Out->InterruptLine = (uint8_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_INTERRUPT_LINE, 1);
    Out->InterruptPin  = (uint8_t)HalpPciReadConfig(Bus, Dev, Fn, PCI_INTERRUPT_PIN, 1);

    for (int i = 0; i < 6; i++) {
        uint8_t off = (uint8_t)(PCI_BAR0 + i * 4);
        uint32_t raw = HalpPciReadConfig(Bus, Dev, Fn, off, 4);
        Out->BarIsIO[i] = (raw & 1) ? 1 : 0;

        if (Out->BarIsIO[i]) {
            Out->Bar[i] = raw & 0xFFFFFFFCu;
        } else {
            uint8_t type = (raw >> 1) & 3;
            if (type == 2 && i < 5) {
                /* 64-bit BAR */
                uint32_t hi = HalpPciReadConfig(Bus, Dev, Fn, (uint8_t)(off + 4), 4);
                Out->Bar[i] = ((uint64_t)hi << 32) | (raw & 0xFFFFFFF0u);
                Out->BarIsIO[i+1] = 2;   
                i++;
            } else {
                Out->Bar[i] = raw & 0xFFFFFFF0u;
            }
        }
    }
    return 0;
}

static void pci_scan_bus(uint8_t bus, int *found, int max_print)
{
    for (uint8_t dev = 0; dev < 32; dev++) {
        for (uint8_t fn = 0; fn < 8; fn++) {
            PCI_DEVICE d;
            if (PciReadDevice(bus, dev, fn, &d) != 0) {
                if (fn == 0) break;   
                continue;
            }

            (*found)++;

            if (*found <= max_print) {
                KdPrintf("[PCI] %02x:%02x.%x  %04x:%04x  cls=%02x:%02x progif=%02x  hdr=%02x  irq=%u\n",
                         d.Bus, d.Device, d.Function,
                         d.VendorId, d.DeviceId,
                         d.ClassCode, d.SubClass, d.ProgIf,
                         d.HeaderType,
                         (unsigned)d.InterruptLine);
                for (int i = 0; i < 6; i++) {
                    if (d.BarIsIO[i] == 2) continue;
                    if (d.Bar[i] == 0) continue;
                    KdPrintf("     BAR%d = 0x%llx (%s)\n",
                             i, (unsigned long long)d.Bar[i],
                             d.BarIsIO[i] ? "IO" : "MEM");
                }
            }

            if (fn == 0 && !(d.HeaderType & 0x80)) break;  
        }
    }
}

void HalpPciInit(void)
{
    KdPrint("[Hal] PCI enumeration\n");
    int found = 0;
    for (uint16_t bus = 0; bus < 256; bus++) {
        pci_scan_bus((uint8_t)bus, &found, 30);
    }
    KdPrintf("[Hal] PCI: %d devices found\n", found);
}

int PciFindDevice(uint16_t VendorId, uint16_t DeviceId, PCI_DEVICE *Out)
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t fn = 0; fn < 8; fn++) {
                PCI_DEVICE d;
                if (PciReadDevice((uint8_t)bus, dev, fn, &d) != 0) {
                    if (fn == 0) break;
                    continue;
                }
                if (d.VendorId == VendorId && d.DeviceId == DeviceId) {
                    *Out = d;
                    return 0;
                }
                if (fn == 0 && !(d.HeaderType & 0x80)) break;
            }
        }
    }
    return -1;
}

int PciFindByClass(uint8_t ClassCode, uint8_t SubClass, PCI_DEVICE *Out)
{
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            for (uint8_t fn = 0; fn < 8; fn++) {
                PCI_DEVICE d;
                if (PciReadDevice((uint8_t)bus, dev, fn, &d) != 0) {
                    if (fn == 0) break;
                    continue;
                }
                if (d.ClassCode == ClassCode && d.SubClass == SubClass) {
                    *Out = d;
                    return 0;
                }
                if (fn == 0 && !(d.HeaderType & 0x80)) break;
            }
        }
    }
    return -1;
}