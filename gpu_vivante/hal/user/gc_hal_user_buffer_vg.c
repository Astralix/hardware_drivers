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




#include "gc_hal_user_precomp.h"

#if gcdENABLE_VG

#define _GC_OBJ_ZONE    gcvZONE_VG

/******************************************************************************\
********************************* Definitions **********************************
\******************************************************************************/

/* If not zero, COMMIT will wait until the hardware is idle. */
#define gcvFORCE_COMMIT_WAIT                0

/* Completion debugging. */
#define gcvENABLE_COMPLETION_DEBUG          0
#define gcvIGNORE_STATIC_COMPLETIONS        1

/* Command buffer debugging. */
#define gcvENABLE_BUFFER_DEBUG              0
#define gcvPRINT_BUFFER_CONTENT             0

/* Define a value N greater then zero to skip the first N messages. */
#define gcvMESSAGE_SKIP_COUNT               0

/* Validation enable for the buffer. */
#define gcvENABLE_CMDBUFFER_VALIDATION      0
#define gcvVALIDATE_COMMAND_BUFFER_VERBOSE  0

/* Validation for debugging. */
#if ENABLE_HAL_VALIDATION && gcvENABLE_CMDBUFFER_VALIDATION
#   define gcvVALIDATE_COMMAND_BUFFER       1
#else
#   define gcvVALIDATE_COMMAND_BUFFER       0
#endif

#if gcvVALIDATE_COMMAND_BUFFER_VERBOSE
#   define gcmCMDBUF_VERBOSE gcmBUFFER_TRACE
#else
#   define gcmCMDBUF_VERBOSE
#endif

#if gcvENABLE_COMPLETION_DEBUG
#   define gcmPRINT_COMPLETION(Buffer, CommandBuffer) \
        _PrintCompletion(__FUNCTION__, __LINE__, Buffer, CommandBuffer)
#else
#   define gcmPRINT_COMPLETION(Buffer, CommandBuffer)
#endif

#if gcvENABLE_BUFFER_DEBUG
#   define gcmPRINT_COMMAND_BUFFER(Buffer) \
        _PrintBuffers(__FUNCTION__, __LINE__, Buffer)
#else
#   define gcmPRINT_COMMAND_BUFFER(Buffer)
#endif

#if gcvMESSAGE_SKIP_COUNT
    static gctBOOL _bufferTraceEnable = gcvFALSE;
    static gctBOOL _callCounter = 0;
#   define gcmBUFFER_TRACE \
        if (_bufferTraceEnable) gcmTRACE_ZONE
#   define gcmCHECK_PRINT_WINDOW() \
        if (_callCounter == gcvMESSAGE_SKIP_COUNT) \
        { \
            _bufferTraceEnable = gcvTRUE; \
        } \
        _callCounter += 1
#else
#   define gcmBUFFER_TRACE \
        gcmTRACE_ZONE
#   define gcmCHECK_PRINT_WINDOW()
#endif

/* Number of completions to allocate per pool. */
#define gcvCOMPLETION_POOL_SIZE             4096
#define gcvCOMPLETIONS_PER_POOL \
    ( \
        (gcvCOMPLETION_POOL_SIZE - gcmSIZEOF(gcsCOMPLETION_POOL)) \
        / gcmSIZEOF(gcsCOMPLETION_SIGNAL) \
    )

/* Shorthand for asserting on invalid condition during buffer validation. */
#define gcmQUALIFY_BUFFER(Condition) \
{ \
    if (!(Condition)) \
    { \
        gcmFATAL( \
            "%s(%d): BUFFER INVALID [called from %s(%d)])\n", \
            __FUNCTION__, __LINE__, CallerName, LineNumber \
            ); \
        \
        return gcvSTATUS_INVALID_OBJECT; \
    } \
}

/* Validation macro. */
#if gcvVALIDATE_COMMAND_BUFFER
#   define vgmVALIDATE_BUFFER(Buffer) \
        gcmVERIFY_OK(_ValidateBuffer(Buffer, __FUNCTION__, __LINE__))
#else
#   define vgmVALIDATE_BUFFER(Buffer)
#endif

/* Empty queue marker. */
static gcsVGCMDQUEUE _emptyQueueMark;
#define gcvEMPTY_QUEUE (& _emptyQueueMark)


/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

/* Completion signal pool. */
typedef struct _gcsCOMPLETION_POOL * gcsCOMPLETION_POOL_PTR;
typedef struct _gcsCOMPLETION_POOL
{
    /* Pointer to the next pool. */
    gcsCOMPLETION_POOL_PTR      next;
}
gcsCOMPLETION_POOL;

/* Command buffer completion signal. */
typedef struct _gcsCOMPLETION_SIGNAL
{
    /* Signal to be set when associated command buffers have been executed. */
    gctSIGNAL                   complete;

    /* Number of command buffers that are associated with the signal. */
    gctINT                      reference;

    /* Pointer to the next vacant completion when a part of the free list. */
    gcsCOMPLETION_SIGNAL_PTR    nextFree;
}
gcsCOMPLETION_SIGNAL;

/* Task storage. */
typedef struct _gcsTASK_STORAGE * gcsTASK_STORAGE_PTR;
typedef struct _gcsTASK_STORAGE
{
    /* Pointer to the next storage buffer. */
    gcsTASK_STORAGE_PTR         next;
}
gcsTASK_STORAGE;

/* The command buffer management object. */
struct _gcoVGBUFFER
{
    /* Object. */
    gcsOBJECT                   object;

    /* Pointer to the required objects. */
    gcoOS                       os;
    gcoHAL                      hal;
    gcoVGHARDWARE                   hardware;
    gcsVGCONTEXT_PTR                context;

    /** Use CALL/RETURN enable. */
    gctBOOL                     useCallReturn;

    /* Command buffer attributes. */
    gcsCOMMAND_BUFFER_INFO      bufferInfo;

    /* Main container buffer node. */
    gcuVIDMEM_NODE_PTR          containerNode;

    /* The minimum and maximum buffer sizes. */
    gctSIZE_T                   bufferMinSize;
    gctSIZE_T                   bufferMaxSize;

    /* Command buffer repository. */
    gctUINT                     bufferRepositorySize;
    gcsCMDBUFFER_PTR            bufferCurrent;
    gcsCMDBUFFER_PTR            bufferLast;
    gctUINT8_PTR                bufferLimit;

    /* Uncommitted data threshold counters. */
    gctUINT                     uncommittedSize;
    gctUINT                     uncommittedThreshold;

    /* Task storage. */
    gcsTASK_STORAGE_PTR         taskStorageHead;
    gcsTASK_STORAGE_PTR         taskStorageCurr;
    gctUINT8_PTR                taskStorageCurrPtr;
    gctUINT                     taskStorageCurrAvailable;
    gctUINT                     taskStorageGranularity;
    gctUINT                     taskStorageMaxSize;

    /* Task tables. */
#ifndef __QNXNTO__
    gcsTASK_MASTER_TABLE        taskTable;
#else
    gcsTASK_MASTER_TABLE      * pTaskTable;
#endif
    gctUINT                     taskBlockCount;

    /* Command queue. */
    gcsVGCMDQUEUE_PTR               queueFirst;
    gcsVGCMDQUEUE_PTR               queueLast;
    gcsVGCMDQUEUE_PTR               queueCurrent;

    /* Pool of completion signals. */
    gctUINT                     completionsAllocated;
    gctUINT                     completionsFree;
    gcsCOMPLETION_POOL_PTR      completionPool;
    gcsCOMPLETION_SIGNAL_PTR    completionFree;
    gcsCOMPLETION_SIGNAL_PTR    completionCurrent;
    gcsCOMPLETION_SIGNAL_PTR    completionPrevious;
    gctHANDLE                   processID;

    /* Performance counters. */
    gctUINT                     bufferOverflow;
    gctUINT                     taskOverflow;
    gctUINT                     queueOverflow;
    gctUINT                     commitCount;
    gctUINT                     reserveCount;

    /* Restart command state. */
    gctPOINTER                  restart;
};

#if gcvVALIDATE_COMMAND_BUFFER
    static gctUINT32 _validateCounter = 0;
#endif


/******************************************************************************\
********************************* Support Code *********************************
\******************************************************************************/

#if gcvVALIDATE_COMMAND_BUFFER
gceSTATUS
_ValidateBuffer(
    IN gcoVGBUFFER Buffer,
    IN gctSTRING CallerName,
    IN gctUINT LineNumber
    )
{
    gceSTATUS status;
    gcsCMDBUFFER_PTR currentBuffer;
    gctUINT32 bufferAlignmentMask;
    gctUINT32 totalSize;
    gctUINT32 currentSize;
    gctUINT queueLength;
    gctUINT bufferCount;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    /* Update the entry counter. */
    _validateCounter++;

    /* Assume success. */
    status = gcvSTATUS_OK;

    /* Reset the total size. */
    totalSize = 0;

    /* Determine the alignment mask. */
    bufferAlignmentMask = Buffer->bufferInfo.addressAlignment - 1;

    gcmCMDBUF_VERBOSE(
        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
        "%s(%d) CURRENT BUFFER STATE (validation #%d)\n",
        __FUNCTION__, __LINE__, _validateCounter
        );

    /* Determine the number of items in the queue. */
    queueLength = (Buffer->queueCurrent == gcvEMPTY_QUEUE)
        ? 0
        : Buffer->queueCurrent - Buffer->queueFirst + 1;

    gcmCMDBUF_VERBOSE(
        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
        "  The queue currently contains %d buffer(s) scheduled.\n",
        queueLength
        );

    if (queueLength)
    {
        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "  Last scheduled buffer is %s @ 0x%08X\n",
            Buffer->queueCurrent->dynamic
                ? "dynamic"
                : "static",
            Buffer->queueCurrent->commandBuffer
            );
    }

    /* Set the first buffer. */
    currentBuffer = Buffer->bufferCurrent;
    gcmQUALIFY_BUFFER(currentBuffer != gcvNULL);

    /* Is the current buffer queued? */
    if (Buffer->queueCurrent->commandBuffer == currentBuffer)
    {
        gcmQUALIFY_BUFFER(currentBuffer->reference == gcvREF_OCCUPIED);
    }

    /* Reset buffer counter. */
    bufferCount = 0;

    do
    {
        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "  Command buffer #%d @ 0x%08X\n",
            bufferCount, currentBuffer
            );

        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "    Reference counter ...................... %d\n",
            currentBuffer->reference
            );

        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "    Container buffer node .................. 0x%08X\n",
            currentBuffer->node
            );

        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "    Hardware address of the buffer ......... 0x%08X\n",
            currentBuffer->address
            );

        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "    Offset of the buffer from the header ... %d\n",
            currentBuffer->bufferOffset
            );

        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "    Buffer size ............................ %d\n",
            currentBuffer->size
            );

        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "    Current offset into the buffer ......... %d\n",
            currentBuffer->offset
            );

        gcmCMDBUF_VERBOSE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "    Data count in the buffer ............... %d\n",
            currentBuffer->dataCount
            );

        gcmQUALIFY_BUFFER(currentBuffer                != gcvNULL);
        gcmQUALIFY_BUFFER(currentBuffer->reference     >= 0);
        gcmQUALIFY_BUFFER(currentBuffer->node          != 0);
        gcmQUALIFY_BUFFER(currentBuffer->address       != 0);
        gcmQUALIFY_BUFFER(currentBuffer->bufferOffset  >= gcmSIZEOF(gcsCMDBUFFER));
        gcmQUALIFY_BUFFER(currentBuffer->size          <= Buffer->bufferMaxSize);
        gcmQUALIFY_BUFFER(currentBuffer->offset        <= currentBuffer->size);
        gcmQUALIFY_BUFFER(currentBuffer->nextAllocated != gcvNULL);
        gcmQUALIFY_BUFFER(currentBuffer->nextSubBuffer == gcvNULL);

        gcmQUALIFY_BUFFER((currentBuffer->address & bufferAlignmentMask) == 0);

        if (currentBuffer->reference == gcvREF_VACANT)
        {
            gcmQUALIFY_BUFFER(currentBuffer->offset == 0);
        }

        currentSize
            = currentBuffer->bufferOffset
            + currentBuffer->size
            + Buffer->bufferInfo.dynamicTailSize;

        totalSize += currentSize;

        gcmQUALIFY_BUFFER(totalSize <= Buffer->bufferRepositorySize);

        currentBuffer = currentBuffer->nextAllocated;

        bufferCount += 1;
    }
    while (currentBuffer != Buffer->bufferCurrent);

    gcmQUALIFY_BUFFER(totalSize - Buffer->bufferRepositorySize == 0);

    /* Success. */
    return gcvSTATUS_OK;
}
#endif

