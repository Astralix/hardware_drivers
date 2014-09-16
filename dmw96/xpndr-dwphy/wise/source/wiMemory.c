//--< wiMemory.c >---------------------------------------------------------------------------------
//=================================================================================================
//
//  WiSE Memory Allocation Routines
//  Copyright 2010-2011 DSP Group, Inc. All rights reserved.
//
//=================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "wiUtil.h"
#include "wiMemory.h"

/////////////////////////////////////////////////////////////////////////////////////////
///
///  Platform-Specific Definitions
///
#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
	#include <psapi.h>
#endif
///
/////////////////////////////////////////////////////////////////////////////////////////

//=================================================================================================
//--  PARAMETERS WITH MODULE (FILE) SCOPE
//=================================================================================================
static wiBoolean   Verbose      = WI_FALSE;
static wiBoolean   traceVerbose = WI_FALSE;

static wiBoolean   ModuleIsInitialized = WI_FALSE;
static wiMutex_t   TraceMemoryMutex;

static wiMallocFn  mallocFunction  = malloc;  // ptr -> function for wiMalloc()
static wiCallocFn  callocFunction  = calloc;  // ptr -> function for wiCalloc()
static wiReallocFn reallocFunction = realloc; // ptr -> function for wiRealloc()
static wiFreeFn    freeFunction    = free;    // ptr -> function for wiFree()

static wiMallocFn  traceMallocFunction  = malloc;  // ptr -> function for wiMalloc() using trace
static wiCallocFn  traceCallocFunction  = calloc;  // ptr -> function for wiCalloc() using trace
static wiReallocFn traceReallocFunction = realloc; // ptr -> function for wiRealloc() using trace
static wiFreeFn    traceFreeFunction    = free;    // ptr -> function for wiFree() using trace

static const unsigned long CheckAddrLimit = 0x09000000;

typedef struct traceMem_S
{
    struct traceMem_S *next;
    struct traceMem_S *prev;
    size_t size;
    void  *ptr;
    void  *baseptr;
    unsigned sequence;
    unsigned ThreadIndex;
} wiMemory_traceMem_t;

static wiMemory_traceMem_t *traceMemList = NULL;
static unsigned traceMemSequence[WISE_MAX_THREADS] = {0};
static unsigned CurrentAllocation = 0;
static unsigned PeakAllocation = 0;

const unsigned int tracePadBack   = 100;        // number of byte to pad to the block end
const unsigned int traceHashFront = 0x5AA5F00F; // hash sequence at start of the block
const unsigned int traceHashBack  = 0x13572468; // hash sequence at the end of the block

// Function Prototypes (used locally)
//
wiStatus wiMemory_CheckTraceBlock(wiMemory_traceMem_t *item);


//=================================================================================================
//--  ERROR HANDLING
//=================================================================================================
#define  STATUS(Status) WICHK((Status))
#define XSTATUS(Status) if (STATUS(Status) < 0) return WI_ERROR_RETURNED;

// ================================================================================================
// FUNCTION  : wiMemory_Init()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_Init(void)
{
    if (ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_ALREADY_INIT);

    XSTATUS( wiProcess_CreateMutex(&TraceMemoryMutex) );
    ModuleIsInitialized = WI_TRUE;
    return WI_SUCCESS;
}
// end of wiMemory_Init()

// ================================================================================================
// FUNCTION  : wiMemory_Close()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_Close(void)
{
    if (!ModuleIsInitialized) return STATUS(WI_ERROR_MODULE_NOT_INITIALIZED);

	XSTATUS( wiMemory_ListTraceBlocks() );
    XSTATUS( wiProcess_CloseMutex(&TraceMemoryMutex) );

    ModuleIsInitialized = WI_FALSE;
    return WI_SUCCESS;
}
// end of wiMemory_Close()

