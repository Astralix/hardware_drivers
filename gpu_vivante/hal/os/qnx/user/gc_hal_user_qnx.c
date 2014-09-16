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



#include "gc_hal_user_qnx.h"
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

#define _GC_OBJ_ZONE    gcvZONE_OS

screen_context_t screen_ctx = gcvNULL;

static halKeyMap keys[] =
{
    /* 00 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 01 */ { HAL_ESCAPE,          HAL_UNKNOWN     },
    /* 02 */ { HAL_1,               HAL_UNKNOWN     },
    /* 03 */ { HAL_2,               HAL_UNKNOWN     },
    /* 04 */ { HAL_3,               HAL_UNKNOWN     },
    /* 05 */ { HAL_4,               HAL_UNKNOWN     },
    /* 06 */ { HAL_5,               HAL_UNKNOWN     },
    /* 07 */ { HAL_6,               HAL_UNKNOWN     },
    /* 08 */ { HAL_7,               HAL_UNKNOWN     },
    /* 09 */ { HAL_8,               HAL_UNKNOWN     },
    /* 0A */ { HAL_9,               HAL_UNKNOWN     },
    /* 0B */ { HAL_0,               HAL_UNKNOWN     },
    /* 0C */ { HAL_HYPHEN,          HAL_UNKNOWN     },
    /* 0D */ { HAL_EQUAL,           HAL_UNKNOWN     },
    /* 0E */ { HAL_BACKSPACE,       HAL_UNKNOWN     },
    /* 0F */ { HAL_TAB,             HAL_UNKNOWN     },
    /* 10 */ { HAL_Q,               HAL_UNKNOWN     },
    /* 11 */ { HAL_W,               HAL_UNKNOWN     },
    /* 12 */ { HAL_E,               HAL_UNKNOWN     },
    /* 13 */ { HAL_R,               HAL_UNKNOWN     },
    /* 14 */ { HAL_T,               HAL_UNKNOWN     },
    /* 15 */ { HAL_Y,               HAL_UNKNOWN     },
    /* 16 */ { HAL_U,               HAL_UNKNOWN     },
    /* 17 */ { HAL_I,               HAL_UNKNOWN     },
    /* 18 */ { HAL_O,               HAL_UNKNOWN     },
    /* 19 */ { HAL_P,               HAL_UNKNOWN     },
    /* 1A */ { HAL_LBRACKET,        HAL_UNKNOWN     },
    /* 1B */ { HAL_RBRACKET,        HAL_UNKNOWN     },
    /* 1C */ { HAL_ENTER,           HAL_PAD_ENTER   },
    /* 1D */ { HAL_LCTRL,           HAL_RCTRL       },
    /* 1E */ { HAL_A,               HAL_UNKNOWN     },
    /* 1F */ { HAL_S,               HAL_UNKNOWN     },
    /* 20 */ { HAL_D,               HAL_UNKNOWN     },
    /* 21 */ { HAL_F,               HAL_UNKNOWN     },
    /* 22 */ { HAL_G,               HAL_UNKNOWN     },
    /* 23 */ { HAL_H,               HAL_UNKNOWN     },
    /* 24 */ { HAL_J,               HAL_UNKNOWN     },
    /* 25 */ { HAL_K,               HAL_UNKNOWN     },
    /* 26 */ { HAL_L,               HAL_UNKNOWN     },
    /* 27 */ { HAL_SEMICOLON,       HAL_UNKNOWN     },
    /* 28 */ { HAL_SINGLEQUOTE,     HAL_UNKNOWN     },
    /* 29 */ { HAL_BACKQUOTE,       HAL_UNKNOWN     },
    /* 2A */ { HAL_LSHIFT,          HAL_UNKNOWN     },
    /* 2B */ { HAL_BACKSLASH,       HAL_UNKNOWN     },
    /* 2C */ { HAL_Z,               HAL_UNKNOWN     },
    /* 2D */ { HAL_X,               HAL_UNKNOWN     },
    /* 2E */ { HAL_C,               HAL_UNKNOWN     },
    /* 2F */ { HAL_V,               HAL_UNKNOWN     },
    /* 30 */ { HAL_B,               HAL_UNKNOWN     },
    /* 31 */ { HAL_N,               HAL_UNKNOWN     },
    /* 32 */ { HAL_M,               HAL_UNKNOWN     },
    /* 33 */ { HAL_COMMA,           HAL_UNKNOWN     },
    /* 34 */ { HAL_PERIOD,          HAL_UNKNOWN     },
    /* 35 */ { HAL_SLASH,           HAL_PAD_SLASH   },
    /* 36 */ { HAL_RSHIFT,          HAL_UNKNOWN     },
    /* 37 */ { HAL_PAD_ASTERISK,    HAL_PRNTSCRN    },
    /* 38 */ { HAL_LALT,            HAL_RALT        },
    /* 39 */ { HAL_SPACE,           HAL_UNKNOWN     },
    /* 3A */ { HAL_CAPSLOCK,        HAL_UNKNOWN     },
    /* 3B */ { HAL_F1,              HAL_UNKNOWN     },
    /* 3C */ { HAL_F2,              HAL_UNKNOWN     },
    /* 3D */ { HAL_F3,              HAL_UNKNOWN     },
    /* 3E */ { HAL_F4,              HAL_UNKNOWN     },
    /* 3F */ { HAL_F5,              HAL_UNKNOWN     },
    /* 40 */ { HAL_F6,              HAL_UNKNOWN     },
    /* 41 */ { HAL_F7,              HAL_UNKNOWN     },
    /* 42 */ { HAL_F8,              HAL_UNKNOWN     },
    /* 43 */ { HAL_F9,              HAL_UNKNOWN     },
    /* 44 */ { HAL_F10,             HAL_UNKNOWN     },
    /* 45 */ { HAL_NUMLOCK,         HAL_UNKNOWN     },
    /* 46 */ { HAL_SCROLLLOCK,      HAL_BREAK       },
    /* 47 */ { HAL_PAD_7,           HAL_HOME        },
    /* 48 */ { HAL_PAD_8,           HAL_UP          },
    /* 49 */ { HAL_PAD_9,           HAL_PGUP        },
    /* 4A */ { HAL_PAD_HYPHEN,      HAL_UNKNOWN     },
    /* 4B */ { HAL_PAD_4,           HAL_LEFT        },
    /* 4C */ { HAL_PAD_5,           HAL_UNKNOWN     },
    /* 4D */ { HAL_PAD_6,           HAL_RIGHT       },
    /* 4E */ { HAL_PAD_PLUS,        HAL_UNKNOWN     },
    /* 4F */ { HAL_PAD_1,           HAL_END         },
    /* 50 */ { HAL_PAD_2,           HAL_DOWN        },
    /* 51 */ { HAL_PAD_3,           HAL_PGDN        },
    /* 52 */ { HAL_PAD_0,           HAL_INSERT      },
    /* 53 */ { HAL_PAD_PERIOD,      HAL_DELETE      },
    /* 54 */ { HAL_SYSRQ,           HAL_UNKNOWN     },
    /* 55 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 56 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 57 */ { HAL_F11,             HAL_UNKNOWN     },
    /* 58 */ { HAL_F12,             HAL_UNKNOWN     },
    /* 59 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 5A */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 5B */ { HAL_UNKNOWN,         HAL_LWINDOW     },
    /* 5C */ { HAL_UNKNOWN,         HAL_RWINDOW     },
    /* 5D */ { HAL_UNKNOWN,         HAL_MENU        },
    /* 5E */ { HAL_UNKNOWN,         HAL_POWER       },
    /* 5F */ { HAL_UNKNOWN,         HAL_SLEEP       },
    /* 60 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 61 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 62 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 63 */ { HAL_UNKNOWN,         HAL_WAKE        },
    /* 64 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 65 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 66 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 67 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 68 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 69 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6A */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6B */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6C */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6D */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6E */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6F */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 70 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 71 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 72 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 73 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 74 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 75 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 76 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 77 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 78 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 79 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7A */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7B */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7C */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7D */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7E */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 7F */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
};

