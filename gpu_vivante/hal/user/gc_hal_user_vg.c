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

#include "gc_hal_user_vg.h"
#include <math.h>

#define _GC_OBJ_ZONE    gcvZONE_VG

#if gcdENABLE_SHARE_TESSELLATION
static gcsTESSELATION_PTR   g_tsBuffer;
static gctPOINTER           g_tsMutex;
#endif

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

static gceSTATUS
_FreeTessellationBuffer(
    IN gcoVG Vg,
    IN gcsTESSELATION_PTR TessellationBuffer
    )
{
    gceSTATUS status;

    do
    {
        /* Delete the old buffer if any. */
        if (TessellationBuffer->node == gcvNULL)
        {
            gcmASSERT(TessellationBuffer->allocatedSize == 0);
            status = gcvSTATUS_OK;
            break;
        }

        /* Schedule to delete the old buffer. */
        gcmERR_BREAK(gcoHAL_ScheduleVideoMemory(
            Vg->hal, TessellationBuffer->node
            ));

        /* Reset the buffer. */
        TessellationBuffer->node = gcvNULL;
        TessellationBuffer->allocatedSize = 0;
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

#if gcdENABLE_SHARE_TESSELLATION

static gceSTATUS
_TessellationBufferAddRef(
        IN gcoVG Vg,
        IN gcsTESSELATION_PTR  TessellationBuffer)
{
    gcmASSERT(TessellationBuffer->reference >= 0);

    TessellationBuffer->reference++;

    return gcvSTATUS_OK;
}

static gceSTATUS
_TessellationBufferRelease(
        IN gcoVG Vg,
        IN gcsTESSELATION_PTR  TessellationBuffer)
{
    gceSTATUS status = gcvSTATUS_OK;

    TessellationBuffer->reference--;

    gcmASSERT(TessellationBuffer->reference >= 0);

    do
    {
        if (TessellationBuffer->reference == 0)
        {
            gcmERR_BREAK(_FreeTessellationBuffer(Vg, TessellationBuffer));
            gcmERR_BREAK(gcmOS_SAFE_FREE(Vg->os, TessellationBuffer));

            if (TessellationBuffer == g_tsBuffer) g_tsBuffer = gcvNULL;
        }

    }while(gcvFALSE);

    return status;
}

static gceSTATUS
_CreateTSMutex(IN gcoVG Vg)
{
    if (g_tsMutex == gcvNULL)
    {
        return gcoOS_CreateMutex(Vg->os, &g_tsMutex);
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_DestroyTSMutex(IN gcoVG Vg)
{
    if (g_tsMutex != gcvNULL)
    {
        return gcoOS_DeleteMutex(Vg->os, g_tsMutex);
    }

    return gcvSTATUS_OK;
}

#endif

static gceSTATUS
_GetTessellationBuffer(
    IN gcoVG Vg,
    OUT gcsTESSELATION_PTR * TessellationBuffer
    )
{
    gceSTATUS status;

    do
    {
        gctUINT allocationSize;
        gctUINT bufferSize, alignedBufferSize;
        gctUINT L1Size, alignedL1Size;
        gctUINT L2Size, alignedL2Size;
        gctUINT bufferStride, shift;
        gcsTESSELATION_PTR buffer;

        if (Vg->vg20)
        {
            /* The tessellation buffer is 128x16 tiled or 512x16 tiles without
               MSAA. */
            bufferStride = (Vg->renderQuality == gcvVG_NONANTIALIASED)
                ? (((Vg->targetWidth + 511) & ~511) * 2)
                : (((Vg->targetWidth + 127) & ~127) * 8);

            bufferSize = bufferStride * ((Vg->targetHeight + 15) & ~15);

            /* Determine buffer shift value. */
            shift = 0;
        }
        else
        {
            /* For MSAA, we use 8x1 64-bit pixels per cache line;
               otherwise we use 32x1 16-bit pixels per cache line. */
            bufferStride = (Vg->renderQuality == gcvVG_NONANTIALIASED)
                ? (((Vg->targetWidth + 31) & ~31) * 2)
                : (((Vg->targetWidth +  7) &  ~7) * 8);

            bufferSize = bufferStride * Vg->targetHeight;

            /* Determine buffer shift value. */
            shift = (Vg->targetWidth > 2048)
                ? 3
                : (Vg->targetWidth > 1024)
                    ? 2
                    : (Vg->targetWidth > 512)
                        ? 1
                        : 0;
        }

        /* L1 status.  Each bit represents 64 bytes from Buffer.
           So we need BufferSize / 64 bits, grouped in 64-bit cache lines. */
        L1Size = ((bufferSize / 64 + 63) & ~63) / 8;

        /* L2 status.  Each bit represents 8 bytes from L1 status.
           So we need L1Size / 8 bits, grouped in 64-bit cache lines. */
        L2Size  = ((L1Size / 8 + 63) & ~63) / 8;

        /* Align the buffer sizes. */
        alignedBufferSize = gcmALIGN(bufferSize, 64);
        alignedL1Size     = gcmALIGN(L1Size,     64);
        alignedL2Size     = gcmALIGN(L2Size,     64);

        /* Determin the allocation size. */
        allocationSize = alignedBufferSize + alignedL1Size + alignedL2Size;

#if gcdENABLE_SHARE_TESSELLATION

        status = _CreateTSMutex(Vg);
        if (gcmIS_ERROR(status))
        {
            return status;
        }

        status = gcoOS_AcquireMutex(Vg->os, g_tsMutex, gcvINFINITE);
        if (gcmIS_ERROR(status))
        {
            return status;
        }

        if ((g_tsBuffer == gcvNULL) || (g_tsBuffer->allocatedSize < allocationSize))
        {
            gcmERR_BREAK(gcoOS_Allocate(Vg->os,  gcmSIZEOF(gcsTESSELATION), (gctPOINTER *) &buffer));

            /* Allocate a new buffer. */
            gcmERR_BREAK(gcoVGHARDWARE_AllocateLinearVideoMemory(
                Vg->hw,
                allocationSize, 64,
                gcvPOOL_SYSTEM,
                &buffer->node,
                &buffer->tsBufferPhysical,
                (gctPOINTER *) &buffer->tsBufferLogical
                ));

            /* Set the new size. */
            buffer->allocatedSize = allocationSize;
            /* Determine buffer pointers. */
            buffer->L1BufferLogical  = buffer->tsBufferLogical  + alignedBufferSize;
            buffer->L1BufferPhysical = buffer->tsBufferPhysical + alignedBufferSize;
            buffer->L2BufferLogical  = buffer->L1BufferLogical  + alignedL1Size;
            buffer->L2BufferPhysical = buffer->L1BufferPhysical + alignedL1Size;

            /* Set buffer parameters. */
            buffer->stride    = bufferStride;
            buffer->shift     = shift;
            buffer->clearSize = alignedL2Size;
            buffer->reference = 0;

            g_tsBuffer = buffer;
        }

        if (Vg->tsBuffer != g_tsBuffer)
        {
            if (Vg->tsBuffer != gcvNULL)
            {
                gcmERR_BREAK(_TessellationBufferRelease(Vg, Vg->tsBuffer));
            }

            Vg->tsBuffer = g_tsBuffer;

            gcmERR_BREAK(_TessellationBufferAddRef(Vg, Vg->tsBuffer));
        }


        Vg->tsBuffer2.node              = Vg->tsBuffer->node;
        Vg->tsBuffer2.tsBufferLogical   = Vg->tsBuffer->tsBufferLogical;
        Vg->tsBuffer2.tsBufferPhysical  = Vg->tsBuffer->tsBufferPhysical;
        Vg->tsBuffer2.allocatedSize     = Vg->tsBuffer->allocatedSize;

        Vg->tsBuffer2.L1BufferLogical   = Vg->tsBuffer->tsBufferLogical  + alignedBufferSize;
        Vg->tsBuffer2.L1BufferPhysical  = Vg->tsBuffer->tsBufferPhysical + alignedBufferSize;
        Vg->tsBuffer2.L2BufferLogical   = Vg->tsBuffer->L1BufferLogical  + alignedL1Size;
        Vg->tsBuffer2.L2BufferPhysical  = Vg->tsBuffer->L1BufferPhysical + alignedL1Size;
        Vg->tsBuffer2.stride            = bufferStride;
        Vg->tsBuffer2.shift             = shift;
        Vg->tsBuffer2.clearSize         = alignedL2Size;

        /* Get the buffer pointer. */
        buffer = &Vg->tsBuffer2;
#else
        /* Advance to the next buffer. */
        if (Vg->tsBufferIndex == gcmCOUNTOF(Vg->tsBuffer) - 1)
        {
            Vg->tsBufferIndex = 0;
        }
        else
        {
            Vg->tsBufferIndex += 1;
        }

        /* Get the buffer pointer. */
        buffer = &Vg->tsBuffer[Vg->tsBufferIndex];

        /* Is the buffer big enough? */
        if (buffer->allocatedSize < allocationSize)
        {
            /* No, reallocate the buffer. */
            gcmERR_BREAK(_FreeTessellationBuffer(Vg, buffer));

            /* Allocate a new buffer. */
            gcmERR_BREAK(gcoVGHARDWARE_AllocateLinearVideoMemory(
                Vg->hw,
                allocationSize, 64,
                gcvPOOL_DEFAULT,
                &buffer->node,
                &buffer->tsBufferPhysical,
                (gctPOINTER *) &buffer->tsBufferLogical
                ));

            /* Set the new size. */
            buffer->allocatedSize = allocationSize;
        }
        else
        {
            /* Success. */
            status = gcvSTATUS_OK;
        }

        /* Determine buffer pointers. */
        buffer->L1BufferLogical  = buffer->tsBufferLogical  + alignedBufferSize;
        buffer->L1BufferPhysical = buffer->tsBufferPhysical + alignedBufferSize;
        buffer->L2BufferLogical  = buffer->L1BufferLogical  + alignedL1Size;
        buffer->L2BufferPhysical = buffer->L1BufferPhysical + alignedL1Size;

        /* Set buffer parameters. */
        buffer->stride    = bufferStride;
        buffer->shift     = shift;
        buffer->clearSize = alignedL2Size;
#endif
        /* Set the result pointer. */
        (* TessellationBuffer) = buffer;
    }
    while (gcvFALSE);

#if gcdENABLE_SHARE_TESSELLATION
    gcmVERIFY_OK(gcoOS_ReleaseMutex(Vg->os, g_tsMutex));
#endif
    /* Return status. */
    return status;
}


/******************************************************************************\
********************************* gcoHAL Object ********************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoHAL_QueryPathStorage
**
**  Query path data storage attributes.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to the gcoHAL object.
**
**  OUTPUT:
**
**      gcsPATH_BUFFER_INFO_PTR Information
**          Pointer to the information structure to receive buffer attributes.
*/
gceSTATUS
gcoHAL_QueryPathStorage(
    IN gcoHAL Hal,
    OUT gcsPATH_BUFFER_INFO_PTR Information
    )
{
    gceSTATUS status;
    /* Copy the command buffer information. */
    status = gcoVGHARDWARE_QueryPathStorage(
        gcvNULL, Information
        );

    /* Return status. */
    return status;
}

gceSTATUS
gcoHAL_AssociateCompletion(
    IN gcoHAL Hal,
    IN gcsPATH_DATA_PTR PathData
    )
{
    gceSTATUS status;
    /* Relay the call. */
    status = gcoVGHARDWARE_AssociateCompletion(
        gcvNULL, &PathData->data
        );

    /* Return status. */
    return status;
}

gceSTATUS
gcoHAL_DeassociateCompletion(
    IN gcoHAL Hal,
    IN gcsPATH_DATA_PTR PathData
    )
{
    gceSTATUS status;
    /* Relay the call. */
    status = gcoVGHARDWARE_DeassociateCompletion(
        gcvNULL, &PathData->data
        );

    /* Return status. */
    return status;
}

gceSTATUS
gcoHAL_CheckCompletion(
    IN gcoHAL Hal,
    IN gcsPATH_DATA_PTR PathData
    )
{
    gceSTATUS status;
    /* Relay the call. */
    status = gcoVGHARDWARE_CheckCompletion(
        gcvNULL, &PathData->data
        );

    /* Return status. */
    return status;
}

gceSTATUS
gcoHAL_WaitCompletion(
    IN gcoHAL Hal,
    IN gcsPATH_DATA_PTR PathData
    )
{
    gceSTATUS status;
    /* Relay the call. */
    status = gcoVGHARDWARE_WaitCompletion(
        gcvNULL, &PathData->data
        );

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoHAL_Flush
**
**  Flush the pixel cache.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to the gcoHAL object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_Flush(
    IN gcoHAL Hal
    )
{
    /* Flush the current pipe. */
    return gcoVGHARDWARE_FlushPipe(gcvNULL);
}

/*******************************************************************************
**
**  gcoHAL_AllocateLinearVideoMemory
**
**  Allocate and lock linear video memory. If both Address and Memory parameters
**  are given as gcvNULL, the memory will not be locked, only allocated.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to the gcoHAL object.
**
**      gctUINT Size
**          The size of the area to allocate.
**
**      gctUINT Alignment
**          The minimum alignment for the allocated area.
**
**      gcePOOL Pool
**          The desired pool for the allocated area.
**
**  OUTPUT:
**
**      gcuVIDMEM_NODE_PTR * Node
**          Pointer to the variable to receive the node of the allocated area.
**
**      gctUINT32 * Address
**          Pointer to the variable to receive the hardware address of the
**          allocated area.  Can be gcvNULL.
**
**      gctPOINTER * Memory
**          Pointer to the variable to receive the logical pointer to the
**          allocated area.  Can be gcvNULL.
*/
gceSTATUS
gcoHAL_AllocateLinearVideoMemory(
    IN gcoHAL Hal,
    IN gctUINT Size,
    IN gctUINT Alignment,
    IN gcePOOL Pool,
    OUT gcuVIDMEM_NODE_PTR * Node,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;
    /* Call down to gcoHARDWARE. */
    status = gcoVGHARDWARE_AllocateLinearVideoMemory(
        gcvNULL, Size, Alignment, Pool, Node, Address, Memory
        );

    /* Return the status. */
    return status;
}
/*******************************************************************************
**
**  gcoHAL_FreeVideoMemory
**
**  Free linear video memory allocated with gcoHAL_AllocateLinearVideoMemory.
**  Assumed that the memory is locked.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to the gcoHAL object.
**
**      gcuVIDMEM_NODE_PTR Node
**          Node of the allocated memory to free.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_FreeVideoMemory(
    IN gcoHAL Hal,
    IN gcuVIDMEM_NODE_PTR Node
    )
{
    gceSTATUS status;
    /* Call down to gcoHARDWARE. */
    status = gcoVGHARDWARE_FreeVideoMemory(gcvNULL, Node);

    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoHAL_ScheduleVideoMemory
**
**  Schedule to free linear video memory allocated with
**  gcoHAL_AllocateLinearVideoMemory. Assumed that the memory is locked.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to the gcoHAL object.
**
**      gcuVIDMEM_NODE_PTR Node
**          Node of the allocated memory to free.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoHAL_ScheduleVideoMemory(
    IN gcoHAL Hal,
    IN gcuVIDMEM_NODE_PTR Node
    )
{
    gceSTATUS status;
    /* Call down to gcoHARDWARE. */
    status = gcoVGHARDWARE_ScheduleVideoMemory(gcvNULL, Node, gcvTRUE);

    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoHAL_SplitAddress
**
**  Split a hardware specific memory address into a pool and offset.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to the gcoHAL object.
**
**      gctUINT32 Address
**          Address in hardware specific format.
**
**  OUTPUT:
**
**      gcePOOL * Pool
**          Pointer to a variable that will hold the pool type for the address.
**
**      gctUINT32 * Offset
**          Pointer to a variable that will hold the offset for the address.
*/
gceSTATUS
gcoHAL_SplitAddress(
    IN gcoHAL Hal,
    IN gctUINT32 Address,
    OUT gcePOOL * Pool,
    OUT gctUINT32 * Offset
    )
{
    /* Call hardware library. */
    return gcoVGHARDWARE_SplitAddress(gcvNULL, Address, Pool, Offset);
}

/*******************************************************************************
**
**  gcoHAL_CombineAddress
**
**  Combine pool and offset into a harwdare address.
**
**  INPUT:
**
**      gcoHALE Hardware
**          Pointer to the gcoHAL object.
**
**      gcePOOL Pool
**          Pool type for the address.
**
**      gctUINT32 Offset
**          Offset for the address.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Pointer to the combined address.
*/
gceSTATUS
gcoHAL_CombineAddress(
    IN gcoHAL Hal,
    IN gcePOOL Pool,
    IN gctUINT32 Offset,
    OUT gctUINT32 * Address
    )
{
    /* Call hardware library. */
    return gcoVGHARDWARE_CombineAddress(gcvNULL, Pool, Offset, Address);
}

/*******************************************************************************
**
**  gcoHAL_QueryCommandBuffer
**
**  Query command buffer attributes.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to the gcoHAL object.
**
**  OUTPUT:
**
**      gcsCOMMAND_BUFFER_INFO_PTR Information
**          Pointer to the information structure to receive buffer attributes.
*/
gceSTATUS
gcoHAL_QueryCommandBuffer(
    IN gcoHAL Hal,
    OUT gcsCOMMAND_BUFFER_INFO_PTR Information
    )
{
    gceSTATUS status;
    /* Copy the command buffer information. */
    status = gcoVGHARDWARE_QueryCommandBuffer(
        gcvNULL, Information
        );

    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoHAL_GetAlignedSurfaceSize
**
**  Align the specified size accordingly to the hardware requirements.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceSURF_TYPE Type
**          Type of surface to create.
**
**      gctUINT32_PTR Width
**          Pointer to the width to be aligned.  If 'Width' is gcvNULL, no width
**          will be aligned.
**
**      gctUINT32_PTR Height
**          Pointer to the height to be aligned.  If 'Height' is gcvNULL, no height
**          will be aligned.
**
**  OUTPUT:
**
**      gctUINT32_PTR Width
**          Pointer to a variable that will receive the aligned width.
**
**      gctUINT32_PTR Height
**          Pointer to a variable that will receive the aligned height.
*/
gceSTATUS
gcoHAL_GetAlignedSurfaceSize(
    IN gcoHAL Hal,
    IN gceSURF_TYPE Type,
    IN OUT gctUINT32_PTR Width,
    IN OUT gctUINT32_PTR Height
    )
{
    gceSTATUS status;
    /* Align the sizes. */
    status = gcoVGHARDWARE_AlignToTile(
        gcvNULL, Type, Width, Height
        );

    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoHAL_ReserveTask
**
**  Allocate a task to be perfomed when signaled from the specified block.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to the gcoHAL object.
**
**      gceBLOCK Block
**          Specifies the hardware block that should generate the event to
**          perform the task.
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
gcoHAL_ReserveTask(
    IN gcoHAL Hal,
    IN gceBLOCK Block,
    IN gctUINT TaskCount,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    /* Allocate an event. */
    return gcoVGHARDWARE_ReserveTask(
        gcvNULL, Block, TaskCount, Bytes, Memory
        );
}

/******************************************************************************\
********************************** gcoVG Object ********************************
\******************************************************************************/

gctBOOL
gcoVG_IsMaskSupported(
    IN gceSURF_FORMAT Format
    )
{
    return gcoVGHARDWARE_IsMaskSupported(Format);
}

gctBOOL
gcoVG_IsTargetSupported(
    IN gceSURF_FORMAT Format
    )
{
    return gcoVGHARDWARE_IsTargetSupported(Format);
}

gctBOOL
gcoVG_IsImageSupported(
    IN gceSURF_FORMAT Format
    )
{
    return gcoVGHARDWARE_IsImageSupported(Format);
}

gctUINT8 gcoVG_PackColorComponent(
    gctFLOAT Value
    )
{
    return gcoVGHARDWARE_PackColorComponent(Value);
}


/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   The gcoVG object constructor.
**
**  gcoVG_Construct constructs a new gcoVG object if the running hardware
**  has an OpenVG pipe.
**
**  @param[in]  Hal     Pointer to an gcoHAL object.
**  @param[out] Vg      Pointer to a variable receiving the gcoVG object
**                      pointer.
*/
gceSTATUS
gcoVG_Construct(
    IN gcoHAL Hal,
    OUT gcoVG * Vg
    )
{
    gceSTATUS status, last;
    gcoVG vg = gcvNULL;

    gcmHEADER_ARG("Hal=0x%x Vg=0x%x ",
                  Hal, Vg);
    gcmVERIFY_ARGUMENT(Vg != gcvNULL);

    do
    {
#if !gcdENABLE_SHARE_TESSELLATION
        gctUINT i;
#endif

        /* Make sure the hardware supports VG. */
        if (!gcoVGHARDWARE_IsFeatureAvailable(gcvNULL, gcvFEATURE_PIPE_VG))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* Allocate the gcoVG structure. */
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL, gcmSIZEOF(struct _gcoVG), (gctPOINTER *) &vg
            ));

        /* Reset everything to 0. */
        gcmERR_BREAK(gcoOS_ZeroMemory(vg, sizeof(struct _gcoVG)));

        /* Initialize the object. */
        vg->object.type = gcvOBJ_VG;

        /* Copy object pointers. */
        vg->hal = Hal;
        vg->os  = gcvNULL;
        vg->hw  = gcvNULL;

        /* Query VG version support. */
        vg->vg20 = gcoVGHARDWARE_IsFeatureAvailable(
            vg->hw, gcvFEATURE_VG20
            );

        vg->vg21 = gcoVGHARDWARE_IsFeatureAvailable(
            vg->hw, gcvFEATURE_VG21
            );

        /* Set default gcoVG states. */
        vg->renderQuality = gcvVG_NONANTIALIASED;

        /* Query path data command buffer attributes. */
        gcmERR_BREAK(gcoVGHARDWARE_QueryPathStorage(vg->hw, &vg->pathInfo));

        /* Query generic command buffer attributes. */
        gcmERR_BREAK(gcoVGHARDWARE_QueryCommandBuffer(vg->hw, &vg->bufferInfo));

#if gcdENABLE_SHARE_TESSELLATION
        vg->tsBuffer = gcvNULL;
#else
        /* Reset the tesselation buffers. */
        vg->tsBufferIndex = gcmCOUNTOF(vg->tsBuffer) - 1;

        for (i = 0; i < gcmCOUNTOF(vg->tsBuffer); i += 1)
        {
            vg->tsBuffer[i].node = gcvNULL;
            vg->tsBuffer[i].allocatedSize = 0;
        }
#endif
        /* Return gcoVG object pointer. */
        *Vg = vg;

        /* Success. */
        gcmFOOTER_ARG("*Vg=0x%x", *Vg);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (vg != gcvNULL)
    {
        /* Free the gcoVG structure. */
        gcmCHECK_STATUS(gcmOS_SAFE_FREE(gcvNULL, vg));
    }

    /* Return the error. */
    gcmFOOTER();
    return status;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   The gcoVG object destructor.
**
**  gcoVG_Destroy destroys a previously constructed gcoVG object.
**
**  @param[in]  Vg      Pointer to the gcoVG object that needs to be destroyed.
*/
gceSTATUS
gcoVG_Destroy(
    IN gcoVG Vg
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x",
                  Vg);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    do
    {
#if gcdENABLE_SHARE_TESSELLATION
        if (Vg->tsBuffer != gcvNULL)
        {
            gcmERR_BREAK(gcoOS_AcquireMutex(Vg->os, g_tsMutex, gcvINFINITE));

            status = _TessellationBufferRelease(Vg, Vg->tsBuffer);

            gcmVERIFY_OK(gcoOS_ReleaseMutex(Vg->os, g_tsMutex));

            gcmERR_BREAK(status);
        }
#else
        gctUINT i;

        /* Destroy tesselation buffers. */
        for (i = 0; i < gcmCOUNTOF(Vg->tsBuffer); i += 1)
        {
            gcmERR_BREAK(_FreeTessellationBuffer(Vg, &Vg->tsBuffer[i]));
        }
#endif
        /* Destroy scissor surface. */
        if (Vg->scissor != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Vg->scissor));
            Vg->scissor = gcvNULL;
        }

        /* Mark the object as unknown. */
        Vg->object.type = gcvOBJ_UNKNOWN;

        /* Free the gcoVG structure. */
        gcmERR_BREAK(gcmOS_SAFE_FREE(Vg->os, Vg));
    }
    while (gcvFALSE);

    gcmFOOTER_NO();
    /* Return status. */
    return gcvSTATUS_OK;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Set the current render target.
**
**  Specify the current render target for the gcoVG object to render into.
**
**  @param[in]  Vg      Pointer to the gcoVG object.
**  @param[in]  Target  Pointer to a gcoSURF object that defines the target.
*/
gceSTATUS
gcoVG_SetTarget(
    IN gcoVG Vg,
    IN gcoSURF Target
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x Target=0x%x ",
                  Vg, Target);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    do
    {
        /* Is there a target currently set? */
        if (Target == gcvNULL)
        {
            /* Call hardware layer to set the target. */
            gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
                Vg->hw, gcvNULL
                ));

            /* Reset the target. */
            Vg->target       = gcvNULL;
            Vg->targetWidth  = 0;
            Vg->targetHeight = 0;
        }
        else
        {
            /* Call hardware layer to set the target. */
            gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
                Vg->hw, &Target->info
                ));

            /* Set target. */
            Vg->target       = Target;
            Vg->targetWidth  = Target->info.rect.right;
            Vg->targetHeight = Target->info.rect.bottom;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the error. */
    return status;
}

gceSTATUS
gcoVG_UnsetTarget(
    IN gcoVG Vg,
    IN gcoSURF Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Vg=0x%x Surface=0x%x ",
                  Vg, Surface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Only unset surface if it is the current render target. */
    if (Vg->target == Surface)
    {
        status = gcoVG_SetTarget(Vg, gcvNULL);
    }

    gcmFOOTER();
    /* Return the status. */
    return status;
}

gceSTATUS
gcoVG_SetUserToSurface(
    IN gcoVG Vg,
    IN gctFLOAT UserToSurface[9]
    )
{
    gcmHEADER_ARG("Vg=0x%x UserToSurface=0x%x ",
                  Vg, UserToSurface);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);
    gcmVERIFY_ARGUMENT(UserToSurface != gcvNULL);

    /* Copy first row. */
    Vg->userToSurface[0] = UserToSurface[0];
    Vg->userToSurface[1] = UserToSurface[3];
    Vg->userToSurface[2] = UserToSurface[6];

    /* Copy second row. */
    Vg->userToSurface[3] = UserToSurface[1];
    Vg->userToSurface[4] = UserToSurface[4];
    Vg->userToSurface[5] = UserToSurface[7];

    /* Copy third row. */
    Vg->userToSurface[6] = UserToSurface[2];
    Vg->userToSurface[7] = UserToSurface[5];
    Vg->userToSurface[8] = UserToSurface[8];

    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

gceSTATUS
gcoVG_SetSurfaceToImage(
    IN gcoVG Vg,
    IN gctFLOAT SurfaceToImage[9]
    )
{
    gcmHEADER_ARG("Vg=0x%x UserToSurface=0x%x ",
                  Vg, SurfaceToImage);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);
    gcmVERIFY_ARGUMENT(SurfaceToImage != gcvNULL);

    /* Copy first row. */
    Vg->surfaceToImage[0] = SurfaceToImage[0];
    Vg->surfaceToImage[1] = SurfaceToImage[3];
    Vg->surfaceToImage[2] = SurfaceToImage[6];

    /* Copy second row. */
    Vg->surfaceToImage[3] = SurfaceToImage[1];
    Vg->surfaceToImage[4] = SurfaceToImage[4];
    Vg->surfaceToImage[5] = SurfaceToImage[7];

    /* Copy third row. */
    Vg->surfaceToImage[6] = SurfaceToImage[2];
    Vg->surfaceToImage[7] = SurfaceToImage[5];
    Vg->surfaceToImage[8] = SurfaceToImage[8];

    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Enable or disable masking.
**
**  Enable or disable the masking.  When masking is enabled, make sure to set
**  a masking surface.
**
**  @param[in]  Vg      Pointer to the gcoVG object.
**  @param[in]  Enable  gcvTRUE to enable masking or gcvFALSE to disable
**                      masking.  Masking is by default disabled.
*/
gceSTATUS
gcoVG_EnableMask(
    IN gcoVG Vg,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Enable=0x%x ",
                  Vg, Enable);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Call gcoVGHARDWARE to enable masking. */
    status = gcoVGHARDWARE_EnableMask(Vg->hw, Enable);
    gcmFOOTER();
    return status;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Set the current mask buffer.
**
**  Specify the current mask buffer to be used by the gcoVG object.
**
**  @param[in]  Vg      Pointer to the gcoVG object.
**  @param[in]  Mask    Pointer to a gcoSURF object that defines the mask.
*/
gceSTATUS
gcoVG_SetMask(
    IN gcoVG Vg,
    IN gcoSURF Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x Mask=0x%x ",
                  Vg, Mask);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    do
    {
        if (Vg->mask == Mask)
        {
            /* The mask did not change, do nothing. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        /* Is there a mask currently set? */
        if (Vg->mask != gcvNULL)
        {
            /* Unlock the current mask. */
            gcmERR_BREAK(gcoSURF_Unlock(
                Vg->mask, gcvNULL
                ));

            /* Reset the mask. */
            Vg->mask = gcvNULL;
        }

        /* Mask reset? */
        if (Mask == gcvNULL)
        {
            /* Success. */
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        /* Make sure we have a good surface. */
        gcmVERIFY_OBJECT(Mask, gcvOBJ_SURF);

        /* Test if surface format is supported. */
        if (!gcoVG_IsMaskSupported(Mask->info.format))
        {
            /* Format not supported. */
            gcmFOOTER_NO();
            return gcvSTATUS_NOT_SUPPORTED;
        }

        /* Set mask. */
        Vg->mask = Mask;

        /* Lock the surface. */
        gcmERR_BREAK(gcoSURF_Lock(
            Mask, gcvNULL, gcvNULL
            ));

        /* Program hardware. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgMask(
            Vg->hw, &Mask->info
            ));

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the error. */
    return status;
}

gceSTATUS
gcoVG_UnsetMask(
    IN gcoVG Vg,
    IN gcoSURF Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Vg=0x%x Surface=0x%x ",
                  Vg, Surface);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);
    gcmVERIFY_OBJECT(Surface, gcvOBJ_SURF);

    /* Only unset surface if it is the current mask. */
    if (Vg->mask == Surface)
    {
        status = gcoVG_SetMask(Vg, gcvNULL);
    }

    gcmFOOTER();
    /* Return the status. */
    return status;
}

gceSTATUS
gcoVG_FlushMask(
    IN gcoVG Vg
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x",
                  Vg);
    /* Verify the argument. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Flush mask. */
    status = gcoVGHARDWARE_FlushVgMask(Vg->hw);

    gcmFOOTER();
    /* Return the status. */
    return status;
}


/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Enable or disable scissoring.
**
**  Enable or disable scissoring.  When scissoring is enabled, make sure to set
**  the scissor regions.
**
**  @param[in]  Vg      Pointer to the gcoVG object.
**  @param[in]  Enable  gcvTRUE to enable scissoring or gcvFALSE to disable
**                      scissoring.  Scissoring is by default disabled.
*/
gceSTATUS
gcoVG_EnableScissor(
    IN gcoVG Vg,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Enable=0x%x ",
                  Vg, Enable);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Call gcoVGHARDWARE to enable scissoring. */
    status = gcoVGHARDWARE_EnableScissor(Vg->hw, Enable);

    gcmFOOTER();
    return status;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Set the scissor rectangles.
**
**  Set the scissor rectangles.  If there are no rectangles specified, the
**  scissoring region is empty an nothing will be rendered when scissoring is
**  enabled.
**
**  @param[in]  Vg              Pointer to the gcoVG object.
**  @param[in]  RectangleCount  Number of rectangles to be used as the
**                              scissoring region.
**  @Param[in]  Rectangles      Pointer to the rectangles to be used as the
**                              scissoring region.
*/
gceSTATUS
gcoVG_SetScissor(
    IN gcoVG Vg,
    IN gctSIZE_T RectangleCount,
    IN gcsVG_RECT_PTR Rectangles
    )
{
    gceSTATUS status, last;
    gctUINT8_PTR bits = gcvNULL;

    gcmHEADER_ARG("Vg=0x%x RectangleCount=0x%x  Rectangles=0x%x",
                  Vg, RectangleCount, Rectangles);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    if (RectangleCount > 0)
    {
        gcmVERIFY_ARGUMENT(Rectangles != gcvNULL);
    }

    do
    {
        /* Free any current scissor surface. */
        if (Vg->scissor != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Vg->scissor));
            Vg->scissor = gcvNULL;
        }

        /* Construct a new scissor surface. */
        gcmERR_BREAK(gcoSURF_Construct(
            Vg->hal,
            Vg->targetWidth,
            Vg->targetHeight,
            1,
            gcvSURF_SCISSOR,
            gcvSURF_A1,
            gcvPOOL_DEFAULT,
            &Vg->scissor
            ));

        /* Lock scissor surface. */
        gcmERR_BREAK(gcoSURF_Lock(
            Vg->scissor,
            &Vg->scissorAddress,
            &Vg->scissorBits
            ));

        bits = (gctUINT8_PTR) Vg->scissorBits;

        /* Zero the entire surface. */
        gcmERR_BREAK(gcoOS_ZeroMemory(
            bits,
            Vg->scissor->info.stride * Vg->targetHeight
            ));

        /* Loop while there are rectangles to process. */
        while (RectangleCount > 0)
        {
            gctINT left, right, top, bottom;

            /* Intersect rectangle with the surface. */
            left   = gcmMAX(Rectangles->x, 0);
            right  = gcmMIN(Rectangles->x + Rectangles->width,  Vg->targetWidth);
            top    = gcmMAX(Rectangles->y, 0);
            bottom = gcmMIN(Rectangles->y + Rectangles->height, Vg->targetHeight);

            /* Test for valid scissor region. */
            if ((left < right) && (top < bottom))
            {
                /* Compute left and right byte. */
                gctINT leftByte  = left        >> 3;
                gctINT rightByte = (right - 1) >> 3;

                /* Compute left and right mask. */
                gctUINT8 leftMask  = 0xFF << (left & 7);
                gctUINT8 rightMask = 0xFF >> (((right - 1) & 7) ^ 7);

                /* Start at top line of region. */
                gctUINT32 offset = top * Vg->scissor->info.stride;
                while (top++ < bottom)
                {
                    /* See if left and right bytes overlap. */
                    if (leftByte == rightByte)
                    {
                        /* Allow left and right pixels. */
                        bits[offset + leftByte] |= leftMask & rightMask;
                    }

                    else
                    {
                        /* Allow left pixels. */
                        bits[offset + leftByte] |= leftMask;

                        /* See if we have any middle bytes to fill. */
                        if (leftByte + 1 < rightByte)
                        {
                            /* Allow all middle pixels. */
                            gcmVERIFY_OK(gcoOS_MemFill(
                                &bits[offset + leftByte + 1],
                                0xFF,
                                rightByte - leftByte - 1
                                ));
                        }

                        /* Allow right pixels. */
                        bits[offset + rightByte] |= rightMask;
                    }

                    /* Next line. */
                    offset += Vg->scissor->info.stride;
                }
            }

            /* Next rectangle. */
            ++Rectangles;
            --RectangleCount;
        }

        /* Program the scissors to the hardware. */
        gcmERR_BREAK(gcoVGHARDWARE_SetScissor(
            Vg->hw,
            Vg->scissorAddress,
            Vg->scissor->info.stride
            ));

        /* Unlock the scissor surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(
            Vg->scissor, bits
            ));
        gcmFOOTER_NO();
        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (bits != gcvNULL)
    {
        /* Unlock the scissor surface. */
        gcmCHECK_STATUS(gcoSURF_Unlock(
            Vg->scissor, bits
            ));
    }

    gcmFOOTER();
    /* Return the error. */
    return status;
}

