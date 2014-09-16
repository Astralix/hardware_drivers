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


#ifndef __gc_glsl_ir_h_
#define __gc_glsl_ir_h_

#include "gc_glsl_compiler_int.h"

#define slmF2B(f)		(((f) != (gctFLOAT)0.0)? gcvTRUE : gcvFALSE)
#define slmF2I(f)		((gctINT32)(f))
#define slmF2UI(f)		((gctUINT32)(f))

#define slmI2F(i)		((gctFLOAT)(i))
#define slmI2B(i)		(((i) != (gctINT32)0)? gcvTRUE : gcvFALSE)
#define slmI2UI(i)		((gctUINT32)(i))

#define slmUI2I(i)		((gctINT32)(i))
#define slmUI2F(i)		((gctFLOAT)(i))
#define slmUI2B(i)		(((i) != (gctUINT32)0)? gcvTRUE : gcvFALSE)

#define slmB2F(b)		((b)? (gctFLOAT)1.0 : (gctFLOAT)0.0)
#define slmB2I(b)		((b)? (gctINT32)1 : (gctINT32)0)
#define slmB2UI(b)		((b)? (gctUINT32)1 : (gctUINT32)0)

struct _slsNAME;

/* Data type */
enum _sleQUALIFIER
{
	slvQUALIFIER_NONE	= 0,

	slvQUALIFIER_CONST,

	slvQUALIFIER_UNIFORM,
	slvQUALIFIER_ATTRIBUTE,					/* Vertex only */

	slvQUALIFIER_VARYING_OUT,				/* Vertex only */
	slvQUALIFIER_VARYING_IN,				/* Fragment only */
	slvQUALIFIER_INVARIANT_VARYING_OUT,		/* Vertex only */
	slvQUALIFIER_INVARIANT_VARYING_IN,		/* Fragment only */
	slvQUALIFIER_FRAGMENT_OUT,

	slvQUALIFIER_CONST_IN,
	slvQUALIFIER_IN,
	slvQUALIFIER_OUT,
	slvQUALIFIER_INOUT
};

typedef gctUINT8	sltQUALIFIER;

gctCONST_STRING
slGetQualifierName(
	IN sltQUALIFIER Qualifier
	);

enum _slePRECISION
{
	slvPRECISION_DEFAULT	= 0,

	slvPRECISION_HIGH,
	slvPRECISION_MEDIUM,
	slvPRECISION_LOW
};

typedef struct _slsDEFAULT_PRECISION
{
    /*
       Initially, they are set to slvPRECISION_DEFAULT, and then if
       'precision' statement is called, it will set to exact precision
       value
    */

    enum _slePRECISION  floatPrecision;
    enum _slePRECISION  intPrecision;
}slsDEFAULT_PRECISION;

typedef gctUINT8	sltPRECISION;

enum _sleELEMENT_TYPE
{
	slvTYPE_VOID			= 0,

	slvTYPE_BOOL,
	slvTYPE_INT,
	slvTYPE_UINT,
	slvTYPE_FLOAT,

	slvTYPE_SAMPLER2D,
	slvTYPE_SAMPLERCUBE,

	slvTYPE_STRUCT,

	slvTYPE_SAMPLER3D,
	slvTYPE_SAMPLER1DARRAY,
	slvTYPE_SAMPLER2DARRAY,
	slvTYPE_SAMPLER1DARRAYSHADOW,
	slvTYPE_SAMPLER2DARRAYSHADOW,

	slvTYPE_ISAMPLER2D,
	slvTYPE_ISAMPLERCUBE,
	slvTYPE_ISAMPLER3D,
	slvTYPE_ISAMPLER2DARRAY,

	slvTYPE_USAMPLER2D,
	slvTYPE_USAMPLERCUBE,
	slvTYPE_USAMPLER3D,
	slvTYPE_USAMPLER2DARRAY,

	slvTYPE_SAMPLEREXTERNALOES
};

typedef gctUINT8	sltELEMENT_TYPE;

struct _slsNAME_SPACE;

struct _slsMATRIX_SIZE;
typedef struct _slsMATRIX_SIZE
{
  gctUINT8  rowCount;	  /* vector if rowCount >0 and
                                     columnCount = 0 */
  gctUINT8  columnCount;  /* 0 means not matrix, column dimension */
} slsMATRIX_SIZE;

typedef struct _slsDATA_TYPE
{
	slsDLINK_NODE		node;

	sltQUALIFIER		qualifier;

	sltPRECISION		precision;

	sltELEMENT_TYPE		elementType;

        slsMATRIX_SIZE		matrixSize;

	gctUINT			arrayLength;	/* 0 means not array */

	struct _slsNAME_SPACE *	fieldSpace; 	/* Only for struct */
}
slsDATA_TYPE;

gceSTATUS
slsDATA_TYPE_Construct(
	IN sloCOMPILER Compiler,
	IN gctINT TokenType,
	IN struct _slsNAME_SPACE * FieldSpace,
	OUT slsDATA_TYPE ** DataType
	);

gceSTATUS
slsDATA_TYPE_ConstructArray(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * ElementDataType,
	IN gctUINT ArrayLength,
	OUT slsDATA_TYPE ** DataType
	);

