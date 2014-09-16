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


/**
 * \file gc_dfb_utils.c
 */

#include <directfb.h>

#include <direct/messages.h>

#include <core/coredefs.h>
#include <core/coretypes.h>
#include <core/state.h>
#include <core/gfxcard.h>
#include <core/surface.h>
#include <core/palette.h>

#include <gfx/convert.h>

#include "gc_dfb_state.h"
#include "gc_dfb_utils.h"
#include "gc_dfb.h"

D_DEBUG_DOMAIN( Gal_Utils, "Gal/Utils", "Utilities." );

/**
 * Utilities.
 */

/**
 * Check whether the color format is supported
 * by hardware as source surface color format.
 *
 * \param [in]    format: DFB color format
 *
 * \return    true: supported
 * \return    false: not supported
 */
bool
gal_is_source_format( void                  *dev,
                      DFBSurfacePixelFormat  color )
{
    GalDeviceData *vdev      = dev;
    bool           supported = false;

    D_ASSERT( vdev != NULL );

    supported = ((color == DSPF_ARGB1555) || /* A1R5G5B5 */
                 (color == DSPF_ARGB4444) || /* A4R4G4B4 */
                 (color == DSPF_ARGB)     || /* A8R8G8B8 */
                 (color == DSPF_RGB16)    || /* R5G6B5 */
                 (color == DSPF_RGB555)   || /* X1R5G5B5 */
                 (color == DSPF_RGB444)   || /* X4R4G4B4 */
                 (color == DSPF_RGB32)    || /* X8R8G8B8 */
                 (color == DSPF_LUT8)     || /* INDEX8 */
                 (color == DSPF_YV12)     || /* YV12 */
                 (color == DSPF_I420)     || /* I420 */
                 (color == DSPF_YUY2)     || /* YUY2 */
                 (color == DSPF_UYVY));      /* UYVY */

    if (!supported && vdev->hw_2d_pe20) {
        supported = ((color == DSPF_BGR555)    || /* B5G5R5X1 */
#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 4)
                      (color == DSPF_RGBA4444) || /* B4G4R4A4 */
#endif
                      (color == DSPF_RGB444)   || /* B4G4R4X4 */
                      (color == DSPF_A8)       || /* A8 */
                      (color == DSPF_NV12)     || /* NV12 */
                      (color == DSPF_NV21)     || /* NV21 */
                      (color == DSPF_NV16));      /* NV16 */
    }

    return supported;
}

/**
 * Check whether the color format is supported
 * by hardware as dest surface color format.
 *
 * \param [in]    format: DFB color format
 *
 * \return    true: supported
 * \return    false: not supported
 */
bool
gal_is_dest_format( void                  *dev,
                    DFBSurfacePixelFormat  color )
{
    GalDeviceData *vdev  = dev;

    bool supported = false;

    D_ASSERT( dev != NULL );

    supported = ((color == DSPF_ARGB1555) || /* A1R5G5B5 */
                 (color == DSPF_ARGB4444) || /* A4R4G4B4 */
                 (color == DSPF_ARGB)     || /* A8R8G8B8 */
                 (color == DSPF_RGB16)    || /* R5G6B5 */
                 (color == DSPF_RGB555)   || /* X1R5G5B5 */
                 (color == DSPF_RGB444)   || /* X4R4G4B4 */
                 (color == DSPF_RGB32));     /* X8R8G8B8 */

    if (!supported && vdev->hw_2d_pe20) {
        supported = ((color == DSPF_BGR555)    || /* B5G5R5X1 */
#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 4)
                     (color == DSPF_RGBA4444 ) || /* R4G4B4A4 */
#endif
                     (color == DSPF_A8)        || /* A8 */
                     (color == DSPF_YUY2));       /* YUY2 */
    }

    return supported;
}

/**
 * Check whether the color is YUV format.
 *
 * \param [in]    format: DFB color format
 *
 * \return    true: YUV format
 * \return    false: not YUV format
 */
bool
gal_is_yuv_format( DFBSurfacePixelFormat color )
{
    return ((color == DSPF_YV12) || /* YV12 */
            (color == DSPF_I420) || /* I420 */
            (color == DSPF_YUY2) || /* YUY2 */
            (color == DSPF_UYVY) || /* UYVY */
            (color == DSPF_NV12) || /* NV12 */
            (color == DSPF_NV21) || /* NV21 */
            (color == DSPF_AYUV) || /* AYUV */
            (color == DSPF_NV16));  /* NV16 */
}

/**
 * Convert DFB color format to native format.
 *
 * \param [in]    format: DFB color format
 * \param [out]    native_format: gc color format
 *
 * \return    true: success
 * \return    false: the format is not supported nativly
 */
