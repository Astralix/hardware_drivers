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
#include <stdlib.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <string.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <screen/screen.h>
#include <KHR/khronos_utils.h>
#include "gc_hal_eglplatform.h"

screen_context_t screen_ctx = NULL;

struct _vdkPrivate
{
	vdkDisplay	display;
	vdkKeyMap *	map;
	void *		egl;
	int		displayId;
};

static vdkKeyMap keys[] =
{
    /* 00 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 01 */ { VDK_ESCAPE,          VDK_UNKNOWN     },
    /* 02 */ { VDK_1,               VDK_UNKNOWN     },
    /* 03 */ { VDK_2,               VDK_UNKNOWN     },
    /* 04 */ { VDK_3,               VDK_UNKNOWN     },
    /* 05 */ { VDK_4,               VDK_UNKNOWN     },
    /* 06 */ { VDK_5,               VDK_UNKNOWN     },
    /* 07 */ { VDK_6,               VDK_UNKNOWN     },
    /* 08 */ { VDK_7,               VDK_UNKNOWN     },
    /* 09 */ { VDK_8,               VDK_UNKNOWN     },
    /* 0A */ { VDK_9,               VDK_UNKNOWN     },
    /* 0B */ { VDK_0,               VDK_UNKNOWN     },
    /* 0C */ { VDK_HYPHEN,          VDK_UNKNOWN     },
    /* 0D */ { VDK_EQUAL,           VDK_UNKNOWN     },
    /* 0E */ { VDK_BACKSPACE,       VDK_UNKNOWN     },
    /* 0F */ { VDK_TAB,             VDK_UNKNOWN     },
    /* 10 */ { VDK_Q,               VDK_UNKNOWN     },
    /* 11 */ { VDK_W,               VDK_UNKNOWN     },
    /* 12 */ { VDK_E,               VDK_UNKNOWN     },
    /* 13 */ { VDK_R,               VDK_UNKNOWN     },
    /* 14 */ { VDK_T,               VDK_UNKNOWN     },
    /* 15 */ { VDK_Y,               VDK_UNKNOWN     },
    /* 16 */ { VDK_U,               VDK_UNKNOWN     },
    /* 17 */ { VDK_I,               VDK_UNKNOWN     },
    /* 18 */ { VDK_O,               VDK_UNKNOWN     },
    /* 19 */ { VDK_P,               VDK_UNKNOWN     },
    /* 1A */ { VDK_LBRACKET,        VDK_UNKNOWN     },
    /* 1B */ { VDK_RBRACKET,        VDK_UNKNOWN     },
    /* 1C */ { VDK_ENTER,           VDK_PAD_ENTER   },
    /* 1D */ { VDK_LCTRL,           VDK_RCTRL       },
    /* 1E */ { VDK_A,               VDK_UNKNOWN     },
    /* 1F */ { VDK_S,               VDK_UNKNOWN     },
    /* 20 */ { VDK_D,               VDK_UNKNOWN     },
    /* 21 */ { VDK_F,               VDK_UNKNOWN     },
    /* 22 */ { VDK_G,               VDK_UNKNOWN     },
    /* 23 */ { VDK_H,               VDK_UNKNOWN     },
    /* 24 */ { VDK_J,               VDK_UNKNOWN     },
    /* 25 */ { VDK_K,               VDK_UNKNOWN     },
    /* 26 */ { VDK_L,               VDK_UNKNOWN     },
    /* 27 */ { VDK_SEMICOLON,       VDK_UNKNOWN     },
    /* 28 */ { VDK_SINGLEQUOTE,     VDK_UNKNOWN     },
    /* 29 */ { VDK_BACKQUOTE,       VDK_UNKNOWN     },
    /* 2A */ { VDK_LSHIFT,          VDK_UNKNOWN     },
    /* 2B */ { VDK_BACKSLASH,       VDK_UNKNOWN     },
    /* 2C */ { VDK_Z,               VDK_UNKNOWN     },
    /* 2D */ { VDK_X,               VDK_UNKNOWN     },
    /* 2E */ { VDK_C,               VDK_UNKNOWN     },
    /* 2F */ { VDK_V,               VDK_UNKNOWN     },
    /* 30 */ { VDK_B,               VDK_UNKNOWN     },
    /* 31 */ { VDK_N,               VDK_UNKNOWN     },
    /* 32 */ { VDK_M,               VDK_UNKNOWN     },
    /* 33 */ { VDK_COMMA,           VDK_UNKNOWN     },
    /* 34 */ { VDK_PERIOD,          VDK_UNKNOWN     },
    /* 35 */ { VDK_SLASH,           VDK_PAD_SLASH   },
    /* 36 */ { VDK_RSHIFT,          VDK_UNKNOWN     },
    /* 37 */ { VDK_PAD_ASTERISK,    VDK_PRNTSCRN    },
    /* 38 */ { VDK_LALT,            VDK_RALT        },
    /* 39 */ { VDK_SPACE,           VDK_UNKNOWN     },
    /* 3A */ { VDK_CAPSLOCK,        VDK_UNKNOWN     },
    /* 3B */ { VDK_F1,              VDK_UNKNOWN     },
    /* 3C */ { VDK_F2,              VDK_UNKNOWN     },
    /* 3D */ { VDK_F3,              VDK_UNKNOWN     },
    /* 3E */ { VDK_F4,              VDK_UNKNOWN     },
    /* 3F */ { VDK_F5,              VDK_UNKNOWN     },
    /* 40 */ { VDK_F6,              VDK_UNKNOWN     },
    /* 41 */ { VDK_F7,              VDK_UNKNOWN     },
    /* 42 */ { VDK_F8,              VDK_UNKNOWN     },
    /* 43 */ { VDK_F9,              VDK_UNKNOWN     },
    /* 44 */ { VDK_F10,             VDK_UNKNOWN     },
    /* 45 */ { VDK_NUMLOCK,         VDK_UNKNOWN     },
    /* 46 */ { VDK_SCROLLLOCK,      VDK_BREAK       },
    /* 47 */ { VDK_PAD_7,           VDK_HOME        },
    /* 48 */ { VDK_PAD_8,           VDK_UP          },
    /* 49 */ { VDK_PAD_9,           VDK_PGUP        },
    /* 4A */ { VDK_PAD_HYPHEN,      VDK_UNKNOWN     },
    /* 4B */ { VDK_PAD_4,           VDK_LEFT        },
    /* 4C */ { VDK_PAD_5,           VDK_UNKNOWN     },
    /* 4D */ { VDK_PAD_6,           VDK_RIGHT       },
    /* 4E */ { VDK_PAD_PLUS,        VDK_UNKNOWN     },
    /* 4F */ { VDK_PAD_1,           VDK_END         },
    /* 50 */ { VDK_PAD_2,           VDK_DOWN        },
    /* 51 */ { VDK_PAD_3,           VDK_PGDN        },
    /* 52 */ { VDK_PAD_0,           VDK_INSERT      },
    /* 53 */ { VDK_PAD_PERIOD,      VDK_DELETE      },
    /* 54 */ { VDK_SYSRQ,           VDK_UNKNOWN     },
    /* 55 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 56 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 57 */ { VDK_F11,             VDK_UNKNOWN     },
    /* 58 */ { VDK_F12,             VDK_UNKNOWN     },
    /* 59 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 5A */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 5B */ { VDK_UNKNOWN,         VDK_LWINDOW     },
    /* 5C */ { VDK_UNKNOWN,         VDK_RWINDOW     },
    /* 5D */ { VDK_UNKNOWN,         VDK_MENU        },
    /* 5E */ { VDK_UNKNOWN,         VDK_POWER       },
    /* 5F */ { VDK_UNKNOWN,         VDK_SLEEP       },
    /* 60 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 61 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 62 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 63 */ { VDK_UNKNOWN,         VDK_WAKE        },
    /* 64 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 65 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 66 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 67 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 68 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 69 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6A */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6B */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6C */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6D */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6E */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6F */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 70 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 71 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 72 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 73 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 74 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 75 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 76 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 77 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 78 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 79 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7A */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7B */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7C */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7D */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7E */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 7F */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
};

