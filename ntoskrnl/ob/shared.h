
#ifndef OB_SHARED_H
#define OB_SHARED_H

#include <stdint.h>
#include <stddef.h>
#include <nt/ntstatus.h>
#include <nt/ntdefs.h>

struct _EPROCESS;
struct _KTHREAD;

typedef struct _OBJECT_TYPE OBJECT_TYPE;

typedef struct _OBJECT_HEADER {
    OBJECT_TYPE      *Type;
    int32_t           RefCount;
    int32_t           HandleCount;
    int32_t           SignalState;
    uint16_t          PageCount;
    uint16_t          _pad;
    struct _KTHREAD  *WaiterHead;
} OBJECT_HEADER;

struct _OBJECT_TYPE {
    const char *Name;
    uint32_t    TypeId;
    void      (*DeleteProcedure)(void *Body);
    int       (*SatisfyProcedure)(OBJECT_HEADER *Hdr, struct _KTHREAD *Waiter);
};

#define OBJ_BODY(hdr)    ((void *)((uint8_t *)(hdr) + sizeof(OBJECT_HEADER)))
#define OBJ_HEADER(body) ((OBJECT_HEADER *)((uint8_t *)(body) - sizeof(OBJECT_HEADER)))

void  ObInitSystem(void);

void *ObCreateObject(OBJECT_TYPE *Type, size_t BodySize);
void  ObReferenceObject(void *Body);
void  ObDereferenceObject(void *Body);

HANDLE         ObpAllocateHandle(struct _EPROCESS *Proc, OBJECT_HEADER *Hdr,
                                 uint32_t Access);
void           ObpFreeHandle(struct _EPROCESS *Proc, HANDLE Handle);
OBJECT_HEADER *ObpLookupHandle(struct _EPROCESS *Proc, HANDLE Handle,
                               OBJECT_TYPE *Expected);
                               
void ObSignalObject(OBJECT_HEADER *Hdr);
void ObResetObject(OBJECT_HEADER *Hdr);
int  ObWaitForSingleObject(OBJECT_HEADER *Hdr, uint64_t TimeoutTicks);
void ObUnwaitThread(OBJECT_HEADER *Hdr, struct _KTHREAD *Thread);
void ObpThreadTerminated(struct _KTHREAD *Thread);

extern OBJECT_TYPE ObpProcessType;
extern OBJECT_TYPE ObpThreadType;
extern OBJECT_TYPE ObpMutantType;
extern OBJECT_TYPE ObpEventType;
extern OBJECT_TYPE ObpSemaphoreType;

typedef struct _KMUTANT {
    struct _KTHREAD *Owner;
    int32_t          Count;
    int32_t          Abandoned;
} KMUTANT;

#define NotificationEvent     0
#define SynchronizationEvent  1

typedef struct _KEVENT {
    uint32_t EventType;
} KEVENT;

typedef struct _KSEMAPHORE {
    int32_t Limit;
    int32_t Count;
} KSEMAPHORE;

#define OBJ_NAME_MAX  64
#define OBJ_DIR_MAX   64

typedef struct {
    char           Name[OBJ_NAME_MAX];
    OBJECT_HEADER *Object;   
} OBJECT_DIRECTORY_ENTRY;

typedef struct {
    uint32_t               EntryCount;
    OBJECT_DIRECTORY_ENTRY Entries[OBJ_DIR_MAX];
} OBJECT_DIRECTORY;

extern OBJECT_TYPE ObpDirectoryType;

extern OBJECT_HEADER *ObpBaseNamedObjects;

int             ObpInsertObjectByName(OBJECT_HEADER *Dir, const char *Name,
                                      OBJECT_HEADER *Obj);
OBJECT_HEADER  *ObpLookupObjectByName(OBJECT_HEADER *Dir, const char *Name);
void            ObpDeleteObjectByName(OBJECT_HEADER *Dir, const char *Name);

int             ObpParsePath(const char *Path,
                             OBJECT_HEADER **OutDir, char *OutName, size_t NameSize);

#endif