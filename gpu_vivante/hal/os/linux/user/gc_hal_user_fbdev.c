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
#ifdef EGL_API_FB
/* Enable sigaction declarations. */
#if defined(LINUX)
#if !defined _XOPEN_SOURCE
#   define _XOPEN_SOURCE 501
#endif
#endif
#include "gc_hal_user_linux.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <linux/vt.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "gc_hal_eglplatform.h"

#define _GC_OBJ_ZONE    gcvZONE_OS

/* Structure that defines a display. */
struct _FBDisplay
{
    gctINT                         file;
    gctSIZE_T               physical;
    gctINT                         stride;
    gctINT                         width;
    gctINT                         height;
    gctINT                         bpp;
    gctINT                         size;
    gctPOINTER                      memory;
    struct fb_fix_screeninfo    fixInfo;
    struct fb_var_screeninfo    varInfo;
    struct fb_var_screeninfo    orgVarInfo;
    gctINT                         backBufferY;
    gctINT                         multiBuffer;
    pthread_cond_t              cond;
    pthread_mutex_t             condMutex;
    gctUINT                alphaLength;
    gctUINT                alphaOffset;
    gctUINT                redLength;
    gctUINT                redOffset;
    gctUINT                greenLength;
    gctUINT                greenOffset;
    gctUINT                blueLength;
    gctUINT                blueOffset;
    gceSURF_FORMAT      format;
};

/* Structure that defines a window. */
struct _FBWindow
{
    struct _FBDisplay*      display;
    gctUINT    offset;
    gctINT             x, y;
    gctINT             width;
    gctINT             height;
    /* Color format. */
    gceSURF_FORMAT      format;
};

/* Structure that defines a pixmap. */
struct _FBPixmap
{
    /* Pointer to memory bits. */
    gctPOINTER       bits;

    /* Bits per pixel. */
    gctINT         bpp;

    /* Size. */
    gctINT         width, height;
    gctINT         stride;
};

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
    /* For TTC Board only */
    /* 66 */ { HAL_HOME,            HAL_UNKNOWN     },
    /* 67 */ { HAL_UP,              HAL_UNKNOWN     },
    /* 68 */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 69 */ { HAL_LEFT,            HAL_UNKNOWN     },
    /* 6A */ { HAL_RIGHT,           HAL_UNKNOWN     },
    /* 6B */ { HAL_ESCAPE,          HAL_UNKNOWN     },
    /* 6C */ { HAL_DOWN,            HAL_UNKNOWN     },
    /* 6D */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6E */ { HAL_UNKNOWN,         HAL_UNKNOWN     },
    /* 6F */ { HAL_BACKSPACE,       HAL_UNKNOWN     },
    /* End */
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

gctCHAR            name[11];
gctINT             uid, gid;
gctINT             active;
gctINT             file = -1;
gctINT             mice = -1;
gctINT             mode;
struct termios  tty;
gctINT ignore;
gctINT hookSEGV = 0;
#if defined(LINUX)
static void sig_handler(gctINT signo)
{
    if(hookSEGV == 0)
    {
        signal(SIGSEGV, sig_handler);
        hookSEGV = 1;
    }
    gcoOS_FreeEGLLibrary(gcvNULL);
    exit(0);
}
#endif

