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

/* Segment append function name. */
#define vgmAPPEND(Command) \
	_Append_ ## Command

/* Segment append function definition. */
#define vgmDEFINEAPPEND(Command) \
	static gceSTATUS vgmAPPEND(Command) ( \
		vgmSEGMENTHANDLERPARAMETERS \
		)


/******************************************************************************\
********************************* Invalid Entry ********************************
\******************************************************************************/

vgmDEFINEAPPEND(Invalid)
{
	/* This function should never be called. */
	gcmASSERT(gcvFALSE);
	return gcvSTATUS_GENERIC_IO;
}


/******************************************************************************\
******************************** gcvVGCMD_CLOSE ********************************
\******************************************************************************/

vgmDEFINEAPPEND(gcvVGCMD_CLOSE)
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

vgmDEFINEAPPEND(gcvVGCMD_MOVE)
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

		/* Extract the command coordinates. */
		moveToX = Source->get(Source);
		moveToY = Source->get(Source);

		/* Store the coordinates in the destination. */
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

vgmDEFINEAPPEND(gcvVGCMD_MOVE_REL)
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
			Destination, gcvVGCMD_MOVE_REL
			));

		/* Extract the command coordinates. */
		moveToX = Source->get(Source);
		moveToY = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, moveToX);
		Destination->set(Destination, moveToY);

		/* Determine the absolute coordinates. */
		moveToX += coords->lastX;
		moveToY += coords->lastY;

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

vgmDEFINEAPPEND(gcvVGCMD_LINE)
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

		/* Extract the command coordinates. */
		lineToX = Source->get(Source);
		lineToY = Source->get(Source);

		/* Store the coordinates in the destination. */
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

vgmDEFINEAPPEND(gcvVGCMD_LINE_REL)
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
			Destination, gcvVGCMD_LINE_REL
			));

		/* Extract the command coordinates. */
		lineToX = Source->get(Source);
		lineToY = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, lineToX);
		Destination->set(Destination, lineToY);

		/* Determine the absolute coordinates. */
		lineToX += coords->lastX;
		lineToY += coords->lastY;

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
***************************** gcvVGCMD_HLINE_EMUL ******************************
\******************************************************************************/

vgmDEFINEAPPEND(gcvVGCMD_HLINE_EMUL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT lineToX;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_HLINE_EMUL
			));

		/* Extract the command coordinate. */
		lineToX = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, lineToX);
		Destination->set(Destination, coords->lastY);

		/* Update the control coordinates. */
		coords->lastX    = lineToX;
		coords->controlX = lineToX;
		coords->controlY = coords->lastY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINEAPPEND(gcvVGCMD_HLINE_EMUL_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT lineToX;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_HLINE_EMUL_REL
			));

		/* Extract the command coordinate. */
		lineToX = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, lineToX);
		Destination->set(Destination, 0.0f);

		/* Determine the absolute coordinates. */
		lineToX += coords->lastX;

		/* Update the control coordinates. */
		coords->lastX    = lineToX;
		coords->controlX = lineToX;
		coords->controlY = coords->lastY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/******************************************************************************\
***************************** gcvVGCMD_VLINE_EMUL ******************************
\******************************************************************************/

vgmDEFINEAPPEND(gcvVGCMD_VLINE_EMUL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT lineToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_VLINE_EMUL
			));

		/* Extract the command coordinate. */
		          Source->get(Source);
		lineToY = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, coords->lastX);
		Destination->set(Destination, lineToY);

		/* Update the control coordinates. */
		coords->lastY    = lineToY;
		coords->controlX = coords->lastX;
		coords->controlY = lineToY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINEAPPEND(gcvVGCMD_VLINE_EMUL_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT lineToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_VLINE_EMUL_REL
			));

		/* Extract the command coordinate. */
		          Source->get(Source);
		lineToY = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, 0.0f);
		Destination->set(Destination, lineToY);

		/* Determine the absolute coordinates. */
		lineToY += coords->lastY;

		/* Update the control coordinates. */
		coords->lastY    = lineToY;
		coords->controlX = coords->lastX;
		coords->controlY = lineToY;

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

