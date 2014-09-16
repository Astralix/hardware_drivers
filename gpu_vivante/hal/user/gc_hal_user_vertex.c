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

#define _GC_OBJ_ZONE            gcvZONE_STREAM
#define gcdDEBUG_REBUILD        0

#define ENABLE_STREAM_CACHE_STATIC     0
#define OPT_CACHE_USAGE                1
/*******************************************************************************
**  Stream Cache
**
**  The stream cache works as a dynamic stream with the addition of caching
**  previous vertices.  The cache is a simple lookup of vertex array attributes,
**  which contain the information about the attributes of the stream.
**
**  To speed up the lookup, a has table is used as well.
**
**  The cache will have multiple buffers and validate all of them for any vertex
**  stream coming in.  Ince the last buffer of the cache starts being used as
**  the cache, the oldest buffer will be flushed and marked as free
**  asynchronously by the hardware
*/
#define gcdSTREAM_CACHE_SLOTS   2048
#define gcdSTREAM_CACHE_HASH    8192
#define gcdSTREAM_CACHE_SIZE    (1024 << 10)
#define gcdSTREAM_CACHE_COUNT   5

typedef enum _gceSTREAM_CACHE_TYPE
{
    /* Slot of free. */
    gcvSTREAM_CACHE_FREE = 0,

    /* Attributes are just copied in. */
    gcvSTREAM_CACHE_COPIED,

    /* Attributes are static. */
    gcvSTREAM_CACHE_STATIC,

    /* Attributes are dynamic. */
    gcvSTREAM_CACHE_DYNAMIC,
}
gceSTREAM_CACHE_TYPE;

typedef struct _gcsSTREAM_CACHE     gcsSTREAM_CACHE;
typedef struct _gcsSTREAM_CACHE *   gcsSTREAM_CACHE_PTR;
struct _gcsSTREAM_CACHE
{
    /* Type of the attributes. */
    gceSTREAM_CACHE_TYPE        type;

    /* Number of attributes in cache. */
    gctUINT                     attributeCount;

    /* Attributes. */
    gcsVERTEXARRAY_ATTRIBUTE    attributes[gcdATTRIBUTE_COUNT];

    /* Offset into the stream. */
    gctUINT                     offset;

    /* Stride of the stream. */
    gctUINT                     stride;

    /* Valid match countdown. */
    gctUINT                     countdown;

    /* CRC value of the data. */
    gctUINT32                   crc32;

    /* Hash key. */
    gctUINT                     key;

    /* Pointer to next hash entry. */
    gcsSTREAM_CACHE_PTR         next;

    /* Number of bytes. */
    gctSIZE_T                   bytes;
};

typedef struct _gcsSTREAM_CACHE_BUFFER
{
    /* Signal for cache. */
    gctSIGNAL                   signal;

    /* Allocated video memory node. */
    gcsSURF_NODE                node;

    /* Size of the cache. */
    gctUINT                     bytes;

    /* Offset to next free byte in the cache. */
    gctUINT                     offset;

    /* Number of free bytes in the cache. */
    gctUINT                     free;

    /* Index into cacheArray of next free entry. */
    gctUINT                     index;

    /* The array of cached attributes. */
    gcsSTREAM_CACHE             cacheArray[gcdSTREAM_CACHE_SLOTS];

    /* The hash table for the cached attributes. */
    gcsSTREAM_CACHE_PTR         cacheHash[gcdSTREAM_CACHE_HASH];
}
gcsSTREAM_CACHE_BUFFER,
* gcsSTREAM_CACHE_BUFFER_PTR;

typedef struct _gcsSTREAM_RANGE
{
    gctUINT32   stream;
    gctUINT32   start;
    gctUINT32   size;
    gctUINT32   end;
}
gcsSTREAM_RANGE;

/* Dynamic buffer management. */
typedef struct _gcsSTREAM_DYNAMIC
{
    gctUINT32                   physical;
    gctUINT8_PTR                logical;
    gctSIGNAL                   signal;

    gctSIZE_T                   bytes;
    gctSIZE_T                   free;

    gctUINT32                   lastStart;
    gctUINT32                   lastEnd;

    struct _gcsSTREAM_DYNAMIC * next;
}
* gcsSTREAM_DYNAMIC_PTR;

/**
 * gcoSTREAM object defintion.
 */
struct _gcoSTREAM
{
    /* The object. */
    gcsOBJECT                   object;

    /* Allocated node for stream. */
    gcsSURF_NODE                node;

    /* Size of allocated memory for stream. */
    gctSIZE_T                   size;

    /* Stride of the stream. */
    gctUINT32                   stride;

    /* Dynamic buffer management. */
    gctUINT32                   lastStart;
    gctUINT32                   lastEnd;

    /* Info for stream rebuilding. */
    gcoSTREAM                   rebuild;
    gcsSTREAM_RANGE             mapping[gcdATTRIBUTE_COUNT];

    /* Dynamic management. */
    gcsSTREAM_DYNAMIC_PTR       dynamic;
    gcsSTREAM_DYNAMIC_PTR       dynamicHead;
    gcsSTREAM_DYNAMIC_PTR       dynamicTail;

    /* New substream management. */
    gctUINT                     subStreamCount;
    gctUINT                     subStreamStride;
    gcsSTREAM_SUBSTREAM         subStreams[256];

    gcoSTREAM                   merged;
    gctBOOL                     dirty;
    gcsATOM_PTR                 reference;
    gctUINT                     count;

    /***** Stream Cache *******************************************************/

    /* Number of stream caches. */
    gctUINT                     cacheCount;

    /* Current stream cache. */
    gctUINT                     cacheCurrent;
    gctUINT                     cacheLastHit;

    /* The cached streams. */
    gcsSTREAM_CACHE_BUFFER_PTR  cache;
};

/**
 * Static function that unlocks and frees the allocated
 * memory attached to the gcoSTREAM object.
 *
 * @param Stream Pointer to an initialized gcoSTREAM object.
 *
 * @return Status of the destruction of the memory allocated for the
 *         gcoSTREAM.
 */
static gceSTATUS
_FreeMemory(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status;

    do
    {
        /* See if the stream memory is locked or not. */
        if (Stream->node.logical != gcvNULL)
        {
            /* Dump deletion. */
            if (gcPLS.hal->dump != gcvNULL)
            {
                gcmVERIFY_OK(gcoDUMP_Delete(
                    gcPLS.hal->dump,
                    Stream->node.physical
                    ));
            }

            /* Unlock the stream memory. */
            gcmERR_BREAK(gcoHARDWARE_Unlock(
                &Stream->node,
                gcvSURF_VERTEX
                ));

            /* Mark the stream memory as unlocked. */
            Stream->node.logical = gcvNULL;
        }

        /* See if the stream memory is allocated. */
        if (Stream->node.pool != gcvPOOL_UNKNOWN)
        {
            /* Free the allocated video memory asynchronously. */
            gcmERR_BREAK(gcoHARDWARE_ScheduleVideoMemory(
                &Stream->node
                ));

            /* Mark the stream memory as unallocated. */
            Stream->node.pool  = gcvPOOL_UNKNOWN;
            Stream->node.valid = gcvFALSE;
        }

        if (Stream->rebuild != gcvNULL)
        {
            gcmERR_BREAK(gcoSTREAM_Destroy(Stream->rebuild));
            Stream->rebuild = gcvNULL;
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    return status;
}

/**
 * Construct a new gcoSTREAM object.
 *
 * @param Hal    Pointer to an initialized gcoHAL object.
 * @param Stream Pointer to a variable receiving the gcSTERAM object
 *               pointer on success.
 *
 * @return The status of the gcoSTREAM object construction.
 */
gceSTATUS
gcoSTREAM_Construct(
    IN gcoHAL Hal,
    OUT gcoSTREAM * Stream
    )
{
    gcoSTREAM stream;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Stream != gcvNULL);

    do
    {
        /* Allocate the object structure. */
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(struct _gcoSTREAM),
                                    &pointer));

        stream = pointer;

        /* Initialize the object. */
        stream->object.type = gcvOBJ_STREAM;

        /* Initialize the fields. */
        stream->node.pool           = gcvPOOL_UNKNOWN;
        stream->node.valid          = gcvFALSE;
        stream->node.logical        = gcvNULL;
        stream->node.lockCount      = 0;
        stream->node.lockedInKernel = 0;
        stream->size                = 0;
        stream->stride              = 0;
        stream->lastStart           = 0;
        stream->lastEnd             = 0;
        stream->rebuild             = gcvNULL;
        stream->dynamic             = gcvNULL;
        gcoOS_ZeroMemory(stream->mapping, gcmSIZEOF(stream->mapping));
        stream->subStreamCount      = 0;
        stream->cache               = gcvNULL;

        stream->merged              = gcvNULL;
        stream->dirty               = gcvFALSE;
        stream->reference           = gcvNULL;
        stream->count               = 0;

        /* Return the object pointer. */
        *Stream = stream;

        /* Success. */
        gcmFOOTER_ARG("*Stream=0x%x", *Stream);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);


    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Destroy a previously constructed gcoSTREAM object.
 *
 * @param Stream Pointer to the gcoSTREAM object that needs to be
 *               destroyed.
 *
 * @return The status of the object destruction.
 */
gceSTATUS
gcoSTREAM_Destroy(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status;
    gcsSTREAM_DYNAMIC_PTR dynamic;
    gctUINT i;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    do
    {
        /* Free the memory attached to the gcoSTREAM object. */
        gcmERR_BREAK(_FreeMemory(Stream));

        /* Destroy the dynamic buffer. */
        if (Stream->dynamic != gcvNULL)
        {
            Stream->dynamicTail->next = gcvNULL;

            for (dynamic = Stream->dynamicHead;
                 dynamic != gcvNULL;
                 dynamic = dynamic->next)
            {
                gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL,
                                                 dynamic->signal));
            }

            /* Free the allocation of the buffer structures. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Stream->dynamic));
        }

        /* Check if there is a merged stream object. */
        if (Stream->merged != gcvNULL)
        {
            gctINT oldValue = 0;

            /* Decrement the reference counter of the merged stream. */
            gcmVERIFY_OK(gcoOS_AtomDecrement(gcvNULL,
                                             Stream->merged->reference,
                                             &oldValue));

            /* Check if we were the last stream attached to the merged
            ** stream. */
            if (oldValue == 1)
            {
                /* Destroy the merged stream. */
                gcmVERIFY_OK(gcoSTREAM_Destroy(Stream->merged));
            }
        }

        if (Stream->reference != gcvNULL)
        {
            /* Destroy the reference counter. */
            gcmVERIFY_OK(gcoOS_AtomDestroy(gcvNULL, Stream->reference));
        }

        /* Destroy the stream cache. */
        if (Stream->cache != gcvNULL)
        {
            /* Walk all cache buffers. */
            for (i = 0; i < Stream->cacheCount; ++i)
            {
                /* Unlock the stream. */
                gcmVERIFY_OK(gcoHARDWARE_Unlock(&Stream->cache[i].node,
                                                gcvSURF_VERTEX));

                /* Free the video memory. */
                gcmONERROR(gcoHARDWARE_ScheduleVideoMemory(&Stream->cache[i].node));

                /* Destroy the signal. */
                gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL,
                                                 Stream->cache[i].signal));
            }

            /* Free the stream cache. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Stream->cache));
        }

        /* Free the object structure. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Stream));

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Upload data to the memory attached to a gcoSTREAM object.
 * If there is no memory allocated or the allocated memory
 * is too small, the memory will be reallocated to fit the
 * request if offset is zero.
 *
 * @param Stream  Pointer to a gcoSTREAM object.
 * @param Buffer  Pointer to the data that needs to be uploaded.
 * @param Offset  Offset into the stream memory where to start the upload.
 * @param Bytes   Number of bytes to upload.
 * @param Dynamic Specifies whether the stream is dynamic or static.
 *                Dynamic streams will be allocted in system memory while
 *                static streams will be allocated in the default memory
 *                pool.
 *
 * @return Status of the upload.
 */
