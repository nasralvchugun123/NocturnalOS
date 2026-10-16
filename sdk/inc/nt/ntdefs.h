#ifndef _NTDEFS_H
#define _NTDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void                VOID;
typedef void               *PVOID;
typedef char                CHAR;
typedef signed char         INT8;
typedef unsigned char       UINT8;
typedef unsigned char       BYTE;
typedef unsigned char       BOOLEAN;
typedef short               SHORT;
typedef unsigned short      USHORT;
typedef unsigned short      WORD;
typedef unsigned short      WCHAR;
typedef unsigned short      UINT16;
typedef int                 INT;
typedef int                 INT32;
typedef unsigned int        UINT;
typedef unsigned int        UINT32;
typedef unsigned int        DWORD;
typedef long long           INT64;
typedef long long           LONG64;
typedef unsigned long long  UINT64;
typedef unsigned long long  ULONGLONG;
typedef unsigned long long  QWORD;
typedef long                LONG;
typedef unsigned long       ULONG;
typedef long long           LONGLONG;
typedef long long           LONG_PTR;
typedef unsigned long long  ULONG_PTR;
typedef unsigned long long  SIZE_T;
typedef long long           SSIZE_T;
typedef unsigned long long  ULONG64;
typedef unsigned long       ULONG32;

typedef unsigned long long  UINT_PTR;
typedef long long           INT_PTR;

typedef ULONG_PTR           KAFFINITY;
typedef LONG                NTSTATUS;
typedef void               *HANDLE;
typedef HANDLE             *PHANDLE;
typedef ULONG               ACCESS_MASK;
typedef ACCESS_MASK        *PACCESS_MASK;

typedef CHAR           *PCHAR;
typedef CHAR           *PSTR;
typedef const CHAR     *PCSTR;
typedef WCHAR          *PWSTR;
typedef WCHAR          *PWCHAR;
typedef const WCHAR    *PCWSTR;
typedef UINT8          *PUINT8;
typedef UINT16         *PUINT16;
typedef UINT32         *PUINT32;
typedef UINT64         *PUINT64;
typedef ULONG          *PULONG;
typedef LONG           *PLONG;
typedef USHORT         *PUSHORT;
typedef SHORT          *PSHORT;
typedef BOOLEAN        *PBOOLEAN;
typedef SIZE_T         *PSIZE_T;
typedef BYTE           *PBYTE;
typedef UINT           *PUINT;
typedef INT            *PINT;
typedef ULONG_PTR      *PULONG_PTR;
typedef ULONG64        *PULONG64;

#ifndef NULL
#define NULL ((void*)0)
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define NTAPI
#define NTSYSAPI
#define NTKERNELAPI

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR  Buffer;
} ANSI_STRING, *PANSI_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG           Length;
    HANDLE          RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG           Attributes;
    PVOID           SecurityDescriptor;
    PVOID           SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _LARGE_INTEGER {
    union {
        struct { ULONG LowPart; LONG HighPart; } u;
        LONGLONG QuadPart;
    };
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef struct _ULARGE_INTEGER {
    union {
        struct { ULONG LowPart; ULONG HighPart; } u;
        ULONGLONG QuadPart;
    };
} ULARGE_INTEGER, *PULARGE_INTEGER;

typedef struct _LIST_ENTRY {
    struct _LIST_ENTRY *Flink;
    struct _LIST_ENTRY *Blink;
} LIST_ENTRY, *PLIST_ENTRY;

typedef struct _SINGLE_LIST_ENTRY {
    struct _SINGLE_LIST_ENTRY *Next;
} SINGLE_LIST_ENTRY, *PSINGLE_LIST_ENTRY;

#define CONTAINING_RECORD(address, type, field) \
    ((type *)((PCHAR)(address) - (ULONG_PTR)(&((type *)0)->field)))

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID    Pointer;
    };
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef
VOID
(NTAPI *PIO_APC_ROUTINE)(
    PVOID            ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG            Reserved
);

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    ULONG           MaximumLength;
    ULONG           Length;
    ULONG           Flags;
    ULONG           DebugFlags;

    HANDLE          ConsoleHandle;
    ULONG           ConsoleFlags;
    HANDLE          StandardInput;
    HANDLE          StandardOutput;
    HANDLE          StandardError;

    UNICODE_STRING  CurrentDirectoryPath;
    HANDLE          CurrentDirectoryHandle;
    UNICODE_STRING  DllPath;
    UNICODE_STRING  ImagePathName;
    UNICODE_STRING  CommandLine;

    PVOID           Environment;

    ULONG           StartingX;
    ULONG           StartingY;
    ULONG           CountX;
    ULONG           CountY;
    ULONG           CountCharsX;
    ULONG           CountCharsY;
    ULONG           FillAttribute;
    ULONG           WindowFlags;
    ULONG           ShowWindowFlags;

    UNICODE_STRING  WindowTitle;
    UNICODE_STRING  DesktopInfo;
    UNICODE_STRING  ShellInfo;
    UNICODE_STRING  RuntimeData;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

// DLL
#define DLL_PROCESS_DETACH  0
#define DLL_PROCESS_ATTACH  1
#define DLL_THREAD_DETACH   3
#define DLL_THREAD_ATTACH   2

typedef struct _PEB_LDR_DATA {
    ULONG      Length;
    BOOLEAN    Initialized;
    PVOID      SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY     InLoadOrderLinks;
    LIST_ENTRY     InMemoryOrderLinks;
    LIST_ENTRY     InInitializationOrderLinks;
    PVOID          DllBase;
    PVOID          EntryPoint;
    ULONG          SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG          Flags;
    USHORT         ObsoleteLoadCount;
    USHORT         TlsIndex;
    LIST_ENTRY     HashLinks;
    ULONG          TimeDateStamp;
    PVOID          EntryPointActivationContext;
    PVOID          PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

// ldr
#define LDR_ENTRY_PROCESSED     0x00000001
#define LDR_ENTRY_IN_LOAD_LIST  0x00000002
#define LDR_ENTRY_IN_INIT_LIST  0x00000004
#define LDR_ENTRY_IN_MEM_LIST   0x00000008

typedef struct _PEB {
    BOOLEAN       InheritedAddressSpace;
    BOOLEAN       ReadImageFileExecOptions;
    BOOLEAN       BeingDebugged;
    BOOLEAN       Spare;
    HANDLE        Mutant;
    PVOID         ImageBaseAddress;
    PPEB_LDR_DATA Ldr;
    PVOID         ProcessParameters;
    PVOID         SubSystemData;
    HANDLE        ProcessHeap;
    PVOID         FastPebLock;
} PEB, *PPEB;

typedef struct _NT_TIB {
    PVOID ExceptionList;              
    PVOID StackBase;                  
    PVOID StackLimit;                 
    PVOID SubSystemTib;               
    union {
        PVOID FiberData;               
        ULONG Version;
    };
    PVOID ArbitraryUserPointer;        
    struct _NT_TIB *Self;             
} NT_TIB, *PNT_TIB;

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;              
    HANDLE UniqueThread;              
} CLIENT_ID, *PCLIENT_ID;

typedef struct _TEB {
    NT_TIB  NtTib;                  
    PVOID   EnvironmentPointer;        
    CLIENT_ID ClientId;                
    PVOID   ActiveRpcHandle;          
    PVOID   ThreadLocalStoragePointer; 
    PVOID   ProcessEnvironmentBlock;   
    ULONG   LastErrorValue;
} TEB, *PTEB;

#ifdef __cplusplus
}
#endif
#endif /* _NTDEFS_H */