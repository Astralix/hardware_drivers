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




/**
**	@file
**	Architecture independent capability query functions.
**
*/

#include "gc_hal_user_precomp.h"

#define _GC_OBJ_ZONE		gcvZONE_SURFACE

gcsFORMAT_COMPONENT gcvPIXEL_COMP_XXX8 = {  0, 8 };
gcsFORMAT_COMPONENT gcvPIXEL_COMP_XX8X = {  8, 8 };
gcsFORMAT_COMPONENT gcvPIXEL_COMP_X8XX = { 16, 8 };
gcsFORMAT_COMPONENT gcvPIXEL_COMP_8XXX = { 24, 8 };

static struct _gcsSURF_FORMAT_INFO FormatInfoArray[] =
{
	/*
		Palettized formats (gcsFORMAT_CLASS_TYPE_INDEX).
	*/
	{gcvSURF_INDEX1, gcvFORMAT_CLASS_INDEX, 1, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			{0, 1}
		}}
	},
	{gcvSURF_INDEX4, gcvFORMAT_CLASS_INDEX, 4, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			{0, 4}
		}}
	},
	{gcvSURF_INDEX8, gcvFORMAT_CLASS_INDEX, 8, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			{0, 8}
		}}
	},

	/*
		RGB formats (gcsFORMAT_CLASS_TYPE_RGBA).
	*/
	{gcvSURF_A2R2G2B2, gcvFORMAT_CLASS_RGBA, 8, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */  {6,  2},
			/* R */  {4,  2},
			/* G */  {2,  2},
			/* B */  {0,  2}
		}}
	},
	{gcvSURF_R3G3B2, gcvFORMAT_CLASS_RGBA, 8, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */  {5,  3},
			/* G */  {2,  3},
			/* B */  {0,  2}
		}}
	},
	{gcvSURF_A8R3G3B2, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */  {8,  8},
			/* R */  {5,  3},
			/* G */  {2,  3},
			/* B */  {0,  2}
		}}
	},
	{gcvSURF_X4R4G4B4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {12,  4 | gcvCOMPONENT_DONTCARE},
			/* R */  {8,  4},
			/* G */  {4,  4},
			/* B */  {0,  4}
		}}
	},
	{gcvSURF_A4R4G4B4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {12,  4},
			/* R */  {8,  4},
			/* G */  {4,  4},
			/* B */  {0,  4}
		}}
	},
	{gcvSURF_R4G4B4A4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_RGBA, gcvFALSE,
		{{
			/* A */  {0,  4},
			/* R */ {12,  4},
			/* G */  {8,  4},
			/* B */  {4,  4}
		}}
	},
	{gcvSURF_X1R5G5B5, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {15,  1 | gcvCOMPONENT_DONTCARE},
			/* R */ {10,  5},
			/* G */  {5,  5},
			/* B */  {0,  5}
		}}
	},
	{gcvSURF_A1R5G5B5, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {15,  1},
			/* R */ {10,  5},
			/* G */  {5,  5},
			/* B */  {0,  5}
		}}
	},
	{gcvSURF_R5G5B5A1, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_RGBA, gcvFALSE,
		{{
			/* A */  {0,  1},
			/* R */ {11,  5},
			/* G */  {6,  5},
			/* B */  {1,  5},
		}}
	},
	{gcvSURF_R5G6B5, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */ {11,  5},
			/* G */  {5,  6},
			/* B */  {0,  5}
		}}
	},
	{gcvSURF_R8G8B8, gcvFORMAT_CLASS_RGBA, 24, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */ {16,  8},
			/* G */  {8,  8},
			/* B */  {0,  8}
		}}
	},
	{gcvSURF_X8R8G8B8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {24,  8 | gcvCOMPONENT_DONTCARE},
			/* R */ {16,  8},
			/* G */  {8,  8},
			/* B */  {0,  8}
		}}
	},
	{gcvSURF_R8G8B8X8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_RGBA, gcvFALSE,
		{{
			/* A */  {0,  8 | gcvCOMPONENT_DONTCARE},
			/* R */ {24,  8},
			/* G */ {16,  8},
			/* B */  {8,  8}
		}}
	},
	{gcvSURF_A8R8G8B8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {24,  8},
			/* R */ {16,  8},
			/* G */  {8,  8},
			/* B */  {0,  8}
		}}
	},
	{gcvSURF_R8G8B8A8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_RGBA, gcvFALSE,
		{{
			/* A */  {0,  8},
			/* R */ {24,  8},
			/* G */ {16,  8},
			/* B */  {8,  8}
		}}
	},
	{gcvSURF_G8R8G8B8, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ARGB, gcvTRUE,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */ {16,  8},
			/* G */  {8,  8},
			/* B */  {0,  8}
		}}
	},
	{gcvSURF_G8R8G8B8, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ARGB, gcvTRUE | gcvCOMPONENT_ODD,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */ {16,  8},
			/* G */ {24,  8},
			/* B */  {0,  8}
		}}
	},
	{gcvSURF_R8G8B8G8, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_RGBA, gcvTRUE,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */ {24,  8},
			/* G */  {0,  8},
			/* B */  {8,  8}
		}}
	},
	{gcvSURF_R8G8B8G8, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_RGBA, gcvTRUE | gcvCOMPONENT_ODD,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */ {24,  8},
			/* G */ {16,  8},
			/* B */  {8,  8}
		}}
	},
	{gcvSURF_X2R10G10B10, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {30,  2 | gcvCOMPONENT_DONTCARE},
			/* R */ {20, 10},
			/* G */ {10, 10},
			/* B */  {0, 10}
		}}
	},
	{gcvSURF_A2R10G10B10, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {30,  2},
			/* R */ {20, 10},
			/* G */ {10, 10},
			/* B */  {0, 10}
		}}
	},
	{gcvSURF_X12R12G12B12, gcvFORMAT_CLASS_RGBA, 48, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {36, 12 | gcvCOMPONENT_DONTCARE},
			/* R */ {24, 12},
			/* G */ {12, 12},
			/* B */  {0, 12}
		}}
	},
	{gcvSURF_A12R12G12B12, gcvFORMAT_CLASS_RGBA, 48, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {36, 12},
			/* R */ {24, 12},
			/* G */ {12, 12},
			/* B */  {0, 12}
		}}
	},
	{gcvSURF_X16R16G16B16, gcvFORMAT_CLASS_RGBA, 64, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {48, 16 | gcvCOMPONENT_DONTCARE},
			/* R */ {32, 16},
			/* G */ {16, 16},
			/* B */  {0, 16}
		}}
	},
	{gcvSURF_A16R16G16B16, gcvFORMAT_CLASS_RGBA, 64, gcvSURF_ARGB, gcvFALSE,
		{{
			/* A */ {48, 16},
			/* R */ {32, 16},
			/* G */ {16, 16},
			/* B */  {0, 16}
		}}
	},
	{gcvSURF_R5G5B5X1, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_RGBA, gcvFALSE,
		{{
			/* A */ { 0,  1 | gcvCOMPONENT_DONTCARE},
			/* R */ {11,  5},
			/* G */  {6,  5},
			/* B */  {1,  5}
		}}
	},
	{gcvSURF_R4G4B4X4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_RGBA, gcvFALSE,
		{{
			/* A */ { 0,  4 | gcvCOMPONENT_DONTCARE},
			/* R */ {12,  4},
			/* G */  {8,  4},
			/* B */  {4,  4}
		}}
	},
	{gcvSURF_R8G8B8X8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_RGBA, gcvFALSE,
		{{
			/* A */ { 0,  8 | gcvCOMPONENT_DONTCARE},
			/* R */ {24,  8},
			/* G */ {16,  8},
			/* B */  {8,  8}
		}}
	},

	/*
		BGR formats (gcsFORMAT_CLASS_TYPE_RGBA).
	*/
	{gcvSURF_A4B4G4R4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */ {12,  4},
			/* R */  {0,  4},
			/* G */  {4,  4},
			/* B */  {8,  4}
		}}
	},
	{gcvSURF_B4G4R4A4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */  {0,  4},
			/* R */  {4,  4},
			/* G */  {8,  4},
			/* B */ {12,  4}
		}}
	},
	{gcvSURF_A1B5G5R5, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */ {15,  1},
			/* R */  {0,  5},
			/* G */  {5,  5},
			/* B */ {10,  5}
		}}
	},
	{gcvSURF_B5G5R5A1, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */  {0,  1},
			/* R */  {1,  5},
			/* G */  {6,  5},
			/* B */ {11,  5}
		}}
	},
	{gcvSURF_B5G6R5, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */  {0,  5},
			/* G */  {5,  6},
			/* B */ {11,  5}
		}}
	},
	{gcvSURF_B8G8R8, gcvFORMAT_CLASS_RGBA, 24, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */  {0,  8},
			/* G */  {8,  8},
			/* B */ {16,  8}
		}}
	},
	{gcvSURF_X8B8G8R8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */ {24,  8 | gcvCOMPONENT_DONTCARE},
			/* R */  {0,  8},
			/* G */  {8,  8},
			/* B */ {16,  8}
		}}
	},
	{gcvSURF_B8G8R8X8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */  {0,  8 | gcvCOMPONENT_DONTCARE},
			/* R */  {8,  8},
			/* G */ {16,  8},
			/* B */ {24,  8}
		}}
	},
	{gcvSURF_A8B8G8R8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */ {24,  8},
			/* R */  {0,  8},
			/* G */  {8,  8},
			/* B */ {16,  8}
		}}
	},
	{gcvSURF_B8G8R8A8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */  {0,  8},
			/* R */  {8,  8},
			/* G */ {16,  8},
			/* B */ {24,  8}
		}}
	},
	{gcvSURF_A2B10G10R10, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */ {30,  2},
			/* R */  {0, 10},
			/* G */ {10, 10},
			/* B */ {20, 10}
		}}
	},
	{gcvSURF_A16B16G16R16, gcvFORMAT_CLASS_RGBA, 64, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */ {48, 16},
			/* R */  {0, 16},
			/* G */ {16, 16},
			/* B */ {32, 16}
		}}
	},
	{gcvSURF_G16R16, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* A */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* R */  {0, 16},
			/* G */ {16, 16},
			/* B */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_B4G4R4A4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */ { 0,  4},
			/* R */ { 4,  4},
			/* G */ { 8,  4},
			/* B */ {12,  4}
		}}
	},
	{gcvSURF_B5G5R5A1, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */ { 0,  1},
			/* R */ { 1,  5},
			/* G */ { 6,  5},
			/* B */ {11,  5}
		}}
	},
	{gcvSURF_B8G8R8X8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */ { 0,  8 | gcvCOMPONENT_DONTCARE},
			/* R */  {8,  8},
			/* G */ {16,  8},
			/* B */ {24,  8}
		}}
	},
	{gcvSURF_B8G8R8A8, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */ { 0,  8},
			/* R */  {8,  8},
			/* G */ {16,  8},
			/* B */ {24,  8}
		}}
	},
	{gcvSURF_X4B4G4R4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */ {12,  4 | gcvCOMPONENT_DONTCARE},
			/* R */  {0,  4},
			/* G */  {4,  4},
			/* B */  {8,  4}
		}}
	},
	{gcvSURF_X1B5G5R5, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_ABGR, gcvFALSE,
		{{
			/* A */ {15,  1 | gcvCOMPONENT_DONTCARE},
			/* R */  {0,  5},
			/* G */  {5,  5},
			/* B */ {10,  5}
		}}
	},
	{gcvSURF_B4G4R4X4, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */  {0,  4 | gcvCOMPONENT_DONTCARE},
			/* R */  {4,  4},
			/* G */  {8,  4},
			/* B */ {12,  4}
		}}
	},
	{gcvSURF_B5G5R5X1, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_BGRA, gcvFALSE,
		{{
			/* A */  {0,  1 | gcvCOMPONENT_DONTCARE},
			/* R */  {1,  5},
			/* G */  {6,  5},
			/* B */ {11,  5}
		}}
	},

	/*
		YUV formats (gcsFORMAT_CLASS_TYPE_YUV).
	*/
	{gcvSURF_YUY2, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvTRUE,
		{{
			/* Y */  {0,  8},
			/* U */  {8,  8},
			/* V */ {24,  8}
		}}
	},
	{gcvSURF_YUY2, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvTRUE | gcvCOMPONENT_ODD,
		{{
			/* Y */ {16,  8},
			/* U */  {8,  8},
			/* V */ {24,  8}
		}}
	},
	{gcvSURF_UYVY, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvTRUE,
		{{
			/* Y */  {8,  8},
			/* U */  {0,  8},
			/* V */ {16,  8}
		}}
	},
	{gcvSURF_UYVY, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvTRUE | gcvCOMPONENT_ODD,
		{{
			/* Y */ {24,  8},
			/* U */  {0,  8},
			/* V */ {16,  8}
		}}
	},
	{gcvSURF_YV12, gcvFORMAT_CLASS_YUV, 12, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Y */ {0,  8},
			/* U */ {0,  8},
			/* V */ {0,  8}
		}}
	},
	{gcvSURF_I420, gcvFORMAT_CLASS_YUV, 12, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Y */ {0,  8},
			/* U */ {0,  8},
			/* V */ {0,  8}
		}}
	},
	{gcvSURF_NV12, gcvFORMAT_CLASS_YUV, 12, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Y */ {0,  8},
			/* U */ {0,  8},
			/* V */ {0,  8}
		}}
	},
	{gcvSURF_NV21, gcvFORMAT_CLASS_YUV, 12, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Y */ {0,  8},
			/* U */ {0,  8},
			/* V */ {0,  8}
		}}
	},
	{gcvSURF_NV16, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Y */ {0,  8},
			/* U */ {0,  8},
			/* V */ {0,  8}
		}}
	},
	{gcvSURF_NV61, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Y */ {0,  8},
			/* U */ {0,  8},
			/* V */ {0,  8}
		}}
	},
	{gcvSURF_YVYU, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvTRUE,
		{{
			/* Y */  {0,  8},
			/* U */	{24,  8},
			/* V */  {8,  8}
		}}
	},
	{gcvSURF_YVYU, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvTRUE | gcvCOMPONENT_ODD,
		{{
			/* Y */ {16,  8},
			/* U */	{24,  8},
			/* V */  {8,  8}
		}}
	},
	{gcvSURF_VYUY, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvTRUE,
		{{
			/* Y */  {8,  8},
			/* U */ {16,  8},
			/* V */  {0,  8}
		}}
	},
	{gcvSURF_VYUY, gcvFORMAT_CLASS_YUV, 16, gcvSURF_NOSWIZZLE, gcvTRUE | gcvCOMPONENT_ODD,
		{{
			/* Y */ {24,  8},
			/* U */ {16,  8},
			/* V */  {0,  8}
		}}
	},

	/*
		Alpha formats (gcsFORMAT_CLASS_TYPE_RGBA).
	*/
	{gcvSURF_A1, gcvFORMAT_CLASS_RGBA, 1, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* A */  {0,  1},
			/* R */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* G */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* B */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_A4, gcvFORMAT_CLASS_RGBA, 4, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* A */  {0,  4},
			/* R */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* G */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* B */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_A8, gcvFORMAT_CLASS_RGBA, 8, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* A */  {0,  8},
			/* R */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* G */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* B */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_A12, gcvFORMAT_CLASS_RGBA, 12, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* A */  {0, 12},
			/* R */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* G */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* B */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_A16, gcvFORMAT_CLASS_RGBA, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* A */  {0, 16},
			/* R */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* G */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* B */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_A32, gcvFORMAT_CLASS_RGBA, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* A */  {0, 32},
			/* R */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* G */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* B */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},

	/*
		Luminance formats (gcsFORMAT_CLASS_TYPE_LUMINANCE).
	*/
	{gcvSURF_L1, gcvFORMAT_CLASS_LUMINANCE, 1, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* Lum   */  {0,  1}
		}}
	},
	{gcvSURF_L4, gcvFORMAT_CLASS_LUMINANCE, 4, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* Lum   */  {0,  4}
		}}
	},
	{gcvSURF_L8, gcvFORMAT_CLASS_LUMINANCE, 8, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* Lum   */  {0,  8}
		}}
	},
	{gcvSURF_L12, gcvFORMAT_CLASS_LUMINANCE, 12, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* Lum   */  {0, 12}
		}}
	},
	{gcvSURF_L16, gcvFORMAT_CLASS_LUMINANCE, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* Lum   */  {0, 16}
		}}
	},
	{gcvSURF_L32, gcvFORMAT_CLASS_LUMINANCE, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* Lum   */  {0, 32}
		}}
	},
	{gcvSURF_A4L4, gcvFORMAT_CLASS_LUMINANCE, 8, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {4,  4},
			/* Lum   */  {0,  4}
		}}
	},
	{gcvSURF_A2L6, gcvFORMAT_CLASS_LUMINANCE, 8, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {6,  2},
			/* Lum   */  {0,  6}
		}}
	},
	{gcvSURF_A8L8, gcvFORMAT_CLASS_LUMINANCE, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {8,  8},
			/* Lum   */  {0,  8}
		}}
	},
	{gcvSURF_A4L12, gcvFORMAT_CLASS_LUMINANCE, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */ {12,  4},
			/* Lum   */  {0, 12}
		}}
	},
	{gcvSURF_A12L12, gcvFORMAT_CLASS_LUMINANCE, 24, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */ {12, 12},
			/* Lum   */  {0, 12}
		}}
	},
	{gcvSURF_A16L16, gcvFORMAT_CLASS_LUMINANCE, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */ {16, 16},
			/* Lum   */  {0, 16}
		}}
	},

	/*
		Bump formats (gcsFORMAT_CLASS_TYPE_BUMP).
	*/
	{gcvSURF_L6V5U5, gcvFORMAT_CLASS_LUMINANCE, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* L     */ {10,  6},
			/* V     */  {5,  5},
			/* U     */  {0,  5},
			/* Q     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* W     */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_V8U8, gcvFORMAT_CLASS_LUMINANCE, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* L     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* V     */  {8,  8},
			/* U     */  {0,  8},
			/* Q     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* W     */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_X8L8V8U8, gcvFORMAT_CLASS_LUMINANCE, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */ {24,  8 | gcvCOMPONENT_DONTCARE},
			/* L     */ {16,  8},
			/* V     */  {8,  8},
			/* U     */  {0,  8},
			/* Q     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* W     */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_Q8W8V8U8, gcvFORMAT_CLASS_LUMINANCE, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* L     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* V     */  {8,  8},
			/* U     */  {0,  8},
			/* Q     */ {24,  8},
			/* W     */ {16,  8}
		}}
	},
	{gcvSURF_A2W10V10U10, gcvFORMAT_CLASS_LUMINANCE, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */ {30,  2},
			/* L     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* V     */ {10, 10},
			/* U     */  {0, 10},
			/* Q     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* W     */ {20, 10}
		}}
	},
	{gcvSURF_V16U16, gcvFORMAT_CLASS_LUMINANCE, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* L     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* V     */ {16, 16},
			/* U     */  {0, 16},
			/* Q     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* W     */  {0,  gcvCOMPONENT_NOTPRESENT}
		}}
	},
	{gcvSURF_Q16W16V16U16, gcvFORMAT_CLASS_LUMINANCE, 64, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Alpha */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* L     */  {0,  gcvCOMPONENT_NOTPRESENT},
			/* V     */ {16, 16},
			/* U     */  {0, 16},
			/* Q     */ {48, 16},
			/* W     */ {32, 16}
		}}
	},

	/*
		Depth and stencil formats (gcsFORMAT_CLASS_TYPE_DEPTH).
	*/
	{ gcvSURF_D16, gcvFORMAT_CLASS_DEPTH, 16, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Depth   */ { 0, 16                      },
			/* Stencil */ { 0, gcvCOMPONENT_NOTPRESENT },
		}}
	},
	{ gcvSURF_D24S8, gcvFORMAT_CLASS_DEPTH, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Depth   */ { 8, 24 },
			/* Stencil */ { 0,  8 },
		}}
	},
	{ gcvSURF_D32, gcvFORMAT_CLASS_DEPTH, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Depth   */ { 0, 32 },
			/* Stencil */ { 0, gcvCOMPONENT_NOTPRESENT },
		}}
	},
	{ gcvSURF_D24X8, gcvFORMAT_CLASS_DEPTH, 32, gcvSURF_NOSWIZZLE, gcvFALSE,
		{{
			/* Depth   */ { 8, 24 },
			/* Stencil */ { 0,  gcvCOMPONENT_NOTPRESENT },
		}}
	},
};

