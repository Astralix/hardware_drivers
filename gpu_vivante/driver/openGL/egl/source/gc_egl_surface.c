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






#include "gc_egl_precomp.h"

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcdZONE_EGL_SURFACE

static void _veglAddSurface(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    gctUINT32 address[3] = { 0 };
    gctPOINTER memory[3] = { gcvNULL };
    gctUINT width, height;
    gctINT stride;
    gceSTATUS status;
    gcoSURF surface = gcvNULL;

    if ( (Thread->dump != gcvNULL) && (Surface->renderTarget != gcvNULL) )
    {
        /* Lock the render target. */
        gcmONERROR(gcoSURF_Lock(Surface->renderTarget, address, memory));
        surface = Surface->renderTarget;

        /* Get the size of the render target. */
        gcmONERROR(gcoSURF_GetAlignedSize(Surface->renderTarget,
                                          &width,
                                          &height,
                                          &stride));

        /* Dump the allocation for the render target. */
        gcmONERROR(gcoDUMP_AddSurface(Thread->dump,
                                      width,
                                      height,
                                      Surface->renderTargetFormat,
                                      address[0],
                                      stride * height));

        /* Unlock the render target. */
        gcmONERROR(gcoSURF_Unlock(Surface->renderTarget, memory[0]));
        surface = gcvNULL;
    }

    if ( (Thread->dump != gcvNULL) && (Surface->depthBuffer != gcvNULL) )
    {
        /* Lock the depth buffer. */
        gcmONERROR(gcoSURF_Lock(Surface->depthBuffer, address, memory));
        surface = Surface->depthBuffer;

        /* Get the size of the depth buffer. */
        gcmONERROR(gcoSURF_GetAlignedSize(Surface->depthBuffer,
                                          &width,
                                          &height,
                                          &stride));

        /* Dump the allocation for the depth buffer. */
        gcmONERROR(gcoDUMP_AddSurface(Thread->dump,
                                      width,
                                      height,
                                      Surface->depthFormat,
                                      address[0],
                                      stride * height));

        /* Unlock the depth buffer. */
        gcmONERROR(gcoSURF_Unlock(Surface->depthBuffer, memory[0]));
        surface = gcvNULL;
    }

    if ( (Thread->dump != gcvNULL) && (Surface->resolve != gcvNULL) )
    {
        /* Lock the resolve buffer. */
        gcmONERROR(gcoSURF_Lock(Surface->resolve, address, memory));
        surface = Surface->resolve;

        /* Get the size of the resolve buffer. */
        gcmONERROR(gcoSURF_GetAlignedSize(Surface->resolve,
                                          &width,
                                          &height,
                                          &stride));

        /* Dump the allocation for the resolve buffer. */
        gcmONERROR(gcoDUMP_AddSurface(Thread->dump,
                                      width,
                                      height,
                                      Surface->depthFormat,
                                      address[0],
                                      stride * height));

        /* Unlock the resolve buffer. */
        gcmONERROR(gcoSURF_Unlock(Surface->resolve, memory[0]));
        surface = gcvNULL;
    }

OnError:
    if (surface != gcvNULL)
    {
        /* Unlock the failed surface. */
        gcmVERIFY_OK(gcoSURF_Unlock(surface, memory[0]));
    }

    return;
}

static EGLint _CreateSurfaceObjects(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface,
    IN gceSURF_FORMAT ResolveFormat
    )
{
    gceSTATUS status;
    gctUINT i;

    do
    {
        /* Verify the surface. */
        gcmASSERT(Surface->fbWrapper        == gcvNULL);
#ifdef __QNXNTO__
        gcmASSERT(Surface->fbWrapper2       == gcvNULL);
#endif
        gcmASSERT(Surface->renderTarget     == gcvNULL);
        gcmASSERT(Surface->renderTargetBits == gcvNULL);
        gcmASSERT(Surface->depthBuffer      == gcvNULL);
        gcmASSERT(Surface->resolve          == gcvNULL);
        gcmASSERT(Surface->resolveBits      == gcvNULL);

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
            "%s(%d): fbDirect=%d",
            __FUNCTION__, __LINE__,
            Surface->fbDirect
            );

        if (Surface->fbDirect)
        {
#if !defined(ANDROID)
            /* Do not create fbWrapper on android. */
            /* We have direct access to the frame buffer, create a wrapper. */
            gcmERR_BREAK(gcoSURF_ConstructWrapper(
                gcvNULL,
                &Surface->fbWrapper
                ));

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                "%s(%d): ResolveFormat=%d, Stride=%d",
                __FUNCTION__, __LINE__,
                ResolveFormat,
                Surface->fbInfo.stride
                );

            /* Set the underlying frame buffer surface. */
            gcmERR_BREAK(gcoSURF_SetBuffer(
                Surface->fbWrapper,
                gcvSURF_BITMAP,
                ResolveFormat,
                Surface->fbInfo.stride,
                Surface->fbInfo.logical,
                Surface->fbInfo.physical
                ));

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                "%s(%d): window=%d,%d %dx%d",
                __FUNCTION__, __LINE__,
                Surface->bitsX,
                Surface->bitsY,
                Surface->fbInfo.width,
                Surface->fbInfo.height
                );

            /* For a new surface, clear it to get rid of noises. */
            gcmERR_BREAK(gcoOS_MemFill(
                Surface->fbInfo.logical,
                0,
                  Surface->bitsPerPixel / 8
                * Surface->fbInfo.width
                * Surface->fbInfo.height)
                );

#ifndef __QNXNTO__
            if (Surface->openVG)
            {   /* 355_FB_MULTI_BUFFER */
                gcmERR_BREAK(gcoSURF_SetWindow(
                    Surface->fbWrapper,
                    Surface->bitsX,
                    Surface->bitsY,
                    Surface->config.width,
                    Surface->config.height
                    ));
            }
            else
#endif
            {
                /* Set the window. */
                gcmERR_BREAK(gcoSURF_SetWindow(
                    Surface->fbWrapper,
                    Surface->bitsX,
                    Surface->bitsY,
                    Surface->fbInfo.width,
                    Surface->fbInfo.height
                    ));
            }

/* 355_FB_MULTI_BUFFER */
#ifndef __QNXNTO__
            if (Surface->openVG)
            {
                /* Create the 2DVG backbuffer FBWrappers for FB_MULTI_BUFFER. */
                if (Surface->fbInfo.multiBuffer > 1)
                {       /* Create the first backbuffer. */
                    gctUINT32 backOffset = (gctUINT32)(Surface->fbInfo.stride *
                                            Surface->fbInfo.height / Surface->fbInfo.multiBuffer);
                    gcmASSERT(Surface->fbWrappers[0] == gcvNULL);
                    gcmERR_BREAK(gcoSURF_ConstructWrapper(gcvNULL, &Surface->fbWrappers[0]));
                    Surface->fbInfo.logicals[0] =
                        (gctPOINTER)((gctUINT32)Surface->fbInfo.logical + backOffset);
                    Surface->fbInfo.physicals[0] = Surface->fbInfo.physical + backOffset;

                    /* Set the underlying frame buffer surface. */
                    gcmERR_BREAK(gcoSURF_SetBuffer(
                        Surface->fbWrappers[0],
                        gcvSURF_BITMAP,
                        ResolveFormat,
                        Surface->fbInfo.stride,
                        Surface->fbInfo.logicals[0],
                        Surface->fbInfo.physicals[0]
                        ));

                    /* Set the window. */
                    gcmERR_BREAK(gcoSURF_SetWindow(
                        Surface->fbWrappers[0],
                        Surface->bitsX,
                        Surface->bitsY,
                        Surface->config.width,
                        Surface->config.height
                        ));
                }

                if (Surface->fbInfo.multiBuffer > 2)
                {       /* Setup the 2nd backbuffer. */
                    gctUINT32 backOffset = (gctUINT32)(Surface->fbInfo.stride * 2 *
                                            Surface->fbInfo.height / Surface->fbInfo.multiBuffer);
                    gcmASSERT(Surface->fbWrappers[1] == gcvNULL);
                    gcmERR_BREAK(gcoSURF_ConstructWrapper(gcvNULL, &Surface->fbWrappers[1]));
                    /* Calculate the memory address for backbuffer. */
                    Surface->fbInfo.logicals[1] =
                        (gctPOINTER)((gctUINT32)Surface->fbInfo.logical + backOffset);
                    Surface->fbInfo.physicals[1] = Surface->fbInfo.physical + backOffset;

                    /* Set the underlying frame buffer surface. */
                    gcmERR_BREAK(gcoSURF_SetBuffer(
                        Surface->fbWrappers[1],
                        gcvSURF_BITMAP,
                        ResolveFormat,
                        Surface->fbInfo.stride,
                        Surface->fbInfo.logicals[1],
                        Surface->fbInfo.physicals[1]
                        ));

                    /* Set the window. */
                    gcmERR_BREAK(gcoSURF_SetWindow(
                        Surface->fbWrappers[1],
                        Surface->bitsX,
                        Surface->bitsY,
                        Surface->config.width,
                        Surface->config.height
                        ));
                }
            }
#endif


#ifdef __QNXNTO__
            /* We have direct access to the frame buffer, create a wrapper. */
            gcmERR_BREAK(gcoSURF_ConstructWrapper(
                gcvNULL,
                &Surface->fbWrapper2
                ));

            /* Set the underlying frame buffer surface. */
            gcmERR_BREAK(gcoSURF_SetBuffer(
                Surface->fbWrapper2,
                gcvSURF_BITMAP,
                ResolveFormat,
                Surface->fbInfo.stride,
                Surface->fbInfo.logical2,
                Surface->fbInfo.physical2
                ));

            /* For a new surface, clear it to get rid of noises. */
            gcmERR_BREAK(gcoOS_MemFill(
                Surface->fbInfo.logical2,
                0,
                  Surface->bitsPerPixel / 8
                * Surface->fbInfo.width
                * Surface->fbInfo.height)
                );

            /* Set the window. */
            gcmERR_BREAK(gcoSURF_SetWindow(
                Surface->fbWrapper2,
                Surface->bitsX,
                Surface->bitsY,
                Surface->fbInfo.width,
                Surface->fbInfo.height
                ));

#endif /* __QNXNTO__ */

#endif
        }

        /* OpenVG surface? */
        if (Surface->openVG)
        {
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                "%s(%d): OPEN VG PIPE.",
                __FUNCTION__, __LINE__
                );

            /* Do we have direct access to the frame buffer? */
            if ((Surface->fbWrapper != gcvNULL) &&
                (Surface->renderTargetFormat == ResolveFormat))
            {
                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                    "%s(%d): assuming the wrapper as the render target as well.",
                    __FUNCTION__, __LINE__
                    );

                /* 355_FB_MULTI_BUFFER */