static void
halOnExit(
    void
    )
{
#if defined(LINUX)
    signal(SIGINT,  SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
#endif
    gcoOS_FreeEGLLibrary(gcvNULL);
}


/*******************************************************************************
** Display. ********************************************************************
*/

gceSTATUS
gcoOS_GetDisplay(
    OUT HALNativeDisplayType * Display
    )
{
    return gcoOS_GetDisplayByIndex(0, Display);
}

gceSTATUS
gcoOS_GetDisplayByIndex(
    IN gctINT DisplayIndex,
    OUT HALNativeDisplayType * Display
    )
{
    char *dev, *p;
    struct _FBDisplay* display;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER();

    do
    {
        if((DisplayIndex > 3) || (DisplayIndex < 0))
            return gcvSTATUS_INVALID_ARGUMENT;
        display = (struct _FBDisplay*) malloc(sizeof(struct _FBDisplay));

        if (display == gcvNULL)
        {
            break;
        }

        display->memory   = gcvNULL;
        display->file     = -1;

        p = getenv("FB_MULTI_BUFFER");
        if (p == NULL)
        {
            display->multiBuffer = 0;
        }
        else
        {
            display->multiBuffer = atoi(p);
        }
        switch(DisplayIndex)
        {
        case 0:
            dev = getenv("FB_FRAMEBUFFER_0");
            if (dev != gcvNULL)
            {
                display->file = open(dev, O_RDWR);
            }

            if (display->file < 0)
            {
                unsigned char i = 0;
                char const * const GALDeviceName[] =
                {
                    "/dev/fb0",
                    "/dev/graphics/fb0",
                    gcvNULL
                };

                /* Create a handle to the device. */
                while ((display->file == -1) && GALDeviceName[i])
                {
                    display->file = open(GALDeviceName[i], O_RDWR);
                    i++;
                }
            }
            break;
        case 1:
            dev = getenv("FB_FRAMEBUFFER_1");
            if (dev != gcvNULL)
            {
                display->file = open(dev, O_RDWR);
            }

            if (display->file < 0)
            {
                unsigned char i = 0;
                char const * const GALDeviceName[] =
                {
                    "/dev/fb1",
                    "/dev/graphics/fb1",
                    gcvNULL
                };

                /* Create a handle to the device. */
                while ((display->file == -1) && GALDeviceName[i])
                {
                    display->file = open(GALDeviceName[i], O_RDWR);
                    i++;
                }
            }
            break;
        case 2:
            dev = getenv("FB_FRAMEBUFFER_2");
            if (dev != gcvNULL)
            {
                display->file = open(dev, O_RDWR);
            }

            if (display->file < 0)
            {
                unsigned char i = 0;
                char const * const GALDeviceName[] =
                {
                    "/dev/fb2",
                    "/dev/graphics/fb2",
                    gcvNULL
                };

                /* Create a handle to the device. */
                while ((display->file == -1) && GALDeviceName[i])
                {
                    display->file = open(GALDeviceName[i], O_RDWR);
                    i++;
                }
            }
            break;
        case 3:
            dev = getenv("FB_FRAMEBUFFER_3");
            if (dev != gcvNULL)
            {
                display->file = open(dev, O_RDWR);
            }

            if (display->file < 0)
            {
                unsigned char i = 0;
                char const * const GALDeviceName[] =
                {
                    "/dev/fb3",
                    "/dev/graphics/fb3",
                    gcvNULL
                };

                /* Create a handle to the device. */
                while ((display->file == -1) && GALDeviceName[i])
                {
                    display->file = open(GALDeviceName[i], O_RDWR);
                    i++;
                }
            }
            break;
        default:
            break;
        }

        if (display->file < 0)
        {
            break;
        }

        if (ioctl(display->file, FBIOGET_FSCREENINFO, &(display->fixInfo)) < 0)
        {
            break;
        }
        display->physical = display->fixInfo.smem_start;
        display->stride   = display->fixInfo.line_length;
        display->size     = display->fixInfo.smem_len;

        if (ioctl(display->file, FBIOGET_VSCREENINFO, &(display->varInfo)) < 0)
        {
            break;
        }

        display->orgVarInfo = display->varInfo;

        display->width  = display->varInfo.xres;
        display->height = display->varInfo.yres;
        display->bpp    = display->varInfo.bits_per_pixel;

        /* Get the color format. */
        switch (display->varInfo.green.length)
        {
        case 4:
            if (display->varInfo.blue.offset == 0)
            {
                display->format = (display->varInfo.transp.length == 0) ? gcvSURF_X4R4G4B4 : gcvSURF_A4R4G4B4;
            }
            else
            {
                display->format = (display->varInfo.transp.length == 0) ? gcvSURF_X4B4G4R4 : gcvSURF_A4B4G4R4;
            }
            break;

        case 5:
            if (display->varInfo.blue.offset == 0)
            {
                display->format = (display->varInfo.transp.length == 0) ? gcvSURF_X1R5G5B5 : gcvSURF_A1R5G5B5;
            }
            else
            {
                display->format = (display->varInfo.transp.length == 0) ? gcvSURF_X1B5G5R5 : gcvSURF_A1B5G5R5;
            }
            break;

        case 6:
            display->format = gcvSURF_R5G6B5;
            break;

        case 8:
            if (display->varInfo.blue.offset == 0)
            {
                display->format = (display->varInfo.transp.length == 0) ? gcvSURF_X8R8G8B8 : gcvSURF_A8R8G8B8;
            }
            else
            {
                display->format = (display->varInfo.transp.length == 0) ? gcvSURF_X8B8G8R8 : gcvSURF_A8B8G8R8;
            }
            break;

        default:
            display->format = gcvSURF_UNKNOWN;
            break;
        }

        /* Get the color info. */
        display->alphaLength = display->varInfo.transp.length;
        display->alphaOffset = display->varInfo.transp.offset;

        display->redLength   = display->varInfo.red.length;
        display->redOffset   = display->varInfo.red.offset;

        display->greenLength = display->varInfo.green.length;
        display->greenOffset = display->varInfo.green.offset;

        display->blueLength  = display->varInfo.blue.length;
        display->blueOffset  = display->varInfo.blue.offset;





        display->memory = mmap(0,
                               display->size,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED,
                               display->file,
                               0);

        if (display->memory == MAP_FAILED)
        {
            break;
        }

        pthread_cond_init(&(display->cond), gcvNULL);
        pthread_mutex_init(&(display->condMutex), gcvNULL);
        *Display = (HALNativeDisplayType)display;
        gcmFOOTER_ARG("*Display=0x%x", *Display);
        return status;
    }
    while (0);

    if (display != gcvNULL)
    {
        if (display->memory != gcvNULL)
        {
            munmap(display->memory, display->size);
        }

        if (display->file >= 0)
        {
            ioctl(display->file, FBIOPUT_VSCREENINFO, &(display->orgVarInfo));
            close(display->file);
        }

        free(display);
        display = gcvNULL;
        *Display = gcvNULL;
    }
    status = gcvSTATUS_OUT_OF_RESOURCES;
    gcmFOOTER();
    return status;
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
    struct _FBDisplay* display;
    gcmHEADER_ARG("Display=0x%x", Display);
    display = (struct _FBDisplay*)Display;
    if (display == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Width != gcvNULL)
    {
        *Width = display->width;
    }

    if (Height != gcvNULL)
    {
        *Height = display->height;
    }

    if (Physical != gcvNULL)
    {
#if USE_SW_FB
        *Physical = ~0;
#else
        *Physical = display->physical;
#endif
    }

    if (Stride != gcvNULL)
    {
        *Stride = display->stride;
    }

    if (BitsPerPixel != gcvNULL)
    {
        *BitsPerPixel = display->bpp;
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
    gceSTATUS status = gcvSTATUS_OK;
    struct _FBDisplay* display;
    gcmHEADER_ARG("Display=0x%x Window=0x%x DisplayInfoSize=%u", Display, Window, DisplayInfoSize);
    display = (struct _FBDisplay*)Display;
    /* Valid display? and structure size? */
    if ((display == gcvNULL) || (DisplayInfoSize != sizeof(halDISPLAY_INFO)))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

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
    DisplayInfo->alphaLength = display->alphaLength;
    DisplayInfo->alphaOffset = display->alphaOffset;
    DisplayInfo->redLength   = display->redLength;
    DisplayInfo->redOffset   = display->redOffset;
    DisplayInfo->greenLength = display->greenLength;
    DisplayInfo->greenOffset = display->greenOffset;
    DisplayInfo->blueLength  = display->blueLength;
    DisplayInfo->blueOffset  = display->blueOffset;

    /* Determine flip support. */
    DisplayInfo->flip = (display->multiBuffer > 1);

#ifndef __QNXNTO__
    /* 355_FB_MULTI_BUFFER */
    DisplayInfo->multiBuffer = Display->multiBuffer;
    DisplayInfo->backBufferY = Display->backBufferY;
#endif

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
    gctINT i;
    gceSTATUS status = gcvSTATUS_OK;
    struct _FBDisplay* display;
    gcmHEADER_ARG("Display=0x%x", Display);
    display = (struct _FBDisplay*)Display;
    if (display == gcvNULL)
    {
        /* Bad display pointer. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (display->multiBuffer <= 1)
    {
        /* Turned off. */
        status = gcvSTATUS_NOT_SUPPORTED;
        gcmFOOTER_NO();
        return status;
    }

    for (i = display->multiBuffer; i > 1; --i)
    {
        /* Try setting up multi buffering. */
        display->varInfo.yres_virtual = display->height * i;
        if (ioctl(display->file, FBIOPUT_VSCREENINFO, &(display->varInfo)) >= 0)
        {
            break;
        }
    }

    /* Get current virtual screen size. */
    ioctl(display->file, FBIOGET_VSCREENINFO, &(display->varInfo));

    /* Compute number of buffers. */
#ifndef __QNXNTO__
    /* 355_FB_MULTI_BUFFER */
    Display->multiBuffer = i;
    Display->varInfo.yres_virtual = Display->height * i;
#else
    display->multiBuffer = display->varInfo.yres_virtual / display->height;
#endif

    /* Move to off-screen memory. */
    display->backBufferY = display->varInfo.yoffset + display->height;
    if (display->backBufferY >= (int)(display->varInfo.yres_virtual))
    {
        /* Warp around. */
        display->backBufferY = 0;
    }

    /* Save size of virtual buffer. */
    *Width  = display->varInfo.xres_virtual;
    *Height = display->varInfo.yres_virtual;

    /* Success. */
    gcmFOOTER_ARG("*Width=%d *Height=%d", *Width, *Height);
    return status;
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
    gceSTATUS status = gcvSTATUS_OK;
    struct _FBDisplay* display;
    gcmHEADER_ARG("Display=0x%x", Display);
    display = (struct _FBDisplay*)Display;
    if (display == gcvNULL)
    {
        /* Invalid display pointer. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (display->multiBuffer <= 1)
    {
        /* Turned off. */
        status = gcvSTATUS_NOT_SUPPORTED;
        gcmFOOTER_NO();
        return status;
    }

    pthread_mutex_lock(&(display->condMutex));

    /* Return current back buffer. */
    *X = 0;
    *Y = display->backBufferY;

    while (display->backBufferY == (volatile int) (display->varInfo.yoffset))
    {
        pthread_cond_wait(&(display->cond), &(display->condMutex));
    }

    /* Move to next back buffer. */
    display->backBufferY += display->height;
    if (display->backBufferY >= (int)(display->varInfo.yres_virtual))
    {
        /* Wrap around. */
        display->backBufferY = 0;
    }

    pthread_mutex_unlock(&(display->condMutex));

    /* Success. */
    gcmFOOTER_ARG("*Offset=%u *X=%d *Y=%d", *Offset, *X, *Y);
    return status;
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
    gceSTATUS status = gcvSTATUS_OK;
    struct _FBDisplay* display;
    gcmHEADER_ARG("Display=0x%x Window=0x%x Offset=%u X=%d Y=%d", Display, Window, Offset, X, Y);
    display = (struct _FBDisplay*)Display;
    if (display == gcvNULL)
    {
        /* Invalid display pointer. */
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (display->multiBuffer > 1)
    {
        pthread_mutex_lock(&(display->condMutex));

        /* Set display offset. */
        display->varInfo.xoffset  = X;
        display->varInfo.yoffset  = Y;
        display->varInfo.activate = FB_ACTIVATE_VBL;
        ioctl(display->file, FBIOPAN_DISPLAY, &(display->varInfo));

        pthread_cond_broadcast(&(display->cond));
        pthread_mutex_unlock(&(display->condMutex));
    }

    /* Success. */
    gcmFOOTER_NO();
    return status;
}

gceSTATUS
gcoOS_DestroyDisplay(
    IN HALNativeDisplayType Display
    )
{
    struct _FBDisplay* display;
    display = (struct _FBDisplay*)Display;
    if (display != NULL)
    {
        if (display->memory != NULL)
        {
            munmap(display->memory, display->size);
            display->memory = NULL;
        }

        ioctl(display->file, FBIOPUT_VSCREENINFO, &(display->orgVarInfo));

        if (display->file >= 0)
        {
            close(display->file);
            display->file = -1;
        }

        pthread_mutex_destroy(&(display->condMutex));
        pthread_cond_destroy(&(display->cond));

        free(display);
        display = gcvNULL;
        Display = gcvNULL;
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
    gceSTATUS status = gcvSTATUS_OK;
    struct _FBDisplay* display;
    struct _FBWindow* window;
    gcmHEADER_ARG("Display=0x%x X=%d Y=%d Width=%d Height=%d", Display, X, Y, Width, Height);
    display = (struct _FBDisplay*)Display;
    if (display == NULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

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
        X = ((display->width - Width) / 2) & ~15;
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
        window = (struct _FBWindow*) malloc(sizeof(struct _FBWindow));

        if (window == NULL)
        {
            status = gcvSTATUS_OUT_OF_RESOURCES;
            break;
        }

        window->offset = Y * display->stride + X * ((display->bpp + 7) / 8);

        window->display = display;
        window->format = display->format;
        window->x = X;
        window->y = Y;

        window->width  = Width;
        window->height = Height;
        *Window = (HALNativeWindowType)window;
    }
    while (0);

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
    OUT gctUINT * Offset
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    struct _FBWindow* window;
    gcmHEADER_ARG("Display=0x%x Window=0x%x", Display, Window);
    window = (struct _FBWindow*)Window;
    if (window == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (X != gcvNULL)
    {
            *X = window->x;
    }

    if (Y != gcvNULL)
    {
            *Y = window->y;
    }

    if (Width != gcvNULL)
    {
        *Width = window->width;
    }

    if (Height != gcvNULL)
    {
        *Height = window->height;
    }

    if (BitsPerPixel != gcvNULL)
    {
        *BitsPerPixel = window->display->bpp;
    }

    if (Offset != gcvNULL)
    {
        *Offset = window->offset;
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
    if (Window != gcvNULL)
    {
        free(Window);
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
    unsigned char * ptr;
    int y;
    unsigned int bytes     = (Right - Left) * BitsPerPixel / 8;
    unsigned char * source = (unsigned char *) Bits;
    struct _FBDisplay* display;
    struct _FBWindow* window;
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Display=0x%x Window=0x%x Left=%d Top=%d Right=%d Bottom=%d Width=%d Height=%d BitsPerPixel=%d Bits=0x%x",
                  Display, Window, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits);
    window = (struct _FBWindow*)Window;
    if ((window == gcvNULL) || (Bits == gcvNULL))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
    else
    {
        display = window->display;
    }

    ptr = (unsigned char *) display->memory + (Left * display->bpp / 8);

    if (BitsPerPixel != display->bpp)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Height < 0)
    {
        for (y = Bottom - 1; y >= Top; --y)
        {
            unsigned char * line = ptr + y * display->stride;
            memcpy(line, source, bytes);

            source += Width * BitsPerPixel / 8;
        }
    }
    else
    {
        for (y = Top; y < Bottom; ++y)
        {
            unsigned char * line = ptr + y * display->stride;
            memcpy(line, source, bytes);

            source += Width * BitsPerPixel / 8;
        }
    }

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
    gceSTATUS status = gcvSTATUS_OK;
    struct _FBPixmap* pixmap;
    gcmHEADER_ARG("Display=0x%x Width=%d Height=%d BitsPerPixel=%d", Display, Width, Height, BitsPerPixel);

    if ((Width <= 0) || (Height <= 0) || (BitsPerPixel <= 0))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    do
    {
        Width   = (Width + 0x0F) & (~0x0F); /* 16 bytes alignment. */
        Height  = (Height+ 0x03) & (~0x03); /*  4 bytes alignment. */

        pixmap = (struct _FBPixmap*) malloc(sizeof (struct _FBPixmap));

        if (pixmap == gcvNULL)
        {
            break;
        }

        pixmap->bits = malloc(Width * Height * (BitsPerPixel + 7) / 8);
        if (pixmap->bits == gcvNULL)
        {
            free(pixmap);
            pixmap = gcvNULL;

            break;
        }

        /*pixmap->display = (struct _FBDisplay*)Display;*/

        pixmap->width = Width;
        pixmap->height = Height;

        pixmap->bpp     = (BitsPerPixel + 0x07) & (~0x07);
        pixmap->stride  = Width * (pixmap->bpp) / 8;
        *Pixmap = (HALNativePixmapType)pixmap;
        gcmFOOTER_ARG("*Pixmap=0x%x", *Pixmap);
        return status;
    }
    while (0);

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
    gceSTATUS status = gcvSTATUS_OK;
    struct _FBPixmap* pixmap;
    gcmHEADER_ARG("Display=0x%x Pixmap=0x%x", Display, Pixmap);
    pixmap = (struct _FBPixmap*)Pixmap;
    if (pixmap == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Width != NULL)
    {
        *Width = pixmap->width;
    }

    if (Height != NULL)
    {
        *Height = pixmap->height;
    }

    if (BitsPerPixel != NULL)
    {
        *BitsPerPixel = pixmap->bpp;
    }

    if (Stride != NULL)
    {
        *Stride = pixmap->stride;
    }

    if (Bits != NULL)
    {
        *Bits = pixmap->bits;
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
    struct _FBPixmap* pixmap;
    pixmap = (struct _FBPixmap*)Pixmap;
    if (pixmap != gcvNULL)
    {
        if (pixmap->bits != NULL)
        {
            free(pixmap->bits);
        }
        free(pixmap);
        pixmap = gcvNULL;
        Pixmap = gcvNULL;
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
    atexit(halOnExit);

    /* Initialize the global data structure. */
    file    = -1;
    mice    = -1;


#if defined(ANDROID)
    status = gcoOS_LoadLibrary(gcvNULL, "libEGL_VIVANTE.so", Handle);
#else
    status = gcoOS_LoadLibrary(gcvNULL, "libEGL.so", Handle);
#endif

#if defined(LINUX)
    signal(SIGINT,  sig_handler);
    signal(SIGQUIT, sig_handler);
    signal(SIGTERM, sig_handler);
#endif

    return status;
}

gceSTATUS
gcoOS_FreeEGLLibrary(
    IN gctHANDLE Handle
    )
{
    if (Handle != gcvNULL)
    {
        void (*fini)(void);
        int *fptr;
        fptr = (int *)dlsym(Handle, "__fini");
        fini = *((void (**)(void))&fptr);

        if (fini != gcvNULL)
        {
            fini();
        }

        gcoOS_FreeLibrary(gcvNULL, Handle);
    }
    if (file > 0)
    {
        ioctl(file, KDSKBMODE, mode);
        tcsetattr(file, TCSANOW, &tty);

        ioctl(file, KDSETMODE, KD_TEXT);

        if (active != -1)
        {
            ioctl(file, VT_ACTIVATE, active);
            ioctl(file, VT_WAITACTIVE, active);
        }

        close(file);

        if (uid != -1)
        {
            ignore = chown(name, uid, gid);
        }
    }

    if (mice > 0)
    {
        close(mice);
    }
    file    = -1;
    mice    = -1;
    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_ShowWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    int i, fd, term = -1;
    struct stat st;
    struct vt_stat v;
    struct termios t;

    if (file != -1)
    {
        return gcvSTATUS_OK;
    }

    do
    {
        /* Try opening the TTY device. */
        for (i = 0; i < 2; ++i)
        {
            static const char * dev[] = { "/dev/tty0", "/dev/vc/0" };

            /* Open the TTY device. */
            fd = open(dev[i], O_RDWR, 0);

            if (fd >= 0)
            {
                /* Break on success. */
                break;
            }
        }

        if (fd < 0)
        {
            /* Break when TTY device cannot be opened. */
            break;
        }

        if ((ioctl(fd, VT_OPENQRY, &term) < 0) || (term == -1))
        {
            break;
        }

        close(fd);
        fd = -1;

        for (i = 0; i < 2; ++i)
        {
            static const char * dev[] = { "/dev/tty%d", "/dev/vc/%d" };

            sprintf(name, dev[i], term);
            file = open(name, O_RDWR | O_NONBLOCK, 0);

            if (file >= 0)
            {
                break;
            }
        }

        if (file < 0)
        {
            break;
        }

        if (stat(name, &st) == 0)
        {
            uid = st.st_uid;
            gid = st.st_gid;

            ignore = chown(name, getuid(), getgid());
        }
        else
        {
            uid = -1;
        }

        if (ioctl(file, VT_GETSTATE, &v) >= 0)
        {
            active = v.v_active;
        }

        ioctl(file, VT_ACTIVATE, term);
        ioctl(file, VT_WAITACTIVE, term);
        ioctl(file, KDSETMODE, KD_GRAPHICS);

        ioctl(file, KDGKBMODE, &mode);
        tcgetattr(file, &tty);

        ioctl(file, KDSKBMODE, K_RAW);

        t = tty;
        t.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
        t.c_oflag = 0;
        t.c_cflag = CREAD | CS8;
        t.c_lflag = 0;
        t.c_cc[VTIME] = 0;
        t.c_cc[VMIN] = 1;
        tcsetattr(file, TCSANOW, &t);

        if (mice == -1)
        {
            mice = open("/dev/input/mice", O_RDONLY | O_NONBLOCK, 0);
        }

        return gcvSTATUS_OK;
    }
    while (0);

    if (fd >= 0)
    {
        close(fd);
    }

    return gcvSTATUS_NOT_SUPPORTED;
}

gceSTATUS
gcoOS_HideWindow(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window
    )
{
    if (file < 0)
    {
        return gcvSTATUS_OK;
    }

    ioctl(file, KDSKBMODE, mode);
    tcsetattr(file, TCSANOW, &tty);

    ioctl(file, KDSETMODE, KD_TEXT);

    if (active != -1)
    {
        ioctl(file, VT_ACTIVATE,   active);
        ioctl(file, VT_WAITACTIVE, active);
    }

    close(file);
    file = -1;

    if (uid != -1)
    {
        ignore = chown(name, uid, gid);
    }

    return gcvSTATUS_OK;
}

gceSTATUS
gcoOS_SetWindowTitle(
    IN HALNativeDisplayType Display,
    IN HALNativeWindowType Window,
    IN gctCONST_STRING Title
    )
{
    return gcvSTATUS_NOT_SUPPORTED;
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
    static int prefix = 0;
    unsigned char buffer;

    signed char mouse[3];
    static int x, y;
    static char left, right, middle;

    if ((Window == gcvNULL) || (Event == gcvNULL))
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (file >= 0)
    {
        while (read(file, &buffer, 1) == 1)
        {
            halKeys scancode;

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

            Event->type              = HAL_KEYBOARD;
            Event->data.keyboard.scancode = scancode;
            Event->data.keyboard.pressed  = (buffer < 0x80);
            Event->data.keyboard.key      = (  (scancode < HAL_SPACE)
                || (scancode >= HAL_F1)
                )
                ? 0
                : (char) scancode;
            return gcvSTATUS_OK;
        }
    }

    if (mice >= 0)
    {
        if (read(mice, mouse, 3) == 3)
        {
            char l, m, r;

            x += mouse[1];
            y -= mouse[2];

            /*
            x = (x < 0) ? 0 : x;
            x = (x > Window->display->width) ? Window->display->width : x;

            y = (y < 0) ? 0 : y;
            y = (y > Window->display->height) ? Window->display->height : y;
            */

            l = mouse[0] & 0x01;
            r = mouse[0] & 0x02;
            m = mouse[0] & 0x04;

            if ((l ^ left) || (r ^ right) || (m ^ middle))
            {
                Event->type                 = HAL_BUTTON;
                Event->data.button.left     = left      = l;
                Event->data.button.right    = right     = r;
                Event->data.button.middle   = middle    = m;
                Event->data.button.x        = x;
                Event->data.button.y        = y;
            }
            else
            {
                Event->type                 = HAL_POINTER;
                Event->data.pointer.x       = x;
                Event->data.pointer.y       = y;
            }

            return gcvSTATUS_OK;
        }
    }

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
