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




/*******************************************************************************
** Include standard libraries.
*/

#if !(defined(LINUX) || defined(__QNXNTO__))
#	define _CRT_SECURE_NO_WARNINGS
#	define WIN32_LEAN_AND_MEAN 1
#	include <windows.h>
#endif

#include <VG/openvg.h>
#include <VG/vgext.h>


/*******************************************************************************
** Inline macro definition.
*/

#if defined(LINUX)
#	define vgmINLINE
#else
#	define vgmINLINE __inline
#endif


/*******************************************************************************
** Forward declare pointers.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _vgsCONTEXT       * vgsCONTEXT_PTR;
typedef struct _vgsPAINT         * vgsPAINT_PTR;
typedef struct _vgsIMAGE_FORMAT  * vgsIMAGE_FORMAT_PTR;
typedef struct _vgsIMAGE         * vgsIMAGE_PTR;
typedef struct _vgsPATH          * vgsPATH_PTR;
typedef struct _vgsPATHSTORAGE   * vgsPATHSTORAGE_PTR;
typedef struct _vgsPATHWALKER    * vgsPATHWALKER_PTR;
typedef struct _vgsCONTAINER     * vgsCONTAINER_PTR;
typedef struct _vgsCONTROL_COORD * vgsCONTROL_COORD_PTR;

#ifdef __cplusplus
}
#endif


/*******************************************************************************
** Include the rest of the driver declarations.
*/

#include <math.h>
#include <gc_hal.h>

#if gcdENABLE_VG
#include <gc_hal_engine_vg.h>
#else
#include <gc_hal_vg.h>
#endif

#include "gc_vg_basic_defines.h"
#include "gc_vg_debug.h"
#include "gc_vg_format.h"
#include "gc_vg_state.h"
#include "gc_vg_object.h"
#include "gc_vg_memory_manager.h"
#include "gc_vg_paint.h"
#include "gc_vg_path_storage.h"
#include "gc_vg_path_coordinate.h"
#include "gc_vg_path_append.h"
#include "gc_vg_path_import.h"
#include "gc_vg_path_normalize.h"
#include "gc_vg_path_interpolate.h"
#include "gc_vg_path_transform.h"
#include "gc_vg_path_modify.h"
#include "gc_vg_path_walker.h"
#include "gc_vg_path_point_along.h"
#include "gc_vg_path.h"
#include "gc_vg_stroke.h"
#include "gc_vg_arc.h"
#include "gc_vg_image.h"
#include "gc_vg_mask.h"
#include "gc_vg_matrix.h"
#include "gc_vg_text.h"
#include "gc_vg_context.h"
#include "gc_vgu_library.h"
