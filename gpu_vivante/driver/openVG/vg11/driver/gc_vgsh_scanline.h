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




/*
**	Scan line (new tessellation) module.
*/

#ifndef __gc_vgsh_scanline_h_
#define __gc_vgsh_scanline_h_

#if USE_SCAN_LINE

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
	);

/*******************************************************************************
**								ScanLineCleanup
********************************************************************************
**
**	Cleanup scan lines in Context.
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
	);

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
	);

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
	);

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
	);

/*******************************************************************************
**						ScanLine Data Type and Algorithm Control
*******************************************************************************/
/* Epsilon used for rounding y coordinate. */
#if USE_FIXED_POINT_FOR_COORDINATES
#define COORD_ZERO			0
#define COORD_EPSILON		64
#define COORD_ROUNDING_EPSILON		64
#define COORD_ONE			(1 << FRACTION_BITS)
#define COORD_TWO			(2 << FRACTION_BITS)
#define COORD_FOUR			(4 << FRACTION_BITS)
#define COORD_360			(360 << FRACTION_BITS)
#define COORD_ONE_HALF		(1 << (FRACTION_BITS - 1))
#define COORD_ONE_FOURTH	(1 << (FRACTION_BITS - 2))
#define COORD_TWO_AND_HALF	(5 << (FRACTION_BITS - 1))
#define COORD_MIN			((vgCOORD) 0x80000000)
#define COORD_MAX			((vgCOORD) 0x7fffffff)
#define COORD_INT(X)		((vgCOORD)((VGuint)(X) & 0xffff0000))
#define COORD_FRAC(X)		((vgCOORD)((VGuint)(X) & 0x0000ffff))
#define COORD_CEIL(X)		COORD_INT(X + COORD_ONE - 1)
#define COORD_FLOOR(X)		COORD_INT(X)
#define COORD_ABS(X)		abs(X)
#define COORD_SQRT(X)		FLOAT_TO_COORD(gcoMATH_SquareRoot(COORD_TO_FLOAT(X)))
#define COORD_SIN(X)		FLOAT_TO_COORD(gcoMATH_Sine(COORD_TO_FLOAT(X)))
#define COORD_COS(X)		FLOAT_TO_COORD(gcoMATH_Cosine(COORD_TO_FLOAT(X)))
#define COORD_TAN(X)		FLOAT_TO_COORD(gcoMATH_Tangent(COORD_TO_FLOAT(X)))
#define COORD_ACOS(X)		FLOAT_TO_COORD(gcoMATH_ArcCosine(COORD_TO_FLOAT(X)))
#define COORD_MUL(X1, X2)	((vgCOORD) (((gctINT64) (X1) * (X2)) >> FRACTION_BITS))
#define COORD_DIV(X1, X2)	((vgCOORD) ((((gctINT64) (X1)) << FRACTION_BITS) / (X2)))
#define COORD_MUL_BY_TWO(X)	((X) << 1)
#define COORD_DIV_BY_TWO(X)	((X) >> 1)
#define COORD_TO_INT(X)		((VGint)((X) >> FRACTION_BITS))
#define INT_TO_COORD(X)		((vgCOORD)((X) << FRACTION_BITS))
#define FLOAT_TO_COORD(X)	((vgCOORD)((X) * COORD_ONE))
#define COORD_TO_FLOAT(X)	((VGfloat)(X) / COORD_ONE)
#else
#define COORD_ZERO			0.0f
/*#define COORD_EPSILON		0.001f*/
#define COORD_EPSILON		1.0e-20f
#define COORD_ROUNDING_EPSILON		0.001f
#define COORD_ONE			1.0f
#define COORD_TWO			2.0f
#define COORD_FOUR			4.0f
#define COORD_360			360.0f
#define COORD_ONE_HALF		0.5f
#define COORD_ONE_FOURTH	0.25f
#define COORD_TWO_AND_HALF	2.5f
#define COORD_MIN			-3.4e+38F
#define COORD_MAX			3.4e+38F
#define COORD_INT(X)		((VGint)(X))
#define COORD_CEIL(X)		gcoMATH_Ceiling((X) - COORD_ROUNDING_EPSILON)
#define COORD_FLOOR(X)		gcoMATH_Floor((X) + COORD_ROUNDING_EPSILON)
#define COORD_ABS(X)		gcoMATH_Absolute(X)
#define COORD_SQRT(X)		gcoMATH_SquareRoot(X)
#define COORD_SIN(X)		gcoMATH_Sine(X)
#define COORD_COS(X)		gcoMATH_Cosine(X)
#define COORD_TAN(X)		gcoMATH_Tangent(X)
#define COORD_ACOS(X)		gcoMATH_ArcCosine(X)
#define COORD_MUL(X1, X2)	((X1) * (X2))
#define COORD_DIV(X1, X2)	((X1) / (X2))
#define COORD_MUL_BY_TWO(X)	((X) * COORD_TWO)
#define COORD_DIV_BY_TWO(X)	((X) * COORD_ONE_HALF)
#define COORD_TO_INT(X)		((VGint)(X))
#define INT_TO_COORD(X)		((vgCOORD)(X))
#define FLOAT_TO_COORD(X)	(X)
#define COORD_TO_FLOAT(X)	(X)
#endif

