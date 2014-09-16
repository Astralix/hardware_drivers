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




#include "gc_hal_user_hardware_precomp.h"

#ifndef VIVANTE_NO_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_HARDWARE

#define _gcmSETSTATEDATA(StateDelta, Memory, Address, Data) \
    do \
    { \
        gctUINT32 __temp_data32__ = Data; \
        \
        *Memory++ = __temp_data32__; \
        \
        gcoHARDWARE_UpdateDelta(\
            StateDelta, gcvFALSE, Address, 0, __temp_data32__ \
            ); \
        \
        gcmDUMPSTATEDATA(StateDelta, gcvFALSE, Address, __temp_data32__); \
        \
        Address += 1; \
    } \
    while (gcvFALSE)

/**
 * Program the stream information into the hardware.
 *
 * @param Index    Stream number.
 * @param Address  Physical base address of the stream.
 * @param Stride   Stride of the stream in bytes.
 *
 * @return Status of the programming.
 */
gceSTATUS
gcoHARDWARE_SetStream(
    IN gctUINT32 Index,
    IN gctUINT32 Address,
    IN gctUINT32 Stride
    )
{
    gceSTATUS status;
    gctUINT32 offset;
    gcoHARDWARE hardware;

    gcmHEADER_ARG("Index=%d Address=%u Stride=%d",
                    Index, Address, Stride);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

    /* Verify the stream index. */
    if (Index >= hardware->streamCount)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Compute the offset.*/
    offset = Index << 2;

    /* Program the stream base address and stride. */
    if (Index < 1)
    {
        gcmONERROR(
            gcoHARDWARE_LoadState32(hardware,
                                    0x0064C + offset,
                                    Address));

        /* Program the stream stride. */
        gcmONERROR(
            gcoHARDWARE_LoadState32(hardware,
                                    0x00650 + offset,
                                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))));
    }
    else
    {
        gcmONERROR(
            gcoHARDWARE_LoadState32(hardware,
                                    0x00680 + offset,
                                    Address));

        /* Program the stream stride. */
        gcmONERROR(
            gcoHARDWARE_LoadState32(hardware,
                                    0x006A0 + offset,
                                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))));
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/**
 * Program the attributes required for the Vertex Shader into
 * the hardware.
 *
 * @param Attributes Pointer to the attribute information.
 * @param AttributeCount
 *                   Number of attributes to program.
 *
 * @return Status of the attributes.
 */
