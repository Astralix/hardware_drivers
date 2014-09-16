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
#include <stdlib.h>

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
** _CLOSE_PATH
**
** Generate VG_CLOSE_PATH path command and add it to the path.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker structure.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _CLOSE_PATH(
	vgsPATHWALKER_PTR Destination
	)
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


/*******************************************************************************
**
** _MOVE_TO_ABS
**
** Generate VG_MOVE_TO_ABS path command and add it to the path.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker structure.
**
**    X, Y
**       Coordinate values.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _MOVE_TO_ABS(
	vgsPATHWALKER_PTR Destination,
	gctFLOAT X,
	gctFLOAT Y
	)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_MOVE
			));

		/* Set the command coordinates. */
		Destination->set(Destination, X);
		Destination->set(Destination, Y);

		/* Update the control coordinates. */
		coords->startX   = X;
		coords->startY   = Y;
		coords->lastX    = X;
		coords->lastY    = Y;
		coords->controlX = X;
		coords->controlY = Y;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _LINE_TO_ABS
**
** Generate VG_LINE_TO_ABS path command and add it to the path.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker structure.
**
**    X, Y
**       Coordinate values.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _LINE_TO_ABS(
	vgsPATHWALKER_PTR Destination,
	gctFLOAT X,
	gctFLOAT Y
	)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_LINE
			));

		/* Set the command coordinates. */
		Destination->set(Destination, X);
		Destination->set(Destination, Y);

		/* Update the control coordinates. */
		coords->lastX    = X;
		coords->lastY    = Y;
		coords->controlX = X;
		coords->controlY = Y;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _HLINE_TO_REL
**
** Generate VG_HLINE_TO_REL path command and add it to the path.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker structure.
**
**    X
**       Coordinate value.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _HLINE_TO_REL(
	vgsPATHWALKER_PTR Destination,
	gctFLOAT X
	)
{
	gceSTATUS status;
	gctFLOAT absX;
	vgsCONTROL_COORD_PTR coords;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_HLINE_EMUL_REL
			));

		/* Set the command coordinates. */
		Destination->set(Destination, X);
		Destination->set(Destination, 0.0f);

		/* Determine the absolute coordinate. */
		absX = X + coords->lastX;

		/* Update the control coordinates. */
		coords->lastX    = absX;
		coords->controlX = absX;
		coords->controlY = coords->lastY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _VLINE_TO_REL
