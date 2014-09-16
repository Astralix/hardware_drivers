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




/******************************************************************************\
********************************* VG_sRGBX_8888 ********************************
\******************************************************************************/

static void _ReadPixel_RGBX_8888_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_sRGBX_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);
}


/******************************************************************************\
********************************* VG_sRGBA_8888 ********************************
\******************************************************************************/

static void _ReadPixel_sRGBA_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_sRGBA_8888_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sRGBA_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);
}

static void _ReadPixel_sRGBA_8888_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
******************************* VG_sRGBA_8888_PRE ******************************
\******************************************************************************/

static void _ReadPixel_sRGBA_8888_PRE_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >>  8) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >> 24) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];
	}
}

static void _ReadPixel_sRGBA_8888_PRE_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >>  8) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >> 24) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);
	}
}

static void _ReadPixel_sRGBA_8888_PRE_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >>  8) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >> 24) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to linear color space. */
		Value[2] = vgfGetColorInverseGamma(Value[2]);
		Value[1] = vgfGetColorInverseGamma(Value[1]);
		Value[0] = vgfGetColorInverseGamma(Value[0]);
	}
}

static void _ReadPixel_sRGBA_8888_PRE_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >>  8) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >> 24) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to linear color space. */
		Value[2] = vgfGetColorInverseGamma(Value[2]);
		Value[1] = vgfGetColorInverseGamma(Value[1]);
		Value[0] = vgfGetColorInverseGamma(Value[0]);

		/* Premultiply. */
		Value[2] *= Value[3];
		Value[1] *= Value[3];
		Value[0] *= Value[3];
	}
}


/******************************************************************************\
********************************** VG_sRGB_565 *********************************
\******************************************************************************/

static void _ReadPixel_RGB_565_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	bINT =  value        & 0x1F;
	gINT = (value >>  5) & 0x3F;
	rINT = (value >> 11) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 6);
	Value[0] = vgmZERO2ONE(rINT, 5);
}

static void _ReadPixel_sRGB_565_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	bINT =  value        & 0x1F;
	gINT = (value >>  5) & 0x3F;
	rINT = (value >> 11) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 6);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);
}


/******************************************************************************\
********************************* VG_sRGBA_5551 ********************************
\******************************************************************************/

static void _ReadPixel_sRGBA_5551_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x01;
	bINT = (value >>  1) & 0x1F;
	gINT = (value >>  6) & 0x1F;
	rINT = (value >> 11) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 1);
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 5);
	Value[0] = vgmZERO2ONE(rINT, 5);
}

static void _ReadPixel_sRGBA_5551_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x01;
	bINT = (value >>  1) & 0x1F;
	gINT = (value >>  6) & 0x1F;
	rINT = (value >> 11) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 1);
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 5);
	Value[0] = vgmZERO2ONE(rINT, 5);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sRGBA_5551_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x01;
	bINT = (value >>  1) & 0x1F;
	gINT = (value >>  6) & 0x1F;
	rINT = (value >> 11) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 1);
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 5);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);
}

static void _ReadPixel_sRGBA_5551_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x01;
	bINT = (value >>  1) & 0x1F;
	gINT = (value >>  6) & 0x1F;
	rINT = (value >> 11) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 1);
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 5);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
********************************* VG_sRGBA_4444 ********************************
\******************************************************************************/

static void _ReadPixel_sRGBA_4444_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x0F;
	bINT = (value >>  4) & 0x0F;
	gINT = (value >>  8) & 0x0F;
	rINT = (value >> 12) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = vgmZERO2ONE(bINT, 4);
	Value[1] = vgmZERO2ONE(gINT, 4);
	Value[0] = vgmZERO2ONE(rINT, 4);
}

static void _ReadPixel_sRGBA_4444_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x0F;
	bINT = (value >>  4) & 0x0F;
	gINT = (value >>  8) & 0x0F;
	rINT = (value >> 12) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = vgmZERO2ONE(bINT, 4);
	Value[1] = vgmZERO2ONE(gINT, 4);
	Value[0] = vgmZERO2ONE(rINT, 4);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sRGBA_4444_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x0F;
	bINT = (value >>  4) & 0x0F;
	gINT = (value >>  8) & 0x0F;
	rINT = (value >> 12) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 4);
	Value[2] = vgmZERO2ONE_S2L(bINT, 4);
	Value[1] = vgmZERO2ONE_S2L(gINT, 4);
	Value[0] = vgmZERO2ONE_S2L(rINT, 4);
}

