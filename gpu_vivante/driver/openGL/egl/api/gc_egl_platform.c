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




/*
 * VDK API code.
 */

#include "gc_egl_precomp.h"
#include "gc_hal_eglplatform.h"

/*******************************************************************************
***** Version Signature *******************************************************/


NativeDisplayType
veglGetDefaultDisplay(
    void
    )
{
    NativeDisplayType display = gcvNULL;
    gcoOS_GetDisplay((HALNativeDisplayType*)(&display));
    return display;
}

void
veglReleaseDefaultDisplay(
    IN NativeDisplayType Display
    )
{
    gcoOS_DestroyDisplay(Display);
}

#ifndef USE_CE_DIRECTDRAW

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLDisplayInfo Info
    )
{
    halDISPLAY_INFO info;

    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Info != gcvNULL);

    /* Get display information. */
    if (gcmIS_ERROR(gcoOS_GetDisplayInfoEx(
        Display->hdc,
        Window,
        sizeof(info),
        &info)))
    {
        /* Error. */
        return gcvFALSE;
    }

    if ((info.logical == gcvNULL) || (info.physical == ~0U))
    {
        /* No offset. */
        return gcvFALSE;
    }

    /* Return window addresses. */
    Info->logical  = info.logical;
    Info->physical = info.physical;
    Info->stride   = info.stride;

    /* Return flip support. */
    Info->flip = info.flip;

#ifdef __QNXNTO__
    Info->width  = info.width;
    Info->height = info.height;

    if (gcmIS_ERROR(gcoOS_GetNextDisplayInfoEx(
        Display->hdc,
        Window,
        sizeof(info),
        &info)))
    {
        return gcvFALSE;
    }

    Info->logical2  = info.logical;
    Info->physical2 = info.physical;
#else
    /* Get virtual display size. */
    if (gcmIS_ERROR(gcoOS_GetDisplayVirtual(Display->hdc, &Info->width, &Info->height)))
    {
        Info->width  = -1;
        Info->height = -1;
    }
#endif

#ifndef __QNXNTO__
    /* 355_FB_MULTI_BUFFER */
    Info->multiBuffer = info.multiBuffer;
