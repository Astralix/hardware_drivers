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

/* Tesselation segment count. */
#define vgvNUM_TESSELLATED_SEGMENTS 256

/* Segment append function name. */
#define vgmTESSEL(Command) \
	_Tesselate_ ## Command

/* Segment handler function parameters. */
#define vgmTESSHANDLERPARAMETERS \
	vgsPATHWALKER_PTR Source, \
	vgsTESSINFO_PTR Info

/* Segment append function definition. */
#define vgmDEFINETESSELATE(Command) \
	static gceSTATUS vgmTESSEL(Command) ( \
		vgmTESSHANDLERPARAMETERS \
		)

/* Tesselation parameters. */
typedef struct _vgsTESSINFO * vgsTESSINFO_PTR;
typedef struct _vgsTESSINFO
{
	gctBOOL enableUpdate;
	gctBOOL havePoints;
	gctFLOAT distance;

	gctFLOAT positionX;
	gctFLOAT positionY;

	gctFLOAT tangentX;
	gctFLOAT tangentY;

	gctFLOAT length;

	gctFLOAT left;
	gctFLOAT top;
	gctFLOAT right;
	gctFLOAT bottom;

	vgsCONTROL_COORD coords;
}
vgsTESSINFO;

/* Segment handler function type. */
typedef gceSTATUS (* vgtTESSELATEHANDLER) (
	vgmTESSHANDLERPARAMETERS
	);


/*******************************************************************************
**
** _NormalizeVector
**
** Normalize the specified vector by making it unit-long. If the specified
** vector is a zero vector, a zero vector is returned as well.
**
** INPUT:
**
**    VectorX, VectorY
**       Vector to normalize.
**
** OUTPUT:
**
**    ResultX, ResultY
**       Pointers to the unit vector.
*/

static void _NormalizeVector(
	IN gctFLOAT VectorX,
	IN gctFLOAT VectorY,
	OUT gctFLOAT_PTR ResultX,
	OUT gctFLOAT_PTR ResultY
	)
{
	double factor
		= (double) VectorX * (double) VectorX
		+ (double) VectorY * (double) VectorY;

	if (factor != 0.0)
	{
		factor = 1.0 / sqrt(factor);
	}

	* ResultX = (gctFLOAT) (factor * VectorX);
	* ResultY = (gctFLOAT) (factor * VectorY);
}


/*******************************************************************************
**
** _NormalizeVectorDifference
**
** Normalize the specified vector by making it unit-long. If the specified
** vector is a zero vector, a zero vector is returned as well.
**
** INPUT:
**
**    Vector0X, Vector0Y
**    Vector1X, Vector1Y
**       Vectors to normalize.
**
** OUTPUT:
**
**    ResultX, ResultY
**       Pointers to the unit vector.
*/

static void _NormalizeVectorDifference(
	IN gctFLOAT Vector0X,
	IN gctFLOAT Vector0Y,
	IN gctFLOAT Vector1X,
	IN gctFLOAT Vector1Y,
	OUT gctFLOAT_PTR ResultX,
	OUT gctFLOAT_PTR ResultY
	)
{
	gctFLOAT vectorX;
	gctFLOAT vectorY;

	/* Determine the delta vector. */
	vectorX = Vector1X - Vector0X;
	vectorY = Vector1Y - Vector0Y;

	/* Find the normalized vector. */
	_NormalizeVector(vectorX, vectorY, ResultX, ResultY);
}


/*******************************************************************************
**
** _UnitAverage
**
** Form a reliable normalized average of the two unit input vectors.
** The average always lies to the given direction from the first vector.
**
** INPUT:
**
**    Vector0X, Vector0Y
**    Vector1X, Vector1Y
**       Unit vectors.
**
**    CounterClockwise
**       Arc direction.
**
** OUTPUT:
**
**    ResultX, ResultY
**       Pointers to the average unit vector.
*/

static void _UnitAverage(
	IN gctFLOAT Vector0X,
	IN gctFLOAT Vector0Y,
	IN gctFLOAT Vector1X,
	IN gctFLOAT Vector1Y,
	IN gctBOOL CounterClockwise,
	OUT gctFLOAT_PTR ResultX,
	OUT gctFLOAT_PTR ResultY
	)
{
	gctFLOAT length;
	gctFLOAT vectorX, vectorY;
	gctFLOAT normal0X, normal0Y;
	gctFLOAT normal1X, normal1Y;

	/* Compute the average of two vectors.  */
	vectorX = (Vector0X + Vector1X) * 0.5f;
	vectorY = (Vector0Y + Vector1Y) * 0.5f;

	/* Get counterclockwise normal to the first vector. */
	normal0X = -Vector0Y;
	normal0Y =  Vector0X;

	/* Determine the squared length of the average vector. */
	length = vectorX * vectorX + vectorY * vectorY;

	/* Is the average long enough? */
	if (length > 0.25f)
	{
		/*
			The average is long enough and thus reliable.
		*/

		/* Determine the squared distance between two vectors. */
		gctFLOAT distance = normal0X * Vector1X + normal0Y * Vector1Y;

		if (distance < 0.0f)
		{
			/* Choose the larger angle. */
			vectorX = - vectorX;
			vectorY = - vectorY;
		}
	}
	else
	{
		/* The average is too short, use the average of the normals to
		   the vectors instead. */

		/* Get clockwise normal to the second vector. */
		normal1X =  Vector1Y;
		normal1Y = -Vector1X;

		/* Determine the average. */
		vectorX = (normal0X + normal1X) * 0.5f;
		vectorY = (normal0Y + normal1Y) * 0.5f;
	}

	if (!CounterClockwise)
	{
		vectorX = - vectorX;
		vectorY = - vectorY;
	}

	/* Normalize the vector. */
	_NormalizeVector(vectorX, vectorY, ResultX, ResultY);
}


/*******************************************************************************
**
** _CircularLerp
**
** Interpolate the given unit tangent vectors to the given direction on
** a unit circle.
**
** INPUT:
**
**    Vector0, Vector1
**       Unit tangent vectors.
**
**    Ratio
**       Interpolation ratio.
**
**    CounterClockwise
**       Arc direction.
**
** OUTPUT:
**
**    Result
**       Result vector.
*/

static void _CircularLerp(
	IN vgtFLOATVECTOR2 Vector0,
	IN vgtFLOATVECTOR2 Vector1,
	IN gctFLOAT Ratio,
	IN gctBOOL CounterClockwise,
	OUT vgtFLOATVECTOR2 Result
	)
{
	gctUINT i;
	gctFLOAT l0, l1;
	gctFLOAT vector0X, vector0Y;
	gctFLOAT vector1X, vector1Y;
	gctFLOAT averageX, averageY;

	/* Set inintial parameters. */
	vector0X = Vector0[0];
	vector0Y = Vector0[1];

	vector1X = Vector1[0];
	vector1Y = Vector1[1];

	l0 = 0.0f;
	l1 = 1.0f;

	for (i = 0; i < 18; i += 1)
	{
		gctFLOAT l = 0.5f * (l0 + l1);

		_UnitAverage(
			vector0X, vector0Y,
			vector1X, vector1Y,
			CounterClockwise,
			&averageX, &averageY
			);

		if (Ratio < l)
		{
			l1 = l;
			vector1X = averageX;
			vector1Y = averageY;
		}
		else
		{
			l0 = l;
			vector0X = averageX;
			vector0Y = averageY;
		}
	}

	/* Set result. */
	Result[0] = vector0X;
	Result[1] = vector0Y;
}


