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
#define TAG "VIVANTE"
#define LIB_PATH "/system/lib/egl/"
#endif

#define _GC_OBJ_ZONE    gcdZONE_EGL_CONTEXT

#if !gcdSTATIC_LINK
typedef enum _veglAPIINDEX
{
    vegl_EGL,
    vegl_OPENGL_ES11_CL,
    vegl_OPENGL_ES11,
    vegl_OPENGL_ES20,
    vegl_OPENVG,
}
veglAPIINDEX;

#if defined(ANDROID)
/* Try wrapper library first, and then our library. */
static const char * _dlls[][2] =
{
    /* EGL */                       {LIB_PATH "libEGL_" TAG ".so", gcvNULL},
    /* OpenGL ES 1.1 Common Lite */ {"libGLESv1_CL.so",            gcvNULL},
    /* OpenGL ES 1.1 Common */      {"libGLESv1_CM.so",            LIB_PATH "libGLESv1_CM_" TAG ".so"},
    /* OpenGL ES 2.0 */             {"libGLESv2.so",               LIB_PATH "libGLESv2_" TAG ".so"},
    /* OpenVG 1.0 */                {"libOpenVG",                  gcvNULL},
};
#elif defined(__QNXNTO__)
static const char * _dlls[][2] =
{
    /* EGL */                       {"libEGL_viv",   gcvNULL},
    /* OpenGL ES 1.1 Common Lite */ {"glesv1-dlls",  gcvNULL},
    /* OpenGL ES 1.1 Common */      {"glesv1-dlls",  gcvNULL},
    /* OpenGL ES 2.0 */             {"glesv2-dlls",  gcvNULL},
    /* OpenVG 1.0 */                {"vg-dlls",      gcvNULL},
};
#elif defined(__APPLE__)
static const char * _dlls[][2] =
{
    /* EGL */                       {"libEGL.dylib",       gcvNULL},
    /* OpenGL ES 1.1 Common Lite */ {"libGLESv1_CL.dylib", gcvNULL},
    /* OpenGL ES 1.1 Common */      {"libGLESv1_CM.dylib", gcvNULL},
    /* OpenGL ES 2.0 */             {"libGLESv2.dylib",    gcvNULL},
    /* OpenVG 1.0 */                {"libOpenVG.dylib",    gcvNULL},
};
#else
static const char * _dlls[][2] =
{
    /* EGL */                       {"libEGL",         gcvNULL},
    /* OpenGL ES 1.1 Common Lite */ {"libGLESv1_CL",   gcvNULL},
    /* OpenGL ES 1.1 Common */      {"libGLESv1_CM",   gcvNULL},
    /* OpenGL ES 2.0 */             {"libGLESv2",      gcvNULL},
    /* OpenVG 1.0 */                {"libOpenVG",      gcvNULL},
};
#endif

static const char * _dispatchNames[] =
{
    /* EGL                       */ "",
    /* OpenGL ES 1.1 Common Lite */ "GLES_CL_DISPATCH_TABLE",
    /* OpenGL ES 1.1 Common      */ "GLES_CM_DISPATCH_TABLE",
    /* OpenGL ES 2.0             */ "GLESv2_DISPATCH_TABLE",
    /* OpenVG                    */ "OpenVG_DISPATCH_TABLE",
};

static int
_GetAPIIndex(
    EGLBoolean Egl,
    VEGLContext Context
    )
{
    int index = -1;

    do
    {
        VEGLThreadData thread;
        VEGLContext context;
        EGLenum api;
        EGLint client;

        if (Egl)
        {
            index = vegl_EGL;
            break;
        }

        /* Get thread data. */
        thread = veglGetThreadData();
        if (thread == gcvNULL)
        {
            gcmTRACE(
                gcvLEVEL_ERROR,
                "%s(%d): veglGetThreadData failed.",
                __FUNCTION__, __LINE__
                );

            break;
        }

        /* Get current context. */
        context = (Context != gcvNULL) ? Context
                : (thread->api == EGL_NONE)
                ? gcvNULL
                : thread->context;

        /* Is there a current context? */
        if (context == gcvNULL)
        {
            /* No current context, use thread data. */
            api    = thread->api;
            client = thread->lastClient;
        }
        else
        {
            /* Have current context set. */
            api    = context->api;
            client = context->client;
        }

        /* Dispatch base on the API. */
        switch (api)
        {
        case EGL_OPENGL_ES_API:
            index = (client == 2) ? vegl_OPENGL_ES20 : vegl_OPENGL_ES11;
            break;

        case EGL_OPENVG_API:
            index = vegl_OPENVG;
            break;
        }
    }
    while (EGL_FALSE);

    /* Return result. */
    return index;
}

