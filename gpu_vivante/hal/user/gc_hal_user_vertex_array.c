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

#define _GC_OBJ_ZONE            gcvZONE_VERTEX

/*******************************************************************************
***** gcoVERTEX Object ********************************************************/

struct _gcoVERTEXARRAY
{
    /* The object. */
    gcsOBJECT                   object;

    /* Hardware capabilities. */
    gctUINT                     maxAttribute;
    gctUINT                     maxSize;
    gctUINT                     maxStreams;

    /* Dynamic streams. */
    gcoSTREAM                   dynamicStream;
    gcoINDEX                    dynamicIndex;
};

gceSTATUS
gcoVERTEXARRAY_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEXARRAY * Vertex
    )
{
    gceSTATUS status;
    gcoVERTEXARRAY vertexPtr = gcvNULL;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Vertex != gcvNULL);

    /* Allocate the gcoVERTEXARRAY object structure. */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              gcmSIZEOF(struct _gcoVERTEXARRAY),
                              &pointer));

    vertexPtr = pointer;

    /* Initialize the object. */
    vertexPtr->object.type = gcvOBJ_VERTEX;

    /* Initialize the structure. */
    vertexPtr->dynamicStream = gcvNULL;
    vertexPtr->dynamicIndex  = gcvNULL;

    /* Construct a cacheable stream. */
    gcmONERROR(gcoSTREAM_Construct(gcvNULL, &vertexPtr->dynamicStream));
    gcmONERROR(gcoSTREAM_SetCache(vertexPtr->dynamicStream));

    /* Construct a dynamic index. */
    gcmONERROR(gcoINDEX_Construct(gcvNULL, &vertexPtr->dynamicIndex));
    gcmONERROR(gcoINDEX_SetDynamic(vertexPtr->dynamicIndex, 128 << 10, 32));

    /* Query the stream capabilities. */
    gcmONERROR(gcoHARDWARE_QueryStreamCaps(&vertexPtr->maxAttribute,
                                           &vertexPtr->maxSize,
                                           &vertexPtr->maxStreams,
                                           gcvNULL));

    /* Disable falling back to 1 stream. */
    /* TODO: Enable falling back to 1 stream after checking MSB. */

    /* Return the gcoVERTEXARRAY object. */
    *Vertex = vertexPtr;

    /* Success. */
    gcmFOOTER_ARG("*Vertex=0x%x", *Vertex);
    return gcvSTATUS_OK;

