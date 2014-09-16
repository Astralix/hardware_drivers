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
 * Android window system specific egl code
 */

#include "gc_egl_precomp.h"

// To enable verbose messages, see cutils/log.h. Must be included before log.h
#define LOG_NDEBUG 1

#include <cutils/log.h>

#include <pixelflinger/format.h>

#include <ui/android_native_buffer.h>
#include <ui/egl/android_natives.h>
#include "gc_gralloc_priv.h"
#include <errno.h>

#define ENTERFCN() LOGV("%s\n",  __FUNCTION__ )

#define ANDROID_DUMMY (31415926)

gralloc_module_t *gralloc_module = gcvNULL;

NativeDisplayType
veglGetDefaultDisplay(
    void
    )
{
    NativeDisplayType dpy = (NativeDisplayType)ANDROID_DUMMY; /* just a silly, non-zero value.... */
    hw_module_t const* pModule;

    ENTERFCN();

    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pModule);
    gralloc_module = (gralloc_module_t *)(pModule);

    return dpy;
}

void
veglReleaseDefaultDisplay(
    IN NativeDisplayType Display
    )
{
    ENTERFCN();

    if (Display == (NativeDisplayType)ANDROID_DUMMY)
    {
        return;
    }
}

gctBOOL
veglIsValidDisplay(
    IN NativeDisplayType Display
    )
{
    return gcvTRUE;
}

static gceSTATUS
_ConvertFormat(
    IN gctINT format,
    OUT gctUINT_PTR BitsPerPixel,
    OUT gceSURF_FORMAT * Format
    )
{
    /* Convert Android format to surface format. */
    switch (format)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
        *Format = gcvSURF_R5G6B5;
        *BitsPerPixel = 16;
        break;

    case HAL_PIXEL_FORMAT_RGBA_8888:
        *Format = gcvSURF_A8B8G8R8;
        *BitsPerPixel = 32;
        break;

    case HAL_PIXEL_FORMAT_RGBX_8888:
        *Format = gcvSURF_X8B8G8R8;
        *BitsPerPixel = 32;
        break;

    case HAL_PIXEL_FORMAT_RGBA_4444:
        *Format = gcvSURF_R4G4B4A4;
        *BitsPerPixel = 16;
        break;

    case HAL_PIXEL_FORMAT_RGBA_5551:
        *Format = gcvSURF_R5G5B5A1;
        *BitsPerPixel = 16;
        break;

    case HAL_PIXEL_FORMAT_BGRA_8888:
        *Format = gcvSURF_A8R8G8B8;
        *BitsPerPixel = 32;
        break;

    default:
        return gcvSTATUS_NOT_SUPPORTED;
    }

    return gcvSTATUS_OK;
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
    gcmASSERT(Window != NULL);
    gcmASSERT(Width        != NULL);
    gcmASSERT(Height       != NULL);
    gcmASSERT(BitsPerPixel != NULL);
    gcmASSERT(Format       != NULL);

    gctINT width, height, format;
    gceSTATUS status;

    ENTERFCN();

    /* Query native window width and height. */
    Window->query(Window, NATIVE_WINDOW_WIDTH, &width);
    Window->query(Window, NATIVE_WINDOW_HEIGHT, &height);
    Window->query(Window, NATIVE_WINDOW_FORMAT, &format);

    LOGV("Window width = %d, height = %d format =%d", width, height, format);

    gcmONERROR(_ConvertFormat(format, BitsPerPixel, Format));

    /* Set the output window parameters. */
    * X      = 0;
    * Y      = 0;
    * Width  = width;
    * Height = height;

    /* Success. */
    return gcvTRUE;

OnError:
    LOGE("Unsupported format requested");
    * X            = 0;
    * Y            = 0;
    * Width        = 0;
    * Height       = 0;
    * BitsPerPixel = 0;
    return gcvFALSE;
}

