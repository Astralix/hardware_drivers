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
#include <string.h>

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/* Get the color vector out of a color ramp entry. */
#define vgmCOLORFROMRAMP(ColorRamp) \
	(((gctFLOAT_PTR) ColorRamp) + 1)

/* FIXME: Force to recompute for now. */
#define vgvFORCE_PAINT_RECOMPUTE 1


#define vgmFIXED_LERP(v1, v2, w) \
	( vgmFIXED_MUL(v1, w) + vgmFIXED_MUL(v2 ,vgvFIXED_ONE - w))

/* convert the float to fixed(16.16)*/
gcmINLINE  gctFIXED vgfFloatToFixed(
	IN gctFLOAT r
	)
{
		gctINT	E			= 0;
		gctUINT M			= 0;
		gctUINT r_i			= *(gctUINT*)&r;
		gctUINT mantissa	=  r_i			& 0x7fffff;
		gctUINT exp			= (r_i >> 23)	& 0xff;

		gctFIXED result;

		if (exp == 0x0)
		{
			return vgvFIXED_ZERO;
		}
		else if (exp == 0xff)
		{
			return  ((r_i & 0x80000000) ? vgvFIXED_MIN : vgvFIXED_MAX);
		}
		else
		{
			E = exp - 127;
			M = (1 << 23) + mantissa;

			if ((E - 7) >= 0)
			{
				result = M << (E - 7);
			}
			else
			{
				result = M >> (7 - E);
			}

			return ((r_i & 0x80000000) ? (((gctINT64)(gctFIXED)0xffff0000 * result) >> 16) : result);
		}
}


/* convert the fixed(16.16) to float*/
gcmINLINE  gctFLOAT vgfFixedToFloat(
	IN gctFIXED r
	)
{
	return (gctFLOAT) r / vgvFIXED_ONE;
}

gcmINLINE  gctFIXED vgfFixedMul(
        IN gctFIXED r1,
        IN gctFIXED r2
        )
{
        gctINT64 result =  (gctINT64) r1 * r2;

		if (result >= (((gctINT64)vgvFIXED_MAX) << 16))
		{
			return vgvFIXED_MAX;
		}
		if (result <= (((gctINT64)vgvFIXED_MIN) << 16))
		{
			return vgvFIXED_MIN;
		}

		return (gctFIXED)(result >> 16);
}