// ================================================================================================
// FUNCTION  : wiMemory_Check()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_Check(char *Filename, int Line)
{
    STATUS( wiProcess_WaitForMutex(&TraceMemoryMutex) );
    {
        wiMemory_traceMem_t *item = traceMemList;
        unsigned long total = 0;

        wiUtil_WriteLogFile("wiMemory_Check() called from %s line %d in thread %d\n",
            Filename, Line, wiProcess_ThreadIndex() );

        if (traceMemList)
        {
            while (item)
            {
                XSTATUS( wiMemory_CheckTraceBlock(item) )
                total += item->size;
                item = item->next;
            }
            wiUtil_WriteLogFile("    Total Allocated Memory: %d bytes\n",total);
        }
        #ifdef WIN32
        {
            int heapstatus = _heapchk();
            switch (heapstatus)
            {
                case _HEAPOK      : wiUtil_WriteLogFile("    HEAP CHECK: OK - heap is okay\n"); break;
                case _HEAPEMPTY   : wiUtil_WriteLogFile("    HEAP CHECK: OK - heap is empty\n"); break;
                case _HEAPBADBEGIN: wiUtil_WriteLogFile("    HEAP CHECK: ERROR - bad start of heap\n"); break;
                case _HEAPBADNODE : wiUtil_WriteLogFile("    HEAP CHECK: ERROR - bad node in heap\n"); break;
                default           : wiUtil_WriteLogFile("    HEAP CHECK: UNKNOWN STATUS %d\n",heapstatus);
            }
        }
        #endif
    }
    STATUS( wiProcess_ReleaseMutex(&TraceMemoryMutex) );
    return WI_SUCCESS;
}
// end of wiMemory_Check()

// ================================================================================================
// FUNCTION  : wiMemory_RecordHeap()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_RecordHeap(void)
{
#ifdef WIN32

    char filename[256]; FILE *F;
	int heapstatus, numLoops = 0;
	_HEAPINFO hinfo;

    STATUS(wiProcess_WaitForMutex(&TraceMemoryMutex));
    
	sprintf(filename, "HeapImage%u.txt",wiProcess_ThreadIndex());
	printf("filename = %s\n",filename);

    F = fopen(filename, "wt");
	if (!F) return STATUS(WI_ERROR);

	heapstatus = _heapchk();
	fprintf(F,"wiMemory_RecordHeap()\n");
	fprintf(F,"_HEAP_MAXREQ = %u\n",_HEAP_MAXREQ);
	fprintf(F,"heapstatus = %d (OK=%d, EMPTY=%d, BAD START=%d, BAD NODE = %d)\n",heapstatus, _HEAPOK, _HEAPEMPTY, _HEAPBADBEGIN, _HEAPBADNODE);

	hinfo._pentry = NULL;
	while((heapstatus = _heapwalk(&hinfo)) == _HEAPOK)
	{
		fprintf(F,"%6s block at %Fp of size %4.4X\n",
				(hinfo._useflag == _USEDENTRY ? "USED" : "FREE"), hinfo._pentry, hinfo._size);
		numLoops++;
	}
	switch(heapstatus)
	{
	    case _HEAPEMPTY   : fprintf(F,"OK - empty heap\n");             break;
	    case _HEAPEND     : fprintf(F,"OK - end of heap\n");            break;
	    case _HEAPBADPTR  : fprintf(F,"ERROR - bad pointer to heap\n"); break;
	    case _HEAPBADBEGIN: fprintf(F,"ERROR - bad start of heap\n");   break;
	    case _HEAPBADNODE : fprintf(F,"ERROR - bad node in heap\n");    break;
	}
	fclose(F);

    STATUS(wiProcess_ReleaseMutex(&TraceMemoryMutex));
#endif
    return WI_SUCCESS;
}
// end of wiMemory_RecordHeap()

    
// ================================================================================================
// FUNCTION  : wiMemory_RecordWindowsProcessMemory
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_RecordWindowsProcessMemory(void)
{
#ifdef WIN32

	wiPrintfFn LocalPrintf = wiUtil_WriteLogFile;
//	wiPrintfFn LocalPrintf = wiPrintf;

	HANDLE hProcess;
	HMODULE hMod;

	PROCESS_MEMORY_COUNTERS pmc;
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

	const double MB = 1024000.0;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
        return WI_ERROR;

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

	LocalPrintf("\n");
	LocalPrintf("Process Memory Usage (MB)\n");
	LocalPrintf("        ---WorkingSet--  --PageFileUse--\n");
	LocalPrintf("ProcID  Current----Peak  Current----Peak  Process Name\n");

	// Print the memory usage for each process
    for ( i = 0; i < cProcesses; i++ )
	{
		TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
		char cbuf[256];
		size_t sz;

		// Print information about the memory usage of the process.
		hProcess = OpenProcess(  PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i] );
		if (NULL == hProcess) {LocalPrintf("%6u  Unable to access\n",aProcesses[i]); continue;}
		
		if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) )
		{
			GetModuleBaseName( hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR) );
			wcstombs_s(&sz, cbuf, 255, szProcessName, sizeof(szProcessName)/sizeof(TCHAR));
		}
		else sprintf(cbuf, "<unknown>");

		if ( GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) )
		{
			LocalPrintf("%6u  %7.1f %7.1f  %7.1f %7.1f  %s\n",aProcesses[i], 
				pmc.WorkingSetSize/MB, pmc.PeakWorkingSetSize/MB,
				pmc.PagefileUsage/MB, pmc.PeakPagefileUsage/MB,cbuf);
		}
		CloseHandle( hProcess );
	}
	LocalPrintf("----------------------------------------------------\n");

