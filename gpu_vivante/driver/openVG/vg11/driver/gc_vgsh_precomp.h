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


#ifndef __gc_vgsh_precomp_h_
#define __gc_vgsh_precomp_h_

#include <gc_hal.h>
#include <gc_hal_compiler.h>
#include <gc_hal_raster.h>
#include <gc_hal_user_math.h>

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
#define OVG_DBGOUT
#endif

#define OPENVG_DLL_EXPORTS
#define VG_VGEXT_PROTOTYPES

#define USE_TA                  1
#define LOCAL_MEM_OPTIM         0
#define USE_FTA                 1
#define WALK600                 1
#define SPECIAL_1PX_STROKE      0
#define DISABLE_TILE_STATUS     0
#define vgvUSE_VAA              1
#define WALK_MG1                1
#define NO_STENCIL_VG           1
#define vgvUSE_SHADER_OPTIM     0
#define vgvUSE_PSC              1

#ifndef VIVANTE_PROFILER
#   define  VIVANTE_PROFILER    0
#endif

#include <VG/openvg.h>
#include <VG/vgext.h>
#include <VG/vgu.h>
#include <EGL/egl.h>

#if LOCAL_MEM_OPTIM
#include <gc_hal_mem.h>
#endif
#include "gc_vgsh_defs.h"
#include "gc_vgsh_math.h"
#include "gc_vgsh_matrix.h"
#include "gc_vgsh_profiler.h"
#include "gc_vgsh_color.h"
#include "gc_vgsh_hardware.h"
#include "gc_vgsh_shader.h"
#include "gc_vgsh_image.h"
#include "gc_vgsh_paint.h"
#include "gc_vgsh_font.h"
#include "gc_vgsh_tessellator.h"
#include "gc_vgsh_context.h"
#include "gc_vgsh_path.h"
#include "gc_vgsh_mask_layer.h"
#include "gc_vgsh_profiler.h"

#endif /* __gc_vgsh_precomp_h_ */
