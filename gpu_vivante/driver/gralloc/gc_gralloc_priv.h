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




#ifndef __gc_gralloc_priv_h_
#define __gc_gralloc_priv_h_

#include "gralloc_priv.h"
#include "gc_hal.h"
#include "gc_hal_driver.h"


#define MAGIC_VIV gcmCC('G','C','G','R')


#ifdef __cplusplus
struct gc_private_handle_t : public private_handle_t
{
#else
struct gc_private_handle_t
{
    struct private_handle_t common;
#endif

    int     phys;
    int     width;
    int     height;
    int     format;

    int     surface;
    int     vidNode;
    int     pool;
    int     adjustedSize;

    int     resolveSurface;
    int     resolveVidNode;
    int     resolvePool;
    int     resolveAdjustedSize;

    int     hwDoneSignal;

    int     lockUsage;
    int     allocUsage;
    int     clientPID;

    int     dirtyX;
    int     dirtyY;
    int     dirtyWidth;
    int     dirtyHeight;

#ifdef __cplusplus

    static const int sNumInts = 26;
    static const int sNumFds  = 1;
    static const int sMagic   = MAGIC_VIV;

    gc_private_handle_t(int fd, int size, int flags) :
        private_handle_t(fd, size, flags),

        phys(0),
        width(0),
        height(0),
        format(0),

        surface(0),
        vidNode(0),
        pool(0),
        adjustedSize(0),

        resolveSurface(0),
        resolveVidNode(0),
        resolvePool(0),
        resolveAdjustedSize(0),

        hwDoneSignal(0),

        lockUsage(0),
        allocUsage(0),
        clientPID(0),

        dirtyX(0),
        dirtyY(0),
        dirtyWidth(0),
        dirtyHeight(0)
    {
        magic   = sMagic;
        version = sizeof(native_handle);
        numInts = sNumInts;
        numFds  = sNumFds;
    }

    ~gc_private_handle_t()
    {
        magic = 0;
    }

    static int
    validate(
        const native_handle * Handle
        )
    {
        const gc_private_handle_t* hnd = (const gc_private_handle_t *) Handle;

        if (!Handle)
        {
            LOGE("Invalid gralloc handle: NULL");

            return -EINVAL;
        }

        if ((Handle->version != sizeof(native_handle))
        ||  (Handle->numInts != sNumInts)
        ||  (Handle->numFds != sNumFds)
        ||  (hnd->magic != sMagic)
        )
        {
            LOGE("Invalid buffer handle (at %p), "
                 "version=%d, "
                 "sizeof (native_handle)=%d, "
                 "numInts=%d, "
                 "sNumInts=%d, "
                 "numFds=%d, "
                 "sNumFds=%d, "
                 "hnd->magic=%d, "
                 "sMagic=%d",
                 Handle,
                 Handle->version,
                 sizeof (native_handle),
                 Handle->numInts,
                 sNumInts,
                 Handle->numFds,
                 sNumFds,
                 hnd->magic,
                 sMagic);

            return -EINVAL;
        }

        return 0;
    }
#endif
};

#endif /* __gc_gralloc_priv_h_ */

