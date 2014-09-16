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


#ifndef __gc_glsl_compiler_h_
#define __gc_glsl_compiler_h_

/* Dump Options */
#define slmDUMP_OPTIONS		((sltDUMP_OPTIONS)slvDUMP_NONE)

#include "gc_glsl_common.h"

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)

#	define slmVERIFY_OBJECT(obj, objType) \
		do \
		{ \
			if (((obj) == gcvNULL) || (((slsOBJECT *)(obj))->type != (objType))) \
			{ \
				gcmASSERT(((obj) != gcvNULL) && (((slsOBJECT *)(obj))->type == (objType))); \
                gcmFOOTER_ARG("status=%d(gcvSTATUS_INVALID_OBJECT)", gcvSTATUS_INVALID_OBJECT); \
				return gcvSTATUS_INVALID_OBJECT; \
			} \
		} \
		while (gcvFALSE)

#	define slmASSERT_OBJECT(obj, objType) \
		do \
		{ \
			if (((obj) == gcvNULL) || (((slsOBJECT *)(obj))->type != (objType))) \
			{ \
				gcmASSERT(((obj) != gcvNULL) && (((slsOBJECT *)(obj))->type == (objType))); \
			} \
		} \
		while (gcvFALSE)

#else

#	define slmVERIFY_OBJECT(obj, objType) do {} while (gcvFALSE)
#	define slmASSERT_OBJECT(obj, objType) do {} while (gcvFALSE)

#endif

/* Optimization options. */
typedef enum _sleOPTIMIZATION_OPTION
{
	slvOPTIMIZATION_NONE 				= 0x0000,

	slvOPTIMIZATION_CALCULATION			= 0x0001,

	slvOPTIMIZATION_UNROLL_ITERATION	= 0x0010,

	slvOPTIMIZATION_DATA_FLOW			= 0x0100,

	slvOPTIMIZATION_SPECIAL				= 0x8000,

	slvOPTIMIZATION_ALL					= 0xFFFF,
}
sleOPTIMIZATION_OPTION;

typedef gctUINT32						sltOPTIMIZATION_OPTIONS;

/* Dump options. */
typedef enum _sleDUMP_OPTION
{
	slvDUMP_NONE		= 0x0000,
	slvDUMP_SOURCE		= 0x0001,
	slvDUMP_PREPROCESSOR	= 0x0010,
	slvDUMP_SCANNER		= 0x0100,
	slvDUMP_PARSER		= 0x0200,
	slvDUMP_IR		= 0x0400,
	slvDUMP_CODE_GENERATOR	= 0x1000,
	slvDUMP_CODE_EMITTER	= 0x2000,
	slvDUMP_COMPILER	= 0xff00,
	slvDUMP_ALL		= 0xffff
}
sleDUMP_OPTION;

typedef gctUINT16				sltDUMP_OPTIONS;

/* Type of objects. */
typedef enum _sleOBJECT_TYPE
{
	slvOBJ_UNKNOWN 		= 0,

	slvOBJ_COMPILER		= gcmCC('C','M','P','L'),
	slvOBJ_CODE_GENERATOR	= gcmCC('C','G','E','N'),
	slvOBJ_OBJECT_COUNTER	= gcmCC('O','B','J','C'),
	slvOBJ_CODE_EMITTER	= gcmCC('C','E','M','T')
}
sleOBJECT_TYPE;

/* slsOBJECT object defintinon. */
typedef struct _slsOBJECT
{
	/* Type of an object. */
	sleOBJECT_TYPE				type;
}
slsOBJECT;

struct _sloCOMPILER;
struct _sloIR_BASE;

typedef struct _sloCOMPILER *			sloCOMPILER;

struct _sloCODE_GENERATOR;
typedef struct _sloCODE_GENERATOR *sloCODE_GENERATOR;

struct _sloCODE_EMITTER;

typedef struct _sloCODE_EMITTER *		sloCODE_EMITTER;

/* Construct a new sloCOMPILER object. */
typedef enum _sleSHADER_TYPE
{
	slvSHADER_TYPE_VERTEX		= gcSHADER_TYPE_VERTEX,
	slvSHADER_TYPE_FRAGMENT		= gcSHADER_TYPE_FRAGMENT,
	slvSHADER_TYPE_PRECOMPILED	= gcSHADER_TYPE_PRECOMPILED
}
sleSHADER_TYPE;

gctUINT32 *
sloCOMPILER_GetVersion(
	IN sleSHADER_TYPE ShaderType
);

gceSTATUS
sloCOMPILER_Construct(
	IN gcoHAL Hal,
	IN sleSHADER_TYPE ShaderType,
	OUT sloCOMPILER * Compiler
	);

/* Destroy an sloCOMPILER object. */
gceSTATUS
sloCOMPILER_Destroy(
	IN sloCOMPILER Compiler
	);

