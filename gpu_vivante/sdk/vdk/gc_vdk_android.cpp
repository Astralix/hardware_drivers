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




#include "gc_vdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <binder/IServiceManager.h>

#include <cutils/properties.h>

#include <utils/misc.h>
#include <utils/threads.h>
#include <utils/Atomic.h>
#include <utils/Log.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/DisplayInfo.h>
#include <ui/FramebufferNativeWindow.h>

#include <surfaceflinger/SurfaceComposerClient.h>

#if ANDROID_SDK_VERSION <= 7
#   include <ui/ISurfaceFlingerClient.h>
#elif ANDROID_SDK_VERSION == 8
#   include <surfaceflinger/ISurfaceFlingerClient.h>
#else
#   include <surfaceflinger/ISurfaceComposerClient.h>
#endif

#include <EGL/egl.h>
#include <EGL/eglext.h>

using namespace android;

/*******************************************************************************
** VDK private data structure. *************************************************
*/
struct _vdkPrivate
{
    /* 0 for composition mode(android mode).
     * 1 for framebuffer mode(linux mode). */
    int                       mode;
    sp<SurfaceComposerClient> session;
    android_native_window_t * framebuffer;
    void *                    egl;
    /* Display (vdk only support one display) . */
    struct _Display
    {
        unsigned long      physical;
        void *             memory;
        int                width;
        int                height;
        int                format;
        int                bpp;
        int                size;
        int                stride;
        int                alpha_length;
        int                alpha_offset;
        int                red_length;
        int                red_offset;
        int                green_length;
        int                green_offset;
        int                blue_length;
        int                blue_offset;

        void *             display_id;
    }
    * display;
    /* Window list. */
    struct _Window
    {
        int                x, y;
        int                width;
        int                height;
        int                bpp;

        sp<SurfaceControl> control;
        sp<Surface>        surface;
        struct _Window *   next;
    }
    * window_list;
    /* Pixmap list. */
    struct _Pixmap
    {
        egl_native_pixmap_t * pixmap;
        struct _Pixmap *      next;
    }
    * pixmap_list;
};

/*******************************************************************************
** Default keyboard map. *******************************************************
*/

static vdkPrivate _vdk        = NULL;

static __attribute__((destructor)) void
_onexit(
    void
    )
{
    vdkPrivate vdk = _vdk;
    _vdk = NULL;

    if (vdk == NULL)
    {
        return;
    }

    /* Composition mode. */
    if (vdk->mode == 0)
    {
        while (vdk->window_list != NULL)
        {
            struct _vdkPrivate::_Window * window = vdk->window_list;
            vdk->window_list = window->next;

            window->control.clear();
            window->surface.clear();

            free(window);
        }

        vdk->session->dispose();
    }
    else
    /* Framebuffer mode. */
    {
        if (vdk->window_list != NULL)
        {
            free(vdk->window_list);
            vdk->window_list = NULL;
        }
    }

    free(vdk);
}

/*******************************************************************************
** Initialization. *************************************************************
*/