gctHANDLE
veglGetModule(
    IN gcoOS Os,
    IN EGLBoolean Egl,
    IN VEGLContext Context,
    IN gctINT_PTR Index
    )
{
    gctHANDLE library = gcvNULL;
    gctINT index;
    gctUINT libIndex = 0;

    /* Get API index. */
    index = _GetAPIIndex(Egl, Context);

    if (index != -1)
    {
        if (Index != gcvNULL)
        {
            /* Try the next lib, if being repetitively called. */
            if(*Index == index)
            {
                libIndex = 1;
            }
        }

        /* Query the handle. */
        if (_dlls[index][libIndex] != gcvNULL)
        {
            gcoOS_LoadLibrary(Os, _dlls[index][libIndex], &library);

            if ((library == gcvNULL) && (index == vegl_OPENGL_ES11))
            {
                --index;

                /* Query the CL handle if CM not available. */
                gcoOS_LoadLibrary(Os, _dlls[index][libIndex], &library);
            }
        }
    }

    if (Index != gcvNULL)
    {
        *Index = index;
    }

    /* Return result. */
    return library;
}
#endif /* gcdSTATIC_LINK */

veglDISPATCH *
_GetDispatch(
    VEGLThreadData Thread,
    VEGLContext Context
    )
{
    struct eglContext context;

    if (Thread == gcvNULL)
    {
        return gcvNULL;
    }

    if (Context == gcvNULL)
    {
        Context = Thread->context;

        if (Context == gcvNULL)
        {
            /* Use default context. */
            context.thread        = Thread;
            context.api           = Thread->api;
            context.client        = 1;
            context.display       = EGL_NO_DISPLAY;
            context.sharedContext = EGL_NO_CONTEXT;
            context.draw          = EGL_NO_SURFACE;
            context.read          = EGL_NO_SURFACE;
            context.dispatch      = gcvNULL;

            Context = &context;
        }
    }

    if (Context->dispatch == gcvNULL)
    {
#if gcdSTATIC_LINK
#ifndef VIVANTE_NO_3D /*Added for openVG core static link*/
        extern veglDISPATCH GLES_CM_DISPATCH_TABLE;
        extern veglDISPATCH GLESv2_DISPATCH_TABLE;
#endif /* VIVANTE_NO_3D */
#ifndef VIVANTE_NO_VG
        extern veglDISPATCH OpenVG_DISPATCH_TABLE;
#endif /* VIVANTE_NO_VG */

        switch (Context->api)
        {
        default:
#ifndef VIVANTE_NO_3D /*Added for openVG core static link*/
            if (Context->client != 2)
            {
                Context->dispatch = &GLES_CM_DISPATCH_TABLE;
            }
            else
            {
                Context->dispatch = &GLESv2_DISPATCH_TABLE;
            }
#else
            Context->dispatch = gcvNULL;
#endif /* VIVANTE_NO_3D */
            break;

        case EGL_OPENVG_API:
#ifndef VIVANTE_NO_VG
            Context->dispatch = &OpenVG_DISPATCH_TABLE;
#else
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gcdZONE_EGL_CONTEXT,
                          "%s(%d): %s",
                          __FUNCTION__, __LINE__,
                          "VG driver is not available.");

            return gcvNULL;
#endif
            break;
        }
#else
        int index = -1, i;
        gctHANDLE library;
        gctPOINTER pointer = gcvNULL;

        for (i = 0; i < 2; i++)
        {
            gceSTATUS status;

            /* Get module handle and API index. */
            library = veglGetModule(gcvNULL, EGL_FALSE, Context, &index);
            if (library == gcvNULL)
            {
                return gcvNULL;
            }

            /* Query the dispatch table name. */
            status =  gcoOS_GetProcAddress(gcvNULL,
                                           library,
                                           _dispatchNames[index],
                                           &pointer);

            if (gcmIS_SUCCESS(status))
            {
                Context->dispatch = pointer;
                break;
            }

            if (status != gcvSTATUS_NOT_FOUND)
            {
                return gcvNULL;
            }
        }
#endif
    }

    /* Return dispatch table. */
    return Context->dispatch;
}

void *
_CreateApiContext(
    VEGLThreadData Thread,
    VEGLContext Context,
    gcoOS Os,
    gcoHAL Hal,
    void * SharedContext
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, Context);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x,0x%x,0x%x,0x%x",
                  __FUNCTION__, __LINE__,
                  Thread, Context, Os, Hal, SharedContext);

    if ((dispatch == gcvNULL)
    ||  (dispatch->createContext == gcvNULL)
    )
    {
        return gcvNULL;
    }

    return (*dispatch->createContext)(Os, Hal, SharedContext);
}

EGLBoolean
_DestroyApiContext(
    VEGLThreadData Thread,
    VEGLContext Context,
    void * ApiContext
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, Context);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x,0x%x",
                  __FUNCTION__, __LINE__,
                  Thread, ApiContext);

    if ((dispatch == gcvNULL)
    ||  (dispatch->destroyContext == gcvNULL)
    )
    {
        return EGL_FALSE;
    }

    return (*dispatch->destroyContext)(ApiContext);
}

