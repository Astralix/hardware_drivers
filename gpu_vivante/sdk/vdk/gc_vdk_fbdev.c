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




/* Enable sigaction declarations. */
#if defined(LINUX)
#   define _XOPEN_SOURCE 501
#endif

#include "gc_vdk.h"
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

/*******************************************************************************
***** Version Signature *******************************************************/

const char * _VDK_PLATFORM = "\n\0$PLATFORM$FBDev$\n";
int ignore;

/*******************************************************************************
** VDK private data structure. *************************************************
*/
struct _vdkPrivate
{
    char            name[11];
    int             uid, gid;
    int             active;
    int             file;
    int             mice;
    int             mode;
    struct termios  tty;
    vdkKeyMap *     map;
    vdkDisplay      display;
    void *          egl;
};

/*******************************************************************************
** Default keyboard map. *******************************************************
*/
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
    /* For TTC Board only */
    /* 66 */ { VDK_HOME,            VDK_UNKNOWN     },
    /* 67 */ { VDK_UP,              VDK_UNKNOWN     },
    /* 68 */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 69 */ { VDK_LEFT,            VDK_UNKNOWN     },
    /* 6A */ { VDK_RIGHT,           VDK_UNKNOWN     },
    /* 6B */ { VDK_ESCAPE,          VDK_UNKNOWN     },
    /* 6C */ { VDK_DOWN,            VDK_UNKNOWN     },
    /* 6D */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6E */ { VDK_UNKNOWN,         VDK_UNKNOWN     },
    /* 6F */ { VDK_BACKSPACE,       VDK_UNKNOWN     },
    /* End */
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
** Private functions.
*/

static vdkPrivate _vdk     = NULL;
static vdkDisplay _display = NULL;
static int _display_alive  = 0;
static vdkWindow  _window  = NULL;
static vdkPixmap  _pixmap  = NULL;

