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




#include "gc_copybit.h"
#include <stdlib.h>

static int
copybit_device_open(
    const struct hw_module_t * module,
    const char * name,
    struct hw_device_t ** device
    );

static int
copybit_device_close(
    struct hw_device_t * dev
    );

static int
copybit_set_parameter(
    struct copybit_device_t * dev,
    int name,
    int value
    );

static int
copybit_get(
    struct copybit_device_t * dev,
    int name
    );

static int
copybit_blit(
    struct copybit_device_t * dev,
    struct copybit_image_t const * dst,
    struct copybit_image_t const * src,
    struct copybit_region_t const * region
    );

static int
copybit_stretch(
    struct copybit_device_t * dev,
    struct copybit_image_t const * dst,
    struct copybit_image_t const * src,
    struct copybit_rect_t const * dst_rect,
    struct copybit_rect_t const * src_rect,
    struct copybit_region_t const * region
    );

static struct hw_module_methods_t _module_methods =
{
    open: copybit_device_open
};

/* Hardware Module Interface. */
struct copybit_module_t HMI =
{
    common:
    {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: COPYBIT_HARDWARE_MODULE_ID,
        name: "Vivante 2D copybit module",
        author: "Vivante Corporation",
        methods: &_module_methods,
        dso: NULL,
        reserved: {0, }
    }
};


/*****************************************************************************/
static pthread_key_t _key;
static int initialized;

static void __attribute__((constructor))
_Init(
    void
    )
{
    pthread_key_create(&_key, NULL);
    initialized = 1;
}

static void __attribute__((destructor))
_Finish(
    void
    )
{
    if (initialized)
    {
        pthread_key_delete(_key);

        initialized = 0;
        _key = 0;
    }
}

static gcsCOPYBIT_CONTEXT *
_GetContext(
    void
    )
{
    void * tls = pthread_getspecific(_key);
    return (gcsCOPYBIT_CONTEXT *) tls;
}

static void
_SetContext(
    gcsCOPYBIT_CONTEXT * Context
    )
{
    pthread_setspecific(_key, Context);
}

/*****************************************************************************/

int
copybit_set_parameter(
    struct copybit_device_t * dev,
    int name,
    int value
    )
{
    int rel = -EINVAL;
    gcsCOPYBIT_CONTEXT * context = _GetContext();

    if (context == gcvNULL || &context->device != dev)
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "Invalid copybit device!");

        return -EINVAL;
    }

    switch (name)
    {
    case COPYBIT_ROTATION_DEG:
        gcmTRACE(gcvLEVEL_ERROR,
                 "COPYBIT_ROTATION_DEG is not supported.");

        if (value >= 0 && value <= 360)
        {
            if (context->rotationDeg != value)
            {
                context->rotationDeg = value;
                context->dirty.s.rotationDegKey = 1;
            }
            rel = 0;
        }
        else
        {
            gcmTRACE(gcvLEVEL_ERROR,
                     "Invalid roation degree value: %d",
                     value);
        }
        break;

    case COPYBIT_PLANE_ALPHA:
        value = value > 0xFF ? 0xFF : value;
        value = value < 0 ? 0 : value;

        if (context->planeAlpha != value)
        {
            context->planeAlpha = value;
            context->dirty.s.alphaKey = 1;
        }
        rel = 0;
        break;

    case COPYBIT_DITHER:
        /* TODO: Does not support DITHER currently. */
        if ((value == COPYBIT_DISABLE)
        ||  (value == COPYBIT_ENABLE)
        )
        {
            if (context->dither != value)
            {
                context->dither = value;
                context->dirty.s.ditherKey = 1;
            }
            rel = 0;
        }
        else
        {
            gcmTRACE(gcvLEVEL_ERROR,
                     "Invalid dither enabling flag: %d",
                     value);
        }
        break;

    case COPYBIT_BLUR:
        if ((value == COPYBIT_DISABLE)
        ||  (value == COPYBIT_ENABLE)
        )
        {
            if (context->blur != value)
            {
                context->blur = value;
                context->dirty.s.blurKey = 1;
            }
            rel = 0;
        }
        else
        {
            gcmTRACE(gcvLEVEL_ERROR,
                     "Invalid blur enabling flag: %d",
                     value);
        }
        break;

    case COPYBIT_TRANSFORM:
        switch (value)
        {
        case COPYBIT_TRANSFORM_FLIP_H:
        case COPYBIT_TRANSFORM_FLIP_V:
        case COPYBIT_TRANSFORM_ROT_90:
        case COPYBIT_TRANSFORM_ROT_180:
        case COPYBIT_TRANSFORM_ROT_270:
        case 0:
            if (context->transform != value)
            {
                context->transform = value;
                context->dirty.s.transformKey = 1;
            }
            break;

        default:
            gcmTRACE(gcvLEVEL_ERROR,
                     "Invalid transform value: %d",
                     value);
            break;
        }
        break;

    default:
        gcmTRACE(gcvLEVEL_ERROR,
                 "Invalid parameter name: %d",
                 name);
        break;
    }

    return rel;
}

