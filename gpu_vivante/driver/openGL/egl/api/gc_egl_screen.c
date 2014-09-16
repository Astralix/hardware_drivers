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
 * $QNXLicenseC:
 * Copyright 2010, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software.   Free development licenses are
 * available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#include "gc_egl_precomp.h"

#include <stdio.h>
#include <screen/screen.h>
#include <EGL/egl.h>

NativeDisplayType
veglGetDefaultDisplay(
    void
    )
{
    return EGL_DEFAULT_DISPLAY;
}

void
veglReleaseDefaultDisplay(
    IN NativeDisplayType Display
    )
{
    /*
    ** nothing to do because GetDefaultDisplay simply returned an integer...
    */
}

gctBOOL
veglIsValidDisplay(
    IN NativeDisplayType Display
    )
{
    return gcvTRUE;
}

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLDisplayInfo Info
    )
{
    int rc, size[2];
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_buffer_property_iv(buf[0], SCREEN_PROPERTY_BUFFER_SIZE, size);
    if (rc) {
        return gcvFALSE;
    }

    /* For QNX, the Width and Height are size of the framebuffer. */
    Info->width = size[0];
    Info->height = size[1];

    if (!veglGetWindowBits(Display->hdc, Window, &Info->logical, &Info->physical, &Info->stride))
    {
        return gcvFALSE;
    }

    /* The next buffer. */
    rc = screen_post_window((screen_window_t)Window, buf[0], 0, NULL, 0);
    if (rc) {
        return gcvFALSE;
    }

    if (!veglGetWindowBits(Display->hdc, Window,
                &Info->logical2, &Info->physical2, &Info->stride))
    {
        return gcvFALSE;
    }

    printf("screen Width=%d, Height=%d.\n", Info->width, Info->height);

    return gcvTRUE;
}

gctBOOL
veglDisplayBuffer(
    IN NativeDisplayType Display,
    IN NativeWindowType Window
    )
{
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];

    int rc;
    int rects[4];

    rects[0] = 0;
    rects[1] = 0;

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_window_property_iv((screen_window_t)Window, SCREEN_PROPERTY_BUFFER_SIZE, &rects[2]);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_post_window((screen_window_t)Window, buf[0], 1, rects, 0);
    if (rc) {
        return gcvFALSE;
    }

    return gcvTRUE;
}

gctBOOL
veglDisplayBufferRegions(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN gctINT NumRects,
    IN gctINT_PTR Rects
    )
{
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];

    int rc;

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_post_window((screen_window_t)Window, buf[0], NumRects, Rects, 0);
    if (rc) {
        return gcvFALSE;
    }

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
    BackBuffer->context  = gcvNULL;
    BackBuffer->surface  = gcvNULL;
    BackBuffer->offset   = 0;
    BackBuffer->origin.x = 0;
    BackBuffer->origin.y = 0;

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
    return veglDisplayBuffer(Display, Window);
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
    return veglDisplayBufferRegions(Display, Window, NumRects, Rects);
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
    int rc, size[2], format;
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];

    gcmASSERT(WindowHandle != NULL);
    gcmASSERT(X            != NULL);
    gcmASSERT(Y            != NULL);
    gcmASSERT(Width        != NULL);
    gcmASSERT(Height       != NULL);
    gcmASSERT(BitsPerPixel != NULL);
    gcmASSERT(Format       != NULL);

    rc = screen_get_window_property_pv((screen_window_t)WindowHandle, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_buffer_property_iv(buf[0], SCREEN_PROPERTY_BUFFER_SIZE, size);
    if (rc) {
        return gcvFALSE;
    }

    *X = 0;
    *Y = 0;
    *Width = size[0];
    *Height = size[1];

    rc = screen_get_window_property_iv((screen_window_t)WindowHandle, SCREEN_PROPERTY_FORMAT, &format);
    if (rc) {
        return gcvFALSE;
    }

    switch (format) {
        case SCREEN_FORMAT_BYTE:
            *BitsPerPixel = 8;
            *Format = gcvSURF_L8;
            break;
        case SCREEN_FORMAT_RGBA4444:
            *BitsPerPixel = 16;
            *Format = gcvSURF_A4R4G4B4;
            break;
        case SCREEN_FORMAT_RGBX4444:
            *BitsPerPixel = 16;
            *Format = gcvSURF_X4R4G4B4;
            break;
        case SCREEN_FORMAT_RGBA5551:
            *BitsPerPixel = 16;
            *Format = gcvSURF_A1R5G5B5;
            break;
        case SCREEN_FORMAT_RGBX5551:
            *BitsPerPixel = 16;
            *Format = gcvSURF_X1R5G5B5;
            break;
        case SCREEN_FORMAT_RGB565:
            *BitsPerPixel = 16;
            *Format = gcvSURF_R5G6B5;
            break;
        case SCREEN_FORMAT_RGB888:
            *BitsPerPixel = 24;
            *Format = gcvSURF_R8G8B8;
            break;
        case SCREEN_FORMAT_RGBA8888:
            *BitsPerPixel = 32;
            *Format = gcvSURF_A8R8G8B8;
            break;
        case SCREEN_FORMAT_RGBX8888:
            *BitsPerPixel = 32;
            *Format = gcvSURF_X8R8G8B8;
            break;
        default:
            return gcvFALSE;
    }

    return gcvTRUE;
}

