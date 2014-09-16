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




#ifndef __gc_vg_path_walker_h__
#define __gc_vg_path_walker_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*------------------------ Miscellaneous definitions. ------------------------*/

/* Command size calculation shortcuts. */
#define vgmCOMMANDSIZE(CoordinateCount, CoordinateType) \
	((1 + (CoordinateCount)) * gcmSIZEOF(CoordinateType))

#define vgmDATASIZE(CoordinateCount, CoordinateType) \
	((CoordinateCount) * gcmSIZEOF(CoordinateType))

/*----------------------------------------------------------------------------*/
/*---------------- Segment coordinates set/get function types. ---------------*/

typedef void (* vgtCOPYCOORDINATE) (
	vgsPATHWALKER_PTR Source,
	vgsPATHWALKER_PTR Destination
	);

typedef void (* vgtZEROCOORDINATE) (
	vgsPATHWALKER_PTR Destination
	);

/*----------------------------------------------------------------------------*/
/*--------------------------- Path walker structure. -------------------------*/

typedef struct _vgsPATHWALKER
{
#if vgvVALIDATE_PATH_WALKER
	/* Operation flag. */
	gctBOOL					reader;
#endif

	/* Object pointers. */
	vgsCONTEXT_PTR			context;
	vgsPATHSTORAGE_PTR		storage;
	vgsMEMORYMANAGER_PTR	arcCoordinates;
	vgsPATH_PTR				path;
	gcoVG					vg;

	/* The number of bytes reserved for the head. */
	gctUINT					reservedForHead;

	/* Path scale and bias values. */
	gctFLOAT				bias;
	gctFLOAT				scale;

	/* Buffer chain allocated during the current session. */
	vgsPATH_DATA_PTR		head;
	vgsPATH_DATA_PTR		tail;

	/* The current and the last data buffers. */
	vgsPATH_DATA_PTR		currPathData;
	gcsCMDBUFFER_PTR		currBuffer;
	gctUINT					currEndOffset;
	gctUINT8_PTR			currData;

	/* The size of the available space left in the current buffer for writing. */
	gctINT					available;

	/* Last buffer marker for reading. */
	gcsCMDBUFFER_PTR		lastBuffer;
	gctUINT					lastEndOffset;

	/* Coordinate access functions. */
	vgtGETCOORDINATE		get;
	vgtSETCOORDINATE		set;
	vgtZEROCOORDINATE		zero;
	vgtGETCOPYCOORDINATE	getcopy;
	vgtCOPYCOORDINATE		copy;
	vgtGETCOORDINATE *		getArray;
	vgtSETCOORDINATE *		setArray;

	/* Control coordinates of the path. */
	vgsCONTROL_COORD_PTR	coords;

	/* The size of the data type and its alignment. */
	gctBOOL					forcedDataType;
	gcePATHTYPE				dataType;
	gctUINT					dataTypeSize;
	gctUINT					dataAlignment;
	gctUINT					dataMask;

	/* Command size array. */
	gctUINT_PTR				commandSizeArray;

	/* When reading: skipped segment and coordinate counts;
	   When writing: the number of path segments and coordinates written into
	                 the buffer during the current session. */
	gctUINT					numSegments;
	gctUINT					numCoords;

	/* Current command info. */
	gctBOOL					relative;
	gceVGCMD				command;
	gctUINT					segmentOffset;
	gctUINT					segmentSize;
	gctUINT					coordinateCount;
	gctUINT8_PTR			coordinate;
}
vgsPATHWALKER;

/*----------------------------------------------------------------------------*/
/*------------------------------ Path walker API. ----------------------------*/

void vgsPATHWALKER_InitializeImport(
	vgsCONTEXT_PTR Context,
	vgsPATHSTORAGE_PTR Storage,
	vgsPATHWALKER_PTR Source,
	vgsPATH_PTR DestinationPath,
	gctCONST_POINTER Data
	);

void vgsPATHWALKER_InitializeReader(
	vgsCONTEXT_PTR Context,
	vgsPATHSTORAGE_PTR Storage,
	vgsPATHWALKER_PTR Source,
	vgsCONTROL_COORD_PTR ControlCoordinates,
	vgsPATH_PTR Path
	);

void vgsPATHWALKER_InitializeWriter(
	vgsCONTEXT_PTR Context,
	vgsPATHSTORAGE_PTR Storage,
	vgsPATHWALKER_PTR Destination,
	vgsPATH_PTR Path
	);

gceSTATUS vgsPATHWALKER_NextSegment(
	vgsPATHWALKER_PTR Source
	);

gceSTATUS vgsPATHWALKER_NextBuffer(
	vgsPATHWALKER_PTR Source
	);

gctBOOL vgsPATHWALKER_SeekToSegment(
	vgsPATHWALKER_PTR Source,
	gctUINT Segment
	);

void vgsPATHWALKER_SeekToEnd(
	vgsPATHWALKER_PTR Destination
	);

gceSTATUS vgsPATHWALKER_StartSubpath(
	vgsPATHWALKER_PTR Destination,
	gctUINT MinimumSize,
	gcePATHTYPE DataType
	);

vgsPATH_DATA_PTR vgsPATHWALKER_CloseSubpath(
	vgsPATHWALKER_PTR Destination
	);

gceSTATUS vgsPATHWALKER_DoneWriting(
	vgsPATHWALKER_PTR Destination
	);

gceSTATUS vgsPATHWALKER_WriteCommand(
	vgsPATHWALKER_PTR Destination,
	gceVGCMD Command
	);

gceSTATUS vgsPATHWALKER_GetCopyBlockSize(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source,
	gctUINT * Size,
	gctUINT * SegmentCount,
	gctUINT * CoordinateCount
	);

gceSTATUS vgsPATHWALKER_CopyBlock(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source,
	gctUINT Size,
	gctUINT SegmentCount,
	gctUINT CoordinateCount
	);

void vgsPATHWALKER_AppendData(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source,
	gctUINT SegmentCount,
	gctUINT CoordinateCount
	);

void vgsPATHWALKER_ReplaceData(
	vgsPATHWALKER_PTR Destination,
	vgsPATHWALKER_PTR Source
	);

void vgsPATHWALKER_Rollback(
	vgsPATHWALKER_PTR Destination
	);

#ifdef __cplusplus
}
#endif

#endif
