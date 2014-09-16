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

/* Segment normalization function name. */
#define vgmINTERPOL(Command) \
	_Interpolate_ ## Command

/* Segment transformation function definition. */
#define vgmDEFINEINTERPOLATE(Command) \
	static gceSTATUS vgmINTERPOL(Command) ( \
		vgmINTERPOLATEPARAMETERS \
		)


/*******************************************************************************
**
** _Interpolate
**
** Interpolate between two coordinates.
**
** INPUT:
**
**    StartCoordinate
**    EndCoordinate
**       Coordinates to interpolate between.
**
**    Amount
**       Interpolation value.
**
** OUTPUT:
**
**    Nothing.
*/

vgmINLINE static gctFLOAT _Interpolate(
	gctFLOAT StartCoordinate,
	gctFLOAT EndCoordinate,
	gctFLOAT Amount
	)
{
	gctFLOAT coordinate;

	coordinate
		= StartCoordinate * (1.0f - Amount)
		+ EndCoordinate * Amount;

	return coordinate;
}


/*******************************************************************************
**
** _InterpolateArc
**
** Generate an ARC buffer and append it to the specified destination.
**
** INPUT:
**
**    Destination
**       Pointer to the existing path reader / modificator.
**
**    StartCoordinate
**    EndCoordinate
**       Pointers to the coordinates to interpolate between.
**
**    Amount
**       Interpolation value.
**
**    CounterClockwise, RightCenter
**       ARC control flags.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _InterpolateArc(
	vgsPATHWALKER_PTR Destination,
	gctFLOAT_PTR StartCoordinates,
	gctFLOAT_PTR EndCoordinates,
	gctFLOAT Amount,
	gctBOOL CounterClockwise,
	gctBOOL Large
	)
{
	gceSTATUS status;

	do
	{
		vgsCONTEXT_PTR context;
		vgsPATHWALKER arc;
		gctFLOAT horRadius;
		gctFLOAT verRadius;
		gctFLOAT rotAngle;
		gctFLOAT endX;
		gctFLOAT endY;

		/* Determine the interpolated coordinates. */
		horRadius = _Interpolate(
			StartCoordinates[0], EndCoordinates[0], Amount
			);

		verRadius = _Interpolate(
			StartCoordinates[1], EndCoordinates[1], Amount
			);

		rotAngle = _Interpolate(
			StartCoordinates[2], EndCoordinates[2], Amount
			);

		endX = _Interpolate(
			StartCoordinates[3], EndCoordinates[3], Amount
			);

		endY = _Interpolate(
			StartCoordinates[4], EndCoordinates[4], Amount
			);

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
			horRadius, verRadius, rotAngle, endX, endY,
			CounterClockwise, Large, gcvFALSE
			));

		/* Append to the existing buffer. */
		vgsPATHWALKER_AppendData(
			Destination, &arc, 1, 5
			);

		/* Set the ARC present flag. */
		Destination->path->hasArcs = gcvTRUE;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
********************************* Invalid Entry ********************************
\******************************************************************************/

vgmDEFINEINTERPOLATE(Invalid)
{
	/* This function should never be called. */
	gcmASSERT(gcvFALSE);
	return gcvSTATUS_GENERIC_IO;
}


/******************************************************************************\
******************************** gcvVGCMD_CLOSE ********************************
\******************************************************************************/

vgmDEFINEINTERPOLATE(gcvVGCMD_CLOSE)
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

vgmDEFINEINTERPOLATE(gcvVGCMD_MOVE)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT moveToX, moveToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_MOVE
			));

		/* Determine the interpolated coordinates. */
		moveToX = _Interpolate(
			StartCoordinates[0], EndCoordinates[0], Amount
			);

		moveToY = _Interpolate(
			StartCoordinates[1], EndCoordinates[1], Amount
			);

		/* Set the command coordinates. */
		Destination->set(Destination, moveToX);
		Destination->set(Destination, moveToY);

		/* Update the control coordinates. */
		coords->startX   = moveToX;
		coords->startY   = moveToY;
		coords->lastX    = moveToX;
		coords->lastY    = moveToY;
		coords->controlX = moveToX;
		coords->controlY = moveToY;

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

vgmDEFINEINTERPOLATE(gcvVGCMD_LINE)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT lineToX, lineToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_LINE
			));

		/* Determine the interpolated coordinates. */
		lineToX = _Interpolate(
			StartCoordinates[0], EndCoordinates[0], Amount
			);

		lineToY = _Interpolate(
			StartCoordinates[1], EndCoordinates[1], Amount
			);

		/* Set the command coordinates. */
		Destination->set(Destination, lineToX);
		Destination->set(Destination, lineToY);

		/* Update the control coordinates. */
		coords->lastX    = lineToX;
		coords->lastY    = lineToY;
		coords->controlX = lineToX;
		coords->controlY = lineToY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
******************************** gcvVGCMD_CUBIC ********************************
\******************************************************************************/

