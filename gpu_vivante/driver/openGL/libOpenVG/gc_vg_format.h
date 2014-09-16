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




#ifndef __gc_vg_format_h__
#define __gc_vg_format_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*-------------------------- Pixel walker structure. -------------------------*/

#define VG_lRGB_565_VIV (VG_A_4 + 1)

typedef struct _vgsPIXELWALKER * vgsPIXELWALKER_PTR;
typedef struct _vgsPIXELWALKER
{
	gctINT					stride;
	gctUINT8_PTR			line;
	gctUINT					initialBitOffset;
	gctUINT8_PTR			current;
	gctUINT					bitOffset;
}
vgsPIXELWALKER;


/*----------------------------------------------------------------------------*/
/*----------------------- Pixel read/write routine types. --------------------*/

typedef void (* vgtREAD_PIXEL) (
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	);

typedef void (* vgtWRITE_PIXEL) (
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	);


/*----------------------------------------------------------------------------*/
/*------------------------ Image information structure. ----------------------*/

typedef struct _vgsFORMAT * vgsFORMAT_PTR;
typedef struct _vgsFORMAT
{
	gctUINT					tableIndex;
	gctUINT					formatIndex;

	gceSURF_FORMAT			internalFormat;
	vgsFORMAT_PTR			upsampledFormat;
	gctUINT					bitsPerPixel;

	gctBOOL					nativeFormat;
	gctBOOL					premultiplied;
	gctBOOL					linear;
	gctBOOL					grayscale;
	gctBOOL					luminance;

	gcsFORMAT_COMPONENT		r;
	gcsFORMAT_COMPONENT		g;
	gcsFORMAT_COMPONENT		b;
	gcsFORMAT_COMPONENT		a;

	vgtREAD_PIXEL			readPixel[4];
	vgtWRITE_PIXEL			writePixel[4 * 16];
}
vgsFORMAT;


/*----------------------------------------------------------------------------*/
/*--------------------------- Pixel format routines. -------------------------*/

VGImageFormat vgfGetOpenVGFormat(
	gceSURF_FORMAT Format,
	gctBOOL Linear,
	gctBOOL Premultiplied
	);

vgsFORMAT_PTR vgfGetFormatInfo(
	VGImageFormat Format
	);


/*----------------------------------------------------------------------------*/
/*---------------------------- Pixel math routines. --------------------------*/

void vgfClampColor(
	vgtFLOATVECTOR4 Source,
	vgtFLOATVECTOR4 Target,
	gctBOOL Premultiplied
	);

gctFLOAT vgfGetGrayScale(
	gctFLOAT Red,
	gctFLOAT Green,
	gctFLOAT Blue
	);

gctFLOAT vgfGetColorGamma(
	gctFLOAT Color
	);

gctFLOAT vgfGetColorInverseGamma(
	gctFLOAT Color
	);

void vgfConvertColor(
	vgtFLOATVECTOR4 Source,
	vgtFLOATVECTOR4 Target,
	gctBOOL TargetPremultiplied,
	gctBOOL TargetLinear,
	gctBOOL TargetGrayscale
	);


/*----------------------------------------------------------------------------*/
/*--------------------------- Pixel walker routines. -------------------------*/

void vgsPIXELWALKER_Initialize(
	vgsPIXELWALKER_PTR Walker,
	vgsIMAGE_PTR Image,
	gctINT X,
	gctINT Y
	);

void vgsPIXELWALKER_NextLine(
	vgsPIXELWALKER_PTR Walker
	);

#ifdef __cplusplus
}
#endif

#endif
