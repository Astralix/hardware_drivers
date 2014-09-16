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




#include <windows.h>
#ifdef UNDER_CE
#   include <aygshell.h>
#endif
#include "gc_vdk.h"
#include <gc_hal.h>
#include "gc_hal_eglplatform.h"

/*******************************************************************************
***** Version Signature *******************************************************/

const char * _VDK_PLATFORM = "\n\0$PLATFORM$GDI$\n";

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

/*******************************************************************************
** VDK private data structure. *************************************************
*/
struct _vdkPrivate
{
    /* Register class. */
    ATOM        class;

    /* Pointer to current display. */
    vdkDisplay  display;

    /* Handle of EGL library. */
    HMODULE     egl;
};

/*******************************************************************************
** Default keyboard map. *******************************************************
*/
static vdkKeyMap keys[] =
{
    /* 00 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 01 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 02 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 03 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 04 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 05 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 06 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 07 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 08 */ { VDK_BACKSPACE,       VDK_UNKNOWN     },
    /* 09 */ { VDK_TAB,             VDK_UNKNOWN     },
    /* 0A */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 0B */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 0C */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 0D */ { VDK_ENTER,           VDK_UNKNOWN     },
    /* 0E */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 0F */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 10 */ { VDK_LSHIFT,          VDK_NUMLOCK     },
    /* 11 */ { VDK_LCTRL,           VDK_SCROLLLOCK  },
    /* 12 */ { VDK_LALT,            VDK_UNKNOWN     },
    /* 13 */ { VDK_BREAK,           VDK_UNKNOWN     },
    /* 14 */ { VDK_CAPSLOCK,        VDK_UNKNOWN     },
    /* 15 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 16 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 17 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 18 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 19 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 1A */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 1B */ { VDK_ESCAPE,          VDK_UNKNOWN     },
    /* 1C */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 1D */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 1E */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 1F */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 20 */ { VDK_SPACE,           VDK_LSHIFT      },
    /* 21 */ { VDK_PGUP,            VDK_RSHIFT      },
    /* 22 */ { VDK_PGDN,            VDK_LCTRL       },
    /* 23 */ { VDK_END,             VDK_RCTRL       },
    /* 24 */ { VDK_HOME,            VDK_LALT        },
    /* 25 */ { VDK_LEFT,            VDK_RALT        },
    /* 26 */ { VDK_UP,              VDK_UNKNOWN     },
    /* 27 */ { VDK_RIGHT,           VDK_UNKNOWN     },
    /* 28 */ { VDK_DOWN,            VDK_UNKNOWN     },
    /* 29 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 2A */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 2B */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 2C */ { VDK_PRNTSCRN,        VDK_UNKNOWN     },
    /* 2D */ { VDK_INSERT,          VDK_UNKNOWN     },
    /* 2E */ { VDK_DELETE,          VDK_UNKNOWN     },
    /* 2F */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 30 */ { VDK_0,               VDK_UNKNOWN     },
    /* 31 */ { VDK_1,               VDK_UNKNOWN     },
    /* 32 */ { VDK_2,               VDK_UNKNOWN     },
    /* 33 */ { VDK_3,               VDK_UNKNOWN     },
    /* 34 */ { VDK_4,               VDK_UNKNOWN     },
    /* 35 */ { VDK_5,               VDK_UNKNOWN     },
    /* 36 */ { VDK_6,               VDK_UNKNOWN     },
    /* 37 */ { VDK_7,               VDK_UNKNOWN     },
    /* 38 */ { VDK_8,               VDK_UNKNOWN     },
    /* 39 */ { VDK_9,               VDK_UNKNOWN     },
    /* 3A */ { VDK_UNKNOWN,         VDK_SEMICOLON   },
    /* 3B */ { VDK_UNKNOWN,         VDK_EQUAL       },
    /* 3C */ { VDK_UNKNOWN,         VDK_COMMA       },
    /* 3D */ { VDK_UNKNOWN,         VDK_HYPHEN      },
    /* 3E */ { VDK_UNKNOWN,         VDK_PERIOD      },
    /* 3F */ { VDK_UNKNOWN,         VDK_SLASH       },
    /* 40 */ { VDK_UNKNOWN,         VDK_BACKQUOTE   },
    /* 41 */ { VDK_A,               VDK_UNKNOWN     },
    /* 42 */ { VDK_B,               VDK_UNKNOWN     },
    /* 43 */ { VDK_C,               VDK_UNKNOWN     },
    /* 44 */ { VDK_D,               VDK_UNKNOWN     },
    /* 45 */ { VDK_E,               VDK_UNKNOWN     },
    /* 46 */ { VDK_F,               VDK_UNKNOWN     },
    /* 47 */ { VDK_G,               VDK_UNKNOWN     },
    /* 48 */ { VDK_H,               VDK_UNKNOWN     },
    /* 49 */ { VDK_I,               VDK_UNKNOWN     },
    /* 4A */ { VDK_J,               VDK_UNKNOWN     },
    /* 4B */ { VDK_K,               VDK_UNKNOWN     },
    /* 4C */ { VDK_L,               VDK_UNKNOWN     },
    /* 4D */ { VDK_M,               VDK_UNKNOWN     },
    /* 4E */ { VDK_N,               VDK_UNKNOWN     },
    /* 4F */ { VDK_O,               VDK_UNKNOWN     },
    /* 50 */ { VDK_P,               VDK_UNKNOWN     },
    /* 51 */ { VDK_Q,               VDK_UNKNOWN     },
    /* 52 */ { VDK_R,               VDK_UNKNOWN     },
    /* 53 */ { VDK_S,               VDK_UNKNOWN     },
    /* 54 */ { VDK_T,               VDK_UNKNOWN     },
    /* 55 */ { VDK_U,               VDK_UNKNOWN     },
    /* 56 */ { VDK_V,               VDK_UNKNOWN     },
    /* 57 */ { VDK_W,               VDK_UNKNOWN     },
    /* 58 */ { VDK_X,               VDK_UNKNOWN     },
    /* 59 */ { VDK_Y,               VDK_UNKNOWN     },
    /* 5A */ { VDK_Z,               VDK_UNKNOWN     },
    /* 5B */ { VDK_LWINDOW,         VDK_LBRACKET    },
    /* 5C */ { VDK_RWINDOW,         VDK_BACKSLASH   },
    /* 5D */ { VDK_MENU,            VDK_RBRACKET    },
    /* 5E */ { VDK_UNKNOWN,         VDK_SINGLEQUOTE },
    /* 5F */ { VDK_SLEEP,           VDK_UNKNOWN     },
    /* 60 */ { VDK_PAD_0,           VDK_UNKNOWN     },
    /* 61 */ { VDK_PAD_1,           VDK_UNKNOWN     },
    /* 62 */ { VDK_PAD_2,           VDK_UNKNOWN     },
    /* 63 */ { VDK_PAD_3,           VDK_UNKNOWN     },
    /* 64 */ { VDK_PAD_4,           VDK_UNKNOWN     },
    /* 65 */ { VDK_PAD_5,           VDK_UNKNOWN     },
    /* 66 */ { VDK_PAD_6,           VDK_UNKNOWN     },
    /* 67 */ { VDK_PAD_7,           VDK_UNKNOWN     },
    /* 68 */ { VDK_PAD_8,           VDK_UNKNOWN     },
    /* 69 */ { VDK_PAD_9,           VDK_UNKNOWN     },
    /* 6A */ { VDK_PAD_ASTERISK,    VDK_UNKNOWN     },
    /* 6B */ { VDK_PAD_PLUS,        VDK_UNKNOWN     },
    /* 6C */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6D */ { VDK_PAD_HYPHEN,      VDK_UNKNOWN     },
    /* 6E */ { VDK_PAD_PERIOD,      VDK_UNKNOWN     },
    /* 6F */ { VDK_PAD_SLASH,       VDK_UNKNOWN     },
    /* 70 */ { VDK_F1,              VDK_UNKNOWN     },
    /* 71 */ { VDK_F2,              VDK_UNKNOWN     },
    /* 72 */ { VDK_F3,              VDK_UNKNOWN     },
    /* 73 */ { VDK_F4,              VDK_UNKNOWN     },
    /* 74 */ { VDK_F5,              VDK_UNKNOWN     },
    /* 75 */ { VDK_F6,              VDK_UNKNOWN     },
    /* 76 */ { VDK_F7,              VDK_UNKNOWN     },
    /* 77 */ { VDK_F8,              VDK_UNKNOWN     },
    /* 78 */ { VDK_F9,              VDK_UNKNOWN     },
    /* 79 */ { VDK_F10,             VDK_UNKNOWN     },
    /* 7A */ { VDK_F11,             VDK_UNKNOWN     },
    /* 7B */ { VDK_F12,             VDK_UNKNOWN     },
    /* 7C */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7D */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7E */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7F */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
};