#ifndef __QNXNTO__
                if (Surface->fbInfo.multiBuffer > 1)
                {       /* For FB_MULTI_BUFFER, this is the backbuffer where it should render onto. */
                    Surface->renderTarget = Surface->fbWrappers[0];
                    gcmERR_BREAK(gcoSURF_SetOrientation(
                        Surface->fbWrapper,
                        gcvORIENTATION_TOP_BOTTOM
                        ));

                    /* Set render surface type. */
                    gcmERR_BREAK(gcoSURF_SetColorType(
                        Surface->fbWrapper, Surface->colorType
                        ));

                    if (Surface->fbWrappers[1] != gcvNULL)
                    {
                        gcmERR_BREAK(gcoSURF_SetOrientation(
                            Surface->fbWrappers[1],
                            gcvORIENTATION_TOP_BOTTOM
                            ));

                        /* Set render surface type. */
                        gcmERR_BREAK(gcoSURF_SetColorType(
                            Surface->fbWrappers[1], Surface->colorType
                            ));
                    }

                }
                else        /* -- 355_FB_MULTI_BUFFER */
#endif
                {
                    /* We do, assume the wrapper as the render target as well. */
                    Surface->renderTarget = Surface->fbWrapper;
                }
                gcmERR_BREAK(gcoSURF_ReferenceSurface(Surface->renderTarget));

                /* Set orientation. */
                gcmERR_BREAK(gcoSURF_SetOrientation(
                    Surface->renderTarget,
                    gcvORIENTATION_TOP_BOTTOM
                    ));

                /* Set render surface type. */
                gcmERR_BREAK(gcoSURF_SetColorType(
                    Surface->renderTarget, Surface->colorType
                    ));

#ifdef __QNXNTO__
                gcmERR_BREAK(gcoSURF_SetOrientation(
                    Surface->fbWrapper2,
                    gcvORIENTATION_TOP_BOTTOM
                    ));

                gcmERR_BREAK(gcoSURF_SetColorType(
                    Surface->fbWrapper2, Surface->colorType
                    ));
#endif /* __QNXNTO__ */
            }
            else if (gcvNULL != (void *)(Surface->pixmap))
            {
                gcmERR_BREAK(gcoSURF_ConstructWrapper(
                    gcvNULL,
                    &Surface->fbWrapper
                    ));

                /* Set the underlying frame buffer surface. */
                gcmERR_BREAK(gcoSURF_SetBuffer(
                    Surface->fbWrapper,
                    gcvSURF_BITMAP,
                    ResolveFormat,
                    Surface->pixmapStride,
                    Surface->pixmapBits,
                    ~0U
                    ));

                 /* Set the window. */
                gcmERR_BREAK(gcoSURF_SetWindow(
                    Surface->fbWrapper,
                    Surface->bitsX,
                    Surface->bitsY,
                    Surface->bitsWidth,
                    Surface->bitsHeight
                    ));

                Surface->renderTarget = Surface->fbWrapper;
                gcmERR_BREAK(gcoSURF_ReferenceSurface(Surface->renderTarget));

                /* Set orientation. */
                gcmERR_BREAK(gcoSURF_SetOrientation(
                    Surface->renderTarget,
                    gcvORIENTATION_TOP_BOTTOM
                    ));

                /* Set render surface type. */
                gcmERR_BREAK(gcoSURF_SetColorType(
                    Surface->renderTarget, Surface->colorType
                    ));
            }
            else
            {
                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                    "%s(%d): render taget=%dx%d, format=%d",
                    __FUNCTION__, __LINE__,
                    Surface->bitsWidth,
                    Surface->bitsHeight,
                    Surface->renderTargetFormat
                    );

                /* Enable multi-buffer support. */
                Surface->renderListEnable = gcvTRUE;

                /* We don't have direct access to the frame buffer, create
                   a surface for rendering into. */
                gcmERR_BREAK(veglAddRenderListSurface(Thread, Surface));

                /* Set render target to the first entry. */
                Surface->renderTarget = Surface->renderListCurr->surface;
                gcmERR_BREAK(gcoSURF_ReferenceSurface(Surface->renderTarget));

                /* Set the swap surface. */
                Surface->swapSurface
                    = Surface->renderTarget;

                Surface->swapBitsPerPixel
                    = Surface->renderTargetInfo[0]->bitsPerPixel;
            }

            /* Get the aligned size of the surface and the stride. */
            gcmERR_BREAK(gcoSURF_GetAlignedSize(
                Surface->renderTarget,
                &Surface->bitsAlignedWidth,
                &Surface->bitsAlignedHeight,
                &Surface->bitsStride
                ));
        }
        else
        {
#ifndef VIVANTE_NO_3D
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                "%s(%d): 3D PIPE.",
                __FUNCTION__, __LINE__
                );

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                "%s(%d): render taget=%dx%d, format=%d, samples=%d",
                __FUNCTION__, __LINE__,
                Surface->bitsWidth,
                Surface->bitsHeight,
                Surface->renderTargetFormat,
                Surface->config.samples
                );

            /* Create render target. */
            gcmERR_BREAK(gcoSURF_Construct(
                gcvNULL,
                Surface->bitsWidth,
                Surface->bitsHeight, 1,
                gcvSURF_RENDER_TARGET,
                Surface->renderTargetFormat,
                gcvPOOL_DEFAULT,
                &Surface->renderTarget
                ));

            /* Set multi-sampling size. */
            gcmERR_BREAK(gcoSURF_SetSamples(
                Surface->renderTarget,
                Surface->config.samples
                ));

            if (Surface->depthFormat != gcvSURF_UNKNOWN)
            {
                /* Create depth buffer. */
                gcmERR_BREAK(gcoSURF_Construct(
                    gcvNULL,
                    Surface->bitsWidth,
                    Surface->bitsHeight, 1,
                    gcvSURF_DEPTH,
                    Surface->depthFormat,
                    gcvPOOL_DEFAULT,
                    &Surface->depthBuffer
                    ));

                /* Set multi-sampling size. */
                gcmERR_BREAK(gcoSURF_SetSamples(
                    Surface->depthBuffer,
                    Surface->config.samples
                    ));
             }

            if ((Surface->type & EGL_PIXMAP_BIT) != 0)
            {
                gctUINT32 alignment;

                gcmASSERT(Surface->pixmap != 0);

                /* Determine the required alignment. */
                gcmERR_BREAK(gcoSURF_GetAlignment(gcvSURF_RENDER_TARGET, ResolveFormat, &alignment, gcvNULL, gcvNULL));

                if (Surface->pixmapBits != gcvNULL)
                {
                    /* Is the pixmap not properly aligned? */
                    if ((gcmPTR2INT(Surface->pixmapBits) & (alignment - 1)) != 0)
                    {

                        /* The surface is not properly aligned, create a temporary surface
                           to resolve to. */
                        gcmERR_BREAK(gcoSURF_Construct(
                            gcvNULL,
                            Surface->config.width,
                            Surface->config.height,
                            1,
                            gcvSURF_BITMAP,
                            ResolveFormat,
                            gcvPOOL_SYSTEM,
                            &Surface->pixmapSurface
                            ));

		                /* Lock the surface. */
		                gcmERR_BREAK(gcoSURF_Lock(
		                    Surface->pixmapSurface,
                            gcvNULL,
                            &Surface->tempPixmapBits
		                    ));

                        /* Get the stride. */
		                gcmERR_BREAK(gcoSURF_GetAlignedSize(
                            Surface->pixmapSurface,
                            gcvNULL,
                            gcvNULL,
                            &Surface->tempPixmapStride
                            ));
		            }
				    else
				    {
                	    /* Construct a wrapper around the pixmap.  */
                	    gcmERR_BREAK(gcoSURF_ConstructWrapper(
                    	    gcvNULL,
                    	    &Surface->pixmapSurface
                    	    ));

	                    /* Set the underlying frame buffer surface. */
    	                gcmERR_BREAK(gcoSURF_SetBuffer(
        	                Surface->pixmapSurface,
            	            gcvSURF_BITMAP,
                	        ResolveFormat,
                            Surface->pixmapStride,
                            Surface->pixmapBits,
                            ~0U
    	                    ));

        	             /* Set the window. */
            	        gcmERR_BREAK(gcoSURF_SetWindow(
                	        Surface->pixmapSurface,
	                        Surface->bitsX,
    	                    Surface->bitsY,
        	                Surface->bitsWidth,
            	            Surface->bitsHeight
                	        ));
                    }

                    /* Set orientation. */
                    gcmERR_BREAK(gcoSURF_SetOrientation(
                        Surface->pixmapSurface,
                        gcvORIENTATION_BOTTOM_TOP
                        ));
                    }
            }

            /* Create resolve buffer always for pbuffer surface. */
            if (((Surface->type & EGL_PBUFFER_BIT) != 0)
#if !defined(ANDROID)
            /* Do not create resolve for window surface on android. */
            ||  (Surface->fbWrapper == gcvNULL)
#endif
            )
            {
#ifndef __QNXNTO__
                gctPOINTER memoryResolve[3] = {gcvNULL};

                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                    "%s(%d): resolve buffer=%dx%d, format=%d",
                    __FUNCTION__, __LINE__,
                    Surface->bitsWidth,
                    Surface->bitsHeight,
                    ResolveFormat
                    );

                gcmERR_BREAK(gcoSURF_Construct(
                    gcvNULL,
                    Surface->bitsWidth,
                    Surface->bitsHeight, 1,
                    gcvSURF_BITMAP,
                    ResolveFormat,
                    gcvPOOL_UNIFIED,
                    &Surface->resolve
                    ));

                Surface->resolveFormat = ResolveFormat;

                /* Lock the surface. */
                gcmERR_BREAK(gcoSURF_Lock(
                    Surface->resolve, gcvNULL, memoryResolve
                    ));
                Surface->resolveBits = memoryResolve[0];

                /* Get the aligned size of the surface and the stride. */
                gcmERR_BREAK(gcoSURF_GetAlignedSize(
                    Surface->resolve,
                    &Surface->bitsAlignedWidth,
                    &Surface->bitsAlignedHeight,
                    &Surface->bitsStride
                    ));

                /* Set the swap surface. */
                Surface->swapSurface
                    = Surface->resolve;

                Surface->swapBitsPerPixel
                    = Surface->resolveBitsPerPixel;
#endif /* __QNXNTO__ */
            }
            else if (Surface->fbWrapper != gcvNULL)
            {
                /* Get the aligned size of the surface and the stride. */
                gcmERR_BREAK(gcoSURF_GetAlignedSize(
                    Surface->fbWrapper,
                    &Surface->bitsAlignedWidth,
                    &Surface->bitsAlignedHeight,
                    &Surface->bitsStride
                    ));
            }

            /* Set render surface type. */
            gcmERR_BREAK(gcoSURF_SetColorType(
                Surface->renderTarget, Surface->colorType
                ));

            {
                gctPOINTER memory[3] = {gcvNULL};
                gctUINT8_PTR bits;
                gctINT bitsStride;

                /* Get the stride of render target. */
                gcmERR_BREAK(gcoSURF_GetAlignedSize(
                    Surface->renderTarget,
                    gcvNULL,
                    gcvNULL,
                    &bitsStride
                    ));

                /* Lock render target and clear it with zero. for fixing eglmage test bug. */
                gcmERR_BREAK(gcoSURF_Lock(
                    Surface->renderTarget,
                    gcvNULL,
                    memory
                    ));

                bits = (gctUINT8_PTR) memory[0];
                gcmERR_BREAK(gcoOS_ZeroMemory(
                    bits,
                    bitsStride * Surface->bitsHeight
                    ));

                gcmERR_BREAK(gcoSURF_Unlock(
                    Surface->renderTarget, memory[0]
                    ));
            }
