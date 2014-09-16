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



#include <stdlib.h>
#include "gc_vdk.h"
#include <gc_hal.h>
#include "gc_hal_eglplatform.h"


/*******************************************************************************
** VDK private data structure. *************************************************
*/
struct _vdkPrivate
{
    /* Pointer to current display. */
    vdkDisplay  display;

    /* Handle of EGL library. */
    void*     egl;
};
static vdkPrivate _vdk     = NULL;

/*******************************************************************************
** Initialization. *************************************************************
*/

/*
    vdkInitialize

    Initialize the VDK.

    PARAMETERS:

        None.

    RETURNS:

        vdkPrivate
            Pointer to the private data structure that holds the variables for
            the VDK.  If there is an error during initialization, NULL is
            returned.
*/
VDKAPI vdkPrivate VDKLANG
vdkInitialize(
    void
    )
{
    vdkPrivate vdk;

    do
    {
        /* Allocate the private data structure. */
        vdk = (vdkPrivate) malloc(sizeof(struct _vdkPrivate));

        if (vdk == NULL)
        {
            /* Break if we are out of memory. */
            break;
        }
        _vdk = vdk;
        /* Zero display pointer. */
        vdk->display = NULL;

#if gcdSTATIC_LINK
        vdk->egl = NULL;
#else
        if(gcoOS_LoadEGLLibrary(&(vdk->egl)) != gcvSTATUS_OK)
            break;
#endif

        /* Return the poinetr to the private data structure. */
        return vdk;
    }
    while (0);

    /* Roll back on error. */
    if (vdk != NULL)
    {
        /* Free the private data structure. */
        free(vdk);
        vdk = NULL;
        _vdk = NULL;
    }

    /* Error. */
    return NULL;
}

/*
    vdkExit

    Terminate the VDK.  All allocated resources will be released.

    PARAMETERS:

        vdkPrivate Private
            Pointer to the private data structure returned by vdkInitialize.

    RETURNS:

        Nothing.
*/
VDKAPI void VDKLANG
vdkExit(
    vdkPrivate Private
    )
{
    /* Only process if we have a valid pointer. */
    if (Private != NULL)
    {
        if(_vdk == Private)
            _vdk = NULL;
        if (Private->egl != NULL)
        {
            /* Free the libEGL library. */
            gcoOS_FreeEGLLibrary(Private->egl);
        }

        /* Free the pivate data structure. */
        free(Private);
    }
}

/*******************************************************************************
** Display. ********************************************************************
*/


/*
    vdkGetDisplay

    Get the pointer to a structure that holds information about the display.

    PARAMETERS:

        vdkPrivate Private
            Pointer to the private data structure returned by vdkInitialize.

    RETURNS:

        vdkDisplay
            Pointer to the display data structure holding or NULL if there is an
            error.
*/
VDKAPI vdkDisplay VDKLANG
vdkGetDisplay(
    vdkPrivate Private
    )
{
    HALNativeDisplayType display = NULL;

    /* If we have a valid private data structure and the display is already
    ** allocated, we can just return the current display pointer. */
    if ((Private != NULL) && (Private->display != NULL))
    {
        /* Return the current display pointer. */
        return Private->display;
    }
    gcoOS_GetDisplay(&display);
    if(display != NULL)
        Private->display = (vdkDisplay)display;
    return (vdkDisplay)display;
}

/*
    vdkGetDisplayInfo

    Get information about the current display.

    PARAMETERS:

        vdkDisplay Display
            Pointer to the display data structure returned by vdkGetDisplay.

        int * Width
            Pointer to a variable that will receive the width of the display.
            If Width is NULL, the width of the display will not be returned.

        int * Height
            Pointer to a variable that will receive the height of the display.
            If Height is NULL, the height of the display will not be returned.

        unsigned long * Physical
            Pointer to a variable that will receive the physical address of the
            display.  If the physical address is not known, 0xFFFFFFFF will be
            returned.  If Physical is NULL, the physical address of the display
            will not be returned.

        int * Stride
            Pointer to a variable that will receive the stride of the display.
            If the stride is not known, 0xFFFFFFFF will be returned.  If Stride
            is NULL, the stride of the display will not be returned.

        int * BitsPerPixel
            Pointer to a variable that will receive the color depth of the
            display.  If BitsPerPixel is NULL, the color depth of the display
            will not be returned.

    RETURNS:

        int
            1 if Display points to a valid display data structure or 0 if
            Display is not pointing to a valid display data structure.
*/
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
    if(gcoOS_GetDisplayInfo((HALNativeDisplayType)Display, Width, Height, Physical, Stride, BitsPerPixel) == gcvSTATUS_OK)
        return 1;
    return 0;
}

