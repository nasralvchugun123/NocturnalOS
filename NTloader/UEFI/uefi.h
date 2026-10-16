
#ifndef UEFI_H
#define UEFI_H

#include <stdint.h>
#include <stddef.h>

typedef uint64_t EFI_STATUS;
typedef void*    EFI_HANDLE;
typedef uint64_t UINTN;
typedef uint64_t EFI_PHYSICAL_ADDRESS;
typedef uint64_t EFI_VIRTUAL_ADDRESS;

#define EFI_SUCCESS              0ULL
#define EFI_LOAD_ERROR           1ULL
#define EFI_INVALID_PARAMETER    2ULL
#define EFI_UNSUPPORTED          3ULL
#define EFI_BAD_BUFFER_SIZE      4ULL
#define EFI_BUFFER_TOO_SMALL     5ULL
#define EFI_NOT_READY            6ULL
#define EFI_DEVICE_ERROR         7ULL
#define EFI_WRITE_PROTECTED      8ULL
#define EFI_OUT_OF_RESOURCES     9ULL
#define EFI_VOLUME_CORRUPTED     10ULL
#define EFI_VOLUME_FULL          11ULL
#define EFI_NO_MEDIA             12ULL
#define EFI_MEDIA_CHANGED        13ULL
#define EFI_NOT_FOUND            14ULL
#define EFI_ACCESS_DENIED        15ULL
#define EFI_NO_RESPONSE          16ULL
#define EFI_NO_MAPPING           17ULL
#define EFI_TIMEOUT              18ULL
#define EFI_NOT_STARTED          19ULL
#define EFI_ALREADY_STARTED      20ULL
#define EFI_ABORTED              21ULL
#define EFI_ICMP_ERROR           22ULL
#define EFI_TFTP_ERROR           23ULL
#define EFI_PROTOCOL_ERROR       24ULL
#define EFI_INCOMPATIBLE_VERSION 25ULL
#define EFI_SECURITY_VIOLATION   26ULL
#define EFI_CRC_ERROR            27ULL
#define EFI_END_OF_MEDIA         28ULL
#define EFI_END_OF_FILE          31ULL
#define EFI_INVALID_LANGUAGE     32ULL
#define EFI_COMPROMISED_DATA     33ULL

#define EFIAPI __attribute__((ms_abi))

typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} EFI_GUID;

typedef struct {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void       *Reset;
    EFI_STATUS (EFIAPI *OutputString)(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
                                      uint16_t *String);
    void       *TestString;
    void       *QueryMode;
    void       *SetMode;
    void       *SetAttribute;
    void       *ClearScreen;
    void       *SetCursorPosition;
    void       *EnableCursor;
    void       *Mode;
};

