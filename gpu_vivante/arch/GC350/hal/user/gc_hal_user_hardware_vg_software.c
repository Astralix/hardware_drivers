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




#include "gc_hal_user_hardware_precomp_vg.h"

#if gcdENABLE_VG

#include "gc_hal_user_vg.h"

/* TODO: Remove these includes! Not allowed in the HAL! */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <math.h>

#define _GC_OBJ_ZONE            gcvZONE_VG

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

#define gcmSTART_X		ControlCoordinates[0]
#define gcmSTART_Y		ControlCoordinates[1]
#define gcmLAST_X		ControlCoordinates[2]
#define gcmLAST_Y		ControlCoordinates[3]

#define gcvTESS_PATH	(1 << 0)
#define gcvTESS_SUBPATH	(1 << 1)
#define gcvTESS_START	(1 << 2)
#define gcvTESS_END		(1 << 3)

#define gcvCOUNT_COMMANDS		1
#define gcvDUMP_COMMAND_NAME	1
#define gcvDUMP_PARENTHESES		1


#define gcvDEBUG_SCALE_X_BIAS	0.0f
#define gcvDEBUG_SCALE_X_SCALE	1.0f

#define gcvDEBUG_SCALE_Y_BIAS	0.0f
#define gcvDEBUG_SCALE_Y_SCALE	1.0f

#define gcmDEBUG_SCALE_X(X) \
	(((X) - gcvDEBUG_SCALE_X_BIAS) * gcvDEBUG_SCALE_X_SCALE)

#define gcmDEBUG_SCALE_Y(Y) \
	(((Y) - gcvDEBUG_SCALE_Y_BIAS) * gcvDEBUG_SCALE_Y_SCALE)

#define gcmSEGMENT_DUMP_FORMAT(CommandArguments) \
	"%s%s%s%s" CommandArguments "%s\n"

#if gcvDEBUG && gcvCOUNT_COMMANDS
#define gcmGETCOUNT(CommandCount) _GetCommandCount(++CommandCount)
static gctSTRING _GetCommandCount(
	gctUINT CommandCount
	)
{
	static gctCHAR _buffer[10];
	return _buffer;
}
#else
#define gcmGETCOUNT(CommandCount) ""
#endif

#if gcvDUMP_COMMAND_NAME
#	define gcmDUMP_COMMAND_NAME(Name, RelativeCounterPart) \
		  #Name \
, (Name == RelativeCounterPart) ? "_REL" : ""
#else
#	define gcmDUMP_COMMAND_NAME(Name, RelativeCounterPart) \
		  "" \
, ""
#endif

#if gcvDUMP_PARENTHESES
#if gcvDUMP_COMMAND_NAME
#		define gcvLEFT_PARENTHESES " ("
#	else
#		define gcvLEFT_PARENTHESES "("
#	endif
#else
#		define gcvLEFT_PARENTHESES ""
#endif

#if gcvDUMP_PARENTHESES
#	define gcvRIGHT_PARENTHESES ")"
#else
#	define gcvRIGHT_PARENTHESES ""
#endif

static gctFLOAT _Coord(
	IN gcoVGHARDWARE Hardware,
	IN gcePATHTYPE DataType,
	IN gctUINT8_PTR Path,
	IN gctUINT_PTR Offset,
	IN gctFLOAT Current,
	IN gctBOOL Relative
	)
{
	gctFLOAT coord;

	switch (DataType)
	{
	case gcePATHTYPE_INT8:
		coord = (gctFLOAT) * (gctINT8_PTR) (Path + *Offset);
		*Offset += gcmSIZEOF(gctINT8);
		break;

	case gcePATHTYPE_INT16:
		while (*Offset & 1) *Offset += 1;
		coord = (gctFLOAT) * (gctINT16_PTR) (Path + *Offset);
		*Offset += gcmSIZEOF(gctINT16);
		break;

	case gcePATHTYPE_INT32:
		while (*Offset & 3) *Offset += 1;
		coord = (gctFLOAT) * (gctINT32_PTR) (Path + *Offset);
		*Offset += gcmSIZEOF(gctINT32);
		break;

	case gcePATHTYPE_FLOAT:
		while (*Offset & 3) *Offset += 1;
		coord = * (gctFLOAT_PTR) (Path + *Offset);
		*Offset += gcmSIZEOF(gctFLOAT);
		break;

	default:
		gcmASSERT(gcvFALSE);
		coord = 0.0f;
	}

	coord = coord * Hardware->vg.scale + Hardware->vg.bias;

	if (Relative)
	{
		coord += Current;
	}

	return coord;
}