/*******************************************************************************
**
**	gcoSURF_QueryFormat
**
**	Return pixel format parameters.
**
**	INPUT:
**
**		gceSURF_FORMAT Format
**			API format.
**
**	OUTPUT:
**
**		gcsSURF_FORMAT_INFO_PTR * Info
**			Pointer to a variable that will hold the format description entry.
**			If the format in question is interleaved, two pointers will be
**			returned stored in an array fation.
**
*/
gceSTATUS
gcoSURF_QueryFormat(
	IN gceSURF_FORMAT Format,
	OUT gcsSURF_FORMAT_INFO_PTR * Info
	)
{
	gctUINT i;
	gceSTATUS status = gcvSTATUS_OK;

	gcmHEADER_ARG("Format=%d", Format);

	/* Verify the arguments. */
	gcmDEBUG_VERIFY_ARGUMENT(Info != gcvNULL);

	/* Find the format in the format table. */
	for (i = 0; i < gcmCOUNTOF(FormatInfoArray); i++)
	{
		if (FormatInfoArray[i].format == Format)
		{
			/* Set the output to the current entry. */
			Info[0] = &FormatInfoArray[i];
			Info[1] = (FormatInfoArray[i].interleaved && ((i + 1) < gcmCOUNTOF(FormatInfoArray)))
				? &FormatInfoArray[i + 1]
				: gcvNULL;

			/* Success. */
			gcmFOOTER_ARG("Info=0x%x,0x%x", Info[0], Info[1]);
			return gcvSTATUS_OK;
		}

		if (FormatInfoArray[i].interleaved)
		{
			i++;
		}
	}

	/* Failed to find the entry. */
	status = gcvSTATUS_NOT_SUPPORTED;
	gcmFOOTER();
	return status;
}