vgmDEFINEINTERPOLATE(gcvVGCMD_CUBIC)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT controlX1, controlY1;
	gctFLOAT controlX2, controlY2;
	gctFLOAT cubicToX, cubicToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_CUBIC
			));

		/* Determine the interpolated coordinates. */
		controlX1 = _Interpolate(
			StartCoordinates[0], EndCoordinates[0], Amount
			);

		controlY1 = _Interpolate(
			StartCoordinates[1], EndCoordinates[1], Amount
			);

		controlX2 = _Interpolate(
			StartCoordinates[2], EndCoordinates[2], Amount
			);

		controlY2 = _Interpolate(
			StartCoordinates[3], EndCoordinates[3], Amount
			);

		cubicToX = _Interpolate(
			StartCoordinates[4], EndCoordinates[4], Amount
			);

		cubicToY = _Interpolate(
			StartCoordinates[5], EndCoordinates[5], Amount
			);

		/* Set the command coordinates. */
		Destination->set(Destination, controlX1);
		Destination->set(Destination, controlY1);
		Destination->set(Destination, controlX2);
		Destination->set(Destination, controlY2);
		Destination->set(Destination, cubicToX);
		Destination->set(Destination, cubicToY);

		/* Update the control coordinates. */
		coords->lastX    = cubicToX;
		coords->lastY    = cubicToY;
		coords->controlX = controlX2;
		coords->controlY = controlY2;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
******************************* gcvVGCMD_SCCWARC *******************************
\******************************************************************************/

vgmDEFINEINTERPOLATE(gcvVGCMD_SCCWARC)
{
	/* Generate the ARC. */
	return _InterpolateArc(
		Destination,
		StartCoordinates,
		EndCoordinates,
		Amount,
		gcvTRUE,
		gcvFALSE
		);
}


/******************************************************************************\
******************************** gcvVGCMD_SCWARC *******************************
\******************************************************************************/

vgmDEFINEINTERPOLATE(gcvVGCMD_SCWARC)
{
	/* Generate the ARC. */
	return _InterpolateArc(
		Destination,
		StartCoordinates,
		EndCoordinates,
		Amount,
		gcvFALSE,
		gcvFALSE
		);
}


/******************************************************************************\
******************************* gcvVGCMD_LCCWARC *******************************
\******************************************************************************/

vgmDEFINEINTERPOLATE(gcvVGCMD_LCCWARC)
{
	/* Generate the ARC. */
	return _InterpolateArc(
		Destination,
		StartCoordinates,
		EndCoordinates,
		Amount,
		gcvTRUE,
		gcvTRUE
		);
}


/******************************************************************************\
******************************** gcvVGCMD_LCWARC *******************************
\******************************************************************************/

vgmDEFINEINTERPOLATE(gcvVGCMD_LCWARC)
{
	/* Generate the ARC. */
	return _InterpolateArc(
		Destination,
		StartCoordinates,
		EndCoordinates,
		Amount,
		gcvFALSE,
		gcvTRUE
		);
}


/*******************************************************************************
**
** vgfGetInterpolationArray
**
** Returns the pointer to the array of coordinate interpolation functions.
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

void vgfGetInterpolationArray(
	vgtINTERPOLATEFUNCTION ** Array,
	gctUINT_PTR Count
	)
{
	static vgtINTERPOLATEFUNCTION _interpolateSegment[] =
	{
		vgmINTERPOL(Invalid),                  /*   0: gcvVGCMD_END             */
		vgmINTERPOL(gcvVGCMD_CLOSE),           /*   1: gcvVGCMD_CLOSE           */
		vgmINTERPOL(gcvVGCMD_MOVE),            /*   2: gcvVGCMD_MOVE            */
		vgmINTERPOL(Invalid),                  /*   3: gcvVGCMD_MOVE_REL        */
		vgmINTERPOL(gcvVGCMD_LINE),            /*   4: gcvVGCMD_LINE            */
		vgmINTERPOL(Invalid),                  /*   5: gcvVGCMD_LINE_REL        */
		vgmINTERPOL(Invalid),                  /*   6: gcvVGCMD_QUAD            */
		vgmINTERPOL(Invalid),                  /*   7: gcvVGCMD_QUAD_REL        */
		vgmINTERPOL(gcvVGCMD_CUBIC),           /*   8: gcvVGCMD_CUBIC           */
		vgmINTERPOL(Invalid),                  /*   9: gcvVGCMD_CUBIC_REL       */
		vgmINTERPOL(Invalid),                  /*  10: gcvVGCMD_BREAK           */
		vgmINTERPOL(Invalid),                  /*  11: **** R E S E R V E D *****/
		vgmINTERPOL(Invalid),                  /*  12: **** R E S E R V E D *****/
		vgmINTERPOL(Invalid),                  /*  13: **** R E S E R V E D *****/
		vgmINTERPOL(Invalid),                  /*  14: **** R E S E R V E D *****/
		vgmINTERPOL(Invalid),                  /*  15: **** R E S E R V E D *****/
		vgmINTERPOL(Invalid),                  /*  16: **** R E S E R V E D *****/
		vgmINTERPOL(Invalid),                  /*  17: **** R E S E R V E D *****/
		vgmINTERPOL(Invalid),                  /*  18: **** R E S E R V E D *****/
		vgmINTERPOL(gcvVGCMD_SCCWARC),         /*  19: gcvVGCMD_SCCWARC         */
		vgmINTERPOL(Invalid),                  /*  20: **** R E S E R V E D *****/
		vgmINTERPOL(gcvVGCMD_SCWARC),          /*  21: gcvVGCMD_SCWARC          */
		vgmINTERPOL(Invalid),                  /*  22: **** R E S E R V E D *****/
		vgmINTERPOL(gcvVGCMD_LCCWARC),         /*  23: gcvVGCMD_LCCWARC         */
		vgmINTERPOL(Invalid),                  /*  24: **** R E S E R V E D *****/
		vgmINTERPOL(gcvVGCMD_LCWARC),          /*  25: gcvVGCMD_LCWARC          */
	};

	* Array = _interpolateSegment;
	* Count = gcmCOUNTOF(_interpolateSegment);
}
