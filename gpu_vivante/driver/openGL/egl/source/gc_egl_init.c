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
#define NO_STENCIL_VG   1
#define _GC_OBJ_ZONE    gcdZONE_EGL_INIT


static struct EGL_CONFIG_COLOR eglConfigColor[] =
{
#ifndef ANDROID
    { 16, 4, 4, 4, 0, VEGL_444  },  /* X4R4G4B4 */
    { 16, 4, 4, 4, 4, VEGL_4444 },  /* A4R4G4B4 */
    { 16, 5, 5, 5, 0, VEGL_555  },  /* X1R5G5B5 */
    { 16, 5, 5, 5, 1, VEGL_5551 },  /* A1R5G5B5 */
#endif
    { 16, 5, 6, 5, 0, VEGL_565  },  /* R5G6B5   */
    { 32, 8, 8, 8, 0, VEGL_888  },  /* X8R8G8B8 */
    { 32, 8, 8, 8, 8, (VEGL_COLOR_FORMAT) (VEGL_8888 | VEGL_DEFAULT) }, /* A8R8G8B8 */
};

static struct EGL_CONFIG_DEPTH eglConfigDepth[] =
{
    {  0, 0 },  /*       */
    { 16, 0 },  /* D16   */
    { 24, 0 },  /* D24X8 */
    { 24, 8 },  /* D24S8 */
};

static const char extension[] =
    "EGL_KHR_reusable_sync"
    " "
    "EGL_KHR_fence_sync"
    " "
    "EGL_KHR_image_base"
    " "
    "EGL_KHR_image_pixmap"
    " "
    "EGL_KHR_image"
    " "
    "EGL_KHR_gl_texture_2D_image"
    " "
    "EGL_KHR_gl_texture_cubmap_image"
    " "
    "EGL_KHR_gl_renderbuffer_image"
    " "
    "EGL_KHR_lock_surface"
#if defined(ANDROID)
    " "
    "EGL_ANDROID_image_native_buffer"
    " "
    "EGL_ANDROID_blob_cache"
    " "
    "EGL_ANDROID_recordable"
#endif
    ;

#if gcdENABLE_TIMEOUT_DETECTION
static gceSTATUS
_SwapSignalTimeoutCallback(
    IN gcoOS Os,
    IN gctPOINTER Context,
    IN gctSIGNAL Signal,
    IN gctBOOL Reset
    )
{
    VEGLDisplay display;

    /* Cast display pointer. */
    display = (VEGLDisplay) Context;

    /* Ignore swap thread start signal. */
    if (Signal == display->startSignal)
    {
        return gcvSTATUS_NOT_SUPPORTED;
    }
    else
    {
        return gcvSTATUS_OK;
    }
}
#endif

static EGLBoolean
_ValidateMode(
    NativeDisplayType DeviceContext,
    VEGLConfigColor Color,
    VEGLConfigDepth Depth,
    EGLint * MaxSamples
    )
{
    /* Get thread data. */
    VEGLThreadData thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        return EGL_FALSE;
    }

    /* Get sample count from thread data. */
    *MaxSamples = (thread->chipModel == gcv500) ? 0 : thread->maxSamples;

    /* Success. */
    return EGL_TRUE;
}

