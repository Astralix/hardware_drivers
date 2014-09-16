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





#include "gc_glff_precomp.h"

#ifdef gcdES11_CORE_WITH_EGL

static gctHANDLE eglModule;
static const char * _eglLib = "libEGL.dll";

typedef EGLBoolean (* peglGETCONFIGATTRIB)(
    EGLDisplay dpy,
    EGLConfig config,
    EGLint attribute,
    EGLint * value);

typedef const char * (* peglQUERYSTRING)(
        EGLDisplay dpy,
        EGLint name);

typedef void (EGLAPIENTRYP EGL_PROC)(void);

typedef EGL_PROC (* peglGETPROCADDRESS)(
        const char *procname);

typedef EGLBoolean (* peglSWAPBUFFERS)(
        EGLDisplay dpy,
        EGLSurface surface);

typedef EGLBoolean (* peglDESTROYSURFACE)(
        EGLDisplay dpy,
        EGLSurface surface);

typedef EGLBoolean (* peglTERMINATE)(
        EGLDisplay dpy);

typedef EGLDisplay (* peglGETDISPLAY)(
        EGLNativeDisplayType display_id);

typedef EGLBoolean (* peglINITIALIZE)(
        EGLDisplay dpy,
        EGLint *major,
        EGLint *minor);

typedef EGLBoolean (* peglGETCONFIGS)(
        EGLDisplay dpy,
        EGLConfig *configs,
        EGLint config_size,
        EGLint *num_config);

typedef EGLBoolean (* peglCHOOSECONFIG)(
        EGLDisplay dpy,
        const EGLint *attrib_list,
        EGLConfig *configs,
        EGLint config_size,
        EGLint *num_config);

typedef EGLSurface (* peglCREATEWINDOWSURFACE)(
        EGLDisplay dpy,
        EGLConfig config,
        EGLNativeWindowType win,
        const EGLint *attrib_list);

typedef EGLint (* peglGETERROR)(
        void);

typedef EGLContext (* peglCREATECONTEXT)(
        EGLDisplay dpy,
        EGLConfig config,
        EGLContext share_context,
        const EGLint *attrib_list);

typedef EGLBoolean (* peglDESTROYCONTEXT)(
        EGLDisplay dpy,
        EGLContext ctx);

typedef EGLBoolean (* peglMAKECURRENT)(
        EGLDisplay dpy,
        EGLSurface draw,
        EGLSurface read,
        EGLContext ctx);

typedef EGLSurface (* peglCREATEPBUFFERSURFACE)(
        EGLDisplay dpy,
        EGLConfig config,
        const EGLint *attrib_list);

typedef EGLSurface (* peglCREATEPIXMAPSURFACE)(
        EGLDisplay dpy,
        EGLConfig config,
        EGLNativePixmapType pixmap,
        const EGLint *attrib_list);

typedef EGLBoolean (* peglQUERYSURFACE)(
        EGLDisplay dpy,
        EGLSurface surface,
        EGLint attribute,
        EGLint *value);

typedef EGLBoolean (* peglBINDAPI)(
        EGLenum api);

typedef EGLenum (* peglQUERYAPI)(
        void);

typedef EGLBoolean (* peglWAITCLIENT)(
        void);

typedef EGLBoolean (* peglRELEASETHREAD)(
        void);

typedef EGLSurface (* peglCREATEPBUFFERFROMCLIENTBUFFER)(
        EGLDisplay dpy,
        EGLenum buftype,
        EGLClientBuffer buffer,
        EGLConfig config,
        const EGLint *attrib_list);

typedef EGLBoolean (* peglSURFACEATTRIB)(
        EGLDisplay dpy,
        EGLSurface surface,
        EGLint attribute,
        EGLint value);

typedef EGLBoolean (* peglBINDTEXIMAGE)(
        EGLDisplay dpy,
        EGLSurface surface,
        EGLint buffer);

typedef EGLBoolean (* peglRELEASETEXIMAGE)(
        EGLDisplay dpy,
        EGLSurface surface,
        EGLint buffer);

typedef EGLBoolean (* peglSWAPINTERVAL)(
        EGLDisplay dpy,
        EGLint interval);

typedef EGLContext (* peglGETCURRENTCONTEXT)(
        void);

typedef EGLSurface (* peglGETCURRENTSURFACE)(
        EGLint readdraw);

typedef EGLDisplay (* peglGETCURRENTDISPLAY)(
        void);

typedef EGLBoolean (* peglQUERYCONTEXT)(
        EGLDisplay dpy,
        EGLContext ctx,
        EGLint attribute,
        EGLint *value);

