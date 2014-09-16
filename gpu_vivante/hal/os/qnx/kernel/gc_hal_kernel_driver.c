/****************************************************************************
*
*    Copyright (C) 2005 - 2012 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/




#include "gc_hal_kernel_qnx.h"
#include "gc_hal_driver.h"

# include <screen/gpu.h>

static gckGALDEVICE galDevice;

/* GN TODO take these from the graphics.conf file. */
/*#ifdef MMP2*/

/* IMX6X */
#ifdef IMX6X
int irqLine = 41;
long registerMemBase = 0x00130000;
unsigned long registerMemSize = (2 << 10);            /* GPU register memory size. */
unsigned long baseAddress = 0x10000000;
unsigned long physSize = (2 << 30);
#else
#ifdef MMP2
int irqLine = 8;
long registerMemBase = 0xd420d000;
unsigned long registerMemSize = (2 << 10);            /* GPU register memory size. */
/* If mmap returns addresses in 0x80000000+ range, then baseAddress needs to be 0x80000000.
 * otherwise it needs to be 0x0.
 */
unsigned long baseAddress = 0x0;
unsigned long physSize = 0;
#else
#error Board type not defined. (needed for setting irq etc)
#endif
#endif

#ifdef IMX6X
int irqLine2D = 42;
long registerMemBase2D = 0x00134000;
unsigned long registerMemSize2D = (2 << 10);            /* GPU register memory size. */
#else
int irqLine2D = -1;
long registerMemBase2D = 0;
long registerMemSize2D = 0;
#endif

#ifdef IMX6X
int irqLineVG = 43;
long registerMemBaseVG = 0x02204000;
unsigned long registerMemSizeVG = (2 << 10);            /* GPU register memory size. */
#else
int irqLineVG = -1;
long registerMemBaseVG = 0;
long registerMemSizeVG = 0;
#endif

/* Configurable Memory sizes. */
#if gcdUSE_VIDMEM_PER_PID
unsigned long contiguousSize  =           0;            /* Video memory pool. */
#else
unsigned long contiguousSize  = (256 << 20);            /* Video memory pool. */
#endif

unsigned int internalPoolSize = (  2 << 20);            /* Kernel local memory pool. */
unsigned int sharedPoolSize   = (  2 << 20);            /* Shared per-client memory pool initial size. */
unsigned int sharedPoolPageSize   = (  1 << 12);        /* Shared per-client memory pool page size. */
unsigned int internalPoolPageSize = (  1 << 12);        /* Kernel local memory pool page size. */

/* ContiguousBase should be 0,
 * for video memory to be allocated from the memory pool. */
unsigned long contiguousBase = 0;
long bankSize = 0;
int fastClear = -1;
int compression = -1;

/*----------------------------------------------------------------------------*/
/*---------------------------- Global Memory Pool ----------------------------*/
/* Global video memory pool. */
typedef struct _gcsMEM_POOL
{
    gctINT32 freePage;
    gctSIZE_T pageCount;
    gctUINT32 pageSize;
    gctUINT32 poolSize;
    pthread_mutex_t mutex;
    gctUINT32 addr;
    gctUINT32 paddr;
    gckPAGE_USAGE pageUsage;
    gctCHAR fdName[64];
    gctINT fd;
} gcsMEM_POOL;

gcsMEM_POOL memPool;
gceSTATUS gckVIDMEM_FreeHandleMemory(IN gckKERNEL Kernel, IN gckVIDMEM Memory, IN gctUINT32 Pid);


/* Reference count handling per pid.
 * To invoke startup/cleanup on first and last connection respectively.
 */
struct pid_hash
{
    pid_t pid;
    int refCount;
    struct pid_hash* next;
};
struct pid_hash* pid_hash_head;

gceSTATUS
drv_mempool_init()
{
    off64_t paddr;
    void* addr;
    size_t pcontig;

    memPool.pageSize = internalPoolPageSize;

    /* Compute number of pages. */
    memPool.pageCount = (contiguousSize + internalPoolSize) / memPool.pageSize;
    gcmkASSERT(memPool.pageCount <= 65536);

    /* Align memPoolSize to page size. */
    memPool.poolSize = memPool.pageCount * memPool.pageSize;
    /*printf("memPoolSize: %d\n", memPool.poolSize);*/

    /* Allocate a single chunk of physical memory.
     * Zero memory with MAP_ANON so we don't leak any sensitive information by chance. */
    snprintf(memPool.fdName, sizeof(memPool.fdName), "galcore:vidmem:%d", getpid());
    memPool.fd = shm_open(memPool.fdName, O_RDWR|O_CREAT, 0777);
    if (memPool.fd == -1) {
        fprintf(stderr, "galcore:%s[%d]: shm_open failed\n", __FUNCTION__, __LINE__);
        return gcvSTATUS_GENERIC_IO;
    }

    shm_unlink(memPool.fdName);

    /**
     * TODO: Re-enable write-combine.
     * Should it be 0x9 for ARM??
     *
     * The 0x41 is intended for normal memory that does not have side effects so
     * the restrictions are different to Shared Device.
     *
     * TODO: Special values should be ifdef ARM, or otherwise abstracted.
     */
    if (shm_ctl_special(memPool.fd, SHMCTL_ANON|SHMCTL_PHYS, 0, memPool.poolSize, 0x5) == -1) {
        fprintf(stderr, "galcore:%s[%d]: shm_ctl_special failed\n", __FUNCTION__, __LINE__);
        close(memPool.fd);
        memPool.fd = -1;
        return gcvSTATUS_GENERIC_IO;
    }

    addr = mmap64(0, memPool.poolSize, PROT_READ|PROT_WRITE, MAP_SHARED, memPool.fd, 0);

    if (addr == MAP_FAILED) {
        fprintf(stderr, "galcore:%s[%d]: mmap64 failed, errno=%d.\n", __FUNCTION__, __LINE__, errno);
        close(memPool.fd);
        memPool.fd = -1;
        return gcvSTATUS_GENERIC_IO;
    }
    memPool.addr = (gctUINT32)addr;

    if (mem_offset64(addr, NOFD, memPool.poolSize, &paddr, &pcontig) == -1)
    {
        fprintf(stderr, "galcore:%s[%d]: mem_offset64 failed\n", __FUNCTION__, __LINE__);
        munmap(addr, memPool.poolSize);
        close(memPool.fd);
        memPool.fd = -1;
        memPool.addr = NULL;
        return gcvSTATUS_GENERIC_IO;
    }

    /* TODO. Truncating 64bit value. */
    memPool.paddr = (gctUINT32)paddr;

    printf( "Mempool Map addr range[%x-%x]\n", memPool.addr, memPool.addr +  memPool.poolSize);
    printf( "Mempool Map paddr range[%x-%x]\n", memPool.paddr, memPool.paddr +  memPool.poolSize );

    /* Allocate the page usage array and Initialize all pages to free. */
    memPool.pageUsage = (gckPAGE_USAGE)calloc(
            memPool.pageCount,
            sizeof(struct _gckPAGE_USAGE));

    if (memPool.pageUsage == gcvNULL)
    {
        fprintf( stderr, "malloc failed: %s\n", strerror( errno ) );
        munmap(addr, memPool.poolSize);
        close(memPool.fd);
        memPool.fd = -1;
        memPool.addr = NULL;
        memPool.paddr = 0;
        return gcvSTATUS_GENERIC_IO;
    }

    /* The first page is free.*/
    memPool.freePage = 0;

    /* Initialize the semaphore. */
    if (pthread_mutex_init(&memPool.mutex, NULL) != EOK)
    {
        free(memPool.pageUsage);
        munmap(addr, memPool.poolSize);
        close(memPool.fd);
        memPool.fd = -1;
        memPool.addr = NULL;
        memPool.paddr = 0;
        return gcvSTATUS_GENERIC_IO;
    }

    return gcvSTATUS_OK;
}