OnError:
    if (vertexPtr != gcvNULL)
    {
        if (vertexPtr->dynamicStream != gcvNULL)
        {
            /* Destroy the dynamic stream. */
            gcmVERIFY_OK(gcoSTREAM_Destroy(vertexPtr->dynamicStream));
        }

        if (vertexPtr->dynamicIndex != gcvNULL)
        {
            /* Destroy the dynamic index. */
            gcmVERIFY_OK(gcoINDEX_Destroy(vertexPtr->dynamicIndex));
        }

        /* Free the gcoVERTEXARRAY object. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, vertexPtr));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVERTEXARRAY_Destroy(
    IN gcoVERTEXARRAY Vertex
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Vertex=0x%x", Vertex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);

    if (Vertex->dynamicStream != gcvNULL)
    {
        /* Destroy the dynamic stream. */
        gcmONERROR(gcoSTREAM_Destroy(Vertex->dynamicStream));
        Vertex->dynamicStream = gcvNULL;
    }

    if (Vertex->dynamicIndex != gcvNULL)
    {
        /* Destroy the dynamic index. */
        gcmONERROR(gcoINDEX_Destroy(Vertex->dynamicIndex));
        Vertex->dynamicIndex = gcvNULL;
    }

    /* Free the gcoVERTEX object. */
    gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, Vertex));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVERTEXARRAY_Bind(
    IN gcoVERTEXARRAY Vertex,
    IN gctUINT32 EnableBits,
    IN gcsVERTEXARRAY_PTR VertexArray,
    IN gctUINT First,
    IN gctSIZE_T Count,
    IN gceINDEX_TYPE IndexType,
    IN gcoINDEX IndexObject,
    IN gctPOINTER IndexMemory,
    IN OUT gcePRIMITIVE * PrimitiveType,
    IN OUT gctUINT * PrimitiveCount
    )
{
    gceSTATUS status;

    gcsVERTEXARRAY_PTR vertexPtr = VertexArray;
    gctUINT i, n;
    gctUINT first, count;

    /* Zero the arrays. */
    gcsVERTEXARRAY_ATTRIBUTE attributeArray[gcdATTRIBUTE_COUNT];
    gcsVERTEXARRAY_ATTRIBUTE_PTR attributePtr = attributeArray;
    gctUINT attributeCount = 0;

    /* Zero the arrays. */
    gcsVERTEXARRAY_ATTRIBUTE copyArray[gcdATTRIBUTE_COUNT];
    gcsVERTEXARRAY_ATTRIBUTE_PTR copyPtr = copyArray;
    gctUINT copyCount = 0, copyBytes = 0;
    gctUINT32 copyPhysical = ~0U;

    /* Zero the arrays. */
    gcsVERTEXARRAY_STREAM streamArray[gcdATTRIBUTE_COUNT];
    gcsVERTEXARRAY_STREAM_PTR streamPtr;
    gctUINT stream, streamCount = 0;

#if OPT_VERTEX_ARRAY
    gcsVERTEXARRAY_BUFFER bufferArray[gcdATTRIBUTE_COUNT];
    gcsVERTEXARRAY_BUFFER_PTR bufferPtr = bufferArray;
    gctUINT bufferCount = 0;
    gctUINT bufferSize = 0;
#endif

    gctBOOL fakeLineLoop;
    gctPOINTER indexBuffer = gcvNULL;

    gcmHEADER_ARG("Vertex=0x%x EnableBits=0x%x VertexArray=0x%x First=%u "
		"Count=%lu, *PrimitiveType=%lu",
                  Vertex, EnableBits, VertexArray, First, Count,
                  gcmOPT_VALUE(PrimitiveType));

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vertex, gcvOBJ_VERTEX);
    gcmVERIFY_ARGUMENT(EnableBits != 0);
    gcmVERIFY_ARGUMENT(VertexArray != gcvNULL);
    gcmVERIFY_ARGUMENT(Count > 0);
    gcmVERIFY_ARGUMENT(PrimitiveType != gcvNULL);

#if OPT_VERTEX_ARRAY
    gcmONERROR(gcoOS_ZeroMemory(copyArray,   sizeof(copyArray)));
    gcmONERROR(gcoOS_ZeroMemory(bufferArray, sizeof(bufferArray)));
