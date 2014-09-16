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

#define _GC_OBJ_ZONE            gcdZONE_EGL_API

/*******************************************************************************
***** Version Signature *******************************************************/

#define _gcmTXT2STR(t) #t
#define  gcmTXT2STR(t) _gcmTXT2STR(t)

const char * _EGL_VERSION = "\n\0$VERSION$"
                            gcmTXT2STR(gcvVERSION_MAJOR) "."
                            gcmTXT2STR(gcvVERSION_MINOR) "."
                            gcmTXT2STR(gcvVERSION_PATCH) ":"
                            gcmTXT2STR(gcvVERSION_BUILD) "$\n";

static void
_DestroyThreadData(
    gcsTLS_PTR TLS
    )
{
    gcmHEADER_ARG("TLS=0x%x", TLS);

    if (TLS->context != gcvNULL)
    {
        /*VEGLThreadData thread;*/
        VEGLDisplay head;

        /*thread = (VEGLThreadData) TLS->context;*/

        head = (VEGLDisplay) gcoOS_GetPLSValue(gcePLS_VALUE_EGL_DISPLAY_INFO);

        while (head != EGL_NO_DISPLAY)
        {
            VEGLDisplay display = head;

            if (display->ownerThread == gcoOS_GetCurrentThreadID())
            {
                /* This thread has not eglTerminated the display.*/
                eglTerminate(display);
            }

            head = display->next;
            if (TLS->ProcessExiting)
            {
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, display));
                gcoOS_SetPLSValue(gcePLS_VALUE_EGL_DISPLAY_INFO, (gctPOINTER) head);
            }
        }

        TLS->context = gcvNULL;
    }

    gcmFOOTER_NO();
}

#if gcdENABLE_VG
static EGLBoolean _IsHWVGDriverAvailable(VEGLThreadData thread)
{
    struct eglContext context;
    veglDISPATCH * dispatch;

    context.thread        = thread;
    context.api           = EGL_OPENVG_API;
    context.client        = 1;
    context.display       = EGL_NO_DISPLAY;
    context.sharedContext = EGL_NO_CONTEXT;
    context.draw          = EGL_NO_SURFACE;
    context.read          = EGL_NO_SURFACE;
    context.dispatch      = gcvNULL;

    dispatch = _GetDispatch(thread, &context);

    if ((dispatch == gcvNULL)
    ||  (dispatch->queryHWVG == gcvNULL)
    )
    {
        return EGL_FALSE;
    }

    return (*dispatch->queryHWVG)();
}
#endif

VEGLThreadData
veglGetThreadData(
    void
    )
{
    gcsTLS_PTR tls = gcvNULL;
    gceSTATUS status;
#if gcdENABLE_VG
    gctINT32 i;
#endif
    gcmHEADER();

    gcmONERROR(gcoOS_GetTLS(&tls));

    if (tls->context == gcvNULL)
    {
        gctPOINTER pointer = gcvNULL;
        VEGLThreadData thread;

        gcmONERROR(gcoOS_Allocate(
            gcvNULL, gcmSIZEOF(struct eglThreadData), &pointer
            ));

        gcmONERROR(gcoOS_ZeroMemory(
            pointer, gcmSIZEOF(struct eglThreadData)
            ));

        /* Initialize TLS record. */
        tls->context    = pointer;
        tls->destructor = _DestroyThreadData;

        /* Cast the pointer. */
        thread = (VEGLThreadData) pointer;

        /* Initialize the thread data. */
        thread->error             = EGL_SUCCESS;
        thread->api               = EGL_OPENGL_ES_API;
        thread->lastClient        = 1;
		thread->worker = gcvNULL;

        tls->currentType           = gcvHARDWARE_3D;
#if veglUSE_HAL_DUMP
        /* Get the gcoDUMP object. */
        gcmONERROR(gcoHAL_GetDump(thread->hal, &thread->dump));
#endif

#if gcdENABLE_VG
        gcmONERROR(gcoHAL_QueryChipCount(gcvNULL, &thread->chipCount));

        for (i = 0; i < thread->chipCount; i++)
        {
            gcmONERROR(gcoHAL_QueryChipLimits(gcvNULL, i, &thread->chipLimits[i]));
        }

        for (i = 0; i < thread->chipCount; i++)
        {
            thread->maxWidth =
                gcmMAX(thread->maxWidth, (gctINT32)thread->chipLimits[i].maxWidth);

            thread->maxHeight =
                gcmMAX(thread->maxHeight, (gctINT32)thread->chipLimits[i].maxHeight);

            thread->maxSamples =
                gcmMAX(thread->maxSamples, (gctINT32)thread->chipLimits[i].maxSamples);
        }

        for (i = 0; i < thread->chipCount; i++)
        {
            /* Query VAA support. */
            if (gcoHAL_QueryChipFeature(gcvNULL, i, gcvFEATURE_VAA))
            {
                thread->vaa = gcvTRUE;
                break;
            }
        }

        for (i = 0; i < thread->chipCount; i++)
        {
            if (gcoHAL_QueryChipFeature(gcvNULL, i, gcvFEATURE_PIPE_VG))
            {
                thread->openVGpipe = _IsHWVGDriverAvailable(thread);
                break;
            }
        }
#else
        /* Query the hardware identity. */
        gcmONERROR(gcoHAL_QueryChipIdentity(
            gcvNULL,
            &thread->chipModel,
            gcvNULL, gcvNULL, gcvNULL
            ));

        /* Query the hardware capabilities. */
        gcmONERROR(gcoHAL_QueryTargetCaps(
            gcvNULL,
            (gctUINT32_PTR) &thread->maxWidth,
            (gctUINT32_PTR) &thread->maxHeight,
            gcvNULL,
            (gctUINT32_PTR) &thread->maxSamples
            ));

        /* Query VAA support. */
        thread->vaa
            =  gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_VAA)
            == gcvSTATUS_TRUE;

        /* Query OpenVG pipe support. */
        thread->openVGpipe
            =  gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_PIPE_VG)
            == gcvSTATUS_TRUE;
#endif

        gcmTRACE_ZONE(
            gcvLEVEL_VERBOSE, gcdZONE_EGL_API,
            "%s(%d): maxWidth=%d maxHeight=%d maxSamples=%d vaa=%d openVG=%d",
            __FUNCTION__, __LINE__,
            thread->maxWidth, thread->maxHeight, thread->maxSamples,
            thread->vaa, thread->openVGpipe
            );
    }

    /* Return pointer to thread data. */
    gcmFOOTER_ARG("0x%x", tls->context);
    return (VEGLThreadData) tls->context;

OnError:
    /* Roll back. */
    if (tls != gcvNULL)
    {
        _DestroyThreadData(tls);
    }

    /* Return error. */
    gcmFOOTER_ARG("0x%x", gcvNULL);
    return gcvNULL;
}
