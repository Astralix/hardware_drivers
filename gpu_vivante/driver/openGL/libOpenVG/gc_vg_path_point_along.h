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




#ifndef __gc_vg_path_point_along_h__
#define __gc_vg_path_point_along_h__

#ifdef __cplusplus
extern "C" {
#endif

gctBOOL vgfComputePointAlongPath(
	IN vgsCONTEXT_PTR Context,
	IN vgsPATH_PTR Path,
	IN gctUINT StartSegment,
	IN gctUINT NumSegments,
	IN gctFLOAT Distance,
	OUT gctFLOAT_PTR X,
	OUT gctFLOAT_PTR Y,
	OUT gctFLOAT_PTR TangentX,
	OUT gctFLOAT_PTR TangentY,
	OUT gctFLOAT_PTR Length,
	OUT gctFLOAT_PTR PathLeft,
	OUT gctFLOAT_PTR PathTop,
	OUT gctFLOAT_PTR PathRight,
	OUT gctFLOAT_PTR PathBottom
	);

#ifdef __cplusplus
}
#endif

#endif