gceSTATUS
gcoVG_SetTileFillColor(
    IN gcoVG Vg,
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gcmHEADER_ARG("Vg=0x%x Red=0x%x  Green=0x%x Blue=0x%x  Alpha=0x%x",
                  Vg, Red, Green, Blue, Alpha);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set tile fill color. */
    Vg->tileFillColor = gcoVGHARDWARE_PackColor32(Red, Green, Blue, Alpha);

    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Enable or disable color transformation.
**
**  Enable or disable color transformation.
**
**  @param[in]  Vg      Pointer to the gcoVG object.
**  @param[in]  Enable  gcvTRUE to enable color transformation or gcvFALSE to
**                      disable color transformation.  Color transformation is
**                      by default disabled.
*/
gceSTATUS
gcoVG_EnableColorTransform(
    IN gcoVG Vg,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Enable=0x%x",
                  Vg, Enable);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Call gcoVGHARDWARE to enable color transformation. */
    status = gcoVGHARDWARE_EnableColorTransform(Vg->hw, Enable);
    gcmFOOTER();
    return status;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Set the color transformation values.
**
**  Set the color transformation values.
**
**  @param[in]  Vg              Pointer to the gcoVG object.
**  @param[in]  ColorTransform  Pointer to an array of four scale and four bias
**                              values for each color channel, in the order:
**                              red, green, blue, alpha.
*/
gceSTATUS
gcoVG_SetColorTransform(
    IN gcoVG Vg,
    IN gctFLOAT ColorTransform[8]
    )
{
    gctINT i;
    gctFLOAT_PTR scale, offset;
    gctFLOAT clampedScale[4], clampedOffset[4];
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x ColorTransform=0x%x",
                  Vg, ColorTransform);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);
    gcmVERIFY_ARGUMENT(ColorTransform != gcvNULL);

    /* Set initial pointers. */
    scale  = &ColorTransform[0];
    offset = &ColorTransform[4];

    /* Clamp scale and offset values. */
    for (i = 0; i < 4; i += 1)
    {
        clampedScale[i]  = gcmCLAMP(scale[i], -127.0f, 127.0f);
        clampedOffset[i] = gcmCLAMP(offset[i],  -1.0f,   1.0f);
    }

    /* Send scale and offset to hardware. */
    status = gcoVGHARDWARE_SetColorTransform(Vg->hw, clampedScale, clampedOffset);
    gcmFOOTER();
    return status;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Set the paint to be used by the next rendering.
