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

#ifndef VIVANTE_NO_3D

#if gcdNULL_DRIVER < 2

#define _GC_OBJ_ZONE            gcvZONE_INDEX

/******************************************************************************\
********************************** Structures **********************************
\******************************************************************************/

/* Number of hashed ranges. */
#define gcvRANGEHASHNUMBER      16

/* Index buffer range information. */
typedef struct _gcsINDEXRANGE
{
    gctUINT32                   offset;
    gctUINT32                   count;
    gctUINT32                   minIndex;
    gctUINT32                   maxIndex;
}
* gcsINDEXRANGE_PTR;

/* Dynamic buffer management. */
typedef struct _gcsINDEX_DYNAMIC
{
    gctUINT32                   physical;
    gctUINT8_PTR                logical;
    gctSIGNAL                   signal;

    gctSIZE_T                   bytes;
    gctSIZE_T                   free;

    gctUINT32                   lastStart;
    gctUINT32                   lastEnd;

    gctUINT32                   minIndex;
    gctUINT32                   maxIndex;

    struct _gcsINDEX_DYNAMIC *  next;
}
* gcsINDEX_DYNAMIC_PTR;

/* gcoINDEX object. */
struct _gcoINDEX
{
    /* Object. */
    gcsOBJECT                   object;

    /* Number of bytes allocated for memory. */
    gctSIZE_T                   bytes;

    /* Index buffer range hash table. */
    struct _gcsINDEXRANGE       indexRange[gcvRANGEHASHNUMBER];

    /* Index buffer node. */
    gcsSURF_NODE                memory;

    /* Dynamic management. */
    gctUINT                     dynamicCount;
    gcsINDEX_DYNAMIC_PTR        dynamic;
    gcsINDEX_DYNAMIC_PTR        dynamicHead;
    gcsINDEX_DYNAMIC_PTR        dynamicTail;
};

/******************************************************************************\
******************************* gcoINDEX API Code ******************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoINDEX_Construct
**
**  Construct a new gcoINDEX object.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoINDEX * Index
**          Pointer to a variable that will receive the gcoINDEX object pointer.
*/
gceSTATUS
gcoINDEX_Construct(
    IN gcoHAL Hal,
    OUT gcoINDEX * Index
    )
{
    gcoOS os;
    gceSTATUS status;
    gcoINDEX index;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Index != gcvNULL);

    /* Extract the gcoOS object pointer. */
    os = gcvNULL;

    /* Allocate the gcoINDEX object. */
    gcmONERROR(gcoOS_Allocate(os,
                              gcmSIZEOF(struct _gcoINDEX),
                              &pointer));

    index = pointer;

    /* Initialize the gcoINDEX object. */
    index->object.type = gcvOBJ_INDEX;

    /* Reset ranage values. */
    gcmVERIFY_OK(
        gcoOS_ZeroMemory(&index->indexRange, gcmSIZEOF(index->indexRange)));

    /* No attributes and memory assigned yet. */
    index->bytes                 = 0;
    index->memory.pool           = gcvPOOL_UNKNOWN;
    index->memory.valid          = gcvFALSE;
    index->memory.lockCount      = 0;
    index->memory.lockedInKernel = gcvFALSE;
    index->dynamic               = gcvNULL;
    index->dynamicCount          = 0;

    /* Return pointer to the gcoINDEX object. */
    *Index = index;

    gcmPROFILE_GC(GLINDEX_OBJECT, 1);

    /* Success. */
    gcmFOOTER_ARG("*Index=0x%x", *Index);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Destroy
