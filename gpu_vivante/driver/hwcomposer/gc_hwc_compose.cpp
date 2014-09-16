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




#include "gc_hwc.h"
#include "gc_hwc_debug.h"

#include <gc_hal_engine.h>
#include <gc_gralloc_priv.h>


/* Setup framebuffer source(for swap rectangle). */
static gceSTATUS
_SwapRectangle(
    IN hwcContext * Context
    );

/* Setup worm hole source. */
static gceSTATUS
_WormHole(
    IN hwcContext * Context,
    IN gcsRECT * Rect
    );

/* Setup blit source. */
static gceSTATUS
_Blit(
    IN hwcContext * Context,
    IN gctUINT32 Index,
    IN hwcArea * Area
    );

static gctUINT32
_GetSourceEigen(
    IN  hwcLayer * Layer,
    IN  gcsRECT *  DestRect
    );

/* Setup multi-source blit source. */
static gceSTATUS
_MultiSourceBlit(
    IN hwcContext * Context,
    IN gctUINT32 Indices[4],
    IN gctUINT32 Count,
    IN gctUINT32 EigenL,
    IN gctUINT32 EigenD,
    IN hwcArea * Area
    );


/*******************************************************************************
**
**  hwcCompose
**
**  Do actual composition in hwcomposer.
**
**  1. Do swap rectangle optimization if enabled.
**  2. Compose all layers which can be composed by hwcomposer: dim layer,
**     'clear hole' layer, normal layer and 'overlay clear'.
**  3. Fill worm holes with opaque black.
**
**  INPUT:
**
**      hwcContext * Context
**          hwcomposer context pointer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
hwcCompose(
    IN hwcContext * Context
    )
{
    gceSTATUS status;
    hwcArea * area;

    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;

#if DUMP_COMPOSE

    LOGD("COMPOSE %d layers", Context->layerCount);
#endif


    /***************************************************************************
    ** Swap Rectangle Optimization.
    */

#if ENABLE_SWAP_RECTANGLE

    if (Context->swapArea != NULL)
    {
        /* Do swap rectangle optimization. */
        gcmONERROR(
            _SwapRectangle(Context));
    }

#else

    /* Just to disable warning. */
    (void) _SwapRectangle;
