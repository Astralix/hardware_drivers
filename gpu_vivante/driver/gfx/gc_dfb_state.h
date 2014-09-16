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


/**
 * \file gc_dfb_state.h
 */

#ifndef __gc_dfb_state_h_
#define __gc_dfb_state_h_

#include <core/coretypes.h>
#include <gc_hal_types.h>

/**
 * Validation flags.
 */
typedef enum {
    HSVF_NOFX           = 0x00000000,
    HSVF_SOURCE         = 0x00000001,
    HSVF_TARGET         = 0x00000002,
    HSVF_CLIP           = 0x00000004,
    HSVF_BLEND          = 0x00000008,
    HSVF_MIRROR         = 0x00000010,
    HSVF_BRUSH          = 0x00000020,
    HSVF_SRC_CKEY_VALUE = 0x00000040,
    HSVF_DST_CKEY_VALUE = 0x00000080,
    HSVF_CKEY_MODE      = 0x00000100,
    HSVF_MULTIPLY       = 0x00000200,
    HSVF_COLOR          = 0x00000400,
    HSVF_ROTATION       = 0x00000800,
    HSVF_PALETTE        = 0x00001000
} HWStateValidateFlags;

void galCheckState( void                *drv,
                    void                *dev,
                    CardState           *state,
                    DFBAccelerationMask  accel );

void galSetState( void                *drv,
                  void                *dev,
                  GraphicsDeviceFuncs *funcs,
                  CardState           *state,
                  DFBAccelerationMask  accel );
#endif /* __gc_dfb_state_h_ */