#endif	
	return WI_SUCCESS;
}
// end of wiMemory_RecordWindowsProcessMemory()

// ================================================================================================
// MEMORY ALLOCATION FUNCTIONS
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void * wiMemory_Malloc(size_t size)
{
    return mallocFunction(size);
}

void * wiMemory_Calloc(size_t nitems, size_t size)
{
    return callocFunction(nitems, size);
}

void * wiMemory_Realloc(void * block, size_t size)
{
    return reallocFunction(block, size);
}

void wiMemory_Free(void * block)
{
    freeFunction(block);
}

// ================================================================================================
// VERBOSE MEMORY ALLOCATION FUNCTIONS
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void * wiMemory_VerboseMalloc(size_t size, char *Filename, int Line)
{
    void *ptr = mallocFunction(size);
    wiPrintf("wiMalloc(%u) = @x%08X for %s line %d\n",size, ptr, Filename, Line);
    return ptr;
}

void * wiMemory_VerboseCalloc(size_t nitems, size_t size, char *Filename, int Line)
{
    void *ptr = callocFunction(nitems, size);
    wiPrintf("wiCalloc(%u, %u) = @x%08X for %s line %d\n",nitems, size, ptr, Filename, Line);
    return ptr;
}

void * wiMemory_VerboseRealloc(void * block, size_t size, char *Filename, int Line)
{
    void *ptr = reallocFunction(block, size);
    wiPrintf("wiRealloc(@x%08X, %u) = @x%08X for %s line %d\n",block, size, ptr, Filename, Line);
    return ptr;
}

void wiMemory_VerboseFree(void * block, char *Filename, int Line)
{
    wiPrintf("wiFree(@x%08X) for %s line %d\n",block, Filename, Line);
    freeFunction(block);
}