gceSTATUS
gcoSTREAM_Upload(
    IN gcoSTREAM Stream,
    IN gctCONST_POINTER Buffer,
    IN gctUINT32 Offset,
    IN gctSIZE_T Bytes,
    IN gctBOOL Dynamic
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;
    gcePOOL pool;
    gctSIZE_T bytes;

    gcmHEADER_ARG("Stream=0x%x Buffer=0x%x Offset=%u Bytes=%lu Dynamic=%d",
              Stream, Buffer, Offset, Bytes, Dynamic);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    /* Not available in dynamic streams. */
    if (Stream->dynamic != gcvNULL)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_REQUEST);
        return gcvSTATUS_INVALID_REQUEST;
    }

    do
    {
        /* Do we have memory already allooacted? */
        if (Stream->node.pool != gcvPOOL_UNKNOWN)
        {
            /* Test if the requested size is too big. */
            if (Offset + Bytes > Stream->size)
            {
                /* Error. */
                gcmFOOTER_ARG("status=%d", gcvSTATUS_BUFFER_TOO_SMALL);
                return gcvSTATUS_BUFFER_TOO_SMALL;
            }

            /* If the data range overlaps for a dynamic stream, create a new
               buffer. */
            if ( Dynamic &&
                 (Offset < Stream->lastEnd) &&
                 (Offset + Bytes > Stream->lastStart) )
            {
                gcmERR_BREAK(_FreeMemory(Stream));

                /* Clear dynamic range. */
                Stream->lastStart = 0;
                Stream->lastEnd   = 0;
            }
        }

        /* Do we need to allocate memory? */
        if (Stream->node.pool == gcvPOOL_UNKNOWN)
        {
            gctUINT32 alignment;

            /* Define the pool to allocate from based on the Dynamic
            ** parameter. */
            pool = Dynamic ? gcvPOOL_UNIFIED : gcvPOOL_DEFAULT;

            bytes = gcmMAX(Stream->size, Offset + Bytes);

            /* Query the stream alignment. */
            gcmERR_BREAK(
                gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            &alignment));

            /* Allocate the linear memory. */
            iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
            iface.u.AllocateLinearVideoMemory.pool      = pool;
            iface.u.AllocateLinearVideoMemory.bytes     = bytes;
            iface.u.AllocateLinearVideoMemory.type      = gcvSURF_VERTEX;
            iface.u.AllocateLinearVideoMemory.alignment = alignment;

            /* Call kernel HAL. */
            gcmERR_BREAK(gcoHAL_Call(gcvNULL, &iface));

            /* Save allocated information. */
            Stream->node.pool
                = iface.u.AllocateLinearVideoMemory.pool;
            Stream->node.u.normal.node
                = iface.u.AllocateLinearVideoMemory.node;
            Stream->size
                = iface.u.AllocateLinearVideoMemory.bytes;
            Stream->node.u.normal.cacheable = gcvFALSE;

            /* Reset the pointers. */
            Stream->node.logical = gcvNULL;

            /* Lock the memory. */
            gcmERR_BREAK(gcoHARDWARE_Lock(
                &Stream->node,
                gcvNULL,
                gcvNULL
                ));
        }

        if (Buffer != gcvNULL)
        {
            /* Copy user buffer to stream memory. */
            gcmERR_BREAK(gcoOS_MemCopy(
                (gctUINT8 *) Stream->node.logical + Offset,
                Buffer,
                Bytes
                ));

            /* Flush the vertex cache. */
            gcmERR_BREAK(
                gcoSTREAM_Flush(Stream));

            /* Flush the CPU cache. */
            gcmERR_BREAK(gcoSURF_NODE_Cache(&Stream->node,
                                          Stream->node.logical + Offset,
                                          Bytes,
                                          gcvCACHE_CLEAN));

            if (gcPLS.hal->dump != gcvNULL)
            {
                /* Dump the stream data. */
                gcmVERIFY_OK(gcoDUMP_DumpData(gcPLS.hal->dump,
                                              gcvTAG_STREAM,
                                              Stream->node.physical + Offset,
                                              Bytes,
                                              Buffer));
            }

            /* Update written range for dynamic buffers. */
            if (Dynamic)
            {
                if (Offset < Stream->lastStart)
                {
                    Stream->lastStart = Offset;
                }

                if (Offset + Bytes > Stream->lastEnd)
                {
                    Stream->lastEnd = Offset + Bytes;
                }
            }

            /* Delete any rebuilt buffer. */
            if (Stream->rebuild != gcvNULL)
            {
                gcmERR_BREAK(
                    gcoSTREAM_Destroy(Stream->rebuild));

                Stream->rebuild = gcvNULL;
            }

            /* Mark any merged stream as dirty. */
            if (Stream->merged != gcvNULL)
            {
                Stream->merged->dirty = gcvTRUE;
            }
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmPROFILE_GC(GLVERTEX_OBJECT_BYTES, Bytes);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Set the number of bytes per vertex inside a gcoSTREAM
 * object.
 *
 * @param Stream Pointer to a gcoSTREAM object.
 * @param Stride Number of bytes per vertex.
 *
 * @return Status of the stride setting.
 */
gceSTATUS
gcoSTREAM_SetStride(
    IN gcoSTREAM Stream,
    IN gctUINT32 Stride
    )
{
    gcmHEADER_ARG("Stream=0x%x Stride=%u", Stream, Stride);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Set stream stride. */
    Stream->stride = Stride;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/**
 * Lock the stream and return the pointers.
 *
 * @param Stream   Pointer to an initialized gcoSTREAM object.
 * @param Logical  Pointer to a variable receiving the logical address of
 *                 the locked memory.  If Logical is gcvNULL, the logical
 *                 address is not returned.
 * @param Physical Pointer to a variable receiving the physical address of
 *                 the locked memory.  If Physical is gcvNULL, the physical
 *                 address is not returned.
 *
 * @return The status of the lock.
 */
gceSTATUS
gcoSTREAM_Lock(
    IN gcoSTREAM Stream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical
    )
{
    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    if (Logical != gcvNULL)
    {
        /* Return logical address. */
        *Logical = (Stream->dynamic != gcvNULL)
                 ? Stream->dynamicHead->logical
                 : Stream->node.logical;
    }

    if (Physical != gcvNULL)
    {
        /* Return physical address. */
        *Physical = (Stream->dynamic != gcvNULL)
                  ? Stream->dynamicHead->physical
                  : Stream->node.physical;
    }

    /* Success. */
    gcmFOOTER_ARG("*Logical=0x%x *Physical=%08x",
                  gcmOPT_POINTER(Logical), gcmOPT_VALUE(Physical));
    return gcvSTATUS_OK;

}

/**
 * Unlock stream that was previously locked with gcoSTREAM_Lock.
 *
 * @param Stream   Pointer to an initialized gcoSTREAM object.
 *
 * @return The status of the Unlock.
 */
gceSTATUS
gcoSTREAM_Unlock(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Do we have a buffer allocated? */
    if (Stream->node.pool == gcvPOOL_UNKNOWN)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Unlock the veretx buffer. */
    status = gcoHARDWARE_Unlock(&Stream->node,
                                gcvSURF_VERTEX);

    /* Return the status. */
    gcmFOOTER();
    return status;
}


gceSTATUS
gcoSTREAM_Reserve(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Bytes
    )
{
    gctBOOL allocate;
    gceSTATUS status;
    gcsHAL_INTERFACE iface;

    gcmHEADER_ARG("Stream=0x%x Bytes=%lu", Stream, Bytes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Bytes > 0);

    do
    {
        /* Do we have memory already allooacted? */
        if (Stream->node.pool == gcvPOOL_UNKNOWN)
        {
            allocate = gcvTRUE;
        }

        else if (Stream->size < Bytes)
        {
            gcmERR_BREAK(_FreeMemory(Stream));

            allocate = gcvTRUE;
        }

        else
        {
            allocate = gcvFALSE;
        }

        Stream->lastStart = Stream->lastEnd = 0;

        if (allocate)
        {
            gctUINT32 alignment;

            /* Query the stream alignment. */
            gcmERR_BREAK(
                gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                            gcvNULL,
                                            gcvNULL,
                                            &alignment));

            /* Allocate the linear memory. */
            iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
            iface.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;
            iface.u.AllocateLinearVideoMemory.bytes     = Bytes;
            iface.u.AllocateLinearVideoMemory.alignment = alignment;
            iface.u.AllocateLinearVideoMemory.type      = gcvSURF_VERTEX;

            /* Call kernel HAL. */
            gcmERR_BREAK(gcoHAL_Call(gcvNULL, &iface));

            /* Save allocated information. */
            Stream->node.pool
                = iface.u.AllocateLinearVideoMemory.pool;
            Stream->node.u.normal.node
                = iface.u.AllocateLinearVideoMemory.node;
            Stream->size
                = iface.u.AllocateLinearVideoMemory.bytes;

            Stream->node.u.normal.cacheable = gcvFALSE;

            /* Reset the pointers. */
            Stream->node.logical = gcvNULL;

            /* Lock the memory. */
            gcmERR_BREAK(gcoHARDWARE_Lock(
                &Stream->node,
                gcvNULL,
                gcvNULL
                ));
        }

        /* Success. */
        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * gcoVERTEX object definition.
 */
struct _gcoVERTEX
{
    /* The object. */
    gcsOBJECT                   object;

    /* Hardware capabilities. */
    gctUINT                     maxAttribute;
    gctUINT                     maxSize;
    gctUINT                     maxStreams;

    /* Attribute array. */
    struct _gcsVERTEX_ATTRIBUTE
    {
        /* Format for attribute. */
        gceVERTEX_FORMAT        format;

        /* Whether attribute needs to be normalized or not. */
        gctBOOL                 normalized;

        /* Number of components in attribute. */
        gctUINT32               components;

        /* Size of the attribute in bytes. */
        gctSIZE_T               size;

        /* Stream holding attribute. */
        gcoSTREAM               stream;

        /* Offset into stream. */
        gctUINT32               offset;

        /* Stride in stream. */
        gctUINT32               stride;
    }
    attributes[gcdATTRIBUTE_COUNT];

    /* Combined stream. */
    gcoSTREAM                   combinedStream;
};

/**
 * Construct a new gcoVERTEX object.
 *
 * @param Hal    Pointer to an initialized gcoHAL object.
 * @param Vertex Pointer to a variable receiving the gcoVERTEX object
 *               pointer.
 *
 * @return Status of the construction.
 */
gceSTATUS
gcoVERTEX_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEX * Vertex
    )
{
    gceSTATUS status;
    gcoVERTEX vertex;
    gctSIZE_T i;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Vertex != gcvNULL);

    do
    {
        /* Allocate the object structure. */
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(struct _gcoVERTEX),
                                    &pointer));

        vertex = pointer;

        /* Initialize the object. */
        vertex->object.type = gcvOBJ_VERTEX;

        /* Query the stream capabilities. */
        gcmVERIFY_OK(gcoHARDWARE_QueryStreamCaps(&vertex->maxAttribute,
                                                 &vertex->maxSize,
                                                 &vertex->maxStreams,
                                                 gcvNULL));

        /* Zero out all attributes. */
        for (i = 0; i < gcmCOUNTOF(vertex->attributes); i++)
        {
            vertex->attributes[i].components = 0;
        }

        /* No combined stream. */
        vertex->combinedStream = gcvNULL;

        /* Return the gcoVERTEX object. */
        *Vertex = vertex;

        gcmPROFILE_GC(GLVERTEX_OBJECT, 1);

        /* Success. */
        gcmFOOTER_ARG("*Vertex=0x%x", *Vertex);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Destroy a previously constructed gcoVERTEX object.
 *
 * @param Vertex Pointer to a gcoVERTEX object.
 *
 * @return Status of the destruction.
 */
gceSTATUS
gcoVERTEX_Destroy(
    IN gcoVERTEX Vertex
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vertex=0x%x", Vertex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    gcmPROFILE_GC(GLVERTEX_OBJECT, -1);

    do
    {
        /* Destroy any combined stream. */
        if (Vertex->combinedStream != gcvNULL)
        {
            gcmERR_BREAK(gcoSTREAM_Destroy(Vertex->combinedStream));
            Vertex->combinedStream = gcvNULL;
        }

        gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, Vertex));
    }
    while (gcvFALSE);

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVERTEX_Reset(
    IN gcoVERTEX Vertex
    )
{
    gctSIZE_T i;

    gcmHEADER_ARG("Vertex=0x%x", Vertex);

    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    /* Destroy any combined stream. */
    if (Vertex->combinedStream != gcvNULL)
    {
        gcmVERIFY_OK(
            gcoSTREAM_Destroy(Vertex->combinedStream));

        Vertex->combinedStream = gcvNULL;
    }

    /* Zero out all attributes. */
    for (i = 0; i < gcmCOUNTOF(Vertex->attributes); i++)
    {
        Vertex->attributes[i].components = 0;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/**
 * Enable an attribute with the specified format sourcing
 * from the specified gcoSTREAM object.
 *
 * @param Vertex     Pointer to an initialized gcoVERTEX object.
 * @param Index      Attribute index.
 * @param Format     Format for attribute.
 * @param Normalized Flag indicating whether the attribute should be
 *                   normalized (gcvTRUE) or not (gcvFALSE).
 * @param Components Number of components for attribute.
 * @param Stream     Pointer to a gcoSTREAM object which is the source for the
 *                   attribute.
 * @param Offset     Ofsfet into the stream for the attribute.
 *
 * @return Status of the enable.
 */
gceSTATUS
gcoVERTEX_EnableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index,
    IN gceVERTEX_FORMAT Format,
    IN gctBOOL Normalized,
    IN gctUINT32 Components,
    IN gcoSTREAM Stream,
    IN gctUINT32 Offset,
    IN gctUINT32 Stride
    )
{
    gcmHEADER_ARG("Vertex=0x%x Index=%u Format=%d Normalized=%d Components=%u "
              "Stream=0x%x Offset=%u Stride=%u",
              Vertex, Index, Format, Normalized, Components, Stream, Offset,
              Stride);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);
    gcmVERIFY_ARGUMENT((Components > 0) && (Components <= 4));
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    if (Index >= gcmCOUNTOF(Vertex->attributes))
    {
        /* Invalid attribute index. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Determine the size of the attribute. */
    switch (Format)
    {
    case gcvVERTEX_BYTE:
    case gcvVERTEX_UNSIGNED_BYTE:
        Vertex->attributes[Index].size = Components * sizeof(gctUINT8);
        break;

    case gcvVERTEX_SHORT:
    case gcvVERTEX_UNSIGNED_SHORT:
    case gcvVERTEX_HALF:
        Vertex->attributes[Index].size = Components * sizeof(gctUINT16);
        break;

    case gcvVERTEX_INT:
    case gcvVERTEX_UNSIGNED_INT:
    case gcvVERTEX_FIXED:
    case gcvVERTEX_FLOAT:
    case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
    case gcvVERTEX_INT_10_10_10_2:
        Vertex->attributes[Index].size = Components * sizeof(gctUINT32);
        break;

    default:
        gcmFATAL("Unknown vertex format %d", Format);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Save vertex attributes. */
    Vertex->attributes[Index].format     = Format;
    Vertex->attributes[Index].normalized = Normalized;
    Vertex->attributes[Index].components = Components;
    Vertex->attributes[Index].stream     = Stream;
    Vertex->attributes[Index].offset     = Offset;
    Vertex->attributes[Index].stride     = Stride;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/**
 * Disable the specified attribute.
 *
 * @param Vertex Pointer to an initialized gcoVERTEX object.
 * @param Index  Attribute index to disable.
 *
 * @return Status of the disable.
 */
gceSTATUS
gcoVERTEX_DisableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index
    )
{
    gcmHEADER_ARG("Vertex=0x%x Index=%u", Vertex, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    if (Index >= gcmCOUNTOF(Vertex->attributes))
    {
        /* Invalid attribute index. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Disable attribute. */
    Vertex->attributes[Index].components = 0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

typedef struct _gcoStreamInfo
{
    /* Stream object. */
    gcoSTREAM   stream;

    /* Attribute count in this stream. */
    gctUINT32   attCount;

    /* If the attributes in the stream is packed by vertices. */
    gctBOOL     unpacked;

    /* Attributes in the stream. */
    gctUINT32   attributes[16];

} gcoStreamInfo;

static void
_SortStreamAttribute(
    IN OUT gcoStreamInfo * StreamInfo,
    IN gcsVERTEX_ATTRIBUTES_PTR Mapping
    )
{
    gctUINT32 i, j, k;

    /* Bubble sort by attribute offset. */
    for (i = 0; i < StreamInfo->attCount; i++)
    {
        for (j = StreamInfo->attCount - 1; j > i; j--)
        {
            if(Mapping[StreamInfo->attributes[j]].offset <
                    Mapping[StreamInfo->attributes[j-1]].offset)
            {
                k                           = StreamInfo->attributes[j];
                StreamInfo->attributes[j]   = StreamInfo->attributes[j-1];
                StreamInfo->attributes[j-1] = k;
            }
        }
    }
}

static gceSTATUS
_PackStreams(
    IN gcoVERTEX Vertex,
    IN OUT gctUINT32_PTR StreamCount,
    IN OUT gcoSTREAM * Streams,
    IN gctUINT32 AttributeCount,
    IN OUT gcsVERTEX_ATTRIBUTES_PTR Mapping,
    IN OUT gctUINT32 * Base
    )
{
    gctUINT32 i, j;
    gctSIZE_T bytes, stride;
    gctUINT32 count;
    gctUINT32 start, end, size;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 usedStreams = 0;
    gctBOOL repack = gcvFALSE;
    gcoSTREAM stream = gcvNULL;
    gcoStreamInfo *streamInfo;
    gctUINT8_PTR src, srcEnd, dest, source;
    gcoStreamInfo  *currentStream;
    gctUINT32 destIndex, destOffset;

    do
    {
	    gcmERR_BREAK(gcoOS_Allocate(gcvNULL, 16 * sizeof(gcoStreamInfo), (gctPOINTER *)&streamInfo));
        gcmERR_BREAK(gcoOS_ZeroMemory(streamInfo, 16 * sizeof(gcoStreamInfo)));

        stream = Streams[Mapping[0].stream];
        /* Construct streamInfo. */
        for (i = 0; i < AttributeCount; i++)
        {
            stream = Streams[Mapping[i].stream];

            /* Find the attribute in which stream. */
            for (j = 0; j < usedStreams; j++)
            {
                if (stream == streamInfo[j].stream)
                {
                    streamInfo[j].attributes[streamInfo[j].attCount] = i;
                    streamInfo[j].attCount++;

                    break;
                }
            }

            /* Can't find a stream, save it in a new stream. */
            if (j == usedStreams)
            {
                streamInfo[usedStreams].stream        = stream;
                streamInfo[usedStreams].attCount      = 1;
                streamInfo[usedStreams].attributes[0] = i;

                usedStreams++;
            }
        }

        if (usedStreams != *StreamCount)
        {
			if (streamInfo != gcvNULL)
			{
				gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, streamInfo));
			}
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        stride = 0;
        count  = 0;

        /* Check need to merge and calc merged size. */
        for (i = 0; i < usedStreams; i++)
        {
            gctUINT32 minOffset, maxOffset;
            gctUINT32 vertexCount;

            currentStream = &streamInfo[i];

            /* Sort attributes in the stream. */
            _SortStreamAttribute(currentStream, Mapping);

            /* Min and max offset of attributes in the stream. */
            minOffset = Mapping[currentStream->attributes[0]].offset;
            maxOffset = Mapping[currentStream->attributes[currentStream->attCount-1]].offset;

            if (maxOffset - minOffset > currentStream->stream->stride)
            {
                /* The attributes in the stream isn't packed by vertices. */
                currentStream->unpacked = gcvTRUE;
                repack                  = gcvTRUE;
            }

            /* Calc total stream size and stride. */
            for (j = 0; j < currentStream->attCount; j++)
            {
                gctUINT32 index = currentStream->attributes[j];

                stride += Mapping[index].size;
                start   = Mapping[index].offset;
                end     = Streams[Mapping[index].stream]->size;

                /* How many vertex count the attribute for? */
                vertexCount = (end - start + Mapping[index].stride)
                                / stream->stride;

                /* Get max vertex count. */
                count = count < vertexCount ? vertexCount : count;
            }
        }


        /* If doesn't need merge or repack, return OK. */
        if (!(repack || (*StreamCount > Vertex->maxStreams)))
        {
			if (streamInfo != gcvNULL)
			{
				gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, streamInfo));
			}
            return gcvSTATUS_OK;
        }

        /* Do stream merge and pack. */

        /* Get combined stream size. */
        bytes = stride * count;

        if (Vertex->combinedStream == gcvNULL)
        {
            gcmERR_BREAK(gcoSTREAM_Construct(gcvNULL,
                                             &Vertex->combinedStream));
        }

        gcmERR_BREAK(gcoSTREAM_Reserve(Vertex->combinedStream, bytes));

        gcmERR_BREAK(gcoSTREAM_SetStride(Vertex->combinedStream, stride));

        gcmERR_BREAK(gcoSTREAM_Lock(Vertex->combinedStream,
                                    (gctPOINTER) &dest,
                                    gcvNULL));

        destIndex = 0;

        /* Merge every attribute in streams. */
        for (i = 0; i < usedStreams; i++)
        {
            currentStream = &streamInfo[i];
            stream        = currentStream->stream;

            gcmERR_BREAK(gcoSTREAM_Lock(stream,
                                        (gctPOINTER) &source,
                                        gcvNULL));

            for (j = 0; j < currentStream->attCount; j++)
            {
                gctUINT32 index = currentStream->attributes[j];
                end     = Streams[Mapping[index].stream]->size;

                size        = Mapping[index].size;
                src         = source + Mapping[index].offset;
                srcEnd      = source + end;
                destOffset  = destIndex;

                while (src < srcEnd && destOffset < bytes)
                {
                    gcmERR_BREAK(gcoOS_MemCopy(dest + destOffset, src, size));

                    destOffset += stride;
                    src        += Mapping[index].stride;
                }

                Mapping[index].stream = 0;
                Mapping[index].offset = destIndex;
                destIndex += size;
            }
        }

        /* Update attributes and stream information. */
        if (gcmIS_SUCCESS(status))
        {
            /* Flush the stream. */
            gcmERR_BREAK(
                gcoSTREAM_Flush(Vertex->combinedStream));

            /* Flush the CPU cache. */
            gcmERR_BREAK(
                gcoSURF_NODE_Cache(&Vertex->combinedStream->node,
                                 Vertex->combinedStream->node.logical,
                                 Vertex->combinedStream->size,
                                 gcvCACHE_CLEAN));

            Streams[0]   = Vertex->combinedStream;
            *StreamCount = 1;

            for (i = 0; i < *StreamCount; i++)
            {
                Base[i] = 0;
            }

            if (gcPLS.hal->dump != gcvNULL)
            {
                /* Dump the stream data. */
                gcmVERIFY_OK(gcoDUMP_DumpData(gcPLS.hal->dump,
                                              gcvTAG_STREAM,
                                              Streams[0]->node.physical,
                                              Streams[0]->size,
                                              dest));
            }
        }

    } while(gcvFALSE);

	if (streamInfo != gcvNULL)
	{
	    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, streamInfo));
	}
    return gcvSTATUS_OK;
}