#endif


    /***************************************************************************
    ** Compose Areas.
    */

    area = Context->compositionArea;

    /* Setup clipping to swap rectangle. */
    gcmONERROR(
        gco2D_SetClipping(Context->engine,
                          &target->swapRect));

    /* Go througn all areas. */
    while (area != NULL)
    {
        /* Check worm hole first. */
        if (area->owners == 0U)
        {
            /* Setup worm hole source. */
            gcmONERROR(
                _WormHole(Context, &area->rect));

            /* Advance to next area. */
            area = area->next;
            continue;
        }

#if ENABLE_SWAP_RECTANGLE
        /* Compare with swap rectangle. */
        if ((area->rect.left   > target->swapRect.right)
        ||  (area->rect.top    > target->swapRect.bottom)
        ||  (area->rect.right  < target->swapRect.left)
        ||  (area->rect.bottom < target->swapRect.top)
        )
        {
            /* Skip areas out of swap rectangle. */
            area = area->next;
            continue;
        }
#endif

        /* Detect multi-source capabitilty and adjust source/dest surface
         * address(es) and coordinates according to hardware limitation.
         *
         * Multi-source blit limmitation is, source rectangle and dest rectangle
         * must be the same (actually source rectnagle can be smaller than dest
         * rectangle for second or more layers. But this feature makes no sense
         * for hwcomposer).
         * Secondly, physical address of the surfaces(source and dest) MUST be 8
         * pixel aligned.
         *
         * We then need to move blit rectangle to a proper size and make some
         * corresponding adjustment in physical address(es).
         *
         * Considering both limitations in surface address and rectangle
         * coordinates, we move left coordinate to (0~7) which is the lowest 3
         * bits of left coordinate of area rectangle aka dest rectangle. This
         * can make dest surface address always aligned.
         *
         * Without rotation in source surface, we need check corresponding
         * address of source address. But if source surface is rotated, which
         * means, left coordinate is some vertical value for surface surface,
         * so, it is always aligned because 'stride' is always 8 pixel aligned.
         *
         * In the same theory, top coordinate can be arbitrary value if source
         * surface is not rotated because 'stride' is algned.  But if source
         * surface rotated, top coordinate is actually some horizontal value for
         * source surface. Let's pick up top/bottom coordinate from source
         * surface and check the dest then.
         */

        /* Multi-source blit locals. */
        gctUINT32 sourceIndices[4];
        gctUINT32 sourceCount = 0U;

        /* Get dest eigen value. */
        gctUINT32 eigenD = area->rect.left
                         & ((16 / framebuffer->bytesPerPixel - 1));

        /* Source eigen values. */
        gctUINT32 eigenL = 0U;

        /* YUV input existed.
         * Multi-source blit can only support one YUV input. */
        gctBOOL hasYuv   = gcvFALSE;

        /* Last source rotation. */
        /* TODO: Support different rotation for sources. */
        gceSURF_ROTATION rotation = gcvSURF_0_DEGREE;

        /* eigenvalues determined?. */
        gctBOOL determined = gcvFALSE;

        /* Has multiple layers for this area?
         * If it is not the case, we do not need multi-source blit. */
        gctBOOL multiLayer = ((area->owners - 1U) & (area->owners)) != 0U;

        /* Go through all layers. */
        for (gctUINT32 i = 0; i < Context->layerCount; i++)
        {
            gctUINT32 owner = (1U << i);

            if (!(area->owners & owner))
            {
                if (owner > area->owners)
                {
                    /* No more layers. */
                    break;
                }

                /* No such layer, go to next. */
                continue;
            }

            /* Get short cut. */
            hwcLayer * layer = &Context->layers[i];

            /* Check multi-source blit when multiple layers are there. */
            if (multiLayer && Context->multiSourceBlt)
            {
                /* Check source surface. */
                if (layer->source == gcvNULL)
                {
                    /* No source layers will not affect this detection.  But if
                     * it is the case, this layer can be done with multi-source
                     * blit.
                     *
                     * No source layers are: OVERLAY, DIM and CLEAR_HOLE.
                     *
                     * For OVERLAY, it will have only one layer in the area
                     * (see hwcSet, OVERLAY correction part), so it will never
                     * come here.
                     *
                     * For DIM, since we optimized DIM as global alpha
                     * premultiply, it will not affect anything. But a corener
                     * case is, when the DIM layer is on the very bottom of
                     * this area. We must treat it as a layer and use a solid
                     * brush for color source.
                     *
                     * For CLEAR_HOLE, it can only be on the very bottom (see
                     * hwcSet, CLEAR_HOLE correction part). We treat it as a
                     * layer and use a solid brush for color source.
                     *
                     * Multi-source blit can only have one solid brush. So it
                     * will have problems if multiple layers are no source
                     * layers. Fortunately, this problem can never happen
                     * because bottom DIM layer and bottom CLEAR_HOLE layer can
                     * never exist together.
                     *
                     * So here we only treat a very bottom no source layer as
                     * a multi-source blit source input. Do thing for other
                     * cases(must be other DIM layers).
                     */
                    if (((owner - 1U) & area->owners) == 0U)
                    {
                        /* Update source array. */
                        sourceIndices[sourceCount++] = i;
                    }

                    /* No source layers is always handled. */
                    continue;
                }

                /* Get eigenvalue of the area on this layer.
                 * See _GetSourceEigen on what eigenvalue means here. */
                gctUINT32 eigen = _GetSourceEigen(layer, &area->rect);

                if (determined)
                {
                    /* Source engin must be the same to do multi-source blit
                     * together with previous layers.  */
                    if ((eigen == eigenL)
                    &&  (layer->stretch  == gcvFALSE)
                    &&  (layer->rotation == rotation)
                    &&  (
                            (hasYuv == gcvFALSE)
                        ||  (layer->yuv == gcvFALSE)
                        )
                    )
                    {
                        /* Update source array. */
                        sourceIndices[sourceCount++] = i;

                        /* Update hasYuv flag. */
                        hasYuv = layer->yuv;

                        /* Check max source limitation. */
                        if ((
                               (!Context->layers[sourceIndices[0]].opaque)
                            && (sourceCount >= Context->maxSource - 1)
                            )
                        ||  (sourceCount >= Context->maxSource)
                        )
                        {
                            /* Trigger multi-source blit. */
                            gcmONERROR(
                                _MultiSourceBlit(Context,
                                                 sourceIndices,
                                                 sourceCount,
                                                 eigenL,
                                                 eigenD,
                                                 area));

                            sourceCount = 0U;
                            hasYuv      = gcvFALSE;
                            determined  = gcvFALSE;
                        }

                        continue;
                    }

                    else
                    {
                        /* This layer can not do multi-source with previous
                         * layers. So we need to start multi-source blit on
                         * accumulated previous layers. */
                        if (sourceCount > 1)
                        {
                            /* Trigger multi-source blit. */
                            gcmONERROR(
                                _MultiSourceBlit(Context,
                                                 sourceIndices,
                                                 sourceCount,
                                                 eigenL,
                                                 eigenD,
                                                 area));
                        }

                        else
                        /* sourceCount == 1 */
                        {
                            /* Use single-source blit instead. */
                            gcmONERROR(_Blit(Context, sourceIndices[0], area));
                        }

                        /* Reset multi-source count. */
                        sourceCount = 0U;
                        hasYuv      = gcvFALSE;
                    }
                }

                /* eigenL is not determined, or this layer can not do multi-
                 * source blit with preivous layers.  but it may be able to
                 * do multi-source blit still with later layers. */
                determined = (layer->stretch == gcvFALSE)
                          && (
                                 (layer->rotation != gcvSURF_0_DEGREE)
                              || (eigen == eigenD)
                             );

                if (determined)
                {
                    /* Yes, this layer can use multi-source blit. */
                    eigenL = eigen;

                    /* Update source count. */
                    sourceIndices[sourceCount++] = i;

                    /* Update hasYuv flag. */
                    hasYuv = layer->yuv;

                    /* Update source rotation. */
                    rotation = layer->rotation;

                    continue;
                }

                else if (sourceCount > 0)
                {
                    /* Layer exists before the first source layer, there must
                     * be a no-source DIM layer. And must be ONLY ONE. Blit
                     * the previous DIM layer before current layer. */
                    gcmONERROR(_Blit(Context, sourceIndices[0], area));

                    /* Reset source count. */
                    sourceCount = 0U;
                }
            }

            if ((layer->source == gcvNULL)
            &&  (((owner - 1U) & area->owners) != 0U)
            )
            {
                /* Skip all non-bottom and no-source layers.
                 * See comments above for reason. */
                continue;
            }

            /* Using single source blit. */
            gcmONERROR(_Blit(Context, i, area));
        }

        /* Start multi-source blit for accumulated layers if any. */
        if (Context->multiSourceBlt && sourceCount > 0)
        {
            if (sourceCount > 1)
            {
                /* Trigger multi-source blit. */
                gcmONERROR(
                    _MultiSourceBlit(Context,
                                     sourceIndices,
                                     sourceCount,
                                     eigenL,
                                     eigenD,
                                     area));
            }

            else
            /* sourceCount == 1 */
            {
                /* Use single-source blit instead. */
                gcmONERROR(_Blit(Context, sourceIndices[0], area));
            }
        }

        /* Advance to next area. */
        area = area->next;
    }

    return gcvSTATUS_OK;