static gceSTATUS
_ConvertWindowFormat(
    IN gctINT ScreenFormat,
    OUT gctINT * BitsPerPixel,
    OUT gctINT * Format
    )
{
    gctUINT bpp, fmt;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("ScreenFormat=%d.", ScreenFormat);

    switch (ScreenFormat)
    {
        case SCREEN_FORMAT_BYTE:
            bpp = 8;
            fmt = gcvSURF_L8;
            break;

        case SCREEN_FORMAT_RGBA4444:
            bpp = 16;
            fmt = gcvSURF_A4R4G4B4;
            break;

        case SCREEN_FORMAT_RGBX4444:
            bpp = 16;
            fmt = gcvSURF_X4R4G4B4;
            break;

        case SCREEN_FORMAT_RGBA5551:
            bpp = 16;
            fmt = gcvSURF_A1R5G5B5;
            break;

        case SCREEN_FORMAT_RGBX5551:
            bpp = 16;
            fmt = gcvSURF_X1R5G5B5;
            break;

        case SCREEN_FORMAT_RGB565:
            bpp = 16;
            fmt = gcvSURF_R5G6B5;
            break;

        case SCREEN_FORMAT_RGB888:
            bpp = 24;
            fmt = gcvSURF_R8G8B8;
            break;

        case SCREEN_FORMAT_RGBA8888:
            bpp = 32;
            fmt = gcvSURF_A8R8G8B8;
            break;

        case SCREEN_FORMAT_RGBX8888:
            bpp = 32;
            fmt = gcvSURF_X8R8G8B8;
            break;

        default:
            goto OnError;
    }

    if (gcvNULL != BitsPerPixel)
    {
        *BitsPerPixel = bpp;
    }

    if (gcvNULL != Format)
    {
        *Format = fmt;
    }

    gcmFOOTER_ARG("*BitsPerPixel=%d, *Format=%d.", bpp, fmt);
    return status;

OnError:
    status = gcvSTATUS_INVALID_ARGUMENT;
    gcmFOOTER();
    return status;
}

