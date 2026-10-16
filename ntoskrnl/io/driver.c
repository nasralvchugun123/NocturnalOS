#include "io.h"
#include <rt.h>
#include "../mm/mm.h"
#include <dbg/print/kdprint.h>

extern void *MmAllocatePages(uint32_t PageCount);
extern void  MmFreePages(void *Pages, uint32_t PageCount);

static DRIVER_OBJECT *gDrivers[IO_MAX_DRIVERS];
static uint32_t       gDriverCount = 0;

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

static NTSTATUS IopInvalidDeviceRequest(DEVICE_OBJECT *Device, IRP *Irp)
{
    (void)Device;
    Irp->IoStatus = STATUS_INVALID_DEVICE_REQUEST;
    Irp->Information = 0;
    IoCompleteRequest(Irp, 0);
    return STATUS_INVALID_DEVICE_REQUEST;
}

DRIVER_OBJECT *IoFindDriver(const char *Name)
{
    if (!Name) return 0;
    for (uint32_t i = 0; i < gDriverCount; i++)
        if (gDrivers[i] && StrEqI(gDrivers[i]->Name, Name))
            return gDrivers[i];
    return 0;
}

NTSTATUS IoCreateDriver(const char *Name, PDRIVER_INITIALIZE Init)
{
    if (!Name || !Init) return STATUS_INVALID_PARAMETER;
    if (gDriverCount >= IO_MAX_DRIVERS) return STATUS_INSUFFICIENT_RESOURCES;
    if (IoFindDriver(Name)) return STATUS_OBJECT_NAME_COLLISION;

    DRIVER_OBJECT *drv = (DRIVER_OBJECT *)MmAllocatePages(1);
    if (!drv) return STATUS_INSUFFICIENT_RESOURCES;

    uint8_t *p = (uint8_t *)drv;
    for (uint32_t i = 0; i < 4096; i++) p[i] = 0;

    drv->Type = 4;           
    drv->Size = sizeof(DRIVER_OBJECT);
    CopyName(Name, drv->Name, sizeof(drv->Name));
    drv->DriverInit = Init;

    for (int i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        drv->MajorFunction[i] = IopInvalidDeviceRequest;

    gDrivers[gDriverCount++] = drv;

    KdPrintf("[IO] IoCreateDriver: '%s'\n", Name);

    NTSTATUS st = Init(drv, 0);
    if (!NT_SUCCESS(st)) {
        KdPrintf("[IO] DriverEntry('%s') failed: 0x%x\n", Name, st);
        IoDeleteDriver(drv);
        return st;
    }
    return STATUS_SUCCESS;
}

NTSTATUS IoDeleteDriver(DRIVER_OBJECT *Driver)
{
    if (!Driver) return STATUS_INVALID_PARAMETER;

    for (uint32_t i = 0; i < gDriverCount; i++) {
        if (gDrivers[i] != Driver) continue;
        for (uint32_t j = i; j + 1 < gDriverCount; j++)
            gDrivers[j] = gDrivers[j + 1];
        gDriverCount--;
        gDrivers[gDriverCount] = 0;
        MmFreePages(Driver, 1);
        return STATUS_SUCCESS;
    }
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

uint32_t IoGetDriverCount(void) { return gDriverCount; }