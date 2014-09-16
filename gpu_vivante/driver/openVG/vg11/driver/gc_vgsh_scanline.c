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




#if defined(USE_SCAN_LINE) && USE_SCAN_LINE

/*
**	Scan line tessellation module.
**	To be included by vgScanLineTess.c in OpenVG implementations.
*/

#define USE_SHIFT_FOR_SCALE		1
#define USE_INT64_FOR_BEZIER	1
#define USE_QUALITY_SCALE		0

#define USE_ONE_TEST_LOG		1
#define TEST_LOG_FILENAME		"path.log"
#define TEST_CMD_FILENAME		"path_cmd.log"
#define TEST_COORD_FILENAME		"path_coord.log"

/******************************************************************************\
|************************* Memory management Functions ************************|
|******************************************************************************|
|* There are only two types of memory pools here:                             *|
|*   - Fixed size node for linked list                                        *|
|*   - Fixed size node array                                                  *|
|* Names are not used in optimizer, so there is no need for variable size     *|
|* memory pool.                                                               *|
|* The implementation is specialized for optimizer to improve performance.    *|
\******************************************************************************/

/*
    _USE_LOCAL_MEMORY_POOL_

    This define enables the local memory management to improve performance.
*/
#define _USE_LOCAL_MEMORY_POOL_		1

#ifdef __GNUC__
#	define MEM_NODE_DATA_OFFSET			(gctUINT) (gctSIZE_T) &(((vgSL_MEM_NODE)0)->data)
#	define MEM_ARRAY_NODE_DATA_OFFSET	(gctUINT) (gctSIZE_T) &(((vgSL_MEM_ARRAY_NODE)0)->data)
#elif definded(UNDER_CE)
#	define MEM_NODE_DATA_OFFSET			(gctUINT) (gctSIZE_T) &(((vgSL_MEM_NODE)0)->data)
#	define MEM_ARRAY_NODE_DATA_OFFSET	(gctUINT) (gctSIZE_T) &(((vgSL_MEM_ARRAY_NODE)0)->data)
#else
#	define MEM_NODE_DATA_OFFSET			(gctUINT) (gctUINT64) &(((vgSL_MEM_NODE)0)->data)
#	define MEM_ARRAY_NODE_DATA_OFFSET	(gctUINT) (gctUINT64) &(((vgSL_MEM_ARRAY_NODE)0)->data)
#endif

static void
_InitMemPool(
	IN vgSL_MEM_POOL *	MemPool,
	IN gcoOS OS,
	IN gctUINT NodeCount,
	IN gctUINT NodeSize
	)
{
	MemPool->os			= OS;
	MemPool->blockList	= gcvNULL;
	MemPool->freeList	= gcvNULL;
	MemPool->nodeCount	= NodeCount;
	MemPool->nodeSize	= NodeSize;
}

static void
_FreeMemPool(
	IN vgSL_MEM_POOL *	MemPool
	)
{
	while (MemPool->blockList != gcvNULL)
	{
		vgSL_MEM_NODE block = MemPool->blockList;

		MemPool->blockList = block->next;
		gcmVERIFY_OK(gcoOS_Free(MemPool->os, block));
	}

	MemPool->freeList = gcvNULL;
}

static gceSTATUS
_MemPoolGetANode(
	IN vgSL_MEM_POOL *	MemPool,
	OUT gctPOINTER *	Node
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

	if (MemPool->freeList == gcvNULL)
	{
		vgSL_MEM_NODE block, node;
		gctSTRING data;
		gctUINT i;

		/* Allocation next block and put them in free list */
		gcmERR_RETURN(gcoOS_Allocate(MemPool->os,
						MemPool->nodeCount * MemPool->nodeSize + MEM_NODE_DATA_OFFSET,
						(gctPOINTER *) &block));

		block->next = MemPool->blockList;
		MemPool->blockList = block;

		/* Create linked list */
		node = MemPool->freeList = (vgSL_MEM_NODE) ((gctSTRING) block + MEM_NODE_DATA_OFFSET);
		data = (gctSTRING) node + MemPool->nodeSize;
		for (i = 1; i < MemPool->nodeCount; i++, data += MemPool->nodeSize)
		{
			node->next = (vgSL_MEM_NODE) data;
			node = (vgSL_MEM_NODE) data;
		}
		node->next = gcvNULL;
	}

	*Node = (gctPOINTER) MemPool->freeList;
	MemPool->freeList = MemPool->freeList->next;

	gcmVERIFY_OK(gcoOS_ZeroMemory(*Node, MemPool->nodeSize));

	return gcvSTATUS_OK;
}

static gceSTATUS
_MemPoolFreeANode(
	IN vgSL_MEM_POOL *	MemPool,
	IN gctPOINTER		Node
	)
{
	vgSL_MEM_NODE		node = (vgSL_MEM_NODE) Node;

	node->next = MemPool->freeList;
	MemPool->freeList = node;

	return gcvSTATUS_OK;
}

static void
_InitMemArrayPool(
	IN vgSL_MEM_ARRAY_POOL *	MemArrayPool,
	IN gcoOS					OS,
	IN gctUINT					NodeSize
	)
{
	MemArrayPool->os		= OS;
	MemArrayPool->blockList	= gcvNULL;
	MemArrayPool->freeList	= gcvNULL;
	MemArrayPool->nodeSize	= NodeSize;
}

static void
_FreeMemArrayPool(
	IN vgSL_MEM_ARRAY_POOL * MemArrayPool
	)
{
	while (MemArrayPool->blockList != gcvNULL)
	{
		vgSL_MEM_ARRAY_NODE block = MemArrayPool->blockList;

		MemArrayPool->blockList = block->next;
		gcmVERIFY_OK(gcoOS_Free(MemArrayPool->os, block));
	}

	while (MemArrayPool->freeList != gcvNULL)
	{
		vgSL_MEM_ARRAY_NODE block = MemArrayPool->freeList;

		MemArrayPool->freeList = block->next;
		gcmVERIFY_OK(gcoOS_Free(MemArrayPool->os, block));
	}
}

static gceSTATUS
_MemArrayPoolGetANode(
	IN vgSL_MEM_ARRAY_POOL *	MemArrayPool,
	IN gctUINT					Count,
	OUT gctPOINTER *			Node
	)
{
	gceSTATUS					status = gcvSTATUS_OK;
	vgSL_MEM_ARRAY_NODE			list;

	for (list = MemArrayPool->freeList; list; list = list->next)
	{
		if (list->nodeCount >= Count) break;
	}

	if (list)
	{
		/* Unlink list from freeList */
		if (list->prev != gcvNULL)
		{
			list->prev->next = list->next;
		}
		else
		{
			gcmASSERT(list == MemArrayPool->freeList);
			MemArrayPool->freeList = list->next;
		}
		if (list->next != gcvNULL)
		{
			list->next->prev = list->prev;
		}
	}
	else
	{
		/* Allocate a new array node */
		gcmERR_RETURN(gcoOS_Allocate(MemArrayPool->os,
						Count * MemArrayPool->nodeSize + MEM_ARRAY_NODE_DATA_OFFSET,
						(gctPOINTER *) &list));

		list->nodeCount = Count;
	}

	/* Insert list to blockList */
	list->prev = gcvNULL;
	list->next = MemArrayPool->blockList;
	if (MemArrayPool->blockList)
	{
		MemArrayPool->blockList->prev = list;
	}
	MemArrayPool->blockList = list;

	*Node = (gctPOINTER) ((gctSTRING) list + MEM_ARRAY_NODE_DATA_OFFSET);

	gcmVERIFY_OK(gcoOS_ZeroMemory(*Node, Count * MemArrayPool->nodeSize));

	return gcvSTATUS_OK;
}

static gceSTATUS
_MemArrayPoolFreeANode(
	IN vgSL_MEM_ARRAY_POOL *	MemArrayPool,
	IN gctPOINTER				Node
	)
{
	vgSL_MEM_ARRAY_NODE list = (vgSL_MEM_ARRAY_NODE) ((gctSTRING) Node - MEM_ARRAY_NODE_DATA_OFFSET);
	vgSL_MEM_ARRAY_NODE flist, plist;

	/* Unlink list from blockList */
	if (list->prev != gcvNULL)
	{
		list->prev->next = list->next;
	}
	else
	{
		gcmASSERT(list == MemArrayPool->blockList);
		MemArrayPool->blockList = list->next;
	}
	if (list->next != gcvNULL)
	{
		list->next->prev = list->prev;
	}

	/* Insert list to freeList in ascending order in terms of count */
	if (MemArrayPool->freeList == gcvNULL)
	{
		list->prev = list->next = gcvNULL;
		MemArrayPool->freeList = list;
		return gcvSTATUS_OK;
	}

	plist = gcvNULL;
	for (flist = MemArrayPool->freeList; flist; flist = flist->next)
	{
		if (flist->nodeCount >= list->nodeCount) break;
		plist = flist;
	}

	if (plist)
	{
		list->next = plist->next;
		if (plist->next != gcvNULL)
		{
			plist->next->prev = list;
		}
		list->prev = plist;
		plist->next = list;
	}
	else
	{
		list->prev = gcvNULL;
		MemArrayPool->freeList->prev = list;
		list->next = MemArrayPool->freeList;
		MemArrayPool->freeList = list;
	}

	return gcvSTATUS_OK;
}

void
vgSL_InitializeMemPool(
	IN vgSCANLINETESS	ScanLineTess
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	gcoOS				os = ScanLineTess->os;

	_InitMemPool(&ScanLineTess->subPathMemPool,		os,  10, sizeof(struct _vgSL_SUBPATH));
	_InitMemPool(&ScanLineTess->pointMemPool,		os, 100, sizeof(struct _vgSL_POINT));
	_InitMemPool(&ScanLineTess->xCoordMemPool,		os, 500, sizeof(struct _vgSL_XCOORD));
#if SCAN_LINE_HAS_3D_SUPPORT
	_InitMemPool(&ScanLineTess->regionMemPool,		os,  10, sizeof(struct _vgSL_REGION));
	_InitMemPool(&ScanLineTess->regionPointMemPool,	os, 100, sizeof(struct _vgSL_REGIONPOINT));
	_InitMemPool(&ScanLineTess->triangleMemPool,	os, 500, sizeof(struct _vgSL_TRIANGLE));
#endif
	_InitMemArrayPool(&ScanLineTess->yScanLineMemArrayPool,	os, sizeof(struct _vgSL_YSCANLINE));
	_InitMemArrayPool(&ScanLineTess->vector2MemArrayPool,	os, sizeof(struct _vgSL_VECTOR2));
#endif
}

void
vgSL_CleanupMemPool(
	IN vgSCANLINETESS	ScanLineTess
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	_FreeMemPool(&ScanLineTess->subPathMemPool);
	_FreeMemPool(&ScanLineTess->pointMemPool);
	_FreeMemPool(&ScanLineTess->xCoordMemPool);
#if SCAN_LINE_HAS_3D_SUPPORT
	_FreeMemPool(&ScanLineTess->regionPointMemPool);
	_FreeMemPool(&ScanLineTess->regionMemPool);
	_FreeMemPool(&ScanLineTess->triangleMemPool);
#endif
	_FreeMemArrayPool(&ScanLineTess->yScanLineMemArrayPool);
	_FreeMemArrayPool(&ScanLineTess->vector2MemArrayPool);
#endif
}

gceSTATUS
vgSL_AllocateASubPath(
	vgSCANLINETESS		ScanLineTess,
	vgSL_SUBPATH *		SubPath
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

#if _USE_LOCAL_MEMORY_POOL_
	gcmERR_RETURN(_MemPoolGetANode(&ScanLineTess->subPathMemPool, (gctPOINTER *) SubPath));
#else
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
						gcmSIZEOF(struct _vgSL_SUBPATH),
						(gctPOINTER *) SubPath));

	gcmVERIFY_OK(gcoOS_ZeroMemory(*SubPath, gcmSIZEOF(struct _vgSL_SUBPATH)));
#endif
	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FreeASubPath(
	vgSCANLINETESS		ScanLineTess,
	vgSL_SUBPATH		SubPath
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	return(_MemPoolFreeANode(&ScanLineTess->subPathMemPool, (gctPOINTER) SubPath));
#else
	return(gcoOS_Free(ScanLineTess->os, SubPath));
#endif
}

gceSTATUS
vgSL_AllocateAPoint(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT *		Point
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

#if _USE_LOCAL_MEMORY_POOL_
	gcmERR_RETURN(_MemPoolGetANode(&ScanLineTess->pointMemPool, (gctPOINTER *) Point));
#else
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
						gcmSIZEOF(struct _vgSL_POINT),
						(gctPOINTER *) Point));

	gcmVERIFY_OK(gcoOS_ZeroMemory(*Point, gcmSIZEOF(struct _vgSL_POINT)));
#endif
	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FreeAPoint(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	return(_MemPoolFreeANode(&ScanLineTess->pointMemPool, (gctPOINTER) Point));
#else
	return(gcoOS_Free(ScanLineTess->os, Point));
#endif
}

#if SCAN_LINE_HAS_3D_SUPPORT
gceSTATUS
vgSL_AllocateARegion(
	vgSCANLINETESS		ScanLineTess,
	vgSL_REGION *		Region
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

#if _USE_LOCAL_MEMORY_POOL_
	gcmERR_RETURN(_MemPoolGetANode(&ScanLineTess->regionMemPool, (gctPOINTER *) Region));
#else
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
						gcmSIZEOF(struct _vgSL_REGION),
						(gctPOINTER *) Region));

	gcmVERIFY_OK(gcoOS_ZeroMemory(*Region, gcmSIZEOF(struct _vgSL_REGION)));
#endif
	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FreeARegion(
	vgSCANLINETESS		ScanLineTess,
	vgSL_REGION			Region
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	return(_MemPoolFreeANode(&ScanLineTess->regionMemPool, (gctPOINTER) Region));
#else
	return(gcoOS_Free(ScanLineTess->os, Region));
#endif
}

gceSTATUS
vgSL_AllocateARegionPoint(
	vgSCANLINETESS		ScanLineTess,
	vgSL_REGIONPOINT *	RegionPoint
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

#if _USE_LOCAL_MEMORY_POOL_
	gcmERR_RETURN(_MemPoolGetANode(&ScanLineTess->regionPointMemPool, (gctPOINTER *) RegionPoint));
#else
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
						gcmSIZEOF(struct _vgSL_REGIONPOINT),
						(gctPOINTER *) RegionPoint));

	gcmVERIFY_OK(gcoOS_ZeroMemory(*RegionPoint, gcmSIZEOF(struct _vgSL_POINT)));
#endif
	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FreeARegionPoint(
	vgSCANLINETESS		ScanLineTess,
	vgSL_REGIONPOINT	RegionPoint
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	return(_MemPoolFreeANode(&ScanLineTess->regionPointMemPool, (gctPOINTER) RegionPoint));
#else
	return(gcoOS_Free(ScanLineTess->os, RegionPoint));
#endif
}

gceSTATUS
vgSL_AllocateATriangle(
	vgSCANLINETESS		ScanLineTess,
	vgSL_TRIANGLE *		Triangle
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

#if _USE_LOCAL_MEMORY_POOL_
	gcmERR_RETURN(_MemPoolGetANode(&ScanLineTess->triangleMemPool, (gctPOINTER *) Triangle));
#else
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
						gcmSIZEOF(struct _vgSL_TRIANGLE),
						(gctPOINTER *) Triangle));

	gcmVERIFY_OK(gcoOS_ZeroMemory(*RegionPoint, gcmSIZEOF(struct _vgSL_TRIANGLE)));
#endif
	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FreeATriangle(
	vgSCANLINETESS		ScanLineTess,
	vgSL_TRIANGLE		Triangle
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	return(_MemPoolFreeANode(&ScanLineTess->triangleMemPool, (gctPOINTER) Triangle));
#else
	return(gcoOS_Free(ScanLineTess->os, Triangle));
#endif
}
#endif

gceSTATUS
vgSL_AllocateAnXCoord(
	vgSCANLINETESS		ScanLineTess,
	vgSL_XCOORD *		XCoord
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

#if _USE_LOCAL_MEMORY_POOL_
	gcmERR_RETURN(_MemPoolGetANode(&ScanLineTess->xCoordMemPool, (gctPOINTER *) XCoord));
#else
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
						gcmSIZEOF(struct _vgSL_XCOORD),
						(gctPOINTER *) XCoord));

	gcmVERIFY_OK(gcoOS_ZeroMemory(*XCoord, gcmSIZEOF(struct _vgSL_XCOORD)));
#endif
	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FreeAnXCoord(
	vgSCANLINETESS		ScanLineTess,
	vgSL_XCOORD			XCoord
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	return(_MemPoolFreeANode(&ScanLineTess->xCoordMemPool, (gctPOINTER) XCoord));
#else
	return(gcoOS_Free(ScanLineTess->os, XCoord));
#endif
}

gceSTATUS
vgSL_AllocateAYScanLineArray(
	vgSCANLINETESS		ScanLineTess,
	vgSL_YSCANLINE *	YScanLineArray,
	gctUINT Count
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

#if _USE_LOCAL_MEMORY_POOL_
	gcmERR_RETURN(_MemArrayPoolGetANode(&ScanLineTess->yScanLineMemArrayPool, Count, (gctPOINTER *) YScanLineArray));
#else
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
						Count * gcmSIZEOF(struct _vgSL_YSCANLINE),
						(gctPOINTER *) YScanLineArray));

	gcmVERIFY_OK(gcoOS_ZeroMemory(*YScanLineArray, Count * gcmSIZEOF(struct _vgSL_YSCANLINE)));
#endif
	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FreeAYScanLineArray(
	vgSCANLINETESS		ScanLineTess,
	vgSL_YSCANLINE		YScanLineArray
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	return(_MemArrayPoolFreeANode(&ScanLineTess->yScanLineMemArrayPool, (gctPOINTER) YScanLineArray));
#else
	return(gcoOS_Free(ScanLineTess->os, YScanLineArray));
#endif
}

gceSTATUS
vgSL_AllocateAVector2Array(
	vgSCANLINETESS		ScanLineTess,
	vgSL_VECTOR2 *		Vector2Array,
	gctUINT Count
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

#if _USE_LOCAL_MEMORY_POOL_
	gcmERR_RETURN(_MemArrayPoolGetANode(&ScanLineTess->vector2MemArrayPool, Count, (gctPOINTER *) Vector2Array));
#else
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
						Count * gcmSIZEOF(struct _vgSL_YSCANLINE),
						(gctPOINTER *) Vector2Array));

	gcmVERIFY_OK(gcoOS_ZeroMemory(*Vector2Array, Count * gcmSIZEOF(struct _vgSL_VECTOR2)));
#endif
	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FreeAVector2Array(
	vgSCANLINETESS		ScanLineTess,
	vgSL_VECTOR2		Vector2Array
	)
{
#if _USE_LOCAL_MEMORY_POOL_
	return(_MemArrayPoolFreeANode(&ScanLineTess->vector2MemArrayPool, (gctPOINTER) Vector2Array));
#else
	return(gcoOS_Free(ScanLineTess->os, Vector2Array));
#endif
}

/***************************************************************************\
|*************************** Scan Line Functions ***************************|
\***************************************************************************/

/*******************************************************************************
**							vgSL_ConstructScanLineTess
********************************************************************************
**
**	Construct a new vgSCANLINETESS structure.
**
**	INPUT:
**
**		_VGContext * Context
**			Pointer to a _VGContext structure holding the environment data.
**
**	OUT:
**
**		vgSCANLINETESS * ScanLineTess
**			Pointer to a vgSCANLINETESS structure.
*/
gceSTATUS
vgSL_ConstructScanLineTess(
	IN _VGContext *			Context,
	OUT vgSCANLINETESS *	ScanLineTess
	)
{
	gceSTATUS				status = gcvSTATUS_OK;
	vgSCANLINETESS			scanLineTess = gcvNULL;

	do
	{
		/* Allocate vgSCANLINETESS data structure. */
#if USE_SCAN_LINE_ON_RI
		gcmERR_RETURN(gcoOS_Allocate(NULL,
						gcmSIZEOF(struct _vgSCANLINETESS),
						(gctPOINTER *) &scanLineTess));
#else
		if (Context->os == gcvNULL) return gcvSTATUS_INVALID_DATA;
		gcmERR_RETURN(gcoOS_Allocate(Context->os,
						gcmSIZEOF(struct _vgSCANLINETESS),
						(gctPOINTER *) &scanLineTess));
#endif

		gcmVERIFY_OK(gcoOS_ZeroMemory(scanLineTess, gcmSIZEOF(struct _vgSCANLINETESS)));

		scanLineTess->context			= Context;
#if !USE_SCAN_LINE_ON_RI
		scanLineTess->os				= Context->os;
#endif

		/* Copy parameters from Context. */
		scanLineTess->strokeLineWidth	= FLOAT_TO_COORD(VGSL_GETCONTEXTSTOKELINEWIDTH(Context));
		scanLineTess->strokeCapStyle	= VGSL_GETCONTEXTSTOKECAPSTYLE(Context);
		scanLineTess->strokeJoinStyle	= VGSL_GETCONTEXTSTOKEJOINSTYLE(Context);
		scanLineTess->strokeMiterLimit	= FLOAT_TO_COORD(VGSL_GETCONTEXTSTOKEMITERLIMIT(Context));

		/* Stroke dash parameters are copied in during stroke tessellation. */

		/* Initialize internal scales. */
		scanLineTess->scale				= COORD_ONE;
		scanLineTess->strokeScale		= COORD_ONE;

		scanLineTess->minXCoord			= COORD_MAX;
		scanLineTess->maxXCoord			= COORD_MIN;
		scanLineTess->minYCoord			= COORD_MAX;
		scanLineTess->maxYCoord			= COORD_MIN;

		/* Initialize memory pools. */
		vgSL_InitializeMemPool(scanLineTess);
	}
	while (gcvFALSE);

	*ScanLineTess = scanLineTess;
	return status;
}

/*******************************************************************************
**							vgSL_DestroyScanLineTess
********************************************************************************
**
**	Destroy a vgSCANLINETESS structure.
**
**	IN:
**
**		vgSCANLINETESS * ScanLineTess
**			Pointer to a vgSCANLINETESS structure.
*/
gceSTATUS
vgSL_DestroyScanLineTess(
	IN vgSCANLINETESS	ScanLineTess
	)
{
	if (ScanLineTess == gcvNULL) return gcvSTATUS_OK;

	if (ScanLineTess->strokeDashPatternCount > 0)
	{
		gcmVERIFY_OK(gcoOS_Free(ScanLineTess->os, ScanLineTess->strokeDashPattern));
	}

	/* Free subPath list. */
	if (ScanLineTess->subPathList)
	{
		vgSL_SUBPATH subPath, nextSubPath;

		for (subPath = ScanLineTess->subPathList; subPath; subPath = nextSubPath)
		{
			vgSL_POINT point, nextPoint;
			VGint i;

			nextSubPath = subPath->next;
			for (i = 0, point = subPath->pointList; i < subPath->pointCount; i++, point = nextPoint)
			{
				nextPoint = point->next;
				vgSL_FreeAPoint(ScanLineTess, point);
			}
#if SCAN_LINE_HAS_3D_SUPPORT
			gcmASSERT(point == subPath->pointList || point == gcvNULL);
#else
			gcmASSERT(point == gcvNULL);
#endif
			vgSL_FreeASubPath(ScanLineTess, subPath);
		}
	}
	if (ScanLineTess->strokeSubPathList)
	{
		vgSL_SUBPATH subPath, nextSubPath;

		for (subPath = ScanLineTess->strokeSubPathList; subPath; subPath = nextSubPath)
		{
			vgSL_POINT point, nextPoint;
			VGint i;

			nextSubPath = subPath->next;
			for (i = 0, point = subPath->pointList; i < subPath->pointCount; i++, point = nextPoint)
			{
				if (point == gcvNULL)
				{
					gcmASSERT(point);
					break;
				}
				nextPoint = point->next;
				vgSL_FreeAPoint(ScanLineTess, point);
			}
			gcmASSERT(point == gcvNULL);
			vgSL_FreeASubPath(ScanLineTess, subPath);
		}
	}

	/* Free yScanLine array. */
	if (ScanLineTess->yScanLineArray)
	{
		vgSL_YSCANLINE yScanLine = ScanLineTess->yScanLineArray;
		VGint i;

		/* Free xCoords. */
		for (i = ScanLineTess->yMin; i <= ScanLineTess->yMax; i++, yScanLine++)
		{
			while (yScanLine->xcoords)
			{
				vgSL_XCOORD xCoord = yScanLine->xcoords;

				yScanLine->xcoords = xCoord->next;
				gcmVERIFY_OK(vgSL_FreeAnXCoord(ScanLineTess, xCoord));
			}
		}

		gcmVERIFY_OK(vgSL_FreeAYScanLineArray(ScanLineTess, ScanLineTess->yScanLineArray));
	}

	/* Free memory pools. */
	vgSL_CleanupMemPool(ScanLineTess);

	/* Free optimizer data structure. */
	gcmVERIFY_OK(gcoOS_Free(ScanLineTess->os, ScanLineTess));

	return gcvSTATUS_OK;
}

INLINE void
vgSL_NormalizePointTangent(
	vgSL_POINT			Point
	)
{
	VGfloat x = COORD_TO_FLOAT(Point->tangentX);
	VGfloat y = COORD_TO_FLOAT(Point->tangentY);
	VGfloat l = x * x + y * y;
	if (l != 0.0f)
	{
		l = 1.0f / (VGfloat) sqrt((double) l);
		Point->tangentX = FLOAT_TO_COORD(x * l);
		Point->tangentY = FLOAT_TO_COORD(y * l);
	}
}

void
vgSL_SetPointTangent(
	vgSL_POINT			Point
	)
{
	vgSL_POINT			point1 = Point->next;
	VGfloat				x, y, l;

	if (point1 == gcvNULL)
	{
		/* Zero length line. */
		Point->length = COORD_ZERO;
		Point->tangentX = COORD_ZERO;
		Point->tangentY = COORD_ZERO;
		return;
	}

	x = COORD_TO_FLOAT(point1->x - Point->x);
	y = COORD_TO_FLOAT(point1->y - Point->y);
	l = x * x + y * y;
	if (l != 0.0f)
	{
	    vgCOORD angle;
		Point->length = l = (VGfloat) sqrt((double) l);
		l = 1.0f / l;
		Point->tangentX = x = FLOAT_TO_COORD(x * l);
		Point->tangentY = y = FLOAT_TO_COORD(y * l);
		angle = COORD_ACOS(x);
		if (y < 0) angle = -angle;
		if (angle < COORD_ZERO) angle += FLOAT_TO_COORD(PI_TWO);
		//Point->angle = COORD_MUL(COORD_DIV(angle, PI), INT_TO_COORD(vgcCOS_TABLE_SIZE << 1));
		Point->angle = angle;
	}
	else
	{
		Point->length = COORD_ZERO;
		Point->tangentX = COORD_ZERO;
		Point->tangentY = COORD_ZERO;
	}
}

gceSTATUS
vgSL_AddSubPath(
	vgSCANLINETESS	ScanLineTess,
	vgSL_SUBPATH *	SubPath
	)
{
	gceSTATUS		status = gcvSTATUS_OK;

	gcmERR_RETURN(vgSL_AllocateASubPath(ScanLineTess, SubPath));

	if (ScanLineTess->lastSubPath != gcvNULL)
	{
		ScanLineTess->lastSubPath->next = *SubPath;
		ScanLineTess->lastSubPath = *SubPath;
	}
	else
	{
		ScanLineTess->lastSubPath = ScanLineTess->subPathList = *SubPath;
	}

	return status;
}

gceSTATUS
vgSL_AddPointToSubPath(
	vgSCANLINETESS	ScanLineTess,
	vgCOORD			X,
	vgCOORD			Y,
	vgSL_SUBPATH	SubPath
	)
{
	gceSTATUS		status = gcvSTATUS_OK;
	vgSL_POINT		lastPoint = SubPath->lastPoint;
	vgSL_POINT		point;

	/* Check if the point is the same as the previous point. */
	if (lastPoint != gcvNULL)
	{
		vgCOORD deltaX = COORD_ABS(X - lastPoint->x);
		vgCOORD deltaY = COORD_ABS(Y - lastPoint->y);

		gcmASSERT(SubPath->pointCount > 0);
		/* Check for degenerated line. */
		if (deltaX == COORD_ZERO && deltaY == COORD_ZERO)
		{
			/* Skip degenerated line. */
			return gcvSTATUS_OK;
		}
		if (deltaX < COORD_ROUNDING_EPSILON && deltaY < COORD_ROUNDING_EPSILON)
		{
			vgCOORD ratioX, ratioY;

			if (deltaX == COORD_ZERO)
			{
				ratioX = COORD_ZERO;
			}
			else if (X == COORD_ZERO)
			{
				ratioX = deltaX;
			}
			else
			{
				ratioX = COORD_ABS(COORD_DIV(deltaX, X));
			}
			if (deltaY == COORD_ZERO)
			{
				ratioY = COORD_ZERO;
			}
			else if (Y == COORD_ZERO)
			{
				ratioY = deltaY;
			}
			else
			{
				ratioY = COORD_ABS(COORD_DIV(deltaY, Y));
			}
			if (ratioX < 1.0e-6f && ratioY < 1.0e-6f)
			{
				/* Skip degenerated line. */
				return gcvSTATUS_OK;
			}
		}
	}

	gcmERR_RETURN(vgSL_AllocateAPoint(ScanLineTess, &point));

	point->x = X;
	point->y = Y;

	if (ScanLineTess->isFlattening) point->isFlattened = VG_TRUE;

	if (lastPoint != gcvNULL)
	{
		point->prev = lastPoint;
		lastPoint->next = point;
		SubPath->lastPoint = point;
		vgSL_SetPointTangent(lastPoint);
	}
	else
	{
		SubPath->lastPoint = SubPath->pointList = point;
	}
	SubPath->pointCount++;

	return status;
}

INLINE VGboolean
vgSL_SnapCoordinate(
	vgCOORD *			X
	)
{
	vgCOORD				frac;

#if USE_FIXED_POINT_FOR_COORDINATES
	if (COORD_FRAC(*X) == 0) return VG_TRUE;

	frac = (VGuint)(*X) & 0x0000ffc0;
	if (frac == 0x0000ffc0)
	{
		*X = (vgCOORD)((VGuint)(*X) & 0xffff0000) + 0x00010000;
		return VG_TRUE;
	}
	if (frac == 0)
	{
		*X = (vgCOORD)((VGuint)(*X) & 0xffffffc0);
		return VG_TRUE;
	}
	return VG_FALSE;
#else
	vgCOORD			floorX = COORD_FLOOR(*X);

	frac = *X - floorX;
	if (frac == COORD_ZERO) return VG_TRUE;

	if (frac < COORD_EPSILON)
	{
		*X = floorX;
		return VG_TRUE;
	}
	if (COORD_ONE - frac < COORD_EPSILON)
	{
		*X = floorX + COORD_ONE;
		return VG_TRUE;
	}
	return VG_FALSE;
#endif
}

INLINE void
vgSL_SnapPoint(
	vgSL_POINT			Point
	)
{
#if !FOR_CONFORMANCE_TEST
	vgSL_SnapCoordinate(&Point->x);
	Point->yIsInteger = vgSL_SnapCoordinate(&Point->y);
#endif
}

void
vgSL_CalculatePointDeltaXAndOrientation(
	vgSL_POINT			Point1,
	vgSL_POINT			Point2
	)
{
	vgCOORD				deltaX = Point2->x - Point1->x;
	vgCOORD				deltaY = Point2->y - Point1->y;

	if (COORD_ABS(deltaY) >= COORD_EPSILON)
	{
		Point1->deltaX = COORD_DIV(Point2->x - Point1->x, deltaY);
		Point1->orientation = deltaY > COORD_ZERO ? vgvSL_UP : vgvSL_DOWN;
	}
	else if (COORD_ABS(deltaX) >= COORD_EPSILON)
	{
		Point1->deltaX = deltaX > COORD_ZERO ? COORD_MAX : COORD_MIN;
		Point1->orientation = vgvSL_FLAT;
	}
	else
	{
		/* Special points to handle incoming and outgoing tangent. */
		gcmASSERT(Point1->isFlattened || Point2->isFlattened);
	}
}

void
vgSL_CalculatePathDeltaXAndOrientation(
	vgSCANLINETESS		ScanLineTess,
	vgSL_SUBPATH		SubPathList,
	VGboolean			NeedToFilter
	)
{
	vgCOORD				intY, prevIntY, nextIntY;
	vgSL_SUBPATH		subPath;
	vgSL_POINT			point;
	vgSL_POINT			nextPoint;
	vgSL_POINT			prevPoint;
	VGboolean			checkIntY;

	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		if (subPath->pointCount < 2) continue;

		/* Snap first two points. */
		prevPoint = subPath->pointList;
		point = prevPoint->next;
		vgSL_SnapPoint(prevPoint);
		vgSL_SnapPoint(point);
		if (NeedToFilter)
		{
			while (point->x == prevPoint->x && point->y == prevPoint->y)
			{
				/* point is redundant in scan line algorithm. */
				prevPoint->next = point->next;
				vgSL_FreeAPoint(ScanLineTess, point);
				subPath->pointCount--;
				point = prevPoint->next;
				vgSL_SnapPoint(point);
			}
		}
		prevIntY = COORD_FLOOR(prevPoint->y);
		intY = COORD_FLOOR(point->y);
		checkIntY = (NeedToFilter && intY == prevIntY) ? VG_TRUE : VG_FALSE;
		if (! checkIntY)
		{
			vgSL_CalculatePointDeltaXAndOrientation(prevPoint, point);
		}

		for (nextPoint = point->next; nextPoint; nextPoint = point->next)
		{
			/* Snap nextPoint. */
			vgSL_SnapPoint(nextPoint);
			nextIntY = COORD_FLOOR(nextPoint->y);
			if (NeedToFilter && point->x == nextPoint->x && point->y == nextPoint->y)
			{
				/* nextPoint is redundant in scan line algorithm. */
				point->next = nextPoint->next;
				vgSL_FreeAPoint(ScanLineTess, nextPoint);
				subPath->pointCount--;
				continue;
			}

			/* Eliminat point if prevPoint, point and nextPoint are in the same interval. */
			if (checkIntY)
			{
				if (intY == nextIntY)
				{
					/* point is redundant in scan line algorithm. */
					prevPoint->next = nextPoint;
					vgSL_FreeAPoint(ScanLineTess, point);
					subPath->pointCount--;
					point = nextPoint;
					continue;
				}
				else
				{
					/* Calculate deltaX and orientation for prevPoint and point. */
					vgSL_CalculatePointDeltaXAndOrientation(prevPoint, point);
					vgSL_CalculatePointDeltaXAndOrientation(point, nextPoint);
					checkIntY = VG_FALSE;
				}
			}
			else if (NeedToFilter && intY == nextIntY)
			{
				checkIntY = VG_TRUE;
			}
			else
			{
				/* Calculate deltaX and orientation for point. */
				vgSL_CalculatePointDeltaXAndOrientation(point, nextPoint);
			}

			prevPoint = point;
			point = nextPoint;
			prevIntY = intY;
			intY = nextIntY;
		}

		if (checkIntY)
		{
			/* Calculate deltaX and orientation for prevPoint. */
			vgSL_CalculatePointDeltaXAndOrientation(prevPoint, point);
		}

		/* Process the last point if not the same as the first point. */
		nextPoint = subPath->pointList;
		if (point->x != nextPoint->x || point->y != nextPoint->y)
		{
			/* Calculate deltaX and orientation for point. */
			vgSL_CalculatePointDeltaXAndOrientation(point, nextPoint);
		}
	}
}

gceSTATUS
vgSL_FlattenQuadBezier(
	vgSCANLINETESS	ScanLineTess,
	vgCOORD			X0,
	vgCOORD			Y0,
	vgCOORD			X1,
	vgCOORD			Y1,
	vgCOORD			X2,
	vgCOORD			Y2,
	vgSL_SUBPATH	SubPath
	)
{
	gceSTATUS		status = gcvSTATUS_OK;
	vgSL_POINT		point0, point1;
	VGint			i, n;
	vgCOORD			x, y;
	vgCOORD			dxC, dyC, ddxC, ddyC;
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
	/* Due to overflow/underflow problem when using fixed point, use float for calculation. */
	vgCOORD			a1x, a1y, a2x, a2y;
	_VGint64		c0, c1, d0, d1, f1, f2, t1, t2, upperBound;
	_VGint64		dx, dy, ddx, ddy;
	VGint			nShift, dShift, dsquareShift;
#else
	/* Due to overflow/underflow problem when using fixed point, use float for calculation. */
	VGfloat			a1x, a1y, a2x, a2y;
	VGfloat			c0, c1, d0, d1, f1, f2, t1, t2, upperBound;
	VGfloat			d, dsquare, dx, dy, ddx, ddy;
#endif

	/* Formula.
	 * f(t) = (1 - t)^2 * p0 + 2 * t * (1 - t) * p1 + t^2 * p2
	 *      = a0 + a1 * t + a2 * t^2
	 *   a0 = p0
	 *   a1 = 2 * (-p0 + p1)
	 *   a2 = p0 - 2 * p1 + p2
	 */
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
	x = X1 - X0;
	a1x = x + x;
	y = Y1 - Y0;
	a1y = y + y;
	a2x = X0 - X1 - X1 + X2;
	a2y = Y0 - Y1 - Y1 + Y2;
#else
	x = -X0 + X1;
	x += x;
	a1x = COORD_TO_FLOAT(x);
	y = -Y0 + Y1;
	y += y;
	a1y = COORD_TO_FLOAT(y);
	x = X0 - X1 - X1 + X2;
	a2x = COORD_TO_FLOAT(x);
	y = Y0 - Y1 - Y1 + Y2;
	a2y = COORD_TO_FLOAT(y);
#endif

	/* Step 1: Calculate N. */
	/* Lefan's method. */
	/* dist(t) = ...
	 * t2 = ...
	 * if 0 <= t2 <= 1
	 *    upperBound = dist(t2)
	 * else
	 *    upperBound = max(dist(0), dist(1))
	 * N = ceil(sqrt(upperbound / epsilon / 8))
	 */
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
	/* Prepare dist(t). */
	c0	= (_VGint64) (X1 - X0);
	c1	= (_VGint64) (X2 + X0 - X1 - X1);
	d0	= (_VGint64) (Y1 - Y0);
	d1	= (_VGint64) (Y2 + Y0 - Y1 - Y1);
	f1	= c0 * d1 - c1 * d0;
	if (f1 < 0) f1 = -f1;
	f1 += f1;
	if (f1 > 0)
	{
		/* Calculate t2. */
		t1	= (_VGint64) (X2 + X0 - X1 - X1);
		t1 *= t1;
		t2	= (_VGint64) (Y2 + Y0 - Y1 - Y1);
		t2 *= t2;
		t1 += t2;
		t2	= ((_VGint64) (X0 - X1)) * (X2 + X0 - X1 - X1);
		t2 += ((_VGint64) (Y0 - Y1)) * (Y2 + Y0 - Y1 - Y1);
		if (t1 > COORD_ONE)
		{
			t2	= (t2 / (t1 >> FRACTION_BITS));
		}
		else if (t1 != 0)
		{
			t2 = (t2 << FRACTION_BITS) / t1;
		}
		else
		{
			t2 = COORD_TWO;
		}
		/* Calculate upperBound. */
		if (t2 >= 0 && t2 <= COORD_ONE)
		{
			f2 = c0 + ((c1 * t2) >> FRACTION_BITS);
			f2 *= f2;
			t1 = d0 + ((d1 * t2) >> FRACTION_BITS);
			t1 *= t1;
			upperBound = f1 / ((_VGint64) sqrtf((VGfloat)(t1 + f2)));
		}
		else
		{
			f2 = c0 + c1;
			f2 *= f2;
			t1 = d0 + d1;
			t1 *= t1;
			t2 = f1 / ((_VGint64) sqrtf((VGfloat)(t1 + f2)));
			t1 = f1 / ((_VGint64) sqrtf((VGfloat)(c0 * c0 + d0 * d0)));
			upperBound = t1 > t2 ? t1 : t2;
		}

		/* Calculate n. */
		//n = (VGint) ceil(sqrtf(upperBound * 0.125f * DIFF_EPSILON_RCP));
		n = (VGint) (upperBound >> FRACTION_BITS);
		if (n > 256)
		{
			if (n > 4096)
			{
				if (n > 16384)	{ n = 256;	nShift = 8; }
				else			{ n = 128;	nShift = 7; }
			}
			else
			{
				if (n > 1024)	{ n = 64;	nShift = 6; }
				else			{ n = 32;	nShift = 5; }
			}
		}
		else
		{
			if (n > 16)
			{
				if (n > 64)		{ n = 16;	nShift = 4; }
				else			{ n = 8;	nShift = 3; }
			}
			else
			{
				if (n > 4)		{ n = 4;	nShift = 2; }
				else if (n > 1)	{ n = 2;	nShift = 1; }
				else			{ n = 1;	nShift = 0; }
			}
		}
	}
	else
	{
		/* n = 0 => n = 256. */
		n = 256;	nShift = 8;
	}
#else
	/* Prepare dist(t). */
	c0 = COORD_TO_FLOAT(X1 - X0);
	c1 = COORD_TO_FLOAT(X2 + X0 - X1 - X1);
	d0 = COORD_TO_FLOAT(Y1 - Y0);
	d1 = COORD_TO_FLOAT(Y2 + Y0 - Y1 - Y1);
	f1 = fabsf(c0 * d1 - c1 * d0);
	f1 += f1;
	if (f1 > 0.0f)
	{
		/* Calculate t2. */
		t1 = COORD_TO_FLOAT(X2 + X0 - X1 - X1);
		t1 *= t1;
		t2 = COORD_TO_FLOAT(Y2 + Y0 - Y1 - Y1);
		t2 *= t2;
		t1 += t2;
		t2 = ((X0 - X1) * (X2 + X0 - X1 - X1) + (Y0 - Y1) * (Y2 + Y0 - Y1 - Y1)) / t1;
		/* Calculate upperBound. */
		if (t2 >= 0.0f && t2 <= 1.0f)
		{
			f2 = c0 + c1 * t2;
			f2 *= f2;
			t1 = d0 + d1 * t2;
			t1 *= t1;
			upperBound = f1 / sqrtf(t1 + f2);
		}
		else
		{
			f2 = c0 + c1;
			f2 *= f2;
			t1 = d0 + d1;
			t1 *= t1;
			t2 = f1 / sqrtf(t1 + f2);
			t1 = f1 / sqrtf(c0 * c0 + d0 * d0);
			upperBound = t1 > t2 ? t1 : t2;
		}
		/* Calculate n. */
		if ((ScanLineTess->paintModes & VG_STROKE_PATH) && ScanLineTess->strokeLineWidth > FAT_LINE_WIDTH)
		{
			VGfloat width = COORD_TO_FLOAT(ScanLineTess->strokeLineWidth);
			upperBound *= width * width;
		}
		upperBound *= COORD_TO_FLOAT(ScanLineTess->transformScale);
		n = (VGint) ceil(sqrtf(upperBound));
	}
	else
	{
		/* n = 0 => n = 256. */
		n = 256;
	}
#endif

	if (n == 0 || n > 256)
	{
		n = 256;
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
		nShift = 8;
#endif
	}

	/* Turn on flattening flag. */
	ScanLineTess->isFlattening = VG_TRUE;

	/* Add extra P0 for incoming tangent. */
	point0 = SubPath->lastPoint;
	gcmASSERT(point0);
	/* First add P1 to calculate incoming tangent, which is saved in P0. */
	gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X1, Y1, SubPath));
	point1 = SubPath->lastPoint;
#if !FOR_CONFORMANCE_TEST
	vgSL_CalculatePointDeltaXAndOrientation(point0, point1);
#endif
	/* Change the point1's coordinates back to P0. */
	point1->x = X0;
	point1->y = Y0;
	point0->length = COORD_ZERO;

	if (n > 1)
	{
		/* Step 2: Calculate deltas. */
		/*   Df(t) = f(t + d) - f(t)
		 *         = a1 * d + a2 * d^2 + 2 * a2 * d * t
		 *  DDf(t) = Df(t + d) - Df(t)
		 *         = 2 * a2 * d^2
		 *    f(0) = a0
		 *   Df(0) = a1 * d + a2 * d^2
		 *  DDf(0) = 2 * a2 * d^2
		 */
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
		dShift			= nShift;
		dsquareShift	= dShift << 1;
		ddx	= (((_VGint64) a2x) << FRACTION_BITS) >> dsquareShift;
		ddy = (((_VGint64) a2y) << FRACTION_BITS) >> dsquareShift;
		dx	= ((((_VGint64) a1x) << FRACTION_BITS) >> dShift) + ddx;
		dy	= ((((_VGint64) a1y) << FRACTION_BITS) >> dShift) + ddy;
		ddx	+= ddx;
		ddy += ddy;

		dxC   = (vgCOORD) (dx >> FRACTION_BITS);
		dyC   = (vgCOORD) (dy >> FRACTION_BITS);
		ddxC  = (vgCOORD) (ddx >> FRACTION_BITS);
		ddyC  = (vgCOORD) (ddy >> FRACTION_BITS);
#else
		d = 1.0f / (VGfloat) n;
		dsquare = d * d;
		ddx	= a2x * dsquare;
		ddy = a2y * dsquare;
		dx	= a1x * d + ddx;
		dy	= a1y * d + ddy;
		ddx	+= ddx;
		ddy += ddy;

		dxC   = FLOAT_TO_COORD(dx);
		dyC   = FLOAT_TO_COORD(dy);
		ddxC  = FLOAT_TO_COORD(ddx);
		ddyC  = FLOAT_TO_COORD(ddy);
#endif

		/* Step 3: Add points. */
		x	  = X0;
		y	  = Y0;
		for (i = 1; i < n; i++)
		{
			x += dxC; dxC += ddxC;
			y += dyC; dyC += ddyC;

			/* Add a point to subpath. */
			gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x, y, SubPath));
		}
	}

	/* Add point 2 separately to avoid cumulative errors. */
	gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X2, Y2, SubPath));

	/* Add extra P2 for outgoing tangent. */
	/* First change P2(point0)'s coordinates to P1. */
	point0 = SubPath->lastPoint;
	point0->x = X1;
	point0->y = Y1;

	/* Turn off flattening flag. */
	ScanLineTess->isFlattening = VG_FALSE;

	/* Add P2 to calculate outgoing tangent. */
	gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X2, Y2, SubPath));
	point1 = SubPath->lastPoint;
#if !FOR_CONFORMANCE_TEST
	vgSL_CalculatePointDeltaXAndOrientation(point0, point1);
#endif

	/* Change point0's coordinates back to P2. */
	point0->x = X2;
	point0->y = Y2;
	point0->length = COORD_ZERO;

	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_FlattenCubicBezier(
	vgSCANLINETESS	ScanLineTess,
	vgCOORD			X0,
	vgCOORD			Y0,
	vgCOORD			X1,
	vgCOORD			Y1,
	vgCOORD			X2,
	vgCOORD			Y2,
	vgCOORD			X3,
	vgCOORD			Y3,
	vgSL_SUBPATH	SubPath
	)
{
	gceSTATUS		status = gcvSTATUS_OK;
	vgSL_POINT		point0, point1;
	VGint			i, n;
	vgCOORD			x, y;
	vgCOORD			dxC, dyC, ddxC, ddyC, dddxC, dddyC;
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
	/* Due to overflow/underflow problem when using fixed point, use _VGint64 (32, 32) for calculation. */
	vgCOORD			a1x, a1y, a2x, a2y, a3x, a3y;
	vgCOORD			ddf0, ddf1, upperBound;
	_VGint64		t1, t2;
	_VGint64		dx, dy, ddx, ddy, dddx, dddy;
	VGint			nShift, dShift, dsquareShift, dcubeShift;
#else
	/* Due to overflow/underflow problem when using fixed point, use float for calculation. */
	VGfloat			a1x, a1y, a2x, a2y, a3x, a3y;
	VGfloat			ddf0, ddf1, t1, t2, upperBound;
	VGfloat			d, dsquare, dcube, dx, dy, ddx, ddy, dddx, dddy;
#endif

	/* Formula.
	 * f(t) = (1 - t)^3 * p0 + 3 * t * (1 - t)^2 * p1 + 3 * t^2 * (1 - t) * p2 + t^3 * p3
	 *      = a0 + a1 * t + a2 * t^2 + a3 * t^3
	 */
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
	x = X1 - X0;
	a1x = x + x + x;
	y = Y1 - Y0;
	a1y = y + y + y;
	x = X0 - X1 - X1 + X2;
	a2x = x + x + x;
	y = Y0 - Y1 - Y1 + Y2;
	a2y = y + y + y;
	x = X1 - X2;
	a3x = x + x + x + X3 - X0;
	y = Y1 - Y2;
	a3y = y + y + y + Y3 - Y0;
#else
	x = -X0 + X1;
	x = x + x + x;
	a1x = COORD_TO_FLOAT(x);
	y = -Y0 + Y1;
	y = y + y + y;
	a1y = COORD_TO_FLOAT(y);
	x = X0 - X1 - X1 + X2;
	x = x + x + x;
	a2x = COORD_TO_FLOAT(x);
	y = Y0 - Y1 - Y1 + Y2;
	y = y + y + y;
	a2y = COORD_TO_FLOAT(y);
	x = X1 - X2;
	x = -X0 + x + x + x + X3;
	a3x = COORD_TO_FLOAT(x);
	y = Y1 - Y2;
	y = -Y0 + y + y + y + Y3;
	a3y = COORD_TO_FLOAT(y);
#endif

	/* Step 1: Calculate N. */
	/* Lefan's method. */
	/*  df(t)/dt  = a1 + 2 * a2 * t + 3 * a3 * t^2
	 * d2f(t)/dt2 = 2 * a2 + 6 * a3 * t
	 * N = ceil(sqrt(max(ddfx(0)^2 + ddfy(0)^2, ddfx(1)^2 + ddyf(1)^2) / epsilon / 8))
	 */
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
	ddf0 = (vgCOORD) sqrtf((VGfloat)(a2x * a2x + a2y * a2y));
	ddf0 += ddf0;
	t1 = (_VGint64) (a2x + a3x + a3x + a3x);
	t1 += t1;
	t1 *= t1;
	t2 = (_VGint64) (a2y + a3y + a3y + a3y);
	t2 += t2;
	t2 *= t2;
	ddf1 = (vgCOORD) sqrtf((VGfloat)(t1 + t2));
	upperBound = ddf0 > ddf1 ? ddf0 : ddf1;
	//n = (VGint) ceil(sqrtf(upperBound * 0.125f * DIFF_EPSILON_RCP));
	n = upperBound >> FRACTION_BITS;
	if (n > 256)
	{
		if (n > 4096)
		{
			if (n > 16384)	{ n = 256;	nShift = 8; }
			else			{ n = 128;	nShift = 7; }
		}
		else
		{
			if (n > 1024)	{ n = 64;	nShift = 6; }
			else			{ n = 32;	nShift = 5; }
		}
	}
	else
	{
		if (n > 16)
		{
			if (n > 64)		{ n = 16;	nShift = 4; }
			else			{ n = 8;	nShift = 3; }
		}
		else
		{
			if (n > 4)		{ n = 4;	nShift = 2; }
			else if (n > 1)	{ n = 2;	nShift = 1; }
			else			{ n = 1;	nShift = 0; }
		}
	}

#else
	ddf0 = sqrtf(a2x * a2x + a2y * a2y);
	ddf0 += ddf0;
	t1 = a2x + a3x + a3x + a3x;
	t1 += t1;
	t1 *= t1;
	t2 = a2y + a3y + a3y + a3y;
	t2 += t2;
	t2 *= t2;
	ddf1 = sqrtf(t1 + t2);
	upperBound = ddf0 > ddf1 ? ddf0: ddf1;
	if ((ScanLineTess->paintModes & VG_STROKE_PATH) && ScanLineTess->strokeLineWidth > FAT_LINE_WIDTH)
	{
		VGfloat width = COORD_TO_FLOAT(ScanLineTess->strokeLineWidth);
		upperBound *= width * width;
	}
	upperBound *= COORD_TO_FLOAT(ScanLineTess->transformScale);
	n = (VGint) ceil(sqrtf(upperBound));
#endif

	if (n == 0 || n > 256)
	{
		n = 256;
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
		nShift = 8;
#endif
	}

	/* Turn on flattening flag. */
	ScanLineTess->isFlattening = VG_TRUE;

	/* Add extra P0 for incoming tangent. */
	point0 = SubPath->lastPoint;
	gcmASSERT(point0);
	/* First add P1/P2/P3 to calculate incoming tangent, which is saved in P0. */
	if (X0 != X1 || Y0 != Y1)
	{
		gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X1, Y1, SubPath));
	}
	else if (X0 != X2 || Y0 != Y2)
	{
		gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X2, Y2, SubPath));
	}
	else
	{
		gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X3, Y3, SubPath));
	}
	point1 = SubPath->lastPoint;
#if !FOR_CONFORMANCE_TEST
	vgSL_CalculatePointDeltaXAndOrientation(point0, point1);
#endif
	/* Change the point1's coordinates back to P0. */
	point1->x = X0;
	point1->y = Y0;
	point0->length = COORD_ZERO;

	if (n > 1)
	{
		/* Step 2: Calculate deltas */
		/*   Df(t) = f(t + d) - f(t)
		 *  DDf(t) = Df(t + d) - Df(t)
		 * DDDf(t) = DDf(t + d) - DDf(t)
		 *    f(0) = a0
		 *   Df(0) = a1 * d + a2 * d^2 + a3 * d^3
		 *  DDf(0) = 2 * a2 * d^2 + 6 * a3 * d^3
		 * DDDf(0) = 6 * a3 * d^3
		 */
#if USE_FIXED_POINT_FOR_COORDINATES && USE_INT64_FOR_BEZIER
		dShift			= nShift;
		dsquareShift	= dShift << 1;
		dcubeShift		= dShift + dsquareShift;
		ddx   = (((_VGint64) a2x) << FRACTION_BITS) >> dsquareShift;
		ddy   = (((_VGint64) a2y) << FRACTION_BITS) >> dsquareShift;
		dddx  = (((_VGint64) a3x) << FRACTION_BITS) >> dcubeShift;
		dddy  = (((_VGint64) a3y) << FRACTION_BITS) >> dcubeShift;
		dx    = ((((_VGint64) a1x) << FRACTION_BITS) >> dShift) + ddx + dddx;
		dy    = ((((_VGint64) a1y) << FRACTION_BITS) >> dShift) + ddy + dddy;
		ddx  += ddx;
		ddy  += ddy;
		dddx *= 6;
		dddy *= 6;
		ddx  += dddx;
		ddy  += dddy;

		dxC   = (vgCOORD) (dx >> FRACTION_BITS);
		dyC   = (vgCOORD) (dy >> FRACTION_BITS);
		ddxC  = (vgCOORD) (ddx >> FRACTION_BITS);
		ddyC  = (vgCOORD) (ddy >> FRACTION_BITS);
		dddxC = (vgCOORD) (dddx >> FRACTION_BITS);
		dddyC = (vgCOORD) (dddy >> FRACTION_BITS);
#else
		d = 1.0f / (VGfloat) n;
		dsquare   = d * d;
		dcube     = dsquare * d;
		ddx  = a2x * dsquare;
		ddy  = a2y * dsquare;
		dddx = a3x * dcube;
		dddy = a3y * dcube;
		dx   = a1x * d + ddx + dddx;
		dy   = a1y * d + ddy + dddy;
		ddx  += ddx;
		ddy  += ddy;
		dddx = 6.0f * dddx;
		dddy = 6.0f * dddy;
		ddx  += dddx;
		ddy  += dddy;

		dxC   = FLOAT_TO_COORD(dx);
		dyC   = FLOAT_TO_COORD(dy);
		ddxC  = FLOAT_TO_COORD(ddx);
		ddyC  = FLOAT_TO_COORD(ddy);
		dddxC = FLOAT_TO_COORD(dddx);
		dddyC = FLOAT_TO_COORD(dddy);
#endif

		/* Step 3: Add points. */
		x	  = X0;
		y	  = Y0;
		for (i = 1; i < n; i++)
		{
			x += dxC; dxC += ddxC; ddxC += dddxC;
			y += dyC; dyC += ddyC; ddyC += dddyC;

			/* Add a point to subpath. */
			gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x, y, SubPath));
		}
	}

	/* Add point 3 separately to avoid cumulative errors. */
	gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X3, Y3, SubPath));

	/* Add extra P3 for outgoing tangent. */
	/* First change P3(point0)'s coordinates to P0/P1/P2. */
	point0 = SubPath->lastPoint;
	if (X3 != X2 || Y3 != Y2)
	{
		point0->x = X2;
		point0->y = Y2;
	}
	else if (X3 != X1 || Y3 != Y1)
	{
		point0->x = X1;
		point0->y = Y1;
	}
	else
	{
		point0->x = X0;
		point0->y = Y0;
	}

	/* Turn off flattening flag. */
	ScanLineTess->isFlattening = VG_FALSE;

	/* Add P3 to calculate outgoing tangent. */
	gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X3, Y3, SubPath));
	point1 = SubPath->lastPoint;
#if !FOR_CONFORMANCE_TEST
	vgSL_CalculatePointDeltaXAndOrientation(point0, point1);
#endif

	/* Change point0's coordinates back to P3. */
	point0->x = X3;
	point0->y = Y3;
	point0->length = COORD_ZERO;

	return gcvSTATUS_OK;
}

#define vgcCOS_TABLE_SIZE	256

#if USE_FIXED_POINT_FOR_COORDINATES
static const vgCOORD vgcCosine[vgcCOS_TABLE_SIZE + 1] = {
	65536,	65534,	65531,	65524,	65516,
	65505,	65491,	65475,	65457,	65436,
	65412,	65386,	65358,	65327,	65294,
	65258,	65220,	65179,	65136,	65091,
	65043,	64992,	64939,	64884,	64826,
	64766,	64703,	64638,	64571,	64501,
	64428,	64353,	64276,	64197,	64115,
	64030,	63943,	63854,	63762,	63668,
	63571,	63473,	63371,	63268,	63162,
	63053,	62942,	62829,	62714,	62596,
	62475,	62353,	62228,	62100,	61971,
	61839,	61705,	61568,	61429,	61288,
	61144,	60998,	60850,	60700,	60547,
	60392,	60235,	60075,	59913,	59749,
	59583,	59414,	59243,	59070,	58895,
	58718,	58538,	58356,	58172,	57986,
	57797,	57606,	57414,	57219,	57022,
	56822,	56621,	56417,	56212,	56004,
	55794,	55582,	55368,	55152,	54933,
	54713,	54491,	54266,	54040,	53811,
	53581,	53348,	53114,	52877,	52639,
	52398,	52155,	51911,	51665,	51416,
	51166,	50914,	50660,	50403,	50146,
	49886,	49624,	49360,	49095,	48828,
	48558,	48288,	48015,	47740,	47464,
	47186,	46906,	46624,	46340,	46055,
	45768,	45480,	45189,	44897,	44603,
	44308,	44011,	43712,	43412,	43110,
	42806,	42501,	42194,	41885,	41575,
	41263,	40950,	40636,	40319,	40002,
	39682,	39362,	39039,	38716,	38390,
	38064,	37736,	37406,	37075,	36743,
	36409,	36074,	35738,	35400,	35061,
	34721,	34379,	34036,	33692,	33346,
	32999,	32651,	32302,	31952,	31600,
	31247,	30893,	30538,	30181,	29824,
	29465,	29105,	28745,	28383,	28020,
	27656,	27291,	26925,	26557,	26189,
	25820,	25450,	25079,	24707,	24334,
	23960,	23586,	23210,	22833,	22456,
	22078,	21699,	21319,	20938,	20557,
	20175,	19792,	19408,	19024,	18638,
	18253,	17866,	17479,	17091,	16702,
	16313,	15923,	15533,	15142,	14751,
	14359,	13966,	13573,	13179,	12785,
	12390,	11995,	11600,	11204,	10807,
	10410,	10013,	 9616,	 9218,	 8819,
	 8421,	 8022,	 7623,	 7223,	 6823,
	 6423,	 6023,	 5622,	 5222,	 4821,
	 4420,	 4018,	 3617,	 3215,	 2814,
	 2412,	 2010,	 1608,	 1206,	  804,
	  402,	    0
};

static const vgCOORD vgcCotangent[vgcCOS_TABLE_SIZE + 1] = {
	COORD_MAX,10680573,5340085,3559833,2669640,
	2135471,1779313,1524876,1334015,1185538,
	1066729,969498,	888449,	819849,	761030,
	710035,	665398,	625996,	590957,	559593,
	531351,	505787,	482534,	461291,	441807,
	423871,	407305,	391956,	377693,	364404,
	351993,	340373,	329471,	319222,	309567,
	300457,	291845,	283691,	275959,	268616,
	261634,	254986,	248647,	242597,	236816,
	231286,	225990,	220913,	216043,	211365,
	206869,	202544,	198380,	194367,	190499,
	186765,	183160,	179677,	176309,	173050,
	169895,	166839,	163877,	161005,	158217,
	155512,	152883,	150329,	147846,	145431,
	143081,	140792,	138564,	136392,	134275,
	132211,	130197,	128232,	126313,	124439,
	122609,	120820,	119071,	117360,	115687,
	114049,	112446,	110877,	109340,	107834,
	106358,	104911,	103492,	102101,	100736,
	 99396,	 98081,	 96790,	 95522,	 94277,
	 93053,	 91851,	 90669,	 89507,	 88365,
	 87241,	 86135,	 85047,	 83976,	 82922,
	 81884,	 80862,	 79855,	 78864,	 77886,
	 76923,	 75974,	 75038,	 74115,	 73205,
	 72307,	 71422,	 70548,	 69685,	 68834,
	 67994,	 67164,	 66345,	 65536,	 64736,
	 63946,	 63166,	 62395,	 61633,	 60879,
	 60134,	 59398,	 58669,	 57949,	 57236,
	 56531,	 55834,	 55143,	 54460,	 53784,
	 53114,	 52451,	 51794,	 51144,	 50500,
	 49862,	 49230,	 48604,	 47984,	 47369,
	 46759,	 46155,	 45556,	 44962,	 44373,
	 43789,	 43210,	 42635,	 42065,	 41500,
	 40939,	 40382,	 39829,	 39280,	 38736,
	 38195,	 37658,	 37125,	 36596,	 36070,
	 35548,	 35029,	 34514,	 34002,	 33493,
	 32988,	 32485,	 31986,	 31489,	 30996,
	 30505,	 30017,	 29532,	 29050,	 28570,
	 28092,	 27618,	 27145,	 26675,	 26208,
	 25743,	 25280,	 24819,	 24360,	 23903,
	 23449,	 22996,	 22545,	 22097,	 21650,
	 21205,	 20761,	 20320,	 19880,	 19441,
	 19005,	 18569,	 18136,	 17704,	 17273,
	 16843,	 16415,	 15989,	 15563,	 15139,
	 14716,	 14294,	 13874,	 13454,	 13035,
	 12618,	 12201,	 11786,	 11371,	 10957,
	 10544,	 10132,	  9721,	  9310,	  8900,
	  8491,	  8083,	  7675,	  7267,	  6861,
	  6454,	  6048,	  5643,	  5238,	  4834,
	  4430,	  4026,	  3622,	  3219,	  2816,
	  2413,	  2011,	  1608,	  1206,	   804,
	   402,	     0
};

static const vgCOORD vgcSecant[vgcCOS_TABLE_SIZE + 1] = {
	65536,	65537,	65540,	65547,	65555,
	65566,	65580,	65596,	65615,	65636,
	65659,	65685,	65714,	65745,	65778,
	65814,	65853,	65894,	65937,	65983,
	66032,	66083,	66137,	66194,	66253,
	66314,	66378,	66445,	66515,	66587,
	66662,	66739,	66819,	66902,	66988,
	67076,	67168,	67261,	67358,	67458,
	67560,	67665,	67774,	67885,	67999,
	68116,	68236,	68358,	68484,	68613,
	68746,	68881,	69019,	69161,	69305,
	69453,	69604,	69759,	69917,	70078,
	70242,	70410,	70582,	70757,	70935,
	71117,	71303,	71492,	71685,	71882,
	72083,	72288,	72496,	72708,	72925,
	73145,	73370,	73598,	73831,	74068,
	74310,	74556,	74806,	75061,	75321,
	75585,	75854,	76127,	76406,	76689,
	76978,	77272,	77570,	77874,	78184,
	78499,	78819,	79145,	79477,	79814,
	80158,	80507,	80862,	81224,	81592,
	81967,	82348,	82736,	83130,	83532,
	83941,	84357,	84780,	85210,	85649,
	86095,	86549,	87011,	87482,	87960,
	88448,	88944,	89450,	89964,	90488,
	91021,	91565,	92118,	92681,	93255,
	93840,	94436,	95042,	95661,	96291,
	96933,	97587,	98254,	98934,	99628,
	100334,	101055,	101790,	102540,	103305,
	104085,	104881,	105693,	106522,	107368,
	108232,	109114,	110015,	110935,	111874,
	112834,	113815,	114818,	115842,	116890,
	117961,	119057,	120177,	121324,	122497,
	123698,	124928,	126187,	127476,	128797,
	130150,	131538,	132960,	134418,	135914,
	137449,	139025,	140642,	142302,	144008,
	145761,	147563,	149415,	151320,	153280,
	155298,	157375,	159515,	161720,	163994,
	166338,	168757,	171253,	173832,	176496,
	179249,	182097,	185044,	188095,	191256,
	194532,	197930,	201456,	205119,	208925,
	212883,	217002,	221292,	225764,	230429,
	235301,	240392,	245717,	251294,	257139,
	263273,	269717,	276495,	283634,	291162,
	299112,	307521,	316428,	325880,	335926,
	346625,	358042,	370251,	383336,	397397,
	412544,	428908,	446642,	465923,	486964,
	510015,	535378,	563417,	594580,	629417,
	668617,	713053,	763846,	822464,	890863,
	971711,1068741,1187348,1335624,1526284,
   1780520,2136476,2670444,3560436,5340487,
  10680774,COORD_MAX
};
#else
static const vgCOORD vgcCosine[vgcCOS_TABLE_SIZE + 1] = {
	1.000000F,	0.999981F,	0.999925F,	0.999831F,	0.999699F,
	0.999529F,	0.999322F,	0.999078F,	0.998795F,	0.998476F,
	0.998118F,	0.997723F,	0.997290F,	0.996820F,	0.996313F,
	0.995767F,	0.995185F,	0.994565F,	0.993907F,	0.993212F,
	0.992480F,	0.991710F,	0.990903F,	0.990058F,	0.989177F,
	0.988258F,	0.987301F,	0.986308F,	0.985278F,	0.984210F,
	0.983105F,	0.981964F,	0.980785F,	0.979570F,	0.978317F,
	0.977028F,	0.975702F,	0.974339F,	0.972940F,	0.971504F,
	0.970031F,	0.968522F,	0.966976F,	0.965394F,	0.963776F,
	0.962121F,	0.960431F,	0.958703F,	0.956940F,	0.955141F,
	0.953306F,	0.951435F,	0.949528F,	0.947586F,	0.945607F,
	0.943593F,	0.941544F,	0.939459F,	0.937339F,	0.935184F,
	0.932993F,	0.930767F,	0.928506F,	0.926210F,	0.923880F,
	0.921514F,	0.919114F,	0.916679F,	0.914210F,	0.911706F,
	0.909168F,	0.906596F,	0.903989F,	0.901349F,	0.898674F,
	0.895966F,	0.893224F,	0.890449F,	0.887640F,	0.884797F,
	0.881921F,	0.879012F,	0.876070F,	0.873095F,	0.870087F,
	0.867046F,	0.863973F,	0.860867F,	0.857729F,	0.854558F,
	0.851355F,	0.848120F,	0.844854F,	0.841555F,	0.838225F,
	0.834863F,	0.831470F,	0.828045F,	0.824589F,	0.821103F,
	0.817585F,	0.814036F,	0.810457F,	0.806848F,	0.803208F,
	0.799537F,	0.795837F,	0.792107F,	0.788346F,	0.784557F,
	0.780737F,	0.776888F,	0.773010F,	0.769103F,	0.765167F,
	0.761202F,	0.757209F,	0.753187F,	0.749136F,	0.745058F,
	0.740951F,	0.736817F,	0.732654F,	0.728464F,	0.724247F,
	0.720003F,	0.715731F,	0.711432F,	0.707107F,	0.702755F,
	0.698376F,	0.693971F,	0.689541F,	0.685084F,	0.680601F,
	0.676093F,	0.671559F,	0.667000F,	0.662416F,	0.657807F,
	0.653173F,	0.648514F,	0.643832F,	0.639124F,	0.634393F,
	0.629638F,	0.624859F,	0.620057F,	0.615232F,	0.610383F,
	0.605511F,	0.600616F,	0.595699F,	0.590760F,	0.585798F,
	0.580814F,	0.575808F,	0.570781F,	0.565732F,	0.560662F,
	0.555570F,	0.550458F,	0.545325F,	0.540171F,	0.534998F,
	0.529804F,	0.524590F,	0.519356F,	0.514103F,	0.508830F,
	0.503538F,	0.498228F,	0.492898F,	0.487550F,	0.482184F,
	0.476799F,	0.471397F,	0.465976F,	0.460539F,	0.455084F,
	0.449611F,	0.444122F,	0.438616F,	0.433094F,	0.427555F,
	0.422000F,	0.416430F,	0.410843F,	0.405241F,	0.399624F,
	0.393992F,	0.388345F,	0.382683F,	0.377007F,	0.371317F,
	0.365613F,	0.359895F,	0.354164F,	0.348419F,	0.342661F,
	0.336890F,	0.331106F,	0.325310F,	0.319502F,	0.313682F,
	0.307850F,	0.302006F,	0.296151F,	0.290285F,	0.284408F,
	0.278520F,	0.272621F,	0.266713F,	0.260794F,	0.254866F,
	0.248928F,	0.242980F,	0.237024F,	0.231058F,	0.225084F,
	0.219101F,	0.213110F,	0.207111F,	0.201105F,	0.195090F,
	0.189069F,	0.183040F,	0.177004F,	0.170962F,	0.164913F,
	0.158858F,	0.152797F,	0.146730F,	0.140658F,	0.134581F,
	0.128498F,	0.122411F,	0.116319F,	0.110222F,	0.104122F,
	0.098017F,	0.091909F,	0.085797F,	0.079682F,	0.073565F,
	0.067444F,	0.061321F,	0.055195F,	0.049068F,	0.042938F,
	0.036807F,	0.030675F,	0.024541F,	0.018407F,	0.012272F,
	0.006136F,	0.000000F
};

static const vgCOORD vgcCotangent[vgcCOS_TABLE_SIZE + 1] = {
	COORD_MAX,162.972616F, 81.483240F, 54.318751F, 40.735484F,
	32.584705F,27.150171F, 23.267776F, 20.355468F, 18.089884F,
	16.277008F,14.793373F, 13.556669F, 12.509912F, 11.612399F,
	10.834280F,10.153170F,	9.551949F,	9.017302F,	8.538718F,
	8.107786F,	7.717699F,	7.362888F,	7.038750F,	6.741452F,
	6.467773F,	6.214988F,	5.980774F,	5.763142F,	5.560376F,
	5.370990F,	5.193689F,	5.027339F,	4.870946F,	4.723629F,
	4.584612F,	4.453202F,	4.328783F,	4.210802F,	4.098764F,
	3.992224F,	3.890778F,	3.794063F,	3.701749F,	3.613536F,
	3.529149F,	3.448340F,	3.370879F,	3.296558F,	3.225184F,
	3.156580F,	3.090584F,	3.027043F,	2.965820F,	2.906786F,
	2.849820F,	2.794813F,	2.741660F,	2.690266F,	2.640542F,
	2.592403F,	2.545771F,	2.500574F,	2.456743F,	2.414214F,
	2.372926F,	2.332823F,	2.293853F,	2.255964F,	2.219110F,
	2.183246F,	2.148330F,	2.114322F,	2.081186F,	2.048886F,
	2.017387F,	1.986659F,	1.956671F,	1.927394F,	1.898802F,
	1.870868F,	1.843569F,	1.816880F,	1.790780F,	1.765247F,
	1.740261F,	1.715803F,	1.691855F,	1.668399F,	1.645419F,
	1.622897F,	1.600820F,	1.579173F,	1.557940F,	1.537110F,
	1.516670F,	1.496606F,	1.476907F,	1.457562F,	1.438560F,
	1.419891F,	1.401544F,	1.383510F,	1.365780F,	1.348344F,
	1.331194F,	1.314323F,	1.297721F,	1.281382F,	1.265297F,
	1.249460F,	1.233865F,	1.218504F,	1.203370F,	1.188459F,
	1.173763F,	1.159278F,	1.144997F,	1.130916F,	1.117028F,
	1.103330F,	1.089816F,	1.076481F,	1.063322F,	1.050333F,
	1.037510F,	1.024850F,	1.012348F,	1.000000F,	0.987803F,
	0.975753F,	0.963846F,	0.952079F,	0.940449F,	0.928952F,
	0.917586F,	0.906347F,	0.895232F,	0.884239F,	0.873365F,
	0.862606F,	0.851961F,	0.841426F,	0.830999F,	0.820679F,
	0.810462F,	0.800345F,	0.790328F,	0.780408F,	0.770582F,
	0.760848F,	0.751205F,	0.741651F,	0.732183F,	0.722799F,
	0.713499F,	0.704279F,	0.695139F,	0.686077F,	0.677091F,
	0.668179F,	0.659339F,	0.650571F,	0.641873F,	0.633243F,
	0.624680F,	0.616182F,	0.607748F,	0.599377F,	0.591067F,
	0.582817F,	0.574626F,	0.566493F,	0.558416F,	0.550394F,
	0.542426F,	0.534511F,	0.526648F,	0.518835F,	0.511072F,
	0.503358F,	0.495691F,	0.488070F,	0.480495F,	0.472965F,
	0.465478F,	0.458034F,	0.450631F,	0.443270F,	0.435948F,
	0.428665F,	0.421421F,	0.414214F,	0.407043F,	0.399908F,
	0.392808F,	0.385743F,	0.378710F,	0.371710F,	0.364743F,
	0.357806F,	0.350899F,	0.344023F,	0.337175F,	0.330355F,
	0.323563F,	0.316799F,	0.310060F,	0.303347F,	0.296659F,
	0.289995F,	0.283354F,	0.276737F,	0.270143F,	0.263570F,
	0.257018F,	0.250487F,	0.243976F,	0.237484F,	0.231012F,
	0.224558F,	0.218121F,	0.211702F,	0.205299F,	0.198912F,
	0.192541F,	0.186185F,	0.179844F,	0.173516F,	0.167202F,
	0.160901F,	0.154613F,	0.148336F,	0.142071F,	0.135816F,
	0.129572F,	0.123338F,	0.117114F,	0.110898F,	0.104691F,
	0.098491F,	0.092300F,	0.086115F,	0.079937F,	0.073764F,
	0.067598F,	0.061436F,	0.055280F,	0.049127F,	0.042978F,
	0.036832F,	0.030689F,	0.024549F,	0.018410F,	0.012272F,
	0.006136F,	0.000000F
};

static const vgCOORD vgcSecant[vgcCOS_TABLE_SIZE + 1] = {
	1.000000F,	1.000019F,	1.000075F,	1.000169F,	1.000301F,
	1.000471F,	1.000678F,	1.000923F,	1.001206F,	1.001527F,
	1.001885F,	1.002282F,	1.002717F,	1.003190F,	1.003701F,
	1.004251F,	1.004839F,	1.005465F,	1.006130F,	1.006834F,
	1.007577F,	1.008360F,	1.009181F,	1.010042F,	1.010942F,
	1.011882F,	1.012862F,	1.013882F,	1.014942F,	1.016043F,
	1.017185F,	1.018367F,	1.019591F,	1.020856F,	1.022163F,
	1.023512F,	1.024903F,	1.026336F,	1.027813F,	1.029332F,
	1.030895F,	1.032501F,	1.034151F,	1.035846F,	1.037585F,
	1.039370F,	1.041200F,	1.043075F,	1.044997F,	1.046966F,
	1.048981F,	1.051044F,	1.053155F,	1.055314F,	1.057521F,
	1.059778F,	1.062085F,	1.064442F,	1.066850F,	1.069309F,
	1.071820F,	1.074383F,	1.076999F,	1.079668F,	1.082392F,
	1.085171F,	1.088004F,	1.090894F,	1.093841F,	1.096845F,
	1.099907F,	1.103028F,	1.106208F,	1.109448F,	1.112750F,
	1.116113F,	1.119540F,	1.123029F,	1.126583F,	1.130203F,
	1.133888F,	1.137641F,	1.141461F,	1.145351F,	1.149310F,
	1.153341F,	1.157444F,	1.161620F,	1.165870F,	1.170196F,
	1.174598F,	1.179078F,	1.183637F,	1.188276F,	1.192998F,
	1.197801F,	1.202690F,	1.207664F,	1.212725F,	1.217875F,
	1.223115F,	1.228446F,	1.233871F,	1.239392F,	1.245008F,
	1.250723F,	1.256539F,	1.262456F,	1.268478F,	1.274605F,
	1.280841F,	1.287186F,	1.293644F,	1.300215F,	1.306904F,
	1.313711F,	1.320640F,	1.327692F,	1.334870F,	1.342178F,
	1.349617F,	1.357190F,	1.364900F,	1.372751F,	1.380744F,
	1.388884F,	1.397173F,	1.405615F,	1.414214F,	1.422972F,
	1.431893F,	1.440981F,	1.450241F,	1.459676F,	1.469290F,
	1.479087F,	1.489073F,	1.499251F,	1.509626F,	1.520203F,
	1.530988F,	1.541986F,	1.553201F,	1.564641F,	1.576309F,
	1.588214F,	1.600360F,	1.612754F,	1.625404F,	1.638316F,
	1.651498F,	1.664956F,	1.678699F,	1.692736F,	1.707074F,
	1.721722F,	1.736689F,	1.751986F,	1.767622F,	1.783607F,
	1.799952F,	1.816669F,	1.833769F,	1.851264F,	1.869167F,
	1.887492F,	1.906252F,	1.925462F,	1.945136F,	1.965292F,
	1.985946F,	2.007115F,	2.028817F,	2.051071F,	2.073898F,
	2.097319F,	2.121355F,	2.146031F,	2.171370F,	2.197399F,
	2.224143F,	2.251633F,	2.279897F,	2.308969F,	2.338880F,
	2.369667F,	2.401367F,	2.434019F,	2.467665F,	2.502351F,
	2.538122F,	2.575030F,	2.613126F,	2.652468F,	2.693115F,
	2.735133F,	2.778588F,	2.823554F,	2.870110F,	2.918339F,
	2.968329F,	3.020178F,	3.073988F,	3.129871F,	3.187945F,
	3.248339F,	3.311193F,	3.376657F,	3.444894F,	3.516081F,
	3.590410F,	3.668091F,	3.749352F,	3.834442F,	3.923636F,
	4.017232F,	4.115562F,	4.218989F,	4.327916F,	4.442788F,
	4.564100F,	4.692405F,	4.828320F,	4.972536F,	5.125831F,
	5.289084F,	5.463290F,	5.649583F,	5.849257F,	6.063799F,
	6.294924F,	6.544623F,	6.815217F,	7.109431F,	7.430485F,
	7.782216F,	8.169222F,	8.597075F,	9.072582F,	9.604152F,
	10.202297F,10.880332F, 11.655377F, 12.549817F, 13.593501F,
	14.827134F,16.307697F, 18.117503F, 20.380016F, 23.289255F,
	27.168581F,32.600046F, 40.747756F, 54.327955F, 81.489376F,
   162.975684F,COORD_MAX
};
#endif

/* This is not forward differencing algorithm. */
/* A big cosine table is used to reduce calculation. */
gceSTATUS
vgSL_FlattenEllipse(
	vgSCANLINETESS	ScanLineTess,
	VGbyte			PathCommand,
	vgCOORD			Rh,
	vgCOORD			Rv,
	vgCOORD			Rot,
	vgCOORD			X0,
	vgCOORD			Y0,
	vgCOORD			X1,
	vgCOORD			Y1,
	vgCOORD			DX,
	vgCOORD			DY,
	vgSL_SUBPATH	SubPath
	)
{
	gceSTATUS		status = gcvSTATUS_OK;
	vgSL_POINT		point0, point1;
	vgCOORD			cosRot, sinRot;
	vgCOORD			x0, y0, x1, y1;
	vgCOORD			cx, cy;
	vgCOORD			x0p, y0p, x1p, y1p, pcx, pcy;
	vgCOORD			dx, dy, xm, ym;
	vgCOORD			dsq, disc, s, sdx, sdy;
	VGboolean		largeArc, counterClockwise;
	VGboolean		rightCenter;
    vgCOORD			startAngle, endAngle;
	vgCOORD			incomingTangentX, incomingTangentY;
	vgCOORD			outgoingTangentX, outgoingTangentY;
	vgCOORD			upperBound;
	VGint			n, n2, n4, inc;
	VGint			i, j, k;
	vgCOORD			normStartAngle, normEndAngle;
	VGint			startQuadrant, endQuadrant;	/* Real quadrant - 1 */
	VGint			startIdx, endIdx;
	vgSL_VECTOR2	pointArray;
	vgSL_VECTOR2	point, point2, point3, point4;

	/* Initialize parameters. */
	switch (PathCommand)
	{
    case VG_SCCWARC_TO:	largeArc = VG_FALSE;	counterClockwise = VG_TRUE;		rightCenter = VG_TRUE;	break;
    case VG_SCWARC_TO:	largeArc = VG_FALSE;	counterClockwise = VG_FALSE;	rightCenter = VG_FALSE;	break;
    case VG_LCCWARC_TO:	largeArc = VG_TRUE;		counterClockwise = VG_TRUE;		rightCenter = VG_FALSE;	break;
    case VG_LCWARC_TO:	largeArc = VG_TRUE;		counterClockwise = VG_FALSE;	rightCenter = VG_TRUE;	break;
	}

	if (Rot == COORD_ZERO)
	{
		cosRot = COORD_ONE;
		sinRot = COORD_ZERO;
	}
	else
	{
		while (Rot < COORD_ZERO)	Rot += COORD_360;
		while (Rot > COORD_360)		Rot -= COORD_360;
		Rot = COORD_MUL(Rot, FLOAT_TO_COORD(PI / 180.0f));
		cosRot = COORD_COS(Rot);
		sinRot = COORD_SIN(Rot);
	}

	/* For conformance test, shift (X0, Y0) (X1, Y1) to (0, 0) (DX, DY) for accuracy. */
	x0 = COORD_ZERO;
	y0 = COORD_ZERO;
	x1 = DX;
	y1 = DY;

	/* Step 1: Transform (x0, y0) and (x1, y1) into unit space. */
	x0p = COORD_DIV((COORD_MUL( x0, cosRot) + COORD_MUL(y0, sinRot)), Rh);
	y0p = COORD_DIV((COORD_MUL(-x0, sinRot) + COORD_MUL(y0, cosRot)), Rv);
	x1p = COORD_DIV((COORD_MUL( x1, cosRot) + COORD_MUL(y1, sinRot)), Rh);
	y1p = COORD_DIV((COORD_MUL(-x1, sinRot) + COORD_MUL(y1, cosRot)), Rv);

	/* Step 2: Find unit circles. */
	dx = x0p - x1p;
	dy = y0p - y1p;
	xm = COORD_DIV_BY_TWO(x0p + x1p);
	ym = COORD_DIV_BY_TWO(y0p + y1p);
	dsq = COORD_MUL(dx, dx) + COORD_MUL(dy, dy);
	if (dsq == COORD_ZERO)
	{
		/* The ellips is too flat. */
		/* Treat it as one of Rh/Rv is zero. */
		/* Add point 1. */
		gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X1, Y1, SubPath));
		return gcvSTATUS_OK;
	}
	disc = COORD_DIV(COORD_ONE, dsq) - COORD_ONE_FOURTH;
	if (disc < COORD_ZERO)
	{
		/* Points are too far apart. */
		/* Need to scale Rh and Rv to make disc == 0.0f */
		vgCOORD scale = COORD_DIV_BY_TWO(COORD_SQRT(dsq));
		Rh  = COORD_MUL(Rh,  scale);
		Rv  = COORD_MUL(Rv,  scale);
		x0p = COORD_DIV(x0p, scale);
		y0p = COORD_DIV(y0p, scale);
		x1p = COORD_DIV(x1p, scale);
		y1p = COORD_DIV(y1p, scale);
		xm = COORD_DIV_BY_TWO(x0p + x1p);
		ym = COORD_DIV_BY_TWO(y0p + y1p);
		pcx = xm;
		pcy = ym;
	}
	else
	{
		s = COORD_SQRT(disc);
		sdx = COORD_MUL(s, dx);
		sdy = COORD_MUL(s, dy);
		if (rightCenter)
		{
			pcx = xm + sdy;
			pcy = ym - sdx;
		}
		else
		{
			pcx = xm - sdy;
			pcy = ym + sdx;
		}
	}

	/* Step 3: Calculate start and end angles in the unit circle. */
	dx = x0p - pcx;
	dy = y0p - pcy;
	if (dx > COORD_ONE) dx = COORD_ONE;
	else if (dx < - COORD_ONE) dx = - COORD_ONE;
	startAngle = COORD_ACOS(dx);
	if (dy < COORD_ZERO) startAngle = -startAngle;
	if (startAngle < COORD_ZERO) startAngle += FLOAT_TO_COORD(PI + PI);
	/* Calculate incoming tangent. */
	if (counterClockwise)
	{
		vgCOORD tx = COORD_MUL(-dy, Rh);
		vgCOORD ty = COORD_MUL(dx, Rv);
		incomingTangentX = COORD_MUL(tx, cosRot) - COORD_MUL(ty, sinRot);
		incomingTangentY = COORD_MUL(tx, sinRot) + COORD_MUL(ty, cosRot);
	}
	else
	{
		vgCOORD tx = COORD_MUL(dy, Rh);
		vgCOORD ty = COORD_MUL(-dx, Rv);
		incomingTangentX = COORD_MUL(tx, cosRot) - COORD_MUL(ty, sinRot);
		incomingTangentY = COORD_MUL(ty, sinRot) + COORD_MUL(ty, cosRot);
	}

	dx = x1p - pcx;
	dy = y1p - pcy;
	if (dx > COORD_ONE) dx = COORD_ONE;
	else if (dx < - COORD_ONE) dx = - COORD_ONE;
	endAngle = COORD_ACOS(dx);
	if (dy < COORD_ZERO) endAngle = -endAngle;
	if (endAngle < COORD_ZERO) endAngle += FLOAT_TO_COORD(PI + PI);
	if (counterClockwise)
	{
		vgCOORD tx = COORD_MUL(-dy, Rh);
		vgCOORD ty = COORD_MUL(dx, Rv);
		outgoingTangentX = COORD_MUL(tx, cosRot) - COORD_MUL(ty, sinRot);
		outgoingTangentY = COORD_MUL(ty, sinRot) + COORD_MUL(ty, cosRot);
	}
	else
	{
		vgCOORD tx = COORD_MUL(dy, Rh);
		vgCOORD ty = COORD_MUL(-dx, Rv);
		outgoingTangentX = COORD_MUL(tx, cosRot) - COORD_MUL(ty, sinRot);
		outgoingTangentY = COORD_MUL(ty, sinRot) + COORD_MUL(ty, cosRot);
	}

	/* Step 4: Calculate center. */
	/* Transform back to original coordinate space. */
	pcx = COORD_MUL(pcx, Rh);
	pcy = COORD_MUL(pcy, Rv);
	cx = COORD_MUL(pcx, cosRot) - COORD_MUL(pcy, sinRot);
	cy = COORD_MUL(pcx, sinRot) + COORD_MUL(pcy, cosRot);

	/* Shift (cx, cy) back to original space. */
	cx += X0;
	cy += Y0;

	/* Step 5: Calculate N. */
	/* Lefan's method. */
	/* In a quadrant,
	 * N = ceil(sqrt(max(rh, rv) * PI / 2 / epsilon / 8))
	 *
	 * Remark: the epsilon here is about 71% of the epsilon used in bezier curve for better fitting.
	 */
	upperBound = Rh > Rv ? Rh : Rv;
	if (ScanLineTess->paintModes & VG_STROKE_PATH && ScanLineTess->strokeLineWidth > FAT_LINE_WIDTH)
	{
		VGfloat width = COORD_TO_FLOAT(ScanLineTess->strokeLineWidth);
		upperBound *= width * width;
	}
	upperBound *= COORD_TO_FLOAT(ScanLineTess->transformScale);
	if (upperBound < INT_TO_COORD(15000))
	{
		n = COORD_TO_INT(COORD_CEIL(COORD_MUL(FLOAT_TO_COORD(PI_HALF), COORD_SQRT(upperBound))));
		gcmASSERT(n < 256);
	}
	else
	{
		/* Due to overflow problem, set n to 256 directly. */
		/* TODO - can be improve to use float and increase the cosine table. */
		n = 256;
	}
	if (n > 16)
	{
		if (n > 64)
		{
			if (n > 128)	{ n = 256;	inc = 1; }
			else			{ n = 128;	inc = 2; }
		}
		else
		{
			if (n > 32)		{ n = 64;	inc = 4; }
			else			{ n = 32;	inc = 8; }
		}
	}
	else
	{
		if (n > 4)
		{
			if (n > 8)		{ n = 16;	inc = 16; }
			else			{ n = 8;	inc = 32; }
		}
		else
		{
			if (n > 2)		{ n = 4;	inc = 64; }
			else if (n > 1)	{ n = 2;	inc = 128; }
			else			{ n = 1;	inc = 256; }
		}
	}
	n2 = n + n;
	n4 = n2 + n2;

	gcmERR_RETURN(vgSL_AllocateAVector2Array(ScanLineTess, &pointArray, n4));

	/* Setp 6: Determine the quadrants needed. */
	gcmASSERT(startAngle >= 0);
	gcmASSERT(endAngle >= 0);
	normStartAngle = COORD_MUL(startAngle, FLOAT_TO_COORD(2.0f * PI_RCP));
	normEndAngle = COORD_MUL(endAngle, FLOAT_TO_COORD(2.0f * PI_RCP));
	startQuadrant = (VGint) normStartAngle;
	endQuadrant = (VGint) normEndAngle;
	gcmASSERT(normStartAngle < COORD_FOUR);
	gcmASSERT(normEndAngle < COORD_FOUR);


	/* Step 7: Create an array of points for the first quadrant of a non-rotated ellipse. */
	startIdx = 0;
	endIdx = n;
	j = startIdx * inc;
	k = vgcCOS_TABLE_SIZE;
	point = pointArray;
	for (i = startIdx; i <= endIdx; i++, j += inc, k -= inc, point++)
	{
		point->x = COORD_MUL(vgcCosine[j], Rh);
		point->y = COORD_MUL(vgcCosine[k], Rv);
	}

	/* Setp 8: Rotate the ellipse if needed.  And transform points for quadrant 2, 3, 4. */
	if (Rot != COORD_ZERO)
	{
		/* Transform points to quadrant 2. */
		point = pointArray;
		point2 = pointArray + n2;
		for (i = 0; i < n; i++, point++, point2--)
		{
			point2->x = -(point->x);
			point2->y = point->y;
		}
		gcmASSERT(point == point2);

		/* Rotate points in quadrant 1, 2. */
		point = pointArray;
		for (i = 0; i <= n2; i++, point++)
		{
			vgCOORD x = point->x;
			vgCOORD y = point->y;
			point->x = COORD_MUL(x, cosRot) - COORD_MUL(y, sinRot);
			point->y = COORD_MUL(x, sinRot) + COORD_MUL(y, cosRot);
		}

		/* Transform points to quadrant 3, 4. */
		point = pointArray + 1;
		point3 = point + n2;
		for (i = 1; i < n2; i++, point++, point3++)
		{
			point3->x = -(point->x);
			point3->y = -(point->y);
		}
	}
	else
	{
		/* Transform points to quadrant 2, 3, 4. */
		point = pointArray;
		point3 = pointArray + n2;
		point2 = point3 - 1;	/* point2 = pointArray + n2 - 1; */
		point4 = point2 + n2;	/* point4 = pointArray + n4 - 1; */
		point3->x = -(point->x);
		point3->y = -(point->y);
		point++;
		point3++;
		for (i = 1; i < n; i++, point++, point2--, point3++, point4--)
		{
			point4->x = point->x;
			point3->x = point2->x = -(point->x);
			point2->y = point->y;
			point4->y = point3->y = -(point->y);
		}
		point3->x = -(point->x);
		point3->y = -(point->y);
	}

	/* Step 9: Move the center. */
	if (cx != 0.0 || cy != 0.0)
	{
		point = pointArray;
		for (i = 0; i < n4; i++, point++)
		{
			point->x += cx;
			point->y += cy;
		}
	}

	/* Step 10: Add points. */

	/* Turn on flattening flag. */
	ScanLineTess->isFlattening = VG_TRUE;

	/* Add extra P0 for incoming tangent. */
	point0 = SubPath->lastPoint;
	gcmASSERT(point0);
	/* First add point1 to calculate incoming tangent, which is saved in P0. */
	gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess,
							X0 + incomingTangentX, Y0 + incomingTangentY, SubPath));
	point1 = SubPath->lastPoint;
#if !FOR_CONFORMANCE_TEST
	vgSL_CalculatePointDeltaXAndOrientation(point0, point1);
#endif

	/* Change the point1's coordinates back to P0. */
	point1->x = X0;
	point1->y = Y0;
	point0->length = COORD_ZERO;

	if (counterClockwise)
	{
		startIdx = COORD_TO_INT(COORD_FLOOR((normStartAngle + COORD_ROUNDING_EPSILON) * n)) + 1;
		endIdx = COORD_TO_INT(COORD_CEIL((normEndAngle - COORD_ROUNDING_EPSILON) * n));
		if (normEndAngle > normStartAngle)
		{
			point = pointArray + startIdx;
			for (i = startIdx; i < endIdx; i++, point++)
			{
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, point->x, point->y, SubPath));
			}
		}
		else
		{
			point = pointArray + startIdx;
			for (i = startIdx; i < n4; i++, point++)
			{
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, point->x, point->y, SubPath));
			}
			point = pointArray;
			for (i = 0; i < endIdx; i++, point++)
			{
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, point->x, point->y, SubPath));
			}
		}
	}
	else
	{
		startIdx = COORD_TO_INT(COORD_CEIL((normStartAngle - COORD_ROUNDING_EPSILON) * n)) - 1;
		endIdx = COORD_TO_INT(COORD_FLOOR((normEndAngle + COORD_ROUNDING_EPSILON) * n));
		if (normEndAngle < normStartAngle)
		{
			point = pointArray + startIdx;
			for (i = startIdx; i > endIdx; i--, point--)
			{
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, point->x, point->y, SubPath));
			}
		}
		else
		{
			point = pointArray + startIdx;
			for (i = startIdx; i >= 0; i--, point--)
			{
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, point->x, point->y, SubPath));
			}
			point = pointArray + n4 - 1;
			for (i = n4 - 1; i > endIdx; i--, point--)
			{
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, point->x, point->y, SubPath));
			}
		}
	}

	/* Add point 1. */
	gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, X1, Y1, SubPath));

	/* Turn off flattening flag. */
	ScanLineTess->isFlattening = VG_FALSE;

	/* Add extra P1 for outgoing tangent. */
	point0 = SubPath->lastPoint;
	/* First add point1 to calculate outgoing tangent, which is saved in point0. */
	gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess,
							X1 + outgoingTangentX, Y1 + outgoingTangentY, SubPath));
	point1 = SubPath->lastPoint;
#if !FOR_CONFORMANCE_TEST
	vgSL_CalculatePointDeltaXAndOrientation(point0, point1);
#endif

	/* Change point0's coordinates back to P2. */
	point1->x = X1;
	point1->y = Y1;
	point0->length = COORD_ZERO;

	/* Free pointArray. */
	vgSL_FreeAVector2Array(ScanLineTess, pointArray);

	return status;
}

void
vgSL_CalculateQualityScaleAndMatrix(
	vgSCANLINETESS		ScanLineTess,
	_VGPath *			InputPath,
	_VGMatrix3x3 *		UserMatrix
	)
{
#if FOR_CONFORMANCE_TEST
	VGfloat				pathScale, pathBias;
	vgCOORD				scaleX, scaleY;

	ScanLineTess->pathScale = pathScale = VGSL_GETPATHSCALE(InputPath);
	ScanLineTess->pathBias  = pathBias  = VGSL_GETPATHBIAS(InputPath);
	ScanLineTess->scale = FLOAT_TO_COORD(pathScale);
	ScanLineTess->scaleShiftBits = 0;
	ScanLineTess->strokeScale = COORD_ONE;
	ScanLineTess->bias = FLOAT_TO_COORD(pathBias);
#if USE_SCAN_LINE_ON_RI
	{
	_VGMatrix3x3 &mat = *UserMatrix;

	/* Calculate quality scale. */
	scaleX = COORD_ABS(FLOAT_TO_COORD(mat[0][0])) + COORD_ABS(FLOAT_TO_COORD(mat[0][1]));
	scaleY = COORD_ABS(FLOAT_TO_COORD(mat[1][0])) + COORD_ABS(FLOAT_TO_COORD(mat[1][1]));
	ScanLineTess->transformScale = (scaleX > scaleY ? scaleX : scaleY);
	}
#else
	/* Calculate quality scale. */
	scaleX = COORD_ABS(FLOAT_TO_COORD(UserMatrix->m[0][0])) + COORD_ABS(FLOAT_TO_COORD(UserMatrix->m[0][1]));
	scaleY = COORD_ABS(FLOAT_TO_COORD(UserMatrix->m[1][0])) + COORD_ABS(FLOAT_TO_COORD(UserMatrix->m[1][1]));
	ScanLineTess->transformScale = (scaleX > scaleY ? scaleX : scaleY);
#endif

#else
#if !gcdUSE_VG
	VGfloat				pathScale, pathBias;
#endif
	vgCOORD				scale, scaleX, scaleY;
	VGint				shiftBits = 0;
#if USE_QUALITY_SCALE
	vgCOORD				lineWidth = ScanLineTess->strokeLineWidth;
	VGint				qualityScale = ScanLineTess->qualityScale;
#endif

	/* Initialize matrix from InputPath's scale/bias. */
#if gcdUSE_VG
	/* No path scale or bias. */
#else
	ScanLineTess->pathScale = pathScale = VGSL_GETPATHSCALE(InputPath);
	ScanLineTess->pathBias  = pathBias  = VGSL_GETPATHBIAS(InputPath);
#endif

	/* Multiple UserMatrix by matrix to create combined transform matrix. */
#if gcdUSE_VG
	ScanLineTess->matrix[0][0] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 0, 0));
	ScanLineTess->matrix[0][1] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 0, 1));
	ScanLineTess->matrix[0][2] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 0, 2));
	ScanLineTess->matrix[1][0] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 1, 0));
	ScanLineTess->matrix[1][1] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 1, 1));
	ScanLineTess->matrix[1][2] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 1, 2));
	ScanLineTess->matrix[2][0] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 1, 0));
	ScanLineTess->matrix[2][1] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 2, 1));
	ScanLineTess->matrix[2][2] = FLOAT_TO_COORD(vgmMAT(UserMatrix, 2, 2));
#elif USE_SCAN_LINE_ON_RI
	_VGMatrix3x3 &mat = *UserMatrix;

	ScanLineTess->matrix[0][0] = FLOAT_TO_COORD(mat[0][0] * pathScale);
	ScanLineTess->matrix[0][1] = FLOAT_TO_COORD(mat[0][1] * pathScale);
	ScanLineTess->matrix[0][2] = FLOAT_TO_COORD(mat[0][0] * pathBias
											  + mat[0][1] * pathBias
											  + mat[0][2]);
	ScanLineTess->matrix[1][0] = FLOAT_TO_COORD(mat[1][0] * pathScale);
	ScanLineTess->matrix[1][1] = FLOAT_TO_COORD(mat[1][1] * pathScale);
	ScanLineTess->matrix[1][2] = FLOAT_TO_COORD(mat[1][0] * pathBias
											  + mat[1][1] * pathBias
											  + mat[1][2]);
	ScanLineTess->matrix[2][0] = FLOAT_TO_COORD(mat[1][0] * pathScale);
	ScanLineTess->matrix[2][1] = FLOAT_TO_COORD(mat[2][1] * pathScale);
	ScanLineTess->matrix[2][2] = FLOAT_TO_COORD(mat[2][0] * pathBias
											  + mat[2][1] * pathBias
											  + mat[2][2]);
#else
	ScanLineTess->matrix[0][0] = FLOAT_TO_COORD(UserMatrix->m[0][0] * pathScale);
	ScanLineTess->matrix[0][1] = FLOAT_TO_COORD(UserMatrix->m[0][1] * pathScale);
	ScanLineTess->matrix[0][2] = FLOAT_TO_COORD(UserMatrix->m[0][0] * pathBias
											  + UserMatrix->m[0][1] * pathBias);
	ScanLineTess->matrix[1][0] = FLOAT_TO_COORD(UserMatrix->m[1][0] * pathScale);
	ScanLineTess->matrix[1][1] = FLOAT_TO_COORD(UserMatrix->m[1][1] * pathScale);
	ScanLineTess->matrix[1][2] = FLOAT_TO_COORD(UserMatrix->m[1][0] * pathBias
											  + UserMatrix->m[1][1] * pathBias);
	ScanLineTess->matrix[2][0] = FLOAT_TO_COORD(UserMatrix->m[1][0] * pathScale);
	ScanLineTess->matrix[2][1] = FLOAT_TO_COORD(UserMatrix->m[2][1] * pathScale);
	ScanLineTess->matrix[2][2] = FLOAT_TO_COORD(UserMatrix->m[2][0] * pathBias
											  + UserMatrix->m[2][1] * pathBias
											  + UserMatrix->m[2][2]);
#endif

	/* Calculate quality scale. */
	scaleX = COORD_ABS(ScanLineTess->matrix[0][0]) + COORD_ABS(ScanLineTess->matrix[0][1]);
	scaleY = COORD_ABS(ScanLineTess->matrix[1][0]) + COORD_ABS(ScanLineTess->matrix[1][1]);
	scale = (scaleX > scaleY ? scaleX : scaleY);

	/* Adjust scale to be power of 2. */
	if (scale <= COORD_ONE_HALF)
	{
		ScanLineTess->scaleUp = VG_FALSE;
		do
		{
			scale = COORD_MUL_BY_TWO(scale);
			shiftBits++;
		}
		while (scale <= COORD_ONE_HALF);
		if (shiftBits > FRACTION_BITS)
		{
			gcmASSERT(shiftBits <= FRACTION_BITS);
			shiftBits = FRACTION_BITS;
		}
#if USE_FIXED_POINT_FOR_COORDINATES
		scale = COORD_ONE >> shiftBits;
#else
		scale = 1.0f / ((VGfloat) (1 << shiftBits));
#endif
	}
	else if (scale >= COORD_ONE + COORD_ONE_HALF)
	{
		/* Use 1.5 as threshold to avoid scale up 45-degree rotation (1.414). */
		ScanLineTess->scaleUp = VG_TRUE;
		do
		{
			scale = COORD_DIV_BY_TWO(scale);
			shiftBits++;
		}
		while (scale >= COORD_ONE + COORD_ONE_HALF);
		if (shiftBits > FRACTION_BITS)
		{
			gcmASSERT(shiftBits <= FRACTION_BITS);
			shiftBits = FRACTION_BITS;
		}
#if USE_FIXED_POINT_FOR_COORDINATES
		scale = COORD_ONE << shiftBits;
#else
		scale = (VGfloat) (1 << shiftBits);
#endif
	}
	else
	{
		scale = COORD_ONE;
		shiftBits = 0;
	}

	ScanLineTess->scale = scale;
	ScanLineTess->scaleShiftBits = shiftBits;
	// For no scaling
	//ScanLineTess->scale = COORD_ONE;
	//ScanLineTess->scaleShiftBits = 0;
	// For non-power-of-two scaling
	//ScanLineTess->scale = scale;

#if USE_QUALITY_SCALE && ! gcdUSE_VG
	/* Adjust scale according to QualityScale and strokeLineWidth. */
	lineWidth = COORD_DIV(COORD_MUL(lineWidth, scale), FLOAT_TO_COORD(pathScale));

	if (lineWidth < COORD_ONE)
	{
		do
		{
			lineWidth = COORD_MUL_BY_TWO(lineWidth);
			scale = COORD_MUL_BY_TWO(scale);
			//qualityScale++;
			if (ScanLineTess->scaleUp)
			{
				shiftBits++;
			}
			else if (shiftBits > 0)
			{
				shiftBits--;
			}
			else
			{
				ScanLineTess->scaleUp = VG_TRUE;
				shiftBits = 1;
			}
		}
		while (lineWidth < COORD_ONE);

		//ScanLineTess->qualityScale = qualityScale;
	}
#endif

#if !gcdUSE_VG
	if (ScanLineTess->paintModes & VG_STROKE_PATH)
	{
		/* Calculate scale for stroke parameters, and adjust stroke line width. */
		vgCOORD sscale;
#if !FOR_CONFORMANCE_TEST
		gcmASSERT(pathScale >= 0.001f || pathScale <= -0.001f);
		gcmASSERT(pathScale < 65536.0f && pathScale > -65536.0f);
#endif
		ScanLineTess->strokeScale = sscale = COORD_ABS(COORD_DIV(scale, FLOAT_TO_COORD(pathScale)));
		ScanLineTess->strokeLineWidth = COORD_MUL(ScanLineTess->strokeLineWidth, sscale);
	}

	if (pathBias == 0.0f)
	{
		ScanLineTess->bias = COORD_ZERO;
	}
	else
	{
		/* Calculate bias for elliptic arc parameters. */
		vgCOORD sscale = COORD_DIV(scale, FLOAT_TO_COORD(pathScale));
#if !FOR_CONFORMANCE_TEST
		gcmASSERT(pathBias >= 0.001f || pathBias <= -0.001f);
		gcmASSERT(pathBias < 65536.0f && pathBias > -65536.0f);
#endif
		ScanLineTess->bias = COORD_MUL(FLOAT_TO_COORD(pathBias), sscale);
	}
#endif

	/* Adjust matrix according to quality scale. */
	if (ScanLineTess->scale != COORD_ONE)
	{
#if USE_FIXED_POINT_FOR_COORDINATES && USE_SHIFT_FOR_SCALE
		if (ScanLineTess->scaleUp)
		{
			ScanLineTess->matrix[0][0] >>= shiftBits;
			ScanLineTess->matrix[0][1] >>= shiftBits;
			ScanLineTess->matrix[0][2] >>= shiftBits;
			ScanLineTess->matrix[1][0] >>= shiftBits;
			ScanLineTess->matrix[1][1] >>= shiftBits;
			ScanLineTess->matrix[1][2] >>= shiftBits;
		}
		else
		{
			ScanLineTess->matrix[0][0] <<= shiftBits;
			ScanLineTess->matrix[0][1] <<= shiftBits;
			ScanLineTess->matrix[0][2] <<= shiftBits;
			ScanLineTess->matrix[1][0] <<= shiftBits;
			ScanLineTess->matrix[1][1] <<= shiftBits;
			ScanLineTess->matrix[1][2] <<= shiftBits;
		}
#else
		scale = COORD_DIV(COORD_ONE, ScanLineTess->scale);
		ScanLineTess->matrix[0][0] = COORD_MUL(ScanLineTess->matrix[0][0], scale);
		ScanLineTess->matrix[0][1] = COORD_MUL(ScanLineTess->matrix[0][1], scale);
		ScanLineTess->matrix[0][2] = COORD_MUL(ScanLineTess->matrix[0][2], scale);
		ScanLineTess->matrix[1][0] = COORD_MUL(ScanLineTess->matrix[1][0], scale);
		ScanLineTess->matrix[1][1] = COORD_MUL(ScanLineTess->matrix[1][1], scale);
		ScanLineTess->matrix[1][2] = COORD_MUL(ScanLineTess->matrix[1][2], scale);
#endif
	}
#endif
}

#if USE_FIXED_POINT_FOR_COORDINATES && USE_SHIFT_FOR_SCALE
typedef vgCOORD (* vgSL_ValueGetter)(VGbyte * Data, VGint ShiftBits);

vgCOORD vgSL_GetS8 (
	VGbyte *	Data,
	VGint		ShiftBits
	)
{
	VGint		x0 = *Data;
	_VGint64	x = (_VGint64) x0;

	x <<= ShiftBits;

	return (vgCOORD) x;
}

vgCOORD vgSL_GetS16(
	VGbyte *	Data,
	VGint		ShiftBits
	)
{
	VGint		x0 = *((VGshort *) Data);
	_VGint64	x = (_VGint64) x0;

	x <<= ShiftBits;

	return (vgCOORD) x;
}

vgCOORD vgSL_GetS32(
	VGbyte *	Data,
	VGint		ShiftBits
	)
{
	VGint		x0 = *((VGint *) Data);
	_VGint64	x = (_VGint64) x0;

	x <<= ShiftBits;

	if (x > COORD_MAX)
	{
		gcmASSERT(x <= COORD_MAX);
		return COORD_MAX;
	}
	if (x < COORD_MIN)
	{
		gcmASSERT(x >= COORD_MIN);
		return COORD_MIN;
	}

	return (vgCOORD) x;
}

vgCOORD vgSL_GetF(
	VGbyte *	Data,
	VGint		ShiftBits
	)
{
	VGfloat		x0 = *((VGfloat *) Data);
	_VGint64	x = (_VGint64) (x0 * (1 << ShiftBits));

	if (x > COORD_MAX)
	{
		gcmASSERT(x <= COORD_MAX);
		return COORD_MAX;
	}
	if (x < COORD_MIN)
	{
		gcmASSERT(x >= COORD_MIN);
		return COORD_MIN;
	}

	return (vgCOORD) x;
}
#else
typedef vgCOORD (* vgSL_ValueGetter)(VGbyte * Data, vgCOORD PathScale);

vgCOORD vgSL_GetS8 (
	VGbyte *	Data,
	vgCOORD		PathScale
	)
{
	VGint		x0 = *Data;
#if USE_FIXED_POINT_FOR_COORDINATES
	_VGint64	x = (_VGint64) x0;

	x *= PathScale;

	return (vgCOORD) x;
#else
	VGfloat		x = INT_TO_COORD(x0);

	x *= PathScale;

	return (vgCOORD) x;
#endif
}

vgCOORD vgSL_GetS16(
	VGbyte *	Data,
	vgCOORD		PathScale
	)
{
	VGint		x0 = *((VGshort *) Data);
#if USE_FIXED_POINT_FOR_COORDINATES
	_VGint64	x = (_VGint64) x0;

	x *= PathScale;

	return (vgCOORD) x;
#else
	VGfloat		x = INT_TO_COORD(x0);

	x *= PathScale;

	return (vgCOORD) x;
#endif
}

vgCOORD vgSL_GetS32(
	VGbyte *	Data,
	vgCOORD		PathScale
	)
{
	VGint		x0 = *((VGint *) Data);
#if USE_FIXED_POINT_FOR_COORDINATES
	_VGint64	x = (_VGint64) x0;

	x *= PathScale;

	if (x > COORD_MAX)
	{
		gcmASSERT(x <= COORD_MAX);
		return COORD_MAX;
	}
	if (x < COORD_MIN)
	{
		gcmASSERT(x >= COORD_MIN);
		return COORD_MIN;
	}

	return (vgCOORD) x;
#else
	VGfloat		x = INT_TO_COORD(x0);

	x *= PathScale;

	return (vgCOORD) x;
#endif
}

vgCOORD vgSL_GetF(
	VGbyte *	Data,
	vgCOORD		PathScale
	)
{
	VGfloat		x0 = *((VGfloat *) Data);
#if USE_FIXED_POINT_FOR_COORDINATES
	_VGint64	x = (_VGint64) (x0 * (COORD_ONE));

	x *= PathScale;
	x >>= FRACTION_BITS;

	if (x > COORD_MAX)
	{
		gcmASSERT(x <= COORD_MAX);
		return COORD_MAX;
	}
	if (x < COORD_MIN)
	{
		gcmASSERT(x >= COORD_MIN);
		return COORD_MIN;
	}

	return (vgCOORD) x;
#else
	x0 *= PathScale;

	return (vgCOORD) x0;
#endif
}
#endif

gceSTATUS
vgSL_FlattenPath(
	vgSCANLINETESS		ScanLineTess,
	_VGPath *			Path
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_SUBPATH		subPath = gcvNULL;
	vgSL_SUBPATH		prevSubPath = gcvNULL;
	vgCOORD				pathBias;
	vgCOORD				pathScale;
	VGboolean			isRelative;
	VGint				numCommands;            // Number of commands in the path.
	VGbyte				pathCommand;            // Path command of each segment in the VGPath.
	VGbyte				prevCommand;			// Previous command when iterating a path.
	vgSL_ValueGetter	getValue = NULL;		// Funcion pointer to read a value from a general memory address.
	VGint				dataTypeSize;           // Size of the data type.
	VGbyte *			dataPointer = NULL;		// Pointer to the data of the VGPath.
	VGbyte *			alignedPointer = NULL;	// Pointer to the alighed data pointer.
	vgCOORD				sx, sy;					// Starting point of the current subpath.
	vgCOORD				ox, oy;					// Last point of the previous segment.
	vgCOORD				px, py;					// Last internal control point of the previous segment.
	vgCOORD				x0, y0, x1, y1, x2, y2;	// Coordinates of the command.
#if !gcdUSE_VG
	vgCOORD				rh, rv, rot;			// The ellipse parameters.
#endif
	VGint				i;
#if USE_FIXED_POINT_FOR_COORDINATES && USE_SHIFT_FOR_SCALE
	VGint				shiftBits;
#endif

#if gcdUSE_VG
	numCommands = Path->size;
	dataPointer = (VGbyte *) (Path->logical);
#else
	numCommands = VGSL_GETPATHSEGMENTSSIZE(Path);
	dataPointer = (VGbyte *) VGSL_GETPATHDATAPOINTER(Path);
#endif

	if (numCommands == 0) return gcvSTATUS_INVALID_DATA;

	/* Select the data picker. */
    switch (VGSL_GETPATHDATATYPE(Path))
	{
	case vgvSL_PATH_DATATYPE_S_8:
		getValue = vgSL_GetS8;
		dataTypeSize = 1;
		break;

	case vgvSL_PATH_DATATYPE_S_16:
		getValue = vgSL_GetS16;
		dataTypeSize = 2;
		break;

	case vgvSL_PATH_DATATYPE_S_32:
		getValue = vgSL_GetS32;
		dataTypeSize = 4;
		break;

	case vgvSL_PATH_DATATYPE_F:
		getValue = vgSL_GetF;
		dataTypeSize = 4;
		break;

	default:
		gcmASSERT(0);
		return gcvSTATUS_INVALID_ARGUMENT;
		break;
	}

	/* Initialize path bias and scale. */
	pathBias        = ScanLineTess->bias;
	pathScale       = ScanLineTess->scale;

	sx = sy = ox = oy = px = py = COORD_ZERO;

	/* Add a subpath. */
	gcmERR_RETURN(vgSL_AddSubPath(ScanLineTess, &subPath));

#if gcdUSE_VG
	if ((*Path->logical & 0x1e) != vgvSL_MOVE_TO)
#else
	if ((ARRAY_ITEM(VGSL_GETPATHSEGMENTS(Path), 0) & 0x1e) != vgvSL_MOVE_TO)
#endif
	{
	    /* Add an extra VG_MOVE_TO 0.0 0.0 to handle the case the first command is not VG_MOVE_TO. */
		/* Add first point to subpath. */
		gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, sx, sy, subPath));
	}

#if gcdUSE_VG
#define VGSL_GETALIGN(P, ALIGN) \
	alignedPointer = (VGbyte *)(((VGuint)(P) + ((ALIGN) - 1)) & ~((ALIGN) - 1)); \
	if (alignedPointer != P) { i += alignedPointer - P; P = alignedPointer; }
#endif

#if USE_FIXED_POINT_FOR_COORDINATES && USE_SHIFT_FOR_SCALE
	gcmASSERT(ScanLineTess->scaleShiftBits <= FRACTION_BITS);
	if (ScanLineTess->scaleUp)
	{
		shiftBits = FRACTION_BITS + ScanLineTess->scaleShiftBits;
	}
	else
	{
		shiftBits = FRACTION_BITS - ScanLineTess->scaleShiftBits;
	}

#if gcdUSE_VG
#define VGSL_GETVALUE(X) \
	VGSL_GETALIGN(dataPointer, dataTypeSize); \
	X = getValue(dataPointer, shiftBits); dataPointer += dataTypeSize; i++;
#else
#define VGSL_GETVALUE(X) \
	X = getValue(dataPointer, shiftBits); dataPointer += dataTypeSize;
#endif
#else
#if gcdUSE_VG
#define VGSL_GETVALUE(X) \
	VGSL_GETALIGN(dataPointer, dataTypeSize); \
	X = getValue(dataPointer, pathScale); dataPointer += dataTypeSize; i++;
#else
#define VGSL_GETVALUE(X) \
	X = getValue(dataPointer, pathScale) + pathBias; dataPointer += dataTypeSize;
#endif
#endif

#define VGSL_GETCOORD(X, OX) \
	VGSL_GETVALUE(X); if (isRelative) X += OX;
#define VGSL_GETCOORDX(X) \
	VGSL_GETVALUE(X); if (isRelative) X += ox;
#define VGSL_GETCOORDY(Y) \
	VGSL_GETVALUE(Y); if (isRelative) Y += oy;

	prevCommand = VG_MOVE_TO;
    for (i = 0; i < numCommands; i++)
    {
#if gcdUSE_VG
		pathCommand = *dataPointer;
		isRelative  = (VGboolean) (pathCommand & 0x01);
		if (pathCommand != gcePATHCOMMAND_CLOSE) pathCommand &= 0x1e;
		dataPointer += dataTypeSize;
#else
		pathCommand = (ARRAY_ITEM(VGSL_GETPATHSEGMENTS(Path), i) & 0x1e);
		isRelative  = (VGboolean) (ARRAY_ITEM(VGSL_GETPATHSEGMENTS(Path), i) & 0x01);
#endif

		switch (pathCommand)
        {
        case vgvSL_CLOSE_PATH:
			/* Check if subPath is already closed. */
			if (ox != sx || oy != sy)
			{
				/* Add a line from current point to the first point of current subpath. */
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, sx, sy, subPath));
			}
			{
				/* Copy tangent data from first point to lastpoint. */
				vgSL_POINT firstPoint = subPath->pointList;
				vgSL_POINT lastPoint = subPath->lastPoint;
				lastPoint->length = firstPoint->length;
				lastPoint->angle = firstPoint->angle;
				lastPoint->tangentX = firstPoint->tangentX;
				lastPoint->tangentY = firstPoint->tangentY;
			}
			subPath->closed = VG_TRUE;

			/* Add a subpath. */
			prevSubPath = subPath;
			gcmERR_RETURN(vgSL_AddSubPath(ScanLineTess, &subPath));

			/* Add first point to subpath. */
			gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, sx, sy, subPath));
			px = ox = sx;
			py = oy = sy;
			break;

        case vgvSL_MOVE_TO:        /* Indicate the beginning of a new sub-path. */
			VGSL_GETCOORDX(x0);
			VGSL_GETCOORDY(y0);
			if (prevCommand != VG_MOVE_TO && prevCommand != VG_CLOSE_PATH)
			{
				/* Close the current subpath, and add a subpath. */
				prevSubPath = subPath;
				gcmERR_RETURN(vgSL_AddSubPath(ScanLineTess, &subPath));

				/* Add first point to subpath. */
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x0, y0, subPath));
			}
			else if (subPath->pointCount == 0)
			{
				/* First command is VG_MOVE_TO. */
				/* Add first point to subpath. */
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x0, y0, subPath));
			}
			else
			{
				/* Continuous VG_MOVE_TO. */
				/* Replace the previous move-to point to current move-to point. */
				gcmASSERT(subPath != gcvNULL);
				gcmASSERT(subPath->pointCount == 1);
				subPath->pointList->x = x0;
				subPath->pointList->y = y0;
			}
			sx = px = ox = x0;
			sy = py = oy = y0;
			break;

        case vgvSL_LINE_TO:
			VGSL_GETCOORDX(x0);
			VGSL_GETCOORDY(y0);
			/* Add a point to subpath. */
			gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x0, y0, subPath));
			px = ox = x0;
			py = oy = y0;
			break;

#if !gcdUSE_VG
        case vgvSL_HLINE_TO:
			VGSL_GETCOORDX(x0);
			y0 = oy;
			/* Add a point to subpath. */
			gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x0, y0, subPath));
			px = ox = x0;
			py = oy;
			break;

        case vgvSL_VLINE_TO:
			x0 = ox;
			VGSL_GETCOORDY(y0);
			/* Add a point to subpath. */
			gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x0, y0, subPath));
			px = ox;
			py = oy = y0;
			break;

        case vgvSL_SQUAD_TO:
			x0 = 2 * ox - px;
			y0 = 2 * oy - py;
			goto common_quad_to;
#endif

        case vgvSL_QUAD_TO:
			VGSL_GETCOORDX(x0);
			VGSL_GETCOORDY(y0);
#if !gcdUSE_VG
		  common_quad_to:
#endif
			VGSL_GETCOORDX(x1);
			VGSL_GETCOORDY(y1);

			if ((ox == x0 && oy == y0) && (ox == x1 && oy == y1))
			{
				/* Degenerated Bezier curve.  Becomes a point. */
				/* Discard zero-length segments. */
			}
			else if ((ox == x0 && oy == y0) || (x0 == x1 && y0 == y1))
			{
				/* Degenerated Bezier curve.  Becomes a line. */
				/* Add a point to subpath. */
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x1, y1, subPath));
			}
			else
			{
				gcmERR_RETURN(vgSL_FlattenQuadBezier(ScanLineTess, ox, oy, x0, y0, x1, y1, subPath));
			}

			px = x0;
			py = y0;
			ox = x1;
			oy = y1;
			break;

#if !gcdUSE_VG
        case vgvSL_SCUBIC_TO:
			x0 = 2 * ox - px;
			y0 = 2 * oy - py;
			goto common_cubic_to;
#endif

        case vgvSL_CUBIC_TO:
			VGSL_GETCOORDX(x0);
			VGSL_GETCOORDY(y0);
#if !gcdUSE_VG
		  common_cubic_to:
#endif
			VGSL_GETCOORDX(x1);
			VGSL_GETCOORDY(y1);
			VGSL_GETCOORDX(x2);
			VGSL_GETCOORDY(y2);
			if ((ox == x0 && oy == y0) && (ox == x1 && oy == y1) && (ox == x2 && oy == y2))
			{
				/* Degenerated Bezier curve.  Becomes a point. */
				/* Discard zero-length segments. */
			}
			else
			{
				gcmERR_RETURN(vgSL_FlattenCubicBezier(ScanLineTess,
											ox, oy,
											x0, y0,
											x1, y1,
											x2, y2,
											subPath));
			}
			px = x1;
			py = y1;
			ox = x2;
			oy = y2;
			break;

#if !gcdUSE_VG
        case vgvSL_SCCWARC_TO:
        case vgvSL_SCWARC_TO:
        case vgvSL_LCCWARC_TO:
        case vgvSL_LCWARC_TO:
			VGSL_GETVALUE(rh);
			VGSL_GETVALUE(rv);
			VGSL_GETVALUE(rot);

			/* This is special hack to keep accuracy for conformance test. */
			//VGSL_GETCOORDX(x0);
			//VGSL_GETCOORDY(y0);
			/* (x1, y1) is the difference between P0 and P1. */
			VGSL_GETVALUE(x1);
			VGSL_GETVALUE(y1);
			if (isRelative)
			{
				x0 = x1 + ox;
				y0 = y1 + oy;
			}
			else
			{
				x0 = x1;
				y0 = y1;
				x1 -= ox;
				y1 -= oy;
			}

#if !FOR_CONFORMANCE_TEST
			if (ScanLineTess->bias != COORD_ZERO)
			{
				/* Adjust rh, rv, and rot for pathBias. */
				rh  += ScanLineTess->bias;
				rv  += ScanLineTess->bias;
				rot += ScanLineTess->bias;
			}
#endif
			if (x0 == ox && y0 == oy)
			{
				/* Degenerated ellipic cureve.  Becomes a point. */
			}
			else if (rh == COORD_ZERO || rv == COORD_ZERO)
			{
				/* Degenerated elliptic curve.  Becomes a line. */
				/* Add a point to subpath. */
				gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, x0, y0, subPath));
			}
			else
			{
				if (rh < COORD_ZERO) rh = -rh;
				if (rv < COORD_ZERO) rv = -rv;
				gcmERR_RETURN(vgSL_FlattenEllipse(ScanLineTess, pathCommand,
												rh, rv, rot, ox, oy, x0, y0, x1, y1, subPath));
			}
			px = ox = x0;
			py = oy = y0;
			break;
#endif

        default:
			gcmASSERT(gcvFALSE);
			return gcvSTATUS_INVALID_DATA;
        }
		prevCommand = pathCommand;
    }

	if (prevCommand == vgvSL_CLOSE_PATH || prevCommand == vgvSL_MOVE_TO)
	{
		/* Remove the extra subpath. */
		gcmASSERT(subPath->pointCount == 1);
		vgSL_FreeAPoint(ScanLineTess, subPath->pointList);
		vgSL_FreeASubPath(ScanLineTess, subPath);
		prevSubPath->next = gcvNULL;
		ScanLineTess->lastSubPath = prevSubPath;
	}

	return status;
}

#if !gcdUSE_VG
// coefficients for error estimation
// while using cubic Bézier curves for approximation
// 0 < b/a < 1/4
static VGfloat coeffs3Low[2][4][4] = {
	{
		{  3.85268F,   -21.229F,      -0.330434F,    0.0127842F  },
		{ -1.61486F,     0.706564F,    0.225945F,    0.263682F   },
		{ -0.910164F,    0.388383F,    0.00551445F,  0.00671814F },
		{ -0.630184F,    0.192402F,    0.0098871F,   0.0102527F  }
	}, {
		{ -0.162211F,    9.94329F,     0.13723F,     0.0124084F  },
		{ -0.253135F,    0.00187735F,  0.0230286F,   0.01264F    },
		{ -0.0695069F,  -0.0437594F,   0.0120636F,   0.0163087F  },
		{ -0.0328856F,  -0.00926032F, -0.00173573F,  0.00527385F }
	}
};

// coefficients for error estimation
// while using cubic Bézier curves for approximation
// 1/4 <= b/a <= 1
static VGfloat coeffs3High[2][4][4] = {
    {
      {  0.0899116F, -19.2349F,     -4.11711F,     0.183362F   },
      {  0.138148F,   -1.45804F,     1.32044F,     1.38474F    },
      {  0.230903F,   -0.450262F,    0.219963F,    0.414038F   },
      {  0.0590565F,  -0.101062F,    0.0430592F,   0.0204699F  }
    }, {
      {  0.0164649F,   9.89394F,     0.0919496F,   0.00760802F },
      {  0.0191603F,  -0.0322058F,   0.0134667F,  -0.0825018F  },
      {  0.0156192F,  -0.017535F,    0.00326508F, -0.228157F   },
      { -0.0236752F,   0.0405821F,  -0.0173086F,   0.176187F   }
    }
};

// safety factor to convert the "best" error approximation
// into a "max bound" error
static VGfloat safety3[4] = {
	0.001F, 4.98F, 0.207F, 0.0067F
};

VGfloat
vgSL_RationalFunction(
	VGfloat				X,
	VGfloat				C[]
	)
{
	return (X * (X * C[0] + C[1]) + C[2]) / (X + C[3]);
}

VGfloat
vgSL_EstimateError(
	VGfloat				A,
	VGfloat				B,
	VGfloat				EtaA,
	VGfloat				EtaB
	)
{
	VGfloat				eta  = 0.5f * (EtaA + EtaB);
	VGfloat				x    = B / A;
	VGfloat				dEta = EtaB - EtaA;
	VGfloat				cos2 = cosf(2 * eta);
	VGfloat				cos4 = cosf(4 * eta);
	VGfloat				cos6 = cosf(6 * eta);
	VGfloat				c0, c1;

	if (x < 0.25)
	{
		c0 = vgSL_RationalFunction(x, coeffs3Low[0][0])
			+ cos2 * vgSL_RationalFunction(x, coeffs3Low[0][1])
			+ cos4 * vgSL_RationalFunction(x, coeffs3Low[0][2])
			+ cos6 * vgSL_RationalFunction(x, coeffs3Low[0][3]);
		c1 = vgSL_RationalFunction(x, coeffs3Low[1][0])
			+ cos2 * vgSL_RationalFunction(x, coeffs3Low[1][1])
			+ cos4 * vgSL_RationalFunction(x, coeffs3Low[1][2])
			+ cos6 * vgSL_RationalFunction(x, coeffs3Low[1][3]);
	}
	else
	{
		c0 = vgSL_RationalFunction(x, coeffs3High[0][0])
			+ cos2 * vgSL_RationalFunction(x, coeffs3High[0][1])
			+ cos4 * vgSL_RationalFunction(x, coeffs3High[0][2])
			+ cos6 * vgSL_RationalFunction(x, coeffs3High[0][3]);
		c1 = vgSL_RationalFunction(x, coeffs3High[1][0])
			+ cos2 * vgSL_RationalFunction(x, coeffs3High[1][1])
			+ cos4 * vgSL_RationalFunction(x, coeffs3High[1][2])
			+ cos6 * vgSL_RationalFunction(x, coeffs3High[1][3]);
	}

	return vgSL_RationalFunction(x, safety3) * A * expf(c0 + c1 * dEta);
}

VGint
vgSL_EstimateNumberOfBezierCurves(
	VGfloat				A,
	VGfloat				B,
	VGfloat				Eta1,
	VGfloat				Eta2,
	VGfloat				Threshold
	)
{
	/* Find the number of Bézier curves needed. */
	VGboolean			found = VG_FALSE;
	VGint				i;
	VGint				n = 1;
	VGfloat				dEta = Eta2 -Eta1;

	if (dEta < 0.0f)	dEta = -dEta;

	while ((! found) && (n < 1024)) {
		if (dEta <= 0.5f * PI) {
			VGfloat etaB = Eta1;
			found = VG_TRUE;
			for (i = 0; found && (i < n); ++i) {
				VGfloat etaA = etaB;
				VGfloat error;
				etaB += dEta;
				error = vgSL_EstimateError(A, B, etaA, etaB);
				found = (error <= Threshold ? VG_TRUE : VG_FALSE);
			}
		}
		dEta *= 0.5f;
		n = n << 1;
	}

	return n;
}

gceSTATUS
vgSL_FitEllipticArcWithBezierCurves(
	vgSCANLINETESS		ScanLineTess,
	vgSL_SUBPATH		SubPath,
	VGfloat				Cx,
	VGfloat				Cy,
	VGfloat				A,
	VGfloat				B,
	VGfloat				CosTheta,
	VGfloat				SinTheta,
	VGfloat				Eta1,
	VGfloat				Eta2,
	VGfloat				Threshold,
	VGint *				N
    )
{
	gceSTATUS			status = gcvSTATUS_OK;
	VGint				n = vgSL_EstimateNumberOfBezierCurves(A, B, Eta1, Eta2, Threshold);
	VGfloat				dEta = (Eta2 - Eta1) / n;
	VGfloat				etaB = Eta1;
	VGfloat				cosEtaB  = cosf(etaB);
	VGfloat				sinEtaB  = sinf(etaB);
	VGfloat				aCosEtaB = A * cosEtaB;
	VGfloat				bSinEtaB = B * sinEtaB;
	VGfloat				aSinEtaB = A * sinEtaB;
	VGfloat				bCosEtaB = B * cosEtaB;
	VGfloat				xB       = Cx + aCosEtaB * CosTheta - bSinEtaB * SinTheta;
	VGfloat				yB       = Cy + aCosEtaB * SinTheta + bSinEtaB * CosTheta;
	VGfloat				xBDot    = -aSinEtaB * CosTheta - bCosEtaB * SinTheta;
	VGfloat				yBDot    = -aSinEtaB * SinTheta + bCosEtaB * CosTheta;

	VGfloat				t     = tanf(0.5f * dEta);
	VGfloat				alpha = sinf(dEta) * ((VGfloat)sqrt(4.0 + 3.0 * t * t) - 1.0f) / 3.0f;
	VGint				i;

	for (i = 0; i < n; ++i) {
		VGfloat etaA  = etaB;
		VGfloat xA    = xB;
		VGfloat yA    = yB;
		VGfloat xADot = xBDot;
		VGfloat yADot = yBDot;
		VGfloat x, y;

		etaB    += dEta;
		cosEtaB  = cosf(etaB);
		sinEtaB  = sinf(etaB);
		aCosEtaB = A * cosEtaB;
		bSinEtaB = B * sinEtaB;
		aSinEtaB = A * sinEtaB;
		bCosEtaB = B * cosEtaB;
		xB       = Cx + aCosEtaB * CosTheta - bSinEtaB * SinTheta;
		yB       = Cy + aCosEtaB * SinTheta + bSinEtaB * CosTheta;
		xBDot    = -aSinEtaB * CosTheta - bCosEtaB * SinTheta;
		yBDot    = -aSinEtaB * SinTheta + bCosEtaB * CosTheta;

		x = xA + alpha * xADot;
		y = yA + alpha * yADot;
		gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, *((vgCOORD *) &x), *((vgCOORD *) &y), SubPath));
		x = xB - alpha * xBDot;
		y = yB - alpha * yBDot;
		gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, *((vgCOORD *) &x), *((vgCOORD *) &y), SubPath));
		gcmERR_RETURN(vgSL_AddPointToSubPath(ScanLineTess, *((vgCOORD *) &xB), *((vgCOORD *) &yB), SubPath));
	}

	*N = n;
	return status;
}

gceSTATUS
vgSL_SimplifyEllipticArc(
	vgSCANLINETESS		ScanLineTess,
	VGbyte				PathCommand,
	VGfloat				Rh,
	VGfloat				Rv,
	VGfloat				Rot,
	VGfloat				X0,
	VGfloat				Y0,
	VGfloat				X1,
	VGfloat				Y1,
	vgCOORD				DX,
	vgCOORD				DY,
	VGint *				N
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_SUBPATH		subPath = gcvNULL;		// Place holder for Bézier curve points.
	VGfloat				cosRot, sinRot;
	VGfloat				x0, y0, x1, y1;
	VGfloat				cx, cy;
	VGfloat				x0p, y0p, x1p, y1p, pcx, pcy;
	VGfloat				dx, dy, xm, ym;
	VGfloat				dsq, disc, s, sdx, sdy;
	VGboolean			largeArc, counterClockwise;
	VGboolean			rightCenter;
    VGfloat				startAngle, endAngle;
	VGfloat				threshold = DIFF_EPSILON / 16.0f;
	VGint				n;

	/* Initialize parameters. */
	switch (PathCommand)
	{
    case VG_SCCWARC_TO:	largeArc = VG_FALSE;	counterClockwise = VG_TRUE;		rightCenter = VG_TRUE;	break;
    case VG_SCWARC_TO:	largeArc = VG_FALSE;	counterClockwise = VG_FALSE;	rightCenter = VG_FALSE;	break;
    case VG_LCCWARC_TO:	largeArc = VG_TRUE;		counterClockwise = VG_TRUE;		rightCenter = VG_FALSE;	break;
    case VG_LCWARC_TO:	largeArc = VG_TRUE;		counterClockwise = VG_FALSE;	rightCenter = VG_TRUE;	break;
	}

	/* Make sure Rh >= Rv. */
	if (Rv > Rh)
	{
		VGfloat temp = Rv;

		Rv = Rh;
		Rh = temp;
		Rot += 90.0f;
	}

	/* Make sure 0 <= Rot < 180.0f. */
	while (Rot < 0.0f)		Rot += 360.0f;
	while (Rot > 360.0f)	Rot -= 360.0f;
	if (Rot > 180.0f)		Rot -= 180.0f;
	if (Rot == 0.0f)
	{
		cosRot = 1.0f;
		sinRot = 0.0f;
	}
	else if (Rot == 90.0f)
	{
		cosRot = 0.0f;
		sinRot = 1.0f;
	}
	else
	{
		Rot *= PI / 180.0f;
		cosRot = cosf(Rot);
		sinRot = sinf(Rot);
	}

	/* For conformance test, shift (X0, Y0) (X1, Y1) to (0, 0) (DX, DY) for accuracy. */
	x0 = 0.0f;
	y0 = 0.0f;
	x1 = DX;
	y1 = DY;

	/* Step 1: Transform (X0, Y0) and (X1, Y1) into unit space. */
	x0p = ( x0 * cosRot + y0 * sinRot) / Rh;
	y0p = (-x0 * sinRot + y0 * cosRot) / Rv;
	x1p = ( x1 * cosRot + y1 * sinRot) / Rh;
	y1p = (-x1 * sinRot + y1 * cosRot) / Rv;

	/* Step 2: Find unit circles. */
	dx = x0p - x1p;
	dy = y0p - y1p;
	xm = 0.5f * (x0p + x1p);
	ym = 0.5f * (y0p + y1p);
	dsq = (dx * dx) + (dy * dy);
	if (dsq == 0.0f)
	{
		/* The ellips is too flat. */
		/* Treat it as one of Rh/Rv is zero. */
		/* Make n -1 for specail handling. */
		*N = -1;
		return gcvSTATUS_OK;
	}
	disc = (1.0f / dsq) - 0.25f;
	if (disc < 0.0f)
	{
		/* Points are too far apart. */
		/* Need to scale Rh and Rv to make disc == 0.0f */
		VGfloat scale = 0.5f * ((VGfloat) sqrt((double) dsq));
		Rh  = Rh *  scale;
		Rv  = Rv *  scale;
		x0p = x0p / scale;
		y0p = y0p / scale;
		x1p = x1p / scale;
		y1p = y1p / scale;
		xm = 0.5f * (x0p + x1p);
		ym = 0.5f * (y0p + y1p);
		pcx = xm;
		pcy = ym;
	}
	else
	{
		s = (VGfloat) sqrt((double) disc);
		sdx = s * dx;
		sdy = s * dy;
		if (rightCenter)
		{
			pcx = xm + sdy;
			pcy = ym - sdx;
		}
		else
		{
			pcx = xm - sdy;
			pcy = ym + sdx;
		}
	}

	/* Step 3: Calculate start and end angles in the unit circle. */
	dx = x0p - pcx;
	dy = y0p - pcy;
	if (dx > 1.0f) dx = 1.0f;
	else if (dx < - 1.0f) dx = - 1.0f;
	startAngle = acosf(dx);
	if (dy < 0.0f) startAngle = -startAngle;
	if (startAngle < 0.0f) startAngle += PI + PI;

	dx = x1p - pcx;
	dy = y1p - pcy;
	if (dx > 1.0f) dx = 1.0f;
	else if (dx < - 1.0f) dx = - 1.0f;
	endAngle = acosf(dx);
	if (dy < 0.0f) endAngle = -endAngle;
	if (endAngle < 0.0f) endAngle += PI + PI;

	/* Step 4: Calculate center. */
	/* Transform back to original coordinate space. */
	pcx = pcx * Rh;
	pcy = pcy * Rv;
	cx = (pcx * cosRot) - (pcy * sinRot);
	cy = (pcx * sinRot) + (pcy * cosRot);

	/* Shift (cx, cy) back to original space. */
	cx += X0;
	cy += Y0;

	/* Add a subpath. */
	gcmERR_RETURN(vgSL_AddSubPath(ScanLineTess, &subPath));

	/* Fit the arc. */
	*N = 0;
	if (counterClockwise)
	{
		if (startAngle < endAngle)
		{
			gcmERR_RETURN(vgSL_FitEllipticArcWithBezierCurves(
										ScanLineTess, subPath,
										cx, cy,
										Rh, Rv,
										cosRot, sinRot,
										startAngle,
										endAngle,
										threshold, &n));
			*N += n;
		}
		else
		{
			/* Splite the elliptic arc into two arcs. */
			if (startAngle < PI_TWO)
			{
				gcmERR_RETURN(vgSL_FitEllipticArcWithBezierCurves(
										ScanLineTess, subPath,
										cx, cy,
										Rh, Rv,
										cosRot, sinRot,
										startAngle,
										PI_TWO,
										threshold, &n));
				*N += n;
			}
			if (endAngle > 0.0f)
			{
				gcmERR_RETURN(vgSL_FitEllipticArcWithBezierCurves(
										ScanLineTess, subPath,
										cx, cy,
										Rh, Rv,
										cosRot, sinRot,
										0.0f,
										endAngle,
										threshold, &n));
				*N += n;
			}
		}
	}
	else
	{
		if (startAngle > endAngle)
		{
			gcmERR_RETURN(vgSL_FitEllipticArcWithBezierCurves(
										ScanLineTess, subPath,
										cx, cy,
										Rh, Rv,
										cosRot, sinRot,
										startAngle,
										endAngle,
										threshold, &n));
			*N += n;
		}
		else
		{
			/* Splite the elliptic arc into two arcs. */
			if (startAngle > 0.0f)
			{
				gcmERR_RETURN(vgSL_FitEllipticArcWithBezierCurves(
										ScanLineTess, subPath,
										cx, cy,
										Rh, Rv,
										cosRot, sinRot,
										startAngle,
										0.0f,
										threshold, &n));
				*N += n;
			}
			if (endAngle < PI_TWO)
			{
				gcmERR_RETURN(vgSL_FitEllipticArcWithBezierCurves(
										ScanLineTess, subPath,
										cx, cy,
										Rh, Rv,
										cosRot, sinRot,
										PI_TWO,
										endAngle,
										threshold, &n));
				*N += n;
			}
		}
	}

	return status;
}

typedef VGfloat (* vgSL_ValueGetterF)(VGbyte * Data);

VGfloat vgSL_GetS8F(
	VGbyte *	Data
	)
{
	VGint		x = *Data;
	return (VGfloat) x;
}

VGfloat vgSL_GetS16F(
	VGbyte *	Data
	)
{
	VGint		x = *((VGshort *) Data);
	return (VGfloat) x;
}

VGfloat vgSL_GetS32F(
	VGbyte *	Data
	)
{
	VGint		x = *((VGint *) Data);

	return (VGfloat) x;
}

VGfloat vgSL_GetFF(
	VGbyte *	Data
	)
{
	VGfloat		x = *((VGfloat *) Data);
	return x;
}

void vgSL_SetFF(
	VGfloat		Value,
	VGbyte *	Data
	)
{
	VGfloat *	dataP = (VGfloat *) Data;

	*dataP = Value;
}

/* Since this would be done only once per path, use floating point to avoid precision problem. */
gceSTATUS
vgSL_SimplifyPath(
	vgSCANLINETESS		ScanLineTess,
	_VGPath *			InputPath,
	_VGPath *			OutputPath
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_SUBPATH		subPath = gcvNULL;		// Place holder for simplified commands.
	VGfloat				pathBias = VGSL_GETPATHBIAS(InputPath);
	VGfloat				pathScale = VGSL_GETPATHSCALE(InputPath);
	VGboolean			isRelative;
	VGint				numCommands;			// Number of commands in InputPath.
	VGbyte				pathCommand;			// Path command of each segment in InputPath.
	VGbyte				pathCommand0;			// Path command of each segment in InputPath.
	vgSL_ValueGetterF	getValue = NULL;		// Funcion pointer to read a value from a general memory address.
	VGint				dataTypeSize;			// Size of the data type.
	VGbyte				*dataPointer = NULL;	// Pointer to the point data of the InputPath.
	VGint				outDataTypeSize;			// Size of the data type.
	VGbyte				*outSegmentPointer = NULL;	// Pointer to the command segments of the OutputPath.
	VGbyte				*outDataPointer = NULL;	// Pointer to the point data of the OutputPath.
	VGint				outSegmentCount = 0;	// Number of commands in OutputPath.
	VGint				outDataPointCount = 0;	// Number of points in the data of OuputPath.
	VGfloat				sx, sy;					// Starting point of the current subpath.
	VGfloat				ox, oy;					// Last point of the previous segment.
	VGfloat				px, py;					// Last internal control point of the previous segment.
	VGfloat				x0, y0, x1, y1, x2, y2;	// Coordinates of the command.
	VGfloat				rh, rv, rot;			// The ellipse parameters.
	VGint				i;

	numCommands = VGSL_GETPATHSEGMENTSSIZE(InputPath);
	if (numCommands == 0) return gcvSTATUS_INVALID_DATA;

	/* Select the data picker. */
	switch (VGSL_GETPATHDATATYPE(InputPath))
	{
	case vgvSL_PATH_DATATYPE_S_8:
		getValue = vgSL_GetS8F;
		dataTypeSize = 1;
		break;

	case vgvSL_PATH_DATATYPE_S_16:
		getValue = vgSL_GetS16F;
		dataTypeSize = 2;
		break;

	case vgvSL_PATH_DATATYPE_S_32:
		getValue = vgSL_GetS32F;
		dataTypeSize = 4;
		break;

	case vgvSL_PATH_DATATYPE_F:
		getValue = vgSL_GetFF;
		dataTypeSize = 4;
		break;

	default:
		gcmASSERT(0);
		return gcvSTATUS_INVALID_ARGUMENT;
		break;
	}

	/* Do some initial settings. */
	dataPointer		= (VGbyte *) VGSL_GETPATHDATAPOINTER(InputPath);

	/* Set data format and size. */
	gcmASSERT(VGSL_GETPATHSCALE(OutputPath) == 1.0f);
	gcmASSERT(VGSL_GETPATHBIAS(OutputPath) == 0.0f);
	gcmASSERT(VGSL_GETPATHDATATYPE(OutputPath) == vgvSL_PATH_DATATYPE_F);
	outDataTypeSize = 4;

#undef VGSL_GETVALUE
#undef VGSL_GETCOORDX
#undef VGSL_GETCOORDY
#define VGSL_GETVALUE(X) \
	X = getValue(dataPointer) * pathScale + pathBias; dataPointer += dataTypeSize;
#define VGSL_GETCOORDX(X) \
	VGSL_GETVALUE(X); if (isRelative) X += ox;
#define VGSL_GETCOORDY(Y) \
	VGSL_GETVALUE(Y); if (isRelative) Y += oy;
#define VGSL_GETCOORDXY(X, Y) \
	VGSL_GETVALUE(X); VGSL_GETVALUE(Y); if (isRelative) { X += ox; Y += oy; }

	/* Count the segments and data points for OutputPath. */
	sx = sy = ox = oy = px = py = 0.0f;

	for (i = 0; i < numCommands; i++)
	{
		pathCommand0 = ARRAY_ITEM(VGSL_GETPATHSEGMENTS(InputPath), i);
		pathCommand  = (pathCommand0 & 0x1e);
		isRelative   = (VGboolean) (pathCommand0 & 0x01);

		switch (pathCommand)
        {
        case VG_CLOSE_PATH:
			/* No points. */
			outSegmentCount++;

			px = ox = sx;
			py = oy = sy;
			break;

        case VG_MOVE_TO:
			/* One point. */
			VGSL_GETCOORDXY(x0, y0);
			outSegmentCount++;
			outDataPointCount++;

			sx = px = ox = x0;
			sy = py = oy = y0;
			break;

        case VG_LINE_TO:
			/* One point. */
			VGSL_GETCOORDXY(x0, y0);
			outSegmentCount++;
			outDataPointCount++;

			px = ox = x0;
			py = oy = y0;
			break;

        case VG_HLINE_TO:
			/* One point. */
			VGSL_GETCOORDX(x0);
			y0 = oy;
			outSegmentCount++;
			outDataPointCount++;

			px = ox = x0;
			py = oy;
			break;

        case VG_VLINE_TO:
			/* One point. */
			x0 = ox;
			VGSL_GETCOORDY(y0);
			outSegmentCount++;
			outDataPointCount++;

			px = ox;
			py = oy = y0;
			break;

        case VG_SQUAD_TO:
			x0 = 2 * ox - px;
			y0 = 2 * oy - py;
			goto common_quad_to;
        case VG_QUAD_TO:
			/* Two points. */
			VGSL_GETCOORDXY(x0, y0);
		  common_quad_to:
			VGSL_GETCOORDXY(x1, y1);
			outSegmentCount++;
			outDataPointCount += 2;

			px = x0;
			py = y0;
			ox = x1;
			oy = y1;
			break;
        case VG_SCUBIC_TO:
			x0 = 2 * ox - px;
			y0 = 2 * oy - py;
			goto common_cubic_to;
        case VG_CUBIC_TO:
			/* Three points. */
			VGSL_GETCOORDXY(x0, y0);
		  common_cubic_to:
			VGSL_GETCOORDXY(x1, y1);
			VGSL_GETCOORDXY(x2, y2);
			outSegmentCount++;
			outDataPointCount += 3;

			px = x1;
			py = y1;
			ox = x2;
			oy = y2;
			break;

        case VG_SCCWARC_TO:
        case VG_LCCWARC_TO:
        case VG_SCWARC_TO:
        case VG_LCWARC_TO:
			VGSL_GETVALUE(rh);
			VGSL_GETVALUE(rv);
			VGSL_GETVALUE(rot);

			/* This is special hack to keep accuracy for conformance test. */
			//VGSL_GETCOORDX(x0);
			//VGSL_GETCOORDY(y0);
			/* (x1, y1) is the difference between P0 and P1. */
			VGSL_GETVALUE(x1);
			VGSL_GETVALUE(y1);
			if (isRelative)
			{
				x0 = x1 + ox;
				y0 = y1 + oy;
			}
			else
			{
				x0 = x1;
				y0 = y1;
				x1 -= ox;
				y1 -= oy;
			}

#if !FOR_CONFORMANCE_TEST
			if (ScanLineTess->bias != COORD_ZERO)
			{
				/* Adjust rh, rv, and rot for pathBias. */
				rh  += ScanLineTess->bias;
				rv  += ScanLineTess->bias;
				rot += ScanLineTess->bias;
			}
#endif

			if (x0 == ox && y0 == oy)
			{
				/* Degenerated ellipic cureve.  Becomes a point. */
			}
			else if (rh == 0.0f || rv == 0.0f)
			{
				/* Degenerated elliptic curve.  Becomes a line. */
				outSegmentCount++;
				outDataPointCount++;
			}
			else
			{
				VGint n;

				if (rh < 0.0f) rh = -rh;
				if (rv < 0.0f) rv = -rv;

				/* Estimate the number of Bezier curves to fix the elliptic arc. */
				gcmERR_RETURN(vgSL_SimplifyEllipticArc(ScanLineTess, pathCommand,
											rh, rv, rot, ox, oy, x0, y0, x1, y1, &n));

				if (n > 0)
				{
					outSegmentCount += n;
					outDataPointCount += n * 3;
				}
				else
				{
					/* Degenerated elliptic curve.  Becomes a line. */
					outSegmentCount++;
					outDataPointCount++;
				}
			}
			px = ox = x0;
			py = oy = y0;
			break;

        default:
			gcmASSERT(gcvFALSE);
			return gcvSTATUS_INVALID_DATA;
        }
    }

	/* Allocate segments and data for OutputPath. */
	ARRAY_RESIZE(VGSL_GETPATHSEGMENTS(OutputPath), outSegmentCount);
	ARRAY_RESIZE(VGSL_GETPATHDATA(OutputPath), outDataTypeSize * (outDataPointCount << 1));

#if !USE_SCAN_LINE_ON_RI
	if (OutputPath->segments.allocated == 0) return gcvSTATUS_OUT_OF_MEMORY;
	if (OutputPath->data.allocated == 0) return gcvSTATUS_OUT_OF_MEMORY;
#endif

    outSegmentPointer	= (VGbyte *) VGSL_GETPATHSEGMENTSPOINTER(OutputPath);
    outDataPointer		= (VGbyte *) VGSL_GETPATHDATAPOINTER(OutputPath);
	dataPointer			= (VGbyte *) VGSL_GETPATHDATAPOINTER(InputPath);

	/* Output the segments and data points to OutputPath. */
	sx = sy = ox = oy = px = py = 0.0f;

#define VGSL_SETVALUE(X) \
	vgSL_SetFF(X, outDataPointer); outDataPointer += outDataTypeSize;

	subPath = ScanLineTess->subPathList;
	for (i = 0; i < numCommands; i++)
	{
		pathCommand0 = ARRAY_ITEM(VGSL_GETPATHSEGMENTS(InputPath), i);
		pathCommand  = (pathCommand0 & 0x1e);
		isRelative   = (VGboolean) (pathCommand0 & 0x01);

		switch (pathCommand)
        {
        case VG_CLOSE_PATH:
			/* No points. */
			*outSegmentPointer = pathCommand0;
			outSegmentPointer++;
			outSegmentCount--;

			px = ox = sx;
			py = oy = sy;
			break;

        case VG_MOVE_TO:
			/* One point. */
			VGSL_GETCOORDXY(x0, y0);

			*outSegmentPointer = VG_MOVE_TO_ABS;
			outSegmentPointer++;
			outSegmentCount--;
			VGSL_SETVALUE(x0);
			VGSL_SETVALUE(y0);
			outDataPointCount--;

			sx = px = ox = x0;
			sy = py = oy = y0;
			break;

        case VG_LINE_TO:
			/* One point. */
			VGSL_GETCOORDXY(x0, y0);

			*outSegmentPointer = VG_LINE_TO_ABS;
			outSegmentPointer++;
			outSegmentCount--;
			VGSL_SETVALUE(x0);
			VGSL_SETVALUE(y0);
			outDataPointCount--;

			px = ox = x0;
			py = oy = y0;
			break;

        case VG_HLINE_TO:
			/* One point. */
			VGSL_GETCOORDX(x0);
			y0 = oy;

			*outSegmentPointer = VG_LINE_TO_ABS;
			outSegmentPointer++;
			outSegmentCount--;
			VGSL_SETVALUE(x0);
			VGSL_SETVALUE(y0);
			outDataPointCount--;

			px = ox = x0;
			py = oy;
			break;

        case VG_VLINE_TO:
			/* One point. */
			x0 = ox;
			VGSL_GETCOORDY(y0);

			*outSegmentPointer = VG_LINE_TO_ABS;
			outSegmentPointer++;
			outSegmentCount--;
			VGSL_SETVALUE(x0);
			VGSL_SETVALUE(y0);
			outDataPointCount--;

			px = ox;
			py = oy = y0;
			break;

        case VG_SQUAD_TO:
			x0 = 2 * ox - px;
			y0 = 2 * oy - py;
			VGSL_SETVALUE(x0);
			VGSL_SETVALUE(y0);
			outDataPointCount--;
			goto common_quad_to1;
        case VG_QUAD_TO:
			/* Two points. */
			VGSL_GETCOORDXY(x0, y0);

			VGSL_SETVALUE(x0);
			VGSL_SETVALUE(y0);
			outDataPointCount--;

		  common_quad_to1:
			VGSL_GETCOORDXY(x1, y1);

			VGSL_SETVALUE(x1);
			VGSL_SETVALUE(y1);
			outDataPointCount--;

			*outSegmentPointer = VG_QUAD_TO_ABS;
			outSegmentPointer++;
			outSegmentCount--;

			px = x0;
			py = y0;
			ox = x1;
			oy = y1;
			break;
        case VG_SCUBIC_TO:
			x0 = 2 * ox - px;
			y0 = 2 * oy - py;
			VGSL_SETVALUE(x0);
			VGSL_SETVALUE(y0);
			outDataPointCount--;
			goto common_cubic_to1;
        case VG_CUBIC_TO:
			/* Three points. */
			VGSL_GETCOORDXY(x0, y0);

			VGSL_SETVALUE(x0);
			VGSL_SETVALUE(y0);
			outDataPointCount--;

		  common_cubic_to1:
			VGSL_GETCOORDXY(x1, y1);
			VGSL_GETCOORDXY(x2, y2);

			VGSL_SETVALUE(x1);
			VGSL_SETVALUE(y1);
			VGSL_SETVALUE(x2);
			VGSL_SETVALUE(y2);
			outDataPointCount -= 2;

			*outSegmentPointer = VG_CUBIC_TO_ABS;
			outSegmentPointer++;
			outSegmentCount--;

			px = x1;
			py = y1;
			ox = x2;
			oy = y2;
			break;

        case VG_SCCWARC_TO:
        case VG_LCCWARC_TO:
        case VG_SCWARC_TO:
        case VG_LCWARC_TO:
			VGSL_GETVALUE(rh);
			VGSL_GETVALUE(rv);
			VGSL_GETVALUE(rot);
			VGSL_GETCOORDXY(x0, y0);

			if (x0 == ox && y0 == oy)
			{
				/* Degenerated ellipic cureve.  Becomes a point. */
			}
			else if (rh == 0.0f || rv == 0.0f)
			{
				/* Degenerated elliptic curve.  Becomes a line. */
				*outSegmentPointer = VG_LINE_TO_ABS;
				outSegmentPointer++;
				outSegmentCount--;

				VGSL_SETVALUE(x0);
				VGSL_SETVALUE(y0);
				outDataPointCount--;
			}
			else
			{
				/* Output Bezier curves to OutputPath. */
				VGint i, j, n;
				vgSL_POINT point;
				VGfloat x, y;

				gcmASSERT(subPath);
				n = subPath->pointCount / 3;
				point = subPath->pointList;
				for (i = 0; i < n; i++)
				{
					*outSegmentPointer = VG_CUBIC_TO_ABS;
					outSegmentPointer++;
					outSegmentCount--;

					for (j = 0; j < 3; j++, point = point->next)
					{
						x = *((VGfloat *) &(point->x));
						y = *((VGfloat *) &(point->y));
						VGSL_SETVALUE(x);
						VGSL_SETVALUE(y);
						outDataPointCount--;
					}

					px = ox = x;
					py = oy = y;
				}
				gcmASSERT(point == gcvNULL);
				subPath = subPath->next;
			}
			px = ox = x0;
			py = oy = y0;
			break;
        }
    }
	gcmASSERT(outSegmentCount == 0);
	gcmASSERT(outDataPointCount == 0);
	gcmASSERT(subPath == gcvNULL);

	return status;
}
#endif

void
vgSL_TransformPath(
	vgSCANLINETESS		ScanLineTess,
	vgSL_SUBPATH		SubPathList,
	VGint				GetRange
	)
{
	vgSL_SUBPATH		subPath;
	vgSL_POINT			point;
	/*
	vgCOORD				minX = COORD_MAX;
	vgCOORD				maxX = COORD_MIN;
	*/
	vgCOORD				minY = COORD_MAX;
	vgCOORD				maxY = COORD_MIN;
	vgCOORD				m00 = ScanLineTess->matrix[0][0];
	vgCOORD				m01 = ScanLineTess->matrix[0][1];
	vgCOORD				m02 = ScanLineTess->matrix[0][2];
	vgCOORD				m10 = ScanLineTess->matrix[1][0];
	vgCOORD				m11 = ScanLineTess->matrix[1][1];
	vgCOORD				m12 = ScanLineTess->matrix[1][2];

	if (GetRange == 0)
	{
		for (subPath = SubPathList; subPath; subPath = subPath->next)
		{
			for (point = subPath->pointList; point; point = point->next)
			{
				vgCOORD x0 = point->x;
				vgCOORD y0 = point->y;
				point->x = COORD_MUL(x0, m00) + COORD_MUL(y0, m01) + m02;
				point->y = COORD_MUL(x0, m10) + COORD_MUL(y0, m11) + m12;
			}
		}
		return;
	}

	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		for (point = subPath->pointList; point; point = point->next)
		{
			vgCOORD		x0, y0, y;
			x0 = point->x;
			y0 = y = point->y;
			point->x = COORD_MUL(x0, m00) + COORD_MUL(y0, m01) + m02;
			point->y = y = COORD_MUL(x0, m10) + COORD_MUL(y0, m11) + m12;

			/*
			if (point->x < minX) minX = point->x;
			if (point->x > maxX) maxX = point->x;
			*/
			if (y < minY) minY = y;
			if (y > maxY) maxY = y;
		}
	}
	/*
	ScanLineTess->minXCoord = minX;
	ScanLineTess->maxXCoord = maxX;
	*/
	ScanLineTess->minYCoord = minY;
	ScanLineTess->maxYCoord = maxY;

	if (GetRange > 1)
	{
		/* Expand the min/max y range for strokes. */
		if (ScanLineTess->strokeJoinStyle == VG_JOIN_MITER)
		{
			vgCOORD extraRange = COORD_DIV_BY_TWO(ScanLineTess->strokeMiterLimit);
			extraRange = COORD_MUL(ScanLineTess->strokeLineWidth, extraRange);
			ScanLineTess->maxYCoord += extraRange;
			ScanLineTess->minYCoord -= extraRange;
		}
		else
		{
			ScanLineTess->maxYCoord += ScanLineTess->strokeLineWidth;
			ScanLineTess->minYCoord -= ScanLineTess->strokeLineWidth;
		}
	}
}

gceSTATUS
vgSL_AddStrokeSubPath(
	vgSCANLINETESS		ScanLineTess,
	vgSL_SUBPATH *		SubPath
	)
{
	gceSTATUS		status = gcvSTATUS_OK;

	gcmERR_RETURN(vgSL_AllocateASubPath(ScanLineTess, SubPath));

	if (ScanLineTess->lastStrokeSubPath != gcvNULL)
	{
		ScanLineTess->lastStrokeSubPath->next = *SubPath;
		ScanLineTess->lastStrokeSubPath = *SubPath;
	}
	else
	{
		ScanLineTess->lastStrokeSubPath = ScanLineTess->strokeSubPathList = *SubPath;
	}

	return status;
}

void
vgSL_GetLineDegreeAndLength(
	vgSL_POINT			Line,
	vgCOORD				HalfWidth,
	VGint *				Degree,
	vgCOORD *			Length,
	vgCOORD *			Dx,
	vgCOORD *			Dy,
	vgCOORD *			Ux,
	vgCOORD *			Uy
	)
{
	vgCOORD angle = COORD_MUL(COORD_DIV(Line->angle, PI), INT_TO_COORD(vgcCOS_TABLE_SIZE << 1));
	*Degree = COORD_TO_INT(angle);
	*Length = Line->length;
	*Ux = Line->tangentX;
	*Uy = Line->tangentY;
	*Dx = COORD_MUL(*Uy, HalfWidth);
	*Dy = COORD_MUL(-(*Ux), HalfWidth);
}

INLINE gceSTATUS
vgSL_AddAPointToLeftStrokePointListHead(
	vgSCANLINETESS		ScanLineTess,
	vgCOORD				X,
	vgCOORD				Y
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_POINT			point;

	gcmERR_RETURN(vgSL_AllocateAPoint(ScanLineTess, &point));
	point->x = X;
	point->y = Y;
	point->next = ScanLineTess->leftStrokePoint;
	ScanLineTess->leftStrokePoint = point;
	ScanLineTess->lastStrokeSubPath->pointCount++;

	return status;
}

INLINE gceSTATUS
vgSL_AddAPointToRightStrokePointListTail(
	vgSCANLINETESS		ScanLineTess,
	vgCOORD				X,
	vgCOORD				Y
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_POINT			point;

	gcmERR_RETURN(vgSL_AllocateAPoint(ScanLineTess, &point));
	point->x = X;
	point->y = Y;
	ScanLineTess->lastRightStrokePoint->next = point;
	ScanLineTess->lastRightStrokePoint = point;
	ScanLineTess->lastStrokeSubPath->pointCount++;

	return status;
}

gceSTATUS
vgSL_CreateEndCapTable(
	vgSCANLINETESS		ScanLineTess,
	vgCOORD				Radius
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_VECTOR2		pointArray;
	vgSL_VECTOR2		point, point2, point3, point4;
	VGint				n, n2, n4, inc;
	VGint				i, j, k;

	gcmASSERT(ScanLineTess->endCapTable == gcvNULL);
	//n = COORD_TO_INT(COORD_CEIL(COORD_MUL(FLOAT_TO_COORD(PI), COORD_SQRT(Radius))));
	n = COORD_TO_INT(COORD_CEIL(COORD_MUL(FLOAT_TO_COORD(PI_TWO), COORD_SQRT(Radius))));
	//n = COORD_TO_INT(COORD_CEIL(COORD_MUL(FLOAT_TO_COORD(PI), Radius)));
	gcmASSERT(n < 256);
	if (n > 16)
	{
		if (n > 64)
		{
			if (n > 128)	{ n = 256;	inc = 1; }
			else			{ n = 128;	inc = 2; }
		}
		else
		{
			if (n > 32)		{ n = 64;	inc = 4; }
			else			{ n = 32;	inc = 8; }
		}
	}
	else
	{
		if (n > 4)
		{
			if (n > 8)		{ n = 16;	inc = 16; }
			else			{ n = 8;	inc = 32; }
		}
		else
		{
			if (n > 2)		{ n = 4;	inc = 64; }
			else if (n == 2){ inc = 128; }
			else
			{ inc = 256; gcmASSERT(n == 1);  }
		}
	}
	n2 = n + n;
	n4 = n2 + n2;

	gcmERR_RETURN(vgSL_AllocateAVector2Array(ScanLineTess, &pointArray, n4));

	j = 0;
	k = vgcCOS_TABLE_SIZE;
	point = pointArray;
	for (i = 0; i <= n; i++, j += inc, k -= inc, point++)
	{
		point->x = COORD_MUL(vgcCosine[j], Radius);
		point->y = COORD_MUL(vgcCosine[k], Radius);
	}

	/* Transform points to quadrant 2, 3, 4. */
	point = pointArray;
	point3 = pointArray + n2;
	point2 = point3 - 1;	/* point2 = pointArray + n2 - 1; */
	point4 = point2 + n2;	/* point4 = pointArray + n4 - 1; */
	point3->x = -(point->x);
	point3->y = -(point->y);
	point++;
	point3++;
	for (i = 1; i < n; i++, point++, point2--, point3++, point4--)
	{
		point4->x = point->x;
		point3->x = point2->x = -(point->x);
		point2->y = point->y;
		point4->y = point3->y = -(point->y);
	}
	point3->x = -(point->x);
	point3->y = -(point->y);

	ScanLineTess->endCapTable = pointArray;
	ScanLineTess->endCapTableQuadrantSize = n;
	ScanLineTess->endCapTableIncrement = inc;
	return status;
}

gceSTATUS
vgSL_AddArcPoints(
	vgSCANLINETESS		ScanLineTess,
	vgCOORD				X,
	vgCOORD				Y,
	VGint				StartIdx,
	VGint				EndIdx,
	VGboolean			CounterClockwise
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_VECTOR2		pointArray = ScanLineTess->endCapTable;
	vgSL_VECTOR2		point;
	VGint				i;

	if (CounterClockwise)
	{
		if (EndIdx > StartIdx)
		{
			point = pointArray + StartIdx;
			for (i = StartIdx; i < EndIdx; i++, point++)
			{
				gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										X + point->x, Y + point->y));
			}
		}
		else
		{
			VGint n4 = ScanLineTess->endCapTableQuadrantSize << 2;

			point = pointArray + StartIdx;
			for (i = StartIdx; i < n4; i++, point++)
			{
				gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										X + point->x, Y + point->y));
			}
			point = pointArray;
			for (i = 0; i < EndIdx; i++, point++)
			{
				gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										X + point->x, Y + point->y));
			}
		}
	}
	else
	{
		if (EndIdx < StartIdx)
		{
			point = pointArray + StartIdx;
			for (i = StartIdx; i > EndIdx; i--, point--)
			{
				gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess,
										X + point->x, Y + point->y));
			}
		}
		else
		{
			VGint n4 = ScanLineTess->endCapTableQuadrantSize << 2;

			point = pointArray + StartIdx;
			for (i = StartIdx; i >= 0; i--, point--)
			{
				gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess,
										X + point->x, Y + point->y));
			}
			point = pointArray + n4 - 1;
			for (i = n4 - 1; i > EndIdx; i--, point--)
			{
				gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess,
										X + point->x, Y + point->y));
			}
		}
	}

	return status;
}

void
vgSL_AdjustJointPoint(
	vgSL_POINT			Point,
	vgSL_POINT			JoinPoint,
	vgCOORD				X,
	vgCOORD				Y,
	vgCOORD				Ratio
	)
{
	vgCOORD				mx = COORD_DIV_BY_TWO(JoinPoint->x + X);
	vgCOORD				my = COORD_DIV_BY_TWO(JoinPoint->y + Y);
	vgCOORD				dx = mx - Point->x;
	vgCOORD				dy = my - Point->y;

	dx = COORD_MUL(dx, Ratio);
	dy = COORD_MUL(dy, Ratio);
	JoinPoint->x = Point->x + dx;
	JoinPoint->y = Point->y + dy;
}

gceSTATUS
vgSL_ProcessLineJoint(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point,
	VGint				Degree,
	VGint				PrevDegree,
	vgCOORD				Length,
	vgCOORD				PrevLength,
	vgCOORD				HalfWidth,
	vgCOORD				X1,
	vgCOORD				Y1,
	vgCOORD				X2,
	vgCOORD				Y2
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	VGJoinStyle			strokeJoinStyle = ScanLineTess->strokeJoinStyle;
	VGint				theta, diffDegree;
	vgCOORD				thetaF;
	vgCOORD				miterRatio, ratio;
	vgCOORD				halfMiterLength;
	VGint				startIdx, endIdx;
	VGint				n, n4, inc;
	VGboolean			counterClockwise;
	VGboolean			fatLine = (ScanLineTess->strokeLineWidth >= FAT_LINE_WIDTH ? VG_TRUE : VG_FALSE);

	/* For flattened curves/arcs, the join style is always round. */
	if (Point->isFlattened && fatLine)
	{
		strokeJoinStyle = VG_JOIN_ROUND;
		if (ScanLineTess->endCapTable == gcvNULL)
		{
			/* Create endCapTable. */
			gcmERR_RETURN(vgSL_CreateEndCapTable(ScanLineTess, HalfWidth));
		}
	}

	/* First, determine the turn is clockwise or counter-clockwise. */
	theta = diffDegree = Degree - PrevDegree;
	thetaF = Point->angle - Point->prev->angle;
	if (theta == 0)
	{
		/* Straight line--no need to handle join. */
		return gcvSTATUS_OK;
	}

	/* Skip very small turn for thin lines. */
	if (HalfWidth < COORD_ONE)
	{
		if (abs(theta) < 20)
			return gcvSTATUS_OK;
	}

	if (theta >= 0)
	{
		if (theta > (vgcCOS_TABLE_SIZE << 1))
		{
			counterClockwise = VG_FALSE;
			theta = (vgcCOS_TABLE_SIZE << 2) - theta;
			thetaF = FLOAT_TO_COORD(PI_TWO) - thetaF;
		}
		else
		{
			counterClockwise = VG_TRUE;
		}
	}
	else
	{
		theta = -theta;
		thetaF = -thetaF;
		if (theta > (vgcCOS_TABLE_SIZE << 1))
		{
			counterClockwise = VG_TRUE;
			theta = (vgcCOS_TABLE_SIZE << 2) - theta;
			thetaF = FLOAT_TO_COORD(PI_TWO) - thetaF;
		}
		else
		{
			counterClockwise = VG_FALSE;
		}
	}

	if (theta < (vgcCOS_TABLE_SIZE << 1))
	{
		vgCOORD halfTheta = COORD_DIV_BY_TWO(thetaF);
		miterRatio = COORD_DIV(COORD_ONE, COORD_COS(halfTheta));
		ratio = COORD_MUL(miterRatio, miterRatio);
		halfMiterLength = COORD_MUL(HalfWidth, COORD_TAN(halfTheta));
	}
	else
	{
		miterRatio = COORD_MAX;
		ratio = COORD_MAX;
		halfMiterLength = COORD_MAX;
	}
	if (counterClockwise)
	{
		/* Check if the miter length is too long for inner intersection. */
		if (halfMiterLength <= Length && halfMiterLength <= PrevLength)
		{
			/* Adjust leftStrokePoint to the intersection point. */
			vgSL_AdjustJointPoint(Point, ScanLineTess->leftStrokePoint, X2, Y2, ratio);
		}
		else if (! Point->isFlattened)
		{
			/* Add the point to avoid incorrect sharp angle. */
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, Point->x, Point->y));
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, X2, Y2));
		}
		else if (! fatLine)
		{
			/* Flattened line segments should not have sharp angle. */
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, X2, Y2));
		}
		else
		{
			/* For flattend fat line with short line length, need to handle swing out pie shapes. */
			vgSL_POINT prevPoint = ScanLineTess->leftStrokePoint;

			/* Create a pie shape loop to cover the area. */
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, Point->x, Point->y));
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, X2, Y2));
			inc = ScanLineTess->endCapTableIncrement;
			if (diffDegree > inc || (diffDegree < 0 && diffDegree > inc - (vgcCOS_TABLE_SIZE << 2)))
			{
				n = ScanLineTess->endCapTableQuadrantSize;
				n4 = n << 2;
				startIdx = (PrevDegree + inc - 1) / inc + n - 1;
				if (startIdx >= n4) startIdx -= n4;
				endIdx = Degree / inc + n;
				if (endIdx >= n4) endIdx -= n4;
				if (startIdx != endIdx || abs(PrevDegree - Degree) > inc)
				{
					gcmERR_RETURN(vgSL_AddArcPoints(ScanLineTess, Point->x, Point->y, endIdx, startIdx, VG_FALSE));
				}
			}
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, prevPoint->x, prevPoint->y));
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, Point->x, Point->y));
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, X2, Y2));
		}

		switch (strokeJoinStyle)
		{
		case VG_JOIN_ROUND:
			inc = ScanLineTess->endCapTableIncrement;
			if (diffDegree > inc || (diffDegree < 0 && diffDegree > inc - (vgcCOS_TABLE_SIZE << 2)))
			{
				n = ScanLineTess->endCapTableQuadrantSize;
				n4 = n << 2;
				startIdx = PrevDegree / inc - n + 1;
				if (startIdx < 0) startIdx += n4;
				endIdx = (Degree + inc - 1) / inc - n;
				if (endIdx < 0) endIdx += n4;
				gcmASSERT(startIdx != endIdx || abs(PrevDegree - Degree) > inc);
				if (startIdx != endIdx || abs(PrevDegree - Degree) > inc)
				{
					gcmERR_RETURN(vgSL_AddArcPoints(ScanLineTess, Point->x, Point->y, startIdx, endIdx,
											counterClockwise));
				}
			}
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, X1, Y1));
			break;
		case VG_JOIN_MITER:
			if (miterRatio <= ScanLineTess->strokeMiterLimit)
			{
				/* Adjust lastRightStrokePoint to the outer intersection point. */
				vgSL_AdjustJointPoint(Point, ScanLineTess->lastRightStrokePoint, X1, Y1, ratio);
				break;
			}
			/* Else use Bevel join style. */
		case VG_JOIN_BEVEL:
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, X1, Y1));
			break;
		}
	}
	else
	{
		/* Check if the miter length is too long for inner intersection. */
		if (halfMiterLength <= Length && halfMiterLength <= PrevLength)
		{
			/* Adjust lastRightStrokePoint to the intersection point. */
			vgSL_AdjustJointPoint(Point, ScanLineTess->lastRightStrokePoint, X1, Y1, ratio);
		}
		else if (! Point->isFlattened)
		{
			/* Add the point to avoid incorrect sharp angle. */
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, Point->x, Point->y));
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, X1, Y1));
		}
		else if (! fatLine)
		{
			/* Flattened line segments should not have sharp angle. */
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, X1, Y1));
		}
		else
		{
			/* For flattend fat line with short line length, need to handle swing out pie shapes. */
			vgSL_POINT prevPoint = ScanLineTess->lastRightStrokePoint;

			/* Create a pie shape loop to cover the area. */
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, Point->x, Point->y));
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, X1, Y1));
			inc = ScanLineTess->endCapTableIncrement;
			if (diffDegree < -inc || (diffDegree > 0 && diffDegree < (vgcCOS_TABLE_SIZE << 2) - inc))
			{
				n = ScanLineTess->endCapTableQuadrantSize;
				n4 = n << 2;
				startIdx = PrevDegree / inc - n + 1;
				if (startIdx < 0) startIdx += n4;
				endIdx = (Degree + inc - 1) / inc - n;
				if (endIdx < 0) endIdx += n4;
				if (startIdx != endIdx || abs(PrevDegree - Degree) > inc)
				{
					gcmERR_RETURN(vgSL_AddArcPoints(ScanLineTess, Point->x, Point->y, endIdx, startIdx, VG_TRUE));
				}
			}
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, prevPoint->x, prevPoint->y));
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, Point->x, Point->y));
			gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess, X1, Y1));
		}

		switch (strokeJoinStyle)
		{
		case VG_JOIN_ROUND:
			inc = ScanLineTess->endCapTableIncrement;
			if (diffDegree < -inc || (diffDegree > 0 && diffDegree < (vgcCOS_TABLE_SIZE << 2) - inc))
			{
				n = ScanLineTess->endCapTableQuadrantSize;
				n4 = n << 2;
				startIdx = (PrevDegree + inc - 1) / inc + n - 1;
				if (startIdx >= n4) startIdx -= n4;
				endIdx = Degree / inc + n;
				if (endIdx >= n4) endIdx -= n4;
				if (startIdx != endIdx || abs(PrevDegree - Degree) > inc)
				{
					gcmERR_RETURN(vgSL_AddArcPoints(ScanLineTess, Point->x, Point->y, startIdx, endIdx,
											counterClockwise));
				}
			}
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, X2, Y2));
			break;
		case VG_JOIN_MITER:
			if (miterRatio <= ScanLineTess->strokeMiterLimit)
			{
				/* Adjust leftStrokePoint to the outer intersection point. */
				vgSL_AdjustJointPoint(Point, ScanLineTess->leftStrokePoint, X2, Y2, ratio);
				break;
			}
			/* Else use Bevel join style. */
		case VG_JOIN_BEVEL:
			gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess, X2, Y2));
			break;
		}
	}

	return status;
}

gceSTATUS
vgSL_AddZeroLengthStrokeSubPath(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_SUBPATH		strokeSubPath;
	vgSL_POINT			newPoint;
	vgCOORD				halfWidth = COORD_DIV_BY_TWO(ScanLineTess->strokeLineWidth);

	if (ScanLineTess->strokeCapStyle == VG_CAP_BUTT)
	{
		/* No need to draw zero-length subPath for VG_CAP_BUTT. */
		return gcvSTATUS_OK;
	}

	gcmERR_RETURN(vgSL_AddStrokeSubPath(ScanLineTess, &strokeSubPath));
	if (ScanLineTess->strokeCapStyle == VG_CAP_SQUARE)
	{
		/* Draw a square. */
		gcmERR_RETURN(vgSL_AllocateAPoint(ScanLineTess, &newPoint));
		newPoint->x = Point->x + halfWidth;
		newPoint->y = Point->y + halfWidth;
		strokeSubPath->pointList = ScanLineTess->lastRightStrokePoint = newPoint;
		strokeSubPath->pointCount = 1;
		gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										Point->x - halfWidth, Point->y + halfWidth));
		gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										Point->x - halfWidth, Point->y - halfWidth));
		gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										Point->x + halfWidth, Point->y - halfWidth));
	}
	else
	{
		/* Draw a circle. */
		gcmERR_RETURN(vgSL_AllocateAPoint(ScanLineTess, &newPoint));
		newPoint->x = Point->x + halfWidth;
		newPoint->y = Point->y;
		strokeSubPath->pointList = ScanLineTess->lastRightStrokePoint = newPoint;
		strokeSubPath->pointCount = 1;
		gcmERR_RETURN(vgSL_AddArcPoints(ScanLineTess, Point->x, Point->y, 1,
										(ScanLineTess->endCapTableQuadrantSize << 2) - 1,
										VG_TRUE));
	}

	strokeSubPath->lastPoint = ScanLineTess->lastRightStrokePoint;

	return status;
}

gceSTATUS
vgSL_StartANewStrokeSubPath(
	vgSCANLINETESS		ScanLineTess,
	vgCOORD				X,
	vgCOORD				Y,
	VGint				Degree,
	vgCOORD				Dx,
	vgCOORD				Dy,
	VGboolean			AddEndCap,
	vgSL_SUBPATH *		StrokeSubPath
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_SUBPATH		strokeSubPath;
	vgSL_POINT			newPoint;
	VGint				startIdx, endIdx;
	VGint				n, n4, inc;

	gcmERR_RETURN(vgSL_AddStrokeSubPath(ScanLineTess, &strokeSubPath));

	gcmERR_RETURN(vgSL_AllocateAPoint(ScanLineTess, &newPoint));
	newPoint->x = X + Dx;
	newPoint->y = Y + Dy;
	strokeSubPath->pointList = ScanLineTess->lastRightStrokePoint = newPoint;

	gcmERR_RETURN(vgSL_AllocateAPoint(ScanLineTess, &newPoint));
	newPoint->x = X - Dx;
	newPoint->y = Y - Dy;
	strokeSubPath->lastPoint = ScanLineTess->leftStrokePoint = newPoint;

	strokeSubPath->pointCount = 2;

	if (AddEndCap)
	{
		/* Add end cap if the subPath is not closed. */
		switch (ScanLineTess->strokeCapStyle)
		{
		case VG_CAP_BUTT:
			/* No adjustment needed. */
			break;
		case VG_CAP_ROUND:
			inc = ScanLineTess->endCapTableIncrement;
			n = ScanLineTess->endCapTableQuadrantSize;
			n4 = n << 2;
			startIdx = Degree / inc + n + 1;
			if (startIdx >= n4) startIdx -= n4;
			endIdx = (Degree + inc - 1) / inc - n;
			if (endIdx < 0) endIdx += n4;

			/* Fake ScanLineTess->lastRightStrokePoint for vgSL_AddArcPoints. */
			ScanLineTess->lastRightStrokePoint = strokeSubPath->lastPoint;
			gcmERR_RETURN(vgSL_AddArcPoints(ScanLineTess, X, Y,
										startIdx, endIdx, VG_TRUE));

			/* Restore back correct pointers. */
			strokeSubPath->lastPoint = ScanLineTess->lastRightStrokePoint;
			ScanLineTess->lastRightStrokePoint = strokeSubPath->pointList;
			break;
		case VG_CAP_SQUARE:
			ScanLineTess->lastRightStrokePoint->x += Dy;
			ScanLineTess->lastRightStrokePoint->y -= Dx;
			ScanLineTess->leftStrokePoint->x += Dy;
			ScanLineTess->leftStrokePoint->y -= Dx;
			break;
		}
	}

	*StrokeSubPath = strokeSubPath;
	return status;
}

gceSTATUS
vgSL_EndAStrokeSubPath(
	vgSCANLINETESS		ScanLineTess,
	vgCOORD				X,
	vgCOORD				Y,
	VGint				Degree,
	vgCOORD				Dx,
	vgCOORD				Dy
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	VGint				startIdx, endIdx;
	VGint				n, n4, inc;

	/* Add points for end of line. */
	gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										X + Dx, Y + Dy));
	gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess,
										X - Dx, Y - Dy));

	/* Add end cap if the subPath is not closed. */
	switch (ScanLineTess->strokeCapStyle)
	{
	case VG_CAP_BUTT:
		/* No adjustment needed. */
		break;
	case VG_CAP_ROUND:
		inc = ScanLineTess->endCapTableIncrement;
		n = ScanLineTess->endCapTableQuadrantSize;
		n4 = n << 2;
		startIdx = Degree / inc - n + 1;
		if (startIdx < 0) startIdx += n4;
		endIdx = (Degree + inc - 1) / inc + n;
		if (endIdx >= n4) endIdx -= n4;
		gcmERR_RETURN(vgSL_AddArcPoints(ScanLineTess, X, Y,
										startIdx, endIdx, VG_TRUE));
		break;
	case VG_CAP_SQUARE:
		ScanLineTess->lastRightStrokePoint->x -= Dy;
		ScanLineTess->lastRightStrokePoint->y += Dx;
		ScanLineTess->leftStrokePoint->x -= Dy;
		ScanLineTess->leftStrokePoint->y += Dx;
		break;
	}

	/* Concatnate right and left point lists. */
	ScanLineTess->lastRightStrokePoint->next = ScanLineTess->leftStrokePoint;

	return status;
}

gceSTATUS
vgSL_CloseAStrokeSubPath(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point,
	VGint				Degree,
	VGint				PrevDegree,
	vgCOORD				Length,
	vgCOORD				PrevLength,
	vgCOORD				HalfWidth,
	vgSL_POINT			firstStrokePoint,
	vgSL_POINT			lastStrokePoint
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

	/* Handle line joint style for the first/last point in closed path. */
	gcmERR_RETURN(vgSL_ProcessLineJoint(ScanLineTess, Point, Degree, PrevDegree,
										Length, PrevLength, HalfWidth,
										firstStrokePoint->x, firstStrokePoint->y,
										lastStrokePoint->x, lastStrokePoint->y));

	/* Adjust the two end ponts of the first point. */
	firstStrokePoint->x = ScanLineTess->lastRightStrokePoint->x;
	firstStrokePoint->y = ScanLineTess->lastRightStrokePoint->y;
	lastStrokePoint->x = ScanLineTess->leftStrokePoint->x;
	lastStrokePoint->y = ScanLineTess->leftStrokePoint->y;

	/* Concatnate right and left point lists. */
	ScanLineTess->lastRightStrokePoint->next = ScanLineTess->leftStrokePoint;

	return status;
}

gceSTATUS
vgSL_InitializeStrokeParameters(
	vgSCANLINETESS		ScanLineTess,
	VGboolean *			Dashing
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgCOORD				scale = ScanLineTess->strokeScale;
	_VGContext *		context = ScanLineTess->context;
	VGint				count = VGSL_GETCONTEXTSTOKEDASHPATTERNSIZE(context);

	if (count == 0)
	{
		*Dashing = VG_FALSE;
	}
	else
	{
		VGint		i;
		VGfloat *	patternSrc;
		vgCOORD *	pattern;
		vgCOORD		length;

		*Dashing = VG_TRUE;
		ScanLineTess->strokeDashPhase = length = COORD_MUL(FLOAT_TO_COORD(VGSL_GETCONTEXTSTOKEDASHPHASE(context)), scale);
		ScanLineTess->strokeDashPhaseReset = VGSL_GETCONTEXTSTOKEDASHPHASERESET(context);

		/* The last pattern is ignored if the number is odd. */
		if (count & 0x1) count--;

		ScanLineTess->strokeDashPatternCount = count;
		gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
							count * gcmSIZEOF(vgCOORD),
							(gctPOINTER *) &pattern));
		ScanLineTess->strokeDashPattern = pattern;
		ScanLineTess->strokeDashPatternLength = COORD_ZERO;
		patternSrc = VGSL_GETCONTEXTSTOKEDASHPATTERNPOINTER(context);
		for (i = 0; i < count; i++, pattern++, patternSrc++)
		{
			if (*patternSrc < 0.0f)
			{
				*pattern = COORD_ZERO;
			}
			else if (*patternSrc >= 32768.0f)
			{
				*pattern = COORD_MAX;
			}
			else
			{
				*pattern = COORD_MUL(FLOAT_TO_COORD(*patternSrc), scale);
			}
			ScanLineTess->strokeDashPatternLength += *pattern;
		}

		if (ScanLineTess->strokeDashPatternLength < COORD_EPSILON)
		{
			*Dashing = VG_FALSE;
			return gcvSTATUS_OK;
		}

		while (length < COORD_ZERO)
		{
			length += ScanLineTess->strokeDashPatternLength;
		}

		while (length >= ScanLineTess->strokeDashPatternLength)
		{
			length -= ScanLineTess->strokeDashPatternLength;
		}

		pattern = ScanLineTess->strokeDashPattern;
		for (i = 0; i < ScanLineTess->strokeDashPatternCount; i++, pattern++)
		{
			if (length <= *pattern) break;

			length -= *pattern;
		}
		ScanLineTess->strokeDashInitialIndex = i;
		ScanLineTess->strokeDashInitialLength = *pattern - length;
	}

	return status;
}

INLINE void
vgSL_GetNextDashLength(
	vgSCANLINETESS		ScanLineTess,
	VGint *				DashIndex,
	vgCOORD *			DashLength
	)
{
	(*DashIndex)++;
	if (*DashIndex == ScanLineTess->strokeDashPatternCount)
	{
		*DashIndex = 0;
	}
	*DashLength = ScanLineTess->strokeDashPattern[*DashIndex];
}

gceSTATUS
vgSL_CreateStrokePath(
	vgSCANLINETESS		ScanLineTess
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_SUBPATH		subPath, strokeSubPath;
	vgSL_POINT			point, nextPoint;
	vgCOORD				halfWidth = COORD_DIV_BY_TWO(ScanLineTess->strokeLineWidth);
	vgCOORD				x, y;
	vgCOORD				dx, dy, ux, uy;
	vgCOORD				length, prevLength, firstLength;
	VGint				degree, prevDegree, firstDegree;
	vgCOORD				dashLength;
	VGint				dashIndex;
	VGboolean			dashing;
	VGboolean			addEndCap;

	gcmERR_RETURN(vgSL_InitializeStrokeParameters(ScanLineTess, &dashing));
	dashIndex = ScanLineTess->strokeDashInitialIndex;
	dashLength = ScanLineTess->strokeDashInitialLength;

	if (ScanLineTess->strokeCapStyle == VG_CAP_ROUND ||
		ScanLineTess->strokeJoinStyle == VG_JOIN_ROUND)
	{
		/* Create endCapTable. */
		gcmERR_RETURN(vgSL_CreateEndCapTable(ScanLineTess, halfWidth));
	}

	for (subPath = ScanLineTess->subPathList; subPath; subPath = subPath->next)
	{
		vgSL_SUBPATH firstStrokeSubPath = gcvNULL;
		vgSL_POINT	firstRightPoint = gcvNULL;
		vgSL_POINT	lastLeftPoint = gcvNULL;
		vgCOORD		firstDx, firstDy;
		VGboolean	drawing = VG_FALSE;

		if (dashing && ScanLineTess->strokeDashPhaseReset)
		{
			dashIndex = ScanLineTess->strokeDashInitialIndex;
			dashLength = ScanLineTess->strokeDashInitialLength;
		}

		point = subPath->pointList;
		gcmASSERT(point != gcvNULL);

		nextPoint = point->next;
		if (nextPoint == gcvNULL)
		{
			if (!dashing || ((dashIndex & 0x1) == 0))
			{
				/* Single point (zero-length) subpath. */
				/* Note that one-MOVE_TO subpaths are removed during parsing. */
				gcmERR_RETURN(vgSL_AddZeroLengthStrokeSubPath(ScanLineTess, point));
			}
			continue;
		}

		/* Adjust closed status for dashing. */
		if (dashing && subPath->closed && ((dashIndex & 0x1) == 1))
		{
			subPath->closed = VG_FALSE;
		}

		/* Set addEndCap. */
		addEndCap = subPath->closed ? VG_FALSE : VG_TRUE;

		/* Process first line. */
		vgSL_GetLineDegreeAndLength(point, halfWidth, &firstDegree, &firstLength,
										&dx, &dy, &ux, &uy);

		if (dashing)
		{
			vgCOORD deltaLength;

			/* Draw dashes. */
			x = point->x;
			y = point->y;
			do
			{
				if ((dashIndex & 0x1) == 0)
				{
					gcmERR_RETURN(vgSL_StartANewStrokeSubPath(ScanLineTess,
										x, y, firstDegree,
										dx, dy, addEndCap,
										&strokeSubPath));
					drawing = VG_TRUE;
					addEndCap = VG_TRUE;
					if (subPath->closed && (firstStrokeSubPath == gcvNULL))
					{
						firstStrokeSubPath = ScanLineTess->lastStrokeSubPath;
						firstRightPoint = ScanLineTess->lastRightStrokePoint;
						lastLeftPoint = ScanLineTess->leftStrokePoint;
						gcmASSERT(lastLeftPoint->next == gcvNULL);
						firstDx = dx;
						firstDy = dy;
					}
				}

				deltaLength = firstLength - dashLength;
				if (deltaLength >= COORD_EPSILON)
				{
					/* Move (x, y) forward along the line by dashLength. */
					x += COORD_MUL(ux, dashLength);
					y += COORD_MUL(uy, dashLength);

					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(drawing);
						gcmERR_RETURN(vgSL_EndAStrokeSubPath(ScanLineTess,
										x, y, firstDegree, dx, dy));
						drawing = VG_FALSE;
					}

					vgSL_GetNextDashLength(ScanLineTess, &dashIndex, &dashLength);
					firstLength = deltaLength;
				}
				else if (deltaLength <= -COORD_EPSILON)
				{
					dashLength = -deltaLength;
					break;
				}
				else
				{
					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(drawing);
						gcmERR_RETURN(vgSL_EndAStrokeSubPath(ScanLineTess,
										nextPoint->x, nextPoint->y, firstDegree, dx, dy));
						drawing = VG_FALSE;
					}
					vgSL_GetNextDashLength(ScanLineTess, &dashIndex, &dashLength);
					firstLength = 0;
					break;
				}
			}
			while (VG_TRUE);
		}
		else
		{
			gcmERR_RETURN(vgSL_StartANewStrokeSubPath(ScanLineTess,
										point->x, point->y, firstDegree,
										dx, dy, addEndCap,
										&strokeSubPath));
			drawing = VG_TRUE;
			addEndCap = VG_TRUE;
		}

		/* Process the rest of lines. */
		prevDegree = firstDegree;
		prevLength = firstLength;
		for (point = nextPoint, nextPoint = point->next; nextPoint;
			 point = nextPoint, nextPoint = point->next)
		{
			if (!dashing || ((dashIndex & 0x1) == 0 && drawing))
			{
				/* Add points for end of line for line join process with next line. */
				gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										point->x + dx, point->y + dy));
				gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess,
										point->x - dx, point->y - dy));
			}

			vgSL_GetLineDegreeAndLength(point, halfWidth, &degree, &length, &dx, &dy,
										&ux, &uy);

			if (!dashing)
			{
				/* Handle line joint style. */
				gcmERR_RETURN(vgSL_ProcessLineJoint(ScanLineTess, point, degree, prevDegree,
										length, prevLength, halfWidth,
										point->x + dx, point->y + dy,
										point->x - dx, point->y - dy));
			}
			else
			{
				vgCOORD deltaLength;

				/* Draw dashes. */
				x = point->x;
				y = point->y;
				if ((dashIndex & 0x1) == 0)
				{
					if (drawing)
					{
						/* Handle line joint style. */
						gcmERR_RETURN(vgSL_ProcessLineJoint(ScanLineTess, point, degree, prevDegree,
										dashLength, prevLength, halfWidth,
										x + dx, y + dy,
										x - dx, y - dy));
					}
					else
					{
						/* Start a new sub path. */
						gcmASSERT(addEndCap);
						gcmERR_RETURN(vgSL_StartANewStrokeSubPath(ScanLineTess,
										x, y, degree,
										dx, dy, addEndCap,
										&strokeSubPath));
						drawing = VG_TRUE;
						addEndCap = VG_TRUE;
					}
				}
				do
				{
					deltaLength = length - dashLength;
					if (deltaLength >= COORD_EPSILON)
					{
						/* Move (x, y) forward along the line by dashLength. */
						x += COORD_MUL(ux, dashLength);
						y += COORD_MUL(uy, dashLength);

						if ((dashIndex & 0x1) == 0)
						{
							gcmASSERT(drawing);
							gcmERR_RETURN(vgSL_EndAStrokeSubPath(ScanLineTess,
											x, y, degree, dx, dy));
							drawing = VG_FALSE;
						}

						vgSL_GetNextDashLength(ScanLineTess, &dashIndex, &dashLength);
						length = deltaLength;
					}
					else if (deltaLength <= -COORD_EPSILON)
					{
						dashLength = -deltaLength;
						break;
					}
					else
					{
						if ((dashIndex & 0x1) == 0)
						{
							gcmASSERT(drawing);
							gcmERR_RETURN(vgSL_EndAStrokeSubPath(ScanLineTess,
											nextPoint->x, nextPoint->y, degree, dx, dy));
							drawing = VG_FALSE;
						}
						vgSL_GetNextDashLength(ScanLineTess, &dashIndex, &dashLength);
						length = 0;
						break;
					}

					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(addEndCap);
						gcmERR_RETURN(vgSL_StartANewStrokeSubPath(ScanLineTess,
											x, y, degree,
											dx, dy, addEndCap,
											&strokeSubPath));
						drawing = VG_TRUE;
						addEndCap = VG_TRUE;
					}
				}
				while (VG_TRUE);
			}

			prevDegree = degree;
			prevLength = length;
		}

		if (subPath->closed)
		{
			if (! dashing || drawing)
			{
				/* Add points for end of line. */
				gcmERR_RETURN(vgSL_AddAPointToRightStrokePointListTail(ScanLineTess,
										point->x + dx, point->y + dy));
				gcmERR_RETURN(vgSL_AddAPointToLeftStrokePointListHead(ScanLineTess,
										point->x - dx, point->y - dy));

				if (! dashing)
				{
					/* Handle line joint style for the first/last point in closed path. */
					gcmERR_RETURN(vgSL_CloseAStrokeSubPath(ScanLineTess, point,
										firstDegree, prevDegree,
										firstLength, prevLength, halfWidth,
										strokeSubPath->pointList, strokeSubPath->lastPoint));
				}
				else
				{
					/* Handle line joint style for the first/last point in closed path. */
					gcmERR_RETURN(vgSL_CloseAStrokeSubPath(ScanLineTess, point,
										firstDegree, prevDegree,
										firstLength, prevLength, halfWidth,
										firstRightPoint, lastLeftPoint));
				}
			}
			else if (ScanLineTess->strokeCapStyle != VG_CAP_BUTT)
			{
				/* No closing join need.  Add end cap for the starting point. */

				if (ScanLineTess->strokeCapStyle == VG_CAP_SQUARE)
				{
					firstRightPoint->x += firstDy;
					firstRightPoint->y -= firstDx;
					lastLeftPoint->x += firstDy;
					lastLeftPoint->y -= firstDx;
				}
				else
				{
					VGint startIdx, endIdx;
					VGint n, n4, inc;
					vgSL_SUBPATH lastStrokeSubPath = ScanLineTess->lastStrokeSubPath;

					inc = ScanLineTess->endCapTableIncrement;
					n = ScanLineTess->endCapTableQuadrantSize;
					n4 = n << 2;
					startIdx = firstDegree / inc + n + 1;
					if (startIdx >= n4) startIdx -= n4;
					endIdx = (firstDegree + inc - 1) / inc - n;
					if (endIdx < 0) endIdx += n4;

					/* Fake ScanLineTess->lastStrokeSubPath and ScanLineTess->lastRightStrokePoint for vgSL_AddArcPoints. */
					gcmASSERT(firstStrokeSubPath);
					ScanLineTess->lastStrokeSubPath = firstStrokeSubPath;
					ScanLineTess->lastRightStrokePoint = lastLeftPoint;
					gcmERR_RETURN(vgSL_AddArcPoints(ScanLineTess,
												subPath->pointList->x, subPath->pointList->y,
												startIdx, endIdx, VG_TRUE));

					/* Restore back correct pointers. */
					ScanLineTess->lastStrokeSubPath = lastStrokeSubPath;
					firstStrokeSubPath->lastPoint = ScanLineTess->lastRightStrokePoint;
				}
			}
		}
		else if (! dashing ||
				 (((dashIndex & 0x1) == 0) && (dashLength < ScanLineTess->strokeDashPattern[dashIndex])))
		{
			/* Add end cap if the subPath is not closed. */
			gcmERR_RETURN(vgSL_EndAStrokeSubPath(ScanLineTess,
										point->x, point->y, prevDegree, dx, dy));
			drawing = VG_FALSE;
		}
	}

	if (ScanLineTess->endCapTable)
	{
		/* Free endCapTable. */
		vgSL_FreeAVector2Array(ScanLineTess, ScanLineTess->endCapTable);
		ScanLineTess->endCapTable = gcvNULL;
	}

	return status;
}

void vgSL_SetF  (
	vgSL_POINT			Point,
	VGbyte *			Data
	)
{
	VGfloat *			dataP = (VGfloat *) Data;

	*dataP = COORD_TO_FLOAT(Point->x);
	dataP++;
	*dataP = COORD_TO_FLOAT(Point->y);
}

gceSTATUS
vgSL_CopyStrokePath(
	vgSCANLINETESS		ScanLineTess,
	_VGPath *			InputPath,
	_VGPath *			OutputPath
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	VGint				dataTypeSize;           // Size of the data type.
	VGint				pointDataSize;          // Size of the data type for a point.
	VGint				segmentCount = 0;
	VGint				pointCount = 0;
#if !gcdUSE_VG
	VGbyte *			segmentPointer;			// Pointer to the command segments of the VGPath.
#endif
	VGbyte *			dataPointer;			// Pointer to the point data of the VGPath.
	VGbyte				spaceForCloseCommand = 1;
	VGbyte				moveCommand;
	VGbyte				lineCommand;
	VGbyte				closeCommand;
	vgCOORD				scale = COORD_DIV(COORD_ONE, ScanLineTess->scale);
	vgSL_SUBPATH		subPath = gcvNULL;
	vgSL_POINT			point = gcvNULL;
	vgSL_POINT			startPoint = gcvNULL;

	/* Set data format and size. */
	gcmASSERT(VGSL_GETPATHSCALE(OutputPath) == 1.0f);
	gcmASSERT(VGSL_GETPATHBIAS(OutputPath) == 0.0f);
	gcmASSERT(VGSL_GETPATHDATATYPE(OutputPath) == vgvSL_PATH_DATATYPE_F);
	dataTypeSize = 4;

	/* Count segment number and data size. */
#if gcdUSE_VG
	/* CLOSE_PATH can share the same word with next MOVE_TO. */
	spaceForCloseCommand = 0;

	/* Last CLOSE_PATH cannot share with other command. */
	segmentCount++;
#endif

	for (subPath = ScanLineTess->strokeSubPathList; subPath; subPath = subPath->next)
	{
		segmentCount += subPath->pointCount + spaceForCloseCommand;
		pointCount += subPath->pointCount;
	}

	if (pointCount == 0) return gcvSTATUS_OK;

#if gcdUSE_VG
	/* TODO - Free old segments and data of Outputpath. */
#else
	/* Free old segments and data of Outputpath. */
	if (ARRAY_SIZE(VGSL_GETPATHSEGMENTS(OutputPath)) > 0)
	{
		ARRAY_CLEAR(VGSL_GETPATHSEGMENTS(OutputPath));
	}
	if (ARRAY_SIZE(VGSL_GETPATHDATA(OutputPath)) > 0)
	{
		ARRAY_CLEAR(VGSL_GETPATHDATA(OutputPath));
	}
#endif

	/* Allocate segments and data for OutputPath. */
	pointDataSize = dataTypeSize << 1;

#if gcdUSE_VG
	OutputPath->size = segmentCount * dataTypeSize +  pointCount * pointDataSize;
	gcmERR_RETURN(gcoOS_Allocate(NULL, OutputPath->size, (gctPOINTER *) &OutputPath->logical));

	dataPointer		= (VGbyte *) OutputPath->logical;
#else
	ARRAY_RESIZE(VGSL_GETPATHSEGMENTS(OutputPath), segmentCount);
	ARRAY_RESIZE(VGSL_GETPATHDATA(OutputPath), pointDataSize * pointCount);

#if !USE_SCAN_LINE_ON_RI
	if (OutputPath->segments.allocated == 0) return gcvSTATUS_OUT_OF_MEMORY;
	if (OutputPath->data.allocated == 0) return gcvSTATUS_OUT_OF_MEMORY;
#endif

	segmentPointer	= (VGbyte *) VGSL_GETPATHSEGMENTSPOINTER(OutputPath);
	dataPointer		= (VGbyte *) VGSL_GETPATHDATAPOINTER(OutputPath);
#endif

	moveCommand  = vgvSL_MOVE_TO;
	lineCommand  = VG_LINE_TO_ABS;
	closeCommand = vgvSL_CLOSE_PATH;

#if gcdUSE_VG
#define VGSL_SETALIGN(P, ALIGN) \
	P = (VGbyte *)(((VGuint) (P) + ((ALIGN) - 1)) & ~((ALIGN) - 1));
#define VGSL_SETCOMMAND(COMMAND) \
	*dataPointer = COMMAND; \
	dataPointer++;
#define VGSL_SETPOINT(POINT) \
	VGSL_SETALIGN(dataPointer, dataTypeSize); \
	vgSL_SetF(POINT, dataPointer); \
	dataPointer += pointDataSize; \
	pointCount--;
#else
#define VGSL_SETCOMMAND(COMMAND) \
	*segmentPointer = COMMAND; \
	segmentPointer++; \
	segmentCount--;
#define VGSL_SETPOINT(POINT) \
	vgSL_SetF(POINT, dataPointer); \
	dataPointer += pointDataSize; \
	pointCount--;
#endif

	/* Copy stroke path to OutputPath with reversed scale. */
	for (subPath = ScanLineTess->strokeSubPathList; subPath; subPath = subPath->next)
	{
		/* Create a MOVE_TO command for the first point. */
		startPoint = point = subPath->pointList;
		gcmASSERT(point);
		VGSL_SETCOMMAND(moveCommand);
		VGSL_SETPOINT(point);

		/* Create a LINE_TO command for the rest of points. */
		for (point = point->next; point; point = point->next)
		{
			VGSL_SETCOMMAND(lineCommand);
			VGSL_SETPOINT(point);
		}

		/* Create a CLOSE_PATH command at the end. */
		VGSL_SETCOMMAND(closeCommand);
	}
#if gcdUSE_VG
	VGSL_SETALIGN(dataPointer, dataTypeSize);
	gcmASSERT(dataPointer - OutputPath->logical == OutputPath->size);
#else
	gcmASSERT(segmentCount == 0);
#endif
	gcmASSERT(pointCount == 0);

	return status;
}

#if !gcdUSE_VG
#if SCAN_LINE_HAS_3D_SUPPORT
VGint
vgSL_XCoordCompare(
	vgSL_XCOORD		XCoord1,
	vgSL_XCOORD		XCoord2,
	vgCOORD			Y
	)
{
	vgCOORD			xDelta = XCoord1->x - XCoord2->x;
	vgSL_POINT		line1, line2;
	VGboolean		below1 = VG_TRUE;
	VGboolean		below2 = VG_TRUE;

	if (xDelta >= COORD_EPSILON) return 1;
	if (xDelta <= -COORD_EPSILON) return -1;
	/* When both XCoords are not endpoints, it does not matter.  Just return -1. */
	line1 = XCoord1->line;
	line2 = XCoord2->line;
	if (line1->lowerPointOnYLine)
	{
		if ((line1->orientation == vgvSL_UP && XCoord1->endLineStatus == 1) ||
			(line1->orientation == vgvSL_DOWN && XCoord1->endLineStatus == 2))
		{
			below1 = VG_FALSE;
		}
	}
	if (line2->lowerPointOnYLine)
	{
		if ((line2->orientation == vgvSL_UP && XCoord2->endLineStatus == 1) ||
			(line2->orientation == vgvSL_DOWN && XCoord2->endLineStatus == 2))
		{
			below2 = VG_FALSE;
		}
	}
	xDelta = line1->deltaX - line2->deltaX;

	if (below1)
	{
		if (below2)
		{
			if (xDelta < COORD_ZERO) return 1;
			if (xDelta > COORD_ZERO) return -1;
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		if (below2)
		{
			return 1;
		}
		else
		{
			if (xDelta < COORD_ZERO) return -1;
			if (xDelta > COORD_ZERO) return 1;
			return 0;
		}
	}
}
#else
VGint
vgSL_XCoordCompare(
	const void *		Element1,
	const void *		Element2
	)
{
	vgSL_XCOORD		xCoord1 = *((vgSL_XCOORD *) Element1);
	vgSL_XCOORD		xCoord2 = *((vgSL_XCOORD *) Element2);

	if (xCoord1->x > xCoord2->x) return 1;
	else return -1;
}
#endif

void
vgSL_sortXCoords(
	vgSL_YSCANLINE		YScanLine,
	vgSL_XCOORD *		XCoordArray
	)
{
	VGint				count = YScanLine->xcoordCount;
	VGint				i, j;
	vgSL_XCOORD			xCoord1, xCoord2, xCoord3;
	vgSL_XCOORD *		xCoordP;
#if SCAN_LINE_HAS_3D_SUPPORT
	vgCOORD				y = YScanLine->y;
#endif

	/* TODO - Can be improved by checking if most of them in ascending or descending order. */

	gcmASSERT(count > 1);

#if !SCAN_LINE_HAS_3D_SUPPORT
	if (count < 10)
	{
#endif
		/* Bubble sort. */
		//gcmASSERT(count < 60);
		for (i = count - 1; i > 0; i--)
		{
			xCoord1 = gcvNULL;
			xCoord2 = YScanLine->xcoords;
			for (j = 0; j < i; j++)
			{
				xCoord3 = xCoord2->next;
				gcmASSERT(xCoord3);

#if SCAN_LINE_HAS_3D_SUPPORT
				if (vgSL_XCoordCompare(xCoord2, xCoord3, y) > 0)
#else
				if (xCoord2->x > xCoord3->x)
#endif
				{
					/* Swap xCoord2 and xCoord3. */
					xCoord2->next = xCoord3->next;
					xCoord3->next = xCoord2;
					if (xCoord1)
					{
						xCoord1->next = xCoord3;
					}
					else
					{
						YScanLine->xcoords = xCoord3;
					}
					xCoord1 = xCoord3;
				}
				else
				{
					xCoord1 = xCoord2;
					xCoord2 = xCoord3;
				}
			}
		}
#if !SCAN_LINE_HAS_3D_SUPPORT
	}
	else
	{
		/* QSort. */
		/* Build xcoord array of pointers to xcoord list. */
		xCoordP = XCoordArray;
		xCoord1 = YScanLine->xcoords;
		for (i = 0; i < count; i++, xCoordP++, xCoord1 = xCoord1->next)
		{
			*xCoordP = xCoord1;
		}

		/* Sort the array. */
		qsort(XCoordArray, count, sizeof(vgSL_XCOORD), vgSL_XCoordCompare);

		/* Rebuild xcoord list from the array. */
		xCoordP = XCoordArray;
		YScanLine->xcoords = xCoord1 = *xCoordP;
		for (i = 1, xCoordP++; i < count; i++, xCoordP++)
		{
			xCoord1->next = *xCoordP;
			xCoord1 = *xCoordP;
		}
		xCoord1->next = gcvNULL;
	}
#endif
}

gceSTATUS
vgSL_AddXCoordToYScanLine(
	vgSCANLINETESS		ScanLineTess,
	vgCOORD				X,
	vgSL_ORIENTATION	Orientation,
	VGbyte				EndLineStatus,
	vgSL_POINT			Line,
	vgSL_YSCANLINE		YScanLine
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_XCOORD			xCoord;
#if SCAN_LINE_HAS_3D_SUPPORT
	vgCOORD				y = YScanLine->y;
#endif

	gcmERR_RETURN(vgSL_AllocateAnXCoord(ScanLineTess, &xCoord));

	xCoord->x			= X;
	xCoord->orientation	= Orientation;
#if SCAN_LINE_HAS_3D_SUPPORT
	xCoord->endLineStatus = EndLineStatus;
	xCoord->line		= Line;
	if (EndLineStatus) YScanLine->numLineEnds++;
#endif

	xCoord->next = YScanLine->xcoords;
	YScanLine->xcoords = xCoord;
	YScanLine->xcoordCount++;
#if SCAN_LINE_HAS_3D_SUPPORT
	Line->crossYLinesCount++;
#endif

	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_AddPointsForLine(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point1,
	vgSL_POINT			Point2
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgCOORD				x1 = Point1->x;
	vgCOORD				y1 = Point1->y;
	vgCOORD				x2 = Point2->x;
	vgCOORD				y2 = Point2->y;
	vgCOORD				deltaX = Point1->deltaX;
	vgSL_ORIENTATION	orientation = Point1->orientation;
	vgSL_YSCANLINE		yScanLine;
	VGint				intY, intYLast;
	vgCOORD				x, y;
	VGbyte				endLineStatus = 1;

	ScanLineTess->currentOrientation = orientation;

	if (orientation == vgvSL_FLAT) return gcvSTATUS_OK;

	/* Get the first y. */
	if (Point1->yIsInteger)
	{
		y = y1;
	}
	else if (orientation == vgvSL_UP)
	{
		y = COORD_CEIL(y1);
	}
	else
	{
		y = COORD_FLOOR(y1);
	}
	intY = COORD_TO_INT(y);

	/* Get yScanLine. */
	yScanLine = ScanLineTess->yScanLineArray + (intY - ScanLineTess->yMin);
	gcmASSERT(yScanLine->y == y);

	if (Point1->yIsInteger)
	{
		/* Check if the first point should be added. */
		if (orientation != ScanLineTess->prevOrientation && orientation != vgvSL_FLAT)
		{
			vgSL_ORIENTATION orient;

			if (ScanLineTess->prevOrientation != vgvSL_FLAT)
			{
				orient = orientation;
			}
			else if (ScanLineTess->prevPrevOrientation != vgvSL_FLAT)
			{
				if (ScanLineTess->prevPrevOrientation != orientation)
				{
					orient = orientation;
				}
				else
				{
					orient = vgvSL_FLAT;
				}
			}
			else
			{
				gcmASSERT(ScanLineTess->prevPrevOrientation != vgvSL_FLAT);
				/* Cannot determine the orientation. */
				/* It is due to the first line is flat. */
				orient = orientation;
			}
			gcmERR_RETURN(vgSL_AddXCoordToYScanLine(ScanLineTess, x1, orient,
										1,
										Point1, yScanLine));
		}
#if SCAN_LINE_HAS_3D_SUPPORT
		endLineStatus = 0;
#endif

		/* Move to next x, y and yScanLine. */
		if (orientation == vgvSL_UP)
		{
			x = x1 + deltaX;
			intY++;
			yScanLine++;
#if SCAN_LINE_HAS_3D_SUPPORT
			Point1->lowerPointOnYLine = VG_TRUE;
#endif
		}
		else
		{
			x = x1 - deltaX;
			intY--;
			yScanLine--;
#if SCAN_LINE_HAS_3D_SUPPORT
			Point1->higherPointOnYLine = VG_TRUE;
#endif
		}
	}
	else
	{
		/* Calculate x coordinate at y. */
		x = x1 + COORD_MUL(y - y1, deltaX);
	}

	if (orientation == vgvSL_UP)
	{
		/* Get the last y. */
		if (Point2->yIsInteger)
		{
			y = y2;
		}
		else
		{
			y = COORD_FLOOR(y2);
		}
		intYLast = COORD_TO_INT(y);

		while (intY < intYLast)
		{
			/* Add point (x, y) to yScanLine. */
			gcmERR_RETURN(vgSL_AddXCoordToYScanLine(ScanLineTess, x, orientation,
											endLineStatus,
											Point1, yScanLine));
#if SCAN_LINE_HAS_3D_SUPPORT
			endLineStatus = 0;
#endif

			/* Move to next x, y and yScanLine. */
			x += deltaX;
			intY++;
			yScanLine++;
		}
		if (intY == intYLast)
		{
			/* Add end point to yScanLine. */
#if SCAN_LINE_HAS_3D_SUPPORT
			if (Point2->yIsInteger)
			{
				endLineStatus = 2;
				Point1->higherPointOnYLine = VG_TRUE;

				/* Snap x to x2 to avoid accuminated errors. */
				x = x2;
			}
			else
			{
				endLineStatus |= 2;
			}
#endif
			gcmERR_RETURN(vgSL_AddXCoordToYScanLine(ScanLineTess, x, orientation,
											endLineStatus,
											Point1, yScanLine));
		}
	}
	else if (orientation == vgvSL_DOWN)
	{
		/* Get the last y. */
		if (Point2->yIsInteger)
		{
			y = y2;
		}
		else
		{
			y = COORD_CEIL(y2);
		}
		intYLast = COORD_TO_INT(y);

		while (intY > intYLast)
		{
			/* Add point (x, y) to yScanLine. */
			gcmERR_RETURN(vgSL_AddXCoordToYScanLine(ScanLineTess, x, orientation,
											endLineStatus,
											Point1, yScanLine));
#if SCAN_LINE_HAS_3D_SUPPORT
			endLineStatus = 0;
#endif

			/* Move to next x, y and yScanLine. */
			x -= deltaX;
			intY--;
			yScanLine--;
		}
		if (intY == intYLast)
		{
			/* Add end point to yScanLine. */
#if SCAN_LINE_HAS_3D_SUPPORT
			if (Point2->yIsInteger)
			{
				endLineStatus = 2;
				Point1->lowerPointOnYLine = VG_TRUE;

				/* Snap x to x2 to avoid accuminated errors. */
				x = x2;
			}
			else
			{
				endLineStatus |= 2;
			}
#endif
			gcmERR_RETURN(vgSL_AddXCoordToYScanLine(ScanLineTess, x, orientation,
											endLineStatus,
											Point1, yScanLine));
		}
	}

	return status;
}

gceSTATUS
vgSL_BuildScanLineArray(
	vgSCANLINETESS		ScanLineTess,
	vgSL_SUBPATH		SubPathList
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_YSCANLINE		yScanLine;
	vgCOORD				y;
	VGint				minY, maxY;
	VGuint				numRows;
	vgSL_SUBPATH		subPath = gcvNULL;
	vgSL_POINT			point = gcvNULL;
	vgSL_POINT			prevPoint = gcvNULL;
	vgSL_POINT			firstProcessedPoint = gcvNULL;
	VGint				pointCount = 0;
	VGint				i;
	VGint				maxXCoordCount = 0;
	vgSL_XCOORD *		xcoordArray;

	/* Step 1: Set up yScanLineArray. */
	if (ScanLineTess->yScanLineArray == gcvNULL)
	{
		/* Get y range. */
		minY = ScanLineTess->yMin = (VGint) COORD_TO_INT(COORD_FLOOR(ScanLineTess->minYCoord));
		maxY = ScanLineTess->yMax = (VGint) COORD_TO_INT(COORD_CEIL(ScanLineTess->maxYCoord));
		ScanLineTess->yScanLineArraySize = numRows = maxY - minY + 1;

		/* Create yScanLineArray. */
		gcmERR_RETURN(vgSL_AllocateAYScanLineArray(ScanLineTess, &yScanLine, numRows));
		ScanLineTess->yScanLineArray = yScanLine;
		y = INT_TO_COORD(minY);
		for (i = minY; i <= maxY; i++, yScanLine++)
		{
			yScanLine->y = y;
			y += COORD_ONE;
		}
	}
	else
	{
		minY = ScanLineTess->yMin;
		maxY = ScanLineTess->yMax;
		numRows = ScanLineTess->yScanLineArraySize;

		/* Free existing xCoords. */
		yScanLine = ScanLineTess->yScanLineArray;
		for (i = minY; i <= maxY; i++, yScanLine++)
		{
			vgSL_XCOORD	xcoord = yScanLine->xcoords;
			vgSL_XCOORD	nextXcoord;

			if (xcoord == gcvNULL)
			{
				gcmASSERT(yScanLine->xcoordCount == 0);
				continue;
			}

			for (; xcoord; xcoord = nextXcoord)
			{
				nextXcoord = xcoord->next;
				vgSL_FreeAnXCoord(ScanLineTess, xcoord);
			}
			yScanLine->xcoords = gcvNULL;
			yScanLine->xcoordCount = 0;
		}
	}

	/* Step 2: Get x values for each y. */
	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		/* Skip no-area subpath. */
		if (subPath->pointCount < 3) continue;

		ScanLineTess->prevOrientation = vgvSL_FLAT;
		ScanLineTess->prevPrevOrientation = vgvSL_FLAT;

		/* Skip initial flat lines and first non-flat line. */
		for (point = subPath->pointList; point; prevPoint = point, point = point->next)
		{
			if (point->orientation)
			{
				/* Find the first non-flat line. */
				ScanLineTess->prevOrientation = point->orientation;
				break;
			}
		}
		gcmASSERT(point);
		prevPoint = point->next;
		point = prevPoint->next;
		firstProcessedPoint = point;
		for (; point; prevPoint = point, point = point->next)
		{
			gcmERR_RETURN(vgSL_AddPointsForLine(ScanLineTess, prevPoint, point));
			if (ScanLineTess->currentOrientation == vgvSL_FLAT &&
				ScanLineTess->prevOrientation != vgvSL_FLAT)
			{
				ScanLineTess->prevPrevOrientation = ScanLineTess->prevOrientation;
			}
			ScanLineTess->prevOrientation = ScanLineTess->currentOrientation;
		}

		/* Check if implicit closure is needed. */
		point = subPath->pointList;
		if (prevPoint->x != point->x || prevPoint->y != point->y)
		{
			/* Add the implicit closure line. */
			gcmERR_RETURN(vgSL_AddPointsForLine(ScanLineTess, prevPoint, point));
			if (ScanLineTess->currentOrientation == vgvSL_FLAT)
			{
				ScanLineTess->prevPrevOrientation = ScanLineTess->prevOrientation;
			}
			ScanLineTess->prevOrientation = ScanLineTess->currentOrientation;
#if SCAN_LINE_HAS_3D_SUPPORT
			/* Make the path a loop for easy traverse in triangulation. */
			prevPoint->next = point;
			point->prev = prevPoint;
		}
		else
		{
			/* Make the path a loop for easy traverse in triangulation. */
			gcmASSERT(prevPoint->prev && prevPoint->prev != point);
			prevPoint->prev->next = point;
			point->prev = prevPoint->prev;
			vgSL_FreeAPoint(ScanLineTess, prevPoint);
			subPath->pointCount--;
#endif
		}

		/* Process the initial flat lines and first non-flat line. */
		prevPoint = subPath->pointList;
		for (point = prevPoint->next; point != firstProcessedPoint; prevPoint = point, point = point->next)
		{
			gcmERR_RETURN(vgSL_AddPointsForLine(ScanLineTess, prevPoint, point));
			if (ScanLineTess->currentOrientation == vgvSL_FLAT &&
				ScanLineTess->prevOrientation != vgvSL_FLAT)
			{
				ScanLineTess->prevPrevOrientation = ScanLineTess->prevOrientation;
			}
			ScanLineTess->prevOrientation = ScanLineTess->currentOrientation;
		}

#if SCAN_LINE_HAS_3D_SUPPORT
		/* Merge the overcrowded lines between crossing integer y lines. */
		/* Make sure at most two points between crossing integer y lines. */
		/* That means no continuous lines with 0 crossYLineCount. */

		/* Find the first non-zero-cross-y-lines line. */
		point = subPath->pointList;
		if (point->crossYLinesCount == 0)
		{
			for (point = point->next; point != subPath->pointList; point = point->next)
			{
				if (point->crossYLinesCount) break;
			}

			if (point == subPath->pointList)
			{
				/* TODO - The subPath is too small.  Remove it all. */
				gcmASSERT(point != subPath->pointList);
			}
		}
		firstProcessedPoint = prevPoint = point;
		for (point = prevPoint->next; point != firstProcessedPoint; point = point->next)
		{
			if (point->crossYLinesCount || prevPoint->crossYLinesCount)
			{
				prevPoint = point;
			}
			else
			{
				/* Merge point into prevPoint. */
				vgCOORD deltaY = point->next->y - prevPoint->y;
				prevPoint->orientation = deltaY >= COORD_EPSILON ? vgvSL_UP :
										 deltaY <= -COORD_EPSILON ? vgvSL_DOWN : vgvSL_FLAT;
				if (prevPoint->orientation)
				{
					prevPoint->deltaX = COORD_DIV(point->next->x - prevPoint->x, deltaY);
				}
				else
				{
					prevPoint->deltaX = COORD_MAX;
				}

				/* Delete point from subPath. */
				if (point == subPath->pointList) subPath->pointList = point->next;
				point->next->prev = prevPoint;
				prevPoint->next = point->next;
				vgSL_FreeAPoint(ScanLineTess, point);
				subPath->pointCount--;
			}
		}


		pointCount += subPath->pointCount;
#endif
	}
#if SCAN_LINE_HAS_3D_SUPPORT
	ScanLineTess->pointCount = pointCount;
#endif

	/* Step 3: Sort xCoords in each yScanLine. */
	yScanLine = ScanLineTess->yScanLineArray;
	for (i = minY; i <= maxY; i++, yScanLine++)
	{
		if (yScanLine->xcoordCount > maxXCoordCount)
		{
			maxXCoordCount = yScanLine->xcoordCount;
		}
	}
	if (maxXCoordCount < 2) return gcvSTATUS_OK;

	yScanLine = ScanLineTess->yScanLineArray;
	gcmERR_RETURN(gcoOS_Allocate(ScanLineTess->os,
							maxXCoordCount * gcmSIZEOF(vgSL_XCOORD),
							(gctPOINTER *) &xcoordArray));
	for (i = minY; i <= maxY; i++, yScanLine++)
	{
		if (yScanLine->xcoordCount == 0) continue;
		vgSL_sortXCoords(yScanLine, xcoordArray);
	}
	gcmVERIFY_OK(gcoOS_Free(ScanLineTess->os, xcoordArray));

	return status;
}

gceSTATUS
vgSL_CreateScanLines(
	vgSCANLINETESS		ScanLineTess,
	_VGVertexBuffer *	ScanLineBuffer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	VGint				numLines = 0;
	vgSL_YSCANLINE		yScanLine;
	VGint				i;
	VGint				bufferSize;
	_VGScanLine *		hLine;

	/* Count the number of lines. */
	yScanLine = ScanLineTess->yScanLineArray;
	for (i = ScanLineTess->yMin; i <= ScanLineTess->yMax; i++, yScanLine++)
	{
		vgSL_XCOORD xCoord1 = yScanLine->xcoords;
		vgSL_XCOORD xCoord2;
		VGint fillCount = 0;

		if (xCoord1 == gcvNULL)
		{
			gcmASSERT(yScanLine->xcoordCount == 0);
			continue;
		}

		/* Check if the orientation of the first point is vgvSL_FLAT. */
		if (xCoord1->orientation == vgvSL_FLAT)
		{
			xCoord2 = xCoord1->next;
			if (xCoord2 == gcvNULL)
			{
				gcmASSERT(xCoord2);
				continue;
			}
			/* Switch orientation for xCoord1 and xCoord2. */
			/* This will fill the horizontal line. */
			gcmASSERT(xCoord2->orientation != 0);
			xCoord1->orientation = xCoord2->orientation;
			xCoord2->orientation = 0;
		}

		/* Count the number of lines in yScanLine. */
		if (ScanLineTess->fillRule == VG_EVEN_ODD)
		{
			/* VG_EVEN_ODD rule. */
			while (xCoord1)
			{
				fillCount += xCoord1->orientation;
				xCoord2 = xCoord1->next;
				if (xCoord2 == gcvNULL) break;

				if (xCoord2->orientation == 0 && fillCount == 0)
				{
					/* Switch orientation for xCoord1 and xCoord2. */
					/* This will fill the horizontal line. */
					gcmASSERT(xCoord1->orientation != 0);
					fillCount -= xCoord1->orientation;
					xCoord2->orientation = xCoord1->orientation;
					xCoord1->orientation = 0;
				}
				if ((VGuint) fillCount & 0x1)
				{
					numLines++;
				}

				xCoord1 = xCoord2;
			}
		}
		else
		{
			/* VG_NON_ZERO rule. */
			while (xCoord1)
			{
				fillCount += xCoord1->orientation;
				xCoord2 = xCoord1->next;
				if (xCoord2 == gcvNULL) break;

				if (xCoord2->orientation == 0 && fillCount == 0)
				{
					/* Switch orientation for xCoord1 and xCoord2. */
					/* This will fill the horizontal line. */
					gcmASSERT(xCoord1->orientation != 0);
					fillCount -= xCoord1->orientation;
					xCoord2->orientation = xCoord1->orientation;
					xCoord1->orientation = 0;
				}
				if (fillCount != 0)
				{
					numLines++;
				}

				xCoord1 = xCoord2;
			}
		}
		gcmASSERT(fillCount == 0);
	}

	if (ARRAY_SIZE(ScanLineBuffer->data) > 0)
	{
		/* Free previous scan line data. */
		ARRAY_CLEAR(ScanLineBuffer->data);
	}

	if (numLines == 0) return gcvSTATUS_OK /*gcvSTATUS_INVALID_DATA*/;

	/* Allocate the scan line array. */
	bufferSize = numLines * sizeof(_VGScanLine);
	ARRAY_RESIZE(ScanLineBuffer->data, bufferSize);
#if !USE_SCAN_LINE_ON_RI
	if (ScanLineBuffer->data.allocated == 0) return gcvSTATUS_OUT_OF_MEMORY;
#endif
	hLine = (_VGScanLine *) &ARRAY_ITEM(ScanLineBuffer->data, 0);

	/* Fill the array. */
	yScanLine = ScanLineTess->yScanLineArray;
	if (ScanLineTess->fillRule == VG_EVEN_ODD)
	{
		/* VG_EVEN_ODD rule. */
		for (i = ScanLineTess->yMin; i <= ScanLineTess->yMax; i++, yScanLine++)
		{
			vgSL_XCOORD xCoord1 = yScanLine->xcoords;
			vgSL_XCOORD xCoord2;
			VGint fillCount = 0;

			while (xCoord1)
			{
				fillCount += xCoord1->orientation;
				xCoord2 = xCoord1->next;
				if (xCoord2 == gcvNULL) break;

				if ((VGuint) fillCount & 0x1)
				{
					/* Fill the horizontal fill line info. */
#if USE_NEW_SCAN_LINE_FORMAT
#if USE_FIXED_POINT_FOR_COORDINATES
					hLine->x1 = xCoord1->x >> (FRACTION_BITS - SCANLINE_FRACTION_BITS);
					hLine->x2 = xCoord2->x >> (FRACTION_BITS - SCANLINE_FRACTION_BITS);
					hLine->y  = yScanLine->y >> FRACTION_BITS;
#else
					hLine->x1 = COORD_TO_INT(xCoord1->x * 256.0);
					hLine->x2 = COORD_TO_INT(xCoord2->x * 256.0);
					hLine->y  = COORD_TO_INT(yScanLine->y);
#endif
#else
					hLine->x1 = COORD_TO_FLOAT(xCoord1->x);
					hLine->y1 = COORD_TO_FLOAT(yScanLine->y);
					hLine->x2 = COORD_TO_FLOAT(xCoord2->x);
					hLine->y2 = COORD_TO_FLOAT(yScanLine->y);
#endif
					hLine++;
				}
				xCoord1 = xCoord2;
			}
		}
	}
	else
	{
		/* VG_NON_ZERO rule. */
		for (i = ScanLineTess->yMin; i <= ScanLineTess->yMax; i++, yScanLine++)
		{
			vgSL_XCOORD xCoord1 = yScanLine->xcoords;
			vgSL_XCOORD xCoord2;
			VGint fillCount = 0;

			while (xCoord1)
			{
				fillCount += xCoord1->orientation;
				xCoord2 = xCoord1->next;
				if (xCoord2 == gcvNULL) break;

				if (fillCount != 0)
				{
					/* Fill the horizontal fill line info. */
#if USE_NEW_SCAN_LINE_FORMAT
#if USE_FIXED_POINT_FOR_COORDINATES
					hLine->x1 = xCoord1->x >> (FRACTION_BITS - SCANLINE_FRACTION_BITS);
					hLine->x2 = xCoord2->x >> (FRACTION_BITS - SCANLINE_FRACTION_BITS);
					hLine->y  = yScanLine->y >> FRACTION_BITS;
#else
					hLine->x1 = COORD_TO_INT(xCoord1->x * 256.0);
					hLine->x2 = COORD_TO_INT(xCoord2->x * 256.0);
					hLine->y  = COORD_TO_INT(yScanLine->y);
#endif
#else
					hLine->x1 = COORD_TO_FLOAT(xCoord1->x);
					hLine->y1 = COORD_TO_FLOAT(yScanLine->y);
					hLine->x2 = COORD_TO_FLOAT(xCoord2->x);
					hLine->y2 = COORD_TO_FLOAT(yScanLine->y);
#endif
					hLine++;
				}

				xCoord1 = xCoord2;
			}
		}
	}
	gcmASSERT(hLine - (_VGScanLine *) &ARRAY_ITEM(ScanLineBuffer->data, 0) == numLines);

#if !USE_SCAN_LINE_ON_RI
#if USE_NEW_SCAN_LINE_FORMAT
	/* TODO - May need to adjust the values for the new VG core. */
	ScanLineBuffer->size = 1;
	ScanLineBuffer->type = gcvVERTEX_FIXED;
	ScanLineBuffer->stride = sizeof(_VGScanLine);
#else
	ScanLineBuffer->size = 2;
	ScanLineBuffer->type = gcvVERTEX_FLOAT;
	ScanLineBuffer->stride = sizeof(_VGScanLine) / 2;
#endif
	ScanLineBuffer->normalized = gcvFALSE;
	ScanLineBuffer->elementCount = numLines;
#endif

	return status;
}
#endif

#if SCAN_LINE_HAS_3D_SUPPORT
INLINE void
vgSL_GetEndPointOrHightEndPoint(
	vgSL_XCOORD			XCoord,
	vgSL_POINT *		Point
	)
{
	*Point = XCoord->endLineStatus == 1 ? XCoord->line :
			 XCoord->endLineStatus == 2 ? XCoord->line->next :
			 XCoord->orientation == vgvSL_DOWN ? XCoord->line :
			 XCoord->line->next;
}

INLINE void
vgSL_GetEndPointOrLowerEndPoint(
	vgSL_XCOORD			XCoord,
	vgSL_POINT *		Point
	)
{
	*Point = XCoord->endLineStatus == 1 ? XCoord->line :
			 XCoord->endLineStatus == 2 ? XCoord->line->next :
			 XCoord->orientation == vgvSL_UP ? XCoord->line :
			 XCoord->line->next;
}

void
vgSL_GetIntersection(
	vgCOORD				X1,
	vgCOORD				X2,
	vgCOORD				X3,
	vgCOORD				X4,
	vgCOORD				Y1,
	vgCOORD *			X,
	vgCOORD *			Y
	)
{
	vgCOORD l1 = X2 - X1;
	vgCOORD l2 = X4 - X3;
	vgCOORD dY;

	//gcmASSERT(l1 >= COORD_EPSILON);
	//gcmASSERT(l2 >= COORD_EPSILON);
	dY = COORD_DIV(l1, l1 + l2);
	*Y = Y1 + dY;
	*X = X1 + COORD_MUL(dY, X4 - X1);
}

void
vgSL_GetIntersectionW2Y(
	vgCOORD				X1,
	vgCOORD				X2,
	vgCOORD				X3,
	vgCOORD				X4,
	vgCOORD				Y1,
	vgCOORD				Y2,
	vgCOORD *			X,
	vgCOORD *			Y
	)
{
	vgCOORD l1 = X2 - X1;
	vgCOORD l2 = X4 - X3;
	vgCOORD dY;

	//gcmASSERT(l1 >= COORD_EPSILON);
	//gcmASSERT(l2 >= COORD_EPSILON);
	dY = COORD_MUL(COORD_DIV(l1, l1 + l2), Y2 - Y1);
	*Y = Y1 + dY;
	*X = X1 + COORD_MUL(dY, X4 - X1);
}

void
vgSL_GetXCoords(
	vgSL_REGION			Region,
	vgSL_YSCANLINE		YScanLine,
	vgSL_XCOORD *		XCoord1,
	vgSL_XCOORD *		XCoord2
	)
{
	vgSL_XCOORD xCoord = YScanLine->xcoords;

	while (xCoord)
	{
		if (xCoord->line == Region->leftLine)
		{
			*XCoord1 = xCoord;
			xCoord = xCoord->next;
			if (Region->rightLine == xCoord->line)
			{
				*XCoord2 = xCoord;
			}
			return;
		}
		xCoord = xCoord->next;
	}

	/* No cross point for YScanLine and Region->leftLine. */
	/* This is due to rightLine advancement for zero-cross-yScanLine lines. */
	/* Check Region->rightLine. */
	*XCoord1 = gcvNULL;
	xCoord = YScanLine->xcoords;
	while (xCoord)
	{
		if (xCoord->line == Region->rightLine)
		{
			*XCoord2 = xCoord;
			return;
		}
		xCoord = xCoord->next;
	}
	*XCoord2 = gcvNULL;
}

vgCOORD
vgSL_GetDeltaX(
	vgSL_POINT			Point1,
	vgSL_POINT			Point2
	)
{
	vgCOORD				deltaY = Point2->y - Point1->y;

	if (COORD_ABS(deltaY) < COORD_EPSILON) return COORD_MAX;

	return COORD_DIV(Point2->x - Point1->x, deltaY);
}

gceSTATUS
vgSL_AddACrossPoint(
	vgSCANLINETESS		ScanLineTess,
	vgCOORD				X,
	vgCOORD				Y,
	vgSL_POINT			Line1,
	vgSL_POINT			Line2,
	vgSL_POINT *		Point
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_POINT			point;
	vgSL_REGIONPOINT	rPoint;

	gcmERR_RETURN(vgSL_AllocateAPoint(ScanLineTess, &point));

	point->x = X;
	point->y = Y;
	point->orientation = vgvSL_CROSSPOINT;

	/* Add point to crossPointList of Line1 and Line2. */
	gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
	rPoint->point = point;
	rPoint->next = Line1->crossPointList;
	Line1->crossPointList = rPoint;
	gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
	rPoint->point = point;
	rPoint->next = Line2->crossPointList;
	Line2->crossPointList = rPoint;

	/* Add Line1 and Line2 to point's crossPointList (cross line list). */
	gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
	rPoint->point = Line1;
	point->crossPointList = rPoint;
	gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
	rPoint->point = Line2;
	point->crossPointList->next = rPoint;

	/* Add to ScanLineTess's crossPointList. */
	point->next = ScanLineTess->crossPointList;
	ScanLineTess->crossPointList = point;
	ScanLineTess->crossPointCount++;

	*Point = point;
	return gcvSTATUS_OK;
}

void
vgSL_GetCrossPoint(
	vgSL_XCOORD			XCoord,
	vgSL_POINT *		Point,
	vgSL_YSCANLINE		YScanLine
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_POINT			line = XCoord->line;
	vgSL_REGIONPOINT	rPoint = line->crossPointList;

	gcmASSERT(rPoint);

	if (rPoint == gcvNULL)
	{
		*Point = gcvNULL;
		return;
	}
	*Point = rPoint->point;
	gcmASSERT((*Point)->y <= YScanLine->y);
	for (rPoint = rPoint->next; rPoint; rPoint = rPoint->next)
	{
		gcmASSERT(rPoint->point->y <= YScanLine->y);
		if (rPoint->point->y > (*Point)->y) *Point = rPoint->point;
	}
	gcmASSERT((*Point)->y >= YScanLine->y - COORD_ONE);
}

gceSTATUS
vgSL_AddATriangle(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point1,
	vgSL_POINT			Point2,
	vgSL_POINT			Point3
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_TRIANGLE		triangle;

	gcmERR_RETURN(vgSL_AllocateATriangle(ScanLineTess, &triangle));

	triangle->point1 = Point1;
	triangle->point2 = Point2;
	triangle->point3 = Point3;
	triangle->next   = ScanLineTess->triangleList;
	ScanLineTess->triangleList = triangle;
	ScanLineTess->triangleCount++;

	return gcvSTATUS_OK;
}

INLINE void
vgSL_SetRegionLeftLine(
	vgSL_REGION			Region,
	vgSL_POINT			Line
	)
{
	if (Region->leftLine) Region->leftLine->regionWLeft = gcvNULL;
	Region->leftLine	= Line;
	if (Line)
	{
		Line->regionWLeft	= Region;
		Region->leftDeltaX	= Line->deltaX;
		if (Line->orientation == vgvSL_FLAT)
		{
			/* Check if the line go west. */
			if (Region->leftPointList)
			{
				vgCOORD x = Region->leftPointList->point->x;
				if (Line->x < x || Line->next->x < x)
				{
					Region->leftDeltaX = COORD_MIN;
				}
				else
				{
					gcmASSERT(Region->leftDeltaX == COORD_MAX);
				}
			}
			else
			{
				gcmASSERT(Region->rightPointList);
			}
		}
	}
}

INLINE void
vgSL_SetRegionRightLine(
	vgSL_REGION			Region,
	vgSL_POINT			Line
	)
{
	if (Region->rightLine) Region->rightLine->regionWRight = gcvNULL;
	Region->rightLine	= Line;
	if (Line)
	{
		Line->regionWRight	= Region;
		Region->rightDeltaX	= Line->deltaX;
		if (Line->orientation == vgvSL_FLAT)
		{
			/* Check if the line go west. */
			if (Region->rightPointList)
			{
				vgCOORD x = Region->rightPointList->point->x;
				if (Line->x < x || Line->next->x < x)
				{
					Region->rightDeltaX = COORD_MIN;
				}
				else
				{
					gcmASSERT(Region->rightDeltaX == COORD_MAX);
				}
			}
			else
			{
				gcmASSERT(Region->rightPointList);
			}
		}
	}
}

gceSTATUS
vgSL_SetRegionBottom(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point1,
	vgSL_POINT			Point2,
	vgSL_REGION			Region
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

	/* Check if it is one-point bottom. */
	if (Point1 == Point2)
	{
		/* One point bottom. */
		Region->bottomPoint = Point1;
		Region->leftPointList = Region->rightPointList = gcvNULL;
	}
	else
	{
		vgSL_REGIONPOINT rPoint;
		vgCOORD deltaY = Point2->y - Point1->y;

		/* TODO - Check to make sure no extra lines between point1 and point2. */
		//gcmASSERT(Point1 == Point2->next || Point2 == Point1->next);

		if (COORD_ABS(deltaY) < COORD_EPSILON)
		{
			/* Flat bottom. */
			Region->bottomPoint = gcvNULL;

			/* Initiate left branch. */
			gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
			rPoint->point = Point1;
			rPoint->deltaX = COORD_MIN;
			Region->leftPointList = rPoint;

			/* Initiate right branch. */
			gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
			rPoint->point = Point2;
			rPoint->deltaX = COORD_MAX;
			Region->rightPointList = rPoint;
		}
		else
		{
			vgCOORD deltaX = COORD_DIV(Point2->x - Point1->x, deltaY);
			gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
			rPoint->deltaX = deltaX;

			if (deltaY > COORD_ZERO)
			{
				/* Point1 is bottom, and point2 is in right branch. */
				Region->bottomPoint = Point1;
				rPoint->point = Point2;
				Region->rightPointList = rPoint;
				Region->leftPointList = gcvNULL;
			}
			else
			{
				/* Point2 is bottom, and point1 is in left branch. */
				Region->bottomPoint = Point2;
				rPoint->point = Point1;
				Region->leftPointList = rPoint;
				Region->rightPointList = gcvNULL;
			}
		}
	}

	return status;
}

gceSTATUS
vgSL_AddARegion(
	vgSCANLINETESS		ScanLineTess,
	vgSL_YSCANLINE		YScanLine,
	vgSL_XCOORD			XCoord1,
	vgSL_XCOORD			XCoord2,
	vgSL_REGION			NextRegion
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_POINT			line1 = XCoord1->line;
	vgSL_POINT			line2 = XCoord2->line;
	vgSL_POINT			point1, point2;
	vgSL_REGION			region;

	/* Add a new region to regionList. */
	gcmERR_RETURN(vgSL_AllocateARegion(ScanLineTess, &region));

	if (XCoord1->endLineStatus)
	{
		if (line1->orientation == vgvSL_UP)
		{
			point1 = line1;
			region->leftOrientation = vgvSL_UP;
			region->isLeftNext = VG_TRUE;
		}
		else
		{
			gcmASSERT(line1->orientation == vgvSL_DOWN);
			point1 = line1->next;
			region->leftOrientation = vgvSL_DOWN;
			region->isLeftNext = VG_FALSE;
		}
	}
	else
	{
		gcmASSERT(line1->crossPointList);
		vgSL_GetCrossPoint(XCoord1, &point1, YScanLine);
		gcmASSERT(point1);
		region->leftOrientation = line1->orientation;
		region->isLeftNext = (line1->orientation == vgvSL_UP);
	}

	if (XCoord2->endLineStatus)
	{
		if (line2->orientation == vgvSL_UP)
		{
			point2 = line2;
			region->rightOrientation = vgvSL_UP;
			region->isRightNext = VG_TRUE;
		}
		else
		{
			gcmASSERT(line2->orientation == vgvSL_DOWN);
			point2 = line2->next;
			region->rightOrientation = vgvSL_DOWN;
			region->isRightNext = VG_FALSE;
		}
	}
	else
	{
		gcmASSERT(line2->crossPointList);
		vgSL_GetCrossPoint(XCoord2, &point2, YScanLine);
		gcmASSERT(point2);
		region->rightOrientation = line2->orientation;
		region->isRightNext = (line1->orientation == vgvSL_UP);
	}

	/* TODO - Check if there is a cross point here. */
	/* gcmASSERT(point1->x <= point2->x);  or no cross point */

	/* Set region bottom. */
	gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, point1, point2, region));

	/* Set left and right branches. */
	vgSL_SetRegionLeftLine(region, line1);
	vgSL_SetRegionRightLine(region, line2);

	if (NextRegion)
	{
		region->prev = NextRegion->prev;
		region->next = NextRegion;
		if (NextRegion->prev)
		{
			NextRegion->prev->next = region;
		}
		else
		{
			ScanLineTess->regionList = region;
		}
		NextRegion->prev = region;
	}
	else
	{
		if (ScanLineTess->lastRegion)
		{
			ScanLineTess->lastRegion->next = region;
			region->prev = ScanLineTess->lastRegion;
		}
		else
		{
			ScanLineTess->regionList = region;
		}
		ScanLineTess->lastRegion = region;
	}

	return status;
}

gceSTATUS
vgSL_DeleteARegion(
	vgSCANLINETESS		ScanLineTess,
	vgSL_REGION			Region
	)
{
	if (Region->prev)
	{
		Region->prev->next = Region->next;
	}
	else
	{
		ScanLineTess->regionList = Region->next;
	}
	if (Region->next)
	{
		Region->next->prev = Region->prev;
	}
	else
	{
		ScanLineTess->lastRegion = Region->prev;
	}
	vgSL_FreeARegion(ScanLineTess, Region);

	return gcvSTATUS_OK;
}

gceSTATUS
vgSL_AddAPointToRegionLeftBranch(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point,
	vgSL_REGION			Region
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgCOORD				deltaX = Region->leftDeltaX;
	vgSL_REGIONPOINT	newRPoint;

	gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &newRPoint));
	newRPoint->point = Point;

	gcmASSERT(Point->orientation == vgvSL_CROSSPOINT ||
			  Point == Region->leftLine || Point == Region->leftLine->next);
	if (Region->rightPointList)
	{
		/* Create one triangle for each point in rightPointList. */
		vgSL_REGIONPOINT rPoint, nextRPoint;
		vgSL_POINT point1, point2, point3;

		rPoint = Region->rightPointList;
		point1 = point2 = rPoint->point;
		for (; rPoint; rPoint = nextRPoint)
		{
			nextRPoint = rPoint->next;
			if (nextRPoint)
			{
				point3 = nextRPoint->point;
			}
			else if (Region->bottomPoint)
			{
				gcmASSERT(Region->leftPointList == gcvNULL);
				point3 = Region->bottomPoint;
			}
			else
			{
				gcmASSERT(Region->leftPointList);
				gcmASSERT(Region->leftPointList->next == gcvNULL);
				point3 = Region->leftPointList->point;
				vgSL_FreeARegionPoint(ScanLineTess, Region->leftPointList);
				Region->leftPointList = gcvNULL;
			}
			gcmERR_RETURN(vgSL_AddATriangle(ScanLineTess, Point, point2, point3));
			point2 = point3;
			vgSL_FreeARegionPoint(ScanLineTess, rPoint);
		}
		if (Region->rightLine)
		{
			Region->rightPointList = gcvNULL;
			Region->bottomPoint = point1;
			deltaX = vgSL_GetDeltaX(point1, Point);
		}
		else
		{
			/* Merge with next region. */
			vgSL_REGION nextRegion = Region->next;
			gcmASSERT(nextRegion);
			gcmASSERT(nextRegion->leftLine == gcvNULL);
			gcmASSERT(nextRegion->leftPointList->point == point1);
			Region->rightPointList = nextRegion->rightPointList;
			Region->leftPointList = nextRegion->leftPointList;
			Region->bottomPoint = nextRegion->bottomPoint;
			vgSL_SetRegionRightLine(Region, nextRegion->rightLine);

			/* Delete nextRegion. */
			vgSL_DeleteARegion(ScanLineTess, nextRegion);
			deltaX = vgSL_GetDeltaX(point1, Point);
		}
	}

	/* Need to check leftPointList again if merged with next region. */
	if (Region->leftPointList)
	{
		/* Create one triangle for each point in leftPointList that is convex. */
		vgSL_REGIONPOINT rPoint, nextRPoint;
		vgSL_POINT point2, point3;

		rPoint = Region->leftPointList;
		point2 = rPoint->point;
		/* TODO - may need to handle deltaX == rPoint->deltaX. */
		while (rPoint && deltaX > rPoint->deltaX)
		{
			nextRPoint = rPoint->next;
			if (nextRPoint)
			{
				point3 = nextRPoint->point;
			}
			else
			{
				gcmASSERT(Region->bottomPoint);
				point3 = Region->bottomPoint;
			}
			gcmERR_RETURN(vgSL_AddATriangle(ScanLineTess, Point, point2, point3));
			deltaX = vgSL_GetDeltaX(point3, Point);
			point2 = point3;
			vgSL_FreeARegionPoint(ScanLineTess, rPoint);
			rPoint = Region->leftPointList = nextRPoint;
		}
	}
	newRPoint->deltaX = deltaX;
	newRPoint->next = Region->leftPointList;
	Region->leftPointList = newRPoint;

	if (Point->orientation == vgvSL_CROSSPOINT)
	{
		/* This is a cross point, so don't change the left branch. */
		/* It will be updated by caller. */
		return status;
	}

	/* Find next left line. */
	if (! Region->isLeftNext) Point = Point->prev;

	if (Point->orientation == Region->leftOrientation ||
		Point->orientation == vgvSL_FLAT)
	{
		vgSL_SetRegionLeftLine(Region, Point);
	}
	else
	{
		vgSL_SetRegionLeftLine(Region, gcvNULL);
		Region->leftDeltaX = COORD_MAX;
	}

	return status;
}

gceSTATUS
vgSL_AddAPointToRegionRightBranch(
	vgSCANLINETESS		ScanLineTess,
	vgSL_POINT			Point,
	vgSL_REGION			Region
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgCOORD				deltaX = Region->rightDeltaX;
	vgSL_REGIONPOINT	newRPoint;
	VGboolean			rightPointListIsProcessed = VG_FALSE;

	gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &newRPoint));
	newRPoint->point = Point;

	gcmASSERT(Point->orientation == vgvSL_CROSSPOINT ||
			  Point == Region->rightLine || Point == Region->rightLine->next);
	if (Region->leftPointList)
	{
		/* Create one triangle for each point in leftPointList. */
		vgSL_REGIONPOINT rPoint, nextRPoint;
		vgSL_POINT point1, point2, point3;
		vgCOORD deltaY;

		rPoint = Region->leftPointList;
		point1 = point2 = rPoint->point;
		for (; rPoint; rPoint = nextRPoint)
		{
			nextRPoint = rPoint->next;
			if (nextRPoint)
			{
				point3 = nextRPoint->point;
			}
			else if (Region->bottomPoint)
			{
				gcmASSERT(Region->rightPointList == gcvNULL);
				point3 = Region->bottomPoint;
			}
			else
			{
				gcmASSERT(Region->rightPointList);
				gcmASSERT(Region->rightPointList->next == gcvNULL);
				point3 = Region->rightPointList->point;
				vgSL_FreeARegionPoint(ScanLineTess, Region->rightPointList);
				Region->rightPointList = gcvNULL;
			}
			gcmERR_RETURN(vgSL_AddATriangle(ScanLineTess, Point, point2, point3));
			point2 = point3;
			vgSL_FreeARegionPoint(ScanLineTess, rPoint);
		}
		gcmASSERT(Region->rightPointList == gcvNULL);

		/* Check deltaY for Point and point1. */
		deltaY = Point->y - point1->y;
		if (deltaY >= COORD_EPSILON)
		{
			/* point1 becomes bottom point. */
			Region->leftPointList = gcvNULL;
			Region->bottomPoint = point1;
		}
		else if (deltaY <= -COORD_EPSILON)
		{
			/* Point become bottom point -- add point1 back to leftPointList. */
			Region->bottomPoint = Point;

			/* Reuse newRPoint for point1. */
			newRPoint->point = point1;
			newRPoint->deltaX = vgSL_GetDeltaX(Point, point1);
			Region->leftPointList = newRPoint;

			/* Reset newRPoint for later handling. */
			newRPoint = gcvNULL;
		}
		else
		{
			/* Flat bottome. */
			Region->bottomPoint = gcvNULL;
			gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
			rPoint->point = point1;
			Region->leftPointList = rPoint;
		}

		/* Check if it is necessary to merge with prev region. */
		if (Region->leftLine == gcvNULL && Region->prev && Region->prev->rightLine == gcvNULL)
		{
			/* Check if it is necessary to merge with prev region. */
			vgSL_REGION prevRegion = Region->prev;
			if (prevRegion->rightPointList == gcvNULL ||
				prevRegion->bottomPoint == gcvNULL ||
				Region->leftPointList == gcvNULL ||
				Region->bottomPoint == gcvNULL)
			{
				/* Need to merge with prev region. */
				gcmASSERT(Region->isLeftNext == prevRegion->isLeftNext);
				gcmASSERT(Region->isRightNext == prevRegion->isRightNext);
				if (Region->leftPointList == gcvNULL)
				{
					/* Region is higher than prevRegion. */
					gcmASSERT(Region->bottomPoint == point1);

					/* Copy data from prevRegion to Region. */
					Region->bottomPoint = prevRegion->bottomPoint;
					Region->leftPointList = prevRegion->leftPointList;
					Region->rightPointList = prevRegion->rightPointList;
					vgSL_SetRegionLeftLine(Region, prevRegion->leftLine);
				}
				else if (Region->bottomPoint == gcvNULL)
				{
					/* Region has flat bottom. */
					if (prevRegion->bottomPoint == gcvNULL)
					{
						/* prevRegion also has flat bottom. */
						/* Region has flat bottom after merging prevRegion. */
						gcmASSERT(Region->leftPointList->point == point1);
						gcmASSERT(prevRegion->rightPointList->point == point1);
						vgSL_FreeARegionPoint(ScanLineTess, Region->leftPointList);
						vgSL_FreeARegionPoint(ScanLineTess, prevRegion->rightPointList);
						Region->leftPointList = prevRegion->leftPointList;
						vgSL_SetRegionLeftLine(Region, prevRegion->leftLine);
					}
					else if (prevRegion->rightPointList == gcvNULL)
					{
						/* prevRegion is higher. */
						gcmASSERT(prevRegion->bottomPoint == point1);
						gcmASSERT(prevRegion->leftPointList);
						gcmASSERT(prevRegion->leftPointList->next == gcvNULL);

						/* Fake leftLine and leftDeltaX to add point to Region. */
						Region->leftLine = prevRegion->leftPointList->point;
						Region->leftDeltaX = prevRegion->leftPointList->deltaX;
						gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess,
									prevRegion->leftPointList->point, Region));
						vgSL_FreeARegionPoint(ScanLineTess, prevRegion->leftPointList);
					}
					else
					{
						/* Region is higher. */
						gcmASSERT(prevRegion->rightPointList->point == point1);
						vgSL_FreeARegionPoint(ScanLineTess, Region->leftPointList);

						/* Copy data from prevRegion to Region. */
						Region->bottomPoint = prevRegion->bottomPoint;
						Region->leftPointList = prevRegion->leftPointList;
						Region->rightPointList = prevRegion->rightPointList;
						vgSL_SetRegionLeftLine(Region, prevRegion->leftLine);
					}
				}
				else
				{
					/* prevRegion is higher. */
					gcmASSERT(prevRegion->leftPointList);
					gcmASSERT(prevRegion->leftPointList->next == gcvNULL);
					if (prevRegion->rightPointList)
					{
						/* prevRegion has flat bottom. */
						gcmASSERT(prevRegion->rightPointList->point == point1);
						gcmASSERT(prevRegion->rightPointList->next == gcvNULL);
						vgSL_FreeARegionPoint(ScanLineTess, prevRegion->rightPointList);
						prevRegion->rightPointList = gcvNULL;
					}
					else
					{
						gcmASSERT(prevRegion->bottomPoint == point1);
					}

					/* Fake leftLine and leftDeltaX to add point to Region. */
					Region->leftLine = prevRegion->leftPointList->point;
					Region->leftDeltaX = prevRegion->leftPointList->deltaX;
					gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess,
							prevRegion->leftPointList->point, Region));
					vgSL_FreeARegionPoint(ScanLineTess, prevRegion->leftPointList);
				}

				/* Remove prevRegion. */
				vgSL_DeleteARegion(ScanLineTess, prevRegion);

				/* Calculate new deltaX. */
				deltaX = vgSL_GetDeltaX(point1, Point);
			}
		}
	}

	/* Need to check rightPointList again if merged with prev region. */
	if (Region->rightPointList)
	{
		/* Create one triangle for each point in leftPointList that is convex. */
		vgSL_REGIONPOINT rPoint, nextRPoint;
		vgSL_POINT point2, point3;

		rPoint = Region->rightPointList;
		point2 = rPoint->point;

		/* TODO - may need to handle deltaX == rPoint->deltaX. */

		while (rPoint && deltaX < rPoint->deltaX)
		{
			nextRPoint = rPoint->next;
			if (nextRPoint)
			{
				point3 = nextRPoint->point;
			}
			else
			{
				gcmASSERT(Region->bottomPoint);
				point3 = Region->bottomPoint;
			}
			gcmERR_RETURN(vgSL_AddATriangle(ScanLineTess, Point, point2, point3));
			deltaX = vgSL_GetDeltaX(point3, Point);
			point2 = point3;
			vgSL_FreeARegionPoint(ScanLineTess, rPoint);
			rPoint = Region->rightPointList = nextRPoint;
		}
	}
	if (newRPoint)
	{
		newRPoint->deltaX = deltaX;
		newRPoint->next = Region->rightPointList;
		Region->rightPointList = newRPoint;
	}
	else
	{
		/* Special case when Point is lower than point1. */
		gcmASSERT(Region->bottomPoint == Point);
	}

	if (Point->orientation == vgvSL_CROSSPOINT)
	{
		/* This is a cross point, so don't change the right branch. */
		/* It will be updated by caller. */
		return status;
	}

	/* Find next right line. */
	if (! Region->isRightNext) Point = Point->prev;

	if (Point->orientation == Region->rightOrientation ||
		Point->orientation == vgvSL_FLAT)
	{
		vgSL_SetRegionRightLine(Region, Point);
	}
	else
	{
		vgSL_SetRegionRightLine(Region, gcvNULL);
		Region->rightDeltaX = COORD_MAX;
	}

	return status;
}

gceSTATUS
vgSL_SplitARegion(
	vgSCANLINETESS		ScanLineTess,
	vgSL_XCOORD			XCoord,
	vgSL_REGION			Region,
	VGboolean			UseToBeMergedRegion
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_XCOORD			xCoord2 = XCoord;
	vgSL_XCOORD			xCoord1;
	vgSL_POINT			line1;
	vgSL_POINT			line2 = XCoord->line;
	vgSL_POINT			point1, point2;
	vgSL_REGION			newRegion;
	vgSL_REGIONPOINT	rPoint;
	vgCOORD				deltaX, y;
	VGboolean			isPoint2Lower = VG_TRUE;

	/* xCoord1, line1, and point1 are for left branch of newRegion. */
	/* Peek the next xCoord to get line1 (leftLine) of newRegion. */
	xCoord1 = XCoord->next;
	gcmASSERT(xCoord1);
	line1 = xCoord1->line;

	if (UseToBeMergedRegion)
	{
		newRegion = Region->next;
		gcmASSERT(newRegion);
		gcmASSERT(Region->rightLine == gcvNULL);
		gcmASSERT(Region->rightPointList);
		gcmASSERT(newRegion->leftLine == gcvNULL);
		gcmASSERT(newRegion->leftPointList);
	}
	else
	{
		/* Add a new region to regionList. */
		gcmERR_RETURN(vgSL_AllocateARegion(ScanLineTess, &newRegion));

		/* Copy all data from Region to newRegion. */
		*newRegion = *Region;

		/* Insert newRegion after Region. */
		newRegion->prev = Region;
		newRegion->next = Region->next;
		if (Region->next)
		{
			Region->next->prev = newRegion;
		}
		else
		{
			ScanLineTess->lastRegion = newRegion;
		}
		Region->next = newRegion;
	}

	/* xCoord2, line2, and point2 are for right branch of Region. */
	if (line2->orientation == vgvSL_UP)
	{
		point2 = line2;
		Region->rightOrientation = vgvSL_UP;
		Region->isRightNext = VG_TRUE;
	}
	else
	{
		gcmASSERT(line2->orientation == vgvSL_DOWN);
		point2 = line2->next;
		Region->rightOrientation = vgvSL_DOWN;
		Region->isRightNext = VG_FALSE;
	}

	if (line1->orientation == vgvSL_UP)
	{
		point1 = line1;
		newRegion->leftOrientation = vgvSL_UP;
		newRegion->isLeftNext = VG_TRUE;
	}
	else
	{
		gcmASSERT(line1->orientation == vgvSL_DOWN);
		point1 = line1->next;
		newRegion->leftOrientation = vgvSL_DOWN;
		newRegion->isLeftNext = VG_FALSE;
	}

	if (line1->orientation == line2->orientation)
	{
		/* Check for flat line on y line. */
		y = COORD_FLOOR(point2->y + COORD_EPSILON);
		if (COORD_ABS(point2->y - y) < COORD_EPSILON)
		{
			/* point2 is on y line. */
			/* Check if line2 connected to a flat line. */
			if (Region->isRightNext)
			{
				if (line2->prev->orientation == vgvSL_FLAT)
				{
					line1 = line2->prev;
					gcmASSERT(line1->x == xCoord1->x);
					point1 = point2;
					newRegion->leftOrientation = vgvSL_DOWN;
					newRegion->isLeftNext = VG_FALSE;
				}
			}
			else
			{
				if (line2->next->orientation == vgvSL_FLAT)
				{
					line1 = line2->next;
					gcmASSERT(line1->next->x == xCoord1->x);
					point1 = point2;
					newRegion->leftOrientation = vgvSL_UP;
					newRegion->isLeftNext = VG_TRUE;
				}
			}
		}
	}

	/* Set y to the lower y value of point1 and point2. */
	y = point2->y;
	if (point1 != point2)
	{
		if (point1->y < y)
		{
			isPoint2Lower = VG_FALSE;
			y = point1->y;
		}
	}

	if (UseToBeMergedRegion)
	{
		/* Split the to-be-merged regions from lower point{1,2} to the joint of Region and newRegion. */
		/* Process Region. */
		if (isPoint2Lower)
		{
			/* Fake rightLine and rightDeltaX to add point2. */
			Region->rightLine = line2;
			Region->rightDeltaX = vgSL_GetDeltaX(Region->rightPointList->point, point2);
			gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, Region));
		}
		else
		{
			/* Fake rightLine and rightDeltaX to add point1. */
			Region->rightLine = line1;
			Region->rightDeltaX = vgSL_GetDeltaX(Region->rightPointList->point, point1);
			gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point1, Region));

			if (point2 != point1)
			{
				/* Fake rightLine and rightDeltaX to add point2. */
				Region->rightLine = line2;
				Region->rightDeltaX = vgSL_GetDeltaX(point1, point2);
				gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, Region));
			}
		}

		/* Process newRegion. */
		if (isPoint2Lower)
		{
			/* Fake leftLine and leftDeltaX to add point2. */
			newRegion->leftLine = line2;
			newRegion->leftDeltaX = vgSL_GetDeltaX(newRegion->leftPointList->point, point2);
			gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point2, newRegion));

			if (point1 != point2)
			{
				/* Fake leftLine and leftDeltaX to add point1. */
				newRegion->leftLine = line1;
				newRegion->leftDeltaX = vgSL_GetDeltaX(point2, point1);
				gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, newRegion));
			}
		}
		else
		{
			/* Fake leftLine and leftDeltaX to add point1. */
			newRegion->leftLine = line1;
			newRegion->leftDeltaX = vgSL_GetDeltaX(newRegion->leftPointList->point, point1);
			gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, newRegion));
		}
	}
	else if (Region->leftPointList)
	{
		/* Split the region from lower point{1,2} to the first left point that is lower than point{1,2}. */
		/* Find the first left point that is lower than lower point{1,2}. */
		vgSL_REGIONPOINT nextRPoint = gcvNULL;
		vgSL_REGIONPOINT leftRPointList = gcvNULL;
		for (rPoint = Region->leftPointList; rPoint; rPoint = nextRPoint)
		{
			nextRPoint = rPoint->next;
			if (rPoint->point->y <= y) break;

			/* Move rPoint to leftRPointList in reverse order. */
			rPoint->next = leftRPointList;
			leftRPointList = rPoint;
		}

		/* Process Region. */
		gcmASSERT(rPoint || Region->bottomPoint);
		if (isPoint2Lower)
		{
			if (rPoint)
			{
				gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, rPoint->point, point2, Region));
			}
			else
			{
				gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, Region->bottomPoint, point2, Region));
			}
		}
		else
		{
			if (rPoint)
			{
				gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, rPoint->point, point1, Region));
			}
			else
			{
				gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, Region->bottomPoint, point1, Region));
			}

			/* Fake rightLine and rightDeltaX to add point2. */
			Region->rightLine = line2;
			Region->rightDeltaX = vgSL_GetDeltaX(point1, point2);
			gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, Region));
		}
		if (leftRPointList)
		{
			/* Add points in leftRPointList to left branch. */
			vgSL_REGIONPOINT lrPoint, nextLRPoint;
			for (lrPoint = leftRPointList; lrPoint; lrPoint = nextLRPoint)
			{
				nextLRPoint = lrPoint->next;
				gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, lrPoint->point, Region));
				vgSL_FreeARegionPoint(ScanLineTess, lrPoint);
			}
		}

		/* Process newRegion. */
		newRegion->leftPointList = rPoint;
		if (isPoint2Lower)
		{
			/* Fake leftLine and leftDeltaX to add point2. */
			newRegion->leftLine = line2;
			if (rPoint)
			{
				newRegion->leftDeltaX = vgSL_GetDeltaX(rPoint->point, point2);
			}
			else
			{
				gcmASSERT(newRegion->bottomPoint);
				newRegion->leftDeltaX = vgSL_GetDeltaX(newRegion->bottomPoint, point2);
			}
			gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point2, newRegion));

			if (point1 != point2)
			{
				/* Fake leftLine and leftDeltaX to add point1. */
				newRegion->leftLine = line1;
				newRegion->leftDeltaX = vgSL_GetDeltaX(point2, point1);
				gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, newRegion));
			}
		}
		else
		{
			/* Fake leftLine and leftDeltaX to add point1. */
			newRegion->leftLine = line1;
			if (rPoint)
			{
				newRegion->leftDeltaX = vgSL_GetDeltaX(rPoint->point, point1);
			}
			else
			{
				gcmASSERT(newRegion->bottomPoint);
				newRegion->leftDeltaX = vgSL_GetDeltaX(newRegion->bottomPoint, point1);
			}
			gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, newRegion));
		}
	}
	else if (Region->rightPointList)
	{
		/* Split the region from lower point{1,2} to the first right point that is lower than point{1,2}. */
		/* Find the first right point that is lower than lower point{1,2}. */
		vgSL_REGIONPOINT nextRPoint = gcvNULL;
		vgSL_REGIONPOINT rightRPointList = gcvNULL;
		for (rPoint = Region->rightPointList; rPoint; rPoint = nextRPoint)
		{
			nextRPoint = rPoint->next;
			if (rPoint->point->y <= y) break;

			/* Move rPoint to rightRPointList in reverse order. */
			rPoint->next = rightRPointList;
			rightRPointList = rPoint;
		}

		/* Process newRegion. */
		gcmASSERT(rPoint || Region->bottomPoint);
		if (isPoint2Lower)
		{
			if (rPoint)
			{
				gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, point2, rPoint->point, newRegion));
			}
			else
			{
				gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, point2, Region->bottomPoint, newRegion));
			}

			if (point1 != point2)
			{
				/* Fake leftLine and leftDeltaX to add point1. */
				newRegion->leftLine = line1;
				newRegion->leftDeltaX = vgSL_GetDeltaX(point2, point1);
				gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, newRegion));
			}
		}
		else
		{
			if (rPoint)
			{
				gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, point1, rPoint->point, newRegion));
			}
			else
			{
				gcmERR_RETURN(vgSL_SetRegionBottom(ScanLineTess, point1, Region->bottomPoint, newRegion));
			}
		}
		if (rightRPointList)
		{
			/* Add points in leftRPointList to left branch. */
			vgSL_REGIONPOINT rrPoint, nextLRPoint;
			for (rrPoint = rightRPointList; rrPoint; rrPoint = nextLRPoint)
			{
				nextLRPoint = rrPoint->next;
				gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, rrPoint->point, newRegion));
				vgSL_FreeARegionPoint(ScanLineTess, rrPoint);
			}
		}

		/* Process Region. */
		Region->rightPointList = rPoint;
		if (isPoint2Lower)
		{
			/* Fake rightLine and rightDeltaX to add point2. */
			Region->rightLine = line2;
			if (rPoint)
			{
				Region->rightDeltaX = vgSL_GetDeltaX(rPoint->point, point2);
			}
			else
			{
				gcmASSERT(Region->bottomPoint);
				Region->rightDeltaX = vgSL_GetDeltaX(Region->bottomPoint, point2);
			}
			gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, Region));
		}
		else
		{
			/* Fake rightLine and rightDeltaX to add point1. */
			Region->rightLine = line1;
			if (rPoint)
			{
				Region->rightDeltaX = vgSL_GetDeltaX(rPoint->point, point1);
			}
			else
			{
				gcmASSERT(Region->bottomPoint);
				Region->rightDeltaX = vgSL_GetDeltaX(Region->bottomPoint, point1);
			}
			gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point1, Region));

			if (point2 != point1)
			{
				/* Fake rightLine and rightDeltaX to add point2. */
				Region->rightLine = line2;
				Region->rightDeltaX = vgSL_GetDeltaX(point1, point2);
				gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, Region));
			}
		}
	}
	else
	{
		gcmASSERT(Region->bottomPoint);
		if (isPoint2Lower)
		{
			/* Split the region from bottomPoint to Point2. */
			deltaX = vgSL_GetDeltaX(Region->bottomPoint, point2);

			/* Add a point to Region's right branch. */
			gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
			rPoint->point = point2;
			rPoint->deltaX = deltaX;
			Region->rightPointList = rPoint;

			/* Add a point to newRegion's left branch. */
			gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
			rPoint->point = point2;
			rPoint->deltaX = deltaX;
			newRegion->leftPointList = rPoint;

			if (point1 != point2)
			{
				/* Fake leftLine and leftDeltaX to add point1 to newRegion. */
				newRegion->leftLine = line1;
				newRegion->leftDeltaX = vgSL_GetDeltaX(point2, point1);
				gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, newRegion));
			}
		}
		else
		{
			gcmASSERT(point1 != point2);
			/* Split the region from bottomPoint to Point1. */
			deltaX = vgSL_GetDeltaX(Region->bottomPoint, point1);

			/* Add a point to Region's right branch. */
			gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
			rPoint->point = point1;
			rPoint->deltaX = deltaX;
			Region->rightPointList = rPoint;

			/* Fake rightLine and rightDeltaX to add point2 to Region. */
			newRegion->rightLine = line2;
			newRegion->rightDeltaX = vgSL_GetDeltaX(point1, point2);
			gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, Region));

			/* Add a point to newRegion's left branch. */
			gcmERR_RETURN(vgSL_AllocateARegionPoint(ScanLineTess, &rPoint));
			rPoint->point = point1;
			rPoint->deltaX = deltaX;
			newRegion->leftPointList = rPoint;
		}
	}

	/* Set Region's right branch. */
	vgSL_SetRegionRightLine(Region, line2);

	/* Set newRegion's left branch. */
	vgSL_SetRegionLeftLine(newRegion, line1);

	return status;
}

gceSTATUS
vgSL_HandleSimpleCrossForUpDownRegions(
	vgSCANLINETESS		ScanLineTess,
	vgSL_XCOORD			XCoord1,
	vgSL_XCOORD			XCoord2,
	vgSL_YSCANLINE		PrevYScanLine,
	vgSL_REGION			Region
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_POINT			newPoint, tmpLine;
	vgSL_XCOORD			pxc1, pxc2;
	vgCOORD				x, y;
	vgSL_ORIENTATION	tmpOrient;
	VGboolean			tmpBool;

	/* Get the X1, X2 in the previous yScanLine. */
	vgSL_GetXCoords(Region, PrevYScanLine, &pxc1, &pxc2);

	/* Get the cross point. */
	if (pxc1 && pxc2)
	{
		/* Simple case. */
		vgSL_GetIntersection(pxc1->x, pxc2->x, XCoord1->x, XCoord2->x, PrevYScanLine->y, &x, &y);
	}
	else if (pxc1)
	{
		vgSL_POINT point2;
		vgCOORD x1, x2, x3, x4;
		vgCOORD y1, y2;

		/* Get pxc2. */
		pxc2 = pxc1->next;
		gcmASSERT(pxc2);
		gcmASSERT(pxc2->endLineStatus);
		gcmASSERT(Region->rightPointList);

		/* Create virtual scan line(s) to get cross point. */
		y1 = PrevYScanLine->y;
		x1 = pxc1->x;
		x2 = pxc2->x;
		vgSL_GetEndPointOrHightEndPoint(pxc2, &point2);
		do
		{
			x3 = point2->x;
			y2 = point2->y;
			x4 = x1 + COORD_MUL(y2 - y1, Region->leftDeltaX);
			if (x3 < x4) break;
			x1 = x4;
			x2 = x3;
			y1 = y2;
			if (point2 == Region->leftPointList->point)
			{
				/* Reach the upper most point. */
				x3 = XCoord1->x;
				x4 = XCoord2->x;
				y2 = PrevYScanLine->y + COORD_ONE;
				break;
			}

			/* Move to next point. */
			point2 = Region->isRightNext ? point2->next : point2->prev;
		} while (VG_TRUE);

		/* Get the cross point. */
		vgSL_GetIntersectionW2Y(x1, x2, x3, x4, y1, y2, &x, &y);
		/* TODO - Create virtual scan line(s) to get cross point. */
		gcmASSERT(0);
	}
	else if (pxc2)
	{
		vgSL_POINT point1;
		vgCOORD x1, x2, x3, x4;
		vgCOORD y1, y2;
		vgSL_XCOORD nextXCoord;

		/* Get pxc1. */
		for (pxc1 = PrevYScanLine->xcoords; pxc1; pxc1 = nextXCoord)
		{
			nextXCoord = pxc1->next;
			if (nextXCoord == pxc2) break;
		}
		gcmASSERT(pxc1);
		gcmASSERT(pxc1->endLineStatus);
		gcmASSERT(Region->leftPointList);

		/* Create virtual scan line(s) to get cross point. */
		y1 = PrevYScanLine->y;
		x1 = pxc1->x;
		x2 = pxc2->x;
		vgSL_GetEndPointOrHightEndPoint(pxc1, &point1);
		do
		{
			x4 = point1->x;
			y2 = point1->y;
			x3 = x2 + COORD_MUL(y2 - y1, Region->rightDeltaX);
			if (x3 < x4) break;
			x1 = x4;
			x2 = x3;
			y1 = y2;
			if (point1 == Region->leftPointList->point)
			{
				/* Reach the upper most point. */
				x3 = XCoord1->x;
				x4 = XCoord2->x;
				y2 = PrevYScanLine->y + COORD_ONE;
				break;
			}

			/* Move to next point. */
			point1 = Region->isLeftNext ? point1->next : point1->prev;
		} while (VG_TRUE);

		/* Get the cross point. */
		vgSL_GetIntersectionW2Y(x1, x2, x3, x4, y1, y2, &x, &y);
	}
	else
	{
		/* TODO - Complicate case. */
		gcmASSERT(0);
	}

	/* Add the cross point. */
	gcmERR_RETURN(vgSL_AddACrossPoint(ScanLineTess, x, y,
							Region->leftLine, Region->rightLine, &newPoint));

	/* Add the point to region - will create a triangle to close the region. */
	gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, newPoint, Region));

	/* Virtually close the current region by freeing it's contents. */
	gcmASSERT(Region->rightPointList == gcvNULL);
	gcmASSERT(Region->leftPointList->next == gcvNULL);
	vgSL_FreeARegionPoint(ScanLineTess, Region->leftPointList);
	Region->leftPointList = gcvNULL;

	/* Set region's bottom, and swap region's left and right branches. */
	Region->bottomPoint = newPoint;

	tmpLine		= Region->leftLine;
	tmpOrient	= Region->leftOrientation;
	tmpBool		= Region->isLeftNext;

	vgSL_SetRegionLeftLine(Region, Region->rightLine);
	Region->leftOrientation	= Region->rightOrientation;
	Region->isLeftNext		= Region->isRightNext;

	vgSL_SetRegionRightLine(Region, tmpLine);
	Region->rightOrientation	= tmpOrient;
	Region->isRightNext			= tmpBool;

	return status;
}

gceSTATUS
vgSL_HandleSimpleCrossForLeftRightRegions(
	vgSCANLINETESS		ScanLineTess,
	vgSL_XCOORD			XCoord2,
	vgSL_YSCANLINE		PrevYScanLine,
	vgSL_REGION			Region
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_REGION			nextRegion = Region->next;
	vgSL_POINT			newPoint, tmpLine;
	vgSL_XCOORD			pxc1, pxc2;
	vgCOORD				x, y;
	vgSL_ORIENTATION	tmpOrient;
	VGboolean			tmpBool;

	/* Get the X1, X2 in the previous yScanLine. */
	vgSL_GetXCoords(Region, PrevYScanLine, &pxc1, &pxc2);
	gcmASSERT(pxc2 && pxc2->next && pxc2->next->line == Region->next->leftLine);

	/* Get the cross point coordinates and add a cross point. */
	vgSL_GetIntersection(pxc2->x, pxc2->next->x, XCoord2->x, XCoord2->next->x,
						PrevYScanLine->y, &x, &y);
	gcmERR_RETURN(vgSL_AddACrossPoint(ScanLineTess, x, y,
							Region->rightLine, nextRegion->leftLine, &newPoint));

	/* Add cross point to right branch. */
	/* It must be a convex point. */
	gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, newPoint, Region));

	/* Add cross point to next region's left branch. */
	/* It must be a convex point. */
	gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, newPoint, nextRegion));

	/* Swap region's right branch and nextRegion's left branch. */
	tmpLine		= Region->rightLine;
	tmpOrient	= Region->rightOrientation;
	tmpBool		= Region->isRightNext;

	vgSL_SetRegionRightLine(Region, nextRegion->leftLine);
	Region->rightOrientation	= nextRegion->leftOrientation;
	Region->isRightNext			= nextRegion->isLeftNext;

	vgSL_SetRegionLeftLine(nextRegion, tmpLine);
	nextRegion->leftOrientation	= tmpOrient;
	nextRegion->isLeftNext		= tmpBool;

	return status;
}

gceSTATUS
vgSL_MergePrevRegion(
	vgSCANLINETESS		ScanLineTess,
	vgSL_REGION			Region,
	vgSL_POINT *		Point
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_REGIONPOINT	rPoint, nextRPoint;
	vgSL_REGIONPOINT	leftRPointList = gcvNULL;
	vgSL_REGION			prevRegion = Region->prev;

	gcmASSERT(prevRegion->leftPointList);
	gcmASSERT(prevRegion->leftPointList->next == gcvNULL);
	if (prevRegion->rightPointList)
	{
		/* prevRegion has flat bottom. */
		gcmASSERT(prevRegion->rightPointList->point == *Point);
		gcmASSERT(prevRegion->rightPointList->next == gcvNULL);
		vgSL_FreeARegionPoint(ScanLineTess, prevRegion->rightPointList);
		prevRegion->rightPointList = gcvNULL;
	}
	else
	{
		gcmASSERT(prevRegion->bottomPoint == *Point);
	}

	/* Add the points in prevRegion's left branch to region's left branch. */
	for (rPoint = prevRegion->leftPointList; rPoint; rPoint = nextRPoint)
	{
		nextRPoint = rPoint->next;
		/* Move rPoint to leftRPointList in reverse order. */
		rPoint->next = leftRPointList;
		leftRPointList = rPoint;
	}

	/* Add points in leftRPointList to left branch. */
	for (rPoint = leftRPointList; rPoint; rPoint = nextRPoint)
	{
		nextRPoint = rPoint->next;

		/* Fake leftLine and leftDeltaX to add point. */
		Region->leftLine = rPoint->point;
		Region->leftDeltaX = rPoint->deltaX;
		gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess,
				rPoint->point, Region));
		vgSL_FreeARegionPoint(ScanLineTess, rPoint);
	}

	/* Remove prevRegion. */
	vgSL_DeleteARegion(ScanLineTess, prevRegion);

	/* Update Point. */
	*Point = Region->leftPointList->point;

	return status;
}

INLINE VGboolean
vgSL_NeedToAdvancePoint(
	vgSL_POINT			Line
	)
{
	if (Line == gcvNULL) return VG_FALSE;
	if (Line->crossYLinesCount == 0) return VG_TRUE;
	if (Line->crossYLinesCount == 1 && Line->lowerPointOnYLine)
	{
		if (Line->orientation == vgvSL_DOWN) return VG_TRUE;
		if (Line->prev->orientation == vgvSL_DOWN) return VG_TRUE;
	}
	return VG_FALSE;
}

gceSTATUS
vgSL_CreateTriangles(
	vgSCANLINETESS		ScanLineTess,
	_VGIndexBuffer *	IndexBuffer,
	_VGVertexBuffer *	VertexBuffer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSL_YSCANLINE		yScanLine;
	VGint				i;
	vgSL_REGION			region;
	vgSL_SUBPATH		subPath = gcvNULL;
	vgSL_POINT			point = gcvNULL;
	vgSL_VECTOR2		vec;
	VGshort				id;
	VGshort *			index;
	vgSL_TRIANGLE		triangle;
	VGint				bufferSize;

	/* Process yScanLines. */
	/* - identify regions. */
	/* - create triangles. */
	/* - find cross points of edges if any. */
	yScanLine = ScanLineTess->yScanLineArray;
	for (i = ScanLineTess->yMin; i <= ScanLineTess->yMax; i++, yScanLine++)
	{
		vgSL_XCOORD xCoord1;
		vgSL_XCOORD xCoord2 = yScanLine->xcoords;
		VGint fillCount = 0;
		vgSL_POINT point1, point2;

		if (xCoord2 == gcvNULL)
		{
			gcmASSERT(yScanLine->xcoordCount == 0);
			gcmASSERT(ScanLineTess->regionList == gcvNULL);
			continue;
		}

		/* Check if the orientation of the first point is vgvSL_FLAT. */
		if (xCoord2->orientation == vgvSL_FLAT)
		{
			/* Every horizontal line should be filled. */
			xCoord1 = xCoord2->next;
			if (xCoord1 == gcvNULL)
			{
				gcmASSERT(xCoord1);
				continue;
			}
			/* Switch orientation for xCoord1 and xCoord2. */
			/* This will fill the horizontal line. */
			gcmASSERT(xCoord1->orientation != 0);
			xCoord2->orientation = xCoord1->orientation;
			xCoord1->orientation = 0;
		}

		region = ScanLineTess->regionList;
		while (gcvTRUE)
		{
			VGboolean leftLineBegin = VG_FALSE;
			VGboolean leftLineEnd = VG_FALSE;
			VGboolean rightLineBegin = VG_FALSE;
			VGboolean rightLineEnd = VG_FALSE;
			VGboolean needToRestoreRegion = VG_FALSE;

			xCoord1 = xCoord2;
			fillCount += xCoord1->orientation;
			xCoord2 = xCoord1->next;
			if (xCoord2 == gcvNULL) break;

			/* Determine if this is a filled scan line. */
			if (xCoord2->orientation == vgvSL_FLAT && fillCount == 0)
			{
				/* Every horizontal line should be filled. */
				/* Switch orientation for xCoord1 and xCoord2. */
				/* This will fill the horizontal line. */
				gcmASSERT(xCoord1->orientation != 0);
				fillCount -= xCoord1->orientation;
				xCoord2->orientation = xCoord1->orientation;
				xCoord1->orientation = 0;
			}
			if (ScanLineTess->fillRule == VG_EVEN_ODD)
			{
				/* VG_EVEN_ODD rule - skip line segments with even fill count. */
				if (((VGuint) fillCount & 0x1) == 0) continue;
			}
			else
			{
				/* VG_NON_ZERO rule - skip line segments with zero fill count. */
				if (fillCount == 0) continue;
			}

			/* Check xCoord1 with left branch. */
			if (region == gcvNULL)
			{
				/* Add a new region to the end of region list. */
				gcmERR_RETURN(vgSL_AddARegion(ScanLineTess, yScanLine, xCoord1, xCoord2, gcvNULL));

				/* xCoord1 and xCoord2 may be the only cross point(s) on left/right line(s). */
				/* So, they need to be check again. */
				region = ScanLineTess->lastRegion;
			}

		AfterAddARegion:

			point1 = point2 = gcvNULL;

			/* Check if this is a new region or there is a cross point below this yScanLine. */
			if (region->leftLine &&
				(region->leftOrientation != xCoord1->line->orientation ||
				 (xCoord1->line != region->leftLine && xCoord1->line != region->leftLine->prev)))
			{
				/* First, check if this is simple up/down fill-region cross point. */
				if (xCoord1->line == region->rightLine && xCoord2->line == region->leftLine)
				{
					/* Simple up/down fill regions. */
					gcmERR_RETURN(vgSL_HandleSimpleCrossForUpDownRegions(ScanLineTess,
										xCoord1, xCoord2, yScanLine - 1, region));
				}
				else if (xCoord1->endLineStatus)
				{
					/* Check if xCoord2 is the end of previous line - a flat line. */
					if (xCoord2->line == region->leftLine)
					{
						/* TODO - Find a flat line. */
						/* Create a new region without rightLine, ready to be merged with next region. */
						/* TODO - Need to merge this into vgSL_AddARegion as a special case. */
						gcmASSERT(0);

						gcmASSERT(xCoord2->endLineStatus);
						vgSL_GetEndPointOrHightEndPoint(xCoord2, &point2);
						do
						{
							gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point2, region));
							point2 = region->isLeftNext ? point2->next : point2->prev;
						} while (point2 != point1);
						gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, region));
					}
					else
					{
						/* TODO - Check xCoord2->x is smaller than leftLine crossing point. */

						/* Find a new region. */
						/* Add the new region before the current region. */
						gcmERR_RETURN(vgSL_AddARegion(ScanLineTess, yScanLine, xCoord1, xCoord2, region));

						/* Move region to the newly added region. */
						region = region->prev;

						/* Go to AfterAddARegion to handle both branches. */
						goto AfterAddARegion;
					}
				}
				else
				{
					/* TODO - Complicate case. */
					gcmASSERT(xCoord1->line == region->rightLine && xCoord2->line == region->leftLine);
				}
			}

			if (xCoord1->endLineStatus)
			{
				vgSL_GetEndPointOrHightEndPoint(xCoord1, &point1);

				/* Check if this is the beginning/end of left line. */
				if ((region->isLeftNext && region->leftLine && point1 == region->leftLine->next)
				||  (!region->isLeftNext && point1 == region->leftLine))
				{
					/* End of left line. */
					leftLineEnd = VG_TRUE;
				}
				else if ((region->isLeftNext && point1 == region->leftLine)
					 ||  (!region->isLeftNext && region->leftLine && point1 == region->leftLine->next))
				{
					/* Beginning of left line. */
					leftLineBegin = VG_TRUE;
				}
				else
				{
					/* Should not reach here. */
					gcmASSERT(0);

					/* Check if xCoord2 is the end of previous line - a flat line. */
					if (xCoord2->line == region->leftLine)
					{
						/* TODO - Find a flat line. */
						/* Create a new region without rightLine, ready to be merged with next region. */
						/* TODO - Need to merge this into vgSL_AddARegion as a special case. */
						gcmASSERT(0);

						gcmASSERT(xCoord2->endLineStatus);
						vgSL_GetEndPointOrHightEndPoint(xCoord2, &point2);
						do
						{
							gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point2, region));
							point2 = region->isLeftNext ? point2->next : point2->prev;
						} while (point2 != point1);
						gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, region));
					}
					else
					{
						/* TODO - Check xCoord2->x is smaller than leftLine crossing point. */

						/* Find a new region. */
						/* Add the new region before the current region. */
						gcmERR_RETURN(vgSL_AddARegion(ScanLineTess, yScanLine, xCoord1, xCoord2, region));

						/* Move region to the newly added region. */
						region = region->prev;

						/* Go to AfterAddARegion to handle both branches. */
						goto AfterAddARegion;
					}
				}
			}

			if (region->rightLine == gcvNULL)
			{
				needToRestoreRegion = VG_TRUE;
				gcmASSERT(region->next);
				region = region->next;
				gcmASSERT(region->leftLine == gcvNULL);
			}

			/* Check xCoord2 with right branch. */
			if (xCoord2->endLineStatus)
			{
				vgSL_GetEndPointOrHightEndPoint(xCoord2, &point2);

				/* Check if this is the beginning/end of right line. */
				if ((region->isRightNext && region->rightLine && point2 == region->rightLine->next)
				||  (!region->isRightNext && point2 == region->rightLine))
				{
					/* End of right line. */
					rightLineEnd = VG_TRUE;
				}
				else if ((region->isRightNext && point2 == region->rightLine)
					 ||  (!region->isRightNext && region->rightLine && point2 == region->rightLine->next))
				{
					/* Beginning of right line. */
					rightLineBegin = VG_TRUE;
				}
				else if (xCoord2->orientation)
				{
					/* Find a hole, nutch, a cross point, or a flat right branch. */
					gcmASSERT(xCoord2->next);

					/* First, check if this a cross point at right branch with a new region. */
					if (xCoord2->orientation == xCoord2->next->orientation &&
						xCoord2->next->line == region->rightLine)
					{
						vgSL_XCOORD xCoord3 = xCoord2->next->next;
						gcmASSERT(xCoord3);

						/* Create a new region with xCoord2 and xCoord3. */
						gcmERR_RETURN(vgSL_AddARegion(ScanLineTess, yScanLine, xCoord2, xCoord3, region->next));

						/* TODO - Handle Left/right fill regions. */
						/*gcmERR_RETURN(vgSL_HandleCrossRegionAtRightBranch(ScanLineTess,
										xCoord2, yScanLine, region));*/
					}
					else
					{
						/* Need to split the region before handling the scan line segment. */
						if (needToRestoreRegion)
						{
							region = region->prev;
						}

						gcmERR_RETURN(vgSL_SplitARegion(ScanLineTess, xCoord2, region, needToRestoreRegion));
						needToRestoreRegion = VG_FALSE;

						/* Go to AfterAddARegion to handle both branches. */
						leftLineBegin = leftLineEnd = VG_FALSE;
						goto AfterAddARegion;
					}
				}
				else
				{
					gcmASSERT(xCoord2->orientation);
				}
			}

			if (needToRestoreRegion)
			{
				gcmASSERT(region->prev);
				region = region->prev;
				gcmASSERT(region->rightLine == gcvNULL);
			}

			if (leftLineBegin)
			{
				/* point1 is the first point of left line. */
				/* Usually skip it, but need to check the special case after addARegion or splitARegion. */
				/* Advance point1 to next non-zero-crossing-yLine line. */
				if (vgSL_NeedToAdvancePoint(region->leftLine))
				{
					/* Advance point1 to next non-zero-crossing-yLine line. */
					do
					{
						point1 = region->isLeftNext ? point1->next : point1->prev;
						gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, region));
					}
					while (point1 != point2 && vgSL_NeedToAdvancePoint(region->leftLine));

					if (point1 == point2)
					{
						/* Check if point2 is the end of right line. */
						if (region->rightLine && point2 != region->rightLine && point2 != region->rightLine->next)
						{
							/* xCoord2 is already processed. */
							continue;
						}
					}
				}
			}
			else if (leftLineEnd)
			{
				/* End of left line.  Add the point to left branch. */
				gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, region));

				/* Check if it is necessary to merge with prev region. */
				if (region->leftLine == gcvNULL && region->prev &&
					region->prev->rightLine == gcvNULL &&
					(region->prev->rightPointList == gcvNULL || region->prev->bottomPoint == gcvNULL))
				{
					/* Need to merge with prev region. */
					gcmERR_RETURN(vgSL_MergePrevRegion(ScanLineTess, region, &point1));
				}

				/* Check if leftLine has crossYLines. */
				if (vgSL_NeedToAdvancePoint(region->leftLine))
				{
					/* Advance point1 to next non-zero-crossing-yLine line. */
					VGboolean regionClosed = VG_FALSE;

					/*vgSL_GetEndPointOrHightEndPoint(xCoord2, &point2);*/
					do
					{
						vgSL_POINT point = region->isLeftNext ? point1->next : point1->prev;
						if (region->leftLine->orientation == vgvSL_FLAT && point->x > xCoord2->x)
						{
							/* Encounter a cross point with one flat line. */
							/* Flat top - close the current region. */
							vgSL_REGION nextRegion = region->next;
							vgSL_POINT newPoint;

							gcmASSERT(region->rightPointList == gcvNULL);
							if (xCoord2->endLineStatus == 0)
							{
								/* (xCoord2->x, y) is the cross point.  Add a cross point.*/
								gcmASSERT(xCoord2->line == region->rightLine);
								gcmERR_RETURN(vgSL_AddACrossPoint(ScanLineTess, xCoord2->x, yScanLine->y,
													region->leftLine, region->rightLine, &newPoint));

								/* Add the point to region - will create a triangle to close the region. */
								gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, newPoint, region));
							}
							else
							{
								/* TODO - Check if point2 is on the yScanLine. */
								gcmASSERT(xCoord2->endLineStatus == 0);
							}

							/* Close region. */
							vgSL_SetRegionLeftLine(region, gcvNULL);
							vgSL_SetRegionRightLine(region, gcvNULL);
							gcmASSERT(region->leftPointList->next == gcvNULL);
							vgSL_FreeARegionPoint(ScanLineTess, region->leftPointList);

							/* Delete the region. */
							vgSL_DeleteARegion(ScanLineTess, region);

							/* Move to next region. */
							region = nextRegion;
							regionClosed = VG_TRUE;
							break;
						}
						point1 = point;
						gcmERR_RETURN(vgSL_AddAPointToRegionLeftBranch(ScanLineTess, point1, region));

						/* Check if it is necessary to merge with prev region. */
						if (region->leftLine == gcvNULL && region->prev &&
							region->prev->rightLine == gcvNULL &&
							(region->prev->rightPointList == gcvNULL || region->prev->bottomPoint == gcvNULL))
						{
							/* Need to merge with prev region. */
							gcmERR_RETURN(vgSL_MergePrevRegion(ScanLineTess, region, &point1));
						}
					}
					while (point1 != point2 && vgSL_NeedToAdvancePoint(region->leftLine));

					if (regionClosed)
					{
						/* region is already advanced to next region. */
						continue;
					}

					if (point1 == point2)
					{
						/* Check if point2 is the end of right line. */
						if (region->rightLine && point2 != region->rightLine && point2 != region->rightLine->next)
						{
							/* xCoord2 is already processed. */
							/* It is still in the same region. */
							continue;
						}
					}
				}
			}
			else
			{
				/* Already handled. */
			}

			if (region->rightLine == gcvNULL)
			{
				gcmASSERT(region->next);
				region = region->next;
				gcmASSERT(region->leftLine == gcvNULL);
			}

			if (rightLineBegin)
			{
				/* point2 is the first point of right line. */
				/* Usually skip it, but need to check the special case after addARegion or splitARegion. */
				/* Advance point2 to next non-zero-crossing-yLine line. */
				if (vgSL_NeedToAdvancePoint(region->rightLine))
				{
					/* Advance point1 to next non-zero-crossing-yLine line. */
					do
					{
						point2 = region->isRightNext ? point2->next : point2->prev;
						if (point2 == point1) break;
						gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, region));
					}
					while (vgSL_NeedToAdvancePoint(region->rightLine));

					/* Check if region ends here with one top point. */
					if (point2 == point1)
					{
						/* End of regoin, and the region is already trigulated when processing point1. */
						vgSL_REGION nextRegion = region->next;

						gcmASSERT(region->leftLine == gcvNULL);
						gcmASSERT(region->leftPointList->point == point1);
						gcmASSERT(region->leftPointList->next == gcvNULL);
						gcmASSERT(region->rightPointList == gcvNULL);
						vgSL_FreeARegionPoint(ScanLineTess, region->leftPointList);

						/* Delete the region. */
						vgSL_DeleteARegion(ScanLineTess, region);

						/* Move to next region. */
						region = nextRegion;
						continue;
					}
				}
			}
			else if (rightLineEnd)
			{
				/* End of right line. */
				if (point1 != point2)
				{
					gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, region));

					if (vgSL_NeedToAdvancePoint(region->rightLine))
					{
						/* Advance point2 to next non-zero-crossing-yLine line. */
						VGboolean regionClosed = VG_FALSE;

						do
						{
							vgSL_POINT point = region->isRightNext ? point2->next : point2->prev;
							if (region->rightLine->orientation == vgvSL_FLAT && point->x < xCoord1->x)
							{
								/* Encounter a cross point with one flat line. */
								/* Flat top - close the current region. */
								vgSL_REGION nextRegion = region->next;
								vgSL_POINT newPoint;

								gcmASSERT(region->leftPointList == gcvNULL);
								if (xCoord1->endLineStatus == 0)
								{
									/* (xCoord1->x, y) is the cross point.  Add a cross point.*/
									gcmASSERT(xCoord1->line == region->leftLine);
									gcmERR_RETURN(vgSL_AddACrossPoint(ScanLineTess, xCoord1->x, yScanLine->y,
														region->leftLine, region->rightLine, &newPoint));

									/* Add the point to region - will create a triangle to close the region. */
									gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, newPoint, region));
								}
								else
								{
									/* TODO - Check if point2 is on the yScanLine. */
									gcmASSERT(xCoord1->endLineStatus == 0);
								}

								/* Close region. */
								vgSL_SetRegionLeftLine(region, gcvNULL);
								vgSL_SetRegionRightLine(region, gcvNULL);
								gcmASSERT(region->rightPointList->next == gcvNULL);
								vgSL_FreeARegionPoint(ScanLineTess, region->rightPointList);

								/* Delete the region. */
								vgSL_DeleteARegion(ScanLineTess, region);

								/* Move to next region. */
								region = nextRegion;
								regionClosed = VG_TRUE;
								break;
							}
							point2 = point;
							if (point2 == point1) break;
							gcmERR_RETURN(vgSL_AddAPointToRegionRightBranch(ScanLineTess, point2, region));
						}
						while (vgSL_NeedToAdvancePoint(region->rightLine));

						if (regionClosed)
						{
							/* region is already advanced to next region. */
							continue;
						}
					}
				}

				/* Check if region ends here with one top point. */
				if (point1 == point2)
				{
					/* End of regoin, and the region is already trigulated when processing point1. */
					vgSL_REGION nextRegion = region->next;

					gcmASSERT(region->leftLine == gcvNULL);
					gcmASSERT(region->leftPointList->point == point1);
					gcmASSERT(region->leftPointList->next == gcvNULL);
					gcmASSERT(region->rightPointList == gcvNULL);
					vgSL_FreeARegionPoint(ScanLineTess, region->leftPointList);

					/* Delete the region. */
					vgSL_DeleteARegion(ScanLineTess, region);

					/* Move to next region. */
					region = nextRegion;
					continue;
				}
			}
			else
			{
				/* Mid of right line. */
				/* Check if xCoord2's line the same as the region's right line. */
				if (xCoord2->line != region->rightLine)
				{
					/* Encounter a cross point. */
					vgSL_REGION nextRegion = region->next;

					/* First, check if this is simple left/right fill-region cross point. */
					gcmASSERT(nextRegion);
					if (xCoord2->line == nextRegion->leftLine &&
						xCoord2->next && xCoord2->next->line == region->rightLine)
					{
						/* Simple left/right fill regions. */
						gcmERR_RETURN(vgSL_HandleSimpleCrossForLeftRightRegions(ScanLineTess,
										xCoord2, yScanLine - 1, region));
					}
					else
					{
						/* TODO - Complicate case. */
						gcmASSERT(xCoord2->line == nextRegion->leftLine &&
								  xCoord2->next && xCoord2->next->line == region->rightLine);
					}
				}
			}

			/* Move to next region for next scan line segment. */
			region = region->next;
		}
		gcmASSERT(fillCount == 0);
		gcmASSERT(region == gcvNULL);
	}

	if (ScanLineTess->triangleCount == 0) return gcvSTATUS_INVALID_DATA;

	/* Fill the vertex buffer. */
	ARRAY_CTOR(VertexBuffer->data, ScanLineTess->os);
	bufferSize = (ScanLineTess->pointCount + ScanLineTess->crossPointCount) * sizeof(struct _vgSL_VECTOR2);
	ARRAY_ALLOCATE(VertexBuffer->data, bufferSize);
	if (VertexBuffer->data.allocated == 0) return gcvSTATUS_OUT_OF_MEMORY;

	id = 0;
	vec = (vgSL_VECTOR2) VertexBuffer->data.items;
	for (subPath = ScanLineTess->subPathList; subPath; subPath = subPath->next)
	{
		/* Skip no-area subpath. */
		if (subPath->pointCount < 3) continue;

		for (i = 0, point = subPath->pointList; i < subPath->pointCount; i++, point = point->next)
		{
			vec->x = point->x;
			vec->y = point->y;
			vec++;
			point->id = id;
			id++;
		}
	}
	for (point = ScanLineTess->crossPointList; point; point = point->next)
	{
		vec->x = point->x;
		vec->y = point->y;
		vec++;
		point->id = id;
		id++;
	}
	gcmASSERT(id == ScanLineTess->pointCount + ScanLineTess->crossPointCount);
	VertexBuffer->data.size = bufferSize;
	VertexBuffer->size = 2;
	VertexBuffer->type = gcvVERTEX_FLOAT;
	VertexBuffer->normalized = gcvFALSE;
	VertexBuffer->stride = sizeof(struct _vgSL_VECTOR2);
	VertexBuffer->elementCount = 1;

	//Fill the index buffer
	ARRAY_CTOR(IndexBuffer->data, ScanLineTess->os);
	bufferSize = ScanLineTess->triangleCount * 3 * sizeof(VGshort);
	ARRAY_ALLOCATE(IndexBuffer->data, bufferSize);
	if (IndexBuffer->data.allocated == 0) return gcvSTATUS_OUT_OF_MEMORY;

	index = (VGshort *) IndexBuffer->data.items;
	for (triangle = ScanLineTess->triangleList; triangle; triangle = triangle->next)
	{
		*index = triangle->point1->id; index++;
		*index = triangle->point2->id; index++;
		*index = triangle->point3->id; index++;
	}
	IndexBuffer->data.size = bufferSize;
	IndexBuffer->indexType = gcvINDEX_16;
	IndexBuffer->count = ScanLineTess->triangleCount * 3;

	return status;
}
#endif

#if USE_TEST_LOG
gceSTATUS
vgSL_OutputPath(
	vgSCANLINETESS		ScanLineTess,
	vgSL_SUBPATH		SubPathList,
	gctFILE				CmdFile,
	gctFILE				CoordFile
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcoOS				os = ScanLineTess->os;
	VGint				segmentCount = 0;
	VGint				pointCount = 0;
	vgCOORD				scale = COORD_DIV(COORD_ONE, ScanLineTess->scale);
	vgSL_SUBPATH		subPath = gcvNULL;
	vgSL_POINT			point = gcvNULL;
	VGbyte				realBuffer[256];
	gctSTRING			buffer = (gctSTRING) realBuffer;
	gctUINT				offset = 0;

	scale = COORD_ONE;

	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		segmentCount += subPath->pointCount + 1;
		pointCount += subPath->pointCount;
	}

#if USE_ONE_TEST_LOG
#define VGSL_WIRTECOMMAND(COMMAND) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "%s\t", COMMAND)); \
	gcoOS_Write(os, CmdFile, offset, buffer); \
	segmentCount--;
#define VGSL_WIRTECOMMAND1(COMMAND) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "%s\n", COMMAND)); \
	gcoOS_Write(os, CmdFile, offset, buffer); \
	segmentCount--;
#else
#define VGSL_WIRTECOMMAND(COMMAND) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "%s,\n", COMMAND)); \
	gcoOS_Write(os, CmdFile, offset, buffer); \
	segmentCount--;
#define VGSL_WIRTECOMMAND1(COMMAND) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "%s\n", COMMAND)); \
	gcoOS_Write(os, CmdFile, offset, buffer); \
	segmentCount--;
#endif

#define VGSL_WIRTEPOINT(POINT) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset, "%12.6ff, %12.6ff,\n", \
				COORD_TO_FLOAT(COORD_MUL(POINT->x, scale)), \
				COORD_TO_FLOAT(COORD_MUL(POINT->y, scale)))); \
	gcoOS_Write(os, CoordFile, offset, buffer); \
	pointCount--;

	/* Output path with reversed scale. */
	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		/* Create a MOVE_TO command for the first point. */
		point = subPath->pointList;
		gcmASSERT(point);
		VGSL_WIRTECOMMAND("VG_MOVE_TO");
		VGSL_WIRTEPOINT(point);

		/* Create a LINE_TO command for the rest of points. */
		for (point = point->next; point; point = point->next)
		{
			VGSL_WIRTECOMMAND("VG_LINE_TO");
			VGSL_WIRTEPOINT(point);
		}

		/* Create a CLOSE_PATH command at the end. */
		VGSL_WIRTECOMMAND1("VG_CLOSE_PATH");
	}
	gcmASSERT(segmentCount == 0);
	gcmASSERT(pointCount == 0);

	return status;
}
#endif

#if !gcdUSE_VG
/*******************************************************************************
**								ScanLineTessellation
********************************************************************************
**
**	Construct a list of scan lines for the path.
**
**	INPUT:
**
**		_VGContext * Context
**			Pointer to a _VGContext structure holding the environment data.
**
**		_VGPath * Path
**			Pointer to a _VGPath structure holding path data.
**
**		_VGMatrix3x3 * Matrix
**			Pointer to a _VGMatrix3x3 structure holding user-to-surface matrix.
**
**		VGbitfield * PaintModes
**			Paint modes to process.
*/
gctBOOL
ScanLineTessellation(
	IN _VGContext *		Context,
	IN _VGPath *		Path,
	IN _VGMatrix3x3 *	Matrix,
	IN VGbitfield		PaintModes
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSCANLINETESS		scanLineTess = gcvNULL;
#if SCAN_LINE_HAS_3D_SUPPORT
	VGboolean			createTriangles = VG_TRUE;
#endif
#if USE_QUALITY_SCALE
	VGint				qualityScale = 0;
#endif
#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
	gctFILE				logFile = gcvNULL;
#else
	gctFILE				cmdFile = gcvNULL;
	gctFILE				coordFile = gcvNULL;
#endif
#endif

	if (VGSL_GETCONTEXTSTOKELINEWIDTH(Context) <= COORD_ZERO)
	{
		/* Turn off stroke drawing. */
		PaintModes &= VG_FILL_PATH;

		/* Check if there is nothing to draw. */
		if (PaintModes == 0)
		{
			/* Clean up scan line buffers. */
			ScanLineCleanup(Context, Path);
			return gcvTRUE;
		}
	}

	do
	{
		/* Construct vgSCANLINETESS structure. */
		gcmERR_BREAK(vgSL_ConstructScanLineTess(Context, &scanLineTess));

		scanLineTess->userMatrix = Matrix;
		scanLineTess->paintModes = PaintModes;
#if USE_QUALITY_SCALE
		scanLineTess->qualityScale = qualityScale;
#endif

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_LOG_FILENAME, gcvFILE_APPENDTEXT, &logFile));
#else
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_CMD_FILENAME, gcvFILE_APPENDTEXT, &cmdFile));
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_COORD_FILENAME, gcvFILE_APPENDTEXT, &coordFile));
#endif
#endif

		/* Calculate internal quality scale. */
		vgSL_CalculateQualityScaleAndMatrix(scanLineTess, Path, Matrix);

		/* Flatten path if necessary and create subpaths. */
		gcmERR_BREAK(vgSL_FlattenPath(scanLineTess, Path));

		if (PaintModes & VG_STROKE_PATH)
		{
			/* Calculate deltaX and orientation for each point/line without filtering points. */
			vgSL_CalculatePathDeltaXAndOrientation(scanLineTess, scanLineTess->subPathList, VG_FALSE);

			/* Create sub paths from flattened path for stroke. */
			gcmERR_BREAK(vgSL_CreateStrokePath(scanLineTess));
		}

		/* Filling tessellation. */
		if (PaintModes & VG_FILL_PATH)
		{
			/* Transform flattened path. */
			vgSL_TransformPath(scanLineTess, scanLineTess->subPathList, 2 /* Get range + extra range for stroke. */);

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
			/* Output path cmd and coordinates to log files. */
			gcmERR_BREAK(vgSL_OutputPath(scanLineTess, scanLineTess->subPathList, logFile, logFile));
#else
			/* Output path cmd and coordinates to log files. */
			gcmERR_BREAK(vgSL_OutputPath(scanLineTess, scanLineTess->subPathList, cmdFile, coordFile));
#endif
#endif
			/* Calculate deltaX and orientation for each point/line with filtering points. */
			vgSL_CalculatePathDeltaXAndOrientation(scanLineTess, scanLineTess->subPathList, VG_TRUE);

			/* Create scan line array with x coordinates for each y. */
			gcmERR_BREAK(vgSL_BuildScanLineArray(scanLineTess, scanLineTess->subPathList));

			/* Set fill rule for fill drawing. */
			scanLineTess->fillRule = VGSL_GETCONTEXTFILLRULE(Context);

			/* Create scan lines or triangles. */
#if SCAN_LINE_HAS_3D_SUPPORT
			if (createTriangles)
			{
				gcmERR_BREAK(vgSL_CreateTriangles(scanLineTess, &Path->fillIndexBuffer,
												&Path->fillVertexBuffer));
			}
			else
#endif
			{
				gcmERR_BREAK(vgSL_CreateScanLines(scanLineTess, &(VGSL_GETPATHFILLBUFFER(Path))));
			}
		}

		/* Stroking tessellation. */
		if (PaintModes & VG_STROKE_PATH)
		{
			/* Transform flattened path. */
			vgSL_TransformPath(scanLineTess, scanLineTess->strokeSubPathList,
							PaintModes & VG_FILL_PATH ? 0  /* No need to get range. */ : 1 /* Get range. */);

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
			/* Output path cmd and coordinates to log files. */
			gcmERR_BREAK(vgSL_OutputPath(scanLineTess, scanLineTess->strokeSubPathList, logFile, logFile));
#else
			/* Output path cmd and coordinates to log files. */
			gcmERR_BREAK(vgSL_OutputPath(scanLineTess, scanLineTess->strokeSubPathList, cmdFile, coordFile));
#endif
#endif
			/* Calculate deltaX and orientation for each point/line with filtering points. */
			vgSL_CalculatePathDeltaXAndOrientation(scanLineTess, scanLineTess->strokeSubPathList, VG_TRUE);

			/* Create scan line array with x coordinates for each y. */
			gcmERR_BREAK(vgSL_BuildScanLineArray(scanLineTess, scanLineTess->strokeSubPathList));

			/* Set fill rule to non-zero for stroke drawing. */
			scanLineTess->fillRule = VG_NON_ZERO;

			/* Create scan lines or triangles. */
#if SCAN_LINE_HAS_3D_SUPPORT
			if (createTriangles)
			{
				gcmERR_BREAK(vgSL_CreateTriangles(scanLineTess, &Path->strokeIndexBuffer,
												&Path->strokeVertexBuffer));
			}
			else
#endif
			{
				gcmERR_BREAK(vgSL_CreateScanLines(scanLineTess, &(VGSL_GETPATHSTROKEBUFFER(Path))));
			}
		}
	}
	while (gcvFALSE);

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, logFile));
#else
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, cmdFile));
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, coordFile));
#endif
#endif

	/* Clean up. */
	if (scanLineTess != gcvNULL)
	{
		/* Free vgSCANLINETESS structure. */
		gcmVERIFY_OK(vgSL_DestroyScanLineTess(scanLineTess));
		scanLineTess = gcvNULL;
	}

	if (status != gcvSTATUS_OK)
	{
		/* Clean up scan line buffers. */
		ScanLineCleanup(Context, Path);
	}

	return (status == gcvSTATUS_OK);
}

/*******************************************************************************
**								ScanLineCleanup
********************************************************************************
**
**	Cleanup scan lines in a path.
**
**	INPUT:
**
**		_VGContext * Context
**			Pointer to a _VGContext structure holding the environment data.
**
**		_VGPath * Path
**			Pointer to a _VGPath structure holding path data.
*/
gctBOOL
ScanLineCleanup(
	IN _VGContext *		Context,
	IN _VGPath *		Path
	)
{
#if USE_SCAN_LINE_ON_RI
	ARRAY_CLEAR(Path->m_scanLineFillBuffer.data);
	ARRAY_CLEAR(Path->m_scanLineStrokeBuffer.data);
#else
	ARRAY_DTOR(Path->scanLineFillBuffer.data);
	ARRAY_DTOR(Path->scanLineStrokeBuffer.data);
#if SCAN_LINE_HAS_3D_SUPPORT
	ARRAY_DTOR(Path->fillIndexBuffer.data);
	ARRAY_DTOR(Path->fillVertexBuffer.data);
	ARRAY_DTOR(Path->strokeIndexBuffer.data);
	ARRAY_DTOR(Path->strokeVertexBuffer.data);
#endif
#endif

	return gcvTRUE;
}
#endif

/*******************************************************************************
**								CreateFillPathForStrokePath
********************************************************************************
**
**	Create a fill path for a stroke path.
**
**	INPUT:
**
**		_VGContext * Context
**			Pointer to a _VGContext structure holding the environment data.
**
**		_VGPath * InputPath
**			Pointer to a _VGPath structure holding input path data.
**
**		_VGPath * OutputPath
**			Pointer to a _VGPath structure holding output path data.
**
**		_VGMatrix3x3 * Matrix
**			Pointer to a _VGMatrix3x3 structure holding user-to-surface matrix.
*/
gctBOOL
CreateFillPathForStrokePath(
	IN _VGContext *		Context,
	IN _VGPath *		InputPath,
	IN _VGPath *		OutputPath,
	IN _VGMatrix3x3 *	Matrix
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSCANLINETESS		scanLineTess = gcvNULL;
#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
	gctFILE				logFile = gcvNULL;
#else
	gctFILE				cmdFile = gcvNULL;
	gctFILE				coordFile = gcvNULL;
#endif
#endif

	if (VGSL_GETCONTEXTSTOKELINEWIDTH(Context) <= 0.0f)
	{
		/* The stroke path would be empty. */
#if gcdUSE_VG
		/* TODO - Cleanup/destroy OutputPath's data. */
#else
		ARRAY_DTOR(VGSL_GETPATHSEGMENTS(OutputPath));
		ARRAY_DTOR(VGSL_GETPATHDATA(OutputPath));
#endif
		return gcvTRUE;
	}

	do
	{
		/* Construct vgSCANLINETESS structure. */
		gcmERR_BREAK(vgSL_ConstructScanLineTess(Context, &scanLineTess));
		scanLineTess->paintModes = VG_STROKE_PATH;

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_LOG_FILENAME, gcvFILE_APPENDTEXT, &logFile));
#else
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_CMD_FILENAME, gcvFILE_APPENDTEXT, &cmdFile));
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_COORD_FILENAME, gcvFILE_APPENDTEXT, &coordFile));
#endif
#endif

		/* Calculate internal quality scale. */
		vgSL_CalculateQualityScaleAndMatrix(scanLineTess, InputPath, Matrix);

		/* Flatten InputPath if necessary and create subpaths. */
		gcmERR_BREAK(vgSL_FlattenPath(scanLineTess, InputPath));

		/* Create sub paths from flattened path for stroke. */
		gcmERR_BREAK(vgSL_CreateStrokePath(scanLineTess));

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
		/* Output path cmd and coordinates to log files. */
		gcmERR_BREAK(vgSL_OutputPath(scanLineTess, scanLineTess->strokeSubPathList, logFile, logFile));
#else
		/* Output path cmd and coordinates to log files. */
		gcmERR_BREAK(vgSL_OutputPath(scanLineTess, scanLineTess->strokeSubPathList, cmdFile, coordFile));
#endif
#endif

		/* Copy stroke path to OutputPath. */
		gcmERR_BREAK(vgSL_CopyStrokePath(scanLineTess, InputPath, OutputPath));
	}
	while (gcvFALSE);

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, logFile));
#else
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, cmdFile));
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, coordFile));
#endif
#endif

	/* Clean up. */
	if (scanLineTess != gcvNULL)
	{
		/* Free vgSCANLINETESS structure. */
		gcmVERIFY_OK(vgSL_DestroyScanLineTess(scanLineTess));
		scanLineTess = gcvNULL;
	}

	return (status == gcvSTATUS_OK);
}

#if !gcdUSE_VG
/*******************************************************************************
**								NeedToSimplifyPath
********************************************************************************
**
**	Check if a path has elliptical arcs to simplify.
**
**	INPUT:
**		_VGPath * Path
**			Pointer to a _VGPath structure holding input path data.
*/
gctBOOL
NeedToSimplifyPath(
	IN _VGPath *		Path
	)
{
	VGint				numCommands;			// Number of commands in InputPath.
	VGbyte				pathCommand;			// Path command of each segment in InputPath.
	VGint				i;

	numCommands = VGSL_GETPATHSEGMENTSSIZE(Path);
	if (numCommands == 0) return gcvFALSE;

	for (i = 0; i < numCommands; i++)
	{
		pathCommand = (ARRAY_ITEM(VGSL_GETPATHSEGMENTS(Path), i) & 0x1e);

		switch (pathCommand)
		{
		case VG_HLINE_TO:
		case VG_VLINE_TO:
		case VG_SQUAD_TO:
		case VG_SCUBIC_TO:
		case VG_SCCWARC_TO:
		case VG_LCCWARC_TO:
		case VG_SCWARC_TO:
		case VG_LCWARC_TO:
			return gcvTRUE;

		default:
			break;
		}
	}

	return gcvFALSE;
}

/*******************************************************************************
**								SimplifyPath
********************************************************************************
**
**	Simplify a path by converting HLINE_TO/VLINE_TO to LINE_TO,
**  and converting elliptical arcs to bezier curves.
**
**	INPUT:
**
**		_VGContext * Context
**			Pointer to a _VGContext structure holding the environment data.
**
**		_VGPath * InputPath
**			Pointer to a _VGPath structure holding input path data.
**
**		_VGPath * OutputPath
**			Pointer to a _VGPath structure holding output path data.
**
**		_VGMatrix3x3 * Matrix
**			Pointer to a _VGMatrix3x3 structure holding user-to-surface matrix.
*/
gctBOOL
SimplifyPath(
	IN _VGContext *		Context,
	IN _VGPath *		InputPath,
	IN _VGPath *		OutputPath,
	IN _VGMatrix3x3 *	Matrix
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	vgSCANLINETESS		scanLineTess = gcvNULL;
#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
	gctFILE				logFile = gcvNULL;
#else
	gctFILE				cmdFile = gcvNULL;
	gctFILE				coordFile = gcvNULL;
#endif
#endif

	do
	{
		/* Construct vgSCANLINETESS structure. */
		gcmERR_BREAK(vgSL_ConstructScanLineTess(Context, &scanLineTess));

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_LOG_FILENAME, gcvFILE_APPENDTEXT, &logFile));
#else
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_CMD_FILENAME, gcvFILE_APPENDTEXT, &cmdFile));
		gcmERR_BREAK(gcoOS_Open(scanLineTess->os, TEST_COORD_FILENAME, gcvFILE_APPENDTEXT, &coordFile));
#endif
#endif

		/* Calculate internal quality scale. */
		vgSL_CalculateQualityScaleAndMatrix(scanLineTess, InputPath, Matrix);

		/* Simplify InputPath if necessary and create new OutputPath. */
		gcmERR_BREAK(vgSL_SimplifyPath(scanLineTess, InputPath, OutputPath));

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
		/* Output path cmd and coordinates to log files. */
		gcmERR_BREAK(vgSL_OutputPath(scanLineTess, scanLineTess->strokeSubPathList, logFile, logFile));
#else
		/* Output path cmd and coordinates to log files. */
		gcmERR_BREAK(vgSL_OutputPath(scanLineTess, scanLineTess->strokeSubPathList, cmdFile, coordFile));
#endif
#endif
	}
	while (gcvFALSE);

#if USE_TEST_LOG
#if USE_ONE_TEST_LOG
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, logFile));
#else
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, cmdFile));
	gcmVERIFY_OK(gcoOS_Close(scanLineTess->os, coordFile));
#endif
#endif

	/* Clean up. */
	if (scanLineTess != gcvNULL)
	{
		/* Free vgSCANLINETESS structure. */
		gcmVERIFY_OK(vgSL_DestroyScanLineTess(scanLineTess));
		scanLineTess = gcvNULL;
	}

	return (status == gcvSTATUS_OK);
}
#endif

#else /* USE_SCAN_LINE */
void
gc_vgsh_scanline_FunctionToKeepStrictCompilerOptionHappy(
	void
	)
{
	/* a translation unit must contain at least one declaration */
}
#endif /* USE_SCAN_LINE */
