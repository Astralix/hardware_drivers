/****************************************************************************
*
*    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/




/**
**  @file
**  Architecture independent event queue management.
**
*/

#include "gc_hal_user_precomp.h"
#include "gc_hal_user_buffer.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_BUFFER

#ifdef __QNXNTO__
/* 32K buffer size, good enough for 1 frame. */
#define BUFFER_SIZE (1 << 15)
#endif

gceSTATUS
gcoQUEUE_Construct(
    IN gcoOS Os,
    OUT gcoQUEUE * Queue
    )
{
    gcoQUEUE queue = gcvNULL;
    gceSTATUS status;
#ifdef __QNXNTO__
    gctSIZE_T allocationSize;
    gctPHYS_ADDR physAddr;
#endif
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Queue != gcvNULL);

    /* Create the queue. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(struct _gcoQUEUE),
                       &pointer));
    queue = pointer;

    /* Initialize the object. */
    queue->object.type = gcvOBJ_QUEUE;

    /* Nothing in the queue yet. */
    queue->head = queue->tail = gcvNULL;

    queue->recordCount = 0;

#ifdef __QNXNTO__
    /* Allocate buffer of records. */
    allocationSize = BUFFER_SIZE;
    physAddr = 0;
    gcmONERROR(
        gcoOS_AllocateNonPagedMemory(gcvNULL,
                                     gcvTRUE,
                                     &allocationSize,
                                     &physAddr,
                                     (gctPOINTER *) &queue->records));
    queue->freeBytes = allocationSize;
    queue->offset = 0;
#else
    queue->freeList = gcvNULL;
#endif

    /* Return gcoQUEUE pointer. */
    *Queue = queue;

    /* Success. */
    gcmFOOTER_ARG("*Queue=0x%x", *Queue);
    return gcvSTATUS_OK;

OnError:
    if (queue != gcvNULL)
    {
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, queue));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoQUEUE_Destroy(
    IN gcoQUEUE Queue
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Queue=0x%x", Queue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);

#ifndef __QNXNTO__
    /* Free any records in the queue. */
    while (Queue->head != gcvNULL)
    {
        gcsQUEUE_PTR record;

        /* Unlink the first record from the queue. */
        record      = Queue->head;
        Queue->head = record->next;

        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, record));
    }

    /* Free any records in the free list. */
    while (Queue->freeList != gcvNULL)
    {
        gcsQUEUE_PTR record;

        /* Unlink the first record from the queue. */
        record          = Queue->freeList;
        Queue->freeList = record->next;

        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, record));
    }
#else
    /* Free any records in the queue. */
    if ( Queue->records )
    {
        gcmVERIFY_OK(gcoOS_FreeNonPagedMemory(gcvNULL,
                                              BUFFER_SIZE,
                                              gcvNULL,
                                              Queue->records));
    }
#endif

    /* Free the queue. */
    gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Queue));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoQUEUE_AppendEvent(
    IN gcoQUEUE Queue,
    IN gcsHAL_INTERFACE * Interface
    )
{
    gceSTATUS status;
    gcsQUEUE_PTR record = gcvNULL;
#ifdef __QNXNTO__
    gctSIZE_T allocationSize;
    gctPHYS_ADDR physAddr;
#else
    gctPOINTER pointer = gcvNULL;
#endif

    gcmHEADER_ARG("Queue=0x%x Interface=0x%x", Queue, Interface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);
    gcmVERIFY_ARGUMENT(Interface != gcvNULL);

    /* Allocate record. */
#ifdef __QNXNTO__
    allocationSize = gcmSIZEOF(gcsQUEUE);
    if (Queue->freeBytes < allocationSize)
    {
        gctSIZE_T recordsSize = BUFFER_SIZE;
        gcsQUEUE_PTR prevRecords = Queue->records;
        gcsHAL_INTERFACE iface;

        /* Allocate new set of records. */
        gcmONERROR(
            gcoOS_AllocateNonPagedMemory(gcvNULL,
                                         gcvTRUE,
                                         &recordsSize,
                                         &physAddr,
                                         (gctPOINTER *) &Queue->records));
        Queue->freeBytes = recordsSize;
        Queue->offset = 0;

        if ( Queue->freeBytes < allocationSize )
        {
            gcmFOOTER_ARG("status=%d", gcvSTATUS_DATA_TOO_LARGE);
            return gcvSTATUS_DATA_TOO_LARGE;
        }

        /* Schedule to free Queue->records,
         * hence not unmapping currently scheduled events immediately. */
        iface.command = gcvHAL_FREE_NON_PAGED_MEMORY;
        iface.u.FreeNonPagedMemory.bytes = BUFFER_SIZE;
        iface.u.FreeNonPagedMemory.physical = 0;
        iface.u.FreeNonPagedMemory.logical = prevRecords;

        gcmONERROR(
            gcoQUEUE_AppendEvent(Queue, &iface));
    }

    record = (gcsQUEUE_PTR)((gctUINT32)Queue->records + Queue->offset);
    Queue->offset += allocationSize;
    Queue->freeBytes -= allocationSize;
#else
    /* Check if we have records on the free list. */
    if (Queue->freeList != gcvNULL)
    {
        /* Allocate from hte free list. */
        record          = Queue->freeList;
        Queue->freeList = record->next;
    }
    else
    {
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(gcsQUEUE),
                                  &pointer));
        record = pointer;
    }
#endif

    /* Initialize record. */
    record->next  = gcvNULL;
    gcoOS_MemCopy(&record->iface, Interface, gcmSIZEOF(record->iface));

    if (Queue->head == gcvNULL)
    {
        /* Initialize queue. */
        Queue->head = record;
    }
    else
    {
        /* Append record to end of queue. */
        Queue->tail->next = record;
    }

    /* Mark end of queue. */
    Queue->tail = record;

    /* update count */
    Queue->recordCount++;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
#ifndef __QNXNTO__
    if (pointer != gcvNULL)
    {
        /* Put record on free list. */
        record->next    = Queue->freeList;
        Queue->freeList = record;
    }
#endif

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoQUEUE_Free(
    IN gcoQUEUE Queue
    )
{
    gcmHEADER_ARG("Queue=0x%x", Queue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);

    /* Free any records in the queue. */
#ifdef __QNXNTO__
    Queue->head = gcvNULL;
#else
    while (Queue->head != gcvNULL)
    {
        gcsQUEUE_PTR record;

        /* Unlink the first record from the queue. */
        record      = Queue->head;
        Queue->head = record->next;

        /* Put record on free list. */
        record->next    = Queue->freeList;
        Queue->freeList = record;
    }
#endif

    /* Update count */
    Queue->recordCount = 0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcoQUEUE_Commit(
    IN gcoQUEUE Queue
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Queue=0x%x", Queue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);

    if (Queue->head != gcvNULL)
    {
        /* Initialize event commit command. */
        iface.command       = gcvHAL_EVENT_COMMIT;
        iface.u.Event.queue = Queue->head;

        /* Send command to kernel. */
        gcmONERROR(
            gcoOS_DeviceControl(gcvNULL,
                                IOCTL_GCHAL_INTERFACE,
                                &iface, gcmSIZEOF(iface),
                                &iface, gcmSIZEOF(iface)));

        /* Test for error. */
        gcmONERROR(iface.status);

        /* Free any records in the queue. */
        gcmONERROR(gcoQUEUE_Free(Queue));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
