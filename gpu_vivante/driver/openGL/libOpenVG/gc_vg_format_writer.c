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
********************** Support definitions and functions. **********************
\******************************************************************************/

#define vgmPACKCOMPONENT(Value, ComponentWidth, PixelWidth) \
	(gctUINT ## PixelWidth) _PackComponent( \
		(Value), \
		((1 << ComponentWidth) - 1) \
		)

#define vgmPACKUPSAMPLEDCOMPONENT(Value, ComponentWidth, PixelWidth) \
	(gctUINT ## PixelWidth) _PackUpsampledComponent( \
		(Value), \
		((1 << ComponentWidth) - 1) \
		)

static gctUINT32 _PackComponent(
	gctFLOAT Value,
	gctUINT32 Mask
	)
{
	/* Compute the rounded normalized value. */
	gctFLOAT rounded = Value * Mask + 0.5f;

	/* Get the integer part. */
	gctINT roundedInt = (gctINT) rounded;

	/* Clamp to 0..1 range. */
	gctUINT32 clamped = gcmCLAMP(roundedInt, 0, (gctINT) Mask);

	/* Return result. */
	return clamped;
}

static gctUINT32 _PackUpsampledComponent(
	gctFLOAT Value,
	gctUINT32 Mask
	)
{
	/* Round and truncate the value. */
	gctINT rounded = (gctINT) (Value + 0.5f);

	/* Determine the result. */
	gctUINT32 result = rounded ? Mask : 0;

	/* Return result. */
	return result;
}


/******************************************************************************\
******************************* Upsampled VG_lL_8 ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_To_Upsampled_lL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgB, trgG, trgR;
	gctFLOAT tmpB, tmpG, tmpR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);

	/* Convert to linear color space. */
	tmpB = vgfGetColorInverseGamma(trgB);
	tmpG = vgfGetColorInverseGamma(trgG);
	tmpR = vgfGetColorInverseGamma(trgR);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(tmpR, tmpG, tmpB);

	/* Convert the final pixel value. */
	value = vgmPACKUPSAMPLEDCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_sRGBx_PRE_To_Upsampled_lL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA, trgB, trgG, trgR;
	gctFLOAT tmpB, tmpG, tmpR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);
	trgB = gcmCLAMP(Value[2], 0.0f, trgA);
	trgG = gcmCLAMP(Value[1], 0.0f, trgA);
	trgR = gcmCLAMP(Value[0], 0.0f, trgA);

	/* Unpremultiply. */
	if (trgA == 0.0f)
	{
		trgB = 0.0f;
		trgG = 0.0f;
		trgR = 0.0f;
	}
	else
	{
		trgB /= trgA;
		trgG /= trgA;
		trgR /= trgA;
	}

	/* Convert to linear color space. */
	tmpB = vgfGetColorInverseGamma(trgB);
	tmpG = vgfGetColorInverseGamma(trgG);
	tmpR = vgfGetColorInverseGamma(trgR);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(tmpR, tmpG, tmpB);

	/* Convert the final pixel value. */
	value = vgmPACKUPSAMPLEDCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_lRGBx_To_Upsampled_lL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgB, trgG, trgR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(trgR, trgG, trgB);

	/* Convert the final pixel value. */
	value = vgmPACKUPSAMPLEDCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_lRGBx_PRE_To_Upsampled_lL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA, trgB, trgG, trgR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);
	trgB = gcmCLAMP(Value[2], 0.0f, trgA);
	trgG = gcmCLAMP(Value[1], 0.0f, trgA);
	trgR = gcmCLAMP(Value[0], 0.0f, trgA);

	/* Unpremultiply. */
	if (trgA == 0.0f)
	{
		trgB = 0.0f;
		trgG = 0.0f;
		trgR = 0.0f;
	}
	else
	{
		trgB /= trgA;
		trgG /= trgA;
		trgR /= trgA;
	}

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(trgR, trgG, trgB);

	/* Convert the final pixel value. */
	value = vgmPACKUPSAMPLEDCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}


/******************************************************************************\
******************************* Upsampled VG_A_4 *******************************
\******************************************************************************/

static void _WritePixel_xxxA_To_Upsampled_A_4(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 4)
	{
		/* Compute the pixel value. */
		value   = vgmPACKUPSAMPLEDCOMPONENT(trgA, 4, 8);
		value <<= 4;

		/* Modify the pixel value. */
		* Pixel->current &= ~(0x0F << 4);
		* Pixel->current |= value;

		/* Advance to the next set of pixels. */
		Pixel->current++;

		/* Reset the bit offset. */
		Pixel->bitOffset = 0;
	}
	else
	{
		/* Compute the pixel value. */
		value = vgmPACKUPSAMPLEDCOMPONENT(trgA, 4, 8);

		/* Modify the pixel value. */
		* Pixel->current &= ~0x0F;
		* Pixel->current |= value;

		/* Advance to the next pixel. */
		Pixel->bitOffset += 4;
	}
}


