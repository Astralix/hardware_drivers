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
#define _GC_OBJ_ZONE    gcdZONE_EGL_API

EGLAPI EGLBoolean EGLAPIENTRY eglSwapInterval(
    EGLDisplay Dpy,
    EGLint Interval
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;

    gcmHEADER_ARG("Dpy=0x%x Interval=%d", Dpy, Interval);
    gcmDUMP_API("$(EGL eglSwapInterval 0x%08X 0x%08X}", Dpy, Interval);

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

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ( (dpy == gcvNULL) ||
         (dpy->signature != EGL_DISPLAY_SIGNATURE) )
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

    /* Ignore Interval value. (Clamp to 1). */

    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLContext EGLAPIENTRY
eglGetCurrentContext(
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

        /* Fatal error. */
        gcmFOOTER_ARG("return=0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    if (thread->api == EGL_NONE)
    {
        /* Fatal error. */
        gcmFOOTER_ARG("return=0x%x", EGL_NO_CONTEXT);
        return EGL_NO_CONTEXT;
    }

    gcmDUMP_API("${EGL eglGetCurrentContext := 0x%08X}", thread->context);
    gcmFOOTER_ARG("return=0x%x", thread->context);
    return thread->context;
}

EGLAPI EGLSurface EGLAPIENTRY
eglGetCurrentSurface(
    EGLint readdraw
    )
{
    VEGLThreadData thread;
    EGLSurface result;

    gcmHEADER_ARG("readdraw=0x%x", readdraw);

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

    if ( (thread->context          == gcvNULL)           ||
         (thread->context->context == EGL_NO_CONTEXT) )
    {
        gcmFOOTER_ARG("return=0x%x", EGL_NO_SURFACE);
        return EGL_NO_SURFACE;
    }

    switch (readdraw)
    {
    case EGL_READ:
        result = thread->context->read;
        break;

    case EGL_DRAW:
        result = thread->context->draw;
        break;

    default:
        thread->error = EGL_BAD_PARAMETER;
        result = EGL_NO_SURFACE;
        break;
    }

    gcmDUMP_API("${EGL eglGetCurrentSurface 0x%08X := 0x%08X}",
                readdraw, result);
    gcmFOOTER_ARG("return=0x%x", result);
    return result;
}

EGLAPI EGLDisplay EGLAPIENTRY
eglGetCurrentDisplay(
    void
    )
{
    VEGLThreadData thread;

    gcmHEADER();
    gcmDUMP_API("${EGL %s}", __FUNCTION__);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("return=0x%x", EGL_NO_DISPLAY);
        return EGL_NO_DISPLAY;
    }

    if (thread->context == EGL_NO_CONTEXT)
    {
        /* Fatal error. */
        gcmFOOTER_ARG("return=0x%x", EGL_NO_DISPLAY);
        return EGL_NO_DISPLAY;
    }

    gcmDUMP_API("${EGL eglGetCurrentDisplay := 0x%08X}",
                thread->context->display);
    gcmFOOTER_ARG("return=0x%x", thread->context->display);
    return thread->context->display;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQueryContext(
    EGLDisplay dpy,
    EGLContext ctx,
    EGLint attribute,
    EGLint *value
    )
{
    VEGLDisplay display;
    VEGLContext context, p_ctx;
    VEGLThreadData thread;

    gcmHEADER_ARG("dpy=0x%x ctx=0x%x attribute=%d value=0x%x",
                    dpy, ctx, attribute, value);

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

    /* Test for a valid display. */
    display = VEGL_DISPLAY(dpy);
    if ( (display == gcvNULL) ||
         (display->signature != EGL_DISPLAY_SIGNATURE) )
    {   /* An invalid display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for a valid context. */
    context = VEGL_CONTEXT(ctx);
    for (p_ctx = display->contextStack;
         p_ctx != gcvNULL;
         p_ctx = p_ctx->next)
    {
        if (p_ctx == context)
            break;
    }
    if ( (p_ctx == gcvNULL) ||
         (context == gcvNULL) ||
         (context->thread != thread) ||
         (context->api != thread->api) )
    {   /* An invalid context. */
        thread->error = EGL_BAD_CONTEXT;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if (value != gcvNULL)
    {
        switch (attribute)
        {
        case EGL_CONFIG_ID:
            if (thread->context == EGL_NO_CONTEXT)
            {
                /* Fatal error. */
                gcmFOOTER_ARG("return=%d", EGL_FALSE);
                return EGL_FALSE;
            }
            *value = context->draw->config.configId;
            break;

        case EGL_CONTEXT_CLIENT_TYPE:
            *value = context->api;
            break;

        case EGL_CONTEXT_CLIENT_VERSION:
            *value = context->client;
            break;

        case EGL_RENDER_BUFFER:
            {
                if (thread->context == EGL_NO_CONTEXT)
                {
                    /* Fatal error. */
                    gcmFOOTER_ARG("return=%d", EGL_NONE);
                    *value = EGL_NONE;
                    return EGL_TRUE;
                }
                if (context->draw->config.surfaceType & EGL_PBUFFER_BIT)
                    *value = EGL_BACK_BUFFER;
                else if (context->draw->config.surfaceType & EGL_PIXMAP_BIT)
                    *value = EGL_SINGLE_BUFFER;
                else if (context->draw->config.surfaceType & EGL_WINDOW_BIT)
                    *value = context->draw->buffer;
            }
            break;

        default:
            thread->error = EGL_BAD_ATTRIBUTE;
            gcmFOOTER_ARG("return=%d", EGL_FALSE);
            return EGL_FALSE;
        }

        gcmDUMP_API("${EGL eglQueryContext 0x%08X 0x%08X 0x%08X := 0x%08X}",
                    dpy, ctx, attribute, *value);
    }

    gcmFOOTER_ARG("return=%d", EGL_TRUE);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitNative(
    EGLint engine
    )
{
    VEGLThreadData thread;
    EGLBoolean result;

    gcmHEADER_ARG("engine=0x%x", engine);

    gcmDUMP_API("${EGL eglWaitNative 0x%08X}", engine);

    thread = veglGetThreadData();

    result = EGL_TRUE;

    if (thread &&
        thread->context &&
        thread->context->draw &&
        thread->context->draw->renderTarget &&
        thread->context->draw->pixmap)
    {
        VEGLSurface surface = thread->context->draw;
        gcmASSERT(surface != gcvNULL);

        /* Use the temp surface. */
        if (surface->tempPixmapBits != gcvNULL)
        {
            EGLint height;

            height = surface->config.height;

            if (surface->pixmapStride == surface->tempPixmapStride)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(
                    surface->tempPixmapBits,
                    surface->pixmapBits,
                    surface->pixmapStride * height
                    ));
            }
            else
            {
                gctINT line;

                for (line = 0; line < height; line++)
                {
                    /* Copy a scanline. */
                    gcmVERIFY_OK(gcoOS_MemCopy(
                        (gctUINT8_PTR) surface->tempPixmapBits + surface->tempPixmapStride * line,
                        (gctUINT8_PTR) surface->pixmapBits + surface->pixmapStride * line,
                        surface->config.width
                        ));
                }
            }
        }

        /*
           Resolve from pixmap to RT.
           No need to do a stalled commit here as
           the next operation will be a GPU op as well.
         */
        if (gcmIS_ERROR(gcoSURF_Resolve(
                            surface->pixmapSurface,
                            surface->renderTarget)))
        {
            result = EGL_FALSE;
        }
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

typedef void (* EGL_PROC)(void);

#if !defined ANDROID && !defined (__QNXNTO__)
static veglLOOKUP _veglLookup[] =
{
    eglMAKE_LOOKUP(eglGetError),
    eglMAKE_LOOKUP(eglGetDisplay),
    eglMAKE_LOOKUP(eglInitialize),
    eglMAKE_LOOKUP(eglTerminate),
    eglMAKE_LOOKUP(eglQueryString),
    eglMAKE_LOOKUP(eglGetConfigs),
    eglMAKE_LOOKUP(eglChooseConfig),
    eglMAKE_LOOKUP(eglGetConfigAttrib),
    eglMAKE_LOOKUP(eglCreateWindowSurface),
    eglMAKE_LOOKUP(eglCreatePbufferSurface),
    eglMAKE_LOOKUP(eglCreatePixmapSurface),
    eglMAKE_LOOKUP(eglDestroySurface),
    eglMAKE_LOOKUP(eglQuerySurface),
    eglMAKE_LOOKUP(eglBindAPI),
    eglMAKE_LOOKUP(eglQueryAPI),
    eglMAKE_LOOKUP(eglWaitClient),
    eglMAKE_LOOKUP(eglReleaseThread),
    eglMAKE_LOOKUP(eglCreatePbufferFromClientBuffer),
    eglMAKE_LOOKUP(eglSurfaceAttrib),
    eglMAKE_LOOKUP(eglBindTexImage),
    eglMAKE_LOOKUP(eglReleaseTexImage),
    eglMAKE_LOOKUP(eglSwapInterval),
    eglMAKE_LOOKUP(eglCreateContext),
    eglMAKE_LOOKUP(eglDestroyContext),
    eglMAKE_LOOKUP(eglMakeCurrent),
    eglMAKE_LOOKUP(eglGetCurrentContext),
    eglMAKE_LOOKUP(eglGetCurrentSurface),
    eglMAKE_LOOKUP(eglGetCurrentDisplay),
    eglMAKE_LOOKUP(eglQueryContext),
    eglMAKE_LOOKUP(eglWaitGL),
    eglMAKE_LOOKUP(eglWaitNative),
    eglMAKE_LOOKUP(eglSwapBuffers),
    eglMAKE_LOOKUP(eglCopyBuffers),
    eglMAKE_LOOKUP(eglGetProcAddress),
    eglMAKE_LOOKUP(eglLockSurfaceKHR),
    eglMAKE_LOOKUP(eglUnlockSurfaceKHR),
    eglMAKE_LOOKUP(eglCreateImageKHR),
    eglMAKE_LOOKUP(eglDestroyImageKHR),
    eglMAKE_LOOKUP(eglCreateSyncKHR),
    eglMAKE_LOOKUP(eglDestroySyncKHR),
    eglMAKE_LOOKUP(eglClientWaitSyncKHR),
    eglMAKE_LOOKUP(eglSignalSyncKHR),
    eglMAKE_LOOKUP(eglGetSyncAttribKHR),
#ifndef VIVANTE_NO_3D
    eglMAKE_LOOKUP(eglCompositionBeginVIV),
    eglMAKE_LOOKUP(eglComposeLayerVIV),
    eglMAKE_LOOKUP(eglCompositionEndVIV),
#endif
    { gcvNULL, gcvNULL }
};

static EGL_PROC
_Lookup(
    veglLOOKUP * Lookup,
    const char * Name,
    const char * Appendix
    )
{
    /* Test for lookup. */
    if (Lookup != gcvNULL)
    {
        /* Loop while there are entries in the lookup tabke. */
        while (Lookup->name != gcvNULL)
        {
            const char *p = Name;
            const char *q = Lookup->name;

            /* Compare the name and the lookup table. */
            while ((*p == *q) && (*p != '\0') && (*q != '\0'))
            {
                ++p;
                ++q;
            }

            /* No match yet, see if it matches if we append the appendix. */
            if ((*p != *q) && (*p == '\0') && (Appendix != gcvNULL))
            {
                p = Appendix;

                /* Compare the appendix and the lookup table. */
                while ((*p == *q) && (*p != '\0') && (*q != '\0'))
                {
                    ++p;
                    ++q;
                }
            }

            /* See if we have a match. */
            if (*p == *q)
            {
                /* Return the function pointer. */
                return Lookup->function;
            }

            /* Next lookup entry. */
            ++Lookup;
        }
    }

    /* No match found. */
    return gcvNULL;
}
#endif

#define gcmDEF2STRING_1(def) #def
#define gcmDEF2STRING(def) gcmDEF2STRING_1(def)

#if gcdSTATIC_LINK
#ifndef VIVANTE_NO_3D
        extern veglDISPATCH GLES_CM_DISPATCH_TABLE;
        extern veglDISPATCH GLESv2_DISPATCH_TABLE;
#   endif
#ifndef VIVANTE_NO_VG
        extern veglDISPATCH OpenVG_DISPATCH_TABLE;
#   endif
#endif

EGLAPI EGL_PROC EGLAPIENTRY
eglGetProcAddress(
    const char *procname
    )
{
    union gcuVARIANT
    {
        EGL_PROC   func;
        gctPOINTER ptr;
    }
    proc;
    VEGLThreadData thread;
    const char * appendix;
    veglDISPATCH * dispatch;
#if !gcdSTATIC_LINK
    gctHANDLE library;
    char * name;
    gctSIZE_T nameLen = 0, appendixLen = 0;
    gctSIZE_T len = 0;
    gctPOINTER pointer = gcvNULL;
    gctINT32 index = -1;
    gctUINT32 i;
#endif

    gcmHEADER_ARG("procname=%s", procname);

    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("0x%x", gcvNULL);
        return gcvNULL;
    }

    if ((procname == gcvNULL)
    ||  (procname[0] == '\0')
    )
    {
        gcmFOOTER_ARG("0x%x", gcvNULL);
        return gcvNULL;
    }

    /* In Android, we want to go through the Android' redirection libraries,
       to get the function pointers, instead of looking them up directly. */
#if !defined ANDROID && !defined (__QNXNTO__)
    /* Lookup in EGL API. */
#if defined _EGL_APPENDIX
    proc.func = _Lookup(_veglLookup, procname, gcmDEF2STRING(_EGL_APPENDIX));
#else
    proc.func = _Lookup(_veglLookup, procname, gcvNULL);
#endif
    if (proc.func != gcvNULL)
    {
        goto Exit;
    }
#endif

#if !gcdSTATIC_LINK
    dispatch = _GetDispatch(thread, gcvNULL);
#endif

    switch (thread->api)
    {
    default:
#ifndef VIVANTE_NO_3D  /*Added for openVG core static link*/
        if ((thread->context == gcvNULL)
        ||  (thread->context->client != 2)
        )
        {
            /* OpenGL ES 1.1 API. */
#if gcdSTATIC_LINK
            dispatch = &GLES_CM_DISPATCH_TABLE;
#endif
#if defined _GL_11_APPENDIX
            appendix = gcmDEF2STRING(_GL_11_APPENDIX);
#else
            appendix = gcvNULL;
#endif
        }
        else
        {
            /* OpenGL ES 2.0 API. */
#if gcdSTATIC_LINK
            dispatch = &GLESv2_DISPATCH_TABLE;
#endif
#if defined _GL_2_APPENDIX
            appendix = gcmDEF2STRING(_GL_2_APPENDIX);
#else
            appendix = gcvNULL;
#endif
        }
#else
        appendix = gcvNULL;
#endif /* VIVANTE_NO_3D */
        break;

    case EGL_OPENVG_API:
        /* OpenVG API. */
#if !defined(VIVANTE_NO_VG)
#if gcdSTATIC_LINK
        dispatch = &OpenVG_DISPATCH_TABLE;
#endif
#if defined _VG_APPENDIX
        appendix = gcmDEF2STRING(_VG_APPENDIX);
#else
        appendix = gcvNULL;
#endif
        break;
#else
        /* VG driver is not available. */
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gcdZONE_EGL_API,
                      "%s(%d): %s",
                      __FUNCTION__, __LINE__,
                      "VG driver is not available.");

        gcmFOOTER_ARG("0x%x", gcvNULL);
        return gcvNULL;
#endif
    }

    /* In Android, we want to go through the Android' redirection libraries,
       to get the function pointers, instead of looking them up directly. */
#if !defined ANDROID && !defined (__QNXNTO__)
    /* Lookup the name with the appendix. */
    if ((dispatch != gcvNULL)
    &&  (dispatch->lookup != gcvNULL)
    )
    {
        proc.func = _Lookup(dispatch->lookup, procname, appendix);
    }

    if (proc.func != gcvNULL)
    {
        goto Exit;
    }
#endif

#if !gcdSTATIC_LINK
    gcmVERIFY_OK(gcoOS_StrLen(procname, &nameLen));

#if defined _EGL_APPENDIX
    len = nameLen + gcmSIZEOF(gcmDEF2STRING(_EGL_APPENDIX)) + 1;

    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   len,
                                   (gctPOINTER *) &name)))
    {
        gcmFOOTER_ARG("0x%x", gcvNULL);
        return gcvNULL;
    }

    gcmVERIFY_OK(gcoOS_StrCopySafe(name, len, procname));
    gcmVERIFY_OK(gcoOS_StrCatSafe(name, len, gcmDEF2STRING(_EGL_APPENDIX)));
#else
    name = gcvNULL;
#endif

    /* Try loading from libEGL. */
    library = veglGetModule(gcvNULL, EGL_TRUE, gcvNULL, gcvNULL);

    if (library != gcvNULL)
    {
        if (gcmIS_SUCCESS(gcoOS_GetProcAddress(gcvNULL,
                                               library,
                                               procname,
                                               &proc.ptr)))
        {
            goto Done;
        }

        if ((name != gcvNULL)
        &&  gcmIS_SUCCESS(gcoOS_GetProcAddress(gcvNULL,
                                               library,
                                               name,
                                               &proc.ptr))
        )
        {
            goto Done;
        }
    }

    if (name != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, name));
    }

    switch (thread->api)
    {
    default:
        if ((thread->context == gcvNULL)
        ||  (thread->context->client != 2)
        )
        {
#if defined _GL_11_APPENDIX
            appendix    = gcmDEF2STRING(_GL_11_APPENDIX);
            appendixLen = gcmSIZEOF(gcmDEF2STRING(_GL_11_APPENDIX));
#else
            appendix = gcvNULL;
#endif
        }
        else
        {
#if defined _GL_2_APPENDIX
            appendix    = gcmDEF2STRING(_GL_2_APPENDIX);
            appendixLen = gcmSIZEOF(gcmDEF2STRING(_GL_2_APPENDIX));
#else
            appendix = gcvNULL;
#endif
        }
        break;

    case EGL_OPENVG_API:
#if defined _VG_APPENDIX
        appendix    = gcmDEF2STRING(_VG_APPENDIX);
        appendixLen = gcmSIZEOF(gcmDEF2STRING(_VG_APPENDIX));
#else
        appendix = gcvNULL;
#endif
        break;
    }

    if (appendix == gcvNULL)
    {
        name = gcvNULL;
    }
    else
    {
        len = nameLen + appendixLen + 1;
        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                       len,
                                       &pointer)))
        {
            gcmFOOTER_ARG("0x%x", gcvNULL);
            return gcvNULL;
        }

        name = pointer;

        gcmVERIFY_OK(gcoOS_StrCopySafe(name, len, procname));
        gcmVERIFY_OK(gcoOS_StrCatSafe(name, len, appendix));
    }

    /* Try loading from API library. */
    for (i = 0; i < 2; i++)
    {
        /* Sending index second time, makes the code switch to next lib. */
        library = veglGetModule(gcvNULL, EGL_FALSE, gcvNULL, &index);

        if (library != gcvNULL)
        {
            if (gcmIS_SUCCESS(gcoOS_GetProcAddress(gcvNULL,
                                                   library,
                                                   procname,
                                                   &proc.ptr)))
            {
                goto Done;
            }

            if ((name != gcvNULL)
            &&  gcmIS_SUCCESS(gcoOS_GetProcAddress(gcvNULL,
                                                   library,
                                                   name,
                                                   &proc.ptr))
            )
            {
                goto Done;
            }
        }
    }

    proc.func = gcvNULL;

Done:
    if (name != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, name));
    }
#endif /* gcdSTATIC_LINK */

#if !defined ANDROID && !defined (__QNXNTO__)
Exit:
#endif
    gcmDUMP_API("${EGL eglGetProcAddress (0x%08X) := 0x%08X",
                procname, proc.func);
    gcmDUMP_API_DATA(procname, 0);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("0x%x", proc.func);
    return proc.func;
}
