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




#include "gc_hal_user_hardware_precomp_vg.h"

#if gcdENABLE_VG

#include "gc_hal_user_vg.h"

/* TODO: Remove these includes! Not allowed in the HAL! */
#include <math.h>

#define _GC_OBJ_ZONE            gcvZONE_VG

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

#define gcvREPORT_TARGET        0
#define gcvDUMP_COMMAND_BUFFER  0

#   define gcvCOMMAND_BUFFER_NAME   "TSOverflow"

/* Rectangle structure used by the VG hardware. */
typedef struct _gcsVG_HWRECT * gcsVG_HWRECT_PTR;
typedef struct _gcsVG_HWRECT
{
    /** Left location of the rectangle. */
    gctUINT     x       : 16;

    /** Top location of the rectangle. */
    gctUINT     y       : 16;

    /** Width of the rectangle. */
    gctUINT     width   : 16;

    /** Height of the rectangle. */
    gctUINT     height  : 16;
}
gcsVG_HWRECT;

static gceSTATUS
_GetVgFormat(
    IN gcsSURF_INFO_PTR Surface,
    OUT gctUINT32_PTR Format,
    OUT gctUINT32_PTR Swizzle
    )
{
    switch (Surface->format)
    {
    case gcvSURF_A8:
        *Format  = 0x0;
        *Swizzle = 0x0;
        break;

    case gcvSURF_A4R4G4B4:
        *Format  = 0x4;
        *Swizzle = 0x0;
        break;

    case gcvSURF_R4G4B4A4:
        *Format  = 0x4;
        *Swizzle = 0x1;
        break;

    case gcvSURF_A4B4G4R4:
        *Format  = 0x4;
        *Swizzle = 0x2;
        break;

    case gcvSURF_B4G4R4A4:
        *Format  = 0x4;
        *Swizzle = 0x3;
        break;

    case gcvSURF_A1R5G5B5:
        *Format  = 0x5;
        *Swizzle = 0x0;
        break;

    case gcvSURF_R5G5B5A1:
        *Format  = 0x5;
        *Swizzle = 0x1;
        break;

    case gcvSURF_A1B5G5R5:
        *Format  = 0x5;
        *Swizzle = 0x2;
        break;

    case gcvSURF_B5G5R5A1:
        *Format  = 0x5;
        *Swizzle = 0x3;
        break;

    case gcvSURF_R5G6B5:
        *Format  = 0x1;
        *Swizzle = 0x0;
        break;

    case gcvSURF_B5G6R5:
        *Format  = 0x1;
        *Swizzle = 0x2;
        break;

    case gcvSURF_X8R8G8B8:
        *Format  = 0x2;
        *Swizzle = 0x0;
        break;

    case gcvSURF_A8R8G8B8:
        *Format  = 0x3;
        *Swizzle = 0x0;
        break;

    case gcvSURF_R8G8B8X8:
        *Format  = 0x2;
        *Swizzle = 0x1;
        break;

    case gcvSURF_R8G8B8A8:
        *Format  = 0x3;
        *Swizzle = 0x1;
        break;

    case gcvSURF_B8G8R8X8:
        *Format  = 0x3;
        *Swizzle = 0x3;
        break;

    case gcvSURF_B8G8R8A8:
        *Format  = 0x3;
        *Swizzle = 0x3;
        break;

    case gcvSURF_X8B8G8R8:
        *Format  = 0x3;
        *Swizzle = 0x2;
        break;

    case gcvSURF_A8B8G8R8:
        *Format  = 0x3;
        *Swizzle = 0x2;
        break;

    default:
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): Unsupported format %d.\n",
            __FUNCTION__, __LINE__,
            Surface->format
            );

        return gcvSTATUS_NOT_SUPPORTED;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvertFormat(
    IN gceSURF_FORMAT Format,
    OUT gctUINT32_PTR ImageFormat,
    OUT gctUINT32_PTR Swizzle
    )
{
    switch (Format)
    {
    case gcvSURF_A4:
        *ImageFormat = 0x1;
        break;

    case gcvSURF_A8:
        *ImageFormat = 0x2;
        break;

    case gcvSURF_L8:
        *ImageFormat = 0x0;
        break;

    case gcvSURF_A4R4G4B4:
    case gcvSURF_R4G4B4A4:
    case gcvSURF_A4B4G4R4:
    case gcvSURF_B4G4R4A4:
        *ImageFormat = 0x3;
        break;

    case gcvSURF_A1R5G5B5:
    case gcvSURF_R5G5B5A1:
    case gcvSURF_A1B5G5R5:
    case gcvSURF_B5G5R5A1:
        *ImageFormat = 0x4;
        break;

    case gcvSURF_R5G6B5:
    case gcvSURF_B5G6R5:
        *ImageFormat = 0x5;
        break;

    case gcvSURF_X8R8G8B8:
    case gcvSURF_R8G8B8X8:
    case gcvSURF_X8B8G8R8:
    case gcvSURF_B8G8R8X8:
        *ImageFormat = 0x6;
        break;

    case gcvSURF_A8R8G8B8:
    case gcvSURF_R8G8B8A8:
    case gcvSURF_A8B8G8R8:
    case gcvSURF_B8G8R8A8:
        *ImageFormat = 0x7;
        break;

    default:
        return gcvSTATUS_NOT_SUPPORTED;
    }

    switch (Format)
    {
    case gcvSURF_A4:
    case gcvSURF_A8:
    case gcvSURF_L8:
    case gcvSURF_A4R4G4B4:
    case gcvSURF_A1R5G5B5:
    case gcvSURF_R5G6B5:
    case gcvSURF_X8R8G8B8:
    case gcvSURF_A8R8G8B8:
        *Swizzle = 0x0;
        break;

    case gcvSURF_R4G4B4A4:
    case gcvSURF_R5G5B5A1:
    case gcvSURF_R8G8B8X8:
    case gcvSURF_R8G8B8A8:
        *Swizzle = 0x1;
        break;

    case gcvSURF_A4B4G4R4:
    case gcvSURF_A1B5G5R5:
    case gcvSURF_B5G6R5:
    case gcvSURF_X8B8G8R8:
    case gcvSURF_A8B8G8R8:
        *Swizzle = 0x2;
        break;

    case gcvSURF_B4G4R4A4:
    case gcvSURF_B5G5R5A1:
    case gcvSURF_B8G8R8X8:
    case gcvSURF_B8G8R8A8:
        *Swizzle = 0x3;
        break;

    default:
        return gcvSTATUS_NOT_SUPPORTED;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_SetSampler(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT Sampler,
    IN gcsSURF_INFO_PTR Image,
    IN gceTILE_MODE TileMode,
    IN gctBOOL Mask,
    IN gceIMAGE_FILTER BilinearFilter,
    IN gctBOOL ImageFilter,
    IN gctUINT32 OriginX,
    IN gctUINT32 OriginY,
    IN gctUINT32 SizeX,
    IN gctUINT32 SizeY
    )
{
    gceSTATUS status;


    static const gctUINT32 _tileMode[] =
    {
        0x0, /* gcvTILE_FILL    */
        0x1, /* gcvTILE_PAD     */
        0x2, /* gcvTILE_REPEAT  */
        0x3             /* gcvTILE_REFLECT */
    };

    static const gctUINT32 _mask[] =
    {
        0x0, /* gcvFALSE */
        0x1                /* gcvTRUE  */
    };

    static const gctUINT32 _filter[] =
    {
        0x0, /* gcvFILTER_POINT     */
        0x1, /* gcvFILTER_LINEAR    */
        0x2, /* gcvFILTER_BI_LINEAR */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Sampler < 2);
    gcmVERIFY_ARGUMENT(Image != gcvNULL);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(TileMode, _tileMode));
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Mask, _mask));
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(BilinearFilter, _filter));

    do
    {
        gctUINT32 format, swizzle;
        gctUINT32 filter;
        gctUINT32 preMultipliedIn;
        gctUINT32 preMultipliedOut;
        gctUINT32 coordType;
        gctUINT32 control;
        gctUINT32 stride;
        gctUINT32 origin;
        gctUINT32 size;
        gcePOOL pool;
        gctUINT32 offset;

        /* Convert the image format. */
        gcmERR_BREAK(_ConvertFormat(Image->format, &format, &swizzle));

        /* Determine the filter value. */
        filter = _filter[BilinearFilter];

        /* Never premultiply input paint/image. */
        preMultipliedIn = 0x1;

        /* Demultiply if input is premultiplied. */
        preMultipliedOut = (Image->colorType & gcvSURF_COLOR_ALPHA_PRE)
            ? 0x0
            : 0x1;

        /* Determine the type of coordinates. */
        coordType = ImageFilter
            ? 0x1
            : 0x0;

        control
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 3:0) - (0 ? 3:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:0) - (0 ? 3:0) + 1))))))) << (0 ? 3:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4))) | (((gctUINT32) ((gctUINT32) (swizzle) & ((gctUINT32) ((((1 ? 6:4) - (0 ? 6:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:4) - (0 ? 6:4) + 1))))))) << (0 ? 6:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) ((gctUINT32) (preMultipliedIn) & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (_tileMode[TileMode]) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (filter) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20))) | (((gctUINT32) ((gctUINT32) (_mask[Mask]) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24))) | (((gctUINT32) ((gctUINT32) (preMultipliedOut) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28))) | (((gctUINT32) ((gctUINT32) (coordType) & ((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28)));

        stride = (SizeY == 0)
            ? 0
            : Image->stride;

        origin
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (OriginX) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (OriginY) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)));

        size
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0))) | (((gctUINT32) ((gctUINT32) (SizeX) & ((gctUINT32) ((((1 ? 13:0) - (0 ? 13:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:0) - (0 ? 13:0) + 1))))))) << (0 ? 13:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16))) | (((gctUINT32) ((gctUINT32) (SizeY) & ((gctUINT32) ((((1 ? 29:16) - (0 ? 29:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:16) - (0 ? 29:16) + 1))))))) << (0 ? 29:16)));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x0287C >> 2,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x02890 >> 2) + Sampler,
            control
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SplitAddress(
            Hardware,
            Image->node.physical,
            &pool, &offset));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x028A0 >> 2) + Sampler,
            (pool == gcvPOOL_VIRTUAL) ?
            Image->node.physical : gcmFIXADDRESS(Image->node.physical)
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x028A8 >> 2) + Sampler,
            stride
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x028B0 >> 2) + Sampler,
            origin
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x028B8 >> 2) + Sampler,
            size
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_SendSemaphore(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    OUT gctSIZE_T * Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Logical=0x%x Bytes=0x%x",
                  Hardware, Logical, Bytes);

    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
#if gcdENABLE_TS_DOUBLE_BUFFER
        if (Hardware->vgDoubleBuffer)
        {
            /* Send 0/1 switching semaphore. */
            gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
                Hardware,
                Logical,
                gcvBLOCK_TESSELLATOR2,
                gcvBLOCK_VG2,
                gcvHOW_SEMAPHORE,
                Bytes
                ));
        }
        else
#endif
        {
            /* No STALL needed. */
            if (Bytes != gcvNULL)
            {
                * Bytes = 0;
            }

            /* Success. */
            status = gcvSTATUS_OK;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_SendStall(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    OUT gctSIZE_T * Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Logical=0x%x Bytes=0x%x",
                  Hardware, Logical, Bytes);

    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
#if gcdENABLE_TS_DOUBLE_BUFFER
        if (Hardware->vgDoubleBuffer)
        {
            /* Still need to skip initial stalls? */
            if (Hardware->vg.stallSkipCount > 0)
            {
                /* No STALL this time. */
                if (Bytes != gcvNULL)
                {
                    * Bytes = 0;
                }
                else
                {
                    /* Update skip counter. */
                    Hardware->vg.stallSkipCount -= 1;
                }

                /* Success. */
                status = gcvSTATUS_OK;
            }
            else
            {
                /* Send 0/1 switching state. */
                gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
                    Hardware,
                    Logical,
                    gcvBLOCK_TESSELLATOR2,
                    gcvBLOCK_VG2,
                    gcvHOW_STALL,
                    Bytes
                    ));
            }
        }
        else
        {
            /* No STALL needed. */
            if (Bytes != gcvNULL)
            {
                * Bytes = 0;
            }

            /* If the additional semaphore/stall commands are not
               supported by the hardware, vgDoubleBuffer will be set
               to zero and we will get to this point. Nothing needs
               to be done here because the context WILL NOT cache
               states in this case and VG states will buffer up and
               stall TS making sure that TS clear does not destroy
               the contents of TS buffer prematurely. */
            status = gcvSTATUS_OK;
        }
#else
        gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
            Hardware,
            Logical,
            gcvBLOCK_TESSELLATOR,
            gcvBLOCK_VG,
            gcvHOW_SEMAPHORE_STALL,
            Bytes
            ));
#endif
    }
    while (gcvFALSE);


    gcmFOOTER();
    return status;
}


/******************************************************************************\
********************************** gcoVG Object ********************************
\******************************************************************************/

#if gcvDUMP_COMMAND_BUFFER
static void
_DumpCommandBuffer(
    IN gcoVG Vg,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gcsCMDBUFFER_PTR buffer;
    gctUINT bufferCount;
    gctUINT bufferIndex;
    gctUINT i, count;
    gctUINT size;
    gctUINT32_PTR data;

#if defined(gcvCOMMAND_BUFFER_NAME)
    static gctUINT arrayCount = 0;
#endif

    /* Reset the count. */
    bufferCount = 0;

    /* Set the initial buffer. */
    buffer = CommandBuffer;

    /* Loop through all subbuffers. */
    while (buffer)
    {
        /* Update the count. */
        bufferCount += 1;

        /* Advance to the next subbuffer. */
        buffer = buffer->nextSubBuffer;
    }

    if (bufferCount > 1)
    {
#if defined(gcvCOMMAND_BUFFER_NAME)
        gcmTRACE_ZONE(
            gcvLEVEL_INFO,
            gcvZONE_COMMAND,
            "/* COMMAND BUFFER SET: %d buffers. */\n",
            bufferCount
            );
#else
        gcmTRACE_ZONE(
            gcvLEVEL_INFO,
            gcvZONE_COMMAND,
            "COMMAND BUFFER SET: %d buffers.\n",
            bufferCount
            );
#endif
    }

    /* Reset the buffer index. */
    bufferIndex = 0;

    /* Set the initial buffer. */
    buffer = CommandBuffer;

    /* Loop through all subbuffers. */
    while (buffer)
    {
        /* Determine the size of the buffer. */
        size = buffer->dataCount * Vg->bufferInfo.commandAlignment;

        /* A single buffer? */
        if (bufferCount == 1)
        {
#if defined(gcvCOMMAND_BUFFER_NAME)
            gcmTRACE_ZONE(
                gcvLEVEL_INFO,
                gcvZONE_COMMAND,
                "/* COMMAND BUFFER: count=%d (0x%X), size=%d bytes @ %08X. */\n",
                buffer->dataCount,
                buffer->dataCount,
                size,
                buffer->address
                );
#else
            gcmTRACE_ZONE(
                gcvLEVEL_INFO,
                gcvZONE_COMMAND,
                "/* COMMAND BUFFER: count=%d (0x%X), size=%d bytes @ %08X. */\n",
                buffer->dataCount,
                buffer->dataCount,
                size,
                buffer->address
                );
#endif
        }
        else
        {
#if defined(gcvCOMMAND_BUFFER_NAME)
            gcmTRACE_ZONE(
                gcvLEVEL_INFO,
                gcvZONE_COMMAND,
                "/* COMMAND BUFFER %d: count=%d (0x%X), size=%d bytes @ %08X. */\n",
                bufferIndex,
                buffer->dataCount,
                buffer->dataCount,
                size,
                buffer->address
                );
#else
            gcmTRACE_ZONE(
                gcvLEVEL_INFO,
                gcvZONE_COMMAND,
                "COMMAND BUFFER %d: count=%d (0x%X), size=%d bytes @ %08X.\n",
                bufferIndex,
                buffer->dataCount,
                buffer->dataCount,
                size,
                buffer->address
                );
#endif
        }

        /* Determine the number of double words to print. */
        count = size / 4;

        /* Determine the buffer location. */
        data = (gctUINT32_PTR)
        (
            (gctUINT8_PTR) buffer + buffer->bufferOffset
        );

#if defined(gcvCOMMAND_BUFFER_NAME)
        gcmTRACE_ZONE(
            gcvLEVEL_INFO,
            gcvZONE_COMMAND,
            "unsigned int _" gcvCOMMAND_BUFFER_NAME "_%d[] =\n",
            arrayCount
            );

        gcmTRACE_ZONE(
            gcvLEVEL_INFO,
            gcvZONE_COMMAND,
            "{\n"
            );

        arrayCount += 1;
#endif

        for (i = 0; i < count; i += 1)
        {
            if ((i % 8) == 0)
            {
#if defined(gcvCOMMAND_BUFFER_NAME)
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMMAND, "\t");
#else
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMMAND, "  ");
#endif
            }

            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMMAND, "0x%08X", data[i]);

            if (i + 1 == count)
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMMAND, "\n");
            }
            else
            {
                if (((i + 1) % 8) == 0)
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMMAND, ",\n");
                }
                else
                {
                    gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMMAND, ", ");
                }
            }
        }

#if defined(gcvCOMMAND_BUFFER_NAME)
        gcmTRACE_ZONE(
            gcvLEVEL_INFO,
            gcvZONE_COMMAND,
            "};\n\n"
            );
#endif

        /* Advance to the next subbuffer. */
        buffer = buffer->nextSubBuffer;
        bufferIndex += 1;
    }
}
#endif

gceSTATUS
gcoVG_FinalizePath(
    IN gcoVG Vg,
    IN gcsPATH_DATA_PTR PathData
    )
{
    gceSTATUS status;

    gcsPATH_BUFFER_INFO_PTR pathInfo;
    gcsCOMMAND_BUFFER_INFO_PTR bufferInfo;

    gctUINT pathHeadReserve;
    gctUINT pathTailReserve;
    gctUINT bufferTailReserve;
    gctUINT commandAlignment;
    gctINT32 interruptId;

    gcsPATH_DATA_PTR path;
    gcsCMDBUFFER_PTR next;
    gcsCMDBUFFER_PTR buffer;

    gctUINT8_PTR data;
    gctUINT32_PTR headCommands;
    gctUINT8_PTR previousPathTerminator;
    gctUINT8_PTR previousBufferTerminator;

    gctBOOL vg20;

    gctUINT tailOffset;
    gctUINT pathDataSize;
    gctUINT pathDataCount;

    gctUINT dataSize;
    gctUINT bufferDataSize;
    gctUINT bufferDataCount;

    gcmHEADER_ARG("Vg=0x%x PathData=0x%x",
                  Vg, PathData);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);
    gcmVERIFY_ARGUMENT(PathData != gcvNULL);

    /* Get shortcuts to the buffer parameters. */
    pathInfo   = &Vg->pathInfo;
    bufferInfo = &Vg->bufferInfo;

    pathHeadReserve   = pathInfo->reservedForHead;
    pathTailReserve   = pathInfo->reservedForTail;
    bufferTailReserve = bufferInfo->staticTailSize;
    commandAlignment  = bufferInfo->commandAlignment;
    interruptId       = bufferInfo->feBufferInt;

    /* Get VG 2.0 support flag. */
    vg20 = Vg->vg20;

    /* Set the initial buffer. */
    path = PathData;

    /* Reset the previous buffer terminator pointer. */
    previousPathTerminator   = gcvNULL;
    previousBufferTerminator = gcvNULL;

    /* Walk through all buffers. */
    while (path)
    {
        /* Get a shortcut to the command buffer and determine
           the data pointer. */
        buffer = &path->data;
        data = (gctUINT8_PTR) buffer + buffer->bufferOffset;

        /* Get the pointer to the next buffer. */
        next = buffer->nextSubBuffer;

        /* Get a shortcut to the accumulated data size. */
        dataSize = buffer->offset;

        /* Determine the buffer tail offset. */
        tailOffset = gcmALIGN(dataSize + pathTailReserve, commandAlignment);

        /* Determine the size of the path data. */
        pathDataSize  = tailOffset - pathHeadReserve;
        pathDataCount = pathDataSize / commandAlignment;

        /* Determine the size of the buffer. */
        bufferDataSize  = tailOffset + bufferTailReserve;
        bufferDataCount = bufferDataSize / commandAlignment;

        /* Set the buffer data count. */
        buffer->dataCount = bufferDataCount;

        /* Set initial command pointer. */
        headCommands = (gctUINT32_PTR) data;

        /* Determine placements of the head commands. */
        if (vg20)
        {
            gctUINT32 format;

            /* Data type translation table. */
            static const gctUINT32 _format[] =
            {
                0x0, /* gcePATHTYPE_INT8  */
                0x1, /* gcePATHTYPE_INT16 */
                0x2, /* gcePATHTYPE_INT32 */
                0x3     /* gcePATHTYPE_FLOAT */
            };

            /* Get data type. */
            gcmASSERT(gcmIS_VALID_INDEX(path->dataType, _format));
            format = _format[path->dataType];

            /* Initialize the STATE command. */
            gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
                Vg->hw,
                headCommands, 0x028D0 >> 2, 1,
                gcvNULL
                ));

            /* Set the datatype. */
            headCommands[1]
                = (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) ((gctUINT32) (format) & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) &((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23))));

            /* Advance the command pointer. */
            headCommands += 2;
        }

        /* Initialize the DATA command. */
        gcmERR_BREAK(gcoVGHARDWARE_DataCommand(
            Vg->hw,
            headCommands, pathDataCount,
            gcvNULL
            ));

        /* Is there a previous buffer? */
        if (previousBufferTerminator != gcvNULL)
        {
            /* Terminate the path data. */
            *previousPathTerminator = 0x0A;

            /* Link the previous command buffer to the current one. */
            gcmERR_BREAK(gcoVGHARDWARE_FetchCommand(
                Vg->hw,
                previousBufferTerminator,
                buffer->address,
                bufferDataCount,
                gcvNULL
                ));
        }

        /* Determine the location of the terminating commands. */
        previousPathTerminator   = data + dataSize;
        previousBufferTerminator = data + tailOffset;

        /* Advance to the next buffer. */
        path = (gcsPATH_DATA_PTR) next;
    }

    /* Terminate the last path data. */
    *previousPathTerminator = 0x00;

    /* Terminate the command buffer. */
    if (Vg->vg21)
    {
        status = gcoVGHARDWARE_ReturnCommand(
            Vg->hw,
            previousBufferTerminator,
            gcvNULL
            );
    }
    else
    {
        status = gcoVGHARDWARE_EndCommand(
            Vg->hw,
            previousBufferTerminator,
            interruptId, gcvNULL
            );
    }

    /* Dump the contents of the buffer. */
#if gcvDUMP_COMMAND_BUFFER
    _DumpCommandBuffer(Vg, &PathData->data);
#endif

    /* Return status. */
    gcmFOOTER();
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Miscellaneous Functions. ***************************************
\******************************************************************************/

gctUINT8
gcoVGHARDWARE_PackColorComponent(
    gctFLOAT Value
    )
{
#if gcdENABLE_PERFORMANCE_PRIOR
    gctUINT8 result = 0;

    if (Value >= 1.0)
    {
        result = 255;
    }
    else if (Value <= 0.0)
    {
        result = 0;
    }
    else
    {
        gctINT      E           = 0;
        gctUINT     M           = 0;
        gctUINT     iValue      = *(gctUINT*)&Value;
        gctUINT     mantissa    = iValue            & 0x7fffff;
        gctUINT     exp         = (iValue >> 23)    & 0xff;
        /*gctUINT       sign        = (iValue >> 31)    & 0x1;*/

        gcmASSERT ((exp != 0) && (exp != 0xff));
        E = exp - 127;
        M = (1 << 23) + mantissa;

        if ((E - 15) >= 0)
        {
            result = M << (E - 15);
        }
        else
        {
            result = M >> (15 - E);
        }

    }

    return result;
#else
    /* Compute the rounded normalized value. */
    gctFLOAT rounded = Value * 255.0f + 0.5f;

    /* Get the integer part. */
    gctINT roundedInt = (gctINT) rounded;

    /* Clamp to 0..1 range. */
    gctUINT8 clamped = (gctUINT8) gcmCLAMP(roundedInt, 0, 255);

    /* Return result. */
    return clamped;
#endif
}

gctUINT32
gcoVGHARDWARE_PackColor32(
    IN gctFLOAT Red,
    IN gctFLOAT Green,
    IN gctFLOAT Blue,
    IN gctFLOAT Alpha
    )
{
    gctUINT8 r, g, b, a;
    gctUINT32 result;

    /* Convert color to gctUINT8 format. */
    r = gcoVGHARDWARE_PackColorComponent(Red);
    g = gcoVGHARDWARE_PackColorComponent(Green);
    b = gcoVGHARDWARE_PackColorComponent(Blue);
    a = gcoVGHARDWARE_PackColorComponent(Alpha);

    /* Assemble colors. */
    result = (a << 24) | (r << 16) | (g <<  8) |  b;

    /* Return result. */
    return result;
}


/******************************************************************************\
** gcoVGHARDWARE: Query functions. ***********************************************
\******************************************************************************/

gctBOOL
gcoVGHARDWARE_IsMaskSupported(
    IN gceSURF_FORMAT Format
    )
{
    return (Format == gcvSURF_A8);
}

gctBOOL
gcoVGHARDWARE_IsTargetSupported(
    IN gceSURF_FORMAT Format
    )
{
    switch (Format)
    {
    /* Alpha formats. */
    case gcvSURF_A8:

    /* 16-bit A4R4G4B4 formats. */
    case gcvSURF_A4R4G4B4:
    case gcvSURF_R4G4B4A4:
    case gcvSURF_A4B4G4R4:
    case gcvSURF_B4G4R4A4:

    /* 16-bit A1R5G5B5 formats. */
    case gcvSURF_A1R5G5B5:
    case gcvSURF_R5G5B5A1:
    case gcvSURF_A1B5G5R5:
    case gcvSURF_B5G5R5A1:

    /* 16-bit R5G6B5 formats. */
    case gcvSURF_R5G6B5:
    case gcvSURF_B5G6R5:

    /* 32-bit X8R8G8B8 formats. */
    case gcvSURF_X8R8G8B8:
    case gcvSURF_R8G8B8X8:
    case gcvSURF_X8B8G8R8:
    case gcvSURF_B8G8R8X8:

    /* 32-bit A8R8G8B8 formats. */
    case gcvSURF_A8R8G8B8:
    case gcvSURF_R8G8B8A8:
    case gcvSURF_A8B8G8R8:
    case gcvSURF_B8G8R8A8:

        /* Supported. */
        return gcvTRUE;

    default:
        /* Not supported. */
        return gcvFALSE;
    }
}

gctBOOL
gcoVGHARDWARE_IsImageSupported(
    IN gceSURF_FORMAT Format
    )
{
    switch (Format)
    {
    /* Luminance formats. */
    case gcvSURF_L1:
    case gcvSURF_L8:

    /* Alpha formats. */
    case gcvSURF_A1:
    case gcvSURF_A4:
    case gcvSURF_A8:

    /* 16-bit A4R4G4B4 formats. */
    case gcvSURF_A4R4G4B4:
    case gcvSURF_A4B4G4R4:
    case gcvSURF_R4G4B4A4:
    case gcvSURF_B4G4R4A4:

    /* 16-bit A1R5G5B5 formats. */
    case gcvSURF_A1R5G5B5:
    case gcvSURF_A1B5G5R5:
    case gcvSURF_R5G5B5A1:
    case gcvSURF_B5G5R5A1:

    /* 16-bit R5G6B5 formats. */
    case gcvSURF_R5G6B5:
    case gcvSURF_B5G6R5:

    /* 32-bit X8R8G8B8 formats. */
    case gcvSURF_X8R8G8B8:
    case gcvSURF_X8B8G8R8:
    case gcvSURF_R8G8B8X8:
    case gcvSURF_B8G8R8X8:

    /* 32-bit A8R8G8B8 formats. */
    case gcvSURF_A8R8G8B8:
    case gcvSURF_A8B8G8R8:
    case gcvSURF_R8G8B8A8:
    case gcvSURF_B8G8R8A8:

        /* Supported. */
        return gcvTRUE;

    default:
        /* Not supported. */
        return gcvFALSE;
    }
}


