/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2011 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description : System Wrapper Layer for Linux using IRQs
--
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "dwl.h"
#include "dwl_linux.h"
#include "hx170dec.h"
#include "memalloc.h"

#ifdef USE_LINUX_LOCK_IOCTL
  #include "dwl_linux_lock_ioctl.h"
#else
  #include "dwl_linux_lock.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifdef _DWL_HW_PERFORMANCE
#include <sys/time.h>
#endif

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#include <pthread.h>

#include <errno.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_EFENCE
#include "efence.h"
#endif

#ifdef _DWL_HW_PERFORMANCE
#define HW_PERFORMANCE_LOG "/tmp/hw_performance.log"
#define HW_PERFORMANCE_RESOLUTION 0.00001   /* 10^-5 */
static FILE *hw_performance_log = NULL;
static u32 dec_pic_counter = 0;
static u32 pp_pic_counter = 0;
u32 dec_time;                /* decode time in milliseconds */
struct timeval dec_start_time;
struct timeval pp_start_time;
u32 pp_time /* pp time in milliseconds */ ;
#endif

#define X170_SEM_KEY 0x8070

/* a mutex protecting the wrapper init */
pthread_mutex_t x170_init_mutex = PTHREAD_MUTEX_INITIALIZER;

/* the decoder device driver nod */
const char *dec_dev = DEC_MODULE_PATH;

/* the memalloc device driver nod */
#ifdef DWL_USE_PMEM
const char *mem_dev = PMEM_MODULE_PATH;
#else
const char *mem_dev = MEMALLOC_MODULE_PATH;
#endif

#if 0
static volatile sig_atomic_t x170_sig_dec_delivered = 0;
static volatile sig_atomic_t x170_sig_pp_delivered = 0;

