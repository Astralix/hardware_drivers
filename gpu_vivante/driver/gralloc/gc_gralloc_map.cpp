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
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>
#include "gc_gralloc_priv.h"

#include <gc_hal_user.h>
#include <gc_hal_base.h>

#if gcdDEBUG
/* Must be included after Vivante headers. */
#include <utils/Timers.h>
#endif


/*******************************************************************************
**
**  gc_gralloc_map
**
**  Map android native buffer to userspace.
**  This is not for client. So only the linear surface is exported since only
**  linear surface can be used for software access.
**  Reference table in gralloc_alloc_buffer(gc_gralloc_alloc.cpp)
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
gc_gralloc_map(
    gralloc_module_t const * Module,
    buffer_handle_t Handle,
    void ** Vaddr
    )
{
    gc_private_handle_t * hnd = (gc_private_handle_t *) Handle;

    /* Map if non-framebuffer and not mapped. */
    if (!(hnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER))
    {
        gctUINT32 Address[3];
        gctPOINTER Memory[3];
        gceSTATUS status;

        gcoSURF surface = hnd->surface ? (gcoSURF) hnd->surface
                        : (gcoSURF) hnd->resolveSurface;

        /* Lock surface for memory. */
        gcmONERROR(
            gcoSURF_Lock(surface,
                         Address,
                         Memory));

        hnd->phys = (int) Address[0];
        hnd->base = intptr_t(Memory[0]);

        LOGV("mapped buffer "
             "hnd=%p, "
             "surface=%p, "
             "physical=%p, "
             "logical=%p",
             (void *) hnd,
             surface,
             (void *) hnd->phys, Memory[0]);
    }

    /* Obtain virtual address. */
    *Vaddr = (void *) hnd->base;

    return 0;

OnError:
    LOGE("Failed to map hnd=%p",
         (void *) hnd);

    return -EINVAL;
}

/*******************************************************************************
**
**  gc_gralloc_unmap
**
**  Unmap mapped android native buffer.
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
**      Nothing.
**
*/
int
gc_gralloc_unmap(
    gralloc_module_t const * Module,
    buffer_handle_t Handle
    )
{
    gc_private_handle_t * hnd = (gc_private_handle_t *) Handle;

    /* Only unmap non-framebuffer and mapped buffer. */
    if (!(hnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER))
    {
        gceSTATUS status;

        gcoSURF surface = hnd->surface ? (gcoSURF) hnd->surface
                        : (gcoSURF) hnd->resolveSurface;

        /* Unlock surface. */
        gcmONERROR(
            gcoSURF_Unlock(surface,
                           gcvNULL));

        hnd->phys = 0;
        hnd->base = 0;

        LOGV("unmapped buffer hnd=%p", (void *) hnd);
    }

    return 0;

OnError:
    LOGV("Failed to unmap hnd=%p", (void *) hnd);

    return -EINVAL;
}

