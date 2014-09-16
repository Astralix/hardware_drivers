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




#ifndef __gc_vg_basic_defines_h__
#define __gc_vg_basic_defines_h__

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
******************************** Various Options. ******************************
\******************************************************************************/

/* Profiler options. */
#define vgvENABLE_PROFILER			0
#define vgvENABLE_STROKE_PROFILER	0

/* Stroke optimizations. */
#define vgvSTROKE_COPY_OPTIMIZE		1

/* Tesselation forcing control. */
#define vgvFORCE_SW_TS				0
#define vgvFORCE_HW_TS				0

/* Hardware filter enable control. */
#define vgvENABLE_HW_FILTER			0
#define vgvENABLE_FAST_FILTER			1

#define vgvNAMED_OBJECTS_HASH			256

/******************************************************************************\
************************* Function call dumping macros. ************************
\******************************************************************************/


#if vgvENABLE_PROFILER
#	define vgmPROFILEENTRY(FunctionName) \
		vgfProfileEntry(vgmPROFILER_ENTRY_NAME(FunctionName))
#	define vgmPROFILEEXIT(FunctionName) \
		vgfProfileExit(vgmPROFILER_ENTRY_NAME(FunctionName))
#if gcvENABLE_INTERNAL_PROFILER
#		define vgmPROFILEINTENTRY(FunctionName) \
			vgfProfileEntry(vgmPROFILER_ENTRY_NAME(FunctionName))
#		define vgmPROFILEINTEXIT(FunctionName) \
			vgfProfileExit(vgmPROFILER_ENTRY_NAME(FunctionName))
#	else
#		define vgmPROFILEINTENTRY(FunctionName)
#		define vgmPROFILEINTEXIT(FunctionName)
#	endif
#else
#	define vgmPROFILEENTRY(FunctionName)
#	define vgmPROFILEEXIT(FunctionName)
#	define vgmPROFILEINTENTRY(FunctionName)
#	define vgmPROFILEINTEXIT(FunctionName)
#endif

#if vgvENABLE_STROKE_PROFILER
#	define vgmPROFILESTROKEENTRY(FunctionName) \
		vgfProfileEntry(vgmPROFILER_ENTRY_NAME(FunctionName))
#	define vgmPROFILESTROKEEXIT(FunctionName) \
		vgfProfileExit(vgmPROFILER_ENTRY_NAME(FunctionName))
#else
#	define vgmPROFILESTROKEENTRY(FunctionName)
#	define vgmPROFILESTROKEEXIT(FunctionName)
#endif

#define vgmERROR(Result) \
	{ \
		VGErrorCode lastResult = Result; \
		if (vgmIS_SUCCESS(Context->error)) \
		{ \
			Context->error = lastResult; \
		} \
		gcmTRACE( \
			gcvLEVEL_WARNING, \
			"vgmERROR: error=%d @ line=%d in %s.\n", \
			Result, __LINE__, __FILE__ \
			); \
		vgmERRORCALLBACK(Context, lastResult); \
	}