vgmDEFINEAPPEND(gcvVGCMD_QUAD)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT controlX, controlY;
	gctFLOAT quadToX, quadToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_QUAD
			));

		/* Extract the command coordinates. */
		controlX = Source->get(Source);
		controlY = Source->get(Source);
		quadToX  = Source->get(Source);
		quadToY  = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, controlX);
		Destination->set(Destination, controlY);
		Destination->set(Destination, quadToX);
		Destination->set(Destination, quadToY);

		/* Update the control coordinates. */
		coords->lastX    = quadToX;
		coords->lastY    = quadToY;
		coords->controlX = controlX;
		coords->controlY = controlY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINEAPPEND(gcvVGCMD_QUAD_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT controlX, controlY;
	gctFLOAT quadToX, quadToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_QUAD_REL
			));

		/* Extract the command coordinates. */
		controlX = Source->get(Source);
		controlY = Source->get(Source);
		quadToX  = Source->get(Source);
		quadToY  = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, controlX);
		Destination->set(Destination, controlY);
		Destination->set(Destination, quadToX);
		Destination->set(Destination, quadToY);

		/* Determine the absolute coordinates. */
		controlX += coords->lastX;
		controlY += coords->lastY;
		quadToX  += coords->lastX;
		quadToY  += coords->lastY;

		/* Update the last coordinates. */
		coords->lastX    = quadToX;
		coords->lastY    = quadToY;
		coords->controlX = controlX;
		coords->controlY = controlY;

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

vgmDEFINEAPPEND(gcvVGCMD_CUBIC)
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

		/* Extract the command coordinates. */
		controlX1 = Source->get(Source);
		controlY1 = Source->get(Source);
		controlX2 = Source->get(Source);
		controlY2 = Source->get(Source);
		cubicToX  = Source->get(Source);
		cubicToY  = Source->get(Source);

		/* Store the coordinates in the destination. */
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

vgmDEFINEAPPEND(gcvVGCMD_CUBIC_REL)
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
			Destination, gcvVGCMD_CUBIC_REL
			));

		/* Extract the command coordinates. */
		controlX1 = Source->get(Source);
		controlY1 = Source->get(Source);
		controlX2 = Source->get(Source);
		controlY2 = Source->get(Source);
		cubicToX  = Source->get(Source);
		cubicToY  = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, controlX1);
		Destination->set(Destination, controlY1);
		Destination->set(Destination, controlX2);
		Destination->set(Destination, controlY2);
		Destination->set(Destination, cubicToX);
		Destination->set(Destination, cubicToY);

		/* Determine the absolute coordinates. */
		controlX2 += coords->lastX;
		controlY2 += coords->lastY;
		cubicToX  += coords->lastX;
		cubicToY  += coords->lastY;

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
****************************** gcvVGCMD_SQUAD_EMUL *****************************
\******************************************************************************/

vgmDEFINEAPPEND(gcvVGCMD_SQUAD_EMUL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT controlX, controlY;
	gctFLOAT quadToX, quadToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_SQUAD_EMUL
			));

		/* Compute the control point. */
		controlX = 2.0f * coords->lastX - coords->controlX;
		controlY = 2.0f * coords->lastY - coords->controlY;

		/* Extract the command coordinates. */
		          Source->get(Source);
		          Source->get(Source);
		quadToX = Source->get(Source);
		quadToY = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, controlX);
		Destination->set(Destination, controlY);
		Destination->set(Destination, quadToX);
		Destination->set(Destination, quadToY);

		/* Update the control coordinates. */
		coords->lastX    = quadToX;
		coords->lastY    = quadToY;
		coords->controlX = controlX;
		coords->controlY = controlY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINEAPPEND(gcvVGCMD_SQUAD_EMUL_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT controlX, controlY;
	gctFLOAT quadToX, quadToY;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_SQUAD_EMUL_REL
			));

		/* Compute the relative control point. */
		controlX = coords->lastX - coords->controlX;
		controlY = coords->lastY - coords->controlY;

		/* Extract the command coordinates. */
		          Source->get(Source);
		          Source->get(Source);
		quadToX = Source->get(Source);
		quadToY = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, controlX);
		Destination->set(Destination, controlY);
		Destination->set(Destination, quadToX);
		Destination->set(Destination, quadToY);

		/* Determine the absolute coordinates. */
		controlX += coords->lastX;
		controlY += coords->lastY;
		quadToX  += coords->lastX;
		quadToY  += coords->lastY;

		/* Update the control coordinates. */
		coords->lastX    = quadToX;
		coords->lastY    = quadToY;
		coords->controlX = controlX;
		coords->controlY = controlY;

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

