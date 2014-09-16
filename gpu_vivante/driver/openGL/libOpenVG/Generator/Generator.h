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


#ifndef __GENERATOR_H__
#define __GENERATOR_H__

typedef int						gctINT;
typedef signed char				gctINT8;
typedef signed short			gctINT16;
typedef signed int				gctINT32;
typedef signed long long		gctINT64;

typedef gctINT *				gctINT_PTR;
typedef gctINT8 *				gctINT8_PTR;
typedef gctINT16 *				gctINT16_PTR;
typedef gctINT32 *				gctINT32_PTR;
typedef gctINT64 *				gctINT64_PTR;

typedef unsigned int			gctUINT;
typedef unsigned char			gctUINT8;
typedef unsigned short			gctUINT16;
typedef unsigned int			gctUINT32;
typedef unsigned long long		gctUINT64;

typedef gctUINT *				gctUINT_PTR;
typedef gctUINT8 *				gctUINT8_PTR;
typedef gctUINT16 *				gctUINT16_PTR;
typedef gctUINT32 *				gctUINT32_PTR;
typedef gctUINT64 *				gctUINT64_PTR;

typedef float					gctFLOAT;
typedef int						gctBOOL;
typedef char *					gctSTRING;

#define gcvFALSE				0
#define gcvTRUE					1

#ifdef __cplusplus
#	define gcvNULL				0
#else
#	define gcvNULL				((void *) 0)
#endif

#define gcmCOUNTOF(a) \
( \
	sizeof(a) / sizeof(a[0]) \
)

#define gcmCLAMP(x, min, max) \
( \
	((x) < (min)) \
		? (min) \
		: ((x) > (max)) \
			? (max) \
			: (x) \
)

#define gcmMIN(x, y) \
( \
	((x) <= (y)) \
		? (x) \
		: (y) \
)

#define gcmMAX(x, y) \
( \
	((x) >= (y)) \
		? (x) \
		: (y) \
)

typedef enum _vgeIMAGE_CHANNEL
{
	VG_RED   = (1 << 3),
	VG_GREEN = (1 << 2),
	VG_BLUE  = (1 << 1),
	VG_ALPHA = (1 << 0),

	VG_RGB   = VG_RED | VG_GREEN | VG_BLUE,
	VG_RGBA  = VG_RED | VG_GREEN | VG_BLUE | VG_ALPHA,
}
vgeIMAGE_CHANNEL;

extern gctSTRING maskArray[16];
extern gctSTRING colorSpaceArray[2];
extern gctSTRING premulArray[2];

#define vgvUPSAMPLED_INFO_NAME	"_upsampled_%s"
#define vgvUPSAMPLED_FUNC_NAME	"Upsampled_%s"
#define vgvPIXEL_READ			"_ReadPixel_%s_To_%sRGBA%s"
#define vgvFULL_PIXEL_WRITE		"_WritePixel_%s%s%s_To_%s"
#define vgvMASKED_PIXEL_WRITE	"_WritePixel_%s%s%s_Masked_To_%s"

#define vgvTAB \
	"\t"

#define vgmPRINTCMNT(Tab, Comment) \
	vgfWriteLine(File, Tab "/* " # Comment " */\n")

#define vgmPRINTF(Tab, Line, ...) \
	vgfWriteLine(File, Tab Line "\n", __VA_ARGS__)

#define vgmPRINTLINE(Tab, Line) \
	vgfWriteLine(File, Tab # Line "\n")

void vgfWriteLine(
	CStdioFile& File,
	LPCTSTR Line,
	...
	);

void vgfGenerateCaption(
	CStdioFile& File,
	CString Description
	);

#endif