int
copybit_get(
    struct copybit_device_t * dev,
    int name
    )
{
    int rel = -EINVAL;
    gcsCOPYBIT_CONTEXT * context = _GetContext();

    if (context == gcvNULL || &context->device != dev)
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "Invalid copybit device!");

        return -EINVAL;
    }

    switch (name)
    {
    case COPYBIT_MINIFICATION_LIMIT:
        rel = 16;
        break;

    case COPYBIT_MAGNIFICATION_LIMIT:
        rel = 4;
        break ;

    case COPYBIT_SCALING_FRAC_BITS:
        rel = 16;
        break ;

    case COPYBIT_ROTATION_STEP_DEG:
        rel = 90;
        break;

    default :
        gcmTRACE(gcvLEVEL_ERROR,
                 "Invalid parameter name: %d",
                 name);
        break;
    }

    return rel;
}


int
copybit_blit(
struct copybit_device_t * dev,
    struct copybit_image_t const * dst,
    struct copybit_image_t const * src,
    struct copybit_region_t const * region
    )
{
    struct copybit_rect_t bounds =
    {
        0, 0, dst->w, dst->h
    };

    LOGV("Blit requested");

    return copybit_stretch(dev, dst, src, &bounds, &bounds, region);
}

int
copybit_stretch(
    struct copybit_device_t * dev,
    struct copybit_image_t const * dst,
    struct copybit_image_t const * src,
    struct copybit_rect_t const * dst_rect,
    struct copybit_rect_t const * src_rect,
    struct copybit_region_t const * region
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsCOPYBIT_CONTEXT * context = _GetContext();

    if (context == gcvNULL || &context->device != dev)
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "Invalid copybit device!");

        return -EINVAL;
    }

    LOGV("Stretch blit requested");

    if (context->hasPE20)
    {
        status = _StretchBlit(context,
                              dst,
                              src,
                              dst_rect,
                              src_rect,
                              region);
    }
    else
    {
        status = _StretchBlitPE1x(context,
                                  dst,
                                  src,
                                  dst_rect,
                                  src_rect,
                                  region);
    }

    if (gcmIS_SUCCESS(status))
    {
        return 0;
    }
    else
    {
        return -EINVAL;
    }
}

int
copybit_sync_engine(
    struct copybit_device_t * dev
    )
{
    gceSTATUS status;
    gcsCOPYBIT_CONTEXT * context = _GetContext();

    if (context == gcvNULL || &context->device != dev)
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "Invalid copybit device!");

        return -EINVAL;
    }

    do
    {
        gcmERR_BREAK(
            gco2D_Flush(context->engine));

        gcmERR_BREAK(
            gcoHAL_Commit(gcvNULL, gcvTRUE));
    }
    while (gcvFALSE);

    if (gcmIS_SUCCESS(status))
    {
        return 0;
    }
    else
    {
        return -EINVAL;
    }
}


