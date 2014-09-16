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
//#define LOG_NDEBUG 0
#define LOG_TAG "omxil_decoder_OSAL"
#include <utils/Log.h>
#define TRACE LOGV
#define TRACE_PRINT LOGV

#include "OSAL.h"


#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <signal.h>

#ifdef MEMALLOCHW
#include <memalloc.h>
#endif

#ifdef OMX_MEM_TRC
 #include <stdio.h>
 FILE * pf = NULL;
#endif

#ifdef ANDROID
#include "dwl_linux.h"
#include "dwl.h"
#endif

/*------------------------------------------------------------------------------
    Definitions
------------------------------------------------------------------------------*/

#define MEMORY_SENTINEL 0xACDCACDC;

typedef struct OSAL_THREADDATATYPE {
    pthread_t oPosixThread;
    pthread_attr_t oThreadAttr;
    OSAL_U32 (*pFunc)(OSAL_PTR pParam);
    OSAL_PTR pParam;
    OSAL_U32 uReturn;
} OSAL_THREADDATATYPE;

typedef struct {
    OSAL_BOOL       bSignaled;
    pthread_mutex_t mutex;
    int             fd[2];
} OSAL_THREAD_EVENT;

/*------------------------------------------------------------------------------
    External compiler flags
--------------------------------------------------------------------------------

MEMALLOCHW: If defined we will use kernel driver for HW memory allocation


--------------------------------------------------------------------------------
    OSAL_Malloc
------------------------------------------------------------------------------*/
OSAL_PTR OSAL_Malloc( OSAL_U32 size )
{
    OSAL_U32 extra = sizeof(OSAL_U32) * 2;
    OSAL_U8*  data = (OSAL_U8*)malloc(size + extra);
    OSAL_U32 sentinel = MEMORY_SENTINEL;

    memcpy(data, &size, sizeof(size));
    memcpy(&data[size + sizeof(size)], &sentinel, sizeof(sentinel));

    return data + sizeof(size);
}

/*------------------------------------------------------------------------------
    OSAL_Free
------------------------------------------------------------------------------*/
void OSAL_Free( OSAL_PTR pData )
{
    OSAL_U8* block    = ((OSAL_U8*)pData) - sizeof(OSAL_U32);
#ifdef ASSERT
    OSAL_U32 sentinel = MEMORY_SENTINEL;
    OSAL_U32 size     = *((OSAL_U32*)block);

    assert(memcmp(&block[size+sizeof(size)],&sentinel, sizeof(sentinel))==0 &&
            "mem corruption detected");
#endif

    free(block);
}