/*******************************************************************************
**
** _AddPoint
**
** Add a new point to the path.
**
** INPUT:
**
**    Info
**       Pointer to the tesselation structure.
**
**    PositionX, PositionY
**       Position of the current point.
**
**    TangentX, TangentY
**       Tangent of the current point.
**
**    UpdateLength
**       Permission to update the current path length.
**
**    OverrideLastTangent
**       If not zero, the specified tangent will be used as the last instead
**       of the one stored.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _AddPoint(
	IN vgsTESSINFO_PTR Info,
	IN gctFLOAT PositionX,
	IN gctFLOAT PositionY,
	IN gctFLOAT TangentX,
	IN gctFLOAT TangentY,
	IN gctBOOL UpdateLength,
	IN gctBOOL OverrideLastTangent
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	if (Info->enableUpdate)
	{
		if (UpdateLength)
		{
			gctFLOAT deltaX, deltaY;
			gctFLOAT distance, pathLength;

			/* Determine the deltas between the two points. */
			deltaX = PositionX - Info->coords.lastX;
			deltaY = PositionY - Info->coords.lastY;

			/* Determine the distance between two points. */
			distance = (gctFLOAT) sqrt(deltaX * deltaX + deltaY * deltaY);

			/* Determine the path length at the new point. */
			pathLength = gcmMIN(Info->length + distance, gcvMAX_POS_FLOAT);

			/* Does the point in interest belong to the segment? */
			if (pathLength > Info->distance)
			{
				/* Yes, set the status so that all loops will be broken. */
				status = gcvSTATUS_TRUE;

				/* Interpolate linearly between the end points. */
				if (distance != 0.0f)
				{
					/* Compute the offset of the point in interest within
					   the segment. */
					gctFLOAT pointOffset = Info->distance - Info->length;

					/* Compute the scaling ratio. */
					gctFLOAT ratio    = pointOffset / distance;
					gctFLOAT invRatio = 1.0f - ratio;

					PositionX = invRatio * Info->coords.lastX + ratio * PositionX;
					PositionY = invRatio * Info->coords.lastY + ratio * PositionY;

					if (!OverrideLastTangent)
					{
						TangentX = invRatio * Info->tangentX + ratio * TangentX;
						TangentY = invRatio * Info->tangentY + ratio * TangentY;
					}

					pathLength = gcmMIN(
						Info->length + pointOffset, gcvMAX_POS_FLOAT
						);
				}
			}

			/* Update the current information. */
			Info->positionX = PositionX;
			Info->positionY = PositionY;
			Info->tangentX  = TangentX;
			Info->tangentY  = TangentY;
			Info->length    = pathLength;
		}

		Info->left   = gcmMIN(Info->left,   PositionX);
		Info->top    = gcmMIN(Info->top,    PositionY);
		Info->right  = gcmMAX(Info->right,  PositionX);
		Info->bottom = gcmMAX(Info->bottom, PositionY);

		Info->havePoints = gcvTRUE;
	}

	/* Update the last coordinates. */
	Info->coords.lastX = PositionX;
	Info->coords.lastY = PositionY;

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _AddEndPath
**
** Tessellates a end-path segment.
**
** INPUT:
**
**    Info
**       Pointer to the tesselation structure.
**
**    LastX, LastY
**       The last point of the path.
**
**    StartX, StartY
**       The first point of the path.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _AddEndPath(
	IN vgsTESSINFO_PTR Info,
	IN gctFLOAT LastX,
	IN gctFLOAT LastY,
	IN gctFLOAT StartX,
	IN gctFLOAT StartY
	)
{
	gceSTATUS status;
	gctFLOAT tangentX, tangentY;

	/* Collapsed end-of-path line? */
	if ((LastX == StartX) && (LastY == StartY))
	{
		return _AddPoint(
			Info,
			StartX, StartY,
			Info->tangentX, Info->tangentY,
			gcvTRUE,
			gcvTRUE
			);
	}

	/* Compute the tangent. */
	_NormalizeVectorDifference(
		LastX, LastY,
		StartX, StartY,
		&tangentX, &tangentY
		);

	/* Compute the point. */
	status = _AddPoint(
		Info,
		StartX, StartY,
		tangentX, tangentY,
		gcvTRUE,
		gcvTRUE
		);

	return status;
}


/*******************************************************************************
**
** _AddLineTo
**
** Tessellates a line-to segment.
**
** INPUT:
**
**    Info
**       Pointer to the tesselation structure.
**
**    LastX, LastY
**       The last point of the path.
**
**    LineToX, LineToY
**       The line coordinates.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _AddLineTo(
	IN vgsTESSINFO_PTR Info,
	IN gctFLOAT LastX,
	IN gctFLOAT LastY,
	IN gctFLOAT LineToX,
	IN gctFLOAT LineToY
	)
{
	gceSTATUS status;
	gctFLOAT tangentX, tangentY;

	/* Collapsed line? */
	if ((LastX == LineToX) && (LastY == LineToY))
	{
		return _AddPoint(
			Info,
			LineToX, LineToY,
			Info->tangentX, Info->tangentY,
			gcvTRUE,
			gcvTRUE
			);
	}

	/* Compute the tangent. */
	_NormalizeVectorDifference(
		LastX, LastY,
		LineToX, LineToY,
		&tangentX, &tangentY
		);

	/* Compute the point. */
	status = _AddPoint(
		Info,
		LineToX, LineToY,
		tangentX, tangentY,
		gcvTRUE,
		gcvTRUE
		);

	return status;
}


/*******************************************************************************
**
** _AddQuadTo
**
** Tessellates a quad-to segment.
**
** INPUT:
**
**    Info
**       Pointer to the tesselation structure.
**
**    LastX, LastY
**       The last point of the path.
**
**    ControlX, ControlY
**       The coordinates of the control point.
**
**    QuadToX, QuadToY
**       The quad coordinates.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _AddQuadTo(
	IN vgsTESSINFO_PTR Info,
	IN gctFLOAT LastX,
	IN gctFLOAT LastY,
	IN gctFLOAT ControlX,
	IN gctFLOAT ControlY,
	IN gctFLOAT QuadToX,
	IN gctFLOAT QuadToY
	)
{
	gctFLOAT incomingTangentX, incomingTangentY;
	gctFLOAT outgoingTangentX, outgoingTangentY;
	gctFLOAT currPointX, currPointY;
	gctFLOAT nextPointX, nextPointY;
	gctFLOAT currTangentX, currTangentY;
	gctFLOAT nextTangentX, nextTangentY;
	gceSTATUS status;
	gctUINT i;

	if ((LastX == ControlX) && (LastY == ControlY))
	{
		if ((ControlX == QuadToX) && (ControlY == QuadToY))
		{
			return _AddPoint(
				Info,
				QuadToX, QuadToY,
				Info->tangentX, Info->tangentY,
				gcvTRUE,
				gcvTRUE
				);
		}
		else
		{
			_NormalizeVectorDifference(
				LastX, LastY, QuadToX, QuadToY,
				&incomingTangentX, &incomingTangentY
				);

			_NormalizeVectorDifference(
				ControlX, ControlY, QuadToX, QuadToY,
				&outgoingTangentX, &outgoingTangentY
				);
		}
	}
	else
	{
		_NormalizeVectorDifference(
			LastX, LastY, ControlX, ControlY,
			&incomingTangentX, &incomingTangentY
			);

		if ((ControlX == QuadToX) && (ControlY == QuadToY))
		{
			_NormalizeVectorDifference(
				LastX, LastY, QuadToX, QuadToY,
				&outgoingTangentX, &outgoingTangentY
				);
		}
		else
		{
			_NormalizeVectorDifference(
				ControlX, ControlY, QuadToX, QuadToY,
				&outgoingTangentX, &outgoingTangentY
				);
		}
	}

	/* Set the initial point. */
	currPointX   = LastX;
	currPointY   = LastY;
	currTangentX = incomingTangentX;
	currTangentY = incomingTangentY;

	for (i = 1; i < vgvNUM_TESSELLATED_SEGMENTS;i++)
	{
		gctFLOAT t  = (gctFLOAT) i / vgvNUM_TESSELLATED_SEGMENTS;
		gctFLOAT u  = 1.0f - t;

		gctFLOAT pTerm1 = u * u;
		gctFLOAT pTerm2 = t * u * 2.0f;
		gctFLOAT pTerm3 = t * t;

		gctFLOAT tTerm1 = t - 1.0f;
		gctFLOAT tTerm2 = 1.0f - 2.0f * t;

		nextPointX = LastX * pTerm1 + ControlX * pTerm2 + QuadToX * pTerm3;
		nextPointY = LastY * pTerm1 + ControlY * pTerm2 + QuadToY * pTerm3;

		nextTangentX = LastX * tTerm1 + ControlX * tTerm2 + QuadToX * t;
		nextTangentY = LastY * tTerm1 + ControlY * tTerm2 + QuadToY * t;

		_NormalizeVector(
			 nextTangentX,  nextTangentY,
			&nextTangentX, &nextTangentY
			);

		if ((nextTangentX == 0.0f) && (nextTangentY == 0.0f))
		{
			nextTangentX = currTangentX;
			nextTangentY = currTangentY;
		}

		status = _AddPoint(
			Info,
			currPointX, currPointY,
			currTangentX, currTangentY,
			gcvTRUE,
			gcvFALSE
			);

		if (status == gcvSTATUS_TRUE)
		{
			return gcvSTATUS_TRUE;
		}

		currPointX   = nextPointX;
		currPointY   = nextPointY;
		currTangentX = nextTangentX;
		currTangentY = nextTangentY;
	}

	status = _AddPoint(
		Info,
		QuadToX, QuadToY,
		outgoingTangentX, outgoingTangentY,
		gcvTRUE,
		gcvFALSE
		);

	return status;
}