#else
            gcmERR_BREAK(gcvSTATUS_GENERIC_IO);
#endif
        }

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
            "%s(%d): aligned size=%dx%d",
            __FUNCTION__, __LINE__,
            Surface->bitsAlignedWidth,
            Surface->bitsAlignedHeight
            );

        for (i = 0; i < EGL_WORKER_COUNT; i++)
        {
            Surface->workers[i].draw                   = gcvNULL;
            Surface->workers[i].backBuffer.origin.x    = -1;
            Surface->workers[i].backBuffer.origin.y    = -1;
            Surface->workers[i].backBuffer.size.width  = -1;
            Surface->workers[i].backBuffer.size.height = -1;

            Surface->workers[i].next  = Surface->availableWorkers;
            Surface->availableWorkers = &Surface->workers[i];
        }

        gcmERR_BREAK(gcoOS_CreateMutex(gcvNULL, &Surface->workerMutex));

        /* Success. */
        return EGL_SUCCESS;
    }
    while (gcvFALSE);

    /* Allocation failed. */
    gcmFATAL("Failed to allocate surface objects: %d", status);

    /* Roll back. */
    do
    {
        if (Surface->tempPixmapBits != gcvNULL)
        {
            gcmASSERT(Surface->pixmapSurface != gcvNULL);
            gcmERR_BREAK(gcoSURF_Unlock(
                Surface->pixmapSurface, Surface->tempPixmapBits
                ));
            Surface->tempPixmapBits = gcvNULL;
        }

        if (Surface->pixmapSurface != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Surface->pixmapSurface));
            Surface->pixmapSurface = gcvNULL;
        }

        if (Surface->resolveBits != gcvNULL)
        {
            gcmASSERT(Surface->resolve != gcvNULL);
            gcmERR_BREAK(gcoSURF_Unlock(
                Surface->resolve, Surface->resolveBits
                ));
            Surface->resolveBits = gcvNULL;
        }

        if (Surface->resolve != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Surface->resolve));
            Surface->resolve = gcvNULL;
        }

        if (Surface->depthBuffer != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Surface->depthBuffer));
            Surface->depthBuffer = gcvNULL;
        }

        if (Surface->renderTargetBits != gcvNULL)
        {
            gcmASSERT(Surface->renderTarget != gcvNULL);
            gcmERR_BREAK(gcoSURF_Unlock(
                Surface->renderTarget, Surface->renderTargetBits
                ));
            Surface->renderTargetBits = gcvNULL;
        }

        if (Surface->renderTarget != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Surface->renderTarget));

            Surface->renderTarget = gcvNULL;
        }

        if (Surface->fbWrapper != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Surface->fbWrapper));
            Surface->fbWrapper = gcvNULL;
        }
#ifndef __QNXNTO__
        /* 355_FB_MULTI_BUFFER. */
        if (Surface->fbWrappers[0] != gcvNULL)
        {
            /* Destroy the back buffer wrapper surface. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->fbWrappers[0]));
            Surface->fbWrappers[0] = gcvNULL;
        }
        if (Surface->fbWrappers[1] != gcvNULL)
        {
            /* Destroy the back buffer wrapper surface. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->fbWrappers[1]));
            Surface->fbWrappers[1] = gcvNULL;
        }
#endif
    }
    while (gcvFALSE);

    /* Return error code. */
    return EGL_BAD_ALLOC;
}

static gceSTATUS _DestroySurfaceObjects(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        /* Free the workers. */
        gctUINT32 i;

        /* Wait for workers to become idle. */
        for (i = 0; i < EGL_WORKER_COUNT; i++)
        {
            gcmASSERT(Surface->workers != gcvNULL);
            while(Surface->workers[i].draw != gcvNULL)
            {
                /* Sleep for a while */
                gcmVERIFY_OK(gcoOS_Delay(gcvNULL, 10));
            }
        }

        for (i = 0; i < EGL_WORKER_COUNT; i++)
        {
            /* Make sure all worker threads are finished. */
            if (Surface->workers[i].signal != gcvNULL)
            {
                gcmVERIFY_OK(
                    gcoOS_DestroySignal(gcvNULL, Surface->workers[i].signal));
                Surface->workers[i].signal = gcvNULL;
            }
        }

        /* Destroy worker mutex. */
        if (Surface->workerMutex != gcvNULL)
        {
            gcmERR_BREAK(
                gcoOS_DeleteMutex(gcvNULL, Surface->workerMutex));
        }

        if (Surface->tempPixmapBits != gcvNULL)
        {
            gcmASSERT(Surface->pixmapSurface != gcvNULL);
            gcmERR_BREAK(gcoSURF_Unlock(
                Surface->pixmapSurface, Surface->tempPixmapBits
                ));
            Surface->tempPixmapBits = gcvNULL;
        }

        if (Surface->pixmapSurface != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(Surface->pixmapSurface));
            Surface->pixmapSurface = gcvNULL;
        }

        if (Surface->resolveBits != gcvNULL)
        {
            /* Unlock the resolve buffer. */
            gcmASSERT(Surface->resolve != gcvNULL);
            gcmERR_BREAK(gcoSURF_Unlock(
                Surface->resolve, Surface->resolveBits
                ));
            Surface->resolveBits = gcvNULL;
        }

        if (Surface->resolve != gcvNULL)
        {
            /* Destroy the resolve buffer. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->resolve));
            Surface->resolve = gcvNULL;
        }

        if (Surface->depthBuffer != gcvNULL)
        {
#ifndef VIVANTE_NO_3D
            /* Flush pixels and disable the tile status. */
            gcmERR_BREAK(gcoSURF_DisableTileStatus(
                Surface->depthBuffer, gcvFALSE
                ));
#endif

            /* Destroy the depth buffer */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->depthBuffer));
            Surface->depthBuffer = gcvNULL;
        }

        if (Surface->renderTargetBits != gcvNULL)
        {
            gcmASSERT(Surface->renderTarget != gcvNULL);
            gcmERR_BREAK(gcoSURF_Unlock(
                Surface->renderTarget, Surface->renderTargetBits
                ));
            Surface->renderTargetBits = gcvNULL;
        }

        if (Surface->renderTarget != gcvNULL)
        {
#ifndef VIVANTE_NO_3D
            /* Flush pixels and disable the tile status. */
            gcmERR_BREAK(gcoSURF_DisableTileStatus(
                Surface->renderTarget, gcvFALSE
                ));
#endif

            /* Destroy the render target. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->renderTarget));
            Surface->renderTarget = gcvNULL;
        }

        if (Surface->fbWrapper != gcvNULL)
        {
            /* Destroy the frame buffer wrapper surface. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->fbWrapper));
            Surface->fbWrapper = gcvNULL;
        }

#ifndef __QNXNTO__
        /* 355_FB_MULTI_BUFFER */
        if (Surface->fbWrappers[0] != gcvNULL)
        {
            /* Destroy the back buffer wrapper surface. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->fbWrappers[0]));
            Surface->fbWrappers[0] = gcvNULL;
        }
        if (Surface->fbWrappers[1] != gcvNULL)
        {
            /* Destroy the back buffer wrapper surface. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->fbWrappers[1]));
            Surface->fbWrappers[1] = gcvNULL;
        }
#endif


#ifdef __QNXNTO__
        if (Surface->fbWrapper2 != gcvNULL)
        {
            /* Destroy the frame buffer wrapper surface. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->fbWrapper2));
            Surface->fbWrapper2 = gcvNULL;
        }
#endif /* __QNXNTO__ */

        if (Surface->lockBuffer != gcvNULL)
        {
            /* Destroy the bitmap surface. */
            gcmERR_BREAK(gcoSURF_Destroy(Surface->lockBuffer));
            Surface->lockBuffer = gcvNULL;
            Surface->lockBits   = gcvNULL;
        }

        /* Destroy render surface array. */
        gcmERR_BREAK(veglFreeRenderList(Thread, Surface));
    }
    while (gcvFALSE);

    /* Return status. */
    return status;
}

EGLint veglResizeSurface(
    IN VEGLSurface Surface,
    IN gctUINT Width,
    IN gctUINT Height,
    gceSURF_FORMAT ResolveFormat,
    gctUINT BitsPerPixel
    )
{
    EGLint eglResult;
    gceSTATUS status;

    gcmHEADER_ARG("Surface=0x%x Width=%u Height=%u ResolveFormat=%d "
                  "BitsPerPixel=%u",
                  Surface, Width, Height, ResolveFormat, BitsPerPixel);

    do
    {
        VEGLThreadData thread;
        EGLBoolean result;

        /* Get thread data. */
        thread = veglGetThreadData();
        if (thread == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): veglGetThreadData failed.",
                __FUNCTION__, __LINE__
                );

            eglResult = EGL_BAD_SURFACE;
            break;
        }

        /* Destroy existing objects. */
        status = _DestroySurfaceObjects(thread, Surface);

        if (gcmIS_ERROR(status))
        {
            /* Error feeing surface objects. */
            eglResult = EGL_BAD_ALLOC;
            break;
        }

        /* Set new parameters. */
        Surface->bitsWidth          = Width;
        Surface->bitsHeight         = Height;
        Surface->bitsPerPixel       = BitsPerPixel;
        Surface->config.width       = Width;
        Surface->config.height      = Height;

#if gcdSUPPORT_SWAP_RECTANGLE
        /* Set the swap rectangle size. */
        Surface->swapOrigin.x = 0;
        Surface->swapOrigin.y = 0;
        Surface->swapSize.x   = Width;
        Surface->swapSize.y   = Height;

        /* Reset buffer count. */
        Surface->bufferCount  = 0;
#endif

        /* Create surface objects. */
        eglResult = _CreateSurfaceObjects(thread, Surface, ResolveFormat);

        if (eglResult != EGL_SUCCESS)
        {
            break;
        }

        /* Update the driver with the new objects. */
        result = _SetApiContext(
            thread,
            thread->context,
            thread->context->context,
            Surface->renderTarget,
            thread->context->read->renderTarget,
            Surface->depthBuffer
            );

        if (!result)
        {
            /* Error feeing surface objects. */
            eglResult = EGL_BAD_ALLOC;
            break;
        }

        /* Set the target. */
        if (gcmIS_ERROR(veglSetDriverTarget(thread, Surface)))
        {
            /* Error feeing surface objects. */
            eglResult = EGL_BAD_ALLOC;
            break;
        }

        /* Return error code. */
        gcmFOOTER_ARG("return=%d", EGL_SUCCESS);

        /* Success. */
        return EGL_SUCCESS;
    }
    while (gcvFALSE);

    /* Return error code. */
    gcmFOOTER_ARG("return=%d", eglResult);
    return eglResult;
}