/******************************************************************************\
********************************* VG_sRGBX_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBX_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBX_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_sRGBA_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBA_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgA, 8, 32);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBA_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgA, 8, 32);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
******************************* VG_sRGBA_8888_PRE ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_Masked_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR  *= curA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_Masked_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgA, 8, 32);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	}
	else
	{
		bINT   = (value >> 8) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	}
	else
	{
		gINT   = (value >> 16) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	}
	else
	{
		rINT   = (value >> 24) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBx_PRE_Masked_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBA_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR  *= curA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		}
		else
		{
			bINT   = (value >> 8) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
			trgB  *= trgA;
		}

		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		}
		else
		{
			gINT   = (value >> 16) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
			trgG  *= trgA;
		}

		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		}
		else
		{
			rINT   = (value >> 24) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
			trgR  *= trgA;
		}

		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_Masked_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		trgB  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		trgG  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		trgR  *= curA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgA, 8, 32);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
	}
	else
	{
		bINT   = (value >> 8) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
	}
	else
	{
		gINT   = (value >> 16) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
	}
	else
	{
		rINT   = (value >> 24) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_PRE_Masked_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBA_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			trgB  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			trgG  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			trgR  *= curA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
		}
		else
		{
			bINT   = (value >> 8) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
		}

		trgB  *= trgA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
		}
		else
		{
			gINT   = (value >> 16) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
		}

		trgG  *= trgA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
		}
		else
		{
			rINT   = (value >> 24) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
		}

		trgR  *= trgA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		trgB  *= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		trgG  *= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		trgR  *= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************** VG_sRGB_565 *********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgB, 5, 16);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x07E0;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 5, 16);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGB_565, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgB, 5, 16);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x07E0;
			value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 5, 16);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgB, 5, 16);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x07E0;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 5, 16);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGB_565, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgB, 5, 16);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x07E0;
			value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 5, 16);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_sRGBA_5551 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sRGBA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x0001;
		value |= vgmPACKCOMPONENT(trgA, 1, 16);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x003E;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 1;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x07C0;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sRGBA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 1, 16);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 5, 16) << 1;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sRGBA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBA_5551, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x0001;
			value |= vgmPACKCOMPONENT(trgA, 1, 16);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x003E;
			value |= vgmPACKCOMPONENT(trgB, 5, 16) << 1;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x07C0;
			value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sRGBA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 1, 16);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 1;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sRGBA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x0001;
		value |= vgmPACKCOMPONENT(trgA, 1, 16);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x003E;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 1;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x07C0;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sRGBA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 1, 16);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 5, 16) << 1;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sRGBA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBA_5551, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x0001;
			value |= vgmPACKCOMPONENT(trgA, 1, 16);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x003E;
			value |= vgmPACKCOMPONENT(trgB, 5, 16) << 1;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x07C0;
			value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sRGBA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 1, 16);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 1;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_sRGBA_4444 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sRGBA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000F;
		value |= vgmPACKCOMPONENT(trgA, 4, 16);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x00F0;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 4;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0F00;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0xF000;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 12;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sRGBA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 4, 16);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 4, 16) << 4;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 4, 16) << 12;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sRGBA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBA_4444, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000F;
			value |= vgmPACKCOMPONENT(trgA, 4, 16);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x00F0;
			value |= vgmPACKCOMPONENT(trgB, 4, 16) << 4;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0F00;
			value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0xF000;
			value |= vgmPACKCOMPONENT(trgR, 4, 16) << 12;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sRGBA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 4, 16);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 4;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 12;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sRGBA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000F;
		value |= vgmPACKCOMPONENT(trgA, 4, 16);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x00F0;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 4;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x0F00;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0xF000;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 12;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sRGBA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 4, 16);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 4, 16) << 4;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 4, 16) << 12;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sRGBA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sRGBA_4444, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000F;
			value |= vgmPACKCOMPONENT(trgA, 4, 16);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x00F0;
			value |= vgmPACKCOMPONENT(trgB, 4, 16) << 4;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x0F00;
			value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0xF000;
			value |= vgmPACKCOMPONENT(trgR, 4, 16) << 12;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sRGBA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 4, 16);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 4;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 12;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
************************************ VG_sL_8 ***********************************
\******************************************************************************/

static void _WritePixel_sRGBx_To_sL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgB, trgG, trgR;
	gctFLOAT tmpB, tmpG, tmpR;
	gctFLOAT trgL, grayScale;

	/* Clamp the incoming color. */
	trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);

	/* Convert to linear color space. */
	tmpB = vgfGetColorInverseGamma(trgB);
	tmpG = vgfGetColorInverseGamma(trgG);
	tmpR = vgfGetColorInverseGamma(trgR);

	/* Compute the grayscale value. */
	grayScale = vgfGetGrayScale(tmpR, tmpG, tmpB);

	/* Convert to non-linear color space. */
	trgL = vgfGetColorGamma(grayScale);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_sRGBx_PRE_To_sL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA, trgB, trgG, trgR;
	gctFLOAT tmpB, tmpG, tmpR;
	gctFLOAT trgL, grayScale;

	/* Clamp the incoming color. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);
	trgB = gcmCLAMP(Value[2], 0.0f, trgA);
	trgG = gcmCLAMP(Value[1], 0.0f, trgA);
	trgR = gcmCLAMP(Value[0], 0.0f, trgA);

	/* Unpremultiply. */
	if (trgA == 0.0f)
	{
		trgB = 0.0f;
		trgG = 0.0f;
		trgR = 0.0f;
	}
	else
	{
		trgB /= trgA;
		trgG /= trgA;
		trgR /= trgA;
	}

	/* Convert to linear color space. */
	tmpB = vgfGetColorInverseGamma(trgB);
	tmpG = vgfGetColorInverseGamma(trgG);
	tmpR = vgfGetColorInverseGamma(trgR);

	/* Compute the grayscale value. */
	grayScale = vgfGetGrayScale(tmpR, tmpG, tmpB);

	/* Convert to non-linear color space. */
	trgL = vgfGetColorGamma(grayScale);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_lRGBx_To_sL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgB, trgG, trgR;
	gctFLOAT trgL, grayScale;

	/* Clamp the incoming color. */
	trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);

	/* Compute the grayscale value. */
	grayScale = vgfGetGrayScale(trgR, trgG, trgB);

	/* Convert to non-linear color space. */
	trgL = vgfGetColorGamma(grayScale);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_lRGBx_PRE_To_sL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA, trgB, trgG, trgR;
	gctFLOAT trgL, grayScale;

	/* Clamp the incoming color. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);
	trgB = gcmCLAMP(Value[2], 0.0f, trgA);
	trgG = gcmCLAMP(Value[1], 0.0f, trgA);
	trgR = gcmCLAMP(Value[0], 0.0f, trgA);

	/* Unpremultiply. */
	if (trgA == 0.0f)
	{
		trgB = 0.0f;
		trgG = 0.0f;
		trgR = 0.0f;
	}
	else
	{
		trgB /= trgA;
		trgG /= trgA;
		trgR /= trgA;
	}

	/* Compute the grayscale value. */
	grayScale = vgfGetGrayScale(trgR, trgG, trgB);

	/* Convert to non-linear color space. */
	trgL = vgfGetColorGamma(grayScale);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}


