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
#include "gc_hal_mem.h"
#include <math.h>

/*******************************************************************************
**						Stroke Conversion Algorithm Control
*******************************************************************************/
#define vgvENABLE_OPTIMIZATIONS			1

#define USE_NEW_SWING_HANDLE_FOR_END	0

#define USE_LOCAL_SQRT					1
#define USE_MIN_ARC_FILTER				1

#define OUTPUT_FLATTENED_PATH			0
#define OUTPUT_STROKE_PATH				0
#define OUTPUT_STROKE_PATH_INCLUDE_CMD	0
#define PATH_LOG_FILENAME				"path.log"

#define vgvPRINT_DIAG					0
#define vgvPRINT_PATH_DIAG				0

#define COLLECT_STROKE_STAT				0

#if vgvENABLE_OPTIMIZATIONS
#	define USE_FIXED_POINT				1
#	define FAST_FIXED_EPSILON			FIXED_EIGHTH
#	define vgvENABLE_ANGLE_RELAXATION	0
#	define gcvRELAX_FACTOR				10

#	define gcvENABLE_STROKE_CACHE		1

#	define gcvSEGMENT_COUNT				0

#if gcdENABLE_PERFORMANCE_PRIOR
#	define gcvUSE_INTERNAL_SCALE		1
#	define gcvRELAX_FAT_LINE			1
#	define gcvUSE_FAST_FAT_LINE_HANDLE	1
#	define gcvRELAX_BEZIER_CHECK		1
#else
#	define gcvUSE_INTERNAL_SCALE		1
#	define gcvRELAX_FAT_LINE			0
#	define gcvUSE_FAST_FAT_LINE_HANDLE	0
#	define gcvRELAX_BEZIER_CHECK		0
#endif

#	define gcvSTROKE_REDUCE_SEGMENT		0
#	define gcvCALCULATE_HALF_TANGENT	0
#	define gcvUSE_FAST_STROKE			1
#	define gcvUSE_FAST_COPY				1
#	define gcvUSE_SQRT_TABLE			0

#else
#	define USE_FIXED_POINT				1
#	define vgvENABLE_ANGLE_RELAXATION	0

#	define gcvENABLE_STROKE_CACHE		1

#	define gcvSEGMENT_COUNT				0

#	define gcvUSE_INTERNAL_SCALE		0
#	define gcvSTROKE_REDUCE_SEGMENT		0
#	define gcvCALCULATE_HALF_TANGENT	0
#	define gcvUSE_FAST_STROKE			0
#	define gcvUSE_FAST_COPY				0
#	define gcvRELAX_FAT_LINE			0
#	define gcvUSE_SQRT_TABLE			0
#	define gcvUSE_FAST_FAT_LINE_HANDLE	0
#endif

/*******************************************************************************
**						Stroke Conversion Control Parameters
*******************************************************************************/

/* Control Parameters in floating point format. */
/* No relaxation for floating point implementation, which is for conformance test. */
/* Default. */
#if gcvRELAX_FAT_LINE
#define FLOAT_FAT_LINE_WIDTH			6.0f
#else
#define FLOAT_FAT_LINE_WIDTH			2.5f
#endif
#define FLOAT_DIFF_EPSILON				0.125f
#define FLOAT_SWING_CENTER_RANGE		0.125f
#define FLOAT_ANGLE_EPSILON				0.0045f
#define FLOAT_ANGLE_EPSILON_COS			0.99999f

/* 5 degree = 0.0872665f */
#define FLOAT_MIN_ARC_ANGLE				0.044f
#define FLOAT_MIN_ARC_ANGLE_COS			0.999f

/* Control Parameters in fixed point format. */
#if vgvENABLE_ANGLE_RELAXATION
#	define FIXED_RELAX_FACTOR			gcvRELAX_FACTOR

#if gcvRELAX_FAT_LINE
#		define FIXED_FAT_LINE_WIDTH		FIXED_SIX
#	else
#		define FIXED_FAT_LINE_WIDTH		FIXED_TWO_AND_HALF
#	endif
#	define FIXED_DIFF_EPSILON			FIXED_EIGHTH
#	define FIXED_SWING_CENTER_RANGE		FIXED_EIGHTH
#	define FIXED_ANGLE_EPSILON			((gctFIXED) 0x000002D4 * FIXED_RELAX_FACTOR)
#	define FIXED_ANGLE_EPSILON_COS		((gctFIXED) (cos(((gctFLOAT) FIXED_ANGLE_EPSILON) / 65536.0f) * 65536.0))

#	define FIXED_MIN_ARC_ANGLE			((gctFIXED) 0x00000B60 * FIXED_RELAX_FACTOR)
#	define FIXED_MIN_ARC_ANGLE_COS		((gctFIXED) (cos(((gctFLOAT) FIXED_MIN_ARC_ANGLE) / 65536.0f) * 65536.0))

#	define FIXED_MIN_BEZIER_LENGTH		FIXED_EIGHTH
#else
	/* Default. */
#if gcvRELAX_FAT_LINE
#		define FIXED_FAT_LINE_WIDTH		FIXED_SIX
#	else
#		define FIXED_FAT_LINE_WIDTH		FIXED_TWO_AND_HALF
#	endif
#	define FIXED_DIFF_EPSILON			FIXED_EIGHTH
#	define FIXED_SWING_CENTER_RANGE		FIXED_EIGHTH
#	define FIXED_ANGLE_EPSILON			((gctFIXED) 0x000002D4)
#	define FIXED_ANGLE_EPSILON_COS		((gctFIXED) 0x0000FFFC)

	/* 5 degree = 0.0872665f */
	/* MIN_ARC_ANGLE = 0.044f */
#	define FIXED_MIN_ARC_ANGLE			((gctFIXED) 0x00000B60)
#	define FIXED_MIN_ARC_ANGLE_COS		((gctFIXED) 0x0000FFC0)
#if gcvRELAX_BEZIER_CHECK
#	    define FIXED_MIN_BEZIER_LENGTH		FIXED_SIXTEENTH
#   else
#	    define FIXED_MIN_BEZIER_LENGTH		FIXED_EIGHTH
#   endif
#endif

#if gcvQUAD_STATISTICS
gctUINT g_minQuadSegments = ~0;
gctUINT g_maxQuadSegments =  0;
gctUINT g_avrQuadSegments = ~0;
#endif

#if gcvCUBIC_STATISTICS
gctUINT g_minCubicSegments = ~0;
gctUINT g_maxCubicSegments =  0;
gctUINT g_avrCubicSegments = ~0;
#endif


/*******************************************************************************
**						Stroke Conversion Data Type and Constants
*******************************************************************************/
/* Constants in floating point format. */
#define FLOAT_EPSILON				0.001f
#define FLOAT_MIN					gcvMAX_NEG_FLOAT
#define FLOAT_MAX					gcvMAX_POS_FLOAT

#define FLOAT_PI					3.141592654f
#define FLOAT_PI_TWO				6.283185307f
#define FLOAT_PI_THREE_QUARTER		2.356194490f
#define FLOAT_PI_HALF				1.570796327f
#define FLOAT_PI_QUARTER			0.7853981634f
#define FLOAT_PI_EIGHTH				0.3926990817f
/* cos(PI/8) */
#define FLOAT_COS_PI_EIGHTH			0.9238795325f

/* Floating point operations. */
#define FLOAT_CEIL(X)				((gctFLOAT)ceil((double)(X) - FLOAT_EPSILON))
#define FLOAT_FLOOR(X)				((gctFLOAT)floor((double)(X) + FLOAT_EPSILON))
#define FLOAT_SQRT(X)				((gctFLOAT)sqrt((double)(X)))


#define FIXED_INTEGER_BITS			16
#define FIXED_FRACTION_BITS			16

/* Epsilon is a little less than 0.001. */
#define FIXED_EPSILON				((gctFIXED) 0x00000040)
#define FIXED_MIN					((gctFIXED) 0x80000000)
#define FIXED_MAX					((gctFIXED) 0x7FFFFFFF)

#define FIXED_ZERO					((gctFIXED) 0x00000000)
#define FIXED_ONE					((gctFIXED) 0x00010000)
#define FIXED_TWO					((gctFIXED) 0x00020000)
#define FIXED_HALF					((gctFIXED) 0x00008000)
#define FIXED_QUARTER				((gctFIXED) 0x00004000)
#define FIXED_EIGHTH				((gctFIXED) 0x00002000)
#define FIXED_SIXTEENTH				((gctFIXED) 0x00001000)
#define FIXED_TWO_AND_HALF			((gctFIXED) 0x00028000)
#define FIXED_SIX					((gctFIXED) 0x00060000)

#define	FIXED_PI					((gctFIXED) 0x0003243F)
#define FIXED_PI_TWO				((gctFIXED) 0x0006487F)
#define FIXED_PI_THREE_QUARTER		((gctFIXED) 0x00025B2B)
#define FIXED_PI_HALF				((gctFIXED) 0x0001921F)
#define FIXED_PI_QUARTER			((gctFIXED) 0x0000C90F)
#define FIXED_PI_EIGHTH				((gctFIXED) 0x00006487)
/* cos(PI/8) */
#define FIXED_COS_PI_EIGHTH			((gctFIXED) 0x0000EC83)

/* Fixed point operations. */
#define FIXED_TO_INT(X)				((gctINT)((X) >> 16))
#define INT_TO_FIXED(X)				((gctFIXED)((X) << 16))
#define FLOAT_TO_FIXED(X)			((gctFIXED)((X) * FIXED_ONE))
#define FIXED_TO_FLOAT(X)			((gctFLOAT)(X) / FIXED_ONE)
#define FIXED_INT(X)				((gctFIXED)((gctUINT)(X) & 0xffff0000))
#define FIXED_FRAC(X)				((gctFIXED)((gctUINT)(X) & 0x0000ffff))
#define FIXED_CEIL(X)				FIXED_INT(X + FIXED_ONE - 1)
#define FIXED_FLOOR(X)				FIXED_INT(X)
#define FIXED_MUL(X1, X2)			((gctFIXED) (((gctINT64) (X1) * (X2)) >> 16))
#define FIXED_DIV(X1, X2)			((gctFIXED) ((((gctINT64) (X1)) << 16) / (X2)))
#define FIXED_MUL_BY_TWO(X)			((X) << 1)
#define FIXED_DIV_BY_TWO(X)			((X) >> 1)
#define FIXED_SQRT(X)				FLOAT_TO_FIXED(gcmSQRTF(FIXED_TO_FLOAT(X)))

#define FLOAT_TO_FIXED_WCHECK(X)	((X) > 32767.999f ? FIXED_MAX : (X) < -32767.999 ? FIXED_MIN : FLOAT_TO_FIXED(X))

/*******************************************************************************
**							Stroke Conversion Data Structures
*******************************************************************************/

#if COLLECT_STROKE_STAT
#define VGSL_STAT_COUNTER_INCREASE(C)	(C)++
#define VGSL_STAT_COUNTER_ADD(C, N)		(C) += (N)

int vgStrokePathCount = 0;
int vgStrokeCurveCount = 0;
int vgStrokeFlattenedSegmentCount = 0;
int vgStrokeLineCount = 0;
int vgStrokeFilteredLinesByAngleEpsilonCount = 0;
int vgStrokeLineAfterFilterCount = 0;
int vgStrokeFilteredJointsByAngleEpsilonCount = 0;
int vgStrokeFilteredByMinArcAngleCount = 0;
int vgStrokeArcCount = 0;
int vgStrokeCommandCount = 0;
int vgStrokeCount = 0;
#else
#define VGSL_STAT_COUNTER_INCREASE(C)
#define VGSL_STAT_COUNTER_ADD(C, N)
#endif

/* Point flatten type for flattened line segments. */
#define vgcFLATTEN_NO			0
#define vgcFLATTEN_START		1
#define vgcFLATTEN_MIDDLE		2
#define vgcFLATTEN_END			3

/* Point curve type for generated stroke path. */
#define vgcCURVE_LINE			0
#define vgcCURVE_QUAD_CONTROL	1
#define vgcCURVE_QUAD_ANCHOR	2
#define vgcCURVE_ARC_SCCW		3
#define vgcCURVE_ARC_SCCW_HALF	4

/* Swing handing control. */
#define vgcSWING_NO				0
#define vgcSWING_OUT			1
#define vgcSWING_IN				2

/* Point node in a sub path. */
typedef struct _vgsPOINT *		vgsPOINT_PTR;
typedef struct _vgsPOINT
{
	/* Pointer to next point node. */
	vgsPOINT_PTR				next;

	/* Pointer to previous point node. */
	vgsPOINT_PTR				prev;

	/* X coordinate. */
	gctFLOAT					x;

	/* Y coordinate. */
	gctFLOAT					y;

	/* Flatten flag for flattened path. */
	gctUINT8					flattenFlag;

	/* Curve type for stroke path. */
	gctUINT8					curveType;

	/* X tangent. */
	gctFLOAT					tangentX;

	/* Y tangent. */
	gctFLOAT					tangentY;

	/* Length of the line. */
	gctFLOAT					length;

#define centerX					tangentX
#define centerY					tangentY
}
vgsPOINT;

typedef struct _vgsPOINT_I *		vgsPOINT_I_PTR;
typedef struct _vgsPOINT_I
{
	/* Pointer to next point node. */
	vgsPOINT_I_PTR				next;

	/* Pointer to previous point node. */
	vgsPOINT_I_PTR				prev;

	/* X coordinate. */
	gctFIXED					x;

	/* Y coordinate. */
	gctFIXED					y;

	/* Flatten flag for flattened path. */
	gctUINT8					flattenFlag;

	/* Curve type for stroke path. */
	gctUINT8					curveType;

	/* X tangent. */
	gctFIXED					tangentX;

	/* Y tangent. */
	gctFIXED					tangentY;

	/* Length of the line. */
	gctFIXED					length;
}
vgsPOINT_I;

/* Sub path. */
typedef struct _vgsSUBPATH *	vgsSUBPATH_PTR;
typedef struct _vgsSUBPATH
{
	/* Pointer to next sub path. */
	vgsSUBPATH_PTR				next;

	/* Number of points. */
	gctINT						pointCount;

	/* Point list. */
	vgsPOINT_PTR				pointList;

	/* Last point. */
	vgsPOINT_PTR				lastPoint;

	/* Whether is path is closed. */
	gctBOOL						closed;

	/* Sub path length. */
	gctFLOAT					length;
}
vgsSUBPATH;

typedef struct _vgsSUBPATH_I *	vgsSUBPATH_I_PTR;
typedef struct _vgsSUBPATH_I
{
	/* Pointer to next sub path. */
	vgsSUBPATH_I_PTR			next;

	/* Number of points. */
	gctINT						pointCount;

	/* Point list. */
	vgsPOINT_I_PTR				pointList;

	/* Last point. */
	vgsPOINT_I_PTR				lastPoint;

	/* Whether is path is closed. */
	gctBOOL						closed;

	/* Sub path length. */
	gctFIXED					length;
}
vgsSUBPATH_I;

typedef struct _vgsSTROKECONVERSION
{
	/* HAL OS object passed in through Context. */
	gcoOS						os;

	/* Use fixed point representation, instead of floating point. */
	gctBOOL						useFixedPoint;

	/* Use fast mode for thin stroke in fixed point representation. */
	gctBOOL						useFastMode;

	/* Use fast mode for fat stroke in fixed point representation. */
	gctBOOL						useFastFatMode;

	/* The stroke line is fat line. */
	gctBOOL						isFat;

	/* Internal scale: bits shift right. */
	gctINT						internalShiftBits;

	/* Path scale and bias. */
	gctFLOAT					pathScale;
	gctFLOAT					pathBias;

	/* Transform scale. */
	gctFLOAT					largeTransformScale;
	gctFLOAT					smallTransformScale;

	/* Stroke parameters */
	gceCAP_STYLE				strokeCapStyle;
	gceJOIN_STYLE				strokeJoinStyle;
	gctFLOAT					strokeLineWidth;
	gctFLOAT					strokeMiterLimit;
	gctFLOAT *					strokeDashPattern;
	gctINT						strokeDashPatternCount;
	gctFLOAT					strokeDashPhase;
	gctBOOL						strokeDashPhaseReset;
	gctFLOAT					strokeDashInitialLength;
	gctINT						strokeDashInitialIndex;

	gctFLOAT					halfLineWidth;

	/* Total length of stroke dash patterns. */
	gctFLOAT					strokeDashPatternLength;

	/* For fast checking. */
	gctFLOAT					strokeMiterLimitSquare;

	/* Sub path list. */
	vgsSUBPATH_PTR				subPathList;

	/* Last sub path. */
	vgsSUBPATH_PTR				lastSubPath;

	/* Sub path list for stroke path. */
	vgsSUBPATH_PTR				strokeSubPathList;

	/* Last sub path for stroke path. */
	vgsSUBPATH_PTR				lastStrokeSubPath;

	/* The sub path being processed. */
	vgsSUBPATH_PTR				currentSubPath;

	/* Temp storage of stroke subPath. */
	vgsPOINT_PTR				leftStrokePoint;
	vgsPOINT_PTR				lastRightStrokePoint;

	/* Swing area handling. */
	gctBOOL						swingNeedToHandle;
	gctUINT						swingHandling;
	gctBOOL						swingCounterClockwise;
	gctFLOAT					swingStrokeDeltaX;
	gctFLOAT					swingStrokeDeltaY;
	vgsPOINT_PTR				swingStartPoint;		/* Start center point. */
	vgsPOINT_PTR				swingStartStrokePoint;	/* Start stroke point. */
	gctFLOAT					swingAccuLength;
	gctFLOAT					swingCenterLength;
	gctUINT						swingCount;

	/* Memory pools. */
	gcsMEM_FS_MEM_POOL			subPathMemPool;
	gcsMEM_FS_MEM_POOL			pointMemPool;
}
vgsSTROKECONVERSION;


typedef struct _vgsSTROKECONVERSION_I * vgsSTROKECONVERSION_I_PTR;
typedef struct _vgsSTROKECONVERSION_I
{
	/* HAL OS object passed in through Context. */
	gcoOS						os;

	/* Use fixed point representation, instead of floating point. */
	gctBOOL						useFixedPoint;

	/* Use fast mode for thin stroke in fixed point representation. */
	gctBOOL						useFastMode;

	/* Use fast mode for fat stroke in fixed point representation. */
	gctBOOL						useFastFatMode;

	/* The stroke line is fat line. */
	gctBOOL						isFat;

	/* Internal scale: bits shift right. */
	gctINT						internalShiftBits;

	/* Path scale and bias. */
	gctFIXED					pathScale;
	gctFIXED					pathBias;

	/* Transform scale. */
	gctFIXED					largeTransformScale;
	gctFIXED					smallTransformScale;

	/* Stroke parameters */
	gceCAP_STYLE				strokeCapStyle;
	gceJOIN_STYLE				strokeJoinStyle;
	gctFIXED					strokeLineWidth;
	gctFIXED					strokeMiterLimit;
	gctFIXED *					strokeDashPattern;
	gctINT						strokeDashPatternCount;
	gctFIXED					strokeDashPhase;
	gctBOOL						strokeDashPhaseReset;
	gctFIXED					strokeDashInitialLength;
	gctINT						strokeDashInitialIndex;

	gctFIXED					halfLineWidth;

	/* Total length of stroke dash patterns. */
	gctFIXED					strokeDashPatternLength;

	/* For fast checking. */
	gctFIXED					strokeMiterLimitSquare;

	/* Sub path list. */
	vgsSUBPATH_I_PTR			subPathList;

	/* Last sub path. */
	vgsSUBPATH_I_PTR			lastSubPath;

	/* Sub path list for stroke path. */
	vgsSUBPATH_I_PTR			strokeSubPathList;

	/* Last sub path for stroke path. */
	vgsSUBPATH_I_PTR			lastStrokeSubPath;

	/* The sub path being processed. */
	vgsSUBPATH_I_PTR			currentSubPath;

	/* Temp storage of stroke subPath. */
	vgsPOINT_I_PTR				leftStrokePoint;
	vgsPOINT_I_PTR				lastRightStrokePoint;

	/* Swing area handling. */
	gctBOOL						swingNeedToHandle;
	gctUINT						swingHandling;
	gctBOOL						swingCounterClockwise;
	gctFIXED					swingStrokeDeltaX;
	gctFIXED					swingStrokeDeltaY;
	vgsPOINT_I_PTR				swingStartPoint;		/* Start center point. */
	vgsPOINT_I_PTR				swingStartStrokePoint;	/* Start stroke point. */
	gctFIXED					swingAccuLength;
	gctFIXED					swingCenterLength;
	gctUINT						swingCount;

	/* Memory pools. */
	gcsMEM_FS_MEM_POOL			subPathMemPool;
	gcsMEM_FS_MEM_POOL			pointMemPool;
}
vgsSTROKECONVERSION_I;

/******************************************************************************\
|*********************** Local Optimized Math Functions ***********************|
|******************************************************************************/
/* Special sqrt(1.0f + x) for quick calculation when 0 <= x <= 1. */
static gctFLOAT _Sqrt(
	IN gctFLOAT X
	)
{
	gctFLOAT x = X;
	gctFLOAT s = 1.0f;

	s += x * 0.5f;
	x *= X;
	s -= x * 0.12445995211601257f;
	x *= X;
	s += x * 0.058032196015119553f;
	x *= X;
	s -= x * 0.025314478203654289f;
	x *= X;
	s += x * 0.0059584137052297592f;

	return s;
}

/* Special sin(x) for quick calculation when -PI <= x <= PI. */
#ifndef __QNXNTO__
static gctFLOAT _Sin(
#else
static gctFLOAT _Sin_qnx(
#endif
	IN gctFLOAT X
	)
{
	gctFLOAT x = X;
	gctFLOAT x2 = X * X;
	gctFLOAT s = X;

	x *= x2;
	s -= x * 0.16664527099620879f;
	x *= x2;
	s += x * 0.0083154803736487041f;
	x *= x2;
	s -= x * 0.00019344151251408578f;
	x *= x2;
	s += x * 0.0000021810214160988925f;

	return s;
}

/* Special cos(x) for quick calculation when -PI <= x <= PI. */
static gctFLOAT _Cos(
	IN gctFLOAT X
	)
{
	gctFLOAT x2 = X * X;
	gctFLOAT x = x2;
	gctFLOAT s = 1.0f;

	s -= x * 0.49985163079668843f;
	x *= x2;
	s += x * 0.041518066216932693f;
	x *= x2;
	s -= x * 0.0013422997970712939f;
	x *= x2;
	s += x * 0.000018930111278021357f;

	return s;
}

/* Special asin(x) for quick calculation when -sqrt(0.5) <= x <= sqrt(0.5). */
static gctFLOAT _Asin(
	IN gctFLOAT X
	)
{
	gctFLOAT x = X;
	gctFLOAT x2 = X * X;
	gctFLOAT s = X;

	x *= x2;
	s += x * 0.1660510562575219f;
	x *= x2;
	s += x * 0.084044676143618186f;
	x *= x2;
	s += x * 0.0023776176698039313f;
	x *= x2;
	s += x * 0.10211922020091345f;

	return s;
}

/* Fixed point version of local math functions. */
#if gcvUSE_FAST_STROKE
static gctFIXED _FastSqrtTable_I[256] = {
	0x1003F, 0x100BF, 0x1013F, 0x101BE, 0x1023D, 0x102BC, 0x1033A, 0x103B9,
	0x10437, 0x104B4, 0x10532, 0x105AF, 0x1062C, 0x106A9, 0x10726, 0x107A2,
	0x1081F, 0x1089A, 0x10916, 0x10992, 0x10A0D, 0x10A88, 0x10B03, 0x10B7D,
	0x10BF8, 0x10C72, 0x10CEC, 0x10D66, 0x10DDF, 0x10E59, 0x10ED2, 0x10F4B,
	0x10FC3, 0x1103C, 0x110B4, 0x1112C, 0x111A4, 0x1121C, 0x11293, 0x1130A,
	0x11381, 0x113F8, 0x1146F, 0x114E5, 0x1155B, 0x115D1, 0x11647, 0x116BD,
	0x11732, 0x117A8, 0x1181D, 0x11892, 0x11906, 0x1197B, 0x119EF, 0x11A63,
	0x11AD7, 0x11B4B, 0x11BBF, 0x11C32, 0x11CA5, 0x11D18, 0x11D8B, 0x11DFE,
	0x11E70, 0x11EE3, 0x11F55, 0x11FC7, 0x12038, 0x120AA, 0x1211B, 0x1218D,
	0x121FE, 0x1226F, 0x122DF, 0x12350, 0x123C0, 0x12431, 0x124A1, 0x12511,
	0x12580, 0x125F0, 0x1265F, 0x126CE, 0x1273E, 0x127AC, 0x1281B, 0x1288A,
	0x128F8, 0x12966, 0x129D5, 0x12A42, 0x12AB0, 0x12B1E, 0x12B8B, 0x12BF9,
	0x12C66, 0x12CD3, 0x12D40, 0x12DAC, 0x12E19, 0x12E85, 0x12EF2, 0x12F5E,
	0x12FCA, 0x13035, 0x130A1, 0x1310D, 0x13178, 0x131E3, 0x1324E, 0x132B9,
	0x13324, 0x1338E, 0x133F9, 0x13463, 0x134CD, 0x13537, 0x135A1, 0x1360B,
	0x13675, 0x136DE, 0x13747, 0x137B1, 0x1381A, 0x13883, 0x138EB, 0x13954,
	0x139BD, 0x13A25, 0x13A8D, 0x13AF5, 0x13B5D, 0x13BC5, 0x13C2D, 0x13C94,
	0x13CFC, 0x13D63, 0x13DCA, 0x13E31, 0x13E98, 0x13EFF, 0x13F66, 0x13FCC,
	0x14033, 0x14099, 0x140FF, 0x14165, 0x141CB, 0x14231, 0x14296, 0x142FC,
	0x14361, 0x143C7, 0x1442C, 0x14491, 0x144F6, 0x1455A, 0x145BF, 0x14624,
	0x14688, 0x146EC, 0x14750, 0x147B5, 0x14818, 0x1487C, 0x148E0, 0x14944,
	0x149A7, 0x14A0A, 0x14A6E, 0x14AD1, 0x14B34, 0x14B97, 0x14BF9, 0x14C5C,
	0x14CBF, 0x14D21, 0x14D83, 0x14DE5, 0x14E48, 0x14EA9, 0x14F0B, 0x14F6D,
	0x14FCF, 0x15030, 0x15092, 0x150F3, 0x15154, 0x151B5, 0x15216, 0x15277,
	0x152D8, 0x15338, 0x15399, 0x153F9, 0x1545A, 0x154BA, 0x1551A, 0x1557A,
	0x155DA, 0x1563A, 0x1569A, 0x156F9, 0x15759, 0x157B8, 0x15817, 0x15876,
	0x158D6, 0x15935, 0x15993, 0x159F2, 0x15A51, 0x15AAF, 0x15B0E, 0x15B6C,
	0x15BCB, 0x15C29, 0x15C87, 0x15CE5, 0x15D43, 0x15DA0, 0x15DFE, 0x15E5C,
	0x15EB9, 0x15F16, 0x15F74, 0x15FD1, 0x1602E, 0x1608B, 0x160E8, 0x16145,
	0x161A1, 0x161FE, 0x1625B, 0x162B7, 0x16313, 0x16370, 0x163CC, 0x16428,
	0x16484, 0x164E0, 0x1653B, 0x16597, 0x165F3, 0x1664E, 0x166AA, 0x16705,
	0x16760, 0x167BB, 0x16816, 0x16871, 0x168CC, 0x16927, 0x16982, 0x169DC
};

/* Special sqrt(1.0f + x) for quick calculation when 0 <= x <= 1. */
/* Use table lookup for performance. */
static gctFIXED _FastSqrt_I(
	IN gctFIXED X
	)
{
	gctUINT idx;
	gcmASSERT(((gctUINT) X & 0xFFFF0000) == 0);
    idx = ((gctUINT) X) >> 8;
    return _FastSqrtTable_I[idx];
}
#endif

#if gcvUSE_SQRT_TABLE
static gctFIXED _SqrtTable_I[257] = {
0x10000, 0x1007F, 0x100FF, 0x1017E, 0x101FE, 0x1027C, 0x102FB, 0x10379,
0x103F8, 0x10476, 0x104F3, 0x10571, 0x105EE, 0x1066B, 0x106E8, 0x10764,
0x107E0, 0x1085D, 0x108D8, 0x10954, 0x109CF, 0x10A4B, 0x10AC5, 0x10B40,
0x10BBB, 0x10C35, 0x10CAF, 0x10D29, 0x10DA3, 0x10E1C, 0x10E95, 0x10F0E,
0x10F87, 0x11000, 0x11078, 0x110F0, 0x11168, 0x111E0, 0x11257, 0x112CF,
0x11346, 0x113BD, 0x11433, 0x114AA, 0x11520, 0x11596, 0x1160C, 0x11682,
0x116F8, 0x1176D, 0x117E2, 0x11857, 0x118CC, 0x11941, 0x119B5, 0x11A29,
0x11A9D, 0x11B11, 0x11B85, 0x11BF8, 0x11C6C, 0x11CDF, 0x11D52, 0x11DC4,
0x11E37, 0x11EA9, 0x11F1C, 0x11F8E, 0x12000, 0x12071, 0x120E3, 0x12154,
0x121C5, 0x12236, 0x122A7, 0x12318, 0x12388, 0x123F8, 0x12469, 0x124D9,
0x12548, 0x125B8, 0x12628, 0x12697, 0x12706, 0x12775, 0x127E4, 0x12852,
0x128C1, 0x1292F, 0x1299E, 0x12A0C, 0x12A79, 0x12AE7, 0x12B55, 0x12BC2,
0x12C2F, 0x12C9C, 0x12D09, 0x12D76, 0x12DE3, 0x12E4F, 0x12EBB, 0x12F28,
0x12F94, 0x13000, 0x1306B, 0x130D7, 0x13142, 0x131AD, 0x13219, 0x13284,
0x132EE, 0x13359, 0x133C4, 0x1342E, 0x13498, 0x13502, 0x1356C, 0x135D6,
0x13640, 0x136A9, 0x13713, 0x1377C, 0x137E5, 0x1384E, 0x138B7, 0x13920,
0x13988, 0x139F1, 0x13A59, 0x13AC1, 0x13B29, 0x13B91, 0x13BF9, 0x13C61,
0x13CC8, 0x13D30, 0x13D97, 0x13DFE, 0x13E65, 0x13ECC, 0x13F32, 0x13F99,
0x14000, 0x14066, 0x140CC, 0x14132, 0x14198, 0x141FE, 0x14264, 0x142C9,
0x1432F, 0x14394, 0x143F9, 0x1445E, 0x144C3, 0x14528, 0x1458D, 0x145F1,
0x14656, 0x146BA, 0x1471E, 0x14783, 0x147E7, 0x1484A, 0x148AE, 0x14912,
0x14975, 0x149D9, 0x14A3C, 0x14A9F, 0x14B02, 0x14B65, 0x14BC8, 0x14C2B,
0x14C8D, 0x14CF0, 0x14D52, 0x14DB4, 0x14E16, 0x14E79, 0x14EDA, 0x14F3C,
0x14F9E, 0x15000, 0x15061, 0x150C2, 0x15124, 0x15185, 0x151E6, 0x15247,
0x152A7, 0x15308, 0x15369, 0x153C9, 0x1542A, 0x1548A, 0x154EA, 0x1554A,
0x155AA, 0x1560A, 0x1566A, 0x156C9, 0x15729, 0x15788, 0x157E8, 0x15847,
0x158A6, 0x15905, 0x15964, 0x159C3, 0x15A22, 0x15A80, 0x15ADF, 0x15B3D,
0x15B9B, 0x15BFA, 0x15C58, 0x15CB6, 0x15D14, 0x15D71, 0x15DCF, 0x15E2D,
0x15E8A, 0x15EE8, 0x15F45, 0x15FA2, 0x16000, 0x1605D, 0x160B9, 0x16116,
0x16173, 0x161D0, 0x1622C, 0x16289, 0x162E5, 0x16341, 0x1639E, 0x163FA,
0x16456, 0x164B2, 0x1650D, 0x16569, 0x165C5, 0x16620, 0x1667C, 0x166D7,
0x16732, 0x1678E, 0x167E9, 0x16844, 0x1689F, 0x168F9, 0x16954, 0x169AF,
0x16A09
};
#endif

/*
static gctFIXED _SqrtSlopeTable_I[256] = {
};
*/

/* Special sqrt(1.0f + x) for quick calculation when 0 <= x <= 1. */
/* Use 64-bit for accuracy. */
static gctFIXED _Sqrt_I(
	IN gctFIXED X
	)
{
#if gcvUSE_SQRT_TABLE
	{
	gctFIXED sq, *sqPtr;
	gcmASSERT(((gctUINT) X & 0xFFFF0000) == 0);
	sqPtr = _SqrtTable_I + (((gctUINT) X) >> 8);
	sq = *sqPtr;
	sqPtr++;
    return sq + ((((gctUINT) X & 0xFF) * (*sqPtr - sq)) >> 8);
	}
#else
	gctINT64 x = ((gctINT64) X) << 16;
	gctINT64 s = ((gctINT64) FIXED_ONE) << 16;

	s += x >> 1;
	x *= X;
	x >>= 16;
	s -= (x * (gctINT64) 0x1FDC9B80) >> 32;
	x *= X;
	x >>= 16;
	s += (x * (gctINT64) 0x0EDB32B0) >> 32;
	x *= X;
	x >>= 16;
	s -= (x * (gctINT64) 0x067B0278) >> 32;
	x *= X;
	x >>= 16;
	s += (x * (gctINT64) 0x01867D98) >> 32;

	return (gctFIXED) (s >> 16);
#endif
}

/* Special sin(x) for quick calculation when -PI <= x <= PI. */
static gctFIXED _Sin_I(
	IN gctFIXED X
	)
{
	gctINT64 x = ((gctINT64) X);
	gctINT64 x2 = (((gctINT64) X) * ((gctINT64) X));
	gctINT64 s = x << 16;

	x *= x2;
	x >>= 32;
	s -= (x * (gctINT64) 0x2AA943B5) >> 16;
	x *= x2;
	x >>= 32;
	s += (x * (gctINT64) 0x0220F69C) >> 16;
	x *= x2;
	x >>= 32;
	s -= (x * (gctINT64) 0x000CAD69) >> 16;
	x *= x2;
	x >>= 32;
	s += (x * (gctINT64) 0x00002497) >> 16;

	return (gctFIXED) (s >> 16);
}

/* Special cos(x) for quick calculation when -PI <= x <= PI. */
static gctFIXED _Cos_I(
	IN gctFIXED X
	)
{
	gctINT64 x2 = ((gctINT64) X) * ((gctINT64) X);
	gctINT64 x = x2 >> 16;
	gctINT64 s = ((gctINT64) FIXED_ONE) << 16;

	s -= (x * (gctINT64) 0x7FF646C7) >> 16;
	x *= x2;
	x >>= 32;
	s += (x * (gctINT64) 0x0AA0ED91) >> 16;
	x *= x2;
	x >>= 32;
	s -= (x * (gctINT64) 0x0057F80E) >> 16;
	x *= x2;
	x >>= 32;
	s += (x * (gctINT64) 0x00013D98) >> 16;

	return (gctFIXED) (s >> 16);
}

/* Special asin(x) for quick calculation when -sqrt(0.5) <= x <= sqrt(0.5). */
static gctFIXED _Asin_I(
	IN gctFIXED X
	)
{
	gctINT64 x = ((gctINT64) X) << 16;
	gctINT64 x2 = ((gctINT64) X) * ((gctINT64) X);
	gctINT64 s = x;

	x *= x2;
	x >>= 32;
	s += (x * (gctINT64) 0x2A825270) >> 32;
	x *= x2;
	x >>= 32;
	s += (x * (gctINT64) 0x1583F3AF) >> 32;
	x *= x2;
	x >>= 32;
	s += (x * (gctINT64) 0x009BD1CE) >> 32;
	x *= x2;
	x >>= 32;
	s += (x * (gctINT64) 0x1A247C37) >> 32;

	return (gctFIXED) (s >> 16);
}

/******************************************************************************\
|************************* Memory management Functions ************************|
|******************************************************************************/
gcmMEM_DeclareFSMemPool(struct _vgsSUBPATH,	SubPath,	);
gcmMEM_DeclareFSMemPool(struct _vgsPOINT,	Point,		);

#define _AllocatePoint _CAllocatePoint

static void
_InitMemPool(
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	gcoOS os = StrokeConversion->os;

	gcfMEM_InitFSMemPool(&StrokeConversion->subPathMemPool,	os,  10, sizeof(struct _vgsSUBPATH));
	gcfMEM_InitFSMemPool(&StrokeConversion->pointMemPool,	os, 500, sizeof(struct _vgsPOINT));
}

static void
_CleanupMemPool(
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	gcfMEM_FreeFSMemPool(&StrokeConversion->subPathMemPool);
	gcfMEM_FreeFSMemPool(&StrokeConversion->pointMemPool);
}

/******************************************************************************\
|***************** Constructor/Initialization and Destructor ******************|
|******************************************************************************/
static gceSTATUS
_InitializeStrokeParameters(
	IN vgsCONTEXT_PTR Context,
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gctINT count = Context->strokeDashPtrnCount;
	gctINT i;
	gctFLOAT * patternSrc;
	gctFLOAT * pattern;
	gctFLOAT length;

	if (count == 0) return gcvSTATUS_OK;

	StrokeConversion->strokeDashPhase = length
		= Context->strokeDashPhase;

	StrokeConversion->strokeDashPhaseReset
		= Context->strokeDashPhaseReset;

	/* The last pattern is ignored if the number is odd. */
	if (count & 0x1) count--;

	StrokeConversion->strokeDashPatternCount = count;
	gcmERROR_RETURN(gcoOS_Allocate(
		StrokeConversion->os,
		count * gcmSIZEOF(gctFLOAT),
		(gctPOINTER *) &pattern
		));

	StrokeConversion->strokeDashPattern = pattern;
	StrokeConversion->strokeDashPatternLength = 0.0f;
	patternSrc = Context->strokeDashPtrn;

	for (i = 0; i < count; i++, pattern++, patternSrc++)
	{
		if (*patternSrc < 0.0f)
		{
			*pattern = 0.0f;
		}
		else
		{
			*pattern = *patternSrc;
		}
		StrokeConversion->strokeDashPatternLength += *pattern;
	}

	if (StrokeConversion->strokeDashPatternLength < FLOAT_EPSILON)
	{
		StrokeConversion->strokeDashPatternCount = 0;
		gcmVERIFY_OK(gcoOS_Free(StrokeConversion->os, StrokeConversion->strokeDashPattern));
		return gcvSTATUS_OK;
	}

	while (length < 0.0f)
	{
		length += StrokeConversion->strokeDashPatternLength;
	}

	while (length >= StrokeConversion->strokeDashPatternLength)
	{
		length -= StrokeConversion->strokeDashPatternLength;
	}

	pattern = StrokeConversion->strokeDashPattern;
	for (i = 0; i < StrokeConversion->strokeDashPatternCount; i++, pattern++)
	{
		if (length <= *pattern) break;

		length -= *pattern;
	}

	StrokeConversion->strokeDashInitialIndex = i;
	StrokeConversion->strokeDashInitialLength = *pattern - length;

	return gcvSTATUS_OK;
}

static gceSTATUS
_InitStrokeConversion(
	IN vgsCONTEXT_PTR Context,
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	gceSTATUS status = gcvSTATUS_OK;


	StrokeConversion->strokeMiterLimit = Context->strokeMiterLimit;
	StrokeConversion->strokeCapStyle = Context->halStrokeCapStyle;
	StrokeConversion->strokeJoinStyle = Context->halStrokeJoinStyle;
	StrokeConversion->strokeLineWidth = Context->strokeLineWidth;

	StrokeConversion->halfLineWidth = StrokeConversion->strokeLineWidth / 2.0f;
	StrokeConversion->strokeMiterLimitSquare = Context->strokeMiterLimit * Context->strokeMiterLimit;

	gcmERROR_RETURN(_InitializeStrokeParameters(Context, StrokeConversion));

	return gcvSTATUS_OK;
}

static void
_CheckScaleAndBias(
	IN vgsCONTEXT_PTR Context,
	IN vgsPATH_PTR Path,
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	gctFLOAT pathScale, pathBias;
	gctFLOAT scaleX, scaleY, t;
	vgsMATRIX_PTR userToSurface;

	/* Get the matrix. */
	userToSurface = Context->currentUserToSurface;

	/* Set scale, bias. */
	StrokeConversion->pathScale = pathScale = Path->scale;
	StrokeConversion->pathBias  = pathBias  = Path->bias;

	/* Calculate quality scale. */
#if gcdENABLE_PERFORMANCE_PRIOR
	t = vgmFIXED_TO_FLOAT(vgmMAT(userToSurface, 0, 0));	scaleX  = (t >= 0.0f ? t : -t);
	t = vgmFIXED_TO_FLOAT(vgmMAT(userToSurface, 0, 1));	scaleX += (t >= 0.0f ? t : -t);
	t = vgmFIXED_TO_FLOAT(vgmMAT(userToSurface, 1, 0));	scaleY  = (t >= 0.0f ? t : -t);
	t = vgmFIXED_TO_FLOAT(vgmMAT(userToSurface, 1, 1));	scaleY += (t >= 0.0f ? t : -t);
#else
	t = vgmMAT(userToSurface, 0, 0);	scaleX  = (t >= 0.0f ? t : -t);
	t = vgmMAT(userToSurface, 0, 1);	scaleX += (t >= 0.0f ? t : -t);
	t = vgmMAT(userToSurface, 1, 0);	scaleY  = (t >= 0.0f ? t : -t);
	t = vgmMAT(userToSurface, 1, 1);	scaleY += (t >= 0.0f ? t : -t);
#endif

	if (scaleX > scaleY)
	{
		StrokeConversion->largeTransformScale = scaleX;
		StrokeConversion->smallTransformScale = scaleY;
	}
	else
	{
		StrokeConversion->largeTransformScale = scaleY;
		StrokeConversion->smallTransformScale = scaleX;
	}

#if vgvSTROKE_COPY_OPTIMIZE && gcvUSE_INTERNAL_SCALE
	/* If transform scale is too large or too small, use floating point. */
	if (StrokeConversion->largeTransformScale > 256.0f
	||	StrokeConversion->smallTransformScale < 0.00390625f)
	{
		/* Currently, scale limit is 256 (8bits). */
		StrokeConversion->useFixedPoint = gcvFALSE;
	}
	else if (StrokeConversion->largeTransformScale / StrokeConversion->smallTransformScale > 256.0f)
	{
		/* Currently, aspect ratio limit is 256. */
		StrokeConversion->useFixedPoint = gcvFALSE;
	}
#else
	/* If transform scal is too large or too small, use floating point. */
	if (StrokeConversion->largeTransformScale > 4.0f
	||	StrokeConversion->smallTransformScale < 0.25f)
	{
		StrokeConversion->useFixedPoint = gcvFALSE;
	}
#endif

	if (StrokeConversion->strokeLineWidth * StrokeConversion->largeTransformScale >= FLOAT_FAT_LINE_WIDTH
    &&  StrokeConversion->strokeLineWidth >= 1.0f)
	{
		StrokeConversion->isFat = gcvTRUE;
	}
}

static gceSTATUS
_ResetStrokeConversion(
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	if (StrokeConversion->strokeDashPatternCount > 0)
	{
		StrokeConversion->strokeDashPatternCount = 0;
		gcmVERIFY_OK(gcoOS_Free(StrokeConversion->os, StrokeConversion->strokeDashPattern));
	}

	/* Free subPath list. */
	if (StrokeConversion->subPathList)
	{
		vgsSUBPATH_PTR subPath, nextSubPath;

		for (subPath = StrokeConversion->subPathList; subPath; subPath = nextSubPath)
		{
#if !gcvUSE_FAST_STROKE /*|| gcvDEBUG*/
			vgsPOINT_PTR point, nextPoint;
			gctINT i;

			nextSubPath = subPath->next;
			for (i = 0, point = subPath->pointList; i < subPath->pointCount; i++, point = nextPoint)
			{
				nextPoint = point->next;
				_FreePoint(StrokeConversion->pointMemPool, point);
			}
			gcmASSERT(point == gcvNULL);
			_FreeSubPath(StrokeConversion->subPathMemPool, subPath);
#else
			nextSubPath = subPath->next;
			_FreePointList(StrokeConversion->pointMemPool,
				subPath->pointList, subPath->lastPoint);
#endif
		}

#if gcvUSE_FAST_STROKE /*&& ! gcvDEBUG*/
		_FreeSubPathList(StrokeConversion->subPathMemPool,
			StrokeConversion->subPathList, StrokeConversion->lastSubPath);
#endif

		StrokeConversion->subPathList = gcvNULL;
		StrokeConversion->lastSubPath = gcvNULL;
	}

	if (StrokeConversion->strokeSubPathList)
	{
		vgsSUBPATH_PTR subPath, nextSubPath;

		for (subPath = StrokeConversion->strokeSubPathList; subPath; subPath = nextSubPath)
		{
#if !gcvUSE_FAST_STROKE /*|| gcvDEBUG*/
			vgsPOINT_PTR point, nextPoint;
			gctINT i;

			nextSubPath = subPath->next;
			for (i = 0, point = subPath->pointList; i < subPath->pointCount; i++, point = nextPoint)
			{
				if (point == gcvNULL)
				{
					gcmASSERT(point);
					break;
				}
				nextPoint = point->next;
				_FreePoint(StrokeConversion->pointMemPool, point);
			}
			gcmASSERT(point == gcvNULL);
			_FreeSubPath(StrokeConversion->subPathMemPool, subPath);
#else
			nextSubPath = subPath->next;
			_FreePointList(StrokeConversion->pointMemPool,
				subPath->pointList, subPath->lastPoint);
#endif
		}


#if gcvUSE_FAST_STROKE /*&& ! gcvDEBUG*/
		_FreeSubPathList(StrokeConversion->subPathMemPool,
			StrokeConversion->strokeSubPathList, StrokeConversion->lastStrokeSubPath);
#endif

		StrokeConversion->strokeSubPathList = gcvNULL;
		StrokeConversion->lastStrokeSubPath = gcvNULL;

		StrokeConversion->leftStrokePoint = gcvNULL;
		StrokeConversion->lastRightStrokePoint = gcvNULL;

		if (! StrokeConversion->useFastMode)
		{
			/* TODO - Clear swing related data. */
		}
	}

	return gcvSTATUS_OK;
}

#define vgmGETINCREMENT(Pointer, DatatypeSize) \
	(DatatypeSize - (gcmPTR2INT(Pointer) & (DatatypeSize - 1)))

#define vgmSKIPTODATA(Pointer, DatatypeSize, SIZE) \
	/* Determine the increment value. */ \
	increment = vgmGETINCREMENT(Pointer, DatatypeSize); \
	/* Skip to the data. */ \
	Pointer += increment; \
	SIZE -= increment

#if vgvSTROKE_COPY_OPTIMIZE && gcvUSE_INTERNAL_SCALE
typedef gctINT (* vgSL_ValueGetter_I_RAW) (
	gctUINT8 * Data
	);

static gctINT _GetS8_I_RAW(
	gctUINT8 * Data
	)
{
	gctINT x = *((gctINT8 *) Data);

	return x;
}

static gctINT _GetS16_I_RAW(
	gctUINT8 * Data
	)
{
	gctINT x = *((gctINT16 *) Data);

	return x;
}

static gctINT _GetS32_I_RAW(
	gctUINT8 * Data
	)
{
	gctINT x = *((gctINT *) Data);

	return x;
}

#define VGSL_GETANDCHECKVALUE(X) \
	X = *((gctFLOAT *) dataPointer); \
	dataPointer += dataTypeSize; \
	size -= dataTypeSize; \
	if (X < 0.0f) X = -X; \
	if (X > maxF) maxF = X; \
	else if (X < minF && X > 0.0001f) minF = X

#define VGSL_GETANDCHECKVALUE_I(X) \
	X = getValue_I(dataPointer); \
	dataPointer += dataTypeSize; \
	size -= dataTypeSize; \
	if (X < 0) X = -X; \
	if (X > maxI) maxI = X; \
	else if (X < minI && X > 0) minI = X

static gceSTATUS
_CheckInputPath(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsPATH_PTR Path
	)
{
	gctUINT pathHeadReserve;
	gcsCMDBUFFER_PTR currentBuffer;
	vgsPATH_DATA_PTR currentPath;

	gctUINT increment;
	gctUINT32 size;						/* The data size of the path. */
	gceVGCMD pathCommand;				/* Path command of each segment in the VGPath. */
	gctINT dataTypeSize;				/* Size of the data type. */
	gctUINT8_PTR dataPointer = gcvNULL;	/* Pointer to the data of the VGPath. */
	gctFLOAT minF, maxF;				/* Min/max of float input data. */
	gctINT minI, maxI;					/* Min/max of integer input data. */
	gctFLOAT scale;
	gctINT shiftBits;

	StrokeConversion->internalShiftBits = 0;
	if (! StrokeConversion->useFixedPoint)
	{
		return gcvSTATUS_OK;
	}

	/* Get the min/max of input data without scale/bias. */
	minF = maxF = 1.0f;
	minI = 32767;
	maxI = 0;

	/* Get the path head reserve size. */
	pathHeadReserve = Path->pathInfo.reservedForHead;

	/* Set the initial buffer. */
	currentPath = Path->head;

	while (currentPath)
	{
		/* Get a shortcut to the path buffer. */
		currentBuffer = vgmGETCMDBUFFER(currentPath);

		/* Determine the data size. */
		size = currentBuffer->offset - pathHeadReserve;

		/* Determine the beginning of the path data. */
		dataPointer
			= (gctUINT8_PTR) currentBuffer
			+ currentBuffer->bufferOffset
			+ pathHeadReserve;

		if (currentPath->data.dataType == gcePATHTYPE_FLOAT)
		{
			gctFLOAT x0, y0;

			dataTypeSize = 4;
			while (size > 0)
			{
				/* Get the command. */
				pathCommand = *dataPointer & gcvVGCMD_MASK;
				switch (pathCommand)
				{
				case gcvVGCMD_CLOSE:
					/* Skip the command. */
					dataPointer += 1;
					size -= 1;
					break;

				case gcvVGCMD_MOVE_REL:
				case gcvVGCMD_MOVE:
					/* Skip to the data. */
					vgmSKIPTODATA(dataPointer, dataTypeSize, size);

					VGSL_GETANDCHECKVALUE(x0);
					VGSL_GETANDCHECKVALUE(y0);
					break;

				case gcvVGCMD_LINE_REL:
				case gcvVGCMD_LINE:
					/* Skip to the data. */
					vgmSKIPTODATA(dataPointer, dataTypeSize, size);

					VGSL_GETANDCHECKVALUE(x0);
					VGSL_GETANDCHECKVALUE(y0);
					break;

				case gcvVGCMD_QUAD_REL:
				case gcvVGCMD_QUAD:
					/* Skip to the data. */
					vgmSKIPTODATA(dataPointer, dataTypeSize, size);

					VGSL_GETANDCHECKVALUE(x0);
					VGSL_GETANDCHECKVALUE(y0);
					VGSL_GETANDCHECKVALUE(x0);
					VGSL_GETANDCHECKVALUE(y0);
					break;

				case gcvVGCMD_CUBIC_REL:
				case gcvVGCMD_CUBIC:
					/* Skip to the data. */
					vgmSKIPTODATA(dataPointer, dataTypeSize, size);

					VGSL_GETANDCHECKVALUE(x0);
					VGSL_GETANDCHECKVALUE(y0);
					VGSL_GETANDCHECKVALUE(x0);
					VGSL_GETANDCHECKVALUE(y0);
					VGSL_GETANDCHECKVALUE(x0);
					VGSL_GETANDCHECKVALUE(y0);
					break;

				default:
					gcmASSERT(gcvFALSE);
					return gcvSTATUS_INVALID_DATA;
				}
			}
		}
		else
		{
			vgSL_ValueGetter_I_RAW getValue_I;
			gctFIXED x0, y0;

			/* Select the data picker. */
			switch (currentPath->data.dataType)
			{
			case gcePATHTYPE_INT8:
				getValue_I = _GetS8_I_RAW;
				dataTypeSize = 1;
				break;

			case gcePATHTYPE_INT16:
				getValue_I = _GetS16_I_RAW;
				dataTypeSize = 2;
				break;

			case gcePATHTYPE_INT32:
				getValue_I = _GetS32_I_RAW;
				dataTypeSize = 4;
				break;

			default:
				gcmASSERT(0);
				return gcvSTATUS_INVALID_ARGUMENT;
			}

			while (size > 0)
			{
				/* Get the command. */
				pathCommand = *dataPointer & gcvVGCMD_MASK;
				switch (pathCommand)
				{
				case gcvVGCMD_CLOSE:
					/* Skip the command. */
					dataPointer += 1;
					size -= 1;
					break;

				case gcvVGCMD_MOVE_REL:
				case gcvVGCMD_MOVE:
					/* Skip to the data. */
					vgmSKIPTODATA(dataPointer, dataTypeSize, size);

					VGSL_GETANDCHECKVALUE_I(x0);
					VGSL_GETANDCHECKVALUE_I(y0);
					break;

				case gcvVGCMD_LINE_REL:
				case gcvVGCMD_LINE:
					/* Skip to the data. */
					vgmSKIPTODATA(dataPointer, dataTypeSize, size);

					VGSL_GETANDCHECKVALUE_I(x0);
					VGSL_GETANDCHECKVALUE_I(y0);
					break;

				case gcvVGCMD_QUAD_REL:
				case gcvVGCMD_QUAD:
					/* Skip to the data. */
					vgmSKIPTODATA(dataPointer, dataTypeSize, size);

					VGSL_GETANDCHECKVALUE_I(x0);
					VGSL_GETANDCHECKVALUE_I(y0);
					VGSL_GETANDCHECKVALUE_I(x0);
					VGSL_GETANDCHECKVALUE_I(y0);
					break;

				case gcvVGCMD_CUBIC_REL:
				case gcvVGCMD_CUBIC:
					/* Skip to the data. */
					vgmSKIPTODATA(dataPointer, dataTypeSize, size);

					VGSL_GETANDCHECKVALUE_I(x0);
					VGSL_GETANDCHECKVALUE_I(y0);
					VGSL_GETANDCHECKVALUE_I(x0);
					VGSL_GETANDCHECKVALUE_I(y0);
					VGSL_GETANDCHECKVALUE_I(x0);
					VGSL_GETANDCHECKVALUE_I(y0);
					break;

				default:
					gcmASSERT(gcvFALSE);
					return gcvSTATUS_INVALID_DATA;
				}
			}
		}

		/* Advance to the next subbuffer. */
		currentPath = (vgsPATH_DATA_PTR) currentBuffer->nextSubBuffer;
	}

	if (maxF == minF)
	{
		/* All input data are integers. */
		gcmASSERT(maxF == 1.0f);
		/* Skip the cases that all data are either 0.0 or 1.0. */
		if (maxI > 0)
		{
			maxF = (gctFLOAT) maxI;
			minF = (gctFLOAT) minI;
		}
	}
	else if (maxI > 0)
	{
		if (maxF < (gctFLOAT) maxI) maxF = (gctFLOAT) maxI;
		if (minF > (gctFLOAT) minI) minF = (gctFLOAT) minI;
	}

	/* Check the range of the input data. */
	if ((double)maxF / (double)minF > 4194304.0) /* 2^12 * 2^10 */
	{
		/* Range is too large--have to use float. */
		StrokeConversion->useFixedPoint = gcvFALSE;
		return gcvSTATUS_OK;
	}

	/* Check if internal scaling is needed for transform matrix. */
	if (StrokeConversion->largeTransformScale >= 4.0f)
	{
		shiftBits = (gctINT) (log(StrokeConversion->largeTransformScale) / log(2.0));
		StrokeConversion->internalShiftBits = -shiftBits;	/* Shift left. */
		scale = (gctFLOAT) (1 << shiftBits);
	}
	else if (StrokeConversion->largeTransformScale <= 0.25f)
	{
		scale = 1.0f / StrokeConversion->largeTransformScale;
		shiftBits = (gctINT) (log(scale) / log(2.0));
		StrokeConversion->internalShiftBits = shiftBits;	/* Shift right. */
		scale = 1.0f / (gctFLOAT) (1 << shiftBits);
	}
	else
	{
		scale = 1.0f;
	}

	/* Check if more internal scaling is needed for input data. */
	maxF *= (Path->scale > 0.0f ? Path->scale : -Path->scale);
	maxF += (Path->bias >= 0.0f ? Path->bias : -Path->bias);
	maxF *= scale;
	if (maxF > 8192.0f)
	{
		scale = maxF / 8192.0f; /* 2^13 */
		shiftBits = (gctINT) ceil(log(scale) / log(2.0));
		StrokeConversion->internalShiftBits += shiftBits;	/* Shift right. */
	}


	return gcvSTATUS_OK;
}
#endif

/******************************************************************************\
|************************ Floating Point Implementation ***********************|
|******************************************************************************/
typedef gctFLOAT (* vgSL_ValueGetter) (
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	);

static gctFLOAT _GetS8(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT8 *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x *= PathScale;
	x += PathBias;

	return (gctFLOAT) x;
}

static gctFLOAT _GetS16(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT16 *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x *= PathScale;
	x += PathBias;

	return x;
}

static gctFLOAT _GetS32(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x *= PathScale;
	x += PathBias;

	return x;
}

static gctFLOAT _GetF(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctFLOAT x = *((gctFLOAT *) Data);
	x *= PathScale;
	x += PathBias;

	return x;
}

static gctFLOAT _GetS8_NB(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT8 *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x *= PathScale;

	return x;
}

static gctFLOAT _GetS16_NB(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT16 *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x *= PathScale;

	return x;
}

static gctFLOAT _GetS32_NB(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x *= PathScale;

	return x;
}

static gctFLOAT _GetF_NB(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctFLOAT x = *((gctFLOAT *) Data);
	x *= PathScale;

	return x;
}

static gctFLOAT _GetS8_NS(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT8 *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x += PathBias;

	return x;
}

static gctFLOAT _GetS16_NS(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT16 *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x += PathBias;

	return x;
}

static gctFLOAT _GetS32_NS(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	x += PathBias;

	return x;
}

static gctFLOAT _GetF_NS(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctFLOAT x = *((gctFLOAT *) Data);

	x += PathBias;

	return x;
}

static gctFLOAT _GetS8_NS_NB(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT8 *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	return x;
}

static gctFLOAT _GetS16_NS_NB(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT16 *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	return x;
}

static gctFLOAT _GetS32_NS_NB(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctINT x0 = *((gctINT *) Data);
	gctFLOAT x = (gctFLOAT) x0;

	return x;
}

static gctFLOAT _GetF_NS_NB(
	gctUINT8 * Data,
	gctFLOAT PathScale,
	gctFLOAT PathBias
	)
{
	gctFLOAT x = *((gctFLOAT *) Data);

	return x;
}

#define VGSL_GETVALUE(X) \
	X = getValue(dataPointer, pathScale, pathBias); \
	dataPointer += dataTypeSize; \
	size -= dataTypeSize

#define VGSL_GETCOORDXY(X, Y) \
	VGSL_GETVALUE(X); \
	VGSL_GETVALUE(Y); \
	if (isRelative) { X += ox; Y += oy; }

static gceSTATUS
_AddSubPath(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	vgsSUBPATH_PTR * SubPath
	)
{
	gceSTATUS status;

	gcmERROR_RETURN(_CAllocateSubPath(StrokeConversion->subPathMemPool, SubPath));

	if (StrokeConversion->lastSubPath != gcvNULL)
	{
		StrokeConversion->lastSubPath->next = *SubPath;
		StrokeConversion->lastSubPath = *SubPath;
	}
	else
	{
		StrokeConversion->lastSubPath = StrokeConversion->subPathList = *SubPath;
	}

	return status;
}


static void
_SetPointTangent(
	vgsPOINT_PTR Point,
	gctFLOAT Dx,
	gctFLOAT Dy
	)
{
	if (Dx == 0.0f)
	{
		if (Dy == 0.0f)
		{
			if (Point->prev)
			{
				Point->length = 0.0f;
				Point->tangentX = Point->prev->tangentX;
				Point->tangentY = Point->prev->tangentY;
			}
			else
			{
				gcmASSERT(Point->prev);
				Point->length = 0.0f;
				Point->tangentX = 0.0f;
				Point->tangentY = 0.0f;
			}
		}
		else
		{
			Point->tangentX = 0.0f;
			if (Dy > 0.0f)
			{
				Point->length = Dy;
				Point->tangentY = 1.0f;
			}
			else
			{
				Point->length = -Dy;
				Point->tangentY = -1.0f;
			}
		}
	}
	else if (Dy == 0.0f)
	{
		Point->tangentY = 0.0f;
		if (Dx > 0.0f)
		{
			Point->length = Dx;
			Point->tangentX = 1.0f;
		}
		else
		{
			Point->length = -Dx;
			Point->tangentX = -1.0f;
		}
	}
	else
	{
		gctFLOAT l, tx, ty;

#if USE_LOCAL_SQRT
		gctFLOAT dx, dy;
		gctFLOAT t, t2;

		dx = (Dx >= 0.0f ? Dx : -Dx);
		dy = (Dy >= 0.0f ? Dy : -Dy);
		if (dx >= dy)
		{
			t = dy / dx;
			t2 = t * t;
			l = _Sqrt(t2);
			Point->length = l * dx;

			tx = 1.0f / l;
			ty = tx * t;
		}
		else
		{
			t = dx / dy;
			t2 = t * t;
			l = _Sqrt(t2);
			Point->length = l * dy;

			ty = 1.0f / l;
			tx = ty * t;
		}
		if (Dx < 0.0f) tx = -tx;
		if (Dy < 0.0f) ty = -ty;
#else
		l = Dx * Dx + Dy * Dy;
		gcmASSERT(l != 0.0f);
		l = gcmSQRTF(l);
		Point->length = l;

		l = 1.0f / l;
		tx = Dx * l;
		ty = Dy * l;
#endif
		tx = gcmCLAMP(tx, -1.0f, 1.0f);
		ty = gcmCLAMP(ty, -1.0f, 1.0f);
		Point->tangentX = tx;
		Point->tangentY = ty;
	}
}

static gceSTATUS
_AddPointToSubPathWDelta(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFLOAT X,
	gctFLOAT Y,
	gctFLOAT DX,
	gctFLOAT DY,
	vgsSUBPATH_PTR SubPath,
	gctUINT FlattenFlag
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsPOINT_PTR lastPoint = SubPath->lastPoint;
	vgsPOINT_PTR point;

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &point));

	point->x = X;
	point->y = Y;
	point->flattenFlag = FlattenFlag;

	/* Calculate tangent for lastPoint. */
	gcmASSERT(lastPoint);
	_SetPointTangent(lastPoint, DX, DY);

	lastPoint->next = point;
	SubPath->lastPoint = point;
	point->prev = lastPoint;
	SubPath->pointCount++;

	return status;
}

static gceSTATUS
_AddPointToSubPath(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFLOAT X,
	gctFLOAT Y,
	vgsSUBPATH_PTR SubPath,
	gctUINT FlattenFlag
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsPOINT_PTR lastPoint = SubPath->lastPoint;
	vgsPOINT_PTR point;

	if (lastPoint == gcvNULL)
	{
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &point));

		gcmASSERT(FlattenFlag == vgcFLATTEN_NO);
		point->x = X;
		point->y = Y;
		point->flattenFlag = FlattenFlag;
		point->prev = gcvNULL;
		SubPath->lastPoint = SubPath->pointList = point;
		SubPath->pointCount++;

		return gcvSTATUS_OK;
	}
	else
	{
		gctFLOAT dX = X - lastPoint->x;
		gctFLOAT dY = Y - lastPoint->y;
		gctFLOAT deltaX = (dX >= 0.0f ? dX : -dX);
		gctFLOAT deltaY = (dY >= 0.0f ? dY : -dY);

		/* Check for degenerated line. */
		if (deltaX == 0.0f && deltaY == 0.0f)
		{
			/* Skip degenerated line. */
			return gcvSTATUS_OK;
		}
		if (deltaX < FLOAT_EPSILON && deltaY < FLOAT_EPSILON)
		{
			gctFLOAT ratioX, ratioY;

			if (deltaX == 0.0f)
			{
				ratioX = 0.0f;
			}
			else if (X == 0.0f)
			{
				ratioX = deltaX;
			}
			else
			{
				ratioX = deltaX / X;
				if (ratioX < 0.0f) ratioX = -ratioX;
			}
			if (deltaY == 0.0f)
			{
				ratioY = 0.0f;
			}
			else if (Y == 0.0f)
			{
				ratioY = deltaY;
			}
			else
			{
				ratioY = deltaY / Y;
				if (ratioY < 0.0f) ratioY = -ratioY;
			}
			if (ratioX < 1.0e-6f && ratioY < 1.0e-6f)
			{
				/* Skip degenerated line. */
				return gcvSTATUS_OK;
			}
		}

		return _AddPointToSubPathWDelta(StrokeConversion, X, Y, dX, dY, SubPath, FlattenFlag);
	}
}

static gceSTATUS
_FlattenQuadBezier(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFLOAT X0,
	gctFLOAT Y0,
	gctFLOAT X1,
	gctFLOAT Y1,
	gctFLOAT X2,
	gctFLOAT Y2,
	vgsSUBPATH_PTR SubPath
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gctINT n;
	vgsPOINT_PTR point0, point1;
	gctFLOAT x, y;
	gctFLOAT a1x, a1y, a2x, a2y;

	VGSL_STAT_COUNTER_INCREASE(vgStrokeCurveCount);

	/* Formula.
	 * f(t) = (1 - t)^2 * p0 + 2 * t * (1 - t) * p1 + t^2 * p2
	 *      = a0 + a1 * t + a2 * t^2
	 *   a0 = p0
	 *   a1 = 2 * (-p0 + p1)
	 *   a2 = p0 - 2 * p1 + p2
	 */
	x = X1 - X0;
	a1x = x + x;
	y = Y1 - Y0;
	a1y = y + y;
	a2x = X0 - X1 - X1 + X2;
	a2y = Y0 - Y1 - Y1 + Y2;

	/* Step 1: Calculate N. */
#if gcvSEGMENT_COUNT
	n = gcvSEGMENT_COUNT;
#else
	{
		/* Lefan's method. */
		/* dist(t) = ...
		 * t2 = ...
		 * if 0 <= t2 <= 1
		 *    upperBound = dist(t2)
		 * else
		 *    upperBound = max(dist(0), dist(1))
		 * N = ceil(sqrt(upperbound / epsilon / 8))
		 */
		/* Prepare dist(t). */
		gctFLOAT f1, f2, t1, t2, upperBound;

		f1 = a1x * a2y - a2x * a1y;
		if (f1 != 0.0f)
		{
			if (f1 < 0.0f) f1 = -f1;

			/* Calculate t2. */
			t1 = a2x * a2x + a2y * a2y;
			t2 = -(x * a2x + y * a2y) / t1;
			/* Calculate upperBound. */
			if (t2 >= 0.0f && t2 <= 1.0f)
			{
				f2 = x + a2x * t2;
				f2 *= f2;
				t1 = y + a2y * t2;
				t1 *= t1;
				upperBound = t1 + f2;
			}
			else
			{
				f2 = x + a2x;
				f2 *= f2;
				t1 = y + a2y;
				t1 *= t1;
				t2 = t1 + f2;
				t1 = x * x + y * y;
				upperBound = t1 < t2 ? t1 : t2;
			}
			/* Calculate n. */
			upperBound = f1 / gcmSQRTF(upperBound);
			upperBound *= StrokeConversion->largeTransformScale;
			upperBound = gcmSQRTF(upperBound);
			if (StrokeConversion->isFat)
			{
				upperBound *= StrokeConversion->strokeLineWidth;
			}
			n = (gctINT) ceil(upperBound);
		}
		else
		{
			/* n = 0 => n = 256. */
			n = 256;
		}
	}
#endif

	if (n == 0 || n > 256)
	{
		n = 256;
	}

	VGSL_STAT_COUNTER_ADD(vgStrokeFlattenedSegmentCount, n);

	/* Add extra P0 for incoming tangent. */
	point0 = SubPath->lastPoint;
	gcmASSERT(point0);
	/* First add P1 to calculate incoming tangent, which is saved in P0. */
	gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, X1, Y1, SubPath, vgcFLATTEN_START));
	point1 = SubPath->lastPoint;
	/* Change the point1's coordinates back to P0. */
	point1->x = X0;
	point1->y = Y0;
	point0->length = 0.0f;

	if (n > 1)
	{
		gctFLOAT d, dsquare, dx, dy, ddx, ddy;
		gctFLOAT ratioX, ratioY;
		gctINT i;

		/* Step 2: Calculate deltas. */
		/*   Df(t) = f(t + d) - f(t)
		 *         = a1 * d + a2 * d^2 + 2 * a2 * d * t
		 *  DDf(t) = Df(t + d) - Df(t)
		 *         = 2 * a2 * d^2
		 *    f(0) = a0
		 *   Df(0) = a1 * d + a2 * d^2
		 *  DDf(0) = 2 * a2 * d^2
		 */
		d = 1.0f / (gctFLOAT) n;
		dsquare = d * d;
		ddx	= a2x * dsquare;
		ddy = a2y * dsquare;
		dx	= a1x * d + ddx;
		dy	= a1y * d + ddy;
		ddx	+= ddx;
		ddy += ddy;

		/* Step 3: Add points. */
		ratioX = dx / X0;
		if (ratioX < 0.0f) ratioX = -ratioX;
		ratioY = dy / Y0;
		if (ratioY < 0.0f) ratioY = -ratioY;
		if (ratioX > 1.0e-6f && ratioY > 1.0e-6f)
		{
			x = X0;
			y = Y0;
			for (i = 1; i < n; i++)
			{
				x += dx;
				y += dy;

				/* Add a point to subpath. */
				gcmERROR_RETURN(_AddPointToSubPathWDelta(StrokeConversion, x, y, dx, dy, SubPath, vgcFLATTEN_MIDDLE));

				dx += ddx;
				dy += ddy;
			}
		}
		else
		{
			for (i = 1; i < n; i++)
			{
				gctFLOAT t = (gctFLOAT) i / (gctFLOAT) n;
				gctFLOAT u = 1.0f - t;
				gctFLOAT a0 = u * u;
				gctFLOAT a1 = 2.0f * t * u;
				gctFLOAT a2 = t * t;

				x  = a0 * X0 + a1 * X1 + a2 * X2;
				y  = a0 * Y0 + a1 * Y1 + a2 * Y2;

				/* Add a point to subpath. */
				gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, x, y, SubPath, vgcFLATTEN_MIDDLE));
			}
		}
	}

	/* Add point 2 separately to avoid cumulative errors. */
	gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, X2, Y2, SubPath, vgcFLATTEN_END));

	/* Add extra P2 for outgoing tangent. */
	/* First change P2(point0)'s coordinates to P1. */
	point0 = SubPath->lastPoint;
	point0->x = X1;
	point0->y = Y1;

	/* Add P2 to calculate outgoing tangent. */
	gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, X2, Y2, SubPath, vgcFLATTEN_NO));
	point1 = SubPath->lastPoint;

	/* Change point0's coordinates back to P2. */
	point0->x = X2;
	point0->y = Y2;
	point0->length = 0.0f;

	return gcvSTATUS_OK;
}

gceSTATUS
_FlattenCubicBezier(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFLOAT X0,
	gctFLOAT Y0,
	gctFLOAT X1,
	gctFLOAT Y1,
	gctFLOAT X2,
	gctFLOAT Y2,
	gctFLOAT X3,
	gctFLOAT Y3,
	vgsSUBPATH_PTR SubPath
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gctINT n;
	vgsPOINT_PTR point0, point1;
	gctFLOAT x, y;
	gctFLOAT a1x, a1y, a2x, a2y, a3x, a3y;

	VGSL_STAT_COUNTER_INCREASE(vgStrokeCurveCount);

	/* Formula.
	 * f(t) = (1 - t)^3 * p0 + 3 * t * (1 - t)^2 * p1 + 3 * t^2 * (1 - t) * p2 + t^3 * p3
	 *      = a0 + a1 * t + a2 * t^2 + a3 * t^3
	 */
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

	/* Step 1: Calculate N. */
#if gcvSEGMENT_COUNT
	n = gcvSEGMENT_COUNT;
#else
	{
		/* Lefan's method. */
		/*  df(t)/dt  = a1 + 2 * a2 * t + 3 * a3 * t^2
		 * d2f(t)/dt2 = 2 * a2 + 6 * a3 * t
		 * N = ceil(sqrt(max(ddfx(0)^2 + ddfy(0)^2, ddfx(1)^2 + ddyf(1)^2) / epsilon / 8))
		 */
		gctFLOAT ddf0, ddf1, t1, t2, upperBound;

		ddf0 = a2x * a2x + a2y * a2y;
		t1 = a2x + a3x + a3x + a3x;
		t2 = a2y + a3y + a3y + a3y;
		ddf1 = t1 * t1 + t2 * t2;
		upperBound = ddf0 > ddf1 ? ddf0: ddf1;
		upperBound = gcmSQRTF(upperBound);
		upperBound += upperBound;
		upperBound *= StrokeConversion->largeTransformScale;
		upperBound = gcmSQRTF(upperBound);
		if (StrokeConversion->isFat)
		{
			upperBound *= StrokeConversion->strokeLineWidth;
		}
		n = (gctINT) ceil(upperBound);
	}
#endif

	if (n == 0 || n > 256)
	{
		n = 256;
	}

	VGSL_STAT_COUNTER_ADD(vgStrokeFlattenedSegmentCount, n);

	/* Add extra P0 for incoming tangent. */
	point0 = SubPath->lastPoint;
	gcmASSERT(point0);
	/* First add P1/P2/P3 to calculate incoming tangent, which is saved in P0. */
	if (X0 != X1 || Y0 != Y1)
	{
		gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, X1, Y1, SubPath, vgcFLATTEN_START));
	}
	else if (X0 != X2 || Y0 != Y2)
	{
		gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, X2, Y2, SubPath, vgcFLATTEN_START));
	}
	else
	{
		gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, X3, Y3, SubPath, vgcFLATTEN_START));
	}
	point1 = SubPath->lastPoint;
	/* Change the point1's coordinates back to P0. */
	point1->x = X0;
	point1->y = Y0;
	point0->length = 0.0f;

	if (n > 1)
	{
		gctFLOAT d, dsquare, dcube, dx, dy, ddx, ddy, dddx, dddy;
		gctFLOAT ratioX, ratioY;
		gctINT i;

		/* Step 2: Calculate deltas */
		/*   Df(t) = f(t + d) - f(t)
		 *  DDf(t) = Df(t + d) - Df(t)
		 * DDDf(t) = DDf(t + d) - DDf(t)
		 *    f(0) = a0
		 *   Df(0) = a1 * d + a2 * d^2 + a3 * d^3
		 *  DDf(0) = 2 * a2 * d^2 + 6 * a3 * d^3
		 * DDDf(0) = 6 * a3 * d^3
		 */
		d = 1.0f / (gctFLOAT) n;
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
		dddx *= 6.0f;
		dddy *= 6.0f;
		ddx  += dddx;
		ddy  += dddy;

		/* Step 3: Add points. */
		ratioX = dx / X0;
		if (ratioX < 0.0f) ratioX = -ratioX;
		ratioY = dy / Y0;
		if (ratioY < 0.0f) ratioY = -ratioY;
		if (ratioX > 1.0e-6f && ratioY > 1.0e-6f)
		{
			x = X0;
			y = Y0;
			for (i = 1; i < n; i++)
			{
				x += dx;
				y += dy;

				/* Add a point to subpath. */
				gcmERROR_RETURN(_AddPointToSubPathWDelta(StrokeConversion, x, y, dx, dy, SubPath, vgcFLATTEN_MIDDLE));

				dx += ddx; ddx += dddx;
				dy += ddy; ddy += dddy;
			}
		}
		else
		{
			for (i = 1; i < n; i++)
			{
				gctFLOAT t = (gctFLOAT) i / (gctFLOAT) n;
				gctFLOAT tSquare = t * t;
				gctFLOAT tCube = tSquare * t;
				gctFLOAT a0 =  1.0f -  3.0f * t + 3.0f * tSquare -        tCube;
				gctFLOAT a1 =          3.0f * t - 6.0f * tSquare + 3.0f * tCube;
				gctFLOAT a2 =                     3.0f * tSquare - 3.0f * tCube;
				gctFLOAT a3 =                                             tCube;

				x  = a0 * X0 + a1 * X1 + a2 * X2 + a3 * X3;
				y  = a0 * Y0 + a1 * Y1 + a2 * Y2 + a3 * Y3;

				/* Add a point to subpath. */
				gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, x, y, SubPath, vgcFLATTEN_MIDDLE));
			}
		}
	}

	/* Add point 3 separately to avoid cumulative errors. */
	gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, X3, Y3, SubPath, vgcFLATTEN_END));

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

	/* Add P3 to calculate outgoing tangent. */
	gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, X3, Y3, SubPath, vgcFLATTEN_NO));
	point1 = SubPath->lastPoint;

	/* Change point0's coordinates back to P3. */
	point0->x = X3;
	point0->y = Y3;
	point0->length = 0.0f;

	return gcvSTATUS_OK;
}

static gceSTATUS
_FlattenPath(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsPATH_PTR Path
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	gctUINT pathHeadReserve;
	gcsCMDBUFFER_PTR currentBuffer;
	vgsPATH_DATA_PTR currentPath;

	gctUINT increment;
	vgsSUBPATH_PTR subPath = gcvNULL;
	vgsSUBPATH_PTR prevSubPath = gcvNULL;
	gctBOOL isRelative;
	gctFLOAT pathBias;
	gctFLOAT pathScale;
	gctUINT32 size;						/* The data size of the path. */
	gceVGCMD pathCommand;				/* Path command of each segment in the VGPath. */
	gceVGCMD prevCommand;				/* Previous command when iterating a path. */
	vgSL_ValueGetter getValue = gcvNULL;/* Funcion pointer to read a value from a general memory address. */
	gctINT dataTypeSize;				/* Size of the data type. */
	gctUINT8_PTR dataPointer = gcvNULL;	/* Pointer to the data of the VGPath. */
	gctFLOAT sx, sy;					/* Starting point of the current subpath. */
	gctFLOAT ox, oy;					/* Last point of the previous segment. */
	gctFLOAT px, py;					/* Last internal control point of the previous segment. */
	gctFLOAT x0, y0, x1, y1, x2, y2;	/* Coordinates of the command. */

	/* Get the path head reserve size. */
	pathHeadReserve = Path->pathInfo.reservedForHead;

	/* Set the initial buffer. */
	currentPath = Path->head;

	/* Initialize path bias and scale. */
	pathBias  = StrokeConversion->pathBias;
	pathScale = StrokeConversion->pathScale;

	sx = sy = ox = oy = px = py = 0.0f;

	prevCommand = gcvVGCMD_MOVE;

	/* Add a subpath. */
	gcmERROR_RETURN(_AddSubPath(StrokeConversion, &subPath));

	while (currentPath)
	{
		/* Get a shortcut to the path buffer. */
		currentBuffer = vgmGETCMDBUFFER(currentPath);

		/* Determine the data size. */
		size = currentBuffer->offset - pathHeadReserve;

		/* Determine the beginning of the path data. */
		dataPointer
			= (gctUINT8_PTR) currentBuffer
			+ currentBuffer->bufferOffset
			+ pathHeadReserve;

		/* Select the data picker. */
		switch (currentPath->data.dataType)
		{
		case gcePATHTYPE_INT8:
			if (pathScale == 1.0f)
			{
				getValue = (pathBias == 0.0f ? _GetS8_NS_NB : _GetS8_NS);
			}
			else
			{
				getValue = (pathBias == 0.0f ? _GetS8_NB : _GetS8);
			}
			dataTypeSize = 1;
			break;

		case gcePATHTYPE_INT16:
			if (pathScale == 1.0f)
			{
				getValue = (pathBias == 0.0f ? _GetS16_NS_NB : _GetS16_NS);
			}
			else
			{
				getValue = (pathBias == 0.0f ? _GetS16_NB : _GetS16);
			}
			dataTypeSize = 2;
			break;

		case gcePATHTYPE_INT32:
			if (pathScale == 1.0f)
			{
				getValue = (pathBias == 0.0f ? _GetS32_NS_NB : _GetS32_NS);
			}
			else
			{
				getValue = (pathBias == 0.0f ? _GetS32_NB : _GetS32);
			}
			dataTypeSize = 4;
			break;

		case gcePATHTYPE_FLOAT:
			if (pathScale == 1.0f)
			{
				getValue = (pathBias == 0.0f ? _GetF_NS_NB : _GetF_NS);
			}
			else
			{
				getValue = (pathBias == 0.0f ? _GetF_NB : _GetF);
			}
			dataTypeSize = 4;
			break;

		default:
			gcmASSERT(0);
			return gcvSTATUS_INVALID_ARGUMENT;
		}

		/* Add an extra gcvVGCMD_MOVE 0.0 0.0 to handle the case the first command is not gcvVGCMD_MOVE. */
		if ((currentPath == Path->head) &&
			(*dataPointer != gcvVGCMD_MOVE))
		{
			/* Add first point to subpath. */
			gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, sx, sy, subPath, vgcFLATTEN_NO));
		}

		while (size > 0)
		{
			/* Get the command. */
			pathCommand = *dataPointer & gcvVGCMD_MASK;

			/* Assume absolute. */
			isRelative = gcvFALSE;

			switch (pathCommand)
			{
			case gcvVGCMD_CLOSE:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_CLOSE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip the command. */
				dataPointer += 1;
				size -= 1;

				if (prevCommand == gcvVGCMD_CLOSE)
				{
					/* Continuous gcvVGCMD_CLOSE - do nothing. */
					break;
				}

				/* Check if subPath is already closed. */
				if (ox != sx || oy != sy)
				{
					/* Add a line from current point to the first point of current subpath. */
					gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, sx, sy, subPath, vgcFLATTEN_NO));
				}
				if (subPath->pointList != subPath->lastPoint)
				{
					/* Copy tangent data from first point to lastpoint. */
					vgsPOINT_PTR firstPoint = subPath->pointList;
					vgsPOINT_PTR lastPoint = subPath->lastPoint;
					lastPoint->length = firstPoint->length;
					lastPoint->tangentX = firstPoint->tangentX;
					lastPoint->tangentY = firstPoint->tangentY;
				}
				else
				{
					/* Single point path. */
					vgsPOINT_PTR point = subPath->pointList;
					point->tangentX = 0.0f;
					point->tangentY = 0.0f;
					point->length = 0.0f;
				}
				subPath->closed = gcvTRUE;
				subPath->lastPoint->next = gcvNULL;

				/* Add a subpath. */
				prevSubPath = subPath;
				gcmERROR_RETURN(_AddSubPath(StrokeConversion, &subPath));

				/* Add first point to subpath. */
				gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, sx, sy, subPath, vgcFLATTEN_NO));

				px = ox = sx;
				py = oy = sy;
				break;

			case gcvVGCMD_MOVE_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_MOVE:        /* Indicate the beginning of a new sub-path. */

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_MOVE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY(x0, y0);

				if (prevCommand != gcvVGCMD_MOVE && prevCommand != gcvVGCMD_MOVE_REL
					&& prevCommand != gcvVGCMD_CLOSE)
				{
					/* Close the current subpath, and add a subpath. */
					subPath->lastPoint->next = gcvNULL;
					if (subPath->pointCount == 1)
					{
						/* Single point path. */
						vgsPOINT_PTR point = subPath->pointList;
						point->tangentX = 0.0f;
						point->tangentY = 0.0f;
						point->length = 0.0f;
					}
					prevSubPath = subPath;
					gcmERROR_RETURN(_AddSubPath(StrokeConversion, &subPath));

					/* Add first point to subpath. */
					gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, x0, y0, subPath, vgcFLATTEN_NO));
				}
				else if (subPath->pointCount == 0)
				{
					/* First command is gcvVGCMD_MOVE. */
					/* Add first point to subpath. */
					gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, x0, y0, subPath, vgcFLATTEN_NO));
				}
				else
				{
					/* Continuous gcvVGCMD_MOVE. */
					/* Replace the previous move-to point to current move-to point. */
					gcmASSERT(subPath != gcvNULL);
					gcmASSERT(subPath->pointCount == 1);
					subPath->pointList->x = x0;
					subPath->pointList->y = y0;
				}

				sx = px = ox = x0;
				sy = py = oy = y0;
				break;

			case gcvVGCMD_LINE_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_LINE:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_LINE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY(x0, y0);

				/* Add a point to subpath. */
				gcmERROR_RETURN(_AddPointToSubPath(StrokeConversion, x0, y0, subPath, vgcFLATTEN_NO));

				px = ox = x0;
				py = oy = y0;
				break;

			case gcvVGCMD_QUAD_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_QUAD:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_QUAD, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY(x0, y0);
				VGSL_GETCOORDXY(x1, y1);

				if ((ox == x0 && oy == y0) && (ox == x1 && oy == y1))
				{
					/* Degenerated Bezier curve.  Becomes a point. */
					/* Discard zero-length segments. */
				}
				else if ((ox == x0 && oy == y0) || (x0 == x1 && y0 == y1))
				{
					/* Degenerated Bezier curve.  Becomes a line. */
					/* Add a point to subpath. */
					gcmERROR_RETURN(_AddPointToSubPath(
						StrokeConversion, x1, y1, subPath, vgcFLATTEN_NO
						));
				}
				else
				{
					gcmERROR_RETURN(_FlattenQuadBezier(
						StrokeConversion, ox, oy, x0, y0, x1, y1, subPath
						));
				}

				px = x0;
				py = y0;
				ox = x1;
				oy = y1;
				break;

			case gcvVGCMD_CUBIC_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_CUBIC:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_CUBIC, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY(x0, y0);
				VGSL_GETCOORDXY(x1, y1);
				VGSL_GETCOORDXY(x2, y2);

				if ((ox == x0 && oy == y0) && (ox == x1 && oy == y1) && (ox == x2 && oy == y2))
				{
					/* Degenerated Bezier curve.  Becomes a point. */
					/* Discard zero-length segments. */
				}
				else
				{
					gcmERROR_RETURN(_FlattenCubicBezier(
						StrokeConversion, ox, oy, x0, y0, x1, y1, x2, y2, subPath
						));
				}

				px = x1;
				py = y1;
				ox = x2;
				oy = y2;
				break;

			default:
				gcmASSERT(gcvFALSE);
				return gcvSTATUS_INVALID_DATA;
			}

			prevCommand = pathCommand;
		}

		/* Advance to the next subbuffer. */
		currentPath = (vgsPATH_DATA_PTR) currentBuffer->nextSubBuffer;
	}

	if ((prevCommand == gcvVGCMD_CLOSE) ||
		(prevCommand == gcvVGCMD_MOVE))
	{
		/* Remove the extra subpath. */
		gcmASSERT(subPath->pointCount == 1);
		_FreePoint(StrokeConversion->pointMemPool, subPath->pointList);
		_FreeSubPath(StrokeConversion->subPathMemPool, subPath);
		if (prevSubPath)
		{
			prevSubPath->next = gcvNULL;
		}
		else
		{
			StrokeConversion->subPathList = gcvNULL;
		}
		StrokeConversion->lastSubPath = prevSubPath;
	}
	else
	{
		subPath->lastPoint->next = gcvNULL;
		if (subPath->pointCount == 1)
		{
			/* Single point path. */
			vgsPOINT_PTR point = subPath->pointList;
			point->tangentX = 0.0f;
			point->tangentY = 0.0f;
			point->length = 0.0f;
		}
	}

	return status;
}




static gctBOOL
_IsAngleSpanAcute(
	gctFLOAT Ux,
	gctFLOAT Uy,
	gctFLOAT Vx,
	gctFLOAT Vy
	)
{
	return ((Ux * Vx + Uy * Vy) > 0.0f ? gcvTRUE : gcvFALSE);
}


static gctFLOAT
_Angle(
	gctFLOAT X,
	gctFLOAT Y,
	gctFLOAT Length
	)
{
	gctFLOAT angle;
	gctFLOAT ux = (X >= 0.0f ? X : -X);
	gctFLOAT uy = (Y >= 0.0f ? Y : -Y);

	/* angle = gcmACOSF(X / Length); */
	if (ux > uy)
	{
		angle = ((uy > 0.0f && ux < Length) ? _Asin(uy / Length) : 0.0f);
	}
	else
	{
		angle = ((ux > 0.0f && uy < Length) ? (FLOAT_PI_HALF - _Asin(ux / Length)) : FLOAT_PI_HALF);
	}
	gcmASSERT(angle >= -FLOAT_PI_HALF && angle <= FLOAT_PI_HALF);
	if (X < 0.0f) angle = FLOAT_PI - angle;
	if (Y < 0.0f) angle = -angle;

	return angle;
}

/* The arc is always counter clockwise and less than half circle (small). */
static gceSTATUS
_ConvertCircleArc(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	gctFLOAT Radius,
	gctFLOAT CenterX,
	gctFLOAT CenterY,
	gctFLOAT StartX,
	gctFLOAT StartY,
	gctFLOAT EndX,
	gctFLOAT EndY,
	gctBOOL HalfCircle,
	vgsPOINT_PTR *PointList
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	/*gceVGCMD segmentCommand;*/
	gctFLOAT theta1, thetaSpan;
	gctINT segs;
	gctFLOAT theta, thetaHalf, theta2;
	gctFLOAT cosThetaHalf;
	gctFLOAT controlRatio;
	gctFLOAT controlX, controlY, anchorX, anchorY;
	/*gctFLOAT lastX, lastY;*/
	vgsPOINT_PTR point, startPoint, lastPoint;

	/* Converting. */
	gcmASSERT(PointList);
	gcmASSERT(StartX != CenterX || StartY != CenterY);
	theta1 = _Angle(StartX - CenterX, StartY - CenterY, Radius);
	if (HalfCircle)
	{
		thetaSpan = FLOAT_PI;
		segs = 4;
		theta = FLOAT_PI_QUARTER;
		thetaHalf = FLOAT_PI_EIGHTH;
		cosThetaHalf = FLOAT_COS_PI_EIGHTH;
	}
	else
	{
		gcmASSERT(EndX != CenterX || EndY != CenterY);
		thetaSpan = _Angle(EndX - CenterX, EndY - CenterY, Radius) - theta1;
		if (thetaSpan == 0.0f)
		{
			/* Handle specail case for huge scaling. */
			gcmASSERT(thetaSpan != 0.0f);
			*PointList = gcvNULL;
			return gcvSTATUS_OK;
		}

		if ((thetaSpan < 0))
		{
			thetaSpan += FLOAT_PI_TWO;
		}

		gcmASSERT(thetaSpan > 0.0f && thetaSpan < FLOAT_PI + 0.015f);

		/* Calculate the number of quadratic Bezier curves. */
		/* Assumption: most of angles are small angles. */
		if      (thetaSpan <= FLOAT_PI_QUARTER)			segs = 1;
		else if (thetaSpan <= FLOAT_PI_HALF)			segs = 2;
		else if (thetaSpan <= FLOAT_PI_THREE_QUARTER)	segs = 3;
		else											segs = 4;

		theta = thetaSpan / segs;
		thetaHalf = theta / 2.0f;
		cosThetaHalf = _Cos(thetaHalf);
	}

	/* Determine the segment command. */
	/*egmentCommand = gcvVGCMD_ARC_QUAD;*/


	/* Generate quadratic Bezier curves. */
	startPoint = lastPoint = gcvNULL;
	controlRatio = Radius / cosThetaHalf;
	while (segs-- > 0)
	{
		theta1 += theta;

		theta2 = theta1 - thetaHalf;
		if (theta2 > FLOAT_PI) theta2 -= FLOAT_PI_TWO;
		controlX = CenterX + _Cos(theta2) * controlRatio;
#ifndef __QNXNTO__
		controlY = CenterY + _Sin(theta2) * controlRatio;
#else
		controlY = CenterY + _Sin_qnx(theta2) * controlRatio;
#endif

		theta2 = theta1;
		if (theta2 > FLOAT_PI) theta2 -= FLOAT_PI_TWO;
		anchorX = CenterX + _Cos(theta2) * Radius;
#ifndef __QNXNTO__
		anchorY = CenterY + _Sin(theta2) * Radius;
#else
		anchorY = CenterY + _Sin_qnx(theta2) * Radius;
#endif

		if (segs == 0)
		{
			/* Use end point directly to avoid accumulated errors. */
			anchorX = EndX;
			anchorY = EndY;
		}

		/* Add control point. */
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &point));
		point->x = controlX;
		point->y = controlY;
		point->curveType = vgcCURVE_QUAD_CONTROL;
		if (lastPoint)
		{
			lastPoint->next = point;
			lastPoint = point;
		}
		else
		{
			startPoint = lastPoint = point;
		}

		/* Add anchor point. */
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &point));
		point->x = anchorX;
		point->y = anchorY;
		point->curveType = vgcCURVE_QUAD_ANCHOR;
		lastPoint->next = point;
		lastPoint = point;
	}

	lastPoint->next = gcvNULL;
	*PointList = startPoint;

	/* Return status. */
	return status;
}

static gceSTATUS
_AddStrokeSubPath(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsSUBPATH_PTR * SubPath
	)
{
	gceSTATUS status;

	gcmERROR_RETURN(_CAllocateSubPath(StrokeConversion->subPathMemPool, SubPath));

	if (StrokeConversion->lastStrokeSubPath != gcvNULL)
	{
		StrokeConversion->lastStrokeSubPath->next = *SubPath;
		StrokeConversion->lastStrokeSubPath = *SubPath;
	}
	else
	{
		StrokeConversion->lastStrokeSubPath = StrokeConversion->strokeSubPathList = *SubPath;
	}

	return status;
}

gcmINLINE gceSTATUS
_AddAPointToRightStrokePointListTail(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	gctFLOAT X,
	gctFLOAT Y
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsPOINT_PTR point;

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &point));
	point->x = X;
	point->y = Y;
	point->curveType = vgcCURVE_LINE;
	point->prev = StrokeConversion->lastRightStrokePoint;
	point->next = gcvNULL;
	StrokeConversion->lastRightStrokePoint->next = point;
	StrokeConversion->lastRightStrokePoint = point;
	StrokeConversion->lastStrokeSubPath->pointCount++;

	return status;
}

gcmINLINE gceSTATUS
_AddAPointToLeftStrokePointListHead(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	gctFLOAT X,
	gctFLOAT Y
	)
{
	gceSTATUS status;
	vgsPOINT_PTR point;

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &point));
	point->x = X;
	point->y = Y;
	point->curveType = vgcCURVE_LINE;
	point->next = StrokeConversion->leftStrokePoint;
	point->prev = gcvNULL;
	StrokeConversion->leftStrokePoint->prev = point;
	StrokeConversion->leftStrokePoint = point;
	StrokeConversion->lastStrokeSubPath->pointCount++;

	return status;
}

static gceSTATUS
_AddZeroLengthStrokeSubPath(
	IN vgsSTROKECONVERSION_PTR StrokeConversion,
	IN vgsPOINT_PTR Point
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsSUBPATH_PTR strokeSubPath;
	vgsPOINT_PTR newPoint;
	gctFLOAT halfWidth = StrokeConversion->halfLineWidth;

	if (StrokeConversion->strokeCapStyle == gcvCAP_BUTT)
	{
		/* No need to draw zero-length subPath for gcvCAP_BUTT. */
		return gcvSTATUS_OK;
	}

	gcmERROR_RETURN(_AddStrokeSubPath(StrokeConversion, &strokeSubPath));
	if (StrokeConversion->strokeCapStyle == gcvCAP_SQUARE)
	{
		/* Draw a square along the point's direction. */
		gctFLOAT dx, dy;

		if (Point->tangentX == 0.0f || Point->tangentY == 0.0f)
		{
			dx = halfWidth;
			dy = 0.0f;
		}
		else
		{
			dx =  Point->tangentY * halfWidth;
			dy = -Point->tangentX * halfWidth;
		}
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &newPoint));
		newPoint->x = Point->x + dx + dy;
		newPoint->y = Point->y - dx + dy;
		newPoint->curveType = vgcCURVE_LINE;
		strokeSubPath->pointList = StrokeConversion->lastRightStrokePoint = newPoint;
		strokeSubPath->pointCount = 1;
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
										Point->x + dx - dy, Point->y + dx + dy));
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
										Point->x - dx - dy, Point->y + dx - dy));
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
										Point->x - dx + dy, Point->y - dx - dy));
	}
	else
	{
		/* Draw a circle. */
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &newPoint));
		newPoint->x = Point->x + halfWidth;
		newPoint->y = Point->y;
		newPoint->curveType = vgcCURVE_LINE;
		strokeSubPath->pointList = StrokeConversion->lastRightStrokePoint = newPoint;
		strokeSubPath->pointCount = 1;

		/* Add upper half circle. */
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
										Point->x - halfWidth, Point->y));
		StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW_HALF;
		StrokeConversion->lastRightStrokePoint->centerX = Point->x;
		StrokeConversion->lastRightStrokePoint->centerY = Point->y;

		/* Add lower half circle. */
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
										Point->x + halfWidth, Point->y));
		StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW_HALF;
		StrokeConversion->lastRightStrokePoint->centerX = Point->x;
		StrokeConversion->lastRightStrokePoint->centerY = Point->y;
	}

	strokeSubPath->lastPoint = StrokeConversion->lastRightStrokePoint;
	strokeSubPath->lastPoint->next = gcvNULL;

	return status;
}

static gceSTATUS
_StartANewStrokeSubPath(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	gctFLOAT X,
	gctFLOAT Y,
	gctFLOAT Dx,
	gctFLOAT Dy,
	gctBOOL AddEndCap,
	vgsSUBPATH_PTR * StrokeSubPath
	)
{
	gceSTATUS status;
	vgsSUBPATH_PTR strokeSubPath;
	vgsPOINT_PTR newPoint;

	gcmERROR_RETURN(_AddStrokeSubPath(StrokeConversion, &strokeSubPath));

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &newPoint));
	newPoint->x = X + Dx;
	newPoint->y = Y + Dy;
	newPoint->prev = gcvNULL;
	newPoint->curveType = vgcCURVE_LINE;
	strokeSubPath->pointList = StrokeConversion->lastRightStrokePoint = newPoint;

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &newPoint));
	newPoint->x = X - Dx;
	newPoint->y = Y - Dy;
	newPoint->curveType = vgcCURVE_LINE;
	newPoint->next = gcvNULL;
	strokeSubPath->lastPoint = StrokeConversion->leftStrokePoint = newPoint;

	strokeSubPath->pointCount = 2;

	if (AddEndCap)
	{
		/* Add end cap if the subPath is not closed. */
		switch (StrokeConversion->strokeCapStyle)
		{
		case gcvCAP_BUTT:
			/* No adjustment needed. */
			break;
		case gcvCAP_ROUND:
			/* Add curve. */
			/* Add the starting point again as arc. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
					strokeSubPath->pointList->x, strokeSubPath->pointList->y));
			StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW_HALF;
			StrokeConversion->lastRightStrokePoint->centerX = X;
			StrokeConversion->lastRightStrokePoint->centerY = Y;
			/* Change the starting point to end point. */
			strokeSubPath->pointList->x = strokeSubPath->lastPoint->x;
			strokeSubPath->pointList->y = strokeSubPath->lastPoint->y;
			break;
		case gcvCAP_SQUARE:
			StrokeConversion->lastRightStrokePoint->x += Dy;
			StrokeConversion->lastRightStrokePoint->y -= Dx;
			StrokeConversion->leftStrokePoint->x += Dy;
			StrokeConversion->leftStrokePoint->y -= Dx;
			break;
		}
	}

	*StrokeSubPath = strokeSubPath;
	return status;
}

static gceSTATUS
_EndAStrokeSubPath(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	gctFLOAT X,
	gctFLOAT Y,
	gctFLOAT Dx,
	gctFLOAT Dy
	)
{
	gceSTATUS status;

	/* Add points for end of line. */
	gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
										X + Dx, Y + Dy));
	gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion,
										X - Dx, Y - Dy));

	/* Add end cap if the subPath is not closed. */
	switch (StrokeConversion->strokeCapStyle)
	{
	case gcvCAP_BUTT:
		/* No adjustment needed. */
		break;
	case gcvCAP_ROUND:
		/* Add curve. */
		gcmASSERT(StrokeConversion->leftStrokePoint->curveType == vgcCURVE_LINE);
		StrokeConversion->leftStrokePoint->curveType = vgcCURVE_ARC_SCCW_HALF;
		StrokeConversion->leftStrokePoint->centerX = X;
		StrokeConversion->leftStrokePoint->centerY = Y;
		break;
	case gcvCAP_SQUARE:
		StrokeConversion->lastRightStrokePoint->x -= Dy;
		StrokeConversion->lastRightStrokePoint->y += Dx;
		StrokeConversion->leftStrokePoint->x -= Dy;
		StrokeConversion->leftStrokePoint->y += Dx;
		break;
	}

	/* Concatnate right and left point lists. */
	StrokeConversion->lastRightStrokePoint->next = StrokeConversion->leftStrokePoint;
	StrokeConversion->leftStrokePoint->prev = StrokeConversion->lastRightStrokePoint;

	/*gcmERROR_RETURN(_CheckStrokeSubPath(StrokeConversion->lastStrokeSubPath));*/

	return status;
}

gcmINLINE void
_GetNextDashLength(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	gctINT * DashIndex,
	gctFLOAT * DashLength
	)
{
	(*DashIndex)++;
	if (*DashIndex == StrokeConversion->strokeDashPatternCount)
	{
		*DashIndex = 0;
	}
	*DashLength = StrokeConversion->strokeDashPattern[*DashIndex];
}

static gceSTATUS
_DrawSwingPieArea(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsPOINT_PTR CenterPoint,
	gctBOOL EndAtPrevPoint
	)
{
	gceSTATUS status;
#if USE_NEW_SWING_HANDLE_FOR_END
	gctFLOAT halfWidth = StrokeConversion->halfLineWidth;
	vgsSUBPATH_PTR subPath = StrokeConversion->currentSubPath;
	vgsSUBPATH_PTR strokeSubPath = StrokeConversion->lastStrokeSubPath;
#endif

	/*gcmERROR_RETURN(_CheckLastStrokeSubPathPartial(StrokeConversion));*/

	if (StrokeConversion->swingCounterClockwise)
	{
		vgsPOINT_PTR startPoint = StrokeConversion->swingStartStrokePoint;
		vgsPOINT_PTR endPoint = gcvNULL, realEndPoint = gcvNULL;
		vgsPOINT_PTR point, prevPoint;
		gctUINT count = 0;

#if USE_NEW_SWING_HANDLE_FOR_END
		if ((StrokeConversion->swingHandling == vgcSWING_OUT &&
			 subPath->length - StrokeConversion->swingAccuLength > halfWidth) ||
			(StrokeConversion->swingHandling == vgcSWING_IN &&
			 StrokeConversion->swingAccuLength > halfWidth))
		{
			vgsPOINT_PTR preStartPoint = startPoint->next;

			/* Delete the points between preStartPoint and endPoint. */
			endPoint = StrokeConversion->leftStrokePoint;
			if (endPoint->next == startPoint)
			{
				gcmASSERT(endPoint->curveType == vgcCURVE_ARC_SCCW);
				gcmASSERT(startPoint->curveType == vgcCURVE_LINE);
				endPoint->curveType = vgcCURVE_LINE;
				preStartPoint->prev = endPoint;
				endPoint->next = preStartPoint;
				_FreePoint(StrokeConversion->pointMemPool, startPoint);
				strokeSubPath->pointCount--;
			}
			else
			{
				if (endPoint->curveType == vgcCURVE_LINE)
				{
					/* Delete endPoint because it is not the max swing out point. */
					gcmASSERT(endPoint->next->curveType == vgcCURVE_ARC_SCCW);
					StrokeConversion->leftStrokePoint = endPoint->next;
					_FreePoint(StrokeConversion->pointMemPool, endPoint);
					strokeSubPath->pointCount--;
					endPoint = StrokeConversion->leftStrokePoint;
					endPoint->prev = gcvNULL;
				}
				gcmASSERT(endPoint->curveType == vgcCURVE_ARC_SCCW);
				for (point = startPoint; point != endPoint; point = prevPoint)
				{
					prevPoint = point->prev;
					_FreePoint(StrokeConversion->pointMemPool, point);
					strokeSubPath->pointCount--;
				}
				gcmASSERT(endPoint->curveType == vgcCURVE_ARC_SCCW);
				endPoint->curveType = vgcCURVE_LINE;
				preStartPoint->prev = endPoint;
				endPoint->next = preStartPoint;
			}
		}
		else
#endif
		{
			if (EndAtPrevPoint)
			{
				/* Detach the end point from leftStrokePoint. */
				/* The end point will be added back later. */
				realEndPoint = StrokeConversion->leftStrokePoint;
				StrokeConversion->leftStrokePoint = realEndPoint->next;
				StrokeConversion->leftStrokePoint->prev = gcvNULL;
			}

			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion,
							CenterPoint->x, CenterPoint->y));
			endPoint = StrokeConversion->leftStrokePoint;

			/* Reverse the point list from startPoint to endPoint. */
			for (point = startPoint; point; point = prevPoint)
			{
				prevPoint = point->prev;
				point->prev = point->next;
				point->next = prevPoint;
				count++;
			}
			gcmASSERT(count == StrokeConversion->swingCount * 2 + 1);
			gcmASSERT(endPoint->next == gcvNULL);
			endPoint->next = startPoint->prev;
			startPoint->prev->prev = endPoint;
			startPoint->prev = gcvNULL;
			StrokeConversion->leftStrokePoint = startPoint;

			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion,
							CenterPoint->x, CenterPoint->y));
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion,
							StrokeConversion->swingStartPoint->x,
							StrokeConversion->swingStartPoint->y));
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion,
							endPoint->prev->x, endPoint->prev->y));

			if (EndAtPrevPoint)
			{
				realEndPoint->next = StrokeConversion->leftStrokePoint;
				StrokeConversion->leftStrokePoint->prev = realEndPoint;
				StrokeConversion->leftStrokePoint = realEndPoint;
			}
		}
	}
	else
	{
		vgsPOINT_PTR startPoint = StrokeConversion->swingStartStrokePoint;
		vgsPOINT_PTR endPoint = gcvNULL, realEndPoint = gcvNULL;
		vgsPOINT_PTR point, nextPoint;
		gctUINT count = 0;

#if USE_NEW_SWING_HANDLE_FOR_END
		if ((StrokeConversion->swingHandling == vgcSWING_OUT &&
			 subPath->length - StrokeConversion->swingAccuLength > halfWidth) ||
			(StrokeConversion->swingHandling == vgcSWING_IN &&
			 StrokeConversion->swingAccuLength > halfWidth))
		{
			vgsPOINT_PTR preStartPoint = startPoint->prev;

			/* Delete the points between preStartPoint and endPoint. */
			endPoint = StrokeConversion->lastRightStrokePoint;
			if (endPoint->prev == startPoint)
			{
				gcmASSERT(endPoint->curveType == vgcCURVE_LINE);
				gcmASSERT(startPoint->curveType == vgcCURVE_ARC_SCCW);
				preStartPoint->next = endPoint;
				endPoint->prev = preStartPoint;
				_FreePoint(StrokeConversion->pointMemPool, startPoint);
				strokeSubPath->pointCount--;
			}
			else
			{
				gcmASSERT(endPoint->curveType == vgcCURVE_LINE);
				if (endPoint->prev->curveType == vgcCURVE_LINE)
				{
					/* Delete endPoint because it is not the max swing out point. */
					StrokeConversion->lastRightStrokePoint = endPoint->prev;
					_FreePoint(StrokeConversion->pointMemPool, endPoint);
					strokeSubPath->pointCount--;
					endPoint = StrokeConversion->lastRightStrokePoint;
					endPoint->next = gcvNULL;
				}
				gcmASSERT(endPoint->prev->curveType == vgcCURVE_ARC_SCCW);
				for (point = startPoint; point != endPoint; point = nextPoint)
				{
					nextPoint = point->next;
					_FreePoint(StrokeConversion->pointMemPool, point);
					strokeSubPath->pointCount--;
				}
				gcmASSERT(endPoint->curveType == vgcCURVE_LINE);
				preStartPoint->next = endPoint;
				endPoint->prev = preStartPoint;
			}
		}
		else
#endif
		{
			if (EndAtPrevPoint)
			{
				/* Detach the end point from leftStrokePoint. */
				/* The end point will be added back later. */
				realEndPoint = StrokeConversion->lastRightStrokePoint;
				StrokeConversion->lastRightStrokePoint = realEndPoint->prev;
				StrokeConversion->lastRightStrokePoint->next = gcvNULL;
			}

			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
							CenterPoint->x, CenterPoint->y));
			endPoint = StrokeConversion->lastRightStrokePoint;

			/* Reverse the point list from startPoint to endPoint. */
			for (point = startPoint; point; point = nextPoint)
			{
				nextPoint = point->next;
				point->next = point->prev;
				point->prev = nextPoint;
				count++;
			}
			gcmASSERT(count == StrokeConversion->swingCount * 2 + 1);
			gcmASSERT(endPoint->prev == gcvNULL);
			endPoint->prev = startPoint->next;
			startPoint->next->next = endPoint;
			startPoint->next = gcvNULL;
			StrokeConversion->lastRightStrokePoint = startPoint;

			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
							CenterPoint->x, CenterPoint->y));
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
							StrokeConversion->swingStartPoint->x,
							StrokeConversion->swingStartPoint->y));
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
							endPoint->next->x, endPoint->next->y));

			if (EndAtPrevPoint)
			{
				realEndPoint->prev = StrokeConversion->lastRightStrokePoint;
				StrokeConversion->lastRightStrokePoint->next = realEndPoint;
				StrokeConversion->lastRightStrokePoint = realEndPoint;
			}
		}
	}

	/*gcmERROR_RETURN(_CheckLastStrokeSubPathPartial(StrokeConversion));*/

	StrokeConversion->swingHandling = vgcSWING_NO;
	return gcvSTATUS_OK;
}

static void
_AdjustJointPoint(
	vgsPOINT_PTR Point,
	vgsPOINT_PTR JoinPoint,
	gctFLOAT X,
	gctFLOAT Y,
	gctFLOAT Ratio
	)
{
	gctFLOAT mx = (JoinPoint->x + X) / 2.0f;
	gctFLOAT my = (JoinPoint->y + Y) / 2.0f;
	gctFLOAT dx = mx - Point->x;
	gctFLOAT dy = my - Point->y;

	dx = dx * Ratio;
	dy = dy * Ratio;
	JoinPoint->x = Point->x + dx;
	JoinPoint->y = Point->y + dy;
}

static gceSTATUS
_ProcessLineJoint(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsPOINT_PTR Point,
	gctFLOAT Length,
	gctFLOAT PrevLength,
	gctUINT SwingHandling,
	gctFLOAT X1,
	gctFLOAT Y1,
	gctFLOAT X2,
	gctFLOAT Y2
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gceJOIN_STYLE strokeJoinStyle = StrokeConversion->strokeJoinStyle;
	gctFLOAT halfWidth = StrokeConversion->halfLineWidth;
	gctFLOAT ratio;
	gctFLOAT minLengthSquare;
	gctFLOAT cosTheta;
	gctBOOL counterClockwise;
	gctBOOL fatLine = StrokeConversion->isFat;
	gctUINT swingHandling = vgcSWING_NO;
	gctBOOL handleShortLine = gcvFALSE;

	if (StrokeConversion->swingAccuLength < halfWidth)
	{
		if (StrokeConversion->swingNeedToHandle)
		{
			swingHandling = vgcSWING_OUT;
		}
		else
		{
			handleShortLine = gcvTRUE;
		}
	}
	else if (StrokeConversion->currentSubPath->length - StrokeConversion->swingAccuLength < halfWidth)
	{
		if (StrokeConversion->swingNeedToHandle)
		{
			swingHandling = vgcSWING_IN;
		}
		else
		{
			handleShortLine = gcvTRUE;
		}
	}

	if (swingHandling != SwingHandling)
    {
        gcmASSERT(SwingHandling != vgcSWING_NO);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

	if (StrokeConversion->swingHandling != vgcSWING_NO)
	{
		gcmASSERT(SwingHandling != vgcSWING_NO);
	}

	/* For flattened curves/arcs, the join style is always round. */
	if ((Point->flattenFlag != vgcFLATTEN_NO) && fatLine)
	{
		strokeJoinStyle = gcvJOIN_ROUND;
	}

	/* First, determine the turn is clockwise or counter-clockwise. */
	cosTheta = Point->prev->tangentX * Point->tangentX + Point->prev->tangentY * Point->tangentY;

	if (cosTheta > FLOAT_ANGLE_EPSILON_COS)
	{
		/* Straight line or semi-straight line--no need to handle join. */
		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			/* Begin to swing to the opposite direction. */
			/* Draw the swing area (pie area). */
			gcmERROR_RETURN(_DrawSwingPieArea(StrokeConversion, Point->prev, gcvTRUE));
		}

		/* Add the new stroke points. */
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, X1, Y1));
		gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, X2, Y2));
		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			gcmASSERT(Point->flattenFlag == vgcFLATTEN_END);
			StrokeConversion->swingCount++;
		}

		VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredJointsByAngleEpsilonCount);
		goto endCheck;
	}
	else if (cosTheta < -FLOAT_ANGLE_EPSILON_COS)
	{
		/* Almost 180 degree turn. */
		counterClockwise = gcvTRUE;
		ratio = FLOAT_MAX;
		minLengthSquare = FLOAT_MAX;
	}
	else
	{
		gctFLOAT angleSign = Point->prev->tangentX * Point->tangentY - Point->prev->tangentY * Point->tangentX;
		counterClockwise = (angleSign >= 0.0f ? gcvTRUE : gcvFALSE);
		/*gctFLOAT halfTheta = theta / 2.0f;*/
		/*miterRatio = 1.0f / gcmCOSF(halfTheta);*/
		/*ratio = miterRatio * miterRatio;*/
		/*halfMiterLength = halfWidth * tanf(halfTheta);*/
		ratio = 2.0f / (1.0f + cosTheta);
		minLengthSquare = halfWidth * halfWidth * (1.0f - cosTheta) / (1.0f + cosTheta) + 0.02f;
	}

	if (StrokeConversion->swingHandling != vgcSWING_NO)
	{
		gcmASSERT(SwingHandling != vgcSWING_NO);
		if (counterClockwise != StrokeConversion->swingCounterClockwise)
		{
			/* Swing to the opposite direction. */
			/* Draw the swing area (pie area). */
			gcmERROR_RETURN(_DrawSwingPieArea(StrokeConversion, Point->prev, gcvTRUE));
		}
	}

	if (counterClockwise)
	{
		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			vgsPOINT_PTR prevPoint = StrokeConversion->leftStrokePoint->next;	/* Skip the line segment movement. */
			gctFLOAT deltaX = X2 - prevPoint->x;
			gctFLOAT deltaY = Y2 - prevPoint->y;
			if (_IsAngleSpanAcute(StrokeConversion->swingStrokeDeltaX,
									StrokeConversion->swingStrokeDeltaY,
									deltaX, deltaY))
			{
				/* Continue swinging. */
				StrokeConversion->swingStrokeDeltaX = deltaX;
				StrokeConversion->swingStrokeDeltaY = deltaY;
			}
			else
			{
				/* Swing to the max. */
				/* Draw the swing area (pie area). */
				gcmERROR_RETURN(_DrawSwingPieArea(StrokeConversion, Point->prev, gcvTRUE));
			}
		}

		/* Check if the miter length is too long for inner intersection. */
		if (StrokeConversion->swingHandling == vgcSWING_NO
		&& ! handleShortLine
		&& minLengthSquare <= Length * Length
		&& minLengthSquare <= PrevLength * PrevLength)
		{
			/* Adjust leftStrokePoint to the intersection point. */
			_AdjustJointPoint(Point, StrokeConversion->leftStrokePoint, X2, Y2, ratio);
		}
		else if (StrokeConversion->swingHandling == vgcSWING_NO && Point->flattenFlag == vgcFLATTEN_NO)
		{
			/* Add the point to avoid incorrect sharp angle. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, Point->x, Point->y));
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, X2, Y2));
		}
		else if (StrokeConversion->swingHandling == vgcSWING_NO && (! fatLine || SwingHandling == vgcSWING_NO))
		{
			/* Flattened line segments should not have sharp angle. */
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, X2, Y2));
		}
		else
		{
			if (StrokeConversion->swingHandling == vgcSWING_NO)
			{
				vgsPOINT_PTR prevPoint = StrokeConversion->leftStrokePoint;

				/* Start swing handling. */
				StrokeConversion->swingHandling = SwingHandling;
				StrokeConversion->swingCounterClockwise = gcvTRUE;
				StrokeConversion->swingStartPoint = Point;
				StrokeConversion->swingCenterLength = 0.0f;
				StrokeConversion->swingCount= 0;

				/* Save stroking path delta. */
				StrokeConversion->swingStrokeDeltaX = X2 - prevPoint->x;
				StrokeConversion->swingStrokeDeltaY = Y2 - prevPoint->y;

				/* Add extra center point for swing out pie area. */
				/* TODO - Should adjust prevPoint, instead of adding new point? */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, Point->x, Point->y));

				/* Add extra start stroke point for swing out pie area. */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, prevPoint->x, prevPoint->y));

				StrokeConversion->swingStartStrokePoint = StrokeConversion->leftStrokePoint;
			}

#if USE_MIN_ARC_FILTER
			if (cosTheta > FLOAT_MIN_ARC_ANGLE_COS)
			{
				/* Add a point. */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, X2, Y2));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredByMinArcAngleCount);
			}
			else
#endif
			{
				/* Add curve. */
				/* Note that the curve will be reversed, so the direction is CW. */
				/* Then, left side is in reversed order, so the direction is CCW. */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, X2, Y2));
				StrokeConversion->leftStrokePoint->curveType = vgcCURVE_ARC_SCCW;
				StrokeConversion->leftStrokePoint->centerX = Point->x;
				StrokeConversion->leftStrokePoint->centerY = Point->y;
			}
			StrokeConversion->swingCount++;
		}

		switch (strokeJoinStyle)
		{
		case gcvJOIN_ROUND:
#if USE_MIN_ARC_FILTER
			if (cosTheta > FLOAT_MIN_ARC_ANGLE_COS)
			{
				/* Add a point. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, X1, Y1));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredByMinArcAngleCount);
			}
			else
#endif
			{
				/* Add curve. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, X1, Y1));
				StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW;
				StrokeConversion->lastRightStrokePoint->centerX = Point->x;
				StrokeConversion->lastRightStrokePoint->centerY = Point->y;
			}
			break;
		case gcvJOIN_MITER:
			if (ratio <= StrokeConversion->strokeMiterLimitSquare)
			{
				/* Adjust lastRightStrokePoint to the outer intersection point. */
				_AdjustJointPoint(Point, StrokeConversion->lastRightStrokePoint, X1, Y1, ratio);
				break;
			}
			/* Else use Bevel join style. */
		case gcvJOIN_BEVEL:
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, X1, Y1));
			break;
		}
	}
	else
	{
		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			vgsPOINT_PTR prevPoint = StrokeConversion->lastRightStrokePoint->prev;	/* Skip the line segment movement. */
			gctFLOAT deltaX = X1 - prevPoint->x;
			gctFLOAT deltaY = Y1 - prevPoint->y;
			if (_IsAngleSpanAcute(StrokeConversion->swingStrokeDeltaX,
									StrokeConversion->swingStrokeDeltaY,
									deltaX, deltaY))
			{
				/* Continue swinging. */
				StrokeConversion->swingStrokeDeltaX = deltaX;
				StrokeConversion->swingStrokeDeltaY = deltaY;
			}
			else
			{
				/* Swing to the max. */
				/* Draw the swing area (pie area). */
				gcmERROR_RETURN(_DrawSwingPieArea(StrokeConversion, Point->prev, gcvTRUE));
			}
		}

		/* Check if the miter length is too long for inner intersection. */
		if (StrokeConversion->swingHandling == vgcSWING_NO
		&& ! handleShortLine
		&& minLengthSquare <= Length * Length
		&& minLengthSquare <= PrevLength * PrevLength)
		{
			/* Adjust lastRightStrokePoint to the intersection point. */
			_AdjustJointPoint(Point, StrokeConversion->lastRightStrokePoint, X1, Y1, ratio);
		}
		else if (StrokeConversion->swingHandling == vgcSWING_NO && Point->flattenFlag == vgcFLATTEN_NO)
		{
			/* Add the point to avoid incorrect sharp angle. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, Point->x, Point->y));
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, X1, Y1));
		}
		else if (StrokeConversion->swingHandling == vgcSWING_NO && (! fatLine || SwingHandling == vgcSWING_NO))
		{
			/* Flattened line segments should not have sharp angle. */
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, X1, Y1));
		}
		else
		{
			if (StrokeConversion->swingHandling == vgcSWING_NO)
			{
				vgsPOINT_PTR prevPoint = StrokeConversion->lastRightStrokePoint;

				/* Start swing handling. */
				StrokeConversion->swingHandling = SwingHandling;
				StrokeConversion->swingCounterClockwise = gcvFALSE;
				StrokeConversion->swingStartPoint = Point;
				StrokeConversion->swingCenterLength = 0.0f;
				StrokeConversion->swingCount= 0;

				/* Save stroking path delta. */
				StrokeConversion->swingStrokeDeltaX = X1 - prevPoint->x;
				StrokeConversion->swingStrokeDeltaY = Y1 - prevPoint->y;

				/* Add extra center point for swing out pie area. */
				/* TODO - Should adjust prevPoint, instead of adding new point? */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, Point->x, Point->y));

				/* Add extra start stroke point for swing out pie area. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, prevPoint->x, prevPoint->y));

				StrokeConversion->swingStartStrokePoint = StrokeConversion->lastRightStrokePoint;
			}

#if USE_MIN_ARC_FILTER
			if (cosTheta > FLOAT_MIN_ARC_ANGLE_COS)
			{
				/* Add a point. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, X1, Y1));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredByMinArcAngleCount);
			}
			else
#endif
			{
				/* Add curve. */
				/* Note that the curve will be reversed, so the direction is CCW. */
				StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW;
				StrokeConversion->lastRightStrokePoint->centerX = Point->x;
				StrokeConversion->lastRightStrokePoint->centerY = Point->y;
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion, X1, Y1));
			}
			StrokeConversion->swingCount++;
		}

		switch (strokeJoinStyle)
		{
		case gcvJOIN_ROUND:
#if USE_MIN_ARC_FILTER
			if (cosTheta > FLOAT_MIN_ARC_ANGLE_COS)
			{
				/* Add a point. */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, X2, Y2));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredByMinArcAngleCount);
			}
			else
#endif
			{
				/* Add curve. */
				StrokeConversion->leftStrokePoint->curveType = vgcCURVE_ARC_SCCW;
				StrokeConversion->leftStrokePoint->centerX = Point->x;
				StrokeConversion->leftStrokePoint->centerY = Point->y;
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, X2, Y2));
			}
			break;
		case gcvJOIN_MITER:
			if (ratio <= StrokeConversion->strokeMiterLimitSquare)
			{
				/* Adjust leftStrokePoint to the outer intersection point. */
				_AdjustJointPoint(Point, StrokeConversion->leftStrokePoint, X2, Y2, ratio);
				break;
			}
			/* Else use Bevel join style. */
		case gcvJOIN_BEVEL:
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion, X2, Y2));
			break;
		}
	}

endCheck:
	if (StrokeConversion->swingNeedToHandle)
	{
		StrokeConversion->swingAccuLength += Point->length;
	}
	if (StrokeConversion->swingHandling != vgcSWING_NO)
	{
		if (Point->flattenFlag == vgcFLATTEN_END ||
			(StrokeConversion->swingHandling == vgcSWING_OUT &&
			 StrokeConversion->swingAccuLength > halfWidth))
		{
			/* Draw the swing area (pie area). */
			gcmERROR_RETURN(_DrawSwingPieArea(StrokeConversion, Point, gcvFALSE));
		}
		else
		{
			/* Check if center line will move too far. */
			StrokeConversion->swingCenterLength += Point->length;
			if (StrokeConversion->swingCenterLength > FLOAT_SWING_CENTER_RANGE)
			{
#if USE_NEW_SWING_HANDLE_FOR_END
				if (StrokeConversion->currentSubPath->length < halfWidth ||
					Point->next->flattenFlag == vgcFLATTEN_END)
#endif
				{
					/* Draw the swing area (pie area). */
					gcmERROR_RETURN(_DrawSwingPieArea(StrokeConversion, Point, gcvFALSE));
				}
			}
		}
	}

	return status;
}

static gceSTATUS
_CloseAStrokeSubPath(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsPOINT_PTR Point,
	gctFLOAT Length,
	gctFLOAT PrevLength,
	gctUINT SwingHandling,
	vgsPOINT_PTR firstStrokePoint,
	vgsPOINT_PTR lastStrokePoint
	)
{
	gceSTATUS status;

	/* Handle line joint style for the first/last point in closed path. */
	gcmERROR_RETURN(_ProcessLineJoint(
		StrokeConversion, Point,
		Length, PrevLength, SwingHandling,
		firstStrokePoint->x, firstStrokePoint->y,
		lastStrokePoint->x, lastStrokePoint->y
		));

	/* Adjust the two end ponts of the first point. */
	firstStrokePoint->x = StrokeConversion->lastRightStrokePoint->x;
	firstStrokePoint->y = StrokeConversion->lastRightStrokePoint->y;
	lastStrokePoint->x = StrokeConversion->leftStrokePoint->x;
	lastStrokePoint->y = StrokeConversion->leftStrokePoint->y;

	/* Concatnate right and left point lists. */
	StrokeConversion->lastRightStrokePoint->next = StrokeConversion->leftStrokePoint;
	StrokeConversion->leftStrokePoint->prev = StrokeConversion->lastRightStrokePoint;

	/*gcmERROR_RETURN(_CheckStrokeSubPath(StrokeConversion->lastStrokeSubPath));*/

	return status;
}

static gceSTATUS
_CreateStrokePath(
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsSUBPATH_PTR subPath, strokeSubPath;
	vgsPOINT_PTR point, nextPoint;
	gctFLOAT halfWidth = StrokeConversion->halfLineWidth;
	gctFLOAT x, y;
	gctFLOAT dx, dy, ux, uy;
	gctFLOAT length, prevLength, firstLength;
	gctFLOAT dashLength;
	gctINT dashIndex;
	gctBOOL dashing;
	gctBOOL addEndCap;
	gctBOOL needToHandleSwing = gcvTRUE /* (StrokeConversion->strokeCapStyle == gcvCAP_BUTT) */;

	dashing = StrokeConversion->strokeDashPatternCount > 0 ? gcvTRUE : gcvFALSE;
	dashIndex = StrokeConversion->strokeDashInitialIndex;
	dashLength = StrokeConversion->strokeDashInitialLength;

	for (subPath = StrokeConversion->subPathList; subPath; subPath = subPath->next)
	{
		vgsSUBPATH_PTR firstStrokeSubPath = gcvNULL;
		vgsPOINT_PTR firstRightPoint = gcvNULL;
		vgsPOINT_PTR lastLeftPoint = gcvNULL;
		gctFLOAT firstDx = 0.0f, firstDy = 0.0f;
		gctBOOL drawing = gcvFALSE;
		gctFLOAT totalLength = 0.0f;
		gctFLOAT accuLength = 0.0f;
		gctUINT swingHandling = vgcSWING_NO;

		StrokeConversion->currentSubPath = subPath;

		/* TODO - Need to check/debug closed stroke path. */
		needToHandleSwing = (StrokeConversion->strokeCapStyle == gcvCAP_BUTT || subPath->closed);
		if (needToHandleSwing)
		{
			gctBOOL reallyNeedToHandleSwing = gcvFALSE;

			/* Calculate the total length. */
			for (point = subPath->pointList; point; point = point->next)
			{
				totalLength += point->length;
				if (point->flattenFlag != vgcFLATTEN_NO)
				{
					reallyNeedToHandleSwing = gcvTRUE;
				}
			}
			subPath->length = totalLength;
			if (reallyNeedToHandleSwing)
			{
				swingHandling = vgcSWING_OUT;
			}
			else
			{
				needToHandleSwing = gcvFALSE;
				swingHandling = vgcSWING_NO;
			}
		}
		StrokeConversion->swingNeedToHandle = needToHandleSwing;

		if (dashing && StrokeConversion->strokeDashPhaseReset)
		{
			dashIndex = StrokeConversion->strokeDashInitialIndex;
			dashLength = StrokeConversion->strokeDashInitialLength;
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
				gcmERROR_RETURN(_AddZeroLengthStrokeSubPath(StrokeConversion, point));
			}
			continue;
		}

		/* Adjust closed status for dashing. */
		if (dashing && subPath->closed && ((dashIndex & 0x1) == 1))
		{
			subPath->closed = gcvFALSE;
		}

		/* Set addEndCap. */
		addEndCap = subPath->closed ? gcvFALSE : gcvTRUE;

		/* Process first line. */
		firstLength = point->length;
		ux = point->tangentX;
		uy = point->tangentY;
		dx =  uy * halfWidth;
		dy = -ux * halfWidth;
		if (needToHandleSwing)
		{
			StrokeConversion->swingAccuLength = firstLength;
		}

		if (dashing)
		{
			gctFLOAT deltaLength;

			/* Draw dashes. */
			x = point->x;
			y = point->y;
			do
			{
				if ((dashIndex & 0x1) == 0)
				{
					gcmERROR_RETURN(_StartANewStrokeSubPath(
						StrokeConversion,
						x, y,
						dx, dy, addEndCap,
						&strokeSubPath
						));

					drawing = gcvTRUE;
					addEndCap = gcvTRUE;
					if (subPath->closed && (firstStrokeSubPath == gcvNULL))
					{
						firstStrokeSubPath = StrokeConversion->lastStrokeSubPath;
						firstRightPoint = StrokeConversion->lastRightStrokePoint;
						lastLeftPoint = StrokeConversion->leftStrokePoint;
						gcmASSERT(lastLeftPoint->next == gcvNULL);
						firstDx = dx;
						firstDy = dy;
					}
				}

				deltaLength = firstLength - dashLength;
				if (deltaLength >= FLOAT_EPSILON)
				{
					/* Move (x, y) forward along the line by dashLength. */
					x += ux * dashLength;
					y += uy * dashLength;

					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(drawing);
						gcmERROR_RETURN(_EndAStrokeSubPath(
							StrokeConversion,
							x, y,
							dx, dy
							));

						drawing = gcvFALSE;
					}

					_GetNextDashLength(StrokeConversion, &dashIndex, &dashLength);
					firstLength = deltaLength;
				}
				else if (deltaLength <= -FLOAT_EPSILON)
				{
					dashLength = -deltaLength;
					break;
				}
				else
				{
					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(drawing);
						gcmERROR_RETURN(_EndAStrokeSubPath(
							StrokeConversion,
							nextPoint->x, nextPoint->y,
							dx, dy
							));

						drawing = gcvFALSE;
					}

					_GetNextDashLength(StrokeConversion, &dashIndex, &dashLength);
					firstLength = 0;
					break;
				}
			}
			while (gcvTRUE);
		}
		else
		{
			gcmERROR_RETURN(_StartANewStrokeSubPath(
				StrokeConversion,
				point->x, point->y,
				dx, dy, addEndCap,
				&strokeSubPath
				));

			drawing = gcvTRUE;
			addEndCap = gcvTRUE;
		}

		/* Process the rest of lines. */
		prevLength = firstLength;
		for (point = nextPoint, nextPoint = point->next; nextPoint;
			 point = nextPoint, nextPoint = point->next)
		{
			if (!dashing || ((dashIndex & 0x1) == 0 && drawing))
			{
				/* Add points for end of line for line join process with next line. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
										point->x + dx, point->y + dy));
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion,
										point->x - dx, point->y - dy));
			}

			length = point->length;
			ux = point->tangentX;
			uy = point->tangentY;
			dx =  uy * halfWidth;
			dy = -ux * halfWidth;
			if (needToHandleSwing)
			{
				accuLength += point->prev->length;
				if (!dashing)
				{
					gcmASSERT(StrokeConversion->swingAccuLength == accuLength);
				}
				else
				{
					StrokeConversion->swingAccuLength = accuLength;
				}
				if (accuLength < halfWidth)
				{
					swingHandling = vgcSWING_OUT;
				}
				else if (totalLength - accuLength < halfWidth)
				{
					swingHandling = vgcSWING_IN;
				}
				else
				{
					swingHandling = vgcSWING_NO;
				}
			}

			if (!dashing)
			{
				/* Handle line joint style. */
				gcmERROR_RETURN(_ProcessLineJoint(
					StrokeConversion, point,
					length, prevLength, swingHandling,
					point->x + dx, point->y + dy,
					point->x - dx, point->y - dy
					));
			}
			else
			{
				gctFLOAT deltaLength;

				/* Draw dashes. */
				x = point->x;
				y = point->y;
				if ((dashIndex & 0x1) == 0)
				{
					if (drawing)
					{
						/* Handle line joint style. */
						gcmERROR_RETURN(_ProcessLineJoint(
							StrokeConversion, point,
							dashLength, prevLength, swingHandling,
							x + dx, y + dy,
							x - dx, y - dy
							));
					}
					else
					{
						/* Start a new sub path. */
						gcmASSERT(addEndCap);
						gcmERROR_RETURN(_StartANewStrokeSubPath(
							StrokeConversion,
							x, y,
							dx, dy, addEndCap,
							&strokeSubPath
							));

						drawing = gcvTRUE;
						addEndCap = gcvTRUE;
					}
				}
				do
				{
					deltaLength = length - dashLength;
					if (deltaLength >= FLOAT_EPSILON)
					{
						/* Move (x, y) forward along the line by dashLength. */
						x += ux * dashLength;
						y += uy * dashLength;

						if ((dashIndex & 0x1) == 0)
						{
							gcmASSERT(drawing);
							gcmERROR_RETURN(_EndAStrokeSubPath(
								StrokeConversion,
								x, y, dx, dy
								));

							drawing = gcvFALSE;
						}

						_GetNextDashLength(StrokeConversion, &dashIndex, &dashLength);
						length = deltaLength;
					}
					else if (deltaLength <= -FLOAT_EPSILON)
					{
						dashLength = -deltaLength;
						break;
					}
					else
					{
						if ((dashIndex & 0x1) == 0)
						{
							gcmASSERT(drawing);
							gcmERROR_RETURN(_EndAStrokeSubPath(
								StrokeConversion,
								nextPoint->x, nextPoint->y,
								dx, dy
								));

							drawing = gcvFALSE;
						}

						_GetNextDashLength(StrokeConversion, &dashIndex, &dashLength);
						length = 0;
						break;
					}

					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(addEndCap);
						gcmERROR_RETURN(_StartANewStrokeSubPath(
							StrokeConversion,
							x, y,
							dx, dy, addEndCap,
							&strokeSubPath
							));

						drawing = gcvTRUE;
						addEndCap = gcvTRUE;
					}
				}
				while (gcvTRUE);
			}

			prevLength = length;
		}

		if (needToHandleSwing)
		{
			accuLength += point->prev->length;
			if (!dashing)
			{
				gcmASSERT(StrokeConversion->swingAccuLength == accuLength);
			}
			else
			{
				StrokeConversion->swingAccuLength = accuLength;
			}
			if (accuLength < halfWidth)
			{
				swingHandling = vgcSWING_OUT;
			}
			else if (totalLength - accuLength < halfWidth)
			{
				swingHandling = vgcSWING_IN;
			}
			else
			{
				swingHandling = vgcSWING_NO;
			}
		}

		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			gcmASSERT(StrokeConversion->swingHandling == vgcSWING_NO);
			/* Draw the swing area (pie area). */
			gcmERROR_RETURN(_DrawSwingPieArea(StrokeConversion, subPath->lastPoint, gcvFALSE));
		}

		if (subPath->closed)
		{
			if (! dashing || drawing)
			{
				/* Add points for end of line. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail(StrokeConversion,
										point->x + dx, point->y + dy));
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead(StrokeConversion,
										point->x - dx, point->y - dy));

				if (! dashing)
				{
					/* Handle line joint style for the first/last point in closed path. */
					gcmERROR_RETURN(_CloseAStrokeSubPath(
						StrokeConversion, point,
						firstLength, prevLength, swingHandling,
						strokeSubPath->pointList, strokeSubPath->lastPoint
						));
				}
				else
				{
					/* Handle line joint style for the first/last point in closed path. */
					gcmERROR_RETURN(_CloseAStrokeSubPath(
						StrokeConversion, point,
						firstLength, prevLength, swingHandling,
						firstRightPoint, lastLeftPoint
						));
				}
			}
			else if (StrokeConversion->strokeCapStyle != gcvCAP_BUTT)
			{
				/* No closing join need.  Add end cap for the starting point. */

				if (StrokeConversion->strokeCapStyle == gcvCAP_SQUARE)
				{
					firstRightPoint->x += firstDy;
					firstRightPoint->y -= firstDx;
					lastLeftPoint->x += firstDy;
					lastLeftPoint->y -= firstDx;
				}
				else
				{
					vgsSUBPATH_PTR lastStrokeSubPath = StrokeConversion->lastStrokeSubPath;
					vgsPOINT_PTR startPoint = lastStrokeSubPath->pointList;
					vgsPOINT_PTR point;

					/* Add curve. */
					/* Add extra point to the beginning with end point's coordinates. */
					gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, &point));
					point->x = lastStrokeSubPath->lastPoint->x;
					point->y = lastStrokeSubPath->lastPoint->y;
					point->next = startPoint;
					startPoint->prev = point;
					startPoint->curveType = vgcCURVE_ARC_SCCW;
					startPoint->centerX = subPath->pointList->x;
					startPoint->centerY = subPath->pointList->y;
					lastStrokeSubPath->pointList = point;
				}
			}
		}
		else if (! dashing ||
				 (((dashIndex & 0x1) == 0) && (dashLength < StrokeConversion->strokeDashPattern[dashIndex])))
		{
			/* Add end cap if the subPath is not closed. */
			gcmERROR_RETURN(_EndAStrokeSubPath(
				StrokeConversion,
				point->x, point->y,
				dx, dy
				));

			drawing = gcvFALSE;
		}
	}

	return status;
}

#if OUTPUT_STROKE_PATH || OUTPUT_FLATTENED_PATH
static gceSTATUS
_OutputPath(
	vgsCONTEXT_PTR Context,
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsSUBPATH_PTR SubPathList
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gcoOS os = StrokeConversion->os;
	gctFILE logFile = gcvNULL;
	gctINT segmentCount = 0;
	gctINT pointCount = 0;
	gctFLOAT scale = 1.0f / StrokeConversion->pathScale;
	vgsSUBPATH_PTR subPath = gcvNULL;
	vgsPOINT_PTR point = gcvNULL;
	gctINT8 realBuffer[256];
	gctSTRING buffer = (gctSTRING) realBuffer;
	gctUINT offset = 0;

	/*gcmERROR_RETURN(gcoOS_Open(os, PATH_LOG_FILENAME, gcvFILE_APPENDTEXT, &logFile));*/
	gcmERROR_RETURN(gcoOS_Open(os, PATH_LOG_FILENAME, gcvFILE_CREATETEXT, &logFile));

	scale = 1.0f;

	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		segmentCount += subPath->pointCount + 1;
		pointCount += subPath->pointCount;
	}

#if OUTPUT_STROKE_PATH_INCLUDE_CMD
#define VGSL_WRITECOMMAND(COMMAND) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStr(buffer, &offset, "%s\t", COMMAND)); \
	gcoOS_Write(os, logFile, offset, buffer); \
	segmentCount--
#define VGSL_WRITECOMMAND1(COMMAND) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStr(buffer, &offset, "%s\n", COMMAND)); \
	gcoOS_Write(os, logFile, offset, buffer); \
	segmentCount--
#else
#define VGSL_WRITECOMMAND(COMMAND)		segmentCount--
#define VGSL_WRITECOMMAND1(COMMAND)		segmentCount--
#endif

#define VGSL_WRITEPOINT(POINT) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStr(buffer, &offset, "%12.6f\t%12.6f\n", \
				POINT->x * scale, \
				POINT->y * scale)); \
	gcoOS_Write(os, logFile, offset, buffer); \
	pointCount--

	/* Output path with reversed scale. */
	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		/* Create a MOVE_TO command for the first point. */
		point = subPath->pointList;
		gcmASSERT(point);
		VGSL_WRITECOMMAND("VG_MOVE_TO");
		VGSL_WRITEPOINT(point);

		/* Create a LINE_TO command for the rest of points. */
		for (point = point->next; point; point = point->next)
		{
			VGSL_WRITECOMMAND("VG_LINE_TO");
			VGSL_WRITEPOINT(point);
		}

		/* Create a CLOSE_PATH command at the end. */
		VGSL_WRITECOMMAND1("VG_CLOSE_PATH");
	}
	gcmASSERT(segmentCount == 0);
	gcmASSERT(pointCount == 0);

	gcmVERIFY_OK(gcoOS_Close(os, logFile));

	return status;
}
#endif

/******************************************************************************\
|************************* Fixed Point Implementation *************************|
|******************************************************************************/
typedef gceSTATUS (* vgSL_ValueGetter_I) (
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	);

static gceSTATUS _GetS8_I(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT8 *) Data);

	x *= PathScale;		/* Includes 16 bits shift. */
	x += PathBias;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS16_I(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT16 *) Data);

	x *= PathScale;	/* Includes 16 bits shift. */
	x += PathBias;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS32_I(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT *) Data);

	if (x > FIXED_MAX || x < FIXED_MIN)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}

	x *= PathScale;	/* Includes 16 bits shift. */
	x += PathBias;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetF_I(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFLOAT x0 = *((gctFLOAT *) Data);
	gctINT64 x = (gctINT64) (x0 * PathScale);	/* Includes 16 bits shift. */

	x += PathBias;

	if (x > FIXED_MAX || x < FIXED_MIN)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}
	if (x != 0 && x < FIXED_EPSILON && x > -FIXED_EPSILON)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}

	*X = (gctFIXED) x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS8_I_NB(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT8 *) Data);

	x *= PathScale;		/* Includes 16 bits shift. */
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS16_I_NB(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT16 *) Data);

	x *= PathScale;	/* Includes 16 bits shift. */
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS32_I_NB(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT *) Data);

	if (x > FIXED_MAX || x < FIXED_MIN)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}

	x *= PathScale;	/* Includes 16 bits shift. */
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetF_I_NB(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFLOAT x0 = *((gctFLOAT *) Data);
	gctINT64 x = (gctINT64) (x0 * PathScale);	/* Includes 16 bits shift. */

	if (x > FIXED_MAX || x < FIXED_MIN)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}
	if (x != 0 && x < FIXED_EPSILON && x > -FIXED_EPSILON)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}

	*X = (gctFIXED) x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS8_I_NS(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT8 *) Data);

	x <<= FIXED_FRACTION_BITS;
	x += PathBias;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS16_I_NS(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT16 *) Data);

	x <<= FIXED_FRACTION_BITS;
	x += PathBias;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS32_I_NS(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT *) Data);

	if (x > FIXED_MAX || x < FIXED_MIN)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}

	x <<= FIXED_FRACTION_BITS;
	x += PathBias;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetF_I_NS(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFLOAT x0 = *((gctFLOAT *) Data);
	gctINT64 x = (gctINT64) (x0 * FIXED_ONE);

	x += PathBias;

	if (x > FIXED_MAX || x < FIXED_MIN)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}
	if (x != 0 && x < FIXED_EPSILON && x > -FIXED_EPSILON)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}

	*X = (gctFIXED) x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS8_I_NS_NB(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT8 *) Data);

	x <<= FIXED_FRACTION_BITS;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS16_I_NS_NB(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT16 *) Data);

	x <<= FIXED_FRACTION_BITS;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetS32_I_NS_NB(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFIXED x = (gctFIXED) *((gctINT *) Data);

	if (x > FIXED_MAX || x < FIXED_MIN)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}

	x <<= FIXED_FRACTION_BITS;
	*X = x;

	return gcvSTATUS_OK;
}

static gceSTATUS _GetF_I_NS_NB(
	gctFIXED * X,
	gctUINT8 * Data,
	gctFIXED PathScale,
	gctFIXED PathBias
	)
{
	gctFLOAT x0 = *((gctFLOAT *) Data);
	gctINT64 x = (gctINT64) (x0 * FIXED_ONE);

	if (x > FIXED_MAX || x < FIXED_MIN)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}
	if (x != 0 && x < FIXED_EPSILON && x > -FIXED_EPSILON)
	{
		return gcvSTATUS_NOT_SUPPORTED;
	}

	*X = (gctFIXED) x;

	return gcvSTATUS_OK;
}

#define VGSL_GETVALUE_I(X) \
	status = getValue(&(X), dataPointer, pathScale, pathBias); \
	if (status == gcvSTATUS_NOT_SUPPORTED) goto needToUseFloat; \
	dataPointer += dataTypeSize; \
	size -= dataTypeSize

#define VGSL_GETCOORDXY_I(X, Y) \
	VGSL_GETVALUE_I(X); \
	VGSL_GETVALUE_I(Y); \
	if (isRelative) { X += ox; Y += oy; }


static void
_SetPointTangent_I(
	vgsPOINT_I_PTR Point,
	gctFIXED Dx,
	gctFIXED Dy
	)
{
	if (Dx == FIXED_ZERO)
	{
		if (Dy == FIXED_ZERO)
		{
			if (Point->prev)
			{
				Point->length = FIXED_ZERO;
				Point->tangentX = Point->prev->tangentX;
				Point->tangentY = Point->prev->tangentY;
			}
			else
			{
				gcmASSERT(Point->prev);
				Point->length = FIXED_ZERO;
				Point->tangentX = FIXED_ZERO;
				Point->tangentY = FIXED_ZERO;
			}
		}
		else
		{
			Point->tangentX = FIXED_ZERO;
			if (Dy > FIXED_ZERO)
			{
				Point->length = Dy;
				Point->tangentY = FIXED_ONE;
			}
			else
			{
				Point->length = -Dy;
				Point->tangentY = -FIXED_ONE;
			}
		}
	}
	else if (Dy == FIXED_ZERO)
	{
		Point->tangentY = FIXED_ZERO;
		if (Dx > FIXED_ZERO)
		{
			Point->length = Dx;
			Point->tangentX = FIXED_ONE;
		}
		else
		{
			Point->length = -Dx;
			Point->tangentX = -FIXED_ONE;
		}
	}
	else
	{
		gctFIXED l, tx, ty;

#if USE_LOCAL_SQRT
		gctFIXED dx, dy;
		gctFIXED t, t2;

		dx = (Dx >= FIXED_ZERO ? Dx : -Dx);
		dy = (Dy >= FIXED_ZERO ? Dy : -Dy);
		if (dx > dy)
		{
			t = FIXED_DIV(dy, dx);
			t2 = (gctFIXED) (((gctUINT) t * (gctUINT) t) >> 16);
			l = _Sqrt_I(t2);
			Point->length = FIXED_MUL(l, dx);

			tx = FIXED_DIV(FIXED_ONE, l);
			ty = (gctFIXED) (((gctUINT) tx * (gctUINT) t) >> 16);
		}
		else if (dx < dy)
		{
			t = FIXED_DIV(dx, dy);
			t2 = (gctFIXED) (((gctUINT) t * (gctUINT) t) >> 16);
			l = _Sqrt_I(t2);
			Point->length = FIXED_MUL(l, dy);

			ty = FIXED_DIV(FIXED_ONE, l);
			tx = (gctFIXED) (((gctUINT) ty * (gctUINT) t) >> 16);
		}
		else
		{
			tx = 0x0000b505;
			ty = 0x0000b505;
			Point->length = FIXED_MUL(dx, 0x00016a0a);
		}
		if (Dx < FIXED_ZERO) tx = -tx;
		if (Dy < FIXED_ZERO) ty = -ty;
#else
		l = FIXED_MUL(Dx, Dx) + FIXED_MUL(Dy, Dy);
		gcmASSERT(l != FIXED_ZERO);
		l = gcmSQRTF(l);
		Point->length = l;

		l = FIXED_DIV(FIXED_ONE, l);
		tx = FIXED_MUL(Dx, l);
		ty = FIXED_MUL(Dy, l);
#endif
		tx = gcmCLAMP(tx, -FIXED_ONE, FIXED_ONE);
		ty = gcmCLAMP(ty, -FIXED_ONE, FIXED_ONE);
		Point->tangentX = tx;
		Point->tangentY = ty;
	}
}

static gceSTATUS
_AddPointToSubPathWDelta_I(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFIXED X,
	gctFIXED Y,
	gctFIXED DX,
	gctFIXED DY,
	vgsSUBPATH_I_PTR SubPath,
	gctUINT FlattenFlag
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsPOINT_I_PTR lastPoint = SubPath->lastPoint;
	vgsPOINT_I_PTR point;

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));

	point->x = X;
	point->y = Y;
	point->flattenFlag = FlattenFlag;

	/* Calculate tangent for lastPoint. */
	gcmASSERT(lastPoint);
	_SetPointTangent_I(lastPoint, DX, DY);

	lastPoint->next = point;
	SubPath->lastPoint = point;
	point->prev = lastPoint;
	SubPath->pointCount++;

	return status;
}

static gceSTATUS
_AddPointToSubPath_I(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFIXED X,
	gctFIXED Y,
	vgsSUBPATH_I_PTR SubPath,
	gctUINT FlattenFlag
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsPOINT_I_PTR lastPoint = SubPath->lastPoint;
	vgsPOINT_I_PTR point;

	if (lastPoint == gcvNULL)
	{
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));

		gcmASSERT(FlattenFlag == vgcFLATTEN_NO);
		point->x = X;
		point->y = Y;
		point->flattenFlag = FlattenFlag;
		SubPath->lastPoint = SubPath->pointList = point;
		SubPath->pointCount++;

		return gcvSTATUS_OK;
	}
	else
	{
		gctFIXED dX = X - lastPoint->x;
		gctFIXED dY = Y - lastPoint->y;
		gctFIXED deltaX = (dX >= FIXED_ZERO ? dX : -dX);
		gctFIXED deltaY = (dY >= FIXED_ZERO ? dY : -dY);

		/* Check for degenerated line. */
		if (deltaX == FIXED_ZERO && deltaY == FIXED_ZERO)
		{
			/* Skip degenerated line. */
			return gcvSTATUS_OK;
		}
		if (deltaX < FIXED_EPSILON && deltaY < FIXED_EPSILON)
		{
			/*gcmASSERT(deltaX >= FIXED_EPSILON || deltaY >= FIXED_EPSILON);*/
			/* Skip degenerated line. */
			return gcvSTATUS_OK;
		}

		return _AddPointToSubPathWDelta_I(StrokeConversion, X, Y, dX, dY, SubPath, FlattenFlag);
	}
}

#if gcvUSE_FAST_STROKE
static gceSTATUS
_FastAddPointToSubPathWDelta_I(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFIXED X,
	gctFIXED Y,
	gctFIXED DX,
	gctFIXED DY,
	vgsSUBPATH_I_PTR SubPath
	)
{
	gceSTATUS status;
	vgsPOINT_I_PTR lastPoint;
	vgsPOINT_I_PTR point;

	/* Check for degenerated line. */
	if (DX < FAST_FIXED_EPSILON && DX > -FAST_FIXED_EPSILON
	&&	DY < FAST_FIXED_EPSILON && DY > -FAST_FIXED_EPSILON)
	{
		/* Skip degenerated line. */
		return gcvSTATUS_OK;
	}

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));

	point->x = X;
	point->y = Y;

	lastPoint = SubPath->lastPoint;
	gcmASSERT(lastPoint);
	lastPoint->next = point;
	SubPath->lastPoint = point;
	point->prev = lastPoint;
	SubPath->pointCount++;

	return gcvSTATUS_OK;
}

static gceSTATUS
_FastAddPointToSubPath_I(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFIXED X,
	gctFIXED Y,
	vgsSUBPATH_I_PTR SubPath
	)
{
	vgsPOINT_I_PTR lastPoint = SubPath->lastPoint;

	if (lastPoint == gcvNULL)
	{
		gceSTATUS status;
		vgsPOINT_I_PTR point;

		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));

		point->x = X;
		point->y = Y;
		SubPath->lastPoint = SubPath->pointList = point;
		SubPath->pointCount++;

		return gcvSTATUS_OK;
	}
	else
	{
		return _FastAddPointToSubPathWDelta_I(StrokeConversion,
			X, Y, X - lastPoint->x, Y - lastPoint->y, SubPath);
	}
}
#endif

static gceSTATUS
_FlattenQuadBezier_I(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFIXED X0,
	gctFIXED Y0,
	gctFIXED X1,
	gctFIXED Y1,
	gctFIXED X2,
	gctFIXED Y2,
	vgsSUBPATH_I_PTR SubPath
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gctINT n;
	vgsPOINT_I_PTR point0, point1;
	gctFIXED x, y;
	gctFIXED a1x, a1y, a2x, a2y;
	gctINT nShift;
	gctFIXED dx1 = X1 - X0;
	gctFIXED dy1 = Y1 - Y0;
	gctFIXED dx2 = X2 - X1;
	gctFIXED dy2 = Y2 - Y1;
	gctFIXED deltax1 = (dx1 >= FIXED_ZERO ? dx1 : -dx1);
	gctFIXED deltay1 = (dy1 >= FIXED_ZERO ? dy1 : -dy1);
	gctFIXED deltax2 = (dx2 >= FIXED_ZERO ? dx2 : -dx2);
	gctFIXED deltay2 = (dy2 >= FIXED_ZERO ? dy2 : -dy2);
	gctFIXED delta1 = deltax1 + deltay1;
	gctFIXED delta2 = deltax2 + deltay2;

	if (delta1 == FIXED_ZERO)
	{
		if (delta2 == FIXED_ZERO)
		{
			/* Degenerated Bezier curve.  Becomes a point. */
			/* Discard zero-length segments. */
			return gcvSTATUS_OK;
		}
		else
		{
			/* Degenerated Bezier curve.  Becomes a line. */
			/* Add a point to subpath. */
			return(_AddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath, vgcFLATTEN_NO));
		}
	}
	else if (delta2 == FIXED_ZERO)
	{
		/* Degenerated Bezier curve.  Becomes a line. */
		/* Add a point to subpath. */
			return(_AddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath, vgcFLATTEN_NO));
	}
	else if (deltax1 != FIXED_ZERO && deltax1 < FIXED_MIN_BEZIER_LENGTH
		  && deltay1 != FIXED_ZERO && deltay1 < FIXED_MIN_BEZIER_LENGTH)
	{
		StrokeConversion->useFixedPoint = gcvFALSE;
		return gcvSTATUS_OK;
	}
	else if (deltax2 != FIXED_ZERO && deltax2 < FIXED_MIN_BEZIER_LENGTH
		  && deltay2 != FIXED_ZERO && deltay2 < FIXED_MIN_BEZIER_LENGTH)
	{
		StrokeConversion->useFixedPoint = gcvFALSE;
		return gcvSTATUS_OK;
	}

	VGSL_STAT_COUNTER_INCREASE(vgStrokeCurveCount);

	/* Formula.
	 * f(t) = (1 - t)^2 * p0 + 2 * t * (1 - t) * p1 + t^2 * p2
	 *      = a0 + a1 * t + a2 * t^2
	 *   a0 = p0
	 *   a1 = 2 * (-p0 + p1)
	 *   a2 = p0 - 2 * p1 + p2
	 */
	a1x = dx1 + dx1;
	a1y = dy1 + dy1;
	a2x = dx2 - dx1;
	a2y = dy2 - dy1;

	/* Step 1: Calculate N. */
#if !gcvSEGMENT_COUNT
	{
		/* Lefan's method. */
		/* dist(t) = ...
		 * t2 = ...
		 * if 0 <= t2 <= 1
		 *    upperBound = dist(t2)
		 * else
		 *    upperBound = max(dist(0), dist(1))
		 * N = ceil(sqrt(upperbound / epsilon / 8))
		 */
		/* Prepare dist(t). */
		gctINT64 f1, f2, t1, t2, upperBound;

		f1 = (gctINT64) a1x * (gctINT64) a2y - (gctINT64) a2x * (gctINT64) a1y;
		if (f1 != 0)
		{
#ifdef UNDER_CE
			double tempDouble = 0.0;
#endif
			if (f1 < 0) f1 = -f1;

			/* Calculate t2. */
			t1 = (gctINT64) a2x * (gctINT64) a2x + (gctINT64) a2y * (gctINT64) a2y;
			t2 = -((gctINT64) dx1 * (gctINT64) a2x + (gctINT64) dy1 * (gctINT64) a2y);
			t2 = (t2 << 16) / t1;
			/* Calculate upperBound. */
			if (t2 >= 0 && t2 <= FIXED_ONE)
			{
				f2 = (gctINT64) dx1 + (((gctINT64) a2x * t2) >> 16);
				f2 *= f2;
				t1 = (gctINT64) dy1 + (((gctINT64) a2y * t2) >> 16);
				t1 *= t1;
				upperBound = t1 + f2;
			}
			else
			{
				t1 = (gctINT64) dx1 * (gctINT64) dx1 + (gctINT64) dy1 * (gctINT64) dy1;
				t2 = (gctINT64) dx2 * (gctINT64) dx2 + (gctINT64) dy2 * (gctINT64) dy2;
				upperBound = t1 < t2 ? t1 : t2;
			}
			/* Calculate n. */
#ifdef UNDER_CE
			tempDouble = (double)upperBound;
			upperBound = f1 / (gctINT64) sqrt(tempDouble);
#else
			upperBound = f1 / (gctINT64) sqrt((double) upperBound);
#endif
#if !vgvSTROKE_COPY_OPTIMIZE || ! gcvUSE_INTERNAL_SCALE
			if (StrokeConversion->largeTransformScale >= 2.0f)
			{
				upperBound += upperBound;
			}
#endif
			if (StrokeConversion->strokeLineWidth > 1.414f)
			{
#if vgvSTROKE_COPY_OPTIMIZE && gcvUSE_INTERNAL_SCALE
				gctFLOAT one = (gctFLOAT) (1 << (FIXED_FRACTION_BITS - StrokeConversion->internalShiftBits));
				gctFIXED width = (gctFIXED) (StrokeConversion->strokeLineWidth * one);
#else
				gctFIXED width = FLOAT_TO_FIXED(StrokeConversion->strokeLineWidth);
#endif
				upperBound = (upperBound * width) >> 16;
				upperBound = (upperBound * width) >> 16;
			}
#if gcvSTROKE_REDUCE_SEGMENT
			n = (gctINT) (upperBound >> 18);
#else
			n = (gctINT) (upperBound >> 16);
#endif

			if (n > 256)
			{
				/*if (n > 1024)
				{
					if (n > 4096)
					{
						StrokeConversion->useFixedPoint = gcvFALSE;
						return gcvSTATUS_OK;
					}
					else			{ n = 64;	nShift = 6; }
				}
				else*/				{ n = 32;	nShift = 5; }
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
			/* n = 0 => n = 64. */
			n = 64;	nShift = 6;
		}
	}
#else
	n = gcvSEGMENT_COUNT;
	if (n > 16)
	{
		if (n > 64)
		{
			if (n > 128)	{ n = 256;	nShift = 8; }
			else			{ n = 128;	nShift = 7; }
		}
		else
		{
			if (n > 32)		{ n = 64;	nShift = 6; }
			else			{ n = 32;	nShift = 5; }
		}
	}
	else
	{
		if (n > 4)
		{
			if (n > 8)		{ n = 16;	nShift = 4; }
			else			{ n = 8;	nShift = 3; }
		}
		else
		{
			if (n > 2)		{ n = 4;	nShift = 2; }
			else if (n > 1)	{ n = 2;	nShift = 1; }
			else			{ n = 1;	nShift = 0; }
		}
	}
#endif

	VGSL_STAT_COUNTER_ADD(vgStrokeFlattenedSegmentCount, n);

#if gcvQUAD_STATISTICS
	if (n < g_minQuadSegments)
	{
		g_minQuadSegments = n;
	}

	if (n > g_maxQuadSegments)
	{
		g_maxQuadSegments = n;
	}

	if (g_avrQuadSegments == ~0)
	{
		g_avrQuadSegments = n;
	}
	else
	{
		g_avrQuadSegments = (g_avrQuadSegments + n) / 2;
	}
#endif

	/* Add extra P0 for incoming tangent. */
	point0 = SubPath->lastPoint;
	gcmASSERT(point0);
	/* First add P1 to calculate incoming tangent, which is saved in P0. */
	gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, X1, Y1, SubPath, vgcFLATTEN_START));
	point1 = SubPath->lastPoint;
	/* Change the point1's coordinates back to P0. */
	point1->x = X0;
	point1->y = Y0;
	point0->length = FIXED_ZERO;

	if (n == 2)
	{
		/* Add middle point. */
		x = (X0 + X1 + X1 + X2) >> 2;
		y = (Y0 + Y1 + Y1 + Y2) >> 2;
		gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, x, y, SubPath, vgcFLATTEN_MIDDLE));
	}
	else if (n > 1)
	{
		gctINT dShift, dsquareShift;
		gctINT64 dx, dy, ddx, ddy;
		gctINT64 x, y;
		gctINT i;

		/* Step 2: Calculate deltas. */
		/*   Df(t) = f(t + d) - f(t)
		 *         = a1 * d + a2 * d^2 + 2 * a2 * d * t
		 *  DDf(t) = Df(t + d) - Df(t)
		 *         = 2 * a2 * d^2
		 *    f(0) = a0
		 *   Df(0) = a1 * d + a2 * d^2
		 *  DDf(0) = 2 * a2 * d^2
		 */
		dShift       = nShift;
		dsquareShift = dShift << 1;
		ddx	= (((gctINT64) a2x) << 16) >> dsquareShift;
		ddy = (((gctINT64) a2y) << 16) >> dsquareShift;
		dx	= ((((gctINT64) a1x) << 16) >> dShift) + ddx;
		dy	= ((((gctINT64) a1y) << 16) >> dShift) + ddy;
		ddx	+= ddx;
		ddy += ddy;

		/* Step 3: Add points. */
		/*gcmASSERT((dx < -(FIXED_EPSILON << 16) || dx > (FIXED_EPSILON << 16) || dx == FIXED_ZERO) &&
				  (dy < -(FIXED_EPSILON << 16) || dy > (FIXED_EPSILON << 16) || dy == FIXED_ZERO));*/
		x = ((gctINT64) X0) << 16;
		y = ((gctINT64) Y0) << 16;
		for (i = 1; i < n; i++)
		{
			x += dx;
			y += dy;

			/* Add a point to subpath. */
			gcmERROR_RETURN(_AddPointToSubPathWDelta_I(StrokeConversion,
				(gctFIXED) (x >> 16), (gctFIXED) (y >> 16),
				(gctFIXED) (dx >> 16), (gctFIXED) (dy >> 16),
				SubPath, vgcFLATTEN_MIDDLE));

			dx += ddx;
			dy += ddy;
		}
	}

	/* Add point 2 separately to avoid cumulative errors. */
	gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath, vgcFLATTEN_END));

	/* Add extra P2 for outgoing tangent. */
	/* First change P2(point0)'s coordinates to P1. */
	point0 = SubPath->lastPoint;
	point0->x = X1;
	point0->y = Y1;

	/* Add P2 to calculate outgoing tangent. */
	gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath, vgcFLATTEN_NO));
	point1 = SubPath->lastPoint;

	/* Change point0's coordinates back to P2. */
	point0->x = X2;
	point0->y = Y2;
	point0->length = FIXED_ZERO;

	return gcvSTATUS_OK;
}

gceSTATUS
_FlattenCubicBezier_I(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFIXED X0,
	gctFIXED Y0,
	gctFIXED X1,
	gctFIXED Y1,
	gctFIXED X2,
	gctFIXED Y2,
	gctFIXED X3,
	gctFIXED Y3,
	vgsSUBPATH_I_PTR SubPath
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gctINT n;
	vgsPOINT_I_PTR point0, point1;
	gctFIXED a1x, a1y, a2x, a2y, a3x, a3y;
	gctINT nShift;
	gctFIXED dx1 = X1 - X0;
	gctFIXED dy1 = Y1 - Y0;
	gctFIXED dx2 = X2 - X1;
	gctFIXED dy2 = Y2 - Y1;
	gctFIXED dx3 = X3 - X2;
	gctFIXED dy3 = Y3 - Y2;
	gctFIXED deltax1 = (dx1 >= FIXED_ZERO ? dx1 : -dx1);
	gctFIXED deltay1 = (dy1 >= FIXED_ZERO ? dy1 : -dy1);
	gctFIXED deltax2 = (dx2 >= FIXED_ZERO ? dx2 : -dx2);
	gctFIXED deltay2 = (dy2 >= FIXED_ZERO ? dy2 : -dy2);
	gctFIXED deltax3 = (dx3 >= FIXED_ZERO ? dx3 : -dx3);
	gctFIXED deltay3 = (dy3 >= FIXED_ZERO ? dy3 : -dy3);

	if (deltax1 + deltay1 + deltax2 + deltay2 + deltax3 + deltay3 == FIXED_ZERO)
	{
		/* Degenerated Bezier curve.  Becomes a point. */
		/* Discard zero-length segments. */
		return gcvSTATUS_OK;
	}
	else if (deltax1 != FIXED_ZERO && deltax1 < FIXED_MIN_BEZIER_LENGTH
		  && deltay1 != FIXED_ZERO && deltay1 < FIXED_MIN_BEZIER_LENGTH)
	{
		StrokeConversion->useFixedPoint = gcvFALSE;
		return gcvSTATUS_OK;
	}
	else if (deltax2 != FIXED_ZERO && deltax2 < FIXED_MIN_BEZIER_LENGTH
		  && deltay2 != FIXED_ZERO && deltay2 < FIXED_MIN_BEZIER_LENGTH)
	{
		StrokeConversion->useFixedPoint = gcvFALSE;
		return gcvSTATUS_OK;
	}
	else if (deltax3 != FIXED_ZERO && deltax3 < FIXED_MIN_BEZIER_LENGTH
		  && deltay3 != FIXED_ZERO && deltay3 < FIXED_MIN_BEZIER_LENGTH)
	{
		StrokeConversion->useFixedPoint = gcvFALSE;
		return gcvSTATUS_OK;
	}

	VGSL_STAT_COUNTER_INCREASE(vgStrokeCurveCount);

	/* Formula.
	 * f(t) = (1 - t)^3 * p0 + 3 * t * (1 - t)^2 * p1 + 3 * t^2 * (1 - t) * p2 + t^3 * p3
	 *      = a0 + a1 * t + a2 * t^2 + a3 * t^3
	 */
	a1x  = dx1 + dx1 + dx1;
	a1y  = dy1 + dy1 + dy1;
	a2x  = dx2 - dx1;
	a2x *= 3;
	a2y  = dy2 - dy1;
	a2y *= 3;
	a3x  = (X3 - X0) - dx2 - dx2 - dx2;
	a3y  = (Y3 - Y0) - dy2 - dy2 - dy2;

	/* Step 1: Calculate N. */
#if !gcvSEGMENT_COUNT
	{
		/* Lefan's method. */
		/*  df(t)/dt  = a1 + 2 * a2 * t + 3 * a3 * t^2
		 * d2f(t)/dt2 = 2 * a2 + 6 * a3 * t
		 * N = ceil(sqrt(max(ddfx(0)^2 + ddfy(0)^2, ddfx(1)^2 + ddyf(1)^2) / epsilon / 8))
		 */
		gctINT64 ddf0, ddf1, upperBound;
		gctINT64 t1, t2;
#ifdef UNDER_CE
		double tempDouble = 0.0;
#endif

		ddf0 = (gctINT64) a2x * (gctINT64) a2x + (gctINT64) a2y * (gctINT64) a2y;
		t1 = (gctINT64) (a2x + a3x + a3x + a3x);
		t2 = (gctINT64) (a2y + a3y + a3y + a3y);
		ddf1 = t1 * t1 + t2 * t2;
		upperBound = ddf0 > ddf1 ? ddf0: ddf1;
#ifdef UNDER_CE
		tempDouble = (double)upperBound;
		upperBound = (gctINT64)sqrt(tempDouble);
#else
		upperBound = (gctINT64) sqrt((double)upperBound);
#endif
#if !vgvSTROKE_COPY_OPTIMIZE || ! gcvUSE_INTERNAL_SCALE
		if (StrokeConversion->largeTransformScale >= 2.0f)
		{
			upperBound += upperBound;
		}
#endif
		if (StrokeConversion->strokeLineWidth > 1.414f)
		{
#if vgvSTROKE_COPY_OPTIMIZE && gcvUSE_INTERNAL_SCALE
			gctFLOAT one = (gctFLOAT) (1 << (FIXED_FRACTION_BITS - StrokeConversion->internalShiftBits));
			gctFIXED width = (gctFIXED) (StrokeConversion->strokeLineWidth * one);
#else
			gctFIXED width = FLOAT_TO_FIXED(StrokeConversion->strokeLineWidth);
#endif
			upperBound = (upperBound * width) >> 16;
			upperBound = (upperBound * width) >> 16;
		}
#if gcvSTROKE_REDUCE_SEGMENT
		n = (gctINT) (upperBound >> 20);
#else
		n = (gctINT) (upperBound >> 18);
#endif

		if (n > 256)
		{
			if (n > 1024)
			{
				/*if (n > 4096)
				{
					StrokeConversion->useFixedPoint = gcvFALSE;
					return gcvSTATUS_OK;
				}
				else*/			{ n = 64;	nShift = 6; }
			}
			else				{ n = 32;	nShift = 5; }
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
#else
	n = gcvSEGMENT_COUNT;
	if (n > 16)
	{
		if (n > 64)
		{
			if (n > 128)	{ n = 256;	nShift = 8; }
			else			{ n = 128;	nShift = 7; }
		}
		else
		{
			if (n > 32)		{ n = 64;	nShift = 6; }
			else			{ n = 32;	nShift = 5; }
		}
	}
	else
	{
		if (n > 4)
		{
			if (n > 8)		{ n = 16;	nShift = 4; }
			else			{ n = 8;	nShift = 3; }
		}
		else
		{
			if (n > 2)		{ n = 4;	nShift = 2; }
			else if (n > 1)	{ n = 2;	nShift = 1; }
			else			{ n = 1;	nShift = 0; }
		}
	}
#endif

	VGSL_STAT_COUNTER_ADD(vgStrokeFlattenedSegmentCount, n);

#if gcvCUBIC_STATISTICS
	if (n < g_minCubicSegments)
	{
		g_minCubicSegments = n;
	}

	if (n > g_maxCubicSegments)
	{
		g_maxCubicSegments = n;
	}

	if (g_avrCubicSegments == ~0)
	{
		g_avrCubicSegments = n;
	}
	else
	{
		g_avrCubicSegments = (g_avrCubicSegments + n) / 2;
	}
#endif

	/* Add extra P0 for incoming tangent. */
	point0 = SubPath->lastPoint;
	gcmASSERT(point0);
	/* First add P1/P2/P3 to calculate incoming tangent, which is saved in P0. */
	if (X0 != X1 || Y0 != Y1)
	{
		gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, X1, Y1, SubPath, vgcFLATTEN_START));
	}
	else if (X0 != X2 || Y0 != Y2)
	{
		gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath, vgcFLATTEN_START));
	}
	else
	{
		gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, X3, Y3, SubPath, vgcFLATTEN_START));
	}
	point1 = SubPath->lastPoint;
	/* Change the point1's coordinates back to P0. */
	point1->x = X0;
	point1->y = Y0;
	point0->length = FIXED_ZERO;

	if (n == 2)
	{
		gctFIXED x, y;

		/* Add middle point. */
		x = X1 + X2;
		x = x + x + x;
		x = (x + X0 + X3) >> 3;
		y = Y1 + Y2;
		y = y + y + y;
		y = (y + Y0 + Y3) >> 3;
		gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, x, y, SubPath, vgcFLATTEN_MIDDLE));
	}
	else if (n > 1)
	{
		gctINT dShift, dsquareShift, dcubeShift;
		gctINT64 dx, dy, ddx, ddy, dddx, dddy;
		gctINT64 x, y;
		gctINT i;

		/* Step 2: Calculate deltas */
		/*   Df(t) = f(t + d) - f(t)
		 *  DDf(t) = Df(t + d) - Df(t)
		 * DDDf(t) = DDf(t + d) - DDf(t)
		 *    f(0) = a0
		 *   Df(0) = a1 * d + a2 * d^2 + a3 * d^3
		 *  DDf(0) = 2 * a2 * d^2 + 6 * a3 * d^3
		 * DDDf(0) = 6 * a3 * d^3
		 */
		dShift			= nShift;
		dsquareShift	= dShift << 1;
		dcubeShift		= dShift + dsquareShift;
		ddx   = (((gctINT64) a2x) << 16) >> dsquareShift;
		ddy   = (((gctINT64) a2y) << 16) >> dsquareShift;
		dddx  = (((gctINT64) a3x) << 16) >> dcubeShift;
		dddy  = (((gctINT64) a3y) << 16) >> dcubeShift;
		dx    = ((((gctINT64) a1x) << 16) >> dShift) + ddx + dddx;
		dy    = ((((gctINT64) a1y) << 16) >> dShift) + ddy + dddy;
		ddx  += ddx;
		ddy  += ddy;
		dddx *= 6;
		dddy *= 6;
		ddx  += dddx;
		ddy  += dddy;

		/* Step 3: Add points. */
		/*gcmASSERT((dx < -(FIXED_EPSILON << 16) || dx > (FIXED_EPSILON << 16) || dx == FIXED_ZERO) &&
				  (dy < -(FIXED_EPSILON << 16) || dy > (FIXED_EPSILON << 16) || dy == FIXED_ZERO));*/
		x = ((gctINT64) X0) << 16;
		y = ((gctINT64) Y0) << 16;
		for (i = 1; i < n; i++)
		{
			x += dx;
			y += dy;

			/* Add a point to subpath. */
			gcmERROR_RETURN(_AddPointToSubPathWDelta_I(StrokeConversion,
				(gctFIXED) (x >> 16), (gctFIXED) (y >> 16),
				(gctFIXED) (dx >> 16), (gctFIXED) (dy >> 16),
				SubPath, vgcFLATTEN_MIDDLE));

			dx += ddx; ddx += dddx;
			dy += ddy; ddy += dddy;
		}
	}

	/* Add point 3 separately to avoid cumulative errors. */
	gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, X3, Y3, SubPath, vgcFLATTEN_END));

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

	/* Add P3 to calculate outgoing tangent. */
	gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, X3, Y3, SubPath, vgcFLATTEN_NO));
	point1 = SubPath->lastPoint;

	/* Change point0's coordinates back to P3. */
	point0->x = X3;
	point0->y = Y3;
	point0->length = FIXED_ZERO;

	return gcvSTATUS_OK;
}

#if gcvUSE_FAST_STROKE
static gceSTATUS
_FastFlattenQuadBezier_I(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFIXED X0,
	gctFIXED Y0,
	gctFIXED X1,
	gctFIXED Y1,
	gctFIXED X2,
	gctFIXED Y2,
	vgsSUBPATH_I_PTR SubPath
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gctINT n;
	gctFIXED a1x, a1y, a2x, a2y;
	gctINT nShift;
	gctFIXED dx1 = X1 - X0;
	gctFIXED dy1 = Y1 - Y0;
	gctFIXED dx2 = X2 - X1;
	gctFIXED dy2 = Y2 - Y1;
	gctFIXED delta1 = (dx1 >= 0 ? dx1 : -dx1) + (dy1 >= 0 ? dy1 : -dy1);
	gctFIXED delta2 = (dx2 >= 0 ? dx2 : -dx2) + (dy2 >= 0 ? dy2 : -dy2);

	VGSL_STAT_COUNTER_INCREASE(vgStrokeCurveCount);

	if (delta1 == FIXED_ZERO)
	{
		if (delta2 == FIXED_ZERO)
		{
			/* Degenerated Bezier curve.  Becomes a point. */
			/* Discard zero-length segments. */
			return gcvSTATUS_OK;
		}
		else
		{
			/* Degenerated Bezier curve.  Becomes a line. */
			/* Add a point to subpath. */
			return(_FastAddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath));
		}
	}
	else if (delta2 == FIXED_ZERO)
	{
		/* Degenerated Bezier curve.  Becomes a line. */
		/* Add a point to subpath. */
		return(_FastAddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath));
	}
	else
	{
		gctFIXED delta = delta1 + delta2;

		if (delta <= FAST_FIXED_EPSILON + FAST_FIXED_EPSILON)
		{
			/* Extremely short curve.  Treat it as a line. */
			return(_FastAddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath));
		}
		else if (delta <= (FAST_FIXED_EPSILON << 2))
		{
			/* Very short curve.  Set n = 2. */
			n = 2;
			goto AddLines;
		}
	}

	/* Formula.
	 * f(t) = (1 - t)^2 * p0 + 2 * t * (1 - t) * p1 + t^2 * p2
	 *      = a0 + a1 * t + a2 * t^2
	 *   a0 = p0
	 *   a1 = 2 * (-p0 + p1)
	 *   a2 = p0 - 2 * p1 + p2
	 */
	a1x = dx1 + dx1;
	a1y = dy1 + dy1;
	a2x = dx2 - dx1;
	a2y = dy2 - dy1;

	/* Step 1: Calculate N. */
#if !gcvSEGMENT_COUNT
	{
		/* Lefan's method. */
		/* dist(t) = ...
		 * t2 = ...
		 * if 0 <= t2 <= 1
		 *    upperBound = dist(t2)
		 * else
		 *    upperBound = max(dist(0), dist(1))
		 * N = ceil(sqrt(upperbound / epsilon / 8))
		 */
		/* Prepare dist(t). */
		gctINT64 f1, f2, t1, t2, upperBound;

		f1 = (gctINT64) a1x * (gctINT64) a2y - (gctINT64) a2x * (gctINT64) a1y;
		if (f1 != 0)
		{
#ifdef UNDER_CE
			double tempDouble = 0.0;
#endif
			if (f1 < 0) f1 = -f1;

			/* Calculate t2. */
			t1 = (gctINT64) a2x * (gctINT64) a2x + (gctINT64) a2y * (gctINT64) a2y;
			t2 = -((gctINT64) dx1 * (gctINT64) a2x + (gctINT64) dy1 * (gctINT64) a2y);
			t2 = (t2 << 16) / t1;
			/* Calculate upperBound. */
			if (t2 >= 0 && t2 <= FIXED_ONE)
			{
				f2 = (gctINT64) dx1 + (((gctINT64) a2x * t2) >> 16);
				f2 *= f2;
				t1 = (gctINT64) dy1 + (((gctINT64) a2y * t2) >> 16);
				t1 *= t1;
				upperBound = t1 + f2;
			}
			else
			{
				t1 = (gctINT64) dx1 * (gctINT64) dx1 + (gctINT64) dy1 * (gctINT64) dy1;
				t2 = (gctINT64) dx2 * (gctINT64) dx2 + (gctINT64) dy2 * (gctINT64) dy2;
				upperBound = t1 < t2 ? t1 : t2;
			}
			/* Calculate n. */
#ifdef UNDER_CE
			tempDouble = (double)upperBound;
			upperBound = f1 / (gctINT64)sqrt(tempDouble);
#else
			upperBound = f1 / (gctINT64) sqrt((double) upperBound);
#endif
#if !vgvSTROKE_COPY_OPTIMIZE || ! gcvUSE_INTERNAL_SCALE
			if (StrokeConversion->largeTransformScale >= 2.0f)
			{
				upperBound += upperBound;
			}
#endif
			if (StrokeConversion->strokeLineWidth > 1.414f)
			{
				upperBound += upperBound;
			}
#if gcvSTROKE_REDUCE_SEGMENT
			n = (gctINT) (upperBound >> 18);
#else
			n = (gctINT) (upperBound >> 16);
#endif

			if (n > 16)
			{
				if (n > 64)
				{
					if (n > 256)
					{
								StrokeConversion->useFastMode = gcvFALSE;
								return gcvSTATUS_OK;
					}
					else		{ n = 16;	nShift = 4; }
				}
				else			{ n = 8;	nShift = 3; }
			}
			else
			{
				if (n > 4)		{ n = 4;	nShift = 2; }
				else if (n > 1)	{ n = 2;	nShift = 1; }
				else			{ n = 1;	nShift = 0; }
			}
		}
		else
		{
			/* n = 0 => n = 16. */
			n = 16;	nShift = 4;
		}
	}
#else
	n = gcvSEGMENT_COUNT;
	if (n > 4)
	{
		if (n > 8)		{ n = 16;	nShift = 4; }
		else			{ n = 8;	nShift = 3; }
	}
	else
	{
		if (n > 2)		{ n = 4;	nShift = 2; }
		else if (n > 1)	{ n = 2;	nShift = 1; }
		else			{ n = 1;	nShift = 0; }
	}
#endif

#if gcvQUAD_STATISTICS
	if (n < g_minQuadSegments)
	{
		g_minQuadSegments = n;
	}

	if (n > g_maxQuadSegments)
	{
		g_maxQuadSegments = n;
	}

	if (g_avrQuadSegments == ~0)
	{
		g_avrQuadSegments = n;
	}
	else
	{
		g_avrQuadSegments = (g_avrQuadSegments + n) / 2;
	}
#endif

  AddLines:

	VGSL_STAT_COUNTER_ADD(vgStrokeFlattenedSegmentCount, n);

	if (n == 2)
	{
		gctFIXED x, y;

		/* Add middle point. */
		x = (X0 + X1 + X1 + X2) >> 2;
		y = (Y0 + Y1 + Y1 + Y2) >> 2;
		gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, x, y, SubPath));
	}
	else if (n > 1)
	{
		gctINT dShift, dsquareShift;
		gctFIXED dx, dy, ddx, ddy;
		gctFIXED x, y;
		gctINT i;

		/* Step 2: Calculate deltas. */
		/*   Df(t) = f(t + d) - f(t)
		 *         = a1 * d + a2 * d^2 + 2 * a2 * d * t
		 *  DDf(t) = Df(t + d) - Df(t)
		 *         = 2 * a2 * d^2
		 *    f(0) = a0
		 *   Df(0) = a1 * d + a2 * d^2
		 *  DDf(0) = 2 * a2 * d^2
		 */
		dShift       = nShift;
		dsquareShift = dShift << 1;
		ddx	= a2x >> dsquareShift;
		ddy = a2y >> dsquareShift;
		dx	= (a1x >> dShift) + ddx;
		dy	= (a1y >> dShift) + ddy;
		ddx	+= ddx;
		ddy += ddy;

		/* Step 3: Add points. */
		x = X0;
		y = Y0;
		for (i = 1; i < n; i++)
		{
			x += dx;
			y += dy;

			/* Add a point to subpath. */
			gcmERROR_RETURN(_FastAddPointToSubPathWDelta_I(StrokeConversion,
				x, y, dx, dy, SubPath));

			dx += ddx;
			dy += ddy;
		}
	}

	/* Add point 2 separately to avoid cumulative errors. */
	gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath));

	return gcvSTATUS_OK;
}

gceSTATUS
_FastFlattenCubicBezier_I(
	vgsSTROKECONVERSION_PTR	StrokeConversion,
	gctFIXED X0,
	gctFIXED Y0,
	gctFIXED X1,
	gctFIXED Y1,
	gctFIXED X2,
	gctFIXED Y2,
	gctFIXED X3,
	gctFIXED Y3,
	vgsSUBPATH_I_PTR SubPath
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gctINT n;
	gctFIXED a1x, a1y, a2x, a2y, a3x, a3y;
	gctINT nShift;
	gctFIXED dx1 = X1 - X0;
	gctFIXED dy1 = Y1 - Y0;
	gctFIXED dx2 = X2 - X1;
	gctFIXED dy2 = Y2 - Y1;
	gctFIXED dx3 = X3 - X2;
	gctFIXED dy3 = Y3 - Y2;
	gctFIXED delta = (dx1 >= 0 ? dx1 : -dx1) + (dy1 >= 0 ? dy1 : -dy1);
	delta += (dx2 >= 0 ? dx2 : -dx2) + (dy2 >= 0 ? dy2 : -dy2);
	delta += (dx3 >= 0 ? dx3 : -dx3) + (dy3 >= 0 ? dy3 : -dy3);

	VGSL_STAT_COUNTER_INCREASE(vgStrokeCurveCount);

	if (delta == FIXED_ZERO)
	{
		/* Degenerated Bezier curve.  Becomes a point. */
		/* Discard zero-length segments. */
		return gcvSTATUS_OK;
	}
	else if (delta <= FAST_FIXED_EPSILON + FAST_FIXED_EPSILON)
	{
		/* Extremely short curve.  Treat it as a line. */
		return(_FastAddPointToSubPath_I(StrokeConversion, X2, Y2, SubPath));
	}
	else if (delta <= (FAST_FIXED_EPSILON << 2))
	{
		/* Very short curve.  Set n = 2. */
		n = 2;
		goto AddLines;
	}

	/* Formula.
	 * f(t) = (1 - t)^3 * p0 + 3 * t * (1 - t)^2 * p1 + 3 * t^2 * (1 - t) * p2 + t^3 * p3
	 *      = a0 + a1 * t + a2 * t^2 + a3 * t^3
	 */
	a1x  = dx1 + dx1 + dx1;
	a1y  = dy1 + dy1 + dy1;
	a2x  = dx2 - dx1;
	a2x *= 3;
	a2y  = dy2 - dy1;
	a2y *= 3;
	a3x  = (X3 - X0) - dx2 - dx2 - dx2;
	a3y  = (Y3 - Y0) - dy2 - dy2 - dy2;

	/* Step 1: Calculate N. */
#if !gcvSEGMENT_COUNT
	{
		/* Lefan's method. */
		/*  df(t)/dt  = a1 + 2 * a2 * t + 3 * a3 * t^2
		 * d2f(t)/dt2 = 2 * a2 + 6 * a3 * t
		 * N = ceil(sqrt(max(ddfx(0)^2 + ddfy(0)^2, ddfx(1)^2 + ddyf(1)^2) / epsilon / 8))
		 */
		gctINT64 ddf0, ddf1, upperBound;
		gctINT64 t1, t2;
#ifdef UNDER_CE
		double tempDouble = 0.0;
#endif

		ddf0 = (gctINT64) a2x * (gctINT64) a2x + (gctINT64) a2y * (gctINT64) a2y;
		t1 = (gctINT64) (a2x + a3x + a3x + a3x);
		t2 = (gctINT64) (a2y + a3y + a3y + a3y);
		ddf1 = t1 * t1 + t2 * t2;
		upperBound = ddf0 > ddf1 ? ddf0: ddf1;
#ifdef UNDER_CE
		tempDouble = (double)upperBound;
		upperBound = (gctINT64) sqrt(tempDouble);
#else
		upperBound = (gctINT64) sqrt((double)upperBound);
#endif
#if !vgvSTROKE_COPY_OPTIMIZE || ! gcvUSE_INTERNAL_SCALE
		if (StrokeConversion->largeTransformScale >= 2.0f)
		{
			upperBound += upperBound;
		}
#endif
		if (StrokeConversion->strokeLineWidth > 1.414f)
		{
			upperBound += upperBound;
		}
#if gcvSTROKE_REDUCE_SEGMENT
		n = (gctINT) (upperBound >> 20);
#else
		n = (gctINT) (upperBound >> 18);
#endif

		if (n > 16)
		{
			if (n > 64)
			{
				if (n > 256)
				{
							StrokeConversion->useFastMode = gcvFALSE;
							return gcvSTATUS_OK;
				}
				else		{ n = 16;	nShift = 4; }
			}
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
	n = gcvSEGMENT_COUNT;
	if (n > 4)
	{
		if (n > 8)		{ n = 16;	nShift = 4; }
		else			{ n = 8;	nShift = 3; }
	}
	else
	{
		if (n > 2)		{ n = 4;	nShift = 2; }
		else if (n > 1)	{ n = 2;	nShift = 1; }
		else			{ n = 1;	nShift = 0; }
	}
#endif

#if gcvCUBIC_STATISTICS
	if (n < g_minCubicSegments)
	{
		g_minCubicSegments = n;
	}

	if (n > g_maxCubicSegments)
	{
		g_maxCubicSegments = n;
	}

	if (g_avrCubicSegments == ~0)
	{
		g_avrCubicSegments = n;
	}
	else
	{
		g_avrCubicSegments = (g_avrCubicSegments + n) / 2;
	}
#endif

  AddLines:

	VGSL_STAT_COUNTER_ADD(vgStrokeFlattenedSegmentCount, n);

	if (n == 2)
	{
		gctFIXED x, y;

		/* Add middle point. */
		x = X1 + X2;
		x = x + x + x;
		x = (x + X0 + X3) >> 3;
		y = Y1 + Y2;
		y = y + y + y;
		y = (y + Y0 + Y3) >> 3;
		gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, x, y, SubPath));
	}
	else if (n > 1)
	{
		gctINT dShift, dsquareShift, dcubeShift;
		gctFIXED dx, dy, ddx, ddy, dddx, dddy;
		gctFIXED x, y;
		gctINT i;

		/* Step 2: Calculate deltas */
		/*   Df(t) = f(t + d) - f(t)
		 *  DDf(t) = Df(t + d) - Df(t)
		 * DDDf(t) = DDf(t + d) - DDf(t)
		 *    f(0) = a0
		 *   Df(0) = a1 * d + a2 * d^2 + a3 * d^3
		 *  DDf(0) = 2 * a2 * d^2 + 6 * a3 * d^3
		 * DDDf(0) = 6 * a3 * d^3
		 */
		dShift       = nShift;
		dsquareShift = dShift << 1;
		dcubeShift   = dShift + dsquareShift;
		ddx   = a2x >> dsquareShift;
		ddy   = a2y >> dsquareShift;
		dddx  = a3x >> dcubeShift;
		dddy  = a3y >> dcubeShift;
		dx    = (a1x >> dShift) + ddx + dddx;
		dy    = (a1y >> dShift) + ddy + dddy;
		ddx  += ddx;
		ddy  += ddy;
		dddx *= 6;
		dddy *= 6;
		ddx  += dddx;
		ddy  += dddy;

		/* Step 3: Add points. */
		x = X0;
		y = Y0;
		for (i = 1; i < n; i++)
		{
			x += dx;
			y += dy;

			/* Add a point to subpath. */
			gcmERROR_RETURN(_FastAddPointToSubPathWDelta_I(StrokeConversion,
				x, y, dx, dy, SubPath));

			dx += ddx; ddx += dddx;
			dy += ddy; ddy += dddy;
		}
	}

	/* Add point 3 separately to avoid cumulative errors. */
	gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, X3, Y3, SubPath));

	return gcvSTATUS_OK;
}
#endif

static gceSTATUS _GetValueGetter_I(
	gcePATHTYPE DataType,
	gctFIXED PathScale,
	gctFIXED PathBias,
	vgSL_ValueGetter_I * GetValue,
	gctINT * DataTypeSize
	)
{

	/* Select the data picker. */
	switch (DataType)
	{
	case gcePATHTYPE_INT8:
		if (PathScale == FIXED_ONE)
		{
			*GetValue = (PathBias == FIXED_ZERO ? _GetS8_I_NS_NB : _GetS8_I_NS);
		}
		else
		{
			*GetValue = (PathBias == FIXED_ZERO ? _GetS8_I_NB : _GetS8_I);
		}
		*DataTypeSize = 1;
		break;

	case gcePATHTYPE_INT16:
		if (PathScale == FIXED_ONE)
		{
			*GetValue = (PathBias == FIXED_ZERO ? _GetS16_I_NS_NB : _GetS16_I_NS);
		}
		else
		{
			*GetValue = (PathBias == FIXED_ZERO ? _GetS16_I_NB : _GetS16_I);
		}
		*DataTypeSize = 2;
		break;

	case gcePATHTYPE_INT32:
		if (PathScale == FIXED_ONE)
		{
			*GetValue = (PathBias == FIXED_ZERO ? _GetS32_I_NS_NB : _GetS32_I_NS);
		}
		else
		{
			*GetValue = (PathBias == FIXED_ZERO ? _GetS32_I_NB : _GetS32_I);
		}
		*DataTypeSize = 4;
		break;

	case gcePATHTYPE_FLOAT:
		if (PathScale == FIXED_ONE)
		{
			*GetValue = (PathBias == FIXED_ZERO ? _GetF_I_NS_NB : _GetF_I_NS);
		}
		else
		{
			*GetValue = (PathBias == FIXED_ZERO ? _GetF_I_NB : _GetF_I);
		}
		*DataTypeSize = 4;
		break;

	default:
		gcmASSERT(0);
		return gcvSTATUS_INVALID_ARGUMENT;
	}

	return gcvSTATUS_OK;
}

static gceSTATUS
_FlattenPath_I(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsPATH_PTR Path
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	gctUINT pathHeadReserve;
	gcsCMDBUFFER_PTR currentBuffer;
	vgsPATH_DATA_PTR currentPath;

	gctUINT increment;
	vgsSUBPATH_I_PTR subPath = gcvNULL;
	vgsSUBPATH_I_PTR prevSubPath = gcvNULL;
	gctBOOL isRelative;
	gctFIXED pathBias;
	gctFIXED pathScale;
	gctUINT32 size;						/* The data size of the path. */
	gceVGCMD pathCommand;				/* Path command of each segment in the VGPath. */
	gceVGCMD prevCommand;				/* Previous command when iterating a path. */
	vgSL_ValueGetter_I getValue;		/* Funcion pointer to read a value from a general memory address. */
	gctINT dataTypeSize;				/* Size of the data type. */
	gctUINT8_PTR dataPointer = gcvNULL;	/* Pointer to the data of the VGPath. */
	gctFIXED sx, sy;					/* Starting point of the current subpath. */
	gctFIXED ox, oy;					/* Last point of the previous segment. */
	gctFIXED px, py;					/* Last internal control point of the previous segment. */
	gctFIXED x0, y0, x1, y1, x2, y2;	/* Coordinates of the command. */

	/* Get the path head reserve size. */
	pathHeadReserve = Path->pathInfo.reservedForHead;

	/* Set the initial buffer. */
	currentPath = Path->head;

	/* Initialize path bias and scale. */
#if vgvSTROKE_COPY_OPTIMIZE && gcvUSE_INTERNAL_SCALE
	{
		gctFLOAT one = (gctFLOAT) (1 << (FIXED_FRACTION_BITS - StrokeConversion->internalShiftBits));
		pathBias  = (gctFIXED) (StrokeConversion->pathBias  * one);
		pathScale = (gctFIXED) (StrokeConversion->pathScale * one);
	}
#else
	pathBias  = FLOAT_TO_FIXED(StrokeConversion->pathBias);
	pathScale = FLOAT_TO_FIXED(StrokeConversion->pathScale);
#endif

	sx = sy = ox = oy = px = py = FIXED_ZERO;

	prevCommand = gcvVGCMD_MOVE;

	/* Add a subpath. */
	gcmERROR_RETURN(_AddSubPath(StrokeConversion, (vgsSUBPATH_PTR *) &subPath));

	while (currentPath)
	{
		/* Get a shortcut to the path buffer. */
		currentBuffer = vgmGETCMDBUFFER(currentPath);

		/* Determine the data size. */
		size = currentBuffer->offset - pathHeadReserve;

		/* Determine the beginning of the path data. */
		dataPointer
			= (gctUINT8_PTR) currentBuffer
			+ currentBuffer->bufferOffset
			+ pathHeadReserve;

		/* Select the data picker. */
		gcmERROR_RETURN(_GetValueGetter_I(currentPath->data.dataType,
			pathScale, pathBias, &getValue, &dataTypeSize));

		/* Add an extra gcvVGCMD_MOVE 0.0 0.0 to handle the case the first command is not gcvVGCMD_MOVE. */
		if ((currentPath == Path->head) &&
			(*dataPointer != gcvVGCMD_MOVE))
		{
			/* Add first point to subpath. */
			gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, sx, sy, subPath, vgcFLATTEN_NO));
		}

		while (size > 0)
		{
			/* Get the command. */
			pathCommand = *dataPointer & gcvVGCMD_MASK;

			/* Assume absolute. */
			isRelative = gcvFALSE;

			switch (pathCommand)
			{
			case gcvVGCMD_CLOSE:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_CLOSE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip the command. */
				dataPointer += 1;
				size -= 1;

				/* Check if subPath is already closed. */
				if (ox != sx || oy != sy)
				{
					/* Add a line from current point to the first point of current subpath. */
					gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, sx, sy, subPath, vgcFLATTEN_NO));
				}
				if (subPath->pointList != subPath->lastPoint)
				{
					/* Copy tangent data from first point to lastpoint. */
					vgsPOINT_I_PTR firstPoint = subPath->pointList;
					vgsPOINT_I_PTR lastPoint = subPath->lastPoint;
					lastPoint->length = firstPoint->length;
					lastPoint->tangentX = firstPoint->tangentX;
					lastPoint->tangentY = firstPoint->tangentY;
				}
				else
				{
					/* Single point path. */
					vgsPOINT_I_PTR point = subPath->pointList;
					point->tangentX = FIXED_ZERO;
					point->tangentY = FIXED_ZERO;
					point->length = FIXED_ZERO;
				}
				subPath->closed = gcvTRUE;
				subPath->lastPoint->next = gcvNULL;

				/* Add a subpath. */
				prevSubPath = subPath;
				gcmERROR_RETURN(_AddSubPath(StrokeConversion, (vgsSUBPATH_PTR *) &subPath));

				/* Add first point to subpath. */
				gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, sx, sy, subPath, vgcFLATTEN_NO));

				px = ox = sx;
				py = oy = sy;
				break;

			case gcvVGCMD_MOVE_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_MOVE:        /* Indicate the beginning of a new sub-path. */

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_MOVE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY_I(x0, y0);

				if (prevCommand != gcvVGCMD_MOVE && prevCommand != gcvVGCMD_MOVE_REL
					&&prevCommand != gcvVGCMD_CLOSE)
				{
					/* Close the current subpath, and add a subpath. */
					subPath->lastPoint->next = gcvNULL;
					if (subPath->pointCount == 1)
					{
						/* Single point path. */
						vgsPOINT_I_PTR point = subPath->pointList;
						point->tangentX = FIXED_ZERO;
						point->tangentY = FIXED_ZERO;
						point->length = FIXED_ZERO;
					}
					prevSubPath = subPath;
					gcmERROR_RETURN(_AddSubPath(StrokeConversion, (vgsSUBPATH_PTR *) &subPath));

					/* Add first point to subpath. */
					gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, x0, y0, subPath, vgcFLATTEN_NO));
				}
				else if (subPath->pointCount == 0)
				{
					/* First command is gcvVGCMD_MOVE. */
					/* Add first point to subpath. */
					gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, x0, y0, subPath, vgcFLATTEN_NO));
				}
				else
				{
					/* Continuous gcvVGCMD_MOVE. */
					/* Replace the previous move-to point to current move-to point. */
					gcmASSERT(subPath != gcvNULL);
					gcmASSERT(subPath->pointCount == 1);
					subPath->pointList->x = x0;
					subPath->pointList->y = y0;
				}

				sx = px = ox = x0;
				sy = py = oy = y0;
				break;

			case gcvVGCMD_LINE_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_LINE:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_LINE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY_I(x0, y0);

				/* Add a point to subpath. */
				gcmERROR_RETURN(_AddPointToSubPath_I(StrokeConversion, x0, y0, subPath, vgcFLATTEN_NO));

				px = ox = x0;
				py = oy = y0;
				break;

			case gcvVGCMD_QUAD_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_QUAD:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_QUAD, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY_I(x0, y0);
				VGSL_GETCOORDXY_I(x1, y1);

				gcmERROR_RETURN(_FlattenQuadBezier_I(
					StrokeConversion, ox, oy, x0, y0, x1, y1, subPath
					));

				if (StrokeConversion->useFixedPoint == gcvFALSE)
				{
					goto needToUseFloat;
				}

				px = x0;
				py = y0;
				ox = x1;
				oy = y1;
				break;

			case gcvVGCMD_CUBIC_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_CUBIC:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_CUBIC, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY_I(x0, y0);
				VGSL_GETCOORDXY_I(x1, y1);
				VGSL_GETCOORDXY_I(x2, y2);

				gcmERROR_RETURN(_FlattenCubicBezier_I(
					StrokeConversion, ox, oy, x0, y0, x1, y1, x2, y2, subPath
					));

				if (StrokeConversion->useFixedPoint == gcvFALSE)
				{
					goto needToUseFloat;
				}

				px = x1;
				py = y1;
				ox = x2;
				oy = y2;
				break;

			default:
				gcmASSERT(gcvFALSE);
				return gcvSTATUS_INVALID_DATA;
			}

			prevCommand = pathCommand;
		}

		/* Advance to the next subbuffer. */
		currentPath = (vgsPATH_DATA_PTR) currentBuffer->nextSubBuffer;
	}

	if ((prevCommand == gcvVGCMD_CLOSE) ||
		(prevCommand == gcvVGCMD_MOVE))
	{
		/* Remove the extra subpath. */
		gcmASSERT(subPath->pointCount == 1);
		_FreePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR) subPath->pointList);
		_FreeSubPath(StrokeConversion->subPathMemPool, (vgsSUBPATH_PTR) subPath);
		if (prevSubPath)
		{
			prevSubPath->next = gcvNULL;
		}
		else
		{
			StrokeConversion->subPathList = gcvNULL;
		}
		StrokeConversion->lastSubPath = (vgsSUBPATH_PTR) prevSubPath;
	}
	else
	{
		subPath->lastPoint->next = gcvNULL;
		if (subPath->pointCount == 1)
		{
			/* Single point path. */
			vgsPOINT_I_PTR point = subPath->pointList;
			point->tangentX = FIXED_ZERO;
			point->tangentY = FIXED_ZERO;
			point->length = FIXED_ZERO;
		}
	}

	return status;

needToUseFloat:
	StrokeConversion->useFixedPoint = gcvFALSE;

	/* TODO - Use free list. */
	/* Free subPaths. */
	if (StrokeConversion->subPathList)
	{
		vgsSUBPATH_PTR subPath, nextSubPath;

		for (subPath = StrokeConversion->subPathList; subPath; subPath = nextSubPath)
		{
			vgsPOINT_PTR point, nextPoint;
			gctINT i;

			nextSubPath = subPath->next;
			if (subPath->lastPoint == gcvNULL)
			{
				continue;
			}
			subPath->lastPoint->next = gcvNULL;
			for (i = 0, point = subPath->pointList; i < subPath->pointCount; i++, point = nextPoint)
			{
				nextPoint = point->next;
				_FreePoint(StrokeConversion->pointMemPool, point);
			}
			gcmASSERT(point == gcvNULL);
			_FreeSubPath(StrokeConversion->subPathMemPool, subPath);
		}
		StrokeConversion->subPathList = StrokeConversion->lastSubPath = gcvNULL;
	}

	return gcvSTATUS_OK;
}

#if gcvUSE_FAST_STROKE
static gceSTATUS
_FastFlattenPath_I(
	vgsSTROKECONVERSION_PTR StrokeConversion,
	vgsPATH_PTR Path
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	gctUINT pathHeadReserve;
	gcsCMDBUFFER_PTR currentBuffer;
	vgsPATH_DATA_PTR currentPath;

	gctUINT increment;
	vgsSUBPATH_I_PTR subPath = gcvNULL;
	vgsSUBPATH_I_PTR prevSubPath = gcvNULL;
	gctBOOL isRelative;
	gctFIXED pathBias;
	gctFIXED pathScale;
	gctUINT32 size;						/* The data size of the path. */
	gceVGCMD pathCommand;				/* Path command of each segment in the VGPath. */
	gceVGCMD prevCommand;				/* Previous command when iterating a path. */
	vgSL_ValueGetter_I getValue;		/* Funcion pointer to read a value from a general memory address. */
	gctINT dataTypeSize;				/* Size of the data type. */
	gctUINT8_PTR dataPointer = gcvNULL;	/* Pointer to the data of the VGPath. */
	gctFIXED sx, sy;					/* Starting point of the current subpath. */
	gctFIXED ox, oy;					/* Last point of the previous segment. */
	gctFIXED px, py;					/* Last internal control point of the previous segment. */
	gctFIXED x0, y0, x1, y1, x2, y2;	/* Coordinates of the command. */

	/* Get the path head reserve size. */
	pathHeadReserve = Path->pathInfo.reservedForHead;

	/* Set the initial buffer. */
	currentPath = Path->head;

	/* Initialize path bias and scale. */
#if vgvSTROKE_COPY_OPTIMIZE && gcvUSE_INTERNAL_SCALE
	{
		gctINT one = 1 << (FIXED_FRACTION_BITS - StrokeConversion->internalShiftBits);
		pathBias  = (gctFIXED) (StrokeConversion->pathBias  * one);
		pathScale = (gctFIXED) (StrokeConversion->pathScale * one);
	}
#else
	pathBias  = FLOAT_TO_FIXED(StrokeConversion->pathBias);
	pathScale = FLOAT_TO_FIXED(StrokeConversion->pathScale);
#endif

	sx = sy = ox = oy = px = py = FIXED_ZERO;

	prevCommand = gcvVGCMD_MOVE;

	/* Add a subpath. */
	gcmERROR_RETURN(_AddSubPath(StrokeConversion, (vgsSUBPATH_PTR *) &subPath));

	while (currentPath)
	{
		/* Get a shortcut to the path buffer. */
		currentBuffer = vgmGETCMDBUFFER(currentPath);

		/* Determine the data size. */
		size = currentBuffer->offset - pathHeadReserve;

		/* Determine the beginning of the path data. */
		dataPointer
			= (gctUINT8_PTR) currentBuffer
			+ currentBuffer->bufferOffset
			+ pathHeadReserve;

		/* Select the data picker. */
		gcmERROR_RETURN(_GetValueGetter_I(currentPath->data.dataType,
			pathScale, pathBias, &getValue, &dataTypeSize));

		/* Add an extra gcvVGCMD_MOVE 0.0 0.0 to handle the case the first command is not gcvVGCMD_MOVE. */
		if ((currentPath == Path->head) &&
			(*dataPointer != gcvVGCMD_MOVE))
		{
			/* Add first point to subpath. */
			gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, sx, sy, subPath));
		}

		while (size > 0)
		{
			/* Get the command. */
			pathCommand = *dataPointer & gcvVGCMD_MASK;

			/* Assume absolute. */
			isRelative = gcvFALSE;

			switch (pathCommand)
			{
			case gcvVGCMD_CLOSE:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_CLOSE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip the command. */
				dataPointer += 1;
				size -= 1;

				/* Check if subPath is already closed. */
				if (ox != sx || oy != sy)
				{
					/* Add a line from current point to the first point of current subpath. */
					gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, sx, sy, subPath));
				}
				subPath->closed = gcvTRUE;
				subPath->lastPoint->next = gcvNULL;

				/* Add a subpath. */
				prevSubPath = subPath;
				gcmERROR_RETURN(_AddSubPath(StrokeConversion, (vgsSUBPATH_PTR *) &subPath));

				/* Add first point to subpath. */
				gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, sx, sy, subPath));

				px = ox = sx;
				py = oy = sy;
				break;

			case gcvVGCMD_MOVE_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_MOVE:        /* Indicate the beginning of a new sub-path. */

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_MOVE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY_I(x0, y0);

				if (prevCommand != gcvVGCMD_MOVE && prevCommand != gcvVGCMD_MOVE_REL
					&&prevCommand != gcvVGCMD_CLOSE)
				{
					/* Close the current subpath, and add a subpath. */
					subPath->lastPoint->next = gcvNULL;
					prevSubPath = subPath;
					gcmERROR_RETURN(_AddSubPath(StrokeConversion, (vgsSUBPATH_PTR *) &subPath));

					/* Add first point to subpath. */
					gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, x0, y0, subPath));
				}
				else if (subPath->pointCount == 0)
				{
					/* First command is gcvVGCMD_MOVE. */
					/* Add first point to subpath. */
					gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, x0, y0, subPath));
				}
				else
				{
					/* Continuous gcvVGCMD_MOVE. */
					/* Replace the previous move-to point to current move-to point. */
					gcmASSERT(subPath != gcvNULL);
					gcmASSERT(subPath->pointCount == 1);
					subPath->pointList->x = x0;
					subPath->pointList->y = y0;
				}

				sx = px = ox = x0;
				sy = py = oy = y0;
				break;

			case gcvVGCMD_LINE_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_LINE:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_LINE, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY_I(x0, y0);

				/* Add a point to subpath. */
				gcmERROR_RETURN(_FastAddPointToSubPath_I(StrokeConversion, x0, y0, subPath));

				px = ox = x0;
				py = oy = y0;
				break;

			case gcvVGCMD_QUAD_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_QUAD:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_QUAD, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY_I(x0, y0);
				VGSL_GETCOORDXY_I(x1, y1);

				gcmERROR_RETURN(_FastFlattenQuadBezier_I(
					StrokeConversion, ox, oy, x0, y0, x1, y1, subPath
					));

				if (StrokeConversion->useFastMode == gcvFALSE)
				{
					goto needToUseSlowMode;
				}

				px = x0;
				py = y0;
				ox = x1;
				oy = y1;
				break;

			case gcvVGCMD_CUBIC_REL:
				isRelative = gcvTRUE;
				/* fall through */

			case gcvVGCMD_CUBIC:

#if vgvPRINT_PATH_DIAG
				printf("%s: gcvVGCMD_CUBIC, rel=%d\n", __FUNCTION__, isRelative);
#endif

				/* Skip to the data. */
				vgmSKIPTODATA(dataPointer, dataTypeSize, size);

				VGSL_GETCOORDXY_I(x0, y0);
				VGSL_GETCOORDXY_I(x1, y1);
				VGSL_GETCOORDXY_I(x2, y2);

				gcmERROR_RETURN(_FastFlattenCubicBezier_I(
					StrokeConversion, ox, oy, x0, y0, x1, y1, x2, y2, subPath
					));

				if (StrokeConversion->useFastMode == gcvFALSE)
				{
					goto needToUseSlowMode;
				}

				px = x1;
				py = y1;
				ox = x2;
				oy = y2;
				break;

			default:
				gcmASSERT(gcvFALSE);
				return gcvSTATUS_INVALID_DATA;
			}

			prevCommand = pathCommand;
		}

		/* Advance to the next subbuffer. */
		currentPath = (vgsPATH_DATA_PTR) currentBuffer->nextSubBuffer;
	}

	if ((prevCommand == gcvVGCMD_CLOSE) ||
		(prevCommand == gcvVGCMD_MOVE))
	{
		/* Remove the extra subpath. */
		gcmASSERT(subPath->pointCount == 1);
		_FreePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR) subPath->pointList);
		_FreeSubPath(StrokeConversion->subPathMemPool, (vgsSUBPATH_PTR) subPath);
		if (prevSubPath)
		{
			prevSubPath->next = gcvNULL;
		}
		else
		{
			StrokeConversion->subPathList = gcvNULL;
		}
		StrokeConversion->lastSubPath = (vgsSUBPATH_PTR) prevSubPath;
	}
	else
	{
		subPath->lastPoint->next = gcvNULL;
	}

	return status;

needToUseFloat:
	StrokeConversion->useFixedPoint = gcvFALSE;

needToUseSlowMode:
	StrokeConversion->useFastMode   = gcvFALSE;

	/* TODO - Use free list. */
	/* Free subPaths. */
	if (StrokeConversion->subPathList)
	{
		vgsSUBPATH_PTR subPath, nextSubPath;

		for (subPath = StrokeConversion->subPathList; subPath; subPath = nextSubPath)
		{
			vgsPOINT_PTR point, nextPoint;
			gctINT i;

			nextSubPath = subPath->next;
			if (subPath->lastPoint == gcvNULL)
			{
				continue;
			}
			subPath->lastPoint->next = gcvNULL;
			for (i = 0, point = subPath->pointList; i < subPath->pointCount; i++, point = nextPoint)
			{
				nextPoint = point->next;
				_FreePoint(StrokeConversion->pointMemPool, point);
			}
			gcmASSERT(point == gcvNULL);
			_FreeSubPath(StrokeConversion->subPathMemPool, subPath);
		}
		StrokeConversion->subPathList = StrokeConversion->lastSubPath = gcvNULL;
	}

	return gcvSTATUS_OK;
}
#endif

static void
_ConvertParameters(
	vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	gctINT i;
	vgsSTROKECONVERSION_I_PTR fixedStrokeConversion = (vgsSTROKECONVERSION_I_PTR) StrokeConversion;

#if vgvSTROKE_COPY_OPTIMIZE && gcvUSE_INTERNAL_SCALE
	gctFLOAT one = (gctFLOAT) (1 << (FIXED_FRACTION_BITS - StrokeConversion->internalShiftBits));
	gctFLOAT max;

	if (StrokeConversion->internalShiftBits > 0)
	{
		max = 32767.999f * (1 << StrokeConversion->internalShiftBits);
	}
	else
	{
		max = 32767.999f / (1 << (-StrokeConversion->internalShiftBits));
	}

#define FLOAT_TO_FIXED_SCALE(X)			(gctFIXED) ((X) * one)
#define FLOAT_TO_FIXED_WCHECK_SCALE(X)	((X) > max ? FIXED_MAX : (X) < -max ? FIXED_MIN : FLOAT_TO_FIXED_SCALE(X))

	fixedStrokeConversion->pathScale = FLOAT_TO_FIXED_SCALE(StrokeConversion->pathScale);
	fixedStrokeConversion->pathBias = FLOAT_TO_FIXED_SCALE(StrokeConversion->pathBias);
	fixedStrokeConversion->largeTransformScale = FLOAT_TO_FIXED(StrokeConversion->largeTransformScale);
	fixedStrokeConversion->smallTransformScale = FLOAT_TO_FIXED(StrokeConversion->smallTransformScale);

	fixedStrokeConversion->strokeLineWidth = FLOAT_TO_FIXED_SCALE(StrokeConversion->strokeLineWidth);
	fixedStrokeConversion->halfLineWidth   = FLOAT_TO_FIXED_SCALE(StrokeConversion->halfLineWidth);

	if (StrokeConversion->strokeMiterLimit < 256.0f)
	{
		/* Miter limit does not need to scale. */
		fixedStrokeConversion->strokeMiterLimit = FLOAT_TO_FIXED(StrokeConversion->strokeMiterLimit);
		fixedStrokeConversion->strokeMiterLimitSquare = FLOAT_TO_FIXED(StrokeConversion->strokeMiterLimitSquare);
	}
	else
	{
		/* Miter limit does not need to scale. */
		fixedStrokeConversion->strokeMiterLimit = FLOAT_TO_FIXED(StrokeConversion->strokeMiterLimit);
		fixedStrokeConversion->strokeMiterLimitSquare = FIXED_MAX;
	}

	if (StrokeConversion->strokeDashPatternCount)
	{
		fixedStrokeConversion->strokeDashPhase = FLOAT_TO_FIXED_WCHECK_SCALE(StrokeConversion->strokeDashPhase);
		fixedStrokeConversion->strokeDashInitialLength = FLOAT_TO_FIXED_WCHECK_SCALE(StrokeConversion->strokeDashInitialLength);
		fixedStrokeConversion->strokeDashPatternLength = FLOAT_TO_FIXED_WCHECK_SCALE(StrokeConversion->strokeDashPatternLength);
		for (i = 0; i < StrokeConversion->strokeDashPatternCount; i++)
		{
			fixedStrokeConversion->strokeDashPattern[i] = FLOAT_TO_FIXED_WCHECK_SCALE(StrokeConversion->strokeDashPattern[i]);
		}
	}
#else
	fixedStrokeConversion->pathScale = FLOAT_TO_FIXED(StrokeConversion->pathScale);
	fixedStrokeConversion->pathBias = FLOAT_TO_FIXED(StrokeConversion->pathBias);
	fixedStrokeConversion->largeTransformScale = FLOAT_TO_FIXED(StrokeConversion->largeTransformScale);
	fixedStrokeConversion->smallTransformScale = FLOAT_TO_FIXED(StrokeConversion->smallTransformScale);

	fixedStrokeConversion->strokeLineWidth = FLOAT_TO_FIXED(StrokeConversion->strokeLineWidth);
	fixedStrokeConversion->halfLineWidth = FLOAT_TO_FIXED(StrokeConversion->halfLineWidth);

	if (StrokeConversion->strokeMiterLimit < 256.0f)
	{
		fixedStrokeConversion->strokeMiterLimit = FLOAT_TO_FIXED(StrokeConversion->strokeMiterLimit);
		fixedStrokeConversion->strokeMiterLimitSquare = FLOAT_TO_FIXED(StrokeConversion->strokeMiterLimitSquare);
	}
	else
	{
		fixedStrokeConversion->strokeMiterLimit = FLOAT_TO_FIXED_WCHECK(StrokeConversion->strokeMiterLimit);
		fixedStrokeConversion->strokeMiterLimitSquare = FIXED_MAX;
	}

	if (StrokeConversion->strokeDashPatternCount)
	{
		fixedStrokeConversion->strokeDashPhase = FLOAT_TO_FIXED_WCHECK(StrokeConversion->strokeDashPhase);
		fixedStrokeConversion->strokeDashInitialLength = FLOAT_TO_FIXED_WCHECK(StrokeConversion->strokeDashInitialLength);
		fixedStrokeConversion->strokeDashPatternLength = FLOAT_TO_FIXED_WCHECK(StrokeConversion->strokeDashPatternLength);
		for (i = 0; i < StrokeConversion->strokeDashPatternCount; i++)
		{
			fixedStrokeConversion->strokeDashPattern[i] = FLOAT_TO_FIXED_WCHECK(StrokeConversion->strokeDashPattern[i]);
		}
	}
#endif
}


static gctBOOL
_IsAngleSpanAcute_I(
	gctFIXED Ux,
	gctFIXED Uy,
	gctFIXED Vx,
	gctFIXED Vy
	)
{
	return (((gctINT64) Ux * Vx + (gctINT64) Uy * Vy) > 0 ? gcvTRUE : gcvFALSE);
}

static gctFIXED
_Angle_I(
	gctFIXED X,
	gctFIXED Y,
	gctFIXED Length
	)
{
	gctFIXED angle;
	gctFIXED ux = (X >= FIXED_ZERO ? X : -X);
	gctFIXED uy = (Y >= FIXED_ZERO ? Y : -Y);

	/* angle = gcmACOSF(X / Length); */
	if (ux > uy)
	{
		angle = ((uy > FIXED_ZERO && ux < Length) ? _Asin_I(FIXED_DIV(uy, Length)) : FIXED_ZERO);
	}
	else
	{
		angle = ((ux > FIXED_ZERO && uy < Length) ? (FIXED_PI_HALF - _Asin_I(FIXED_DIV(ux, Length))) : FIXED_PI_HALF);
	}
	gcmASSERT(angle >= -FIXED_PI_HALF && angle <= FIXED_PI_HALF);
	if (X < FIXED_ZERO) angle = FIXED_PI - angle;
	if (Y < FIXED_ZERO) angle = -angle;

	return angle;
}

/* The arc is always counter clockwise and less than half circle (small). */
static gceSTATUS
_ConvertCircleArc_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	gctFIXED Radius,
	gctFIXED CenterX,
	gctFIXED CenterY,
	gctFIXED StartX,
	gctFIXED StartY,
	gctFIXED EndX,
	gctFIXED EndY,
	gctBOOL HalfCircle,
	vgsPOINT_I_PTR *PointList
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	/*gceVGCMD segmentCommand;*/
	gctFIXED theta1, thetaSpan;
	gctINT segs;
	gctFIXED theta, thetaHalf, theta2;
	gctFIXED cosThetaHalf;
	gctFIXED controlRatio;
	gctFIXED controlX, controlY, anchorX, anchorY;
	/*gctFIXED lastX, lastY;*/
	vgsPOINT_I_PTR point, startPoint, lastPoint;

	/* Converting. */
	gcmASSERT(PointList);
	gcmASSERT(StartX != CenterX || StartY != CenterY);
	theta1 = _Angle_I(StartX - CenterX, StartY - CenterY, Radius);
	if (HalfCircle)
	{
		thetaSpan = FIXED_PI;
		segs = 4;
		theta = FIXED_PI_QUARTER;
		thetaHalf = FIXED_PI_EIGHTH;
		cosThetaHalf = FIXED_COS_PI_EIGHTH;
	}
	else
	{
		gcmASSERT(EndX != CenterX || EndY != CenterY);
		thetaSpan = _Angle_I(EndX - CenterX, EndY - CenterY, Radius) - theta1;
		if (thetaSpan == FIXED_ZERO)
		{
			/* Handle specail case for huge scaling. */
			/*gcmASSERT(thetaSpan != FIXED_ZERO);*/
			*PointList = gcvNULL;
			return gcvSTATUS_OK;
		}

		if ((thetaSpan < FIXED_ZERO))
		{
			thetaSpan += FIXED_PI_TWO;
		}
		/*gcmASSERT(thetaSpan > FIXED_ZERO && thetaSpan < FIXED_PI + FIXED_EPSILON);*/

		/* Calculate the number of quadratic Bezier curves. */
		/* Assumption: most of angles are small angles. */
		if      (thetaSpan <= FIXED_PI_QUARTER)			segs = 1;
		else if (thetaSpan <= FIXED_PI_HALF)			segs = 2;
		else if (thetaSpan <= FIXED_PI_THREE_QUARTER)	segs = 3;
		else											segs = 4;

		theta = thetaSpan / segs;
		thetaHalf = theta >> 1;
		cosThetaHalf = _Cos_I(thetaHalf);
	}

	/* Determine the segment command. */
	/*segmentCommand = gcvVGCMD_ARC_QUAD;*/


	/* Generate quadratic Bezier curves. */
	startPoint = lastPoint = gcvNULL;
	controlRatio = FIXED_DIV(Radius, cosThetaHalf);
	while (segs-- > 0)
	{
		theta1 += theta;

		theta2 = theta1 - thetaHalf;
		if (theta2 > FIXED_PI) theta2 -= FIXED_PI_TWO;
		controlX = CenterX + FIXED_MUL(_Cos_I(theta2), controlRatio);
		controlY = CenterY + FIXED_MUL(_Sin_I(theta2), controlRatio);

		if (segs == 0)
		{
			/* Use end point directly to avoid accumulated errors. */
			anchorX = EndX;
			anchorY = EndY;
		}
		else
		{
			theta2 = theta1;
			if (theta2 > FIXED_PI) theta2 -= FIXED_PI_TWO;
			anchorX = CenterX + FIXED_MUL(_Cos_I(theta2), Radius);
			anchorY = CenterY + FIXED_MUL(_Sin_I(theta2), Radius);
		}

		/* Add control point. */
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));
		point->x = controlX;
		point->y = controlY;
		point->curveType = vgcCURVE_QUAD_CONTROL;
		if (lastPoint)
		{
			lastPoint->next = point;
			lastPoint = point;
		}
		else
		{
			startPoint = lastPoint = point;
		}

		/* Add anchor point. */
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));
		point->x = anchorX;
		point->y = anchorY;
		point->curveType = vgcCURVE_QUAD_ANCHOR;
		lastPoint->next = point;
		lastPoint = point;
	}

	lastPoint->next = gcvNULL;
	*PointList = startPoint;

	/* Return status. */
	return status;
}

static gceSTATUS
_AddStrokeSubPath_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	vgsSUBPATH_I_PTR * SubPath
	)
{
	gceSTATUS status;

	gcmERROR_RETURN(_CAllocateSubPath(StrokeConversion->subPathMemPool, (vgsSUBPATH_PTR *) SubPath));

	if (StrokeConversion->lastStrokeSubPath != gcvNULL)
	{
		StrokeConversion->lastStrokeSubPath->next = *SubPath;
		StrokeConversion->lastStrokeSubPath = *SubPath;
	}
	else
	{
		StrokeConversion->lastStrokeSubPath = StrokeConversion->strokeSubPathList = *SubPath;
	}

	return status;
}

gcmINLINE gceSTATUS
_AddAPointToRightStrokePointListTail_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	gctFIXED X,
	gctFIXED Y
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsPOINT_I_PTR point;

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));
	point->x = X;
	point->y = Y;
	point->curveType = vgcCURVE_LINE;
	point->prev = StrokeConversion->lastRightStrokePoint;
	point->next = gcvNULL;
	StrokeConversion->lastRightStrokePoint->next = point;
	StrokeConversion->lastRightStrokePoint = point;
	StrokeConversion->lastStrokeSubPath->pointCount++;

	return status;
}

gcmINLINE gceSTATUS
_AddAPointToLeftStrokePointListHead_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	gctFIXED X,
	gctFIXED Y
	)
{
	gceSTATUS status;
	vgsPOINT_I_PTR point;

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));
	point->x = X;
	point->y = Y;
	point->curveType = vgcCURVE_LINE;
	point->next = StrokeConversion->leftStrokePoint;
	point->prev = gcvNULL;
	StrokeConversion->leftStrokePoint->prev = point;
	StrokeConversion->leftStrokePoint = point;
	StrokeConversion->lastStrokeSubPath->pointCount++;

	return status;
}

static gceSTATUS
_AddZeroLengthStrokeSubPath_I(
	IN vgsSTROKECONVERSION_I_PTR StrokeConversion,
	IN vgsPOINT_I_PTR Point
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsSUBPATH_I_PTR strokeSubPath;
	vgsPOINT_I_PTR newPoint;
	gctFIXED halfWidth = StrokeConversion->halfLineWidth;

	if (StrokeConversion->strokeCapStyle == gcvCAP_BUTT)
	{
		/* No need to draw zero-length subPath for gcvCAP_BUTT. */
		return gcvSTATUS_OK;
	}

	gcmERROR_RETURN(_AddStrokeSubPath_I(StrokeConversion, &strokeSubPath));
	if (StrokeConversion->strokeCapStyle == gcvCAP_SQUARE || StrokeConversion->useFastMode)
	{
		/* Draw a square along the point's direction. */
		gctFIXED dx, dy;

		if (Point->tangentX != FIXED_ZERO || Point->tangentY != FIXED_ZERO)
		{
			dx = FIXED_MUL(Point->tangentY, halfWidth);
			dy = FIXED_MUL(-(Point->tangentX), halfWidth);
		}
		else
		{
			dx = halfWidth;
			dy = FIXED_ZERO;
		}
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &newPoint));
		newPoint->x = Point->x + dx + dy;
		newPoint->y = Point->y - dx + dy;
		newPoint->curveType = vgcCURVE_LINE;
		strokeSubPath->pointList = StrokeConversion->lastRightStrokePoint = newPoint;
		strokeSubPath->pointCount = 1;
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
										Point->x + dx - dy, Point->y + dx + dy));
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
										Point->x - dx - dy, Point->y + dx - dy));
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
										Point->x - dx + dy, Point->y - dx - dy));
	}
	else
	{
		/* Draw a circle. */
		gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &newPoint));
		newPoint->x = Point->x + halfWidth;
		newPoint->y = Point->y;
		newPoint->curveType = vgcCURVE_LINE;
		strokeSubPath->pointList = StrokeConversion->lastRightStrokePoint = newPoint;
		strokeSubPath->pointCount = 1;

		/* Add upper half circle. */
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
										Point->x - halfWidth, Point->y));
		StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW_HALF;
		StrokeConversion->lastRightStrokePoint->centerX = Point->x;
		StrokeConversion->lastRightStrokePoint->centerY = Point->y;

		/* Add lower half circle. */
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
										Point->x + halfWidth, Point->y));
		StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW_HALF;
		StrokeConversion->lastRightStrokePoint->centerX = Point->x;
		StrokeConversion->lastRightStrokePoint->centerY = Point->y;
	}

	strokeSubPath->lastPoint = StrokeConversion->lastRightStrokePoint;
	strokeSubPath->lastPoint->next = gcvNULL;

	return status;
}

static gceSTATUS
_StartANewStrokeSubPath_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	gctFIXED X,
	gctFIXED Y,
	gctFIXED Dx,
	gctFIXED Dy,
	gctBOOL AddEndCap,
	vgsSUBPATH_I_PTR * StrokeSubPath
	)
{
	gceSTATUS status;
	vgsSUBPATH_I_PTR strokeSubPath;
	vgsPOINT_I_PTR newPoint;

	gcmERROR_RETURN(_AddStrokeSubPath_I(StrokeConversion, &strokeSubPath));

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &newPoint));
	newPoint->x = X + Dx;
	newPoint->y = Y + Dy;
	newPoint->curveType = vgcCURVE_LINE;
	newPoint->prev = gcvNULL;
	strokeSubPath->pointList = StrokeConversion->lastRightStrokePoint = newPoint;

	gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &newPoint));
	newPoint->x = X - Dx;
	newPoint->y = Y - Dy;
	newPoint->curveType = vgcCURVE_LINE;
	newPoint->next = gcvNULL;
	strokeSubPath->lastPoint = StrokeConversion->leftStrokePoint = newPoint;

	strokeSubPath->pointCount = 2;

	if (AddEndCap)
	{
		/* Add end cap if the subPath is not closed. */
		switch (StrokeConversion->strokeCapStyle)
		{
		case gcvCAP_BUTT:
			/* No adjustment needed. */
			break;
		case gcvCAP_ROUND:
#if gcvUSE_FAST_STROKE
			if (StrokeConversion->useFastMode)
			{
				/* TODO - Add a point. */
			}
			else
#endif
			{
				/* Add curve. */
				/* Add the starting point again as arc. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
						strokeSubPath->pointList->x, strokeSubPath->pointList->y));
				StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW_HALF;
				StrokeConversion->lastRightStrokePoint->centerX = X;
				StrokeConversion->lastRightStrokePoint->centerY = Y;
				/* Change the starting point to end point. */
				strokeSubPath->pointList->x = strokeSubPath->lastPoint->x;
				strokeSubPath->pointList->y = strokeSubPath->lastPoint->y;
			}
			break;
		case gcvCAP_SQUARE:
			StrokeConversion->lastRightStrokePoint->x += Dy;
			StrokeConversion->lastRightStrokePoint->y -= Dx;
			StrokeConversion->leftStrokePoint->x += Dy;
			StrokeConversion->leftStrokePoint->y -= Dx;
			break;
		}
	}

	*StrokeSubPath = strokeSubPath;
	return status;
}

static gceSTATUS
_EndAStrokeSubPath_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	gctFIXED X,
	gctFIXED Y,
	gctFIXED Dx,
	gctFIXED Dy
	)
{
	gceSTATUS status;

	/* Add points for end of line. */
	gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
										X + Dx, Y + Dy));
	gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
										X - Dx, Y - Dy));

	/* Add end cap if the subPath is not closed. */
	switch (StrokeConversion->strokeCapStyle)
	{
	case gcvCAP_BUTT:
		/* No adjustment needed. */
		break;
	case gcvCAP_ROUND:
#if gcvUSE_FAST_STROKE
		if (StrokeConversion->useFastMode)
		{
			/* TODO - Add a point. */
		}
		else
#endif
		{
			/* Add curve. */
			gcmASSERT(StrokeConversion->leftStrokePoint->curveType == vgcCURVE_LINE);
			StrokeConversion->leftStrokePoint->curveType = vgcCURVE_ARC_SCCW_HALF;
			StrokeConversion->leftStrokePoint->centerX = X;
			StrokeConversion->leftStrokePoint->centerY = Y;
		}
		break;
	case gcvCAP_SQUARE:
		StrokeConversion->lastRightStrokePoint->x -= Dy;
		StrokeConversion->lastRightStrokePoint->y += Dx;
		StrokeConversion->leftStrokePoint->x -= Dy;
		StrokeConversion->leftStrokePoint->y += Dx;
		break;
	}

	/* Concatnate right and left point lists. */
	StrokeConversion->lastRightStrokePoint->next = StrokeConversion->leftStrokePoint;
	StrokeConversion->leftStrokePoint->prev = StrokeConversion->lastRightStrokePoint;

	/*gcmERROR_RETURN(_CheckStrokeSubPath(StrokeConversion->lastStrokeSubPath));*/

	return status;
}

gcmINLINE void
_GetNextDashLength_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	gctINT * DashIndex,
	gctFIXED * DashLength
	)
{
	(*DashIndex)++;
	if (*DashIndex == StrokeConversion->strokeDashPatternCount)
	{
		*DashIndex = 0;
	}
	*DashLength = StrokeConversion->strokeDashPattern[*DashIndex];
}

static gceSTATUS
_DrawSwingPieArea_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	vgsPOINT_I_PTR CenterPoint,
	gctBOOL EndAtPrevPoint
	)
{
	gceSTATUS status;
#if USE_NEW_SWING_HANDLE_FOR_END
	gctFIXED halfWidth = StrokeConversion->halfLineWidth;
	vgsSUBPATH_I_PTR subPath = StrokeConversion->currentSubPath;
	vgsSUBPATH_I_PTR strokeSubPath = StrokeConversion->lastStrokeSubPath;
#endif

	/*gcmERROR_RETURN(_CheckLastStrokeSubPathPartial(StrokeConversion));*/

	if (StrokeConversion->swingCounterClockwise)
	{
		vgsPOINT_I_PTR startPoint = StrokeConversion->swingStartStrokePoint;
		vgsPOINT_I_PTR endPoint = gcvNULL, realEndPoint = gcvNULL;
		vgsPOINT_I_PTR point, prevPoint;
		gctUINT count = 0;

#if USE_NEW_SWING_HANDLE_FOR_END
		if ((StrokeConversion->swingHandling == vgcSWING_OUT &&
			 subPath->length - StrokeConversion->swingAccuLength > halfWidth) ||
			(StrokeConversion->swingHandling == vgcSWING_IN &&
			 StrokeConversion->swingAccuLength > halfWidth))
		{
			vgsPOINT_I_PTR preStartPoint = startPoint->next;

			/* Delete the points between preStartPoint and endPoint. */
			endPoint = StrokeConversion->leftStrokePoint;
			if (endPoint->next == startPoint)
			{
				gcmASSERT(endPoint->curveType == vgcCURVE_ARC_SCCW);
				gcmASSERT(startPoint->curveType == vgcCURVE_LINE);
				endPoint->curveType = vgcCURVE_LINE;
				preStartPoint->prev = endPoint;
				endPoint->next = preStartPoint;
				_FreePoint(StrokeConversion->pointMemPool, startPoint);
				strokeSubPath->pointCount--;
			}
			else
			{
				if (endPoint->curveType == vgcCURVE_LINE)
				{
					/* Delete endPoint because it is not the max swing out point. */
					gcmASSERT(endPoint->next->curveType == vgcCURVE_ARC_SCCW);
					StrokeConversion->leftStrokePoint = endPoint->next;
					_FreePoint(StrokeConversion->pointMemPool, endPoint);
					strokeSubPath->pointCount--;
					endPoint = StrokeConversion->leftStrokePoint;
					endPoint->prev = gcvNULL;
				}
				gcmASSERT(endPoint->curveType == vgcCURVE_ARC_SCCW);
				for (point = startPoint; point != endPoint; point = prevPoint)
				{
					prevPoint = point->prev;
					_FreePoint(StrokeConversion->pointMemPool, point);
					strokeSubPath->pointCount--;
				}
				gcmASSERT(endPoint->curveType == vgcCURVE_ARC_SCCW);
				endPoint->curveType = vgcCURVE_LINE;
				preStartPoint->prev = endPoint;
				endPoint->next = preStartPoint;
			}
		}
		else
#endif
		{
			if (EndAtPrevPoint)
			{
				/* Detach the end point from leftStrokePoint. */
				/* The end point will be added back later. */
				realEndPoint = StrokeConversion->leftStrokePoint;
				StrokeConversion->leftStrokePoint = realEndPoint->next;
				StrokeConversion->leftStrokePoint->prev = gcvNULL;
			}

			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
							CenterPoint->x, CenterPoint->y));
			endPoint = StrokeConversion->leftStrokePoint;

			/* Reverse the point list from startPoint to endPoint. */
			for (point = startPoint; point; point = prevPoint)
			{
				prevPoint = point->prev;
				point->prev = point->next;
				point->next = prevPoint;
				count++;
			}
			gcmASSERT(count == StrokeConversion->swingCount * 2 + 1);
			gcmASSERT(endPoint->next == gcvNULL);
			endPoint->next = startPoint->prev;
			startPoint->prev->prev = endPoint;
			startPoint->prev = gcvNULL;
			StrokeConversion->leftStrokePoint = startPoint;

			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
							CenterPoint->x, CenterPoint->y));
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
							StrokeConversion->swingStartPoint->x,
							StrokeConversion->swingStartPoint->y));
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
							endPoint->prev->x, endPoint->prev->y));

			if (EndAtPrevPoint)
			{
				realEndPoint->next = StrokeConversion->leftStrokePoint;
				StrokeConversion->leftStrokePoint->prev = realEndPoint;
				StrokeConversion->leftStrokePoint = realEndPoint;
			}
		}
	}
	else
	{
		vgsPOINT_I_PTR startPoint = StrokeConversion->swingStartStrokePoint;
		vgsPOINT_I_PTR endPoint = gcvNULL, realEndPoint = gcvNULL;
		vgsPOINT_I_PTR point, nextPoint;
		gctUINT count = 0;

#if USE_NEW_SWING_HANDLE_FOR_END
		if ((StrokeConversion->swingHandling == vgcSWING_OUT &&
			 subPath->length - StrokeConversion->swingAccuLength > halfWidth) ||
			(StrokeConversion->swingHandling == vgcSWING_IN &&
			 StrokeConversion->swingAccuLength > halfWidth))
		{
			vgsPOINT_I_PTR preStartPoint = startPoint->prev;

			/* Delete the points between preStartPoint and endPoint. */
			endPoint = StrokeConversion->lastRightStrokePoint;
			if (endPoint->prev == startPoint)
			{
				gcmASSERT(endPoint->curveType == vgcCURVE_LINE);
				gcmASSERT(startPoint->curveType == vgcCURVE_ARC_SCCW);
				preStartPoint->next = endPoint;
				endPoint->prev = preStartPoint;
				_FreePoint(StrokeConversion->pointMemPool, startPoint);
				strokeSubPath->pointCount--;
			}
			else
			{
				gcmASSERT(endPoint->curveType == vgcCURVE_LINE);
				if (endPoint->prev->curveType == vgcCURVE_LINE)
				{
					/* Delete endPoint because it is not the max swing out point. */
					StrokeConversion->lastRightStrokePoint = endPoint->prev;
					_FreePoint(StrokeConversion->pointMemPool, endPoint);
					strokeSubPath->pointCount--;
					endPoint = StrokeConversion->lastRightStrokePoint;
					endPoint->next = gcvNULL;
				}
				gcmASSERT(endPoint->prev->curveType == vgcCURVE_ARC_SCCW);
				for (point = startPoint; point != endPoint; point = nextPoint)
				{
					nextPoint = point->next;
					_FreePoint(StrokeConversion->pointMemPool, point);
					strokeSubPath->pointCount--;
				}
				gcmASSERT(endPoint->curveType == vgcCURVE_LINE);
				preStartPoint->next = endPoint;
				endPoint->prev = preStartPoint;
			}
		}
		else
#endif
		{
			if (EndAtPrevPoint)
			{
				/* Detach the end point from leftStrokePoint. */
				/* The end point will be added back later. */
				realEndPoint = StrokeConversion->lastRightStrokePoint;
				StrokeConversion->lastRightStrokePoint = realEndPoint->prev;
				StrokeConversion->lastRightStrokePoint->next = gcvNULL;
			}

			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
							CenterPoint->x, CenterPoint->y));
			endPoint = StrokeConversion->lastRightStrokePoint;

			/* Reverse the point list from startPoint to endPoint. */
			for (point = startPoint; point; point = nextPoint)
			{
				nextPoint = point->next;
				point->next = point->prev;
				point->prev = nextPoint;
				count++;
			}
			gcmASSERT(count == StrokeConversion->swingCount * 2 + 1);
			gcmASSERT(endPoint->prev == gcvNULL);
			endPoint->prev = startPoint->next;
			startPoint->next->next = endPoint;
			startPoint->next = gcvNULL;
			StrokeConversion->lastRightStrokePoint = startPoint;

			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
							CenterPoint->x, CenterPoint->y));
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
							StrokeConversion->swingStartPoint->x,
							StrokeConversion->swingStartPoint->y));
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
							endPoint->next->x, endPoint->next->y));

			if (EndAtPrevPoint)
			{
				realEndPoint->prev = StrokeConversion->lastRightStrokePoint;
				StrokeConversion->lastRightStrokePoint->next = realEndPoint;
				StrokeConversion->lastRightStrokePoint = realEndPoint;
			}
		}
	}

	/*gcmERROR_RETURN(_CheckLastStrokeSubPathPartial(StrokeConversion));*/

	StrokeConversion->swingHandling = vgcSWING_NO;
	return gcvSTATUS_OK;
}

static void
_AdjustJointPoint_I(
	vgsPOINT_I_PTR Point,
	vgsPOINT_I_PTR JoinPoint,
	gctFIXED X,
	gctFIXED Y,
	gctFIXED Ratio
	)
{
	gctFIXED mx = (JoinPoint->x + X) >> 1;
	gctFIXED my = (JoinPoint->y + Y) >> 1;
	gctFIXED dx = mx - Point->x;
	gctFIXED dy = my - Point->y;

	dx = FIXED_MUL(dx, Ratio);
	dy = FIXED_MUL(dy, Ratio);
	JoinPoint->x = Point->x + dx;
	JoinPoint->y = Point->y + dy;
}

static gceSTATUS
_ProcessLineJoint_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	vgsPOINT_I_PTR Point,
	gctFIXED Length,
	gctFIXED PrevLength,
	gctUINT SwingHandling,
	gctFIXED X1,
	gctFIXED Y1,
	gctFIXED X2,
	gctFIXED Y2
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gceJOIN_STYLE strokeJoinStyle = StrokeConversion->strokeJoinStyle;
	gctFIXED halfWidth = StrokeConversion->halfLineWidth;
	gctFIXED ratio;
	gctFIXED minLengthSquare;
	gctFIXED cosTheta;
	gctBOOL counterClockwise;
	gctBOOL fatLine = StrokeConversion->isFat;
	gctUINT swingHandling = vgcSWING_NO;
	gctBOOL handleShortLine = gcvFALSE;

	if (StrokeConversion->swingAccuLength < halfWidth)
	{
		if (StrokeConversion->swingNeedToHandle)
		{
			swingHandling = vgcSWING_OUT;
		}
		else
		{
			handleShortLine = gcvTRUE;
		}
	}
	else if (StrokeConversion->currentSubPath->length - StrokeConversion->swingAccuLength < halfWidth)
	{
		if (StrokeConversion->swingNeedToHandle)
		{
			swingHandling = vgcSWING_IN;
		}
		else
		{
			handleShortLine = gcvTRUE;
		}
	}

    if (swingHandling != SwingHandling) {
    	gcmASSERT(swingHandling == SwingHandling);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

	if (StrokeConversion->swingHandling != vgcSWING_NO)
	{
		gcmASSERT(SwingHandling == vgcSWING_NO);
	}

	/* For flattened curves/arcs, the join style is always round. */
	if ((Point->flattenFlag != vgcFLATTEN_NO) && fatLine)
	{
		strokeJoinStyle = gcvJOIN_ROUND;
	}

	/* First, determine the turn is clockwise or counter-clockwise. */
	cosTheta = (gctFIXED)(((gctINT64) Point->prev->tangentX * Point->tangentX + (gctINT64) Point->prev->tangentY * Point->tangentY) >> 16);

	if (cosTheta >= FIXED_ANGLE_EPSILON_COS)
	{
		/* Straight line or semi-straight line--no need to handle join. */
		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			/* Begin to swing to the opposite direction. */
			/* Draw the swing area (pie area). */
			gcmERROR_RETURN(_DrawSwingPieArea_I(StrokeConversion, Point->prev, gcvTRUE));
		}

		/* Add the new stroke points. */
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
		gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			gcmASSERT(Point->flattenFlag == vgcFLATTEN_END);
			StrokeConversion->swingCount++;
		}

		VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredJointsByAngleEpsilonCount);
		goto endCheck;
	}
	else if (cosTheta <= -FIXED_ANGLE_EPSILON_COS)
	{
		/* Almost 180 degree turn. */
		counterClockwise = gcvTRUE;
		ratio = FIXED_MAX;
		minLengthSquare = FIXED_MAX;
	}
	else
	{
		gctINT64 angleSign = (gctINT64) Point->prev->tangentX * Point->tangentY - (gctINT64) Point->prev->tangentY * Point->tangentX;
		counterClockwise = (angleSign >= 0 ? gcvTRUE : gcvFALSE);
		/*gctFIXED halfTheta = FIXED_DIV_BY_TWO(theta);*/
		/*miterRatio = FIXED_DIV(FIXED_ONE, gcmCOSF(halfTheta));*/
		/*ratio = FIXED_MUL(miterRatio, miterRatio);*/
		/*halfMiterLength = FIXED_MUL(halfWidth, tanf(halfTheta));*/
		ratio = FIXED_DIV(FIXED_TWO, (FIXED_ONE + cosTheta));
		gcmASSERT(ratio > FIXED_ONE);
		minLengthSquare = (gctFIXED)((((gctINT64) halfWidth * halfWidth * (FIXED_ONE - cosTheta) / (FIXED_ONE + cosTheta)) >> 16) + FIXED_SIXTEENTH);
		if (minLengthSquare <= FIXED_ZERO)
		{
			minLengthSquare = FIXED_MAX;
		}
	}

	if (StrokeConversion->swingHandling != vgcSWING_NO)
	{
		gcmASSERT(SwingHandling != vgcSWING_NO);
		if (counterClockwise != StrokeConversion->swingCounterClockwise)
		{
			/* Swing to the opposite direction. */
			/* Draw the swing area (pie area). */
			gcmERROR_RETURN(_DrawSwingPieArea_I(StrokeConversion, Point->prev, gcvTRUE));
		}
	}

	if (counterClockwise)
	{
		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			vgsPOINT_I_PTR prevPoint = StrokeConversion->leftStrokePoint->next;	/* Skip the line segment movement. */
			gctFIXED deltaX = X2 - prevPoint->x;
			gctFIXED deltaY = Y2 - prevPoint->y;
			if (_IsAngleSpanAcute_I(StrokeConversion->swingStrokeDeltaX,
									StrokeConversion->swingStrokeDeltaY,
									deltaX, deltaY))
			{
				/* Continue swinging. */
				StrokeConversion->swingStrokeDeltaX = deltaX;
				StrokeConversion->swingStrokeDeltaY = deltaY;
			}
			else
			{
				/* Swing to the max. */
				/* Draw the swing area (pie area). */
				gcmERROR_RETURN(_DrawSwingPieArea_I(StrokeConversion, Point->prev, gcvTRUE));
			}
		}

		/* Check if the miter length is too long for inner intersection. */
		if (StrokeConversion->swingHandling == vgcSWING_NO
		&& ! handleShortLine
		&& minLengthSquare <= FIXED_MUL(Length, Length)
		&& minLengthSquare <= FIXED_MUL(PrevLength, PrevLength))
		{
			/* Adjust leftStrokePoint to the intersection point. */
			_AdjustJointPoint_I(Point, StrokeConversion->leftStrokePoint, X2, Y2, ratio);
		}
		else if (StrokeConversion->swingHandling == vgcSWING_NO && Point->flattenFlag == vgcFLATTEN_NO)
		{
			/* Add the point to avoid incorrect sharp angle. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, Point->x, Point->y));
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
		}
		else if (StrokeConversion->swingHandling == vgcSWING_NO && (! fatLine || SwingHandling == vgcSWING_NO))
		{
			/* Flattened line segments should not have sharp angle. */
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
		}
		else
		{
			if (StrokeConversion->swingHandling == vgcSWING_NO)
			{
				vgsPOINT_I_PTR prevPoint = StrokeConversion->leftStrokePoint;

				/* Start swing handling. */
				StrokeConversion->swingHandling = SwingHandling;
				StrokeConversion->swingCounterClockwise = gcvTRUE;
				StrokeConversion->swingStartPoint = Point;
				StrokeConversion->swingCenterLength = FIXED_ZERO;
				StrokeConversion->swingCount= 0;

				/* Save stroking path delta. */
				StrokeConversion->swingStrokeDeltaX = X2 - prevPoint->x;
				StrokeConversion->swingStrokeDeltaY = Y2 - prevPoint->y;

				/* Add extra center point for swing out pie area. */
				/* TODO - Should adjust prevPoint, instead of adding new point? */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, Point->x, Point->y));

				/* Add extra start stroke point for swing out pie area. */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, prevPoint->x, prevPoint->y));

				StrokeConversion->swingStartStrokePoint = StrokeConversion->leftStrokePoint;
			}

#if USE_MIN_ARC_FILTER
			if (cosTheta > FIXED_MIN_ARC_ANGLE_COS)
			{
				/* Add a point. */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredByMinArcAngleCount);
			}
			else
#endif
			{
				/* Add curve. */
				/* Note that the curve will be reversed, so the direction is CW. */
				/* Then, left side is in reversed order, so the direction is CCW. */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
				StrokeConversion->leftStrokePoint->curveType = vgcCURVE_ARC_SCCW;
				StrokeConversion->leftStrokePoint->centerX = Point->x;
				StrokeConversion->leftStrokePoint->centerY = Point->y;
			}
			StrokeConversion->swingCount++;
		}

		switch (strokeJoinStyle)
		{
		case gcvJOIN_ROUND:
#if USE_MIN_ARC_FILTER
			if (cosTheta > FIXED_MIN_ARC_ANGLE_COS)
			{
				/* Add a point. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredByMinArcAngleCount);
			}
			else
#endif
			{
				/* Add curve. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
				StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW;
				StrokeConversion->lastRightStrokePoint->centerX = Point->x;
				StrokeConversion->lastRightStrokePoint->centerY = Point->y;
			}
			break;
		case gcvJOIN_MITER:
			if (ratio <= StrokeConversion->strokeMiterLimitSquare)
			{
				/* Adjust lastRightStrokePoint to the outer intersection point. */
				_AdjustJointPoint_I(Point, StrokeConversion->lastRightStrokePoint, X1, Y1, ratio);
				break;
			}
			/* Else use Bevel join style. */
		case gcvJOIN_BEVEL:
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
			break;
		}
	}
	else
	{
		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			vgsPOINT_I_PTR prevPoint = StrokeConversion->lastRightStrokePoint->prev;	/* Skip the line segment movement. */
			gctFIXED deltaX = X1 - prevPoint->x;
			gctFIXED deltaY = Y1 - prevPoint->y;
			if (_IsAngleSpanAcute_I(StrokeConversion->swingStrokeDeltaX,
									StrokeConversion->swingStrokeDeltaY,
									deltaX, deltaY))
			{
				/* Continue swinging. */
				StrokeConversion->swingStrokeDeltaX = deltaX;
				StrokeConversion->swingStrokeDeltaY = deltaY;
			}
			else
			{
				/* Swing to the max. */
				/* Draw the swing area (pie area). */
				gcmERROR_RETURN(_DrawSwingPieArea_I(StrokeConversion, Point->prev, gcvTRUE));
			}
		}

		/* Check if the miter length is too long for inner intersection. */
		if (StrokeConversion->swingHandling == vgcSWING_NO
		&& ! handleShortLine
		&& minLengthSquare <= FIXED_MUL(Length, Length)
		&& minLengthSquare <= FIXED_MUL(PrevLength, PrevLength))
		{
			/* Adjust lastRightStrokePoint to the intersection point. */
			_AdjustJointPoint_I(Point, StrokeConversion->lastRightStrokePoint, X1, Y1, ratio);
		}
		else if (StrokeConversion->swingHandling == vgcSWING_NO && Point->flattenFlag == vgcFLATTEN_NO)
		{
			/* Add the point to avoid incorrect sharp angle. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, Point->x, Point->y));
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
		}
		else if (StrokeConversion->swingHandling == vgcSWING_NO && (! fatLine || SwingHandling == vgcSWING_NO))
		{
			/* Flattened line segments should not have sharp angle. */
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
		}
		else
		{
			if (StrokeConversion->swingHandling == vgcSWING_NO)
			{
				vgsPOINT_I_PTR prevPoint = StrokeConversion->lastRightStrokePoint;

				/* Start swing handling. */
				StrokeConversion->swingHandling = SwingHandling;
				StrokeConversion->swingCounterClockwise = gcvFALSE;
				StrokeConversion->swingStartPoint = Point;
				StrokeConversion->swingCenterLength = FIXED_ZERO;
				StrokeConversion->swingCount= 0;

				/* Save stroking path delta. */
				StrokeConversion->swingStrokeDeltaX = X1 - prevPoint->x;
				StrokeConversion->swingStrokeDeltaY = Y1 - prevPoint->y;

				/* Add extra center point for swing out pie area. */
				/* TODO - Should adjust prevPoint, instead of adding new point? */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, Point->x, Point->y));

				/* Add extra start stroke point for swing out pie area. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, prevPoint->x, prevPoint->y));

				StrokeConversion->swingStartStrokePoint = StrokeConversion->lastRightStrokePoint;
			}

#if USE_MIN_ARC_FILTER
			if (cosTheta > FIXED_MIN_ARC_ANGLE_COS)
			{
				/* Add a point. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredByMinArcAngleCount);
			}
			else
#endif
			{
				/* Add curve. */
				/* Note that the curve will be reversed, so the direction is CCW. */
				StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW;
				StrokeConversion->lastRightStrokePoint->centerX = Point->x;
				StrokeConversion->lastRightStrokePoint->centerY = Point->y;
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
			}
			StrokeConversion->swingCount++;
		}

		switch (strokeJoinStyle)
		{
		case gcvJOIN_ROUND:
#if USE_MIN_ARC_FILTER
			if (cosTheta > FIXED_MIN_ARC_ANGLE_COS)
			{
				/* Add a point. */
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredByMinArcAngleCount);
			}
			else
#endif
			{
				/* Add curve. */
				StrokeConversion->leftStrokePoint->curveType = vgcCURVE_ARC_SCCW;
				StrokeConversion->leftStrokePoint->centerX = Point->x;
				StrokeConversion->leftStrokePoint->centerY = Point->y;
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
			}
			break;
		case gcvJOIN_MITER:
			if (ratio <= StrokeConversion->strokeMiterLimitSquare)
			{
				/* Adjust leftStrokePoint to the outer intersection point. */
				_AdjustJointPoint_I(Point, StrokeConversion->leftStrokePoint, X2, Y2, ratio);
				break;
			}
			/* Else use Bevel join style. */
		case gcvJOIN_BEVEL:
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
			break;
		}
	}

endCheck:
	if (StrokeConversion->swingNeedToHandle)
	{
		StrokeConversion->swingAccuLength += Point->length;
	}
	if (StrokeConversion->swingHandling != vgcSWING_NO)
	{
		if (Point->flattenFlag == vgcFLATTEN_END ||
			(StrokeConversion->swingHandling == vgcSWING_OUT &&
			 StrokeConversion->swingAccuLength > halfWidth))
		{
			/* Draw the swing area (pie area). */
			gcmERROR_RETURN(_DrawSwingPieArea_I(StrokeConversion, Point, gcvFALSE));
		}
		else
		{
			/* Check if center line will move too far. */
			StrokeConversion->swingCenterLength += Point->length;
			if (StrokeConversion->swingCenterLength > FIXED_SWING_CENTER_RANGE)
			{
#if USE_NEW_SWING_HANDLE_FOR_END
				if (StrokeConversion->currentSubPath->length < halfWidth ||
					Point->next->flattenFlag == vgcFLATTEN_END)
#endif
				{
					/* Draw the swing area (pie area). */
					gcmERROR_RETURN(_DrawSwingPieArea_I(StrokeConversion, Point, gcvFALSE));
				}
			}
		}
	}

	return status;
}

static gceSTATUS
_CloseAStrokeSubPath_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	vgsPOINT_I_PTR Point,
	gctFIXED Length,
	gctFIXED PrevLength,
	gctUINT SwingHandling,
	vgsPOINT_I_PTR firstStrokePoint,
	vgsPOINT_I_PTR lastStrokePoint
	)
{
	gceSTATUS status;

	/* Handle line joint style for the first/last point in closed path. */
	gcmERROR_RETURN(_ProcessLineJoint_I(
		StrokeConversion, Point,
		Length, PrevLength, SwingHandling,
		firstStrokePoint->x, firstStrokePoint->y,
		lastStrokePoint->x, lastStrokePoint->y
		));

	/* Adjust the two end ponts of the first point. */
	firstStrokePoint->x = StrokeConversion->lastRightStrokePoint->x;
	firstStrokePoint->y = StrokeConversion->lastRightStrokePoint->y;
	lastStrokePoint->x = StrokeConversion->leftStrokePoint->x;
	lastStrokePoint->y = StrokeConversion->leftStrokePoint->y;

	/* Concatnate right and left point lists. */
	StrokeConversion->lastRightStrokePoint->next = StrokeConversion->leftStrokePoint;
	StrokeConversion->leftStrokePoint->prev = StrokeConversion->lastRightStrokePoint;

	/*gcmERROR_RETURN(_CheckStrokeSubPath(StrokeConversion->lastStrokeSubPath));*/

	return status;
}

static gceSTATUS
_CreateStrokePath_I(
	IN vgsSTROKECONVERSION_I_PTR StrokeConversion
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsSUBPATH_I_PTR subPath, strokeSubPath;
	vgsPOINT_I_PTR point, nextPoint;
	gctFIXED halfWidth = StrokeConversion->halfLineWidth;
	gctFIXED x, y;
	gctFIXED dx, dy, ux, uy;
	gctFIXED length, prevLength, firstLength;
	gctFIXED dashLength;
	gctINT dashIndex;
	gctBOOL dashing;
	gctBOOL addEndCap;
	gctBOOL needToHandleSwing = gcvTRUE /* (StrokeConversion->strokeCapStyle == gcvCAP_BUTT) */;

	dashing = StrokeConversion->strokeDashPatternCount > 0 ? gcvTRUE : gcvFALSE;
	dashIndex = StrokeConversion->strokeDashInitialIndex;
	dashLength = StrokeConversion->strokeDashInitialLength;

	for (subPath = StrokeConversion->subPathList; subPath; subPath = subPath->next)
	{
		vgsSUBPATH_I_PTR firstStrokeSubPath = gcvNULL;
		vgsPOINT_I_PTR firstRightPoint = gcvNULL;
		vgsPOINT_I_PTR lastLeftPoint = gcvNULL;
		gctFIXED firstDx = FIXED_ZERO, firstDy = FIXED_ZERO;
		gctBOOL drawing = gcvFALSE;
		gctFIXED totalLength = FIXED_ZERO;
		gctFIXED accuLength = FIXED_ZERO;
		gctUINT swingHandling = vgcSWING_NO;

		StrokeConversion->currentSubPath = subPath;

		/* TODO - Need to check/debug closed stroke path. */
		needToHandleSwing = (StrokeConversion->strokeCapStyle == gcvCAP_BUTT || subPath->closed);
		if (needToHandleSwing)
		{
			gctBOOL reallyNeedToHandleSwing = gcvFALSE;

			/* Calculate the total length. */
			for (point = subPath->pointList; point; point = point->next)
			{
				totalLength += point->length;
				if (point->flattenFlag != vgcFLATTEN_NO)
				{
					reallyNeedToHandleSwing = gcvTRUE;
				}
			}
			subPath->length = totalLength;
			if (reallyNeedToHandleSwing)
			{
				swingHandling = vgcSWING_OUT;
			}
			else
			{
				needToHandleSwing = gcvFALSE;
				swingHandling = vgcSWING_NO;
			}
		}
		StrokeConversion->swingNeedToHandle = needToHandleSwing;

		if (dashing && StrokeConversion->strokeDashPhaseReset)
		{
			dashIndex = StrokeConversion->strokeDashInitialIndex;
			dashLength = StrokeConversion->strokeDashInitialLength;
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
				gcmERROR_RETURN(_AddZeroLengthStrokeSubPath_I(StrokeConversion, point));
			}
			continue;
		}

		/* Adjust closed status for dashing. */
		if (dashing && subPath->closed && ((dashIndex & 0x1) == 1))
		{
			subPath->closed = gcvFALSE;
		}

		/* Set addEndCap. */
		addEndCap = subPath->closed ? gcvFALSE : gcvTRUE;

		/* Process first line. */
		firstLength = point->length;
		ux = point->tangentX;
		uy = point->tangentY;
		dx = FIXED_MUL(uy, halfWidth);
		dy = FIXED_MUL(-(ux), halfWidth);
		if (needToHandleSwing)
		{
			StrokeConversion->swingAccuLength = firstLength;
		}

		if (dashing)
		{
			gctFIXED deltaLength;

			/* Draw dashes. */
			x = point->x;
			y = point->y;
			do
			{
				if ((dashIndex & 0x1) == 0)
				{
					gcmERROR_RETURN(_StartANewStrokeSubPath_I(
						StrokeConversion,
						x, y,
						dx, dy, addEndCap,
						&strokeSubPath
						));

					drawing = gcvTRUE;
					addEndCap = gcvTRUE;
					if (subPath->closed && (firstStrokeSubPath == gcvNULL))
					{
						firstStrokeSubPath = StrokeConversion->lastStrokeSubPath;
						firstRightPoint = StrokeConversion->lastRightStrokePoint;
						lastLeftPoint = StrokeConversion->leftStrokePoint;
						gcmASSERT(lastLeftPoint->next == gcvNULL);
						firstDx = dx;
						firstDy = dy;
					}
				}

				deltaLength = firstLength - dashLength;
				if (deltaLength >= FIXED_EPSILON)
				{
					/* Move (x, y) forward along the line by dashLength. */
					x += FIXED_MUL(ux, dashLength);
					y += FIXED_MUL(uy, dashLength);

					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(drawing);
						gcmERROR_RETURN(_EndAStrokeSubPath_I(
							StrokeConversion,
							x, y,
							dx, dy
							));

						drawing = gcvFALSE;
					}

					_GetNextDashLength_I(StrokeConversion, &dashIndex, &dashLength);
					firstLength = deltaLength;
				}
				else if (deltaLength <= -FIXED_EPSILON)
				{
					dashLength = -deltaLength;
					break;
				}
				else
				{
					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(drawing);
						gcmERROR_RETURN(_EndAStrokeSubPath_I(
							StrokeConversion,
							nextPoint->x, nextPoint->y,
							dx, dy
							));

						drawing = gcvFALSE;
					}

					_GetNextDashLength_I(StrokeConversion, &dashIndex, &dashLength);
					firstLength = 0;
					break;
				}
			}
			while (gcvTRUE);
		}
		else
		{
			gcmERROR_RETURN(_StartANewStrokeSubPath_I(
				StrokeConversion,
				point->x, point->y,
				dx, dy, addEndCap,
				&strokeSubPath
				));

			drawing = gcvTRUE;
			addEndCap = gcvTRUE;
		}

		/* Process the rest of lines. */
		prevLength = firstLength;
		for (point = nextPoint, nextPoint = point->next; nextPoint;
			 point = nextPoint, nextPoint = point->next)
		{
			if (!dashing || ((dashIndex & 0x1) == 0 && drawing))
			{
				/* Add points for end of line for line join process with next line. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
										point->x + dx, point->y + dy));
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
										point->x - dx, point->y - dy));
			}

			length = point->length;
			ux = point->tangentX;
			uy = point->tangentY;
			dx = FIXED_MUL(uy, halfWidth);
			dy = FIXED_MUL(-(ux), halfWidth);
			if (needToHandleSwing)
			{
				accuLength += point->prev->length;
				if (!dashing)
				{
					gcmASSERT(StrokeConversion->swingAccuLength == accuLength);
				}
				else
				{
					StrokeConversion->swingAccuLength = accuLength;
				}
				if (accuLength < halfWidth)
				{
					swingHandling = vgcSWING_OUT;
				}
				else if (totalLength - accuLength < halfWidth)
				{
					swingHandling = vgcSWING_IN;
				}
				else
				{
					swingHandling = vgcSWING_NO;
				}
			}

			if (!dashing)
			{
				/* Handle line joint style. */
				gcmERROR_RETURN(_ProcessLineJoint_I(
					StrokeConversion, point,
					length, prevLength, swingHandling,
					point->x + dx, point->y + dy,
					point->x - dx, point->y - dy
					));
			}
			else
			{
				gctFIXED deltaLength;

				/* Draw dashes. */
				x = point->x;
				y = point->y;
				if ((dashIndex & 0x1) == 0)
				{
					if (drawing)
					{
						/* Handle line joint style. */
						gcmERROR_RETURN(_ProcessLineJoint_I(
							StrokeConversion, point,
							dashLength, prevLength, swingHandling,
							x + dx, y + dy,
							x - dx, y - dy
							));
					}
					else
					{
						/* Start a new sub path. */
						gcmASSERT(addEndCap);
						gcmERROR_RETURN(_StartANewStrokeSubPath_I(
							StrokeConversion,
							x, y,
							dx, dy, addEndCap,
							&strokeSubPath
							));

						drawing = gcvTRUE;
						addEndCap = gcvTRUE;
					}
				}
				do
				{
					deltaLength = length - dashLength;
					if (deltaLength >= FIXED_EPSILON)
					{
						/* Move (x, y) forward along the line by dashLength. */
						x += FIXED_MUL(ux, dashLength);
						y += FIXED_MUL(uy, dashLength);

						if ((dashIndex & 0x1) == 0)
						{
							gcmASSERT(drawing);
							gcmERROR_RETURN(_EndAStrokeSubPath_I(
								StrokeConversion,
								x, y, dx, dy
								));

							drawing = gcvFALSE;
						}

						_GetNextDashLength_I(StrokeConversion, &dashIndex, &dashLength);
						length = deltaLength;
					}
					else if (deltaLength <= -FIXED_EPSILON)
					{
						dashLength = -deltaLength;
						break;
					}
					else
					{
						if ((dashIndex & 0x1) == 0)
						{
							gcmASSERT(drawing);
							gcmERROR_RETURN(_EndAStrokeSubPath_I(
								StrokeConversion,
								nextPoint->x, nextPoint->y,
								dx, dy
								));

							drawing = gcvFALSE;
						}

						_GetNextDashLength_I(StrokeConversion, &dashIndex, &dashLength);
						length = 0;
						break;
					}

					if ((dashIndex & 0x1) == 0)
					{
						gcmASSERT(addEndCap);
						gcmERROR_RETURN(_StartANewStrokeSubPath_I(
							StrokeConversion,
							x, y,
							dx, dy, addEndCap,
							&strokeSubPath
							));

						drawing = gcvTRUE;
						addEndCap = gcvTRUE;
					}
				}
				while (gcvTRUE);
			}

			prevLength = length;
		}

		if (needToHandleSwing)
		{
			accuLength += point->prev->length;
			if (!dashing)
			{
				gcmASSERT(StrokeConversion->swingAccuLength == accuLength);
			}
			else
			{
				StrokeConversion->swingAccuLength = accuLength;
			}
			if (accuLength < halfWidth)
			{
				swingHandling = vgcSWING_OUT;
			}
			else if (totalLength - accuLength < halfWidth)
			{
				swingHandling = vgcSWING_IN;
			}
			else
			{
				swingHandling = vgcSWING_NO;
			}
		}

		if (StrokeConversion->swingHandling != vgcSWING_NO)
		{
			gcmASSERT(StrokeConversion->swingHandling == vgcSWING_NO);
			/* Draw the swing area (pie area). */
			gcmERROR_RETURN(_DrawSwingPieArea_I(StrokeConversion, subPath->lastPoint, gcvFALSE));
		}

		if (subPath->closed)
		{
			if (! dashing || drawing)
			{
				/* Add points for end of line. */
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
										point->x + dx, point->y + dy));
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
										point->x - dx, point->y - dy));

				if (! dashing)
				{
					/* Handle line joint style for the first/last point in closed path. */
					gcmERROR_RETURN(_CloseAStrokeSubPath_I(
						StrokeConversion, point,
						firstLength, prevLength, swingHandling,
						strokeSubPath->pointList, strokeSubPath->lastPoint
						));
				}
				else
				{
					/* Handle line joint style for the first/last point in closed path. */
					gcmERROR_RETURN(_CloseAStrokeSubPath_I(
						StrokeConversion, point,
						firstLength, prevLength, swingHandling,
						firstRightPoint, lastLeftPoint
						));
				}
			}
			else if (StrokeConversion->strokeCapStyle != gcvCAP_BUTT)
			{
				/* No closing join need.  Add end cap for the starting point. */

				if (StrokeConversion->strokeCapStyle == gcvCAP_SQUARE)
				{
					firstRightPoint->x += firstDy;
					firstRightPoint->y -= firstDx;
					lastLeftPoint->x += firstDy;
					lastLeftPoint->y -= firstDx;
				}
				else
				{
					vgsSUBPATH_I_PTR lastStrokeSubPath = StrokeConversion->lastStrokeSubPath;
					vgsPOINT_I_PTR startPoint = lastStrokeSubPath->pointList;
					vgsPOINT_I_PTR point;

					/* Add curve. */
					/* Add extra point to the beginning with end point's coordinates. */
					gcmERROR_RETURN(_AllocatePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR *) &point));
					point->x = lastStrokeSubPath->lastPoint->x;
					point->y = lastStrokeSubPath->lastPoint->y;
					point->next = startPoint;
					startPoint->prev = point;
					startPoint->curveType = vgcCURVE_ARC_SCCW;
					startPoint->centerX = subPath->pointList->x;
					startPoint->centerY = subPath->pointList->y;
					lastStrokeSubPath->pointList = point;
				}
			}
		}
		else if (! dashing ||
				 (((dashIndex & 0x1) == 0) && (dashLength < StrokeConversion->strokeDashPattern[dashIndex])))
		{
			/* Add end cap if the subPath is not closed. */
			gcmERROR_RETURN(_EndAStrokeSubPath_I(
				StrokeConversion,
				point->x, point->y,
				dx, dy
				));

			drawing = gcvFALSE;
		}
	}

	return status;
}

#if gcvUSE_FAST_STROKE
static void
_FastSetPointTangent_I(
	vgsPOINT_I_PTR Point
	)
{
	vgsPOINT_I_PTR nextPoint = Point->next;
	gctFIXED Dx, Dy;

	if (nextPoint)
	{
		Dx = nextPoint->x - Point->x;
		Dy = nextPoint->y - Point->y;
	}
	else
	{
		Dx = Point->x - Point->prev->x;
		Dy = Point->y - Point->prev->y;
	}

	if (Dx == FIXED_ZERO)
	{
		if (Dy == FIXED_ZERO)
		{
			gcmASSERT(Dx || Dy);
			if (Point->prev)
			{
				Point->tangentX = Point->prev->tangentX;
				Point->tangentY = Point->prev->tangentY;
			}
			else
			{
				gcmASSERT(Point->prev);
				Point->tangentX = FIXED_ZERO;
				Point->tangentY = FIXED_ZERO;
			}
		}
		else
		{
			Point->tangentX = FIXED_ZERO;
			if (Dy > FIXED_ZERO)
			{
				Point->tangentY = FIXED_ONE;
			}
			else
			{
				Point->tangentY = -FIXED_ONE;
			}
		}
	}
	else if (Dy == FIXED_ZERO)
	{
		Point->tangentY = FIXED_ZERO;
		if (Dx > FIXED_ZERO)
		{
			Point->tangentX = FIXED_ONE;
		}
		else
		{
			Point->tangentX = -FIXED_ONE;
		}
	}
	else
	{
		gctFIXED l, tx, ty;
		gctFIXED dx, dy;
		gctFIXED t, t2;

		dx = (Dx >= FIXED_ZERO ? Dx : -Dx);
		dy = (Dy >= FIXED_ZERO ? Dy : -Dy);
		if (dx > dy)
		{
			t = FIXED_DIV(dy, dx);
			/* No need to use 64-bit. */
			t2 = (gctFIXED) (((gctUINT) t * (gctUINT) t) >> 16);
			l = _FastSqrt_I(t2);

			tx = FIXED_DIV(FIXED_ONE, l);
			/* No need to use 64-bit. */
			ty = (gctFIXED) (((gctUINT) tx * (gctUINT) t) >> 16);
		}
		else if (dx < dy)
		{
			t = FIXED_DIV(dx, dy);
			/* No need to use 64-bit. */
			t2 = (gctFIXED) (((gctUINT) t * (gctUINT) t) >> 16);
			l = _FastSqrt_I(t2);

			ty = FIXED_DIV(FIXED_ONE, l);
			/* No need to use 64-bit. */
			tx = (gctFIXED) (((gctUINT) ty * (gctUINT) t) >> 16);
		}
		else
		{
			tx = 0x0000b505;
			ty = 0x0000b505;
		}
		if (Dx < FIXED_ZERO) tx = -tx;
		if (Dy < FIXED_ZERO) ty = -ty;
		tx = gcmCLAMP(tx, -FIXED_ONE, FIXED_ONE);
		ty = gcmCLAMP(ty, -FIXED_ONE, FIXED_ONE);
		Point->tangentX = tx;
		Point->tangentY = ty;
	}
}

#if vgvSTROKE_COPY_OPTIMIZE
#define VGSL_SETCOORD_IF(X, Y) \
		coordinates = (gctINT_PTR) stroke.coordinate; \
		coordinates[0] = X; \
		coordinates[1] = Y
#else
#define VGSL_SETCOORD_IF(X, Y) \
		stroke.set(&stroke, FIXED_TO_FLOAT(X)); \
		stroke.set(&stroke, FIXED_TO_FLOAT(Y))
#endif

/* Fast mode for non-dash, non-fat stroke paths. */
static gceSTATUS
_FastCreateStrokePath_I(
	IN vgsCONTEXT_PTR Context,
	IN vgsSTROKECONVERSION_I_PTR StrokeConversion,
	IN vgsPATH_PTR Path
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsSUBPATH_I_PTR subPath, strokeSubPath;
	vgsPOINT_I_PTR point, nextPoint;
	/* Use eighthWidth to make sure no overflow in 32-bit multiplication. */
	gctFIXED eighthWidth = StrokeConversion->halfLineWidth >> 2;
	gctFIXED dx, dy;
#if gcvUSE_FAST_COPY
	vgsPATHWALKER stroke;
	vgsPATH_DATA_PTR strokePath;
#if vgvSTROKE_COPY_OPTIMIZE
	gctINT_PTR coordinates;
#endif

	/* Validate the storage. */
	vgmVALIDATE_STORAGE(Context->strokeStorage);

	/* Prepare for writing the destination. */
	vgsPATHWALKER_InitializeWriter(
		Context, Context->strokeStorage, &stroke, Path
		);

	/* Allocate a new subpath. */
#if vgvSTROKE_COPY_OPTIMIZE
	gcmERROR_RETURN(vgsPATHWALKER_StartSubpath(
		&stroke, vgmCOMMANDSIZE(2, gctUINT32), gcePATHTYPE_INT32
		));
#else
	gcmERROR_RETURN(vgsPATHWALKER_StartSubpath(
		&stroke, vgmCOMMANDSIZE(2, gctFLOAT), gcePATHTYPE_FLOAT
		));
#endif

	/* Add to the MRU list. */
	vgsPATHSTORAGE_UpdateMRU(Context->strokeStorage, stroke.tail);
#endif

	gcmASSERT(StrokeConversion->strokeDashPatternCount == 0);

	for (subPath = StrokeConversion->subPathList; subPath; subPath = subPath->next)
	{
#if gcvCALCULATE_HALF_TANGENT
		gctINT count = 0;
#endif

		StrokeConversion->currentSubPath = subPath;
		point = subPath->pointList;
		gcmASSERT(point != gcvNULL);

		nextPoint = point->next;
		if (nextPoint == gcvNULL)
		{
			/* Single point (zero-length) subpath. */
			/* Note that one-MOVE_TO subpaths are removed during parsing. */
			gcmERROR_RETURN(_AddZeroLengthStrokeSubPath_I(StrokeConversion, point));

#if gcvUSE_FAST_COPY
			if (StrokeConversion->strokeCapStyle != gcvCAP_BUTT)
			{
				/* Add new command. */
				gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
					&stroke, gcvVGCMD_MOVE
					));

				point = StrokeConversion->lastStrokeSubPath->pointList;
				VGSL_SETCOORD_IF(point->x, point->y);

				/* Jump to endOfSubPath to output the rest of points. */
				point = point->next;
				goto endOfSubPath;
			}
#endif
			continue;
		}

		/* Process first line. */
		_FastSetPointTangent_I(point);
		/* No need to use 64-bit. */
		dx = (  point->tangentY  * eighthWidth) >> 14;
		dy = (-(point->tangentX) * eighthWidth) >> 14;

		gcmERROR_RETURN(_StartANewStrokeSubPath_I(
			StrokeConversion,
			point->x, point->y,
			dx, dy, gcvTRUE,
			&strokeSubPath
			));

#if gcvUSE_FAST_COPY
		/* Add new command. */
		gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
			&stroke, gcvVGCMD_MOVE
			));

		point = strokeSubPath->pointList;
		VGSL_SETCOORD_IF(point->x, point->y);
#endif

		/* Process the rest of lines. */
		for (point = nextPoint, nextPoint = point->next; nextPoint;
			 point = nextPoint, nextPoint = point->next)
		{
#if gcvCALCULATE_HALF_TANGENT
			count++;
			if (count & 0x1)
			{
				/* Add points for end of line for line join process with next line. */
#if gcvUSE_FAST_COPY
				/* Add new command. */
				gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
					&stroke, gcvVGCMD_LINE
					));

				VGSL_SETCOORD_IF(point->x + dx, point->y + dy);
#else
				gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
									point->x + dx, point->y + dy));
#endif
				gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
									point->x - dx, point->y - dy));

				/* Skip the next line. */
				continue;
			}
#else
#if gcvUSE_FAST_COPY
			/* Add new command. */
			gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
				&stroke, gcvVGCMD_LINE
				));

			VGSL_SETCOORD_IF(point->x + dx, point->y + dy);
#else
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
								point->x + dx, point->y + dy));
#endif
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
								point->x - dx, point->y - dy));
#endif

			_FastSetPointTangent_I(point);
			/* No need to use 64-bit. */
			dx = (  point->tangentY  * eighthWidth) >> 14;
			dy = (-(point->tangentX) * eighthWidth) >> 14;

			/* Add points for beginning of the next line--bevel joint. */
#if gcvUSE_FAST_COPY
			/* Add new command. */
			gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
				&stroke, gcvVGCMD_LINE
				));

			VGSL_SETCOORD_IF(point->x + dx, point->y + dy);

			VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);
#else
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
									point->x + dx, point->y + dy));
#endif
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
									point->x - dx, point->y - dy));
		}

#if gcvCALCULATE_HALF_TANGENT
		if ((count & 0x1) == 0)
		{
			_FastSetPointTangent_I(point);
			/* No need to use 64-bit. */
			dx = (  point->tangentY  * eighthWidth) >> 14;
			dy = (-(point->tangentX) * eighthWidth) >> 14;
		}
#endif

		/* Add end cap if the subPath is not closed. */
		gcmERROR_RETURN(_EndAStrokeSubPath_I(
			StrokeConversion,
			point->x, point->y,
			dx, dy
			));

#if gcvUSE_FAST_COPY
		/* Copy the second half of the stroke subPath. */
		/* Only VG_LINE_TO commands in stroke subPath. */
		/*
		gcmERROR_RETURN(_CopyStrokeSubPath_I(
			StrokeConversion,
			&stroke, strokeSubPath
			));
		*/
		/* The first point already output as MOVE command. */
		point = strokeSubPath->pointList->next;

endOfSubPath:
		for (; point; point = point->next)
		{
			/* Add new command. */
			gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
				&stroke, gcvVGCMD_LINE
				));

			VGSL_SETCOORD_IF(point->x, point->y);

			VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);
		}

		/* Create a CLOSE_PATH command at the end. */
		gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
			&stroke, gcvVGCMD_CLOSE
			));
#endif
	}

#if gcvUSE_FAST_COPY
	/* Close the path. */
	vgsPATHWALKER_CloseSubpath(&stroke);

	/* Get the shortcut to the path. */
	strokePath = stroke.head;
	if (strokePath != gcvNULL)
	{
		/* Finalize the path structure. */
		gcmERROR_RETURN(gcoVG_FinalizePath(Context->vg, &strokePath->data));

		/* Set the result. */
		gcmASSERT(Path->stroke == gcvNULL);
		Path->stroke = strokePath;

		/* Validate the storage. */
		vgmVALIDATE_STORAGE(Context->strokeStorage);
	}
#endif

	return status;
}
#endif

#if gcvUSE_FAST_FAT_LINE_HANDLE
static gceSTATUS
_FastProcessLineJoint_I(
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	vgsPOINT_I_PTR Point,
	gctFIXED X1,
	gctFIXED Y1,
	gctFIXED X2,
	gctFIXED Y2
	)
{
	gceSTATUS status;
	gceJOIN_STYLE strokeJoinStyle;
	gctFIXED ratio;
	gctFIXED cosTheta;
	gctBOOL counterClockwise;

	/* First, determine the turn is clockwise or counter-clockwise. */
	cosTheta = ((gctINT64) Point->prev->tangentX * Point->tangentX + (gctINT64) Point->prev->tangentY * Point->tangentY) >> 16;

	if (cosTheta >= FIXED_MIN_ARC_ANGLE_COS)
	{
		/* Straight line or semi-straight line--no need to handle join. */
		/* Add the new stroke points. */
		gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
		gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));

		VGSL_STAT_COUNTER_INCREASE(vgStrokeFilteredJointsByAngleEpsilonCount);
		return gcvSTATUS_OK;
	}

	/* For flattened curves/arcs, the join style is always round. */
	if (Point->flattenFlag != vgcFLATTEN_NO)
	{
		strokeJoinStyle = gcvJOIN_ROUND;
	}
	else
	{
		strokeJoinStyle = StrokeConversion->strokeJoinStyle;
	}

	ratio = FIXED_ZERO;
	if (cosTheta <= -FIXED_ANGLE_EPSILON_COS)
	{
		/* Almost 180 degree turn. */
		counterClockwise = gcvTRUE;
		ratio = FIXED_MAX;
	}
	else
	{
		gctINT64 angleSign = (gctINT64) Point->prev->tangentX * Point->tangentY - (gctINT64) Point->prev->tangentY * Point->tangentX;
		counterClockwise = (angleSign >= 0 ? gcvTRUE : gcvFALSE);
		if (strokeJoinStyle == gcvJOIN_MITER)
		{
			ratio = FIXED_DIV(FIXED_TWO, (FIXED_ONE + cosTheta));
			gcmASSERT(ratio > FIXED_ONE);
		}
	}

	if (counterClockwise)
	{
		/* TODO - Check if the turn is more than 90 degree. */
		if (Point->flattenFlag == vgcFLATTEN_NO)
		{
			/* Add the point to avoid incorrect sharp angle. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, Point->x, Point->y));
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
		}
		else
		{
			/* Flattened line segments should not have sharp angle. */
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
		}

		switch (strokeJoinStyle)
		{
		case gcvJOIN_ROUND:
			/* Add curve. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
			StrokeConversion->lastRightStrokePoint->curveType = vgcCURVE_ARC_SCCW;
			StrokeConversion->lastRightStrokePoint->centerX = Point->x;
			StrokeConversion->lastRightStrokePoint->centerY = Point->y;
			break;
		case gcvJOIN_MITER:
			if (ratio <= StrokeConversion->strokeMiterLimitSquare)
			{
				/* Adjust lastRightStrokePoint to the outer intersection point. */
				_AdjustJointPoint_I(Point, StrokeConversion->lastRightStrokePoint, X1, Y1, ratio);
				break;
			}
			/* Else use Bevel join style. */
		case gcvJOIN_BEVEL:
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
			break;
		}
	}
	else
	{
		/* TODO - Check if the turn is more than 90 degree. */
		if (Point->flattenFlag == vgcFLATTEN_NO)
		{
			/* Add the point to avoid incorrect sharp angle. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, Point->x, Point->y));
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
		}
		else
		{
			/* Flattened line segments should not have sharp angle. */
			/* Add the point to form a loop to avoid out-of-bound problem. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion, X1, Y1));
		}

		switch (strokeJoinStyle)
		{
		case gcvJOIN_ROUND:
			/* Add curve. */
			StrokeConversion->leftStrokePoint->curveType = vgcCURVE_ARC_SCCW;
			StrokeConversion->leftStrokePoint->centerX = Point->x;
			StrokeConversion->leftStrokePoint->centerY = Point->y;
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
			break;
		case gcvJOIN_MITER:
			if (ratio <= StrokeConversion->strokeMiterLimitSquare)
			{
				/* Adjust leftStrokePoint to the outer intersection point. */
				_AdjustJointPoint_I(Point, StrokeConversion->leftStrokePoint, X2, Y2, ratio);
				break;
			}
			/* Else use Bevel join style. */
		case gcvJOIN_BEVEL:
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion, X2, Y2));
			break;
		}
	}

	return gcvSTATUS_OK;
}

static gceSTATUS
_CopyStrokeSubPath_I(
	IN vgsSTROKECONVERSION_I_PTR StrokeConversion,
	IN vgsPATHWALKER_PTR Stroke,
	IN vgsSUBPATH_I_PTR SubPath
	);

/* Fast mode for non-dash, non-butt-endcap fat stroke paths. */
static gceSTATUS
_FastCreateFatStrokePath_I(
	IN vgsCONTEXT_PTR Context,
	IN vgsSTROKECONVERSION_I_PTR StrokeConversion,
	IN vgsPATH_PTR Path
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsSUBPATH_I_PTR subPath, strokeSubPath;
	vgsPOINT_I_PTR point, nextPoint;
	gctFIXED halfWidth = StrokeConversion->halfLineWidth;
	/* Use eighthWidth to make sure no overflow in 32-bit multiplication. */
	/*gctFIXED eighthWidth = StrokeConversion->halfLineWidth >> 2;*/
	gctFIXED dx, dy;
	gctBOOL addEndCap;

	gcmASSERT(StrokeConversion->strokeDashPatternCount == 0);
	gcmASSERT(StrokeConversion->strokeCapStyle != gcvCAP_BUTT);

	for (subPath = StrokeConversion->subPathList; subPath; subPath = subPath->next)
	{
		StrokeConversion->currentSubPath = subPath;
		point = subPath->pointList;
		gcmASSERT(point != gcvNULL);

		nextPoint = point->next;
		if (nextPoint == gcvNULL)
		{
			/* Single point (zero-length) subpath. */
			/* Note that one-MOVE_TO subpaths are removed during parsing. */
			gcmERROR_RETURN(_AddZeroLengthStrokeSubPath_I(StrokeConversion, point));
			continue;
		}

		/* Set addEndCap. */
		addEndCap = subPath->closed ? gcvFALSE : gcvTRUE;

		/* Process first line. */
		dx = FIXED_MUL(point->tangentY, halfWidth);
		dy = FIXED_MUL(-(point->tangentX), halfWidth);

		gcmERROR_RETURN(_StartANewStrokeSubPath_I(
			StrokeConversion,
			point->x, point->y,
			dx, dy, addEndCap,
			&strokeSubPath
			));

		addEndCap = gcvTRUE;


		/* Process the rest of lines. */
		for (point = nextPoint, nextPoint = point->next; nextPoint;
			 point = nextPoint, nextPoint = point->next)
		{
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
								point->x + dx, point->y + dy));
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
								point->x - dx, point->y - dy));

			dx = FIXED_MUL(point->tangentY, halfWidth);
			dy = FIXED_MUL(-(point->tangentX), halfWidth);

			/* Handle line joint style. */
			gcmERROR_RETURN(_FastProcessLineJoint_I(
				StrokeConversion, point,
				point->x + dx, point->y + dy,
				point->x - dx, point->y - dy
				));
		}

		if (subPath->closed)
		{
			/* Add points for end of line. */
			gcmERROR_RETURN(_AddAPointToRightStrokePointListTail_I(StrokeConversion,
									point->x + dx, point->y + dy));
			gcmERROR_RETURN(_AddAPointToLeftStrokePointListHead_I(StrokeConversion,
									point->x - dx, point->y - dy));

			/* Handle line joint style for the first/last point in closed path. */
			/* Handle line joint style for the first/last point in closed path. */
			gcmERROR_RETURN(_FastProcessLineJoint_I(
				StrokeConversion, point,
				strokeSubPath->pointList->x, strokeSubPath->pointList->y,
				strokeSubPath->lastPoint->x, strokeSubPath->lastPoint->y
				));

			/* Adjust the two end ponts of the first point. */
			strokeSubPath->pointList->x = StrokeConversion->lastRightStrokePoint->x;
			strokeSubPath->pointList->y = StrokeConversion->lastRightStrokePoint->y;
			strokeSubPath->lastPoint->x = StrokeConversion->leftStrokePoint->x;
			strokeSubPath->lastPoint->y = StrokeConversion->leftStrokePoint->y;

			/* Concatnate right and left point lists. */
			StrokeConversion->lastRightStrokePoint->next = StrokeConversion->leftStrokePoint;
			StrokeConversion->leftStrokePoint->prev = StrokeConversion->lastRightStrokePoint;

			/*gcmERROR_RETURN(_CheckStrokeSubPath(StrokeConversion->lastStrokeSubPath));*/
		}
		else
		{
			/* Add end cap if the subPath is not closed. */
			gcmERROR_RETURN(_EndAStrokeSubPath_I(
				StrokeConversion,
				point->x, point->y,
				dx, dy
				));
		}

	}


	return status;
}
#endif

#if OUTPUT_STROKE_PATH || OUTPUT_FLATTENED_PATH
static gceSTATUS
_OutputPath_I(
	vgsCONTEXT_PTR Context,
	vgsSTROKECONVERSION_I_PTR StrokeConversion,
	vgsSUBPATH_I_PTR SubPathList
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	gcoOS os = StrokeConversion->os;
	gctFILE logFile = gcvNULL;
	gctINT segmentCount = 0;
	gctINT pointCount = 0;
	gctFIXED scale = FIXED_DIV(FIXED_ONE, StrokeConversion->pathScale);
	vgsSUBPATH_I_PTR subPath = gcvNULL;
	vgsPOINT_I_PTR point = gcvNULL;
	gctINT8 realBuffer[256];
	gctSTRING buffer = (gctSTRING) realBuffer;
	gctUINT offset = 0;

	/*gcmERROR_RETURN(gcoOS_Open(os, PATH_LOG_FILENAME, gcvFILE_APPENDTEXT, &logFile));*/
	gcmERROR_RETURN(gcoOS_Open(os, PATH_LOG_FILENAME, gcvFILE_CREATETEXT, &logFile));

	scale = FIXED_ONE;

	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		segmentCount += subPath->pointCount + 1;
		pointCount += subPath->pointCount;
	}

#if OUTPUT_STROKE_PATH_INCLUDE_CMD
#define VGSL_WRITECOMMAND(COMMAND) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStr(buffer, &offset, "%s\t", COMMAND)); \
	gcoOS_Write(os, logFile, offset, buffer); \
	segmentCount--
#define VGSL_WRITECOMMAND1(COMMAND) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStr(buffer, &offset, "%s\n", COMMAND)); \
	gcoOS_Write(os, logFile, offset, buffer); \
	segmentCount--
#else
#define VGSL_WRITECOMMAND(COMMAND)		segmentCount--
#define VGSL_WRITECOMMAND1(COMMAND)		segmentCount--
#endif

#define VGSL_WRITEPOINT_I(POINT) \
	offset = 0; \
	gcmVERIFY_OK(gcoOS_PrintStr(buffer, &offset, "%12.6f\t%12.6f\n", \
				FIXED_TO_FLOAT(FIXED_MUL(POINT->x, scale)), \
				FIXED_TO_FLOAT(FIXED_MUL(POINT->y, scale)))); \
	gcoOS_Write(os, logFile, offset, buffer); \
	pointCount--

	/* Output path with reversed scale. */
	for (subPath = SubPathList; subPath; subPath = subPath->next)
	{
		/* Create a MOVE_TO command for the first point. */
		point = subPath->pointList;
		gcmASSERT(point);
		VGSL_WRITECOMMAND("VG_MOVE_TO");
		VGSL_WRITEPOINT_I(point);

		/* Create a LINE_TO command for the rest of points. */
		for (point = point->next; point; point = point->next)
		{
			VGSL_WRITECOMMAND("VG_LINE_TO");
			VGSL_WRITEPOINT_I(point);
		}

		/* Create a CLOSE_PATH command at the end. */
		VGSL_WRITECOMMAND1("VG_CLOSE_PATH");
	}
	gcmASSERT(segmentCount == 0);
	gcmASSERT(pointCount == 0);

	gcmVERIFY_OK(gcoOS_Close(os, logFile));

	return status;
}
#endif

/******************************************************************************\
|********************** Copy Stroke Path to Path Buffer ***********************|
|******************************************************************************/
#if vgvSTROKE_COPY_OPTIMIZE
#define VGSL_SETCOORD(X, Y) \
		coordinates = (gctFLOAT_PTR) Stroke->coordinate; \
		coordinates[0] = X; \
		coordinates[1] = Y
#else
#define VGSL_SETCOORD(X, Y) \
		Stroke->set(Stroke, X); \
		Stroke->set(Stroke, Y)
#endif

static gceSTATUS
_CopyStrokeSubPath(
	IN vgsSTROKECONVERSION_PTR StrokeConversion,
	IN vgsPATHWALKER_PTR Stroke,
	IN vgsSUBPATH_PTR SubPath
	)
{
	gceSTATUS status;
	gctFLOAT halfWidth = StrokeConversion->halfLineWidth;
	vgsSUBPATH_PTR subPath = SubPath;
	vgsPOINT_PTR point;
	vgsPOINT_PTR prevPoint;

#if vgvSTROKE_COPY_OPTIMIZE
	gctFLOAT_PTR coordinates;
#endif

	/* Copy the stroke to the final buffer. */
	while (subPath)
	{
		/* Create a MOVE_TO command for the first point. */
		prevPoint = point = subPath->pointList;
		gcmASSERT(point);

		/* Add new command. */
		gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
			Stroke, gcvVGCMD_MOVE
			));

		VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

		/* Set the coordinates. */
		VGSL_SETCOORD(point->x, point->y);

		/* Create a LINE_TO or QUAD_TO command for the rest of points. */
		for (point = point->next; point; prevPoint = point, point = point->next)
		{
			if (point->curveType == vgcCURVE_LINE)
			{
				if (point->x == prevPoint->x && point->y == prevPoint->y)
				{
					/* Skip zero-length lines. */
					continue;
				}

				/* Add new command. */
				gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
					Stroke, gcvVGCMD_LINE
					));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

				/* Set the coordinates. */
				VGSL_SETCOORD(point->x, point->y);
			}
			else if (point->curveType == vgcCURVE_QUAD_CONTROL)
			{
				/* Add new command. */
				gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
					Stroke, gcvVGCMD_ARC_QUAD
					));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

				/* Set the coordinates. */
#if vgvSTROKE_COPY_OPTIMIZE
				prevPoint = point, point = point->next;
				coordinates = (gctFLOAT_PTR) Stroke->coordinate;
				coordinates[0] = prevPoint->x;
				coordinates[1] = prevPoint->y;
				coordinates[2] = point->x;
				coordinates[3] = point->y;
#else
				/* Set the control coordinates. */
				gcmASSERT(point->curveType == vgcCURVE_QUAD_CONTROL);
				Stroke->set(Stroke, point->x);
				Stroke->set(Stroke, point->y);

				/* Set the anchor coordinates. */
				prevPoint = point, point = point->next;
				gcmASSERT(point);
				gcmASSERT(point->curveType == vgcCURVE_QUAD_ANCHOR);
				Stroke->set(Stroke, point->x);
				Stroke->set(Stroke, point->y);
#endif
			}
			else
			{
				vgsPOINT_PTR pointList, p, nextP;

#if vgvSTROKE_COPY_OPTIMIZE
				vgsPOINT_PTR p2;
#endif

				if (point->curveType == vgcCURVE_ARC_SCCW)
				{
					/* Convert an arc to Bezier curves. */
					gcmERROR_RETURN(_ConvertCircleArc(StrokeConversion, halfWidth,
								point->centerX, point->centerY,
								prevPoint->x, prevPoint->y,
								point->x, point->y,
								gcvFALSE, &pointList));
				}
				else
				{
					/* Convert a half circle to Bezier curves. */
					gcmASSERT(point->curveType == vgcCURVE_ARC_SCCW_HALF);
					gcmERROR_RETURN(_ConvertCircleArc(StrokeConversion, halfWidth,
								point->centerX, point->centerY,
								prevPoint->x, prevPoint->y,
								point->x, point->y,
								gcvTRUE, &pointList));
				}

				VGSL_STAT_COUNTER_INCREASE(vgStrokeArcCount);

				if (pointList)
				{
					for (p = pointList; p; p = nextP)
					{
						/* Add new command. */
						gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
							Stroke, gcvVGCMD_ARC_QUAD
							));

						VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

						/* Set the coordinates. */
#if vgvSTROKE_COPY_OPTIMIZE
						p2 = p->next;
						nextP = p2->next;
						coordinates = (gctFLOAT_PTR) Stroke->coordinate;
						coordinates[0] = p->x;
						coordinates[1] = p->y;
						coordinates[2] = p2->x;
						coordinates[3] = p2->y;
						_FreePoint(StrokeConversion->pointMemPool, p);
						_FreePoint(StrokeConversion->pointMemPool, p2);
#else
						/* Set the control coordinates. */
						gcmASSERT(p->curveType == vgcCURVE_QUAD_CONTROL);
						Stroke->set(Stroke, p->x);
						Stroke->set(Stroke, p->y);

						/* Free the point. */
						nextP = p->next;
						_FreePoint(StrokeConversion->pointMemPool, p);

						/* Set the anchor coordinates. */
						p = nextP;
						gcmASSERT(p);
						gcmASSERT(p->curveType == vgcCURVE_QUAD_ANCHOR);
						Stroke->set(Stroke, p->x);
						Stroke->set(Stroke, p->y);

						/* Free the point. */
						nextP = p->next;
						_FreePoint(StrokeConversion->pointMemPool, p);
#endif
					}
				}
				else
				{
					/* Handle special case of huge scaling. */
					/* Add new command. */
					gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
						Stroke, gcvVGCMD_LINE
						));

					VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

					/* Set the coordinates. */
					VGSL_SETCOORD(point->x, point->y);
				}
			}
		}

		/* Create a CLOSE_PATH command at the end. */
		gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
			Stroke, gcvVGCMD_CLOSE
			));

		VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

		/* Advance to the next subpath. */
		subPath = subPath->next;
	}

	return gcvSTATUS_OK;
}

#if vgvSTROKE_COPY_OPTIMIZE
#define VGSL_SETCOORD_I(X, Y) \
		coordinates = (gctINT_PTR) Stroke->coordinate; \
		coordinates[0] = X; \
		coordinates[1] = Y
#else
#define VGSL_SETCOORD_I(X, Y) \
		Stroke->set(Stroke, FIXED_TO_FLOAT(X)); \
		Stroke->set(Stroke, FIXED_TO_FLOAT(Y))
#endif

static gceSTATUS
_CopyStrokeSubPath_I(
	IN vgsSTROKECONVERSION_I_PTR StrokeConversion,
	IN vgsPATHWALKER_PTR Stroke,
	IN vgsSUBPATH_I_PTR SubPath
	)
{
	gceSTATUS status;
	gctFIXED halfWidth = StrokeConversion->halfLineWidth;
	vgsSUBPATH_I_PTR subPath = SubPath;
	vgsPOINT_I_PTR point;
	vgsPOINT_I_PTR prevPoint;

#if vgvSTROKE_COPY_OPTIMIZE
	gctINT_PTR coordinates;
#endif

	/* Copy the stroke to the final buffer. */
	while (subPath)
	{
		/* Create a MOVE_TO command for the first point. */
		prevPoint = point = subPath->pointList;
		gcmASSERT(point);

		/* Add new command. */
		gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
			Stroke, gcvVGCMD_MOVE
			));

		VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

		/* Set the coordinates. */
		VGSL_SETCOORD_I(point->x, point->y);

		/* Create a LINE_TO or QUAD_TO command for the rest of points. */
		for (point = point->next; point; prevPoint = point, point = point->next)
		{
			if (point->curveType == vgcCURVE_LINE)
			{
				if (point->x == prevPoint->x && point->y == prevPoint->y)
				{
					/* Skip zero-length lines. */
					continue;
				}

				/* Add new command. */
				gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
					Stroke, gcvVGCMD_LINE
					));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

				/* Set the coordinates. */
				VGSL_SETCOORD_I(point->x, point->y);
			}
			else if (point->curveType == vgcCURVE_QUAD_CONTROL)
			{
				/* Add new command. */
				gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
					Stroke, gcvVGCMD_ARC_QUAD
					));

				VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

#if vgvSTROKE_COPY_OPTIMIZE
				prevPoint = point, point = point->next;
				coordinates = (gctINT_PTR) Stroke->coordinate;
				coordinates[0] = prevPoint->x;
				coordinates[1] = prevPoint->y;
				coordinates[2] = point->x;
				coordinates[3] = point->y;
#else
				/* Set the control coordinates. */
				gcmASSERT(point->curveType == vgcCURVE_QUAD_CONTROL);
				Stroke->set(Stroke, FIXED_TO_FLOAT(point->x));
				Stroke->set(Stroke, FIXED_TO_FLOAT(point->y));

				/* Set the anchor coordinates. */
				prevPoint = point, point = point->next;
				gcmASSERT(point);
				gcmASSERT(point->curveType == vgcCURVE_QUAD_ANCHOR);
				Stroke->set(Stroke, FIXED_TO_FLOAT(point->x));
				Stroke->set(Stroke, FIXED_TO_FLOAT(point->y));
#endif
			}
			else
			{
				vgsPOINT_I_PTR pointList, p, nextP;

#if vgvSTROKE_COPY_OPTIMIZE
				vgsPOINT_I_PTR p2;
#endif

				if (point->curveType == vgcCURVE_ARC_SCCW)
				{
					/* Convert an arc to Bezier curves. */
					gcmERROR_RETURN(_ConvertCircleArc_I(StrokeConversion, halfWidth,
								point->centerX, point->centerY,
								prevPoint->x, prevPoint->y,
								point->x, point->y,
								gcvFALSE, &pointList));
				}
				else
				{
					/* Convert a half circle to Bezier curves. */
					gcmASSERT(point->curveType == vgcCURVE_ARC_SCCW_HALF);
					gcmERROR_RETURN(_ConvertCircleArc_I(StrokeConversion, halfWidth,
								point->centerX, point->centerY,
								prevPoint->x, prevPoint->y,
								point->x, point->y,
								gcvTRUE, &pointList));
				}

				VGSL_STAT_COUNTER_INCREASE(vgStrokeArcCount);

				for (p = pointList; p; p = nextP)
				{
					/* Add new command. */
					gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
						Stroke, gcvVGCMD_ARC_QUAD
						));

					VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

#if vgvSTROKE_COPY_OPTIMIZE
					p2 = p->next;
					nextP = p2->next;
					coordinates = (gctINT_PTR) Stroke->coordinate;
					coordinates[0] = p->x;
					coordinates[1] = p->y;
					coordinates[2] = p2->x;
					coordinates[3] = p2->y;
					_FreePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR) p);
					_FreePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR) p2);
#else
					/* Set the control coordinates. */
					gcmASSERT(p->curveType == vgcCURVE_QUAD_CONTROL);
					Stroke->set(Stroke, FIXED_TO_FLOAT(p->x));
					Stroke->set(Stroke, FIXED_TO_FLOAT(p->y));

					/* Free the point. */
					nextP = p->next;
					_FreePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR) p);

					/* Set the anchor coordinates. */
					p = nextP;
					gcmASSERT(p);
					gcmASSERT(p->curveType == vgcCURVE_QUAD_ANCHOR);
					Stroke->set(Stroke, FIXED_TO_FLOAT(p->x));
					Stroke->set(Stroke, FIXED_TO_FLOAT(p->y));

					/* Free the point. */
					nextP = p->next;
					_FreePoint(StrokeConversion->pointMemPool, (vgsPOINT_PTR) p);
#endif
				}
			}
		}

		/* Create a CLOSE_PATH command at the end. */
		gcmERROR_RETURN(vgsPATHWALKER_WriteCommand(
			Stroke, gcvVGCMD_CLOSE
			));

		VGSL_STAT_COUNTER_INCREASE(vgStrokeCommandCount);

		/* Advance to the next subpath. */
		subPath = subPath->next;
	}

	return gcvSTATUS_OK;
}

static gceSTATUS
_CopyStrokePath(
	IN vgsCONTEXT_PTR Context,
	IN vgsSTROKECONVERSION_PTR StrokeConversion,
	IN vgsPATH_PTR Path
	)
{
	gceSTATUS status;

	/*gcmERROR_RETURN(_CheckStrokePath(StrokeConversion));*/

#if OUTPUT_STROKE_PATH
	/* Output path cmd and coordinates to log files. */
	if (StrokeConversion->useFixedPoint)
	{
		gcmERROR_RETURN(_OutputPath_I(
			Context, (vgsSTROKECONVERSION_I_PTR) StrokeConversion,
			(vgsSUBPATH_I_PTR) StrokeConversion->strokeSubPathList
			));
	}
	else
	{
		gcmERROR_RETURN(_OutputPath(
			Context, StrokeConversion,
			StrokeConversion->strokeSubPathList
			));
	}
#endif

	do
	{
		vgsSUBPATH_PTR subPath;
		gctBOOL havePoints;
		vgsPATHWALKER stroke;
		vgsPATH_DATA_PTR strokePath;


		/***********************************************************************
		** Determine the number of coordinates.
		*/

		/* Reset the number of points. */
		havePoints = gcvFALSE;

		/* Set initial subpath. */
		subPath = StrokeConversion->strokeSubPathList;

		/* Copy the stroke to the final buffer. */
		while (subPath)
		{
			/* Add the current number of points. */
			if (subPath->pointCount > 0)
			{
				havePoints = gcvTRUE;
				break;
			}

			/* Advance to the next subpath. */
			subPath = subPath->next;
		}

		/* No points - exit. */
		if (!havePoints)
		{
			status = gcvSTATUS_OK;
			break;
		}


		/***********************************************************************
		** Have points, copy to the final buffer.
		*/

		/* Set initial subpath. */
		subPath = StrokeConversion->strokeSubPathList;

		/* Validate the storage. */
		vgmVALIDATE_STORAGE(Context->strokeStorage);

		/* Prepare for writing the destination. */
		vgsPATHWALKER_InitializeWriter(
			Context, Context->strokeStorage, &stroke, Path
			);

		/* Copy the stroke to the final buffer. */
		if (StrokeConversion->useFixedPoint)
		{
			/* Allocate a new subpath. */
#if vgvSTROKE_COPY_OPTIMIZE
			gcmERR_BREAK(vgsPATHWALKER_StartSubpath(
				&stroke, vgmCOMMANDSIZE(2, gctUINT32), gcePATHTYPE_INT32
				));
#else
			gcmERR_BREAK(vgsPATHWALKER_StartSubpath(
				&stroke, vgmCOMMANDSIZE(2, gctFLOAT), gcePATHTYPE_FLOAT
				));
#endif

			/* Add to the MRU list. */
			vgsPATHSTORAGE_UpdateMRU(Context->strokeStorage, stroke.tail);

			gcmERR_BREAK(_CopyStrokeSubPath_I(
				(vgsSTROKECONVERSION_I_PTR) StrokeConversion,
				&stroke, (vgsSUBPATH_I_PTR) subPath
				));
		}
		else
		{
			/* Allocate a new subpath. */
			gcmERR_BREAK(vgsPATHWALKER_StartSubpath(
				&stroke, vgmCOMMANDSIZE(2, gctFLOAT), gcePATHTYPE_FLOAT
				));

			/* Add to the MRU list. */
			vgsPATHSTORAGE_UpdateMRU(Context->strokeStorage, stroke.tail);

			gcmERR_BREAK(_CopyStrokeSubPath(
				StrokeConversion, &stroke, subPath
				));
		}

		/* Close the path. */
		vgsPATHWALKER_CloseSubpath(&stroke);

		/* Get the shortcut to the path. */
		strokePath = stroke.head;
		gcmASSERT(strokePath != gcvNULL);

		/* Finalize the path structure. */
		gcmERR_BREAK(gcoVG_FinalizePath(Context->vg, &strokePath->data));

		/* Set the result. */
		gcmASSERT(Path->stroke == gcvNULL);
		Path->stroke = strokePath;

		/* Validate the storage. */
		vgmVALIDATE_STORAGE(Context->strokeStorage);
	}
	while (gcvFALSE);

	/* Return result. */
	return status;
}

/******************************************************************************\
|*************************** Main Control Functions ***************************|
|******************************************************************************/
static gceSTATUS
_ConvertStroke(
	IN vgsCONTEXT_PTR Context,
	IN vgsPATH_PTR Path
	)
{
	gceSTATUS status;
#if gcvSTROKE_KEEP_MEMPOOL
	vgsSTROKECONVERSION_PTR strokeConversion = Context->strokeConversion;
#else
	vgsSTROKECONVERSION realStrokeConversion;
	vgsSTROKECONVERSION_PTR strokeConversion = &realStrokeConversion;
#endif

	VGSL_STAT_COUNTER_INCREASE(vgStrokePathCount);

	do
	{
		vgmPROFILESTROKEENTRY(_ConstructStrokeConversion);

#if gcvSTROKE_KEEP_MEMPOOL
		if (strokeConversion == gcvNULL)
		{
			/* First time--allocate strokeconversion. */
			gcmERROR_RETURN(gcoOS_Allocate(
				Context->os,
				gcmSIZEOF(vgsSTROKECONVERSION),
				(gctPOINTER *) &strokeConversion
				));

			Context->strokeConversion = strokeConversion;

			gcmVERIFY_OK(gcoOS_ZeroMemory(
				strokeConversion,
				sizeof(vgsSTROKECONVERSION)
				));

			/* Initialize memory pools. */
			strokeConversion->os = Context->os;
			_InitMemPool(strokeConversion);
		}
#else
		gcmVERIFY_OK(gcoOS_ZeroMemory(
			strokeConversion,
			sizeof(vgsSTROKECONVERSION)
			));

		/* Initialize memory pools. */
		strokeConversion->os = Context->os;
		_InitMemPool(strokeConversion);
#endif

		/* Construct vgsSTROKECONVERSION_PTR structure. */
		gcmERR_BREAK(_InitStrokeConversion(
			Context, strokeConversion
			));

		vgmPROFILESTROKEEXIT(_ConstructStrokeConversion);

		vgmPROFILESTROKEENTRY(_CheckScaleAndBias);

#if USE_FIXED_POINT
		strokeConversion->useFixedPoint = gcvTRUE;
#else
		strokeConversion->useFixedPoint = gcvFALSE;
#endif

		/* Set scale and bias. */
		_CheckScaleAndBias(
			Context, Path, strokeConversion
			);

		vgmPROFILESTROKEEXIT(_CheckScaleAndBias);

#if vgvSTROKE_COPY_OPTIMIZE && gcvUSE_INTERNAL_SCALE
		gcmERR_BREAK(_CheckInputPath(
			strokeConversion, Path
			));
#endif

		/* Flatten InputPath if necessary and create subpaths. */
		if (strokeConversion->useFixedPoint)
		{
			vgmPROFILESTROKEENTRY(_FlattenPath_I);

#if gcvUSE_FAST_STROKE
			if (! strokeConversion->isFat &&
				strokeConversion->strokeDashPatternCount == 0
            &&  strokeConversion->strokeMiterLimit <= 4.0f
#if gcvUSE_INTERNAL_SCALE
            &&  strokeConversion->internalShiftBits == 0
#endif
                )
			{
				strokeConversion->useFastMode = gcvTRUE;
			}

			if (strokeConversion->useFastMode)
			{
				gcmERR_BREAK(_FastFlattenPath_I(
					strokeConversion, Path
					));
			}

			/* If useFastMode fails, use slow mode. */
			if (! strokeConversion->useFastMode && strokeConversion->useFixedPoint)
#endif
			{
				gcmERR_BREAK(_FlattenPath_I(
					strokeConversion, Path
					));
			}

			vgmPROFILESTROKEEXIT(_FlattenPath_I);
		}

		if (strokeConversion->useFixedPoint)
		{
			vgmPROFILESTROKEENTRY(_ConvertParameters);

			/* Convert parameters. */
			_ConvertParameters(
				strokeConversion
				);

			vgmPROFILESTROKEEXIT(_ConvertParameters);

			vgmPROFILESTROKEENTRY(_ReducePath_I);

			vgmPROFILESTROKEEXIT(_ReducePath_I);

#if OUTPUT_FLATTENED_PATH
			gcmERROR_RETURN(_OutputPath_I(
				Context, (vgsSTROKECONVERSION_I_PTR) strokeConversion,
				(vgsSUBPATH_I_PTR) strokeConversion->subPathList
				));
#endif

#if gcvUSE_FAST_STROKE
			if (strokeConversion->useFastMode)
			{
				vgmPROFILESTROKEENTRY(_FastCreateStrokePath_I);

				/* Create sub paths from flattened path for stroke. */
				gcmERR_BREAK(_FastCreateStrokePath_I(
					Context,
					(vgsSTROKECONVERSION_I_PTR) strokeConversion,
					Path
					));

				vgmPROFILESTROKEEXIT(_FastCreateStrokePath_I);
			}
			else
#endif
			{
				vgmPROFILESTROKEENTRY(_CreateStrokePath_I);

#if gcvUSE_FAST_FAT_LINE_HANDLE
				if (strokeConversion->strokeDashPatternCount == 0
				&&	strokeConversion->strokeCapStyle != gcvCAP_BUTT)
				{
					if (Context->targetImage.size.width  != 64 ||
						Context->targetImage.size.height != 64)
					{
						strokeConversion->useFastFatMode = gcvTRUE;
					}
				}

				/* Create sub paths from flattened path for stroke. */
				if (strokeConversion->useFastFatMode)
				{
					gcmERR_BREAK(_FastCreateFatStrokePath_I(
						Context,
						(vgsSTROKECONVERSION_I_PTR) strokeConversion,
						Path
						));
				}
				else
#endif
				{
					gcmERR_BREAK(_CreateStrokePath_I(
						(vgsSTROKECONVERSION_I_PTR) strokeConversion
						));
				}

				vgmPROFILESTROKEEXIT(_CreateStrokePath_I);
			}

		}
		else
		{
			vgmPROFILESTROKEENTRY(_FlattenPath);

			gcmERR_BREAK(_FlattenPath(
				strokeConversion, Path
				));

			vgmPROFILESTROKEEXIT(_FlattenPath);

			vgmPROFILESTROKEENTRY(_ReducePath);


			vgmPROFILESTROKEEXIT(_ReducePath);

#if OUTPUT_FLATTENED_PATH
			gcmERROR_RETURN(_OutputPath(
				Context, strokeConversion,
				strokeConversion->subPathList
				));
#endif

			vgmPROFILESTROKEENTRY(_CreateStrokePath);

			/* Create sub paths from flattened path for stroke. */
			gcmERR_BREAK(_CreateStrokePath(
				strokeConversion
				));

			vgmPROFILESTROKEEXIT(_CreateStrokePath);

		}

		vgmPROFILESTROKEENTRY(_CopyStrokePath);

#if gcvUSE_FAST_STROKE
#if gcvUSE_FAST_COPY
		if (! strokeConversion->useFastMode)
#endif
#endif
		{
			/* Copy stroke path to OutputPath. */
			gcmERR_BREAK(_CopyStrokePath(
				Context, strokeConversion, Path
				));
		}

		vgmPROFILESTROKEEXIT(_CopyStrokePath);

#if vgvSTROKE_COPY_OPTIMIZE
		if (strokeConversion->useFixedPoint)
		{
#if gcvUSE_INTERNAL_SCALE
			Path->strokeScale = 1.0f / (1 << (16 - strokeConversion->internalShiftBits));
#else
			Path->strokeScale = 1.0f / (1 << 16);
#endif
		}
		else
		{
			Path->strokeScale = 1.0f;
		}
#endif
	}
	while (gcvFALSE);

	vgmPROFILESTROKEENTRY(_DestroyStrokeConversion);

	/* Clean up. */
	gcmVERIFY_OK(_ResetStrokeConversion(
		strokeConversion
		));

#if !gcvSTROKE_KEEP_MEMPOOL
	gcmVERIFY_OK(vgfDestroyStrokeConversion(
		strokeConversion
		));
#endif

	vgmPROFILESTROKEEXIT(_DestroyStrokeConversion);

	return status;
}

gceSTATUS vgfUpdateStroke(
	IN vgsCONTEXT_PTR Context,
	IN vgsPATH_PTR Path
	)
{
	gceSTATUS status;

	do
	{
		/* Verify whether we need to generate new stroke. */
		do
		{
			/* First time? */
			if (Path->stroke == gcvNULL)
			{
#if vgvPRINT_DIAG && gcvENABLE_DIAG_MESSAGES
				printf("%s: %d, path=0x%08X, no stroke yet - generating\n", __FUNCTION__, __LINE__, Path);
#endif

				break;
			}

			/* Is stroke path valid? */
			if (!Path->strokeValid)
			{
#if vgvPRINT_DIAG && gcvENABLE_DIAG_MESSAGES
				printf("%s: %d, path=0x%08X, stroke is invalid - regenerating\n", __FUNCTION__, __LINE__, Path);
#endif

				break;
			}

#if !gcvENABLE_STROKE_CACHE
#if vgvPRINT_DIAG && gcvENABLE_DIAG_MESSAGES
			printf("%s: %d, path=0x%08X, stroke cache is disabled - regenerating\n", __FUNCTION__, __LINE__, Path);
#endif
			break;
#endif

			/* Compare stroking parameters. */
			if ((Path->lineWidth  != Context->strokeLineWidth)   ||
				(Path->miterLimit != Context->strokeMiterLimit)  ||
				(Path->capStyle   != Context->halStrokeCapStyle) ||
				(Path->joinStyle  != Context->halStrokeJoinStyle))
			{
#if vgvPRINT_DIAG && gcvENABLE_DIAG_MESSAGES
				printf("%s: %d, path=0x%08X, stroke parameters changed - regenerating\n", __FUNCTION__, __LINE__, Path);
#endif

				break;
			}

			/* Compare dashing parameters. */
			if ((Path->dashPatternCount != Context->strokeDashPtrnCount)  ||
				(Path->dashPhaseReset   != Context->strokeDashPhaseReset) ||
				(Path->dashPhase        != Context->strokeDashPhase))
			{
#if vgvPRINT_DIAG && gcvENABLE_DIAG_MESSAGES
				printf("%s: %d, path=0x%08X, dashing parameters changed - regenerating\n", __FUNCTION__, __LINE__, Path);
#endif

				break;
			}

			/* Compare the dash pattern. */
			if (Path->dashPatternCount != 0)
			{
				status = gcoOS_MemCmp(
					Path->dashPattern,
					Context->strokeDashPtrn,
					gcmSIZEOF(gctFLOAT) * Path->dashPatternCount
					);

				if (status == gcvSTATUS_MISMATCH)
				{
#if vgvPRINT_DIAG && gcvENABLE_DIAG_MESSAGES
				printf("%s: %d, path=0x%08X, dashing pattern changed - regenerating\n", __FUNCTION__, __LINE__, Path);
#endif

					break;
				}
			}

#if vgvPRINT_DIAG && gcvENABLE_DIAG_MESSAGES
			printf("%s: %d, path=0x%08X, reusing the stroke\n", __FUNCTION__, __LINE__, Path);
#endif

			/* No need. */
			return gcvSTATUS_OK;
		}
		while (gcvFALSE);

		vgmPROFILESTROKEENTRY(vgfUpdateStroke);

		/* Free the stroke. */
		if (Path->stroke != gcvNULL)
		{
			vgsPATHSTORAGE_Free(
				Context->strokeStorage,
				Path->stroke,
				gcvTRUE
				);

			/* Reset the stroke. */
			Path->strokeValid = gcvFALSE;
			Path->stroke      = gcvNULL;

			/* Validate the storage. */
			vgmVALIDATE_STORAGE(Context->strokeStorage);
		}

		vgmPROFILESTROKEENTRY(_ConvertStroke);

		/* Generate new stroke. */
		gcmERR_BREAK(_ConvertStroke(Context, Path));

		vgmPROFILESTROKEEXIT(_ConvertStroke);

		/* Set new parameters. */
		gcmERR_BREAK(gcoOS_MemCopy(
			Path->dashPattern,
			Context->strokeDashPtrn,
			gcmSIZEOF(Path->dashPattern)
			));

		Path->lineWidth  = Context->strokeLineWidth;
		Path->miterLimit = Context->strokeMiterLimit;
		Path->capStyle   = Context->halStrokeCapStyle;
		Path->joinStyle  = Context->halStrokeJoinStyle;

		Path->dashPatternCount = Context->strokeDashPtrnCount;
		Path->dashPhaseReset   = Context->strokeDashPhaseReset;
		Path->dashPhase        = Context->strokeDashPhase;

		/* Validate stroke path. */
		Path->strokeValid = gcvTRUE;

#if vgvPRINT_DIAG && gcvENABLE_DIAG_MESSAGES
		printf("%s: %d, stroke path=0x%08X, stroke valid=%d\n",
			__FUNCTION__, __LINE__, Path->stroke, Path->strokeValid
			);
#endif

		vgmPROFILESTROKEEXIT(vgfUpdateStroke);
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}

gceSTATUS
vgfDestroyStrokeConversion(
	IN vgsSTROKECONVERSION_PTR StrokeConversion
	)
{
	if (StrokeConversion == gcvNULL)
	{
		return gcvSTATUS_OK;
	}

#if gcvDEBUG
	/* Clean up. */
	gcmVERIFY_OK(_ResetStrokeConversion(
		StrokeConversion
		));
#else
	/* Free allocated memory. */
	if (StrokeConversion->strokeDashPatternCount > 0)
	{
		gcmVERIFY_OK(gcoOS_Free(StrokeConversion->os, StrokeConversion->strokeDashPattern));
	}
#endif

	/* Free memory pools. */
	_CleanupMemPool(StrokeConversion);

#if gcvSTROKE_KEEP_MEMPOOL
	/* Free stroke conversion object. */
	gcmVERIFY_OK(gcoOS_Free(StrokeConversion->os, StrokeConversion));
#endif

	return gcvSTATUS_OK;
}
