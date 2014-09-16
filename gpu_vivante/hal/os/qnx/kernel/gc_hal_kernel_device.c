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
#include <sys/mman.h>
#include <sys/siginfo.h>
#include <errno.h>

#define GC_HAL_QNX_PULSEVAL_INTR    (_PULSE_CODE_MINAVAIL)
#define GC_HAL_QNX_PULSEVAL_MAJOR   (GC_HAL_QNX_PULSEVAL_INTR + gcvCORE_MAJOR)
#define GC_HAL_QNX_PULSEVAL_2D      (GC_HAL_QNX_PULSEVAL_INTR + gcvCORE_2D)
#define GC_HAL_QNX_PULSEVAL_VG      (GC_HAL_QNX_PULSEVAL_INTR + gcvCORE_VG)

#define _GC_OBJ_ZONE    gcvZONE_DEVICE

/******************************************************************************\
******************************** gckGALDEVICE Code *******************************
\******************************************************************************/

gceSTATUS
gckGALDEVICE_AllocateMemory(
    IN gckGALDEVICE Device,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER *Logical,
    OUT gctPHYS_ADDR *Physical,
    OUT gctUINT32 *PhysAddr
    )
{
    gceSTATUS status;

    gcmkHEADER_ARG("Bytes=%u", Bytes);

    gcmkVERIFY_ARGUMENT(Device != NULL);
    gcmkVERIFY_ARGUMENT(Logical != NULL);
    gcmkVERIFY_ARGUMENT(Physical != NULL);
    gcmkVERIFY_ARGUMENT(PhysAddr != NULL);

    status = gckOS_AllocateContiguous(Device->os,
                      gcvFALSE,
                      &Bytes,
                      Physical,
                      Logical);

    if (gcmIS_ERROR(status))
    {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
                       "gckGALDEVICE_AllocateMemory: error status->0x%x",
                       status);
        gcmkFOOTER();
        return status;
    }

    *PhysAddr = (gctUINT32)(*(off_t*) Physical) - Device->baseAddress;
    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                   "gckGALDEVICE_AllocateMemory: phys_addr->0x%x phsical->0x%x Logical->0x%x",
                   (gctUINT32)*Physical,
                   (gctUINT32)*PhysAddr,
                   (gctUINT32)*Logical);

    /* Success. */
    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckGALDEVICE_FreeMemory(
    IN gckGALDEVICE Device,
    IN gctPOINTER Logical,
    IN gctPHYS_ADDR Physical)
{
    gceSTATUS status;

    gcmkHEADER_ARG("Logical=0x%08x", Logical);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    status = gckOS_FreeContiguous(Device->os,
                                Physical,
                                Logical,
                                0);

    gcmkFOOTER();
    return status;
}

/* TODO: Make part of device. */
struct sigevent gIntrEvents[gcdCORE_COUNT];
int gIntrChids[gcdCORE_COUNT];
int gIntrCoids[gcdCORE_COUNT];
volatile unsigned g_nQnxInIsrs = 0;

const struct sigevent* isrRoutine(void* arg, int id)
{
    gckGALDEVICE device = (gckGALDEVICE)arg;

    atomic_add(&g_nQnxInIsrs, 1);

    if (gckKERNEL_Notify(device->kernels[gcvCORE_MAJOR],
                         gcvNOTIFY_INTERRUPT,
                         gcvTRUE) == gcvSTATUS_OK)
    {
        atomic_sub(&g_nQnxInIsrs, 1);

        return &gIntrEvents[gcvCORE_MAJOR];
    }

    atomic_sub(&g_nQnxInIsrs, 1);

    return gcvNULL;
}

