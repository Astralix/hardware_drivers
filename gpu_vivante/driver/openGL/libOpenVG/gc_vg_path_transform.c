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

/* Segment transformation function name. */
#define vgmXFORM(Command) \
	_Transform_ ## Command

/* Segment transformation function declaration and definition. */
#define vgmDEFINETRANSFORM(Command) \
	static gceSTATUS vgmXFORM(Command) (vgmTRANSFORMPARAMETERS)


/******************************************************************************\
********************************* Invalid Entry ********************************
\******************************************************************************/

vgmDEFINETRANSFORM(Invalid)
{
	/* This function should never be called. */
	gcmASSERT(gcvFALSE);
	return gcvSTATUS_GENERIC_IO;
}


/******************************************************************************\
******************************** gcvVGCMD_CLOSE ********************************
\******************************************************************************/

vgmDEFINETRANSFORM(gcvVGCMD_CLOSE)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_CLOSE
			));

		/* Update the control coordinates. */
		coords->lastX    = coords->startX;
		coords->lastY    = coords->startY;
		coords->controlX = coords->startX;
		coords->controlY = coords->startY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
******************************** gcvVGCMD_MOVE *********************************
\******************************************************************************/

vgmDEFINETRANSFORM(gcvVGCMD_MOVE)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 current, transformed;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_MOVE
			));

		/* Extract the command coordinates. */
		current[0] = Source->get(Source);
		current[1] = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix3x2(current, Transform, transformed);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, transformed[0]);
		Destination->set(Destination, transformed[1]);

		/* Update the control coordinates. */
		coords->startX   = transformed[0];
		coords->startY   = transformed[1];
		coords->lastX    = transformed[0];
		coords->lastY    = transformed[1];
		coords->controlX = transformed[0];
		coords->controlY = transformed[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINETRANSFORM(gcvVGCMD_MOVE_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 current, transformed;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_MOVE_REL
			));

		/* Extract the command coordinates. */
		current[0] = Source->get(Source);
		current[1] = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix2x2(current, Transform, transformed);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, transformed[0]);
		Destination->set(Destination, transformed[1]);

		/* Determine the absolute coordinates. */
		transformed[0] += coords->lastX;
		transformed[1] += coords->lastY;

		/* Update the control coordinates. */
		coords->startX   = transformed[0];
		coords->startY   = transformed[1];
		coords->lastX    = transformed[0];
		coords->lastY    = transformed[1];
		coords->controlX = transformed[0];
		coords->controlY = transformed[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
******************************** gcvVGCMD_LINE *********************************
\******************************************************************************/

vgmDEFINETRANSFORM(gcvVGCMD_LINE)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 current, transformed;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_LINE
			));

		/* Extract the command coordinates. */
		current[0] = Source->get(Source);
		current[1] = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix3x2(current, Transform, transformed);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, transformed[0]);
		Destination->set(Destination, transformed[1]);

		/* Update the control coordinates. */
		coords->lastX    = transformed[0];
		coords->lastY    = transformed[1];
		coords->controlX = transformed[0];
		coords->controlY = transformed[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINETRANSFORM(gcvVGCMD_LINE_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 current, transformed;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_LINE_REL
			));

		/* Extract the command coordinates. */
		current[0] = Source->get(Source);
		current[1] = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix2x2(current, Transform, transformed);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, transformed[0]);
		Destination->set(Destination, transformed[1]);

		/* Determine the absolute coordinates. */
		transformed[0] += coords->lastX;
		transformed[1] += coords->lastY;

		/* Update the control coordinates. */
		coords->lastX    = transformed[0];
		coords->lastY    = transformed[1];
		coords->controlX = transformed[0];
		coords->controlY = transformed[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
********************************* gcvVGCMD_QUAD ********************************
\******************************************************************************/

vgmDEFINETRANSFORM(gcvVGCMD_QUAD)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 currentControl, transformedControl;
	vgtFLOATVECTOR2 currentQuadTo, transformedQuadTo;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_QUAD
			));

		/* Extract the command coordinates. */
		currentControl[0] = Source->get(Source);
		currentControl[1] = Source->get(Source);
		currentQuadTo[0]  = Source->get(Source);
		currentQuadTo[1]  = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix3x2(
			currentControl, Transform, transformedControl
			);

		vgfMultiplyVector2ByMatrix3x2(
			currentQuadTo, Transform, transformedQuadTo
			);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, transformedControl[0]);
		Destination->set(Destination, transformedControl[1]);
		Destination->set(Destination, transformedQuadTo[0]);
		Destination->set(Destination, transformedQuadTo[1]);

		/* Update the control coordinates. */
		coords->lastX    = transformedQuadTo[0];
		coords->lastY    = transformedQuadTo[1];
		coords->controlX = transformedControl[0];
		coords->controlY = transformedControl[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINETRANSFORM(gcvVGCMD_QUAD_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 currentControl, transformedControl;
	vgtFLOATVECTOR2 currentQuadTo, transformedQuadTo;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_QUAD_REL
			));

		/* Extract the command coordinates. */
		currentControl[0] = Source->get(Source);
		currentControl[1] = Source->get(Source);
		currentQuadTo[0]  = Source->get(Source);
		currentQuadTo[1]  = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix2x2(
			currentControl, Transform, transformedControl
			);

		vgfMultiplyVector2ByMatrix2x2(
			currentQuadTo, Transform, transformedQuadTo
			);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, transformedControl[0]);
		Destination->set(Destination, transformedControl[1]);
		Destination->set(Destination, transformedQuadTo[0]);
		Destination->set(Destination, transformedQuadTo[1]);

		/* Determine the absolute coordinates. */
		transformedControl[0] += coords->lastX;
		transformedControl[1] += coords->lastY;
		transformedQuadTo[0]  += coords->lastX;
		transformedQuadTo[1]  += coords->lastY;

		/* Update the last coordinates. */
		coords->lastX    = transformedQuadTo[0];
		coords->lastY    = transformedQuadTo[1];
		coords->controlX = transformedControl[0];
		coords->controlY = transformedControl[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
********************************* gcvVGCMD_CUBIC *******************************
\******************************************************************************/

vgmDEFINETRANSFORM(gcvVGCMD_CUBIC)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 currentControl1, transformedControl1;
	vgtFLOATVECTOR2 currentControl2, transformedControl2;
	vgtFLOATVECTOR2 currentCubicTo, transformedCubicTo;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_CUBIC
			));

		/* Extract the command coordinates. */
		currentControl1[0] = Source->get(Source);
		currentControl1[1] = Source->get(Source);
		currentControl2[0] = Source->get(Source);
		currentControl2[1] = Source->get(Source);
		currentCubicTo[0]  = Source->get(Source);
		currentCubicTo[1]  = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix3x2(
			currentControl1, Transform, transformedControl1
			);

		vgfMultiplyVector2ByMatrix3x2(
			currentControl2, Transform, transformedControl2
			);

		vgfMultiplyVector2ByMatrix3x2(
			currentCubicTo, Transform, transformedCubicTo
			);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, transformedControl1[0]);
		Destination->set(Destination, transformedControl1[1]);
		Destination->set(Destination, transformedControl2[0]);
		Destination->set(Destination, transformedControl2[1]);
		Destination->set(Destination, transformedCubicTo[0]);
		Destination->set(Destination, transformedCubicTo[1]);

		/* Update the control coordinates. */
		coords->lastX    = transformedCubicTo[0];
		coords->lastY    = transformedCubicTo[1];
		coords->controlX = transformedControl2[0];
		coords->controlY = transformedControl2[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINETRANSFORM(gcvVGCMD_CUBIC_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 currentControl1, transformedControl1;
	vgtFLOATVECTOR2 currentControl2, transformedControl2;
	vgtFLOATVECTOR2 currentCubicTo, transformedCubicTo;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_CUBIC_REL
			));

		/* Extract the command coordinates. */
		currentControl1[0] = Source->get(Source);
		currentControl1[1] = Source->get(Source);
		currentControl2[0] = Source->get(Source);
		currentControl2[1] = Source->get(Source);
		currentCubicTo[0]  = Source->get(Source);
		currentCubicTo[1]  = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix2x2(
			currentControl1, Transform, transformedControl1
			);

		vgfMultiplyVector2ByMatrix2x2(
			currentControl2, Transform, transformedControl2
			);

		vgfMultiplyVector2ByMatrix2x2(
			currentCubicTo, Transform, transformedCubicTo
			);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, transformedControl1[0]);
		Destination->set(Destination, transformedControl1[1]);
		Destination->set(Destination, transformedControl2[0]);
		Destination->set(Destination, transformedControl2[1]);
		Destination->set(Destination, transformedCubicTo[0]);
		Destination->set(Destination, transformedCubicTo[1]);

		/* Determine the absolute coordinates. */
		transformedControl2[0] += coords->lastX;
		transformedControl2[1] += coords->lastY;
		transformedCubicTo[0]  += coords->lastX;
		transformedCubicTo[1]  += coords->lastY;

		/* Update the control coordinates. */
		coords->lastX    = transformedCubicTo[0];
		coords->lastY    = transformedCubicTo[1];
		coords->controlX = transformedControl2[0];
		coords->controlY = transformedControl2[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
****************************** gcvVGCMD_SQUAD_EMUL *****************************
\******************************************************************************/

vgmDEFINETRANSFORM(gcvVGCMD_SQUAD_EMUL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 currentControl;
	vgtFLOATVECTOR2 currentQuadTo, transformedQuadTo;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_SQUAD_EMUL
			));

		/* Compute the control point. */
		currentControl[0] = 2.0f * coords->lastX - coords->controlX;
		currentControl[1] = 2.0f * coords->lastY - coords->controlY;

		/* Extract the command coordinates. */
		                   Source->get(Source);
		                   Source->get(Source);
		currentQuadTo[0] = Source->get(Source);
		currentQuadTo[1] = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix3x2(
			currentQuadTo, Transform, transformedQuadTo
			);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, currentControl[0]);
		Destination->set(Destination, currentControl[1]);
		Destination->set(Destination, transformedQuadTo[0]);
		Destination->set(Destination, transformedQuadTo[1]);

		/* Update the control coordinates. */
		coords->lastX    = transformedQuadTo[0];
		coords->lastY    = transformedQuadTo[1];
		coords->controlX = currentControl[0];
		coords->controlY = currentControl[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINETRANSFORM(gcvVGCMD_SQUAD_EMUL_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 currentControl;
	vgtFLOATVECTOR2 currentQuadTo, transformedQuadTo;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_SQUAD_EMUL_REL
			));

		/* Compute the control point. */
		currentControl[0] = coords->lastX - coords->controlX;
		currentControl[1] = coords->lastY - coords->controlY;

		/* Extract the command coordinates. */
		                   Source->get(Source);
		                   Source->get(Source);
		currentQuadTo[0] = Source->get(Source);
		currentQuadTo[1] = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix2x2(
			currentQuadTo, Transform, transformedQuadTo
			);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, currentControl[0]);
		Destination->set(Destination, currentControl[1]);
		Destination->set(Destination, transformedQuadTo[0]);
		Destination->set(Destination, transformedQuadTo[1]);

		/* Determine the absolute coordinates. */
		currentControl[0]    += coords->lastX;
		currentControl[1]    += coords->lastY;
		transformedQuadTo[0] += coords->lastX;
		transformedQuadTo[1] += coords->lastY;

		/* Update the control coordinates. */
		coords->lastX    = transformedQuadTo[0];
		coords->lastY    = transformedQuadTo[1];
		coords->controlX = currentControl[0];
		coords->controlY = currentControl[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
***************************** gcvVGCMD_SCUBIC_EMUL *****************************
\******************************************************************************/

vgmDEFINETRANSFORM(gcvVGCMD_SCUBIC_EMUL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 currentControl1;
	vgtFLOATVECTOR2 currentControl2, transformedControl2;
	vgtFLOATVECTOR2 currentCubicTo, transformedCubicTo;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_SCUBIC_EMUL
			));

		/* Compute the control point. */
		currentControl1[0] = 2.0f * coords->lastX - coords->controlX;
		currentControl1[1] = 2.0f * coords->lastY - coords->controlY;

		/* Extract the command coordinates. */
		                     Source->get(Source);
		                     Source->get(Source);
		currentControl2[0] = Source->get(Source);
		currentControl2[1] = Source->get(Source);
		currentCubicTo[0]  = Source->get(Source);
		currentCubicTo[1]  = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix3x2(
			currentControl2, Transform, transformedControl2
			);

		vgfMultiplyVector2ByMatrix3x2(
			currentCubicTo, Transform, transformedCubicTo
			);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, currentControl1[0]);
		Destination->set(Destination, currentControl1[1]);
		Destination->set(Destination, transformedControl2[0]);
		Destination->set(Destination, transformedControl2[1]);
		Destination->set(Destination, transformedCubicTo[0]);
		Destination->set(Destination, transformedCubicTo[1]);

		/* Update the control coordinates. */
		coords->lastX    = transformedCubicTo[0];
		coords->lastY    = transformedCubicTo[1];
		coords->controlX = transformedControl2[0];
		coords->controlY = transformedControl2[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINETRANSFORM(gcvVGCMD_SCUBIC_EMUL_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgtFLOATVECTOR2 currentControl1;
	vgtFLOATVECTOR2 currentControl2, transformedControl2;
	vgtFLOATVECTOR2 currentCubicTo, transformedCubicTo;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_SCUBIC_EMUL_REL
			));

		/* Compute the control point. */
		currentControl1[0] = coords->lastX - coords->controlX;
		currentControl1[1] = coords->lastY - coords->controlY;

		/* Extract the command coordinates. */
		                     Source->get(Source);
		                     Source->get(Source);
		currentControl2[0] = Source->get(Source);
		currentControl2[1] = Source->get(Source);
		currentCubicTo[0]  = Source->get(Source);
		currentCubicTo[1]  = Source->get(Source);

		/* Transform with the specified matrix. */
		vgfMultiplyVector2ByMatrix2x2(
			currentControl2, Transform, transformedControl2
			);

		vgfMultiplyVector2ByMatrix2x2(
			currentCubicTo, Transform, transformedCubicTo
			);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, currentControl1[0]);
		Destination->set(Destination, currentControl1[1]);
		Destination->set(Destination, transformedControl2[0]);
		Destination->set(Destination, transformedControl2[1]);
		Destination->set(Destination, transformedCubicTo[0]);
		Destination->set(Destination, transformedCubicTo[1]);

		/* Determine the absolute coordinates. */
		transformedControl2[0] += coords->lastX;
		transformedControl2[1] += coords->lastY;
		transformedCubicTo[0]  += coords->lastX;
		transformedCubicTo[1]  += coords->lastY;

		/* Update the control coordinates. */
		coords->lastX    = transformedCubicTo[0];
		coords->lastY    = transformedCubicTo[1];
		coords->controlX = transformedControl2[0];
		coords->controlY = transformedControl2[1];

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
********************************** gcvVGCMD_ARC ********************************
\******************************************************************************/

