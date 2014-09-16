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
 * \file gc_dfb_conf.h
 */

#ifndef __gc_dfb_conf_h_
#define __gc_dfb_conf_h_

#include <linux/limits.h>

#include <core/coretypes.h>
#include <directfb.h>

typedef enum   _GALPrimitive        GALPrimitive;
typedef struct _GALConfig           GALConfig;
typedef struct _GALPrimitiveSetting GALPrimitiveSetting;
typedef struct _GALRenderFlag       GALRenderFlag;

/*
 * We support the following primitives.
 *
 * DFXL_FILLRECTANGLE
 * DFXL_DRAWRECTANGLE
 * DFXL_DRAWLINE
 * DFXL_FILLTRIANGLE
 * DFXL_BLIT
 * DFXL_STRETCHBLIT
 */

/* Primitives number. */
#define MAX_PRIMITIVES 6

/* Flag numbers. */
#define MAX_DRAW_FLAG  2
#define MAX_BLIT_FLAG  7

enum _GALPrimitive {
    GAL_CONF_FILLRECTANGLE = 0,
    GAL_CONF_DRAWRECTANGLE = 1,
    GAL_CONF_DRAWLINE      = 2,
    GAL_CONF_FILLTRIANGLE  = 3,
    GAL_CONF_BLIT          = 4,
    GAL_CONF_STRETCHBLIT   = 5
};

struct _GALRenderFlag {
    char         name[PATH_MAX + 1];
    unsigned int value;
};

struct _GALPrimitiveSetting {
    char           name[PATH_MAX];

    int            flag_num;
    GALRenderFlag *render_flags;

    bool           none;
    unsigned int   flags;
};

struct _GALConfig {
    char filename[PATH_MAX + 1];

    int  draw_flag_num;
    int  blit_flag_num;

    GALRenderFlag draw_flags[MAX_DRAW_FLAG];
    GALRenderFlag blit_flags[MAX_BLIT_FLAG];

    GALPrimitiveSetting primitive_settings[MAX_PRIMITIVES];
};

DFBResult
gal_config_init( void *dev, GALConfig *config );

DFBResult
gal_config_read( GALConfig *config);

DFBResult
gal_config_set( GALConfig *config, const char *name, const char *value );

bool
gal_config_accelerated( GALConfig *config, CardState *state, DFBAccelerationMask accel );

#endif /* __gc_dfb_conf_h_ */