/*
    vdkGetDisplayInfoEx

    Get information about the current display.

    PARAMETERS:

        vdkDisplay Display
            Pointer to the display data structure returned by vdkGetDisplay.

        unsigned int DisplayInfoSize
            The size of the structure pointed to by the DisplayInfo parameter.

        vdkDISPLAY_INFO * DisplayInfo
            Pointer to the display information structure which will receive
            information about the specified display.

    RETURNS:

        int
            1 if Display points to a valid display data structure or 0 if
            Display is not pointing to a valid display data structure.
*/
VDKAPI int VDKLANG
vdkGetDisplayInfoEx(
    vdkDisplay Display,
    unsigned int DisplayInfoSize,
    vdkDISPLAY_INFO * DisplayInfo
    )
{
    /* Success. */
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
vdkGetDisplayBackbufferCount(
    vdkDisplay Display,
    unsigned int * Count
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

/*
    vdkDestroyDisplay

    Destroy the display data structure.  All resources will be released.

    PARAMETERS:

        vdkDisplay Display
            Pointer to the display data structure returned by vdkGetDisplay.

    RETURNS:

        Nothing.
*/
VDKAPI void VDKLANG
vdkDestroyDisplay(
    vdkDisplay Display
    )
{
    gcoOS_DestroyDisplay((HALNativeDisplayType)Display);
}

/*******************************************************************************
** Windows. ********************************************************************
*/

/*
    vdkCreateWindow

    Create a window.  The window can be full screen (optionally up to a
    specified size), fixed on a certain dispay location, or centered on the
    display.

    PARAMETERS:

        vdkDisplay Display
            Pointer to the display data structure returned by vdkGetDisplay.

        int X
            X coordinate of the window.  If X is -1, the window will be centered
            on the display horizontally.

        int Y
            Y coordinate of the window.  If Y is -1, the window will be centered
            on the display vertically.

        int Width
            Width of the window.  If Width is 0, a window will be created with
            the width of the display.  The width is always clamped to the width
            of the display.

        int Height
            Height of the window.  If Height is 0, a window will be created with
            the height of the display.  The height is always clamped to the
            height of the display.

    RETURNS:

        vdkWindow
            Pointer to the window data structure or NULL if there is an error.
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
    HALNativeWindowType Window = 0;
    gcoOS_CreateWindow((HALNativeDisplayType)Display, X, Y, Width, Height, &Window);
    return (vdkWindow)Window;
}

/*
    vdkGetWindowInfo

    Get information about a window.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow.

        int * X
            Pointer to a variable that will receive the x coordinate of the
            window.  If X is NULL, the x coordinate of the window will not be
            returned.

        int * Y
            Pointer to a variable that will receive the y coordinate of the
            window.  If Y is NULL, the y coordinate of the window will not be
            returned.

        int * Width
            Pointer to a variable that will receive the width of the window.  If
            Width is NULL, the width of the window will not be returned.

        int * Height
            Pointer to a variable that will receive the height of the window.
            If Height is NULL, the height of the window will not be returned.

        int * BitsPerPixel
            Pointer to a variable that will receive the color depth of the
            window.  If BitsPerPixel is NULL, the color depth of the window
            will not be returned.

        unsigned int * Offset
            Pointer to a variable that will receive the offset into the display
            memory of teh top-left pixel of the window.  If the offset is not
            known, 0xFFFFFFFF will be returned.  If Offset is NULL, the offset
            will not be returned.

    RETURNS:

        int
            1 if Window points to a valid window data structure or 0 if Window
            is not pointing to a valid window data structure.
*/
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
    if(gcoOS_GetWindowInfo((HALNativeDisplayType)_vdk->display,
                           (HALNativeWindowType)Window, X, Y, Width, Height,
                           BitsPerPixel,
#ifdef __QNXNTO__
                           gcvNULL,
#endif
                           Offset) == gcvSTATUS_OK)
        return 1;
    return 0;
}

/*
    vdkDestroyWindow

    Destroy the window data structure.  All resources will be released.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow.

    RETURNS:

        Nothing.
*/
VDKAPI void VDKLANG
vdkDestroyWindow(
    vdkWindow Window
    )
{
    gcoOS_DestroyWindow((HALNativeDisplayType)_vdk->display, (HALNativeWindowType)Window);
}

/*
    vdkShowWindow

    Show a window on the display.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow.

    RETURNS:

        int
            1 on success, 0 on failure.
*/
VDKAPI int VDKLANG
vdkShowWindow(
    vdkWindow Window
    )
{
    if(gcoOS_ShowWindow((HALNativeDisplayType)_vdk->display, (HALNativeWindowType)Window) != gcvSTATUS_OK)
        return 0;
    return 1;
}

/*
    vdkHideWindow

    Remove a window from the display.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow.

    RETURNS:

        int
            1 on success, 0 on failure.
*/
VDKAPI int VDKLANG
vdkHideWindow(
    vdkWindow Window
    )
{
    if(gcoOS_HideWindow((HALNativeDisplayType)_vdk->display, (HALNativeWindowType)Window) != gcvSTATUS_OK)
        return 0;
    return 1;
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
    if(gcoOS_DrawImage((HALNativeDisplayType)_vdk->display, (HALNativeWindowType)Window, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits) == gcvSTATUS_OK)
        return 1;
    return 0;
}

VDKAPI int VDKLANG
vdkGetImage(
    vdkWindow Window,
    int Left,
    int Top,
    int Right,
    int Bottom,
    int * BitsPerPixel,
    void ** Bits
    )
{
    if(gcoOS_GetImage((HALNativeWindowType)Window, Left, Top, Right, Bottom, BitsPerPixel, Bits) == gcvSTATUS_OK)
        return 1;
    return 0;
}

/*
    vdkSetWindowTitle

    Set the title of the window.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow.

        const char * Title
            Pointer to a string containing the title.

    RETURNS:

        Nothing.
*/
VDKAPI void VDKLANG
vdkSetWindowTitle(
    vdkWindow Window,
    const char * Title
    )
{
    gcoOS_SetWindowTitle((HALNativeDisplayType)_vdk->display, (HALNativeWindowType)Window, Title);
}

/*
    vdkCapturePointer

    Capture or release the pointer.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow if
            the pointer needs to be captured or NULL if the captured pointer
            needs to be released.

    RETURNS:

        Nothing.
*/
VDKAPI void VDKLANG
vdkCapturePointer(
    vdkWindow Window
    )
{
    gcoOS_CapturePointer((HALNativeDisplayType)_vdk->display, (HALNativeWindowType)Window);
}

/*******************************************************************************
** Events. *********************************************************************
*/

/*
    vdkGetEvent

    Get a pending event for the specified window.

    PARAMETERS:

        vdkWindow Window
            Pointer to the window data structure returned by vdkCreateWindow.

        vdkEvent * Event
            Pointer to an event structure that will receive the event, if an
            event if pending.

    RETURNS:

        int
            1 if an event is pending or 0 if there is no event pending.
*/
VDKAPI int VDKLANG
vdkGetEvent(
    vdkWindow Window,
    vdkEvent * Event
    )
{
    halEvent halEvent;
    int result = 0;
    if(gcoOS_GetEvent((HALNativeDisplayType)_vdk->display, (HALNativeWindowType)Window, &halEvent) == gcvSTATUS_OK)
    {
        result = 1;
        switch(halEvent.type)
        {
        case HAL_KEYBOARD:
            Event->type = halEvent.type;
            Event->data.keyboard.scancode = halEvent.data.keyboard.scancode;
            Event->data.keyboard.pressed  = halEvent.data.keyboard.pressed;
            Event->data.keyboard.key      = halEvent.data.keyboard.key;
            break;
        case HAL_BUTTON:
            Event->type = halEvent.type;
            Event->data.button.left     = halEvent.data.button.left;
            Event->data.button.right    = halEvent.data.button.right;
            Event->data.button.middle   = halEvent.data.button.middle;
            Event->data.button.x        = halEvent.data.button.x;
            Event->data.button.y        = halEvent.data.button.y;
            break;
        case HAL_POINTER:
            Event->type = halEvent.type;
            Event->data.pointer.x = halEvent.data.pointer.x;
            Event->data.pointer.y = halEvent.data.pointer.y;
            break;
        case HAL_CLOSE:
            Event->type = halEvent.type;
            break;
        case HAL_WINDOW_UPDATE:
            Event->type = halEvent.type;
            break;
        default:
            return 0;
        }
    }
    return result;
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
    EGL_ADDRESS func = NULL;
    /* Make sure we have a valid pointer and that the EGL library has been
    ** loaded. */
    if ((Private == NULL) || (Private->egl == NULL))
    {
        return func;
    }
    gcoOS_GetProcAddress(gcvNULL, Private->egl, Function, (void*)(&func));
    return func;
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
    /* Return the OS tick count. */
    return gcoOS_GetTicks();
}

/*******************************************************************************
** Pixmaps. ********************************************************************
*/

/*
    vdkCreatePixmap

    Create a pixmap.

    PARAMETERS:

        vdkDisplay Display
            Pointer to the display data structure returned by vdkGetDisplay.

        int Width
            Width of the pixmap.

        int Height
            Height of the pixmap.

        int BitsPerPixel
            Number of bits per pixel.  If BitsPerPixel is zero, the pixmap will
            be created with the same number of bits per pixel as the display.

    RETURNS:

        vdkPixmap
            Pointer to the pixmap data structure or NULL if there is an error.
*/
VDKAPI vdkPixmap VDKLANG
vdkCreatePixmap(
    vdkDisplay Display,
    int Width,
    int Height,
    int BitsPerPixel
    )
{
    HALNativePixmapType Pixmap = 0;
    gcoOS_CreatePixmap((HALNativeDisplayType)Display, Width, Height, BitsPerPixel, &Pixmap);
    return (vdkPixmap)Pixmap;
}

/*
    vdkGetPixmapInfo

    Get information about a pixmap.

    PARAMETERS:

        vdkPixmap Pixmap
            Pointer to the pixmap data structure returned by vdkCreatePixmap.

        int * Width
            Pointer to a variable that will receive the width of the pixmap.  If
            Width is NULL, the width of the pixmap will not be returned.

        int * Height
            Pointer to a variable that will receive the height of the pixmap.
            If Height is NULL, the height of the pixmap will not be returned.

        int * BitsPerPixel
            Pointer to a variable that will receive the color depth of the
            pixmap.  If BitsPerPixel is NULL, the color depth of the pixmap
            will not be returned.

        int * Stride
            Pointer to a variable that will receive the stride of the pixmap.
            If Stride is NULL, the stride of the pixmap will not be returned.

        void ** Bits
            Pointer to a variable that will receive the bits of the pixmap.
            If Bits is NULL, the bits of the pixmap will not be returned.

RETURNS:

        int
            1 if Pixmap points to a valid pixmap data structure or 0 if Pixmap
            is not pointing to a valid pixmap data structure.
*/
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
    if(gcoOS_GetPixmapInfo((HALNativeDisplayType)_vdk->display, (HALNativePixmapType)Pixmap, Width, Height, BitsPerPixel, Stride, Bits) == gcvSTATUS_OK)
        return 1;
    return 0;
}

