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


#ifndef __gc_glsl_gen_code_h_
#define __gc_glsl_gen_code_h_

#include "gc_glsl_ir.h"

typedef	gctUINT16		gctREG_INDEX;
typedef	gctUINT			gctLABEL;

slsCOMPONENT_SELECTION
slGetDefaultComponentSelection(
	IN gcSHADER_TYPE DataType
	);

/* OPCODE */
typedef enum _sleOPCODE
{
	slvOPCODE_INVALID					= 0,

	/* Assignment Operation */
	slvOPCODE_ASSIGN,

	/* Arithmetic Operations */
	slvOPCODE_ADD,
	slvOPCODE_SUB,
	slvOPCODE_MUL,
	slvOPCODE_DIV,

	/* Texture Operations */
	slvOPCODE_TEXTURE_LOAD,
	slvOPCODE_TEXTURE_LOAD_PROJ,
	slvOPCODE_TEXTURE_BIAS,
	slvOPCODE_TEXTURE_LOD,

	/* Conversion Operations */
	slvOPCODE_FLOAT_TO_INT,
	slvOPCODE_FLOAT_TO_BOOL,
	slvOPCODE_INT_TO_BOOL,

	/* Other Calculation Operations */
	slvOPCODE_INVERSE,

	slvOPCODE_LESS_THAN,
	slvOPCODE_LESS_THAN_EQUAL,
	slvOPCODE_GREATER_THAN,
	slvOPCODE_GREATER_THAN_EQUAL,
	slvOPCODE_EQUAL,
	slvOPCODE_NOT_EQUAL,

	slvOPCODE_ANY,
	slvOPCODE_ALL,
	slvOPCODE_NOT,

	slvOPCODE_SIN,
	slvOPCODE_COS,
	slvOPCODE_TAN,

	slvOPCODE_ASIN,
	slvOPCODE_ACOS,
	slvOPCODE_ATAN,
	slvOPCODE_ATAN2,

	slvOPCODE_POW,
	slvOPCODE_EXP2,
	slvOPCODE_LOG2,
	slvOPCODE_SQRT,
	slvOPCODE_INVERSE_SQRT,

	slvOPCODE_ABS,
	slvOPCODE_SIGN,
	slvOPCODE_FLOOR,
	slvOPCODE_CEIL,
	slvOPCODE_FRACT,
	slvOPCODE_MIN,
	slvOPCODE_MAX,
	slvOPCODE_SATURATE,
	slvOPCODE_STEP,
	slvOPCODE_DOT,
	slvOPCODE_CROSS,
	slvOPCODE_NORMALIZE,

	/* Branch Operations */
	slvOPCODE_JUMP,
	slvOPCODE_CALL,
	slvOPCODE_RETURN,
	slvOPCODE_DISCARD,

	/* Derivative Operations */
	slvOPCODE_DFDX,
	slvOPCODE_DFDY,
	slvOPCODE_FWIDTH
}
sleOPCODE;

gctCONST_STRING
slGetOpcodeName(
	IN sleOPCODE Opcode
	);

/* CONDITION */
typedef enum _sleCONDITION
{
	slvCONDITION_INVALID,

	slvCONDITION_EQUAL,
	slvCONDITION_NOT_EQUAL,
	slvCONDITION_LESS_THAN,
	slvCONDITION_LESS_THAN_EQUAL,
	slvCONDITION_GREATER_THAN,
	slvCONDITION_GREATER_THAN_EQUAL,
	slvCONDITION_XOR
}
sleCONDITION;

gctCONST_STRING
slGetConditionName(
	IN sleCONDITION Condition
	);

sleCONDITION
slGetNotCondition(
	IN sleCONDITION Condition
	);

/* slsLOPERAND and slsROPERAND */
typedef enum _sleINDEX_MODE
{
	slvINDEX_NONE,
	slvINDEX_REG,
	slvINDEX_CONSTANT
}
sleINDEX_MODE;