/*******************************************************************************
**
**  gc_gralloc_register_buffer
**
**  Register buffer to current process (in client).
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
**      Nothing.
**
*/
int
gc_gralloc_register_buffer(
    gralloc_module_t const * Module,
    buffer_handle_t Handle
    )
{
    int err = 0;
    gc_private_handle_t* hnd = (gc_private_handle_t *) Handle;

    gcoSURF surface        = gcvNULL;
    gcoSURF resolveSurface = gcvNULL;
    gctSIGNAL signal       = gcvNULL;

    if (gc_private_handle_t::validate(Handle) < 0)
    {
        return -EINVAL;
    }

#ifdef BUILD_GOOGLETV
    /* No need to do more things for these formats. */
    if (hnd->format == HAL_PIXEL_FORMAT_GTV_VIDEO_HOLE ||
        hnd->format == HAL_PIXEL_FORMAT_GTV_OPAQUE_BLACK)
    {
        return 0;
    }
#endif

    /* if this handle was created in this process, then we keep it as is. */
    if (hnd->pid != getpid())
    {
        gceSTATUS status;
        void * vaddr;

        /* Register linear surface if exists. */
        if (hnd->surface != 0)
        {
            /* Map cacaheable for CPU buffers. */
            gceSURF_TYPE type = (hnd->allocUsage & GRALLOC_USAGE_SW_WRITE_MASK)
                              ? gcvSURF_CACHEABLE_BITMAP_NO_VIDMEM
                              : gcvSURF_BITMAP_NO_VIDMEM;

            /* This video node was created in another process.
             * Import it into this process..
             * gcvSURF_CACHEABLE_BITMAP_NO_VIDMEM is used to signify that no video memory
             * node should be created for this surface, as it has alreaady been
             * created in the server, and that the surface should be mapped cached. */
            gcmONERROR(
                gcoSURF_Construct(gcvNULL,
                                  (gctUINT) hnd->width,
                                  (gctUINT) hnd->height,
                                  1,
                                  type,
                                  (gceSURF_FORMAT) hnd->format,
                                  (gcePOOL) hnd->pool,
                                  &surface));

            /* Save surface info. */
            surface->info.node.u.normal.node = (gcuVIDMEM_NODE_PTR) hnd->vidNode;
            surface->info.node.pool          = (gcePOOL) hnd->pool;
            surface->info.node.size          = hnd->adjustedSize;

            /* Lock once as it's done in gcoSURF_Construct with vidmem. */
            gcmONERROR(
                gcoSURF_Lock(surface,
                             gcvNULL,
                             gcvNULL));

            hnd->surface = (int) surface;

            LOGV("registered buffer hnd=%p, "
                 "vidNode=%p, "
                 "surface=%p",
                 (void *) hnd,
                 (void *) hnd->vidNode,
                 (void *) hnd->surface);
        }

#if !gcdGPU_LINEAR_BUFFER_ENABLED
        /* Register tile surface if exists. */
        if (hnd->resolveSurface != 0)
        {
            gceSURF_FORMAT resolveFormat = gcvSURF_UNKNOWN;

            gcmONERROR(
                gcoTEXTURE_GetClosestFormat(gcvNULL,
                                            (gceSURF_FORMAT) hnd->format,
                                            &resolveFormat));

            /* This video node was created in another process.
             * Import it into this process.. */
            gcmONERROR(
                gcoSURF_Construct(gcvNULL,
                                  (gctUINT) hnd->width,
                                  (gctUINT) hnd->height,
                                  1,
                                  gcvSURF_TEXTURE_NO_VIDMEM,
                                  resolveFormat,
                                  gcvPOOL_DEFAULT,
                                  &resolveSurface));

            /* Save surface info. */
            resolveSurface->info.node.u.normal.node = (gcuVIDMEM_NODE_PTR) hnd->resolveVidNode;
            resolveSurface->info.node.pool          = (gcePOOL) hnd->resolvePool;
            resolveSurface->info.node.size          = hnd->resolveAdjustedSize;

            gcmONERROR(
                gcoSURF_Lock(resolveSurface,
                             gcvNULL,
                             gcvNULL));

            hnd->resolveSurface = (int) resolveSurface;

            LOGV("registered resolve buffer hnd=%p, "
                 "resolveVidNode=%p, "
                 "resolveSurface=%p",
                 (void *) hnd,
                 (void *) hnd->resolveVidNode,
                 (void *) hnd->resolveSurface);

            /* HW surfaces are bottom-top. */
            gcmONERROR(
                gcoSURF_SetOrientation((gcoSURF)hnd->resolveSurface,
                                       gcvORIENTATION_BOTTOM_TOP));
        }
#endif

        /* Map signal if exists. */
        if (hnd->hwDoneSignal != 0)
        {
            /* Map signal to be used in CPU/GPU synchronization. */
            gcmONERROR(
                gcoOS_MapSignal((gctSIGNAL) hnd->hwDoneSignal,
                                &signal));

            hnd->hwDoneSignal = (int) signal;

            LOGV("Created hwDoneSignal=%p for hnd=%p", signal, hnd);
        }

        /* This *IS* the client. Make sure server has the correct information. */
        gcmASSERT(hnd->clientPID == getpid());

        err = gc_gralloc_map(Module, Handle, &vaddr);
    }

    return err;

OnError:
    /* Error roll back. */
    if (surface != gcvNULL)
    {
        gcmVERIFY_OK(
            gcoSURF_Destroy(surface));

        hnd->surface = 0;
    }

    if (resolveSurface != gcvNULL)
    {
        gcmVERIFY_OK(
            gcoSURF_Destroy(resolveSurface));

        hnd->resolveSurface = 0;
    }

    if (signal)
    {
        gcmVERIFY_OK(
            gcoOS_DestroySignal(gcvNULL, (gctSIGNAL)hnd->hwDoneSignal));

        hnd->hwDoneSignal = 0;
    }

    LOGE("Failed to register buffer hnd=%p", (void *) hnd);

    return -EINVAL;
}

