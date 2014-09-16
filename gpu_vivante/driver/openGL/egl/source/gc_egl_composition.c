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
#if defined(ANDROID)
#include <gc_gralloc_priv.h>
#endif

#define NO_STENCIL_VG   1
#define _GC_OBJ_ZONE    glvZONE_EGL_COMPOSE


/*******************************************************************************
**
** EGL composition context.
**
** TODO: This is quick implementation.
**       Only record android native buffers currently.
*/

struct eglComposition
{
   /* AndroidNativeBuffer count of current composition. */
   gctUINT count;

   struct NativeBuffer
   {
       /* Imported render target. */
       gcoSURF   surface;
       /* Is 3D application? */
       gctBOOL   gpuApp;
       /* Signal: resolveSubmittedSignal or hwDoneSignal. */
       gctSIGNAL signal;
       /* Client process id. */
       gctHANDLE clientPID;
   }
   buffers[16];
};


/******************************************************************************\
*************************** Composition Extension API **************************
\******************************************************************************/

/*******************************************************************************
**
**  eglCompositionBeginVIV (EGL_VIV_composition)
**
**  Starts the process of compositing multiple surfaces to the current target.
**  The expected calling sequence for the composition API is as follows:
**
**      eglCompositionBeginVIV();            start composition
**          eglComposeLayerVIV(...);         pass the 1st layer to the engine
**          eglComposeLayerVIV(...);         pass the 2nd layer to the engine
**          . . . . .
**      eglCompositionEndVIV();              finish composition
**
**  ERRORS:
**
**      GL_INVALID_OPERATION
**          - if called while already in composition mode.
**
**  INPUT:
**
**      None.
**
**  OUTPUT:
**
**      None.
*/

EGLAPI EGLBoolean EGLAPIENTRY eglCompositionBeginVIV(
    void
    )
{
    gceSTATUS status;
    VEGLThreadData thread;

    gcmHEADER();
    gcmDUMP_API("${EGL eglCompositionBeginVIV}");

    /* Get thread data. */
    thread = veglGetThreadData();

    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        goto OnError;
    }

    /* Initiate the composition. */
    status = gco3D_CompositionBegin();

    /* Error? */
    if (gcmIS_ERROR(status))
    {
        thread->error = EGL_BAD_ACCESS;
        goto OnError;
    }

    /* Initiate states. */
    if (thread->composition == gcvNULL)
    {
        gctPOINTER pointer = gcvNULL;

        gcmONERROR(gcoOS_Allocate(
            gcvNULL, gcmSIZEOF(struct eglComposition), &pointer
            ));

        gcmONERROR(gcoOS_ZeroMemory(
            pointer, gcmSIZEOF(struct eglComposition)
            ));

        /* Cast the pointer. */
        thread->composition = (VEGLComposition) pointer;
    }

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;

OnError:
    /* Failed. */
    gcmFOOTER_ARG("return=%d", EGL_FALSE);
    return EGL_FALSE;
}


/*******************************************************************************
**
**  eglComposeLayerVIV (EGL_VIV_composition)
**
**  Specifies the surface to be composed.
**
**  ERRORS:
**
**      GL_INVALID_OPERATION
**          - if called while not in composition mode;
**          - if Layer does not point to a valid supported surface.
**
**      GL_INVALID_ENUM
**          - if SrcBlendFunction is not one of supported values;
**          - if TextureFunction is not one of supported values.
**
**  INPUT:
**
**      Layer
**          Pointer to the later composition structure. Structure members are
**          defined as follows
**
**          GLuint size
**              Size of the structure in bytes;
**
**          GLeglImageOES layer
**              EGL image handle to the layer to be composed. The following
**              special values are allowed:
**                  - GL_VIV_BLUR_LAYER (to blur the specified destination)
**                  - GL_VIV_DIM_LAYER  (to dim the specified destination)
**
**          GLboolean blendLayer
**              Whether or not alpha blending is enabled for the layer;
**
**          GLenum srcBlendFunction
**          GLenum trgBlendFunction
**              Source and target alpha blending functions;
**
**          GLubyte alphaValue
**              Modulation alpha value for the layer. To disable alpha
**              modulation, pass 0xFF value.
**
**          GLint angle
**              Rotation angle;
**
**          GLfloat scaleX
**          GLfloat scaleY
**              Scale factors;
**
**          GLint srcX
**          GLint srcY
**              The origin within the layer to be composed from;
**
**          GLint trgX
**          GLint trgY
**              The origin within the current target to be composed to;
**
**          GLint width
**          GLint height
**              The size of the target rectangle to be composed.
**
**  OUTPUT:
**
**      None.
*/

