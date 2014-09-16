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




#include "gc_vg_precomp.h"

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

static gceVG_BLEND _blendOperation[] =
{
	/* VG_SET_MASK       : TARGET = SOURCE                          */
	gcvVG_BLEND_SRC,

	/* VG_UNION_MASK     : TARGET = 1 - (1 - SOURCE) * (1 - TARGET) */
	gcvVG_BLEND_DST_OVER,

	/* VG_INTERSECT_MASK : TARGET = TARGET * SOURCE                 */
	gcvVG_BLEND_DST_IN,

	/* VG_SUBTRACT_MASK  : TARGET = TARGET * (1 - SOURCE)           */
	gcvVG_BLEND_SUBTRACT
};


/*******************************************************************************
**
** _MaskDestructor
**
** Mask object destructor. Called when the object is being destroyed.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Object
**       Pointer to the object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _MaskDestructor(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		/* Cast the object. */
		vgsMASK_PTR maskLayer = (vgsMASK_PTR) Object;

		/* Destroy layer surface. */
		gcmERR_BREAK(vgfReleaseImage(Context, &maskLayer->image));
	}
	while (gcvFALSE);

	return status;
}


/*******************************************************************************
**
** _ModifyMask
**
** Modify the drawing surface mask values according to a given operation.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Image
**       Poiter to the source image.
**
**    Operation
**       Operation to be performed on the mask.
**
**    X, Y
**    Width, Height
**       The affected region of the current drawing surface mask.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _ModifyMask(
	vgsCONTEXT_PTR Context,
	vgsIMAGE_PTR Image,
	VGMaskOperation Operation,
	VGint X,
	VGint Y,
	VGint Width,
	VGint Height
	)
{
	gceSTATUS status;

	do
	{
		/* VG_FILL_MASK: TARGET = 0 */
		if (Operation == VG_CLEAR_MASK)
		{
			gcmERR_BREAK(vgfFillColor(
				Context,
				&Context->maskImage,
				X, Y, Width, Height,
				vgvFLOATCOLOR0000,
				vgvBYTECOLOR0000,
				gcvFALSE
				));
		}

		/* VG_FILL_MASK: TARGET = 1 */
		else if (Operation == VG_FILL_MASK)
		{
			gcmERR_BREAK(vgfFillColor(
				Context,
				&Context->maskImage,
				X, Y, Width, Height,
				vgvFLOATCOLOR0001,
				vgvBYTECOLOR0001,
				gcvFALSE
				));
		}

		else
		{
			/* Determine the operation. */
			gceVG_BLEND operation = _blendOperation[Operation - VG_SET_MASK];

			gcmERR_BREAK(vgfDrawImage(
				Context,
				Image,
				&Context->maskImage,
				0, 0,
				X, Y, Width, Height,
				operation,
				Context->colTransform,
				gcvFALSE,
				gcvFALSE,
				gcvFALSE,
				gcvTRUE
				));
		}

		/* Invalidate mask. */
		Context->maskDirty = gcvTRUE;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _RenderToMask