**
**  Program the hardware with the specified color.
**
**  @param[in]  Vg          Pointer to the gcoVG object.
**  @param[in]  Red         The value of the red channel.
**  @param[in]  Green       The value of the green channel.
**  @param[in]  Blue        The value of the blue channel.
**  @param[in]  Alpha       The value of the alpha channel.
*/
gceSTATUS
gcoVG_SetSolidPaint(
    IN gcoVG Vg,
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Red=0x%x Green=0x%x Blue=0x%x Alpha=0x%x",
                  Vg, Red, Green, Blue, Alpha);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set solid paint. */
    status = gcoVGHARDWARE_SetPaintSolid(
        Vg->hw, Red, Green, Blue, Alpha
        );
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVG_SetLinearPaint(
    IN gcoVG Vg,
    IN gctFLOAT Constant,
    IN gctFLOAT StepX,
    IN gctFLOAT StepY
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Constant=0x%x StepX=0x%x StepY=0x%x",
                  Vg, Constant, StepX, StepY);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set solid paint. */
    status =  gcoVGHARDWARE_SetPaintLinear(
        Vg->hw, Constant, StepX, StepY
        );
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVG_SetRadialPaint(
    IN gcoVG Vg,
    IN gctFLOAT LinConstant,
    IN gctFLOAT LinStepX,
    IN gctFLOAT LinStepY,
    IN gctFLOAT RadConstant,
    IN gctFLOAT RadStepX,
    IN gctFLOAT RadStepY,
    IN gctFLOAT RadStepXX,
    IN gctFLOAT RadStepYY,
    IN gctFLOAT RadStepXY
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x LinConstant=0x%x LinStepX=0x%x LinStepY=0x%x"
                "RadConstant=0x%x RadStepX=0x%x RadStepY=0x%x RadStepXX=0x%x RadStepXY=0x%x",
                  Vg, LinConstant, LinStepX, LinStepY, RadConstant, RadStepX, RadStepY, RadStepXX, RadStepXY
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set solid paint. */
    status = gcoVGHARDWARE_SetPaintRadial(
        Vg->hw,
        LinConstant,
        LinStepX,
        LinStepY,
        RadConstant,
        RadStepX,
        RadStepY,
        RadStepXX,
        RadStepYY,
        RadStepXY
        );

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVG_SetPatternPaint(
    IN gcoVG Vg,
    IN gctFLOAT UConstant,
    IN gctFLOAT UStepX,
    IN gctFLOAT UStepY,
    IN gctFLOAT VConstant,
    IN gctFLOAT VStepX,
    IN gctFLOAT VStepY,
    IN gctBOOL Linear
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x UConstant=0x%x UStepX=0x%x UStepY=0x%x"
                "VConstant=0x%x VStepX=0x%x VStepY=0x%x Linear=0x%x",
                  Vg, UConstant, UStepX, UStepY, VConstant, VStepX, VStepY, Linear
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set solid paint. */
    status = gcoVGHARDWARE_SetPaintPattern(
        Vg->hw,
        UConstant, UStepX, UStepY,
        VConstant, VStepX, VStepY,
        Linear
        );

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVG_SetColorRamp(
    IN gcoVG Vg,
    IN gcoSURF ColorRamp,
    IN gceTILE_MODE ColorRampSpreadMode
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x ColorRamp=0x%x ColorRampSpreadMode=0x%x",
                  Vg, ColorRamp, ColorRampSpreadMode
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set solid paint. */
    status = gcoVGHARDWARE_SetPaintImage(
        Vg->hw,
        &ColorRamp->info,
        ColorRampSpreadMode,
        gcvFILTER_LINEAR,
        Vg->tileFillColor
        );
    gcmFOOTER();
    return status;

}

gceSTATUS
gcoVG_SetPattern(
    IN gcoVG Vg,
    IN gcoSURF Pattern,
    IN gceTILE_MODE TileMode,
    IN gceIMAGE_FILTER Filter
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Pattern=0x%x TileMode=0x%x Filter=0x%x",
                  Vg, Pattern, TileMode, Filter
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set solid paint. */
    status = gcoVGHARDWARE_SetPaintImage(
        Vg->hw,
        &Pattern->info,
        TileMode,
        Filter,
        Vg->tileFillColor
        );
    gcmFOOTER();
    return status;
}


/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Set the blend mode.
**
**  Set the blend mode to be used by the next rendering.
**
**  @param[in]  Vg          Pointer to the gcoVG object.
**  @param[in]  Mode        Blend mode.
*/
gceSTATUS
gcoVG_SetBlendMode(
    IN gcoVG Vg,
    IN gceVG_BLEND Mode
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Mode=0x%x",
                  Vg, Mode
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set blend mode in hardware. */
    status = gcoVGHARDWARE_SetVgBlendMode(Vg->hw, Mode);
    gcmFOOTER();
    return status;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Clear the current render target.
**
**  The current render target will be cleared using the specified color.  The
**  color is specified in pre-multiplied sRGB format.
**
**  @param[in]  Vg      Pointer to the gcoVG object.
**  @param[in]  Origin  Pointer to the origin of the rectangle to be cleared.
**  @param[in]  Size    Pointer to the size of the rectangle to be cleared.
*/
gceSTATUS
gcoVG_Clear(
    IN gcoVG Vg,
    IN gctINT X,
    IN gctINT Y,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x X=0x%x Width=0x%x Height=0x%x",
                  Vg, X, Width,  Height
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Clear rectangle. */
    status = gcoVGHARDWARE_VgClear(Vg->hw, X, Y, Width, Height);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Draw a path with the current settings.
**
**  Draw the specified path into the current render target.
**  All current states will take effect, including blending, paint, scissoring,
**  masking, and color transformation.  The path will be offset by the speficied
**  value to lower the retesselation of paths that only "move" on the screen.
**
**  @param[in]  Vg                  Pointer to the gcoVG object.
**  @param[in]  PathData            Pointer to the path data buffer.
**  @param[in]  Scale               Path scale.
**  @param[in]  Bias                Path bias.
**  @param[in]  SoftwareTesselation Software vs. hardware tesselation.
*/

gceSTATUS
gcoVG_DrawPath(
    IN gcoVG Vg,
    IN gcsPATH_DATA_PTR PathData,
    IN gctFLOAT Scale,
    IN gctFLOAT Bias,
    IN gctBOOL SoftwareTesselation
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x PathData=0x%x Scale=0x%x Bias=0x%x SoftwareTesselation=0x%x",
                  Vg, PathData, Scale,  Bias, SoftwareTesselation
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    do
    {
        gcsTESSELATION_PTR tessellationBuffer;

        /* Get the pointer to the current tesselation buffer. */
        gcmERR_BREAK(_GetTessellationBuffer(
            Vg, &tessellationBuffer
            ));

        /* Set path type. In VG version before 2.0 it was not possible to change
           the type of the path data within a path, therefore the data within
           a path had to be of the same type; we set the type here. With VG 2.0
           and above the path objects carry data types within them. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPathDataType(
            Vg->hw, PathData->dataType
            ));

        /* Program tesselation buffers. */
        gcmERR_BREAK(gcoVGHARDWARE_SetTessellation(
            Vg->hw,
            SoftwareTesselation,
            (gctUINT16)Vg->targetWidth, (gctUINT16)Vg->targetHeight,
            Bias, Scale,
            Vg->userToSurface,
            tessellationBuffer
            ));

        /* Draw the path. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawPath(
            Vg->hw,
            SoftwareTesselation,
            PathData,
            tessellationBuffer,
            gcvNULL
            ));

        gcmFOOTER_NO();
        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVG_DrawImage(
    IN gcoVG Vg,
    IN gcoSURF Source,
    IN gcsPOINT_PTR SourceOrigin,
    IN gcsPOINT_PTR TargetOrigin,
    IN gcsSIZE_PTR SourceSize,
    IN gctINT SourceX,
    IN gctINT SourceY,
    IN gctINT TargetX,
    IN gctINT TargetY,
    IN gctINT Width,
    IN gctINT Height,
    IN gctBOOL Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x ",
                  Vg
                  );

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    do
    {
        gcsVG_RECT srcRect;
        gcsVG_RECT trgRect;

        srcRect.x      = SourceOrigin->x + SourceX;
        srcRect.y      = SourceOrigin->y + SourceY;
        srcRect.width  = Width;
        srcRect.height = Height;

        trgRect.x      = TargetOrigin->x + TargetX;
        trgRect.y      = TargetOrigin->y + TargetY;
        trgRect.width  = Width;
        trgRect.height = Height;

        /* Draw the image. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawImage(
            Vg->hw,
            &Source->info,
            &srcRect,
            &trgRect,
            gcvFILTER_POINT,
            Mask
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVG_TesselateImage(
    IN gcoVG Vg,
    IN gcoSURF Image,
    IN gcsVG_RECT_PTR Rectangle,
    IN gceIMAGE_FILTER Filter,
    IN gctBOOL Mask,
    IN gctBOOL SoftwareTesselation
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x Image=0x%x Rectangle=0x%x Filter=0x%x Mask=0x%x, SoftwareTesselation=0x%x",
                  Vg, Image, Rectangle,  Filter, Mask, SoftwareTesselation
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    do
    {
        static gctFLOAT userToSurface[6] =
        {
            1.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f
        };

        gcsTESSELATION_PTR tessellationBuffer;

        /* Get the pointer to the current tesselation buffer. */
        gcmERR_BREAK(_GetTessellationBuffer(
            Vg, &tessellationBuffer
            ));

        /* Program tesselation buffers. */
        gcmERR_BREAK(gcoVGHARDWARE_SetTessellation(
            Vg->hw,
            SoftwareTesselation,
            (gctUINT16)Vg->targetWidth, (gctUINT16)Vg->targetHeight,
            0.0f, 1.0f,
            userToSurface,
            tessellationBuffer
            ));

        /* Draw the image. */
        status = gcoVGHARDWARE_TesselateImage(
            Vg->hw,
            SoftwareTesselation,
            &Image->info,
            Rectangle,
            Filter,
            Mask,
            Vg->userToSurface,
            Vg->surfaceToImage,
            tessellationBuffer
            );
    
        if ((status != gcvSTATUS_OK) && !SoftwareTesselation)
        {
            /* Program tesselation buffers. */
            gcmERR_BREAK(gcoVGHARDWARE_SetTessellation(
                Vg->hw,
                gcvTRUE,
                (gctUINT16)Vg->targetWidth, (gctUINT16)Vg->targetHeight,
                0.0f, 1.0f,
                userToSurface,
                tessellationBuffer
                ));

            /* Draw the image. */
            gcmERR_BREAK(gcoVGHARDWARE_TesselateImage(
                Vg->hw,
                gcvTRUE,
                &Image->info,
                Rectangle,
                Filter,
                Mask,
                Vg->userToSurface,
                Vg->surfaceToImage,
                tessellationBuffer
            ));
    }
	}
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVG_Blit(
    IN gcoVG Vg,
    IN gcoSURF Source,
    IN gcoSURF Target,
    IN gcsVG_RECT_PTR SrcRect,
    IN gcsVG_RECT_PTR TrgRect,
    IN gceIMAGE_FILTER Filter,
    IN gceVG_BLEND Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x Source=0x%x Target=0x%x SrcRect=0x%x TrgRect=0x%x, Filter=0x%x Mode =0x%x",
                  Vg, Source, Target,  SrcRect, TrgRect, Filter, Mode
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Send to the hardware. */
    status = gcoVGHARDWARE_VgBlit(
        Vg->hw,
        &Source->info,
        &Target->info,
        SrcRect,
        TrgRect,
        Filter,
        gcvFALSE
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVG_ColorMatrix(
    IN gcoVG Vg,
    IN gcoSURF Source,
    IN gcoSURF Target,
    IN const gctFLOAT * Matrix,
    IN gceCHANNEL ColorChannels,
    IN gctBOOL FilterLinear,
    IN gctBOOL FilterPremultiplied,
    IN gcsPOINT_PTR SourceOrigin,
    IN gcsPOINT_PTR TargetOrigin,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x Source=0x%x Target=0x%x Matrix=0x%x ColorChannels=0x%x, FilterLinear=0x%x FilterPremultiplied =0x%x"
                    "SourceOrigin=0x%x  TargetOrigin=0x%x Width=0x%x Height=0x%x",
                  Vg, Source, Target,  Matrix, ColorChannels, FilterLinear, FilterPremultiplied, SourceOrigin,
                    TargetOrigin, Width, Height
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Execute color matrix. */
    status = gcoVGHARDWARE_ColorMatrix(
        Vg->hw,
        &Source->info,
        &Target->info,
        Matrix,
        ColorChannels,
        FilterLinear,
        FilterPremultiplied,
        SourceOrigin,
        TargetOrigin,
        Width, Height
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVG_SeparableConvolve(
    IN gcoVG Vg,
    IN gcoSURF Source,
    IN gcoSURF Target,
    IN gctINT KernelWidth,
    IN gctINT KernelHeight,
    IN gctINT ShiftX,
    IN gctINT ShiftY,
    IN const gctINT16 * KernelX,
    IN const gctINT16 * KernelY,
    IN gctFLOAT Scale,
    IN gctFLOAT Bias,
    IN gceTILE_MODE TilingMode,
    IN gctFLOAT_PTR FillColor,
    IN gceCHANNEL ColorChannels,
    IN gctBOOL FilterLinear,
    IN gctBOOL FilterPremultiplied,
    IN gcsPOINT_PTR SourceOrigin,
    IN gcsPOINT_PTR TargetOrigin,
    IN gcsSIZE_PTR SourceSize,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x Source=0x%x Target=0x%x",
                  Vg, Source, Target
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Execute color matrix. */
    status = gcoVGHARDWARE_SeparableConvolve(
        Vg->hw,
        &Source->info, &Target->info,
        KernelWidth, KernelHeight,
        ShiftX, ShiftY,
        KernelX, KernelY,
        Scale, Bias,
        TilingMode, FillColor,
        ColorChannels,
        FilterLinear,
        FilterPremultiplied,
        SourceOrigin,
        TargetOrigin,
        SourceSize,
        Width, Height
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVG_GaussianBlur(
    IN gcoVG Vg,
    IN gcoSURF Source,
    IN gcoSURF Target,
    IN gctFLOAT StdDeviationX,
    IN gctFLOAT StdDeviationY,
    IN gceTILE_MODE TilingMode,
    IN gctFLOAT_PTR FillColor,
    IN gceCHANNEL ColorChannels,
    IN gctBOOL FilterLinear,
    IN gctBOOL FilterPremultiplied,
    IN gcsPOINT_PTR SourceOrigin,
    IN gcsPOINT_PTR TargetOrigin,
    IN gcsSIZE_PTR SourceSize,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;


    gcmHEADER_ARG("Vg=0x%x Source=0x%x Target=0x%x",
                  Vg, Source, Target
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Execute color matrix. */
    status = gcoVGHARDWARE_GaussianBlur(
        Vg->hw,
        &Source->info, &Target->info,
        StdDeviationX, StdDeviationY,
        TilingMode, FillColor,
        ColorChannels,
        FilterLinear,
        FilterPremultiplied,
        SourceOrigin,
        TargetOrigin,
        SourceSize,
        Width, Height
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Set the image mode.
**
**  Set the image mode to be used by the next rendering.
**
**  @param[in]  Vg          Pointer to the gcoVG object.
**  @param[in]  Mode        Image mode.
*/
gceSTATUS
gcoVG_SetImageMode(
    IN gcoVG Vg,
    IN gceVG_IMAGE Mode
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Mode=0x%x ",
                  Vg, Mode
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set image mode in hardware. */
    status = gcoVGHARDWARE_SetVgImageMode(Vg->hw, Mode);
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVG_SetRenderingQuality(
    IN gcoVG Vg,
    IN gceRENDER_QUALITY Quality
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vg=0x%x Quality=0x%x ",
                  Vg, Quality
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Set the quality. */
    Vg->renderQuality = Quality;

    /* Update the setting in hardware. */
    status = gcoVGHARDWARE_SetRenderingQuality(Vg->hw, Quality);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVG_SetFillRule(
    IN gcoVG Vg,
    IN gceFILL_RULE FillRule
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x FillRule=0x%x ",
                  Vg, FillRule
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Update the setting in hardware. */
    status = gcoVGHARDWARE_SetFillRule(Vg->hw, FillRule);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/******************************************************************************/
/** @ingroup gcoVG
**
**  @brief   Enable or disable dither.
**
**  @param[in]  Vg      Pointer to the gcoVG object.
**  @param[in]  Enable  gcvTRUE to enable dither or gcvFALSE to disable
**                      dither.  dither is by default disabled.
*/
gceSTATUS
gcoVG_EnableDither(
    IN gcoVG Vg,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Vg=0x%x Enable=0x%x ",
                  Vg, Enable
                  );
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);

    /* Call gcoVGHARDWARE to enable scissoring. */
    status = gcoVGHARDWARE_EnableDither(Vg->hw, Enable);
    gcmFOOTER();
    return status;
}

#endif /* gcdENABLE_VG */