vgmDEFINEAPPEND(gcvVGCMD_SCUBIC_EMUL)
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
			Destination, gcvVGCMD_SCUBIC_EMUL
			));

		/* Compute the control point. */
		controlX1 = 2.0f * coords->lastX - coords->controlX;
		controlY1 = 2.0f * coords->lastY - coords->controlY;

		/* Extract the command coordinates. */
		            Source->get(Source);
		            Source->get(Source);
		controlX2 = Source->get(Source);
		controlY2 = Source->get(Source);
		cubicToX  = Source->get(Source);
		cubicToY  = Source->get(Source);

		/* Store the coordinates in the destination. */
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

vgmDEFINEAPPEND(gcvVGCMD_SCUBIC_EMUL_REL)
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
			Destination, gcvVGCMD_SCUBIC_EMUL_REL
			));

		/* Compute the relative control point. */
		controlX1 = coords->lastX - coords->controlX;
		controlY1 = coords->lastY - coords->controlY;

		/* Extract the command coordinates. */
		            Source->get(Source);
		            Source->get(Source);
		controlX2 = Source->get(Source);
		controlY2 = Source->get(Source);
		cubicToX  = Source->get(Source);
		cubicToY  = Source->get(Source);

		/* Store the coordinates in the destination. */
		Destination->set(Destination, controlX1);
		Destination->set(Destination, controlY1);
		Destination->set(Destination, controlX2);
		Destination->set(Destination, controlY2);
		Destination->set(Destination, cubicToX);
		Destination->set(Destination, cubicToY);

		/* Determine the absolute coordinates. */
		controlX2 += coords->lastX;
		controlY2 += coords->lastY;
		cubicToX  += coords->lastX;
		cubicToY  += coords->lastY;

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
********************************** gcvVGCMD_ARC ********************************
\******************************************************************************/

static gceSTATUS _AppendArc(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source,
	gctBOOL Relative
	)
{
	gceSTATUS status;

	do
	{
		vgsCONTEXT_PTR context;
		vgsARCCOORDINATES_PTR arcCoords;
		vgsPATHWALKER arc;

		/* Get a pointer to the original ARC data. */
		arcCoords = (vgsARCCOORDINATES_PTR) Source->currPathData->extra;
		gcmASSERT(arcCoords != gcvNULL);

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
			arcCoords->horRadius, arcCoords->verRadius,
			arcCoords->rotAngle,
			arcCoords->endX, arcCoords->endY,
			arcCoords->counterClockwise,
			arcCoords->large,
			Relative
			));

		/* Append to the existing buffer. */
		vgsPATHWALKER_AppendData(
			Destination, &arc, 1, 5
			);

		/* Skip the ARC path in the source. */
		vgsPATHWALKER_SeekToEnd(Source);

		/* Set the ARC present flag. */
		Destination->path->hasArcs = gcvTRUE;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

vgmDEFINEAPPEND(gcvVGCMD_ARC)
{
	return _AppendArc(Destination, Source, gcvFALSE);
}

vgmDEFINEAPPEND(gcvVGCMD_ARC_REL)
{
	return _AppendArc(Destination, Source, gcvTRUE);
}