static void
_FillIn(
    VEGLConfig Config,
    EGLint * Index,
    VEGLConfigColor Color,
    VEGLConfigDepth Depth,
    EGLint Samples
    )
{
    VEGLConfig config;
    VEGLThreadData thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        return;
    }

    /* Get a shortcut to the current configuration. */
    config = &Config[*Index];

    config->bufferSize  = Color->bufferSize;
    config->alphaSize   = Color->alphaSize;
    config->blueSize    = Color->blueSize;
    config->greenSize   = Color->greenSize;
    config->redSize     = Color->redSize;
    config->depthSize   = Depth->depthSize;
    config->stencilSize = Depth->stencilSize;

    config->configCaveat = EGL_NONE;

    config->configId      = 1 + *Index;
    config->defaultConfig = (Color->formatFlags & VEGL_DEFAULT) != 0;

    config->nativeRenderable
        =  ((Color->formatFlags & VEGL_565) != 0)
        || ((Color->formatFlags & VEGL_888) != 0);

    config->nativeVisualType = config->nativeRenderable
        ? (Color->redSize == 8) ? 32 : 16
        : EGL_NONE;

    config->samples       = Samples;
    config->sampleBuffers = (Samples > 0) ? 1 : 0;

    config->surfaceType = EGL_WINDOW_BIT
                        | EGL_PBUFFER_BIT
                        | EGL_PIXMAP_BIT
                        | EGL_LOCK_SURFACE_BIT_KHR
                        | EGL_OPTIMAL_FORMAT_BIT_KHR
                        | EGL_SWAP_BEHAVIOR_PRESERVED_BIT;

    config->bindToTetxureRGB  = (Color->alphaSize == 0);
    config->bindToTetxureRGBA = EGL_TRUE;

    config->luminanceSize = 0;
    config->alphaMaskSize = 0;

    config->colorBufferType = EGL_RGB_BUFFER;

    config->renderableType = EGL_OPENGL_ES_BIT
                           | EGL_OPENGL_ES2_BIT;

    config->conformant     = EGL_OPENGL_ES_BIT
                           | EGL_OPENGL_ES2_BIT;

    if (Samples == 16)
    {
        /* Remove VAA configuration for OpenGL ES 1.1. */
        config->renderableType &= ~EGL_OPENGL_ES_BIT;
        config->conformant     &= ~EGL_OPENGL_ES_BIT;
    }

    config->alphaMaskSize = 8;

    config->matchFormat = (config->greenSize == 6)
                        ? EGL_FORMAT_RGB_565_EXACT_KHR
                        :   ( (config->greenSize == 8)
                            ? EGL_FORMAT_RGBA_8888_EXACT_KHR
                            : EGL_DONT_CARE
                            );

    config->matchNativePixmap = EGL_NONE;

    config->recordableConfig = EGL_TRUE;

    /* Is hardware VG pipe present? */
    if (thread->openVGpipe)
    {
        if ((Samples == 0)
        &&  (config->stencilSize ==  0)
        &&  (config->depthSize   == 16)
        &&  (  ((Color->formatFlags & VEGL_4444)  == VEGL_4444)
            || ((Color->formatFlags & VEGL_5551)  == VEGL_5551)
            || ((Color->formatFlags & VEGL_565)   == VEGL_565)
            || ((Color->formatFlags & VEGL_8888)  == VEGL_8888)
            || ((Color->formatFlags & VEGL_888)   == VEGL_888)
            )
        )
        {
            if ((Color->formatFlags & VEGL_888)  != VEGL_888)
            {
                config->conformant      |= EGL_OPENVG_BIT;
            }

            config->renderableType  |= EGL_OPENVG_BIT;
#ifndef __QNXNTO__

            if (    ((Color->formatFlags & VEGL_565)   == VEGL_565)
                ||  ((Color->formatFlags & VEGL_8888)  == VEGL_8888))
#else
            if (1)
#endif
            {
                config->surfaceType     |= EGL_VG_ALPHA_FORMAT_PRE_BIT
                                        |  EGL_VG_COLORSPACE_LINEAR_BIT;
            }
        }
    }

    /* No, 2D/3D implementation only. */
    else
    {
        /* Determing the number of samples required for OpenVG. */

        /* TODO: Need to get rid of depth for OpenVG. */
#if NO_STENCIL_VG
        if ((Depth->stencilSize == 0 && Depth->depthSize == 16) &&
#else
        if ((Depth->stencilSize > 0)  &&
#endif
            (((Color->formatFlags & VEGL_565)  == VEGL_565) ||
            ((Color->formatFlags & VEGL_8888) == VEGL_8888) ||
            ((Color->formatFlags & VEGL_888) == VEGL_888)))
        {
            config->renderableType |= EGL_OPENVG_BIT;

            /* Why? 3D OpenVG does not support color format with alpha channel. */
            if (((Color->formatFlags & VEGL_ALPHA) == 0) && (Samples >= (thread->vaa ? 4 : thread->maxSamples)))
            {
                config->conformant  |= EGL_OPENVG_BIT;
                config->surfaceType |= EGL_VG_ALPHA_FORMAT_PRE_BIT
                                    |  EGL_VG_COLORSPACE_LINEAR_BIT;
            }
        }
    }

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "EGL CONFIGURATION:"
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "  config index=%d; config ID=%d; caviat=0x%X",
        *Index, config->configId, config->configCaveat
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "  surface type=0x%X; color buffer type=0x%X; renderable type=0x%X",
        config->surfaceType, config->colorBufferType, config->renderableType
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "  buffer size=%d",
        config->bufferSize
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "  RGBA sizes=%d, %d, %d, %d; depth=%d; stencil=%d",
        config->redSize, config->greenSize,
        config->blueSize, config->alphaSize,
        config->depthSize, config->stencilSize
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "  luminance size=%d; alpha mask size=%d",
        config->luminanceSize, config->alphaMaskSize
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "  native renderable=%d; native visual type=%d",
        config->nativeRenderable, config->nativeVisualType
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "  samples=%d; sample buffers=%d",
        config->samples, config->sampleBuffers
        );

    gcmTRACE_ZONE(
        gcvLEVEL_VERBOSE, gcdZONE_EGL_INIT,
        "  bind to tetxure RGB=%d; bind to tetxure RGBA=%d",
        config->bindToTetxureRGB, config->bindToTetxureRGBA
        );

    /* Advance to the next entry. */
    (*Index) ++;
}

EGLAPI EGLint EGLAPIENTRY
eglGetError(
    void
    )
{
    VEGLThreadData thread;
    EGLint error;

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

        error = EGL_BAD_ALLOC;
    }
    else
    {
        error = thread->error;
    }

    gcmDUMP_API("${EGL eglGetError := 0x%08X}", error);
    gcmFOOTER_ARG("0x%04x", error);
    return error;
}