/*******************************************************************************
**
** _FreeRamp
**
** Free the current color ramp surface.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Paint
**       Pointer to the paint object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _FreeRamp(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		/* Unlock the surface. */
		if (Paint->colorRampBits != gcvNULL)
		{
			gcmASSERT(Paint->colorRampSurface != gcvNULL);

			gcmERR_BREAK(gcoSURF_Unlock(
				Paint->colorRampSurface,
				Paint->colorRampBits
				));

			/* Reset the pointer to the surface bits. */
			Paint->colorRampBits = gcvNULL;
		}

		/* Destroy the surface. */
		if (Paint->colorRampSurface != gcvNULL)
		{
			gcmERR_BREAK(gcoSURF_Destroy(
				Paint->colorRampSurface
				));

			/* Reset the surface object. */
			Paint->colorRampSurface = gcvNULL;
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _AllocateRamp
**
** Allocate color ramp surface.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Paint
**       Pointer to the paint object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _AllocateRamp(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint,
	gctUINT Width
	)
{
	gceSTATUS status;

	do
	{
		/* Destroy the current ramp surface if any. */
		gcmERR_BREAK(_FreeRamp(
			Context, Paint
			));

		/* Create a new color ramp surface. */
		gcmERR_BREAK(gcoSURF_Construct(
			Context->hal,
			Width, 0, 1,
			gcvSURF_IMAGE,
			gcvSURF_R8G8B8A8,
			gcvPOOL_DEFAULT,
			&Paint->colorRampSurface
			));

		/* Lock the color ramp surface. */
		gcmERR_BREAK(gcoSURF_Lock(
			Paint->colorRampSurface,
			gcvNULL,
			(gctPOINTER *) &Paint->colorRampBits
			));

		/* Set premultiplied flag as necessary. */
		gcmERR_BREAK(gcoSURF_SetColorType(
			Paint->colorRampSurface,
			Paint->colorRampPremultiplied
				? gcvSURF_COLOR_ALPHA_PRE
				: gcvSURF_COLOR_UNKNOWN
			));
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _UpdateInternalColorRamp
**
** Recompute the internal color ramp.
**
** INPUT:
**
**    Paint
**       Pointer to the paint object.
**
** OUTPUT:
**
**    Nothing.
*/

static void _UpdateInternalColorRamp(
	vgsPAINT_PTR Paint
	)
{
	/* Color ramp dirty? */
	if (Paint->intColorRampRecompute)
	{
		static gcsCOLOR_RAMP defaultRamp[] =
		{
			{
				0.0f,
				0.0f, 0.0f, 0.0f, 1.0f
			},
			{
				1.0f,
				1.0f, 1.0f, 1.0f, 1.0f
			}
		};

		gctUINT trgCount;
		gctFLOAT prevStop;
		gcsCOLOR_RAMP_PTR srcRamp;
		gcsCOLOR_RAMP_PTR srcRampLast;
		gcsCOLOR_RAMP_PTR trgRamp;

		/* Determine the last source ramp. */
		srcRampLast
			= Paint->vgColorRamp
			+ Paint->vgColorRampLength;

		/* Set the initial previous stop. */
		prevStop = gcvMAX_NEG_FLOAT;

		/* Reset the count. */
		trgCount = 0;

		/* Walk through the source ramp. */
		for (
			srcRamp = Paint->vgColorRamp, trgRamp = Paint->intColorRamp;
			srcRamp < srcRampLast;
			srcRamp += 1
			)
		{
			/* Must be in increasing order. */
			if (srcRamp->stop < prevStop)
			{
				/* Ignore the entire sequence. */
				trgCount = 0;
				break;
			}

			/* Update the previous stop value. */
			prevStop = srcRamp->stop;

			/* Must be within [0..1] range. */
			if ((srcRamp->stop < 0.0f) || (srcRamp->stop > 1.0f))
			{
				/* Ignore. */
				continue;
			}

			/* Clamp color. */
			vgfClampColor(
				vgmCOLORFROMRAMP(srcRamp),
				vgmCOLORFROMRAMP(trgRamp),
				gcvFALSE
				);

			/* First stop greater then zero? */
			if ((trgCount == 0) && (srcRamp->stop > 0.0f))
			{
				/* Force the first stop to 0.0f. */
				trgRamp->stop = 0.0f;

				/* Replicate the entry. */
				trgRamp[1] = *trgRamp;
				trgRamp[1].stop = srcRamp->stop;

				/* Advance. */
				trgRamp  += 2;
				trgCount += 2;
			}
			else
			{
				/* Set the stop value. */
				trgRamp->stop = srcRamp->stop;

				/* Advance. */
				trgRamp  += 1;
				trgCount += 1;
			}
		}

		/* Empty sequence? */
		if (trgCount == 0)
		{
			gcmVERIFY_OK(gcoOS_MemCopy(
				Paint->intColorRamp,
				defaultRamp,
				gcmSIZEOF(defaultRamp)
				));

			Paint->intColorRampLength = gcmCOUNTOF(defaultRamp);
		}
		else
		{
			/* The last stop must be at 1.0. */
			if (trgRamp[-1].stop != 1.0f)
			{
				/* Replicate the last entry. */
				*trgRamp = trgRamp[-1];

				/* Force the last stop to 1.0f. */
				trgRamp->stop = 1.0f;

				/* Update the final entry count. */
				trgCount += 1;
			}

			/* Set new length. */
			Paint->intColorRampLength = trgCount;
		}

		/* Mark the internal color ramp as recomputed. */
		Paint->intColorRampRecompute = gcvFALSE;

		/* Recompute the color ramp surface. */
		Paint->colorRampSurfaceRecompute = gcvTRUE;
	}
}


/*******************************************************************************
**
** _UpdateColorRampSurface
**
** Recompute the color ramp surface values.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Paint
**       Pointer to the paint object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _UpdateColorRampSurface(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	)
{
	gceSTATUS status;

	do
	{
#if gcdENABLE_PERFORMANCE_PRIOR
		gctSIZE_T		colorRampLength;
		gctINT			stop;
		gctUINT8_PTR	bits;
		gctSIZE_T		i;

		/* Need to recompute the surface? */
		if (Paint->colorRampSurfaceRecompute)
		{
			gctFIXED				common, width, ii;
			gcsFIXED_COLOR_RAMP_PTR colorRamp;
			gcsCOLOR_RAMP_PTR		intColorRamp;
			gcsFIXED_COLOR_RAMP		tempColorRamp[vgvMAX_COLOR_RAMP_STOPS + 2];

			/* Get shortcuts to the color ramp. */
			colorRampLength		= Paint->intColorRampLength;
			intColorRamp		= Paint->intColorRamp;

			/* Find the common denominator of the color ramp stops. */
			common = vgmINT_TO_FIXED(1);

			/*convert to the fixed point*/
			for (i = 0; i < colorRampLength; ++i)
			{
				tempColorRamp[i].alpha	= vgmFLOAT_TO_FIXED_SPECIAL(intColorRamp[i].alpha);
				tempColorRamp[i].red	= vgmFLOAT_TO_FIXED_SPECIAL(intColorRamp[i].red);
				tempColorRamp[i].green	= vgmFLOAT_TO_FIXED_SPECIAL(intColorRamp[i].green);
				tempColorRamp[i].blue	= vgmFLOAT_TO_FIXED_SPECIAL(intColorRamp[i].blue);
				tempColorRamp[i].stop	= vgmFLOAT_TO_FIXED_SPECIAL(intColorRamp[i].stop);
			}

			colorRamp = tempColorRamp;

			for (i = 0; i < colorRampLength; ++i)
			{
				if (colorRamp[i].stop != vgvFIXED_ZERO)
				{
					gctFIXED mul  = vgmFIXED_MUL(common, colorRamp[i].stop);

					gctFIXED frac = vgmFIXED_FRAC(mul);
					if (frac != vgvFIXED_ZERO)
					{
						common = gcmMAX(common, vgmFIXED_INT(vgmFIXED_DIV(vgvFIXED_ONE, frac) + vgvFIXED_HALF));
					}
				}
			}

			/* Compute the width of the required color array. */
			width = common + vgvFIXED_ONE;

			/* Allocate the color ramp surface. */
			gcmERR_BREAK(_AllocateRamp(
				Context, Paint, vgmFIXED_TO_UINT(width)
				));

			/* Set pointer to color array. */
			bits = (gctUINT8_PTR) Paint->colorRampBits;

			/* Start filling the color array. */
			stop = 0;
			for (ii = vgvFIXED_ZERO; ii < width; ii += vgvFIXED_ONE)
			{
				gctFIXED gradient;
				gctFIXED color[4];
				gctFIXED color1[4];
				gctFIXED color2[4];
				gctFIXED weight;

				/* Compute gradient for current color array entry. */
				gradient = vgmFIXED_DIV(ii, width - vgvFIXED_ONE);

				/* Find the entry in the color ramp that matches or exceeds this
				** gradient. */
				while (gradient > colorRamp[stop].stop)
				{
					++stop;
				}

				if (gradient == colorRamp[stop].stop)
				{
					/* Perfect match weight 1.0. */
					weight = vgvFIXED_ONE;

					/* Use color ramp color. */
					color1[3] = colorRamp[stop].alpha;
					color1[2] = colorRamp[stop].blue;
					color1[1] = colorRamp[stop].green;
					color1[0] = colorRamp[stop].red;

					color2[3] =
					color2[2] =
					color2[1] =
					color2[0] = vgvFIXED_ZERO;
				}
				else
				{
					/* Compute weight. */
					weight = vgmFIXED_DIV(colorRamp[stop].stop - gradient, colorRamp[stop].stop - colorRamp[stop - 1].stop);

					/* Grab color ramp color of previous stop. */
					color1[3] = colorRamp[stop - 1].alpha;
					color1[2] = colorRamp[stop - 1].blue;
					color1[1] = colorRamp[stop - 1].green;
					color1[0] = colorRamp[stop - 1].red;

					/* Grab color ramp color of current stop. */
					color2[3] = colorRamp[stop].alpha;
					color2[2] = colorRamp[stop].blue;
					color2[1] = colorRamp[stop].green;
					color2[0] = colorRamp[stop].red;
				}

				if (Paint->colorRampPremultiplied)
				{
					/* Pre-multiply the first color. */

					color1[2] = vgmFIXED_MUL(color1[2], color1[3]);
					color1[1] = vgmFIXED_MUL(color1[1], color1[3]);
					color1[0] = vgmFIXED_MUL(color1[0], color1[3]);


					/* Pre-multiply the second color. */
					color2[2] = vgmFIXED_MUL(color2[2], color2[3]);
					color2[1] = vgmFIXED_MUL(color2[1], color2[3]);
					color2[0] = vgmFIXED_MUL(color2[0], color2[3]);

				}

				/* Filter the colors per channel. */
				color[3] = vgmFIXED_LERP(color1[3], color2[3], weight);
				color[2] = vgmFIXED_LERP(color1[2], color2[2], weight);
				color[1] = vgmFIXED_LERP(color1[1], color2[1], weight);
				color[0] = vgmFIXED_LERP(color1[0], color2[0], weight);

				/* Pack the final color. */
				*bits++ = vgmPACK_FIXED_COLOR_COMPONENT(color[3]);
				*bits++ = vgmPACK_FIXED_COLOR_COMPONENT(color[2]);
				*bits++ = vgmPACK_FIXED_COLOR_COMPONENT(color[1]);
				*bits++ = vgmPACK_FIXED_COLOR_COMPONENT(color[0]);

			}
			/* Mark as recomputed. */
			Paint->colorRampSurfaceRecompute = gcvFALSE;
		}
#else
		gctSIZE_T colorRampLength;
		gcsCOLOR_RAMP_PTR colorRamp;
		gctINT common, stop;
		gctSIZE_T i, width;
		gctUINT8_PTR bits;

		/* Need to recompute the surface? */
		if (Paint->colorRampSurfaceRecompute)
		{
			/* Get shortcuts to the color ramp. */
			colorRampLength = Paint->intColorRampLength;
			colorRamp       = Paint->intColorRamp;

			/* Find the common denominator of the color ramp stops. */
			common = 1;
			for (i = 0; i < colorRampLength; ++i)
			{
				if (colorRamp[i].stop != 0.0f)
				{
					gctFLOAT mul  = common * colorRamp[i].stop;
					gctFLOAT frac = mul - (gctFLOAT) floor(mul);
					if (frac != 0)
					{
						common = gcmMAX(common, (gctINT) (1.0f / frac + 0.5f));
					}
				}
			}

			/* Compute the width of the required color array. */
			width = common + 1;

			/* Allocate the color ramp surface. */
			gcmERR_BREAK(_AllocateRamp(
				Context, Paint, width
				));

			/* Set pointer to color array. */
			bits = (gctUINT8_PTR) Paint->colorRampBits;

			/* Start filling the color array. */
			stop = 0;
			for (i = 0; i < width; ++i)
			{
				gctFLOAT gradient;
				gctFLOAT color[4];
				gctFLOAT color1[4];
				gctFLOAT color2[4];
				gctFLOAT weight;

				/* Compute gradient for current color array entry. */
				gradient = (gctFLOAT) i / (gctFLOAT) (width - 1);

				/* Find the entry in the color ramp that matches or exceeds this
				** gradient. */
				while (gradient > colorRamp[stop].stop)
				{
					++stop;
				}

				if (gradient == colorRamp[stop].stop)
				{
					/* Perfect match weight 1.0. */
					weight = 1.0f;

					/* Use color ramp color. */
					color1[3] = colorRamp[stop].alpha;
					color1[2] = colorRamp[stop].blue;
					color1[1] = colorRamp[stop].green;
					color1[0] = colorRamp[stop].red;

					color2[3] =
					color2[2] =
					color2[1] =
					color2[0] = 0.0f;
				}
				else
				{
					/* Compute weight. */
					weight = (colorRamp[stop].stop - gradient)
						   / (colorRamp[stop].stop - colorRamp[stop - 1].stop);

					/* Grab color ramp color of previous stop. */
					color1[3] = colorRamp[stop - 1].alpha;
					color1[2] = colorRamp[stop - 1].blue;
					color1[1] = colorRamp[stop - 1].green;
					color1[0] = colorRamp[stop - 1].red;

					/* Grab color ramp color of current stop. */
					color2[3] = colorRamp[stop].alpha;
					color2[2] = colorRamp[stop].blue;
					color2[1] = colorRamp[stop].green;
					color2[0] = colorRamp[stop].red;
				}

				if (Paint->colorRampPremultiplied)
				{
					/* Pre-multiply the first color. */
					color1[2] *= color1[3];
					color1[1] *= color1[3];
					color1[0] *= color1[3];

					/* Pre-multiply the second color. */
					color2[2] *= color2[3];
					color2[1] *= color2[3];
					color2[0] *= color2[3];
				}

				/* Filter the colors per channel. */
				color[3] = gcmLERP(color1[3], color2[3], weight);
				color[2] = gcmLERP(color1[2], color2[2], weight);
				color[1] = gcmLERP(color1[1], color2[1], weight);
				color[0] = gcmLERP(color1[0], color2[0], weight);

				/* Pack the final color. */
				*bits++ = gcoVG_PackColorComponent(color[3]);
				*bits++ = gcoVG_PackColorComponent(color[2]);
				*bits++ = gcoVG_PackColorComponent(color[1]);
				*bits++ = gcoVG_PackColorComponent(color[0]);
			}

			/* Mark as recomputed. */
			Paint->colorRampSurfaceRecompute = gcvFALSE;
		}
#endif
		/* Program color ramp. */
		gcmERR_BREAK(gcoVG_SetColorRamp(
			Context->vg,
			Paint->colorRampSurface,
			Paint->halColorRampSpreadMode
			));
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _SetDefaultGradientColor
**
** Sets the default color for linear and radial gradient paints.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Paint
**       Pointer to the paint object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _SetDefaultGradientColor(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	)
{
	gceSTATUS status;
	gctUINT8 r, g, b, a;
	gcsCOLOR_RAMP_PTR rampEntry;

	/* Select the proper color ramp entry. */
	if (Paint->halColorRampSpreadMode == gcvTILE_REPEAT)
	{
		rampEntry = Paint->intColorRamp;
	}
	else
	{
		rampEntry = &Paint->intColorRamp[Paint->intColorRampLength - 1];
	}

	/* Convert the color. */
	r = gcoVG_PackColorComponent(rampEntry->red);
	g = gcoVG_PackColorComponent(rampEntry->green);
	b = gcoVG_PackColorComponent(rampEntry->blue);
	a = gcoVG_PackColorComponent(rampEntry->alpha);

	/* Use solid color paint for invalid linear gradients. */
	status = gcoVG_SetSolidPaint(
		Context->vg, r, g, b, a
		);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _UpdatePaint
**
** Program the hardware states of the current paint.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Paint
**       Pointer to the paint object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _UpdateColorPaint(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	)
{
	/* Assume success. */
	gceSTATUS status;

	/* Program solid color paint. */
	status = gcoVG_SetSolidPaint(
		Context->vg,
		Paint->byteColor[0],
		Paint->byteColor[1],
		Paint->byteColor[2],
		Paint->byteColor[3]
		);

	/* Return status. */
	return status;
}

static gceSTATUS _UpdateLinearGradientPaint(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	)
{
	/* Assume success. */
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		gctFLOAT zeroX, oneX;
		gctFLOAT zeroY, oneY;

		/* Update the color ramp. */
		_UpdateInternalColorRamp(Paint);

		/* Make shortcuts to the linear gradient coordinates. */
		zeroX = Paint->linearGradient[0];
		zeroY = Paint->linearGradient[1];
		oneX  = Paint->linearGradient[2];
		oneY  = Paint->linearGradient[3];

		/* Check if linear gradient is valid. */
		if ((zeroX != oneX) || (zeroY != oneY))
		{
			/* Update the color ramp surface. */
			gcmERR_BREAK(_UpdateColorRampSurface(Context, Paint));

			/* Need to recompute? */
			if (Paint->lgRecompute || vgvFORCE_PAINT_RECOMPUTE)
			{
#if gcdENABLE_PERFORMANCE_PRIOR
				gctFIXED dx, dy, dxdx_dydy;
				vgsMATRIX_PTR matrix;

				/* Compute constants of the equation. */
				dx = vgmFLOAT_TO_FIXED_SPECIAL(oneX - zeroX);
				dy = vgmFLOAT_TO_FIXED_SPECIAL(oneY - zeroY);

				dxdx_dydy = vgmFIXED_MUL(dx, dx) + vgmFIXED_MUL(dy, dy);

				/* Make a shortcut to the current matrix. */
				matrix = Paint->matrix;

				/*
				**		dx (T(x) - x0) + dy (T(y) - y0)
				**	g = -------------------------------
				**			      dx^2 + dy^2
				**
				**	where
				**
				**		dx := x1 - x0
				**		dy := y1 - y1
				**		T(x) := (x + 0.5) m00 + (y + 0.5) m01 + m02
				**			  = x m00 + y m01 + 0.5 (m00 + m01) + m02
				**		T(y) := (x + 0.5) m10 + (y + 0.5) m11 + m12
				**			  = x m10 + y m11 + 0.5 (m10 + m11) + m12.
				**
				**	We can factor the top line into:
				**
				**		= dx (x m00 + y m01 + 0.5 (m00 + m01) + m02 - x0)
				**		+ dy (x m10 + y m11 + 0.5 (m10 + m11) + m12 - y0)
				**
				**		= x (dx m00 + dy m10)
				**		+ y (dx m01 + dy m11)
				**		+ dx (0.5 (m00 + m01) + m02 - x0)
				**		+ dy (0.5 (m10 + m11) + m12 - y0).
				*/

				Paint->lgStepXLin
					= vgmFIXED_TO_FLOAT(
						vgmFIXED_DIV( (vgmFIXED_MUL(dx, vgmMAT(matrix, 0, 0)) + vgmFIXED_MUL(dy, vgmMAT(matrix, 1, 0))), dxdx_dydy )
						);

				Paint->lgStepYLin
					= vgmFIXED_TO_FLOAT (
						vgmFIXED_DIV(vgmFIXED_MUL(dx, vgmMAT(matrix, 0, 1)) + vgmFIXED_MUL(dy, vgmMAT(matrix, 1, 1)), dxdx_dydy)
						);

				Paint->lgConstantLin =
					vgmFIXED_TO_FLOAT(
					vgmFIXED_DIV(
						vgmFIXED_MUL(
							vgmFIXED_MUL(vgvFIXED_HALF, vgmMAT(matrix, 0, 0) + vgmMAT(matrix, 0, 1) )
							+ vgmMAT(matrix, 0, 2) - vgmFLOAT_TO_FIXED_SPECIAL(zeroX)
						, dx)

						+

						vgmFIXED_MUL(
							vgmFIXED_MUL(vgvFIXED_HALF, vgmMAT(matrix, 1, 0) + vgmMAT(matrix, 1, 1) )
							+ vgmMAT(matrix, 1, 2) - vgmFLOAT_TO_FIXED_SPECIAL(zeroY)
						, dy)
					, dxdx_dydy)
					);
#else
				gctFLOAT dx, dy, dxdx_dydy;
				vgsMATRIX_PTR matrix;

				/* Compute constants of the equation. */
				dx = oneX - zeroX;
				dy = oneY - zeroY;

				dxdx_dydy = dx * dx + dy * dy;

				/* Make a shortcut to the current matrix. */
				matrix = Paint->matrix;

				/*
				**		dx (T(x) - x0) + dy (T(y) - y0)
				**	g = -------------------------------
				**			      dx^2 + dy^2
				**
				**	where
				**
				**		dx := x1 - x0
				**		dy := y1 - y1
				**		T(x) := (x + 0.5) m00 + (y + 0.5) m01 + m02
				**			  = x m00 + y m01 + 0.5 (m00 + m01) + m02
				**		T(y) := (x + 0.5) m10 + (y + 0.5) m11 + m12
				**			  = x m10 + y m11 + 0.5 (m10 + m11) + m12.
				**
				**	We can factor the top line into:
				**
				**		= dx (x m00 + y m01 + 0.5 (m00 + m01) + m02 - x0)
				**		+ dy (x m10 + y m11 + 0.5 (m10 + m11) + m12 - y0)
				**
				**		= x (dx m00 + dy m10)
				**		+ y (dx m01 + dy m11)
				**		+ dx (0.5 (m00 + m01) + m02 - x0)
				**		+ dy (0.5 (m10 + m11) + m12 - y0).
				*/

				Paint->lgStepXLin
					= (dx * vgmMAT(matrix, 0, 0) + dy * vgmMAT(matrix, 1, 0))
					/ dxdx_dydy;

				Paint->lgStepYLin
					= (dx * vgmMAT(matrix, 0, 1) + dy * vgmMAT(matrix, 1, 1))
					/ dxdx_dydy;

				Paint->lgConstantLin =
					(
						(
							0.5f * ( vgmMAT(matrix, 0, 0) + vgmMAT(matrix, 0, 1) )
							+ vgmMAT(matrix, 0, 2) - zeroX
						) * dx

						+

						(
							0.5f * ( vgmMAT(matrix, 1, 0) + vgmMAT(matrix, 1, 1) )
							+ vgmMAT(matrix, 1, 2) - zeroY
						) * dy
					)
					/ dxdx_dydy;
#endif
				/* Mark as recomputed. */
				Paint->lgRecompute = gcvFALSE;
			}

			/* Program linear paint. */
			gcmERR_BREAK(gcoVG_SetLinearPaint(
				Context->vg,
				Paint->lgConstantLin,
				Paint->lgStepXLin,
				Paint->lgStepYLin
				));
		}
		else
		{
			/* Program to the default color. */
			gcmERR_BREAK(_SetDefaultGradientColor(Context, Paint));
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

static gceSTATUS _UpdateRadialGradientPaint(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	)
{
	/* Assume success. */
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		gctFLOAT radius;

		/* Update the color ramp. */
		_UpdateInternalColorRamp(Paint);

		/* Make a shortcut to the gradient radius. */
		radius = Paint->radialGradient[4];

		/* Check if radial gradient is valid. */
		if (radius > 0)
		{
			/* Update the color ramp surface. */
			gcmERR_BREAK(_UpdateColorRampSurface(Context, Paint));

			/* Need to recompute? */
			if (Paint->rgRecompute || vgvFORCE_PAINT_RECOMPUTE)
			{
#if gcdENABLE_PERFORMANCE_PRIOR
				gctFIXED centerX, centerY;
				gctFIXED focalX, focalY;
				gctFIXED fx, fy;
				gctFIXED fxfy_2;
				gctFIXED radius2, radius1;
				gctFIXED r2_fx2, r2_fy2;
				gctFIXED r2_fx2_2, r2_fy2_2;
				gctFIXED r2_fx2_fy2;
				gctFIXED r2_fx2_fy2sq;
				gctFIXED cx, cy;
				vgsMATRIX_PTR matrix;

				/* Make shortcuts to the gradient information. */
				centerX = vgmFLOAT_TO_FIXED_SPECIAL(Paint->radialGradient[0]);
				centerY = vgmFLOAT_TO_FIXED_SPECIAL(Paint->radialGradient[1]);
				focalX  = vgmFLOAT_TO_FIXED_SPECIAL(Paint->radialGradient[2]);
				focalY  = vgmFLOAT_TO_FIXED_SPECIAL(Paint->radialGradient[3]);

				radius1 = vgmFLOAT_TO_FIXED_SPECIAL(radius);

				/* Compute constants of the equation. */
				fx           = focalX - centerX;
				fy           = focalY - centerY;
				fxfy_2       = vgmFIXED_MUL(vgvFIXED_TWO,  vgmFIXED_MUL(fx, fy));
				radius2      = vgmFIXED_MUL(radius1, radius1);
				r2_fx2       = radius2 - vgmFIXED_MUL(fx, fx);
				r2_fy2       = radius2 - vgmFIXED_MUL(fy, fy);
				r2_fx2_2     = vgmFIXED_MUL(vgvFIXED_TWO, r2_fx2);
				r2_fy2_2     = vgmFIXED_MUL(vgvFIXED_TWO, r2_fy2);
				r2_fx2_fy2   = r2_fx2 - vgmFIXED_MUL(fy, fy);
				r2_fx2_fy2sq = vgmFIXED_MUL(r2_fx2_fy2, r2_fx2_fy2);

				/* Make a shortcut to the current matrix. */
				matrix = Paint->matrix;

				/*                        _____________________________________
				**		dx fx + dy fy + \/r^2 (dx^2 + dy^2) - (dx fy - dy fx)^2
				**	g = -------------------------------------------------------
				**						   r^2 - fx^2 - fy^2
				**
				**	Where
				**
				**		dx := F(x) - focalX
				**		dy := F(y) - focalY
				**		fx := focalX - centerX
				**		fy := focalX - centerY
				**
				**	and
				**
				**		F(x) := (x + 0.5) m00 + (y + 0.5) m01 + m02
				**      F(y) := (x + 0.5) m10 + (y + 0.5) m11 + m12
				**
				**	So, dx can be factored into
				**
				**		dx = (x + 0.5) m00 + (y + 0.5) m01 + m02 - focalX
				**		   = x m00 + y m01 + 0.5 m00 + 0.5 m01 + m02 - focalX
				**
				**		   = x m00 + y m01 + cx
				**
				**	where
				**
				**		cx := 0.5 m00 + 0.5 m01 + m02 - focalX
				**
				**	The same way we can factor dy into
				**
				**		dy = x m10 + y m11 + cy
				**
				**	where
				**
				**		cy := 0.5 m10 + 0.5 m11 + m12 - focalY.
				**
				**	Now we can rewrite g as
				**								 ______________________________________
				**		  dx fx + dy fy         / r^2 (dx^2 + dy^2) - (dx fy - dy fx)^2
				**	g = ----------------- + \  /  -------------------------------------
				**		r^2 - fx^2 - fy^2    \/           (r^2 - fx^2 - fy^2)^2
				**				 ____
				**	  = gLin + \/gRad
				**
				**	where
				**
				**				  dx fx + dy fy
				**		gLin := -----------------
				**				r^2 - fx^2 - fy^2
				**
				**				r^2 (dx^2 + dy^2) - (dx fy - dy fx)^2
				**		gRad := -------------------------------------
				**						(r^2 - fx^2 - fy^2)^2
				*/

				cx
					= vgmFIXED_MUL(vgvFIXED_HALF,  vgmMAT(matrix, 0, 0) + vgmMAT(matrix, 0, 1) )
					+ vgmMAT(matrix, 0, 2)
					- focalX;

				cy
					= vgmFIXED_MUL(vgvFIXED_HALF, vgmMAT(matrix, 1, 0) + vgmMAT(matrix, 1, 1) )
					+ vgmMAT(matrix, 1, 2)
					- focalY;

				/*
				**			  dx fx + dy fy
				**	gLin := -----------------
				**			r^2 - fx^2 - fy^2
				**
				**	We can factor the top half into
				**
				**		= (x m00 + y m01 + cx) fx + (x m10 + y m11 + cy) fy
				**
				**		= x (m00 fx + m10 fy)
				**		+ y (m01 fx + m11 fy)
				**		+ cx fx + cy fy.
				*/

				Paint->rgStepXLin
					= vgmFIXED_TO_FLOAT(
						vgmFIXED_DIV( vgmFIXED_MUL(vgmMAT(matrix, 0, 0), fx) + vgmFIXED_MUL(vgmMAT(matrix, 1, 0), fy)
					, r2_fx2_fy2));

				Paint->rgStepYLin
					= vgmFIXED_TO_FLOAT(
						vgmFIXED_DIV( vgmFIXED_MUL(vgmMAT(matrix, 0, 1), fx) + vgmFIXED_MUL(vgmMAT(matrix, 1, 1), fy)
					,r2_fx2_fy2));

				Paint->rgConstantLin = vgmFIXED_TO_FLOAT(vgmFIXED_DIV( vgmFIXED_MUL(cx, fx)  + vgmFIXED_MUL(cy, fy), r2_fx2_fy2));

				/*
				**			r^2 (dx^2 + dy^2) - (dx fy - dy fx)^2
				**	gRad := -------------------------------------
				**					(r^2 - fx^2 - fy^2)^2
				**
				**			r^2 (dx^2 + dy^2) - dx^2 fy^2 - dy^2 fx^2 + 2 dx dy fx fy
				**		 := ---------------------------------------------------------
				**							  (r^2 - fx^2 - fy^2)^2
				**
				**			dx^2 (r^2 - fy^2) + dy^2 (r^2 - fx^2) + 2 dx dy fx fy
				**		 := -----------------------------------------------------
				**							(r^2 - fx^2 - fy^2)^2
				**
				**	First, lets factor dx^2 into
				**
				**		dx^2 = (x m00 + y m01 + cx)^2
				**			 = x^2 m00^2 + y^2 m01^2 + 2 x y m00 m01
				**			 + 2 x m00 cx + 2 y m01 cx + cx^2
				**
				**			 = x^2 (m00^2)
				**			 + y^2 (m01^2)
				**			 + x y (2 m00 m01)
				**			 + x (2 m00 cx)
				**			 + y (2 m01 cx)
				**			 + cx^2.
				**
				**	The same can be done for dy^2:
				**
				**		dy^2 = x^2 (m10^2)
				**			 + y^2 (m11^2)
				**			 + x y (2 m10 m11)
				**			 + x (2 m10 cy)
				**			 + y (2 m11 cy)
				**			 + cy^2.
				**
				**	Let's also factor dx dy into
				**
				**		dx dy = (x m00 + y m01 + cx) (x m10 + y m11 + cy)
				**			  = x^2 m00 m10 + y^2 m01 m11 + x y m00 m11 + x y m01 m10
				**			  + x m00 cy + x m10 cx + y m01 cy + y m11 cx + cx cy
				**
				**			  = x^2 (m00 m10)
				**			  + y^2 (m01 m11)
				**			  + x y (m00 m11 + m01 m10)
				**			  + x (m00 cy + m10 cx)
				**			  + y (m01 cy + m11 cx)
				**			  + cx cy.
				**
				**	Now that we have all this, lets look at the top of gRad.
				**
				**		= dx^2 (r^2 - fy^2) + dy^2 (r^2 - fx^2) + 2 dx dy fx fy
				**		= x^2 m00^2 (r^2 - fy^2) + y^2 m01^2 (r^2 - fy^2)
				**		+ x y 2 m00 m01 (r^2 - fy^2) + x 2 m00 cx (r^2 - fy^2)
				**		+ y 2 m01 cx (r^2 - fy^2) + cx^2 (r^2 - fy^2)
				**		+ x^2 m10^2 (r^2 - fx^2) + y^2 m11^2 (r^2 - fx^2)
				**		+ x y 2 m10 m11 (r^2 - fx^2) + x 2 m10 cy (r^2 - fx^2)
				**		+ y 2 m11 cy (r^2 - fx^2) + cy^2 (r^2 - fx^2)
				**		+ x^2 m00 m10 2 fx fy + y^2 m01 m11 2 fx fy
				**		+ x y (m00 m11 + m01 m10) 2 fx fy
				**		+ x (m00 cy + m10 cx) 2 fx fy + y (m01 cy + m11 cx) 2 fx fy
				**		+ cx cy 2 fx fy
				**
				**		= x^2 ( m00^2 (r^2 - fy^2)
				**			  + m10^2 (r^2 - fx^2)
				**			  + m00 m10 2 fx fy
				**			  )
				**		+ y^2 ( m01^2 (r^2 - fy^2)
				**			  + m11^2 (r^2 - fx^2)
				**			  + m01 m11 2 fx fy
				**			  )
				**		+ x y ( 2 m00 m01 (r^2 - fy^2)
				**			  + 2 m10 m11 (r^2 - fx^2)
				**			  + (m00 m11 + m01 m10) 2 fx fy
				**			  )
				**		+ x ( 2 m00 cx (r^2 - fy^2)
				**			+ 2 m10 cy (r^2 - fx^2)
				**			+ (m00 cy + m10 cx) 2 fx fy
				**			)
				**		+ y ( 2 m01 cx (r^2 - fy^2)
				**			+ 2 m11 cy (r^2 - fx^2)
				**			+ (m01 cy + m11 cx) 2 fx fy
				**			)
				**		+ cx^2 (r^2 - fy^2) + cy^2 (r^2 - fx^2) + cx cy 2 fx fy.
				*/

				Paint->rgStepXXRad =
					vgmFIXED_TO_FLOAT(
					vgmFIXED_DIV(
						  vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(matrix, 0, 0)), r2_fy2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 1, 0), vgmMAT(matrix, 1, 0)), r2_fx2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(matrix, 1, 0)), fxfy_2)
					, r2_fx2_fy2sq));

				Paint->rgStepYYRad =
					vgmFIXED_TO_FLOAT(
					vgmFIXED_DIV(
						  vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(matrix, 0, 1)), r2_fy2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 1, 1), vgmMAT(matrix, 1, 1)), r2_fx2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(matrix, 1, 1)), fxfy_2)
					, r2_fx2_fy2sq ));

				Paint->rgStepXYRad =
					vgmFIXED_TO_FLOAT(
					vgmFIXED_DIV(
						  vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(matrix, 0, 1)), r2_fy2_2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 1, 0), vgmMAT(matrix, 1, 1)), r2_fx2_2)
						+ vgmFIXED_MUL(
							  vgmFIXED_MUL(vgmMAT(matrix, 0, 0), vgmMAT(matrix, 1, 1))
							+ vgmFIXED_MUL(vgmMAT(matrix, 0, 1), vgmMAT(matrix, 1, 0))

						  , fxfy_2)
					, r2_fx2_fy2sq));

				Paint->rgStepXRad =
					vgmFIXED_TO_FLOAT(
					vgmFIXED_DIV
					(
						  vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 0, 0), cx), r2_fy2_2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 1, 0), cy), r2_fx2_2)
						+ vgmFIXED_MUL(
							  vgmFIXED_MUL(vgmMAT(matrix, 0, 0), cy)
							+ vgmFIXED_MUL(vgmMAT(matrix, 1, 0), cx)
						  , fxfy_2)
					, r2_fx2_fy2sq));

				Paint->rgStepYRad =
					vgmFIXED_TO_FLOAT(
					vgmFIXED_DIV
					(
						  vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 0, 1), cx), r2_fy2_2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(vgmMAT(matrix, 1, 1), cy), r2_fx2_2)
						+ vgmFIXED_MUL(
							  vgmFIXED_MUL(vgmMAT(matrix, 0, 1), cy)
							+ vgmFIXED_MUL(vgmMAT(matrix, 1, 1), cx)
						  , fxfy_2)
					, r2_fx2_fy2sq));

				Paint->rgConstantRad =
					vgmFIXED_TO_FLOAT(
					vgmFIXED_DIV
					(
						  vgmFIXED_MUL(vgmFIXED_MUL(cx, cx), r2_fy2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(cy, cy), r2_fx2)
						+ vgmFIXED_MUL(vgmFIXED_MUL(cx, cy), fxfy_2)
					,r2_fx2_fy2sq));