/*******************************************************************************
**
**	gcoSURF_ComputeColorMask
**
**	Computes the color pixel mask; for RGB formats alpha channel is not
**	included in the mask.
**
**	INPUT:
**
**		gcsSURF_FORMAT_INFO_PTR Format
**			Pointer to the format information structure.
**
**	OUTPUT:
**
**		gctUINT32_PTR ColorMask
**			Pixel color mask.
*/
gceSTATUS
gcoSURF_ComputeColorMask(
	IN gcsSURF_FORMAT_INFO_PTR Format,
	OUT gctUINT32_PTR ColorMask
	)
{
	gcmHEADER_ARG("Format=0x%x", Format);

	/* Verify the arguments. */
	gcmDEBUG_VERIFY_ARGUMENT(Format != gcvNULL);
	gcmDEBUG_VERIFY_ARGUMENT(ColorMask != gcvNULL);

	/* Commpute source pixel mask. */
	if (Format[0].fmtClass == gcvFORMAT_CLASS_RGBA)
	{
		/* Reset the mask. */
		*ColorMask = 0;

		/* Add components. */
		if (Format[0].u.rgba.red.width)
		{
			*ColorMask
				|= ((1 << Format[0].u.rgba.red.width) - 1)
				<< Format[0].u.rgba.red.start;
		}

		if (Format[0].u.rgba.green.width)
		{
			*ColorMask
				|= ((1 << Format[0].u.rgba.green.width) - 1)
				<< Format[0].u.rgba.green.start;
		}

		if (Format[0].u.rgba.blue.width)
		{
			*ColorMask
				|= ((1 << Format[0].u.rgba.blue.width) - 1)
				<< Format[0].u.rgba.blue.start;
		}
	}
	else
	{
		*ColorMask = (1 << Format[0].bitsPerPixel) - 1;
	}

	/* Success. */
	gcmFOOTER_ARG("*ColorMask=%08x", *ColorMask);
	return gcvSTATUS_OK;
}

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE		gcvZONE_HAL

