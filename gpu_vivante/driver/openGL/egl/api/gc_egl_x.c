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
 * X Windows graphics API code.
 */

#include "gc_egl_precomp.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <memory.h>

/*******************************************************************************
***** Version Signature *******************************************************/

const char * _EGL_PLATFORM = "\n\0$PLATFORM$X$\n";

NativeDisplayType
veglGetDefaultDisplay(
    void
    )
{
    return XOpenDisplay(NULL);
}

void
veglReleaseDefaultDisplay(
    IN NativeDisplayType Display
    )
{
    if (Display != gcvNULL)
    {
        XCloseDisplay(Display);
    }
}

gctBOOL
veglIsValidDisplay(
    IN NativeDisplayType Display
    )
{
    if (XProtocolVersion(Display) == 0)
    {
        return gcvFALSE;
    }

    return gcvTRUE;
}

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
	IN NativeWindowType Window,
	OUT VEGLDisplayInfo Info
    )
{
    return gcvFALSE;
}

gctBOOL
veglGetDisplayBackBuffer(
    IN VEGLDisplay Display,
	IN NativeWindowType Window,
    OUT VEGLBackBuffer BackBuffer,
    OUT gctBOOL_PTR Flip
    )
{
    *Flip = gcvFALSE;

    return gcvFALSE;
}

gctBOOL
veglSetDisplayFlip(
    IN NativeDisplayType Display,
	IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer
    )
{
    return gcvFALSE;
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
    return gcvFALSE;
}

void
_GetColorBitsInfoFromMask(
    unsigned long Mask,
    unsigned int *Length,
    unsigned int *Offset
)
{
    size_t start, end;
    size_t i;

    if (Length == NULL && Offset == NULL)
    {
        return;
    }

    if (Mask == 0)
    {
        start = 0;
        end   = 0;
    }
    else
    {
        start = end = -1;

        for (i = 0; i < (sizeof(Mask) * 8); i++)
        {
            if (start == -1)
            {
                if ((Mask & (1 << i)) > 0)
                {
                    start = i;
                }
            }
            else if ((Mask & (1 << i)) == 0)
            {
                end = i;
                break;
            }
        }

        if (end == -1)
        {
            end = i;
        }
    }

    if (Length != NULL)
    {
        *Length = (unsigned) (end - start);
    }

    if (Offset != NULL)
    {
        *Offset = (unsigned) start;
    }
}