/*------------------------------------------------------------------------------
    OSAL_AllocatorInit
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_AllocatorInit(OSAL_ALLOCATOR* alloc)
{
#ifdef OMX_MEM_TRC

        pf = fopen("omx_mem_trc.csv", "w");
        if(pf)
            fprintf(pf,
            "linead memory usage in bytes;linear memory usage in 4096 pages;\n");

#endif
#ifdef ANDROID
    if(alloc->pdwl)
        return OSAL_ERRORNONE;

    TRACE("OSAL_Init\n");

    DWLInitParam_t dwlInit;

    dwlInit.clientType = DWL_CLIENT_TYPE_H264_DEC;

    alloc->pdwl = (void*)DWLInit(&dwlInit);


    // NOTE: error handling
    return OSAL_ERRORNONE;
#endif
#ifdef MEMALLOCHW
    OSAL_ERRORTYPE err = OSAL_ERRORNONE;
    // open memalloc for linear memory allocation
    alloc->fd_memalloc = open("/tmp/dev/memalloc", O_RDWR | O_SYNC);
    //alloc->fd_memalloc = open("/dev/memalloc", O_RDWR | O_SYNC);
    if (alloc->fd_memalloc == -1)
    {
        TRACE_PRINT("memalloc not found\n");
        err = OSAL_ERROR_UNDEFINED;
        goto FAIL;
    }
    // open raw memory for memory mapping
    alloc->fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    //alloc->fd_mem = open("/dev/uio0", O_RDWR | O_SYNC);
    if (alloc->fd_mem == -1)
    {
        TRACE_PRINT("uio0 not found\n");
        err = OSAL_ERROR_UNDEFINED;
        goto FAIL;
    }
    return OSAL_ERRORNONE;
 FAIL:
    if (alloc->fd_memalloc > 0) close(alloc->fd_memalloc);
    if (alloc->fd_mem > 0)      close(alloc->fd_mem);
    return err;
#else
    return OSAL_ERRORNONE;
#endif
}

/*------------------------------------------------------------------------------
    OSAL_AllocatorDestroy
------------------------------------------------------------------------------*/
void OSAL_AllocatorDestroy(OSAL_ALLOCATOR* alloc)
{
#ifdef OMX_MEM_TRC

    if(pf)
    {
        fclose(pf);
        pf =NULL;
    }

#endif
#ifdef ANDROID
    assert(alloc);
    DWLRelease(alloc->pdwl);
    alloc->pdwl = NULL;
#endif
#ifdef MEMALLOCHW
    assert(alloc->fd_memalloc > 0);
    assert(alloc->fd_mem > 0);
    int ret = 0;
    ret = close(alloc->fd_memalloc);
    assert( ret == 0 );
    ret= close(alloc->fd_mem);
    assert( ret == 0 );

    alloc->fd_memalloc = 0;
    alloc->fd_mem      = 0;
#endif
}