static void* threadRoutine(void *ctxt)
{
    int rc;
    struct _pulse pulse;

    gckGALDEVICE device = (gckGALDEVICE) ctxt;

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                "Starting ISR Thread with irq:%d\n",
                device->irqLines[gcvCORE_MAJOR]);

    device->isrInitializeds[gcvCORE_MAJOR] = gcvFALSE;

    if ((gIntrChids[gcvCORE_MAJOR] = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                        gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Setup_ISR: "
                        "Could not create channel (%s)\n",
                        strerror(errno));
        return (void *)1;
    }

    if ((gIntrCoids[gcvCORE_MAJOR] = ConnectAttach(0, 0, gIntrChids[gcvCORE_MAJOR], _NTO_SIDE_CHANNEL, 0)) == -1) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                        gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Setup_ISR: "
                        "Could not create connection (%s)\n",
                        strerror(errno));
        goto fail1;
    }

    SIGEV_PULSE_INIT(&gIntrEvents[gcvCORE_MAJOR], gIntrCoids[gcvCORE_MAJOR],
            21, GC_HAL_QNX_PULSEVAL_MAJOR, gcvNULL);

    /* Obtain I/O privileges */
    ThreadCtl( _NTO_TCTL_IO, 0 );

    device->irqIds[gcvCORE_MAJOR] = InterruptAttach(device->irqLines[gcvCORE_MAJOR],
                                    isrRoutine,
                                    (void*)device,
                                    gcmSIZEOF(struct _gckGALDEVICE),
                                    _NTO_INTR_FLAGS_TRK_MSK);

    if (device->irqIds[gcvCORE_MAJOR] < 0) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                gcvZONE_DRIVER,
                "[galcore] gckGALDEVICE_Setup_ISR: "
                "Could not register irq line->%d\n",
                device->irqLines[gcvCORE_MAJOR]);

        goto fail2;
    }

    gcmkTRACE_ZONE(gcvLEVEL_INFO,
                    gcvZONE_DRIVER,
                    "irqId:%d\n",
                    device->irqIds[gcvCORE_MAJOR]);

    printf("[Interrupt] Attached irqLine %d with id %d.\n",
        device->irqLines[gcvCORE_MAJOR], device->irqIds[gcvCORE_MAJOR]);

    device->isrInitializeds[gcvCORE_MAJOR] = gcvTRUE;

    while (1)
    {
        rc = MsgReceivePulse_r(gIntrChids[gcvCORE_MAJOR], &pulse, gcmSIZEOF(pulse), gcvNULL);

        if (rc == (-1 * EINTR))
        {
            continue;
        }

        if (rc == (-1 * ESRCH))
        {
            break;
        }

        if (rc != EOK)
        {
            continue;
        }

        if (pulse.code == GC_HAL_QNX_PULSEVAL_MAJOR)
        {
            gckKERNEL_Notify(device->kernels[gcvCORE_MAJOR], gcvNOTIFY_INTERRUPT, gcvFALSE);
        }
    }

    return (void *)0;

fail2:
    ConnectDetach(gIntrCoids[gcvCORE_MAJOR]);

fail1:
    ChannelDestroy(gIntrChids[gcvCORE_MAJOR]);

    return (void *)1;
}

const struct sigevent* isrRoutine2D(void* arg, int id)
{
    gckGALDEVICE device = (gckGALDEVICE)arg;

    atomic_add(&g_nQnxInIsrs, 1);

    if (gckKERNEL_Notify(device->kernels[gcvCORE_2D],
                         gcvNOTIFY_INTERRUPT,
                         gcvTRUE) == gcvSTATUS_OK)
    {
        atomic_sub(&g_nQnxInIsrs, 1);

        return &gIntrEvents[gcvCORE_2D];
    }

    atomic_sub(&g_nQnxInIsrs, 1);

    return gcvNULL;
}

static void* threadRoutine2D(void *ctxt)
{
    int rc;
    struct _pulse pulse;

    gckGALDEVICE device = (gckGALDEVICE) ctxt;

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                "Starting ISR Thread with irq:%d\n",
                device->irqLines[gcvCORE_2D]);

    device->isrInitializeds[gcvCORE_2D] = gcvFALSE;

    if ((gIntrChids[gcvCORE_2D] = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                        gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Setup_ISR: "
                        "Could not create channel (%s)\n",
                        strerror(errno));
        return (void *)1;
    }

    if ((gIntrCoids[gcvCORE_2D] = ConnectAttach(0, 0, gIntrChids[gcvCORE_2D], _NTO_SIDE_CHANNEL, 0)) == -1) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                        gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Setup_ISR: "
                        "Could not create connection (%s)\n",
                        strerror(errno));
        goto fail1;
    }

    SIGEV_PULSE_INIT(&gIntrEvents[gcvCORE_2D], gIntrCoids[gcvCORE_2D],
            21, GC_HAL_QNX_PULSEVAL_2D, gcvNULL);

    /* Obtain I/O privileges */
    ThreadCtl( _NTO_TCTL_IO, 0 );

    device->irqIds[gcvCORE_2D] = InterruptAttach(device->irqLines[gcvCORE_2D],
    								isrRoutine2D,
                                    (void*)device,
                                    gcmSIZEOF(struct _gckGALDEVICE),
                                    _NTO_INTR_FLAGS_TRK_MSK);

    if (device->irqIds[gcvCORE_2D] < 0) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                gcvZONE_DRIVER,
                "[galcore] gckGALDEVICE_Setup_ISR: "
                "Could not register irq line->%d\n",
                device->irqLines[gcvCORE_2D]);

        goto fail2;
    }

    gcmkTRACE_ZONE(gcvLEVEL_INFO,
                    gcvZONE_DRIVER,
                    "irqId:%d\n",
                    device->irqIds[gcvCORE_2D]);

    printf("[Interrupt] Attached irqLine %d with id %d.\n",
        device->irqLines[gcvCORE_2D], device->irqIds[gcvCORE_2D]);

    device->isrInitializeds[gcvCORE_2D] = gcvTRUE;

    while (1)
    {
        rc = MsgReceivePulse_r(gIntrChids[gcvCORE_2D], &pulse, gcmSIZEOF(pulse), gcvNULL);

        if (rc == (-1 * EINTR))
        {
            continue;
        }

        if (rc == (-1 * ESRCH))
        {
            break;
        }

        if (rc != EOK)
        {
            continue;
        }

        if (pulse.code == GC_HAL_QNX_PULSEVAL_2D)
        {
            gckKERNEL_Notify(device->kernels[gcvCORE_2D], gcvNOTIFY_INTERRUPT, gcvFALSE);
        }
    }

    return (void *)0;

