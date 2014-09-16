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
 * Frame buffer graphics API code.
 */

#include "gc_egl_precomp.h"

#include <linux/fb.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/*******************************************************************************
***** Version Signature *******************************************************/

const char * _EGL_PLATFORM = "\n\0$PLATFORM$FBDev$\n";

struct _FBDisplay
{
    /* File descriptor to /dev/fb0. */
    gctINT              fd;

    /* Physical address */
    gctUINT32           physical;

    /* Logical address. */
    gctPOINTER          base;

    /* Size. */
    gctINT              width, height;

    /* Stride. */
    gctINT              stride;

    /* Size of the screen in bytes. */
    gctSIZE_T           scrnSize;

    /* Bits per pixel. */
    gctINT              bpp;

    /* Color format. */
    gceSURF_FORMAT      format;


	/* Multi-Buffer support. */
    struct fb_fix_screeninfo    fixInfo;
    struct fb_var_screeninfo    varInfo;
    struct fb_var_screeninfo    orgVarInfo;
    int                         backBufferY;
    int                         multiBuffer;
    pthread_cond_t              cond;
    pthread_mutex_t             condMutex;
};

struct _FBWindow
{
    /* Memory offset. */
    gctUINT32           offset;

    /* Bits per pixel. */
    gctINT              bpp;

    /* Color format. */
    gceSURF_FORMAT      format;

    /* Size. */
    gctINT              x, y;
    gctINT              width, height;

    /* Cursor. */
    gctINT              cursor;
};

struct _FBPixmap
{
    /* Pixels. */
    gctPOINTER          bits;

    /* Bits per pixel. */
    gctINT              bpp;

    /* Size. */
    gctINT              width, height;
    gctINT              stride;
};

NativeDisplayType
veglGetDefaultDisplay(
    void
    )
{
    NativeDisplayType dpy = gcvNULL;
    struct fb_fix_screeninfo fInfo;
    struct fb_var_screeninfo vInfo;
    VEGLThreadData thread;
	char *dev, *p;

    thread = veglGetThreadData();

    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        goto OnError;
    }

    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   gcmSIZEOF(struct _FBDisplay),
                                   (gctPOINTER *) &dpy)))
    {
        goto OnError;
    }

    gcoOS_ZeroMemory(dpy, sizeof(struct _FBDisplay));

	/* Initialize muti-buffer count. */
    p = getenv("FB_MULTI_BUFFER");
    if (p == NULL)
    {
        dpy->multiBuffer = 0;
    }
    else
    {
        dpy->multiBuffer = atoi(p);
    }

	/* Open the framebuffer device. */
    dev = getenv("FB_FRAMEBUFFER");

	dpy->fd = -1;
    if (dev != NULL)
    {
        dpy->fd = open(dev, O_RDWR);
    }

    if (dpy->fd < 0)
    {
        unsigned char i = 0;
        char const * const GALDeviceName[] =
        {
            "/dev/fb0",
            "/dev/graphics/fb0",
            gcvNULL
        };
        while((dpy->fd == -1) && GALDeviceName[i])
        {
            dpy->fd = open(GALDeviceName[i], O_RDWR);
            i++;
        }
    }

    if (dpy->fd < 0)
    {
        goto OnError;
    }

    /* Get fixed framebuffer information. */
    if (ioctl(dpy->fd, FBIOGET_FSCREENINFO, &fInfo) < 0)
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "FBEGL: Error reading fixed framebuffer information.");

        goto OnError;
    }

	dpy->fixInfo  = fInfo;
    dpy->physical = fInfo.smem_start;
    dpy->stride   = fInfo.line_length;

    /* Get variable framebuffer information. */
    if (ioctl(dpy->fd, FBIOGET_VSCREENINFO, &vInfo) < 0)
    {
        gcmTRACE(gcvLEVEL_ERROR,
             "FBEGL Error reading variable framebuffer information.");

        goto OnError;
    }

	dpy->orgVarInfo = dpy->varInfo= vInfo;
    dpy->width  = vInfo.xres;
    dpy->height = vInfo.yres;
    dpy->bpp    = vInfo.bits_per_pixel;

    /* Get the color format. */
    switch (vInfo.green.length)
    {
    case 4:
        if (vInfo.blue.offset == 0)
        {
            dpy->format = (vInfo.transp.length == 0) ? gcvSURF_X4R4G4B4 : gcvSURF_A4R4G4B4;
        }
        else
        {
            dpy->format = (vInfo.transp.length == 0) ? gcvSURF_X4B4G4R4 : gcvSURF_A4B4G4R4;
        }
        break;

    case 5:
        if (vInfo.blue.offset == 0)
        {
            dpy->format = (vInfo.transp.length == 0) ? gcvSURF_X1R5G5B5 : gcvSURF_A1R5G5B5;
        }
        else
        {
            dpy->format = (vInfo.transp.length == 0) ? gcvSURF_X1B5G5R5 : gcvSURF_A1B5G5R5;
        }
        break;

    case 6:
        dpy->format = gcvSURF_R5G6B5;
        break;

    case 8:
        if (vInfo.blue.offset == 0)
        {
            dpy->format = (vInfo.transp.length == 0) ? gcvSURF_X8R8G8B8 : gcvSURF_A8R8G8B8;
        }
        else
        {
            dpy->format = (vInfo.transp.length == 0) ? gcvSURF_X8B8G8R8 : gcvSURF_A8B8G8R8;
        }
        break;

    default:
        dpy->format = gcvSURF_UNKNOWN;
        break;
    }

    /* Figure out the size of the screen in bytes. */
    dpy->scrnSize = fInfo.smem_len;

    /* Map the framebuffer device to memory. */
    dpy->base = mmap(0,
                     dpy->scrnSize,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     dpy->fd,
                     0);

    if (dpy->base == MAP_FAILED)
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "FBDEV: Error mapping framebuffer device to memory.");

        goto OnError;
    }

	/* Mutex initialization. */
    pthread_cond_init(&dpy->cond, NULL);
    pthread_mutex_init(&dpy->condMutex, NULL);

    /* Success. */
    return dpy;