EGLAPI EGLBoolean EGLAPIENTRY eglComposeLayerVIV(
    struct EGLCompositionLayer * Layer
    )
{
    gceSTATUS status;
    VEGLThreadData thread;
    gcsCOMPOSITION layer;

    gcmHEADER_ARG("Layer=0x%x", Layer);
    gcmDUMP_API("${EGL eglComposeLayerVIV 0x%08X}", Layer);

    /* Get thread data. */
    thread = veglGetThreadData();

    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        goto OnError;
    }

    /* Set the size of the structure. */
    layer.structSize = gcmSIZEOF(layer);

    /* Clear? */
    if (Layer->layer == EGL_VIV_CLEAR_LAYER)
    {
        /* Clear. */
        layer.operation = gcvCOMPOSE_CLEAR;

        /* Set clear color. */
        layer.r = (gctFLOAT) Layer->red   / 255.0f;
        layer.g = (gctFLOAT) Layer->green / 255.0f;
        layer.b = (gctFLOAT) Layer->blue  / 255.0f;
        layer.a = (gctFLOAT) Layer->alpha / 255.0f;
    }

    /* Blur? */
    else if (Layer->layer == EGL_VIV_BLUR_LAYER)
    {
        /* Blur. */
        layer.operation = gcvCOMPOSE_BLUR;
    }

    /* Dim? */
    else if (Layer->layer == EGL_VIV_DIM_LAYER)
    {
        /* Dim. */
        layer.operation = gcvCOMPOSE_DIM;
    }

    /* Regular layer object. */
    else
    {
        /* Cast. */
        VEGLImage image = VEGL_IMAGE(Layer->layer);

        /* Test if layer is valid. */
        if ((image == gcvNULL) || (image->signature != EGL_IMAGE_SIGNATURE))
        {
            thread->error = EGL_BAD_PARAMETER;
            goto OnError;
        }

        /* Layer. */
        layer.operation = gcvCOMPOSE_LAYER;

        /* Set the surface. */
        switch (image->image.type)
        {
        case KHR_IMAGE_ANDROID_NATIVE_BUFFER:
#if defined(ANDROID)
            {
                /* Make shortcuts. */
                VEGLComposition comp = thread->composition;
                gctINT index         = comp->count;

                struct gc_private_handle_t * handle =
                    (struct gc_private_handle_t *) image->image.u.androidNativeBuffer.privHandle;

                /* Compose the CPU surface. */
                layer.layer      = image->image.surface;

                if (handle->hwDoneSignal)
                {
                    /* Set hwDoneSignal to un-sigaled state. */
                    gcmONERROR(gcoOS_Signal(
                         gcvNULL,
                         (gctSIGNAL) handle->hwDoneSignal,
                         gcvFALSE
                         ));

                    /* Set no gpu application. */
                    comp->buffers[index].gpuApp    = gcvFALSE;

                    /* Get signal. */
                    comp->buffers[index].signal    = (gctSIGNAL) handle->hwDoneSignal;

                    /* Get process id. */
                    comp->buffers[index].clientPID = (gctHANDLE) handle->clientPID;

                    /* Increase AndroidNativeBuffer count. */
                    comp->count ++;
                }

                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcvZONE_COMPOSITION,
                    "%s(%d): Compose cpu surface=%p",
                    __FUNCTION__, __LINE__, layer.layer
                    );
            }
            break;
#endif
        default:
            layer.layer      = image->image.surface;
            break;
        }

        /* Set source coordinates. */
        layer.srcRect.left   = (gctINT)  Layer->srcX;
        layer.srcRect.top    = (gctINT)  Layer->srcY;
        layer.srcRect.right  = (gctINT) (Layer->srcX + Layer->srcWidth);
        layer.srcRect.bottom = (gctINT) (Layer->srcY + Layer->srcHeight);

        /* Set target rectangle. */
        layer.v0.x           = Layer->x0;
        layer.v0.y           = Layer->y0;

        layer.v1.x           = Layer->x1;
        layer.v1.y           = Layer->y1;

        layer.v2.x           = Layer->x2;
        layer.v2.y           = Layer->y2;
    }

    /* Set blending. */
    layer.enableBlending = (gctBOOL)  Layer->enableBlending;
    layer.premultiplied  = (gctBOOL)  Layer->premultiplied;
    layer.alphaValue     = (gctUINT8) Layer->alphaValue;

    /* Set target coordinates. */
    layer.trgRect.left   = (gctINT)  Layer->trgX;
    layer.trgRect.top    = (gctINT)  Layer->trgY;
    layer.trgRect.right  = (gctINT) (Layer->trgX + Layer->trgWidth);
    layer.trgRect.bottom = (gctINT) (Layer->trgY + Layer->trgHeight);

    /* Add the layer to the composition engine. */
    status = gco3D_ComposeLayer(&layer);

    if (gcmIS_ERROR(status))
    {
        thread->error = EGL_BAD_PARAMETER;
        goto OnError;
    }

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;