OnError:
    LOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


gceSTATUS
_SwapRectangle(
    IN hwcContext * Context
    )
{
    gceSTATUS status;
    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;
    hwcArea * area               = Context->swapArea;


    /***************************************************************************
    ** Setup Source.
    */

    /* Setup source index. */
    gcmONERROR(
        gco2D_SetCurrentSourceIndex(Context->engine, 0U));

    /* Setup source. */
    gcmONERROR(
        gco2D_SetGenericSource(Context->engine,
                               &target->prev->physical,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               framebuffer->res.right,
                               framebuffer->res.bottom));

    /* Setup mirror. */
    gcmONERROR(
        gco2D_SetBitBlitMirror(Context->engine,
                               gcvFALSE,
                               gcvFALSE));

    /* Disable alhpa blending. */
    gcmONERROR(
        gco2D_DisableAlphaBlend(Context->engine));

    /* Disable premultiply. */
    gcmONERROR(
        gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                           gcv2D_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_COLOR_MULTIPLY_DISABLE));

    /* Setup clipping to full screen. */
    gcmONERROR(
        gco2D_SetClipping(Context->engine,
                          &framebuffer->res));


    /***************************************************************************
    ** Setup Target.
    */

    gcmONERROR(
        gco2D_SetGenericTarget(Context->engine,
                               &target->physical,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               framebuffer->res.right,
                               framebuffer->res.bottom));

    /***************************************************************************
    ** Copy Swap Areas.
    */

    while (area != NULL)
    {
        /* Blit only layers without owners. No owner means we need copy area
         * from front buffer to current. */
        if (area->owners == 0U)
        {
            /* Do batchblit. */
            gcmONERROR(
                gco2D_BatchBlit(Context->engine,
                                1U,
                                &area->rect,
                                &area->rect,
                                0xCC,
                                0xCC,
                                framebuffer->format));

#if DUMP_COMPOSE

        LOGD("  SWAP RECT: [%d,%d,%d,%d] (%08x)=>(%08x)",
             area->rect.left,
             area->rect.top,
             area->rect.right,
             area->rect.bottom,
             target->prev->physical,
             target->physical);
#endif
        }

        /* Advance to next area. */
        area = area->next;
    }

    return gcvSTATUS_OK;

OnError:
    LOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


gceSTATUS
_WormHole(
    IN hwcContext * Context,
    IN gcsRECT * Rect
    )
{
    gceSTATUS status;
    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;

    /* Disable alpha blending. */
    gcmONERROR(
        gco2D_DisableAlphaBlend(Context->engine));

    /* No premultiply. */
    gcmONERROR(
        gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                           gcv2D_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE,
                                           gcv2D_COLOR_MULTIPLY_DISABLE));

    /* Setup Target. */
    gcmONERROR(
        gco2D_SetGenericTarget(Context->engine,
                               &target->physical,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               framebuffer->res.right,
                               framebuffer->res.bottom));

#if DUMP_COMPOSE

        LOGD("  WORMHOLE: [%d,%d,%d,%d] (%08x)",
             Rect->left,
             Rect->top,
             Rect->right,
             Rect->bottom,
             framebuffer->target->physical);
#endif

    /* Perform a Clear. */
    gcmONERROR(
        gco2D_Clear(Context->engine,
                    1U,
                    Rect,
                    0x00000000,
                    0xCC,
                    0xCC,
                    framebuffer->format));


    return gcvSTATUS_OK;

OnError:
    LOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