EGLAPI EGLDisplay EGLAPIENTRY
eglGetDisplay(
    NativeDisplayType display_id
    )
{
    VEGLThreadData thread;
    VEGLDisplay display;
    gctBOOL releaseDpy = gcvFALSE;

    gcmHEADER_ARG("display_id=0x%x", display_id);

#if (gcdDUMP || gcdDUMP_API) && !defined(__APPLE__)
    {
        gceSTATUS status = gcvSTATUS_TRUE;

        /* Set this to a string that appears in 'cat /proc/<pid>/cmdline'. E.g. 'camera'.
           HAL will create dumps for all processes matching this key. */
        #define DUMP_KEY           "process"

#if gcdDUMP
        #define DUMP_FILE_PREFIX   "hal"
#else
        #define DUMP_FILE_PREFIX   "api"
#endif

#if defined(ANDROID)
        /* Some processes cannot write to the sdcard.
           Try apps' data dir, e.g. /data/data/com.android.launcher */
        #define DUMP_PATH   "/mnt/sdcard/"

        status = gcoOS_DetectProcessByName(DUMP_KEY);
#else
        #define DUMP_PATH   "./"
#endif

        if (status == gcvSTATUS_TRUE)
        {
            char dump_file[128];
            gctUINT offset = 0;

            /* Customize filename as needed. */
            gcmVERIFY_OK(gcoOS_PrintStrSafe(dump_file,
                         gcmSIZEOF(dump_file),
                         &offset,
                         "%s%s_dump_%d_%s.log",
                         DUMP_PATH,
                         DUMP_FILE_PREFIX,
                         gcoOS_GetCurrentProcessID(),
                         DUMP_KEY));

            gcoOS_SetDebugFile(dump_file);
        }
	}
#endif


    /* Add Necessary signal handlers */
    /* Add signal handler for SIGFPE when error no is zero. */
    gcoOS_AddSignalHandler(gcvHANDLE_SIGFPE_WHEN_SIGNAL_CODE_IS_0);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        gcmFOOTER_ARG("0x%x", EGL_NO_DISPLAY);
        return EGL_NO_DISPLAY;
    }

    if (display_id == EGL_DEFAULT_DISPLAY)
    {
        /* Get default device context for desktop window. */
        display_id = veglGetDefaultDisplay();
        releaseDpy = gcvTRUE;
    }
    else
    {
        if (!veglIsValidDisplay(display_id))
        {
            thread->error = EGL_BAD_PARAMETER;
            gcmFOOTER_ARG("0x%x", EGL_NO_DISPLAY);
            return EGL_NO_DISPLAY;
        }
    }

    /* Try finding the display_id inside the EGLDisplay stack. */
    for (display = (VEGLDisplay) gcoOS_GetPLSValue(gcePLS_VALUE_EGL_DISPLAY_INFO);
         display != gcvNULL;
         display = display->next)
    {
        if (display->hdc == display_id)
        {
            /* Release DC if necessary. */
            if (releaseDpy)
            {
                veglReleaseDefaultDisplay(display_id);
            }

            /* Got it! */
            break;
        }
    }

    if (display == gcvNULL)
    {
        gctPOINTER pointer = gcvNULL;

        /* Allocate memory for eglDisplay structure. */
        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                       sizeof(struct eglDisplay),
                                       &pointer)))
        {
            /* Allocation error. */
            thread->error = EGL_BAD_ALLOC;
            gcmFOOTER_ARG("0x%x", EGL_NO_DISPLAY);
            return EGL_NO_DISPLAY;
        }

        display = pointer;

        /* Initialize EGLDisplay. */
        display->signature     = EGL_DISPLAY_SIGNATURE;
        display->hdc           = display_id;
        display->releaseDpy    = releaseDpy;
        display->referenceDpy  = gcvNULL;
        display->configCount   = 0;
        display->config        = gcvNULL;
        display->surfaceStack  = gcvNULL;
        display->imageStack    = gcvNULL;
        display->imageRefStack = gcvNULL;
        display->contextStack  = gcvNULL;
        display->process       = gcoOS_GetCurrentProcessID();
        display->ownerThread   = gcvNULL;
        display->workerThread  = gcvNULL;
        display->startSignal   = gcvNULL;
        display->stopSignal    = gcvNULL;
        display->doneSignal    = gcvNULL;
        display->suspendMutex  = gcvNULL;
        display->blobCacheGet  = gcvNULL;
        display->blobCacheSet  = gcvNULL;
        display->localInfo     = gcvNULL;

        /* Push EGLDisplay onto stack. */
        display->next = (VEGLDisplay) gcoOS_GetPLSValue(gcePLS_VALUE_EGL_DISPLAY_INFO);
        gcoOS_SetPLSValue(gcePLS_VALUE_EGL_DISPLAY_INFO, (gctPOINTER) display);
    }

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglGetDisplay 0x%08X := 0x%08X}", display_id, display);
    gcmFOOTER_ARG("0x%x", display);
    return display;
}

