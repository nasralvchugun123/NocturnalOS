#ifndef IO_TYPES_H
#define IO_TYPES_H

#include <stdint.h>
#include <nt/ntdefs.h>
#include <nt/ntstatus.h>
#include "io_const.h"

typedef struct _DEVICE_OBJECT  DEVICE_OBJECT;
typedef struct _DRIVER_OBJECT  DRIVER_OBJECT;
typedef struct _IRP            IRP;
typedef struct _IO_STACK_LOCATION IO_STACK_LOCATION;
typedef struct _FILE_OBJECT    FILE_OBJECT;
typedef NTSTATUS (*PDRIVER_DISPATCH)(DEVICE_OBJECT *Device, IRP *Irp);
typedef NTSTATUS (*PDRIVER_INITIALIZE)(DRIVER_OBJECT *Driver, void *RegistryPath);
typedef NTSTATUS (*PIO_COMPLETION_ROUTINE)(DEVICE_OBJECT *Device, IRP *Irp, void *Context);

struct _IO_STACK_LOCATION {
    uint8_t         MajorFunction;
    uint8_t         MinorFunction;
    uint8_t         Flags;
    uint8_t         Control;

    DEVICE_OBJECT  *DeviceObject;
    FILE_OBJECT    *FileObject;

    PIO_COMPLETION_ROUTINE  CompletionRoutine;
    void                   *Context;

    union {
        struct {
            uint32_t Length;
            uint32_t Key;
            uint64_t ByteOffset;
            void    *Buffer;
        } Read;

        struct {
            uint32_t Length;
            uint32_t Key;
            uint64_t ByteOffset;
            void    *Buffer;
        } Write;

        struct {
            uint32_t OutputBufferLength;
            uint32_t InputBufferLength;
            uint32_t IoControlCode;
            void    *Type3InputBuffer;
        } DeviceIoControl;

        struct {
            uint32_t Length;
            void    *Buffer;
        } QueryDirectory;

        struct {
            uint32_t FsControlCode;
            void    *Type3InputBuffer;
        } FileSystemControl;

        struct {
            uint32_t Length;
            void    *Buffer;
        } Shutdown;
    } Parameters;
};

struct _IRP {
    uint8_t         Type;
    uint16_t        Size;
    uint8_t         CurrentLocation;   
    uint8_t         PendingReturned;
    uint8_t         Cancel;
    uint8_t         Flags;
    uint8_t         StackCount;      
    uint8_t         Reserved;

    NTSTATUS        IoStatus;
    uint64_t        Information;
    void           *UserBuffer;
    void           *SystemBuffer;
    void           *UserEvent;

    IO_STACK_LOCATION *CurrentStackLocation;
    IO_STACK_LOCATION  Stack[IO_MAX_IRP_STACK];
};

struct _DEVICE_OBJECT {
    uint16_t        Type;
    uint16_t        Size;
    uint32_t        ReferenceCount;
    DRIVER_OBJECT  *DriverObject;
    DEVICE_OBJECT  *NextDevice;       
    DEVICE_OBJECT  *AttachedDevice;
    void           *DeviceExtension;
    uint32_t        DeviceType;
    uint32_t        Characteristics;
    uint32_t        Flags;
    char            Name[64];
};

struct _DRIVER_OBJECT {
    uint16_t        Type;
    uint16_t        Size;
    DEVICE_OBJECT  *DeviceObject;       
    uint32_t        Flags;
    uint32_t        DriverStart;       
    uint32_t        DriverSize;
    void           *DriverSection;
    void           *DriverExtension;
    char            Name[64];

    PDRIVER_INITIALIZE  DriverInit;
    void               *DriverStartIo;
    void               *DriverUnload;

    PDRIVER_DISPATCH    MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
};

#endif