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




#ifndef __gc_hwc_h_
#define __gc_hwc_h_


/*******************************************************************************
** Build options.
*/

/*
    ENABLE_SWAP_RECTANGLE

        Set to 1 for enabling swap rectangle optimization.
*/
#define ENABLE_SWAP_RECTANGLE 1

/*
    ENABLE_DIM

        Set to 1 for enabling dim layer support in hwc.
*/
#define ENABLE_DIM            1

/*
    ENABLE_CLEAR_HOLE

        Set it to 1 to enable clear hole support in hwc.

        NOTICE: This feature will block arbitrary rotation. Disable it if
        arbitrary rotation is needed.
*/
#define ENABLE_CLEAR_HOLE     0

/*
    CLEAR_FB_FOR_OVERAY

        Enable it to clear overlay area to transparet in framebuffer memory.
        It is done with glClear for 3D composition (SurfaceFlinger.cpp),
        and with a 2D clear for hwcomposer (gc_hwc_set.cpp).
*/
#define CLEAR_FB_FOR_OVERLAY  1


/******************************************************************************/

#include <stdlib.h>

#include <hardware/hwcomposer.h>
#include <ui/android_native_buffer.h>
#include <cutils/log.h>

#include <gc_hal_base.h>
#include <gc_hal_raster.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Extended composition types. */
enum
{
    /* NOTE: These enums are unknown to Android.
     * Android only checks against HWC_FRAMEBUFFER. */
    HWC_BLITTER = 100,
    HWC_DIM,
    HWC_CLEAR_HOLE,
    HWC_BYPASS
};


/* A framebuffer buffer. */
struct hwcBuffer;

/* Framebuffer window struct. */
struct hwcFramebuffer
{
    /* Framebuffer resolution. */
    gcsRECT                          res;

    /* Framebuffer stride. */
    gctUINT32                        stride;

    /* Framebuffer tiling. */
    gceTILING                        tiling;

    /* Framebuffer format. */
    gceSURF_FORMAT                   format;

    /* Bytes per pixel. */
    gctUINT32                        bytesPerPixel;

    /* Swap rectangles are valid? */
    gctBOOL                          valid;

    /* The first framebuffer buffer. */
    struct hwcBuffer *               head;

    /* Current frameubffer buffer. */
    struct hwcBuffer *               target;
};


/* Framebuffer buffers in linked list. */
struct hwcBuffer
{
     /* Buffer handle. */
     native_handle_t *               handle;

     /* Physical address of this buffer. */
     gctUINT32                       physical;

     /* Swap rectangle. */
     gcsRECT                         swapRect;

     /* Point to prev/next buffer. */
     struct hwcBuffer *              prev;
     struct hwcBuffer *              next;
};


/* Area struct. */
struct hwcArea
{
    /* Area potisition. */
    gcsRECT                          rect;

    /* Bit field, layers who own this Area. */
    gctUINT32                        owners;

    /* Point to next area. */
    struct hwcArea *                 next;
};


/* Area pool struct. */
struct hwcAreaPool
{
    /* Pre-allocated areas. */
    hwcArea *                        areas;

    /* Point to free area. */
    hwcArea *                        freeNodes;

    /* Next area pool. */
    hwcAreaPool *                    next;
};


/* Layer struct. */
struct hwcLayer
{
    /* Composition type of this layer. */
    gctUINT32                        compositionType;

    /***************************************************************************
    ** Color source from surface memory.
    */

    /* Source surface. */
    gcoSURF                          source;

    /* Source address. */
    gctUINT32                        addresses[3];
    gctUINT32                        addressNum;

    /* Source stride. */
    gctUINT32                        strides[3];
    gctUINT32                        strideNum;

    /* Source tiling. */
    gceTILING                        tiling;

    /* Source format. */
    gceSURF_FORMAT                   format;

    /* Source rotation. */
    gceSURF_ROTATION                 rotation;

    /* Source size. */
    gctUINT32                        width;
    gctUINT32                        height;

    /* Source orientation. */
    gceORIENTATION                   orientation;

    /* Mirror parameters. */
    gctBOOL                          hMirror;
    gctBOOL                          vMirror;

    /* Full source rectangle. */
    gcsRECT                          srcRect;

    /* YUV format. */
    gctBOOL                          yuv;

    /* Bytes per pixel. */
    gctUINT32                        bytesPerPixel;

    /* Original source rectangle before transformation. */
    gcsRECT                          orgRect;

    /***************************************************************************
    ** Color source from solid color.
    */

    /* Color value, this is for OVERLAY, CLEAR_HOLE and DIM. */
    gctUINT32                        color32;

