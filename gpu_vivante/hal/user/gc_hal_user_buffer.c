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
**  gcoBUFFER object for user HAL layers.
**
*/

#include "gc_hal_user_precomp.h"
#include "gc_hal_user_buffer.h"

#define _GC_OBJ_ZONE            gcvZONE_BUFFER

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

struct _gcoBUFFER
{
    /* Object. */
    gcsOBJECT                   object;

    /* Pointer to the required objects. */
    gcoHAL                      hal;

    /* Pointer to gckCONTEXT object. */
    gckCONTEXT                  context;

    /* Size of buffer. */
    gctSIZE_T                   size;
    gctSIZE_T                   maxSize;

    /* Number of command buffers. */
    gctSIZE_T                   count;

    /* Array of command buffers and their signals. */
    gcoCMDBUF                   commandBuffers[gcdMAX_CMD_BUFFERS];
    gctSIGNAL                   signal[gcdMAX_CMD_BUFFERS];

    /* Current command buffer. */
    gctUINT                     currentCommandBufferIndex;
    gcoCMDBUF                   currentCommandBuffer;

    /* Reserved bytes. */
    struct _gcsCOMMAND_INFO
    {
        gctSIZE_T   alignment;
        gctSIZE_T   reservedHead;
        gctSIZE_T   reservedTail;
    }                           info;
    gctSIZE_T                   totalReserved;
};

/******************************************************************************\
******************************* Private Functions ******************************
\******************************************************************************/

static gceSTATUS
gcoBUFFER_FreeObjects(
    IN gcoBUFFER Buffer
    )
{
    gceSTATUS status;
    gctUINT i;

    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    /* Roll back all command buffers. */
    for (i = 0; i < gcmCOUNTOF(Buffer->commandBuffers); i += 1)
    {
        if (Buffer->commandBuffers[i] != gcvNULL)
        {
            /* Destroy command buffer. */
            gcmONERROR(gcoCMDBUF_Destroy(Buffer->commandBuffers[i]));
            Buffer->commandBuffers[i] = gcvNULL;
        }

        if (Buffer->signal[i] != gcvNULL)
        {
            /* Destroy signal. */
            gcmONERROR(gcoOS_DestroySignal(gcvNULL, Buffer->signal[i]));
            Buffer->signal[i] = gcvNULL;
        }
    }

    /* Free the object memory. */
    gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Buffer));

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gceSTATUS
gcoBUFFER_GetCMDBUF(
    IN gcoBUFFER Buffer
    )
{
    gceSTATUS status;
    gcoCMDBUF command;
    gctSIZE_T index;
    gcePIPE_SELECT entryPipe;

    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    /* Determine the next command buffer. */
    if (Buffer->currentCommandBuffer == gcvNULL)
    {
        /* First time - get the first buffer. */
        index = 0;

        /* Select 3D pipe for the first buffer. */
        entryPipe = gcvPIPE_3D;
    }
    else
    {
        /* Get current entry pipe. */
        entryPipe = Buffer->currentCommandBuffer->entryPipe;

        /* Determine the next command buffer index. */
        index = (Buffer->currentCommandBufferIndex + 1) % Buffer->count;
    }

    /* Test if command buffer is available. */
    status = gcoOS_WaitSignal(gcvNULL,
                              Buffer->signal[index],
                              0);

    if (status == gcvSTATUS_TIMEOUT)
    {
        if (Buffer->count < gcdMAX_CMD_BUFFERS)
        {
            do
            {
                if (Buffer->commandBuffers[Buffer->count] == gcvNULL)
                {
                    /* Construct a command buffer. */
                    gcmERR_BREAK(gcoCMDBUF_Construct(
                        gcvNULL,
                        gcvNULL,
                        Buffer->maxSize,
                        &Buffer->info,
                        &Buffer->commandBuffers[Buffer->count]));
                }

                if (Buffer->signal[Buffer->count] == gcvNULL)
                {
                    /* Create the signal. */
                    gcmERR_BREAK(gcoOS_CreateSignal(
                        gcvNULL,
                        gcvFALSE,
                        &Buffer->signal[Buffer->count]));

                    gcmTRACE_ZONE(
                        gcvLEVEL_INFO, gcvZONE_SIGNAL,
                        "%s(%d): buffer %d signal created 0x%08X",
                        __FUNCTION__, __LINE__,
                        Buffer->count, Buffer->signal[Buffer->count]);
                }

                /* Mark the buffer as available. */
                gcmERR_BREAK(gcoOS_Signal(gcvNULL,
                                          Buffer->signal[Buffer->count],
                                          gcvTRUE));

                /* Use the new buffer. */
                index          = Buffer->count;
                Buffer->count += 1;

                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_BUFFER,
                              "Using %lu command buffers.",
                              Buffer->count);
            }
            while (gcvFALSE);
        }

        /* Wait for buffer to become available. */
        gcmONERROR(gcoOS_WaitSignal(gcvNULL,
                                    Buffer->signal[index],
                                    gcPLS.hal->timeOut));
    }
    else
    {
        gcmONERROR(status);
    }

    /* Select new command buffer. */
    Buffer->currentCommandBufferIndex = index;
    Buffer->currentCommandBuffer      = Buffer->commandBuffers[index];

    /* Grab pointer to current command buffer. */
    command = Buffer->currentCommandBuffer;

    /* Set the entry pipe. */
    command->entryPipe = entryPipe;

    /* Reset command buffer. */
    command->startOffset = 0;
    command->offset      = Buffer->info.reservedHead;
    command->free        = command->bytes - Buffer->totalReserved;

    /* Succees. */
    gcmFOOTER_ARG("currentCommandBufferIndex=%d",
                  Buffer->currentCommandBufferIndex);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}