VDKAPI vdkPrivate VDKLANG
vdkInitialize(
    void
    )
{
    vdkPrivate vdk = NULL;

    /* Return if already initialized. */
    if (_vdk != NULL)
    {
        return _vdk;
    }

    do
    {
        char value[PROPERTY_VALUE_MAX];
        status_t err = NO_ERROR;

        /* Allocate the private data structure. */
        vdk = (vdkPrivate) malloc(sizeof(struct _vdkPrivate));

        if (vdk == NULL)
        {
            /* Break if we are out of memory. */
            break;
        }

        memset(vdk, 0, sizeof (struct _vdkPrivate));

        /* Initialize the private data structure. */
        property_get("vdk.window_mode", value, "0");

        do
        {
            /* Safe mode is framebuffer mode. */
            vdk->mode = 1;

            if (value[0] != '0')
            {
                /* Set framebuffer mode. */
                break;
            }

            /* Try composition mode. */
            sp<IBinder> binder;
            sp<IServiceManager> sm = defaultServiceManager();

            /* 1. Find surfaceflinger service. */
            binder = sm->checkService(String16("SurfaceFlinger"));
            if (binder == 0)
            {
                printf("Try \'Composition\' mode failed: "
                       "SurfaceFlinger not published.\n");

                /* Fail back to framebuffer mode. */
                break;
            }

            /* 2. Create session. */
            vdk->session = new SurfaceComposerClient();

            /* Init check. */
            err = vdk->session->initCheck();

            if (err == NO_INIT)
            {
                /* Composition not started. */
                vdk->session->dispose();
                vdk->session = NULL;


                printf("Try \'Composition\' mode failed: "
                       "Composition not started.\n");

                /* Fail back to framebuffer mode. */
                break;
            }

            /* Set composition mode success. */
            vdk->mode = 0;
        }
        while (0);

        printf("Running in \'%s\' mode\n",
            (vdk->mode == 0 ? "Composition" : "FramebufferNativeWindow"));

        if (vdk->mode == 1)
        {
            /* Get display surface immediately if using mode 1. */
            vdk->framebuffer = (android_native_window_t *) android_createDisplaySurface();
        }

        /* Get libEGL handle. */
        vdk->egl = dlopen("libEGL.so", RTLD_NOW);

        /* Do not need lock. */
        _vdk = vdk;
        atexit(_onexit);

        return vdk;
    }
    while (0);

    if (vdk != NULL)
    {
        free(vdk);
    }

    return NULL;
}

VDKAPI void VDKLANG
vdkExit(
    vdkPrivate Private
    )
{
    if ((Private != NULL) && (Private == _vdk))
    {
        /* _onexit(); */
    }
}

/*******************************************************************************
** Display. ********************************************************************
*/

VDKAPI vdkDisplay VDKLANG
vdkGetDisplay(
    vdkPrivate Private
    )
{
    struct _vdkPrivate::_Display * display;

    if (Private == NULL || Private != _vdk)
    {
        /* Because EGL_DEFAULT_DISPLAY is actually 0. */
        return (vdkDisplay) -1;
    }

    if (Private->display != NULL)
    {
        return (vdkDisplay) Private->display->display_id;
    }

    display = (struct _vdkPrivate::_Display *) malloc(
            sizeof (struct _vdkPrivate::_Display));

    /* Andriod only support one display: 0.*/
    display->display_id = EGL_DEFAULT_DISPLAY;

    /* Composition mode. */
    if (_vdk->mode == 0)
    {
        DisplayInfo info;
        status_t status;

        status = _vdk->session->getDisplayInfo(0, &info);
        if (status != NO_ERROR)
        {
            return NULL;
        }

        display->physical = ~0UL; /* Can not get physical. */
        display->memory   = NULL; /* Can not get logical. */
        display->width    = info.w;
        display->height   = info.h;
        display->format   = info.pixelFormatInfo.format;
    }
    else
    /* Framebuffer mode. */
    {
        android_native_window_t * fb =
            (android_native_window_t *) _vdk->framebuffer;

        display->physical = ~0UL; /* Can not get physical. */
        display->memory   = NULL; /* Can not get logical. */
        fb->query(fb, NATIVE_WINDOW_WIDTH,  &display->width);
        fb->query(fb, NATIVE_WINDOW_HEIGHT, &display->height);
        fb->query(fb, NATIVE_WINDOW_FORMAT, &display->format);
    }

    /* Obtain format information. */
    switch (display->format)
    {
    case PIXEL_FORMAT_RGBA_8888:
        display->bpp          = 32;
        display->size         = display->width * display->height * 4;
        display->stride       = display->width * 4;
        display->alpha_length = 8;
        display->alpha_offset = 24;
        display->red_length   = 8;
        display->red_offset   = 0;
        display->green_length = 8;
        display->green_offset = 8;
        display->blue_length  = 8;
        display->blue_offset  = 16;
        break;

    case PIXEL_FORMAT_RGBX_8888:
        display->bpp          = 32;
        display->size         = display->width * display->height * 4;
        display->stride       = display->width * 4;
        display->alpha_length = 0;
        display->alpha_offset = 24;
        display->red_length   = 8;
        display->red_offset   = 0;
        display->green_length = 8;
        display->green_offset = 8;
        display->blue_length  = 8;
        display->blue_offset  = 16;
        break;

    case PIXEL_FORMAT_RGB_888:
        display->bpp          = 24;
        display->size         = display->width * display->height * 3;
        display->stride       = display->width * 3;
        display->alpha_length = 0;
        display->alpha_offset = 0;
        display->red_length   = 8;
        display->red_offset   = 0;
        display->green_length = 8;
        display->green_offset = 8;
        display->blue_length  = 8;
        display->blue_offset  = 16;
        break;

    case PIXEL_FORMAT_BGRA_8888:
        display->bpp          = 32;
        display->size         = display->width * display->height * 4;
        display->stride       = display->width * 4;
        display->alpha_length = 8;
        display->alpha_offset = 24;
        display->red_length   = 8;
        display->red_offset   = 16;
        display->green_length = 8;
        display->green_offset = 8;
        display->blue_length  = 8;
        display->blue_offset  = 0;
        break;

    case PIXEL_FORMAT_RGB_565:
    default:
        display->bpp          = 16;
        display->size         = display->width * display->height * 2;
        display->stride       = display->width * 2;
        display->alpha_length = 0;
        display->alpha_offset = 0;
        display->red_length   = 5;
        display->red_offset   = 11;
        display->green_length = 6;
        display->green_offset = 5;
        display->blue_length  = 5;
        display->blue_offset  = 0;
        break;
    }

    /* Save display to vdkPrivate. */
    _vdk->display = display;

    return (vdkDisplay) display->display_id;
}