bool
gal_get_native_format( DFBSurfacePixelFormat  format,
                       gceSURF_FORMAT        *native_format )
{
    gceSURF_FORMAT native;

    D_ASSERT( native_format != NULL );

    switch (format)
    {
    /* A1R5G5B5 */
    case DSPF_ARGB1555:
        native = gcvSURF_A1R5G5B5;
        break;
    /* A4R4G4B4 */
    case DSPF_ARGB4444:
        native = gcvSURF_A4R4G4B4;
        break;
#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 4)
    /* R4G4B4A4 */
    case DSPF_RGBA4444:
        native = gcvSURF_R4G4B4A4;
        break;
#endif
    /* A8R8G8B8 */
    case DSPF_ARGB:
        native = gcvSURF_A8R8G8B8;
        break;
    /* R5G6B5 */
    case DSPF_RGB16:
        native = gcvSURF_R5G6B5;
        break;
    /* X1R5G5B5 */
    case DSPF_RGB555:
        native = gcvSURF_X1R5G5B5;
        break;
    /* X4R4G4B4 */
    case DSPF_RGB444:
        native = gcvSURF_X4R4G4B4;
        break;
    /* X8R8G8B8 */
    case DSPF_RGB32:
        native = gcvSURF_X8R8G8B8;
        break;
    /* INDEX8 */
    case DSPF_LUT8:
        native = gcvSURF_INDEX8;
        break;
    /* YV12 */
    case DSPF_YV12:
        native = gcvSURF_YV12;
        break;
    /* I420 */
    case DSPF_I420:
        native = gcvSURF_I420;
        break;
    /* NV12 */
    case DSPF_NV12:
        native = gcvSURF_NV12;
        break;
    /* NV21 */
    case DSPF_NV21:
        native = gcvSURF_NV21;
        break;
    /* NV16 */
    case DSPF_NV16:
        native = gcvSURF_NV16;
        break;
    /* YUY2 */
    case DSPF_YUY2:
        native = gcvSURF_YUY2;
        break;
    /* UYVY */
    case DSPF_UYVY:
        native = gcvSURF_UYVY;
        break;
    case DSPF_A8:
        native = gcvSURF_A8;
        break;
    /* UNKNOWN */
    default:
        native = gcvSURF_UNKNOWN;

        D_BUG("unexpected pixelformat.\n");
        return false;
    }

    *native_format = native;

    return true;
}

/**
 * Union the rects.
 *
 * \param [in]    rects: rects array need to be unioned
 * \param [in]    num:   number of the rects
 * \param [out]    rect:  unioned rect
 *
 * \return    true: succeed
 * \return    false: failed
 */
bool
gal_rect_union( gcsRECT *rects,
                int         num,
                gcsRECT *rect )
{
    int i;

    D_ASSERT( rects != NULL );
    D_ASSERT( rect  != NULL );

    if (num < 1) {
        D_DEBUG_AT( Gal_Utils,
                    "Num is invalid.\n" );

        return false;
    }

    rect->left   = rects[0].left;
    rect->top    = rects[0].top;
    rect->right  = rects[0].right;
    rect->bottom = rects[0].bottom;

    for (i = 1; i < num; i++) {
        rect->left      = MIN( rect->left, rects[i].left );
        rect->top      = MIN( rect->top, rects[i].top );
        rect->right  = MAX( rect->right, rects[i].right );
        rect->bottom = MAX( rect->bottom, rects[i].bottom );
    }

    return true;
}

/**
 * Convert the pixel to color according to the format.
 *
 * \param [in]    format:    color format
 * \param [in]    pixel:       input pixel
 * \param [out]    ret_color: output color
 *
 * \return    none
 */