static gctBOOL _UpdatePixel(
	IN gcoVGHARDWARE Hardware,
	IN gctINT X,
	IN gctINT Y,
	IN gctINT Adjustment,
	IN gctINT Valid
	)
{
	gctBOOL result;
	gctINT x;
	gctUINT offset;
	gctUINT l1Offset, l1Bit, l2Offset, l2Bit, l2shift;
	gctUINT8 l1Mask, l2Mask;
	gctINT16 data, count;
	gctINT maxNegative;
	gctUINT8_PTR tsBuffer, l1Buffer, l2Buffer;

#if gcvDEBUG
	gctUINT _caseCount = 0;
#endif

	x = (X < 0) ? 0 : X;

	switch (Hardware->vg.tsQuality)
	{
	case 0x0:
		/* X11 X10 X9 X8 X7 X6 X5 X4 X3 X2 X1 X0 0 */
		offset = (x & 0xFFF0) >> 3;
		break;

	case 0x1:
		/* X11 X10 X9 X8 X7 X6 X5 X4 X3 X2 X1 X0 y3 x3 0 */
		offset = ((x & 0x0008) >> 2)
			   | ((Y & 0x0008) >> 1)
			   | ((x & 0xFFF0) >> 1);
		break;

    case 0x2:

	case 0x3:
		/* X11 X10 X9 X8 X7 X6 X5 X4 X3 X2 X1 X0 y3 y2 x3 */
		offset = ((x & 0x0008) >> 3)
			   | ((Y & 0x000C) >> 1)
			   | ((x & 0xFFF0) >> 1);
		break;

	default:
		gcmASSERT(gcvFALSE);
		offset = 0;
	}

	offset += (Y >> 4) * Hardware->vg.tsBuffer->stride;

	l1Offset = offset >> 9;
	l1Bit    = (offset >> 6) & 7;
	l1Mask   = 1 << l1Bit;
	l2shift  = Hardware->vg.tsBuffer->shift;
	tsBuffer = Hardware->vg.tsBuffer->tsBufferLogical;
	l1Buffer = Hardware->vg.tsBuffer->L1BufferLogical;
	l2Buffer = Hardware->vg.tsBuffer->L2BufferLogical;

	if (Hardware->vg21)
	{
#if gcvDEBUG
		_caseCount |= (1 << 0);
#endif

		l2Offset = ((offset >> (17 + l2shift)) & 7)
				 | (((offset >> 14) & ((1 << l2shift) - 1)) << 3)
				 | ((offset >> (20 + l2shift)) << (3 + l2shift));
		l2Bit    = (offset >> (14 + l2shift)) & 7;
	}
	else
	{
		l2Offset = ((offset >> (15 + l2shift)) & 7)
				 | (((offset >> 12) & ((1 << l2shift) - 1)) << 3)
				 | ((offset >> (18 + l2shift)) << (3 + l2shift));
		l2Bit    = (offset >> (12 + l2shift)) & 7;
	}

	l2Mask = 1 << l2Bit;

	if ((l2Buffer[l2Offset] & l2Mask) == 0)
	{
#if gcvDEBUG
		_caseCount |= (1 << 0);
#endif

		l2Buffer[l2Offset] |= l2Mask;

		if (Hardware->vg21)
		{
#if gcvDEBUG
			_caseCount |= (1 << 0);
#endif

			gcoOS_ZeroMemory(l1Buffer + (l1Offset & ~31), 32);
		}
		else
		{
			gcoOS_ZeroMemory(l1Buffer + (l1Offset & ~7), 8);
		}
	}

	if ((l1Buffer[l1Offset] & l1Mask) == 0)
	{
#if gcvDEBUG
		_caseCount |= (1 << 0);
#endif

		l1Buffer[l1Offset] |= l1Mask;

		gcoOS_ZeroMemory(tsBuffer + (offset & ~63), 64);
	}

	switch (Hardware->vg.tsQuality)
	{
	case 0x0:
	case 0x1:
		data        = ((gctINT16_PTR) tsBuffer)[offset >> 1];
		maxNegative = -1 << 8;
		break;

	case 0x2:
		data        = (gctINT8) tsBuffer[offset];
		maxNegative = -1 << 6;
		break;

	case 0x3:
		data        = (gctINT8) tsBuffer[offset];
		maxNegative = -1 << 2;
		if (x & 0x4)
			data >>= 4;
		else
			data = (data & 0xF) | ((data & 0x8) ? 0xFFF0 : 0);
		break;

	default:
		gcmASSERT(gcvFALSE);
		maxNegative = 0;
		data = 0;
	}

	count = data >> 1;

	if ((count == maxNegative) && !(data & 1))
	{
#if gcvDEBUG
		_caseCount |= (1 << 0);
#endif

		count = 0;
	}

	if (Valid && (X >= 0))
	{
#if gcvDEBUG
		_caseCount |= (1 << 0);
#endif

		data |= 1;
	}

	count += (gctINT16)Adjustment;

	if (count == maxNegative)
	{
#if gcvDEBUG
		_caseCount |= (1 << 4);
#endif

		result = gcvFALSE;
	}
	else
	{
		if ((count == 0) && (X >= 0) && !(data & 1))
		{
#if gcvDEBUG
			_caseCount |= (1 << 0);
#endif

			count = (gctINT16)maxNegative;
		}

		data = (data & 1) | (count << 1);

		switch (Hardware->vg.tsQuality)
		{
		case 0x0:
		case 0x1:
			((gctINT16_PTR) tsBuffer)[offset >> 1] = data;
			break;

		case 0x2:
			tsBuffer[offset] = (gctUINT8) data;
			break;

		case 0x3:
			if (x & 0x4)
			{
				tsBuffer[offset] &= 0x0F;
				tsBuffer[offset] |= (gctUINT8) (data << 4);
			}
			else
			{
				tsBuffer[offset] &= 0xF0;
				tsBuffer[offset] |= (gctUINT8) (data & 0x0F);
			}
			break;
		}

		result = gcvTRUE;
	}

#if gcvDEBUG
#if defined(gcvTRACE_LINE)
	if ((Y >> 4) == gcvTRACE_LINE)
#endif
	{
		gcmTRACE_ZONE(
			gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
			"      update: X = %d, Y = %d, offset = 0x%08X, case count = 0x%08X\n"
			"          L1: l1Offset = 0x%08X, l1Bit = %d, l1Mask = 0x%02X\n"
			"          L2: l2Offset = 0x%08X, l2Bit = %d, l2Mask = 0x%02X\n",
			(X >> 4), (Y >> 4), offset, _caseCount,
			l1Offset, l1Bit, l1Mask,
			l2Offset, l2Bit, l2Mask
			);
	}
#endif

	return result;
}