/* Single-source blit. */
gceSTATUS
_Blit(
    IN hwcContext * Context,
    IN gctUINT32 Index,
    IN hwcArea * Area
    )
{
    gceSTATUS status;
    gcsRECT srcRect;

    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;

    hwcLayer * layer = &Context->layers[Index];

    /* This layer is the very bottom layer? */
    gctBOOL ground = (((1U << Index) - 1U) & Area->owners) == 0U;


    /***************************************************************************
    ** Dim detection.
    */

    /* We are handling DIM differently as last hwcomposer version.
     * DIM alpha 255 is special because it is actually a clear. This 'solid dim'
     * is handled in 'DIM Optimization' section in hwcSet.
     *
     * For normal DIM case, it is doing DIM lie,
     *
     *   CsAs = (0, 0, 0, alpha)
     *
     *   Cd' = Cs + Cd * (1 - As)
     *   Ad' = As + Ad * (1 - As)
     *
     * That is,
     *   Cd' = Cd * (1 - As) = Cd * (1 - alpha)
     *
     * So, can easily 'premultiply' a (1 - alpha) value for each layers before
     * this DIM layer and skip the DIM layer.
     * This optimization will reduce a layer!
     */
    gctBOOL   hasDim   = gcvFALSE;
    gctUINT32 alphaDim = 0xFF;

    if (Context->hasDim)
    {
        /* Go through above layers. */
        for (gctUINT32 i = Index + 1; i < Context->layerCount; i++)
        {
            gctUINT32 owner = (1U << i);

            if ((Context->layers[i].opaque == gcvFALSE)
            &&  (Context->layers[i].compositionType == HWC_DIM)
            &&  (Area->owners & owner)
            )
            {
               /* OK, find a dim layer and it covers the currect layer. */
               alphaDim  *= 0xFF - (Context->layers[i].color32 >> 24);
               alphaDim >>= 8;

               hasDim = gcvTRUE;
            }
        }
    }


    /***************************************************************************
    ** Setup Source.
    */

    /* Setup source index. */
    gcmONERROR(
        gco2D_SetCurrentSourceIndex(Context->engine, 0U));

    /* Setup source. */
    if (layer->source != gcvNULL)
    {
        /* Get shortcuts. */
        gctFLOAT hfactor = layer->hfactor;
        gctFLOAT vfactor = layer->vfactor;

        /* Compute delta(s). */
        gctFLOAT dl = (layer->dstRect.left   - Area->rect.left)   * hfactor;
        gctFLOAT dt = (layer->dstRect.top    - Area->rect.top)    * vfactor;
        gctFLOAT dr = (layer->dstRect.right  - Area->rect.right)  * hfactor;
        gctFLOAT db = (layer->dstRect.bottom - Area->rect.bottom) * vfactor;

        if (layer->hMirror)
        {
            gctFLOAT tl = dl;

            dl = -dr;
            dr = -tl;
        }

        if (layer->vMirror)
        {
            gctFLOAT tt = dt;

            dt = -db;
            db = -tt;
        }

        srcRect.left   = gctINT(layer->srcRect.left   - dl);
        srcRect.top    = gctINT(layer->srcRect.top    - dt);
        srcRect.right  = gctINT(layer->srcRect.right  - dr);
        srcRect.bottom = gctINT(layer->srcRect.bottom - db);

        /* TODO: skip if out of clip rectangle. */
        gcmONERROR(
            gco2D_SetGenericSource(Context->engine,
                                   layer->addresses,
                                   layer->addressNum,
                                   layer->strides,
                                   layer->strideNum,
                                   layer->tiling,
                                   layer->format,
                                   layer->rotation,
                                   layer->width,
                                   layer->height));

        /* Setup mirror. */
        gcmONERROR(
            gco2D_SetBitBlitMirror(Context->engine,
                                   layer->hMirror,
                                   layer->vMirror));


        /* Set source rect. */
        gcmONERROR(
            gco2D_SetSource(Context->engine,
                            &srcRect));

#if DUMP_COMPOSE

        LOGD("  BLIT: layer[%d] (alpha=%d): "
             "[%d,%d,%d,%d] (%08x) => [%d,%d,%d,%d] (%08x)",
             Index,
             alphaDim,
             srcRect.left,
             srcRect.top,
             srcRect.right,
             srcRect.bottom,
             layer->addresses[0],
             Area->rect.left,
             Area->rect.top,
             Area->rect.right,
             Area->rect.bottom,
             framebuffer->target->physical);
#endif
    }

    else
    {
        gcmONERROR(
            gco2D_LoadSolidBrush(Context->engine,
                                 /* This should not be taken. */
                                 gcvSURF_UNKNOWN,
                                 gcvTRUE,
                                 layer->color32,
                                 0));
#if DUMP_COMPOSE

        LOGD("  BLIT: layer[%d]: color32=0x%08x => [%d,%d,%d,%d] (%08x)",
             Index,
             layer->color32,
             Area->rect.left,
             Area->rect.top,
             Area->rect.right,
             Area->rect.bottom,
             framebuffer->target->physical);
#endif
    }

    /* Setup alpha blending. */
    if (layer->opaque || ground)
    {
        /* The layer is at the very bottom in this area, and it needs
         * alpha blending. Where is the alpha blending target then?
         * This is actually another type of 'WormHole' and target area
         * should be cleared as [0,0,0,0] before blitting this layer.
         * But we can easily disable alpha blending to get the same
         * result. */
        gcmONERROR(
            gco2D_DisableAlphaBlend(Context->engine));
    }

    else
    {
        gcmONERROR(
            gco2D_EnableAlphaBlendAdvanced(Context->engine,
                                           layer->srcAlphaMode,
                                           layer->dstAlphaMode,
                                           layer->srcGlobalAlphaMode,
                                           layer->dstGlobalAlphaMode,
                                           layer->srcFactorMode,
                                           layer->dstFactorMode));
    }

    /* Setup premultiply. */
    if (hasDim)
    {
        gctUINT32 srcGlobalAlpha = (layer->srcGlobalAlpha >> 8) * alphaDim;
        gctUINT32 dstGlobalAlpha = (layer->dstGlobalAlpha >> 8) * alphaDim;

        srcGlobalAlpha &= 0xFF000000;
        dstGlobalAlpha &= 0xFF000000;

        /* Dim optimization. */
        gcmONERROR(
            gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                               layer->srcPremultSrcAlpha,
                                               layer->dstPremultDstAlpha,
                                               gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA,
                                               layer->dstDemultDstAlpha));

        gcmONERROR(
            gco2D_SetSourceGlobalColorAdvanced(Context->engine,
                                               srcGlobalAlpha));

        gcmONERROR(
            gco2D_SetTargetGlobalColorAdvanced(Context->engine,
                                               dstGlobalAlpha));
    }

    else
    {
        gcmONERROR(
            gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                               layer->srcPremultSrcAlpha,
                                               layer->dstPremultDstAlpha,
                                               layer->srcPremultGlobalMode,
                                               layer->dstDemultDstAlpha));

        gcmONERROR(
            gco2D_SetSourceGlobalColorAdvanced(Context->engine,
                                               layer->srcGlobalAlpha));

        gcmONERROR(
            gco2D_SetTargetGlobalColorAdvanced(Context->engine,
                                               layer->dstGlobalAlpha));
    }


    /***************************************************************************
    ** Setup Target.
    */

    gcmONERROR(
        gco2D_SetGenericTarget(Context->engine,
                               &target->physical,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               framebuffer->res.right,
                               framebuffer->res.bottom));


    /***************************************************************************
    ** Start Single-Source Blit(Clear).
    */

    if (layer->source != gcvNULL)
    {
        if ((layer->yuv)
        &&  (layer->stretch || Context->opf == gcvFALSE)
        )
        {
            /* Use filterBlit to blit YUV source if YUV blit not supported. */
            /* Set kernel size. */
            gcmONERROR(
                gco2D_SetKernelSize(Context->engine,
                                    layer->hkernel,
                                    layer->vkernel));

            gcmONERROR(
                gco2D_SetFilterType(Context->engine,
                                    gcvFILTER_SYNC));

            /* Trigger filter blit. */
            gcmONERROR(
                gco2D_FilterBlitEx(Context->engine,
                                   layer->addresses[0],
                                   layer->strides[0],
                                   layer->addresses[1],
                                   layer->strides[1],
                                   layer->addresses[2],
                                   layer->strides[2],
                                   layer->format,
                                   layer->rotation,
                                   layer->width,
                                   layer->height,
                                   &srcRect,
                                   target->physical,
                                   framebuffer->stride,
                                   framebuffer->format,
                                   gcvSURF_0_DEGREE,
                                   framebuffer->res.right,
                                   framebuffer->res.bottom,
                                   &Area->rect,
                                   gcvNULL));
        }

        else if (layer->stretch)
        {
            /* Update stretch factors. */
            gcmONERROR(
                gco2D_SetStretchFactors(Context->engine,
                                        (gctINT32) (layer->hfactor * 65536),
                                        (gctINT32) (layer->vfactor * 65536)));
            /* StretchBlit. */
            gcmONERROR(
                gco2D_StretchBlit(Context->engine,
                                  1U,
                                  &Area->rect,
                                  0xCC,
                                  0xCC,
                                  framebuffer->format));
        }

        else
        {
            /* Do bit blit. */
            gcmONERROR(
                gco2D_Blit(Context->engine,
                           1U,
                           &Area->rect,
                           0xCC,
                           0xCC,
                           framebuffer->format));
        }
    }

    else if (layer->opaque)
    {
        /* Do clear. */
        gcmONERROR(
            gco2D_Clear(Context->engine,
                        1U,
                        &Area->rect,
                        layer->color32,
                        0xCC,
                        0xCC,
                        framebuffer->format));
    }

    else
    {
        /* Do bit blit. */
        gcmONERROR(
            gco2D_Blit(Context->engine,
                       1U,
                       &Area->rect,
                       0xF0,
                       0xF0,
                       framebuffer->format));
    }

    return gcvSTATUS_OK;

