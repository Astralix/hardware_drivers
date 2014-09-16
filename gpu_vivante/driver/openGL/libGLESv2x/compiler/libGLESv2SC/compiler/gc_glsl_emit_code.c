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


#include "gc_glsl_emit_code.h"

gceSTATUS
sloCODE_EMITTER_NewBasicBlock(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter
	);

gceSTATUS
sloCODE_EMITTER_EndBasicBlock(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter
	);

gceSTATUS
sloCODE_EMITTER_EmitCode1(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	);

gceSTATUS
sloCODE_EMITTER_EmitCode2(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	);

gctSIZE_T
gcGetDataTypeSize(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X1:
	case gcSHADER_FLOAT_X2:
	case gcSHADER_FLOAT_X3:
	case gcSHADER_FLOAT_X4:
	case gcSHADER_BOOLEAN_X1:
	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_BOOLEAN_X4:
	case gcSHADER_INTEGER_X1:
	case gcSHADER_INTEGER_X2:
	case gcSHADER_INTEGER_X3:
	case gcSHADER_INTEGER_X4:
	case gcSHADER_SAMPLER_1D:
	case gcSHADER_SAMPLER_2D:
	case gcSHADER_SAMPLER_3D:
	case gcSHADER_SAMPLER_CUBIC:
	case gcSHADER_SAMPLER_EXTERNAL_OES:
		return 1;

	case gcSHADER_FLOAT_2X2:
		return 2;

	case gcSHADER_FLOAT_3X3:
		return 3;

	case gcSHADER_FLOAT_4X4:
		return 4;

    default:
		gcmASSERT(0);
		return 1;
    }
}

gctUINT8
gcGetDataTypeComponentCount(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
    case gcSHADER_FLOAT_X1:
	case gcSHADER_BOOLEAN_X1:
	case gcSHADER_INTEGER_X1:
		return 1;

	case gcSHADER_FLOAT_X2:
	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_INTEGER_X2:
		return 2;

	case gcSHADER_FLOAT_X3:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_INTEGER_X3:
		return 3;

	case gcSHADER_FLOAT_X4:
	case gcSHADER_BOOLEAN_X4:
	case gcSHADER_INTEGER_X4:
	case gcSHADER_SAMPLER_1D:
	case gcSHADER_SAMPLER_2D:
	case gcSHADER_SAMPLER_3D:
	case gcSHADER_SAMPLER_CUBIC:
	case gcSHADER_SAMPLER_EXTERNAL_OES:
		return 4;

	case gcSHADER_FLOAT_2X2:
		return 4;

	case gcSHADER_FLOAT_3X3:
		return 9;

	case gcSHADER_FLOAT_4X4:
		return 16;

    default:
		gcmASSERT(0);
		return 1;
    }
}

gcSHADER_TYPE
gcGetComponentDataType(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X1:
	case gcSHADER_FLOAT_X2:
	case gcSHADER_FLOAT_X3:
	case gcSHADER_FLOAT_X4:
	case gcSHADER_FLOAT_2X2:
	case gcSHADER_FLOAT_3X3:
	case gcSHADER_FLOAT_4X4:
		return gcSHADER_FLOAT_X1;

	case gcSHADER_BOOLEAN_X1:
	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_BOOLEAN_X4:
		return gcSHADER_BOOLEAN_X1;

	case gcSHADER_INTEGER_X1:
	case gcSHADER_INTEGER_X2:
	case gcSHADER_INTEGER_X3:
	case gcSHADER_INTEGER_X4:
		return gcSHADER_INTEGER_X1;

    default:
		gcmASSERT(0);
		return gcSHADER_FLOAT_X1;
    }
}

static gctCONST_STRING
_GetOpcodeName(
	IN gcSL_OPCODE Opcode
	)
{
    switch (Opcode)
	{
    case gcSL_NOP:      return "gcSL_NOP";
	case gcSL_MOV:      return "gcSL_MOV";
	case gcSL_SAT:      return "gcSL_SAT";
	case gcSL_DP3:      return "gcSL_DP3";
	case gcSL_DP4:      return "gcSL_DP4";
	case gcSL_ABS:      return "gcSL_ABS";
	case gcSL_JMP:      return "gcSL_JMP";
	case gcSL_ADD:      return "gcSL_ADD";
	case gcSL_MUL:      return "gcSL_MUL";
	case gcSL_RCP:      return "gcSL_RCP";
	case gcSL_SUB:      return "gcSL_SUB";
	case gcSL_KILL:     return "gcSL_KILL";
	case gcSL_TEXLD:    return "gcSL_TEXLD";
	case gcSL_CALL:     return "gcSL_CALL";
	case gcSL_RET:      return "gcSL_RET";
	case gcSL_NORM:     return "gcSL_NORM";
	case gcSL_MAX:      return "gcSL_MAX";
	case gcSL_MIN:      return "gcSL_MIN";
	case gcSL_POW:      return "gcSL_POW";
	case gcSL_RSQ:      return "gcSL_RSQ";
	case gcSL_LOG:      return "gcSL_LOG";
	case gcSL_FRAC:     return "gcSL_FRAC";
	case gcSL_FLOOR:    return "gcSL_FLOOR";
	case gcSL_CEIL:     return "gcSL_CEIL";
	case gcSL_CROSS:    return "gcSL_CROSS";
	case gcSL_TEXLDP:   return "gcSL_TEXLDP";
	case gcSL_TEXBIAS:  return "gcSL_TEXBIAS";
    case gcSL_TEXGRAD:	return "gcSL_TEXGRAD";
    case gcSL_TEXLOD:	return "gcSL_TEXLOD";
	case gcSL_SIN:		return "gcSL_SIN";
	case gcSL_COS:		return "gcSL_COS";
	case gcSL_TAN:		return "gcSL_TAN";
	case gcSL_EXP:		return "gcSL_EXP";
	case gcSL_SIGN:		return "gcSL_SIGN";
	case gcSL_STEP:		return "gcSL_STEP";
	case gcSL_SQRT:		return "gcSL_SQRT";
	case gcSL_ACOS:		return "gcSL_ACOS";
	case gcSL_ASIN:		return "gcSL_ASIN";
	case gcSL_ATAN:		return "gcSL_ATAN";
	case gcSL_SET:		return "gcSL_SET";
	case gcSL_DSX:		return "gcSL_DSX";
	case gcSL_DSY:		return "gcSL_DSY";
	case gcSL_FWIDTH:	return "gcSL_FWIDTH";

    default:
		gcmASSERT(0);
		return "Invalid";
    }
}

static gctCONST_STRING
_GetConditionName(
	IN gcSL_CONDITION Condition
	)
{
    switch (Condition)
	{
    case gcSL_ALWAYS:           return "gcSL_ALWAYS";
	case gcSL_NOT_EQUAL:        return "gcSL_NOT_EQUAL";
	case gcSL_LESS_OR_EQUAL:    return "gcSL_LESS_OR_EQUAL";
	case gcSL_LESS:             return "gcSL_LESS";
	case gcSL_EQUAL:            return "gcSL_EQUAL";
	case gcSL_GREATER:          return "gcSL_GREATER";
	case gcSL_GREATER_OR_EQUAL: return "gcSL_GREATER_OR_EQUAL";
	case gcSL_AND:              return "gcSL_AND";
	case gcSL_OR:               return "gcSL_OR";
	case gcSL_XOR:              return "gcSL_XOR";

    default:
		gcmASSERT(0);
		return "Invalid";
    }
}

static gctCONST_STRING
_GetEnableName(
	IN gctUINT8 Enable,
	OUT gctCHAR buf[5]
	)
{
    gctINT i = 0;

    if (Enable & gcSL_ENABLE_X) buf[i++] = 'X';
    if (Enable & gcSL_ENABLE_Y) buf[i++] = 'Y';
    if (Enable & gcSL_ENABLE_Z) buf[i++] = 'Z';
    if (Enable & gcSL_ENABLE_W) buf[i++] = 'W';

    gcmASSERT(i > 0);
    buf[i] = '\0';

    return buf;
}

static gctCHAR
_GetSwizzleChar(
	IN gctUINT8 Swizzle
	)
{
	switch (Swizzle)
	{
	case gcSL_SWIZZLE_X: return 'X';
    case gcSL_SWIZZLE_Y: return 'Y';
    case gcSL_SWIZZLE_Z: return 'Z';
    case gcSL_SWIZZLE_W: return 'W';

	default:
		gcmASSERT(0);
		return 'X';
	}
}

static gctCONST_STRING
_GetSwizzleName(
	IN gctUINT8 Swizzle,
	OUT gctCHAR buf[5]
	)
{
    buf[0] = _GetSwizzleChar((Swizzle >> 0) & 3);
    buf[1] = _GetSwizzleChar((Swizzle >> 2) & 3);
    buf[2] = _GetSwizzleChar((Swizzle >> 4) & 3);
    buf[3] = _GetSwizzleChar((Swizzle >> 6) & 3);
    buf[4] = '\0';

    return buf;
}

static gctCONST_STRING
_GetIndexModeName(
	IN gcSL_INDEXED IndexMode
	)
{
    switch (IndexMode)
	{
    case gcSL_NOT_INDEXED:  return "gcSL_NOT_INDEXED";
	case gcSL_INDEXED_X:    return "gcSL_INDEXED_X";
	case gcSL_INDEXED_Y:    return "gcSL_INDEXED_Y";
	case gcSL_INDEXED_Z:    return "gcSL_INDEXED_Z";
	case gcSL_INDEXED_W:    return "gcSL_INDEXED_W";

    default:
		gcmASSERT(0);
		return "Invalid";
    }
}

static gctCONST_STRING
_GetTypeName(
	IN gcSL_TYPE Type
	)
{
    switch (Type)
	{
    case gcSL_NONE:			return "gcSL_NONE";
	case gcSL_TEMP:			return "gcSL_TEMP";
	case gcSL_ATTRIBUTE:	return "gcSL_ATTRIBUTE";
	case gcSL_UNIFORM:		return "gcSL_UNIFORM";
	case gcSL_SAMPLER:		return "gcSL_SAMPLER";
	case gcSL_CONSTANT:		return "gcSL_CONSTANT";

    default:
		gcmASSERT(0);
		return "Invalid";
    }
}

gctCONST_STRING
gcGetDataTypeName(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X1:			return "gcSHADER_FLOAT_X1";
	case gcSHADER_FLOAT_X2:			return "gcSHADER_FLOAT_X2";
	case gcSHADER_FLOAT_X3:			return "gcSHADER_FLOAT_X3";
	case gcSHADER_FLOAT_X4:			return "gcSHADER_FLOAT_X4";
	case gcSHADER_FLOAT_2X2:		return "gcSHADER_FLOAT_2X2";
	case gcSHADER_FLOAT_3X3:		return "gcSHADER_FLOAT_3X3";
	case gcSHADER_FLOAT_4X4:		return "gcSHADER_FLOAT_4X4";
	case gcSHADER_BOOLEAN_X1:		return "gcSHADER_BOOLEAN_X1";
	case gcSHADER_BOOLEAN_X2:		return "gcSHADER_BOOLEAN_X2";
	case gcSHADER_BOOLEAN_X3:		return "gcSHADER_BOOLEAN_X3";
	case gcSHADER_BOOLEAN_X4:		return "gcSHADER_BOOLEAN_X4";
	case gcSHADER_INTEGER_X1:		return "gcSHADER_INTEGER_X1";
	case gcSHADER_INTEGER_X2:		return "gcSHADER_INTEGER_X2";
	case gcSHADER_INTEGER_X3:		return "gcSHADER_INTEGER_X3";
	case gcSHADER_INTEGER_X4:		return "gcSHADER_INTEGER_X4";
	case gcSHADER_SAMPLER_1D:		return "gcSHADER_SAMPLER_1D";
	case gcSHADER_SAMPLER_2D:		return "gcSHADER_SAMPLER_2D";
	case gcSHADER_SAMPLER_3D:		return "gcSHADER_SAMPLER_3D";
	case gcSHADER_SAMPLER_CUBIC:		return "gcSHADER_SAMPLER_CUBIC";
	case gcSHADER_SAMPLER_EXTERNAL_OES:	return "gcSHADER_SAMPLER_EXTERNAL_OES";

    default:
		gcmASSERT(0);
		return "Invalid";
    }
}

static gctCONST_STRING
_GetFormatName(
	IN gcSL_FORMAT Format
	)
{
	switch (Format)
	{
	case gcSL_FLOAT:	return "gcSL_FLOAT";
    case gcSL_INTEGER:	return "gcSL_INTEGER";
    case gcSL_BOOLEAN:	return "gcSL_BOOLEAN";

	default:
		gcmASSERT(0);
		return "Invalid";
	}
}

static gctCONST_STRING
_GetQualifierName(
	IN gceINPUT_OUTPUT Qualifier
	)
{
	switch (Qualifier)
	{
	case gcvFUNCTION_INPUT: return "gcvFUNCTION_INPUT";
    case gcvFUNCTION_OUTPUT: return "gcvFUNCTION_OUTPUT";
    case gcvFUNCTION_INOUT: return "gcvFUNCTION_INOUT";

	default:
		gcmASSERT(0);
		return "Invalid";
	}
}

gctBOOL
gcIsSamplerDataType(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_SAMPLER_1D:
	case gcSHADER_SAMPLER_2D:
	case gcSHADER_SAMPLER_3D:
	case gcSHADER_SAMPLER_CUBIC:
	case gcSHADER_ISAMPLER_2D:
	case gcSHADER_ISAMPLER_3D:
	case gcSHADER_ISAMPLER_CUBIC:
	case gcSHADER_USAMPLER_2D:
	case gcSHADER_USAMPLER_3D:
	case gcSHADER_USAMPLER_CUBIC:
	case gcSHADER_SAMPLER_EXTERNAL_OES:
		return gcvTRUE;

    default:
		return gcvFALSE;
    }
}

gctBOOL
gcIsScalarDataType(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
    case gcSHADER_FLOAT_X1:
	case gcSHADER_BOOLEAN_X1:
	case gcSHADER_INTEGER_X1:
		return gcvTRUE;

    default:
		return gcvFALSE;
    }
}

gctBOOL
gcIsVectorDataType(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X2:
	case gcSHADER_FLOAT_X3:
	case gcSHADER_FLOAT_X4:
	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_BOOLEAN_X4:
	case gcSHADER_INTEGER_X2:
	case gcSHADER_INTEGER_X3:
	case gcSHADER_INTEGER_X4:
		return gcvTRUE;

    default:
		return gcvFALSE;
    }
}