void
drv_mempool_destroy()
{
    pthread_mutex_destroy(&memPool.mutex);
    free(memPool.pageUsage);
    memPool.pageUsage = NULL;
    munmap((void*)memPool.addr, memPool.poolSize);
    close(memPool.fd);
    memPool.fd = -1;
    memPool.addr = NULL;
    memPool.paddr = 0;
}

gctINT
drv_mempool_get_fileDescriptor()
{
    return memPool.fd;
}

gctUINT32
drv_mempool_get_basePAddress()
{
    return memPool.paddr;
}

gctUINT32
drv_mempool_get_baseAddress()
{
    return memPool.addr;
}

gctUINT32
drv_mempool_get_page_size()
{
    return memPool.pageSize;
}

gceSTATUS
drv_mempool_mem_offset(
    IN gctPOINTER Logical,
    OUT gctUINT32 * Address)
{
    gctUINT32 logical = (gctUINT32)Logical;

    if ( Address == gcvNULL )
        return gcvSTATUS_INVALID_ARGUMENT;

    if ( logical < memPool.addr
        ||  logical > (memPool.addr + memPool.poolSize))
        return gcvSTATUS_INVALID_ARGUMENT;

    *Address = (logical - memPool.addr) + memPool.paddr;

    return gcvSTATUS_OK;
}

/* Allocate pages from mapped shared memory.
   Return Physical and Logical addresses.
*/
void
drv_mempool_alloc_contiguous(
    IN gctUINT32 Bytes,
    OUT gctPHYS_ADDR * Physical,
    OUT gctPOINTER * Logical
    )
{
    gctSIZE_T i, j;
    gctSIZE_T pageCount;

    pthread_mutex_lock(&memPool.mutex);

    /* Compute the number of required pages. */
    pageCount = gcmALIGN(Bytes, drv_mempool_get_page_size()) / drv_mempool_get_page_size();

    if ( (pageCount <= 0) || (memPool.freePage < 0) )
    {
        /*printf("%s[%d] No free pages left!\n", __FUNCTION__, __LINE__);*/
        *Physical = gcvNULL;
        *Logical = gcvNULL;
        pthread_mutex_unlock(&memPool.mutex);
        /* No free pages left. */
        return;
    }

    /* Try finding enough contiguous free pages. */
    for (i = memPool.freePage; i < memPool.pageCount;)
    {
        /* All pages behind this free page should be free. */
        gctSIZE_T j;
        for (j = 1; j < pageCount; ++j)
        {
            if (memPool.pageUsage[i + j].pageCount != 0)
            {
                /* Bail out if page is allocated. */
                break;
            }
        }

        if (j == pageCount)
        {
            /* We found a spot that has enough free pages. */
            break;
        }

        /* Move to the page after the allocated page. */
        i += j + 1;

        /* Find the next free page. */
        while ((i < memPool.pageCount) && (memPool.pageUsage[i].pageCount != 0))
        {
            ++i;
        }
    }

    if (i >= memPool.pageCount)
    {
        /*printf("Not enough contiguous pages. pageCount:%d\n", (int)memPool.pageCount);*/
        *Physical = gcvNULL;
        *Logical = gcvNULL;
        pthread_mutex_unlock(&memPool.mutex);
        /* Not enough contiguous pages. */
        return;
    }

    /* Check if we allocate from the first free page. */
    if (i == memPool.freePage)
    {
        /* Move first free page to beyond the contiguous request. */
        memPool.freePage = i + pageCount;

        /* Find first free page. */
        while ( (memPool.freePage < memPool.pageCount) &&
                (memPool.pageUsage[memPool.freePage].pageCount != 0) )
        {
            ++memPool.freePage;
        }

        if (memPool.freePage >= memPool.pageCount)
        {
            /* No more free pages. */
            memPool.freePage = -1;
        }
    }

    /* Walk all pages. */
    for (j = 0; j < pageCount; ++j)
    {
        /* Store page count in each pageUsage to mark page is allocated. */
        memPool.pageUsage[i+j].pageCount = pageCount;
    }

    gcmkTRACE(gcvLEVEL_INFO, "Allocated %u contiguous pages from 0x%X\n",
        pageCount, i);

    /*printf("%s[%d] Allocated %u contiguos pages from 0x%X\n", __FUNCTION__, __LINE__, (gctUINT32)pageCount, (gctUINT32)i);*/

    /* Success. */
    *Physical = (gctPHYS_ADDR)(i * memPool.pageSize + (gctUINT32)memPool.paddr);
    *Logical = (gctPOINTER)(i * memPool.pageSize + (gctUINT32)memPool.addr);

    pthread_mutex_unlock(&memPool.mutex);
}

int drv_mempool_free(gctPOINTER Logical)
{
    gctUINT16 pageCount;
    gctSIZE_T i;
    gctINT32 pageIndex;

    gcmkTRACE(gcvLEVEL_INFO, "Freeing pages @ %x\n", Logical);

    pthread_mutex_lock(&memPool.mutex);

    pageIndex = ((gctUINT32)Logical - (gctUINT32)memPool.addr) / memPool.pageSize;

    /* Verify the memory is valid and unlocked. */
    if ( (pageIndex < 0) || (pageIndex >= memPool.pageCount) )
    {
        pthread_mutex_unlock(&memPool.mutex);
        /*printf("%s[%d] Invalid page index @ %d\n", __FUNCTION__, __LINE__, pageIndex);*/
        gcmkTRACE(gcvLEVEL_ERROR, "Invalid page index @ %d\n", pageIndex);
        return -1;
    }

    pageCount = memPool.pageUsage[pageIndex].pageCount;

    /* Mark all used pages as free. */
    for (i = 0; i < pageCount; ++i)
    {
        gcmkASSERT(memPool.pageUsage[i + pageIndex].pageCount == pageCount);

        memPool.pageUsage[i + pageIndex].pageCount = 0;
    }

    /* Update first free page. */
    if ( (memPool.freePage < 0) || (pageIndex < memPool.freePage) )
    {
        memPool.freePage = pageIndex;
    }

    pthread_mutex_unlock(&memPool.mutex);

    gcmkTRACE(gcvLEVEL_INFO, "Free'd %u contiguos pages from 0x%X @ 0x%x\n",
        pageCount, pageIndex);

    return 1;
}

/*----------------------------------------------------------------------------*/
/*---------------------------- Shared Memory Pool ----------------------------*/

/* Per process shared memory pool. */


/* Pointer to list of shared memory pools. */
gckSHM_POOL shmPoolList;
pthread_mutex_t shmPoolListMutex;