static gceSTATUS _TransformArc(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source,
	vgsMATRIX_PTR Transform,
	gctBOOL Relative
	)
{
	gceSTATUS status;

	do
	{
		vgsCONTEXT_PTR context;
		vgsARCCOORDINATES_PTR arcCoords;
		gctFLOAT horRadius, verRadius, rotAngle;
		vgtFLOATVECTOR2 current, transformed;
		gctBOOL transformedDirection;
#if gcdENABLE_PERFORMANCE_PRIOR
		gctFIXED elps00, elps01;
		gctFIXED elps10, elps11;

		gctFIXED fixedRotAngle;
		gctFIXED fixedHorRadius;
		gctFIXED fixedVerRadius;
#else
		gctFLOAT elps00, elps01;
		gctFLOAT elps10, elps11;
#endif
		vgtFLOATVECTOR2 p, q;
		gctFLOAT pDot, qDot;
		vgtFLOATVECTOR2 h, hp;
		gctFLOAT hLen, hpLen;
		vgtFLOATVECTOR2 v;
		gctFLOAT vDot;
		vgsPATHWALKER arc;


		/***********************************************************************
		** Transform the original ARC coordinates.
		*/

		/* Get the coordinates. */
		arcCoords = (vgsARCCOORDINATES_PTR) Source->currPathData->extra;
		gcmASSERT(arcCoords != gcvNULL);

		/* Set the radiuses. */
		horRadius = arcCoords->horRadius;
		verRadius = arcCoords->verRadius;

		/* Set end ARC coordinates. */
		current[0] = arcCoords->endX;
		current[1] = arcCoords->endY;

		/* Convert the rotation angle to radians. */
		rotAngle = arcCoords->rotAngle * vgvPI / 180.0f;

		/* The matrix maps from the unit circle to transformed ellipse */
#if gcdENABLE_PERFORMANCE_PRIOR
		fixedRotAngle	= vgmFLOAT_TO_FIXED_SPECIAL(rotAngle);
		fixedHorRadius = vgmFLOAT_TO_FIXED_SPECIAL(horRadius);
		fixedVerRadius = vgmFLOAT_TO_FIXED_SPECIAL(verRadius);

		elps00 = vgmFIXED_MUL(cosx(fixedRotAngle), fixedHorRadius);
		elps01 = vgmFIXED_MUL(vgmINT_TO_FIXED(-1), vgmFIXED_MUL(sinx(fixedRotAngle),  fixedVerRadius));
		elps10 = vgmFIXED_MUL(sinx(fixedRotAngle), fixedHorRadius);
		elps11 = vgmFIXED_MUL(cosx(fixedRotAngle), fixedVerRadius);

		p[0] = vgmFIXED_TO_FLOAT( vgmFIXED_MUL(vgmMAT(Transform, 0, 0), elps00) + vgmFIXED_MUL(vgmMAT(Transform, 0, 1), elps10) );
		p[1] = vgmFIXED_TO_FLOAT( vgmFIXED_MUL(vgmMAT(Transform, 1, 0), elps00) + vgmFIXED_MUL(vgmMAT(Transform, 1, 1), elps10) );
		q[0] = vgmFIXED_TO_FLOAT( vgmFIXED_MUL(vgmMAT(Transform, 1, 0), elps01) + vgmFIXED_MUL(vgmMAT(Transform, 1, 1), elps11) );
		q[1] = vgmFIXED_TO_FLOAT( vgmFIXED_MUL(vgmINT_TO_FIXED(-1), vgmFIXED_MUL(vgmMAT(Transform, 0, 0), elps01)) - vgmFIXED_MUL(vgmMAT(Transform, 0, 1), elps11) );
#else
		elps00 = (gctFLOAT)  cos(rotAngle) * horRadius;
		elps01 = (gctFLOAT) -sin(rotAngle) * verRadius;
		elps10 = (gctFLOAT)  sin(rotAngle) * horRadius;
		elps11 = (gctFLOAT)  cos(rotAngle) * verRadius;

		p[0] =   vgmMAT(Transform, 0, 0) * elps00 + vgmMAT(Transform, 0, 1) * elps10;
		p[1] =   vgmMAT(Transform, 1, 0) * elps00 + vgmMAT(Transform, 1, 1) * elps10;
		q[0] =   vgmMAT(Transform, 1, 0) * elps01 + vgmMAT(Transform, 1, 1) * elps11;
		q[1] = - vgmMAT(Transform, 0, 0) * elps01 - vgmMAT(Transform, 0, 1) * elps11;
#endif
		/* Compute the squared length of the vectors. */
		pDot = p[0] * p[0] + p[1] * p[1];
		qDot = q[0] * q[0] + q[1] * q[1];

		/* Make vector P the longest. */
		if (pDot < qDot)
		{
			gctFLOAT t0, t1;

			t0 = p[0];
			t1 = p[1];

			p[0] = q[0];
			p[1] = q[1];

			q[0] = t0;
			q[1] = t1;
		}

		h[0] = (p[0] + q[0]) * 0.5f;
		h[1] = (p[1] + q[1]) * 0.5f;

		hp[0] = (p[0] - q[0]) * 0.5f;
		hp[1] = (p[1] - q[1]) * 0.5f;

		hLen  = (gctFLOAT) sqrt(h [0] * h [0] + h [1] * h [1]);
		hpLen = (gctFLOAT) sqrt(hp[0] * hp[0] + hp[1] * hp[1]);

		horRadius = hLen + hpLen;
		verRadius = hLen - hpLen;

		v[0] = hpLen * h[0] + hLen * hp[0];
		v[1] = hpLen * h[1] + hLen * hp[1];

		vDot = v[0] * v[0] + v[1] * v[1];

		if (vDot == 0.0f)
		{
			rotAngle = 0.0f;
		}
		else
		{
			gctFLOAT norm = 1.0f / (gctFLOAT) sqrt(vDot);

			v[0] *= norm;
			v[1] *= norm;

			rotAngle = (gctFLOAT) acos(v[0]);

			if (v[1] < 0.0f)
			{
				rotAngle = 2.0f * vgvPI - rotAngle;
			}
		}

		if (pDot < qDot)
		{
			rotAngle += vgvPI * 0.5f;
		}

		/* Convert radians to degrees. */
		rotAngle = rotAngle * 180.0f / vgvPI;

		/* Relative coordinates? */
		if (Relative)
		{
			/* Transform with the specified matrix. */
			vgfMultiplyVector2ByMatrix2x2(current, Transform, transformed);
		}
		else
		{
			/* Transform with the specified matrix. */
			vgfMultiplyVector2ByMatrix3x2(current, Transform, transformed);
		}

		/* Flip winding if the determinant is negative. */
		transformedDirection = (vgfGetDeterminant(Transform) < 0.0f)
			? !arcCoords->counterClockwise
			:  arcCoords->counterClockwise;

		/* Skip the ARC path in the source. */
		vgsPATHWALKER_SeekToEnd(Source);


		/***********************************************************************
		** Generate the ARC path.
		*/

		/* Close the current subpath. */
		vgsPATHWALKER_CloseSubpath(Destination);

		/* Get a shortcut to the context. */
		context = Destination->context;

		/* Prepare for writing the destination. */
		vgsPATHWALKER_InitializeWriter(
			context, context->pathStorage, &arc, Destination->path
			);

		/* Generate the ARC buffer. */
		gcmERR_BREAK(vgfConvertArc(
			&arc,
			horRadius, verRadius, rotAngle,
			transformed[0], transformed[1],
			transformedDirection,
			arcCoords->large,
			Relative
			));

		/* Append to the existing buffer. */
		vgsPATHWALKER_AppendData(
			Destination, &arc, 1, 5
			);

		/* Set the ARC present flag. */
		Destination->path->hasArcs = gcvTRUE;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINETRANSFORM(gcvVGCMD_ARC)
{
	return _TransformArc(Destination, Source, Transform, gcvFALSE);
}

vgmDEFINETRANSFORM(gcvVGCMD_ARC_REL)
{
	return _TransformArc(Destination, Source, Transform, gcvTRUE);
}


/*******************************************************************************
**
** vgfGetTransformArray
**
** Returns the pointer to the array of coordinate transformation functions.
**
** INPUT:
**
**    Nothing.
**
** OUTPUT:
**
**    Array
**       Pointer to the function array.
**
**    Count
**       Pointer to the number of functions in the array.
*/

void vgfGetTransformArray(
	vgtTRANSFORMFUNCTION ** Array,
	gctUINT_PTR Count
	)
{
	static vgtTRANSFORMFUNCTION _transformSegment[] =
	{
		vgmXFORM(Invalid),                   /*   0: gcvVGCMD_END             */
		vgmXFORM(gcvVGCMD_CLOSE),            /*   1: gcvVGCMD_CLOSE           */
		vgmXFORM(gcvVGCMD_MOVE),             /*   2: gcvVGCMD_MOVE            */
		vgmXFORM(gcvVGCMD_MOVE_REL),         /*   3: gcvVGCMD_MOVE_REL        */
		vgmXFORM(gcvVGCMD_LINE),             /*   4: gcvVGCMD_LINE            */
		vgmXFORM(gcvVGCMD_LINE_REL),         /*   5: gcvVGCMD_LINE_REL        */
		vgmXFORM(gcvVGCMD_QUAD),             /*   6: gcvVGCMD_QUAD            */
		vgmXFORM(gcvVGCMD_QUAD_REL),         /*   7: gcvVGCMD_QUAD_REL        */
		vgmXFORM(gcvVGCMD_CUBIC),            /*   8: gcvVGCMD_CUBIC           */
		vgmXFORM(gcvVGCMD_CUBIC_REL),        /*   9: gcvVGCMD_CUBIC_REL       */
		vgmXFORM(Invalid),                   /*  10: gcvVGCMD_BREAK           */
		vgmXFORM(Invalid),                   /*  11: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  12: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  13: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  14: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  15: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  16: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  17: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  18: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  19: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  20: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  21: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  22: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  23: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  24: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  25: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  26: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  27: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  28: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  29: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  30: **** R E S E R V E D *****/
		vgmXFORM(Invalid),                   /*  31: **** R E S E R V E D *****/

		vgmXFORM(Invalid),                   /*  32: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  33: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  34: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  35: ***** I N V A L I D ******/
		vgmXFORM(gcvVGCMD_LINE),             /*  36: gcvVGCMD_HLINE_EMUL      */
		vgmXFORM(gcvVGCMD_LINE_REL),         /*  37: gcvVGCMD_HLINE_EMUL_REL  */
		vgmXFORM(Invalid),                   /*  38: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  39: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  40: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  41: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  42: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  43: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  44: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  45: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  46: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  47: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  48: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  49: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  50: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  51: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  52: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  53: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  54: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  55: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  56: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  57: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  58: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  59: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  60: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  61: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  62: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  63: ***** I N V A L I D ******/

		vgmXFORM(Invalid),                   /*  64: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  65: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  66: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  67: ***** I N V A L I D ******/
		vgmXFORM(gcvVGCMD_LINE),             /*  68: gcvVGCMD_VLINE_EMUL      */
		vgmXFORM(gcvVGCMD_LINE_REL),         /*  69: gcvVGCMD_VLINE_EMUL_REL  */
		vgmXFORM(Invalid),                   /*  70: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  71: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  72: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  73: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  74: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  75: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  76: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  77: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  78: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  79: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  80: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  81: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  82: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  83: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  84: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  85: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  86: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  87: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  88: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  89: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  90: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  91: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  92: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  93: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  94: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  95: ***** I N V A L I D ******/

		vgmXFORM(Invalid),                   /*  96: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  97: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  98: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /*  99: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 100: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 101: ***** I N V A L I D ******/
		vgmXFORM(gcvVGCMD_SQUAD_EMUL),       /* 102: gcvVGCMD_SQUAD_EMUL      */
		vgmXFORM(gcvVGCMD_SQUAD_EMUL_REL),   /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
		vgmXFORM(gcvVGCMD_SCUBIC_EMUL),      /* 104: gcvVGCMD_SCUBIC_EMUL     */
		vgmXFORM(gcvVGCMD_SCUBIC_EMUL_REL),  /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
		vgmXFORM(Invalid),                   /* 106: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 107: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 108: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 109: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 110: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 111: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 112: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 113: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 114: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 115: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 116: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 117: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 118: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 119: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 120: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 121: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 122: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 123: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 124: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 125: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 126: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 127: ***** I N V A L I D ******/

		vgmXFORM(Invalid),                   /* 128: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 129: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 130: ***** I N V A L I D ******/
		vgmXFORM(Invalid),                   /* 131: ***** I N V A L I D ******/
		vgmXFORM(gcvVGCMD_ARC),              /* 132: gcvVGCMD_ARC_LINE        */
		vgmXFORM(gcvVGCMD_ARC_REL),          /* 133: gcvVGCMD_ARC_LINE_REL    */
		vgmXFORM(gcvVGCMD_ARC),              /* 134: gcvVGCMD_ARC_QUAD        */
		vgmXFORM(gcvVGCMD_ARC_REL)           /* 135: gcvVGCMD_ARC_QUAD_REL    */
	};

	* Array = _transformSegment;
	* Count = gcmCOUNTOF(_transformSegment);
}