static EGLBoolean
_SortAfter(
    struct eglConfig Config1,
    struct eglConfig Config2
    )
{
    EGLint bits1, bits2;

    /* Priority 1. */
    if (Config1.configCaveat < Config2.configCaveat)
    {
        return EGL_FALSE;
    }
    else if (Config1.configCaveat > Config2.configCaveat)
    {
        return EGL_TRUE;
    }

    /* Priority 2. */
    if (Config1.colorBufferType < Config2.colorBufferType)
    {
        return EGL_FALSE;
    }
    else if (Config1.colorBufferType > Config2.colorBufferType)
    {
        return EGL_TRUE;
    }

    /* Compute number of color bits based on attributes. */
    bits1 = bits2 = 0;
    if (Config1.colorBufferType == EGL_RGB_BUFFER)
    {
        bits1 += Config1.redSize + Config1.greenSize + Config1.blueSize;
        bits2 += Config2.redSize + Config2.greenSize + Config2.blueSize;
    }
    else
    {
        bits1 += Config1.luminanceSize;
        bits2 += Config2.luminanceSize;
    }

    if (bits1 == bits2)
    {
        if (Config1.alphaSize < Config2.alphaSize)
        {
            return EGL_FALSE;
        }
        else if (Config1.alphaSize > Config2.alphaSize)
        {
            return EGL_TRUE;
        }
    }

    bits1 += Config1.alphaSize;
    bits2 += Config2.alphaSize;


    /* Priority 3. */
    if (bits1 < bits2)
    {
        return EGL_TRUE;
    }
    else if (bits1 > bits2)
    {
        return EGL_FALSE;
    }

    /* Priority 4. */
    if (Config1.bufferSize < Config2.bufferSize)
    {
        return EGL_FALSE;
    }
    else if (Config1.bufferSize > Config2.bufferSize)
    {
        return EGL_TRUE;
    }

    /* Priority 5. */
    if (Config1.sampleBuffers < Config2.sampleBuffers)
    {
        return EGL_FALSE;
    }
    else if (Config1.sampleBuffers > Config2.sampleBuffers)
    {
        return EGL_TRUE;
    }

    /* Priority 6. */
    if (Config1.samples < Config2.samples)
    {
        return EGL_FALSE;
    }
    else if (Config1.samples > Config2.samples)
    {
        return EGL_TRUE;
    }

    /* Priority 7. */
    if (Config1.depthSize < Config2.depthSize)
    {
        return EGL_FALSE;
    }
    else if (Config1.depthSize > Config2.depthSize)
    {
        return EGL_TRUE;
    }

    /* Priority 8. */
    if (Config1.stencilSize < Config2.stencilSize)
    {
        return EGL_FALSE;
    }
    else if (Config1.stencilSize > Config2.stencilSize)
    {
        return EGL_TRUE;
    }

    /* Priority 9. */
    if (Config1.alphaMaskSize < Config2.alphaMaskSize)
    {
        return EGL_FALSE;
    }
    else if (Config1.alphaMaskSize > Config2.alphaMaskSize)
    {
        return EGL_TRUE;
    }

    /* Compute native visual type sorting. */
    bits1 = (Config1.nativeVisualType == EGL_NONE)
        ? 0
        : Config1.nativeVisualType;
    bits2 = (Config2.nativeVisualType == EGL_NONE)
        ? 0
        : Config2.nativeVisualType;

    /* Priority 10. */
    if (bits1 < bits2)
    {
        return EGL_FALSE;
    }
    else if (bits1 > bits2)
    {
        return EGL_TRUE;
    }

    /* Priority 11. */
    if (Config1.configId < Config2.configId)
    {
        return EGL_FALSE;
    }
    else if (Config1.configId > Config2.configId)
    {
        return EGL_TRUE;
    }

    /* Nothing to sort. */
    return EGL_FALSE;
}

static void
_Sort(
    VEGLConfig Config,
    EGLint ConfigCount
    )
{
    EGLBoolean swapped;
    EGLint i;

    do
    {
        /* Assume no sorting has happened. */
        swapped = EGL_FALSE;

        /* Loop through all configurations. */
        for (i = 0; i < ConfigCount - 1; i++)
        {
            /* Do we need to swap the current and next configuration? */
            if (_SortAfter(Config[i], Config[i+1]))
            {
                /* Swap configurations. */
                struct eglConfig temp = Config[i];
                Config[i] = Config[i+1];
                Config[i+1] = temp;

                /* We need another pass. */
                swapped = EGL_TRUE;
            }
        }
    }
    while (swapped);
}