EGLBoolean
_FlushApiContext(
    VEGLThreadData Thread,
    VEGLContext Context,
    void * ApiContext
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, Context);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x,0x%x",
                  __FUNCTION__, __LINE__,
                  Thread, ApiContext);

    if (ApiContext == gcvNULL)
    {
        return EGL_TRUE;
    }

    if ((dispatch == gcvNULL)
    ||  (dispatch->flushContext == gcvNULL)
    )
    {
        return EGL_FALSE;
    }

    return (*dispatch->flushContext)(Context);
}

EGLBoolean
_SetApiContext(
    VEGLThreadData Thread,
    VEGLContext Context,
    void * ApiContext,
    gcoSURF Draw,
    gcoSURF Read,
    gcoSURF Depth
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, Context);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x,0x%x,0x%x,0x%x,0x%x",
                  __FUNCTION__, __LINE__,
                  Thread, ApiContext, Draw, Read, Depth);

    if ((dispatch == gcvNULL)
    ||  (dispatch->setContext == gcvNULL)
    )
    {
        return (ApiContext == gcvNULL) ? EGL_TRUE : EGL_FALSE;
    }

    return (*dispatch->setContext)(ApiContext, Draw, Read, Depth);
}

gceSTATUS
_SetBuffer(
    VEGLThreadData Thread,
    gcoSURF Draw
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x,0x%x",
                  __FUNCTION__, __LINE__,
                  Thread, Draw);

    if ((dispatch == gcvNULL)
    ||  (dispatch->setBuffer == gcvNULL)
    )
    {
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return (*dispatch->setBuffer)(Draw);
}

EGLBoolean
_Flush(
    VEGLThreadData Thread
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x",
                  __FUNCTION__, __LINE__,
                  Thread);

    if ((dispatch == gcvNULL)
    ||  (dispatch->flush == gcvNULL)
    )
    {
        return EGL_FALSE;
    }

    if (Thread->context != gcvNULL)
    {
        (*dispatch->flush)();
    }

    return EGL_TRUE;
}

EGLBoolean
_Finish(
    VEGLThreadData Thread
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x",
                  __FUNCTION__, __LINE__,
                  Thread);

    if ((dispatch == gcvNULL)
    ||  (dispatch->finish == gcvNULL)
    )
    {
        return EGL_FALSE;
    }

    if (Thread->context != gcvNULL)
    {
        (*dispatch->finish)();
    }

    return EGL_TRUE;
}

gcoSURF
_GetClientBuffer(
    VEGLThreadData Thread,
    void * Context,
    EGLClientBuffer Buffer
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x,0x%x,0x%x",
                  __FUNCTION__, __LINE__,
                  Thread, Context, Buffer);

    if ((dispatch == gcvNULL)
    ||  (dispatch->getClientBuffer == gcvNULL)
    )
    {
        return gcvNULL;
    }

    return (*dispatch->getClientBuffer)(Context, Buffer);
}

EGLBoolean
_eglProfileCallback(
    VEGLThreadData Thread,
    gctUINT32 Enum,
    gctUINT32 Value
    )
{
#if VIVANTE_PROFILER
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): 0x%x,%u,%u",
                  __FUNCTION__, __LINE__,
                  Thread, Enum, Value);

    if ((dispatch == gcvNULL)
    ||  (dispatch->profiler == gcvNULL)
    )
    {
        return EGL_FALSE;
    }

    (*dispatch->profiler)(gcvNULL, Enum, Value);
#endif

    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindAPI(
    EGLenum api
    )
{
    VEGLThreadData thread;

    gcmHEADER_ARG("api=0x%04x", api);
    gcmDUMP_API("${EGL eglBindAPI 0x%08X}", api);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        /* Fatal error. */
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    switch (api)
    {
    case EGL_OPENGL_ES_API:
        /* OpenGL ES API. */
        if (thread->api != api)
        {
            thread->api = api;
        }
        break;

    case EGL_OPENVG_API:
        /* OpenVG API. */
        if (thread->api != api)
        {
            thread->api = api;
        }
#if gcdENABLE_VG
        if (thread->openVGpipe)
        {
            gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_VG));
        }
        else
        {
            gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));
        }

#endif
        break;

    default:
        /* Bad parameter. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLenum EGLAPIENTRY
eglQueryAPI(
    void
    )
{
    VEGLThreadData thread;

    gcmHEADER();

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("%04x", EGL_NONE);
        return EGL_NONE;
    }

    /* Return current API. */
    gcmDUMP_API("${EGL eglQueryAPI := 0x%08X}", thread->api);
    gcmFOOTER_ARG("%04x", thread->api);
    return thread->api;
}