gctBOOL
veglGetWindowInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType WindowHandle,
    OUT gctINT_PTR X,
    OUT gctINT_PTR Y,
    OUT gctUINT_PTR Width,
    OUT gctUINT_PTR Height,
    OUT gctUINT_PTR BitsPerPixel,
    OUT gceSURF_FORMAT * Format
    )
{
    XStatus status;
    XWindowAttributes attr;
    XImage *image;
    unsigned int    alphaLength;
    unsigned int    alphaOffset;
    unsigned int    redLength;
    unsigned int    redOffset;
    unsigned int    greenLength;
    unsigned int    greenOffset;
    unsigned int    blueLength;
    unsigned int    blueOffset;

    if(Display == gcvNULL)
        return gcvFALSE;
    /* Query window parameters. */
    status = XGetWindowAttributes(Display->hdc, WindowHandle, &attr);

    if (status == False)
    {
        /* Error. */
        return gcvFALSE;
    }

    if (X != NULL)
    {
        *X = attr.x;
    }

    if (Y != NULL)
    {
        *Y = attr.y;
    }

    if (Width != NULL)
    {
        *Width = attr.width;
    }

    if (Height != NULL)
    {
        *Height = attr.height;
    }

    image = XGetImage(Display->hdc,
        DefaultRootWindow(Display->hdc),
        0, 0, 1, 1, AllPlanes, ZPixmap);

    if (image == NULL)
    {
        /* Error. */
        return gcvFALSE;
    }
    if(BitsPerPixel != NULL)
        *BitsPerPixel = image->bits_per_pixel;

    if(Format != NULL)
    {
        /* Get the color info. */
        alphaLength = image->bits_per_pixel - image->depth;
        alphaOffset = image->depth;

        _GetColorBitsInfoFromMask(image->red_mask, &redLength, &redOffset);
        _GetColorBitsInfoFromMask(image->green_mask, &greenLength, &greenOffset);
        _GetColorBitsInfoFromMask(image->blue_mask, &blueLength, &blueOffset);


        /* Get the color format. */
        switch (greenLength)
        {
        case 4:
            if (blueOffset == 0)
            {
                *Format = (alphaLength == 0) ? gcvSURF_X4R4G4B4 : gcvSURF_A4R4G4B4;
            }
            else
            {
                *Format = (alphaLength == 0) ? gcvSURF_X4B4G4R4 : gcvSURF_A4B4G4R4;
            }
            break;

        case 5:
            if (blueOffset == 0)
            {
                *Format = (alphaLength == 0) ? gcvSURF_X1R5G5B5 : gcvSURF_A1R5G5B5;
            }
            else
            {
                *Format = (alphaLength == 0) ? gcvSURF_X1B5G5R5 : gcvSURF_A1B5G5R5;
            }
            break;

        case 6:
            *Format = gcvSURF_R5G6B5;
            break;

        case 8:
            if (blueOffset == 0)
            {
                *Format = (alphaLength == 0) ? gcvSURF_X8R8G8B8 : gcvSURF_A8R8G8B8;
            }
            else
            {
                *Format = (alphaLength == 0) ? gcvSURF_X8B8G8R8 : gcvSURF_A8B8G8R8;
            }
            break;

        default:
            /* Unsupported colot depth. */
            return gcvFALSE;
        }
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
    gctBOOL result = gcvFALSE;
    XImage* xImage = NULL;
    Screen* xScreen;
    Visual* xVisual;
    gctUINT xDepth;
    gctUINT xBpp;
    int     status;
    gctPOINTER bits = Bits;
    GC gc;

    do
    {
        gctUINT alignedWidth, alignedHeight;
        gceORIENTATION orientation;

        /* Create graphics context. */
        gc = XCreateGC(Display->hdc, Surface->hwnd, 0, gcvNULL);

        /* Fetch defaults. */
        xScreen = DefaultScreenOfDisplay(Display->hdc);
        xVisual = DefaultVisualOfScreen(xScreen);
        xDepth  = DefaultDepthOfScreen(xScreen);
        xBpp = Surface->swapBitsPerPixel;

        alignedWidth = Surface->bitsAlignedWidth;
        alignedHeight = Surface->bitsAlignedHeight;

        gcmVERIFY_OK(gcoSURF_QueryOrientation(
            Surface->swapSurface, &orientation
            ));

        if (orientation == gcvORIENTATION_BOTTOM_TOP)
        {
            gctINT stride = alignedWidth * xBpp / 8;
            gctINT bitmapSize = alignedHeight * stride;

            char* dest;
            char* src;
            int i;

            if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL, bitmapSize, &bits)))
            {
                break;
            }

            dest = (char*) bits;
            src  = (char*) Bits + stride * (alignedHeight - 1);

            for (i = 0; i < alignedHeight; i++)
            {
                memcpy(dest, src, stride);

                dest = dest + stride;
                src  = src  - stride;
            }
        }

        /* Create image from the bits. */
        xImage = XCreateImage(
            Display->hdc,
            xVisual,
            xDepth,
            ZPixmap,
            0,
            (char*) bits,
            alignedWidth,
            alignedHeight,
            8,
            0
            );

        if (xImage == gcvNULL)
        {
            /* Error. */
            break;
        }

        /* Draw the image. */
        status = XPutImage(
            Display->hdc,
            Surface->hwnd,
            gc,
            xImage,
            Left, Top,      /* Source origin. */
            Left, Top,      /* Destination origin. */
            Width, Height   /* Image size. */
            );

        if (status != Success)
        {
            /* Error. */
            break;
        }

        /* Flush buffers. */
        status = XFlush(Display->hdc);

        if (status != Success)
        {
            /* Error. */
            break;
        }

        /* Success. */
        result = gcvTRUE;
    }
    while (gcvFALSE);

    /* Destroy the image. */
    if (xImage != NULL)
    {
        xImage->data = gcvNULL;
        XDestroyImage(xImage);
    }

    /* Free graphics context. */
    XFreeGC(Display->hdc, gc);

    if ((bits != Bits) && (bits != gcvNULL))
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, bits));
    }

    /* Return result. */
    return result;
}