**
** Generate VG_VLINE_TO_REL path command and add it to the path.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker structure.
**
**    Y
**       Coordinate value.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _VLINE_TO_REL(
	vgsPATHWALKER_PTR Destination,
	gctFLOAT Y
	)
{
	gceSTATUS status;
	gctFLOAT absY;
	vgsCONTROL_COORD_PTR coords;

	do
	{
		/* Get a shortcut to the control coordinates. */
		coords = Destination->coords;

		/* Add new command. */
		gcmERR_BREAK(vgsPATHWALKER_WriteCommand(
			Destination, gcvVGCMD_VLINE_EMUL_REL
			));

		/* Set the command coordinates. */
		Destination->set(Destination, 0.0f);
		Destination->set(Destination, Y);

		/* Determine the absolute coordinate. */
		absY = Y + coords->lastY;

		/* Update the control coordinates. */
		coords->lastY    = absY;
		coords->controlX = coords->lastX;
		coords->controlY = absY;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _SMALL_ARC
**
** Generate a small ARC path command and add it to the path.
**
** INPUT:
**
**    Destination
**       Pointer to the destination walker structure.
**
**    CounterClockwise
**       Clockwise or counter clockwise arc direction.
**
**    Relative
**       Relative vs. absolute ARC.
**
**    HorRadius, VerRadius
**       Horizontal and vertical radius values.
**
**    RotAngle
**       Angle of the curve.
**
**    EndX, EndY
**       The end point coordinates.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _SMALL_ARC(
	vgsPATHWALKER_PTR Destination,
	gctBOOL CounterClockwise,
	gctBOOL Relative,
	gctFLOAT HorRadius,
	gctFLOAT VerRadius,
	gctFLOAT RotAngle,
	gctFLOAT EndX,
	gctFLOAT EndY
	)
{
	gceSTATUS status;
	vgsPATHWALKER arc;
	vgsCONTEXT_PTR context;

	do
	{
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
			HorRadius, VerRadius,
			RotAngle,
			EndX, EndY,
			CounterClockwise,
			gcvFALSE,
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


/******************************************************************************\
***************************** Internal Utility API *****************************
\******************************************************************************/

/*******************************************************************************
**
** vgfNormalizeCoordinates
**
** Verifies the source and target coordinates and adjusts them as necessary
** to make sure they are within the allowed range.
**
** INPUT:
**
**    X, Y
**       Pointers to the rectangle origin.
**
**    Width, Height
**       Pointers to the rectangle size.
**
**    SurfaceSize
**       Pointer to the surface size.
**
** OUTPUT:
**
**    Returns non zero if normalization was successful.
*/

gctBOOL vgfNormalizeCoordinates(
	IN OUT gctINT_PTR X,
	IN OUT gctINT_PTR Y,
	IN OUT gctINT_PTR Width,
	IN OUT gctINT_PTR Height,
	IN gcsSIZE_PTR SurfaceSize
	)
{
	gctINT32 right;
	gctINT32 bottom;

	/* Adjust for negative origins. */
	if ((* X) < 0)
	{
		(* Width) += (* X);
		(* X)      = 0;
	}

	if ((* Y) < 0)
	{
		(* Height) += (* Y);
		(* Y)       = 0;
	}

	/* Verify the origins. */
	if (((* X) >= SurfaceSize->width) ||
		((* Y) >= SurfaceSize->height))
	{
		return gcvFALSE;
	}

	/* Adjust for large rectangles. */
	right  = ((* X) + (* Width))  - SurfaceSize->width;
	bottom = ((* Y) + (* Height)) - SurfaceSize->height;

	if (right  > 0) (* Width)  -= right;
	if (bottom > 0) (* Height) -= bottom;

	/* Determine the result. */
	return ((* Width) > 0) && ((* Height) > 0);
}


/*******************************************************************************
**
** vgfNormalizeCoordinatePairs
**
** Verifies the source and target coordinates and adjusts them as necessary
** to make sure they are within the allowed range.
**
** INPUT:
**
**    SourceX, SourceY
**    TargetX, TargetY
**       Pointers to the source and target origins.
**
**    Width, Height
**       Pointers to the rectangle size.
**
**    SourceSize
**    TargetSize
**       Pointers to the source and target surface sizes.
**
** OUTPUT:
**
**    Returns non zero if normalization was successful.
*/

gctBOOL vgfNormalizeCoordinatePairs(
	IN OUT gctINT_PTR SourceX,
	IN OUT gctINT_PTR SourceY,
	IN OUT gctINT_PTR TargetX,
	IN OUT gctINT_PTR TargetY,
	IN OUT gctINT_PTR Width,
	IN OUT gctINT_PTR Height,
	IN gcsSIZE_PTR SourceSize,
	IN gcsSIZE_PTR TargetSize
	)
{
	gctINT64 sourceX, sourceY;
	gctINT64 targetX, targetY;
	gctINT64 width, height;
	gctINT32 sourceRight;
	gctINT32 sourceBottom;
	gctINT32 targetRight;
	gctINT32 targetBottom;

	/* Copy inputs. */
	sourceX = * SourceX;
	sourceY = * SourceY;
	targetX = * TargetX;
	targetY = * TargetY;
	width   = * Width;
	height  = * Height;

	/* Adjust for negative origins. */
	if (sourceX < 0)
	{
		width   += sourceX;
		targetX -= sourceX;
		sourceX  = 0;
	}

	if (targetX < 0)
	{
		width   += targetX;
		sourceX -= targetX;
		targetX  = 0;
	}

	if (sourceY < 0)
	{
		height  += sourceY;
		targetY -= sourceY;
		sourceY  = 0;
	}

	if (targetY < 0)
	{
		height  += targetY;
		sourceY -= targetY;
		targetY  = 0;
	}

	/* Verify the origins. */
	if ((sourceX >= SourceSize->width)  ||
		(sourceY >= SourceSize->height) ||
		(targetX >= TargetSize->width)  ||
		(targetY >= TargetSize->height))
	{
		return gcvFALSE;
	}

	/* Adjust for large rectangles inside the source. */
	sourceRight  = (gctINT) ((sourceX + width)  - SourceSize->width);
	sourceBottom = (gctINT) ((sourceY + height) - SourceSize->height);

	if (sourceRight  > 0) width  -= sourceRight;
	if (sourceBottom > 0) height -= sourceBottom;

	/* Adjust for large rectangles inside the target. */
	targetRight  = (gctINT) ((targetX + width)  - TargetSize->width);
	targetBottom = (gctINT) ((targetY + height) - TargetSize->height);

	if (targetRight  > 0) width  -= targetRight;
	if (targetBottom > 0) height -= targetBottom;

	/* Verify the size. */
	if ((width <= 0) || (height <= 0))
	{
		return gcvFALSE;
	}

	/* Set the result. */
	(* SourceX) = (gctINT) sourceX;
	(* SourceY) = (gctINT) sourceY;
	(* TargetX) = (gctINT) targetX;
	(* TargetY) = (gctINT) targetY;
	(* Width)   = (gctINT) width;
	(* Height)  = (gctINT) height;

	/* Success. */
	return gcvTRUE;
}


/******************************************************************************\
******************************** VG Utility API ********************************
\******************************************************************************/

/*******************************************************************************
**
** vguLine
**
** vguLine appends a line segment to a path. This is equivalent to
** the following pseudocode:
**
**    LINE(x0, y0, x1, y1):
**       MOVE_TO_ABS x0, y0
**       LINE_TO_ABS x1, y1
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    X0, Y0, X1, Y1
**       Line coordinates.
**
** OUTPUT:
**
**    Nothing.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguLine(
	VGPath Path,
	VGfloat X0,
	VGfloat Y0,
	VGfloat X1,
	VGfloat Y1
	)
{
	VGUErrorCode result = VGU_BAD_HANDLE_ERROR;

	vgmENTERAPI(vguLine)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATHWALKER destination;

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			result = VGU_BAD_HANDLE_ERROR;
			break;
		}

		/* Cast the object. */
		dstPath = (vgsPATH_PTR) Path;

		/* Verify the capability. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_APPEND_TO) == 0)
		{
			result = VGU_PATH_CAPABILITY_ERROR;
			break;
		}

		/* Initialize the destination walker. */
		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Add path commands. */
		gcmERR_GOTO(_MOVE_TO_ABS(&destination, X0, Y0));
		gcmERR_GOTO(_LINE_TO_ABS(&destination, X1, Y1));

		/* Close the buffer. */
		gcmERR_GOTO(vgsPATHWALKER_DoneWriting(&destination));

		/* Success. */
		result = VGU_NO_ERROR;
		break;

ErrorHandler:

		/* Roll back path changes if any. */
		vgsPATHWALKER_Rollback(&destination);

		/* Set error condition. */
		result = VGU_OUT_OF_MEMORY_ERROR;
	}
	vgmLEAVEAPI(vguLine);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vguPolygon
**
** vguPolygon appends a polyline (connected sequence of line segments) or
** polygon to a path. This is equivalent to the following pseudo-code:
**
**    POLYGON(points, count):
**       MOVE_TO_ABS points[0], points[1]
**       for (i = 1; i < count; i++)
**       {
**          LINE_TO_ABS points[2*i], points[2*i + 1]
**       }
**       if (closed) CLOSE_PATH
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    Points
**       Pointer to the point array.
**
**    Count
**       Number of points in the point array.
**
**    Closed
**       The path will be closed if VG_TRUE is given.
**
** OUTPUT:
**
**    Nothing.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguPolygon(
	VGPath Path,
	const VGfloat * Points,
	VGint Count,
	VGboolean Closed
	)
{
	VGUErrorCode result = VGU_BAD_HANDLE_ERROR;

	vgmENTERAPI(vguPolygon)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATHWALKER destination;
		gctUINT datatypeSize;
		gctFLOAT_PTR points;
		gctINT i;

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			result = VGU_BAD_HANDLE_ERROR;
			break;
		}

		/* Cast the object. */
		dstPath = (vgsPATH_PTR) Path;

		/* Verify the capability. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_APPEND_TO) == 0)
		{
			result = VGU_PATH_CAPABILITY_ERROR;
			break;
		}

		/* Get the datatype size. */
		datatypeSize = vgfGetPathDataSize(dstPath->halDataType);

		/* Verify alignment. */
		if (vgmIS_MISSALIGNED(Points, datatypeSize))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		/* Verify the arguments. */
		if ((Points == gcvNULL) || (Count <= 0))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		/* Initialize the destination walker. */
		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Set initial points pointer. */
		points = (gctFLOAT_PTR) Points;

		/* Add the initial move. */
		gcmERR_GOTO(_MOVE_TO_ABS(&destination, points[0], points[1]));
		points += 2;

		/* Add polygon sides. */
		for (i = 1; i < Count; i++)
		{
			gcmERR_GOTO(_LINE_TO_ABS(&destination, points[0], points[1]));
			points += 2;
		}

		/* Close the path. */
		if (Closed)
		{
			gcmERR_GOTO(_CLOSE_PATH(&destination));
		}

		/* Close the buffer. */
		gcmERR_GOTO(vgsPATHWALKER_DoneWriting(&destination));

		/* Success. */
		result = VGU_NO_ERROR;
		break;

ErrorHandler:

		/* Roll back path changes if any. */
		vgsPATHWALKER_Rollback(&destination);

		/* Set error condition. */
		result = VGU_OUT_OF_MEMORY_ERROR;
	}
	vgmLEAVEAPI(vguPolygon);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vguRect
**
** The vguRect function appends an axis-aligned rectangle with its lower-left
** corner at (X, Y) and a given Width and Height to a path. This is equivalent
** to the following pseudo-code:
**
**    RECT(x, y, width, height):
**       MOVE_TO_ABS x, y
**       HLINE_TO_REL width
**       VLINE_TO_REL height
**       HLINE_TO_REL -width
**       CLOSE_PATH
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    X, Y
**    Width, Height
**       Rectangle coordinates.
**
** OUTPUT:
**
**    Nothing.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguRect(
	VGPath Path,
	VGfloat X,
	VGfloat Y,
	VGfloat Width,
	VGfloat Height
	)
{
	VGUErrorCode result = VGU_BAD_HANDLE_ERROR;

	vgmENTERAPI(vguRect)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATHWALKER destination;

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			result = VGU_BAD_HANDLE_ERROR;
			break;
		}

		/* Cast the object. */
		dstPath = (vgsPATH_PTR) Path;

		/* Verify the capability. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_APPEND_TO) == 0)
		{
			result = VGU_PATH_CAPABILITY_ERROR;
			break;
		}

		/* Verify the arguments. */
		if ((Width <= 0) || (Height <= 0))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		/* Initialize the destination walker. */
		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Add the initial move. */
		gcmERR_GOTO(_MOVE_TO_ABS(&destination, X, Y));

		/* Add rectangle sides. */
		gcmERR_GOTO(_HLINE_TO_REL(&destination,  Width));
		gcmERR_GOTO(_VLINE_TO_REL(&destination,  Height));
		gcmERR_GOTO(_HLINE_TO_REL(&destination, -Width));

		/* Close the path. */
		gcmERR_GOTO(_CLOSE_PATH(&destination));

		/* Close the buffer. */
		gcmERR_GOTO(vgsPATHWALKER_DoneWriting(&destination));

		/* Success. */
		result = VGU_NO_ERROR;
		break;

ErrorHandler:

		/* Roll back path changes if any. */
		vgsPATHWALKER_Rollback(&destination);

		/* Set error condition. */
		result = VGU_OUT_OF_MEMORY_ERROR;
	}
	vgmLEAVEAPI(vguRect);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vguRoundRect
**
** The vguRoundRect function appends an axis-aligned round-cornered rectangle
** with the lower-left corner of its rectangular bounding box at (X, Y) and a
** given Width, Height, ArcWidth, and ArcHeight to a path. This is equivalent
** to the following pseudo-code:
**
**    ROUNDRECT(x, y, w, h, arcWidth, arcHeight):
**       MOVE_TO_ABS (x + arcWidth/2), y
**
**       HLINE_TO_REL width - arcWidth
**       SCCWARC_TO_REL arcWidth/2, arcHeight/2, 0, arcWidth/2, arcHeight/2
**
**       VLINE_TO_REL height - arcHeight
**       SCCWARC_TO_REL arcWidth/2, arcHeight/2, 0, -arcWidth/2, arcHeight/2
**
**       HLINE_TO_REL -(width - arcWidth)
**       SCCWARC_TO_REL arcWidth/2, arcHeight/2, 0, -arcWidth/2, -arcHeight/2
**
**       VLINE_TO_REL -(height - arcHeight)
**       SCCWARC_TO_REL arcWidth/2, arcHeight/2, 0, arcWidth/2, -arcHeight/2
**
**       CLOSE_PATH
**
** If ArcWidth is less than 0, it is clamped to 0. If ArcWidth is greater than
** Width, its value is clamped to that of Width. Similarly, ArcHeight is clamped
** to a value between 0 and Height. The arcs are included even when ArcWidth
** and/or ArcHeight is 0.
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    X, Y
**    Width, Height
**       Rectangle coordinates.
**
**    ArcWidth, ArcHeight
**       Horizontal and vertical diameters of the ellipse.
**
** OUTPUT:
**
**    Nothing.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguRoundRect(
	VGPath Path,
	VGfloat X,
	VGfloat Y,
	VGfloat Width,
	VGfloat Height,
	VGfloat ArcWidth,
	VGfloat ArcHeight
	)
{
	VGUErrorCode result = VGU_BAD_HANDLE_ERROR;

	vgmENTERAPI(vguRoundRect)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATHWALKER destination;
		VGfloat arcWidth, arcHeight;
		VGfloat hRadius, vRadius;
		VGfloat width, height;

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			result = VGU_BAD_HANDLE_ERROR;
			break;
		}

		/* Cast the object. */
		dstPath = (vgsPATH_PTR) Path;

		/* Verify the capability. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_APPEND_TO) == 0)
		{
			result = VGU_PATH_CAPABILITY_ERROR;
			break;
		}

		/* Verify the arguments. */
		if ((Width <= 0) || (Height <= 0))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		/* Clamp. */
		arcWidth  = gcmCLAMP(ArcWidth,  0.0f, Width);
		arcHeight = gcmCLAMP(ArcHeight, 0.0f, Height);

		/* Compute parameters. */
		hRadius = arcWidth  / 2;
		vRadius = arcHeight / 2;
		width  = Width  - arcWidth;
		height = Height - arcHeight;

		/* Initialize the destination walker. */
		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Add the initial move. */
		gcmERR_GOTO(_MOVE_TO_ABS(&destination, X + hRadius, Y));

		/* Add rectangle sides and corners. */
		gcmERR_GOTO(_HLINE_TO_REL(&destination, width));
		gcmERR_GOTO(_SMALL_ARC(
			&destination, gcvTRUE, gcvTRUE,
			hRadius, vRadius, 0,  hRadius,  vRadius
			));

		gcmERR_GOTO(_VLINE_TO_REL(&destination, height));
		gcmERR_GOTO(_SMALL_ARC(
			&destination, gcvTRUE, gcvTRUE,
			hRadius, vRadius, 0, -hRadius,  vRadius
			));

		gcmERR_GOTO(_HLINE_TO_REL(&destination, -width));
		gcmERR_GOTO(_SMALL_ARC(
			&destination, gcvTRUE, gcvTRUE,
			hRadius, vRadius, 0, -hRadius, -vRadius
			));

		gcmERR_GOTO(_VLINE_TO_REL(&destination, -height));
		gcmERR_GOTO(_SMALL_ARC(
			&destination, gcvTRUE, gcvTRUE,
			hRadius, vRadius, 0,  hRadius, -vRadius
			));

		/* Close the path. */
		gcmERR_GOTO(_CLOSE_PATH(&destination));

		/* Close the buffer. */
		gcmERR_GOTO(vgsPATHWALKER_DoneWriting(&destination));

		/* Success. */
		result = VGU_NO_ERROR;
		break;

ErrorHandler:

		/* Roll back path changes if any. */
		vgsPATHWALKER_Rollback(&destination);

		/* Set error condition. */
		result = VGU_OUT_OF_MEMORY_ERROR;
	}
	vgmLEAVEAPI(vguRoundRect);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vguEllipse