OnError:
    /* Failed and roll back the resource. */
    if (dpy != gcvNULL)
    {
        if(dpy->base != NULL)
        {
            munmap(dpy->base, dpy->scrnSize);
        }
        if (dpy->fd >= 0)
        {
            ioctl(dpy->fd, FBIOPUT_VSCREENINFO, &dpy->orgVarInfo);
            close(dpy->fd);
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, dpy));
    }

    return gcvNULL;
}

void
veglReleaseDefaultDisplay(
    IN NativeDisplayType Display
    )
{
    if (Display == gcvNULL)
    {
        return;
    }

    if (Display->base != gcvNULL)
    {
        /* Unmap the frame buffer. */
        munmap(Display->base, Display->scrnSize);
		Display->base = gcvNULL;
    }

	/* Reset the screen. */
    ioctl(Display->fd, FBIOPUT_VSCREENINFO, &Display->orgVarInfo);

    /* Close the file. */
    if (Display->fd > 0)
    {
        close(Display->fd);
		Display->fd = -1;
    }

	/* Destroy Mutex. */
    pthread_mutex_destroy(&Display->condMutex);
    pthread_cond_destroy(&Display->cond);

    /* Free the data structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Display));
}

gctBOOL
veglIsValidDisplay(
    IN NativeDisplayType Display
    )
{
    if (Display->fd <= 0)
    {
        return gcvFALSE;
    }

    return gcvTRUE;
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
    gcmASSERT(WindowHandle != NULL);
    gcmASSERT(Width        != NULL);
    gcmASSERT(Height       != NULL);
    gcmASSERT(BitsPerPixel != NULL);
    gcmASSERT(Format       != NULL);

    /* Set the output window parameters. */
    if(X != NULL)
        * X            = WindowHandle->x;
    if(Y != NULL)
        * Y            = WindowHandle->y;
    if(Width != NULL)
        * Width        = WindowHandle->width;
    if(Height != NULL)
        * Height       = WindowHandle->height;
    if(BitsPerPixel != NULL)
        * BitsPerPixel = WindowHandle->bpp;

    if (WindowHandle->format == gcvSURF_UNKNOWN)
    {
        return gcvFALSE;
    }
    else
    {
        *Format = WindowHandle->format;
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
#if USE_SW_FB
    return gcvFALSE;
#else
    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Window != gcvNULL);
    gcmASSERT(Logical != gcvNULL);
    gcmASSERT(Physical != gcvNULL);
    gcmASSERT(Stride != gcvNULL);
    if((Display == gcvNULL) || (Window == gcvNULL))
        return gcvFALSE;
    if((Window->offset == ~0U) || (Display->base == gcvNULL) || (Display->physical == ~0U))
        return gcvFALSE;
    if(Logical != gcvNULL)
        *Logical  = (gctUINT8_PTR) Display->base + Window->offset;
    if(Physical != gcvNULL)
        *Physical = Display->physical + Window->offset;
    if(Stride != gcvNULL)
        *Stride   = Display->stride;

    return gcvTRUE;
#endif
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
    gctINT y, bottom;
    gctINT width, height;
    gceORIENTATION orientation;
    gctUINT bytes;
    gctUINT8_PTR source;
    gctUINT8_PTR ptr;

    width  = Surface->bitsAlignedWidth;
    height = Surface->bitsAlignedHeight;

    gcmVERIFY_OK(gcoSURF_QueryOrientation(
        Surface->swapSurface, &orientation
        ));

    if (orientation == gcvORIENTATION_BOTTOM_TOP)
    {
        height = -height;
    }

    ptr = (gctUINT8_PTR) Display->hdc->base + (Left * Display->hdc->bpp / 8);

    bytes = Width * Surface->swapBitsPerPixel / 8;

    source = (gctUINT8_PTR) Bits;

    bottom = Top + Height;

    if(Display->hdc->bpp != Surface->swapBitsPerPixel)
        return 0;

    if (height < 0)
    {
        for (y = bottom - 1; y >= Top; --y)
        {
            gctUINT8_PTR line = ptr + y * Display->hdc->stride;
            memcpy(line, source, bytes);

            source += width * Surface->swapBitsPerPixel / 8;
        }
    }
    else
    {
        for (y = Top; y < bottom; ++y)
        {
            gctUINT8_PTR line = ptr + y * Display->hdc->stride;
            memcpy(line, source, bytes);

            source += width * Surface->swapBitsPerPixel / 8;
        }
    }

    return gcvTRUE;
}

