
#ifndef HAL_PCI_H
#define HAL_PCI_H

#include <stdint.h>

/* Config space offsets */
#define PCI_VENDOR_ID       0x00
#define PCI_DEVICE_ID       0x02
#define PCI_COMMAND         0x04
#define PCI_STATUS          0x06
#define PCI_REVISION        0x08
#define PCI_PROG_IF         0x09
#define PCI_SUBCLASS        0x0A
#define PCI_CLASS           0x0B
#define PCI_HEADER_TYPE     0x0E
#define PCI_BAR0            0x10
#define PCI_BAR1            0x14
#define PCI_BAR2            0x18
#define PCI_BAR3            0x1C
#define PCI_BAR4            0x20
#define PCI_BAR5            0x24
#define PCI_INTERRUPT_LINE  0x3C
#define PCI_INTERRUPT_PIN   0x3D

/* Command register bits */
#define PCI_CMD_IO          0x0001
#define PCI_CMD_MEM         0x0002
#define PCI_CMD_BUS_MASTER  0x0004

typedef struct {
    uint8_t  Bus;
    uint8_t  Device;
    uint8_t  Function;
    uint16_t VendorId;
    uint16_t DeviceId;
    uint8_t  ClassCode;
    uint8_t  SubClass;
    uint8_t  ProgIf;
    uint8_t  Revision;
    uint8_t  HeaderType;
    uint8_t  InterruptLine;
    uint8_t  InterruptPin;
    uint64_t Bar[6];        
    int      BarIsIO[6];    
} PCI_DEVICE;

void     HalpPciInit(void);

uint32_t HalpPciReadConfig(uint8_t Bus, uint8_t Dev, uint8_t Fn, uint8_t Offset, uint8_t Size);
void     HalpPciWriteConfig(uint8_t Bus, uint8_t Dev, uint8_t Fn, uint8_t Offset, uint8_t Size, uint32_t Value);

int      PciFindDevice(uint16_t VendorId, uint16_t DeviceId, PCI_DEVICE *Out);
int      PciFindByClass(uint8_t ClassCode, uint8_t SubClass, PCI_DEVICE *Out);
int      PciReadDevice(uint8_t Bus, uint8_t Dev, uint8_t Fn, PCI_DEVICE *Out);

#endif