gctBOOL
gcIsMatrixDataType(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_2X2:
	case gcSHADER_FLOAT_3X3:
	case gcSHADER_FLOAT_4X4:
		return gcvTRUE;

    default:
		return gcvFALSE;
    }
}

gcSHADER_TYPE
gcGetVectorComponentDataType(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X2:
	case gcSHADER_FLOAT_X3:
	case gcSHADER_FLOAT_X4:
		return gcSHADER_FLOAT_X1;

	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_BOOLEAN_X4:
		return gcSHADER_BOOLEAN_X1;

	case gcSHADER_INTEGER_X2:
	case gcSHADER_INTEGER_X3:
	case gcSHADER_INTEGER_X4:
		return gcSHADER_INTEGER_X1;

    default:
		gcmASSERT(0);
		return gcSHADER_FLOAT_X1;
    }
}

gcSHADER_TYPE
gcGetVectorSliceDataType(
	IN gcSHADER_TYPE DataType,
	IN gctUINT8 Components
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X2:
	case gcSHADER_FLOAT_X3:
	case gcSHADER_FLOAT_X4:
		switch (Components)
		{
		case 1: return gcSHADER_FLOAT_X1;
		case 2: return gcSHADER_FLOAT_X2;
		case 3: return gcSHADER_FLOAT_X3;
		case 4: return gcSHADER_FLOAT_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_FLOAT_X1;
		}

	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_BOOLEAN_X4:
		switch (Components)
		{
		case 1: return gcSHADER_BOOLEAN_X1;
		case 2: return gcSHADER_BOOLEAN_X2;
		case 3: return gcSHADER_BOOLEAN_X3;
		case 4: return gcSHADER_BOOLEAN_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_BOOLEAN_X1;
		}

	case gcSHADER_INTEGER_X2:
	case gcSHADER_INTEGER_X3:
	case gcSHADER_INTEGER_X4:
		switch (Components)
		{
		case 1: return gcSHADER_INTEGER_X1;
		case 2: return gcSHADER_INTEGER_X2;
		case 3: return gcSHADER_INTEGER_X3;
		case 4: return gcSHADER_INTEGER_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_INTEGER_X1;
		}

    default:
		gcmASSERT(0);
		return gcSHADER_FLOAT_X1;
    }
}

gcSHADER_TYPE
gcConvScalarToVectorDataType(
	IN gcSHADER_TYPE DataType,
	IN gctUINT8 Components
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X1:
		switch (Components)
		{
		case 2: return gcSHADER_FLOAT_X2;
		case 3: return gcSHADER_FLOAT_X3;
		case 4: return gcSHADER_FLOAT_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_FLOAT_X4;
		}

	case gcSHADER_BOOLEAN_X1:
		switch (Components)
		{
		case 2: return gcSHADER_BOOLEAN_X2;
		case 3: return gcSHADER_BOOLEAN_X3;
		case 4: return gcSHADER_BOOLEAN_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_BOOLEAN_X4;
		}

	case gcSHADER_INTEGER_X1:
		switch (Components)
		{
		case 2: return gcSHADER_INTEGER_X2;
		case 3: return gcSHADER_INTEGER_X3;
		case 4: return gcSHADER_INTEGER_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_INTEGER_X4;
		}

    default:
		gcmASSERT(0);
		return gcSHADER_FLOAT_X4;
    }
}

gctUINT8
gcGetVectorDataTypeComponentCount(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X2:
	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_INTEGER_X2:
		return 2;

	case gcSHADER_FLOAT_X3:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_INTEGER_X3:
		return 3;

	case gcSHADER_FLOAT_X4:
	case gcSHADER_BOOLEAN_X4:
	case gcSHADER_INTEGER_X4:
		return 4;

    default:
		gcmASSERT(0);
		return 4;
    }
}

gcSHADER_TYPE
gcGetVectorComponentSelectionDataType(
	IN gcSHADER_TYPE DataType,
	IN gctUINT8	Components
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_X2:
	case gcSHADER_FLOAT_X3:
	case gcSHADER_FLOAT_X4:
		switch (Components)
		{
		case 1: return gcSHADER_FLOAT_X1;
		case 2: return gcSHADER_FLOAT_X2;
		case 3: return gcSHADER_FLOAT_X3;
		case 4: return gcSHADER_FLOAT_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_FLOAT_X1;
		}

	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_BOOLEAN_X4:
		switch (Components)
		{
		case 1: return gcSHADER_BOOLEAN_X1;
		case 2: return gcSHADER_BOOLEAN_X2;
		case 3: return gcSHADER_BOOLEAN_X3;
		case 4: return gcSHADER_BOOLEAN_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_BOOLEAN_X1;
		}

	case gcSHADER_INTEGER_X2:
	case gcSHADER_INTEGER_X3:
	case gcSHADER_INTEGER_X4:
		switch (Components)
		{
		case 1: return gcSHADER_INTEGER_X1;
		case 2: return gcSHADER_INTEGER_X2;
		case 3: return gcSHADER_INTEGER_X3;
		case 4: return gcSHADER_INTEGER_X4;

		default:
			gcmASSERT(0);
			return gcSHADER_INTEGER_X1;
		}

    default:
		gcmASSERT(0);
		return gcSHADER_FLOAT_X1;
    }
}

gcSHADER_TYPE
gcGetMatrixColumnDataType(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_2X2:
		return gcSHADER_FLOAT_X2;

	case gcSHADER_FLOAT_3X3:
		return gcSHADER_FLOAT_X3;

	case gcSHADER_FLOAT_4X4:
		return gcSHADER_FLOAT_X4;

    default:
		gcmASSERT(0);
		return gcSHADER_FLOAT_X4;
    }
}

gctUINT
gcGetMatrixDataTypeColumnCount(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
	case gcSHADER_FLOAT_2X2:
		return 2;

	case gcSHADER_FLOAT_3X3:
		return 3;

	case gcSHADER_FLOAT_4X4:
		return 4;

    default:
		gcmASSERT(0);
		return 4;
    }
}

static gceSTATUS
_AddAttribute(
	IN sloCOMPILER Compiler,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE Type,
	IN gctSIZE_T Length,
	IN gctBOOL IsTexture,
	OUT gcATTRIBUTE * Attribute
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmASSERT(Attribute);

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddAttribute(Shader, \"%s\", %s, %d, %s);",
								Name,
								gcGetDataTypeName(Type),
								Length,
								(IsTexture)? "true" : "false"));

    status = gcSHADER_AddAttribute(binary, Name, Type, Length, IsTexture, Attribute);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddUniform(
	IN sloCOMPILER Compiler,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE Type,
    IN gcSHADER_PRECISION precision,
	IN gctSIZE_T Length,
	OUT gcUNIFORM * Uniform
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmASSERT(Uniform);

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddUniform(Shader, \"%s\", %s, %d);",
								Name,
								gcGetDataTypeName(Type),
								Length));

    status = gcSHADER_AddUniformEx(binary, Name, Type, precision, Length, Uniform);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddOutput(
	IN sloCOMPILER Compiler,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE Type,
	IN gctSIZE_T Length,
	IN gctUINT16 TempRegister
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddOutput(Shader, \"%s\", %s, %d, %d);",
								Name,
								gcGetDataTypeName(Type),
								Length,
								TempRegister));

    status = gcSHADER_AddOutput(binary, Name, Type, Length, TempRegister);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddOutputIndexed(
	IN sloCOMPILER Compiler,
	IN gctCONST_STRING Name,
	IN gctSIZE_T Index,
	IN gctUINT16 TempIndex
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddOutputIndexed(Shader, \"%s\", %d, %d);",
								Name,
								Index,
								TempIndex));

    status = gcSHADER_AddOutputIndexed(binary, Name, Index, TempIndex);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddVariable(
	IN sloCOMPILER Compiler,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE Type,
	IN gctSIZE_T Length,
	IN gctUINT16 TempRegister,
    IN gcSHADER_VAR_CATEGORY varCategory,
    IN gctUINT16 numStructureElement,
    IN gctINT16 parent,
    IN gctINT16 prevSibling,
    OUT gctINT16* ThisVarIndex
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddVariableEx(Shader, \"%s\", %s, %d, %d);",
								Name,
								gcGetDataTypeName(Type),
								Length,
								TempRegister));

    status = gcSHADER_AddVariableEx(binary,
                                    Name,
                                    Type,
                                    Length,
                                    TempRegister,
                                    varCategory,
                                    numStructureElement,
                                    parent,
                                    prevSibling,
                                    ThisVarIndex);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_UpdateVariableTempReg(
	IN sloCOMPILER Compiler,
	IN gctUINT varIndex,
	IN gctREG_INDEX newTempRegIndex
	)
{
    gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

    gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	status = gcSHADER_UpdateVariable(binary,
                                     varIndex,
                                     gceVARIABLE_UPDATE_TEMPREG,
                                     newTempRegIndex);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddOpcode(
	IN sloCOMPILER Compiler,
	IN gcSL_OPCODE Opcode,
	IN gctUINT16 TempRegister,
	IN gctUINT8 Enable
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddOpcode(Shader, %s, %d, gcSL_ENABLE_%s, %s);",
								_GetOpcodeName(Opcode),
								TempRegister,
								_GetEnableName(Enable, buf),
								_GetFormatName(gcSL_FLOAT)));

    status = gcSHADER_AddOpcode(binary, Opcode, TempRegister, Enable, gcSL_FLOAT);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddOpcodeIndexed(
	IN sloCOMPILER Compiler,
	IN gcSL_OPCODE Opcode,
	IN gctUINT16 TempRegister,
	IN gctUINT8 Enable,
	IN gcSL_INDEXED Mode,
	IN gctUINT16 IndexRegister
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddOpcodeIndexed(Shader, %s, %d, gcSL_ENABLE_%s, %s, %d, %s);",
								_GetOpcodeName(Opcode),
								TempRegister,
								_GetEnableName(Enable, buf),
								_GetIndexModeName(Mode),
								IndexRegister,
								_GetFormatName(gcSL_FLOAT)));

    status = gcSHADER_AddOpcodeIndexed(binary, Opcode, TempRegister, Enable, Mode, IndexRegister, gcSL_FLOAT);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddSource(
	IN sloCOMPILER Compiler,
	IN gcSL_TYPE Type,
	IN gctUINT16 SourceIndex,
	IN gctUINT8 Swizzle
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddSource(Shader, %s, %d, gcSL_SWIZZLE_%s, %s);",
								_GetTypeName(Type),
								SourceIndex,
								_GetSwizzleName(Swizzle, buf),
								_GetFormatName(gcSL_FLOAT)));

    status = gcSHADER_AddSource(binary, Type, SourceIndex, Swizzle, gcSL_FLOAT);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddSourceIndexed(
	IN sloCOMPILER Compiler,
	IN gcSL_TYPE Type,
	IN gctUINT16 SourceIndex,
	IN gctUINT8 Swizzle,
	IN gcSL_INDEXED Mode,
	IN gctUINT16 IndexRegister
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddSourceIndexed(Shader, %s, %d, gcSL_SWIZZLE_%s, %s, %d, %s);",
								_GetTypeName(Type),
								SourceIndex,
								_GetSwizzleName(Swizzle, buf),
								_GetIndexModeName(Mode),
								IndexRegister,
								_GetFormatName(gcSL_FLOAT)));

    status = gcSHADER_AddSourceIndexed(binary, Type, SourceIndex, Swizzle, Mode, IndexRegister, gcSL_FLOAT);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddSourceSamplerIndexed(
	IN sloCOMPILER Compiler,
	IN gctUINT8 Swizzle,
	IN gcSL_INDEXED Mode,
	IN gctUINT16 IndexRegister
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddSourceSamplerIndexed(Shader,"
								" gcSL_SWIZZLE_%s, %s, %d);",
								_GetSwizzleName(Swizzle, buf),
								_GetIndexModeName(Mode),
								IndexRegister));

    status = gcSHADER_AddSourceSamplerIndexed(binary, Swizzle, Mode, IndexRegister);

    gcmFOOTER();
    return status;
}

gctCONST_STRING
gcGetAttributeName(
	IN gcATTRIBUTE Attribute
	)
{
	gctCONST_STRING	attributeName;

	gcmVERIFY_OK(gcATTRIBUTE_GetName(Attribute, gcvNULL, &attributeName));

	return attributeName;
}

static gceSTATUS
_AddSourceAttribute(
	IN sloCOMPILER Compiler,
	IN gcATTRIBUTE Attribute,
	IN gctUINT8 Swizzle,
	IN gctINT Index
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddSourceAttribute(Shader, \"%s\","
								" gcSL_SWIZZLE_%s, %d);",
								gcGetAttributeName(Attribute),
								_GetSwizzleName(Swizzle, buf),
								Index));

    status = gcSHADER_AddSourceAttribute(binary, Attribute, Swizzle, Index);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddSourceAttributeIndexed(
	IN sloCOMPILER Compiler,
	IN gcATTRIBUTE Attribute,
	IN gctUINT8 Swizzle,
	IN gctINT Index,
	IN gcSL_INDEXED Mode,
	IN gctUINT16 IndexRegister
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddSourceAttributeIndexed(Shader, \"%s\","
								" gcSL_SWIZZLE_%s, %d, %s, %d);",
								gcGetAttributeName(Attribute),
								_GetSwizzleName(Swizzle, buf),
								Index,
								_GetIndexModeName(Mode),
								IndexRegister));

    status = gcSHADER_AddSourceAttributeIndexed(binary, Attribute, Swizzle, Index, Mode, IndexRegister);

    gcmFOOTER();
    return status;
}

gctCONST_STRING
gcGetUniformName(
	IN gcUNIFORM Uniform
	)
{
	gctCONST_STRING	uniformName;

	gcmVERIFY_OK(gcUNIFORM_GetName(Uniform, gcvNULL, &uniformName));

	return uniformName;
}


