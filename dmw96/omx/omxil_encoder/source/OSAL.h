/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  Operationg System Abstraction Layer
--
------------------------------------------------------------------------------*/

#ifndef HANTRO_OSAL_H
#define HANTRO_OSAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Types compatible with OMX types */
typedef unsigned long OSAL_U32;
typedef unsigned char OSAL_U8;
typedef void * OSAL_PTR;
typedef unsigned long OSAL_ERRORTYPE;
typedef unsigned long OSAL_BOOL;

/* Error values compatible with OMX_ERRORTYPE */
#define OSAL_ERRORNONE                          0
#define OSAL_ERROR_INSUFFICIENT_RESOURCES       0x80001000
#define OSAL_ERROR_UNDEFINED                    0x80001001
#define OSAL_ERROR_BAD_PARAMETER                0x80001005
#define OSAL_ERROR_NOT_READY                    0x80001010

typedef OSAL_U32 OSAL_BUS_WIDTH;

typedef struct OSAL_ALLOCATOR {
    int fd_mem;             // /dev/mem
    int fd_memalloc;        // /tmp/dev/memalloc
} OSAL_ALLOCATOR;


/*------------------------------------------------------------------------------
    Memory
------------------------------------------------------------------------------*/

OSAL_PTR        OSAL_Malloc( OSAL_U32 size );
void            OSAL_Free( OSAL_PTR pData );

OSAL_PTR        OSAL_Memset( OSAL_PTR pDest, OSAL_U32 ch, OSAL_U32 count);
OSAL_PTR        OSAL_Memcpy( OSAL_PTR pDest, OSAL_PTR pSrc, OSAL_U32 count);


OSAL_ERRORTYPE  OSAL_AllocatorInit(OSAL_ALLOCATOR* alloc);

void            OSAL_AllocatorDestroy(OSAL_ALLOCATOR* alloc);

OSAL_ERRORTYPE  OSAL_AllocatorAllocMem(OSAL_ALLOCATOR* alloc,
                    OSAL_U32* size, OSAL_U8** bus_data,
                    OSAL_BUS_WIDTH* bus_address);

void            OSAL_AllocatorFreeMem(OSAL_ALLOCATOR* alloc,
                    OSAL_U32 size, OSAL_U8* bus_data,
                    OSAL_BUS_WIDTH bus_address);

OSAL_BOOL       OSAL_AllocatorIsReady(const OSAL_ALLOCATOR* alloc);


/*------------------------------------------------------------------------------
    Thread
------------------------------------------------------------------------------*/

OSAL_ERRORTYPE  OSAL_ThreadCreate(OSAL_U32 (*pFunc)(OSAL_PTR pParam),
                    OSAL_PTR pParam, OSAL_U32 nPriority, OSAL_PTR *phThread );

OSAL_ERRORTYPE  OSAL_ThreadDestroy(OSAL_PTR hThread );
void            OSAL_ThreadSleep(OSAL_U32 ms);

/*------------------------------------------------------------------------------
    Mutex
------------------------------------------------------------------------------*/

OSAL_ERRORTYPE  OSAL_MutexCreate(OSAL_PTR *phMutex);
OSAL_ERRORTYPE  OSAL_MutexDestroy(OSAL_PTR hMutex);
OSAL_ERRORTYPE  OSAL_MutexLock(OSAL_PTR hMutex);
OSAL_ERRORTYPE  OSAL_MutexUnlock(OSAL_PTR hMutex);

/*------------------------------------------------------------------------------
    Events
------------------------------------------------------------------------------*/

#define INFINITE_WAIT 0xffffffff

OSAL_ERRORTYPE  OSAL_EventCreate(OSAL_PTR *phEvent);
OSAL_ERRORTYPE  OSAL_EventDestroy(OSAL_PTR hEvent);
OSAL_ERRORTYPE  OSAL_EventReset(OSAL_PTR hEvent);
OSAL_ERRORTYPE  OSAL_EventSet(OSAL_PTR hEvent);
OSAL_ERRORTYPE  OSAL_EventWait(OSAL_PTR hEvent, OSAL_U32 mSec,
                    OSAL_BOOL *pbTimedOut);

OSAL_ERRORTYPE  OSAL_EventWaitMultiple(OSAL_PTR* hEvents, OSAL_BOOL* bSignaled,
                    OSAL_U32 nCount, OSAL_U32 mSec, OSAL_BOOL* pbTimedOut);

/*------------------------------------------------------------------------------
    Time
------------------------------------------------------------------------------*/

OSAL_U32        OSAL_GetTime();


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

