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




#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "gc_hwc.h"
#include "gc_hwc_debug.h"

#include <gc_hal.h>
#include <gc_hal_driver.h>
#include <gc_gralloc_priv.h>

#include <linux/fb.h>

#include <stdlib.h>
#include <errno.h>


static hwcArea *
_AllocateArea(
    IN hwcContext * Context,
    IN hwcArea * Slibing,
    IN gcsRECT * Rect,
    IN gctUINT32 Owner
    );

static void
_FreeArea(
    IN hwcContext * Context,
    IN hwcArea *    Head
    );

static void
_SplitArea(
    IN hwcContext * Context,
    IN hwcArea * Area,
    IN gcsRECT * Rect,
    IN gctUINT32 Owner
    );

static gceTILING
_TranslateTiling(
    IN gceSURF_TYPE Type
    );

static gctBOOL
_HasAlpha(
    IN gceSURF_FORMAT Format
    );

static void
_ComputeUVAddress(
    IN  gceSURF_FORMAT Format,
    IN  gctUINT32      Physical,
    IN  gctUINT32      Height,
    IN  gctUINT32      Stride,
    OUT gctUINT32      Addresses[3],
    OUT gctUINT32_PTR  AdressNum,
    OUT gctUINT32      Stridees[3],
    OUT gctUINT32_PTR  StrideNum
    );