#define DIFF_EPSILON		0.1f
#define DIFF_EPSILON_RCP	(1.0f / DIFF_EPSILON)
#define PI_RCP				(1.0f / PI)
#define PI_TWO				(PI + PI)
#define PI_HALF				(0.5f * PI)

#define FAT_LINE_WIDTH		COORD_TWO_AND_HALF

/*******************************************************************************
**							Memory Pool Data Structures
*******************************************************************************/
typedef struct _vgSL_MEM_POOL			vgSL_MEM_POOL;
typedef struct _vgSL_MEM_NODE *			vgSL_MEM_NODE;
typedef struct _vgSL_MEM_ARRAY_POOL		vgSL_MEM_ARRAY_POOL;
typedef struct _vgSL_MEM_ARRAY_NODE *	vgSL_MEM_ARRAY_NODE;

struct _vgSL_MEM_NODE
{
	/* Next memory node. */
	vgSL_MEM_NODE			next;

	/* Data area - should not be used */
	gctPOINTER				data;
};

struct _vgSL_MEM_POOL
{
	/* Pointer to the os. */
	gcoOS					os;

	/* Allocated block list. */
	vgSL_MEM_NODE			blockList;

	/* Free node list. */
	vgSL_MEM_NODE			freeList;

	/* Number of nodes in each memory block. */
	gctUINT					nodeCount;

	/* Node size. */
	gctUINT					nodeSize;
};

struct _vgSL_MEM_ARRAY_NODE
{
	/* Previous memory array node. */
	vgSL_MEM_ARRAY_NODE		prev;

	/* Next memory array node. */
	vgSL_MEM_ARRAY_NODE		next;

	/* Number of nodes in each memory block. */
	gctUINT					nodeCount;

	/* Data area - should not be used. */
	gctPOINTER				data;
};

struct _vgSL_MEM_ARRAY_POOL
{
	/* Pointer to the os. */
	gcoOS					os;

	/* Allocated block list. */
	vgSL_MEM_ARRAY_NODE		blockList;

	/* Free node list */
	vgSL_MEM_ARRAY_NODE		freeList;

	/* Node size. */
	gctUINT					nodeSize;
};

/*******************************************************************************
**							Scan Line Data Structures
*******************************************************************************/
typedef struct _vgSCANLINETESS *		vgSCANLINETESS;
typedef struct _vgSL_SUBPATH *			vgSL_SUBPATH;
typedef struct _vgSL_POINT *			vgSL_POINT;
typedef struct _vgSL_YSCANLINE *		vgSL_YSCANLINE;
typedef struct _vgSL_XCOORD *			vgSL_XCOORD;
typedef struct _vgSL_HLINE *			vgSL_HLINE;
typedef struct _vgSL_VECTOR2 *			vgSL_VECTOR2;

#if SCAN_LINE_HAS_3D_SUPPORT
typedef struct _vgSL_REGION *			vgSL_REGION;
typedef struct _vgSL_REGIONPOINT *		vgSL_REGIONPOINT;
typedef struct _vgSL_TRIANGLE *			vgSL_TRIANGLE;
#endif

typedef _VGcoord						vgCOORD;

typedef VGbyte							vgSL_ORIENTATION;

/* Values of vgSL_ORIENTATION. */
#define vgvSL_UP		 1
#define vgvSL_FLAT		 0
#define vgvSL_DOWN		-1
/* Special orientation for cross point that does not have any direction. */
#define vgvSL_CROSSPOINT 2

/* Sub path. */
struct _vgSL_SUBPATH
{
	/* Number of points. */
	VGint						pointCount;

	/* Point list. */
	vgSL_POINT					pointList;

	/* Last point. */
	vgSL_POINT					lastPoint;

