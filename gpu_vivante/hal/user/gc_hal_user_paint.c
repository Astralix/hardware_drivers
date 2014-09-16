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






#include "gc_hal_user_precomp.h"
#if gcdENABLE_VG
#include "gc_hal_vg.h"
#include "gc_hal_user_vg.h"
#endif

#if gcdUSE_VG

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	The different paint modes for the gcoPAINT object.
**
**	This enumeration lists the available paint modes a gcoPAINT object can be
**	in.
*/
typedef enum _gcePAINT_MODE
{
	/** Solid color. */
	gcvPAINT_MODE_SOLID,

	/** Linear gradient. */
	gcvPAINT_MODE_LINEAR,

	/** Radial gradient. */
	gcvPAINT_MODE_RADIAL,

	/** Pattern. */
	gcvPAINT_MODE_PATTERN,
}
gcePAINT_MODE;

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	The gcoPAINT object structure.
**
**	This structure defines the fields inside the gcoPAINT object.
*/
struct _gcoPAINT
{
	/** The object. */
	gcsOBJECT		object;

	/** Pointer to the gcoVG object. */
	gcoVG			vg;

	/** Current paint mode. */
	gcePAINT_MODE	mode;

	/** Color for solid color. */
	gctUINT8		red;
	gctUINT8		green;
	gctUINT8		blue;
	gctUINT8		alpha;

	/** Color ramp colors. */
	gctSIZE_T		colorRampEntries;
	gctPOINTER		colorRampBits;

	/** Linear gradient X coordinate where the gradient is zero. */
	gctFLOAT		zeroX;
	/** Linear gradient Y coordinate where the gradient is zero. */
	gctFLOAT		zeroY;
	/** OneX - ZeroX. */
	gctFLOAT		dx;
	/** OneY - ZeroY. */
	gctFLOAT		dy;
	/** (OneX - ZeroX) ^ 2 + (OneY - ZeroY) ^ 2. */
	gctFLOAT		dxdySquare;

	/** Radial gradient X coordinate for the focal point where the gradient is
		zero.
	*/
	gctFLOAT		focalX;
	/** Radial gradient Y coordinate for the focal point where the gradient is
		one.
	*/
	gctFLOAT		focalY;
	/** Radius^2. */
	gctFLOAT		radiusSquare;
	/** FocalX - CenterX. */
	gctFLOAT		fx;
	/** FocalY - CenterY. */
	gctFLOAT		fy;
	/** Radius^2 - (FocalX - CenterX)^2 - (FocalY - CenterY)^2. */
	gctFLOAT		rfxfySquare;

	/** Surface that holds the pattern. */
	gcoSURF			pattern;
	/** Width of the pattern. */
	gctFLOAT		width;
	/** Height of the pattern. */
	gctFLOAT		height;