typedef struct _slsLOGICAL_REG
{
	sltQUALIFIER			qualifier;

	gcSHADER_TYPE			dataType;

    union
	{
		gcATTRIBUTE			attribute;

		gcUNIFORM			uniform;
	}	u;

	gctREG_INDEX			regIndex;

	slsCOMPONENT_SELECTION	componentSelection;
}
slsLOGICAL_REG;

#define slsLOGICAL_REG_InitializeTemp(reg, _qualifier, _dataType, _regIndex) \
	do \
	{ \
		(reg)->qualifier			= (_qualifier); \
		(reg)->dataType				= (_dataType); \
		(reg)->regIndex				= (_regIndex); \
		(reg)->componentSelection	= slGetDefaultComponentSelection(_dataType); \
	} \
	while (gcvFALSE)

#define slsLOGICAL_REG_InitializeAttribute(reg, _qualifier, _dataType, _attribute, _regIndex) \
	do \
	{ \
		(reg)->qualifier			= (_qualifier); \
		(reg)->dataType				= (_dataType); \
		(reg)->u.attribute			= (_attribute); \
		(reg)->regIndex				= (_regIndex); \
		(reg)->componentSelection	= slGetDefaultComponentSelection(_dataType); \
	} \
	while (gcvFALSE)

#define slsLOGICAL_REG_InitializeUniform(reg, _qualifier, _dataType, _uniform, _regIndex) \
	do \
	{ \
		(reg)->qualifier			= (_qualifier); \
		(reg)->dataType				= (_dataType); \
		(reg)->u.uniform			= (_uniform); \
		(reg)->regIndex				= (_regIndex); \
		(reg)->componentSelection	= slGetDefaultComponentSelection(_dataType); \
	} \
	while (gcvFALSE)

gceSTATUS
slsNAME_AllocLogicalRegs(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN slsNAME * Name
	);

gceSTATUS
slsNAME_FreeLogicalRegs(
	IN sloCOMPILER Compiler,
	IN slsNAME * Name
	);

typedef struct _slsINDEX
{
	sleINDEX_MODE	mode;

	union
	{
		gctREG_INDEX	indexRegIndex;

		gctREG_INDEX	constant;
	}	u;
}
slsINDEX;

typedef struct _slsLOPERAND
{
	gcSHADER_TYPE		dataType;

	slsLOGICAL_REG		reg;

	slsINDEX			arrayIndex;

	slsINDEX			matrixIndex;

	slsINDEX			vectorIndex;
}
slsLOPERAND;

#define slsLOPERAND_Initialize(operand, _reg) \
	do \
	{ \
		(operand)->dataType			= (_reg)->dataType; \
		(operand)->reg				= *(_reg); \
		(operand)->arrayIndex.mode	= slvINDEX_NONE; \
		(operand)->matrixIndex.mode	= slvINDEX_NONE; \
		(operand)->vectorIndex.mode	= slvINDEX_NONE; \
	} \
	while (gcvFALSE)

#define slsLOPERAND_InitializeTempReg(operand, _qualifier, _dataType, _regIndex) \
	do \
	{ \
		(operand)->dataType			= (_dataType); \
		slsLOGICAL_REG_InitializeTemp(&(operand)->reg, (_qualifier), (_dataType), (_regIndex)); \
		(operand)->arrayIndex.mode	= slvINDEX_NONE; \
		(operand)->matrixIndex.mode	= slvINDEX_NONE; \
		(operand)->vectorIndex.mode	= slvINDEX_NONE; \
	} \
	while (gcvFALSE)

#define slsLOPERAND_InitializeAsMatrixColumn(operand, _matrixOperand, _matrixIndex) \
	do \
	{ \
		gcmASSERT((_matrixOperand)->matrixIndex.mode == slvINDEX_NONE); \
		\
		*(operand)							= *(_matrixOperand); \
		(operand)->dataType					= \
							gcGetMatrixColumnDataType((_matrixOperand)->dataType); \
		(operand)->matrixIndex.mode			= slvINDEX_CONSTANT; \
		(operand)->matrixIndex.u.constant	= (gctREG_INDEX) (_matrixIndex); \
	} \
	while (gcvFALSE)