EGLint
veglGetNativeVisualId(
    IN VEGLDisplay Display,
    IN VEGLConfig Config
    )
{
    /* Useless. */
    return 0;
}

#ifdef CUSTOM_PIXMAP
typedef struct tagNWSPixmap
{
    int             iWidth;     /* Pixmap colorbuffer width. */
    int             iHeight;    /* Pixmap colorbuffer width. */
    int             iStride;    /* Pixmap stride. */
    gceSURF_FORMAT  eFormat;    /* Pixmap colorbuffer format. */
    void *          pBits;      /* Pixmap colorbuffer pointer. */
}
NWSPixmap;
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
    gcmASSERT(Display != NULL);
    gcmASSERT(Pixmap != NULL);
    gcmASSERT(Width != NULL);
    gcmASSERT(Height != NULL);
    gcmASSERT(BitsPerPixel != NULL);
    gcmASSERT(Format != NULL);

    if ((Pixmap == gcvNULL)     ||
        (Display == gcvNULL)    ||
        (Width  == gcvNULL)     ||
        (Height == gcvNULL)     ||
        (BitsPerPixel == gcvNULL)   ||
        (Format == gcvNULL))
    {
        return gcvFALSE;
    }
#ifdef CUSTOM_PIXMAP
    {
        NWSPixmap * pixmap;
        gcsSURF_FORMAT_INFO_PTR format[2];

        /* Cast the pointer. */
        pixmap = (NWSPixmap *) Pixmap;

        /* Query the format specifics. */
        if (gcmIS_ERROR(gcoSURF_QueryFormat(pixmap->eFormat, format)))
        {
            return gcvFALSE;
        }

        *Width        = pixmap->iWidth;
        *Height       = pixmap->iHeight;
        *BitsPerPixel = format[0]->bitsPerPixel;
        *Format       = pixmap->eFormat;
    }
#else
    *Width        = Pixmap->width;
    *Height       = Pixmap->height;
    *BitsPerPixel = Pixmap->bpp;

    /* Convert window depth to surface format. */
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
#endif

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
    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Pixmap != gcvNULL);
    gcmASSERT(Bits != gcvNULL);
    gcmASSERT(Stride != gcvNULL);
    if(Pixmap == gcvNULL)
        return gcvFALSE;
#ifdef CUSTOM_PIXMAP
    if(Bits != gcvNULL)
        *Bits   = ((NWSPixmap *) Pixmap)->pBits;
    if(Stride != gcvNULL)
        *Stride = ((NWSPixmap *) Pixmap)->iStride;
#else
    if(Bits != gcvNULL)
        *Bits   = Pixmap->bits;
    if(Stride != gcvNULL)
        *Stride = Pixmap->stride;
#endif

    if (Physical != gcvNULL)
    {
        /* No physical address for pixmaps. */
        *Physical = ~0;
    }

    return gcvTRUE;
}

  /*****************************************************************************/
 /*************************************************************** FBDEV API ***/
/*****************************************************************************/