gctBOOL
veglSwapBuffer(
    IN NativeDisplayType Display,
    IN NativeWindowType Window
    )
{
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];
    int rc;

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_post_window((screen_window_t)Window, buf[0], 0, NULL, 0);
    if (rc) {
        return gcvFALSE;
    }

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
    /* no need as long as veglGetDisplayInfo returns true */
    return gcvFALSE;
}

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
    /* not needed as long as veglGetPixmapBits returns a pointer */
    return gcvFALSE;
}

EGLint
veglGetNativeVisualId(
    IN VEGLDisplay Display,
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
    int rc, size[2], format;

    rc = screen_get_pixmap_property_iv(Pixmap, SCREEN_PROPERTY_BUFFER_SIZE, size);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_pixmap_property_iv(Pixmap, SCREEN_PROPERTY_FORMAT, &format);
    if (rc) {
        return gcvFALSE;
    }

    switch (format) {
        case SCREEN_FORMAT_BYTE:
            *BitsPerPixel = 8;
            *Format = gcvSURF_L8;
            break;
        case SCREEN_FORMAT_RGBA4444:
            *BitsPerPixel = 16;
            *Format = gcvSURF_A4R4G4B4;
            break;
        case SCREEN_FORMAT_RGBX4444:
            *BitsPerPixel = 16;
            *Format = gcvSURF_X4R4G4B4;
            break;
        case SCREEN_FORMAT_RGBA5551:
            *BitsPerPixel = 16;
            *Format = gcvSURF_A1R5G5B5;
            break;
        case SCREEN_FORMAT_RGBX5551:
            *BitsPerPixel = 16;
            *Format = gcvSURF_X1R5G5B5;
            break;
        case SCREEN_FORMAT_RGB565:
            *BitsPerPixel = 16;
            *Format = gcvSURF_R5G6B5;
            break;
        case SCREEN_FORMAT_RGB888:
            *BitsPerPixel = 24;
            *Format = gcvSURF_R8G8B8;
            break;
        case SCREEN_FORMAT_RGBA8888:
            *BitsPerPixel = 32;
            *Format = gcvSURF_A8R8G8B8;
            break;
        case SCREEN_FORMAT_RGBX8888:
            *BitsPerPixel = 32;
            *Format = gcvSURF_X8R8G8B8;
            break;
        default:
            return gcvFALSE;
    }

    *Width = size[0];
    *Height = size[1];

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
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];
    void *pointer;
    off64_t paddr;
    int rc, stride;

    rc = screen_get_pixmap_property_pv(Pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void **)buf);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_buffer_property_pv(buf[0], SCREEN_PROPERTY_POINTER, &pointer);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_buffer_property_iv(buf[0], SCREEN_PROPERTY_STRIDE, &stride);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_buffer_property_llv(buf[0], SCREEN_PROPERTY_PHYSICAL_ADDRESS, &paddr);
    if (rc) {
        return gcvFALSE;
    }

    *Bits = pointer;
    *Stride = stride;

    if (Physical) {
        *Physical = paddr;
    }

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
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];
    void *pointer;
    off64_t paddr;
    int rc, stride;

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_buffer_property_pv(buf[0], SCREEN_PROPERTY_POINTER, &pointer);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_buffer_property_iv(buf[0], SCREEN_PROPERTY_STRIDE, &stride);
    if (rc) {
        return gcvFALSE;
    }

    rc = screen_get_buffer_property_llv(buf[0], SCREEN_PROPERTY_PHYSICAL_ADDRESS, &paddr);
    if (rc) {
        return gcvFALSE;
    }

    *Logical = pointer;
    *Physical = paddr;
    *Stride = stride;

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
