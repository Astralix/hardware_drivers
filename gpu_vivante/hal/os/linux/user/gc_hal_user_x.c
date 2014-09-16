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


#ifndef VIVANTE_NO_3D

#ifndef EGL_API_FB
#include "gc_hal_user_linux.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <string.h>
#include <signal.h>
#include "gc_hal_eglplatform.h"

#define _GC_OBJ_ZONE    gcvZONE_OS

static halKeyMap keys[] =
{
    /* 00 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 01 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 02 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 03 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 04 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 05 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 06 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 07 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 08 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 09 */ { HAL_ESCAPE,          HAL_UNKNOWN },
    /* 0A */ { HAL_1,               HAL_UNKNOWN },
    /* 0B */ { HAL_2,               HAL_UNKNOWN },
    /* 0C */ { HAL_3,               HAL_UNKNOWN },
    /* 0D */ { HAL_4,               HAL_UNKNOWN },
    /* 0E */ { HAL_5,               HAL_UNKNOWN },
    /* 0F */ { HAL_6,               HAL_UNKNOWN },
    /* 10 */ { HAL_7,               HAL_UNKNOWN },
    /* 11 */ { HAL_8,               HAL_UNKNOWN },
    /* 12 */ { HAL_9,               HAL_UNKNOWN },
    /* 13 */ { HAL_0,               HAL_UNKNOWN },
    /* 14 */ { HAL_HYPHEN,          HAL_UNKNOWN },
    /* 15 */ { HAL_EQUAL,           HAL_UNKNOWN },
    /* 16 */ { HAL_BACKSPACE,       HAL_UNKNOWN },
    /* 17 */ { HAL_TAB,             HAL_UNKNOWN },
    /* 18 */ { HAL_Q,               HAL_UNKNOWN },
    /* 19 */ { HAL_W,               HAL_UNKNOWN },
    /* 1A */ { HAL_E,               HAL_UNKNOWN },
    /* 1B */ { HAL_R,               HAL_UNKNOWN },
    /* 1C */ { HAL_T,               HAL_UNKNOWN },
    /* 1D */ { HAL_Y,               HAL_UNKNOWN },
    /* 1E */ { HAL_U,               HAL_UNKNOWN },
    /* 1F */ { HAL_I,               HAL_UNKNOWN },
    /* 20 */ { HAL_O,               HAL_UNKNOWN },
    /* 21 */ { HAL_P,               HAL_UNKNOWN },
    /* 22 */ { HAL_LBRACKET,        HAL_UNKNOWN },
    /* 23 */ { HAL_RBRACKET,        HAL_UNKNOWN },
    /* 24 */ { HAL_ENTER,           HAL_UNKNOWN },
    /* 25 */ { HAL_LCTRL,           HAL_UNKNOWN },
    /* 26 */ { HAL_A,               HAL_UNKNOWN },
    /* 27 */ { HAL_S,               HAL_UNKNOWN },
    /* 28 */ { HAL_D,               HAL_UNKNOWN },
    /* 29 */ { HAL_F,               HAL_UNKNOWN },
    /* 2A */ { HAL_G,               HAL_UNKNOWN },
    /* 2B */ { HAL_H,               HAL_UNKNOWN },
    /* 2C */ { HAL_J,               HAL_UNKNOWN },
    /* 2D */ { HAL_K,               HAL_UNKNOWN },
    /* 2E */ { HAL_L,               HAL_UNKNOWN },
    /* 2F */ { HAL_SEMICOLON,       HAL_UNKNOWN },
    /* 30 */ { HAL_SINGLEQUOTE,     HAL_UNKNOWN },
    /* 31 */ { HAL_BACKQUOTE,       HAL_UNKNOWN },
    /* 32 */ { HAL_LSHIFT,          HAL_UNKNOWN },
    /* 33 */ { HAL_BACKSLASH,       HAL_UNKNOWN },
    /* 34 */ { HAL_Z,               HAL_UNKNOWN },
    /* 35 */ { HAL_X,               HAL_UNKNOWN },
    /* 36 */ { HAL_C,               HAL_UNKNOWN },
    /* 37 */ { HAL_V,               HAL_UNKNOWN },
    /* 38 */ { HAL_B,               HAL_UNKNOWN },
    /* 39 */ { HAL_N,               HAL_UNKNOWN },
    /* 3A */ { HAL_M,               HAL_UNKNOWN },
    /* 3B */ { HAL_COMMA,           HAL_UNKNOWN },
    /* 3C */ { HAL_PERIOD,          HAL_UNKNOWN },
    /* 3D */ { HAL_SLASH,           HAL_UNKNOWN },
    /* 3E */ { HAL_RSHIFT,          HAL_UNKNOWN },
    /* 3F */ { HAL_PAD_ASTERISK,    HAL_UNKNOWN },
    /* 40 */ { HAL_LALT,            HAL_UNKNOWN },
    /* 41 */ { HAL_SPACE,           HAL_UNKNOWN },
    /* 42 */ { HAL_CAPSLOCK,        HAL_UNKNOWN },
    /* 43 */ { HAL_F1,              HAL_UNKNOWN },
    /* 44 */ { HAL_F2,              HAL_UNKNOWN },
    /* 45 */ { HAL_F3,              HAL_UNKNOWN },
    /* 46 */ { HAL_F4,              HAL_UNKNOWN },
    /* 47 */ { HAL_F5,              HAL_UNKNOWN },
    /* 48 */ { HAL_F6,              HAL_UNKNOWN },
    /* 49 */ { HAL_F7,              HAL_UNKNOWN },
    /* 4A */ { HAL_F8,              HAL_UNKNOWN },
    /* 4B */ { HAL_F9,              HAL_UNKNOWN },
    /* 4C */ { HAL_F10,             HAL_UNKNOWN },
    /* 4D */ { HAL_NUMLOCK,         HAL_UNKNOWN },
    /* 4E */ { HAL_SCROLLLOCK,      HAL_UNKNOWN },
    /* 4F */ { HAL_PAD_7,           HAL_UNKNOWN },
    /* 50 */ { HAL_PAD_8,           HAL_UNKNOWN },
    /* 51 */ { HAL_PAD_9,           HAL_UNKNOWN },
    /* 52 */ { HAL_PAD_HYPHEN,      HAL_UNKNOWN },
    /* 53 */ { HAL_PAD_4,           HAL_UNKNOWN },
    /* 54 */ { HAL_PAD_5,           HAL_UNKNOWN },
    /* 55 */ { HAL_PAD_6,           HAL_UNKNOWN },
    /* 56 */ { HAL_PAD_PLUS,        HAL_UNKNOWN },
    /* 57 */ { HAL_PAD_1,           HAL_UNKNOWN },
    /* 58 */ { HAL_PAD_2,           HAL_UNKNOWN },
    /* 59 */ { HAL_PAD_3,           HAL_UNKNOWN },
    /* 5A */ { HAL_PAD_0,           HAL_UNKNOWN },
    /* 5B */ { HAL_PAD_PERIOD,      HAL_UNKNOWN },
    /* 5C */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 5D */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 5E */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 5F */ { HAL_F11,             HAL_UNKNOWN },
    /* 60 */ { HAL_F12,             HAL_UNKNOWN },
    /* 61 */ { HAL_HOME,            HAL_UNKNOWN },
    /* 62 */ { HAL_UP,              HAL_UNKNOWN },
    /* 63 */ { HAL_PGUP,            HAL_UNKNOWN },
    /* 64 */ { HAL_LEFT,            HAL_UNKNOWN },
    /* 65 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 66 */ { HAL_RIGHT,           HAL_UNKNOWN },
    /* 67 */ { HAL_END,             HAL_UNKNOWN },
    /* 68 */ { HAL_DOWN,            HAL_UNKNOWN },
    /* 69 */ { HAL_PGDN,            HAL_UNKNOWN },
    /* 6A */ { HAL_INSERT,          HAL_UNKNOWN },
    /* 6B */ { HAL_DELETE,          HAL_UNKNOWN },
    /* 6C */ { HAL_PAD_ENTER,       HAL_UNKNOWN },
    /* 6D */ { HAL_RCTRL,           HAL_UNKNOWN },
    /* 6E */ { HAL_BREAK,           HAL_UNKNOWN },
    /* 6F */ { HAL_PRNTSCRN,        HAL_UNKNOWN },
    /* 70 */ { HAL_PAD_SLASH,       HAL_UNKNOWN },
    /* 71 */ { HAL_RALT,            HAL_UNKNOWN },
    /* 72 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 73 */ { HAL_LWINDOW,         HAL_UNKNOWN },
    /* 74 */ { HAL_RWINDOW,         HAL_UNKNOWN },
    /* 75 */ { HAL_MENU,            HAL_UNKNOWN },
    /* 76 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 77 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 78 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 79 */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 7A */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 7B */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 7C */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 7D */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 7E */ { HAL_UNKNOWN,         HAL_UNKNOWN },
    /* 7F */ { HAL_UNKNOWN,         HAL_UNKNOWN },
};