/******************************************************************************\
** gcoVGHARDWARE: Path API functions. ********************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_QueryPathStorage(
    IN gcoVGHARDWARE Hardware,
    OUT gcsPATH_BUFFER_INFO_PTR PathInfo
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Hardware=0x%x PathInfo=0x%x",
                  Hardware, PathInfo);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* FE DATA command. */
    PathInfo->reservedForHead = Hardware->bufferInfo.commandAlignment;

    if (Hardware->vg20)
    {
        /* FE STATE command. */
        PathInfo->reservedForHead += Hardware->bufferInfo.commandAlignment;
    }

    /* Reserve space for the END/BREAK path commands. */
    PathInfo->reservedForTail = gcmSIZEOF(gctUINT8);

    gcmFOOTER();
    /* Success. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_AssociateCompletion(
    IN gcoVGHARDWARE Hardware,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x CommandBuffer=0x%x",
                  Hardware, CommandBuffer);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    /* Relay the call. */
    status = gcoVGBUFFER_AssociateCompletion(
        Hardware->buffer, CommandBuffer
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_DeassociateCompletion(
    IN gcoVGHARDWARE Hardware,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x CommandBuffer=0x%x",
                  Hardware, CommandBuffer);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    /* Relay the call. */
    status = gcoVGBUFFER_DeassociateCompletion(
        Hardware->buffer, CommandBuffer
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_CheckCompletion(
    IN gcoVGHARDWARE Hardware,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x CommandBuffer=0x%x",
                  Hardware, CommandBuffer);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    /* Relay the call. */
    status = gcoVGBUFFER_CheckCompletion(
        Hardware->buffer, CommandBuffer
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_WaitCompletion(
    IN gcoVGHARDWARE Hardware,
    IN gcsCMDBUFFER_PTR CommandBuffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x CommandBuffer=0x%x",
                  Hardware, CommandBuffer);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(CommandBuffer != gcvNULL);

    /* Relay the call. */
    status = gcoVGBUFFER_WaitCompletion(
        Hardware->buffer, CommandBuffer
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: VG Target API functions. ***************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_SetVgTarget(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Target
    )
{
    gceSTATUS status;

    do
    {
        gctUINT32 format;
        gctUINT32 swizzle;
        gceORIENTATION orientation;
        gcePOOL pool;
        gctUINT32 offset;

        gcmGETVGHARDWARE(Hardware);

        /* Same target being set again? */
        if (Hardware->vg.target == Target)
        {
            /* The target did not change. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Is there a target currently set? */
        if (Hardware->vg.target != gcvNULL)
        {
            /* Unlock the current target. */
            gcmERR_BREAK(gcoVGHARDWARE_Unlock(
                Hardware,
                &Hardware->vg.target->node,
                 Hardware->vg.target->type
                ));

            /* Reset the target. */
            Hardware->vg.target       = gcvNULL;
            Hardware->vg.targetWidth  = 0;
            Hardware->vg.targetHeight = 0;
        }

        /* Target reset? */
        if (Target == gcvNULL)
        {
            /* Done. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Is the target supported? */
        if (!gcoVGHARDWARE_IsTargetSupported(Target->format))
        {
            /* Format not supported. */
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* Lock the target. */
        gcmERR_BREAK(gcoVGHARDWARE_Lock(
            Hardware, &Target->node, gcvNULL, gcvNULL
            ));

        /* Translate the surface format. */
        gcmERR_BREAK(_GetVgFormat(
            Target, &format, &swizzle
            ));

        /* Determine target orientation. */
        orientation = (Target->orientation == gcvORIENTATION_TOP_BOTTOM)
            ? 0x0
            : 0x1;

        gcmERR_BREAK(gcoVGHARDWARE_SplitAddress(
            Hardware,
            Target->node.physical,
            &pool, &offset));

        /* Load the target base address. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02844 >> 2,
            (pool == gcvPOOL_VIRTUAL) ?
            Target->node.physical : gcmFIXADDRESS(Target->node.physical)
            ));

        /* Load the target stride. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02848 >> 2,
            Target->stride
            ));

        /* Load the target bounding box. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x0284C >> 2,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (Target->rect.right) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (Target->rect.bottom) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)))
            ));

        /* Set the format and swizzle. */
        Hardware->vg.targetControl = ((((gctUINT32) (Hardware->vg.targetControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0))) | (((gctUINT32) ((gctUINT32) (format ) & ((gctUINT32) ((((1 ? 2:0) - (0 ? 2:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:0) - (0 ? 2:0) + 1))))))) << (0 ? 2:0)));

        Hardware->vg.targetControl = ((((gctUINT32) (Hardware->vg.targetControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4))) | (((gctUINT32) ((gctUINT32) (swizzle ) & ((gctUINT32) ((((1 ? 5:4) - (0 ? 5:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:4) - (0 ? 5:4) + 1))))))) << (0 ? 5:4)));

        /* Set orinentation. */
        Hardware->vg.targetControl = ((((gctUINT32) (Hardware->vg.targetControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (orientation ) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));

        /* Determine the premultiplied field. */
        Hardware->vg.targetPremultiplied
            = (Target->colorType & gcvSURF_COLOR_ALPHA_PRE) != 0;

        Hardware->vg.targetLinear
            = (Target->colorType & gcvSURF_COLOR_LINEAR) != 0;

        /* Set the target surface. */
        Hardware->vg.target = Target;

        /* Set the target size. */
        Hardware->vg.targetWidth  = Target->rect.right;
        Hardware->vg.targetHeight = Target->rect.bottom;

#if gcvREPORT_TARGET
        gcmTRACE(
            gcvLEVEL_ERROR, "%s: target=0x%08X\n",
            __FUNCTION__, Target->node.logical
            );
#endif

        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: VG Drawing API functions. **************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_DrawVgRect(
    IN gcoVGHARDWARE Hardware,
    IN gctINT X,
    IN gctINT Y,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x X=0x%x  Y=0x%x  Width=0x%x  Height=0x%x",
                  Hardware, X, Y, Width, Height);
    gcmGETVGHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctUINT32_PTR memory;
        gcsVG_HWRECT_PTR rect;

        /* Flush PE cache if required. */
        if (Hardware->vg.peFlushNeeded)
        {
            gcmERR_BREAK(gcoVGHARDWARE_FlushPipe(
                Hardware
                ));

            Hardware->vg.peFlushNeeded = gcvFALSE;
        }

        /* Disable fill path. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableTessellation(
            Hardware, gcvFALSE
            ));

        /* Set target control. */
        gcmERR_BREAK(gcoVGHARDWARE_ProgramControl(
            Hardware, gcvNULL, gcvNULL
            ));

        /* Reserve space in the command buffer. */
        gcmERR_BREAK(gcoVGBUFFER_Reserve(
            Hardware->buffer,
            sizeof(gctUINT32) * 2 + sizeof(gcsVG_HWRECT),
            gcvTRUE,
            (gctPOINTER *) &memory
            ));

        /* Add a data command. */
        *memory++
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x4 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

        *memory++
            = 0;

        /* Set the rectangle pointer. */
        rect = (gcsVG_HWRECT_PTR) memory;

        /* Set the rectangle. */
        rect->x      = X;
        rect->y      = Y;
        rect->width  = Width;
        rect->height = Height;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_VgClear(
    IN gcoVGHARDWARE Hardware,
    IN gctINT X,
    IN gctINT Y,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x X=0x%x  Y=0x%x  Width=0x%x  Height=0x%x",
                  Hardware, X, Y, Width, Height);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Enable paint/image transform premultiplication if target is premultiplied. */
        if (Hardware->vg.targetPremultiplied)
        {
            /* Premultiply transformed color. */
            Hardware->vg.colorPremultiply = 0x0;

            /* Target is premultiplied - disable demultiplier. */
            Hardware->vg.targetPremultiply = 0x1;
        }
        else
        {
            /* Do not mul/demul colors. */
            Hardware->vg.colorPremultiply  = 0x1;
            Hardware->vg.targetPremultiply = 0x1;
        }

        /* Set rectangle primitive. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
            Hardware, gcvVG_RECTANGLE
            ));

        /* Render the rectangle. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
            Hardware, X, Y, Width, Height
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_DrawImage(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Image,
    IN gcsVG_RECT_PTR SrcRect,
    IN gcsVG_RECT_PTR TrgRect,
    IN gceIMAGE_FILTER Filter,
    IN gctBOOL Mask
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x Image=0x%x  SrcRect=0x%x  TrgRect=0x%x  Filter=0x%x Mask=0x%x",
                  Hardware, Image, SrcRect, TrgRect, Filter, Mask);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /***********************************************************************
        ** Define target-to-source coordinate translation.
        */

        gctINT srcY;

        gctFLOAT imageStepX[3];
        gctFLOAT imageStepY[3];
        gctFLOAT imageConst[3];

        /* srcX = (trgX - trgXorigin + 0.5) / trgWidth */
        imageStepX[0] = 1.0f / TrgRect->width;
        imageStepY[0] = 0.0f;
        imageConst[0] = (gctFLOAT) (0.5f - TrgRect->x) / TrgRect->width;

        /* srcW = 1 (no perspective correction needed). */
        imageStepX[2] = 0.0f;
        imageStepY[2] = 0.0f;
        imageConst[2] = 1.0f;

        if (Image->orientation == gcvORIENTATION_TOP_BOTTOM)
        {
            /* srcY = (trgHeight - (trgY - trgYorigin + 0.5) - 1) / trgHeight
                    = (-trgY + (trgHeight + trgYorigin - 0.5)) / trgHeight */
            imageStepX[1] =  0.0f;
            imageStepY[1] = -1.0f / TrgRect->height;
            imageConst[1] = (gctFLOAT) (TrgRect->height + TrgRect->y - 0.5f)
                          / TrgRect->height;

            /* Invert the source origin. */
            srcY = Image->rect.bottom - SrcRect->y - SrcRect->height;
        }
        else
        {
            /* srcY = (trgY - trgYorigin + 0.5) / trgHeight */
            imageStepX[1] = 0.0f;
            imageStepY[1] = 1.0f / TrgRect->height;
            imageConst[1] = (gctFLOAT) (0.5f - TrgRect->y) / TrgRect->height;

            /* Source origin remains as specified. */
            srcY = SrcRect->y;
        }


        /***********************************************************************
        ** Set image parameters.
        */

        gcmERR_BREAK(gcoVGHARDWARE_SetStates(
            Hardware,
            0x02870 >> 2,
            gcmCOUNTOF(imageStepX), imageStepX
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetStates(
            Hardware,
            0x02880 >> 2,
            gcmCOUNTOF(imageStepY), imageStepY
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetStates(
            Hardware,
            0x02860 >> 2,
            gcmCOUNTOF(imageConst), imageConst
            ));

        /***********************************************************************
        ** Determine whether the pattern is linear.
        */

        Hardware->vg.imageLinear
            = (Image->colorType & gcvSURF_COLOR_LINEAR) != 0;


        /***********************************************************************
        ** Setup image sampler.
        */

        gcmERR_BREAK(_SetSampler(
            Hardware,
            1, /* Use the second sampler for images. */
            Image,
            gcvTILE_PAD, /* Tile mode.                         */
            Mask, /* Mask vs. image selection.          */
            Filter, /* Image quailty filter mode.         */
            gcvFALSE, /* Not in image filter mode.          */
            SrcRect->x,
            srcY,
            SrcRect->width,
            SrcRect->height
            ));

        /* Enable paint/image transform premultiplication if... */
        if (

            /* ...target is premultiplied. */
            Hardware->vg.targetPremultiplied

            /* ...paint/image output is not premultiplied. */
            || (Image->colorType & gcvSURF_COLOR_ALPHA_PRE)

            /* ...image filter is enabled. */
            || (Filter != gcvFILTER_POINT)

            /* ...blending is enabled. */
            || ((Hardware->vg.blendMode != gcvVG_BLEND_SRC) &&
                (Hardware->vg.blendMode != gcvVG_BLEND_FILTER))

            /* ...color transform is ebabled. */
            || Hardware->vg.colorTransform
            )
        {
            /* Premultiply transformed color if not in image filter mode. */
            Hardware->vg.colorPremultiply = 0x0;

            /* Demultiply the color if target is not premultiplied. */
            Hardware->vg.targetPremultiply = Hardware->vg.targetPremultiplied
                ? 0x1
                : 0x0;
        }
        else
        {
            /* Do not mul/demul colors. */
            Hardware->vg.colorPremultiply  = 0x1;
            Hardware->vg.targetPremultiply = 0x1;
        }


        /***********************************************************************
        ** Draw image.
        */

        /* Set rectangle primitive. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
            Hardware,
            gcvVG_RECTANGLE
            ));

        /* Render the rectangle. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
            Hardware,
            TrgRect->x,
            TrgRect->y,
            TrgRect->width,
            TrgRect->height
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVGHARDWARE_TesselateImage(
    IN gcoVGHARDWARE Hardware,
    IN gctBOOL SoftwareTesselation,
    IN gcsSURF_INFO_PTR Image,
    IN gcsVG_RECT_PTR Rectangle,
    IN gceIMAGE_FILTER Filter,
    IN gctBOOL Mask,
    IN gctFLOAT UserToSurface[9],
    IN gctFLOAT SurfaceToImage[9],
    IN gcsTESSELATION_PTR TessellationBuffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x",
                  Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
#       define gcvPATH_DATA_SIZE \
        (\
            /* Move to point 0. */ \
            gcmSIZEOF(gctUINT32) + gcmSIZEOF(gctFLOAT) + gcmSIZEOF(gctFLOAT) + \
            \
            /* Line to point 1. */ \
            gcmSIZEOF(gctUINT32) + gcmSIZEOF(gctFLOAT) + gcmSIZEOF(gctFLOAT) + \
            \
            /* Line to point 2. */ \
            gcmSIZEOF(gctUINT32) + gcmSIZEOF(gctFLOAT) + gcmSIZEOF(gctFLOAT) + \
            \
            /* Line to point 3. */ \
            gcmSIZEOF(gctUINT32) + gcmSIZEOF(gctFLOAT) + gcmSIZEOF(gctFLOAT) + \
            \
            /* Finished. */ \
            gcmSIZEOF(gctUINT32) \
        )

#       define gcvPATH_DATA_COUNT \
        (\
            /* Align to 64-bits. */ \
            gcmALIGN(gcvPATH_DATA_SIZE, gcmSIZEOF(gctUINT64)) \
            \
            /* Divide by the size of the chunk. */ \
            / gcmSIZEOF(gctUINT64) \
        )

#       define gcvPATH_DATA_BUFFER_SIZE \
        (\
            /* Path data header. */ \
            gcmSIZEOF(gcsPATH_DATA) + \
            \
            /* DATA command. */ \
            gcmSIZEOF(gctUINT64) + \
            \
            /* Aligned path data. */ \
            gcmALIGN(gcvPATH_DATA_SIZE, gcmSIZEOF(gctUINT64)) + \
            \
            /* END command. */ \
            gcmSIZEOF(gctUINT64) \
        )

#       define gcvPATH_DATA_BUFFER_END_OFFSET \
        (\
            /* Path data header. */ \
            gcmSIZEOF(gcsPATH_DATA) + \
            \
            /* DATA command. */ \
            gcmSIZEOF(gctUINT64) + \
            \
            /* Aligned path data. */ \
            gcmALIGN(gcvPATH_DATA_SIZE, gcmSIZEOF(gctUINT64)) \
        )

        gctUINT8_PTR path;
        gctSIZE_T dataCommandSize;

        float widthProduct[3];
        float heightProduct[3];

        float point0[3];
        float point1[3];
        float point2[3];
        float point3[3];

        float imageStepX[3];
        float imageStepY[3];
        float imageConst[3];

        gctUINT8 pathDataBuffer[gcvPATH_DATA_BUFFER_SIZE];
        gcsPATH_DATA_PTR pathData = (gcsPATH_DATA_PTR) pathDataBuffer;


        /***********************************************************************
        ** Get shortcuts to the size of the image.
        */

        gctINT width  = Rectangle->width;
        gctINT height = Rectangle->height;


        /***********************************************************************
        ** Transform image corners into the surface space.
        */

        widthProduct[0] = gcmMAT(UserToSurface, 0, 0) * width;
        widthProduct[1] = gcmMAT(UserToSurface, 1, 0) * width;
        widthProduct[2] = gcmMAT(UserToSurface, 2, 0) * width;

        heightProduct[0] = gcmMAT(UserToSurface, 0, 1) * height;
        heightProduct[1] = gcmMAT(UserToSurface, 1, 1) * height;
        heightProduct[2] = gcmMAT(UserToSurface, 2, 1) * height;

        point0[0] = gcmMAT(UserToSurface, 0, 2);
        point0[1] = gcmMAT(UserToSurface, 1, 2);
        point0[2] = gcmMAT(UserToSurface, 2, 2);

        point1[0] = heightProduct[0] + point0[0];
        point1[1] = heightProduct[1] + point0[1];
        point1[2] = heightProduct[2] + point0[2];

        point2[0] = widthProduct[0] + point1[0];
        point2[1] = widthProduct[1] + point1[1];
        point2[2] = widthProduct[2] + point1[2];

        point3[0] = widthProduct[0] + point0[0];
        point3[1] = widthProduct[1] + point0[1];
        point3[2] = widthProduct[2] + point0[2];

        if ((point0[2] <= 0.0f) || (point1[2] <= 0.0f) ||
            (point2[2] <= 0.0f) || (point3[2] <= 0.0f))
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Projection. */
        point0[0] /= point0[2];
        point0[1] /= point0[2];
        point0[2]  = 1.0f;

        point1[0] /= point1[2];
        point1[1] /= point1[2];
        point1[2]  = 1.0f;

        point2[0] /= point2[2];
        point2[1] /= point2[2];
        point2[2]  = 1.0f;

        point3[0] /= point3[2];
        point3[1] /= point3[2];
        point3[2]  = 1.0f;

#define NEG_MOST    (float)(-(2^19))
#define POS_MOST    (float)((2^19) - 1)
        if ((point0[0] > POS_MOST) || (point0[0] < NEG_MOST) ||
            (point0[1] > POS_MOST) || (point0[1] < NEG_MOST) ||
            (point1[0] > POS_MOST) || (point1[0] < NEG_MOST) ||
            (point1[1] > POS_MOST) || (point1[1] < NEG_MOST) ||
            (point2[0] > POS_MOST) || (point2[0] < NEG_MOST) ||
            (point2[1] > POS_MOST) || (point2[1] < NEG_MOST) ||
            (point3[0] > POS_MOST) || (point3[0] < NEG_MOST) ||
            (point3[1] > POS_MOST) || (point3[1] < NEG_MOST))
        {
            status = gcvSTATUS_DATA_TOO_LARGE;
            break;
        }
#undef NEG_MOST
#undef POS_MOST

        /***********************************************************************
        ** Transform image parameters.
        */

        imageStepX[0] = gcmMAT(SurfaceToImage, 0, 0) / width;
        imageStepX[1] = gcmMAT(SurfaceToImage, 1, 0) / height;
        imageStepX[2] = gcmMAT(SurfaceToImage, 2, 0);

        imageStepY[0] = gcmMAT(SurfaceToImage, 0, 1) / width;
        imageStepY[1] = gcmMAT(SurfaceToImage, 1, 1) / height;
        imageStepY[2] = gcmMAT(SurfaceToImage, 2, 1);

        imageConst[0] =
            (
                0.5f
                    * (gcmMAT(SurfaceToImage, 0, 0) + gcmMAT(SurfaceToImage, 0, 1) )
                    +   gcmMAT(SurfaceToImage, 0, 2)
            )
            / width;

        imageConst[1] =
            (
                0.5f
                    * (gcmMAT(SurfaceToImage, 1, 0) + gcmMAT(SurfaceToImage, 1, 1) )
                    +   gcmMAT(SurfaceToImage, 1, 2)
            )
            / height;

        imageConst[2] =
            (
                0.5f
                    * (gcmMAT(SurfaceToImage, 2, 0) + gcmMAT(SurfaceToImage, 2, 1) )
                    +   gcmMAT(SurfaceToImage, 2, 2)
            );


        /***********************************************************************
        ** Determine whether the pattern is linear.
        */

        Hardware->vg.imageLinear
            = (Image->colorType & gcvSURF_COLOR_LINEAR) != 0;


        /***********************************************************************
        ** Set image parameters.
        */

        gcmERR_BREAK(gcoVGHARDWARE_SetStates(
            Hardware,
            0x02870 >> 2,
            gcmCOUNTOF(imageStepX), imageStepX
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetStates(
            Hardware,
            0x02880 >> 2,
            gcmCOUNTOF(imageStepY), imageStepY
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetStates(
            Hardware,
            0x02860 >> 2,
            gcmCOUNTOF(imageConst), imageConst
            ));


        /***********************************************************************
        ** Setup image sampler.
        */

        gcmERR_BREAK(_SetSampler(
            Hardware,
            1, /* Use the second sampler for images. */
            Image,
            gcvTILE_PAD, /* Tile mode.                         */
            Mask, /* Mask vs. image selection.          */
            Filter, /* Image quailty filter mode.         */
            gcvFALSE, /* Not in image filter mode.          */
            Rectangle->x,
            Rectangle->y,
            Rectangle->width,
            Rectangle->height
            ));

        /* Premultiply transformed color if not in image filter mode. */
        Hardware->vg.colorPremultiply = 0x0;

        /* Demultiply the color if target is not premultiplied. */
        Hardware->vg.targetPremultiply = Hardware->vg.targetPremultiplied
            ? 0x1
            : 0x0;


        /***********************************************************************
        ** Generate a path around the image.
        */

        /* Get the size of DATA command. */
        gcmERR_BREAK(gcoVGHARDWARE_DataCommand(
            Hardware,
            gcvNULL, 0,
            &dataCommandSize
            ));

        /* Initialize the path. */
        pathData->dataType           = gcePATHTYPE_FLOAT;
        pathData->data.address       = 0;
        pathData->data.bufferOffset  = gcmSIZEOF(gcsPATH_DATA);
        pathData->data.size          = dataCommandSize + gcvPATH_DATA_COUNT * 8;
        pathData->data.nextSubBuffer = gcvNULL;

        if (SoftwareTesselation)
        {
            gcmASSERT(dataCommandSize == 8);

            /* Set data offset. */
            path = pathDataBuffer + gcmSIZEOF(gcsPATH_DATA);

            /* Initialize the END command. */
            gcmERR_BREAK(gcoVGHARDWARE_EndCommand(
                Hardware,
                pathDataBuffer + gcvPATH_DATA_BUFFER_END_OFFSET, 0,
                gcvNULL
                ));
        }
        else
        {
            /* Set data type. */
            gcmERR_BREAK(gcoVGHARDWARE_SetPathDataType(
                Hardware, gcePATHTYPE_FLOAT
                ));

            /* Draw the image. */
            gcmERR_BREAK(gcoVGHARDWARE_DrawPath(
                Hardware, gcvFALSE, pathData,
                TessellationBuffer, (gctPOINTER *) &path
                ));
        }

        /* Initialize the DATA command. */
        gcmERR_BREAK(gcoVGHARDWARE_DataCommand(
            Hardware,
            path, gcvPATH_DATA_COUNT,
            gcvNULL
            ));

        path += dataCommandSize;

        /* Move to point 0. */
        *                 path  = 0x02;     path += 4;
        * ((gctFLOAT_PTR) path) = point0[0];                path += 4;
        * ((gctFLOAT_PTR) path) = point0[1];                path += 4;

        /* Line to point 1. */
        *                 path  = 0x04;     path += 4;
        * ((gctFLOAT_PTR) path) = point1[0];                path += 4;
        * ((gctFLOAT_PTR) path) = point1[1];                path += 4;

        /* Line to point 2. */
        *                 path  = 0x04;     path += 4;
        * ((gctFLOAT_PTR) path) = point2[0];                path += 4;
        * ((gctFLOAT_PTR) path) = point2[1];                path += 4;

        /* Line to point 3. */
        *                 path  = 0x04;     path += 4;
        * ((gctFLOAT_PTR) path) = point3[0];                path += 4;
        * ((gctFLOAT_PTR) path) = point3[1];                path += 4;

        /* Finished. */
        *                 path  = 0x00;

        if (SoftwareTesselation)
        {
            gcsRECT boundingBox;

            /* Process tesselation buffer. */
            gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
                Hardware,
                SoftwareTesselation
                    ? gcvVG_TESSELLATED
                    : Hardware->vg.tesselationPrimitive
                ));

            /* Tesselate path. */
            gcmERR_BREAK(gcoVGHARDWARE_Tesselate(
                Hardware, pathData, &boundingBox
                ));

            /* Render the rectangle. */
            gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
                Hardware,
                boundingBox.left,
                boundingBox.top,
                boundingBox.right,
                boundingBox.bottom
                ));
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVGHARDWARE_VgBlit(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Target,
    IN gcsVG_RECT_PTR SrcRect,
    IN gcsVG_RECT_PTR TrgRect,
    IN gceIMAGE_FILTER Filter,
    IN gceVG_BLEND Mode
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctBOOL prevMaskEnable;
        gctBOOL prevScissorEnable;
        gctBOOL prevColorTransform;
        gceVG_IMAGE prevImageMode;
        gceVG_BLEND prevBlendMode;
        gcsVG_RECT tempRect;
        gctBOOL overlap;
        gcsSURF_INFO_PTR prevTarget;


        /***********************************************************************
        ** Save the current state.
        */

        prevMaskEnable     = Hardware->vg.maskEnable;
        prevScissorEnable  = Hardware->vg.scissorEnable;
        prevColorTransform = Hardware->vg.colorTransform;
        prevImageMode      = Hardware->vg.imageMode;
        prevBlendMode      = Hardware->vg.blendMode;
        prevTarget         = Hardware->vg.target;


        /***********************************************************************
        ** Configure VG pipe.
        */

        /* Disable masking. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableMask(
            Hardware, gcvFALSE
            ));

        /* Disable scissor. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableScissor(
            Hardware, gcvFALSE
            ));

        /* Disable color transformation. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableColorTransform(
            Hardware, gcvFALSE
            ));

        /* Set normal image mode. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgImageMode(
            Hardware, gcvVG_IMAGE_NORMAL
            ));


        /***********************************************************************
        ** Create a source copy in case if the source and destination overlap.
        */

        /* Determine whether the source and destination overlap. */
        overlap
            =  (Source == Target)
            && ((SrcRect->x < TrgRect->x)
                    ? (SrcRect->x + SrcRect->width  > TrgRect->x)
                    : (TrgRect->x + TrgRect->width  > SrcRect->x))
            && ((SrcRect->y < TrgRect->y)
                    ? (SrcRect->y + SrcRect->height > TrgRect->y)
                    : (TrgRect->y + TrgRect->height > SrcRect->y));

        /* Do we have an overlap? */
        if (overlap)
        {
            gcsSURF_FORMAT_INFO_PTR tempFormat[2];

            /* Get the format descriptor. */
            gcmERR_BREAK(gcoSURF_QueryFormat(
                Source->format, tempFormat
                ));

            /* Allocate the temporary surface. */
            gcmERR_BREAK(gcoVGHARDWARE_AllocateTemporarySurface(
                Hardware,
                SrcRect->width,
                SrcRect->height,
                tempFormat[0],
                gcvSURF_BITMAP
                ));

            /* Copy the attributes. */
            Hardware->tempBuffer.colorType   = Source->colorType;
            Hardware->tempBuffer.orientation = Source->orientation;

            /* Set the temporary rectangle. */
            tempRect.x      = 0;
            tempRect.y      = 0;
            tempRect.width  = SrcRect->width;
            tempRect.height = SrcRect->height;

            /* Set the target surface. */
            gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
                Hardware, &Hardware->tempBuffer
                ));

            /* Set blending mode. */
            gcmERR_BREAK(gcoVGHARDWARE_SetVgBlendMode(
                Hardware, gcvVG_BLEND_SRC
                ));

            /* Draw the image. */
            gcmERR_BREAK(gcoVGHARDWARE_DrawImage(
                Hardware,
                Source,
                SrcRect,
                &tempRect,
                gcvFILTER_POINT,
                gcvFALSE
                ));

            /* Flush PE so that imager in the next pass would be able
               to get the image data correctly. */
            gcmERR_BREAK(gcoVGHARDWARE_FlushPipe(
                Hardware
                ));

            /* Override the source. */
            Source  = &Hardware->tempBuffer;
            SrcRect = &tempRect;
        }


        /***********************************************************************
        ** Draw the destination image.
        */

        /* Set the target surface. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
            Hardware, Target
            ));

        /* Set blending mode. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgBlendMode(
            Hardware, Mode
            ));

        /* Draw the image. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawImage(
            Hardware,
            Source,
            SrcRect,
            TrgRect,
            Filter,
            gcvFALSE
            ));


        /***********************************************************************
        ** Restore the original state.
        */

        gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
                Hardware, prevTarget
                ));

        gcmERR_BREAK(gcoVGHARDWARE_EnableMask(
            Hardware, prevMaskEnable
            ));

        gcmERR_BREAK(gcoVGHARDWARE_EnableScissor(
            Hardware, prevScissorEnable
            ));

        gcmERR_BREAK(gcoVGHARDWARE_EnableColorTransform(
            Hardware, prevColorTransform
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetVgImageMode(
            Hardware, prevImageMode
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetVgBlendMode(
            Hardware, prevBlendMode
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoVGHARDWARE_DrawPath(
    IN gcoVGHARDWARE Hardware,
    IN gctBOOL SoftwareTesselation,
    IN gcsPATH_DATA_PTR PathData,
    IN gcsTESSELATION_PTR TessellationBuffer,
    OUT gctPOINTER * Path
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(PathData != gcvNULL);

    do
    {
        /* Process tesselation buffer. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
            Hardware,
            SoftwareTesselation
                ? gcvVG_TESSELLATED
                : Hardware->vg.tesselationPrimitive
            ));

        /* Using software tesselation? */
        if (SoftwareTesselation)
        {
            gcsRECT boundingBox;

            /* Tesselate path. */
            gcmERR_BREAK(gcoVGHARDWARE_Tesselate(
                Hardware, PathData, &boundingBox
                ));

            /* Render the rectangle. */
            gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
                Hardware,
                boundingBox.left,
                boundingBox.top,
                boundingBox.right,
                boundingBox.bottom
                ));
        }
        else
        {
            if (Hardware->vg21 && Hardware->vgRestart)
            {
                gctSIZE_T stallSize;
                gctSIZE_T beginSize;
                gctSIZE_T controlSize;
                gctSIZE_T appendSize;
                gctSIZE_T endSize;
                gctSIZE_T semaphoreSize;
                gctSIZE_T totalSize = 0;
                gctUINT32 currCmdAddress;
                gctUINT32 prevCmdAddress;
                gctUINT32 restartAddress;
                gctUINT32 restartAlignment = 0;
                gctUINT i, nopCount;
                gctUINT8_PTR memory;

                /* Query the sizes. */
                gcmERR_BREAK(_SendStall(
                    Hardware, gcvNULL, &stallSize
                    ));

                gcmERR_BREAK(gcoVGBUFFER_MarkRestart(
                    Hardware->buffer, gcvNULL, gcvTRUE, &beginSize
                    ));

                gcmERR_BREAK(gcoVGHARDWARE_ProgramControl(
                    Hardware, gcvNULL, &controlSize
                    ));

                gcmERR_BREAK(gcoVGBUFFER_MarkRestart(
                    Hardware->buffer, gcvNULL, gcvFALSE, &endSize
                    ));

                gcmERR_BREAK(_SendSemaphore(
                    Hardware, gcvNULL, &semaphoreSize
                    ));

                /* Path gets directly set into the command buffer? */
                if (PathData->data.address == 0)
                {
                    appendSize = PathData->data.size;
                }
                else
                {
                    gcmERR_BREAK(gcoVGBUFFER_AppendBuffer(
                        Hardware->buffer, gcvNULL, &PathData->data, &appendSize
                        ));
                }

                /* Get the current address inside the command buffer. */
                gcmERR_BREAK(gcoVGBUFFER_GetCurrentAddress(
                    Hardware->buffer, gcvTRUE, &currCmdAddress
                    ));

                for (i = 0; i < 2; i += 1)
                {
                    /* Compute the address of RESTART. */
                    restartAddress
                        = currCmdAddress
                        + stallSize;

                    /* Compute the 64-byte aligned address. */
                    restartAlignment
                        = gcmALIGN(restartAddress, 64)
                        - restartAddress;

                    /* Determine the block size. */
                    totalSize
                        =  stallSize
                        +  restartAlignment
                        +  beginSize
                        +  controlSize
                        + (Hardware->bufferInfo.stateCommandSize + gcmSIZEOF(gctUINT32)) * 2
                        +  appendSize
                        +  endSize
                        +  semaphoreSize;

                    /* Current becomes previous. */
                    prevCmdAddress = currCmdAddress;

                    /* Make sure there is enough space in the current buffer. */
                    gcmERR_BREAK(gcoVGBUFFER_EnsureSpace(
                        Hardware->buffer, totalSize, gcvTRUE
                        ));

                    /* Get the current address inside the command buffer. */
                    gcmERR_BREAK(gcoVGBUFFER_GetCurrentAddress(
                        Hardware->buffer, gcvTRUE, &currCmdAddress
                        ));

                    /* Are we at the same place in the buffer? */
                    if (prevCmdAddress == currCmdAddress)
                    {
                        break;
                    }
                }

                /* Two loops should always be enough. */
                if (i == 2)
                {
                    status = gcvSTATUS_GENERIC_IO;
                    break;
                }

                /* Reserve space in the command buffer. */
                gcmERR_BREAK(gcoVGBUFFER_Reserve(
                    Hardware->buffer,
                    totalSize,
                    gcvTRUE,
                    (gctPOINTER *) &memory
                    ));

                /* Schedule a stall. */
                gcmERR_BREAK(_SendStall(
                    Hardware, memory, gcvNULL
                    ));

                memory += stallSize;

                /* Determine the number of NOP commands. */
                nopCount
                    = restartAlignment
                    / Hardware->bufferInfo.commandAlignment;

                /* Insert NOPs for alignment. */
                for (i = 0; i < nopCount; i += 1)
                {
                    gcmERR_BREAK(gcoVGHARDWARE_NopCommand(
                        Hardware, memory, gcvNULL
                        ));

                    memory += Hardware->bufferInfo.commandAlignment;
                }

                /* Mark the beginning of the restart area. */
                gcmERR_BREAK(gcoVGBUFFER_MarkRestart(
                    Hardware->buffer, memory, gcvTRUE, gcvNULL
                    ));

                memory += beginSize;

                /* Set control states. */
                gcmERR_BREAK(gcoVGHARDWARE_ProgramControl(
                    Hardware, memory, gcvNULL
                    ));

                memory += controlSize;

                /* Flush tesselation cache. */
                gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
                    Hardware,
                    memory, 0x0286C >> 2, 1,
                    gcvNULL
                    ));

                memory += Hardware->bufferInfo.stateCommandSize;

                * ((gctUINT32_PTR) memory) = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1))))))) << (0 ? 12:12)));

                memory += gcmSIZEOF(gctUINT32);

                /* Clear TS buffer. */
                gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
                    Hardware,
                    memory, 0x028F4 >> 2, 1,
                    gcvNULL
                    ));

                memory += Hardware->bufferInfo.stateCommandSize;

                * ((gctUINT32_PTR) memory) = TessellationBuffer->clearSize >> 6;

                memory += gcmSIZEOF(gctUINT32);

                /* Path gets directly set into the command buffer? */
                if (PathData->data.address == 0)
                {
                    /* Path pointer must be provided. */
                    if (Path == gcvNULL)
                    {
                        status = gcvSTATUS_NOT_SUPPORTED;
                        break;
                    }
                    else
                    {
                        * Path = memory;
                    }
                }
                else
                {
                    /* Append to the command queue. */
                    gcmERR_BREAK(gcoVGBUFFER_AppendBuffer(
                        Hardware->buffer, memory, &PathData->data, gcvNULL
                        ));
                }

                memory += appendSize;

                /* Mark the end of the restart area. */
                gcmERR_BREAK(gcoVGBUFFER_MarkRestart(
                    Hardware->buffer, memory, gcvFALSE, gcvNULL
                    ));

                memory += endSize;

                /* Schedule a semaphore. */
                gcmERR_BREAK(_SendSemaphore(
                    Hardware, memory, gcvNULL
                    ));

                memory += semaphoreSize;
            }
            else
            {
                /* Schedule a stall. */
                gcmERR_BREAK(_SendStall(
                    Hardware, gcvNULL, gcvNULL
                    ));

                /* Set control states. */
                gcmERR_BREAK(gcoVGHARDWARE_ProgramControl(
                    Hardware, gcvNULL, gcvNULL
                    ));

                /* Clear TS buffer. */
                gcmERR_BREAK(gcoVGHARDWARE_SetState(
                    Hardware,
                    0x028F4 >> 2,
                    TessellationBuffer->clearSize >> 6
                    ));

                /* Path gets directly set into the command buffer? */
                if (PathData->data.address == 0)
                {
                    if (Path == gcvNULL)
                    {
                        status = gcvSTATUS_NOT_SUPPORTED;
                        break;
                    }
                    else
                    {
                        /* Reserve command buffer space for the path. */
                        gcmERR_BREAK(gcoVGBUFFER_Reserve(
                            Hardware->buffer,
                            PathData->data.size,
                            gcvTRUE,
                            Path
                            ));
                    }
                }
                else
                {
                    /* Append to the command queue. */
                    gcmERR_BREAK(gcoVGBUFFER_AppendBuffer(
                        Hardware->buffer, gcvNULL, &PathData->data, gcvNULL
                        ));
                }

                /* Schedule a semaphore. */
                gcmERR_BREAK(_SendSemaphore(Hardware, gcvNULL, gcvNULL));
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Tesselator Control API functions. ******************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_SetRenderingQuality(
    IN gcoVGHARDWARE Hardware,
    IN gceRENDER_QUALITY Quality
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x0, /* gcvVG_NONANTIALIASED */
        0x1, /* gcvVG_2X2_MSAA       */
        0x2, /* gcvVG_2X4_MSAA       */
        0x3        /* gcvVG_4X4_MSAA       */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Quality, _value));

    /* Store rendering quality value. */
    Hardware->vg.tsQuality = _value[Quality];

    /* Success. */
    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoVGHARDWARE_SetPathDataType(
    IN gcoVGHARDWARE Hardware,
    IN gcePATHTYPE Type
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    /* Data type translation table. */
    static const gctUINT32 _value[] =
    {
        0x0, /* gcePATHTYPE_INT8  */
        0x1, /* gcePATHTYPE_INT16 */
        0x2, /* gcePATHTYPE_INT32 */
        0x3     /* gcePATHTYPE_FLOAT */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Type, _value));

    /* Store data type value. */
    Hardware->vg.tsDataType = _value[Type];

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoVGHARDWARE_SetFillRule(
    IN gcoVGHARDWARE Hardware,
    IN gceFILL_RULE FillRule
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x1, /* gcvVG_EVEN_ODD */
        0x0, /* gcvVG_NON_ZERO */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(FillRule, _value));

    /* Store fill rule value. */
    Hardware->vg.tsFillRule = _value[FillRule];

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoVGHARDWARE_EnableTessellation(
    IN gcoVGHARDWARE Hardware,
    IN gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x0, /* gcvFALSE */
        0x1                      /* gcvTRUE  */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Enable, _value));

    /* Store tesselation mode value. */
    Hardware->vg.tsMode = _value[Enable];

    gcmFOOTER_NO();
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: VG Control API functions. **************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_SetPrimitiveMode(
    gcoVGHARDWARE Hardware,
    gceVG_PRIMITIVE Mode
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x0, /* gcvVG_SCANLINE          */
        0x1, /* gcvVG_RECTANGLE         */
        0x2, /* gcvVG_TESSELLATED       */
        0x3 /* gcvVG_TESSELLATED_TILED */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Mode, _value));

    /* Store the primitive mode. */
    Hardware->vg.primitiveMode = Mode;

    /* Set scissoring value. */
    Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (_value[Mode] ) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoVGHARDWARE_EnableScissor(
    gcoVGHARDWARE Hardware,
    gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x0, /* gcvFALSE */
        0x1                 /* gcvTRUE  */
    };
    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Enable, _value));

    /* Store scissor enable. */
    Hardware->vg.scissorEnable = Enable;

    /* Set scissoring value. */
    Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) ((gctUINT32) (_value[Enable] ) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)));

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoVGHARDWARE_EnableColorTransform(
    gcoVGHARDWARE Hardware,
    gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x0, /* gcvFALSE */
        0x1         /* gcvTRUE  */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Enable, _value));

    /* Store the value in the context. */
    Hardware->vg.colorTransform = Enable;

    /* Set color transform value. */
    Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (_value[Enable] ) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)));

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoVGHARDWARE_SetVgBlendMode(
    gcoVGHARDWARE Hardware,
    gceVG_BLEND Mode
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x0, /* gcvVG_BLEND_SRC      */
        0x1, /* gcvVG_BLEND_SRC_OVER */
        0x2, /* gcvVG_BLEND_DST_OVER */
        0x3, /* gcvVG_BLEND_SRC_IN   */
        0x4, /* gcvVG_BLEND_DST_IN   */
        0x5, /* gcvVG_BLEND_MULTIPLY */
        0x6, /* gcvVG_BLEND_SCREEN   */
        0x7, /* gcvVG_BLEND_DARKEN   */
        0x8, /* gcvVG_BLEND_LIGHTEN  */
        0x9, /* gcvVG_BLEND_ADDITIVE */
        0xA, /* gcvVG_BLEND_SUBTRACT */
        0xB          /* gcvVG_BLEND_FILTER   */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Mode, _value));

    /* gcvVG_BLEND_SRC blending mode does not require and does not force
       the hardware to read the destinaton. When we switch from gcvVG_BLEND_SRC
       to any other mode that will require destination, we need to flush the
       PE cache to avoid using invalid data that might still be in the cache.
       gcvVG_BLEND_FILTER also does not use destination, hence does not reuqire
       a flush. */
    Hardware->vg.peFlushNeeded
        |= (Hardware->vg.blendMode != Mode)
        &&
        (
            (Hardware->vg.blendMode == gcvVG_BLEND_SRC) ||
            (Hardware->vg.blendMode == gcvVG_BLEND_FILTER)
        );

    /* Set the mode. */
    Hardware->vg.blendMode = Mode;

    /* Convert HAL blend mode into HW blend mode. */
    Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8))) | (((gctUINT32) ((gctUINT32) (_value[Mode] ) & ((gctUINT32) ((((1 ? 11:8) - (0 ? 11:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:8) - (0 ? 11:8) + 1))))))) << (0 ? 11:8)));

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoVGHARDWARE_SetVgImageMode(
    gcoVGHARDWARE Hardware,
    gceVG_IMAGE Mode
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x0, /* gcvVG_IMAGE_NONE     */
        0x1, /* gcvVG_IMAGE_NORMAL   */
        0x2, /* gcvVG_IMAGE_MULTIPLY */
        0x3, /* gcvVG_IMAGE_STENCIL  */
        0x4          /* gcvVG_IMAGE_FILTER   */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Mode, _value));

    /* Enable paint transform premultiplication when image is disabled. */
    if (Mode == gcvVG_IMAGE_NONE)
    {
        Hardware->vg.colorPremultiply  = 0x0;
        Hardware->vg.targetPremultiply = Hardware->vg.targetPremultiplied
            ? 0x1
            : 0x0;
    }

    /* Store the mode. */
    Hardware->vg.imageMode = Mode;

    /* Convert HAL image mode into HW image mode. */
    Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12))) | (((gctUINT32) ((gctUINT32) (_value[Mode] ) & ((gctUINT32) ((((1 ? 14:12) - (0 ? 14:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:12) - (0 ? 14:12) + 1))))))) << (0 ? 14:12)));

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoVGHARDWARE_EnableMask(
    gcoVGHARDWARE Hardware,
    gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    static const gctUINT32 _value[] =
    {
        0x0, /* gcvFALSE */
        0x1                 /* gcvTRUE  */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Enable, _value));

    /* Store the enable value. */
    Hardware->vg.maskEnable = Enable;

    /* Set scissoring value. */
    Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20))) | (((gctUINT32) ((gctUINT32) (_value[Enable] ) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)));

    gcmFOOTER_NO();
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Mask API functions. ********************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_FlushVgMask(
    gcoVGHARDWARE Hardware
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Invalidate mask cache. */
    status = gcoVGHARDWARE_SetState(
        Hardware,
        0x0286C >> 2,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Scissor API functions. *****************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_SetScissor(
    gcoVGHARDWARE Hardware,
    gctUINT32 Address,
    gctINT Stride
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Program scissor address. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02858 >> 2,
            gcmFIXADDRESS(Address)
            ));

        /* Program scissor stride. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x0285C >> 2,
            Stride
            ));

        /* Flush scissor cache. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x0286C >> 2,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
            ));

        /* Success. */
        gcmFOOTER();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the error. */
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Color Transformation API functions. ****************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_SetColorTransform(
    IN gcoVGHARDWARE Hardware,
    IN gctFLOAT Scale[4],
    IN gctFLOAT Offset[4]
    )
{
    gctINT i;
    gctUINT32 states[4];
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Compute color transform. */
    for (i = 0; i < 4; ++i)
    {
        states[i]
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) ((gctINT) (Scale[i]  * 256.0f)) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) ((gctINT) (Offset[i] * 256.0f)) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)));
    }

    status =  gcoVGHARDWARE_SetStates(
        Hardware,
        0x02830 >> 2,
        gcmCOUNTOF(states), states
        );

   gcmFOOTER();
   return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Paint API functions. *******************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_SetPaintSolid(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT8 Red,
    IN gctUINT8 Green,
    IN gctUINT8 Blue,
    IN gctUINT8 Alpha
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Build color value. */
        gctUINT32 color
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0))) | (((gctUINT32) ((gctUINT32) (Red) & ((gctUINT32) ((((1 ? 7:0) - (0 ? 7:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:0) - (0 ? 7:0) + 1))))))) << (0 ? 7:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8))) | (((gctUINT32) ((gctUINT32) (Green) & ((gctUINT32) ((((1 ? 15:8) - (0 ? 15:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:8) - (0 ? 15:8) + 1))))))) << (0 ? 15:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16))) | (((gctUINT32) ((gctUINT32) (Blue) & ((gctUINT32) ((((1 ? 23:16) - (0 ? 23:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:16) - (0 ? 23:16) + 1))))))) << (0 ? 23:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24))) | (((gctUINT32) ((gctUINT32) (Alpha) & ((gctUINT32) ((((1 ? 31:24) - (0 ? 31:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:24) - (0 ? 31:24) + 1))))))) << (0 ? 31:24)));

        /* Program paint color. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02808 >> 2,
            color
            ));

        /* Set VG Control paint mode. */
        Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) (0x0  & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)));

        /* The paint is non-linear. */
        Hardware->vg.paintLinear = gcvFALSE;

        /* Success. */
        gcmFOOTER();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the error. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_SetPaintLinear(
    IN gcoVGHARDWARE Hardware,
    IN gctFLOAT Constant,
    IN gctFLOAT StepX,
    IN gctFLOAT StepY
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Program linear gradient interpolation values. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02810 >> 2,
            *(gctUINT32*) &Constant
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02818 >> 2,
            *(gctUINT32*) &StepX
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02820 >> 2,
            *(gctUINT32*) &StepY
            ));

        /* Set VG Control paint mode. */
        Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) (0x1  & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)));

        /* The paint is non-linear. */
        Hardware->vg.paintLinear = gcvFALSE;

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
gcoVGHARDWARE_SetPaintRadial(
    IN gcoVGHARDWARE Hardware,
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

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Program radial gradient interpolation values. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02810 >> 2,
            *(gctUINT32*) &LinConstant
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02818 >> 2,
            *(gctUINT32*) &LinStepX
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02820 >> 2,
            *(gctUINT32*) &LinStepY
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x02810 >> 2) + 1,
            *(gctUINT32*) &RadConstant
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x02818 >> 2) + 1,
            *(gctUINT32*) &RadStepX
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x02820 >> 2) + 1,
            *(gctUINT32*) &RadStepY
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x0280C >> 2,
            *(gctUINT32*) &RadStepXX
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02828 >> 2,
            *(gctUINT32*) &RadStepYY
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x0282C >> 2,
            *(gctUINT32*) &RadStepXY
            ));

        /* Set VG Control paint mode. */
        Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) (0x2  & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)));

        /* The paint is non-linear. */
        Hardware->vg.paintLinear = gcvFALSE;

        /* Success. */
        gcmFOOTER();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the error. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_SetPaintPattern(
    IN gcoVGHARDWARE Hardware,
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

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctFLOAT paintOrigin[2];
        gctFLOAT paintStepX[2];
        gctFLOAT paintStepY[2];

        paintOrigin[0] = UConstant;
        paintOrigin[1] = VConstant;

        paintStepX[0] = UStepX;
        paintStepX[1] = VStepX;

        paintStepY[0] = UStepY;
        paintStepY[1] = VStepY;

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x02810 >> 2,
                *(gctUINT32*) &paintOrigin[0]
                ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                (0x02810 >> 2) + 1,
                 *(gctUINT32*) &paintOrigin[1]
                ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x02818 >> 2,
                *(gctUINT32*) &paintStepX[0]
                ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                (0x02818 >> 2) + 1,
                *(gctUINT32*) &paintStepX[1]
                ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x02820 >> 2,
                *(gctUINT32*) &paintStepY[0]
                ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                (0x02820 >> 2) + 1,
                *(gctUINT32*) &paintStepY[1]
                ));

        /* Set VG Control paint mode. */
        Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) (0x3  & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)));

        /* The paint is non-linear. */
        Hardware->vg.paintLinear = Linear;

        /* Success. */
        gcmFOOTER();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the error. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_SetPaintImage(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Image,
    IN gceTILE_MODE TileMode,
    IN gceIMAGE_FILTER Filter,
    IN gctUINT32 FillColor
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware=0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    do
    {
        gcmERR_BREAK(_SetSampler(
            Hardware,
            0, /* Use the first sampler for paints.  */
            Image,
            TileMode, /* Tile mode.                         */
            gcvFALSE, /* Disable mask.                      */
            Filter, /* Image quailty filter mode.         */
            gcvFALSE, /* Not in image filter mode.          */
            0, 0,
            Image->rect.right,
            Image->rect.bottom
            ));

        /* Enable paint/image transform premultiplication if... */
        if (

            /* ...target is premultiplied. */
            Hardware->vg.targetPremultiplied

            /* ...paint/image output is not premultiplied. */
            || (Image->colorType & gcvSURF_COLOR_ALPHA_PRE)

            /* ...image filter is enabled. */
            || (Filter != gcvFILTER_POINT)

            /* ...blending is enabled. */
            || ((Hardware->vg.blendMode != gcvVG_BLEND_SRC) &&
                (Hardware->vg.blendMode != gcvVG_BLEND_FILTER))

            /* ...color transform is ebabled. */
            || Hardware->vg.colorTransform
            )
        {
            /* Premultiply transformed color if not in image filter mode. */
            Hardware->vg.colorPremultiply = 0x0;

            /* Demultiply the color if target is not premultiplied. */
            Hardware->vg.targetPremultiply = Hardware->vg.targetPremultiplied
                ? 0x1
                : 0x0;
        }
        else
        {
            /* Do not mul/demul colors. */
            Hardware->vg.colorPremultiply  = 0x1;
            Hardware->vg.targetPremultiply = 0x1;
        }

        if (TileMode == 0x0)
        {
            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x02898 >> 2,
                FillColor
                ));
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Mask API functions. ********************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_SetVgMask(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Mask
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware=0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    do
    {
        /* Program hardware with mask states. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02850 >> 2,
            gcmFIXADDRESS(Mask->node.physical)
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02854 >> 2,
            Mask->stride
            ));

        /* Success. */
        gcmFOOTER();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the error. */
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Tessellation API functions. ************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_SetTessellation(
    IN gcoVGHARDWARE Hardware,
    IN gctBOOL SoftwareTesselation,
    IN gctUINT16 TargetWidth,
    IN gctUINT16 TargetHeight,
    IN gctFLOAT Bias,
    IN gctFLOAT Scale,
    IN gctFLOAT_PTR UserToSurface,
    IN gcsTESSELATION_PTR TessellationBuffer
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /*
            VG states first.
        */

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x0286C >> 2,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)))
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x028C0 >> 2,
            gcmFIXADDRESS(TessellationBuffer->tsBufferPhysical)
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x028C4 >> 2,
            gcmFIXADDRESS(TessellationBuffer->L1BufferPhysical)
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x028C8 >> 2,
            gcmFIXADDRESS(TessellationBuffer->L2BufferPhysical)
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x028CC >> 2,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (TessellationBuffer->stride) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16))) | (((gctUINT32) ((gctUINT32) (TessellationBuffer->shift) & ((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)))
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02804 >> 2,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            ));


        /*
            Tessellator states.
        */

        if (SoftwareTesselation)
        {
            Hardware->vg.tsBuffer = TessellationBuffer;

            Hardware->vg.bias  = Bias;
            Hardware->vg.scale = Scale;

            gcoOS_MemCopy(
                Hardware->vg.userToSurface,
                UserToSurface,
                gcmSIZEOF(gctFLOAT) * 6
                );
        }
        else
        {
            gcmERR_BREAK(gcoVGHARDWARE_EnableTessellation(
                Hardware, gcvTRUE
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x028D4 >> 2,
                gcmFIXADDRESS(TessellationBuffer->tsBufferPhysical)
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x028D8 >> 2,
                gcmFIXADDRESS(TessellationBuffer->L1BufferPhysical)
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x028DC >> 2,
                gcmFIXADDRESS(TessellationBuffer->L2BufferPhysical)
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x028E0 >> 2,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (TessellationBuffer->stride) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16))) | (((gctUINT32) ((gctUINT32) (TessellationBuffer->shift) & ((gctUINT32) ((((1 ? 18:16) - (0 ? 18:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:16) - (0 ? 18:16) + 1))))))) << (0 ? 18:16)))
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x028E4 >> 2,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16)))
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x028E8 >> 2,
                  ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0))) | (((gctUINT32) ((gctUINT32) (TargetWidth) & ((gctUINT32) ((((1 ? 12:0) - (0 ? 12:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:0) - (0 ? 12:0) + 1))))))) << (0 ? 12:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16))) | (((gctUINT32) ((gctUINT32) (TargetHeight) & ((gctUINT32) ((((1 ? 28:16) - (0 ? 28:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:16) - (0 ? 28:16) + 1))))))) << (0 ? 28:16)))
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x028EC >> 2,
                * (gctUINT32_PTR) &Scale
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetState(
                Hardware,
                0x028F0 >> 2,
                * (gctUINT32_PTR) &Bias
                ));

            gcmERR_BREAK(gcoVGHARDWARE_SetStates(
                Hardware,
                0x02900 >> 2,
                6, UserToSurface
                ));
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Programming of control registers. ******************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_ProgramControl(
    gcoVGHARDWARE Hardware,
    gctPOINTER Logical,
    gctSIZE_T * Bytes
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    do
    {
        gctUINT stateBytes;
        gctUINT stateCount;
        gctUINT gamma;
        gctUINT filterControl;
        gctUINT tsControl;
        gctINT32 overflowInt;
        gctUINT32_PTR memory;

        /* Determine the size of a state with data. */
        stateBytes
            = Hardware->bufferInfo.stateCommandSize
            + gcmSIZEOF(gctUINT32);

        if (Bytes != gcvNULL)
        {
            /* Return the buffer size required for the control states. */
            * Bytes = stateBytes * 4;

            /* Success. */
            status = gcvSTATUS_OK;
            break;
        }


        /* Verify the arguments. */
        gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

        /* Determine target color space. */
        switch (Hardware->vg.imageMode)
        {
        case 0x0:
        case 0x3:
            /* Use paint for color space conversion. */
            gamma = (Hardware->vg.paintLinear == Hardware->vg.targetLinear)
                  ? 0x0
                  : Hardware->vg.targetLinear
                      ? 0x1
                      : 0x2;
            break;

        case 0x1:
        case 0x2:
        case 0x4:
            /* Use image for color space conversion. */
            gamma = (Hardware->vg.imageLinear == Hardware->vg.targetLinear)
                  ? 0x0
                  : Hardware->vg.targetLinear
                      ? 0x1
                      : 0x2;
            break;

        default:
            gcmASSERT(gcvFALSE);
            gamma = 0;
        }

        /* Set the control fields. */
        Hardware->vg.targetControl = ((((gctUINT32) (Hardware->vg.targetControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (gamma ) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)));

        Hardware->vg.targetControl = ((((gctUINT32) (Hardware->vg.targetControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.targetPremultiply ) & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)));

        Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.colorPremultiply ) & ((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28)));

        /* Get TS overflow interrupt number. */
        overflowInt = Hardware->bufferInfo.tsOverflowInt;

        /* Determine TS control. */
        if (Hardware->vg20)
        {
            tsControl
                = (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.tsQuality) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) &((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))))
                & (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.tsFillRule) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) &((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))) << (0 ? 7:7))))
                & (
                      ((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 11:11) - (0 ? 11:11) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:11) - (0 ? 11:11) + 1))))))) << (0 ? 11:11)))
                    & ((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
                    & ((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:12) - (0 ? 16:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:12) - (0 ? 16:12) + 1))))))) << (0 ? 16:12))) | (((gctUINT32) ((gctUINT32) (overflowInt) & ((gctUINT32) ((((1 ? 16:12) - (0 ? 16:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:12) - (0 ? 16:12) + 1))))))) << (0 ? 16:12)))
                )
                & (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.tsDataType) & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) &((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 23:23) - (0 ? 23:23) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 23:23) - (0 ? 23:23) + 1))))))) << (0 ? 23:23))))
                & (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.tsMode) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24))) &((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:27) - (0 ? 27:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:27) - (0 ? 27:27) + 1))))))) << (0 ? 27:27))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 27:27) - (0 ? 27:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:27) - (0 ? 27:27) + 1))))))) << (0 ? 27:27))))
                & (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1))))))) << (0 ? 28:28))) &  ((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 29:29) - (0 ? 29:29) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 29:29) - (0 ? 29:29) + 1))))))) << (0 ? 29:29))) );

            /* Enable auto-restart. */
            if (Hardware->vgRestart)
            {
                tsControl &= (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 30:30) - (0 ? 30:30) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 30:30) - (0 ? 30:30) + 1))))))) << (0 ? 30:30))) &  ((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 31:31) - (0 ? 31:31) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:31) - (0 ? 31:31) + 1))))))) << (0 ? 31:31))) );
            }
        }
        else
        {
            tsControl
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.tsQuality) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.tsFillRule) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1))))))) << (0 ? 4:4)))
                | (
                      ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:12) - (0 ? 16:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:12) - (0 ? 16:12) + 1))))))) << (0 ? 16:12))) | (((gctUINT32) ((gctUINT32) (overflowInt) & ((gctUINT32) ((((1 ? 16:12) - (0 ? 16:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:12) - (0 ? 16:12) + 1))))))) << (0 ? 16:12)))
                )
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.tsDataType) & ((gctUINT32) ((((1 ? 21:20) - (0 ? 21:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:20) - (0 ? 21:20) + 1))))))) << (0 ? 21:20)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.tsMode) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24)));
         }

        /* Determine filter control value. */
        filterControl
            = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.filterMethod) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.filterChannels) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.filterColorSpace) & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.filterPremultiply) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1))))))) << (0 ? 16:16)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.filterDemultiply) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1))))))) << (0 ? 20:20)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12))) | (((gctUINT32) ((gctUINT32) (Hardware->vg.filterKernelSize) & ((gctUINT32) ((((1 ? 15:12) - (0 ? 15:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:12) - (0 ? 15:12) + 1))))))) << (0 ? 15:12)));

        /* Is the space already reserved? */
        if (Logical != gcvNULL)
        {
            /* Set the initial pointer. */
            memory = (gctUINT32_PTR) Logical;
        }
        else
        {
            /* Reserve space. */
            gcmERR_BREAK(gcoVGBUFFER_Reserve(
                Hardware->buffer,
                stateBytes * 4,
                gcvTRUE,
                (gctPOINTER *) &memory
                ));
        }

        /* Detemrine state command dword count. */
        stateCount = stateBytes / gcmSIZEOF(gctUINT32);

        /* Program filter control. */
        gcmERR_BREAK(gcoVGHARDWARE_UpdateState(
            Hardware,
            0x02950 >> 2,
            filterControl
            ));

        gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
            Hardware,
            memory,
            0x02950 >> 2, 1,
            gcvNULL
            ));

        memory[1] = filterControl;
        memory   += stateCount;

        /* Program TS control. */
        gcmERR_BREAK(gcoVGHARDWARE_UpdateState(
            Hardware,
            0x028D0 >> 2,
            tsControl
            ));

        gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
            Hardware,
            memory,
            0x028D0 >> 2, 1,
            gcvNULL
            ));

        memory[1] = tsControl;
        memory   += stateCount;

        /* Program VG control. */
        gcmERR_BREAK(gcoVGHARDWARE_UpdateState(
            Hardware,
            0x02800 >> 2,
            Hardware->vg.vgControl
            ));

        gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
            Hardware,
            memory,
            0x02800 >> 2, 1,
            gcvNULL
            ));

        memory[1] = Hardware->vg.vgControl;
        memory   += stateCount;

        /* Set the target control. */
        gcmERR_BREAK(gcoVGHARDWARE_UpdateState(
            Hardware,
            0x02840 >> 2,
            Hardware->vg.targetControl
            ));

        gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
            Hardware,
            memory,
            0x02840 >> 2, 1,
            gcvNULL
            ));

        memory[1] = Hardware->vg.targetControl;
        memory   += stateCount;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/******************************************************************************\
** gcoVGHARDWARE: Image Filters. *************************************************
\******************************************************************************/