#if gcvENABLE_COMPLETION_DEBUG
static void
_PrintCompletion(
    IN gctSTRING Name,
    IN gctINT Line,
    IN gcoVGBUFFER Buffer,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
#if gcvIGNORE_STATIC_COMPLETIONS
    if (CommandBuffer->node != Buffer->containerNode)
    {
        /* This is not a dynamic buffer, ignore. */
        return;
    }
#endif

    if (CommandBuffer->completion == gcvVACANT_BUFFER)
    {
        gcmBUFFER_TRACE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "%s(%d): buf=0x%08X, completion not assigned\n",
            Name, Line,
            CommandBuffer
            );
    }
    else
    {
        gceSTATUS status;

        status = gcoOS_WaitSignal(
            Buffer->os, CommandBuffer->completion->complete, 0
            );

        gcmBUFFER_TRACE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "%s(%d): buf=0x%08X, compl=0x%08X, sig=0x%08X, ref=%d, completed=%s\n",
            Name, Line,
            CommandBuffer,
            CommandBuffer->completion,
            CommandBuffer->completion->complete,
            CommandBuffer->completion->reference,
            gcmIS_SUCCESS(status)
                ? "yes"
                : "no"
            );
    }
}
#endif

#if gcvENABLE_BUFFER_DEBUG
static void
_PrintBuffers(
    IN gctSTRING Name,
    IN gctINT Line,
    IN gcoVGBUFFER Buffer
    )
{
    gctUINT currentSize, totalSize;
    gcsCMDBUFFER_PTR current;

    totalSize = 0;
    current = Buffer->bufferLast->nextAllocated;

    gcmBUFFER_TRACE(
        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
        "%s(%d): uncommitted=%d, threshold=%d\n",
        Name, Line,
        Buffer->uncommittedSize,
        Buffer->uncommittedThreshold
        );

    while (gcvTRUE)
    {
        if (current == Buffer->bufferLast)
        {
            /* This is approximate. */
            currentSize
                = current->bufferOffset
                + current->size;
        }
        else
        {
            currentSize
                = (gctUINT8_PTR) current->nextAllocated
                - (gctUINT8_PTR) current;
        }

        totalSize  += currentSize;

        gcmBUFFER_TRACE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "  %s wrapper @ 0x%08X, node=0x%08X, address=0x%08X, buffer offset=%d\n",
            (current == Buffer->bufferCurrent)
                ? "-->"
                : "   ",
            current,
            current->node,
            current->address,
            current->bufferOffset
            );

        gcmBUFFER_TRACE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "      size=%d, offset=%d, data count=%d, size with header=%d\n",
            current->size,
            current->offset,
            current->dataCount,
            currentSize
            );

        if (current->completion == gcvVACANT_BUFFER)
        {
            gcmBUFFER_TRACE(
                gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                "        completion not assigned\n"
                );
        }
        else
        {
            gceSTATUS status;

            status = gcoOS_WaitSignal(
                Buffer->os, current->completion->complete, 0
                );

            gcmBUFFER_TRACE(
                gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                "        completion=0x%08X, signal=0x%08X, reference=%d, complete=%s\n",
                current->completion,
                current->completion->complete,
                current->completion->reference,
                gcmIS_SUCCESS(status)
                    ? "yes"
                    : "no"
                );
        }

#if gcvPRINT_BUFFER_CONTENT
        {
            gctUINT i, size;
            gctUINT32_PTR data;

            data = (gctUINT32_PTR) (((gctUINT8_PTR) current) + current->bufferOffset);
            size = current->size + Buffer->bufferInfo.dynamicTailSize;

            for (i = 0; i < size; i += 4)
            {
                if ((i % 32) == 0)
                {
                    gcmBUFFER_TRACE(
                        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                        "\n        %4d:",
                        i / 8 + 1
                        );
                }

                gcmBUFFER_TRACE(
                    gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                    " %08X",
                    * data
                    );

                data += 1;
            }

            if (i != 0)
            {
                gcmBUFFER_TRACE(
                    gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                    "\n\n"
                    );
            }
        }
#endif

        if (current == Buffer->bufferLast)
        {
            break;
        }

        current = current->nextAllocated;
    }

    gcmBUFFER_TRACE(
        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
        "Total buffer size=%d\n",
        totalSize
        );
}
#endif

#if gcdENABLE_TIMEOUT_DETECTION
static gceSTATUS
_BufferSignalTimeoutCallback(
    IN gcoOS Os,
    IN gctPOINTER Context,
    IN gctSIGNAL Signal,
    IN gctBOOL Reset
    )
{
    static gctUINT count;
    static gctUINT printBufferCount;

    /* Reset? */
    if (Reset)
    {
        /* Initizalize the count. */
        count = 0;

        /* Determine count on which the buffer is to be printed. */
        printBufferCount = (Signal == (gctSIGNAL) ~0)
            ? 1
            : 5;
    }
    else
    {
        /* Advance the counter. */
        count += 1;

        /* Print out the buffers only once. */
        if (count == printBufferCount)
        {
            gcoVGBUFFER buffer;

            /* Cast the buffer pointer. */
            buffer = (gcoVGBUFFER) Context;

            /* Print the command buffer state. */
            gcmPRINT_COMMAND_BUFFER(buffer);
        }
    }

    /* Success. */
    return gcvSTATUS_OK;
}
#endif