/*******************************************************************************
** Initialization.
*/

/*
** GN NOTE This function never gets called by EGL and is there only for applications
** to initialize the VDK (so it can find EGL).
*/

VDKAPI vdkPrivate VDKLANG
vdkInitialize(
    void
    )
{
    vdkPrivate vdk;
    int        rc;

    do
    {
        vdk = (vdkPrivate)calloc(1, sizeof(struct _vdkPrivate));
        if (vdk == NULL)
        {
			printf("Could not allocate memory.\n");
            break;
        }

		/* other members are initialized to zero through the calloc call */
        vdk->map     = keys;
		vdk->displayId = (int) EGL_DEFAULT_DISPLAY;
        vdk->egl = dlopen("libEGL.so.1", RTLD_NOW);
        if (vdk->egl == NULL)
        {
			printf("Could not find libEGL.so.1\n");
            break;
        }

		rc = screen_create_context(&screen_ctx, 0);
		if (rc)
		{
			fprintf(stderr, "screen_create_context failed with error %d (0x%08x)\n", errno, errno);
			break;
		}

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
	if (screen_ctx)
	{
		screen_destroy_context(screen_ctx);
		screen_ctx = NULL;
	}

	if (Private->egl != NULL)
    {
    	dlclose(Private->egl);
    }

    if (Private != NULL)
    {
        free(Private);
    }
}

/*******************************************************************************
** Display.
*/

/*
** GN NOTE  For QNX, we just want the vdkDisplay to be a display_id (i.e. a number).
** EGL and VDK only call this function to get the default display_id, but with QNX's
** redirection layer, the default display is already taken care of (i.e. it determines
** which display_id it should use when given EGL_DEFAULT_DISPLAY, so we simply need to
** return EGL_DEFAULT_DISPLAY.
*/
VDKAPI vdkDisplay VDKLANG
vdkGetDisplay(
    vdkPrivate Data
    )
{
	return (vdkDisplay)gcoOS_GetDisplay();
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
    vdkWindow Window,
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
	vdkWindow Window,
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
    return gcoOS_GetWindowInfo((EGLNativeWindowType)Window, X, Y, Width, Height, BitsPerPixel, Offset);
}

VDKAPI void VDKLANG
vdkDestroyWindow(
    vdkWindow Window
    )
{
    gcoOS_DestroyWindow((EGLNativeWindowType)Window);
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
vdkShowWindow(
    vdkWindow Window
    )
{
	return 1;
}

VDKAPI int VDKLANG
vdkHideWindow(
    vdkWindow Window
    )
{
	return 1;
}

VDKAPI void VDKLANG
vdkSetWindowTitle(
    vdkWindow Window,
    const char * Title
    )
{
	return;
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
	screen_event_t screen_evt;
	int rc;
	int result = 0;

	rc = screen_create_event(&screen_evt);

	while (screen_get_event(screen_ctx, screen_evt, 0L) == 0)
	{
		int type;

		screen_get_event_property_iv(screen_evt, SCREEN_PROPERTY_TYPE, &type);

		if (type ==  SCREEN_EVENT_CLOSE)
		{
			Event->type = VDK_CLOSE;

			result = 1;
			break;
		}
		else if (type == SCREEN_EVENT_POINTER)
		{
			static int last_buttons;
			int buttons;
			int pointer[2];

			screen_get_event_property_iv(screen_evt, SCREEN_PROPERTY_BUTTONS, &buttons);
			screen_get_event_property_iv(screen_evt, SCREEN_PROPERTY_POSITION, pointer);

			if (buttons != last_buttons)
			{
				Event->type = VDK_BUTTON;
				Event->data.button.left   = (buttons & 0x0001);
				/* TODO
				Event->data.button.middle = (buttons & 0x????);
				Event->data.button.right  = (buttons & 0x????);
				*/
				Event->data.button.x = pointer[0];
				Event->data.button.y = pointer[1];

				last_buttons =  buttons;
			}
			else
			{
				Event->type = VDK_POINTER;
				Event->data.pointer.x = pointer[0];
				Event->data.pointer.y = pointer[1];
			}

			result = 1;
			break;
		}
		else if (type == SCREEN_EVENT_KEYBOARD)
		{
			int buffer;
			int scancode;
			static int prefix;

			screen_get_event_property_iv(screen_evt, SCREEN_PROPERTY_KEY_SCAN, &buffer);

	        if ((buffer == 0xE0) || (buffer == 0xE1))
			{
				prefix = buffer;
				continue;
			}

			if (prefix)
			{
				scancode = keys[buffer & 0x7F].extended;
				prefix = 0;
			}
			else
			{
				scancode = keys[buffer & 0x7F].normal;
			}

			if (scancode == VDK_UNKNOWN)
			{
				continue;
			}

        	Event->type					  = VDK_KEYBOARD;
			Event->data.keyboard.scancode = scancode;
			Event->data.keyboard.pressed  = buffer < 0x80;
        	Event->data.keyboard.key      = (  (scancode < VDK_SPACE)
	                                        || (scancode >= VDK_F1)
    	                                    )
        	                                ? 0
            	                            : (char) scancode;
			result = 1;
			break;
		}
		else if (type == SCREEN_EVENT_NONE)
		{
			break;
		}
		else
		{
			break;
		}
	}

	screen_destroy_event(screen_evt);

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
    return gcoOS_GetPixmapInfo((EGLNativePixmapType)Pixmap, Width, Height, BitsPerPixel, Stride, Bits);
}

VDKAPI void VDKLANG
vdkDestroyPixmap(
    vdkPixmap Pixmap
    )
{
    gcoOS_DestroyPixmap((EGLNativePixmapType)Pixmap);
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