/*******************************************************************************
**
**  hwcSet
**
**  Translate android composition parameters to hardware specific.
**
**  For 2D compsition, it will translate android parameters to GC parameters
**  and start GC 2D operations.
**
**  For 3D compsition when failed to use hwc composition, this function will
*   start overlay only.
**
**  Swap rectangle change is not indicated by geometry chagne, so swap rect
**  clamp is not done in parameter generation.
**
**
**  INPUT:
**
**      hwcContext * Context
**          hwcomposer context pointer.
**
**      android_native_buffer_t * BackBuffer
**          The display back buffer to render to.
**
**      hwc_layer_list_t * List
**          All layers to be composed.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
hwcSet(
    IN hwcContext * Context,
    IN android_native_buffer_t * BackBuffer,
    IN hwc_layer_list_t * List
    )
{
    gceSTATUS status = gcvSTATUS_OK;


    /***************************************************************************
    ** Framebuffer Detection.
    */

    if (Context->hasComposition)
    {
        /* Get GC handle. */
        gc_private_handle_t * handle = (gc_private_handle_t *) BackBuffer->handle;

        /* Get shortcuts. */
        hwcFramebuffer * framebuffer = Context->framebuffer;

        hwcBuffer * target;

        gcsSURF_FORMAT_INFO_PTR info[2];

        /* Detect framebuffer information. */
        if (framebuffer == NULL)
        {
            /* Framebuffer resolution. */
            gctUINT32 xres;
            gctUINT32 yres;

            /* Allocate memory for framebuffer struct. */
            framebuffer = (hwcFramebuffer *) malloc( sizeof (hwcFramebuffer));

            /* Get surface wrapper. */
            gcoSURF surface = handle->surface != 0 ? (gcoSURF) handle->surface
                            : (gcoSURF) handle->resolveSurface;

            /* Get framebuffer size. */
            gcmVERIFY_OK(
                gcoSURF_GetSize(surface,
                                (gctUINT32 *) &xres,
                                (gctUINT32 *) &yres,
                                gcvNULL));

            /* Save framebuffer resolution. */
            framebuffer->res.left   = 0;
            framebuffer->res.top    = 0;
            framebuffer->res.right  = xres;
            framebuffer->res.bottom = yres;

            /* Get stride. */
            gcmVERIFY_OK(
                gcoSURF_GetAlignedSize(surface,
                                       gcvNULL,
                                       gcvNULL,
                                       (gctINT_PTR) &framebuffer->stride));

            /* TODO: Get tiling. */
            framebuffer->tiling = gcvLINEAR;

            /* Get format. */
            gcmVERIFY_OK(
                gcoSURF_GetFormat(surface,
                                  gcvNULL,
                                  &framebuffer->format));

            /* Query format info. */
            gcmVERIFY_OK(
                gcoSURF_QueryFormat(framebuffer->format, info));

            /* Get bytes per pixel. */
            framebuffer->bytesPerPixel = info[0]->bitsPerPixel / 8;

            /* Clear valid flag. */
            framebuffer->valid = gcvFALSE;

            /* Allocate target framebuffer buffer. */
            target = (hwcBuffer *) malloc(sizeof (hwcBuffer));

            /* Save handle. */
            target->handle   = handle;

            /* Save physical address. */
            target->physical = (gctUINT32) handle->phys;

            /* Point prev/next buffer to self. */
            target->prev = target->next = target;

            /* Save target handle to framebuffer. */
            framebuffer->head = framebuffer->target = target;

            /* Save framebuffer handle to context. */
            Context->framebuffer    = framebuffer;

            /* Logs. */
            LOGI("  Framebuffer(%p) detected: format=%d %dx%d",
                 framebuffer, framebuffer->format, xres, yres);

            LOGI("  Framebuffer buffer(%p) detected: physical=0x%08x",
                 target, target->physical);
        }

        /* Get target framebuffer buffer. */
        target = framebuffer->target = framebuffer->target->next;

        /* Check physical address. */
        if (target->physical != (gctUINT32) handle->phys)
        {
            /* Buffer physical is not the same as address pointed by the,
             * handle, we need to check again.
             * This case may be because some frames composed by 3D or a new
             * buffer detected. */
            do
            {
                if (target->physical == (gctUINT32) handle->phys)
                {
                    /* Found. */
                    framebuffer->target = target;

                    break;
                }

                target = target->next;
            }
            while (target != framebuffer->target);
        }

        /* Check physical address. */
        if (target->physical != (gctUINT32) handle->phys)
        {
            /* Allocate new buffer when not found. */
            target = (hwcBuffer *) malloc(sizeof (hwcBuffer));

            /* Save buffer handle. */
            target->handle   = handle;

            /* Get buffer address. */
            target->physical = (gctUINT32) handle->phys;

            /* Insert the new buffer to proper place. */
            if (target->physical < framebuffer->head->physical)
            {
                /* This is the first framebuffer buffer.
                 * Update framebuffer head. */
                hwcBuffer * prev = framebuffer->head->prev;

                /* Set the new buffer to header. */
                framebuffer->head->prev = target;
                target->next            = framebuffer->head;

                prev->next              = target;
                target->prev            = prev;

                framebuffer->head       = target;
            }

            else
            {
                /* Insert the buffer to corrent point. */
                hwcBuffer * b = framebuffer->head->next;

                while (b != framebuffer->head)
                {
                    if ((b->physical < target->physical)
                    &&  (target->physical < b->next->physical)
                    )
                    {
                        /* Found. */
                        break;
                    }

                    b = b->next;
                }

                if (b == framebuffer->head)
                {
                    /* Can not find, insert it before header. */
                    b = framebuffer->head->prev;
                }

                /* Insert the buffer between b and b->next. */
                b->next->prev = target;
                target->next  = b->next;

                b->next       = target;
                target->prev  = b;
            }

            /* Update target buffer. */
            framebuffer->target  = target;

            /* Logs. */
            LOGI("  Framebuffer buffer(%p) detected: physical=0x%08x",
                 target, target->physical);
        }

#if ENABLE_SWAP_RECTANGLE

        /* Get swap rectangle from android_native_buffer_t. */
        gctUINT32 origin = (gctUINT32) BackBuffer->common.reserved[0];
        gctUINT32 size   = (gctUINT32) BackBuffer->common.reserved[1];

        /* Update swap rectangle. */
        if (origin == 0 && size == 0)
        {
            /* This means full screen swap rectangle. */
            target->swapRect = framebuffer->res;
        }

        else
        {
            target->swapRect.left   = (origin >> 16);
            target->swapRect.top    = (origin &  0xFFFF);
            target->swapRect.right  = (origin >> 16) + (size >> 16);
            target->swapRect.bottom = (origin &  0xFFFF) + (size & 0xFFFF);
        }
#else

        /* Always set to full screen swap rectangle. */
        target->swapRect = framebuffer->res;
#endif

        /* Update valid flag. */
        if (framebuffer->valid == gcvFALSE)
        {
            /* Reset swap rectangles of other buffers. */
            hwcBuffer * buffer = target;

            while (buffer != target)
            {
                /* Reset size to full screen. */
                buffer->swapRect = framebuffer->res;

                buffer = buffer->next;
            }

            /* Set valid falg. */
            framebuffer->valid = gcvTRUE;
        }
    }

    else if (Context->framebuffer != NULL)
    {
        /* 3D composition path, remove the valid flag. */
        Context->framebuffer->valid = gcvFALSE;
    }


    /***************************************************************************
    ** Update Layer Information and Geometry.
    */

    if (Context->geometryChanged && Context->hasComposition)
    {
        /* Geometry changed and has composition. */

        /* Update layer count. */
        Context->layerCount = List->numHwLayers;

        /* Generate new layers. */
        for (gctUINT32 i = 0; i < Context->layerCount; i++)
        {
            /* Get shortcuts. */
            hwc_layer_t *  hwLayer = &List->hwLayers[i];
            hwcLayer *     layer   = &Context->layers[i];

            gcsSURF_FORMAT_INFO_PTR info[2];

            gc_private_handle_t * handle;
            gcoSURF surface;
            gceSURF_TYPE type;

            gcsRECT * srcRect;
            gcsRECT * sourceCrop;

            gctBOOL perpixelAlpha;
            gctUINT32 planeAlpha;

            gctINT  top;
            gctINT  bottom;
            gctBOOL yflip;

            /* Pass composition type. */
            layer->compositionType = hwLayer->compositionType;

            /* Dest rectangle. */
            layer->dstRect         = *(gcsRECT *) &hwLayer->displayFrame;

            switch (hwLayer->compositionType)
            {
            case HWC_OVERLAY:

                /* Set no source surface. Color should be from solid color32. */
                layer->source = gcvNULL;

                /* Normally transparent black for overlay area. */
                layer->color32 = 0U;

                /* Disable alpha blending. */
                layer->opaque  = gcvTRUE;

                break;

            case HWC_BLITTER:

                /* Get source handle. */
                handle = (gc_private_handle_t *) hwLayer->handle;

                /* Get source surface. */
                surface = handle->surface != 0 ? (gcoSURF) handle->surface
                        : (gcoSURF) handle->resolveSurface;

                /* Set source color from surface. */
                layer->source = surface;

                /* Get source surface stride. */
                gcmVERIFY_OK(
                    gcoSURF_GetAlignedSize(surface,
                                           gcvNULL, gcvNULL,
                                           (gctINT32 *) layer->strides));

                /* Get source surface format. */
                gcmVERIFY_OK(
                    gcoSURF_GetFormat(surface, &type, &layer->format));

                /* Query format info. */
                gcmVERIFY_OK(
                    gcoSURF_QueryFormat(layer->format, info));

                /* Get bytes per pixel. */
                layer->bytesPerPixel = info[0]->bitsPerPixel / 8;

                /* Check YV12 format. */
                layer->yuv  = (layer->format >= gcvSURF_YUY2)
                           && (layer->format <= gcvSURF_NV61);

                /* Clear u,v address and stride for RGBA formats. */
                if (layer->yuv == gcvFALSE)
                {
                    layer->addresses[1] = layer->addresses[2] = 0U;
                    layer->strides[1]   = layer->strides[2]   = 0U;

                    layer->addressNum   = layer->strideNum    = 1U;
                }

                /* Get source surface tiling.
                 * TODO: Replace it with gcoSURF_GetTiling() */
                layer->tiling = _TranslateTiling(type);

                /* Get source surface size. */
                gcmVERIFY_OK(
                    gcoSURF_GetSize(surface,
                                    &layer->width,
                                    &layer->height,
                                    gcvNULL));

                /* Get shortcuts. */
                srcRect    = &layer->srcRect;
                sourceCrop = (gcsRECT *) &hwLayer->sourceCrop;

                /* Get source surface orientation. */
                gcmVERIFY_OK(
                    gcoSURF_QueryOrientation(surface, &layer->orientation));

                /* Support bottom-top orientation. */
                if (layer->orientation == gcvORIENTATION_TOP_BOTTOM)
                {
                    top    = sourceCrop->top;
                    bottom = sourceCrop->bottom;
                    yflip  = gcvFALSE;
                }

                else
                {
                    top    = layer->height - sourceCrop->bottom;
                    bottom = layer->height - sourceCrop->top;
                    yflip  = gcvTRUE;
                }

                /* Get mirror/rotation states and srcRect.
                 * Android is given src rectangle in original coord system
                 * before trasformation. But G2D needs src rectangle given in
                 * transformed coord system (source coord system). G2D also
                 * needs dest rectangle given in transformed coord system.
                 * But we never make dest rotated. Dest rectangle given by
                 * android is directly suitable for G2D (dest coord system).
                 * Between source coord system and dest coord system, there's
                 * only stretching or traslating.
                 * Here, it is to calculation src rectangle from original coord
                 * system to source coord system. */
                switch (hwLayer->transform)
                {
                case 0:
                    srcRect->left   = sourceCrop->left;
                    srcRect->top    = top;
                    srcRect->right  = sourceCrop->right;
                    srcRect->bottom = bottom;

                    layer->hMirror  = gcvFALSE;
                    layer->vMirror  = yflip;
                    layer->rotation = gcvSURF_0_DEGREE;
                    break;

                case HWC_TRANSFORM_FLIP_H:
                    srcRect->left   = sourceCrop->left;
                    srcRect->top    = top;
                    srcRect->right  = sourceCrop->right;
                    srcRect->bottom = bottom;

                    layer->hMirror  = gcvTRUE;
                    layer->vMirror  = yflip;
                    layer->rotation = gcvSURF_0_DEGREE;
                    break;

                case HWC_TRANSFORM_FLIP_V:
                    srcRect->left   = sourceCrop->left;
                    srcRect->top    = top;
                    srcRect->right  = sourceCrop->right;
                    srcRect->bottom = bottom;

                    layer->hMirror  = gcvFALSE;
                    layer->vMirror  = !yflip;
                    layer->rotation = gcvSURF_0_DEGREE;
                    break;

                case HWC_TRANSFORM_ROT_90:
                    srcRect->left   = layer->height - bottom;
                    srcRect->top    = sourceCrop->left,
                    srcRect->right  = layer->height - top;
                    srcRect->bottom = sourceCrop->right;

                    layer->hMirror  = yflip;
                    layer->vMirror  = gcvFALSE;
                    layer->rotation = gcvSURF_270_DEGREE;
                    break;

                case HWC_TRANSFORM_ROT_180:
                    srcRect->left   = layer->width - sourceCrop->right;
                    srcRect->top    = top;
                    srcRect->right  = layer->width - sourceCrop->left;
                    srcRect->bottom = bottom;

                    layer->hMirror  = gcvFALSE;
                    layer->vMirror  = yflip;
                    layer->rotation = gcvSURF_180_DEGREE;
                    break;

                case HWC_TRANSFORM_ROT_270:
                    srcRect->left   = top;
                    srcRect->top    = layer->width  - sourceCrop->right;
                    srcRect->right  = bottom;
                    srcRect->bottom = layer->width  - sourceCrop->left;

                    layer->hMirror  = yflip;
                    layer->vMirror  = gcvFALSE;
                    layer->rotation = gcvSURF_90_DEGREE;
                    break;

                case (HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_H):
                    srcRect->left   = layer->height - bottom;
                    srcRect->top    = sourceCrop->left,
                    srcRect->right  = layer->height - top;
                    srcRect->bottom = sourceCrop->right;

                    layer->hMirror  = yflip;
                    layer->vMirror  = gcvTRUE;
                    layer->rotation = gcvSURF_270_DEGREE;
                    break;

                case (HWC_TRANSFORM_ROT_90 | HWC_TRANSFORM_FLIP_V):
                    srcRect->left   = layer->height - bottom;
                    srcRect->top    = sourceCrop->left,
                    srcRect->right  = layer->height - top;
                    srcRect->bottom = sourceCrop->right;

                    layer->hMirror  = !yflip;
                    layer->vMirror  = gcvFALSE;
                    layer->rotation = gcvSURF_270_DEGREE;
                    break;

                default:
                    /* Should never happen. */
                    gcmONERROR(gcvSTATUS_INVALID_DATA);
                    break;
                }

                /* Save original rectangle. */
                layer->orgRect = *(gcsRECT *) &hwLayer->sourceCrop;

                /* Get alpha blending/premultiply states. */
                switch (hwLayer->blending & 0xFFFF)
                {
                case HWC_BLENDING_PREMULT:

                    /* Get perpixelAlpha enabling state. */
                    perpixelAlpha = _HasAlpha(layer->format);

                    /* Get plane alpha value. */
                    planeAlpha    = hwLayer->blending >> 16;

                    /* Alpha blending is needed. */
                    layer->opaque = gcvFALSE;

                    if (perpixelAlpha && planeAlpha < 0xFF)
                    {
                        /* Perpixel alpha and plane alpha. */
                        layer->srcAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                        layer->dstAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                        layer->srcGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_SCALE;
                        layer->dstGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_OFF;
                        layer->srcFactorMode        = gcvSURF_BLEND_ONE;
                        layer->dstFactorMode        = gcvSURF_BLEND_INVERSED;

                        /* Premultiply with src global alpha. */
                        layer->srcPremultSrcAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                        layer->dstPremultDstAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                        layer->srcPremultGlobalMode = gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA;
                        layer->dstDemultDstAlpha    = gcv2D_COLOR_MULTIPLY_DISABLE;

                        /* Global alpha values. */
                        layer->srcGlobalAlpha       = planeAlpha << 24;
                        layer->dstGlobalAlpha       = planeAlpha << 24;
                    }

                    else if (perpixelAlpha)
                    {
                        /* Perpixel alpha only. */
                        layer->srcAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                        layer->dstAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                        layer->srcGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_OFF;
                        layer->dstGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_OFF;
                        layer->srcFactorMode        = gcvSURF_BLEND_ONE;
                        layer->dstFactorMode        = gcvSURF_BLEND_INVERSED;

                        /* Disable premultiply. */
                        layer->srcPremultSrcAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                        layer->dstPremultDstAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                        layer->srcPremultGlobalMode = gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE;
                        layer->dstDemultDstAlpha    = gcv2D_COLOR_MULTIPLY_DISABLE;

                        /* Global alpha values. */
                        layer->srcGlobalAlpha       = 0xFF000000;
                        layer->dstGlobalAlpha       = 0xFF000000;
                    }

                    else
                    {
                        /* Plane alpha only. */
                        layer->srcAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                        layer->dstAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                        layer->srcGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_ON;
                        layer->dstGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_OFF;
                        layer->srcFactorMode        = gcvSURF_BLEND_ONE;
                        layer->dstFactorMode        = gcvSURF_BLEND_INVERSED;

                        /* Premultiply with src global alpha. */
                        layer->srcPremultSrcAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                        layer->dstPremultDstAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                        layer->srcPremultGlobalMode = gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA;
                        layer->dstDemultDstAlpha    = gcv2D_COLOR_MULTIPLY_DISABLE;

                        /* Global alpha values. */
                        layer->srcGlobalAlpha       = planeAlpha << 24;
                        layer->dstGlobalAlpha       = planeAlpha << 24;
                    }

                    break;

                case HWC_BLENDING_COVERAGE:

                    LOGW("COVERAGE alpha blending...");
                    /* Get plane alpha value. */
                    planeAlpha    = hwLayer->blending >> 16;

                    /* Alpha blending is needed. */
                    layer->opaque = gcvFALSE;

                    /* Alpha blending parameters.
                     * TODO: this may be incorrect. */
                    layer->srcAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                    layer->dstAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                    layer->srcGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_ON;
                    layer->dstGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_OFF;
                    layer->srcFactorMode        = gcvSURF_BLEND_ONE;
                    layer->dstFactorMode        = gcvSURF_BLEND_INVERSED;

                    /* Premultiply with src global alpha. */
                    layer->srcPremultSrcAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                    layer->dstPremultDstAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                    layer->srcPremultGlobalMode = gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA;
                    layer->dstDemultDstAlpha    = gcv2D_COLOR_MULTIPLY_DISABLE;

                    /* Global alpha values. */
                    layer->srcGlobalAlpha       = planeAlpha << 24;
                    layer->dstGlobalAlpha       = planeAlpha << 24;

                    break;

                case HWC_BLENDING_NONE:
                default:

                    /* Alpha blending is not needed. */
                    layer->opaque = gcvTRUE;

                    /* Disable premultiply. */
                    layer->srcPremultSrcAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                    layer->dstPremultDstAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                    layer->srcPremultGlobalMode = gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE;
                    layer->dstDemultDstAlpha    = gcv2D_COLOR_MULTIPLY_DISABLE;

                    /* Global alpha values. */
                    layer->srcGlobalAlpha       = 0xFF000000;
                    layer->dstGlobalAlpha       = 0xFF000000;

                    break;
                }

                /* Compute stretch factor. */
                layer->hfactor = (gctFLOAT) (srcRect->right - srcRect->left)
                               / (layer->dstRect.right - layer->dstRect.left);

                layer->vfactor = (gctFLOAT) (srcRect->bottom - srcRect->top)
                               / (layer->dstRect.bottom - layer->dstRect.top);

                /* Get stretch flag. */
                layer->stretch = (layer->hfactor != 1.0f)
                              || (layer->vfactor != 1.0f);

                /* Determine filter blit kernel size. */
                if (layer->yuv)
                {
                    layer->hkernel = (layer->hfactor == 1.0f / 1) ? 1U
                                   : (layer->hfactor >= 1.0f / 2) ? 3U
                                   : (layer->hfactor >= 1.0f / 3) ? 5U
                                   : (layer->hfactor >= 1.0f / 4) ? 7U
                                   : 9U;

                    layer->vkernel = (layer->vfactor == 1.0f / 1) ? 1U
                                   : (layer->vfactor >= 1.0f / 2) ? 3U
                                   : (layer->vfactor >= 1.0f / 3) ? 5U
                                   : (layer->vfactor >= 1.0f / 4) ? 7U
                                   : 9U;
                }

                break;

#if ENABLE_CLEAR_HOLE

            case HWC_CLEAR_HOLE:

                /* Set no source surface. Color should be from solid color32. */
                layer->source     = gcvNULL;

                /* Opaque black for clear hole. */
                layer->color32    = 0xFF000000;

                /* Disable alpha blending. */
                layer->opaque     = gcvTRUE;

                /* Disable premutiply. */
                layer->srcPremultSrcAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                layer->dstPremultDstAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                layer->srcPremultGlobalMode = gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE;
                layer->dstDemultDstAlpha    = gcv2D_COLOR_MULTIPLY_DISABLE;

                break;
#endif

            case HWC_DIM:

                /* Set no source surface. Color should be from solid color32. */
                layer->source  = gcvNULL;

                /* Get alpha channel. */
                layer->color32 = (hwLayer->blending & 0xFF0000) << 8;

                if (layer->color32 == 0xFF000000)
                {
                    /* Make a clear without alpha blending when alpha is 0xFF. */
                    layer->opaque  = gcvTRUE;

                    /* Disable premutiply. */
                    layer->srcPremultSrcAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                    layer->dstPremultDstAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                    layer->srcPremultGlobalMode = gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE;
                    layer->dstDemultDstAlpha    = gcv2D_COLOR_MULTIPLY_DISABLE;
                }

                else
                {
                    /* Do a blit with alpha blending. */
                    layer->opaque = gcvFALSE;

                    /* Alpha blending parameters. */
                    layer->srcAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                    layer->dstAlphaMode         = gcvSURF_PIXEL_ALPHA_STRAIGHT;
                    layer->srcGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_OFF;
                    layer->dstGlobalAlphaMode   = gcvSURF_GLOBAL_ALPHA_OFF;
                    layer->srcFactorMode        = gcvSURF_BLEND_ONE;
                    layer->dstFactorMode        = gcvSURF_BLEND_INVERSED;

                    /* Disable premutiply. */
                    layer->srcPremultSrcAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                    layer->dstPremultDstAlpha   = gcv2D_COLOR_MULTIPLY_DISABLE;
                    layer->srcPremultGlobalMode = gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE;
                    layer->dstDemultDstAlpha    = gcv2D_COLOR_MULTIPLY_DISABLE;
                }

                /* Global alpha values. */
                layer->srcGlobalAlpha       = 0xFF000000;
                layer->dstGlobalAlpha       = 0xFF000000;

                break;

            default:
                break;
            }
        }

        /* Reset allocated areas. */
        if (Context->compositionArea != NULL)
        {
            _FreeArea(Context, Context->compositionArea);

            Context->compositionArea = NULL;
#if ENABLE_SWAP_RECTANGLE
            Context->swapArea        = NULL;
#endif
        }

        /* Generate new areas. */
        /* Put a no-owner area with screen size, this is for worm hole,
         * and is needed for clipping. */
        Context->compositionArea = _AllocateArea(Context,
                                                 NULL,
                                                 &Context->framebuffer->res,
                                                 0U);

        /* Split areas: go through all regions. */
        for (gctUINT32 i = 0; i < List->numHwLayers; i++)
        {
            gctUINT32 owner = 1U << i;
            hwc_layer_t *  hwLayer = &List->hwLayers[i];
            hwc_region_t * region  = &hwLayer->visibleRegionScreen;

            /* Now go through all rectangles to split areas. */
            for (gctUINT32 j = 0; j < region->numRects; j++)
            {
                /* Assume the region will never go out of dest surface. */
                _SplitArea(Context,
                           Context->compositionArea,
                           (gcsRECT *) &region->rects[j],
                           owner);
            }
        }
    }