#endif

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglGetWindowInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT gctINT_PTR X,
    OUT gctINT_PTR Y,
    OUT gctUINT_PTR Width,
    OUT gctUINT_PTR Height,
    OUT gctUINT_PTR BitsPerPixel,
    OUT gceSURF_FORMAT * Format
    )
{
    halDISPLAY_INFO info;

#ifdef __QNXNTO__
    if (gcmIS_ERROR(gcoOS_GetWindowInfo(
                          Display->hdc,
                          Window,
                          X, Y,
                          (gctINT_PTR) Width, (gctINT_PTR) Height,
                          (gctINT_PTR) BitsPerPixel,
                          (gctINT_PTR) Format,
                          gcvNULL)))
    {
        return gcvFALSE;
    }

    return gcvTRUE;
#else
    if (gcmIS_ERROR(gcoOS_GetWindowInfo(
                          Display->hdc,
                          Window,
                          X,
						  Y,
                          (gctINT_PTR) Width,
						  (gctINT_PTR) Height,
                          (gctINT_PTR) BitsPerPixel,
                          gcvNULL)))
    {
        return gcvFALSE;
    }
#endif

    if (gcmIS_ERROR(gcoOS_GetDisplayInfoEx(
        Display->hdc,
        Window,
        sizeof(info),
        &info)))
    {
        return gcvFALSE;
    }

    /* Get the color format. */
    switch (info.greenLength)
    {
    case 4:
        if (info.blueOffset == 0)
        {
            *Format = (info.alphaLength == 0) ? gcvSURF_X4R4G4B4 : gcvSURF_A4R4G4B4;
        }
        else
        {
            *Format = (info.alphaLength == 0) ? gcvSURF_X4B4G4R4 : gcvSURF_A4B4G4R4;
        }
        break;

    case 5:
        if (info.blueOffset == 0)
        {
            *Format = (info.alphaLength == 0) ? gcvSURF_X1R5G5B5 : gcvSURF_A1R5G5B5;
        }
        else
        {
            *Format = (info.alphaLength == 0) ? gcvSURF_X1B5G5R5 : gcvSURF_A1B5G5R5;
        }
        break;

    case 6:
        *Format = gcvSURF_R5G6B5;
        break;

    case 8:
        if (info.blueOffset == 0)
        {
            *Format = (info.alphaLength == 0) ? gcvSURF_X8R8G8B8 : gcvSURF_A8R8G8B8;
        }
        else
        {
            *Format = (info.alphaLength == 0) ? gcvSURF_X8B8G8R8 : gcvSURF_A8B8G8R8;
        }
        break;

    default:
        /* Unsupported colot depth. */
        return gcvFALSE;
    }

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglDrawImage(
    IN VEGLDisplay Display,
    IN VEGLSurface Surface,
    IN gctPOINTER Bits,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Width,
    IN gctINT Height
    )
{
    gctINT width, height;
    gceORIENTATION orientation;

    width  = Surface->bitsAlignedWidth;
    height = Surface->bitsAlignedHeight;

    gcmVERIFY_OK(gcoSURF_QueryOrientation(
        Surface->swapSurface, &orientation
        ));

    if (orientation == gcvORIENTATION_BOTTOM_TOP)
    {
        height = -height;
    }

    if(gcmIS_ERROR(gcoOS_DrawImage(
        Display->hdc,
        Surface->hwnd,
        Left, Top, Left + Width, Top + Height,
        width, height,
        Surface->swapBitsPerPixel,
        Bits
        )))
        return gcvFALSE;
    else
        return gcvTRUE;
}



gctBOOL
veglGetDisplayBackBuffer(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLBackBuffer BackBuffer,
    OUT gctBOOL_PTR Flip
    )
{
    BackBuffer->context = gcvNULL;
    BackBuffer->surface = gcvNULL;
    if(gcmIS_ERROR(gcoOS_GetDisplayBackbuffer(Display->hdc,
                                    Window,
                                    BackBuffer->context,
                                    BackBuffer->surface,
                                    &BackBuffer->offset,
                                    &BackBuffer->origin.x,
                                    &BackBuffer->origin.y)))
        *Flip = gcvFALSE;
    else
        *Flip = gcvTRUE;

    return gcvTRUE;
}

gctBOOL
veglSetDisplayFlip(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer
    )
{
    gctUINT offset = 0;
    gctINT x = 0, y = 0;
    gceSTATUS status;

    if (BackBuffer != gcvNULL)
    {
        offset = BackBuffer->offset;
        x = BackBuffer->origin.x;
        y = BackBuffer->origin.y;
    }

    status = gcoOS_SetDisplayVirtual(Display, Window, offset, x, y);

    if(gcmIS_ERROR(status))
        return gcvFALSE;
    else
        return gcvTRUE;
}

gctBOOL
veglSetDisplayFlipRegions(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer,
    IN gctINT NumRects,
    IN gctINT_PTR Rects
    )
{
#ifndef __QNXNTO__
    return veglSetDisplayFlip(Display, Window, BackBuffer);
#else
    gceSTATUS status = gcoOS_DisplayBufferRegions(Display, Window, NumRects, Rects);
    if(gcmIS_ERROR(status))
        return gcvFALSE;
    else
        return gcvTRUE;
#endif
}

gctBOOL
veglInitLocalDisplayInfo(
    VEGLDisplay Display
    )
{
    return gcvTRUE;
}

gctBOOL
veglDeinitLocalDisplayInfo(
    VEGLDisplay Display
    )
{
    return gcvTRUE;
}

#endif

gctBOOL
veglDrawPixmap(
    IN VEGLDisplay Display,
    IN EGLNativePixmapType Pixmap,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel,
    IN gctPOINTER Bits
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    status = gcoOS_DrawPixmap(
                    Display->hdc,
                    Pixmap,
                    Left, Top, Right, Bottom,
                    Width, Height,
                    BitsPerPixel, Bits
                    );
    if(gcmIS_ERROR(status))
        return gcvFALSE;
    else
        return gcvTRUE;
}

EGLint
veglGetNativeVisualId(
    IN VEGLDisplay dpy,
    IN VEGLConfig Config
    )
{
    return 0;
}

gctBOOL
veglGetPixmapInfo(
    IN NativeDisplayType Display,
    IN NativePixmapType Pixmap,
    OUT gctUINT_PTR Width,
    OUT gctUINT_PTR Height,
    OUT gctUINT_PTR BitsPerPixel,
    OUT gceSURF_FORMAT * Format
    )
{
    if (gcmIS_ERROR(gcoOS_GetPixmapInfo(
                        Display,
                        Pixmap,
                      (gctINT_PTR) Width, (gctINT_PTR) Height,
                      (gctINT_PTR) BitsPerPixel,
                      gcvNULL,
                      gcvNULL)))
    {
        return gcvFALSE;
    }

    /* Return format for pixmap depth. */
    switch (*BitsPerPixel)
    {
    case 16:
        *Format = gcvSURF_R5G6B5;
        break;

    case 32:
        *Format = gcvSURF_X8R8G8B8;
        break;

    default:
        return gcvFALSE;
    }

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglGetPixmapBits(
    IN NativeDisplayType Display,
    IN NativePixmapType Pixmap,
    OUT gctPOINTER * Bits,
    OUT gctINT_PTR Stride,
    OUT gctUINT32_PTR Physical
    )
{
    /* Query pixmap bits. */
    if (gcmIS_ERROR(gcoOS_GetPixmapInfo(
                        Display,
                        Pixmap,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        Stride,
                        Bits)))
    {
        return gcvFALSE;
    }

    if (Physical != gcvNULL)
    {
        /* No physical address for pixmaps. */
        *Physical = ~0U;
    }

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglGetWindowBits(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    OUT gctPOINTER * Logical,
    OUT gctUINT32_PTR Physical,
    OUT gctINT32_PTR Stride
    )
{
    halDISPLAY_INFO info;
    gctUINT offset;

    /* Get display information. */
    if (gcmIS_ERROR(gcoOS_GetDisplayInfoEx(
                        Display,
                        Window,
                        sizeof(info),
                        &info)))
    {
        /* Error. */
        return gcvFALSE;
    }

#ifdef __QNXNTO__
    /* Get window information. */
    if (gcmIS_ERROR(gcoOS_GetWindowInfo(
                        Display,
                        Window,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,

                        gcvNULL,

                        &offset)))
    {
        /* Error. */
        return gcvFALSE;
    }
#else
    /* Get window information. */
    if (gcmIS_ERROR(gcoOS_GetWindowInfo(
                        Display,
                        Window,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        gcvNULL,
                        &offset)))
    {
        /* Error. */
        return gcvFALSE;
    }
#endif


    if ((offset == ~0U) || (info.logical == gcvNULL) || (info.physical == ~0U))
    {
        /* No offset. */
        return gcvFALSE;
    }

    /* Compute window addresses. */
    *Logical  = (gctUINT8_PTR) info.logical + offset;
    *Physical = info.physical + offset;
    *Stride   = info.stride;

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglCopyPixmapBits(
    IN VEGLDisplay Display,
    IN NativePixmapType Pixmap,
    IN gctUINT DstWidth,
    IN gctUINT DstHeight,
    IN gctINT DstStride,
    IN gceSURF_FORMAT DstFormat,
    OUT gctPOINTER DstBits
    )
{
    return gcvFALSE;
}



gctBOOL
veglIsColorFormatSupport(
    IN NativeDisplayType Display,
    IN VEGLConfig       Config
    )
{
    return gcvTRUE;
}

gctBOOL
veglIsValidDisplay(
    IN NativeDisplayType Display
    )
{
    return (Display != gcvNULL);
}

#if defined(LINUX) && defined(EGL_API_FB) && !defined(__APPLE__)

NativeDisplayType
fbGetDisplay(
    void
    )
{
    NativeDisplayType display = gcvNULL;
    gcoOS_GetDisplay((HALNativeDisplayType*)(&display));
    return display;
}

EGLNativeDisplayType
fbGetDisplayByIndex(
    IN gctINT DisplayIndex
    )
{
    NativeDisplayType display = gcvNULL;
    gcoOS_GetDisplayByIndex(DisplayIndex, (HALNativeDisplayType*)(&display));
    return display;
}

void
fbGetDisplayGeometry(
    IN NativeDisplayType Display,
    OUT gctINT_PTR Width,
    OUT gctINT_PTR Height
    )
{
    gcoOS_GetDisplayInfo((HALNativeDisplayType)Display, Width, Height, gcvNULL, gcvNULL, gcvNULL);
}

void
fbDestroyDisplay(
    IN NativeDisplayType Display
    )
{
    gcoOS_DestroyDisplay((HALNativeDisplayType)Display);
}

NativeWindowType
fbCreateWindow(
    IN NativeDisplayType Display,
    IN gctINT X,
    IN gctINT Y,
    IN gctINT Width,
    IN gctINT Height
    )
{
    NativeWindowType Window = gcvNULL;
    gcoOS_CreateWindow((HALNativeDisplayType)Display, X, Y, Width, Height, (HALNativeWindowType*)(&Window));
    return Window;
}

void
fbGetWindowGeometry(
    IN NativeWindowType Window,
    OUT gctINT_PTR X,
    OUT gctINT_PTR Y,
    OUT gctINT_PTR Width,
    OUT gctINT_PTR Height
    )
{
    gcoOS_GetWindowInfo(gcvNULL, (HALNativeWindowType)Window, X, Y, Width, Height, gcvNULL, gcvNULL);
}

void
fbDestroyWindow(
    IN NativeWindowType Window
    )
{
    gcoOS_DestroyWindow(gcvNULL, (HALNativeWindowType)Window);
}

NativePixmapType
fbCreatePixmap(
    IN NativeDisplayType Display,
    IN gctINT Width,
    IN gctINT Height
    )
{
    NativePixmapType Pixmap = gcvNULL;
    gcoOS_CreatePixmap((HALNativeDisplayType)Display, Width, Height, 32, (HALNativePixmapType*)(&Pixmap));
    return Pixmap;
}

NativePixmapType
fbCreatePixmapWithBpp(
    IN NativeDisplayType Display,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel
    )
{
    NativePixmapType Pixmap = gcvNULL;
    gcoOS_CreatePixmap((HALNativeDisplayType)Display, Width, Height, BitsPerPixel, (HALNativePixmapType*)(&Pixmap));
    return Pixmap;
}

void
fbGetPixmapGeometry(
    IN NativePixmapType Pixmap,
    OUT gctINT_PTR Width,
    OUT gctINT_PTR Height
    )
{
    gcoOS_GetPixmapInfo(gcvNULL, (HALNativePixmapType)Pixmap, Width, Height, gcvNULL, gcvNULL, gcvNULL);
}

void
fbDestroyPixmap(
    IN NativePixmapType Pixmap
    )
{
    gcoOS_DestroyPixmap(gcvNULL, (HALNativePixmapType)Pixmap);
}

#endif