	/** Computed constant value in linear mode. */
	gctFLOAT		constantLin;
	/** Computed step value for X in linear mode. */
	gctFLOAT		stepXLin;
	/** Computed step value for Y in linear mode. */
	gctFLOAT		stepYLin;
	/** Computed constant value in radial or pattern mode. */
	gctFLOAT		constantRad;
	/** Computed step value for X in radial or pattern mode. */
	gctFLOAT		stepXRad;
	/** Computed step value for Y in radial or pattern mode. */
	gctFLOAT		stepYRad;
	/** Computed step value for XX in radial mode. */
	gctFLOAT		stepXXRad;
	/** Computed step value for YY in radial mode. */
	gctFLOAT		stepYYRad;
	/** Computed step value for XY in radial mode. */
	gctFLOAT		stepXYRad;
};

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 The gcoPAINT object constructor.
**
**	gcoPAINT_Construct constructs a new gcoPAINT object.
**
**	@param[in]	Vg		Pointer to a gcoVG object.
**	@param[out]	Paint	Pointer to a variable receiving the gcoPAINT object
**						pointer.
*/
gceSTATUS
gcoPAINT_Construct(
	IN gcoVG Vg,
	OUT gcoPAINT * Paint
	)
{
	gceSTATUS status;
	gcoPAINT paint;

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Vg, gcvOBJ_VG);
	gcmVERIFY_ARGUMENT(Paint != gcvNULL);

	do
	{
		/* Allocate the gcoPAINT structure. */
		gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
									gcmSIZEOF(struct _gcoPAINT),
									(gctPOINTER *) &paint));

		/* Initialize the object. */
		paint->object.type = gcvOBJ_PAINT;

		paint->vg = Vg;

		/* Default to solid color RGBA = 0, 0, 0, 1.0f. */
		paint->mode  = gcvPAINT_MODE_SOLID;
		paint->red   = 0x00;
		paint->green = 0x00;
		paint->blue  = 0x00;
		paint->alpha = 0xFF;

		/* Return gcoPAINT object pointer. */
		*Paint = paint;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return the error. */
	return status;
}

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 The gcoPAINT object destructor.
**
**	gcoPAINT_Destroy destroys a previously constructed gcoPAINT object.
**
**	@param[in]	Paint	Pointer to the gcoPAINT object that needs to be
**						destroyed.
*/
gceSTATUS
gcoPAINT_Destroy(
	IN gcoPAINT Paint
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Paint, gcvOBJ_PAINT);

	/* Destroy the color ramp. */
	if (Paint->colorRampBits != gcvNULL)
	{
		/* Destroy the color ramp. */
		gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Paint->colorRampBits));
	}

	/* Mark the object as unknown. */
	Paint->object.type = gcvOBJ_UNKNOWN;

	/* Free the gcoPAINT structure. */
	gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Paint));

	/* Success. */
	return gcvSTATUS_OK;
}

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 Set the gcoPAINT object to solid color.
**
**	gcoPAINT_SolidColor sets the gcoPAINT object to solid color mode and defines
**	the color.
**
**	@param[in]	Paint	Pointer to the gcoPAINT object.
**	@param[in]	Red		Value for the red color channel.
**	@param[in]	Green	Value for the green color channel.
**	@param[in]	Blue	Value for the blue color channel.
**	@param[in]	Alpha	Value for the alpha color channel.
*/
gceSTATUS
gcoPAINT_SolidColor(
	IN gcoPAINT Paint,
	IN gctFLOAT Red,
	IN gctFLOAT Green,
	IN gctFLOAT Blue,
	IN gctFLOAT Alpha
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Paint, gcvOBJ_PAINT);

	/* Set paint to solid color mode. */
	Paint->mode = gcvPAINT_MODE_SOLID;

	/* Convert color to U08 format. */
	Paint->red   = (gctUINT8) (gcmCLAMP(Red,   0.0f, 1.0f) * 255.0f);
	Paint->green = (gctUINT8) (gcmCLAMP(Green, 0.0f, 1.0f) * 255.0f);
	Paint->blue  = (gctUINT8) (gcmCLAMP(Blue,  0.0f, 1.0f) * 255.0f);
	Paint->alpha = (gctUINT8) (gcmCLAMP(Alpha, 0.0f, 1.0f) * 255.0f);

	/* Success. */
	return gcvSTATUS_OK;
}

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 Set the gcoPAINT object to linear gradient.
**
**	gcoPAINT_Linear sets the gcoPAINT object to linear gradient mode.
**
**	@param[in]	Paint	Pointer to the gcoPAINT object.
**	@param[in]	ZeroX	X coordinate where the gradient is 0.
**	@param[in]	ZeroY	Y coordinate where the gradient is 0.
**	@param[in]	OneX	X coordinate where the gradient is 1.0.
**	@param[in]	OneY	Y coordinate where the gradient is 1.0.
*/
gceSTATUS
gcoPAINT_Linear(
	IN gcoPAINT Paint,
	IN gctFLOAT ZeroX,
	IN gctFLOAT ZeroY,
	IN gctFLOAT OneX,
	IN gctFLOAT OneY
	)
{
	/* Identify matrix. */
	gctFLOAT identity[] =
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
	};

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Paint, gcvOBJ_PAINT);

	/* Set paint to linear gradient mode. */
	Paint->mode = gcvPAINT_MODE_LINEAR;

	/* Save gradient information. */
	Paint->zeroX = ZeroX;
	Paint->zeroY = ZeroY;

	/* Compute constants of the equation. */
	Paint->dx         = OneX - ZeroX;
	Paint->dy         = OneY - ZeroY;
	Paint->dxdySquare = Paint->dx * Paint->dx + Paint->dy * Paint->dy;

	/* Apply identity matrix as default transformation. */
	gcmVERIFY_OK(gcoPAINT_Transform(Paint, identity));

	/* Success. */
	return gcvSTATUS_OK;
}

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 Set the gcoPAINT object to radial gradient.
**
**	gcoPAINT_Radial sets the gcoPAINT object to radial gradient mode.
**
**	@param[in]	Paint	Pointer to the gcoPAINT object.
**	@param[in]	CenterX	X location of the gradient.
**	@param[in]	CenterY	Y location of the gradient.
**	@param[in]	FocalX	X location of the focal point where the gradient is 0.
**	@param[in]	FocalY	Y location of the focal point where the gradient is 0.
**	@param[in]	Radius	Radius of the circle where the gradient is 1.0.
*/
gceSTATUS
gcoPAINT_Radial(
	IN gcoPAINT Paint,
	IN gctFLOAT CenterX,
	IN gctFLOAT CenterY,
	IN gctFLOAT FocalX,
	IN gctFLOAT FocalY,
	IN gctFLOAT Radius
	)
{
	/* Identify matrix. */
	gctFLOAT identity[] =
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
	};

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Paint, gcvOBJ_PAINT);

	/* Set paint to radial gradient mode. */
	Paint->mode = gcvPAINT_MODE_RADIAL;

	/* Save gradient information. */
	Paint->focalX = FocalX;
	Paint->focalY = FocalY;

	/* Compute constants of the equation. */
	Paint->fx           = FocalX - CenterX;
	Paint->fy           = FocalY - CenterY;
	Paint->radiusSquare = Radius * Radius;
	Paint->rfxfySquare  = Paint->radiusSquare
						- Paint->fx * Paint->fx
						- Paint->fy * Paint->fy;

	/* Apply identity matrix as default transformation. */
	gcmVERIFY_OK(gcoPAINT_Transform(Paint, identity));

	/* Success. */
	return gcvSTATUS_OK;
}

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 Set the gcoPAINT object to pattern mode.
**
**	gcoPAINT_Pattern sets the gcoPAINT object to pattern mode and defines the
**	pattern to be used.
**
**	@param[in]	Paint	Pointer to the gcoPAINT object.
**	@param[in]	Surface	Pointer to the gcoSURF object which defines the pattern.
*/
gceSTATUS
gcoPAINT_Pattern(
	IN gcoPAINT Paint,
	IN gcoSURF Pattern
	)
{
	gctUINT width, height;
	gceSTATUS status;

	/* Identify matrix. */
	gctFLOAT identity[] =
	{
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
	};

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Paint, gcvOBJ_PAINT);
	gcmVERIFY_OBJECT(Pattern, gcvOBJ_SURF);

	do
	{
		/* Verify if pattern is supported. */
		gcmERR_BREAK(gcoVG_IsFormatSupported(gcvSURF_TEXTURE,
											 Pattern->info.format));

		/* Compute constants of the equation. */
		gcmERR_BREAK(gcoSURF_GetSize(Pattern, &width, &height, gcvNULL));
		Paint->width  = (gctFLOAT) width;
		Paint->height = (gctFLOAT) height;

		/* Set paint to pattern mode. */
		Paint->mode = gcvPAINT_MODE_PATTERN;

		/* Save pattern information. */
		Paint->pattern = Pattern;

		/* Apply identity matrix as default transformation. */
		gcmERR_BREAK(gcoPAINT_Transform(Paint, identity));

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return the error. */
	return status;
}