/*
 * Initialize shm pool list and mutex.
 */
gceSTATUS
drv_shm_init()
{
    shmPoolList = gcvNULL;
    pthread_mutex_init(&shmPoolListMutex, 0);
    return gcvSTATUS_OK;
}

gceSTATUS
drv_shm_destroy()
{
    gckSHM_POOL shmPool, nextPool;

    shmPool = shmPoolList;
    while (shmPool != gcvNULL)
    {
        nextPool = shmPool->nextPool;
        /* Remove this pool from the list. */
        drv_shmpool_destroy(shmPool);

        free(shmPool);
        shmPool = nextPool;
    }
    pthread_mutex_destroy(&shmPoolListMutex);

    return gcvSTATUS_OK;
}

/*
 * Get the shm pool associated with this PID and lock it.
 * Create one, if not present.
 */
gckSHM_POOL
drv_shm_acquire_pool(
        IN gctUINT32 Pid
        )
{
    gckSHM_POOL shmPool, tail = gcvNULL;

    pthread_mutex_lock(&shmPoolListMutex);

    shmPool = shmPoolList;
    while (shmPool != gcvNULL)
    {
        if (shmPool->pid == Pid)
        {
            pthread_mutex_unlock(&shmPoolListMutex);
            return shmPool;
        }

        tail = shmPool;
        shmPool = shmPool->nextPool;
    }

    /*printf("[galcore] Creating shmpool for Pid:%u\n", Pid);*/

    shmPool = drv_shmpool_create(Pid, sharedPoolSize, sharedPoolPageSize);

    /* Add this pool to tail. */
    if ( shmPool != gcvNULL )
    {
        if (tail != gcvNULL )
        {
            tail->nextPool = shmPool;
        }
        else
        {
            /* Set this pool as head. */
            shmPoolList = shmPool;
        }

        shmPool->nextPool = gcvNULL;
    }
    else
    {
        /* Failed to create new shmPool. */
        printf("%s[%d] Failed to create new shmPool.\n", __FUNCTION__, __LINE__);
    }

    pthread_mutex_unlock(&shmPoolListMutex);
    return shmPool;
}

/*
 * Get the shm pool associated with this Logical pointer.
 */
gckSHM_POOL
drv_shm_acquire_pool2(
        IN gctPOINTER Logical)
{
    gckSHM_POOL shmPool, shmPoolPid;

    pthread_mutex_lock(&shmPoolListMutex);

    shmPool = shmPoolList;
    while (shmPool != gcvNULL)
    {
        shmPoolPid = shmPool;

        while (shmPoolPid != gcvNULL)
        {
            /* Check if this address is in range of this shmPool. */
            if ((shmPoolPid->Logical <= (gctUINT32)Logical) &&
                (shmPoolPid->Logical + shmPoolPid->poolSize) > (gctUINT32)Logical)
            {
                pthread_mutex_unlock(&shmPoolListMutex);
                return shmPoolPid;
            }
            shmPoolPid = shmPoolPid->nextPoolPid;
        }

        shmPool = shmPool->nextPool;
    }

    /* Failed to find associated shmPool. */
    /*printf("%s[%d] Failed to find associated shmPool.\n", __FUNCTION__, __LINE__);*/
    pthread_mutex_unlock(&shmPoolListMutex);
    return shmPool;
}

/*
 * Remove the shm pools associated with this Pid.
 */
gceSTATUS
drv_shm_remove_pool(
        IN gctUINT32 Pid
        )
{
    gckSHM_POOL shmPool, prev = gcvNULL, next;

    pthread_mutex_lock(&shmPoolListMutex);

    shmPool = shmPoolList;

    while (shmPool != gcvNULL)
    {
        /* Remove this pool from the list. */
        if (shmPool->pid == Pid)
        {
            if (prev == gcvNULL)
            {
                shmPoolList = shmPool->nextPool;
            }
            else
            {
                prev->nextPool = shmPool->nextPool;
            }
            next = shmPool->nextPool;

            drv_shmpool_destroy(shmPool);

            free(shmPool);

            shmPool = next;
        }
        else
        {
        prev = shmPool;
        shmPool = shmPool->nextPool;
        }
    }

    pthread_mutex_unlock(&shmPoolListMutex);

    return gcvSTATUS_OK;
}

gceSTATUS
drv_shmpool_mem_offset(
    IN gctUINT32 Pid,
    IN gctPOINTER Logical,
    OUT gctUINT32 * Address)
{
    gctUINT32 logical = (gctUINT32)Logical;
    gckSHM_POOL shmPool;

    if ( Address == gcvNULL )
        return gcvSTATUS_INVALID_ARGUMENT;

    pthread_mutex_lock(&shmPoolListMutex);

    shmPool = shmPoolList;
    while (shmPool != gcvNULL)
    {
        if (shmPool->pid == Pid)
            break;

        shmPool = shmPool->nextPool;
    }

    while (shmPool != gcvNULL)
    {
        if ((logical >= shmPool->Logical) &&
            (logical < (shmPool->Logical + shmPool->poolSize)))
        {
            *Address = (logical - shmPool->Logical ) + shmPool->Physical;
            pthread_mutex_unlock(&shmPoolListMutex);
            return gcvSTATUS_OK;
        }
        shmPool = shmPool->nextPoolPid;
    }

    pthread_mutex_unlock(&shmPoolListMutex);

    return gcvSTATUS_INVALID_ARGUMENT;
}

gceSTATUS
drv_shmpool_mem_map(
    IN gctUINT32 Pid,
    IN gctUINT32 Address,
    OUT gctPOINTER* Logical)
{
    gckSHM_POOL shmPool;

    printf("Pid:%d Address:%x\n", Pid, Address);

    if ( Logical != gcvNULL )
    {
        pthread_mutex_lock(&shmPoolListMutex);

        shmPool = shmPoolList;
        while (shmPool != gcvNULL)
        {
            if (shmPool->pid == Pid)
                break;

            shmPool = shmPool->nextPool;
        }
        while (shmPool != gcvNULL)
        {
            /*printf("shmPool [%x - %x]\n", shmPool->Physical, (shmPool->Physical + shmPool->poolSize));*/
            if ((Address >= shmPool->Physical) &&
                (Address < (shmPool->Physical + shmPool->poolSize)))
            {
                *Logical = (gctPOINTER)((Address - shmPool->Physical ) + shmPool->Logical);

                gcmkASSERT(*Logical != gcvNULL);

                pthread_mutex_unlock(&shmPoolListMutex);
                return gcvSTATUS_OK;
            }

            shmPool = shmPool->nextPoolPid;
        }

        pthread_mutex_unlock(&shmPoolListMutex);
    }

    return gcvSTATUS_INVALID_ARGUMENT;
}