// ================================================================================================
// FUNCTION  : wiMemory_ListTraceBlocks()
// ------------------------------------------------------------------------------------------------
// Purpose   : Call at exit to look forun-free'd memory
// ================================================================================================
wiStatus wiMemory_ListTraceBlocks(void)
{
    unsigned long total = 0;
    wiMemory_traceMem_t *item = traceMemList;
    char msg[1024];

    if (traceMemList)
    {
        wiPrintf("\n Trace Memory ----------------------------------------------\n");
        while (item)
        {
            sprintf(msg," ([%u]%8u) TraceBlock %p: User Block = %p, Size = %ld",
                    item->ThreadIndex, item->sequence, item, item->ptr, item->size);
            wiPrintf("%s\n",msg);
            wiUtil_WriteLogFile("%s\n",msg);
            total += item->size;
            item = item->next;
        }
        wiUtil_WriteLogFile("TraceBlock Allocated Memory = %u kBytes\n",total/1024);
    }
    wiUtil_WriteLogFile("Peak Memory Allocation = %u Bytes\n",PeakAllocation);
    return WI_SUCCESS;
}
// end of wiMemory_ListTraceBlocks()

// ================================================================================================
// FUNCTION  : wiMemory_FindTraceBlock()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_FindTraceBlock(void *ptr, wiMemory_traceMem_t **item)
{
    wiMemory_traceMem_t *listptr = traceMemList;
    *item = NULL;

    while (listptr)
    {
        if (listptr->ptr == ptr) {
            *item = listptr;
            return WI_SUCCESS;
        }
        listptr = listptr->next;
    }
    return WI_ERROR_PARAMETER1; // block not found
}
// end of wiMemory_FindTraceBlock()

// ================================================================================================
// FUNCTION  : wiMemory_AddTraceBlock
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_AddTraceBlock(size_t size, void **ptr)
{
    void *newblock = NULL;
    wiMemory_traceMem_t *item, *listptr = traceMemList;

    newblock = (void *)traceCallocFunction(size + sizeof(traceHashFront) + sizeof(traceHashBack) + tracePadBack, 1);
    if (!newblock) return STATUS(WI_ERROR_MEMORY_ALLOCATION);

    item = (wiMemory_traceMem_t *)traceCallocFunction(1, sizeof(wiMemory_traceMem_t));
    if (!item) return STATUS(WI_ERROR_MEMORY_ALLOCATION);
    
    if (listptr) 
    {
        // step to the end of the list and point to the new item
        while (listptr->next) listptr = listptr->next;
        listptr->next = item;
    }
    else
        traceMemList = item; // traceMemList -> item (first in list)

    // record the new block parameters
    item->next = NULL;
    item->prev = listptr;
    item->baseptr = newblock;
    item->ThreadIndex = wiProcess_ThreadIndex();
    item->sequence = (traceMemSequence[wiProcess_ThreadIndex()]++);
    item->ptr = (void *)((size_t)newblock + sizeof(traceHashFront));
    item->size = size;

    CurrentAllocation += size;
    if (CurrentAllocation > PeakAllocation) PeakAllocation = CurrentAllocation;

    // add the hash sequence at the front and end of the block
    *((unsigned int *)newblock) = traceHashFront;
    newblock = (void *)((size_t)newblock + size + sizeof(traceHashFront));
    *((unsigned int *)newblock) = traceHashBack;

    *ptr = item->ptr;
    if (traceVerbose) wiPrintf(" wiUtil_AddTraceBlock(%d, @x%08X, %d)\n",size,*ptr,item->sequence);

    // Diagnostic messages below
    #if 0
    if ((item->sequence == 109) && (item->ThreadIndex == 1)) {
        wiPrintf(" ptr = @x%08X --> 0\n",ptr);
        *ptr = 0; // should cause memory check error in calling code
    }
    #endif
    return WI_SUCCESS;
}
// end of wiMemory_AddTraceBlock()