#define gcmLERP(v1, v2, w)	((v1) * (w) + (v2) * (1.0f - (w)))

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 Set the color ramp for the linear and radial gradient modes.
**
**	gcoPAINT_SetColorRamp defines the color ramp required by the gradient modes.
**	Every gradient will be interpolated between the color stops specified in the
**	color ramp.  The color ramp needs to start with a color stop at 0.0 and end
**	with a color stop at 1.0.
**
**	@param[in]	Paint			Pointer to the gcoPAINT object.
**	@param[in]	Entries			Number of entries in the color ramp.
**	@param[in]	ColorRamp		Pointer to an array of gcsCOLOR_RAMP structures
**								that define the color ramp.
**	@param[in]	PreMultiplied	When set it means the colors in the color ramp
**								need to be pre-multiplied before being
**								interpolated.
*/
gceSTATUS
gcoPAINT_SetColorRamp(
	IN gcoPAINT Paint,
	IN gctSIZE_T Entries,
	IN gcsCOLOR_RAMP_PTR ColorRamp,
	IN gctBOOL PreMultiplied
	)
{
	gctINT common, stop;
	gctSIZE_T i, width;
	gctUINT8_PTR bits;
	gceSTATUS status;

	/* Find the common denominator of the color ramp stops. */
	common = 1;
	for (i = 1; i < Entries - 1; ++i)
	{
		gctFLOAT mul  = common * ColorRamp[i].stop;
		gctFLOAT frac = mul - (gctINT) mul;
		if (frac != 0)
		{
			common = gcmMAX(common, (gctINT) (1.0f / frac + 0.5f));
		}
	}

	do
	{
		/* Compute the width of the required color array. */
		width = common + 1;

		/* See if the current color array is large enough. */
		if (width > Paint->colorRampEntries)
		{
			if (Paint->colorRampBits != gcvNULL)
			{
				/* Free any current color array. */
				gcmERR_BREAK(gcmOS_SAFE_FREE(gcvNULL, Paint->colorRampBits));
			}

			/* Allocate a new color array. */
			gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
									    width * gcmSIZEOF(gctUINT32),
									    &Paint->colorRampBits));

			/* Mark number of entries in the color array. */
			Paint->colorRampEntries = width;
		}

		/* Set pointer to color array. */
		bits = (gctUINT8_PTR) Paint->colorRampBits;

		/* Start filling the color array. */
		stop = 0;
		for (i = 0; i < width; ++i)
		{
			gctFLOAT gradient;
			gctFLOAT red[2], green[2], blue[2], alpha[2];
			gctFLOAT weight;

			/* Compute gradient for current color array entry. */
			gradient = (gctFLOAT) i / (gctFLOAT) (width - 1);

			/* Find the entry in the color ramp that matches or exceeds this
			** gradient. */
			while (gradient > ColorRamp[stop].stop)
			{
				++stop;
			}

			if (gradient == ColorRamp[stop].stop)
			{
				/* Perfect match weight 1.0. */
				weight   = 1.0f;

				/* Use color ramp color. */
				red[0]   = ColorRamp[stop].red;
				green[0] = ColorRamp[stop].green;
				blue[0]  = ColorRamp[stop].blue;
				alpha[0] = ColorRamp[stop].alpha;

				red[1] = green[1] = blue[1] = alpha[1] = 0;
			}
			else
			{
				/* Compute weight. */
				weight = (ColorRamp[stop].stop - gradient)
					   / (ColorRamp[stop].stop - ColorRamp[stop - 1].stop);

				/* Grab color ramp color of previous stop. */
				red[0]   = ColorRamp[stop - 1].red;
				green[0] = ColorRamp[stop - 1].green;
				blue[0]  = ColorRamp[stop - 1].blue;
				alpha[0] = ColorRamp[stop - 1].alpha;

				/* Grab color ramp color of current stop. */
				red[1]   = ColorRamp[stop].red;
				green[1] = ColorRamp[stop].green;
				blue[1]  = ColorRamp[stop].blue;
				alpha[1] = ColorRamp[stop].alpha;
			}

			if (PreMultiplied)
			{
				/* Pre-multiply the first color. */
				red[0]   *= alpha[0];
				green[0] *= alpha[0];
				blue[0]  *= alpha[0];

				/* Pre-multiply the second color. */
				red[1]   *= alpha[1];
				green[1] *= alpha[1];
				blue[1]  *= alpha[1];
			}

			/* Filter the colors per channel. */
			*bits++ = (gctUINT8) (gcmLERP(alpha[0], alpha[1], weight) * 255.0f);
			*bits++ = (gctUINT8) (gcmLERP(blue[0],  blue[1],  weight) * 255.0f);
			*bits++ = (gctUINT8) (gcmLERP(green[0], green[1], weight) * 255.0f);
			*bits++ = (gctUINT8) (gcmLERP(red[0],   red[1],   weight) * 255.0f);
		}

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return the error. */
	return status;
}

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 Apply a transformation matrix to a gcoPAINT object.
**
**	gcoPAINT_Transform applies a transformation matrix to a gcoPAINT object.
**
**	@param[in]	Paint		Pointer to the gcoPAINT object.
**	@param[in]	Matrix3x2	Pointer to a 3x2 matrix that need to be applied to
**							the current paint.  This matrix has no affect on
**							solid color paints.
*/
gceSTATUS
gcoPAINT_Transform(
	IN gcoPAINT Paint,
	IN gctFLOAT_PTR Matrix3x2
	)
{
	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Paint, gcvOBJ_PAINT);
	gcmVERIFY_ARGUMENT(Matrix3x2 != gcvNULL);

	/* Dispatch based on the paint mode. */
	switch (Paint->mode)
	{
	case gcvPAINT_MODE_SOLID:
		/* Nothing to do in solid mode. */
		break;

	case gcvPAINT_MODE_LINEAR:
		/* g = ( dx * ((x + 0.5) * m00 + (y + 0.5) * m01 + m02 - zeroX)
		**     + dy * ((x + 0.5) * m10 + (y + 0.5) * m11 + m12 - zeroY)
		**	   ) / (dx^2 + dy^2)
		**   = x * (dx * m00 + dy * m10) / (dx^2 + dy^2)
		**   + y * (dx * m01 + dy * m11) / (dx^2 + dy^2)
		**   + ( dx * (0.5 * (m00 + m01) + m02 - zeroX)
		**     + dy * (0.5 * (m10 + m11) + m12 - zeroY)
		**     ) / (dx^2 + dy^2)
		*/
		Paint->stepXLin    = ( Paint->dx * Matrix3x2[0]
							 + Paint->dy * Matrix3x2[4]
							 ) / Paint->dxdySquare;
		Paint->stepYLin    = ( Paint->dx * Matrix3x2[1]
							 + Paint->dy * Matrix3x2[5]
							 ) / Paint->dxdySquare;
		Paint->constantLin = ( Paint->dx * ( 0.5f * (Matrix3x2[0] + Matrix3x2[1])
										   + Matrix3x2[2] - Paint->zeroX
										   )
							 + Paint->dy * ( 0.5f * (Matrix3x2[4] + Matrix3x2[5])
										   + Matrix3x2[6] - Paint->zeroY
										   )
							 ) / Paint->dxdySquare;
		break;

	case gcvPAINT_MODE_RADIAL:
		/*	dx = (x + 0.5) m00 + (y + 0.5) m01 + m02 - focalX
		**	   = x m00 + y m01 + 0.5 m00 + 0.5 m01 + m02 - focalX
		**	dy = (x + 0.5) m10 + (y + 0.5) m11 + m12 - focalY
		**     = x m10 + y m11 + 0.5 m10 + 0.5 m11 + m12 - focalY
		**	dx^2 = (x m00 + y m01 + 0.5 m00 + 0.5 m01 + m02 - focalX) ^ 2
		**		 = x^2 m00^2 + y^2 m01^2 + 0.25 m00^2 + 0.25 m01^2 + m02^2
		**		 + focalX^2 + xy m00 m01
		**		 + x m00 (0.5 m00 + 0.5 m01 + m02 - focalX)
		*/
		break;

	case gcvPAINT_MODE_PATTERN:
		/* u = (x + 0.5) * m00 + (y * 0.5) * m01 + m02.
		**   = x * m00 + y * m01 + 0.5 * (m00 + m01) + m02
		*/
		Paint->stepXLin    = Matrix3x2[0];
		Paint->stepYLin    = Matrix3x2[1];
		Paint->constantLin = 0.5f * (Matrix3x2[0] + Matrix3x2[1])
						   + Matrix3x2[2];

		/* v = (x + 0.5) * m10 + (y * 0.5) * m11 + m12.
		**   = x * m10 + y * m11 + 0.5 * (m10 + m11) + m12
		*/
		Paint->stepXRad    = Matrix3x2[3];
		Paint->stepYRad    = Matrix3x2[4];
		Paint->constantRad = 0.5f * (Matrix3x2[3] + Matrix3x2[4])
						   + Matrix3x2[5];
		break;
	}

	/* Success. */
	return gcvSTATUS_OK;
}

