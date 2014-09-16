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
#include <windows.h>

#include "gc_hal_eglplatform.h"

#define _GC_OBJ_ZONE    gcvZONE_OS

#ifndef IDI_APPLICATION
#   define IDI_APPLICATION 0
#endif

#ifndef WS_POPUPWINDOW
#   define WS_POPUPWINDOW (WS_POPUP | WS_BORDER | WS_SYSMENU)
#endif

/*******************************************************************************
** ABS *************************************************************************
**
** The ABS macro computes the absolute value of the variable.
*/
#ifndef ABS
#   define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

/*******************************************************************************
** countof *********************************************************************
**
** The countof macro computes the number of elements in an array.
*/
#ifndef countof
#   define countof(a) (sizeof(a) / sizeof(a[0]))
#endif


/*
    _WindowProc

    Callback fuction that processes messages send to a window.

    PARAMETERS:

        HWND Window
            Handle of the window that received a message.

        UINT Message
            Specifies the message.

        WPARAM ParameterW
            Specifies additional message information.

        LPARAM ParameterL
            Specifies additional message information.

    RETURNS:

        LRESULT
            The return value depends on the message.
*/
LRESULT CALLBACK
_WindowProc(
    HWND Window,
    UINT Message,
    WPARAM ParameterW,
    LPARAM ParameterL
    )
{
    /* We do nothing here - just return the default method for the message. */
    return DefWindowProc(Window, Message, ParameterW, ParameterL);
}

/*
    _Error

    Function that display the last known system error in a message box.

    PARAMETERS:

        LPCTSTR Title
            Pointer to the title of the message box.

    RETURNS:

        Nothing.
*/
static void
_Error(
    LPCTSTR Title
    )
{
    LPTSTR buffer;

    /* Get the last known system error and return a formatted message. */
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &buffer,
                  0,
                  NULL);

    /* Pop-up a message box with the error. */
    MessageBox(NULL, buffer, Title, MB_ICONSTOP | MB_OK);

    /* Free the formatted message. */
    LocalFree(buffer);
}

/* Register class. */
static ATOM        regClass;