	/* Whether is path is closed. */
	VGboolean					closed;

	/* Pointer to next sub path. */
	vgSL_SUBPATH				next;
};

/* Point node in a sub path. */
struct _vgSL_POINT
{
	/* X coordinate. */
	vgCOORD						x;

	/* Y coordinate. */
	vgCOORD						y;

	/* DeltaX of the line formed by this point and next point. */
	vgCOORD						deltaX;

	/* Orientation of the point. */
	vgSL_ORIENTATION			orientation;

	/* y is an integer. */
	VGbyte						yIsInteger;

	/* Point is a flattened point (not a point in the original path). */
	VGbyte						isFlattened;

	/* X tangent. */
	vgCOORD						tangentX;

	/* Y tangent. */
	vgCOORD						tangentY;

	/* Angle of the line. */
	vgCOORD						angle;

	/* Length of the line. */
	vgCOORD						length;

#if SCAN_LINE_HAS_3D_SUPPORT
	/* Id of the point. */
	VGshort						id;

	/* When used as line, the lower point is on integer y line. */
	VGbyte						lowerPointOnYLine:1;
	VGbyte						higherPointOnYLine:1;

	/* Number of yScanLines crossed. */
	VGint						crossYLinesCount;

	/* Pointer to previous point node. */
	vgSL_REGIONPOINT			crossPointList;

	/* Region using this line as leftLine/rightLine. */
	vgSL_REGION					regionWLeft;
	vgSL_REGION					regionWRight;
#endif

	/* Pointer to previous point node. */
	vgSL_POINT					prev;

	/* Pointer to next point node. */
	vgSL_POINT					next;
};

/* Scan line for y coordinate. */
struct _vgSL_YSCANLINE
{
	/* Y coordinate. */
	vgCOORD						y;

	/* Number of x coordinates. */
	VGint						xcoordCount;

	/* X coordinate. */
	vgSL_XCOORD					xcoords;

#if SCAN_LINE_HAS_3D_SUPPORT
	/* Number of line end points. */
	VGint						numLineEnds;
#endif
};

/* X coordinate node for y scan lines. */
struct _vgSL_XCOORD
{
	/* X coordinate. */
	vgCOORD						x;

	/* Orientation of the point. */
	vgSL_ORIENTATION			orientation;

#if SCAN_LINE_HAS_3D_SUPPORT
	/* End of line status: none=0, begining=1, end=2, both=3. */
	VGbyte						endLineStatus;

	/* Subpath number. */
	VGbyte						subPath;

	/* Pointer to the first point of the line. */
	vgSL_POINT					line;
#endif

	/* Pointer to next x coord node. */
	vgSL_XCOORD					next;
};

/* Two floats vector. */
struct _vgSL_VECTOR2
{
	/* X coordinate. */
	vgCOORD						x;

	/* Y coordinate. */
	vgCOORD						y;
};

#if SCAN_LINE_HAS_3D_SUPPORT
/* Region. */
struct _vgSL_REGION
{
	/* Bottom point. */
	vgSL_POINT					bottomPoint;

	/* Left point list. */
	vgSL_REGIONPOINT			leftPointList;

	/* Left xCoord in previous yScanLine. */
	vgSL_XCOORD					leftXCoord;

	/* Left line. */
	vgSL_POINT					leftLine;

	/* Left deltaX. */
	vgCOORD						leftDeltaX;

	/* Orientation of the left branch. */
	vgSL_ORIENTATION			leftOrientation;

	/* Right point list. */
	vgSL_REGIONPOINT			rightPointList;

	/* Right xCoord in previous yScanLine. */
	vgSL_XCOORD					rightXCoord;

	/* Right line. */
	vgSL_POINT					rightLine;

	/* Left deltaX. */
	vgCOORD						rightDeltaX;

	/* Orientation of the right branch. */
	vgSL_ORIENTATION			rightOrientation;

	/* True if left branch uses next pointer. */
	VGboolean					isLeftNext;

	/* True if right branch uses next pointer. */
	VGboolean					isRightNext;

	/* Pointer to previous region. */
	vgSL_REGION					prev;

	/* Pointer to next region. */
	vgSL_REGION					next;
};

/* Point node in a sub path. */
struct _vgSL_REGIONPOINT
{
	/* Point. */
	vgSL_POINT					point;

	/* deltaX of the line formed by this point and next point. */
	vgCOORD						deltaX;

	/* Pointer to next x coord node. */
	vgSL_REGIONPOINT			next;
};