/******************************************************************************/
/** @ingroup gcoPAINT
**
**	@brief	 Program the hardware with the specified paint object.
**
**	gcoPAINT_Apply programs the specified paint object and its attributes to
**	the hardware for use in the next rendering.
**
**	@param[in]	Paint		Pointer to the gcoPAINT object.  If the pointer is
**							gcvNULL, the default block paint will be loaded.
**	@param[in]	Hardware	Pointer to the gcoHARDWARE object used to program
**							the states.
*/
gceSTATUS
gcoPAINT_Apply(
	IN gcoPAINT Paint,
	IN gcoHARDWARE Hardware
	)
{
	gceSTATUS status;

	/* Verify the arguments. */
	if (Paint != gcvNULL)
	{
		gcmVERIFY_OBJECT(Paint, gcvOBJ_PAINT);
	}
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);

	do
	{
		if (Paint == gcvNULL)
		{
			gcmERR_BREAK(gcoHARDWARE_SetPaintSolid(Hardware,
												   0x00,
												   0x00,
												   0x00,
												   0xFF));
		}
		else
		{
			switch (Paint->mode)
			{
			case gcvPAINT_MODE_SOLID:
				gcmERR_BREAK(gcoHARDWARE_SetPaintSolid(Hardware,
													   Paint->red,
													   Paint->green,
													   Paint->blue,
													   Paint->alpha));
				break;

			case gcvPAINT_MODE_LINEAR:
				gcmERR_BREAK(gcoHARDWARE_SetPaintLinear(Hardware,
														Paint->constantLin,
														Paint->stepXLin,
														Paint->stepYLin,
														gcvNULL));
				break;

			case gcvPAINT_MODE_RADIAL:
				gcmERR_BREAK(gcoHARDWARE_SetPaintRadial(Hardware,
														Paint->constantLin,
														Paint->stepXLin,
														Paint->stepYLin,
														Paint->constantRad,
														Paint->stepXRad,
														Paint->stepYRad,
														Paint->stepXXRad,
														Paint->stepYYRad,
														Paint->stepXYRad,
														gcvNULL));
				break;

			case gcvPAINT_MODE_PATTERN:
				gcmERR_BREAK(gcoHARDWARE_SetPaintPattern(Hardware,
														 Paint->constantLin,
														 Paint->stepXLin,
														 Paint->stepYLin,
														 Paint->constantRad,
														 Paint->stepXRad,
														 Paint->stepYRad,
														 Paint->pattern));
				break;
			}
		}
	}
	while (gcvFALSE);

	/* Return the status. */
	return status;
}

#endif /* gcdUSE_VG */