/*******************************************************************************
**
**  gc_gralloc_unregister_buffer
**
**  Unregister buffer in current process (in client).
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
**      Nothing.
**
*/
int
gc_gralloc_unregister_buffer(
    gralloc_module_t const * Module,
    buffer_handle_t Handle
    )
{
    gceSTATUS status;

    if (gc_private_handle_t::validate(Handle) < 0)
    {
        return -EINVAL;
    }

    gc_private_handle_t* hnd = (gc_private_handle_t*)Handle;

#ifdef BUILD_GOOGLETV
    /* No need to do more things for these formats. */
    if (hnd->format == HAL_PIXEL_FORMAT_GTV_VIDEO_HOLE ||
        hnd->format == HAL_PIXEL_FORMAT_GTV_OPAQUE_BLACK)
    {
        return 0;
    }
#endif

    /* Never unmap buffers that were created in this process. */
    if (hnd->pid != getpid())
    {
        if (hnd->base != 0)
        {
            gc_gralloc_unmap(Module, Handle);
        }

        /* Unregister linear surface if exists. */
        if (hnd->surface != 0)
        {
            /* Non-3D rendering...
             * Append the gcvSURF_NO_VIDMEM flag to indicate we are only destroying
             * the wrapper here, actual vid mem node is destroyed in the process
             * that created it. */
            gcoSURF surface = (gcoSURF) hnd->surface;

            gcmONERROR(
                gcoSURF_Destroy(surface));

            LOGV("unregistered buffer hnd=%p, "
                 "surface=%p", (void*)hnd, (void*)hnd->surface);
        }

        /* Unregister tile surface if exists. */
        if (hnd->resolveSurface != 0)
        {
            /* Only destroying the wrapper here, actual vid mem node is
             * destroyed in the process that created it. */
            gcoSURF surface = (gcoSURF) hnd->resolveSurface;

            gcmONERROR(
                gcoSURF_Destroy(surface));

            LOGV("unregistered buffer hnd=%p, "
                 "resolveSurface=%p", (void*)hnd, (void*)hnd->resolveSurface);
        }

        /* Destroy signal if exist. */
        if (hnd->hwDoneSignal)
        {
            gcmONERROR(
                gcoOS_UnmapSignal((gctSIGNAL)hnd->hwDoneSignal));
            hnd->hwDoneSignal = 0;
        }

        hnd->clientPID = 0;
    }

    /* Commit the command buffer. */
    gcmONERROR(
        gcoHAL_Commit(gcvNULL, gcvFALSE));

    return 0;

OnError:
    LOGE("Failed to unregister buffer, hnd=%p", (void *) hnd);

    return -EINVAL;
}