void x170_sig_handler(int signal_number)
{
    DWL_DEBUG("dwl_x170_linux_irq.c: x170_sig_handler (tid = %d), (pid = %d), (pthread_self = %lu, main_thread = %lu)\n", gettid(), getpid(), pthread_self(), main_thread);

    if(pthread_self() != main_thread)
    {
        pthread_kill(main_thread, SIGIO);
        return;
    }

    x170_sig_dec_delivered++;
    x170_sig_pp_delivered++;
}
#endif
/*------------------------------------------------------------------------------
    Function name   : DWLInit
    Description     : Initialize a DWL instance

    Return type     : const void * - pointer to a DWL instance

    Argument        : void * param - not in use, application passes NULL
------------------------------------------------------------------------------*/
const void *DWLInit(DWLInitParam_t * param)
{
    hX170dwl_t *dec_dwl;
    unsigned long base;

    dec_dwl = (hX170dwl_t *) malloc(sizeof(hX170dwl_t));

    DWL_DEBUG("DWL: INITIALIZE\n");
    if(dec_dwl == NULL)
    {
        DWL_DEBUG("DWL: failed to alloc hX170dwl_t struct\n");
        return NULL;
    }

#ifdef INTERNAL_TEST
    dec_dwl->regDump = fopen(REG_DUMP_FILE, "a+");
    if(NULL == dec_dwl->regDump)
    {
        DWL_DEBUG("DWL: failed to open: %s\n", REG_DUMP_FILE);
        goto err;
    }
#endif

    dec_dwl->clientType = param->clientType;

#ifdef _DWL_HW_PERFORMANCE
    if(NULL == hw_performance_log)
    {
        hw_performance_log = fopen(HW_PERFORMANCE_LOG, "w");
        if(NULL == hw_performance_log)
        {
            DWL_DEBUG("DWL: failed to open: %s\n", HW_PERFORMANCE_LOG);
            goto err;
        }
    }
    switch (dec_dwl->clientType)
    {
    case DWL_CLIENT_TYPE_H264_DEC:
    case DWL_CLIENT_TYPE_MPEG4_DEC:
    case DWL_CLIENT_TYPE_JPEG_DEC:
    case DWL_CLIENT_TYPE_VC1_DEC:
    case DWL_CLIENT_TYPE_MPEG2_DEC:
    case DWL_CLIENT_TYPE_VP6_DEC:
    case DWL_CLIENT_TYPE_VP8_DEC:
    case DWL_CLIENT_TYPE_RV_DEC:
    case DWL_CLIENT_TYPE_AVS_DEC:
        dec_pic_counter = 0;
        dec_time = 0;
        dec_start_time.tv_sec = 0;
        dec_start_time.tv_usec = 0;
        break;
    case DWL_CLIENT_TYPE_PP:
        pp_pic_counter = 0;
        pp_time = 0;
        pp_start_time.tv_sec = 0;
        pp_start_time.tv_usec = 0;
        break;
    }
#endif

    pthread_mutex_lock(&x170_init_mutex);

    dec_dwl->fd_mem = -1;
    dec_dwl->fd_memalloc = -1;
    dec_dwl->pRegBase = MAP_FAILED;

#ifdef ANDROID_MOD
	// DSPG: for ANDROID_MOD allow also the dwl instance of the pp to allocate linear memory
	if (1) 
#else
	/* Linear memories not needed in pp */
    if(dec_dwl->clientType != DWL_CLIENT_TYPE_PP)
#endif
    {
        /* open memalloc for linear memory allocation */
        dec_dwl->fd_memalloc = open(mem_dev, O_RDWR | O_SYNC);

        if(dec_dwl->fd_memalloc == -1)
        {
            DWL_DEBUG("DWL: failed to open: %s\n", mem_dev);
            goto err;
        }

    }

    DWL_DEBUG("user = %d, group = %d\n",getuid(),getgid());

    dec_dwl->fd_mem = open("/dev/mem", O_RDWR | O_SYNC);

    if(dec_dwl->fd_mem == -1)
    {
        DWL_DEBUG("DWL: failed to open: %s\n", "/dev/mem");
        goto err;
    }

    switch (dec_dwl->clientType)
    {
    case DWL_CLIENT_TYPE_H264_DEC:
    case DWL_CLIENT_TYPE_MPEG4_DEC:
    case DWL_CLIENT_TYPE_JPEG_DEC:
    case DWL_CLIENT_TYPE_VC1_DEC:
    case DWL_CLIENT_TYPE_MPEG2_DEC:
    case DWL_CLIENT_TYPE_VP6_DEC:
    case DWL_CLIENT_TYPE_VP8_DEC:
    case DWL_CLIENT_TYPE_RV_DEC:
    case DWL_CLIENT_TYPE_AVS_DEC:
    case DWL_CLIENT_TYPE_PP:
        {
            /* open the device */
            dec_dwl->fd = open(dec_dev, O_RDWR);
            if(dec_dwl->fd == -1)
            {
                DWL_DEBUG("DWL: failed to open '%s'\n", dec_dev);
                goto err;
            }

#if 0
            /* asynchronus notification handler */
            {
                struct sigaction sa;

                memset(&sa, 0, sizeof(sa));
                sa.sa_handler = x170_sig_handler;
                sa.sa_flags |= SA_RESTART;  /* restart of system calls */
                sigaction(SIGIO, &sa, NULL);
            }

            /* Notify kernel module that this is a pp instance */
            /* NOTE: Must be called befor registerring for interrupt! */
            if(dec_dwl->clientType == DWL_CLIENT_TYPE_PP)
                ioctl(dec_dwl->fd, HX170DEC_PP_INSTANCE);

            /* asynchronus notification needed. This will be done in first DWLEnableHW,
             * so that we picup the decoding thread and not the init */
            dec_dwl->sigio_needed = 1;
#endif
            break;
        }
    default:
        {
            DWL_DEBUG("DWL: Unknown client type no. %d\n", dec_dwl->clientType);
            goto err;
        }
    }

    if(ioctl(dec_dwl->fd, HX170DEC_IOCGHWOFFSET, &base) == -1)
    {
        DWL_DEBUG("DWL: ioctl failed\n");
        goto err;
    }

    if(ioctl(dec_dwl->fd, HX170DEC_IOCGHWIOSIZE, &dec_dwl->regSize) == -1)
    {
        DWL_DEBUG("DWL: ioctl failed\n");
        goto err;
    }

    /* map the hw registers to user space */
    dec_dwl->pRegBase =
        DWLMapRegisters(dec_dwl->fd_mem, base, dec_dwl->regSize, 1);

    if(dec_dwl->pRegBase == MAP_FAILED)
    {
        DWL_DEBUG("DWL: Failed to mmap regs\n");
        goto err;
    }

    DWL_DEBUG("DWL: regs size %d bytes, virt %08x\n", dec_dwl->regSize,
              (u32) dec_dwl->pRegBase);

#ifndef USE_LINUX_LOCK_IOCTL
    {
        key_t key = X170_SEM_KEY;
        int semid;

        if((semid = binary_semaphore_allocation(key, O_RDWR)) != -1)
        {
            DWL_DEBUG("DWL: HW lock sem %x aquired\n", key);
        }
        else if(errno == ENOENT)
        {
            semid = binary_semaphore_allocation(key, IPC_CREAT | O_RDWR);

            binary_semaphore_initialize(semid);

            DWL_DEBUG("DWL: HW lock sem %x created\n", key);
        }
        else
        {
            DWL_DEBUG("DWL: FAILED to get HW lock sem %x\n", key);
            goto err;
        }

        dec_dwl->semid = semid;
    }
#else
	/* no need to allocate the semaphore when using ioctls to do the locking */
	binary_semaphore_initialize(dec_dwl->fd);
#endif

    pthread_mutex_unlock(&x170_init_mutex);
    return dec_dwl;

  err:

    pthread_mutex_unlock(&x170_init_mutex);
    DWLRelease(dec_dwl);

    return NULL;
}

