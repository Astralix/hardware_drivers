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
#include "gc_gralloc_priv.h"
#include "gc_hal_user.h"

static gceSTATUS
_UploadStates(
    IN gcsCOPYBIT_CONTEXT * Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        if (Context->dirty.i == 0)
        {
            /* Done. */
            break;
        }

        /* rotationDeg. */
        if (Context->dirty.s.rotationDegKey)
        {
            /* TODO: rotationDeg. */
            Context->dirty.s.rotationDegKey = 0;
        }

        /* planeAlpha. */
        if (Context->dirty.s.alphaKey)
        {
            /* Setup alpha blending. */
            if ((Context->perpixelAlpha)
            ||  (Context->planeAlpha != 0xFF)
            )
            {
                gcmERR_BREAK(
                    gco2D_EnableAlphaBlend(Context->engine,
                                           Context->planeAlpha,
                                           Context->planeAlpha,
                                           gcvSURF_PIXEL_ALPHA_STRAIGHT,
                                           gcvSURF_PIXEL_ALPHA_STRAIGHT,
                                           gcvSURF_GLOBAL_ALPHA_SCALE,
                                           gcvSURF_GLOBAL_ALPHA_ON,
                                           gcvSURF_BLEND_STRAIGHT,
                                           gcvSURF_BLEND_INVERSED,
                                           gcvSURF_COLOR_STRAIGHT,
                                           gcvSURF_COLOR_STRAIGHT));
            }
            else
            {
                gcmERR_BREAK(
                    gco2D_DisableAlphaBlend(Context->engine));
            }

            Context->dirty.s.alphaKey = 0;
        }

        /* Dither. */
        if (Context->dirty.s.ditherKey)
        {
            /* TODO: Dither. */

            Context->dirty.s.ditherKey = 0;
        }

        /* Transform. */
        if (Context->dirty.s.transformKey)
        {
            Context->dstRotation   = gcvSURF_0_DEGREE;
            Context->srcRotation   = gcvSURF_0_DEGREE;
            gctBOOL horizonMirror  = gcvFALSE;
            gctBOOL verticalMirror = gcvFALSE;

            /* Setup transform */
            switch(Context->transform)
            {
            case COPYBIT_TRANSFORM_FLIP_H:
                horizonMirror = gcvTRUE;
                break;

            case COPYBIT_TRANSFORM_FLIP_V:
                verticalMirror = gcvTRUE;
                break;

            case COPYBIT_TRANSFORM_ROT_90:
                Context->dstRotation = gcvSURF_90_DEGREE;
                break;

            case COPYBIT_TRANSFORM_ROT_180:
                horizonMirror  = gcvTRUE;
                verticalMirror = gcvTRUE;
                break;

            case COPYBIT_TRANSFORM_ROT_270:
                Context->srcRotation = gcvSURF_90_DEGREE;
                break;

            default:
                break;
            }

            gcmERR_BREAK(
                gco2D_SetBitBlitMirror(Context->engine,
                                       horizonMirror,
                                       verticalMirror));

            Context->dirty.s.transformKey = 0;
        }

        /* Blur. */
        if (Context->dirty.s.blurKey)
        {
            /* TODO: Blur. */

            Context->dirty.s.blurKey = 0;
        }
    }
    while (gcvFALSE);

    return status;
}

static gceSTATUS
_MonoBlit(
    IN gcsCOPYBIT_CONTEXT * Context,
    IN gctUINT32 SourcePhysical,
    IN gctUINT32 SourceStride,
    IN gceSURF_FORMAT SourceFormat,
    IN gctUINT32 DestPhysical,
    IN gctUINT32 DestStride,
    IN gceSURF_FORMAT DestFormat,
    IN gcsRECT * Rect
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        if ((Context->perpixelAlpha)
        ||  (Context->planeAlpha != 0xFF)
        )
        {
            gcmERR_BREAK(
                gco2D_DisableAlphaBlend(Context->engine));

            /* PlaneAlpha dirty. */
            Context->dirty.s.alphaKey = 1;
        }

        if ((Context->transform == COPYBIT_TRANSFORM_ROT_180)
        ||  (Context->transform == COPYBIT_TRANSFORM_FLIP_H)
        ||  (Context->transform == COPYBIT_TRANSFORM_FLIP_V)
        )
        {
            gcmERR_BREAK(
                gco2D_SetBitBlitMirror(Context->engine,
                                                gcvFALSE,
                                                gcvFALSE));

            /* Transform dirty. */
            Context->dirty.s.transformKey = 1;
        }

        gcmERR_BREAK(
            gco2D_SetColorSource(Context->engine,
                                 SourcePhysical,
                                 SourceStride,
                                 SourceFormat,
                                 gcvSURF_0_DEGREE,
                                 0,
                                 gcvFALSE,
                                 gcvSURF_OPAQUE,
                                 0));

        gcmERR_BREAK(
            gco2D_SetSource(Context->engine,
                            Rect));

        gcmERR_BREAK(
            gco2D_SetTarget(Context->engine,
                            DestPhysical,
                            DestStride,
                            gcvSURF_0_DEGREE,
                            0));

        gcmERR_BREAK(
            gco2D_SetClipping(Context->engine,
                              Rect));

        gcmERR_BREAK(
            gco2D_Blit(Context->engine,
                       1,
                       Rect,
                       0xCC,
                       0xCC,
                       DestFormat));
    }
    while (gcvFALSE);

    return status;
}