#if ENABLE_CLEAR_HOLE

    /***************************************************************************
    ** 'CLEAR_HOLE' Corretion.
    */

    if (Context->hasComposition && Context->geometryChanged
    &&  Context->hasClearHole
    )
    {
        /* If some layer has compositionType 'CLEAR_HOLE', but it has one or
         * more layers below it, skip 'CLEAR_HOLE' operation.
         * See 'no texture' section in drawWithOpenGL in LayerBase.cpp. */
        for (gctUINT32 i = 0; i < Context->layerCount; i++)
        {
            if (Context->layers[i].compositionType == HWC_CLEAR_HOLE)
            {
                /* Find a layer with clear hole. */
                gctUINT32 owner = (1U << i);

                hwcArea * area = Context->compositionArea;

                while (area != NULL)
                {
                    if ((area->owners & owner) && (area->owners & (owner - 1U)))
                    {
                        /* This area has clear hole layer, but it also has
                         * other layers below it. So it should NOT be
                         * cleared */
                        area->owners &= ~owner;
                    }

                    /* Advance to next area. */
                    area = area->next;
                }
            }
        }
    }
#endif

#if ENABLE_DIM

    /***************************************************************************
    ** 'DIM' Optimization.
    */

    if (Context->hasComposition && Context->geometryChanged
    &&  Context->hasDim
    )
    {
        /* DIM value 255 is very special. It does not do any dim effection but
         * only make a clear to (0,0,0,1). So we do not need to draw any layers
         * below it.  See LayerDim::onDraw. */
        for (gctUINT32 i = 0; i < Context->layerCount; i++)
        {
            /* TODO: Maybe we can check all opaque layers like this. */
            if (Context->layers[i].compositionType == HWC_DIM
            &&  Context->layers[i].opaque
            )
            {
                /* Find a layer with solid dim. */
                gctUINT32 owner = (1U << i);

                hwcArea * area = Context->compositionArea;

                while (area != NULL)
                {
                    if ((area->owners & owner) && (area->owners & (owner - 1U)))
                    {
                        /* This area has solid dim, but it also has
                         * other layers below it. We do not need to blit
                         * layers below it. */
                        area->owners &= ~(owner - 1U);
                    }

                    /* Advance to next area. */
                    area = area->next;
                }
            }
        }
    }
