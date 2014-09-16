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


#ifndef __gc_vgsh_hardware_h_
#define __gc_vgsh_hardware_h_

#define vgvMAX_PROGRAM_CACHE		8
#define vgvMAX_FRAG_UNIFORMS		64
#define vgvMAX_VERT_UNIFORMS		64
#define vgvMAX_ATTRIBUTES			3
#define vgvMAX_SAMPLERS				8

#define vgvMAX_PROGRAMS				2048

#define vgvMAX_GAUSSIAN_KERNEL_SIZE		16
#define vgvMAX_GAUSSIAN_COORD_SIZE		1

#define vgvMAX_SEPCONVOLVE_KERNEL_SIZE		8
#define vgvMAX_SEPCONVOLVE_COORD_SIZE		8

#define vgvMAX_CONVOLVE_KERNEL_SIZE		13
#define vgvMAX_CONVOLVE_COORD_SIZE		26

#define vgvORIENTATION_LEFT_TOP			gcvORIENTATION_TOP_BOTTOM
#define vgvORIENTATION_LEFT_BOT			gcvORIENTATION_BOTTOM_TOP

enum
{
	vgvATTRIBUTE_VERTEX = 0,
	vgvATTRIBUTE_UV,
	vgvATTRIBUTE_COLOR,
};

typedef enum _vgeFILTER
{
	vgvFILTER_NONE,
	vgvFILTER_COLOR_MATRIX,
	vgvFILTER_CONVOLVE,
	vgvFILTER_SEPCONVOLVE,
	vgvFILTER_GAUSSIAN,
	vgvFILTER_LOOKUP,
	vgvFILTER_LOOKUP_SINGLE,
	vgvFILTER_COPY,
}vgeFILTER;

typedef enum _vgeDRAWPIPE
{
	vgvDRAWPIPE_NONE,
	vgvDRAWPIPE_PATH,
	vgvDRAWPIPE_IMAGE,
	vgvDRAWPIPE_COLORRAMP,
	vgvDRAWPIPE_FILTER,
	vgvDRAWPIPE_MASK,
	vgvDRAWPIPE_CLEAR,
	vgvDRAWPIPE_COPY,
}vgeDRAWPIPE;


typedef struct _vgHARDWARE _vgHARDWARE;
typedef struct _vgHARDWARE *vgHARDWARE;

typedef gceSTATUS (*vgSAMPLERSET) (
	_vgHARDWARE*,
	gctINT
	);

typedef gceSTATUS (*vgUNIFORMSET) (
	_vgHARDWARE*,
	gcUNIFORM
	);

typedef gceSTATUS (*vgATTRIBUTESET) (
	_vgHARDWARE*,
	gctINT
	);


typedef struct _vgATTRIBUTEWRAP * vgATTRIBUTEWRAP;
typedef struct _vgATTRIBUTEWRAP
{
	gcATTRIBUTE		attribute;
	gctINT16		index;
	vgATTRIBUTESET	setfunc;
}_vgATTRIBUTEWRAP;


typedef struct _vgUNIFORMWRAP * vgUNIFORMWRAP;
typedef struct _vgUNIFORMWRAP
{
	gcUNIFORM		uniform;
	vgUNIFORMSET	setfunc;
}_vgUNIFORMWRAP;

typedef struct _vgSAMPLERWRAP * vgSAMPLERWRAP;
typedef struct _vgSAMPLERWRAP
{
	gctINT			sampler;
	vgSAMPLERSET	setfunc;
}_vgSAMPLERWRAP;

typedef struct _vgATTRIBUTE *vgATTRIBUTE;
typedef struct _vgATTRIBUTE
{
    /* vertex */
	gctINT				location;
	gcoSTREAM			stream;
    gctUINT			    size;
	gceVERTEX_FORMAT	type;
	gctBOOL		        normalized;
	gctINT				offset;
	gctUINT32			stride;
	/* index */
    gceINDEX_TYPE       indexType;
    gcoINDEX            index;
}_vgATTRIBUTE;


