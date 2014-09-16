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




#include "gc_hwc.h"


/*******************************************************************************
**
**  hwcOverlay
**
**  Set all overlay layers.
**
**
**  INPUT:
**
**      hwcContext * Context
**          hwcomposer context pointer.
**
**      hwc_display_t Dpy
**          Specify EGLDisplay handle.
**
**      hwc_surface_t Surf
**          Specify EGLSurface handle.
**
**      hwc_layer_list_t * List
**          Specify the layer list. Please set proper overlap parameters
**          for layers which are tagged with HWC_OVERLAY as compositionType.
**
**  OUTPUT:
**
**      Nothing.
*/
int
hwcOverlay(
    IN hwcContext * Context,
    IN hwc_layer_list_t * List
    )
{
    /* TODO: Set overlay layers. */

    return 0;
}