static void veglGetFormat(
    IN VEGLThreadData Thread,
    IN VEGLConfig Config,
    OUT gceSURF_FORMAT * RenderTarget,
    OUT gceSURF_FORMAT * DepthBuffer
    )
{
    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
        "%s(%d): config %d RGBA sizes=%d, %d, %d, %d; depth size=%d",
        __FUNCTION__, __LINE__,
        Config->configId,
        Config->redSize,
        Config->greenSize,
        Config->blueSize,
        Config->alphaSize,
        Config->depthSize
        );
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
    if (Thread->openVGpipe &&
        (Config->greenSize != 8) && (Config->greenSize != 6))
    {
        gcmTRACE_ZONE(
            gcvLEVEL_WARNING, gcdZONE_EGL_SURFACE,
            "%s(%d): unsupported OpenVG target",
            __FUNCTION__, __LINE__
            );
    }
#endif

    switch (Config->greenSize)
    {
    case 4:
        *RenderTarget = (Config->alphaSize == 0)
            ? gcvSURF_X4R4G4B4
            : gcvSURF_A4R4G4B4;
        break;

    case 5:
        *RenderTarget = (Config->alphaSize == 0)
            ? gcvSURF_X1R5G5B5
            : gcvSURF_A1R5G5B5;
        break;

    case 6:
        *RenderTarget = gcvSURF_R5G6B5;
        break;

    case 8:
        if (Config->bufferSize == 16)
        {
            gcmASSERT(Thread->openVGpipe == gcvFALSE);

            *RenderTarget = gcvSURF_YUY2;
        }
        else
        {
            *RenderTarget = (Config->alphaSize == 0)
                ? gcvSURF_X8R8G8B8
                : gcvSURF_A8R8G8B8;
        }
        break;

    default:
        gcmFATAL("Unsupported format (green size=%d)", Config->greenSize);
    }

    switch (Config->depthSize)
    {
    case 0:
        *DepthBuffer = gcvSURF_UNKNOWN;
        break;

    case 16:
        *DepthBuffer = gcvSURF_D16;
        break;

    case 24:
        if (Config->stencilSize > 0)
        {
            *DepthBuffer = gcvSURF_D24S8;
        }
        else
        {
            *DepthBuffer = gcvSURF_D24X8;
        }
        break;

    default:
        gcmFATAL("Unsupported format (depth size=%d)", Config->depthSize);
    }

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
        "%s(%d): chosen render target=%d; chosen depth=%d",
        __FUNCTION__, __LINE__,
        *RenderTarget,
        *DepthBuffer
        );
}

static VEGLSurface
_InitializeSurface(
    IN VEGLThreadData Thread,
    IN VEGLConfig Config,
    IN EGLint Type
    )
{
    VEGLSurface surface;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    /* Allocate the surface structure. */
    status = gcoOS_Allocate(gcvNULL,
                            sizeof(struct eglSurface),
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Out of memory. */
        Thread->error = EGL_BAD_ALLOC;
        return gcvNULL;
    }

    gcmVERIFY_OK(gcoOS_ZeroMemory(pointer,
                                  sizeof(struct eglSurface)));

    surface = pointer;

    /* Initialize the surface object. */
    surface->signature        = EGL_SURFACE_SIGNATURE;

    surface->bitsPerPixel     = -1;
    surface->bitsWidth        = -1;
    surface->bitsHeight       = -1;
    surface->bitsStride       = -1;

    surface->created          = gcvTRUE;

    surface->type             = Type;
    surface->buffer           = EGL_BACK_BUFFER;
    surface->colorType        = gcvSURF_COLOR_UNKNOWN;

    if (Type & EGL_VG_COLORSPACE_LINEAR_BIT)
    {
        surface->colorType |= gcvSURF_COLOR_LINEAR;
    }

    if (Type & EGL_VG_ALPHA_FORMAT_PRE_BIT)
    {
        surface->colorType |= gcvSURF_COLOR_ALPHA_PRE;
    }

    surface->swapBehavior     = EGL_BUFFER_PRESERVED;

    /* PBuffer attributes. */
    surface->textureFormat    = EGL_NO_TEXTURE;
    surface->textureTarget    = EGL_NO_TEXTURE;

    /* Pixmap attributes. */
    surface->pixmapSurface    = gcvNULL;
    surface->pixmapStride     = -1;
    surface->pixmapBits       = gcvNULL;

    surface->tempPixmapStride = -1;
    surface->tempPixmapBits   = gcvNULL;
    surface->tempPixmapPhys   = ~0U;

#if defined(ANDROID) && (ANDROID_SDK_VERSION >= 11)
    /* Set first frame flag. */
    surface->firstFrame       = EGL_TRUE;
#endif

#ifndef __QNXNTO__
    /* 355_FB_MULTI_BUFFER */
    surface->fbWrappers[0] = surface->fbWrappers[1] = gcvNULL;
#endif

	if (Config != gcvNULL)
    {
        /* Get configuration formats. */
        veglGetFormat(
            Thread,
            Config,
            &surface->renderTargetFormat,
            &surface->depthFormat
            );

        /* Query the format specifics. */
        gcmONERROR(gcoSURF_QueryFormat(
            surface->renderTargetFormat,
            surface->renderTargetInfo
            ));

        /* Copy config information */
        gcoOS_MemCopy(&surface->config, Config, sizeof(surface->config));
    }
OnError:
    /* Success. */
    return surface;
}

static EGLint
_CreateSurface(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN VEGLSurface Surface
    )
{
    EGLint eglResult;

    do
    {
        EGLint surfaceType;
        gctUINT width, height;
        gceSURF_FORMAT resolveFormat = gcvSURF_UNKNOWN;
        gctUINT bitsPerPixel;

        /* Determine the surface type. */
        surfaceType
            = Surface->type
            & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT | EGL_PIXMAP_BIT);

        /***********************************************************************
        ** WINDOW SURFACE.
        */

        if (surfaceType == EGL_WINDOW_BIT)
        {
            gctBOOL result;
            gctUINT32 baseAddress;

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                "%s(%d): EGL_WINDOW_BIT",
                __FUNCTION__, __LINE__
                );

            if (Surface->hwnd == (NativeWindowType) gcvNULL)
            {
                /* No window. */
                eglResult = EGL_BAD_NATIVE_WINDOW;
                break;
            }

            /* Query window parameters. */
            result = veglGetWindowInfo(
                Display,
                Surface->hwnd,
                &Surface->bitsX,
                &Surface->bitsY,
                &width, &height,
                &bitsPerPixel,
                &resolveFormat
                );
#ifdef __QNXNTO__
            /* HACK. for now for QNX. */
            Surface->bitsX = 0;
            Surface->bitsY = 0;
#endif

            if (!result)
            {
                /* Window is gone. */
                eglResult = EGL_BAD_NATIVE_WINDOW;
                break;
            }

            /* Get window memory location. */
            Surface->fbDirect = veglGetDisplayInfo(
                Display,
                Surface->hwnd,
                &Surface->fbInfo
                );

            if (Surface->fbInfo.width  == -1) Surface->fbInfo.width  = width;
            if (Surface->fbInfo.height == -1) Surface->fbInfo.height = height;

            if ((Surface->fbDirect        == gcvTRUE) &&
               ((Surface->fbInfo.width    == 0) ||
                (Surface->fbInfo.height   == 0) ||
                (Surface->fbInfo.stride   == 0) ||
                (Surface->fbInfo.physical == 0)))
            {
                /* Window is gone. */
                eglResult = EGL_BAD_NATIVE_WINDOW;
                break;
            }

            /* Get base address */
            if (gcmIS_ERROR(gcoOS_GetBaseAddress(gcvNULL, &baseAddress)))
            {
                eglResult = EGL_BAD_ACCESS;
                break;
            }

            Surface->fbInfo.physical -= baseAddress;
#ifdef __QNXNTO__
            Surface->fbInfo.physical2 -= baseAddress;
#endif

            /* Save width and height. */
            Surface->config.width  = width;
            Surface->config.height = height;

            /* Set resove bpp. */
            Surface->resolveBitsPerPixel = bitsPerPixel;
        }

        /***********************************************************************
        ** PBUFFER SURFACE.
        */

        else if (surfaceType == EGL_PBUFFER_BIT)
        {
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                "%s(%d): EGL_PBUFFER_BIT",
                __FUNCTION__, __LINE__
                );

            /* Set the window origin. */
            Surface->bitsX = 0;
            Surface->bitsY = 0;

            /* Set width and height from configuration. */
            width        = Surface->config.width;
            height       = Surface->config.height;

            /* Resolve buffer is the same format as the render target. */
            bitsPerPixel  = Surface->config.bufferSize;
            resolveFormat = Surface->renderTargetFormat;

            /* Set resove bpp. */
            Surface->resolveBitsPerPixel
                = Surface->renderTargetInfo[0]->bitsPerPixel;
        }

        /***********************************************************************
        ** PIXMAP SURFACE.
        */

        else if (surfaceType == EGL_PIXMAP_BIT)
        {
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcdZONE_EGL_SURFACE,
                "%s(%d): EGL_PIXMAP_BIT",
                __FUNCTION__, __LINE__
                );

#ifndef __QNXNTO__
            if (Surface->pixmap == (NativePixmapType) gcvNULL)
            {
                /* No window. */
                eglResult = EGL_BAD_NATIVE_PIXMAP;
                break;
            }
#endif

            /* Query the pixmap information. */
            if (!veglGetPixmapInfo(Display->hdc,
                                   Surface->pixmap,
                                   &width, &height,
                                   &bitsPerPixel,
                                   &resolveFormat))
            {
                /* Bad pixmap. */
                eglResult = EGL_BAD_NATIVE_PIXMAP;
                break;
            }

            if (!veglGetPixmapBits(Display->hdc,
                                   Surface->pixmap,
                                   &Surface->pixmapBits,
                                   &Surface->pixmapStride,
                                   gcvNULL))
            {
                /* Bad pixmap. */
                eglResult = EGL_BAD_NATIVE_PIXMAP;
                break;
            }

            /* Set the window origin. */
            Surface->bitsX = 0;
            Surface->bitsY = 0;

            /* Set configuration width and height. */
            Surface->config.width  = width;
            Surface->config.height = height;

            /* Set resove bpp. */
            Surface->resolveBitsPerPixel = bitsPerPixel;
        }

        /***********************************************************************
        ** UNKNOWN SURFACE.
        */

        else
        {
            gcmFATAL("ERROR: Unknown surface type: 0x%04X", surfaceType);
            eglResult = EGL_BAD_PARAMETER;
            break;
        }

        /* Set the size. */
        Surface->bitsWidth    = width;
        Surface->bitsHeight   = height;
        Surface->bitsPerPixel = bitsPerPixel;