EGLBoolean
veglReferenceDisplay(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display
    )
{
    gctUINT color, depth;
    gctINT index;
    EGLint i;
    gceSTATUS status;
    gctINT32 reference;

    if (Display->referenceDpy == gcvNULL)
    {
        /* Create a new reference counter. */
        if (gcmIS_ERROR(gcoOS_AtomConstruct(gcvNULL, &Display->referenceDpy)))
        {
            /* Out of memory. */
            Thread->error = EGL_BAD_ALLOC;
            return EGL_FALSE;
        }
    }

    /* Increment the reference counter. */
    if (gcmIS_ERROR(gcoOS_AtomIncrement(gcvNULL,
                                        Display->referenceDpy,
                                        &reference)))
    {
        return EGL_FALSE;
    }

    /* Test for first reference. */
    if (reference > 0)
    {
        /* Already referenced. */
        return EGL_TRUE;
    }

    do
    {
        gctPOINTER pointer = gcvNULL;

        /* Count the number of valid configurations. */
        gcmASSERT(Display->configCount == 0);
        for (color = 0; color < gcmCOUNTOF(eglConfigColor); color++)
        {
            for (depth = 0; depth < gcmCOUNTOF(eglConfigDepth); depth++)
            {
                EGLBoolean validMode;
                EGLint maxMultiSample = 0;

                validMode = _ValidateMode(
                    Display->hdc,
                    &eglConfigColor[color],
                    &eglConfigDepth[depth],
                    &maxMultiSample
                    );

                if (!validMode)
                {
                    continue;
                }

#if defined(ANDROID)
                /* Duplicate NOAA Z-less config to return BGRA visual. */
                if (((eglConfigColor[color].formatFlags & VEGL_8888) == VEGL_8888)
                &&  (eglConfigDepth[depth].depthSize  == 0)
                )
                {
                    Display->configCount ++;
                }
#endif

                switch (maxMultiSample)
                {
                case 4:
                    Display->configCount ++;

                    /* Fall through. */
                case 2:
                    Display->configCount ++;

                    /* Fall through. */
                default:
                    Display->configCount ++;
                }

                /* Add VAA capability for XRGB8888 modes. */
                if (Thread->vaa
                &&  (eglConfigColor[color].bufferSize == 32)
                )
                {
                    Display->configCount ++;
                }
            }
        }

        gcmASSERT(Display->config == gcvNULL);
        /* Allocate configuration buffer. */
        gcmERR_BREAK(gcoOS_Allocate(
            gcvNULL,
            (gctSIZE_T) Display->configCount * sizeof(struct eglConfig),
            &pointer
            ));

        Display->config = pointer;

        /* Start at beginning of configuration buffer. */
        index = 0;

        /* Fill in the configuration table. */
        for (color = 0; color < gcmCOUNTOF(eglConfigColor); color++)
        {
            for (depth = 0; depth < gcmCOUNTOF(eglConfigDepth); depth++)
            {
                EGLBoolean validMode;
                EGLint maxMultiSample = 0;

                validMode = _ValidateMode(
                    Display->hdc,
                    &eglConfigColor[color],
                    &eglConfigDepth[depth],
                    &maxMultiSample
                    );

                if (!validMode)
                {
                    continue;
                }

                _FillIn(
                    Display->config,
                    &index,
                    &eglConfigColor[color],
                    &eglConfigDepth[depth],
                    0);

#if defined(ANDROID)
                /* Duplicate NOAA Z-less config to return BGRA visual. */
                if (((eglConfigColor[color].formatFlags & VEGL_8888) == VEGL_8888)
                &&  (eglConfigDepth[depth].depthSize  == 0)
                )
                {
                    Display->config[index] = Display->config[index - 1];
                    Display->config[index].swizzleRB = EGL_TRUE;

                    index++;
                }
#endif

                if (maxMultiSample >= 2)
                {
                    _FillIn(
                        Display->config,
                        &index,
                        &eglConfigColor[color],
                        &eglConfigDepth[depth],
                        2);
                }

                if (maxMultiSample >= 4)
                {
                    _FillIn(
                        Display->config,
                        &index,
                        &eglConfigColor[color],
                        &eglConfigDepth[depth],
                        4);
                }

                if (Thread->vaa
                &&  (eglConfigColor[color].bufferSize == 32)
                )
                {
                    _FillIn(
                        Display->config,
                        &index,
                        &eglConfigColor[color],
                        &eglConfigDepth[depth],
                        16);
                }
            }
        }

        if ((Display->configCount > 1) && (Display->config != gcvNULL))
        {
            /* Sort the configurations. */
            _Sort(Display->config, Display->configCount);
            for (i = 0; i < Display->configCount; i++)
            {
                (Display->config + i)->configId = i + 1;
            }
        }

        /* Create worker thread. */
        do
        {
            gcmASSERT(Display->startSignal == gcvNULL);
            /* Create thread start signal. */
            gcmERR_BREAK(gcoOS_CreateSignal(
                gcvNULL,
                gcvFALSE,
                &Display->startSignal
                ));

            gcmTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_SIGNAL,
                "%s(%d): start signal created 0x%08X",
                __FUNCTION__, __LINE__,
                Display->startSignal
                );

            gcmASSERT(Display->stopSignal == gcvNULL);
            /* Create thread stop signal. */
            gcmERR_BREAK(gcoOS_CreateSignal(
                gcvNULL,
                gcvTRUE,
                &Display->stopSignal
                ));

            gcmTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_SIGNAL,
                "%s(%d): stop signal created 0x%08X",
                __FUNCTION__, __LINE__,
                Display->stopSignal
                );

            gcmASSERT(Display->doneSignal == gcvNULL);
            /* Create thread done signal. */
            gcmERR_BREAK(gcoOS_CreateSignal(
                gcvNULL,
                gcvTRUE,
                &Display->doneSignal
                ));

            gcmTRACE_ZONE(
                gcvLEVEL_INFO, gcvZONE_SIGNAL,
                "%s(%d): done signal created 0x%08X",
                __FUNCTION__, __LINE__,
                Display->doneSignal
                );

            gcmASSERT(Display->suspendMutex == gcvNULL);
            /* Create thread lock signal. */
            gcmERR_BREAK(gcoOS_CreateMutex(
                gcvNULL,
                &Display->suspendMutex
                ));

            /* No workers yet. */
            Display->workerSentinel.draw = gcvNULL;
            Display->workerSentinel.prev   = &Display->workerSentinel;
            Display->workerSentinel.next   = &Display->workerSentinel;

            /* Set complete signal. */
            gcmERR_BREAK(gcoOS_Signal(
                gcvNULL,
                Display->doneSignal,
                gcvTRUE
                ));

            gcmASSERT(Display->workerThread == gcvNULL);
            /* Start the thread. */
            gcmERR_BREAK(gcoOS_CreateThread(
                gcvNULL, veglSwapWorker, Display, &Display->workerThread
                ));

#if gcdENABLE_TIMEOUT_DETECTION
            /* Setup the swap thread signal timeout handler. */
            gcmVERIFY_OK(gcoOS_InstallTimeoutCallback(
                _SwapSignalTimeoutCallback, Display
                ));
#endif
        }
        while (gcvFALSE);

        if (!veglInitLocalDisplayInfo(Display))
        {
            break;
        }

        /* Success. */
        return EGL_TRUE;
    }
    while (gcvFALSE);

    if (Display->config != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Display->config));
        Display->configCount = 0;
    }

    gcmVERIFY_OK(gcoOS_AtomDecrement(gcvNULL, Display->referenceDpy, gcvNULL));

    /* Error. */
    return EGL_FALSE;
}