/* Initialize a shm pool for this Pid. */
gckSHM_POOL drv_shmpool_create(
        IN gctUINT32 Pid,
        IN gctUINT32 PoolSize,
        IN gctUINT32 PageSize)
{
    int rc;
    void* addr;
    off64_t paddr;
    char shm_file_name[20] = "shm_galcore";
    gctUINT32 i, pid, fd;
    gckSHM_POOL shm = (gckSHM_POOL) calloc(1, sizeof(struct _gckSHM_POOL));

    /* Compute number of pages. */
    shm->pageSize = PageSize;
    shm->pageCount = PoolSize / shm->pageSize;
    shm->poolSize = shm->pageCount * shm->pageSize;
    shm->pid = Pid;
    shm->nextPool = gcvNULL;
    shm->nextPoolPid = gcvNULL;
    gcmkASSERT(shm->pageCount <= 65536);

    /* The first page is free. */
    shm->freePage = 0;
    shm->freePageCount = shm->pageCount;

    /* Initialize the semaphore. */
    if (pthread_mutex_init(&shm->mutex, NULL) != EOK)
    {
        fprintf( stderr, "pthread_mutex_init failed: %s\n", strerror( errno ) );
        free(shm);
        return gcvNULL;
    }

    /* Create a pseudo unique name, so as to not open
     * the same file twice from different threads at the same time. */
    pid = Pid;
    i = strlen(shm_file_name);
    while(pid)
    {
        shm_file_name[i++] = (char)(pid % 10 + '0');
        pid /= 10;
    }
    shm_file_name[i] = '\0';

    fd = shm_open(shm_file_name, O_RDWR | O_CREAT, 0777);
    if (fd == -1) {
        printf("drv_shm_create: couldn't create shmem: %s\n", strerror( errno ) );
        free(shm);
        return gcvNULL;
    }

    shm_unlink(shm_file_name);

    /* Special flags for this shm, to make it write buffered. */
    rc = shm_ctl_special(fd,
                         SHMCTL_ANON | SHMCTL_PHYS /*| SHMCTL_LAZYWRITE*/,
                         0,
                         shm->poolSize,
                         0x5);
    if (rc == -1) {
        printf("drv_shm_create: shm_ctl_special failed: %s\n", strerror( errno ) );
        close(fd);
        free(shm);
        return gcvNULL;
    }

    /* Map this memory inside user and galcore. */
    addr = mmap64_join(Pid,
                       0,
                       shm->poolSize,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       fd,
                       0);
    if (addr == MAP_FAILED)
    {
         printf("drv_shmpool_create: couldn't map memory of size %d, Pid: %d [errno %s]",
                 shm->poolSize, Pid, strerror( errno ) );
        free(shm);
        return gcvNULL;
    }

    /* TODO: Dont close fd if need to truncate shm later. */
    rc = close(fd);
    if (rc == -1) {
        printf("drv_shm_create: close failed: %s\n", strerror( errno ) );
        free(shm);
        return gcvNULL;
    }

    shm->Logical = (gctUINT32) addr;

    /* fd should be NOFD here, to get physical address. */
    rc = mem_offset64( addr, NOFD, 1, (off64_t *)&paddr, NULL);
    if (rc == -1) {
        printf("drv_shm_create: mem_offset failed (addr:%x): %s\n", (gctUINT32)addr, strerror( errno ) );
        free(shm);
        return gcvNULL;
    }

    shm->Physical = (gctUINT32)paddr;

    /* TODO: MLOCK may or may not be needed!. */
    mlock((void*)shm->Logical, shm->poolSize);

    /* Allocate the page usage array and Initialize all pages to free. */
    shm->pageUsage = (gckPAGE_USAGE)calloc(shm->pageCount, sizeof(struct _gckPAGE_USAGE));
    if (shm->pageUsage == gcvNULL)
    {
        fprintf( stderr, "drv_shm_create malloc failed: %s\n", strerror(errno) );
        munmap((void*)shm->Logical, shm->poolSize);
        munmap_peer(Pid, (void*)shm->Logical, shm->poolSize);
        free(shm);
        return gcvNULL;
    }

    printf("Shm  addr range is [%x-%x]\n", shm->Logical, shm->Logical + shm->poolSize);
    printf("Shm paddr range is [%x-%x]\n", shm->Physical, shm->Physical + shm->poolSize);

    return shm;
}

void
drv_shmpool_destroy(
        IN gckSHM_POOL ShmPool)
{
    if (ShmPool)
    {
        if (ShmPool->nextPoolPid != gcvNULL)
        {
            drv_shmpool_destroy(ShmPool->nextPoolPid);
        }

        pthread_mutex_destroy(&ShmPool->mutex);

        /*printf("Destroying shmPool %p for pid %d. PageCount:%d freePageCount:%d\n", ShmPool, ShmPool->pid, ShmPool->pageCount, ShmPool->freePageCount);*/

        if (ShmPool->pageUsage)
        {
            free(ShmPool->pageUsage);
        }
        if (ShmPool->Logical)
        {
            munmap((void*)ShmPool->Logical, ShmPool->poolSize);
            munmap_peer(ShmPool->pid, (void*)ShmPool->Logical, ShmPool->poolSize);
        }
    }
    else
    {
        gcmkASSERT(0);
    }
}


/* Allocate pages from mapped shared memory.
   Return Logical user address.
*/
gctPOINTER
drv_shmpool_alloc_contiguous(
        IN gctUINT32 Pid,
        IN gctUINT32 Bytes
        )
{
    gctSIZE_T i, j;
    gctSIZE_T pageCount;
    gckSHM_POOL shmPool = drv_shm_acquire_pool(Pid);

    if (shmPool == gcvNULL || Bytes == 0)
    {
        return gcvNULL;
    }

    /* Compute the number of required pages. */
    pageCount = gcmALIGN(Bytes, shmPool->pageSize) / shmPool->pageSize;
    gcmkASSERT(pageCount != 0);

    do
    {
        if (pageCount <= shmPool->freePageCount)
        {

            /* Try finding enough contiguous free pages. */
            for (i = shmPool->freePage; i < shmPool->pageCount;)
            {
                /* All pages after this free page should be free. */
                for (j = 1; j < pageCount; ++j)
                {
                    if (shmPool->pageUsage[i + j].pageCount != 0)
                    {
                        /* Bail out if page is allocated. */
                        break;
                    }
                }

                if (j == pageCount)
                {
                    /* We found a spot that has enough free pages. */
                    break;
                }

                /* Move to the page after the allocated page. */
                i += j + 1;

                /* Find the next free page. */
                while ((i < shmPool->pageCount) && (shmPool->pageUsage[i].pageCount != 0))
                {
                    /*i += shmPool->pageUsage[i].pageCount;*/
                    ++i;
                }
            }

            if (i < shmPool->pageCount)
            {
                /* Found free pages. */
                break;
            }
        }

        if (shmPool->nextPoolPid == gcvNULL)
        {
            gctUINT32 poolSize;

            poolSize = pageCount * shmPool->pageSize;
            /* Align it wrt sharedPoolSize. */
            poolSize = gcmALIGN(poolSize, sharedPoolSize);

            /* Allocate a new shmPool. */
            shmPool->nextPoolPid = drv_shmpool_create(Pid, poolSize, shmPool->pageSize);

            if (shmPool->nextPoolPid == gcvNULL)
            {
                printf("%s OOM!!!!\n", __FUNCTION__);
                /* Out of memory. */
                return gcvNULL;
            }
        }

        shmPool = shmPool->nextPoolPid;

    } while(shmPool != gcvNULL);

    /* Check if we allocate from the first free page. */
    if (i == shmPool->freePage)
    {
        /* Move first free page to beyond the contiguous request. */
        shmPool->freePage = i + pageCount;

        /* Find first free page. */
        while ( (shmPool->freePage < shmPool->pageCount) &&
                (shmPool->pageUsage[shmPool->freePage].pageCount != 0) )
        {
            /*shmPool->freePage += shmPool->pageUsage[shmPool->freePage].pageCount;*/
            shmPool->freePage++;
        }

        if (shmPool->freePage >= shmPool->pageCount)
        {
            /* No more free pages. */
            shmPool->freePage = -1;
        }
    }
    /*printf("Allocating from shmpool: %p. freePage %2d, Bytes:%d\n", shmPool, shmPool->freePage, Bytes);*/

    /* Walk all pages. */
    shmPool->pageUsage[i].pageCount = pageCount;
    for (j = 0; j < pageCount; ++j)
    {
        /* Store page count in each pageUsage to mark page is allocated. */
        shmPool->pageUsage[i+j].pageCount = pageCount;
    }

    shmPool->freePageCount -= pageCount;

    gcmkTRACE(gcvLEVEL_INFO, "Allocated %u contiguous pages from 0x%X\n",
        pageCount, i);


    /* Success. */
    return (gctPOINTER)(i * shmPool->pageSize + shmPool->Logical);
}