#else
				gctFLOAT centerX, centerY;
				gctFLOAT focalX, focalY;
				gctFLOAT fx, fy;
				gctFLOAT fxfy_2;
				gctFLOAT radius2;
				gctFLOAT r2_fx2, r2_fy2;
				gctFLOAT r2_fx2_2, r2_fy2_2;
				gctFLOAT r2_fx2_fy2;
				gctFLOAT r2_fx2_fy2sq;
				gctFLOAT cx, cy;
				vgsMATRIX_PTR matrix;

				/* Make shortcuts to the gradient information. */
				centerX = Paint->radialGradient[0];
				centerY = Paint->radialGradient[1];
				focalX  = Paint->radialGradient[2];
				focalY  = Paint->radialGradient[3];

				/* Compute constants of the equation. */
				fx           = focalX - centerX;
				fy           = focalY - centerY;
				fxfy_2       = 2.0f * fx * fy;
				radius2      = radius * radius;
				r2_fx2       = radius2 - fx * fx;
				r2_fy2       = radius2 - fy * fy;
				r2_fx2_2     = 2.0f * r2_fx2;
				r2_fy2_2     = 2.0f * r2_fy2;
				r2_fx2_fy2   = r2_fx2  - fy * fy;
				r2_fx2_fy2sq = r2_fx2_fy2 * r2_fx2_fy2;

				/* Make a shortcut to the current matrix. */
				matrix = Paint->matrix;

				/*                        _____________________________________
				**		dx fx + dy fy + \/r^2 (dx^2 + dy^2) - (dx fy - dy fx)^2
				**	g = -------------------------------------------------------
				**						   r^2 - fx^2 - fy^2
				**
				**	Where
				**
				**		dx := F(x) - focalX
				**		dy := F(y) - focalY
				**		fx := focalX - centerX
				**		fy := focalX - centerY
				**
				**	and
				**
				**		F(x) := (x + 0.5) m00 + (y + 0.5) m01 + m02
				**      F(y) := (x + 0.5) m10 + (y + 0.5) m11 + m12
				**
				**	So, dx can be factored into
				**
				**		dx = (x + 0.5) m00 + (y + 0.5) m01 + m02 - focalX
				**		   = x m00 + y m01 + 0.5 m00 + 0.5 m01 + m02 - focalX
				**
				**		   = x m00 + y m01 + cx
				**
				**	where
				**
				**		cx := 0.5 m00 + 0.5 m01 + m02 - focalX
				**
				**	The same way we can factor dy into
				**
				**		dy = x m10 + y m11 + cy
				**
				**	where
				**
				**		cy := 0.5 m10 + 0.5 m11 + m12 - focalY.
				**
				**	Now we can rewrite g as
				**								 ______________________________________
				**		  dx fx + dy fy         / r^2 (dx^2 + dy^2) - (dx fy - dy fx)^2
				**	g = ----------------- + \  /  -------------------------------------
				**		r^2 - fx^2 - fy^2    \/           (r^2 - fx^2 - fy^2)^2
				**				 ____
				**	  = gLin + \/gRad
				**
				**	where
				**
				**				  dx fx + dy fy
				**		gLin := -----------------
				**				r^2 - fx^2 - fy^2
				**
				**				r^2 (dx^2 + dy^2) - (dx fy - dy fx)^2
				**		gRad := -------------------------------------
				**						(r^2 - fx^2 - fy^2)^2
				*/

				cx
					= 0.5f * ( vgmMAT(matrix, 0, 0) + vgmMAT(matrix, 0, 1) )
					+ vgmMAT(matrix, 0, 2)
					- focalX;

				cy
					= 0.5f * ( vgmMAT(matrix, 1, 0) + vgmMAT(matrix, 1, 1) )
					+ vgmMAT(matrix, 1, 2)
					- focalY;

				/*
				**			  dx fx + dy fy
				**	gLin := -----------------
				**			r^2 - fx^2 - fy^2
				**
				**	We can factor the top half into
				**
				**		= (x m00 + y m01 + cx) fx + (x m10 + y m11 + cy) fy
				**
				**		= x (m00 fx + m10 fy)
				**		+ y (m01 fx + m11 fy)
				**		+ cx fx + cy fy.
				*/

				Paint->rgStepXLin
					= ( vgmMAT(matrix, 0, 0) * fx + vgmMAT(matrix, 1, 0) * fy )
					/ r2_fx2_fy2;

				Paint->rgStepYLin
					= ( vgmMAT(matrix, 0, 1) * fx + vgmMAT(matrix, 1, 1) * fy )
					/ r2_fx2_fy2;

				Paint->rgConstantLin = ( cx * fx  + cy * fy ) / r2_fx2_fy2;

				/*
				**			r^2 (dx^2 + dy^2) - (dx fy - dy fx)^2
				**	gRad := -------------------------------------
				**					(r^2 - fx^2 - fy^2)^2
				**
				**			r^2 (dx^2 + dy^2) - dx^2 fy^2 - dy^2 fx^2 + 2 dx dy fx fy
				**		 := ---------------------------------------------------------
				**							  (r^2 - fx^2 - fy^2)^2
				**
				**			dx^2 (r^2 - fy^2) + dy^2 (r^2 - fx^2) + 2 dx dy fx fy
				**		 := -----------------------------------------------------
				**							(r^2 - fx^2 - fy^2)^2
				**
				**	First, lets factor dx^2 into
				**
				**		dx^2 = (x m00 + y m01 + cx)^2
				**			 = x^2 m00^2 + y^2 m01^2 + 2 x y m00 m01
				**			 + 2 x m00 cx + 2 y m01 cx + cx^2
				**
				**			 = x^2 (m00^2)
				**			 + y^2 (m01^2)
				**			 + x y (2 m00 m01)
				**			 + x (2 m00 cx)
				**			 + y (2 m01 cx)
				**			 + cx^2.
				**
				**	The same can be done for dy^2:
				**
				**		dy^2 = x^2 (m10^2)
				**			 + y^2 (m11^2)
				**			 + x y (2 m10 m11)
				**			 + x (2 m10 cy)
				**			 + y (2 m11 cy)
				**			 + cy^2.
				**
				**	Let's also factor dx dy into
				**
				**		dx dy = (x m00 + y m01 + cx) (x m10 + y m11 + cy)
				**			  = x^2 m00 m10 + y^2 m01 m11 + x y m00 m11 + x y m01 m10
				**			  + x m00 cy + x m10 cx + y m01 cy + y m11 cx + cx cy
				**
				**			  = x^2 (m00 m10)
				**			  + y^2 (m01 m11)
				**			  + x y (m00 m11 + m01 m10)
				**			  + x (m00 cy + m10 cx)
				**			  + y (m01 cy + m11 cx)
				**			  + cx cy.
				**
				**	Now that we have all this, lets look at the top of gRad.
				**
				**		= dx^2 (r^2 - fy^2) + dy^2 (r^2 - fx^2) + 2 dx dy fx fy
				**		= x^2 m00^2 (r^2 - fy^2) + y^2 m01^2 (r^2 - fy^2)
				**		+ x y 2 m00 m01 (r^2 - fy^2) + x 2 m00 cx (r^2 - fy^2)
				**		+ y 2 m01 cx (r^2 - fy^2) + cx^2 (r^2 - fy^2)
				**		+ x^2 m10^2 (r^2 - fx^2) + y^2 m11^2 (r^2 - fx^2)
				**		+ x y 2 m10 m11 (r^2 - fx^2) + x 2 m10 cy (r^2 - fx^2)
				**		+ y 2 m11 cy (r^2 - fx^2) + cy^2 (r^2 - fx^2)
				**		+ x^2 m00 m10 2 fx fy + y^2 m01 m11 2 fx fy
				**		+ x y (m00 m11 + m01 m10) 2 fx fy
				**		+ x (m00 cy + m10 cx) 2 fx fy + y (m01 cy + m11 cx) 2 fx fy
				**		+ cx cy 2 fx fy
				**
				**		= x^2 ( m00^2 (r^2 - fy^2)
				**			  + m10^2 (r^2 - fx^2)
				**			  + m00 m10 2 fx fy
				**			  )
				**		+ y^2 ( m01^2 (r^2 - fy^2)
				**			  + m11^2 (r^2 - fx^2)
				**			  + m01 m11 2 fx fy
				**			  )
				**		+ x y ( 2 m00 m01 (r^2 - fy^2)
				**			  + 2 m10 m11 (r^2 - fx^2)
				**			  + (m00 m11 + m01 m10) 2 fx fy
				**			  )
				**		+ x ( 2 m00 cx (r^2 - fy^2)
				**			+ 2 m10 cy (r^2 - fx^2)
				**			+ (m00 cy + m10 cx) 2 fx fy
				**			)
				**		+ y ( 2 m01 cx (r^2 - fy^2)
				**			+ 2 m11 cy (r^2 - fx^2)
				**			+ (m01 cy + m11 cx) 2 fx fy
				**			)
				**		+ cx^2 (r^2 - fy^2) + cy^2 (r^2 - fx^2) + cx cy 2 fx fy.
				*/

				Paint->rgStepXXRad =
					(
						  vgmMAT(matrix, 0, 0) * vgmMAT(matrix, 0, 0) * r2_fy2
						+ vgmMAT(matrix, 1, 0) * vgmMAT(matrix, 1, 0) * r2_fx2
						+ vgmMAT(matrix, 0, 0) * vgmMAT(matrix, 1, 0) * fxfy_2
					)
					/ r2_fx2_fy2sq;

				Paint->rgStepYYRad =
					(
						  vgmMAT(matrix, 0, 1) * vgmMAT(matrix, 0, 1) * r2_fy2
						+ vgmMAT(matrix, 1, 1) * vgmMAT(matrix, 1, 1) * r2_fx2
						+ vgmMAT(matrix, 0, 1) * vgmMAT(matrix, 1, 1) * fxfy_2
					)
					/ r2_fx2_fy2sq;

				Paint->rgStepXYRad =
					(
						  vgmMAT(matrix, 0, 0) * vgmMAT(matrix, 0, 1) * r2_fy2_2
						+ vgmMAT(matrix, 1, 0) * vgmMAT(matrix, 1, 1) * r2_fx2_2
						+ (
							  vgmMAT(matrix, 0, 0) * vgmMAT(matrix, 1, 1)
							+ vgmMAT(matrix, 0, 1) * vgmMAT(matrix, 1, 0)
						  )
						  * fxfy_2
					)
					/ r2_fx2_fy2sq;

				Paint->rgStepXRad =
					(
						  vgmMAT(matrix, 0, 0) * cx * r2_fy2_2
						+ vgmMAT(matrix, 1, 0) * cy * r2_fx2_2
						+ (
							  vgmMAT(matrix, 0, 0) * cy
							+ vgmMAT(matrix, 1, 0) * cx
						  )
						  * fxfy_2
					)
					/ r2_fx2_fy2sq;

				Paint->rgStepYRad =
					(
						  vgmMAT(matrix, 0, 1) * cx * r2_fy2_2
						+ vgmMAT(matrix, 1, 1) * cy * r2_fx2_2
						+ (
							  vgmMAT(matrix, 0, 1) * cy
							+ vgmMAT(matrix, 1, 1) * cx
						  )
						  * fxfy_2
					)
					/ r2_fx2_fy2sq;

				Paint->rgConstantRad =
					(
						  cx * cx * r2_fy2
						+ cy * cy * r2_fx2
						+ cx * cy * fxfy_2
					)
					/ r2_fx2_fy2sq;