static void _ReadPixel_sRGBA_4444_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x0F;
	bINT = (value >>  4) & 0x0F;
	gINT = (value >>  8) & 0x0F;
	rINT = (value >> 12) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 4);
	Value[2] = vgmZERO2ONE_S2L(bINT, 4);
	Value[1] = vgmZERO2ONE_S2L(gINT, 4);
	Value[0] = vgmZERO2ONE_S2L(rINT, 4);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
************************************ VG_sL_8 ***********************************
\******************************************************************************/

static void _ReadPixel_L_8_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 lINT;

	/* Get the pixel value. */
	lINT = * Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 1;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] =
	Value[1] =
	Value[0] = vgmZERO2ONE(lINT, 8);
}

static void _ReadPixel_sL_8_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 lINT;

	/* Get the pixel value. */
	lINT = * Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 1;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] =
	Value[1] =
	Value[0] = vgmZERO2ONE_S2L(lINT, 8);
}


/******************************************************************************\
********************************* VG_lRGBX_8888 ********************************
\******************************************************************************/

static void _ReadPixel_lRGBX_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);
}


/******************************************************************************\
********************************* VG_lRGBA_8888 ********************************
\******************************************************************************/

static void _ReadPixel_lRGBA_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);
}

static void _ReadPixel_lRGBA_8888_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_lRGBA_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_lRGBA_8888_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >>  8) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >> 24) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
******************************* VG_lRGBA_8888_PRE ******************************
\******************************************************************************/

static void _ReadPixel_lRGBA_8888_PRE_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >>  8) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >> 24) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to non-linear color space. */
		Value[2] = vgfGetColorGamma(Value[2]);
		Value[1] = vgfGetColorGamma(Value[1]);
		Value[0] = vgfGetColorGamma(Value[0]);
	}
}

static void _ReadPixel_lRGBA_8888_PRE_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >>  8) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >> 24) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to non-linear color space. */
		Value[2] = vgfGetColorGamma(Value[2]);
		Value[1] = vgfGetColorGamma(Value[1]);
		Value[0] = vgfGetColorGamma(Value[0]);

		/* Premultiply. */
		Value[2] *= Value[3];
		Value[1] *= Value[3];
		Value[0] *= Value[3];
	}
}

static void _ReadPixel_lRGBA_8888_PRE_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >>  8) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >> 24) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];
	}
}

static void _ReadPixel_lRGBA_8888_PRE_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >>  8) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >> 24) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);
	}
}


/******************************************************************************\
************************************ VG_lL_8 ***********************************
\******************************************************************************/

static void _ReadPixel_lL_8_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 lINT;

	/* Get the pixel value. */
	lINT = * Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 1;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] =
	Value[1] =
	Value[0] = vgmZERO2ONE_L2S(lINT, 8);
}


/******************************************************************************\
************************************ VG_A_8 ************************************
\******************************************************************************/

static void _ReadPixel_A_8_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 aINT;

	/* Get the pixel value. */
	aINT = * Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 1;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = 1.0f;
	Value[1] = 1.0f;
	Value[0] = 1.0f;
}

static void _ReadPixel_A_8_To_RGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 aINT;

	/* Get the pixel value. */
	aINT = * Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 1;

	/* Convert to 0..1 range. */
	Value[3] =
	Value[2] =
	Value[1] =
	Value[0] = vgmZERO2ONE(aINT, 8);
}


/******************************************************************************\
************************************ VG_BW_1 ***********************************
\******************************************************************************/

static void _ReadPixel_BW_1_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 lINT;

	/* Get the pixel value. */
	lINT = * Pixel->current;
	lINT >>= Pixel->bitOffset;
	lINT &= 0x01;

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Reset the bit offset. */
		Pixel->bitOffset = 0;

		/* Advance to the next byte position. */
		Pixel->current += 1;
	}
	else
	{
		/* Advance to the next bit position. */
		Pixel->bitOffset += 1;
	}

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] =
	Value[1] =
	Value[0] = (lINT == 0) ? 0.0f : 1.0f;
}