static gceSTATUS
_AllocateCompletion(
    IN gcoVGBUFFER Buffer,
    OUT gcsCOMPLETION_SIGNAL_PTR * Completion
    )
{
    gceSTATUS status;
    gcsCOMPLETION_SIGNAL_PTR completion;

    do
    {
        /* No available vacant completions? */
        if (Buffer->completionFree == gcvVACANT_BUFFER)
        {
            gctUINT i;
            gcsCOMPLETION_POOL_PTR pool;
            gcsCOMPLETION_SIGNAL_PTR current;

            gcmBUFFER_TRACE(
                gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                "%s: allocating completion pool.\n",
                __FUNCTION__
                );

            /* Allocate a new completion pool. */
            gcmERR_BREAK(gcoOS_Allocate(
                Buffer->os,
                gcvCOMPLETION_POOL_SIZE,
                (gctPOINTER *) &pool
                ));

            /* Link the pool in. */
            pool->next = Buffer->completionPool;
            Buffer->completionPool = pool;

            /* Get the pointer to the first completion. */
            current = (gcsCOMPLETION_SIGNAL_PTR) (pool + 1);

            /* Set the result pointer. */
            completion = current;

            /* First completion will be immediately allocated. */
            current->complete = gcvNULL;
            current += 1;

            /* Initialize all completions. */
            for (i = 0; i < gcvCOMPLETIONS_PER_POOL - 1; i += 1)
            {
                /* Initialize completion. */
                current->complete  = gcvNULL;
                current->reference = 0;

                /* Link in. */
                current->nextFree = Buffer->completionFree;
                Buffer->completionFree = current;

                /* Advance to the next. */
                current += 1;
            }

            /* Update completion counts. */
            Buffer->completionsAllocated += gcvCOMPLETIONS_PER_POOL;
            Buffer->completionsFree      += gcvCOMPLETIONS_PER_POOL - 1;
        }
        else
        {
            gcmBUFFER_TRACE(
                gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                "%s: using next free completion.\n",
                __FUNCTION__
                );

            /* Get the fisrt vacant completion. */
            completion = Buffer->completionFree;

            /* Update the free pointer. */
            Buffer->completionFree = Buffer->completionFree->nextFree;

            /* Update completion count. */
            Buffer->completionsFree -= 1;

            /* Success. */
            status = gcvSTATUS_OK;
        }

        /* Initialize completion. */
        completion->reference = 0;
        completion->nextFree  = gcvNULL;

        /* Create the signal. */
        if (completion->complete == gcvNULL)
        {
            gcmERR_BREAK(gcoOS_CreateSignal(
                Buffer->os,
                gcvTRUE,
                &completion->complete
                ));

            gcmBUFFER_TRACE(
                gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                "%s: created completion signal 0x%08X\n",
                __FUNCTION__,
                completion->complete
                );
        }
        else
        {
            gcmBUFFER_TRACE(
                gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                "%s: completion signal 0x%08X\n",
                __FUNCTION__,
                completion->complete
                );
        }

        /* Reset the signal. */
        gcmERR_BREAK(gcoOS_Signal(
            Buffer->os,
            completion->complete,
            gcvFALSE
            ));

        /* Set the result. */
        * Completion = completion;

        gcmBUFFER_TRACE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "%s: completion=0x%08X\n",
            __FUNCTION__,
            completion
            );
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

static gceSTATUS
_WaitForComplete(
    IN gcoVGBUFFER Buffer,
    IN gcsCOMPLETION_SIGNAL_PTR Completion
    )
{
    gceSTATUS status;

    do
    {
        /* Is the buffer vacant? */
        if (Completion == gcvVACANT_BUFFER)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Make sure the completion is not NULL. */
        gcmASSERT(Completion != gcvNULL);
        /* Wait until complete. */
        gcmERR_BREAK(gcoOS_WaitSignal(
            Buffer->os, Completion->complete, gcvINFINITE
            ));
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

static gceSTATUS
_AllocateTask(
    IN gcoVGBUFFER Buffer,
    IN gceBLOCK Block,
    IN gctUINT TaskCount,
    IN gctSIZE_T Bytes,
    OUT gcsTASK_PTR * Task
    )
{
    gceSTATUS status = gcvSTATUS_OK;
#ifdef __QNXNTO__
    gctPHYS_ADDR physical;
    gctSIZE_T objSize;
#endif

    do
    {
        gctUINT size;
        gcsTASK_STORAGE_PTR next;
        gcsTASK_MASTER_ENTRY_PTR tableEntry;
        gcsTASK_PTR task;

        /* Determine the number of bytes needed. */
        size = gcmSIZEOF(gcsTASK) + Bytes;

        /* Cannot be greater then the maximum buffer size. */
        gcmASSERT(size <= Buffer->taskStorageMaxSize);

        /* Is there enough space in the current container? */
        if (size > Buffer->taskStorageCurrAvailable)
        {
            /* Get the pointer to the next container. */
            next = Buffer->taskStorageCurr->next;

            /* Is there another container already allocated after this one? */
            if (next == gcvNULL)
            {
                /* Increment the queue overflow counter. */
                Buffer->taskOverflow += 1;

                /* No, allocate another container. */
#ifndef __QNXNTO__
                gcmERR_BREAK(gcoOS_Allocate(
                    Buffer->os,
                    Buffer->taskStorageGranularity,
                    (gctPOINTER *) &next
                    ));
#else
                objSize = (gctSIZE_T) Buffer->taskStorageGranularity;
                gcmERR_BREAK(gcoOS_AllocateNonPagedMemory(
                    gcvNULL, gcvTRUE, &objSize, &physical, (gctPOINTER *) &next
                    ));
#endif

                /* Terminate the new container. */
                next->next = gcvNULL;

                /* Add to the container chain. */
                Buffer->taskStorageCurr->next = next;
                Buffer->taskStorageCurr       = next;
            }
            else
            {
                /* Yes, move to the next buffer. */
                Buffer->taskStorageCurr = next;
            }

            /* Set the current pointer. */
            Buffer->taskStorageCurrPtr
                = (gctUINT8_PTR) (Buffer->taskStorageCurr + 1);

            /* Set the size of available buffer. */
            Buffer->taskStorageCurrAvailable = Buffer->taskStorageMaxSize;
        }

        /* Must have enough space at this point. */
        gcmASSERT(size <= Buffer->taskStorageCurrAvailable);

        /* Reserve the space. */
        task = (gcsTASK_PTR) Buffer->taskStorageCurrPtr;
        Buffer->taskStorageCurrPtr       += size;
        Buffer->taskStorageCurrAvailable -= size;

        /* Get the proper table entry. */
#ifndef __QNXNTO__
        tableEntry = &Buffer->taskTable.table[Block];
#else
        tableEntry = &Buffer->pTaskTable->table[Block];
#endif

        /* Initialize the task. */
        task->next = gcvNULL;
        task->size = Bytes;

        /* Add the task to the table. */
        if (tableEntry->head == gcvNULL)
        {
            tableEntry->head = task;
            tableEntry->tail = task;

            /* Update the size of the task entry. */
            Bytes += gcmSIZEOF(gcsTASK_LINK);

            /* Update the task block count. */
            Buffer->taskBlockCount += 1;
        }
        else
        {
            tableEntry->tail->next = task;
            tableEntry->tail       = task;
        }

        /* Update the total task count. */
#ifndef __QNXNTO__
        Buffer->taskTable.count += TaskCount;
        Buffer->taskTable.size  += Bytes;
#else
        Buffer->pTaskTable->count += TaskCount;
        Buffer->pTaskTable->size  += Bytes;
#endif

        /* Set the result pointer. */
        (* Task) = task;
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

static gceSTATUS
_FinalizeRestartMarks(
    IN gcoVGBUFFER Buffer,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctPOINTER current, restart;
    gctUINT address, count;
    gctUINT offset;
    gctSIZE_T bytes;

    /* Set initial restart location. */
    restart = Buffer->restart;

    /* Process all pending restarts. */
    while (restart != gcvNULL)
    {
        /* Set the current and advance to the next. */
        current = restart;
        restart = * (gctPOINTER *) restart;

        /* Determine the offset of the next command. */
        offset
            = (gctUINT8_PTR) current
            - (gctUINT8_PTR) CommandBuffer
            - CommandBuffer->bufferOffset;

        /* Determine the restart address. */
        address = CommandBuffer->address + offset;

        /* Determine the restart data count. */
        count
            = CommandBuffer->dataCount
            - offset / Buffer->bufferInfo.commandAlignment;

        /* Set the size of the reserved space. */
        bytes = Buffer->bufferInfo.restartCommandSize;

        /* Write the restart BEGIN command. */
        gcmERR_BREAK(gcoVGHARDWARE_RestartCommand(
            Buffer->hardware, current, address, count, &bytes
            ));
    }

    /* Reset the list. */
    Buffer->restart = gcvNULL;

    /* Return status. */
    return status;
}

static gcmINLINE void
_InitializeBuffer(
    IN gcoVGBUFFER Buffer,
    IN gcsCMDBUFFER_PTR CommandBuffer,
    IN gctUINT32 CommandBufferAddress
    )
{
    gctUINT bufferAddress;
    gctUINT bufferOffset;

    /* Determine the command buffer address. */
    bufferAddress = gcmALIGN(
        CommandBufferAddress + gcmSIZEOF(gcsCMDBUFFER),
        Buffer->bufferInfo.addressAlignment
        );

    /* Determine the command buffer offset. */
    bufferOffset = bufferAddress - CommandBufferAddress;

    /* Reset the completion signal. */
    CommandBuffer->completion = gcvVACANT_BUFFER;

    /* Set the memory pool node. */
    CommandBuffer->node = Buffer->containerNode;

    /* Set the buffer hardware address and offset. */
    CommandBuffer->address      = bufferAddress;
    CommandBuffer->bufferOffset = bufferOffset;

    /* Reset the offset. */
    CommandBuffer->offset = 0;

    /* Reset the subbuffer. */
    CommandBuffer->nextSubBuffer = gcvNULL;
}

static gcsCMDBUFFER_PTR
_TerminateCommandBuffer(
    IN gcoVGBUFFER Buffer,
    IN gcsCMDBUFFER_PTR CommandBuffer,
    IN gctSIZE_T EffectiveSize
    )
{
    gcsCMDBUFFER_PTR newBuffer;
    gctUINT32 newBufferAddress;

    /* Set new size. */
    CommandBuffer->size = CommandBuffer->offset;

    /* Determine the placement of the next buffer header. */
    newBuffer = (gcsCMDBUFFER_PTR)
    (
        (gctUINT8_PTR) CommandBuffer
            + CommandBuffer->bufferOffset
            + EffectiveSize
    );

    /* Determine the hardware address of the next buffer header. */
    newBufferAddress = CommandBuffer->address + EffectiveSize;

    /* Initiualize the new buffer. */
    _InitializeBuffer(Buffer, newBuffer, newBufferAddress);

    /* Return the pointer to the new buffer. */
    return newBuffer;
}

static gceSTATUS
_SplitCurrent(
    IN gcoVGBUFFER Buffer,
    IN gctBOOL EnsureVacant,
    IN gctUINT EventCount
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gctUINT8_PTR unalignedData;
    gcsCMDBUFFER_PTR current;
    gcsCMDBUFFER_PTR next;
    gctUINT currentOffset;
    gctSIZE_T effectiveSize;
    gctSIZE_T executionSize;

    gcsCMDBUFFER_PTR newBuffer;
    gctUINT newBufferOffset;
    gctUINT nextBufferOffset;
    gctINT splitoffSize;

    /* Validate the command buffer. */
    vgmVALIDATE_BUFFER(Buffer);

    do
    {
        /* Get the current command buffer. */
        current = Buffer->bufferCurrent;

        /* Determine the current unaligned data pointer. */
        unalignedData
            = ((gctUINT8_PTR) current)
            + current->bufferOffset
            + current->offset;

        /* Aling the buffer offset. */
        current->offset = gcmALIGN(
            current->offset, Buffer->bufferInfo.commandAlignment
            );

        /* Get a shortcut to the current offset. */
        currentOffset = current->offset;

        /* The offset can only be less then the size. */
        gcmASSERT(currentOffset <= current->size);

        /* Determine the size of the data to be executed by the hardware. */
        executionSize
            = currentOffset
            + Buffer->bufferInfo.staticTailSize
            + Buffer->bufferInfo.eventCommandSize * EventCount;

        /* Determine the effective command buffer size. */
        effectiveSize
            = currentOffset
            + Buffer->bufferInfo.dynamicTailSize;

        /* Determine the data count. */
        current->dataCount
            = executionSize / Buffer->bufferInfo.commandAlignment;

        /* Finalize restart marks. */
        _FinalizeRestartMarks(Buffer, current);

        /* Determine the offset of the new buffer. */
        newBufferOffset = gcmALIGN(
            effectiveSize + gcmSIZEOF(gcsCMDBUFFER),
            Buffer->bufferInfo.addressAlignment
            );

        /* If this is not the last buffer and the next buffer is vacant, merge
           the splitoff area with the next buffer and move to the new merged
           buffer. */
        if (current != Buffer->bufferLast)
        {
            /* Get a pointer to the next command buffer. */
            next = current->nextAllocated;

            /* Is the next buffer vacant or complete? */
            if (gcmIS_SUCCESS(gcoVGBUFFER_CheckCompletion(Buffer, next)))
            {
                /* Reset offset. */
                next->offset = 0;

                /* Determine the offset of the next buffer from the current one. */
                nextBufferOffset
                    = (((gctUINT8_PTR) next)    + next->bufferOffset)
                    - (((gctUINT8_PTR) current) + current->bufferOffset);

                /* Determine the delta. */
                splitoffSize = nextBufferOffset - newBufferOffset;

                /* Merge only if the next buffer will grow. */
                if (splitoffSize > 0)
                {
                    /* Determine the new buffer size. */
                    gctSIZE_T newBufferSize = splitoffSize + next->size;

                    /* Get next in link. */
                    gcsCMDBUFFER_PTR nextAllocated = next->nextAllocated;

                    /* Release completion signal from the next buffer. */
                    gcmERR_BREAK(gcoVGBUFFER_DeassociateCompletion(Buffer, next));

                    /* Terminate the command buffer and create a new one from
                       the remaining part of it. */
                    newBuffer = _TerminateCommandBuffer(
                        Buffer, current, effectiveSize
                        );

                    /* Determine the size of the new buffer. */
                    newBuffer->size = newBufferSize;

                    /* Update the last buffer if necessary. */
                    if (next == Buffer->bufferLast)
                    {
                        Buffer->bufferLast = newBuffer;
                    }

                    /* Link in. */
                    newBuffer->nextAllocated = nextAllocated;
                    current->nextAllocated   = newBuffer;

                    /* Update the current command buffer pointer. */
                    Buffer->bufferCurrent = newBuffer;

                    /* Account for the tail and the header. */
                    Buffer->uncommittedSize
                        += (((gctUINT8_PTR) newBuffer) + newBuffer->bufferOffset)
                         - unalignedData;

                    /* Success. */
                    status = gcvSTATUS_OK;
                    break;
                }
            }
        }

        /* Determine the splitoff buffer size. */
        splitoffSize = current->size - newBufferOffset;

        /* Cannot merge with the next buffer, see if the splitoff area is big
           enough to become a buffer on its own. */
        if (splitoffSize >= (gctINT) Buffer->bufferMinSize)
        {
            /* Terminate the command buffer and create a new one from
               the remaining part of it. */
            newBuffer = _TerminateCommandBuffer(Buffer, current, effectiveSize);

            /* Set the size of the new buffer. */
            newBuffer->size = splitoffSize;

            /* Link in. */
            newBuffer->nextAllocated = current->nextAllocated;
            current->nextAllocated   = newBuffer;

            /* Update the last buffer if necessary. */
            if (current == Buffer->bufferLast)
            {
                Buffer->bufferLast = newBuffer;
            }

            /* Update the current command buffer pointer. */
            Buffer->bufferCurrent = newBuffer;

            /* Account for the tail and the header. */
            Buffer->uncommittedSize
                += (((gctUINT8_PTR) newBuffer) + newBuffer->bufferOffset)
                 - unalignedData;

            /* Success. */
            status = gcvSTATUS_OK;
            break;
        }

        /* We are here because of one of the following:
           - The current buffer is completely full so there is nothing
             to split off of it;
           - The current buffer is not the last one, but the next buffer is
             occupied, so we cannot merge the splitoff area with the next
             buffer;
           - The splitoff area is not big enough to become a buffer on its own.
           We have the following options:
           - If the next buffer is not occupied, move the current buffer to it;
           - If the next buffer is occupied, but not yet executed we need to
             commit the current queue. The commit routine will automatically
             move to the next buffer;
           - If the next buffer is busy, we need to wait until it becomes
             unoccupied. */

        /* Get a pointer to the next command buffer. */
        next = current->nextAllocated;

        /* Make sure the next buffer is vacant if requested. */
        if (EnsureVacant)
        {
            /* Wait until the buffer is executed. */
            gcmERR_BREAK(_WaitForComplete(Buffer, next->completion));

            /* Reset offset. */
            next->offset = 0;

        }

        /* Advance to the next buffer. */
        Buffer->bufferCurrent = newBuffer = next;

        /* Account for the tail and the header. */
        Buffer->uncommittedSize
            += (((gctUINT8_PTR) newBuffer) + newBuffer->bufferOffset)
             - unalignedData;
    }
    while (gcvFALSE);

    /* Validate the command buffer. */
    vgmVALIDATE_BUFFER(Buffer);

    /* Return status. */
    return status;
}

static gceSTATUS
_InitEmptyQueue(
    IN gcoVGBUFFER Buffer
    )
{
    gceSTATUS status;

    do
    {
        gcsCMDBUFFER_PTR contextBuffer;
        gcsTASK_SIGNAL_PTR completion;

        /* Is the queue empty? */
        if (Buffer->queueCurrent != gcvEMPTY_QUEUE)
        {
            /* No, nothing to do. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Initialize the first queue entry. */
        Buffer->queueCurrent = Buffer->queueFirst;

        /* Context buffer shortcut. */
        contextBuffer = Buffer->context->header;

        /* No current completion have been assigned yet? */
        if (contextBuffer->completion == gcvVACANT_BUFFER)
        {
            /* Allocate a completion. */
            gcmERR_BREAK(_AllocateCompletion(
                Buffer, &contextBuffer->completion
                ));

            /* Set completion signal handle. */
            Buffer->context->process = Buffer->processID;
            Buffer->context->signal = contextBuffer->completion->complete;

            /* Init reference count. */
            contextBuffer->completion->reference = 1;
        }

        /* The first entry is always the context buffer. */
        Buffer->queueCurrent->commandBuffer = contextBuffer;
        Buffer->queueCurrent->dynamic       = gcvFALSE;

        /* Add a completion signal task. */
        gcmERR_BREAK(gcoVGBUFFER_ReserveTask(
            Buffer,
            gcvBLOCK_PIXEL,
            1, gcmSIZEOF(gcsTASK_SIGNAL),
            (gctPOINTER *) &completion
            ));

        /* Fill in event info. */
        completion->id      = gcvTASK_SIGNAL;
        completion->process = Buffer->processID;
        completion->signal  = contextBuffer->completion->complete;
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

static gceSTATUS
_GetNextQueueEntry(
    IN gcoVGBUFFER Buffer,
    OUT gcsVGCMDQUEUE_PTR * Entry
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        /* If the last queued buffer is the one currently being accumulated
           by gcoVGBUFFER, we have to split it in two leaving the already
           accumulated part queued in front of the new queue entry. */
        if (Buffer->queueCurrent->commandBuffer == Buffer->bufferCurrent)
        {
            /* Current buffer has to have a completion associated. */
            gcmASSERT(Buffer->bufferCurrent->completion != gcvVACANT_BUFFER);

            /* Split it off and advance to the next. */
            gcmERR_BREAK(_SplitCurrent(Buffer, gcvTRUE, 0));
        }

        /* Is the queue full? */
        if (Buffer->queueCurrent == Buffer->queueLast)
        {
            /* Increment the queue overflow counter. */
            Buffer->queueOverflow += 1;

            /* Commit the current queue. */
            gcmERR_BREAK(gcoVGHARDWARE_Commit(Buffer->hardware, gcvFALSE));
        }

        /* Perform queue initialization if empty. */
        _InitEmptyQueue(Buffer);

        /* Advance to the next vacant entry. */
        Buffer->queueCurrent += 1;

        /* Set the result. */
        * Entry = Buffer->queueCurrent;
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

static gceSTATUS
_CheckUncommittedThreshold(
    IN gcoVGBUFFER Buffer
    )
{
    gceSTATUS status;

    /* Did we allocate over the threshold? */
    if (Buffer->uncommittedSize > Buffer->uncommittedThreshold)
    {

        /* Commit the queue. */
        status = gcoVGHARDWARE_Commit(Buffer->hardware, gcvFALSE);

        /* Error? */
        if (gcmIS_ERROR(status))
        {
            /* Report current location. */

        }
        else
        {
            /* Report current location. */

        }
    }
    else
    {
        status = gcvSTATUS_OK;
    }

    /* Return status. */
    return status;
}


/******************************************************************************\
******************************* gcoVGBUFFER API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoVGBUFFER_Construct
**
**  Construct a new gcoVGBUFFER object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gcsVGCONTEXT_PTR Context
**          Pointer to the context structure.
**
**      gctSIZE_T CommandBufferSize
**          The size of the command buffer container.
**
**      gctUINT TaskBufferGranulatiry
**          The size of the task buffer.
**
**      gctUINT QueueEntryCount
**          The length of the command queue.
**
**  OUTPUT:
**
**      gcoVGBUFFER * Buffer
**          Pointer to a variable that will hold the the gcoVGBUFFER object
**          pointer.
*/

gceSTATUS
gcoVGBUFFER_Construct(
    IN gcoHAL Hal,
    IN gcoVGHARDWARE Hardware,
    IN gcsVGCONTEXT_PTR Context,
    IN gctSIZE_T CommandBufferSize,
    IN gctUINT TaskBufferGranulatiry,
    IN gctUINT QueueEntryCount,
    OUT gcoVGBUFFER * Buffer
    )
{
    gceSTATUS status, last;
    gcoVGBUFFER buffer = gcvNULL;
#ifdef __QNXNTO__
    gctPHYS_ADDR physical;
    gctSIZE_T objSize = 0;
#endif

    gcmHEADER_ARG("HAL=0x%x Hardware=0x%x", Hal, Hardware);

    gcmBUFFER_TRACE(
        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
        "[ENTER] %s: CommandBufferSize=%u, TaskBufferGranulatiry=%u, QueueEntryCount=%u\n",
        __FUNCTION__, CommandBufferSize, TaskBufferGranulatiry, QueueEntryCount
        );


    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hal, gcvOBJ_HAL);
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Buffer != gcvNULL);

    do
    {
        /* Container buffer parameters. */
        gctUINT32 currentAddress;
        gctUINT8_PTR currentLogical;

        /* Command buffer repository. */
        gcsCMDBUFFER_PTR currentBuffer;
        gctUINT initialBufferSize;

        /* Command queue. */
        gctUINT queueSize;


        /*--------------------------------------------------------------------*/
        /*----------------------- Generic Initialization ---------------------*/

        /* Allocate the gcoVGBUFFER object. */
#ifndef __QNXNTO__
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            gcmSIZEOF(struct _gcoVGBUFFER),
            (gctPOINTER *) &buffer
            ));
#else
        objSize = gcmSIZEOF(struct _gcoVGBUFFER);
        gcmERR_BREAK(gcoOS_AllocateNonPagedMemory(
            gcvNULL, gcvTRUE, &objSize, &physical,
            (gctPOINTER *) &buffer
            ));
#endif

        /* Initialize the gcoVGBUFFER object. */
        buffer->object.type = gcvOBJ_BUFFER;
        buffer->os          = gcvNULL;
        buffer->hal         = Hal;
        buffer->hardware    = Hardware;
        buffer->context     = Context;

        /* Reset the pointers. */
        buffer->containerNode   = gcvNULL;
        buffer->taskStorageHead = gcvNULL;
        buffer->queueFirst      = gcvNULL;

        /* Reset completion signals. */
        buffer->completionsAllocated = 0;
        buffer->completionsFree      = 0;
        buffer->completionPool       = gcvNULL;
        buffer->completionFree       = gcvVACANT_BUFFER;
        buffer->completionCurrent    = gcvVACANT_BUFFER;
        buffer->completionPrevious   = gcvVACANT_BUFFER;

        /* Reset the performance counters. */
        buffer->bufferOverflow = 0;
        buffer->taskOverflow   = 0;
        buffer->queueOverflow  = 0;
        buffer->commitCount    = 0;
        buffer->reserveCount   = 0;

        /* Reset restart states. */
        buffer->restart = gcvNULL;

        /* Determine whether CALL/RETURN is available. */
        buffer->useCallReturn = gcoVGHARDWARE_IsFeatureAvailable(
            Hardware, gcvFEATURE_VG21
            );

        /* Query command buffer attributes. */
        gcmERR_BREAK(gcoVGHARDWARE_QueryCommandBuffer(
            Hardware, &buffer->bufferInfo
            ));

        /* Get the current process ID. */
        buffer->processID = gcoOS_GetCurrentProcessID();

        /*--------------------------------------------------------------------*/
        /*------------------- Allocate one container buffer. -----------------*/

        /* Align the command buffer size. */
        CommandBufferSize = gcmALIGN(CommandBufferSize, 4096);

        /* Allocate the container buffer. */
        gcmERR_BREAK(gcoVGHARDWARE_AllocateLinearVideoMemory(
            Hardware,
            CommandBufferSize,
            buffer->bufferInfo.addressAlignment,
            gcvPOOL_SYSTEM,
            &buffer->containerNode,
            &currentAddress,
            (gctPOINTER *) &currentLogical
            ));


        /*--------------------------------------------------------------------*/
        /*------------- Initialize the command buffer repository. ------------*/

        /* Cast the buffer pointer. */
        currentBuffer = (gcsCMDBUFFER_PTR) currentLogical;

        /* Initialize the command buffer pointers. */
        buffer->bufferCurrent = buffer->bufferLast = currentBuffer;

        /* Initialize the command buffer. */
        _InitializeBuffer(buffer, currentBuffer, currentAddress);

        /* Determine the size of the aligned buffer with the tail. */
        initialBufferSize = gcmALIGNDOWN(
            CommandBufferSize - currentBuffer->bufferOffset,
            buffer->bufferInfo.commandAlignment
            );

        /* Determine the repository size. */
        buffer->bufferRepositorySize
            = currentBuffer->bufferOffset + initialBufferSize;

        /* Subtract the tail. */
        initialBufferSize -= buffer->bufferInfo.dynamicTailSize;

        /* Complete buffer initialization. */
        currentBuffer->size = initialBufferSize;
        currentBuffer->nextAllocated = currentBuffer;

        /* The buffer limit is the point beyond which there may not be any
           space reserved, except for buffer tail data. */
        buffer->bufferLimit = currentLogical + buffer->bufferRepositorySize;

        /* Initialize the uncommitted data threshold counters. */
        buffer->uncommittedSize = 0;
        buffer->uncommittedThreshold = initialBufferSize / 2;

        /* Store the maximum buffer size. */
        buffer->bufferMaxSize = initialBufferSize;

        /* Let the buffer have at least 10 commands. */
        buffer->bufferMinSize = buffer->bufferInfo.commandAlignment * 10;

        /*--------------------------------------------------------------------*/
        /*-------------------- Initialize task management. -------------------*/

#ifdef __QNXNTO__
        objSize = gcmSIZEOF(*buffer->pTaskTable);
        gcmERR_BREAK(gcoOS_AllocateNonPagedMemory(
            gcvNULL, gcvTRUE, &objSize, &physical, (gctPOINTER *) &buffer->pTaskTable));
#endif

        /* Store the storage granularity. */
        buffer->taskStorageGranularity = TaskBufferGranulatiry;

        /* Determine the maximum availble size of the container. */
        buffer->taskStorageMaxSize
            = buffer->taskStorageGranularity - gcmSIZEOF(gcsTASK_STORAGE);

        /* Allocate the storage. */
#ifndef __QNXNTO__
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            buffer->taskStorageGranularity,
            (gctPOINTER *) &buffer->taskStorageHead
            ));
#else
        objSize = (gctSIZE_T)buffer->taskStorageGranularity;
        gcmERR_BREAK(gcoOS_AllocateNonPagedMemory(
            gcvNULL, gcvTRUE, &objSize, &physical,
            (gctPOINTER *) &buffer->taskStorageHead
            ));
#endif

        /* Initialize the current storage. */
        buffer->taskStorageCurr       = buffer->taskStorageHead;
        buffer->taskStorageCurr->next = gcvNULL;

        /* Set the initial storage size. */
        buffer->taskStorageCurrAvailable = buffer->taskStorageMaxSize;

        /* Set the current pointer. */
        buffer->taskStorageCurrPtr
            = (gctUINT8_PTR) (buffer->taskStorageCurr + 1);

        /* Reset the task master tables. */
#ifndef __QNXNTO__
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            &buffer->taskTable, gcmSIZEOF(buffer->taskTable)
            ));
#else
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            buffer->pTaskTable, gcmSIZEOF(*buffer->pTaskTable)
            ));
#endif

        /* Reset the number of entasked blocks. */
        buffer->taskBlockCount = 0;

        /*--------------------------------------------------------------------*/
        /*-------------------- Allocate the command queue. -------------------*/

        /* Determine the size of the command queue. */
        queueSize = QueueEntryCount * gcmSIZEOF(gcsVGCMDQUEUE);

        /* Allocate the command queue. */
#ifndef __QNXNTO__
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            queueSize,
            (gctPOINTER *) &buffer->queueFirst
            ));
