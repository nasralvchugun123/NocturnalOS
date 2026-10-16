#include "io.h"
#include <rt.h>
#include "../mm/mm.h"
#include <dbg/print/kdprint.h>

extern void *MmAllocatePages(uint32_t PageCount);
extern void  MmFreePages(void *Pages, uint32_t PageCount);

static DEVICE_OBJECT *gDevices[IO_MAX_DEVICES];
static uint32_t       gDeviceCount = 0;

static int CopyName(const char *src, char *dst, uint32_t cap)
{
    uint32_t i = 0;
    for (; i + 1 < cap && src[i]; i++) dst[i] = src[i];
    dst[i] = 0;
    return 0;
}

static int StrEqI(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        char ca = *a, cb = *b;
        if (ca >= 'a' && ca <= 'z') ca -= 32;
        if (cb >= 'a' && cb <= 'z') cb -= 32;
        if (ca != cb) return 0;
    }
    return *a == 0 && *b == 0;
}

DEVICE_OBJECT *IoFindDevice(const char *Name)
{
    if (!Name) return 0;
    for (uint32_t i = 0; i < gDeviceCount; i++)
        if (gDevices[i] && StrEqI(gDevices[i]->Name, Name))
            return gDevices[i];
    return 0;
}

NTSTATUS IoCreateDevice(DRIVER_OBJECT *Driver, uint32_t ExtSize,
                        const char *Name, uint32_t DeviceType,
                        uint32_t Characteristics, DEVICE_OBJECT **Out)
{
    if (!Driver || !Out) return STATUS_INVALID_PARAMETER;
    if (gDeviceCount >= IO_MAX_DEVICES) return STATUS_INSUFFICIENT_RESOURCES;
    if (Name && IoFindDevice(Name)) return STATUS_OBJECT_NAME_COLLISION;

    DEVICE_OBJECT *dev = (DEVICE_OBJECT *)MmAllocatePages(1);
    if (!dev) return STATUS_INSUFFICIENT_RESOURCES;

    uint8_t *p = (uint8_t *)dev;
    for (uint32_t i = 0; i < 4096; i++) p[i] = 0;

    dev->Type             = 3;   
    dev->Size             = sizeof(DEVICE_OBJECT);
    dev->ReferenceCount   = 1;
    dev->DriverObject     = Driver;
    dev->DeviceType       = DeviceType;
    dev->Characteristics  = Characteristics;
    dev->Flags            = DO_DEVICE_INITIALIZING;

    if (Name) CopyName(Name, dev->Name, sizeof(dev->Name));

    if (ExtSize > 0) {
        uint32_t pages = (ExtSize + 4095) / 4096;
        void *ext = MmAllocatePages(pages);
        if (!ext) {
            MmFreePages(dev, 1);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        uint8_t *ep = (uint8_t *)ext;
        for (uint32_t i = 0; i < pages * 4096; i++) ep[i] = 0;
        dev->DeviceExtension = ext;
    }

    dev->NextDevice = Driver->DeviceObject;
    Driver->DeviceObject = dev;

    gDevices[gDeviceCount++] = dev;

    *Out = dev;
    KdPrintf("[IO] IoCreateDevice: '%s' type=0x%x ext=%u\n",
             Name ? Name : "<anon>", DeviceType, ExtSize);
    return STATUS_SUCCESS;
}

void IoDeleteDevice(DEVICE_OBJECT *Device)
{
    if (!Device) return;
    if (Device->DriverObject) {
        DEVICE_OBJECT **pp = &Device->DriverObject->DeviceObject;
        while (*pp) {
            if (*pp == Device) { *pp = Device->NextDevice; break; }
            pp = &(*pp)->NextDevice;
        }
    }

    for (uint32_t i = 0; i < gDeviceCount; i++) {
        if (gDevices[i] != Device) continue;
        for (uint32_t j = i; j + 1 < gDeviceCount; j++)
            gDevices[j] = gDevices[j + 1];
        gDeviceCount--;
        gDevices[gDeviceCount] = 0;
        break;
    }

    if (Device->DeviceExtension)
        MmFreePages(Device->DeviceExtension, 1);
    MmFreePages(Device, 1);
}

uint32_t IoGetDeviceCount(void) { return gDeviceCount; }