gceSTATUS
slsDATA_TYPE_ConstructElement(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * CompoundDataType,
	OUT slsDATA_TYPE ** DataType
	);

gceSTATUS
slsDATA_TYPE_Clone(
	IN sloCOMPILER Compiler,
	IN sltQUALIFIER Qualifier,
	IN slsDATA_TYPE * Source,
	OUT slsDATA_TYPE **	DataType
	);

gceSTATUS
slsDATA_TYPE_Destory(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType
	);

gceSTATUS
slsDATA_TYPE_Dump(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType
	);

gctBOOL
slsDATA_TYPE_IsEqual(
	IN slsDATA_TYPE * DataType1,
	IN slsDATA_TYPE * DataType2
	);

gctSIZE_T
slsDATA_TYPE_GetSize(
	IN slsDATA_TYPE * DataType
	);

gctSIZE_T
slsDATA_TYPE_GetFieldOffset(
	IN slsDATA_TYPE * StructDataType,
	IN struct _slsNAME * FieldName
	);

gctBOOL
slsDATA_TYPE_IsAssignableAndComparable(
	IN slsDATA_TYPE * DataType
	);

#define sldLowestRankedInteger  slvTYPE_BOOL
#define sldHighestRankedInteger slvTYPE_UINT
#define sldHighestRankedFloat slvTYPE_FLOAT

#define sldArithmeticTypeCount (sldHighestRankedFloat - sldLowestRankedInteger + 1)

#define slmIsElementTypeArithmetic(EType) \
 ((EType) >= sldLowestRankedInteger && (EType) <= sldHighestRankedFloat)

#define slmIsElementTypeBoolean(EType) \
   ((EType) == slvTYPE_BOOL)

#define slmIsElementTypeInteger(EType) \
 ((EType) >= sldLowestRankedInteger && (EType) <= sldHighestRankedInteger)

#define slmIsElementTypeFloating(EType) \
 ((EType) == slvTYPE_FLOAT)

#define slmDATA_TYPE_IsIntegerType(d) \
 ((d)->arrayLength == 0  && \
  slmIsElementTypeInteger((d)->elementType))

#define slmDATA_TYPE_IsArithmeticType(d) \
 ((d)->arrayLength == 0  && \
  slmIsElementTypeArithmetic((d)->elementType))

#define slmDATA_TYPE_vectorSize_GET(d) ((d)->matrixSize.columnCount? (gctUINT)0 : (d)->matrixSize.rowCount)
#define slmDATA_TYPE_vectorSize_NOCHECK_GET(d) ((d)->matrixSize.rowCount)
#define slmDATA_TYPE_vectorSize_SET(d, s) do \
        { (d)->matrixSize.rowCount = s; \
	   (d)->matrixSize.columnCount = 0; \
        } \
        while (gcvFALSE)

#define slmDATA_TYPE_matrixSize_GET(d) ((d)->matrixSize.columnCount)
#define slmDATA_TYPE_matrixRowCount_GET(d) ((d)->matrixSize.rowCount)
#define slmDATA_TYPE_matrixColumnCount_GET(d) ((d)->matrixSize.columnCount)
#define slmDATA_TYPE_matrixSize_SET(d, r, c) do \
        { (d)->matrixSize.rowCount = (r); \
	   (d)->matrixSize.columnCount = (c); \
        } \
        while (gcvFALSE)

#define slmDATA_TYPE_matrixRowCount_SET(d, c) (d)->matrixSize.rowCount = (c)
#define slmDATA_TYPE_matrixColumnCount_SET(d, c) (d)->matrixSize.columnCount = (c)

#define slmDATA_TYPE_IsSquareMat(dataType) \
	(slmDATA_TYPE_matrixColumnCount_GET(dataType) != 0 && \
         (slmDATA_TYPE_matrixRowCount_GET(dataType) == slmDATA_TYPE_matrixColumnCount_GET(dataType)))

#define slsDATA_TYPE_IsVoid(dataType) \
	((dataType)->elementType == slvTYPE_VOID)

#define slsDATA_TYPE_IsBVecOrIVecOrVec(dataType) \
	((dataType)->arrayLength == 0 && slmDATA_TYPE_vectorSize_GET(dataType) != 0)

#define slsDATA_TYPE_IsMat(dataType) \
	((dataType)->arrayLength == 0 && (dataType)->matrixSize.columnCount != 0)

#define slsDATA_TYPE_IsScalar(dataType) \
	 (slmDATA_TYPE_IsArithmeticType(dataType) \
	        && (dataType)->matrixSize.rowCount == 0 \
		&& (dataType)->matrixSize.columnCount == 0)

#define slsDATA_TYPE_IsArray(dataType) \
	((dataType)->arrayLength != 0)

#define slmDATA_TYPE_IsScalarInteger(dataType) \
  (slmIsElementTypeInteger((dataType)->elementType) && \
   slsDATA_TYPE_IsScalar(dataType))

#define slmDATA_TYPE_IsScalarFloating(dataType) \
  (slmIsElementTypeFloating((dataType)->elementType) && \
   slsDATA_TYPE_IsScalar(dataType))

#define slsDATA_TYPE_IsBool(dataType) \
  (slmIsElementTypeBoolean((dataType)->elementType) && \
   slsDATA_TYPE_IsScalar(dataType))

