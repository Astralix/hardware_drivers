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

static VGfloat _Angle(
	VGfloat Ux,
	VGfloat Uy,
	VGfloat Vx,
	VGfloat Vy
	)
{
	VGfloat dot, length, angle, cosVal;
	VGint sign;

	dot    = Ux * Vx + Uy * Vy;
	length = gcmSQRTF(Ux * Ux + Uy * Uy) * gcmSQRTF(Vx * Vx + Vy * Vy);
	sign   = (Ux * Vy - Uy * Vx < 0) ? -1 : 1;
	cosVal = dot / length;
	cosVal = gcmCLAMP(cosVal, -1.0f, 1.0f);
	angle  = sign * gcmACOSF(cosVal);

	return angle;
}

gceSTATUS vgfConvertArc(
	vgsPATHWALKER_PTR Destination,
	gctFLOAT HorRadius,
	gctFLOAT VerRadius,
	gctFLOAT RotAngle,
	gctFLOAT EndX,
	gctFLOAT EndY,
	gctBOOL CounterClockwise,
	gctBOOL Large,
	gctBOOL Relative
	)
{
	gceSTATUS status;
	vgsARCCOORDINATES_PTR arcCoords = gcvNULL;

	do
	{
		gctFLOAT endX, endY;
		gceVGCMD segmentCommand;
		vgsCONTROL_COORD_PTR coords;


		/*******************************************************************
		** Determine the absolute end point coordinates.
		*/

		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		if (Relative)
		{
			endX = EndX + coords->lastX;
			endY = EndY + coords->lastY;
		}
		else
		{
			endX = EndX;
			endY = EndY;
		}

		/* If both of the radiuses are zeros, make a line out of the arc. */
		if ((HorRadius == 0.0f) || (VerRadius == 0.0f) ||
			((endX == coords->lastX) && (endY == coords->lastY)))
		{
			/* Determine the segment command. */
			segmentCommand = Relative
				? gcvVGCMD_ARC_LINE_REL
				: gcvVGCMD_ARC_LINE;

			/* Allocate a new subpath. */
			gcmERR_BREAK(vgsPATHWALKER_StartSubpath(
				Destination, vgmCOMMANDSIZE(2, gctFLOAT), gcePATHTYPE_FLOAT
				));

			/* Write the command. */
			gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
				Destination,
				segmentCommand
				));

			/* Write the coordinates. */
			Destination->set(Destination, EndX);
			Destination->set(Destination, EndY);
		}
		else
		{
			VGfloat phi, cosPhi, sinPhi;
			VGfloat dxHalf, dyHalf;
			VGfloat x1Prime, y1Prime;
			VGfloat rx, ry;
			VGfloat x1PrimeSquare, y1PrimeSquare;
			VGfloat lambda;
			VGfloat rxSquare, rySquare;
			VGint sign;
			VGfloat sq, signedSq;
			VGfloat cxPrime, cyPrime;
			VGfloat theta1, thetaSpan;
			VGint segs;
			VGfloat theta, ax, ay, x, y;
			VGfloat controlX, controlY, anchorX, anchorY;
			VGfloat lastX, lastY;
			gctUINT bufferSize;


			/*******************************************************************
			** Converting.
			*/

			phi = RotAngle / 180.0f * vgvPI;
			cosPhi = gcmCOSF(phi);
			sinPhi = gcmSINF(phi);

			if (Relative)
			{
				dxHalf = - EndX / 2.0f;
				dyHalf = - EndY / 2.0f;
			}
			else
			{
				dxHalf = (coords->lastX - endX) / 2.0f;
				dyHalf = (coords->lastY - endY) / 2.0f;
			}

			x1Prime =  cosPhi * dxHalf + sinPhi * dyHalf;
			y1Prime = -sinPhi * dxHalf + cosPhi * dyHalf;

			rx = gcmFABSF(HorRadius);
			ry = gcmFABSF(VerRadius);

			x1PrimeSquare = x1Prime * x1Prime;
			y1PrimeSquare = y1Prime * y1Prime;

			lambda = x1PrimeSquare / (rx * rx) + y1PrimeSquare / (ry * ry);
			if (lambda > 1.0f)
			{
				rx *= gcmSQRTF(lambda);
				ry *= gcmSQRTF(lambda);
			}

			rxSquare = rx * rx;
			rySquare = ry * ry;

			sign     = (Large == CounterClockwise) ? -1 : 1;
			sq       = ( rxSquare * rySquare
					   - rxSquare * y1PrimeSquare
					   - rySquare * x1PrimeSquare
					   )
					   /
					   ( rxSquare * y1PrimeSquare
					   + rySquare * x1PrimeSquare
					   );
			signedSq = sign * ((sq < 0) ? 0 : gcmSQRTF(sq));
			cxPrime  = signedSq *  (rx * y1Prime / ry);
			cyPrime  = signedSq * -(ry * x1Prime / rx);

			theta1 = _Angle(1, 0, (x1Prime - cxPrime) / rx, (y1Prime - cyPrime) / ry);
			theta1 = gcmFMODF(theta1, 2 * vgvPI);

			thetaSpan = _Angle(( x1Prime - cxPrime) / rx, ( y1Prime - cyPrime) / ry,
							   (-x1Prime - cxPrime) / rx, (-y1Prime - cyPrime) / ry);

			if (!CounterClockwise && (thetaSpan > 0))
			{
				thetaSpan -= 2 * vgvPI;
			}
			else if (CounterClockwise && (thetaSpan < 0))
			{
				thetaSpan += 2 * vgvPI;
			}

			thetaSpan = gcmFMODF(thetaSpan, 2 * vgvPI);


			/*******************************************************************
			** Drawing.
			*/

			segs  = (VGint) (gcmCEILF(gcmFABSF(thetaSpan) / (45.0f / 180.0f * vgvPI)));
			gcmASSERT(segs > 0);

			theta = thetaSpan / segs;

			ax = coords->lastX - gcmCOSF(theta1) * rx;
			ay = coords->lastY - gcmSINF(theta1) * ry;

			/* Determine the segment command. */
			segmentCommand = Relative
				? gcvVGCMD_ARC_QUAD_REL
				: gcvVGCMD_ARC_QUAD;

			/* Determine the size of the buffer required. */
			bufferSize = (1 + 2 * 2) * gcmSIZEOF(gctFLOAT) * segs;

			/* Allocate a new subpath. */
			gcmERR_BREAK(vgsPATHWALKER_StartSubpath(
				Destination, bufferSize, gcePATHTYPE_FLOAT
				));

			/* Set initial last point. */
			lastX = coords->lastX;
			lastY = coords->lastY;

			while (segs-- > 0)
			{
				theta1 += theta;

				controlX = ax + gcmCOSF(theta1 - (theta / 2.0f)) * rx / gcmCOSF(theta / 2.0f);
				controlY = ay + gcmSINF(theta1 - (theta / 2.0f)) * ry / gcmCOSF(theta / 2.0f);

				anchorX = ax + gcmCOSF(theta1) * rx;
				anchorY = ay + gcmSINF(theta1) * ry;

				if (RotAngle != 0)
				{
					x = coords->lastX + cosPhi * (controlX - coords->lastX) - sinPhi * (controlY - coords->lastY);
					y = coords->lastY + sinPhi * (controlX - coords->lastX) + cosPhi * (controlY - coords->lastY);
					controlX = x;
					controlY = y;

					x = coords->lastX + cosPhi * (anchorX - coords->lastX) - sinPhi * (anchorY - coords->lastY);
					y = coords->lastY + sinPhi * (anchorX - coords->lastX) + cosPhi * (anchorY - coords->lastY);
					anchorX = x;
					anchorY = y;
				}

				if (segs == 0)
				{
					/* Use end point directly to avoid accumulated errors. */
					anchorX = endX;
					anchorY = endY;
				}

				/* Adjust relative coordinates. */
				if (Relative)
				{
					VGfloat nextLastX = anchorX;
					VGfloat nextLastY = anchorY;

					controlX -= lastX;
					controlY -= lastY;

					anchorX -= lastX;
					anchorY -= lastY;

					lastX = nextLastX;
					lastY = nextLastY;
				}

				/* Write the command. */
				gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
					Destination, segmentCommand
					));

				/* Set the coordinates. */
				Destination->set(Destination, controlX);
				Destination->set(Destination, controlY);
				Destination->set(Destination, anchorX);
				Destination->set(Destination, anchorY);
			}
		}

		/* Store ARC coordinates. */
		gcmERR_BREAK(vgsMEMORYMANAGER_Allocate(
			Destination->arcCoordinates,
			(gctPOINTER *) &arcCoords
			));

		arcCoords->large            = Large;
		arcCoords->counterClockwise = CounterClockwise;
		arcCoords->horRadius        = HorRadius;
		arcCoords->verRadius        = VerRadius;
		arcCoords->rotAngle         = RotAngle;
		arcCoords->endX             = EndX;
		arcCoords->endY             = EndY;

		/* Store the pointer to ARC coordinates. */
		Destination->currPathData->extraManager = Destination->arcCoordinates;
		Destination->currPathData->extra        = arcCoords;

		/* Update the control coordinates. */
		coords->lastX    = endX;
		coords->lastY    = endY;
		coords->controlX = endX;
		coords->controlY = endY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Roll back. */
	vgsPATHWALKER_Rollback(Destination);

	/* Return status. */
	return status;
}