#if defined(LINUX)
int hookSEGV = 0;
static void sig_handler(int signo)
{
    if(hookSEGV == 0)
    {
        signal(SIGSEGV, sig_handler);
        hookSEGV = 1;
    }
    if (_window != NULL)
    {
        /* Hide the window. */
        vdkHideWindow(_window);

        /* Destroy the window. */
        vdkDestroyWindow(_window);

        _window = NULL;
    }

    if (_pixmap != NULL)
    {
        /* Destroy the pixmap. */
        vdkDestroyPixmap(_pixmap);

        _pixmap = NULL;
    }

    if (_display != NULL)
    {
        /* Destroy the display. */
        vdkDestroyDisplay(_display);

        _display = NULL;
        _display_alive = 0;
    }

    if (_vdk != NULL)
    {
        /* Destroy the VDK. */
        vdkExit(_vdk);

        _vdk = NULL;
    }
    exit(0);
}
#endif
static void
vdkOnExit(
    void
    )
{
#if defined(LINUX)
    signal(SIGINT,  SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
#endif

    if (_window != NULL)
    {
        /* Hide the window. */
        vdkHideWindow(_window);

        /* Destroy the window. */
        vdkDestroyWindow(_window);

        _window = NULL;
    }

    if (_pixmap != NULL)
    {
        /* Destroy the pixmap. */
        vdkDestroyPixmap(_pixmap);

        _pixmap = NULL;
    }

    if (_display != NULL)
    {
        /* Destroy the display. */
        vdkDestroyDisplay(_display);

        _display = NULL;
        _display_alive = 0;
    }

    if (_vdk != NULL)
    {
        /* Destroy the VDK. */
        vdkExit(_vdk);

        _vdk = NULL;
    }
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

    do
    {
        atexit(vdkOnExit);

        /* Allocate the private data structure. */
        vdk = (vdkPrivate) malloc(sizeof(struct _vdkPrivate));

        if (vdk == NULL)
        {
            /* Break if we are out of memory. */
            break;
        }

        /* Initialize the private data structure. */
        vdk->file    = -1;
        vdk->mice    = -1;
        vdk->map     = keys;
        vdk->display = NULL;

#if gcdSTATIC_LINK
        vdk->egl = NULL;
#else
#if defined(ANDROID)
        vdk->egl = dlopen("libEGL_VIVANTE.so", RTLD_NOW);
#else
        vdk->egl = dlopen("libEGL.so", RTLD_NOW);
#endif
#endif

        _vdk = vdk;

#if defined(LINUX)

        signal(SIGINT,  sig_handler);
        signal(SIGQUIT, sig_handler);
        signal(SIGTERM, sig_handler);

#endif

        return vdk;
    }
    while (0);

    return NULL;
}

VDKAPI void VDKLANG
vdkExit(
    vdkPrivate Private
    )
{
    if (Private != NULL)
    {
        if (Private->egl != NULL)
        {
            void (*fini)(void);
            int *fptr;
            fptr = (int *)dlsym(Private->egl, "__fini");
            fini = *((void (**)(void))&fptr);

            if (fini != NULL)
            {
                fini();
            }

            dlclose(Private->egl);
        }

        if (Private->file > 0)
        {
            ioctl(Private->file, KDSKBMODE, Private->mode);
            tcsetattr(Private->file, TCSANOW, &Private->tty);

            ioctl(Private->file, KDSETMODE, KD_TEXT);

            if (Private->active != -1)
            {
                ioctl(Private->file, VT_ACTIVATE, Private->active);
                ioctl(Private->file, VT_WAITACTIVE, Private->active);
            }

            close(Private->file);

            if (Private->uid != -1)
            {
                ignore = chown(Private->name, Private->uid, Private->gid);
            }
        }

        if (Private->mice > 0)
        {
            close(Private->mice);
        }

        free(Private);

        if (_vdk == Private) _vdk = NULL;
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
    /* Success. */
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
    /* Success. */
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
    /* Success. */
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

VDKAPI int VDKLANG
vdkShowWindow(
    vdkWindow Window
    )
{
    vdkPrivate vdk;
    int i, fd, term = -1;
    struct stat st;
    struct vt_stat v;
    struct termios t;

    if (Window == NULL)
    {
        return 0;
    }

    vdk = _vdk;

    if (vdk->file != -1)
    {
        return 1;
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

            sprintf(vdk->name, dev[i], term);
            vdk->file = open(vdk->name, O_RDWR | O_NONBLOCK, 0);

            if (vdk->file >= 0)
            {
                break;
            }
        }

        if (vdk->file < 0)
        {
            break;
        }

        if (stat(vdk->name, &st) == 0)
        {
            vdk->uid = st.st_uid;
            vdk->gid = st.st_gid;

            ignore = chown(vdk->name, getuid(), getgid());
        }
        else
        {
            vdk->uid = -1;
        }

        if (ioctl(vdk->file, VT_GETSTATE, &v) >= 0)
        {
            vdk->active = v.v_active;
        }

        ioctl(vdk->file, VT_ACTIVATE, term);
        ioctl(vdk->file, VT_WAITACTIVE, term);
        ioctl(vdk->file, KDSETMODE, KD_GRAPHICS);

        ioctl(vdk->file, KDGKBMODE, &vdk->mode);
        tcgetattr(vdk->file, &vdk->tty);

        ioctl(vdk->file, KDSKBMODE, K_RAW);

        t = vdk->tty;
        t.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
        t.c_oflag = 0;
        t.c_cflag = CREAD | CS8;
        t.c_lflag = 0;
        t.c_cc[VTIME] = 0;
        t.c_cc[VMIN] = 1;
        tcsetattr(vdk->file, TCSANOW, &t);

        if (vdk->mice == -1)
        {
            vdk->mice = open("/dev/input/mice", O_RDONLY | O_NONBLOCK, 0);
        }

        return 1;
    }
    while (0);

    if (fd >= 0)
    {
        close(fd);
    }

    return 0;
}

VDKAPI int VDKLANG
vdkHideWindow(
    vdkWindow Window
    )
{
    vdkPrivate vdk;

    if (Window == NULL)
    {
        return 0;
    }

    vdk = _vdk;

    if (vdk->file < 0)
    {
        return 1;
    }

    ioctl(vdk->file, KDSKBMODE, vdk->mode);
    tcsetattr(vdk->file, TCSANOW, &vdk->tty);

    ioctl(vdk->file, KDSETMODE, KD_TEXT);

    if (vdk->active != -1)
    {
        ioctl(vdk->file, VT_ACTIVATE,   vdk->active);
        ioctl(vdk->file, VT_WAITACTIVE, vdk->active);
    }

    close(vdk->file);
    vdk->file = -1;

    if (vdk->uid != -1)
    {
        ignore = chown(vdk->name, vdk->uid, vdk->gid);
    }

    return 1;
}

VDKAPI void VDKLANG
vdkSetWindowTitle(
    vdkWindow Window,
    const char * Title
    )
{
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
    return gcoOS_DrawImage((EGLNativeWindowType)Window, Left, Top, Right, Bottom, Width, Height, BitsPerPixel, Bits);
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
    static int prefix = 0;
    unsigned char buffer;

    signed char mouse[3];
    static int x, y;
    static char left, right, middle;

    if ((Window == NULL) || (Event == NULL))
    {
        return 0;
    }

    if (_vdk->file >= 0)
    {
        while (read(_vdk->file, &buffer, 1) == 1)
        {
            vdkKeys scancode;

            if ((buffer == 0xE0) || (buffer == 0xE1))
            {
                prefix = buffer;
                continue;
            }

            if (prefix)
            {
                scancode = _vdk->map[buffer & 0x7F].extended;
                prefix = 0;
            }
            else
            {
                scancode = _vdk->map[buffer & 0x7F].normal;
            }

            if (scancode == VDK_UNKNOWN)
            {
                continue;
            }

            Event->type              = VDK_KEYBOARD;
            Event->data.keyboard.scancode = scancode;
            Event->data.keyboard.pressed  = (buffer < 0x80);
            Event->data.keyboard.key      = (  (scancode < VDK_SPACE)
                || (scancode >= VDK_F1)
                )
                ? 0
                : (char) scancode;
            return 1;
        }
    }

    if (_vdk->mice >= 0)
    {
        if (read(_vdk->mice, mouse, 3) == 3)
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
                Event->type                 = VDK_BUTTON;
                Event->data.button.left     = left      = l;
                Event->data.button.right    = right     = r;
                Event->data.button.middle   = middle    = m;
                Event->data.button.x        = x;
                Event->data.button.y        = y;
            }
            else
            {
                Event->type                 = VDK_POINTER;
                Event->data.pointer.x       = x;
                Event->data.pointer.y       = y;
            }

            return 1;
        }
    }

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
    int *fptr;
    if ((Private == NULL) || (Private->egl == NULL))
    {
        return NULL;
    }
    fptr = (int *)dlsym(Private->egl, Function);
    return *((EGL_ADDRESS *)&fptr);
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
    return gcoOS_GetPixmapInfo((EGLNativePixmapType)Pixmap, Width, Height, BitsPerPixel, Stride, Bits);
}

VDKAPI void VDKLANG
vdkDestroyPixmap(
    vdkPixmap Pixmap
    )
{
    if (Pixmap == NULL)
    {
        return;
    }
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
