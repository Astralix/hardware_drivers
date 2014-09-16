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




#include <gc_vdk.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <string.h>
#include <signal.h>
#include "gc_hal_eglplatform.h"

/*******************************************************************************
***** Version Signature *******************************************************/

const char * _VDK_PLATFORM = "\n\0$PLATFORM$X$\n";

struct _vdkPrivate
{
    vdkDisplay  display;
    vdkKeyMap * map;
    void *      egl;
};

static vdkKeyMap keys[] =
{
    /* 00 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 01 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 02 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 03 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 04 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 05 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 06 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 07 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 08 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 09 */ { VDK_ESCAPE,          VDK_UNKNOWN },
    /* 0A */ { VDK_1,               VDK_UNKNOWN },
    /* 0B */ { VDK_2,               VDK_UNKNOWN },
    /* 0C */ { VDK_3,               VDK_UNKNOWN },
    /* 0D */ { VDK_4,               VDK_UNKNOWN },
    /* 0E */ { VDK_5,               VDK_UNKNOWN },
    /* 0F */ { VDK_6,               VDK_UNKNOWN },
    /* 10 */ { VDK_7,               VDK_UNKNOWN },
    /* 11 */ { VDK_8,               VDK_UNKNOWN },
    /* 12 */ { VDK_9,               VDK_UNKNOWN },
    /* 13 */ { VDK_0,               VDK_UNKNOWN },
    /* 14 */ { VDK_HYPHEN,          VDK_UNKNOWN },
    /* 15 */ { VDK_EQUAL,           VDK_UNKNOWN },
    /* 16 */ { VDK_BACKSPACE,       VDK_UNKNOWN },
    /* 17 */ { VDK_TAB,             VDK_UNKNOWN },
    /* 18 */ { VDK_Q,               VDK_UNKNOWN },
    /* 19 */ { VDK_W,               VDK_UNKNOWN },
    /* 1A */ { VDK_E,               VDK_UNKNOWN },
    /* 1B */ { VDK_R,               VDK_UNKNOWN },
    /* 1C */ { VDK_T,               VDK_UNKNOWN },
    /* 1D */ { VDK_Y,               VDK_UNKNOWN },
    /* 1E */ { VDK_U,               VDK_UNKNOWN },
    /* 1F */ { VDK_I,               VDK_UNKNOWN },
    /* 20 */ { VDK_O,               VDK_UNKNOWN },
    /* 21 */ { VDK_P,               VDK_UNKNOWN },
    /* 22 */ { VDK_LBRACKET,        VDK_UNKNOWN },
    /* 23 */ { VDK_RBRACKET,        VDK_UNKNOWN },
    /* 24 */ { VDK_ENTER,           VDK_UNKNOWN },
    /* 25 */ { VDK_LCTRL,           VDK_UNKNOWN },
    /* 26 */ { VDK_A,               VDK_UNKNOWN },
    /* 27 */ { VDK_S,               VDK_UNKNOWN },
    /* 28 */ { VDK_D,               VDK_UNKNOWN },
    /* 29 */ { VDK_F,               VDK_UNKNOWN },
    /* 2A */ { VDK_G,               VDK_UNKNOWN },
    /* 2B */ { VDK_H,               VDK_UNKNOWN },
    /* 2C */ { VDK_J,               VDK_UNKNOWN },
    /* 2D */ { VDK_K,               VDK_UNKNOWN },
    /* 2E */ { VDK_L,               VDK_UNKNOWN },
    /* 2F */ { VDK_SEMICOLON,       VDK_UNKNOWN },
    /* 30 */ { VDK_SINGLEQUOTE,     VDK_UNKNOWN },
    /* 31 */ { VDK_BACKQUOTE,       VDK_UNKNOWN },
    /* 32 */ { VDK_LSHIFT,          VDK_UNKNOWN },
    /* 33 */ { VDK_BACKSLASH,       VDK_UNKNOWN },
    /* 34 */ { VDK_Z,               VDK_UNKNOWN },
    /* 35 */ { VDK_X,               VDK_UNKNOWN },
    /* 36 */ { VDK_C,               VDK_UNKNOWN },
    /* 37 */ { VDK_V,               VDK_UNKNOWN },
    /* 38 */ { VDK_B,               VDK_UNKNOWN },
    /* 39 */ { VDK_N,               VDK_UNKNOWN },
    /* 3A */ { VDK_M,               VDK_UNKNOWN },
    /* 3B */ { VDK_COMMA,           VDK_UNKNOWN },
    /* 3C */ { VDK_PERIOD,          VDK_UNKNOWN },
    /* 3D */ { VDK_SLASH,           VDK_UNKNOWN },
    /* 3E */ { VDK_RSHIFT,          VDK_UNKNOWN },
    /* 3F */ { VDK_PAD_ASTERISK,    VDK_UNKNOWN },
    /* 40 */ { VDK_LALT,            VDK_UNKNOWN },
    /* 41 */ { VDK_SPACE,           VDK_UNKNOWN },
    /* 42 */ { VDK_CAPSLOCK,        VDK_UNKNOWN },
    /* 43 */ { VDK_F1,              VDK_UNKNOWN },
    /* 44 */ { VDK_F2,              VDK_UNKNOWN },
    /* 45 */ { VDK_F3,              VDK_UNKNOWN },
    /* 46 */ { VDK_F4,              VDK_UNKNOWN },
    /* 47 */ { VDK_F5,              VDK_UNKNOWN },
    /* 48 */ { VDK_F6,              VDK_UNKNOWN },
    /* 49 */ { VDK_F7,              VDK_UNKNOWN },
    /* 4A */ { VDK_F8,              VDK_UNKNOWN },
    /* 4B */ { VDK_F9,              VDK_UNKNOWN },
    /* 4C */ { VDK_F10,             VDK_UNKNOWN },
    /* 4D */ { VDK_NUMLOCK,         VDK_UNKNOWN },
    /* 4E */ { VDK_SCROLLLOCK,      VDK_UNKNOWN },
    /* 4F */ { VDK_PAD_7,           VDK_UNKNOWN },
    /* 50 */ { VDK_PAD_8,           VDK_UNKNOWN },
    /* 51 */ { VDK_PAD_9,           VDK_UNKNOWN },
    /* 52 */ { VDK_PAD_HYPHEN,      VDK_UNKNOWN },
    /* 53 */ { VDK_PAD_4,           VDK_UNKNOWN },
    /* 54 */ { VDK_PAD_5,           VDK_UNKNOWN },
    /* 55 */ { VDK_PAD_6,           VDK_UNKNOWN },
    /* 56 */ { VDK_PAD_PLUS,        VDK_UNKNOWN },
    /* 57 */ { VDK_PAD_1,           VDK_UNKNOWN },
    /* 58 */ { VDK_PAD_2,           VDK_UNKNOWN },
    /* 59 */ { VDK_PAD_3,           VDK_UNKNOWN },
    /* 5A */ { VDK_PAD_0,           VDK_UNKNOWN },
    /* 5B */ { VDK_PAD_PERIOD,      VDK_UNKNOWN },
    /* 5C */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 5D */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 5E */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 5F */ { VDK_F11,             VDK_UNKNOWN },
    /* 60 */ { VDK_F12,             VDK_UNKNOWN },
    /* 61 */ { VDK_HOME,            VDK_UNKNOWN },
    /* 62 */ { VDK_UP,              VDK_UNKNOWN },
    /* 63 */ { VDK_PGUP,            VDK_UNKNOWN },
    /* 64 */ { VDK_LEFT,            VDK_UNKNOWN },
    /* 65 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 66 */ { VDK_RIGHT,           VDK_UNKNOWN },
    /* 67 */ { VDK_END,             VDK_UNKNOWN },
    /* 68 */ { VDK_DOWN,            VDK_UNKNOWN },
    /* 69 */ { VDK_PGDN,            VDK_UNKNOWN },
    /* 6A */ { VDK_INSERT,          VDK_UNKNOWN },
    /* 6B */ { VDK_DELETE,          VDK_UNKNOWN },
    /* 6C */ { VDK_PAD_ENTER,       VDK_UNKNOWN },
    /* 6D */ { VDK_RCTRL,           VDK_UNKNOWN },
    /* 6E */ { VDK_BREAK,           VDK_UNKNOWN },
    /* 6F */ { VDK_PRNTSCRN,        VDK_UNKNOWN },
    /* 70 */ { VDK_PAD_SLASH,       VDK_UNKNOWN },
    /* 71 */ { VDK_RALT,            VDK_UNKNOWN },
    /* 72 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 73 */ { VDK_LWINDOW,         VDK_UNKNOWN },
    /* 74 */ { VDK_RWINDOW,         VDK_UNKNOWN },
    /* 75 */ { VDK_MENU,            VDK_UNKNOWN },
    /* 76 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 77 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 78 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 79 */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 7A */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 7B */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 7C */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 7D */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 7E */ { VDK_UNKNOWN,         VDK_UNKNOWN },
    /* 7F */ { VDK_UNKNOWN,         VDK_UNKNOWN },
};

