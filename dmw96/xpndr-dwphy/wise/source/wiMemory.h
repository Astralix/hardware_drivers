//
//  wiMemory.h -- WISE Memory Allocation Functions and Definitions
//  Copyright 2001 Bermai, 2007-2010 DSP Group, Inc. All rights reserved.
//

#ifndef __WIMEMORY_H
#define __WIMEMORY_H

#include "wiType.h"

#if defined(__cplusplus) || defined(__cplusplus__)
    extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DEFINE MEMORY MANAGEMENT
//
//  These macros allow alternate memory management functions to be substituted for the standard
//  malloc, calloc, and free.
//
//  The WIFREE macro combines common tasks associated with freeing
//  memory: specifically it verifies the pointer is not NULL before freeing the memory and then
//  sets the pointer to NULL after freeing memory.
//
//  WICLEAR_PTR operates as WIFREE but includes a parameter "free" to determine whether the
//  memory reference by the pointer should be freed.
//
//#define WIMEMORY_VERBOSE_ALLOCATION

#if defined WIMEMORY_VERBOSE_ALLOCATION

    #define wiMalloc(n)    wiMemory_VerboseMalloc (n,   __FILE__, __LINE__)
    #define wiCalloc(n,m)  wiMemory_VerboseCalloc (n,m, __FILE__, __LINE__)
    #define wiRealloc(n,m) wiMemory_VerboseRealloc(n,m, __FILE__, __LINE__)
    #define wiFree(n)      wiMemory_VerboseFree   (n,   __FILE__, __LINE__)

#else

    #define wiMalloc(n)    wiMemory_Malloc(n)
    #define wiCalloc(n,m)  wiMemory_Calloc(n,m)
    #define wiRealloc(n,m) wiMemory_Realloc(n,m)
    #define wiFree(n)      wiMemory_Free(n)

#endif

#define WICALLOC(_ptr, _numel, _type)                         \
    {                                                         \
        _ptr = (_type *)wiCalloc(_numel, sizeof(_type));      \
        if (!_ptr) return STATUS(WI_ERROR_MEMORY_ALLOCATION); \
    }                                                         \

#define WIFREE(_ptr)                    \
    {                                   \
        if (_ptr != NULL) wiFree(_ptr); \
        _ptr = NULL;                    \
    }

#define WICLEAR_PTR(_free, _ptr)                   \
    {                                              \
        if (_free && (_ptr != NULL)) wiFree(_ptr); \
        _ptr = NULL;                               \
    }

#define WIMEMORY_CHECK WICHK( wiMemory_Check(__FILE__, __LINE__) );

//=================================================================================================
//
//  FUNCTION PROTOTYPES
//
//=================================================================================================
wiStatus  wiMemory_Init(void);
wiStatus  wiMemory_Close(void);

//---- Set/get a pointer to the internal memory management functions ------------------------------
wiStatus  wiMemory_SetMemoryManagement(wiMallocFn  MallocFn, wiCallocFn  CallocFn, 
                                     wiReallocFn  ReallocFn, wiFreeFn  FreeFn);
wiStatus  wiMemory_GetMemoryManagement(wiMallocFn *MallocFn, wiCallocFn *CallocFn, 
                                     wiReallocFn *ReallocFn, wiFreeFn *FreeFn);

//---- Self-checking memory management ------------------------------------------------------------
wiStatus wiMemory_ListTraceBlocks(void);
wiStatus wiMemory_EnableTraceMemoryManagement(void);
wiStatus wiMemory_SetTraceMemoryManagement(wiMallocFn  MallocFn, wiCallocFn  CallocFn, 
                                           wiReallocFn  ReallocFn, wiFreeFn  FreeFn);
wiStatus wiMemory_Check(char *Filename, int Line);

wiStatus wiMemory_RecordWindowsProcessMemory(void);

//---- Memory Address Limiting
wiStatus wiMemory_EnabledMemoryAddressLimit(void);

void * wiMemory_AddrLimitMalloc (size_t size);
void * wiMemory_AddrLimitCalloc (size_t nitems, size_t size);
void * wiMemory_AddrLimitRealloc(void  *block,  size_t size);


// ------------------------------------------------------------------------------------------------
// Do not call functions below; use wiPrintf(), wiMalloc(), wiCalloc(), wiRealloc(), and wiFree()
// ------------------------------------------------------------------------------------------------
void * wiMemory_Malloc (size_t __size);
void * wiMemory_Calloc (size_t __nitems, size_t __size);
void * wiMemory_Realloc(void * __block, size_t __size);
void   wiMemory_Free   (void * __block);

void * wiMemory_VerboseMalloc (size_t __size, char *Filename, int Line);
void * wiMemory_VerboseCalloc (size_t __nitems, size_t __size, char *Filename, int Line);
void * wiMemory_VerboseRealloc(void * __block, size_t __size, char *Filename, int Line);
void   wiMemory_VerboseFree   (void * __block, char *Filename, int Line);

//-------------------------------------------------------------------------------------------------
#if defined(__cplusplus) || defined(__cplusplus__)
    }
#endif

#endif // __WIMEMORY_H
//-------------------------------------------------------------------------------------------------
//--- End of File ---------------------------------------------------------------------------------