static gctBOOL
_IsFilterSupported(
    IN gcoVGHARDWARE Hardware,
    IN gceCHANNEL ColorChannels,
    IN gcsSURF_INFO_PTR Target,
    IN gcsPOINT_PTR TargetOrigin
    )
{
    gctBOOL result;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Is the harwdare filter available? */
        if (!Hardware->vgFilter)
        {
            result = gcvFALSE;
            break;
        }

        /* Make sure the target is supported. */
        if (!gcoVGHARDWARE_IsTargetSupported(Target->format))
        {
            result = gcvFALSE;
            break;
        }

        /* Target cannot be a child image with an offset because there is no
           coordinate transformation between target and source for filters. */
        if ((TargetOrigin->x != 0) || (TargetOrigin->y != 0))
        {
            result = gcvFALSE;
            break;
        }

        /* Target too large? */
        if ((Target->rect.right > 4096) || (Target->rect.bottom > 4096))
        {
            result = gcvFALSE;
            break;
        }

        /* When target is premultiplied and not all filtered (source) components
           are being written, proper operation cannot be conducted because the
           internal operation is performed in premultiplied color, but target
           and source colors are premultiplied with different alphas. */
        if (((Target->colorType & gcvSURF_COLOR_ALPHA_PRE) != 0) &&
            (ColorChannels != gcvCHANNEL_RGBA))
        {
            result = gcvFALSE;
            break;
        }

        /* Supported. */
        result = gcvTRUE;
    }
    while (gcvFALSE);

    gcmFOOTER_NO();

    /* Return result. */
    return result;
}