gctBOOL
veglGetWindowBits(
    IN NativeDisplayType Display,
    IN gctPOINTER Buffer,
    OUT gctPOINTER * Logical,
    OUT gctUINT32_PTR Physical,
    OUT gctINT32_PTR Stride
    )
{
    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Buffer != gcvNULL);
    gcmASSERT(Logical != gcvNULL);
    gcmASSERT(Physical != gcvNULL);
    gcmASSERT(Stride != gcvNULL);

    android_native_buffer_t *buffer = (android_native_buffer_t *)Buffer;
    struct gc_private_handle_t *hnd = (struct gc_private_handle_t *)(buffer->handle);
    gctUINT bitsPerPixel;
    gceSURF_FORMAT format;

    ENTERFCN();

    /* retrieve the physical and logical addresses */
    *Logical = (gctPOINTER)hnd->common.base;
    *Physical = hnd->phys;

    _ConvertFormat(buffer->format, &bitsPerPixel, &format);

    /* Android stride is in pixels, whereas gcoSURF_* stride is in bytes. */
    /* We are assuming Stride parameter is prepopulated with Bpp */
    *Stride   = buffer->stride * bitsPerPixel / 8;

    LOGV("phys : %p, logi : %p, stride : %d, usage : 0x%08X", (void *)*Physical, *Logical, *Stride, buffer->usage);

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
    ENTERFCN();
    return gcvTRUE;
}

EGLint
veglGetNativeVisualId(
    IN VEGLDisplay Display,
    IN VEGLConfig Config
    )
{
    EGLint id = 0;

    gcmASSERT(Config != gcvNULL);

    if (Config->depthSize == 0)
    {
        switch (Config->bufferSize)
        {
        case 16:
            id = GGL_PIXEL_FORMAT_RGB_565;
            break;

        case 32:
            id = (Config->alphaSize == 0) ? GGL_PIXEL_FORMAT_RGBX_8888
               : (Config->swizzleRB == EGL_TRUE) ? GGL_PIXEL_FORMAT_BGRA_8888
               : GGL_PIXEL_FORMAT_RGBA_8888;
            break;

        default:
            break;
        }
    }

    return id;
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
    ENTERFCN();
    return gcvFALSE;
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
    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Pixmap != gcvNULL);

    gceSTATUS status;

    ENTERFCN();

    if (Pixmap->version != gcmSIZEOF(egl_native_pixmap_t))
    {
        LOGE("This is not an Android pixmap");
        goto OnError;
    }

    gcmONERROR(_ConvertFormat(Pixmap->format, BitsPerPixel, Format));

    if (Width != gcvNULL)
    {
        *Width = Pixmap->width;
    }

    if (Height != gcvNULL)
    {
        *Height = Pixmap->height;
    }

    return gcvTRUE;

OnError:
    return gcvFALSE;
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
    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Pixmap != gcvNULL);

    ENTERFCN();

    gctUINT BitsPerPixel;
    gceSURF_FORMAT Format;
    gceSTATUS status;

    if (Pixmap->version != gcmSIZEOF(egl_native_pixmap_t))
    {
        LOGE("This is not an Android pixmap");
        goto OnError;
    }

    if (Bits != gcvNULL)
    {
        *Bits = Pixmap->data;
    }

    gcmONERROR(_ConvertFormat(Pixmap->format, &BitsPerPixel, &Format));

    if (Stride != gcvNULL)
    {
        /* Vivante stride is in bytes. */
        *Stride = Pixmap->stride*(BitsPerPixel/8);
    }

    if (Physical != gcvNULL)
    {
        /* No physical address for pixmaps. */
        *Physical = ~0U;
    }

    return gcvTRUE;

OnError:
    return gcvFALSE;
}

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
    IN NativeWindowType Window,
    OUT VEGLDisplayInfo Info
    )
{
    ENTERFCN();

    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Display->hdc != gcvNULL);
    gcmASSERT(Window != gcvNULL);
    gcmASSERT(Info != gcvNULL);

    /* Set usage for native window. This is an egl surface,
       thus only GPU can render to it.
       GRALLOC_USAGE_SW_READ_NEVER and
       GRALLOC_USAGE_SW_WRITE_NEVER are not really needed,
       but it's better to be explicit. */
    native_window_set_usage(Window, GRALLOC_USAGE_HW_RENDER |
                                    GRALLOC_USAGE_SW_READ_NEVER |
                                    GRALLOC_USAGE_SW_WRITE_NEVER);