#endif
				/* Mark as recomputed. */
				Paint->rgRecompute = gcvFALSE;
			}

			/* Program radial paint. */
			gcmERR_BREAK(gcoVG_SetRadialPaint(
				Context->vg,
				Paint->rgConstantLin, Paint->rgStepXLin, Paint->rgStepYLin,
				Paint->rgConstantRad, Paint->rgStepXRad, Paint->rgStepYRad,
				Paint->rgStepXXRad,
				Paint->rgStepYYRad,
				Paint->rgStepXYRad
				));
		}
		else
		{
			/* Program to the default color. */
			gcmERR_BREAK(_SetDefaultGradientColor(Context, Paint));
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

static gceSTATUS _UpdatePatternPaint(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	)
{
	/* Assume success. */
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		/* Do we have a pattern image set? */
		if (Paint->pattern != gcvNULL)
		{

			gceIMAGE_FILTER halImageQuality;
			VGImageQuality	imageQuality;

			/* Determine the image quality */
			imageQuality = Context->imageQuality & Paint->pattern->allowedQuality;

			switch (imageQuality)
			{
			case VG_IMAGE_QUALITY_FASTER:
				halImageQuality = gcvFILTER_LINEAR;
				break;

			case VG_IMAGE_QUALITY_BETTER:
				halImageQuality = gcvFILTER_BI_LINEAR;
				break;

			default:
				halImageQuality = gcvFILTER_POINT;
			}

			/* Program the pattern. */
			gcmERR_BREAK(gcoVG_SetPattern(
				Context->vg,
				Paint->pattern->surface,
				Paint->halPatternTilingMode,
				halImageQuality
				));

			/* Need to recompute? */
			if (Paint->patRecompute || vgvFORCE_PAINT_RECOMPUTE)
			{
				vgsMATRIX_PTR matrix;

				/* Make a shortcut to the current matrix. */
				matrix = Paint->matrix;

				/*
				**		(x + 0.5) m00 + (y + 0.5) m01 + m02
				**	u = -----------------------------------
				**				   pattern.width
				**
				**	We can factor the top line into:
				**
				**		= x m00
				**		+ y m01
				**		+ 0.5 (m00 + m01) + m02.
				**
				**	Note that at this point we have no clue what the pattern width is
				**	going to be, so we have to do the divide later.
				*/
#if gcdENABLE_PERFORMANCE_PRIOR
				Paint->patStepXLin = vgmFIXED_TO_FLOAT(vgmMAT(matrix, 0, 0));
				Paint->patStepYLin = vgmFIXED_TO_FLOAT(vgmMAT(matrix, 0, 1));

				Paint->patConstantLin
					= vgmFIXED_TO_FLOAT(vgmFIXED_MUL(vgvFIXED_HALF, vgmMAT(matrix, 0, 0) + vgmMAT(matrix, 0, 1))
					+ vgmMAT(matrix, 0, 2));

				/*
				**		(x + 0.5) m10 + (y + 0.5) m11 + m12
				**	v = -----------------------------------
				**				   pattern.height
				**
				**	We can factor the top line into:
				**
				**		= x m10
				**		+ y m11
				**		+ 0.5 (m10 + m11) + m12.
				**
				**	Note that at this point we have no clue what the pattern height is
				**	going to be, so we have to do the divide later.
				*/

				Paint->patStepXRad = vgmFIXED_TO_FLOAT(vgmMAT(matrix, 1, 0));
				Paint->patStepYRad = vgmFIXED_TO_FLOAT(vgmMAT(matrix, 1, 1));

				Paint->patConstantRad
					= vgmFIXED_TO_FLOAT(vgmFIXED_MUL(vgvFIXED_HALF, vgmMAT(matrix, 1, 0) + vgmMAT(matrix, 1, 1))
					+ vgmMAT(matrix, 1, 2));
#else
				Paint->patStepXLin = vgmMAT(matrix, 0, 0);
				Paint->patStepYLin = vgmMAT(matrix, 0, 1);

				Paint->patConstantLin
					= 0.5f * (vgmMAT(matrix, 0, 0) + vgmMAT(matrix, 0, 1))
					+ vgmMAT(matrix, 0, 2);

				/*
				**		(x + 0.5) m10 + (y + 0.5) m11 + m12
				**	v = -----------------------------------
				**				   pattern.height
				**
				**	We can factor the top line into:
				**
				**		= x m10
				**		+ y m11
				**		+ 0.5 (m10 + m11) + m12.
				**
				**	Note that at this point we have no clue what the pattern height is
				**	going to be, so we have to do the divide later.
				*/

				Paint->patStepXRad = vgmMAT(matrix, 1, 0);
				Paint->patStepYRad = vgmMAT(matrix, 1, 1);

				Paint->patConstantRad
					= 0.5f * (vgmMAT(matrix, 1, 0) + vgmMAT(matrix, 1, 1))
					+ vgmMAT(matrix, 1, 2);
#endif
				/* Divide by the pattern size. */
				Paint->patConstantLin /= Paint->patternWidth;
				Paint->patStepXLin    /= Paint->patternWidth;
				Paint->patStepYLin    /= Paint->patternWidth;

				Paint->patConstantRad /= Paint->patternHeight;
				Paint->patStepXRad    /= Paint->patternHeight;
				Paint->patStepYRad    /= Paint->patternHeight;

				/* Mark as recomputed. */
				Paint->patRecompute = gcvFALSE;
			}

			/* Program pattern paint. */
			gcmERR_BREAK(gcoVG_SetPatternPaint(
				Context->vg,
				Paint->patConstantLin, Paint->patStepXLin, Paint->patStepYLin,
				Paint->patConstantRad, Paint->patStepXRad, Paint->patStepYRad,
				Paint->pattern->surfaceFormat->linear
				));
		}
		else
		{
			gcmERR_BREAK(gcoVG_SetSolidPaint(
				Context->vg,
				Paint->byteColor[0],
				Paint->byteColor[1],
				Paint->byteColor[2],
				Paint->byteColor[3]
				));
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _FreePattern
**
** Free the pattern object from the paint.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Paint
**       Pointer to the paint object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _FreePattern(
	vgsCONTEXT_PTR Context,
	vgsPAINT_PTR Paint
	)
{
	gceSTATUS status;

	do
	{
		/* Release the pattern image. */
		if (Paint->pattern != gcvNULL)
		{
			/* Set as not used as a pattern. */
			gcmERR_BREAK(vgfUseImageAsPattern(
				Context, Paint->pattern, VG_FALSE
				));

			/* Reset the pattern pointer. */
			Paint->pattern = gcvNULL;
		}
		else
		{
			/* Success. */
			status = gcvSTATUS_OK;
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _PaintDestructor
**
** Paint object destructor. Called when the object is being destroyed.
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

static gceSTATUS _PaintDestructor(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		/* Cast the object. */
		vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

		/* Release the pattern image. */
		gcmERR_BREAK(_FreePattern(
			Context, paint
			));

		/* Destroy color ramp surface. */
		gcmERR_BREAK(_FreeRamp(
			Context, paint
			));
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
************************** Individual State Functions **************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*------------------------------ VG_PAINT_TYPE -------------------------------*/

vgmVALID_VALUE_ARRAY(VG_PAINT_TYPE)
{
	VG_PAINT_TYPE_COLOR,
	VG_PAINT_TYPE_LINEAR_GRADIENT,
	VG_PAINT_TYPE_RADIAL_GRADIENT,
	VG_PAINT_TYPE_PATTERN
};

vgmSETSTATE_FUNCTION(VG_PAINT_TYPE)
{
	static vgtUPDATEPAINT _update[] =
	{
		_UpdateColorPaint,			/* VG_PAINT_TYPE_COLOR           */
		_UpdateLinearGradientPaint,	/* VG_PAINT_TYPE_LINEAR_GRADIENT */
		_UpdateRadialGradientPaint,	/* VG_PAINT_TYPE_RADIAL_GRADIENT */
		_UpdatePatternPaint			/* VG_PAINT_TYPE_PATTERN         */
	};

	gctUINT index;

	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Set new paint type. */
	paint->type
		= (VGPaintType) * (VGint *) Values;

	/* Convert to HAL enumeration. */
	index = paint->type - VG_PAINT_TYPE_COLOR;

	/* Set the update function. */
	paint->update = _update[index];

	/* Invalidate paint. */
	paint->lgRecompute  = gcvTRUE;
	paint->rgRecompute  = gcvTRUE;
	paint->patRecompute = gcvTRUE;
}

vgmGETSTATE_FUNCTION(VG_PAINT_TYPE)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Get the color. */
	ValueConverter(
		Values, &paint->type, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*------------------------------ VG_PAINT_COLOR ------------------------------*/

vgmSETSTATE_FUNCTION(VG_PAINT_COLOR)
{
	gctFLOAT_PTR floarColor;
	gctUINT8_PTR byteColor;

	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Set the new color. */
	ValueConverter(
		&paint->floatColor, Values, 4, VG_FALSE, VG_FALSE
		);

	/* Make shortcuts to the color arrays. */
	floarColor = paint->floatColor;
	byteColor  = paint->byteColor;

	/* Convert to 8-bit per component. */
	byteColor[0] = gcoVG_PackColorComponent(floarColor[0]);
	byteColor[1] = gcoVG_PackColorComponent(floarColor[1]);
	byteColor[2] = gcoVG_PackColorComponent(floarColor[2]);
	byteColor[3] = gcoVG_PackColorComponent(floarColor[3]);
}

vgmGETSTATE_FUNCTION(VG_PAINT_COLOR)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Get the color. */
	ValueConverter(
		Values, &paint->floatColor, 4, VG_FALSE, VG_TRUE
		);
}

vgmGETARRAYSIZE_FUNCTION(VG_PAINT_COLOR)
{
	return 4;
}

/*----------------------------------------------------------------------------*/
/*------------------------- VG_PAINT_LINEAR_GRADIENT -------------------------*/

vgmSETSTATE_FUNCTION(VG_PAINT_LINEAR_GRADIENT)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Set the linear gradient. */
	ValueConverter(
		paint->linearGradient, Values, 4, VG_FALSE, VG_FALSE
		);

	/* Invalidate linear gradient. */
	paint->lgRecompute = gcvTRUE;
}

vgmGETSTATE_FUNCTION(VG_PAINT_LINEAR_GRADIENT)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Get the linear gradient. */
	ValueConverter(
		Values, &paint->linearGradient, 4, VG_FALSE, VG_TRUE
		);
}

vgmGETARRAYSIZE_FUNCTION(VG_PAINT_LINEAR_GRADIENT)
{
	return 4;
}

/*----------------------------------------------------------------------------*/
/*------------------------- VG_PAINT_RADIAL_GRADIENT -------------------------*/

vgmSETSTATE_FUNCTION(VG_PAINT_RADIAL_GRADIENT)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Set the radial gradient. */
	ValueConverter(
		paint->radialGradient, Values, 5, VG_FALSE, VG_FALSE
		);

	/* Invalidate linear gradient. */
	paint->rgRecompute = gcvTRUE;
}

vgmGETSTATE_FUNCTION(VG_PAINT_RADIAL_GRADIENT)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Get the radial gradient. */
	ValueConverter(
		Values, &paint->radialGradient, 5, VG_FALSE, VG_TRUE
		);
}

vgmGETARRAYSIZE_FUNCTION(VG_PAINT_RADIAL_GRADIENT)
{
	return 5;
}

/*----------------------------------------------------------------------------*/
/*------------------------- VG_PAINT_COLOR_RAMP_STOPS ------------------------*/

vgmSETSTATE_FUNCTION(VG_PAINT_COLOR_RAMP_STOPS)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Set the color ramp count. */
	paint->vgColorRampLength = Count;

	/* Set the color ramp. */
	ValueConverter(
		paint->vgColorRamp, Values, Count * 5, VG_FALSE, VG_FALSE
		);

	/* Mark the internal color ramp as dirty. */
	paint->intColorRampRecompute = gcvTRUE;
}

vgmGETSTATE_FUNCTION(VG_PAINT_COLOR_RAMP_STOPS)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Clamp the count. */
	if ((gctSIZE_T) Count > paint->vgColorRampLength)
	{
		Count = paint->vgColorRampLength;
	}

	/* Get the color ramp values. */
	ValueConverter(
		Values, &paint->vgColorRamp, Count * 5, VG_FALSE, VG_TRUE
		);
}

vgmGETARRAYSIZE_FUNCTION(VG_PAINT_COLOR_RAMP_STOPS)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	return paint->vgColorRampLength * 5;
}