static gceSTATUS
_GetSampler(
	IN sloCOMPILER Compiler,
	IN gcUNIFORM Uniform,
	OUT gctUINT32 * Sampler
	)
{
    gceSTATUS   status;

    gcmHEADER();

	gcmASSERT(Uniform);

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcUNIFORM_GetSampler(\"%s\", Sampler);",
								gcGetUniformName(Uniform)));

	status = gcUNIFORM_GetSampler(Uniform, Sampler);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddSourceUniform(
	IN sloCOMPILER Compiler,
	IN gcUNIFORM Uniform,
	IN gctUINT8 Swizzle,
	IN gctINT Index
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddSourceUniform(Shader, \"%s\","
								" gcSL_SWIZZLE_%s, %d);",
								gcGetUniformName(Uniform),
								_GetSwizzleName(Swizzle, buf),
								Index));

    status = gcSHADER_AddSourceUniform(binary, Uniform, Swizzle, Index);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddSourceUniformIndexed(
	IN sloCOMPILER Compiler,
	IN gcUNIFORM Uniform,
	IN gctUINT8 Swizzle,
	IN gctINT Index,
	IN gcSL_INDEXED Mode,
	IN gctUINT16 IndexRegister
	)
{
	gcSHADER	binary;
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddSourceUniformIndexed(Shader, \"%s\","
								" gcSL_SWIZZLE_%s, %d, %s, %d);",
								gcGetUniformName(Uniform),
								_GetSwizzleName(Swizzle, buf),
								Index,
								_GetIndexModeName(Mode),
								IndexRegister));

    status = gcSHADER_AddSourceUniformIndexed(binary, Uniform, Swizzle, Index, Mode, IndexRegister);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddSourceConstant(
	IN sloCOMPILER Compiler,
	IN gctFLOAT Constant
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddSourceConstant(Shader, %f);",
								Constant));

    status = gcSHADER_AddSourceConstant(binary, Constant);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddLabel(
	IN sloCOMPILER Compiler,
	IN gctUINT Label
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddLabel(Shader, %d);",
								Label));

    status = gcSHADER_AddLabel(binary, Label);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddOpcodeConditional(
	IN sloCOMPILER Compiler,
	IN gcSL_OPCODE Opcode,
	IN gcSL_CONDITION Condition,
	IN gctUINT Label
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddOpcodeConditional(Shader, %s, %s, %d);",
								_GetOpcodeName(Opcode),
								_GetConditionName(Condition),
								Label));

    status = gcSHADER_AddOpcodeConditional(binary, Opcode, Condition, Label);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddFunction(
	IN sloCOMPILER Compiler,
	IN gctCONST_STRING Name,
	OUT gcFUNCTION * Function
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmASSERT(Function);

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_AddFunction(Shader, \"%s\");",
								Name));

	status = gcSHADER_AddFunction(binary, Name, Function);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_BeginFunction(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_BeginFunction(Shader);"));

	status = gcSHADER_BeginFunction(binary, Function);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_EndFunction(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function
	)
{
	gcSHADER	binary;
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &binary));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcSHADER_EndFunction(Shader);"));

	status = gcSHADER_EndFunction(binary, Function);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_AddFunctionArgument(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function,
	IN gctUINT16 TempIndex,
	IN gctUINT8 Enable,
	IN gctUINT8 Qualifier
	)
{
	gctCHAR		buf[5];
    gceSTATUS   status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcFUNCTION_AddArgument(Function, %d, gcSL_ENABLE_%s, %s);",
								TempIndex,
								_GetEnableName(Enable, buf),
								_GetQualifierName((gceINPUT_OUTPUT) Qualifier)));

	status = gcFUNCTION_AddArgument(Function, TempIndex, Enable, Qualifier);

    gcmFOOTER();
    return status;
}

gceSTATUS
_GetFunctionLabel(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function,
	OUT gctUINT_PTR Label
	)
{
    gceSTATUS status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"gcFUNCTION_GetLabel(Function, Label);"));

	status = gcFUNCTION_GetLabel(Function, Label);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_EmitOpcodeAndTarget(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcSL_OPCODE Opcode,
	IN gcsTARGET * Target
	)
{
    gceSTATUS	status;

    gcmHEADER();

	gcmASSERT(Target);

	if (Target->indexMode == gcSL_NOT_INDEXED)
	{
		status = _AddOpcode(
							Compiler,
							Opcode,
							Target->tempRegIndex,
							Target->enable);
	}
	else
	{
		status = _AddOpcodeIndexed(
								Compiler,
								Opcode,
								Target->tempRegIndex,
								Target->enable,
								Target->indexMode,
								Target->indexRegIndex);
	}

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the opcode"));

        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitOpcodeConditional(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcSL_OPCODE Opcode,
	IN gcSL_CONDITION Condition,
	IN gctLABEL Label
	)
{
    gceSTATUS	status;

    gcmHEADER();

	status = _AddOpcodeConditional(
								Compiler,
								Opcode,
								Condition,
								Label);

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the conditional opcode"));

        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitSourceTemp(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctBOOL IsSamplerIndex,
	IN gcsSOURCE_REG * SourceReg
	)
{
	gceSTATUS	status;

    gcmHEADER();

	gcmASSERT(SourceReg);

	if (IsSamplerIndex)
	{
		status = _AddSourceSamplerIndexed(
										Compiler,
										gcSL_SWIZZLE_XYZW,
										gcSL_INDEXED_X,
										SourceReg->regIndex);
	}
	else if (SourceReg->indexMode == gcSL_NOT_INDEXED)
	{
		status = _AddSource(
							Compiler,
							gcSL_TEMP,
							SourceReg->regIndex,
							SourceReg->swizzle);
	}
	else
	{
		status = _AddSourceIndexed(
								Compiler,
								gcSL_TEMP,
								SourceReg->regIndex,
								SourceReg->swizzle,
								SourceReg->indexMode,
								SourceReg->indexRegIndex);
	}

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the source"));

        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitSourceAttribute(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsSOURCE_REG * SourceReg
	)
{
	gceSTATUS	status;

    gcmHEADER();

	gcmASSERT(SourceReg);

	if (SourceReg->indexMode == gcSL_NOT_INDEXED)
	{
		status = _AddSourceAttribute(
									Compiler,
									SourceReg->u.attribute,
									SourceReg->swizzle,
									SourceReg->regIndex);

	}
	else
	{
		status = _AddSourceAttributeIndexed(
										Compiler,
										SourceReg->u.attribute,
										SourceReg->swizzle,
										SourceReg->regIndex,
										SourceReg->indexMode,
										SourceReg->indexRegIndex);
	}

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the source attribute"));

        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitSourceUniform(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsSOURCE_REG * SourceReg
	)
{
	gceSTATUS	status;

    gcmHEADER();

	gcmASSERT(SourceReg);

	if (SourceReg->indexMode == gcSL_NOT_INDEXED)
	{
		status = _AddSourceUniform(
									Compiler,
									SourceReg->u.uniform,
									SourceReg->swizzle,
									SourceReg->regIndex);

	}
	else
	{
		status = _AddSourceUniformIndexed(
										Compiler,
										SourceReg->u.uniform,
										SourceReg->swizzle,
										SourceReg->regIndex,
										SourceReg->indexMode,
										SourceReg->indexRegIndex);
	}

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the source uniform"));

        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitSourceConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctFLOAT Constant
	)
{
	gceSTATUS status;

    gcmHEADER();

	status = _AddSourceConstant(Compiler, Constant);

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the source constant"));

        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitSource(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsSOURCE * Source
	)
{
	gctFLOAT	floatConstant;
    gceSTATUS   status;

    gcmHEADER();

	gcmASSERT(Source);

	switch (Source->type)
	{
        case gcvSOURCE_TEMP:
            status = _EmitSourceTemp(Compiler,
                                     LineNo,
                                     StringNo,
                                     gcIsSamplerDataType(Source->dataType),
                                     &Source->u.sourceReg);
            break;

        case gcvSOURCE_ATTRIBUTE:
            status = _EmitSourceAttribute(Compiler,
                                          LineNo,
                                          StringNo,
                                          &Source->u.sourceReg);
            break;

        case gcvSOURCE_UNIFORM:
            status = _EmitSourceUniform(Compiler,
                                        LineNo,
                                        StringNo,
                                        &Source->u.sourceReg);
            break;

        case gcvSOURCE_CONSTANT:
            switch (Source->dataType)
		{
            case gcSHADER_FLOAT_X1:
            case gcSHADER_FLOAT_X2:
            case gcSHADER_FLOAT_X3:
            case gcSHADER_FLOAT_X4:
                floatConstant = Source->u.sourceConstant.u.floatConstant;
                break;

            case gcSHADER_INTEGER_X1:
            case gcSHADER_INTEGER_X2:
            case gcSHADER_INTEGER_X3:
            case gcSHADER_INTEGER_X4:
                floatConstant = slmI2F(Source->u.sourceConstant.u.intConstant);
                break;

            case gcSHADER_BOOLEAN_X1:
            case gcSHADER_BOOLEAN_X2:
            case gcSHADER_BOOLEAN_X3:
            case gcSHADER_BOOLEAN_X4:
                floatConstant = slmB2F(Source->u.sourceConstant.u.boolConstant);
                break;

            default:
                gcmASSERT(0);
                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
        }

            status = _EmitSourceConstant(
                                         Compiler,
                                         LineNo,
                                         StringNo,
                                         floatConstant);
            break;

        default:
            gcmASSERT(0);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
	}

    gcmFOOTER();
    return status;
}

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
	)
{
	gceSTATUS status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"<ATTRIBUTE line=\"%d\" string=\"%d\" name=\"%s\""
								" dataType=\"%s\" length=\"%d\">",
								LineNo,
								StringNo,
								Name,
								gcGetDataTypeName(DataType),
								Length));

	status = _AddAttribute(
						Compiler,
						Name,
						DataType,
						Length,
						IsTexture,
						Attribute);

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the attribute"));

        gcmFOOTER();
        return status;
    }

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"</ATTRIBUTE>"));

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

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
	)
{
	gceSTATUS status;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"<UNIFORM line=\"%d\" string=\"%d\" name=\"%s\""
								" dataType=\"%s\" length=\"%d\">",
								LineNo,
								StringNo,
								Name,
								gcGetDataTypeName(DataType),
								Length));

	status = _AddUniform(
						Compiler,
						Name,
						DataType,
                        precision,
						Length,
						Uniform);

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the uniform"));

        gcmFOOTER();
        return status;
    }

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"</UNIFORM>"));

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slGetUniformSamplerIndex(
	IN sloCOMPILER Compiler,
	IN gcUNIFORM UniformSampler,
	OUT gctREG_INDEX * Index
	)
{
	gceSTATUS	status;
	gctUINT32	sampler;

    gcmHEADER();

	status = _GetSampler(Compiler, UniformSampler, &sampler);

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										0,
										0,
										slvREPORT_INTERNAL_ERROR,
										"failed to get the uniform index"));

        gcmFOOTER();
        return status;
    }

	*Index = (gctREG_INDEX)sampler;

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slNewOutput(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctCONST_STRING Name,
	IN gcSHADER_TYPE DataType,
	IN gctSIZE_T Length,
	IN gctREG_INDEX TempRegIndex
	)
{
	gceSTATUS	status;
	gctSIZE_T	i;

    gcmHEADER();

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"<OUTPUT line=\"%d\" string=\"%d\" name=\"%s\""
								" dataType=\"%s\" length=\"%d\" tempRegIndex=\"%d\">",
								LineNo,
								StringNo,
								Name,
								gcGetDataTypeName(DataType),
								Length,
								TempRegIndex));

	status = _AddOutput(
						Compiler,
						Name,
						DataType,
						Length,
						TempRegIndex);

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the output"));

        gcmFOOTER();
        return status;
    }

	for (i = 1; i < Length; i++)
	{
		status = _AddOutputIndexed(
								Compiler,
								Name,
								i,
								TempRegIndex + (gctREG_INDEX)i);

		if (gcmIS_ERROR(status))
		{
			gcmVERIFY_OK(sloCOMPILER_Report(
											Compiler,
											LineNo,
											StringNo,
											slvREPORT_INTERNAL_ERROR,
											"failed to add the indexed output"));

            gcmFOOTER();
			return status;
		}
	}

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"</OUTPUT>"));

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

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
	)
{
	gceSTATUS status;

    gcmHEADER();

	gcmVERIFY_OK(
		sloCOMPILER_Dump(Compiler,
						 slvDUMP_CODE_EMITTER,
						 "<VARIABLE line=\"%d\" string=\"%d\" name=\"%s\" "
						 "dataType=\"%s\" length=\"%d\" tempRegIndex=\"%d\">",
						 LineNo,
						 StringNo,
						 Name,
						 gcGetDataTypeName(DataType),
						 Length,
						 TempRegIndex));

	status = _AddVariable(Compiler,
						  Name,
						  DataType,
						  Length,
						  TempRegIndex,
                          varCategory,
                          numStructureElement,
                          parent,
                          prevSibling,
                          ThisVarIndex);

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(
			sloCOMPILER_Report(Compiler,
							   LineNo,
							   StringNo,
							   slvREPORT_INTERNAL_ERROR,
							   "failed to add the variable"));

        gcmFOOTER();
        return status;
    }

	gcmVERIFY_OK(
		sloCOMPILER_Dump(Compiler,
						 slvDUMP_CODE_EMITTER,
						 "</VARIABLE>"));

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slUpdateVariableTempReg(
	IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctUINT varIndex,
	IN gctREG_INDEX newTempRegIndex
	)
{
    gceSTATUS status;

    gcmHEADER();

    status = _UpdateVariableTempReg(Compiler, varIndex, newTempRegIndex);

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(
			sloCOMPILER_Report(Compiler,
							   LineNo,
							   StringNo,
							   slvREPORT_INTERNAL_ERROR,
							   "failed to update the variable"));

        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slSetLabel(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctLABEL Label
	)
{
	gceSTATUS			status;
	sloCODE_EMITTER		codeEmitter;

    gcmHEADER();

	codeEmitter = sloCOMPILER_GetCodeEmitter(Compiler);
	gcmASSERT(codeEmitter);

	status = sloCODE_EMITTER_NewBasicBlock(Compiler, codeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	if (LineNo != 0)
	{
		gcmVERIFY_OK(sloCOMPILER_Dump(
									Compiler,
									slvDUMP_CODE_EMITTER,
									"<LABEL line=\"%d\" string=\"%d\" no=\"%d\">",
									LineNo,
									StringNo,
									Label));
	}
	else
	{
		gcmVERIFY_OK(sloCOMPILER_Dump(
									Compiler,
									slvDUMP_CODE_EMITTER,
									"<LABEL no=\"%d\">",
									Label));
	}

	status = _AddLabel(Compiler, Label);

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"</LABEL>"));

    if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the label"));

        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gctUINT8
gcGetDefaultEnable(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
    case gcSHADER_FLOAT_X1:
	case gcSHADER_BOOLEAN_X1:
	case gcSHADER_INTEGER_X1:		return gcSL_ENABLE_X;

	case gcSHADER_FLOAT_X2:
	case gcSHADER_FLOAT_2X2:
	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_INTEGER_X2:		return gcSL_ENABLE_XY;

	case gcSHADER_FLOAT_X3:
	case gcSHADER_FLOAT_3X3:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_INTEGER_X3:		return gcSL_ENABLE_XYZ;

	case gcSHADER_FLOAT_X4:
	case gcSHADER_FLOAT_4X4:
	case gcSHADER_BOOLEAN_X4:
	case gcSHADER_INTEGER_X4:		return gcSL_ENABLE_XYZW;

	case gcSHADER_SAMPLER_1D:
	case gcSHADER_SAMPLER_2D:
	case gcSHADER_SAMPLER_3D:
	case gcSHADER_SAMPLER_CUBIC:
	case gcSHADER_SAMPLER_EXTERNAL_OES:	return gcSL_ENABLE_X;

    default:
		gcmASSERT(0);
		return gcSL_ENABLE_XYZW;
    }
}