#define slsDATA_TYPE_IsInt(dataType) \
        slmDATA_TYPE_IsScalarInteger(dataType)

#define slsDATA_TYPE_IsFloat(dataType) \
        slmDATA_TYPE_IsScalarFloating(dataType)

#define slsDATA_TYPE_IsBVec(dataType) \
	(slmIsElementTypeBoolean(dataType->elementType) \
         && slmDATA_TYPE_vectorSize_GET(dataType) !=0 \
	        && (dataType)->arrayLength == 0)

#define slsDATA_TYPE_IsIVec(dataType) \
	(slmIsElementTypeInteger((dataType)->elementType) \
         && slmDATA_TYPE_vectorSize_GET(dataType) !=0 \
		&& (dataType)->arrayLength == 0)

#define slsDATA_TYPE_IsVec(dataType) \
	(slmIsElementTypeFloating((dataType)->elementType) \
         && slmDATA_TYPE_vectorSize_GET(dataType) !=0 \
		&& (dataType)->arrayLength == 0)

#define slsDATA_TYPE_IsBoolOrBVec(dataType) \
	(slmIsElementTypeBoolean(dataType->elementType) \
         && slmDATA_TYPE_matrixSize_GET(dataType) == 0  \
		&& (dataType)->arrayLength == 0)

#define slsDATA_TYPE_IsIntOrIVec(dataType) \
	(slmIsElementTypeInteger(dataType->elementType) \
         && slmDATA_TYPE_matrixSize_GET(dataType) == 0  \
		&& (dataType)->arrayLength == 0)

#define slsDATA_TYPE_IsFloatOrVec(dataType) \
	(slmIsElementTypeFloating(dataType->elementType) \
         && slmDATA_TYPE_matrixSize_GET(dataType) == 0  \
		&& (dataType)->arrayLength == 0)

#define slsDATA_TYPE_IsFloatOrVecOrMat(dataType) \
	((dataType)->arrayLength == 0 \
		&& (dataType)->elementType == slvTYPE_FLOAT)

#define slsDATA_TYPE_IsVecOrMat(dataType) \
	((dataType)->elementType == slvTYPE_FLOAT \
		&& ((dataType)->matrixSize.rowCount != 0 || (dataType)->matrixSize.columnCount != 0) \
		&& (dataType)->arrayLength == 0)

#define slsDATA_TYPE_IsStruct(dataType) \
	((dataType)->arrayLength == 0 \
		&& (dataType)->elementType == slvTYPE_STRUCT)

gceSTATUS
sloCOMPILER_CreateDataType(
	IN sloCOMPILER Compiler,
	IN gctINT TokenType,
	IN struct _slsNAME_SPACE * FieldSpace,
	OUT slsDATA_TYPE ** DataType
	);

gceSTATUS
sloCOMPILER_CreateArrayDataType(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * ElementDataType,
	IN gctUINT ArrayLength,
	OUT slsDATA_TYPE ** DataType
	);

gceSTATUS
sloCOMPILER_CreateElementDataType(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * CompoundDataType,
	OUT slsDATA_TYPE ** DataType
	);

gceSTATUS
sloCOMPILER_CloneDataType(
	IN sloCOMPILER Compiler,
	IN sltQUALIFIER Qualifier,
	IN slsDATA_TYPE * Source,
	OUT slsDATA_TYPE **	DataType
	);

/* Name and Name space. */
typedef enum _sleNAME_TYPE
{
	slvVARIABLE_NAME,
	slvPARAMETER_NAME,
	slvFUNC_NAME,
	slvSTRUCT_NAME,
	slvFIELD_NAME
}
sleNAME_TYPE;

struct _slsNAME_SPACE;
struct _sloIR_SET;
struct _sloIR_CONSTANT;
struct _sloIR_POLYNARY_EXPR;
struct _slsLOGICAL_REG;

typedef struct _slsNAME
{
	slsDLINK_NODE			node;

	struct _slsNAME_SPACE *		mySpace;

	gctUINT				lineNo;

	gctUINT				stringNo;

	sleNAME_TYPE			type;

	slsDATA_TYPE *			dataType;

	sltPOOL_STRING			symbol;

	gctBOOL				isBuiltIn;

	sleEXTENSION			extension;

	union
	{
		struct
		{
			struct _sloIR_CONSTANT *	constant;	/* Only for constant variables */
		}
		variableInfo;

		struct
		{
			struct _slsNAME *		aliasName;
		}
		parameterInfo;

		struct
		{
			struct _slsNAME_SPACE *		localSpace;

			gctBOOL				isFuncDef;

			struct _sloIR_SET *		funcBody;
		}
		funcInfo;									/* Only for function names */
	}
	u;

	struct
	{
		gctUINT				logicalRegCount;

		struct _slsLOGICAL_REG *	logicalRegs;

		gctBOOL				useAsTextureCoord;
		gctBOOL				isCounted;

		gcFUNCTION			function;		/* Only for the function/parameter names */
	}
	context;
}
slsNAME;

gceSTATUS
sloNAME_BindFuncBody(
	IN sloCOMPILER Compiler,
	IN slsNAME * FuncName,
	IN struct _sloIR_SET * FuncBody
	);