/*----------------------------------------------------------------------------*/
/*--------------------- VG_PAINT_COLOR_RAMP_PREMULTIPLIED --------------------*/

vgmVALID_VALUE_ARRAY(VG_PAINT_COLOR_RAMP_PREMULTIPLIED)
{
	VG_FALSE,
	VG_TRUE
};

vgmSETSTATE_FUNCTION(VG_PAINT_COLOR_RAMP_PREMULTIPLIED)
{
	VGboolean colorRampPremultiplied;

	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Set premultiplied flag. */
	colorRampPremultiplied = (VGboolean) * (VGint *) Values;

	/* Changed? */
	if (paint->colorRampPremultiplied != colorRampPremultiplied)
	{
		paint->colorRampSurfaceRecompute = gcvTRUE;
		paint->colorRampPremultiplied    = colorRampPremultiplied;
	}
}

vgmGETSTATE_FUNCTION(VG_PAINT_COLOR_RAMP_PREMULTIPLIED)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Get premultiplied flag. */
	ValueConverter(
		Values, &paint->colorRampPremultiplied, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*---------------------- VG_PAINT_COLOR_RAMP_SPREAD_MODE ---------------------*/

vgmVALID_VALUE_ARRAY(VG_PAINT_COLOR_RAMP_SPREAD_MODE)
{
	VG_COLOR_RAMP_SPREAD_PAD,
	VG_COLOR_RAMP_SPREAD_REPEAT,
	VG_COLOR_RAMP_SPREAD_REFLECT
};

vgmSETSTATE_FUNCTION(VG_PAINT_COLOR_RAMP_SPREAD_MODE)
{
	static gceTILE_MODE _spreadMode[] =
	{
		gcvTILE_PAD,		/* VG_COLOR_RAMP_SPREAD_PAD     */
		gcvTILE_REPEAT,		/* VG_COLOR_RAMP_SPREAD_REPEAT  */
		gcvTILE_REFLECT		/* VG_COLOR_RAMP_SPREAD_REFLECT */
	};

	gctUINT index;

	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Set the spread mode. */
	paint->colorRampSpreadMode
		= (VGColorRampSpreadMode) * (VGint *) Values;

	/* Convert to HAL enumeration. */
	index = paint->colorRampSpreadMode - VG_COLOR_RAMP_SPREAD_PAD;
	paint->halColorRampSpreadMode = _spreadMode[index];
}

vgmGETSTATE_FUNCTION(VG_PAINT_COLOR_RAMP_SPREAD_MODE)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Get the spread mode. */
	ValueConverter(
		Values, &paint->colorRampSpreadMode, 1, VG_FALSE, VG_TRUE
		);
}

