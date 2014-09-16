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




#ifndef __gc_vg_stroke_h__
#define __gc_vg_stroke_h__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _vgsSTROKECONVERSION * vgsSTROKECONVERSION_PTR;

gceSTATUS vgfUpdateStroke(
	IN vgsCONTEXT_PTR Context,
	IN vgsPATH_PTR Path
	);

gceSTATUS vgfDestroyStrokeConversion(
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	);

#ifdef __cplusplus
}
#endif

#endif