void
gal_pixel_to_color_wo_expantion( DFBSurfacePixelFormat  format,
                                 unsigned long          pixel,
                                 DFBColor              *ret_color )
{
    D_ASSERT( ret_color != NULL );

    ret_color->a = 0xff;

    switch (format)
    {
    case DSPF_RGB332:
        ret_color->r = (pixel & 0xe0) >> 5;
        ret_color->g = (pixel & 0x1c) >> 2;
        ret_color->b = (pixel & 0x03);
        break;

    case DSPF_ARGB1555:
        ret_color->a =  pixel >> 15;
    case DSPF_RGB555:
        ret_color->r = (pixel & 0x7c00) >> 10;
        ret_color->g = (pixel & 0x03e0) >> 5;
        ret_color->b = (pixel & 0x001f);
        break;

    case DSPF_BGR555:
        ret_color->r = (pixel & 0x001f);
        ret_color->g = (pixel & 0x03e0) >> 5;
        ret_color->b = (pixel & 0x7c00) >> 10;
        break;

    case DSPF_ARGB2554:
        ret_color->a =  pixel >> 14;
        ret_color->r = (pixel & 0x3e00) >> 9;
        ret_color->g = (pixel & 0x01f0) >> 4;
        ret_color->b = (pixel & 0x000f);
        break;

    case DSPF_ARGB4444:
        ret_color->a =  pixel >> 12;
    case DSPF_RGB444:
        ret_color->r = (pixel & 0x0f00) >> 8;
        ret_color->g = (pixel & 0x00f0) >> 4;
        ret_color->b = (pixel & 0x000f);
        break;

#if (DIRECTFB_MAJOR_VERSION >= 1) && (DIRECTFB_MINOR_VERSION >= 4)
    case DSPF_RGBA4444:
        ret_color->r = (pixel         ) >> 12;
        ret_color->g = (pixel & 0x0f00) >> 8;
        ret_color->b = (pixel & 0x00f0) >> 4;
        ret_color->a = (pixel & 0x000f);
        break;
#endif

    case DSPF_RGB16:
        ret_color->r = (pixel & 0xf800) >> 11;
        ret_color->g = (pixel & 0x07e0) >> 5;
        ret_color->b = (pixel & 0x001f);
        break;

    case DSPF_ARGB:
        ret_color->a = pixel >> 24;
    case DSPF_RGB24:
    case DSPF_RGB32:
        ret_color->r = (pixel & 0xff0000) >> 16;
        ret_color->g = (pixel & 0x00ff00) >> 8;
        ret_color->b = (pixel & 0x0000ff);
        break;

    case DSPF_AiRGB:
        ret_color->a = (pixel >> 24) ^ 0xff;
        ret_color->r = (pixel & 0xff0000) >> 16;
        ret_color->g = (pixel & 0x00ff00) >> 8;
        ret_color->b = (pixel & 0x0000ff);
        break;

    default:
        ret_color->r = 0;
        ret_color->g = 0;
        ret_color->b = 0;
    }

    return;
}

/* Scan convert edges into lines. */
/* Assume E1 is left edge and E2 is right edge. */
/* Assume E1,E2 are not horizontal. */
/* Assume E1,E2's top and bottom y are the same. */
void
scanConvertEdges( int E1_tx, int E1_ty,
                  int E1_bx, int E1_by,
                  int E2_tx, int E2_ty,
                  int E2_bx, int E2_by,
                  gcsRECT* drect, int* numRect,
                  GalTriangleUnitType type )
{
    int y;
    double x1, x2, dx1, dx2;
    int rectAdded = 0;
    gcsRECT *rect = &drect[*numRect];

    dx1 = (E1_bx - E1_tx) / (double)(E1_ty - E1_by);
    dx2 = (E2_bx - E2_tx) / (double)(E2_ty - E2_by);

    x1 = E1_tx;
    x2 = E2_tx;

    /* Start with lower of top points */
    y = E1_ty;

    while ( y >= E1_by )
    {
        if ( (int)(x2 - x1) >= 0 )
        {
            rect->left   = (int)x1;
            rect->right  = (int)(x2 + 1);
            rect->top    = y;
            if (type == TRIANGLE_UNIT_RECT) {
                rect->bottom = y + 1;
            }
            else {
                rect->bottom = y;
            }
            rect++;
            rectAdded++;
        }

        x1+= dx1;
        x2+= dx2;
        y--;
    }

    *numRect += rectAdded;
}

int
mod( int x )
{
    return (x > 0) ? x : -x;
}