/******************************************************************************\
********************************* VG_lRGBX_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lRGBX_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lRGBX_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lRGBX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_lRGBA_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lRGBA_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgA, 8, 32);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lRGBA_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgA, 8, 32);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lRGBA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
******************************* VG_lRGBA_8888_PRE ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_Masked_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		trgB  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		trgG  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		trgR  *= curA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_Masked_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgA, 8, 32);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
	}
	else
	{
		bINT   = (value >> 8) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
	}
	else
	{
		gINT   = (value >> 16) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
	}
	else
	{
		rINT   = (value >> 24) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBx_PRE_Masked_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lRGBA_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			trgB  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			trgG  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			trgR  *= curA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
		}
		else
		{
			bINT   = (value >> 8) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
		}

		trgB  *= trgA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
		}
		else
		{
			gINT   = (value >> 16) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
		}

		trgG  *= trgA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
		}
		else
		{
			rINT   = (value >> 24) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
		}

		trgR  *= trgA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		trgB  *= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		trgG  *= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		trgR  *= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_Masked_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR  *= curA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgA, 8, 32);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	}
	else
	{
		bINT   = (value >> 8) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	}
	else
	{
		gINT   = (value >> 16) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	}
	else
	{
		rINT   = (value >> 24) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_PRE_Masked_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lRGBA_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR  *= curA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		}
		else
		{
			bINT   = (value >> 8) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
			trgB  *= trgA;
		}

		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		}
		else
		{
			gINT   = (value >> 16) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
			trgG  *= trgA;
		}

		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		}
		else
		{
			rINT   = (value >> 24) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
			trgR  *= trgA;
		}

		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lRGBA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 24;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
************************************ VG_lL_8 ***********************************
\******************************************************************************/

static void _WritePixel_sRGBx_To_lL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgB, trgG, trgR;
	gctFLOAT tmpB, tmpG, tmpR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);

	/* Convert to linear color space. */
	tmpB = vgfGetColorInverseGamma(trgB);
	tmpG = vgfGetColorInverseGamma(trgG);
	tmpR = vgfGetColorInverseGamma(trgR);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(tmpR, tmpG, tmpB);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_sRGBx_PRE_To_lL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA, trgB, trgG, trgR;
	gctFLOAT tmpB, tmpG, tmpR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);
	trgB = gcmCLAMP(Value[2], 0.0f, trgA);
	trgG = gcmCLAMP(Value[1], 0.0f, trgA);
	trgR = gcmCLAMP(Value[0], 0.0f, trgA);

	/* Unpremultiply. */
	if (trgA == 0.0f)
	{
		trgB = 0.0f;
		trgG = 0.0f;
		trgR = 0.0f;
	}
	else
	{
		trgB /= trgA;
		trgG /= trgA;
		trgR /= trgA;
	}

	/* Convert to linear color space. */
	tmpB = vgfGetColorInverseGamma(trgB);
	tmpG = vgfGetColorInverseGamma(trgG);
	tmpR = vgfGetColorInverseGamma(trgR);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(tmpR, tmpG, tmpB);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_lRGBx_To_lL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgB, trgG, trgR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(trgR, trgG, trgB);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}

static void _WritePixel_lRGBx_PRE_To_lL_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA, trgB, trgG, trgR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);
	trgB = gcmCLAMP(Value[2], 0.0f, trgA);
	trgG = gcmCLAMP(Value[1], 0.0f, trgA);
	trgR = gcmCLAMP(Value[0], 0.0f, trgA);

	/* Unpremultiply. */
	if (trgA == 0.0f)
	{
		trgB = 0.0f;
		trgG = 0.0f;
		trgR = 0.0f;
	}
	else
	{
		trgB /= trgA;
		trgG /= trgA;
		trgR /= trgA;
	}

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(trgR, trgG, trgB);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgL, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}


/******************************************************************************\
************************************ VG_A_8 ************************************
\******************************************************************************/

static void _WritePixel_xxxA_To_A_8(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the final pixel value. */
	value = vgmPACKCOMPONENT(trgA, 8, 8);

	/* Write the pixel value. */
	* (gctUINT8_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 1;
}


/******************************************************************************\
************************************ VG_BW_1 ***********************************
\******************************************************************************/

static void _WritePixel_sRGBx_To_BW_1(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	register gctUINT8 mask;
	gctFLOAT trgB, trgG, trgR;
	gctFLOAT tmpB, tmpG, tmpR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);

	/* Convert to linear color space. */
	tmpB = vgfGetColorInverseGamma(trgB);
	tmpG = vgfGetColorInverseGamma(trgG);
	tmpR = vgfGetColorInverseGamma(trgR);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(tmpR, tmpG, tmpB);

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgL, 1, 8);
		value <<= 7;

		/* Modify the pixel value. */
		* Pixel->current &= ~(0x01 << 7);
		* Pixel->current |= value;

		/* Advance to the next set of pixels. */
		Pixel->current++;

		/* Reset the bit offset. */
		Pixel->bitOffset = 0;
	}
	else
	{
		/* Compute the pixel mask. */
		mask   = 0x01;
		mask <<= Pixel->bitOffset;
		mask   = ~mask;

		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgL, 1, 8);
		value <<= Pixel->bitOffset;

		/* Modify the pixel value. */
		* Pixel->current &= mask;
		* Pixel->current |= value;

		/* Advance to the next pixel. */
		Pixel->bitOffset += 1;
	}
}