EGLContext EGLAPIENTRY
eglCreateContext(
    EGLDisplay Dpy,
    EGLConfig config,
    EGLContext SharedContext,
    const EGLint *attrib_list
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLContext sharedContext;
    gceSTATUS status;
    VEGLContext context;
    EGLint client = 1;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Dpy=0x%x config=0x%x SharedContext=0x%x attrib_list=0x%x",
                  Dpy, config, SharedContext, attrib_list);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    /* Create shortcuts to objects. */
    dpy           = VEGL_DISPLAY(Dpy);
    sharedContext = VEGL_CONTEXT(SharedContext);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL) || (dpy->signature != EGL_DISPLAY_SIGNATURE))
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    /* Test for valid bounded API. */
    if (thread->api == EGL_NONE)
    {
        /* Bad match. */
        thread->error = EGL_BAD_MATCH;
        gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    /* Test for valid config. */
    if (((VEGLConfig) config < dpy->config)
    ||  ((VEGLConfig) config >= dpy->config + dpy->configCount)
    )
    {
        thread->error = EGL_BAD_CONFIG;
        gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    /* Get attribute. */
    if (attrib_list != gcvNULL)
    {
        EGLint i = 0;

        for (i = 0; attrib_list[i] != EGL_NONE; i += 2)
        {
            switch (attrib_list[i])
            {
            case EGL_CONTEXT_CLIENT_VERSION:
                if (thread->api == EGL_OPENGL_ES_API)
                {
                    /* Save client. */
                    client = attrib_list[i + 1];
                    break;
                }
                /* Pass through for error. */
            default:
                /* Invalid attrribute. */
                thread->error = EGL_BAD_MATCH;
                gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
                return EGL_NO_CONTEXT;
            }
        }
    }

    if (thread->api == EGL_OPENGL_ES_API)
    {
        if ((client == 1)
        &&  !(((VEGLConfig) config)->renderableType & EGL_OPENGL_ES_BIT)
        )
        {
            thread->error = EGL_BAD_CONFIG;
            gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
            return EGL_NO_CONTEXT;
        }

        if ((client == 2)
        &&  !(((VEGLConfig) config)->renderableType & EGL_OPENGL_ES2_BIT)
        )
        {
            thread->error = EGL_BAD_CONFIG;
            gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
            return EGL_NO_CONTEXT;
        }
    }
    else
    {
        if ((thread->api == EGL_OPENVG_API)
        &&  !(((VEGLConfig) config)->renderableType & EGL_OPENVG_BIT)
        )
        {
            thread->error = EGL_BAD_CONFIG;
            gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
            return EGL_NO_CONTEXT;
        }
    }


    /* Test if shared context is compatible. */
    if (sharedContext != gcvNULL)
    {
        /* Test for a valid context. */
        VEGLContext p_ctx;
        for (p_ctx = dpy->contextStack; p_ctx != gcvNULL; p_ctx = p_ctx->next)
        {
            if (p_ctx == sharedContext)
            {
                break;
            }
        }

        if (p_ctx == gcvNULL)
        {
            thread->error = EGL_BAD_CONTEXT;
            gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
            return EGL_NO_CONTEXT;
        }

        if ((sharedContext->thread != thread)
        ||  (sharedContext->api    != thread->api)
        )
        {
            /* Bad match. */
            thread->error = EGL_BAD_MATCH;
            gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
            return EGL_NO_CONTEXT;
        }
    }

    /* Flush any existing context. */
    if (thread->context != EGL_NO_CONTEXT)
    {
        gcmVERIFY(_FlushApiContext(thread,
                                   thread->context,
                                   thread->context->context));
    }

    /* Create new context. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(struct eglContext),
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        thread->error = EGL_BAD_ALLOC;
        gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    context = pointer;

    /* Reference the EGLDisplay. */
    if (!veglReferenceDisplay(thread, dpy))
    {
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, context));

        thread->error = EGL_BAD_ALLOC;
        gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    /* Zero the context memory. */
    gcoOS_ZeroMemory(context, gcmSIZEOF(struct eglContext));

    /* Initialize context. */
    context->thread        = thread;
    context->api           = thread->api;
    context->client        = client;
    context->display       = dpy;
    context->sharedContext = sharedContext;
    context->draw          = EGL_NO_SURFACE;
    context->read          = EGL_NO_SURFACE;
    context->dispatch      = gcvNULL;
    context->signature     = EGL_CONTEXT_SIGNATURE;

    /* Push context into stack. */
    context->next     = dpy->contextStack;
    dpy->contextStack = context;

    /* Create context for API. */
    context->context = _CreateApiContext(
        thread, context, gcvNULL, gcvNULL,
        (sharedContext != gcvNULL)
            ? sharedContext->context
            : gcvNULL
        );

#if gcdRENDER_THREADS
    /* Create the render threads. */
    eglfCreateRenderThreads(context);
#endif

    if (context->context == gcvNULL)
    {
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, context));

        thread->error = EGL_FALSE;
        gcmFOOTER_ARG("0x%x", EGL_NO_CONTEXT);
        return gcvNULL;
    }

    /* Useful for debugging */
    gcmTRACE(
        gcvLEVEL_INFO,"a,b,g,r=%d,%d,%d,%d, d,s=%d,%d, id=%d, AA=%d, t=0x%08X",
            ((VEGLConfig) config)->alphaSize,
            ((VEGLConfig) config)->blueSize,
            ((VEGLConfig) config)->greenSize,
            ((VEGLConfig) config)->redSize,
            ((VEGLConfig) config)->depthSize,
            ((VEGLConfig) config)->stencilSize,
            ((VEGLConfig) config)->configId,
            ((VEGLConfig) config)->samples,
            ((VEGLConfig) config)->surfaceType
        );

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglCreateContext 0x%08X 0x%08X 0x%08X (0x%08X) := "
                "0x%08X",
                Dpy, config, SharedContext, attrib_list, context);
    gcmDUMP_API_ARRAY_TOKEN(attrib_list, EGL_NONE);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("0x%x", context);
    return context;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglMakeCurrent(
    EGLDisplay Dpy,
    EGLSurface Draw,
    EGLSurface Read,
    EGLContext Ctx
    )
{
    EGLBoolean result;
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLSurface draw;
    VEGLSurface read;
    VEGLContext ctx, current;

    gcmHEADER_ARG("Dpy=0x%x Draw=0x%x Read=0x%x Ctx=0x%x",
                  Dpy, Draw, Read, Ctx);
    gcmDUMP_API("${EGL eglMakeCurrent 0x%08X 0x%08X 0x%08X 0x%08X}",
                Dpy, Draw, Read, Ctx);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Create shortcuts to objects. */
    dpy  = VEGL_DISPLAY(Dpy);
    draw = VEGL_SURFACE(Draw);
    read = VEGL_SURFACE(Read);
    ctx  = VEGL_CONTEXT(Ctx);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for release of context. */
    if ((draw == EGL_NO_SURFACE)
    &&  (read == EGL_NO_SURFACE)
    &&  (ctx  == EGL_NO_CONTEXT)
    )
    {
        if (thread->context != EGL_NO_CONTEXT)
        {
            /* Flush current context. */
            gcmVERIFY(
                _FlushApiContext(thread,
                                 thread->context,
                                 thread->context->context));

#if gcdRENDER_THREADS
            /* Set API context for all render threads. */
            eglfRenderThreadSetContext(thread->context,
                                       gcvNULL,
                                       gcvNULL,
                                       gcvNULL);

            /* Wait for all render threads. */
            eglfWaitRenderThread(gcvNULL);
#endif

#ifndef VIVANTE_NO_3D
#if gcdENABLE_VG
            if (!(thread->openVGpipe && thread->api == EGL_OPENVG_API))
#endif
            {
                /* Resolve current color buffer. */
                if (gcmIS_ERROR(gcoSURF_Resolve(thread->context->draw->renderTarget,
                                            thread->context->draw->renderTarget)))
                {
                    gcmFOOTER_ARG("%d", EGL_FALSE);
                         thread->error = EGL_FALSE;
                    return EGL_FALSE;
                }
            }
#endif

            /* Remove context. */
            gcmVERIFY(
                _SetApiContext(thread,
                               thread->context,
                               thread->context->context,
                               gcvNULL, gcvNULL, gcvNULL));

            /* Suspend the worker thread. */
            veglSuspendSwapWorker(dpy);

            if (thread->context->draw != gcvNULL)
            {
                /* Make sure all workers have been processed. */
                gcmVERIFY_OK(gcoOS_WaitSignal(gcvNULL,
                                              dpy->doneSignal,
                                              gcvINFINITE));

                /* Dereference the current draw surface. */
                veglDereferenceSurface(thread,
                                       thread->context->draw,
                                       EGL_FALSE);

                thread->context->draw = gcvNULL;
            }

            if (thread->context->read != gcvNULL)
            {
                /* Dereference the current read surface. */
                veglDereferenceSurface(thread,
                                       thread->context->read,
                                       EGL_FALSE);

                thread->context->read = gcvNULL;
            }

            /* Unbind thread from context. */
            thread->context->thread = gcvNULL;

            /* Set context to EGL_NO_CONTEXT. */
            thread->lastClient = thread->context->client;
            thread->context    = EGL_NO_CONTEXT;

            /* Resume the thread. */
            veglResumeSwapWorker(dpy);
        }

        /* Uninstall the owner thread */
        dpy->ownerThread = gcvNULL;

        /* Success. */
        thread->error = EGL_SUCCESS;
        gcmFOOTER_ARG("%d", EGL_TRUE);
        return EGL_TRUE;
    }

    /* Verify both surfaces and the context. */
    if ((draw == EGL_NO_SURFACE)
    ||  (read == EGL_NO_SURFACE)
    ||  (ctx  == EGL_NO_CONTEXT)
    )
    {
        /* Bad match. */
        thread->error = EGL_BAD_MATCH;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test if surface is locked. */
    if (draw->locked || read->locked)
    {
        thread->error = EGL_BAD_ACCESS;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if (thread->openVGpipe && thread->api == EGL_OPENVG_API)
    {
        if ((draw->type == EGL_WINDOW_BIT) &&
            !veglIsColorFormatSupport(dpy->hdc, &draw->config))
        {
            /* Bad match. */
            thread->error = EGL_BAD_MATCH;
            gcmFOOTER_ARG("%d", EGL_FALSE);
            return EGL_FALSE;
        }
    }

    /* Suspend the worker thread. */
    veglSuspendSwapWorker(dpy);

    do
    {
        if (ctx->draw != draw)
        {
#ifndef __QNXNTO__
            if (ctx->draw != gcvNULL &&
                ctx->draw->texBinder != gcvNULL &&
                ctx->draw->renderTarget != gcvNULL)
            {
                /* If the render target is bound to a texture, sync the content. */
                if (gcmIS_ERROR(gcoSURF_Resolve(ctx->draw->renderTarget, ctx->draw->texBinder)))
                {
                    gcmFOOTER_ARG("%d", EGL_FALSE);
                    thread->error = EGL_FALSE;
                    return EGL_FALSE;
                }
            }
#endif

            /* Dereference the current draw surface. */
            if (ctx->draw != gcvNULL)
            {
                veglDereferenceSurface(thread, ctx->draw, EGL_FALSE);

                ctx->draw = EGL_NO_SURFACE;
            }

            /* Reference the new draw surface. */
            if (!veglReferenceSurface(thread, draw))
            {
                /* Error. */
                result = EGL_FALSE;
                break;
            }

            /* Set the new draw surface. */
            ctx->draw = draw;
        }

        if (ctx->read != read)
        {
            /* Dereference the current read surface. */
            if (ctx->read != gcvNULL)
            {
                veglDereferenceSurface(thread, ctx->read, EGL_FALSE);

                ctx->read = EGL_NO_SURFACE;
            }

            /* Reference the new read surface. */
            if (!veglReferenceSurface(thread, read))
            {
                /* Error. */
                result = EGL_FALSE;
                break;
            }

            /* Set the new read surface. */
            ctx->read = read;
        }

        /* Success. */
        result = EGL_TRUE;
    }
    while (gcvFALSE);

    /* Resume the thread. */
    veglResumeSwapWorker(dpy);

    /* Check for a switch in the API. */
    if (thread->api != ctx->api)
    {
        /* This is a context switch. We need to reload the dispatch table. */
        ctx->dispatch = gcvNULL;
    }

    if (!result)
    {
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Set current context. */
    current         = thread->context;
    thread->context = ctx;

    if (!_SetApiContext(thread,
                        ctx,
                        ctx->context,
                        draw->renderTarget,
                        read->renderTarget,
                        draw->depthBuffer))
    {
        /* Roll back the current context. */
        thread->context = current;

        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

#if gcdRENDER_THREADS
    /* Set API context for all render threads. */
    eglfRenderThreadSetContext(ctx,
                               draw->renderTarget,
                               read->renderTarget,
                               draw->depthBuffer);
#endif

    /* Set the target. */
    if (gcmIS_ERROR(veglSetDriverTarget(thread, draw)))
    {
        /* Roll back the current context. */
        thread->context = current;

        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Set current context. */
    thread->context = ctx;
    ctx->thread = thread;

    if (thread->dump != gcvNULL)
    {
        /* Start a new frame. */
        gcmVERIFY_OK(gcoDUMP_FrameBegin(thread->dump));
    }

    /* Install the owner thread */
    dpy->ownerThread = gcoOS_GetCurrentThreadID();

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyContext(
    EGLDisplay Dpy,
    EGLContext Ctx
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLContext ctx;
    VEGLContext stack;

    gcmHEADER_ARG("Dpy=0x%x Ctx=0x%x", Dpy, Ctx);
    gcmDUMP_API("${EGL eglDestroyContext 0x%08X 0x%08X}", Dpy, Ctx);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("%d (line %d)", EGL_FALSE, __LINE__);
        return EGL_FALSE;
    }

    /* Create shortcuts to objects. */
    dpy = VEGL_DISPLAY(Dpy);
    ctx = VEGL_CONTEXT(Ctx);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("%d (line %d)", EGL_FALSE, __LINE__);
        return EGL_FALSE;
    }

    /* Test if EGLDisplay structure has been initialized. */
    if (dpy->referenceDpy == gcvNULL)
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("%d (line %d)", EGL_FALSE, __LINE__);
        return EGL_FALSE;
    }

    /* Test for valid bounded API. */
    if (thread->api == EGL_NONE)
    {
        /* Bad match. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("%d (line %d)", EGL_FALSE, __LINE__);
        return EGL_FALSE;
    }

    /* Test if ctx is valid. */
    if (ctx == gcvNULL)
    {
        thread->error = EGL_BAD_CONTEXT;
        gcmFOOTER_ARG("%d (line %d)", EGL_FALSE, __LINE__);
        return EGL_FALSE;
    }

    /* Find the context object on the stack. */
    for (stack = dpy->contextStack; stack != gcvNULL; stack = stack->next)
    {
        if (stack == ctx)
        {
            break;
        }
    }

    /* Make sure the context is on the stack. */
    if (stack == gcvNULL)
    {
        thread->error = EGL_BAD_CONTEXT;
        gcmFOOTER_ARG("%d (line %d)", EGL_FALSE, __LINE__);
        return EGL_FALSE;
    }

    /* Make sure the API is valid. */
    if (ctx->api != thread->api)
    {
        thread->error = EGL_BAD_CONTEXT;
        gcmFOOTER_ARG("%d (line %d)", EGL_FALSE, __LINE__);
        return EGL_FALSE;
    }

#if gcdRENDER_THREADS
    /* Destroy all render threads. */
    eglfDestroyRenderThreads(ctx);
#endif

    /* Destroy context. */
    _DestroyApiContext(thread, ctx, ctx->context);

    /* Deference any surfaces. */
    if (ctx->draw != EGL_NO_SURFACE)
    {
        veglDereferenceSurface(thread, ctx->draw, EGL_FALSE);
    }

    if (ctx->read != EGL_NO_SURFACE)
    {
        veglDereferenceSurface(thread, ctx->read, EGL_FALSE);
    }

    if (thread->context == ctx)
    {
        /* Current context has been destroyed. */
        thread->context  = gcvNULL;
    }

    /* Pop EGLContext from the stack. */
    if (ctx == dpy->contextStack)
    {
        /* Simple - it is the top of the stack. */
        dpy->contextStack = ctx->next;
    }
    else
    {
        /* Walk the stack. */
        for (stack = dpy->contextStack; stack != gcvNULL; stack = stack->next)
        {
            /* Check if the next context on the stack is ours. */
            if (stack->next == ctx)
            {
                /* Pop the context from the stack. */
                stack->next = ctx->next;
                break;
            }
        }
    }

    /* Dereference the display. */
    veglDereferenceDisplay(thread, ctx->display, EGL_FALSE);

    /* Reset the context. */
    ctx->thread = gcvNULL;
    ctx->api    = EGL_NONE;

    /* Execute events accumulated in the buffer if any. */
    gcmVERIFY_OK(gcoHAL_Commit(gcvNULL, gcvFALSE));

    /* Free the eglContext structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, ctx));

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseThread(
    void
    )
{
    VEGLThreadData thread;

    gcmHEADER();
    gcmDUMP_API("${EGL eglReleaseThread}");

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if (thread->context != gcvNULL)
    {
        /* Unbind the context. */
        eglMakeCurrent(thread->context->display,
                       EGL_NO_SURFACE,
                       EGL_NO_SURFACE,
                       thread->context);

        /* Set API to NONE */
        thread->api = EGL_NONE;
    }

    /* Destroy thread data. */
    gcoOS_FreeThreadData(gcvFALSE);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("%d", EGL_TRUE);
    return EGL_TRUE;
}

#if !gcdSTATIC_LINK
EGLAPI
#endif
void * EGLAPIENTRY
veglGetCurrentAPIContext(
    void
    )
{
    VEGLThreadData thread;
    void * context;

    gcmHEADER();

    thread = veglGetThreadData();

#if gcdRENDER_THREADS
    if (thread->renderThread != gcvNULL)
    {
        /* Return pointer to API context for a render thread. */
        context = thread->renderThread->apiContext;
    }
    else
#endif
    {
        if ((thread == gcvNULL) || (thread->context == gcvNULL))
        {
            context = gcvNULL;
        }
        else
        {
            context = thread->context->context;
        }
    }

    gcmFOOTER_ARG("0x%x", context);
    return context;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitGL(
    void
    )
{
    EGLenum api;
    EGLBoolean result;

    gcmHEADER();
    gcmDUMP_API("${EGL eglWaitGL}");

    /* Backwards compatibility. */
    api = eglQueryAPI();
    eglBindAPI(EGL_OPENGL_ES_API);
    result = eglWaitClient();
    eglBindAPI(api);

    gcmFOOTER_ARG("%d", result);
    return result;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitClient(
    void
    )
{
    VEGLThreadData thread;
    EGLBoolean result = EGL_TRUE;

    gcmHEADER();
    gcmDUMP_API("${EGL eglWaitClient}");

    thread = veglGetThreadData();

    if (thread == gcvNULL)
    {
        result = EGL_FALSE;
    }
    else
    {
        result = _Finish(thread);

        if (result &&
            thread->context &&
            thread->context->draw &&
            thread->context->draw->renderTarget &&
            thread->context->draw->pixmap)
        {
            VEGLSurface surface = thread->context->draw;
            gcmASSERT(surface != gcvNULL);

            /*
               Resolve from RT to pixmap.
             */
            if (gcmIS_ERROR(gcoSURF_Resolve(
                                surface->renderTarget,
                                surface->pixmapSurface)))
            {
                result = EGL_FALSE;
            }
            else
            {
                /*
                  Do a stalled commit here as the
                  next operation could be a CPU op.
                */
                gcoHAL_Commit(gcvNULL, gcvTRUE);

                if (surface->tempPixmapBits != gcvNULL)
                {
                    EGLint height;

                    height = surface->config.height;

                    if (surface->pixmapStride == surface->tempPixmapStride)
                    {
                        gcmVERIFY_OK(gcoOS_MemCopy(
                            surface->pixmapBits,
                            surface->tempPixmapBits,
                            surface->pixmapStride * height
                            ));
                    }
                    else
                    {
                        gctINT line;
                        gctSIZE_T widthInBytes = (gctSIZE_T)(surface->config.width * surface->bitsPerPixel / 8);

                        for (line = 0; line < height; line++)
                        {
                            /* Copy a scanline. */
                            gcmVERIFY_OK(gcoOS_MemCopy(
                                (gctUINT8_PTR) surface->pixmapBits + surface->pixmapStride * line,
                                (gctUINT8_PTR) surface->tempPixmapBits + surface->tempPixmapStride * line,
                                widthInBytes
                                ));
                        }
                    }
                }
            }
        }
    }

    gcmFOOTER_ARG("%d", result);
    return result;
}

EGLenum
_BindTexImage(
    VEGLThreadData Thread,
    gcoSURF Surface,
    EGLenum Format,
    EGLenum Target,
    EGLBoolean Mipmap,
    EGLint Level,
    gcoSURF *BindTo
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): Thread=0x%08x,Surface=0x%08x,Format=0x%04x",
                  __FUNCTION__, __LINE__,
                  Thread, Surface, Format);
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "Target=0x%04x,Mipmap=%d,Level=0x%04x",
                  Target, Mipmap, Level);

    if ((dispatch == gcvNULL)
    ||  (dispatch->bindTexImage == gcvNULL)
    )
    {
        return EGL_BAD_ACCESS;
    }

    return (*dispatch->bindTexImage)(Surface, Format, Target, Mipmap, Level, BindTo);
}

EGLenum
_CreateImageTexture(
    VEGLThreadData Thread,
    EGLenum Target,
    gctINT Texture,
    gctINT Level,
    gctINT Depth,
    gctPOINTER Image
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): Thread=0x%08x,Target=0x%04x,Texture=0x%08x",
                  __FUNCTION__, __LINE__, Thread, Target, Texture);
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "Level=%d,Depth=%d,Image=0x%08x", Level, Depth, Image);

    if ((dispatch == gcvNULL)
    ||  (dispatch->createImageTexture == gcvNULL)
    )
    {
        return EGL_BAD_ACCESS;
    }

    return (*dispatch->createImageTexture)(Target,
                                           Texture,
                                           Level,
                                           Depth,
                                           Image);
}