/*******************************************************************************
**
** _AddCubicTo
**
** Tessellates a cubic-to segment.
**
** INPUT:
**
**    Info
**       Pointer to the tesselation structure.
**
**    LastX, LastY
**       The last point of the path.
**
**    Control1X, Control1Y
**    Control2X, Control2Y
**       The coordinates of the control points.
**
**    QuadToX, QuadToY
**       The quad coordinates.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _AddCubicTo(
	IN vgsTESSINFO_PTR Info,
	IN gctFLOAT LastX,
	IN gctFLOAT LastY,
	IN gctFLOAT Control1X,
	IN gctFLOAT Control1Y,
	IN gctFLOAT Control2X,
	IN gctFLOAT Control2Y,
	IN gctFLOAT CubicToX,
	IN gctFLOAT CubicToY
	)
{
	gctFLOAT incomingTangentX, incomingTangentY;
	gctFLOAT outgoingTangentX, outgoingTangentY;
	gctFLOAT currPointX, currPointY;
	gctFLOAT nextPointX, nextPointY;
	gctFLOAT currTangentX, currTangentY;
	gctFLOAT nextTangentX, nextTangentY;
	gceSTATUS status;
	gctUINT i;

	if ((LastX == Control1X) && (LastY == Control1Y))
	{
		if ((Control1X == Control2X) && (Control1Y == Control2Y))
		{
			if ((Control2X == CubicToX) && (Control2Y == CubicToY))
			{
				return _AddPoint(
					Info,
					CubicToX, CubicToY,
					Info->tangentX, Info->tangentY,
					gcvTRUE,
					gcvTRUE
					);
			}
			else
			{
				_NormalizeVectorDifference(
					LastX, LastY, CubicToX, CubicToY,
					&incomingTangentX, &incomingTangentY
					);

				outgoingTangentX = incomingTangentX;
				outgoingTangentY = incomingTangentY;
			}
		}
		else
		{
			_NormalizeVectorDifference(
				LastX, LastY, Control2X, Control2Y,
				&incomingTangentX, &incomingTangentY
				);

			if ((Control2X == CubicToX) && (Control2Y == CubicToY))
			{
				_NormalizeVectorDifference(
					Control1X, Control1Y, CubicToX, CubicToY,
					&outgoingTangentX, &outgoingTangentY
					);
			}
			else
			{
				_NormalizeVectorDifference(
					Control2X, Control2Y, CubicToX, CubicToY,
					&outgoingTangentX, &outgoingTangentY
					);
			}
		}
	}
	else
	{
		_NormalizeVectorDifference(
			LastX, LastY, Control1X, Control1Y,
			&incomingTangentX, &incomingTangentY
			);

		if ((Control2X == CubicToX) && (Control2Y == CubicToY))
		{
			if ((Control1X == Control2X) && (Control1Y == Control2Y))
			{
				outgoingTangentX = incomingTangentX;
				outgoingTangentY = incomingTangentY;
			}
			else
			{
				_NormalizeVectorDifference(
					Control1X, Control1Y, CubicToX, CubicToY,
					&outgoingTangentX, &outgoingTangentY
					);
			}
		}
		else
		{
			_NormalizeVectorDifference(
				Control2X, Control2Y, CubicToX, CubicToY,
				&outgoingTangentX, &outgoingTangentY
				);
		}
	}

	/* Set the initial point. */
	currPointX   = LastX;
	currPointY   = LastY;
	currTangentX = incomingTangentX;
	currTangentY = incomingTangentY;

	for (i = 1; i < vgvNUM_TESSELLATED_SEGMENTS;i++)
	{
		gctFLOAT t  = (gctFLOAT) i / vgvNUM_TESSELLATED_SEGMENTS;
		gctFLOAT t2 = t * t;
		gctFLOAT t3 = t * t2;

		gctFLOAT pTerm1 =  1.0f -  3.0f * t + 3.0f * t2 -        t3;
		gctFLOAT pTerm2 =          3.0f * t - 6.0f * t2 + 3.0f * t3;
		gctFLOAT pTerm3 =                     3.0f * t2 - 3.0f * t3;
		gctFLOAT pTerm4 =                                        t3;

		gctFLOAT tTerm1 = -3.0f +  6.0f * t - 3.0f * t2;
		gctFLOAT tTerm2 =  3.0f - 12.0f * t + 9.0f * t2;
		gctFLOAT tTerm3 =          6.0f * t - 9.0f * t2;
		gctFLOAT tTerm4 =                     3.0f * t2;

		nextPointX
			= pTerm1 * LastX
			+ pTerm2 * Control1X
			+ pTerm3 * Control2X
			+ pTerm4 * CubicToX;

		nextPointY
			= pTerm1 * LastY
			+ pTerm2 * Control1Y
			+ pTerm3 * Control2Y
			+ pTerm4 * CubicToY;

		nextTangentX
			= tTerm1 * LastX
			+ tTerm2 * Control1X
			+ tTerm3 * Control2X
			+ tTerm4 * CubicToX;

		nextTangentY
			= tTerm1 * LastY
			+ tTerm2 * Control1Y
			+ tTerm3 * Control2Y
			+ tTerm4 * CubicToY;

		_NormalizeVector(
			 nextTangentX,  nextTangentY,
			&nextTangentX, &nextTangentY
			);

		if ((nextTangentX == 0.0f) && (nextTangentY == 0.0f))
		{
			nextTangentX = currTangentX;
			nextTangentY = currTangentY;
		}

		status = _AddPoint(
			Info,
			currPointX, currPointY,
			currTangentX, currTangentY,
			gcvTRUE,
			gcvFALSE
			);

		if (status == gcvSTATUS_TRUE)
		{
			return gcvSTATUS_TRUE;
		}

		currPointX   = nextPointX;
		currPointY   = nextPointY;
		currTangentX = nextTangentX;
		currTangentY = nextTangentY;
	}

	status = _AddPoint(
		Info,
		CubicToX, CubicToY,
		outgoingTangentX, outgoingTangentY,
		gcvTRUE,
		gcvFALSE
		);

	return status;
}


/*******************************************************************************
**
** _ComputeEllipse
**
** Finds an ellipse center and transformation from the unit circle to
** that ellipse.
**
** INPUT:
**
**    Info
**       Pointer to the tesselation structure.
**
**    HorRadius
**       Length of the horizontal axis.
**
**    VerRadius
**       Length of the vertical axis.
**
**    RotAngle
**       Rotation angle.
**
**    UserEndPoint1X, UserEndPoint1Y
**       User space end point of the arc.
**
** OUTPUT:
**
**    Center0, Center1
**       Unit circle space center points of the two ellipses.
**
**    EndPoint0, EndPoint1
**       Unit circle space end points of the arc.
**
**    UunitCircleToEllipse
**       A matrix mapping from unit circle space to user space.
**
** RETURN:
**
**    Not zero if ellipse exists, zero if doesn't.
*/