static void _WritePixel_sRGBx_PRE_To_BW_1(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	register gctUINT8 mask;
	gctFLOAT trgA, trgB, trgG, trgR;
	gctFLOAT tmpB, tmpG, tmpR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);
	trgB = gcmCLAMP(Value[2], 0.0f, trgA);
	trgG = gcmCLAMP(Value[1], 0.0f, trgA);
	trgR = gcmCLAMP(Value[0], 0.0f, trgA);

	/* Unpremultiply. */
	if (trgA == 0.0f)
	{
		trgB = 0.0f;
		trgG = 0.0f;
		trgR = 0.0f;
	}
	else
	{
		trgB /= trgA;
		trgG /= trgA;
		trgR /= trgA;
	}

	/* Convert to linear color space. */
	tmpB = vgfGetColorInverseGamma(trgB);
	tmpG = vgfGetColorInverseGamma(trgG);
	tmpR = vgfGetColorInverseGamma(trgR);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(tmpR, tmpG, tmpB);

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgL, 1, 8);
		value <<= 7;

		/* Modify the pixel value. */
		* Pixel->current &= ~(0x01 << 7);
		* Pixel->current |= value;

		/* Advance to the next set of pixels. */
		Pixel->current++;

		/* Reset the bit offset. */
		Pixel->bitOffset = 0;
	}
	else
	{
		/* Compute the pixel mask. */
		mask   = 0x01;
		mask <<= Pixel->bitOffset;
		mask   = ~mask;

		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgL, 1, 8);
		value <<= Pixel->bitOffset;

		/* Modify the pixel value. */
		* Pixel->current &= mask;
		* Pixel->current |= value;

		/* Advance to the next pixel. */
		Pixel->bitOffset += 1;
	}
}

static void _WritePixel_lRGBx_To_BW_1(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	register gctUINT8 mask;
	gctFLOAT trgB, trgG, trgR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgB = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgG = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgR = gcmCLAMP(Value[0], 0.0f, 1.0f);

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(trgR, trgG, trgB);

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgL, 1, 8);
		value <<= 7;

		/* Modify the pixel value. */
		* Pixel->current &= ~(0x01 << 7);
		* Pixel->current |= value;

		/* Advance to the next set of pixels. */
		Pixel->current++;

		/* Reset the bit offset. */
		Pixel->bitOffset = 0;
	}
	else
	{
		/* Compute the pixel mask. */
		mask   = 0x01;
		mask <<= Pixel->bitOffset;
		mask   = ~mask;

		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgL, 1, 8);
		value <<= Pixel->bitOffset;

		/* Modify the pixel value. */
		* Pixel->current &= mask;
		* Pixel->current |= value;

		/* Advance to the next pixel. */
		Pixel->bitOffset += 1;
	}
}

static void _WritePixel_lRGBx_PRE_To_BW_1(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	register gctUINT8 mask;
	gctFLOAT trgA, trgB, trgG, trgR;
	gctFLOAT trgL;

	/* Clamp the incoming color. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);
	trgB = gcmCLAMP(Value[2], 0.0f, trgA);
	trgG = gcmCLAMP(Value[1], 0.0f, trgA);
	trgR = gcmCLAMP(Value[0], 0.0f, trgA);

	/* Unpremultiply. */
	if (trgA == 0.0f)
	{
		trgB = 0.0f;
		trgG = 0.0f;
		trgR = 0.0f;
	}
	else
	{
		trgB /= trgA;
		trgG /= trgA;
		trgR /= trgA;
	}

	/* Compute the grayscale value. */
	trgL = vgfGetGrayScale(trgR, trgG, trgB);

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgL, 1, 8);
		value <<= 7;

		/* Modify the pixel value. */
		* Pixel->current &= ~(0x01 << 7);
		* Pixel->current |= value;

		/* Advance to the next set of pixels. */
		Pixel->current++;

		/* Reset the bit offset. */
		Pixel->bitOffset = 0;
	}
	else
	{
		/* Compute the pixel mask. */
		mask   = 0x01;
		mask <<= Pixel->bitOffset;
		mask   = ~mask;

		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgL, 1, 8);
		value <<= Pixel->bitOffset;

		/* Modify the pixel value. */
		* Pixel->current &= mask;
		* Pixel->current |= value;

		/* Advance to the next pixel. */
		Pixel->bitOffset += 1;
	}
}


/******************************************************************************\
************************************ VG_A_1 ************************************
\******************************************************************************/

static void _WritePixel_xxxA_To_A_1(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	register gctUINT8 mask;
	gctFLOAT trgA;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 7)
	{
		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgA, 1, 8);
		value <<= 7;

		/* Modify the pixel value. */
		* Pixel->current &= ~(0x01 << 7);
		* Pixel->current |= value;

		/* Advance to the next set of pixels. */
		Pixel->current++;

		/* Reset the bit offset. */
		Pixel->bitOffset = 0;
	}
	else
	{
		/* Compute the pixel mask. */
		mask   = 0x01;
		mask <<= Pixel->bitOffset;
		mask   = ~mask;

		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgA, 1, 8);
		value <<= Pixel->bitOffset;

		/* Modify the pixel value. */
		* Pixel->current &= mask;
		* Pixel->current |= value;

		/* Advance to the next pixel. */
		Pixel->bitOffset += 1;
	}
}


/******************************************************************************\
************************************ VG_A_4 ************************************
\******************************************************************************/

static void _WritePixel_xxxA_To_A_4(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT8 value;
	gctFLOAT trgA;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Last pixel within the byte? */
	if (Pixel->bitOffset == 4)
	{
		/* Compute the pixel value. */
		value   = vgmPACKCOMPONENT(trgA, 4, 8);
		value <<= 4;

		/* Modify the pixel value. */
		* Pixel->current &= ~(0x0F << 4);
		* Pixel->current |= value;

		/* Advance to the next set of pixels. */
		Pixel->current++;

		/* Reset the bit offset. */
		Pixel->bitOffset = 0;
	}
	else
	{
		/* Compute the pixel value. */
		value = vgmPACKCOMPONENT(trgA, 4, 8);

		/* Modify the pixel value. */
		* Pixel->current &= ~0x0F;
		* Pixel->current |= value;

		/* Advance to the next pixel. */
		Pixel->bitOffset += 4;
	}
}


