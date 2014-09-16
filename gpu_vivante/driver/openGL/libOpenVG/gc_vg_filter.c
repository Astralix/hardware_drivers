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

/* Color matrix component formula. */
#define vgmCOMPUTECOLORMATRIX(Matrix, Target, Source, Index) \
	Target[Index] \
		= Matrix[Index + 4 * 0] * Source[0] \
		+ Matrix[Index + 4 * 1] * Source[1] \
		+ Matrix[Index + 4 * 2] * Source[2] \
		+ Matrix[Index + 4 * 3] * Source[3] \
		+ Matrix[Index + 4 * 4]

/* Maximum kernel size for Gaussian blur. */
#define vgvMAXBLURKERNEL \
	((vgvMAX_GAUSSIAN_STD_DEVIATION * 4 + 1) * 2 + 1)


/*******************************************************************************
**
** _GetTileMode
**
** Translate tiling mode from VG to HAL.
**
** INPUT:
**
**    TilingMode
**       Tiling mode in VGTilingMode format.
**
** OUTPUT:
**
**    Tiling mode in gceTILE_MODE format.
*/

vgmINLINE gceTILE_MODE _GetTileMode(
	VGTilingMode TilingMode
	)
{
	static gceTILE_MODE _tilingMode[] =
	{
		gcvTILE_FILL,		/* VG_TILE_FILL    */
		gcvTILE_PAD,		/* VG_TILE_PAD     */
		gcvTILE_REPEAT,		/* VG_TILE_REPEAT  */
		gcvTILE_REFLECT		/* VG_TILE_REFLECT */
	};

	/* Determine the value index. */
	gctINT index = TilingMode - VG_TILE_FILL;

	/* Out of range? */
	if (gcmIS_VALID_INDEX(index, _tilingMode))
	{
		return _tilingMode[index];
	}
	else
	{
		return -1;
	}
}


/*******************************************************************************
**
** _Mod
**
** Computes modulo value.
**
** INPUT:
**
**    Value1
**    Value2
**       Function arguements.
**
** OUTPUT:
**
**    Modulo value.
*/

vgmINLINE gctINT _Mod(
	gctINT Value1,
	gctINT Value2
	)
{
	gctINT result = Value1 % Value2;

	if (result < 0)
	{
		result += Value2;
	}

	return result;
}


/*******************************************************************************
**
** _ComponentToIndex
**
** Convert the specified color component in floating point to an 8-bit index
** value in 0..255 range.
**
** INPUT:
**
**    Value
**       Component value to be converted.
**
** OUTPUT:
**
**    Index value.
*/

vgmINLINE static gctUINT8 _ComponentToIndex(
	gctFLOAT Value
	)
{
	/* Convert to 0..255 range and round. */
	gctFLOAT roundedValue
		= Value * 255.0f + 0.5f;

	/* Clamp the value. */
	gctUINT8 clampedValue
		= (gctUINT8) gcmMIN(gcmMAX((gctUINT) floor(roundedValue), 0), 255);

	/* Return result. */
	return clampedValue;
}


/*******************************************************************************
**
** _IndexToComponent
**
** Convert the specified 8-bit index in 0..255 range to floating point format.
**
** INPUT:
**
**    Value
**       Index value.
**
** OUTPUT:
**
**    Color component value in floating point format.
*/

vgmINLINE static gctFLOAT _IndexToComponent(
	gctUINT8 Index
	)
{
	/* Convert back to float. */
	gctFLOAT resultValue = ((gctFLOAT) Index) / 255.0f;

	/* Return result. */
	return resultValue;
}


/*******************************************************************************
**
** _LookupComponent
**
** Lookup the specified color component in floating point in the specified
** table and convert back to floating point.
**
** INPUT:
**
**    Table
**       Pointer to the 256-item lookup table.
**
**    Value
**       Component value.
**
** OUTPUT:
**
**    Result component value.
*/

vgmINLINE static gctFLOAT _LookupComponent(
	const VGubyte * Table,
	gctFLOAT Value
	)
{
	/* Get the index value. */
	gctUINT8 index = _ComponentToIndex(Value);

	/* Lookup the 8-bit value. */
	gctUINT8 lookupValue = Table[index];

	/* Convert back to float. */
	gctFLOAT resultValue = _IndexToComponent(lookupValue);

	/* Return result. */
	return resultValue;
}


/*******************************************************************************
**
** _ConvertColor32
**
** Convert the specified color value in RGBA8 format to floating point.
**
** INPUT:
**
**    Value
**       Color value to be converted.
**
** OUTPUT:
**
**    Result
**       Return color value in floating point format.
*/

vgmINLINE static void _ConvertColor32(
	gctUINT32 Value,
	vgtFLOATVECTOR4 Result
	)
{
	/* Extract components. */
	gctUINT8 red   = (gctUINT8) (Value >> 24);
	gctUINT8 green = (gctUINT8) (Value >> 16);
	gctUINT8 blue  = (gctUINT8) (Value >>  8);
	gctUINT8 alpha = (gctUINT8)  Value;

	/* Determine the result. */
	Result[0] = _IndexToComponent(red);
	Result[1] = _IndexToComponent(green);
	Result[2] = _IndexToComponent(blue);
	Result[3] = _IndexToComponent(alpha);
}


/*******************************************************************************
**
** _NormalizeSource
**
** Normalize the source image.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Source
**       Pointer to the source image.
**
**    Width, Height
**       The size of the image to normalize.
**
** OUTPUT:
**
**    Not zero if the segments intersect.
*/