**
** Modify the drawing surface mask values according to a given operation.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Path
**       Pointer to the path to render.
**
**    PaintModes
**       Bitwise OR of VG_FILL_PATH and/or VG_STROKE_PATH (VGPaintMode).
**
**    Operation
**       Operation to perform on the current drawing surface mask.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _RenderToMask(
	vgsCONTEXT_PTR Context,
	vgsPATH_PTR Path,
	VGbitfield PaintModes,
	VGMaskOperation Operation
	)
{
	gceSTATUS status;

	do
	{
		/* Get the size of the mask surface. */
		gctINT32 width  = Context->maskImage.size.width;
		gctINT32 height = Context->maskImage.size.height;

		/* VG_CLEAR_MASK: TARGET = 0 */
		if (Operation == VG_CLEAR_MASK)
		{
			gcmERR_BREAK(vgfFillColor(
				Context,
				&Context->maskImage,
				0, 0, width, height,
				vgvFLOATCOLOR0000,
				vgvBYTECOLOR0000,
				gcvFALSE
				));
		}

		/* VG_FILL_MASK: TARGET = 1 */
		else if (Operation == VG_FILL_MASK)
		{
			gcmERR_BREAK(vgfFillColor(
				Context,
				&Context->maskImage,
				0, 0, width, height,
				vgvFLOATCOLOR0001,
				vgvBYTECOLOR0001,
				gcvFALSE
				));
		}

		else
		{
			/* Determine the operation. */
			gceVG_BLEND operation = _blendOperation[Operation - VG_SET_MASK];

			/* Set the temporary mask image pointer. */
			vgsIMAGE_PTR tempMaskImage = &Context->tempMaskImage;

			/* Allocate temporary mask. */
			if (tempMaskImage->surface == gcvNULL)
			{
				/* Create the image. */
				gcmERR_BREAK(vgfCreateImage(
					Context,
					VG_A_8,
					width, height,
					vgvIMAGE_QUALITY_ALL,
					&tempMaskImage
					));
			}

			/* Set the proper matrices. */
			Context->drawPathFillSurfaceToPaint
				= &Context->pathFillSurfaceToPaint;

			Context->drawPathStrokeSurfaceToPaint
				= &Context->pathStrokeSurfaceToPaint;

			if ((PaintModes & VG_FILL_PATH) == VG_FILL_PATH)
			{
				/* Reset the temporaty mask. */
				gcmERR_BREAK(vgfFillColor(
					Context,
					tempMaskImage,
					0, 0, width, height,
					vgvFLOATCOLOR0000,
					vgvBYTECOLOR0000,
					gcvFALSE
					));

				/* Draw the path into the temporary mask. */
				gcmERR_BREAK(vgfDrawPath(
					Context,
					tempMaskImage,
					Path,
					VG_FILL_PATH,
					Context->defaultPaint,
					Context->defaultPaint,
					gcvFALSE,
					gcvFALSE
					));

				/* Blit the image with the specified operation. */
				gcmERR_BREAK(vgfDrawImage(
					Context,
					tempMaskImage,
					&Context->maskImage,
					0, 0, 0, 0, width, height,
					operation,
					gcvFALSE,
					gcvFALSE,
					gcvFALSE,
					gcvFALSE,
					gcvTRUE
					));
			}

			if ((PaintModes & VG_STROKE_PATH) == VG_STROKE_PATH)
			{
				/* Reset the temporaty mask. */
				gcmERR_BREAK(vgfFillColor(
					Context,
					tempMaskImage,
					0, 0, width, height,
					vgvFLOATCOLOR0000,
					vgvBYTECOLOR0000,
					gcvFALSE
					));

				/* Draw the path into the temporary mask. */
				gcmERR_BREAK(vgfDrawPath(
					Context,
					tempMaskImage,
					Path,
					VG_STROKE_PATH,
					Context->defaultPaint,
					Context->defaultPaint,
					gcvFALSE,
					gcvFALSE
					));

				/* Blit the image with the specified operation. */
				gcmERR_BREAK(vgfDrawImage(
					Context,
					tempMaskImage,
					&Context->maskImage,
					0, 0, 0, 0, width, height,
					operation,
					gcvFALSE,
					gcvFALSE,
					gcvFALSE,
					gcvFALSE,
					gcvTRUE
					));
			}
		}

		/* Invalidate mask. */
		Context->maskDirty = gcvTRUE;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
************************* Internal Mask Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgfReferenceMask
**
** vgfReferenceMask increments the reference count of the given VGMaskLayer
** object. If the object does not exist yet, the object will be created and
** initialized with the default values.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    vgsMASK_PTR * Mask
**       Handle to the new VGMaskLayer object.
*/

gceSTATUS vgfReferenceMask(
	vgsCONTEXT_PTR Context,
	vgsMASK_PTR * Mask
	)
{
	gceSTATUS status, last;
	vgsMASK_PTR mask = gcvNULL;

	do
	{
		/* Create the object if does not exist yet. */
		if (*Mask == gcvNULL)
		{
			/* Allocate the object structure. */
			gcmERR_BREAK(gcoOS_Allocate(
				Context->os,
				gcmSIZEOF(vgsMASK),
				(gctPOINTER *) &mask
				));

			/* Initialize the object structure. */
			mask->object.type           = vgvOBJECTTYPE_MASK;
			mask->object.prev           = gcvNULL;
			mask->object.next           = gcvNULL;
			mask->object.referenceCount = 0;
			mask->object.userValid      = VG_TRUE;

			/* Insert the object into the cache. */
			gcmERR_BREAK(vgfObjectCacheInsert(
				Context, (vgsOBJECT_PTR) mask
				));

			/* Set default object states. */
			mask->image.imageAllocated   = gcvFALSE;
			mask->image.surfaceAllocated = gcvFALSE;
			mask->image.surfaceLocked    = gcvFALSE;

			/* Set the result pointer. */
			*Mask = mask;
		}

		/* Increment the reference count. */
		(*Mask)->object.referenceCount++;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Roll back. */
	if (mask != gcvNULL)
	{
		gcmCHECK_STATUS(gcoOS_Free(Context->os, mask));
	}

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgfSetMaskObjectList
**
** Initialize object cache list information.
**
** INPUT:
**
**    ListEntry
**       Entry to initialize.
**
** OUTPUT:
**
**    Nothing.
*/

void vgfSetMaskObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	)
{
	ListEntry->stateArray     = gcvNULL;
	ListEntry->stateArraySize = 0;
	ListEntry->destructor     = _MaskDestructor;
}


/******************************************************************************\
************************** OpenVG Paint Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgCreateMaskLayer
**
** vgCreateMaskLayer creates an object capable of storing a mask layer with
** the given width and height and returns a VGMaskLayer handle to it. The mask
** layer is defined to be compatible with the format and multisampling
** properties of the current drawing surface. If there is no current drawing
** surface, no mask is configured for the current drawing surface, or an error
** occurs, VG_INVALID_HANDLE is returned. All mask layer values are initially
** set to one.
**
** INPUT:
**
**    Width, Height
**       Dimensions of the mask layer to be created.
**
** OUTPUT:
**
**    VGMaskLayer
**       Handle to the new VGMaskLayer object.
*/

VG_API_CALL VGMaskLayer VG_API_ENTRY vgCreateMaskLayer(
	VGint Width,
	VGint Height
	)
{
	vgsMASK_PTR maskLayer = gcvNULL;

	vgmENTERAPI(vgCreateMaskLayer)
	{
		gceSTATUS status;
		vgsIMAGE_PTR maskImage;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(%d, %d);\n",
			__FUNCTION__,
			Width, Height
			);

		/* Validate the arguments. */
		if ((Width  <= 0) || (Width  > Context->maxImageWidth) ||
			(Height <= 0) || (Height > Context->maxImageHeight))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		if (Width * Height > Context->maxImagePixels)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Allocate the mask layer object. */
		gcmERR_GOTO(vgfReferenceMask(
			Context, &maskLayer
			));

		/* Set a pointer to the image. */
		maskImage = &maskLayer->image;

		/* Create the image. */
		gcmERR_GOTO(vgfCreateImage(
			Context,
			VG_A_8,
			Width, Height,
			vgvIMAGE_QUALITY_ALL,
			&maskImage
			));

		/* Set the mask to one. */
		gcmERR_GOTO(vgfFillColor(
			Context,
			maskImage,
			0, 0, Width, Height,
			vgvFLOATCOLOR0001,
			vgvBYTECOLOR0001,
			gcvFALSE
			));

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s() = ;\n",
			__FUNCTION__,
			maskLayer
			);

		/* Success. */
		break;

ErrorHandler:

		/* Roll back. */
		gcmVERIFY_OK(vgfDereferenceObject(
			Context,
			(vgsOBJECT_PTR *) &maskLayer
			));

		vgmERROR(VG_OUT_OF_MEMORY_ERROR);
	}
	vgmLEAVEAPI(vgCreateMaskLayer);

	return (VGMaskLayer) maskLayer;
}


/*******************************************************************************
**
** vgDestroyMaskLayer
**
** The resources associated with a mask layer may be deallocated by calling
** vgDestroyMaskLayer. Following the call, the maskLayer handle is no longer
** valid in the current context.
**
** INPUT:
**
**    MaskLayer
**       Handle to a valid VGMaskLayer object.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgDestroyMaskLayer(
	VGMaskLayer MaskLayer
	)
{
	vgmENTERAPI(vgDestroyMaskLayer)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X);\n",
			__FUNCTION__,
			MaskLayer
			);

		if (!vgfVerifyUserObject(Context, MaskLayer, vgvOBJECTTYPE_MASK))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Invalidate the object. */
		((vgsOBJECT_PTR) MaskLayer)->userValid = VG_FALSE;

		/* Decrement the reference count. */
		gcmVERIFY_OK(vgfDereferenceObject(
			Context,
			(vgsOBJECT_PTR *) &MaskLayer
			));
	}
	vgmLEAVEAPI(vgDestroyMaskLayer);
}


