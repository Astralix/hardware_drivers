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




#ifndef __gc_gralloc_gr_h_
#define __gc_gralloc_gr_h_

#include <stdint.h>
#include <asm/page.h>
#include <limits.h>
#include <sys/cdefs.h>
#include <hardware/gralloc.h>
#include <pthread.h>
#include <errno.h>
#include <cutils/native_handle.h>


struct private_module_t;
struct private_handle_t;

inline size_t
roundUpToPageSize(size_t x)
{
    return (x + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
}

int
mapFrameBufferLocked(
    struct private_module_t * Module
    );

int
gc_gralloc_map(
    gralloc_module_t const * Module,
    buffer_handle_t Handle,
    void ** Vaddr
    );

int
gc_gralloc_unmap(
    gralloc_module_t const * Module,
    buffer_handle_t Handle
    );

int
gc_gralloc_alloc(
    alloc_device_t * Dev,
    int Width,
    int Height,
    int Format,
    int Usage,
    buffer_handle_t * Handle,
    int * Stride
    );

int
gc_gralloc_free(
    alloc_device_t * Dev,
    buffer_handle_t Handle
    );

int
gc_gralloc_register_buffer(
    gralloc_module_t const * Module,
    buffer_handle_t Handle
    );

int
gc_gralloc_unregister_buffer(
    gralloc_module_t const * Module,
    buffer_handle_t Handle
    );

int
gc_gralloc_lock(
    gralloc_module_t const * Module,
    buffer_handle_t Handle,
    int Usage,
    int Left,
    int Top,
    int Width,
    int Height,
    void ** Vaddr
    );

int
gc_gralloc_unlock(
    gralloc_module_t const * Module,
    buffer_handle_t Handle
    );

int
gc_gralloc_perform(
    gralloc_module_t const * Module,
    int Operation,
    ...
    );

#endif /* __gc_gralloc_gr_h_ */