fail2:
    ConnectDetach(gIntrCoids[gcvCORE_2D]);

fail1:
    ChannelDestroy(gIntrChids[gcvCORE_2D]);

    return (void *)1;
}

#if gcdENABLE_VG

static struct
{
    gckOS           os;
    gctSEMAPHORE    semaphore;
} s_VgIsrNotify[1024];

volatile unsigned s_nVgIsrIndex = 0;
volatile unsigned s_nVgIstIndex = 0;

const struct sigevent* isrRoutineVG(void* arg, int id)
{
    gckGALDEVICE device = (gckGALDEVICE)arg;

    gckOS os = gcvNULL;
    gctSEMAPHORE semaphore = gcvNULL;

    atomic_add(&g_nQnxInIsrs, 1);

    gckVGINTERRUPT_Enque(
            device->kernels[gcvCORE_VG]->vg->interrupt,
            &os, &semaphore);

    s_VgIsrNotify[s_nVgIsrIndex].os = os;
    s_VgIsrNotify[s_nVgIsrIndex].semaphore = semaphore;

    s_nVgIsrIndex = (s_nVgIsrIndex + 1) % gcmCOUNTOF(s_VgIsrNotify);

    atomic_sub(&g_nQnxInIsrs, 1);

    return &gIntrEvents[gcvCORE_VG];
}

static void* threadRoutineVG(void *ctxt)
{
    int rc;
    struct _pulse pulse;

    gckGALDEVICE device = (gckGALDEVICE) ctxt;

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                "Starting ISR Thread with irq:%d\n",
                device->irqLines[gcvCORE_VG]);

    device->isrInitializeds[gcvCORE_VG] = gcvFALSE;

    if ((gIntrChids[gcvCORE_VG] = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK)) == -1) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                        gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Setup_ISR: "
                        "Could not create channel (%s)\n",
                        strerror(errno));
        return (void *)1;
    }

    if ((gIntrCoids[gcvCORE_VG] = ConnectAttach(0, 0, gIntrChids[gcvCORE_VG], _NTO_SIDE_CHANNEL, 0)) == -1) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                        gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Setup_ISR: "
                        "Could not create connection (%s)\n",
                        strerror(errno));
        goto fail1;
    }

    SIGEV_PULSE_INIT(&gIntrEvents[gcvCORE_VG], gIntrCoids[gcvCORE_VG],
            21, GC_HAL_QNX_PULSEVAL_VG, gcvNULL);

    /* Obtain I/O privileges */
    ThreadCtl( _NTO_TCTL_IO, 0 );

    device->irqIds[gcvCORE_VG] = InterruptAttach(device->irqLines[gcvCORE_VG],
                                    isrRoutineVG,
                                    (void*)device,
                                    gcmSIZEOF(struct _gckGALDEVICE),
                                    _NTO_INTR_FLAGS_TRK_MSK);

    if (device->irqIds[gcvCORE_VG] < 0) {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                gcvZONE_DRIVER,
                "[galcore] gckGALDEVICE_Setup_ISR: "
                "Could not register irq line->%d\n",
                device->irqLines[gcvCORE_VG]);

        goto fail2;
    }

    gcmkTRACE_ZONE(gcvLEVEL_INFO,
                    gcvZONE_DRIVER,
                    "irqId:%d\n",
                    device->irqIds[gcvCORE_VG]);

    printf("[Interrupt] Attached irqLine %d with id %d.\n",
        device->irqLines[gcvCORE_VG], device->irqIds[gcvCORE_VG]);

    device->isrInitializeds[gcvCORE_VG] = gcvTRUE;

    while (1)
    {
        rc = MsgReceivePulse_r(gIntrChids[gcvCORE_VG], &pulse, gcmSIZEOF(pulse), gcvNULL);

        if (rc == (-1 * EINTR))
        {
            continue;
        }

        if (rc == (-1 * ESRCH))
        {
            break;
        }

        if (rc != EOK)
        {
            continue;
        }

        if (pulse.code == GC_HAL_QNX_PULSEVAL_VG)
        {
            if (s_VgIsrNotify[s_nVgIstIndex].semaphore != gcvNULL)
            {
                gckOS_IncrementSemaphore(
                        s_VgIsrNotify[s_nVgIstIndex].os,
                        s_VgIsrNotify[s_nVgIstIndex].semaphore);
            }

            s_nVgIstIndex = (s_nVgIstIndex + 1) % gcmCOUNTOF(s_VgIsrNotify);
        }
    }

    return (void *)0;

fail2:
    ConnectDetach(gIntrCoids[gcvCORE_VG]);

fail1:
    ChannelDestroy(gIntrChids[gcvCORE_VG]);

    return (void *)1;
}

#endif /* gcdENABLE_VG */

