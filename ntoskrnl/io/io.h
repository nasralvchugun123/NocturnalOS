#ifndef IO_H
#define IO_H

#include "io_types.h"

void IoInitSystem(void);

NTSTATUS        IoCreateDriver(const char *Name, PDRIVER_INITIALIZE Init);
NTSTATUS        IoDeleteDriver(DRIVER_OBJECT *Driver);
DRIVER_OBJECT  *IoFindDriver(const char *Name);
uint32_t        IoGetDriverCount(void);

NTSTATUS        IoCreateDevice(DRIVER_OBJECT *Driver, uint32_t ExtSize,
                               const char *Name, uint32_t DeviceType,
                               uint32_t Characteristics, DEVICE_OBJECT **Out);
void            IoDeleteDevice(DEVICE_OBJECT *Device);
DEVICE_OBJECT  *IoFindDevice(const char *Name);
uint32_t        IoGetDeviceCount(void);

IRP              *IoAllocateIrp(uint8_t StackSize);
void              IoFreeIrp(IRP *Irp);
void              IoInitializeIrp(IRP *Irp, uint16_t Size, uint8_t StackSize);

IO_STACK_LOCATION *IoGetCurrentIrpStackLocation(IRP *Irp);
IO_STACK_LOCATION *IoGetNextIrpStackLocation(IRP *Irp);
void               IoSkipCurrentIrpStackLocation(IRP *Irp);
void               IoSetNextIrpStackLocation(IRP *Irp);

NTSTATUS  IoCallDriver(DEVICE_OBJECT *Device, IRP *Irp);
void      IoCompleteRequest(IRP *Irp, uint8_t PriorityBoost);
void      IoSetCompletionRoutine(IRP *Irp, PIO_COMPLETION_ROUTINE Routine,
                                 void *Context,
                                 int OnSuccess, int OnError, int OnCancel);

#endif