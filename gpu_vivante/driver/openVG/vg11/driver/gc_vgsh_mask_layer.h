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


#ifndef __gc_vgsh_mask_layer_h_
#define __gc_vgsh_mask_layer_h_


struct _VGMaskLayer
{
    _VGObject		object;
    _VGImage        image;
};

void _VGMaskLayerCtor(gcoOS os, _VGMaskLayer *maskLayer);
void _VGMaskLayerDtor(gcoOS os, _VGMaskLayer *maskLayer);
void initMaskLayer(_VGContext *context, _VGMaskLayer *maskLayer, gctINT32 width, gctINT32 height);
void CreateMaskBuffer(_VGContext	*context);

#endif /* __gc_vgsh_mask_layer_h_ */
