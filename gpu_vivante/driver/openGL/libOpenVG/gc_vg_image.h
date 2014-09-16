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




#ifndef __gc_vg_image_h__
#define __gc_vg_image_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*---------------------------- Image definitions. ----------------------------*/

/* Image quality constants. */
#define vgvIMAGE_QUALITY_ALL \
( \
	VG_IMAGE_QUALITY_NONANTIALIASED \
	| VG_IMAGE_QUALITY_FASTER \
	| VG_IMAGE_QUALITY_BETTER \
)

#define vgvIMAGE_QUALITY_MASK \
( \
	~vgvIMAGE_QUALITY_ALL \
)

/* Default image orientation. */
#define vgvIMAGE_ORIENTATION \
( \
	gcvORIENTATION_BOTTOM_TOP \
)

extern vgtFLOATVECTOR4 vgvFLOATCOLOR0000;
extern vgtBYTEVECTOR4  vgvBYTECOLOR0000;

extern vgtFLOATVECTOR4 vgvFLOATCOLOR0001;
extern vgtBYTEVECTOR4  vgvBYTECOLOR0001;

extern vgtFLOATVECTOR4 vgvFLOATCOLOR1111;
extern vgtBYTEVECTOR4  vgvBYTECOLOR1111;

/*----------------------------------------------------------------------------*/
/*----------------------- Image dirty state management. ----------------------*/

typedef enum _vgeIMAGE_DIRTY
{
	vgvIMAGE_READY        =  0,
	vgvIMAGE_NOT_FLUSHED  = (1 << 0),
	vgvIMAGE_NOT_FINISHED = (1 << 1),
	vgvIMAGE_NOT_READY    =  vgvIMAGE_NOT_FLUSHED | vgvIMAGE_NOT_FINISHED
}
vgeIMAGE_DIRTY;

/*----------------------------------------------------------------------------*/
/*----------------------- Object structure definition. -----------------------*/

typedef struct _vgsIMAGE
{
	/* Embedded object. */
	vgsOBJECT				object;

	/* Object states. */
	VGImageFormat			format;				/* VG_IMAGE_FORMAT */
	gcsSIZE					size;				/* VG_IMAGE_WIDTH/HEIGHT */
	gcsPOINT				origin;
	gctINT					stride;

	/* Allowed image quality. */
	VGImageQuality			allowedQuality;

	/* Image format information. */
	gctBOOL					upsample;
	vgsFORMAT_PTR			wrapperFormat;
	vgsFORMAT_PTR			surfaceFormat;
	gceORIENTATION			orientation;

	/* Image usage flags. */
	gctINT					glyph;
	gctINT					pattern;
	gctINT					renderTarget;

	/* Parent-child states. */
	vgsIMAGE_PTR			parent;
	gctINT					childrenCount;

	/* Image surfrace. */
	gcoSURF					surface;
	gctUINT8_PTR			buffer;

	/* Allocation flags. */
	gctBOOL					imageAllocated;
	gctBOOL					surfaceAllocated;
	gctBOOL					surfaceLocked;

	/* Image dirty state management. */
	vgeIMAGE_DIRTY			imageDirty;
	vgeIMAGE_DIRTY			*imageDirtyPtr;
}
vgsIMAGE;

/*----------------------------------------------------------------------------*/
/*------------------ Object management function declarations. ----------------*/

gceSTATUS vgfReferenceImage(
	vgsCONTEXT_PTR Context,
	vgsIMAGE_PTR * Image
	);

void vgfSetImageObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	);

gceSTATUS vgfInitializeImage(
	vgsCONTEXT_PTR Context,
	vgsIMAGE_PTR Image,
	gcoSURF Surface
	);

gceSTATUS vgfInitializeWrapper(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Wrapper,
	IN VGImageFormat DataFormat,
	IN vgsFORMAT_PTR FormatInfo,
	IN gctINT DataStride,
	IN gctINT Width,
	IN gctINT Height,
	IN gctCONST_POINTER Logical,
	IN gctUINT32 Physical
	);

gceSTATUS vgfInitializeTempImage(
	IN vgsCONTEXT_PTR Context,
	IN VGImageFormat DataFormat,
	IN gctINT Width,
	IN gctINT Height
	);

gceSTATUS vgfCreateImage(
	IN vgsCONTEXT_PTR Context,
	IN VGImageFormat DataFormat,
	IN gctINT Width,
	IN gctINT Height,
	IN VGImageQuality AllowedQuality,
	IN vgsIMAGE_PTR * Image
	);

gceSTATUS vgfReleaseImage(
	vgsCONTEXT_PTR Context,
	vgsIMAGE_PTR Image
	);

gceSTATUS vgfUseImageAsGlyph(
	vgsCONTEXT_PTR Context,
	vgsIMAGE_PTR Image,
	VGboolean Enable
	);

gceSTATUS vgfUseImageAsPattern(
	vgsCONTEXT_PTR Context,
	vgsIMAGE_PTR Image,
	VGboolean Enable
	);

gceSTATUS vgfUseImageAsRenderTarget(
	vgsCONTEXT_PTR Context,
	vgsIMAGE_PTR Image,
	VGboolean Enable
	);

gctBOOL vgfIsImageRenderTarget(
	vgsIMAGE_PTR Image
	);

vgsIMAGE_PTR vgfGetRootImage(
	vgsIMAGE_PTR Image
	);

gctBOOL vgfImagesRelated(
	vgsIMAGE_PTR First,
	vgsIMAGE_PTR Second
	);

gceSTATUS vgfFlushImage(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Image,
	IN gctBOOL Finish
	);

gceSTATUS vgfCPUBlit(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Source,
	IN vgsIMAGE_PTR Target,
	IN gctINT SourceX,
	IN gctINT SourceY,
	IN gctINT TargetX,
	IN gctINT TargetY,
	IN gctINT Width,
	IN gctINT Height,
	IN gctBOOL Dither
	);

gceSTATUS vgfCPUFill(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Target,
	IN gctINT TargetX,
	IN gctINT TargetY,
	IN gctINT Width,
	IN gctINT Height,
	IN gctFLOAT Red,
	IN gctFLOAT Green,
	IN gctFLOAT Blue,
	IN gctFLOAT Alpha
	);

gceSTATUS vgfTesselateImage(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Target,
	IN vgsIMAGE_PTR Image
	);

gceSTATUS vgfDrawImage(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Source,
	IN vgsIMAGE_PTR Target,
	IN gctINT SourceX,
	IN gctINT SourceY,
	IN gctINT TargetX,
	IN gctINT TargetY,
	IN gctINT Width,
	IN gctINT Height,
	IN gceVG_BLEND BlendMode,
	IN gctBOOL ColorTransformEnable,
	IN gctBOOL ScissorEnable,
	IN gctBOOL MaskingEnable,
	IN gctBOOL DitherEnable,
	IN gctBOOL Mask
	);

gceSTATUS vgfFillColor(
	IN vgsCONTEXT_PTR Context,
	IN vgsIMAGE_PTR Target,
	IN gctINT TargetX,
	IN gctINT TargetY,
	IN gctINT Width,
	IN gctINT Height,
	IN vgtFLOATVECTOR4 FloatColor,
	IN vgtBYTEVECTOR4 ByteColor,
	IN gctBOOL ScissorEnable
	);

#ifdef __cplusplus
}
#endif

#endif