static gceSTATUS
_SetWeightArray(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT WeightCount,
    IN const gctINT16 * WeightArray,
    IN gctBOOL Reverse
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctUINT count;
        gctUINT32_PTR memory;

        /* Determine the number of states. */
        count = (WeightCount + 1) / 2;

        /* Reserve space in the command buffer. */
        gcmERR_BREAK(gcoVGBUFFER_Reserve(
            Hardware->buffer,
            4 + count * 4,
            gcvTRUE,
            (gctPOINTER *) &memory
            ));

        /* Assemble load state command. */
        gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
            Hardware,
            memory,
            0x02980 >> 2,
            count,
            gcvNULL
            ));

        /* Copy states. */
        if (Reverse)
        {
            gctUINT i;
            gctINT16_PTR source = (gctINT16_PTR) WeightArray + WeightCount - 1;
            gctINT16_PTR target = (gctINT16_PTR) (memory + 1);

            /* Copy in reverse order. */
            for (i = 0; i < WeightCount; i += 1)
            {
                *target++ = *source--;
            }
        }
        else
        {
            /* Copy the state buffer. */
            gcmVERIFY_OK(gcoOS_MemCopy(
                &memory[1], WeightArray, WeightCount * gcmSIZEOF(gctINT16)
                ));
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

static gceSTATUS
_SetFilterScale(
    IN gcoVGHARDWARE Hardware,
    IN gctFLOAT PreBias,
    IN gctFLOAT Scale,
    IN gctFLOAT Bias
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctFLOAT data[3];

        /* Fill in data. */
        data[0] = PreBias;
        data[1] = Scale;
        data[2] = Bias;

        /* Load the states. */
        gcmERR_BREAK(gcoVGHARDWARE_SetStates(
            Hardware,
            0x02958 >> 2,
            gcmCOUNTOF(data), data
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_ColorMatrixSinglePass(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Target,
    IN gctINT16_PTR Matrix,
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

    static const gctUINT32 _colorSpace[] =
    {
        0x0, /* SOURCE: non-linear, FILTER: non-linear. */
        0x2, /* SOURCE: linear, FILTER: non-linear. */
        0x1, /* SOURCE: non-linear, FILTER: linear.     */
        0x0, /* SOURCE: linear, FILTER: linear.     */
    };

    /* When the color reaches the filter, it is always in non-premultiplied
       format, so we only need to know the filter premultiplication state. */
    static const gctUINT32 _premultiply[] =
    {
        0x0, /* FILTER: non-premul. */
        0x1, /* FILTER: premul.     */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctUINT colorSpaceIndex;
        gctBOOL sourceLinear;


        /***********************************************************************
        ** Generic pipe setup.
        */

        /* Set the target. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
            Hardware, Target
            ));

        /* Set special blending mode to allow for channel masking. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgBlendMode(
            Hardware, gcvVG_BLEND_FILTER
            ));

        /* Image filter mode:
           - allow paint to be processed - to be used for channel selection;
           - image processing is used for special blending setup. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgImageMode(
            Hardware, gcvVG_IMAGE_FILTER
            ));

        /* Set PATTERN paint mode. This will enable the paint to be "filtered",
           where it will be set to the preset channel enable (0 or 1) values to
           be later used during special blending. */
        Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) (0x3  & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)));

        /* Set image linear state to filter linear state. */
        Hardware->vg.imageLinear = FilterLinear;


        /***********************************************************************
        ** Determine filter control.
        */

        /* Determine whether the source is linear. */
        sourceLinear = (Source->colorType & gcvSURF_COLOR_LINEAR) != 0;

        /* Determine colorspace index. */
        colorSpaceIndex = sourceLinear | (FilterLinear << 1);

        /* Set filter parameters. */
        Hardware->vg.filterMethod      = 0x3;
        Hardware->vg.filterChannels    = ColorChannels;
        Hardware->vg.filterColorSpace  = _colorSpace[colorSpaceIndex];
        Hardware->vg.filterPremultiply = _premultiply[FilterPremultiplied];


        /***********************************************************************
        ** Setup image sampler.
        */

        /* Setup image sampler. */
        gcmERR_BREAK(_SetSampler(
            Hardware,
            1, /* Use the second sampler for images. */
            Source,
            gcvTILE_PAD, /* Tile mode.                         */
            gcvFALSE, /* Disable mask.                      */
            0x0,
            gcvTRUE, /* Image filter mode.                 */
            SourceOrigin->x,
            SourceOrigin->y,
            Width, Height
            ));

        /* Demultiply the color if the filter is premultiplied and the target is not. */
        if (FilterPremultiplied)
        {
            if (Hardware->vg.targetPremultiplied)
            {
                Hardware->vg.colorPremultiply  = 0x1;
                Hardware->vg.targetPremultiply = 0x1;
            }
            else
            {
                Hardware->vg.colorPremultiply  = 0x1;
                Hardware->vg.targetPremultiply = 0x0;
            }
        }
        else
        {
            if (Hardware->vg.targetPremultiplied)
            {
                Hardware->vg.colorPremultiply  = 0x0;
                Hardware->vg.targetPremultiply = 0x1;
            }
            else
            {
                Hardware->vg.colorPremultiply = 0x1;
                Hardware->vg.targetPremultiply = 0x1;
            }
        }


        /***********************************************************************
        ** Program the weights.
        */

        gcmERR_BREAK(_SetWeightArray(
            Hardware,
            4 * 5,
            Matrix,
            gcvFALSE
            ));


        /***********************************************************************
        ** Draw image.
        */

        /* Set rectangle primitive. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
            Hardware,
            gcvVG_RECTANGLE
            ));

        /* Render the rectangle. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
            Hardware,
            TargetOrigin->x,
            TargetOrigin->y,
            Width, Height
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_ColorMatrixMultiPass(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Temp,
    IN gcsSURF_INFO_PTR Target,
    IN gctINT16_PTR Matrix,
    IN gctUINT ColorChannels,
    IN gctBOOL FilterLinear,
    IN gcsPOINT_PTR SourceOrigin,
    IN gcsPOINT_PTR TargetOrigin,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gceSTATUS status;

    static const gctUINT32 _colorSpace[] =
    {
        0x0, /* SOURCE: non-linear, FILTER: non-linear. */
        0x2, /* SOURCE: linear, FILTER: non-linear. */
        0x1, /* SOURCE: non-linear, FILTER: linear.     */
        0x0, /* SOURCE: linear, FILTER: linear.     */
    };

    /* Filter identity matrix. */
    static const gctINT16 _identity[] =
    {
        (1 << 8), 0, 0, 0, 0,
        (1 << 8), 0, 0, 0, 0,
        (1 << 8), 0, 0, 0, 0,
        (1 << 8), 0, 0, 0, 0
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctUINT colorSpaceIndex;
        gctBOOL sourceLinear;


/*----------------------------------------------------------------------------*/
/*-------------------------------- First pass. -------------------------------*/

        /***********************************************************************
        ** Generic pipe setup.
        */

        /* Set the target. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
            Hardware, Temp
            ));

        /* Disable blending for the first pass. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgBlendMode(
            Hardware, gcvVG_BLEND_SRC
            ));

        /* Normal image mode for the first pass: paint is disabled. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgImageMode(
            Hardware, gcvVG_IMAGE_NORMAL
            ));

        /* Disable color space conversion. */
        Hardware->vg.imageLinear = Hardware->vg.targetLinear;


        /***********************************************************************
        ** Determine filter control.
        */

        /* Determine whether the source is linear. */
        sourceLinear = (Source->colorType & gcvSURF_COLOR_LINEAR) != 0;

        /* Determine colorspace index. */
        colorSpaceIndex = sourceLinear | (FilterLinear << 1);

        /* Set filter parameters. */
        Hardware->vg.filterMethod      = 0x3;
        Hardware->vg.filterChannels    = gcvCHANNEL_RGBA;
        Hardware->vg.filterColorSpace  = _colorSpace[colorSpaceIndex];
        Hardware->vg.filterPremultiply = 0x1;


        /***********************************************************************
        ** Setup image sampler.
        */

        /* Setup image sampler. */
        gcmERR_BREAK(_SetSampler(
            Hardware,
            1, /* Use the second sampler for images. */
            Source,
            gcvTILE_PAD, /* Tile mode.                         */
            gcvFALSE, /* Disable mask.                      */
            0x0,
            gcvTRUE, /* Image filter mode.                 */
            SourceOrigin->x,
            SourceOrigin->y,
            Width, Height
            ));

        Hardware->vg.colorPremultiply = 0x1;
        Hardware->vg.targetPremultiply = 0x1;


        /***********************************************************************
        ** Program the weights.
        */

        gcmERR_BREAK(_SetWeightArray(
            Hardware,
            4 * 5,
            Matrix,
            gcvFALSE
            ));


        /***********************************************************************
        ** Draw image.
        */

        /* Set rectangle primitive. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
            Hardware,
            gcvVG_RECTANGLE
            ));

        /* Render the rectangle. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
            Hardware,
            0, 0,
            Width, Height
            ));

        /* Flush PE before the imager gets to access the rendered image. */
        Hardware->vg.peFlushNeeded = gcvFALSE;
        gcmERR_BREAK(gcoVGHARDWARE_FlushPipe(
            Hardware
            ));

        /* Make sure PE is done before proceeding.  */
        gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
            Hardware,
            gcvNULL,
            gcvBLOCK_VG,
            gcvBLOCK_PIXEL,
            gcvHOW_SEMAPHORE_STALL,
            gcvNULL
            ));


/*----------------------------------------------------------------------------*/
/*-------------------------------- Second pass. ------------------------------*/

        /***********************************************************************
        ** Generic pipe setup.
        */

        /* Set the target. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
            Hardware, Target
            ));

        /* Set special blending mode to allow for channel masking. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgBlendMode(
            Hardware, gcvVG_BLEND_FILTER
            ));

        /* Image filter mode:
           - allow paint to be processed - to be used for channel selection;
           - image processing is used for special blending setup. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgImageMode(
            Hardware, gcvVG_IMAGE_FILTER
            ));

        /* Set PATTERN paint mode. This will enable the paint to be "filtered",
           where it will be set to the preset channel enable (0 or 1) values to
           be later used during special blending. */
        Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) (0x3  & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)));

        /* Set image linear state to filter linear state. */
        Hardware->vg.imageLinear = FilterLinear;


        /***********************************************************************
        ** Determine filter control.
        */

        /* Set filter parameters. */
        Hardware->vg.filterMethod      = 0x3;
        Hardware->vg.filterChannels    = ColorChannels;
        Hardware->vg.filterColorSpace  = 0x0;
        Hardware->vg.filterPremultiply = 0x0;


        /***********************************************************************
        ** Setup image sampler.
        */

        /* Setup image sampler. */
        gcmERR_BREAK(_SetSampler(
            Hardware,
            1, /* Use the second sampler for images. */
            Temp,
            gcvTILE_PAD, /* Tile mode.                         */
            gcvFALSE, /* Disable mask.                      */
            0x0,
            gcvTRUE, /* Image filter mode.                 */
            0, 0,
            Width, Height
            ));

        if (Hardware->vg.targetPremultiplied)
        {
            Hardware->vg.colorPremultiply  = 0x0;
            Hardware->vg.targetPremultiply = 0x1;
        }
        else
        {
            Hardware->vg.colorPremultiply  = 0x1;
            Hardware->vg.targetPremultiply = 0x1;
        }


        /***********************************************************************
        ** Program the identity matrix.
        */

        gcmERR_BREAK(_SetWeightArray(
            Hardware,
            gcmCOUNTOF(_identity),
            (gctPOINTER) _identity,
            gcvFALSE
            ));


        /***********************************************************************
        ** Draw image.
        */

        /* Set rectangle primitive. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
            Hardware,
            gcvVG_RECTANGLE
            ));

        /* Render the rectangle. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
            Hardware,
            TargetOrigin->x,
            TargetOrigin->y,
            Width, Height
            ));

        /* Flush PE before the imager gets to access the rendered image. */
        Hardware->vg.peFlushNeeded = gcvFALSE;
        gcmERR_BREAK(gcoVGHARDWARE_FlushPipe(
            Hardware
            ));

        /* Make sure PE is done before proceeding.  */
        gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
            Hardware,
            gcvNULL,
            gcvBLOCK_VG,
            gcvBLOCK_PIXEL,
            gcvHOW_SEMAPHORE_STALL,
            gcvNULL
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_ColorMatrix(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Target,
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

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctUINT i;
        gctINT16 weightArray[20];
        gctBOOL targetLinear;
        gctBOOL targetPremultiplied;

        /* Is the filter combination supported? */
        if (!_IsFilterSupported(Hardware, ColorChannels, Target, TargetOrigin))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* Convert the matrix to signed 8.8 fixed point. */
        for (i = 0; i < gcmCOUNTOF(weightArray); ++i)
        {
            gctFLOAT scaled = (gctFLOAT) floor(Matrix[i] * 256.0f + 0.5f);

            if ((scaled < -32768.0f) || (scaled > 32767.0f))
            {
                gcmFOOTER_NO();
                return gcvSTATUS_NOT_SUPPORTED;
            }

            weightArray[i] = (gctINT16) scaled;
        }

        /* Scissors are not used in filtering. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableScissor(Hardware, gcvFALSE));

        /* Masking is not used in filtering. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableMask(Hardware, gcvFALSE));

        /* Color transform is not used in filtering. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableColorTransform(Hardware, gcvFALSE));

        /* Determine target image characteristics. */
        targetLinear        = (Target->colorType & gcvSURF_COLOR_LINEAR)    != 0;
        targetPremultiplied = (Target->colorType & gcvSURF_COLOR_ALPHA_PRE) != 0;

        /* Determine whether two passes are needed. */
        if (
                FilterPremultiplied &&
                (
                    (FilterLinear != targetLinear) ||
                    ((ColorChannels != gcvCHANNEL_RGBA) && !targetPremultiplied)
                )
           )
        {
            /*******************************************************************
            ** Special case with two passes when the filter is being done in
            ** premultiplied format and one of the following is true:
            ** - filter color space is not the same as the target color space;
            **   this limitation is there because there is no alpha devider
            **   between the filter and the color space conversion module;
            ** - target is not premultiplied and not all color channels are
            **   enabled; this limitation is there because there is no way to
            **   unpremultiply the end result correctly because some channels
            **   of the result are replaced with the current premultiplied
            **   destination, but the alpha values of the incoming image and
            **   the current destination may not match.
            */

            gcsSURF_FORMAT_INFO_PTR tempFormat[2];

            /* Get the format descriptor. */
            gcmERR_BREAK(gcoSURF_QueryFormat(
                gcvSURF_A8R8G8B8, tempFormat
                ));

            /* Allocate the temporary surface. */
            gcmERR_BREAK(gcoVGHARDWARE_AllocateTemporarySurface(
                Hardware, Width, Height, tempFormat[0], gcvSURF_BITMAP
                ));

            /* The temporary buffer color type. */
            Hardware->tempBuffer.colorType = ((Source->colorType & gcvSURF_COLOR_LINEAR) != 0)
                ? gcvSURF_COLOR_ALPHA_PRE | gcvSURF_COLOR_LINEAR
                : gcvSURF_COLOR_ALPHA_PRE;

            /* Execute color matrix. */
            gcmERR_BREAK(gcoVGHARDWARE_ColorMatrixMultiPass(
                Hardware,
                Source,
                &Hardware->tempBuffer,
                Target,
                weightArray,
                ColorChannels,
                FilterLinear,
                SourceOrigin,
                TargetOrigin,
                Width, Height
                ));
        }
        else
        {
            /*******************************************************************
            ** Just one pass is needed here.
            */

            /* Execute color matrix. */
            gcmERR_BREAK(gcoVGHARDWARE_ColorMatrixSinglePass(
                Hardware,
                Source,
                Target,
                weightArray,
                ColorChannels,
                FilterLinear,
                FilterPremultiplied,
                SourceOrigin,
                TargetOrigin,
                Width, Height
                ));
        }
    }
    while (gcvFALSE);

    /* Disable filter. */
    Hardware->vg.filterMethod = 0x0;

    gcmFOOTER();
    /* Return status. */
    return status;
}

typedef struct _gcsCONVOLVE * gcsCONVOLVE_PTR;
typedef struct _gcsCONVOLVE
{
    gctFLOAT hPreBias;
    gctFLOAT hScale;
    gctFLOAT hBias;

    gctFLOAT vPreBias;
    gctFLOAT vScale;
    gctFLOAT vBias;

    gctINT shiftX;
    gctINT shiftY;
}
gcsCONVOLVE;

static gceSTATUS
_SeparableConvolve(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Target,
    IN gctINT KernelWidth,
    IN gctINT KernelHeight,
    IN gcsCONVOLVE_PTR Info,
    IN const gctINT16 * KernelX,
    IN const gctINT16 * KernelY,
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

    /* Temporary surface color depends on the filter color. */
    static const gctUINT32 _tempColor[] =
    {
        gcvSURF_COLOR_UNKNOWN, /* FILTER: non-linear, non-premul. */
        gcvSURF_COLOR_LINEAR, /* FILTER: linear, non-premul. */
        gcvSURF_COLOR_ALPHA_PRE, /* FILTER: non-linear, premul.     */
        gcvSURF_COLOR_LINEAR | gcvSURF_COLOR_ALPHA_PRE  /* FILTER: linear, premul.     */
    };

    static const gctUINT32 _colorSpace[] =
    {
        0x0, /* SOURCE: non-linear, FILTER: non-linear. */
        0x2, /* SOURCE: linear, FILTER: non-linear. */
        0x1, /* SOURCE: non-linear, FILTER: linear.     */
        0x0, /* SOURCE: linear, FILTER: linear.     */
    };

    /* When the color reaches the filter, it is always in non-premultiplied
       format, so we only need to know the filter premultiplication state. */
    static const gctUINT32 _premultiply[] =
    {
        0x0, /* FILTER: non-premul. */
        0x1, /* FILTER: premul.     */
    };

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gcsSURF_FORMAT_INFO_PTR tempFormat[2];
        gctUINT tempColorIndex;
        gctUINT colorSpaceIndex;
        gctUINT demultiply;
        gctBOOL sourceLinear;
        gctUINT32 fillColor;


        /***********************************************************************
        ** Generic pipe setup.
        */

        /* Get the format descriptor. */
        gcmERR_BREAK(gcoSURF_QueryFormat(
            gcvSURF_A8R8G8B8, tempFormat
            ));

        /* Allocate the temporary surface. */
        gcmERR_BREAK(gcoVGHARDWARE_AllocateTemporarySurface(
            Hardware, Width, Height, tempFormat[0], gcvSURF_BITMAP
            ));

        /* Determine the color type. */
        tempColorIndex = FilterLinear | (FilterPremultiplied << 1);
        Hardware->tempBuffer.colorType = _tempColor[tempColorIndex];

        /* Scissors are not used in filtering. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableScissor(Hardware, gcvFALSE));

        /* Masking is not used in filtering. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableMask(Hardware, gcvFALSE));

        /* Color transform is not used in filtering. */
        gcmERR_BREAK(gcoVGHARDWARE_EnableColorTransform(Hardware, gcvFALSE));

        /* Convert fill color. */
        fillColor = gcoVGHARDWARE_PackColor32(
            FillColor[0],
            FillColor[1],
            FillColor[2],
            FillColor[3]
            );

        /* Program fill color. */
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            (0x02898 >> 2) + 1,
            fillColor
            ));

        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x02954 >> 2,
              ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Info->shiftX) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
            | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16))) | (((gctUINT32) ((gctUINT32) (Info->shiftY) & ((gctUINT32) ((((1 ? 31:16) - (0 ? 31:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:16) - (0 ? 31:16) + 1))))))) << (0 ? 31:16)))
            ));


/*----------------------------------------------------------------------------*/
/*----------------------------- Horizontal pass. -----------------------------*/

        /***********************************************************************
        ** Generic pipe setup.
        */

        /* Set the target. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
            Hardware, &Hardware->tempBuffer
            ));

        /* Disable blending for the first pass. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgBlendMode(
            Hardware, gcvVG_BLEND_SRC
            ));

        /* Normal image mode for the first pass: paint is disabled. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgImageMode(
            Hardware, gcvVG_IMAGE_NORMAL
            ));

        /* Disable color space conversion. */
        Hardware->vg.imageLinear = Hardware->vg.targetLinear;


        /***********************************************************************
        ** Determine filter control.
        */

        /* Determine whether the source is linear. */
        sourceLinear = (Source->colorType & gcvSURF_COLOR_LINEAR) != 0;

        /* Determine colorspace index. */
        colorSpaceIndex = sourceLinear | (FilterLinear << 1);

        /* Set filter parameters. */
        Hardware->vg.filterMethod      = 0x1;
        Hardware->vg.filterChannels    = gcvCHANNEL_RGBA;
        Hardware->vg.filterColorSpace  = _colorSpace[colorSpaceIndex];
        Hardware->vg.filterPremultiply = _premultiply[FilterPremultiplied];
        Hardware->vg.filterDemultiply  = 0x0;
        Hardware->vg.filterKernelSize  = KernelWidth;


        /***********************************************************************
        ** Setup image sampler.
        */

        /* Setup image sampler. */
        gcmERR_BREAK(_SetSampler(
            Hardware,
            1, /* Use the second sampler for images. */
            Source,
            TilingMode, /* Tile mode.                         */
            gcvFALSE, /* Disable mask.                      */
            0x0,
            gcvTRUE, /* Image filter mode.                 */
            SourceOrigin->x,
            SourceOrigin->y,
            SourceSize->width,
            SourceSize->height
            ));

        Hardware->vg.colorPremultiply = 0x1;
        Hardware->vg.targetPremultiply = 0x1;


        /***********************************************************************
        ** Program filter parameters.
        */

        gcmERR_BREAK(_SetFilterScale(
            Hardware,
            Info->hPreBias,
            Info->hScale,
            Info->hBias
            ));

        gcmERR_BREAK(_SetWeightArray(
            Hardware,
            KernelWidth,
            KernelX,
            gcvTRUE
            ));


        /***********************************************************************
        ** Draw image.
        */

        /* Set rectangle primitive. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
            Hardware,
            gcvVG_RECTANGLE
            ));

        /* Render the rectangle. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
            Hardware,
            0, 0,
            Width, Height
            ));

        /* Flush PE before the imager gets to access the rendered image. */
        Hardware->vg.peFlushNeeded = gcvFALSE;
        gcmERR_BREAK(gcoVGHARDWARE_FlushPipe(
            Hardware
            ));

        /* Make sure PE is done before proceeding.  */
        gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
            Hardware,
            gcvNULL,
            gcvBLOCK_VG,
            gcvBLOCK_PIXEL,
            gcvHOW_SEMAPHORE_STALL,
            gcvNULL
            ));