/*******************************************************************************
** Display.
*/

gceSTATUS
gcoOS_GetDisplay(
    OUT HALNativeDisplayType * Display
    )
{
    *Display = (HALNativeDisplayType)1;
    return gcvSTATUS_OK;
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
    return gcvSTATUS_NOT_SUPPORTED;
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
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];
    gctPOINTER pointer;
    off64_t paddr;
    gctINT rc, stride;
    gctINT size[2];

    gcmHEADER_ARG("Display=0x%x Window=0x%x DisplayInfoSize=%u", Display, Window, DisplayInfoSize);

    /* Valid Window? and structure size? */
    if ((Window == gcvNULL) || (DisplayInfoSize != sizeof(halDISPLAY_INFO)))
    {
        goto OnError;
    }

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc)
    {
        goto OnError;
    }

    /* For QNX, the Width and Height are size of the framebuffer. */
    rc = screen_get_buffer_property_iv(buf[0], SCREEN_PROPERTY_BUFFER_SIZE, size);
    if (rc)
    {
        goto OnError;
    }

    DisplayInfo->width  = size[0];
    DisplayInfo->height = size[1];

    /* The stride of the window. */
    rc = screen_get_buffer_property_iv(buf[0], SCREEN_PROPERTY_STRIDE, &stride);
    if (rc)
    {
        goto OnError;
    }

    DisplayInfo->stride = stride;

    /* The logical address of the window. */
    rc = screen_get_buffer_property_pv(buf[0], SCREEN_PROPERTY_POINTER, &pointer);
    if (rc)
    {
        goto OnError;
    }

    DisplayInfo->logical = pointer;

    /* The physical address of the window. */
    rc = screen_get_buffer_property_llv(buf[0], SCREEN_PROPERTY_PHYSICAL_ADDRESS, &paddr);
    if (rc)
    {
        goto OnError;
    }

    DisplayInfo->physical = paddr;

    /* Flip. */
    DisplayInfo->flip = 1;

    /* Success. */
    gcmFOOTER_ARG("*DisplayInfo=0x%x", *DisplayInfo);
    return status;

