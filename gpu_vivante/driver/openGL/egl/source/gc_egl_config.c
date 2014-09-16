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

#define _GC_OBJ_ZONE    gcdZONE_EGL_CONFIG

static EGLBoolean
veglParseAttributes(
    VEGLDisplay Display,
    const EGLint * AttributeList,
    VEGLConfig Configuration
    )
{
    VEGLThreadData thread;
    EGLenum attribute;
    EGLint value = 0;

    /* Get thread data. */
    thread = veglGetThreadData();
    if (thread == gcvNULL)
    {
        gcmTRACE(
            gcvLEVEL_ERROR,
            "%s(%d): veglGetThreadData failed.",
            __FUNCTION__, __LINE__
            );

        return EGL_FALSE;
    }

    /* Assume success. */
    thread->error = EGL_SUCCESS;

    /* Fill in default attributes. */
    Configuration->bufferSize        = 0;
    Configuration->alphaSize         = 0;
    Configuration->blueSize          = 0;
    Configuration->greenSize         = 0;
    Configuration->redSize           = 0;
    Configuration->depthSize         = 0;
    Configuration->stencilSize       = 0;
    Configuration->configCaveat      = (EGLenum) EGL_DONT_CARE;
    Configuration->configId          = EGL_DONT_CARE;
    Configuration->nativeRenderable  = (EGLBoolean) EGL_DONT_CARE;
    Configuration->nativeVisualType  = EGL_DONT_CARE;
    Configuration->samples           = 0;
    Configuration->sampleBuffers     = 0;
    Configuration->surfaceType       = EGL_WINDOW_BIT;
    Configuration->bindToTetxureRGB  = (EGLBoolean) EGL_DONT_CARE;
    Configuration->bindToTetxureRGBA = (EGLBoolean) EGL_DONT_CARE;
    Configuration->luminanceSize     = 0;
    Configuration->alphaMaskSize     = 0;
    Configuration->colorBufferType   = EGL_RGB_BUFFER;
    Configuration->renderableType    = EGL_OPENGL_ES_BIT;
    Configuration->conformant        = 0;
    Configuration->matchFormat       = (EGLenum) EGL_DONT_CARE;
    Configuration->matchNativePixmap = EGL_NONE;
    Configuration->recordableConfig  = (EGLBoolean) EGL_DONT_CARE;

    /* Parse the attribute list. */
    do
    {
        if (AttributeList != gcvNULL)
        {
            attribute      = AttributeList[0];
            value          = AttributeList[1];
            AttributeList += 2;
        }
        else
        {
            attribute = EGL_NONE;
        }

        switch (attribute)
        {
        case EGL_BUFFER_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                         "%s: EGL_BUFFER_SIZE=%d",
                         __FUNCTION__, value);
            Configuration->bufferSize = value;
            break;

        case EGL_ALPHA_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                         "%s: EGL_ALPHA_SIZE=%d",
                         __FUNCTION__, value);
            Configuration->alphaSize = value;
            break;

        case EGL_BLUE_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_BLUE_SIZE=%d",
                          __FUNCTION__, value);
            Configuration->blueSize = value;
            break;

        case EGL_GREEN_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_GREEN_SIZE=%d",
                          __FUNCTION__, value);
            Configuration->greenSize = value;
            break;

        case EGL_RED_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_RED_SIZE=%d",
                          __FUNCTION__, value);
            Configuration->redSize = value;
            break;

        case EGL_DEPTH_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_DEPTH_SIZE=%d",
                          __FUNCTION__, value);
            Configuration->depthSize = value;
            break;

        case EGL_STENCIL_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_STENCIL_SIZE=%d",
                          __FUNCTION__, value);
            Configuration->stencilSize = value;
            break;

        case EGL_CONFIG_CAVEAT:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_CONFIG_CAVEAT=%d",
                          __FUNCTION__, value);
            Configuration->configCaveat = value;
            break;

        case EGL_CONFIG_ID:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_CONFIG_ID=%d",
                          __FUNCTION__, value);
            Configuration->configId = value;
            break;

        case EGL_LEVEL:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_LEVEL=%d",
                          __FUNCTION__, value);
            if ((value != EGL_DONT_CARE) && (value != 0))
            {
                return EGL_FALSE;
            }
            break;

        case EGL_MAX_PBUFFER_WIDTH:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_MAX_PBUFFER_WIDTH=%d",
                          __FUNCTION__, value);
            if ((value != EGL_DONT_CARE) && (value > thread->maxWidth))
            {
                return EGL_FALSE;
            }
            break;

        case EGL_MAX_PBUFFER_HEIGHT:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_MAX_PBUFFER_HEIGHT=%d",
                          __FUNCTION__, value);
            if ((value != EGL_DONT_CARE) && (value > thread->maxHeight))
            {
                return EGL_FALSE;
            }
            break;

        case EGL_MAX_PBUFFER_PIXELS:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_MAX_PBUFFER_PIXELS=%d",
                          __FUNCTION__, value);
            if ( (value != EGL_DONT_CARE) &&
                 (value > thread->maxWidth * thread->maxHeight) )
            {
                return EGL_FALSE;
            }
            break;

        case EGL_NATIVE_RENDERABLE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_NATIVE_RENDERABLE=%d",
                          __FUNCTION__, value);
            Configuration->nativeRenderable = value;
            break;

        case EGL_NATIVE_VISUAL_ID:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_NATIVE_VISUAL_ID=%d",
                          __FUNCTION__, value);
            if
            (
                (value != EGL_DONT_CARE)
                &&
                (gcmINT2PTR(value) != (void*) Display->hdc)
            )
            {
                return EGL_FALSE;
            }
            break;

        case EGL_NATIVE_VISUAL_TYPE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_NATIVE_VISUAL_TYPE=%d",
                          __FUNCTION__, value);
            Configuration->nativeVisualType = value;
            break;

        case EGL_SAMPLES:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_SAMPLES=%d",
                          __FUNCTION__, value);
            Configuration->samples = value;
            break;

        case EGL_SAMPLE_BUFFERS:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_SAMPLE_BUFFERS=%d",
                          __FUNCTION__, value);
            Configuration->sampleBuffers = value;
            break;

        case EGL_SURFACE_TYPE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_SURFACE_TYPE=%d",
                          __FUNCTION__, value);
            Configuration->surfaceType = value;
            break;

        case EGL_TRANSPARENT_TYPE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_TRANSPARENT_TYPE=%d",
                          __FUNCTION__, value);
            if ((value != EGL_NONE) && (value != EGL_DONT_CARE))
            {
                return EGL_FALSE;
            }
            break;

        case EGL_TRANSPARENT_BLUE_VALUE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_TRANSPARENT_BLUE_VALUE=%d",
                          __FUNCTION__, value);
            if (value != EGL_DONT_CARE)
            {
                return EGL_FALSE;
            }
            break;

        case EGL_TRANSPARENT_GREEN_VALUE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_TRANSPARENT_GREEN_VALUE=%d",
                          __FUNCTION__, value);
            if (value != EGL_DONT_CARE)
            {
                return EGL_FALSE;
            }
            break;

        case EGL_TRANSPARENT_RED_VALUE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_TRANSPARENT_RED_VALUE=%d",
                          __FUNCTION__, value);
            if (value != EGL_DONT_CARE)
            {
                return EGL_FALSE;
            }
            break;

        case EGL_BIND_TO_TEXTURE_RGB:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_BIND_TO_TEXTURE_RGB=%d",
                          __FUNCTION__, value);
            Configuration->bindToTetxureRGB = value;
            break;

        case EGL_BIND_TO_TEXTURE_RGBA:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_BIND_TO_TEXTURE_RGBA=%d",
                          __FUNCTION__, value);
            Configuration->bindToTetxureRGBA = value;
            break;

        case EGL_MIN_SWAP_INTERVAL:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_MIN_SWAP_INTERVAL=%d",
                          __FUNCTION__, value);
            if ((value != EGL_DONT_CARE) && (value > 1))
            {
                return EGL_FALSE;
            }
            break;

        case EGL_MAX_SWAP_INTERVAL:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_MAX_SWAP_INTERVAL=%d",
                          __FUNCTION__, value);
            if ((value != EGL_DONT_CARE) && (value > 1))
            {
                return EGL_FALSE;
            }
            break;

        case EGL_LUMINANCE_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_LUMINANCE_SIZE=%d",
                          __FUNCTION__, value);
            Configuration->luminanceSize = value;
            break;

        case EGL_ALPHA_MASK_SIZE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_ALPHA_MASK_SIZE=%d",
                          __FUNCTION__, value);
            Configuration->alphaMaskSize = value;
            break;

        case EGL_COLOR_BUFFER_TYPE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_COLOR_BUFFER_TYPE=%d",
                          __FUNCTION__, value);
            Configuration->colorBufferType = value;
            break;

        case EGL_RENDERABLE_TYPE:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_RENDERABLE_TYPE=%d",
                          __FUNCTION__, value);
            Configuration->renderableType = value;
            break;

        case EGL_CONFORMANT:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_RENDERABLE_TYPE=%d",
                          __FUNCTION__, value);
            Configuration->conformant = value;
            break;

        case EGL_MATCH_FORMAT_KHR:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_MATCH_FORMAT_KHR=%d",
                          __FUNCTION__, value);
            Configuration->matchFormat = value;
            break;

        case EGL_MATCH_NATIVE_PIXMAP:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_MATCH_NATIVE_PIXMAP=%d",
                          __FUNCTION__, value);
            Configuration->matchNativePixmap = value;
            break;

        case EGL_RECORDABLE_ANDROID:
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "%s: EGL_RECORDABLE_ANDROID=%d",
                          __FUNCTION__, value);
            Configuration->recordableConfig = value;
            break;

        case EGL_NONE:
            break;

        default:
            /* Bad attribute. */
            thread->error = EGL_BAD_ATTRIBUTE;
            return EGL_FALSE;
        }
    }
    while (attribute != EGL_NONE);

    /* Success. */
    return EGL_TRUE;
}

