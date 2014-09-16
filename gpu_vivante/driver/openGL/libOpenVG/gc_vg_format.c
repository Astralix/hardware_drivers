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
#include "gc_vg_format_lookup.c"
#include "gc_vg_format_reader.c"
#include "gc_vg_format_writer.c"
#include "gc_vg_format_array.c"


/*******************************************************************************
**
** vgfClampColor
**
** Clamps the floating point color to its valid range.
**
** INPUT:
**
**    Source
**       Pointer to the source color to be clamped.
**
**    Target
**       Pointer to the result color.
**
**    Premultiplied
**       Premultiplied format flag.
**
** OUTPUT:
**
**    Nothing.
*/

void vgfClampColor(
	vgtFLOATVECTOR4 Source,
	vgtFLOATVECTOR4 Target,
	gctBOOL Premultiplied
	)
{
	gctFLOAT colorMax;

	/* Clamp the alpha channel. */
	Target[3] = gcmCLAMP(Source[3], 0.0f, 1.0f);

	/* Determine the maximum value for the color channels. */
	colorMax = Premultiplied
		? Target[3]
		: 1.0f;

	/* Clamp the color channels. */
	Target[0] = gcmCLAMP(Source[0], 0.0f, colorMax);
	Target[1] = gcmCLAMP(Source[1], 0.0f, colorMax);
	Target[2] = gcmCLAMP(Source[2], 0.0f, colorMax);
}


/*******************************************************************************
**
** vgfGetColorGamma
**
** Convert linear color component to its non-lienar form.
**
** INPUT:
**
**    Color
**       Linear color value.
**
** OUTPUT:
**
**    Non-linear color value.
*/

gctFLOAT vgfGetColorGamma(
	gctFLOAT Color
	)
{
	gctFLOAT result;

	result = (Color <= 0.00304f)
		? Color * 12.92f
		: 1.0556f * ((gctFLOAT) pow(Color, 1.0f / 2.4f)) - 0.0556f;

	return result;
}


/*******************************************************************************
**
** vgfGetColorInverseGamma
**
** Convert non-linear color component to its lienar form.
**
** INPUT:
**
**    Color
**       Non-linear color value.
**
** OUTPUT:
**
**    Linear color value.
*/

gctFLOAT vgfGetColorInverseGamma(
	gctFLOAT Color
	)
{
	if (Color <= 0.03928f)
	{
		Color /= 12.92f;
	}
	else
	{
		Color = (gctFLOAT) pow((Color + 0.0556f) / 1.0556f, 2.4f);
	}

	return Color;
}


/*******************************************************************************
**
** vgfGetGrayScale
**
** Convert linear RGB color to its gray scale equivalent.
**
** INPUT:
**
**    Color
**       Linear color value.
**
** OUTPUT:
**
**    Non-linear color value.
*/

gctFLOAT vgfGetGrayScale(
	gctFLOAT Red,
	gctFLOAT Green,
	gctFLOAT Blue
	)
{
	return 0.2126f * Red + 0.7152f * Green + 0.0722f * Blue;
}


/*******************************************************************************
**
** vgfConvertColor
**
** Convert sRGB non-premultiplied color.
**
** INPUT:
**
**    Source
**       Pointer to the source color in sRGB non-premultiplied format.
**
**    TargetPremultiplied
**       Premultiplied target flag.
**
**    TargetLinear
**       Linear target flag.
**
**    TargetGrayscale
**       Grayscale target flag.
**
** OUTPUT:
**
**    Target
**       Converted color value.
*/

void vgfConvertColor(
	vgtFLOATVECTOR4 Source,
	vgtFLOATVECTOR4 Target,
	gctBOOL TargetPremultiplied,
	gctBOOL TargetLinear,
	gctBOOL TargetGrayscale
	)
{
	/* Clamp the color. */
	vgfClampColor(Source, Target, gcvFALSE);

	/* Grayscale format? */
	if (TargetGrayscale)
	{
		gctFLOAT r, g, b;
		gctFLOAT grayscale;

		/* Convert to linear space. */
		r = vgfGetColorInverseGamma(Target[0]);
		g = vgfGetColorInverseGamma(Target[1]);
		b = vgfGetColorInverseGamma(Target[2]);

		/* Convert to grayscale. */
		grayscale = vgfGetGrayScale(r, g, b);

		/* Linear? */
		if (!TargetLinear)
		{
			grayscale = vgfGetColorGamma(grayscale);
		}

		/* Premultiply if necessary. */
		if (TargetPremultiplied)
		{
			grayscale *= Target[3];
		}

		/* Set result. */
		Target[0] = grayscale;
		Target[1] = grayscale;
		Target[2] = grayscale;
	}
	else
	{
		/* Linear? */
		if (TargetLinear)
		{
			Target[0] = vgfGetColorInverseGamma(Target[0]);
			Target[1] = vgfGetColorInverseGamma(Target[1]);
			Target[2] = vgfGetColorInverseGamma(Target[2]);
		}

		/* Premultiply if necessary. */
		if (TargetPremultiplied)
		{
			Target[0] *= Target[3];
			Target[1] *= Target[3];
			Target[2] *= Target[3];
		}
	}
}