static void _ReadPixel_BW_1_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 lINT;

	/* Get the pixel value. */
	lINT = * Pixel->current;
	lINT >>= Pixel->bitOffset;
	lINT &= 0x01;

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Reset the bit offset. */
		Pixel->bitOffset = 0;

		/* Advance to the next byte position. */
		Pixel->current += 1;
	}
	else
	{
		/* Advance to the next bit position. */
		Pixel->bitOffset += 1;
	}

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] =
	Value[1] =
	Value[0] = (lINT == 0) ? 0.0f : 1.0f;
}


/******************************************************************************\
************************************ VG_A_1 ************************************
\******************************************************************************/

static void _ReadPixel_A_1_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 aINT;

	/* Get the pixel value. */
	aINT = * Pixel->current;
	aINT >>= Pixel->bitOffset;
	aINT &= 0x01;

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Reset the bit offset. */
		Pixel->bitOffset = 0;

		/* Advance to the next byte position. */
		Pixel->current += 1;
	}
	else
	{
		/* Advance to the next bit position. */
		Pixel->bitOffset += 1;
	}

	/* Convert to 0..1 range. */
	Value[3] = (aINT == 0) ? 0.0f : 1.0f;
	Value[2] = 1.0f;
	Value[1] = 1.0f;
	Value[0] = 1.0f;
}

static void _ReadPixel_A_1_To_RGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 aINT;

	/* Get the pixel value. */
	aINT = * Pixel->current;
	aINT >>= Pixel->bitOffset;
	aINT &= 0x01;

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Reset the bit offset. */
		Pixel->bitOffset = 0;

		/* Advance to the next byte position. */
		Pixel->current += 1;
	}
	else
	{
		/* Advance to the next bit position. */
		Pixel->bitOffset += 1;
	}

	/* Convert to 0..1 range. */
	Value[3] =
	Value[2] =
	Value[1] =
	Value[0] = (aINT == 0) ? 0.0f : 1.0f;
}


/******************************************************************************\
************************************ VG_A_4 ************************************
\******************************************************************************/

static void _ReadPixel_A_4_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 aINT;

	/* Get the pixel value. */
	aINT = * Pixel->current;
	aINT >>= Pixel->bitOffset;
	aINT  &=   0x0F;

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 4)
	{
		/* Reset the bit offset. */
		Pixel->bitOffset = 0;

		/* Advance to the next byte position. */
		Pixel->current += 1;
	}
	else
	{
		/* Advance to the next bit position. */
		Pixel->bitOffset += 4;
	}

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = 1.0f;
	Value[1] = 1.0f;
	Value[0] = 1.0f;
}

static void _ReadPixel_A_4_To_RGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT8 aINT;

	/* Get the pixel value. */
	aINT = * Pixel->current;
	aINT >>= Pixel->bitOffset;
	aINT  &=   0x0F;

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 4)
	{
		/* Reset the bit offset. */
		Pixel->bitOffset = 0;

		/* Advance to the next byte position. */
		Pixel->current += 1;
	}
	else
	{
		/* Advance to the next bit position. */
		Pixel->bitOffset += 4;
	}

	/* Convert to 0..1 range. */
	Value[3] =
	Value[2] =
	Value[1] =
	Value[0] = vgmZERO2ONE(aINT, 4);
}


/******************************************************************************\
******************************** VG_lRGB_565_VIV *******************************
\******************************************************************************/

static void _ReadPixel_lRGB_565_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	bINT =  value        & 0x1F;
	gINT = (value >>  5) & 0x3F;
	rINT = (value >> 11) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_L2S(bINT, 5);
	Value[1] = vgmZERO2ONE_L2S(gINT, 6);
	Value[0] = vgmZERO2ONE_L2S(rINT, 5);
}


/******************************************************************************\
********************************* VG_sXRGB_8888 ********************************
\******************************************************************************/

static void _ReadPixel_XRGB_8888_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_sXRGB_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);
}


/******************************************************************************\
********************************* VG_sARGB_8888 ********************************
\******************************************************************************/

