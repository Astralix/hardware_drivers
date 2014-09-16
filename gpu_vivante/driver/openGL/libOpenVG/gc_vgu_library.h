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




#ifndef __gc_vgu_library_h__
#define __gc_vgu_library_h__

#ifdef __cplusplus
extern "C" {
#endif

gctBOOL vgfNormalizeCoordinates(
	IN OUT gctINT_PTR X,
	IN OUT gctINT_PTR Y,
	IN OUT gctINT_PTR Width,
	IN OUT gctINT_PTR Height,
	IN gcsSIZE_PTR SurfaceSize
	);

gctBOOL vgfNormalizeCoordinatePairs(
	IN OUT gctINT_PTR SourceX,
	IN OUT gctINT_PTR SourceY,
	IN OUT gctINT_PTR TargetX,
	IN OUT gctINT_PTR TargetY,
	IN OUT gctINT_PTR Width,
	IN OUT gctINT_PTR Height,
	IN gcsSIZE_PTR SourceSize,
	IN gcsSIZE_PTR TargetSize
	);

#ifdef __cplusplus
}
#endif

#endif