/******************************************************************************\
***************************** gcoCMDBUF Object Code *****************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoCMDBUF_Construct
**
**  Construct a new gcoCMDBUF object.
**
**  INPUT:
**
**      gcoOS Os
**          Pointer to a gcoOS object.
**
**      gcoHARDWARE Hardware
**          Pointer to a gcoHARDWARE object.
**
**      gctSIZE_T Bytes
**          Number of bytes for the buffer.
**
**      gcsCOMMAND_BUFFER_PTR Info
**          Alignment and head/tail information.
**
**  OUTPUT:
**
**      gcoCMDBUF * CommandBuffer
**          Pointer to a variable that will hold the the gcoCMDBUF object
**          pointer.
*/
gceSTATUS
gcoCMDBUF_Construct(
    IN gcoOS Os,
    IN gcoHARDWARE Hardware,
    IN gctSIZE_T Bytes,
    IN gcsCOMMAND_INFO_PTR Info,
    OUT gcoCMDBUF * CommandBuffer
    )
{
    gceSTATUS status;
    gcoCMDBUF commandBuffer = gcvNULL;
    gctSIZE_T objectSize;

#ifdef __QNXNTO__
    gctPHYS_ADDR physical;
#else
    gctPOINTER pointer = gcvNULL;
#endif

    gcmHEADER_ARG("Bytes=%lu Info=0x%x", Bytes, Info);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    /* Set the size of the object. */
    objectSize = gcmSIZEOF(struct _gcoCMDBUF);

    /* Allocate the gcoCMDBUF object. */
#ifdef __QNXNTO__
    /* gcoCMDBUF object needs to be accessible from the kernel; to avoid
       copying of the data for each access, allocate the object from the
       kernel non-paged memory. */
    gcmONERROR(gcoOS_AllocateNonPagedMemory(
        gcvNULL, gcvTRUE, &objectSize, &physical, (gctPOINTER *) &commandBuffer
        ));
#else
    /* Currently in most OS we are able to access the user-side data from
       the kernel by simple memory mapping, therefore here we allocate the
       object from the cached user memory. */
    gcmONERROR(gcoOS_Allocate(gcvNULL, objectSize, &pointer));

    commandBuffer = pointer;
#endif

    /* Reset the command buffer object. */
    gcmONERROR(gcoOS_ZeroMemory(commandBuffer, objectSize));

    /* Initialize the gcoCMDBUF object. */
    commandBuffer->object.type = gcvOBJ_COMMANDBUFFER;
    commandBuffer->bytes       = Bytes;

    /* Allocate the physical buffer for the command. */
    gcmONERROR(gcoOS_AllocateContiguous(
        gcvNULL, gcvTRUE,
        &commandBuffer->bytes,
        &commandBuffer->physical,
        &commandBuffer->logical
        ));

    /* Initialize command buffer. */
    commandBuffer->free = commandBuffer->bytes;

#if gcdSECURE_USER
    /* Determine the size of the state array. */
    commandBuffer->hintArraySize = Bytes;

    /* Allocate the state array. */
#ifdef __QNXNTO__
    gcmONERROR(gcoOS_AllocateNonPagedMemory(
        gcvNULL, gcvTRUE,
        &commandBuffer->hintArraySize,
        &physical,
        (gctPOINTER *) &commandBuffer->hintArray
        ));
#else
    gcmONERROR(gcoOS_Allocate(
        gcvNULL,
        commandBuffer->hintArraySize,
        &pointer
        ));

    commandBuffer->hintArray = pointer;
#endif

    /* Initialize the state array tail pointer. */
    commandBuffer->hintArrayTail = commandBuffer->hintArray;
#endif

    /* Return pointer to the gcoCMDBUF object. */
    *CommandBuffer = commandBuffer;

    /* Success. */
    gcmFOOTER_ARG("*CommandBuffer=0x%x", *CommandBuffer);
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    if (commandBuffer != gcvNULL)
    {
        if (commandBuffer->logical != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_FreeContiguous(
                gcvNULL,
                commandBuffer->physical,
                commandBuffer->logical,
                commandBuffer->bytes
                ));
        }

#if gcdSECURE_USER
        if (commandBuffer->hintArray != gcvNULL)
        {
#ifdef __QNXNTO__
            gcmVERIFY_OK(gcoOS_FreeNonPagedMemory(
                gcvNULL,
                commandBuffer->hintArraySize,
                gcvNULL,
                commandBuffer->hintArray
                ));
#else
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, commandBuffer->hintArray));
#endif
        }