/*----------------------------------------------------------------------------*/
/*----------------------- VG_PAINT_PATTERN_TILING_MODE -----------------------*/

vgmVALID_VALUE_ARRAY(VG_PAINT_PATTERN_TILING_MODE)
{
	VG_TILE_FILL,
	VG_TILE_PAD,
	VG_TILE_REPEAT,
	VG_TILE_REFLECT
};

vgmSETSTATE_FUNCTION(VG_PAINT_PATTERN_TILING_MODE)
{
	static gceTILE_MODE _tilingMode[] =
	{
		gcvTILE_FILL,		/* VG_TILE_FILL    */
		gcvTILE_PAD,		/* VG_TILE_PAD     */
		gcvTILE_REPEAT,		/* VG_TILE_REPEAT  */
		gcvTILE_REFLECT		/* VG_TILE_REFLECT */
	};

	gctUINT index;

	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Set the tiling mode. */
	paint->patternTilingMode
		= (VGTilingMode) * (VGint *) Values;

	/* Determine the spread mode index. */
	index = paint->patternTilingMode - VG_TILE_FILL;
	paint->halPatternTilingMode = _tilingMode[index];
}

vgmGETSTATE_FUNCTION(VG_PAINT_PATTERN_TILING_MODE)
{
	/* Cast the object. */
	vgsPAINT_PTR paint = (vgsPAINT_PTR) Object;

	/* Get the tiling mode. */
	ValueConverter(
		Values, &paint->patternTilingMode, 1, VG_FALSE, VG_TRUE
		);
}


