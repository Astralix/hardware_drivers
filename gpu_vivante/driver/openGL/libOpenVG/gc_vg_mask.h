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




#ifndef __gc_vg_mask_h__
#define __gc_vg_mask_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*----------------------- Object structure definition. -----------------------*/

typedef struct _vgsMASK * vgsMASK_PTR;
typedef struct _vgsMASK
{
	/* Embedded object. */
	vgsOBJECT				object;

	/* Mask image. */
	vgsIMAGE				image;
}
vgsMASK;

/*----------------------------------------------------------------------------*/
/*------------------ Object management function declarations. ----------------*/

gceSTATUS vgfReferenceMask(
	vgsCONTEXT_PTR Context,
	vgsMASK_PTR * Mask
	);

void vgfSetMaskObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	);

#ifdef __cplusplus
}
#endif

#endif
