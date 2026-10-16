
#include "../ntoskrnl.h"
#include <stdio.h>
#include <string.h>

#define PS_MAX_PROCESSES 256

typedef struct {
    uint32_t ProcessId;
    char ImageName[256];
    uint32_t State;
    uint32_t Priority;
} PROCESS_ENTRY;

static PROCESS_ENTRY gProcessTable[PS_MAX_PROCESSES];
static uint32_t gProcessCount = 1; 
static uint32_t gIdleProcess = 0;

void PsInitializeProcessManager(void)
{
    memset(gProcessTable, 0, sizeof(gProcessTable));
    gProcessCount = 1;
    
    gProcessTable[0].ProcessId = 0;
    strcpy(gProcessTable[0].ImageName, "Idle");
    gProcessTable[0].State = 1;  
    gProcessTable[0].Priority = 0;
    
    printf("[PS] Process Manager initialized - Max %d processes\n", PS_MAX_PROCESSES);
}

EPROCESS *PsCreateProcess(const char *ImagePath)
{
    if (!ImagePath)
        return NULL;
    
    if (gProcessCount >= PS_MAX_PROCESSES) {
        printf("[PS] Error: Process table full\n");
        return NULL;
    }
    
    uint32_t pid = gProcessCount++;
    PROCESS_ENTRY *proc = &gProcessTable[pid];
    
    proc->ProcessId = pid;
    proc->State = 1;  
    proc->Priority = 8;  
    
    strncpy(proc->ImageName, ImagePath, sizeof(proc->ImageName) - 1);
    proc->ImageName[sizeof(proc->ImageName) - 1] = '\0';
    
    printf("[PS] Process created: %s (PID=%u)\n", ImagePath, pid);
    
    return (EPROCESS *)proc;
}

int PsTerminateProcess(uint32_t ProcessId, uint32_t ExitCode)
{
    if (ProcessId >= gProcessCount)
        return -1;
    
    PROCESS_ENTRY *proc = &gProcessTable[ProcessId];
    if (proc->State == 0)
        return -1;   
    
    proc->State = 0;  // Terminated 
    printf("[PS] Process terminated: PID=%u (Exit code: %u)\n", ProcessId, ExitCode);
    return 0;
}

uint32_t PsGetProcessCount(void)
{
    return gProcessCount;
}

// Get process information 
int PsQueryProcess(uint32_t ProcessId, char *NameBuffer, uint32_t BufferSize)
{
    if (ProcessId >= gProcessCount || !NameBuffer)
        return -1;
    
    PROCESS_ENTRY *proc = &gProcessTable[ProcessId];
    snprintf(NameBuffer, BufferSize, "%s", proc->ImageName);
    return 0;
}

int PsSetProcessPriority(uint32_t ProcessId, uint32_t Priority)
{
    if (ProcessId >= gProcessCount)
        return -1;
    
    if (Priority > 31)
        Priority = 31;
    
    gProcessTable[ProcessId].Priority = Priority;
    return 0;
}
