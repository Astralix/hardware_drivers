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




#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <cutils/ashmem.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <binder/IPCThreadState.h>

#include "gc_gralloc_priv.h"
#include "gc_gralloc_gr.h"

#include <gc_hal_user.h>
#include <gc_hal_base.h>

using namespace android;


/*******************************************************************************
**
**  YUV pixel formats of android hal.
**
**  Different android versions have different definitaions.
**  These are collected from hardware/libhardware/include/hardware/hardware.h
*/
enum
{
    ANDROID_HAL_PIXEL_FORMAT_YCbCr_422_SP       = 0x10, // NV16
    ANDROID_HAL_PIXEL_FORMAT_YCrCb_420_SP       = 0x11, // NV21
    ANDROID_HAL_PIXEL_FORMAT_YCbCr_422_P        = 0x12,
    ANDROID_HAL_PIXEL_FORMAT_YCbCr_420_P        = 0x13, // I420
    ANDROID_HAL_PIXEL_FORMAT_YCbCr_422_I        = 0x14, // YUY2
    ANDROID_HAL_PIXEL_FORMAT_YCbCr_420_I        = 0x15,
    ANDROID_HAL_PIXEL_FORMAT_CbYCrY_422_I       = 0x16, // UYVY
    ANDROID_HAL_PIXEL_FORMAT_CbYCrY_420_I       = 0x17,
    ANDROID_HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED = 0x20,
    ANDROID_HAL_PIXEL_FORMAT_YCbCr_420_SP       = 0x21, // NV12
    ANDROID_HAL_PIXEL_FORMAT_YCrCb_420_SP_TILED = 0x22,
    ANDROID_HAL_PIXEL_FORMAT_YCrCb_422_SP       = 0x23, // NV61

    ANDROID_HAL_PIXEL_FORMAT_YV12               = 0x32315659, // YCrCb 4:2:0 Planar
};


/*******************************************************************************
**
**  _ConvertAndroid2HALFormat
**
**  Convert android pixel format to Vivante HAL pixel format.
**
**  INPUT:
**
**      int Format
**          Specified android pixel format.
**
**  OUTPUT:
**
**      gceSURF_FORMAT HalFormat
**          Pointer to hold hal pixel format.
*/
static gceSTATUS
_ConvertAndroid2HALFormat(
    int Format,
    gceSURF_FORMAT * HalFormat
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    switch (Format)
    {
    case HAL_PIXEL_FORMAT_RGB_565:
        *HalFormat = gcvSURF_R5G6B5;
        break;

    case HAL_PIXEL_FORMAT_RGBA_8888:
        *HalFormat = gcvSURF_A8B8G8R8;
        break;

    case HAL_PIXEL_FORMAT_RGBX_8888:
        *HalFormat = gcvSURF_X8B8G8R8;
        break;

    case HAL_PIXEL_FORMAT_RGBA_4444:
        *HalFormat = gcvSURF_R4G4B4A4;
        break;

    case HAL_PIXEL_FORMAT_RGBA_5551:
        *HalFormat = gcvSURF_R5G5B5A1;
        break;

    case HAL_PIXEL_FORMAT_BGRA_8888:
        *HalFormat = gcvSURF_A8R8G8B8;
        break;

    case ANDROID_HAL_PIXEL_FORMAT_YCbCr_420_SP:
        /* YUV 420 semi planner: NV12 */
        *HalFormat = gcvSURF_NV12;
        break;

    case ANDROID_HAL_PIXEL_FORMAT_YCrCb_420_SP:
        /* YVU 420 semi planner: NV21 */
        *HalFormat = gcvSURF_NV21;
        break;

    case ANDROID_HAL_PIXEL_FORMAT_YCbCr_420_P:
        /* YUV 420 planner: I420 */
        *HalFormat = gcvSURF_I420;
        break;

    case ANDROID_HAL_PIXEL_FORMAT_YV12:
        /* YVU 420 planner: YV12 */
        *HalFormat = gcvSURF_YV12;
        break;

    case ANDROID_HAL_PIXEL_FORMAT_YCbCr_422_I:
        /* YUV 422 package: YUYV, YUY2 */
        *HalFormat = gcvSURF_YUY2;
        break;

    case ANDROID_HAL_PIXEL_FORMAT_CbYCrY_422_I:
        /* UYVY 422 package: UYVY */
        *HalFormat = gcvSURF_UYVY;
        break;


    default:
        *HalFormat = gcvSURF_UNKNOWN;
        status = gcvSTATUS_INVALID_ARGUMENT;

        LOGE("Unknown format %d", Format);
        break;
    }

    return status;
}