#endif

#if CLEAR_FB_FOR_OVERLAY

    /***************************************************************************
    ** 'CLEAR_FB' Detection.
    */

    if (Context->hasComposition && Context->geometryChanged
    &&  Context->hasOverlay
    )
    {
        /* We Always need to clear when tagged with overlay. So if one layer
         * the area is 'OVERLAY', we skip other layers. */
        for (gctUINT32 i = 0; i < Context->layerCount; i++)
        {
            if (Context->layers[i].compositionType == HWC_OVERLAY)
            {
                /* Find a layer with clear hole. */
                gctUINT32 owner = (1U << i);

                hwcArea * area = Context->compositionArea;

                while (area != NULL)
                {
                    if ((area->owners & owner) && (area->owners & ~owner))
                    {
                        /* Do not need clear fb layer if other non-overlay
                         * layers are there. */
                        area->owners &= ~owner;
                    }

                    /* Advance to next area. */
                    area = area->next;
                }
            }
        }
    }
#endif

    /***************************************************************************
    ** Source Buffer Detection.
    */

    if (Context->hasComposition)
    {
        /* Geometry is not changed and we are doing 2D composition. So we need
         * to update source for each layer. */
        for (gctUINT32 i = 0; i < List->numHwLayers; i++)
        {
            /* Get shortcuts. */
            hwcLayer * layer = &Context->layers[i];

            /* Update layer source (for blitter layer). */
            if (layer->source != gcvNULL)
            {
                gc_private_handle_t * handle
                    = (gc_private_handle_t *) List->hwLayers[i].handle;

                /* Assume the two(or more) buffers are same in size,
                 * format, tiling etc. */
                if (layer->yuv)
                {
                    /* Compute addresses from base address. */
                    _ComputeUVAddress(layer->format,
                                      (gctUINT32) handle->phys,
                                      layer->height,
                                      layer->strides[0],
                                      layer->addresses,
                                      &layer->addressNum,
                                      layer->strides,
                                      &layer->strideNum);
                }
                else
                {
                    /* Get source surface base address. */
                    layer->addresses[0] = (gctUINT32) handle->phys;
                }

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
                if ((handle->allocUsage & GRALLOC_USAGE_HW_RENDER)
                &&  (layer->yuv == gcvFALSE)
                &&  (layer->tiling == gcvLINEAR)
                )
                {
                    gctUINT32 alignedHeight = gcmALIGN(layer->height, 8);

                    layer->addresses[0] += layer->strides[0]
                                         * (alignedHeight - layer->height);
                }
#endif
            }
        }

        /* Clear geometry change flag. */
        Context->geometryChanged = gcvFALSE;
    }

