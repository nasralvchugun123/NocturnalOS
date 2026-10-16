#include "io.h"
#include <rt.h>
#include "../mm/mm.h"

extern void *MmAllocatePages(uint32_t PageCount);
extern void  MmFreePages(void *Pages, uint32_t PageCount);

void IoInitializeIrp(IRP *Irp, uint16_t Size, uint8_t StackSize)
{
    if (!Irp) return;

    uint8_t *p = (uint8_t *)Irp;
    for (uint16_t i = 0; i < Size; i++) p[i] = 0;

    Irp->Type            = 6;   
    Irp->Size            = Size;
    Irp->StackCount      = StackSize;
    Irp->CurrentLocation = 0;
    Irp->IoStatus        = STATUS_SUCCESS;
    Irp->CurrentStackLocation = &Irp->Stack[0];
}

IRP *IoAllocateIrp(uint8_t StackSize)
{
    if (StackSize == 0 || StackSize > IO_MAX_IRP_STACK)
        return 0;

    IRP *irp = (IRP *)MmAllocatePages(1);
    if (!irp) return 0;

    IoInitializeIrp(irp, sizeof(IRP), StackSize);
    return irp;
}

void IoFreeIrp(IRP *Irp)
{
    if (!Irp) return;
    MmFreePages(Irp, 1);
}

IO_STACK_LOCATION *IoGetCurrentIrpStackLocation(IRP *Irp)
{
    if (!Irp) return 0;
    if (Irp->CurrentLocation >= Irp->StackCount) return 0;
    return &Irp->Stack[Irp->CurrentLocation];
}

IO_STACK_LOCATION *IoGetNextIrpStackLocation(IRP *Irp)
{
    if (!Irp) return 0;
    if (Irp->CurrentLocation + 1 >= Irp->StackCount) return 0;
    return &Irp->Stack[Irp->CurrentLocation + 1];
}

int IopAdvanceIrpStack(IRP *Irp)
{
    if (!Irp) return -1;
    if (Irp->CurrentLocation + 1 >= Irp->StackCount) return -1;
    Irp->CurrentLocation++;
    Irp->CurrentStackLocation = &Irp->Stack[Irp->CurrentLocation];
    return 0;
}

void IoSkipCurrentIrpStackLocation(IRP *Irp)
{
    IopAdvanceIrpStack(Irp);
}

void IoSetNextIrpStackLocation(IRP *Irp)
{
    /* Legacy alias */
    IopAdvanceIrpStack(Irp);
}