gceSTATUS
sloNAME_GetParamCount(
	IN sloCOMPILER Compiler,
	IN slsNAME * FuncName,
	OUT gctUINT * ParamCount
	);

gceSTATUS
slsNAME_BindAliasParamNames(
	IN sloCOMPILER Compiler,
	IN OUT slsNAME * FuncDefName,
	IN slsNAME * FuncDeclName
	);

gceSTATUS
slsNAME_Dump(
	IN sloCOMPILER Compiler,
	IN slsNAME * Name
	);

typedef struct _slsNAME_SPACE
{
	slsDLINK_NODE			node;

	struct _slsNAME_SPACE *	parent;

	slsDLINK_LIST			names;

	slsDLINK_LIST			subSpaces;

    /* Precision has same scope rule as variables, so put it */
    /* in namespace to better management. Only used for parsing */
    /* time, codegen does not use it */
    slsDEFAULT_PRECISION    defaultPrecision;
}
slsNAME_SPACE;

gceSTATUS
slsNAME_SPACE_Construct(
	IN sloCOMPILER Compiler,
	IN slsNAME_SPACE * Parent,
	OUT slsNAME_SPACE ** NameSpace
	);

gceSTATUS
slsNAME_SPACE_Destory(
	IN sloCOMPILER Compiler,
	IN slsNAME_SPACE * NameSpace
	);

gceSTATUS
slsNAME_SPACE_Dump(
	IN sloCOMPILER Compiler,
	IN slsNAME_SPACE * NameSpace
	);

gceSTATUS
slsNAME_SPACE_Search(
	IN sloCOMPILER Compiler,
	IN slsNAME_SPACE * NameSpace,
	IN sltPOOL_STRING Symbol,
	IN gctBOOL Recursive,
	OUT slsNAME ** Name
	);

gceSTATUS
slsNAME_SPACE_CheckNewFuncName(
	IN sloCOMPILER Compiler,
	IN slsNAME_SPACE * NameSpace,
	IN slsNAME * FuncName,
	OUT slsNAME ** FirstFuncName
	);

gceSTATUS
slsNAME_SPACE_BindFuncName(
	IN sloCOMPILER Compiler,
	IN slsNAME_SPACE * NameSpace,
	IN OUT struct _sloIR_POLYNARY_EXPR * PolynaryExpr
	);

gceSTATUS
slsNAME_SPACE_CreateName(
	IN sloCOMPILER Compiler,
	IN slsNAME_SPACE * NameSpace,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleNAME_TYPE Type,
	IN slsDATA_TYPE * DataType,
	IN sltPOOL_STRING Symbol,
	IN gctBOOL IsBuiltIn,
	IN sleEXTENSION Extension,
	OUT slsNAME ** Name
	);

gceSTATUS
sloCOMPILER_CreateName(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleNAME_TYPE Type,
	IN slsDATA_TYPE * DataType,
	IN sltPOOL_STRING Symbol,
	IN sleEXTENSION Extension,
	OUT slsNAME ** Name
	);

gceSTATUS
sloCOMPILER_CreateAuxiliaryName(
	IN sloCOMPILER Compiler,
    IN slsNAME* refName,
    IN gctUINT LineNo,
	IN gctUINT StringNo,
    IN slsDATA_TYPE * DataType,
	OUT slsNAME ** Name
	);

gceSTATUS
sloCOMPILER_SearchName(
	IN sloCOMPILER Compiler,
	IN sltPOOL_STRING Symbol,
	IN gctBOOL Recursive,
	OUT slsNAME ** Name
	);

gceSTATUS
sloCOMPILER_CheckNewFuncName(
	IN sloCOMPILER Compiler,
	IN slsNAME * FuncName,
	OUT slsNAME ** FirstFuncName
	);

gceSTATUS
sloCOMPILER_CreateNameSpace(
	IN sloCOMPILER Compiler,
	OUT slsNAME_SPACE ** NameSpace
	);

gceSTATUS
sloCOMPILER_PopCurrentNameSpace(
	IN sloCOMPILER Compiler,
	OUT slsNAME_SPACE ** PrevNameSpace
	);

gceSTATUS
sloCOMPILER_AtGlobalNameSpace(
	IN sloCOMPILER Compiler,
	OUT gctBOOL * AtGlobalNameSpace
	);

gceSTATUS
sloCOMPILER_SetDefaultPrecision(
	IN sloCOMPILER Compiler,
	IN sltELEMENT_TYPE typeToSet,
    IN sltPRECISION precision
	);

gceSTATUS
sloCOMPILER_GetDefaultPrecision(
	IN sloCOMPILER Compiler,
	IN sltELEMENT_TYPE typeToGet,
    sltPRECISION *precision
	);

/* Type of IR objects. */
typedef enum _sleIR_OBJECT_TYPE
{
	slvIR_UNKNOWN 				= 0,

	slvIR_SET					= gcmCC('S','E','T','\0'),
	slvIR_ITERATION				= gcmCC('I','T','E','R'),
	slvIR_JUMP					= gcmCC('J','U','M','P'),
	slvIR_VARIABLE				= gcmCC('V','A','R','\0'),
	slvIR_CONSTANT				= gcmCC('C','N','S','T'),
	slvIR_UNARY_EXPR			= gcmCC('U','N','R','Y'),
	slvIR_BINARY_EXPR			= gcmCC('B','N','R','Y'),
	slvIR_SELECTION				= gcmCC('S','E','L','T'),
	slvIR_POLYNARY_EXPR			= gcmCC('P','O','L','Y')
}
sleIR_OBJECT_TYPE;

