#ifndef NTOSKRNL_H
#define NTOSKRNL_H

#include <stdint.h>
#include <stddef.h>

#include <boot_info.h>
#include <rt.h>
#include <dbg/print/kdprint.h>
#include <ds/fb/fb.h>
#include <ke/kcpu.h>

//
void HalInitSystem(void);
void HalInitializeProcessor(void);
void MmInitSystem(BOOT_INFO *BootInfo);
void KeInitSystem(void);
void ObInitSystem(void);
void PsInitSystem(void);
void IoInitSystem(void);

void     KeInitializeClock(uint32_t Interval);
void     KeUpdateClock(void);
uint64_t KeGetClockInterruptCount(void);
void     KeInitializeExceptionHandling(void);
void     KeDispatchException(uint32_t ExceptionCode);
uint64_t KeGetExceptionCount(void);
void     KeFatalException(uint32_t Code, const char *Message);
void     KeFreezeCPU(void);
void     KeInitializeProcessor(void);
void     KeWaitForObject(void *Object);
void     KeIncrementTick(void);
uint64_t KeGetTickCount(void);

void *MmAllocatePages(uint32_t PageCount);
void  MmFreePages(void *Address, uint32_t PageCount);

void VfsInitializeFileSystem(void);

#endif