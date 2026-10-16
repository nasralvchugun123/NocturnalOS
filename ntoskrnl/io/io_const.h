#ifndef IO_CONST_H
#define IO_CONST_H

#define IO_MAX_DRIVERS          32
#define IO_MAX_DEVICES          64
#define IO_MAX_IRP_STACK        8

#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0A
#define IRP_MJ_DIRECTORY_CONTROL        0x0C
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0D
#define IRP_MJ_DEVICE_CONTROL           0x0E
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0F
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_PNP                      0x1B
#define IRP_MJ_MAXIMUM_FUNCTION         0x1C

#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009

#define DO_BUFFERED_IO          0x00000004
#define DO_DIRECT_IO            0x00000010
#define DO_DEVICE_INITIALIZING  0x00000080

#define IRP_NOCACHE             0x00000001
#define IRP_PAGING_IO           0x00000002
#define IRP_SYNCHRONOUS_API     0x00000040

#endif