#if ENABLE_SWAP_RECTANGLE

    /***************************************************************************
    ** Swap Rectangle Optimization.
    */

    if (Context->hasComposition)
    {
        /* Get short cuts. */
        hwcFramebuffer * framebuffer = Context->framebuffer;
        hwcBuffer * buffer           = framebuffer->target;

        /* Free all allocated swap areas. */
        if (Context->swapArea != NULL)
        {
            _FreeArea(Context, Context->swapArea);
            Context->swapArea = NULL;
        }

        /* Optimize: do not need split area for full screen composition. */
        if (
            (   (buffer->swapRect.left > 0)
            ||  (buffer->swapRect.top  > 0)
            ||  (buffer->swapRect.right  < framebuffer->res.right)
            ||  (buffer->swapRect.bottom < framebuffer->res.bottom)
            )
        &&  (buffer != buffer->next)
        )
        {
            /* Put target swap rectangle. */
            Context->swapArea = _AllocateArea(Context,
                                              NULL,
                                              &buffer->swapRect,
                                              1U);

            /* Point to earlier buffer. */
            buffer = buffer->next;

            /* Split area with rectangles of earlier buffer(with no-owner).
             * Now owner 0 means we need copy area from front buffer.
             * 1 means target swap rectangle. */
            while (buffer != framebuffer->target)
            {
                _SplitArea(Context, Context->swapArea, &buffer->swapRect, 0U);

                /* Advance to next framebuffer buffer. */
                buffer = buffer->next;
            }
        }
    }