static gctBOOL _Pixel(
	IN gcoVGHARDWARE Hardware,
	IN gctINT X,
	IN gctINT Y,
	IN gctUINT Flags,
	OUT gcsRECT_PTR BoundingBox
	)
{
	static gctINT lastX, lastY, firstX;
	static gctBOOL lastInside, lastUp, lastHorz, lastLeft, firstPixel;
	static gctBOOL firstUp, firstHorz, firstLeft, firstInside, anyPixel;

	gctINT x, y, adjust;
	gctBOOL up, left, inside, valid;

	x = X >> 4;
	y = Y >> 4;

	up   = !((Flags & gcvTESS_PATH) || (Y < lastY));
	left = !((Flags & gcvTESS_PATH) || (X > lastX));

#if defined(gcvTRACE_LINE)
	if (Y == gcvTRACE_LINE)
#endif
	{
		gcmTRACE_ZONE(
			gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
			"      X = %d, Y = %d, x = %d, y = %d, up = %d, left = %d, anyPixel = %d\n",
			X, Y, x, y, up, left, anyPixel
			);
	}

	if (BoundingBox == gcvNULL)
	{
		if (!anyPixel)
		{
		}

		else if (lastHorz && firstHorz)
		{
			gctINT index = (lastUp << 3) + (lastLeft << 2) + (firstUp << 1) + firstUp;

			adjust = 0;
			valid  = (index == 3);

			if (lastInside && (adjust || valid))
			{
				if (!_UpdatePixel(Hardware, lastX, lastY, adjust, valid))
				{
					return gcvFALSE;
				}
			}

			adjust = (index & 1) ? -1 : 0;
			valid  = (index & 1) ?  1 : 0;

			if (firstInside && (adjust || valid))
			{
				if (!_UpdatePixel(Hardware, firstX, lastY, adjust, valid))
				{
					return gcvFALSE;
				}
			}
		}

		else if (lastHorz && !firstHorz)
		{
			static const gctINT adjustLast[8] = { 0, -1, 0, -1, 0, -1, 0, -1 };
			static const gctINT8 validLast[8] = { 1, 1, 0, 1, 0, 1, 0, 1 };

			gctINT index = (lastUp << 2) + (lastLeft << 1) + (firstUp << 0);

			adjust = adjustLast[index];
			valid  = validLast[index];

			if (lastInside && (adjust || valid))
			{
				if (!_UpdatePixel(Hardware, lastX, lastY, adjust, valid))
				{
					return gcvFALSE;
				}
			}
		}

		else if (!lastHorz && firstHorz)
		{
			static const gctINT adjustLast[8]  = { 1, 1, 1, 1, 0, 0, 0, 0 };
			static const gctINT8 validLast[8]  = { 1, 1, 1, 1, 0, 0, 0, 0 };
			static const gctINT adjustFirst[8] = { 0, -1, 0, -1, 0, -1, 0, -1 };
			static const gctINT8 validFirst[8] = { 1, 1, 0, 1, 0, 1, 0, 1 };

			gctINT index = (lastUp << 2) + (firstLeft << 1) + (firstUp << 0);

			adjust = adjustLast[index];
			valid  = validLast[index];

			if (lastInside && (adjust || valid))
			{
				if (!_UpdatePixel(Hardware, lastX, lastY, adjust, valid))
				{
					return gcvFALSE;
				}
			}

			adjust = adjustFirst[index];
			valid  = validFirst[index];

			if (firstInside && (adjust || valid))
			{
				if (!_UpdatePixel(Hardware, firstX, lastY, adjust, valid))
				{
					return gcvFALSE;
				}
			}
		}

		else if (lastInside)
		{
			adjust = (lastUp != firstUp) ? 0 : lastUp ? -1 : 1;
			valid  = (lastUp != firstUp) ? 0 : 1;

			return _UpdatePixel(Hardware, firstX, lastY, adjust, valid);
		}

		return gcvTRUE;
	}

	inside
		=  (x < Hardware->vg.targetWidth)
		&& (y < Hardware->vg.targetHeight)
		&& (y >= 0);

	x = (x < 0)
		? 0
		: (x >= Hardware->vg.targetWidth)
			? Hardware->vg.targetWidth - 1
			: x;

	y = (y < 0)
		? 0
		: (y >= Hardware->vg.targetHeight)
			? Hardware->vg.targetHeight - 1
			: y;

	if (x < BoundingBox->left)   BoundingBox->left   = x;
	if (x > BoundingBox->right)  BoundingBox->right  = x;
	if (y < BoundingBox->top)    BoundingBox->top    = y;
	if (y > BoundingBox->bottom) BoundingBox->bottom = y;

	if (Flags & gcvTESS_SUBPATH)
	{
		lastX      = X;
		lastY      = Y;
		lastInside = inside;
		lastUp     = 1;
		lastHorz   = 0;
		lastLeft   = 0;
		firstPixel = 1;
		anyPixel   = 0;
	}

	else if ((Y == lastY) && (X != lastX))
	{
		if (!lastHorz)
		{
			static const gctINT adjustLast[4] = { 1, 1, 0, 0 };
			static const gctINT8 validLast[4] = { 1, 1, 0, 0 };

			gctINT index = (lastUp << 1) + (left << 0);

			adjust = adjustLast[index];
			valid  = validLast[index];

			lastLeft = left;
		}
		else
		{
			adjust = 0;
			valid  = 0;
		}

		if (lastInside)
		{
			if (!_UpdatePixel(Hardware, lastX, lastY,
							  firstPixel ? 0 : adjust,
							  firstPixel ? 0 : valid))
			{
				return gcvFALSE;
			}
		}

		lastX      = X;
		lastInside = inside;
		lastHorz   = 1;
	}

	else if (Y != lastY)
	{
		if (lastHorz)
		{
			static const gctINT adjustLast[8] = { 0, -1, 0, -1, 0, -1, 0, -1 };
			static const gctINT8 validLast[8] = { 1, 1, 0, 1, 0, 1, 0, 1 };

			gctINT index = (lastUp << 2) + (lastLeft << 1) + (up << 0);

			adjust = adjustLast[index];
			valid  = validLast[index];
		}
		else
		{
			adjust = (up != lastUp) ? 0 : up ? -1 : 1;
			valid  = (up != lastUp) ? 0 : 1;
		}

		if (lastInside)
		{
			if (!_UpdatePixel(Hardware, lastX, lastY,
							  firstPixel ? 0 : adjust,
							  firstPixel ? 0 : valid))
			{
				return gcvFALSE;
			}
		}

		if (firstPixel)
		{
			firstPixel  = 0;
			firstX      = lastX;
			firstUp     = up;
			firstLeft   = lastLeft;
			firstHorz   = lastHorz;
			firstInside = lastInside;
		}

		lastX      = X;
		lastY      = Y;
		lastInside = inside;
		lastUp     = up;
		lastLeft   = left;
		lastHorz   = 0;
		anyPixel   = 1;
	}

	return gcvTRUE;
}

