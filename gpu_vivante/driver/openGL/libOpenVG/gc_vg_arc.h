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




#ifndef __gc_vg_arc_h_
#define __gc_vg_arc_h_

/*----------------------------------------------------------------------------*/
/*-------------------------- ARC input coordinates. --------------------------*/

typedef struct _vgsARCCOORDINATES * vgsARCCOORDINATES_PTR;
typedef struct _vgsARCCOORDINATES
{
	gctBOOL large;
	gctBOOL counterClockwise;
	gctFLOAT horRadius;
	gctFLOAT verRadius;
	gctFLOAT rotAngle;
	gctFLOAT endX;
	gctFLOAT endY;
}
vgsARCCOORDINATES;

/*----------------------------------------------------------------------------*/
/*--------------------------------- ARC API. ---------------------------------*/

gceSTATUS vgfConvertArc(
	vgsPATHWALKER_PTR Destination,
	gctFLOAT HorRadius,
	gctFLOAT VerRadius,
	gctFLOAT RotAngle,
	gctFLOAT EndX,
	gctFLOAT EndY,
	gctBOOL CounterClockwise,
	gctBOOL Large,
	gctBOOL Relative
	);

#endif