static void _ReadPixel_sARGB_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_sARGB_8888_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sARGB_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);
}

static void _ReadPixel_sARGB_8888_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
******************************* VG_sARGB_8888_PRE ******************************
\******************************************************************************/

static void _ReadPixel_sARGB_8888_PRE_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT =  value        & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT = (value >> 16) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];
	}
}

static void _ReadPixel_sARGB_8888_PRE_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT =  value        & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT = (value >> 16) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);
	}
}

static void _ReadPixel_sARGB_8888_PRE_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT =  value        & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT = (value >> 16) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to linear color space. */
		Value[2] = vgfGetColorInverseGamma(Value[2]);
		Value[1] = vgfGetColorInverseGamma(Value[1]);
		Value[0] = vgfGetColorInverseGamma(Value[0]);
	}
}

static void _ReadPixel_sARGB_8888_PRE_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT =  value        & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT = (value >> 16) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to linear color space. */
		Value[2] = vgfGetColorInverseGamma(Value[2]);
		Value[1] = vgfGetColorInverseGamma(Value[1]);
		Value[0] = vgfGetColorInverseGamma(Value[0]);

		/* Premultiply. */
		Value[2] *= Value[3];
		Value[1] *= Value[3];
		Value[0] *= Value[3];
	}
}


/******************************************************************************\
********************************* VG_sARGB_1555 ********************************
\******************************************************************************/

static void _ReadPixel_sARGB_1555_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 15) & 0x01;
	bINT =  value        & 0x1F;
	gINT = (value >>  5) & 0x1F;
	rINT = (value >> 10) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 1);
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 5);
	Value[0] = vgmZERO2ONE(rINT, 5);
}

static void _ReadPixel_sARGB_1555_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 15) & 0x01;
	bINT =  value        & 0x1F;
	gINT = (value >>  5) & 0x1F;
	rINT = (value >> 10) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 1);
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 5);
	Value[0] = vgmZERO2ONE(rINT, 5);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sARGB_1555_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 15) & 0x01;
	bINT =  value        & 0x1F;
	gINT = (value >>  5) & 0x1F;
	rINT = (value >> 10) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 1);
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 5);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);
}

static void _ReadPixel_sARGB_1555_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 15) & 0x01;
	bINT =  value        & 0x1F;
	gINT = (value >>  5) & 0x1F;
	rINT = (value >> 10) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 1);
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 5);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
********************************* VG_sARGB_4444 ********************************
\******************************************************************************/

static void _ReadPixel_sARGB_4444_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 12) & 0x0F;
	bINT =  value        & 0x0F;
	gINT = (value >>  4) & 0x0F;
	rINT = (value >>  8) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = vgmZERO2ONE(bINT, 4);
	Value[1] = vgmZERO2ONE(gINT, 4);
	Value[0] = vgmZERO2ONE(rINT, 4);
}

static void _ReadPixel_sARGB_4444_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 12) & 0x0F;
	bINT =  value        & 0x0F;
	gINT = (value >>  4) & 0x0F;
	rINT = (value >>  8) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = vgmZERO2ONE(bINT, 4);
	Value[1] = vgmZERO2ONE(gINT, 4);
	Value[0] = vgmZERO2ONE(rINT, 4);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sARGB_4444_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 12) & 0x0F;
	bINT =  value        & 0x0F;
	gINT = (value >>  4) & 0x0F;
	rINT = (value >>  8) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 4);
	Value[2] = vgmZERO2ONE_S2L(bINT, 4);
	Value[1] = vgmZERO2ONE_S2L(gINT, 4);
	Value[0] = vgmZERO2ONE_S2L(rINT, 4);
}

static void _ReadPixel_sARGB_4444_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 12) & 0x0F;
	bINT =  value        & 0x0F;
	gINT = (value >>  4) & 0x0F;
	rINT = (value >>  8) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 4);
	Value[2] = vgmZERO2ONE_S2L(bINT, 4);
	Value[1] = vgmZERO2ONE_S2L(gINT, 4);
	Value[0] = vgmZERO2ONE_S2L(rINT, 4);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
