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

/*******************************************************************************
**
** _GetCoordinate
**
** Retrieve the specified transformed segment coordinate.
**
** INPUT:
**
**    Walker
**       Pointer to the source walker.
**
** OUTPUT:
**
**    Returns the specified transformed coordinate.
*/

static VGfloat _GetCoordinate_S_8(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT8_PTR) Walker->coordinate);

	/* Scale and bias the source coordinate. */
	coordinate *= Walker->scale;
	coordinate += Walker->bias;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT8);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOBIAS_S_8(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT8_PTR) Walker->coordinate);

	/* Scale the source coordinate. */
	coordinate *= Walker->scale;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT8);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOSCALE_S_8(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT8_PTR) Walker->coordinate);

	/* Bias the source coordinate. */
	coordinate += Walker->bias;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT8);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOBIAS_NOSCALE_S_8(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT8_PTR) Walker->coordinate);

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT8);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_S_16(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT16_PTR) Walker->coordinate);

	/* Scale and bias the source coordinate. */
	coordinate *= Walker->scale;
	coordinate += Walker->bias;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT16);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOBIAS_S_16(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT16_PTR) Walker->coordinate);

	/* Scale the source coordinate. */
	coordinate *= Walker->scale;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT16);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOSCALE_S_16(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT16_PTR) Walker->coordinate);

	/* Bias the source coordinate. */
	coordinate += Walker->bias;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT16);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOBIAS_NOSCALE_S_16(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT16_PTR) Walker->coordinate);

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT16);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_S_32(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT32_PTR) Walker->coordinate);

	/* Scale and bias the source coordinate. */
	coordinate *= Walker->scale;
	coordinate += Walker->bias;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT32);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOBIAS_S_32(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT32_PTR) Walker->coordinate);

	/* Scale the source coordinate. */
	coordinate *= Walker->scale;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT32);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOSCALE_S_32(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT32_PTR) Walker->coordinate);

	/* Bias the source coordinate. */
	coordinate += Walker->bias;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT32);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOBIAS_NOSCALE_S_32(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctINT32_PTR) Walker->coordinate);

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctINT32);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_F(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctFLOAT_PTR) Walker->coordinate);

	/* Scale and bias the source coordinate. */
	coordinate *= Walker->scale;
	coordinate += Walker->bias;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctFLOAT);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOBIAS_F(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctFLOAT_PTR) Walker->coordinate);

	/* Scale the source coordinate. */
	coordinate *= Walker->scale;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctFLOAT);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOSCALE_F(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctFLOAT_PTR) Walker->coordinate);

	/* Bias the source coordinate. */
	coordinate += Walker->bias;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctFLOAT);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCoordinate_NOBIAS_NOSCALE_F(
	vgsPATHWALKER_PTR Walker
	)
{
	/* Get the source coordinate. */
	VGfloat coordinate = (VGfloat) * ((gctFLOAT_PTR) Walker->coordinate);

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(gctFLOAT);

	/* Return transformed coordinate. */
	return coordinate;
}


/*******************************************************************************
**
** _SetCoordinate
**
** Store the specified transformed segment coordinate.
**
** INPUT:
**
**    Walker
**       Pointer to the destination walker.
**
**    Coordinate
**       Coordinate value to transform and store.
**
** OUTPUT:
**
**    Nothing.
*/