#endif

#ifdef __QNXNTO__
        gcmVERIFY_OK(gcoOS_FreeNonPagedMemory(
            gcvNULL, objectSize, gcvNULL, commandBuffer
            ));
#else
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, commandBuffer));
#endif
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoCMDBUF_Destroy
**
**  Destroy a gcoCMDBUF object.
**
**  INPUT:
**
**      gcoCMDBUF CommandBuffer
**          Pointer to an gcoCMDBUF object.
**
**  OUTPUT:
**
**      None.
*/
gceSTATUS
gcoCMDBUF_Destroy(
    IN gcoCMDBUF CommandBuffer
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("CommandBuffer=0x%x", CommandBuffer);

    /* Verify the object. */
    gcmVERIFY_OBJECT(CommandBuffer, gcvOBJ_COMMANDBUFFER);

    if (CommandBuffer->logical != gcvNULL)
    {
        /* Use events to free the buffer. */
        iface.command = gcvHAL_FREE_CONTIGUOUS_MEMORY;
        iface.u.FreeContiguousMemory.bytes    = CommandBuffer->bytes;
        iface.u.FreeContiguousMemory.physical = CommandBuffer->physical;
        iface.u.FreeContiguousMemory.logical  = CommandBuffer->logical;

        /* Send event. */
        gcmONERROR(gcoHARDWARE_CallEvent(&iface));

        /* Reset the buffer pointer. */
        CommandBuffer->logical = gcvNULL;
    }

#if gcdSECURE_USER
    if (CommandBuffer->hintArray != gcvNULL)
    {
#ifdef __QNXNTO__
        gcmONERROR(gcoOS_FreeNonPagedMemory(
            gcvNULL,
            CommandBuffer->hintArraySize,
            gcvNULL,
            CommandBuffer->hintArray
            ));
#else
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, CommandBuffer->hintArray));
#endif

        CommandBuffer->hintArray =
        CommandBuffer->hintArrayTail = gcvNULL;
    }
#endif

    /* Free the gcoCMDBUF object. */
#ifdef __QNXNTO__
    gcmONERROR(gcoOS_FreeNonPagedMemory(
        gcvNULL, gcmSIZEOF(struct _gcoCMDBUF), gcvNULL, CommandBuffer
        ));
#else
    gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, CommandBuffer));