/*******************************************************************************
**
** vgFillMaskLayer
**
** The vgFillMaskLayer function sets the values of a given maskLayer within
** a given rectangular region to a given value. The floating-point value must
** be between 0 and 1. The value is rounded to the closest available value
** supported by the mask layer. If two values are equally close, the larger
** value is used.
**
** INPUT:
**
**    MaskLayer
**       Handle to a valid VGMaskLayer object.
**
**    X, Y
**       Width, Height
**       Rectangular area of the mask layer to be affected.
**
**    Value
**       Value to fill the area with.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgFillMaskLayer(
	VGMaskLayer MaskLayer,
	VGint X,
	VGint Y,
	VGint Width,
	VGint Height,
	VGfloat Value
	)
{
	vgmENTERAPI(vgFillMaskLayer)
	{
		vgsMASK_PTR maskLayer;
		vgtFLOATVECTOR4 floatColor;
		vgtBYTEVECTOR4 byteColor;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d, %d, %d, %d, %.10ff);\n",
			__FUNCTION__,
			MaskLayer, X, Y, Width, Height, Value
			);

		if (!vgfVerifyUserObject(Context, MaskLayer, vgvOBJECTTYPE_MASK))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		maskLayer = (vgsMASK_PTR) MaskLayer;

		/* Validate the arguments. */
		if ((Value < 0.0f) || (Value > 1.0f))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		if ((X < 0) || (Width  <= 0) ||
			(Y < 0) || (Height <= 0))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		if ((X > maskLayer->image.size.width  - Width) ||
			(Y > maskLayer->image.size.height - Height))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Set colors. */
		floatColor[0] = 0.0f;
		floatColor[1] = 0.0f;
		floatColor[2] = 0.0f;
		floatColor[3] = Value;

		byteColor[0] = 0x00;
		byteColor[1] = 0x00;
		byteColor[2] = 0x00;
		byteColor[3] = gcoVG_PackColorComponent(Value);

		/* Fill the image. */
		gcmVERIFY_OK(vgfFillColor(
			Context,
			&maskLayer->image,
			X, Y, Width, Height,
			floatColor,
			byteColor,
			gcvFALSE
			));
	}
	vgmLEAVEAPI(vgFillMaskLayer);
}