/* sloIR_BASE object. */
struct _slsVTAB;
typedef struct _slsVTAB *	sltVPTR;

struct _sloIR_BASE
{
	slsDLINK_NODE		node;

	sltVPTR				vptr;

	gctUINT				lineNo;

	gctUINT				stringNo;
};

typedef struct _sloIR_BASE *			sloIR_BASE;

typedef gceSTATUS
(* sltDESTROY_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN sloIR_BASE This
	);

typedef gceSTATUS
(* sltDUMP_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN sloIR_BASE This
	);

struct _slsVISITOR;

typedef gceSTATUS
(* sltACCEPT_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN sloIR_BASE This,
	IN struct _slsVISITOR * Visitor,
	IN OUT gctPOINTER Parameters
	);

typedef struct _slsVTAB
{
	sleIR_OBJECT_TYPE	type;

	sltDESTROY_FUNC_PTR	destroy;

	sltDUMP_FUNC_PTR	dump;

	sltACCEPT_FUNC_PTR	accept;
}
slsVTAB;

#define sloIR_BASE_Initialize(base, finalVPtr, finalLineNo, finalStringNo) \
	do \
	{ \
		(base)->vptr		= (finalVPtr); \
		(base)->lineNo		= (finalLineNo); \
		(base)->stringNo	= (finalStringNo); \
	} \
	while (gcvFALSE)

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
#	define slmVERIFY_IR_OBJECT(obj, objType) \
		do \
		{ \
			if (((obj) == gcvNULL) || (((sloIR_BASE)(obj))->vptr->type != (objType))) \
			{ \
				gcmASSERT(((obj) != gcvNULL) && (((sloIR_BASE)(obj))->vptr->type == (objType))); \
				return gcvSTATUS_INVALID_OBJECT; \
			} \
		} \
		while (gcvFALSE)
#else
#	define slmVERIFY_IR_OBJECT(obj, objType) do {} while (gcvFALSE)
#endif

#define sloIR_OBJECT_GetType(obj) \
	((obj)->vptr->type)

#define sloIR_OBJECT_Destroy(compiler, obj) \
	((obj)->vptr->destroy((compiler), (obj)))

#define sloIR_OBJECT_Dump(compiler, obj) \
	((obj)->vptr->dump((compiler), (obj)))

#define sloIR_OBJECT_Accept(compiler, obj, visitor, parameters) \
	((obj)->vptr->accept((compiler), (obj), (visitor), (parameters)))

/* sloIR_SET object. */
typedef enum _sleSET_TYPE
{
	slvDECL_SET,
	slvSTATEMENT_SET,
	slvEXPR_SET
}
sleSET_TYPE;

struct _sloIR_SET
{
	struct _sloIR_BASE	base;

	sleSET_TYPE			type;

	slsDLINK_LIST		members;

	slsNAME *			funcName;	/* Only for the function definition */
};

typedef struct _sloIR_SET *				sloIR_SET;

gceSTATUS
sloIR_SET_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleSET_TYPE Type,
	OUT sloIR_SET * Set
	);

gceSTATUS
sloIR_SET_AddMember(
	IN sloCOMPILER Compiler,
	IN sloIR_SET Set,
	IN sloIR_BASE Member
	);

gceSTATUS
sloIR_SET_GetMemberCount(
	IN sloCOMPILER Compiler,
	IN sloIR_SET Set,
	OUT gctUINT * MemberCount
	);

gceSTATUS
sloIR_SET_IsRoot(
	IN sloCOMPILER Compiler,
	IN sloIR_SET Set,
	OUT gctBOOL * IsRoot
	);

gceSTATUS
sloCOMPILER_AddExternalDecl(
	IN sloCOMPILER Compiler,
	IN sloIR_BASE ExternalDecl
	);

/* sloIR_EXPR object. */
struct _sloIR_EXPR
{
	struct _sloIR_BASE	base;

	slsDATA_TYPE *		dataType;
};

typedef struct _sloIR_EXPR *			sloIR_EXPR;

#define sloIR_EXPR_Initialize(expr, finalVPtr, finalLineNo, finalStringNo, exprDataType) \
	do \
	{ \
		sloIR_BASE_Initialize(&(expr)->base, (finalVPtr), (finalLineNo), (finalStringNo)); \
		\
		(expr)->dataType = (exprDataType); \
	} \
	while (gcvFALSE)

/* sloIR_ITERATION object. */
typedef enum _sleITERATION_TYPE
{
	slvFOR,
	slvWHILE,
	slvDO_WHILE
}
sleITERATION_TYPE;

struct _sloIR_ITERATION
{
	struct _sloIR_BASE	base;

	sleITERATION_TYPE	type;

	sloIR_EXPR			condExpr;

	sloIR_BASE			loopBody;

	slsNAME_SPACE *		forSpace;

	sloIR_BASE			forInitStatement;

	sloIR_EXPR			forRestExpr;
};