#define vgmENTERAPI(FunctionName) \
	VGErrorCode error = VG_NO_ERROR; \
	vgsTHREADDATA_PTR Thread; \
	vgsCONTEXT_PTR Context; \
	\
	vgmPROFILEENTRY(FunctionName); \
	\
	do \
	{ \
		Thread = vgfGetThreadData(gcvFALSE); \
		\
		if (Thread == gcvNULL) \
		{ \
			Context = gcvNULL; \
			error = VG_NO_CONTEXT_ERROR; \
			break; \
		} \
		\
		Context = Thread->context; \
		\
		if (Context == gcvNULL) \
		{ \
			error = VG_NO_CONTEXT_ERROR; \
			break; \
		} \
		\
		vgmDEBUGENTRY(Context, #FunctionName);

#define vgmLEAVEAPI(FunctionName) \
	} \
	while (gcvFALSE); \
	\
	vgmPROFILEEXIT(FunctionName); \
	\
	vgmDEBUGEXIT(Context, #FunctionName); \
	\
	if (error != VG_NO_ERROR) \
	\
	{ \
		gcmASSERT(error == VG_NO_ERROR); \
	\
	}


/******************************************************************************\
**************************** Error checking macros. ****************************
\******************************************************************************/

#define vgmIS_ERROR(func) \
	((func) != VG_NO_ERROR)

#define vgmIS_SUCCESS(func) \
	((func) == VG_NO_ERROR)

#define vgmERR_BREAK(func) \
	result = func; \
	if (vgmIS_ERROR(result)) \
	{ \
		break; \
	}


/******************************************************************************\
************************ Implementation-defined limits. ************************
\******************************************************************************/

/* VG_MAX_SCISSOR_RECTS */
#define vgvMAX_SCISSOR_RECTS			32

/* VG_MAX_DASH_COUNT */
#define vgvMAX_DASH_COUNT				16

/* VG_MAX_KERNEL_SIZE */
#define vgvMAX_KERNEL_SIZE				9

/* VG_MAX_SEPARABLE_KERNEL_SIZE */
#define vgvMAX_SEPARABLE_KERNEL_SIZE	15

/* VG_MAX_COLOR_RAMP_STOPS */
#define vgvMAX_COLOR_RAMP_STOPS			32

/* VG_MAX_IMAGE_WIDTH */
#define vgvMAX_IMAGE_WIDTH				8192

/* VG_MAX_IMAGE_HEIGHT */
#define vgvMAX_IMAGE_HEIGHT				8192

/* VG_MAX_IMAGE_PIXELS */
#define vgvMAX_IMAGE_PIXELS				(vgvMAX_IMAGE_WIDTH * \
										 vgvMAX_IMAGE_HEIGHT)

/* VG_MAX_IMAGE_BYTES */
#define vgvMAX_IMAGE_BYTES				(vgvMAX_IMAGE_PIXELS * 4)

/* VG_MAX_GAUSSIAN_STD_DEVIATION */
#define vgvMAX_GAUSSIAN_STD_DEVIATION	16


/******************************************************************************\
******************************* Helper Macros. *********************************
\******************************************************************************/

#define vgvPI				3.141592653589793238462643383279502f
#define vgv2xPI				(vgvPI + vgvPI)

#define vgvRGB				(VG_RED | VG_GREEN | VG_BLUE)
#define vgvRGBA				(VG_RED | VG_GREEN | VG_BLUE | VG_ALPHA)

#define vgvMATRIXROWS		3
#define vgvMATRIXCOLUMNS	3
#define vgvMATRIXITEMS		(vgvMATRIXROWS * vgvMATRIXCOLUMNS)

#if gcdENABLE_PERFORMANCE_PRIOR
#define vgmFLOATMAT(Matrix, Row, Column) \
	(vgmGETFLOATMATRIXVALUES(Matrix)[Column * vgvMATRIXROWS + Row])

#define vgmCONVERTMAT(DstType, SrcType, Opt, DstValue, SrcValue) \
	do \
	{\
		gcmASSERT(vgvMATRIXITEMS == 9);\
		((DstType*)DstValue)[0] = Opt(((SrcType*)SrcValue)[0]); \
		((DstType*)DstValue)[1] = Opt(((SrcType*)SrcValue)[1]); \
		((DstType*)DstValue)[2] = Opt(((SrcType*)SrcValue)[2]); \
		((DstType*)DstValue)[3] = Opt(((SrcType*)SrcValue)[3]); \
		((DstType*)DstValue)[4] = Opt(((SrcType*)SrcValue)[4]); \
		((DstType*)DstValue)[5] = Opt(((SrcType*)SrcValue)[5]); \
		((DstType*)DstValue)[6] = Opt(((SrcType*)SrcValue)[6]); \
		((DstType*)DstValue)[7] = Opt(((SrcType*)SrcValue)[7]); \
		((DstType*)DstValue)[8] = Opt(((SrcType*)SrcValue)[8]); \
	}while (gcvFALSE)

#endif

#define vgmMAT(Matrix, Row, Column) \
	(vgmGETMATRIXVALUES(Matrix)[Column * vgvMATRIXROWS + Row])

#define vgmGETRED(RGBA) \
	((VGfloat) (((RGBA) >> 24) & 0xFF))

#define vgmGETGREEN(RGBA) \
	((VGfloat) (((RGBA) >> 16) & 0xFF))

#define vgmGETBLUE(RGBA) \
	((VGfloat) (((RGBA) >> 8) & 0xFF))

#define vgmGETALPHA(RGBA) \
	((VGfloat) ((RGBA) & 0xFF))

#define vgmCLAMP(Value, Min, Max) \
	((Value) < (Min)) \
		? (Min) \
		: ((Value) > (Max)) \
			? (Max) \
			: (Value)

#define vgmIS_MISSALIGNED(Pointer, Alignment) \
	((gcmPTR2INT(Pointer) & ((Alignment) - 1)) != 0)

#define vgmIS_INVALID_PTR(Pointer, Alignment) \
	(((Pointer) == gcvNULL) || vgmIS_MISSALIGNED(Pointer, Alignment))


/******************************************************************************\
******************************* Fixed Point Macros. *********************************
\******************************************************************************/
/* Constants in fixed point format. */
/* Constants in fixed point format. */
typedef gctFIXED_POINT					gctFIXED;

#define vgvFIXED_INTEGER_BITS			16
#define vgvFIXED_FRACTION_BITS			16

#define vgvFIXED_MIN					((gctFIXED) 0x80000000)
#define vgvFIXED_MAX					((gctFIXED) 0x7FFFFFFF)

#define vgvFIXED_ZERO					((gctFIXED) 0x00000000)
#define vgvFIXED_ONE					((gctFIXED) 0x00010000)
#define vgvFIXED_TWO					((gctFIXED) 0x00020000)
#define vgvFIXED_HALF					((gctFIXED) 0x00008000)
#define vgvFIXED_QUARTER				((gctFIXED) 0x00004000)
#define vgvFIXED_EIGHTH					((gctFIXED) 0x00002000)
#define vgvFIXED_SIXTEENTH				((gctFIXED) 0x00001000)
#define vgvFIXED_TWO_AND_HALF			((gctFIXED) 0x00028000)
#define vgvFIXED_SIX					((gctFIXED) 0x00060000)

#define	vgvFIXED_PI						((gctFIXED) 0x0003243F)
#define vgvFIXED_PI_TWO					((gctFIXED) 0x0006487F)
#define vgvFIXED_PI_THREE_QUARTER		((gctFIXED) 0x00025B2B)
#define vgvFIXED_PI_HALF				((gctFIXED) 0x0001921F)
#define vgvFIXED_PI_QUARTER				((gctFIXED) 0x0000C90F)
#define vgvFIXED_PI_EIGHTH				((gctFIXED) 0x00006487)
#define vgvFIXED_PI_DIV_180				((gctFIXED) 0x00000478)
/* cos(PI/8) */
#define vgvFIXED_COS_PI_EIGHTH			((gctFIXED) 0x0000EC83)

/* Fixed point operations. */
#define vgmFIXED_TO_INT(X)				((gctINT)((X) >> 16))
#define vgmFIXED_TO_UINT(X)				((gctUINT)((X) >> 16))
#define vgmINT_TO_FIXED(X)				((gctFIXED)((X) << 16))
#define vgmFLOAT_TO_FIXED(X)			((gctFIXED)((X) * vgvFIXED_ONE))
#define vgmFIXED_TO_FLOAT(X)			((gctFLOAT)(X) / vgvFIXED_ONE)
#define vgmFIXED_INT(X)					((gctFIXED)((gctUINT)(X) & 0xffff0000))
#define vgmFIXED_FRAC(X)				((gctFIXED)((gctUINT)(X) & 0x0000ffff))
#define vgmFIXED_CEIL(X)				vgmFIXED_INT(X + vgvFIXED_ONE - 1)
#define vgmFIXED_FLOOR(X)				vgmFIXED_INT(X)
#define vgmFIXED_MUL(X1, X2)			((gctFIXED) (((gctINT64) (X1) * (X2)) >> 16))
#define vgmFIXED_DIV(X1, X2)			(((X2) == vgvFIXED_ZERO) ? vgvFIXED_ZERO : (gctFIXED) ((((gctINT64) (X1)) << 16) / (X2)))

#define vgmFIXED_MUL_BY_TWO(X)			((X) << 1)
#define vgmFIXED_DIV_BY_TWO(X)			((X) >> 1)
#define vgmFIXED_SQRT(X)				vgmFLOAT_TO_FIXED(gcmSQRTF(vgmFIXED_TO_FLOAT(X)))

#define vgmPACK_FIXED_COLOR_COMPONENT(X)	\
	(((X) >= vgvFIXED_ONE) ? 0xff : ((X) < vgvFIXED_ZERO ? 0x0 : (vgmFIXED_TO_INT(vgmFIXED_MUL(X, vgmINT_TO_FIXED(0xff))))))


#define vgmFLOAT_TO_FIXED_SPECIAL(X)	vgfFloatToFixed(X)
#define vgmFIXED_TO_FLOAT_SPECIAL(X)	vgfFixedToFloat(X)

/******************************************************************************\
*********************** Path segment handler definitions. **********************
\******************************************************************************/

/* Segment handler function parameters. */
#define vgmSEGMENTHANDLERPARAMETERS \
	vgsPATHWALKER_PTR Destination, \
	vgsPATHWALKER_PTR Source

/* Segment handler function type. */
typedef gceSTATUS (* vgtSEGMENTHANDLER) (
	vgmSEGMENTHANDLERPARAMETERS
	);


/******************************************************************************\
******************************* Internal types. ********************************
\******************************************************************************/

#if gcdENABLE_PERFORMANCE_PRIOR
#define vgmGETMATRIXVALUES(Pointer) \
	((gctFIXED*) (Pointer))

#define vgmGETFLOATMATRIXVALUES(Pointer) \
	((gctFLOAT_PTR) (Pointer))

#else
#define vgmGETMATRIXVALUES(Pointer) \
	((gctFLOAT_PTR) (Pointer))
#endif


#define vgmGETMATRIX(Pointer) \
	((vgsMATRIX_PTR) (Pointer))

typedef struct _vgsMATRIX          * vgsMATRIX_PTR;
typedef struct _vgsMATRIXCONTAINER * vgsMATRIXCONTAINER_PTR;

typedef VGfloat vgtFLOATVECTOR2[2];
typedef VGfloat vgtFLOATVECTOR3[3];
typedef VGfloat vgtFLOATVECTOR4[4];
typedef VGfloat vgtFLOATVECTOR5[5];

/*fixed vector*/
typedef gctFIXED vgtFIXEDVECTOR2[2];
typedef gctFIXED vgtFIXEDVECTOR3[3];



typedef VGint vgtINTVECTOR2[2];
typedef VGint vgtINTVECTOR3[3];
typedef VGint vgtINTVECTOR4[4];
typedef VGint vgtINTVECTOR5[5];

typedef gctUINT8 vgtBYTEVECTOR2[2];
typedef gctUINT8 vgtBYTEVECTOR3[3];
typedef gctUINT8 vgtBYTEVECTOR4[4];
typedef gctUINT8 vgtBYTEVECTOR5[5];

typedef void (* vgtMATRIXINVALIDATE) (
	vgsCONTEXT_PTR Context
	);

typedef gceSTATUS (* vgtMATRIXUPDATE) (
	vgsCONTEXT_PTR Context
	);

#if gcdENABLE_PERFORMANCE_PRIOR
typedef gctFIXED vgtMATRIXVALUES[vgvMATRIXITEMS];
#else
typedef VGfloat vgtMATRIXVALUES[vgvMATRIXITEMS];
#endif

typedef struct _vgsMATRIX
{
	/* Matrix values. */
	vgtMATRIXVALUES		values;
	gctBOOL				valuesDirty;

	/* Identity matrix flag. */
	gctBOOL				identity;
	gctBOOL				identityDirty;

	/* Matrix determinant. */
	gctFLOAT			det;
	gctBOOL				detDirty;

	/* Matrix valid flag. */
	gctBOOL				valid;
}
vgsMATRIX;

typedef struct _vgsMATRIXCONTAINER
{
	/* Matrix values. */
	vgsMATRIX			matrix;

	/* Called to invalidate the value itself and the dependents. */
	vgtMATRIXINVALIDATE	invalidate;

	/* Called to update the value of the matrix. */
	vgtMATRIXUPDATE		update;
}
vgsMATRIXCONTAINER;


/******************************************************************************\
********************************* Object types. ********************************
\******************************************************************************/

typedef enum _vgeOBJECTTYPE
{
	vgvOBJECTTYPE_PATH = 0,
	vgvOBJECTTYPE_IMAGE,
	vgvOBJECTTYPE_MASK,
	vgvOBJECTTYPE_FONT,
	vgvOBJECTTYPE_PAINT,

	vgvOBJECTTYPE_COUNT
}
vgeOBJECTTYPE;

#ifdef __cplusplus
}
#endif

#endif
