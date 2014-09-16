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


#ifndef __gc_glsl_emit_code_h_
#define __gc_glsl_emit_code_h_

#include "gc_glsl_parser.h"
#include "gc_glsl_gen_code.h"

#define _PI				((gctFLOAT)3.14159265358979323846)
#define _HALF_PI		(_PI / (gctFLOAT)2.0)

gctSIZE_T
gcGetDataTypeSize(
	IN gcSHADER_TYPE DataType
	);

gctUINT8
gcGetDataTypeComponentCount(
	IN gcSHADER_TYPE DataType
	);

gcSHADER_TYPE
gcGetComponentDataType(
	IN gcSHADER_TYPE DataType
	);

gctCONST_STRING
gcGetDataTypeName(
	IN gcSHADER_TYPE DataType
	);

gctBOOL
gcIsSamplerDataType(
	IN gcSHADER_TYPE DataType
	);

gctBOOL
gcIsScalarDataType(
	IN gcSHADER_TYPE DataType
	);

gctBOOL
gcIsVectorDataType(
	IN gcSHADER_TYPE DataType
	);

gctBOOL
gcIsMatrixDataType(
	IN gcSHADER_TYPE DataType
	);

gcSHADER_TYPE
gcGetVectorComponentDataType(
	IN gcSHADER_TYPE DataType
	);

gcSHADER_TYPE
gcGetVectorSliceDataType(
	IN gcSHADER_TYPE DataType,
	IN gctUINT8 Components
	);

gcSHADER_TYPE
gcConvScalarToVectorDataType(
	IN gcSHADER_TYPE DataType,
	IN gctUINT8 Components
	);

gctUINT8
gcGetVectorDataTypeComponentCount(
	IN gcSHADER_TYPE DataType
	);

gcSHADER_TYPE
gcGetVectorComponentSelectionDataType(
	IN gcSHADER_TYPE DataType,
	IN gctUINT8	Components
	);

gcSHADER_TYPE
gcGetMatrixColumnDataType(
	IN gcSHADER_TYPE DataType
	);

gctUINT
gcGetMatrixDataTypeColumnCount(
	IN gcSHADER_TYPE DataType
	);

#define	gcGetMatrixDataTypeRowCount(dataType)	gcGetMatrixDataTypeColumnCount(dataType)

gctCONST_STRING
gcGetAttributeName(
	IN gcATTRIBUTE Attribute
	);

gctCONST_STRING
gcGetUniformName(
	IN gcUNIFORM Uniform
	);

gctUINT8
gcGetDefaultEnable(
	IN gcSHADER_TYPE DataType
	);

gctUINT8
gcGetVectorComponentEnable(
	IN gctUINT8 Enable,
	IN gctUINT8 Component
	);

gctUINT8
gcGetDefaultSwizzle(
	IN gcSHADER_TYPE DataType
	);

gctUINT8
gcGetVectorComponentSwizzle(
	IN gctUINT8 Swizzle,
	IN gctUINT8 Component
	);

typedef struct _gcsTARGET
{
	gcSHADER_TYPE	dataType;		/* Excluding gcSHADER_FLOAT_2X2/3X3/4X4 */

	gctREG_INDEX	tempRegIndex;

	gctUINT8		enable;

	gcSL_INDEXED	indexMode;

	gctREG_INDEX	indexRegIndex;
}
gcsTARGET;

#define gcsTARGET_Initialize(target, _dataType, _tempRegIndex, _enable, _indexMode, _indexRegIndex) \
	do \
	{ \
		(target)->dataType		= (_dataType); \
		(target)->tempRegIndex	= (_tempRegIndex); \
		(target)->enable		= (_enable); \
		(target)->indexMode		= (_indexMode); \
		(target)->indexRegIndex	= (_indexRegIndex); \
	} \
	while (gcvFALSE)