/*------------------------------------------------------------------------------
    Function name   : DWLRelease
    Description     : Release a DWl instance

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - instance to be released
------------------------------------------------------------------------------*/
i32 DWLRelease(const void *instance)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    assert(dec_dwl != NULL);

    pthread_mutex_lock(&x170_init_mutex);
    if(dec_dwl->pRegBase != MAP_FAILED)
        DWLUnmapRegisters((void *) dec_dwl->pRegBase, dec_dwl->regSize);

    if(dec_dwl->fd_mem != -1)
        close(dec_dwl->fd_mem);

    if(dec_dwl->fd != -1)
        close(dec_dwl->fd);

    /* linear memory allocator */
    if(dec_dwl->fd_memalloc != -1)
        close(dec_dwl->fd_memalloc);

#ifdef _DWL_HW_PERFORMANCE
    switch (dec_dwl->clientType)
    {
    case DWL_CLIENT_TYPE_H264_DEC:
    case DWL_CLIENT_TYPE_MPEG4_DEC:
    case DWL_CLIENT_TYPE_JPEG_DEC:
    case DWL_CLIENT_TYPE_VC1_DEC:
    case DWL_CLIENT_TYPE_MPEG2_DEC:
    case DWL_CLIENT_TYPE_VP6_DEC:
    case DWL_CLIENT_TYPE_VP8_DEC:
    case DWL_CLIENT_TYPE_RV_DEC:
    case DWL_CLIENT_TYPE_AVS_DEC:
        fprintf(hw_performance_log, "dec;");
        fprintf(hw_performance_log, "%d;", dec_pic_counter);
        fprintf(hw_performance_log, "%d\n", dec_time);
        break;
    case DWL_CLIENT_TYPE_PP:
        fprintf(hw_performance_log, "pp;");
        fprintf(hw_performance_log, "%d;", pp_pic_counter);
        fprintf(hw_performance_log, "%d\n", pp_time);
    }
#endif

#ifdef INTERNAL_TEST
    fclose(dec_dwl->regDump);
    dec_dwl->regDump = NULL;
#endif
    free(dec_dwl);

    pthread_mutex_unlock(&x170_init_mutex);

    return (DWL_OK);
}

/*------------------------------------------------------------------------------
    Function name   : DWLWaitDecHwReady
    Description     : Wait until hardware has stopped running
                        and Decoder interrupt comes.
                      Used for synchronizing software runs with the hardware.
                      The wait could succed, timeout, or fail with an error.

    Return type     : i32 - one of the values DWL_HW_WAIT_OK
                                              DWL_HW_WAIT_TIMEOUT
                                              DWL_HW_WAIT_ERROR

    Argument        : const void * instance - DWL instance
------------------------------------------------------------------------------*/
i32 DWLWaitDecHwReady(const void *instance, u32 timeout)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	int ret;

#ifdef _DWL_HW_PERFORMANCE
    u32 offset = 0;
    u32 val = 0;
    struct timeval end_time;
#endif

    DWL_DEBUG("DWLWaitDecHwReady: Start\n");

    /* Check invalid parameters */
    if(dec_dwl == NULL)
    {
        assert(0);
        return DWL_HW_WAIT_ERROR;
    }

    /* keep looping for the int until it is a decoder int */
    while(!DWL_DECODER_INT)
    {
		ret = ioctl(dec_dwl->fd, HX170DEC_IOC_WAIT_DEC, NULL);
    }

#ifdef _DWL_HW_PERFORMANCE
    /*if (-1 == gettimeofday(&end_time, NULL))
     * assert(0); */

    /* decoder mode is enabled */
    if(dec_start_time.tv_sec)
    {
        offset = HX170DEC_REG_START / 4;
        val = *(dec_dwl->pRegBase + offset);
        if(val & DWL_HW_PIC_RDY_BIT)
        {
            u32 tmp = 0;

            DWL_DEBUG("DWLWaitDecHwReady: decoder pic rdy\n");
            dec_pic_counter++;
            ioctl(dec_dwl->fd, HX170DEC_HW_PERFORMANCE, &end_time);
            tmp =
                (end_time.tv_sec -
                 dec_start_time.tv_sec) * (1 / HW_PERFORMANCE_RESOLUTION);
            tmp +=
                (end_time.tv_usec -
                 dec_start_time.tv_usec) / (1000000 / (1 /
                                                       HW_PERFORMANCE_RESOLUTION));
            dec_time += tmp;
        }
    }