typedef struct _vgSHADERKEY
{
	gctUINT32	drawPipe			: 3;
	gctUINT32	blend				: 4;
	gctUINT32	colorTransform		: 1;
	gctUINT32	scissoring			: 1;
	gctUINT32	masking				: 1;
	gctUINT32	paintType			: 2;
	gctUINT32	imageMode			: 2;
	gctUINT32	rootImage			: 1;
	gctUINT32	patternTilingMode	: 2;
	gctUINT32	filterType			: 3;
	gctUINT32	filterHoriz			: 1;
	gctUINT32	maskOperation		: 3;
	gctUINT32	hasAlpha			: 1;
	gctUINT32	pack				: 2;
	gctUINT32	srcConvert			: 4;
	gctUINT32	srcConvertAlpha		: 2;
	gctUINT32	dstConvert			: 4;
	gctUINT32	dstConvertAlpha		: 2;

	gctUINT32	extra						: 2;
	gctUINT32	maskColorConvert1			: 4;
	gctUINT32	maskColorConvertAlpha1		: 2;
	gctUINT32	maskColorConvert2			: 4;
	gctUINT32	maskColorConvertAlpha2		: 2;

	gctUINT32	alphaOnly					: 1;
	gctUINT32	colorRampPremultiplied		: 1;
	gctUINT32	paintZero					: 1;
	gctUINT32   clampToAlpha				: 1;
	gctUINT32   filterChannel				: 4;
	gctUINT32	guassianOPT					: 1;
	gctUINT32	disableClamp				: 1;
	gctUINT32	disableTiling				: 1;
}_vgSHADERKEY;



typedef struct _vgPROGRAMTABLE *vgPROGRAMTABLE;
typedef struct _vgPROGRAMTABLE
{
	_vgSHADERKEY	entry;
	_VGProgram		*program;
}_vgPROGRAMTABLE;

typedef struct _vgPROGRAMSLOT *vgPROGRAMSLOT;
typedef struct _vgPROGRAMSLOT
{
	_VGProgram **programs;
	gctUINT8	*index;
	gctUINT32	*programCount;
}_vgPROGRAMSLOT;


typedef struct _vgCORE	*vgCORE;
typedef struct _vgCORE
{
	/*hal objects*/
	gcoOS			os;
	gcoHAL			hal;
	gco3D			engine;
	gcoSURF			draw;
	gcoSURF			depth;

	gctUINT			samples;
	gctUINT			width;
	gctUINT			height;
	gctBOOL			blend;
	gceCOMPARE		depthCompare;
	gctBOOL			depthWrite;
	gctUINT8		colorWrite;
	gceDEPTH_MODE	depthMode;

	gceSTENCIL_MODE			stencilMode;
	gceCOMPARE				stencilCompare;
	gctUINT8				stencilRef;
	gctUINT8				stencilMask;
	gceSTENCIL_OPERATION	stencilFail;

	gctBOOL					invalidCache;

	/*shader states*/
	gctSIZE_T	        statesSize;
	gctPOINTER	        states;
	gcsHINT_PTR			hints;


	/*bind vertex*/
	gcoVERTEX			vertex;


	/*primitive info*/
	gctINT				start;
	gctINT				base;
	gctSIZE_T			primitiveCount;
	gcePRIMITIVE		primitiveType;
	gctBOOL				drawIndex;

	/*blend info*/
	gceBLEND_FUNCTION	sourceFuncRGB;
	gceBLEND_FUNCTION	sourceFuncAlpha;
	gceBLEND_FUNCTION	destFuncRGB;
	gceBLEND_FUNCTION	destFuncAlpha;

}_vgCORE;

struct _vgHARDWARE
{
	_vgCORE			core;
#
	_VGImage		*srcImage;
	_VGImage		*dstImage;
	_VGPaint		*paint;
	_VGPath			*path;


    _VGImage        *mask;


	VGMaskOperation maskOperation;

	VGBlendMode		blendMode;
	VGImageMode		imageMode;
	VGPaintMode		paintMode;

	gctBOOL			masking;
	gctBOOL			colorTransform;
	gctBOOL			scissoring;
	gctBOOL			blending;
	gceCOMPARE		depthCompare;
	gctBOOL			depthWrite;