    /***************************************************************************
    ** Alpha blending and premultiply parametes.
    */

    /* Opaque layer, means no alpha blending. */
    gctBOOL                          opaque;

    /* Alpha blending mode. */
    gceSURF_PIXEL_ALPHA_MODE         srcAlphaMode;
    gceSURF_PIXEL_ALPHA_MODE         dstAlphaMode;

    /* Global alpha blending mode. */
    gceSURF_GLOBAL_ALPHA_MODE        srcGlobalAlphaMode;
    gceSURF_GLOBAL_ALPHA_MODE        dstGlobalAlphaMode;

    /* Blend factor. */
    gceSURF_BLEND_FACTOR_MODE        srcFactorMode;
    gceSURF_BLEND_FACTOR_MODE        dstFactorMode;

    /* Perpixel alpha premultiply. */
    gce2D_PIXEL_COLOR_MULTIPLY_MODE  srcPremultSrcAlpha;
    gce2D_PIXEL_COLOR_MULTIPLY_MODE  dstPremultDstAlpha;

    /* Source global alpha premultiply. */
    gce2D_GLOBAL_COLOR_MULTIPLY_MODE srcPremultGlobalMode;

    /* Dest global alpha demultiply. */
    gce2D_PIXEL_COLOR_MULTIPLY_MODE  dstDemultDstAlpha;

    /* Global alpha value. */
    gctUINT32                        srcGlobalAlpha;
    gctUINT32                        dstGlobalAlpha;

    /***************************************************************************
    ** Dest information.
    */

    /* Full dest rectangle. */
    gcsRECT                          dstRect;

    /***************************************************************************
    ** Stretching.
    */

    /* Stretch factors. */
    gctFLOAT                         hfactor;
    gctFLOAT                         vfactor;

    /* Kernel size for filter blit. */
    gctUINT8                         hkernel;
    gctUINT8                         vkernel;

    /* Stretch flag. */
    gctBOOL                          stretch;
};


/* HWC context. */
struct hwcContext
{
    hwc_composer_device_t device;

    /***************************************************************************
    ** Features.
    */

    /* Feature: 2D separated core? */
    gctBOOL                          separated2D;

    /* Feature: 2D PE 2.0. */
    gctBOOL                          pe20;

    /* Feature: 2D multi-source blit. */
    gctBOOL                          multiSourceBlt;

    /* Feature: 2D multi-source blit extended. */
    gctBOOL                          multiSourceBltEx;
    /* Feature: Max multi-source count. */
    gctUINT32                        maxSource;

    /* Feature: One pass filter/YUV blit/2D tiling input. */
    gctBOOL                          opf;

    /***************************************************************************
    ** States.
    */

    /* Geometry change flag. */
    gctBOOL                          geometryChanged;

    /* Flag: has hwc composition. */
    gctBOOL                          hasComposition;

    /* Flag: has dim layer. */
    gctBOOL                          hasDim;

    /* Flag: has clear hole layer. */
    gctBOOL                          hasClearHole;

    /* Flag: has overlay layer. */
    gctBOOL                          hasOverlay;

    /* Target framebuffer information. */
    hwcFramebuffer *                 framebuffer;

    /* Layers, max 32 layers. */
    gctUINT32                        layerCount;
    hwcLayer                         layers[32];

    /* Splited composition area queue. */
    hwcArea *                        compositionArea;

    /* Rectangles to be copyed from previous buffer. */
    hwcArea *                        swapArea;

    /* Pre-allocated area pool. */
    hwcAreaPool                      areaPool;

    /***************************************************************************
    ** GC Objects.
    */

    /* OS object. */
    gcoOS                            os;

    /* HAL object. */
    gcoHAL                           hal;

    /* Raster engine */
    gco2D                            engine;

#if defined(gcdDEFER_RESOLVES) && gcdDEFER_RESOLVES
    /* Imported render target. */
    gcoSURF                          importedRT;
#endif
};


/*******************************************************************************
** Functions.
*/

gceSTATUS
hwcPrepare(
    IN hwcContext * Context,
    IN hwc_layer_list_t * List
    );


gceSTATUS
hwcSet(
    IN hwcContext * Context,
    IN android_native_buffer_t * BackBuffer,
    IN hwc_layer_list_t * List
    );


gceSTATUS
hwcCompose(
    IN hwcContext * Context
    );


int
hwcOverlay(
    IN hwcContext * Context,
    IN hwc_layer_list_t * List
    );


#ifdef __cplusplus
}
#endif

#endif /* __gc_hwc_h_ */