#if defined(LINUX)
static int _terminate = 0;

static void
vdkSignalHandler(
    int SigNum
    )
{
    signal(SIGINT,  vdkSignalHandler);
    signal(SIGQUIT, vdkSignalHandler);

    _terminate = 1;
}
#endif

/*******************************************************************************
** Display.
*/

void
_GetColorBitsInfoFromMask(
    gctSIZE_T Mask,
    gctUINT *Length,
    gctUINT *Offset
)
{
    gctINT start, end;
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
        *Length = end - start;
    }

    if (Offset != NULL)
    {
        *Offset = start;
    }
}

gceSTATUS
gcoOS_GetDisplay(
    OUT HALNativeDisplayType * Display
    )
{
    XImage *image;
    gctINT          screen;
    Visual *        visual;
    gctINT          depth;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();
    do
    {

        *Display = XOpenDisplay(gcvNULL);
        if (*Display == gcvNULL)
        {
            status = gcvSTATUS_OUT_OF_RESOURCES;
        }

        screen = DefaultScreen(*Display);

        visual = DefaultVisual(*Display, screen);


        depth = DefaultDepth(*Display, screen);

        image = XGetImage(*Display,
                          DefaultRootWindow(*Display),
                          0, 0, 1, 1, AllPlanes, ZPixmap);

        if (image == gcvNULL)
        {
            /* Error. */
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }
        XDestroyImage(image);

        gcmFOOTER_ARG("*Display=0x%x", *Display);
        return status;
    }
    while (0);

    if (*Display != gcvNULL)
    {
        XCloseDisplay(*Display);
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_GetDisplayByIndex(
    IN gctINT DisplayIndex,
    OUT HALNativeDisplayType * Display
    )
{
    return gcoOS_GetDisplay(Display);
}

gceSTATUS
gcoOS_GetDisplayInfo(
    IN HALNativeDisplayType Display,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctSIZE_T * Physical,
    OUT gctINT * Stride,
    OUT gctINT * BitsPerPixel
    )
{
    Screen * screen;
    XImage *image;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x", Display);
    if (Display == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    screen = XScreenOfDisplay(Display, DefaultScreen(Display));

    if (Width != gcvNULL)
    {
        *Width = XWidthOfScreen(screen);
    }

    if (Height != gcvNULL)
    {
        *Height = XHeightOfScreen(screen);
    }

    if (Physical != gcvNULL)
    {
        *Physical = ~0;
    }

    if (Stride != gcvNULL)
    {
        *Stride = 0;
    }

    if (BitsPerPixel != gcvNULL)
    {
        image = XGetImage(Display,
            DefaultRootWindow(Display),
            0, 0, 1, 1, AllPlanes, ZPixmap);

        if (image != gcvNULL)
        {
            *BitsPerPixel = image->bits_per_pixel;
            XDestroyImage(image);
        }

    }

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoOS_GetDisplayInfoEx(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctUINT DisplayInfoSize,
    OUT halDISPLAY_INFO * DisplayInfo
    )
{
    Screen * screen;
    XImage *image;

    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Window=0x%x DisplayInfoSize=%u", Display, Window, DisplayInfoSize);
    /* Valid display? and structure size? */
    if ((Display == gcvNULL) || (DisplayInfoSize != sizeof(halDISPLAY_INFO)))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    image = XGetImage(Display,
        DefaultRootWindow(Display),
        0, 0, 1, 1, AllPlanes, ZPixmap);

    if (image == gcvNULL)
    {
        /* Error. */
        status = gcvSTATUS_NOT_SUPPORTED;
        gcmFOOTER();
        return status;
    }

    _GetColorBitsInfoFromMask(image->red_mask, &DisplayInfo->redLength, &DisplayInfo->redOffset);
    _GetColorBitsInfoFromMask(image->green_mask, &DisplayInfo->greenLength, &DisplayInfo->greenOffset);
    _GetColorBitsInfoFromMask(image->blue_mask, &DisplayInfo->blueLength, &DisplayInfo->blueOffset);

    XDestroyImage(image);

    screen = XScreenOfDisplay(Display, DefaultScreen(Display));

    /* Return the size of the display. */
    DisplayInfo->width  = XWidthOfScreen(screen);
    DisplayInfo->height = XHeightOfScreen(screen);

    /* The stride of the display is not known in the X environment. */
    DisplayInfo->stride = ~0;

    DisplayInfo->bitsPerPixel = image->bits_per_pixel;

    /* Get the color info. */
    DisplayInfo->alphaLength = image->bits_per_pixel - image->depth;
    DisplayInfo->alphaOffset = image->depth;

    /* The logical address of the display is not known in the X
    ** environment. */
    DisplayInfo->logical = NULL;

    /* The physical address of the display is not known in the X
    ** environment. */
    DisplayInfo->physical = ~0;

    /* No flip. */
    DisplayInfo->flip = 0;

    /* Success. */
    gcmFOOTER_ARG("*DisplayInfo=0x%x", *DisplayInfo);
    return status;
}

gceSTATUS
gcoOS_GetDisplayVirtual(
    IN HALNativeDisplayType Display,
    OUT gctINT * Width,
    OUT gctINT * Height
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gcoOS_GetDisplayBackbuffer(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctPOINTER    context,
    IN gcoSURF       surface,
    OUT gctUINT * Offset,
    OUT gctINT * X,
    OUT gctINT * Y
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gcoOS_SetDisplayVirtual(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctUINT Offset,
    IN gctINT X,
    IN gctINT Y
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gcoOS_DestroyDisplay(
    IN HALNativeDisplayType Display
    )
{
    if (Display != gcvNULL)
    {
        XCloseDisplay(Display);
    }
    return gcvSTATUS_OK;
}

/*******************************************************************************
** Windows
*/

gceSTATUS
gcoOS_CreateWindow(
    IN HALNativeDisplayType Display,
    IN gctINT X,
    IN gctINT Y,
    IN gctINT Width,
    IN gctINT Height,
    OUT HALNativeWindowType * Window
    )
{
    XSetWindowAttributes attr;
    Screen * screen;
    gctINT width, height;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x X=%d Y=%d Width=%d Height=%d", Display, X, Y, Width, Height);
    /* Test if we have a valid display data structure pointer. */
    if (Display == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
    do
    {
        screen = XScreenOfDisplay(Display, DefaultScreen(Display));
        width  = XWidthOfScreen (screen);
        height = XHeightOfScreen(screen);

        attr.background_pixmap = 0;
        attr.border_pixel      = 0;
        attr.event_mask        = ButtonPressMask
                               | ButtonReleaseMask
                               | KeyPressMask
                               | KeyReleaseMask
                               | ResizeRedirectMask
                               | PointerMotionMask;


        /* Test for zero width. */
        if (Width == 0)
        {
            /* Use display width instead. */
            Width = width;
        }

        /* Test for zero height. */
        if (Height == 0)
        {
            /* Use display height instead. */
            Height = height;
        }

        /* Test for auto-center X coordinate. */
        if (X == -1)
        {
            /* Center the window horizontally. */
            X = (width - Width) / 2;
        }

        /* Test for auto-center Y coordinate. */
        if (Y == -1)
        {
            /* Center the window vertically. */
            Y = (height - Height) / 2;
        }

        /* Clamp coordinates to display. */
        if (X < 0) X = 0;
        if (Y < 0) Y = 0;
        if (X + Width  > width)  Width  = width  - X;
        if (Y + Height > height) Height = height - Y;

        *Window = XCreateWindow(Display,
                               RootWindow(Display,
                                          DefaultScreen(Display)),
                               X, Y,
                               Width, Height,
                               0,
                               DefaultDepth(Display, DefaultScreen(Display)),
                               InputOutput,
                               DefaultVisual(Display, DefaultScreen(Display)),
                               CWBackPixel
                               | CWBorderPixel
                               | CWEventMask,
                               &attr);

        if (!*Window)
        {
            break;
        }

        XMoveWindow(Display, *Window, X, Y);

        gcmFOOTER_ARG("*Window=0x%x", *Window);
        return status;
    }
    while (0);

    if (*Window)
    {
        XUnmapWindow(Display, *Window);
        XDestroyWindow(Display, *Window);
    }

    status = gcvSTATUS_OUT_OF_RESOURCES;
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_GetWindowInfo(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    OUT gctINT * X,
    OUT gctINT * Y,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctINT * BitsPerPixel,
    OUT gctUINT * Offset
    )
{
    XWindowAttributes attr;
    XImage *image;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Window=0x%x", Display, Window);
    if (Window == 0)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    XGetWindowAttributes(Display, Window, &attr);

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

    if (BitsPerPixel != NULL)
    {
        image = XGetImage(Display,
            DefaultRootWindow(Display),
            0, 0, 1, 1, AllPlanes, ZPixmap);

        if (image != NULL)
        {
            *BitsPerPixel = image->bits_per_pixel;
            XDestroyImage(image);
        }
    }

    if (Offset != NULL)
    {
        *Offset = 0;
    }

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoOS_DestroyWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    if((Display == gcvNULL) || (Window == 0))
        return gcvSTATUS_INVALID_ARGUMENT;
    XUnmapWindow(Display, Window);
    XDestroyWindow(Display, Window);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DrawImage(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
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
    GC gc = gcvNULL;
    XImage * image = gcvNULL;
    void * bits = Bits;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Window=0x%x Left=%d Top=%d Right=%d Bottom=%d Width=%d Height=%d BitsPerPixel=%d Bits=0x%x",
                  Display, Window, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits);
    do
    {
        gc = XCreateGC(Display,
            Window,
            0,
            gcvNULL);

        if (gc == gcvNULL)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        if (Height < 0)
        {
            int stride = Width * BitsPerPixel / 8;
            int bitmapSize = -Height * stride;

            char * dest;
            char * src;
            int i;

            Height = -Height;

            bits = malloc(bitmapSize);
            if (bits == gcvNULL)
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            dest = (char *) bits;
            src  = (char *) Bits + stride * (Height - 1);

            for (i = 0; i < Height; i++)
            {
                memcpy(dest, src, stride);

                dest = dest + stride;
                src  = src  - stride;
            }
        }

        image = XCreateImage(Display,
            DefaultVisual(Display, DefaultScreen(Display)),
            DefaultDepth(Display, DefaultScreen(Display)),
            ZPixmap,
            0,
            bits,
            Width,
            Height,
            8,
            Width * BitsPerPixel / 8);

        if (image == gcvNULL)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        if (XPutImage(Display,
            Window,
            gc,
            image,
            Left, Top,
            Left, Top,
            Right - Left, Bottom - Top) != Success)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        XFlush(Display);

    }
    while (0);

    if (image != gcvNULL)
    {
        image->data = gcvNULL;
        XDestroyImage(image);
    }

    if (gc != gcvNULL)
    {
        XFreeGC(Display, gc);
    }

    if ((bits != Bits) && (bits != gcvNULL))
    {
        free(bits);
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_GetImage(
    IN HALNativeWindowType Window,
    IN gctINT Left,
    IN gctINT Top,
    IN gctINT Right,
    IN gctINT Bottom,
    OUT gctINT * BitsPerPixel,
    OUT gctPOINTER * Bits
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

/*******************************************************************************
** Pixmaps. ********************************************************************
*/

gceSTATUS
gcoOS_CreatePixmap(
    IN HALNativeDisplayType Display,
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT BitsPerPixel,
    OUT HALNativePixmapType * Pixmap
    )
{
    /* Get default root window. */
    XImage *image;
    Window rootWindow;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Width=%d Height=%d BitsPerPixel=%d", Display, Width, Height, BitsPerPixel);
    /* Test if we have a valid display data structure pointer. */
    if (Display == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
    if ((Width <= 0) || (Height <= 0))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
    rootWindow = DefaultRootWindow(Display);

    /* Check BitsPerPixel, only support 16bit and 32bit now. */
    switch (BitsPerPixel)
    {
    case 0:
        image = XGetImage(Display,
            rootWindow,
            0, 0, 1, 1, AllPlanes, ZPixmap);

        if (image == gcvNULL)
        {
            /* Error. */
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
        BitsPerPixel = image->bits_per_pixel;
        break;

    case 16:
    case 32:
        break;

    default:
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    do
    {
        /* Create native pixmap, ie X11 Pixmap. */
        *Pixmap = XCreatePixmap(Display,
            rootWindow,
            Width,
            Height,
            BitsPerPixel);

        if (*Pixmap == 0)
        {
            break;
        }

        /* Flush command buffer. */
        XFlush(Display);

        gcmFOOTER_ARG("*Pixmap=0x%x", *Pixmap);
        return status;
    }
    while (0);

    /* Roll back. */
    if (*Pixmap != 0)
    {
        XFreePixmap(Display, *Pixmap);
        XFlush(Display);
    }

    status = gcvSTATUS_OUT_OF_RESOURCES;
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_GetPixmapInfo(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctINT * BitsPerPixel,
    OUT gctINT * Stride,
    OUT gctPOINTER * Bits
    )
{
    Window rootWindow;
    gctINT x, y;
    gctUINT width, height, borderWidth, bitsPerPixel;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Pixmap=0x%x", Display, Pixmap);
    if (Pixmap == 0)
    {
        /* Pixmap is not a valid pixmap data structure pointer. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Query pixmap parameters. */
    if (XGetGeometry(Display,
        Pixmap,
        &rootWindow,
        &x, &y, &width, &height,
        &borderWidth,
        &bitsPerPixel) == False)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Set back values. */
    if (Width != NULL)
    {
        *Width = width;
    }

    if (Height != NULL)
    {
        *Height = height;
    }

    if (BitsPerPixel != NULL)
    {
        *BitsPerPixel = bitsPerPixel;
    }

    /* Can not get stride or Bits from Pixmap. */
    if (((Stride != NULL) && (*Stride != -1)) ||
        ((Bits != NULL) && (*Bits != NULL)))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoOS_DestroyPixmap(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap
    )
{
    if (Pixmap != 0)
    {
        XFreePixmap(Display, Pixmap);
        XFlush(Display);
    }
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_DrawPixmap(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap,
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
    Screen* xScreen;
    Visual* xVisual;
    gctINT     status;
    XImage* xImage = gcvNULL;
    GC gc = NULL;

    Drawable pixmap = Pixmap;
    gceSTATUS Status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Pixmap=0x%x Left=%d Top=%d Right=%d Bottom=%d Width=%d Height=%d BitsPerPixel=%d Bits=0x%x",
        Display, Pixmap, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits);

    do
    {
        /* Create graphics context. */
        gc = XCreateGC(Display, pixmap, 0, gcvNULL);

        /* Fetch defaults. */
        xScreen = DefaultScreenOfDisplay(Display);
        xVisual = DefaultVisualOfScreen(xScreen);

        /* Create image from the bits. */
        xImage = XCreateImage(Display,
            xVisual,
            BitsPerPixel,
            ZPixmap,
            0,
            (char*) Bits,
            Width,
            Height,
            8,
            Width * BitsPerPixel / 8);

        if (xImage == gcvNULL)
        {
            /* Error. */
            Status = gcvSTATUS_OUT_OF_RESOURCES;
            break;
        }

        /* Draw the image. */
        status = XPutImage(Display,
            Pixmap,
            gc,
            xImage,
            Left, Top,           /* Source origin. */
            Left, Top,           /* Destination origin. */
            Right - Left, Bottom - Top);

        if (status != Success)
        {
            /* Error. */
            Status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Flush buffers. */
        status = XFlush(Display);

        if (status != Success)
        {
            /* Error. */
            Status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }
    }
    while (0);

    /* Destroy the image. */
    if (xImage != gcvNULL)
    {
        xImage->data = gcvNULL;
        XDestroyImage(xImage);
    }

    /* Free graphics context. */
    XFreeGC(Display, gc);

    /* Return result. */
    gcmFOOTER();
    return Status;
}

gceSTATUS
gcoOS_LoadEGLLibrary(
    OUT gctHANDLE * Handle
    )
{
    XInitThreads();
#if defined(LINUX)
        signal(SIGINT,  vdkSignalHandler);
        signal(SIGQUIT, vdkSignalHandler);
#endif

#if defined(__APPLE__)
    return gcoOS_LoadLibrary(gcvNULL, "libEGL.dylib", Handle);
#else
    return gcoOS_LoadLibrary(gcvNULL, "libEGL.so", Handle);
#endif
}

gceSTATUS
gcoOS_FreeEGLLibrary(
    IN gctHANDLE Handle
    )
{
    return gcoOS_FreeLibrary(gcvNULL, Handle);
}

gceSTATUS
gcoOS_ShowWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    if((Display == gcvNULL) || (Window == 0))
        return gcvSTATUS_INVALID_ARGUMENT;
    XMapRaised(Display, Window);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_HideWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    if((Display == gcvNULL) || (Window == 0))
        return gcvSTATUS_INVALID_ARGUMENT;
    XUnmapWindow(Display, Window);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_SetWindowTitle(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctCONST_STRING Title
    )
{
    XTextProperty tp;

    if((Display == gcvNULL) || (Window == 0))
        return gcvSTATUS_INVALID_ARGUMENT;

    XStringListToTextProperty((char**) &Title, 1, &tp);

    XSetWMProperties(Display,
        Window,
        &tp,
        &tp,
        gcvNULL,
        0,
        gcvNULL,
        gcvNULL,
        gcvNULL);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_CapturePointer(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gcoOS_GetEvent(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    OUT halEvent * Event
    )
{
    XEvent event;
    halKeys scancode;

    if ((Display == gcvNULL) || (Window == 0) || (Event == gcvNULL))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    while (XPending(Display))
    {
        XNextEvent(Display, &event);

        switch (event.type)
        {
        case MotionNotify:
            Event->type = HAL_POINTER;
            Event->data.pointer.x = event.xmotion.x;
            Event->data.pointer.y = event.xmotion.y;
            return gcvSTATUS_OK;

        case ButtonRelease:
            Event->type = HAL_BUTTON;
            Event->data.button.left   = event.xbutton.state & Button1Mask;
            Event->data.button.middle = event.xbutton.state & Button2Mask;
            Event->data.button.right  = event.xbutton.state & Button3Mask;
            Event->data.button.x      = event.xbutton.x;
            Event->data.button.y      = event.xbutton.y;
            return gcvSTATUS_OK;

        case KeyPress:
        case KeyRelease:
            scancode = keys[event.xkey.keycode].normal;

            if (scancode == HAL_UNKNOWN)
            {
                break;
            }

            Event->type                   = HAL_KEYBOARD;
            Event->data.keyboard.pressed  = (event.type == KeyPress);
            Event->data.keyboard.scancode = scancode;
            Event->data.keyboard.key      = (  (scancode < HAL_SPACE)
                || (scancode >= HAL_F1)
                )
                ? 0
                : (char) scancode;
            return gcvSTATUS_OK;
        }
    }

#if defined(LINUX)
        if (_terminate)
        {
            _terminate = 0;
            Event->type = HAL_CLOSE;
            return gcvSTATUS_OK;
        }
#endif

    return gcvSTATUS_NOT_FOUND;
}

gceSTATUS
gcoOS_CreateClientBuffer(
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT Format,
    IN gctINT Type,
    OUT gctPOINTER * ClientBuffer
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gcoOS_GetClientBufferInfo(
    IN gctPOINTER ClientBuffer,
    OUT gctINT * Width,
    OUT gctINT * Height,
    OUT gctINT * Stride,
    OUT gctPOINTER * Bits
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gcoOS_DestroyClientBuffer(
    IN gctPOINTER ClientBuffer
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
}

#endif
#endif /* VIVANTE_NO_3D */

