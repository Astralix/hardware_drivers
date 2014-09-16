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


#ifndef __gc_glsl_built_ins_h_
#define __gc_glsl_built_ins_h_

#include "gc_glsl_gen_code.h"
#include "gc_glsl_emit_code.h"

#define slmMAX_BUILT_IN_PARAMETER_COUNT		3

gceSTATUS
slLoadBuiltIns(
	IN sloCOMPILER Compiler,
	IN sleSHADER_TYPE ShaderType
	);

gceSTATUS
slGetBuiltInVariableImplSymbol(
	IN gctCONST_STRING Symbol,
	OUT gctCONST_STRING * ImplSymbol,
	OUT sltQUALIFIER * ImplQualifier
	);

gctBOOL
slIsTextureLookupFunction(
	IN gctCONST_STRING Symbol
	);

gceSTATUS
slEvaluateBuiltInFunction(
	IN sloCOMPILER Compiler,
	IN sloIR_POLYNARY_EXPR PolynaryExpr,
	IN gctUINT OperandCount,
	IN sloIR_CONSTANT * OperandConstants,
	OUT sloIR_CONSTANT * ResultConstant
	);

gceSTATUS
slGenBuiltInFunctionCode(
	IN sloCOMPILER Compiler,
	IN sloCODE_GENERATOR CodeGenerator,
	IN sloIR_POLYNARY_EXPR PolynaryExpr,
	IN gctUINT OperandCount,
	IN slsGEN_CODE_PARAMETERS * OperandsParameters,
	IN slsIOPERAND * IOperand,
	IN OUT slsGEN_CODE_PARAMETERS * Parameters
	);

#endif /* __gc_glsl_built_ins_h_ */
