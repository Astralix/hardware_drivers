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


#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "gc_gralloc_priv.h"
#include "gc_gralloc_gr.h"


/*****************************************************************************/

struct gralloc_context_t
{
    alloc_device_t device;
};


/*****************************************************************************/

extern int fb_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device);

static int gralloc_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device);

static int gralloc_register_buffer(gralloc_module_t const* module,
        buffer_handle_t handle);

static int gralloc_unregister_buffer(gralloc_module_t const* module,
        buffer_handle_t handle);

static int gralloc_lock(gralloc_module_t const* module,
        buffer_handle_t handle, int usage,
        int l, int t, int w, int h,
        void** vaddr);

static int gralloc_unlock(gralloc_module_t const* module,
        buffer_handle_t handle);

static int gralloc_perform(struct gralloc_module_t const* module,
        int operation, ... );


/*****************************************************************************/

static struct hw_module_methods_t gralloc_module_methods =
{
    open: gralloc_device_open
};

struct private_module_t HAL_MODULE_INFO_SYM =
{
    base:
    {
        common:
        {
            tag: HARDWARE_MODULE_TAG,
            version_major: 1,
            version_minor: 0,
            id: GRALLOC_HARDWARE_MODULE_ID,
            name: "Graphics Memory Allocator Module",
            author: "Vivante Corporation",
            methods: &gralloc_module_methods
        },

        registerBuffer: gralloc_register_buffer,
        unregisterBuffer: gralloc_unregister_buffer,
        lock: gralloc_lock,
        unlock: gralloc_unlock,
        perform: gralloc_perform,
    },

    framebuffer: 0,
    flags: 0,
    numBuffers: 0,
    bufferMask: 0,
    lock: PTHREAD_MUTEX_INITIALIZER,
    currentBuffer: 0,
};


/*****************************************************************************/

static int gralloc_alloc(struct alloc_device_t* dev,
        int w, int h, int format, int usage,
        buffer_handle_t* handle, int* stride)
{
    int rel = gc_gralloc_alloc(dev, w, h, format, usage, handle, stride);

    return rel;
}

static int gralloc_free(struct alloc_device_t* dev,
        buffer_handle_t handle)

{
    int rel = gc_gralloc_free(dev, handle);

    return rel;
}

static int gralloc_close(struct hw_device_t * dev)
{
    gralloc_context_t * ctx =
        reinterpret_cast<gralloc_context_t *>(dev);

    if (ctx != NULL)
    {
        free(ctx);
    }

    return 0;
}

int gralloc_device_open(const hw_module_t* module, const char* name,
        hw_device_t** device)
{
    int status = -EINVAL;

    if (!strcmp(name, GRALLOC_HARDWARE_GPU0))
    {
        /* Open alloc device. */
        gralloc_context_t * dev;
        dev = (gralloc_context_t *) malloc(sizeof (*dev));

        /* Initialize our state here */
        memset(dev, 0, sizeof (*dev));

        /* initialize the procs */
        dev->device.common.tag     = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module  = const_cast<hw_module_t *>(module);
        dev->device.common.close   = gralloc_close;
        dev->device.alloc          = gralloc_alloc;
        dev->device.free           = gralloc_free;

        *device = &dev->device.common;
        status = 0;
    }
    else
    {
        /* Open framebuffer device. */
        status = fb_device_open(module, name, device);
    }

    return status;
}


/*****************************************************************************/

int gralloc_register_buffer(gralloc_module_t const* module,
        buffer_handle_t handle)
{
    int rel = gc_gralloc_register_buffer(module, handle);

    return rel;
}

int gralloc_unregister_buffer(gralloc_module_t const* module,
        buffer_handle_t handle)
{
    int rel = gc_gralloc_unregister_buffer(module, handle);

    return rel;
}

int gralloc_lock(gralloc_module_t const* module,
        buffer_handle_t handle, int usage,
        int l, int t, int w, int h,
        void** vaddr)
{
    int rel = gc_gralloc_lock(module, handle, usage, l, t, w, h, vaddr);

    return rel;
}

int gralloc_unlock(gralloc_module_t const* module,
        buffer_handle_t handle)
{
    int rel = gc_gralloc_unlock(module, handle);

    return rel;
}

int gralloc_perform(struct gralloc_module_t const* module,
        int operation, ... )
{
    return 0;
}

