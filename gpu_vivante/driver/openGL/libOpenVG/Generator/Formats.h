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


#ifndef __FORMATS_H__
#define __FORMATS_H__

typedef struct _vgsCHANNEL * vgsCHANNEL_PTR;
typedef struct _vgsCHANNEL
{
	gctUINT		width;
	gctUINT		start;
	gctUINT		mask;
}
vgsCHANNEL;

typedef struct _vgsFORMAT_INFO * vgsFORMAT_INFO_PTR;
typedef struct _vgsFORMAT_INFO
{
	gctSTRING	name;
	gctSTRING	internalFormat;
	gctSTRING	upsampledFormat;
	gctBOOL		nativeFormat;
	gctUINT		bitsPerPixel;
	gctBOOL		premultiplied;
	gctBOOL		linear;
	vgsCHANNEL	r;
	vgsCHANNEL	g;
	vgsCHANNEL	b;
	vgsCHANNEL	a;
	vgsCHANNEL	l;
}
vgsFORMAT_INFO;

typedef struct _vgsFORMAT_LIST * vgsFORMAT_LIST_PTR;
typedef struct _vgsFORMAT_LIST
{
	vgsFORMAT_INFO_PTR	group;
	gctUINT				count;
}
vgsFORMAT_LIST;

gctFLOAT vgfGetColorGamma(
	gctFLOAT Color
	);

gctFLOAT vgfGetColorInverseGamma(
	gctFLOAT Color
	);

gctFLOAT vgfGetGrayScale(
	gctFLOAT Red,
	gctFLOAT Green,
	gctFLOAT Blue
	);

void vgfGetFormatGroupList(
	vgsFORMAT_LIST_PTR * List,
	gctUINT_PTR Count
	);

void vgfGetUpsampledFormatList(
	vgsFORMAT_LIST_PTR * List
	);

#endif
