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




/*
 * OS specific functions.
 */

#ifndef __gc_egl_os_h_
#define __gc_egl_os_h_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN 1
#	include <windows.h>
#	include <tchar.h>
#elif defined(LINUX) || defined (__QNXNTO__)
#	include <dlfcn.h>
#	include <unistd.h>
	typedef char TCHAR;
#endif

#include <gc_egl.h>

#ifdef __cplusplus
}
#endif

#endif /* __gc_egl_os_h_ */