/*******************************************************************************
**
**  _MapBuffer
**
**  Map android native buffer (call gc_gralloc_map).
**
**  INPUT:
**
**      gralloc_module_t const * Module
**          Specified gralloc module.
**
**      buffer_handle_t Handle
**          Specified buffer handle.
**
**  OUTPUT:
**
**      void ** Vaddr
**          Point to save virtual address pointer.
*/
int
_MapBuffer(
    gralloc_module_t const * Module,
    private_handle_t * Handle)
{
    void * vaddr;
    int retCode = 0;

    retCode = gc_gralloc_map(Module, Handle, &vaddr);

    if (retCode < 0)
    {
        return retCode;
    }

    /* Cleanup noise */
    gc_private_handle_t* hnd = (gc_private_handle_t*) Handle;

    /* Clear memory. */
    gcoOS_MemFill(vaddr,
                  0,
                  (hnd->surface != 0)
                  ? hnd->adjustedSize
                  : hnd->resolveAdjustedSize);

    return retCode;
}

/*******************************************************************************
**
**  _TerminateBuffer
**
**  Destroy android native buffer and free resource (in surfaceflinger).
**
**  INPUT:
**
**      gralloc_module_t const * Module
**          Specified gralloc module.
**
**      buffer_handle_t Handle
**          Specified buffer handle.
**
**  OUTPUT:
**
**      void ** Vaddr
**          Point to save virtual address pointer.
*/
static int
_TerminateBuffer(
    gralloc_module_t const * Module,
    private_handle_t * Handle
    )
{
    gceSTATUS status;

    gc_private_handle_t* hnd = (gc_private_handle_t*) Handle;

    if (hnd->base)
    {
        /* this buffer was mapped, unmap it now. */
        gc_gralloc_unmap(Module, Handle);
    }

    /* Memory is merely scheduled to be freed.
       It will be freed after a h/w event. */
    if (hnd->surface != 0)
    {
        gcmVERIFY_OK(
            gcoSURF_Destroy((gcoSURF) hnd->surface));

        LOGV("Freed buffer handle=%p, "
             "surface=%p",
             (void *) hnd,
             (void *) hnd->surface);
    }

    /* Destroy tile surface. */
    if (hnd->resolveSurface != 0)
    {
        gcmVERIFY_OK(
            gcoSURF_Destroy((gcoSURF) hnd->resolveSurface));

        LOGV("Freed buffer handle=%p, "
             "resolveSurface=%p",
             (void *) hnd,
             (void *) hnd->resolveSurface);
    }

    /* Destroy signal if exists. */
    if (hnd->hwDoneSignal != 0)
    {
        gcmVERIFY_OK(
            gcoOS_DestroySignal(gcvNULL, (gctSIGNAL) hnd->hwDoneSignal));

        hnd->hwDoneSignal = 0;
    }

    hnd->clientPID = 0;

    /* Commit command buffer. */
    gcmONERROR(
        gcoHAL_Commit(gcvNULL, gcvFALSE));

    return 0;

OnError:
    LOGE("Failed to free buffer, status=%d", status);

    return -EINVAL;
}