static EGLBoolean
veglSortAfter(
    VEGLConfig Config1,
    VEGLConfig Config2,
    VEGLConfig Attributes
    )
{
    EGLint bits1, bits2;

    /* Priority 1. */
    if (Config1->configCaveat < Config2->configCaveat)
    {
        return EGL_FALSE;
    }
    else if (Config1->configCaveat > Config2->configCaveat)
    {
        return EGL_TRUE;
    }

    /* Priority 2. */
    if (Config1->colorBufferType < Config2->colorBufferType)
    {
        return EGL_FALSE;
    }
    else if (Config1->colorBufferType > Config2->colorBufferType)
    {
        return EGL_TRUE;
    }

    /* Compute number of color bits based on attributes. */
    bits1 = bits2 = 0;
    if (Config1->colorBufferType == EGL_RGB_BUFFER)
    {
        bits1 += Config1->redSize + Config1->greenSize + Config1->blueSize;
        bits2 += Config2->redSize + Config2->greenSize + Config2->blueSize;
    }
    else
    {
        bits1 += Config1->luminanceSize;
        bits2 += Config2->luminanceSize;
    }

    bits1 += Config1->alphaSize;
    bits2 += Config2->alphaSize;


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
    if (Config1->bufferSize < Config2->bufferSize)
    {
        return EGL_FALSE;
    }
    else if (Config1->bufferSize > Config2->bufferSize)
    {
        return EGL_TRUE;
    }

    /* Priority 5. */
    if (Config1->sampleBuffers < Config2->sampleBuffers)
    {
        return EGL_FALSE;
    }
    else if (Config1->sampleBuffers > Config2->sampleBuffers)
    {
        return EGL_TRUE;
    }

    /* Priority 6. */
    if (Config1->samples < Config2->samples)
    {
        return EGL_FALSE;
    }
    else if (Config1->samples > Config2->samples)
    {
        return EGL_TRUE;
    }

    /* Priority 7. */
    if (Config1->depthSize < Config2->depthSize)
    {
        return EGL_FALSE;
    }
    else if (Config1->depthSize > Config2->depthSize)
    {
        return EGL_TRUE;
    }

    /* Priority 8. */
    if (Config1->stencilSize < Config2->stencilSize)
    {
        return EGL_FALSE;
    }
    else if (Config1->stencilSize > Config2->stencilSize)
    {
        return EGL_TRUE;
    }

    /* Priority 9. */
    if (Config1->alphaMaskSize < Config2->alphaMaskSize)
    {
        return EGL_FALSE;
    }
    else if (Config1->alphaMaskSize > Config2->alphaMaskSize)
    {
        return EGL_TRUE;
    }

    /* Compute native visual type sorting. */
    bits1 = (Config1->nativeVisualType == EGL_NONE)
        ? 0
        : Config1->nativeVisualType;
    bits2 = (Config2->nativeVisualType == EGL_NONE)
        ? 0
        : Config2->nativeVisualType;

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
    if (Config1->configId < Config2->configId)
    {
        return EGL_FALSE;
    }
    else if (Config1->configId > Config2->configId)
    {
        return EGL_TRUE;
    }

    /* Nothing to sort. */
    return EGL_FALSE;
}