/*******************************************************************************
** Default keyboard map. *******************************************************
*/
static halKeyMap keys[] =
{
    /* 00 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 01 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 02 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 03 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 04 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 05 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 06 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 07 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 08 */ { HAL_BACKSPACE,       HAL_UNKNOWN     },
    /* 09 */ { HAL_TAB,             HAL_UNKNOWN     },
    /* 0A */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 0B */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 0C */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 0D */ { HAL_ENTER,           HAL_UNKNOWN     },
    /* 0E */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 0F */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 10 */ { HAL_LSHIFT,          HAL_NUMLOCK     },
    /* 11 */ { HAL_LCTRL,           HAL_SCROLLLOCK  },
    /* 12 */ { HAL_LALT,            HAL_UNKNOWN     },
    /* 13 */ { HAL_BREAK,           HAL_UNKNOWN     },
    /* 14 */ { HAL_CAPSLOCK,        HAL_UNKNOWN     },
    /* 15 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 16 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 17 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 18 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 19 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 1A */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 1B */ { HAL_ESCAPE,          HAL_UNKNOWN     },
    /* 1C */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 1D */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 1E */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 1F */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 20 */ { HAL_SPACE,           HAL_LSHIFT      },
    /* 21 */ { HAL_PGUP,            HAL_RSHIFT      },
    /* 22 */ { HAL_PGDN,            HAL_LCTRL       },
    /* 23 */ { HAL_END,             HAL_RCTRL       },
    /* 24 */ { HAL_HOME,            HAL_LALT        },
    /* 25 */ { HAL_LEFT,            HAL_RALT        },
    /* 26 */ { HAL_UP,              HAL_UNKNOWN     },
    /* 27 */ { HAL_RIGHT,           HAL_UNKNOWN     },
    /* 28 */ { HAL_DOWN,            HAL_UNKNOWN     },
    /* 29 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 2A */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 2B */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 2C */ { HAL_PRNTSCRN,        HAL_UNKNOWN     },
    /* 2D */ { HAL_INSERT,          HAL_UNKNOWN     },
    /* 2E */ { HAL_DELETE,          HAL_UNKNOWN     },
    /* 2F */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 30 */ { HAL_0,               HAL_UNKNOWN     },
    /* 31 */ { HAL_1,               HAL_UNKNOWN     },
    /* 32 */ { HAL_2,               HAL_UNKNOWN     },
    /* 33 */ { HAL_3,               HAL_UNKNOWN     },
    /* 34 */ { HAL_4,               HAL_UNKNOWN     },
    /* 35 */ { HAL_5,               HAL_UNKNOWN     },
    /* 36 */ { HAL_6,               HAL_UNKNOWN     },
    /* 37 */ { HAL_7,               HAL_UNKNOWN     },
    /* 38 */ { HAL_8,               HAL_UNKNOWN     },
    /* 39 */ { HAL_9,               HAL_UNKNOWN     },
    /* 3A */ { HAL_UNKNOWN,         HAL_SEMICOLON   },
    /* 3B */ { HAL_UNKNOWN,         HAL_EQUAL       },
    /* 3C */ { HAL_UNKNOWN,         HAL_COMMA       },
    /* 3D */ { HAL_UNKNOWN,         HAL_HYPHEN      },
    /* 3E */ { HAL_UNKNOWN,         HAL_PERIOD      },
    /* 3F */ { HAL_UNKNOWN,         HAL_SLASH       },
    /* 40 */ { HAL_UNKNOWN,         HAL_BACKQUOTE   },
    /* 41 */ { HAL_A,               HAL_UNKNOWN     },
    /* 42 */ { HAL_B,               HAL_UNKNOWN     },
    /* 43 */ { HAL_C,               HAL_UNKNOWN     },
    /* 44 */ { HAL_D,               HAL_UNKNOWN     },
    /* 45 */ { HAL_E,               HAL_UNKNOWN     },
    /* 46 */ { HAL_F,               HAL_UNKNOWN     },
    /* 47 */ { HAL_G,               HAL_UNKNOWN     },
    /* 48 */ { HAL_H,               HAL_UNKNOWN     },
    /* 49 */ { HAL_I,               HAL_UNKNOWN     },
    /* 4A */ { HAL_J,               HAL_UNKNOWN     },
    /* 4B */ { HAL_K,               HAL_UNKNOWN     },
    /* 4C */ { HAL_L,               HAL_UNKNOWN     },
    /* 4D */ { HAL_M,               HAL_UNKNOWN     },
    /* 4E */ { HAL_N,               HAL_UNKNOWN     },
    /* 4F */ { HAL_O,               HAL_UNKNOWN     },
    /* 50 */ { HAL_P,               HAL_UNKNOWN     },
    /* 51 */ { HAL_Q,               HAL_UNKNOWN     },
    /* 52 */ { HAL_R,               HAL_UNKNOWN     },
    /* 53 */ { HAL_S,               HAL_UNKNOWN     },
    /* 54 */ { HAL_T,               HAL_UNKNOWN     },
    /* 55 */ { HAL_U,               HAL_UNKNOWN     },
    /* 56 */ { HAL_V,               HAL_UNKNOWN     },
    /* 57 */ { HAL_W,               HAL_UNKNOWN     },
    /* 58 */ { HAL_X,               HAL_UNKNOWN     },
    /* 59 */ { HAL_Y,               HAL_UNKNOWN     },
    /* 5A */ { HAL_Z,               HAL_UNKNOWN     },
    /* 5B */ { HAL_LWINDOW,         HAL_LBRACKET    },
    /* 5C */ { HAL_RWINDOW,         HAL_BACKSLASH   },
    /* 5D */ { HAL_MENU,            HAL_RBRACKET    },
    /* 5E */ { HAL_UNKNOWN,         HAL_SINGLEQUOTE },
    /* 5F */ { HAL_SLEEP,           HAL_UNKNOWN     },
    /* 60 */ { HAL_PAD_0,           HAL_UNKNOWN     },
    /* 61 */ { HAL_PAD_1,           HAL_UNKNOWN     },
    /* 62 */ { HAL_PAD_2,           HAL_UNKNOWN     },
    /* 63 */ { HAL_PAD_3,           HAL_UNKNOWN     },
    /* 64 */ { HAL_PAD_4,           HAL_UNKNOWN     },
    /* 65 */ { HAL_PAD_5,           HAL_UNKNOWN     },
    /* 66 */ { HAL_PAD_6,           HAL_UNKNOWN     },
    /* 67 */ { HAL_PAD_7,           HAL_UNKNOWN     },
    /* 68 */ { HAL_PAD_8,           HAL_UNKNOWN     },
    /* 69 */ { HAL_PAD_9,           HAL_UNKNOWN     },
    /* 6A */ { HAL_PAD_ASTERISK,    HAL_UNKNOWN     },
    /* 6B */ { HAL_PAD_PLUS,        HAL_UNKNOWN     },
    /* 6C */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6D */ { HAL_PAD_HYPHEN,      HAL_UNKNOWN     },
    /* 6E */ { HAL_PAD_PERIOD,      HAL_UNKNOWN     },
    /* 6F */ { HAL_PAD_SLASH,       HAL_UNKNOWN     },
    /* 70 */ { HAL_F1,              HAL_UNKNOWN     },
    /* 71 */ { HAL_F2,              HAL_UNKNOWN     },
    /* 72 */ { HAL_F3,              HAL_UNKNOWN     },
    /* 73 */ { HAL_F4,              HAL_UNKNOWN     },
    /* 74 */ { HAL_F5,              HAL_UNKNOWN     },
    /* 75 */ { HAL_F6,              HAL_UNKNOWN     },
    /* 76 */ { HAL_F7,              HAL_UNKNOWN     },
    /* 77 */ { HAL_F8,              HAL_UNKNOWN     },
    /* 78 */ { HAL_F9,              HAL_UNKNOWN     },
    /* 79 */ { HAL_F10,             HAL_UNKNOWN     },
    /* 7A */ { HAL_F11,             HAL_UNKNOWN     },
    /* 7B */ { HAL_F12,             HAL_UNKNOWN     },
    /* 7C */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7D */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7E */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7F */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
};