gctUINT32
drv_shmpool_free(
        IN gctPOINTER Logical
        )
{
    gctUINT16 pageCount;
    gctSIZE_T i;
    gctINT32 pageIndex;
    gckSHM_POOL shmPool;

    shmPool = drv_shm_acquire_pool2(Logical);
/*
    printf("ShmPool %p Stats:\n", shmPool);
    printf("pageCount %d:\n", shmPool->pageCount);
    printf("pageSize %x:\n", shmPool->pageSize);
*/

    if (shmPool == gcvNULL)
    {
        gcmkTRACE(gcvLEVEL_ERROR, "Invalid Logical addr: %x.\n", Logical);
        return 0;
    }

    pageIndex = ((gctUINT32)Logical - shmPool->Logical)/shmPool->pageSize;

    gcmkTRACE(gcvLEVEL_INFO, "Freeing pages @ %d\n", pageIndex);

    /* Verify the memory is valid and unlocked. */
    if ( (pageIndex < 0) || (pageIndex >= shmPool->pageCount) )
    {
        gcmkTRACE(gcvLEVEL_ERROR, "Invalid page index @ %d\n", pageIndex);

        return 0;
    }

    pageCount = shmPool->pageUsage[pageIndex].pageCount;
    gcmkASSERT(shmPool->pageUsage[pageIndex].pageCount == pageCount);

    /* Mark all used pages as free. */
    for (i = 0; i < pageCount; ++i)
    {

        shmPool->pageUsage[i + pageIndex].pageCount = 0;
    }

    /* Update first free page. */
    if ( (shmPool->freePage < 0) || (pageIndex < shmPool->freePage) )
    {
        shmPool->freePage = pageIndex;
    }
    /*printf("Freeing    from shmpool: %p. freePage %2d, Bytes:%d\n", shmPool, shmPool->freePage, pageCount*shmPool->pageSize);*/

    shmPool->freePageCount += pageCount;

    gcmkTRACE(gcvLEVEL_INFO, "Free'd %u contiguos pages from 0x%X @ 0x%x\n",
        pageCount, pageIndex);

    return 1;
}

/*----------------------------------------------------------------------------*/
/*----------------------------- Resource Manager -----------------------------*/

/* Resource Manager Globals. */
struct _gcskRESMGR_GLOBALS
{
    dispatch_t *dpp;
    dispatch_context_t *ctp;
    int id;
    thread_pool_attr_t pool_attr;
    thread_pool_t *tpp;
    pthread_t root;
} resmgr_globals;

win_gpu_2_cm_iface_t *g_qnx_gpu_2_cm_iface = 0;
static resmgr_connect_funcs_t connect_funcs;
static resmgr_io_funcs_t io_funcs;
static iofunc_attr_t attr;

pthread_key_t thread_specific_key;                      /* To store calling process attributes. */

void thread_specific_key_destruct( void *value )
{
    /*printf("Destroying thread: myTid:%u, value:%d\n", pthread_self(), (gctUINT32)value);*/
    if (value != gcvNULL)
    {
        free( value );
    }
    pthread_setspecific( thread_specific_key, NULL );
}

int drv_thread_specific_key_assign(
        gctUINT32 Pid,
        gctUINT32 Tid
    )
{
    gcskTHREAD_USER_DATA *data;

    data = (gcskTHREAD_USER_DATA *)pthread_getspecific( thread_specific_key );

    if( data == NULL ) {
        data = (gcskTHREAD_USER_DATA *) malloc( sizeof(gcskTHREAD_USER_DATA) );

        /*printf("First time. Pid:%u, Tid:%u, myTid:%u\n", Pid, Tid, pthread_self());*/

        if( data == NULL )
        {
            return 0;
        }
    }

    /*printf("Pid:%u, Tid:%u, myTid:%u\n", Pid, Tid, pthread_self());*/
    data->Pid = Pid;
    data->Tid = Tid;

    if ( pthread_setspecific( thread_specific_key, (void *)data ) != EOK)
    {
        return 0;
    }

    return 1;
}

gctUINT32 drv_get_user_pid( void )
{
    gcskTHREAD_USER_DATA *data;

    data = (gcskTHREAD_USER_DATA *)pthread_getspecific( thread_specific_key );

    if (data == gcvNULL)
    {
        /*printf("%s[%d] pid = 0\n", __FUNCTION__, __LINE__);*/
        /* Galcore process. */
        return 0;
    }
    /*printf("%s[%d] pid = %u\n", __FUNCTION__, __LINE__, data->Pid);*/
    return data->Pid;
}

gctUINT32 drv_get_user_tid( void )
{
    gcskTHREAD_USER_DATA *data;

    data = (gcskTHREAD_USER_DATA *)pthread_getspecific( thread_specific_key );

    if (data == gcvNULL)
    {
        return (gctUINT32)-1;
    }

    /*printf("%s[%d] Tid = %u\n", __FUNCTION__, __LINE__, data->Tid);*/
    return data->Tid;
}