/******************************************************************************\
******************************** VG_lRGB_565_VIV *******************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgB, 5, 16);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x07E0;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_lRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 5, 16);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lRGB_565, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgB, 5, 16);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x07E0;
			value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_lRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 5, 16);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_lRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgB, 5, 16);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x07E0;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_lRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 5, 16);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lRGB_565, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgB, 5, 16);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x07E0;
			value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_lRGB_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 5, 16);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 11;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_sXRGB_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sXRGB_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sXRGB_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_sARGB_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sARGB_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sARGB_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
******************************* VG_sARGB_8888_PRE ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_Masked_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB  *= curA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_Masked_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	}
	else
	{
		bINT   = value & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	}
	else
	{
		gINT   = (value >> 8) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	}
	else
	{
		rINT   = (value >> 16) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBx_PRE_Masked_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sARGB_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB  *= curA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		}
		else
		{
			bINT   = value & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
			trgB  *= trgA;
		}

		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		}
		else
		{
			gINT   = (value >> 8) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
			trgG  *= trgA;
		}

		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		}
		else
		{
			rINT   = (value >> 16) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
			trgR  *= trgA;
		}

		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_Masked_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		trgB  *= curA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		trgG  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		trgR  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
	}
	else
	{
		bINT   = value & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
	}
	else
	{
		gINT   = (value >> 8) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
	}
	else
	{
		rINT   = (value >> 16) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_PRE_Masked_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sARGB_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			trgB  *= curA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			trgG  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			trgR  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
		}
		else
		{
			bINT   = value & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
		}

		trgB  *= trgA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
		}
		else
		{
			gINT   = (value >> 8) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
		}

		trgG  *= trgA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
		}
		else
		{
			rINT   = (value >> 16) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
		}

		trgR  *= trgA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		trgB  *= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		trgG  *= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		trgR  *= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_sARGB_1555 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sARGB_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x8000;
		value |= vgmPACKCOMPONENT(trgA, 1, 16) << 15;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgB, 5, 16);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x03E0;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x7C00;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 10;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sARGB_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 1, 16) << 15;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 5, 16);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 10;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sARGB_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sARGB_1555, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x8000;
			value |= vgmPACKCOMPONENT(trgA, 1, 16) << 15;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgB, 5, 16);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x03E0;
			value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x7C00;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 10;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sARGB_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 1, 16) << 15;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 5, 16);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 10;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sARGB_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x8000;
		value |= vgmPACKCOMPONENT(trgA, 1, 16) << 15;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgB, 5, 16);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x03E0;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x7C00;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 10;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sARGB_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 1, 16) << 15;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 5, 16);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 10;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sARGB_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sARGB_1555, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x8000;
			value |= vgmPACKCOMPONENT(trgA, 1, 16) << 15;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgB, 5, 16);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x03E0;
			value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x7C00;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 10;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sARGB_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 1, 16) << 15;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 5, 16);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 10;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_sARGB_4444 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sARGB_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xF000;
		value |= vgmPACKCOMPONENT(trgA, 4, 16) << 12;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x000F;
		value |= vgmPACKCOMPONENT(trgB, 4, 16);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00F0;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x0F00;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sARGB_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 4, 16) << 12;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 4, 16);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 4, 16) << 8;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sARGB_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sARGB_4444, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xF000;
			value |= vgmPACKCOMPONENT(trgA, 4, 16) << 12;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x000F;
			value |= vgmPACKCOMPONENT(trgB, 4, 16);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00F0;
			value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x0F00;
			value |= vgmPACKCOMPONENT(trgR, 4, 16) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sARGB_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 4, 16) << 12;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 4, 16);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sARGB_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xF000;
		value |= vgmPACKCOMPONENT(trgA, 4, 16) << 12;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x000F;
		value |= vgmPACKCOMPONENT(trgB, 4, 16);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x00F0;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x0F00;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sARGB_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 4, 16) << 12;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 4, 16);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 4, 16) << 8;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sARGB_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sARGB_4444, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xF000;
			value |= vgmPACKCOMPONENT(trgA, 4, 16) << 12;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x000F;
			value |= vgmPACKCOMPONENT(trgB, 4, 16);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x00F0;
			value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x0F00;
			value |= vgmPACKCOMPONENT(trgR, 4, 16) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sARGB_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 4, 16) << 12;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 4, 16);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_lXRGB_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lXRGB_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lXRGB_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lXRGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_lARGB_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lARGB_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lARGB_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lARGB_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
******************************* VG_lARGB_8888_PRE ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_Masked_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		trgB  *= curA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		trgG  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		trgR  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_Masked_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
	}
	else
	{
		bINT   = value & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
	}
	else
	{
		gINT   = (value >> 8) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
	}
	else
	{
		rINT   = (value >> 16) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBx_PRE_Masked_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lARGB_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			trgB  *= curA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			trgG  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			trgR  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
		}
		else
		{
			bINT   = value & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
		}

		trgB  *= trgA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
		}
		else
		{
			gINT   = (value >> 8) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
		}

		trgG  *= trgA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
		}
		else
		{
			rINT   = (value >> 16) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
		}

		trgR  *= trgA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		trgB  *= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		trgG  *= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		trgR  *= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_Masked_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB  *= curA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	}
	else
	{
		bINT   = value & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	}
	else
	{
		gINT   = (value >> 8) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	}
	else
	{
		rINT   = (value >> 16) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32);

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_PRE_Masked_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lARGB_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB  *= curA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgB, 8, 32);
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		}
		else
		{
			bINT   = value & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
			trgB  *= trgA;
		}

		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		}
		else
		{
			gINT   = (value >> 8) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
			trgG  *= trgA;
		}

		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		}
		else
		{
			rINT   = (value >> 16) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
			trgR  *= trgA;
		}

		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lARGB_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgB, 8, 32);

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 16;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_sBGRX_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRX_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRX_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_sBGRA_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRA_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgA, 8, 32);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRA_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgA, 8, 32);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
******************************* VG_sBGRA_8888_PRE ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_Masked_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB  *= curA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_Masked_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgA, 8, 32);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	}
	else
	{
		bINT   = (value >> 24) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	}
	else
	{
		gINT   = (value >> 16) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	}
	else
	{
		rINT   = (value >> 8) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBx_PRE_Masked_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRA_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB  *= curA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		}
		else
		{
			bINT   = (value >> 24) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
			trgB  *= trgA;
		}

		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		}
		else
		{
			gINT   = (value >> 16) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
			trgG  *= trgA;
		}

		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		}
		else
		{
			rINT   = (value >> 8) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
			trgR  *= trgA;
		}

		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_Masked_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		trgB  *= curA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		trgG  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		trgR  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgA, 8, 32);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
	}
	else
	{
		bINT   = (value >> 24) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
	}
	else
	{
		gINT   = (value >> 16) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
	}
	else
	{
		rINT   = (value >> 8) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_PRE_Masked_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRA_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			trgB  *= curA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			trgG  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			trgR  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
		}
		else
		{
			bINT   = (value >> 24) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
		}

		trgB  *= trgA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
		}
		else
		{
			gINT   = (value >> 16) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
		}

		trgG  *= trgA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
		}
		else
		{
			rINT   = (value >> 8) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
		}

		trgR  *= trgA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		trgB  *= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		trgG  *= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		trgR  *= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************** VG_sBGR_565 *********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sBGR_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x07E0;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgR, 5, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sBGR_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 5, 16) << 11;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 5, 16);

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sBGR_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGR_565, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x07E0;
			value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgR, 5, 16);
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sBGR_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 5, 16) << 11;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 5, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sBGR_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x07E0;
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgR, 5, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sBGR_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 5, 16) << 11;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 5, 16);

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sBGR_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGR_565, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x07E0;
			value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgR, 5, 16);
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sBGR_565(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 5, 16) << 11;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 6, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 5, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_sBGRA_5551 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sBGRA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x0001;
		value |= vgmPACKCOMPONENT(trgA, 1, 16);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x07C0;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x003E;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 1;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sBGRA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 1, 16);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 1;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sBGRA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRA_5551, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x0001;
			value |= vgmPACKCOMPONENT(trgA, 1, 16);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x07C0;
			value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x003E;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 1;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sBGRA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 1, 16);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 1;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sBGRA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x0001;
		value |= vgmPACKCOMPONENT(trgA, 1, 16);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0xF800;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x07C0;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x003E;
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 1;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sBGRA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 1, 16);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 5, 16) << 1;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sBGRA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRA_5551, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x0001;
			value |= vgmPACKCOMPONENT(trgA, 1, 16);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0xF800;
			value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x07C0;
			value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x003E;
			value |= vgmPACKCOMPONENT(trgR, 5, 16) << 1;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sBGRA_5551(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 1, 16);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 11;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 6;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 5, 16) << 1;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_sBGRA_4444 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sBGRA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000F;
		value |= vgmPACKCOMPONENT(trgA, 4, 16);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0xF000;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 12;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0F00;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x00F0;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 4;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sBGRA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 4, 16);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 4, 16) << 12;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 4, 16) << 4;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sBGRA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRA_4444, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000F;
			value |= vgmPACKCOMPONENT(trgA, 4, 16);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0xF000;
			value |= vgmPACKCOMPONENT(trgB, 4, 16) << 12;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0F00;
			value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x00F0;
			value |= vgmPACKCOMPONENT(trgR, 4, 16) << 4;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sBGRA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 4, 16);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 12;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 4;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sBGRA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000F;
		value |= vgmPACKCOMPONENT(trgA, 4, 16);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0xF000;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 12;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x0F00;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x00F0;
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 4;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sBGRA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 4, 16);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 4, 16) << 12;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 4, 16) << 4;

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sBGRA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sBGRA_4444, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000F;
			value |= vgmPACKCOMPONENT(trgA, 4, 16);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0xF000;
			value |= vgmPACKCOMPONENT(trgB, 4, 16) << 12;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x0F00;
			value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x00F0;
			value |= vgmPACKCOMPONENT(trgR, 4, 16) << 4;
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sBGRA_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 4, 16);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 12;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 4, 16) << 4;
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_lBGRX_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lBGRX_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lBGRX_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lBGRX_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_lBGRA_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lBGRA_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgA, 8, 32);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lBGRA_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgA, 8, 32);
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lBGRA_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
******************************* VG_lBGRA_8888_PRE ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_Masked_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		trgB  *= curA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		trgG  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		trgR  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_Masked_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgA, 8, 32);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
	}
	else
	{
		bINT   = (value >> 24) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
	}
	else
	{
		gINT   = (value >> 16) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
	}
	else
	{
		rINT   = (value >> 8) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBx_PRE_Masked_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lBGRA_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			trgB  *= curA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			trgG  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			trgR  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
		}
		else
		{
			bINT   = (value >> 24) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
		}

		trgB  *= trgA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
		}
		else
		{
			gINT   = (value >> 16) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
		}

		trgG  *= trgA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
		}
		else
		{
			rINT   = (value >> 8) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
		}

		trgR  *= trgA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		trgB  *= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		trgG  *= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		trgR  *= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_Masked_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB  *= curA;
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgA, 8, 32);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	}
	else
	{
		bINT   = (value >> 24) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	}
	else
	{
		gINT   = (value >> 16) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	}
	else
	{
		rINT   = (value >> 8) & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32);

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_PRE_Masked_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lBGRA_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB  *= curA;
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = value & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgA, 8, 32);

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		}
		else
		{
			bINT   = (value >> 24) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
			trgB  *= trgA;
		}

		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		}
		else
		{
			gINT   = (value >> 16) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
			trgG  *= trgA;
		}

		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		}
		else
		{
			rINT   = (value >> 8) & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
			trgR  *= trgA;
		}

		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lBGRA_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32);

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 24;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 16;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgR, 8, 32) << 8;
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_sXBGR_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sXBGR_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sXBGR_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_sABGR_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sABGR_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sABGR_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
******************************* VG_sABGR_8888_PRE ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_Masked_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR  *= curA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_Masked_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	}
	else
	{
		bINT   = (value >> 16) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	}
	else
	{
		gINT   = (value >> 8) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	}
	else
	{
		rINT   = value & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBx_PRE_Masked_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sABGR_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR  *= curA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		}
		else
		{
			bINT   = (value >> 16) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
			trgB  *= trgA;
		}

		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		}
		else
		{
			gINT   = (value >> 8) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
			trgG  *= trgA;
		}

		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		}
		else
		{
			rINT   = value & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
			trgR  *= trgA;
		}

		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_Masked_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		trgB  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		trgG  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		trgR  *= curA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
	}
	else
	{
		bINT   = (value >> 16) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
	}
	else
	{
		gINT   = (value >> 8) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
	}
	else
	{
		rINT   = value & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_PRE_Masked_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sABGR_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			trgB  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			trgG  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			trgR  *= curA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
		}
		else
		{
			bINT   = (value >> 16) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
		}

		trgB  *= trgA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
		}
		else
		{
			gINT   = (value >> 8) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
		}

		trgG  *= trgA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
		}
		else
		{
			rINT   = value & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
		}

		trgR  *= trgA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_sABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		trgB  *= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		trgG  *= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		trgR  *= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_sABGR_1555 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sABGR_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x8000;
		value |= vgmPACKCOMPONENT(trgA, 1, 16) << 15;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x7C00;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 10;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x03E0;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgR, 5, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sABGR_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 1, 16) << 15;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 5, 16) << 10;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 5, 16);

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sABGR_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sABGR_1555, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x8000;
			value |= vgmPACKCOMPONENT(trgA, 1, 16) << 15;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x7C00;
			value |= vgmPACKCOMPONENT(trgB, 5, 16) << 10;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x03E0;
			value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgR, 5, 16);
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sABGR_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 1, 16) << 15;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 10;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 5, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sABGR_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0x8000;
		value |= vgmPACKCOMPONENT(trgA, 1, 16) << 15;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x7C00;
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 10;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x03E0;
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x001F;
		value |= vgmPACKCOMPONENT(trgR, 5, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sABGR_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 1, 16) << 15;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 5, 16) << 10;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 5, 16);

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sABGR_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sABGR_1555, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0x8000;
			value |= vgmPACKCOMPONENT(trgA, 1, 16) << 15;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x7C00;
			value |= vgmPACKCOMPONENT(trgB, 5, 16) << 10;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x03E0;
			value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x001F;
			value |= vgmPACKCOMPONENT(trgR, 5, 16);
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sABGR_1555(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 1, 16) << 15;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 5, 16) << 10;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 5, 16) << 5;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 5, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_sABGR_4444 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_sABGR_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xF000;
		value |= vgmPACKCOMPONENT(trgA, 4, 16) << 12;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x0F00;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x00F0;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x000F;
		value |= vgmPACKCOMPONENT(trgR, 4, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_To_sABGR_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 4, 16) << 12;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 4, 16) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 4, 16);

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_Masked_To_sABGR_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sABGR_4444, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xF000;
			value |= vgmPACKCOMPONENT(trgA, 4, 16) << 12;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x0F00;
			value |= vgmPACKCOMPONENT(trgB, 4, 16) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x00F0;
			value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x000F;
			value |= vgmPACKCOMPONENT(trgR, 4, 16);
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_sRGBA_PRE_To_sABGR_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 4, 16) << 12;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 4, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_Masked_To_sABGR_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xF000;
		value |= vgmPACKCOMPONENT(trgA, 4, 16) << 12;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorGamma(trgB);
		value &= ~0x0F00;
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 8;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorGamma(trgG);
		value &= ~0x00F0;
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorGamma(trgR);
		value &= ~0x000F;
		value |= vgmPACKCOMPONENT(trgR, 4, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_To_sABGR_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 4, 16) << 12;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 4, 16) << 8;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 4, 16);

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_Masked_To_sABGR_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT16_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(sABGR_4444, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xF000;
			value |= vgmPACKCOMPONENT(trgA, 4, 16) << 12;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorGamma(trgB);
			value &= ~0x0F00;
			value |= vgmPACKCOMPONENT(trgB, 4, 16) << 8;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorGamma(trgG);
			value &= ~0x00F0;
			value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorGamma(trgR);
			value &= ~0x000F;
			value |= vgmPACKCOMPONENT(trgR, 4, 16);
		}
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}

static void _WritePixel_lRGBA_PRE_To_sABGR_4444(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT16 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 4, 16) << 12;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 4, 16) << 8;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 4, 16) << 4;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 4, 16);
	}

	/* Write the pixel value. */
	* (gctUINT16_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 2;
}