void
veglDereferenceDisplay(
    IN VEGLThreadData Thread,
    IN VEGLDisplay Display,
    IN EGLBoolean Always
    )
{
    gctINT32 reference;

    if (Display->referenceDpy != gcvNULL)
    {
        /* Decrement the reference counter. */
        gcmVERIFY_OK(gcoOS_AtomDecrement(gcvNULL,
                                         Display->referenceDpy,
                                         &reference));
    }
    else
    {
        /* Initialize reference coiunter to something. */
        reference = 0;
    }

    if ((reference == 1) || Always)
    {
        /* Stop the thread. */
        if (Display->workerThread != gcvNULL)
        {
            /* Set thread's stop signal. */
            gcmASSERT(Display->stopSignal != gcvNULL);
            gcmVERIFY_OK(gcoOS_Signal(
                gcvNULL,
                Display->stopSignal,
                gcvTRUE
                ));

            /* Set thread's start signal. */
            gcmASSERT(Display->startSignal != gcvNULL);
            gcmVERIFY_OK(gcoOS_Signal(
                gcvNULL,
                Display->startSignal,
                gcvTRUE
                ));

            /* Wait until the thread is closed. */
            gcmVERIFY_OK(gcoOS_CloseThread(
                gcvNULL, Display->workerThread
                ));
            Display->workerThread = gcvNULL;
        }

#if gcdENABLE_TIMEOUT_DETECTION
        /* Remove the default timeout handler. */
        gcmVERIFY_OK(gcoOS_RemoveTimeoutCallback(
            _SwapSignalTimeoutCallback
            ));
#endif

        /* Delete the start signal. */
        if (Display->startSignal != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Display->startSignal));
            Display->startSignal = gcvNULL;
        }

        /* Delete the stop signal. */
        if (Display->stopSignal != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Display->stopSignal));
            Display->stopSignal = gcvNULL;
        }

        /* Delete the done signal. */
        if (Display->doneSignal != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_DestroySignal(gcvNULL, Display->doneSignal));
            Display->doneSignal = gcvNULL;
        }

        /* Delete the mutex. */
        if (Display->suspendMutex != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_DeleteMutex(gcvNULL, Display->suspendMutex));
            Display->suspendMutex = gcvNULL;
        }

        /* Dereference all surfaces. */
        while (Display->surfaceStack != gcvNULL)
        {
            VEGLSurface surface   = Display->surfaceStack;
            Display->surfaceStack = surface->next;

            veglDereferenceSurface(Thread, surface, EGL_TRUE);
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, surface));
        }

        /* Destroy the imageStack. */
        while (Display->imageStack != gcvNULL)
        {
            VEGLImage image     = Display->imageStack;
            Display->imageStack = image->next;

            veglDestroyImage(Thread, Display, image);
        }

        /* Destroy the imageRefStack. */
        while (Display->imageRefStack != gcvNULL)
        {
            VEGLImageRef ref       = Display->imageRefStack;
            Display->imageRefStack = ref->next;

            if (ref->surface != gcvNULL)
            {
                gcmVERIFY_OK(gcoHAL_DestroySurface(gcvNULL, ref->surface));
            }

            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, ref));
        }

        /* Free the configuration buffer. */
        if (Display->config != gcvNULL)
        {
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Display->config));
            Display->configCount = 0;
        }

        /* Free the reference counter. */
        if (Display->referenceDpy != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_AtomDestroy(gcvNULL, Display->referenceDpy));
            Display->referenceDpy = gcvNULL;
        }

        veglDeinitLocalDisplayInfo(Display);
    }
}