#if gcdSUPPORT_SWAP_RECTANGLE
        /* Set the swap rectangle size. */
        Surface->swapOrigin.x  = 0;
        Surface->swapOrigin.y  = 0;
        Surface->swapSize.x    = width;
        Surface->swapSize.y    = height;

        /* Reset buffer count. */
        Surface->bufferCount  = 0;
#endif

        /* Determine whether OpenVG pipe is present and OpenVG API is active. */
        Surface->openVG = Thread->openVGpipe && (Thread->api == EGL_OPENVG_API);
#if gcdENABLE_VG
        if (Surface->openVG)
        {
            gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_VG));
            Surface->resolveFormat = Surface->renderTargetFormat;
        }
#endif
        /* Create surface objects. */
        eglResult = _CreateSurfaceObjects(Thread, Surface, resolveFormat);

        if (eglResult != EGL_SUCCESS)
        {
            break;
        }

        /* Dump the surface. */
        _veglAddSurface(Thread, Surface);

        /* Success. */
        return EGL_SUCCESS;
    }
    while (gcvFALSE);

    /* Return the error code. */
    return eglResult;
}

static void
_DestroySurface(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    gcmVERIFY_OK(_DestroySurfaceObjects(Thread, Surface));

    if (Surface->reference != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_AtomDestroy(gcvNULL, Surface->reference));
        Surface->reference = gcvNULL;
    }
}

static EGLBoolean
_MapLockedBuffer(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    gceSTATUS status;
    gctPOINTER lockBits[3] = {gcvNULL};

    /* Create a bitmap surface. */
    status = gcoSURF_Construct(
        gcvNULL,
        Surface->bitsWidth,
        Surface->bitsHeight, 1,
        gcvSURF_BITMAP,
        Surface->renderTargetFormat,
        gcvPOOL_DEFAULT,
        &Surface->lockBuffer
        );

    /* Create fail. */
    if (gcmIS_ERROR(status))
    {
        Thread->error = EGL_BAD_ACCESS;
        return EGL_FALSE;
    }

    /* Lock bitmap address. */
    status = gcoSURF_Lock(
        Surface->lockBuffer,
        gcvNULL,
        lockBits
        );
    Surface->lockBits = lockBits[0];

    if (gcmIS_ERROR(status))
    {
        /* Roll back and return false. */
        gcoSURF_Destroy(Surface->lockBuffer);

        Thread->error = EGL_BAD_ACCESS;
        return EGL_FALSE;
    }

    Surface->lockBufferStride = Surface->bitsStride;

    /* If Preserve bit is set. */
    if (Surface->lockPreserve)
    {
        /* Resolve color buffer to bitmap. */
        status = gcoSURF_Resolve(Surface->renderTarget,
                                 Surface->lockBuffer);
        if(gcmIS_ERROR(status))
        {
            Thread->error = EGL_BAD_ACCESS;
            return EGL_FALSE;
        }

        status = gcoHAL_Commit(gcvNULL, gcvTRUE);
        if (gcmIS_ERROR(status))
        {
            Thread->error = EGL_BAD_ACCESS;
            return EGL_FALSE;
        }
    }

    Thread->error = EGL_SUCCESS;
    return EGL_TRUE;
}

static EGLint
_GetLockedBufferPixelChannelOffset(
    IN VEGLSurface Surface,
    IN EGLint      Attribute
    )
{
    gceSURF_FORMAT format;
    EGLint         redOffset       = 0;
    EGLint         greenOffset     = 0;
    EGLint         blueOffset      = 0;
    EGLint         alphaOffset     = 0;
    EGLint         luminanceOffset = 0;

    if (!Surface->locked || !Surface->lockBuffer)
    {
        return 0;
    }

    format = Surface->lockBufferFormat;

    switch(format)
    {
    case gcvSURF_A8R8G8B8:
        alphaOffset = 24;
        redOffset   = 16;
        greenOffset = 8;
        break;

    case gcvSURF_X8R8G8B8:
        redOffset   = 16;
        greenOffset = 8;
        break;

    case gcvSURF_R5G6B5:
        redOffset   = 11;
        greenOffset = 6;
        break;

    case gcvSURF_A4R4G4B4:
        alphaOffset = 12;
        redOffset   = 8;
        greenOffset = 4;
        break;

    case gcvSURF_A1R5G5B5:
        alphaOffset = 15;
        redOffset   = 10;
        greenOffset = 5;
        break;

    default:
        break;
    }

    switch (Attribute)
    {
    case EGL_BITMAP_PIXEL_RED_OFFSET_KHR:
        return redOffset;

    case EGL_BITMAP_PIXEL_GREEN_OFFSET_KHR:
        return greenOffset;

    case EGL_BITMAP_PIXEL_BLUE_OFFSET_KHR:
        return blueOffset;

    case EGL_BITMAP_PIXEL_ALPHA_OFFSET_KHR:
        return alphaOffset;

    case EGL_BITMAP_PIXEL_LUMINANCE_OFFSET_KHR:
        return luminanceOffset;

    default:
        break;
    }

    return 0;
}