VDKAPI int VDKLANG
vdkGetDisplayInfo(
    vdkDisplay Display,
    int * Width,
    int * Height,
    unsigned long * Physical,
    int * Stride,
    int * BitsPerPixel
    )
{
    struct _vdkPrivate::_Display * display;

    if ((Display != EGL_DEFAULT_DISPLAY)
    ||  (_vdk == NULL)
    ||  (_vdk->display == NULL)
    )
    {
        return 0;
    }

    display = _vdk->display;

    if (Width != NULL)
    {
        *Width  = display->width;
    }

    if (Height != NULL)
    {
        *Height = display->height;
    }

    if (Physical != NULL)
    {
        *Physical = display->physical;
    }

    if (Stride != NULL)
    {
        *Stride = display->stride;
    }

    if (BitsPerPixel != NULL)
    {
        *BitsPerPixel = display->bpp;
    }

    return 1;
}

VDKAPI int VDKLANG
vdkGetDisplayInfoEx(
    vdkDisplay Display,
    unsigned int DisplayInfoSize,
    vdkDISPLAY_INFO * DisplayInfo
    )
{
    /* Valid display? */
    struct _vdkPrivate::_Display * display;

    if ((Display != EGL_DEFAULT_DISPLAY)
    ||  (_vdk == NULL)
    ||  (_vdk->display == NULL)
    )
    {
        return 0;
    }

    /* Valid structure size? */
    if (DisplayInfoSize != sizeof(vdkDISPLAY_INFO))
    {
        /* Invalid structure size. */
        return 0;
    }

    display = _vdk->display;

    /* Return the size of the display. */
    DisplayInfo->width  = display->width;
    DisplayInfo->height = display->height;

    /* Return the stride of the display. */
    DisplayInfo->stride = display->stride;

    /* Return the color depth of the display. */
    DisplayInfo->bitsPerPixel = display->bpp;

    /* Return the logical pointer to the display. */
    DisplayInfo->logical = display->memory;

    /* Return the physical address of the display. */
    DisplayInfo->physical = display->physical;

    /* Return the color info. */
    DisplayInfo->alphaLength = display->alpha_length;
    DisplayInfo->alphaOffset = display->alpha_offset;
    DisplayInfo->redLength   = display->red_length;
    DisplayInfo->redOffset   = display->red_offset;
    DisplayInfo->greenLength = display->green_length;
    DisplayInfo->greenOffset = display->green_offset;
    DisplayInfo->blueLength  = display->blue_length;
    DisplayInfo->blueOffset  = display->blue_offset;

    /* In Android we always flip. */
    DisplayInfo->flip = 1;

    /* Success. */
    return 1;
}

