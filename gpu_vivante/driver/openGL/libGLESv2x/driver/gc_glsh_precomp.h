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


#ifndef __gc_glsh_precomp_h_
#define __gc_glsh_precomp_h_

#include <gc_hal.h>
#include <gc_hal_compiler.h>
#include <gc_hal_raster.h>
#include "gc_hal_user_math.h"
#include "gc_hal_user_compiler.h"

#if defined(_WIN32) && !defined(__SCITECH_SNAP__)
#define GL_APICALL __declspec(dllexport)
#else
#define GL_APICALL
#endif

#define GL_GLEXT_PROTOTYPES
#if defined(GC_USE_GL)
#	include <GL/gl.h>
#	include <GL/glext.h>
#else
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#endif

#include "gc_glsh_profiler.h"
#include "gc_glsh.h"

#endif /* __gc_glsh_precomp_h_ */