#endif

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/******************************************************************************\
******************************* gcoBUFFER API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoBUFFER_Construct
**
**  Construct a new gcoBUFFER object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**      gcoHARDWARE Hardware
**          Pointer to a gcoHARDWARE object.
**
**      gckCONTEXT Context
**          Pointer to a gckCONTEXT object.
**
**      gctSIZE_T MaxSize
**          Maximum size of buffer.
**
**  OUTPUT:
**
**      gcoBUFFER * Buffer
**          Pointer to a variable that will hold the the gcoBUFFER object
**          pointer.
*/
gceSTATUS
gcoBUFFER_Construct(
    IN gcoHAL Hal,
    IN gcoHARDWARE Hardware,
    IN gckCONTEXT Context,
    IN gctSIZE_T MaxSize,
    OUT gcoBUFFER * Buffer
    )
{
    gceSTATUS status;
    gcoBUFFER buffer = gcvNULL;
    gctUINT i = 0;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Hal=0x%x Hardware=0x%x Context=0x%x MaxSize=%lu",
                  Hal, Hardware, Context, MaxSize);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hal, gcvOBJ_HAL);
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT(Buffer != gcvNULL);

    /***************************************************************************
    ** Allocate and reset the gcoBUFFER object.
    */

    gcmONERROR(gcoOS_Allocate(
        gcvNULL, gcmSIZEOF(struct _gcoBUFFER), &pointer
        ));

    buffer = pointer;

    /* Initialize the gcoBUFFER object. */
    buffer->object.type = gcvOBJ_BUFFER;
    buffer->hal         = Hal;
    buffer->context     = Context;

    /* Maximum size of buffer. */
    buffer->size    = 0;
    buffer->maxSize = MaxSize;

    /* Zero the command buffers. */
    for (i = 0; i < gcmCOUNTOF(buffer->commandBuffers); ++i)
    {
        buffer->commandBuffers[i] = gcvNULL;
        buffer->signal[i]         = gcvNULL;
    }


    /***************************************************************************
    ** Query alignment.
    */

    gcmONERROR(gcoHARDWARE_QueryCommandBuffer(
        &buffer->info.alignment,
        &buffer->info.reservedHead,
        &buffer->info.reservedTail
        ));

    buffer->totalReserved
        = buffer->info.reservedHead
        + buffer->info.reservedTail
        + buffer->info.alignment;


    /***************************************************************************
    ** Initialize the command buffers.
    */

    for (i = 0; i < gcdCMD_BUFFERS; ++i)
    {
        /* Construct a command buffer. */
        gcmONERROR(gcoCMDBUF_Construct(
            gcvNULL,
            gcvNULL,
            buffer->maxSize,
            &buffer->info,
            &buffer->commandBuffers[i]
            ));

        /* Create the signal. */
        gcmONERROR(gcoOS_CreateSignal(
            gcvNULL, gcvFALSE, &buffer->signal[i]
            ));

        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_SIGNAL,
            "%s(%d): buffer %d signal created 0x%08X",
            __FUNCTION__, __LINE__,
            i, buffer->signal[i]
            );

        /* Mark the buffer as available. */
        gcmONERROR(gcoOS_Signal(
            gcvNULL, buffer->signal[i], gcvTRUE\
            ));
    }

    /* Number of buffers initialized. */
    buffer->count = gcdCMD_BUFFERS;

    /* Grab the first command buffer. */
    buffer->currentCommandBuffer = gcvNULL;
    gcmONERROR(gcoBUFFER_GetCMDBUF(buffer));

    /* Return pointer to the gcoBUFFER object. */
    *Buffer = buffer;

    /* Success. */
    gcmFOOTER_ARG("*Buffer=0x%x", *Buffer);
    return gcvSTATUS_OK;

