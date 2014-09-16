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




#ifndef __gc_hwc_debug_h_
#define __gc_hwc_debug_h_

/*******************************************************************************
** Debug options.
*/


/*
    DUMP_LAYER

       Dump information of source layers.
*/
#define DUMP_LAYER          0

/*
    DUMP_BITMAP

       Dump source input into bitmap. Dump only when hwc compsoition.
*/
#define DUMP_BITMAP         0

/*
    DUMP_SPLIT_AREA

       Dump splited area information. Dump only when hwc compsoition.
*/
#define DUMP_SPLIT_AREA     0

/*
    DUMP_SWAP_AREA

       Dump swap area information (swap rectangle optimization).
       Dump only when hwc compsoition.
*/
#define DUMP_SWAP_AREA      0

/*
    DUMP_COMPOSE

       Dump 2D compose procedure.
       Dump only when hwc compsoition.
*/
#define DUMP_COMPOSE        0

/*
    DUMP_SET_TIME

        Dump set time of every frame.
 */
#define DUMP_SET_TIME       0


/******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void
hwcDumpLayer(
    IN hwc_layer_list_t * List
    );

void
hwcDumpBitmap(
    IN gctUINT32 Count,
    IN hwcLayer * Layers
    );

void
hwcDumpArea(
    IN hwcArea * Area
    );

#ifdef __cplusplus
}
#endif

#endif