int drv_msg(resmgr_context_t *ctp,
            io_msg_t *msg,
            RESMGR_OCB_T *ocb)
{
    gcsDRIVER_ARGS *drvArgs = (gcsDRIVER_ARGS *)msg;
    int rc;
    gceSTATUS status;
    gcsQUEUE_PTR queue;
    gctINT32 i, count;

/*#define UNLOCK_RESMGR*/
#ifdef UNLOCK_RESMGR
    iofunc_attr_unlock(&attr);
#endif

    if ((drvArgs->iomsg.i.type != _IO_MSG)
    || (drvArgs->iomsg.i.mgrid != _IOMGR_VIVANTE)
    || (drvArgs->iomsg.i.subtype != IOCTL_GCHAL_INTERFACE
        && drvArgs->iomsg.i.subtype != IOCTL_GCHAL_KERNEL_INTERFACE
        && drvArgs->iomsg.i.subtype != IOCTL_GCHAL_TERMINATE))
    {
        /* Unknown command. Fail the I/O. */
#ifdef UNLOCK_RESMGR
        iofunc_attr_lock(&attr);
#endif
        rc = ENOSYS;
        if (ctp->info.scoid != -1)
            return _RESMGR_STATUS(ctp, rc);
        return _RESMGR_NOREPLY;
    }

    if (drvArgs->iomsg.i.subtype == IOCTL_GCHAL_TERMINATE)
    {
        /* terminate the resource manager */
        pthread_kill(resmgr_globals.root, SIGTERM);
#ifdef UNLOCK_RESMGR
        iofunc_attr_lock(&attr);
#endif
        return _RESMGR_NOREPLY;
    }

    /* Save channel handle and pid for later functions. */
    /*drvArgs->iface.pid = (gctUINT32)ctp->info.pid;*/

    if ( drv_thread_specific_key_assign((gctUINT32)ctp->info.pid,
                                    (gctUINT32)ctp->info.tid)
            == 0)
    {
        drvArgs->iface.status = gcvSTATUS_GENERIC_IO;
        goto OnError;
    }


    /* Store receive ID with signal event so that we can later respond via pulse. */
    switch (drvArgs->iface.command)
    {
    case gcvHAL_SIGNAL:
        /*printf("Setup rcvid as:%d\n", ctp->rcvid);*/
        drvArgs->iface.u.Signal.rcvid = ctp->rcvid;
        break;

    case gcvHAL_COMPOSE:
        drvArgs->iface.u.Compose.rcvid = ctp->rcvid;
        break;

    case gcvHAL_EVENT_COMMIT:
        for (queue = drvArgs->iface.u.Event.queue; queue != gcvNULL; queue = queue->next)
        {
            if (queue->iface.command == gcvHAL_SIGNAL)
            {
                queue->iface.u.Signal.rcvid = ctp->rcvid;
            }
            else if (queue->iface.command == gcvHAL_COMPOSE)
            {
                queue->iface.u.Compose.rcvid = ctp->rcvid;
            }
        }
        break;

    case gcvHAL_COMMIT:
#if gcdENABLE_VG
        if (galDevice->coreMapping[drvArgs->iface.hardwareType] == gcvCORE_VG)
        {
            drvArgs->iface.u.VGCommit.context->rcvid = ctp->rcvid;
        }
        else
#endif
        {
            for (queue = drvArgs->iface.u.Commit.queue; queue != gcvNULL; queue = queue->next)
            {
                if (queue->iface.command == gcvHAL_SIGNAL)
                {
                    queue->iface.u.Signal.rcvid = ctp->rcvid;
                }
                else if (queue->iface.command == gcvHAL_COMPOSE)
                {
                    queue->iface.u.Compose.rcvid = ctp->rcvid;
                }
            }
        }
        break;

    default:
        break;
    }

    if (drvArgs->iface.command == gcvHAL_CHIP_INFO)
    {
        count = 0;
        for (i = 0; i < gcdCORE_COUNT; i++)
        {
            if (galDevice->kernels[i] != gcvNULL)
            {
#if gcdENABLE_VG
                if (i == gcvCORE_VG)
                {
                    drvArgs->iface.u.ChipInfo.types[count] = gcvHARDWARE_VG;
                }
                else
#endif
                {
                    gcmkVERIFY_OK(gckHARDWARE_GetType(galDevice->kernels[i]->hardware,
                                                      &drvArgs->iface.u.ChipInfo.types[count]));
                }
                count++;
            }
        }

        drvArgs->iface.u.ChipInfo.count = count;
        status = gcvSTATUS_OK;
    }
    else
    {
        if (drvArgs->iface.hardwareType < 0 || drvArgs->iface.hardwareType > 7)
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_ERROR, gcvZONE_DRIVER,
                "%s(%d): unknown hardwareType %d\n",
                __FUNCTION__, __LINE__,
                drvArgs->iface.hardwareType
                );

            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

#if gcdENABLE_VG
        if (galDevice->coreMapping[drvArgs->iface.hardwareType] == gcvCORE_VG)
        {
            status = gckVGKERNEL_Dispatch(galDevice->kernels[gcvCORE_VG],
                                        (drvArgs->iomsg.i.subtype == IOCTL_GCHAL_INTERFACE),
                                        &drvArgs->iface);
        }
        else
#endif
        {
            status = gckKERNEL_Dispatch(galDevice->kernels[galDevice->coreMapping[drvArgs->iface.hardwareType]],
                                        (drvArgs->iomsg.i.subtype == IOCTL_GCHAL_INTERFACE),
                                        &drvArgs->iface);
        }
    }

#if gcdDEBUG
    if (gcmIS_ERROR(status))
    {
        gcmkTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_DRIVER,
                  "[galcore] gckKERNEL_Dispatch returned %d.\n",
              status);
    }
    else if (gcmIS_ERROR(drvArgs->iface.status))
    {
        gcmkTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_DRIVER,
                  "[galcore] IOCTL %d returned %d.\n",
              drvArgs->iface.command,
              drvArgs->iface.status);
    }
#endif

OnError:
    /* Reply data back to the user. */
    MsgReply(ctp->rcvid, EOK, (gctPOINTER) &drvArgs->iface, sizeof(gcsHAL_INTERFACE));

#ifdef UNLOCK_RESMGR
    iofunc_attr_lock(&attr);
#endif

    gcmkTRACE(gcvLEVEL_INFO, "[galcore] Replied message with command %d, status %d\n",
        drvArgs->iface.command,
        drvArgs->iface.status);

    return (_RESMGR_NOREPLY);
}

/*----------------------------------------------------------------------------*/
/*-------------------------- DRIVER INITIALIZATIONS --------------------------*/

static int drv_init(void)
{
    gceSTATUS status;

    /* TODO: Enable clock by driver support? */
    gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
                  "[galcore] Entering drv_init\n");

    /* Create the GAL device. */
    if (gcmIS_ERROR(gckGALDEVICE_Construct(irqLine, registerMemBase, registerMemSize,
                    irqLine2D, registerMemBase2D, registerMemSize2D,
                    irqLineVG, registerMemBaseVG, registerMemSizeVG,
                    contiguousBase,
                    contiguousSize,
                    bankSize,
                    fastClear,
                    compression,
                    baseAddress,
                    &galDevice)))
    {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
                      "[galcore] Can't create the gal device.\n");

        return -1;
    }

    gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
                  "[galcore] Device constructed.\n");

    /* Start the GAL device. */
    if (gcmIS_ERROR(gckGALDEVICE_Start(galDevice)))
    {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
                      "[galcore] Can't start the gal device.\n");

        /* Roll back. */
        gckGALDEVICE_Stop(galDevice);
        gckGALDEVICE_Destroy(galDevice);

        return -1;
    }

    if ((physSize != 0)
       && (galDevice->kernels[gcvCORE_MAJOR] != gcvNULL)
       && (galDevice->kernels[gcvCORE_MAJOR]->hardware->mmuVersion != 0))
    {
        status = gckMMU_Enable(galDevice->kernels[gcvCORE_MAJOR]->mmu, baseAddress, physSize);
        gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
            "Enable new MMU: status=%d\n", status);

        if ((galDevice->kernels[gcvCORE_2D] != gcvNULL)
            && (galDevice->kernels[gcvCORE_2D]->hardware->mmuVersion != 0))
        {
            status = gckMMU_Enable(galDevice->kernels[gcvCORE_2D]->mmu, baseAddress, physSize);
            gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                "Enable new MMU for 2D: status=%d\n", status);
        }

        /* Reset the base address */
        baseAddress = 0;
        galDevice->baseAddress = 0;
    }

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                  "[galcore] irqLine->%ld, contiguousSize->%lu, memBase->0x%lX\n",
          irqLine,
          contiguousSize,
          registerMemBase);

    gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
                  "[galcore] driver registered successfully.\n");

    return 0;
}