********************************* VG_lXRGB_8888 ********************************
\******************************************************************************/

static void _ReadPixel_lXRGB_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);
}


/******************************************************************************\
********************************* VG_lARGB_8888 ********************************
\******************************************************************************/

static void _ReadPixel_lARGB_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);
}

static void _ReadPixel_lARGB_8888_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_lARGB_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_lARGB_8888_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT =  value        & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT = (value >> 16) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
******************************* VG_lARGB_8888_PRE ******************************
\******************************************************************************/

static void _ReadPixel_lARGB_8888_PRE_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT =  value        & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT = (value >> 16) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to non-linear color space. */
		Value[2] = vgfGetColorGamma(Value[2]);
		Value[1] = vgfGetColorGamma(Value[1]);
		Value[0] = vgfGetColorGamma(Value[0]);
	}
}

static void _ReadPixel_lARGB_8888_PRE_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT =  value        & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT = (value >> 16) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to non-linear color space. */
		Value[2] = vgfGetColorGamma(Value[2]);
		Value[1] = vgfGetColorGamma(Value[1]);
		Value[0] = vgfGetColorGamma(Value[0]);

		/* Premultiply. */
		Value[2] *= Value[3];
		Value[1] *= Value[3];
		Value[0] *= Value[3];
	}
}

static void _ReadPixel_lARGB_8888_PRE_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT =  value        & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT = (value >> 16) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];
	}
}

static void _ReadPixel_lARGB_8888_PRE_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT =  value        & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT = (value >> 16) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);
	}
}


/******************************************************************************\
********************************* VG_sBGRX_8888 ********************************
\******************************************************************************/

static void _ReadPixel_BGRX_8888_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_sBGRX_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);
}


/******************************************************************************\
********************************* VG_sBGRA_8888 ********************************
\******************************************************************************/

static void _ReadPixel_sBGRA_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_sBGRA_8888_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sBGRA_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);
}

static void _ReadPixel_sBGRA_8888_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
******************************* VG_sBGRA_8888_PRE ******************************
\******************************************************************************/

static void _ReadPixel_sBGRA_8888_PRE_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 24) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >>  8) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];
	}
}

static void _ReadPixel_sBGRA_8888_PRE_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 24) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >>  8) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);
	}
}

static void _ReadPixel_sBGRA_8888_PRE_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 24) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >>  8) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to linear color space. */
		Value[2] = vgfGetColorInverseGamma(Value[2]);
		Value[1] = vgfGetColorInverseGamma(Value[1]);
		Value[0] = vgfGetColorInverseGamma(Value[0]);
	}
}

static void _ReadPixel_sBGRA_8888_PRE_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 24) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >>  8) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to linear color space. */
		Value[2] = vgfGetColorInverseGamma(Value[2]);
		Value[1] = vgfGetColorInverseGamma(Value[1]);
		Value[0] = vgfGetColorInverseGamma(Value[0]);

		/* Premultiply. */
		Value[2] *= Value[3];
		Value[1] *= Value[3];
		Value[0] *= Value[3];
	}
}


/******************************************************************************\
********************************** VG_sBGR_565 *********************************
\******************************************************************************/

static void _ReadPixel_BGR_565_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	bINT = (value >> 11) & 0x1F;
	gINT = (value >>  5) & 0x3F;
	rINT =  value        & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 6);
	Value[0] = vgmZERO2ONE(rINT, 5);
}

static void _ReadPixel_sBGR_565_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	bINT = (value >> 11) & 0x1F;
	gINT = (value >>  5) & 0x3F;
	rINT =  value        & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 6);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);
}


/******************************************************************************\
********************************* VG_sBGRA_5551 ********************************
\******************************************************************************/

static void _ReadPixel_sBGRA_5551_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x01;
	bINT = (value >> 11) & 0x1F;
	gINT = (value >>  6) & 0x1F;
	rINT = (value >>  1) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 1);
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 5);
	Value[0] = vgmZERO2ONE(rINT, 5);
}