typedef struct _sloIR_ITERATION *		sloIR_ITERATION;

gceSTATUS
sloIR_ITERATION_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleITERATION_TYPE Type,
	IN sloIR_EXPR CondExpr,
	IN sloIR_BASE LoopBody,
	IN slsNAME_SPACE * ForSpace,
	IN sloIR_BASE ForInitStatement,
	IN sloIR_EXPR ForRestExpr,
	OUT sloIR_ITERATION * Iteration
	);

/* sloIR_JUMP object. */
typedef enum _sleJUMP_TYPE
{
	slvCONTINUE,
	slvBREAK,
	slvRETURN,
	slvDISCARD
}
sleJUMP_TYPE;

gctCONST_STRING
slGetIRJumpTypeName(
	IN sleJUMP_TYPE JumpType
	);

struct _sloIR_JUMP
{
	struct _sloIR_BASE	base;

	sleJUMP_TYPE		type;

	sloIR_EXPR			returnExpr;
};

typedef struct _sloIR_JUMP *			sloIR_JUMP;

gceSTATUS
sloIR_JUMP_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleJUMP_TYPE Type,
	IN sloIR_EXPR ReturnExpr,
	OUT sloIR_JUMP * Jump
	);

/* sloIR_VARIABLE object. */
struct _sloIR_VARIABLE
{
	struct _sloIR_EXPR	exprBase;

	slsNAME *			name;
};

typedef struct _sloIR_VARIABLE *		sloIR_VARIABLE;

gceSTATUS
sloIR_VARIABLE_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN slsNAME * Name,
	OUT sloIR_VARIABLE * Variable
	);

/* sloIR_CONSTANT object. */
typedef union _sluCONSTANT_VALUE
{
	gctBOOL		boolValue;
	gctINT32	intValue;
	gctUINT32 	uintValue;
	gctFLOAT	floatValue;
}
sluCONSTANT_VALUE;

struct _sloIR_CONSTANT
{
	struct _sloIR_EXPR	exprBase;

	gctUINT				valueCount;

	sluCONSTANT_VALUE * values;
};

typedef struct _sloIR_CONSTANT *		sloIR_CONSTANT;

gceSTATUS
sloIR_CONSTANT_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN slsDATA_TYPE * DataType,
	OUT sloIR_CONSTANT * Constant
	);

gceSTATUS
sloIR_CONSTANT_Clone(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sloIR_CONSTANT Source,
	OUT sloIR_CONSTANT * Constant
	);

gceSTATUS
sloIR_CONSTANT_AddValues(
	IN sloCOMPILER Compiler,
	IN sloIR_CONSTANT Constant,
	IN gctUINT ValueCount,
	IN sluCONSTANT_VALUE * Values
	);

gceSTATUS
sloIR_CONSTANT_GetBoolValue(
	IN sloCOMPILER Compiler,
	IN sloIR_CONSTANT Constant,
	IN gctUINT ValueNo,
	OUT sluCONSTANT_VALUE * Value
	);

gceSTATUS
sloIR_CONSTANT_GetIntValue(
	IN sloCOMPILER Compiler,
	IN sloIR_CONSTANT Constant,
	IN gctUINT ValueNo,
	OUT sluCONSTANT_VALUE * Value
	);

gceSTATUS
sloIR_CONSTANT_GetFloatValue(
	IN sloCOMPILER Compiler,
	IN sloIR_CONSTANT Constant,
	IN gctUINT ValueNo,
	OUT sluCONSTANT_VALUE * Value
	);

typedef gceSTATUS
(* sltEVALUATE_FUNC_PTR)(
	IN sltELEMENT_TYPE Type,
	IN OUT sluCONSTANT_VALUE * Value
	);

gceSTATUS
sloIR_CONSTANT_Evaluate(
	IN sloCOMPILER Compiler,
	IN OUT sloIR_CONSTANT Constant,
	IN sltEVALUATE_FUNC_PTR Evaluate
	);

/* sloIR_UNARY_EXPR object. */
typedef enum _sleUNARY_EXPR_TYPE
{
	slvUNARY_FIELD_SELECTION,
	slvUNARY_COMPONENT_SELECTION,

	slvUNARY_POST_INC,
	slvUNARY_POST_DEC,
	slvUNARY_PRE_INC,
	slvUNARY_PRE_DEC,

	slvUNARY_NEG,

	slvUNARY_NOT
}
sleUNARY_EXPR_TYPE;

typedef enum _sleCOMPONENT
{
	slvCOMPONENT_X	= 0x0,
	slvCOMPONENT_Y	= 0x1,
	slvCOMPONENT_Z	= 0x2,
	slvCOMPONENT_W	= 0x3
}
sleCOMPONENT;

typedef struct _slsCOMPONENT_SELECTION
{
	gctUINT8	components;

	gctUINT8	x;

	gctUINT8	y;

	gctUINT8	z;

	gctUINT8	w;
}
slsCOMPONENT_SELECTION;

extern slsCOMPONENT_SELECTION	ComponentSelection_X;
extern slsCOMPONENT_SELECTION	ComponentSelection_Y;
extern slsCOMPONENT_SELECTION	ComponentSelection_Z;
extern slsCOMPONENT_SELECTION	ComponentSelection_W;
extern slsCOMPONENT_SELECTION	ComponentSelection_XY;
extern slsCOMPONENT_SELECTION	ComponentSelection_XYZ;
extern slsCOMPONENT_SELECTION	ComponentSelection_XYZW;