/*******************************************************************************
** Private functions.
*/
static vdkPrivate _vdk     = NULL;

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
** Initialization.
*/

VDKAPI vdkPrivate VDKLANG
vdkInitialize(
    void
    )
{
    vdkPrivate vdk;

    do
    {
        XInitThreads();

        vdk = (vdkPrivate) malloc(sizeof(struct _vdkPrivate));
        if (vdk == NULL)
        {
            break;
        }

        vdk->display = NULL;
        vdk->map     = keys;

#if gcdSTATIC_LINK
        vdk->egl = NULL;
#elif defined(__APPLE__)
        vdk->egl = dlopen("libEGL.dylib", RTLD_NOW);
#else
        vdk->egl = dlopen("libEGL.so", RTLD_NOW);
#endif

#if defined(LINUX)
        signal(SIGINT,  vdkSignalHandler);
        signal(SIGQUIT, vdkSignalHandler);
#endif
        _vdk = vdk;
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
#if defined(LINUX)
    signal(SIGINT,  SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
#endif

    if (Private->egl != NULL)
    {
        dlclose(Private->egl);
    }

    if (Private != NULL)
    {
        free(Private);
        if (_vdk == Private)
            _vdk = NULL;
    }
}

/*******************************************************************************
** Display.
*/

VDKAPI vdkDisplay VDKLANG
vdkGetDisplay(
    vdkPrivate Private
    )
{
    vdkDisplay display;
    if ((Private != NULL) && (Private->display != NULL))
    {
        return Private->display;
    }

    display = (vdkDisplay)gcoOS_GetDisplay();
    if(display != NULL)
        Private->display = display;
    return display;
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
    return gcoOS_GetDisplayInfo((EGLNativeDisplayType)Display, Width, Height, Physical, Stride, BitsPerPixel);
}

VDKAPI int VDKLANG
vdkGetDisplayInfoEx(
    vdkDisplay Display,
    unsigned int DisplayInfoSize,
    vdkDISPLAY_INFO * DisplayInfo
    )
{
    return 0;
}

VDKAPI int VDKLANG
vdkGetDisplayVirtual(
    vdkDisplay Display,
    int * Width,
    int * Height
    )
{
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
    return 0;
}

VDKAPI void VDKLANG
vdkDestroyDisplay(
    vdkDisplay Display
    )
{
    gcoOS_DestroyDisplay((EGLNativeDisplayType)Display);
}

/*******************************************************************************
** Windows
*/

vdkWindow
vdkCreateWindow(
    vdkDisplay Display,
    int X,
    int Y,
    int Width,
    int Height
    )
{
    return (vdkWindow)gcoOS_CreateWindow((EGLNativeDisplayType)Display, X, Y, Width, Height);
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
    if(_vdk == NULL)
        return 0;
    return gcoOS_GetWindowInfo((EGLNativeDisplayType)_vdk->display, (EGLNativeWindowType)Window, X, Y, Width, Height, BitsPerPixel, Offset);
}

VDKAPI void VDKLANG
vdkDestroyWindow(
    vdkWindow Window
    )
{
    if(_vdk != NULL)
        gcoOS_DestroyWindow((EGLNativeDisplayType)_vdk->display, (EGLNativeWindowType)Window);
}

/*
    vdkDrawImage

    Draw a rectangle from a bitmap to the window client area.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow.

        int Left
            Left coordinate of rectangle.

        int Top
            Top coordinate of rectangle.

        int Right
            Right coordinate of rectangle.

        int Bottom
            Bottom coordinate of rectangle.

        int Width
            Width of the specified bitmap.

        int Height
            Height of the specified bitmap.  If height is negative, the bitmap
            is bottom-to-top.

        int BitsPerPixel
            Color depth of the bitmap.

        void * Bits
            Pointer to the bits of the bitmap.

    RETURNS:

        1 if the rectangle has been copied to the window, or 0 if there is an
        error.

*/
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
    if(_vdk == NULL)
        return 0;
    return gcoOS_DrawImage((EGLNativeDisplayType)_vdk->display, (EGLNativeWindowType)Window, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits);
}

VDKAPI int VDKLANG
vdkShowWindow(
    vdkWindow Window
    )
{
    if ((_vdk == NULL) || (_vdk->display == NULL) ||(Window == 0))
    {
        return 0;
    }

    XMapRaised((EGLNativeDisplayType)_vdk->display, (EGLNativeWindowType)Window);

    return 1;
}

VDKAPI int VDKLANG
vdkHideWindow(
    vdkWindow Window
    )
{
    if ((_vdk == NULL) || (_vdk->display == NULL) ||(Window == 0))
    {
        return 0;
    }
    XUnmapWindow((EGLNativeDisplayType)_vdk->display, (EGLNativeWindowType)Window);

    return 1;
}

VDKAPI void VDKLANG
vdkSetWindowTitle(
    vdkWindow Window,
    const char * Title
    )
{
    XTextProperty tp;
    if ((_vdk == NULL) || (_vdk->display == NULL) ||(Window == 0))
    {
        return;
    }
    XStringListToTextProperty((char**) &Title, 1, &tp);

    XSetWMProperties((EGLNativeDisplayType)_vdk->display,
        (EGLNativeWindowType)Window,
        &tp,
        &tp,
        NULL,
        0,
        NULL,
        NULL,
        NULL);
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
    XEvent event;
    vdkKeys scancode;
    vdkPrivate data;

    if (Window == 0)
    {
        return 0;
    }

    data = _vdk;
    if (data == NULL)
    {
        return 0;
    }

    while (XPending((EGLNativeDisplayType)data->display))
    {
        XNextEvent((EGLNativeDisplayType)data->display, &event);

        switch (event.type)
        {
        case MotionNotify:
            Event->type = VDK_POINTER;
            Event->data.pointer.x = event.xmotion.x;
            Event->data.pointer.y = event.xmotion.y;
            return 1;

        case ButtonRelease:
            Event->type = VDK_BUTTON;
            Event->data.button.left   = event.xbutton.state & Button1Mask;
            Event->data.button.middle = event.xbutton.state & Button2Mask;
            Event->data.button.right  = event.xbutton.state & Button3Mask;
            Event->data.button.x      = event.xbutton.x;
            Event->data.button.y      = event.xbutton.y;
            return 1;

        case KeyPress:
        case KeyRelease:
            scancode = data->map[event.xkey.keycode].normal;

            if (scancode == VDK_UNKNOWN)
            {
                break;
            }

            Event->type                   = VDK_KEYBOARD;
            Event->data.keyboard.pressed  = (event.type == KeyPress);
            Event->data.keyboard.scancode = scancode;
            Event->data.keyboard.key      = (  (scancode < VDK_SPACE)
                || (scancode >= VDK_F1)
                )
                ? 0
                : (char) scancode;
            return 1;
        }
    }

#if defined(LINUX)
        if (_terminate)
        {
            _terminate = 0;
            Event->type = VDK_CLOSE;
            return 1;
        }
#endif

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
#if gcdSTATIC_LINK
    return (EGL_ADDRESS) eglGetProcAddress(Function);
#else
    union gcsVARIANT
    {
        void *      ptr;
        EGL_ADDRESS func;
    }
    address;

    if ((Private == NULL) || (Private->egl == NULL))
    {
        return NULL;
    }

    address.ptr = dlsym(Private->egl, Function);

    return address.func;
 #endif
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
    return (vdkPixmap)gcoOS_CreatePixmap((EGLNativeDisplayType)Display, Width, Height, BitsPerPixel);
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
    if (Pixmap == 0)
    {
        return 0;
    }
    if ((_vdk == NULL) || (_vdk->display == NULL))
    {
        return 0;
    }

    return gcoOS_GetPixmapInfo((EGLNativeDisplayType)_vdk->display, (EGLNativePixmapType)Pixmap, Width, Height, BitsPerPixel, Stride, Bits);
}

VDKAPI void VDKLANG
vdkDestroyPixmap(
    vdkPixmap Pixmap
    )
{
    if (Pixmap == 0)
    {
        return;
    }
    if ((_vdk == NULL) || (_vdk->display == NULL))
    {
        return;
    }
    gcoOS_DestroyPixmap((EGLNativeDisplayType)_vdk->display, (EGLNativePixmapType)Pixmap);
}

/*
    vdkDrawPixmap

    Draw a rectangle from a bitmap to the pixmap client area.

    PARAMETERS:

        vdkPixmap Pixmap
            Pointer to the pixmap data structure returned by vdkCreatePixmap.

        int Left
            Left coordinate of rectangle.

        int Top
            Top coordinate of rectangle.

        int Right
            Right coordinate of rectangle.

        int Bottom
            Bottom coordinate of rectangle.

        int Width
            Width of the specified bitmap.

        int Height
            Height of the specified bitmap.  If height is negative, the bitmap
            is bottom-to-top.

        int BitsPerPixel
            Color depth of the bitmap.

        void * Bits
            Pointer to the bits of the bitmap.

    RETURNS:

        1 if the rectangle has been copied to the pixmap, or 0 if there is an
        error.
*/
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
    if (Pixmap == 0)
    {
        return 0;
    }
    if ((_vdk == NULL) || (_vdk->display == NULL))
    {
        return 0;
    }

    return gcoOS_DrawPixmap((EGLNativeDisplayType)_vdk->display,(EGLNativePixmapType)Pixmap, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits);
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