/*******************************************************************************
**
**  gckGALDEVICE_Setup_ISR
**
**  Start the ISR routine.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK
**          Setup successfully.
**      gcvSTATUS_GENERIC_IO
**          Setup failed.
*/
gceSTATUS
gckGALDEVICE_Setup_ISR(
    IN gckGALDEVICE Device
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gckGALDEVICE_Setup_ISR_2D(
    IN gckGALDEVICE Device
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gckGALDEVICE_Setup_ISR_VG(
    IN gckGALDEVICE Device
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckGALDEVICE_Release_ISR
**
**  Release the irq line.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Release_ISR(
    IN gckGALDEVICE Device
    )
{
    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckGALDEVICE_Release_ISR_2D(
    IN gckGALDEVICE Device
    )
{
    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gckGALDEVICE_Release_ISR_VG(
    IN gckGALDEVICE Device
    )
{
    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

int
gsl_free_interrupts()
{
    return 0;
}
/*******************************************************************************
**
**  gckGALDEVICE_Start_Threads
**
**  Start the daemon thread.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK
**          Start successfully.
**      gcvSTATUS_GENERIC_IO
**          Start failed.
*/
gceSTATUS
gckGALDEVICE_Start_Threads(
    IN gckGALDEVICE Device
    )
{
    gceSTATUS status;
    gctINT ret;

    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    gcmkTRACE_ZONE(gcvLEVEL_INFO,
                  gcvZONE_DRIVER,
                  "[galcore] gckGALDEVICE_Start_Threads: Creating threadRoutine\n");

    if (Device->kernels[gcvCORE_MAJOR] != gcvNULL)
    {
        /* Start the interrupt service thread */
        if ((ret = pthread_create(&Device->threadCtxts[gcvCORE_MAJOR], gcvNULL, threadRoutine, Device)) != 0)
        {
            gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                    gcvZONE_DRIVER,
                    "[galcore] gckGALDEVICE_Start_Threads: Failed with code %d\n",
                    ret);

            gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
        }

        pthread_setname_np(Device->threadCtxts[gcvCORE_MAJOR], "galcore-IST");

        Device->threadInitializeds[gcvCORE_MAJOR] = gcvTRUE;
        gcmkTRACE_ZONE(gcvLEVEL_INFO,
                gcvZONE_DRIVER,
                "[galcore] gckGALDEVICE_Start_Threads: "
                "Start the daemon thread.");
    }

    if (Device->kernels[gcvCORE_2D] != gcvNULL)
    {
        /* Start the interrupt service thread */
        if ((ret = pthread_create(&Device->threadCtxts[gcvCORE_2D], gcvNULL, threadRoutine2D, Device)) != 0)
        {
            gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                    gcvZONE_DRIVER,
                    "[galcore] gckGALDEVICE_Start_Threads: Failed with code %d\n",
                    ret);

            gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
        }

        pthread_setname_np(Device->threadCtxts[gcvCORE_2D], "galcore-IST-2D");

        Device->threadInitializeds[gcvCORE_2D] = gcvTRUE;
        gcmkTRACE_ZONE(gcvLEVEL_INFO,
                gcvZONE_DRIVER,
                "[galcore] gckGALDEVICE_Start_Threads: "
                "Start the daemon thread.");
    }
    else
    {
        Device->threadInitializeds[gcvCORE_2D]  = gcvFALSE;
    }

#if gcdENABLE_VG
    if (Device->kernels[gcvCORE_VG] != gcvNULL)
    {
        /* Start the interrupt service thread */
        if ((ret = pthread_create(&Device->threadCtxts[gcvCORE_VG], gcvNULL, threadRoutineVG, Device)) != 0)
        {
            gcmkTRACE_ZONE(gcvLEVEL_ERROR,
                    gcvZONE_DRIVER,
                    "[galcore] gckGALDEVICE_Start_Threads: Failed with code %d\n",
                    ret);

            gcmkONERROR(gcvSTATUS_OUT_OF_RESOURCES);
        }

        pthread_setname_np(Device->threadCtxts[gcvCORE_VG], "galcore-IST-VG");

        Device->threadInitializeds[gcvCORE_VG] = gcvTRUE;
        gcmkTRACE_ZONE(gcvLEVEL_INFO,
                gcvZONE_DRIVER,
                "[galcore] gckGALDEVICE_Start_Threads: "
                "Start the daemon thread.");
    }
    else
#endif /* gcdENABLE_VG */
    {
        Device->threadInitializeds[gcvCORE_VG]  = gcvFALSE;
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmkFOOTER();
    return status;
}


/*******************************************************************************
**
**  gckGALDEVICE_Stop_Threads
**
**  Stop the gal device, including the following actions: stop the daemon
**  thread, release the irq.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Stop_Threads(
    gckGALDEVICE Device
    )
{
    gctINT i;

    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        /* stop the threads */
        if (Device->threadInitializeds[i])
        {
            InterruptDetach(Device->irqIds[i]);
            Device->irqIds[i] = 0;
            ConnectDetach(Device->coids[i]);
            Device->coids[i] = 0;
            ChannelDestroy(Device->chids[i]);
            Device->chids[i] = 0;

            pthread_cancel(Device->threadCtxts[i]);
            pthread_join(Device->threadCtxts[i], NULL);
            Device->threadCtxts[i] = 0;

            Device->threadInitializeds[i] = gcvFALSE;
            Device->isrInitializeds[i] = gcvFALSE;
        }
    }

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckGALDEVICE_Start
**
**  Start the gal device, including the following actions: setup the isr routine
**  and start the daemon thread.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      gcvSTATUS_OK
**          Start successfully.
*/
gceSTATUS
gckGALDEVICE_Start(
    IN gckGALDEVICE Device
    )
{
    gceSTATUS ret;

    /* Start the daemon thread. */
    gcmkVERIFY_OK((ret = gckGALDEVICE_Start_Threads(Device)));

    return ret;
}

/*******************************************************************************
**
**  gckGALDEVICE_Stop
**
**  Stop the gal device, including the following actions: stop the daemon
**  thread, release the irq.
**
**  INPUT:
**
**      gckGALDEVICE Device
**          Pointer to an gckGALDEVICE object.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Stop(
    gckGALDEVICE Device
    )
{
    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkVERIFY_ARGUMENT(Device != NULL);

    gckGALDEVICE_Stop_Threads(Device);

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gckGALDEVICE_Construct
**
**  Constructor.
**
**  INPUT:
**
**  OUTPUT:
**
**      gckGALDEVICE * Device
**          Pointer to a variable receiving the gckGALDEVICE object pointer on
**          success.
*/
gceSTATUS
gckGALDEVICE_Construct(
    IN gctINT IrqLine,
    IN gctUINT32 RegisterMemBase,
    IN gctSIZE_T RegisterMemSize,
    IN gctINT IrqLine2D,
    IN gctUINT32 RegisterMemBase2D,
    IN gctSIZE_T RegisterMemSize2D,
    IN gctINT IrqLineVG,
    IN gctUINT32 RegisterMemBaseVG,
    IN gctSIZE_T RegisterMemSizeVG,
    IN gctUINT32 ContiguousBase,
    IN gctSIZE_T ContiguousSize,
    IN gctSIZE_T BankSize,
    IN gctINT FastClear,
    IN gctINT Compression,
    IN gctUINT32 BaseAddress,
    OUT gckGALDEVICE *Device
)
{
    gctUINT32 internalBaseAddress = 0, internalAlignment = 0;
    gctUINT32 externalBaseAddress = 0, externalAlignment = 0;
    gctUINT32 horizontalTileSize, verticalTileSize;
    gctUINT32 physAddr;
    gctUINT32 physical;
    gckGALDEVICE device;
    gceSTATUS status;
    gctINT32 i;
    gceHARDWARE_TYPE type;
    gckDB sharedDB = gcvNULL;

    gcmkTRACE(gcvLEVEL_VERBOSE, "[galcore] Enter gckGALDEVICE_Construct");

    /* Allocate device structure. */
    device = calloc(1, gcmSIZEOF(struct _gckGALDEVICE));
    if (!device)
    {
        gcmkTRACE_ZONE(gcvLEVEL_ERROR, gcvZONE_DRIVER,
                      "[galcore] gckGALDEVICE_Construct: Can't allocate memory.");

        return gcvSTATUS_OUT_OF_MEMORY;
    }

    memset(device, 0, gcmSIZEOF(struct _gckGALDEVICE));

    if (IrqLine != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_MAJOR]    = RegisterMemBase;
        device->requestedRegisterMemSizes[gcvCORE_MAJOR]    = RegisterMemSize;
    }

    if (IrqLine2D != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_2D]       = RegisterMemBase2D;
        device->requestedRegisterMemSizes[gcvCORE_2D]       = RegisterMemSize2D;
    }

    if (IrqLineVG != -1)
    {
        device->requestedRegisterMemBases[gcvCORE_VG]       = RegisterMemBaseVG;
        device->requestedRegisterMemSizes[gcvCORE_VG]       = RegisterMemSizeVG;
    }

    device->requestedContiguousBase  = 0;
    device->requestedContiguousSize  = 0;

    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        physical = device->requestedRegisterMemBases[i];

        /* Set up register memory region */
        if (physical != 0)
        {
            /* Request a region. */
            device->registerBases[i] = (gctPOINTER)mmap_device_io(
                    device->requestedRegisterMemSizes[i],
                    device->requestedRegisterMemBases[i]);

            if ((uintptr_t)device->registerBases[i] == MAP_DEVICE_FAILED)
            {
                gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Construct: Unable to map location->0x%lX for size->%ld\n",
                        device->requestedRegisterMemBases[i],
                        device->requestedRegisterMemSizes[i]);

                return gcvSTATUS_OUT_OF_RESOURCES;
            }

            physical += device->requestedRegisterMemSizes[i];

            device->registerSizes[i] = device->requestedRegisterMemSizes[i];

            gcmkTRACE_ZONE(gcvLEVEL_INFO,
                    gcvZONE_DRIVER,
                    "[galcore] gckGALDEVICE_Construct: "
                    "RegisterBase after mapping Address->0x%x is 0x%x",
                    (gctUINT32)device->requestedRegisterMemBases[i],
                    (gctUINT32)device->registerBases[i]);
        }
        else
        {
            device->registerBases[i] = gcvNULL;
        }
    }

    /* Set the base address */
    device->baseAddress = BaseAddress;

    /* construct the gckOS object */
    gcmkONERROR(gckOS_Construct(device, &device->os));

#if gcdDEBUG
    gckOS_SetDebugLevel(gcvLEVEL_VERBOSE);
    /*gckOS_SetDebugFile("/var/gc_dump.txt");*/

    /*gckOS_SetDebugZones(gcvZONE_API_HAL | gcvZONE_VIDMEM, gcvTRUE);*/
    gckOS_SetDebugZones(gcvZONE_API_HAL | gcvZONE_ALL, gcvTRUE);
#endif

    if (IrqLine != -1)
    {
        /* construct the gckKERNEL object. */
        gcmkONERROR(
                gckKERNEL_Construct(device->os, gcvCORE_MAJOR, device, gcvNULL, &device->kernels[gcvCORE_MAJOR]));

        sharedDB = device->kernels[gcvCORE_MAJOR]->db;

        /* Initialize core mapping */
        for (i = 0; i < 8; i++)
        {
            device->coreMapping[i] = gcvCORE_MAJOR;
        }

        gcmkONERROR(
                gckHARDWARE_SetFastClear(device->kernels[gcvCORE_MAJOR]->hardware,
                    FastClear,
                    Compression));

#if COMMAND_PROCESSOR_VERSION == 1
        /* start the command queue */
        gcmkVERIFY_OK(gckCOMMAND_Start(device->kernels[gcvCORE_MAJOR]->command));
#endif
    }
    else
    {
        device->kernels[gcvCORE_MAJOR] = gcvNULL;
    }

    if (IrqLine2D != -1)
    {
        /* construct the gckKERNEL object. */
        gcmkONERROR(
                gckKERNEL_Construct(device->os, gcvCORE_2D, device, gcvNULL, &device->kernels[gcvCORE_2D]));

        if (sharedDB == gcvNULL)
        {
            sharedDB = device->kernels[gcvCORE_2D]->db;
        }

        /* Verify the hardware type */
        gcmkONERROR(gckHARDWARE_GetType(device->kernels[gcvCORE_2D]->hardware, &type));

        if (type != gcvHARDWARE_2D)
        {
            gcmkTRACE_ZONE(
                    gcvLEVEL_ERROR, gcvZONE_DRIVER,
                    "%s(%d): Unexpected hardware type: %d\n",
                    __FUNCTION__, __LINE__,
                    type
                    );

            gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }

        /* Initialize core mapping */
        if (device->kernels[gcvCORE_MAJOR] == gcvNULL)
        {
            for (i = 0; i < 8; i++)
            {
                device->coreMapping[i] = gcvCORE_2D;
            }
        }
        else
        {
            device->coreMapping[gcvHARDWARE_2D] = gcvCORE_2D;
        }

#if COMMAND_PROCESSOR_VERSION == 1
        /* start the command queue */
        gcmkVERIFY_OK(gckCOMMAND_Start(device->kernels[gcvCORE_2D]->command));
#endif
    }
    else
    {
        device->kernels[gcvCORE_2D] = gcvNULL;
    }

    if (IrqLineVG != -1)
    {
#if gcdENABLE_VG
        gcmkONERROR(gckKERNEL_Construct(
                    device->os, gcvCORE_VG, device,
                    sharedDB, &device->kernels[gcvCORE_VG]));

        /* Initialize core mapping */
        if (device->kernels[gcvCORE_MAJOR] == gcvNULL
                && device->kernels[gcvCORE_2D] == gcvNULL)
        {
            for (i = 0; i < 8; i++)
            {
                device->coreMapping[i] = gcvCORE_VG;
            }
        }
        else
        {
            device->coreMapping[gcvHARDWARE_VG] = gcvCORE_VG;
        }
#endif /* gcdENABLE_VG */
    }
    else
    {
        device->kernels[gcvCORE_VG] = gcvNULL;
    }

    /* Initialize the ISR. */
    device->irqLines[gcvCORE_MAJOR] = IrqLine;
    device->irqLines[gcvCORE_2D]    = IrqLine2D;
    device->irqLines[gcvCORE_VG]    = IrqLineVG;

    for (i = 0; i < gcdCORE_COUNT; i++)
    {
        if (device->kernels[i] != gcvNULL) break;
    }

    if (i == gcdCORE_COUNT)
    {
        gcmkONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

#if gcdENABLE_VG
    if (i == gcvCORE_VG)
    {
        /* query the ceiling of the system memory */
        gcmkONERROR(gckVGHARDWARE_QuerySystemMemory(device->kernels[i]->vg->hardware,
                    &device->systemMemorySize,
                    &device->systemMemoryBaseAddress));

        /* query the amount of video memory */
        gcmkVERIFY_OK(gckVGHARDWARE_QueryMemory(device->kernels[i]->vg->hardware,
                    &device->internalSize,
                    &internalBaseAddress,
                    &internalAlignment,
                    &device->externalSize,
                    &externalBaseAddress,
                    &externalAlignment,
                    &horizontalTileSize,
                    &verticalTileSize));
    }
    else
#endif /* gcdENABLE_VG */
    {
        /* query the ceiling of the system memory */
        gcmkONERROR(gckHARDWARE_QuerySystemMemory(device->kernels[i]->hardware,
                    &device->systemMemorySize,
                    &device->systemMemoryBaseAddress));

        /* query the amount of video memory */
        gcmkVERIFY_OK(gckHARDWARE_QueryMemory(device->kernels[i]->hardware,
                    &device->internalSize,
                    &internalBaseAddress,
                    &internalAlignment,
                    &device->externalSize,
                    &externalBaseAddress,
                    &externalAlignment,
                    &horizontalTileSize,
                    &verticalTileSize));
    }

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
            "[galcore] gckGALDEVICE_Construct: "
            "Will be trying to allocate contiguous memory of 0x%x bytes",
            (gctUINT32)device->systemMemoryBaseAddress);

    /* initialize the thread daemon */
    memset(&device->isrLock, 0, sizeof(device->isrLock));


    /* set up the internal memory region */
    if (device->internalSize > 0)
    {
        gceSTATUS status = gckVIDMEM_Construct(device->os,
                    internalBaseAddress,
                    device->internalSize,
                    internalAlignment,
                    0,
                    &device->internalVidMem);

        if (gcmIS_ERROR(status))
        {
            /* error, remove internal heap */
            device->internalSize = 0;
        }
        else
        {
            /* map internal memory */
            device->internalPhysical  = (gctPHYS_ADDR)physical;
            device->internalLogical = (gctPOINTER)mmap_device_io(device->internalSize, physical);

            gcmkASSERT(device->internalLogical != NULL);

            physical += device->internalSize;
        }
    }

    if (device->externalSize > 0)
    {
        /* create the external memory heap */
        gceSTATUS status = gckVIDMEM_Construct(device->os,
                    externalBaseAddress,
                    device->externalSize,
                    externalAlignment,
                    0,
                    &device->externalVidMem);

        if (gcmIS_ERROR(status))
        {
            /* error, remove internal heap */
            device->externalSize = 0;
        }
        else
        {
            /* map internal memory */
            device->externalPhysical = (gctPHYS_ADDR)physical;
            device->externalLogical = (gctPOINTER)mmap_device_io(device->externalSize, physical);

            gcmkASSERT(device->externalLogical != NULL);

            physical += device->externalSize;
        }
    }

    /* set up the contiguous memory */
    device->contiguousSize = ContiguousSize;

    if ((ContiguousSize > 0) && (ContiguousBase == 0))
    {
        status = gcvSTATUS_OUT_OF_MEMORY;

        while (device->contiguousSize > 0)
        {
            gcmkTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_DRIVER,
                "[galcore] gckGALDEVICE_Construct: Will be trying to allocate contiguous memory of %ld bytes",
                device->contiguousSize
                );

            /* allocate contiguous memory */
            status = gckGALDEVICE_AllocateMemory(
                device,
                device->contiguousSize,
                &device->contiguousBase,
                &device->contiguousPhysical,
                &physAddr
                );

            if (gcmIS_SUCCESS(status))
            {
                gcmkTRACE_ZONE(
                    gcvLEVEL_INFO, gcvZONE_DRIVER,
                    "[galcore] gckGALDEVICE_Construct: Contiguous allocated size->0x%08X Virt->0x%08lX physAddr->0x%08X",
                    device->contiguousSize,
                    device->contiguousBase,
                    physAddr
                    );

                status = gckVIDMEM_Construct(
                    device->os,
                    physAddr | device->systemMemoryBaseAddress,
                    device->contiguousSize,
                    64,
                    BankSize,
                    &device->contiguousVidMem
                    );

                if (gcmIS_SUCCESS(status))
                {
                    device->contiguousMapped = gcvFALSE;

                    /* success, abort loop */
                    gcmkTRACE_ZONE(
                        gcvLEVEL_INFO, gcvZONE_DRIVER,
                        "Using %u bytes of contiguous memory.",
                        device->contiguousSize
                        );

                    break;
                }

                gcmkVERIFY_OK(gckGALDEVICE_FreeMemory(
                    device,
                    device->contiguousBase,
                    device->contiguousPhysical
                    ));

                device->contiguousBase = NULL;
            }

            if (device->contiguousSize <= (4 << 20))
            {
                device->contiguousSize = 0;
            }
            else
            {
                device->contiguousSize -= (4 << 20);
            }
        }
    }
    else if (ContiguousSize > 0)
    {
        /* Create the contiguous memory heap. */
        status = gckVIDMEM_Construct(
            device->os,
            (ContiguousBase - device->baseAddress) | device->systemMemoryBaseAddress,
            ContiguousSize,
            64,
            BankSize,
            &device->contiguousVidMem
            );

        if (gcmIS_ERROR(status))
        {
            /* Error, roll back. */
            device->contiguousVidMem = gcvNULL;
            device->contiguousSize   = 0;
        }
        else
        {
            /* Map the contiguous memory. */
            device->contiguousPhysical = (gctPHYS_ADDR) ContiguousBase;
            device->contiguousSize     = ContiguousSize;
            device->contiguousBase     = (gctPOINTER) mmap_device_io(ContiguousSize, ContiguousBase);
            device->contiguousMapped   = gcvTRUE;

            if (device->contiguousBase == gcvNULL)
            {
                /* Error, roll back. */
                gcmkVERIFY_OK(gckVIDMEM_Destroy(device->contiguousVidMem));
                device->contiguousVidMem = gcvNULL;
                device->contiguousSize   = 0;

                status = gcvSTATUS_OUT_OF_RESOURCES;
            }
        }
    }