/*******************************************************************************
**
** vgMask
**
** The vgMask function modifies the drawing surface mask values according to
** a given operation, possibly using coverage values taken from a mask layer
** or bitmap image given by the Mask parameter. If no mask is configured for
** the current drawing surface, vgMask has no effect.
**
** The affected region is the intersection of the drawing surface bounds with
** the rectangle extending from pixel (X, Y) of the drawing surface and having
** the given width and height in pixels. For operations that make use of the
** Mask parameter (i.e., operations other than VG_CLEAR_MASK and VG_FILL_MASK),
** mask pixels starting at (0, 0) are used, and the region is further limited
** to the width and height of mask. For the VG_CLEAR_MASK and VG_FILL_MASK
** operations, the mask parameter is ignored and does not affect the region
** being modified. The value VG_INVALID_HANDLE may be supplied in place of
** an actual image handle.
**
** If Mask is a VGImage handle, the image defines coverage values at each of
** its pixels as follows. If the image pixel format includes an alpha channel,
** the alpha channel is used. Otherwise, values from the red (for color image
** formats) or grayscale (for grayscale formats) channel are used. The value
** is divided by the maximum value for the channel to obtain a value between
** 0 and 1. If the image is bi-level (black and white), black pixels receive
** a value of 0 and white pixels receive a value of 1.
**
** If Mask is a VGMaskLayer handle, it must be compatible with the current
** drawing surface mask. If the drawing surface mask is multisampled, this
** operation may perform dithering. That is, it may assign different values
** to different drawing surface mask samples within a pixel so that the average
** mask value for the pixel will match the incoming value more accurately.
**
** INPUT:
**
**    Mask
**       Handle to a VGImage or VGMaskLayer object to be used as the source
**       in the specified operation.
**
**    Operation
**       Operation to perform on the current drawing surface mask. One of:
**          VG_CLEAR_MASK
**          VG_FILL_MASK
**          VG_SET_MASK
**          VG_UNION_MASK
**          VG_INTERSECT_MASK
**          VG_SUBTRACT_MASK
**
**    X, Y
**    Width, Height
**       The affected region of the current drawing surface mask.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgMask(
	VGHandle Mask,
	VGMaskOperation Operation,
	VGint X,
	VGint Y,
	VGint Width,
	VGint Height
	)
{
	vgmENTERAPI(vgMask)
	{
		vgsIMAGE_PTR source = gcvNULL;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%04X, %d, %d, %d, %d);\n",
			__FUNCTION__,
			Mask, X, Y, Width, Height
			);

		/* Validate mask operation. */
		if ((Operation < VG_CLEAR_MASK) || (Operation > VG_SUBTRACT_MASK))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the rectangle size. */
		if ((Width <= 0) || (Height <= 0))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the arguments. */
		if ((Operation != VG_CLEAR_MASK) &&
			(Operation != VG_FILL_MASK))
		{
			vgsOBJECT_PTR object;

			/* Verify the object. */
			if (!vgfVerifyUserObject(Context, Mask, vgvOBJECTTYPE_COUNT))
			{
				vgmERROR(VG_BAD_HANDLE_ERROR);
				break;
			}

			/* Cast the object. */
			object = (vgsOBJECT_PTR) Mask;

			/* Is it a mask layer? */
			if (object->type == vgvOBJECTTYPE_MASK)
			{
				/* Cast the object. */
				vgsMASK_PTR mask = (vgsMASK_PTR) object;

				/* Cast the object. */
				source = &mask->image;
			}

			/* Is it an image? */
			else if (object->type == vgvOBJECTTYPE_IMAGE)
			{
				/* Cast the object. */
				vgsIMAGE_PTR image = (vgsIMAGE_PTR) object;

				/* Cannot be the rendering target. */
				if (vgfIsImageRenderTarget(image))
				{
					vgmERROR(VG_IMAGE_IN_USE_ERROR);
					break;
				}

				/* Get the surface pointer. */
				source = image;
			}

			/* Not supported type. */
			else
			{
				vgmERROR(VG_BAD_HANDLE_ERROR);
				break;
			}
		}

		/* Perform the operation. */
		gcmVERIFY_OK(_ModifyMask(
			Context, source, Operation, X, Y, Width, Height
			));
	}
	vgmLEAVEAPI(vgMask);
}


