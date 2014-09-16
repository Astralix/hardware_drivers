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


#ifndef __gc_glsl_common_h_
#define __gc_glsl_common_h_

#include "gc_hal.h"
#include "gc_hal_user.h"
#include "gc_hal_driver.h"
#include "gc_hal_types.h"
#include "gc_hal_compiler.h"

typedef char *			gctCHAR_PTR;
typedef const char *	gctCONST_CHAR_PTR;

#include "gc_glsl_link_list.h"
#include "gc_glsl_hash_table.h"

#ifndef _GC_OBJ_ZONE
#define _GC_OBJ_ZONE	gcvZONE_COMPILER
#endif

#endif /* __gc_glsl_common_h_ */