/*----------------------------------------------------------------------------*/
/*------------------------------- Vertical pass. -----------------------------*/

        /***********************************************************************
        ** Generic pipe setup.
        */

        /* Set the target. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgTarget(
            Hardware, Target
            ));

        /* Set special blending mode to allow for channel masking. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgBlendMode(
            Hardware, gcvVG_BLEND_FILTER
            ));

        /* Image filter mode:
           - allow paint to be processed - to be used for channel selection;
           - image processing is used for special blending setup. */
        gcmERR_BREAK(gcoVGHARDWARE_SetVgImageMode(
            Hardware, gcvVG_IMAGE_FILTER
            ));

        /* Set PATTERN paint mode. This will enable the paint to be "filtered",
           where it will be set to the preset channel enable (0 or 1) values to
           be later used during special blending. */
        Hardware->vg.vgControl = ((((gctUINT32) (Hardware->vg.vgControl)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24))) | (((gctUINT32) (0x3  & ((gctUINT32) ((((1 ? 25:24) - (0 ? 25:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:24) - (0 ? 25:24) + 1))))))) << (0 ? 25:24)));

        /* Set image linear state to filter linear state. */
        Hardware->vg.imageLinear = FilterLinear;


        /***********************************************************************
        ** Determine filter control.
        */

        /* Determine the demultiplication setting. */
        if (
                FilterPremultiplied &&
                (
                    (FilterLinear != Hardware->vg.targetLinear) ||
                    !Hardware->vg.targetPremultiplied
                )
           )
        {
            demultiply = 0x1;
        }
        else
        {
            demultiply = 0x0;
        }

        /* Set filter parameters. */
        Hardware->vg.filterMethod      = 0x2;
        Hardware->vg.filterChannels    = ColorChannels;
        Hardware->vg.filterColorSpace  = 0x0;
        Hardware->vg.filterPremultiply = 0x0;
        Hardware->vg.filterDemultiply  = demultiply;
        Hardware->vg.filterKernelSize  = KernelHeight;


        /***********************************************************************
        ** Setup image sampler.
        */

        /* Setup image sampler. */
        gcmERR_BREAK(_SetSampler(
            Hardware,
            1, /* Use the second sampler for images. */
            &Hardware->tempBuffer,
            TilingMode, /* Tile mode.                         */
            gcvFALSE, /* Disable mask.                      */
            0x0,
            gcvTRUE, /* Image filter mode.                 */
            0, 0,
            Width, Height
            ));

        if (Hardware->vg.targetPremultiplied)
        {
            Hardware->vg.colorPremultiply = 0x0;
            Hardware->vg.targetPremultiply = 0x1;
        }
        else
        {
            Hardware->vg.colorPremultiply = 0x1;
            Hardware->vg.targetPremultiply = 0x1;
        }


        /***********************************************************************
        ** Program filter parameters.
        */

        gcmERR_BREAK(_SetFilterScale(
            Hardware,
            Info->vPreBias,
            Info->vScale,
            Info->vBias
            ));

        gcmERR_BREAK(_SetWeightArray(
            Hardware,
            KernelHeight,
            KernelY,
            gcvTRUE
            ));


        /***********************************************************************
        ** Draw image.
        */

        /* Set rectangle primitive. */
        gcmERR_BREAK(gcoVGHARDWARE_SetPrimitiveMode(
            Hardware,
            gcvVG_RECTANGLE
            ));

        /* Render the rectangle. */
        gcmERR_BREAK(gcoVGHARDWARE_DrawVgRect(
            Hardware,
            TargetOrigin->x,
            TargetOrigin->y,
            Width, Height
            ));

        /* Flush PE before the imager gets to access the rendered image. */
        Hardware->vg.peFlushNeeded = gcvFALSE;
        gcmERR_BREAK(gcoVGHARDWARE_FlushPipe(
            Hardware
            ));

        /* Make sure PE is done before proceeding.  */
        gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
            Hardware,
            gcvNULL,
            gcvBLOCK_VG,
            gcvBLOCK_PIXEL,
            gcvHOW_SEMAPHORE_STALL,
            gcvNULL
            ));
    }
    while (gcvFALSE);

    /* Disable filter. */
    Hardware->vg.filterMethod = 0x0;

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_SeparableConvolve(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Target,
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

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctINT i, hMin, hNeg, hPos, hRange, vSum;
        gcsCONVOLVE info;

        /* Is the filter combination supported? */
        if (!_IsFilterSupported(Hardware, ColorChannels, Target, TargetOrigin))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* Verify kernel size. */
        if ((KernelWidth > 15) || (KernelHeight > 15))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }


        /***********************************************************************
        ** Compute scale and bias.
        */

        hMin = 0;
        hNeg = 0;
        hPos = 0;

        for (i = 0; i < KernelWidth; i += 1)
        {
            /* Find the minimum kernel value. */
            if (KernelX[i] < hMin)
            {
                hMin = KernelX[i];
            }

            /* Find negative and positive sums. */
            if (KernelX[i] < 0)
            {
                hNeg += KernelX[i];
            }
            else
            {
                hPos += KernelX[i];
            }
        }

        hRange = hPos - hNeg;

        info.hPreBias = 0.0f;
        info.hScale   = 1.0f / hRange;
        info.hBias    = ((gctFLOAT) (-hMin)) / hRange;

        vSum = 0;

        for (i = 0; i < KernelHeight; i += 1)
        {
            vSum += KernelY[i];
        }

        info.vPreBias = vSum  * -info.hBias;
        info.vScale   = Scale /  info.hScale;
        info.vBias    = Bias;


        /***********************************************************************
        ** Execute convolve.
        */

        info.shiftX = ShiftX % Width;
        info.shiftY = ShiftY % Height;

        status = _SeparableConvolve(
            Hardware,
            Source, Target,
            KernelWidth, KernelHeight,
            &info,
            KernelX, KernelY,
            TilingMode, FillColor,
            ColorChannels,
            FilterLinear, FilterPremultiplied,
            SourceOrigin, TargetOrigin,
            SourceSize,
            Width, Height
            );
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_GaussianBlur(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_INFO_PTR Source,
    IN gcsSURF_INFO_PTR Target,
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

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        const gctFLOAT factor = 16384.0f;

        gctUINT i;
        gctINT16 weightX[15];
        gctINT16 weightY[15];
        gctFLOAT expX, expY;
        gctUINT halfKernelX;
        gctUINT halfKernelY;
        gctUINT kernelWidth;
        gctUINT kernelHeight;
        gcsCONVOLVE info;

        /* Is the filter combination supported? */
        if (!_IsFilterSupported(Hardware, ColorChannels, Target, TargetOrigin))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* We don't have enough filter taps to support anything larger. */
        if ((StdDeviationX > 4.0f) || (StdDeviationY > 4.0f))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* We can only wrap around once in integer mode. */
        if (((SourceSize->width < 7) || (SourceSize->height < 7)) &&
            ((TilingMode == gcvTILE_REPEAT) || (TilingMode == gcvTILE_REFLECT)))
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }


        /***********************************************************************
        ** Compute the kernel.
        */

        expX = -1.0f / (2.0f * StdDeviationX * StdDeviationX);
        expY = -1.0f / (2.0f * StdDeviationY * StdDeviationY);

        /* Compute the width of the kernel. */
        for (halfKernelX = 7; halfKernelX > 0; halfKernelX -= 1)
        {
            gctFLOAT x2 = (gctFLOAT) (halfKernelX * halfKernelX);

            if (((gctINT16) (exp(x2 * expX) * factor + 0.5f)) != 0)
            {
                break;
            }
        }

        kernelWidth = halfKernelX * 2 + 1;

        /* Compute the height of the kernel. */
        for (halfKernelY = 7; halfKernelY > 0; halfKernelY -= 1)
        {
            gctFLOAT y2 = (gctFLOAT) (halfKernelY * halfKernelY);

            if (((gctINT16) (exp(y2 * expY) * factor + 0.5f)) != 0)
            {
                break;
            }
        }

        kernelHeight = halfKernelY * 2 + 1;

        /* Compute X kernel. */
        info.hPreBias = 0.0f;
        info.hScale   = 0.0f;
        info.hBias    = 0.0f;

        for (i = 0; i < kernelWidth; ++i)
        {
            gctFLOAT i2 = (gctFLOAT) ((i - halfKernelX) * (i - halfKernelX));
            gctFLOAT x  = (gctFLOAT) exp(i2 * expX);
            weightX[i] = (gctINT16) (x * factor + 0.5f);
            info.hScale += weightX[i];
        }

        info.hScale = 1.0f / info.hScale;

        /* Compute Y kernel. */
        info.vPreBias = 0.0f;
        info.vScale   = 0.0f;
        info.vBias    = 0.0f;

        for (i = 0; i < kernelHeight; ++i)
        {
            gctFLOAT i2 = (gctFLOAT) ((i - halfKernelY) * (i - halfKernelY));
            gctFLOAT y  = (gctFLOAT) exp(i2 * expY);
            weightY[i] = (gctINT16) (y * factor + 0.5f);
            info.vScale += weightY[i];
        }

        info.vScale = 1.0f / info.vScale;


        /***********************************************************************
        ** Execute convolve.
        */

        info.shiftX = halfKernelX;
        info.shiftY = halfKernelY;

        status = _SeparableConvolve(
            Hardware,
            Source, Target,
            kernelWidth, kernelHeight,
            &info,
            weightX, weightY,
            TilingMode, FillColor,
            ColorChannels,
            FilterLinear, FilterPremultiplied,
            SourceOrigin, TargetOrigin,
            SourceSize,
            Width, Height
            );
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

gceSTATUS
gcoVGHARDWARE_EnableDither(
    gcoVGHARDWARE Hardware,
    gctBOOL Enable
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);


    do
    {
        if (Hardware->vgPEDither)
        {
            if (Enable && !Hardware->vg.ditherEnable)
            {
                gcmERR_BREAK(gcoVGHARDWARE_SetState(
                    Hardware,
                    0x02968 >> 2,
                    0x6E4CA280
                    ));

                gcmERR_BREAK(gcoVGHARDWARE_SetState(
                    Hardware,
                    0x0296C >> 2,
                    0x5D7F91B3
                    ));
            }
            else if (!Enable && Hardware->vg.ditherEnable)
            {
                gcmERR_BREAK(gcoVGHARDWARE_SetState(
                    Hardware,
                    0x02968 >> 2,
                    (gctUINT32)~0
                    ));

                gcmERR_BREAK(gcoVGHARDWARE_SetState(
                    Hardware,
                    0x0296C >> 2,
                    (gctUINT32)~0
                    ));
            }

            /* Store dither enable. */
            Hardware->vg.ditherEnable = Enable;
        }

    }
    while(gcvFALSE);

    gcmFOOTER();
    /* Success. */
    return status;
}


/******************************************************************************\
****************************** gcoVGHARDWARE API code *****************************
\******************************************************************************/

/*******************************************************************************
**
**  gcoVGHARDWARE_Construct
**
**  Construct a new gcoVGHARDWARE object.
**
**  INPUT:
**
**      gcoOS Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gcoVGHARDWARE * Hardware
**          Pointer to a variable that will hold the gcoVGHARDWARE object.
*/
gceSTATUS
gcoVGHARDWARE_Construct(
    IN gcoHAL Hal,
    OUT gcoVGHARDWARE * Hardware
    )
{
    gceSTATUS status, last;
    gcoOS os;
    gcoHAL hal;
    gcoVGHARDWARE hardware = gcvNULL;

    gcmHEADER_ARG("Hal=0x%x Hardware = 0x%x", Hal, Hardware);
    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Hardware != gcvNULL);

    do
    {
        gcsHAL_INTERFACE halInterface;

        /* Cast gcoHAL object. */
        hal = (gcoHAL) Hal;

        /* Verify gcoHAL object. */
        gcmVERIFY_OBJECT(hal, gcvOBJ_HAL);

        /* Shortcut to gcoOS object. */
        os = gcvNULL;

        /* Allocate the gcoVGHARDWARE object. */
        gcmERR_BREAK(gcoOS_Allocate(
            os, gcmSIZEOF(struct _gcoVGHARDWARE), (gctPOINTER *) &hardware
            ));

        /* Reset the object. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            hardware, gcmSIZEOF(struct _gcoVGHARDWARE)
            ));

        /* Initialize the gcoVGHARDWARE object. */
        hardware->object.type = gcvOBJ_HARDWARE;
        hardware->os  = os;
        hardware->hal = hal;

        /* Query chip identity. */
        halInterface.command = gcvHAL_QUERY_CHIP_IDENTITY;

        gcmERR_BREAK(gcoOS_DeviceControl(
            os,
            IOCTL_GCHAL_INTERFACE,
            &halInterface, gcmSIZEOF(halInterface),
            &halInterface, gcmSIZEOF(halInterface)
            ));

        gcmERR_BREAK(halInterface.status);

        hardware->chipModel          = halInterface.u.QueryChipIdentity.chipModel;
        hardware->chipRevision       = halInterface.u.QueryChipIdentity.chipRevision;
        hardware->chipFeatures       = halInterface.u.QueryChipIdentity.chipFeatures;
        hardware->chipMinorFeatures  = halInterface.u.QueryChipIdentity.chipMinorFeatures;
        hardware->chipMinorFeatures2 = halInterface.u.QueryChipIdentity.chipMinorFeatures2;

        /* Query the command buffer attributes. */
        halInterface.command = gcvHAL_QUERY_COMMAND_BUFFER;

        gcmERR_BREAK(gcoOS_DeviceControl(
            os,
            IOCTL_GCHAL_INTERFACE,
            &halInterface, gcmSIZEOF(halInterface),
            &halInterface, gcmSIZEOF(halInterface)
            ));

        gcmERR_BREAK(halInterface.status);

        /* Copy the command buffer information. */
        gcmVERIFY_OK(gcoOS_MemCopy(
            &hardware->bufferInfo,
            &halInterface.u.QueryCommandBuffer.information,
            gcmSIZEOF(hardware->bufferInfo)
            ));

        /* Set default API. */
        hardware->api = gcvAPI_OPENGL;

        /* Determine whether 2D hardware is present. */
        hardware->hw2DEngine = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 9:9) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))));

        /* Determine whether 3D hardware is present. */
        hardware->hw3DEngine = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 2:2) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))));

        /* Determine whether 3D hardware is present. */
        hardware->hwVGEngine = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 26:26) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))));

        /* Determine whether FE 2.0 is present. */
        hardware->fe20 = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 28:28) & ((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 28:28) - (0 ? 28:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 28:28) - (0 ? 28:28) + 1)))))));

        /* Determine whether VG 2.0 is present. */
        hardware->vg20 = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 13:13) & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))));

        /* Determine whether VG 2.1 is present. */
        hardware->vg21 = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 18:18) & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))));

        /* Determine whether VG filter is present. */
        hardware->vgFilter = ((((gctUINT32) (hardware->chipMinorFeatures)) >> (0 ? 17:17) & ((gctUINT32) ((((1 ? 17:17) - (0 ? 17:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:17) - (0 ? 17:17) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 17:17) - (0 ? 17:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:17) - (0 ? 17:17) + 1)))))));

        /* Determine whether VG double buffering is supported. */
        hardware->vgDoubleBuffer = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 2:2) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))));

        /* Determine whether VG TS restart is supported. */
        hardware->vgRestart = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 8:8) & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1)))))));

        /* Determine whether VG Dither is supported. */
        hardware->vgPEDither = ((((gctUINT32) (hardware->chipMinorFeatures2)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))));
        /* Benefit from double buffering feature is not significant, yet it
           presents a potential issue when running multiple applications at the
           same time. Disable double buffering here. */
        hardware->vgDoubleBuffer = gcvFALSE;

        /* Determine VG primitive to use for tesselation. */
        hardware->vg.tesselationPrimitive = hardware->vg20
            ? gcvVG_TESSELLATED_TILED
            : gcvVG_TESSELLATED;

        /* Initialize the semaphore states. */
        hardware->vg.stallSkipCount = 2;

        hardware->vg.ditherEnable = gcvFALSE;

        /* Don't force software by default. */
        hardware->sw2DEngine = gcvFALSE;

        /* Determine whether byte write feature is present in the chip. */
        hardware->byteWrite = ((((gctUINT32) (hardware->chipFeatures)) >> (0 ? 19:19) & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))));

        /* Report. */
        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_DRIVER,
            "%s(%d):\n"
            "  2D=%d, 3D=%d, VG=%d\n"
            "  FE20=%d, VG20=%d, VG21=%d\n"
            "  TS Prim Tiled=%d\n"
            "  2D Byte Write=%d\n",
            __FUNCTION__, __LINE__,
            hardware->hw2DEngine, hardware->hw3DEngine, hardware->hwVGEngine,
            hardware->fe20, hardware->vg20, hardware->vg21,
            hardware->vg.tesselationPrimitive,
            hardware->byteWrite
            );

        /**/
        /* Return pointer to the gcoVGHARDWARE object. */
        *Hardware = hardware;


#ifdef __QNXNTO__
        {
            gctSIZE_T objSize = gcmSIZEOF(*hardware->pContext);
            gctPHYS_ADDR physical;

            gcmERR_BREAK(gcoOS_AllocateNonPagedMemory(
                gcvNULL, gcvTRUE, &objSize, &physical, (gctPOINTER *) &hardware->pContext));
        }
#endif

        /* Initilize the context management. */
        gcmERR_BREAK(gcoVGHARDWARE_OpenContext(
            hardware
            ));

        /* Construct the command buffer. */
#ifndef __QNXNTO__
        gcmERR_BREAK(gcoVGBUFFER_Construct(
            hal,
            hardware,
            &hardware->context,
            gcmKB2BYTES(32), /* Command buffer ring buffer size. */
            gcmKB2BYTES(8), /* Granularity of the task buffer. */
            1000, /* The length of the command queue. */
            &hardware->buffer
            ));
#else
        gcmERR_BREAK(gcoVGBUFFER_Construct(
            hal,
            hardware,
            hardware->pContext,
            gcmKB2BYTES(32), /* Command buffer ring buffer size. */
            gcmKB2BYTES(8), /* Granularity of the task buffer. */
            1000, /* The length of the command queue. */
            &hardware->buffer
            ));
#endif

        /* Sync filter variables. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(&hardware->horSyncFilterKernel,
                                      gcmSIZEOF(hardware->horSyncFilterKernel)));

        gcmVERIFY_OK(gcoOS_ZeroMemory(&hardware->verSyncFilterKernel,
                                      gcmSIZEOF(hardware->verSyncFilterKernel)));

        hardware->horSyncFilterKernel.filterType = gcvFILTER_SYNC;
        hardware->verSyncFilterKernel.filterType = gcvFILTER_SYNC;

        /* Blur filter variables. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(&hardware->horBlurFilterKernel,
                                      gcmSIZEOF(hardware->horBlurFilterKernel)));

        gcmVERIFY_OK(gcoOS_ZeroMemory(&hardware->verBlurFilterKernel,
                                      gcmSIZEOF(hardware->verBlurFilterKernel)));

        hardware->horBlurFilterKernel.filterType = gcvFILTER_BLUR;
        hardware->verBlurFilterKernel.filterType = gcvFILTER_BLUR;

        /* Filter blit variables. */
        hardware->loadedFilterType  = gcvFILTER_SYNC;
        hardware->loadedKernelSize  = 0;
        hardware->loadedScaleFactor = 0;

        hardware->newFilterType     = gcvFILTER_SYNC;
        hardware->newHorKernelSize  = 9;
        hardware->newVerKernelSize  = 9;

        /* Reset the temporary surface. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            &hardware->tempBuffer, sizeof(hardware->tempBuffer)
            ));

        hardware->tempBuffer.node.pool = gcvPOOL_UNKNOWN;

        hardware->stencilMode    = 0x0;
        hardware->stencilEnabled = gcvFALSE;

        /* Reset stencil array. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            hardware->stencilKeepFront,
            gcmSIZEOF(hardware->stencilKeepFront)
            ));

        gcmVERIFY_OK(gcoOS_ZeroMemory(
            hardware->stencilKeepBack,
            gcmSIZEOF(hardware->stencilKeepBack)
            ));

        /* Reset sampler modes. */
        gcmVERIFY_OK(gcoOS_ZeroMemory(
            hardware->samplerMode,
            gcmSIZEOF(hardware->samplerMode)
            ));

        gcmVERIFY_OK(gcoOS_ZeroMemory(
            hardware->samplerLOD,
            gcmSIZEOF(hardware->samplerLOD)
            ));

        /* Don't stall before primitive. */
        hardware->stallPrimitive = gcvFALSE;
        hardware->earlyDepth     = gcvFALSE;
        hardware->memoryConfig             = 0;

        /* Disable anti-alias. */
        hardware->sampleMask   = 0xF;
        hardware->sampleEnable = 0xF;
        hardware->samples.x    = 1;
        hardware->samples.y    = 1;

        /* Disable dithering. */
        hardware->dither[0] = hardware->dither[1] = (gctUINT32)~0;


        gcmFOOTER();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Roll back. */
    if (hardware != gcvNULL)
    {
        if (hardware->buffer != gcvNULL)
        {
            gcmCHECK_STATUS(gcoVGBUFFER_Destroy(hardware->buffer));
        }

        gcmCHECK_STATUS(gcoVGHARDWARE_CloseContext(hardware));

        gcmCHECK_STATUS(gcoOS_Free(os, hardware));
    }

    gcmFOOTER();
    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_Destroy