OnError:
    LOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}


/*
 * Source eigenvalue is to determine left offset to make sure source physical
 * address is aligned.
 *
 * In original coord sys (source before rotation):
 *
 *    [+]--------------------
 *     |
 *     |
 *     |
 *     |     +--------------+
 *     |     | original rect|
 *     |<-L->| (blit rect   |
 *     |     |before trans- |
 *     |     |from)         |
 *     |     +--------------+
 *     |
 *     |
 *     |
 *     +---------------------
 *
 * Horizontal 'L'(eigenL) is (OrigRect.left & 7), so that source
 * surface physical is 8 pixel aligned.
 * Since original rect is transformed to Area->rect, we need to calculate left
 * coordinate on original coord sys for Area->rect.
 *
 * On the other side, dest eigenvalue is to determine left offset to make sure dest
 * physical address is aligned.
 *
 *    [+]--------------------
 *     |
 *     |
 *     |
 *     |     +--------------+
 *     |     | blit rect    |
 *     |<-D->|              |
 *     |     |              |
 *     |     |              |
 *     |     +--------------+
 *     |
 *     |
 *     |
 *     +---------------------
 *
 * Horizontal 'D'(eigenD) is (Area->rect.left & 7), so that
 * dest surface physical is 8 pixel aligned.
 * Dest is not rotated/scaled, we easily generte eigenD by
 * (Area->rect.left & 7) in other place.
 */
gctUINT32
_GetSourceEigen(
    IN  hwcLayer * Layer,
    IN  gcsRECT *  DestRect
    )
{
    gctUINT32 left;

    /* Compute left coordinate value in origin coord sys. */
    switch (Layer->rotation)
    {
    case gcvSURF_0_DEGREE:
    default:
        left   = Layer->orgRect.left
               - (Layer->dstRect.left   - DestRect->left);
        break;

    case gcvSURF_90_DEGREE:
        left   = Layer->orgRect.left
               + (Layer->dstRect.bottom - DestRect->bottom);
        break;

    case gcvSURF_180_DEGREE:
        left   = Layer->orgRect.left
               + (Layer->dstRect.right  - DestRect->right);
        break;

    case gcvSURF_270_DEGREE:
        left   = Layer->orgRect.left
               - (Layer->dstRect.top    - DestRect->top);
        break;
    }

    /* Generate source eigenvalue. */
    return (left & ((Layer->yuv ? 64 : 16) / Layer->bytesPerPixel - 1));
}