static void _Fill(
	IN gcoVGHARDWARE Hardware,
	IN gctFLOAT X0,
	IN gctFLOAT Y0,
	IN gctFLOAT X1,
	IN gctFLOAT Y1,
	IN gctUINT Flags,
	OUT gcsRECT_PTR BoundingBox
	)
{
	gctFLOAT_PTR userToSurface = Hardware->vg.userToSurface;
	gctBOOL first = (gctBOOL) (Flags & gcvTESS_SUBPATH);

	/* Transform the coordinates. */
	gctFLOAT x0
		= gcmMAT(userToSurface, 0, 0) * X0
		+ gcmMAT(userToSurface, 0, 1) * Y0
		+ gcmMAT(userToSurface, 0, 2);

	gctFLOAT y0
		= gcmMAT(userToSurface, 1, 0) * X0
		+ gcmMAT(userToSurface, 1, 1) * Y0
		+ gcmMAT(userToSurface, 1, 2);

	gctFLOAT x1
		= gcmMAT(userToSurface, 0, 0) * X1
		+ gcmMAT(userToSurface, 0, 1) * Y1
		+ gcmMAT(userToSurface, 0, 2);

	gctFLOAT y1
		= gcmMAT(userToSurface, 1, 0) * X1
		+ gcmMAT(userToSurface, 1, 1) * Y1
		+ gcmMAT(userToSurface, 1, 2);

	/* Compute deltas. */
	gctFLOAT dx = x1 - x0;
	gctFLOAT dy = y1 - y0;

	/* Determine line type. */
	gctBOOL xMajor = gcmFABSF(dx) > gcmFABSF(dy);
	gctBOOL left   = dx < 0;
	gctBOOL down   = dy < 0;

	gctUINT maskX, maskY;
	gctINT incX, incY;
	gctINT x, y, xEnd, yEnd;

	/* Compute mask and steps based on rendering quality. */
	switch (Hardware->vg.tsQuality)
	{
	case 0x0:
		maskX = (gctUINT)(~0 << 4);
		maskY = (gctUINT)(~0 << 4);
		incX  =  1 << 4;
		incY  =  1 << 4;
		break;

	case 0x1:
		maskX = (gctUINT)(~0 << 3);
		maskY = (gctUINT)(~0 << 3);
		incX  =  1 << 3;
		incY  =  1 << 3;
		break;

	case 0x2:
		maskX = (gctUINT)(~0 << 3);
		maskY = (gctUINT)(~0 << 2);
		incX  =  1 << 3;
		incY  =  1 << 2;
		break;

	case 0x3:
		maskX = (gctUINT)(~0 << 2);
		maskY = (gctUINT)(~0 << 2);
		incX  =  1 << 2;
		incY  =  1 << 2;
		break;

	default:
		gcmASSERT(gcvFALSE);
		maskX = maskY = incX = incY = 0;
	}

	x    =  (gctINT) floor(x0 * 16.0f + 0.5f) & maskX;
	y    = ((gctINT) floor(y0 * 16.0f + 0.5f) + incY / 2 - 1) & maskY;
	xEnd = ((gctINT) floor(x1 * 16.0f + 0.5f) + incX / 2 - 1) & maskX;
	yEnd = ((gctINT) floor(y1 * 16.0f + 0.5f) + incY / 2 - 1) & maskY;

#if defined(gcvTRACE_LINE)
	if ((y >> 4) == gcvTRACE_LINE)
#endif
	{
		gcmTRACE_ZONE(
			gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
			"   FILL:\n"
			"      original X0 = %2.10f, Y0 = %2.10f, X1   = %2.10f, Y1   = %2.10f\n"
			"      xformed  x0 = %2.10f, y0 = %2.10f, x1   = %2.10f, y1   = %2.10f\n"
			"      rounded  x  = %2.10f, y  = %2.10f, xEnd = %2.10f, yEnd = %2.10f\n",
			X0, Y0, X1, Y1, x0, y0, x1, y1,
			x / 16.0, y / 16.0, xEnd / 16.0, yEnd / 16.0
			);

		gcmTRACE_ZONE(
			gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
			"      dx = %2.10f, dy = %2.10f, first = %d, xMajor = %d, left = %d, (y==yEnd) = %d\n",
			dx, dy, first, xMajor, left, (y == yEnd)
			);
	}

	if (y == yEnd)
	{
		x = ((gctINT) floor(x0 * 16.0f + 0.5f) + incX / 2 - 1) & maskX;

		if (left)
		{
			for (; x >= xEnd; x -= incX)
			{
				_Pixel(Hardware, x, y, first, BoundingBox);
				first = 0;
			}
		}
		else
		{
			for (; x <= xEnd; x += incX)
			{
				_Pixel(Hardware, x, y, first, BoundingBox);
				first = 0;
			}
		}
	}

	else
	{
		gctFLOAT nx = down ?  dy : -dy;
		gctFLOAT ny = down ? -dx :  dx;
		gctFLOAT nc = (down ? x1 : x0) * nx + (down ? y1 : y0) * ny;
		gctFLOAT error;
		gctINT plotX;

		gcmTRACE_ZONE(
			gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
			"      down = %d, nx = %2.10f, ny = %2.10f, nc = %2.10f\n",
			down, nx, ny, nc
			);

		if (down)
		{
			for (; y >= yEnd; y -= incY)
			{
				error = ((x + incX / 2) / 16.0f) * nx + ((y + incY / 2) / 16.0f) * ny - nc;

				gcmTRACE_ZONE(
					gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
					"      error = %2.10f\n",
					error
					);

				while ((error != 0) && ((left && (error < 0)) || (!left && (error >= 0))))
				{
					x += left ? -incX : incX;
					error = ((x + incX / 2) / 16.0f) * nx + ((y + incY / 2) / 16.0f) * ny - nc;

					gcmTRACE_ZONE(
						gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
						"      x = %2.10f, error = %2.10f\n",
						x / 16.0, error
						);
				}

				plotX = x + ((error > 0) ? incX : 0);

				gcmTRACE_ZONE(
					gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
					"      plotX = %2.10f\n",
					plotX
					);

				if (xMajor && (left ? (plotX < xEnd) : (plotX > xEnd)))
				{
					gcmTRACE_ZONE(
						gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
						"      use xEnd as plotX value, xEnd = %2.10f\n",
						xEnd
						);

					plotX = xEnd;
				}

				_Pixel(Hardware, plotX, y, first, BoundingBox);
				first = 0;
			}
		}
		else
		{
			for (; y <= yEnd; y += incY)
			{
				error = ((x + incX / 2) / 16.0f) * nx + ((y + incY / 2) / 16.0f) * ny - nc;

				gcmTRACE_ZONE(
					gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
					"      error = %2.10f\n",
					error
					);

				while ((error != 0) && ((left && (error < 0)) || (!left && (error >= 0))))
				{
					x += left ? -incX : incX;
					error = ((x + incX / 2) / 16.0f) * nx + ((y + incY / 2) / 16.0f) * ny - nc;

					gcmTRACE_ZONE(
						gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
						"      x = %2.10f, error = %2.10f\n",
						x / 16.0, error
						);
				}

				plotX = x + ((error > 0) ? incX : 0);

				gcmTRACE_ZONE(
					gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
					"      plotX = %2.10f\n",
					plotX
					);

				if (xMajor && (left ? (plotX < xEnd) : (plotX > xEnd)))
				{
					gcmTRACE_ZONE(
						gcvLEVEL_VERBOSE, gcvZONE_UTILITY,
						"      use xEnd as plotX value, xEnd = %2.10f\n",
						xEnd
						);

					plotX = xEnd;
				}

				_Pixel(Hardware, plotX, y, first, BoundingBox);
				first = 0;
			}
		}
	}

	if (xMajor && left && (x > xEnd))
	{
		_Pixel(Hardware, xEnd, yEnd, first, BoundingBox);
	}
}