#define slsLOPERAND_InitializeAsMatrixComponent(operand, _matrixOperand, \
												_matrixIndex, _vectorIndex) \
	do \
	{ \
		gcmASSERT((_matrixOperand)->matrixIndex.mode == slvINDEX_NONE); \
		gcmASSERT((_matrixOperand)->vectorIndex.mode == slvINDEX_NONE); \
		\
		*(operand)							= *(_matrixOperand); \
		(operand)->dataType					= \
							gcGetComponentDataType((_matrixOperand)->dataType); \
		(operand)->matrixIndex.mode			= slvINDEX_CONSTANT; \
		(operand)->matrixIndex.u.constant	= (gctREG_INDEX) (_matrixIndex); \
		(operand)->vectorIndex.mode			= slvINDEX_CONSTANT; \
		(operand)->vectorIndex.u.constant	= (gctREG_INDEX) (_vectorIndex); \
	} \
	while (gcvFALSE)

#define slsLOPERAND_InitializeAsVectorComponent(operand, _vectorOperand, _vectorIndex) \
	do \
	{ \
		gcmASSERT((_vectorOperand)->vectorIndex.mode == slvINDEX_NONE); \
		\
		*(operand)							= *(_vectorOperand); \
		(operand)->dataType					= \
							gcGetVectorComponentDataType((_vectorOperand)->dataType); \
		(operand)->vectorIndex.mode			= slvINDEX_CONSTANT; \
		(operand)->vectorIndex.u.constant	= (gctREG_INDEX) (_vectorIndex); \
	} \
	while (gcvFALSE)

#define slsLOPERAND_InitializeUsingIOperand(operand, iOperand) \
	slsLOPERAND_InitializeTempReg( \
								(operand), \
								slvQUALIFIER_NONE, \
								(iOperand)->dataType, \
								(iOperand)->tempRegIndex)

#define slsLOPERAND_InitializeWithRegROPERAND(operand, _rOperand) \
	do \
	{ \
		gcmASSERT((_rOperand)->isReg); \
		(operand)->dataType					= (_rOperand)->dataType; \
		(operand)->reg                      = (_rOperand)->u.reg; \
		(operand)->vectorIndex              = (_rOperand)->vectorIndex; \
        (operand)->arrayIndex               = (_rOperand)->arrayIndex; \
        (operand)->matrixIndex              = (_rOperand)->matrixIndex; \
	} \
	while (gcvFALSE)

typedef struct _slsVEC2ARRAY
{
    slsNAME*            scalarArrayName;
    slsLOPERAND         vecOperand;
}
slsVEC2ARRAY;

typedef struct _slsOPERAND_CONSTANT
{
	gcSHADER_TYPE		dataType;

	gctUINT				valueCount;

	sluCONSTANT_VALUE	values[16];
}
slsOPERAND_CONSTANT;

typedef struct _slsROPERAND
{
	gcSHADER_TYPE		dataType;

	gctBOOL				isReg;

	union
	{
		slsLOGICAL_REG			reg;

		slsOPERAND_CONSTANT		constant;
	}	u;

	slsINDEX			arrayIndex;

	slsINDEX			matrixIndex;

	slsINDEX			vectorIndex;
}
slsROPERAND;