#endif


    /***************************************************************************
    ** Debug and Dumps.
    */

#if DUMP_LAYER

    LOGD("LAYERS:");
    hwcDumpLayer(List);
#endif

#if DUMP_BITMAP

    if (Context->hasComposition)
    {
        hwcDumpBitmap(Context->layerCount, Context->layers);
    }
#endif

#if DUMP_SPLIT_AREA

    if (Context->hasComposition)
    {
        LOGD("SPLITED AREA:");
        hwcDumpArea(Context->compositionArea);
    }
#endif

#if DUMP_SWAP_AREA

    if (Context->hasComposition)
    {
        LOGD("SWAP AREA:");
        hwcDumpArea(Context->swapArea);
    }
#endif


    /***************************************************************************
    ** Do Compose.
    */

    if (Context->hasComposition)
    {
        /* Start composition if we have hwc composition. */
        gcmONERROR(
            hwcCompose(Context));
    }


    /***************************************************************************
    ** Do Overlay.
    */

    /* Compose overlay layers. */
    if (Context->hasOverlay)
    {
        if (hwcOverlay(Context, List) != 0)
        {
            LOGE("%s(%d): failed in setting overlay", __FUNCTION__, __LINE__);
        }
    }

    return gcvSTATUS_OK;

OnError:
    /* Error. */
    LOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


static gceTILING
_TranslateTiling(
    IN gceSURF_TYPE Type
    )
{
    /* TODO: Better way to get tiling info. */
    switch (Type)
    {
    case gcvSURF_TEXTURE:

       return gcvTILED;
       break;

    case gcvSURF_RENDER_TARGET:
    case gcvSURF_DEPTH:

       return gcvMULTI_SUPERTILED;
       break;

    case gcvSURF_BITMAP:
    default:

       return gcvLINEAR;
    }
}


static gctBOOL
_HasAlpha(
    IN gceSURF_FORMAT Format
    )
{
    return (Format == gcvSURF_R5G6B5) ? gcvFALSE
         : (
               (Format == gcvSURF_A8R8G8B8)
            || (Format == gcvSURF_A8B8G8R8)
            || (Format == gcvSURF_B8G8R8A8)
            || (Format == gcvSURF_R8G8B8A8)
           );
}


static void
_ComputeUVAddress(
    IN  gceSURF_FORMAT Format,
    IN  gctUINT32      Physical,
    IN  gctUINT32      Height,
    IN  gctUINT32      Stride,
    OUT gctUINT32      Addresses[3],
    OUT gctUINT32_PTR  AddressNum,
    OUT gctUINT32      Strides[3],
    OUT gctUINT32_PTR  StrideNum
    )
{
    gctUINT32 uPhysical = 0U;
    gctUINT32 vPhysical = 0U;
    gctUINT32 uStride   = 0U;
    gctUINT32 vStride   = 0U;

    switch (Format)
    {
    case gcvSURF_NV12:
    case gcvSURF_NV21:
    case gcvSURF_NV16:
    case gcvSURF_NV61:

        uStride   = vStride   = Stride;
        uPhysical = vPhysical = Physical + Stride * Height;
        *AddressNum = *StrideNum = 2U;
        break;

    case gcvSURF_YV12:

        /* This alignment limitation is from
         * frameworks/base/libs/gui/tests/SurfaceTexture_test.cpp. */
        uStride   = vStride   = (Stride / 2 + 0xf) & ~0xf;
        vPhysical =  Physical + Stride * Height;
        uPhysical = vPhysical + vStride * (Height / 2);
        *AddressNum = *StrideNum = 3U;
        break;

    case gcvSURF_I420:

        /* This alignment limitation is from
         * frameworks/base/libs/gui/tests/SurfaceTexture_test.cpp. */
        uStride   = vStride   = (Stride / 2 + 0xf) & ~0xf;
        uPhysical =  Physical + Stride * Height;
        vPhysical = uPhysical + uStride * (Height / 2);
        *AddressNum = *StrideNum = 3U;
        break;

    case gcvSURF_YUY2:
    case gcvSURF_UYVY:

        uPhysical = vPhysical = Physical;
        uStride   = vStride   = Stride;
        *AddressNum = *StrideNum = 1U;
        break;

    default:

        *AddressNum = *StrideNum = 1U;
        break;
    }

    /* Return results. */
    Addresses[0] =  Physical;
    Addresses[1] = uPhysical;
    Addresses[2] = vPhysical;

    Strides[0] =  Stride;
    Strides[1] = uStride;
    Strides[2] = vStride;
}