static void _ReadPixel_sBGRA_5551_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x01;
	bINT = (value >> 11) & 0x1F;
	gINT = (value >>  6) & 0x1F;
	rINT = (value >>  1) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 1);
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 5);
	Value[0] = vgmZERO2ONE(rINT, 5);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sBGRA_5551_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x01;
	bINT = (value >> 11) & 0x1F;
	gINT = (value >>  6) & 0x1F;
	rINT = (value >>  1) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 1);
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 5);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);
}

static void _ReadPixel_sBGRA_5551_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x01;
	bINT = (value >> 11) & 0x1F;
	gINT = (value >>  6) & 0x1F;
	rINT = (value >>  1) & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 1);
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 5);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
********************************* VG_sBGRA_4444 ********************************
\******************************************************************************/

static void _ReadPixel_sBGRA_4444_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x0F;
	bINT = (value >> 12) & 0x0F;
	gINT = (value >>  8) & 0x0F;
	rINT = (value >>  4) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = vgmZERO2ONE(bINT, 4);
	Value[1] = vgmZERO2ONE(gINT, 4);
	Value[0] = vgmZERO2ONE(rINT, 4);
}

static void _ReadPixel_sBGRA_4444_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x0F;
	bINT = (value >> 12) & 0x0F;
	gINT = (value >>  8) & 0x0F;
	rINT = (value >>  4) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = vgmZERO2ONE(bINT, 4);
	Value[1] = vgmZERO2ONE(gINT, 4);
	Value[0] = vgmZERO2ONE(rINT, 4);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sBGRA_4444_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x0F;
	bINT = (value >> 12) & 0x0F;
	gINT = (value >>  8) & 0x0F;
	rINT = (value >>  4) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 4);
	Value[2] = vgmZERO2ONE_S2L(bINT, 4);
	Value[1] = vgmZERO2ONE_S2L(gINT, 4);
	Value[0] = vgmZERO2ONE_S2L(rINT, 4);
}

static void _ReadPixel_sBGRA_4444_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT =  value        & 0x0F;
	bINT = (value >> 12) & 0x0F;
	gINT = (value >>  8) & 0x0F;
	rINT = (value >>  4) & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 4);
	Value[2] = vgmZERO2ONE_S2L(bINT, 4);
	Value[1] = vgmZERO2ONE_S2L(gINT, 4);
	Value[0] = vgmZERO2ONE_S2L(rINT, 4);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
********************************* VG_lBGRX_8888 ********************************
\******************************************************************************/

static void _ReadPixel_lBGRX_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);
}


/******************************************************************************\
********************************* VG_lBGRA_8888 ********************************
\******************************************************************************/

static void _ReadPixel_lBGRA_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);
}

static void _ReadPixel_lBGRA_8888_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_lBGRA_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_lBGRA_8888_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT =  value        & 0xFF;
	bINT = (value >> 24) & 0xFF;
	gINT = (value >> 16) & 0xFF;
	rINT = (value >>  8) & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
******************************* VG_lBGRA_8888_PRE ******************************
\******************************************************************************/

static void _ReadPixel_lBGRA_8888_PRE_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 24) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >>  8) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to non-linear color space. */
		Value[2] = vgfGetColorGamma(Value[2]);
		Value[1] = vgfGetColorGamma(Value[1]);
		Value[0] = vgfGetColorGamma(Value[0]);
	}
}

static void _ReadPixel_lBGRA_8888_PRE_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 24) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >>  8) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to non-linear color space. */
		Value[2] = vgfGetColorGamma(Value[2]);
		Value[1] = vgfGetColorGamma(Value[1]);
		Value[0] = vgfGetColorGamma(Value[0]);

		/* Premultiply. */
		Value[2] *= Value[3];
		Value[1] *= Value[3];
		Value[0] *= Value[3];
	}
}

static void _ReadPixel_lBGRA_8888_PRE_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 24) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >>  8) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];
	}
}

static void _ReadPixel_lBGRA_8888_PRE_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = value & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 24) & 0xFF;
		gINT = (value >> 16) & 0xFF;
		rINT = (value >>  8) & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);
	}
}


/******************************************************************************\
********************************* VG_sXBGR_8888 ********************************
\******************************************************************************/

static void _ReadPixel_XBGR_8888_To_RGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_sXBGR_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);
}


/******************************************************************************\
********************************* VG_sABGR_8888 ********************************
\******************************************************************************/