/* Multi-source blit. */
gceSTATUS
_MultiSourceBlit(
    IN hwcContext * Context,
    IN gctUINT32 Indices[4],
    IN gctUINT32 Count,
    IN gctUINT32 EigenL,
    IN gctUINT32 EigenD,
    IN hwcArea * Area
    )
{
    gceSTATUS status;

    hwcFramebuffer * framebuffer = Context->framebuffer;
    hwcBuffer * target           = framebuffer->target;

    /* This layer is the very bottom layer? */
    gctBOOL ground = (((1U << Indices[0]) - 1U) & Area->owners) == 0U;

    gctUINT32 sourceMask = 0U;
    gctUINT32 sourceNum  = 0U;

    /* Get shortcuts. */
    gcsRECT * dstRect = &Area->rect;

    /* Blit rectangle, which is the same rectangle on source and dest. */
    gcsRECT blitRect;

    /* Corresponding left/top of blit rectangle in original coord sys
     * (before rotation). */
    gctINT orgBlitX;
    gctINT orgBlitY;

    /* Surface size, which is the same size on source and dest. */
    gctUINT32 width;
    gctUINT32 height;

    /* Target physical address. */
    gctUINT32 dstAddress;

    /* Source rotation. */
    gceSURF_ROTATION rotation = gcvSURF_0_DEGREE;

    /* Get source rotation.
     * TODO: Support different rotation for sources. */
    for (gctUINT32 i = 0; i < Count; i++)
    {
        hwcLayer * layer = &Context->layers[Indices[i]];

        if (layer->source != gcvNULL)
        {
            rotation = layer->rotation;
            break;
        }
    }


    /***************************************************************************
    ** Determine blit rectangle and surface size.
    */

    blitRect.left  = EigenD;

    blitRect.right = EigenD + (dstRect->right - dstRect->left);

    /*  Corresponding 'eigenL' position after source surface rotated.
     *
     *     +--------------------------+
     *     |        ^                 |
     *     |        L (270 deg)       |
     *     |        v                 |
     *     |     +--------------+     |
     *     |     | blit rect    |     |
     *     |<-L->|              |<-L->|
     *     |(0   |              |(180 |
     *     | deg)|              | deg)|
     *     |     +--------------+     |
     *     |        ^                 |
     *     |        L (90 deg)        |
     *     |        v                 |
     *     +--------------------------+
     *
     * So,
     * 0   deg: 'L' becomes 'left'
     * 90  deg: 'L' becomes 'height - bottom'
     * 180 deg: 'L' becomes 'width - right'
     * 270 deg: 'L' becomes 'top'
     */
    switch (rotation)
    {
    case gcvSURF_0_DEGREE:
    default:
        /*
         * Adjusted dest surface and rectangle:
         *     +-----+--------------+
         *     |     | blit rect    |
         *     |<-D->|              |
         *     |<-L->|              |
         *     |     |              |
         *     +-----+--------------+
         */
        /* So this is why eigenL and eigenD must be the same when no rotation.
         * Any top value is OK for no-rotation case. Let's easily
         * set it to 0. */
        blitRect.top    = 0;

        blitRect.bottom = dstRect->bottom - dstRect->top;

        /* Top/left in original coord sys. */
        orgBlitX        = EigenL;
        orgBlitY        = 0;

        /* Surface size. */
        width           = blitRect.right;
        height          = blitRect.bottom;

        break;

    case gcvSURF_90_DEGREE:
        /*
         * Adjusted dest surface and rectangle:
         *     +-----+--------------+
         *     |     | blit rect    |
         *     |<-D->|              |
         *     |     |              |
         *     |     |              |
         *     |     +--------------+
         *     |        ^           |
         *     |        L (90 deg)  |
         *     |        v           |
         *     +--------------------+
         */
        blitRect.top    = 0;
        blitRect.bottom = dstRect->bottom - dstRect->top;

        /* Top/left in original coord sys. */
        orgBlitX        = EigenL;
        orgBlitY        = EigenD;

        /* Surface size. */
        width           = blitRect.right;
        height          = blitRect.bottom + EigenL;

        break;

    case gcvSURF_180_DEGREE:
        /*
         * Adjusted dest surface and rectangle:
         *     +-----+--------------+-----+
         *     |     | blit rect    |     |
         *     |<-D->|              |<-L->|
         *     |     |              |     |
         *     |     |              |     |
         *     +-----+--------------+-----+
         */
        blitRect.top    = 0;
        blitRect.bottom = dstRect->bottom - dstRect->top;

        /* Top/left in original coord sys. */
        orgBlitX        = EigenL;
        orgBlitY        = 0;

        /* Dest surface size. */
        width           = blitRect.right + EigenL;
        height          = blitRect.bottom;

        break;

    case gcvSURF_270_DEGREE:
        /*
         * Adjusted dest surface and rectangle:
         *     +--------------------+
         *     |        ^           |
         *     |        L (270 deg) |
         *     |        v           |
         *     +-----+--------------+
         *     |     | blit rect    |
         *     |<-D->|              |
         *     |     |              |
         *     |     |              |
         *     +-----+--------------+
         */
        blitRect.top    = EigenL;
        blitRect.bottom = EigenL + dstRect->bottom - dstRect->top;

        /* Top/left in original coord sys. */
        orgBlitX        = EigenL;
        orgBlitY        = 0;

        /* Dest surfae size. */
        width           = blitRect.right;
        height          = blitRect.bottom;

        break;
    }


    /***************************************************************************
    ** Setup target.
    */

    /* Compute offset to dest initial point. */
    gctINT dx = dstRect->left - blitRect.left;
    gctINT dy = dstRect->top  - blitRect.top;

    /* Move target physical to dest initial point. */
    dstAddress = target->physical
               + framebuffer->stride * dy
               + framebuffer->bytesPerPixel * dx;

    /* Setup target. */
    gcmONERROR(
        gco2D_SetGenericTarget(Context->engine,
                               &dstAddress,
                               1U,
                               &framebuffer->stride,
                               1U,
                               framebuffer->tiling,
                               framebuffer->format,
                               gcvSURF_0_DEGREE,
                               width,
                               height));

#if DUMP_COMPOSE

    LOGD("  MULTI-SOURCE BLIT: (eigenL=%d, eigenD=%d) blitRect[%d,%d,%d,%d]",
         EigenL,
         EigenD,
         blitRect.left,
         blitRect.top,
         blitRect.right,
         blitRect.bottom);

    LOGD("   dstRect[%d,%d,%d,%d] dx=%d,dy=%d: 0x%08x->0x%08x",
         dstRect->left,
         dstRect->top,
         dstRect->right,
         dstRect->bottom,
         dx, dy,
         framebuffer->target->physical,
         dstAddress);
#endif


    /***************************************************************************
    ** Check Alpha Blending on first source.
    */

    if (!Context->layers[Indices[0]].opaque
    &&  !ground
    )
    {
        /* Setup source index. */
        gcmONERROR(
            gco2D_SetCurrentSourceIndex(Context->engine, 0U));

        /* This layer is the first layer in this multi-source blit batch.
         * Hardware limitation, first layer will NOT do alpha blending with
         * target. So we need to insert target surface as the first source and
         * treat the actual first layer as the second. */
        gcmONERROR(
            gco2D_SetGenericSource(Context->engine,
                                   &dstAddress,
                                   1U,
                                   &framebuffer->stride,
                                   1U,
                                   framebuffer->tiling,
                                   framebuffer->format,
                                   gcvSURF_0_DEGREE,
                                   width,
                                   height));

        /* Setup mirror. */
        gcmONERROR(
            gco2D_SetBitBlitMirror(Context->engine,
                                   gcvFALSE,
                                   gcvFALSE));

        /* Set source rect. */
        gcmONERROR(
            gco2D_SetSource(Context->engine,
                            &blitRect));

        /* ROP. */
        gcmONERROR(
            gco2D_SetROP(Context->engine,
                         0xCC,
                         0xCC));

        /* Can never have alpha blending for this case. */
        gcmONERROR(
            gco2D_DisableAlphaBlend(Context->engine));

        gcmONERROR(
            gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                               gcv2D_COLOR_MULTIPLY_DISABLE,
                                               gcv2D_COLOR_MULTIPLY_DISABLE,
                                               gcv2D_GLOBAL_COLOR_MULTIPLY_DISABLE,
                                               gcv2D_COLOR_MULTIPLY_DISABLE));

        /* Target source inserted at index 0. So we need to update
         * index and source count. */
        sourceNum  = 1U;
        sourceMask = 1U;

#if DUMP_COMPOSE
        LOGD("   layer: Framebuffer target as source 0");
#endif
    }


    /***************************************************************************
    ** Setup color source(s) and Start Blit.
    */

    for (gctUINT32 j = 0; j < Count; j++)
    {
        /* Get shortcuts. */
        hwcLayer * layer = &Context->layers[Indices[j]];

        gctBOOL   hasDim   = gcvFALSE;
        gctUINT32 alphaDim = 0xFF;

        if (j == 1)
        {
            /* The second layer can never be at ground. */
            ground = gcvFALSE;
        }

        /* Dim detection. */
        if (Context->hasDim)
        {
            /* Go through above layers. */
            for (gctUINT32 i = Indices[j] + 1; i < Context->layerCount; i++)
            {
                gctUINT32 owner = (1U << i);

                if ((Context->layers[i].opaque == gcvFALSE)
                &&  (Context->layers[i].compositionType == HWC_DIM)
                &&  (Area->owners & owner)
                )
                {
                   /* OK, find a dim layer and it covers the currect layer. */
                   alphaDim  *= 0xFF - (Context->layers[i].color32 >> 24);
                   alphaDim >>= 8;

                   hasDim = gcvTRUE;
                }
            }
        }

        /* Setup source index. */
        gcmONERROR(
            gco2D_SetCurrentSourceIndex(Context->engine, sourceNum));

        /* Setup source. */
        if (layer->source != gcvNULL)
        {
            /* Source physical address offset. */
            gctUINT32 addresses[3];

            /* Area->rect in original coord sys. */
            gctINT orgLeft;
            gctINT orgTop;

            /* Source size. */
            gctINT srcWidth;
            gctINT srcHeight;

            /* Compute delta(s). */
            gctINT dl;
            gctINT dt;
            gctINT dr;
            gctINT db;

            if (layer->orientation == gcvORIENTATION_TOP_BOTTOM)
            {
                dl = layer->dstRect.left   - dstRect->left;
                dt = layer->dstRect.top    - dstRect->top;
                dr = layer->dstRect.right  - dstRect->right;
                db = layer->dstRect.bottom - dstRect->bottom;
            }

            else
            {
                dl = layer->dstRect.left   - dstRect->left;
                dt = - (layer->dstRect.bottom - dstRect->bottom);
                dr = layer->dstRect.right  - dstRect->right;
                db = - (layer->dstRect.top    - dstRect->top);
            }

            /* Transform Area->rect to original coord sys. */
            switch (layer->rotation)
            {
            case gcvSURF_0_DEGREE:
            default:
                orgLeft = layer->orgRect.left - dl;
                orgTop  = layer->orgRect.top  - dt;

                srcWidth  = width;
                srcHeight = height;
                break;

            case gcvSURF_90_DEGREE:
                orgLeft = layer->orgRect.left + db;
                orgTop  = layer->orgRect.top  - dl;

                srcWidth  = height;
                srcHeight = width;
                break;

            case gcvSURF_180_DEGREE:
                orgLeft = layer->orgRect.left + dr;
                orgTop  = layer->orgRect.top  + db;

                srcWidth  = width;
                srcHeight = height;
                break;

            case gcvSURF_270_DEGREE:
                orgLeft = layer->orgRect.left - dt;
                orgTop  = layer->orgRect.top  + dr;

                srcWidth  = height;
                srcHeight = width;
                break;
            }

            /* Compute offset to source initial point. */
            dx = orgLeft - orgBlitX;
            dy = orgTop  - orgBlitY;

            /* Get original source physical addresses. */
            addresses[0] = layer->addresses[0]
                         + layer->strides[0] * dy + layer->bytesPerPixel * dx;

            addresses[1] = layer->addresses[1]
                         + layer->strides[1] * dy + layer->bytesPerPixel * dx;

            addresses[2] = layer->addresses[2]
                         + layer->strides[2] * dy + layer->bytesPerPixel * dx;

#if DUMP_COMPOSE
            LOGD("   layer[%d]: (alpha=%d) "
                 "orgRect=[%d,%d], orgBlit=[%d,%d] dx=%d,dy=%d: %08x->%08x",
                 Indices[j],
                 alphaDim,
                 orgLeft,
                 orgTop,
                 orgBlitX,
                 orgBlitY,
                 dx, dy,
                 layer->addresses[0],
                 addresses[0]);
#endif

            /* TODO: skip if out of clip rectangle. */
            /* Setup source. */
            gcmONERROR(
                gco2D_SetGenericSource(Context->engine,
                                       addresses,
                                       layer->addressNum,
                                       layer->strides,
                                       layer->strideNum,
                                       layer->tiling,
                                       layer->format,
                                       layer->rotation,
                                       srcWidth,
                                       srcHeight));

            /* Setup mirror. */
            gcmONERROR(
                gco2D_SetBitBlitMirror(Context->engine,
                                       layer->hMirror,
                                       layer->vMirror));


            /* Set source rect (equal to dstRect). */
            gcmONERROR(
                gco2D_SetSource(Context->engine,
                                &blitRect));

            /* Set ROP. */
            gcmONERROR(
                gco2D_SetROP(Context->engine,
                             0xCC,
                             0xCC));
        }

        else
        {
            /* Color source is still needed if uses patthen only. We set dummy
             * color source to framebuffer target. */
            gcmONERROR(
                gco2D_SetGenericSource(Context->engine,
                                       &dstAddress,
                                       1U,
                                       &framebuffer->stride,
                                       1U,
                                       framebuffer->tiling,
                                       framebuffer->format,
                                       gcvSURF_0_DEGREE,
                                       width,
                                       height));

            /* Setup mirror. */
            gcmONERROR(
                gco2D_SetBitBlitMirror(Context->engine,
                                       gcvFALSE,
                                       gcvFALSE));

            /* Set source rect (equal to dstRect). */
            gcmONERROR(
                gco2D_SetSource(Context->engine,
                                &blitRect));

            gcmONERROR(
                gco2D_LoadSolidBrush(Context->engine,
                                     /* This should not be taken. */
                                     gcvSURF_UNKNOWN,
                                     gcvTRUE,
                                     layer->color32,
                                     0U));

            /* Set ROP: use pattern only. */
            gcmONERROR(
                gco2D_SetROP(Context->engine,
                             0xF0,
                             0xF0));
#if DUMP_COMPOSE

            LOGD("   layer[%d]: color32=0x%08x",
                 Indices[j], layer->color32);
#endif
        }

        /* Setup alpha blending. */
        if (layer->opaque || ground)
        {
            /* The layer is at the very bottom in this area, and it needs
             * alpha blending. Where is the alpha blending target then?
             * This is actually another type of 'WormHole' and target area
             * should be cleared as [0,0,0,0] before blitting this layer.
             * But we can easily disable alpha blending to get the same
             * result. */
            gcmONERROR(
                gco2D_DisableAlphaBlend(Context->engine));
        }

        else
        {
            gcmONERROR(
                gco2D_EnableAlphaBlendAdvanced(Context->engine,
                                               layer->srcAlphaMode,
                                               layer->dstAlphaMode,
                                               layer->srcGlobalAlphaMode,
                                               layer->dstGlobalAlphaMode,
                                               layer->srcFactorMode,
                                               layer->dstFactorMode));
        }

        /* Setup premultiply. */
        if (hasDim)
        {
            gctUINT32 srcGlobalAlpha = (layer->srcGlobalAlpha >> 8) * alphaDim;
            gctUINT32 dstGlobalAlpha = (layer->dstGlobalAlpha >> 8) * alphaDim;

            srcGlobalAlpha &= 0xFF000000;
            dstGlobalAlpha &= 0xFF000000;

            /* Dim optimization. */
            gcmONERROR(
                gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                                   layer->srcPremultSrcAlpha,
                                                   layer->dstPremultDstAlpha,
                                                   gcv2D_GLOBAL_COLOR_MULTIPLY_ALPHA,
                                                   layer->dstDemultDstAlpha));

            gcmONERROR(
                gco2D_SetSourceGlobalColorAdvanced(Context->engine,
                                                   srcGlobalAlpha));

            gcmONERROR(
                gco2D_SetTargetGlobalColorAdvanced(Context->engine,
                                                   dstGlobalAlpha));
        }

        else
        {
            gcmONERROR(
                gco2D_SetPixelMultiplyModeAdvanced(Context->engine,
                                                   layer->srcPremultSrcAlpha,
                                                   layer->dstPremultDstAlpha,
                                                   layer->srcPremultGlobalMode,
                                                   layer->dstDemultDstAlpha));

            gcmONERROR(
                gco2D_SetSourceGlobalColorAdvanced(Context->engine,
                                                   layer->srcGlobalAlpha));

            gcmONERROR(
                gco2D_SetTargetGlobalColorAdvanced(Context->engine,
                                                   layer->dstGlobalAlpha));
        }

        /* Append mask to sourceMask. */
        sourceMask = ((sourceMask << 1U) | 1U);
        sourceNum++;
    }


    /***************************************************************************
    ** Trigger Multi-Source Blit.
    */

    gcmONERROR(
        gco2D_MultiSourceBlit(Context->engine,
                              sourceMask,
                              &blitRect,
                              1U));

    return gcvSTATUS_OK;

OnError:
    LOGE("Failed in %s: status=%d", __FUNCTION__, status);
    return status;
}