static void _SetCoordinate_S_8(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT8 coordinate;

	/* Rescale and debias for destination path. */
	Coordinate -= Walker->bias;
	Coordinate /= Walker->scale;

	/* Convert the coordinate. */
	coordinate = (gctINT8) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT8_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOBIAS_S_8(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT8 coordinate;

	/* Rescale for destination path. */
	Coordinate /= Walker->scale;

	/* Convert the coordinate. */
	coordinate = (gctINT8) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT8_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOSCALE_S_8(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT8 coordinate;

	/* Debias for destination path. */
	Coordinate -= Walker->bias;

	/* Convert the coordinate. */
	coordinate = (gctINT8) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT8_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOBIAS_NOSCALE_S_8(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	/* Convert the coordinate. */
	gctINT8 coordinate = (gctINT8) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT8_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_S_16(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT16 coordinate;

	/* Rescale and debias for destination path. */
	Coordinate -= Walker->bias;
	Coordinate /= Walker->scale;

	/* Convert the coordinate. */
	coordinate = (gctINT16) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT16_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOBIAS_S_16(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT16 coordinate;

	/* Rescale for destination path. */
	Coordinate /= Walker->scale;

	/* Convert the coordinate. */
	coordinate = (gctINT16) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT16_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOSCALE_S_16(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT16 coordinate;

	/* Debias for destination path. */
	Coordinate -= Walker->bias;

	/* Convert the coordinate. */
	coordinate = (gctINT16) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT16_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOBIAS_NOSCALE_S_16(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	/* Convert the coordinate. */
	gctINT16 coordinate = (gctINT16) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT16_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_S_32(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT32 coordinate;

	/* Rescale and debias for destination path. */
	Coordinate -= Walker->bias;
	Coordinate /= Walker->scale;

	/* Convert the coordinate. */
	coordinate = (gctINT32) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT32_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOBIAS_S_32(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT32 coordinate;

	/* Rescale for destination path. */
	Coordinate /= Walker->scale;

	/* Convert the coordinate. */
	coordinate = (gctINT32) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT32_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOSCALE_S_32(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	gctINT32 coordinate;

	/* Debias for destination path. */
	Coordinate -= Walker->bias;

	/* Convert the coordinate. */
	coordinate = (gctINT32) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT32_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_NOBIAS_NOSCALE_S_32(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	/* Convert the coordinate. */
	gctINT32 coordinate = (gctINT32) floor(Coordinate + 0.5f);

	/* Set the destination coordinate. */
	* ((gctINT32_PTR) Walker->coordinate) = coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(coordinate);
}

static void _SetCoordinate_F(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	/* Rescale and debias for destination path. */
	Coordinate -= Walker->bias;
	Coordinate /= Walker->scale;

	/* Set the destination coordinate. */
	* ((gctFLOAT_PTR) Walker->coordinate) = Coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(Coordinate);
}

static void _SetCoordinate_NOBIAS_F(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	/* Rescale for destination path. */
	Coordinate /= Walker->scale;

	/* Set the destination coordinate. */
	* ((gctFLOAT_PTR) Walker->coordinate) = Coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(Coordinate);
}

static void _SetCoordinate_NOSCALE_F(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	/* Debias for destination path. */
	Coordinate -= Walker->bias;

	/* Set the destination coordinate. */
	* ((gctFLOAT_PTR) Walker->coordinate) = Coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(Coordinate);
}

static void _SetCoordinate_NOBIAS_NOSCALE_F(
	vgsPATHWALKER_PTR Walker,
	VGfloat Coordinate
	)
{
	/* Set the destination coordinate. */
	* ((gctFLOAT_PTR) Walker->coordinate) = Coordinate;

	/* Advance to the next coordinate. */
	Walker->coordinate += sizeof(Coordinate);
}


/*******************************************************************************
**
** _GetCopyCoordinate
**
** Copy the current coordinate from the source to the destination buffer and
** return transformed value of the coordinate.
**
** ASSUMPTION: the source and destination are of the same format
**             and have the same bias and scale values.
**
** INPUT:
**
**    Source
**       Pointer to the source walker.
**
**    Destination
**       Pointer to the destination walker.
**
** OUTPUT:
**
**    Nothing.
*/

static VGfloat _GetCopyCoordinate_S_8(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT8 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT8_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT8_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = Destination->scale * coordinate + Destination->bias;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOBIAS_S_8(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT8 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT8_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT8_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = Destination->scale * coordinate;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOSCALE_S_8(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT8 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT8_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT8_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = coordinate + Destination->bias;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOBIAS_NOSCALE_S_8(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	gctINT8 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT8_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT8_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCopyCoordinate_S_16(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT16 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT16_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT16_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = Destination->scale * coordinate + Destination->bias;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOBIAS_S_16(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT16 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT16_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT16_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = Destination->scale * coordinate;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOSCALE_S_16(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT16 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT16_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT16_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = coordinate + Destination->bias;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOBIAS_NOSCALE_S_16(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	gctINT16 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT16_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT16_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Return transformed coordinate. */
	return coordinate;
}

static VGfloat _GetCopyCoordinate_S_32(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT32 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT32_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT32_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = Destination->scale * coordinate + Destination->bias;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOBIAS_S_32(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT32 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT32_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT32_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = Destination->scale * coordinate;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOSCALE_S_32(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctINT32 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT32_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT32_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = coordinate + Destination->bias;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOBIAS_NOSCALE_S_32(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	gctINT32 coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctINT32_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctINT32_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Return transformed coordinate. */
	return (VGfloat) coordinate;
}

static VGfloat _GetCopyCoordinate_F(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctFLOAT coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctFLOAT_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctFLOAT_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = Destination->scale * coordinate + Destination->bias;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOBIAS_F(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctFLOAT coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctFLOAT_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctFLOAT_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = Destination->scale * coordinate;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOSCALE_F(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	VGfloat result;
	gctFLOAT coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctFLOAT_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctFLOAT_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Compute the result. */
	result = coordinate + Destination->bias;

	/* Return transformed coordinate. */
	return result;
}

static VGfloat _GetCopyCoordinate_NOBIAS_NOSCALE_F(
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	)
{
	gctFLOAT coordinate;

	/* Get the source coordinate. */
	coordinate = * (gctFLOAT_PTR) Source->coordinate;
	Source->coordinate += sizeof(coordinate);

	/* Store the coordinate in the destination. */
	* (gctFLOAT_PTR) Destination->coordinate = coordinate;
	Destination->coordinate += sizeof(coordinate);

	/* Return transformed coordinate. */
	return coordinate;
}


/*******************************************************************************
**
** vgfGetCoordinateAccessArrays
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
**    Pointer to the function array.
*/

void vgfGetCoordinateAccessArrays(
	gctFLOAT Scale,
	gctFLOAT Bias,
	vgtGETCOORDINATE ** GetArray,
	vgtSETCOORDINATE ** SetArray,
	vgtGETCOPYCOORDINATE ** GetCopyArray
	)
{
	/***************************************************************************
	** Getters.
	*/

	static vgtGETCOORDINATE _getCoordinate[] =
	{
		_GetCoordinate_S_8,						/* VG_PATH_DATATYPE_S_8  */
		_GetCoordinate_S_16,					/* VG_PATH_DATATYPE_S_16 */
		_GetCoordinate_S_32,					/* VG_PATH_DATATYPE_S_32 */
		_GetCoordinate_F						/* VG_PATH_DATATYPE_F    */
	};

	static vgtGETCOORDINATE _getCoordinateNobias[] =
	{
		_GetCoordinate_NOBIAS_S_8,				/* VG_PATH_DATATYPE_S_8  */
		_GetCoordinate_NOBIAS_S_16,				/* VG_PATH_DATATYPE_S_16 */
		_GetCoordinate_NOBIAS_S_32,				/* VG_PATH_DATATYPE_S_32 */
		_GetCoordinate_NOBIAS_F					/* VG_PATH_DATATYPE_F    */
	};

	static vgtGETCOORDINATE _getCoordinateNoscale[] =
	{
		_GetCoordinate_NOSCALE_S_8,				/* VG_PATH_DATATYPE_S_8  */
		_GetCoordinate_NOSCALE_S_16,			/* VG_PATH_DATATYPE_S_16 */
		_GetCoordinate_NOSCALE_S_32,			/* VG_PATH_DATATYPE_S_32 */
		_GetCoordinate_NOSCALE_F				/* VG_PATH_DATATYPE_F    */
	};

	static vgtGETCOORDINATE _getCoordinateNobiasNoscale[] =
	{
		_GetCoordinate_NOBIAS_NOSCALE_S_8,		/* VG_PATH_DATATYPE_S_8  */
		_GetCoordinate_NOBIAS_NOSCALE_S_16,		/* VG_PATH_DATATYPE_S_16 */
		_GetCoordinate_NOBIAS_NOSCALE_S_32,		/* VG_PATH_DATATYPE_S_32 */
		_GetCoordinate_NOBIAS_NOSCALE_F			/* VG_PATH_DATATYPE_F    */
	};

	/***************************************************************************
	** Setters.
	*/

	static vgtSETCOORDINATE _setCoordinate[] =
	{
		_SetCoordinate_S_8,						/* VG_PATH_DATATYPE_S_8  */
		_SetCoordinate_S_16,					/* VG_PATH_DATATYPE_S_16 */
		_SetCoordinate_S_32,					/* VG_PATH_DATATYPE_S_32 */
		_SetCoordinate_F						/* VG_PATH_DATATYPE_F    */
	};

	static vgtSETCOORDINATE _setCoordinateNobias[] =
	{
		_SetCoordinate_NOBIAS_S_8,				/* VG_PATH_DATATYPE_S_8  */
		_SetCoordinate_NOBIAS_S_16,				/* VG_PATH_DATATYPE_S_16 */
		_SetCoordinate_NOBIAS_S_32,				/* VG_PATH_DATATYPE_S_32 */
		_SetCoordinate_NOBIAS_F					/* VG_PATH_DATATYPE_F    */
	};

	static vgtSETCOORDINATE _setCoordinateNoscale[] =
	{
		_SetCoordinate_NOSCALE_S_8,				/* VG_PATH_DATATYPE_S_8  */
		_SetCoordinate_NOSCALE_S_16,			/* VG_PATH_DATATYPE_S_16 */
		_SetCoordinate_NOSCALE_S_32,			/* VG_PATH_DATATYPE_S_32 */
		_SetCoordinate_NOSCALE_F				/* VG_PATH_DATATYPE_F    */
	};

	static vgtSETCOORDINATE _setCoordinateNobiasNoscale[] =
	{
		_SetCoordinate_NOBIAS_NOSCALE_S_8,		/* VG_PATH_DATATYPE_S_8  */
		_SetCoordinate_NOBIAS_NOSCALE_S_16,		/* VG_PATH_DATATYPE_S_16 */
		_SetCoordinate_NOBIAS_NOSCALE_S_32,		/* VG_PATH_DATATYPE_S_32 */
		_SetCoordinate_NOBIAS_NOSCALE_F			/* VG_PATH_DATATYPE_F    */
	};

	/***************************************************************************
	** Getters/copiers.
	*/

	static vgtGETCOPYCOORDINATE _getcopyCoordinate[] =
	{
		_GetCopyCoordinate_S_8,					/* VG_PATH_DATATYPE_S_8  */
		_GetCopyCoordinate_S_16,				/* VG_PATH_DATATYPE_S_16 */
		_GetCopyCoordinate_S_32,				/* VG_PATH_DATATYPE_S_32 */
		_GetCopyCoordinate_F					/* VG_PATH_DATATYPE_F    */
	};

	static vgtGETCOPYCOORDINATE _getcopyCoordinateNobias[] =
	{
		_GetCopyCoordinate_NOBIAS_S_8,			/* VG_PATH_DATATYPE_S_8  */
		_GetCopyCoordinate_NOBIAS_S_16,			/* VG_PATH_DATATYPE_S_16 */
		_GetCopyCoordinate_NOBIAS_S_32,			/* VG_PATH_DATATYPE_S_32 */
		_GetCopyCoordinate_NOBIAS_F				/* VG_PATH_DATATYPE_F    */
	};

	static vgtGETCOPYCOORDINATE _getcopyCoordinateNoscale[] =
	{
		_GetCopyCoordinate_NOSCALE_S_8,			/* VG_PATH_DATATYPE_S_8  */
		_GetCopyCoordinate_NOSCALE_S_16,		/* VG_PATH_DATATYPE_S_16 */
		_GetCopyCoordinate_NOSCALE_S_32,		/* VG_PATH_DATATYPE_S_32 */
		_GetCopyCoordinate_NOSCALE_F			/* VG_PATH_DATATYPE_F    */
	};

	static vgtGETCOPYCOORDINATE _getcopyCoordinateNobiasNoscale[] =
	{
		_GetCopyCoordinate_NOBIAS_NOSCALE_S_8,	/* VG_PATH_DATATYPE_S_8  */
		_GetCopyCoordinate_NOBIAS_NOSCALE_S_16,	/* VG_PATH_DATATYPE_S_16 */
		_GetCopyCoordinate_NOBIAS_NOSCALE_S_32,	/* VG_PATH_DATATYPE_S_32 */
		_GetCopyCoordinate_NOBIAS_NOSCALE_F		/* VG_PATH_DATATYPE_F    */
	};

	/***************************************************************************
	** Determine the right set.
	*/

	if (Bias == 0.0f)
	{
		if (Scale == 1.0f)
		{
			* GetArray     = _getCoordinateNobiasNoscale;
			* SetArray     = _setCoordinateNobiasNoscale;
			* GetCopyArray = _getcopyCoordinateNobiasNoscale;
		}
		else
		{
			* GetArray     = _getCoordinateNobias;
			* SetArray     = _setCoordinateNobias;
			* GetCopyArray = _getcopyCoordinateNobias;
		}
	}
	else if (Scale == 1.0f)
	{
		* GetArray     = _getCoordinateNoscale;
		* SetArray     = _setCoordinateNoscale;
		* GetCopyArray = _getcopyCoordinateNoscale;
	}
	else
	{
		* GetArray     = _getCoordinate;
		* SetArray     = _setCoordinate;
		* GetCopyArray = _getcopyCoordinate;
	}
}