static gctBOOL _ComputeEllipse(
	IN gctFLOAT HorRadius,
	IN gctFLOAT VerRadius,
	IN gctFLOAT RotAngle,
	IN gctFLOAT UserEndPoint1X,
	IN gctFLOAT UserEndPoint1Y,
	IN gctBOOL CounterClockwise,
	IN gctBOOL Large,
	OUT vgtFLOATVECTOR2 Center0,
	OUT vgtFLOATVECTOR2 Center1,
	OUT vgtFLOATVECTOR2 EndPoint0,
	OUT vgtFLOATVECTOR2 EndPoint1,
	OUT vgsMATRIX_PTR UnitCircleToEllipse
	)
{
#if gcdENABLE_PERFORMANCE_PRIOR
	vgsMATRIX ellipseToUnitCircle;
	vgtFLOATVECTOR2 endPoint0;
	vgtFLOATVECTOR2 endPoint1;
	vgtFLOATVECTOR2 delta;
	vgtFLOATVECTOR2 m;
	vgtFLOATVECTOR2 sd;
	vgtFLOATVECTOR2 sp;
	vgtFLOATVECTOR2 cp;
	gctFLOAT deltaLength2;
	gctFLOAT disc;

	gctFIXED fixedHorRadius;
	gctFIXED fixedVerRadius;
	gctFIXED fixedRotAngle;

	HorRadius = gcmABS(HorRadius);
	VerRadius = gcmABS(VerRadius);

	/* Degenerate ellipse. */
	if ((HorRadius == 0.0f) || (VerRadius == 0.0f) ||
		((UserEndPoint1X == 0.0f) && (UserEndPoint1Y == 0.0f)))
	{
		return gcvFALSE;
	}
	fixedHorRadius = vgmFLOAT_TO_FIXED_SPECIAL(HorRadius);
	fixedVerRadius = vgmFLOAT_TO_FIXED_SPECIAL(VerRadius);

	/* Convert to radians. */
	fixedRotAngle = vgmFIXED_MUL(vgmFLOAT_TO_FIXED_SPECIAL(RotAngle),  vgvFIXED_PI_DIV_180);

	/* First row. */
	vgmMAT(UnitCircleToEllipse, 0, 0) =  vgmFIXED_MUL(cosx(fixedRotAngle), fixedHorRadius);
	vgmMAT(UnitCircleToEllipse, 0, 1) =  vgmFIXED_MUL(vgmINT_TO_FIXED(-1),  vgmFIXED_MUL(sinx(fixedRotAngle), fixedVerRadius));
	vgmMAT(UnitCircleToEllipse, 0, 2) =  vgvFIXED_ZERO;

	/* Second row. */
	vgmMAT(UnitCircleToEllipse, 1, 0) = vgmFIXED_MUL(sinx(fixedRotAngle), fixedHorRadius);
	vgmMAT(UnitCircleToEllipse, 1, 1) = vgmFIXED_MUL(cosx(fixedRotAngle), fixedVerRadius);
	vgmMAT(UnitCircleToEllipse, 1, 2) = vgvFIXED_ZERO;

	/* Third row. */
	vgmMAT(UnitCircleToEllipse, 2, 0) = vgvFIXED_ZERO;
	vgmMAT(UnitCircleToEllipse, 2, 1) = vgvFIXED_ZERO;
	vgmMAT(UnitCircleToEllipse, 2, 2) = vgvFIXED_ONE;

	/* Compute ellipse-to-unit circle matrix. */
	gcmVERIFY(vgfInvertMatrix(UnitCircleToEllipse, &ellipseToUnitCircle));
	vgmMAT(&ellipseToUnitCircle, 2, 0) = vgvFIXED_ZERO;
	vgmMAT(&ellipseToUnitCircle, 2, 1) = vgvFIXED_ZERO;
	vgmMAT(&ellipseToUnitCircle, 2, 2) = vgvFIXED_ONE;

	/* Initialize end point vectors. */
	endPoint0[0] = 0.0f;
	endPoint0[1] = 0.0f;
	endPoint1[0] = UserEndPoint1X;
	endPoint1[1] = UserEndPoint1Y;

	/* Transform endPoint0 and endPoint1 into unit space. */
	vgfMultiplyVector2ByMatrix3x2(endPoint0, &ellipseToUnitCircle, EndPoint0);
	vgfMultiplyVector2ByMatrix3x2(endPoint1, &ellipseToUnitCircle, EndPoint1);

	/* Solve for intersecting unit circles. */
	delta[0] = EndPoint0[0] - EndPoint1[0];
	delta[1] = EndPoint0[1] - EndPoint1[1];

	/* Compute the squared length of the delta vector. */
	deltaLength2 = delta[0] * delta[0] + delta[1] * delta[1];

	/* The points are coincident. */
	if (deltaLength2 == 0.0f)
	{
		return gcvFALSE;
	}

	disc = (1.0f / deltaLength2) - 0.25f;

	if (disc < 0.0f)
	{
		/* The points are too far apart for a solution to exist,
		   scale the axes so that there is a solution. */

		gctFIXED length = vgmFLOAT_TO_FIXED_SPECIAL((gctFLOAT)sqrt(deltaLength2));

		length =  vgmFIXED_MUL(length, vgvFIXED_HALF);

		fixedHorRadius = vgmFIXED_MUL(fixedHorRadius, length);
		fixedVerRadius = vgmFIXED_MUL(fixedVerRadius, length);

		/*
			Redo the computation with scaled axes.
		*/

		/* First row. */
		vgmMAT(UnitCircleToEllipse, 0, 0) = vgmFIXED_MUL(cosx(fixedRotAngle), fixedHorRadius);
		vgmMAT(UnitCircleToEllipse, 0, 1) = vgmFIXED_MUL(vgmINT_TO_FIXED(-1),  vgmFIXED_MUL(sinx(fixedRotAngle), fixedVerRadius));
		vgmMAT(UnitCircleToEllipse, 0, 2) =  vgvFIXED_ZERO;

		/* Second row. */
		vgmMAT(UnitCircleToEllipse, 1, 0) = vgmFIXED_MUL(sinx(fixedRotAngle), fixedHorRadius);
		vgmMAT(UnitCircleToEllipse, 1, 1) = vgmFIXED_MUL(cosx(fixedRotAngle), fixedVerRadius);
		vgmMAT(UnitCircleToEllipse, 1, 2) = vgvFIXED_ZERO;

		/* Third row. */
		vgmMAT(UnitCircleToEllipse, 2, 0) = vgvFIXED_ZERO;
		vgmMAT(UnitCircleToEllipse, 2, 1) = vgvFIXED_ZERO;
		vgmMAT(UnitCircleToEllipse, 2, 2) = vgvFIXED_ONE;

		/* Compute ellipse-to-unit circle matrix. */
		gcmVERIFY(vgfInvertMatrix(UnitCircleToEllipse, &ellipseToUnitCircle));
		vgmMAT(&ellipseToUnitCircle, 2, 0) = vgvFIXED_ZERO;
		vgmMAT(&ellipseToUnitCircle, 2, 1) = vgvFIXED_ZERO;
		vgmMAT(&ellipseToUnitCircle, 2, 2) = vgvFIXED_ONE;

		/* Transform endPoint0 and endPoint1 into unit space. */
		vgfMultiplyVector2ByMatrix3x2(endPoint0, &ellipseToUnitCircle, EndPoint0);
		vgfMultiplyVector2ByMatrix3x2(endPoint1, &ellipseToUnitCircle, EndPoint1);

		/* Solve for intersecting unit circles. */
		delta[0] = EndPoint0[0] - EndPoint1[0];
		delta[1] = EndPoint0[1] - EndPoint1[1];

		/* Compute the squared length of the delta vector. */
		deltaLength2 = delta[0] * delta[0] + delta[1] * delta[1];

		/* The points are coincident. */
		if (deltaLength2 == 0.0f)
		{
			return gcvFALSE;
		}

		disc = gcmMAX(0.0f, 1.0f / deltaLength2 - 0.25f);
	}

	m[0] = 0.5f * (EndPoint0[0] + EndPoint1[0]);
	m[1] = 0.5f * (EndPoint0[1] + EndPoint1[1]);

	sd[0] = delta[0] * (gctFLOAT) sqrt(disc);
	sd[1] = delta[1] * (gctFLOAT) sqrt(disc);

	sp[0] =  sd[1];
	sp[1] = -sd[0];

	Center0[0] = m[0] + sp[0];
	Center0[1] = m[1] + sp[1];

	Center1[0] = m[0] - sp[0];
	Center1[1] = m[1] - sp[1];

	/* Choose the center point and direction. */
	if (CounterClockwise == Large)
	{
		cp[0] = Center1[0];
		cp[1] = Center1[1];
	}
	else
	{
		cp[0] = Center0[0];
		cp[1] = Center0[1];
	}

	/* Move the unit circle origin to the chosen center point. */
	EndPoint0[0] -= cp[0];
	EndPoint0[1] -= cp[1];

	EndPoint1[0] -= cp[0];
	EndPoint1[1] -= cp[1];

	if (((EndPoint0[0] == EndPoint1[0]) && (EndPoint0[1] == EndPoint1[1])) ||
		((EndPoint0[0] == 0.0f) && (EndPoint0[1] == 0.0f)) ||
		((EndPoint1[0] == 0.0f) && (EndPoint1[1] == 0.0f)))
	{
		return gcvFALSE;
	}

	/* Transform back to the original coordinate space. */
	vgfMultiplyVector2ByMatrix3x2(cp, UnitCircleToEllipse, cp);

	vgmMAT(UnitCircleToEllipse, 0, 2) = vgmFLOAT_TO_FIXED_SPECIAL(cp[0]);
	vgmMAT(UnitCircleToEllipse, 1, 2) = vgmFLOAT_TO_FIXED_SPECIAL(cp[1]);

	return gcvTRUE;
#else
	vgsMATRIX ellipseToUnitCircle;
	vgtFLOATVECTOR2 endPoint0;
	vgtFLOATVECTOR2 endPoint1;
	vgtFLOATVECTOR2 delta;
	vgtFLOATVECTOR2 m;
	vgtFLOATVECTOR2 sd;
	vgtFLOATVECTOR2 sp;
	vgtFLOATVECTOR2 cp;
	gctFLOAT deltaLength2;
	gctFLOAT disc;

	HorRadius = gcmABS(HorRadius);
	VerRadius = gcmABS(VerRadius);

	/* Degenerate ellipse. */
	if ((HorRadius == 0.0f) || (VerRadius == 0.0f) ||
		((UserEndPoint1X == 0.0f) && (UserEndPoint1Y == 0.0f)))
	{
		return gcvFALSE;
	}

	/* Convert to radians. */
	RotAngle = RotAngle / 180.0f * vgvPI;

	/* First row. */
	vgmMAT(UnitCircleToEllipse, 0, 0) =  gcmCOSF(RotAngle) * HorRadius;
	vgmMAT(UnitCircleToEllipse, 0, 1) = -gcmSINF(RotAngle) * VerRadius;
	vgmMAT(UnitCircleToEllipse, 0, 2) =  0.0f;

	/* Second row. */
	vgmMAT(UnitCircleToEllipse, 1, 0) = gcmSINF(RotAngle) * HorRadius;
	vgmMAT(UnitCircleToEllipse, 1, 1) = gcmCOSF(RotAngle) * VerRadius;
	vgmMAT(UnitCircleToEllipse, 1, 2) = 0.0f;

	/* Third row. */
	vgmMAT(UnitCircleToEllipse, 2, 0) = 0.0f;
	vgmMAT(UnitCircleToEllipse, 2, 1) = 0.0f;
	vgmMAT(UnitCircleToEllipse, 2, 2) = 1.0f;

	/* Compute ellipse-to-unit circle matrix. */
	gcmVERIFY(vgfInvertMatrix(UnitCircleToEllipse, &ellipseToUnitCircle));
	vgmMAT(&ellipseToUnitCircle, 2, 0) = 0.0f;
	vgmMAT(&ellipseToUnitCircle, 2, 1) = 0.0f;
	vgmMAT(&ellipseToUnitCircle, 2, 2) = 1.0f;

	/* Initialize end point vectors. */
	endPoint0[0] = 0.0f;
	endPoint0[1] = 0.0f;
	endPoint1[0] = UserEndPoint1X;
	endPoint1[1] = UserEndPoint1Y;

	/* Transform endPoint0 and endPoint1 into unit space. */
	vgfMultiplyVector2ByMatrix3x2(endPoint0, &ellipseToUnitCircle, EndPoint0);
	vgfMultiplyVector2ByMatrix3x2(endPoint1, &ellipseToUnitCircle, EndPoint1);

	/* Solve for intersecting unit circles. */
	delta[0] = EndPoint0[0] - EndPoint1[0];
	delta[1] = EndPoint0[1] - EndPoint1[1];

	/* Compute the squared length of the delta vector. */
	deltaLength2 = delta[0] * delta[0] + delta[1] * delta[1];

	/* The points are coincident. */
	if (deltaLength2 == 0.0f)
	{
		return gcvFALSE;
	}

	disc = (1.0f / deltaLength2) - 0.25f;

	if (disc < 0.0f)
	{
		/* The points are too far apart for a solution to exist,
		   scale the axes so that there is a solution. */

		gctFLOAT length = (gctFLOAT) sqrt(deltaLength2);

		length *= 0.5f;

		HorRadius *= length;
		VerRadius *= length;

		/*
			Redo the computation with scaled axes.
		*/

		/* First row. */
		vgmMAT(UnitCircleToEllipse, 0, 0) =  gcmCOSF(RotAngle) * HorRadius;
		vgmMAT(UnitCircleToEllipse, 0, 1) = -gcmSINF(RotAngle) * VerRadius;
		vgmMAT(UnitCircleToEllipse, 0, 2) =  0.0f;

		/* Second row. */
		vgmMAT(UnitCircleToEllipse, 1, 0) = gcmSINF(RotAngle) * HorRadius;
		vgmMAT(UnitCircleToEllipse, 1, 1) = gcmCOSF(RotAngle) * VerRadius;
		vgmMAT(UnitCircleToEllipse, 1, 2) = 0.0f;

		/* Third row. */
		vgmMAT(UnitCircleToEllipse, 2, 0) = 0.0f;
		vgmMAT(UnitCircleToEllipse, 2, 1) = 0.0f;
		vgmMAT(UnitCircleToEllipse, 2, 2) = 1.0f;

		/* Compute ellipse-to-unit circle matrix. */
		gcmVERIFY(vgfInvertMatrix(UnitCircleToEllipse, &ellipseToUnitCircle));
		vgmMAT(&ellipseToUnitCircle, 2, 0) = 0.0f;
		vgmMAT(&ellipseToUnitCircle, 2, 1) = 0.0f;
		vgmMAT(&ellipseToUnitCircle, 2, 2) = 1.0f;

		/* Transform endPoint0 and endPoint1 into unit space. */
		vgfMultiplyVector2ByMatrix3x2(endPoint0, &ellipseToUnitCircle, EndPoint0);
		vgfMultiplyVector2ByMatrix3x2(endPoint1, &ellipseToUnitCircle, EndPoint1);

		/* Solve for intersecting unit circles. */
		delta[0] = EndPoint0[0] - EndPoint1[0];
		delta[1] = EndPoint0[1] - EndPoint1[1];

		/* Compute the squared length of the delta vector. */
		deltaLength2 = delta[0] * delta[0] + delta[1] * delta[1];

		/* The points are coincident. */
		if (deltaLength2 == 0.0f)
		{
			return gcvFALSE;
		}

		disc = gcmMAX(0.0f, 1.0f / deltaLength2 - 0.25f);
	}

	m[0] = 0.5f * (EndPoint0[0] + EndPoint1[0]);
	m[1] = 0.5f * (EndPoint0[1] + EndPoint1[1]);

	sd[0] = delta[0] * (gctFLOAT) sqrt(disc);
	sd[1] = delta[1] * (gctFLOAT) sqrt(disc);

	sp[0] =  sd[1];
	sp[1] = -sd[0];

	Center0[0] = m[0] + sp[0];
	Center0[1] = m[1] + sp[1];

	Center1[0] = m[0] - sp[0];
	Center1[1] = m[1] - sp[1];

	/* Choose the center point and direction. */
	if (CounterClockwise == Large)
	{
		cp[0] = Center1[0];
		cp[1] = Center1[1];
	}
	else
	{
		cp[0] = Center0[0];
		cp[1] = Center0[1];
	}

	/* Move the unit circle origin to the chosen center point. */
	EndPoint0[0] -= cp[0];
	EndPoint0[1] -= cp[1];

	EndPoint1[0] -= cp[0];
	EndPoint1[1] -= cp[1];

	if (((EndPoint0[0] == EndPoint1[0]) && (EndPoint0[1] == EndPoint1[1])) ||
		((EndPoint0[0] == 0.0f) && (EndPoint0[1] == 0.0f)) ||
		((EndPoint1[0] == 0.0f) && (EndPoint1[1] == 0.0f)))
	{
		return gcvFALSE;
	}

	/* Transform back to the original coordinate space. */
	vgfMultiplyVector2ByMatrix3x2(cp, UnitCircleToEllipse, cp);

	vgmMAT(UnitCircleToEllipse, 0, 2) = cp[0];
	vgmMAT(UnitCircleToEllipse, 1, 2) = cp[1];

	return gcvTRUE;
#endif
}