NativeDisplayType
fbGetDisplay(
    void
    )
{
    NativeDisplayType display = veglGetDefaultDisplay();
    return display;
}

void
fbGetDisplayGeometry(
    IN NativeDisplayType Display,
    OUT gctINT_PTR Width,
    OUT gctINT_PTR Height
    )
{
    gcmASSERT(Display != gcvNULL);

    if (Width  != gcvNULL) *Width  = Display->width;
    if (Height != gcvNULL) *Height = Display->height;
}

void
fbDestroyDisplay(
    IN NativeDisplayType Display
    )
{
    veglReleaseDefaultDisplay(Display);
}

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
	IN NativeWindowType Window,
	OUT VEGLDisplayInfo Info
    )
{
#if USE_SW_FB
    return gcvFALSE;
#else
    int i;

    gcmASSERT(Display != gcvNULL);
    gcmASSERT(Info != gcvNULL);

    if (Display->hdc == NULL)
    {
        /* Bad display pointer. */
        return gcvFALSE;
    }

    Info->logical  = Display->hdc->base;
    Info->physical = Display->hdc->physical;
    Info->stride   = Display->hdc->stride;

    if ((Info->logical == gcvNULL) || (Info->physical == ~0U))
    {
        /* No offset. */
        return gcvFALSE;
    }

    if(Display->hdc->multiBuffer <= 1)
    {
        Info->width = -1;
        Info->height = -1;
    }
    else
    {
        for (i = Display->hdc->multiBuffer; i > 1; --i)
        {
            /* Try setting up multi buffering. */
            Display->hdc->varInfo.yres_virtual = Display->hdc->height * i;

            if (ioctl(Display->hdc->fd, FBIOPUT_VSCREENINFO, &Display->hdc->varInfo) >= 0)
            {
                break;
            }
        }

        /* Get current virtual screen size. */
        ioctl(Display->hdc->fd, FBIOGET_VSCREENINFO, &Display->hdc->varInfo);

        /* Compute number of buffers. Allow Multi Buffer can be disabled by
        ** export FB_MULTI_BUFFER = 1 */
        Display->hdc->multiBuffer = Display->hdc->varInfo.yres_virtual / Display->hdc->height;

        /* Move to off-screen memory. */
        Display->hdc->backBufferY = Display->hdc->varInfo.yoffset + Display->hdc->height;

        if (Display->hdc->backBufferY >= Display->hdc->varInfo.yres_virtual)
        {
            /* Warp around. */
            Display->hdc->backBufferY = 0;
        }

        Info->width  = Display->hdc->varInfo.xres_virtual;
        Info->height = Display->hdc->varInfo.yres_virtual;
    }

    Info->flip = (Display->hdc->multiBuffer > 1);

    /* Success. */
    return gcvTRUE;
#endif
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
    NativeWindowType window = gcvNULL;
    NativeDisplayType display = gcvNULL;

    do
    {
        gctUINT32 offset;
        int handle;

        if (Display == gcvNULL)
        {
            display = veglGetDefaultDisplay();
            if (display == gcvNULL)
            {
                break;
            }
        }
        else
        {
            display = Display;
        }

        if(Width == 0)
        {
            Width = display->width;
        }
        if(Height == 0)
        {
            Height = display->height;
        }

        if(X == -1)
        {
            X = ((display->width - Width) / 2) & ~15;
        }

        if(Y == -1)
        {
            Y = ((display->height - Height) / 2) & ~7;
        }
        if(X < 0) X = 0;
        if(Y < 0) Y = 0;
        if(X + Width > display->width) Width = display->width - X;
        if(Y + Height > display->height) Height = display->height - Y;

        offset = X *((display->bpp + 7) / 8) + Y * display->stride;

        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                       gcmSIZEOF(struct _FBWindow),
                                       (gctPOINTER *) &window)))
        {
            break;
        }

        window->offset = offset;
        window->bpp    = display->bpp;
        window->x      = X;
        window->y      = Y;
        window->width  = Width;
        window->height = Height;
        window->format = display->format;

        handle = open("/sys/class/graphics/fbcon/cursor_blink", O_RDWR);
        if (handle == -1)
        {
            window->cursor = -1;
        }
        else
        {
            if (-1 == read(handle, &window->cursor, 1))
            {
                window->cursor = -1;
            }
            else
            {
                if (-1 == write(handle, "0", 1))
                {
                    window->cursor = -1;
                }
            }
            close(handle);
        }
    }
    while (gcvFALSE);

    if ((Display == gcvNULL) && (display != gcvNULL))
    {
        veglReleaseDefaultDisplay(display);
    }

    return window;
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
    *X      = Window->x;
    *Y      = Window->y;
    *Width  = Window->width;
    *Height = Window->height;
}