/******************************************************************************\
***************************** Context State Array ******************************
\******************************************************************************/

static vgsSTATEENTRY _stateArray[] =
{
	vgmDEFINE_ENUMERATED_ENTRY(VG_PAINT_TYPE),
	vgmDEFINE_ARRAY_ENTRY(VG_PAINT_COLOR, FLOAT, 4, 1),
	vgmDEFINE_ARRAY_ENTRY(VG_PAINT_LINEAR_GRADIENT, FLOAT, 4, 1),
	vgmDEFINE_ARRAY_ENTRY(VG_PAINT_RADIAL_GRADIENT, FLOAT, 5, 1),
	vgmDEFINE_ARRAY_ENTRY(VG_PAINT_COLOR_RAMP_STOPS, FLOAT, 5,
						  vgvMAX_COLOR_RAMP_STOPS),
	vgmDEFINE_ENUMERATED_ENTRY(VG_PAINT_COLOR_RAMP_PREMULTIPLIED),
	vgmDEFINE_ENUMERATED_ENTRY(VG_PAINT_COLOR_RAMP_SPREAD_MODE),
	vgmDEFINE_ENUMERATED_ENTRY(VG_PAINT_PATTERN_TILING_MODE)
};


/******************************************************************************\
************************ Internal Paint Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgfReferencePaint
**
** vgfReferencePaint increments the reference count of the given VGPaint object.
** If the object does not exist yet, the object will be created and initialized
** with the default values.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    vgsPAINT_PTR * Paint
**       Handle to the new VGPaint object.
*/

gceSTATUS vgfReferencePaint(
	IN vgsCONTEXT_PTR Context,
	IN OUT vgsPAINT_PTR * Paint
	)
{
	gceSTATUS status, last;
	vgsPAINT_PTR paint = gcvNULL;

	do
	{
		/* Create the object if does not exist yet. */
		if (*Paint == gcvNULL)
		{
			/* Allocate the object structure. */
			gcmERR_BREAK(gcoOS_Allocate(
				Context->os,
				gcmSIZEOF(vgsPAINT),
				(gctPOINTER *) &paint
				));

			/* Initialize the object structure. */
			paint->object.type           = vgvOBJECTTYPE_PAINT;
			paint->object.prev           = gcvNULL;
			paint->object.next           = gcvNULL;
			paint->object.referenceCount = 0;
			paint->object.userValid      = VG_TRUE;

			/* Insert the object into the cache. */
			gcmERR_BREAK(vgfObjectCacheInsert(
				Context, (vgsOBJECT_PTR) paint
				));

			/* Set default object states. */
			vgmSET_OBJECT_ENUM(
				paint,
				VG_PAINT_TYPE,
				VG_PAINT_TYPE_COLOR
				);

			vgmSET_OBJECT_ENUM(
				paint,
				VG_PAINT_COLOR_RAMP_SPREAD_MODE,
				VG_COLOR_RAMP_SPREAD_PAD
				);

			vgmSET_OBJECT_ENUM(
				paint,
				VG_PAINT_PATTERN_TILING_MODE,
				VG_TILE_FILL
				);

			vgmSET_OBJECT_ENUM(
				paint,
				VG_PAINT_COLOR_RAMP_PREMULTIPLIED,
				VG_TRUE
				);

			vgmSET_OBJECT_NULL_ARRAY(
				paint,
				VG_PAINT_COLOR_RAMP_STOPS,
				FLOAT
				);

			vgmSET_OBJECT_ARRAY(
				paint, VG_PAINT_COLOR, FLOAT, vgvFLOATCOLOR0001
				);

			{
				static vgtFLOATVECTOR4 linearGradient =
					{ 0.0f, 0.0f, 1.0f, 0.0f };

				vgmSET_OBJECT_ARRAY(
					paint, VG_PAINT_LINEAR_GRADIENT, FLOAT, linearGradient
					);
			}

			{
				static vgtFLOATVECTOR5 radialGradient =
					{ 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

				vgmSET_OBJECT_ARRAY(
					paint, VG_PAINT_RADIAL_GRADIENT, FLOAT, radialGradient
					);
			}

			/* Reset color ramp surface. */
			paint->colorRampSurface = gcvNULL;
			paint->colorRampBits    = gcvNULL;

			/* Reset pattern. */
			paint->pattern = gcvNULL;

			/* Reset the current surface-to-paint matrix. */
			paint->matrix = gcvNULL;

			/* Set the result pointer. */
			*Paint = paint;
		}

		/* Increment the reference count. */
		(*Paint)->object.referenceCount++;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Roll back. */
	if (paint != gcvNULL)
	{
		gcmCHECK_STATUS(gcoOS_Free(Context->os, paint));
	}

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgfSetPaintObjectList
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

void vgfSetPaintObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	)
{
	ListEntry->stateArray     = _stateArray;
	ListEntry->stateArraySize = gcmCOUNTOF(_stateArray);
	ListEntry->destructor     = _PaintDestructor;
}


/*******************************************************************************
**
** vgfUpdatePaint
**
** Update hardware paint states as necessary.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    PaintMode
**       Requested paint mode.
**
** OUTPUT:
**
**    Nothing.
*/

gceSTATUS vgfUpdatePaint(
	IN vgsCONTEXT_PTR Context,
	IN vgsMATRIXCONTAINER_PTR Matrix,
	IN vgsPAINT_PTR Paint
	)
{
	gceSTATUS status;

	/* Update the matrix. */
	Matrix->update(Context);

	/* Update the paint. */
	status = Paint->update(Context, Paint);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgfPaintMatrixChanged
**
** This function is called when the surface-to-paint matrix changes.
**
** INPUT:
**
**    Paint
**       Pointer to the paint to invalidate.
**
**    SurfaceToPaint
**       Pointer to the surface-to-paint matrix.
**
** OUTPUT:
**
**    Nothing.
*/

void vgfPaintMatrixChanged(
	IN vgsPAINT_PTR Paint,
	IN vgsMATRIX_PTR SurfaceToPaint
	)
{
	/* Verify the arguments. */
	gcmASSERT(Paint != gcvNULL);
	gcmASSERT(SurfaceToPaint != gcvNULL);

	/* Invalidate paint. */
	Paint->lgRecompute  = gcvTRUE;
	Paint->rgRecompute  = gcvTRUE;
	Paint->patRecompute = gcvTRUE;

	/* Set current matrix. */
	Paint->matrix = SurfaceToPaint;
}


/*******************************************************************************
**
** vgfPaintMatrixVerifyCurrent
**
** Reset the current matrix if not the same as specified.
**
** INPUT:
**
**    Paint
**       Pointer to the paint to invalidate.
**
**    SurfaceToPaint
**       Pointer to the surface-to-paint matrix.
**
** OUTPUT:
**
**    Nothing.
*/

void vgfPaintMatrixVerifyCurrent(
	IN vgsPAINT_PTR Paint,
	IN vgsMATRIX_PTR SurfaceToPaint
	)
{
	/* Verify the arguments. */
	gcmASSERT(Paint != gcvNULL);
	gcmASSERT(SurfaceToPaint != gcvNULL);

	/* Not the same? */
	if (Paint->matrix != SurfaceToPaint)
	{
		/* Invalidate paint. */
		Paint->lgRecompute  = gcvTRUE;
		Paint->rgRecompute  = gcvTRUE;
		Paint->patRecompute = gcvTRUE;

		/* Set current matrix. */
		Paint->matrix = SurfaceToPaint;
	}
}


/******************************************************************************\
************************** OpenVG Paint Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgCreatePaint
**
** vgCreatePaint creates a new paint object that is initialized to a set of
** default values and returns a VGPaint handle to it. If insufficient memory
** is available to allocate a new object, VG_INVALID_HANDLE is returned.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    VGPaint
**       Handle to the new VGPaint object.
*/

VG_API_CALL VGPaint VG_API_ENTRY vgCreatePaint(
	void
	)
{
	vgsPAINT_PTR paint = gcvNULL;

	vgmENTERAPI(vgCreatePaint)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s();\n",
			__FUNCTION__
			);

		/* Allocate a new object. */
		if (gcmIS_ERROR(vgfReferencePaint(Context, &paint)))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s() = 0x%08X;\n",
			__FUNCTION__,
			paint
			);
	}
	vgmLEAVEAPI(vgCreatePaint);

	return (VGPaint) paint;
}


/*******************************************************************************
**
** vgDestroyPaint
**
** The resources associated with a paint object may be deallocated by calling
** vgDestroyPaint. Following the call, the paint handle is no longer valid in
** any of the contexts that shared it. If the paint object is currently active
** in a drawing context, the context continues to access it until it is replaced
** or the context is destroyed.
**
** INPUT:
**
**    Paint
**       Handle to a valid VGPaint object.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgDestroyPaint(
	VGPaint Paint
	)
{
	vgmENTERAPI(vgDestroyPaint)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X);\n",
			__FUNCTION__,
			Paint
			);

		if (!vgfVerifyUserObject(Context, Paint, vgvOBJECTTYPE_PAINT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Invalidate the object. */
		((vgsOBJECT_PTR) Paint)->userValid = VG_FALSE;

		/* Decrement the reference count. */
		gcmVERIFY_OK(vgfDereferenceObject(
			Context,
			(vgsOBJECT_PTR *) &Paint
			));
	}
	vgmLEAVEAPI(vgDestroyPaint);
}