#if gcdUSE_VIDMEM_PER_PID
    device->contiguousVidMemList = gcvNULL;
    gcmkVERIFY_OK(gckOS_CreateMutex(device->os, &device->contiguousVidMemListMutex));
#endif

    *Device = device;

    gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                  "[galcore] gckGALDEVICE_Construct: Initialized device->%p contiguous->%lu @ %p (0x%08X)",
                  device,
                  device->contiguousSize,
                  device->contiguousBase,
                  device->contiguousPhysical);

    return gcvSTATUS_OK;
OnError:
    /* Roll back. */

    /* Destroy the gckKERNEL object. */
    if ( Device != gcvNULL)
    {
        gcmkVERIFY_OK(gckGALDEVICE_Destroy(*Device));
    }

    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gckGALDEVICE_Destroy
**
**  Class destructor.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURNS:
**
**      Nothing.
*/
gceSTATUS
gckGALDEVICE_Destroy(
    gckGALDEVICE Device)
{
    gctINT i;

    gcmkHEADER_ARG("Device=0x%x", Device);

    gcmkTRACE(gcvLEVEL_VERBOSE, "[ENTER] gckGALDEVICE_Destroy");

    if (Device != gcvNULL)
    {
        for (i = 0; i < gcdCORE_COUNT; i++)
        {
            if (Device->kernels[i] != gcvNULL)
            {
                /* Destroy the gckKERNEL object. */
                gcmkVERIFY_OK(gckKERNEL_Destroy(Device->kernels[i]));
                Device->kernels[i] = gcvNULL;
            }
        }

        if (Device->internalVidMem != gcvNULL)
        {
            /* destroy the internal heap */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->internalVidMem));

            /* unmap the internal memory */
            munmap_device_io((uintptr_t)Device->internalLogical, Device->internalSize);
        }

        if (Device->externalVidMem != gcvNULL)
        {
            /* destroy the internal heap */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->externalVidMem));

            /* unmap the external memory */
            munmap_device_io((uintptr_t)Device->externalLogical, Device->externalSize);
        }

        if (Device->contiguousVidMem != gcvNULL)
        {
            /* Destroy the contiguous heap */
            gcmkVERIFY_OK(gckVIDMEM_Destroy(Device->contiguousVidMem));

            if (Device->contiguousMapped)
            {
                gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Destroy: "
                        "Unmapping contiguous memory->0x%08lX",
                        Device->contiguousBase);

                munmap_device_io((uintptr_t)Device->contiguousBase, Device->contiguousSize);
            }
            else if (Device->contiguousBase != gcvNULL)
            {
                gcmkTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_DRIVER,
                        "[galcore] gckGALDEVICE_Destroy: "
                        "Freeing contiguous memory->0x%08lX",
                        Device->contiguousBase);

                gcmkVERIFY_OK(gckGALDEVICE_FreeMemory(Device,
                            Device->contiguousBase,
                            Device->contiguousPhysical));
            }
        }