/*******************************************************************************
** Windows OS wrappers. ********************************************************
*/

/*
    DllMain

    DLL entry point.  We do nothing here.

    PARAMETERS:

        HMODULE Module
            Module of the library.

        DWORD ReasonForCall
            Reason why the entry point is called:

            DLL_PROCESS_ATTACH
                Indicates the DLL is loaded by the current process.

            DLL_THREAD_ATTACH
                Indicates the current process is creating a new thread.

            DLL_THREAD_DETACH
                Indicates that a thread is exiting.

            DLL_PROCESS_DETACH
                Indicates the current process is exiting or called FreeLibrary.

        LPVOID Reserved
            NULL is all cases except when the reason is DLL_PROCESS_DETACH and
            the current process is exiting.

    RETURNS:

        BOOL
            TRUE if the DLL initialized successfully or FALSE if there was an
            error.
*/
#if !gcdSTATIC_LINK
BOOL APIENTRY
DllMain(
    HMODULE Module,
    DWORD ReasonForCall,
    LPVOID Reserved
    )
{
    UNREFERENCED_PARAMETER(Module);
    UNREFERENCED_PARAMETER(ReasonForCall);
    UNREFERENCED_PARAMETER(Reserved);

    /* Success. */
    return TRUE;
}
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
    WNDCLASS class;

    do
    {
        /* Allocate the private data structure. */
        vdk = (vdkPrivate) malloc(sizeof(struct _vdkPrivate));

        if (vdk == NULL)
        {
            /* Break if we are out of memory. */
            break;
        }

        /* Initialize the WNDCLASS structure. */
        ZeroMemory(&class, sizeof(class));
        class.lpfnWndProc   = _WindowProc;
        class.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
        class.hCursor       = LoadCursor(NULL, IDC_CROSS);
        class.hbrBackground = GetStockObject(WHITE_BRUSH);
        class.lpszClassName = TEXT("vdkClass");

        /* Register the window class. */
        vdk->class = RegisterClass(&class);

        if (vdk->class == 0)
        {
            /* RegisterClass failed. */
            _Error(TEXT("RegisterClass"));
            break;
        }

        /* Zero display pointer. */
        vdk->display = NULL;

#if gcdSTATIC_LINK
        vdk->egl = NULL;
#else
        /* Load the libEGL library.  We have to do this manually since we have a
        ** circular dependency: libEGL is dependent on libVDK, and libVDK
        ** calls libEGL again.  This creates havoc in Windows land. */
        vdk->egl = LoadLibrary(TEXT("libEGL.dll"));
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
        if (Private->class != 0)
        {
            /* Unregister the window class. */
            UnregisterClass(TEXT("vdkClass"), NULL);
        }

        if (Private->egl != NULL)
        {
            /* Free the libEGL library. */
            FreeLibrary(Private->egl);
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
    vdkDisplay display;

    /* If we have a valid private data structure and the display is already
    ** allocated, we can just return the current display pointer. */
    if ((Private != NULL) && (Private->display != NULL))
    {
        /* Return the current display pointer. */
        return Private->display;
    }
    display = (vdkDisplay)gcoOS_GetDisplay();
    if(display != NULL)
        Private->display = display;
    return display;
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
    return gcoOS_GetDisplayInfo((EGLNativeDisplayType)Display, Width, Height, Physical, Stride, BitsPerPixel);
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
    *Count = 1;
    return 1;
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
    gcoOS_DestroyDisplay((EGLNativeDisplayType)Display);
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
    return (vdkWindow)gcoOS_CreateWindow((EGLNativeDisplayType)Display, X, Y, Width, Height);
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
    return gcoOS_GetWindowInfo((EGLNativeWindowType)Window, X, Y, Width, Height, BitsPerPixel, Offset);
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
    gcoOS_DestroyWindow((EGLNativeWindowType)Window);
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
    HWND window;

    if (Window == NULL)
    {
        return 0;
    }

    /* Get the window handle. */
    if (IsWindow((HWND) Window))
    {
        window = (HWND) Window;
    }
    else
    {
        return 0;
    }

    /* Show the window. */
    ShowWindow(window, SW_SHOWNORMAL);

    /* Initial paint of the window. */
    UpdateWindow(window);

    /* Success. */
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
    HWND window;

    if (Window == NULL)
    {
        return 0;
    }

    /* Get the window handle. */
    if (IsWindow((HWND) Window))
    {
        window = (HWND) Window;
    }
    else
    {
        return 0;
    }

    /* Hide the window. */
    ShowWindow(window, SW_HIDE);

    /* Success. */
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
    return gcoOS_DrawImage((EGLNativeWindowType)Window, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits);
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
    return gcoOS_GetImage((EGLNativeWindowType)Window, Left, Top, Right, Bottom, BitsPerPixel, Bits);
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
    HWND window;
#ifdef UNICODE
    /* Temporary buffer. */
    LPTSTR title;

    /* Number of characters required for the temporary buffer. */
    int count;
#endif

    if (IsWindow((HWND) Window))
    {
        window = (HWND) Window;
    }
    else
    {
        return;
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
    if (title != NULL)
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
    if (Window == NULL)
    {
        ReleaseCapture();
        ClipCursor(NULL);
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
            return;
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
    /* Message. */
    MSG msg;

    /* Translated scancode. */
    vdkKeys scancode;

    /* Test for valid Window and Event pointers. */
    if ((Window == NULL) || (Event == NULL))
    {
        return 0;
    }

    /* Loop while there are messages in the queue for the window. */
    while (PeekMessage(&msg, (EGLNativeWindowType)Window, 0, 0, PM_REMOVE))
    {
        switch (msg.message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
            /* Keyboard event. */
            Event->type = VDK_KEYBOARD;

            /* Translate the scancode. */
            scancode = (msg.wParam & 0x80)
                     ? keys[msg.wParam & 0x7F].extended
                     : keys[msg.wParam & 0x7F].normal;

            /* Set scancode. */
            Event->data.keyboard.scancode = scancode;

            /* Set ASCII key. */
            Event->data.keyboard.key = (  (scancode < VDK_SPACE)
                                       || (scancode >= VDK_F1)
                                       )
                                       ? 0
                                       : (char) scancode;

            /* Set up or down flag. */
            Event->data.keyboard.pressed = (msg.message == WM_KEYDOWN);

            /* Valid event. */
            return 1;

        case WM_CLOSE:
        case WM_DESTROY:
        case WM_QUIT:
            /* Application should close. */
            Event->type = VDK_CLOSE;

            /* Valid event. */
            return 1;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            /* Button event. */
            Event->type = VDK_BUTTON;

            /* Set button states. */
            Event->data.button.left   = (msg.wParam & MK_LBUTTON) ? 1 : 0;
            Event->data.button.middle = (msg.wParam & MK_MBUTTON) ? 1 : 0;
            Event->data.button.right  = (msg.wParam & MK_RBUTTON) ? 1 : 0;
            Event->data.button.x      = LOWORD(msg.lParam);
            Event->data.button.y      = HIWORD(msg.lParam);

            /* Valid event. */
            return 1;

        case WM_MOUSEMOVE:
            /* Pointer event.*/
            Event->type = VDK_POINTER;

            /* Set pointer location. */
            Event->data.pointer.x = LOWORD(msg.lParam);
            Event->data.pointer.y = HIWORD(msg.lParam);

            /* Valid event. */
            return 1;

        default:
            /* Translate and dispatch message. */
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
    }

    /* Test if the window is still valid. */
    if (!IsWindow((EGLNativeWindowType)Window))
    {
        /* Application should close. */
        Event->type = VDK_CLOSE;

        /* Valid event. */
        return 1;
    }

    /* No event pending. */
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
    /* Make sure we have a valid pointer and that the EGL library has been
    ** loaded. */
    if ((Private == NULL) || (Private->egl == NULL))
    {
        return NULL;
    }

#ifdef UNICODE
    {
        /* Temporary buffer. */
        LPTSTR function;

        /* Function address. */
        EGL_ADDRESS pointer = NULL;

        /* Number of characters required for the temporary buffer. */
        int count;

        /* Query number of characters required for the temporary buffer. */
        count = MultiByteToWideChar(CP_ACP,
                                    MB_PRECOMPOSED,
                                    Function, -1,
                                    NULL, 0);

        /* Allocate the temporary buffer. */
        function = (LPTSTR) malloc(count * sizeof(TCHAR));

        /* Only process if the allocation succeeded. */
        if (function != NULL)
        {
            /* Convert the title into UNICODE. */
            MultiByteToWideChar(CP_ACP,
                                MB_PRECOMPOSED,
                                Function, -1,
                                function, count);

            /* Get address of function. */
            pointer = (EGL_ADDRESS) GetProcAddress(Private->egl, function);

            /* Free the temporary buffer. */
            free(function);
        }

        /* Return the address of the function. */
        return pointer;
    }
#else
    /* Get address of function. */
    return (EGL_ADDRESS) GetProcAddress(Private->egl, Function);
#endif
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
    return GetTickCount();
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
    return (vdkPixmap)gcoOS_CreatePixmap((EGLNativeDisplayType)Display, Width, Height, BitsPerPixel);
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
    return gcoOS_GetPixmapInfo((EGLNativePixmapType)Pixmap, Width, Height, BitsPerPixel, Stride, Bits);
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
    gcoOS_DestroyPixmap((EGLNativePixmapType)Pixmap);
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

#define gcdSUPPORT_EXTERNAL_IMAGE_EXT 1

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
            return gcvNULL;
        }

        if(gcoSURF_Construct(gcvNULL,
                          Width,
                          Height,
                          1,
                          gcvSURF_BITMAP,
                          format,
                          gcvPOOL_SYSTEM,
                          &surf) != gcvSTATUS_OK)
        {
            break;
        }

        /* Return pointer to the pixmap data structure. */
        return (vdkClientBuffer)surf;
    }
    while (0);
#endif

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
#if gcdSUPPORT_EXTERNAL_IMAGE_EXT
    gceSTATUS status;

    if (gcoSURF_IsValid((gcoSURF)ClientBuffer) != gcvSTATUS_TRUE)
    {
        return 0;
    }

    if (Width || Height)
    {
        status = gcoSURF_GetSize((gcoSURF)ClientBuffer,
                                 (unsigned int*)Width,
                                 (unsigned int*)Height,
                                 gcvNULL);

        if (status != gcvSTATUS_OK)
        {
            return 0;
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
            return 0;
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
            return 0;
        }

        *Bits = memory[0];

    }

    return 1;
#else
    return 0;
#endif
}

VDKAPI int VDKLANG
vdkDestroyClientBuffer(
    vdkClientBuffer ClientBuffer
    )
{
#if gcdSUPPORT_EXTERNAL_IMAGE_EXT
    return gcoSURF_Destroy((gcoSURF)ClientBuffer) == gcvSTATUS_OK;
#else
    return 0;
#endif
}