/*------------------------------------------------------------------------------
    OSAL_AllocatorAllocMem
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_AllocatorAllocMem(OSAL_ALLOCATOR* alloc, OSAL_U32* size,
        OSAL_U8** bus_data, OSAL_BUS_WIDTH* bus_address)
{
#ifdef OMX_MEM_TRC

    if(pf)
        fprintf(pf, "%d bytes; %d chunks;\n", *size, *size/4096);

#endif
#ifdef ANDROID

    DWLLinearMem_t info;
 //   memset(&info, 0, sizeof(DWLLinearMem_t));
    TRACE("OSAL_AllocatorAllocMem size=%d\n",*size);

    if(alloc->pdwl == 0)
    {
        OSAL_ALLOCATOR a;
        OSAL_AllocatorInit(&a);
    }

    int ret = DWLMallocLinear(alloc->pdwl, *size, &info);
    if (ret == 0)
    {
        *bus_data = (OSAL_U8*)info.virtualAddress;
        *bus_address = (OSAL_BUS_WIDTH)info.busAddress;
        TRACE("OSAL_AllocatorAllocMem OK\n    bus addr = 0x%08x\n    vir addr = 0x%08x\n",info.busAddress, info.virtualAddress);
        memset(info.virtualAddress, 0xab, *size);
        return OSAL_ERRORNONE;
    }
    else
    {
        LOGE("MallocLinear error %d\n",ret);
        return OSAL_ERROR_UNDEFINED;
    }
#endif
#ifdef MEMALLOCHW
    assert(alloc->fd_memalloc > 0);
    assert(alloc->fd_mem > 0);
    int pgsize = getpagesize();

    MemallocParams params;
    memset(&params, 0, sizeof(MemallocParams));

    *size = (*size + pgsize) & (~(pgsize - 1));
    params.size = *size;
    *bus_data   = MAP_FAILED;
    // get linear memory buffer
    ioctl(alloc->fd_memalloc, MEMALLOC_IOCXGETBUFFER, &params);
    if (params.busAddress == 0)
    {
        LOGE("OSAL_ERROR_INSUFFICIENT_RESOURCES\n");
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;
    }
    // map the bus address to a virtual address

    TRACE_PRINT("alloc success. bus addr = 0x%x\n",params.busAddress);

    TRACE_PRINT("mmap(0, %d, %x, %x, %d, 0x%x);\n", *size, PROT_READ | PROT_WRITE, MAP_SHARED, alloc->fd_mem, params.busAddress);
    *bus_data = (OSAL_U8*)mmap(0, *size, PROT_READ | PROT_WRITE, MAP_SHARED, alloc->fd_mem, params.busAddress);
    if (*bus_data == MAP_FAILED)
    {
        TRACE_PRINT("mmap failed %s\n",strerror(errno));
        return OSAL_ERROR_UNDEFINED;
    }

    TRACE_PRINT("mmap success. vir addr = 0x%x\n",*bus_data);
    memset(*bus_data, 0, *size);

    TRACE_PRINT("memset OK\n");
    *bus_address = params.busAddress;
    return OSAL_ERRORNONE;
#else
    OSAL_U32 extra = sizeof(OSAL_U32);
    OSAL_U8* data  = (OSAL_U8*)malloc(*size + extra);
    if (data==NULL)
    {
        LOGE("OSAL_ERROR_INSUFFICIENT_RESOURCES\n");
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;
    }

    OSAL_U32 sentinel = MEMORY_SENTINEL;
    // copy sentinel at the end of mem block
    memcpy(&data[*size], &sentinel, sizeof(OSAL_U32));

    *bus_data    = data;
    *bus_address = (OSAL_BUS_WIDTH)data;
    return OSAL_ERRORNONE;
#endif
}

/*------------------------------------------------------------------------------
    OSAL_AllocatorFreeMem
------------------------------------------------------------------------------*/
void OSAL_AllocatorFreeMem(OSAL_ALLOCATOR* alloc, OSAL_U32 size,
        OSAL_U8* bus_data, OSAL_BUS_WIDTH bus_address)
{
#ifdef ANDROID
    TRACE("OSAL_AllocatorFreeMem\b");
	DWLLinearMem_t info;
	info.size = size;
	info.virtualAddress = (u32*)bus_data;
	info.busAddress = bus_address;

	DWLFreeLinear(alloc->pdwl, &info);

	TRACE("OSAL_AllocatorFreeMem %x ok\n", bus_address);
    return;
#endif
#ifdef MEMALLOCHW
    assert(alloc->fd_memalloc > 0);
    assert(alloc->fd_mem > 0);

    if (bus_address)
        ioctl(alloc->fd_memalloc, MEMALLOC_IOCSFREEBUFFER, &bus_address);
    if (bus_data != MAP_FAILED)
        munmap(bus_data, size);

#else
#ifdef ASSERT
    assert(((OSAL_BUS_WIDTH)bus_data) == bus_address);
    OSAL_U32 sentinel = MEMORY_SENTINEL;
    assert(memcmp(&bus_data[size], &sentinel, sizeof(OSAL_U32)) == 0 &&
            "memory corruption detected");
#endif
    free(bus_data);
#endif
}


/*------------------------------------------------------------------------------
    OSAL_AllocatorIsReady
------------------------------------------------------------------------------*/
OSAL_BOOL OSAL_AllocatorIsReady(const OSAL_ALLOCATOR* alloc)
{
#ifdef MEMALLOCHW
    if (alloc->fd_memalloc > 0 &&
        alloc->fd_mem > 0)
        return 1;
    return 0;
#endif

#ifdef ANDROID
    if(alloc->pdwl)
        return 1;
    else
        return 0;
#endif

    return 1;
}


/*------------------------------------------------------------------------------
    OSAL_Memset
------------------------------------------------------------------------------*/
OSAL_PTR OSAL_Memset( OSAL_PTR pDest, OSAL_U32 cChar, OSAL_U32 nCount)
{
    return memset(pDest, cChar, nCount);
}

/*------------------------------------------------------------------------------
    OSAL_Memcpy
------------------------------------------------------------------------------*/
OSAL_PTR OSAL_Memcpy( OSAL_PTR pDest, OSAL_PTR pSrc, OSAL_U32 nCount)
{
    return memcpy(pDest, pSrc, nCount);
}