static void _Flatten(
	IN gcoVGHARDWARE Hardware,
	IN gctUINT Flags,
	IN gctINT VertexCount,
	IN gctFLOAT X[4],
	IN gctFLOAT Y[4],
	OUT gcsRECT_PTR BoundingBox
	)
{
	gctFLOAT x, y;
	gctFLOAT step, t, lastX, lastY;
	gctUINT flags = Flags;

	gcmASSERT(VertexCount > 1);

	/* Lines don't need flattening. */
	if (VertexCount == 2)
	{
		_Fill(
			Hardware,
			X[0], Y[0],
			X[1], Y[1],
			flags | gcvTESS_START | gcvTESS_END,
			BoundingBox
			);

		return;
	}

	/* Select flattening quality based on rendering quality. */
	switch (Hardware->vg.tsQuality)
	{
	case 0x0:
		step = 1.0f / 32.0f;
		break;

	case 0x1:
		step = 1.0f / 64.0f;
		break;

	case 0x2:
		step = 1.0f / 128.0f;
		break;

	case 0x3:
		step = 1.0f / 256.0f;
		break;

	default:
		gcmASSERT(gcvFALSE);
		step = 0.0f;
	}

	/* Save current location. */
	lastX = X[0];
	lastY = Y[0];

	/* Walk through the curve. */
	for (t = step; t < 1.0f; t += step)
	{
		gctFLOAT tInv = 1.0f - t;

		/* Flatten a cubic curve. */
		if (VertexCount == 4)
		{
			gctFLOAT f0 = tInv * tInv * tInv;
			gctFLOAT f1 = 3 * tInv * tInv * t;
			gctFLOAT f2 = 3 * tInv * t * t;
			gctFLOAT f3 = t * t * t;

			x = X[0] * f0 + X[1] * f1 + X[2] * f2 + X[3] * f3;
			y = Y[0] * f0 + Y[1] * f1 + Y[2] * f2 + Y[3] * f3;
		}

		/* Flatten a quadratic curve. */
		else
		{
			gctFLOAT f0 = tInv * tInv;
			gctFLOAT f1 = 2 * tInv * t;
			gctFLOAT f2 = t * t;

			x = X[0] * f0 + X[1] * f1 + X[2] * f2;
			y = Y[0] * f0 + Y[1] * f1 + Y[2] * f2;
		}

		/* Render the line from the last point to the flattened point. */
		_Fill(
			Hardware,
			lastX, lastY,
			x, y,
			flags | gcvTESS_START,
			BoundingBox
			);

		/* Save current point. */
		lastX = x;
		lastY = y;
		flags = 0;
	}

	/* Render the line to the last point. */
	_Fill(
		Hardware,
		lastX, lastY,
		X[VertexCount - 1], Y[VertexCount - 1],
		flags | gcvTESS_END,
		BoundingBox
		);
}