#if ANDROID_SDK_VERSION >= 11
    /* Set buffer count for Honeycomb and later. */
    gctINT minUndequeued;
    gctINT concreteType;

    Window->query(Window, NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS, &minUndequeued);
    Window->query(Window, NATIVE_WINDOW_CONCRETE_TYPE, &concreteType);

    if (concreteType != NATIVE_WINDOW_FRAMEBUFFER)
    {
        /* Set native buffer count. */
        native_window_set_buffer_count(Window, minUndequeued + 1);

    }
#endif

    /* We don't know these values until we dequeue a buffer.
       We can't dequeue here. For now use a dummy value. */
    Info->logical = (gctPOINTER) ANDROID_DUMMY;
    Info->physical = ANDROID_DUMMY;

    /* Bytes or pixels? Dummy for now. */
    Info->stride = ANDROID_DUMMY;

    /* Query native window width and height. */
    Window->query(Window, NATIVE_WINDOW_WIDTH, &Info->width);
    Window->query(Window, NATIVE_WINDOW_HEIGHT, &Info->height);

    /* In Android we always flip. */
    Info->flip = gcvTRUE;

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
    android_native_buffer_t *buffer;
    struct gc_private_handle_t *hnd;
    void *vaddr = gcvNULL;
    int rel;
    gctINT tryCount;

    ENTERFCN();

    /* Dequeue a buffer from native window. */
    for (tryCount = 0; tryCount  < 200; tryCount++)
    {
        rel = Window->dequeueBuffer(Window, &buffer);

        if (rel != -EBUSY)
        {
            break;
        }

        gcoOS_Delay(gcvNULL, tryCount / 2);
    }

    if (rel != 0)
    {
        gcsHAL_INTERFACE iface;
        LOGE("Failed to dequeue buffer: errno=%d", rel);

        iface.command = gcvHAL_RESET;
        gcoHAL_Call(gcvNULL, &iface);

        return gcvFALSE;
    }

    /* Lock buffer before rendering. */
    if (Window->lockBuffer(Window, buffer))
    {
        LOGE("Failed to lock the window buffer");

#if ANDROID_SDK_VERSION >= 9
        /* Cancel this buffer. */
        Window->cancelBuffer(Window, buffer);
#else
        /* Queue back. */
        Window->queueBuffer(Window, buffer);
#endif

        return gcvFALSE;
    }

    /* Keep a reference on the buffer. */
    buffer->common.incRef(&buffer->common);

    /* Now lock for our usage :
       GRALLOC_USAGE_SW_READ and
       GRALLOC_USAGE_SW_WRITE are not really needed, but we need them for now,
       per our gralloc logic */
    if (gralloc_module->lock(gralloc_module, buffer->handle,
                                    GRALLOC_USAGE_HW_RENDER /* |
                                    GRALLOC_USAGE_SW_READ_RARELY |
                                    GRALLOC_USAGE_SW_WRITE_RARELY */,
                            0, 0, buffer->width, buffer->height, &vaddr))
    {
        LOGE("gralloc failed to lock the buffer");
        return gcvFALSE;
    }

    hnd = (struct gc_private_handle_t *)(buffer->handle);

    BackBuffer->context = buffer;

    /* Get back buffer. */
    BackBuffer->surface = (hnd->surface != 0) ? (gcoSURF) hnd->surface
                        : (gcoSURF) hnd->resolveSurface;

    BackBuffer->offset   = 0;
    BackBuffer->origin.x = 0;
    BackBuffer->origin.y = 0;

    *Flip = gcvTRUE;

    LOGV("Buffer handle: %p", buffer);
    return gcvTRUE;
}

gctBOOL
veglSetDisplayFlip(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer
    )
{
    ENTERFCN();

    android_native_buffer_t * buffer;

    buffer = (android_native_buffer_t *) BackBuffer->context;


    if (gralloc_module->unlock(gralloc_module, buffer->handle))
    {
        LOGE("gralloc failed to unlock the buffer");
        return gcvFALSE;
    }

    /* Queue native buffer back into native window. Triggers composition. */
    Window->queueBuffer(Window, buffer);

    /* Decrease reference of native buffer.*/
    buffer->common.decRef(&buffer->common);

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
    return veglSetDisplayFlip(Display, Window, BackBuffer);
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