/*
 * Area spliting feature depends on the following 3 functions:
 * '_AllocateArea', '_FreeArea' and '_SplitArea'.
 */
#define POOL_SIZE 512

hwcArea *
_AllocateArea(
    IN hwcContext * Context,
    IN hwcArea * Slibing,
    IN gcsRECT * Rect,
    IN gctUINT32 Owner
    )
{
    hwcArea * area;
    hwcAreaPool * pool  = &Context->areaPool;

    for (;;)
    {
        if (pool->areas == NULL)
        {
            /* No areas allocated, allocate now. */
            pool->areas = (hwcArea *) malloc(sizeof (hwcArea) * POOL_SIZE);

            /* Get area. */
            area = pool->areas;

            /* Update freeNodes. */
            pool->freeNodes = area + 1;

            break;
        }

        else if (pool->freeNodes - pool->areas >= POOL_SIZE)
        {
            /* This pool is full. */
            if (pool->next == NULL)
            {
                /* No more pools, allocate one. */
                pool->next = (hwcAreaPool *) malloc(sizeof (hwcAreaPool));

                /* Point to the new pool. */
                pool = pool->next;

                /* Clear fields. */
                pool->areas     = NULL;
                pool->freeNodes = NULL;
                pool->next      = NULL;
            }

            else
            {
                /* Advance to next pool. */
                pool = pool->next;
            }
        }

        else
        {
            /* Get area and update freeNodes. */
            area = pool->freeNodes++;

            break;
        }
    }

    /* Update area fields. */
    area->rect   = *Rect;
    area->owners = Owner;

    if (Slibing == NULL)
    {
        area->next = NULL;
    }

    else if (Slibing->next == NULL)
    {
        area->next = NULL;
        Slibing->next = area;
    }

    else
    {
        area->next = Slibing->next;
        Slibing->next = area;
    }

    return area;
}


void
_FreeArea(
    IN hwcContext * Context,
    IN hwcArea* Head
    )
{
    /* Free the first node is enough. */
    hwcAreaPool * pool  = &Context->areaPool;

    while (pool != NULL)
    {
        if (Head >= pool->areas && Head < pool->areas + POOL_SIZE)
        {
            /* Belongs to this pool. */
            if (Head < pool->freeNodes)
            {
                /* Update freeNodes if the 'Head' is older. */
                pool->freeNodes = Head;

                /* Reset all later pools. */
                while (pool->next != NULL)
                {
                    /* Advance to next pool. */
                    pool = pool->next;

                    /* Reset freeNodes. */
                    pool->freeNodes = pool->areas;
                }
            }

            /* Done. */
            break;
        }

        else if (pool->freeNodes < pool->areas + POOL_SIZE)
        {
            /* Already tagged as freed. */
            break;
        }

        else
        {
            /* Advance to next pool. */
            pool = pool->next;
        }
    }
}