/******************************************************************************\
********************************* VG_lXBGR_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lXBGR_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgB, trgG, trgR;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lXBGR_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lXBGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value  = vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
********************************* VG_lABGR_8888 ********************************
\******************************************************************************/

static void _WritePixel_sRGBA_Masked_To_lABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lABGR_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
	{
		trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
	}

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lABGR_8888, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_ALPHA) == VG_ALPHA)
		{
			value &= ~0xFF000000;
			value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;
		}

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lABGR_8888(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}


/******************************************************************************\
******************************* VG_lABGR_8888_PRE ******************************
\******************************************************************************/

static void _WritePixel_sRGBx_Masked_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
		trgB  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
		trgG  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
		trgR  *= curA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_Masked_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB   = vgfGetColorInverseGamma(trgB);
	}
	else
	{
		bINT   = (value >> 16) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG   = vgfGetColorInverseGamma(trgG);
	}
	else
	{
		gINT   = (value >> 8) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR   = vgfGetColorInverseGamma(trgR);
	}
	else
	{
		rINT   = value & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB   = vgfGetColorInverseGamma(trgB);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG   = vgfGetColorInverseGamma(trgG);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR   = vgfGetColorInverseGamma(trgR);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBx_PRE_Masked_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lABGR_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
			trgB  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
			trgG  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
			trgR  *= curA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_Masked_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB   = vgfGetColorInverseGamma(trgB);
		}
		else
		{
			bINT   = (value >> 16) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
		}

		trgB  *= trgA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG   = vgfGetColorInverseGamma(trgG);
		}
		else
		{
			gINT   = (value >> 8) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
		}

		trgG  *= trgA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR   = vgfGetColorInverseGamma(trgR);
		}
		else
		{
			rINT   = value & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
		}

		trgR  *= trgA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_sRGBA_PRE_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		trgB  /= trgA;
		trgB   = vgfGetColorInverseGamma(trgB);
		trgB  *= trgA;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		trgG  /= trgA;
		trgG   = vgfGetColorInverseGamma(trgG);
		trgG  *= trgA;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		trgR  /= trgA;
		trgR   = vgfGetColorInverseGamma(trgR);
		trgR  *= trgA;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_Masked_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
		trgB  *= curA;
		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
	}

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
		trgG  *= curA;
		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
	}

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
		trgR  *= curA;
		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_Masked_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value &= ~0xFF000000;
	value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	if ((ChannelMask & VG_BLUE) == VG_BLUE)
	{
		trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	}
	else
	{
		bINT   = (value >> 16) & 0xFF;
		trgB   = vgmZERO2ONE(bINT, 8);
		trgB  /= curA;
	}

	trgB  *= trgA;
	value &= ~0x00FF0000;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	if ((ChannelMask & VG_GREEN) == VG_GREEN)
	{
		trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	}
	else
	{
		gINT   = (value >> 8) & 0xFF;
		trgG   = vgmZERO2ONE(gINT, 8);
		trgG  /= curA;
	}

	trgG  *= trgA;
	value &= ~0x0000FF00;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	if ((ChannelMask & VG_RED) == VG_RED)
	{
		trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	}
	else
	{
		rINT   = value & 0xFF;
		trgR   = vgmZERO2ONE(rINT, 8);
		trgR  /= curA;
	}

	trgR  *= trgA;
	value &= ~0x000000FF;
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	trgA   = gcmCLAMP(Value[3], 0.0f, 1.0f);
	value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

	trgB   = gcmCLAMP(Value[2], 0.0f, 1.0f);
	trgB  *= trgA;
	value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

	trgG   = gcmCLAMP(Value[1], 0.0f, 1.0f);
	trgG  *= trgA;
	value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

	trgR   = gcmCLAMP(Value[0], 0.0f, 1.0f);
	trgR  *= trgA;
	value |= vgmPACKCOMPONENT(trgR, 8, 32);

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBx_PRE_Masked_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value &= vgmCHANNELMASK(lABGR_8888_PRE, ChannelMask);
	}
	else
	{
		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
			trgB  /= trgA;
			trgB  *= curA;
			value &= ~0x00FF0000;
			value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;
		}

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
			trgG  /= trgA;
			trgG  *= curA;
			value &= ~0x0000FF00;
			value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;
		}

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
			trgR  /= trgA;
			trgR  *= curA;
			value &= ~0x000000FF;
			value |= vgmPACKCOMPONENT(trgR, 8, 32);
		}
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_Masked_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	register gctUINT8 aINT, bINT, gINT, rINT;
	gctFLOAT curA, trgA, trgB, trgG, trgR;

	/* Read the current destination value. */
	value = * (gctUINT32_PTR) Pixel->current;

	/* Convert the current value of the alpha channel. */
	aINT = (value >> 24) & 0xFF;
	curA = vgmZERO2ONE(aINT, 8);

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value &= ~0xFF000000;
		value |= vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		if ((ChannelMask & VG_BLUE) == VG_BLUE)
		{
			trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		}
		else
		{
			bINT   = (value >> 16) & 0xFF;
			trgB   = vgmZERO2ONE(bINT, 8);
			trgB  /= curA;
			trgB  *= trgA;
		}

		value &= ~0x00FF0000;
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		if ((ChannelMask & VG_GREEN) == VG_GREEN)
		{
			trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		}
		else
		{
			gINT   = (value >> 8) & 0xFF;
			trgG   = vgmZERO2ONE(gINT, 8);
			trgG  /= curA;
			trgG  *= trgA;
		}

		value &= ~0x0000FF00;
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		if ((ChannelMask & VG_RED) == VG_RED)
		{
			trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		}
		else
		{
			rINT   = value & 0xFF;
			trgR   = vgmZERO2ONE(rINT, 8);
			trgR  /= curA;
			trgR  *= trgA;
		}

		value &= ~0x000000FF;
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}

static void _WritePixel_lRGBA_PRE_To_lABGR_8888_PRE(
	vgsPIXELWALKER_PTR Pixel,
	vgtFLOATVECTOR4 Value,
	gctUINT ChannelMask
	)
{
	register gctUINT32 value;
	gctFLOAT trgA, trgB, trgG, trgR;

	/* Clamp the incoming alpha. */
	trgA = gcmCLAMP(Value[3], 0.0f, 1.0f);

	/* Convert the incoming color. */
	if (trgA == 0.0f)
	{
		value = 0;
	}
	else
	{
		value  = vgmPACKCOMPONENT(trgA, 8, 32) << 24;

		trgB   = gcmCLAMP(Value[2], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgB, 8, 32) << 16;

		trgG   = gcmCLAMP(Value[1], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgG, 8, 32) << 8;

		trgR   = gcmCLAMP(Value[0], 0.0f, trgA);
		value |= vgmPACKCOMPONENT(trgR, 8, 32);
	}

	/* Write the pixel value. */
	* (gctUINT32_PTR) Pixel->current = value;

	/* Advance to the next pixel. */
	Pixel->current += 4;
}