VDKAPI int VDKLANG
vdkGetDisplayVirtual(
    vdkDisplay Display,
    int * Width,
    int * Height
    )
{
    (void) Display;
    (void) Width;
    (void) Height;

    /* Not supported. */
    return 0;
}

VDKAPI int VDKLANG
vdkGetDisplayBackbuffer(
    vdkDisplay Display,
    unsigned int * Offset,
    int * X,
    int * Y
    )
{
    (void) Display;
    (void) Offset;
    (void) X;
    (void) Y;

    /* Not supported. */
    return 0;
}

VDKAPI int VDKLANG
vdkSetDisplayVirtual(
    vdkDisplay Display,
    unsigned int Offset,
    int X,
    int Y
    )
{
    (void) Display;
    (void) Offset;
    (void) X;
    (void) Y;

    /* Not supported. */
    return 0;
}

VDKAPI void VDKLANG
vdkDestroyDisplay(
    vdkDisplay Display
    )
{
    if ((Display != EGL_DEFAULT_DISPLAY)
    ||  (_vdk == NULL)
    ||  (_vdk->display == NULL)
    )
    {
        return;
    }

    free(_vdk->display);
    _vdk->display = NULL;
}

/*******************************************************************************
** Windows. ********************************************************************
*/

VDKAPI vdkWindow VDKLANG
vdkCreateWindow(
    vdkDisplay Display,
    int X,
    int Y,
    int Width,
    int Height
    )
{
    struct _vdkPrivate::_Display * display = NULL;
    struct _vdkPrivate::_Window  * window  = NULL;

    /* Check display. */
    if ((Display != EGL_DEFAULT_DISPLAY)
    ||  (_vdk == NULL)
    ||  (_vdk->display == NULL)
    )
    {
        return NULL;
    }

    display = _vdk->display;

    if (Width == 0)
    {
        Width = display->width;
    }

    if (Height == 0)
    {
        Height = display->height;
    }

    if (X == -1)
    {
        X = ((display->width - Width) / 2) & ~7;
    }

    if (Y == -1)
    {
        Y = ((display->height - Height) / 2) & ~7;
    }

    if (X < 0) X = 0;
    if (Y < 0) Y = 0;
    if (X + Width  > display->width)  Width  = display->width  - X;
    if (Y + Height > display->height) Height = display->height - Y;

    do
    {
        window = (struct _vdkPrivate::_Window *) malloc(
                sizeof (struct _vdkPrivate::_Window));

        if (window == NULL)
        {
            break;
        }

        memset(window, 0, sizeof (struct _vdkPrivate::_Window));

        /* Composition mode. */
        if (_vdk->mode == 0)
        {
            Surface::SurfaceInfo info;

            /* Create surface control. */
#if ANDROID_SDK_VERSION < 14
            window->control = _vdk->session->createSurface(
                    getpid(), 0, Width, Height, display->format);
#else
			window->control = _vdk->session->createSurface(
					0, Width, Height, display->format);
#endif

            /* Set Z order and position. */
            #if ANDROID_SDK_VERSION >= 14
            SurfaceComposerClient::openGlobalTransaction();
            #else
            _vdk->session->openTransaction();
            #endif
            window->control->setLayer(0x40000000);
            window->control->setPosition(X, Y);
            #if ANDROID_SDK_VERSION >= 14
            SurfaceComposerClient::closeGlobalTransaction();
            #else
            _vdk->session->closeTransaction();
            #endif
            /* Get surface. */
            window->surface = window->control->getSurface();

            /* Save parameters. */
            window->x      = X;
            window->y      = Y;

            window->surface->lock(&info, false);
            window->surface->unlockAndPost();

            window->width  = info.w;
            window->height = info.h;
            window->bpp    = display->bpp;

            /* Add to window list. */
            window->next      = _vdk->window_list;
            _vdk->window_list = window;

            return (vdkWindow) (android_native_window_t *) window->surface.get();
        }
        else
        /* Framebuffer mode. */
        {
            if (_vdk->window_list != NULL)
            {
                /* Can only create one window instance in fb mode. */
                break;
            }

            /* Save dummy window info. */
            window->control = NULL;
            window->surface = NULL;
            window->x       = 0;
            window->y       = 0;
            window->width   = display->width;
            window->height  = display->height;
            window->bpp     = display->bpp;
            window->next    = NULL;

            _vdk->window_list = window;

            return (vdkWindow) (android_native_window_t *) _vdk->framebuffer;
        }
    }
    while (0);

    if (window != NULL)
    {
        free(window);
    }

    return NULL;
}