EGLBoolean
veglReferenceSurface(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    EGLint error;
    gctINT32 reference;

    gcmHEADER_ARG("Thread=0x%x Surface=0x%x", Thread, Surface);

    if (Surface->reference == gcvNULL)
    {
        if (gcmIS_ERROR(gcoOS_AtomConstruct(gcvNULL, &Surface->reference)))
        {
            /* Out of memory. */
            Thread->error = EGL_BAD_ALLOC;
            gcmFOOTER_ARG("return=%d", EGL_FALSE);
            return EGL_FALSE;
        }
    }

    /* Increment surface reference counter. */
    if (gcmIS_ERROR(gcoOS_AtomIncrement(gcvNULL,
                                        Surface->reference,
                                        &reference)))
    {
        /* Error. */
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if ((reference > 0) || Surface->created)
    {
        /* The surface has already been referenced. */
        Surface->created = gcvFALSE;
        gcmFOOTER_ARG("return=%d", EGL_TRUE);
        return EGL_TRUE;
    }

    error = _CreateSurface(Thread, Thread->context->display, Surface);

    if (error == EGL_SUCCESS)
    {
        /* Success. */
        gcmFOOTER_ARG("return=%d", EGL_TRUE);
        return EGL_TRUE;
    }

    /* Roll back. */
    veglDereferenceSurface(Thread, Surface, EGL_TRUE);

    /* Error. */
    gcmFOOTER_ARG("return=%d", EGL_FALSE);
    return EGL_FALSE;
}

void
veglDereferenceSurface(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface,
    IN EGLBoolean Always
    )
{
    gctINT32 reference = 0;

    gcmHEADER_ARG("Thread=0x%x Surface=0x%x Always=%d", Thread, Surface, Always);

    if (Surface->reference != gcvNULL)
    {
        /* Decrement surface reference counter. */
        gcmVERIFY_OK(gcoOS_AtomDecrement(gcvNULL,
                                         Surface->reference,
                                         &reference));
    }

    if ((reference == 1) || Always)
    {
        _DestroySurface(Thread, Surface);
    }

    gcmFOOTER_NO();
}

gceSTATUS
veglAddRenderListSurface(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    gceSTATUS status, last;

    struct eglRenderList * entry = gcvNULL;
    gctSIGNAL signal = gcvNULL;
    gcoSURF surface = gcvNULL;
    gctPOINTER bits[3] = {gcvNULL};

    gcmHEADER_ARG("Thread=0x%x Surface=0x%x", Thread, Surface);

    do
    {
        gctPOINTER pointer = gcvNULL;

        /* Multi-buffer support must be enabled. */
        gcmASSERT(Surface->renderListEnable);

        /* The entry count cannot be greater then allowed. */
        gcmASSERT(Surface->renderListCount <= veglRENDER_LIST_LENGTH);

        /* All allowed entries are already allocated? */
        if (Surface->renderListCount == veglRENDER_LIST_LENGTH)
        {
            status = gcvSTATUS_NO_MORE_DATA;
            break;
        }

        /* Allocate the entry. */
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            gcmSIZEOF(struct eglRenderList),
            &pointer
            ));

        entry = pointer;

        /* Create the surface signal. */
        gcmERR_BREAK(gcoOS_CreateSignal(gcvNULL, gcvFALSE, &signal));

        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcvZONE_SIGNAL,
            "%s(%d): surface signal created 0x%08X",
            __FUNCTION__, __LINE__,
            signal
            );

        /* Set the signal. */
        gcmERR_BREAK(gcoOS_Signal(gcvNULL, signal, gcvTRUE));

        /* Create the surface. */
        gcmERR_BREAK(gcoSURF_Construct(
            gcvNULL,
            Surface->bitsWidth,
            Surface->bitsHeight, 1,
            gcvSURF_BITMAP,
            Surface->renderTargetFormat,
            gcvPOOL_DEFAULT,
            &surface
            ));

        /* Lock the surface. */
        gcmERR_BREAK(gcoSURF_Lock(surface, gcvNULL, bits));

        /* Set orientation. */
        gcmERR_BREAK(gcoSURF_SetOrientation(
            surface, gcvORIENTATION_TOP_BOTTOM
            ));

        /* Set render surface type. */
        gcmERR_BREAK(gcoSURF_SetColorType(
            surface, Surface->colorType
            ));

        /* Link in the entry. */
        if (Surface->renderListCurr == gcvNULL)
        {
            entry->prev =
            entry->next =
            Surface->renderListCurr =
            Surface->renderListHead = entry;
        }
        else
        {
            entry->prev = Surface->renderListCurr;
            entry->next = Surface->renderListCurr->next;

            Surface->renderListCurr->next->prev = entry;
            Surface->renderListCurr->next       = entry;
        }

        /* Initialize the entry. */
        entry->signal  = signal;
        entry->surface = surface;
        entry->bits    = bits[0];

        gcmTRACE_ZONE(
            gcvLEVEL_INFO, gcdZONE_EGL_SURFACE,
            "%s(%d): surface 0x%08X, signal 0x%08X",
            __FUNCTION__, __LINE__,
            surface, signal
            );

        /* Update the number of entries. */
        Surface->renderListCount += 1;

        /* Success. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    /* Roll back. */
    if (entry != gcvNULL)
    {
        gcmCHECK_STATUS(gcmOS_SAFE_FREE(gcvNULL, entry));
    }

    if (signal != gcvNULL)
    {
        gcmCHECK_STATUS(gcoOS_DestroySignal(gcvNULL, signal));
    }

    if (bits[0] != gcvNULL)
    {
        gcmCHECK_STATUS(gcoSURF_Unlock(surface, bits[0]));
    }

    if (surface != gcvNULL)
    {
        gcmCHECK_STATUS(gcoSURF_Destroy(surface));
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
veglFreeRenderList(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Thread=0x%x Surface=0x%x", Thread, Surface);

    /* Delete in reverse order. */
    while (Surface->renderListCount > 0)
    {
        /* Get a shortcut to the entry. */
        struct eglRenderList * entry = Surface->renderListCurr;

        /* Advance to the next entry. */
        Surface->renderListCurr = entry->next;

        if (entry->signal != gcvNULL)
        {
            gcmERR_BREAK(gcoOS_DestroySignal(gcvNULL, entry->signal));
            entry->signal = gcvNULL;
        }

        if (entry->bits != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Unlock(entry->surface, entry->bits));
            entry->bits = gcvNULL;
        }

        if (entry->surface != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_Destroy(entry->surface));
            entry->surface = gcvNULL;
        }

        gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, entry));

        /* Update the number of entries. */
        Surface->renderListCount -= 1;
    }

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
veglSetDriverTarget(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Thread=0x%x Surface=0x%x", Thread, Surface);

    do
    {
        struct eglRenderList * current;

        /* Is multi-buffer rendering enabled? */
        if (!Surface->renderListEnable)
        {
            /* No, ignore the call. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Get a pointer to the current surface. */
        current = Surface->renderListCurr;

        /* Acquire the surface's mutex. */
        status = gcoOS_WaitSignal(
            gcvNULL, current->signal, 0
            );

        /* Only timeout error is allowed. */
        if (status != gcvSTATUS_TIMEOUT)
        {
            break;
        }

        /* Set the next surface as the next render target. */
        gcmERR_BREAK(_SetBuffer(Thread, current->surface));
    }
    while (gcvFALSE);

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
veglDriverSurfaceSwap(
    IN VEGLThreadData Thread,
    IN VEGLSurface Surface
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Thread=0x%x Surface=0x%x", Thread, Surface);

    do
    {
        struct eglRenderList * nextSurface;

        /* Is multi-buffer rendering enabled? */
        if (!Surface->renderListEnable)
        {
            /* No, ignore the call. */
            status = gcvSTATUS_OK;
            break;
        }

        /* Get the pointer to the next  */
        nextSurface = Surface->renderListCurr->next;

        /* Attempt to acquire the next surface's mutex. */
        status = gcoOS_WaitSignal(
            gcvNULL, nextSurface->signal, 0
            );

        /* Error? */
        if (gcmIS_ERROR(status))
        {
            /* Only timeout error is allowed. */
            if (status != gcvSTATUS_TIMEOUT)
            {
                break;
            }

            /* Attempt to create a new surface. */
            gcmERR_BREAK(veglAddRenderListSurface(Thread, Surface));

            /* Was the new surface created? */
            if (gcmIS_SUCCESS(status))
            {
                /* Update the next surface pointer. */
                nextSurface = Surface->renderListCurr->next;
            }

            /* Acquire the next surface's mutex. */
            gcmERR_BREAK(gcoOS_WaitSignal(
                gcvNULL, nextSurface->signal, gcvINFINITE
                ));
        }

        /* Advance to the next surface. */
        Surface->renderListCurr = nextSurface;

        /* Set the new surface to the driver. */
        gcmERR_BREAK(veglSetDriverTarget(Thread, Surface));
    }
    while (gcvFALSE);

    /* Return status. */
    gcmFOOTER();
    return status;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface(
    EGLDisplay Dpy,
    EGLConfig config,
    NativeWindowType window,
    const EGLint* attrib_list
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    EGLenum renderBuffer = EGL_BACK_BUFFER;
    VEGLSurface surface;
    EGLint error;
    EGLint type = EGL_WINDOW_BIT;

    gcmHEADER_ARG("Dpy=0x%x config=0x%x window=%d attrib_list=0x%x",
                    Dpy, config, window, attrib_list);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Parse attributes. */
    if (attrib_list != gcvNULL)
    {
        EGLint i;

        for (i = 0; attrib_list[i] != EGL_NONE; i += 2)
        {
            switch (attrib_list[i])
            {
            case EGL_RENDER_BUFFER:
                renderBuffer = attrib_list[i + 1];
                break;

            case EGL_COLORSPACE:
                if (attrib_list[i + 1] == EGL_COLORSPACE_LINEAR)
                {
                    type |= EGL_VG_COLORSPACE_LINEAR_BIT;
                }
                break;

            case EGL_ALPHA_FORMAT:
                if (attrib_list[i + 1] == EGL_ALPHA_FORMAT_PRE)
                {
                    type |= EGL_VG_ALPHA_FORMAT_PRE_BIT;
                }
                break;
            }
        }
    }

    /* Create and initialize the surface. */
    surface = _InitializeSurface(thread, config, type);

    if (surface == EGL_NO_SURFACE)
    {
        /* Bad allocation. */
        thread->error = EGL_BAD_ALLOC;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    if (thread->openVGpipe && thread->api == EGL_OPENVG_API)
    {
        if (!veglIsColorFormatSupport(dpy->hdc, &surface->config))
        {
			/* Roll back. */
			_DestroySurface(thread, surface);
			gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface));
            /* Bad match. */
            thread->error = EGL_BAD_MATCH;
            gcmFOOTER_ARG("%d", EGL_FALSE);
            return EGL_NO_SURFACE;
        }
    }


    /* Set Window attributes. */
    surface->hwnd   = window;
    surface->buffer = renderBuffer;

    /* Create the surface. */
    error = _CreateSurface(thread, dpy, surface);

    if (error != EGL_SUCCESS)
    {
        /* Roll back. */
        _DestroySurface(thread, surface);
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface));

        thread->error = error;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Push surface into stack. */
    surface->next = dpy->surfaceStack;
    dpy->surfaceStack = surface;

    /* Reference surface. Temp fix for lock surface. */
    veglReferenceSurface(thread, surface);

    /* Stash the VEGLSurface in PLS. This is used in Android HWC. */
    gcoOS_SetPLSValue(gcePLS_VALUE_EGL_SURFACE_INFO, (gctPOINTER) surface);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglCreateWindowSurface 0x%08X 0x%08X 0x%08X (0x%08X) := "
                "0x%08X (%dx%d)",
                Dpy, config, window, attrib_list, surface,
                surface->config.width, surface->config.height);
    gcmDUMP_API_ARRAY_TOKEN(attrib_list, EGL_NONE);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("return=0x%x", surface);
    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySurface(
    EGLDisplay Dpy,
    EGLSurface Surface
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface surface;
    VEGLSurface stack;

    gcmHEADER_ARG("Dpy=0x%x Surface=0x%x", Dpy, Surface);
    gcmDUMP_API("${EGL eglDestroySurface 0x%08X 0x%08X}", Dpy, Surface);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create shortcuts to objects. */
    dpy     = VEGL_DISPLAY(Dpy);
    surface = VEGL_SURFACE(Surface);

    /* Test for valid EGLDisplay structure */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for valid EGLSurface structure */
    for (stack = dpy->surfaceStack;
         stack != gcvNULL;
         stack = stack->next)
    {
        if (stack == surface)
            break;
    }
    if (surface == gcvNULL ||
        stack == gcvNULL)
    {
        thread->error = EGL_BAD_SURFACE;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if surface is locked. */
    if (surface->locked)
    {
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* we should deference the surface, not destroy, since others are maybe using it*/
    veglDereferenceSurface(thread, surface, EGL_FALSE);

    /* Pop EGLSurface from the stack. */
    if (surface == dpy->surfaceStack)
    {
        /* Simple - it is the top of the stack. */
        dpy->surfaceStack = surface->next;
    }
    else
    {
        /* Walk the stack. */
        for (stack = dpy->surfaceStack;
             stack != gcvNULL;
             stack = stack->next)
        {
            /* Check if the next surface on the stack is ours. */
            if (stack->next == surface)
            {
                /* Pop the surface from the stack. */
                stack->next = surface->next;
                break;
            }
        }
    }

    /*if the surface has been destoried then free the struct */
    if (surface->reference == gcvNULL)
    {
        /* Free the EglSurface structure. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface));
    }

    /* Invalidate the VEGLSurface in PLS. */
    gcoOS_SetPLSValue(gcePLS_VALUE_EGL_SURFACE_INFO, (gctPOINTER) gcvNULL);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferSurface(
    IN EGLDisplay Dpy,
    IN EGLConfig Config,
    IN const EGLint * attrib_list
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLConfig config;
    VEGLSurface surface;
    EGLint error;
    EGLint i;

    /* Default attributes. */
    EGLint width   = 0;
    EGLint height  = 0;
    EGLint largest = EGL_FALSE;
    EGLint format  = EGL_NO_TEXTURE;
    EGLint target  = EGL_NO_TEXTURE;
    EGLint mipmap  = EGL_FALSE;
    EGLint type = EGL_PBUFFER_BIT;

    gcmHEADER_ARG("Dpy=0x%x Config=0x%x attrib_list=0x%x",
                  Dpy, Config, attrib_list);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Create shortcuts to objects. */
    dpy    = VEGL_DISPLAY(Dpy);
    config = VEGL_CONFIG(Config);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Test if the config is a valid one. */
    for (i = 0; i < dpy->configCount; i++)
    {
        if (&dpy->config[i] == config)
            break;
    }

    if (i >= dpy->configCount)
    {
        thread->error = EGL_BAD_CONFIG;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Determine if the configuration is valid. */
    if (!(config->surfaceType & EGL_PBUFFER_BIT))
    {
        thread->error = EGL_BAD_MATCH;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Parse attributes. */
    if (attrib_list != gcvNULL)
    {
        for (i = 0; attrib_list[i] != EGL_NONE; i += 2)
        {
            switch (attrib_list[i])
            {
            case EGL_WIDTH:
                width = attrib_list[i + 1];
                if (width < 0)
                {
                    thread->error = EGL_BAD_PARAMETER;
                    gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
                    return EGL_NO_SURFACE;
                }
                break;

            case EGL_HEIGHT:
                height = attrib_list[i + 1];
                if (height < 0)
                {
                    thread->error = EGL_BAD_PARAMETER;
                    gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
                    return EGL_NO_SURFACE;
                }
                break;

            case EGL_LARGEST_PBUFFER:
                largest = attrib_list[i + 1];
                break;

            case EGL_TEXTURE_FORMAT:
                format = attrib_list[i + 1];
                break;

            case EGL_TEXTURE_TARGET:
                target = attrib_list[i + 1];
                break;

            case EGL_MIPMAP_TEXTURE:
                mipmap = attrib_list[i + 1];
                break;

            case EGL_COLORSPACE:
                if (attrib_list[i + 1] == EGL_COLORSPACE_LINEAR)
                {
                    type |= EGL_VG_COLORSPACE_LINEAR_BIT;
                }
                break;

            case EGL_ALPHA_FORMAT:
                if (attrib_list[i + 1] == EGL_ALPHA_FORMAT_PRE)
                {
                    type |= EGL_VG_ALPHA_FORMAT_PRE_BIT;
                }
                break;
            }
        }
    }

    /* Get width from configuration is not specified. */
    if (width == 0)
    {
        width = config->width;
    }

    /* Get height from configuration is not specified. */
    if (height == 0)
    {
        height = config->height;
    }

    if ((width  < 1)
    ||  (width  > thread->maxWidth)
    ||  (height < 1)
    || (height > thread->maxHeight)
    )
    {
        thread->error = EGL_BAD_MATCH;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Create surface object. */
    surface = _InitializeSurface(thread, config, type);

    if (surface == EGL_NO_SURFACE)
    {
        /* Bad allocation. */
        thread->error = EGL_BAD_ALLOC;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Set PBuffer attributes. */
    surface->config.width   = width;
    surface->config.height  = height;
    surface->largestPBuffer = largest;
    surface->mipmapTexture  = mipmap;
    surface->textureFormat  = format;
    surface->textureTarget  = target;

    /* Create the surface. */
    error = _CreateSurface(thread, dpy, surface);

    if (error != EGL_SUCCESS)
    {
        /* Roll back. */
        _DestroySurface(thread, surface);
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface));

        /* Return error. */
        thread->error = error;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Push surface into stack. */
    surface->next = dpy->surfaceStack;
    dpy->surfaceStack = surface;

    veglReferenceSurface(thread, surface);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglCreatePbufferSurface 0x%08X 0x%08X (0x%08X) := "
                "0x%08X (%dx%d)",
                Dpy, Config, attrib_list, surface, surface->config.width,
                surface->config.height);
    gcmDUMP_API_ARRAY_TOKEN(attrib_list, EGL_NONE);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("return=0x%x", surface);
    return surface;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferFromClientBuffer(
    EGLDisplay Dpy,
    EGLenum buftype,
    EGLClientBuffer buffer,
    EGLConfig Config,
    const EGLint *attrib_list
    )
{

    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLConfig config;
    VEGLSurface surface;
    gceSTATUS status;

    /* Default attributes. */
    EGLint largest = EGL_FALSE;
    EGLint format  = EGL_NO_TEXTURE;
    EGLint target  = EGL_NO_TEXTURE;
    EGLint mipmap  = EGL_FALSE;

    gcmHEADER_ARG("Dpy=0x%x buftype=%d buffer=0x%x Config=0x%x attrib_list=0x%x",
                    Dpy, buftype, buffer, Config, attrib_list);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Create shortcuts to objects. */
    dpy     = VEGL_DISPLAY(Dpy);
    config  = VEGL_CONFIG(Config);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    if (buftype != EGL_OPENVG_IMAGE)
    {
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Determine if the configuration is valid. */
    if (!(config->surfaceType & EGL_PBUFFER_BIT))
    {
        thread->error = EGL_BAD_MATCH;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    if (thread->context == EGL_NO_CONTEXT)
    {
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Parse attributes. */
    if (attrib_list != gcvNULL)
    {
        EGLint i;

        for (i = 0; attrib_list[i] != EGL_NONE; i += 2)
        {
            switch (attrib_list[i])
            {
            case EGL_TEXTURE_FORMAT:
                format = attrib_list[i + 1];
                break;

            case EGL_TEXTURE_TARGET:
                target = attrib_list[i + 1];
                break;

            case EGL_MIPMAP_TEXTURE:
                mipmap = attrib_list[i + 1];
                break;

            default:
                gcmTRACE_ZONE(
                    gcvLEVEL_ERROR, gcdZONE_EGL_SURFACE,
                    "%s(%d): Unknown attribute 0x%04X = 0x%04X.",
                    __FUNCTION__, __LINE__,
                    attrib_list[i], attrib_list[i + 1]
                    );

                thread->error = EGL_BAD_PARAMETER;
                gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
                return EGL_NO_SURFACE;
            }
        }
    }

    /* Create surface object. */
    surface = _InitializeSurface(thread, config, EGL_PBUFFER_BIT);

    if (surface == EGL_NO_SURFACE)
    {
        /* Bad allocation. */
        thread->error = EGL_BAD_ALLOC;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    surface->renderTarget = _GetClientBuffer(
        thread, thread->context->context, buffer
        );


    if (!surface->renderTarget)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface));

        /* Bad access. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Set PBuffer attributes. */
    gcmONERROR(gcoSURF_GetSize(surface->renderTarget,
                                 (gctUINT*)&surface->config.width,
                                 (gctUINT*)&surface->config.height,
                                 gcvNULL));

    surface->largestPBuffer = largest;
    surface->mipmapTexture  = mipmap;
    surface->textureFormat  = format;
    surface->textureTarget  = target;
    surface->buffer         = EGL_BACK_BUFFER;

#if gcdENABLE_VG
        /* Determine whether OpenVG pipe is present and OpenVG API is active. */
    surface->openVG = thread->openVGpipe && (thread->api == EGL_OPENVG_API);
    if (surface->depthFormat != gcvSURF_UNKNOWN && !surface->openVG)
#else
    /* Create depth buffer. */
    if (surface->depthFormat != gcvSURF_UNKNOWN)
#endif
    {
        gcmASSERT(surface->depthBuffer == gcvNULL);
        gcmONERROR(gcoSURF_Construct(gcvNULL,
                                     surface->config.width, surface->config.height, 1,
                                     gcvSURF_DEPTH,
                                     surface->depthFormat,
                                     gcvPOOL_DEFAULT,
                                     &surface->depthBuffer));

#ifndef VIVANTE_NO_3D
        /* Set multi-sampling size. */
        gcmONERROR(gcoSURF_SetSamples(surface->depthBuffer,
                                        surface->config.samples));
#endif
    }

    /* Push surface into stack. */
    surface->next = dpy->surfaceStack;
    dpy->surfaceStack = surface;

    veglReferenceSurface(thread, surface);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglCreatePbufferFromClientBuffer 0x%08X 0x%08X 0x%08X "
                "0x%08X (0x%08X) := 0x%08X (%dx%d)",
                Dpy, buftype, buffer, Config, attrib_list, surface,
                surface->config.width, surface->config.height);
    gcmDUMP_API_ARRAY_TOKEN(attrib_list, EGL_NONE);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("return=0x%x", surface);
    return surface;

OnError:
    if (surface->depthBuffer != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface->depthBuffer));
    }

	if (surface != gcvNULL)
	{
		gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface));
	}

    /* Return error. */
    thread->error = EGL_BAD_ALLOC;
    gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
    return EGL_NO_SURFACE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSurfaceAttrib(
    EGLDisplay Dpy,
    EGLSurface Surface,
    EGLint attribute,
    EGLint value
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface surface;

    gcmHEADER_ARG("Dpy=0x%x Surface=0x%x attribute=%d value=%d",
                    Dpy, Surface, attribute, value);
    gcmDUMP_API("${EGL eglSurfaceAttrib 0x%08X 0x%08X 0x%08X 0x%08X}",
                Dpy, Surface, attribute, value);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create shortcuts to objects. */
    dpy     = VEGL_DISPLAY(Dpy);
    surface = VEGL_SURFACE(Surface);

    /* Test for valid EGLDisplay structure */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for valid EGLSurface structure */
    if (surface == gcvNULL)
    {
        thread->error = EGL_BAD_SURFACE;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    switch (attribute)
    {
    case EGL_MIPMAP_LEVEL:
        surface->mipmapLevel = value;
        break;

    case EGL_SWAP_BEHAVIOR:
        surface->swapBehavior = value;
        break;

    default:
        /* Invalid attribute. */
        thread->error = EGL_BAD_ATTRIBUTE;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQuerySurface(
    EGLDisplay Dpy,
    EGLSurface Surface,
    EGLint attribute,
    EGLint *value
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface surface;
    VEGLSurface p_surface;

    gcmHEADER_ARG("Dpy=0x%x Surface=0x%x attribute=%d value=0x%x",
                    Dpy, Surface, attribute, value);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create shortcuts to objects. */
    dpy     = VEGL_DISPLAY(Dpy);
    surface = VEGL_SURFACE(Surface);

    /* Test for valid EGLDisplay structure */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for valid EGLSurface structure */
    for (p_surface = dpy->surfaceStack;
         p_surface != gcvNULL;
         p_surface = p_surface->next)
    {
        if (p_surface == surface)
            break;
    }
    if (surface == gcvNULL ||
        p_surface == gcvNULL)
    {
        thread->error = EGL_BAD_SURFACE;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    switch (attribute)
    {
    case EGL_CONFIG_ID:
        *value = surface->config.configId;
        break;

    case EGL_WIDTH:
        *value = surface->config.width;
        break;

    case EGL_HEIGHT:
        *value = surface->config.height;
        break;

    case EGL_MIPMAP_TEXTURE:
        *value = surface->mipmapTexture;
        break;

    case EGL_MIPMAP_LEVEL:
        *value = surface->mipmapLevel;
        break;

    case EGL_TEXTURE_FORMAT:
        *value = surface->textureFormat;
        break;

    case EGL_TEXTURE_TARGET:
        *value = surface->textureTarget;
        break;

    case EGL_LARGEST_PBUFFER:
        *value = surface->largestPBuffer;
        break;

    case EGL_HORIZONTAL_RESOLUTION:
    case EGL_VERTICAL_RESOLUTION:
    case EGL_PIXEL_ASPECT_RATIO:
        *value = EGL_UNKNOWN;
        break;

    case EGL_RENDER_BUFFER:
        *value = surface->buffer;
        break;

    case EGL_SWAP_BEHAVIOR:
        *value = surface->swapBehavior;
        break;

    case EGL_VG_ALPHA_FORMAT:
    case EGL_VG_COLORSPACE:
        /* Not yet implemented. */
        gcmBREAK();
        break;

    case EGL_BITMAP_POINTER_KHR:
        if (surface->locked)
        {
            if (!surface->lockBuffer)
            {
                if (!_MapLockedBuffer(thread, surface))
                {
                    break;
                }
            }

            *value = gcmPTR2INT(surface->lockBits);
        }
        else
        {
            thread->error = EGL_BAD_ACCESS;
        }
        break;

    case EGL_BITMAP_PITCH_KHR:
        if (surface->locked)
        {
            if (!surface->lockBuffer)
            {
                if (!_MapLockedBuffer(thread, surface))
                {
                    break;
                }
            }

            *value = surface->lockBufferStride;
        }
        else
        {
            thread->error = EGL_BAD_ACCESS;
        }

        break;

    case EGL_BITMAP_ORIGIN_KHR:
        *value = EGL_UPPER_LEFT_KHR;
        break;

    case EGL_BITMAP_PIXEL_RED_OFFSET_KHR:
    case EGL_BITMAP_PIXEL_GREEN_OFFSET_KHR:
    case EGL_BITMAP_PIXEL_BLUE_OFFSET_KHR:
    case EGL_BITMAP_PIXEL_ALPHA_OFFSET_KHR:
    case EGL_BITMAP_PIXEL_LUMINANCE_OFFSET_KHR:
        *value = _GetLockedBufferPixelChannelOffset(Surface, attribute);
        break;

    default:
        /* Bad attribute. */
        thread->error = EGL_BAD_ATTRIBUTE;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Success. */
    gcmDUMP_API("${EGL eglQuerySurface 0x%08X 0x%08X 0x%08X := 0x%08X}",
                Dpy, Surface, attribute, *value);
    gcmFOOTER_ARG("*value=%d return=%d", *value, EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurface(
    EGLDisplay Dpy,
    EGLConfig Config,
    NativePixmapType pixmap,
    const EGLint * attrib_list
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLConfig config;
    VEGLSurface surface;
    EGLint error;

    gcmHEADER_ARG("Dpy=0x%x Config=0x%x pixmap=0x%x attrib_list=0x%x",
                    Dpy, Config, pixmap, attrib_list);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Create shortcuts to objects. */
    dpy    = VEGL_DISPLAY(Dpy);
    config = VEGL_CONFIG(Config);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Determine if the configuration is valid. */
    if (!(config->surfaceType & EGL_PIXMAP_BIT))
    {
        thread->error = EGL_BAD_MATCH;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Parse attributes. */
    if (attrib_list != gcvNULL)
    {
        if (attrib_list[0] != EGL_NONE)
        {
            gcmTRACE_ZONE(
                gcvLEVEL_ERROR, gcdZONE_EGL_SURFACE,
                "%s(%d): ERROR: Unknown attribute 0x%04X = 0x%04X.",
                __FUNCTION__, __LINE__,
                attrib_list[0], attrib_list[1]
                );

            thread->error = EGL_BAD_PARAMETER;
            gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
            return EGL_NO_SURFACE;
        }
    }

    /* Create surface object. */
    surface = _InitializeSurface(thread, config, EGL_PIXMAP_BIT);

    if (surface == EGL_NO_SURFACE)
    {
        /* Bad allocation. */
        thread->error = EGL_BAD_ALLOC;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Set Pixmap attributes. */
    surface->pixmap = pixmap;
    surface->buffer = EGL_SINGLE_BUFFER;

    /* Create the surface. */
    error = _CreateSurface(thread, dpy, surface);

    if (error != EGL_SUCCESS)
    {
        /* Roll back. */
        _DestroySurface(thread, surface);
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface));

        /* Return error. */
        thread->error = error;
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    /* Push surface into stack. */
    surface->next = dpy->surfaceStack;
    dpy->surfaceStack = surface;

    veglReferenceSurface(thread, surface);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglCreatePixmapSurface 0x%08X 0x%08X 0x%08X (0x%08X) := "
                "0x%08X (%dx%d)",
                Dpy, Config, pixmap, attrib_list, surface,
                surface->config.width, surface->config.height);
    gcmDUMP_API_ARRAY_TOKEN(attrib_list, EGL_NONE);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("return=0x%x", surface);
    return surface;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindTexImage(
    EGLDisplay Display,
    EGLSurface Surface,
    EGLint Buffer
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface surface;

    gcmHEADER_ARG("Display=0x%x Surface=0x%x Buffer=%d",
                  Display, Surface, Buffer);
    gcmDUMP_API("${EGL eglBindTexImage 0x%08X 0x%08X 0x%08X}",
                Display, Surface, Buffer);

    /* Get current thread. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display. */
    dpy     = VEGL_DISPLAY(Display);
    surface = VEGL_SURFACE(Surface);

    /* Test for valid display. */
    if ((dpy == EGL_NO_DISPLAY)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for valid surface. */
    if ((surface == EGL_NO_SURFACE)
    ||  (surface->signature != EGL_SURFACE_SIGNATURE)
    ||  (surface->buffer != EGL_BACK_BUFFER)
    )
    {
        /* Bad surface. */
        thread->error = EGL_BAD_SURFACE;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if surface locked. */
    if (surface->locked)
    {
        /* Bad access. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test surface arguments. */
    if (surface->textureFormat == EGL_NO_TEXTURE)
    {
        /* Bad match. */
        thread->error = EGL_BAD_MATCH;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for valid buffer. */
    if (Buffer != EGL_BACK_BUFFER)
    {
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if Display and Surface are attached to the current context. */
    if ((thread->context->display == Display)
    &&  (thread->context->draw == Surface)
    )
    {
        /* Call glFlush. */
        _Flush(thread);
    }
    else
    {
        /* TODO: Call glFinish on the context the Surface is bound to. */
    }

    /* Call the GL to bind the texture. */
    thread->error = _BindTexImage(thread,
                                  surface->renderTarget,
                                  surface->textureFormat,
                                  surface->textureTarget,
                                  surface->mipmapTexture,
                                  surface->mipmapLevel,
                                  &surface->texBinder);

    /* Return success or failure. */
    gcmFOOTER_ARG("return=%d", (thread->error == EGL_SUCCESS) ? EGL_TRUE : EGL_FALSE);
    return (thread->error == EGL_SUCCESS) ? EGL_TRUE : EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseTexImage(
    EGLDisplay Display,
    EGLSurface Surface,
    EGLint Buffer
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface surface;

    gcmHEADER_ARG("Display=0x%x Surface=0x%x Buffer=%d",
                  Display, Surface, Buffer);
    gcmDUMP_API("${EGL eglReleaseTexImage 0x%08X 0x%08X 0x%08X}",
                Display, Surface, Buffer);

    /* Get current thread. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display and the surface. */
    dpy     = VEGL_DISPLAY(Display);
    surface = VEGL_SURFACE(Surface);

    /* Test for valid display. */
    if ((dpy == EGL_NO_DISPLAY)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for valid surface. */
    if ((surface == EGL_NO_SURFACE)
    ||  (surface->signature != EGL_SURFACE_SIGNATURE)
    ||  (surface->buffer != EGL_BACK_BUFFER)
    )
    {
        /* Bad surface. */
        thread->error = EGL_BAD_SURFACE;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if surface is locked. */
    if (surface->locked)
    {
        /* Bad access. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test surface arguments. */
    if (surface->textureFormat == EGL_NO_TEXTURE)
    {
        /* Bad match. */
        thread->error = EGL_BAD_MATCH;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for valid buffer. */
    if (Buffer != EGL_BACK_BUFFER)
    {
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Call the GL to unbind the texture. */
    thread->error = _BindTexImage(thread,
                                  gcvNULL,
                                  surface->textureFormat,
                                  surface->textureTarget,
                                  EGL_FALSE,
                                  0,
                                  &surface->texBinder);

    /* Return success or failure. */
    gcmFOOTER_ARG("return=%d", (thread->error == EGL_SUCCESS) ? EGL_TRUE : EGL_FALSE);
    return (thread->error == EGL_SUCCESS) ? EGL_TRUE : EGL_FALSE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglLockSurfaceKHR(
    EGLDisplay Display,
    EGLSurface Surface,
    const EGLint *Attrib_list
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface surface;
    VEGLContext ctx;

    EGLBoolean  preserve = EGL_FALSE;

    gcmHEADER_ARG("Display=0x%x Surface=0x%x Attrib_list=0x%x",
                    Display, Surface, Attrib_list);
    gcmDUMP_API("${EGL eglLockSurfaceKHR 0x%08X 0x%08X (0x%08X)",
                Display, Surface, Attrib_list);
    gcmDUMP_API_ARRAY_TOKEN(Attrib_list, EGL_NONE);
    gcmDUMP_API("$}");

    /* Get current thread. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display and the surface. */
    dpy     = VEGL_DISPLAY(Display);
    surface = VEGL_SURFACE(Surface);

    /* Test for valid display. */
    if ((dpy == EGL_NO_DISPLAY)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Todo : check EGL_LOCK_SURFACE_BIT_KHR bit in config. */
    if (surface->locked)
    {
        /* Not a lockable surface, or surface has been locked. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    for (ctx = dpy->contextStack; ctx != gcvNULL; ctx = ctx->next)
    {
        if (ctx->draw == surface || ctx->read == surface)
        {
            /* Surface is current of some context. */
            thread->error = EGL_BAD_ACCESS;
            gcmFOOTER_ARG("return=%d", EGL_FALSE);
            return EGL_FALSE;
        }
    }

    /* Parse the attribute list. */
    if (Attrib_list != gcvNULL)
    {
        while (*Attrib_list != EGL_NONE)
        {
            EGLint attribute = *Attrib_list++;
            EGLint value     = *Attrib_list++;

            switch(attribute)
            {
            case EGL_MAP_PRESERVE_PIXELS_KHR:
                preserve = value;
                break;

            case EGL_LOCK_USAGE_HINT_KHR:
                break;

            default:
                /* Ignore unacceptable attributes. */
                gcmTRACE_ZONE(
                    gcvLEVEL_ERROR, gcdZONE_EGL_SURFACE,
                    "%s(%d): Unknown attribute 0x%04X = 0x%04X.",
                    __FUNCTION__, __LINE__,
                    attribute, value
                    );

                thread->error = EGL_BAD_ATTRIBUTE;
                gcmFOOTER_ARG("return=%d", EGL_FALSE);
                return EGL_FALSE;
            }
        }
    }


    /* Set surface as locked. */
    surface->locked           = EGL_TRUE;
    surface->lockBufferFormat = surface->renderTargetFormat;
    surface->lockBuffer       = gcvNULL;
    surface->lockBits         = gcvNULL;
    surface->lockPreserve     = preserve;

    thread->error = EGL_SUCCESS;

    /* Return success or failure. */
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglUnlockSurfaceKHR(
    EGLDisplay Display,
    EGLSurface Surface
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface surface;
    gceSTATUS   status;

    gcmHEADER_ARG("Display=0x%x Surface=0x%x", Display, Surface);
    gcmDUMP_API("${EGL eglUnlockSurfaceKHR 0x%08X 0x%08X}", Display, Surface);

    /* Get current thread. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create a shortcut to the display and the surface. */
    dpy     = VEGL_DISPLAY(Display);
    surface = VEGL_SURFACE(Surface);

    /* Test for valid display. */
    if ((dpy == EGL_NO_DISPLAY)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if (!surface->locked)
    {
        /* Surface has been unlocked. */
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* If surface is mapped. */
    if (surface->lockBuffer)
    {
        /* Reflect mapped buffer to color buffer. */
        status = gcoSURF_Resolve(surface->lockBuffer, surface->renderTarget);

        if(gcmIS_ERROR(status))
        {
            thread->error = EGL_BAD_ACCESS;
            gcmFOOTER_ARG("return=%d", EGL_FALSE);
            return EGL_FALSE;
        }

        status = gcoHAL_Commit(gcvNULL, gcvTRUE);
        if (gcmIS_ERROR(status))
        {
			thread->error = EGL_BAD_ACCESS;
			gcmFOOTER_ARG("return=%d", EGL_FALSE);
            return EGL_FALSE;
		}

        /* Unlock bitmap. */
        gcoSURF_Unlock(surface->lockBuffer, surface->lockBits);

        /* Destroy bitmap. */
        gcoSURF_Destroy(surface->lockBuffer);
    }

    surface->locked = EGL_FALSE;

    /* Should clean locked buffer? */
    surface->lockBufferFormat = gcvSURF_UNKNOWN;
    surface->lockBuffer       = gcvNULL;
    surface->lockBits         = gcvNULL;

    thread->error = EGL_SUCCESS;

    /* Return success or failure. */
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}