OnError:
    status = gcvSTATUS_INVALID_ARGUMENT;
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_GetNextDisplayInfoEx(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctUINT DisplayInfoSize,
    OUT halDISPLAY_INFO * DisplayInfo
    )
{
    int rc;
    gceSTATUS status = gcvSTATUS_OK;
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];

    gcmHEADER_ARG("Display=0x%x Window=0x%x DisplayInfoSize=%u", Display, Window, DisplayInfoSize);

    if ((Window == gcvNULL) || (DisplayInfoSize != sizeof(halDISPLAY_INFO)))
    {
        goto OnError;
    }

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc)
    {
        goto OnError;
    }

    rc = screen_post_window((screen_window_t)Window, buf[0], 0, NULL, 0);
    if (rc)
    {
        goto OnError;
    }

    status = gcoOS_GetDisplayInfoEx(Display, Window, DisplayInfoSize, DisplayInfo);

    gcmFOOTER_ARG("*DisplayInfo=0x%x", *DisplayInfo);
    return status;

OnError:
    status = gcvSTATUS_INVALID_ARGUMENT;
    gcmFOOTER();
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
    *Offset = 0;
    *X = 0;
    *Y = 0;
    return gcvSTATUS_OK;
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
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];
    int rc;
    int rects[4];
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Display=0x%x Window=0x%x Offset=%u X=%d Y=%d", Display, Window, Offset, X, Y);

    rects[0] = 0;
    rects[1] = 0;

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc)
    {
        goto OnError;
    }

    rc = screen_get_window_property_iv((screen_window_t)Window, SCREEN_PROPERTY_BUFFER_SIZE, &rects[2]);
    if (rc)
    {
        goto OnError;
    }

    rc = screen_post_window((screen_window_t)Window, buf[0], 1, rects, 0);
    if (rc)
    {
        goto OnError;
    }

    gcmFOOTER_NO();
    return status;

OnError:
    status = gcvSTATUS_INVALID_ARGUMENT;
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_DisplayBufferRegions(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctINT NumRects,
    IN gctINT_PTR Rects
    )
{
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];
    int rc;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Display=0x%x Window=0x%x NumRects=%d", Display, Window, NumRects);

    rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
    if (rc) {
        goto OnError;
    }

    rc = screen_post_window((screen_window_t)Window, buf[0], NumRects, Rects, 0);
    if (rc) {
        goto OnError;
    }

    gcmFOOTER_NO();
    return status;