/* Triangle. */
struct _vgSL_TRIANGLE
{
	/* Points. */
	vgSL_POINT					point1;
	vgSL_POINT					point2;
	vgSL_POINT					point3;

	/* Pointer to next triangle. */
	vgSL_TRIANGLE				next;
};
#endif

struct _vgSCANLINETESS
{
	/* Pointer to the context. */
	_VGContext *				context;

	/* Pointer to the os. */
	gcoOS						os;

	/* User to surface matrix. */
	_VGMatrix3x3 *				userMatrix;

	/* Path scale and bias. */
	VGfloat						pathScale;
	VGfloat						pathBias;

	/* Combined matrix from path's scale/bias and user-to-surface matrix. */
	vgCOORD						matrix[3][3];

	/* Internal quality scale and bias. */
	vgCOORD						scale;
	VGint						scaleShiftBits;
	VGboolean					scaleUp;
	vgCOORD						strokeScale;
	vgCOORD						transformScale;
	vgCOORD						bias;

	/* Paint modes. */
	VGbitfield					paintModes;

	/* Fill rule. */
	VGFillRule					fillRule;

	/* Stroke parameters */
	vgCOORD						strokeLineWidth;
	VGCapStyle					strokeCapStyle;
	VGJoinStyle					strokeJoinStyle;
	vgCOORD						strokeMiterLimit;
	vgCOORD *					strokeDashPattern;
	VGint						strokeDashPatternCount;
	vgCOORD						strokeDashPhase;
	VGboolean					strokeDashPhaseReset;
	vgCOORD						strokeDashInitialLength;
	VGint						strokeDashInitialIndex;

	/* Total length of stroke dash patterns. */
	vgCOORD						strokeDashPatternLength;

	/* In the middle of curve flattening. */
	VGboolean					isFlattening;

	/* Range of yScanLineArray. */
	VGint						yMin;
	VGint						yMax;

	/* Size of scan line array. */
	VGint						yScanLineArraySize;

	/* Scan line array. */
	vgSL_YSCANLINE				yScanLineArray;

	/* Sub path list. */
	vgSL_SUBPATH				subPathList;

	/* Last sub path. */
	vgSL_SUBPATH				lastSubPath;

	/* Sub path list for stroke path. */
	vgSL_SUBPATH				strokeSubPathList;

	/* Last sub path for stroke path. */
	vgSL_SUBPATH				lastStrokeSubPath;

	/* Temp storage of stroke subPath. */
	vgSL_POINT					leftStrokePoint;
	vgSL_POINT					lastRightStrokePoint;

	/* Circle for the end cap. */
	vgSL_VECTOR2				endCapTable;
	VGint						endCapTableQuadrantSize;
	VGint						endCapTableIncrement;

	/* Min/Max of points. */
	vgCOORD						minXCoord;
	vgCOORD						maxXCoord;
	vgCOORD						minYCoord;
	vgCOORD						maxYCoord;

	/* Orientations while parsing sub path. */
	vgSL_ORIENTATION			prevPrevOrientation;
	vgSL_ORIENTATION			prevOrientation;
	vgSL_ORIENTATION			currentOrientation;

	/* Quality scale to ensure picture quality. */
	VGint						qualityScale;

#if SCAN_LINE_HAS_3D_SUPPORT
	/* Region data for triangulation. */
	vgSL_REGION					regionList;
	vgSL_REGION					lastRegion;
	vgSL_REGION					currentRegion;

	/* Triangle list. */
	vgSL_TRIANGLE				triangleList;
	VGint						triangleCount;

	/* Data for vertex buffer. */
	VGint						pointCount;

	/* Cross points caused by crossed edges. */
	VGint						crossPointCount;
	vgSL_POINT					crossPointList;
#endif

	/* Memory pools. */
	vgSL_MEM_POOL				subPathMemPool;
	vgSL_MEM_POOL				pointMemPool;
	vgSL_MEM_POOL				xCoordMemPool;
#if SCAN_LINE_HAS_3D_SUPPORT
	vgSL_MEM_POOL				regionMemPool;
	vgSL_MEM_POOL				regionPointMemPool;
	vgSL_MEM_POOL				triangleMemPool;
#endif
	vgSL_MEM_ARRAY_POOL			yScanLineMemArrayPool;
	vgSL_MEM_ARRAY_POOL			vector2MemArrayPool;
};

#endif /* USE_SCAN_LINE */

#endif /* __gc_vgsh_scanline_h_ */