/*******************************************************************************
**
**  gcoHAL_QueryChipIdentity
**
**  Query the identity of the hardware.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gceCHIPMODEL* ChipModel
**          If 'ChipModel' is not gcvNULL, the variable it points to will
**          receive the model of the chip.
**
**      gctUINT32* ChipRevision
**          If 'ChipRevision' is not gcvNULL, the variable it points to will
**          receive the revision of the chip.
**
**      gctUINT32* ChipFeatures
**          If 'ChipFeatures' is not gcvNULL, the variable it points to will
**          receive the feature set of the chip.
**
**      gctUINT32 * ChipMinorFeatures
**          If 'ChipMinorFeatures' is not gcvNULL, the variable it points to
**          will receive the minor feature set of the chip.
**
*/
gceSTATUS gcoHAL_QueryChipIdentity(
    IN gcoHAL Hal,
    OUT gceCHIPMODEL* ChipModel,
    OUT gctUINT32* ChipRevision,
    OUT gctUINT32* ChipFeatures,
    OUT gctUINT32* ChipMinorFeatures
    )
{
    gceSTATUS status;
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentHW;
#endif

    gcmHEADER();
#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentHW);
    if (currentHW == gcvHARDWARE_VG)
    {
        /* Query identity from hardware object. */
        status = gcoVGHARDWARE_QueryChipIdentity(gcvNULL,
                                               ChipModel,
                                               ChipRevision,
                                               ChipFeatures,
                                               ChipMinorFeatures,
                                               gcvNULL);
    }
    else
#endif
    {
        /* Query identity from hardware object. */
        status = gcoHARDWARE_QueryChipIdentity(
                                           ChipModel,
                                           ChipRevision,
                                           ChipFeatures,
                                           ChipMinorFeatures,
                                           gcvNULL,
                                           gcvNULL);
    }

    /* Return status. */
    gcmFOOTER_ARG("status=%d *ChipModel=%d *ChipRevision=%x *ChipFeatures=%08x "
                  "*ChipMinorFeatures=%08x", status, gcmOPT_VALUE(ChipModel),
                  gcmOPT_VALUE(ChipRevision), gcmOPT_VALUE(ChipFeatures),
                  gcmOPT_VALUE(ChipMinorFeatures));
    return status;
}