**
**  Destroy the gcoVGHARDWARE object.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object that needs to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gcoVGHARDWARE_Destroy(
    IN gcoVGHARDWARE Hardware
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    /* Destroy the command buffer. */
    if (Hardware->buffer != gcvNULL)
    {
        gcmVERIFY_OK(gcoVGBUFFER_Destroy(Hardware->buffer));
    }

    /* Destroy the context management. */
    gcmVERIFY_OK(gcoVGHARDWARE_CloseContext(Hardware));

#ifdef __QNXNTO__
    if (Hardware->pContext != gcvNULL)
    {
        gcoOS_FreeNonPagedMemory(
            gcvNULL, gcmSIZEOF(*Hardware->pContext), gcvNULL, Hardware->pContext);
        Hardware->pContext = gcvNULL;
    }
#endif

    /* Free temporary buffer allocated by filter blit operation. */
    gcmVERIFY_OK(gcoVGHARDWARE_FreeTemporarySurface(Hardware, gcvFALSE));

    /* Mark the gcoVGHARDWARE object as unknown. */
    Hardware->object.type = gcvOBJ_UNKNOWN;

    /* Free the gcoVGHARDWARE object. */
    gcmVERIFY_OK(gcoOS_Free(Hardware->os, Hardware));

    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_QueryChipIdentity
**
**  Query the identity of the hardware.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**  OUTPUT:
**
**      gceCHIPMODEL* ChipModel
**          If 'ChipModel' is not gcvNULL, the variable it points to will
**          receive the model of the chip.
**
**      gctUINT32* ChipRevision
**          If 'ChipRevision' is not gcvNULL, the variable it points to will
**          receive the revision of the chip.
**
**      gctUINT32* ChipFeatures
**          If 'ChipFeatures' is not gcvNULL, the variable it points to will
**          receive the feature set of the chip.
**
**      gctUINT32 * ChipMinorFeatures
**          If 'ChipMinorFeatures' is not gcvNULL, the variable it points to
**          will receive the minor feature set of the chip.
**
**      gctUINT32 * ChipMinorFeatures2
**          If 'ChipMinorFeatures2' is not gcvNULL, the variable it points to
**          will receive the minor feature set #2 of the chip.
**
*/
gceSTATUS gcoVGHARDWARE_QueryChipIdentity(
    IN gcoVGHARDWARE Hardware,
    OUT gceCHIPMODEL* ChipModel,
    OUT gctUINT32* ChipRevision,
    OUT gctUINT32* ChipFeatures,
    OUT gctUINT32* ChipMinorFeatures,
    OUT gctUINT32* ChipMinorFeatures2
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Return chip model. */
    if (ChipModel != gcvNULL)
    {
        *ChipModel = Hardware->chipModel;
    }

    /* Return revision number. */
    if (ChipRevision != gcvNULL)
    {
        *ChipRevision = Hardware->chipRevision;
    }

    /* Return feature set. */
    if (ChipFeatures != gcvNULL)
    {
        *ChipFeatures = Hardware->chipFeatures;
    }

    /* Return minor feature set. */
    if (ChipMinorFeatures != gcvNULL)
    {
        *ChipMinorFeatures = Hardware->chipMinorFeatures;
    }

    /* Return minor feature set. */
    if (ChipMinorFeatures2 != gcvNULL)
    {
        *ChipMinorFeatures2 = Hardware->chipMinorFeatures2;
    }

    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_QueryCommandBuffer
**
**  Query command buffer attributes.
**
**  INPUT:
**
**      gcoVGHARDWARE Harwdare
**          Pointer to the gcoVGHARDWARE object.
**
**  OUTPUT:
**
**      gcsCOMMAND_BUFFER_INFO_PTR Information
**          Pointer to the information structure to receive buffer attributes.
*/
gceSTATUS
gcoVGHARDWARE_QueryCommandBuffer(
    IN gcoVGHARDWARE Hardware,
    OUT gcsCOMMAND_BUFFER_INFO_PTR Information
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Information != gcvNULL);

    /* Copy the command buffer information. */
    status = gcoOS_MemCopy(
        Information,
        &Hardware->bufferInfo,
        gcmSIZEOF(Hardware->bufferInfo)
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslateSourceFormat
**
**  Translate API source color format to its hardware value.
**
**  INPUT:
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslateSourceFormat(
    IN gceSURF_FORMAT APIValue,
    OUT gctUINT32 * HwValue
    )
{
    /* Dispatch on format. */
    switch (APIValue)
    {
    case gcvSURF_INDEX8:
        *HwValue = 0x9;
        break;

    case gcvSURF_X4R4G4B4:
        *HwValue = 0x0;
        break;

    case gcvSURF_A4R4G4B4:
        *HwValue = 0x1;
        break;

    case gcvSURF_X1R5G5B5:
        *HwValue = 0x2;
        break;

    case gcvSURF_A1R5G5B5:
        *HwValue = 0x3;
        break;

    case gcvSURF_R5G6B5:
    case gcvSURF_D16:
        *HwValue = 0x4;
        break;

    case gcvSURF_X8R8G8B8:
        *HwValue = 0x5;
        break;

    case gcvSURF_A8R8G8B8:
    case gcvSURF_D24S8:
        *HwValue = 0x6;
        break;

    case gcvSURF_YUY2:
        *HwValue = 0x7;
        break;

    case gcvSURF_UYVY:
        *HwValue = 0x8;
        break;

    case gcvSURF_YV12:
        *HwValue = 0xF;
        break;

    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslateDestinationFormat
**
**  Translate API destination color format to its hardware value.
**
**  INPUT:
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslateDestinationFormat(
    IN gceSURF_FORMAT APIValue,
    OUT gctUINT32 * HwValue
    )
{
    /* Dispatch on format. */
    switch (APIValue)
    {
    case gcvSURF_X4R4G4B4:
        *HwValue = 0x0;
        break;

    case gcvSURF_A4R4G4B4:
        *HwValue = 0x1;
        break;

    case gcvSURF_X1R5G5B5:
        *HwValue = 0x2;
        break;

    case gcvSURF_A1R5G5B5:
        *HwValue = 0x3;
        break;

    case gcvSURF_R5G6B5:
    case gcvSURF_D16:
        *HwValue = 0x4;
        break;

    case gcvSURF_X8R8G8B8:
        *HwValue = 0x5;
        break;

    case gcvSURF_A8R8G8B8:
    case gcvSURF_D24S8:
        *HwValue = 0x6;
        break;


    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslatePatternFormat
**
**  Translate API pattern color format to its hardware value.
**
**  INPUT:
**
**      gceSURF_FORMAT APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslatePatternFormat(
    IN gceSURF_FORMAT APIValue,
    OUT gctUINT32 * HwValue
    )
{
    /* Dispatch on format. */
    switch (APIValue)
    {
    case gcvSURF_X4R4G4B4:
        *HwValue = 0x0;
        break;

    case gcvSURF_A4R4G4B4:
        *HwValue = 0x1;
        break;

    case gcvSURF_X1R5G5B5:
        *HwValue = 0x2;
        break;

    case gcvSURF_A1R5G5B5:
        *HwValue = 0x3;
        break;

    case gcvSURF_R5G6B5:
    case gcvSURF_D16:
        *HwValue = 0x4;
        break;

    case gcvSURF_X8R8G8B8:
        *HwValue = 0x5;
        break;

    case gcvSURF_A8R8G8B8:
    case gcvSURF_D24S8:
        *HwValue = 0x6;
        break;


    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslateTransparency
**
**  Translate API transparency mode to its hardware value.
**
**  INPUT:
**
**      gceSURF_TRANSPARENCY APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslateTransparency(
    IN gceSURF_TRANSPARENCY APIValue,
    OUT gctUINT32 * HwValue
    )
{
    /* Dispatch on transparency. */
    switch (APIValue)
    {
    case gcvSURF_OPAQUE:
        *HwValue = 0x0;
        break;

    case gcvSURF_SOURCE_MATCH:
        *HwValue = 0x1;
        break;

    case gcvSURF_SOURCE_MASK:
        *HwValue = 0x2;
        break;

    case gcvSURF_PATTERN_MASK:
        *HwValue = 0x3;
        break;

    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslateMonoPack
**
**  Translate API mono packing mode to its hardware value.
**
**  INPUT:
**
**      gceSURF_MONOPACK APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslateMonoPack(
    IN gceSURF_MONOPACK APIValue,
    OUT gctUINT32 * HwValue
    )
{
    /* Dispatch on monochrome packing. */
    switch (APIValue)
    {
    case gcvSURF_PACKED8:
        *HwValue = 0x0;
        break;

    case gcvSURF_PACKED16:
        *HwValue = 0x1;
        break;

    case gcvSURF_PACKED32:
        *HwValue = 0x2;
        break;

    case gcvSURF_UNPACKED:
        *HwValue = 0x3;
        break;

    default:
        /* Not supprted. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslateCommand
**
**  Translate API 2D command to its hardware value.
**
**  INPUT:
**
**      gce2D_COMMAND APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslateCommand(
    IN gce2D_COMMAND APIValue,
    OUT gctUINT32 * HwValue
    )
{
    /* Dispatch on command. */
    switch (APIValue)
    {
    case gcv2D_CLEAR:
        *HwValue = 0x0;
        break;

    case gcv2D_LINE:
        *HwValue = 0x1;
        break;

    case gcv2D_BLT:
        *HwValue = 0x2;
        break;

    case gcv2D_STRETCH:
        *HwValue = 0x4;
        break;

    case gcv2D_HOR_FILTER:
        *HwValue = 0x5;
        break;

    case gcv2D_VER_FILTER:
        *HwValue = 0x6;
        break;

    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslatePixelAlphaMode
**
**  Translate API per-pixel alpha mode to its hardware value.
**
**  INPUT:
**
**      gceSURF_PIXEL_ALPHA_MODE APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslatePixelAlphaMode(
    IN gceSURF_PIXEL_ALPHA_MODE APIValue,
    OUT gctUINT32* HwValue
    )
{
    /* Dispatch on command. */
    switch (APIValue)
    {
    case gcvSURF_PIXEL_ALPHA_STRAIGHT:
        *HwValue = 0x0;
        break;

    case gcvSURF_PIXEL_ALPHA_INVERSED:
        *HwValue = 0x1;
        break;

    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslateGlobalAlphaMode
**
**  Translate API global alpha mode to its hardware value.
**
**  INPUT:
**
**      gceSURF_GLOBAL_ALPHA_MODE APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslateGlobalAlphaMode(
    IN gceSURF_GLOBAL_ALPHA_MODE APIValue,
    OUT gctUINT32* HwValue
    )
{
    /* Dispatch on command. */
    switch (APIValue)
    {
    case gcvSURF_GLOBAL_ALPHA_OFF:
        *HwValue = 0x0;
        break;

    case gcvSURF_GLOBAL_ALPHA_ON:
        *HwValue = 0x1;
        break;

    case gcvSURF_GLOBAL_ALPHA_SCALE:
        *HwValue = 0x2;
        break;

    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslatePixelColorMode
**
**  Translate API per-pixel color mode to its hardware value.
**
**  INPUT:
**
**      gceSURF_PIXEL_COLOR_MODE APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslatePixelColorMode(
    IN gceSURF_PIXEL_COLOR_MODE APIValue,
    OUT gctUINT32* HwValue
    )
{
    /* Dispatch on command. */
    switch (APIValue)
    {
    case gcvSURF_COLOR_STRAIGHT:
        *HwValue = 0x0;
        break;

    case gcvSURF_COLOR_MULTIPLY:
        *HwValue = 0x1;
        break;

    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_TranslateAlphaFactorMode
**
**  Translate API alpha factor mode to its hardware value.
**
**  INPUT:
**
**      gceSURF_BLEND_FACTOR_MODE APIValue
**          API value.
**
**  OUTPUT:
**
**      gctUINT32 * HwValue
**          Corresponding hardware value.
*/
gceSTATUS gcoVGHARDWARE_TranslateAlphaFactorMode(
    IN gceSURF_BLEND_FACTOR_MODE APIValue,
    OUT gctUINT32* HwValue
    )
{
    /* Dispatch on command. */
    switch (APIValue)
    {
    case gcvSURF_BLEND_ZERO:
        *HwValue = 0x0;
        break;

    case gcvSURF_BLEND_ONE:
        *HwValue = 0x1;
        break;

    case gcvSURF_BLEND_STRAIGHT:
        *HwValue = 0x2;
        break;

    case gcvSURF_BLEND_INVERSED:
        *HwValue = 0x3;
        break;

    default:
        /* Not supported. */
        return gcvSTATUS_NOT_SUPPORTED;
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_GetStretchFactor
**
**  Calculate stretch factor.
**
**  INPUT:
**
**      gctINT32 SrcSize
**          The size in pixels of a source dimension (width or height).
**
**      gctINT32 DestSize
**          The size in pixels of a destination dimension (width or height).
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURN:
**
**      gctUINT32
**          Stretch factor in 16.16 fixed point format.
*/
gctUINT32 gcoVGHARDWARE_GetStretchFactor(
    IN gctINT32 SrcSize,
    IN gctINT32 DestSize
    )
{
    gctUINT stretchFactor;

    if ((SrcSize > 0) && (DestSize > 1))
    {
        stretchFactor = ((SrcSize - 1) << 16) / (DestSize - 1);
    }
    else
    {
        stretchFactor = 0;
    }

    return stretchFactor;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_GetStretchFactors
**
**  Calculate the stretch factors.
**
**  INPUT:
**
**      gcsRECT_PTR SrcRect
**          Pointer to a valid source rectangles.
**
**      gcsRECT_PTR DestRect
**          Pointer to a valid destination rectangles.
**
**  OUTPUT:
**
**      gctUINT32 * HorFactor
**          Pointer to a variable that will receive the horizontal stretch
**          factor.
**
**      gctUINT32 * VerFactor
**          Pointer to a variable that will receive the vertical stretch factor.
*/
gceSTATUS gcoVGHARDWARE_GetStretchFactors(
    IN gcsRECT_PTR SrcRect,
    IN gcsRECT_PTR DestRect,
    OUT gctUINT32 * HorFactor,
    OUT gctUINT32 * VerFactor
    )
{
    if (HorFactor != gcvNULL)
    {
        gctINT32 src, dest;

        /* Compute width of rectangles. */
        gcmVERIFY_OK(gcsRECT_Width(SrcRect, &src));
        gcmVERIFY_OK(gcsRECT_Width(DestRect, &dest));

        /* Compute and return horizontal stretch factor. */
        *HorFactor = gcoVGHARDWARE_GetStretchFactor(src, dest);
    }

    if (VerFactor != gcvNULL)
    {
        gctINT32 src, dest;

        /* Compute height of rectangles. */
        gcmVERIFY_OK(gcsRECT_Height(SrcRect, &src));
        gcmVERIFY_OK(gcsRECT_Height(DestRect, &dest));

        /* Compute and return vertical stretch factor. */
        *VerFactor = gcoVGHARDWARE_GetStretchFactor(src, dest);
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_SetStretchFactors
**
**  Calculate and program the stretch factors.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT32 HorFactor
**          Horizontal stretch factor.
**
**      gctUINT32 VerFactor
**          Vertical stretch factor.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gcoVGHARDWARE_SetStretchFactors(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT32 HorFactor,
    IN gctUINT32 VerFactor
    )
{
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_EnableAlphaBlend
**
**  Enable alpha blending engine in the hardware and disengage the ROP engine.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT8 SrcGlobalAlphaValue
**      gctUINT8 DstGlobalAlphaValue
**          Global alpha value for the color components.
**
**      gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode
**      gceSURF_PIXEL_ALPHA_MODE DstAlphaMode
**          Per-pixel alpha component mode.
**
**      gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode
**      gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode
**          Global/per-pixel alpha values selection.
**
**      gceSURF_BLEND_FACTOR_MODE SrcFactorMode
**      gceSURF_BLEND_FACTOR_MODE DstFactorMode
**          Final blending factor mode.
**
**      gceSURF_PIXEL_COLOR_MODE SrcColorMode
**      gceSURF_PIXEL_COLOR_MODE DstColorMode
**          Per-pixel color component mode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gcoVGHARDWARE_EnableAlphaBlend(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT8 SrcGlobalAlphaValue,
    IN gctUINT8 DstGlobalAlphaValue,
    IN gceSURF_PIXEL_ALPHA_MODE SrcAlphaMode,
    IN gceSURF_PIXEL_ALPHA_MODE DstAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE SrcGlobalAlphaMode,
    IN gceSURF_GLOBAL_ALPHA_MODE DstGlobalAlphaMode,
    IN gceSURF_BLEND_FACTOR_MODE SrcFactorMode,
    IN gceSURF_BLEND_FACTOR_MODE DstFactorMode,
    IN gceSURF_PIXEL_COLOR_MODE SrcColorMode,
    IN gceSURF_PIXEL_COLOR_MODE DstColorMode
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_DisableAlphaBlend
**
**  Disable alpha blending engine in the hardware and engage the ROP engine.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS gcoVGHARDWARE_DisableAlphaBlend(
    IN gcoVGHARDWARE Hardware
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_LoadState
**
**  Load a state buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT32 Address
**          Starting register address of the state buffer.
**
**      gctUINT32 Count
**          Number of states in state buffer.
**
**      gctPOINTER Data
**          Pointer to state buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_LoadState(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctSIZE_T Count,
    IN gctPOINTER Data
    )
{
    gcmASSERT(gcvFALSE);
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_LoadState32
**
**  Load one 32-bit state.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT32 Address
**          Register address of the state.
**
**      gctUINT32 Data
**          Value of the state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_LoadState32(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT32 Data
    )
{
    gcmASSERT(gcvFALSE);
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_LoadState64
**
**  Load one 64-bit state.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT32 Address
**          Register address of the state.
**
**      gctUINT64 Data
**          Value of the state.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_LoadState64(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT32 Address,
    IN gctUINT64 Data
    )
{
    gcmASSERT(gcvFALSE);
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_Commit
**
**  Commit the current command buffer to the hardware.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
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
gcoVGHARDWARE_Commit(
    IN gcoVGHARDWARE Hardware,
    IN gctBOOL Stall
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Commit the current command buffer. */
    status = gcoVGBUFFER_Commit(Hardware->buffer, Stall);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGHARDWARE_Lock
**
**  Lock video memory.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to a gcoVGHARDWARE object.
**
**      gcsSURF_NODE_PTR Node
**          Pointer to a gcsSURF_NODE structure that describes the video
**          memory to lock.
**
**  OUTPUT:
**
**      gctUINT32 * Address
**          Physical address of the surface.
**
**      gctPOINTER * Memory
**          Logical address of the surface.
*/
gceSTATUS
gcoVGHARDWARE_Lock(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_NODE_PTR Node,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    do
    {
        if (Node->valid)
        {
            status = gcvSTATUS_OK;
        }
        else
        {
            gcsHAL_INTERFACE halInterface;

            /* User pools have to be mapped first. */
            if (Node->pool == gcvPOOL_USER)
            {
                status = gcvSTATUS_MEMORY_UNLOCKED;
                break;
            }

            /* Fill in the kernel call structure. */
            halInterface.command = gcvHAL_LOCK_VIDEO_MEMORY;
            halInterface.u.LockVideoMemory.node = Node->u.normal.node;
            halInterface.u.LockVideoMemory.cacheable = gcvFALSE;

            /* Call the kernel. */
            gcmERR_BREAK(gcoOS_DeviceControl(
                gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &halInterface, sizeof(halInterface),
                &halInterface, sizeof(halInterface)
                ));

            /* Success? */
            gcmERR_BREAK(halInterface.status);

            /* Validate the node. */
            Node->valid = gcvTRUE;

            /* Store pointers. */
            Node->physical = halInterface.u.LockVideoMemory.address;
            Node->logical  = halInterface.u.LockVideoMemory.memory;

            /* Set locked in the kernel flag. */
            Node->lockedInKernel = gcvTRUE;
        }

        /* Increment the lock count. */
        Node->lockCount++;

        /* Set the result. */
        if (Address != gcvNULL)
        {
            *Address = Node->physical;
        }

        if (Memory != gcvNULL)
        {
            *Memory = Node->logical;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_Unlock
**
**  Unlock video memory.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gcsSURF_NODE_PTR Node
**          Pointer to a gcsSURF_NODE structure that describes the video
**          memory to unlock.
**
**      gceSURF_TYPE Type
**          Type of surface to unlock.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_Unlock(
    IN gcoVGHARDWARE Hardware,
    IN gcsSURF_NODE_PTR Node,
    IN gceSURF_TYPE Type
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware = 0x%x", Hardware);
    do
    {
        gcsHAL_INTERFACE halInterface;

        /* Verify whether the node is valid. */
        if (!Node->valid || (Node->lockCount == 0))
        {
            gcmTRACE_ZONE(
                gcvLEVEL_WARNING, gcvZONE_SURFACE,
                "gcoVGHARDWARE_Unlock: Node=%p; unlock called on an unlocked surface.\n",
                Node
                );

            status = gcvSTATUS_OK;
            break;
        }

        /* Locked more then once? */
        if (Node->lockCount > 1)
        {
            /* Decrement the lock count. */
            Node->lockCount--;

            /* Success. */
            status = gcvSTATUS_OK;
            break;
        }

        /* User mapped surfaces don't need to be unlocked. */
        if (Node->pool == gcvPOOL_USER)
        {
            /* Reset the count and leave the node as valid. */
            Node->lockCount = 0;

            /* Success. */
            status = gcvSTATUS_OK;
            break;
        }

        if (Node->lockedInKernel)
        {
            /* If we need to unlock a node from virtual memory we have to be
               very carefull.  If the node is still inside the caches we
               might get a bus error later if the cache line needs to be
               replaced.  So - we have to flush the caches before we do
               anything.  We also need to stall to make sure the flush has
               happened. */

            if (Node->pool == gcvPOOL_VIRTUAL)
            {
                gctUINT32 flush;

                /* 2D surface? */
                if (Type == gcvSURF_BITMAP)
                {
                    /* Flush 2D cache. */
                    flush = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)));
                }

                /* 3D render target? */
                else if (Type == gcvSURF_RENDER_TARGET)
                {
                    /* Flush Color cache. */
                    flush = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)));
                }

                /* Depth buffer? */
                else if (Type == gcvSURF_DEPTH)
                {
                    /* Flush depth cache. */
                    flush = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)));
                }

                else
                {
                    /* No cache to flush otherwise. */
                    flush = 0;
                }

                /* Anything to flush? */
                if (flush != 0)
                {
                    /* Flush proper cache. */
                    gcmERR_BREAK(gcoVGHARDWARE_LoadState32(
                        Hardware,
                        0x0380C,
                        flush
                        ));

                    /* Commit the command queue and stall. */
                    gcmERR_BREAK(gcoVGHARDWARE_Commit(Hardware, gcvTRUE));
                }
            }

            /* Unlock the video memory node. */
            halInterface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
            halInterface.u.UnlockVideoMemory.node = Node->u.normal.node;
            halInterface.u.UnlockVideoMemory.type = Type;

            /* Call the kernel. */
            gcmERR_BREAK(gcoOS_DeviceControl(
                gcvNULL,
                IOCTL_GCHAL_INTERFACE,
                &halInterface, gcmSIZEOF(halInterface),
                &halInterface, gcmSIZEOF(halInterface)
                ));

            /* Verify result. */
            gcmERR_BREAK(halInterface.status);

            /* Reset locked in the kernel flag. */
            Node->lockedInKernel = gcvFALSE;
        }
        else
        {
            /* Success. */
            status = gcvSTATUS_OK;
        }

        /* Reset the valid flag. */
        if (Node->pool == gcvPOOL_VIRTUAL)
        {
            Node->valid = gcvFALSE;
        }

        /* Reset the lock count. */
        Node->lockCount = 0;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_ReserveTask
**
**  Allocate a task to be perfomed when signaled from the specified block.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
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
gcoVGHARDWARE_ReserveTask(
    IN gcoVGHARDWARE Hardware,
    IN gceBLOCK Block,
    IN gctUINT TaskCount,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Append the event to the event queue. */
    status = gcoVGBUFFER_ReserveTask(
        Hardware->buffer, Block, TaskCount, Bytes, Memory
        );

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_AllocateVideoMemory
**
**  Allocate and lock linear video memory. If both Address and Memory parameters
**  are given as gcvNULL, the memory will not be locked, only allocated.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctUINT Width
**          Width of surface to create in pixels.
**
**      gctUINT Height
**          Height of surface to create in lines.
**
**      gctUINT Depth
**          Depth of surface to create in planes.
**
**      gceSURF_TYPE Type
**          Type of surface to create.
**
**      gceSURF_FORMAT Format
**          Format of surface to create.
**
**      gcePOOL Pool
**          Pool to allocate surface from.
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
**
**      gcePOOL * ActualPool
**          Pointer to the variable to receive the pool where the surface was
**          allocated.  Can be gcvNULL.
*/
gceSTATUS
gcoVGHARDWARE_AllocateVideoMemory(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gctUINT Depth,
    IN gceSURF_TYPE Type,
    IN gceSURF_FORMAT Format,
    IN gcePOOL Pool,
    OUT gcuVIDMEM_NODE_PTR * Node,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory,
    OUT gcePOOL * ActualPool
    )
{
    gceSTATUS status, last;
    gcsHAL_INTERFACE halInterface;
    gcuVIDMEM_NODE_PTR node = gcvNULL;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Node != gcvNULL);

    do
    {
        gcePOOL pool;

        /* Call to allocate the buffer. */
        halInterface.command = gcvHAL_ALLOCATE_VIDEO_MEMORY;
        halInterface.u.AllocateVideoMemory.width  = Width;
        halInterface.u.AllocateVideoMemory.height = Height;
        halInterface.u.AllocateVideoMemory.depth  = Depth;
        halInterface.u.AllocateVideoMemory.format = Format;
        halInterface.u.AllocateVideoMemory.pool   = Pool;
        halInterface.u.AllocateVideoMemory.type   = Type;

        /* Call kernel service. */
        gcmERR_BREAK(gcoOS_DeviceControl(
            Hardware->os, IOCTL_GCHAL_INTERFACE,
            &halInterface, sizeof(gcsHAL_INTERFACE),
            &halInterface, sizeof(gcsHAL_INTERFACE)
            ));

        /* Validate the return value. */
        gcmERR_BREAK(halInterface.status);

        /* Get the node and pool values. */
        node = halInterface.u.AllocateVideoMemory.node;
        pool = halInterface.u.AllocateVideoMemory.pool;

        /* Do we need to lock the buffer as well? */
        if ((Address != gcvNULL) || (Memory != gcvNULL))
        {
            /* Lock the buffer. */
            halInterface.command = gcvHAL_LOCK_VIDEO_MEMORY;
            halInterface.u.LockVideoMemory.node = node;
            halInterface.u.LockVideoMemory.cacheable = gcvFALSE;

            /* Call kernel service. */
            gcmERR_BREAK(gcoOS_DeviceControl(
                Hardware->os, IOCTL_GCHAL_INTERFACE,
                &halInterface, sizeof(gcsHAL_INTERFACE),
                &halInterface, sizeof(gcsHAL_INTERFACE)
                ));

            /* Validate the return value. */
            gcmERR_BREAK(halInterface.status);

            /* Set return pointers. */
            if (Address != gcvNULL)
            {
                * Address = halInterface.u.LockVideoMemory.address;
            }

            if (Memory != gcvNULL)
            {
                * Memory = halInterface.u.LockVideoMemory.memory;
            }
        }

        /* Set return node. */
        * Node = node;

        /* Set the actual pool. */
        if (ActualPool != gcvNULL)
        {
            * ActualPool = pool;
        }

        gcmFOOTER();
        /* Success. */
        return status;
    }
    while (gcvFALSE);

    /* Roll back. */
    if (node != gcvNULL)
    {
        halInterface.command = gcvHAL_FREE_VIDEO_MEMORY;
        halInterface.u.FreeVideoMemory.node = node;

        /* Call kernel service. */
        gcmCHECK_STATUS(gcoOS_DeviceControl(
            Hardware->os, IOCTL_GCHAL_INTERFACE,
            &halInterface, sizeof(gcsHAL_INTERFACE),
            &halInterface, sizeof(gcsHAL_INTERFACE)
            ));

        /* Validate the return value. */
        gcmCHECK_STATUS(halInterface.status);
    }

    gcmFOOTER();
    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_AllocateLinearVideoMemory
**
**  Allocate and lock linear video memory. If both Address and Memory parameters
**  are given as gcvNULL, the memory will not be locked, only allocated.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
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
gcoVGHARDWARE_AllocateLinearVideoMemory(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT Size,
    IN gctUINT Alignment,
    IN gcePOOL Pool,
    OUT gcuVIDMEM_NODE_PTR * Node,
    OUT gctUINT32 * Address,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS status, last;
    gcsHAL_INTERFACE halInterface;
    gcuVIDMEM_NODE_PTR node = gcvNULL;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Node != gcvNULL);

    do
    {
        /* Call to allocate the buffer. */
        halInterface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
        halInterface.u.AllocateLinearVideoMemory.bytes     = Size;
        halInterface.u.AllocateLinearVideoMemory.alignment = Alignment;
        halInterface.u.AllocateLinearVideoMemory.type      = gcvSURF_TYPE_UNKNOWN;
        halInterface.u.AllocateLinearVideoMemory.pool      = Pool;

        /* Call kernel service. */
        gcmERR_BREAK(gcoOS_DeviceControl(
            Hardware->os, IOCTL_GCHAL_INTERFACE,
            &halInterface, sizeof(gcsHAL_INTERFACE),
            &halInterface, sizeof(gcsHAL_INTERFACE)
            ));

        /* Validate the return value. */
        gcmERR_BREAK(halInterface.status);

        /* Get the node value. */
        node = halInterface.u.AllocateLinearVideoMemory.node;

        /* Do we need to lock the buffer as well? */
        if ((Address != gcvNULL) || (Memory != gcvNULL))
        {
            /* Lock the buffer. */
            halInterface.command = gcvHAL_LOCK_VIDEO_MEMORY;
            halInterface.u.LockVideoMemory.node = node;
            halInterface.u.LockVideoMemory.cacheable = gcvFALSE;

            /* Call kernel service. */
            gcmERR_BREAK(gcoOS_DeviceControl(
                gcvNULL, IOCTL_GCHAL_INTERFACE,
                &halInterface, sizeof(gcsHAL_INTERFACE),
                &halInterface, sizeof(gcsHAL_INTERFACE)
                ));

            /* Validate the return value. */
            gcmERR_BREAK(halInterface.status);

            /* Set return pointers. */
            if (Address != gcvNULL)
            {
                * Address = halInterface.u.LockVideoMemory.address;
            }

            if (Memory != gcvNULL)
            {
                * Memory = halInterface.u.LockVideoMemory.memory;
            }
        }

        /* Set return node. */
        * Node = node;

        gcmFOOTER_ARG("*Node=0x%x", *Node);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Roll back. */
    if (node != gcvNULL)
    {
        halInterface.command = gcvHAL_FREE_VIDEO_MEMORY;
        halInterface.u.FreeVideoMemory.node = node;

        /* Call kernel service. */
        gcmCHECK_STATUS(gcoOS_DeviceControl(
            Hardware->os, IOCTL_GCHAL_INTERFACE,
            &halInterface, sizeof(gcsHAL_INTERFACE),
            &halInterface, sizeof(gcsHAL_INTERFACE)
            ));

        /* Validate the return value. */
        gcmCHECK_STATUS(halInterface.status);
    }

    gcmFOOTER();
    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_FreeVideoMemory
**
**  Free linear video memory allocated with gcoVGHARDWARE_AllocateLinearVideoMemory.
**  Assumed that the memory is locked.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gcuVIDMEM_NODE_PTR Node
**          Node of the allocated memory to free.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_FreeVideoMemory(
    IN gcoVGHARDWARE Hardware,
    IN gcuVIDMEM_NODE_PTR Node
    )
{
    gceSTATUS status;
    gcsHAL_INTERFACE halInterface;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Node != gcvNULL);

    do
    {
        /* Unlock the buffer. */
        halInterface.command = gcvHAL_UNLOCK_VIDEO_MEMORY;
        halInterface.u.UnlockVideoMemory.node = Node;
        halInterface.u.UnlockVideoMemory.type = gcvSURF_TYPE_UNKNOWN;

        /* Call kernel service. */
        gcmERR_BREAK(gcoOS_DeviceControl(
            Hardware->os, IOCTL_GCHAL_INTERFACE,
            &halInterface, sizeof(gcsHAL_INTERFACE),
            &halInterface, sizeof(gcsHAL_INTERFACE)
            ));

        /* Validate the return value. */
        gcmERR_BREAK(halInterface.status);

        /* Free the buffer. */
        halInterface.command = gcvHAL_FREE_VIDEO_MEMORY;
        halInterface.u.FreeVideoMemory.node = Node;

        /* Call kernel service. */
        gcmERR_BREAK(gcoOS_DeviceControl(
            Hardware->os, IOCTL_GCHAL_INTERFACE,
            &halInterface, sizeof(gcsHAL_INTERFACE),
            &halInterface, sizeof(gcsHAL_INTERFACE)
            ));

        /* Validate the return value. */
        gcmERR_BREAK(halInterface.status);
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_ScheduleVideoMemory
**
**  Schedule to free linear video memory allocated with
**  gcoVGHARDWARE_AllocateLinearVideoMemory. Assumed that the memory is locked.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gcuVIDMEM_NODE_PTR Node
**          Node of the allocated memory to free.
**
**      gctBOOL Unlock
**          The memory will be first unlocked if not zero.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_ScheduleVideoMemory(
    IN gcoVGHARDWARE Hardware,
    IN gcuVIDMEM_NODE_PTR Node,
    IN gctBOOL Unlock
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Node != gcvNULL);

    do
    {
        gcsTASK_UNLOCK_VIDEO_MEMORY_PTR unlockMemory;
        gcsTASK_FREE_VIDEO_MEMORY_PTR freeMemory;

        if (Unlock)
        {
            /* Allocate a cluster event. */
            gcmERR_BREAK(gcoVGHARDWARE_ReserveTask(
                Hardware,
                gcvBLOCK_PIXEL,
                2,
                gcmSIZEOF(gcsTASK_UNLOCK_VIDEO_MEMORY) +
                gcmSIZEOF(gcsTASK_FREE_VIDEO_MEMORY),
                (gctPOINTER *) &unlockMemory
                ));

            /* Determine the free memory task pointer. */
            freeMemory = (gcsTASK_FREE_VIDEO_MEMORY_PTR) (unlockMemory + 1);

            /* Fill in event info. */
            unlockMemory->id   = gcvTASK_UNLOCK_VIDEO_MEMORY;
            unlockMemory->node = Node;
        }
        else
        {
            /* Allocate a cluster event. */
            gcmERR_BREAK(gcoVGHARDWARE_ReserveTask(
                Hardware,
                gcvBLOCK_PIXEL,
                1,
                gcmSIZEOF(gcsTASK_FREE_VIDEO_MEMORY),
                (gctPOINTER *) &freeMemory
                ));
        }

        /* Fill in event info. */
        freeMemory->id   = gcvTASK_FREE_VIDEO_MEMORY;
        freeMemory->node = Node;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_AllocateTemporarySurface
**
**  Allocates a temporary surface with specified parameters.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to a gcoVGHARDWARE object.
**
**      gctUINT Width, Height
**          The aligned size of the surface to be allocated.
**
**      gcsSURF_FORMAT_INFO_PTR Format
**          The format of the surface to be allocated.
**
**      gceSURF_TYPE Type
**          The type of the surface to be allocated.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_AllocateTemporarySurface(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT Width,
    IN gctUINT Height,
    IN gcsSURF_FORMAT_INFO_PTR Format,
    IN gceSURF_TYPE Type
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        gctUINT32 alignedWidth;
        gctUINT32 alignedHeight;
        gctUINT stride, size;

        /* Set the initial aligned size. */
        alignedWidth  = Width;
        alignedHeight = Height;

        /* Aligned the requested size. */
        gcmERR_BREAK(gcoVGHARDWARE_AlignToTile(
            Hardware, Type, &alignedWidth, &alignedHeight
            ));

        /* Compute the stride and the required size of the surface. */
        stride = (alignedWidth * Format->bitsPerPixel) / 8;
        size   = stride * alignedHeight;

        /* Do we already have a big enough surface? */
        if (size > Hardware->tempBufferSize)
        {
            gcsHAL_INTERFACE halInterface;

            /* No. Free the currently allocated surface if any. */
            gcmERR_BREAK(gcoVGHARDWARE_FreeTemporarySurface(Hardware, gcvTRUE));

            /* Align the size to 4K. */
            size = gcmALIGN(size, 4096);

            /* Call to allocate the buffer. */
            halInterface.command = gcvHAL_ALLOCATE_LINEAR_VIDEO_MEMORY;
            halInterface.u.AllocateLinearVideoMemory.bytes     = size;
            halInterface.u.AllocateLinearVideoMemory.alignment = 64;
            halInterface.u.AllocateLinearVideoMemory.type      = Type;
            halInterface.u.AllocateLinearVideoMemory.pool      = gcvPOOL_DEFAULT;

            /* Call kernel service. */
            gcmERR_BREAK(gcoOS_DeviceControl(
                Hardware->os, IOCTL_GCHAL_INTERFACE,
                &halInterface, sizeof(gcsHAL_INTERFACE),
                &halInterface, sizeof(gcsHAL_INTERFACE)
                ));

            /* Validate the return value. */
            gcmERR_BREAK(halInterface.status);

            /* Initialize the node. */
            Hardware->tempBuffer.node.valid          = gcvFALSE;
            Hardware->tempBuffer.node.lockCount      = 0;
            Hardware->tempBuffer.node.lockedInKernel = gcvFALSE;
            Hardware->tempBuffer.node.logical        = gcvNULL;
            Hardware->tempBuffer.node.physical       = (gctUINT32)~0;
            Hardware->tempBuffer.node.pool
                = halInterface.u.AllocateLinearVideoMemory.pool;
            Hardware->tempBuffer.node.u.normal.node
                = halInterface.u.AllocateLinearVideoMemory.node;

            /* Set the buffer size. */
            Hardware->tempBufferSize = size;

            /* Lock the temporary surface. */
            gcmERR_BREAK(gcoVGHARDWARE_Lock(
                Hardware,
                &Hardware->tempBuffer.node,
                gcvNULL,
                gcvNULL
                ));
        }

        /* Set the new parameters. */
        Hardware->tempBuffer.type   = Type;
        Hardware->tempBuffer.format = Format->format;
        Hardware->tempBuffer.stride = stride;

        Hardware->tempBuffer.samples.x = 1;
        Hardware->tempBuffer.samples.y = 1;

        Hardware->tempBuffer.rect.left   = 0;
        Hardware->tempBuffer.rect.top    = 0;
        Hardware->tempBuffer.rect.right  = Width;
        Hardware->tempBuffer.rect.bottom = Height;

        Hardware->tempBuffer.alignedWidth  = alignedWidth;
        Hardware->tempBuffer.alignedHeight = alignedHeight;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_FreeTemporarySurface
**
**  Free the temporary surface.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to a gcoVGHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_FreeTemporarySurface(
    IN gcoVGHARDWARE Hardware,
    IN gctBOOL Synchronized
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    do
    {
        /* Is there a surface allocated? */
        if (Hardware->tempBuffer.node.pool == gcvPOOL_UNKNOWN)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Schedule deletion. */
        if (Synchronized)
        {
            gcmERR_BREAK(gcoVGHARDWARE_ScheduleVideoMemory(
                Hardware,
                Hardware->tempBuffer.node.u.normal.node,
                gcvTRUE
                ));
        }

        /* Not synchronized --> delete immediately. */
        else
        {
            gcsHAL_INTERFACE halInterface;

            /* Unlock the temporary surface. */
            gcmERR_BREAK(gcoVGHARDWARE_Unlock(
                Hardware,
                &Hardware->tempBuffer.node,
                 Hardware->tempBuffer.type
                ));

            /* Free the video memory. */
            halInterface.command = gcvHAL_FREE_VIDEO_MEMORY;
            halInterface.u.FreeVideoMemory.node
                = Hardware->tempBuffer.node.u.normal.node;

            /* Call kernel API. */
            gcmERR_BREAK(gcoHAL_Call(Hardware->hal, &halInterface));
        }

        /* Reset the temporary surface. */
        Hardware->tempBufferSize = 0;
        gcoOS_ZeroMemory(&Hardware->tempBuffer, sizeof(Hardware->tempBuffer));
        Hardware->tempBuffer.node.pool = gcvPOOL_UNKNOWN;
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/******************************************************************************\
********************************* Support Code *********************************
\******************************************************************************/

static gctUINT32 _SetBitWidth(
    gctUINT32 Value,
    gctINT8 CurWidth,
    gctINT8 NewWidth
    )
{
    gctUINT32 result;
    gctINT8 widthDiff;

    /* Mask source bits. */
    Value &= ((gctUINT64) 1 << CurWidth) - 1;

    /* Init the result. */
    result = Value;

    /* Determine the difference in width. */
    widthDiff = NewWidth - CurWidth;

    /* Until the difference is not zero... */
    while (widthDiff)
    {
        /* New value is thiner then current? */
        if (widthDiff < 0)
        {
            result >>= -widthDiff;
            widthDiff = 0;
        }

        /* Full source replication? */
        else if (widthDiff >= CurWidth)
        {
            result = (result << CurWidth) | Value;
            widthDiff -= CurWidth;
        }

        /* Partial source replication. */
        else
        {
            result = (result << widthDiff) | (Value >> (CurWidth - widthDiff));
            widthDiff = 0;
        }
    }

    /* Return result. */
    return result;
}

static void _ConvertComponent(
    gctUINT8* SrcPixel,
    gctUINT8* TrgPixel,
    gctUINT SrcBit,
    gctUINT TrgBit,
    gcsFORMAT_COMPONENT* SrcComponent,
    gcsFORMAT_COMPONENT* TrgComponent,
    gcsBOUNDARY_PTR SrcBoundary,
    gcsBOUNDARY_PTR TrgBoundary,
    gctUINT32 Default
    )
{
    gctUINT32 srcValue;
    gctUINT8 srcWidth;
    gctUINT8 trgWidth;
    gctUINT32 trgMask;
    gctUINT32 bits;

    /* Exit if target is beyond the boundary. */
    if ((TrgBoundary != gcvNULL) &&
        ((TrgBoundary->x < 0) || (TrgBoundary->x >= TrgBoundary->width) ||
         (TrgBoundary->y < 0) || (TrgBoundary->y >= TrgBoundary->height)))
    {
        return;
    }

    /* Exit if target component is not present. */
    if ((TrgComponent->width & gcvCOMPONENT_DONTCARE) != 0)
    {
        return;
    }

    /* Extract target width. */
    trgWidth = TrgComponent->width & gcvCOMPONENT_WIDTHMASK;

    /* Extract the source. */
    if ((SrcComponent == gcvNULL) ||
        ((SrcComponent->width & gcvCOMPONENT_DONTCARE) != 0) ||
        ((SrcBoundary != gcvNULL) &&
         ((SrcBoundary->x < 0) || (SrcBoundary->x >= SrcBoundary->width) ||
          (SrcBoundary->y < 0) || (SrcBoundary->y >= SrcBoundary->height))))
    {
        srcValue = Default;
        srcWidth = 32;
    }
    else
    {
        /* Extract source width. */
        srcWidth = SrcComponent->width & gcvCOMPONENT_WIDTHMASK;

        /* Compute source position. */
        SrcBit += SrcComponent->start;
        SrcPixel += SrcBit >> 3;
        SrcBit &= 7;

        /* Compute number of bits to read from source. */
        bits = SrcBit + srcWidth;

        /* Read the value. */
        srcValue = SrcPixel[0] >> SrcBit;

        if (bits > 8)
        {
            /* Read up to 16 bits. */
            srcValue |= SrcPixel[1] << (8 - SrcBit);
        }

        if (bits > 16)
        {
            /* Read up to 24 bits. */
            srcValue |= SrcPixel[2] << (16 - SrcBit);
        }

        if (bits > 24)
        {
            /* Read up to 32 bits. */
            srcValue |= SrcPixel[3] << (24 - SrcBit);
        }
    }

    /* Make the source component the same width as the target. */
    srcValue = _SetBitWidth(srcValue, srcWidth, trgWidth);

    /* Compute destination position. */
    TrgBit += TrgComponent->start;
    TrgPixel += TrgBit >> 3;
    TrgBit &= 7;

    /* Determine the target mask. */
    trgMask = (gctUINT32) (((gctUINT64) 1 << trgWidth) - 1);
    trgMask <<= TrgBit;

    /* Align the source value. */
    srcValue <<= TrgBit;

    /* Loop while there are bits to set. */
    while (trgMask != 0)
    {
        /* Set 8 bits of the pixel value. */
        if ((trgMask & 0xFF) == 0xFF)
        {
            /* Set all 8 bits. */
            *TrgPixel = (gctUINT8) srcValue;
        }
        else
        {
            /* Set the required bits. */
            *TrgPixel = (gctUINT8) ((*TrgPixel & ~trgMask) | srcValue);
        }

        /* Next 8 bits. */
        TrgPixel ++;
        trgMask  >>= 8;
        srcValue >>= 8;
    }
}


static gcmINLINE gctUINT32
_GetPipe(
    IN gcoVGHARDWARE Hardware
    )
{
    static gctUINT32 pipe[] =
    {
        0x1,
        0x2,
        0x0
    };

    /* Determine the pipe switch value. */
    return pipe[2];
}


/*******************************************************************************
**
**  gcoVGHARDWARE_ConvertPixel
**
**  Convert source pixel from source format to target pixel' target format.
**  The source format class should be either identical or convertible to
**  the target format class.
**
**  INPUT:
**
**      gctPOINTER SrcPixel, TrgPixel,
**          Pointers to source and target pixels.
**
**      gctUINT SrcBitOffset, TrgBitOffset
**          Bit offsets of the source and target pixels relative to their
**          respective pointers.
**
**      gcsSURF_FORMAT_INFO_PTR SrcFormat, TrgFormat
**          Pointers to source and target pixel format descriptors.
**
**      gcsBOUNDARY_PTR SrcBoundary, TrgBoundary
**          Pointers to optional boundary structures to verify the source
**          and target position. If the source is found to be beyond the
**          defined boundary, it will be assumed to be 0. If the target
**          is found to be beyond the defined boundary, the write will
**          be ignored. If boundary checking is not needed, gcvNULL can be
**          passed.
**
**  OUTPUT:
**
**      gctPOINTER TrgPixel + TrgBitOffset
**          Converted pixel.
*/
gceSTATUS gcoVGHARDWARE_ConvertPixel(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER SrcPixel,
    OUT gctPOINTER TrgPixel,
    IN gctUINT SrcBitOffset,
    IN gctUINT TrgBitOffset,
    IN gcsSURF_FORMAT_INFO_PTR SrcFormat,
    IN gcsSURF_FORMAT_INFO_PTR TrgFormat,
    IN gcsBOUNDARY_PTR SrcBoundary,
    IN gcsBOUNDARY_PTR TrgBoundary
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (SrcFormat->fmtClass == gcvFORMAT_CLASS_RGBA)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_RGBA)
        {
            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.alpha,
                &TrgFormat->u.rgba.alpha,
                SrcBoundary, TrgBoundary,
                (gctUINT32)~0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.red,
                &TrgFormat->u.rgba.red,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.green,
                &TrgFormat->u.rgba.green,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.blue,
                &TrgFormat->u.rgba.blue,
                SrcBoundary, TrgBoundary,
                0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else if (TrgFormat->fmtClass == gcvFORMAT_CLASS_LUMINANCE)
        {
            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.red,
                &TrgFormat->u.lum.value,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.rgba.alpha,
                &TrgFormat->u.lum.alpha,
                SrcBoundary, TrgBoundary,
                (gctUINT32)~0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else if (TrgFormat->fmtClass == gcvFORMAT_CLASS_YUV)
        {
            gctUINT8 r, g, b;
            gctUINT8 y, u, v;

            /*
                Get RGB value.
            */

            _ConvertComponent(
                SrcPixel, &r, SrcBitOffset, 0,
                &SrcFormat->u.rgba.red, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                );

            _ConvertComponent(
                SrcPixel, &g, SrcBitOffset, 0,
                &SrcFormat->u.rgba.green, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                );

            _ConvertComponent(
                SrcPixel, &b, SrcBitOffset, 0,
                &SrcFormat->u.rgba.blue, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                );

            /*
                Convert to YUV.
            */

            gcoVGHARDWARE_RGB2YUV(
                 r, g, b,
                &y, &u, &v
                );

            /*
                Average Us and Vs for odd pixels.
            */
            if ((TrgFormat->interleaved & gcvCOMPONENT_ODD) != 0)
            {
                gctUINT8 curU, curV;

                _ConvertComponent(
                    TrgPixel, &curU, TrgBitOffset, 0,
                    &TrgFormat->u.yuv.u, &gcvPIXEL_COMP_XXX8,
                    TrgBoundary, gcvNULL, 0
                    );

                _ConvertComponent(
                    TrgPixel, &curV, TrgBitOffset, 0,
                    &TrgFormat->u.yuv.v, &gcvPIXEL_COMP_XXX8,
                    TrgBoundary, gcvNULL, 0
                    );


                u = (gctUINT8) (((gctUINT16) u + (gctUINT16) curU) >> 1);
                v = (gctUINT8) (((gctUINT16) v + (gctUINT16) curV) >> 1);
            }

            /*
                Convert to the final format.
            */

            _ConvertComponent(
                &y, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.yuv.y,
                gcvNULL, TrgBoundary, 0
                );

            _ConvertComponent(
                &u, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.yuv.u,
                gcvNULL, TrgBoundary, 0
                );

            _ConvertComponent(
                &v, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.yuv.v,
                gcvNULL, TrgBoundary, 0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else
        {
            /* Not supported combination. */
            status = gcvSTATUS_NOT_SUPPORTED;
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_YUV)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_YUV)
        {
            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.yuv.y,
                &TrgFormat->u.yuv.y,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.yuv.u,
                &TrgFormat->u.yuv.u,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.yuv.v,
                &TrgFormat->u.yuv.v,
                SrcBoundary, TrgBoundary,
                0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else if (TrgFormat->fmtClass == gcvFORMAT_CLASS_RGBA)
        {
            gctUINT8 y, u, v;
            gctUINT8 r, g, b;

            /*
                Get YUV value.
            */

            _ConvertComponent(
                SrcPixel, &y, SrcBitOffset, 0,
                &SrcFormat->u.yuv.y, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                );

            _ConvertComponent(
                SrcPixel, &u, SrcBitOffset, 0,
                &SrcFormat->u.yuv.u, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                );

            _ConvertComponent(
                SrcPixel, &v, SrcBitOffset, 0,
                &SrcFormat->u.yuv.v, &gcvPIXEL_COMP_XXX8,
                SrcBoundary, gcvNULL, 0
                );

            /*
                Convert to RGB.
            */

            gcoVGHARDWARE_YUV2RGB(
                 y, u, v,
                &r, &g, &b
                );

            /*
                Convert to the final format.
            */

            _ConvertComponent(
                gcvNULL, TrgPixel, 0, TrgBitOffset,
                gcvNULL, &TrgFormat->u.rgba.alpha,
                gcvNULL, TrgBoundary, (gctUINT32)~0
                );

            _ConvertComponent(
                &r, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.rgba.red,
                gcvNULL, TrgBoundary, 0
                );

            _ConvertComponent(
                &g, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.rgba.green,
                gcvNULL, TrgBoundary, 0
                );

            _ConvertComponent(
                &b, TrgPixel, 0, TrgBitOffset,
                &gcvPIXEL_COMP_XXX8, &TrgFormat->u.rgba.blue,
                gcvNULL, TrgBoundary, 0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else
        {
            /* Not supported combination. */
            status = gcvSTATUS_NOT_SUPPORTED;
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_INDEX)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_INDEX)
        {
            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.index.value,
                &TrgFormat->u.index.value,
                SrcBoundary, TrgBoundary,
                0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else
        {
            /* Not supported combination. */
            status = gcvSTATUS_NOT_SUPPORTED;
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_LUMINANCE)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_LUMINANCE)
        {
            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.lum.alpha,
                &TrgFormat->u.lum.alpha,
                SrcBoundary, TrgBoundary,
                (gctUINT32)~0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.lum.value,
                &TrgFormat->u.lum.value,
                SrcBoundary, TrgBoundary,
                0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else
        {
            /* Not supported combination. */
            status = gcvSTATUS_NOT_SUPPORTED;
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_BUMP)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_BUMP)
        {
            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.alpha,
                &TrgFormat->u.bump.alpha,
                SrcBoundary, TrgBoundary,
                (gctUINT32)~0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.l,
                &TrgFormat->u.bump.l,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.v,
                &TrgFormat->u.bump.v,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.u,
                &TrgFormat->u.bump.u,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.q,
                &TrgFormat->u.bump.q,
                SrcBoundary, TrgBoundary,
                0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.bump.w,
                &TrgFormat->u.bump.w,
                SrcBoundary, TrgBoundary,
                0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else
        {
            /* Not supported combination. */
            status = gcvSTATUS_NOT_SUPPORTED;
        }
    }

    else if (SrcFormat->fmtClass == gcvFORMAT_CLASS_DEPTH)
    {
        if (TrgFormat->fmtClass == gcvFORMAT_CLASS_DEPTH)
        {
            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.depth.depth,
                &TrgFormat->u.depth.depth,
                SrcBoundary, TrgBoundary,
                (gctUINT32)~0
                );

            _ConvertComponent(
                SrcPixel, TrgPixel,
                SrcBitOffset, TrgBitOffset,
                &SrcFormat->u.depth.stencil,
                &TrgFormat->u.depth.stencil,
                SrcBoundary, TrgBoundary,
                0
                );

            /* Success. */
            status = gcvSTATUS_OK;
        }

        else
        {
            /* Not supported combination. */
            status = gcvSTATUS_NOT_SUPPORTED;
        }
    }

    else
    {
        /* Not supported class. */
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    gcmFOOTER();
    /* Return status. */
    return status;
}



/*******************************************************************************
**
**  gcoVGHARDWARE_FlushPipe
**
**  Flush the current graphics pipe.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**  OUTPUT:
**
**      Nothing.
*/

typedef gceSTATUS (* gctFLUSHPIXELCACHE) (
    IN gcoVGHARDWARE Hardware
    );

static gceSTATUS
_Flush2DPixelCache(
    IN gcoVGHARDWARE Hardware
    )
{
    return gcoVGHARDWARE_LoadState32(
        Hardware,
        0x0380C,
        ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1))))))) << (0 ? 3:3)))
        );
}

static gceSTATUS
_Flush3DPixelCache(
    IN gcoVGHARDWARE Hardware
    )
{
    return gcoVGHARDWARE_LoadState32(
        Hardware,
        0x0380C,
          ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1))))))) << (0 ? 1:1)))
        );
}

static gceSTATUS
_FlushVGPixelCache(
    IN gcoVGHARDWARE Hardware
    )
{
    gceSTATUS status;

    do
    {
        gcmERR_BREAK(gcoVGHARDWARE_SetState(
            Hardware,
            0x0286C >> 2,
            ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1))))))) << (0 ? 0:0)))
            ));

        gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
            Hardware,
            gcvNULL,
            gcvBLOCK_COMMAND,
            gcvBLOCK_PIXEL,
            gcvHOW_SEMAPHORE_STALL,
            gcvNULL
            ));
    }
    while (gcvFALSE);

    return status;
}

gceSTATUS
gcoVGHARDWARE_FlushPipe(
    IN gcoVGHARDWARE Hardware
    )
{
    gceSTATUS status;

    static gctFLUSHPIXELCACHE flushPipe[] =
    {
        _Flush3DPixelCache,
        _Flush2DPixelCache,
        _FlushVGPixelCache
    };
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Call appropriate cache flush. */
#ifndef __QNXNTO__
    status =  flushPipe[Hardware->context.currentPipe] (Hardware);
#else
    status =  flushPipe[Hardware->pContext->currentPipe] (Hardware);
#endif

    gcmFOOTER();

    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_Semaphore
**
**  Send sempahore and stall until sempahore is signalled.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Points to the location in the command buffer where the command
**          is to be formed.  If gcvNULL is passed, necessary space will be
**          reserved internally.
**
**      gceBLOCK From
**          Semaphore source.
**
**      gceBLOCK To
**          Sempahore destination.
**
**      gceHOW How
**          What to do.  Can be a one of the following values:
**
**              gcvHOW_SEMAPHORE        Send sempahore.
**              gcvHOW_STALL            Stall.
**              gcvHOW_SEMAPHORE_STALL  Send semaphore and stall.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Number of bytes required for the command.  gcvNULL is allowed.
*/
gceSTATUS
gcoVGHARDWARE_Semaphore(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gceBLOCK From,
    IN gceBLOCK To,
    IN gceHOW How,
    OUT gctSIZE_T * Bytes
    )
{
    static gctSIZE_T _reserveSize[] =
    {
        0, /* Invalid                */
        sizeof(gctUINT64), /* gcvHOW_SEMAPHORE       */
        sizeof(gctUINT64), /* gcvHOW_STALL           */
        sizeof(gctUINT64) * 2, /* gcvHOW_SEMAPHORE_STALL */
    };

    static gctUINT32 _target[] =
    {
        0x01, /* gcvBLOCK_COMMAND      */
        0x10, /* gcvBLOCK_TESSELLATOR  */
        0x12, /* gcvBLOCK_TESSELLATOR2 */
        0x14, /* gcvBLOCK_TESSELLATOR3 */
        0x05, /* gcvBLOCK_RASTER       */
        0x0F, /* gcvBLOCK_VG           */
        0x11, /* gcvBLOCK_VG2          */
        0x13, /* gcvBLOCK_VG3          */
        0x07    /* gcvBLOCK_PIXEL        */
    };

    gceSTATUS status;
    gctUINT32_PTR memory;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT((How > 0) && (How < gcmCOUNTOF(_reserveSize)));

    do
    {
        /* Command size request? */
        if (Bytes != gcvNULL)
        {
            /* Set the size. */
            * Bytes = _reserveSize[How];

            /* Success. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Is the space already reserved? */
        if (Logical != gcvNULL)
        {
            /* Set the initial pointer. */
            memory = (gctUINT32_PTR) Logical;

            /* Success. */
            status = gcvSTATUS_OK;
        }
        else
        {
            /* Reserve space. */
            gcmERR_BREAK(gcoVGBUFFER_Reserve(
                Hardware->buffer,
                _reserveSize[How],
                gcvTRUE,
                (gctPOINTER *) &memory
                ));
        }

        /* Is the semaphore set from FE? */
        if (From == gcvBLOCK_COMMAND)
        {
            if (Hardware->fe20)
            {
                gctUINT32 dst;

                /* Determine the pipe switch value. */
                gcmASSERT((To >= 0) && (To < gcmCOUNTOF(_target)));

                dst = _target[To];

                if ((How & gcvHOW_SEMAPHORE) != 0)
                {
                    *memory++
                        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (dst) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8))) | (((gctUINT32) ((gctUINT32) (_GetPipe(Hardware)) & ((gctUINT32) ((((1 ? 9:8) - (0 ? 9:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:8) - (0 ? 9:8) + 1))))))) << (0 ? 9:8)));

                    *memory++
                        = 0;
                }

                if ((How & gcvHOW_STALL) != 0)
                {
                    *memory++
                        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                        | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (dst) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)));

                    *memory++
                        = 0;
                }
            }
            else
            {
                gctUINT32 src;
                gctUINT32 dst;
                gctUINT32 route;

                /* Determine the semaphore/stall route. */
                gcmASSERT((From >= 0) && (From < gcmCOUNTOF(_target)));
                gcmASSERT((To   >= 0) && (To   < gcmCOUNTOF(_target)));

                src = _target[From];
                dst = _target[To];

                route
                    = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (src) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (dst) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

                if ((How & gcvHOW_SEMAPHORE) != 0)
                {
                    gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
                        Hardware,
                        memory, 0x03808 >> 2, 1,
                        gcvNULL
                        ));

                    memory[1] = route;

                    memory += 2;
                }

                if ((How & gcvHOW_STALL) != 0)
                {
                    *memory++
                        = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x09 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));

                    *memory++
                        = route;
                }
            }
        }

        /* The source is not FE. */
        else
        {
            gctUINT32 src;
            gctUINT32 dst;
            gctUINT32 route;

            /* Determine the semaphore/stall route. */
            gcmASSERT((From >= 0) && (From < gcmCOUNTOF(_target)));
            gcmASSERT((To   >= 0) && (To   < gcmCOUNTOF(_target)));

            src = _target[From];
            dst = _target[To];

            route
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (src) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (dst) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));

            if ((How & gcvHOW_SEMAPHORE) != 0)
            {
                gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
                    Hardware,
                    memory, 0x03808 >> 2, 1,
                    gcvNULL
                    ));

                memory[1] = route;

                memory += 2;
            }

            if ((How & gcvHOW_STALL) != 0)
            {
                gcmERR_BREAK(gcoVGHARDWARE_StateCommand(
                    Hardware,
                    memory, 0x03C00 >> 2, 1,
                    gcvNULL
                    ));

                memory[1] = route;
            }
        }

        /* Success. */
        gcmFOOTER();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);


    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  gcoVGHARDWARE_SetAntiAlias