gceSTATUS
gcoHARDWARE_SetAttributes(
    IN gcsVERTEX_ATTRIBUTES_PTR Attributes,
    IN gctUINT32 AttributeCount
    )
{
    gcoHARDWARE hardware;
    gcsVERTEX_ATTRIBUTES_PTR mapping[16];
    gctUINT32 i, j, k, attribCountMax;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 format = 0, size = 0, endian = 0, normalize, fetchBreak, fetchSize;
    gctUINT32 link, linkState = 0;

    gcoCMDBUF reserve;
    gcsSTATE_DELTA_PTR stateDelta;
    gctSIZE_T vertexCtrlStateCount, vertexCtrlReserveCount;
    gctSIZE_T shaderCtrlStateCount, shaderCtrlReserveCount;
    gctSIZE_T reserveSize;
    gctUINT32_PTR vertexCtrl;
    gctUINT32_PTR shaderCtrl;

    gctUINT vertexCtrlState;
    gctUINT shaderCtrlState;

    gcmHEADER_ARG("Attributes=0x%x AttributeCount=%d",
                  Attributes, AttributeCount);

    gcmGETHARDWARE(hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(hardware, gcvOBJ_HARDWARE);

	attribCountMax = 12;
	if ((((((gctUINT32) (hardware->chipMinorFeatures1)) >> (0 ? 23:23)) & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1)))))) ))
	{
		attribCountMax = 16;
	}

    /* Verify the number of attributes. */
    if (AttributeCount >= attribCountMax)
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    gcmVERIFY_OK(gcoOS_ZeroMemory(mapping, gcmSIZEOF(mapping)));

    /***************************************************************************
    ** Sort all attributes by stream/offset.
    */

    for (i = 0; i < AttributeCount; ++i)
    {
        /* Find the slot for the current attribute. */
        for (j = 0; j < i; ++j)
        {
            if (
                /* If the current attribute's stream lower? */
                (Attributes[i].stream < mapping[j]->stream)

                /* Or is it the same and the offset is lower? */
            ||  ((Attributes[i].stream == mapping[j]->stream)
                && (Attributes[i].offset <  mapping[j]->offset)
                )
            )
            {
                /* We found our slot. */
                break;
            }
        }

        /* Make space at the current slot. */
        for (k = i; k > j; --k)
        {
            mapping[k] = mapping[k - 1];
        }

        /* Save the sorting order. */
        mapping[j] = &Attributes[i];
    }

    /***************************************************************************
    ** Determine the counts and reserve size.
    */

    /* State counts. */
    vertexCtrlStateCount = AttributeCount;
    shaderCtrlStateCount = gcmALIGN(AttributeCount, 4) >> 2;

    /* Reserve counts. */
    vertexCtrlReserveCount = 1 + (vertexCtrlStateCount | 1);
    shaderCtrlReserveCount = 1 + (shaderCtrlStateCount | 1);

    /* Set initial state addresses. */
    vertexCtrlState = 0x0180;
    shaderCtrlState = 0x0208;

    /* Determine the size of the buffer to reserve. */
    reserveSize
        = (vertexCtrlReserveCount + shaderCtrlReserveCount)
        *  gcmSIZEOF(gctUINT32);

    /***************************************************************************
    ** Reserve command buffer state and init state commands.
    */

    /* Reserve space in the command buffer. */
    gcmONERROR(gcoBUFFER_Reserve(
        hardware->buffer, reserveSize, gcvTRUE, &reserve
        ));

    /* Shortcut to the current delta. */
    stateDelta = hardware->delta;

    /* Update the number of the elements. */
    stateDelta->elementCount = AttributeCount;

    /* Determine buffer pointers. */
    vertexCtrl = (gctUINT32_PTR) reserve->lastReserve;
    shaderCtrl = vertexCtrl + vertexCtrlReserveCount;

    /* Init load state commands. */
    *vertexCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (vertexCtrlStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (vertexCtrlState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *shaderCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (shaderCtrlStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (shaderCtrlState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    /* Zero out fetch size. */
    fetchSize = 0;

    /* Walk all attributes. */
    for (i = 0; i < AttributeCount; ++i)
    {
        /* Convert format. */
        switch (mapping[i]->format)
        {
        case gcvVERTEX_BYTE:
            format = 0x0;
            endian = 0x0;
            size   = 1;
            break;

        case gcvVERTEX_UNSIGNED_BYTE:
            format = 0x1;
            endian = 0x0;
            size   = 1;
            break;

        case gcvVERTEX_SHORT:
            format = 0x2;
            endian = hardware->bigEndian
                   ? 0x1
                   : 0x0;
            size   = 2;
            break;

        case gcvVERTEX_UNSIGNED_SHORT:
            format = 0x3;
            endian = hardware->bigEndian
                   ? 0x1
                   : 0x0;
            size   = 2;
            break;

        case gcvVERTEX_INT:
            format = 0x4;
            endian = hardware->bigEndian
                   ? 0x2
                   : 0x0;
            size   = 4;
            break;

        case gcvVERTEX_UNSIGNED_INT:
            format = 0x5;
            endian = hardware->bigEndian
                   ? 0x2
                   : 0x0;
            size   = 4;
            break;

        case gcvVERTEX_FIXED:
            format = 0xB;
            endian = hardware->bigEndian
                   ? 0x2
                   : 0x0;
            size   = 4;
            break;

        case gcvVERTEX_HALF:
            format = 0x9;
            endian = hardware->bigEndian
                   ? 0x1
                   : 0x0;
            size   = 2;
            break;

        case gcvVERTEX_FLOAT:
            format = 0x8;
            endian = hardware->bigEndian
                   ? 0x2
                   : 0x0;
            size   = 4;
            break;

        case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
            format = 0xD;
            endian = hardware->bigEndian
                   ? 0x2
                   : 0x0;
            break;

        case gcvVERTEX_INT_10_10_10_2:
            format = 0xC;
            endian = hardware->bigEndian
                   ? 0x2
                   : 0x0;
            break;

        default:
            gcmFATAL("Unknown format: %d", mapping[i]->format);
            gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
        }

        /* Convert normalized. */
        normalize = mapping[i]->normalized
                  ? (hardware->api == gcvAPI_OPENGL)
                    ? 0x2
                    : 0x1
                  : 0x0;

        /* Adjust fetch size. */
        fetchSize += size * mapping[i]->components;

        /* Determine if this is a fetch break. */
        fetchBreak =  (i + 1              == AttributeCount)
                   || (mapping[i]->stream != mapping[i + 1]->stream)
                   || (fetchSize          != mapping[i + 1]->offset);

        /* Store the current vertex element control value. */
        _gcmSETSTATEDATA(
            stateDelta, vertexCtrl, vertexCtrlState,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (mapping[i]->components) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) (mapping[i]->stream) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (mapping[i]->offset) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
            );

        /* Compute vertex shader input register for attribute. */
        link = (gctUINT32) (mapping[i] - Attributes);

        /* Set vertex shader input linkage. */
        switch (i & 3)
        {
        case 0:
            linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
            break;

        case 1:
            linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
            break;

        case 2:
            linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
            break;

        case 3:
            linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

            /* Store the current shader input control value. */
            _gcmSETSTATEDATA(
                stateDelta, shaderCtrl, shaderCtrlState, linkState
                );
            break;

        default:
            break;
        }

        /* Reset fetch size on a break. */
        if (fetchBreak)
        {
            fetchSize = 0;
        }
    }

    /* See if there are any attributes left to program in the vertex shader
    ** shader input registers. */
    if ((i & 3) != 0)
    {
        /* Store the current shader input control value. */
        _gcmSETSTATEDATA(
            stateDelta, shaderCtrl, shaderCtrlState, linkState
            );
    }

    /* Return the status. */
    gcmFOOTER();
    return status;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHARDWARE_FlushVertex(
    IN gcoHARDWARE Hardware
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETHARDWARE(Hardware);

    /* Verify the input parameters. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->chipModel == gcv700
#if gcd6000_SUPPORT
        || 1
#endif
        )
    {
        /* Flush L2 cache for GC700. */
        Hardware->flushL2 = gcvTRUE;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#if !OPT_VERTEX_ARRAY
gceSTATUS
gcoHARDWARE_SetVertexArray(
    IN gcoHARDWARE Hardware,
    IN gctUINT First,
    IN gctUINT32 Physical,
    IN gctUINT Stride,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams
    )
{
    gceSTATUS status;
    gctUINT i;
    gctBOOL multiStream;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attribute;
    gcsVERTEXARRAY_STREAM_PTR streamPtr;
    gctUINT32 streamCount = 0;
    gctUINT32 streamsTotal;
    gctUINT32 attributeCount = 0;
    gctUINT32 attributesTotal;
    gctUINT32 format = 0, size, endian = 0, normalize;
    gctUINT32 offset, fetchSize, fetchBreak;
    gctUINT32 stream, base, stride, link;
    gctUINT linkState = 0, linkCount = 0;

    gcoCMDBUF reserve;
    gcsSTATE_DELTA_PTR stateDelta;
    gctSIZE_T vertexCtrlStateCount, vertexCtrlReserveCount;
    gctSIZE_T shaderCtrlStateCount, shaderCtrlReserveCount;
    gctSIZE_T streamAddressStateCount, streamAddressReserveCount;
    gctSIZE_T streamStrideStateCount, streamStrideReserveCount;
    gctSIZE_T reserveSize;
    gctUINT32_PTR vertexCtrl;
    gctUINT32_PTR shaderCtrl;
    gctUINT32_PTR streamAddress;
    gctUINT32_PTR streamStride;

    gctUINT vertexCtrlState;
    gctUINT shaderCtrlState;
    gctUINT streamAddressState;
    gctUINT streamStrideState;
    gctUINT lastPhysical = 0;

    gcmHEADER_ARG("Hardware=0x%x First=%u Physical=0x%08x Stride=%u "
                  "AttributeCount=%u Attributes=0x%x StreamCount=%u "
                  "Streams=0x%x",
                  Hardware, First, Physical, Stride, AttributeCount, Attributes,
                  StreamCount, Streams);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT((AttributeCount == 0) || (Attributes != gcvNULL));
    gcmDEBUG_VERIFY_ARGUMENT((StreamCount == 0) || (Streams != gcvNULL));

    /* Determine whether the hardware supports more then one stream. */
    multiStream = (Hardware->streamCount > 1);

    /***************************************************************************
    ** Determine number of attributes and streams.
    */

    attributesTotal =  AttributeCount;
    streamsTotal    = (AttributeCount > 0) ? 1 : 0;

    for (i = 0, streamPtr = Streams; i < StreamCount; ++i, ++streamPtr)
    {
        /* Check if we have to skip this stream. */
        if (streamPtr->logical == gcvNULL)
        {
            continue;
        }

        /* Get stride and offset for this stream. */
        stream = streamsTotal;
        stride = streamPtr->subStream->stride;
        base   = streamPtr->attribute->offset;

        /* Make sure the streams don't overflow. */
        if ((stream >= Hardware->streamCount)
        ||  (stream >= gcdATTRIBUTE_COUNT)
        )
        {
            gcmONERROR(gcvSTATUS_TOO_COMPLEX);
        }

        /* Next stream. */
        ++streamsTotal;

        /* Walk all attributes in the stream. */
        for (attribute = streamPtr->attribute;
             attribute != gcvNULL;
             attribute = attribute->next
        )
        {
            /* Check if attribute falls outside the current stream. */
            if (attribute->offset >= base + stride)
            {
                /* Get stride and offset for this stream. */
                stream = streamsTotal;
                stride = attribute->vertexPtr->stride;
                base   = attribute->offset;

                /* Make sure the streams don't overflow. */
                if ((stream >= Hardware->streamCount)
                ||  (stream >= gcdATTRIBUTE_COUNT)
                )
                {
                    gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                }

                /* Next stream. */
                ++streamsTotal;
            }

            /* Next attribute. */
            ++attributesTotal;
        }
    }

    /* Return error if no attribute is enabled. */
    if (attributesTotal == 0)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    /***************************************************************************
    ** Determine the counts and reserve size.
    */

    /* State counts. */
    vertexCtrlStateCount = attributesTotal;
    shaderCtrlStateCount = gcmALIGN(attributesTotal, 4) >> 2;

    /* Reserve counts. */
    vertexCtrlReserveCount = 1 + (vertexCtrlStateCount | 1);
    shaderCtrlReserveCount = 1 + (shaderCtrlStateCount | 1);

    /* Set initial state addresses. */
    vertexCtrlState = 0x0180;
    shaderCtrlState = 0x0208;

    /* Determine the initial size of the buffer to reserve. */
    reserveSize
        = (vertexCtrlReserveCount + shaderCtrlReserveCount)
        *  gcmSIZEOF(gctUINT32);

    if (multiStream)
    {
        /* State counts. */
        streamAddressStateCount = Hardware->mixedStreams
                                ? streamsTotal
                                : Hardware->streamCount;
        streamStrideStateCount  = streamsTotal;

        /* Reserve counts. */
        streamAddressReserveCount = 1 + (streamAddressStateCount | 1);
        streamStrideReserveCount  = 1 + (streamStrideStateCount  | 1);

        /* Set initial state addresses. */
        streamAddressState = 0x01A0;
        streamStrideState  = 0x01A8;

        /* Add stream states. */
        reserveSize
            += (streamAddressReserveCount + streamStrideReserveCount)
             *  gcmSIZEOF(gctUINT32);
    }
    else
    {
        /* State counts. */
        streamAddressStateCount = 2;
        streamStrideStateCount  = 0;

        /* Reserve counts. */
        streamAddressReserveCount = 1 + (streamAddressStateCount | 1);
        streamStrideReserveCount  = 0;

        /* Set initial state addresses. */
        streamAddressState = 0x0193;
        streamStrideState  = 0x0194;

        /* Add stream states. */
        reserveSize
            += (streamAddressReserveCount + streamStrideReserveCount)
             *  gcmSIZEOF(gctUINT32);
    }

    /***************************************************************************
    ** Reserve command buffer state and init state commands.
    */

    /* Reserve space in the command buffer. */
    gcmONERROR(gcoBUFFER_Reserve(
        Hardware->buffer, reserveSize, gcvTRUE, &reserve
        ));

    /* Shortcut to the current delta. */
    stateDelta = Hardware->delta;

    /* Update the number of the elements. */
    stateDelta->elementCount = attributesTotal;

    /* Determine buffer pointers. */
    vertexCtrl    = (gctUINT32_PTR) reserve->lastReserve;
    shaderCtrl    = vertexCtrl + vertexCtrlReserveCount;
    streamAddress = shaderCtrl + shaderCtrlReserveCount;
    streamStride  = multiStream
                  ? streamAddress + streamAddressReserveCount
                  : streamAddress + 2;

    /* Init load state commands. */
    *vertexCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (vertexCtrlStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (vertexCtrlState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *shaderCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (shaderCtrlStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (shaderCtrlState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *streamAddress++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (streamAddressStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (streamAddressState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    if (multiStream)
    {
        *streamStride++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (streamStrideStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (streamStrideState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
    }

    /* Process the copied stream first. */
    if (AttributeCount > 0)
    {
        /* Store the stream address. */
        _gcmSETSTATEDATA(
            stateDelta, streamAddress, streamAddressState,
            lastPhysical = Physical - First * Stride
            );

        /* Store the stream stride. */
        _gcmSETSTATEDATA(
            stateDelta, streamStride, streamStrideState,
            multiStream
                ? ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (Stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
            );

        /* Next stream. */
        ++streamCount;

        /* Walk all attributes. */
        for (i = 0, attribute = Attributes, fetchSize = 0;
             i < AttributeCount;
             ++i, ++attribute
        )
        {
            /* Dispatch on vertex format. */
            switch (attribute->format)
            {
            case gcvVERTEX_BYTE:
                format = 0x0;
                endian = 0x0;
                break;

            case gcvVERTEX_UNSIGNED_BYTE:
                format = 0x1;
                endian = 0x0;
                break;

            case gcvVERTEX_SHORT:
                format = 0x2;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_SHORT:
                format = 0x3;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_INT:
                format = 0x4;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT:
                format = 0x5;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_FIXED:
                format = 0xB;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_HALF:
                format = 0x9;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_FLOAT:
                format = 0x8;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
                format = 0xD;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT_10_10_10_2:
                format = 0xC;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Get size. */
            size = attribute->vertexPtr->enable ? (gctUINT)attribute->vertexPtr->size : attribute->bytes / 4;
            link = attribute->vertexPtr->linkage;

            /* Get normalized flag. */
            normalize = attribute->vertexPtr->normalized
                      ? 0x2
                      : 0x0;

            /* Get vertex offset and size. */
            offset      = attribute->offset;
            fetchSize  += attribute->bytes;
            fetchBreak
                = (((i + 1) == AttributeCount)
                || (offset + attribute->bytes != attribute[1].offset))
                    ? 1 : 0;

            /* Store the current vertex element control value. */
            _gcmSETSTATEDATA(
                stateDelta, vertexCtrl, vertexCtrlState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (offset) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                );

            /* Next attribute. */
            ++attributeCount;

            /* Set vertex shader input linkage. */
            switch (linkCount & 3)
            {
            case 0:
                linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                break;

            case 1:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                break;

            case 2:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                break;

            case 3:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

                /* Store the current shader input control value. */
                _gcmSETSTATEDATA(
                    stateDelta, shaderCtrl, shaderCtrlState, linkState
                    );
                break;

            default:
                break;
            }

            /* Next vertex shader linkage. */
            ++linkCount;

            if (fetchBreak)
            {
                /* Reset fetch size on a break. */
                fetchSize = 0;
            }
        }
    }

    /* Walk all stream objects. */
    for (i = 0, streamPtr = Streams; i < StreamCount; ++i, ++streamPtr)
    {
        /* Check if we have to skip this stream. */
        if (streamPtr->logical == gcvNULL)
        {
            continue;
        }

        /* Get stride and offset for this stream. */
        stream = streamCount;
        stride = streamPtr->subStream->stride;
        base   = streamPtr->attribute->offset;

        /* Store the stream address. */
        _gcmSETSTATEDATA(
            stateDelta, streamAddress, streamAddressState,
            lastPhysical = streamPtr->physical + base
            );

        /* Store the stream stride. */
        _gcmSETSTATEDATA(
            stateDelta, streamStride, streamStrideState,
            multiStream
                ? ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
            );

        /* Next stream. */
        ++streamCount;

        /* Walk all attributes in the stream. */
        for (attribute = streamPtr->attribute, fetchSize = 0;
             attribute != gcvNULL;
             attribute = attribute->next
        )
        {
            /* Check if attribute falls outside the current stream. */
            if (attribute->offset >= base + stride)
            {
                /* Get stride and offset for this stream. */
                stream = streamCount;
                stride = attribute->vertexPtr->stride;
                base   = attribute->offset;

                /* Store the stream address. */
                _gcmSETSTATEDATA(
                    stateDelta, streamAddress, streamAddressState,
                    lastPhysical = streamPtr->physical + base
                    );

                /* Store the stream stride. */
                _gcmSETSTATEDATA(
                    stateDelta, streamStride, streamStrideState,
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                    );

                /* Next stream. */
                ++streamCount;
            }

            /* Dispatch on vertex format. */
            switch (attribute->format)
            {
            case gcvVERTEX_SHORT:
                format = 0x2;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_SHORT:
                format = 0x3;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_INT:
                format = 0x4;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT:
                format = 0x5;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_FIXED:
                format = 0xB;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_HALF:
                format = 0x9;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_FLOAT:
                format = 0x8;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
                format = 0xD;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT_10_10_10_2:
                format = 0xC;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_BYTE:
                format = 0x0;
                endian = 0x0;
                break;

            case gcvVERTEX_UNSIGNED_BYTE:
                format = 0x1;
                endian = 0x0;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Get size. */
            size = attribute->vertexPtr->size;
            link = attribute->vertexPtr->linkage;

            /* Get normalized flag. */
            normalize = attribute->vertexPtr->normalized
                      ? 0x2
                      : 0x0;

            /* Get vertex offset and size. */
            offset      = attribute->offset;
            fetchSize  += attribute->bytes;
            fetchBreak
                =  (((i + 1) == AttributeCount)
                || (attribute->next == gcvNULL)
                || (offset + attribute->bytes != attribute->next->offset))
                    ? 1 : 0;

            /* Store the current vertex element control value. */
            _gcmSETSTATEDATA(
                stateDelta, vertexCtrl, vertexCtrlState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (offset - base) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                );

            /* Next attribute. */
            ++attributeCount;

            /* Set vertex shader input linkage. */
            switch (linkCount & 3)
            {
            case 0:
                linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                break;

            case 1:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                break;

            case 2:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                break;

            case 3:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

                /* Store the current shader input control value. */
                _gcmSETSTATEDATA(
                    stateDelta, shaderCtrl, shaderCtrlState, linkState
                    );
                break;

            default:
                break;
            }

            /* Next vertex shader linkage. */
            ++linkCount;

            if (fetchBreak)
            {
                /* Reset fetch size on a break. */
                fetchSize = 0;
            }
        }
    }

    /* Check if the IP requires all streams to be programmed. */
    if (!Hardware->mixedStreams)
    {
        for (i = streamsTotal; i < streamAddressStateCount; ++i)
        {
            /* Replicate the last physical address for unknown stream
            ** addresses. */
            _gcmSETSTATEDATA(
                stateDelta, streamAddress, streamAddressState,
                lastPhysical
                );
        }
    }

    /* See if there are any attributes left to program in the vertex shader
    ** shader input registers. */
    if ((linkCount & 3) != 0)
    {
        _gcmSETSTATEDATA(
            stateDelta, shaderCtrl, shaderCtrlState, linkState
            );
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

#else

gceSTATUS
gcoHARDWARE_SetVertexArray(
    IN gcoHARDWARE Hardware,
    IN gctUINT First,
    IN gctUINT32 Physical,
    IN gctUINT BufferCount,
    IN gcsVERTEXARRAY_BUFFER_PTR Buffers,
    IN gctUINT AttributeCount,
    IN gcsVERTEXARRAY_ATTRIBUTE_PTR Attributes,
    IN gctUINT StreamCount,
    IN gcsVERTEXARRAY_STREAM_PTR Streams
    )
{
    gceSTATUS status;
    gctUINT i, j;
    gctBOOL multiStream;
    gcsVERTEXARRAY_ATTRIBUTE_PTR attribute;
    gcsVERTEXARRAY_STREAM_PTR streamPtr;
    gcsVERTEXARRAY_BUFFER_PTR bufferPtr;
    gctUINT32 streamCount = 0;
    gctUINT32 streamsTotal;
    gctUINT32 attributeCount = 0;
    gctUINT32 attributesTotal;
    gctUINT32 format = 0, size, endian = 0, normalize;
    gctUINT32 offset, fetchSize, fetchBreak;
    gctUINT32 stream, base, stride, link;
    gctUINT linkState = 0, linkCount = 0;

    gcoCMDBUF reserve;
    gcsSTATE_DELTA_PTR stateDelta;
    gctSIZE_T vertexCtrlStateCount, vertexCtrlReserveCount;
    gctSIZE_T shaderCtrlStateCount, shaderCtrlReserveCount;
    gctSIZE_T streamAddressStateCount, streamAddressReserveCount;
    gctSIZE_T streamStrideStateCount, streamStrideReserveCount;
    gctSIZE_T reserveSize;
    gctUINT32_PTR vertexCtrl;
    gctUINT32_PTR shaderCtrl;
    gctUINT32_PTR streamAddress;
    gctUINT32_PTR streamStride;

    gctUINT vertexCtrlState;
    gctUINT shaderCtrlState;
    gctUINT streamAddressState;
    gctUINT streamStrideState;
    gctUINT lastPhysical = 0;

    gcmHEADER_ARG("Hardware=0x%x First=%u Physical=0x%08x BufferCount=%u "
                  "Buffers=0x%x StreamCount=%u Streams=0x%x",
                  Hardware, First, Physical, BufferCount, Buffers,
                  StreamCount, Streams);

    gcmGETHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmDEBUG_VERIFY_ARGUMENT((BufferCount == 0) || (Buffers != gcvNULL));
    gcmDEBUG_VERIFY_ARGUMENT((AttributeCount == 0) || (Attributes != gcvNULL));
    gcmDEBUG_VERIFY_ARGUMENT((StreamCount == 0) || (Streams != gcvNULL));

    /* Determine whether the hardware supports more then one stream. */
    multiStream = (Hardware->streamCount > 1);

    /***************************************************************************
    ** Determine number of attributes and streams.
    */

    attributesTotal = 0;
    streamsTotal    = 0;

    for (i = 0, bufferPtr = Buffers; i < BufferCount; ++i, ++bufferPtr)
    {
        /* Check if we have to skip this buffer. */
        if (bufferPtr->count == 0)
        {
            continue;
        }

        stream = streamsTotal;

        /* Make sure the streams don't overflow. */
        if ((stream >= Hardware->streamCount)
        ||  (stream >= gcdATTRIBUTE_COUNT)
        )
        {
            gcmONERROR(gcvSTATUS_TOO_COMPLEX);
        }

        attributesTotal += bufferPtr->count;
        streamsTotal++;
    }

    /* If attribute total in buffers isn't the same as Attribute */
    /* Something must be wrong. */
    if (attributesTotal != AttributeCount)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    for (i = 0, streamPtr = Streams; i < StreamCount; ++i, ++streamPtr)
    {
        /* Check if we have to skip this stream. */
        if (streamPtr->logical == gcvNULL)
        {
            continue;
        }

        /* Get stride and offset for this stream. */
        stream = streamsTotal;
        stride = streamPtr->subStream->stride;
        base   = streamPtr->attribute->offset;

        /* Make sure the streams don't overflow. */
        if ((stream >= Hardware->streamCount)
        ||  (stream >= gcdATTRIBUTE_COUNT)
        )
        {
            gcmONERROR(gcvSTATUS_TOO_COMPLEX);
        }

        /* Next stream. */
        ++streamsTotal;

        /* Walk all attributes in the stream. */
        for (attribute = streamPtr->attribute;
             attribute != gcvNULL;
             attribute = attribute->next
        )
        {
            /* Check if attribute falls outside the current stream. */
            if (attribute->offset >= base + stride)
            {
                /* Get stride and offset for this stream. */
                stream = streamsTotal;
                stride = attribute->vertexPtr->stride;
                base   = attribute->offset;

                /* Make sure the streams don't overflow. */
                if ((stream >= Hardware->streamCount)
                ||  (stream >= gcdATTRIBUTE_COUNT)
                )
                {
                    gcmONERROR(gcvSTATUS_TOO_COMPLEX);
                }

                /* Next stream. */
                ++streamsTotal;
            }

            /* Next attribute. */
            ++attributesTotal;
        }
    }

    /* Return error if no attribute is enabled. */
    if (attributesTotal == 0)
    {
        gcmONERROR(gcvSTATUS_TOO_COMPLEX);
    }

    /***************************************************************************
    ** Determine the counts and reserve size.
    */

    /* State counts. */
    vertexCtrlStateCount = attributesTotal;
    shaderCtrlStateCount = gcmALIGN(attributesTotal, 4) >> 2;

    /* Reserve counts. */
    vertexCtrlReserveCount = 1 + (vertexCtrlStateCount | 1);
    shaderCtrlReserveCount = 1 + (shaderCtrlStateCount | 1);

    /* Set initial state addresses. */
    vertexCtrlState = 0x0180;
    shaderCtrlState = 0x0208;

    /* Determine the initial size of the buffer to reserve. */
    reserveSize
        = (vertexCtrlReserveCount + shaderCtrlReserveCount)
        *  gcmSIZEOF(gctUINT32);

    if (multiStream)
    {
        /* State counts. */
        streamAddressStateCount = Hardware->mixedStreams
                                ? streamsTotal
                                : Hardware->streamCount;
        streamStrideStateCount  = streamsTotal;

        /* Reserve counts. */
        streamAddressReserveCount = 1 + (streamAddressStateCount | 1);
        streamStrideReserveCount  = 1 + (streamStrideStateCount  | 1);

        /* Set initial state addresses. */
        streamAddressState = 0x01A0;
        streamStrideState  = 0x01A8;

        /* Add stream states. */
        reserveSize
            += (streamAddressReserveCount + streamStrideReserveCount)
             *  gcmSIZEOF(gctUINT32);
    }
    else
    {
        /* State counts. */
        streamAddressStateCount = 2;
        streamStrideStateCount  = 0;

        /* Reserve counts. */
        streamAddressReserveCount = 1 + (streamAddressStateCount | 1);
        streamStrideReserveCount  = 0;

        /* Set initial state addresses. */
        streamAddressState = 0x0193;
        streamStrideState  = 0x0194;

        /* Add stream states. */
        reserveSize
            += (streamAddressReserveCount + streamStrideReserveCount)
             *  gcmSIZEOF(gctUINT32);
    }

    /***************************************************************************
    ** Reserve command buffer state and init state commands.
    */

    /* Reserve space in the command buffer. */
    gcmONERROR(gcoBUFFER_Reserve(
        Hardware->buffer, reserveSize, gcvTRUE, &reserve
        ));

    /* Shortcut to the current delta. */
    stateDelta = Hardware->delta;

    /* Update the number of the elements. */
    stateDelta->elementCount = attributesTotal;

    /* Determine buffer pointers. */
    vertexCtrl    = (gctUINT32_PTR) reserve->lastReserve;
    shaderCtrl    = vertexCtrl + vertexCtrlReserveCount;
    streamAddress = shaderCtrl + shaderCtrlReserveCount;
    streamStride  = multiStream
                  ? streamAddress + streamAddressReserveCount
                  : streamAddress + 2;

    /* Init load state commands. */
    *vertexCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (vertexCtrlStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (vertexCtrlState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *shaderCtrl++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (shaderCtrlStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (shaderCtrlState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    *streamAddress++
        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (streamAddressStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (streamAddressState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

    if (multiStream)
    {
        *streamStride++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (streamStrideStateCount) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (streamStrideState) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
    }

    /* Process the copied stream first. */
    for(i = 0, bufferPtr = Buffers; i < BufferCount; ++i, ++bufferPtr)
    {
        /* Get stride and offset for this stream. */
        stream = streamCount;
        stride = bufferPtr->stride;

        /* Store the stream address. */
        _gcmSETSTATEDATA(
            stateDelta, streamAddress, streamAddressState,
            lastPhysical = Physical + bufferPtr->offset - First * stride
            );

        /* Store the stream stride. */
        _gcmSETSTATEDATA(
            stateDelta, streamStride, streamStrideState,
            multiStream
                ? ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
            );

        /* Next stream. */
        ++streamCount;

        /* Walk all attributes. */
        for (j = 0, fetchSize = 0; j < bufferPtr->count; j++)
        {
            attribute = Attributes + bufferPtr->map[j];

            /* Dispatch on vertex format. */
            switch (attribute->format)
            {
            case gcvVERTEX_BYTE:
                format = 0x0;
                endian = 0x0;
                break;

            case gcvVERTEX_UNSIGNED_BYTE:
                format = 0x1;
                endian = 0x0;
                break;

            case gcvVERTEX_SHORT:
                format = 0x2;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_SHORT:
                format = 0x3;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_INT:
                format = 0x4;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT:
                format = 0x5;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_FIXED:
                format = 0xB;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_HALF:
                format = 0x9;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_FLOAT:
                format = 0x8;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
                format = 0xD;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT_10_10_10_2:
                format = 0xC;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Get size. */
            size = attribute->vertexPtr->enable ? (gctUINT)attribute->vertexPtr->size : attribute->bytes / 4;
            link = attribute->vertexPtr->linkage;

            /* Get normalized flag. */
            normalize = attribute->vertexPtr->normalized
                      ? 0x2
                      : 0x0;

            /* Get vertex offset and size. */
            offset      = attribute->offset;
            fetchSize  += attribute->bytes;
            fetchBreak
                = ((j + 1) == bufferPtr->count)
                || offset + attribute->bytes != Attributes[bufferPtr->map[j+1]].offset
                    ? 1 : 0;

            /* Store the current vertex element control value. */
            _gcmSETSTATEDATA(
                stateDelta, vertexCtrl, vertexCtrlState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (offset) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                );

            /* Next attribute. */
            ++attributeCount;

            /* Set vertex shader input linkage. */
            switch (linkCount & 3)
            {
            case 0:
                linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                break;

            case 1:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                break;

            case 2:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                break;

            case 3:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

                /* Store the current shader input control value. */
                _gcmSETSTATEDATA(
                    stateDelta, shaderCtrl, shaderCtrlState, linkState
                    );
                break;

            default:
                break;
            }

            /* Next vertex shader linkage. */
            ++linkCount;

            if (fetchBreak)
            {
                /* Reset fetch size on a break. */
                fetchSize = 0;
            }
        }
    }

    /* Walk all stream objects. */
    for (i = 0, streamPtr = Streams; i < StreamCount; ++i, ++streamPtr)
    {
        /* Check if we have to skip this stream. */
        if (streamPtr->logical == gcvNULL)
        {
            continue;
        }

        /* Get stride and offset for this stream. */
        stream = streamCount;
        stride = streamPtr->subStream->stride;
        base   = streamPtr->attribute->offset;

        /* Store the stream address. */
        _gcmSETSTATEDATA(
            stateDelta, streamAddress, streamAddressState,
            lastPhysical = streamPtr->physical + base
            );

        /* Store the stream stride. */
        _gcmSETSTATEDATA(
            stateDelta, streamStride, streamStrideState,
            multiStream
                ? ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
            );

        /* Next stream. */
        ++streamCount;

        /* Walk all attributes in the stream. */
        for (attribute = streamPtr->attribute, fetchSize = 0;
             attribute != gcvNULL;
             attribute = attribute->next
        )
        {
            /* Check if attribute falls outside the current stream. */
            if (attribute->offset >= base + stride)
            {
                /* Get stride and offset for this stream. */
                stream = streamCount;
                stride = attribute->vertexPtr->stride;
                base   = attribute->offset;

                /* Store the stream address. */
                _gcmSETSTATEDATA(
                    stateDelta, streamAddress, streamAddressState,
                    lastPhysical = streamPtr->physical + base
                    );

                /* Store the stream stride. */
                _gcmSETSTATEDATA(
                    stateDelta, streamStride, streamStrideState,
                    ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0))) | (((gctUINT32) ((gctUINT32) (stride) & ((gctUINT32) ((((1 ? 8:0) - (0 ? 8:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:0) - (0 ? 8:0) + 1))))))) << (0 ? 8:0)))
                    );

                /* Next stream. */
                ++streamCount;
            }

            /* Dispatch on vertex format. */
            switch (attribute->format)
            {
            case gcvVERTEX_SHORT:
                format = 0x2;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_SHORT:
                format = 0x3;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_INT:
                format = 0x4;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT:
                format = 0x5;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_FIXED:
                format = 0xB;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_HALF:
                format = 0x9;
                endian = Hardware->bigEndian
                       ? 0x1
                       : 0x0;
                break;

            case gcvVERTEX_FLOAT:
                format = 0x8;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_UNSIGNED_INT_10_10_10_2:
                format = 0xD;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_INT_10_10_10_2:
                format = 0xC;
                endian = Hardware->bigEndian
                       ? 0x2
                       : 0x0;
                break;

            case gcvVERTEX_BYTE:
                format = 0x0;
                endian = 0x0;
                break;

            case gcvVERTEX_UNSIGNED_BYTE:
                format = 0x1;
                endian = 0x0;
                break;

            default:
                gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
            }

            /* Get size. */
            size = attribute->vertexPtr->size;
            link = attribute->vertexPtr->linkage;

            /* Get normalized flag. */
            normalize = attribute->vertexPtr->normalized
                      ? 0x2
                      : 0x0;

            /* Get vertex offset and size. */
            offset      = attribute->offset;
            fetchSize  += attribute->bytes;
            fetchBreak
                =
                   (attribute->next == gcvNULL)
                || (offset + attribute->bytes != attribute->next->offset)
                    ? 1 : 0;

            /* Store the current vertex element control value. */
            _gcmSETSTATEDATA(
                stateDelta, vertexCtrl, vertexCtrlState,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (size) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14))) | (((gctUINT32) ((gctUINT32) (normalize) & ((gctUINT32) ((((1 ? 15:14) - (0 ? 15:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:14) - (0 ? 15:14) + 1))))))) << (0 ? 15:14)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (endian) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8))) | (((gctUINT32) ((gctUINT32) (stream) & ((gctUINT32) ((((1 ? 10:8) - (0 ? 10:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:8) - (0 ? 10:8) + 1))))))) << (0 ? 10:8)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (offset - base) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (fetchSize) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) ((gctUINT32) (fetchBreak) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7)))
                );

            /* Next attribute. */
            ++attributeCount;

            /* Set vertex shader input linkage. */
            switch (linkCount & 3)
            {
            case 0:
                linkState = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 5:0) - (0 ? 5:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:0) - (0 ? 5:0) + 1))))))) << (0 ? 5:0)));
                break;

            case 1:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 13:8) - (0 ? 13:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:8) - (0 ? 13:8) + 1))))))) << (0 ? 13:8)));
                break;

            case 2:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 21:16) - (0 ? 21:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:16) - (0 ? 21:16) + 1))))))) << (0 ? 21:16)));
                break;

            case 3:
                linkState = ((((gctUINT32) (linkState)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24))) | (((gctUINT32) ((gctUINT32) (link ) & ((gctUINT32) ((((1 ? 29:24) - (0 ? 29:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:24) - (0 ? 29:24) + 1))))))) << (0 ? 29:24)));

                /* Store the current shader input control value. */
                _gcmSETSTATEDATA(
                    stateDelta, shaderCtrl, shaderCtrlState, linkState
                    );
                break;

            default:
                break;
            }

            /* Next vertex shader linkage. */
            ++linkCount;

            if (fetchBreak)
            {
                /* Reset fetch size on a break. */
                fetchSize = 0;
            }
        }
    }

    /* Check if the IP requires all streams to be programmed. */
    if (!Hardware->mixedStreams)
    {
        for (i = streamsTotal; i < streamAddressStateCount; ++i)
        {
            /* Replicate the last physical address for unknown stream
            ** addresses. */
            _gcmSETSTATEDATA(
                stateDelta, streamAddress, streamAddressState,
                lastPhysical
                );
        }
    }

    /* See if there are any attributes left to program in the vertex shader
    ** shader input registers. */
    if ((linkCount & 3) != 0)
    {
        _gcmSETSTATEDATA(
            stateDelta, shaderCtrl, shaderCtrlState, linkState
            );
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif

#endif /* VIVANTE_NO_3D */