OnError:
    /* Failed. */
    gcmFOOTER_ARG("return=%d", EGL_FALSE);
    return EGL_FALSE;
}


/*******************************************************************************
**
**  eglCompositionEndVIV (EGL_VIV_composition)
**
**  Ends the process of compositing multiple surfaces to the current target.
**
**  ERRORS:
**
**      GL_INVALID_OPERATION
**          - if called while not in composition mode.
**
**  INPUT:
**
**      Display
**          EGL display handle.
**
**      Surface
**          EGL surface to which the composition is to be done.
**
**  OUTPUT:
**
**      None.
*/

EGLAPI EGLBoolean EGLAPIENTRY eglCompositionEndVIV(
    EGLDisplay Display,
    EGLSurface Surface,
    EGLint Synchronous
    )
{
    EGLBoolean result;
    gceSTATUS status;
    VEGLThreadData thread;
    VEGLDisplay display = gcvNULL;
    VEGLSurface surface = gcvNULL;
    VEGLSurface stack;
    struct eglBackBuffer backBuffer;
    gctBOOL ownBackBuffer = gcvFALSE;
    gcoSURF target;
    gctBOOL suspended = gcvFALSE;
    VEGLWorkerInfo worker = gcvNULL;

    gcmHEADER_ARG("Display=0x%x Surface=0x%x", Display, Surface);
    gcmDUMP_API("${EGL eglCompositionEndVIV 0x%08X 0x%08X 0x%08X}",
        Display, Surface, Synchronous);

    /* Get thread data. */
    thread = veglGetThreadData();

    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        goto OnError;
    }

    /* Cast the objects. */
    display = VEGL_DISPLAY(Display);
    surface = VEGL_SURFACE(Surface);

    /* Test for valid EGLDisplay structure */
    if ((display == gcvNULL) || (display->signature != EGL_DISPLAY_SIGNATURE))
    {
        thread->error = EGL_BAD_DISPLAY;
        goto OnError;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (display->referenceDpy == gcvNULL)
    {
        /* Not initialized */
        thread->error = EGL_NOT_INITIALIZED;
        goto OnError;
    }

    /* Test for valid EGLSurface structure */
    for (stack = display->surfaceStack; stack != gcvNULL; stack = stack->next)
    {
        if (stack == surface)
        {
            break;
        }
    }

    if ((surface == gcvNULL) || (stack == gcvNULL))
    {
        thread->error = EGL_BAD_SURFACE;
        goto OnError;
    }

    /* Make sure we have something to composite. */
    status = gco3D_ProbeComposition(gcvTRUE);

    /* Error? */
    if (gcmIS_ERROR(status))
    {
        thread->error = EGL_BAD_ACCESS;
        goto OnError;
    }

    /* Nothing to compose? */
    if (!gcmIS_SUCCESS(status))
    {
        thread->error = EGL_SUCCESS;
        goto OnError;
    }

    /* Init the context to make compiler happy. */
    backBuffer.context = gcvNULL;

    if ((surface->type & EGL_WINDOW_BIT) != 0)
    {
        /* Request back buffer for window surface. */
        gctBOOL success = veglGetDisplayBackBuffer(
            display, surface->hwnd, &backBuffer, &ownBackBuffer
            );

        if (!success)
        {
            ownBackBuffer = gcvFALSE;
            goto OnError;
        }

        /* Determine the target surface. */
        if (ownBackBuffer && (backBuffer.surface != gcvNULL))
        {
            target = backBuffer.surface;
        }
        else if (surface->fbDirect)
        {
            target = surface->fbWrapper;
        }
        else
        {
            target = surface->resolve;
        }
    }
    else
    {
        /* No backBuffer for Pbuffer/Bitmap surface. */
        ownBackBuffer = gcvFALSE;

        /* Target is render target. */
        target = surface->renderTarget;
    }

    if (target == gcvNULL)
    {
        thread->error = EGL_BAD_SURFACE;
        goto OnError;
    }

    /* Schedule a swap worker. */
    if (ownBackBuffer || !surface->fbDirect)
    {
        /* Suspend the worker thread. */
        veglSuspendSwapWorker(display);
        suspended = gcvTRUE;

        /* Find an available worker. */
        worker = veglGetWorker(thread, display, surface);

        if (worker == gcvNULL)
        {
            thread->error = EGL_BAD_ALLOC;
            goto OnError;
        }

        /* Set worker signals. */
        status = gco3D_CompositionSignals(
            display->process, worker->signal, display->startSignal
            );

        /* Error? */
        if (gcmIS_ERROR(status))
        {
            thread->error = EGL_BAD_ACCESS;
            goto OnError;
        }

        /* Start composition. */
        status = gco3D_CompositionEnd(target, Synchronous);

        /* Error? */
        if (gcmIS_ERROR(status))
        {
            thread->error = EGL_BAD_ACCESS;
            goto OnError;
        }

        if (ownBackBuffer)
        {
            worker->draw         = surface;
            worker->backBuffer   = backBuffer;
            worker->targetSignal = gcvNULL;
        }
        else
        {
            worker->draw                   = surface;
            worker->backBuffer.origin.x    = 0;
            worker->backBuffer.origin.y    = 0;
            worker->backBuffer.size.width  = surface->bitsWidth;
            worker->backBuffer.size.height = surface->bitsHeight;
            worker->next                   = gcvNULL;

            /* Multi-buffered target enabled? */
            if (surface->renderListEnable)
            {
                worker->targetSignal = surface->renderListCurr->signal;
                worker->bits         = surface->renderListCurr->bits;
            }
            else
            {
                /* Set surface attributes. */
                worker->targetSignal = gcvNULL;
                worker->bits         = surface->resolveBits;
            }
        }

        /* Submit worker. */
        veglSubmitWorker(thread, display, worker, gcvFALSE);

        /* Resume the thread. */
        veglResumeSwapWorker(display);
        suspended = gcvFALSE;

        /* A flip has been scheduled, reset the flag. */
        ownBackBuffer = gcvFALSE;
    }
    else
    {
        /* Start composition. */
        status = gco3D_CompositionEnd(target, Synchronous);

        /* Error? */
        if (gcmIS_ERROR(status))
        {
            thread->error = EGL_BAD_ACCESS;
            goto OnError;
        }

        /* Nothing to compose? */
        if (status == gcvSTATUS_NO_MORE_DATA)
        {
            thread->error = EGL_SUCCESS;
            goto OnError;
        }
    }

    /* Set the target. */
    if (gcmIS_ERROR(veglDriverSurfaceSwap(thread, surface)))
    {
        thread->error = EGL_BAD_SURFACE;
        goto OnError;
    }

    /* Success. */
    thread->error = EGL_SUCCESS;

OnError:

#if defined(ANDROID)
    /* Try to send signals. */
    if (thread->composition->count > 0)
    {
        gctUINT  i;
        gctBOOL needCommit = gcvFALSE;

        /* Make shortcuts. */
        VEGLComposition comp = thread->composition;

        for (i = 0; i < comp->count; i++)
        {
            if (!comp->buffers[i].gpuApp)
            {
                /* Schedule a signal for cpu app. */
                gcsHAL_INTERFACE iface;

                iface.command            = gcvHAL_SIGNAL;
                iface.u.Signal.signal    = comp->buffers[i].signal;
                iface.u.Signal.auxSignal = gcvNULL;
                iface.u.Signal.process   = comp->buffers[i].clientPID;
                iface.u.Signal.fromWhere = gcvKERNEL_PIXEL;

                /* Schedule the event. */
                gcoHAL_ScheduleEvent(
                    gcvNULL,
                    &iface
                    );

                needCommit = gcvTRUE;
            }
        }

        if (needCommit)
        {
            /* Commit events. */
            gcoHAL_Commit(gcvNULL, gcvFALSE);
        }

        /* Reset AndroidNativeBuffer count. */
        comp->count = 0;
    }
#endif

    if (suspended)
    {
        if (worker != gcvNULL)
        {
            gcmVERIFY_OK(veglReleaseWorker(worker));
        }

        veglResumeSwapWorker(display);
    }

    if (ownBackBuffer)
    {
        gcmVERIFY(veglSetDisplayFlip(
            display->hdc, surface->hwnd, &backBuffer
            ));
    }

	if (thread != gcvNULL)
	{
		result = (thread->error == EGL_SUCCESS)
			? EGL_TRUE
			: EGL_FALSE;
	}
	else
	{
		result = EGL_FALSE;
	}

    gcmFOOTER_ARG("return=%d", result);
    return result;
}
