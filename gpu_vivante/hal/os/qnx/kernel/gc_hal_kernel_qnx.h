/****************************************************************************
*
*    Copyright (C) 2005 - 2012 by Vivante Corp.
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the license, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****************************************************************************/




#ifndef __gc_hal_kernel_qnx_h_
#define __gc_hal_kernel_qnx_h_

#define _QNX_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/procmgr.h>
#include <sys/memmsg.h>
#include <fcntl.h>
#include <hw/inout.h>
#include <atomic.h>

#define NTSTRSAFE_NO_CCH_FUNCTIONS
#include "gc_hal.h"
#include "gc_hal_driver.h"
#include "gc_hal_kernel.h"
#include "gc_hal_kernel_device.h"
#include "gc_hal_kernel_os.h"
#include "gc_hal_kernel_buffer.h"
#include "../inc/gc_hal_common_qnx.h"

#define _WIDE(string)               L##string
#define WIDE(string)                _WIDE(string)

#define countof(a)                  (sizeof(a) / sizeof(a[0]))

#ifndef GAL_DEV
#define GAL_DEV "/dev/galcore"
#endif

struct _gcskTHREAD_USER_DATA
{
    gctUINT32 Pid;          /* Process ID. */
    gctUINT32 Tid;          /* Thread ID. */
} ;

typedef struct _gcskTHREAD_USER_DATA gcskTHREAD_USER_DATA;

#endif /* __gc_hal_kernel_qnx_h_ */