gceSTATUS
_StretchBlitPE1x(
    IN gcsCOPYBIT_CONTEXT * Context,
    IN struct copybit_image_t const  * Dest,
    IN struct copybit_image_t const  * Source,
    IN struct copybit_rect_t const   * DestRect,
    IN struct copybit_rect_t const   * SourceRect,
    IN struct copybit_region_t const * Clip)
{
    gceSTATUS status = gcvSTATUS_OK;

    gceSURF_FORMAT siFormat;
    gceSURF_FORMAT diFormat;

    gctUINT32      diPhysical = ~0;
    gctUINT32      diAlignedWidth;
    gctUINT32      diAlignedHeight;
    gctINT         diStride;

    gcsRECT srcRect;
    gcsRECT dstRect;
    copybit_rect_t rect;

    gctUINT32      srcPhysical = ~0;
    gctINT         srcStride;
    gctUINT32      srcAlignedWidth;
    gctUINT32      srcAlignedHeight;
    gceSURF_FORMAT srcFormat;

    gctUINT32      dstPhysical = ~0;
    gctINT         dstStride;
    gctUINT32      dstAlignedWidth;
    gctUINT32      dstAlignedHeight;
    gceSURF_FORMAT dstFormat;

    gctBOOL stretch   = gcvFALSE;
    gctBOOL yuvFormat = gcvFALSE;
    gctBOOL perpixelAlpha;

    gc_private_handle_t* dsthnd = (gc_private_handle_t *) Dest->handle;
    gc_private_handle_t* srchnd = (gc_private_handle_t *) Source->handle;

    LOGV("Blit from Source hnd=%p, to Dest hnd=%p", srchnd, dsthnd);

    if (gc_private_handle_t::validate(dsthnd) < 0)
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "Invalid hnd in funciton %s",
                 __func__);

        return gcvSTATUS_INVALID_ARGUMENT;
    }

    siFormat = (gceSURF_FORMAT) srchnd->format;
    diFormat = (gceSURF_FORMAT) dsthnd->format;

    if ((siFormat == gcvSURF_UNKNOWN)
    ||  (diFormat == gcvSURF_UNKNOWN)
    )
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "Image format not support in copybit!");

        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Convert to supported Source format. */
    siFormat = (siFormat == gcvSURF_A8B8G8R8) ? gcvSURF_A8R8G8B8 : siFormat;
    siFormat = (siFormat == gcvSURF_X8B8G8R8) ? gcvSURF_X8R8G8B8 : siFormat;

    /* Convert to supported Dest format. */
    diFormat = (diFormat == gcvSURF_A8B8G8R8) ? gcvSURF_A8R8G8B8 : diFormat;
    diFormat = (diFormat == gcvSURF_X8B8G8R8) ? gcvSURF_X8R8G8B8 : diFormat;

    do
    {
        srcPhysical = srchnd->phys;
        gcoSURF_GetAlignedSize((gcoSURF) srchnd->surface,
                               &srcAlignedWidth,
                               &srcAlignedHeight,
                               &srcStride);

        diPhysical = dsthnd->phys;
        gcoSURF_GetAlignedSize((gcoSURF) dsthnd->surface,
                               &diAlignedWidth,
                               &diAlignedHeight,
                               &diStride);

        if  ((((gcoSURF)srchnd->surface)->info.type == gcvSURF_BITMAP) &&
            !(srchnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER))
        {
            /* Clean the CPU cache. Source would've been rendered by the CPU. */
            gcmERR_BREAK(
                gcoSURF_CPUCacheOperation(
                            (gcoSURF) srchnd->surface,
                            gcvCACHE_CLEAN
                            )
            );
        }

        perpixelAlpha = _HasAlpha(siFormat) &&
                 (dsthnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER) &&
                 !(srchnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER);

        if (Context->perpixelAlpha != perpixelAlpha)
        {
            Context->perpixelAlpha = perpixelAlpha;

            if (Context->planeAlpha == 0xff)
            {
                Context->dirty.s.alphaKey = 1;
            }
        }

        /* Need temp surface if source has alpha channel, but dest not. */
        Context->needAlphaDest = Context->perpixelAlpha &&
                !_HasAlpha(diFormat);

        if (Context->needAlphaDest)
        {
            gcsRECT tempRect;

            tempRect.left   = gcmMAX(0, DestRect->l);
            tempRect.top    = gcmMAX(0, DestRect->t);
            tempRect.right  = gcmMIN((int32_t) Dest->w, DestRect->r);
            tempRect.bottom = gcmMIN((int32_t) Dest->h, DestRect->b);

            gcmERR_BREAK(
                _FitSurface(Context,
                            &Context->alphaDest,
                            Dest->w,
                            Dest->h));

            if (Context->alphaDest.surface == gcvNULL)
            {
                gcmTRACE(gcvLEVEL_ERROR,
                         "fail to construct tmp surface for per_pixel_alpha");

                break;
            }

            /* Copy dest surface to temp surface. */
            gcmERR_BREAK(
                _MonoBlit(Context,
                          diPhysical,
                          diStride,
                          diFormat,
                          Context->alphaDest.physical,
                          Context->alphaDest.stride,
                          Context->alphaDest.format,
                          &tempRect));
        }

        gcmERR_BREAK(
            _UploadStates(Context));

        if (Context->needAlphaDest)
        {
            dstPhysical      = Context->alphaDest.physical;
            dstStride        = Context->alphaDest.stride;
            dstAlignedWidth  = Context->alphaDest.alignedWidth;
            dstAlignedHeight = Context->alphaDest.alignedHeight;
            dstFormat        = Context->alphaDest.format;
        }
        else
        {
            dstPhysical      = diPhysical;
            dstStride        = diStride;
            dstAlignedWidth  = diAlignedWidth;
            dstAlignedHeight = diAlignedHeight;
            dstFormat        = diFormat;
        }

        srcFormat = siFormat;

        if (Context->transform == COPYBIT_TRANSFORM_ROT_270)
        {
            srcRect.left   = SourceRect->t;
            srcRect.top    = Source->w - SourceRect->r;
            srcRect.right  = SourceRect->b;
            srcRect.bottom = Source->w - SourceRect->l;
        }
        else
        {
            srcRect.left   = SourceRect->l;
            srcRect.top    = SourceRect->t;
            srcRect.right  = SourceRect->r;
            srcRect.bottom = SourceRect->b;
        }

        if (Context->transform ==  COPYBIT_TRANSFORM_ROT_90)
        {
            dstRect.left   = DestRect->t;
            dstRect.top    = Dest->w - DestRect->r;
            dstRect.right  = DestRect->b;
            dstRect.bottom = Dest->w - DestRect->l;
        }
        else
        {
            dstRect.left   = DestRect->l;
            dstRect.top    = DestRect->t;
            dstRect.right  = DestRect->r;
            dstRect.bottom = DestRect->b;
        }

        /* Check yuv format. */
        yuvFormat = (srcFormat == gcvSURF_YUY2 || srcFormat == gcvSURF_UYVY);

        stretch =
            (srcRect.right - srcRect.left) != (dstRect.right - dstRect.left) ||
            (srcRect.bottom - srcRect.top) != (dstRect.bottom - dstRect.top);

        /* Upload stretch factor. */
        if (stretch)
        {
            int hFactor;
            int vFactor;

            if ((dstRect.right-dstRect.left) > 1 && (dstRect.bottom-dstRect.top) > 1)
            {
                hFactor = ((srcRect.right - srcRect.left - 1) << 16) /
                        (dstRect.right - dstRect.left - 1);

                vFactor = ((srcRect.bottom - srcRect.top - 1) << 16) /
                        (dstRect.bottom - dstRect.top - 1);
            }
            else
            {
                hFactor = 0;
                vFactor = 0;
            }

            gcmERR_BREAK(
                gco2D_SetStretchFactors(Context->engine,
                                        hFactor,
                                        vFactor));
        }

        /* Prepare source and target for normal blit. */
        gcmERR_BREAK(
            gco2D_SetColorSource(Context->engine,
                                 srcPhysical,
                                 srcStride,
                                 srcFormat,
                                 Context->srcRotation,
                                 srcAlignedWidth,
                                 gcvFALSE,
                                 gcvSURF_OPAQUE,
                                 0));

        gcmERR_BREAK(
            gco2D_SetSource(Context->engine,
                            &srcRect));

        gcmERR_BREAK(
            gco2D_SetTarget(Context->engine,
                            dstPhysical,
                            dstStride,
                            Context->dstRotation,
                            dstAlignedWidth));

        gcsRECT srcRectBackup = srcRect;
        gcsRECT dstRectBackup = dstRect;

        /* Go though all clip rectangles. */
        while (Clip->next(Clip, &rect))
        {
            gcsRECT clipRect;

            srcRect = srcRectBackup;
            dstRect = dstRectBackup;

            if (Context->transform ==  COPYBIT_TRANSFORM_ROT_90)
            {
                clipRect.left   = rect.t;
                clipRect.top    = Dest->w - rect.r;
                clipRect.right  = rect.b;
                clipRect.bottom = Dest->w - rect.l;
            }
            else if (Context->transform == COPYBIT_TRANSFORM_ROT_180)
            {
                float hfactor = (float) (SourceRect->r - SourceRect->l)
                                 / (DestRect->r - DestRect->l);

                float vfactor = (float) (SourceRect->b - SourceRect->t)
                                 / (DestRect->b - DestRect->t);

                /* Intersect. */
                clipRect.left   = gcmMAX(dstRect.left, rect.l);
                clipRect.top    = gcmMAX(dstRect.top, rect.t);
                clipRect.right  = gcmMIN(dstRect.right, rect.r);
                clipRect.bottom = gcmMIN(dstRect.bottom, rect.b);

                /* Adjust src rectangle. */
                srcRect.left   += (int) ((dstRect.right - clipRect.right) * hfactor);
                srcRect.top    += (int) ((dstRect.bottom - clipRect.bottom) * vfactor);
                srcRect.right  -= (int) ((clipRect.left - dstRect.left) * hfactor);
                srcRect.bottom -= (int) ((clipRect.top - dstRect.top) * vfactor);

                /* Set dstRect to clip rectangle. */
                dstRect = clipRect;

                if ((srcRect.left   != srcRectBackup.left)
                ||  (srcRect.right  != srcRectBackup.right)
                ||  (srcRect.top    != srcRectBackup.top)
                ||  (srcRect.bottom != srcRectBackup.bottom)
                )
                {
                    gcmERR_BREAK(
                        gco2D_SetSource(Context->engine,
                                        &srcRect));
                }
            }
            else
            {
                clipRect.left   = rect.l;
                clipRect.top    = rect.t;
                clipRect.right  = rect.r;
                clipRect.bottom = rect.b;
            }

            /* Clamp clip rectangle. */
            if (clipRect.right > dstRect.right)
                clipRect.right = dstRect.right;

            if (clipRect.bottom > dstRect.bottom)
                clipRect.bottom = dstRect.bottom;

            if (clipRect.left < dstRect.left)
                clipRect.left = dstRect.left;

            if (clipRect.top < dstRect.top)
                clipRect.top = dstRect.top;

            gcmERR_BREAK(
                gco2D_SetClipping(Context->engine,
                                  &clipRect));

            if (yuvFormat)
            {
                /* TODO: FilterBlit does not support rotation before PE20. */
                /* Video filter blit */
                /* 1. SetClipping() has no effect for FilterBlit()
                 *    so we use dstSubRect to realize clipping effect
                 * 2. Only FilterBlit support yuv format covertion.
                 */
                gcsRECT dstSubRect;
                int dstWidth   = dstRect.right   - dstRect.left;
                int dstHeight  = dstRect.bottom  - dstRect.top;
                int clipWidth  = clipRect.right  - clipRect.left;
                int clipHeight = clipRect.bottom - clipRect.top;

                dstSubRect.left   = clipRect.left - dstRect.left;
                dstSubRect.top    = clipRect.top - dstRect.top;
                dstSubRect.right  = dstWidth < clipWidth ?
                        dstSubRect.left + dstWidth :
                        dstSubRect.left + clipWidth;
                dstSubRect.bottom = dstHeight < clipHeight ?
                        dstSubRect.top  + dstHeight :
                        dstSubRect.top  + clipHeight;

                gcmERR_BREAK(
                    gco2D_SetKernelSize(Context->engine,
                                        gcdFILTER_BLOCK_SIZE,
                                        gcdFILTER_BLOCK_SIZE));

                gcmERR_BREAK(
                    gco2D_SetFilterType(Context->engine,
                                        gcvFILTER_SYNC));

                gcmERR_BREAK(
                    gco2D_FilterBlit(Context->engine,
                                     srcPhysical,
                                     srcStride,
                                     0, 0, 0, 0,
                                     srcFormat,
                                     Context->srcRotation,
                                     srcAlignedWidth,
                                     &srcRect,
                                     dstPhysical,
                                     dstStride,
                                     dstFormat,
                                     Context->dstRotation,
                                     dstAlignedWidth,
                                     &dstRect,
                                     &dstSubRect));
            }
            else if (Context->blur)
            {
                /* TODO: FilterBlit does not support rotation before PE20. */
                gcsRECT dstSubRect;
                dstSubRect.left  = 0;
                dstSubRect.top   = 0;
                dstSubRect.right = dstRect.right - dstRect.left;
                dstSubRect.bottom = dstRect.bottom - dstRect.top;

                /* Blur blit. */
                gcmERR_BREAK(
                    gco2D_SetKernelSize(Context->engine,
                                        gcdFILTER_BLOCK_SIZE,
                                        gcdFILTER_BLOCK_SIZE));

                gcmERR_BREAK(
                    gco2D_SetFilterType(Context->engine,
                                        gcvFILTER_BLUR));

                gcmERR_BREAK(
                    gco2D_FilterBlit(Context->engine,
                                     srcPhysical,
                                     srcStride,
                                     0, 0, 0, 0,
                                     srcFormat,
                                     gcvSURF_0_DEGREE,
                                     srcAlignedWidth,
                                     &srcRect,
                                     dstPhysical,
                                     dstStride,
                                     dstFormat,
                                     gcvSURF_0_DEGREE,
                                     dstAlignedWidth,
                                     &dstRect,
                                     &dstSubRect));

                gcmERR_BREAK(
                    gco2D_FilterBlit(Context->engine,
                                     dstPhysical,
                                     dstStride,
                                     0, 0, 0, 0,
                                     dstFormat,
                                     gcvSURF_0_DEGREE,
                                     dstAlignedWidth,
                                     &dstRect,
                                     dstPhysical,
                                     dstStride,
                                     dstFormat,
                                     gcvSURF_0_DEGREE,
                                     dstAlignedWidth,
                                     &dstRect,
                                     &dstSubRect));

                /* TODO: surfaceflinger set blur issue. */
                Context->blur = COPYBIT_DISABLE;
            }
            else if (stretch == gcvFALSE)
            {
                /* BitBlit. */
                gcmERR_BREAK(
                    gco2D_Blit(Context->engine,
                               1,
                               &dstRect,
                               0xCC,
                               0xCC,
                               dstFormat));
            }
            else
            {
                /* Normal stretch blit. */
                gcmERR_BREAK(
                    gco2D_StretchBlit(Context->engine,
                                      1,
                                      &dstRect,
                                      0xCC,
                                      0xCC,
                                      dstFormat));
            }
        }

        if (gcmIS_ERROR(status))
        {
            break;
        }

        if (Context->needAlphaDest)
        {
            gcsRECT tempRect;

            tempRect.left   = gcmMAX(0, DestRect->l);
            tempRect.top    = gcmMAX(0, DestRect->t);
            tempRect.right  = gcmMIN((int32_t) Dest->w, DestRect->r);
            tempRect.bottom = gcmMIN((int32_t) Dest->h, DestRect->b);

            /* Blit back to actual dest. */
            gcmERR_BREAK(
                _MonoBlit(Context,
                          Context->alphaDest.physical,
                          Context->alphaDest.stride,
                          Context->alphaDest.format,
                          diPhysical,
                          diStride,
                          diFormat,
                          &tempRect));
        }

        /* Flush and commit. */
        gcmERR_BREAK(
            gco2D_Flush(Context->engine));

        gcmERR_BREAK(
            gcoHAL_Commit(gcvNULL, gcvFALSE));
    }
    while (gcvFALSE);

    return status;
}