**
** vguEllipse appends an axis-aligned ellipse to a path. The center of the
** ellipse is given by (CenterX, CenterY) and the dimensions of the axis-aligned
** rectangle enclosing the ellipse are given by Width and Height. The ellipse
** begins at (CenterX + Width / 2, CenterY) and is stroked as two equal counter-
** clockwise arcs. This is equivalent to the following pseudo-code:
**
**    ELLIPSE(cx, cy, width, height):
**       MOVE_TO_ABS cx + width/2, cy
**       SCCWARC_TO_REL width/2, height/2, 0, -width, 0
**       SCCWARC_TO_REL width/2, height/2, 0,  width, 0
**       CLOSE_PATH
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    CenterX, CenterY
**       The center coordinates of the ellipse.
**
**    Width, Height
**       Ellipse horizontal and vertical diameters.
**
** OUTPUT:
**
**    Nothing.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguEllipse(
	VGPath Path,
	VGfloat CenterX,
	VGfloat CenterY,
	VGfloat Width,
	VGfloat Height
	)
{
	VGUErrorCode result = VGU_BAD_HANDLE_ERROR;

	vgmENTERAPI(vguEllipse)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATHWALKER destination;
		VGfloat hRadius, vRadius;

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			result = VGU_BAD_HANDLE_ERROR;
			break;
		}

		/* Cast the object. */
		dstPath = (vgsPATH_PTR) Path;

		/* Verify the capability. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_APPEND_TO) == 0)
		{
			result = VGU_PATH_CAPABILITY_ERROR;
			break;
		}

		/* Verify the arguments. */
		if ((Width <= 0) || (Height <= 0))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		/* Compute parameters. */
		hRadius = Width  / 2;
		vRadius = Height / 2;

		/* Initialize the destination walker. */
		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Add the initial move. */
		gcmERR_GOTO(_MOVE_TO_ABS(&destination, CenterX + hRadius, CenterY));

		/* Add ellipse curves. */
		gcmERR_GOTO(_SMALL_ARC(
			&destination, gcvTRUE, gcvTRUE,
			hRadius, vRadius, 0, -Width, 0
			));

		gcmERR_GOTO(_SMALL_ARC(
			&destination, gcvTRUE, gcvTRUE,
			hRadius, vRadius, 0,  Width, 0
			));

		/* Close the path. */
		gcmERR_GOTO(_CLOSE_PATH(&destination));

		/* Close the buffer. */
		gcmERR_GOTO(vgsPATHWALKER_DoneWriting(&destination));

		/* Success. */
		result = VGU_NO_ERROR;
		break;