#else
        objSize = (gctSIZE_T)queueSize;
        gcmERR_BREAK(gcoOS_AllocateNonPagedMemory(
            gcvNULL, gcvTRUE, &objSize, &physical, (gctPOINTER *) &buffer->queueFirst));
#endif

        /* The queue is initially empty. */
        buffer->queueCurrent = gcvEMPTY_QUEUE;

        /* Determine the last queue entry. The last committed queued buffer
           always has to be dynamic; reserve one entry in the queue to be
           always able to add an entry at the end if the last committed
           buffer is not dynamic. */
        buffer->queueLast = buffer->queueFirst + QueueEntryCount - 1 - 1;

        /*--------------------------------------------------------------------*/
        /*----------------------- Setup timeout callback. ---------------------*/

#if gcdENABLE_TIMEOUT_DETECTION
        /* Setup the buffer timeout handler. */
        gcmVERIFY_OK(gcoOS_InstallTimeoutCallback(
            _BufferSignalTimeoutCallback, buffer
            ));
#endif

        /*--------------------------------------------------------------------*/
        /*--------------------------- Return result. -------------------------*/

        /* Validate the command buffer. */
        vgmVALIDATE_BUFFER(buffer);

        /* Return pointer to the ABUFFER object. */
        *Buffer = buffer;

        gcmBUFFER_TRACE(
            gcvLEVEL_INFO, gcvZONE_BUFFER,
            "[LEAVE] %s: Created gcoVGBUFFER at %p\n",
            __FUNCTION__, buffer
            );

        gcmFOOTER_NO();
        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmBUFFER_TRACE(
        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
        "[FAILED] %s: %d\n",
        __FUNCTION__, status
        );

    /* Roll back. */
    if (buffer != gcvNULL)
    {
        /* Free the command queue. */
        if (buffer->queueFirst != gcvNULL)
        {
#ifndef __QNXNTO__
            gcmCHECK_STATUS(gcoOS_Free(
                gcvNULL, buffer->queueFirst
                ));
#else
            gcmCHECK_STATUS(gcoOS_FreeNonPagedMemory(
                gcvNULL, objSize, gcvNULL, buffer->queueFirst));
#endif
        }

        /* Free the task storage. */
        if (buffer->taskStorageHead != gcvNULL)
        {
#ifndef __QNXNTO__
            gcmCHECK_STATUS(gcoOS_Free(
                gcvNULL, buffer->taskStorageHead
                ));
#else
            gcmCHECK_STATUS(gcoOS_FreeNonPagedMemory(
                gcvNULL, (gctSIZE_T)buffer->taskStorageGranularity, gcvNULL, buffer->taskStorageHead
                ));
#endif
        }

        /* Free the command buffer container. */
        if (buffer->containerNode != gcvNULL)
        {
            gcmCHECK_STATUS(gcoHAL_FreeVideoMemory(
                Hal, buffer->containerNode
                ));
        }

        /* Free the command buffer object. */
#ifndef __QNXNTO__
        gcmCHECK_STATUS(gcoOS_Free(
            gcvNULL, buffer
            ));
#else
        gcmCHECK_STATUS(gcoOS_FreeNonPagedMemory(
            gcvNULL, gcmSIZEOF(struct _gcoVGBUFFER), gcvNULL, buffer
            ));
#endif
    }

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_Destroy
**
**  Destroy an gcoVGBUFFER object.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to an gcoVGBUFFER object to delete.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_Destroy(
    IN gcoVGBUFFER Buffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x",
                  Buffer);

    gcmBUFFER_TRACE(
        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
        "[ENTER] %s: Buffer=%p\n",
        __FUNCTION__, Buffer
        );


    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    do
    {
        gcsCMDBUFFER_PTR currentBuffer;

        /* Commit the command queue and stall. */
        gcmERR_BREAK(gcoVGHARDWARE_Commit(
            Buffer->hardware, gcvTRUE
            ));

        /* Dereference the context buffer completion. */
        gcmERR_BREAK(gcoVGBUFFER_DeassociateCompletion(
            Buffer, Buffer->context->header
            ));

        /* Get the current buffer. */
        currentBuffer = Buffer->bufferCurrent;

        /* Release all associated completions. */
        do
        {
            /* Release the completion from the current buffer. */
            gcmERR_BREAK(gcoVGBUFFER_DeassociateCompletion(Buffer, currentBuffer));

            /* Advance to the next buffer. */
            currentBuffer = currentBuffer->nextAllocated;
        }
        while (currentBuffer != Buffer->bufferCurrent);

        /* Error? */
        if (gcmIS_ERROR(status))
        {
            break;
        }

        /* Free completions. */
        gcmERR_BREAK(gcoVGBUFFER_FreeCompletions(Buffer));

        /* Free the task storage. */
        if (Buffer->taskStorageHead != gcvNULL)
        {
            gcsTASK_STORAGE_PTR next;

            /* Remove all buffers. */
            do
            {
                /* Get the next buffer. */
                next = Buffer->taskStorageHead->next;

                /* Free current. */
#ifndef __QNXNTO__
                gcmERR_BREAK(gcoOS_Free(
                    Buffer->os, Buffer->taskStorageHead
                    ));
#else
                gcmERR_BREAK(gcoOS_FreeNonPagedMemory(
                    gcvNULL, (gctSIZE_T)Buffer->taskStorageGranularity, gcvNULL, Buffer->taskStorageHead
                    ));
#endif

                /* The next becomes the new head. */
                Buffer->taskStorageHead = next;
            }
            while (next != gcvNULL);

            /* Error? */
            if (gcmIS_ERROR(status))
            {
                break;
            }
        }

        /* Free the command queue. */
        if (Buffer->queueFirst != gcvNULL)
        {
#ifndef __QNXNTO__
            gcmERR_BREAK(gcoOS_Free(
                Buffer->os, Buffer->queueFirst
                ));
#else
            gctSIZE_T objSize = Buffer->queueLast - Buffer->queueFirst + 2;
            gcmERR_BREAK(gcoOS_FreeNonPagedMemory(
                gcvNULL, objSize, gcvNULL, Buffer->queueFirst));
#endif

            /* Reset the queue. */
            Buffer->queueFirst = gcvNULL;
        }

        /* Free the command buffer container. */
        if (Buffer->containerNode != gcvNULL)
        {
            gcmERR_BREAK(gcoHAL_FreeVideoMemory(
                Buffer->hal, Buffer->containerNode
                ));

            /* Reset the node. */
            Buffer->containerNode = gcvNULL;
        }

#if gcdENABLE_TIMEOUT_DETECTION
        /* Remove the default timeout handler. */
        gcmVERIFY_OK(gcoOS_RemoveTimeoutCallback(
            _BufferSignalTimeoutCallback
            ));
#endif

#ifdef __QNXNTO__
        gcmERR_BREAK(gcoOS_FreeNonPagedMemory(
            gcvNULL, gcmSIZEOF(*Buffer->pTaskTable), gcvNULL, Buffer->pTaskTable
            ));
#endif

        /* Mark gcoVGBUFFER object as unknown. */
        Buffer->object.type = gcvOBJ_UNKNOWN;

        /* Free the gcoVGBUFFER object. */
#ifndef __QNXNTO__
        gcmERR_BREAK(gcoOS_Free(Buffer->os, Buffer));
#else
        gcmERR_BREAK(gcoOS_FreeNonPagedMemory(
            gcvNULL, gcmSIZEOF(struct _gcoVGBUFFER), gcvNULL, Buffer
            ));
#endif
    }
    while (gcvFALSE);

    gcmBUFFER_TRACE(
        gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
        "[LEAVE] %s: %d\n",
        __FUNCTION__, status
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGBUFFER_FreeCompletions
**
**  Free all memory associated with allocated completion signals.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to the gcoVGBUFFER object.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_FreeCompletions(
    IN gcoVGBUFFER Buffer
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Buffer=0x%x",
                  Buffer);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    do
    {
        gcsCOMPLETION_SIGNAL_PTR completion;

        /* Is there a pool allocated? */
        if (Buffer->completionPool == gcvNULL)
        {
            /* No, no pool have been allocated. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Have all completions been released? */
        if (Buffer->completionsAllocated != Buffer->completionsFree)
        {
            gcmFATAL(
                "%s(%d): Not all completion signals have been released!\n",
                __FUNCTION__, __LINE__
                );

            status = gcvSTATUS_GENERIC_IO;
            break;
        }

        /* Loop through completions. */
        completion = Buffer->completionFree;

        while (completion != gcvVACANT_BUFFER)
        {
            /* Validate completion. */
            gcmASSERT(completion != gcvNULL);
            gcmASSERT(completion->reference == 0);

            /* Is there a signal attached? */
            if (completion->complete != gcvNULL)
            {
                /* Destory the signal. */
                gcmERR_BREAK(gcoOS_DestroySignal(
                    Buffer->os, completion->complete
                    ));

                /* Reset the signal. */
                completion->complete = gcvNULL;
            }

            /* Advance to the next completion. */
            completion = completion->nextFree;
        }

        /* Error? */
        if (gcmIS_ERROR(status))
        {
            break;
        }

        /* Loop through pools. */
        while (Buffer->completionPool != gcvNULL)
        {
            /* Get the current pool. */
            gcsCOMPLETION_POOL_PTR current = Buffer->completionPool;

            /* Advance to the next one. */
            Buffer->completionPool = current->next;

            /* Free the pool. */
            gcmERR_BREAK(gcoOS_Free(Buffer->os, current));
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGBUFFER_AssociateCompletion
**
**  Allocate if necessary and associate a completion signal with the specified
**  command buffer. If the command buffer is already associated with another
**  completion, it will be deassociated first.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to the gcoVGBUFFER object.
**
**      gcsCMDBUFFER_PTR CommandBuffer
**          Pointer to the command buffer.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_AssociateCompletion(
    IN gcoVGBUFFER Buffer,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x CommandBuffer=0x%x",
                  Buffer, CommandBuffer);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    do
    {
        /* Dereference the current completion if any. */
        gcmERR_BREAK(gcoVGBUFFER_DeassociateCompletion(Buffer, CommandBuffer));

        /* No current completion have been assigned yet? */
        if (Buffer->completionCurrent == gcvVACANT_BUFFER)
        {
            gcmBUFFER_TRACE(
                gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                "%s: assigning the current completion.\n",
                __FUNCTION__
                );

            /* Allocate a completion. */
            gcmERR_BREAK(_AllocateCompletion(
                Buffer, &Buffer->completionCurrent
                ));
        }

        /* Increment the reference counter. */
        Buffer->completionCurrent->reference += 1;

        /* Set the new completion. */
        CommandBuffer->completion = Buffer->completionCurrent;

        /* Report completion. */
        gcmPRINT_COMPLETION(Buffer, CommandBuffer);
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGBUFFER_DeassociateCompletion
**
**  If the specified command buffer has a completion associated with it,
**  it will be released.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to the gcoVGBUFFER object.
**
**      gcsCMDBUFFER_PTR CommandBuffer
**          Pointer to the command buffer.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_DeassociateCompletion(
    IN gcoVGBUFFER Buffer,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gcmHEADER_ARG("Buffer=0x%x CommandBuffer=0x%x",
                  Buffer, CommandBuffer);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    do
    {
        gcsCOMPLETION_SIGNAL_PTR completion;

        /* Get a shortcut to the completion. */
        completion = CommandBuffer->completion;

        /* Is there a completion associated? */
        if (completion == gcvVACANT_BUFFER)
        {
            break;
        }

        /* Decrement the reference counter. */
        gcmASSERT(completion != gcvNULL);
        gcmASSERT(completion->reference > 0);
        completion->reference -= 1;

        /* Last reference being deleted? */
        if (completion->reference == 0)
        {
            /* Reset pointers as necessary. */
            if (completion == Buffer->completionPrevious)
            {
                Buffer->completionPrevious = gcvVACANT_BUFFER;
            }

            if (completion == Buffer->completionCurrent)
            {
                Buffer->completionCurrent = gcvVACANT_BUFFER;
            }

            /* Insert into the free list. */
            completion->nextFree = Buffer->completionFree;
            Buffer->completionFree = completion;

            /* Update completion count. */
            Buffer->completionsFree += 1;
        }

        /* Report completion. */
        gcmPRINT_COMPLETION(Buffer, CommandBuffer);

        /* Reset the command buffer completion pointer. */
        CommandBuffer->completion = gcvVACANT_BUFFER;
    }
    while (gcvFALSE);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGBUFFER_CheckCompletion
**
**  Verify whether the command buffer is still in use.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to the gcoVGBUFFER object.
**
**      gcsCMDBUFFER_PTR CommandBuffer
**          Pointer to the command buffer.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_CheckCompletion(
    IN gcoVGBUFFER Buffer,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gceSTATUS status;

    do
    {
        gcsCOMPLETION_SIGNAL_PTR completion;

        /* Get the completion signal. */
        completion = CommandBuffer->completion;

        /* No completion associated? */
        if (completion == gcvVACANT_BUFFER)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Check if the buffer has been processed. */
        gcmASSERT(completion != gcvNULL);
        status = gcoOS_WaitSignal(
            Buffer->os, completion->complete, 0
            );
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGBUFFER_WaitCompletion
**
**  Wait until the command buffer is no longer in use.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to the gcoVGBUFFER object.
**
**      gcsCMDBUFFER_PTR CommandBuffer
**          Pointer to the command buffer.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_WaitCompletion(
    IN gcoVGBUFFER Buffer,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gceSTATUS status;

    /* Report current location. */

    /* Wait until the buffer is executed. */
    status = _WaitForComplete(Buffer, CommandBuffer->completion);

    /* Report current location. */

    /* Return status. */
    return status;
}

/******************************************************************************\
**
**  gcoVGBUFFER_MarkRestart
**
**  Mark begin/end points of restart area due to a possible TS overlfow.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to an gcoVGBUFFER object.
**
**      gctBOOL Begin
**          Not zero for beginning and zero for the end points.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGBUFFER_MarkRestart(
    IN gcoVGBUFFER Buffer,
    IN gctPOINTER Logical,
    IN gctBOOL Begin,
    OUT gctSIZE_T * Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x Logical=0x%x Begin=0x%x Bytes=0x%x",
                  Buffer, Logical, Begin, Bytes);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    do
    {
        gctPOINTER restart;

        /* Command size request? */
        if (Bytes != gcvNULL)
        {
            /* Set the size. */
            * Bytes = Buffer->bufferInfo.restartCommandSize;

            /* Success. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Is the space already reserved? */
        if (Logical != gcvNULL)
        {
            /* Set the initial pointer. */
            restart = Logical;

            /* Success. */
            status = gcvSTATUS_OK;
        }
        else
        {
            /* Reserve data in the buffer. */
            gcmERR_BREAK(gcoVGBUFFER_Reserve(
                Buffer,
                Buffer->bufferInfo.restartCommandSize,
                gcvTRUE,
                &restart
                ));
        }

        if (Begin)
        {
            /* Set the pointer to the next restart. */
            * (gctPOINTER *) restart = Buffer->restart;

            /* Set new head. */
            Buffer->restart = restart;
        }
        else
        {
            gctSIZE_T bytes;

            /* Set the size of the reserved space. */
            bytes = Buffer->bufferInfo.restartCommandSize;

            /* Write the restart END command. */
            gcmERR_BREAK(gcoVGHARDWARE_RestartCommand(
                Buffer->hardware, restart, 0, 0, &bytes
                ));
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_GetCurrentAddress
**
**  Return the current address inside the current command buffe.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to an gcoVGBUFFER object.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned.
**
**  OUTPUT:
**
**      OUT gctUINT32_PTR Address
**          Pointer to a variable that will hold the current address inside
**          the current command buffer.
*/

/* Reserve space in a command buffer. */
gceSTATUS
gcoVGBUFFER_GetCurrentAddress(
    IN gcoVGBUFFER Buffer,
    IN gctBOOL Aligned,
    OUT gctUINT32_PTR Address
    )
{
    gceSTATUS status;

    do
    {
        gcsCMDBUFFER_PTR current;
        gctSIZE_T offset;

        /* Check the uncommitted threshold. */
        gcmERR_BREAK(_CheckUncommittedThreshold(Buffer));

        /* Get the current buffer. */
        current = Buffer->bufferCurrent;

        /* Determine the current offset into the buffer. */
        offset = Aligned
            ? gcmALIGN(current->offset, Buffer->bufferInfo.commandAlignment)
            : current->offset;

        /* Set the current address. */
        * Address = current->address + offset;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_EnsureSpace
**
**  Ensure there is space for the specified number of bytes.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to an gcoVGBUFFER object.
**
**      IN gctSIZE_T Bytes
**          Number of bytes to reserve.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_EnsureSpace(
    IN gcoVGBUFFER Buffer,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;
    gcsCMDBUFFER_PTR current, next;
    gctSIZE_T offset, available;

    gcmHEADER_ARG("Buffer=0x%x Bytes=0x%x Aligned=0x%x",
                  Buffer, Bytes, Aligned);

    status = gcvSTATUS_OK;
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    /* Validate the command buffer. */
    vgmVALIDATE_BUFFER(Buffer);

    /* Report current location. */
    gcmLOG_LOCATION();

    /* Make sure the reservation is possible. */
    if (Bytes > Buffer->bufferMaxSize)
    {
        /* Report current location. */
        gcmLOG_LOCATION();
        gcmFOOTER();
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    /* Check the uncommitted threshold. */
    status = _CheckUncommittedThreshold(Buffer);

    /* Error? */
    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    /* Report current location. */
    gcmLOG_LOCATION();

    /* Get the current buffer. */
    current = Buffer->bufferCurrent;

    do
    {
        /* Validate the command buffer. */
        vgmVALIDATE_BUFFER(Buffer);

        /* Determine the current offset into the buffer. */
        offset = Aligned
            ? gcmALIGN(current->offset, Buffer->bufferInfo.commandAlignment)
            : current->offset;

        /* Determine the size of available memory in the buffer. */
        available = current->size - offset;

        /* Is the buffer big enough? */
        if (Bytes <= available)
        {
            /* Queue the buffer if not already queued. */
            if (Buffer->queueCurrent->commandBuffer != current)
            {
                gcsVGCMDQUEUE_PTR entry;

                /* Report current location. */
                gcmLOG_LOCATION();

                /* Offset has to be reset already. */
                gcmASSERT(current->offset == 0);

                /* Get the next available queue entry. */
                gcmERR_BREAK(_GetNextQueueEntry(Buffer, &entry));

                /* Associate a completion with the current buffer. */
                gcmERR_BREAK(gcoVGBUFFER_AssociateCompletion(Buffer, current));

                /* Set the buffer as the last scheduled. */
                entry->commandBuffer = current;
                entry->dynamic       = gcvTRUE;
            }

            /* Report current location. */
            gcmLOG_LOCATION();

            /* Success. */
            status = gcvSTATUS_OK;
            break;
        }

        /* The current buffer is not big enough, get the next buffer. */
        next = current->nextAllocated;

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Wait until the buffer is executed. */
        gcmERR_BREAK(_WaitForComplete(Buffer, next->completion));

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Reset the offset. */
        next->offset = 0;

        /* Is this the last buffer? */
        if (current == Buffer->bufferLast)
        {
            /* The buffer is not big enough and it's the last one in
               the command buffer container, so we cannot merge it with
               the next one. The only thing to do here is to skip to the
               next buffer. */

            /* Account for the space in the current buffer that might have been left
               unused. */
            Buffer->uncommittedSize
                += Buffer->bufferLimit
                 - (((gctUINT8_PTR) current) + current->bufferOffset + current->offset);

            /* Does the current buffer contain any data? */
            if (current->offset != 0)
            {
                gctSIZE_T executionSize;

                /* Has to be the current command buffer. */
                gcmASSERT(current == Buffer->queueCurrent->commandBuffer);

                /* Current buffer has to have a completion associated. */
                gcmASSERT(current->completion != gcvVACANT_BUFFER);

                /* Report current location. */
                gcmLOG_LOCATION();

                /* Make sure the offset got aligned properly. */
                offset = gcmALIGN(
                    current->offset, Buffer->bufferInfo.commandAlignment
                    );

                /* Determine the size of the data to be executed by the hardware. */
                executionSize = offset + Buffer->bufferInfo.staticTailSize;

                /* Determine the data count. */
                current->dataCount
                    = executionSize / Buffer->bufferInfo.commandAlignment;

                /* Finalize restart marks. */
                _FinalizeRestartMarks(Buffer, current);
            }

            /* Report current location. */
            gcmLOG_LOCATION();

            /* Advance to the next buffer. */
            Buffer->bufferCurrent = current = next;

            /* Restart buffer analysis. */
            continue;
        }

        /* If we've reached this point, we've established the following:
             - the current buffer is not big enough for the new allocation;
             - the current buffer is not the last one;
             - the next buffer is vacant;
           This means that we can merge the current and the next buffers and
           try the requested alocation again on the bigger merged buffer. */

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Release completion signal from the next buffer. */
        gcmERR_BREAK(gcoVGBUFFER_DeassociateCompletion(Buffer, next));

        /* Determine the size of the merged buffer. */
        current->size
            += Buffer->bufferInfo.dynamicTailSize
            +  next->bufferOffset
            +  next->size;

        /* Update the last buffer pointer. */
        if (Buffer->bufferLast == next)
        {
            Buffer->bufferLast = current;
        }

        /* Remove the next buffer from the list. */
        current->nextAllocated = next->nextAllocated;

        /* Report current location. */
        gcmLOG_LOCATION();
    }
    while (gcvTRUE);

    /* Report current location. */
    gcmLOG_LOCATION();

    /* Validate the command buffer. */
    vgmVALIDATE_BUFFER(Buffer);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_Reserve
**
**  Reserve a number of bytes in the buffer.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to an gcoVGBUFFER object.
**
**      IN gctSIZE_T Bytes
**          Number of bytes to reserve.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned.
**
**  OUTPUT:
**
**      OUT gctPOINTER * Memory
**          Pointer to a variable that will hold the address of location in the
**          buffer that has been reserved.
*/

gceSTATUS
gcoVGBUFFER_Reserve(
    IN gcoVGBUFFER Buffer,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x Bytes=0x%x Aligned=0x%x Memory=0x%x",
                  Buffer, Bytes, Aligned, Memory);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);

    do
    {
        gcsCMDBUFFER_PTR current;
        gctSIZE_T offset/*, available*/;

        /* Validate the command buffer. */
        vgmVALIDATE_BUFFER(Buffer);

        /* Update the call counter and enable messages if it's time. */
        gcmCHECK_PRINT_WINDOW();

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Update reserve count. */
        Buffer->reserveCount += 1;

        /* Make sure there is enough space in the current buffer. */
        gcmERR_BREAK(gcoVGBUFFER_EnsureSpace(Buffer, Bytes, Aligned));

        /* Validate the command buffer. */
        vgmVALIDATE_BUFFER(Buffer);

        /* Get the current buffer. */
        current = Buffer->bufferCurrent;

        /* Determine the current offset into the buffer. */
        offset = Aligned
            ? gcmALIGN(current->offset, Buffer->bufferInfo.commandAlignment)
            : current->offset;

        /* Update the uncommitted data counter. */
        Buffer->uncommittedSize += Bytes + (offset - current->offset);

        /* Advance the offset. */
        current->offset = offset + Bytes;

        /* Set the pointer to the reserved buffer. */
        * Memory
            = (gctUINT8_PTR) current
            + current->bufferOffset
            + offset;
    }
    while (gcvFALSE);

    /* Report current location. */
    gcmLOG_LOCATION();

    /* Validate the command buffer. */
    vgmVALIDATE_BUFFER(Buffer);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_Write
**
**  Copy a number of bytes into the buffer.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to an gcoVGBUFFER object.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer that contains the data to be copied.
**
**      IN gctSIZE_T Bytes
**          Number of bytes to copy.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_Write(
    IN gcoVGBUFFER Buffer,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x Data=0x%x Bytes=0x%x Aligned=0x%x",
                  Buffer, Data, Bytes, Aligned);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_ARGUMENT(Data != gcvNULL);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    do
    {
        gctPOINTER memory;

        /* Reserve data in the buffer. */
        gcmERR_BREAK(gcoVGBUFFER_Reserve(
            Buffer, Bytes, Aligned, &memory
            ));

        /* Write data into the buffer. */
        gcmERR_BREAK(gcoOS_MemCopy(
            memory, Data, Bytes
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_Write32
**
**  Copy a 32-bit value into the buffer.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to an gcoVGBUFFER object.
**
**      gctUINT32 Data
**          Data to be writte into the buffer.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_Write32(
    IN gcoVGBUFFER Buffer,
    IN gctUINT32 Data,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x Data=0x%x  Aligned=0x%x",
                  Buffer, Data, Aligned);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    do
    {
        gctPOINTER memory;

        /* Reserve data in the buffer. */
        gcmERR_BREAK(gcoVGBUFFER_Reserve(
            Buffer, gcmSIZEOF(gctUINT32), Aligned, &memory
            ));

        /* Write data into the buffer. */
        * (gctUINT32_PTR) memory = Data;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_Write64
**
**  Copy a 64-bit value into the buffer.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to an gcoVGBUFFER object.
**
**      gctUINT64 Data
**          Data to be writte into the buffer.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned to 64-bit.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_Write64(
    IN gcoVGBUFFER Buffer,
    IN gctUINT64 Data,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x Data=0x%x  Aligned=0x%x",
                  Buffer, Data, Aligned);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    do
    {
        gctPOINTER memory;

        /* Reserve data in the buffer. */
        gcmERR_BREAK(gcoVGBUFFER_Reserve(
            Buffer, gcmSIZEOF(gctUINT64), Aligned, &memory
            ));

        /* Write data into the buffer. */
        * (gctUINT64_PTR) memory = Data;
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_AppendBuffer
**
**  Append an externally allocated command buffer to the current command queue.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to a gcoVGBUFFER object.
**
**      gctPOINTER Logical
**          Points to the location in the command buffer where the command
**          is to be formed.  If gcvNULL is passed, necessary space will be
**          reserved internally.
**
**      gcsCMDBUFFER_PTR CommandBuffer
**          Pointer to the command buffer to append.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Number of bytes required for the command.  gcvNULL is allowed.
*/

gceSTATUS
gcoVGBUFFER_AppendBuffer(
    IN gcoVGBUFFER Buffer,
    IN gctPOINTER Logical,
    IN gcsCMDBUFFER_PTR CommandBuffer,
    OUT gctSIZE_T * Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x Logical=0x%x  CommandBuffer=0x%x Bytes=0x%x",
                  Buffer, Logical, CommandBuffer, Bytes);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    do
    {
        /* Make sure the alignment is proper. */
        gcmASSERT((CommandBuffer->address & Buffer->bufferInfo.addressMask) == 0);

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Use CALL/RETURN if available. */
        if (Buffer->useCallReturn)
        {
            gctPOINTER callCommand;

            /* Command size request? */
            if (Bytes != gcvNULL)
            {
                /* Set the size. */
                * Bytes = Buffer->bufferInfo.callCommandSize;

                /* Success. */
                status = gcvSTATUS_OK;
                break;
            }

            /* Is the space already reserved? */
            if (Logical != gcvNULL)
            {
                /* Set the initial pointer. */
                callCommand = Logical;
            }
            else
            {
                /* Reserve data in the buffer. */
                gcmERR_BREAK(gcoVGBUFFER_Reserve(
                    Buffer,
                    Buffer->bufferInfo.callCommandSize,
                    gcvTRUE,
                    &callCommand
                    ));
            }

            /* Append CALL command. */
            gcmERR_BREAK(gcoVGHARDWARE_CallCommand(
                Buffer->hardware,
                callCommand,
                CommandBuffer->address,
                CommandBuffer->dataCount,
                gcvNULL
                ));
        }
        else
        {
            gcsVGCMDQUEUE_PTR queueEntry;

            /* Extrernal reservation makes no sense here. */
            if ((Bytes != gcvNULL) || (Logical != gcvNULL))
            {
                status = gcvSTATUS_NOT_SUPPORTED;
                break;
            }

            /* Get the next available command queue entry. */
            gcmERR_BREAK(_GetNextQueueEntry(Buffer, &queueEntry));

            /* Initialize the queue entry. */
            queueEntry->commandBuffer = CommandBuffer;
            queueEntry->dynamic       = gcvFALSE;
        }

        /* Associate the buffer with the current completion. */
        gcmERR_BREAK(gcoVGBUFFER_AssociateCompletion(
            Buffer, CommandBuffer
            ));

        /* Report current location. */
        gcmLOG_LOCATION();
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_ReserveTask
**
**  Reserve the specified number of bytes for a task.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to a gcoVGBUFFER object.
**
**      gceBLOCK Block
**          Specifies the hardware block that should generate the event to
**          perform the task.
**
**      gctBOOL Immediate
**          If not zero, the task is immediately scheduled in the command queue.
**          If zero, the task is stored internally and will actually be
**          scheduled during command buffer comitting.
**
**      gctUINT TaskCount
**          The number of tasks to add.
**
**      gctSIZE_T Bytes
**          The size of the task in bytes.
**
**  OUTPUT:
**
**      gctPOINTER * Memory
**          Pointer to the reserved task to be filled in by the caller.
*/

gceSTATUS
gcoVGBUFFER_ReserveTask(
    IN gcoVGBUFFER Buffer,
    IN gceBLOCK Block,
    IN gctUINT TaskCount,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x Block=0x%x  TaskCount=0x%x Bytes=0x%x Memory=0x%x",
                  Buffer, Block, TaskCount, Bytes, Memory);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_ARGUMENT(Memory != gcvNULL);

    do
    {
        gcsTASK_MASTER_ENTRY_PTR tableEntry;
        gcsTASK_PTR clusterHeader;
        gcsTASK_CLUSTER_PTR clusterTask;
        gcsTASK_PTR task;

        /* Get the proper table entry. */
#ifndef __QNXNTO__
        tableEntry = &Buffer->taskTable.table[Block];
#else
        tableEntry = &Buffer->pTaskTable->table[Block];
#endif

        /* Is this the first task to be scheduled? */
        if (tableEntry->head == gcvNULL)
        {
            /* Allocate the cluster. */
            gcmERR_BREAK(_AllocateTask(
                Buffer,
                Block,
                0,          /* Cluster is not counted as a task. */
                gcmSIZEOF(gcsTASK_CLUSTER),
                &clusterHeader
                ));

            /* Skip the header to determine the location of the cluster task. */
            clusterTask = (gcsTASK_CLUSTER_PTR) (clusterHeader + 1);

            /* Initialize the cluster task. */
            clusterTask->id        = gcvTASK_CLUSTER;
            clusterTask->taskCount = 0;
        }
        else
        {
            /* Skip the header to determine the location of the cluster task. */
            clusterTask = (gcsTASK_CLUSTER_PTR) (tableEntry->head + 1);
        }

        /* Allocate space for the task. */
        gcmERR_BREAK(_AllocateTask(
            Buffer, Block, TaskCount, Bytes, &task
            ));

        /* Update cluster count. */
        clusterTask->taskCount += TaskCount;

        /* Set the return value. */
        * Memory = task + 1;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGBUFFER_Commit
**
**  Commit the command buffer to the hardware.
**
**  INPUT:
**
**      gcoVGBUFFER Buffer
**          Pointer to a gcoVGBUFFER object.
**
**      gctBOOL Stall
**          If not zero, the pipe will be stalled until the hardware
**          is finished.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS
gcoVGBUFFER_Commit(
    IN gcoVGBUFFER Buffer,
    IN gctBOOL Stall
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x Stall=0x%x ",
                  Buffer, Stall);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    do
    {
        gctUINT queueLength;
        gcsCMDBUFFER_PTR bufferCurrent;
        gcsHAL_INTERFACE halInterface;
        gcsTASK_SIGNAL_PTR completion;

        /* Validate the command buffer. */
        vgmVALIDATE_BUFFER(Buffer);

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Update commit count. */
        Buffer->commitCount += 1;

        /* Is the queue empty? */
        if ((Buffer->queueCurrent == gcvEMPTY_QUEUE) &&
            (Buffer->taskBlockCount == 0))
        {
            /* Make sure all commands have been executed. */
            if (Stall && (Buffer->completionPrevious != gcvVACANT_BUFFER))
            {
                /* Report current location. */
                gcmLOG_LOCATION();

                /* Wait until the buffer is executed. */
                status = _WaitForComplete(Buffer, Buffer->completionPrevious);

                /* Report current location. */
                gcmLOG_LOCATION();
            }
            else
            {
                /* Report current location. */
                gcmLOG_LOCATION();

                /* Nothing to do. */
                status = gcvSTATUS_OK;
            }

            /* Report current location. */
            gcmLOG_LOCATION();

            /* Done. */
            break;
        }

        /* Merge any delayed context modifications if any. */
        gcmERR_BREAK(gcoVGHARDWARE_MergeContext(
            Buffer->hardware
            ));

        /* Reset the context completion signal. */
        if (Buffer->context->header->completion != gcvVACANT_BUFFER)
        {
            gcmERR_BREAK(gcoOS_Signal(
                Buffer->os,
                Buffer->context->header->completion->complete,
                gcvFALSE
                ));
        }

        /* Current command buffer shortcut. */
        bufferCurrent = Buffer->bufferCurrent;

        /* Reset the uncommitted data counter. */
        Buffer->uncommittedSize = 0;

        /* Add a completion signal task. */
        gcmERR_BREAK(gcoVGBUFFER_ReserveTask(
            Buffer,
            gcvBLOCK_PIXEL,
            1, gcmSIZEOF(gcsTASK_SIGNAL),
            (gctPOINTER *) &completion
            ));

        /* If the last scheduled buffer is the one currently being accumulated
           by gcoVGBUFFER object, split it in two. */
        if (Buffer->queueCurrent->commandBuffer == bufferCurrent)
        {
            /* Report current location. */
            gcmLOG_LOCATION();

            /* Split the current buffer. */
            gcmERR_BREAK(_SplitCurrent(Buffer, gcvFALSE, Buffer->taskBlockCount));
        }

        /* The last scheduled buffer is static. Schedule another one from
           dynamic pool if there are tasks to be performed. */
        else if (Buffer->taskBlockCount > 0)
        {
            /* Report current location. */
            gcmLOG_LOCATION();

            /* Perform queue initialization if empty. */
            _InitEmptyQueue(Buffer);

            /* Advance to the next queue entry (even if the queue is considered
               to be full, there is always one more reserved entry available). */
            Buffer->queueCurrent += 1;

            /* Set the buffer as the last scheduled. */
            Buffer->queueCurrent->commandBuffer = bufferCurrent;
            Buffer->queueCurrent->dynamic       = gcvTRUE;

            /* Associate a completion with the current buffer. */
            gcmERR_BREAK(gcoVGBUFFER_AssociateCompletion(Buffer, bufferCurrent));

            /* Split the buffer. */
            _SplitCurrent(Buffer, gcvFALSE, Buffer->taskBlockCount);
        }
        else
        {
            /* Report current location. */
            gcmLOG_LOCATION();

            /* Queue must not be empty. */
            gcmASSERT(Buffer->queueCurrent != gcvEMPTY_QUEUE);
        }

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Current buffer has to have a completion associated. */
        gcmASSERT(bufferCurrent->completion != gcvVACANT_BUFFER);

        /* Have to have the current completion. */
        gcmASSERT(Buffer->completionCurrent != gcvVACANT_BUFFER);

        /* Fill in event info. */
        completion->id      = gcvTASK_SIGNAL;
        completion->process = Buffer->processID;
        completion->signal  = Buffer->completionCurrent->complete;

        /* Reset the completion signal. */
        gcmERR_BREAK(gcoOS_Signal(
            Buffer->os, completion->signal, gcvFALSE
            ));

        /* Report completion. */
        gcmPRINT_COMPLETION(Buffer, bufferCurrent);

        /* The current completion becomes the previous. */
        Buffer->completionPrevious = Buffer->completionCurrent;
        Buffer->completionCurrent  = gcvVACANT_BUFFER;

        /* Determine the length of the queue. */
        queueLength = Buffer->queueCurrent - Buffer->queueFirst + 1;

        /* Dump the command buffer. */
        if (Buffer->hal->dump != gcvNULL)
        {
            gctUINT i;
            gcsVGCMDQUEUE_PTR queueTemp;
            gcsCMDBUFFER_PTR commandBuffer;
            gctUINT8_PTR data;

            /* Start scanning from the first queue entry. */
            queueTemp = Buffer->queueFirst;

            /* Scan the queue. */
            for (i = 0; i < queueLength; i++)
            {
                /* Get the command buffer pointer. */
                commandBuffer = queueTemp->commandBuffer;

                /* Determine the data logical pointer. */
                data
                    = (gctUINT8_PTR) commandBuffer
                    + commandBuffer->bufferOffset;

                /* Dump it. */
                gcmVERIFY_OK(gcoDUMP_DumpData(
                    Buffer->hal->dump, gcvTAG_COMMAND,
                    0, commandBuffer->offset, data
                    ));

                /* Advance to the next entry. */
                queueTemp += 1;
            }
        }

#ifndef __QNXNTO__
        gcmBUFFER_TRACE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "%s: queue length=%d, task couint=%d\n",
            __FUNCTION__,
            queueLength, Buffer->taskTable.count
            );
#else
        gcmBUFFER_TRACE(
            gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
            "%s: queue length=%d, task couint=%d\n",
            __FUNCTION__,
            queueLength, Buffer->pTaskTable->count
            );
#endif

        /* Send command and context buffer to hardware. */
        halInterface.command = gcvHAL_COMMIT;
        halInterface.u.VGCommit.context    = Buffer->context;
        halInterface.u.VGCommit.queue      = Buffer->queueFirst;
        halInterface.u.VGCommit.entryCount = queueLength;
#ifndef __QNXNTO__
        halInterface.u.VGCommit.taskTable  = &Buffer->taskTable;
#else
        halInterface.u.VGCommit.taskTable  = Buffer->pTaskTable;
#endif

        /* Call kernel service. */
        gcmERR_BREAK(gcoOS_DeviceControl(
            Buffer->os,
            IOCTL_GCHAL_INTERFACE,
            &halInterface, gcmSIZEOF(halInterface),
            &halInterface, gcmSIZEOF(halInterface)
            ));

        /* Verify the result. */
        gcmERR_BREAK(halInterface.status);

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Stall pipe. */
        if (Stall || gcvFORCE_COMMIT_WAIT)
        {
            gcmASSERT(Buffer->completionPrevious != gcvVACANT_BUFFER);

            /* Report current location. */
            gcmLOG_LOCATION();

            /* Wait until the buffer is executed. */
            gcmERR_BREAK(_WaitForComplete(Buffer, Buffer->completionPrevious));

            /* Report current location. */
            gcmLOG_LOCATION();
        }

        /* Reset the command queue. */
        Buffer->queueCurrent = gcvEMPTY_QUEUE;

        /* Reset the task buffer. */
        if (Buffer->taskBlockCount > 0)
        {
            Buffer->taskBlockCount = 0;

            Buffer->taskStorageCurr          = Buffer->taskStorageHead;
            Buffer->taskStorageCurrPtr       = (gctUINT8_PTR) (Buffer->taskStorageCurr + 1);
            Buffer->taskStorageCurrAvailable = Buffer->taskStorageMaxSize;

#ifndef __QNXNTO__
            gcmVERIFY_OK(gcoOS_ZeroMemory(
                &Buffer->taskTable, gcmSIZEOF(Buffer->taskTable)
                ));
#else
            gcmVERIFY_OK(gcoOS_ZeroMemory(
                Buffer->pTaskTable, gcmSIZEOF(*Buffer->pTaskTable)
                ));
#endif
        }

        /* Refetch the current. */
        bufferCurrent = Buffer->bufferCurrent;

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Wait until the buffer is executed. */
        gcmERR_BREAK(_WaitForComplete(Buffer, bufferCurrent->completion));

        /* Report current location. */
        gcmLOG_LOCATION();

        /* Reset the offset. */
        bufferCurrent->offset = 0;

        /* Validate the command buffer. */
        vgmVALIDATE_BUFFER(Buffer);
    }
    while (gcvFALSE);

    /* Report current location. */
    gcmLOG_LOCATION();

    gcmFOOTER();
    /* Return status. */
    return status;
}

#endif /* gcdENABLE_VG */