void
fbDestroyWindow(
    IN NativeWindowType Window
    )
{
    if (Window != gcvNULL)
    {
        if (Window->cursor != -1)
        {
            int handle = open("/sys/class/graphics/fbcon/cursor_blink", O_RDWR);
            if (handle != -1)
            {
                if (-1 == write(handle, &Window->cursor, 1))
                {
                    Window->cursor = -1;
                }
                close(handle);
            }
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Window));
    }
}

NativePixmapType
fbCreatePixmap(
    IN NativeDisplayType Display,
    IN gctINT Width,
    IN gctINT Height,
	IN gctINT BitsPerPixel
    )
{
    NativePixmapType pixmap = gcvNULL;

    do
    {
        gctINT width, height;

        if ((Width == 0) || (Height == 0))
        {
            return gcvNULL;
        }

        width  = gcmALIGN(Width, 16);
        height = gcmALIGN(Height, 4);

        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
            gcmSIZEOF(struct _FBPixmap),
            (gctPOINTER *) &pixmap)))
        {
            break;
        }

        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
            width * height * (BitsPerPixel + 7) / 8,
            &pixmap->bits)))
        {
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, pixmap));
        }
        else
        {
            pixmap->bpp    = (BitsPerPixel+ 0x07) & (~0x07);
            pixmap->width  = width;
            pixmap->height = height;
            pixmap->stride = width * pixmap->bpp / 8;
        }
    }
    while (gcvFALSE);

    return pixmap;
}

void
fbGetPixmapGeometry(
    IN NativePixmapType Pixmap,
    OUT gctINT_PTR Width,
    OUT gctINT_PTR Height
    )
{
    *Width  = Pixmap->width;
    *Height = Pixmap->height;
}

void
fbDestroyPixmap(
    IN NativePixmapType Pixmap
    )
{
    if (Pixmap != gcvNULL)
    {
        if (Pixmap->bits != gcvNULL)
        {
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Pixmap->bits));
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Pixmap));
    }
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
    if ((Display == NULL) || (Display->hdc == NULL))
    {
        /* Invalid display pointer. */
        return gcvFALSE;
    }

	BackBuffer->origin.x = 0;
    BackBuffer->origin.y = 0;
    BackBuffer->offset = 0;

	if (Display->hdc->multiBuffer <= 1)
	{
	    *Flip = gcvFALSE;
	}

    if (Display->hdc->multiBuffer > 1)
    {
        pthread_mutex_lock(&Display->hdc->condMutex);

        /* Return current back buffer. */
        BackBuffer->origin.y = Display->hdc->backBufferY;

        while (Display->hdc->backBufferY == (volatile int) Display->hdc->varInfo.yoffset)
        {
            pthread_cond_wait(&Display->hdc->cond, &Display->hdc->condMutex);
        }

        /* Move to next back buffer. */
        Display->hdc->backBufferY += Display->hdc->height;
        if (Display->hdc->backBufferY >= Display->hdc->varInfo.yres_virtual)
        {
            /* Wrap around. */
            Display->hdc->backBufferY = 0;
        }

        pthread_mutex_unlock(&Display->hdc->condMutex);

        *Flip = gcvTRUE;
    }

    /* Success. */
    return gcvTRUE;
}

gctBOOL
veglSetDisplayFlip(
    IN NativeDisplayType Display,
	IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer
    )
{
    if (Display == NULL)
    {
        /* Invalid display pointer. */
        return gcvFALSE;
    }

    if (Display->multiBuffer > 1)
    {
        pthread_mutex_lock(&Display->condMutex);

        /* Set display offset. */
        Display->varInfo.xoffset  = BackBuffer->origin.x;
        Display->varInfo.yoffset  = BackBuffer->origin.y;
        Display->varInfo.activate = FB_ACTIVATE_VBL;
        ioctl(Display->fd, FBIOPAN_DISPLAY, &Display->varInfo);

        pthread_cond_broadcast(&Display->cond);
        pthread_mutex_unlock(&Display->condMutex);
    }

    /* Success. */
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
    /* just a simple check for fb mode */
    if ((Display->bpp == 16 &&
        (Config->redSize == 5 && Config->greenSize == 6 && Config->blueSize == 5))
        || (Display->bpp == 32 &&
        (Config->redSize == 8 && Config->greenSize == 8 && Config->blueSize == 8))
        )
    {
        return gcvTRUE;
    }

    return gcvFALSE;
}