// ================================================================================================
// FUNCTION  : wiMemory_CheckTraceBlock()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_CheckTraceBlock(wiMemory_traceMem_t *item)
{
    void *block = item->baseptr;
    unsigned int *uptr; char *cptr;
    char msg[1024];
    int ok=1; unsigned i;

    // check the hash sequences
    uptr = (unsigned int *)(item->baseptr);
    if (*uptr != traceHashFront)
    {
        sprintf(msg,"TraceBlock Check Error: Sequence #  = %u\n"
                    "                        ThreadIndex = %u\n" 
                    "                        Trace Block = %p\n"
                    "                        User Block  = %p\n"
                    "                        Block Size  = %lu bytes\n"
                    "                  Front Sequence    = 0x%08X\n",
                    item->sequence, item->ThreadIndex, item, item->ptr, item->size, *uptr);
        wiPrintf(" \n%s\n",msg);
        wiUtil_WriteLogFile("%s\n",msg);
    }
    uptr = (unsigned int *)((size_t)block + item->size + sizeof(traceHashFront));
    if (*uptr != traceHashBack)
    {
        sprintf(msg,"TraceBlock Check Error: Sequence #  = %u\n"
                    "                        ThreadIndex = %u\n"
                    "                        Trace Block = %p\n"
                    "                        User Block  = %p\n"
                    "                        Block Size  = %lu bytes\n"
                    "                   Back Sequence    = 0x%08X\n",
                    item->sequence, item->ThreadIndex, item, item->ptr, item->size, *uptr);
        wiPrintf(" \n%s\n",msg);
        wiUtil_WriteLogFile("%s\n",msg);
    }
    cptr = ((char *)(uptr+1));
    for (i=0; i<tracePadBack; i++, cptr++)
        if (*cptr) ok = 0;
    if (!ok)
    {
        sprintf(msg,"TracePadBack Check Error: Sequence #  = %u\n"
                    "                          ThreadIndex = %u\n"
                    "                          Trace Block = %p\n"
                    "                          User Block  = %p\n"
                    "                          Block Size  = %lu bytes\n",
                    item->sequence, item->ThreadIndex, item, item->ptr, item->size);
        wiPrintf(" \n%s\n\n",msg);
        wiPrintf(" Sequence = \"");
        wiUtil_WriteLogFile("%s\n",msg);
        cptr = (char *)(uptr + 1);
        for (i=0; i<tracePadBack; i++, cptr++) wiPrintf("%c",*cptr);
        wiPrintf("\"\n");
    }
    return WI_SUCCESS;
}
// end of wiMemory_CheckTraceBlock()

// ================================================================================================
// FUNCTION  : wiMemory_FreeTraceBlock()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_FreeTraceBlock(void *ptr)
{
    wiMemory_traceMem_t *item = NULL;

    if (traceVerbose) wiPrintf(" wiMemory_FreeTraceBlock(@x%08X)\n",ptr);

    XSTATUS(wiMemory_FindTraceBlock(ptr, &item));
    XSTATUS(wiMemory_CheckTraceBlock(item));

    CurrentAllocation -= item->size;  // decrease allocated memory size count
    traceFreeFunction(item->baseptr); // deallocate block

    // modify the traceMem list
    if (item->prev) {
        item->prev->next = item->next;
        if (item->next)
            item->next->prev = item->prev;
    }
    else {
        traceMemList = item->next;
        if (item->next)
            item->next->prev = NULL;
    }
    traceFreeFunction(item);

    return WI_SUCCESS;
}
// end of wiMemory_FreeTraceBlock()

// ================================================================================================
// FUNCTION  : wiMemory_traceMalloc()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void * wiMemory_traceMalloc(size_t size)
{
    void *ptr = NULL;
    STATUS(wiProcess_WaitForMutex(&TraceMemoryMutex));
    STATUS(wiMemory_AddTraceBlock(size, &ptr));
    STATUS(wiProcess_ReleaseMutex(&TraceMemoryMutex));
    return ptr;
}
// end of wiMemory_traceMalloc()

// ================================================================================================
// FUNCTION  : wiMemory_traceCalloc()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void * wiMemory_traceCalloc(size_t nitems, size_t size)
{
    void *ptr = NULL;
    STATUS(wiProcess_WaitForMutex(&TraceMemoryMutex));
    STATUS(wiMemory_AddTraceBlock(nitems*size, &ptr));
    STATUS(wiProcess_ReleaseMutex(&TraceMemoryMutex));
    return ptr;
}
// end of wiMemory_traceCalloc()

