#include "io.h"
#include <rt.h>

extern int IopAdvanceIrpStack(IRP *Irp);

NTSTATUS IoCallDriver(DEVICE_OBJECT *Device, IRP *Irp)
{
    if (!Device || !Irp) return STATUS_INVALID_PARAMETER;

    IO_STACK_LOCATION *loc = IoGetCurrentIrpStackLocation(Irp);
    if (!loc) {
        Irp->IoStatus = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    loc->DeviceObject = Device;

    DRIVER_OBJECT *drv = Device->DriverObject;
    if (!drv) {
        Irp->IoStatus = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, 0);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    uint8_t mj = loc->MajorFunction;
    if (mj > IRP_MJ_MAXIMUM_FUNCTION) {
        Irp->IoStatus = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, 0);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    PDRIVER_DISPATCH dispatch = drv->MajorFunction[mj];
    if (!dispatch) {
        Irp->IoStatus = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, 0);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return dispatch(Device, Irp);
}

void IoCompleteRequest(IRP *Irp, uint8_t PriorityBoost)
{
    (void)PriorityBoost;
    if (!Irp) return;

    while (Irp->CurrentLocation > 0) {
        IO_STACK_LOCATION *loc = &Irp->Stack[Irp->CurrentLocation];
        if (loc->CompletionRoutine) {
            NTSTATUS st = loc->CompletionRoutine(
                loc->DeviceObject, Irp, loc->Context);
            if (st == STATUS_MORE_PROCESSING_REQUIRED) {
                return;
            }
        }
        Irp->CurrentLocation--;
    }

    IO_STACK_LOCATION *top = &Irp->Stack[0];
    if (top->CompletionRoutine) {
        NTSTATUS st = top->CompletionRoutine(top->DeviceObject, Irp, top->Context);
        if (st == STATUS_MORE_PROCESSING_REQUIRED) return;
    }

}