static gceSTATUS
_RebuildStream(
    IN OUT gcoSTREAM * StreamArray,
    IN gctUINT32 Stream,
    IN OUT gcsVERTEX_ATTRIBUTES_PTR Mapping,
    IN gctUINT32 AttributeCount,
    IN gcsSTREAM_RANGE * Range,
    IN gctUINT32 RangeCount
    )
{
    gcoSTREAM stream;
    gctUINT32 i, j;
    gctUINT32 stride;
    gctUINT32 sort[16] = {0}, count;
    gceSTATUS status;
    gctUINT maxStride;
    gctUINT32 mapping[16];

#if gcdDEBUG_REBUILD
    gcmPRINT("++%s: Stream=%p", __FUNCTION__, StreamArray[Stream]);

    for (i = 0; i < AttributeCount; ++i)
    {
        gcmPRINT("MAP:%d format=%d comp=%u size=%lu stream=%u offset=%u "
                 "stride=%u",
                 i, Mapping[i].format, Mapping[i].components, Mapping[i].size,
                 Mapping[i].stream, Mapping[i].offset, Mapping[i].stride);
    }
#endif

redo:
    /* Step 1.  Sort the range. */
    for (i = count = stride = 0; i < RangeCount; ++i)
    {
        if (Range[i].stream == Stream)
        {
            gctUINT32 k;

            /* Compute the stride while we are at it. */
            stride += Range[i].size;

            /* Find the slot we need to insert this range in to. */
            for (j = 0; j < count; ++j)
            {
                if (Range[i].start < Range[sort[j]].start)
                {
                    break;
                }
            }

            /* Make room in the array. */
            for (k = count; k > j; --k)
            {
                sort[k] = sort[k - 1];
            }

            /* Save the range. */
            sort[j] = i;

            /* Increment number of ranges. */
            ++ count;
        }
    }

#if gcdDEBUG_REBUILD
    for (i = 0; i < RangeCount; ++i)
    {
        gcmPRINT("RANGE:%d start=%u end=%u size=%u",
                 i, Range[i].start, Range[i].end, Range[i].size);
    }
#endif

    /* Step 2.  See if the stream already has a matching rebuilt stream. */
    stream = StreamArray[Stream]->rebuild;
    if (stream != gcvNULL)
    {
        /* Walk all attributes. */
        for (i = 0; i < AttributeCount; ++i)
        {
            /* This attribute doesn't match the stream. */
            if (Mapping[i].stream != Stream)
            {
                continue;
            }

            /* Walk all rebuilt entries. */
            for (j = 0; (j < 16) && (stream->mapping[j].size > 0); ++j)
            {
                /* Check if this attribute matches anything in the rebuilt stream. */
                if ((stream->mapping[j].size == Mapping[i].stride)
                &&  (stream->mapping[j].start <= Mapping[i].offset)
                &&  (Mapping[i].offset + Mapping[i].size <= stream->mapping[j].end)
                )
                {
                    break;
                }
            }

            /* Bail out if this attribute doesn't match anything. */
            if ((j == 16) || (stream->mapping[j].size == 0))
            {
#if gcdDEBUG_REBUILD
                gcmPRINT("MISS:%p i=%d", stream, i);

                for (j = 0; (j < 16) && ( stream->mapping[j].size > 0); ++j)
                {
                    gcmPRINT("  %d: start=%u end=%u size=%u",
                             j, stream->mapping[j].start,
                             stream->mapping[j].end, stream->mapping[j].size);
                }
#endif
                break;
            }

            /* Save mapping for this attribute. */
            mapping[i] = stream->mapping[j].stream
                       + Mapping[i].offset - stream->mapping[j].start;
        }

        /* See if the stream is already completely mapped. */
        if (i == AttributeCount)
        {
            /* Copy all the mappings of the attributes belonging to this
            ** stream. */
#if gcdDEBUG_REBUILD
            gcmPRINT("CACHED:%p", stream);
#endif
            while (i-- > 0)
            {
                if (Mapping[i].stream == Stream)
                {
                    /* Override offset and stride. */
                    Mapping[i].offset = mapping[i];
                    Mapping[i].stride = stream->stride;
#if gcdDEBUG_REBUILD
                    gcmPRINT("CACHED:%d offset=%u stride=%u",
                             i, Mapping[i].offset, Mapping[i].stride);
#endif
                }
            }

            /* Use the rebuilt stream. */
            StreamArray[Stream] = stream;

            /* Success. */
#if gcdDEBUG_REBUILD
            gcmPRINT("--%s", __FUNCTION__);
#endif
            return gcvSTATUS_OK;
        }
    }

    do
    {
        gctUINT8_PTR srcLogical, dstLogical;
        gctPOINTER pointer = gcvNULL;

        /* Step 3.  Destroy any previous rebuilt stream. */
        if (stream != gcvNULL)
        {
            gctBOOL changed = gcvFALSE;

            /* Add any missing map to the current range. */
            for (i = 0; (i < 16) && (stream->mapping[i].size > 0); ++i)
            {
                /* Walk the current range to find this mapping. */
                for (j = 0; j < RangeCount; ++j)
                {
                    if ((stream->mapping[i].start == Range[j].start)
                    &&  (stream->mapping[i].size  == Range[j].size)
                    )
                    {
                        break;
                    }
                }

                if ((j == RangeCount) && (j < 16))
                {
#if gcdDEBUG_REBUILD
                    gcmPRINT("Adding mapping %d at %d: %u-%u size=%u",
                             i, j, stream->mapping[i].start,
                             stream->mapping[i].end, stream->mapping[i].size);
#endif

                    Range[j].stream = 0;
                    Range[j].start  = stream->mapping[i].start;
                    Range[j].end    = stream->mapping[i].end;
                    Range[j].size   = stream->mapping[i].size;
                    RangeCount     += 1;

                    changed = gcvTRUE;
                }
            }

            /* Destroy the stream. */
            gcmERR_BREAK(gcoSTREAM_Destroy(stream));

            stream                       = gcvNULL;
            StreamArray[Stream]->rebuild = gcvNULL;

            if (changed)
            {
                goto redo;
            }
        }

        /* Step 4.  Allocate a new rebuilt stream. */
        gcmERR_BREAK(
            gcoSTREAM_Construct(gcvNULL, &stream));

        gcmERR_BREAK(
            gcoSTREAM_Reserve(stream, StreamArray[Stream]->size));

        gcmERR_BREAK(
            gcoHARDWARE_QueryStreamCaps(gcvNULL,
                                        &maxStride,
                                        gcvNULL,
                                        gcvNULL));

        /* Step 5.  Generate the mapping. */
        if (stride > maxStride)
        {
            /* Sort the attributes. */
            for (i = count = 0; i < AttributeCount; ++i)
            {
                gctUINT32 k;

                if (Mapping[i].stream != Stream)
                {
                    continue;
                }

                for (j = 0; j < count; ++j)
                {
                    if (Mapping[i].offset < Mapping[sort[i]].offset)
                    {
                        break;
                    }
                }

                for (k = count; k > j; --k)
                {
                    sort[k] = sort[k -1];
                }

                sort[j] = i;
                ++ count;
            }

            /* Generate the mapping. */
            for (i = stride = 0; i < count; ++i)
            {
                stream->mapping[i].stream = stride;
                stream->mapping[i].start  = Mapping[sort[i]].offset;
                stream->mapping[i].end    = Mapping[sort[i]].offset
                                          + Mapping[sort[i]].size;
                stream->mapping[i].size   = Mapping[sort[i]].stride;

                stride += Mapping[sort[i]].size;
            }

            gcmASSERT(stride <= maxStride);
            stream->stride = stride;
        }
        else
        {
            stream->stride = stride;

            stream->mapping[0].stream = 0;
            stream->mapping[0].start  = 0;
            stream->mapping[0].size   = Range[sort[0]].size;
            stream->mapping[0].end    = stream->mapping[0].size;

            for (i = 1; i < count; ++i)
            {
                /* Mapping starts at the end of the previous mapping. */
                stream->mapping[i].stream = stream->mapping[i - 1].stream
                                          + stream->mapping[i - 1].size;
                stream->mapping[i].start  = Range[sort[i]].start;
                stream->mapping[i].size   = Range[sort[i]].size;
                stream->mapping[i].end    = stream->mapping[i].start
                                          + stream->mapping[i].size;
            }
        }

        /* Zero out the remaining mappings. */
        for (; i < 16; ++i)
        {
            stream->mapping[i].stream = 0;
            stream->mapping[i].size   = 0;
        }

        /* Step 6.  Copy the data. */
        gcmERR_BREAK(gcoSTREAM_Lock(StreamArray[Stream], &pointer, gcvNULL));

        srcLogical = pointer;

        gcmERR_BREAK(gcoSTREAM_Lock(stream, &pointer, gcvNULL));

        dstLogical = pointer;

        for (i = 0; i < count; ++i)
        {
            gctUINT32 src   = stream->mapping[i].start;
            gctUINT32 end   = (i + 1 == count) ? stream->size
                                               : stream->mapping[i + 1].start;
            gctUINT32 dst   = stream->mapping[i].stream;
            gctUINT32 size  = stream->mapping[i].size;
            gctUINT32 bytes = (i + 1 == count) ? (stride - dst)
                                               : (stream->mapping[i + 1].stream - dst);

            while ((src < end) && (dst + size <= stream->size))
            {
                gcmASSERT(src + size <= StreamArray[Stream]->size);

                gcmVERIFY_OK(
                    gcoOS_MemCopy(dstLogical + dst, srcLogical + src, bytes));

                src += size;
                dst += stride;
            }
        }

        /* Flush the stream. */
        gcmERR_BREAK(
            gcoSTREAM_Flush(stream));

        /* Flush the CPU cache. */
        gcmERR_BREAK(gcoSURF_NODE_Cache(&stream->node,
                                      dstLogical,
                                      StreamArray[Stream]->size,
                                      gcvCACHE_CLEAN));

        /* Step 7.  Perform the mapping. */
#if gcdDEBUG_REBUILD
        gcmPRINT("REBUILT:%p", stream);
#endif
        for (i = 0; i < AttributeCount; ++i)
        {
            if (Mapping[i].stream == Stream)
            {
                for (j = 0; j < count; ++j)
                {
                    if ((stream->mapping[j].start <= Mapping[i].offset)
                    &&  (Mapping[i].offset + Mapping[i].size <= stream->mapping[j].end)
                    )
                    {
                        break;
                    }
                }

                gcmASSERT(j < count);

                Mapping[i].offset += stream->mapping[j].stream
                                  -  stream->mapping[j].start;
                Mapping[i].stride = stride;
#if gcdDEBUG_REBUILD
                gcmPRINT("REMAP:%d offset=%u stride=%u",
                         i, Mapping[i].offset, Mapping[i].stride);
#endif
            }
        }

        /* Use newly rebuilt stream. */
        StreamArray[Stream]->rebuild = stream;
        StreamArray[Stream] = stream;

        /* Success. */
#if gcdDEBUG_REBUILD
        gcmPRINT("--%s", __FUNCTION__);
#endif
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (stream != gcvNULL)
    {
        /* Destroy rebuilt stream. */
        gcmVERIFY_OK(
            gcoSTREAM_Destroy(stream));
    }

    /* Return the error. */
#if gcdDEBUG_REBUILD
    gcmPRINT("--%s: status=%d", __FUNCTION__, status);
#endif
    return status;
}

static gceSTATUS
_Unalias(
    IN gcoSTREAM Stream,
    IN gctUINT Attributes,
    IN gcsVERTEX_ATTRIBUTES_PTR Mapping,
    OUT gcoSTREAM * UseStream
    )
{
    gceSTATUS status;
    gcoSTREAM stream = gcvNULL;

    gcmHEADER_ARG("Stream=0x%x Attributes=%u Mapping=0x%x",
                  Stream, Attributes, Mapping);

    /* Special case 2 attribites. */
    if ((Attributes == 2)

    /* Same pointing at the same stream with a stride less than 8. */
    &&  (Mapping[0].stream == Mapping[1].stream)
    &&  (Mapping[0].stride <  8)

    /* Attribute 0 starts at offset 0. */
    &&  (Mapping[0].offset == 0)

    /* Attribute 1 aliases attribute 0. */
    &&  (Mapping[1].offset == 0)
    &&  (Mapping[1].size   == Mapping[0].size)
    )
    {
        /* Check if we already rebuilt this stream. */
        if (Stream->rebuild == gcvNULL)
        {
            gctUINT8_PTR src, dst;
            gctSIZE_T bytes;
            gctUINT stride;

            /* Construct a new stream. */
            gcmONERROR(gcoSTREAM_Construct(gcvNULL, &stream));

            /* Reserve twice the amount of data. */
            gcmONERROR(gcoSTREAM_Reserve(stream, Stream->size * 2));

            /* Get source stride. */
            stride = Stream->stride;

            /* Set destination stride to twice the source stride. */
            stream->stride = stride * 2;

            /* Get pointers to streams. */
            src = Stream->node.logical;
            dst = stream->node.logical;

            /* Copy all vertices. */
            for (bytes = Stream->size; bytes > 0; bytes -= stride)
            {
                /* Copy vertex. */
                gcmONERROR(gcoOS_MemCopy(dst, src, stride));

                /* Copy vertex again. */
                gcmONERROR(gcoOS_MemCopy(dst + stride, src, stride));

                /* Advance pointers. */
                src += stride;
                dst += stride * 2;
            }

            /* Flush the vertex buffer cache. */
            gcmONERROR(gcoSURF_NODE_Cache(&stream->node,
                                        stream->node.logical,
                                        stream->size,
                                        gcvCACHE_CLEAN));

            /* Save new stream as a rebuilt stream. */
            Stream->rebuild = stream;
        }
        else
        {
            /* Stream has already been rebuilt. */
            stream = Stream->rebuild;
        }

        /* Update mapping. */
        Mapping[0].stride = stream->stride;
        Mapping[1].offset = Stream->stride;
        Mapping[1].stride = stream->stride;
    }
    else
    {
        /* No need to rebuild anything. */
        stream = Stream;
    }

    /* Return stream. */
    *UseStream = stream;

    /* Success. */
    gcmFOOTER_ARG("*UseStream=0x%x", *UseStream);
    return gcvSTATUS_OK;

OnError:
    if (stream != gcvNULL)
    {
        /* Detsroy newly created stream. */
        gcmVERIFY_OK(gcoSTREAM_Destroy(stream));
    }

    /* Return error. */
    gcmFOOTER();
    return status;
}

/**
 * Bind a vertex to the hardware.
 *
 * @param   Vertex  Pointer to an initialized gcoVERTEX object.
 *
 * @return  Status of the binding.
 */
gceSTATUS
gcoVERTEX_Bind(
    IN gcoVERTEX Vertex
    )
{
    gctUINT32 streams, attributes;
    gctUINT i, j;
    gcoSTREAM usedStreams[16];
    gctBOOL rebuild[16];
    gcsVERTEX_ATTRIBUTES mapping[16];
    gceSTATUS status;
    gctUINT32 address, base[16];
    gcsSTREAM_RANGE range[16];
    gctUINT32 offset, size, stream;
    gctUINT32 rangeCount;

    gcmHEADER_ARG("Vertex=0x%x", Vertex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    /* Zero the stream usage table. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(usedStreams, sizeof(usedStreams)));

    /* Zero the range */
    gcmVERIFY_OK(gcoOS_ZeroMemory(range, sizeof(range)));

    /* Count the number of attributes and required streams. */
    streams = attributes = rangeCount = 0;

    /* Initialize stream base addresses. */
    for (i = 0; i < 16; ++i)
    {
        base[i]    = ~0U;
        rebuild[i] = gcvFALSE;
    }

    /* Walk through all attributes. */
    for (i = 0; i < Vertex->maxAttribute; ++i)
    {
        /* Verify if the attribute is enabled. */
        if (Vertex->attributes[i].components > 0)
        {
            /* Walk all used streams so far. */
            for (j = 0; j < streams; ++j)
            {
                /* If the attribute stream has been used, abort loop. */
                if (usedStreams[j] == Vertex->attributes[i].stream)
                {
                    break;
                }
            }

            /* See if the attribute streams has been used. */
            if (j == streams)
            {
                /* Save stream. */
                usedStreams[streams] = Vertex->attributes[i].stream;

                /* Increment stream count. */
                ++streams;
            }

            /* Fill in the mapping. */
            mapping[attributes].format     = Vertex->attributes[i].format;
            mapping[attributes].normalized = Vertex->attributes[i].normalized;
            mapping[attributes].components = Vertex->attributes[i].components;
            mapping[attributes].size       = Vertex->attributes[i].size;
            mapping[attributes].stream     = j;
            mapping[attributes].offset     = Vertex->attributes[i].offset;
            mapping[attributes].stride     = Vertex->attributes[i].stride;

            if (Vertex->attributes[i].offset < base[j])
            {
                base[j] = Vertex->attributes[i].offset;
            }

            offset = mapping[attributes].offset;
            size   = mapping[attributes].size;
            stream = mapping[attributes].stream;

            for (j = 0; j < rangeCount; ++j)
            {
                if ((range[j].stream == stream)
                &&  (range[j].size   == mapping[attributes].stride)
                )
                {
                    if ((range[j].start <= offset)
                    &&  (offset < range[j].start + range[j].size)
                    )
                    {
                        range[j].end = offset + size;
                        break;
                    }

                    if ((offset <= range[j].start)
                    &&  (range[j].end <= offset + range[j].size)
                    )
                    {
                        range[j].start = offset;
                        break;
                    }

                    if (offset + size == range[j].start)
                    {
                        range[j].start = offset;
                        break;
                    }

                    if (offset == range[j].end)
                    {
                        range[j].end += size;
                        break;
                    }
                }
            }

            if (j == rangeCount)
            {
                range[j].stream = stream;
                range[j].start  = offset;
                range[j].end    = offset + size;
                range[j].size   = mapping[attributes].stride;

                ++ rangeCount;

                if (!rebuild[stream])
                {
                    while (j-- > 0)
                    {
                        if (range[j].stream == stream)
                        {
                            rebuild[stream] = gcvTRUE;
                            break;
                        }
                    }
                }
            }

            /* Increment attribute count. */
            ++attributes;
        }
    }

    /* See if hardware supports the number of attributes. */
    if ((attributes == 0)
    ||  (attributes > Vertex->maxAttribute)
    )
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* See if we need to rebuild any stream. */
    if (streams <= Vertex->maxStreams)
    {
        for (i = 0; i < streams; ++i)
        {
            if (rebuild[i])
            {
                /* Rebuild the stream. */
                gcmONERROR(
                    _RebuildStream(usedStreams, i,
                                   mapping, attributes,
                                   range, rangeCount));
            }

            /* Check for stream aliasing. */
            else
            if ((usedStreams[i]->stride < 8)
            &&  (attributes == 2)
            &&  (mapping[0].offset == mapping[1].offset)
            )
            {
                /* Try to unalias the stream. */
                gcmONERROR(_Unalias(usedStreams[i],
                                    attributes, mapping,
                                    &usedStreams[i]));
            }
        }
    }

    /* Check if need to do stream merge and pack. */
    gcmONERROR(_PackStreams(Vertex,
                            &streams,
                            usedStreams,
                            attributes,
                            mapping,
                            base));

    /* Walk all streams. */
    for (i = 0; i < streams; ++i)
    {
        /* Lock the stream. */
        gcmONERROR(gcoSTREAM_Lock(usedStreams[i], gcvNULL, &address));

        /* Program the stream in hardware. */
        gcmONERROR(gcoHARDWARE_SetStream(i,
                                         address + base[i],
                                         usedStreams[i]->stride));
    }

    /* Adjust all base addresses. */
    for (i = 0; i < attributes; ++i)
    {
        mapping[i].offset -= base[mapping[i].stream];
    }

    /* Program the attributes in hardware. */
    gcmONERROR(gcoHARDWARE_SetAttributes(mapping,
                                         attributes));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_Flush(
    gcoSTREAM Stream
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    if (gcPLS.hal->dump != gcvNULL)
    {
        /* Dump the stream data. */
        gcmVERIFY_OK(gcoDUMP_DumpData(gcPLS.hal->dump,
                                      gcvTAG_STREAM,
                                      Stream->node.physical,
                                      Stream->size,
                                      Stream->node.logical));
    }

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   "stream",
                   Stream->node.physical,
                   Stream->node.logical,
                   0,
                   Stream->size);

    status = gcoHARDWARE_FlushVertex(gcvNULL);
    gcmFOOTER();
    return status;
}

/*******************************************************************************
*************************** DYNAMIC BUFFER MANAGEMENT **************************
*******************************************************************************/

/*******************************************************************************
**
**  gcoSTREAM_SetDynamic
**
**  Mark the gcoSTREAM object as dynamic.  A dynamic object will allocate the
**  specified number of buffers and upload data after the previous upload.  This
**  way there is no need for synchronizing the GPU or allocating new objects
**  each time an stream buffer is required.
**
**  INPUT:
**
**      gcoSTREAM Stream
**          Pointer to an gcoSTREAM object that needs to be converted to dynamic.
**
**      gctSIZE_T Bytes
**          Number of bytes per buffer.
**
**      gctUINT Buffers
**          Number of buffers.
*/
gceSTATUS
gcoSTREAM_SetDynamic(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Bytes,
    IN gctUINT Buffers
    )
{
    gceSTATUS status;
    gctUINT i;
    gctSIZE_T bytes;
    gcsSTREAM_DYNAMIC_PTR dynamic;
    gctUINT32 physical;
    gctUINT8_PTR logical;
    gcsHAL_INTERFACE iface;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Stream=0x%x Bytes=%lu Buffers=%u", Stream, Bytes, Buffers);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Bytes > 0);
    gcmVERIFY_ARGUMENT(Buffers > 0);

    /* We can only do this once. */
    if (Stream->dynamic != gcvNULL)
    {
        gcmFOOTER_ARG("Status=%d", gcvSTATUS_INVALID_REQUEST);
        return gcvSTATUS_INVALID_REQUEST;
    }

    /* Free any allocated video memory. */
    gcmONERROR(
        _FreeMemory(Stream));

    /* Allocate the video memory. */
    bytes = gcmALIGN(Bytes, 64) * Buffers;
    iface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
    iface.u.AllocateLinearVideoMemory.bytes     = bytes;
    iface.u.AllocateLinearVideoMemory.alignment = 64;
    iface.u.AllocateLinearVideoMemory.type      = gcvSURF_VERTEX;
    iface.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;

    /* Call the kernel. */
    gcmONERROR(
        gcoHAL_Call(gcvNULL, &iface));

    /* Initialize index. */
    Stream->size               = iface.u.AllocateLinearVideoMemory.bytes;
    Stream->node.pool          = iface.u.AllocateLinearVideoMemory.pool;
    Stream->node.u.normal.node = iface.u.AllocateLinearVideoMemory.node;
    Stream->node.u.normal.cacheable = gcvFALSE;

    /* Lock the stream buffer. */
    gcmONERROR(
        gcoHARDWARE_Lock(&Stream->node,
                         &physical,
                         &pointer));

    logical = pointer;

    /* Allocate memory for the buffer structures. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       Buffers * gcmSIZEOF(struct _gcsSTREAM_DYNAMIC),
                       &pointer));

    Stream->dynamic = pointer;

    gcmONERROR(
        gcoOS_ZeroMemory(Stream->dynamic,
                         Buffers * gcmSIZEOF(struct _gcsSTREAM_DYNAMIC)));

    bytes = Stream->size / Buffers;

    /* Initialize all buffer structures. */
    for (i = 0, dynamic = Stream->dynamic; i < Buffers; ++i, ++dynamic)
    {
        /* Create the signal. */
        gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvTRUE, &dynamic->signal));

        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_SIGNAL,
            "%s(%d): vertex buffer %d signal created 0x%08X",
            __FUNCTION__, __LINE__,
            i, dynamic->signal);

        /* Mark buffer as usuable. */
        gcmONERROR(gcoOS_Signal(gcvNULL, dynamic->signal, gcvTRUE));

        /* Set buffer address. */
        dynamic->physical = physical;
        dynamic->logical  = logical;

        /* Set buffer size. */
        dynamic->bytes = bytes;
        dynamic->free  = bytes;

        /* Set usage. */
        dynamic->lastStart = ~0U;
        dynamic->lastEnd   = 0;

        /* Link buffer in chain. */
        dynamic->next = dynamic + 1;

        /* Advance buffer addresses. */
        physical += bytes;
        logical  += bytes;
    }

    /* Initilaize chain of buffer structures. */
    Stream->dynamicHead       = Stream->dynamic;
    Stream->dynamicTail       = Stream->dynamic + Buffers - 1;
    Stream->dynamicTail->next = gcvNULL;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

    /***************************************************************************
    **  ERROR HANDLER
    */