typedef EGLBoolean (* peglWAITGL)(
        void);

typedef EGLBoolean (* peglWAITNATIVE)(
        void);

typedef EGLBoolean (* peglCOPYBUFFERS)(
        EGLDisplay dpy,
        EGLSurface surface,
        EGLNativePixmapType target);


static gctPOINTER*
_GetProc(
    IN gctCONST_STRING Name
    )
{
    gctPOINTER pointer = gcvNULL;

    if (eglModule == gcvNULL)
    {
        if(gcoOS_LoadLibrary(gcvNULL, _eglLib, &eglModule) != gcvSTATUS_OK)
        {
            return gcvNULL;
        }
    }

    if (gcmIS_ERROR(
            gcoOS_GetProcAddress(gcvNULL,
                                 eglModule,
                                 Name,
                                 &pointer)))
    {
        return gcvNULL;
    }

    return pointer;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigAttrib(
    EGLDisplay dpy,
    EGLConfig config,
    EGLint attribute,
    EGLint * value
    )
{
    static peglGETCONFIGATTRIB* peglGetConfigAttrib;

    gcmTRACE(0, "GLES -> eglGetConfigAttrib");

    if (peglGetConfigAttrib == gcvNULL)
    {
        peglGetConfigAttrib = (peglGETCONFIGATTRIB*)_GetProc("eglGetConfigAttrib");
        if(peglGetConfigAttrib == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglGetConfigAttrib)(dpy, config, attribute, value);
}

EGLAPI const char * EGLAPIENTRY
eglQueryString(
    EGLDisplay dpy,
    EGLint name
    )
{
    static peglQUERYSTRING *peglQueryString;

    gcmTRACE(0, "GLES -> eglQueryString");

    if (peglQueryString == gcvNULL)
    {
        peglQueryString = (peglQUERYSTRING*)_GetProc("eglQueryString");
        if(peglQueryString == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglQueryString)(dpy, name);
}


EGLAPI EGL_PROC EGLAPIENTRY
eglGetProcAddress(
    const char *procname
    )
{
    static peglGETPROCADDRESS *peglGetProcAddress;

    gcmTRACE(0, "GLES -> eglGetProcAddress");

    if (peglGetProcAddress == gcvNULL)
    {
        peglGetProcAddress = (peglGETPROCADDRESS*)_GetProc("eglGetProcAddress");
        if(peglGetProcAddress == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglGetProcAddress)(procname);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapBuffers(
    EGLDisplay dpy,
    EGLSurface surface
    )
{
    static peglSWAPBUFFERS *peglSwapBuffers;

    gcmTRACE(0, "GLES -> eglSwapBuffers");

    if (peglSwapBuffers == gcvNULL)
    {
        peglSwapBuffers = (peglSWAPBUFFERS*)_GetProc("eglSwapBuffers");
        if(peglSwapBuffers == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglSwapBuffers)(dpy, surface);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroySurface(
    EGLDisplay dpy,
    EGLSurface surface
    )
{
    static peglDESTROYSURFACE *peglDestroySurface;

    gcmTRACE(0, "GLES -> eglDestroySurface");

    if (peglDestroySurface == gcvNULL)
    {
        peglDestroySurface = (peglDESTROYSURFACE*)_GetProc("eglDestroySurface");
        if(peglDestroySurface == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglDestroySurface)(dpy, surface);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglTerminate(
    EGLDisplay dpy
    )
{
    static peglTERMINATE *peglTerminate;

    gcmTRACE(0, "GLES -> eglTerminate");

    if (peglTerminate == gcvNULL)
    {
        peglTerminate = (peglTERMINATE*)_GetProc("eglTerminate");
        if(peglTerminate == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglTerminate)(dpy);
}

EGLAPI EGLDisplay EGLAPIENTRY
eglGetDisplay(
    EGLNativeDisplayType display_id
    )
{
    static peglGETDISPLAY *peglGetDisplay;

    gcmTRACE(0, "GLES -> eglGetDisplay");

    if (peglGetDisplay == gcvNULL)
    {
        peglGetDisplay = (peglGETDISPLAY*)_GetProc("eglGetDisplay");
        if(peglGetDisplay == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglGetDisplay)(display_id);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglInitialize(
    EGLDisplay dpy,
    EGLint *major,
    EGLint *minor
    )
{
    static peglINITIALIZE *peglInitialize;

    gcmTRACE(0, "GLES -> eglInitialize");

    if (peglInitialize == gcvNULL)
    {
        peglInitialize = (peglINITIALIZE*)_GetProc("eglInitialize");
        if(peglInitialize == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglInitialize)(dpy, major, minor);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigs(
    EGLDisplay dpy,
    EGLConfig *configs,
    EGLint config_size,
    EGLint *num_config
    )
{
    static peglGETCONFIGS *peglGetConfigs;

    gcmTRACE(0, "GLES -> eglGetConfigs");

    if (peglGetConfigs == gcvNULL)
    {
        peglGetConfigs = (peglGETCONFIGS*)_GetProc("eglGetConfigs");
        if(peglGetConfigs == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglGetConfigs)(dpy, configs, config_size, num_config);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglChooseConfig(
    EGLDisplay dpy,
    const EGLint *attrib_list,
    EGLConfig *configs,
    EGLint config_size,
    EGLint *num_config
    )
{
    static peglCHOOSECONFIG *peglChooseConfig;

    gcmTRACE(0, "GLES -> eglChooseConfig");

    if (peglChooseConfig == gcvNULL)
    {
        peglChooseConfig = (peglCHOOSECONFIG*)_GetProc("eglChooseConfig");
        if(peglChooseConfig == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglChooseConfig)(dpy, attrib_list, configs, config_size, num_config);
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreateWindowSurface(
    EGLDisplay dpy,
    EGLConfig config,
    EGLNativeWindowType win,
    const EGLint *attrib_list
    )
{
    static peglCREATEWINDOWSURFACE *peglCreateWindowSurface;

    gcmTRACE(0, "GLES -> eglCreateWindowSurface");

    if (peglCreateWindowSurface == gcvNULL)
    {
        peglCreateWindowSurface = (peglCREATEWINDOWSURFACE*)_GetProc("eglCreateWindowSurface");
        if(peglCreateWindowSurface == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglCreateWindowSurface)(dpy, config, win, attrib_list);
}

EGLAPI EGLint EGLAPIENTRY
eglGetError(
    void
    )
{
    static peglGETERROR *peglGetError;

    gcmTRACE(0, "GLES -> eglGetError");

    if (peglGetError == gcvNULL)
    {
        peglGetError = (peglGETERROR*)_GetProc("eglGetError");
        if(peglGetError == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglGetError)();
}

EGLAPI EGLContext EGLAPIENTRY
eglCreateContext(
    EGLDisplay dpy,
    EGLConfig config,
    EGLContext share_context,
    const EGLint *attrib_list
    )
{
    static peglCREATECONTEXT *peglCreateContext;

    gcmTRACE(0, "GLES -> eglCreateContext");

    if (peglCreateContext == gcvNULL)
    {
        peglCreateContext = (peglCREATECONTEXT*)_GetProc("eglCreateContext");
        if(peglCreateContext == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglCreateContext)(dpy, config, share_context, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglDestroyContext(
    EGLDisplay dpy,
    EGLContext ctx
    )
{
    static peglDESTROYCONTEXT *peglDestroyContext;

    gcmTRACE(0, "GLES -> eglDestroyContext");

    if (peglDestroyContext == gcvNULL)
    {
        peglDestroyContext = (peglDESTROYCONTEXT*)_GetProc("eglDestroyContext");
        if(peglDestroyContext == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglDestroyContext)(dpy, ctx);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglMakeCurrent(
    EGLDisplay dpy,
    EGLSurface draw,
    EGLSurface read,
    EGLContext ctx
    )
{
    static peglMAKECURRENT *peglMakeCurrent;

    gcmTRACE(0, "GLES -> eglMakeCurrent");

    if (peglMakeCurrent == gcvNULL)
    {
        peglMakeCurrent = (peglMAKECURRENT*)_GetProc("eglMakeCurrent");
        if(peglMakeCurrent == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglMakeCurrent)(dpy, draw, read, ctx);
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferSurface(
    EGLDisplay dpy,
    EGLConfig config,
    const EGLint *attrib_list
    )
{
    static peglCREATEPBUFFERSURFACE *peglCreatePbufferSurface;

    gcmTRACE(0, "GLES -> eglCreatePbufferSurface");

    if (peglCreatePbufferSurface == gcvNULL)
    {
        peglCreatePbufferSurface = (peglCREATEPBUFFERSURFACE*)_GetProc("eglCreatePbufferSurface");
        if(peglCreatePbufferSurface == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglCreatePbufferSurface)(dpy, config, attrib_list);
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePixmapSurface(
    EGLDisplay dpy,
    EGLConfig config,
    EGLNativePixmapType pixmap,
    const EGLint *attrib_list
    )
{
    static peglCREATEPIXMAPSURFACE *peglCreatePixmapSurface;

    gcmTRACE(0, "GLES -> eglCreatePixmapSurface");

    if (peglCreatePixmapSurface == gcvNULL)
    {
        peglCreatePixmapSurface = (peglCREATEPIXMAPSURFACE*)_GetProc("eglCreatePixmapSurface");
        if(peglCreatePixmapSurface == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglCreatePixmapSurface)(dpy, config, pixmap, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQuerySurface(
    EGLDisplay dpy,
    EGLSurface surface,
    EGLint attribute,
    EGLint *value
    )
{
    static peglQUERYSURFACE *peglQuerySurface;

    gcmTRACE(0, "GLES -> eglQuerySurface");

    if (peglQuerySurface == gcvNULL)
    {
        peglQuerySurface = (peglQUERYSURFACE*)_GetProc("eglQuerySurface");
        if(peglQuerySurface == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglQuerySurface)(dpy, surface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindAPI(
    EGLenum api
    )
{
    static peglBINDAPI *peglBindAPI;

    gcmTRACE(0, "GLES -> eglBindAPI");

    if (peglBindAPI == gcvNULL)
    {
        peglBindAPI = (peglBINDAPI*)_GetProc("eglBindAPI");
        if(peglBindAPI == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglBindAPI)(api);
}

EGLAPI EGLenum EGLAPIENTRY
eglQueryAPI(
    void
    )
{
    static peglQUERYAPI *peglQueryAPI;

    gcmTRACE(0, "GLES -> eglQueryAPI");

    if (peglQueryAPI == gcvNULL)
    {
        peglQueryAPI = (peglQUERYAPI*)_GetProc("eglQueryAPI");
        if(peglQueryAPI == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglQueryAPI)();
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitClient(
    void
    )
{
    static peglWAITCLIENT *peglWaitClient;

    gcmTRACE(0, "GLES -> eglWaitClient");

    if (peglWaitClient == gcvNULL)
    {
        peglWaitClient = (peglWAITCLIENT*)_GetProc("eglWaitClient");
        if(peglWaitClient == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglWaitClient)();
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseThread(
    void
    )
{
    static peglRELEASETHREAD *peglReleaseThread;

    gcmTRACE(0, "GLES -> eglReleaseThread");

    if (peglReleaseThread == gcvNULL)
    {
        peglReleaseThread = (peglRELEASETHREAD*)_GetProc("eglReleaseThread");
        if(peglReleaseThread == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglReleaseThread)();
}

EGLAPI EGLSurface EGLAPIENTRY
eglCreatePbufferFromClientBuffer(
    EGLDisplay dpy,
    EGLenum buftype,
    EGLClientBuffer buffer,
    EGLConfig config,
    const EGLint *attrib_list
    )
{
    static peglCREATEPBUFFERFROMCLIENTBUFFER *peglCreatePbufferFromClientBuffer;

    gcmTRACE(0, "GLES -> eglCreatePbufferFromClientBuffer");

    if (peglCreatePbufferFromClientBuffer == gcvNULL)
    {
        peglCreatePbufferFromClientBuffer =
            (peglCREATEPBUFFERFROMCLIENTBUFFER*)_GetProc("eglCreatePbufferFromClientBuffer");
        if(peglCreatePbufferFromClientBuffer == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglCreatePbufferFromClientBuffer)(dpy, buftype, buffer, config, attrib_list);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSurfaceAttrib(
    EGLDisplay dpy,
    EGLSurface surface,
    EGLint attribute,
    EGLint value
    )
{
    static peglSURFACEATTRIB *peglSurfaceAttrib;

    gcmTRACE(0, "GLES -> eglSurfaceAttrib");

    if (peglSurfaceAttrib == gcvNULL)
    {
        peglSurfaceAttrib = (peglSURFACEATTRIB*)_GetProc("eglSurfaceAttrib");
        if(peglSurfaceAttrib == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglSurfaceAttrib)(dpy, surface, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglBindTexImage(
    EGLDisplay dpy,
    EGLSurface surface,
    EGLint buffer
    )
{
    static peglBINDTEXIMAGE *peglBindTexImage;

    gcmTRACE(0, "GLES -> eglBindTexImage");

    if (peglBindTexImage == gcvNULL)
    {
        peglBindTexImage = (peglBINDTEXIMAGE*)_GetProc("eglBindTexImage");
        if(peglBindTexImage == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglBindTexImage)(dpy, surface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglReleaseTexImage(
    EGLDisplay dpy,
    EGLSurface surface,
    EGLint buffer
    )
{
    static peglRELEASETEXIMAGE *peglReleaseTexImage;

    gcmTRACE(0, "GLES -> eglReleaseTexImage");

    if (peglReleaseTexImage == gcvNULL)
    {
        peglReleaseTexImage = (peglRELEASETEXIMAGE*)_GetProc("eglReleaseTexImage");
        if(peglReleaseTexImage == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglReleaseTexImage)(dpy, surface, buffer);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglSwapInterval(
    EGLDisplay dpy,
    EGLint interval
    )
{
    static peglSWAPINTERVAL *peglSwapInterval;

    gcmTRACE(0, "GLES -> eglSwapInterval");

    if (peglSwapInterval == gcvNULL)
    {
        peglSwapInterval = (peglSWAPINTERVAL*)_GetProc("eglSwapInterval");
        if(peglSwapInterval == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglSwapInterval)(dpy, interval);
}

EGLAPI EGLContext EGLAPIENTRY
eglGetCurrentContext(
    void
    )
{
    static peglGETCURRENTCONTEXT *peglGetCurrentContext;

    gcmTRACE(0, "GLES -> eglGetCurrentContext");

    if (peglGetCurrentContext == gcvNULL)
    {
        peglGetCurrentContext = (peglGETCURRENTCONTEXT*)_GetProc("eglGetCurrentContext");
        if(peglGetCurrentContext == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglGetCurrentContext)();
}

EGLAPI EGLSurface EGLAPIENTRY
eglGetCurrentSurface(
    EGLint readdraw
    )
{
    static peglGETCURRENTSURFACE *peglGetCurrentSurface;

    gcmTRACE(0, "GLES -> eglGetCurrentSurface");

    if (peglGetCurrentSurface == gcvNULL)
    {
        peglGetCurrentSurface = (peglGETCURRENTSURFACE*)_GetProc("eglGetCurrentSurface");
        if(peglGetCurrentSurface == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglGetCurrentSurface)(readdraw);
}

EGLAPI EGLDisplay EGLAPIENTRY
eglGetCurrentDisplay(
    void
    )
{
    static peglGETCURRENTDISPLAY *peglGetCurrentDisplay;

    gcmTRACE(0, "GLES -> eglGetCurrentDisplay");

    if (peglGetCurrentDisplay == gcvNULL)
    {
        peglGetCurrentDisplay = (peglGETCURRENTDISPLAY*)_GetProc("eglGetCurrentDisplay");
        if(peglGetCurrentDisplay == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglGetCurrentDisplay)();
}

EGLAPI EGLBoolean EGLAPIENTRY
eglQueryContext(
    EGLDisplay dpy,
    EGLContext ctx,
    EGLint attribute,
    EGLint *value
    )
{
    static peglQUERYCONTEXT *peglQueryContext;

    gcmTRACE(0, "GLES -> eglQueryContext");

    if (peglQueryContext == gcvNULL)
    {
        peglQueryContext = (peglQUERYCONTEXT*)_GetProc("eglQueryContext");
        if(peglQueryContext == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglQueryContext)(dpy, ctx, attribute, value);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitGL(
    void
    )
{
    static peglWAITGL *peglWaitGL;

    gcmTRACE(0, "GLES -> eglWaitGL");

    if (peglWaitGL == gcvNULL)
    {
        peglWaitGL = (peglWAITGL*)_GetProc("eglWaitGL");
        if(peglWaitGL == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglWaitGL)();
}

EGLAPI EGLBoolean EGLAPIENTRY
eglWaitNative(
    EGLint engine
    )
{
    static peglWAITNATIVE *peglWaitNative;

    gcmTRACE(0, "GLES -> eglWaitNative");

    if (peglWaitNative == gcvNULL)
    {
        peglWaitNative = (peglWAITNATIVE*)_GetProc("eglWaitNative");
        if(peglWaitNative == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglWaitNative)();
}

EGLAPI EGLBoolean EGLAPIENTRY
eglCopyBuffers(
    EGLDisplay dpy,
    EGLSurface surface,
    EGLNativePixmapType target
    )
{
    static peglCOPYBUFFERS *peglCopyBuffers;

    gcmTRACE(0, "GLES -> eglCopyBuffers");

    if (peglCopyBuffers == gcvNULL)
    {
        peglCopyBuffers = (peglCOPYBUFFERS*)_GetProc("eglCopyBuffers");
        if(peglCopyBuffers == gcvNULL)
        {
            return EGL_FALSE;
        }
    }

    return (*peglCopyBuffers)(dpy, surface, target);
}

#endif /* gcdES11_CORE_WITH_EGL */