static void drv_exit(void)
{
    gcmkTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
                  "[galcore] Entering drv_exit\n");

    gckGALDEVICE_Stop(galDevice);
    gckGALDEVICE_Destroy(galDevice);
}

/* Invoked by OS, when a new connection is created. */
int drv_open_connection(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle, void *extra)
{
    struct pid_hash* node;
    gctINT i;

    if (msg->connect.subtype == _IO_CONNECT_OPEN)
    {
        /* Assign pid and tid, before calling gckKERNEL_AttachProcess. */
        if (drv_thread_specific_key_assign(ctp->info.pid, ctp->info.tid))
        {
            /* A process gets attached. */
            for (i = 0; i < gcdCORE_COUNT; i++)
            {
                if (galDevice->kernels[i] != gcvNULL)
                {
                    if (gcmIS_ERROR(gckKERNEL_AttachProcess(galDevice->kernels[i], gcvTRUE)))
                    {
                        return EINVAL;
                    }
                }
            }
        }
    }

    /* Update reference count for this pid. */
    node = pid_hash_head;
    while(node != gcvNULL)
    {
        if(node->pid == ctp->info.pid)
        {
            break;
        }
        node = node->next;
    }

    if (node == gcvNULL)
    {
        /* New connection. */
        node = (struct pid_hash*) malloc(sizeof(struct pid_hash));
        if (node == gcvNULL) return ENOMEM;

        node->pid = ctp->info.pid;
        node->refCount = 0;


        /* Append node at head. */
        node->next = pid_hash_head;
        pid_hash_head = node;
    }

    node->refCount++;

    return iofunc_open_default(ctp, msg, handle, extra);
}

/* Invoked by OS, when a connection is closed or dies. */
int
drv_close_connection(resmgr_context_t *ctp, void *reserved, RESMGR_OCB_T *ocb)
{
    gckVIDMEM videoMemory;
    gckKERNEL kernel;
    gceSTATUS status;
    struct pid_hash *node, *prev_node;

    /* Update reference count for this pid. */
    prev_node = pid_hash_head;

    if ( pid_hash_head != gcvNULL )
    {
        if ( prev_node->pid == ctp->info.pid )
        {
            /* Hit on first node. */
            node = prev_node;
        }
        else
        {
            node = prev_node->next;
            while(node != gcvNULL)
            {
                if(node->pid == ctp->info.pid)
                {
                    break;
                }
                prev_node = node;
                node = node->next;
            }
        }

        if ( node != gcvNULL )
        {
            node->refCount--;

            /* Last close. Close connection. */
            if ( node->refCount == 0)
            {
                if (node == prev_node)
                {
                    /* Head node. */
                    pid_hash_head = node->next;
                }
                else
                {
                    /* Delete from middle. */
                    prev_node->next = node->next;
                }

                free(node);

                /* Choose a kernel */
                kernel = galDevice->kernels[gcvCORE_MAJOR];

                if (kernel == gcvNULL)
                {
                    kernel = galDevice->kernels[gcvCORE_2D];
                }

                if (kernel == gcvNULL)
                {
                    kernel = galDevice->kernels[gcvCORE_VG];
                }

                gcmkASSERT(kernel != gcvNULL);

                /* Free system memory owned by the client. */
#if gcdUSE_VIDMEM_PER_PID
                status = gckKERNEL_GetVideoMemoryPoolPid(
                            kernel,
                            gcvPOOL_SYSTEM, ctp->info.pid, &videoMemory);

                printf("Closing connection for pid:%d\n", ctp->info.pid);

                /* Remove this video memory pool as well. */
                if (status == gcvSTATUS_OK && videoMemory != gcvNULL)
                {
                    /* Free virtual memory owned by this client. */
                    gcmkVERIFY_OK(gckMMU_FreeHandleMemory(
                                kernel->mmu,
                                kernel->mmu,
                                ctp->info.pid));

                    /* Free video memory owned by this client. */
                    gcmkVERIFY_OK(gckKERNEL_RemoveVideoMemoryPoolPid(
                                kernel,
                                videoMemory));

                    gcmkVERIFY_OK(gckVIDMEM_Destroy(videoMemory));
                }

#else
                status = gckKERNEL_GetVideoMemoryPool(
                            kernel, gcvPOOL_SYSTEM, &videoMemory);
                if (status == gcvSTATUS_OK)
                {
                    gcmkVERIFY_OK(gckVIDMEM_FreeHandleMemory(
                                kernel, videoMemory, ctp->info.pid));
                }
#endif

                /* Free shared memory and its mapping. */
                drv_shm_remove_pool(ctp->info.pid);
            }
        }
    }

    return iofunc_close_ocb_default(ctp, reserved, ocb);
}