VDKAPI int VDKLANG
vdkGetWindowInfo(
    vdkWindow Window,
    int * X,
    int * Y,
    int * Width,
    int * Height,
    int * BitsPerPixel,
    unsigned int * Offset
    )
{
    struct _vdkPrivate::_Window * window = NULL;

    if ((Window == NULL)
    ||  (_vdk == NULL)
    ||  (_vdk->window_list == NULL)
    )
    {
        return 0;
    }

    window = _vdk->window_list;

    if (_vdk->mode == 0)
    {
        /* Composition mode. */
        while (window != NULL)
        {
            if (Window == (vdkWindow) (android_native_window_t *)
                    window->surface.get())
            {
                break;
            }

            window = window->next;
        }
    }

    if (window != NULL)
    {
        if (X != NULL)
        {
            *X = window->x;
        }

        if (Y != NULL)
        {
            *Y = window->y;
        }

        if (Width != NULL)
        {
            *Width = window->width;
        }

        if (Height != NULL)
        {
            *Height = window->height;
        }

        if (BitsPerPixel != NULL)
        {
            *BitsPerPixel = window->bpp;
        }

        if (Offset != NULL)
        {
            *Offset = 0;
        }

        return 1;
    }

    return 0;
}

VDKAPI void VDKLANG
vdkDestroyWindow(
    vdkWindow Window
    )
{
    struct _vdkPrivate::_Window * window = NULL;

    if ((Window == NULL)
    ||  (_vdk == NULL)
    ||  (_vdk->window_list == NULL)
    )
    {
        return;
    }

    window = _vdk->window_list;

    /* Composition mode. */
    if (_vdk->mode == 0)
    {
        while (window != NULL)
        {
            if (Window == (vdkWindow) (android_native_window_t *)
                    window->surface.get())
            {
                break;
            }

            window = window->next;
        }

        if (window != NULL)
        {
            window->control.clear();
            window->surface.clear();

            if (window == _vdk->window_list)
            {
                _vdk->window_list = window->next;
            }
            else
            {
                struct _vdkPrivate::_Window * last = _vdk->window_list;

                while (last->next != window)
                {
                    last = last->next;
                }

                /* Remove window from list. */
                last->next = window->next;
            }

            free(window);
        }
    }
    else
    /* Framebuffer mode. */
    {
        /* Nothing to do. */
        _vdk->window_list = NULL;
        free(window);
    }
}