typedef enum {
    AllocateAnyPages = 0,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
    EfiReservedMemoryType = 0,
    EfiLoaderCode, EfiLoaderData,
    EfiBootServicesCode, EfiBootServicesData,
    EfiRuntimeServicesCode, EfiRuntimeServicesData,
    EfiConventionalMemory, EfiUnusableMemory,
    EfiACPIReclaimMemory, EfiACPIMemoryNVS,
    EfiMemoryMappedIO, EfiMemoryMappedIOPortSpace,
    EfiPalCode, EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct {
    uint32_t Type;
    uint32_t Pad;
    EFI_PHYSICAL_ADDRESS PhysicalStart;
    EFI_VIRTUAL_ADDRESS  VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    EFI_TABLE_HEADER Hdr;                                       // 0..23

    void *RaiseTPL;                                             // 24
    void *RestoreTPL;                                           // 32

    EFI_STATUS (EFIAPI *AllocatePages)(EFI_ALLOCATE_TYPE,
                                       EFI_MEMORY_TYPE, UINTN,
                                       EFI_PHYSICAL_ADDRESS *); // 40
    EFI_STATUS (EFIAPI *FreePages)(EFI_PHYSICAL_ADDRESS, UINTN); // 48
    EFI_STATUS (EFIAPI *GetMemoryMap)(UINTN *, EFI_MEMORY_DESCRIPTOR *,
                                      UINTN *, UINTN *, uint32_t *); // 56
    EFI_STATUS (EFIAPI *AllocatePool)(EFI_MEMORY_TYPE, UINTN, void **); // 64
    EFI_STATUS (EFIAPI *FreePool)(void *);                       // 72

    void *CreateEvent;                                          // 80
    void *SetTimer;                                             // 88
    void *WaitForEvent;                                         // 96
    void *SignalEvent;                                          // 104
    void *CloseEvent;                                           // 112
    void *CheckEvent;                                           // 120

    void *InstallProtocolInterface;                             // 128
    void *ReinstallProtocolInterface;                           // 136
    void *UninstallProtocolInterface;                           // 144
    EFI_STATUS (EFIAPI *HandleProtocol)(EFI_HANDLE, EFI_GUID *, void **); // 152
    void *Reserved;                                             // 160
    void *RegisterProtocolNotify;                               // 168
    void *LocateHandle;                                         // 176
    void *LocateDevicePath;                                     // 184
    void *InstallConfigurationTable;                            // 192

    void *LoadImage;                                            // 200
    void *StartImage;                                           // 208
    void *Exit;                                                 // 216
    void *UnloadImage;                                          // 224
    EFI_STATUS (EFIAPI *ExitBootServices)(EFI_HANDLE, UINTN);   // 232

    void *GetNextMonotonicCount;                                // 240
    void *Stall;                                                // 248
    void *SetWatchdogTimer;                                     // 256
    void *ConnectController;                                    // 264
    void *DisconnectController;                                 // 272

    EFI_STATUS (EFIAPI *OpenProtocol)(EFI_HANDLE Handle,
                                      EFI_GUID *Protocol,
                                      void **Interface,
                                      EFI_HANDLE AgentHandle,
                                      EFI_HANDLE ControllerHandle,
                                      uint32_t Attributes);    // 280
    void *CloseProtocol;                                        // 288
    void *OpenProtocolInformation;                              // 296

    void *ProtocolsPerHandle;                                   // 304
//////////////////////////////////////////////////////////////////////////////////!
EFI_STATUS (EFIAPI *LocateHandleBuffer)(int SearchType,
                                        EFI_GUID *Protocol,
                                        void *SearchKey,
                                        UINTN *NoHandles,
                                        EFI_HANDLE **Buffer);       // 312
EFI_STATUS (EFIAPI *LocateProtocol)(EFI_GUID *, void *, void **); // 320
//////////////////////////////////////////////////////////////////////////////////!
void *InstallMultipleProtocolInterfaces;                    // 328
    void *UninstallMultipleProtocolInterfaces;                  // 336
    void *CalculateCrc32;                                       // 344

    void *CopyMem;                                              // 352
    void *SetMem;                                               // 360
    void *CreateEventEx;                                        // 368
} EFI_BOOT_SERVICES;

typedef struct {
    EFI_GUID  VendorGuid;
    void     *VendorTable;
} EFI_CONFIGURATION_TABLE;

typedef struct {
    EFI_TABLE_HEADER Hdr;                                       // 0
    uint16_t        *FirmwareVendor;                            // 24
    uint32_t         FirmwareRevision;                          // 32
    uint32_t         _pad0;                                     // 36
    EFI_HANDLE       ConsoleInHandle;                           // 40
    void            *ConIn;                                     // 48
    EFI_HANDLE       ConsoleOutHandle;                          // 56
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;                    // 64
    EFI_HANDLE       StandardErrorHandle;                       // 72
    void            *StdErr;                                    // 80
    void            *RuntimeServices;                           // 88
    EFI_BOOT_SERVICES *BootServices;                            // 96
    UINTN                     NumberOfTableEntries;             // 104
    EFI_CONFIGURATION_TABLE  *ConfigurationTable;               // 112
} EFI_SYSTEM_TABLE;

extern EFI_SYSTEM_TABLE *gST;

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1,0x9562,0x11D2,{0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x964E5B22,0x6459,0x11D2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_FILE_INFO_GUID \
    {0x09576E92,0x6D3F,0x11D2,{0x8E,0x39,0x00,0xA0,0xC9,0x69,0x72,0x3B}}
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042A9DE,0x23DC,0x4A38,{0x96,0xFB,0x7A,0xDE,0xD0,0x80,0x51,0x6A}}

typedef struct {
    uint32_t Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    EFI_HANDLE DeviceHandle;
    void *FilePath;
    void *Reserved;
    uint32_t LoadOptionsSize;
    void *LoadOptions;
    void *ImageBase;
    uint64_t ImageSize;
    uint32_t ImageCodeType;
    uint32_t ImageDataType;
    void *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;
struct _EFI_FILE_PROTOCOL {
    uint64_t Revision;
    EFI_STATUS (EFIAPI *Open)(EFI_FILE_PROTOCOL *, EFI_FILE_PROTOCOL **,
                              uint16_t *, uint64_t, uint64_t);
    EFI_STATUS (EFIAPI *Close)(EFI_FILE_PROTOCOL *);
    EFI_STATUS (EFIAPI *Delete)(EFI_FILE_PROTOCOL *);
    EFI_STATUS (EFIAPI *Read)(EFI_FILE_PROTOCOL *, uint64_t *, void *);
    EFI_STATUS (EFIAPI *Write)(EFI_FILE_PROTOCOL *, uint64_t *, void *);
    EFI_STATUS (EFIAPI *GetPosition)(EFI_FILE_PROTOCOL *, uint64_t *);
    EFI_STATUS (EFIAPI *SetPosition)(EFI_FILE_PROTOCOL *, uint64_t);
    EFI_STATUS (EFIAPI *GetInfo)(EFI_FILE_PROTOCOL *, EFI_GUID *,
                                 uint64_t *, void *);
    EFI_STATUS (EFIAPI *SetInfo)(EFI_FILE_PROTOCOL *, EFI_GUID *,
                                 uint64_t, void *);
    EFI_STATUS (EFIAPI *Flush)(EFI_FILE_PROTOCOL *);
};

typedef struct {
    uint64_t Revision;
    EFI_STATUS (EFIAPI *OpenVolume)(void *This, EFI_FILE_PROTOCOL **Root);
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct {
    uint16_t Year; uint8_t Month, Day, Hour, Minute, Second, Pad1;
    uint32_t Nanosecond; int16_t TimeZone; uint8_t Daylight, Pad2;
} EFI_TIME;

typedef struct {
    uint64_t Size;
    uint64_t FileSize;
    uint64_t PhysicalSize;
    EFI_TIME CreateTime;
    EFI_TIME LastAccessTime;
    EFI_TIME ModificationTime;
    uint64_t Attribute;
    uint16_t FileName[];
} EFI_FILE_INFO;

#define EFI_FILE_MODE_READ   0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE 0x8000000000000000ULL

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define AllHandles   0
#define ByRegisterNotify 1
#define ByProtocol  2
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

typedef struct {
    uint32_t RedMask, GreenMask, BlueMask, ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    uint32_t Version;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    uint32_t PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    uint32_t MaxMode;
    uint32_t Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct {
    EFI_STATUS (EFIAPI *QueryMode)(void *This, uint32_t ModeNumber,
                                   UINTN *SizeOfInfo,
                                   EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);
    EFI_STATUS (EFIAPI *SetMode)(void *This, uint32_t ModeNumber);
    void       *Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

#endif // UEFI_H 