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




#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <stdint.h>
#include <limits.h>
#include <sys/cdefs.h>
#include <hardware/gralloc.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include <cutils/native_handle.h>
#include <utils/Log.h>

#include <linux/fb.h>


struct private_module_t;
struct private_handle_t;
struct gc_private_handle_t;

struct private_module_t
{
    gralloc_module_t base;

    struct gc_private_handle_t* framebuffer;
    uint32_t flags;
    uint32_t numBuffers;
    uint32_t bufferMask;
    pthread_mutex_t lock;
    buffer_handle_t currentBuffer;

    struct fb_var_screeninfo info;
    struct fb_fix_screeninfo finfo;
    float xdpi;
    float ydpi;
    float fps;
};


#ifdef __cplusplus
struct private_handle_t : public native_handle {

    enum {
        PRIV_FLAGS_FRAMEBUFFER = 0x00000001
    };

#else
struct private_handle_t {
    native_handle_t nativeHandle;
#endif

    /* file-descriptors */
    int     fd;
    /* ints */
    int     magic;
    int     flags;
    int     size;
    int     offset;

    int     base;
    int     pid;

#ifdef __cplusplus
    static const int sNumInts = 6;
    static const int sNumFds = 1;
    static const int sMagic = 0x3141592;

    private_handle_t(int fd, int size, int flags) :
        fd(fd),
        magic(sMagic),
        flags(flags),
        size(size),
        offset(0),
        base(0),
        pid(getpid())
    {
        version = sizeof(native_handle);
        numInts = sNumInts;
        numFds = sNumFds;
    }

    ~private_handle_t()
    {
        magic = 0;
    }

    static int validate(const native_handle* h)
    {
        const private_handle_t* hnd = (const private_handle_t*)h;

        if (!h || h->version != sizeof(native_handle) ||
                h->numInts != sNumInts || h->numFds != sNumFds ||
                hnd->magic != sMagic)
        {
            LOGE("invalid gralloc handle (at %p)", h);
            return -EINVAL;
        }
        return 0;
    }
#endif
};

#endif /* GRALLOC_PRIV_H_ */