#define gcsTARGET_InitializeAsVectorComponent(componentTarget, target, component) \
	do \
	{ \
		*(componentTarget)			= *(target); \
		(componentTarget)->dataType	= gcGetVectorComponentDataType((target)->dataType); \
		(componentTarget)->enable	= gcGetVectorComponentEnable((target)->enable, (gctUINT8) component); \
	} \
	while (gcvFALSE)

#define gcsTARGET_InitializeUsingIOperand(target, iOperand) \
	gcsTARGET_Initialize( \
						(target), \
						(iOperand)->dataType, \
						(iOperand)->tempRegIndex, \
						gcGetDefaultEnable((iOperand)->dataType), \
						gcSL_NOT_INDEXED, \
						0)

typedef enum _gceSOURCE_TYPE
{
    gcvSOURCE_TEMP,
    gcvSOURCE_ATTRIBUTE,
	gcvSOURCE_UNIFORM,
    gcvSOURCE_CONSTANT
}
gceSOURCE_TYPE;

typedef struct _gcsSOURCE_REG
{
    union
	{
		gcATTRIBUTE		attribute;

		gcUNIFORM		uniform;
	}
	u;

	gctREG_INDEX		regIndex;

	gctUINT8			swizzle;

	gcSL_INDEXED		indexMode;

	gctREG_INDEX		indexRegIndex;
}
gcsSOURCE_REG;

typedef struct _gcsSOURCE_CONSTANT
{
    union
	{
        gctFLOAT	floatConstant;

        gctINT		intConstant;

        gctBOOL		boolConstant;
    }
	u;
}
gcsSOURCE_CONSTANT;

typedef struct _gcsSOURCE
{
    gceSOURCE_TYPE	type;

    gcSHADER_TYPE	dataType;		/* Excluding gcSHADER_FLOAT_2X2/3X3/4X4 */

    union
	{
		gcsSOURCE_REG		sourceReg;

		gcsSOURCE_CONSTANT	sourceConstant;
    }
	u;
}
gcsSOURCE;

#define gcsSOURCE_InitializeReg(source, _type, _dataType, _sourceReg) \
	do \
	{ \
		(source)->type						= (_type); \
		(source)->dataType					= (_dataType); \
		(source)->u.sourceReg				= (_sourceReg); \
	} \
	while (gcvFALSE)

#define gcsSOURCE_InitializeConstant(source, _dataType, _sourceConstant) \
	do \
	{ \
		(source)->type						= gcvSOURCE_CONSTANT; \
		(source)->dataType					= (_dataType); \
		(source)->u.sourceConstant			= (_sourceConstant); \
	} \
	while (gcvFALSE)

#define gcsSOURCE_InitializeFloatConstant(source, _floatValue) \
	do \
	{ \
		(source)->type								= gcvSOURCE_CONSTANT; \
		(source)->dataType							= gcSHADER_FLOAT_X1; \
		(source)->u.sourceConstant.u.floatConstant	= (_floatValue); \
	} \
	while (gcvFALSE)

#define gcsSOURCE_InitializeTempReg(source, _dataType, _tempRegIndex, _swizzle, _indexMode, _indexRegIndex) \
	do \
	{ \
		(source)->type						= gcvSOURCE_TEMP; \
		(source)->dataType					= (_dataType); \
		(source)->u.sourceReg.regIndex		= (_tempRegIndex); \
		(source)->u.sourceReg.swizzle		= (_swizzle); \
		(source)->u.sourceReg.indexMode		= (_indexMode); \
		(source)->u.sourceReg.indexRegIndex	= (_indexRegIndex); \
	} \
	while (gcvFALSE)

#define gcsSOURCE_InitializeAsVectorComponent(componentSource, source, component) \
	do \
	{ \
		*(componentSource)					= *(source); \
		(componentSource)->dataType			= gcGetVectorComponentDataType((source)->dataType); \
		\
		if ((source)->type != gcvSOURCE_CONSTANT) \
		{ \
			(componentSource)->u.sourceReg.swizzle	= \
					gcGetVectorComponentSwizzle((source)->u.sourceReg.swizzle, (gctUINT8) component); \
		} \
	} \
	while (gcvFALSE)

