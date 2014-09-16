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




#ifndef __gc_copybit_h_
#define __gc_copybit_h_

// To enable verbose messages (0=Enable), see cutils/log.h
#define LOG_NDEBUG 1

#include <errno.h>
#include <linux/fb.h>
#include <linux/fs.h>
#include <linux/android_pmem.h>
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/copybit.h>

#include <gc_hal.h>
#include <gc_hal_raster.h>

/* Use filter stretch blit (Only PE20). */
#define gcdNEED_FILTER_BLIT     0
/* Filter blit block size: 5 or 9 */
#define gcdFILTER_BLOCK_SIZE    9

typedef struct _gcs_SURFACE
{
    gcoSURF        surface;
    gctUINT32      physical;
    gctPOINTER     logical;
    gctUINT32      width;
    gctUINT32      height;
    gctINT         stride;
    gctUINT32      alignedWidth;
    gctUINT32      alignedHeight;
    gceSURF_FORMAT format;
}
gcsSURFACE;

/* copybit context */
typedef struct _gcsCOPYBIT_CONTEXT
{
    struct copybit_device_t device;

    /* Reference count. */
    int reference;

    /* Our private state goes below here */
    int rotationDeg;
    int planeAlpha;
    int dither;
    int transform;
    int blur;

    /* Hash key. */
    union
    {
        struct
        {
            int rotationDegKey   : 1;
            int alphaKey         : 1;
            int ditherKey        : 1;
            int transformKey     : 1;
            int blurKey          : 1;

            int dummyKey         : 27; /* 32 - 5 */
        }
        s;

        int i;
    }
    dirty;

    /* PE 2.0 feature supported. */
    gctBOOL hasPE20;

    gctBOOL perpixelAlpha;

    /* Temp dest surface with alpha. */
    gctBOOL needAlphaDest;
    gcsSURFACE alphaDest;

#if gcdNEED_FILTER_BLIT
    gcsSURFACE tempSurface;
#endif

    /* GC objects. */
    gco2D   engine;

    /* GC rotation states. */
    gceSURF_ROTATION srcRotation;
    gceSURF_ROTATION dstRotation;
}
gcsCOPYBIT_CONTEXT;

/* Blit. */
gceSTATUS
_StretchBlit(
    IN gcsCOPYBIT_CONTEXT * Context,
    IN struct copybit_image_t const  * Dest,
    IN struct copybit_image_t const  * Source,
    IN struct copybit_rect_t const   * DestRect,
    IN struct copybit_rect_t const   * SourceRect,
    IN struct copybit_region_t const * Clip
    );

gceSTATUS
_StretchBlitPE1x(
    IN gcsCOPYBIT_CONTEXT * Context,
    IN struct copybit_image_t const  * Dest,
    IN struct copybit_image_t const  * Source,
    IN struct copybit_rect_t const   * DestRect,
    IN struct copybit_rect_t const   * SourceRect,
    IN struct copybit_region_t const * Clip
    );

/* Misc. */
gceSURF_FORMAT
_ConvertFormat(
    IN int Format
    );

gctUINT32
_GetStride(
    IN int Format,
    IN gctUINT32 Width
    );

gctBOOL
_HasAlpha(
    IN gceSURF_FORMAT Format
    );

gceSTATUS
_FitSurface(
    IN gcsCOPYBIT_CONTEXT * Context,
    IN gcsSURFACE * Surface,
    IN gctUINT32 Width,
    IN gctUINT32 Height
    );

gceSTATUS
_FreeSurface(
    IN gcsSURFACE * Surface
    );

#endif /* __gc_copybit_h_ */