/*******************************************************************************
**
**  gc_gralloc_lock
**
**  Lock android native buffer and get address.
**
**  INPUT:
**
**      gralloc_module_t const * Module
**          Specified gralloc module.
**
**      buffer_handle_t Handle
**          Specified buffer handle.
**
**      int Usage
**          Usage for lock.
**
**      int Left, Top, Width, Height
**          Lock area.
**
**  OUTPUT:
**
**      void ** Vaddr
**          Point to save virtual address pointer.
*/
int
gc_gralloc_lock(
    gralloc_module_t const* Module,
    buffer_handle_t Handle,
    int Usage,
    int Left,
    int Top,
    int Width,
    int Height,
    void ** Vaddr
    )
{
    if (gc_private_handle_t::validate(Handle) < 0)
        return -EINVAL;

    gc_private_handle_t* hnd = (gc_private_handle_t*)Handle;
    *Vaddr = (void *) hnd->base;

    LOGV("To lock buffer hnd=%p, for usage=0x%08X, in (l=%d, t=%d, w=%d, h=%d)",
         (void *) hnd, Usage, Left, Top, Width, Height);

    if (!(hnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER))
    {
        hnd->lockUsage = (int) Usage;

#if gcdCOPYBLT_OPTIMIZATION
        if (Usage & GRALLOC_USAGE_SW_WRITE_MASK)
        {
            /* Record the dirty area. */
            hnd->dirtyX      = Left;
            hnd->dirtyY      = Top;
            hnd->dirtyWidth  = Width;
            hnd->dirtyHeight = Height;
        }
#endif

        if (hnd->hwDoneSignal != 0)
        {
            LOGV("Waiting for compositor hnd=%p, "
                 "for hwDoneSignal=0x%08X",
                 (void *) hnd,
                 hnd->hwDoneSignal);

            /* Wait for compositor (2D, 3D, CE) to source from the CPU buffer. */
            gcmVERIFY_OK(
                gcoOS_WaitSignal(gcvNULL,
                                 (gctSIGNAL) hnd->hwDoneSignal,
                                 gcvINFINITE));
        }
    }


    /* LOGV("locked buffer hnd=%p", (void *) hnd); */
    return 0;
}

/*******************************************************************************
**
**  gc_gralloc_unlock
**
**  Unlock android native buffer.
**  For 3D composition, it will resolve linear to tile for SW surfaces.
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
**      Nothing.
**
*/
int
gc_gralloc_unlock(
    gralloc_module_t const * Module,
    buffer_handle_t Handle
)
{
    if (gc_private_handle_t::validate(Handle) < 0)
        return -EINVAL;

    gc_private_handle_t * hnd = (gc_private_handle_t *) Handle;

    if (!(hnd->flags & gc_private_handle_t::PRIV_FLAGS_FRAMEBUFFER))
    {
        /* LOGV("To unlock buffer hnd=%p", (void *) Handle); */

        if (hnd->lockUsage & GRALLOC_USAGE_SW_WRITE_MASK)
        {
            /* Ignoring the error. */
            gcmVERIFY_OK(
                gcoSURF_CPUCacheOperation((gcoSURF) hnd->surface,
                                          gcvCACHE_CLEAN));

#if gcdCOPYBLT_OPTIMIZATION
            /* CopyBlt locks the surface only for write, whereas the app locks it for
             * read/write which will be submitted for composition. */
            gcsVIDMEM_NODE_SHARED_INFO dynamicInfo;

            /* Src and Dst have identical geometries, so we use the src origin and size. */
            dynamicInfo.SrcOrigin.x     = hnd->dirtyX;
            dynamicInfo.SrcOrigin.y     = hnd->dirtyY;
            dynamicInfo.RectSize.width  = hnd->dirtyWidth;
            dynamicInfo.RectSize.height = hnd->dirtyHeight;

            /* Ignore this error. */
            gcmVERIFY_OK(
                gcoHAL_SetSharedInfo(0,
                                     gcvNULL,
                                     0,
                                     ((gcoSURF)(hnd->surface))->info.node.u.normal.node,
                                     (gctUINT8_PTR)(&dynamicInfo),
                                     gcvVIDMEM_INFO_DIRTY_RECTANGLE));

            LOGV("Passing dirty rect to kernel (l=%d, t=%d, w=%d, h=%d)",
                hnd->dirtyX, hnd->dirtyY, hnd->dirtyWidth, hnd->dirtyHeight);
#endif
        }

        hnd->lockUsage = 0;


        LOGV("Unlocked buffer hnd=%p", (void *) Handle);
    }
    else
    {
        /* Do nothing for framebuffer buffer. */
        LOGV("Unlocked framebuffer hnd=%p", (void *) Handle);
    }

    return 0;
}

