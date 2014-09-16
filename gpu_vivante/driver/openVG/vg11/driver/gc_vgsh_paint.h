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


#ifndef __gc_vgsh_paint_h_
#define __gc_vgsh_paint_h_

#define RADIAL_GRADIENT_H        256
#define RADIAL_GRADIENT_W        256

#define LINEAR_GRADIENT_H        1
#define LINEAR_GRADIENT_W        256

typedef enum OVGPaintDirty
{
	OVGPaintDirty_None		= 0x0,
	OVGPaintDirty_ColorStop = 0x01,
	OVGPaintDirty_ColorRampPremultiplied = 0x02,
}OVGPaintDirty;

typedef struct _VGGradientStop
{
	VGfloat   offset;
   _VGColor   color;
}_VGGradientStop;

ARRAY_DEFINE(_VGGradientStop);

struct _VGPaint
{
	_VGObject               object;
	VGPaintType				paintType;
	_VGColor				paintColor;
	_VGColor				inputPaintColor;
	_VGColor                avgRampColor;
	VGColorRampSpreadMode	colorRampSpreadMode;
	_VGGradientStopArray	colorRampStops;
	_VGGradientStopArray	inputColorRampStops;
	VGboolean				colorRampPremultiplied;

	_VGTemplate             linearGradient[4];
	_VGTemplate             inputRadialGradient[5];
	_VGTemplate             radialGradient[5];

	VGTilingMode			patternTilingMode;
	_VGImage*				pattern;
    _VGImage               colorRamp;
    OVGPaintDirty           dirty;
    _VGMatrix3x3            paintToUser;
    _VGMatrix3x3            linearGraMatrix;
    _VGMatrix3x3            radialGraMatrix;
    gcoSURF                 texSurface;
	gctBOOL					zeroFlag;
	gctFLOAT                alphaValue;
	gctBOOL					alphaChanged;
	_VGfloatArray			colorStops;
	_VGVector2				udusq;
	gctFLOAT				radialC;
	_VGVector2				radialA;
	_VGVector2				radialB;
	gcoSTREAM				lineStream;
	_VGColorFormat			colorRampTextureForamt;
};


void _VGPaintCtor(gcoOS os, _VGPaint *paint);
void _VGPaintDtor(gcoOS os, _VGPaint *paint);

void genLinearGradient(_VGContext * context,  _VGPaint *paint);
void genRadialGradient(_VGContext * context, _VGPath *path ,_VGPaint *paint, _VGMatrix3x3* paintToUser);

void PaintDirty(_VGPaint *paint, OVGPaintDirty dirty);
void PaintClean(_VGPaint *paint);
gctBOOL IsPaintDirty(_VGPaint *paint);

#endif /* __gc_vgsh_paint_h_ */