OnError:
    if (buffer != gcvNULL)
    {
        gcmVERIFY_OK(gcoBUFFER_FreeObjects(buffer));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoBUFFER_Destroy
**
**  Destroy an gcoBUFFER object.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to an gcoBUFFER object to delete.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoBUFFER_Destroy(
    IN gcoBUFFER Buffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Buffer=0x%x", Buffer);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);

    /* Commit buffer before destroying it. */
    if (Buffer->size != 0)
    {
        /* Commit the command buffers. */
        gcmONERROR(gcoHARDWARE_Commit());

        /* Stall the hardware. */
        gcmONERROR(gcoHARDWARE_Stall());
    }

    /* Free all associated objects. */
    gcmONERROR(gcoBUFFER_FreeObjects(Buffer));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoBUFFER_Write
**
**  Copy a number of bytes into the buffer.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to an gcoBUFFER object.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer that contains the data to be copied.
**
**      IN gctSIZE_T Bytes
**          Number of bytes to copy.
**
**      IN gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned to 64-bit.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoBUFFER_Write(
    IN gcoBUFFER Buffer,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;
    gcoCMDBUF reserve;

    gcmHEADER_ARG("Buffer=0x%x Data=0x%x Bytes=%lu Aligned=%d",
                  Buffer, Data, Bytes, Aligned);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* Reserve data in the buffer. */
    gcmONERROR(gcoBUFFER_Reserve(Buffer, Bytes, Aligned, &reserve));

    /* Write data into the buffer. */
    gcmONERROR(gcoOS_MemCopy(reserve->lastReserve, Data, Bytes));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoBUFFER_Reserve
**
**  Reserve a number of bytes in the buffer.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to an gcoBUFFER object.
**
**      gctSIZE_T Bytes
**          Number of bytes to reserve.
**
**      gctBOOL Aligned
**          gcvTRUE if the data needs to be aligned to 64-bit.
**
**  OUTPUT:
**
**      gctUINT32_PTR ** AddressHints
**          Pointer to a variable that receives the current position in the
**          state hint array.  gcvNULL is allowed.
**
**      gctPOINTER * Memory
**          Pointer to a variable that will hold the address of location in the
**          buffer that has been reserved.
*/
gceSTATUS
gcoBUFFER_Reserve(
    IN gcoBUFFER Buffer,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned,
    OUT gcoCMDBUF * Reserve
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcoCMDBUF current;
    gctSIZE_T alignBytes, bytes;
    gctUINT offset;

    gcmHEADER_ARG("Buffer=0x%x Bytes=%lu Aligned=%d Reserve=0x%x",
                  Buffer, Bytes, Aligned, Reserve);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmDEBUG_VERIFY_ARGUMENT(Reserve != gcvNULL);

    /* Get the current command buffer. */
    current = Buffer->currentCommandBuffer;

    /* Compute the number of aligned bytes. */
    alignBytes = Aligned
               ? ( gcmALIGN(current->offset, Buffer->info.alignment)
                 - current->offset
                 )
               : 0;

    /* Compute the number of required bytes. */
    bytes = Bytes + alignBytes;

    if (bytes > current->free)
    {
        gcsHAL_INTERFACE iface;

        if (bytes > Buffer->maxSize - Buffer->totalReserved)
        {
            /* This just won't fit! */
            gcmFATAL("FATAL: Command of %lu bytes is too big!", Bytes);
            gcmONERROR(gcvSTATUS_OUT_OF_MEMORY);
        }

        /* Sent event to signal when command buffer completes. */
        iface.command            = gcvHAL_SIGNAL;
        iface.u.Signal.signal    = Buffer->signal
                                   [Buffer->currentCommandBufferIndex];
        iface.u.Signal.auxSignal = gcvNULL;
        iface.u.Signal.process   = gcoOS_GetCurrentProcessID();
        iface.u.Signal.fromWhere = gcvKERNEL_COMMAND;

        /* Send event. */
        gcmONERROR(
            gcoHARDWARE_CallEvent(&iface));

        /* Commit current command buffer. */
        gcmONERROR(
            gcoHARDWARE_Commit());

        /* Grab a new command buffer. */
        gcmONERROR(
            gcoBUFFER_GetCMDBUF(Buffer));

        /* Get the pointer. */
        current = Buffer->currentCommandBuffer;

        /* Calculate total bytes again. */
        alignBytes = 0;
        bytes      = Bytes;
    }

    gcmASSERT(current != gcvNULL);
    gcmASSERT(bytes   <= current->free);

    /* Determine the data offset. */
    offset = current->offset + alignBytes;

    /* Update the last reserved location. */
    current->lastReserve = (gctUINT8_PTR) current->logical + offset;
    current->lastOffset  = offset;

    /* Adjust command buffer size. */
    current->offset += bytes;
    current->free   -= bytes;

    /* Set the result. */
    * Reserve = current;

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoBUFFER_Commit
**
**  Commit the command buffer to the hardware.
**
**  INPUT:
**
**      gcoBUFFER Buffer
**          Pointer to a gcoBUFFER object.
**
**      gcePIPE_SELECT CurrentPipe
**          Current graphics pipe.
**
**      gcsSTATE_DELTA_PTR StateDelta
**          Pointer to the state delta.
**
**      gcoQUEUE Queue
**          Pointer to a gcoQUEUE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoBUFFER_Commit(
    IN gcoBUFFER Buffer,
    IN gcePIPE_SELECT CurrentPipe,
    IN gcsSTATE_DELTA_PTR StateDelta,
    IN gcoQUEUE Queue
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;
    gcoCMDBUF current;

    gcmHEADER_ARG("Buffer=0x%x Queue=0x%x",
                  Buffer, Queue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Buffer, gcvOBJ_BUFFER);
    gcmVERIFY_OBJECT(Queue, gcvOBJ_QUEUE);

    /* Grab current command buffer. */
    current = Buffer->currentCommandBuffer;

    if (current == gcvNULL)
    {
        /* No command buffer, done. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if (current->offset - current->startOffset <= Buffer->info.reservedHead)
    {
        /* Commit the event queue. */
        status = gcoQUEUE_Commit(Queue);
        gcmFOOTER();
        return status;
    }

    /* Make sure the tail got aligned properly. */
    current->offset = gcmALIGN(current->offset, Buffer->info.alignment);

    if (gcPLS.hal->dump != gcvNULL)
    {
        /* Dump the command buffer. */
        gcmVERIFY_OK(
            gcoDUMP_DumpData(gcPLS.hal->dump,
                             gcvTAG_COMMAND,
                             0,
                             current->offset
                             - current->startOffset
                             - Buffer->info.reservedHead,
                             (gctUINT8_PTR) current->logical
                             + current->startOffset
                             + Buffer->info.reservedHead));
    }

    /* The current pipe becomes the exit pipe for the current command buffer. */
    current->exitPipe = CurrentPipe;

    /* Send command and context buffer to hardware. */
    iface.command = gcvHAL_COMMIT;
    iface.u.Commit.context       =
#ifndef VIVANTE_NO_3D
        (current->using2D && !current->using3D) ? gcvNULL : Buffer->context;
#else
        gcvNULL;
#endif

    iface.u.Commit.commandBuffer = current;
    iface.u.Commit.delta         = StateDelta;
    iface.u.Commit.queue         = Queue->head;

    /* Call kernel service. */
    gcmONERROR(
        gcoOS_DeviceControl(gcvNULL,
                            IOCTL_GCHAL_INTERFACE,
                            &iface, gcmSIZEOF(iface),
                            &iface, gcmSIZEOF(iface)));

    gcmONERROR(iface.status);

    /* Free the event queue. */
    gcmONERROR(gcoQUEUE_Free(Queue));

    /* Advance the offset for next commit. */
    current->startOffset = current->offset + Buffer->info.reservedTail;

    if (current->bytes - current->startOffset > Buffer->totalReserved)
    {
        /* Adjust buffer offset and size. */
        current->offset = current->startOffset + Buffer->info.reservedHead;
        current->free   = current->bytes - current->offset
                        - Buffer->info.alignment
                        - Buffer->info.reservedTail;
    }
    else
    {
        /* Buffer is full. */
        current->startOffset = current->bytes;
        current->offset      = current->bytes;
        current->free        = 0;
    }

    /* The exit pipe becomes the entry pipe for the next command buffer. */
    current->entryPipe = current->exitPipe;

#if gcdSECURE_USER
    /* Reset the state array tail. */
    current->hintArrayTail = current->hintArray;
#endif

    /* Reset usage flags. */
    current->using2D         = gcvFALSE;
    current->using3D         = gcvFALSE;
    current->usingFilterBlit = gcvFALSE;
    current->usingPalette    = gcvFALSE;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