/*******************************************************************************
**
** vgRenderToMask
**
** The vgRenderToMask function modifies the current surface mask by applying
** the given operation to the set of coverage values associated with the
** rendering of the given path. If PaintModes contains VG_FILL_PATH, the path
** is filled; if it contains VG_STROKE_PATH, the path is stroked. If both are
** present, the mask operation is performed in two passes, first on the filled
** path geometry, then on the stroked path geometry.
**
** Conceptually, for each pass, an intermediate single-channel image is
** initialized to 0, then filled with those coverage values that would result
** from the first four stages of the OpenVG pipeline (i.e., state setup,
** stroked path generation if applicable, transformation, and rasterization)
** when drawing a path with vgDrawPath using the given set of paint modes and
** all current OpenVG state settings that affect path rendering (scissor
** rectangles, rendering quality, fill rule, stroke parameters, etc.). Paint
** settings (e.g., paint matrices) are ignored. Finally, the drawing surface
** mask is modified as though vgMask were called using the intermediate image
** as the mask parameter. Changes to path following this call do not affect
** the mask. If operation is VG_CLEAR_MASK or VG_FILL_MASK, path is ignored
** and the entire mask is affected.
**
** An implementation that supports geometric clipping of primitives may cache
** the contents of path and make use of it directly when primitives are drawn,
** without generating a rasterized version of the clip mask. Other
** implementation-specific optimizations may be used to avoid materializing
** a full intermediate mask image.
**
** INPUT:
**
**    Path
**       Handle to a VGImage or VGMaskLayer object to be used as the source
**       in the specified operation.
**
**    PaintModes
**       Bitwise OR of VG_FILL_PATH and/or VG_STROKE_PATH (VGPaintMode).
**
**    Operation
**       Operation to perform on the current drawing surface mask. One of:
**          VG_CLEAR_MASK
**          VG_FILL_MASK
**          VG_SET_MASK
**          VG_UNION_MASK
**          VG_INTERSECT_MASK
**          VG_SUBTRACT_MASK
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgRenderToMask(
	VGPath Path,
	VGbitfield PaintModes,
	VGMaskOperation Operation
	)
{
	vgmENTERAPI(vgRenderToMask)
	{
		vgsPATH_PTR path;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X, 0x%04X);\n",
			__FUNCTION__,
			Path, PaintModes, Operation
			);

		/* Verify the object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Validate paint modes. */
		if ((PaintModes == 0) ||
			((PaintModes & ~(VG_STROKE_PATH | VG_FILL_PATH)) != 0))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate mask operation. */
		if ((Operation < VG_CLEAR_MASK) || (Operation > VG_SUBTRACT_MASK))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Cast the object. */
		path = (vgsPATH_PTR) Path;

		/* Perform the operation. */
		gcmVERIFY_OK(_RenderToMask(
			Context, path, PaintModes, Operation
			));
	}
	vgmLEAVEAPI(vgRenderToMask);
}


