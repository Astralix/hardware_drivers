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




#include <sys/mman.h>

#include <dlfcn.h>

#include <cutils/ashmem.h>
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <linux/fb.h>

#ifdef USE_OSDM
//  #define GRALLOC_RGB565
  #include <osdm_utils.h>
  #include <hardware/overlay.h>
#endif


#include "gc_gralloc_priv.h"
#include "gc_gralloc_gr.h"

/*
     FRAMEBUFFER_PIXEL_FORMAT

         Set framebuffer pixel format to use for android.
         If it is set to '0', current pixel format is detected and set.

         Available format values are:
             HAL_PIXEL_FORMAT_RGBA_8888 : 1
             HAL_PIXEL_FORMAT_RGBX_8888 : 2
             HAL_PIXEL_FORMAT_RGB_888   : 3
             HAL_PIXEL_FORMAT_RGB_565   : 4
             HAL_PIXEL_FORMAT_BGRA_8888 : 5
 */
#define FRAMEBUFFER_PIXEL_FORMAT  0

/*
     NUM_BUFFERS

         Numbers of buffers of framebuffer device for page flipping.

         Normally it can be '2' for double buffering, or '3' for tripple
         buffering.
         This value should equal to (yres_virtual / yres).
 */
#define NUM_BUFFERS               2


/*
     NUM_PAGES_MMAP

         Numbers of pages to mmap framebuffer to userspace.

         Normally the total buffer size should be rounded up to page boundary
         and mapped to userspace. On some platform, maping the rounded size
         will fail. Set non-zero value to specify the page count to map.
 */
#define NUM_PAGES_MMAP            0


/* Framebuffer pixel format table. */
static const struct
{
    int      format;
    uint32_t bits_per_pixel;
    uint32_t red_offset;
    uint32_t red_length;
    uint32_t green_offset;
    uint32_t green_length;
    uint32_t blue_offset;
    uint32_t blue_length;
    uint32_t transp_offset;
    uint32_t transp_length;
}
formatTable[] =
{
    {HAL_PIXEL_FORMAT_RGBA_8888, 32,  0, 8,  8, 8,  16, 8, 24, 8},
    {HAL_PIXEL_FORMAT_RGBX_8888, 32,  0, 8,  8, 8,  16, 8,  0, 0},
    {HAL_PIXEL_FORMAT_RGB_888,   24,  0, 8,  8, 8,  16, 8,  0, 0},
    {HAL_PIXEL_FORMAT_RGB_565,   16, 11, 5,  5, 6,   0, 5,  0, 0},
    {HAL_PIXEL_FORMAT_BGRA_8888, 32, 16, 8,  8, 8,   0, 8, 24, 8}
};

/* Framebuffer pixel format. */
static int format = FRAMEBUFFER_PIXEL_FORMAT;

struct fb_context_t
{
    framebuffer_device_t device;
};

/*******************************************************************************
**
**  fb_setSwapInterval
**
**  Set framebuffer swap interval.
**
**  INPUT:
**
**      framebuffer_device_t * Dev
**          Specified framebuffer device.
**
**      int Interval
**          Specified Interval value.
**
**  OUTPUT:
**
**      Nothing.
*/
static int
fb_setSwapInterval(
    struct framebuffer_device_t * Dev,
    int Interval
    )
{
    if ((Interval < Dev->minSwapInterval)
    ||  (Interval > Dev->maxSwapInterval)
    )
    {
        return -EINVAL;
    }

    /* FIXME: implement fb_setSwapInterval. */
    return 0;
}