VDKAPI int VDKLANG
vdkShowWindow(
    vdkWindow Window
    )
{
    (void) Window;

    /* Not support. */
    return 1;
}

VDKAPI int VDKLANG
vdkHideWindow(
    vdkWindow Window
    )
{
    (void) Window;

    /* Not support. */
    return 1;
}

VDKAPI void VDKLANG
vdkSetWindowTitle(
    vdkWindow Window,
    const char * Title
    )
{
    (void) Window;
    (void) Title;
}

VDKAPI int VDKLANG
vdkDrawImage(
    vdkWindow Window,
    int Left,
    int Top,
    int Right,
    int Bottom,
    int Width,
    int Height,
    int BitsPerPixel,
    void * Bits
    )
{
    return 0;
}

VDKAPI void VDKLANG
vdkCapturePointer(
    vdkWindow Window
    )
{
}

/*******************************************************************************
** Events.
*/

VDKAPI int VDKLANG
vdkGetEvent(
    vdkWindow Window,
    vdkEvent * Event
    )
{
    return 0;
}

/*******************************************************************************
** EGL Support. ****************************************************************
*/

EGL_ADDRESS
vdkGetAddress(
    vdkPrivate Private,
    const char * Function
    )
{
    if ((Private == NULL) || (Private->egl == NULL))
    {
        return NULL;
    }

    return (EGL_ADDRESS) dlsym(Private->egl, Function);
}

/*******************************************************************************
** Time. ***********************************************************************
*/

/*
    vdkGetTicks

    Get the number of milliseconds since the system started.

    PARAMETERS:

        None.

    RETURNS:

        unsigned int
            The number of milliseconds the system has been running.
*/
VDKAPI unsigned int VDKLANG
vdkGetTicks(
    void
    )
{
    struct timeval tv;

    /* Return the time of day in milliseconds. */
    gettimeofday(&tv, 0);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

/*******************************************************************************
** Pixmaps. ********************************************************************
*/


VDKAPI vdkPixmap VDKLANG
vdkCreatePixmap(
    vdkDisplay Display,
    int Width,
    int Height,
    int BitsPerPixel
    )
{
	printf("Warning: Pixmap is not supported on android.\n");
    return NULL;
}

VDKAPI int VDKLANG
vdkGetPixmapInfo(
    vdkPixmap Pixmap,
    int * Width,
    int * Height,
    int * BitsPerPixel,
    int * Stride,
    void ** Bits
    )
{
    return 0;
}

VDKAPI void VDKLANG
vdkDestroyPixmap(
    vdkPixmap Pixmap
    )
{
	printf("Warning: Pixmap is not supported on android.\n");
}

VDKAPI int VDKLANG
vdkDrawPixmap(
    vdkPixmap Pixmap,
    int Left,
    int Top,
    int Right,
    int Bottom,
    int Width,
    int Height,
    int BitsPerPixel,
    void * Bits
    )
{
    return 0;
}

/*******************************************************************************
** ClientBuffers. **************************************************************
*/

/* GL_VIV_direct_texture */
#ifndef GL_VIV_direct_texture
#define GL_VIV_YV12						0x8FC0
#define GL_VIV_NV12						0x8FC1
#define GL_VIV_YUY2						0x8FC2
#define GL_VIV_UYVY						0x8FC3
#define GL_VIV_NV21						0x8FC4
#endif

VDKAPI vdkClientBuffer VDKLANG
vdkCreateClientBuffer(
    int Width,
    int Height,
    int Format,
    int Type
    )
{
    /* Error. */
    return NULL;
}

VDKAPI int VDKLANG
vdkGetClientBufferInfo(
    vdkClientBuffer ClientBuffer,
    int * Width,
    int * Height,
    int * Stride,
    void ** Bits
    )
{
	return 0;
 }

VDKAPI int VDKLANG
vdkDestroyClientBuffer(
    vdkClientBuffer ClientBuffer
    )
{
    return 0;
}