int gpu_init()
{
    /* Declare variables we'll be using. */
    resmgr_attr_t resmgr_attr;
    sigset_t  sigset;
    int rc;

    if (drv_mempool_init() != gcvSTATUS_OK)
    {
        fprintf(stderr, "drv_mempool_init failed.");
        goto fail_001;
    }

    if (drv_shm_init() != gcvSTATUS_OK)
    {
        fprintf(stderr, "drv_shm_init failed.");
        goto fail_002;
    }

    if (drv_init() != 0)
    {
        fprintf(stderr, "drv_init failed.");
        goto fail_003;
    }

    /* Initialize thread local storage. */
    if((pthread_key_create( &thread_specific_key, &thread_specific_key_destruct)) != EOK)
    {
        fprintf(stderr, "Unable to create thread_specific_key.\n");
        goto fail_004;
    }

    /* initialize dispatch interface */
    if((resmgr_globals.dpp = dispatch_create()) == NULL)
    {
        fprintf(stderr, "Unable to allocate dispatch handle.\n");
        goto fail_004;
    }

    /* Initialize per pid reference count list. */
    pid_hash_head = gcvNULL;

    /* initialize resource manager attributes */
    memset(&resmgr_attr, 0, sizeof resmgr_attr);
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;

    /* initialize functions for handling messages */
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
            _RESMGR_IO_NFUNCS, &io_funcs);

    /* Register io handling functions. */
    io_funcs.msg       = drv_msg;
    io_funcs.close_ocb = drv_close_connection;
    connect_funcs.open = drv_open_connection;

    /* initialize attribute structure used by the device */
    iofunc_attr_init(&attr, S_IFNAM | 0666, 0, 0);

    /* attach our device name */
    resmgr_globals.id = resmgr_attach(
            resmgr_globals.dpp,/* dispatch handle */
            &resmgr_attr,           /* resource manager attrs */
            GAL_DEV,                /* device name */
            _FTYPE_ANY,             /* open type */
            _RESMGR_FLAG_SELF,      /* flags */
            &connect_funcs,         /* connect routines */
            &io_funcs,              /* I/O routines */
            &attr);                 /* handle */
    if (resmgr_globals.id == -1) {
        fprintf(stderr, "Unable to attach name.\n");
        goto fail_005;
    }

    printf("Attached resmgr to " GAL_DEV " with id:%d.\n", resmgr_globals.id);

    /* Prevent signals from affecting resmgr threads. */
    sigfillset(&sigset);
    sigdelset(&sigset, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    /* initialize thread pool attributes */
    memset(&resmgr_globals.pool_attr, 0, sizeof(resmgr_globals.pool_attr));
    resmgr_globals.pool_attr.handle = resmgr_globals.dpp;
    resmgr_globals.pool_attr.context_alloc = resmgr_context_alloc;
    resmgr_globals.pool_attr.block_func   = resmgr_block;
    resmgr_globals.pool_attr.unblock_func = resmgr_unblock;
    resmgr_globals.pool_attr.handler_func = resmgr_handler;
    resmgr_globals.pool_attr.context_free = resmgr_context_free;
    resmgr_globals.pool_attr.lo_water = 2;
    resmgr_globals.pool_attr.hi_water = 5;
    resmgr_globals.pool_attr.increment = 1;
    resmgr_globals.pool_attr.maximum = 5;
#if (defined(_NTO_VERSION) && (_NTO_VERSION >= 650))
    resmgr_globals.pool_attr.tid_name = "galcore-msg";
#endif

    /* allocate a thread pool handle */
    resmgr_globals.tpp = thread_pool_create(&resmgr_globals.pool_attr, POOL_FLAG_EXIT_SELF);
    if (resmgr_globals.tpp == NULL)
    {
        goto fail_006;
    }

    rc = pthread_create(NULL, resmgr_globals.pool_attr.attr, (void * (*)(void *))thread_pool_start, resmgr_globals.tpp);
    if (rc != 0)
    {
        goto fail_007;
    }

    /* TODO: gpu_suspend, gpu_resume */

    return EXIT_SUCCESS;

fail_007:
    thread_pool_destroy(resmgr_globals.tpp);
fail_006:
    resmgr_detach(resmgr_globals.dpp, resmgr_globals.id, 0);
fail_005:
    dispatch_destroy(resmgr_globals.dpp);
fail_004:
    drv_exit();
fail_003:
    drv_shm_destroy();
fail_002:
    drv_mempool_destroy();
fail_001:
    return EXIT_FAILURE;
}

int gpu_fini()
{
    printf("Exiting galcore.\n");
    thread_pool_destroy(resmgr_globals.tpp);
    resmgr_detach(resmgr_globals.dpp, resmgr_globals.id, 0);
    dispatch_destroy(resmgr_globals.dpp);
    drv_exit();
    drv_shm_destroy();
    drv_mempool_destroy();
    return EXIT_SUCCESS;
}

#ifndef QNX_USE_OLD_FRAMEWORK

int GPU_Startup(win_gpu_2_cm_iface_t *iface)
{
    g_qnx_gpu_2_cm_iface = iface;
    return gpu_init();
}

int GPU_Shutdown()
{
    g_qnx_gpu_2_cm_iface = NULL;
    return gpu_fini();
}

void win_gpu_module_getfuncs(win_cm_2_gpu_iface_t *iface)
{
    iface->init = GPU_Startup;
    iface->fini = GPU_Shutdown;
}

#else /* QNX_USE_OLD_FRAMEWORK */

int drv_resmgr_loop()
{
    sigset_t  sigset;
    siginfo_t info;

    resmgr_globals.root = pthread_self();

    /* Background ourselves */
    procmgr_daemon(EXIT_SUCCESS, PROCMGR_DAEMON_NODEVNULL |
                                 PROCMGR_DAEMON_NOCHDIR |
                                 PROCMGR_DAEMON_NOCLOSE);

    /*
     * This thread ignores all signals except SIGTERM. On receipt of
     * a SIGTERM, we shut everything down and exit.
     */
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);

    while (1)
    {
        if (SignalWaitinfo(&sigset, &info) == -1)
            continue;
        if (info.si_signo == SIGTERM)
        {
            printf("drv_msg_loop: received term signal.\n");
            break;
        }
    }

    return EXIT_SUCCESS;
}

int drv_start_cmd()
{
    int rc;

    printf("Starting up...\n");
    fflush(stdout);

    pthread_setname_np(pthread_self(), "vivante-monitor");

    if ((rc = gpu_init()) != EXIT_SUCCESS)
    {
        fprintf(stderr, "Initialization failed!, Exiting.");
        //vivante_global_fini();  // tear down global vars here...
        exit(EXIT_FAILURE);
    }

    printf("Running galcore...\n");
    fflush(stdout);
    rc = drv_resmgr_loop();
    printf("Shutting down galcore...\n");
    fflush(stdout);

    gpu_fini();

    return EXIT_SUCCESS;
}

int drv_stop_cmd()
{
    gcsDRIVER_ARGS args;
    int fd, rc;

    /* Open the gpu device. */
    fd = open(DRV_NAME, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Could not connect to " DRV_NAME);
        return EXIT_FAILURE;
    }

    /* Send the term message. */
    args.iomsg.i.type    = _IO_MSG;
    args.iomsg.i.subtype = IOCTL_GCHAL_TERMINATE;
    args.iomsg.i.mgrid   = _IOMGR_VIVANTE;
    args.iomsg.i.combine_len = sizeof(io_msg_t);

    do {
        rc = MsgSend_r(fd, &args, args.iomsg.i.combine_len, NULL, 0);
    } while ((rc * -1) == EINTR);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    enum { start, stop } cmd = start;
    int i;
    int rc = EXIT_FAILURE;

    /* Process command lines -start, -stop, -c (file), -d [file]. */
    for (i = 1; i < argc; i++)
    {
        if (stricmp(argv[i], "-start") == 0)
        {
            cmd = start;
        }
        else if (strcmp(argv[i], "-stop") == 0)
        {
            cmd = stop;
        }
        else if (strncmp(argv[i], "-poolsize=", strlen("-poolsize=")) == 0)
        {
            /* The syntax of the poolsize option is -poolsize=(number).
             * All we need is to convert the number that starts after the '='.*/
            contiguousSize = atoi(argv[i] + strlen("-poolsize="));
            if (contiguousSize <= 0)
            {
                fprintf(stderr, "%s: poolsize needs to be a positive number\n", strerror(errno));
                return rc;
            }
        }
        else
        {
            fprintf(stderr, "%s: bad command line\n", argv[0]);
            return rc;
        }
    }

    switch (cmd)
    {
        case start:
            /* Elevate thread priority to do IO. */
            ThreadCtl(_NTO_TCTL_IO, 0);
            rc = drv_start_cmd();
            break;

        case stop:
            rc = drv_stop_cmd();
            break;
    }

    return rc;
}

#endif /* QNX_USE_OLD_FRAMEWORK */