/* Get the shader type */
gceSTATUS
sloCOMPILER_GetShaderType(
	IN sloCOMPILER Compiler,
	OUT sleSHADER_TYPE * ShaderType
	);

/* Get the binary */
gceSTATUS
sloCOMPILER_GetBinary(
	IN sloCOMPILER Compiler,
	OUT gcSHADER * Binary
	);

gceSTATUS
sloCOMPILER_GetHAL(
	IN sloCOMPILER Compiler,
	OUT gcoHAL * Hal
	);

/* Compile the source strings */
gceSTATUS
sloCOMPILER_Compile(
	IN sloCOMPILER Compiler,
	IN sltOPTIMIZATION_OPTIONS OptimizationOptions,
	IN sltDUMP_OPTIONS DumpOptions,
	IN gctUINT StringCount,
	IN gctCONST_STRING Strings[],
	OUT gcSHADER * Binary,
	OUT gctSTRING * Log
	);

/* Preprocess the source strings only */
gceSTATUS
sloCOMPILER_Preprocess(
	IN sloCOMPILER Compiler,
	IN sltOPTIMIZATION_OPTIONS OptimizationOptions,
	IN sltDUMP_OPTIONS DumpOptions,
	IN gctUINT StringCount,
	IN gctCONST_STRING Strings[],
	OUT gctSTRING * Log
	);

/* Set the current line no */
gceSTATUS
sloCOMPILER_SetCurrentLineNo(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo
	);

/* Set the current string no */
gceSTATUS
sloCOMPILER_SetCurrentStringNo(
	IN sloCOMPILER Compiler,
	IN gctUINT StringNo
	);

/* Get the current line no */
gctUINT
sloCOMPILER_GetCurrentLineNo(
	IN sloCOMPILER Compiler
	);

/* Get the current string no */
gctUINT
sloCOMPILER_GetCurrentStringNo(
	IN sloCOMPILER Compiler
	);

/* Allocate memory blocks for the compiler */
gceSTATUS
sloCOMPILER_Allocate(
	IN sloCOMPILER Compiler,
	IN gctSIZE_T Bytes,
	OUT gctPOINTER * Memory
	);

/* Free memory blocks for the compiler */
gceSTATUS
sloCOMPILER_Free(
	IN sloCOMPILER Compiler,
	IN gctPOINTER Memory
	);

/* Report info */
typedef enum _sleREPORT_TYPE
{
	slvREPORT_FATAL_ERROR 		= 0,
	slvREPORT_INTERNAL_ERROR,
	slvREPORT_ERROR,
	slvREPORT_WARN
}
sleREPORT_TYPE;

gceSTATUS
sloCOMPILER_VReport(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleREPORT_TYPE Type,
	IN gctCONST_STRING Message,
	IN gctARGUMENTS Arguments
	);

gceSTATUS
sloCOMPILER_Report(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleREPORT_TYPE Type,
	IN gctCONST_STRING Message,
	IN ...
	);

/* Dump info */
gceSTATUS
sloCOMPILER_Dump(
	IN sloCOMPILER Compiler,
	IN sleDUMP_OPTION DumpOption,
	IN gctCONST_STRING Message,
	IN ...
	);

/* Allocate pool string */
typedef gctSTRING		sltPOOL_STRING;

gceSTATUS
sloCOMPILER_AllocatePoolString(
	IN sloCOMPILER Compiler,
	IN gctCONST_STRING String,
	OUT sltPOOL_STRING * PoolString
	);

/* Extensions */
typedef enum _sleEXTENSION
{
	slvEXTENSION_NONE			= 0x00000000,

	slvEXTENSION_STANDARD_DERIVATIVES	= 0x00000001,
	slvEXTENSION_TEXTURE_3D			= 0x00000002,
	slvEXTENSION_TEXTURE_ARRAY		= 0x00000004,
	slvEXTENSION_FRAG_DEPTH			= 0x00000008,
	slvEXTENSION_EGL_IMAGE_EXTERNAL		= 0x00000010,

	slvEXTENSION_ALL			= slvEXTENSION_STANDARD_DERIVATIVES |
						  slvEXTENSION_TEXTURE_3D |
						  slvEXTENSION_TEXTURE_ARRAY |
						  slvEXTENSION_FRAG_DEPTH |
						  slvEXTENSION_EGL_IMAGE_EXTERNAL
}
sleEXTENSION;

typedef gctUINT32				sltEXTENSIONS;

/* Enable extension */
gceSTATUS
sloCOMPILER_EnableExtension(
	IN sloCOMPILER Compiler,
	IN sleEXTENSION Extension,
	IN gctBOOL Enable
	);

gceSTATUS
sloCOMPILER_BackPatch(
    IN sloCOMPILER Compiler
    )
;

#endif /* __gc_glsl_compiler_h_ */