/*******************************************************************************
**
**  fb_post
**
**  Post back buffer to display.
**
**  INPUT:
**
**      framebuffer_device_t * Dev
**          Specified framebuffer device.
**
**      int Buffer
**          Specified back buffer.
**
**  OUTPUT:
**
**      Nothing.
*/
static int
fb_post(
    struct framebuffer_device_t * Dev,
    buffer_handle_t Buffer
    )
{
    if (gc_private_handle_t::validate(Buffer) < 0)
    {
        return -EINVAL;
    }

    gc_private_handle_t const * hnd =
        reinterpret_cast<gc_private_handle_t const *>(Buffer);

    private_module_t * m =
        reinterpret_cast<private_module_t *>(Dev->common.module);

    if (hnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
    {
        const size_t offset = hnd->base - m->framebuffer->base;

#ifdef USE_OSDM
	if (osdm_plane_pan_buffer(m->framebuffer->fd, offset / m->finfo.line_length) < 0) {
		LOGE("osdm_plane_pan_buffer failed");
		m->base.unlock(&m->base, Buffer); 
		return -errno;
	}
#else
        m->info.activate = FB_ACTIVATE_VBL;
        m->info.yoffset  = offset / m->finfo.line_length;

#ifndef FRONT_BUFFER
        if (ioctl(m->framebuffer->fd, FBIOPAN_DISPLAY, &m->info) == -1)
        {
            LOGE("FBIOPAN_DISPLAY failed");

            m->base.unlock(&m->base, Buffer);

            return -errno;
        }
#endif
#endif
        m->currentBuffer = Buffer;
    }
    else
    {
        LOGE("Cannot post this buffer");
    }

    return 0;
}

/*******************************************************************************
**
**  fb_compositionComplete
**
**  Sync with the hardware.
**
**  INPUT:
**
**      framebuffer_device_t * Dev
**          Specified framebuffer device.
**
**  OUTPUT:
**
**      Nothing.
*/


/*******************************************************************************
**
**  mapFrameBufferLocked
**
**  Open framebuffer device and initialize.
**
**  INPUT:
**
**      private_module_t * Module
**          Specified gralloc module.
**
**  OUTPUT:
**
**      Nothing.
*/
int
mapFrameBufferLocked(
    struct private_module_t * Module
    )
{
    /* already initialized... */
    if (Module->framebuffer)
    {
        return 0;
    }

    char const * const device_template[] =
    {
        "/dev/graphics/fb%u",
        "/dev/fb%u",
        0
    };

    int     fd = -1;
    uint32_t i = 0;
    char name[64];

    /* Open framebuffer device. */
    while ((fd == -1) && device_template[i])
    {
#ifdef USE_OSDM
	snprintf(name, 64, OSDM_PLANE_MAIN_GUI);
#else
        snprintf(name, 64, device_template[i], 0);
#endif
        fd = open(name, O_RDWR, 0);
        i++;
    }

    if (fd < 0)
    {
        LOGE("Can not find a valid frambuffer device.");

        return -errno;
    }

    /* Get fix screen info. */
    struct fb_fix_screeninfo finfo;
    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        LOGE("ioctl(FBIOGET_FSCREENINFO) failed.");
        return -errno;
    }

    /* Get variable screen info. */
    struct fb_var_screeninfo info;
#ifdef USE_OSDM
    if (osdm_get_varinfo(fd, &info) != 0)
#else
    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
#endif
    {
        LOGE("ioctl(FBIOGET_FSCREENINFO) failed.");
        return -errno;
    }

#ifdef USE_OSDM
	// find the current screen size
	int screen_width, screen_height;
	if (osdm_get_screen_size(&screen_width, &screen_height) != 0) {
		LOGE("osdm_get_screen_size failed");
		return -errno;
	}
#ifdef GRALLOC_RGB565
	if (osdm_plane_create(fd, 0, 0, screen_width, screen_height, OVERLAY_FORMAT_RGB_565, 0) < 0) {
#else
	if (osdm_plane_create(fd, 0, 0, screen_width, screen_height, OVERLAY_FORMAT_RGBA_8888, 0) < 0) {
#endif
		LOGE("cannot create main gui osdm plane");
		return -errno;
	}

	if (osdm_plane_per_pixel_alpha_enable(fd) < 0) {
		LOGE("cannot enable per pixel alpha\n");
		return -1;
	}

	if (osdm_plane_enable(fd) < 0) {
		LOGE("cannot enable plane fd=%d",fd);
		return -errno;
	}

	if (osdm_get_varinfo(fd, &info) != 0)
		return -errno;
#else

    /* Set pixel format. */
    info.reserved[0]    = 0;
    info.reserved[1]    = 0;
    info.reserved[2]    = 0;
    info.xoffset        = 0;
    info.yoffset        = 0;
    info.activate       = FB_ACTIVATE_NOW;

    if (format > 0)
    {
        /* Find specified pixel format info in table. */
        for (i = 0;
             i < sizeof (formatTable) / sizeof (formatTable[0]);
             i++)
        {
             if (formatTable[i].format == format)
             {
                 /* Set pixel format detail. */
                 info.bits_per_pixel = formatTable[i].bits_per_pixel;
                 info.red.offset     = formatTable[i].red_offset;
                 info.red.length     = formatTable[i].red_length;
                 info.green.offset   = formatTable[i].green_offset;
                 info.green.length   = formatTable[i].green_length;
                 info.blue.offset    = formatTable[i].blue_offset;
                 info.blue.length    = formatTable[i].blue_length;
                 info.transp.offset  = formatTable[i].transp_offset;
                 info.transp.length  = formatTable[i].transp_length;

                 break;
             }
        }

        if (i == sizeof (formatTable) / sizeof (formatTable[0]))
        {
            /* Can not find format info in table. */
            LOGW("Unkown format specified: %d", format);

            /* Set to unknown format. */
            format = 0;
        }
    }

    /* Try default format if specified format not found. */
    if (format == 0)
    {
         /* Find default pixel format id. */
         for (i = 0;
              i < sizeof (formatTable) / sizeof (formatTable[0]);
              i++)
         {
             if ((info.bits_per_pixel == formatTable[i].bits_per_pixel)
             &&  (info.red.offset     == formatTable[i].red_offset)
             &&  (info.red.length     == formatTable[i].red_length)
             &&  (info.green.offset   == formatTable[i].green_offset)
             &&  (info.green.length   == formatTable[i].green_length)
             &&  (info.blue.offset    == formatTable[i].blue_offset)
             &&  (info.blue.length    == formatTable[i].blue_length)
          /* &&  (info.transp.offset  == formatTable[i].transp_offset) */
             &&  (info.transp.length  == formatTable[i].transp_length)
             )
             {
                 format = formatTable[i].format;
                 break;
             }
         }

         if (format == 0)
         {
            /* Can not find format info in table. */
             LOGE("Unknown default pixel format");

             return -EFAULT;
         }

         LOGI("Use default framebuffer pixel format=%d", format);
    }

    /* Request NUM_BUFFERS screens (at least 2 for page flipping) */
    info.yres_virtual = info.yres * NUM_BUFFERS;

    if (ioctl(fd, FBIOPUT_VSCREENINFO, &info) == -1)
    {
        info.yres_virtual = info.yres;

        LOGE("FBIOPUT_VSCREENINFO failed, page flipping not supported");
    }

#ifndef FRONT_BUFFER
    if (info.yres_virtual < info.yres * 2)
    {
        /* we need at least 2 for page-flipping. */
        info.yres_virtual = info.yres;

        LOGW("page flipping not supported "
             "(yres_virtual=%d, requested=%d)",
             info.yres_virtual,
             info.yres * 2);
    }
#endif

    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
    {
        LOGE("ioctl(FBIOGET_VSCREENINFO) failed.");

        return -errno;
    }
#endif //USE_OSDM

    uint64_t  refreshQuotient =
    (
            uint64_t( info.upper_margin + info.lower_margin + info.yres )
            * ( info.left_margin  + info.right_margin + info.xres )
            * info.pixclock
    );

    /* Beware, info.pixclock might be 0 under emulation, so avoid a
     * division-by-0 here (SIGFPE on ARM) */
    int refreshRate = refreshQuotient > 0 ? (int)(1000000000000000LLU / refreshQuotient) : 0;

    if (refreshRate == 0) {
        /* bleagh, bad info from the driver */
        refreshRate = 60*1000;  /* 60 Hz */
    }

    if (int(info.width) <= 0 || int(info.height) <= 0)
    {
        /* the driver doesn't return that information default to 160 dpi. */
        info.width  = ((info.xres * 25.4f) / 160.0f + 0.5f);
        info.height = ((info.yres * 25.4f) / 160.0f + 0.5f);
    }

    float xdpi = (info.xres * 25.4f) / info.width;
    float ydpi = (info.yres * 25.4f) / info.height;
    float fps  = refreshRate / 1000.0f;

    LOGI("using (fd=%d)\n"
         "id           = %s\n"
         "xres         = %d px\n"
         "yres         = %d px\n"
         "xres_virtual = %d px\n"
         "yres_virtual = %d px\n"
         "bpp          = %d\n"
         "r            = %2u:%u\n"
         "g            = %2u:%u\n"
         "b            = %2u:%u\n",
         fd,
         finfo.id,
         info.xres,
         info.yres,
         info.xres_virtual,
         info.yres_virtual,
         info.bits_per_pixel,
         info.red.offset, info.red.length,
         info.green.offset, info.green.length,
         info.blue.offset, info.blue.length);

    LOGI("width        = %d mm (%f dpi)\n"
         "height       = %d mm (%f dpi)\n"
         "refresh rate = %.2f Hz\n"
         "Framebuffer phys addr = %p\n",
         info.width,  xdpi,
         info.height, ydpi,
         fps,
         (void *) finfo.smem_start);


    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        return -errno;
    }

#ifdef USE_OSDM
	LOGW("finfo.line_length = %d", finfo.line_length);
	/* Should be set by the driver */
	finfo.line_length = info.xres * info.bits_per_pixel / 8;
#endif

    if (finfo.smem_len <= 0)
    {
        return -errno;
    }

    Module->info  = info;
    Module->finfo = finfo;
    Module->xdpi  = xdpi;
    Module->ydpi  = ydpi;
    Module->fps   = fps;

    /* map the framebuffer */
    size_t fbSize = roundUpToPageSize(finfo.line_length * info.yres_virtual);

    /* Check pages for mapping. */
    if (NUM_PAGES_MMAP != 0)
    {
        fbSize = NUM_PAGES_MMAP * PAGE_SIZE;
    }

    Module->framebuffer = new gc_private_handle_t(dup(fd), fbSize, 0);

#ifndef FRONT_BUFFER
    Module->numBuffers = info.yres_virtual / info.yres;
#else
    Module->numBuffers = 1;
#endif
    Module->bufferMask = 0;

    void * vaddr = mmap(0, fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    if (vaddr == MAP_FAILED)
    {
        LOGE("Error mapping the framebuffer (%s)", strerror(errno));

        return -errno;
    }

    Module->framebuffer->base = intptr_t(vaddr);
    Module->framebuffer->phys = intptr_t(finfo.smem_start);
    memset(vaddr, 0, fbSize);

    return 0;
}

/*******************************************************************************
**
**  mapFrameBuffer
**
**  Open framebuffer device and initialize (call mapFrameBufferLocked).
**
**  INPUT:
**
**      private_module_t * Module
**          Specified gralloc module.
**
**  OUTPUT:
**
**      Nothing.
*/
static int
mapFrameBuffer(
    struct private_module_t* Module
    )
{
    pthread_mutex_lock(&Module->lock);

    int err = mapFrameBufferLocked(Module);

    pthread_mutex_unlock(&Module->lock);

    return err;
}

/*******************************************************************************
**
**  fb_close
**
**  Close framebuffer device.
**
**  INPUT:
**
**      struct hw_device_t * Dev
**          Specified framebuffer device.
**
**  OUTPUT:
**
**      Nothing.
*/
static int
fb_close(
    struct hw_device_t * Dev
    )
{
    fb_context_t * ctx = (fb_context_t*) Dev;

    if (ctx != NULL)
    {
        free(ctx);
    }

    return 0;
}

/*******************************************************************************
**
**  fb_open
**
**  Open framebuffer device.
**
**  INPUT:
**
**      hw_module_t const * Module
**          Specified gralloc module.
**
**      const char * Name
**          Specified framebuffer device name.
**
**  OUTPUT:
**
**      hw_device_t ** Device
**          Framebuffer device handle.
*/
int
fb_device_open(
    hw_module_t const * Module,
    const char * Name,
    hw_device_t ** Device
    )
{
    int status = -EINVAL;

    if (!strcmp(Name, GRALLOC_HARDWARE_FB0))
    {
        alloc_device_t * gralloc_device;

        status = gralloc_open(Module, &gralloc_device);
        if (status < 0)
        {
            return status;
        }

        /* Initialize our state here */
        fb_context_t * dev = (fb_context_t *) malloc(sizeof (*dev));
        memset(dev, 0, sizeof (*dev));

        /* initialize the procs */
        dev->device.common.tag          = HARDWARE_DEVICE_TAG;
        dev->device.common.version      = 0;
        dev->device.common.module       = const_cast<hw_module_t*>(Module);
        dev->device.common.close        = fb_close;
        dev->device.setSwapInterval     = fb_setSwapInterval;
        dev->device.post                = fb_post;
        dev->device.setUpdateRect       = 0;
        /* This function will be called in surfaceflinger. Set it to
         * 'fb_compositionComplete' if you need a hardware sync. Normally for
         * copybit composition (deprecated). */
        dev->device.compositionComplete = NULL;
        private_module_t * m = (private_module_t *) Module;
        status = mapFrameBuffer(m);

        if (status >= 0)
        {
            int stride = m->finfo.line_length / (m->info.bits_per_pixel >> 3);
#ifdef USE_OSDM
		format = (m->info.bits_per_pixel == 32)
					 ? HAL_PIXEL_FORMAT_RGBA_8888
					 : HAL_PIXEL_FORMAT_RGB_565;
		LOGW("format = %d", format);
#endif

            const_cast<uint32_t&>(dev->device.flags)      = 0;
            const_cast<uint32_t&>(dev->device.width)      = m->info.xres;
            const_cast<uint32_t&>(dev->device.height)     = m->info.yres;
            const_cast<int&>(dev->device.stride)          = stride;
            const_cast<int&>(dev->device.format)          = format;
            const_cast<float&>(dev->device.xdpi)          = m->xdpi;
            const_cast<float&>(dev->device.ydpi)          = m->ydpi;
            const_cast<float&>(dev->device.fps)           = m->fps;
            const_cast<int&>(dev->device.minSwapInterval) = 1;
            const_cast<int&>(dev->device.maxSwapInterval) = 1;

            *Device = &dev->device.common;
        }
    }

    return status;
}