#define slsROPERAND_InitializeReg(operand, _reg) \
	do \
	{ \
		(operand)->dataType				= (_reg)->dataType; \
		(operand)->isReg				= gcvTRUE; \
		(operand)->u.reg				= *(_reg); \
		(operand)->arrayIndex.mode		= slvINDEX_NONE; \
		(operand)->matrixIndex.mode		= slvINDEX_NONE; \
		(operand)->vectorIndex.mode		= slvINDEX_NONE; \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeTempReg(operand, _qualifier, _dataType, _regIndex) \
	do \
	{ \
		(operand)->dataType				= (_dataType); \
		(operand)->isReg				= gcvTRUE; \
		slsLOGICAL_REG_InitializeTemp(&(operand)->u.reg, (_qualifier), (_dataType), (_regIndex)); \
		(operand)->arrayIndex.mode		= slvINDEX_NONE; \
		(operand)->matrixIndex.mode		= slvINDEX_NONE; \
		(operand)->vectorIndex.mode		= slvINDEX_NONE; \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeUsingIOperand(operand, iOperand) \
	slsROPERAND_InitializeTempReg( \
								(operand), \
								slvQUALIFIER_NONE, \
								(iOperand)->dataType, \
								(iOperand)->tempRegIndex)

#define slsROPERAND_InitializeConstant(operand, _dataType, _valueCount, _values) \
	do \
	{ \
		gctUINT _i; \
		\
		(operand)->dataType					= (_dataType); \
		(operand)->isReg					= gcvFALSE; \
		(operand)->u.constant.dataType		= (_dataType); \
		(operand)->u.constant.valueCount	= (_valueCount); \
		\
		for (_i = 0; _i < (_valueCount); _i++) \
		{ \
			(operand)->u.constant.values[_i]	= (_values)[_i]; \
		} \
		\
		(operand)->arrayIndex.mode			= slvINDEX_NONE; \
		(operand)->matrixIndex.mode			= slvINDEX_NONE; \
		(operand)->vectorIndex.mode			= slvINDEX_NONE; \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeFloatOrVecOrMatConstant(operand, _dataType, _floatValue) \
	do \
	{ \
		gctUINT _i; \
		gctUINT componentCount;\
		(operand)->dataType					= (_dataType); \
		(operand)->isReg					= gcvFALSE; \
		(operand)->u.constant.dataType		= (_dataType); \
		(operand)->u.constant.valueCount	= gcGetDataTypeComponentCount(_dataType); \
		componentCount = (operand)->u.constant.valueCount;\
		\
		for (_i = 0; _i < componentCount; _i++) \
		{ \
			(operand)->u.constant.values[_i].floatValue	= (_floatValue); \
		} \
		\
		(operand)->arrayIndex.mode			= slvINDEX_NONE; \
		(operand)->matrixIndex.mode			= slvINDEX_NONE; \
		(operand)->vectorIndex.mode			= slvINDEX_NONE; \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeIntOrIVecConstant(operand, _dataType, _intValue) \
	do \
	{ \
		gctUINT _i; \
		\
		(operand)->dataType					= (_dataType); \
		(operand)->isReg					= gcvFALSE; \
		(operand)->u.constant.dataType		= (_dataType); \
		(operand)->u.constant.valueCount	= gcGetDataTypeComponentCount(_dataType); \
		\
		for (_i = 0; _i < gcGetDataTypeComponentCount(_dataType); _i++) \
		{ \
			(operand)->u.constant.values[_i].intValue	= (_intValue); \
		} \
		\
		(operand)->arrayIndex.mode			= slvINDEX_NONE; \
		(operand)->matrixIndex.mode			= slvINDEX_NONE; \
		(operand)->vectorIndex.mode			= slvINDEX_NONE; \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeBoolOrBVecConstant(operand, _dataType, _boolValue) \
	do \
	{ \
		gctUINT _i; \
		\
		(operand)->dataType					= (_dataType); \
		(operand)->isReg					= gcvFALSE; \
		(operand)->u.constant.dataType		= (_dataType); \
		(operand)->u.constant.valueCount	= gcGetDataTypeComponentCount(_dataType); \
		\
		for (_i = 0; _i < gcGetDataTypeComponentCount(_dataType); _i++) \
		{ \
			(operand)->u.constant.values[_i].boolValue	= (_boolValue); \
		} \
		\
		(operand)->arrayIndex.mode			= slvINDEX_NONE; \
		(operand)->matrixIndex.mode			= slvINDEX_NONE; \
		(operand)->vectorIndex.mode			= slvINDEX_NONE; \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeAsMatrixColumn(operand, _matrixOperand, _matrixIndex) \
	do \
	{ \
		gcmASSERT((_matrixOperand)->matrixIndex.mode == slvINDEX_NONE); \
		\
		*(operand)							= *(_matrixOperand); \
		(operand)->dataType					= \
							gcGetMatrixColumnDataType((_matrixOperand)->dataType); \
		(operand)->matrixIndex.mode			= slvINDEX_CONSTANT; \
		(operand)->matrixIndex.u.constant	= (gctREG_INDEX) (_matrixIndex); \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeAsMatrixComponent(operand, _matrixOperand, \
												_matrixIndex, _vectorIndex) \
	do \
	{ \
		gcmASSERT((_matrixOperand)->matrixIndex.mode == slvINDEX_NONE); \
		gcmASSERT((_matrixOperand)->vectorIndex.mode == slvINDEX_NONE); \
		\
		*(operand)							= *(_matrixOperand); \
		(operand)->dataType					= \
							gcGetComponentDataType((_matrixOperand)->dataType); \
		(operand)->matrixIndex.mode			= slvINDEX_CONSTANT; \
		(operand)->matrixIndex.u.constant	= (gctREG_INDEX) (_matrixIndex); \
		(operand)->vectorIndex.mode			= slvINDEX_CONSTANT; \
		(operand)->vectorIndex.u.constant	= (gctREG_INDEX) (_vectorIndex); \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeAsVectorComponent(operand, _vectorOperand, _vectorIndex) \
	do \
	{ \
		gcmASSERT((_vectorOperand)->vectorIndex.mode == slvINDEX_NONE); \
		\
		*(operand)							= *(_vectorOperand); \
		(operand)->dataType					= \
							gcGetVectorComponentDataType((_vectorOperand)->dataType); \
		(operand)->vectorIndex.mode			= slvINDEX_CONSTANT; \
		(operand)->vectorIndex.u.constant	= (gctREG_INDEX) (_vectorIndex); \
	} \
	while (gcvFALSE)

#define slsROPERAND_InitializeWithLOPERAND(operand, _lOperand) \
	do \
	{ \
		(operand)->isReg					= gcvTRUE; \
		(operand)->dataType					= (_lOperand)->dataType; \
		(operand)->u.reg                    = (_lOperand)->reg; \
		(operand)->vectorIndex              = (_lOperand)->vectorIndex; \
        (operand)->arrayIndex               = (_lOperand)->arrayIndex; \
        (operand)->matrixIndex              = (_lOperand)->matrixIndex; \
	} \
	while (gcvFALSE)

gctBOOL
slsROPERAND_IsFloatOrVecConstant(
	IN slsROPERAND * ROperand,
	IN gctFLOAT FloatValue
	);

typedef struct _slsIOPERAND
{
	gcSHADER_TYPE			dataType;

	gctREG_INDEX			tempRegIndex;
}
slsIOPERAND;

#define slsIOPERAND_Initialize(operand, _dataType, _tempRegIndex) \
	do \
	{ \
		(operand)->dataType		= (_dataType); \
		(operand)->tempRegIndex	= (_tempRegIndex); \
	} \
	while (gcvFALSE)

#define slsIOPERAND_New(compiler, operand, _dataType) \
	do \
	{ \
		(operand)->dataType		= (_dataType); \
		(operand)->tempRegIndex	= slNewTempRegs((compiler), gcGetDataTypeSize(_dataType)); \
	} \
	while (gcvFALSE)

#define slsIOPERAND_InitializeAsMatrixColumn(operand, _matrixOperand, _matrixIndex) \
	do \
	{ \
		(operand)->dataType		= gcGetMatrixColumnDataType((_matrixOperand)->dataType); \
		(operand)->tempRegIndex	= (gctREG_INDEX) ((_matrixOperand)->tempRegIndex + (_matrixIndex)); \
	} \
	while (gcvFALSE)

gceSTATUS
slGenAssignCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN slsLOPERAND * LOperand,
	IN slsROPERAND * ROperand
	);

gceSTATUS
slGenArithmeticExprCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN slsIOPERAND * IOperand,
	IN slsROPERAND * ROperand0,
	IN slsROPERAND * ROperand1
	);

gceSTATUS
slGenGenericCode1(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN slsIOPERAND * IOperand,
	IN slsROPERAND * ROperand
	);

gceSTATUS
slGenGenericCode2(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN slsIOPERAND * IOperand,
	IN slsROPERAND * ROperand0,
	IN slsROPERAND * ROperand1
	);

gceSTATUS
slGenTestJumpCode(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctLABEL Label,
	IN gctBOOL TrueJump,
	IN slsROPERAND * ROperand
	);

gceSTATUS
slGenCompareJumpCode(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctLABEL Label,
	IN gctBOOL TrueJump,
	IN sleCONDITION CompareCondition,
	IN slsROPERAND * ROperand0,
	IN slsROPERAND * ROperand1
	);

typedef struct _slsSELECTION_CONTEXT
{
	gctBOOL							hasFalseOperand;

	gctLABEL						endLabel;

	gctLABEL						beginLabelOfFalseOperand;
}
slsSELECTION_CONTEXT;

gceSTATUS
slDefineSelectionBegin(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN gctBOOL HasFalseOperand,
	OUT slsSELECTION_CONTEXT * SelectionContext
	);

gceSTATUS
slDefineSelectionEnd(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN slsSELECTION_CONTEXT * SelectionContext
	);

gceSTATUS
slGenSelectionTestConditionCode(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN slsSELECTION_CONTEXT * SelectionContext,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN slsROPERAND * ROperand
	);

gceSTATUS
slGenSelectionCompareConditionCode(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN slsSELECTION_CONTEXT * SelectionContext,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleCONDITION CompareCondition,
	IN slsROPERAND * ROperand0,
	IN slsROPERAND * ROperand1
	);

gceSTATUS
slDefineSelectionTrueOperandBegin(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN slsSELECTION_CONTEXT * SelectionContext
	);

gceSTATUS
slDefineSelectionTrueOperandEnd(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN slsSELECTION_CONTEXT * SelectionContext,
	IN gctBOOL HasReturn
	);

gceSTATUS
slDefineSelectionFalseOperandBegin(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN slsSELECTION_CONTEXT * SelectionContext
	);

gceSTATUS
slDefineSelectionFalseOperandEnd(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN slsSELECTION_CONTEXT * SelectionContext
	);

gceSTATUS
slVSOuputPatch(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE* globalNameSpace
    );

typedef struct _slsITERATION_CONTEXT
{
    struct _slsITERATION_CONTEXT *	prevContext;

	gctBOOL							isUnrolled;

	union
	{
		struct
		{
			slsNAME *				loopIndexName;

			sluCONSTANT_VALUE		loopIndexValue;

			gctLABEL				bodyEndLabel;
		}
		unrolledInfo;

		struct
		{
			gctBOOL					isTestFirst;

			gctBOOL					hasRestExpr;

			gctLABEL				loopBeginLabel;

			gctLABEL				restBeginLabel;
		}
		genericInfo;
	}
	u;

	gctLABEL						endLabel;
}
slsITERATION_CONTEXT;

typedef struct _slsFUNC_DEF_CONTEXT
{
	gctBOOL				isMain;

	union
	{
		gctLABEL		mainEndLabel;

		sloIR_SET		funcBody;
	}
	u;
}
slsFUNC_DEF_CONTEXT;

/* CODE_GENERATOR */
typedef enum _sleGEN_CODE_HINT
{
	slvGEN_GENERIC_CODE,
	slvGEN_INDEX_CODE,

	slvEVALUATE_ONLY
}
sleGEN_CODE_HINT;

typedef struct _slsGEN_CODE_PARAMETERS
{
	gctBOOL			needLOperand;

	gctBOOL			needROperand;

	sleGEN_CODE_HINT	hint;

	sloIR_CONSTANT		constant;

	gctUINT			operandCount;

	gcSHADER_TYPE *		dataTypes;

	slsLOPERAND *		lOperands;

	slsROPERAND *		rOperands;

	gctBOOL			treatFloatAsInt;
}
slsGEN_CODE_PARAMETERS;

#define slsGEN_CODE_PARAMETERS_Initialize(parameters, _needLOperand, _needROperand) \
	do \
	{ \
		(parameters)->needLOperand		= (_needLOperand); \
		(parameters)->needROperand		= (_needROperand); \
		(parameters)->hint				= slvGEN_GENERIC_CODE; \
		(parameters)->constant			= gcvNULL; \
		(parameters)->operandCount		= 0; \
		(parameters)->dataTypes			= gcvNULL; \
		(parameters)->lOperands			= gcvNULL; \
		(parameters)->rOperands			= gcvNULL; \
		(parameters)->treatFloatAsInt	= gcvFALSE; \
	} \
	while (gcvFALSE)

#define slsGEN_CODE_PARAMETERS_Finalize(parameters) \
	do \
	{ \
		if ((parameters)->constant != gcvNULL) \
		{ \
			gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &(parameters)->constant->exprBase.base)); \
		} \
		\
		if ((parameters)->dataTypes != gcvNULL) \
		{ \
			gcmVERIFY_OK(sloCOMPILER_Free(Compiler, (parameters)->dataTypes)); \
		} \
		\
		if ((parameters)->lOperands != gcvNULL) \
		{ \
			gcmVERIFY_OK(sloCOMPILER_Free(Compiler, (parameters)->lOperands)); \
		} \
		\
		if ((parameters)->rOperands != gcvNULL) \
		{ \
			gcmVERIFY_OK(sloCOMPILER_Free(Compiler, (parameters)->rOperands)); \
		} \
	} \
	while (gcvFALSE)

#define slsGEN_CODE_PARAMETERS_MoveOperands(parameters0, parameters1) \
	do \
	{ \
		*(parameters0)				= *(parameters1); \
		(parameters1)->dataTypes	= gcvNULL; \
		(parameters1)->lOperands	= gcvNULL; \
		(parameters1)->rOperands	= gcvNULL; \
	} \
	while (gcvFALSE)

/* sloCODE_GENERATOR */
struct _sloCODE_GENERATOR
{
	slsVISITOR		visitor;
	slsFUNC_DEF_CONTEXT	currentFuncDefContext;
	slsITERATION_CONTEXT *	currentIterationContext;

        gctUINT                 attributeCount;
        gctUINT                 uniformCount;
        gctUINT                 variableCount;
        gctUINT                 outputCount;
        gctUINT                 functionCount;
};

gceSTATUS
sloIR_ROperandComponentSelect(
    IN sloCOMPILER Compiler,
    IN slsROPERAND *From,
    IN slsCOMPONENT_SELECTION ComponentSelection,
    OUT slsROPERAND *To
    );

gceSTATUS
sloIR_LOperandComponentSelect(
    IN sloCOMPILER Compiler,
    IN slsLOPERAND *From,
    IN slsCOMPONENT_SELECTION ComponentSelection,
    OUT slsLOPERAND *To
    );

gceSTATUS
sloIR_AllocObjectPointerArrays(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
;

gceSTATUS
sloCODE_GENERATOR_Construct(
	IN sloCOMPILER Compiler,
	OUT sloCODE_GENERATOR * CodeGenerator
	);

gceSTATUS
sloCODE_GENERATOR_Destroy(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator
	);

#endif /* __gc_glsl_gen_code_h_ */