gctBOOL
slIsRepeatedComponentSelection(
	IN slsCOMPONENT_SELECTION * ComponentSelection
	);

gctCONST_STRING
slGetIRUnaryExprTypeName(
	IN sleUNARY_EXPR_TYPE UnaryExprType
	);

struct _sloIR_UNARY_EXPR
{
	struct _sloIR_EXPR			exprBase;

	sleUNARY_EXPR_TYPE			type;

	sloIR_EXPR					operand;

	union
	{
		slsNAME *				fieldName;				/* Only for the field selection */

		slsCOMPONENT_SELECTION	componentSelection;		/* Only for the component selection */
	}
	u;
};

typedef struct _sloIR_UNARY_EXPR *		sloIR_UNARY_EXPR;

gceSTATUS
sloIR_UNARY_EXPR_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleUNARY_EXPR_TYPE Type,
	IN sloIR_EXPR Operand,
	IN slsNAME * FieldName,								/* Only for the field selection */
	IN slsCOMPONENT_SELECTION * ComponentSelection,		/* Only for the component selection */
	OUT sloIR_UNARY_EXPR * UnaryExpr
	);

gceSTATUS
sloIR_UNARY_EXPR_Evaluate(
	IN sloCOMPILER Compiler,
	IN sleUNARY_EXPR_TYPE Type,
	IN sloIR_CONSTANT Constant,
	IN slsNAME * FieldName,								/* Only for the field selection */
	IN slsCOMPONENT_SELECTION * ComponentSelection,		/* Only for the component selection */
	OUT sloIR_CONSTANT * ResultConstant
	);

/* sloIR_BINARY_EXPR object. */
typedef enum _sleBINARY_EXPR_TYPE
{
	slvBINARY_SUBSCRIPT,

	slvBINARY_ADD,
	slvBINARY_SUB,
	slvBINARY_MUL,
	slvBINARY_DIV,

	slvBINARY_GREATER_THAN,
	slvBINARY_LESS_THAN,
	slvBINARY_GREATER_THAN_EQUAL,
	slvBINARY_LESS_THAN_EQUAL,

	slvBINARY_EQUAL,
	slvBINARY_NOT_EQUAL,

	slvBINARY_AND,
	slvBINARY_OR,
	slvBINARY_XOR,

	slvBINARY_SEQUENCE,

	slvBINARY_ASSIGN,

	slvBINARY_MUL_ASSIGN,
	slvBINARY_DIV_ASSIGN,
	slvBINARY_ADD_ASSIGN,
	slvBINARY_SUB_ASSIGN
}
sleBINARY_EXPR_TYPE;

struct _sloIR_BINARY_EXPR
{
	struct _sloIR_EXPR	exprBase;

	sleBINARY_EXPR_TYPE	type;

	sloIR_EXPR			leftOperand;

	sloIR_EXPR			rightOperand;

    union
    {
        /* If type is slvBINARY_SUBSCRIPT and leftOperand is vector, */
        /* then we need generate an auxiliary symbol to hold scalar */
        /* array */
        struct _slsVEC2ARRAY*            vec2Array;
    }u;
};

typedef struct _sloIR_BINARY_EXPR *		sloIR_BINARY_EXPR;

gceSTATUS
sloIR_BINARY_EXPR_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleBINARY_EXPR_TYPE Type,
	IN sloIR_EXPR LeftOperand,
	IN sloIR_EXPR RightOperand,
	OUT sloIR_BINARY_EXPR * BinaryExpr
	);

gceSTATUS
sloIR_BINARY_EXPR_Evaluate(
	IN sloCOMPILER Compiler,
	IN sleBINARY_EXPR_TYPE Type,
	IN sloIR_CONSTANT LeftConstant,
	IN sloIR_CONSTANT RightConstant,
	OUT sloIR_CONSTANT * ResultConstant
	);

/* sloIR_SELECTION object. */
struct _sloIR_SELECTION
{
	struct _sloIR_EXPR	exprBase;

	sloIR_EXPR			condExpr;

	sloIR_BASE			trueOperand;

	sloIR_BASE			falseOperand;
};

typedef struct _sloIR_SELECTION *		sloIR_SELECTION;

gceSTATUS
sloIR_SELECTION_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN slsDATA_TYPE * DataType,
	IN sloIR_EXPR CondExpr,
	IN sloIR_BASE TrueOperand,
	IN sloIR_BASE FalseOperand,
	OUT sloIR_SELECTION * Selection
	);