/*******************************************************************************
**
** _AddArcTo
**
** Tessellates an arc-to segment.
**
** INPUT:
**
**    Info
**       Pointer to the tesselation structure.
**
**    LastX, LastY
**       The last point of the path.
**
**    HorRadius, VerRadius
**       Length of the horizontal and vertical axis.
**
**    RotAngle
**       Rotation angle.
**
**    ArcToXRel, ArcToYRel
**    ArcToXAbs, ArcToYAbs
**       ARC end point.
**
**    CounterClockwise
**       Arc direction.
**
**    Large
**       Arc size.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _AddArcTo(
	IN vgsTESSINFO_PTR Info,
	IN gctFLOAT LastX,
	IN gctFLOAT LastY,
	IN gctFLOAT HorRadius,
	IN gctFLOAT VerRadius,
	IN gctFLOAT RotAngle,
	IN gctFLOAT ArcToXRel,
	IN gctFLOAT ArcToYRel,
	IN gctFLOAT ArcToXAbs,
	IN gctFLOAT ArcToYAbs,
	IN gctBOOL CounterClockwise,
	IN gctBOOL Large
	)
{
	gctBOOL success;
	vgtFLOATVECTOR2 center0;
	vgtFLOATVECTOR2 center1;
	vgtFLOATVECTOR2 endPoint0;
	vgtFLOATVECTOR2 endPoint1;
	vgsMATRIX unitCircleToEllipse;
	vgtFLOATVECTOR2 incomingTangent;
	vgtFLOATVECTOR2 outgoingTangent;
	gctFLOAT currPointX, currPointY;
	gctFLOAT currTangentX, currTangentY;
	vgtFLOATVECTOR2 nextPoint;
	vgtFLOATVECTOR2 nextTangent;
	gceSTATUS status;
	gctUINT i;

	if ((LastX == ArcToXAbs) && (LastY == ArcToYAbs))
	{
		return _AddPoint(
			Info,
			ArcToXAbs, ArcToYAbs,
			Info->tangentX, Info->tangentY,
			gcvTRUE,
			gcvTRUE
			);
	}

	/* Reset matrix. */
	vgfInvalidateMatrix(&unitCircleToEllipse);

	/* Compute ellipse parameters. */
	success = _ComputeEllipse(
		HorRadius, VerRadius,
		RotAngle,
		ArcToXRel, ArcToYRel,
		CounterClockwise,
		Large,
		center0, center1,
		endPoint0, endPoint1,
		&unitCircleToEllipse
		);

	if (!success)
	{
		/* Ellipses don't exist, add line instead. */
		gctFLOAT tangentX, tangentY;

		_NormalizeVector(
			ArcToXRel, ArcToYRel,
			&tangentX, &tangentY
			);

		return _AddPoint(
			Info,
			ArcToXAbs, ArcToYAbs,
			tangentX, tangentY,
			gcvTRUE,
			gcvTRUE
			);
	}

	/* Compute end point tangents. */
	if (CounterClockwise)
	{
		incomingTangent[0] = -endPoint0[1];
		incomingTangent[1] =  endPoint0[0];

		outgoingTangent[0] = -endPoint1[1];
		outgoingTangent[1] =  endPoint1[0];
	}
	else
	{
		incomingTangent[0] =  endPoint0[1];
		incomingTangent[1] = -endPoint0[0];

		outgoingTangent[0] =  endPoint1[1];
		outgoingTangent[1] = -endPoint1[0];
	}

	vgfMultiplyVector2ByMatrix2x2(
		incomingTangent, &unitCircleToEllipse, incomingTangent
		);

	vgfMultiplyVector2ByMatrix2x2(
		outgoingTangent, &unitCircleToEllipse, outgoingTangent
		);

	_NormalizeVector(
		 incomingTangent[0],  incomingTangent[1],
		&incomingTangent[0], &incomingTangent[1]
		);

	_NormalizeVector(
		 outgoingTangent[0],  outgoingTangent[1],
		&outgoingTangent[0], &outgoingTangent[1]
		);

	/* Set the initial point. */
	currPointX   = LastX;
	currPointY   = LastY;
	currTangentX = incomingTangent[0];
	currTangentY = incomingTangent[1];

	for (i = 1; i < vgvNUM_TESSELLATED_SEGMENTS;i++)
	{
		gctFLOAT t = (gctFLOAT) i / vgvNUM_TESSELLATED_SEGMENTS;

		_CircularLerp(
			endPoint0, endPoint1,
			t, CounterClockwise,
			nextPoint
			);

		if (CounterClockwise)
		{
			nextTangent[0] = -nextPoint[1];
			nextTangent[1] =  nextPoint[0];
		}
		else
		{
			nextTangent[0] =  nextPoint[1];
			nextTangent[1] = -nextPoint[0];
		}

		vgfMultiplyVector2ByMatrix3x2(
			nextPoint, &unitCircleToEllipse, nextPoint
			);

		nextPoint[0] += LastX;
		nextPoint[1] += LastY;

		vgfMultiplyVector2ByMatrix2x2(
			nextTangent, &unitCircleToEllipse, nextTangent
			);

		_NormalizeVector(
			 nextTangent[0],  nextTangent[1],
			&nextTangent[0], &nextTangent[1]
			);

		if ((nextTangent[0] == 0.0f) && (nextTangent[1] == 0.0f))
		{
			nextTangent[0] = currTangentX;
			nextTangent[1] = currTangentY;
		}

		status = _AddPoint(
			Info,
			currPointX, currPointY,
			currTangentX, currTangentY,
			gcvTRUE,
			gcvFALSE
			);

		if (status == gcvSTATUS_TRUE)
		{
			return gcvSTATUS_TRUE;
		}

		currPointX   = nextPoint[0];
		currPointY   = nextPoint[1];
		currTangentX = nextTangent[0];
		currTangentY = nextTangent[1];
	}

	status = _AddPoint(
		Info,
		ArcToXAbs, ArcToYAbs,
		outgoingTangent[0], outgoingTangent[1],
		gcvTRUE,
		gcvFALSE
		);

	return status;
}