void
_SplitArea(
    IN hwcContext * Context,
    IN hwcArea * Area,
    IN gcsRECT * Rect,
    IN gctUINT32 Owner
    )
{
    gcsRECT r0[4];
    gcsRECT r1[4];
    gctUINT32 c0 = 0;
    gctUINT32 c1 = 0;

    gcsRECT * rect;

    for (;;)
    {
        rect = &Area->rect;

        if ((Rect->left   < rect->right)
        &&  (Rect->top    < rect->bottom)
        &&  (Rect->right  > rect->left)
        &&  (Rect->bottom > rect->top)
        )
        {
            /* Overlapped. */
            break;
        }

        if (Area->next == NULL)
        {
            /* This rectangle is not overlapped with any area. */
            _AllocateArea(Context, Area, Rect, Owner);
            return;
        }

        Area = Area->next;
    }

    /* OK, the rectangle is overlapped with 'rect' area. */
    if ((Rect->left <= rect->left)
    &&  (Rect->right >= rect->right)
    )
    {
        /* |-><-| */
        /* +---+---+---+
         * | X | X | X |
         * +---+---+---+
         * | X | X | X |
         * +---+---+---+
         * | X | X | X |
         * +---+---+---+
         */

        if (Rect->left < rect->left)
        {
            r1[c1].left   = Rect->left;
            r1[c1].top    = Rect->top;
            r1[c1].right  = rect->left;
            r1[c1].bottom = Rect->bottom;

            c1++;
        }

        if (Rect->top < rect->top)
        {
            r1[c1].left   = rect->left;
            r1[c1].top    = Rect->top;
            r1[c1].right  = rect->right;
            r1[c1].bottom = rect->top;

            c1++;
        }

        else if (rect->top < Rect->top)
        {
            r0[c0].left   = rect->left;
            r0[c0].top    = rect->top;
            r0[c0].right  = rect->right;
            r0[c0].bottom = Rect->top;

            c0++;
        }

        if (Rect->right > rect->right)
        {
            r1[c1].left   = rect->right;
            r1[c1].top    = Rect->top;
            r1[c1].right  = Rect->right;
            r1[c1].bottom = Rect->bottom;

            c1++;
        }

        if (Rect->bottom > rect->bottom)
        {
            r1[c1].left   = rect->left;
            r1[c1].top    = rect->bottom;
            r1[c1].right  = rect->right;
            r1[c1].bottom = Rect->bottom;

            c1++;
        }

        else if (rect->bottom > Rect->bottom)
        {
            r0[c0].left   = rect->left;
            r0[c0].top    = Rect->bottom;
            r0[c0].right  = rect->right;
            r0[c0].bottom = rect->bottom;

            c0++;
        }
    }

    else if (Rect->left <= rect->left)
    {
        /* |-> */
        /* +---+---+---+
         * | X | X |   |
         * +---+---+---+
         * | X | X |   |
         * +---+---+---+
         * | X | X |   |
         * +---+---+---+
         */

        if (Rect->left < rect->left)
        {
            r1[c1].left   = Rect->left;
            r1[c1].top    = Rect->top;
            r1[c1].right  = rect->left;
            r1[c1].bottom = Rect->bottom;

            c1++;
        }

        if (Rect->top < rect->top)
        {
            r1[c1].left   = rect->left;
            r1[c1].top    = Rect->top;
            r1[c1].right  = Rect->right;
            r1[c1].bottom = rect->top;

            c1++;
        }

        else if (rect->top < Rect->top)
        {
            r0[c0].left   = rect->left;
            r0[c0].top    = rect->top;
            r0[c0].right  = Rect->right;
            r0[c0].bottom = Rect->top;

            c0++;
        }

        /* if (rect->right > Rect->right) */
        {
            r0[c0].left   = Rect->right;
            r0[c0].top    = rect->top;
            r0[c0].right  = rect->right;
            r0[c0].bottom = rect->bottom;

            c0++;
        }

        if (Rect->bottom > rect->bottom)
        {
            r1[c1].left   = rect->left;
            r1[c1].top    = rect->bottom;
            r1[c1].right  = Rect->right;
            r1[c1].bottom = Rect->bottom;

            c1++;
        }

        else if (rect->bottom > Rect->bottom)
        {
            r0[c0].left   = rect->left;
            r0[c0].top    = Rect->bottom;
            r0[c0].right  = Rect->right;
            r0[c0].bottom = rect->bottom;

            c0++;
        }
    }

    else if (Rect->right >= rect->right)
    {
        /*    <-| */
        /* +---+---+---+
         * |   | X | X |
         * +---+---+---+
         * |   | X | X |
         * +---+---+---+
         * |   | X | X |
         * +---+---+---+
         */

        /* if (rect->left < Rect->left) */
        {
            r0[c0].left   = rect->left;
            r0[c0].top    = rect->top;
            r0[c0].right  = Rect->left;
            r0[c0].bottom = rect->bottom;

            c0++;
        }

        if (Rect->top < rect->top)
        {
            r1[c1].left   = Rect->left;
            r1[c1].top    = Rect->top;
            r1[c1].right  = rect->right;
            r1[c1].bottom = rect->top;

            c1++;
        }

        else if (rect->top < Rect->top)
        {
            r0[c0].left   = Rect->left;
            r0[c0].top    = rect->top;
            r0[c0].right  = rect->right;
            r0[c0].bottom = Rect->top;

            c0++;
        }

        if (Rect->right > rect->right)
        {
            r1[c1].left   = rect->right;
            r1[c1].top    = Rect->top;
            r1[c1].right  = Rect->right;
            r1[c1].bottom = Rect->bottom;

            c1++;
        }

        if (Rect->bottom > rect->bottom)
        {
            r1[c1].left   = Rect->left;
            r1[c1].top    = rect->bottom;
            r1[c1].right  = rect->right;
            r1[c1].bottom = Rect->bottom;

            c1++;
        }

        else if (rect->bottom > Rect->bottom)
        {
            r0[c0].left   = Rect->left;
            r0[c0].top    = Rect->bottom;
            r0[c0].right  = rect->right;
            r0[c0].bottom = rect->bottom;

            c0++;
        }
    }

    else
    {
        /* | */
        /* +---+---+---+
         * |   | X |   |
         * +---+---+---+
         * |   | X |   |
         * +---+---+---+
         * |   | X |   |
         * +---+---+---+
         */

        /* if (rect->left < Rect->left) */
        {
            r0[c0].left   = rect->left;
            r0[c0].top    = rect->top;
            r0[c0].right  = Rect->left;
            r0[c0].bottom = rect->bottom;

            c0++;
        }

        if (Rect->top < rect->top)
        {
            r1[c1].left   = Rect->left;
            r1[c1].top    = Rect->top;
            r1[c1].right  = Rect->right;
            r1[c1].bottom = rect->top;

            c1++;
        }

        else if (rect->top < Rect->top)
        {
            r0[c0].left   = Rect->left;
            r0[c0].top    = rect->top;
            r0[c0].right  = Rect->right;
            r0[c0].bottom = Rect->top;

            c0++;
        }

        /* if (rect->right > Rect->right) */
        {
            r0[c0].left   = Rect->right;
            r0[c0].top    = rect->top;
            r0[c0].right  = rect->right;
            r0[c0].bottom = rect->bottom;

            c0++;
        }

        if (Rect->bottom > rect->bottom)
        {
            r1[c1].left   = Rect->left;
            r1[c1].top    = rect->bottom;
            r1[c1].right  = Rect->right;
            r1[c1].bottom = Rect->bottom;

            c1++;
        }

        else if (rect->bottom > Rect->bottom)
        {
            r0[c0].left   = Rect->left;
            r0[c0].top    = Rect->bottom;
            r0[c0].right  = Rect->right;
            r0[c0].bottom = rect->bottom;

            c0++;
        }
    }

    if (c1 > 0)
    {
        /* Process rects outside area. */
        if (Area->next == NULL)
        {
            /* Save rects outside area. */
            for (gctUINT32 i = 0; i < c1; i++)
            {
                _AllocateArea(Context, Area, &r1[i], Owner);
            }
        }

        else
        {
            /* Rects outside area. */
            for (gctUINT32 i = 0; i < c1; i++)
            {
                _SplitArea(Context, Area, &r1[i], Owner);
            }
        }
    }

    if (c0 > 0)
    {
        /* Save rects inside area but not overlapped. */
        for (gctUINT32 i = 0; i < c0; i++)
        {
            _AllocateArea(Context, Area, &r0[i], Area->owners);
        }

        /* Update overlapped area. */
        if (rect->left   < Rect->left)   { rect->left   = Rect->left;   }
        if (rect->top    < Rect->top)    { rect->top    = Rect->top;    }
        if (rect->right  > Rect->right)  { rect->right  = Rect->right;  }
        if (rect->bottom > Rect->bottom) { rect->bottom = Rect->bottom; }
    }

    /* The area is owned by the new owner as well. */
    Area->owners |= Owner;
}