gctUINT8
gcGetVectorComponentEnable(
	IN gctUINT8 Enable,
	IN gctUINT8 Component
	)
{
    gctUINT8		i;
    gctUINT8        enables[4] = {0};

	for (i = 0; i < 4; i++)
	{
		if (Enable & gcSL_ENABLE_X)
		{
			enables[i]	= gcSL_ENABLE_X;
			Enable		&= ~gcSL_ENABLE_X;
		}
		else if (Enable & gcSL_ENABLE_Y)
		{
			enables[i]	= gcSL_ENABLE_Y;
			Enable		&= ~gcSL_ENABLE_Y;
		}
		else if (Enable & gcSL_ENABLE_Z)
		{
			enables[i]	= gcSL_ENABLE_Z;
			Enable		&= ~gcSL_ENABLE_Z;
		}
		else if (Enable & gcSL_ENABLE_W)
		{
			enables[i]	= gcSL_ENABLE_W;
			Enable		&= ~gcSL_ENABLE_W;
		}
		else
		{
			break;
		}
	}

	gcmASSERT(Component < i);

    if(Component < i)
		return enables[Component];
	else
		return 0;
}

gctUINT8
gcGetDefaultSwizzle(
	IN gcSHADER_TYPE DataType
	)
{
    switch (DataType)
	{
    case gcSHADER_FLOAT_X1:
	case gcSHADER_BOOLEAN_X1:
	case gcSHADER_INTEGER_X1:		return gcSL_SWIZZLE_XXXX;

	case gcSHADER_FLOAT_X2:
	case gcSHADER_BOOLEAN_X2:
	case gcSHADER_INTEGER_X2:		return gcSL_SWIZZLE_XYYY;

	case gcSHADER_FLOAT_X3:
	case gcSHADER_BOOLEAN_X3:
	case gcSHADER_INTEGER_X3:		return gcSL_SWIZZLE_XYZZ;

	case gcSHADER_FLOAT_X4:
	case gcSHADER_BOOLEAN_X4:
	case gcSHADER_INTEGER_X4:
	case gcSHADER_SAMPLER_1D:
	case gcSHADER_SAMPLER_2D:
	case gcSHADER_SAMPLER_3D:
	case gcSHADER_SAMPLER_CUBIC:
	case gcSHADER_SAMPLER_EXTERNAL_OES:
						return gcSL_SWIZZLE_XYZW;

    default:
		gcmASSERT(0);
		return gcSL_SWIZZLE_XYZW;
    }
}

gctUINT8
gcGetVectorComponentSwizzle(
	IN gctUINT8 Swizzle,
	IN gctUINT8 Component
	)
{
	gctUINT8 value = 0;

	switch (Component)
	{
	case 0:
		value = (Swizzle >> 0) & 3;
		break;

	case 1:
		value = (Swizzle >> 2) & 3;
		break;

	case 2:
		value = (Swizzle >> 4) & 3;
		break;

	case 3:
		value = (Swizzle >> 6) & 3;
		break;

	default:
		gcmASSERT(0);
	}

	return value | (value << 2) | (value << 4) | (value << 6);
}

static gceSTATUS
_EmitCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcSL_OPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	);