/*******************************************************************************
** Display. ********************************************************************
*/


gceSTATUS
gcoOS_GetDisplay(
    OUT HALNativeDisplayType * Display
    )
{
    gctINT bpp;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();
    do
    {

        /* Create the device context. */
        *Display = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

        if (*Display == gcvNULL)
        {
            /* Break if the device context could not be created. */
            _Error(TEXT("CreateDC"));
            status = gcvSTATUS_OUT_OF_RESOURCES;
            break;
        }

        /* Get the display size and color depth. */
        bpp    = GetDeviceCaps(*Display, BITSPIXEL);
        if((bpp != 16) && (bpp != 32))
        {
            _Error(TEXT("Unsupported color depth."));
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

        /* Return the pointer to the display data structure. */
        gcmFOOTER_ARG("*Display=0x%x", *Display);
        return status;
    }
    while (0);

    /* Roll back on error. */
    if (*Display != gcvNULL)
    {
        DeleteDC(*Display);
    }

    /* Error. */
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
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x", Display);
    if (Display == gcvNULL)
    {
        /* Display is not a valid display data structure pointer. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Width != gcvNULL)
    {
        /* Return the width of the display. */
        *Width = GetDeviceCaps(Display, HORZRES);
    }

    if (Height != gcvNULL)
    {
        /* Return the height of the display. */
        *Height = GetDeviceCaps(Display, VERTRES);
    }

    if (Physical != gcvNULL)
    {
        /* The physical address of the display is not known in the Windows
        ** environment. */
        *Physical = ~0U;
    }

    if (Stride != gcvNULL)
    {
        /* The stride of the display is not known in the Windows environment. */
        *Stride = -1;
    }

    if (BitsPerPixel != gcvNULL)
    {
        /* Return the color depth of the display. */
        *BitsPerPixel = GetDeviceCaps(Display, BITSPIXEL);
    }

    /* Success. */
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
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Window=0x%x DisplayInfoSize=%u", Display, Window, DisplayInfoSize);
    /* Valid display? and structure size? */
    if ((Display == gcvNULL) || (DisplayInfoSize != sizeof(halDISPLAY_INFO)))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Return the size of the display. */
    DisplayInfo->width  = GetDeviceCaps(Display, HORZRES);
    DisplayInfo->height = GetDeviceCaps(Display, VERTRES);

    /* The stride of the display is not known in the Windows environment. */
    DisplayInfo->stride = -1;

    /* Return the color depth of the display. */
    DisplayInfo->bitsPerPixel = GetDeviceCaps(Display, BITSPIXEL);

    /* The logical address of the display is not known in the Windows
    ** environment. */
    DisplayInfo->logical = gcvNULL;

    /* The physical address of the display is not known in the Windows
    ** environment. */
    DisplayInfo->physical = ~0U;

    /* No flip. */
    DisplayInfo->flip = 0;

    switch (DisplayInfo->bitsPerPixel)
    {
    case 16:
        /* 16-bits per pixel. */
        DisplayInfo->alphaLength = 1;
        DisplayInfo->alphaOffset = 15;

        DisplayInfo->redLength   = 5;
        DisplayInfo->redOffset   = 10;

        DisplayInfo->greenLength = 5;
        DisplayInfo->greenOffset = 5;

        DisplayInfo->blueLength  = 5;
        DisplayInfo->blueOffset  = 0;
        break;

    case 32:
        /* 32-bits per pixel. */
        DisplayInfo->alphaLength = 8;
        DisplayInfo->alphaOffset = 24;

        DisplayInfo->redLength   = 8;
        DisplayInfo->redOffset   = 16;

        DisplayInfo->greenLength = 8;
        DisplayInfo->greenOffset = 8;

        DisplayInfo->blueLength  = 8;
        DisplayInfo->blueOffset  = 0;
        break;

    default:
        _Error(TEXT("Unsupported color depth."));
        status = gcvSTATUS_NOT_SUPPORTED;
        gcmFOOTER();
        return status;
    }

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
    /* Only process if we have a valid pointer. */
    if (Display != NULL)
    {
        /* Delete the device context. */
        DeleteDC(Display);
    }
    return gcvSTATUS_OK;
}

/*******************************************************************************
** Windows. ********************************************************************
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
    gctINT displayWidth, displayHeight;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x X=%d Y=%d Width=%d Height=%d", Display, X, Y, Width, Height);
    /* Test if we have a valid display data structure pointer. */
    if (Display == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
    displayWidth = GetDeviceCaps(Display, HORZRES);
    displayHeight = GetDeviceCaps(Display, VERTRES);

    /* Test for zero width. */
    if (Width == 0)
    {
        /* Use display width instead. */
        Width = displayWidth;
    }
    else
    {
        /* Clamp width to display width. */
        Width = min(Width, displayWidth);
    }

    /* Test for zero height. */
    if (Height == 0)
    {
        /* Use display height instead. */
        Height = displayHeight;
    }
    else
    {
        /* Clamp height to display height. */
        Height = min(Height, displayHeight);
    }

    /* Test for auto-center X coordinate. */
    if (X == -1)
    {
        /* Center the window horizontally. */
        X = (displayWidth - Width) / 2;
    }

    /* Test for auto-center Y coordinate. */
    if (Y == -1)
    {
        /* Center the window vertically. */
        Y = (displayHeight - Height) / 2;
    }

    /* Clamp coordinates to display. */
    if (X < 0) X = 0;
    if (Y < 0) Y = 0;
    if (X + Width  > displayWidth)  Width  = displayWidth  - X;
    if (Y + Height > displayHeight) Height = displayHeight - Y;

    do
    {
        /* Window rectangle. */
        RECT rect;

        /* Window style. */
        UINT style = WS_POPUPWINDOW | WS_CAPTION;
        UINT extra = 0;

        SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, FALSE);
        if (Y + Height > rect.bottom)
        {
            extra = WS_EX_TOPMOST;
        }

        /* Set the window rectangle. */
        rect.left   = X;
        rect.top    = Y;
        rect.right  = X + Width;
        rect.bottom = Y + Height;

        /* Adjust the window rectangle for the style. */
        AdjustWindowRectEx(&rect, style, FALSE, extra);

        /* Create the window. */
        *Window = CreateWindowEx(extra,
                                TEXT("halClass"),
                                TEXT("vdkApp"),
                                style,
                                rect.left,
                                rect.top,
                                rect.right - rect.left,
                                rect.bottom - rect.top,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

        if (*Window == gcvNULL)
        {
            /* Break on bad window. */
            _Error(TEXT("CreateWindow"));
            break;
        }

        /* Return pointer to the window data structure. */
        gcmFOOTER_ARG("*Window=0x%x", *Window);
        return status;
    }
    while (0);

    if(*Window != gcvNULL)
        DestroyWindow(*Window);
    /* Error. */
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
    RECT rect;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Window=0x%x", Display, Window);
    if (Window == gcvNULL)
    {
        /* Window is not a valid window data structure pointer. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Get client rectangle of the window. */
    if (IsWindow((HWND) Window))
    {
        GetClientRect((HWND) Window, &rect);
    }
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (X != gcvNULL)
    {
        /* Return the x coordinate. */
        *X = rect.left;
    }

    if (Y != gcvNULL)
    {
        /* Return the y coordinate. */
        *Y = rect.top;
    }

    if (Width != gcvNULL)
    {
        /* Return the window width. */
        *Width = rect.right - rect.left;
    }

    if (Height != gcvNULL)
    {
        /* Return the window height. */
        *Height = rect.bottom - rect.top;
    }

    if (BitsPerPixel != gcvNULL)
    {
        /* Return the window color depth. */
        if (IsWindow((HWND) Window))
        {
            HDC dc = GetDC((HWND) Window);
            *BitsPerPixel = GetDeviceCaps(dc, BITSPIXEL);
            ReleaseDC((HWND) Window, dc);
        }
        else
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
    }

    if (Offset != gcvNULL)
    {
        /* Offset is not known in the Windows environment. */
        *Offset = ~0U;
    }

    /* Success. */
    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoOS_DestroyWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    /* Only process if we have a valid pointer */
    if (Window != gcvNULL)
    {
        /* Destroy the window. */
        DestroyWindow(Window);
    }
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
    /* Bitmap header. */
    BITMAPINFO info;

    /* Device context. */
    HWND window;
    HDC dc;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Window=0x%x Left=%d Top=%d Right=%d Bottom=%d Width=%d Height=%d BitsPerPixel=%d Bits=0x%x",
                  Display, Window, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits);
    /* Test for valid Window and Bits pointers. */
    if ((Window == gcvNULL) || (Bits == gcvNULL))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Fill in the bitmap header. */
    info.bmiHeader.biSize          = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth         = Width;
    info.bmiHeader.biHeight        = - Height;
    info.bmiHeader.biPlanes        = 1;
    info.bmiHeader.biBitCount      = (WORD) BitsPerPixel;
    info.bmiHeader.biCompression   = BI_RGB;
    info.bmiHeader.biSizeImage     = ((Width * BitsPerPixel / 8 + 3) & ~3)
                                   * ABS(Height);
    info.bmiHeader.biXPelsPerMeter = 0;
    info.bmiHeader.biYPelsPerMeter = 0;
    info.bmiHeader.biClrUsed       = 0;
    info.bmiHeader.biClrImportant  = 0;

    /* Get the device context for the window. */
    if (IsWindow((HWND) Window))
    {
        window = (HWND) Window;
    }
    else
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
    dc = GetDC(window);

    /* Copy the bits. */
    StretchDIBits(dc,
                  Left, Top, Right - Left, Bottom - Top,
                  Left, Top, Right - Left, Bottom - Top,
                  Bits,
                  &info,
                  DIB_RGB_COLORS,
                  SRCCOPY);

    /* Release the bits. */
    ReleaseDC(window, dc);

    /* Success. */
    gcmFOOTER_NO();
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
    /* Function return value. */
    gceSTATUS status = gcvSTATUS_OK;

    /* Window. */
    HWND window = gcvNULL;

    /* Device contexts. */
    HDC dc  = gcvNULL;
    HDC cdc = gcvNULL;

    /* Bitmap to hold the image. */
    HBITMAP bitmap = gcvNULL;
    HBITMAP oldBitmap = gcvNULL;
    gcmHEADER_ARG("Window=0x%x Left=%d Top=%d Right=%d Bottom=%d", Window, Left, Top, Right, Bottom);
    do
    {
        /* Bitmap header. */
        BITMAPINFO info;

        /* Bitmap bits. */
        gctPOINTER bits;

        /* Deermine the size of the area to get. */
        gctINT width  = Right  - Left;
        gctINT height = Bottom - Top;

        /* Test for valid Window and Bits pointers. */
        if ((Window == gcvNULL) || (Bits == gcvNULL))
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Get window handle. */
        if (IsWindow((HWND) Window))
        {
            window = (HWND) Window;
        }
        else
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Get the device context for the window. */
        dc = GetDC(window);

        if (dc == gcvNULL)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Create a compatible device context. */
        cdc = CreateCompatibleDC(dc);

        if (cdc == NULL)
        {
            status = gcvSTATUS_OUT_OF_RESOURCES;
            break;
        }

        /* Fill in the bitmap structure. */
        info.bmiHeader.biSize          = sizeof(info);
        info.bmiHeader.biWidth         = width;
        info.bmiHeader.biHeight        = height;
        info.bmiHeader.biPlanes        = 1;
        info.bmiHeader.biBitCount      = 32;
        info.bmiHeader.biCompression   = BI_RGB;
        info.bmiHeader.biSizeImage     = width * height * 4;
        info.bmiHeader.biXPelsPerMeter = 0;
        info.bmiHeader.biYPelsPerMeter = 0;
        info.bmiHeader.biClrUsed       = 0;
        info.bmiHeader.biClrImportant  = 0;

        /* Create DIB section. */
        bitmap = CreateDIBSection(cdc, &info, DIB_RGB_COLORS, &bits, gcvNULL, 0);

        if (bitmap == gcvNULL)
        {
            status = gcvSTATUS_OUT_OF_RESOURCES;
            break;
        }

        /* Select the bitmap into the compatible context and
           save the old bitmap. */
        oldBitmap = (HBITMAP) SelectObject(cdc, bitmap);

        /* Bitblt from the window's context to the compatible context. */
        BitBlt(
            /* Destination context. */
            cdc,
            /* Destination rectangle (x, y, w, h). */
            0, 0, width, height,
            /* Source context. */
            dc,
            /* Source origin. */
            Left, Top,
            /* Operation. */
            SRCCOPY
            );

        /* Allocate bits. */
        *Bits = malloc(info.bmiHeader.biSizeImage);

        if (*Bits == gcvNULL)
        {
            status = gcvSTATUS_OUT_OF_MEMORY;
            break;
        }

        /* Copy the bitmap. */
        memcpy(*Bits, bits, info.bmiHeader.biSizeImage);

        /* Set bits per pixel. */
        * BitsPerPixel = info.bmiHeader.biBitCount;

    }
    while (FALSE);

    /* Release resources. */
    if (bitmap != gcvNULL)
    {
        SelectObject(cdc, oldBitmap);
        DeleteObject(bitmap);
    }

    if (cdc != gcvNULL)
    {
        DeleteDC(cdc);
    }

    if (dc != gcvNULL)
    {
        ReleaseDC(window, dc);
    }

    /* Return result. */
    gcmFOOTER();
    return status;
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
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Width=%d Height=%d BitsPerPixel=%d", Display, Width, Height, BitsPerPixel);
    /* Test if we have a valid display data structure pointer. */
    if (Display == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
    if ((Width <= 0) || (Height <= 0) || (BitsPerPixel <= 0))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    do
    {
        BITMAPINFO info;
        gctPOINTER bits;

        /* See if we need to get the default number of bits per pixel. */
        if (BitsPerPixel == 0)
        {
            BitsPerPixel = GetDeviceCaps(Display, BITSPIXEL);
        }

        /* Fill in the bitmap info structure. */
        info.bmiHeader.biSize          = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth         = Width;
        info.bmiHeader.biHeight        = -Height;
        info.bmiHeader.biPlanes        = 1;
        info.bmiHeader.biBitCount      = (WORD) BitsPerPixel;
        info.bmiHeader.biCompression   = BI_RGB;
        info.bmiHeader.biSizeImage     = 0;
        info.bmiHeader.biXPelsPerMeter = 0;
        info.bmiHeader.biYPelsPerMeter = 0;
        info.bmiHeader.biClrUsed       = 0;
        info.bmiHeader.biClrImportant  = 0;

        /* Create the bitmap. */
        *Pixmap = CreateDIBSection(Display,
            &info,
            DIB_RGB_COLORS,
            &bits,
            gcvNULL,
            0);

        if (*Pixmap == gcvNULL)
        {
            /* Break on bad bitmap. */
            _Error(TEXT("CreateDIBSection"));
            break;
        }

        /* Return pointer to the pixmap data structure. */
        gcmFOOTER_ARG("*Pixmap=0x%x", *Pixmap);
        return status;
    }
    while (0);

    /* Roll back on error. */
    if (*Pixmap != gcvNULL)
    {
        DeleteObject(*Pixmap);
    }

    /* Error. */
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
    DIBSECTION bitmap;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Pixmap=0x%x", Display, Pixmap);
    if (Pixmap == gcvNULL)
    {
        /* Pixmap is not a valid pixmap data structure pointer. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    /* Get the pixmap information. */
    if (GetObject(Pixmap, sizeof(bitmap), &bitmap) == 0)
    {
        _Error(TEXT("GetObject"));
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Width != gcvNULL)
    {
        /* Return the pixmap width. */
        *Width = bitmap.dsBm.bmWidth;
    }

    if (Height != gcvNULL)
    {
        /* Return the pixmap height. */
        *Height = bitmap.dsBm.bmHeight;
    }

    if (BitsPerPixel != gcvNULL)
    {
        /* Return the pixmap color depth. */
        *BitsPerPixel = bitmap.dsBm.bmBitsPixel;
    }

    if (Stride != gcvNULL)
    {
        /* Return the pixmap stride. */
        *Stride = bitmap.dsBm.bmWidthBytes;
    }

    if (Bits != gcvNULL)
    {
        /* Return the pixmap bits. */
        *Bits = bitmap.dsBm.bmBits;
    }

    /* Success. */
    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoOS_DestroyPixmap(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap
    )
{
    /* Only process if we have a valid pointer */
    if (Pixmap != gcvNULL)
    {
        /* Destroy the pixmap. */
        DeleteObject(Pixmap);
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
    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gcoOS_LoadEGLLibrary(
                     OUT gctHANDLE * Handle
                     )
{
    WNDCLASS class;
    gceSTATUS status = gcvSTATUS_OK;
    /* Initialize the WNDCLASS structure. */
    ZeroMemory(&class, sizeof(class));
    class.lpfnWndProc   = _WindowProc;
    class.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    class.hCursor       = LoadCursor(NULL, IDC_CROSS);
    class.hbrBackground = GetStockObject(WHITE_BRUSH);
    class.lpszClassName = TEXT("halClass");

    /* Register the window class. */
    regClass = RegisterClass(&class);

    if (regClass == 0)
    {
        /* RegisterClass failed. */
        _Error(TEXT("RegisterClass"));
        status = gcvSTATUS_NOT_SUPPORTED;
        return status;
    }
    return gcoOS_LoadLibrary(gcvNULL, "libEGL.dll", Handle);
}

gceSTATUS
gcoOS_FreeEGLLibrary(
    IN gctHANDLE Handle
    )
{
    if(regClass != 0)
    {
        /* Unregister the window class. */
        UnregisterClass(TEXT("halClass"), gcvNULL);
    }
    return gcoOS_FreeLibrary(gcvNULL, Handle);
}

gceSTATUS
gcoOS_ShowWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    HWND window;

    if (Window == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Get the window handle. */
    if (IsWindow((HWND) Window))
    {
        window = (HWND) Window;
    }
    else
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    /* Show the window. */
    ShowWindow(window, SW_SHOWNORMAL);

    /* Initial paint of the window. */
    UpdateWindow(window);

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_HideWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    HWND window;

    if (Window == NULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Get the window handle. */
    if (IsWindow((HWND) Window))
    {
        window = (HWND) Window;
    }
    else
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

    /* Hide the window. */
    ShowWindow(window, SW_HIDE);
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_SetWindowTitle(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctCONST_STRING Title
    )
{
    HWND window;
#ifdef UNICODE
    /* Temporary buffer. */
    LPTSTR title;

    /* Number of characters required for the temporary buffer. */
    gctINT count;
#endif

    if (IsWindow((HWND) Window))
    {
        window = (HWND) Window;
    }
    else
    {
        return gcvSTATUS_INVALID_OBJECT;
    }

#ifdef UNICODE
    /* Query number of characters required for the temporary buffer. */
    count = MultiByteToWideChar(CP_ACP,
                                MB_PRECOMPOSED,
                                Title, -1,
                                NULL, 0);

    /* Allocate the temporary buffer. */
    title = (LPTSTR) malloc(count * sizeof(TCHAR));

    /* Only process if the allocation succeeded. */
    if (title != gcvNULL)
    {
        /* Convert the title into UNICODE. */
        MultiByteToWideChar(CP_ACP,
                            MB_PRECOMPOSED,
                            Title, -1,
                            title, count);

        /* Set the window title. */
        SetWindowText(window, title);

        /* Free the temporary buffer. */
        free(title);
    }
#else
    /* Set the window title. */
    SetWindowText(window, Title);
#endif
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_CapturePointer(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    if (Window == gcvNULL)
    {
        ReleaseCapture();
        ClipCursor(gcvNULL);
    }
    else
    {
        RECT rect;
        POINT ul, br;
        HWND window;

        if (IsWindow((HWND) Window))
        {
            window = (HWND) Window;
        }
        else
        {
            return gcvSTATUS_INVALID_OBJECT;
        }

        GetClientRect(window, &rect);

        ul.x = rect.left;
        ul.y = rect.right;
        ClientToScreen(window, &ul);

        br.x = rect.right  + 1;
        br.y = rect.bottom + 1;
        ClientToScreen(window, &br);

        SetRect(&rect, ul.x, ul.y, br.x, br.y);

        SetCapture(window);
        ClipCursor(&rect);
    }
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_GetEvent(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    OUT halEvent * Event
    )
{
    /* Message. */
    MSG msg;

    /* Translated scancode. */
    halKeys scancode;

    /* Test for valid Window and Event pointers. */
    if ((Window == gcvNULL) || (Event == gcvNULL))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Loop while there are messages in the queue for the window. */
    while (PeekMessage(&msg, (HALNativeWindowType)Window, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
            /* Keyboard event. */
            Event->type = HAL_KEYBOARD;

            /* Translate the scancode. */
            scancode = (msg.wParam & 0x80)
                     ? keys[msg.wParam & 0x7F].extended
                     : keys[msg.wParam & 0x7F].normal;

            /* Set scancode. */
            Event->data.keyboard.scancode = scancode;

            /* Set ASCII key. */
            Event->data.keyboard.key = (  (scancode < HAL_SPACE)
                                       || (scancode >= HAL_F1)
                                       )
                                       ? 0
                                       : (char) scancode;

            /* Set up or down flag. */
            Event->data.keyboard.pressed = (msg.message == WM_KEYDOWN);

            /* Valid event. */
            return gcvSTATUS_OK;

        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT:
            /* Application should close. */
            Event->type = HAL_CLOSE;

            /* Valid event. */
            return gcvSTATUS_OK;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            /* Button event. */
            Event->type = HAL_BUTTON;

            /* Set button states. */
            Event->data.button.left   = (msg.wParam & MK_LBUTTON) ? 1 : 0;
            Event->data.button.middle = (msg.wParam & MK_MBUTTON) ? 1 : 0;
            Event->data.button.right  = (msg.wParam & MK_RBUTTON) ? 1 : 0;
            Event->data.button.x      = LOWORD(msg.lParam);
            Event->data.button.y      = HIWORD(msg.lParam);

            /* Valid event. */
            return gcvSTATUS_OK;

        case WM_MOUSEMOVE:
            /* Pointer event.*/
            Event->type = HAL_POINTER;

            /* Set pointer location. */
            Event->data.pointer.x = LOWORD(msg.lParam);
            Event->data.pointer.y = HIWORD(msg.lParam);

            /* Valid event. */
            return gcvSTATUS_OK;

        default:
            /* Translate and dispatch message. */
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
    }

    /* Test if the window is still valid. */
    if (!IsWindow((HALNativeWindowType)Window))
    {
        /* Application should close. */
        Event->type = HAL_CLOSE;

        /* Valid event. */
        return gcvSTATUS_OK;
    }

    /* No event pending. */
    return gcvSTATUS_NOT_FOUND;
}

#define gcdSUPPORT_EXTERNAL_IMAGE_EXT 1

/* GL_VIV_direct_texture */
#ifndef GL_VIV_direct_texture
#define GL_VIV_YV12						0x8FC0
#define GL_VIV_NV12						0x8FC1
#define GL_VIV_YUY2						0x8FC2
#define GL_VIV_UYVY						0x8FC3
#define GL_VIV_NV21						0x8FC4
#endif

gceSTATUS
gcoOS_CreateClientBuffer(
    IN gctINT Width,
    IN gctINT Height,
    IN gctINT Format,
    IN gctINT Type,
    OUT gctPOINTER * ClientBuffer
    )
{
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;
#if gcdSUPPORT_EXTERNAL_IMAGE_EXT
    do
    {
        gcoSURF surf;
        gceSURF_FORMAT format;

        switch (Format)
        {
        case GL_VIV_YUY2:
            format = gcvSURF_YUY2;
            break;
        case GL_VIV_UYVY:
            format = gcvSURF_UYVY;
            break;
        case GL_VIV_NV12:
            format = gcvSURF_NV12;
            break;
        case GL_VIV_NV21:
            format = gcvSURF_NV21;
            break;
        case GL_VIV_YV12:
            format = gcvSURF_YV12;
            break;
        default:
            return gcvSTATUS_INVALID_ARGUMENT;
        }
        status = gcoSURF_Construct(gcvNULL,
                          Width,
                          Height,
                          1,
                          gcvSURF_BITMAP,
                          format,
                          gcvPOOL_SYSTEM,
                          &surf);
        if(status != gcvSTATUS_OK)
        {
            break;
        }
        *ClientBuffer = (gctPOINTER)surf;
        /* Return pointer to the pixmap data structure. */
        status = gcvSTATUS_OK;
    }
    while (0);
#endif

    /* Error. */
    return status;
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
    gceSTATUS status = gcvSTATUS_NOT_SUPPORTED;
#if gcdSUPPORT_EXTERNAL_IMAGE_EXT
    status = gcvSTATUS_INVALID_ARGUMENT;
    if (gcoSURF_IsValid((gcoSURF)ClientBuffer) != gcvSTATUS_TRUE)
    {
        return status;
    }

    if (Width || Height)
    {
        status = gcoSURF_GetSize((gcoSURF)ClientBuffer,
                                 (unsigned int*)Width,
                                 (unsigned int*)Height,
                                 gcvNULL);

        if (status != gcvSTATUS_OK)
        {
            return status;
        }
    }

    if (Stride)
    {
        status = gcoSURF_GetAlignedSize((gcoSURF)ClientBuffer,
                                         gcvNULL,
                                         gcvNULL,
                                         Stride);

        if (status != gcvSTATUS_OK)
        {
            return status;
        }
    }

    if (Bits)
    {
        gctPOINTER memory[3];

        status = gcoSURF_Lock((gcoSURF)ClientBuffer,
                              gcvNULL,
                              memory);

        if (status != gcvSTATUS_OK)
        {
            return status;
        }

        *Bits = memory[0];

    }

    return status;
#else
    return status;
#endif
}

gceSTATUS
gcoOS_DestroyClientBuffer(
    IN gctPOINTER ClientBuffer
    )
{
#if gcdSUPPORT_EXTERNAL_IMAGE_EXT
    return gcoSURF_Destroy((gcoSURF)ClientBuffer);
#else
    return gcvSTATUS_NOT_SUPPORTED;
#endif
}
#endif /* VIVANTE_NO_3D */