EGLAPI EGLBoolean EGLAPIENTRY
eglInitialize(
    EGLDisplay Dpy,
    EGLint *major,
    EGLint *minor
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLDisplay stack;

    gcmHEADER_ARG("Dpy=0x%x", Dpy);
    gcmDUMP_API("${EGL eglInitialize 0x%08X}", Dpy);

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

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    for (stack = (VEGLDisplay) gcoOS_GetPLSValue(gcePLS_VALUE_EGL_DISPLAY_INFO);
         stack != gcvNULL;
         stack = stack->next)
    {
        if (stack == dpy)
        {
            break;
        }
    }

    if ((dpy            == gcvNULL)
    ||  (stack          == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for "already initialized". */
    if ((dpy->referenceDpy == gcvNULL)
    ||  (dpy->contextStack == gcvNULL)
    )
    {
        if (!veglReferenceDisplay(thread, dpy))
        {
            /* Not initialized. */
            thread->error = EGL_NOT_INITIALIZED;
            gcmFOOTER_ARG("%d", EGL_FALSE);
            return EGL_FALSE;
        }
    }

    if (major != gcvNULL)
    {
        /* EGL 1.4 specification. */
        *major = 1;
    }

    if (minor != gcvNULL)
    {
        /* EGL 1.4 specification. */
        *minor = 4;
    }

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_ARG("*major=%d *minor=%d",
                  gcmOPT_VALUE(major), gcmOPT_VALUE(minor));

    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglTerminate(
    EGLDisplay Dpy
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLDisplay p_dpy;
    VEGLContext ctx = gcvNULL;
    gceSTATUS status;

    gcmHEADER_ARG("Dpy=0x%x", Dpy);
    gcmDUMP_API("${EGL eglTerminate 0x%08X}", Dpy);

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

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    for (p_dpy = (VEGLDisplay) gcoOS_GetPLSValue(gcePLS_VALUE_EGL_DISPLAY_INFO);
         p_dpy != gcvNULL;
         p_dpy = p_dpy->next)
    {
        if (p_dpy == dpy)
        {
            break;
        }
    }

    if ((dpy == gcvNULL)
    ||  (p_dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Execute any left-overs in the command buffer. */
    gcmONERROR(gcoHAL_Commit(gcvNULL, gcvTRUE));

    /* Release DC if necessary. */
    if (dpy->releaseDpy)
    {
        veglReleaseDefaultDisplay(dpy->hdc);

        /* This display should never be found again.*/
        dpy->hdc = EGL_DEFAULT_DISPLAY;

        dpy->releaseDpy = gcvFALSE;
    }

    if (dpy->referenceDpy != gcvNULL)
    {
        /* Clean up the EGLContext stack. */
        while (dpy->contextStack != EGL_NO_CONTEXT)
        {
            /* save the next context from context stack */
            ctx = dpy->contextStack->next;
            gcmVERIFY(eglDestroyContext(dpy, dpy->contextStack));
            dpy->contextStack = ctx;
        }

        /* Destroy the display. */
        veglDereferenceDisplay(thread, dpy, EGL_TRUE);

        /* Execute events accumulated in the buffer if any. */
        gcmONERROR(gcoHAL_Commit(gcvNULL, gcvTRUE));
    }

    /* uninstall the owner thread */
    dpy->ownerThread = gcvNULL;

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmFOOTER_NO();
    return EGL_TRUE;
OnError:
    thread->error = EGL_FALSE;
    gcmFOOTER();
    return EGL_FALSE;
}

EGLAPI const char * EGLAPIENTRY
eglQueryString(
    EGLDisplay Dpy,
    EGLint name
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    const char * ptr;

    gcmHEADER_ARG("Dpy=0x%x name=0x%04x", Dpy, name);

    /* Get thread data. */
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

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("0x%x", gcvNULL);
        return gcvNULL;
    }

    /* Check for reference. */
    if ((dpy->referenceDpy == gcvNULL)
    &&  (dpy->contextStack == EGL_NO_CONTEXT)
    )
    {
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("0x%x", gcvNULL);
        return gcvNULL;
    }

    switch (name)
    {
    case EGL_CLIENT_APIS:
        ptr = "OpenGL_ES OpenVG";
        break;

    case EGL_EXTENSIONS:
        ptr = extension;
        break;

    case EGL_VENDOR:
        ptr = "Vivante Corporation";
        break;

    case EGL_VERSION:
        ptr = "1.4";
        break;

    default:
        /* Bad parameter. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("0x%x", gcvNULL);
        return gcvNULL;
    }

    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglQueryString 0x%08X 0x%08X :=", Dpy, name);
    gcmDUMP_API_DATA(ptr, 0);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("%s", ptr);
    return ptr;
}

/*
    If eglSetBlobCacheFuncsANDROID generates an error then all client APIs must
    behave as though eglSetBlobCacheFuncsANDROID was not called for the display
    <dpy>.  If <set> or <get> is NULL then an EGL_BAD_PARAMETER error is
    generated.  If a successful eglSetBlobCacheFuncsANDROID call was already
    made for <dpy> and the display has not since been terminated then an
    EGL_BAD_PARAMETER error is generated.
*/
EGLAPI void EGLAPIENTRY
eglSetBlobCacheFuncsANDROID(
    EGLDisplay Dpy,
    EGLSetBlobFuncANDROID Set,
    EGLGetBlobFuncANDROID Get
    )
{

    VEGLThreadData thread;
    VEGLDisplay dpy;

    gcmHEADER_ARG("Dpy=0x%x Set=0x%x Get=0x%x", Dpy, Set, Get);
    gcmDUMP_API("${EGL eglSetBlobCacheFuncsANDROID 0x%08X 0x%08X 0x%08X}", Dpy, Set, Get);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_NO();
        return;
    }

    /* Create a shortcut to the display. */
    dpy = VEGL_DISPLAY(Dpy);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): VEGL_DISPLAY failed.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_NO();
        return;
    }

    /* Test for input functions.*/
    if (Set == gcvNULL || Get == gcvNULL)
    {
        /* Bad set or get function. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_NO();
        return;
    }

    /* Test if set/get functions are already called for the display.*/
    if ((dpy->blobCacheGet != gcvNULL) || (dpy->blobCacheSet != gcvNULL))
    {
        /* set/get functions are already called. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_NO();
        return;
    }

    dpy->blobCacheGet = Get;
    dpy->blobCacheSet = Set;

    gcmFOOTER_NO();
    return;
}

#if !gcdSTATIC_LINK
EGLAPI
#endif
EGLsizeiANDROID EGLAPIENTRY
veglGetBlobCache(
    const void* Key,
    EGLsizeiANDROID KeySize,
    void* Value,
    EGLsizeiANDROID ValueSize
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    EGLGetBlobFuncANDROID get;
    EGLsizeiANDROID result;
    gcmHEADER_ARG("Key=0x%x KeySize=0x%x Value=0x%x ValueSize=0x%x", Key, KeySize, Value, ValueSize);

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_ARG("return=0x%x", 0);
        return 0;
    }

    if (thread->context == EGL_NO_CONTEXT)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): thread->context is EGL_NO_CONTEXT.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_ARG("return=0x%x", 0);
        return 0;
    }


    dpy = thread->context->display;
    if (dpy == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): thread->context->display is gcvNULL.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_ARG("return=0x%x", 0);
        return 0;
    }

    get = dpy->blobCacheGet;
    if (get == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): dpy->blobCacheGet is gcvNULL.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_ARG("return=0x%x", 0);
        return 0;
    }

    result = get(Key, KeySize, Value, ValueSize);

    gcmDUMP_API("${EGL veglGetBlobCache 0x%x 0x%x 0x%x 0x%x 0x%x :=", result, ValueSize, Value, KeySize, Key);
    gcmDUMP_API_DATA(Key, KeySize);
    gcmDUMP_API("$}");

    gcmFOOTER_ARG("return=0x%x", result);
    return result;
}


#if !gcdSTATIC_LINK
EGLAPI
#endif
void EGLAPIENTRY
veglSetBlobCache(
    const void* Key,
    EGLsizeiANDROID KeySize,
    const void* Value,
    EGLsizeiANDROID ValueSize
    )
{

    VEGLThreadData thread;
    VEGLDisplay dpy;
    EGLSetBlobFuncANDROID set;

    gcmHEADER_ARG("Key=0x%x KeySize=0x%x Value=0x%x ValueSize=0x%x", Key, KeySize, Value, ValueSize);
    gcmDUMP_API("${EGL veglSetBlobCache 0x%x 0x%x 0x%x 0x%x :=", ValueSize, Value, KeySize, Key);
    gcmDUMP_API_DATA(Key, KeySize);
    gcmDUMP_API("$}");

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_ARG("return=0x%x", 0);
        return;
    }

    if (thread->context == EGL_NO_CONTEXT)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): thread->context is EGL_NO_CONTEXT.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_ARG("return=0x%x", 0);
        return;
    }


    dpy = thread->context->display;
    if (dpy == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): thread->context->display is gcvNULL.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_ARG("return=0x%x", 0);
        return;
    }

    set = dpy->blobCacheSet;
    if (set == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): dpy->blobCacheGet is gcvNULL.",
            __FUNCTION__, __LINE__
            );
        gcmFOOTER_ARG("return=0x%x", 0);
        return;
    }

    set(Key, KeySize, Value, ValueSize);

    gcmFOOTER_NO();
}

