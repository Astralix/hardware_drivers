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
                Context->srcRotation = gcvSURF_270_DEGREE;
                break;

            case COPYBIT_TRANSFORM_ROT_180:
                Context->srcRotation = gcvSURF_180_DEGREE;
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

gceSTATUS
_StretchBlit(
    IN gcsCOPYBIT_CONTEXT * Context,
    IN struct copybit_image_t const  * Dest,
    IN struct copybit_image_t const  * Source,
    IN struct copybit_rect_t const   * DestRect,
    IN struct copybit_rect_t const   * SourceRect,
    IN struct copybit_region_t const * Clip)
{
    gceSTATUS status = gcvSTATUS_OK;

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

    srcFormat = (gceSURF_FORMAT)srchnd->format;
    dstFormat = (gceSURF_FORMAT)dsthnd->format;

    do
    {
        srcPhysical = srchnd->phys;
        gcoSURF_GetAlignedSize((gcoSURF) srchnd->surface,
                               &srcAlignedWidth,
                               &srcAlignedHeight,
                               &srcStride);

        dstPhysical = dsthnd->phys;
        gcoSURF_GetAlignedSize((gcoSURF) dsthnd->surface,
                               &dstAlignedWidth,
                               &dstAlignedHeight,
                               &dstStride);

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

        perpixelAlpha = _HasAlpha(srcFormat) &&
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

        gcmERR_BREAK(
            _UploadStates(Context));

        switch (Context->transform)
        {
        case COPYBIT_TRANSFORM_ROT_90:
            srcRect.left   = Source->h - SourceRect->b;
            srcRect.top    = SourceRect->l;
            srcRect.right  = Source->h - SourceRect->t;
            srcRect.bottom = SourceRect->r;
            break;

        case COPYBIT_TRANSFORM_ROT_270:
            srcRect.left   = SourceRect->t;
            srcRect.top    = Source->w - SourceRect->r;
            srcRect.right  = SourceRect->b;
            srcRect.bottom = Source->w - SourceRect->l;
            break;

        case COPYBIT_TRANSFORM_ROT_180:
            srcRect.left   = Source->w - SourceRect->r;
            srcRect.top    = Source->h - SourceRect->b;
            srcRect.right  = Source->w - SourceRect->l;
            srcRect.bottom = Source->h - SourceRect->t;
            break;

        case COPYBIT_TRANSFORM_FLIP_H:
        case COPYBIT_TRANSFORM_FLIP_V:
            /* TODO: Clip rectangles for mirror. */
        case 0:
            srcRect.left   = SourceRect->l;
            srcRect.top    = SourceRect->t;
            srcRect.right  = SourceRect->r;
            srcRect.bottom = SourceRect->b;
            break;

        default:
            break;
        }

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
            gco2D_SetColorSourceEx(Context->engine,
                                   srcPhysical,
                                   srcStride,
                                   srcFormat,
                                   Context->srcRotation,
                                   srcAlignedWidth,
                                   srcAlignedHeight,
                                   gcvFALSE,
                                   gcvSURF_OPAQUE,
                                   0));

        gcmERR_BREAK(
            gco2D_SetSource(Context->engine,
                            &srcRect));

        gcmERR_BREAK(
            gco2D_SetTargetEx(Context->engine,
                              dstPhysical,
                              dstStride,
                              Context->dstRotation,
                              dstAlignedWidth,
                              dstAlignedHeight));

        /* Go though all clip rectangles. */
        while (Clip->next(Clip, &rect))
        {
            gcsRECT clipRect =
            {
                left:   rect.l,
                top:    rect.t,
                right:  rect.r,
                bottom: rect.b
            };

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
#if !gcdNEED_FILTER_BLIT
                /* Normal stretch blit. */
                gcmERR_BREAK(
                    gco2D_StretchBlit(Context->engine,
                                      1,
                                      &dstRect,
                                      0xCC,
                                      0xCC,
                                      dstFormat));
#else
                gcsRECT dstSubRect;

                /* Filter stretch blit, which do NOT support alpha blend. */
                dstSubRect.left   = clipRect.left - dstRect.left;
                dstSubRect.top    = clipRect.top - dstRect.top;
                dstSubRect.right  = clipRect.right - dstRect.left;
                dstSubRect.bottom =clipRect.bottom - dstRect.top;

                if ((!Context->perpixelAlpha)
                &&  (Context->planeAlpha == 0xFF)
                )
                {
                    /* Does not need alpha blend. */
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
                else
                {
                    /* Need alpha blend. */
                    gcsRECT origclipRect,tmpDestRect;

                    origclipRect.left   = rect.l < DestRect->l ? DestRect->l : rect.l;
                    origclipRect.top    = rect.t < DestRect->t ? DestRect->t : rect.t;
                    origclipRect.right  = rect.r > DestRect->r ? DestRect->r : rect.r;
                    origclipRect.bottom = rect.b > DestRect->b ? DestRect->b : rect.b;

                    tmpDestRect.left   = tmpDestRect.top = 0;
                    tmpDestRect.right  = Dest->w;
                    tmpDestRect.bottom = Dest->h;

                    /* Create a temp surface. */
                    gcmERR_BREAK(
                        _FitSurface(Context,
                                    &Context->tempSurface,
                                    Dest->w,
                                    Dest->h));

                    gcmERR_BREAK(
                        gco2D_SetKernelSize(Context->engine,
                                            gcdFILTER_BLOCK_SIZE,
                                            gcdFILTER_BLOCK_SIZE));

                    gcmERR_BREAK(
                        gco2D_SetFilterType(Context->engine,
                                            gcvFILTER_SYNC));

                    /* FilterBlit to temp surface (rotated). */
                    gcmERR_BREAK(
                        gco2D_FilterBlit(Context->engine,
                                         srcPhysical,
                                         srcStride,
                                         0, 0, 0, 0,
                                         srcFormat,
                                         Context->srcRotation,
                                         srcAlignedWidth,
                                         &srcRect,
                                         Context->tempSurface.physical,
                                         Context->tempSurface.stride,
                                         Context->tempSurface.format,
                                         Context->dstRotation,
                                         Context->tempSurface.alignedWidth,
                                         &dstRect,
                                         &dstSubRect));

                    /* Blit temp surface to dest image. */
                    gcmERR_BREAK(
                        gco2D_SetColorSourceEx(Context->engine,
                                               Context->tempSurface.physical,
                                               Context->tempSurface.stride,
                                               Context->tempSurface.format,
                                               gcvSURF_0_DEGREE,
                                               Context->tempSurface.alignedWidth,
                                               Context->tempSurface.alignedHeight,
                                               gcvFALSE,
                                               gcvSURF_OPAQUE,
                                               0));

                    gcmERR_BREAK(
                        gco2D_SetSource(Context->engine,
                                        &tmpDestRect));

                    gcmERR_BREAK(
                        gco2D_SetTargetEx(Context->engine,
                                          dstPhysical,
                                          dstStride,
                                          gcvSURF_0_DEGREE,
                                          dstAlignedWidth,
                                          dstAlignedHeight));

                    gcmERR_BREAK(
                        gco2D_SetClipping(Context->engine,
                                          &origclipRect));

                    gcmERR_BREAK(
                        gco2D_Blit(Context->engine,
                                   1,
                                   &tmpDestRect,
                                   0xCC,
                                   0xCC,
                                   dstFormat));
                }
#endif  /* gcdNEED_FILTER_BLIT */
            }
        }

        if (gcmIS_ERROR(status))
        {
            break;
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