**
**  Destroy a gcoINDEX object.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_Destroy(
    IN gcoINDEX Index
    )
{
    gcsINDEX_DYNAMIC_PTR dynamic;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    gcmPROFILE_GC(GLINDEX_OBJECT, -1);

    if (Index->dynamic != gcvNULL)
    {
        /* Free all signal creations. */
        for (dynamic = Index->dynamicHead;
             dynamic != gcvNULL;
             dynamic = dynamic->next)
        {
            gcmVERIFY_OK(
                gcoOS_DestroySignal(gcvNULL, dynamic->signal));
        }

        /* Free the buffer structures. */
        gcmVERIFY_OK(
            gcmOS_SAFE_FREE(gcvNULL, Index->dynamic));

        /* No buffers allocated. */
        Index->dynamic = gcvNULL;
    }

    /* Free the index buffer. */
    gcmVERIFY_OK(gcoINDEX_Free(Index));

    /* Mark the gcoINDEX object as unknown. */
    Index->object.type = gcvOBJ_UNKNOWN;

    /* Free the gcoINDEX object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Index));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoINDEX_Lock
**
**  Lock index in memory.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Physical address of the index buffer.
**
**      gctPOINTER * Memory
**          Logical address of the index buffer.
*/
gceSTATUS
gcoINDEX_Lock(
    IN gcoINDEX Index,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* Do we have a buffer allocated? */
    if (Index->memory.pool == gcvPOOL_UNKNOWN)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Lock the index buffer. */
    gcmONERROR(gcoHARDWARE_Lock(&Index->memory,
                                Address,
                                Memory));

    /* Success. */
    gcmFOOTER_ARG("*Address=0x%08x *Memory=0x%x",
                  gcmOPT_VALUE(Address), gcmOPT_POINTER(Memory));
    return status;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Unlock
**
**  Unlock index that was previously locked with gcoINDEX_Lock.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoINDEX_Unlock(
    IN gcoINDEX Index
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* Do we have a buffer allocated? */
    if (Index->memory.pool == gcvPOOL_UNKNOWN)
    {
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Unlock the index buffer. */
    gcmONERROR(gcoHARDWARE_Unlock(&Index->memory,
                                  gcvSURF_INDEX));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Load
**
**  Upload index data into the memory.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**      gceINDEX_TYPE IndexType
**          Index type.
**
**      gctUINT32 IndexCount
**          Number of indices in the buffer.
**
**      gctPOINTER IndexBuffer
**          Pointer to the index buffer.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_Load(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctUINT32 IndexCount,
    IN gctPOINTER IndexBuffer
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE ioctl;
    gctUINT32 indexSize;
    gctUINT32 indexBufferSize;

    gcmHEADER_ARG("Index=0x%x IndexType=%d IndexCount=%u IndexBuffer=0x%x",
                  Index, IndexType, IndexCount, IndexBuffer);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(IndexCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(IndexBuffer != gcvNULL);

    /* Get index size from its type. */
    switch (IndexType)
    {
    case gcvINDEX_8:
        indexSize = 1;
        break;

    case gcvINDEX_16:
        indexSize = 2;
        break;

    case gcvINDEX_32:
        indexSize = 4;
        break;

    default:
        indexSize = 0;
        gcmASSERT(gcvFALSE);
    }

    /* Compute index buffer size. */
    indexBufferSize = indexSize * (IndexCount + 1);

    if (Index->bytes < indexBufferSize)
    {
        /* Free existing index buffer. */
        gcmONERROR(gcoINDEX_Free(Index));

        /* Allocate video memory. */
        ioctl.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        ioctl.u.AllocateLinearVideoMemory.bytes     = indexBufferSize;
        ioctl.u.AllocateLinearVideoMemory.alignment = 64;
        ioctl.u.AllocateLinearVideoMemory.type      = gcvSURF_INDEX;
        ioctl.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;

        /* Call kernel HAL. */
        gcmONERROR(gcoHAL_Call(gcvNULL, &ioctl));

        /* Store the buffer node. */
        Index->memory.pool
            = ioctl.u.AllocateLinearVideoMemory.pool;
        Index->memory.u.normal.node
            = ioctl.u.AllocateLinearVideoMemory.node;
        Index->memory.u.normal.cacheable = gcvFALSE;
        Index->bytes
            = ioctl.u.AllocateLinearVideoMemory.bytes;

        /* Lock the index buffer. */
        gcmONERROR(gcoHARDWARE_Lock(&Index->memory,
                                    gcvNULL,
                                    gcvNULL));
    }

    /* Upload the index buffer. */
    gcmONERROR(gcoINDEX_Upload(Index, IndexBuffer, indexBufferSize));

    /* Program index buffer states. */
    gcmONERROR(gcoHARDWARE_BindIndex(Index->memory.physical,
                                     IndexType));

    gcmPROFILE_GC(GLINDEX_OBJECT_BYTES, Index->bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Bind
**
**  Bind the index object to the hardware.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**      gceINDEX_TYPE IndexType
**          Index type.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_Bind(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    )
{
    gceSTATUS status;
    gctUINT32 address;

    gcmHEADER_ARG("Index=0x%x Type=%d", Index, Type);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    if (Index->dynamic != gcvNULL)
    {
        address = Index->dynamicHead->physical
                + Index->dynamicHead->lastStart;
    }
    else
    {
        address = Index->memory.physical;
    }

    /* Program index buffer states. */
    status = gcoHARDWARE_BindIndex(address,
                                   Type);
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_BindOffset
**
**  Bind the index object to the hardware.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**      gceINDEX_TYPE IndexType
**          Index type.
**
**      gctUINT32 Offset
**          Offset of stream.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_BindOffset(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctUINT32 Offset
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Type=%d Offset=%u", Index, Type, Offset);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* Program index buffer states. */
    status = gcoHARDWARE_BindIndex(Index->memory.physical + Offset,
                                   Type);
    gcmFOOTER();
    return status;
}

static gceSTATUS
_Free(
    IN gcoINDEX Index
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* Do we have a buffer allocated? */
    if (Index->memory.pool != gcvPOOL_UNKNOWN)
    {
        if (gcPLS.hal->dump != gcvNULL)
        {
            /* Dump deletion. */
            gcmVERIFY_OK(gcoDUMP_Delete(gcPLS.hal->dump,
                                        Index->memory.physical));
        }

        /* Unlock the index buffer. */
        gcmONERROR(gcoHARDWARE_Unlock(&Index->memory,
                                      gcvSURF_INDEX));

        /* Create an event to free the video memory. */
        gcmONERROR(gcoHARDWARE_ScheduleVideoMemory(&Index->memory));

        /* Reset range values. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(&Index->indexRange,
                                      gcmSIZEOF(Index->indexRange)));

        /* Reset the pointer. */
        Index->bytes        = 0;
        Index->memory.pool  = gcvPOOL_UNKNOWN;
        Index->memory.valid = gcvFALSE;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Free
**
**  Free existing index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_Free(
    IN gcoINDEX Index
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    gcmPROFILE_GC(GLINDEX_OBJECT_BYTES, -1 * Index->bytes);

    /* Not available when the gcoINDEX is dynamic. */
    if (Index->dynamic != gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Perform the free. */
    gcmONERROR(_Free(Index));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_Upload
**
**  Upload index data into an index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to a gcoINDEX object.
**
**      gctCONST_POINTER Buffer
**          Pointer to index data to upload.
**
**      gctSIZE_T Bytes
**          Number of bytes to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoINDEX_Upload(
    IN gcoINDEX Index,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    gcsHAL_INTERFACE ioctl;
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Buffer=0x%x Bytes=%lu", Index, Buffer, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* Not availabe with dynamic buffers. */
    if (Index->dynamic != gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Reset ranage values. */
    gcmVERIFY_OK(
        gcoOS_ZeroMemory(&Index->indexRange, gcmSIZEOF(Index->indexRange)));

    if (Index->bytes < Bytes)
    {
        /* Free any allocated video memory. */
        gcmONERROR(gcoINDEX_Free(Index));

        /* Allocate video memory. */
        ioctl.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        ioctl.u.AllocateLinearVideoMemory.bytes     = Bytes;
        ioctl.u.AllocateLinearVideoMemory.alignment = 4;
        ioctl.u.AllocateLinearVideoMemory.type      = gcvSURF_INDEX;
        ioctl.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;

        /* Call the kernel. */
        gcmONERROR(gcoHAL_Call(gcvNULL, &ioctl));

        /* Initialize index. */
        Index->bytes                = Bytes;
        Index->memory.pool          = ioctl.u.AllocateLinearVideoMemory.pool;
        Index->memory.u.normal.node = ioctl.u.AllocateLinearVideoMemory.node;
        Index->memory.u.normal.cacheable = gcvFALSE;

        /* Lock the index buffer. */
        gcmONERROR(gcoHARDWARE_Lock(&Index->memory,
                                    gcvNULL,
                                    gcvNULL));
    }

    if (Buffer != gcvNULL)
    {
        /* Upload data into the stream. */
        gcmONERROR(gcoHARDWARE_CopyData(&Index->memory,
                                        0,
                                        Buffer,
                                        Bytes));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       "index",
                       Index->memory.physical,
                       Index->memory.logical,
                       0,
                       Bytes);

        if (gcPLS.hal->dump != gcvNULL)
        {
            /* Dump the index buffer. */
            gcmVERIFY_OK(gcoDUMP_DumpData(gcPLS.hal->dump,
                                          gcvTAG_INDEX,
                                          Index->memory.physical,
                                          Bytes,
                                          Buffer));
        }
    }

    gcmPROFILE_GC(GLINDEX_OBJECT_BYTES, Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_UploadOffset
**
**  Upload index data into an index buffer at an offset.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to a gcoINDEX object.
**
**      gctUINT32 Offset
**          Offset into gcoINDEX buffer to start uploading.
**
**      gctCONST_POINTER Buffer
**          Pointer to index data to upload.
**
**      gctSIZE_T Bytes
**          Number of bytes to upload.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoINDEX_UploadOffset(
    IN gcoINDEX Index,
    IN gctUINT32 Offset,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Offset=%u Buffer=0x%x Bytes=%lu",
                  Index, Offset, Buffer, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    /* Not availabe with dynamic buffers. */
    if (Index->dynamic != gcvNULL)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_REQUEST);
        return gcvSTATUS_INVALID_REQUEST;
    }

    if (Offset + Bytes > Index->bytes)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_BUFFER_TOO_SMALL);
        return gcvSTATUS_BUFFER_TOO_SMALL;
    }

    if (Buffer != gcvNULL)
    {
        /* Reset ranage values. */
        gcoOS_ZeroMemory(&Index->indexRange, sizeof(Index->indexRange));

        /* Upload data into the stream. */
        status = gcoHARDWARE_CopyData(&Index->memory,
                                      Offset,
                                      Buffer,
                                      Bytes);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       "index",
                       Index->memory.physical,
                       Index->memory.logical,
                       Offset,
                       Bytes);

        if (gcPLS.hal->dump != gcvNULL)
        {
            /* Dump the index buffer. */
            gcmVERIFY_OK(gcoDUMP_DumpData(
                gcPLS.hal->dump,
                gcvTAG_INDEX,
                Index->memory.physical + Offset,
                Bytes,
                Buffer
                ));
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoINDEX_QueryCaps
**
**  Query the index capabilities.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      gctBOOL * Index8
**          Pointer to a variable receiving the capability for 8-bit indices.
**
**      gctBOOL * Index16
**          Pointer to a variable receiving the capability for 16-bit indices.
**          target.
**
**      gctBOOL * Index32
**          Pointer to a variable receiving the capability for 32-bit indices.
**
**      gctUINT * MaxIndex
**          Maximum number of indices.
*/
gceSTATUS
gcoINDEX_QueryCaps(
    OUT gctBOOL * Index8,
    OUT gctBOOL * Index16,
    OUT gctBOOL * Index32,
    OUT gctUINT * MaxIndex
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Route to gcoHARDWARE. */
    status = gcoHARDWARE_QueryIndexCaps(Index8, Index16, Index32, MaxIndex);
    gcmFOOTER_ARG("status=%d(%s) *Index8=%d *Index16=%d *Index32=%d *MaxIndex=%u",
                  status, gcoOS_DebugStatus2Name(status), gcmOPT_VALUE(Index8), gcmOPT_VALUE(Index16),
                  gcmOPT_VALUE(Index32), gcmOPT_VALUE(MaxIndex));
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_GetIndexRange
**
**  Determine the index range in the current index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that needs to be destroyed.
**
**      gceINDEX_TYPE Type
**          Index type.
**
**      gctUINT32 Offset
**          Offset into the buffer.
**
**      gctUINT32 Count
**          Number of indices to analyze.
**
**  OUTPUT:
**
**      gctUINT32 * MinimumIndex
**          Pointer to a variable receiving the minimum index value in
**          the index buffer.
**
**      gctUINT32 * MaximumIndex
**          Pointer to a variable receiving the maximum index value in
**          the index buffer.
*/
gceSTATUS
gcoINDEX_GetIndexRange(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctUINT32 Offset,
    IN gctUINT32 Count,
    OUT gctUINT32 * MinimumIndex,
    OUT gctUINT32 * MaximumIndex
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Type=%d Offset=%u Count=%u",
                  Index, Type, Offset, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(MinimumIndex != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(MaximumIndex!= gcvNULL);

    do
    {
        /* Determine the hash table entry. */
        gctUINT32 index = gcoMATH_ModuloInt(Offset * 31 + Count + 31,
                                            gcvRANGEHASHNUMBER);

        /* Shortcut to the entry. */
        gcsINDEXRANGE_PTR entry = &Index->indexRange[index];

        /* Not yet determined? */
        if ((entry->offset != Offset) ||
            (entry->count  != Count))
        {
            gctUINT32 minIndex = ~0U;
            gctUINT32 maxIndex =  0;

            /* Must have buffer. */
            if (Index->memory.pool == gcvPOOL_UNKNOWN)
            {
                status = gcvSTATUS_GENERIC_IO;
                break;
            }

            /* Assume no error. */
            status = gcvSTATUS_OK;

            /* Determine the range. */
            switch (Type)
            {
            case gcvINDEX_8:
                {
                    gctSIZE_T i;

                    gctUINT8_PTR indexBuffer
                        = Index->memory.logical
                        + Offset;

                    if (Offset + Count > Index->bytes)
                    {
                        status = gcvSTATUS_INVALID_ARGUMENT;
                        break;
                    }

                    for (i = 0; i < Count; i++)
                    {
                        gctUINT32 curIndex = *indexBuffer++;

                        if (curIndex < minIndex)
                        {
                            minIndex = curIndex;
                        }

                        if (curIndex > maxIndex)
                        {
                            maxIndex = curIndex;
                        }
                    }
                }
                break;

            case gcvINDEX_16:
                {
                    gctSIZE_T i;

                    gctUINT16_PTR indexBuffer
                        = (gctUINT16_PTR)(Index->memory.logical
                        + Offset);

                    if (Offset + Count * 2 > Index->bytes)
                    {
                        status = gcvSTATUS_INVALID_ARGUMENT;
                        break;
                    }

                    for (i = 0; i < Count; i++)
                    {
                        gctUINT32 curIndex = *indexBuffer++;

                        if (curIndex < minIndex)
                        {
                            minIndex = curIndex;
                        }

                        if (curIndex > maxIndex)
                        {
                            maxIndex = curIndex;
                        }
                    }
                }
                break;

            case gcvINDEX_32:
                {
                    gctSIZE_T i;

                    gctUINT32_PTR indexBuffer
                        = (gctUINT32_PTR)(Index->memory.logical
                        + Offset);

                    if (Offset + Count * 4 > Index->bytes)
                    {
                        status = gcvSTATUS_INVALID_ARGUMENT;
                        break;
                    }

                    for (i = 0; i < Count; i++)
                    {
                        gctUINT32 curIndex = *indexBuffer++;

                        if (curIndex < minIndex)
                        {
                            minIndex = curIndex;
                        }

                        if (curIndex > maxIndex)
                        {
                            maxIndex = curIndex;
                        }
                    }
                }
                break;
            }

            /* Set entry members. */
            entry->offset   = Offset;
            entry->count    = Count;
            entry->minIndex = minIndex;
            entry->maxIndex = maxIndex;
        }
        else
        {
            status = gcvSTATUS_OK;
        }

        /* Set results. */
        *MinimumIndex = entry->minIndex;
        *MaximumIndex = entry->maxIndex;
    }
    while (gcvFALSE);

    /* Return status. */
    if (gcmIS_SUCCESS(status))
    {
        gcmFOOTER_ARG("*MinimumIndex=%u *MaximumIndex=%u",
                      *MinimumIndex, *MaximumIndex);
    }
    else
    {
        gcmFOOTER();
    }
    return status;
}

/*******************************************************************************
*************************** DYNAMIC BUFFER MANAGEMENT **************************
*******************************************************************************/

static gceSTATUS
gcoINDEX_AllocateMemory(
    IN gcoINDEX Index,
    IN gctSIZE_T Bytes,
    IN gctUINT32 Buffers
    )
{
    gctSIZE_T bytes;
    gcsHAL_INTERFACE ioctl;
    gctUINT32 physical;
    gctPOINTER logical;
    gctUINT32 i;
    gcsINDEX_DYNAMIC_PTR dynamic;
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Bytes=%lu Buffers=%u", Index, Bytes, Buffers);

    /* Free any memory. */
    gcmONERROR(_Free(Index));
    Index->dynamic->bytes = 0;

    /* Compute the number of total bytes. */
    bytes = gcmALIGN(Bytes, 64) * Buffers;

    /* Allocate the video memory. */
    ioctl.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
    ioctl.u.AllocateLinearVideoMemory.bytes     = bytes;
    ioctl.u.AllocateLinearVideoMemory.alignment = 64;
    ioctl.u.AllocateLinearVideoMemory.type      = gcvSURF_INDEX;
    ioctl.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;

    /* Call the kernel. */
    gcmONERROR(gcoHAL_Call(gcvNULL, &ioctl));

    /* Initialize index. */
    Index->bytes                = ioctl.u.AllocateLinearVideoMemory.bytes;
    Index->memory.pool          = ioctl.u.AllocateLinearVideoMemory.pool;
    Index->memory.u.normal.node = ioctl.u.AllocateLinearVideoMemory.node;
    Index->memory.u.normal.cacheable = gcvFALSE;

    /* Lock the index buffer. */
    gcmONERROR(gcoHARDWARE_Lock(&Index->memory,
                                &physical,
                                &logical));

    bytes = gcoMATH_DivideUInt(Index->bytes, Buffers);

    /* Initialize all buffer structures. */
    for (i = 0, dynamic = Index->dynamic; i < Buffers; ++i, ++dynamic)
    {
        /* Set buffer address. */
        dynamic->physical = physical;
        dynamic->logical  = logical;

        /* Set buffer size. */
        dynamic->bytes = bytes;
        dynamic->free  = bytes;

        /* Set usage. */
        dynamic->lastStart = ~0U;
        dynamic->lastEnd   = 0;

        /* Advance buffer addresses. */
        physical += bytes;
        logical   = (gctUINT8_PTR) logical + bytes;
    }

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
**  gcoINDEX_SetDynamic
**
**  Mark the gcoINDEX object as dynamic.  A dynamic object will allocate the
**  specified number of buffers and upload data after the previous upload.  This
**  way there is no need for synchronizing the GPU or allocating new objects
**  each time an index buffer is required.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that needs to be converted to dynamic.
**
**      gctSIZE_T Bytes
**          Number of bytes per buffer.
**
**      gctUINT Buffers
**          Number of buffers.
*/
gceSTATUS
gcoINDEX_SetDynamic(
    IN gcoINDEX Index,
    IN gctSIZE_T Bytes,
    IN gctUINT Buffers
    )
{
    gceSTATUS status;
    gctUINT i;
    gcsINDEX_DYNAMIC_PTR dynamic;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Index=0x%x Bytes=%lu Buffers=%u", Index, Bytes, Buffers);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Buffers > 0);

    /* We can only do this once. */
    if (Index->dynamic != gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Allocate memory for the buffer structures. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              Buffers * gcmSIZEOF(struct _gcsINDEX_DYNAMIC),
                              &pointer));

    Index->dynamic = pointer;

    gcmONERROR(gcoOS_ZeroMemory(Index->dynamic,
                                Buffers * gcmSIZEOF(struct _gcsINDEX_DYNAMIC)));

    /* Initialize all buffer structures. */
    for (i = 0, dynamic = Index->dynamic; i < Buffers; ++i, ++dynamic)
    {
        /* Create the signal. */
        gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvTRUE, &dynamic->signal));

        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_SIGNAL,
            "%s(%d): index buffer %d signal created 0x%08X",
            __FUNCTION__, __LINE__,
            i, dynamic->signal);

        /* Mark buffer as usuable. */
        gcmONERROR(gcoOS_Signal(gcvNULL, dynamic->signal, gcvTRUE));

        /* Link buffer in chain. */
        dynamic->next = dynamic + 1;
    }

    /* Initilaize chain of buffer structures. */
    Index->dynamicCount      = Buffers;
    Index->dynamicHead       = Index->dynamic;
    Index->dynamicTail       = Index->dynamic + Buffers - 1;
    Index->dynamicTail->next = gcvNULL;

    /* Allocate the memory. */
    gcmONERROR(gcoINDEX_AllocateMemory(Index, Bytes, Buffers));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Roll back allocation of buffer structures. */
    if (Index->dynamic != gcvNULL)
    {
        /* Roll back all signal creations. */
        for (i = 0; i < Buffers; ++i)
        {
            if (Index->dynamic[i].signal != gcvNULL)
            {
                gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL,
                                                 Index->dynamic[i].signal));
            }
        }

        /* Roll back the allocation of the buffer structures. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Index->dynamic));

        /* No buffers allocated. */
        Index->dynamic = gcvNULL;
    }

    /* Roll back memory allocation. */
    gcmVERIFY_OK(gcoINDEX_Free(Index));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_UploadDynamic
**
**  Upload data into a dynamic index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that has been configured as dynamic.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer containing the data to upload.
**
**      gctSIZE_T Bytes
**          Number of bytes of data to upload.
*/
gceSTATUS
gcoINDEX_UploadDynamic(
    IN gcoINDEX Index,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE ioctl;
    gcsINDEX_DYNAMIC_PTR dynamic;

    gcmHEADER_ARG("Index=0x%x Data=0x%x Bytes=%lu", Index, Data, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    if (Index->dynamic == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Shorthand the pointer. */
    dynamic = Index->dynamicHead;
    gcmASSERT(dynamic != gcvNULL);

    /* Make sure the dynamic index buffer is large enough to hold this data. */
    if (Bytes > dynamic->bytes)
    {
        gcmONERROR(gcvSTATUS_DATA_TOO_LARGE);
    }

    if (dynamic->free < gcmALIGN(Bytes, 4))
    {
        /* Not enough free bytes in this buffer, mark it busy. */
        gcmONERROR(gcoOS_Signal(gcvNULL,
                                dynamic->signal,
                                gcvFALSE));

        /* Schedule a signal event. */
        ioctl.command            = gcvHAL_SIGNAL;
        ioctl.u.Signal.signal    = dynamic->signal;
        ioctl.u.Signal.auxSignal = gcvNULL;
        ioctl.u.Signal.process   = gcoOS_GetCurrentProcessID();
        ioctl.u.Signal.fromWhere = gcvKERNEL_COMMAND;
        gcmONERROR(gcoHARDWARE_CallEvent(&ioctl));

        /* Commit the buffer. */
        gcmONERROR(gcoHARDWARE_Commit());

        /* Move it to the tail of the queue. */
        gcmASSERT(Index->dynamicTail != gcvNULL);
        Index->dynamicTail->next = dynamic;
        Index->dynamicTail       = dynamic;
        Index->dynamicHead       = dynamic->next;
        gcmASSERT(Index->dynamicHead != gcvNULL);

        /* Reinitialize the top of the queue. */
        dynamic            = Index->dynamicHead;
        dynamic->free      = dynamic->bytes;
        dynamic->lastStart = ~0U;
        dynamic->lastEnd   = 0;

        /* Wait for the top of the queue to become available. */
        gcmONERROR(gcoOS_WaitSignal(gcvNULL,
                                    dynamic->signal,
                                    gcPLS.hal->timeOut));
    }

    /* Copy the index data. */
    gcmONERROR(gcoOS_MemCopy(dynamic->logical + dynamic->lastEnd,
                             Data,
                             Bytes));
    /* Flush the cache. */
    gcmONERROR(gcoSURF_NODE_Cache(&Index->memory,
                                dynamic->logical + dynamic->lastEnd,
                                Bytes,
                                gcvCACHE_CLEAN));

    /* Update the pointers. */
    dynamic->lastStart = dynamic->lastEnd;
    dynamic->lastEnd   = dynamic->lastStart + Bytes;
    dynamic->free     -= gcmALIGN(Bytes, 4);

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   "index",
                   dynamic->physical,
                   dynamic->logical,
                   dynamic->lastStart,
                   Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status.*/
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_UploadDynamicEx
**
**  Upload data into a dynamic index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that has been configured as dynamic.
**
**      gceINDEX_TYPE IndexType
**          Type of index data to upload.
**
**      gctCONST_POINTER Data
**          Pointer to a buffer containing the data to upload.
**
**      gctSIZE_T Bytes
**          Number of bytes of data to upload.
*/
gceSTATUS
gcoINDEX_UploadDynamicEx(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE ioctl;
    gcsINDEX_DYNAMIC_PTR dynamic;
    gctUINT32 iMin = ~0U, iMax = 0;
    gctUINT8_PTR src, dest;
    gctSIZE_T aligned, bytes;

    gcmHEADER_ARG("Index=0x%x IndexType=%d Data=0x%x Bytes=%lu",
                  Index, IndexType, Data, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(Data != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Bytes > 0);

    if (Index->dynamic == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Shorthand the pointer. */
    dynamic = Index->dynamicHead;
    gcmASSERT(dynamic != gcvNULL);

    /* Make sure the dynamic index buffer is large enough to hold this data. */
    if (Bytes > dynamic->bytes)
    {
        /* Reallocate the index buffers. */
        gcmONERROR(gcoINDEX_AllocateMemory(Index,
                                           gcmALIGN(Bytes * 2, 4 << 10),
                                           Index->dynamicCount));
    }

    /* Compute number of aligned bytes. */
    aligned = gcmALIGN(Bytes, 4);

    if (dynamic->free < aligned)
    {
        /* Not enough free bytes in this buffer, mark it busy. */
        gcmONERROR(gcoOS_Signal(gcvNULL,
                                dynamic->signal,
                                gcvFALSE));

        /* Schedule a signal event. */
        ioctl.command            = gcvHAL_SIGNAL;
        ioctl.u.Signal.signal    = dynamic->signal;
        ioctl.u.Signal.auxSignal = gcvNULL;
        ioctl.u.Signal.process   = gcoOS_GetCurrentProcessID();
        ioctl.u.Signal.fromWhere = gcvKERNEL_COMMAND;
        gcmONERROR(gcoHARDWARE_CallEvent(&ioctl));

        /* Commit the buffer. */
        gcmONERROR(gcoHARDWARE_Commit());

        /* Move it to the tail of the queue. */
        gcmASSERT(Index->dynamicTail != gcvNULL);
        Index->dynamicTail->next = dynamic;
        Index->dynamicTail       = dynamic;
        Index->dynamicHead       = dynamic->next;
        dynamic->next            = gcvNULL;
        gcmASSERT(Index->dynamicHead != gcvNULL);

        /* Reinitialize the top of the queue. */
        dynamic            = Index->dynamicHead;
        dynamic->free      = dynamic->bytes;
        dynamic->lastStart = ~0U;
        dynamic->lastEnd   = 0;

        status = gcoOS_WaitSignal(gcvNULL, dynamic->signal, 0);
        if (status == gcvSTATUS_TIMEOUT)
        {
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_INDEX,
                          "Waiting for index buffer 0x%x",
                          dynamic);

            /* Wait for the top of the queue to become available. */
            gcmONERROR(gcoOS_WaitSignal(gcvNULL,
                                        dynamic->signal,
                                        gcvINFINITE));
        }
    }

    /* Setup pointers for copying data. */
    src  = (gctUINT8_PTR) Data;
    dest = dynamic->logical + dynamic->lastEnd;

    /* Dispatch on index type. */
    switch (IndexType)
    {
    case gcvINDEX_8:
        /* Copy all indices as 8-bit data. */
        for (bytes = Bytes; bytes >= 1; --bytes)
        {
            gctUINT8 index = *(gctUINT8_PTR) src++;
            *dest++ = index;

            /* Keep track of min/max index. */
            if (index < iMin) iMin = index;
            if (index > iMax) iMax = index;
        }
        break;

    case gcvINDEX_16:
        /* Copy all indices as 16-bit data. */
        for (bytes = Bytes; bytes >= 2; bytes -= 2)
        {
            gctUINT16 index = *(gctUINT16_PTR) src;
            src += 2;
            *(gctUINT16_PTR) dest = index;
            dest += 2;

            /* Keep track of min/max index. */
            if (index < iMin) iMin = index;
            if (index > iMax) iMax = index;
        }
        break;

    case gcvINDEX_32:
        /* Copy all indices as 32-bit data. */
        for (bytes = Bytes; bytes >= 4; bytes -= 4)
        {
            gctUINT32 index = *(gctUINT32_PTR) src;
            src += 4;
            *(gctUINT32_PTR) dest = index;
            dest += 4;

            /* Keep track of min/max index. */
            if (index < iMin) iMin = index;
            if (index > iMax) iMax = index;
        }
        break;
    }

    /* Flush the cache. */
    gcmONERROR(gcoSURF_NODE_Cache(&Index->memory,
                                dynamic->logical + dynamic->lastEnd,
                                Bytes,gcvCACHE_CLEAN));

    /* Update the pointers. */
    dynamic->lastStart = dynamic->lastEnd;
    dynamic->lastEnd   = dynamic->lastStart + aligned;
    dynamic->free     -= aligned;
    dynamic->minIndex  = iMin;
    dynamic->maxIndex  = iMax;

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   "index",
                   dynamic->physical,
                   dynamic->logical,
                   dynamic->lastStart,
                   Bytes);

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status.*/
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_BindDynamic
**
**  Bind the index object to the hardware.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object.
**
**      gceINDEX_TYPE IndexType
**          Index type.
**
**  OUTPUT:
**
**      Nothing
*/
gceSTATUS
gcoINDEX_BindDynamic(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x Type=%d", Index, Type);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);

    /* This only works on dynamic buffers. */
    if (Index->dynamic == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Program index buffer states. */
    gcmONERROR(gcoHARDWARE_BindIndex(( Index->dynamicHead->physical
                                     + Index->dynamicHead->lastStart
                                     ),
                                     Type));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoINDEX_GetIndexRangeDynamic
**
**  Determine the index range in the current dynamic index buffer.
**
**  INPUT:
**
**      gcoINDEX Index
**          Pointer to an gcoINDEX object that needs to be destroyed.
**
**  OUTPUT:
**
**      gctUINT32 * MinimumIndex
**          Pointer to a variable receiving the minimum index value in
**          the index buffer.
**
**      gctUINT32 * MaximumIndex
**          Pointer to a variable receiving the maximum index value in
**          the index buffer.
*/
gceSTATUS
gcoINDEX_GetDynamicIndexRange(
    IN gcoINDEX Index,
    OUT gctUINT32 * MinimumIndex,
    OUT gctUINT32 * MaximumIndex
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Index=0x%x ", Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Index, gcvOBJ_INDEX);
    gcmDEBUG_VERIFY_ARGUMENT(MinimumIndex != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(MaximumIndex!= gcvNULL);

    /* This only works for dynamic index buffers. */
    if (Index->dynamic == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Dynamic index buffers get checked during upload. */
    *MinimumIndex = Index->dynamicHead->minIndex;
    *MaximumIndex = Index->dynamicHead->maxIndex;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS gcoINDEX_Construct(
    IN gcoHAL Hal,
    OUT gcoINDEX * Index
    )
{
    *Index = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Destroy(
    IN gcoINDEX Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Lock(
    IN gcoINDEX Index,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    *Address = 0;
    *Memory = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Unlock(
    IN gcoINDEX Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Load(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctUINT32 IndexCount,
    IN gctPOINTER IndexBuffer
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Bind(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_BindOffset(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctUINT32 Offset
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Free(
    IN gcoINDEX Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_Upload(
    IN gcoINDEX Index,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_UploadOffset(
    IN gcoINDEX Index,
    IN gctUINT32 Offset,
    IN gctCONST_POINTER Buffer,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_QueryCaps(
    OUT gctBOOL * Index8,
    OUT gctBOOL * Index16,
    OUT gctBOOL * Index32,
    OUT gctUINT * MaxIndex
    )
{
    *Index8 = gcvTRUE;
    *Index16 = gcvTRUE;
    *Index32 = gcvTRUE;
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_GetIndexRange(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type,
    IN gctUINT32 Offset,
    IN gctUINT32 Count,
    OUT gctUINT32 * MinimumIndex,
    OUT gctUINT32 * MaximumIndex
    )
{
    *MinimumIndex = 0;
    *MaximumIndex = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_SetDynamic(
    IN gcoINDEX Index,
    IN gctSIZE_T Bytes,
    IN gctUINT Buffers
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_UploadDynamic(
    IN gcoINDEX Index,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_UploadDynamicEx(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE IndexType,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_BindDynamic(
    IN gcoINDEX Index,
    IN gceINDEX_TYPE Type
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoINDEX_GetDynamicIndexRange(
    IN gcoINDEX Index,
    OUT gctUINT32 * MinimumIndex,
    OUT gctUINT32 * MaximumIndex
    )
{
    *MinimumIndex = 0;
    *MaximumIndex = 0;
    return gcvSTATUS_OK;
}

#endif /* gcdNULL_DRIVER < 2 */
#endif /* VIVANTE_NO_3D */
