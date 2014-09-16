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




#ifndef __gc_vg_paint_h__
#define __gc_vg_paint_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*-------------------------------- Paint types. ------------------------------*/

/* Paint update function type. */
typedef gceSTATUS (* vgtUPDATEPAINT) (
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	);

/*----------------------------------------------------------------------------*/
/*----------------------- Object structure definition. -----------------------*/

typedef struct _vgsPAINT
{
	/* Embedded object. */
	vgsOBJECT				object;

	/* VG_PAINT_TYPE */
	VGPaintType				type;

	/* VG_PAINT_TYPE: VG_PAINT_TYPE_COLOR */
	vgtFLOATVECTOR4			floatColor;
	vgtBYTEVECTOR4			byteColor;

	/* VG_PAINT_TYPE: VG_PAINT_TYPE_LINEAR_GRADIENT */
	vgtFLOATVECTOR4			linearGradient;

	/* VG_PAINT_TYPE: VG_PAINT_TYPE_RADIAL_GRADIENT */
	vgtFLOATVECTOR5			radialGradient;

	/* Color ramp parameters for gradient paints provided to the driver. */
	gctSIZE_T				vgColorRampLength;
	gcsCOLOR_RAMP			vgColorRamp[vgvMAX_COLOR_RAMP_STOPS];

	/* Converted internal color ramp. */
	gctBOOL					intColorRampRecompute;
	gctSIZE_T				intColorRampLength;
	gcsCOLOR_RAMP			intColorRamp[vgvMAX_COLOR_RAMP_STOPS + 2];

	/* Color ramp surface. */
	gctBOOL					colorRampSurfaceRecompute;
	gcoSURF					colorRampSurface;
	gctUINT8_PTR			colorRampBits;

	/* VG_PAINT_COLOR_RAMP_PREMULTIPLIED */
	VGboolean				colorRampPremultiplied;

	/* VG_PAINT_COLOR_RAMP_SPREAD_MODE */
	VGColorRampSpreadMode	colorRampSpreadMode;
	gceTILE_MODE			halColorRampSpreadMode;

	/* VG_PAINT_PATTERN_TILING_MODE */
	VGTilingMode			patternTilingMode;
	gceTILE_MODE			halPatternTilingMode;

	/* Pattern image. */
	vgsIMAGE_PTR			pattern;
	gctFLOAT				patternWidth;
	gctFLOAT				patternHeight;

	/* Linear gradient paint parameters. */
	gctBOOL					lgRecompute;
	gctFLOAT				lgConstantLin;
	gctFLOAT				lgStepXLin;
	gctFLOAT				lgStepYLin;

	/* Radial gradient paint parameters. */
	gctBOOL					rgRecompute;
	gctFLOAT				rgConstantLin;
	gctFLOAT				rgStepXLin;
	gctFLOAT				rgStepYLin;
	gctFLOAT				rgConstantRad;
	gctFLOAT				rgStepXRad;
	gctFLOAT				rgStepYRad;
	gctFLOAT				rgStepXXRad;
	gctFLOAT				rgStepYYRad;
	gctFLOAT				rgStepXYRad;

	/* Pattern paint parameters. */
	gctBOOL					patRecompute;
	gctFLOAT				patConstantLin;
	gctFLOAT				patStepXLin;
	gctFLOAT				patStepYLin;
	gctFLOAT				patConstantRad;
	gctFLOAT				patStepXRad;
	gctFLOAT				patStepYRad;

	/* Current surface-to-paint matrix. */
	vgsMATRIX_PTR			matrix;

	/* Current dirty flag and update function. */
	vgtUPDATEPAINT			update;
}
vgsPAINT;

/*----------------------------------------------------------------------------*/
/*------------------ Object management function declarations. ----------------*/

gceSTATUS vgfReferencePaint(
	IN vgsCONTEXT_PTR Context,
	IN OUT vgsPAINT_PTR * Paint
	);

void vgfSetPaintObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	);

gceSTATUS vgfUpdatePaint(
	IN vgsCONTEXT_PTR Context,
	IN vgsMATRIXCONTAINER_PTR Matrix,
	IN vgsPAINT_PTR Paint
	);

void vgfPaintMatrixChanged(
	IN vgsPAINT_PTR Paint,
	IN vgsMATRIX_PTR SurfaceToPaint
	);

void vgfPaintMatrixVerifyCurrent(
	IN vgsPAINT_PTR Paint,
	IN vgsMATRIX_PTR SurfaceToPaint
	);

gcmINLINE  gctFIXED vgfFloatToFixed(
	IN gctFLOAT r
	);

gcmINLINE  gctFLOAT vgfFixedToFloat(
	IN gctFIXED r
	);

gcmINLINE  gctFIXED vgfFixedMul(
        IN gctFIXED r1,
        IN gctFIXED r2
);

#ifdef __cplusplus
}
#endif

#endif