static void _ReadPixel_sABGR_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_sABGR_8888_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sABGR_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);
}

static void _ReadPixel_sABGR_8888_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_S2L(bINT, 8);
	Value[1] = vgmZERO2ONE_S2L(gINT, 8);
	Value[0] = vgmZERO2ONE_S2L(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
******************************* VG_sABGR_8888_PRE ******************************
\******************************************************************************/

static void _ReadPixel_sABGR_8888_PRE_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 16) & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT =  value        & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];
	}
}

static void _ReadPixel_sABGR_8888_PRE_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 16) & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT =  value        & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);
	}
}

static void _ReadPixel_sABGR_8888_PRE_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 16) & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT =  value        & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to linear color space. */
		Value[2] = vgfGetColorInverseGamma(Value[2]);
		Value[1] = vgfGetColorInverseGamma(Value[1]);
		Value[0] = vgfGetColorInverseGamma(Value[0]);
	}
}

static void _ReadPixel_sABGR_8888_PRE_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 16) & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT =  value        & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to linear color space. */
		Value[2] = vgfGetColorInverseGamma(Value[2]);
		Value[1] = vgfGetColorInverseGamma(Value[1]);
		Value[0] = vgfGetColorInverseGamma(Value[0]);

		/* Premultiply. */
		Value[2] *= Value[3];
		Value[1] *= Value[3];
		Value[0] *= Value[3];
	}
}


/******************************************************************************\
********************************* VG_sABGR_1555 ********************************
\******************************************************************************/

static void _ReadPixel_sABGR_1555_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 15) & 0x01;
	bINT = (value >> 10) & 0x1F;
	gINT = (value >>  5) & 0x1F;
	rINT =  value        & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 1);
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 5);
	Value[0] = vgmZERO2ONE(rINT, 5);
}

static void _ReadPixel_sABGR_1555_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 15) & 0x01;
	bINT = (value >> 10) & 0x1F;
	gINT = (value >>  5) & 0x1F;
	rINT =  value        & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 1);
	Value[2] = vgmZERO2ONE(bINT, 5);
	Value[1] = vgmZERO2ONE(gINT, 5);
	Value[0] = vgmZERO2ONE(rINT, 5);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sABGR_1555_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 15) & 0x01;
	bINT = (value >> 10) & 0x1F;
	gINT = (value >>  5) & 0x1F;
	rINT =  value        & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 1);
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 5);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);
}

static void _ReadPixel_sABGR_1555_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 15) & 0x01;
	bINT = (value >> 10) & 0x1F;
	gINT = (value >>  5) & 0x1F;
	rINT =  value        & 0x1F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 1);
	Value[2] = vgmZERO2ONE_S2L(bINT, 5);
	Value[1] = vgmZERO2ONE_S2L(gINT, 5);
	Value[0] = vgmZERO2ONE_S2L(rINT, 5);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
********************************* VG_sABGR_4444 ********************************
\******************************************************************************/

static void _ReadPixel_sABGR_4444_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 12) & 0x0F;
	bINT = (value >>  8) & 0x0F;
	gINT = (value >>  4) & 0x0F;
	rINT =  value        & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = vgmZERO2ONE(bINT, 4);
	Value[1] = vgmZERO2ONE(gINT, 4);
	Value[0] = vgmZERO2ONE(rINT, 4);
}

static void _ReadPixel_sABGR_4444_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 12) & 0x0F;
	bINT = (value >>  8) & 0x0F;
	gINT = (value >>  4) & 0x0F;
	rINT =  value        & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 4);
	Value[2] = vgmZERO2ONE(bINT, 4);
	Value[1] = vgmZERO2ONE(gINT, 4);
	Value[0] = vgmZERO2ONE(rINT, 4);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_sABGR_4444_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 12) & 0x0F;
	bINT = (value >>  8) & 0x0F;
	gINT = (value >>  4) & 0x0F;
	rINT =  value        & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 4);
	Value[2] = vgmZERO2ONE_S2L(bINT, 4);
	Value[1] = vgmZERO2ONE_S2L(gINT, 4);
	Value[0] = vgmZERO2ONE_S2L(rINT, 4);
}