/*******************************************************************************
**
**  gc_gralloc_alloc_framebuffer_locked
**
**  Map framebuffer memory to android native buffer.
**
**  INPUT:
**
**      alloc_device_t * Dev
**          alloc device handle.
**
**      int Width
**          Specified target buffer width.
**
**      int Hieght
**          Specifed target buffer height.
**
**      int Format
**          Specified target buffer format.
**
**      int Usage
**          Specified target buffer usage.
**
**  OUTPUT:
**
**      buffer_handle_t * Handle
**          Pointer to hold the mapped buffer handle.
**
**      int * Stride
**          Pointer to hold buffer stride.
*/
static int
gc_gralloc_alloc_framebuffer_locked(
    alloc_device_t * Dev,
    int Width,
    int Height,
    int Format,
    int Usage,
    buffer_handle_t * Handle,
    int * Stride
    )
{
    private_module_t * m =
        reinterpret_cast<private_module_t*>(Dev->common.module);

    /* Map framebuffer to Vivante hal surface(fb wrapper). */
    gceSTATUS      status         = gcvSTATUS_OK;
    gcoSURF        fbWrapper      = gcvNULL;
    gceSURF_FORMAT format;
    gctUINT32      baseAddress;

    /* Allocate the framebuffer. */
    if (m->framebuffer == NULL)
    {
        /* Initialize the framebuffer, the framebuffer is mapped once
        ** and forever. */
        int err = mapFrameBufferLocked(m);
        if (err < 0)
        {
            return err;
        }
    }

    const uint32_t bufferMask = m->bufferMask;
    const uint32_t numBuffers = m->numBuffers;
    const size_t   bufferSize = m->finfo.line_length * m->info.yres;

    if (bufferMask >= ((1LU << numBuffers) - 1))
    {
        /* We ran out of buffers. */
        return -ENOMEM;
    }

    /* Create handles for it. */
    intptr_t vaddr = intptr_t(m->framebuffer->base);
    intptr_t paddr = intptr_t(m->framebuffer->phys);

    gc_private_handle_t * handle =
        new gc_private_handle_t(dup(m->framebuffer->fd),
                             bufferSize,
                             gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER);

    /* Find a free slot. */
    for (uint32_t i = 0; i < numBuffers; i++)
    {
        if ((bufferMask & (1LU << i)) == 0)
        {
#ifndef FRONT_BUFFER
            m->bufferMask |= (1LU << i);
#endif
            break;
        }
        vaddr += bufferSize;
        paddr += bufferSize;
    }

    /* Save memory address. */
    handle->base   = vaddr;
    handle->offset = vaddr - intptr_t(m->framebuffer->base);
    handle->phys   = paddr;

    LOGV("Allocated framebuffer handle=%p, vaddr=%p, paddr=%p",
         (void *) handle, (void *) vaddr, (void *) paddr);

    *Handle = handle;
    *Stride = m->finfo.line_length / (m->info.bits_per_pixel / 8);

    /* Map framebuffer to Vivante hal surface(fb wrapper). */
    gctUINT32      phys;

	gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

    /* Get base address. */
    gcmONERROR(
        gcoOS_GetBaseAddress(gcvNULL, &baseAddress));

    /* Construct fb wrapper. */
    gcmONERROR(
        gcoSURF_ConstructWrapper(gcvNULL,
                                 &fbWrapper));

    gcmONERROR(
        _ConvertAndroid2HALFormat(Format,
                                  &format));

    /* Set the underlying frame buffer surface. */
	if (!((handle->phys - baseAddress) & 0x80000000)
		&& !((handle->phys + m->finfo.line_length * Height - 1 - baseAddress) & 0x80000000))
	{
		phys = (gctUINT32) handle->phys - baseAddress;
	}
	else
	{
		phys = ~0U;
	}

    gcmONERROR(
        gcoSURF_SetBuffer(fbWrapper,
                          gcvSURF_BITMAP,
                          format,
                          m->finfo.line_length,
                          (gctPOINTER) handle->base,
                          phys));

    /* Set the window. */
    gcmONERROR(
        gcoSURF_SetWindow(fbWrapper,
                          0, 0,
                          Width, Height));

	gctPOINTER memory[3];
	gctUINT32  address[3];

    gcmONERROR(gcoSURF_Lock(fbWrapper,
            				address, memory));

    handle->phys   = address[0];

    gcmONERROR(gcoSURF_Unlock(fbWrapper,
            				  memory));

    /* Save surface to buffer handle. */
    handle->surface        = (int) fbWrapper;
    handle->resolveSurface = 0;
    handle->format         = (int) format;

    return 0;

OnError:
    *Handle = NULL;

    return -ENOMEM;
}