static void _Data(
	IN gcoVGHARDWARE Hardware,
	IN gctUINT8_PTR Path,
	IN gctUINT Size,
	IN gcePATHTYPE DataType,
	IN gctFLOAT_PTR ControlCoordinates,
	IN gctUINT_PTR Flags,
	OUT gcsRECT_PTR BoundingBox
	)
{
	gctFLOAT x[4], y[4];
	gctUINT offset;
	gctUINT8 opcode;

#if gcvDEBUG && gcvCOUNT_COMMANDS
	gctUINT _countCommands = 0;
#endif

	offset = 0;

	for (;;)
	{
		opcode = Path[offset++] & gcvVGCMD_MASK;

		switch (opcode)
		{
		case 0x00:

			if (*Flags == 0)
			{
				x[0] = gcmLAST_X;
				y[0] = gcmLAST_Y;
				x[1] = gcmSTART_X;
				y[1] = gcmSTART_Y;

				_Flatten(Hardware, 0, 2, x, y, BoundingBox);
				_Pixel(Hardware, 0, 0, 0, gcvNULL);
			}

#if gcvDEBUG && gcvDUMP_COMMAND_NAME
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_UTILITY
, gcmSEGMENT_DUMP_FORMAT("")
, gcmGETCOUNT(_countCommands)
, gcmDUMP_COMMAND_NAME(0x00, ~0)
, "", ""
				);
#endif

			return;

		case 0x0A:

#if gcvDEBUG && gcvDUMP_COMMAND_NAME
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_UTILITY
, gcmSEGMENT_DUMP_FORMAT("")
, gcmGETCOUNT(_countCommands)
, gcmDUMP_COMMAND_NAME(0x0A, ~0)
, "", ""
				);
#endif

			return;

		case 0x01:

#if gcvDEBUG
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_UTILITY
, gcmSEGMENT_DUMP_FORMAT("")
, gcmGETCOUNT(_countCommands)
, gcmDUMP_COMMAND_NAME(0x01, ~0)
, "", ""
				);