#ifdef CUSTOM_PIXMAP
typedef struct tagNWSPixmap {
    int iWidth;     //Pixmap colorbuffer width
    int iHeight;        //Pixmap colorbuffer width
    int iStride;        //Pixmap stride
    gceSURF_FORMAT eFormat; //Pixmap colorbuffer format
    void *pBits;        //Pixmap colorbuffer pointer
} NWSPixmap;
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
    gctBOOL result = gcvFALSE;
    Screen* xScreen;
    Visual* xVisual;
    int     status;
    XImage* xImage = NULL;
    GC gc = NULL;

    do
    {
        /* Create graphics context. */
        gc = XCreateGC(Display->hdc, Pixmap, 0, gcvNULL);

        /* Fetch defaults. */
        xScreen = DefaultScreenOfDisplay(Display->hdc);
        xVisual = DefaultVisualOfScreen(xScreen);

        /* Create image from the bits. */
        xImage = XCreateImage(
            Display->hdc,
            xVisual,
            BitsPerPixel,
            ZPixmap,
            0,
            (char*) Bits,
            Width,
            Height,
            8,
            Width * BitsPerPixel / 8
            );

        if (xImage == gcvNULL)
        {
            /* Error. */
            break;
        }

        /* Draw the image. */
        status = XPutImage(
            Display->hdc,
            Pixmap,
            gc,
            xImage,
            Left, Top,      /* Source origin. */
            Left, Top,      /* Destination origin. */
            Right - Left, Bottom - Top
            );

        if (status != Success)
        {
            /* Error. */
            break;
        }

        /* Flush buffers. */
        status = XFlush(Display->hdc);

        if (status != Success)
        {
            /* Error. */
            break;
        }

        /* Success. */
        result = gcvTRUE;
    }
    while (gcvFALSE);

    /* Destroy the image. */
    if (xImage != NULL)
    {
        xImage->data = gcvNULL;
        XDestroyImage(xImage);
    }

    /* Free graphics context. */
    XFreeGC(Display->hdc, gc);

    /* Return result. */
    return result;
}

EGLint
veglGetNativeVisualId(
    IN VEGLDisplay Display,
    IN VEGLConfig Config
    )
{
    /* Return visualid from default screen. */
    return (EGLint)
        DefaultVisual(VEGL_DISPLAY(Display)->hdc,
                      DefaultScreen(VEGL_DISPLAY(Display)->hdc))->visualid;
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
#ifdef CUSTOM_PIXMAP
    *Width = ((NWSPixmap *) Pixmap)->iWidth;
    *Height = ((NWSPixmap *) Pixmap)->iHeight;

    *BitsPerPixel = 16;
    *Format = ((NWSPixmap *) Pixmap)->eFormat;

    return gcvTRUE;
#else
    XStatus  status;
    Window  rootWindow;
    gctUINT borderWidth;
    gctINT  x;
    gctINT  y;

    /* Query pixmap parameters. */
    status = XGetGeometry(Display,
                          Pixmap,
                          &rootWindow,
                          &x, &y, Width, Height,
                          &borderWidth,
                          BitsPerPixel);

    if (status == False)
    {
        return gcvFALSE;
    }

    /* Convert pixmap depth to surface format. */
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

    return gcvTRUE;
#endif
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
#ifdef CUSTOM_PIXMAP
    *Bits = ((NWSPixmap *) Pixmap)->pBits;
    *Stride = ((NWSPixmap *) Pixmap)->iStride;

    if (Physical != gcvNULL)
    {
        /* No physical address for pixmaps. */
        *Physical = ~0;
    }

    return gcvTRUE;
#else

    return gcvFALSE;
#endif
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
    return gcvFALSE;
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

gctBOOL
veglIsColorFormatSupport(
    IN NativeDisplayType Display,
    IN VEGLConfig       Config
    )
{
    return gcvTRUE;
}