// ================================================================================================
// FUNCTION  : wiMemory_traceRealloc()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void * wiMemory_traceRealloc(void *block, size_t size)
{
    wiMemory_traceMem_t *item;
    void *ptr = NULL;

    STATUS(wiProcess_WaitForMutex(&TraceMemoryMutex));
    STATUS(wiMemory_AddTraceBlock(size, &ptr));
    if (block) {
        wiMemory_FindTraceBlock(ptr, &item);
        memcpy(ptr, block, min(size, item->size));
        STATUS(wiMemory_FreeTraceBlock(block));
    }
    STATUS(wiProcess_ReleaseMutex(&TraceMemoryMutex));
    return ptr;
}
// end of wiMemory_traceRealloc()

// ================================================================================================
// FUNCTION  : wiMemory_traceFree()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
void wiMemory_traceFree(void *block)
{
    STATUS(wiProcess_WaitForMutex(&TraceMemoryMutex));
    STATUS(wiMemory_FreeTraceBlock(block));
    STATUS(wiProcess_ReleaseMutex(&TraceMemoryMutex));
}
// end of wiMemory_traceFree()

// ================================================================================================
// FUNCTION  : wiMemory_EnableTraceMemoryManagement()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_EnableTraceMemoryManagement(void)
{
    if (Verbose) wiPrintf(" wiMemory_EnableTraceMemoryManagement()\n");
    wiUtil_WriteLogFile("\n ***** Trace Memory Mangement Enabled *****\n");

    XSTATUS( wiMemory_SetMemoryManagement ( wiMemory_traceMalloc,
                                            wiMemory_traceCalloc,
                                            wiMemory_traceRealloc,
                                            wiMemory_traceFree ) );
    return WI_SUCCESS;
}
// end of wiMemory_EnableTraceMemoryManagement()

// ================================================================================================
// FUNCTION  : wiMemory_SetTraceMemoryManagement()
// ------------------------------------------------------------------------------------------------
// ================================================================================================
wiStatus wiMemory_SetTraceMemoryManagement(wiMallocFn  mallocFn,  wiCallocFn callocFn,
                                           wiReallocFn reallocFn, wiFreeFn   freeFn )
{
    if (Verbose) wiPrintf(" wiMemory_SetTraceMemoryManagement(*,*,*,*)\n");

    if (!mallocFn)  return WI_ERROR_PARAMETER1;
    if (!callocFn)  return WI_ERROR_PARAMETER2;
    if (!reallocFn) return WI_ERROR_PARAMETER3;
    if (!freeFn)    return WI_ERROR_PARAMETER4;

    traceMallocFunction  = mallocFn;
    traceCallocFunction  = callocFn;
    traceReallocFunction = reallocFn;
    traceFreeFunction    = freeFn;

    wiUtil_WriteLogFile("wiMemory_SetTraceMemoryManagement(%p, %p, %p, %p)\n",
                          mallocFn, callocFn, reallocFn, freeFn );
    return WI_SUCCESS;
}
// end of wiMemory_SetTraceMemoryManagement()

///================================================================================================
///////////////////////////////////////////////////////////////////////////////////////////////////
///
///  AddrLimit Memory Allocation
///
///  These functions are used to limit the maximum pointer address in a memory allocation.
///  This functionality is useful for debugging heap fragmentation by causing a fault
///  sooner than if the entire memory space is available.
///
void wiMemory_CheckAddrLimit( void **pptr)
{
	if ((unsigned long)(*pptr) > CheckAddrLimit)
	{
		wiUtil_WriteLogFile("wiMemory_CheckAddrLimit (%p > %p)\n", *pptr, CheckAddrLimit);
		wiFree(*pptr);
		*pptr = NULL;
	}
}