#endif

			if (*Flags == 0)
			{
				x[0] = gcmLAST_X;
				y[0] = gcmLAST_Y;
				x[1] = gcmSTART_X;
				y[1] = gcmSTART_Y;

				_Flatten(Hardware, 0, 2, x, y, BoundingBox);
				_Pixel(Hardware, 0, 0, 0, gcvNULL);

				gcmLAST_X = gcmSTART_X;
				gcmLAST_Y = gcmSTART_Y;

				*Flags = gcvTESS_SUBPATH;
			}
			break;

		case 0x02:
		case 0x03:
			if (*Flags == 0)
			{
				x[0] = gcmLAST_X;
				y[0] = gcmLAST_Y;
				x[1] = gcmSTART_X;
				y[1] = gcmSTART_Y;

				_Flatten(Hardware, 0, 2, x, y, BoundingBox);
				_Pixel(Hardware, 0, 0, 0, gcvNULL);

				*Flags = gcvTESS_SUBPATH;
			}

			gcmSTART_X = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_X,
				opcode == 0x03
				);

			gcmSTART_Y = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_Y,
				opcode == 0x03
				);

			gcmLAST_X = gcmSTART_X;
			gcmLAST_Y = gcmSTART_Y;

#if gcvDEBUG
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_UTILITY
, gcmSEGMENT_DUMP_FORMAT("%2.10f, %2.10f")
, gcmGETCOUNT(_countCommands)
, gcmDUMP_COMMAND_NAME(0x02, 0x03)
, gcvLEFT_PARENTHESES
, gcmDEBUG_SCALE_X(gcmLAST_X)
, gcmDEBUG_SCALE_Y(gcmLAST_Y)
, gcvRIGHT_PARENTHESES
				);
#endif

			break;

		case 0x04:
		case 0x05:
			x[0] = gcmLAST_X;
			y[0] = gcmLAST_Y;

			x[1] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_X,
				opcode == 0x05
				);

			y[1] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_Y,
				opcode == 0x05
				);

			gcmLAST_X = x[1];
			gcmLAST_Y = y[1];

#if gcvDEBUG
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_UTILITY
, gcmSEGMENT_DUMP_FORMAT("%2.10f, %2.10f")
, gcmGETCOUNT(_countCommands)
, gcmDUMP_COMMAND_NAME(0x04, 0x05)
, gcvLEFT_PARENTHESES
, gcmDEBUG_SCALE_X(x[1])
, gcmDEBUG_SCALE_Y(y[1])
, gcvRIGHT_PARENTHESES
				);
#endif

			_Flatten(Hardware, *Flags, 2, x, y, BoundingBox);

			*Flags = 0;
			break;

		case 0x06:
		case 0x07:
			x[0] = gcmLAST_X;
			y[0] = gcmLAST_Y;

			x[1] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_X,
				opcode == 0x07
				);

			y[1] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_Y,
				opcode == 0x07
				);

			x[2] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_X,
				opcode == 0x07
				);

			y[2] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_Y,
				opcode == 0x07
				);

			gcmLAST_X = x[2];
			gcmLAST_Y = y[2];

#if gcvDEBUG
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_UTILITY
, gcmSEGMENT_DUMP_FORMAT("%2.10f, %2.10f, %2.10f, %2.10f")
, gcmGETCOUNT(_countCommands)
, gcmDUMP_COMMAND_NAME(0x06, 0x07)
, gcvLEFT_PARENTHESES
, gcmDEBUG_SCALE_X(x[1])
, gcmDEBUG_SCALE_Y(y[1])
, gcmDEBUG_SCALE_X(x[2])
, gcmDEBUG_SCALE_Y(y[2])
, gcvRIGHT_PARENTHESES
				);
#endif

			_Flatten(Hardware, *Flags, 3, x, y, BoundingBox);

			*Flags = 0;
			break;

		case 0x08:
		case 0x09:
			x[0] = gcmLAST_X;
			y[0] = gcmLAST_Y;

			x[1] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_X,
				opcode == 0x09
				);

			y[1] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_Y,
				opcode == 0x09
				);

			x[2] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_X,
				opcode == 0x09
				);

			y[2] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_Y,
				opcode == 0x09
				);

			x[3] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_X,
				opcode == 0x09
				);

			y[3] = _Coord(
				Hardware,
				DataType,
				Path, &offset,
				gcmLAST_Y,
				opcode == 0x09
				);

#if gcvDEBUG
			gcmTRACE_ZONE(
				gcvLEVEL_INFO, gcvZONE_UTILITY
, gcmSEGMENT_DUMP_FORMAT("%2.10f, %2.10f, %2.10f, %2.10f, %2.10f, %2.10f")
, gcmGETCOUNT(_countCommands)
, gcmDUMP_COMMAND_NAME(0x08, 0x09)
, gcvLEFT_PARENTHESES
, gcmDEBUG_SCALE_X(x[1])
, gcmDEBUG_SCALE_Y(y[1])
, gcmDEBUG_SCALE_X(x[2])
, gcmDEBUG_SCALE_Y(y[2])
, gcmDEBUG_SCALE_X(x[3])
, gcmDEBUG_SCALE_Y(y[3])
, gcvRIGHT_PARENTHESES
				);