/*******************************************************************************
**
** vgfGetAppendArray
**
** Returns the pointer to the array of path appending functions for "slow" mode
** where the appending happens one segment at a time with format conversion.
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

void vgfGetAppendArray(
	vgtSEGMENTHANDLER ** Array,
	gctUINT_PTR Count
	)
{
	static vgtSEGMENTHANDLER _appendCommand[] =
	{
		vgmAPPEND(Invalid),                  /*   0: gcvVGCMD_END             */
		vgmAPPEND(gcvVGCMD_CLOSE),           /*   1: gcvVGCMD_CLOSE           */
		vgmAPPEND(gcvVGCMD_MOVE),            /*   2: gcvVGCMD_MOVE            */
		vgmAPPEND(gcvVGCMD_MOVE_REL),        /*   3: gcvVGCMD_MOVE_REL        */
		vgmAPPEND(gcvVGCMD_LINE),            /*   4: gcvVGCMD_LINE            */
		vgmAPPEND(gcvVGCMD_LINE_REL),        /*   5: gcvVGCMD_LINE_REL        */
		vgmAPPEND(gcvVGCMD_QUAD),            /*   6: gcvVGCMD_QUAD            */
		vgmAPPEND(gcvVGCMD_QUAD_REL),        /*   7: gcvVGCMD_QUAD_REL        */
		vgmAPPEND(gcvVGCMD_CUBIC),           /*   8: gcvVGCMD_CUBIC           */
		vgmAPPEND(gcvVGCMD_CUBIC_REL),       /*   9: gcvVGCMD_CUBIC_REL       */
		vgmAPPEND(Invalid),                  /*  10: gcvVGCMD_BREAK           */
		vgmAPPEND(Invalid),                  /*  11: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  12: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  13: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  14: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  15: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  16: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  17: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  18: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  19: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  20: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  21: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  22: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  23: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  24: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  25: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  26: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  27: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  28: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  29: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  30: **** R E S E R V E D *****/
		vgmAPPEND(Invalid),                  /*  31: **** R E S E R V E D *****/

		vgmAPPEND(Invalid),                  /*  32: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  33: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  34: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  35: ***** I N V A L I D ******/
		vgmAPPEND(gcvVGCMD_HLINE_EMUL),      /*  36: gcvVGCMD_HLINE_EMUL      */
		vgmAPPEND(gcvVGCMD_HLINE_EMUL_REL),  /*  37: gcvVGCMD_HLINE_EMUL_REL  */
		vgmAPPEND(Invalid),                  /*  38: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  39: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  40: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  41: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  42: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  43: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  44: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  45: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  46: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  47: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  48: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  49: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  50: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  51: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  52: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  53: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  54: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  55: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  56: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  57: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  58: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  59: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  60: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  61: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  62: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  63: ***** I N V A L I D ******/

		vgmAPPEND(Invalid),                  /*  64: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  65: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  66: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  67: ***** I N V A L I D ******/
		vgmAPPEND(gcvVGCMD_VLINE_EMUL),      /*  68: gcvVGCMD_VLINE_EMUL      */
		vgmAPPEND(gcvVGCMD_VLINE_EMUL_REL),  /*  69: gcvVGCMD_VLINE_EMUL_REL  */
		vgmAPPEND(Invalid),                  /*  70: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  71: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  72: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  73: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  74: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  75: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  76: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  77: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  78: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  79: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  80: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  81: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  82: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  83: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  84: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  85: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  86: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  87: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  88: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  89: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  90: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  91: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  92: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  93: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  94: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  95: ***** I N V A L I D ******/

		vgmAPPEND(Invalid),                  /*  96: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  97: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  98: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /*  99: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 100: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 101: ***** I N V A L I D ******/
		vgmAPPEND(gcvVGCMD_SQUAD_EMUL),      /* 102: gcvVGCMD_SQUAD_EMUL      */
		vgmAPPEND(gcvVGCMD_SQUAD_EMUL_REL),  /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
		vgmAPPEND(gcvVGCMD_SCUBIC_EMUL),     /* 104: gcvVGCMD_SCUBIC_EMUL     */
		vgmAPPEND(gcvVGCMD_SCUBIC_EMUL_REL), /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
		vgmAPPEND(Invalid),                  /* 106: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 107: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 108: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 109: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 110: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 111: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 112: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 113: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 114: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 115: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 116: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 117: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 118: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 119: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 120: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 121: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 122: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 123: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 124: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 125: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 126: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 127: ***** I N V A L I D ******/

		vgmAPPEND(Invalid),                  /* 128: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 129: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 130: ***** I N V A L I D ******/
		vgmAPPEND(Invalid),                  /* 131: ***** I N V A L I D ******/
		vgmAPPEND(gcvVGCMD_ARC),             /* 132: gcvVGCMD_ARC_LINE        */
		vgmAPPEND(gcvVGCMD_ARC_REL),         /* 133: gcvVGCMD_ARC_LINE_REL    */
		vgmAPPEND(gcvVGCMD_ARC),             /* 134: gcvVGCMD_ARC_QUAD        */
		vgmAPPEND(gcvVGCMD_ARC_REL)          /* 135: gcvVGCMD_ARC_QUAD_REL    */
	};

	* Array = _appendCommand;
	* Count = gcmCOUNTOF(_appendCommand);
}
