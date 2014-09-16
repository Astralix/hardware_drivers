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




#ifndef __gc_vg_context_h__
#define __gc_vg_context_h__

#ifdef __cplusplus
extern "C" {
#endif

#define gcvSTROKE_KEEP_MEMPOOL		0

/*----------------------------------------------------------------------------*/
/*------------------------ Miscellaneous definitions. ------------------------*/

/* The number of format entries per table (native + non-native). */
#define vgvFORMAT_COUNT		(15 + 1)

/*----------------------------------------------------------------------------*/
/*---------------------------- Thread structure. -----------------------------*/

typedef struct _vgsTHREADDATA * vgsTHREADDATA_PTR;
typedef struct _vgsTHREADDATA
{
	vgsCONTEXT_PTR			context;
}
vgsTHREADDATA;

/*----------------------------------------------------------------------------*/
/*------------------------ Chip information structure. -----------------------*/

typedef struct _vgsCHIPINFO * vgsCHIPINFO_PTR;
typedef struct _vgsCHIPINFO
{
	VGubyte*				vendor;
	VGubyte*				renderer;
	VGubyte*				version;
	VGubyte*				extensions;
}
vgsCHIPINFO;

/*----------------------------------------------------------------------------*/
/*---------------------- Clear color wrapper structure. ----------------------*/

typedef struct _vgsCLEARCOLOR * vgsCLEARCOLOR_PTR;
typedef struct _vgsCLEARCOLOR
{
	gctUINT16				validBits;
	gctUINT32				value[vgvFORMAT_COUNT];
}
vgsCLEARCOLOR;

/*----------------------------------------------------------------------------*/
/*---------------------------- Context structure. ----------------------------*/

typedef struct _vgsCONTEXT
{
	/* Context error value. */
	VGErrorCode				error;

	/* HAL objects. */
	gcoHAL					hal;
	gcoOS					os;
	gcoVG					vg;

	/* Chip information. */
	gceCHIPMODEL			chipModel;
	gctUINT32				chipRevision;
	char					chipName[23];
	vgsCHIPINFO				chipInfo;

	/* System images. */
	vgsIMAGE				targetImage;
	vgsIMAGE				maskImage;
	vgsIMAGE				tempMaskImage;
	vgsIMAGE				wrapperImage;

	/* Temporary image. */
	vgsIMAGE				tempImage;
	gcuVIDMEM_NODE_PTR		tempNode;
	gctUINT32				tempPhysical;
	gctUINT8_PTR			tempLogical;
	gctUINT					tempSize;

	/* Mode settings. */
	VGFillRule				fillRule;								/* VG_FILL_RULE */
	gceFILL_RULE			halFillRule;

	VGImageQuality			imageQuality;							/* VG_IMAGE_QUALITY  */
	VGImageMode				imageMode;								/* VG_IMAGE_MODE */
	gceVG_IMAGE				halImageMode;

	VGRenderingQuality		renderingQuality;						/* VG_RENDERING_QUALITY */
	gceRENDER_QUALITY		halRenderingQuality;

	VGBlendMode				blendMode;								/* VG_BLEND_MODE */
	gceVG_BLEND				halBlendMode;

	/* Scissoring. */
	VGboolean				scissoringEnable;						/* VG_SCISSORING */
	gcsVG_RECT				scissoringRects[vgvMAX_SCISSOR_RECTS];	/* VG_SCISSOR_RECTS */
	VGint					scissoringRectsCount;
	VGboolean				scissoringRectsDirty;

	/* Color transformation (x4 RGBA scale factors and x4 RGBA biases). */
	VGboolean				colTransform;							/* VG_COLOR_TRANSFORM */
	VGfloat					colTransformValues[8];					/* VG_COLOR_TRANSFORM_VALUES */
	VGboolean				colTransformValuesDirty;

	/* Stroke parameters. */
	VGfloat					strokeLineWidth;						/* VG_STROKE_LINE_WIDTH */
	VGfloat					strokeMiterLimit;						/* VG_STROKE_MITER_LIMIT */
	VGboolean				strokeDashPtrnEnable;					/* Dash pattern enable flag. */
	VGfloat					strokeDashPtrn[vgvMAX_DASH_COUNT];		/* VG_STROKE_DASH_PATTERN */
	VGint					strokeDashPtrnCount;
	VGfloat					strokeDashPhase;						/* VG_STROKE_DASH_PHASE */
	VGboolean				strokeDashPhaseReset;					/* VG_STROKE_DASH_PHASE_RESET */

	VGCapStyle				strokeCapStyle;							/* VG_STROKE_CAP_STYLE */
	gceCAP_STYLE			halStrokeCapStyle;

	VGJoinStyle				strokeJoinStyle;						/* VG_STROKE_JOIN_STYLE */
	gceJOIN_STYLE			halStrokeJoinStyle;

#if gcvSTROKE_KEEP_MEMPOOL
	/* Stroke conversion object. */
	vgsSTROKECONVERSION_PTR	strokeConversion;
#endif

	/* Edge fill color for VG_TILE_FILL tiling mode. */
	vgtFLOATVECTOR4			tileFillColor;							/* VG_TILE_FILL_COLOR */

	/* Color for vgClear and vgClearImage. */
	vgtFLOATVECTOR4			clearColor;								/* VG_CLEAR_COLOR */
	vgtBYTEVECTOR4			halClearColor;

	/* Glyph origin. */
	VGfloat					glyphOrigin[2];							/* VG_GLYPH_ORIGIN */
	VGfloat					glyphOffsetX;
	VGfloat					glyphOffsetY;

	/* Enable/disable alpha masking. */
	VGboolean				maskingEnable;							/* VG_MASKING */
	gctBOOL					maskDirty;

	/* Pixel layout information. */
	VGPixelLayout			pixelLayout;							/* VG_PIXEL_LAYOUT */
	VGPixelLayout			screenLayout;							/* VG_SCREEN_LAYOUT */

	/* Filters parameters. */
	VGbitfield				fltChannelMask;							/* VG_FILTER_CHANNEL_MASK */
	gctUINT					fltCapChannelMask;
	gceCHANNEL				fltHalChannelMask;
	VGboolean				fltFormatLinear;						/* VG_FILTER_FORMAT_LINEAR */
	VGboolean				fltFormatPremultiplied;					/* VG_FILTER_FORMAT_PREMULTIPLIED */

	/* Implementation limits (read-only). */
	VGint					maxScissorRects;						/* VG_MAX_SCISSOR_RECTS */
	VGint					maxDashCount;							/* VG_MAX_DASH_COUNT */
	VGint					maxKernelSize;							/* VG_MAX_KERNEL_SIZE */
	VGint					maxSeparableKernelSize;					/* VG_MAX_SEPARABLE_KERNEL_SIZE */
	VGint					maxColorRampStops;						/* VG_MAX_COLOR_RAMP_STOPS */
	VGint					maxImageWidth;							/* VG_MAX_IMAGE_WIDTH */
	VGint					maxImageHeight;							/* VG_MAX_IMAGE_HEIGHT */
	VGint					maxImagePixels;							/* VG_MAX_IMAGE_PIXELS */
	VGint					maxImageBytes;							/* VG_MAX_IMAGE_BYTES */
	VGint					maxGaussianDeviation;					/* VG_MAX_GAUSSIAN_STD_DEVIATION */

	/* Object cache. */
	vgsOBJECT_CACHE_PTR		objectCache;

	/* Current VG matrix. */
	VGMatrixMode			matrixMode;								/* VG_MATRIX_MODE */
	vgsMATRIXCONTAINER_PTR	matrix;

	/* VG matrices. */
	vgsMATRIXCONTAINER		pathUserToSurface;
	vgsMATRIXCONTAINER		glyphUserToSurface;
	vgsMATRIXCONTAINER		imageUserToSurface;
	vgsMATRIXCONTAINER		fillPaintToUser;
	vgsMATRIXCONTAINER		strokePaintToUser;

	/* Last set current user-to-surface. */
	vgsMATRIX_PTR			currentUserToSurface;

	/* Translated glyphUserToSurface matrix. */
	vgsMATRIXCONTAINER		translatedGlyphUserToSurface;

	/* DrawPath matrix update functions. */
	vgsMATRIXCONTAINER		pathFillSurfaceToPaint;
	vgsMATRIXCONTAINER		pathStrokeSurfaceToPaint;
	vgsMATRIXCONTAINER		glyphFillSurfaceToPaint;
	vgsMATRIXCONTAINER		glyphStrokeSurfaceToPaint;

	vgsMATRIXCONTAINER_PTR	drawPathFillSurfaceToPaint;
	vgsMATRIXCONTAINER_PTR	drawPathStrokeSurfaceToPaint;

	/* DrawImage matrices. */
	vgsMATRIXCONTAINER		imageFillSurfaceToPaint;
	vgsMATRIXCONTAINER		imageSurfaceToImage;
/*	vgsMATRIXCONTAINER		glyphFillSurfaceToPaint;   <-- same as above. */
	vgsMATRIXCONTAINER		glyphSurfaceToImage;

	vgsMATRIXCONTAINER_PTR	drawImageSurfaceToPaint;
	vgsMATRIXCONTAINER_PTR	drawImageSurfaceToImage;

	/* Path object storage manager. */
	vgsPATHSTORAGE_PTR		pathStorage;
	vgsPATHSTORAGE_PTR		strokeStorage;

	/* Memory manager for ARC coordinates. */
	vgsMEMORYMANAGER_PTR	arcCoordinates;

	/* VG 2.0 presence flag. */
	gctBOOL					vg20;

	/* Filter support flag. */
	gctBOOL					filterSupported;

	/* Software tesselation flag. */
	gctBOOL					conformance;
	gctBOOL					useSoftwareTS;

	/* Image dirty state management. */
	vgeIMAGE_DIRTY			imageDirty;

	/* Temporary image buffer. */
	gctUINT8_PTR			tempBuffer;
	gctUINT					tempBufferSize;

	/* Paint objects. */
	vgsPAINT_PTR			defaultPaint;
	vgsPAINT_PTR			strokePaint;
	vgsPAINT_PTR			fillPaint;
	VGboolean				strokeDefaultPaint;
	VGboolean				fillDefaultPaint;

	/* Signal object for waiting function. */
	gctSIGNAL				waitSignal;

	/* Debug counters. */
	vgmDEFINE_PATH_COUNTER()
	vgmDEFINE_CONTAINER_COUNTER()
	vgmDEFINE_FRAME_COUNTER()
	vgmDEFINE_SET_CONTEXT_COUNTER()
	vgmDEFINE_CLEAR_COUNTER()
	vgmDEFINE_DRAWPATH_COUNTER()
	vgmDEFINE_DRAWIMAGE_COUNTER()
	vgmDEFINE_COPYIMAGE_COUNTER()
	vgmDEFINE_GAUSSIAN_COUNTER()
}
vgsCONTEXT;

/*----------------------------------------------------------------------------*/
/*------------------ Object management function declarations. ----------------*/

gceSTATUS vgfFlushPipe(
	IN vgsCONTEXT_PTR Context,
	IN gctBOOL Finish
	);

vgsTHREADDATA_PTR vgfGetThreadData(
	gctBOOL Create
	);

#ifdef __cplusplus
}
#endif

#endif
