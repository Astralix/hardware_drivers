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




#ifndef __gc_hal_user_qnx_h_
#define __gc_hal_user_qnx_h_

#define _QNX_SOURCE
#include "gc_hal_user.h"
#include "gc_hal_user_buffer.h"
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "../inc/gc_hal_common_qnx.h"

#if gcdENABLE_VG
#include "gc_hal_user_vg.h"
#endif

#define GC_HAL_QNX_PULSEVAL_SIGNAL  (_PULSE_CODE_MINAVAIL+1)

#endif /* __gc_hal_user_qnx_h_ */