static gceSTATUS _NormalizeSource(
	vgsCONTEXT_PTR Context,
	vgsIMAGE_PTR Source,
	gctINT32 Width,
	gctINT32 Height
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		gctUINT bufferSize;
		vgsPIXELWALKER srcPixel;
		gctUINT readPixelIndex;
		vgtREAD_PIXEL readPixel;
		vgtFLOATVECTOR4 * tempLine;
		vgtFLOATVECTOR4 * tempPixel;
		gctINT32 x, y;

		/* Determine the required buffer size. */
		bufferSize = Width * Height * gcmSIZEOF(vgtFLOATVECTOR4);

		/* Do we have enough space in the current temporary buffer? */
		if (Context->tempBufferSize < bufferSize)
		{
			/* Free the existing buffer. */
			if (Context->tempBuffer != gcvNULL)
			{
				gcmERR_BREAK(gcoOS_Free(Context->os, Context->tempBuffer));
				Context->tempBuffer     = gcvNULL;
				Context->tempBufferSize = 0;
			}

			/* Allocate a new buffer. */
			gcmERR_BREAK(gcoOS_Allocate(
				Context->os,
				bufferSize,
				(gctPOINTER) &Context->tempBuffer
				));

			/* Set new size. */
			Context->tempBufferSize = bufferSize;
		}

		/* Flush the image. */
		gcmERR_BREAK(vgfFlushImage(Context, Source, gcvTRUE));

		/* Determine the pixel access routines. */
		readPixelIndex
			= (Context->fltFormatLinear << 1)
			|  Context->fltFormatPremultiplied;

		/* Determine the read pixel routine. */
		readPixel = Source->surfaceFormat->readPixel[readPixelIndex];

		/* Initialize the walker. */
		vgsPIXELWALKER_Initialize(&srcPixel, Source, 0, 0);

		/* Set the initial line in the temporary buffer. */
		tempLine = (vgtFLOATVECTOR4 *) Context->tempBuffer;

		/* Walk the pixels. */
		for (y = 0; y < Height; y++)
		{
			/* Set the initial pixel in the current line. */
			tempPixel = tempLine;

			/* Set the initial source pixel. */
			for (x = 0; x < Width; x++)
			{
				/* Read the pixel and advance to the next. */
				readPixel(&srcPixel, *tempPixel);

				/* Advance to the next pixel. */
				tempPixel += 1;
			}

			/* Advance to the next line. */
			vgsPIXELWALKER_NextLine(&srcPixel);
			tempLine += Width;
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _ReadTempPixel
**
** Functions to read source from the temporary buffer.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    Not zero if the segments intersect.
*/

static void _ReadTempPixel(
	IN gctINT X,
	IN gctINT Y,
	IN gctINT Width,
	IN gctINT Height,
	IN gceTILE_MODE TilingMode,
	IN vgtFLOATVECTOR4 * Image,
	IN vgtFLOATVECTOR4 FillColor,
	OUT vgtFLOATVECTOR4 PixelColor
	)
{
	gctFLOAT_PTR pixel;

	if ((X < 0) || (X >= Width) ||
		(Y < 0) || (Y >= Height))
	{
		switch (TilingMode)
		{
		case gcvTILE_FILL:
			PixelColor[0] = FillColor[0];
			PixelColor[1] = FillColor[1];
			PixelColor[2] = FillColor[2];
			PixelColor[3] = FillColor[3];
			break;

		case gcvTILE_PAD:
			X = gcmMIN(gcmMAX(X, 0), Width  - 1);
			Y = gcmMIN(gcmMAX(Y, 0), Height - 1);

			pixel = Image[Y * Width + X];

			PixelColor[0] = pixel[0];
			PixelColor[1] = pixel[1];
			PixelColor[2] = pixel[2];
			PixelColor[3] = pixel[3];
			break;

		case gcvTILE_REPEAT:
			X = _Mod(X, Width);
			Y = _Mod(Y, Height);

			pixel = Image[Y * Width + X];

			PixelColor[0] = pixel[0];
			PixelColor[1] = pixel[1];
			PixelColor[2] = pixel[2];
			PixelColor[3] = pixel[3];
			break;

		case gcvTILE_REFLECT:
			X = _Mod(X, Width  * 2);
			Y = _Mod(Y, Height * 2);

			if (X >= Width)  X = Width  * 2 - 1 - X;
			if (Y >= Height) Y = Height * 2 - 1 - Y;

			pixel = Image[Y * Width + X];

			PixelColor[0] = pixel[0];
			PixelColor[1] = pixel[1];
			PixelColor[2] = pixel[2];
			PixelColor[3] = pixel[3];
			break;

		default:
			gcmASSERT(gcvFALSE);
		}
	}
	else
	{
		pixel = Image[Y * Width + X];

		PixelColor[0] = pixel[0];
		PixelColor[1] = pixel[1];
		PixelColor[2] = pixel[2];
		PixelColor[3] = pixel[3];
	}
}


/******************************************************************************\
*************************** OpenVG Image Filtering API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgColorMatrix
**
** The vgColorMatrix function computes a linear combination of color and alpha
** values (Rsrc, Gsrc, Bsrc, Asrc) from the normalized source image SourceImage
** at each pixel:
**
**    | Rdst |   | m00 m01 m02 m03 |   | Rsrc |   | m04 |
**    | Gdst | = | m10 m11 m12 m13 | * | Gsrc | + | m14 |
**    | Bdst |   | m20 m21 m22 m23 |   | Bsrc |   | m24 |
**    | Adst |   | m30 m31 m32 m33 |   | Asrc |   | m34 |
**
** or:
**
**    Rdst = m00 * Rsrc + m01 * Gsrc + m02 * Bsrc + m03 * Asrc + m04
**    Gdst = m10 * Rsrc + m11 * Gsrc + m12 * Bsrc + m13 * Asrc + m14
**    Bdst = m20 * Rsrc + m21 * Gsrc + m22 * Bsrc + m23 * Asrc + m24
**    Adst = m30 * Rsrc + m31 * Gsrc + m32 * Bsrc + m33 * Asrc + m34
**
** The matrix entries are supplied in the Matrix argument in the order
**
**    {
**       m00, m10, m20, m30,
**       m01, m11, m21, m31,
**       m02, m12, m22, m32,
**       m03, m13, m23, m33,
**       m04, m14, m24, m34
**    }
**
** INPUT:
**
**    DestinationImage
**       Handle to a valid VGImage object.
**
**    SourceImage
**       Handle to a valid VGImage object.
**
**    Matrix
**       Poinnter to the transformation matrix.
**
** OUTPUT:
**
**    DestinationImage
**       Computed image.
*/

VG_API_CALL void VG_API_ENTRY vgColorMatrix(
	VGImage DestinationImage,
	VGImage SourceImage,
	const VGfloat * Matrix
	)
{
	vgmENTERAPI(vgColorMatrix)
	{
		vgsIMAGE_PTR source;
		vgsIMAGE_PTR target;
		gctINT32 width, height;

		/* Software filter variables. */
		gctINT32 x, y;
		gctUINT channelMask;
		gctUINT readPixelIndex;
		gctUINT writePixelIndex;
		vgtREAD_PIXEL readPixel;
		vgtWRITE_PIXEL writePixel;
		vgsPIXELWALKER srcPixel;
		vgsPIXELWALKER trgPixel;
		vgtFLOATVECTOR4 srcValue;
		vgtFLOATVECTOR4 trgValue;

		vgmDUMP_FLOAT_ARRAY(
			Matrix, 9
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X, Matrix);\n",
			__FUNCTION__,
			DestinationImage, SourceImage
			);

		if (!vgfVerifyUserObject(Context, SourceImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		if (!vgfVerifyUserObject(Context, DestinationImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the objects. */
		source = (vgsIMAGE_PTR) SourceImage;
		target = (vgsIMAGE_PTR) DestinationImage;

		/* Cannot be the render targets. */
		if (vgfIsImageRenderTarget(source) ||
			vgfIsImageRenderTarget(target))
		{
			vgmERROR(VG_IMAGE_IN_USE_ERROR);
			break;
		}

		/* Determine the size of the work area. */
		width  = gcmMIN(source->size.width,  target->size.width);
		height = gcmMIN(source->size.height, target->size.height);

		/* Are the images related? */
		if (vgfImagesRelated(source, target))
		{
			gctBOOL horIntersection;
			gctBOOL verIntersection;

			/* Determine intersections. */
			horIntersection = gcmINTERSECT(
				source->origin.x,
				target->origin.x,
				width
				);

			verIntersection = gcmINTERSECT(
				source->origin.y,
				target->origin.y,
				height
				);

			/* Areas cannot intersect. */
			if (horIntersection && verIntersection)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		/* Matrix cannot be NULL. */
		if (vgmIS_INVALID_PTR(Matrix, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

#if vgvENABLE_HW_FILTER
		{
			gceSTATUS status;

			/* Try the hardware filter first. */
			status = gcoVG_ColorMatrix(
				Context->vg,
				source->surface,
				target->surface,
				Matrix,
				Context->fltHalChannelMask,
				Context->fltFormatLinear,
				Context->fltFormatPremultiplied,
				&source->origin,
				&target->origin,
				width, height
				);

			/* Success? */
			if (gcmIS_SUCCESS(status))
			{
				/* Arm the flush states. */
				*source->imageDirtyPtr  = vgvIMAGE_NOT_FINISHED;
				*target->imageDirtyPtr  = vgvIMAGE_NOT_READY;
				Context->imageDirty = vgvIMAGE_NOT_READY;

				/* Done. */
				break;
			}
		}
#endif

#if vgvENABLE_FAST_FILTER
		if (!Context->conformance)
		{
			gceSTATUS status;
			/* Copy the image. */
			status = vgfDrawImage(
				Context,
				source, target,
				0, 0,
				0, 0,
				width, height,
				gcvVG_BLEND_SRC,
				gcvFALSE,
				gcvFALSE,
				gcvFALSE,
				gcvFALSE,
				gcvFALSE
				);

			/* Success? */
			if (gcmIS_SUCCESS(status))
			{
				/* Arm the flush states. */
				*source->imageDirtyPtr  = vgvIMAGE_NOT_FINISHED;
				*target->imageDirtyPtr  = vgvIMAGE_NOT_READY;
				Context->imageDirty = vgvIMAGE_NOT_READY;

				/* Done. */
				break;
			}
		}
#endif
		/* Make a shortcut to the color channels. */
		channelMask = Context->fltCapChannelMask;

		/* Determine the pixel access routines. */
		readPixelIndex
			= (Context->fltFormatLinear << 1)
			|  Context->fltFormatPremultiplied;

		writePixelIndex
			= (readPixelIndex << 4)
			|  channelMask;

		readPixel
			= source->surfaceFormat->readPixel[readPixelIndex];

		writePixel
			= target->surfaceFormat->writePixel[writePixelIndex];

		/* NULL-writer? */
		if (writePixel == gcvNULL)
		{
			break;
		}

		/* Flush the images. */
		gcmVERIFY_OK(vgfFlushImage(Context, source, gcvTRUE));
		gcmVERIFY_OK(vgfFlushImage(Context, target, gcvTRUE));

		/* Initialize walkers. */
		vgsPIXELWALKER_Initialize(&srcPixel, source, 0, 0);
		vgsPIXELWALKER_Initialize(&trgPixel, target, 0, 0);

		/* Walk the pixels. */
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				/* Read the pixel and advance to the next. */
				readPixel(&srcPixel, srcValue);

				/* Compute the target pixel. */
				vgmCOMPUTECOLORMATRIX(Matrix, trgValue, srcValue, 0);
				vgmCOMPUTECOLORMATRIX(Matrix, trgValue, srcValue, 1);
				vgmCOMPUTECOLORMATRIX(Matrix, trgValue, srcValue, 2);
				vgmCOMPUTECOLORMATRIX(Matrix, trgValue, srcValue, 3);

				/* Write the result pixel and advance to the next. */
				writePixel(&trgPixel, trgValue, channelMask);
			}

			/* Advance to the next line. */
			vgsPIXELWALKER_NextLine(&srcPixel);
			vgsPIXELWALKER_NextLine(&trgPixel);
		}
	}
	vgmLEAVEAPI(vgColorMatrix);
}


/*******************************************************************************
**
** vgConvolve
**
** The vgConvolve function applies a user-supplied convolution kernel to a
** normalized source image SourceImage. The dimensions of the kernel are
** given by KernelWidth and KernelHeight; the kernel values are specified as
** KernelWidth * KernelHeight VGshorts in column-major order. That is, the
** kernel entry (i, j) is located at position i * KernelHeight + j in the input
** sequence. The ShiftX and ShiftY parameters specify a translation between the
** source and destination images. The result of the convolution is multiplied
** by a scale factor, and a bias is added.
**
** The output pixel (x, y) is defined as:
**
**    s * SUMij[ k(w-i-1, h-j-1) * p(x+i-ShiftX, y+j-ShiftY) ] + b,
**
** where
**    i = [0..w),
**    j = [0..h),
**    w = KernelWidth,
**    h = KernelHeight,
**    k[i,j] is the kernel element at position (i,j),
**    s = Scale,
**    b = Bias,
**    p(x, y) is the source pixel at (x, y), or the result of source edge
**            extension defined by TilingMode, which takes a value from the
**            VGTilingMode enumeration.
**
** Note that the use of the kernel index (w-i-1, h-j-1) implies that the kernel
** is rotated 180 degrees relative to the source image in order to conform to
** the mathematical definition of convolution when ShiftX = w - 1 and
** ShiftY = h - 1.
**
** The operation is applied to all channels (color and alpha) independently.
**
** INPUT:
**
**    DestinationImage
**       Handle to a valid VGImage object.
**
**    SourceImage
**       Handle to a valid VGImage object.
**
**    KernelWidth, KernelHeight
**       The dimensions of the kernel.
**
**    ShiftX, ShiftY
**       Translation between the source and destination images.
**
**    Kernel
**       Points to the kernel array.
**
**    Scale
**       Scale factor.
**
**    Bias
**       Bias value.
**
**    TilingMode
**       One of:
**          VG_TILE_FILL
**          VG_TILE_PAD
**          VG_TILE_REPEAT
**          VG_TILE_REFLECT
**
** OUTPUT:
**
**    DestinationImage
**       Computed image.
*/

VG_API_CALL void VG_API_ENTRY vgConvolve(
	VGImage DestinationImage,
	VGImage SourceImage,
	VGint KernelWidth,
	VGint KernelHeight,
	VGint ShiftX,
	VGint ShiftY,
	const VGshort * Kernel,
	VGfloat Scale,
	VGfloat Bias,
	VGTilingMode TilingMode
	)
{
	vgmENTERAPI(vgConvolve)
	{
		gceSTATUS status;

		vgsIMAGE_PTR source;
		vgsIMAGE_PTR target;
		gctINT x, y;
		gctINT width, height;
		gctINT xk, yk;

		gceTILE_MODE tilingMode;
		gctUINT channelMask;
		gctUINT writePixelIndex;
		vgtWRITE_PIXEL writePixel;
		vgsPIXELWALKER trgPixel;
		vgtFLOATVECTOR4 trgValue;
		vgtFLOATVECTOR4 fillColor;
		vgtFLOATVECTOR4 srcPixel;
		vgtFLOATVECTOR4 * tempBuffer;
		gctINT tmpWidth, tmpHeight;
		gctINT kernelIndex;

		vgmDUMP_WORD_ARRAY(
			Kernel, KernelWidth * KernelHeight
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X, %d, %d, %d, %d, Kernel, %.10ff, %.10ff, 0x%04X);\n",
			__FUNCTION__,
			DestinationImage, SourceImage,
			KernelWidth, KernelHeight,
			ShiftX, ShiftY,
			Scale, Bias,
			TilingMode
			);

		if (!vgfVerifyUserObject(Context, SourceImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		if (!vgfVerifyUserObject(Context, DestinationImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the objects. */
		source = (vgsIMAGE_PTR) SourceImage;
		target = (vgsIMAGE_PTR) DestinationImage;

		/* Cannot be the render targets. */
		if (vgfIsImageRenderTarget(source) ||
			vgfIsImageRenderTarget(target))
		{
			vgmERROR(VG_IMAGE_IN_USE_ERROR);
			break;
		}

		/* Determine the size of the work area. */
		width  = gcmMIN(source->size.width,  target->size.width);
		height = gcmMIN(source->size.height, target->size.height);

		/* Are the images related? */
		if (vgfImagesRelated(source, target))
		{
			gctBOOL horIntersection;
			gctBOOL verIntersection;

			/* Determine intersections. */
			horIntersection = gcmINTERSECT(
				source->origin.x,
				target->origin.x,
				width
				);

			verIntersection = gcmINTERSECT(
				source->origin.y,
				target->origin.y,
				height
				);

			/* Areas cannot intersect. */
			if (horIntersection && verIntersection)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		/* Validate the kernel size . */
		if ((KernelWidth  <= 0) ||
			(KernelHeight <= 0) ||
			(KernelWidth  >  Context->maxKernelSize) ||
			(KernelHeight >  Context->maxKernelSize))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Kernel cannot be NULL. */
		if (vgmIS_INVALID_PTR(Kernel, 2))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the tiling mode. */
		tilingMode = _GetTileMode(TilingMode);
		if (tilingMode == -1)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Set the size of the temporary buffer. */
		tmpWidth  = source->size.width;
		tmpHeight = source->size.height;

		/* Normalize the source. */
		status = _NormalizeSource(Context, source, tmpWidth, tmpHeight);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Flush the image. */
		status = vgfFlushImage(Context, target, gcvTRUE);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Convert the fill color. */
		vgfConvertColor(
			Context->tileFillColor, fillColor,
			Context->fltFormatPremultiplied,
			Context->fltFormatLinear,
			gcvFALSE
			);

		/* Cast the source buffer pointer. */
		tempBuffer = (vgtFLOATVECTOR4 *) Context->tempBuffer;

		/* Make a shortcut to the color channels. */
		channelMask = Context->fltCapChannelMask;

		/* Determine the write function. */
		writePixelIndex
			= (Context->fltFormatLinear        << 5)
			| (Context->fltFormatPremultiplied << 4)
			|  channelMask;

		writePixel
			= target->surfaceFormat->writePixel[writePixelIndex];

		/* NULL-writer? */
		if (writePixel == gcvNULL)
		{
			break;
		}

		/* Initialize walkers. */
		vgsPIXELWALKER_Initialize(&trgPixel, target, 0, 0);

		/* Walk the pixels. */
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				trgValue[0] = 0.0f;
				trgValue[1] = 0.0f;
				trgValue[2] = 0.0f;
				trgValue[3] = 0.0f;

				for (yk = 0; yk < KernelHeight; yk++)
				{
					for (xk = 0; xk < KernelWidth; xk++)
					{
						_ReadTempPixel(
							x + xk - ShiftX,
							y + yk - ShiftY,
							tmpWidth, tmpHeight,
							tilingMode,
							tempBuffer,
							fillColor,
							srcPixel
							);

						kernelIndex
							= (KernelWidth  - xk - 1) * KernelHeight
							+ (KernelHeight - yk - 1);

						trgValue[0] += (gctFLOAT) Kernel[kernelIndex] * srcPixel[0];
						trgValue[1] += (gctFLOAT) Kernel[kernelIndex] * srcPixel[1];
						trgValue[2] += (gctFLOAT) Kernel[kernelIndex] * srcPixel[2];
						trgValue[3] += (gctFLOAT) Kernel[kernelIndex] * srcPixel[3];
					}
				}

				trgValue[0] *= Scale;
				trgValue[1] *= Scale;
				trgValue[2] *= Scale;
				trgValue[3] *= Scale;

				trgValue[0] += Bias;
				trgValue[1] += Bias;
				trgValue[2] += Bias;
				trgValue[3] += Bias;

				/* Write the result pixel and advance to the next. */
				writePixel(&trgPixel, trgValue, channelMask);
			}

			/* Advance to the next line. */
			vgsPIXELWALKER_NextLine(&trgPixel);
		}
	}
	vgmLEAVEAPI(vgConvolve);
}


/*******************************************************************************
**
** vgSeparableConvolve
**
** The vgSeparableConvolve function applies a user-supplied separable convolu-
** tion kernel to a normalized source image SourceImage. A separable kernel is
** a two-dimensional kernel in which each entry kij is equal to a product
** kxi * kyj of elements from two one-dimensional kernels, one horizontal and
** one vertical.
**
** The lengths of the one-dimensional arrays KernelX and KernelY are given by
** KernelWidth and KernelHeight, respectively; the kernel values are specified
** as arrays of VGshorts. The ShiftX and ShiftY parameters specify a translation
** between the source and destination images. The result of the convolution is
** multiplied by a scale factor, and a bias is added.
**
** The output pixel (x, y) is defined as:
**
**    s * SUMij[ kx(w-i-1) * ky(h-j-1) * p(x+i-ShiftX, y+j-ShiftY) ] + b,
**
** where
**    i = [0..w),
**    j = [0..h),
**    w = KernelWidth,
**    h = KernelHeight,
**    kxi is the one-dimensional horizontal kernel element at position i,
**    kyj is the one-dimensional vertical kernel element at position j,
**    s = Scale,
**    b = Bias,
**    p(x, y) is the source pixel at (x, y), or the result of source edge
**            extension defined by TilingMode, which takes a value from the
**            VGTilingMode enumeration.
**
** Note that the use of the kernel indices (w-i-1) and (h-j-1) implies that
** the kernel is rotated 180 degrees relative to the source image in order
** to conform to the mathematical definition of convolution.
**
** The operation is applied to all channels (color and alpha) independently.
**
** INPUT:
**
**    DestinationImage
**       Handle to a valid VGImage object.
**
**    SourceImage
**       Handle to a valid VGImage object.
**
**    KernelWidth, KernelHeight
**       The dimensions of the kernel.
**
**    ShiftX, ShiftY
**       Translation between the source and destination images.
**
**    KernelX, KernelY
**       Pointers to the separable kernel array (horizontal and vertical).
**
**    Scale
**       Scale factor.
**
**    Bias
**       Bias value.
**
**    TilingMode
**       One of:
**          VG_TILE_FILL
**          VG_TILE_PAD
**          VG_TILE_REPEAT
**          VG_TILE_REFLECT
**
** OUTPUT:
**
**    DestinationImage
**       Computed image.
*/

VG_API_CALL void VG_API_ENTRY vgSeparableConvolve(
	VGImage DestinationImage,
	VGImage SourceImage,
	VGint KernelWidth,
	VGint KernelHeight,
	VGint ShiftX,
	VGint ShiftY,
	const VGshort * KernelX,
	const VGshort * KernelY,
	VGfloat Scale,
	VGfloat Bias,
	VGTilingMode TilingMode
	)
{
	vgmENTERAPI(vgSeparableConvolve)
	{
		gceSTATUS status;

		vgsIMAGE_PTR source;
		vgsIMAGE_PTR target;
		gctINT x, y;
		gctINT width, height;
		gctINT xk, yk;

		gceTILE_MODE tilingMode;
		gctUINT channelMask;
		gctUINT writePixelIndex;
		vgtWRITE_PIXEL writePixel;
		vgsPIXELWALKER trgPixel;
		vgtFLOATVECTOR4 trgValue;
		vgtFLOATVECTOR4 fillColor;
		vgtFLOATVECTOR4 srcPixel;
		vgtFLOATVECTOR4 * tempBuffer;
		gctINT tmpWidth, tmpHeight;
		gctFLOAT kernelValue;

		vgmDUMP_WORD_ARRAY(
			KernelX, KernelWidth
			);

		vgmDUMP_WORD_ARRAY(
			KernelY, KernelHeight
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X, %d, %d, %d, %d, KernelX, KernelY, %.10ff, %.10ff, 0x%04X);\n",
			__FUNCTION__,
			DestinationImage, SourceImage,
			KernelWidth, KernelHeight,
			ShiftX, ShiftY,
			Scale, Bias,
			TilingMode
			);

		if (!vgfVerifyUserObject(Context, SourceImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		if (!vgfVerifyUserObject(Context, DestinationImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the objects. */
		source = (vgsIMAGE_PTR) SourceImage;
		target = (vgsIMAGE_PTR) DestinationImage;

		/* Cannot be the render targets. */
		if (vgfIsImageRenderTarget(source) ||
			vgfIsImageRenderTarget(target))
		{
			vgmERROR(VG_IMAGE_IN_USE_ERROR);
			break;
		}

		/* Determine the size of the work area. */
		width  = gcmMIN(source->size.width,  target->size.width);
		height = gcmMIN(source->size.height, target->size.height);

		/* Are the images related? */
		if (vgfImagesRelated(source, target))
		{
			gctBOOL horIntersection;
			gctBOOL verIntersection;

			/* Determine intersections. */
			horIntersection = gcmINTERSECT(
				source->origin.x,
				target->origin.x,
				width
				);

			verIntersection = gcmINTERSECT(
				source->origin.y,
				target->origin.y,
				height
				);

			/* Areas cannot intersect. */
			if (horIntersection && verIntersection)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		/* Validate the kernel size . */
		if ((KernelWidth  <= 0) ||
			(KernelHeight <= 0) ||
			(KernelWidth  >  Context->maxSeparableKernelSize) ||
			(KernelHeight >  Context->maxSeparableKernelSize))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Kernel cannot be NULL. */
		if (vgmIS_INVALID_PTR(KernelX, 2) ||
			vgmIS_INVALID_PTR(KernelY, 2))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the tiling mode. */
		tilingMode = _GetTileMode(TilingMode);
		if (tilingMode == -1)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Convert the fill color. */
		vgfConvertColor(
			Context->tileFillColor, fillColor,
			Context->fltFormatPremultiplied,
			Context->fltFormatLinear,
			gcvFALSE
			);

#if vgvENABLE_HW_FILTER
		/* Try the hardware filter first. */
		if (!Context->conformance)
		{
			status = gcoVG_SeparableConvolve(
				Context->vg,
				source->surface, target->surface,
				KernelWidth, KernelHeight,
				ShiftX, ShiftY,
				KernelX, KernelY,
				Scale, Bias,
				tilingMode, fillColor,
				Context->fltHalChannelMask,
				Context->fltFormatLinear,
				Context->fltFormatPremultiplied,
				&source->origin,
				&target->origin,
				&source->size,
				width, height
				);

			/* Success? */
			if (gcmIS_SUCCESS(status))
			{
				/* Arm the flush states. */
				*source->imageDirtyPtr  = vgvIMAGE_NOT_FINISHED;
				*target->imageDirtyPtr  = vgvIMAGE_NOT_READY;
				Context->imageDirty = vgvIMAGE_NOT_READY;

				/* Done. */
				break;
			}
		}
#endif

		/* Set the size of the temporary buffer. */
		tmpWidth  = source->size.width;
		tmpHeight = source->size.height;

		/* Normalize the source. */
		status = _NormalizeSource(Context, source, tmpWidth, tmpHeight);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Flush the image. */
		status = vgfFlushImage(Context, target, gcvTRUE);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Cast the source buffer pointer. */
		tempBuffer = (vgtFLOATVECTOR4 *) Context->tempBuffer;

		/* Make a shortcut to the color channels. */
		channelMask = Context->fltCapChannelMask;

		/* Determine the write function. */
		writePixelIndex
			= (Context->fltFormatLinear        << 5)
			| (Context->fltFormatPremultiplied << 4)
			|  channelMask;

		writePixel
			= target->surfaceFormat->writePixel[writePixelIndex];

		/* NULL-writer? */
		if (writePixel == gcvNULL)
		{
			break;
		}

		/* Initialize walkers. */
		vgsPIXELWALKER_Initialize(&trgPixel, target, 0, 0);

		/* Walk the pixels. */
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				trgValue[0] = 0.0f;
				trgValue[1] = 0.0f;
				trgValue[2] = 0.0f;
				trgValue[3] = 0.0f;

				for (yk = 0; yk < KernelHeight; yk++)
				{
					for (xk = 0; xk < KernelWidth; xk++)
					{
						_ReadTempPixel(
							x + xk - ShiftX,
							y + yk - ShiftY,
							tmpWidth, tmpHeight,
							tilingMode,
							tempBuffer,
							fillColor,
							srcPixel
							);

						kernelValue
							= (gctFLOAT) KernelX[KernelWidth  - xk - 1]
							* (gctFLOAT) KernelY[KernelHeight - yk - 1];

						trgValue[0] += kernelValue * srcPixel[0];
						trgValue[1] += kernelValue * srcPixel[1];
						trgValue[2] += kernelValue * srcPixel[2];
						trgValue[3] += kernelValue * srcPixel[3];
					}
				}

				trgValue[0] *= Scale;
				trgValue[1] *= Scale;
				trgValue[2] *= Scale;
				trgValue[3] *= Scale;

				trgValue[0] += Bias;
				trgValue[1] += Bias;
				trgValue[2] += Bias;
				trgValue[3] += Bias;

				/* Write the result pixel and advance to the next. */
				writePixel(&trgPixel, trgValue, channelMask);
			}

			/* Advance to the next line. */
			vgsPIXELWALKER_NextLine(&trgPixel);
		}
	}
	vgmLEAVEAPI(vgSeparableConvolve);
}


/*******************************************************************************
**
** vgGaussianBlur
**
** The vgGaussianBlur function computes the convolution of a normalized source
** image SourceImage with a separable kernel defined in each dimension by the
** Gaussian function G(x, s):
**
**    G(x, s) = power[e, -sq(x) / (2 * sq(s))] / sqrt[2 * PI * sq(s)]
**
** where s is the standard deviation.
**
** The two-dimensional kernel is defined by multiplying together two one-
** dimensional kernels, one for each axis:
**
**    k(x, y) = G(x, sx) * G(y, sy) =
**            = power[e, - ( sq(x) / (2 * sq(sx)) + sq(y) / (2 * sq(sy)) )]
**            / (2 * PI * sx * sy)
**
** where sx and sy are the (positive) standard deviations in the horizontal and
** vertical directions, given by the StdDeviationX and StdDeviationY parameters
** respectively. This kernel has special properties that allow for very effici-
** ent implementation; for example, the implementation may use multiple passes
** with simple kernels to obtain the same overall result with higher performance
** than direct convolution. If StdDeviationX and StdDeviationY are equal, the
** kernel is rotationally symmetric.
**
** Source pixels outside the source image bounds are defined by tilingMode,
** which takes a value from the VGTilingMode enumeration.
**
** The operation is applied to all channels (color and alpha) independently.
**
** INPUT:
**
**    DestinationImage
**       Handle to a valid VGImage object.
**
**    SourceImage
**       Handle to a valid VGImage object.
**
**    StdDeviationX, StdDeviationY
**       Standard deviations in the horizontal and vertical directions.
**
**    TilingMode
**       One of:
**          VG_TILE_FILL
**          VG_TILE_PAD
**          VG_TILE_REPEAT
**          VG_TILE_REFLECT
**
** OUTPUT:
**
**    DestinationImage
**       Computed image.
*/

VG_API_CALL void VG_API_ENTRY vgGaussianBlur(
	VGImage DestinationImage,
	VGImage SourceImage,
	VGfloat StdDeviationX,
	VGfloat StdDeviationY,
	VGTilingMode TilingMode
	)
{
	vgmENTERAPI(vgGaussianBlur)
	{
		gceSTATUS status;

		vgsIMAGE_PTR source;
		vgsIMAGE_PTR target;
		gctINT x, y;
		gctINT width, height;
		gctINT xk, yk;

		gceTILE_MODE tilingMode;
		gctUINT channelMask;
		gctUINT writePixelIndex;
		vgtWRITE_PIXEL writePixel;
		vgsPIXELWALKER trgPixel;
		vgtFLOATVECTOR4 trgValue;
		vgtFLOATVECTOR4 fillColor;
		vgtFLOATVECTOR4 srcPixel;
		vgtFLOATVECTOR4 * tempBuffer;
		gctINT tmpWidth, tmpHeight;

		gctINT halfKernelX;
		gctINT halfKernelY;
		gctINT kernelWidth;
		gctINT kernelHeight;

		gctFLOAT scaleX, scaleY;
		gctFLOAT expScaleX, expScaleY;
		gctFLOAT kernelX[vgvMAXBLURKERNEL];
		gctFLOAT kernelY[vgvMAXBLURKERNEL];
		gctFLOAT kernelValue;

		vgmADVANCE_GAUSSIAN_COUNT(Context);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X, %.10ff, %.10ff, 0x%04X);\n",
			__FUNCTION__,
			DestinationImage, SourceImage,
			StdDeviationX, StdDeviationY,
			TilingMode
			);

		if (!vgfVerifyUserObject(Context, SourceImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		if (!vgfVerifyUserObject(Context, DestinationImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the objects. */
		source = (vgsIMAGE_PTR) SourceImage;
		target = (vgsIMAGE_PTR) DestinationImage;

		/* Cannot be the render targets. */
		if (vgfIsImageRenderTarget(source) ||
			vgfIsImageRenderTarget(target))
		{
			vgmERROR(VG_IMAGE_IN_USE_ERROR);
			break;
		}

		/* Determine the size of the work area. */
		width  = gcmMIN(source->size.width,  target->size.width);
		height = gcmMIN(source->size.height, target->size.height);

		/* Are the images related? */
		if (vgfImagesRelated(source, target))
		{
			gctBOOL horIntersection;
			gctBOOL verIntersection;

			/* Determine intersections. */
			horIntersection = gcmINTERSECT(
				source->origin.x,
				target->origin.x,
				width
				);

			verIntersection = gcmINTERSECT(
				source->origin.y,
				target->origin.y,
				height
				);

			/* Areas cannot intersect. */
			if (horIntersection && verIntersection)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		/* Validate standard deviation values. */
		if ((StdDeviationX <= 0) ||
			(StdDeviationY <= 0) ||
			(StdDeviationX > Context->maxGaussianDeviation) ||
			(StdDeviationY > Context->maxGaussianDeviation))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the tiling mode. */
		tilingMode = _GetTileMode(TilingMode);
		if (tilingMode == -1)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Convert the fill color. */
		vgfConvertColor(
			Context->tileFillColor, fillColor,
			Context->fltFormatPremultiplied,
			Context->fltFormatLinear,
			gcvFALSE
			);

#if vgvENABLE_HW_FILTER
		/* Try the hardware filter first. */
		if (!Context->conformance)
		{
			status = gcoVG_GaussianBlur(
				Context->vg,
				source->surface, target->surface,
				StdDeviationX, StdDeviationY,
				tilingMode, fillColor,
				Context->fltHalChannelMask,
				Context->fltFormatLinear,
				Context->fltFormatPremultiplied,
				&source->origin,
				&target->origin,
				&source->size,
				width, height
				);

			/* Success? */
			if (gcmIS_SUCCESS(status))
			{
				/* Arm the flush states. */
				*source->imageDirtyPtr  = vgvIMAGE_NOT_FINISHED;
				*target->imageDirtyPtr  = vgvIMAGE_NOT_READY;
				Context->imageDirty = vgvIMAGE_NOT_READY;

				/* Done. */
				break;
			}
		}
#endif

#if vgvENABLE_FAST_FILTER
		if (!Context->conformance)
		{
			/* Copy the image. */
			status = vgfDrawImage(
				Context,
				source, target,
				0, 0,
				0, 0,
				width, height,
				gcvVG_BLEND_SRC,
				gcvFALSE,
				gcvFALSE,
				gcvFALSE,
				gcvFALSE,
				gcvFALSE
				);

			/* Success? */
			if (gcmIS_SUCCESS(status))
			{
				/* Arm the flush states. */
				*source->imageDirtyPtr  = vgvIMAGE_NOT_FINISHED;
				*target->imageDirtyPtr  = vgvIMAGE_NOT_READY;
				Context->imageDirty = vgvIMAGE_NOT_READY;

				/* Done. */
				break;
			}
		}
#endif
		/* Set the size of the temporary buffer. */
		tmpWidth  = source->size.width;
		tmpHeight = source->size.height;

		/* Normalize the source. */
		status = _NormalizeSource(Context, source, tmpWidth, tmpHeight);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Flush the image. */
		status = vgfFlushImage(Context, target, gcvTRUE);

		if (gcmIS_ERROR(status))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Cast the source buffer pointer. */
		tempBuffer = (vgtFLOATVECTOR4 *) Context->tempBuffer;

		/* Make a shortcut to the color channels. */
		channelMask = Context->fltCapChannelMask;

		/* Determine the write function. */
		writePixelIndex
			= (Context->fltFormatLinear        << 5)
			| (Context->fltFormatPremultiplied << 4)
			|  channelMask;

		writePixel
			= target->surfaceFormat->writePixel[writePixelIndex];

		/* NULL-writer? */
		if (writePixel == gcvNULL)
		{
			break;
		}

		/* Determine kernel parameters. */
		expScaleX = -1.0f / (2.0f * StdDeviationX * StdDeviationX);
		expScaleY = -1.0f / (2.0f * StdDeviationY * StdDeviationY);

		halfKernelX = (gctUINT) (StdDeviationX * 4.0f + 1.0f);
		halfKernelY = (gctUINT) (StdDeviationY * 4.0f + 1.0f);

		kernelWidth  = halfKernelX * 2 + 1;
		kernelHeight = halfKernelY * 2 + 1;

		/* Compute the kernel arrays. */
		scaleX = 0.0f;
		for (xk = 0; xk < kernelWidth; xk++)
		{
			x = xk - halfKernelX;
			kernelX[xk] = (gctFLOAT) exp(expScaleX * x * x);
			scaleX += kernelX[xk];
		}

		scaleY = 0.0f;
		for (yk = 0; yk < kernelHeight; yk++)
		{
			y = yk - halfKernelY;
			kernelY[yk] = (gctFLOAT) exp(expScaleY * y * y);
			scaleY += kernelY[yk];
		}

		scaleX = 1.0f / scaleX;
		scaleY = 1.0f / scaleY;

		for (xk = 0; xk < kernelWidth; xk++)
		{
			kernelX[xk] *= scaleX;
		}

		for (yk = 0; yk < kernelHeight; yk++)
		{
			kernelY[yk] *= scaleY;
		}

		/* Initialize walkers. */
		vgsPIXELWALKER_Initialize(&trgPixel, target, 0, 0);

		/* Walk the pixels. */
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				trgValue[0] = 0.0f;
				trgValue[1] = 0.0f;
				trgValue[2] = 0.0f;
				trgValue[3] = 0.0f;

				for (yk = 0; yk < kernelHeight; yk++)
				{
					for (xk = 0; xk < kernelWidth; xk++)
					{
						_ReadTempPixel(
							x + xk - halfKernelX,
							y + yk - halfKernelY,
							tmpWidth, tmpHeight,
							tilingMode,
							tempBuffer,
							fillColor,
							srcPixel
							);

						kernelValue = kernelX[xk] * kernelY[yk];

						trgValue[0] += kernelValue * srcPixel[0];
						trgValue[1] += kernelValue * srcPixel[1];
						trgValue[2] += kernelValue * srcPixel[2];
						trgValue[3] += kernelValue * srcPixel[3];
					}
				}

				/* Write the result pixel and advance to the next. */
				writePixel(&trgPixel, trgValue, channelMask);
			}

			/* Advance to the next line. */
			vgsPIXELWALKER_NextLine(&trgPixel);
		}
	}
	vgmLEAVEAPI(vgGaussianBlur);
}


/*******************************************************************************
**
** vgLookup
**
** The vgLookup function passes each image channel of the normalized source
** image SourceImage through a separate lookup table.
**
** Each channel of the normalized source pixel is used as an index into the
** lookup table for that channel by multiplying the normalized value by 255
** and rounding to obtain an 8-bit integral value. Each LUT parameter should
** contain 256 VGubyte entries. The outputs of the lookup tables are
** concatenated to form an RGBA_8888 pixel value, which is interpreted as
** lRGBA_8888, lRGBA_8888_PRE, sRGBA_8888, or sRGBA_8888_PRE, depending on
** the values of OutputLinear and OutputPremultiplied.
**
** The resulting pixels are converted into the destination format using the
** normal pixel format conversion rules.
**
** INPUT:
**
**    DestinationImage
**       Handle to a valid VGImage object.
**
**    SourceImage
**       Handle to a valid VGImage object.
**
**    RedLUT, GreenLUT, BlueLUT, AlphaLUT
**       Pointers to the channel lookup tables.
**
**    OutputLinear, OutputPremultiplied
**       Control the output format.
**
** OUTPUT:
**
**    DestinationImage
**       Computed image.
*/

VG_API_CALL void VG_API_ENTRY vgLookup(
	VGImage DestinationImage,
	VGImage SourceImage,
	const VGubyte * RedLUT,
	const VGubyte * GreenLUT,
	const VGubyte * BlueLUT,
	const VGubyte * AlphaLUT,
	VGboolean OutputLinear,
	VGboolean OutputPremultiplied
	)
{
	vgmENTERAPI(vgLookup)
	{
		vgsIMAGE_PTR source;
		vgsIMAGE_PTR target;
		gctINT32 x, y;
		gctINT32 width, height;

		gctUINT channelMask;
		gctUINT readPixelIndex;
		gctUINT writePixelIndex;
		vgtREAD_PIXEL readPixel;
		vgtWRITE_PIXEL writePixel;
		vgsPIXELWALKER srcPixel;
		vgsPIXELWALKER trgPixel;
		vgtFLOATVECTOR4 srcValue;
		vgtFLOATVECTOR4 trgValue;

		vgmDUMP_BYTE_ARRAY(
			RedLUT, 256
			);

		vgmDUMP_BYTE_ARRAY(
			GreenLUT, 256
			);

		vgmDUMP_BYTE_ARRAY(
			BlueLUT, 256
			);

		vgmDUMP_BYTE_ARRAY(
			AlphaLUT, 256
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X, RedLUT, GreenLUT, BlueLUT, AlphaLUT, %d, %d);\n",
			__FUNCTION__,
			DestinationImage, SourceImage,
			OutputLinear, OutputPremultiplied
			);

		if (!vgfVerifyUserObject(Context, SourceImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		if (!vgfVerifyUserObject(Context, DestinationImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the objects. */
		source = (vgsIMAGE_PTR) SourceImage;
		target = (vgsIMAGE_PTR) DestinationImage;

		/* Cannot be the render targets. */
		if (vgfIsImageRenderTarget(source) ||
			vgfIsImageRenderTarget(target))
		{
			vgmERROR(VG_IMAGE_IN_USE_ERROR);
			break;
		}

		/* Determine the size of the work area. */
		width  = gcmMIN(source->size.width,  target->size.width);
		height = gcmMIN(source->size.height, target->size.height);

		/* Are the images related? */
		if (vgfImagesRelated(source, target))
		{
			gctBOOL horIntersection;
			gctBOOL verIntersection;

			/* Determine intersections. */
			horIntersection = gcmINTERSECT(
				source->origin.x,
				target->origin.x,
				width
				);

			verIntersection = gcmINTERSECT(
				source->origin.y,
				target->origin.y,
				height
				);

			/* Areas cannot intersect. */
			if (horIntersection && verIntersection)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		/* Validate lookup table pointers. */
		if ((RedLUT  == gcvNULL) || (GreenLUT == gcvNULL) ||
			(BlueLUT == gcvNULL) || (AlphaLUT == gcvNULL))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Make a shortcut to the color channels. */
		channelMask = Context->fltCapChannelMask;

		/* Determine the pixel access routines. */
		readPixelIndex
			= (Context->fltFormatLinear << 1)
			|  Context->fltFormatPremultiplied;

		if (OutputLinear)
		{
			if (OutputPremultiplied)
			{
				writePixelIndex = (3 << 4) | channelMask;
			}
			else
			{
				writePixelIndex = (2 << 4) | channelMask;
			}
		}
		else
		{
			if (OutputPremultiplied)
			{
				writePixelIndex = (1 << 4) | channelMask;
			}
			else
			{
				writePixelIndex = (0 << 4) | channelMask;
			}
		}

		readPixel
			= source->surfaceFormat->readPixel[readPixelIndex];

		writePixel
			= target->surfaceFormat->writePixel[writePixelIndex];

		/* NULL-writer? */
		if (writePixel == gcvNULL)
		{
			break;
		}

		/* Flush the images. */
		gcmVERIFY_OK(vgfFlushImage(Context, source, gcvTRUE));
		gcmVERIFY_OK(vgfFlushImage(Context, target, gcvTRUE));

		/* Initialize walkers. */
		vgsPIXELWALKER_Initialize(&srcPixel, source, 0, 0);
		vgsPIXELWALKER_Initialize(&trgPixel, target, 0, 0);

		/* Walk the pixels. */
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				/* Read the pixel and advance to the next. */
				readPixel(&srcPixel, srcValue);

				/* Lookup channels. */
				trgValue[0] = _LookupComponent(RedLUT,   srcValue[0]);
				trgValue[1] = _LookupComponent(GreenLUT, srcValue[1]);
				trgValue[2] = _LookupComponent(BlueLUT,  srcValue[2]);
				trgValue[3] = _LookupComponent(AlphaLUT, srcValue[3]);

				/* Write the result pixel and advance to the next. */
				writePixel(&trgPixel, trgValue, channelMask);
			}

			/* Advance to the next line. */
			vgsPIXELWALKER_NextLine(&srcPixel);
			vgsPIXELWALKER_NextLine(&trgPixel);
		}
	}
	vgmLEAVEAPI(vgLookup);
}


/*******************************************************************************
**
** vgLookupSingle
**
** The vgLookupSingle function passes a single image channel of the normalized
** source image SourceImage, selected by the SourceChannel parameter, through a
** combined lookup table that produces whole pixel values. Each normalized
** source channel value is multiplied by 255 and rounded to obtain an 8 bit
** integral value.
**
** The specified SourceChannel of the normalized source pixel is used as an
** index into the lookup table. If the source image is in a single-channel
** grayscale (VG_lL_8, VG_sL_8, or VG_BW_1) or alpha-only (VG_A_1, VG_A_4, or
** VG_A_8) format, the SourceChannel parameter is ignored and the single channel
** is used. The LookupTable parameter should contain 256 4-byte aligned entries
** in an RGBA_8888 pixel value, which is interpreted as lRGBA_8888,
** lRGBA_8888_PRE, sRGBA_8888, or sRGBA_8888_PRE, depending on the values of
** OutputLinear and OutputPremultiplied.
**
** The resulting pixels are converted into the destination format using the
** normal pixel format conversion rules.
**
** INPUT:
**
**    DestinationImage
**       Handle to a valid VGImage object.
**
**    SourceImage
**       Handle to a valid VGImage object.
**
**    LookupTable
**       Pointer to the lookup table.
**
**    SourceChannel
**       Specifies the source color channel to be processed.
**
**    OutputLinear, OutputPremultiplied
**       Control the output format.
**
** OUTPUT:
**
**    DestinationImage
**       Computed image.
*/

VG_API_CALL void VG_API_ENTRY vgLookupSingle(
	VGImage DestinationImage,
	VGImage SourceImage,
	const VGuint * LookupTable,
	VGImageChannel SourceChannel,
	VGboolean OutputLinear,
	VGboolean OutputPremultiplied
	)
{
	vgmENTERAPI(vgLookupSingle)
	{
		vgsIMAGE_PTR source;
		vgsIMAGE_PTR target;
		gctINT32 x, y;
		gctINT32 width, height;
		VGImageChannel sourceChannel;
		gctUINT8 index;

		gctUINT channelMask;
		gctUINT readPixelIndex;
		gctUINT writePixelIndex;
		vgtREAD_PIXEL readPixel;
		vgtWRITE_PIXEL writePixel;
		vgsPIXELWALKER srcPixel;
		vgsPIXELWALKER trgPixel;
		vgtFLOATVECTOR4 srcValue;
		vgtFLOATVECTOR4 trgValue;

		vgmDUMP_DWORD_ARRAY(
			LookupTable, 256
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X, LookupTable, 0x%04X, %d, %d);\n",
			__FUNCTION__,
			DestinationImage, SourceImage,
			SourceChannel,
			OutputLinear, OutputPremultiplied
			);

		if (!vgfVerifyUserObject(Context, SourceImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		if (!vgfVerifyUserObject(Context, DestinationImage, vgvOBJECTTYPE_IMAGE))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the objects. */
		source = (vgsIMAGE_PTR) SourceImage;
		target = (vgsIMAGE_PTR) DestinationImage;

		/* Cannot be the render targets. */
		if (vgfIsImageRenderTarget(source) ||
			vgfIsImageRenderTarget(target))
		{
			vgmERROR(VG_IMAGE_IN_USE_ERROR);
			break;
		}

		/* Determine the size of the work area. */
		width  = gcmMIN(source->size.width,  target->size.width);
		height = gcmMIN(source->size.height, target->size.height);

		/* Are the images related? */
		if (vgfImagesRelated(source, target))
		{
			gctBOOL horIntersection;
			gctBOOL verIntersection;

			/* Determine intersections. */
			horIntersection = gcmINTERSECT(
				source->origin.x,
				target->origin.x,
				width
				);

			verIntersection = gcmINTERSECT(
				source->origin.y,
				target->origin.y,
				height
				);

			/* Areas cannot intersect. */
			if (horIntersection && verIntersection)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		/* Determine the source channel. */
		if (source->surfaceFormat->grayscale)
		{
			if (source->surfaceFormat->luminance)
			{
				sourceChannel = VG_RED;
			}
			else
			{
				sourceChannel = VG_ALPHA;
			}
		}
		else if ((SourceChannel == VG_RED)   ||
				 (SourceChannel == VG_GREEN) ||
				 (SourceChannel == VG_BLUE)  ||
				 (SourceChannel == VG_ALPHA))
		{
			sourceChannel = SourceChannel;
		}
		else
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate lookup table pointer. */
		if (vgmIS_INVALID_PTR(LookupTable, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Make a shortcut to the color channels. */
		channelMask = Context->fltCapChannelMask;

		/* Determine the pixel access routines. */
		readPixelIndex
			= (Context->fltFormatLinear << 1)
			|  Context->fltFormatPremultiplied;

		if (OutputLinear)
		{
			if (OutputPremultiplied)
			{
				writePixelIndex = (3 << 4) | channelMask;
			}
			else
			{
				writePixelIndex = (2 << 4) | channelMask;
			}
		}
		else
		{
			if (OutputPremultiplied)
			{
				writePixelIndex = (1 << 4) | channelMask;
			}
			else
			{
				writePixelIndex = (0 << 4) | channelMask;
			}
		}

		readPixel
			= source->surfaceFormat->readPixel[readPixelIndex];

		writePixel
			= target->surfaceFormat->writePixel[writePixelIndex];

		/* NULL-writer? */
		if (writePixel == gcvNULL)
		{
			break;
		}

		/* Flush the images. */
		gcmVERIFY_OK(vgfFlushImage(Context, source, gcvTRUE));
		gcmVERIFY_OK(vgfFlushImage(Context, target, gcvTRUE));

		/* Initialize walkers. */
		vgsPIXELWALKER_Initialize(&srcPixel, source, 0, 0);
		vgsPIXELWALKER_Initialize(&trgPixel, target, 0, 0);

		/* Walk the pixels. */
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				/* Read the pixel and advance to the next. */
				readPixel(&srcPixel, srcValue);

				/* Convert the appropriate channel. */
				switch (sourceChannel)
				{
				case VG_RED:
					index = _ComponentToIndex(srcValue[0]);
					break;

				case VG_GREEN:
					index = _ComponentToIndex(srcValue[1]);
					break;

				case VG_BLUE:
					index = _ComponentToIndex(srcValue[2]);
					break;

				case VG_ALPHA:
					index = _ComponentToIndex(srcValue[3]);
					break;

				default:
					gcmASSERT(gcvFALSE);
					index = 0;
				}

				/* Determine the result pixel. */
				_ConvertColor32(LookupTable[index], trgValue);

				/* Write the result pixel and advance to the next. */
				writePixel(&trgPixel, trgValue, channelMask);
			}

			/* Advance to the next line. */
			vgsPIXELWALKER_NextLine(&srcPixel);
			vgsPIXELWALKER_NextLine(&trgPixel);
		}
	}
	vgmLEAVEAPI(vgLookupSingle);
}