/* sloIR_POLYNARY_EXPR object. */
typedef enum _slePOLYNARY_EXPR_TYPE
{
	slvPOLYNARY_CONSTRUCT_FLOAT,
	slvPOLYNARY_CONSTRUCT_INT,
	slvPOLYNARY_CONSTRUCT_UINT,
	slvPOLYNARY_CONSTRUCT_BOOL,
	slvPOLYNARY_CONSTRUCT_VEC2,
	slvPOLYNARY_CONSTRUCT_VEC3,
	slvPOLYNARY_CONSTRUCT_VEC4,
	slvPOLYNARY_CONSTRUCT_BVEC2,
	slvPOLYNARY_CONSTRUCT_BVEC3,
	slvPOLYNARY_CONSTRUCT_BVEC4,
	slvPOLYNARY_CONSTRUCT_IVEC2,
	slvPOLYNARY_CONSTRUCT_IVEC3,
	slvPOLYNARY_CONSTRUCT_IVEC4,
	slvPOLYNARY_CONSTRUCT_UVEC2,
	slvPOLYNARY_CONSTRUCT_UVEC3,
	slvPOLYNARY_CONSTRUCT_UVEC4,
	slvPOLYNARY_CONSTRUCT_MAT2,
	slvPOLYNARY_CONSTRUCT_MAT2X3,
	slvPOLYNARY_CONSTRUCT_MAT2X4,
	slvPOLYNARY_CONSTRUCT_MAT3,
	slvPOLYNARY_CONSTRUCT_MAT3X2,
	slvPOLYNARY_CONSTRUCT_MAT3X4,
	slvPOLYNARY_CONSTRUCT_MAT4,
	slvPOLYNARY_CONSTRUCT_MAT4X2,
	slvPOLYNARY_CONSTRUCT_MAT4X3,
	slvPOLYNARY_CONSTRUCT_STRUCT,

	slvPOLYNARY_FUNC_CALL
}
slePOLYNARY_EXPR_TYPE;

struct _sloIR_POLYNARY_EXPR
{
	struct _sloIR_EXPR		exprBase;

	slePOLYNARY_EXPR_TYPE	type;

	sltPOOL_STRING			funcSymbol;	/* Only for the function call */

	slsNAME *				funcName;	/* Only for the function call */

	sloIR_SET				operands;
};

typedef struct _sloIR_POLYNARY_EXPR *	sloIR_POLYNARY_EXPR;

gctCONST_STRING
slGetIRPolynaryExprTypeName(
	IN slePOLYNARY_EXPR_TYPE PolynaryExprType
	);

gceSTATUS
sloIR_POLYNARY_EXPR_Construct(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN slePOLYNARY_EXPR_TYPE Type,
	IN slsDATA_TYPE * DataType,
	IN sltPOOL_STRING FuncSymbol,
	OUT sloIR_POLYNARY_EXPR * PolynaryExpr
	);

gceSTATUS
sloCOMPILER_BindFuncCall(
	IN sloCOMPILER Compiler,
	IN OUT sloIR_POLYNARY_EXPR PolynaryExpr
	);

gceSTATUS
sloIR_POLYNARY_EXPR_Evaluate(
	IN sloCOMPILER Compiler,
	IN sloIR_POLYNARY_EXPR PolynaryExpr,
	OUT sloIR_CONSTANT * ResultConstant
	);

/* Visitor */
typedef gceSTATUS
(* sltVISIT_SET_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_SET Set,
	IN OUT gctPOINTER Parameters
	);

typedef gceSTATUS
(* sltVISIT_ITERATION_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_ITERATION Iteration,
	IN OUT gctPOINTER Parameters
	);

typedef gceSTATUS
(* sltVISIT_JUMP_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_JUMP Jump,
	IN OUT gctPOINTER Parameters
	);

typedef gceSTATUS
(* sltVISIT_VARIABLE_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_VARIABLE Variable,
	IN OUT gctPOINTER Parameters
	);

typedef gceSTATUS
(* sltVISIT_CONSTANT_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_CONSTANT Constant,
	IN OUT gctPOINTER Parameters
	);

typedef gceSTATUS
(* sltVISIT_UNARY_EXPR_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_UNARY_EXPR UnaryExpr,
	IN OUT gctPOINTER Parameters
	);

typedef gceSTATUS
(* sltVISIT_BINARY_EXPR_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_BINARY_EXPR BinaryExpr,
	IN OUT gctPOINTER Parameters
	);

typedef gceSTATUS
(* sltVISIT_SELECTION_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_SELECTION Selection,
	IN OUT gctPOINTER Parameters
	);

typedef gceSTATUS
(* sltVISIT_POLYNARY_EXPR_FUNC_PTR)(
	IN sloCOMPILER Compiler,
	IN struct _slsVISITOR * Visitor,
	IN sloIR_POLYNARY_EXPR PolynaryExpr,
	IN OUT gctPOINTER Parameters
	);

typedef struct _slsVISITOR
{
	slsOBJECT							object;

	sltVISIT_SET_FUNC_PTR				visitSet;

	sltVISIT_ITERATION_FUNC_PTR			visitIteration;

	sltVISIT_JUMP_FUNC_PTR				visitJump;

	sltVISIT_VARIABLE_FUNC_PTR			visitVariable;

	sltVISIT_CONSTANT_FUNC_PTR			visitConstant;

	sltVISIT_UNARY_EXPR_FUNC_PTR		visitUnaryExpr;

	sltVISIT_BINARY_EXPR_FUNC_PTR		visitBinaryExpr;

	sltVISIT_SELECTION_FUNC_PTR			visitSelection;

	sltVISIT_POLYNARY_EXPR_FUNC_PTR		visitPolynaryExpr;
}
slsVISITOR;

#endif /* __gc_glsl_ir_h_ */