ErrorHandler:

		/* Roll back path changes if any. */
		vgsPATHWALKER_Rollback(&destination);

		/* Set error condition. */
		result = VGU_OUT_OF_MEMORY_ERROR;
	}
	vgmLEAVEAPI(vguEllipse);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vguArc
**
** vguArc appends an elliptical arc to a path, possibly along with one or two
** line segments, according to the ArcType parameter. The StartAngle and
** AngleExtent parameters are given in degrees, proceeding counter-clockwise
** from the positive X axis. The arc is defined on the unit circle, then scaled
** by the Width and Height of the ellipse; thus, the starting point of the arc
** has coordinates:
**
**    (
**       x + cos(startAngle) * w / 2,
**       y + sin(startAngle) * h / 2
**    )
**
** and the ending point has coordinates:
**
**    (
**       x + cos(startAngle + angleExtent) * w / 2,
**       y + sin(startAngle + angleExtent) * h / 2
**    ).
**
** If AngleExtent is negative, the arc will proceed clockwise; if it is larger
** than 360 or smaller than -360, the arc will wrap around itself. The following
** pseudocode illustrates the arc path generation:
**
**    ARC(x, y, w, h, startAngle, angleExtent, arcType):
**       last = startAngle + angleExtent
**       MOVE_TO_ABS x+cos(startAngle)*w/2, y+sin(startAngle)*h/2
**       if (angleExtent > 0)
**       {
**          angle = startAngle + 180
**          while (angle < last)
**          {
**             SCCWARC_TO_ABS w/2, h/2, 0, x+cos(angle)*w/2, y+sin(angle)*h/2
**             angle += 180
**          }
**          SCCWARC_TO_ABS w/2, h/2, 0, x+cos(last)*w/2, y+sin(last)*h/2
**       }
**       else
**       {
**          angle = startAngle - 180
**          while (angle > last)
**          {
**             SCWARC_TO_ABS w/2, h/2, 0, x+cos(angle)*w/2, y+sin(angle)*h/2
**             angle -= 180
**          }
**          SCWARC_TO_ABS w/2, h/2, 0, x+cos(last)*w/2, y+sin(last)*h/2
**       }
**       if arcType == VGU_ARC_PIE
**       {
**          LINE_TO_ABS x, y
**       }
**       if arcType == VGU_ARC_PIE || arcType == VGU_ARC_CHORD
**       {
**          CLOSE_PATH
**       }
**
** INPUT:
**
**    Path
**       Handle to a valid VGPath object.
**
**    CenterX, CenterY
**       The center coordinates of the ellipse.
**
**    Width, Height
**       Arc horizontal and vertical diameters.
**
**    StartAngle
**    AngleExtent
**       Arc angles given in degrees.
**
**    ArcType
**       One of:
**          VGU_ARC_OPEN
**          VGU_ARC_CHORD
**          VGU_ARC_PIE
**
** OUTPUT:
**
**    Nothing.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguArc(
	VGPath Path,
	VGfloat CenterX,
	VGfloat CenterY,
	VGfloat Width,
	VGfloat Height,
	VGfloat StartAngle,
	VGfloat AngleExtent,
	VGUArcType ArcType
	)
{
	VGUErrorCode result = VGU_BAD_HANDLE_ERROR;

	vgmENTERAPI(vguArc)
	{
		gceSTATUS status;
		vgsPATH_PTR dstPath;
		vgsPATHWALKER destination;
		VGfloat hRadius, vRadius;
		VGfloat startAngle, angleExtent, lastAngle;

		/* Validate the path object. */
		if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
		{
			result = VGU_BAD_HANDLE_ERROR;
			break;
		}

		/* Cast the object. */
		dstPath = (vgsPATH_PTR) Path;

		/* Verify the capability. */
		if ((dstPath->capabilities & VG_PATH_CAPABILITY_APPEND_TO) == 0)
		{
			result = VGU_PATH_CAPABILITY_ERROR;
			break;
		}

		/* Verify the arguments. */
		if ((Width <= 0) || (Height <= 0))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		if ((ArcType != VGU_ARC_OPEN)  &&
			(ArcType != VGU_ARC_CHORD) &&
			(ArcType != VGU_ARC_PIE))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		/* Compute parameters. */
		hRadius = Width  / 2;
		vRadius = Height / 2;

		/* Initialize the destination walker. */
		vgsPATHWALKER_InitializeWriter(
			Context, Context->pathStorage, &destination, dstPath
			);

		/* Compute parameters. */
		hRadius = Width  / 2;
		vRadius = Height / 2;

		startAngle  = StartAngle  * vgvPI / 180.0f;
		angleExtent = AngleExtent * vgvPI / 180.0f;
		lastAngle   = startAngle + angleExtent;

		/* Add the initial move. */
		gcmERR_GOTO(_MOVE_TO_ABS(
			&destination,
			CenterX + (gctFLOAT) cos(startAngle) * hRadius,
			CenterY + (gctFLOAT) sin(startAngle) * vRadius
			));

		/* Add the arc. */
		if (angleExtent > 0)
		{
			VGfloat angle = startAngle + vgvPI;

			while (angle < lastAngle)
			{
				gcmERR_GOTO(_SMALL_ARC(
					&destination, gcvTRUE, gcvFALSE,
					hRadius, vRadius, 0,
					CenterX + (gctFLOAT) cos(angle) * hRadius,
					CenterY + (gctFLOAT) sin(angle) * vRadius
					));

				angle += vgvPI;
			}

			gcmERR_GOTO(_SMALL_ARC(
				&destination, gcvTRUE, gcvFALSE,
				hRadius, vRadius, 0,
				CenterX + (gctFLOAT) cos(lastAngle) * hRadius,
				CenterY + (gctFLOAT) sin(lastAngle) * vRadius
				));
		}
		else
		{
			VGfloat angle = startAngle - vgvPI;

			while (angle > lastAngle)
			{
				gcmERR_GOTO(_SMALL_ARC(
					&destination, gcvFALSE, gcvFALSE,
					hRadius, vRadius, 0,
					CenterX + (gctFLOAT) cos(angle) * hRadius,
					CenterY + (gctFLOAT) sin(angle) * vRadius
					));

				angle -= vgvPI;
			}

			gcmERR_GOTO(_SMALL_ARC(
				&destination, gcvFALSE, gcvFALSE,
				hRadius, vRadius, 0,
				CenterX + (gctFLOAT) cos(lastAngle) * hRadius,
				CenterY + (gctFLOAT) sin(lastAngle) * vRadius
				));
		}

		/* Pie arc? */
		if (ArcType == VGU_ARC_PIE)
		{
			/* Draw 'pie'. */
			gcmERR_GOTO(_LINE_TO_ABS(&destination, CenterX, CenterY));

			/* Close the path. */
			gcmERR_GOTO(_CLOSE_PATH(&destination));
		}
		else if (ArcType == VGU_ARC_CHORD)
		{
			/* Close the path. */
			gcmERR_GOTO(_CLOSE_PATH(&destination));
		}

		/* Close the buffer. */
		gcmERR_GOTO(vgsPATHWALKER_DoneWriting(&destination));

		/* Success. */
		result = VGU_NO_ERROR;
		break;

ErrorHandler:

		/* Roll back path changes if any. */
		vgsPATHWALKER_Rollback(&destination);

		/* Set error condition. */
		result = VGU_OUT_OF_MEMORY_ERROR;
	}
	vgmLEAVEAPI(vguArc);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vguComputeWarpQuadToSquare
**
** The vguComputeWarpQuadToSquare function sets the entries of matrix to a
** projective transformation that maps the points as follows:
**
**    (SourceX0, SourceY0) to (0, 0);
**    (SourceX1, SourceY1) to (1, 0);
**    (SourceX2, SourceY2) to (0, 1);
**    (SourceX3, SourceY3) to (1, 1).
**
** If no non-degenerate matrix satisfies the constraints, VGU_BAD_WARP_ERROR
** is returned and matrix is unchanged.
**
** INPUT:
**
**    Matrix
**       Pointer to the result matrix.
**
**    SourceX0, SourceY0
**    SourceX1, SourceY1
**    SourceX2, SourceY2
**    SourceX3, SourceY3
**       Source coordinates to be mapped.
**
** OUTPUT:
**
**    Matrix
**       Computed matrix.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguComputeWarpQuadToSquare(
	VGfloat SourceX0,
	VGfloat SourceY0,
	VGfloat SourceX1,
	VGfloat SourceY1,
	VGfloat SourceX2,
	VGfloat SourceY2,
	VGfloat SourceX3,
	VGfloat SourceY3,
	VGfloat * Matrix
	)
{
	VGUErrorCode result = VGU_BAD_HANDLE_ERROR;

	vgmENTERAPI(vguComputeWarpQuadToSquare)
	{
		vgsMATRIX projective;
		vgsMATRIX warped;

		/* Validate the input matrix. */
		if (vgmIS_INVALID_PTR(Matrix, 4))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		/* Reset the projective matrix. */
		vgfInvalidateMatrix(&projective);

#if gcdENABLE_PERFORMANCE_PRIOR
		/* Get projective transformation matrix. */
		result = vguComputeWarpSquareToQuad(
			SourceX0, SourceY0,
			SourceX1, SourceY1,
			SourceX2, SourceY2,
			SourceX3, SourceY3,
			Matrix
			);

		/* Error? */
		if (result != VGU_NO_ERROR)
		{
			break;
		}

		/*convert to fixed-point*/
		vgmCONVERTMAT(gctFIXED, gctFLOAT, vgmFLOAT_TO_FIXED_SPECIAL, &projective, Matrix);

		/* Invert the matrix. */
		if (!vgfInvertMatrix(&projective, &warped))
		{
			result = VGU_BAD_WARP_ERROR;
			break;
		}

		/* Set the result matrix. */
		vgmCONVERTMAT(gctFLOAT, gctFIXED, vgmFIXED_TO_FLOAT_SPECIAL, Matrix, &warped);
#else
		/* Get projective transformation matrix. */
		result = vguComputeWarpSquareToQuad(
			SourceX0, SourceY0,
			SourceX1, SourceY1,
			SourceX2, SourceY2,
			SourceX3, SourceY3,
			vgmGETMATRIXVALUES(&projective)
			);

		/* Error? */
		if (result != VGU_NO_ERROR)
		{
			break;
		}

		/* Invert the matrix. */
		if (!vgfInvertMatrix(&projective, &warped))
		{
			result = VGU_BAD_WARP_ERROR;
			break;
		}

		/* Set the result matrix. */
		gcoOS_MemCopy(
			Matrix,
			vgmGETMATRIXVALUES(&warped),
			sizeof(vgtMATRIXVALUES)
			);
#endif
	}
	vgmLEAVEAPI(vguComputeWarpQuadToSquare);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vguComputeWarpSquareToQuad
**
** The vguComputeWarpSquareToQuad function sets the entries of matrix to a
** projective transformation that maps the points as follows:
**
**    (0, 0) to (DestinationX0, DestinationY0);
**    (1, 0) to (DestinationX1, DestinationY1);
**    (0, 1) to (DestinationX2, DestinationY2);
**    (1, 1) to (DestinationX3, DestinationY3).
**
** If no non-degenerate matrix satisfies theconstraints, VGU_BAD_WARP_ERROR is
** returned and matrix is unchanged.
**
** INPUT:
**
**    Matrix
**       Pointer to the result matrix.
**
**    DestinationX0, DestinationY0
**    DestinationX1, DestinationY1
**    DestinationX2, DestinationY2
**    DestinationX3, DestinationY3
**       Destination coordinates to be mapped.
**
** OUTPUT:
**
**    Matrix
**       Computed matrix.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguComputeWarpSquareToQuad(
	VGfloat DestinationX0,
	VGfloat DestinationY0,
	VGfloat DestinationX1,
	VGfloat DestinationY1,
	VGfloat DestinationX2,
	VGfloat DestinationY2,
	VGfloat DestinationX3,
	VGfloat DestinationY3,
	VGfloat * Matrix
	)
{
	VGUErrorCode result = VGU_NO_ERROR;

	vgmENTERAPI(vguComputeWarpSquareToQuad)
	{
		VGfloat det;
		VGfloat sumX, sumY;
		VGfloat diffX1, diffY1, diffX2, diffY2;

		/* Validate the input matrix. */
		if (vgmIS_INVALID_PTR(Matrix, 4))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		diffX1 = DestinationX1 - DestinationX3;
		diffY1 = DestinationY1 - DestinationY3;
		diffX2 = DestinationX2 - DestinationX3;
		diffY2 = DestinationY2 - DestinationY3;

		/* Compute determinant. */
		det = diffX1 * diffY2 - diffX2 * diffY1;

		if (det == 0.0f)
		{
			result = VGU_BAD_WARP_ERROR;
			break;
		}

		sumX = DestinationX0 - DestinationX1 + DestinationX3 - DestinationX2;
		sumY = DestinationY0 - DestinationY1 + DestinationY3 - DestinationY2;
#if gcdENABLE_PERFORMANCE_PRIOR
		/* Affine result? */
		if ((sumX == 0.0f) && (sumY == 0.0f))
		{
			vgmFLOATMAT(Matrix, 0, 0) = DestinationX1 - DestinationX0;
			vgmFLOATMAT(Matrix, 1, 0) = DestinationY1 - DestinationY0;
			vgmFLOATMAT(Matrix, 2, 0) = 0.0f;

			vgmFLOATMAT(Matrix, 0, 1) = DestinationX3 - DestinationX1;
			vgmFLOATMAT(Matrix, 1, 1) = DestinationY3 - DestinationY1;
			vgmFLOATMAT(Matrix, 2, 1) = 0.0f;

			vgmFLOATMAT(Matrix, 0, 2) = DestinationX0;
			vgmFLOATMAT(Matrix, 1, 2) = DestinationY0;
			vgmFLOATMAT(Matrix, 2, 2) = 1.0f;
		}
		else
		{
			VGfloat detrcp, g, h;

			/* Compute reciprocal of the determinant. */
			detrcp = 1.0f / det;

			g = (sumX * diffY2 - sumY * diffX2) * detrcp;
			h = (sumY * diffX1 - sumX * diffY1) * detrcp;

			vgmFLOATMAT(Matrix, 0, 0) = DestinationX1 - DestinationX0 + g * DestinationX1;
			vgmFLOATMAT(Matrix, 1, 0) = DestinationY1 - DestinationY0 + g * DestinationY1;
			vgmFLOATMAT(Matrix, 2, 0) = g;
			vgmFLOATMAT(Matrix, 0, 1) = DestinationX2 - DestinationX0 + h * DestinationX2;
			vgmFLOATMAT(Matrix, 1, 1) = DestinationY2 - DestinationY0 + h * DestinationY2;
			vgmFLOATMAT(Matrix, 2, 1) = h;
			vgmFLOATMAT(Matrix, 0, 2) = DestinationX0;
			vgmFLOATMAT(Matrix, 1, 2) = DestinationY0;
			vgmFLOATMAT(Matrix, 2, 2) = 1.0f;
		}
#else
		/* Affine result? */
		if ((sumX == 0.0f) && (sumY == 0.0f))
		{
			vgmMAT(Matrix, 0, 0) = DestinationX1 - DestinationX0;
			vgmMAT(Matrix, 1, 0) = DestinationY1 - DestinationY0;
			vgmMAT(Matrix, 2, 0) = 0.0f;

			vgmMAT(Matrix, 0, 1) = DestinationX3 - DestinationX1;
			vgmMAT(Matrix, 1, 1) = DestinationY3 - DestinationY1;
			vgmMAT(Matrix, 2, 1) = 0.0f;

			vgmMAT(Matrix, 0, 2) = DestinationX0;
			vgmMAT(Matrix, 1, 2) = DestinationY0;
			vgmMAT(Matrix, 2, 2) = 1.0f;
		}
		else
		{
			VGfloat detrcp, g, h;

			/* Compute reciprocal of the determinant. */
			detrcp = 1.0f / det;

			g = (sumX * diffY2 - sumY * diffX2) * detrcp;
			h = (sumY * diffX1 - sumX * diffY1) * detrcp;

			vgmMAT(Matrix, 0, 0) = DestinationX1 - DestinationX0 + g * DestinationX1;
			vgmMAT(Matrix, 1, 0) = DestinationY1 - DestinationY0 + g * DestinationY1;
			vgmMAT(Matrix, 2, 0) = g;
			vgmMAT(Matrix, 0, 1) = DestinationX2 - DestinationX0 + h * DestinationX2;
			vgmMAT(Matrix, 1, 1) = DestinationY2 - DestinationY0 + h * DestinationY2;
			vgmMAT(Matrix, 2, 1) = h;
			vgmMAT(Matrix, 0, 2) = DestinationX0;
			vgmMAT(Matrix, 1, 2) = DestinationY0;
			vgmMAT(Matrix, 2, 2) = 1.0f;
		}
#endif
	}
	vgmLEAVEAPI(vguComputeWarpSquareToQuad);

	/* Return result. */
	return result;
}


/*******************************************************************************
**
** vguComputeWarpQuadToQuad
**
** The vguComputeWarpQuadToQuad function sets the entries of matrix to a
** projective transformation that maps the points as follows:
**
**    (SourceX0, SourceY0) to (DestinationX0, DestinationY0);
**    (SourceX1, SourceY1) to (DestinationX1, DestinationY1);
**    (SourceX2, SourceY2) to (DestinationX2, DestinationY2);
**    (SourceX3, SourceY3) to (DestinationX3, DestinationY3).
**
** If no non-degenerate matrix satisfies the constraints, VGU_BAD_WARP_ERROR is
** returned and matrix is unchanged.
**
** INPUT:
**
**    Matrix
**       Pointer to the result matrix.
**
**    DestinationX0, DestinationY0
**    DestinationX1, DestinationY1
**    DestinationX2, DestinationY2
**    DestinationX3, DestinationY3
**       Destination coordinates to be mapped.
**
**    SourceX0, SourceY0
**    SourceX1, SourceY1
**    SourceX2, SourceY2
**    SourceX3, SourceY3
**       Source coordinates to be mapped.
**
** OUTPUT:
**
**    Matrix
**       Computed matrix.
*/

VGU_API_CALL VGUErrorCode VGU_API_ENTRY vguComputeWarpQuadToQuad(
	VGfloat DestinationX0,
	VGfloat DestinationY0,
	VGfloat DestinationX1,
	VGfloat DestinationY1,
	VGfloat DestinationX2,
	VGfloat DestinationY2,
	VGfloat DestinationX3,
	VGfloat DestinationY3,
	VGfloat SourceX0,
	VGfloat SourceY0,
	VGfloat SourceX1,
	VGfloat SourceY1,
	VGfloat SourceX2,
	VGfloat SourceY2,
	VGfloat SourceX3,
	VGfloat SourceY3,
	VGfloat * Matrix
	)
{
	VGUErrorCode result = VGU_BAD_HANDLE_ERROR;

	vgmENTERAPI(vguComputeWarpQuadToQuad)
	{
		vgsMATRIX q2s;
		vgsMATRIX s2q;
		vgsMATRIX product;

#if gcdENABLE_PERFORMANCE_PRIOR
		gctFLOAT m1[9], m2[9];
#endif
		/* Validate the input matrix. */
		if (vgmIS_INVALID_PTR(Matrix, 4))
		{
			result = VGU_ILLEGAL_ARGUMENT_ERROR;
			break;
		}

		/* Reset matrices. */
		vgfInvalidateMatrix(&q2s);
		vgfInvalidateMatrix(&s2q);

#if gcdENABLE_PERFORMANCE_PRIOR
		result = vguComputeWarpQuadToSquare(
			SourceX0, SourceY0,
			SourceX1, SourceY1,
			SourceX2, SourceY2,
			SourceX3, SourceY3,
			m1
			);

		/* Error? */
		if (result != VGU_NO_ERROR)
		{
			break;
		}

		vgmCONVERTMAT(gctFIXED, gctFLOAT, vgmFLOAT_TO_FIXED_SPECIAL, &q2s, m1);

		result = vguComputeWarpSquareToQuad(
			DestinationX0, DestinationY0,
			DestinationX1, DestinationY1,
			DestinationX2, DestinationY2,
			DestinationX3, DestinationY3,
			m2
			);

		/* Error? */
		if (result != VGU_NO_ERROR)
		{
			break;
		}

		vgmCONVERTMAT(gctFIXED, gctFLOAT, vgmFLOAT_TO_FIXED_SPECIAL, &s2q, m2);

		/* Find the product. */
		vgfMultiplyMatrix3x3(&s2q, &q2s, &product);

		/* Set the result. */
		vgmCONVERTMAT(gctFLOAT, gctFIXED, vgmFIXED_TO_FLOAT_SPECIAL, Matrix, &product);
#else
		result = vguComputeWarpQuadToSquare(
			SourceX0, SourceY0,
			SourceX1, SourceY1,
			SourceX2, SourceY2,
			SourceX3, SourceY3,
			vgmGETMATRIXVALUES(&q2s)
			);

		/* Error? */
		if (result != VGU_NO_ERROR)
		{
			break;
		}

		result = vguComputeWarpSquareToQuad(
			DestinationX0, DestinationY0,
			DestinationX1, DestinationY1,
			DestinationX2, DestinationY2,
			DestinationX3, DestinationY3,
			vgmGETMATRIXVALUES(&s2q)
			);

		/* Error? */
		if (result != VGU_NO_ERROR)
		{
			break;
		}

		/* Find the product. */
		vgfMultiplyMatrix3x3(&s2q, &q2s, &product);

		/* Set the result. */
		gcoOS_MemCopy(
			Matrix,
			vgmGETMATRIXVALUES(&product),
			sizeof(vgtMATRIXVALUES)
			);
#endif
	}
	vgmLEAVEAPI(vguComputeWarpQuadToQuad);

	/* Return result. */
	return result;
}