/*******************************************************************************
**
**  gcoHAL_QueryChipMinorFeatures
**
**  Query the minor features of the hardware.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gctUINT32* NumFeatures
**          If 'NumFeatures' is not gcvNULL, the variable it points to will
**          receive the count of minor features.
**
**      gctUINT32 * ChipMinorFeatures
**          If 'ChipMinorFeatures' is not gcvNULL, the variable it points to
**          will receive all the minor feature sets of the chip. This address
**          should be able to store NumFeatures count of features.
**
*/
gceSTATUS gcoHAL_QueryChipMinorFeatures(
    IN gcoHAL Hal,
    OUT gctUINT32* NumFeatures,
    OUT gctUINT32* ChipMinorFeatures
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    if (NumFeatures != gcvNULL)
    {
        *NumFeatures = 3;
    }

    if (ChipMinorFeatures != gcvNULL)
    {
        /* Query identity from hardware object. */
        status = gcoHARDWARE_QueryChipIdentity(gcvNULL,
                                               gcvNULL,
                                               gcvNULL,
                                               ChipMinorFeatures,
                                               (ChipMinorFeatures + 1),
                                               (ChipMinorFeatures + 2));
        /* Return status. */
        if (status != gcvSTATUS_OK)
        {
            gcmFOOTER();
        }
        else
        {
            gcmFOOTER_ARG("status=%d *NumFeatures=%08x *ChipMinorFeatures=%08x *ChipMinorFeatures1=%08x "
                          "*ChipMinorFeatures2=%08x", status, gcmOPT_VALUE(NumFeatures),
                          gcmOPT_VALUE(ChipMinorFeatures), gcmOPT_VALUE(ChipMinorFeatures + 1),
                          gcmOPT_VALUE(ChipMinorFeatures + 2));
        }
    }
    else
    {
        /* Return status. */
        gcmFOOTER();
    }
    return status;
}

/*******************************************************************************
**
**  gcoHAL_QueryTiled
**
**  Query the tile sizes.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gctINT32 * TileWidth2D
**          Pointer to a variable receiving the width in pixels per 2D tile.  If
**          the 2D is working in linear space, the width will be 1.  If there is
**          no 2D, the width will be 0.
**
**      gctINT32 * TileHeight2D
**          Pointer to a variable receiving the height in pixels per 2D tile.
**          If the 2D is working in linear space, the height will be 1.  If
**          there is no 2D, the height will be 0.
**
**      gctINT32 * TileWidth3D
**          Pointer to a variable receiving the width in pixels per 3D tile.  If
**          the 3D is working in linear space, the width will be 1.  If there is
**          no 3D, the width will be 0.
**
**      gctINT32 * TileHeight3D
**          Pointer to a variable receiving the height in pixels per 3D tile.
**          If the 3D is working in linear space, the height will be 1.  If
**          there is no 3D, the height will be 0.
*/
gceSTATUS
gcoHAL_QueryTiled(
    IN gcoHAL Hal,
    OUT gctINT32 * TileWidth2D,
    OUT gctINT32 * TileHeight2D,
    OUT gctINT32 * TileWidth3D,
    OUT gctINT32 * TileHeight3D
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Query the tile sizes through gcoHARDWARE. */
    status = gcoHARDWARE_QueryTileSize(TileWidth2D, TileHeight2D,
                                       TileWidth3D, TileHeight3D,
                                       gcvNULL);

    gcmFOOTER_ARG("status=%d *TileWidth2D=%d *TileHeight2D=%d *TileWidth3D=%d "
                  "*TileHeight3D=%d", status, gcmOPT_VALUE(TileWidth2D),
                  gcmOPT_VALUE(TileHeight2D), gcmOPT_VALUE(TileWidth3D),
                  gcmOPT_VALUE(TileHeight3D));
    return status;
}