/*
    vdkDestroyPixmap

    Destroy the pixmap data structure.  All resources will be released.

    PARAMETERS:

        vdkPixmap Pixmap
            Pointer to the pixmap data structure returned by vdkCreatePixmap.

    RETURNS:

        Nothing.
*/
VDKAPI void VDKLANG
vdkDestroyPixmap(
    vdkPixmap Pixmap
    )
{
    gcoOS_DestroyPixmap((HALNativeDisplayType)_vdk->display, (HALNativePixmapType)Pixmap);
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
    if(gcoOS_DrawPixmap((HALNativeDisplayType)_vdk->display,
        (HALNativePixmapType)Pixmap,
        Left, Top, Right, Bottom,
        Width, Height, BitsPerPixel, Bits) != gcvSTATUS_OK)
        return 0;
    return 1;
}

/*******************************************************************************
** ClientBuffers. **************************************************************
*/

VDKAPI vdkClientBuffer VDKLANG
vdkCreateClientBuffer(
    int Width,
    int Height,
    int Format,
    int Type
    )
{
   vdkClientBuffer ClientBuffer = gcvNULL;
   gcoOS_CreateClientBuffer(Width, Height, Format, Type, (gctPOINTER*)(&ClientBuffer));
   return ClientBuffer;
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
    if(gcoOS_GetClientBufferInfo((gctPOINTER)ClientBuffer, Width, Height, Stride, Bits) ==gcvSTATUS_OK)
        return 1;
    return 0;
}

VDKAPI int VDKLANG
vdkDestroyClientBuffer(
    vdkClientBuffer ClientBuffer
    )
{
    if(gcoOS_DestroyClientBuffer((gctPOINTER)ClientBuffer) == gcvSTATUS_OK)
        return 1;
    return 0;
}

