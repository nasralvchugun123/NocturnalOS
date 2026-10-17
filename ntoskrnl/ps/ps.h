
#ifndef PS_H
#define PS_H

#include <nt/ntdefs.h>
#include <stdint.h>
#include "../ob/shared.h"

typedef enum {
    ObjTypeNone    = 0,
    ObjTypeProcess = 1,
    ObjTypeThread  = 2,
} OBJ_KIND;

#define PS_INVALID_HANDLE   ((HANDLE)0)

#define PS_MAX_HANDLES      64

typedef struct {
    OBJECT_HEADER *Object;         
    uint32_t       GrantedAccess;
} HANDLE_ENTRY;

typedef enum {
    ThreadStateUnused     = 0,
    ThreadStateReady      = 1,
    ThreadStateRunning    = 2,
    ThreadStateBlocked    = 3,
    ThreadStateTerminated = 4,
} KTHREAD_STATE;

typedef struct _KTHREAD {
    uint64_t       Rsp;
    uint64_t       KernelStackBase;
    uint64_t       KernelStackTop;

    KTHREAD_STATE  State;
    uint32_t       Id;
    uint32_t       Priority;
    uint32_t       LastCpuIndex; 
    uint32_t       _pad0;          //AL

    void         (*EntryPoint)(void *);
    void          *Argument;

    char           Name[32];

    uint64_t       WakeupTick;
    struct _KTHREAD *BlockedNext;

    struct _KTHREAD  *Next;
    struct _EPROCESS *Process;
   
    OBJECT_HEADER    *WaitObject;
    struct _KTHREAD  *WaiterNext;
    int               WaitSignaled;  
    int32_t           WaitStatus;   
} KTHREAD;

typedef struct _EPROCESS {
    uint32_t   Pid;
    char       Name[32];
    uint64_t   Pml4Phys;
    KTHREAD   *MainThread;

    uint64_t   UserEntryPoint;
    uint64_t   UserStackTop;
    uint64_t   ProcessParameters;

    uint64_t   Peb;           
    uint64_t   Teb;           

    HANDLE_ENTRY Handles[PS_MAX_HANDLES];

    int32_t    ExitCode;
} EPROCESS;

void      PsInitSystem(void);

KTHREAD *PsCreateThread(void (*EntryPoint)(void *), void *Argument, const char *Name, EPROCESS *Process);

void      PsTerminateCurrentThread(void);
EPROCESS *PsGetSystemProcess(void);
EPROCESS *PsCreateUserProcess(const char *Path, const char *CmdLine,
                              HANDLE *HandleOut);

HANDLE    PsCreateHandle(EPROCESS *Owner, OBJECT_TYPE *Type, void *Object);
void     *PsLookupHandle(EPROCESS *Owner, HANDLE H, OBJECT_TYPE *Expected);
void      PsCloseHandle(EPROCESS *Owner, HANDLE H);
void      PsProcessTerminated(EPROCESS *Proc);

KTHREAD  *KeGetCurrentThread(void);
void      KiSetCurrentThread(KTHREAD *Thread);
void      KiInsertReadyThread(KTHREAD *Thread);
void      KiInitScheduler(void);
void      KiScheduleNext(void);
void      KiDispatchTick(void);
void      KiSwapContext(KTHREAD *From, KTHREAD *To, uint64_t NewCr3);
void      KiSetTssRsp0(uint64_t Rsp0);

void      KiBlockCurrentThread(uint64_t WakeupTick);
uint64_t  KiUnblockExpired(void);

#endif