/*******************************************************************************
**
** vgSetPaint
**
** Paint definitions are set on the current context using the vgSetPaint
** function. The PaintModes argument is a bitwise OR of values from the
** VGPaintMode enumeration, determining whether the paint object is to be
** used for filling (VG_FILL_PATH), stroking (VG_STROKE_PATH), or both
** (VG_FILL_PATH | VG_STROKE_PATH). The current paint replaces the previously
** set paint object, if any, for the given paint mode or modes. If Paint is
** equal to VG_INVALID_HANDLE, the previously set paint object for the given
** mode (if present) is removed and the paint settings are restored to their
** default values.
**
** INPUT:
**
**    Paint
**       Handle to a valid VGPaint object.
**
**    PaintModes
**       Bitwise OR of values from the VGPaintMode enumeration, determining
**       what the paint is to be used for.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgSetPaint(
	VGPaint Paint,
	VGbitfield PaintModes
	)
{
	vgmENTERAPI(vgSetPaint)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%04X);\n",
			__FUNCTION__,
			Paint, PaintModes
			);

		/* Validate paint modes. */
		if ((PaintModes == 0) ||
			((PaintModes & ~(VG_STROKE_PATH | VG_FILL_PATH)) != 0))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Reset to the default? */
		if (Paint == VG_INVALID_HANDLE)
		{
			if ((PaintModes & VG_STROKE_PATH) == VG_STROKE_PATH)
			{
				if ((Context->strokePaint != gcvNULL) && (Context->strokePaint != Context->defaultPaint))
				{
					vgfDereferenceObject(Context, (vgsOBJECT_PTR*)&Context->strokePaint);
				}

				Context->strokePaint = Context->defaultPaint;
				Context->strokeDefaultPaint = VG_TRUE;
			}

			if ((PaintModes & VG_FILL_PATH) == VG_FILL_PATH)
			{
				if ((Context->fillPaint != gcvNULL) && (Context->fillPaint != Context->defaultPaint))
				{
					vgfDereferenceObject(Context, (vgsOBJECT_PTR*)&Context->fillPaint);
				}

				Context->fillPaint = Context->defaultPaint;
				Context->fillDefaultPaint = VG_TRUE;
			}
		}

		else
		{
			if (!vgfVerifyUserObject(Context, Paint, vgvOBJECTTYPE_PAINT))
			{
				vgmERROR(VG_BAD_HANDLE_ERROR);
				break;
			}

			if ((PaintModes & VG_STROKE_PATH) == VG_STROKE_PATH)
			{
				if ((Context->strokePaint != gcvNULL) && (Context->strokePaint != Context->defaultPaint))
				{
					vgfDereferenceObject(Context, (vgsOBJECT_PTR*)&Context->strokePaint);
				}

				Context->strokePaint = (vgsPAINT_PTR) Paint;

				gcmVERIFY_OK(vgfReferencePaint(Context, (vgsPAINT_PTR*)&Context->strokePaint));

				Context->strokeDefaultPaint = VG_FALSE;
			}

			if ((PaintModes & VG_FILL_PATH) == VG_FILL_PATH)
			{
				if ((Context->fillPaint != gcvNULL) && (Context->fillPaint != Context->defaultPaint))
				{
					vgfDereferenceObject(Context, (vgsOBJECT_PTR*)&Context->fillPaint);
				}

				Context->fillPaint = (vgsPAINT_PTR) Paint;

				gcmVERIFY_OK(vgfReferencePaint(Context, (vgsPAINT_PTR*)&Context->fillPaint));

				Context->fillDefaultPaint = VG_FALSE;
			}
		}
	}
	vgmLEAVEAPI(vgSetPaint);
}


/*******************************************************************************
**
** vgGetPaint
**
** The vgGetPaint function returns the paint object currently set for the given
** PaintMode, or VG_INVALID_HANDLE if an error occurs or if no paint object is
** set (i.e., the default paint is present) on the given context with the given
** PaintMode.
**
** INPUT:
**
**    PaintModes
**       One of the VGPaintMode values.
**
** OUTPUT:
**
**    VGPaint
**       Handle to the VGPaint object associated with the given mode.
*/

VG_API_CALL VGPaint VG_API_ENTRY vgGetPaint(
	VGPaintMode PaintMode
	)
{
	VGPaint paint = VG_INVALID_HANDLE;

	vgmENTERAPI(vgGetPaint)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%04X);\n",
			__FUNCTION__,
			PaintMode
			);

		switch (PaintMode)
		{
		case VG_STROKE_PATH:
			if (!Context->strokeDefaultPaint)
			{
				paint = (VGPaint) Context->strokePaint;
			}
			break;

		case VG_FILL_PATH:
			if (!Context->fillDefaultPaint)
			{
				paint = (VGPaint) Context->fillPaint;
			}
			break;

		default:
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
		}
	}
	vgmLEAVEAPI(vgGetPaint);

	return paint;
}


/*******************************************************************************
**
** vgSetColor
**
** As a shorthand, the vgSetColor function allows the VG_PAINT_COLOR parameter
** of a given paint object to be set using a 32-bit non-premultiplied sRGBA_8888
** representation. The RGBA parameter is a VGuint with 8 bits of red starting at
** the most significant bit, followed by 8 bits each of green, blue, and alpha.
** Each color or alpha channel value is conceptually divided by 255.0f to obtain
** a value between 0 and 1.
**
** INPUT:
**
**    Paint
**       Handle to a valid VGPaint object.
**
**    RGBA
**       Paint color value to be set.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgSetColor(
	VGPaint Paint,
	VGuint RGBA
	)
{
	vgmENTERAPI(vgSetColor)
	{
		vgsPAINT_PTR paint;
		vgtFLOATVECTOR4 color;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X);\n",
			__FUNCTION__,
			Paint, RGBA
			);

		if (!vgfVerifyUserObject(Context, Paint, vgvOBJECTTYPE_PAINT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		paint = (vgsPAINT_PTR) Paint;

		/* Convert color. */
		color[0] = vgmGETRED(RGBA)   / 255.0f;
		color[1] = vgmGETGREEN(RGBA) / 255.0f;
		color[2] = vgmGETBLUE(RGBA)  / 255.0f;
		color[3] = vgmGETALPHA(RGBA) / 255.0f;

		/* Set new color. */
		vgmSET_OBJECT_ARRAY(paint, VG_PAINT_COLOR, FLOAT, color);
	}
	vgmLEAVEAPI(vgSetColor);
}


/*******************************************************************************
**
** vgGetColor
**
** The current setting of the VG_PAINT_COLOR parameter on a given paint object
** may be queried as a 32-bit non-premultiplied sRGBA_8888 value. Each color
** channel or alpha value is clamped to the range [0, 1], multiplied by 255,
** and rounded to obtain an 8-bit integer; the resulting values are packed into
** a 32-bit value in the same format as for vgSetColor.
**
** INPUT:
**
**    Paint
**       Handle to a valid VGPaint object.
**
** OUTPUT:
**
**    VGuint
**       Paint color value.
*/

VG_API_CALL VGuint VG_API_ENTRY vgGetColor(
	VGPaint Paint
	)
{
	VGuint rgba = 0;

	vgmENTERAPI(vgGetColor)
	{
		vgsPAINT_PTR paint;
		VGfloat fred, fgreen, fblue, falpha;
		VGuint red, green, blue, alpha;

		if (!vgfVerifyUserObject(Context, Paint, vgvOBJECTTYPE_PAINT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		paint = (vgsPAINT_PTR) Paint;

		/* Convert color. */
		fred   = vgmCLAMP(paint->floatColor[0], 0.0f, 1.0f) * 255.0f;
		fgreen = vgmCLAMP(paint->floatColor[1], 0.0f, 1.0f) * 255.0f;
		fblue  = vgmCLAMP(paint->floatColor[2], 0.0f, 1.0f) * 255.0f;
		falpha = vgmCLAMP(paint->floatColor[3], 0.0f, 1.0f) * 255.0f;

		/* Round. */
		red   = (VGubyte) (fred   + 0.5f);
		green = (VGubyte) (fgreen + 0.5f);
		blue  = (VGubyte) (fblue  + 0.5f);
		alpha = (VGubyte) (falpha + 0.5f);

		/* Set the result. */
		rgba = (red << 24) | (green << 16) | (blue << 8) | alpha;
	}
	vgmLEAVEAPI(vgGetColor);

	return rgba;
}


/*******************************************************************************
**
** vgPaintPattern
**
** The vgPaintPattern function replaces any previous pattern image defined on
** the given paint object for the given set of paint modes with a new pattern
** image. A value of VG_INVALID_HANDLE for the pattern parameter removes the
** current pattern image from the paint object.
**
** If the current paint object has its VG_PAINT_TYPE parameter set to
** VG_PAINT_TYPE_PATTERN, but no pattern image is set, the paint object behaves
** as if VG_PAINT_TYPE were set to VG_PAINT_TYPE_COLOR.
**
** While an image is set as the paint pattern for any paint object, it may not
** be used as a rendering target. Conversely, an image that is currently
** a rendering target may not be set as a paint pattern.
**
** INPUT:
**
**    Paint
**       Handle to a valid VGPaint object.
**
**    Pattern
**       Handle to a valid VGImage object to be set as the new paint pattern.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgPaintPattern(
	VGPaint Paint,
	VGImage Pattern
	)
{
	gceSTATUS status;

	vgmENTERAPI(vgPaintPattern)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, 0x%08X);\n",
			__FUNCTION__,
			Paint, Pattern
			);

		if (!vgfVerifyUserObject(Context, Paint, vgvOBJECTTYPE_PAINT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Reset the pattern? */
		if (Pattern == VG_INVALID_HANDLE)
		{
			/* Reset the old pattern. */
			gcmERR_BREAK(_FreePattern(
				Context, (vgsPAINT_PTR) Paint
				));
		}

		/* Set new pattern. */
		else
		{
			vgsPAINT_PTR paint;
			vgsIMAGE_PTR pattern;
			gctUINT width, height;

			if (!vgfVerifyUserObject(Context, Pattern, vgvOBJECTTYPE_IMAGE))
			{
				vgmERROR(VG_BAD_HANDLE_ERROR);
				break;
			}

			/* Cast the objects. */
			paint   = (vgsPAINT_PTR) Paint;
			pattern = (vgsIMAGE_PTR) Pattern;

			/* No change? */
			if (paint->pattern == pattern)
			{
				break;
			}

			/* Set usage for the new pattern. */
			status = vgfUseImageAsPattern(
				Context, pattern, VG_TRUE
				);

			if (gcmIS_ERROR(status))
			{
				vgmERROR(VG_IMAGE_IN_USE_ERROR);
				break;
			}

			/* Reset the old pattern. */
			gcmVERIFY_OK(_FreePattern(
				Context, (vgsPAINT_PTR) Paint
				));

			/* Get the size of the pattern. */
			gcmVERIFY_OK(gcoSURF_GetSize(
				pattern->surface, &width, &height, gcvNULL
				));

			/* Set the size. */
			paint->patternWidth  = (gctFLOAT) width;
			paint->patternHeight = (gctFLOAT) height;

			/* Recompute the paint parameters. */
			paint->patRecompute = gcvTRUE;

			/* Set new pattern. */
			paint->pattern = pattern;
		}
	}
	vgmLEAVEAPI(vgPaintPattern);
}