/*******************************************************************************
**
**  gcoHAL_QueryPowerManagementState
**
**  Query current GPU power state.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gceCHIPPOWERSTATE State
**          Power State.
**
*/
gceSTATUS
gcoHAL_QueryPowerManagementState(
    IN gcoHAL Hal,
    OUT gceCHIPPOWERSTATE * State
    )
{
    gcsHAL_INTERFACE iface;
    gceSTATUS status;

    gcmHEADER();

    /* Call kernel API to set power management state. */
    iface.command = gcvHAL_QUERY_POWER_MANAGEMENT_STATE;
    gcmONERROR(gcoHAL_Call(gcvNULL, &iface));

    /* Return state to the caller. */
    *State = iface.u.QueryPowerManagement.state;

    /* Success. */
    gcmFOOTER_ARG("*State=%d", *State);
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_IsFeatureAvailable
**
**  Verifies whether the specified feature is available in hardware.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**      gceFEATURE Feature
**          Feature to be verified.
*/
gceSTATUS
gcoHAL_IsFeatureAvailable(
    IN gcoHAL Hal,
    IN gceFEATURE Feature
    )
{
    gceSTATUS status;
#if gcdENABLE_VG
    gceHARDWARE_TYPE currentHW;
#endif

    gcmHEADER_ARG("Feature=%d", Feature);
#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentHW);
    if (currentHW == gcvHARDWARE_VG)
    {
        status = gcoVGHARDWARE_IsFeatureAvailable(gcvNULL, Feature);
    }
    else
#endif
    {
        /* Query support from hardware object. */
        status = gcoHARDWARE_IsFeatureAvailable(Feature);
    }


    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_QueryVideoMemory
**
**  Query the amount of video memory.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to an gcoHAL object.
**
**  OUTPUT:
**
**      gctPHYS_ADDR * InternalAddress
**          Pointer to a variable that will hold the physical address of the
**          internal memory.  If 'InternalAddress' is gcvNULL, no information about
**          the internal memory will be returned.
**
**      gctSIZE_T * InternalSize
**          Pointer to a variable that will hold the size of the internal
**          memory.  'InternalSize' cannot be gcvNULL if 'InternalAddress' is not
**          gcvNULL.
**
**      gctPHYS_ADDR * ExternalAddress
**          Pointer to a variable that will hold the physical address of the
**          external memory.  If 'ExternalAddress' is gcvNULL, no information about
**          the external memory will be returned.
**
**      gctSIZE_T * ExternalSize
**          Pointer to a variable that will hold the size of the external
**          memory.  'ExternalSize' cannot be gcvNULL if 'ExternalAddress' is not
**          gcvNULL.
**
**      gctPHYS_ADDR * ContiguousAddress
**          Pointer to a variable that will hold the physical address of the
**          contiguous memory.  If 'ContiguousAddress' is gcvNULL, no information
**          about the contiguous memory will be returned.
**
**      gctSIZE_T * ContiguousSize
**          Pointer to a variable that will hold the size of the contiguous
**          memory.  'ContiguousSize' cannot be gcvNULL if 'ContiguousAddress' is
**          not gcvNULL.
*/
gceSTATUS
gcoHAL_QueryVideoMemory(
    IN gcoHAL Hal,
    OUT gctPHYS_ADDR * InternalAddress,
    OUT gctSIZE_T * InternalSize,
    OUT gctPHYS_ADDR * ExternalAddress,
    OUT gctSIZE_T * ExternalSize,
    OUT gctPHYS_ADDR * ContiguousAddress,
    OUT gctSIZE_T * ContiguousSize
    )
{
    gceSTATUS status;

    gcmHEADER();

    status = gcoOS_QueryVideoMemory(gcvNULL,
                                    InternalAddress, InternalSize,
                                    ExternalAddress, ExternalSize,
                                    ContiguousAddress, ContiguousSize);

    gcmFOOTER_ARG("status=%d InternalAddress=0x%x InternalSize=%lu "
                  "ExternalAddress=0x%x ExternalSize=%lu ContiguousAddress=0x%x "
                  "ContiguousSize=%lu", status, gcmOPT_POINTER(InternalAddress),
                  gcmOPT_VALUE(InternalSize), gcmOPT_POINTER(ExternalAddress),
                  gcmOPT_VALUE(ExternalSize), gcmOPT_POINTER(ContiguousAddress),
                  gcmOPT_VALUE(ContiguousSize));
    return status;
}

#ifndef VIVANTE_NO_3D

/*******************************************************************************
**
**	gcoHAL_QueryTargetCaps
**
**	Query the render target capabilities.
**
**	INPUT:
**
**		gcoHAL Hal
**			Pointer to gcoHAL object.
**
**	OUTPUT:
**
**		gctUINT * MaxWidth
**			Pointer to a variable receiving the maximum width of a render
**			target.
**
**		gctUINT * MaxHeight
**			Pointer to a variable receiving the maximum height of a render
**			target.
**
**		gctUINT * MultiTargetCount
**			Pointer to a variable receiving the number of render targets.
**
**		gctUINT * MaxSamples
**			Pointer to a variable receiving the maximum number of samples per
**			pixel for MSAA.
*/
gceSTATUS
gcoHAL_QueryTargetCaps(
	IN gcoHAL Hal,
	OUT gctUINT * MaxWidth,
	OUT gctUINT * MaxHeight,
	OUT gctUINT * MultiTargetCount,
	OUT gctUINT * MaxSamples
	)
{
	gceSTATUS status;

#if gcdENABLE_VG
    gceHARDWARE_TYPE currentHW;
#endif
	gcmHEADER();

#if gcdENABLE_VG
    gcmGETCURRENTHARDWARE(currentHW);
    if (currentHW == gcvHARDWARE_VG)
    {
	    status = gcoVGHARDWARE_QueryTargetCaps(gcvNULL,
										 MaxWidth, MaxHeight,
										 MultiTargetCount,
										 MaxSamples);
    }
    else
#endif
    {
	    status = gcoHARDWARE_QueryTargetCaps(
										 MaxWidth, MaxHeight,
										 MultiTargetCount,
										 MaxSamples);
    }

	gcmFOOTER_ARG("status=%d *MaxWidth=%u *MaxHeight=%u *MultiTargetCount=%u "
				  "*MaxSamples=%u",
				  status, gcoOS_DebugStatus2Name(status),
				  (MaxWidth         == gcvNULL) ? 0 : *MaxWidth,
				  (MaxHeight        == gcvNULL) ? 0 : *MaxHeight,
				  (MultiTargetCount == gcvNULL) ? 0 : *MultiTargetCount,
				  (MaxSamples       == gcvNULL) ? 0 : *MaxSamples);
	return status;
}

/*******************************************************************************
**
**  gcoHAL_QueryShaderCaps
**
**  Query the shader capabilities.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**  OUTPUT:
**
**      gctUINT * VertexUniforms
**          Pointer to a variable receiving the number of uniforms in the vertex
**          shader.
**
**      gctUINT * FragmentUniforms
**          Pointer to a variable receiving the number of uniforms in the
**          fragment shader.
**
**      gctUINT * Varyings
**          Pointer to a variable receiving the maimum number of varyings.
*/
gceSTATUS
gcoHAL_QueryShaderCaps(
    IN gcoHAL Hal,
    OUT gctUINT * VertexUniforms,
    OUT gctUINT * FragmentUniforms,
    OUT gctUINT * Varyings
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Query the hardware. */
    status = gcoHARDWARE_QueryShaderCaps(VertexUniforms,
                                         FragmentUniforms,
                                         Varyings,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL);

    gcmFOOTER_ARG("status=%d *VertexUniforms=%u *FragmentUniforms=%u "
                  "*Varyings=%u", status, gcmOPT_VALUE(VertexUniforms),
                  gcmOPT_VALUE(FragmentUniforms), gcmOPT_VALUE(Varyings));
    return status;
}

/*******************************************************************************
**
**  gcoHAL_QueryTextureCaps
**
**  Query the texture capabilities.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**  OUTPUT:
**
**      gctUINT * MaxWidth
**          Pointer to a variable receiving the maximum width of a texture.
**
**      gctUINT * MaxHeight
**          Pointer to a variable receiving the maximum height of a texture.
**
**      gctUINT * MaxDepth
**          Pointer to a variable receiving the maximum depth of a texture.  If
**          volume textures are not supported, 0 will be returned.
**
**      gctBOOL * Cubic
**          Pointer to a variable receiving a flag whether the hardware supports
**          cubic textures or not.
**
**      gctBOOL * NonPowerOfTwo
**          Pointer to a variable receiving a flag whether the hardware supports
**          non-power-of-two textures or not.
**
**      gctUINT * VertexSamplers
**          Pointer to a variable receiving the number of sampler units in the
**          vertex shader.
**
**      gctUINT * PixelSamplers
**          Pointer to a variable receiving the number of sampler units in the
**          pixel shader.
*/
gceSTATUS
gcoHAL_QueryTextureCaps(
    IN gcoHAL Hal,
    OUT gctUINT * MaxWidth,
    OUT gctUINT * MaxHeight,
    OUT gctUINT * MaxDepth,
    OUT gctBOOL * Cubic,
    OUT gctBOOL * NonPowerOfTwo,
    OUT gctUINT * VertexSamplers,
    OUT gctUINT * PixelSamplers
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Query the hardware. */
    status = gcoHARDWARE_QueryTextureCaps(MaxWidth,
                                          MaxHeight,
                                          MaxDepth,
                                          Cubic,
                                          NonPowerOfTwo,
                                          VertexSamplers,
                                          PixelSamplers,
                                          gcvNULL);

    gcmFOOTER_ARG("status=%d *MaxWidth=%u *MaxHeight=%u *MaxDepth=%u *Cubic=%d "
                  "*NonPowerOfTwo=%d *VertexSamplers=%u *PixelSamplers=%u",
                  status, gcmOPT_VALUE(MaxWidth), gcmOPT_VALUE(MaxHeight),
                  gcmOPT_VALUE(MaxDepth), gcmOPT_VALUE(Cubic),
                  gcmOPT_VALUE(NonPowerOfTwo), gcmOPT_VALUE(VertexSamplers),
                  gcmOPT_VALUE(PixelSamplers));
    return status;
}

/*******************************************************************************
**
**  gcoHAL_QueryTextureMaxAniso
**
**  Query the texture capabilities.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**  OUTPUT:
**
**      gctUINT * MaxAnisoValue
**          Pointer to a variable receiving the maximum parameter value of
**          anisotropic filter.
*/
gceSTATUS
gcoHAL_QueryTextureMaxAniso(
    IN gcoHAL Hal,
    OUT gctUINT * MaxAnisoValue
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Query the hardware. */
    status = gcoHARDWARE_QueryTextureCaps(gcvNULL,
                                          gcvNULL,
                                          gcvNULL,
                                          gcvNULL,
                                          gcvNULL,
                                          gcvNULL,
                                          gcvNULL,
                                          MaxAnisoValue);

    gcmFOOTER_ARG("status=%d *MaxAnisoValue=%u",
                  status, gcmOPT_VALUE(MaxAnisoValue));
    return status;
}

/*******************************************************************************
**
**  gcoHAL_QueryStreamCaps
**
**  Query the stream capabilities of the hardware.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**  OUTPUT:
**
**      gctUINT * MaxAttributes
**          Pointer to a variable that will hold the maximum number of
**          atrtributes for a primitive on success.
**
**      gctUINT * MaxStreamSize
**          Pointer to a variable that will hold the maximum number of bytes of
**          a stream on success.
**
**      gctUINT * NumberOfStreams
**          Pointer to a variable that will hold the number of streams on
**          success.
**
**      gctUINT * Alignment
**          Pointer to a variable that will receive the stream alignment
**          requirement.
*/
gceSTATUS
gcoHAL_QueryStreamCaps(
    IN gcoHAL Hal,
    OUT gctUINT32 * MaxAttributes,
    OUT gctUINT32 * MaxStreamSize,
    OUT gctUINT32 * NumberOfStreams,
    OUT gctUINT32 * Alignment
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Query the hardware. */
    status = gcoHARDWARE_QueryStreamCaps(MaxAttributes,
                                         MaxStreamSize,
                                         NumberOfStreams,
                                         Alignment);

    gcmFOOTER_ARG("status=%d *MaxAttributes=%u *MaxStreamSize=%u "
                  "*NumberOfStreams=%u *Alignment=%u", status,
                  gcmOPT_VALUE(MaxAttributes), gcmOPT_VALUE(MaxStreamSize),
                  gcmOPT_VALUE(NumberOfStreams), gcmOPT_VALUE(Alignment));
    return status;
}

#endif /* VIVANTE_NO_3D */

/*******************************************************************************
**
**  gcoHAL_SetHardwareType
**
**  Set the hal hardware type to the tls.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**
**      gceHARDWARE_TYPE HardwardType
**          hardware type.
**  OUTPUT:
**
*/
gceSTATUS
gcoHAL_SetHardwareType(
    IN gcoHAL Hal,
    IN gceHARDWARE_TYPE HardwardType
    )
{
    gceSTATUS status;
    gcsTLS_PTR __tls__;

    gcmHEADER_ARG("HardwardType=%d", HardwardType);

    gcmONERROR(gcoOS_GetTLS(&__tls__));

    __tls__->currentType = HardwardType;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  gcoHAL_GetHardwareType
**
**  get the hal hardware type to the tls.
**
**  INPUT:
**
**      gcoHAL Hal
**          Pointer to a gcoHAL object.
**
**  OUTPUT:
**
**      gceHARDWARE_TYPE *HardwardType
**          Pointer to a variable that will hold hardware type.
**
*/
gceSTATUS
gcoHAL_GetHardwareType(
    IN gcoHAL Hal,
    OUT gceHARDWARE_TYPE *HardwardType
    )
{
    gceSTATUS status;
    gcsTLS_PTR __tls__;

    gcmHEADER();

    gcmONERROR(gcoOS_GetTLS(&__tls__));

    if (HardwardType != gcvNULL)
    {
        *HardwardType = __tls__->currentType;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_QueryChipCount(
    IN gcoHAL       Hal,
    OUT gctINT32   *Count
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

	/* Verify the arguments. */
	gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    if (gcPLS.hal->chipCount <= 0 || gcPLS.hal->chipCount > gcdCHIP_COUNT)
    {
        gcmONERROR(gcvSTATUS_CONTEXT_LOSSED);
    }

	*Count = gcPLS.hal->chipCount;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
gcoHAL_QuerySeparated3D2D(
    IN gcoHAL Hal
    )
{
    gceSTATUS status;
    gcmHEADER();

    if (gcPLS.hal->separated3D2D)
    {
        status = gcvSTATUS_TRUE;
        gcmFOOTER();
        return status;
    }
    else
    {
        status = gcvSTATUS_FALSE;
        gcmFOOTER();
        return status;
    }
}

#if gcdENABLE_VG
static gceHARDWARE_TYPE
_GetHardwareType(
    gctINT32 Chip
    )
{
    if (Chip >= gcdCHIP_COUNT)
        return gcvHARDWARE_INVALID;

    return gcPLS.hal->chipTypes[Chip];
}

gceSTATUS
gcoHAL_QueryChipLimits(
    IN gcoHAL           Hal,
    IN gctINT32         Chip,
    OUT gcsHAL_LIMITS   *Limits
    )
{
    gceSTATUS       status;
    gceCHIPMODEL    chipModel   = 0;
    gctUINT32       maxWidth    = 0;
    gctUINT32       maxHeight   = 0;
    gctUINT32       maxSamples  = 0;
    gceHARDWARE_TYPE currentType;
    gceHARDWARE_TYPE type = _GetHardwareType(Chip);

    /* Save the current hardware type */
    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &currentType));

    /* Change to the new hardware type */
    gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, type));

    switch (type)
    {
    case gcvHARDWARE_3D2D:
    case gcvHARDWARE_3D:
    case gcvHARDWARE_2D:
        gcmONERROR(gcoHARDWARE_QueryChipIdentity(
            &chipModel,
            gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL));

        gcmONERROR(gcoHARDWARE_QueryTargetCaps(
            (gctUINT32_PTR) &maxWidth,
            (gctUINT32_PTR) &maxHeight,
            gcvNULL,
            (gctUINT32_PTR) &maxSamples
            ));
        break;

    case gcvHARDWARE_VG:
        gcmONERROR(gcoVGHARDWARE_QueryChipIdentity(
            gcvNULL,
            &chipModel,
            gcvNULL, gcvNULL, gcvNULL, gcvNULL));

        gcmONERROR(gcoVGHARDWARE_QueryTargetCaps(
            gcvNULL,
            (gctUINT32_PTR) &maxWidth,
            (gctUINT32_PTR) &maxHeight,
            gcvNULL,
            (gctUINT32_PTR) &maxSamples
            ));
        break;

    default:
        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    /* Restore the current hardware type */
    gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, currentType));

    if (Limits != gcvNULL)
    {
        Limits->chipModel   = chipModel;
        Limits->maxWidth    = maxWidth;
        Limits->maxHeight   = maxHeight;
        Limits->maxSamples  = maxSamples;
    }

    return gcvSTATUS_OK;

OnError:
    return status;

}

gceSTATUS
gcoHAL_QueryChipFeature(
    IN gcoHAL       Hal,
    IN gctINT32     Chip,
    IN gceFEATURE   Feature
    )
{
    gceSTATUS status;
    gceHARDWARE_TYPE currentType;
    gceHARDWARE_TYPE type = _GetHardwareType(Chip);

    /* Save the current hardware type */
    gcmVERIFY_OK(gcoHAL_GetHardwareType(gcvNULL, &currentType));

    /* Change to the new hardware type */
    gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, type));

    switch (type)
    {
    case gcvHARDWARE_3D2D:
    case gcvHARDWARE_3D:
    case gcvHARDWARE_2D:
        status = gcoHARDWARE_IsFeatureAvailable(Feature);
        break;

    case gcvHARDWARE_VG:
        status = gcoVGHARDWARE_IsFeatureAvailable(gcvNULL, Feature);
        break;

    default:
        status = gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Restore the current hardware type */
    gcmVERIFY_OK(gcoHAL_SetHardwareType(gcvNULL, currentType));

    return status;
}
#endif