/******************************************************************************\
********************************* Invalid Entry ********************************
\******************************************************************************/

vgmDEFINETESSELATE(Invalid)
{
	/* This function should never be called. */
	gcmASSERT(gcvFALSE);
	return gcvSTATUS_GENERIC_IO;
}


/******************************************************************************\
******************************** gcvVGCMD_CLOSE ********************************
\******************************************************************************/

vgmDEFINETESSELATE(gcvVGCMD_CLOSE)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Tesselate the segment. */
	status = _AddEndPath(
		Info,
		coords->lastX, coords->lastX,
		coords->startX, coords->startY
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Update the control coordinates. */
	coords->controlX = coords->startX;
	coords->controlY = coords->startY;

	/* Success. */
	return gcvSTATUS_OK;
}


/******************************************************************************\
******************************** gcvVGCMD_MOVE *********************************
\******************************************************************************/

vgmDEFINETESSELATE(gcvVGCMD_MOVE)
{
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT moveToX, moveToY;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Extract the command coordinates. */
	moveToX = Source->get(Source);
	moveToY = Source->get(Source);

	/* Tesselate the segment. */
	_AddPoint(
		Info, moveToX, moveToY, 0.0f, 0.0f, gcvFALSE, gcvFALSE
		);

	/* Update the control coordinates. */
	coords->startX   = moveToX;
	coords->startY   = moveToY;
	coords->controlX = moveToX;
	coords->controlY = moveToY;

	/* Success. */
	return gcvSTATUS_OK;
}

vgmDEFINETESSELATE(gcvVGCMD_MOVE_REL)
{
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT moveToX, moveToY;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Extract the command coordinates. */
	moveToX = Source->get(Source) + coords->lastX;
	moveToY = Source->get(Source) + coords->lastY;

	/* Tesselate the segment. */
	_AddPoint(
		Info, moveToX, moveToY, 0.0f, 0.0f, gcvFALSE, gcvFALSE
		);

	/* Update the control coordinates. */
	coords->startX   = moveToX;
	coords->startY   = moveToY;
	coords->controlX = moveToX;
	coords->controlY = moveToY;

	/* Success. */
	return gcvSTATUS_OK;
}


/******************************************************************************\
******************************** gcvVGCMD_LINE *********************************
\******************************************************************************/

vgmDEFINETESSELATE(gcvVGCMD_LINE)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT lineToX, lineToY;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Extract the command coordinates. */
	lineToX = Source->get(Source);
	lineToY = Source->get(Source);

	/* Tesselate the segment. */
	status = _AddLineTo(
		Info,
		coords->lastX, coords->lastY,
		lineToX, lineToY
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Update the control coordinates. */
	coords->controlX = lineToX;
	coords->controlY = lineToY;

	/* Success. */
	return gcvSTATUS_OK;
}

