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




#ifndef __gc_vg_path_h__
#define __gc_vg_path_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*------------------------ Miscellaneous definitions. ------------------------*/

/* Container for the control coordinates. */
typedef struct _vgsCONTROL_COORD
{
	gctFLOAT	startX;
	gctFLOAT	startY;
	gctFLOAT	lastX;
	gctFLOAT	lastY;
	gctFLOAT	controlX;
	gctFLOAT	controlY;
}
vgsCONTROL_COORD;

/*----------------------------------------------------------------------------*/
/*----------------------- Object structure definition. -----------------------*/

typedef struct _vgsPATH
{
	/* Embedded object. */
	vgsOBJECT				object;

	/* Object states. */
	VGint					format;				/* VG_PATH_FORMAT */
	VGfloat					scale;				/* VG_PATH_SCALE */
	VGfloat					bias;				/* VG_PATH_BIAS */
	VGint					numSegments;		/* VG_PATH_NUM_SEGMENTS */
	VGint					numCoords;			/* VG_PATH_NUM_COORDS */

	/* Stroke optimizations. */
#if vgvSTROKE_COPY_OPTIMIZE
	gctFLOAT				strokeScale;
#endif

	/* Data type. */
	VGPathDatatype			datatype;			/* VG_PATH_DATATYPE */
	gcePATHTYPE				halDataType;

	/* Capabilities. */
	VGbitfield				capabilities;

	/* Path command buffer attributes. */
	gcsPATH_BUFFER_INFO		pathInfo;

	/* Not zero if path contains ARCs. */
	gctBOOL					hasArcs;

	/* Path data. */
	vgsPATH_DATA_PTR		head;
	vgsPATH_DATA_PTR		tail;

	/* Set/get coordinate fuctions. */
	vgtSETCOORDINATE *		setArray;
	vgtGETCOORDINATE *		getArray;
	vgtGETCOPYCOORDINATE *	getcopyArray;

	/* Stroke path. */
	gctBOOL					strokeValid;
	vgsPATH_DATA_PTR		stroke;

	/* Stroking parameters. */
	gctFLOAT				lineWidth;
	gctFLOAT				miterLimit;
	gceCAP_STYLE			capStyle;
	gceJOIN_STYLE			joinStyle;

	/* Dash parameters. */
	gctUINT					dashPatternCount;
	gctFLOAT				dashPattern[16];
	gctBOOL					dashPhaseReset;
	gctFLOAT				dashPhase;

	/* Current control coordinates. */
	vgsCONTROL_COORD		coords;

	/* Path number. */
	vgmDEFINE_PATH_COUNTER()
}
vgsPATH;

/*----------------------------------------------------------------------------*/
/*------------------ Object management function declarations. ----------------*/

gctUINT vgfGetPathDataSize(
	gcePATHTYPE Type
	);

gctUINT vgfGetSegmentDataCount(
	gceVGCMD Command
	);

gceSTATUS vgfReferencePath(
	vgsCONTEXT_PTR Context,
	vgsPATH_PTR * Path
	);

void vgfSetPathObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	);

void vgfFreePathDataCallback(
	IN vgsPATH_PTR Path,
	IN vgsPATH_DATA_PTR PathData
	);

gceSTATUS vgfDrawPath(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Target,
	IN vgsPATH_PTR Path,
	IN VGbitfield PaintModes,
	IN vgsPAINT_PTR FillPaint,
	IN vgsPAINT_PTR StrokePaint,
	IN gctBOOL ColorTransformEnable,
	IN gctBOOL Mask
	);

#ifdef __cplusplus
}
#endif

#endif