static void
veglSort(
    VEGLConfig *Config,
    EGLint ConfigCount,
    VEGLConfig Attributes
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
            if (veglSortAfter(Config[i], Config[i + 1], Attributes))
            {
                /* Swap configurations. */
                EGLConfig temp = Config[i];
                Config[i] = Config[i + 1];
                Config[i + 1] = temp;

                /* We need another pass. */
                swapped = EGL_TRUE;
            }
        }
    }
    while (swapped);
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigs(
    EGLDisplay Dpy,
    EGLConfig *configs,
    EGLint config_size,
    EGLint *num_config
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;

    gcmHEADER_ARG("Dpy=0x%x configs=0x%x config_size=%d",
                  Dpy, configs, config_size);

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

    /* Test for initialized or not. */
    if (dpy->referenceDpy == gcvNULL &&
        dpy->contextStack == EGL_NO_CONTEXT)
    {
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if (num_config == gcvNULL)
    {
        /* Bad parameter. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Reference the EGLDisplay. */
    if (!veglReferenceDisplay(thread, dpy))
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if (configs == gcvNULL)
    {
        /* Return number of configurations. */
        *num_config = dpy->configCount;
    }
    else
    {
        EGLint index;

        /* Copy pointers to configurations into supplied buffer. */
        for (index = 0; index < dpy->configCount; index++)
        {
            /* Bail out if the supplied buffer is too small. */
            if (index >= config_size)
            {
                break;
            }

            configs[index] = &dpy->config[index];
        }

        *num_config = index;

        /* Zero any remaining configurations in the supplied buffer. */
        while (index < config_size)
        {
            configs[index++] = gcvNULL;
        }
    }

    /* Dereference the EGLDisplay. */
    veglDereferenceDisplay(thread, dpy, EGL_FALSE);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglGetConfigs 0x%08X (0x%08X) 0x%08X (0x%08X)",
                Dpy, configs, config_size, num_config);
    gcmDUMP_API_ARRAY(configs, *num_config);
    gcmDUMP_API_ARRAY(num_config, 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*num_config=%d", *num_config);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglChooseConfig(
    EGLDisplay Dpy,
    const EGLint *attrib_list,
    EGLConfig *configs,
    EGLint config_size,
    EGLint *num_config
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    struct eglConfig criteria = { 0 };
    EGLint config;

    gcmHEADER_ARG("Dpy=0x%x attrib_list=0x%x configs=0x%x config_size=%d",
                  Dpy, attrib_list, configs, config_size);

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
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if (num_config == gcvNULL)
    {
        /* Bad parameter. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Parse attributes. */
    if (!veglParseAttributes(dpy, attrib_list, &criteria))
    {
        /* Bail out on invalid or non-matching attributes. */
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Reference the EGLDisplay. */
    if (!veglReferenceDisplay(thread, dpy))
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Reset number of configurations. */
    *num_config = 0;

    /* Walk through all configurations. */
    for (config = 0; config < dpy->configCount; config++)
    {
        /* Get pointer to configuration. */
        VEGLConfig configuration = &dpy->config[config];

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                      "%s: examining config index=%d",
                      __FUNCTION__, config);

		if (criteria.configId != EGL_DONT_CARE)
        {
            if (!((criteria.configId == configuration->configId) ||
                ((criteria.configId == 0) && configuration->defaultConfig)))
            {
                /* Criterium doesn't match. */
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                    "  rejected on config ID.");
                continue;
            }
            else
            {
                *num_config += 1;

                if (configs != gcvNULL)
                {
                    /* Copy configuration into specified buffer. */
                    *configs = configuration;
                }

                break;
            }
        }

        /* Check configuration against criteria. */
        if (criteria.bufferSize > configuration->bufferSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on buffer size.");
            continue;
        }

        if (criteria.alphaSize > configuration->alphaSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on alpha size.");
            continue;
        }

        if (criteria.blueSize > configuration->blueSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on blue size.");
            continue;
        }

        if (criteria.greenSize > configuration->greenSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on green size.");
            continue;
        }

        if (criteria.redSize > configuration->redSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on red size.");
            continue;
        }

        if (criteria.depthSize > configuration->depthSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on depth size.");
            continue;
        }

        if (criteria.stencilSize > configuration->stencilSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on stencil size.");
            continue;
        }

        if ((criteria.configCaveat != (EGLenum) EGL_DONT_CARE)
        &&  (criteria.configCaveat != configuration->configCaveat)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on config caveat.");
            continue;
        }

        if ((criteria.configId != EGL_DONT_CARE) &&
            !((criteria.configId == configuration->configId) ||
              ((criteria.configId == 0) && configuration->defaultConfig)))
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on config ID.");
            continue;
        }

        if ((criteria.nativeRenderable != (EGLBoolean) EGL_DONT_CARE)
        &&  criteria.nativeRenderable
        &&  (criteria.nativeRenderable != configuration->nativeRenderable)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on native renderable.");
            continue;
        }

        if ((criteria.nativeVisualType != EGL_DONT_CARE) &&
            (criteria.nativeVisualType != configuration->nativeVisualType))
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on native visual type.");
            continue;
        }

        if (criteria.samples > configuration->samples)
        {
            if ((criteria.samples == 1) && (configuration->samples == 0))
            {
                /* Criterium still matches. */
            }
            else
            {
                /* Criterium doesn't match. */
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                              "  rejected on samples.");
                continue;
            }
        }

        if (criteria.sampleBuffers > configuration->sampleBuffers)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on sample buffers.");
            continue;
        }

        if ((criteria.surfaceType != (EGLenum) EGL_DONT_CARE)
        &&  !(criteria.surfaceType & configuration->surfaceType)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on surface type.");
            continue;
        }

        if ((criteria.bindToTetxureRGB != (EGLBoolean) EGL_DONT_CARE)
        &&  (criteria.bindToTetxureRGB != configuration->bindToTetxureRGB)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on bind to tetxure RGB.");
            continue;
        }

        if ((criteria.bindToTetxureRGBA != (EGLBoolean) EGL_DONT_CARE)
        &&  (criteria.bindToTetxureRGBA != configuration->bindToTetxureRGBA)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on bind to tetxure RGBA.");
            continue;
        }

        if (criteria.luminanceSize > configuration->luminanceSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on luminance size.");
            continue;
        }

        if (criteria.alphaMaskSize > configuration->alphaMaskSize)
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on alpha mask size.");
            continue;
        }

        if ((criteria.colorBufferType != (EGLenum) EGL_DONT_CARE)
        &&  (criteria.colorBufferType != configuration->colorBufferType)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on color buffer type.");
            continue;
        }

        if ((criteria.renderableType != (EGLenum) EGL_DONT_CARE)
        &&  ((criteria.renderableType & configuration->renderableType)
             != criteria.renderableType)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on renderable type.");
            continue;
        }

        if ((criteria.conformant != (EGLenum) EGL_DONT_CARE)
        &&  (criteria.conformant != 0)
        &&  !(criteria.conformant & configuration->conformant)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on conformant.");
            continue;
        }

        if ((criteria.matchNativePixmap != EGL_DONT_CARE)
        &&  (criteria.matchNativePixmap != EGL_NONE)
        )
        {
            EGLBoolean bMatch = EGL_TRUE;
            gctUINT width, height;
            gceSURF_FORMAT format = gcvSURF_UNKNOWN;
            gctUINT bitsPerPixel;

            if (!(configuration->surfaceType & EGL_PIXMAP_BIT)
            ||  !veglGetPixmapInfo(dpy->hdc,
                                   (NativePixmapType)
                                   gcmINT2PTR(criteria.matchNativePixmap),
                                   &width, &height,
                                   &bitsPerPixel,
                                   &format)
            )
            {
                /* surface type of config is not EGL_PIXMAP_BIT or bad pixmap. */
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                              "  rejected on match native pixmap.");
                continue;
            }

            switch (format)
            {
            case gcvSURF_R5G6B5:
                if ((configuration->redSize   != 5)
                ||  (configuration->greenSize != 6)
                ||  (configuration->blueSize  != 5)
                )
                {
                    bMatch = EGL_FALSE;
                }
                break;

            case gcvSURF_X8R8G8B8:
                if ((configuration->redSize   != 8)
                ||  (configuration->greenSize != 8)
                ||  (configuration->blueSize  != 8)
                ||  (configuration->alphaSize != 0)
                )
                {
                    bMatch = EGL_FALSE;
                }
                break;

            default:
                break;
            }

            if (!bMatch)
            {
                /* Criterium doesn't match. */
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                              "  rejected on match native pixmap.");
                continue;
            }
        }

        if ((criteria.matchFormat != (EGLenum) EGL_DONT_CARE)
        &&  (criteria.matchFormat != (EGLenum) EGL_NONE)
        )
        {
            EGLBoolean bMatch = EGL_TRUE;

            switch(criteria.matchFormat)
            {
            case EGL_FORMAT_RGB_565_EXACT_KHR:
                if (configuration->matchFormat != EGL_FORMAT_RGB_565_EXACT_KHR)
                {
                    bMatch = EGL_FALSE;
                }
                break;

            case EGL_FORMAT_RGB_565_KHR:
                if ((configuration->matchFormat != EGL_FORMAT_RGB_565_EXACT_KHR)
                &&  (configuration->matchFormat != EGL_FORMAT_RGB_565_KHR)
                )
                {
                    bMatch = EGL_FALSE;
                }
                break;

            case EGL_FORMAT_RGBA_8888_EXACT_KHR:
                if (configuration->matchFormat != EGL_FORMAT_RGBA_8888_EXACT_KHR)
                {
                    bMatch = EGL_FALSE;
                }
                break;

            case EGL_FORMAT_RGBA_8888_KHR:
                if ((configuration->matchFormat != EGL_FORMAT_RGBA_8888_EXACT_KHR)
                &&  (configuration->matchFormat != EGL_FORMAT_RGBA_8888_KHR)
                )
                {
                    bMatch = EGL_FALSE;
                }
                break;

            default:
                break;
            }

            if (!bMatch)
            {
                /* Criterium doesn't match. */
                gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                              "  rejected on conformant.");
                continue;
            }
        }

        if ((criteria.recordableConfig != (EGLBoolean) EGL_DONT_CARE)
        &&  (criteria.recordableConfig != configuration->recordableConfig)
        )
        {
            /* Criterium doesn't match. */
            gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG,
                          "  rejected on recordable config.");
            continue;
        }

        gcmTRACE_ZONE(gcvLEVEL_INFO, gcdZONE_EGL_CONFIG, "  accepted.");

        if (configs != gcvNULL)
        {
            /* Copy configuration into specified buffer. */
            configs[*num_config] = configuration;
        }

        *num_config += 1;

        if (*num_config == config_size)
        {
            /* Bail out if the specified buffer is full. */
            break;
        }
    }

    if ((*num_config > 1) && (configs != gcvNULL))
    {
        /* Sort the matching configurations. */
        veglSort((VEGLConfig *) configs, *num_config, &criteria);
    }

    /* Dereference the EGLDisplay. */
    veglDereferenceDisplay(thread, dpy, EGL_FALSE);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglChooseConfig 0x%08X (0x%08X) (0x%08X) 0x%08X "
                "(0x%08X)",
                Dpy, attrib_list, configs, config_size, num_config);
    gcmDUMP_API_ARRAY_TOKEN(attrib_list, EGL_NONE);
    gcmDUMP_API_ARRAY(configs, *num_config);
    gcmDUMP_API_ARRAY(num_config, 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*num_config=%d", *num_config);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY
eglGetConfigAttrib(
    EGLDisplay Dpy,
    EGLConfig Config,
    EGLint attribute,
    EGLint *value
    )
{
    VEGLThreadData thread;
    VEGLDisplay dpy;
    VEGLConfig config;

    gcmHEADER_ARG("Dpy=0x%x Config=0x%x attribute=%d", Dpy, Config, attribute);

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
    dpy    = VEGL_DISPLAY(Dpy);
    config = VEGL_CONFIG(Config);

    /* Test for valid EGLDisplay structure. */
    if ((dpy == gcvNULL)
    ||  (dpy->signature != EGL_DISPLAY_SIGNATURE)
    )
    {
        /* Bad display. */
        thread->error = EGL_BAD_DISPLAY;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    if (value == gcvNULL)
    {
        /* Bad parameter. */
        thread->error = EGL_BAD_PARAMETER;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Test for valid config. */
    if ((config < dpy->config)
    ||  (config >= dpy->config + dpy->configCount)
    )
    {
        thread->error = EGL_BAD_CONFIG;
        gcmFOOTER_ARG("0x%x", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Reference the EGLDisplay. */
    if (!veglReferenceDisplay(thread, dpy))
    {
        /* Not initialized. */
        thread->error = EGL_NOT_INITIALIZED;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    switch (attribute)
    {
    case EGL_BUFFER_SIZE:
        *value = config->bufferSize;
        break;

    case EGL_ALPHA_SIZE:
        *value = config->alphaSize;
        break;

    case EGL_BLUE_SIZE:
        *value = config->blueSize;
        break;

    case EGL_GREEN_SIZE:
        *value = config->greenSize;
        break;

    case EGL_RED_SIZE:
        *value = config->redSize;
        break;

    case EGL_DEPTH_SIZE:
        *value = config->depthSize;
        break;

    case EGL_STENCIL_SIZE:
        *value = config->stencilSize;
        break;

    case EGL_CONFIG_CAVEAT:
        *value = config->configCaveat;
        break;

    case EGL_CONFIG_ID:
        *value = config->configId;
        break;

    case EGL_LEVEL:
        *value = 0;
        break;

    case EGL_MAX_PBUFFER_WIDTH:
        *value = thread->maxWidth;
        break;

    case EGL_MAX_PBUFFER_HEIGHT:
        *value = thread->maxHeight;
        break;

    case EGL_MAX_PBUFFER_PIXELS:
        *value = thread->maxWidth * thread->maxHeight;
        break;

    case EGL_NATIVE_RENDERABLE:
        *value = config->nativeRenderable;
        break;

    case EGL_NATIVE_VISUAL_ID:
        *value = veglGetNativeVisualId(dpy, config);
        break;

    case EGL_NATIVE_VISUAL_TYPE:
        *value = config->nativeVisualType;
        break;

    case EGL_PRESERVED_RESOURCES:
        *value = EGL_TRUE;
        break;

    case EGL_SAMPLES:
        *value = config->samples;
        break;

    case EGL_SAMPLE_BUFFERS:
        if ((config->samples == 16) && (thread->api == EGL_OPENVG_API))
        {
            *value = 0;
        }
        else
        {
            *value = config->sampleBuffers;
        }
        break;

    case EGL_SURFACE_TYPE:
        *value = config->surfaceType;
        break;

    case EGL_TRANSPARENT_TYPE:
        *value = EGL_NONE;
        break;

    case EGL_TRANSPARENT_BLUE_VALUE:
    case EGL_TRANSPARENT_GREEN_VALUE:
    case EGL_TRANSPARENT_RED_VALUE:
        *value = EGL_DONT_CARE;
        break;

    case EGL_BIND_TO_TEXTURE_RGB:
        *value = config->bindToTetxureRGB;
        break;

    case EGL_BIND_TO_TEXTURE_RGBA:
        *value = config->bindToTetxureRGBA;
        break;

    case EGL_MIN_SWAP_INTERVAL:
    case EGL_MAX_SWAP_INTERVAL:
        *value = 1;
        break;

    case EGL_LUMINANCE_SIZE:
        *value = config->luminanceSize;
        break;

    case EGL_ALPHA_MASK_SIZE:
        *value = config->alphaMaskSize;
        break;

    case EGL_COLOR_BUFFER_TYPE:
        *value = config->colorBufferType;
        break;

    case EGL_RENDERABLE_TYPE:
        *value = config->renderableType;
        break;

    case EGL_CONFORMANT:
        *value = config->conformant;
        break;

    case EGL_MATCH_NATIVE_PIXMAP:
        *value = config->matchNativePixmap;
		break;

    case EGL_RECORDABLE_ANDROID:
        *value = config->recordableConfig;
		break;

    default:
        /* Dereference the EGLDisplay. */
        veglDereferenceDisplay(thread, dpy, EGL_FALSE);

        /* Bad attribute. */
        thread->error = EGL_BAD_ATTRIBUTE;
        gcmFOOTER_ARG("return=%d", EGL_FALSE);
        return EGL_FALSE;
    }

    /* Dereference the EGLDisplay. */
    veglDereferenceDisplay(thread, dpy, EGL_FALSE);

    /* Success. */
    thread->error = EGL_SUCCESS;
    gcmDUMP_API("${EGL eglGetConfigAttrib 0x%08X 0x%08X 0x%08X := 0x%08X}",
                Dpy, Config, attribute, *value);
    gcmFOOTER_ARG("*value=%d", *value);
    return EGL_TRUE;
}