/* Scan convert triangle into lines */
void
scanConvertTriangle( DFBTriangle         *tri,
                     gcsRECT             *drect,
                     int                 *numRect,
                     GalTriangleUnitType  type )
{
    /* TxTy<->BxBy is max edge, VxVy is third vertex */
    int Tx, Ty, Vx, Vy, Bx, By, Ix;

    *numRect = 0;

    /* Find edge TxTy<->BxBy with max y span. */
    if ( mod(tri->y1 - tri->y2) > mod(tri->y1 - tri->y3) )
    {
        if ( mod(tri->y1 - tri->y2) > mod(tri->y2 - tri->y3) )
        {
            /* 1-2 edge has longest y span */
            if ( tri->y1 > tri->y2 )
            {
                Tx = tri->x1;
                Ty = tri->y1;
                Bx = tri->x2;
                By = tri->y2;
                Vx = tri->x3;
                Vy = tri->y3;
            }
            /* 2-1 edge has longest y span */
            else
            {
                Tx = tri->x2;
                Ty = tri->y2;
                Bx = tri->x1;
                By = tri->y1;
                Vx = tri->x3;
                Vy = tri->y3;
            }
        }
        else
        {
            /* 2-3 edge has longest y span */
            if ( tri->y2 > tri->y3 )
            {
                Tx = tri->x2;
                Ty = tri->y2;
                Bx = tri->x3;
                By = tri->y3;
                Vx = tri->x1;
                Vy = tri->y1;
            }
            /* 3-2 edge has longest y span */
            else
            {
                Tx = tri->x3;
                Ty = tri->y3;
                Bx = tri->x2;
                By = tri->y2;
                Vx = tri->x1;
                Vy = tri->y1;
            }
        }
    }
    else
    {
        if ( mod(tri->y1 - tri->y3) > mod(tri->y2 - tri->y3) )
        {
            /* 1-3 edge has longest y span */
            if ( tri->y1 > tri->y3 )
            {
                Tx = tri->x1;
                Ty = tri->y1;
                Bx = tri->x3;
                By = tri->y3;
                Vx = tri->x2;
                Vy = tri->y2;
            }
            /* 3-1 edge has longest y span */
            else
            {
                Tx = tri->x3;
                Ty = tri->y3;
                Bx = tri->x1;
                By = tri->y1;
                Vx = tri->x2;
                Vy = tri->y2;
            }
        }
        else
        {
            /* 2-3 edge has longest y span */
            if ( tri->y2 > tri->y3 )
            {
                Tx = tri->x2;
                Ty = tri->y2;
                Bx = tri->x3;
                By = tri->y3;
                Vx = tri->x1;
                Vy = tri->y1;
            }
            /* 3-2 edge has longest y span */
            else
            {
                Tx = tri->x3;
                Ty = tri->y3;
                Bx = tri->x2;
                By = tri->y2;
                Vx = tri->x1;
                Vy = tri->y1;
            }
        }
    }

    /* Find Intersection point Ix,y on TxTy<->BxBy line where y=Vy */
    Ix = (int)((Vy - Ty)*(Bx-Tx)/((double)(By-Ty)) + Tx);

    /* Top half of triangle */
    /* Check horizontal bottom or top line */
    if ( Vy != Ty )
    {
        if ( Vx < Ix )
            scanConvertEdges(Tx, Ty, Vx, Vy, Tx, Ty, Ix, Vy, drect, numRect, type);
        else
            scanConvertEdges(Tx, Ty, Ix, Vy, Tx, Ty, Vx, Vy, drect, numRect, type);
    }

    /* Remove overlap of one scan line between the two triangles. */
    if ( *numRect > 0 )
        (*numRect)--;

    /* Bottom half of triangle */
    if ( Vy != By )
    {
        if ( Vx < Ix )
            scanConvertEdges(Vx, Vy, Bx, By, Ix, Vy, Bx, By, drect, numRect, type);
        else
            scanConvertEdges(Ix, Vy, Bx, By, Vx, Vy, Bx, By, drect, numRect, type);
    }

}

/**
 * Get single stretch factor.
 *
 * \param [in]    src_size: size of the source
 * \param [in]    src_size: size of the dest
 *
 * \return    strectch factor
 */
static gctUINT32
gal_get_stretch_factor(    gctINT32 src_size,
                        gctINT32 dest_size )
{
    gctUINT32 stretch_factor;

    if ((src_size > 0) && (dest_size > 1)) {
        stretch_factor = ((src_size - 1) << 16) / (dest_size - 1);
    }
    else {
        stretch_factor = 0;
    }

    return stretch_factor;
}

/**
 * Get stretch factors.
 *
 * \param [in]    src_rect: source rectangle
 * \param [in]    dst_rect: dest rectangle
 * \param [out]    hor_factor: hor factor
 * \param [out]    ver_factor: ver factor
 *
 * \return    gcvSTATUS_OK: succeed
 */
gceSTATUS
gal_get_stretch_factors( gcsRECT_PTR  src_rect,
                         gcsRECT_PTR  dst_rect,
                         gctUINT32   *hor_factor,
                         gctUINT32   *ver_factor )
{
    int       src, dest;
    gceSTATUS status;

    D_ASSERT( hor_factor != NULL );
    D_ASSERT( ver_factor != NULL );

    do {
        /* Compute width of rectangles. */
        gcmERR_BREAK(gcsRECT_Width( src_rect, &src ));
        gcmERR_BREAK(gcsRECT_Width( dst_rect, &dest ));

        /* Compute and return horizontal stretch factor. */
        *hor_factor = gal_get_stretch_factor(src, dest);

        /* Compute height of rectangles. */
        gcmERR_BREAK(gcsRECT_Height( src_rect, &src ));
        gcmERR_BREAK(gcsRECT_Height( dst_rect, &dest ));

        /* Compute and return vertical stretch factor. */
        *ver_factor = gal_get_stretch_factor(src, dest);
    } while (0);

    if (gcmIS_ERROR( status )) {
        D_DEBUG_AT( Gal_Utils,
                    "Failed to get the stretch factors.\n" );
    }

    /* Success. */
    return status;
}