int
copybit_device_open(
    const struct hw_module_t * module,
    const char * name,
    struct hw_device_t ** device
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcsCOPYBIT_CONTEXT * context = gcvNULL;

    LOGV("opening copybit device");

    gco2D engine = gcvNULL;

    if (strcmp(name, COPYBIT_HARDWARE_COPYBIT0))
    {
        return -EINVAL;
    }

    /* Try to get context from tls. */
    context = _GetContext();

    /* Return if already initialized. */
    if (context != gcvNULL)
    {
        /* Increament reference count. */
        context->reference++;

        *device = &context->device.common;
        return 0;
    }

    /* In case destructor does not work. */
    atexit(&_Finish);

    /* Initialize our state here. */
    context = (gcsCOPYBIT_CONTEXT *) malloc(sizeof (gcsCOPYBIT_CONTEXT));
    memset(context, 0, sizeof (gcsCOPYBIT_CONTEXT));

    /* Initialize variables. */
    context->device.common.tag     = HARDWARE_DEVICE_TAG;
    context->device.common.version = 0;
    context->device.common.module  = const_cast<hw_module_t *>(module);

    /* Initialize procs. */
    context->device.common.close  = copybit_device_close;
    context->device.set_parameter = copybit_set_parameter;
    context->device.get           = copybit_get;
    context->device.blit          = copybit_blit;
    context->device.stretch       = copybit_stretch;

    /* Initialize GC stuff. */
    do
    {
        /* Get gco2D object pointer. */
        gcmERR_BREAK(
            gcoHAL_Get2DEngine(gcvNULL, &engine));

        /* Set the GC objects. */
        context->engine = engine;

        /* Initialize our private states. */
        context->rotationDeg = 0;
        context->planeAlpha  = 0xFF;
        context->transform   = 0;
        context->dither      = COPYBIT_DISABLE;
        context->blur        = COPYBIT_DISABLE;

        /* Initialize hash key. */
        context->dirty.i          = 1;
        context->dirty.s.dummyKey = 0;

        /* Initialize the GC state. */
        context->srcRotation = gcvSURF_0_DEGREE;
        context->dstRotation = gcvSURF_0_DEGREE;

        context->hasPE20 =
            gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_2DPE20);

        LOGV("PE 2.0 feature is available");

        context->perpixelAlpha = 0;
        context->needAlphaDest = 0;

        context->alphaDest.surface    = gcvNULL;
        context->alphaDest.physical   = ~0;
        context->alphaDest.logical    = gcvNULL;
        context->alphaDest.width      = 0;
        context->alphaDest.height     = 0;
        context->alphaDest.format     = gcvSURF_UNKNOWN;

#if gcdNEED_FILTER_BLIT
        context->tempSurface.surface  = gcvNULL;
        context->tempSurface.physical = ~0;
        context->tempSurface.logical  = gcvNULL;
        context->tempSurface.width    = 0;
        context->tempSurface.height   = 0;
        context->tempSurface.format   = gcvSURF_UNKNOWN;
#endif

        /* Increament reference count. */
        context->reference++;
        _SetContext(context);

        /* Return device handle. */
        *device = &context->device.common;

        return 0;
    }
    while (gcvFALSE);

    /* Error roll back. */
    if (context != gcvNULL)
    {
        free(context);
    }

    *device = NULL;

    return -EINVAL;
}


int
copybit_device_close(
    struct hw_device_t * dev
    )
{
    LOGV("closing copybit device");

    gcsCOPYBIT_CONTEXT * context = _GetContext();

    if (context == gcvNULL || &context->device.common != dev)
    {
        gcmTRACE(gcvLEVEL_ERROR,
                 "Invalid copybit device!");

        return -EINVAL;
    }

    if (--context->reference > 0)
    {
        return 0;
    }

    /* Try to free alphaDest. */
    _FreeSurface(&context->alphaDest);

#if gcdNEED_FILTER_BLIT
    /* Try to free tempSurface. */
    _FreeSurface(&context->tempSurface);
#endif

    /* Free filter buffer. */
    if (context->engine != gcvNULL)
    {
        gcmVERIFY_OK(
            gco2D_FreeFilterBuffer(context->engine));

        context->engine = gcvNULL;
    }

    /* Clean context. */
    free(context);
    _SetContext(gcvNULL);

    return 0;
}