EGLenum
_CreateImageFromRenderBuffer(
    VEGLThreadData Thread,
    gctUINT Renderbuffer,
    gctPOINTER Image
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): Thread=0x%08x, RenderBuffer=%u,Image=0x%08x",
                  __FUNCTION__, __LINE__,
                  Thread, Renderbuffer, Image);

    if ((dispatch == gcvNULL)
    ||  (dispatch->createImageRenderbuffer == gcvNULL)
    )
    {
        return EGL_BAD_ACCESS;
    }

    return (*dispatch->createImageRenderbuffer)(Renderbuffer, Image);
}

EGLenum
_CreateImageFromVGParentImage(
    VEGLThreadData  Thread,
    unsigned int Parent,
    VEGLImage Image
    )
{
    veglDISPATCH * dispatch = _GetDispatch(Thread, gcvNULL);
    EGLenum status;
    gctPOINTER images = gcvNULL;
    int count = 0;

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcdZONE_EGL_CONTEXT,
                  "%s(%d): Thread=0x%08x,Parent=%u,Image=0x%08x",
                  __FUNCTION__, __LINE__,
                  Thread, Parent, Image);

    if ((dispatch == gcvNULL)
    ||  (dispatch->createImageParentImage == gcvNULL)
    )
    {
        return EGL_BAD_ACCESS;
    }

    status = (*dispatch->createImageParentImage)(Parent,
                                                 &images,
                                                 &count);

    if (count == 0)
    {
        return status;
    }

    /* Fill the VEGLImage list. */
    if ((Image != gcvNULL) && (images != gcvNULL))
    {
        gcmVERIFY_OK(gcoOS_MemCopy(&Image->image,
                                   images,
                                   gcmSIZEOF(Image->image)));
        Image->signature = EGL_IMAGE_SIGNATURE;
        Image->next      = gcvNULL;
    }

    if (images != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, images));
    }

    return EGL_SUCCESS;
}