#if gcdUSE_VIDMEM_PER_PID
        if (Device->contiguousVidMemList != gcvNULL)
        {
            gckVIDMEM head;

            head = Device->contiguousVidMemList;

            while (head != gcvNULL)
            {
                Device->contiguousVidMemList = head->next;

                /* Destroy the contiguous heap */
                gcmkVERIFY_OK(gckVIDMEM_Destroy(head));

                head = Device->contiguousVidMemList;
            }

            /* Free the mutex. */
            gcmkVERIFY_OK(gckOS_DeleteMutex(Device->os, Device->contiguousVidMemListMutex));

            /* Shared memory cleanup handled separately. */
        }
#endif

        for (i = 0; i < gcdCORE_COUNT; i++)
        {
            if (Device->registerBases[i] != gcvNULL)
            {
                munmap_device_io((uintptr_t)Device->registerBases[i], Device->registerSizes[i]);
            }

            Device->registerBases[i] = 0;
            Device->registerSizes[i] = 0;
            Device->requestedRegisterMemBases[i] = 0;
            Device->requestedRegisterMemSizes[i] = 0;
        }

        /* Destroy the gckOS object. */
        gcmkVERIFY_OK(gckOS_Destroy(Device->os));

        free(Device);
    }

    gcmkTRACE(gcvLEVEL_VERBOSE, "[galcore] Leave gckGALDEVICE_Destroy");

    gcmkFOOTER_NO();
    return gcvSTATUS_OK;
}
