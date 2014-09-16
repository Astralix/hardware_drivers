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




#include "gc_copybit.h"

gctUINT32
_GetStride(
    IN int Format,
    IN gctUINT32 Width
    )
 {
    gctUINT32 stride = 0;

    switch (Format)
    {
    case COPYBIT_FORMAT_RGBA_8888:
    case COPYBIT_FORMAT_BGRA_8888:
    case COPYBIT_FORMAT_RGBX_8888:
        stride =  Width * 4;
        break;

    case COPYBIT_FORMAT_RGB_565:
    case COPYBIT_FORMAT_RGBA_5551:
    case COPYBIT_FORMAT_RGBA_4444:
        stride = Width * 2;
        break;

    /*
    case COPYBIT_FORMAT_YCbCr_422_SP:
    case COPYBIT_FORMAT_YCbCr_422_I:
        stride = Width * 16 / 8;
        break;

    case COPYBIT_FORMAT_YCbCr_420_SP:
        stride = Width * 12 / 8;
        break;
    */

    default:
        stride = 0;
        break;
    }

    return stride;
}

gctBOOL
_HasAlpha(
    IN gceSURF_FORMAT Format
    )
{
    if ((Format == gcvSURF_A8R8G8B8)
    ||  (Format == gcvSURF_A1R5G5B5)
    ||  (Format == gcvSURF_A4R4G4B4)
    ||  (Format == gcvSURF_A8B8G8R8)
    )
    {
        return gcvTRUE;
    }

    return gcvFALSE;
}

gceSTATUS
_FitSurface(
    IN gcsCOPYBIT_CONTEXT * Context,
    IN gcsSURFACE * Surface,
    IN gctUINT32 Width,
    IN gctUINT32 Height
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    if (Surface == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    do
    {
        /* Create the tmp Surfaceace if necessary. */
        if ((Surface->surface == gcvNULL)
        ||  (Surface->width  < Width)
        ||  (Surface->height < Height)
        )
        {
            /* Destroy last surface. */
            if (Surface->surface != gcvNULL)
            {
                if (Surface->logical != gcvNULL)
                {
                    gcmERR_BREAK(
                        gcoSURF_Unlock(Surface->surface,
                                       Surface->logical));
                }

                gcmERR_BREAK(
                    gcoSURF_Destroy(Surface->surface));
            }

            /* New surface values. */
            Surface->format = gcvSURF_A8R8G8B8;
            Surface->width  = Width;
            Surface->height = Height;

            gcmERR_BREAK(
                gcoSURF_Construct(gcvNULL,
                                  Surface->width,
                                  Surface->height,
                                  1,
                                  gcvSURF_BITMAP,
                                  Surface->format,
                                  gcvPOOL_DEFAULT,
                                  &Surface->surface));

            gcmERR_BREAK(
                gcoSURF_Lock(Surface->surface,
                             &Surface->physical,
                             &Surface->logical));

            gcmERR_BREAK(
                gcoSURF_GetAlignedSize(Surface->surface,
                                       &Surface->alignedWidth,
                                       &Surface->alignedHeight,
                                       &Surface->stride));

        }
    }
    while (gcvFALSE);

    return status;
}

gceSTATUS
_FreeSurface(
    IN gcsSURFACE * Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    if (Surface == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    do
    {
        if (Surface->surface != gcvNULL)
        {
            if (Surface->logical != gcvNULL)
            {
                gcmERR_BREAK(
                    gcoSURF_Unlock(Surface->surface,
                                   Surface->logical));
            }

            gcmERR_BREAK(
                gcoSURF_Destroy(Surface->surface));

            Surface->surface  = gcvNULL;
            Surface->physical = ~0;
            Surface->logical  = gcvNULL;
            Surface->width    = 0;
            Surface->height   = 0;
            Surface->format   = gcvSURF_UNKNOWN;
        }
    }
    while (gcvFALSE);

    return status;
}