/*------------------------------------------------------------------------------
    BlockSIGIO      Linux EWL uses SIGIO to signal interrupt
------------------------------------------------------------------------------*/
static void BlockSIGIO()
{
    sigset_t set, oldset;

    /* Block SIGIO from the main thread to make sure that it will be handled
     * in the encoding thread */

    sigemptyset(&set);
    sigemptyset(&oldset);
    sigaddset(&set, SIGIO);
    pthread_sigmask(SIG_BLOCK, &set, &oldset);

}

/*------------------------------------------------------------------------------
    threadFunc
------------------------------------------------------------------------------*/
static void *threadFunc(void *pParameter)
{
    OSAL_THREADDATATYPE *pThreadData;
    pThreadData = (OSAL_THREADDATATYPE *)pParameter;
    pThreadData->uReturn = pThreadData->pFunc(pThreadData->pParam);
    return pThreadData;
}

/*------------------------------------------------------------------------------
    OSAL_ThreadCreate
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_ThreadCreate( OSAL_U32 (*pFunc)(OSAL_PTR pParam),
        OSAL_PTR pParam, OSAL_U32 nPriority, OSAL_PTR *phThread )
{
    OSAL_THREADDATATYPE *pThreadData;
    struct sched_param sched;

    pThreadData = (OSAL_THREADDATATYPE*)OSAL_Malloc(sizeof(OSAL_THREADDATATYPE));
    if (pThreadData == NULL)
    {
        LOGE("OSAL_ERROR_INSUFFICIENT_RESOURCES\n");
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;
    }

    pThreadData->pFunc = pFunc;
    pThreadData->pParam = pParam;
    pThreadData->uReturn = 0;

    pthread_attr_init(&pThreadData->oThreadAttr);

    pthread_attr_getschedparam(&pThreadData->oThreadAttr, &sched);
    sched.sched_priority += nPriority;
    pthread_attr_setschedparam(&pThreadData->oThreadAttr, &sched);

    if (pthread_create(&pThreadData->oPosixThread,
                       &pThreadData->oThreadAttr,
                       threadFunc,
                       pThreadData)) {
        LOGE("OSAL_ERROR_INSUFFICIENT_RESOURCES\n");
        OSAL_Free(pThreadData);
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;
    }

    BlockSIGIO();

    *phThread = (OSAL_PTR)pThreadData;
    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_ThreadDestroy
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_ThreadDestroy( OSAL_PTR hThread )
{
    OSAL_THREADDATATYPE *pThreadData = (OSAL_THREADDATATYPE *)hThread;
    void *retVal=&pThreadData->uReturn;

    if (pThreadData == NULL)
        return OSAL_ERROR_BAD_PARAMETER;

    //pthread_cancel(pThreadData->oPosixThread);

    if (pthread_join(pThreadData->oPosixThread, &retVal)) {
        return OSAL_ERROR_BAD_PARAMETER;
    }

    OSAL_Free(pThreadData);
    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_ThreadSleep
------------------------------------------------------------------------------*/
void OSAL_ThreadSleep(OSAL_U32 ms)
{
    usleep(ms*1000);
}

