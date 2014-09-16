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




#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "gc_hwc.h"
#include "gc_hwc_debug.h"

#include <hardware/hardware.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/time.h>

static int
hwc_prepare(
    hwc_composer_device_t * dev,
    hwc_layer_list_t * list
    );

static int
hwc_set(
    hwc_composer_device_t * dev,
    hwc_display_t dpy,
    hwc_surface_t surf,
    hwc_layer_list_t * list
    );

static int
hwc_device_close(
    struct hw_device_t * dev
    );

static int
hwc_device_open(
    const struct hw_module_t * module,
    const char * name,
    struct hw_device_t ** device
    );


/******************************************************************************/

static struct hw_module_methods_t hwc_module_methods =
{
    open: hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM =
{
    common:
    {
        tag:           HARDWARE_MODULE_TAG,
        version_major: 2,
        version_minor: 0,
        id:            HWC_HARDWARE_MODULE_ID,
        name:          "Hardware Composer Module",
        author:        "Vivante Corporation",
        methods:       &hwc_module_methods,
        dso:           NULL,
        reserved:      {0, }
    }
};

static PFNEGLGETRENDERBUFFERANDROIDPROC _eglGetRenderBufferANDROID;


/******************************************************************************/

int
hwc_prepare(
    hwc_composer_device_t * dev,
    hwc_layer_list_t * list
    )
{
    hwcContext * context = (hwcContext *) dev;

    /* Check device handle. */
    if (context == gcvNULL)
    {
        LOGE("%s(%d): Invalid device!", __FUNCTION__, __LINE__);
        return HWC_EGL_ERROR;
    }

    /* Check layer count. */
    if ((list == NULL)
    ||  (list->numHwLayers == 0)
    )
    {
        return 0;
    }

    if (gcmIS_ERROR(hwcPrepare(context, list)))
    {
        LOGE("%s(%d): Failed in prepare", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    return 0;
}


int
hwc_set(
    hwc_composer_device_t * dev,
    hwc_display_t dpy,
    hwc_surface_t surf,
    hwc_layer_list_t * list
    )
{
    gceSTATUS status;
    hwcContext * context = (hwcContext *) dev;
    android_native_buffer_t * backBuffer = NULL;

    /* Check device handle. */
    if (context == gcvNULL)
    {
        LOGE("%s(%d): Invalid device!", __FUNCTION__, __LINE__);
        return HWC_EGL_ERROR;
    }

    /* Check layer count. */
    if ((list == NULL)
    ||  (list->numHwLayers == 0)
    )
    {
        /* No layer to set. */
        return 0;
    }

    if (context->hasComposition)
    {
        /* Get back buffer for hwc composition. */
        backBuffer = (android_native_buffer_t *)
           _eglGetRenderBufferANDROID((EGLDisplay) dpy, (EGLSurface) surf);

        if (backBuffer == NULL)
        {
            LOGE("%s(%d): Failed to get back buffer", __FUNCTION__, __LINE__);
            return -EINVAL;
        }

        /* Set cureent hardware type to 2D. */
        if (context->separated2D)
        {
            gcoHAL_SetHardwareType(context->hal, gcvHARDWARE_2D);
        }
    }

#if DUMP_SET_TIME
    struct timeval last;
    struct timeval curr;
    long   expired;

    gettimeofday(&last, NULL);
#endif

    /* Start hwc set. */
    gcmONERROR(
        hwcSet(context, backBuffer, list));

    /* Swap buffers. */
    eglSwapBuffers((EGLDisplay) dpy, (EGLSurface) surf);

    /* Commit and stall. */
    gcmVERIFY_OK(
        gcoHAL_Commit(context->hal, gcvTRUE));

#if DUMP_SET_TIME
    gettimeofday(&curr, NULL);

    expired = (curr.tv_sec - last.tv_sec) * 1000
            + (curr.tv_usec - last.tv_usec) / 1000;

    LOGD("Set %d layer(s): %ld ms", list->numHwLayers, expired);
#endif

    /* Reset hardware type. */
    if (context->separated2D && backBuffer != NULL)
    {
        gcoHAL_SetHardwareType(context->hal, gcvHARDWARE_3D);
    }

    return 0;

OnError:
    LOGE("%s(%d): Failed in set", __FUNCTION__, __LINE__);

    if (backBuffer != NULL)
    {
        /* Must call eglSwapBuffers to queue buffer. */
        eglSwapBuffers((EGLDisplay) dpy, (EGLSurface) surf);

        /* Reset hardware type. */
        if (context->separated2D)
        {
            gcoHAL_SetHardwareType(context->hal, gcvHARDWARE_3D);
        }
    }

    return -EINVAL;
}


int
hwc_device_close(
    struct hw_device_t *dev
    )
{
    hwcContext * context = (hwcContext *) dev;

    /* Check device. */
    if (context == NULL)
    {
        LOGE("%s(%d): Invalid device!", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    /* Free filter buffer. */
    gcmVERIFY_OK(
        gco2D_FreeFilterBuffer(context->engine));

    /* Destroy hal object. */
    gcmVERIFY_OK(
        gcoHAL_Destroy(context->hal));

    /* Destroy os object. */
    gcmVERIFY_OK(
        gcoOS_Destroy(context->os));

    /* TODO: Free allocated memory. */

    /* Clean context. */
    free(context);

    return 0;
}


int
hwc_device_open(
    const struct hw_module_t * module,
    const char * name,
    struct hw_device_t ** device
    )
{
    gceSTATUS  status    = gcvSTATUS_OK;
    hwcContext * context = gcvNULL;

    LOGV("%s(%d): Open hwc device in thread=%d",
         __FUNCTION__, __LINE__, gettid());

    if (strcmp(name, HWC_HARDWARE_COMPOSER) != 0)
    {
        LOGE("%s(%d): Invalid device name!", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    /* TODO: Find a better way instead of EGL_ANDROID_get_render_buffer. */
    _eglGetRenderBufferANDROID = (PFNEGLGETRENDERBUFFERANDROIDPROC)
                                 eglGetProcAddress("eglGetRenderBufferANDROID");

    if (_eglGetRenderBufferANDROID == NULL)
    {
        LOGE("EGL_ANDROID_get_render_buffer extension "
             "not found for hwcomposer");

        return HWC_EGL_ERROR;
    }

    /* Allocate memory for context. */
    context = (hwcContext *) malloc(sizeof (hwcContext));
    memset(context, 0, sizeof (hwcContext));

    /* Initialize variables. */
    context->device.common.tag     = HARDWARE_DEVICE_TAG;
    context->device.common.version = 0;
    context->device.common.module  = (hw_module_t *) module;

    /* initialize the procs */
    context->device.common.close   = hwc_device_close;
    context->device.prepare        = hwc_prepare;
    context->device.set            = hwc_set;

    /* Initialize GC stuff. */
    /* Construct os object. */
    gcmONERROR(
        gcoOS_Construct(gcvNULL, &context->os));

    /* Construct hal object. */
    gcmONERROR(
        gcoHAL_Construct(gcvNULL, context->os, &context->hal));

    /* Query separated 2D/3D cores? */
    context->separated2D =
        gcoHAL_QuerySeparated3D2D(context->hal) == gcvSTATUS_TRUE;

    /* Switch to 2D core if has separated 2D/3D cores. */
    if (context->separated2D)
    {
        gcmONERROR(
            gcoHAL_SetHardwareType(context->hal, gcvHARDWARE_2D));
    }

    /* Check 2D pipe existance. */
    if (!gcoHAL_IsFeatureAvailable(context->hal, gcvFEATURE_PIPE_2D))
    {
        LOGE("%s(%d): 2D PIPE not found", __FUNCTION__, __LINE__);
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Get gco2D object pointer. */
    gcmONERROR(
        gcoHAL_Get2DEngine(context->hal, &context->engine));

    /* Check GPU PE 2.0 feature. */
    context->pe20 =
        gcoHAL_IsFeatureAvailable(context->hal, gcvFEATURE_2DPE20);

    /* TODO: PE1.x path. */
    if (!context->pe20)
    {
        LOGE("%s(%d): PE20 not supported", __FUNCTION__, __LINE__);
        gcmONERROR(gcvSTATUS_NOT_SUPPORTED);
    }

    /* Check multi-source blit feature. */
    context->multiSourceBlt =
        gcoHAL_IsFeatureAvailable(context->hal,
                                  gcvFEATURE_2D_MULTI_SOURCE_BLT);

    context->multiSourceBltEx =
        gcoHAL_IsFeatureAvailable(context->hal,
                                  gcvFEATURE_2D_MULTI_SOURCE_BLT_EX);

    /* Compute max source limit. */
    context->maxSource = context->multiSourceBltEx ? 8
                       : 4;

    /* Check One patch filter blt/YUV blit/tiling input feature. */
    context->opf =
        gcoHAL_IsFeatureAvailable(context->hal, gcvFEATURE_2D_TILING);

    /* Switch back to 3D core. */
    if (context->separated2D)
    {
        gcmONERROR(
            gcoHAL_SetHardwareType(context->hal, gcvHARDWARE_3D));
    }

    /* Return device handle. */
    *device = &context->device.common;

    /* Print infomation. */
    LOGI("Vivante HWComposer v%d.%d\n"
         "Device:               %p\n"
         "Separated 2D/3D:      %s\n"
         "2D PE20:              %s\n"
         "Multi-source blit:    %s\n"
         "Multi-source blit Ex: %s\n"
         "OPF/YUV blit/Tiling : %s\n",
         HMI.common.version_major,
         HMI.common.version_minor,
         (void *) context,
         (context->separated2D      ? "YES" : "NO"),
         (context->pe20             ? "YES" : "NO"),
         (context->multiSourceBlt   ? "YES" : "NO"),
         (context->multiSourceBltEx ? "YES" : "NO"),
         (context->opf              ? "YES" : "NO"));

    return 0;

OnError:
    /* Error roll back. */
    if (context != gcvNULL)
    {
        if (context->hal != gcvNULL)
        {
            gcmVERIFY_OK(
                gcoHAL_Destroy(context->hal));

            gcmVERIFY_OK(
                gcoOS_Destroy(context->os));

            free(context);
        }
    }

    *device = NULL;

    LOGE("%s(%d): Failed to initialize hwcomposer!", __FUNCTION__, __LINE__);

    return -EINVAL;
}