/*******************************************************************************
**
** vgsPIXELWALKER_Initialize
**
** Initializes the pixel reader to the beginning of the specified image.
**
** INPUT:
**
**    Walker
**       Pointer to the walker to be initialized.
**
**    Image
**       Pointer to the image object to be walked.
**
**    X, Y
**       Origin within the image.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPIXELWALKER_Initialize(
	vgsPIXELWALKER_PTR Walker,
	vgsIMAGE_PTR Image,
	gctINT X,
	gctINT Y
	)
{
	gctINT stride;
	gctUINT bitOffset;
	gctINT y;

	/* Retrieve the stride. */
	gcmVERIFY_OK(gcoSURF_GetAlignedSize(
		Image->surface,
		gcvNULL,
		gcvNULL,
		&stride
		));

	/* Compute the bit offset within a line. */
	bitOffset = (Image->origin.x + X) * Image->surfaceFormat->bitsPerPixel;

	/* Native orientation of OpenVG is bottom-top, therefore, if the
	   image orientation is top-bottom, flip the coordinates around. */
	if (Image->orientation == gcvORIENTATION_TOP_BOTTOM)
	{
		gctUINT height;
		gcmVERIFY_OK(gcoSURF_GetSize(Image->surface, gcvNULL, &height, gcvNULL));

		/* Determine the vertical coordinate. */
		y = height - Image->origin.y - Y - 1;

		/* Store the stride. */
		Walker->stride = - stride;
	}
	else
	{
		/* Determine the vertical coordinate. */
		y = Image->origin.y + Y;

		/* Store the stride. */
		Walker->stride = stride;
	}

	/* Determine the address of the beginning of the fisrt scan line. */
	Walker->line
		= Image->buffer
		+ y * stride
		+ bitOffset / 8;

	/* Determine the initial bit offset. */
	Walker->initialBitOffset
		= bitOffset % 8;

	/* Initialize the current pixel pointer. */
	Walker->current   = Walker->line;
	Walker->bitOffset = Walker->initialBitOffset;
}


/*******************************************************************************
**
** vgsPIXELWALKER_NextLine
**
** Advance the walker to the beginning of the next line of the image.
**
** INPUT:
**
**    Walker
**       Pointer to the walker.
**
** OUTPUT:
**
**    Nothing.
*/

void vgsPIXELWALKER_NextLine(
	vgsPIXELWALKER_PTR Walker
	)
{
	/* Advance to the next line. */
	Walker->line += Walker->stride;

	/* Initialize the current pixel pointer. */
	Walker->current = Walker->line;

	/* Set initial bit offset. */
	Walker->bitOffset = Walker->initialBitOffset;
}


/*******************************************************************************
**
** vgfGetOpenVGFormat
**
** Converts the internal surface format to OpenVG counterpart.
**
** INPUT:
**
**    Format
**       OpenVG image format.
**
** OUTPUT:
**
**    Pointer to the format information structure.
*/

VGImageFormat vgfGetOpenVGFormat(
	gceSURF_FORMAT Format,
	gctBOOL Linear,
	gctBOOL Premultiplied
	)
{
	VGImageFormat format;

	switch (Format)
	{
	case gcvSURF_A8:
		gcmASSERT(Linear && !Premultiplied);
		format = VG_A_8;
		break;

	case gcvSURF_X4R4G4B4:
	case gcvSURF_A4R4G4B4:
		format = VG_sARGB_4444;
		break;

	case gcvSURF_X1R5G5B5:
	case gcvSURF_A1R5G5B5:
		gcmASSERT(!Linear && !Premultiplied);
		format = VG_sARGB_1555;
		break;

	case gcvSURF_R5G6B5:
		if (Linear)
		{
			format = VG_lRGB_565_VIV;
		}
		else
		{
			format = VG_sRGB_565;
		}
		break;

	case gcvSURF_R8G8B8X8:
		if (Linear)
		{
			format = VG_lRGBX_8888;
		}
		else
		{
			format = VG_sRGBX_8888;
		}
		break;

	case gcvSURF_X8R8G8B8:
		if (Linear)
		{
			format = VG_lXRGB_8888;
		}
		else
		{
			format = VG_sXRGB_8888;
		}
		break;

	case gcvSURF_R8G8B8A8:
		if (Premultiplied)
		{
			if (Linear)
			{
				format = VG_lRGBA_8888_PRE;
			}
			else
			{
				format = VG_sRGBA_8888_PRE;
			}
		}
		else
		{
			if (Linear)
			{
				format = VG_lRGBA_8888;
			}
			else
			{
				format = VG_sRGBA_8888;
			}
		}
		break;

	case gcvSURF_A8R8G8B8:
		if (Premultiplied)
		{
			if (Linear)
			{
				format = VG_lARGB_8888_PRE;
			}
			else
			{
				format = VG_sARGB_8888_PRE;
			}
		}
		else
		{
			if (Linear)
			{
				format = VG_lARGB_8888;
			}
			else
			{
				format = VG_sARGB_8888;
			}
		}
		break;

	default:
		gcmASSERT(gcvFALSE);
		format = ~0;
	}

	/* Return OpenVG format. */
	return format;
}


/*******************************************************************************
**
** vgfGetFormatInfo
**
** Returns a pointer to the format information structure.
**
** INPUT:
**
**    Format
**       OpenVG image format.
**
** OUTPUT:
**
**    Pointer to the format information structure.
*/

vgsFORMAT_PTR vgfGetFormatInfo(
	VGImageFormat Format
	)
{
	gctINT tableIndex;
	gctINT formatIndex;
	vgsFORMAT_PTR table;
	vgsFORMAT_PTR surfaceFormat;

	/* Reset the index in the format code. */
	formatIndex = Format & ~(3 << 6);

	/* Make sure the format is within range. */
	if ((formatIndex < 0) || (formatIndex >= vgvFORMAT_COUNT))
	{
		return gcvNULL;
	}

	/* Determine the table index. */
	tableIndex = (Format >> 6) & 3;

	/* Fetch the table. */
	table = _masterFormatTable[tableIndex];

	/* Fetch the format. */
	surfaceFormat = &table[formatIndex];

	/* Valid format? */
	if (surfaceFormat->internalFormat == gcvSURF_UNKNOWN)
	{
		return gcvNULL;
	}

	/* Return the format. */
	return surfaceFormat;
}