/*------------------------------------------------------------------------------
    OSAL_MutexCreate
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_MutexCreate(OSAL_PTR *phMutex)
{
    pthread_mutex_t *pMutex = (pthread_mutex_t *)
                                OSAL_Malloc(sizeof(pthread_mutex_t));
    static pthread_mutexattr_t oAttr;
    static pthread_mutexattr_t *pAttr = NULL;

    if (pAttr == NULL &&
        !pthread_mutexattr_init(&oAttr) &&
        !pthread_mutexattr_settype(&oAttr, PTHREAD_MUTEX_RECURSIVE))
    {
        pAttr = &oAttr;
    }

    if (pMutex == NULL)
    {
        LOGE("OSAL_ERROR_INSUFFICIENT_RESOURCES\n");
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;
    }

    if (pthread_mutex_init(pMutex, pAttr)) {
        LOGE("OSAL_ERROR_INSUFFICIENT_RESOURCES\n");
        OSAL_Free(pMutex);
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;
    }

    *phMutex = (void *)pMutex;
    return OSAL_ERRORNONE;
}


/*------------------------------------------------------------------------------
    OSAL_MutexDestroy
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_MutexDestroy(OSAL_PTR hMutex)
{
    pthread_mutex_t *pMutex = (pthread_mutex_t *)hMutex;

    if (pMutex == NULL)
        return OSAL_ERROR_BAD_PARAMETER;

    if (pthread_mutex_destroy(pMutex)) {
        return OSAL_ERROR_BAD_PARAMETER;
    }

    OSAL_Free(pMutex);
    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_MutexLock
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_MutexLock(OSAL_PTR hMutex)
{
    pthread_mutex_t *pMutex = (pthread_mutex_t *)hMutex;
    int err;

    if (pMutex == NULL)
        return OSAL_ERROR_BAD_PARAMETER;

    err = pthread_mutex_lock(pMutex);
    switch (err) {
    case 0:
        return OSAL_ERRORNONE;
    case EINVAL:
        return OSAL_ERROR_BAD_PARAMETER;
    case EDEADLK:
        return OSAL_ERROR_NOT_READY;
    default:
        return OSAL_ERROR_UNDEFINED;
    }

    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_MutexUnlock
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_MutexUnlock(OSAL_PTR hMutex)
{
    pthread_mutex_t *pMutex = (pthread_mutex_t *)hMutex;
    int err;

    if (pMutex == NULL)
        return OSAL_ERROR_BAD_PARAMETER;

    err = pthread_mutex_unlock(pMutex);
    switch (err) {
    case 0:
        return OSAL_ERRORNONE;
    case EINVAL:
        return OSAL_ERROR_BAD_PARAMETER;
    case EPERM:
        return OSAL_ERROR_NOT_READY;
    default:
        return OSAL_ERROR_UNDEFINED;
    }

    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_EventCreate
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_EventCreate(OSAL_PTR *phEvent)
{
    OSAL_THREAD_EVENT *pEvent = OSAL_Malloc(sizeof(OSAL_THREAD_EVENT));

    if (pEvent == NULL)
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;

    pEvent->bSignaled = 0;

    if (pipe(pEvent->fd) == -1)
    {
        OSAL_Free(pEvent);
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;
    }

    if (pthread_mutex_init(&pEvent->mutex, NULL))
    {
        close(pEvent->fd[0]);
        close(pEvent->fd[1]);
        OSAL_Free(pEvent);
        return OSAL_ERROR_INSUFFICIENT_RESOURCES;
    }

    *phEvent = (OSAL_PTR)pEvent;
    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_EventDestroy
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_EventDestroy(OSAL_PTR hEvent)
{
    OSAL_THREAD_EVENT *pEvent = (OSAL_THREAD_EVENT *)hEvent;
    if (pEvent == NULL)
        return OSAL_ERROR_BAD_PARAMETER;

    if (pthread_mutex_lock(&pEvent->mutex))
        return OSAL_ERROR_BAD_PARAMETER;

    int err = 0;
    err = close(pEvent->fd[0]); assert(err == 0);
    err = close(pEvent->fd[1]); assert(err == 0);

    pthread_mutex_unlock(&pEvent->mutex);
    pthread_mutex_destroy(&pEvent->mutex);

    OSAL_Free(pEvent);
    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_EventReset
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_EventReset(OSAL_PTR hEvent)
{
    OSAL_THREAD_EVENT *pEvent = (OSAL_THREAD_EVENT *)hEvent;
    if (pEvent == NULL)
        return OSAL_ERROR_BAD_PARAMETER;

    if (pthread_mutex_lock(&pEvent->mutex))
        return OSAL_ERROR_BAD_PARAMETER;

    if (pEvent->bSignaled)
    {
        // empty the pipe
        char c = 1;
        int ret = read(pEvent->fd[0], &c, 1);
        if (ret == -1)
            return OSAL_ERROR_UNDEFINED;
        pEvent->bSignaled = 0;
    }

    pthread_mutex_unlock(&pEvent->mutex);
    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_GetTime
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_EventSet(OSAL_PTR hEvent)
{
    OSAL_THREAD_EVENT *pEvent = (OSAL_THREAD_EVENT *)hEvent;
    if (pEvent == NULL)
        return OSAL_ERROR_BAD_PARAMETER;

    if (pthread_mutex_lock(&pEvent->mutex))
        return OSAL_ERROR_BAD_PARAMETER;

    if (!pEvent->bSignaled)
    {
        char c = 1;
        int ret = write(pEvent->fd[1], &c, 1);
        if (ret == -1)
            return OSAL_ERROR_UNDEFINED;
        pEvent->bSignaled = 1;
    }

    pthread_mutex_unlock(&pEvent->mutex);
    return OSAL_ERRORNONE;
}

/*------------------------------------------------------------------------------
    OSAL_EventWait
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_EventWait(OSAL_PTR hEvent, OSAL_U32 uMsec,
        OSAL_BOOL* pbTimedOut)
{
    OSAL_BOOL signaled = 0;
    return OSAL_EventWaitMultiple(&hEvent, &signaled, 1, uMsec, pbTimedOut);
}

/*------------------------------------------------------------------------------
    OSAL_EventWaitMultiple
------------------------------------------------------------------------------*/
OSAL_ERRORTYPE OSAL_EventWaitMultiple(OSAL_PTR* hEvents,
        OSAL_BOOL* bSignaled, OSAL_U32 nCount, OSAL_U32 mSecs,
        OSAL_BOOL* pbTimedOut)
{
    assert(hEvents);
    assert(bSignaled);

    fd_set read;
    FD_ZERO(&read);

    int max=0;
    unsigned i=0;
    for (i=0; i<nCount; ++i)
    {
        OSAL_THREAD_EVENT* pEvent = (OSAL_THREAD_EVENT*)(hEvents[i]);

        if (pEvent == NULL)
            return OSAL_ERROR_BAD_PARAMETER;

        int fd = pEvent->fd[0];
        if (fd > max)
            max = fd;

        FD_SET(fd, &read);
    }

    if (mSecs == INFINITE_WAIT)
    {
        int ret = select(max+1, &read, NULL, NULL, NULL);
        if (ret == -1)
            return OSAL_ERROR_UNDEFINED;
    }
    else
    {
        struct timeval tv;
        memset(&tv, 0, sizeof(struct timeval));
        tv.tv_usec = mSecs * 1000;
        int ret = select(max+1, &read, NULL, NULL, &tv);
        if (ret == -1)
            return OSAL_ERROR_UNDEFINED;
        if (ret == 0)
        {
            *pbTimedOut =  1;
        }
    }

    for (i=0; i<nCount; ++i)
    {
        OSAL_THREAD_EVENT* pEvent = (OSAL_THREAD_EVENT*)hEvents[i];

        if (pEvent == NULL)
            return OSAL_ERROR_BAD_PARAMETER;

        int fd = pEvent->fd[0];
        if (FD_ISSET(fd, &read))
            bSignaled[i] = 1;
        else
            bSignaled[i] = 0;
    }

    return OSAL_ERRORNONE;
}


/*------------------------------------------------------------------------------
    OSAL_GetTime
------------------------------------------------------------------------------*/
OSAL_U32 OSAL_GetTime()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return ((OSAL_U32)now.tv_sec) * 1000 + ((OSAL_U32)now.tv_usec) / 1000;
}