#endif

    /* Check for hardware line loop support. */
    fakeLineLoop = (*PrimitiveType == gcvPRIMITIVE_LINE_LOOP)
                 ? !gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_LINE_LOOP)
                 : gcvFALSE;

	if((*PrimitiveType == gcvPRIMITIVE_LINE_LOOP) && (!fakeLineLoop))
	{
		/*for line loop the last line is automatic added in hw implementation*/
		*PrimitiveCount = *PrimitiveCount - 1;
	}
    /***************************************************************************
    ***** Phase 1: Information Gathering **************************************/

    /* Walk through all attributes. */
    for (i = 0;
         (i < gcdATTRIBUTE_COUNT) && (EnableBits != 0);
         ++i, EnableBits >>= 1, ++vertexPtr
    )
    {
        gctBOOL needCopy;

        /* Skip disabled vertex attributes. */
        if ((EnableBits & 1) == 0)
        {
            continue;
        }

        /* Initialize the attribute information. */
        attributePtr->vertexPtr     = vertexPtr;
        attributePtr->genericEnable = 0;
        attributePtr->next = gcvNULL;

        if (vertexPtr->enable)
        {
            attributePtr->format = vertexPtr->format;

            /* Compute the size of the attribute. */
            switch (vertexPtr->format)
            {
            case gcvVERTEX_BYTE:
            case gcvVERTEX_UNSIGNED_BYTE:
                attributePtr->bytes = vertexPtr->size;
                break;

            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
            case gcvVERTEX_INT_10_10_10_2:
                /* 10_10_10_2 format is always in 4 bytes container. */
                attributePtr->bytes = 4;
                break;

            case gcvVERTEX_SHORT:
            case gcvVERTEX_UNSIGNED_SHORT:
            case gcvVERTEX_HALF:
                attributePtr->bytes = vertexPtr->size * 2;
                break;

            default:
                attributePtr->bytes = vertexPtr->size * 4;
                break;
            }

            /* Test for generic data to append to the attribute. */
            switch (vertexPtr->size)
            {
            case 1:
                if (vertexPtr->genericValue[1] != 0.0f)
                {
                    /* Generic v1 data is not 0.0f. */
                    attributePtr->genericEnable |= (1 << 1);
                }
                /* Fall through. */

            case 2:
                if (vertexPtr->genericValue[2] != 0.0f)
                {
                    /* Generic v2 data is not 0.0f. */
                    attributePtr->genericEnable |= (1 << 2);
                }
                /* Fall through. */

            case 3:
                if (vertexPtr->genericValue[3] != 1.0f)
                {
                    /* Generic v3 data is not 1.0f. */
                    attributePtr->genericEnable |= (1 << 3);
                }
                break;

            default:
                break;
            }
        }

        /* Test if this vertex attribute is a gcoSTREAM. */
        if (vertexPtr->enable && (vertexPtr->stream != gcvNULL))
        {
            gcsVERTEXARRAY_ATTRIBUTE_PTR node, prev;

            /* What is this? We have buffers and yet we need to "append" generic
            ** vertex data? What kind of app is this? */
            gcmASSERT(attributePtr->genericEnable == 0);
            attributePtr->genericEnable = 0;

            /* Save offset for this attribute. */
            attributePtr->offset = gcmPTR2INT(vertexPtr->pointer);

            /* Find the stream for this vertex atrribute in the stream array. */
            for (stream = 0, streamPtr = streamArray;
                 stream < streamCount;
                 ++stream, ++streamPtr
            )
            {
                if (streamPtr->stream == vertexPtr->stream)
                {
                    /* Found it. */
                    break;
                }
            }

            /* Test if the stream is not yet known. */
            if (stream == streamCount)
            {
                gctPOINTER pointer = gcvNULL;

                /* Make sure we don't overflow the array. */
                if (stream == gcmCOUNTOF(streamArray))
                {
                    gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                }

                /* Increase the number of streams. */
                streamCount += 1;

                /* Initialize the stream information. */
                streamPtr->stream    = vertexPtr->stream;
                streamPtr->subStream = gcvNULL;

                /* Lock the stream. */
                gcmONERROR(gcoSTREAM_Lock(vertexPtr->stream,
                                          &pointer,
                                          &streamPtr->physical));

                streamPtr->logical = pointer;
                /* Initialize attribute to gcvNULL. */
                streamPtr->attribute = gcvNULL;
            }

            /* Sort this attribute by offset. */
            for (node = streamPtr->attribute, prev = gcvNULL;
                 node != gcvNULL;
                 node = node->next)
            {
                /* Check if we have found an attribute with a larger offset. */
                if (node->offset > attributePtr->offset)
                {
                    break;
                }

                /* Save attribute and move to next one. */
                prev = node;
            }

            /* Link attrribute to this stream. */
            attributePtr->next = node;
            if (prev == gcvNULL)
            {
                streamPtr->attribute = attributePtr;
            }
            else
            {
                prev->next = attributePtr;
            }

            /* Set stream attribute. */
            gcmONERROR(gcoSTREAM_SetAttribute(streamPtr->stream,
                                              gcmPTR2INT(vertexPtr->pointer),
                                              attributePtr->bytes,
                                              vertexPtr->stride,
                                              &streamPtr->subStream));

            /* Set logical pointer to attribute. */
            attributePtr->logical = streamPtr->logical
                                  + gcmPTR2INT(vertexPtr->pointer);

            /* Don't need to copy this attribute yet. */
            needCopy = gcvFALSE;
        }

        /* Test if this vertex attribute is a client pointer. */
        else if (vertexPtr->enable && (vertexPtr->pointer != gcvNULL))
        {
            /* Set logical pointer to attribute. */
            attributePtr->logical = vertexPtr->pointer;
            /* When vertexPtr->enable, generic value is not used for copy */
            attributePtr->genericEnable = 0;
            /* Copy this attribute. */
            needCopy = gcvTRUE;
        }

        /* The vertex attribute is not set. */
        else
        {
            /* Need to copy generic value. */
            attributePtr->genericEnable = 0;
            attributePtr->format        = gcvVERTEX_FLOAT;
            attributePtr->bytes         = vertexPtr->size * 4; /* 16; */

            /* Set logical poointer to attribute. */
            attributePtr->logical = vertexPtr->genericValue;

            /* Copy this attribute. */
            needCopy = gcvTRUE;
        }

        if (needCopy)
        {
            /* Fill in attribute information that needs to be copied. */
            copyPtr->vertexPtr     = vertexPtr;
            copyPtr->logical       = attributePtr->logical;
            copyPtr->offset        = copyBytes;
            copyPtr->genericEnable = attributePtr->genericEnable;

            if (copyPtr->genericEnable)
            {
                /* Attribute has to be converted to float. */
                copyPtr->format = gcvVERTEX_FLOAT;
                copyPtr->bytes  = (copyPtr->genericEnable & 0x8) ? 16
                                : (copyPtr->genericEnable & 0x4) ? 12
                                : (copyPtr->genericEnable & 0x2) ?  8
                                                                 :  4;
            }
            else
            {
                /* No generic data. */
                copyPtr->format = attributePtr->format;
                copyPtr->bytes  = attributePtr->bytes;
            }

            /* Increase number of bytes per vertex. */
            copyBytes += copyPtr->bytes;

            /* Move to next array. */
            copyCount += 1;
            copyPtr   += 1;
        }

        /* Next attribute. */
        attributeCount += 1;
        attributePtr   += 1;
    }

    /* Test if there are too many attributes. */
    if (attributeCount > Vertex->maxAttribute)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Check for aliasing older hardware has issues with. */
    if ((streamCount == 1)
    &&  (attributeCount == 2)
    &&  (attributeArray[0].logical == attributeArray[1].logical)
    &&  (attributeArray[0].bytes < 8)
    &&  (attributeArray[0].vertexPtr->stream != gcvNULL)
    &&  (Count > 31)
    )
    {
        /* Unalias the stream buffer. */
        gcmONERROR(gcoSTREAM_UnAlias(attributeArray[0].vertexPtr->stream,
                                     streamArray->attribute,
                                     &streamArray[0].subStream,
                                     &streamArray[0].logical,
                                     &streamArray[0].physical));
    }

    /* Compute total number of streams. */
    n = (copyCount > 0) ? 1 : 0;
    for (i = 0, streamPtr = streamArray; i < streamCount; ++i, ++streamPtr)
    {
        gctUINT subStreams;

        /* Query number of substreams from this stream. */
        gcmONERROR(gcoSTREAM_QuerySubStreams(streamPtr->stream,
                                             streamPtr->subStream,
                                             &subStreams));

        /* Increase the number of streams. */
        n += subStreams;
    }

    /***************************************************************************
    ***** Phase 2: Index Management *******************************************/

    /* Check if we need to fake the line loop */
    if (fakeLineLoop)
    {
        gctUINT bytes = 0;
        gctUINT bytesPerIndex = 0;

        /* Check if there is an index object or buffer. */
        if ((IndexObject != gcvNULL) || (IndexMemory != gcvNULL))
        {
            /* Dispatch on index type. */
            switch (IndexType)
            {
            case gcvINDEX_8:
                /* 8-bit indices. */
                bytes         = Count;
                bytesPerIndex = 1;
                break;

            case gcvINDEX_16:
                /* 16-bit indices. */
                bytes         = Count * 2;
                bytesPerIndex = 2;
                break;

            case gcvINDEX_32:
                /* 32-bit indices. */
                bytes         = Count * 4;
                bytesPerIndex = 4;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }
        }
        else
        {
            /* Check if the count fits in 8-bit. */
            if (First + Count + 1 < 256)
            {
                /* 8-bit indices. */
                IndexType     = gcvINDEX_8;
                bytes         = Count;
                bytesPerIndex = 1;
            }

            /* Check if the count fits in 16-bit. */
            else if (First + Count + 1 < 65536)
            {
                /* 16-bit indices. */
                IndexType     = gcvINDEX_16;
                bytes         = Count * 2;
                bytesPerIndex = 2;
            }

            else
            {
                /* 32-bit indices. */
                IndexType     = gcvINDEX_32;
                bytes         = Count * 4;
                bytesPerIndex = 4;
            }
        }

        /* Allocate a temporary buffer. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  bytes + bytesPerIndex,
                                  &indexBuffer));

        /* Test if there is a gcoINDEX stream. */
        if (IndexObject != gcvNULL)
        {
            gctPOINTER logical;

            /* Lock the index stream. */
            gcmONERROR(gcoINDEX_Lock(IndexObject, gcvNULL, &logical));

            /* Copy the indices to the temporary memory. */
            gcmONERROR(gcoOS_MemCopy(indexBuffer,
                                     (gctUINT8_PTR) logical
                                     + gcmPTR2INT(IndexMemory),
                                     bytes));

            /* Append the first index to the temporary memory. */
            gcmONERROR(gcoOS_MemCopy((gctUINT8_PTR) indexBuffer + bytes,
                                     (gctUINT8_PTR) logical
                                     + gcmPTR2INT(IndexMemory),
                                     bytesPerIndex));
        }

        else if (IndexMemory != gcvNULL)
        {
            /* Copy the indices to the temporary memory. */
            gcmONERROR(gcoOS_MemCopy(indexBuffer, IndexMemory, bytes));

            /* Append the first index to the temporary memory. */
            gcmONERROR(gcoOS_MemCopy((gctUINT8_PTR) indexBuffer + bytes,
                                     IndexMemory,
                                     bytesPerIndex));
        }

        else
        {
            gctUINT i;

            /* Test for 8-bit indices. */
            if (bytesPerIndex == 1)
            {
                /* Cast pointer to index buffer. */
                gctUINT8_PTR ptr = (gctUINT8_PTR) indexBuffer;

                /* Fill index buffer with First through First + Count - 1. */
                for (i = 0; i < Count; ++i)
                {
                    *ptr++ = (gctUINT8) (First + i);
                }

                /* Append First to index buffer. */
                *ptr = (gctUINT8) First;
            }
            else if (bytesPerIndex == 2)
            {
                /* Cast pointer to index buffer. */
                gctUINT16_PTR ptr = (gctUINT16_PTR) indexBuffer;

                /* Fill index buffer with First through First + Count - 1. */
                for (i = 0; i < Count; ++i)
                {
                    *ptr++ = (gctUINT16) (First + i);
                }

                /* Append First to index buffer. */
                *ptr = (gctUINT16) First;
            }
            else
            {
                /* Cast pointer to index buffer. */
                gctUINT32_PTR ptr = (gctUINT32_PTR) indexBuffer;

                /* Fill index buffer with First through First + Count - 1. */
                for (i = 0; i < Count; ++i)
                {
                    *ptr++ = First + i;
                }

                /* Append First to index buffer. */
                *ptr = First;
            }
        }

        /* Upload the data to the dynamic index buffer. */
        gcmONERROR(gcoINDEX_UploadDynamicEx(Vertex->dynamicIndex,
                                            IndexType,
                                            indexBuffer,
                                            bytes + bytesPerIndex));

        /* Free the temporary index buffer. */
        gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, indexBuffer));

        /* Bind the index buffer to the hardware. */
        gcmONERROR(gcoINDEX_BindDynamic(Vertex->dynamicIndex, IndexType));

        /* Test if need to copy vertex data. */
        if ((copyCount > 0) || (n > Vertex->maxStreams))
        {
            gctUINT minIndex, maxIndex;

            /* Get the index range. */
            gcmONERROR(gcoINDEX_GetDynamicIndexRange(Vertex->dynamicIndex,
                                                     &minIndex, &maxIndex));

            /* Compute vertex range. */
            first = minIndex;
            count = maxIndex - minIndex + 1;
        }
        else
        {
            /* No need to verify or copy anything. */
            first = 0;
            count = Count;
        }
    }
    else
    {
        /* Test if there is a gcoINDEX stream. */
        if (IndexObject != gcvNULL)
        {
            /* Bind the index buffer to the hardware. */
            gcmONERROR(gcoINDEX_BindOffset(IndexObject,
                                           IndexType,
                                           gcmPTR2INT(IndexMemory)));
        }

        /* Test if there are client indices. */
        else if (IndexMemory != gcvNULL)
        {
            gctUINT bytes = 0;

            /* Determine number of bytes in the index buffer. */
            switch (IndexType)
            {
            case gcvINDEX_8:
                bytes = Count;
                break;

            case gcvINDEX_16:
                bytes = Count * 2;
                break;

            case gcvINDEX_32:
                bytes = Count * 4;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Upload the data to the dynamic index buffer. */
            gcmONERROR(gcoINDEX_UploadDynamicEx(Vertex->dynamicIndex,
                                                IndexType,
                                                IndexMemory,
                                                bytes));

            /* Bind the index buffer to the hardware. */
            gcmONERROR(gcoINDEX_BindDynamic(Vertex->dynamicIndex, IndexType));
        }

        /* Test if need to copy vertex data. */
        if ((copyCount > 0) || (n > Vertex->maxStreams))
        {
            gctUINT minIndex, maxIndex;

            /* Test if there is a gcoINDEX object. */
            if (IndexObject != gcvNULL)
            {
                /* Get the index range. */
                gcmONERROR(gcoINDEX_GetIndexRange(IndexObject, IndexType,
                                                  gcmPTR2INT(IndexMemory),
                                                  Count,
                                                  &minIndex, &maxIndex));

                /* Compute vertex range. */
                first = minIndex;
                count = maxIndex - minIndex + 1;
            }

            /* Test if there is an index array. */
            else if (IndexMemory != gcvNULL)
            {
                /* Get the index range. */
                gcmONERROR(gcoINDEX_GetDynamicIndexRange(Vertex->dynamicIndex,
                                                         &minIndex, &maxIndex));

                /* Compute vertex range. */
                first = minIndex;
                count = maxIndex - minIndex + 1;
            }

            /* No indices present. */
            else
            {
                /* Copy vertex range. */
                first = First;
                count = Count;
            }
        }
        else
        {
            /* No need to verify or copy anything. */
            first = 0;
            count = Count;
        }
    }

    /***************************************************************************
    ***** Phase 3: Stream Management ******************************************/

    /* Check if we have too many streams. */
    if ((n > Vertex->maxStreams) && (copyCount == 0))
    {
        gcoSTREAM streamObject;
        gctPOINTER logical;
        gctUINT32 physical;
        gcsSTREAM_SUBSTREAM_PTR subStream;

        /* Merge the streams. */
        status = gcoSTREAM_MergeStreams(streamArray[0].stream,
                                        first, count,
                                        streamCount, streamArray,
                                        &streamObject, &logical, &physical,
                                        &attributePtr, &subStream);

        if (gcmIS_SUCCESS(status))
        {
            /* Copy stream information to stream array. */
            streamArray[0].stream    = streamObject;
            streamArray[0].logical   = logical;
            streamArray[0].physical  = physical;
            streamArray[0].attribute = attributePtr;
            streamArray[0].subStream = subStream;
            streamCount              = 1;
            n                        = 1;
        }
    }

    /* Check if we still have too many streams. */
    if (n > Vertex->maxStreams)
    {
        /* Reset number of streams. */
        n = (copyCount > 0) ? 1 : 0;

        /* Walk all streams. */
        for (i = 0, streamPtr = streamArray;
             i < streamCount;
             ++i, ++streamPtr
        )
        {
            gctUINT subStreams;

            /* Query number of streams. */
            gcmONERROR(gcoSTREAM_QuerySubStreams(streamPtr->stream,
                                                 streamPtr->subStream,
                                                 &subStreams));

            /* Check if we can rebuild this stream and if it makes sense. */
            if ((subStreams > 1) && (n + 1 < Vertex->maxStreams))
            {
                /* Rebuild the stream. */
                status = gcoSTREAM_Rebuild(streamPtr->stream,
                                           first, count,
                                           &subStreams);
                if (gcmIS_ERROR(status))
                {
                    /* Error rebuilding. */
                    gcmTRACE_ZONE(gcvLEVEL_WARNING, gcvZONE_VERTEX,
                                  "gcoSTREAM_Rebuild returned status=%d",
                                  status);
                }
            }

            if ((n + subStreams > Vertex->maxStreams)
            ||  (Vertex->maxStreams == 1)
            )
            {
                /* This stream would overflow the maximum stream count, so we
                ** have to copy it. */

                /* Walk all the stream attributes and append them to the list to
                ** copy. */
                for (attributePtr = streamPtr->attribute;
                     attributePtr != gcvNULL;
                     attributePtr = attributePtr->next
                     )
                {
                    /* Copy this attribute. */
                    copyPtr->vertexPtr     = attributePtr->vertexPtr;
                    copyPtr->logical       = attributePtr->logical;
                    copyPtr->format        = attributePtr->format;
                    copyPtr->offset        = copyBytes;
                    copyPtr->bytes         = attributePtr->bytes;
                    copyPtr->genericEnable = 0;

                    /* Increase number of bytes per vertex. */
                    copyBytes += copyPtr->bytes;

                    /* Move to next array. */
                    copyCount += 1;
                    copyPtr   += 1;
                }

                /* This stream doesn't need to be programmed. */
                streamPtr->logical = gcvNULL;
            }
            else
            {
                /* Advance the number of streams. */
                n += subStreams;
            }
        }
    }

    /***************************************************************************
    ***** Phase 4: Vertex Copy ************************************************/

    if (copyCount > 0)
    {
#if OPT_VERTEX_ARRAY
        gctUINT hwStream = Vertex->maxStreams - n + 1;
        gctUINT bufferOffset = 0;
        gctUINT j, k;

        /* Setup buffers. */
        for (i = 0, copyPtr = copyArray; i < copyCount; i++, copyPtr++)
        {
            gctUINT8_PTR start = (gctUINT8_PTR)copyPtr->logical;
            gctUINT8_PTR end   = (gctUINT8_PTR)copyPtr->logical + copyPtr->bytes;

            for (j = 0, bufferPtr = bufferArray;
                 j < bufferCount;
                 j++, bufferPtr++)
            {
                /* If in a same range. */
                if(bufferPtr->stride == copyPtr->vertexPtr->stride)
                {
                    /* Test if the attribute falls within the buffer range.*/
                    if ((start >= bufferPtr->start) &&
                        (end   <= bufferPtr->end)
                       )
                    {
                        break;
                    }

                    /* Test if the attribute starts within the sliding window. */
                    if ((start <  bufferPtr->start) &&
                        (start >= bufferPtr->minStart)
                       )
                    {
                        bufferPtr->start  = start;
                        bufferPtr->maxEnd = start + bufferPtr->stride;
                        break;
                    }

                    /* Test if attribute ends within the sliding window. */
                    if ((end >  bufferPtr->end) &&
                        (end <= bufferPtr->maxEnd)
                       )
                    {
                        bufferPtr->end      = end;
                        bufferPtr->minStart = end - bufferPtr->stride;
                        break;
                    }
                }
            }

            /* Create a new vertex buffer. */
            if (j == bufferCount)
            {
                bufferCount++;

                bufferPtr->count     = 1;
                bufferPtr->map[0]    = i;

                bufferPtr->stride    = copyPtr->vertexPtr->enable
                                       ? copyPtr->vertexPtr->stride
                                       : copyPtr->bytes;

                bufferPtr->start     = (gctUINT8_PTR)copyPtr->logical;
                bufferPtr->end       = bufferPtr->start + copyPtr->bytes;
                bufferPtr->minStart  = bufferPtr->end   - bufferPtr->stride;
                bufferPtr->maxEnd    = bufferPtr->start + bufferPtr->stride;
            }
            else
            {
                /* Sort the attribute in a buffer. */
                for(k = bufferPtr->count; k != 0; k--)
                {
                    if(copyPtr->logical >= copyArray[bufferPtr->map[k - 1]].logical)
                    {
                        bufferPtr->map[k] = i;
                        break;
                    }

                    bufferPtr->map[k] = bufferPtr->map[k - 1];
                }

                if (k == 0)
                {
                    bufferPtr->map[0] = i;
                }

                bufferPtr->count    += 1;
            }
        }

        /* Adjust attributes. */
        for (i = 0, bufferPtr = bufferArray;
             i < bufferCount;
             i++, bufferPtr++, --hwStream)
        {
            /* Break if only one hw stream avaible, while more than one buffer left. */
            if ((hwStream == 1) && (i != (bufferCount -1)))
            {
                break;
            }

            /* Set offset for a buffer. */
            bufferPtr->offset = bufferOffset;

            /* Adjust attributes offset in a buffer. */
            for (j = 0; j < bufferPtr->count; j++)
            {
                copyPtr = copyArray + bufferPtr->map[j];

                copyPtr->offset = (gctUINT8_PTR)copyPtr->logical
                                - (gctUINT8_PTR)bufferPtr->start;
            }

            /* Increase buffer offset. */
            bufferOffset += bufferPtr->stride * count;
            bufferSize   += bufferPtr->stride;
        }

        /* Check if all buffers have been handled? */
        /* If no, combine all left attributes into the last buffer. */
        if (i < bufferCount)
        {
            gcsVERTEXARRAY_BUFFER_PTR last;
            gctUINT attribCount = 0;
            gctUINT offset = 0;
            gctUINT adjustedBufferCount = i + 1;

            /* Mark last buffer as combined. */
            last           = bufferArray + i;
            last->offset   = bufferOffset;
            last->combined = gcvTRUE;

            /* Merge all left into the last buffer. */
            for (bufferPtr = last;
                 i < bufferCount;
                 ++i, ++bufferPtr)
            {
                for (j = 0; j < bufferPtr->count; j++, attribCount++)
                {
                    /* Adjust attribute offset. */
                    copyPtr = copyArray + bufferPtr->map[j];
                    copyPtr->offset = offset;

                    /* Merge attribute into last buffer.*/
                    if (last != bufferPtr)
                    {
                        last->map[attribCount] = bufferPtr->map[j];
                        last->count++;
                    }

                    /* Increase offset. */
                    offset += copyPtr->bytes;
                }
            }

            /* Adjust buffer stride. */
            last->stride = offset;
            bufferSize  += last->stride;

            /* Re-adjust total buffer count. */
            bufferCount  = adjustedBufferCount;
        }

        /* Copy the vertex data. */
        gcmONERROR(gcoSTREAM_CacheAttributes(Vertex->dynamicStream,
                                             first, count,
                                             bufferSize,
                                             bufferCount, bufferArray,
                                             copyCount, copyArray,
                                             &copyPhysical));
#else

        /* Copy the vertex data. */
        gcmONERROR(gcoSTREAM_CacheAttributes(Vertex->dynamicStream,
                                             first, count,
                                             copyBytes,
                                             copyCount, copyArray,
                                             &copyPhysical));
#endif
    }

    /***************************************************************************
    ***** Phase 5: Program the Hardware ***************************************/

    /* Here it is easy - let the next layer worry about the actual registers. */
#if OPT_VERTEX_ARRAY
    gcmONERROR(gcoHARDWARE_SetVertexArray(gcvNULL,
                                          first, copyPhysical,
                                          bufferCount, bufferArray,
                                          copyCount, copyArray,
                                          streamCount, streamArray));
#else
    gcmONERROR(gcoHARDWARE_SetVertexArray(gcvNULL,
                                          first,
                                          copyPhysical, copyBytes,
                                          copyCount, copyArray,
                                          streamCount, streamArray));
#endif

    if (fakeLineLoop)
    {
        /* Switch to Line Strip. */
        *PrimitiveType = gcvPRIMITIVE_LINE_STRIP;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (indexBuffer != gcvNULL)
    {
        /* Free the temporary index buffer. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, indexBuffer));
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else /* gcdNULL_DRIVER < 2 */


/*******************************************************************************
** NULL DRIVER STUBS
*/

gceSTATUS gcoVERTEXARRAY_Construct(
    IN gcoHAL Hal,
    OUT gcoVERTEXARRAY * Vertex
    )
{
    *Vertex = gcvNULL;
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEXARRAY_Destroy(
    IN gcoVERTEXARRAY Vertex
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS gcoVERTEXARRAY_Bind(
    IN gcoVERTEXARRAY Vertex,
    IN gctUINT32 EnableBits,
    IN gcsVERTEXARRAY_PTR VertexArray,
    IN gctUINT First,
    IN gctSIZE_T Count,
    IN gceINDEX_TYPE IndexType,
    IN gcoINDEX IndexObject,
    IN gctPOINTER IndexMemory,
    IN OUT gcePRIMITIVE * PrimitiveType,
    IN OUT gctUINT * PrimitiveCount
    )
{
    return gcvSTATUS_OK;
}

#endif /* gcdNULL_DRIVER < 2 */
#endif /* VIVANTE_NO_3D */
