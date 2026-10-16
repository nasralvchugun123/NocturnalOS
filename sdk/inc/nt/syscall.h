#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYS_NT_CREATE_MUTANT            0
#define SYS_NT_RELEASE_MUTANT           1
#define SYS_NT_TERMINATE_PROCESS        2
#define SYS_NT_DISPLAY_STRING           3
#define SYS_NT_GET_TICK_COUNT           4
#define SYS_NT_DELAY_EXECUTION          5
#define SYS_NT_READ_FILE                6
#define SYS_NT_CREATE_PROCESS           7
#define SYS_NT_WAIT_FOR_SINGLE_OBJECT   8
#define SYS_NT_CLOSE                    9
#define SYS_NT_CREATE_EVENT             10
#define SYS_NT_SET_EVENT                11
#define SYS_NT_RESET_EVENT              12
#define SYS_NT_PULSE_EVENT              13
#define SYS_NT_CREATE_SEMAPHORE         14
#define SYS_NT_RELEASE_SEMAPHORE        15
#define SYS_NT_OPEN_EVENT               16
#define SYS_NT_OPEN_MUTANT              17
#define SYS_NT_OPEN_SEMAPHORE           18
#define SYS_NT_WRITE_CONSOLE            19
#define SYS_NT_READ_CONSOLE             20
#define SYS_NT_ALLOCATE_VM              21
#define SYS_NT_FREE_VM                  22
#define SYS_NT_QUERY_DIRECTORY          23
#define SYS_NT_LOAD_LIBRARY             24
#define SYS_NT_GET_PROC_ADDRESS         25
#define SYS_NT_FREE_LIBRARY             26
#define SYS_NT_WRITE_FILE               27
#define SYS_NT_CREATE_IMAGE_SECTION     28
#define SYS_NT_MAP_IMAGE_SECTION        29
#define SYS_NT_UNMAP_IMAGE_SECTION      30
#define SYS_NT_QUERY_IMAGE_SECTION      31
#define SYS_NT_PROTECT_VM               32
#define SYS_NT_CURRENT_PEB              33
#define SYS_NT_GET_MODULE_HANDLE        34
#define SYS_NT_MAX                      35

#endif // _SYSCALL_H