OnError:

    /* Roll back allocation of buffer structures. */
    if (Stream->dynamic != gcvNULL)
    {
        /* Roll back all signal creations. */
        for (i = 0; i < Buffers; ++i)
        {
            if (Stream->dynamic[i].signal != gcvNULL)
            {
                gcmVERIFY_OK(
                    gcoOS_DestroySignal(gcvNULL,
                                        Stream->dynamic[i].signal));
            }
        }

        /* Roll back the allocation of the buffer structures. */
        gcmVERIFY_OK(
            gcmOS_SAFE_FREE(gcvNULL, Stream->dynamic));

        /* No buffers allocated. */
        Stream->dynamic = gcvNULL;
    }

    /* Roll back memory allocation. */
    gcmVERIFY_OK(
        _FreeMemory(Stream));

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoSTREAM_UploadDynamic
**
**  Upload data into a dynamic stream buffer.
**
**  INPUT:
**
**      gcoSTREAM Stream
**          Pointer to an gcoSTREAM object that has been configured as dynamic.
**
**      gctUINT VertexCount
**          Number of vertices to upload.
**
**      gctUINT InfoCount
**          Number of entries in the Info array.
**
**      gcsSTREAM_INFO_PTR Info
**          Pointer to an array of InfoCount gcsSTREAM_INFO structures.
**
**      gcoVERTEX Vertex
**          Pointer to a gcoVERTEX object that will be filled in.
*/
gceSTATUS
gcoSTREAM_UploadDynamic(
    IN gcoSTREAM Stream,
    IN gctUINT VertexCount,
    IN gctUINT InfoCount,
    IN gcsSTREAM_INFO_PTR Info,
    IN gcoVERTEX Vertex
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE iface;
    gcsSTREAM_DYNAMIC_PTR dynamic;
    gctUINT bytes;
    gctUINT i, j;
    gctUINT8_PTR ptr;
    gctUINT stride = 0;
    gctUINT8_PTR source[16];
    gctUINT offset;

    gcmHEADER_ARG("Stream=0x%x VertexCount=%d InfoCount=%u Info=0x%x Vertex=0x%x",
              Stream, VertexCount, InfoCount, Info, Vertex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(VertexCount > 0);
    gcmVERIFY_ARGUMENT((InfoCount > 0) && (InfoCount < 16));
    gcmVERIFY_ARGUMENT(Info != gcvNULL);

    /* This has to be a dynamic stream. */
    if (Stream->dynamic == gcvNULL)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_REQUEST);
        return gcvSTATUS_INVALID_REQUEST;
    }

    /* Compute the stride and copy pointers. */
    for (i = 0; i < InfoCount; ++i)
    {
        stride += Info[i].size;

        source[i] = (gctUINT8_PTR) Info[i].data;
    }

    /* Compute the number of bytes required. */
    bytes = stride * VertexCount;

    /* Shorthand the pointer. */
    dynamic = Stream->dynamicHead;
    gcmASSERT(dynamic != gcvNULL);
    if (dynamic == gcvNULL)
    {
        gcmONERROR(gcvSTATUS_HEAP_CORRUPTED);
    }

    /* Make sure the dynamic stream buffer is large enough to hold this data. */
    if (bytes > dynamic->bytes)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_DATA_TOO_LARGE);
        return gcvSTATUS_DATA_TOO_LARGE;
    }

    if (dynamic->free < bytes)
    {
        /* Not enough free bytes in this buffer, mark it busy... */
        gcmONERROR(
            gcoOS_Signal(gcvNULL,
                         dynamic->signal,
                         gcvFALSE));

        /* ...schedule a signal event... */
        iface.command            = gcvHAL_SIGNAL;
        iface.u.Signal.signal    = dynamic->signal;
        iface.u.Signal.auxSignal = gcvNULL;
        iface.u.Signal.process   = gcoOS_GetCurrentProcessID();
        iface.u.Signal.fromWhere = gcvKERNEL_COMMAND;
        gcmONERROR(
            gcoHARDWARE_CallEvent(&iface));

        /* ...commit the buffer... */
        gcmONERROR(
            gcoHARDWARE_Commit());

        /* ...move it to the tail of the queue... */
        gcmASSERT(Stream->dynamicTail != gcvNULL);
        if (Stream->dynamicTail == gcvNULL)
        {
            gcmONERROR(gcvSTATUS_HEAP_CORRUPTED);
        }
        Stream->dynamicTail->next = dynamic;
        Stream->dynamicTail       = dynamic;
        Stream->dynamicHead       = dynamic->next;
        gcmASSERT(Stream->dynamicHead != gcvNULL);

        /* ...reinitialize the top of the queue... */
        dynamic            = Stream->dynamicHead;
        if (dynamic == gcvNULL)
        {
            gcmONERROR(gcvSTATUS_HEAP_CORRUPTED);
        }
        dynamic->free      = dynamic->bytes;
        dynamic->lastStart = ~0U;
        dynamic->lastEnd   = 0;

        /* ...wait for the top of the queue to become available. */
        gcmONERROR(
            gcoOS_WaitSignal(gcvNULL,
                             dynamic->signal,
                             gcPLS.hal->timeOut));
    }

    /* Set pointer. */
    ptr = dynamic->logical + dynamic->lastEnd;

    /* Copy the data into the buffer. */
    for (i = 0; i < VertexCount; ++i)
    {
        for (j = 0; j < InfoCount; ++j)
        {
            gcmONERROR(
                gcoOS_MemCopy(ptr, source[j], Info[j].size));

            source[j] += Info[j].stride;
            ptr       += Info[j].size;
        }
    }

    /* Flush the CPU cache. */
    gcmONERROR(gcoSURF_NODE_Cache(&Stream->node,
                                dynamic->logical + dynamic->lastEnd,
                                bytes,
                                gcvCACHE_CLEAN));

    /* Update the pointers. */
    dynamic->lastStart = dynamic->lastEnd;
    dynamic->lastEnd   = dynamic->lastStart + bytes;
    dynamic->free     -= bytes;
    Stream->stride     = stride;

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   "stream",
                   dynamic->physical,
                   dynamic->logical,
                   dynamic->lastStart,
                   bytes);

    gcmONERROR(
        gcoVERTEX_Reset(Vertex));

    /* Copy data into the gcoVERTEX object. */
    for (i = 0, offset = dynamic->lastStart; i < InfoCount; ++i)
    {
        gcmONERROR(
            gcoVERTEX_EnableAttribute(Vertex,
                                      Info[i].index,
                                      Info[i].format,
                                      Info[i].normalized,
                                      Info[i].components,
                                      Stream,
                                      offset,
                                      stride));

        offset += Info[i].size;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

    /***************************************************************************
    **  ERROR HANDLER
    */
OnError:

    /* Return the status.*/
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_SetAttribute(
    IN gcoSTREAM Stream,
    IN gctUINT Offset,
    IN gctUINT Bytes,
    IN gctUINT Stride,
    IN OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    )
{
    gctUINT end, sub;
    gcsSTREAM_SUBSTREAM_PTR subPtr;
    gceSTATUS status;

    gcmHEADER_ARG("Stream=0x%x Offset=%u Bytes=%u Stride=%u *SubStream=0x%x",
                  Stream, Offset, Bytes, Stride, gcmOPT_POINTER(SubStream));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT((Bytes > 0) && (Bytes <= Stride));
    gcmVERIFY_ARGUMENT(Stride > 0);
    gcmVERIFY_ARGUMENT(SubStream != gcvNULL);

    /* Compute the end. */
    end = Offset + Bytes;

    /* Walk all current sub-streams parsed so far. */
    for (subPtr = *SubStream; subPtr != gcvNULL; subPtr = subPtr->next)
    {
        if (Stride == subPtr->stride)
        {
            /* Test if the attribute falls within the sub-stream. */
            if ((Offset >= subPtr->start)
            &&  (end    <= subPtr->end)
            )
            {
                break;
            }

            /* Test if the attribute starts within the sliding window. */
            if ((Offset >= subPtr->minStart)
            &&  (Offset <  subPtr->start)
            )
            {
                /* Adjust sliding window start. */
                subPtr->start  = Offset;
                subPtr->maxEnd = Offset + Stride;
                break;
            }

            /* Test if attribute ends within the sliding window. */
            if ((end >  subPtr->end)
            &&  (end <= subPtr->maxEnd)
            )
            {
                subPtr->end      = end;
                subPtr->minStart = gcmMAX((gctINT) (end - Stride), 0);
                break;
            }
        }
    }

    if (subPtr == gcvNULL)
    {
        gcsSTREAM_SUBSTREAM_PTR p, prev;

        /* Walk all sub-streams. */
        for (sub = 0, subPtr = Stream->subStreams;
             sub < Stream->subStreamCount;
             ++sub, ++subPtr)
        {
            if (Stride == subPtr->stride)
            {
                /* Test if the attribute falls within the sub-stream. */
                if ((Offset >= subPtr->start)
                &&  (end    <= subPtr->end)
                )
                {
                    break;
                }

                /* Test if the attribute starts within the sliding window. */
                if ((Offset >= subPtr->minStart)
                &&  (Offset <  subPtr->start)
                )
                {
                    /* Adjust sliding window start. */
                    subPtr->start  = Offset;
                    subPtr->maxEnd = Offset + Stride;
                    break;
                }

                /* Test if attribute ends within the sliding window. */
                if ((end >  subPtr->end)
                &&  (end <= subPtr->maxEnd)
                )
                {
                    subPtr->end      = end;
                    subPtr->minStart = gcmMAX((gctINT) (end - Stride), 0);
                    break;
                }
            }
        }

        /* Test if we need to create a new sub-stream. */
        if (sub == Stream->subStreamCount)
        {
            /* Make sure we don't overflow the array. */
            if (sub == gcmCOUNTOF(Stream->subStreams))
            {
                if (*SubStream == gcvNULL)
                {
                    /* If no sub stream is added to the chain, we can reset all sub streams. */
                    subPtr = Stream->subStreams;

                    Stream->subStreamCount = 0;

                    gcmONERROR(gcoOS_ZeroMemory(Stream->subStreams,
                                                sizeof(Stream->subStreams)));
                }
                else
                {
                    gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                }
            }

            /* Fill in the sub-stream. */
            subPtr->start    = Offset;
            subPtr->end      = end;
            subPtr->minStart = gcmMAX((gctINT) (end - Stride), 0);
            subPtr->maxEnd   = Offset + Stride;
            subPtr->stride   = Stride;

            /* Add this substream stride to the total stride. */
            Stream->subStreamStride += Stride;

            /* Increment number of substreams. */
            Stream->subStreamCount += 1;
        }

        /* Link this sub-stream into the linked list. */
        for (p = *SubStream, prev = gcvNULL; p != gcvNULL; p = p->next)
        {
            if (p->start > subPtr->start)
            {
                break;
            }

            prev = p;
        }

        subPtr->next = p;
        if (prev == gcvNULL)
        {
            *SubStream = subPtr;
        }
        else
        {
            prev->next = subPtr;
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_QuerySubStreams(
    IN gcoSTREAM Stream,
    IN gcsSTREAM_SUBSTREAM_PTR SubStream,
    OUT gctUINT_PTR SubStreamCount
    )
{
    gceSTATUS status;
    gctUINT i;

    gcmHEADER_ARG("Stream=0x%x SubStream=0x%x", Stream, SubStream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(SubStreamCount != gcvNULL);

    /* Check whether the substream information is the same for the rebulit
    ** stream. */
    if ((Stream->rebuild != gcvNULL)
    &&  (Stream->subStreamCount == Stream->rebuild->subStreamCount)
    &&  gcmIS_SUCCESS(gcoOS_MemCmp(Stream->subStreams,
                                   Stream->rebuild->subStreams,
                                   ( Stream->subStreamCount
                                   * gcmSIZEOF(gcsSTREAM_SUBSTREAM)
                                   )))
    )
    {
        /* Rebuilt stream seems to be okay - only one stream. */
        *SubStreamCount = 1;
    }
    else
    {
        /* Destroy any current rebuilt stream. */
        if (Stream->rebuild != gcvNULL)
        {
            gcmONERROR(gcoSTREAM_Destroy(Stream->rebuild));
            Stream->rebuild = gcvNULL;
        }

        /* Count the number of entries in the linked list. */
        for (i = 0;
             (i < Stream->subStreamCount) && (SubStream != gcvNULL);
             ++i, SubStream = SubStream->next
        )
        {
        }

        /* Return number of substreams before rebuilding. */
        *SubStreamCount = i;
    }

    /* Success. */
    gcmFOOTER_ARG("*SubStreamCount=%u", *SubStreamCount);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_Rebuild(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    OUT gctUINT_PTR SubStreamCount
    )
{
    gceSTATUS status;
    gctUINT count = First + Count;
    gctUINT8_PTR rebuildPtr, streamPtr[16];
    gctUINT stride[16];
    gctUINT i, j;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u", Stream, First, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Count > 0);
    gcmVERIFY_ARGUMENT(SubStreamCount != gcvNULL);

    /* Only try packing if we have more than 1 stream. */
    if (Stream->subStreamCount > 1)
    {
        /* We don't have a rebuilt stream yet. */
        if ((Stream->rebuild == gcvNULL)

        /* Check if rebulit stream has a different substream count. */
        ||  (Stream->subStreamCount != Stream->rebuild->subStreamCount)

        /* Check if the substream data is different. */
        ||  !gcmIS_SUCCESS(gcoOS_MemCmp(Stream->subStreams,
                                        Stream->rebuild->subStreams,
                                        ( Stream->subStreamCount
                                        * gcmSIZEOF(gcsSTREAM_SUBSTREAM)
                                        )))
        )
        {
            if (Stream->rebuild != gcvNULL)
            {
                /* Destroy the current rebuilt buffer. */
                gcmONERROR(gcoSTREAM_Destroy(Stream->rebuild));
                Stream->rebuild = gcvNULL;
            }

            /* Construct a new stream. */
            gcmONERROR(gcoSTREAM_Construct(gcvNULL, &Stream->rebuild));

            /* Reserve enough data for all the substreams. */
            gcmONERROR(gcoSTREAM_Reserve(Stream->rebuild,
                                         Stream->subStreamStride * count));

            /* Initialize the pointers. */
            rebuildPtr = Stream->rebuild->node.logical;
            for (i = 0; i < Stream->subStreamCount; ++i)
            {
                streamPtr[i] = Stream->node.logical
                             + Stream->subStreams[i].start;

                stride[i] = Stream->subStreams[i].stride;
            }

            /* Walk all vertices. */
            for (i = 0; i < Count; ++i)
            {
                /* Walk all substreams. */
                for (j = 0; j < Stream->subStreamCount; ++j)
                {
                    /* Copy the data from this substream into the rebuild
                    ** buffer. */
                    gcmONERROR(gcoOS_MemCopy(rebuildPtr,
                                             streamPtr[j],
                                             stride[j]));

                    /* Advance the pointers. */
                    rebuildPtr   += stride[j];
                    streamPtr[j] += stride[j];
                }
            }

            /* Copy the substream data. */
            gcmONERROR(gcoOS_MemCopy(
                Stream->rebuild->subStreams,
                Stream->subStreams,
                Stream->subStreamCount * gcmSIZEOF(gcsSTREAM_SUBSTREAM)));

            Stream->rebuild->subStreamCount = Stream->subStreamCount;

            /* Flush the new rebuilt buffer. */
            gcmONERROR(gcoSURF_NODE_Cache(&Stream->rebuild->node,
                                        Stream->rebuild->node.logical,
                                        Stream->rebuild->node.size,
                                        gcvCACHE_CLEAN));
        }
    }

    /* Return number of substreams rebuilt. */
    *SubStreamCount = Stream->subStreamCount;

    /* Success. */
    gcmFOOTER_ARG("*SubStreamCount=%u", *SubStreamCount);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_SetCache(
    IN gcoSTREAM Stream
    )
{
    gceSTATUS status;
    gctUINT i;
    gcsHAL_INTERFACE ioctl;
    gcsSTREAM_CACHE_BUFFER_PTR cache;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Not valid with dynamic streams. */
    if (Stream->dynamic != gcvNULL)
    {
        gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Only process if not yet cached. */
    if (Stream->cache == gcvNULL)
    {
        gctPOINTER pointer = gcvNULL;

        /* Allocate the stream cache. */
        gcmONERROR(
            gcoOS_Allocate(gcvNULL,
                           ( gcmSIZEOF(gcsSTREAM_CACHE_BUFFER)
                           * gcdSTREAM_CACHE_COUNT
                           ),
                           &pointer));

        Stream->cache = pointer;

        /* Zero all memory. */
        gcmONERROR(gcoOS_ZeroMemory(Stream->cache,
                                    ( gcmSIZEOF(gcsSTREAM_CACHE_BUFFER)
                                    * gcdSTREAM_CACHE_COUNT)
                                    ));

        /* Initialize the stream cache. */
        Stream->cacheCount   = gcdSTREAM_CACHE_COUNT;
        Stream->cacheCurrent = 0;
        Stream->cacheLastHit = 0;

        for (i = 0, cache = Stream->cache;
             i < gcdSTREAM_CACHE_COUNT;
             ++i, ++cache
        )
        {
            /* Allocate linear video memory. */
            ioctl.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
            ioctl.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;
            ioctl.u.AllocateLinearVideoMemory.bytes     = gcdSTREAM_CACHE_SIZE;
            ioctl.u.AllocateLinearVideoMemory.alignment = 64;
            ioctl.u.AllocateLinearVideoMemory.type      = gcvSURF_VERTEX;
            gcmONERROR(gcoHAL_Call(gcvNULL, &ioctl));

            /* Save allocation information. */
            cache->node.pool          = ioctl.u.AllocateLinearVideoMemory.pool;
            cache->node.u.normal.node = ioctl.u.AllocateLinearVideoMemory.node;
            cache->node.u.normal.cacheable = gcvFALSE;
            cache->node.size          = ioctl.u.AllocateLinearVideoMemory.bytes;
            cache->node.logical       = gcvNULL;

            /* Lock the stream. */
            gcmONERROR(gcoHARDWARE_Lock(&cache->node,
                                        gcvNULL, gcvNULL));

            /* Initialize the stream cache. */
            cache->bytes  = cache->node.size;
            cache->free   = cache->node.size;
            cache->offset = 0;
            cache->index  = 0;

            /* Create the signal. */
            gcmONERROR(gcoOS_CreateSignal(gcvNULL, gcvTRUE, &cache->signal));

            gcmTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_SIGNAL,
                "%s(%d): cache %d signal created 0x%08X",
                __FUNCTION__, __LINE__,
                i, cache->signal);

            /* Mark the stream as available. */
            gcmONERROR(gcoOS_Signal(gcvNULL, cache->signal, gcvTRUE));
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Roll back. */
    if (Stream->cache != gcvNULL)
    {
        /* Walk all streams. */
        for (i = 0, cache = Stream->cache; i < Stream->cacheCount; ++i, ++cache)
        {
            if (cache->signal != gcvNULL)
            {
                /* Destroy the signal. */
                gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL,
                                                 cache->signal));
            }

            if (cache->node.logical != gcvNULL)
            {
                /* Unlock the stream. */
                gcmVERIFY_OK(gcoHARDWARE_Unlock(&cache->node,
                                                gcvSURF_VERTEX));
            }

            if (cache->node.u.normal.node != gcvNULL)
            {
                /* Free the stream. */
                ioctl.command = gcvHAL_FREE_VIDEO_MEMORY;
                ioctl.u.FreeVideoMemory.node = cache->node.u.normal.node;
                gcmVERIFY_OK(gcoHAL_Call(gcvNULL, &ioctl));
            }
        }

        /* Free the stream cache. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Stream->cache));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if !OPT_CACHE_USAGE
static gctUINT
_Reflect(
    IN gctUINT32 Value,
    IN gctINT Bits
    )
{
    /* Reflection table for nibbles. */
    static gctUINT8 reflect[16] =
    {
        0x0,    /* 0000 -> 0000 */
        0x8,    /* 0001 -> 1000 */
        0x4,    /* 0010 -> 0100 */
        0xC,    /* 0011 -> 1100 */
        0x2,    /* 0100 -> 0010 */
        0xA,    /* 0101 -> 1010 */
        0x6,    /* 0110 -> 0110 */
        0xE,    /* 0111 -> 1110 */
        0x1,    /* 1000 -> 0001 */
        0x9,    /* 1001 -> 1001 */
        0x5,    /* 1010 -> 0101 */
        0xD,    /* 1011 -> 1101 */
        0x3,    /* 1100 -> 0011 */
        0xB,    /* 1101 -> 1011 */
        0x7,    /* 1110 -> 0111 */
        0xF     /* 1111 -> 1111 */
    };

    gctUINT result = 0;

    /* Loop while we have bits to reflect. */
    while (Bits > 0)
    {
        /* Reflect 4 bits at a time. */
        result = (result << 4) | reflect[Value & 0xF];

        /* Next 4 bits. */
        Value >>= 4;
        Bits   -= 4;
    }

    /* Return result. */
    return result;
}


#if OPT_VERTEX_ARRAY
static gceSTATUS
_CRC32(
    IN gcoSTREAM Stream,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctPOINTER Logical OPTIONAL,
    OUT gctUINT32_PTR Crc32 OPTIONAL,
    OUT gctBOOL_PTR Empty OPTIONAL,
    OUT gctSIZE_T_PTR Bytes OPTIONAL
)
{
    gceSTATUS status = gcvSTATUS_OK;

    gctUINT i, j, k;
    gctUINT8_PTR src[gcdATTRIBUTE_COUNT], dst;
    gctUINT32 crc = 0xFFFFFFFF;
    static gctUINT32 crcTable[256];
    static gctBOOL tableSetup = gcvFALSE;
    gctUINT stride = 0;
    gcsVERTEXARRAY_BUFFER_PTR buffer;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attribute = Attributes;
    gctUINT attribStride[gcdATTRIBUTE_COUNT];
    gctSIZE_T bytes = 0;

    gcmHEADER_ARG("Stream=0x%x BufferCount=%u Buffers=0x%x AttributeCount=%u "
                  "Attributes=0x%x First=%u Count=%u Logical=0x%x",
                  Stream, BufferCount, Buffers, AttributeCount,
                  Attributes, First, Count, Logical);

    /* Check if we need to setup the CRC table. */
    if (!tableSetup)
    {
        /* Polynominal widely used for CRC-32 checking. */
        gctUINT32 polynomial = 0x04C11DB7;

        /* Setup the entire table. */
        for (i = 0; i < 256; ++i)
        {
            /* Put the reflection of the index into the table. */
            crcTable[i] = _Reflect(i, 8) << 24;

            /* Shift the table entry by 8 and use the shifted bits to xor in the
            ** polynomial. */
            for (j = 0; j < 8; ++j)
            {
                crcTable[i] = (crcTable[i] << 1)
                            ^ ((crcTable[i] & (1U << 31)) ? polynomial : 0);
            }

            /* Refelect the entire table entry. */
            crcTable[i] = _Reflect(crcTable[i], 32);
        }

        /* CRC-32 table has been setup. */
        tableSetup = gcvTRUE;
    }

    /* Compute the destination address. */
    dst = (gctUINT8_PTR) Logical;

    /* Check if we need to copy the attributes. */
    if (Logical != gcvNULL)
    {
        /* Walk through all buffers. */
        for (i = 0, buffer = Buffers;
             i < BufferCount;
             i++, buffer++)
        {
            /* We copy whole range for non-combined buffer. */
            if (!buffer->combined)
            {
                gctUINT size;

                stride = attribute[buffer->map[0]].vertexPtr->enable
                       ? attribute[buffer->map[0]].vertexPtr->stride
                       : 0;

                src[0] = (gctUINT8_PTR)buffer->start + First * stride;
                dst    = (gctUINT8_PTR)Logical + buffer->offset;
                size   = Count * buffer->stride;

                /* Attribute pointer enabled case. */
                if (stride != 0)
                {
                    gcmONERROR(gcoOS_MemCopy(dst, src[0], size));

                    bytes += size;
                }
                /* Copy from generic value. */
                else
                {
                    for (j = 0; j < Count; j++)
                    {
                        gcmONERROR(gcoOS_MemCopy(dst, src[0], attribute[buffer->map[0]].bytes));

                        dst   += attribute[buffer->map[0]].bytes;
                        bytes += attribute[buffer->map[0]].bytes;
                    }
                }
            }
            /* Combined case, need to pack from all attribute pointers. */
            else
            {
                gctUINT size[gcdATTRIBUTE_COUNT];

                dst = (gctUINT8_PTR)Logical + buffer->offset;

                /* Setup source info for all attributes. */
                for (j = 0; j < buffer->count; j++)
                {
                    attribStride[j]  = Attributes[buffer->map[j]].vertexPtr->enable
                                     ? Attributes[buffer->map[j]].vertexPtr->stride
                                     : 0;

                    /* Add the first vertex. */
                    src[j] = (gctUINT8_PTR) Attributes[buffer->map[j]].logical
                           + First * attribStride[j];

                    size[j] = Attributes[buffer->map[j]].bytes;
                }

                /* Walk through all vertices. */
                for(j = 0; j < Count; j++)
                {
                    /* Walk through all attributes in a buffer. */
                    for(k = 0; k < buffer->count; k++)
                    {
                        gcmONERROR(gcoOS_MemCopy(dst, src[k], size[k]));

                        /* Advance the pointers. */
                        src[k] += attribStride[k];
                        dst    += size[k];
                        bytes  += size[k];
                    }
                }
            }
        }
    }

    /* Check if we need to compute the CRC-32 value. */
    if (Crc32 != gcvNULL)
    {
        /* Compute all source addresses for the attributes. */
        for (i = 0; i < AttributeCount; ++i)
        {
            attribStride[i] = Attributes[i].vertexPtr->enable
                            ? Attributes[i].vertexPtr->stride
                            : 0;

            /* Add the first vertex. */
            src[i] = (gctUINT8_PTR) Attributes[i].logical
                   + First * attribStride[i];
        }

        /* Walk through all vertices. */
        for (i = 0; i < Count; ++i)
        {
            /* Walk through all attributes. */
            for (j = 0; j < AttributeCount; ++j)
            {
                /* Walk through all bytes. */
                for (k = 0; k < Attributes[j].bytes; ++k)
                {
                    /* Compute the CRC32 value. */
                    crc = (crc >> 8) ^ crcTable[(crc & 0xFF) ^ src[j][k]];
                }

                /* Advance the pointers. */
                src[j] += attribStride[j];
            }
        }
    }

    /* Copy the CRC-32 value. */
    if (Crc32 != gcvNULL)
    {
        *Crc32 = crc;
    }

    /* Return the number of bytes copied. */
    if (Bytes != gcvNULL)
    {
        *Bytes = bytes;
    }

    /* Success. */
    gcmFOOTER_ARG("*Crc32=0x%08x *Empty=%d *Bytes=%u",
                  gcmOPT_VALUE(Crc32), gcmOPT_VALUE(Empty),
                  gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#else

static gctFLOAT
_Half2Float(
    IN gctUINT16 Half
    )
{
    union gcsFLOAT_UINT32
    {
        gctFLOAT  f;
        gctUINT32 u;
    } result;
    static gctUINT mantissaTable[2048];
    static gctUINT exponentTable[32];
    static gctBOOL tableSetup = gcvFALSE;
    gctUINT16 mIndex, eIndex, sign;

    gcmHEADER_ARG("Half=0x%04x", Half);

    /* Check if the tables are setup or not. */
    if (!tableSetup)
    {
        gctINT i;
        gctUINT m, e;

        /* Build exponent table. */
        exponentTable[0] = 0;
        for (i = 1; i <= 30; ++i)
        {
            exponentTable[i] = i << 23;
        }
        exponentTable[31] = 0x47800000;

        /* Build mantissa table. */
        mantissaTable[0] = 0;
        for (i = 1; i <= 1023; ++i)
        {
            m = i << 13;
            e = 0;
            while (!(m & 0x00800000))
            {
                e  -= 0x00800000;
                m <<= 1;
            }
            m &= ~0x00800000;
            e +=  0x38800000;
            mantissaTable[i] = m | e;
        }
        for (i = 1024; i <= 2047; ++i)
        {
            mantissaTable[i] = 0x38000000 + ((i - 1024) << 13);
        }

        tableSetup = gcvTRUE;
    }

    /* Compute lookup indicies. */
    eIndex = (Half & 0x7C00) >> 10;
    mIndex = (Half & 0x03FF) + (eIndex ? 1024 : 0);
    sign   = Half & 0x8000;

    /* Convert half into float. */
    result.u = mantissaTable[mIndex]
             + exponentTable[eIndex]
             + (sign ? 0x80000000U : 0);

    /* Return float. */
    gcmFOOTER_ARG("%f", result.f);
    return result.f;
}


static gceSTATUS
_CRC32(
    IN gcoSTREAM Stream,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctPOINTER Logical OPTIONAL,
    OUT gctUINT32_PTR Crc32 OPTIONAL,
    OUT gctBOOL_PTR Empty OPTIONAL,
    OUT gctSIZE_T_PTR Bytes OPTIONAL
)
{
    gctUINT i, j, k;
    gctUINT32 crc = 0xFFFFFFFF;
    gctUINT8_PTR dst, src[gcdATTRIBUTE_COUNT];
    static gctUINT32 crcTable[256];
    static gctBOOL tableSetup = gcvFALSE;
    gceSTATUS status;
    gctINT c;
    gctFLOAT components[4];
    gctBOOL empty[gcdATTRIBUTE_COUNT];
    gctSIZE_T bytes = 0;
    gctUINT attribStride[gcdATTRIBUTE_COUNT];

    gcmHEADER_ARG("Stream=0x%x AttributeCount=%u Attributes=0x%x First=%u "
                  "Count=%u Logical=0x%x",
                  Stream, AttributeCount, Attributes, First, Count, Logical);

    /* Check if we need to setup the CRC table. */
    if (!tableSetup)
    {
        /* Polynominal widely used for CRC-32 checking. */
        gctUINT32 polynomial = 0x04C11DB7;

        /* Setup the entire table. */
        for (i = 0; i < 256; ++i)
        {
            /* Put the reflection of the index into the table. */
            crcTable[i] = _Reflect(i, 8) << 24;

            /* Shift the table entry by 8 and use the shifted bits to xor in the
            ** polynomial. */
            for (j = 0; j < 8; ++j)
            {
                crcTable[i] = (crcTable[i] << 1)
                            ^ ((crcTable[i] & (1U << 31)) ? polynomial : 0);
            }

            /* Refelect the entire table entry. */
            crcTable[i] = _Reflect(crcTable[i], 32);
        }

        /* CRC-32 table has been setup. */
        tableSetup = gcvTRUE;
    }

    /* Compute all source addresses for the attributes. */
    for (i = 0; i < AttributeCount; ++i)
    {
        attribStride[i] = Attributes[i].vertexPtr->enable
                        ? Attributes[i].vertexPtr->stride
                        : 0;

        /* Add the first vertex. */
        src[i] = (gctUINT8_PTR) Attributes[i].logical
               + First * attribStride[i];

        /* Mark array as empty. */
        empty[i] = ((Empty == gcvNULL) || !Attributes[i].vertexPtr->enable)
                 ? gcvFALSE
                 : gcvTRUE;
    }

    /* Compute the destination address. */
    dst = (gctUINT8_PTR) Logical;

    /* Loop through all vertices. */
	/* Optimization for AttributeCount=1 with Empty=gcvNULL (copy case). */
	if ((Crc32 == gcvNULL)
     && (AttributeCount == 1)
     && (Attributes[0].genericEnable == 0)
     && (Empty == gcvNULL)
     && (attribStride[0] == Attributes[0].bytes)
	 && (dst != gcvNULL)
     )
	{
		/* Just copy the attributes. */
		gcmONERROR(gcoOS_MemCopy(dst, src[0], Count * Attributes[0].bytes));
		bytes += Count * Attributes[0].bytes;
	}
	else
	{
        for (i = 0; i < Count; ++i)
        {
            /* Check if we need to compute the CRC-32 value. */
            if (Crc32 != gcvNULL)
            {
                /* Walk through all attributes. */
                for (j = 0; j < AttributeCount; ++j)
                {
                    /* Walk through all bytes. */
                    for (k = 0; k < Attributes[j].bytes; ++k)
                    {
                        /* Compute the CRC32 value. */
                        crc = (crc >> 8) ^ crcTable[(crc & 0xFF) ^ src[j][k]];
                    }
                }
            }

            /* Check if we need to copy the attributes. */
            if (dst != gcvNULL)
            {
                /* Walk all attributes. */
                for (j = 0; j < AttributeCount; ++j)
                {
                    /* Check if we need to append generic attributes. */
                    if (Attributes[j].genericEnable != 0)
                    {
                        /* This has to be converted to float. */
                        gcmASSERT(Attributes[j].format == gcvVERTEX_FLOAT);

                        /* Copy the generic attribute values. */
                        gcmONERROR(gcoOS_MemCopy(
                            components,
                            Attributes[j].vertexPtr->genericValue,
                            gcmSIZEOF(components)));

                        /* Walk all components. */
                        for (c = 0; c < Attributes[j].vertexPtr->size; ++c)
                        {
                            switch (Attributes[j].vertexPtr->format)
                            {
                            case gcvVERTEX_BYTE:
                                /* Convert the signed char to floating point. */
                                components[c] = (gctFLOAT) ((gctINT8 *) src[j])[c];
                                break;

                            case gcvVERTEX_UNSIGNED_BYTE:
                                /* Convert the unsigned char to floating point. */
                                components[c] = (gctFLOAT) ((gctUINT8 *) src[j])[c];
                                break;

                            case gcvVERTEX_SHORT:
                                /* Convert the signed short to floating point. */
                                components[c] = (gctFLOAT) ((gctINT16 *) src[j])[c];
                                break;

                            case gcvVERTEX_UNSIGNED_SHORT:
                                /* Convert the unsigned short to floating point. */
                                components[c] = (gctFLOAT)
                                                ((gctUINT16 *) src[j])[c];
                                break;

                            case gcvVERTEX_INT:
                            case gcvVERTEX_INT_10_10_10_2:
                                /* Convert the signed int to floating point. */
                                components[c] = (gctFLOAT) ((gctINT32 *) src[j])[c];
                                break;

                            case gcvVERTEX_UNSIGNED_INT:
                            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
                                /* Convert the unsigned int to floating point. */
                                components[c] = (gctFLOAT)
                                                ((gctUINT32 *) src[j])[c];
                                break;

                            case gcvVERTEX_FIXED:
                                /* Convert the fixed point to floating point. */
                                components[c] = gcoMATH_Fixed2Float(
                                                    ((gctFIXED_POINT *) src[j])[c]);
                                break;

                            case gcvVERTEX_HALF:
                                /* Expand the half to floating point. */
                                components[c] = _Half2Float(
                                                    ((gctUINT16_PTR) src[j])[c]);
                                break;

                            case gcvVERTEX_FLOAT:
                                /* Just copy the floating point value. */
                                components[c] = ((gctFLOAT *) src[j])[c];
                                break;
                            }
                        }

                        /* Copy the converted components. */
                        gcmONERROR(gcoOS_MemCopy(dst,
                                                 components,
                                                 Attributes[j].bytes));
                    }
                    else
                    {
                        /* Just copy the attributes. */
                        gcmONERROR(gcoOS_MemCopy(dst, src[j], Attributes[j].bytes));
                    }

                    /* Advance the pointers. */
                    dst   += Attributes[j].bytes;
                    bytes += Attributes[j].bytes;
                }
            }

            /* Adjust the source of all attributes. */
            for (j = 0; j < AttributeCount; ++j)
            {
                /* Check if this attribute is the same as the previous attribute,
                ** but only if all attributes so far are the same. */
                if (Empty != gcvNULL)
                {
                    if (empty[j] && (i > 0))
                    {
                        if (gcoOS_MemCmp(src[j],
                                         src[j] - attribStride[j],
                                         Attributes[j].bytes) != gcvSTATUS_OK)
                        {
                            /* We found a different attribute. */
                            empty[j] = gcvFALSE;
                        }
                    }
                }

                /* Add the stride. */
                src[j] += attribStride[j];
            }
        }
    }

    /* Copy the CRC-32 value. */
    if (Crc32 != gcvNULL)
    {
        *Crc32 = crc;
    }

    /* Copy the empty flag. */
    if (Empty != gcvNULL)
    {
        gctBOOL anyEmpty = empty[0];

        /* Find at least one non-empty array. */
        for (i = 1; !anyEmpty && (i < AttributeCount); ++i)
        {
            anyEmpty |= empty[i];
        }

        /* Return any empty flag. */
        *Empty = anyEmpty;;
    }

    /* Return the number of bytes copied. */
    if (Bytes != gcvNULL)
    {
        *Bytes = bytes;
    }

    /* Success. */
    gcmFOOTER_ARG("*Crc32=0x%08x *Empty=%d *Bytes=%u",
                  gcmOPT_VALUE(Crc32), gcmOPT_VALUE(Empty),
                  gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif
#endif
static gceSTATUS
_NewCache(
    IN gcoSTREAM Stream,
    OUT  gcsSTREAM_CACHE_BUFFER_PTR * Cache
    )
{
    gcsSTREAM_CACHE_BUFFER_PTR cache = gcvNULL;
    gcsHAL_INTERFACE ioctl;
    gceSTATUS status;

    gcmHEADER_ARG("Stream=0x%x", Stream);

    /* Move to the oldest cache. */
    cache = &Stream->cache[Stream->cacheCurrent];

    /* Check if the cache is in use. */
    if (cache->offset > 0)
    {
        /* Mark cache as unavailable. */
        gcmONERROR(gcoOS_Signal(gcvNULL, cache->signal, gcvFALSE));

        /* Schedule a signal event. */
        ioctl.command = gcvHAL_SIGNAL;
        ioctl.u.Signal.signal    = cache->signal;
        ioctl.u.Signal.auxSignal = gcvNULL;
        ioctl.u.Signal.process   = gcoOS_GetCurrentProcessID();
        ioctl.u.Signal.fromWhere = gcvKERNEL_COMMAND;
        gcmONERROR(gcoHARDWARE_CallEvent(&ioctl));

        /* Commit the command buffer. */
        gcmONERROR(gcoHARDWARE_Commit());

        /* Reset the cache. */
        cache->offset = 0;
        cache->free   = cache->bytes;
        cache->index  = 0;

#if !OPT_CACHE_USAGE
        /* Clear the caches. */
        gcmONERROR(gcoOS_ZeroMemory(cache->cacheHash,
                                    gcmSIZEOF(cache->cacheHash)));
        gcmONERROR(gcoOS_ZeroMemory(cache->cacheArray,
                                    gcmSIZEOF(cache->cacheArray)));
#endif


    }

    /* Move to next cache. */
    Stream->cacheCurrent = (Stream->cacheCurrent + 1) % Stream->cacheCount;

    cache = &Stream->cache[Stream->cacheCurrent];

    /* Wait for the cache to become ready. */
    status = gcoOS_WaitSignal(gcvNULL, cache->signal, gcvINFINITE);
    if (status == gcvSTATUS_TIMEOUT)
    {
        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_VERTEX,
                      "Waiting for vertex buffer 0x%x",
                      cache);

        gcmONERROR(gcoOS_WaitSignal(gcvNULL,
                                    cache->signal,
                                    gcvINFINITE));
    }

    /* Return cache. */
    *Cache = cache;

    /* Success. */
    gcmFOOTER_ARG("*Cache=0x%x", *Cache);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}



#if OPT_CACHE_USAGE

static gceSTATUS
_copyBuffers(
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctPOINTER Logical,
    OUT gctSIZE_T_PTR Bytes OPTIONAL
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT i, j, k;
    gctUINT8_PTR src[gcdATTRIBUTE_COUNT], dst;
    gctUINT stride = 0;
    gcsVERTEXARRAY_BUFFER_PTR buffer;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attribute = Attributes;
    gctUINT attribStride[gcdATTRIBUTE_COUNT];
    gctSIZE_T bytes = 0;

    gcmHEADER_ARG("BufferCount=%u Buffers=0x%x AttributeCount=%u "
                  "Attributes=0x%x First=%u Count=%u Logical=0x%x",
                  BufferCount, Buffers, AttributeCount, 
                  Attributes, First, Count, Logical);

    /* Compute the destination address. */
    dst = (gctUINT8_PTR) Logical;

    /* Walk through all buffers. */
    for (i = 0, buffer = Buffers; i < BufferCount; i++, buffer++)
    {
        /* We copy whole range for non-combined buffer. */
        if (!buffer->combined)
        {
            gctUINT size;

            stride = attribute[buffer->map[0]].vertexPtr->enable
                   ? attribute[buffer->map[0]].vertexPtr->stride
                   : 0;

            src[0] = (gctUINT8_PTR)buffer->start + First * stride;
            dst    = (gctUINT8_PTR)Logical + buffer->offset;
            size   = Count * buffer->stride;

            /* Attribute pointer enabled case. */
            if (stride != 0)
            {
                gcmONERROR(gcoOS_MemCopy(dst, src[0], size));
                bytes += size;
            }
            /* Copy from generic value. */
            else
            {
                for (j = 0; j < Count; j++)
                {
                    gcmONERROR(gcoOS_MemCopy(dst, src[0], attribute[buffer->map[0]].bytes));
                    dst   += attribute[buffer->map[0]].bytes;
                    bytes += attribute[buffer->map[0]].bytes;
                }
            }
        }
        /* Combined case, need to pack from all attribute pointers. */
        else
        {
            gctUINT size[gcdATTRIBUTE_COUNT]; 

            dst = (gctUINT8_PTR)Logical + buffer->offset;

            /* Setup source info for all attributes. */
            for (j = 0; j < buffer->count; j++)
            {
                attribStride[j]  = Attributes[buffer->map[j]].vertexPtr->enable
                                 ? Attributes[buffer->map[j]].vertexPtr->stride
                                 : 0;

                /* Add the first vertex. */
                src[j] = (gctUINT8_PTR) Attributes[buffer->map[j]].logical
                       + First * attribStride[j]; 

                size[j] = Attributes[buffer->map[j]].bytes;
            }

            /* Walk through all vertices. */
            for(j = 0; j < Count; j++)
            {
                /* Walk through all attributes in a buffer. */
                for(k = 0; k < buffer->count; k++)
                {
                    gcmONERROR(gcoOS_MemCopy(dst, src[k], size[k]));

                    /* Advance the pointers. */
                    src[k] += attribStride[k];
                    dst    += size[k];
                    bytes  += size[k];
                }
            }
        }
    }

    /* Return the number of bytes copied. */
    if (Bytes != gcvNULL)
    {
        *Bytes = bytes;
    }

    /* Success. */
    gcmFOOTER_ARG("*Bytes=%u", gcmOPT_VALUE(Bytes));
    return gcvSTATUS_OK;

OnError:

    /* Return the status. */
    gcmFOOTER();
    return status;
}


gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT Bytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsSTREAM_CACHE_BUFFER_PTR cache = gcvNULL;
    gctUINT bytes;
    gctUINT offset;
    gctSIZE_T copiedBytes = 0;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u Bytes=%u BufferCount=%u "
                  "Buffers=0x%x AttributeCount=%u Attributes=0x%x",
                  Stream, First, Count, Bytes, BufferCount, Buffers, AttributeCount, Attributes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmDEBUG_VERIFY_ARGUMENT(Count > 0);
    gcmDEBUG_VERIFY_ARGUMENT(BufferCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Buffers != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(AttributeCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Attributes != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Physical != gcvNULL);

    /* Compute number of bytes required. */
    bytes = Bytes * Count;

    /* Index current cache. */
    cache = &Stream->cache[Stream->cacheCurrent];

    /* Check if the data is too big. */
    if (bytes > cache->bytes)
    {     
       gcmONERROR(gcvSTATUS_INVALID_REQUEST);
    }

    /* Check if the stream will fit in the current cache. */
    if (bytes > cache->free)
    {
        /* Move to a new cache. */
        gcmONERROR(_NewCache(Stream, &cache));     
    }

    /* Allocate data form the cache. */
    offset         = cache->offset;
    cache->offset += bytes;
    cache->free   -= bytes;

    /* Copy the data. */
    gcmONERROR(_copyBuffers(
        BufferCount,
        Buffers,
        AttributeCount,
        Attributes,
        First,  Count,
        cache->node.logical + offset,
        &copiedBytes
        ));

    /* Flush the uploaded data. */
    gcmONERROR(gcoSURF_NODE_Cache(&cache->node,
                                cache->node.logical + offset,
                                copiedBytes,
                                gcvCACHE_CLEAN));

    /* Dump the buffer. */
    gcmDUMP_BUFFER(gcvNULL,
                   "stream",
                   cache->node.physical,
                   cache->node.logical,
                   offset,
                   copiedBytes);

    /* Return physical address for stream. */
    *Physical = cache->node.physical + offset;

    /* Success. */
    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else
#if OPT_VERTEX_ARRAY
gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT Bytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT keyValue, key;
    gctUINT i;
    gctINT n;
    gcsSTREAM_CACHE_BUFFER_PTR cache = gcvNULL;
    gcsSTREAM_CACHE_PTR ptr = gcvNULL;
    gctUINT bytes;
    gctUINT crc32, offset;
#if ENABLE_STREAM_CACHE_STATIC
    gctBOOL empty;
#endif
    gctSIZE_T copiedBytes = 0;
    gctUINT32 minBytes = 64;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u Bytes=%u BufferCount=%u "
                  "Buffers=0x%x AttributeCount=%u Attributes=0x%x",
                  Stream, First, Count, Bytes, BufferCount, Buffers, AttributeCount, Attributes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmDEBUG_VERIFY_ARGUMENT(Count > 0);
    gcmDEBUG_VERIFY_ARGUMENT(BufferCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Buffers != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(AttributeCount > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Attributes != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Physical != gcvNULL);

    /* Compute number of bytes required. */
    bytes = Bytes * Count;

    /* Check for small number of vertices and bytes per vertex. */
    if ((Count <= 4) && (bytes <= minBytes))
    {
        /* Index current cache. */
        cache = &Stream->cache[Stream->cacheCurrent];

        /* Check if the data is too big. */
        if (bytes > cache->bytes)
        {
            gcmONERROR(gcvSTATUS_INVALID_REQUEST);
        }

        /* Check if the stream will fit in the current cache. */
        if (bytes > cache->free)
        {
            /* Move to a new cache. */
            gcmONERROR(_NewCache(Stream, &cache));
        }

        /* Allocate data form the cache. */
        offset         = cache->offset;
        cache->offset += bytes;
        cache->free   -= bytes;

        /* Copy the data. */
        gcmONERROR(_CRC32(Stream,
                          BufferCount, Buffers,
                          AttributeCount, Attributes,
                          First, Count,
                          cache->node.logical + offset,
                          gcvNULL,
                          gcvNULL,
                          &copiedBytes));

        /* Flush the uploaded data. */
        gcmONERROR(gcoSURF_NODE_Cache(&cache->node,
                                    cache->node.logical + offset,
                                    copiedBytes,
                                    gcvCACHE_CLEAN));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       "stream",
                       cache->node.physical,
                       cache->node.logical,
                       offset,
                       copiedBytes);

        /* Return physical address for stream. */
        *Physical = cache->node.physical + offset;

        /* Success. */
        gcmFOOTER_ARG("*Physical=0x%08x", *Physical);
        return gcvSTATUS_OK;
    }

    /* Compute hash key. */
    keyValue = ((First + (Count << 16)) ^ ((bytes / Count) + (AttributeCount << 16)))
             + gcmPTR2INT(Attributes[AttributeCount - 1].logical);
    key      = (keyValue ^ (keyValue >> 16)) % gcdSTREAM_CACHE_HASH;

    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_STREAM, "Hash key %u", key);

    /* Walk all caches from the last hit to the current. */
    for (i = 0; i < Stream->cacheCount; ++i)
    {
        /* Get pointer to the stream cache. */
        n     = (Stream->cacheLastHit + i) % Stream->cacheCount;
        cache = &Stream->cache[n];

        /* Check whether the stream is available. */
        status = gcoOS_WaitSignal(gcvNULL, cache->signal, 0);

        if (status == gcvSTATUS_OK)
        {
            /* Check the hash table. */
            for (ptr = cache->cacheHash[key]; ptr != gcvNULL; ptr = ptr->next)
            {
                /* Check if the hash key matches. */
                if ((ptr->key == keyValue)

                /* Check if the attribute count matches. */
                &&  (ptr->attributeCount == AttributeCount)

                /* Check if the attributes match. */
                &&  gcmIS_SUCCESS(
                        gcoOS_MemCmp(Attributes,
                                     ptr->attributes,
                                     ( AttributeCount
                                     * gcmSIZEOF(gcsVERTEXARRAY_ATTRIBUTE)
                                     )))
                )
                {
                    /* We have a cache hit. */
                    Stream->cacheLastHit = n;
                    break;
                }
            }

            if (ptr != gcvNULL)
            {
                /* We have a cache hit. */
                break;
            }
        }

        else if (status != gcvSTATUS_TIMEOUT)
        {
            /* Error. */
            gcmONERROR(status);
        }
    }

    /* Check if we need to create a new cache entry. */
Again:
    if ((cache     == gcvNULL)
    ||  (ptr       == gcvNULL)
    ||  (ptr->type == gcvSTREAM_CACHE_DYNAMIC)
    )
    {
        /* Index current cache. */
        cache = &Stream->cache[Stream->cacheCurrent];

        /* Check if the data is too big. */
        if (bytes > cache->bytes)
        {
            gcmONERROR(gcvSTATUS_INVALID_REQUEST);
        }

        /* Check if the stream will fit in the current cache. */
        if ((cache->free < bytes)
        ||  (  (ptr          == gcvNULL)
            && (cache->index >= gcmCOUNTOF(cache->cacheArray))
            )
        )
        {
            /* Move to a new cache. */
            gcmONERROR(_NewCache(Stream, &cache));

            /* Make sure we move to a new cache. */
            ptr = gcvNULL;
        }

        if (ptr == gcvNULL)
        {
            /* Get next cache slot. */
            ptr = &cache->cacheArray[cache->index];

            /* Test for hash collision. */
            gcmDEBUG_ONLY(
                if (cache->cacheHash[key] != gcvNULL)
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_STREAM,
                                  "Stream=0x%x Cache=0x%x Key=%u Value=%u/%u "
                                  "collision",
                                  Stream, cache, key,
                                  cache->cacheHash[key]->key, keyValue);
                }
            );

            /* Link in hash table. */
            ptr->next             = cache->cacheHash[key];
            cache->cacheHash[key] = ptr;

            /* Copy attributes into cache. */
            ptr->attributeCount = AttributeCount;
            gcmONERROR(gcoOS_MemCopy(
                ptr->attributes,
                Attributes,
                AttributeCount * gcmSIZEOF(gcsVERTEXARRAY_ATTRIBUTE)));

            /* Set buffer information. */
            ptr->offset = cache->offset;

            ptr->key    = keyValue;

            /* Set countdown to prevent assuming data as static on the second
               reentry and allowing it to become static only after countdown
               becomes 0. Conformance test (pntszary.c in particular) calls
               draw function twice with the same set of vertex arrays before
               modifying their content and calling again. */
            ptr->countdown = 1;

            /* Update cache usage. */
            cache->index += 1;
        }
        else if (ptr->type == gcvSTREAM_CACHE_DYNAMIC)
        {
            /* Update pointer to new data. */
            ptr->offset = cache->offset;
        }

        /* Update cache usage. */
        cache->offset += bytes;
        cache->free   -= bytes;
    }

    switch (ptr->type)
    {
    case gcvSTREAM_CACHE_FREE:
        /* We need to copy the data and compute a CRC. */
        gcmONERROR(_CRC32(Stream,
                          BufferCount, Buffers,
                          AttributeCount, Attributes,
                          First, Count,
                          cache->node.logical + ptr->offset,
                          &ptr->crc32,
                          gcvNULL,
                          &copiedBytes));

        /* Flush the uploaded data. */
        gcmONERROR(gcoSURF_NODE_Cache(&cache->node,
                                    cache->node.logical + ptr->offset,
                                    copiedBytes,gcvCACHE_CLEAN));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       "stream",
                       cache->node.physical,
                       cache->node.logical,
                       ptr->offset,
                       copiedBytes);

        /* Mark the data as copied. */
        ptr->type  = gcvSTREAM_CACHE_COPIED;
        ptr->bytes = copiedBytes;
        break;

    case gcvSTREAM_CACHE_COPIED:
#if ENABLE_STREAM_CACHE_STATIC
        /* Compute a CRC of the data. */
        gcmONERROR(_CRC32(Stream,
                          BufferCount, Buffers,
                          AttributeCount, Attributes,
                          First, Count,
                          gcvNULL,
                          &crc32,
                          &empty,
                          gcvNULL));

        /* Verify CRC. */
        if (crc32 != ptr->crc32)
        {
            /* Mark data as dynamic. */
            ptr->type = gcvSTREAM_CACHE_DYNAMIC;
            goto Again;
        }
        else if (!empty)
        {
            /* Time to mark as static? */
            if (ptr->countdown == 0)
            {
                /* The buffer remained constant for the specified
                   number of times, mark as static. */
                ptr->type = gcvSTREAM_CACHE_STATIC;
            }
            else
            {
                /* Update countdown. */
                ptr->countdown -= 1;
            }
        }
#else
        /* Compute a CRC of the data. */
        gcmONERROR(_CRC32(Stream,
                          BufferCount, Buffers,
                          AttributeCount, Attributes,
                          First, Count,
                          gcvNULL,
                          &crc32,
                          gcvNULL,
                          gcvNULL));

        /* Verify CRC. */
        if (crc32 != ptr->crc32)
        {
            /* Mark data as dynamic. */
            ptr->type = gcvSTREAM_CACHE_DYNAMIC;
            goto Again;
        }
#endif
        break;

    case gcvSTREAM_CACHE_STATIC:
        /* If the vertex buffer is big enough, we keep it static. */
        if (ptr->bytes > minBytes)
        {
            /* Nothing to do. */
            break;
        }

        /* Fall through. */

    case gcvSTREAM_CACHE_DYNAMIC:
        /* Copy the data. */
        gcmONERROR(_CRC32(Stream,
                          BufferCount, Buffers,
                          AttributeCount, Attributes,
                          First, Count,
                          cache->node.logical + ptr->offset,
                          gcvNULL,
                          gcvNULL,
                          &copiedBytes));

        /* Flush the uploaded data. */
        gcmONERROR(gcoSURF_NODE_Cache(&cache->node,
                                    cache->node.logical + ptr->offset,
                                    copiedBytes, gcvCACHE_CLEAN));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       "stream",
                       cache->node.physical,
                       cache->node.logical,
                       ptr->offset,
                       copiedBytes);
        break;
    }

    /* Return physical address for stream. */
    *Physical = cache->node.physical + ptr->offset;

    /* Success. */
    gcmFOOTER_ARG("*Physical=0x%08x", *Physical);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else

gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT Stride,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    gceSTATUS status;
    gctUINT keyValue, key;
    gctUINT i;
    gctINT n;
    gcsSTREAM_CACHE_BUFFER_PTR cache = gcvNULL;
    gcsSTREAM_CACHE_PTR ptr = gcvNULL;
    gctUINT bytes;
    gctUINT crc32, offset;
    gctBOOL empty;
    gctSIZE_T copiedBytes = 0;
    gctUINT32 minBytes = 64;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u Stride=%u AttributeCount=%u "
                  "Attributes=0x%x",
                  Stream, First, Count, Stride, AttributeCount, Attributes);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Count > 0);
    gcmVERIFY_ARGUMENT(AttributeCount > 0);
    gcmVERIFY_ARGUMENT(Attributes != gcvNULL);
    gcmVERIFY_ARGUMENT(Physical != gcvNULL);

    /* Compute number of bytes required. */
    bytes = Count * Stride;

    /* Check for small number of vertices and bytes per vertex. */
    if ((Count <= 4) && (bytes <= minBytes))
    {
        /* Index current cache. */
        cache = &Stream->cache[Stream->cacheCurrent];

        /* Check if the data is too big. */
        if (bytes > cache->bytes)
        {
            gcmONERROR(gcvSTATUS_INVALID_REQUEST);
        }

        /* Check if the stream will fit in the current cache. */
        if (bytes > cache->free)
        {
            /* Move to a new cache. */
            gcmONERROR(_NewCache(Stream, &cache));
        }

        /* Allocate data form the cache. */
        offset         = cache->offset;
        cache->offset += bytes;
        cache->free   -= bytes;

        /* Copy the data. */
        gcmONERROR(_CRC32(Stream,
                          AttributeCount, Attributes,
                          First, Count,
                          cache->node.logical + offset,
                          gcvNULL,
                          gcvNULL,
                          &copiedBytes));

        /* Flush the uploaded data. */
        gcmONERROR(gcoSURF_NODE_Cache(&cache->node,
                                    cache->node.logical + offset,
                                    copiedBytes,
                                    gcvCACHE_CLEAN));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       "stream",
                       cache->node.physical,
                       cache->node.logical,
                       offset,
                       copiedBytes);

        /* Return physical address for stream. */
        *Physical = cache->node.physical + offset;

        /* Success. */
        gcmFOOTER_ARG("*Physical=0x%08x", *Physical);
        return gcvSTATUS_OK;
    }

    /* Compute hash key. */
    keyValue = ((First + (Count << 16)) ^ (Stride + (AttributeCount << 16)))
             + gcmPTR2INT(Attributes[AttributeCount - 1].logical);
    key      = (keyValue ^ (keyValue >> 16)) % gcdSTREAM_CACHE_HASH;
    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_STREAM, "Hash key %u", key);

    /* Walk all caches from the last hit to the current. */
    for (i = 0; i < Stream->cacheCount; ++i)
    {
        /* Get pointer to the stream cache. */
        n     = (Stream->cacheLastHit + i) % Stream->cacheCount;
        cache = &Stream->cache[n];

        /* Check whether the stream is available. */
        status = gcoOS_WaitSignal(gcvNULL, cache->signal, 0);

        if (status == gcvSTATUS_OK)
        {
            /* Check the hash table. */
            for (ptr = cache->cacheHash[key]; ptr != gcvNULL; ptr = ptr->next)
            {
                /* Check if the hash key matches. */
                if ((ptr->key == keyValue)

                /* Check if the attribute count matches. */
                &&  (ptr->attributeCount == AttributeCount)

                /* Check if the attributes match. */
                &&  gcmIS_SUCCESS(
                        gcoOS_MemCmp(Attributes,
                                     ptr->attributes,
                                     ( AttributeCount
                                     * gcmSIZEOF(gcsVERTEXARRAY_ATTRIBUTE)
                                     )))
                )
                {
                    /* We have a cache hit. */
                    Stream->cacheLastHit = n;
                    break;
                }
            }

            if (ptr != gcvNULL)
            {
                /* We have a cache hit. */
                break;
            }
        }

        else if (status != gcvSTATUS_TIMEOUT)
        {
            /* Error. */
            gcmONERROR(status);
        }
    }

    /* Check if we need to create a new cache entry. */
Again:
    if ((cache     == gcvNULL)
    ||  (ptr       == gcvNULL)
    ||  (ptr->type == gcvSTREAM_CACHE_DYNAMIC)
    )
    {
        /* Index current cache. */
        cache = &Stream->cache[Stream->cacheCurrent];

        /* Check if the data is too big. */
        if (bytes > cache->bytes)
        {
            gcmONERROR(gcvSTATUS_INVALID_REQUEST);
        }

        /* Check if the stream will fit in the current cache. */
        if ((cache->free < bytes)
        ||  (  (ptr          == gcvNULL)
            && (cache->index >= gcmCOUNTOF(cache->cacheArray))
            )
        )
        {
            /* Move to a new cache. */
            gcmONERROR(_NewCache(Stream, &cache));

            /* Make sure we move to a new cache. */
            ptr = gcvNULL;
        }

        if (ptr == gcvNULL)
        {
            /* Get next cache slot. */
            ptr = &cache->cacheArray[cache->index];

            /* Test for hash collision. */
            gcmDEBUG_ONLY(
                if (cache->cacheHash[key] != gcvNULL)
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_STREAM,
                                  "Stream=0x%x Cache=0x%x Key=%u Value=%u/%u "
                                  "collision",
                                  Stream, cache, key,
                                  cache->cacheHash[key]->key, keyValue);
                }
            );

            /* Link in hash table. */
            ptr->next             = cache->cacheHash[key];
            cache->cacheHash[key] = ptr;

            /* Copy attributes into cache. */
            ptr->attributeCount = AttributeCount;
            gcmONERROR(gcoOS_MemCopy(
                ptr->attributes,
                Attributes,
                AttributeCount * gcmSIZEOF(gcsVERTEXARRAY_ATTRIBUTE)));

            /* Set buffer information. */
            ptr->offset = cache->offset;
            ptr->stride = Stride;
            ptr->key    = keyValue;

            /* Set countdown to prevent assuming data as static on the second
               reentry and allowing it to become static only after countdown
               becomes 0. Conformance test (pntszary.c in particular) calls
               draw function twice with the same set of vertex arrays before
               modifying their content and calling again. */
            ptr->countdown = 1;

            /* Update cache usage. */
            cache->index += 1;
        }
        else if (ptr->type == gcvSTREAM_CACHE_DYNAMIC)
        {
            /* Update pointer to new data. */
            ptr->offset = cache->offset;
        }

        /* Update cache usage. */
        cache->offset += bytes;
        cache->free   -= bytes;
    }

    switch (ptr->type)
    {
    case gcvSTREAM_CACHE_FREE:
        /* We need to copy the data and compute a CRC. */
        gcmONERROR(_CRC32(Stream,
                          AttributeCount, Attributes,
                          First, Count,
                          cache->node.logical + ptr->offset,
                          &ptr->crc32,
                          gcvNULL,
                          &copiedBytes));

        /* Flush the uploaded data. */
        gcmONERROR(gcoSURF_NODE_Cache(&cache->node,
                                    cache->node.logical + ptr->offset,
                                    copiedBytes,gcvCACHE_CLEAN));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       "stream",
                       cache->node.physical,
                       cache->node.logical,
                       ptr->offset,
                       copiedBytes);

        /* Mark the data as copied. */
        ptr->type  = gcvSTREAM_CACHE_COPIED;
        ptr->bytes = copiedBytes;
        break;

    case gcvSTREAM_CACHE_COPIED:
        /* Compute a CRC of the data. */
        gcmONERROR(_CRC32(Stream,
                          AttributeCount, Attributes,
                          First, Count,
                          gcvNULL,
                          &crc32,
                          &empty,
                          gcvNULL));

        /* Verify CRC. */
        if (crc32 != ptr->crc32)
        {
            /* Mark data as dynamic. */
            ptr->type = gcvSTREAM_CACHE_DYNAMIC;
            goto Again;
        }
#if ENABLE_STREAM_CACHE_STATIC
        else if (!empty)
        {
            /* Time to mark as static? */
            if (ptr->countdown == 0)
            {
                /* The buffer remained constant for the specified
                   number of times, mark as static. */
                ptr->type = gcvSTREAM_CACHE_STATIC;
            }
            else
            {
                /* Update countdown. */
                ptr->countdown -= 1;
            }
        }
#endif
        break;

    case gcvSTREAM_CACHE_STATIC:
        /* If the vertex buffer is big enough, we keep it static. */
        if (ptr->bytes > minBytes)
        {
            /* Nothing to do. */
            break;
        }

        /* Fall through. */

    case gcvSTREAM_CACHE_DYNAMIC:
        /* Copy the data. */
        gcmONERROR(_CRC32(Stream,
                          AttributeCount, Attributes,
                          First, Count,
                          cache->node.logical + ptr->offset,
                          gcvNULL,
                          gcvNULL,
                          &copiedBytes));

        /* Flush the uploaded data. */
        gcmONERROR(gcoSURF_NODE_Cache(&cache->node,
                                    cache->node.logical + ptr->offset,
                                    copiedBytes, gcvCACHE_CLEAN));

        /* Dump the buffer. */
        gcmDUMP_BUFFER(gcvNULL,
                       "stream",
                       cache->node.physical,
                       cache->node.logical,
                       ptr->offset,
                       copiedBytes);
        break;
    }

    /* Return physical address for stream. */
    *Physical = cache->node.physical + ptr->offset;

    /* Success. */
    gcmFOOTER_ARG("*Physical=0x%08x", *Physical);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif
#endif

gceSTATUS
gcoSTREAM_UnAlias(
    IN gcoSTREAM Stream,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream,
    OUT gctUINT8_PTR * Logical,
    OUT gctUINT32 * Physical
    )
{
    gceSTATUS status;
    gcoSTREAM stream = gcvNULL;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attr[2];

    gcmHEADER_ARG("Stream=0x%x Attributes=0x%x", Stream, Attributes);

    /* Setup quick pointers. */
    attr[0] = Attributes;
    attr[1] = Attributes->next;

    /* Special case 2 attribites. */
    if ((attr[1]       != gcvNULL)
    &&  (attr[1]->next == gcvNULL)

    /* Streams pointing at the same stream with a stride less than 8. */
    &&  (Stream                       == attr[0]->vertexPtr->stream)
    &&  (Stream                       == attr[1]->vertexPtr->stream)
    &&  (Stream->subStreamCount       == 1)
    &&  (Stream->subStreams[0].stride <  8)

    /* Attribute 1 aliases attribute 0. */
    &&  (attr[1]->offset == attr[0]->offset)
    &&  (attr[1]->bytes  == attr[0]->bytes)
    )
    {
        /* Check if we already rebuilt this stream. */
        if (Stream->rebuild == gcvNULL)
        {
            gctUINT8_PTR src, dst;
            gctSIZE_T bytes;
            gctUINT stride;

            /* Construct a new stream. */
            gcmONERROR(gcoSTREAM_Construct(gcvNULL, &stream));

            /* Reserve twice the amount of data. */
            gcmONERROR(gcoSTREAM_Reserve(stream, Stream->size * 2));

            /* Get source stride. */
            stride = Stream->subStreams[0].stride;

            /* Set destination stride to twice the source stride. */
            stream->stride = stride * 2;

            /* Get pointers to streams. */
            src = Stream->node.logical;
            dst = stream->node.logical;

            /* Copy all vertices. */
			/* Aligned case optimization. */
			if ((stride == 4) && !((gctUINT32)src & 0x3) && !((gctUINT32)dst & 0x3))
			{
				gctUINT32_PTR tempSrc, tempDst;
				tempSrc = (gctUINT32_PTR)src;
				tempDst = (gctUINT32_PTR)dst;

				for (bytes = Stream->size; bytes > 0; bytes -= stride)
				{
					/* Copy vertex. */
					*tempDst++ = *tempSrc;

					/* Copy vertex again. */
					*tempDst++ = *tempSrc++;
				}
			}
			else
			{
				for (bytes = Stream->size; bytes > 0; bytes -= stride)
				{
					/* Copy vertex. */
					gcmONERROR(gcoOS_MemCopy(dst, src, stride));

					/* Copy vertex again. */
					gcmONERROR(gcoOS_MemCopy(dst + stride, src, stride));

					/* Advance pointers. */
					src += stride;
					dst += stride * 2;
				}
			}

            /* Flush the vertex cache. */
            gcmONERROR(gcoSTREAM_Flush(stream));

            /* Flush the vertex buffer cache. */
            gcmONERROR(gcoSURF_NODE_Cache(&stream->node,
                                        stream->node.logical,
                                        stream->size,gcvCACHE_CLEAN));

            /* Copy sub-stream. */
            stream->subStreamCount = 1;
            gcmONERROR(gcoOS_MemCopy(stream->subStreams,
                                     Stream->subStreams,
                                     gcmSIZEOF(gcsSTREAM_SUBSTREAM)));

            /* Setup unaliased sub-stream. */
            stream->subStreams[1].start  = 0;
            stream->subStreams[1].end    = stream->stride;
            stream->subStreams[1].stride = stream->stride;
            stream->subStreams[1].next   = gcvNULL;

            /* Save new stream as a rebuilt stream. */
            Stream->rebuild = stream;
        }
        else
        {
            /* Stream has already been rebuilt. */
            stream = Stream->rebuild;
        }

        /* Update mapping. */
        attr[0]->logical = stream->node.logical + attr[0]->offset;
        attr[1]->offset += stream->stride / 2;
        attr[1]->logical = stream->node.logical + attr[1]->offset;

        /* Return new rebuilt stream. */
        *SubStream = &stream->subStreams[1];
        *Logical   = stream->node.logical;
        *Physical  = stream->node.physical;
    }
    else
    {
        /* Skip the stream unaliasing. */
        gcmFOOTER_NO();
        return gcvSTATUS_SKIP;
    }

    /* Success. */
    gcmFOOTER_ARG("*Logical=0x%x *Physical=0x%08x", *Logical, *Physical);
    return gcvSTATUS_OK;

OnError:
    if (stream != gcvNULL)
    {
        /* Detsroy newly created stream. */
        gcmVERIFY_OK(gcoSTREAM_Destroy(stream));
    }

    /* Return error. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_MergeStreams(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams,
    OUT gcoSTREAM * MergedStream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical,
    OUT gcsVERTEXARRAY_ATTRIBUTE_PTR * Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    )
{
    gceSTATUS status;
    gctUINT i, j, stride, index;
    gctUINT8_PTR src[256];
    gctUINT8_PTR dest;
    gcoSTREAM merged = gcvNULL;
    gctBOOL copy;
    gctUINT count = First + Count;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attribute, last = gcvNULL;
    gcsSTREAM_SUBSTREAM_PTR sub, subPtr;
    gctINT oldValue;
    gctUINT mergeStride[256];
    gctBOOL pack = gcvFALSE;
    gctUINT maxStride;
    gctUINT subMap[256]  = {0};
    gctBOOL mergeByChain = gcvFALSE;

    gcmHEADER_ARG("Stream=0x%x First=%u Count=%u StreamCount=%u Streams=0x%x",
                  Stream, First, Count, StreamCount, Streams);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);
    gcmVERIFY_ARGUMENT(Count > 0);
    gcmVERIFY_ARGUMENT(StreamCount > 0);
    gcmVERIFY_ARGUMENT(Streams != gcvNULL);
    gcmVERIFY_ARGUMENT(MergedStream != gcvNULL);
    gcmVERIFY_ARGUMENT(Logical != gcvNULL);
    gcmVERIFY_ARGUMENT(Physical != gcvNULL);
    gcmVERIFY_ARGUMENT(Attributes != gcvNULL);
    gcmVERIFY_ARGUMENT(SubStream != gcvNULL);

    /* Check if there already is a merged stream. */
    if (Stream->merged == gcvNULL)
    {
        /* Construct a new stream. */
        gcmONERROR(gcoSTREAM_Construct(gcvNULL, &merged));

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_STREAM,
                      "Created a new merged stream 0x%x",
                      merged);

        /* We surely need to copy data! */
        copy = gcvTRUE;

        /* Construct a reference counter. */
        gcmONERROR(gcoOS_AtomConstruct(gcvNULL, &merged->reference));
    }
    else
    {
        /* Get the merged stream. */
        merged = Stream->merged;

        /* Only copy if one of the streams is dirty or we havne't copied enough
        ** vertex data yet. */
        copy = merged->dirty || (count > merged->count);

        gcmDEBUG_ONLY(
            if (copy)
            {
                if (merged->dirty)
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_STREAM,
                                  "Merged stream 0x%x is dirty",
                                  merged);
                }
                else
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_STREAM,
                                  "Merged stream 0x%x now holds %u vertices",
                                  merged, count);
                }
            }
        );

        if (!copy)
        {
            /* Count the number of substreams. */
            for (i = 0, index = 0; i < StreamCount; ++i)
            {
                index += Streams[i].stream->subStreamCount;
            }

            /* Check if the number of substreams is larger. */
            if (index > merged->subStreamCount)
            {
                /* We need to copy. */
                copy = gcvTRUE;

                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_STREAM,
                              "Merged stream 0x%x now holds %u streams",
                              merged, index);
            }

            /* Check if any of the incoming streams was never merged. */
            for (i = 0, index = 0; i < StreamCount; ++i)
            {
                if (Streams[i].stream->merged == gcvNULL)
                {
                    /* We need to copy. */
                    copy = gcvTRUE;
                    break;
                }

                /* All streams should merge into 1 merged stream. */
                gcmASSERT(Streams[i].stream->merged == merged);
            }
        }

        /* Check if we need to copy. */
        if (copy)
        {
            do
            {
                /* Decrement the reference counter. */
                gcmONERROR(gcoOS_AtomDecrement(gcvNULL,
                                               merged->reference,
                                               &oldValue));
            }
            while (oldValue > 1);
        }
    }

    /* Check if we need to copy data. */
    if (copy)
    {
        /* Reset everything. */
        index  = 0;
        stride = 0;
        sub    = merged->subStreams;
        /* reset merged pointer for all streams previously merged. */
        for (i = 0; i < merged->subStreamCount; i++)
        {
            merged->subStreams[i].stream->merged = gcvNULL;
            merged->subStreams[i].stream = gcvNULL;
        }

        gcmONERROR(
            gcoHARDWARE_QueryStreamCaps(gcvNULL,
            &maxStride,
            gcvNULL,
            gcvNULL));

        /* Check should we do pack */
        for(i = 0; i < StreamCount; ++i)
        {
            for (j = 0; j < Streams[i].stream->subStreamCount; j++)
            {
                if(Streams[i].stream->subStreams[j].stride > maxStride)
                {
                    pack = gcvTRUE;
                    break;
                }
            }

            if(pack)
            {
                break;
            }
        }

        /* Walk all the streams. */
        for (i = 0; i < StreamCount; ++i)
        {
            gctUINT sortMap[256]={0};
            gctUINT k, l, sortCount;

            /* Reset sorted count before sort a stream. */
            sortCount = 0;

            /* Sort sub streams in a stream by offset. */
            for (j = 0; j < Streams[i].stream->subStreamCount; j++)
            {
                /* Compare offset with sub-streams in the sorted map. */
                for (k = 0; k < sortCount; k++)
                {
                    /* Find the position. */
                    if ( Streams[i].stream->subStreams[j].start
                         < Streams[i].stream->subStreams[sortMap[k]].start
                       )
                    {
                        /* Shift the sorted array for new sub-stream. */
                        for(l = sortCount; l > k; l--)
                        {
                            sortMap[l] = sortMap[l - 1];
                        }

                        /* Insert the sub stream. */
                        sortMap[k] = j;
                        sortCount++;
                        break;
                    }
                }

                /* Append to the end. */
                if (k == sortCount)
                {
                    sortMap[k] = j;
                    sortCount++;
                }
            }

            /* Walk all the substreams in the stream. */
            for (j = 0; j < Streams[i].stream->subStreamCount; ++j)
            {
                /* Copy the substream information. */
                sub->start    = Streams[i].stream->subStreams[sortMap[j]].start;
                sub->stride   = Streams[i].stream->subStreams[sortMap[j]].stride;
                sub->end      = sub->start + sub->stride;
                sub->minStart = stride;
                sub->stream   = Streams[i].stream;

                /* Get pointer to vertex data. */
                src[index] = Streams[i].stream->node.logical + sub->start;

                if(pack)
                {
                    mergeStride[index] = Streams[i].stream->subStreams[sortMap[j]].end
                        - Streams[i].stream->subStreams[sortMap[j]].start;
                }
                else
                {
                    mergeStride[index] = sub->stride;
                }

                /* Adjust stride. */
                stride += mergeStride[index];

                /* Next substream. */
                ++sub;
                ++index;
            }

            /* Increment the reference counter. */
            gcmONERROR(gcoOS_AtomIncrement(gcvNULL,
                                           merged->reference,
                                           &oldValue));

            /* Mark the stream as a client of this merged stream. */
            Streams[i].stream->merged = merged;
        }

        /* If stride excced max stride, only merge the neeeded sub streams .*/
        if (stride > maxStride)
        {
            /* reset merged pointer for all streams. */
            for (i = 0; i < index; i++)
            {
                merged->subStreams[i].stream->merged = gcvNULL;
                merged->subStreams[i].stream = gcvNULL;
            }

            do
            {
                /* Decrement the reference counter. */
                gcmONERROR(gcoOS_AtomDecrement(gcvNULL,
                                               merged->reference,
                                               &oldValue));
            }
            while (oldValue > 1);

            /* Reset everything. */
            index  = 0;
            stride = 0;
            sub    = merged->subStreams;

            for (i = 0; i < StreamCount; ++i)
            {
                /* Walk all the substreams in the stream. */
                for (subPtr = Streams[i].subStream; subPtr != gcvNULL; subPtr = subPtr->next)
                {
                    /* Copy the substream information. */
                    sub->start    = subPtr->start;
                    sub->stride   = subPtr->stride;
                    sub->end      = sub->start + sub->stride;
                    sub->minStart = stride;
                    sub->stream   = Streams[i].stream;

                    /* Get pointer to vertex data. */
                    src[index] = Streams[i].stream->node.logical + sub->start;

                    if(pack)
                    {
                        mergeStride[index] = sub->end - sub->start;
                    }
                    else
                    {
                        mergeStride[index] = sub->stride;
                    }

                    /* Adjust stride. */
                    stride += mergeStride[index];

                    /* Next substream. */
                    ++sub;
                    ++index;
                }

                subMap[i] = index;

                /* Increment the reference counter. */
                gcmONERROR(gcoOS_AtomIncrement(gcvNULL,
                                               merged->reference,
                                               &oldValue));

                /* Mark the stream as a client of this merged stream. */
                Streams[i].stream->merged = merged;
            }

            /* Check if stride meet hardware capability. */
            if (stride <= maxStride)
            {
                mergeByChain = gcvTRUE;
            }
            else
            {
                gcmONERROR(gcvSTATUS_TOO_COMPLEX);
            }
        }

        /* Initialize the last substream for hardware. */
        sub->start    = 0;
        sub->minStart = 0;
        sub->end      = stride;
        sub->maxEnd   = stride;
        sub->stride   = stride;
        sub->next     = gcvNULL;

        /* Set substream information. */
        merged->subStreamCount  = index;
        merged->subStreamStride = stride;

        /* Get the maximum of the saved count versus the current count. */
        if (count > merged->count)
        {
            merged->count = count;
        }
        else
        {
            count = merged->count;
        }

        /* Reserve enough data in the merged stream buffer. */
        gcmONERROR(_FreeMemory(merged));
        gcmONERROR(gcoSTREAM_Reserve(merged, stride * count));

        /* Setup destination pointer. */
        dest = merged->node.logical;

        /* Walk all the verticies. */
        for (i = 0; i < count; ++i)
        {
            /* Walk all the substreams. */
            for (j = 0; j < index; ++j)
            {
                /* Copy data from the setram to the merged stream. */
                gcmONERROR(gcoOS_MemCopy(dest,
                                         src[j],
                                         mergeStride[j]));

                /* Move to next substream. */
                src[j] += merged->subStreams[j].stride;
                dest   += mergeStride[j];
            }
        }

        /* Flush the vertex cache. */
        gcmONERROR(gcoSTREAM_Flush(merged));

        /* Flush the CPU cache . */
        gcmONERROR(gcoSURF_NODE_Cache(&merged->node,
                                    merged->node.logical,
                                    merged->size,
                                    gcvCACHE_CLEAN));

        /* Merged stream has been copied. */
        merged->dirty = gcvFALSE;
    }

    /* Walk all the streams. */
    for (i = 0, index = 0; i < StreamCount; ++i)
    {
        /* Walk all the attributes in the stream. */
        for (attribute = Streams[i].attribute;
             attribute != gcvNULL;
             attribute = attribute->next
        )
        {
            /* Get pointer to the mapped substreams for this stream. */
            sub = &merged->subStreams[index];

            /* Walk all the substreams. */
            for (j = 0; j < Streams[i].stream->subStreamCount; ++j, ++sub)
            {
                /* Check if this attribute is part of this substream. */
                if ((sub->start <= attribute->offset)
                &&  (sub->end   >= attribute->offset)
                )
                {
                    /* Adjust offset of the attribute. */
                    attribute->offset = attribute->offset - sub->start
                                      + sub->minStart;
                    break;
                }
            }

            /* Check for error. */
            if (j == Streams[i].stream->subStreamCount)
            {
                gcmONERROR(gcvSTATUS_TOO_COMPLEX);
            }

            /* Append this attribte to the chain. */
            if (last != gcvNULL)
            {
                last->next = attribute;
            }

            /* Mark this attribute as the last. */
            last = attribute;
        }

        if (mergeByChain)
        {
            /* Move to the next substream block. */
            index += subMap[i];
        }
        else
        {
            /* Move to the next substream block. */
            index += Streams[i].stream->subStreamCount;
        }
    }

    /* Return information about the merged stream. */
    *MergedStream = merged;
    *Logical      = merged->node.logical;
    *Physical     = merged->node.physical;
    *Attributes   = Streams->attribute;
    *SubStream    = &merged->subStreams[merged->subStreamCount];

    /* Success. */
    gcmFOOTER_ARG("*MergedStream=0x%x *Logical=0x%x *Physical=0x%08x "
                  "*Attributes=0x%x *SubStream=0x%x",
                  *MergedStream, *Logical, *Physical, *Attributes, *SubStream);
    return gcvSTATUS_OK;