OnError:
    status = gcvSTATUS_INVALID_ARGUMENT;
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_DestroyDisplay(
    IN HALNativeDisplayType Display
    )
{
    /*
     * nothing to do because GetDefaultDisplay simply returned an integer...
     */

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
    gctINT pos[2];
    gctINT size[2];
    gctINT screen_format = SCREEN_FORMAT_RGBX8888;
    gctINT screen_transparency = SCREEN_TRANSPARENCY_NONE;
    gctINT screen_usage = SCREEN_USAGE_OPENGL_ES1 | SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_OPENVG
            ;
    /* Use 0 for no-vsync, and 1 for vsync limited. */
    gctINT screen_swap_interval = 0;
    gctINT rc;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x X=%d Y=%d Width=%d Height=%d", Display, X, Y, Width, Height);

    /* Create window strcture. */
    rc = screen_create_window((struct _screen_window **)Window, screen_ctx);
    if (rc)
    {
        fprintf(stderr, "screen_create_window failed with error %d (0x%08x)\n", errno, errno);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Set window pximap format. */
    rc = screen_set_window_property_iv(*Window, SCREEN_PROPERTY_FORMAT, &screen_format);
    if (rc)
    {
        fprintf(stderr, "screen_set_window_property_iv(SCREEN_PROPERTY_FORMAT) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_window(*Window);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Set window usage. */
    rc = screen_set_window_property_iv(*Window, SCREEN_PROPERTY_USAGE, &screen_usage);
    if (rc)
    {
        fprintf(stderr, "screen_set_window_property_iv(SCREEN_PROPERTY_USAGE) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_window(*Window);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Get fullscreen window size. */
    rc = screen_get_window_property_iv(*Window, SCREEN_PROPERTY_SIZE, size);
    if (rc)
    {
        fprintf(stderr, "screen_get_window_property_iv(SCREEN_PROPERTY_SIZE) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_window(*Window);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Disable transparency. Due to a bug in Screen, this must be set after format. */
    rc = screen_set_window_property_iv(*Window, SCREEN_PROPERTY_TRANSPARENCY, &screen_transparency);
    if (rc)
    {
        fprintf(stderr, "screen_get_window_property_iv(SCREEN_PROPERTY_TRANSPARENCY) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_window(*Window);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Set swap interval. */
    rc = screen_set_window_property_iv(*Window, SCREEN_PROPERTY_SWAP_INTERVAL, &screen_swap_interval);
    if (rc)
    {
        fprintf(stderr, "screen_get_window_property_iv(SCREEN_PROPERTY_SWAP_INTERVAL) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_window(*Window);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Test for zero width. */
    if (Width == 0)
    {
        Width = size[0];
    }

    /* Test for zero height. */
    if (Height == 0)
    {
        Height = size[1];
    }

    /* Test for auto-center X coordinate. */
    if (X == -1)
    {
        X = (size[0] - Width) / 2;
    }

    /* Test for auto-center X coordinate. */
    if (Y == -1)
    {
        Y = (size[1] - Height) / 2;
    }

    /* Resize the window. */
    size[0] = Width;
    size[1] = Height;

    rc = screen_set_window_property_iv(*Window, SCREEN_PROPERTY_SIZE, size);
    if (rc)
    {
        fprintf(stderr, "screen_set_window_property_iv(SCREEN_PROPERTY_SIZE) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_window(*Window);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Create window buffer. */
    /* Second argument is the number of back buffers to be used. */
    rc = screen_create_window_buffers(*Window, gcdDISPLAY_BACK_BUFFERS);
    if (rc)
    {
        fprintf(stderr, "screen_create_window_buffers failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_window(*Window);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Move window position. */
    pos[0] = X;
    pos[1] = Y;

    rc = screen_set_window_property_iv(*Window, SCREEN_PROPERTY_POSITION, pos);
    if (rc) {
        fprintf(stderr, "screen_set_window_property_iv(SCREEN_PROPERTY_POSITION) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_window(*Window);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    gcmFOOTER_ARG("*Window=0x%x", *Window);
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
    OUT gctINT * Format,
    OUT gctUINT * Offset
    )
{
    gctINT rc, size[2], format;
    screen_buffer_t buf[gcdDISPLAY_BACK_BUFFERS];
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Display=0x%x Window=0x%x", Display, Window);

    if (Window == gcvNULL)
    {
        /* Window is not a valid window data structure pointer. */
        goto OnError;
    }

    if (X != gcvNULL)
    {
        *X = 0;
    }

    if (Y != gcvNULL)
    {
        *Y = 0;
    }

    if ((Width != gcvNULL) || (Height != gcvNULL))
    {
        rc = screen_get_window_property_pv((screen_window_t)Window, SCREEN_PROPERTY_RENDER_BUFFERS, (void**)buf);
        if (rc)
        {
            goto OnError;
        }

        rc = screen_get_buffer_property_iv(buf[0], SCREEN_PROPERTY_BUFFER_SIZE, size);
        if (rc)
        {
            goto OnError;
        }

        if (Width != gcvNULL)
        {
            *Width = size[0];
        }

        if (Height != gcvNULL)
        {
            *Height = size[1];
        }
    }

    if ((BitsPerPixel != gcvNULL) || (Format != gcvNULL))
    {
        rc = screen_get_window_property_iv(Window, SCREEN_PROPERTY_FORMAT, &format);
        if (rc)
        {
            goto OnError;
        }

        _ConvertWindowFormat(format, BitsPerPixel, Format);

    }

    if (Offset != gcvNULL)
    {
        /* TODO: Check if offset means anything in qnx. */
        *Offset = 0;
    }

    /* Success. */
    gcmFOOTER_NO();
    return status;

OnError:
    status = gcvSTATUS_INVALID_ARGUMENT;
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_DestroyWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    if (Window != gcvNULL)
    {
        screen_destroy_window(Window);
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
    return gcvSTATUS_NOT_SUPPORTED;
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
    gctINT size[2];
    gctINT screen_format = SCREEN_FORMAT_RGBA8888;
    gctINT screen_usage = SCREEN_USAGE_OPENGL_ES1 | SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_OPENVG;
    gctINT rc;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Width=%d Height=%d BitsPerPixel=%d", Display, Width, Height, BitsPerPixel);

    if ((Width <= 0) || (Height <= 0) || (BitsPerPixel <= 0))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
    /* Create pixmap structure. */
    rc = screen_create_pixmap((struct _screen_pixmap **)Pixmap, screen_ctx);
    if (rc)
    {
        fprintf(stderr, "screen_create_pixmap failed with error %d (0x%08x)\n", errno, errno);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    switch (BitsPerPixel)
    {
    case 8:
        screen_format = SCREEN_FORMAT_BYTE;
        break;

    case 16:
        screen_format = SCREEN_FORMAT_RGB565;
        break;

    case 24:
        screen_format = SCREEN_FORMAT_RGB888;
        break;

    case 32:
        screen_format = SCREEN_FORMAT_RGBA8888;
        break;

    default:
        break;
    }

    /* Set pximap format. */
    rc = screen_set_pixmap_property_iv(*Pixmap, SCREEN_PROPERTY_FORMAT, &screen_format);
    if (rc)
    {
        fprintf(stderr, "screen_set_pixmap_property_iv(SCREEN_PROPERTY_FORMAT) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_pixmap(*Pixmap);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Set pixmap usage. */
    rc = screen_set_pixmap_property_iv(*Pixmap, SCREEN_PROPERTY_USAGE, &screen_usage);
    if (rc)
    {
        fprintf(stderr, "screen_set_pixmap_property_iv(SCREEN_PROPERTY_USAGE) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_pixmap(*Pixmap);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Resize the pixmap. */
    size[0] = Width;
    size[1] = Height;

    rc = screen_set_pixmap_property_iv(*Pixmap, SCREEN_PROPERTY_BUFFER_SIZE, size);
    if (rc)
    {
        fprintf(stderr, "screen_set_pixmap_property_iv(SCREEN_PROPERTY_BUFFER_SIZE) failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_pixmap(*Pixmap);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    /* Create pixmap buffer. */
    rc = screen_create_pixmap_buffer(*Pixmap);
    if (rc)
    {
        fprintf(stderr, "screen_create_pixmap_buffer failed with error %d (0x%08x)\n", errno, errno);
        screen_destroy_pixmap(*Pixmap);
        status = gcvSTATUS_OUT_OF_RESOURCES;
        gcmFOOTER();
        return status;
    }

    gcmFOOTER_ARG("*Pixmap=0x%x", *Pixmap);
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
    gctINT rc, size[2], format;
    screen_buffer_t buf[2];
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Display=0x%x Pixmap=0x%x", Display, Pixmap);

    if (Pixmap == gcvNULL)
    {
        /* Pixmap is not a valid pixmap data structure pointer. */
        goto OnError;
    }

    if ((Width != gcvNULL) || (Height != gcvNULL))
    {
        rc = screen_get_pixmap_property_iv(Pixmap, SCREEN_PROPERTY_BUFFER_SIZE, size);
        if (rc)
        {
            goto OnError;
        }

        if (Width != gcvNULL)
        {
            *Width = size[0];
        }

        if (Height != gcvNULL)
        {
            *Height = size[1];
        }
    }

    if (BitsPerPixel != gcvNULL)
    {
        rc = screen_get_pixmap_property_iv(Pixmap, SCREEN_PROPERTY_FORMAT, &format);
        if (rc)
        {
            goto OnError;
        }

        _ConvertWindowFormat(format, BitsPerPixel, gcvNULL);
    }

    rc = screen_get_pixmap_property_pv(Pixmap, SCREEN_PROPERTY_RENDER_BUFFERS, (void **) &buf);
    if (rc)
    {
        goto OnError;
    }

    if (Stride != NULL)
    {
        rc = screen_get_buffer_property_iv(buf[0], SCREEN_PROPERTY_STRIDE, (int *) Stride);
        if (rc)
        {
            goto OnError;
        }
    }


    if (Bits != NULL)
    {
        rc = screen_get_buffer_property_pv(buf[0], SCREEN_PROPERTY_POINTER, (void **) Bits);
        if (rc)
        {
            goto OnError;
        }
    }

    gcmFOOTER_NO();
    return status;

OnError:
    status = gcvSTATUS_INVALID_ARGUMENT;
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoOS_DestroyPixmap(
    IN HALNativeDisplayType Display,
    IN HALNativePixmapType Pixmap
    )
{
    if (Pixmap != gcvNULL)
    {
        screen_destroy_pixmap(Pixmap);
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
    gceSTATUS status = gcvSTATUS_OK;
    int        rc;

    *Handle = dlopen("libEGL.so.1", RTLD_NOW);
    if (*Handle == gcvNULL)
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    rc = screen_create_context(&screen_ctx, 0);
    if (rc)
    {
        fprintf(stderr, "screen_create_context failed with error %d (0x%08x)\n", errno, errno);
        status = gcvSTATUS_NOT_SUPPORTED;
    }

    return status;
}

gceSTATUS
gcoOS_FreeEGLLibrary(
    IN gctHANDLE Handle
    )
{
    if (screen_ctx)
    {
        screen_destroy_context(screen_ctx);
        screen_ctx = gcvNULL;
    }

    return gcoOS_FreeLibrary(gcvNULL, Handle);
}

gceSTATUS
gcoOS_ShowWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_HideWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_SetWindowTitle(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctCONST_STRING Title
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_CapturePointer(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_GetEvent(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    OUT halEvent * Event
    )
{
    screen_event_t screen_evt;
    int rc;
    gceSTATUS status = gcvSTATUS_NOT_FOUND;

    rc = screen_create_event(&screen_evt);

    while (screen_get_event(screen_ctx, screen_evt, 0L) == 0)
    {
        int type;

        screen_get_event_property_iv(screen_evt, SCREEN_PROPERTY_TYPE, &type);

        if (type ==  SCREEN_EVENT_CLOSE)
        {
            Event->type = HAL_CLOSE;

            status = gcvSTATUS_OK;
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
                Event->type = HAL_BUTTON;
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
                Event->type = HAL_POINTER;
                Event->data.pointer.x = pointer[0];
                Event->data.pointer.y = pointer[1];
            }

            status = gcvSTATUS_OK;
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

            if (scancode == HAL_UNKNOWN)
            {
                continue;
            }

            Event->type                   = HAL_KEYBOARD;
            Event->data.keyboard.scancode = scancode;
            Event->data.keyboard.pressed  = buffer < 0x80;
            Event->data.keyboard.key      = (  (scancode < HAL_SPACE)
                || (scancode >= HAL_F1)
                )
                ? 0
                : (char) scancode;
            status = gcvSTATUS_OK;
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

    return status;
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

