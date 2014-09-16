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




#ifndef __gc_vg_path_storage_h__
#define __gc_vg_path_storage_h__

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------- Access macros. -------------------------------*/

#define vgmGETCMDBUFFER(PathData) \
	((gcsCMDBUFFER_PTR) PathData)

/* Previous / next allocated path data access. */
#define vgmGETPREVALLOCATED(PathData) \
	(PathData->prevAllocated)

#define vgmGETNEXTALLOCATED(PathData) \
	((vgsPATH_DATA_PTR) (((gcsCMDBUFFER_PTR) PathData)->nextAllocated))

#define vgmSETPREVALLOCATED(PathData, PrevPathData) \
	(PathData->prevAllocated = PrevPathData)

#define vgmSETNEXTALLOCATED(PathData, NextPathData) \
	(((gcsCMDBUFFER_PTR) PathData)->nextAllocated = \
		(gcsCMDBUFFER_PTR) NextPathData)

/* Previous / next subbuffer access. */
#define vgmGETPREVSUBBUFFER(PathData) \
	(PathData->prevSubBuffer)

#define vgmGETNEXTSUBBUFFER(PathData) \
	((vgsPATH_DATA_PTR) vgmGETCMDBUFFER(PathData)->nextSubBuffer)

#define vgmSETPREVSUBBUFFER(PathData, PrevPathData) \
	(PathData->prevSubBuffer = PrevPathData)

#define vgmSETNEXTSUBBUFFER(PathData, NextPathData) \
	(((gcsCMDBUFFER_PTR) PathData)->nextSubBuffer = \
		(gcsCMDBUFFER_PTR) NextPathData)

/*----------------------------------------------------------------------------*/
/*------------------- Path container structure definition. -------------------*/

/* vgsPATHSTORAGE object owns a linked list of allocated buffers, each buffer
   ends with a vgsCONTAINER structure that links all buffers together. */
typedef struct _vgsCONTAINER
{
	gcuVIDMEM_NODE_PTR		node;
	vgsCONTAINER_PTR		next;

	/* Container counter. */
	vgmDEFINE_CONTAINER_COUNTER()
}
vgsCONTAINER;

/*----------------------------------------------------------------------------*/
/*------------- Path segment buffer header structure definition. -------------*/

/* Path command buffer header. */
typedef struct _vgsPATH_DATA * vgsPATH_DATA_PTR;
typedef struct _vgsPATH_DATA
{
	/* Path data. */
	gcsPATH_DATA			data;

	/* The number of path segments and coordinates stored in the buffer. */
	gctUINT					numSegments;
	gctUINT					numCoords;

	/* Point to the next busy command buffer. */
	vgsPATH_DATA_PTR		nextBusy;

	/* Most Recently Used list. */
	vgsPATH_DATA_PTR		prevMRU;
	vgsPATH_DATA_PTR		nextMRU;

	/* Point to the next and previous free command buffers. */
	vgsPATH_DATA_PTR		prevFree;
	vgsPATH_DATA_PTR		nextFree;

	/* Points to the immediate previous allocated command buffer. */
	vgsPATH_DATA_PTR		prevAllocated;

	/* Points to the previous subbuffer if any. */
	vgsPATH_DATA_PTR		prevSubBuffer;

	/* Extra information attached to the buffer. */
	vgsMEMORYMANAGER_PTR	extraManager;
	gctPOINTER				extra;

	/* Owner path. */
	vgsPATH_PTR				path;
}
vgsPATH_DATA;

/*----------------------------------------------------------------------------*/
/*------------------ Object management function declarations. ----------------*/

gceSTATUS vgsPATHSTORAGE_Construct(
	IN vgsCONTEXT_PTR Context,
	IN gctSIZE_T Granularity,
	IN gctSIZE_T AllocationCap,
	OUT vgsPATHSTORAGE_PTR * Storage
	);

gceSTATUS vgsPATHSTORAGE_Destroy(
	IN vgsPATHSTORAGE_PTR Storage
	);

gceSTATUS vgsPATHSTORAGE_Open(
	IN vgsPATHSTORAGE_PTR Storage,
	IN gctUINT Size,
	OUT vgsPATH_DATA_PTR * PathData
	);

void vgsPATHSTORAGE_Close(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	);

gctBOOL vgsPATHSTORAGE_Free(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData,
	IN gctBOOL FreeSubBuffers
	);

void vgsPATHSTORAGE_UpdateMRU(
	IN vgsPATHSTORAGE_PTR Storage,
	IN vgsPATH_DATA_PTR PathData
	);

#ifdef __cplusplus
}
#endif

#endif