**
**  Enable or disable anti-aliasing.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctBOOL Enable
**          Enable anti-aliasing when gcvTRUE and disable anti-aliasing when
**          gcvFALSE.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_SetAntiAlias(
    IN gcoVGHARDWARE Hardware,
    IN gctBOOL Enable
    )
{
    gceSTATUS status;

    gctUINT32 value;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Store value. */
    Hardware->sampleMask = Enable ? 0xF : 0x0;

    if (Enable)
    {
        /* Enable anti-aliasing. */
        value = (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (Hardware->sampleEnable) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) &((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))));
    }
    else
    {
        /* Disable anti-aliasng. */
        value = (((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 7:4) - (0 ? 7:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:4) - (0 ? 7:4) + 1))))))) << (0 ? 7:4))) &((((gctUINT32) (~0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1))))))) << (0 ? 8:8))));
    }

    /* Set the anti-alias enable bits. */
    status = gcoVGHARDWARE_LoadState32(Hardware,
                                   0x03818,
                                   value);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_SetDither
**
**  Enable or disable dithering.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctBOOL Enable
**          gcvTRUE to enable dithering or gcvFALSE to disable it.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_SetDither(
    IN gcoVGHARDWARE Hardware,
    IN gctBOOL Enable
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Set dithering values. */
    Hardware->dither[0] = Enable ? 0x6E4CA280 : ~0;
    Hardware->dither[1] = Enable ? 0x5D7F91B3 : ~0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_AlignToTile
**
**  Align the specified width and height to tile boundaries.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gceSURF_TYPE Type
**          Type of alignment.
**
**      gctUINT32 * Width
**          Pointer to the width to be aligned.  If 'Width' is gcvNULL, no width
**          will be aligned.
**
**      gctUINT32 * Height
**          Pointer to the height to be aligned.  If 'Height' is gcvNULL, no height
**          will be aligned.
**
**  OUTPUT:
**
**      gctUINT32 * Width
**          Pointer to a variable that will receive the aligned width.
**
**      gctUINT32 * Height
**          Pointer to a variable that will receive the aligned height.
*/
gceSTATUS
gcoVGHARDWARE_AlignToTile(
    IN gcoVGHARDWARE Hardware,
    IN gceSURF_TYPE Type,
    IN OUT gctUINT32 * Width,
    IN OUT gctUINT32 * Height
    )
{

    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Width != gcvNULL)
    {
        /* Align the width. */
        *Width = gcmALIGN(*Width, (Type == gcvSURF_TEXTURE) ? 4 : 16);
    }

    if (Height != gcvNULL)
    {
        /* Special case for VG images. */
        if ((*Height == 0) && (Type == gcvSURF_IMAGE))
        {
            *Height = 4;
        }
        else
        {
                /* Align the height. */
            *Height = gcmALIGN(*Height, 4);
        }
    }

    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_IsVirtualAddress
**
**  Verifies whether the specified physical address is virtual.
**
**  INPUT:
**
**      gctUINT32 Address
**          Address to be verified.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcoVGHARDWARE_IsVirtualAddress(
    IN gctUINT32 Address
    )
{
    gctBOOL isVirtual
        = ((((gctUINT32) (Address)) >> (0 ? 1:0) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1)))))) == (0x2 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1)))))));

    /* Return result. */
    return isVirtual
        ? gcvSTATUS_TRUE
        : gcvSTATUS_FALSE;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_SplitAddress
**
**  Split a harwdare address into pool and offset.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
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
gcoVGHARDWARE_SplitAddress(
    IN gcoVGHARDWARE Hardware,
    IN gctUINT32 Address,
    OUT gcePOOL * Pool,
    OUT gctUINT32 * Offset
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Hardware = 0x%x", Hardware);
    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Pool != gcvNULL);
    gcmVERIFY_ARGUMENT(Offset != gcvNULL);

    /* Dispatch on memory type. */
    switch ((((((gctUINT32) (Address)) >> (0 ? 1:0)) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1)))))) ))
    {
    case 0x0:
        /* System memory. */
        *Pool = gcvPOOL_SYSTEM;
        break;

    case 0x2:
        /* Virtual memory. */
        *Pool = gcvPOOL_VIRTUAL;
        break;

    default:
        /* Invalid memory type. */
        gcmFOOTER_NO();
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Return offset of address. */
    *Offset = ((((gctUINT32) (Address)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) ((gctUINT32) (0) & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));

    gcmFOOTER_NO();
    /* Success. */
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_CombineAddress
**
**  Combine pool and offset into a harwdare address.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
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
gcoVGHARDWARE_CombineAddress(
    IN gcoVGHARDWARE Hardware,
    IN gcePOOL Pool,
    IN gctUINT32 Offset,
    OUT gctUINT32 * Address
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
    gcmVERIFY_ARGUMENT(Address != gcvNULL);

    /* Dispatch on memory type. */
    switch (Pool)
    {
    case gcvPOOL_SYSTEM:
        /* System memory. */
        *Address = ((((gctUINT32) (*Address)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));
        break;

    case gcvPOOL_VIRTUAL:
        /* Virtual memory. */
        *Address = ((((gctUINT32) (*Address)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0))) | (((gctUINT32) (0x2 & ((gctUINT32) ((((1 ? 1:0) - (0 ? 1:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:0) - (0 ? 1:0) + 1))))))) << (0 ? 1:0)));
        break;

    default:
        /* Invalid memory type. */
        gcmFOOTER_NO();
        return gcvSTATUS_INVALID_ARGUMENT;
    }


    gcmFOOTER_NO();
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_IsFeatureAvailable
**
**  Verifies whether the specified feature is available in hardware.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gceFEATURE Feature
**          Feature to be verified.
**
**  OUTPUT:
**
**      Returns not zero if the feature is supported.
*/
gctBOOL
gcoVGHARDWARE_IsFeatureAvailable(
    IN gcoVGHARDWARE Hardware,
    IN gceFEATURE Feature
    )
{
    gctBOOL available;

    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    switch (Feature)
    {
    /* Generic features. */
    case gcvFEATURE_PIPE_2D:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 9:9) & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 9:9) - (0 ? 9:9) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 9:9) - (0 ? 9:9) + 1)))))));
        break;

    case gcvFEATURE_PIPE_3D:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 2:2) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))));
        break;

    case gcvFEATURE_PIPE_VG:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 26:26) & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 26:26) - (0 ? 26:26) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 26:26) - (0 ? 26:26) + 1)))))));
        break;

    case gcvFEATURE_DC:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 8:8) & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 8:8) - (0 ? 8:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 8:8) - (0 ? 8:8) + 1)))))));
        break;

    case gcvFEATURE_HIGH_DYNAMIC_RANGE:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 12:12) & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 12:12) - (0 ? 12:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:12) - (0 ? 12:12) + 1)))))));
        break;

    case gcvFEATURE_MODULE_CG:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 14:14) & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 14:14) - (0 ? 14:14) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 14:14) - (0 ? 14:14) + 1)))))));
        break;

    case gcvFEATURE_MIN_AREA:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 15:15) & ((gctUINT32) ((((1 ? 15:15) - (0 ? 15:15) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:15) - (0 ? 15:15) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 15:15) - (0 ? 15:15) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:15) - (0 ? 15:15) + 1)))))));
        break;

    case gcvFEATURE_BUFFER_INTERLEAVING:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 18:18) & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 18:18) - (0 ? 18:18) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 18:18) - (0 ? 18:18) + 1)))))));
        break;

    case gcvFEATURE_BYTE_WRITE_2D:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 19:19) & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 19:19) - (0 ? 19:19) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 19:19) - (0 ? 19:19) + 1)))))));
        break;

    case gcvFEATURE_ENDIANNESS_CONFIG:
        available = ((((gctUINT32) (Hardware->chipMinorFeatures)) >> (0 ? 2:2) & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 2:2) - (0 ? 2:2) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 2:2) - (0 ? 2:2) + 1)))))));
        break;

    case gcvFEATURE_DUAL_RETURN_BUS:
        available = ((((gctUINT32) (Hardware->chipMinorFeatures)) >> (0 ? 1:1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))));
        break;

    case gcvFEATURE_DEBUG_MODE:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 4:4) & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:4) - (0 ? 4:4) + 1)))))));
        break;

    case gcvFEATURE_YUY2_RENDER_TARGET:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 24:24) & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1)))))));
        break;

    /* Resolve. */
    case gcvFEATURE_FAST_CLEAR:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 0:0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))));
        break;

    case gcvFEATURE_YUV420_TILER:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 13:13) & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 13:13) - (0 ? 13:13) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:13) - (0 ? 13:13) + 1)))))));
        break;

    case gcvFEATURE_YUY2_AVERAGING:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 21:21) & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 21:21) - (0 ? 21:21) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 21:21) - (0 ? 21:21) + 1)))))));
        break;

    case gcvFEATURE_FLIP_Y:
        available = ((((gctUINT32) (Hardware->chipMinorFeatures)) >> (0 ? 0:0) & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 0:0) - (0 ? 0:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 0:0) - (0 ? 0:0) + 1)))))));
        break;

    /* Depth. */
    case gcvFEATURE_EARLY_Z:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 16:16) & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))) == (0x0  & ((gctUINT32) ((((1 ? 16:16) - (0 ? 16:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 16:16) - (0 ? 16:16) + 1)))))));
        break;

    case gcvFEATURE_Z_COMPRESSION:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 5:5) & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))));
        break;

    /* MSAA. */
    case gcvFEATURE_MSAA:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))));
        break;

    case gcvFEATURE_SPECIAL_ANTI_ALIASING:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 1:1) & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 1:1) - (0 ? 1:1) + 1)))))));
        break;

    case gcvFEATURE_SPECIAL_MSAA_LOD:
        available = ((((gctUINT32) (Hardware->chipMinorFeatures)) >> (0 ? 5:5) & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1)))))));
        break;

    /* Texture. */
    case gcvFEATURE_422_TEXTURE_COMPRESSION:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 17:17) & ((gctUINT32) ((((1 ? 17:17) - (0 ? 17:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:17) - (0 ? 17:17) + 1)))))) == (0x0  & ((gctUINT32) ((((1 ? 17:17) - (0 ? 17:17) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:17) - (0 ? 17:17) + 1)))))));
        break;

    case gcvFEATURE_DXT_TEXTURE_COMPRESSION:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 3:3) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))));
        break;

    case gcvFEATURE_ETC1_TEXTURE_COMPRESSION:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 10:10) & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 10:10) - (0 ? 10:10) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 10:10) - (0 ? 10:10) + 1)))))));
        break;

    case gcvFEATURE_CORRECT_TEXTURE_CONVERTER:
#if defined(GC_FEATURES_CORRECT_TEXTURE_CONVERTER)
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) & ((gctUINT32) ((((1 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) - (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) + 1) == 32) ? ~0 : (~(~0 << ((1 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) - (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) + 1)))))) == (GC_FEATURES_CORRECT_TEXTURE_CONVERTER_AVAILABLE  & ((gctUINT32) ((((1 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) - (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) + 1) == 32) ? ~0 : (~(~0 << ((1 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) - (0 ? GC_FEATURES_CORRECT_TEXTURE_CONVERTER) + 1)))))));
#else
        available = gcvFALSE;
#endif
        break;

    case gcvFEATURE_TEXTURE_8K:
        available = ((((gctUINT32) (Hardware->chipMinorFeatures)) >> (0 ? 3:3) & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 3:3) - (0 ? 3:3) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 3:3) - (0 ? 3:3) + 1)))))));
        break;

    /* Filter Blit. */
    case gcvFEATURE_SCALER:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 20:20) & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))) == (0x0  & ((gctUINT32) ((((1 ? 20:20) - (0 ? 20:20) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:20) - (0 ? 20:20) + 1)))))));
        break;

    case gcvFEATURE_YUV420_SCALER:
        available = ((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 6:6) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))) == (0x1  & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1)))))));
        break;

    case gcvFEATURE_VG20:
        available = Hardware->vg20;
        break;

    case gcvFEATURE_VG21:
        available = Hardware->vg21;
        break;

    case gcvFEATURE_VG_FILTER:
        available = Hardware->vgFilter;
        break;

    default:
        available = gcvFALSE;
    }

    gcmFOOTER_NO();
    /* Return result. */
    return available;
}


/*******************************************************************************
**
**  gcoVGHARDWARE_WriteBuffer
**
**  Write data into the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
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
gcoVGHARDWARE_WriteBuffer(
    IN gcoVGHARDWARE Hardware,
    IN gctCONST_POINTER Data,
    IN gctSIZE_T Bytes,
    IN gctBOOL Aligned
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    /* Call hardware. */
    status = gcoVGBUFFER_Write(
        Hardware->buffer,
        Data,
        Bytes,
        Aligned
        );


    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_RGB2YUV
**
**  Convert RGB8 color value to YUV color space.
**
**  INPUT:
**
**      gctUINT8 R, G, B
**          RGB color value.
**
**  OUTPUT:
**
**      gctUINT8_PTR Y, U, V
**          YUV color value.
*/
void
gcoVGHARDWARE_RGB2YUV(
    gctUINT8 R,
    gctUINT8 G,
    gctUINT8 B,
    gctUINT8_PTR Y,
    gctUINT8_PTR U,
    gctUINT8_PTR V
    )
{
    *Y = (gctUINT8) (((66 * R + 129 * G +  25 * B + 128) >> 8) +  16);
    *U = (gctUINT8) (((-38 * R -  74 * G + 112 * B + 128) >> 8) + 128);
    *V = (gctUINT8) (((112 * R -  94 * G -  18 * B + 128) >> 8) + 128);
}

/*******************************************************************************
**
**  gcoVGHARDWARE_YUV2RGB
**
**  Convert YUV color value to RGB8 color space.
**
**  INPUT:
**
**      gctUINT8 Y, U, V
**          YUV color value.
**
**  OUTPUT:
**
**      gctUINT8_PTR R, G, B
**          RGB color value.
*/
void
gcoVGHARDWARE_YUV2RGB(
    gctUINT8 Y,
    gctUINT8 U,
    gctUINT8 V,
    gctUINT8_PTR R,
    gctUINT8_PTR G,
    gctUINT8_PTR B
    )
{
    /* Clamp the input values to the legal ranges. */
    gctINT y = (Y < 16) ? 16 : ((Y > 235) ? 235 : Y);
    gctINT u = (U < 16) ? 16 : ((U > 240) ? 240 : U);
    gctINT v = (V < 16) ? 16 : ((V > 240) ? 240 : V);

    /* Shift ranges. */
    gctINT _y = y - 16;
    gctINT _u = u - 128;
    gctINT _v = v - 128;

    /* Convert to RGB. */
    gctINT r = (298 * _y            + 409 * _v + 128) >> 8;
    gctINT g = (298 * _y - 100 * _u - 208 * _v + 128) >> 8;
    gctINT b = (298 * _y + 516 * _u            + 128) >> 8;

    /* Clamp the result. */
    *R = (r < 0)? 0 : (r > 255)? 255 : (gctUINT8) r;
    *G = (g < 0)? 0 : (g > 255)? 255 : (gctUINT8) g;
    *B = (b < 0)? 0 : (b > 255)? 255 : (gctUINT8) b;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_DataCommand
**
**  Append a DATA command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          DATA command at or gcvNULL to query the size of the command.
**
**      gctUINT32 Count
**          Number of 64-bit data quantities.
**          If 'Logical' is gcvNULL, the argument is ignored.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the END command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the DATA command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_DataCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctSIZE_T Count,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Make sure the count fits. */
            gcmASSERT((Count) <= ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))));

            /* Append STATE. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x4 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16))) | (((gctUINT32) ((gctUINT32) (_GetPipe(Hardware)) & ((gctUINT32) ((((1 ? 17:16) - (0 ? 17:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 17:16) - (0 ? 17:16) + 1))))))) << (0 ? 17:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Count) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the STATE command. */
            *Bytes = 8;
        }
    }
    else
    {
        /* Not supported. */
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }


    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_NopCommand
**
**  Append a NOP command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          NOP command at or gcvNULL to query the size of the command.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the NOP command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the NOP command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_NopCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append NOP. */
            buffer[0] = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x8 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the NOP command. */
            *Bytes = 8;
        }
    }
    else
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }


    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_StateCommand
**
**  Append a STATE command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          STATE command at or gcvNULL to query the size of the command.
**
**      gctUINT32 Address
**          Starting register address of the state buffer.
**          If 'Logical' is gcvNULL, this argument is ignored.
**
**      gctUINT32 Count
**          Number of states in state buffer.
**          If 'Logical' is gcvNULL, this argument is ignored.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the STATE command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the STATE command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_StateCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctUINT32 Address,
    IN gctSIZE_T Count,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append STATE. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (Address) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16))) | (((gctUINT32) ((gctUINT32) (Count) & ((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (_GetPipe(Hardware)) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)));
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the STATE command. */
            *Bytes = 4 * (Count + 1);
        }
    }
    else
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append LOAD_STATE. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (Count) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (Address) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the STATE command. */
            *Bytes = 4 * (Count + 1);
        }
    }

    gcmFOOTER_NO();

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_RestartCommand
**
**  Append a RESTART command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          RESTART command at or gcvNULL to query the size of the command.
**
**      gctUINT32 FetchAddress
**          The address of another command buffer to be executed by this RESTART
**          command.  If 'Logical' is gcvNULL, this argument is ignored.
**
**      gctUINT FetchCount
**          The number of 64-bit data quantities in another command buffer to
**          be executed by this RESTART command.  If 'Logical' is gcvNULL, this
**          argument is ignored.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the RESTART command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the RESTART command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_RestartCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctUINT32 FetchAddress,
    IN gctUINT FetchCount,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;
            gctUINT32 beginEndMark;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Determine Begin/End flag. */
            beginEndMark = (FetchCount > 0)
                ? ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24)))
                : ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 24:24) - (0 ? 24:24) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 24:24) - (0 ? 24:24) + 1))))))) << (0 ? 24:24)));

            /* Append RESTART. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x9 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:0) - (0 ? 20:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:0) - (0 ? 20:0) + 1))))))) << (0 ? 20:0))) | (((gctUINT32) ((gctUINT32) (FetchCount) & ((gctUINT32) ((((1 ? 20:0) - (0 ? 20:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:0) - (0 ? 20:0) + 1))))))) << (0 ? 20:0)))
                | beginEndMark;

            buffer[1]
                = gcmFIXADDRESS(FetchAddress);
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the RESTART command. */
            *Bytes = 8;
        }
    }
    else
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }


    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_FetchCommand
**
**  Append a FETCH command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          FETCH command at or gcvNULL to query the size of the command.
**
**      gctUINT32 FetchAddress
**          The address of another command buffer to be executed by this FETCH
**          command.  If 'Logical' is gcvNULL, this argument is ignored.
**
**      gctUINT FetchCount
**          The number of 64-bit data quantities in another command buffer to
**          be executed by this FETCH command.  If 'Logical' is gcvNULL, this
**          argument is ignored.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the FETCH command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the FETCH command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_FetchCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctUINT32 FetchAddress,
    IN gctUINT FetchCount,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append FETCH. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x5 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:0) - (0 ? 20:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:0) - (0 ? 20:0) + 1))))))) << (0 ? 20:0))) | (((gctUINT32) ((gctUINT32) (FetchCount) & ((gctUINT32) ((((1 ? 20:0) - (0 ? 20:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:0) - (0 ? 20:0) + 1))))))) << (0 ? 20:0)));

            buffer[1]
                = gcmFIXADDRESS(FetchAddress);
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the FETCH command. */
            *Bytes = 8;
        }
    }
    else
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append LINK. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x08 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (FetchCount) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)));

            buffer[1]
                = gcmFIXADDRESS(FetchAddress);
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the LINK command. */
            *Bytes = 8;
        }
    }


    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_CallCommand
**
**  Append a CALL command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          CALL command at or gcvNULL to query the size of the command.
**
**      gctUINT32 FetchAddress
**          The address of another command buffer to be executed by this CALL
**          command.  If 'Logical' is gcvNULL, this argument is ignored.
**
**      gctUINT FetchCount
**          The number of 64-bit data quantities in another command buffer to
**          be executed by this CALL command.  If 'Logical' is gcvNULL, this
**          argument is ignored.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the CALL command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the CALL command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_CallCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctUINT32 FetchAddress,
    IN gctUINT FetchCount,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append CALL. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x6 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 20:0) - (0 ? 20:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:0) - (0 ? 20:0) + 1))))))) << (0 ? 20:0))) | (((gctUINT32) ((gctUINT32) (FetchCount) & ((gctUINT32) ((((1 ? 20:0) - (0 ? 20:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 20:0) - (0 ? 20:0) + 1))))))) << (0 ? 20:0)));

            buffer[1]
                = gcmFIXADDRESS(FetchAddress);
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the CALL command. */
            *Bytes = 8;
        }
    }
    else
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }


    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_ReturnCommand
**
**  Append a RETURN command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          RETURN command at or gcvNULL to query the size of the command.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the RETURN command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the RETURN command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_ReturnCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append RETURN. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x7 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)));
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the RETURN command. */
            *Bytes = 8;
        }
    }
    else
    {
        gcmFOOTER_NO();
        return gcvSTATUS_NOT_SUPPORTED;
    }


    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_EventCommand
**
**  Append an EVENT command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          EVENT command at or gcvNULL to query the size of the command.
**
**      gctINT32 InterruptId
**          The ID of the interrupt to generate.
**          If 'Logical' is gcvNULL, this argument is ignored.
**
**      gceBLOCK Block
**          Block that will generate the interrupt.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the EVENT command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the END command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_EventCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gceBLOCK Block,
    IN gctINT32 InterruptId,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        typedef struct _gcsEVENTSTATES
        {
            /* Chips before VG21 use these values. */
            gctUINT     eventFromFE;
            gctUINT     eventFromPE;

            /* VG21 chips and later use SOURCE field. */
            gctUINT     eventSource;
        }
        gcsEVENTSTATES;

        static gcsEVENTSTATES states[] =
        {
            /* gcvBLOCK_COMMAND */
            {
                (gctUINT)~0,
                (gctUINT)~0,
                (gctUINT)~0
            },

            /* gcvBLOCK_TESSELLATOR */
            {
                0x0,
                0x1,
                0x10
            },

            /* gcvBLOCK_RASTER */
            {
                0x0,
                0x1,
                0x07,
            },

            /* gcvBLOCK_VG */
            {
                0x0,
                0x1,
                0x0F
            },

            /* gcvBLOCK_PIXEL */
            {
                0x0,
                0x1,
                0x07
            },
        };

        gcmHEADER_ARG("Hardware = 0x%x", Hardware);
        /* Verify block ID. */
        gcmVERIFY_ARGUMENT(gcmIS_VALID_INDEX(Block, states));

        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Verify the event ID. */
            gcmVERIFY_ARGUMENT(InterruptId >= 0);
            gcmVERIFY_ARGUMENT(InterruptId <= ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))));

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append EVENT. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x3 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0))) | (((gctUINT32) ((gctUINT32) (0x0E01) & ((gctUINT32) ((((1 ? 11:0) - (0 ? 11:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 11:0) - (0 ? 11:0) + 1))))))) << (0 ? 11:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 27:16) - (0 ? 27:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16) - (0 ? 27:16) + 1))))))) << (0 ? 27:16)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12))) | (((gctUINT32) ((gctUINT32) (_GetPipe(Hardware)) & ((gctUINT32) ((((1 ? 13:12) - (0 ? 13:12) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 13:12) - (0 ? 13:12) + 1))))))) << (0 ? 13:12)));

            /* Determine chip version. */
            if (Hardware->vg21)
            {
                /* Get the event source for the block. */
                gctUINT eventSource = states[Block].eventSource;

                /* Supported? */
                if (eventSource == ~0)
                {
                    gcmFOOTER_NO();
                    return gcvSTATUS_NOT_SUPPORTED;
                }

                buffer[1]
                    = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (InterruptId) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8))) | (((gctUINT32) ((gctUINT32) (eventSource) & ((gctUINT32) ((((1 ? 12:8) - (0 ? 12:8) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 12:8) - (0 ? 12:8) + 1))))))) << (0 ? 12:8)));
            }
            else
            {
                /* Get the event source for the block. */
                gctUINT eventFromFE = states[Block].eventFromFE;
                gctUINT eventFromPE = states[Block].eventFromPE;

                /* Supported? */
                if (eventFromFE == ~0)
                {
                    gcmFOOTER_NO();
                    return gcvSTATUS_NOT_SUPPORTED;
                }

                buffer[1]
                    = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (InterruptId) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) ((gctUINT32) (eventFromFE) & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) ((gctUINT32) (eventFromPE) & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
            }
        }

        if (Bytes != gcvNULL)
        {
            /* Make sure the events are directly supported for the block. */
            if (states[Block].eventSource == ~0)
            {
                gcmFOOTER_NO();
                return gcvSTATUS_NOT_SUPPORTED;
            }

            /* Return number of bytes required by the END command. */
            *Bytes = 8;
        }
    }
    else
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Verify the event ID. */
            gcmVERIFY_ARGUMENT(InterruptId >= 0);
            gcmVERIFY_ARGUMENT(InterruptId <= ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))));

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append EVENT. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E01) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));

            /* Determine event source. */
            if (Block == gcvBLOCK_COMMAND)
            {
                buffer[1]
                    = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (InterruptId) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)));
            }
            else
            {
                buffer[1]
                    = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (InterruptId) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                    | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 6:6) - (0 ? 6:6) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 6:6) - (0 ? 6:6) + 1))))))) << (0 ? 6:6)));
            }
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the EVENT and END commands. */
            *Bytes = 8;
        }
    }


    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**  gcoVGHARDWARE_EndCommand
**
**  Append an END command at the specified location in the command buffer.
**
**  INPUT:
**
**      gcoVGHARDWARE Hardware
**          Pointer to the gcoVGHARDWARE object.
**
**      gctPOINTER Logical
**          Pointer to the current location inside the command buffer to append
**          END command at or gcvNULL to query the size of the command.
**
**      gctINT32 InterruptId
**          The ID of the interrupt to generate.
**          If 'Logical' is gcvNULL, this argument is ignored.
**
**      gctSIZE_T * Bytes
**          Pointer to the number of bytes available for the END command.
**          If 'Logical' is gcvNULL, the value from this argument is ignored.
**
**  OUTPUT:
**
**      gctSIZE_T * Bytes
**          Pointer to a variable that will receive the number of bytes required
**          for the END command.  If 'Bytes' is gcvNULL, nothing is returned.
*/
gceSTATUS
gcoVGHARDWARE_EndCommand(
    IN gcoVGHARDWARE Hardware,
    IN gctPOINTER Logical,
    IN gctINT32 InterruptId,
    IN OUT gctSIZE_T * Bytes
    )
{
    gcmHEADER_ARG("Hardware = 0x%x", Hardware);

    gcmGETVGHARDWARE(Hardware);
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

    if (Hardware->fe20)
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Verify the event ID. */
            gcmVERIFY_ARGUMENT(InterruptId >= 0);
            gcmVERIFY_ARGUMENT(InterruptId <= ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))));

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append END. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28))) | (((gctUINT32) (0x0 & ((gctUINT32) ((((1 ? 31:28) - (0 ? 31:28) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28) - (0 ? 31:28) + 1))))))) << (0 ? 31:28)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (InterruptId) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)));
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the END command. */
            *Bytes = 8;
        }
    }
    else
    {
        if (Logical != gcvNULL)
        {
            gctUINT32_PTR buffer;

            /* Verify the event ID. */
            gcmVERIFY_ARGUMENT(InterruptId >= 0);
            gcmVERIFY_ARGUMENT(InterruptId <= ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))));

            /* Cast the buffer pointer. */
            buffer = (gctUINT32_PTR) Logical;

            /* Append EVENT. */
            buffer[0]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x01 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0))) | (((gctUINT32) ((gctUINT32) (0x0E01) & ((gctUINT32) ((((1 ? 15:0) - (0 ? 15:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0) - (0 ? 15:0) + 1))))))) << (0 ? 15:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16))) | (((gctUINT32) ((gctUINT32) (1) & ((gctUINT32) ((((1 ? 25:16) - (0 ? 25:16) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 25:16) - (0 ? 25:16) + 1))))))) << (0 ? 25:16)));

            buffer[1]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0))) | (((gctUINT32) ((gctUINT32) (InterruptId) & ((gctUINT32) ((((1 ? 4:0) - (0 ? 4:0) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 4:0) - (0 ? 4:0) + 1))))))) << (0 ? 4:0)))
                | ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5))) | (((gctUINT32) (0x1 & ((gctUINT32) ((((1 ? 5:5) - (0 ? 5:5) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 5:5) - (0 ? 5:5) + 1))))))) << (0 ? 5:5)));

            /* Append END. */
            buffer[2]
                = ((((gctUINT32) (0)) & ~(((gctUINT32) (((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27))) | (((gctUINT32) (0x02 & ((gctUINT32) ((((1 ? 31:27) - (0 ? 31:27) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:27) - (0 ? 31:27) + 1))))))) << (0 ? 31:27)));
        }

        if (Bytes != gcvNULL)
        {
            /* Return number of bytes required by the EVENT and END commands. */
            *Bytes = 16;
        }
    }


    gcmFOOTER_NO();
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryTargetCaps
**
**	Query the render target capabilities.
**
**	INPUT:
**
**		gcoHARDWARE Hardware
**			Pointer to gcoHARDWARE object.
**
**	OUTPUT:
**
**		gctUINT * MaxWidth
**			Pointer to a variable receiving the maximum width of a render
**			target.
**
**		gctUINT * MaxHeight
**			Pointer to a variable receiving the maximum height of a render
**			target.
**
**		gctUINT * MultiTargetCount
**			Pointer to a variable receiving the number of render targets.
**
**		gctUINT * MaxSamples
**			Pointer to a variable receiving the maximum number of samples per
**			pixel for MSAA.
*/
gceSTATUS
gcoVGHARDWARE_QueryTargetCaps(
	IN gcoVGHARDWARE Hardware,
	OUT gctUINT * MaxWidth,
	OUT gctUINT * MaxHeight,
	OUT gctUINT * MultiTargetCount,
	OUT gctUINT * MaxSamples
	)
{
    gceSTATUS status = gcvSTATUS_OK;

	gcmHEADER_ARG("Hardware=0x%x MaxWidth=0x%x MaxHeight=0x%x "
					"MultiTargetCount=0x%x MaxSamples=0x%x",
					Hardware, MaxWidth, MaxHeight,
					MultiTargetCount, MaxSamples);

    gcmGETVGHARDWARE(Hardware);

	if (MaxWidth != gcvNULL)
	{
		/* Return maximum width of render target for XAQ2. */
		*MaxWidth = 2048;
	}

	if (MaxHeight != gcvNULL)
	{
		/* Return maximum height of render target for XAQ2. */
		*MaxHeight = 2048;
	}

	if (MultiTargetCount != gcvNULL)
	{
		/* Return number of render targets for XAQ2. */
		*MultiTargetCount = 1;
	}

	if (MaxSamples != gcvNULL)
	{
		/* Return number of samples per pixel for XAQ2. */
		if (((((gctUINT32) (Hardware->chipFeatures)) >> (0 ? 7:7) & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1)))))) == (0x1 & ((gctUINT32) ((((1 ? 7:7) - (0 ? 7:7) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 7:7) - (0 ? 7:7) + 1))))))))
		{
			*MaxSamples = 4;
		}
		else
		{
			*MaxSamples = 0;
		}
#if USE_SUPER_SAMPLING
		if (*MaxSamples == 0)
		{
			*MaxSamples = 4;
		}
#endif
	}

    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**	gcoHARDWARE_ConvertFormat
**
**	Convert an API format to hardware parameters.
**
**	INPUT:
**
**		gcoHARDWARE Hardware
**			Pointer to the gcoHARDWARE object.
**
**		gceSURF_FORMAT Format
**			API format to convert.
**
**	OUTPUT:
**
**		gctUINT32_PTR BitsPerPixel
**			Pointer to a variable that will hold the number of bits per pixel.
**
**		gctUINT32_PTR BytesPerTile
**			Pointer to a variable that will hold the number of bytes per tile.
*/
gceSTATUS gcoVGHARDWARE_ConvertFormat(
	IN gcoVGHARDWARE Hardware,
	IN gceSURF_FORMAT Format,
	OUT gctUINT32_PTR BitsPerPixel,
	OUT gctUINT32_PTR BytesPerTile
	)
{
    gctUINT32 bitsPerPixel = 0;
    gctUINT32 bytesPerTile = 0;

    gcmHEADER_ARG("Hardware=0x%x Format=%d BitsPerPixel=0x%x BytesPerTile=0x%x",
                    Hardware, Format, BitsPerPixel, BytesPerTile);

    gcmGETVGHARDWARE(Hardware);

    /* Dispatch on format. */
    /* Dispatch on format. */
    switch (Format)
    {
    case gcvSURF_A1:
    case gcvSURF_L1:
        /* 1-bpp format. */
        bitsPerPixel  = 1;
        bytesPerTile  = (1 * 4 * 4) / 8;
        break;

    case gcvSURF_A4:
        /* 4-bpp format. */
        bitsPerPixel  = 4;
        bytesPerTile  = (4 * 4 * 4) / 8;
        break;

    case gcvSURF_INDEX8:
    case gcvSURF_A8:
    case gcvSURF_L8:
        /* 8-bpp format. */
        bitsPerPixel  = 8;
        bytesPerTile  = (8 * 4 * 4) / 8;
        break;

    case gcvSURF_YV12:
        /* 12-bpp planar YUV formats. */
        bitsPerPixel  = 12;
        bytesPerTile  = (12 * 4 * 4) / 8;
        break;

    /* 4444 variations. */
    case gcvSURF_X4R4G4B4:
    case gcvSURF_A4R4G4B4:
    case gcvSURF_R4G4B4X4:
    case gcvSURF_R4G4B4A4:
    case gcvSURF_B4G4R4X4:
    case gcvSURF_B4G4R4A4:
    case gcvSURF_X4B4G4R4:
    case gcvSURF_A4B4G4R4:

    /* 1555 variations. */
    case gcvSURF_X1R5G5B5:
    case gcvSURF_A1R5G5B5:
    case gcvSURF_R5G5B5X1:
    case gcvSURF_R5G5B5A1:
    case gcvSURF_X1B5G5R5:
    case gcvSURF_A1B5G5R5:
    case gcvSURF_B5G5R5X1:
    case gcvSURF_B5G5R5A1:

    /* 565 variations. */
    case gcvSURF_R5G6B5:
    case gcvSURF_B5G6R5:

    case gcvSURF_A8L8:
    case gcvSURF_YUY2:
    case gcvSURF_UYVY:
    case gcvSURF_D16:
        /* 16-bpp format. */
        bitsPerPixel  = 16;
        bytesPerTile  = (16 * 4 * 4) / 8;
        break;

    case gcvSURF_X8R8G8B8:
    case gcvSURF_A8R8G8B8:
    case gcvSURF_X8B8G8R8:
    case gcvSURF_A8B8G8R8:
    case gcvSURF_R8G8B8X8:
    case gcvSURF_R8G8B8A8:
    case gcvSURF_B8G8R8X8:
    case gcvSURF_B8G8R8A8:
    case gcvSURF_D24S8:
    case gcvSURF_D32:
        /* 32-bpp format. */
        bitsPerPixel  = 32;
        bytesPerTile  = (32 * 4 * 4) / 8;
        break;

    case gcvSURF_DXT1:
    case gcvSURF_ETC1:
        bitsPerPixel = 4;
        bytesPerTile = (4 * 4 * 4) / 8;
        break;

    case gcvSURF_DXT2:
    case gcvSURF_DXT3:
    case gcvSURF_DXT4:
    case gcvSURF_DXT5:
        bitsPerPixel = 4;
        bytesPerTile = (4 * 4 * 4) / 8;
        break;

    default:
        /* Invalid format. */
        gcmFOOTER_NO();
        return gcvSTATUS_INVALID_ARGUMENT;
    }


    /* Set the result. */
    if (BitsPerPixel != gcvNULL)
    {
        * BitsPerPixel = bitsPerPixel;
    }

    if (BytesPerTile != gcvNULL)
    {
        * BytesPerTile = bytesPerTile;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**
**	gcoHARDWARE_QueryTileSize
**
**	Query the tile sizes.
**
**	INPUT:
**
**		Nothing.
**
**	OUTPUT:
**
**		gctINT32 * TileWidth2D
**			Pointer to a variable receiving the width in pixels per 2D tile.  If
**			the 2D is working in linear space, the width will be 1.  If there is
**			no 2D, the width will be 0.
**
**		gctINT32 * TileHeight2D
**			Pointer to a variable receiving the height in pixels per 2D tile.
**			If the 2D is working in linear space, the height will be 1.  If
**			there is no 2D, the height will be 0.
**
**		gctINT32 * TileWidth3D
**			Pointer to a variable receiving the width in pixels per 3D tile.  If
**			the 3D is working in linear space, the width will be 1.  If there is
**			no 3D, the width will be 0.
**
**		gctINT32 * TileHeight3D
**			Pointer to a variable receiving the height in pixels per 3D tile.
**			If the 3D is working in linear space, the height will be 1.  If
**			there is no 3D, the height will be 0.
**
**		gctUINT32 * Stride
**			Pointer to  variable receiving the stride requirement for all modes.
*/
gceSTATUS gcoVGHARDWARE_QueryTileSize(
	OUT gctINT32 * TileWidth2D,
	OUT gctINT32 * TileHeight2D,
	OUT gctINT32 * TileWidth3D,
	OUT gctINT32 * TileHeight3D,
	OUT gctUINT32 * Stride
	)
{
	if (TileWidth2D != gcvNULL)
	{
		/* 1 pixel per 2D tile (linear). */
		*TileWidth2D = 1;
	}

	if (TileHeight2D != gcvNULL)
	{
		/* 1 pixel per 2D tile (linear). */
		*TileHeight2D = 1;
	}

	if (TileWidth3D != gcvNULL)
	{
		/* 4 pixels per 3D tile for now. */
		*TileWidth3D = 4;
	}

	if (TileHeight3D != gcvNULL)
	{
		/* 4 lines per 3D tile. */
		*TileHeight3D = 4;
	}

	if (Stride != gcvNULL)
	{
		/* 64-byte stride requirement. */
		*Stride = 64;
	}

	/* Success. */
	return gcvSTATUS_OK;
}
#endif /* gcdENABLE_VG */