/*******************************************************************************
**
** vgCopyMask
**
** vgCopyMask copies a portion of the current surface mask into a VGMaskLayer
** object. The source region starts at (SourceX, SourceY) in the surface mask,
** and the destination region starts at (TargetX, TargetY) in the
** destination MaskLayer. The copied region is clipped to the given width and
** height and the bounds of the source and destination. If the current context
** does not contain a surface mask, vgCopyMask does nothing.
**
** INPUT:
**
**    MaskLayer
**       Handle to a valid VGMaskLayer object.
**
**    TargetX, TargetY
**       Destination region origin.
**
**    SourceX, SourceY
**       Source region origin.
**
**    Width, Height
**       The size of the area to be copied.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgCopyMask(
	VGMaskLayer MaskLayer,
	VGint TargetX,
	VGint TargetY,
	VGint SourceX,
	VGint SourceY,
	VGint Width,
	VGint Height
	)
{
	vgmENTERAPI(vgCopyMask)
	{
		vgsMASK_PTR maskLayer;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d, %d, %d, %d, %d, %d);\n",
			__FUNCTION__,
			MaskLayer,
			TargetX, TargetY, SourceX, SourceY, Width, Height
			);

		/* Verify the object. */
		if (!vgfVerifyUserObject(Context, MaskLayer, vgvOBJECTTYPE_MASK))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Validate the rectangle size. */
		if ((Width <= 0) || (Height <= 0))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Cast mask layer object. */
		maskLayer = (vgsMASK_PTR) MaskLayer;

		/* Copy the mask. */
		gcmVERIFY_OK(vgfDrawImage(
			Context,
			&Context->maskImage,
			&maskLayer->image,
			SourceX, SourceY,
			TargetX, TargetY,
			Width, Height,
			gcvVG_BLEND_SRC,
			Context->colTransform,
			gcvFALSE,
			gcvFALSE,
			gcvFALSE,
			gcvTRUE
			));
	}
	vgmLEAVEAPI(vgCopyMask);
}


/*******************************************************************************
**
** vgClear
**
** The vgClear function fills the portion of the drawing surface intersecting
** the rectangle extending from pixel (x, y) and having the given width and
** height with a constant color value, taken from the VG_CLEAR_COLOR parameter.
** The color value is expressed in non-premultiplied sRGBA (sRGB color plus
** alpha) format. Values outside the [0, 1] range are interpreted as the
** nearest endpoint of the range. The color is converted to the destination
** color space in the same manner as if a rectangular path were being filled.
** Clipping and scissoring take place in the usual fashion, but antialiasing,
** masking, and blending do not occur.
**
** INPUT:
**
**    X, Y
**    Width, Height
**       Rectangular area of the drawing surface to be cleared.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgClear(
	VGint X,
	VGint Y,
	VGint Width,
	VGint Height
	)
{
	vgmENTERAPI(vgClear)
	{
		vgmADVANCE_CLEAR_COUNT(Context);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(%d, %d, %d, %d);\n",
			__FUNCTION__,
			X, Y, Width, Height
			);

		/* Validate the rectangle size. */
		if ((Width <= 0) || (Height <= 0))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Clear the area. */
		gcmVERIFY_OK(vgfFillColor(
			Context,
			&Context->targetImage,
			X, Y, Width, Height,
			Context->clearColor,
			Context->halClearColor,
			Context->scissoringEnable
			));
	}
	vgmLEAVEAPI(vgClear);
}