void * wiMemory_AddrLimitMalloc(size_t size)
{
    void *ptr = malloc(size);
	wiMemory_CheckAddrLimit(&ptr);
    return ptr;
}

void * wiMemory_AddrLimitCalloc(size_t nitems, size_t size)
{
    void *ptr = calloc(nitems, size);
	wiMemory_CheckAddrLimit(&ptr);
    return ptr;
}

void * wiMemory_AddrLimitRealloc(void *block, size_t size)
{
    void *ptr = realloc(block, size);
	wiMemory_CheckAddrLimit(&ptr);
    return ptr;
}

wiStatus wiMemory_EnabledMemoryAddressLimit(void)
{
    if (Verbose) wiPrintf(" wiMemory_EnabledMemoryAddressLimit()\n");
    wiUtil_WriteLogFile("\n ***** wiMemory_EnabledMemoryAddressLimit *****\n");

XSTATUS( wiMemory_SetMemoryManagement ( wiMemory_AddrLimitMalloc,
                                            wiMemory_AddrLimitCalloc,
                                            wiMemory_AddrLimitRealloc,
                                            free ) );
    return WI_SUCCESS;
}
// end of wiMemory_EnableTraceMemoryManagement()

// ================================================================================================
// FUNCTION  : wiMemory_SetMemoryManagement()
// ------------------------------------------------------------------------------------------------
// Purpose   : Specify functions to be used for memory management.
// Parameters: mallocFn  -- Replaces malloc()
//             callocFn  -- Replaces calloc()
//             reallocFn -- Replaces realloc()
//             freeFn    -- Replaced free()
// ================================================================================================
wiStatus wiMemory_SetMemoryManagement(wiMallocFn  mallocFn,  wiCallocFn callocFn,
                                      wiReallocFn reallocFn, wiFreeFn   freeFn )
{
    if (Verbose) wiPrintf(" wiMemory_SetMemoryManagement(*,*,*,*)\n");

    if (!mallocFn)  return WI_ERROR_PARAMETER1;
    if (!callocFn)  return WI_ERROR_PARAMETER2;
    if (!reallocFn) return WI_ERROR_PARAMETER3;
    if (!freeFn)    return WI_ERROR_PARAMETER4;

    mallocFunction  = mallocFn;
    callocFunction  = callocFn;
    reallocFunction = reallocFn;
    freeFunction    = freeFn;

    wiUtil_WriteLogFile("wiMemory_SetMemoryManagement(%p, %p, %p, %p)\n",
                          mallocFn, callocFn, reallocFn, freeFn );
    return WI_SUCCESS;
}
// end of wiMemory_SetMemoryManagement()

// ================================================================================================
// FUNCTION  : wiMemory_GetMemoryManagement()
// ------------------------------------------------------------------------------------------------
// Purpose   : Retrieve pointers to the memory management functions
// Parameters: mallocFn  -- Replaces malloc()
//             callocFn  -- Replaces calloc()
//             reallocFn -- Replaces realloc()
//             freeFn    -- Replaced free()
// ================================================================================================
wiStatus wiMemory_GetMemoryManagement(wiMallocFn  *mallocFn,  wiCallocFn *callocFn,
                                      wiReallocFn *reallocFn, wiFreeFn   *freeFn )
{
    if (Verbose) wiPrintf(" wiMemory_GetMemoryManagement(**,**,**,**)\n");

    if (mallocFn)  *mallocFn  = mallocFunction;
    if (callocFn)  *callocFn  = callocFunction;
    if (reallocFn) *reallocFn = reallocFunction;
    if (freeFn)    *freeFn    = freeFunction;

    return WI_SUCCESS;
}
// end of wiMemory_GetMemoryManagement()

//-------------------------------------------------------------------------------------------------
//--- End of Source Code --------------------------------------------------------------------------