	gctUINT8		colorWrite;
	gceDEPTH_MODE	depthMode;

	gceSTENCIL_MODE			stencilMode;
	gceCOMPARE				stencilCompare;
	gctUINT8				stencilRef;
	gctUINT8				stencilMask;
	gceSTENCIL_OPERATION	stencilFail;


	gctBOOL			hasAlpha;
	gctBOOL			flush;

	vgeDRAWPIPE		drawPipe;
	vgeFILTER		filterType;


    gctINT32       dx;
    gctINT32       dy;
    gctINT32       sx;
    gctINT32       sy;
    gctINT32       width;
    gctINT32       height;

    gctUINT32		srcConvert;
	gctUINT32		dstConvert;
	gctUINT32		srcConvertAlpha;
	gctUINT32		dstConvertAlpha;

	gctUINT32		pack;
	_VGColor		clearColor;
	gcoSTREAM		rectStream;
	gcoSTREAM		tempStream;

	VGImageChannel	sourceChannel;
	gctBOOL			alphaOnly;

	gctBOOL			filterHoriz;

	VGTilingMode	tilingMode;
    _VGImage        *lutImage;


	_VGMatrix3x3	*paintToUser;
	_VGMatrix3x3	*userToSurface;

	gctFLOAT		*colorMatrix;

	gctFLOAT		*kernel;
	gctFLOAT		kernel0;
	gctFLOAT		kernelSize;
	gctFLOAT		*texCoordOffset;
	gctFLOAT		texCoordOffsetSize;

	gctFLOAT		*kernelY;
	gctFLOAT		kernelSizeY;
	gctFLOAT		*texCoordOffsetY;
	gctFLOAT		texCoordOffsetSizeY;

	gctFLOAT		scale;
	gctFLOAT		bias;

	gctBOOL			disableTiling;
	gctBOOL			disableClamp;
	gctBOOL			gaussianOPT;

	gctFLOAT		zValue;

	gctFLOAT		*colorTransformValues;

	_VGContext		*context;

	_VGProgram		*program;

	_vgPROGRAMTABLE		programTable[vgvMAX_PROGRAMS];



#if WALK600
	gctBOOL			isConformance;
	gctBOOL			isGC600_19;
#endif

#if WALK_MG1
	gctFLOAT		gdx, gdy;
#endif

	gctBOOL			featureVAA;
	gctBOOL			usingMSAA;

};


gceSTATUS vgshHARDWARE_RunPipe(_vgHARDWARE *hardware);

void _vgHARDWARECtor(_vgHARDWARE *hardware);
void _vgHARDWAREDtor(_vgHARDWARE *hardware);
gceSTATUS vgshDrawPath(_VGContext *context, _VGPath *p, VGbitfield paintModes, _VGMatrix3x3 *userToSurface);
gceSTATUS vgshDrawImage(_VGContext *context, _VGImage *image, _VGMatrix3x3 *imageUserToSurface);
gceSTATUS vgshClear(_VGContext *context, _VGImage *image, gctINT32 x, gctINT32 y, gctINT32 width, gctINT32 height, _VGColor *color, \
				  gctBOOL scissoring, gctBOOL flush);
gceSTATUS vgshDrawMaskPath(_VGContext *context, _VGImage* layer,  _VGPath *path, VGbitfield paintModes);
gceSTATUS vgshMask(_VGContext *context, _VGImage* image, VGMaskOperation operation, gctINT32 dx, gctINT32 dy, gctINT32 sx, gctINT32 sy, gctINT32 width, gctINT32 height);
gceSTATUS vgshCreateMaskBuffer(_VGContext	*context);

gceSTATUS vgshCreateTexture(_VGContext *context, gctUINT32 width, gctUINT32 height, gceSURF_FORMAT format, gcoTEXTURE *texture, gcoSURF *surface);

gceSTATUS vgshCreateImageStream(
    _VGContext *hardware,
    _VGImage *image,
    gctINT32 dx,
    gctINT32 dy,
    gctINT32 sx,
    gctINT32 sy,
    gctINT32 width,
    gctINT32 height,
    gcoSTREAM *stream);

#endif /* __gc_vgsh_hardware_h_ */