static gceSTATUS
_MakeNewSource(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsSOURCE * Source,
	OUT gcsSOURCE * NewSource
	)
{
	gceSTATUS	status;
	gcsTARGET	tempTarget;

    gcmHEADER();

	gcsTARGET_Initialize(
						&tempTarget,
						Source->dataType,
						slNewTempRegs(Compiler, 1),
						gcGetDefaultEnable(Source->dataType),
						gcSL_NOT_INDEXED,
						0);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_MOV,
					&tempTarget,
					Source,
					gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	gcsSOURCE_InitializeTempReg(
								NewSource,
								Source->dataType,
								tempTarget.tempRegIndex,
								gcGetDefaultSwizzle(Source->dataType),
								gcSL_NOT_INDEXED,
								0);

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_PrepareSource(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source,
	OUT gcsSOURCE * NewSource
	)
{
	gceSTATUS	status;
	gctBOOL		insertAssign;

    gcmHEADER();

    gcmASSERT(Source);
    gcmASSERT(NewSource);

	if (Target != gcvNULL)
	{
		insertAssign = (Source->type == gcvSOURCE_TEMP
						&& Target->tempRegIndex == Source->u.sourceReg.regIndex);
	}
	else
	{
		insertAssign = (Source->type == gcvSOURCE_UNIFORM);
	}

    if (insertAssign)
	{
		status = _MakeNewSource(
								Compiler,
								LineNo,
								StringNo,
								Source,
								NewSource);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
	{
        *NewSource = *Source;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_PrepareAnotherSource(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1,
	OUT gcsSOURCE * NewSource1
	)
{
	gceSTATUS	status;
	gctBOOL		insertAssign;

    gcmHEADER();

    gcmASSERT(Source0);
	gcmASSERT(Source1);
    gcmASSERT(NewSource1);

	insertAssign =
		(Source1->type == gcvSOURCE_CONSTANT && Source0->type == gcvSOURCE_UNIFORM)
		|| (Source1->type == gcvSOURCE_UNIFORM && Source0->type == gcvSOURCE_CONSTANT)
		|| (Source1->type == gcvSOURCE_UNIFORM && Source0->type == gcvSOURCE_UNIFORM
			&& (Source1->u.sourceReg.u.uniform != Source0->u.sourceReg.u.uniform
				|| Source1->u.sourceReg.regIndex != Source0->u.sourceReg.regIndex));

	if (Target != gcvNULL)
	{
		insertAssign =
			(insertAssign
			|| (Source1->type == gcvSOURCE_TEMP
				&& Target->tempRegIndex == Source1->u.sourceReg.regIndex));
	}

    if (insertAssign)
	{
		status = _MakeNewSource(
								Compiler,
								LineNo,
								StringNo,
								Source1,
								NewSource1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
	{
        *NewSource1 = *Source1;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gcSL_OPCODE
_ConvOpcode(
	sleOPCODE opcode
	)
{
	switch (opcode)
	{
    case slvOPCODE_ASSIGN:				return gcSL_MOV;

	case slvOPCODE_ADD:					return gcSL_ADD;
	case slvOPCODE_SUB:					return gcSL_SUB;
	case slvOPCODE_MUL:					return gcSL_MUL;
	case slvOPCODE_DIV:					gcmASSERT(0); return gcSL_NOP;

	case slvOPCODE_TEXTURE_LOAD:		return gcSL_TEXLD;
	case slvOPCODE_TEXTURE_LOAD_PROJ:	return gcSL_TEXLDP;
	case slvOPCODE_TEXTURE_BIAS:		return gcSL_TEXBIAS;
	case slvOPCODE_TEXTURE_LOD:			return gcSL_TEXLOD;

	case slvOPCODE_FLOAT_TO_INT:		gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_FLOAT_TO_BOOL:		gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_INT_TO_BOOL:			gcmASSERT(0); return gcSL_NOP;

	case slvOPCODE_INVERSE:				return gcSL_RCP;

	case slvOPCODE_LESS_THAN:			gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_LESS_THAN_EQUAL:		gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_GREATER_THAN:		gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_GREATER_THAN_EQUAL:	gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_EQUAL:				gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_NOT_EQUAL:			gcmASSERT(0); return gcSL_NOP;

	case slvOPCODE_ANY:					gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_ALL:					gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_NOT:					gcmASSERT(0); return gcSL_NOP;

	case slvOPCODE_SIN:					return gcSL_SIN;
	case slvOPCODE_COS:					return gcSL_COS;
	case slvOPCODE_TAN:					return gcSL_TAN;

	case slvOPCODE_ASIN:				return gcSL_ASIN;
	case slvOPCODE_ACOS:				return gcSL_ACOS;
	case slvOPCODE_ATAN:				return gcSL_ATAN;
	case slvOPCODE_ATAN2:				gcmASSERT(0); return gcSL_NOP;

	case slvOPCODE_POW:					return gcSL_POW;
	case slvOPCODE_EXP2:				return gcSL_EXP;
	case slvOPCODE_LOG2:				return gcSL_LOG;
	case slvOPCODE_SQRT:				return gcSL_SQRT;
	case slvOPCODE_INVERSE_SQRT:		return gcSL_RSQ;

	case slvOPCODE_ABS:					return gcSL_ABS;
	case slvOPCODE_SIGN:				return gcSL_SIGN;
	case slvOPCODE_FLOOR:				return gcSL_FLOOR;
	case slvOPCODE_CEIL:				return gcSL_CEIL;
	case slvOPCODE_FRACT:				return gcSL_FRAC;
	case slvOPCODE_MIN:					return gcSL_MIN;
	case slvOPCODE_MAX:					return gcSL_MAX;
	case slvOPCODE_SATURATE:			return gcSL_SAT;
	case slvOPCODE_STEP:				return gcSL_STEP;
	case slvOPCODE_DOT:					gcmASSERT(0); return gcSL_NOP;
	case slvOPCODE_CROSS:				return gcSL_CROSS;
	case slvOPCODE_NORMALIZE:			gcmASSERT(0); return gcSL_NOP;

	case slvOPCODE_JUMP:				return gcSL_JMP;
	case slvOPCODE_CALL:				return gcSL_CALL;
	case slvOPCODE_RETURN:				return gcSL_RET;
	case slvOPCODE_DISCARD:				return gcSL_KILL;

	case slvOPCODE_DFDX:				return gcSL_DSX;
	case slvOPCODE_DFDY:				return gcSL_DSY;
	case slvOPCODE_FWIDTH:				return gcSL_FWIDTH;

	default:
		gcmASSERT(0);
		return gcSL_NOP;
	}
}

static gceSTATUS
_EmitCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcSL_OPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS	status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(!gcIsMatrixDataType(Target->dataType));
	gcmASSERT(Source0);
	gcmASSERT(!gcIsMatrixDataType(Source0->dataType));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"<INSTRUCTION line=\"%d\" string=\"%d\" opcode=\"%s\""
								" targetDataType=\"%s\"",
								LineNo,
								StringNo,
								_GetOpcodeName(Opcode),
								gcGetDataTypeName(Target->dataType)));

	if (Source1 == gcvNULL)
	{
		gcmVERIFY_OK(sloCOMPILER_Dump(
									Compiler,
									slvDUMP_CODE_EMITTER,
									" sourceDataType=\"%s\">",
									gcGetDataTypeName(Source0->dataType)));
	}
	else
	{
		gcmASSERT(!gcIsMatrixDataType(Source1->dataType));

		gcmVERIFY_OK(sloCOMPILER_Dump(
									Compiler,
									slvDUMP_CODE_EMITTER,
									" source0DataType=\"%s\" source1DataType=\"%s\">",
									gcGetDataTypeName(Source0->dataType),
									gcGetDataTypeName(Source1->dataType)));
	}

	status = _EmitOpcodeAndTarget(
								Compiler,
								LineNo,
								StringNo,
								Opcode,
								Target);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	status = _EmitSource(
						Compiler,
						LineNo,
						StringNo,
						Source0);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	if (Source1 != gcvNULL)
	{
		status = _EmitSource(
							Compiler,
							LineNo,
							StringNo,
							Source1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"</INSTRUCTION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_EmitBranchCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcSL_OPCODE Opcode,
	IN gcSL_CONDITION Condition,
	IN gctLABEL Label,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS			status;
	sloCODE_EMITTER		codeEmitter;

    gcmHEADER();

	codeEmitter = sloCOMPILER_GetCodeEmitter(Compiler);
	gcmASSERT(codeEmitter);

	status = sloCODE_EMITTER_EndBasicBlock(Compiler, codeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"<INSTRUCTION line=\"%d\" string=\"%d\" opcode=\"%s\""
								" condition=\"%s\" label=\"%d\"",
								LineNo,
								StringNo,
								_GetOpcodeName(Opcode),
								_GetConditionName(Condition),
								Label));

	if (Source0 != gcvNULL)
	{
		gcmASSERT(!gcIsMatrixDataType(Source0->dataType));

		gcmVERIFY_OK(sloCOMPILER_Dump(
									Compiler,
									slvDUMP_CODE_EMITTER,
									" source0DataType=\"%s\"",
									gcGetDataTypeName(Source0->dataType)));
	}

	if (Source1 != gcvNULL)
	{
		gcmASSERT(!gcIsMatrixDataType(Source1->dataType));

		gcmVERIFY_OK(sloCOMPILER_Dump(
									Compiler,
									slvDUMP_CODE_EMITTER,
									" source1DataType=\"%s\"",
									gcGetDataTypeName(Source1->dataType)));
	}

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								">"));

	status = _EmitOpcodeConditional(
									Compiler,
									LineNo,
									StringNo,
									Opcode,
									Condition,
									Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	if (Source0 != gcvNULL)
	{
		status = _EmitSource(
							Compiler,
							LineNo,
							StringNo,
							Source0);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}

	if (Source1 != gcvNULL)
	{
		status = _EmitSource(
							Compiler,
							LineNo,
							StringNo,
							Source1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_CODE_EMITTER,
								"</INSTRUCTION>"));

	status = sloCODE_EMITTER_NewBasicBlock(Compiler, codeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slEmitAssignCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	return slEmitCode1(
					Compiler,
					LineNo,
					StringNo,
					slvOPCODE_ASSIGN,
					Target,
					Source);
}

static gceSTATUS
_EmitFloatToIntCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS		status;
	slsIOPERAND		intermIOperands[3];
	gcsTARGET		intermTargets[3];
	gcsSOURCE		intermSources[3];

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source);



	/* sign t0, source */
	slsIOPERAND_New(Compiler, &intermIOperands[0], Source->dataType);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[0], &intermIOperands[0]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_SIGN,
					&intermTargets[0],
					Source,
					gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* abs t1, source */
	slsIOPERAND_New(Compiler, &intermIOperands[1], Source->dataType);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[1], &intermIOperands[1]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_ABS,
					&intermTargets[1],
					Source,
					gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* floor t2, t1 */
	slsIOPERAND_New(Compiler, &intermIOperands[2], Source->dataType);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[2], &intermIOperands[2]);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[1], &intermIOperands[1]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_FLOOR,
					&intermTargets[2],
					&intermSources[1],
					gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, t0, t2 */
	gcsSOURCE_InitializeUsingIOperand(&intermSources[0], &intermIOperands[0]);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[2], &intermIOperands[2]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_MUL,
					Target,
					&intermSources[0],
					&intermSources[2]);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitScalarFloatOrIntToBoolCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS	status;
	gctLABEL	endLabel;
	gcsSOURCE	constSource;

    gcmHEADER();

	endLabel	= slNewLabel(Compiler);

	/* jump end if !(source) */
	status = slEmitTestBranchCode(
								Compiler,
								LineNo,
								StringNo,
								slvOPCODE_JUMP,
								endLabel,
								gcvFALSE,
								Source);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, true */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvTRUE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* end: */
	status = slSetLabel(Compiler, LineNo, StringNo, endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitFloatOrIntToBoolCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS	status;
	gctUINT		i;
	gcsTARGET	componentTarget;
	gcsSOURCE	componentSource;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source);

	/* mov target, source */
	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						Source,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	if (Target->dataType == gcSHADER_BOOLEAN_X1)
	{
		status = _EmitScalarFloatOrIntToBoolCode(
												Compiler,
												LineNo,
												StringNo,
												Target,
												Source);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}
	else
	{
		gcmASSERT(Target->dataType == gcSHADER_BOOLEAN_X2
					|| Target->dataType == gcSHADER_BOOLEAN_X3
					|| Target->dataType == gcSHADER_BOOLEAN_X4);

		for (i = 0; i < gcGetVectorDataTypeComponentCount(Target->dataType); i++)
		{
			gcsTARGET_InitializeAsVectorComponent(&componentTarget, Target, i);
			gcsSOURCE_InitializeAsVectorComponent(&componentSource, Source, i);

			status = _EmitScalarFloatOrIntToBoolCode(
													Compiler,
													LineNo,
													StringNo,
													&componentTarget,
													&componentSource);

			if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
		}
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitAnyCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS	status;
	gctLABEL	elseLabel, endLabel;
	gcsSOURCE	constSource;

    gcmHEADER();

	elseLabel	= slNewLabel(Compiler);
	endLabel	= slNewLabel(Compiler);

	/* jump else if all components are false */
	status = slEmitTestBranchCode(
								Compiler,
								LineNo,
								StringNo,
								slvOPCODE_JUMP,
								elseLabel,
								gcvFALSE,
								Source);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, true */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvTRUE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else: */
	status = slSetLabel(Compiler, LineNo, StringNo, elseLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, false */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvFALSE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* end: */
	status = slSetLabel(Compiler, LineNo, StringNo, endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitAllCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS	status;
	gctLABEL	elseLabel, endLabel;
	gcsSOURCE	constSource;

    gcmHEADER();

	elseLabel	= slNewLabel(Compiler);
	endLabel	= slNewLabel(Compiler);

	/* jump else if all components are true */
	status = slEmitTestBranchCode(
								Compiler,
								LineNo,
								StringNo,
								slvOPCODE_JUMP,
								elseLabel,
								gcvTRUE,
								Source);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, false */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvFALSE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else: */
	status = slSetLabel(Compiler, LineNo, StringNo, elseLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, true */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvTRUE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* end: */
	status = slSetLabel(Compiler, LineNo, StringNo, endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitScalarNotCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS	status;
	gctLABEL	elseLabel, endLabel;
	gcsSOURCE	constSource;

    gcmHEADER();

	elseLabel	= slNewLabel(Compiler);
	endLabel	= slNewLabel(Compiler);

	/* jump else if (source == true) */
	status = slEmitTestBranchCode(
								Compiler,
								LineNo,
								StringNo,
								slvOPCODE_JUMP,
								elseLabel,
								gcvTRUE,
								Source);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, true */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvTRUE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else: */
	status = slSetLabel(Compiler, LineNo, StringNo, elseLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, false */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvFALSE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* end: */
	status = slSetLabel(Compiler, LineNo, StringNo, endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitNotCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS	status;
	gctUINT		i;
	gcsTARGET	componentTarget;
	gcsSOURCE	componentSource;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source);

	if (Target->dataType == gcSHADER_BOOLEAN_X1)
	{
		status = _EmitScalarNotCode(
									Compiler,
									LineNo,
									StringNo,
									Target,
									Source);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}
	else
	{
		gcmASSERT(Target->dataType == gcSHADER_BOOLEAN_X2
					|| Target->dataType == gcSHADER_BOOLEAN_X3
					|| Target->dataType == gcSHADER_BOOLEAN_X4);

		for (i = 0; i < gcGetVectorDataTypeComponentCount(Target->dataType); i++)
		{
			gcsTARGET_InitializeAsVectorComponent(&componentTarget, Target, i);
			gcsSOURCE_InitializeAsVectorComponent(&componentSource, Source, i);

			status = _EmitScalarNotCode(
										Compiler,
										LineNo,
										StringNo,
										&componentTarget,
										&componentSource);

			if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
		}
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitDP2Code(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	);

static gceSTATUS
_EmitNORM2Code(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS		status;
	slsIOPERAND		intermIOperands[2];
	gcsTARGET		intermTargets[2];
	gcsSOURCE		intermSources[2];

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source);

	/* dp2 t0, source, source */
	slsIOPERAND_New(Compiler, &intermIOperands[0], gcSHADER_FLOAT_X1);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[0], &intermIOperands[0]);

	status = _EmitDP2Code(
					Compiler,
					LineNo,
					StringNo,
					&intermTargets[0],
					Source,
					Source);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* rsq t1, t0 */
	slsIOPERAND_New(Compiler, &intermIOperands[1], gcSHADER_FLOAT_X1);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[1], &intermIOperands[1]);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[0], &intermIOperands[0]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_RSQ,
					&intermTargets[1],
					&intermSources[0],
					gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, source, t1 */
	gcsSOURCE_InitializeUsingIOperand(&intermSources[1], &intermIOperands[1]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_MUL,
					Target,
					Source,
					&intermSources[1]);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitNORM4Code(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS		status;
	slsIOPERAND		intermIOperands[2];
	gcsTARGET		intermTargets[2];
	gcsSOURCE		intermSources[2];

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source);

    if (Source == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

	/* dp4 t0, source, source */
	slsIOPERAND_New(Compiler, &intermIOperands[0], gcSHADER_FLOAT_X1);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[0], &intermIOperands[0]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_DP4,
					&intermTargets[0],
					Source,
					Source);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* rsq t1, t0 */
	slsIOPERAND_New(Compiler, &intermIOperands[1], gcSHADER_FLOAT_X1);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[1], &intermIOperands[1]);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[0], &intermIOperands[0]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_RSQ,
					&intermTargets[1],
					&intermSources[0],
					gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, source, t1 */
	gcsSOURCE_InitializeUsingIOperand(&intermSources[1], &intermIOperands[1]);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_MUL,
					Target,
					Source,
					&intermSources[1]);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitNormalizeCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gcsSOURCE	sourceOne;
    gceSTATUS   status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source);
    if (Source == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

	switch (Source->dataType)
	{
	case gcSHADER_FLOAT_X1:
		gcsSOURCE_InitializeFloatConstant(&sourceOne, (gctFLOAT)1.0);

		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&sourceOne,
						gcvNULL);
        break;

	case gcSHADER_FLOAT_X2:
		status = _EmitNORM2Code(
							Compiler,
							LineNo,
							StringNo,
							Target,
							Source);
        break;

	case gcSHADER_FLOAT_X3:
		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_NORM,
						Target,
						Source,
						gcvNULL);
        break;

	case gcSHADER_FLOAT_X4:
		status = _EmitNORM4Code(
							Compiler,
							LineNo,
							StringNo,
							Target,
							Source);
        break;

	default:
        status = gcvSTATUS_OK;
		gcmASSERT(0);
	}

    gcmFOOTER();
	return status;
}

typedef gceSTATUS
(* sltEMIT_SPECIAL_CODE_FUNC_PTR1)(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	);

typedef struct _slsSPECIAL_CODE_EMITTER1
{
	sleOPCODE						opcode;

	sltEMIT_SPECIAL_CODE_FUNC_PTR1	codeEmitter;
}
slsSPECIAL_CODE_EMITTER1;

static slsSPECIAL_CODE_EMITTER1 SpecialCodeEmitterTable1[] =
{
	{slvOPCODE_FLOAT_TO_INT,	_EmitFloatToIntCode},
	{slvOPCODE_FLOAT_TO_BOOL,	_EmitFloatOrIntToBoolCode},
	{slvOPCODE_INT_TO_BOOL,		_EmitFloatOrIntToBoolCode},

	{slvOPCODE_ANY,				_EmitAnyCode},
	{slvOPCODE_ALL,				_EmitAllCode},
	{slvOPCODE_NOT,				_EmitNotCode},

	{slvOPCODE_NORMALIZE,		_EmitNormalizeCode}
};

const gctUINT SpecialCodeEmitterCount1 =
						sizeof(SpecialCodeEmitterTable1) / sizeof(slsSPECIAL_CODE_EMITTER1);

static gceSTATUS
_EmitCodeImpl1(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS						status;
	gcsSOURCE						newSource;
	gctUINT							i;
	sltEMIT_SPECIAL_CODE_FUNC_PTR1	specialCodeEmitter = gcvNULL;

    gcmHEADER();

	status = _PrepareSource(
							Compiler,
							LineNo,
							StringNo,
							Target,
							Source,
							&newSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	for (i = 0; i < SpecialCodeEmitterCount1; i++)
	{
		if (SpecialCodeEmitterTable1[i].opcode == Opcode)
		{
			specialCodeEmitter = SpecialCodeEmitterTable1[i].codeEmitter;
			break;
		}
	}

	if (specialCodeEmitter != gcvNULL)
	{
		status = (*specialCodeEmitter)(
									Compiler,
									LineNo,
									StringNo,
									Target,
									&newSource);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}
	else
	{
		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						_ConvOpcode(Opcode),
						Target,
						&newSource,
						gcvNULL);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slEmitCode1(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	sloCODE_EMITTER		codeEmitter;
    gceSTATUS           status;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

	codeEmitter = sloCOMPILER_GetCodeEmitter(Compiler);
	gcmASSERT(codeEmitter);

	status = sloCODE_EMITTER_EmitCode1(
									Compiler,
									codeEmitter,
									LineNo,
									StringNo,
									Opcode,
									Target,
									Source);
    gcmFOOTER();
    return status;
}

static void
gcsSOURCE_CONSTANT_Inverse(
	IN OUT gcsSOURCE * Source
	)
{
	gcmASSERT(Source);
	gcmASSERT(Source->type == gcvSOURCE_CONSTANT);

	switch (gcGetComponentDataType(Source->dataType))
	{
	case gcSHADER_FLOAT_X1:
		Source->u.sourceConstant.u.floatConstant =
				(gctFLOAT)1.0 / Source->u.sourceConstant.u.floatConstant;
		break;

	case gcSHADER_INTEGER_X1:
		if (Source->dataType == gcSHADER_INTEGER_X1)
		{
			Source->dataType = gcSHADER_FLOAT_X1;
		}
		else
		{
			Source->dataType = gcConvScalarToVectorDataType(
										gcSHADER_FLOAT_X1,
										gcGetDataTypeComponentCount(Source->dataType));
		}

		Source->u.sourceConstant.u.floatConstant =
				(gctFLOAT)1.0 / (gctFLOAT)Source->u.sourceConstant.u.intConstant;
		break;

	default:
		gcmASSERT(0);
	}
}

static gceSTATUS
_EmitMulForDivCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS		status;
	slsIOPERAND		intermIOperand;
	gcsTARGET		intermTarget;
	gcsSOURCE		intermSource;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	switch (gcGetComponentDataType(Target->dataType))
	{
	case gcSHADER_FLOAT_X1:
		/* mul target, source0, source1 */
		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						Target,
						Source0,
						Source1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		break;

	case gcSHADER_INTEGER_X1:
		/* mul t0, source0, source1 */
		slsIOPERAND_New(Compiler, &intermIOperand, Target->dataType);
		gcsTARGET_InitializeUsingIOperand(&intermTarget, &intermIOperand);

		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						&intermTarget,
						Source0,
						Source1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		/* floor target, t0 */
		gcsSOURCE_InitializeUsingIOperand(&intermSource, &intermIOperand);

        _EmitFloatToIntCode(Compiler, LineNo, StringNo, Target, &intermSource);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		break;

	default:
		gcmASSERT(0);
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitDivCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS		status;
	slsIOPERAND		intermIOperand;
	gcsTARGET		intermTarget;
	gcsSOURCE		intermSource;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	if (Source1->type == gcvSOURCE_CONSTANT)
	{
		/* mul target, source0, 1 / constant_source1 */
		intermSource = *Source1;
		gcsSOURCE_CONSTANT_Inverse(&intermSource);

		status = _EmitMulForDivCode(
									Compiler,
									LineNo,
									StringNo,
									Target,
									Source0,
									&intermSource);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}
	else
	{
		/* rcp t0, source1 */
		slsIOPERAND_New(Compiler, &intermIOperand, Source1->dataType);
		gcsTARGET_InitializeUsingIOperand(&intermTarget, &intermIOperand);

		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_RCP,
						&intermTarget,
						Source1,
						gcvNULL);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		/* mul target, source0, t0 */
		gcsSOURCE_InitializeUsingIOperand(&intermSource, &intermIOperand);

		status = _EmitMulForDivCode(
									Compiler,
									LineNo,
									StringNo,
									Target,
									Source0,
									&intermSource);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitScalarAtan2Code(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS		status;
	gctLABEL		else0Label, else1Label, else2Label, else3Label, endLabel, jmp1Label;
	gcsSOURCE		constSource;
	slsIOPERAND		intermIOperands[5];
	gcsTARGET		intermTargets[5];
	gcsSOURCE		intermSources[5];

    gcmHEADER();

	else0Label	= slNewLabel(Compiler);
	else1Label	= slNewLabel(Compiler);
	else2Label	= slNewLabel(Compiler);
	else3Label	= slNewLabel(Compiler);
	endLabel	= slNewLabel(Compiler);
	jmp1Label	= slNewLabel(Compiler);

	/* sign t0, y (source0) */
	slsIOPERAND_New(Compiler, &intermIOperands[0], Source0->dataType);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[0], &intermIOperands[0]);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_SIGN,
						&intermTargets[0],
						Source0,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump else0 if x (source1) != 0.0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									slvCONDITION_NOT_EQUAL,
									else0Label,
									Source1,
									&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, t0, _HALF_PI */
	gcsSOURCE_InitializeUsingIOperand(&intermSources[0], &intermIOperands[0]);
	gcsSOURCE_InitializeFloatConstant(&constSource, _HALF_PI);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						Target,
						&intermSources[0],
						&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else0: */
	status = slSetLabel(Compiler, LineNo, StringNo, else0Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* div t1, y (source0), x (source1) */
	slsIOPERAND_New(Compiler, &intermIOperands[1], Source0->dataType);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[1], &intermIOperands[1]);

	status = _EmitDivCode(
						Compiler,
						LineNo,
						StringNo,
						&intermTargets[1],
						Source0,
						Source1);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* abs t2, t1 */
	slsIOPERAND_New(Compiler, &intermIOperands[2], Source0->dataType);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[2], &intermIOperands[2]);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[1], &intermIOperands[1]);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_ABS,
						&intermTargets[2],
						&intermSources[1],
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* atan t3, t2 */
	slsIOPERAND_New(Compiler, &intermIOperands[3], Source0->dataType);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[3], &intermIOperands[3]);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[2], &intermIOperands[2]);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_ATAN,
						&intermTargets[3],
						&intermSources[2],
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump else1 if x (source1) > 0.0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									slvCONDITION_GREATER_THAN,
									else1Label,
									Source1,
									&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump else2 if y (source0) != 0.0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									slvCONDITION_NOT_EQUAL,
									else2Label,
									Source0,
									&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, _PI */
	gcsSOURCE_InitializeFloatConstant(&constSource, _PI);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump jmp1 */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									jmp1Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else2: */
	status = slSetLabel(Compiler, LineNo, StringNo, else2Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* sub t4, _PI, t3 */
	slsIOPERAND_New(Compiler, &intermIOperands[4], Source0->dataType);
	gcsTARGET_InitializeUsingIOperand(&intermTargets[4], &intermIOperands[4]);
	gcsSOURCE_InitializeFloatConstant(&constSource, _PI);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[3], &intermIOperands[3]);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_SUB,
						&intermTargets[4],
						&constSource,
						&intermSources[3]);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, t0, t4 */
	gcsSOURCE_InitializeUsingIOperand(&intermSources[0], &intermIOperands[0]);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[4], &intermIOperands[4]);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						Target,
						&intermSources[0],
						&intermSources[4]);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jmp1: */
	status = slSetLabel(Compiler, LineNo, StringNo, jmp1Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else1: */
	status = slSetLabel(Compiler, LineNo, StringNo, else1Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump else3 if y (source0) != 0.0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									slvCONDITION_NOT_EQUAL,
									else3Label,
									Source0,
									&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, 0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else3: */
	status = slSetLabel(Compiler, LineNo, StringNo, else3Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, t0, t3 */
	gcsSOURCE_InitializeUsingIOperand(&intermSources[0], &intermIOperands[0]);
	gcsSOURCE_InitializeUsingIOperand(&intermSources[3], &intermIOperands[3]);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						Target,
						&intermSources[0],
						&intermSources[3]);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* end: */
	status = slSetLabel(Compiler, LineNo, StringNo, endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitVectorComponentAtan2SelectionCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1,
	IN gcsSOURCE IntermSources[5]
	)
{
	gceSTATUS		status;
	gctLABEL		else0Label, else1Label, else2Label, else3Label, endLabel;
	gcsSOURCE		constSource;

    gcmHEADER();

	else0Label	= slNewLabel(Compiler);
	else1Label	= slNewLabel(Compiler);
	else2Label	= slNewLabel(Compiler);
	else3Label	= slNewLabel(Compiler);
	endLabel	= slNewLabel(Compiler);

	/* jump else0 if x (source1) != 0.0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									slvCONDITION_NOT_EQUAL,
									else0Label,
									Source1,
									&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, t0, _HALF_PI */
	gcsSOURCE_InitializeFloatConstant(&constSource, _HALF_PI);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						Target,
						&IntermSources[0],
						&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else0: */
	status = slSetLabel(Compiler, LineNo, StringNo, else0Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump else1 if x (source1) > 0.0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									slvCONDITION_GREATER_THAN,
									else1Label,
									Source1,
									&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump else2 if y (source0) != 0.0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									slvCONDITION_NOT_EQUAL,
									else2Label,
									Source0,
									&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, _PI */
	gcsSOURCE_InitializeFloatConstant(&constSource, _PI);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else2: */
	status = slSetLabel(Compiler, LineNo, StringNo, else2Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, t0, t4 */
	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						Target,
						&IntermSources[0],
						&IntermSources[4]);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else1: */
	status = slSetLabel(Compiler, LineNo, StringNo, else1Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump else3 if y (source0) != 0.0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									slvCONDITION_NOT_EQUAL,
									else3Label,
									Source0,
									&constSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, 0 */
	gcsSOURCE_InitializeFloatConstant(&constSource, (gctFLOAT)0.0);

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else3: */
	status = slSetLabel(Compiler, LineNo, StringNo, else3Label);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mul target, t0, t3 */
	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						Target,
						&IntermSources[0],
						&IntermSources[3]);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* end: */
	status = slSetLabel(Compiler, LineNo, StringNo, endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitAtan2Code(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS		status;
	gctUINT			i, j;
	gcsTARGET		componentTarget;
	gcsSOURCE		componentSource0, componentSource1;
	gcsSOURCE		constSource;
	slsIOPERAND		intermIOperands[5];
	gcsTARGET		intermTargets[5];
	gcsSOURCE		intermSources[5];
	gcsSOURCE		componentIntermSources[5];

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	if (Target->dataType == gcSHADER_FLOAT_X1)
	{
		status = _EmitScalarAtan2Code(
									Compiler,
									LineNo,
									StringNo,
									Target,
									Source0,
									Source1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}
	else
	{
		gcmASSERT(Target->dataType == gcSHADER_FLOAT_X2
					|| Target->dataType == gcSHADER_FLOAT_X3
					|| Target->dataType == gcSHADER_FLOAT_X4);

		/* sign t0, y (source0) */
		slsIOPERAND_New(Compiler, &intermIOperands[0], Source0->dataType);
		gcsTARGET_InitializeUsingIOperand(&intermTargets[0], &intermIOperands[0]);

		status = _EmitCode(
							Compiler,
							LineNo,
							StringNo,
							gcSL_SIGN,
							&intermTargets[0],
							Source0,
							gcvNULL);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }


		/* div t1, y (source0), x (source1) */
		slsIOPERAND_New(Compiler, &intermIOperands[1], Source0->dataType);
		gcsTARGET_InitializeUsingIOperand(&intermTargets[1], &intermIOperands[1]);
        gcsSOURCE_InitializeUsingIOperand(&intermSources[0], &intermIOperands[0]);

		status = _EmitDivCode(
							Compiler,
							LineNo,
							StringNo,
							&intermTargets[1],
							Source0,
							Source1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		/* abs t2, t1 */
		slsIOPERAND_New(Compiler, &intermIOperands[2], Source0->dataType);
		gcsTARGET_InitializeUsingIOperand(&intermTargets[2], &intermIOperands[2]);
		gcsSOURCE_InitializeUsingIOperand(&intermSources[1], &intermIOperands[1]);

		status = _EmitCode(
							Compiler,
							LineNo,
							StringNo,
							gcSL_ABS,
							&intermTargets[2],
							&intermSources[1],
							gcvNULL);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		/* atan t3, t2 */
		slsIOPERAND_New(Compiler, &intermIOperands[3], Source0->dataType);
		gcsTARGET_InitializeUsingIOperand(&intermTargets[3], &intermIOperands[3]);
		gcsSOURCE_InitializeUsingIOperand(&intermSources[2], &intermIOperands[2]);

		status = _EmitCode(
							Compiler,
							LineNo,
							StringNo,
							gcSL_ATAN,
							&intermTargets[3],
							&intermSources[2],
							gcvNULL);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		/* sub t4, _PI, t3 */
		slsIOPERAND_New(Compiler, &intermIOperands[4], Source0->dataType);
		gcsTARGET_InitializeUsingIOperand(&intermTargets[4], &intermIOperands[4]);
		gcsSOURCE_InitializeFloatConstant(&constSource, _PI);
		gcsSOURCE_InitializeUsingIOperand(&intermSources[3], &intermIOperands[3]);

		status = _EmitCode(
							Compiler,
							LineNo,
							StringNo,
							gcSL_SUB,
							&intermTargets[4],
							&constSource,
							&intermSources[3]);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcsSOURCE_InitializeUsingIOperand(&intermSources[4], &intermIOperands[4]);


		for (i = 0; i < gcGetVectorDataTypeComponentCount(Target->dataType); i++)
		{
			gcsTARGET_InitializeAsVectorComponent(&componentTarget, Target, i);
			gcsSOURCE_InitializeAsVectorComponent(&componentSource0, Source0, i);
			gcsSOURCE_InitializeAsVectorComponent(&componentSource1, Source1, i);

			for (j = 0; j < 5; j++)
			{
				gcsSOURCE_InitializeAsVectorComponent(
													&componentIntermSources[j],
													&intermSources[j],
													i);
			}

			status = _EmitVectorComponentAtan2SelectionCode(
															Compiler,
															LineNo,
															StringNo,
															&componentTarget,
															&componentSource0,
															&componentSource1,
															componentIntermSources);

			if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
		}
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitDP2Code(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS		status;
	slsIOPERAND		intermIOperand;
	gcsTARGET		intermTarget;
	gcsSOURCE		intermSourceX, intermSourceY;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	/* mul t0, source0, source1 */
	slsIOPERAND_New(Compiler, &intermIOperand, gcSHADER_FLOAT_X2);
	gcsTARGET_InitializeUsingIOperand(&intermTarget, &intermIOperand);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_MUL,
					&intermTarget,
					Source0,
					Source1);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* add target, t0.x, t0.y */
	gcsSOURCE_InitializeUsingIOperandAsVectorComponent(
													&intermSourceX,
													&intermIOperand,
													gcSL_SWIZZLE_XXXX);

	gcsSOURCE_InitializeUsingIOperandAsVectorComponent(
													&intermSourceY,
													&intermIOperand,
													gcSL_SWIZZLE_YYYY);

	status = _EmitCode(
					Compiler,
					LineNo,
					StringNo,
					gcSL_ADD,
					Target,
					&intermSourceX,
					&intermSourceY);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitScalarCompareCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleCONDITION Condition,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS	status;
	gctLABEL	elseLabel, endLabel;
	gcsSOURCE	constSource;

    gcmHEADER();

	elseLabel	= slNewLabel(Compiler);
	endLabel	= slNewLabel(Compiler);

	/* jump else if true */
	status = slEmitCompareBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									Condition,
									elseLabel,
									Source0,
									Source1);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, false */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvFALSE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* jump end */
	status = slEmitAlwaysBranchCode(
									Compiler,
									LineNo,
									StringNo,
									slvOPCODE_JUMP,
									endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* else: */
	status = slSetLabel(Compiler, LineNo, StringNo, elseLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* mov target, true */
	gcsSOURCE_InitializeFloatConstant(&constSource, slmB2F(gcvTRUE));

	status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MOV,
						Target,
						&constSource,
						gcvNULL);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* end: */
	status = slSetLabel(Compiler, LineNo, StringNo, endLabel);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitCompareCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleCONDITION Condition,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS	status;
	gctUINT		i;
	gcsTARGET	componentTarget;
	gcsSOURCE	componentSource0, componentSource1;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	if (Target->dataType == gcSHADER_BOOLEAN_X1)
	{
		status = _EmitScalarCompareCode(
										Compiler,
										LineNo,
										StringNo,
										Condition,
										Target,
										Source0,
										Source1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}
	else
	{
		gcmASSERT(Target->dataType == gcSHADER_BOOLEAN_X2
					|| Target->dataType == gcSHADER_BOOLEAN_X3
					|| Target->dataType == gcSHADER_BOOLEAN_X4);

		for (i = 0; i < gcGetVectorDataTypeComponentCount(Target->dataType); i++)
		{
			gcsTARGET_InitializeAsVectorComponent(&componentTarget, Target, i);
			gcsSOURCE_InitializeAsVectorComponent(&componentSource0, Source0, i);
			gcsSOURCE_InitializeAsVectorComponent(&componentSource1, Source1, i);

			status = _EmitScalarCompareCode(
											Compiler,
											LineNo,
											StringNo,
											Condition,
											&componentTarget,
											&componentSource0,
											&componentSource1);

			if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
		}
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EmitLessThanCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
    gceSTATUS status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	status = _EmitCompareCode(
							Compiler,
							LineNo,
							StringNo,
							slvCONDITION_LESS_THAN,
							Target,
							Source0,
							Source1);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_EmitLessThanEqualCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
    gceSTATUS status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	status = _EmitCompareCode(
							Compiler,
							LineNo,
							StringNo,
							slvCONDITION_LESS_THAN_EQUAL,
							Target,
							Source0,
							Source1);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_EmitGreaterThanCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
    gceSTATUS status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

    status = _EmitCompareCode(
							Compiler,
							LineNo,
							StringNo,
							slvCONDITION_GREATER_THAN,
							Target,
							Source0,
							Source1);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_EmitGreaterThanEqualCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
    gceSTATUS status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	status = _EmitCompareCode(
							Compiler,
							LineNo,
							StringNo,
							slvCONDITION_GREATER_THAN_EQUAL,
							Target,
							Source0,
							Source1);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_EmitEqualCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
    gceSTATUS status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	status = _EmitCompareCode(
							Compiler,
							LineNo,
							StringNo,
							slvCONDITION_EQUAL,
							Target,
							Source0,
							Source1);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_EmitNotEqualCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	status = _EmitCompareCode(
							Compiler,
							LineNo,
							StringNo,
							slvCONDITION_NOT_EQUAL,
							Target,
							Source0,
							Source1);

    gcmFOOTER();
    return status;
}

static gceSTATUS
_EmitDotCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS status;

    gcmHEADER();

	gcmASSERT(Target);
	gcmASSERT(Source0);
	gcmASSERT(Source1);

	switch (Source0->dataType)
	{
	case gcSHADER_FLOAT_X1:
		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_MUL,
						Target,
						Source0,
						Source1);
        break;

	case gcSHADER_FLOAT_X2:
		status = _EmitDP2Code(
							Compiler,
							LineNo,
							StringNo,
							Target,
							Source0,
							Source1);
        break;

	case gcSHADER_FLOAT_X3:
		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_DP3,
						Target,
						Source0,
						Source1);
        break;

	case gcSHADER_FLOAT_X4:
		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						gcSL_DP4,
						Target,
						Source0,
						Source1);
        break;

	default:
        status = gcvSTATUS_OK;
		gcmASSERT(0);
	}

    gcmFOOTER();
	return status;
}

typedef gceSTATUS
(* sltEMIT_SPECIAL_CODE_FUNC_PTR2)(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	);

typedef struct _slsSPECIAL_CODE_EMITTER2
{
	sleOPCODE						opcode;

	sltEMIT_SPECIAL_CODE_FUNC_PTR2	codeEmitter;
}
slsSPECIAL_CODE_EMITTER2;

static slsSPECIAL_CODE_EMITTER2 SpecialCodeEmitterTable2[] =
{
	{slvOPCODE_DIV,					_EmitDivCode},

	{slvOPCODE_ATAN2,				_EmitAtan2Code},

	{slvOPCODE_LESS_THAN,			_EmitLessThanCode},
	{slvOPCODE_LESS_THAN_EQUAL,		_EmitLessThanEqualCode},
	{slvOPCODE_GREATER_THAN,		_EmitGreaterThanCode},
	{slvOPCODE_GREATER_THAN_EQUAL,	_EmitGreaterThanEqualCode},
	{slvOPCODE_EQUAL,				_EmitEqualCode},
	{slvOPCODE_NOT_EQUAL,			_EmitNotEqualCode},

	{slvOPCODE_DOT,					_EmitDotCode},
};

const gctUINT SpecialCodeEmitterCount2 =
						sizeof(SpecialCodeEmitterTable2) / sizeof(slsSPECIAL_CODE_EMITTER2);

static gceSTATUS
_EmitCodeImpl2(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS						status;
	gcsSOURCE						newSource0, newSource1;
	gctUINT							i;
	sltEMIT_SPECIAL_CODE_FUNC_PTR2	specialCodeEmitter = gcvNULL;

    gcmHEADER();

	status = _PrepareSource(
							Compiler,
							LineNo,
							StringNo,
							Target,
							Source0,
							&newSource0);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	status = _PrepareAnotherSource(
								Compiler,
								LineNo,
								StringNo,
								Target,
								&newSource0,
								Source1,
								&newSource1);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	for (i = 0; i < SpecialCodeEmitterCount2; i++)
	{
		if (SpecialCodeEmitterTable2[i].opcode == Opcode)
		{
			specialCodeEmitter = SpecialCodeEmitterTable2[i].codeEmitter;
			break;
		}
	}

	if (specialCodeEmitter != gcvNULL)
	{
		status = (*specialCodeEmitter)(
									Compiler,
									LineNo,
									StringNo,
									Target,
									&newSource0,
									&newSource1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}
	else
	{
		status = _EmitCode(
						Compiler,
						LineNo,
						StringNo,
						_ConvOpcode(Opcode),
						Target,
						&newSource0,
						&newSource1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slEmitCode2(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	sloCODE_EMITTER		codeEmitter;
    gceSTATUS           status;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

	codeEmitter = sloCOMPILER_GetCodeEmitter(Compiler);
	gcmASSERT(codeEmitter);

	status = sloCODE_EMITTER_EmitCode2(
									Compiler,
									codeEmitter,
									LineNo,
									StringNo,
									Opcode,
									Target,
									Source0,
									Source1);
    gcmFOOTER();
    return status;
}

gceSTATUS
slEmitAlwaysBranchCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gctLABEL Label
	)
{
	return _EmitBranchCode(
						Compiler,
						LineNo,
						StringNo,
						_ConvOpcode(Opcode),
						gcSL_ALWAYS,
						Label,
						gcvNULL,
						gcvNULL);
}

gceSTATUS
slEmitTestBranchCode(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gctLABEL Label,
	IN gctBOOL TrueBranch,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS		status;
	gcsSOURCE		newSource;
	gcsSOURCE		falseSource;

    gcmHEADER();

	gcmASSERT(Source);

	status = _PrepareSource(
							Compiler,
							LineNo,
							StringNo,
							gcvNULL,
							Source,
							&newSource);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	gcsSOURCE_InitializeFloatConstant(&falseSource, slmB2F(gcvFALSE));

	status = _EmitBranchCode(
						Compiler,
						LineNo,
						StringNo,
						_ConvOpcode(Opcode),
						TrueBranch ? gcSL_NOT_EQUAL : gcSL_EQUAL,
						Label,
						&newSource,
						&falseSource);

    gcmFOOTER();
    return status;
}

gcSL_CONDITION
_ConvCondition(
	IN sleCONDITION Condition
	)
{
	switch (Condition)
	{
	case slvCONDITION_EQUAL:				return gcSL_EQUAL;
	case slvCONDITION_NOT_EQUAL:			return gcSL_NOT_EQUAL;
	case slvCONDITION_LESS_THAN:			return gcSL_LESS;
	case slvCONDITION_LESS_THAN_EQUAL:		return gcSL_LESS_OR_EQUAL;
	case slvCONDITION_GREATER_THAN:			return gcSL_GREATER;
	case slvCONDITION_GREATER_THAN_EQUAL:	return gcSL_GREATER_OR_EQUAL;
	case slvCONDITION_XOR:					return gcSL_XOR;

	default:
		gcmASSERT(0);
		return gcSL_EQUAL;
	}
}

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
	)
{
	gceSTATUS		status;
	gcsSOURCE		newSource1;

    gcmHEADER();

	gcmASSERT(Source0);
	gcmASSERT(Source1);

	status = _PrepareAnotherSource(
								Compiler,
								LineNo,
								StringNo,
								gcvNULL,
								Source0,
								Source1,
								&newSource1);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	status = _EmitBranchCode(
						Compiler,
						LineNo,
						StringNo,
						_ConvOpcode(Opcode),
						_ConvCondition(Condition),
						Label,
						Source0,
						&newSource1);

    gcmFOOTER();
    return status;
}

gceSTATUS
slBeginMainFunction(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo
	)
{
	gceSTATUS			status;
	sloCODE_EMITTER		codeEmitter;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

	codeEmitter = sloCOMPILER_GetCodeEmitter(Compiler);
	gcmASSERT(codeEmitter);

	status = sloCODE_EMITTER_NewBasicBlock(Compiler, codeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slEndMainFunction(
	IN sloCOMPILER Compiler
	)
{
	gceSTATUS			status;
	sloCODE_EMITTER		codeEmitter;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

	codeEmitter = sloCOMPILER_GetCodeEmitter(Compiler);
	gcmASSERT(codeEmitter);

	status = sloCODE_EMITTER_EndBasicBlock(Compiler, codeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slNewFunction(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctCONST_STRING Name,
	OUT gcFUNCTION * Function
	)
{
	gceSTATUS			status;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	gcmASSERT(Name);
	gcmASSERT(Function);

	status = _AddFunction(
						Compiler,
						Name,
						Function);

	if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to add the function: '%s'",
										Name));

        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slNewFunctionArgument(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function,
	IN gcSHADER_TYPE DataType,
	IN gctSIZE_T Length,
	IN gctREG_INDEX TempRegIndex,
	IN gctUINT8 Qualifier
	)
{
	gceSTATUS		status;
	gctSIZE_T		i, j, regCount;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	gcmASSERT(Function);

	switch (DataType)
	{
	case gcSHADER_FLOAT_2X2:	regCount = 2; break;
	case gcSHADER_FLOAT_3X3:	regCount = 3; break;
	case gcSHADER_FLOAT_4X4:	regCount = 4; break;

	default:					regCount = 1; break;
	}

	for (i = 0; i < Length; i++)
	{
		for (j = 0; j < regCount; j++)
		{
			status = _AddFunctionArgument(
										Compiler,
										Function,
										TempRegIndex + (gctREG_INDEX)(i * regCount + j),
										gcGetDefaultEnable(DataType),
										Qualifier);

			if (gcmIS_ERROR(status))
			{
				gcmVERIFY_OK(sloCOMPILER_Report(
												Compiler,
												0,
												0,
												slvREPORT_INTERNAL_ERROR,
												"failed to add the function argument"));

                gcmFOOTER();
				return status;
			}
		}
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slBeginFunction(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gcFUNCTION Function
	)
{
	gceSTATUS			status;
	sloCODE_EMITTER		codeEmitter;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	gcmASSERT(Function);

	codeEmitter = sloCOMPILER_GetCodeEmitter(Compiler);
	gcmASSERT(codeEmitter);

	status = sloCODE_EMITTER_NewBasicBlock(Compiler, codeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	status = _BeginFunction(Compiler, Function);

	if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_INTERNAL_ERROR,
										"failed to begin function"));

        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slEndFunction(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function
	)
{
	gceSTATUS			status;
	sloCODE_EMITTER		codeEmitter;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	gcmASSERT(Function);

	codeEmitter = sloCOMPILER_GetCodeEmitter(Compiler);
	gcmASSERT(codeEmitter);

	status = sloCODE_EMITTER_EndBasicBlock(Compiler, codeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	status = _EndFunction(Compiler, Function);

	if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										0,
										0,
										slvREPORT_INTERNAL_ERROR,
										"failed to end function"));

        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
slGetFunctionLabel(
	IN sloCOMPILER Compiler,
	IN gcFUNCTION Function,
	OUT gctLABEL * Label
	)
{
	gceSTATUS	status;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	gcmASSERT(Function);

	status = _GetFunctionLabel(Compiler, Function, Label);

	if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										0,
										0,
										slvREPORT_INTERNAL_ERROR,
										"failed to get function label"));

        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/* sloCODE_EMITTER */
typedef enum _sleCODE_TYPE
{
	slvCODE_INVALID,
	slvCODE_ONE_OPERAND,
	slvCODE_TWO_OPERANDS
}
sleCODE_TYPE;

typedef struct _slsCODE_INFO
{
	sleCODE_TYPE	type;

	gctUINT			lineNo;

	gctUINT			stringNo;

	sleOPCODE		opcode;

	gcsTARGET		target;

	gcsSOURCE		source0;

	gcsSOURCE		source1;
}
slsCODE_INFO;

struct _sloCODE_EMITTER
{
	slsOBJECT		object;

	slsCODE_INFO	currentCodeInfo;
};

gceSTATUS
sloCODE_EMITTER_Construct(
	IN sloCOMPILER Compiler,
	OUT sloCODE_EMITTER * CodeEmitter
	)
{
	gceSTATUS			status;
	sloCODE_EMITTER		codeEmitter;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	gcmASSERT(CodeEmitter);

	do
	{
        gctPOINTER pointer = gcvNULL;
		status = sloCOMPILER_Allocate(
									Compiler,
									(gctSIZE_T)sizeof(struct _sloCODE_EMITTER),
									&pointer);

		if (gcmIS_ERROR(status)) break;

		/* Initialize the members */
        codeEmitter                         = pointer;
		codeEmitter->object.type			= slvOBJ_CODE_EMITTER;

		codeEmitter->currentCodeInfo.type	= slvCODE_INVALID;

		*CodeEmitter = codeEmitter;

        gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	*CodeEmitter = gcvNULL;

    gcmFOOTER();
	return status;
}

gceSTATUS
sloCODE_EMITTER_Destroy(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter
	)
{
    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	slmVERIFY_OBJECT(CodeEmitter, slvOBJ_CODE_EMITTER);;

	gcmVERIFY_OK(sloCOMPILER_Free(Compiler, CodeEmitter));

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
sloCODE_EMITTER_EmitCurrentCode(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter
	)
{
	gceSTATUS			status;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	slmVERIFY_OBJECT(CodeEmitter, slvOBJ_CODE_EMITTER);

	switch (CodeEmitter->currentCodeInfo.type)
	{
	case slvCODE_INVALID:
		break;

	case slvCODE_ONE_OPERAND:
		CodeEmitter->currentCodeInfo.type = slvCODE_INVALID;

		status = _EmitCodeImpl1(
								Compiler,
								CodeEmitter->currentCodeInfo.lineNo,
								CodeEmitter->currentCodeInfo.stringNo,
								CodeEmitter->currentCodeInfo.opcode,
								&CodeEmitter->currentCodeInfo.target,
								&CodeEmitter->currentCodeInfo.source0);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		break;

	case slvCODE_TWO_OPERANDS:
		CodeEmitter->currentCodeInfo.type = slvCODE_INVALID;

		status = _EmitCodeImpl2(
								Compiler,
								CodeEmitter->currentCodeInfo.lineNo,
								CodeEmitter->currentCodeInfo.stringNo,
								CodeEmitter->currentCodeInfo.opcode,
								&CodeEmitter->currentCodeInfo.target,
								&CodeEmitter->currentCodeInfo.source0,
								&CodeEmitter->currentCodeInfo.source1);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		break;

	default:
		gcmASSERT(0);
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
sloCODE_EMITTER_NewBasicBlock(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter
	)
{
	gceSTATUS			status;

    gcmHEADER();

	/* End the previous basic block */
	status = sloCODE_EMITTER_EndBasicBlock(Compiler, CodeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
sloCODE_EMITTER_EndBasicBlock(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter
	)
{
	gceSTATUS			status;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	slmVERIFY_OBJECT(CodeEmitter, slvOBJ_CODE_EMITTER);

	status = sloCODE_EMITTER_EmitCurrentCode(Compiler, CodeEmitter);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

#define slmMERGE_DATA_TYPE(dataType0, dataType1) \
	gcConvScalarToVectorDataType( \
		gcGetComponentDataType(dataType0), \
		gcGetDataTypeComponentCount(dataType0) + gcGetDataTypeComponentCount(dataType1))

static void
_MergeEnable(
	IN OUT gctUINT8 * ResultEnable,
	IN gctUINT8 Enable
	)
{
	gcmASSERT(ResultEnable);

	*ResultEnable |= Enable;
}

static void
_MergeEnableAndSwizzle(
	IN OUT gctUINT8 * ResultEnable,
	IN gctUINT8 Enable,
	IN OUT gctUINT8 * ResultSwizzle,
	IN gctUINT8 Swizzle
	)
{
	gcmASSERT(ResultEnable);
	gcmASSERT(ResultSwizzle);

	if (Enable & gcSL_ENABLE_X)
	{
		*ResultSwizzle = (*ResultSwizzle & ~0x03) | (Swizzle & 0x03);
	}

	if (Enable & gcSL_ENABLE_Y)
	{
		*ResultSwizzle = (*ResultSwizzle & ~0x0C) | (Swizzle & 0x0C);
	}

	if (Enable & gcSL_ENABLE_Z)
	{
		*ResultSwizzle = (*ResultSwizzle & ~0x30) | (Swizzle & 0x30);
	}

	if (Enable & gcSL_ENABLE_W)
	{
		*ResultSwizzle = (*ResultSwizzle & ~0xC0) | (Swizzle & 0xC0);
	}

	*ResultEnable |= Enable;
}

static gctBOOL
_CanTargetsBeMerged(
	IN gcsTARGET * Target0,
	IN gcsTARGET * Target1
	)
{
	gcmASSERT(Target0);
	gcmASSERT(Target1);

	do
	{
		if (gcGetComponentDataType(Target0->dataType)
			!= gcGetComponentDataType(Target1->dataType)) break;

		if (Target0->tempRegIndex != Target1->tempRegIndex) break;

		if (Target0->indexMode != Target1->indexMode) break;

		if (Target0->indexMode != gcSL_NOT_INDEXED
			&& Target0->indexRegIndex != Target1->indexRegIndex) break;

		if ((Target0->enable & Target1->enable) != 0) break;

		return gcvTRUE;
	}
	while (gcvFALSE);

	return gcvFALSE;
}

static gctBOOL
_CanSourcesBeMerged(
	IN gcsTARGET * Target0,
	IN gcsSOURCE * Source0,
	IN gcsTARGET * Target1,
	IN gcsSOURCE * Source1
	)
{
	gcmASSERT(Target0);
	gcmASSERT(Source0);
	gcmASSERT(Target1);
	gcmASSERT(Source1);

	do
	{
		if (Source0->type != Source1->type
			|| gcGetComponentDataType(Source0->dataType)
				!= gcGetComponentDataType(Source1->dataType)) break;

		if (Source0->type == gcvSOURCE_CONSTANT)
		{
			if (Source0->u.sourceConstant.u.intConstant
				!= Source1->u.sourceConstant.u.intConstant) break;
		}
		else
		{
			if (Source1->type == gcvSOURCE_TEMP
				&& Source1->u.sourceReg.regIndex == Target0->tempRegIndex) break;

			if (Source0->type != gcvSOURCE_ATTRIBUTE
				&& Source0->u.sourceReg.u.attribute
					!= Source1->u.sourceReg.u.attribute) break;

			if (Source0->type != gcvSOURCE_UNIFORM
				&& Source0->u.sourceReg.u.uniform
					!= Source1->u.sourceReg.u.uniform) break;

			if (Source0->u.sourceReg.regIndex
				!= Source1->u.sourceReg.regIndex) break;

			if (Source0->u.sourceReg.indexMode
				!= Source1->u.sourceReg.indexMode) break;

			if (Source0->u.sourceReg.indexMode != gcSL_NOT_INDEXED
				&& Source0->u.sourceReg.indexRegIndex
					!= Source1->u.sourceReg.indexRegIndex) break;
		}

		return gcvTRUE;
	}
	while (gcvFALSE);

	return gcvFALSE;
}

gceSTATUS
sloCODE_EMITTER_TryToMergeCode1(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source,
	OUT gctBOOL * Merged
	)
{

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	slmVERIFY_OBJECT(CodeEmitter, slvOBJ_CODE_EMITTER);
	gcmASSERT(Merged);

	do
	{
		/* Check the type and opcode */
		if (CodeEmitter->currentCodeInfo.type != slvCODE_ONE_OPERAND
			|| CodeEmitter->currentCodeInfo.opcode != Opcode) break;

		/* Check the target */
		if (!_CanTargetsBeMerged(&CodeEmitter->currentCodeInfo.target, Target)) break;

		/* Check the source */
		if (!_CanSourcesBeMerged(
								&CodeEmitter->currentCodeInfo.target,
								&CodeEmitter->currentCodeInfo.source0,
								Target,
								Source)) break;

		/* Merge the code */
		CodeEmitter->currentCodeInfo.target.dataType =
				slmMERGE_DATA_TYPE(CodeEmitter->currentCodeInfo.target.dataType, Target->dataType);

		CodeEmitter->currentCodeInfo.source0.dataType =
				slmMERGE_DATA_TYPE(CodeEmitter->currentCodeInfo.source0.dataType, Source->dataType);

		if (CodeEmitter->currentCodeInfo.source0.type == gcvSOURCE_CONSTANT)
		{
			_MergeEnable(&CodeEmitter->currentCodeInfo.target.enable, Target->enable);
		}
		else
		{
			_MergeEnableAndSwizzle(
								&CodeEmitter->currentCodeInfo.target.enable,
								Target->enable,
								&CodeEmitter->currentCodeInfo.source0.u.sourceReg.swizzle,
								Source->u.sourceReg.swizzle);
		}

		*Merged = gcvTRUE;

        gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	*Merged = gcvFALSE;

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
sloCODE_EMITTER_EmitCode1(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source
	)
{
	gceSTATUS	status;
	gctBOOL		merged;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	slmVERIFY_OBJECT(CodeEmitter, slvOBJ_CODE_EMITTER);

	if (!sloCOMPILER_OptimizationEnabled(Compiler, slvOPTIMIZATION_DATA_FLOW))
	{
		status = _EmitCodeImpl1(
							Compiler,
							LineNo,
							StringNo,
							Opcode,
							Target,
							Source);

        gcmFOOTER();
        return status;
	}

	status = sloCODE_EMITTER_TryToMergeCode1(
											Compiler,
											CodeEmitter,
											LineNo,
											StringNo,
											Opcode,
											Target,
											Source,
											&merged);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	if (!merged)
	{
		status = sloCODE_EMITTER_EmitCurrentCode(Compiler, CodeEmitter);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		CodeEmitter->currentCodeInfo.type		= slvCODE_ONE_OPERAND;
		CodeEmitter->currentCodeInfo.lineNo		= LineNo;
		CodeEmitter->currentCodeInfo.stringNo	= StringNo;
		CodeEmitter->currentCodeInfo.opcode		= Opcode;
		CodeEmitter->currentCodeInfo.target		= *Target;
		CodeEmitter->currentCodeInfo.source0	= *Source;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static void
_MergeEnableAndTwoSwizzles(
	IN OUT gctUINT8 * ResultEnable,
	IN gctUINT8 Enable,
	IN OUT gctUINT8 * ResultSwizzle0,
	IN gctUINT8 Swizzle0,
	IN OUT gctUINT8 * ResultSwizzle1,
	IN gctUINT8 Swizzle1
	)
{
	gcmASSERT(ResultEnable);
	gcmASSERT(ResultSwizzle0);
	gcmASSERT(ResultSwizzle1);

	if (Enable & gcSL_ENABLE_X)
	{
		*ResultSwizzle0 = (*ResultSwizzle0 & ~0x03) | (Swizzle0 & 0x03);
		*ResultSwizzle1 = (*ResultSwizzle1 & ~0x03) | (Swizzle1 & 0x03);
	}

	if (Enable & gcSL_ENABLE_Y)
	{
		*ResultSwizzle0 = (*ResultSwizzle0 & ~0x0C) | (Swizzle0 & 0x0C);
		*ResultSwizzle1 = (*ResultSwizzle1 & ~0x0C) | (Swizzle1 & 0x0C);
	}

	if (Enable & gcSL_ENABLE_Z)
	{
		*ResultSwizzle0 = (*ResultSwizzle0 & ~0x30) | (Swizzle0 & 0x30);
		*ResultSwizzle1 = (*ResultSwizzle1 & ~0x30) | (Swizzle1 & 0x30);
	}

	if (Enable & gcSL_ENABLE_W)
	{
		*ResultSwizzle0 = (*ResultSwizzle0 & ~0xC0) | (Swizzle0 & 0xC0);
		*ResultSwizzle1 = (*ResultSwizzle1 & ~0xC0) | (Swizzle1 & 0xC0);
	}

	*ResultEnable |= Enable;
}

gceSTATUS
sloCODE_EMITTER_TryToMergeCode2(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1,
	OUT gctBOOL * Merged
	)
{
    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	slmVERIFY_OBJECT(CodeEmitter, slvOBJ_CODE_EMITTER);
	gcmASSERT(Merged);

	do
	{
		/* Check the type and opcode */
		if (CodeEmitter->currentCodeInfo.type != slvCODE_TWO_OPERANDS
			|| CodeEmitter->currentCodeInfo.opcode != Opcode) break;

		/* Check the target */
		if (!_CanTargetsBeMerged(&CodeEmitter->currentCodeInfo.target, Target)) break;

		/* Check the sources */
		if (!_CanSourcesBeMerged(
								&CodeEmitter->currentCodeInfo.target,
								&CodeEmitter->currentCodeInfo.source0,
								Target,
								Source0)) break;

		if (!_CanSourcesBeMerged(
								&CodeEmitter->currentCodeInfo.target,
								&CodeEmitter->currentCodeInfo.source1,
								Target,
								Source1)) break;

		/* Merge the code */
		CodeEmitter->currentCodeInfo.target.dataType =
				slmMERGE_DATA_TYPE(CodeEmitter->currentCodeInfo.target.dataType, Target->dataType);

		CodeEmitter->currentCodeInfo.source0.dataType =
				slmMERGE_DATA_TYPE(CodeEmitter->currentCodeInfo.source0.dataType, Source0->dataType);

		CodeEmitter->currentCodeInfo.source1.dataType =
				slmMERGE_DATA_TYPE(CodeEmitter->currentCodeInfo.source1.dataType, Source1->dataType);

		if (CodeEmitter->currentCodeInfo.source0.type != gcvSOURCE_CONSTANT
			&& CodeEmitter->currentCodeInfo.source1.type != gcvSOURCE_CONSTANT)
		{
			_MergeEnableAndTwoSwizzles(
								&CodeEmitter->currentCodeInfo.target.enable,
								Target->enable,
								&CodeEmitter->currentCodeInfo.source0.u.sourceReg.swizzle,
								Source0->u.sourceReg.swizzle,
								&CodeEmitter->currentCodeInfo.source1.u.sourceReg.swizzle,
								Source1->u.sourceReg.swizzle);
		}
		else if (CodeEmitter->currentCodeInfo.source0.type != gcvSOURCE_CONSTANT)
		{
			_MergeEnableAndSwizzle(
								&CodeEmitter->currentCodeInfo.target.enable,
								Target->enable,
								&CodeEmitter->currentCodeInfo.source0.u.sourceReg.swizzle,
								Source0->u.sourceReg.swizzle);
		}
		else if (CodeEmitter->currentCodeInfo.source1.type != gcvSOURCE_CONSTANT)
		{
			_MergeEnableAndSwizzle(
								&CodeEmitter->currentCodeInfo.target.enable,
								Target->enable,
								&CodeEmitter->currentCodeInfo.source1.u.sourceReg.swizzle,
								Source1->u.sourceReg.swizzle);
		}
		else
		{
			_MergeEnable(&CodeEmitter->currentCodeInfo.target.enable, Target->enable);
		}

		*Merged = gcvTRUE;

        gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	*Merged = gcvFALSE;

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
sloCODE_EMITTER_EmitCode2(
	IN sloCOMPILER Compiler,
	IN sloCODE_EMITTER CodeEmitter,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleOPCODE Opcode,
	IN gcsTARGET * Target,
	IN gcsSOURCE * Source0,
	IN gcsSOURCE * Source1
	)
{
	gceSTATUS	status;
	gctBOOL		merged;

    gcmHEADER();

	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
	slmVERIFY_OBJECT(CodeEmitter, slvOBJ_CODE_EMITTER);

	if (!sloCOMPILER_OptimizationEnabled(Compiler, slvOPTIMIZATION_DATA_FLOW))
	{
		status = _EmitCodeImpl2(
							Compiler,
							LineNo,
							StringNo,
							Opcode,
							Target,
							Source0,
							Source1);

        gcmFOOTER();
        return status;
	}

	status = sloCODE_EMITTER_TryToMergeCode2(
											Compiler,
											CodeEmitter,
											LineNo,
											StringNo,
											Opcode,
											Target,
											Source0,
											Source1,
											&merged);

	if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	if (!merged)
	{
		status = sloCODE_EMITTER_EmitCurrentCode(Compiler, CodeEmitter);

		if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

		CodeEmitter->currentCodeInfo.type		= slvCODE_TWO_OPERANDS;
		CodeEmitter->currentCodeInfo.lineNo		= LineNo;
		CodeEmitter->currentCodeInfo.stringNo	= StringNo;
		CodeEmitter->currentCodeInfo.opcode		= Opcode;
		CodeEmitter->currentCodeInfo.target		= *Target;
		CodeEmitter->currentCodeInfo.source0	= *Source0;
		CodeEmitter->currentCodeInfo.source1	= *Source1;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}