#endif

    DWL_DEBUG("DWLWaitDecHwReady: OK\n");
    return DWL_HW_WAIT_OK;

}


/*------------------------------------------------------------------------------
    Function name   : DWLWaitPpHwReady
    Description     : Wait until hardware has stopped running
                      and pp interrupt comes.
                      Used for synchronizing software runs with the hardware.
                      The wait could succed, timeout, or fail with an error.

    Return type     : i32 - one of the values DWL_HW_WAIT_OK
                                              DWL_HW_WAIT_TIMEOUT
                                              DWL_HW_WAIT_ERROR

    Argument        : const void * instance - DWL instance
------------------------------------------------------------------------------*/
i32 DWLWaitPpHwReady(const void *instance, u32 timeout)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	int ret;

#ifdef _DWL_HW_PERFORMANCE
    u32 offset = 0;
    u32 val = 0;
    struct timeval end_time;
#endif
    DWL_DEBUG("DWLWaitPpHwReady: Start\n");

    /* Check invalid parameters */
    if(dec_dwl == NULL)
    {
        assert(0);
        return DWL_HW_WAIT_ERROR;
    }

    /* keep looping for the int until it is a PP int */
    while(!DWL_PP_INT)
    {
		ret = ioctl(dec_dwl->fd, HX170DEC_IOC_WAIT_PP, NULL);
    }

#ifdef _DWL_HW_PERFORMANCE
    /*if (-1 == gettimeofday(&end_time, NULL))
     * assert(0); */

    /* pp external mode is enabled */
    if(pp_start_time.tv_sec)
    {
        offset = HX170PP_REG_START / 4;
        val = *(dec_dwl->pRegBase + offset);
        if(val & DWL_HW_PIC_RDY_BIT)
        {
            u32 tmp = 0;

            DWL_DEBUG("DWLWaitPpHwReady: pp pic rdy\n");
            pp_pic_counter++;
            ioctl(dec_dwl->fd, HX170DEC_HW_PERFORMANCE, &end_time);
            tmp =
                (end_time.tv_sec -
                 pp_start_time.tv_sec) * (1 / HW_PERFORMANCE_RESOLUTION);
            tmp +=
                (end_time.tv_usec -
                 pp_start_time.tv_usec) / (1000000 / (1 /
                                                      HW_PERFORMANCE_RESOLUTION));
            pp_time += tmp;
        }
    }
#endif

    DWL_DEBUG("DWLWaitPpHwReady: OK\n");
    return DWL_HW_WAIT_OK;

}

/* HW locking */

/*------------------------------------------------------------------------------
    Function name   : DWLReserveHw
    Description     :
    Return type     : i32
    Argument        : const void *instance
------------------------------------------------------------------------------*/
i32 DWLReserveHw(const void *instance)
{
    i32 ret;
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    do
    {
        /* select which semaphore to use */
        if(dec_dwl->clientType == DWL_CLIENT_TYPE_PP)
        {
            DWL_DEBUG("DWL: PP locked by PID %d\n", getpid());
#ifdef USE_LINUX_LOCK_IOCTL
            ret = binary_semaphore_wait(dec_dwl->fd, 1);
#else
            ret = binary_semaphore_wait(dec_dwl->semid, 1);
#endif
        }
        else
        {
            DWL_DEBUG("DWL: Dec locked by PID %d\n", getpid());
#ifdef USE_LINUX_LOCK_IOCTL
            ret = binary_semaphore_wait(dec_dwl->fd, 0);
#else
            ret = binary_semaphore_wait(dec_dwl->semid, 0);
#endif
        }
    }   /* if error is "error, interrupt", try again */
    while(ret != 0 && errno == EINTR);

    if(ret) return DWL_ERROR;

	DWL_DEBUG("DWL: success\n");

	return DWL_OK;
}

#ifdef _DWL_HW_PERFORMANCE
void DwlDecoderEnable(void)
{
    DWL_DEBUG("DWL: decoder enabled\n");

    if(-1 == gettimeofday(&dec_start_time, NULL))
        assert(0);
}

void DwlPpEnable(void)
{
    DWL_DEBUG("DWL: pp enabled\n");

    if(-1 == gettimeofday(&pp_start_time, NULL))
        assert(0);
}
#endif