static void _ReadPixel_sABGR_4444_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT16 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 2;

	/* Extract components. */
	aINT = (value >> 12) & 0x0F;
	bINT = (value >>  8) & 0x0F;
	gINT = (value >>  4) & 0x0F;
	rINT =  value        & 0x0F;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 4);
	Value[2] = vgmZERO2ONE_S2L(bINT, 4);
	Value[1] = vgmZERO2ONE_S2L(gINT, 4);
	Value[0] = vgmZERO2ONE_S2L(rINT, 4);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
********************************* VG_lXBGR_8888 ********************************
\******************************************************************************/

static void _ReadPixel_lXBGR_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = 1.0f;
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);
}


/******************************************************************************\
********************************* VG_lABGR_8888 ********************************
\******************************************************************************/

static void _ReadPixel_lABGR_8888_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);
}

static void _ReadPixel_lABGR_8888_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE    (aINT, 8);
	Value[2] = vgmZERO2ONE_L2S(bINT, 8);
	Value[1] = vgmZERO2ONE_L2S(gINT, 8);
	Value[0] = vgmZERO2ONE_L2S(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}

static void _ReadPixel_lABGR_8888_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);
}

static void _ReadPixel_lABGR_8888_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract components. */
	aINT = (value >> 24) & 0xFF;
	bINT = (value >> 16) & 0xFF;
	gINT = (value >>  8) & 0xFF;
	rINT =  value        & 0xFF;

	/* Convert to 0..1 range. */
	Value[3] = vgmZERO2ONE(aINT, 8);
	Value[2] = vgmZERO2ONE(bINT, 8);
	Value[1] = vgmZERO2ONE(gINT, 8);
	Value[0] = vgmZERO2ONE(rINT, 8);

	/* Premultiply. */
	Value[2] *= Value[3];
	Value[1] *= Value[3];
	Value[0] *= Value[3];
}


/******************************************************************************\
******************************* VG_lABGR_8888_PRE ******************************
\******************************************************************************/

static void _ReadPixel_lABGR_8888_PRE_To_sRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 16) & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT =  value        & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to non-linear color space. */
		Value[2] = vgfGetColorGamma(Value[2]);
		Value[1] = vgfGetColorGamma(Value[1]);
		Value[0] = vgfGetColorGamma(Value[0]);
	}
}

static void _ReadPixel_lABGR_8888_PRE_To_sRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 16) & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT =  value        & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];

		/* Convert to non-linear color space. */
		Value[2] = vgfGetColorGamma(Value[2]);
		Value[1] = vgfGetColorGamma(Value[1]);
		Value[0] = vgfGetColorGamma(Value[0]);

		/* Premultiply. */
		Value[2] *= Value[3];
		Value[1] *= Value[3];
		Value[0] *= Value[3];
	}
}

static void _ReadPixel_lABGR_8888_PRE_To_lRGBA(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 16) & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT =  value        & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);

		/* Unpremultiply. */
		Value[2] /= Value[3];
		Value[1] /= Value[3];
		Value[0] /= Value[3];
	}
}

static void _ReadPixel_lABGR_8888_PRE_To_lRGBA_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;

	/* Get the pixel value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Advance to the next pixel. */
	Pixel->current += 4;

	/* Extract the alpha component. */
	aINT = (value >> 24) & 0xFF;

	/* Zero? */
	if (aINT == 0)
	{
		Value[3] =
		Value[2] =
		Value[1] =
		Value[0] = 0.0f;
	}
	else
	{
		/* Extract components. */
		bINT = (value >> 16) & 0xFF;
		gINT = (value >>  8) & 0xFF;
		rINT =  value        & 0xFF;

		/* Clamp colors. */
		bINT = gcmMIN(bINT, aINT);
		gINT = gcmMIN(gINT, aINT);
		rINT = gcmMIN(rINT, aINT);

		/* Convert to 0..1 range. */
		Value[3] = vgmZERO2ONE(aINT, 8);
		Value[2] = vgmZERO2ONE(bINT, 8);
		Value[1] = vgmZERO2ONE(gINT, 8);
		Value[0] = vgmZERO2ONE(rINT, 8);
	}
}