/*******************************************************************************
**
**  gc_gralloc_alloc_framebuffer
**
**  Map framebuffer memory to android native buffer.
**
**  INPUT:
**
**      alloc_device_t * Dev
**          alloc device handle.
**
**      int Width
**          Specified target buffer width.
**
**      int Hieght
**          Specified target buffer height.
**
**      int Format
**          Specified target buffer format.
**
**      int Usage
**          Specified target buffer usage.
**
**  OUTPUT:
**
**      buffer_handle_t * Handle
**          Pointer to hold the mapped buffer handle.
**
**      int * Stride
**          Pointer to hold buffer stride.
*/
static int
gc_gralloc_alloc_framebuffer(
    alloc_device_t * Dev,
    int Width,
    int Height,
    int Format,
    int Usage,
    buffer_handle_t * Handle,
    int * Stride
    )
{

    private_module_t * m =
        reinterpret_cast<private_module_t*>(Dev->common.module);

    pthread_mutex_lock(&m->lock);

    int err = gc_gralloc_alloc_framebuffer_locked(
        Dev, Width, Height, Format, Usage, Handle, Stride);

    pthread_mutex_unlock(&m->lock);

    return err;
}

/*******************************************************************************
**
**  gc_gralloc_alloc_buffer
**
**  Allocate android native buffer.
**  Will allocate surface types as follows,
**
**  +------------------------------------------------------------------------+
**  | To Allocate Surface(s)        | Linear(surface) | Tile(resolveSurface) |
**  |------------------------------------------------------------------------|
**  |Enable              |  CPU app |      yes        |                      |
**  |LINEAR OUTPUT       |  3D app  |      yes        |                      |
**  |------------------------------------------------------------------------|
**  |Diable              |  CPU app |      yes        |                      |
**  |LINEAR OUTPUT       |  3D app  |                 |         yes          |
**  +------------------------------------------------------------------------+
**
**
**  INPUT:
**
**      alloc_device_t * Dev
**          alloc device handle.
**
**      int Width
**          Specified target buffer width.
**
**      int Hieght
**          Specified target buffer height.
**
**      int Format
**          Specified target buffer format.
**
**      int Usage
**          Specified target buffer usage.
**
**  OUTPUT:
**
**      buffer_handle_t * Handle
**          Pointer to hold the allocated buffer handle.
**
**      int * Stride
**          Pointer to hold buffer stride.
*/
static int
gc_gralloc_alloc_buffer(
    alloc_device_t * Dev,
    int Width,
    int Height,
    int Format,
    int Usage,
    buffer_handle_t * Handle,
    int * Stride
    )
{
    int err = 0;
    int fd = open("/dev/null",O_RDONLY,0);

    gceSTATUS status = gcvSTATUS_OK;
    gctUINT stride   = 0;

    gceSURF_FORMAT format             = gcvSURF_UNKNOWN;
    gceSURF_FORMAT resolveFormat      = gcvSURF_UNKNOWN;
    (void) resolveFormat;

    /* Linear stuff. */
    gcoSURF surface                   = gcvNULL;
    gcuVIDMEM_NODE_PTR vidNode        = gcvNULL;
    gcePOOL pool                      = gcvPOOL_UNKNOWN;
    gctUINT adjustedSize              = 0U;

    /* Tile stuff. */
    gcoSURF resolveSurface            = gcvNULL;
    gcuVIDMEM_NODE_PTR resolveVidNode = gcvNULL;
    gcePOOL resolvePool               = gcvPOOL_UNKNOWN;
    gctUINT resolveAdjustedSize       = 0U;

    gctSIGNAL signal                  = gcvNULL;
    gctINT clientPID                  = 0;

    gctBOOL forSelf                   = gcvTRUE;

    /* Binder info. */
    IPCThreadState* ipc               = IPCThreadState::self();

    /* Buffer handle. */
    gc_private_handle_t * handle      = NULL;

    /* Cast module. */
    gralloc_module_t * module =
        reinterpret_cast<gralloc_module_t *>(Dev->common.module);

    /* Convert to hal pixel format. */
    gcmONERROR(
       _ConvertAndroid2HALFormat(Format,
                                 &format));

    clientPID = ipc->getCallingPid();

    forSelf = (clientPID == getpid());

    if (forSelf)
    {
        LOGI("Self allocation");
    }

    /* Allocate tiled surface if needed. */
#if !gcdGPU_LINEAR_BUFFER_ENABLED
    if (Usage & GRALLOC_USAGE_HW_RENDER)
    {
        /* Get the resolve format. */
        gcmONERROR(
            gcoTEXTURE_GetClosestFormat(gcvNULL,
                                        format,
                                        &resolveFormat));

        /* Construct the tile surface. */
        gcmONERROR(
            gcoSURF_Construct(gcvNULL,
                              Width,
                              Height,
                              1,
                              gcvSURF_TEXTURE,
                              resolveFormat,
                              gcvPOOL_DEFAULT,
                              &resolveSurface));

        /* 3D surfaces are bottom-top. */
        gcmONERROR(
            gcoSURF_SetOrientation(resolveSurface,
                                   gcvORIENTATION_BOTTOM_TOP));

        /*
        gcmONERROR(gcoSURF_SetUsage(resolveSurface, (Usage & GRALLOC_USAGE_SW_WRITE_MASK) ?
                                                    gcvSURF_USAGE_RESOLVE_AFTER_CPU : gcvSURF_USAGE_RESOLVE_AFTER_3D));
         */

        /* Now retrieve and store vid mem node attributes. */
        gcmONERROR(
           gcoSURF_QueryVidMemNode(resolveSurface,
                                   &resolveVidNode,
                                   &resolvePool,
                                   &resolveAdjustedSize));

        /* Android expects stride in pixels which is returned as alignedWidth. */
        gcmONERROR(
            gcoSURF_GetAlignedSize(resolveSurface,
                                   &stride,
                                   gcvNULL,
                                   gcvNULL));

        LOGV("resolve buffer resolveSurface=%p, "
             "resolveNode=%p, "
             "resolvevidnode=%p, "
             "resolveBytes=%d, "
             "resolvePool=%d, "
             "resolveFormat=%d",
             (void *) resolveSurface,
             (void *) &resolveSurface->info.node,
             (void *) resolveVidNode,
             resolveAdjustedSize,
             resolvePool,
             resolveFormat);
    }
#endif

#if !gcdGPU_LINEAR_BUFFER_ENABLED
    /* Allocate linear surface if needed. */
    if (Usage & GRALLOC_USAGE_SW_WRITE_MASK)
#endif
    {
        /* For CPU apps, if allocating for self (i.e. for server a.k.a. SF),
         * then we must request a cacheable allocation so when it gets mapped,
         * it'll be cacheable. Otherwise, the surface will be mapped uncached
         * in the server, but cached in the client. */
        gceSURF_TYPE type = (forSelf && (Usage & GRALLOC_USAGE_SW_WRITE_MASK))
                          ? gcvSURF_CACHEABLE_BITMAP : gcvSURF_BITMAP;

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
        if (Usage & GRALLOC_USAGE_HW_RENDER)
        {
            type = gcvSURF_FLIP_BITMAP;
        }
#endif

        /* Allocate from contiguous pool for CPU apps while from default pool
         * for GPU apps. */
        pool = (Usage & GRALLOC_USAGE_SW_WRITE_MASK) ? gcvPOOL_CONTIGUOUS
             : gcvPOOL_DEFAULT;

        /* Allocate the linear surface. */
        gcmONERROR(
            gcoSURF_Construct(gcvNULL,
                              Width,
                              Height,
                              1,
                              type,
                              format,
                              pool,
                              &surface));

        /* Now retrieve and store vid mem node attributes. */
        gcmONERROR(
           gcoSURF_QueryVidMemNode(surface,
                                   &vidNode,
                                   &pool,
                                   &adjustedSize));

        /* Get stride. */
        gcmONERROR(
            gcoSURF_GetAlignedSize(surface,
                                   &stride,
                                   gcvNULL,
                                   gcvNULL));

        LOGV("allocated buffer surface=%p, "
             "node=%p, "
             "vidNode=%p, "
             "pool=%d, "
             "bytes=%d, "
             "width=%d, height=%d, "
             "formatRequested=%d, "
             "usage=0x%08X",
             (void *) surface,
             (void *) &surface->info.node,
             (void *) vidNode,
             pool,
             adjustedSize,
             Width, Height,
             format,
             Usage);
    }

    if (!(Usage & GRALLOC_USAGE_HW_RENDER))
    {
        /* For CPU apps, we must synchronize lock requests from CPU with the composition.
         * Composition could happen in the following ways. i)2D, ii)3D, iii)CE, iv)copybit 2D.
         * (Note that, we are not considering copybit composition of 3D apps, which also uses
         * linear surfaces. Can be added later if needed.)

         * In all cases, the following mechanism must be used for proper synchronization
         * between CPU and GPU :

         * - App on gralloc::lock
         *      wait_signal(hwDoneSignal);

         * - Compositor on composition
         *      set_unsignalled(hwDoneSignal);
         *      issue composition;
         *      schedule_event(hwDoneSignal, clientPID);

         *  This is a manually reset signal, which is unsignalled by the compositor when
         *  buffer is in use, prohibiting app from obtaining write lock.
         */
        /* Manually reset signal, for CPU/GPU sync. */
        gcmONERROR(gcoOS_CreateSignal(gcvNULL,
                                      gcvTRUE,
                                      &signal));

        /* Initially signalled. */
        gcmONERROR(gcoOS_Signal(gcvNULL,
                                signal,
                                gcvTRUE));

        LOGV("Created signal=%p for hnd=%p", signal, handle);
    }

    /* Allocate buffer handle. */
    handle = new gc_private_handle_t(fd, stride * Height, 0);

    handle->width               = (int) Width;
    handle->height              = (int) Height;
    handle->format              = (int) format;
    handle->size                = ((surface == gcvNULL) ? 0 : (int) surface->info.node.size);

    /* Save linear surface to buffer handle. */
    handle->surface             = (int) surface;
    handle->vidNode             = (int) vidNode;
    handle->pool                = (int) pool;
    handle->adjustedSize        = (int) adjustedSize;

    /* Save tile resolveSurface to buffer handle. */
    handle->resolveSurface      = (int) resolveSurface;
    handle->resolveVidNode      = (int) resolveVidNode;
    handle->resolvePool         = (int) resolvePool;
    handle->resolveAdjustedSize = (int) resolveAdjustedSize;

    handle->hwDoneSignal        = (int) signal;

    /* Record usage to recall later in hwc. */
    handle->lockUsage           = 0;
    handle->allocUsage          = Usage;
    handle->clientPID           = clientPID;

    LOGV("allocated buffer handle=%p, "
         "width=%d, height=%d, "
         "formatRequested=%d, "
         "usage=0x%08X, "
         "stride=%d pixels",
         (void *) handle,
         Width,
         Height,
         format,
         Usage,
         stride);

    /* Map video memory to userspace. */
    err = _MapBuffer(module, handle);

    if (err == 0)
    {
        *Handle = handle;
        *Stride = stride;
    }
    else
    {
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    return err;

OnError:
    /* Destroy linear surface. */
    if (surface != gcvNULL)
    {
        gcoSURF_Destroy(surface);
    }

    /* Destroy tile resolveSurface. */
    if (resolveSurface != gcvNULL)
    {
        gcoSURF_Destroy(resolveSurface);
    }

    /* Destroy signal. */
    if (signal)
    {
        gcoOS_DestroySignal(gcvNULL, signal);
    }

    /* Error roll back. */
    if (handle != NULL)
    {
        delete handle;
    }

    LOGE("failed to allocate, status=%d", status);

    return -EINVAL;
}

/*******************************************************************************
**
**  gc_gralloc_alloc
**
**  General buffer alloc.
**
**  INPUT:
**
**      alloc_device_t * Dev
**          alloc device handle.
**
**      int Width
**          Specified target buffer width.
**
**      int Hieght
**          Specified target buffer height.
**
**      int Format
**          Specified target buffer format.
**
**      int Usage
**          Specified target buffer usage.
**
**  OUTPUT:
**
**      buffer_handle_t * Handle
**          Pointer to hold the allocated buffer handle.
**
**      int * Stride
**          Pointer to hold buffer stride.
*/
int
gc_gralloc_alloc(
    alloc_device_t * Dev,
    int Width,
    int Height,
    int Format,
    int Usage,
    buffer_handle_t * Handle,
    int * Stride)
{
    int err;

    if (!Handle || !Stride)
    {
        return -EINVAL;
    }

#ifdef BUILD_GOOGLETV
    if ((HAL_PIXEL_FORMAT_GTV_VIDEO_HOLE == Format) ||
        (HAL_PIXEL_FORMAT_GTV_OPAQUE_BLACK == Format))
    {
        gc_private_handle_t *hnd = NULL;
        if (Usage & GRALLOC_USAGE_HW_FB) {
            hnd = new gc_private_handle_t(0, 0, NULL, gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER);
        } else {
            hnd = new gc_private_handle_t(0, 0, NULL, 0);
        }

        hnd->width  = 1;
        hnd->height = 1;
        hnd->format = Format;

        *Handle = hnd;
        return 0;
    }
#endif

    if (Usage & GRALLOC_USAGE_HW_FB)
    {
        /* Alloc framebuffer buffer. */
        err = gc_gralloc_alloc_framebuffer(
            Dev, Width, Height, Format, Usage, Handle, Stride);
    }
    else
    {
        /* Alloc non-framebuffer buffer. */
        err = gc_gralloc_alloc_buffer(
            Dev, Width, Height, Format, Usage, Handle, Stride);
    }

    return err;
}

/*******************************************************************************
**
**  gc_gralloc_free
**
**  General buffer free.
**
**  INPUT:
**
**      alloc_device_t * Dev
**          alloc device handle.
**
**      buffer_handle_t Handle
**          Specified target buffer to free.
**
**  OUTPUT:
**
**      Nothing.
*/
int
gc_gralloc_free(
    alloc_device_t * Dev,
    buffer_handle_t Handle
    )
{
    if (gc_private_handle_t::validate(Handle) < 0)
    {
        return -EINVAL;
    }

    /* Cast private buffer handle. */
    gc_private_handle_t const * hnd =
        reinterpret_cast<gc_private_handle_t const *>(Handle);

#ifdef BUILD_GOOGLETV
    if ((HAL_PIXEL_FORMAT_GTV_VIDEO_HOLE == hnd->format) ||
        (HAL_PIXEL_FORMAT_GTV_OPAQUE_BLACK == hnd->format))
    {
        delete hnd;
        return 0;
    }
#endif

    if (hnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER)
    {
        /* Free framebuffer. */
        private_module_t * m =
            reinterpret_cast<private_module_t *>(Dev->common.module);

        const size_t bufferSize = m->finfo.line_length * m->info.yres;
        int index = (hnd->base - m->framebuffer->base) / bufferSize;

        /* Save unused buffer index. */
        m->bufferMask &= ~(1<<index);
    }
    else
    {
        /* Free non-framebuffer. */
        gralloc_module_t * module =
            reinterpret_cast<gralloc_module_t *>(Dev->common.module);

        _TerminateBuffer(module, const_cast<gc_private_handle_t *>(hnd));
    }

    close(hnd->fd);
    delete hnd;

    return 0;
}