#endif

			gcmLAST_X = x[3];
			gcmLAST_Y = y[3];

			_Flatten(Hardware, *Flags, 4, x, y, BoundingBox);

			*Flags = 0;
			break;

		default:
			gcmASSERT(gcvFALSE);
			return;
		}
	}
}


/******************************************************************************\
** gcoVGHARDWARE: Path API functions. ********************************************
\******************************************************************************/

gceSTATUS
gcoVGHARDWARE_Tesselate(
	IN gcoVGHARDWARE Hardware,
	IN gcsPATH_DATA_PTR PathData,
	OUT gcsRECT_PTR BoundingBox
	)
{
	gceSTATUS status;

    gcmHEADER_ARG("Hardware=0x%x", Hardware);

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Hardware, gcvOBJ_HARDWARE);
	gcmVERIFY_ARGUMENT(PathData != gcvNULL);

	do
	{
		gcsPATH_DATA_PTR pathData;
		gctUINT64_PTR stream;
		gctUINT32 fecommand;
		/*gctUINT32 fedata;*/
		gctUINT8 feopcode;
		gctFLOAT controlCoordinates[4];
		gctUINT flags;

		/* Make sure we don't kill previous buffer during rendering. */
		gcmERR_BREAK(gcoVGHARDWARE_Semaphore(
			Hardware,
			gcvNULL,
			gcvBLOCK_COMMAND,
			gcvBLOCK_PIXEL,
			gcvHOW_SEMAPHORE_STALL,
			gcvNULL
			));

		gcmERR_BREAK(gcoVGHARDWARE_Commit(
			Hardware, gcvTRUE
			));

		/* Clear the tesselation buffer. */
		gcmERR_BREAK(gcoOS_ZeroMemory(
			Hardware->vg.tsBuffer->L2BufferLogical,
			Hardware->vg.tsBuffer->clearSize
			));

		/* Reset the control coordinates. */
		controlCoordinates[0] = 0.0f;
		controlCoordinates[1] = 0.0f;
		controlCoordinates[2] = 0.0f;
		controlCoordinates[3] = 0.0f;

		/* Set initial flags. */
		flags = gcvTESS_PATH | gcvTESS_SUBPATH;

		/* Reset the bounding box. */
		if (BoundingBox != gcvNULL)
		{
			BoundingBox->left   = gcvMAX_POS_INT;
			BoundingBox->top    = gcvMAX_POS_INT;
			BoundingBox->right  = gcvMAX_NEG_INT;
			BoundingBox->bottom = gcvMAX_NEG_INT;
		}

		/* Set initial path data pointer. */
		pathData = PathData;

		/* Iterate through all buffers. */
		while (pathData)
		{
			/* Set initial command stream pointer. */
			stream = (gctUINT64_PTR)
			(
				(gctUINT8_PTR) pathData + pathData->data.bufferOffset
			);

			/* Loop through all commands in the buffer. */
			while (gcvTRUE)
			{
				/* Get the current command and data. */
				fecommand = (gctUINT32)   * stream;
                                /*fedata    = (gctUINT32) ((* stream) >> 32);*/
				/* Extract the opcode. */
				feopcode = (gctUINT8) (((((gctUINT32) (fecommand)) >> (0 ? 31:28 )) & ((gctUINT32) ((((1 ? 31:28 ) - (0 ? 31:28 ) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 31:28 ) - (0 ? 31:28 ) + 1)))))) );

				/* END? */
				if ((feopcode == 0x0) ||
					(feopcode == 0x5) ||
					(feopcode == 0x7))
				{
					break;
				}

				/* STATE? */
				else if (feopcode == 0x3)
				{
					/* Extract the state counter. */
					gctUINT32 stateCount = (((((gctUINT32) (fecommand)) >> (0 ? 27:16 )) & ((gctUINT32) ((((1 ? 27:16 ) - (0 ? 27:16 ) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 27:16 ) - (0 ? 27:16 ) + 1)))))) );

					/* Determine the skip value. */
					gctUINT skipValue = gcmALIGN(stateCount + 1, 2) / 2;

					/* Skip over. */
					stream += skipValue;
				}

				/* DATA? */
				else if (feopcode == 0x4)
				{
					/* Extract the state counter. */
					gctUINT32 dataCount = (((((gctUINT32) (fecommand)) >> (0 ? 15:0 )) & ((gctUINT32) ((((1 ? 15:0 ) - (0 ? 15:0 ) + 1) == 32) ? ~0 : (~(~0 << ((1 ? 15:0 ) - (0 ? 15:0 ) + 1)))))) );

					/* Skip the command. */
					stream += 1;

					/* Process path data. */
					_Data(
						Hardware,
						(gctUINT8_PTR) stream,
						dataCount * 8,
						pathData->dataType,
						controlCoordinates,
						&flags,
						BoundingBox
						);

					/* Skip over data. */
					stream += (dataCount);
				}

				/* Unsupported command. */
				else
				{
					gcmASSERT(gcvFALSE);
					break;
				}
			}

			/* Advance to the next buffer. */
			pathData = (gcsPATH_DATA_PTR) pathData->data.nextSubBuffer;
		}
	}
	while (gcvFALSE);

	/* Return status. */
        gcmFOOTER();
	return status;
}

#endif /* gcdENABLE_VG */