OnError:
    if (merged != gcvNULL)
    {
        /* Destroy the created merged stream. */
        gcmVERIFY_OK(gcoSTREAM_Destroy(merged));
    }

    for (i = 0; i < StreamCount; ++i)
    {
        /* Remove merged stream. */
        Streams[i].stream->merged = gcvNULL;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoSTREAM_CPUCacheOperation(
    IN gcoSTREAM Stream,
    IN gceCACHEOPERATION Operation
    )
{
    gceSTATUS status;
    gctPOINTER memory;
    gctBOOL locked = gcvFALSE;

    gcmHEADER_ARG("Stream=0x%x, Operation=%d", Stream, Operation);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Stream, gcvOBJ_STREAM);

    /* Lock the node. */
    gcmONERROR(gcoHARDWARE_Lock(&Stream->node, gcvNULL, &memory));
    locked = gcvTRUE;

    gcmONERROR(gcoSURF_NODE_Cache(&Stream->node,
                                  memory,
                                  Stream->size,
                                  Operation));


    /* Unlock the node. */
    gcmONERROR(gcoHARDWARE_Unlock(&Stream->node, gcvSURF_VERTEX));
    locked = gcvFALSE;

    gcmFOOTER();
    return gcvSTATUS_OK;

OnError:
    if (locked)
    {
        gcmVERIFY_OK(gcoHARDWARE_Unlock(&Stream->node, gcvSURF_VERTEX));
    }

    gcmFOOTER();
    return status;
}
#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS gcoSTREAM_Construct(
    IN gcoHAL Hal,
    OUT gcoSTREAM * Stream
    )
{
    *Stream = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Destroy(
    IN gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Upload(
    IN gcoSTREAM Stream,
    IN gctCONST_POINTER Buffer,
    IN gctUINT32 Offset,
    IN gctSIZE_T Bytes,
    IN gctBOOL Dynamic
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_SetStride(
    IN gcoSTREAM Stream,
    IN gctUINT32 Stride
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Lock(
    IN gcoSTREAM Stream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical
    )
{
    *Logical = gcvNULL;
    *Physical = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Unlock(
    IN gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Reserve(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Bytes
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEX * Vertex
    )
{
    *Vertex = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_Destroy(
    IN gcoVERTEX Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_Reset(
    IN gcoVERTEX Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_EnableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index,
    IN gceVERTEX_FORMAT Format,
    IN gctBOOL Normalized,
    IN gctUINT32 Components,
    IN gcoSTREAM Stream,
    IN gctUINT32 Offset,
    IN gctUINT32 Stride
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_DisableAttribute(
    IN gcoVERTEX Vertex,
    IN gctUINT32 Index
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEX_Bind(
    IN gcoVERTEX Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_SetDynamic(
    IN gcoSTREAM Stream,
    IN gctSIZE_T Bytes,
    IN gctUINT Buffers
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_UploadDynamic(
    IN gcoSTREAM Stream,
    IN gctUINT VertexCount,
    IN gctUINT InfoCount,
    IN gcsSTREAM_INFO_PTR Info,
    IN gcoVERTEX Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_SetAttribute(
    IN gcoSTREAM Stream,
    IN gctUINT Offset,
    IN gctUINT Bytes,
    IN gctUINT Stride,
    IN OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_QuerySubStreams(
    IN gcoSTREAM Stream,
    IN gcsSTREAM_SUBSTREAM_PTR SubStream,
    OUT gctUINT_PTR SubStreamCount
    )
{
    *SubStreamCount = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_Rebuild(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    OUT gctUINT_PTR SubStreamCount
    )
{
    *SubStreamCount = 0;
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_SetCache(
    IN gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

#if OPT_VERTEX_ARRAY
gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT Bytes,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    *Physical = 0;
    return gcvSTATUS_OK;
}

#else

gceSTATUS
gcoSTREAM_CacheAttributes(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT Stride,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gctUINT32_PTR Physical
    )
{
    *Physical = 0;
    return gcvSTATUS_OK;
}
#endif

gceSTATUS gcoSTREAM_UnAlias(
    IN gcoSTREAM Stream,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream,
    OUT gctUINT8_PTR * Logical,
    OUT gctUINT32 * Physical
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoSTREAM_MergeStreams(
    IN gcoSTREAM Stream,
    IN gctUINT First,
    IN gctUINT Count,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams,
    OUT gcoSTREAM * MergedStream,
    OUT gctPOINTER * Logical,
    OUT gctUINT32 * Physical,
    OUT gcsVERTEXARRAY_ATTRIBUTE_PTR * Attributes,
    OUT gcsSTREAM_SUBSTREAM_PTR * SubStream
    )
{
    *MergedStream = gcvNULL;
    *Logical = gcvNULL;
    *Physical = 0;
    *Attributes = gcvNULL;
    *SubStream = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_Flush(
    gcoSTREAM Stream
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoSTREAM_CPUCacheOperation(
    IN gcoSTREAM Stream,
    IN gceCACHEOPERATION Operation
    )
{
    return gcvSTATUS_OK;
}
#endif /* gcdNULL_DRIVER < 2 */
#endif /* VIVANTE_NO_3D */