vgmDEFINETESSELATE(gcvVGCMD_LINE_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT lineToX, lineToY;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Extract the command coordinates. */
	lineToX = Source->get(Source) + coords->lastX;
	lineToY = Source->get(Source) + coords->lastY;

	/* Tesselate the end of the path. */
	status = _AddLineTo(
		Info,
		coords->lastX, coords->lastY,
		lineToX, lineToY
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Update the control coordinates. */
	coords->controlX = lineToX;
	coords->controlY = lineToY;

	/* Success. */
	return gcvSTATUS_OK;
}


/******************************************************************************\
********************************* gcvVGCMD_QUAD ********************************
\******************************************************************************/

vgmDEFINETESSELATE(gcvVGCMD_QUAD)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT controlX, controlY;
	gctFLOAT quadToX, quadToY;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Extract the command coordinates. */
	controlX = Source->get(Source);
	controlY = Source->get(Source);
	quadToX  = Source->get(Source);
	quadToY  = Source->get(Source);

	/* Tesselate the end of the path. */
	status = _AddQuadTo(
		Info,
		coords->lastX, coords->lastY,
		controlX, controlY,
		quadToX, quadToY
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Update the control coordinates. */
	coords->controlX = controlX;
	coords->controlY = controlY;

	/* Success. */
	return gcvSTATUS_OK;
}

vgmDEFINETESSELATE(gcvVGCMD_QUAD_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT controlX, controlY;
	gctFLOAT quadToX, quadToY;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Extract the command coordinates. */
	controlX = Source->get(Source) + coords->lastX;
	controlY = Source->get(Source) + coords->lastY;
	quadToX  = Source->get(Source) + coords->lastX;
	quadToY  = Source->get(Source) + coords->lastY;

	/* Tesselate the end of the path. */
	status = _AddQuadTo(
		Info,
		coords->lastX, coords->lastY,
		controlX, controlY,
		quadToX, quadToY
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Update the control coordinates. */
	coords->controlX = controlX;
	coords->controlY = controlY;

	/* Success. */
	return gcvSTATUS_OK;
}


/******************************************************************************\
********************************* gcvVGCMD_CUBIC *******************************
\******************************************************************************/

vgmDEFINETESSELATE(gcvVGCMD_CUBIC)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT control1X, control1Y;
	gctFLOAT control2X, control2Y;
	gctFLOAT cubicToX, cubicToY;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Extract the command coordinates. */
	control1X = Source->get(Source);
	control1Y = Source->get(Source);
	control2X = Source->get(Source);
	control2Y = Source->get(Source);
	cubicToX  = Source->get(Source);
	cubicToY  = Source->get(Source);

	/* Tesselate the end of the path. */
	status = _AddCubicTo(
		Info,
		coords->lastX, coords->lastY,
		control1X, control1Y,
		control2X, control2Y,
		cubicToX, cubicToY
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Update the control coordinates. */
	coords->controlX = control2X;
	coords->controlY = control2Y;

	/* Success. */
	return gcvSTATUS_OK;
}

vgmDEFINETESSELATE(gcvVGCMD_CUBIC_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	gctFLOAT control1X, control1Y;
	gctFLOAT control2X, control2Y;
	gctFLOAT cubicToX, cubicToY;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Extract the command coordinates. */
	control1X = Source->get(Source) + coords->lastX;
	control1Y = Source->get(Source) + coords->lastY;
	control2X = Source->get(Source) + coords->lastX;
	control2Y = Source->get(Source) + coords->lastY;
	cubicToX  = Source->get(Source) + coords->lastX;
	cubicToY  = Source->get(Source) + coords->lastY;

	/* Tesselate the end of the path. */
	status = _AddCubicTo(
		Info,
		coords->lastX, coords->lastY,
		control1X, control1Y,
		control2X, control2Y,
		cubicToX, cubicToY
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Update the control coordinates. */
	coords->controlX = control2X;
	coords->controlY = control2Y;

	/* Success. */
	return gcvSTATUS_OK;
}


/******************************************************************************\
********************************** gcvVGCMD_ARC ********************************
\******************************************************************************/

vgmDEFINETESSELATE(gcvVGCMD_ARC)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgsARCCOORDINATES_PTR arcCoords;
	gctFLOAT arcToXAbs, arcToYAbs;
	gctFLOAT arcToXRel, arcToYRel;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Get a pointer to the original ARC data. */
	arcCoords = (vgsARCCOORDINATES_PTR) Source->currPathData->extra;
	gcmASSERT(arcCoords != gcvNULL);

	/* Get the arc end point. */
	arcToXRel = arcCoords->endX - coords->lastX;
	arcToYRel = arcCoords->endY - coords->lastY;
	arcToXAbs = arcCoords->endX;
	arcToYAbs = arcCoords->endY;

	/* Tesselate the end of the path. */
	status = _AddArcTo(
		Info,
		coords->lastX, coords->lastY,
		arcCoords->horRadius,
		arcCoords->verRadius,
		arcCoords->rotAngle,
		arcToXRel, arcToYRel,
		arcToXAbs, arcToYAbs,
		arcCoords->counterClockwise,
		arcCoords->large
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Skip the ARC path in the source. */
	vgsPATHWALKER_SeekToEnd(Source);

	/* Update the control coordinates. */
	coords->controlX = arcToXAbs;
	coords->controlY = arcToYAbs;

	/* Success. */
	return gcvSTATUS_OK;
}

vgmDEFINETESSELATE(gcvVGCMD_ARC_REL)
{
	gceSTATUS status;
	vgsCONTROL_COORD_PTR coords;
	vgsARCCOORDINATES_PTR arcCoords;
	gctFLOAT arcToXRel, arcToYRel;
	gctFLOAT arcToXAbs, arcToYAbs;

	/* Get a shortcut to the control coordinates. */
	coords = Source->coords;

	/* Get a pointer to the original ARC data. */
	arcCoords = (vgsARCCOORDINATES_PTR) Source->currPathData->extra;
	gcmASSERT(arcCoords != gcvNULL);

	/* Get the arc end point. */
	arcToXRel = arcCoords->endX;
	arcToYRel = arcCoords->endY;
	arcToXAbs = arcCoords->endX + coords->lastX;
	arcToYAbs = arcCoords->endY + coords->lastY;

	/* Tesselate the end of the path. */
	status = _AddArcTo(
		Info,
		coords->lastX, coords->lastY,
		arcCoords->horRadius,
		arcCoords->verRadius,
		arcCoords->rotAngle,
		arcToXRel, arcToYRel,
		arcToXAbs, arcToYAbs,
		arcCoords->counterClockwise,
		arcCoords->large
		);

	if (status == gcvSTATUS_TRUE)
	{
		return gcvSTATUS_TRUE;
	}

	/* Skip the ARC path in the source. */
	vgsPATHWALKER_SeekToEnd(Source);

	/* Update the control coordinates. */
	coords->controlX = arcToXAbs;
	coords->controlY = arcToYAbs;

	/* Success. */
	return gcvSTATUS_OK;
}


/******************************************************************************\
******************************* Tesselation Array ******************************
\******************************************************************************/

static vgtTESSELATEHANDLER _tesselateCommand[] =
{
	vgmTESSEL(Invalid),                      /*   0: gcvVGCMD_END             */
	vgmTESSEL(gcvVGCMD_CLOSE),               /*   1: gcvVGCMD_CLOSE           */
	vgmTESSEL(gcvVGCMD_MOVE),                /*   2: gcvVGCMD_MOVE            */
	vgmTESSEL(gcvVGCMD_MOVE_REL),            /*   3: gcvVGCMD_MOVE_REL        */
	vgmTESSEL(gcvVGCMD_LINE),                /*   4: gcvVGCMD_LINE            */
	vgmTESSEL(gcvVGCMD_LINE_REL),            /*   5: gcvVGCMD_LINE_REL        */
	vgmTESSEL(gcvVGCMD_QUAD),                /*   6: gcvVGCMD_QUAD            */
	vgmTESSEL(gcvVGCMD_QUAD_REL),            /*   7: gcvVGCMD_QUAD_REL        */
	vgmTESSEL(gcvVGCMD_CUBIC),               /*   8: gcvVGCMD_CUBIC           */
	vgmTESSEL(gcvVGCMD_CUBIC_REL),           /*   9: gcvVGCMD_CUBIC_REL       */
	vgmTESSEL(Invalid),                      /*  10: gcvVGCMD_BREAK           */
	vgmTESSEL(Invalid),                      /*  11: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  12: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  13: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  14: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  15: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  16: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  17: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  18: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  19: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  20: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  21: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  22: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  23: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  24: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  25: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  26: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  27: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  28: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  29: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  30: **** R E S E R V E D *****/
	vgmTESSEL(Invalid),                      /*  31: **** R E S E R V E D *****/

	vgmTESSEL(Invalid),                      /*  32: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  33: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  34: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  35: ***** I N V A L I D ******/
	vgmTESSEL(gcvVGCMD_LINE),                /*  36: gcvVGCMD_HLINE_EMUL      */
	vgmTESSEL(gcvVGCMD_LINE_REL),            /*  37: gcvVGCMD_HLINE_EMUL_REL  */
	vgmTESSEL(Invalid),                      /*  38: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  39: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  40: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  41: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  42: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  43: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  44: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  45: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  46: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  47: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  48: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  49: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  50: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  51: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  52: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  53: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  54: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  55: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  56: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  57: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  58: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  59: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  60: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  61: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  62: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  63: ***** I N V A L I D ******/

	vgmTESSEL(Invalid),                      /*  64: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  65: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  66: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  67: ***** I N V A L I D ******/
	vgmTESSEL(gcvVGCMD_LINE),                /*  68: gcvVGCMD_VLINE_EMUL      */
	vgmTESSEL(gcvVGCMD_LINE_REL),            /*  69: gcvVGCMD_VLINE_EMUL_REL  */
	vgmTESSEL(Invalid),                      /*  70: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  71: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  72: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  73: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  74: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  75: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  76: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  77: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  78: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  79: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  80: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  81: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  82: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  83: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  84: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  85: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  86: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  87: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  88: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  89: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  90: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  91: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  92: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  93: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  94: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  95: ***** I N V A L I D ******/

	vgmTESSEL(Invalid),                      /*  96: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  97: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  98: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /*  99: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 100: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 101: ***** I N V A L I D ******/
	vgmTESSEL(gcvVGCMD_QUAD),                /* 102: gcvVGCMD_SQUAD_EMUL      */
	vgmTESSEL(gcvVGCMD_QUAD_REL),            /* 103: gcvVGCMD_SQUAD_EMUL_REL  */
	vgmTESSEL(gcvVGCMD_CUBIC),               /* 104: gcvVGCMD_SCUBIC_EMUL     */
	vgmTESSEL(gcvVGCMD_CUBIC_REL),           /* 105: gcvVGCMD_SCUBIC_EMUL_REL */
	vgmTESSEL(Invalid),                      /* 106: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 107: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 108: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 109: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 110: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 111: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 112: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 113: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 114: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 115: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 116: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 117: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 118: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 119: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 120: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 121: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 122: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 123: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 124: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 125: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 126: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 127: ***** I N V A L I D ******/

	vgmTESSEL(Invalid),                      /* 128: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 129: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 130: ***** I N V A L I D ******/
	vgmTESSEL(Invalid),                      /* 131: ***** I N V A L I D ******/
	vgmTESSEL(gcvVGCMD_ARC),                 /* 132: gcvVGCMD_ARC_LINE        */
	vgmTESSEL(gcvVGCMD_ARC_REL),             /* 133: gcvVGCMD_ARC_LINE_REL    */
	vgmTESSEL(gcvVGCMD_ARC),                 /* 134: gcvVGCMD_ARC_QUAD        */
	vgmTESSEL(gcvVGCMD_ARC_REL)              /* 135: gcvVGCMD_ARC_QUAD_REL    */
};


/*******************************************************************************
**
** vgfComputePointAlongPath
**
** Determines a set of path parameters:
**   - coordinates of a point along the path;
**   - distance to the point from the beginning of the path;
**   - tangent at the point;
**   - bounding box of the path.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Path
**       Pointer to the path object.
**
**    StartSegment
**    NumSegments
**       Subpath in question.
**
**    Distance
**       Distance from the beginning of the subpath to the point.
**
** OUTPUT:
**
**    X, Y
**       Pointers to the coordinates of the point.
**
**    TangentX, TangentY
**       Pointers to the tangent of the point.
**
**    Length
**       Pointer to the length of the path from the beginning to the point.
**
**    PathLeft, PathTop, PathRight, PathBottom
**       Pointers to the bounding box of the path.
*/

gctBOOL vgfComputePointAlongPath(
	IN vgsCONTEXT_PTR Context,
	IN vgsPATH_PTR Path,
	IN gctUINT StartSegment,
	IN gctUINT NumSegments,
	IN gctFLOAT Distance,
	OUT gctFLOAT_PTR X,
	OUT gctFLOAT_PTR Y,
	OUT gctFLOAT_PTR TangentX,
	OUT gctFLOAT_PTR TangentY,
	OUT gctFLOAT_PTR Length,
	OUT gctFLOAT_PTR PathLeft,
	OUT gctFLOAT_PTR PathTop,
	OUT gctFLOAT_PTR PathRight,
	OUT gctFLOAT_PTR PathBottom
	)
{
	gceSTATUS status;
	vgsTESSINFO info;
	vgsPATHWALKER source;
	gctUINT currentSegment;
	gctUINT lastSegment;

	/* Empty path? */
	if (Path->numCoords == 0)
	{
		return gcvFALSE;
	}

	/* Reset the tesselation parameters. */
	info.enableUpdate = gcvFALSE;
	info.havePoints   = gcvFALSE;

	info.distance = (Distance >= 0.0f)
		? Distance
		: 0.0f;

	info.positionX = 0.0f;
	info.positionY = 0.0f;

	info.tangentX = 1.0f;
	info.tangentY = 0.0f;

	info.length = 0.0f;

	info.left   = gcvMAX_POS_FLOAT;
	info.top    = gcvMAX_POS_FLOAT;
	info.right  = gcvMAX_NEG_FLOAT;
	info.bottom = gcvMAX_NEG_FLOAT;

	/* Reset control coordinates. */
	info.coords.startX   = 0.0f;
	info.coords.startY   = 0.0f;
	info.coords.lastX    = 0.0f;
	info.coords.lastY    = 0.0f;
	info.coords.controlX = 0.0f;
	info.coords.controlY = 0.0f;

	/* Initialize walkers. */
	vgsPATHWALKER_InitializeReader(
		Context, Context->pathStorage, &source, &info.coords, Path
		);

	/* Reset the initial segment. */
	currentSegment = 0;

	/* Determine the last segment. */
	lastSegment = StartSegment + NumSegments;

	/* Walk the path. */
	while (gcvTRUE)
	{
		/* Get the current command. */
		gceVGCMD command = source.command;

		/* Is it time to enable tesselation? */
		if (!info.enableUpdate && (currentSegment == StartSegment))
		{
			info.enableUpdate = gcvTRUE;
		}

		/* Call the handler. */
		gcmASSERT(gcmIS_VALID_INDEX(command, _tesselateCommand));
		status = _tesselateCommand[command] (&source, &info);

		/* Finished? */
		if (status == gcvSTATUS_TRUE)
		{
			break;
		}

		/* Advance to the next segment. */
		currentSegment += 1;

		/* Finished? */
		if (currentSegment == lastSegment)
		{
			break;
		}

		/* Get the next segment. */
		gcmVERIFY_OK(vgsPATHWALKER_NextSegment(&source));
	}

	/* Is there any geometry in the path? */
	if (!info.havePoints)
	{
		return gcvFALSE;
	}

	/* Set the results. */
	if (X != gcvNULL) * X = info.positionX;
	if (Y != gcvNULL) * Y = info.positionY;

	if (TangentX != gcvNULL) * TangentX = info.tangentX;
	if (TangentY != gcvNULL) * TangentY = info.tangentY;

	if (Length != gcvNULL) * Length = info.length;

	if (PathLeft   != gcvNULL) * PathLeft   = info.left;
	if (PathTop    != gcvNULL) * PathTop    = info.top;
	if (PathRight  != gcvNULL) * PathRight  = info.right;
	if (PathBottom != gcvNULL) * PathBottom = info.bottom;

	/* Success. */
	return gcvTRUE;
}
