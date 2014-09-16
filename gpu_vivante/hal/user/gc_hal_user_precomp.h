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




#ifndef __gc_hal_user_precomp_h__
#define __gc_hal_user_precomp_h__

#if !defined(HAL_EXPORTS)
#   define HAL_EXPORTS
#endif

#include "gc_hal_user.h"
#include "gc_hal_user_math.h"

#if gcdENABLE_VG
#include "gc_hal_user_vg.h"
#endif

#ifndef VIVANTE_NO_3D
#   include "gc_hal_compiler.h"
#   include "gc_hal_user_compiler.h"
#   include "gc_hal_optimizer.h"
#endif

#include "gc_hal_profiler.h"

/* Workaround GCC optimizer's bug */
#if defined(__GNUC__) && !gcmIS_DEBUG(gcdDEBUG_TRACE)
    gcmINLINE static void
    __do_nothing(
        IN gctUINT32 Level,
        IN gctUINT32 Zone,
        IN gctCONST_STRING Message,
        ...
        )
    {
        static volatile int c;

        c++;
    }

#   undef  gcmTRACE_ZONE
#   define gcmTRACE_ZONE            __do_nothing
#endif

#endif /* __gc_hal_user_precomp_h__ */