#define gcsSOURCE_InitializeUsingIOperand(source, iOperand) \
	gcsSOURCE_InitializeTempReg( \
								(source), \
								(iOperand)->dataType, \
								(iOperand)->tempRegIndex, \
								gcGetDefaultSwizzle((iOperand)->dataType), \
								gcSL_NOT_INDEXED, \
								0)

#define gcsSOURCE_InitializeUsingIOperandAsVectorComponent(source, iOperand, swizzle) \
	gcsSOURCE_InitializeTempReg( \
								(source), \
								gcGetVectorComponentDataType((iOperand)->dataType), \
								(iOperand)->tempRegIndex, \
								(swizzle), \
								gcSL_NOT_INDEXED, \
								0)

gceSTATUS
slNewAttribute(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE DataType,
	IN gctSIZE_T Length,
	IN gctBOOL IsTexture,
	OUT gcATTRIBUTE * Attribute
	);

gceSTATUS
slNewUniform(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE DataType,
    IN gcSHADER_PRECISION precision,
	IN gctSIZE_T Length,
	OUT gcUNIFORM * Uniform
	);

gceSTATUS
slGetUniformSamplerIndex(
	IN sloCOMPILER Compiler,
	IN gcUNIFORM UniformSampler,
	OUT gctREG_INDEX * Index
	);

gceSTATUS
slNewOutput(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE DataType,
	IN gctSIZE_T Length,
	IN gctREG_INDEX TempRegIndex
	);

gceSTATUS
slNewVariable(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE DataType,
	IN gctSIZE_T Length,
	IN gctREG_INDEX TempRegIndex,
    IN gcSHADER_VAR_CATEGORY varCategory,
    IN gctUINT16 numStructureElement,
    IN gctINT16 parent,
    IN gctINT16 prevSibling,
    OUT gctINT16* ThisVarIndex
	);

gceSTATUS
slUpdateVariableTempReg(
	IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctUINT varIndex,
	IN gctREG_INDEX newTempRegIndex
	);

gctREG_INDEX
slNewTempRegs(
	IN sloCOMPILER Compiler,
	IN gctUINT RegCount
	);

gctLABEL
slNewLabel(
	IN sloCOMPILER Compiler
	);

gceSTATUS
slSetLabel(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctLABEL Label
	);

gceSTATUS
slEmitAssignCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	);

gceSTATUS
slEmitCode1(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	);

gceSTATUS
slEmitCode2(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	);

gceSTATUS
slEmitAlwaysBranchCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gctLABEL Label
	);

gceSTATUS
slEmitTestBranchCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gctLABEL Label,
	IN gctBOOL TrueBranch,
	IN gcsSOURCE * Source
	);

gceSTATUS
slEmitCompareBranchCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN sleCONDITION Condition,
	IN gctLABEL Label,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	);

gceSTATUS
slBeginMainFunction(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo
	);

gceSTATUS
slEndMainFunction(
	IN sloCOMPILER Compiler
	);

gceSTATUS
slNewFunction(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctCONST_STRING Name,
	OUT gcFUNCTION * Function
	);

gceSTATUS
slNewFunctionArgument(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function,
	IN gcSHADER_TYPE DataType,
	IN gctSIZE_T Length,
	IN gctREG_INDEX TempRegIndex,
	IN gctUINT8 Qualifier
	);

gceSTATUS
slBeginFunction(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcFUNCTION Function
	);

gceSTATUS
slEndFunction(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function
	);

gceSTATUS
slGetFunctionLabel(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function,
	OUT gctLABEL * Label
	);

gceSTATUS
sloCODE_EMITTER_Construct(
	IN sloCOMPILER Compiler,
	OUT sloCODE_EMITTER * CodeEmitter
	);

gceSTATUS
sloCODE_EMITTER_Destroy(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter
	);

#endif /* __gc_glsl_emit_code_h_ */
