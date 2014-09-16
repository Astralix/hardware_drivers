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


#include "gc_glsl_gen_code.h"
#include "gc_glsl_emit_code.h"

#define SUPPORT_STRUCT_ELEMENT_IN_VARIABLE    0

void
slsOPERAND_CONSTANT_ChangeFloatFamilyDataType(
    IN gcSHADER_TYPE NewDataType,
    IN OUT slsOPERAND_CONSTANT * OperandConstant
    )
{
    gctUINT     i;

    gcmASSERT(OperandConstant);

    switch (gcGetComponentDataType(NewDataType))
    {
    case gcSHADER_FLOAT_X1:
        break;

    case gcSHADER_BOOLEAN_X1:
        for (i = 0; i < OperandConstant->valueCount; i++)
        {
            OperandConstant->values[i].boolValue = slmF2B(OperandConstant->values[i].floatValue);
        }
        break;

    case gcSHADER_INTEGER_X1:
        for (i = 0; i < OperandConstant->valueCount; i++)
        {
            OperandConstant->values[i].intValue = slmF2I(OperandConstant->values[i].floatValue);
        }
        break;

    default:
        gcmASSERT(0);
    }

    OperandConstant->dataType   = NewDataType;
}

void
slsOPERAND_CONSTANT_ChangeIntegerFamilyDataType(
    IN gcSHADER_TYPE NewDataType,
    IN OUT slsOPERAND_CONSTANT * OperandConstant
    )
{
    gctUINT     i;

    gcmASSERT(OperandConstant);

    switch (gcGetComponentDataType(NewDataType))
    {
    case gcSHADER_FLOAT_X1:
        for (i = 0; i < OperandConstant->valueCount; i++)
        {
            OperandConstant->values[i].floatValue = slmI2F(OperandConstant->values[i].intValue);
        }
        break;

    case gcSHADER_BOOLEAN_X1:
        for (i = 0; i < OperandConstant->valueCount; i++)
        {
            OperandConstant->values[i].boolValue = slmI2B(OperandConstant->values[i].intValue);
        }
        break;

    case gcSHADER_INTEGER_X1:
        break;

    default:
        gcmASSERT(0);
    }

    OperandConstant->dataType   = NewDataType;
}

void
slsOPERAND_CONSTANT_ChangeBooleanFamilyDataType(
    IN gcSHADER_TYPE NewDataType,
    IN OUT slsOPERAND_CONSTANT * OperandConstant
    )
{
    gctUINT     i;

    gcmASSERT(OperandConstant);

    switch (gcGetComponentDataType(NewDataType))
    {
    case gcSHADER_FLOAT_X1:
        for (i = 0; i < OperandConstant->valueCount; i++)
        {
            OperandConstant->values[i].floatValue = slmB2F(OperandConstant->values[i].boolValue);
        }
        break;

    case gcSHADER_BOOLEAN_X1:
        break;

    case gcSHADER_INTEGER_X1:
        for (i = 0; i < OperandConstant->valueCount; i++)
        {
            OperandConstant->values[i].intValue = slmB2I(OperandConstant->values[i].boolValue);
        }
        break;

    default:
        gcmASSERT(0);
    }

    OperandConstant->dataType   = NewDataType;
}

gceSTATUS
slsROPERAND_ChangeDataTypeFamily(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN gctBOOL TreatFloatAsInt,
    IN gcSHADER_TYPE NewDataType,
    IN OUT slsROPERAND * ROperand
    )
{
    gceSTATUS       status;
    sleOPCODE       opcode;
    slsIOPERAND     intermIOperand;

    gcmHEADER();

    gcmASSERT(ROperand);
    gcmASSERT(gcGetDataTypeComponentCount(ROperand->dataType)
                == gcGetDataTypeComponentCount(NewDataType));

    if (ROperand->isReg)
    {
        opcode = slvOPCODE_INVALID;

        switch (gcGetComponentDataType(NewDataType))
        {
        case gcSHADER_BOOLEAN_X1:
            switch (gcGetComponentDataType(ROperand->dataType))
            {
            case gcSHADER_FLOAT_X1:     opcode = slvOPCODE_FLOAT_TO_BOOL;   break;
            case gcSHADER_INTEGER_X1:   opcode = slvOPCODE_INT_TO_BOOL;     break;
            default: break;
            }
            break;

        case gcSHADER_INTEGER_X1:
            switch (gcGetComponentDataType(ROperand->dataType))
            {
            case gcSHADER_FLOAT_X1:
                if (!TreatFloatAsInt) opcode = slvOPCODE_FLOAT_TO_INT;
                break;
            default: break;
            }
            break;

        default: break;
        }

        if (opcode == slvOPCODE_INVALID)
        {
            ROperand->dataType  = NewDataType;
        }
        else
        {
            slsIOPERAND_New(Compiler, &intermIOperand, NewDataType);

            status = slGenGenericCode1(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    opcode,
                                    &intermIOperand,
                                    ROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            slsROPERAND_InitializeUsingIOperand(ROperand, &intermIOperand);
        }
    }
    else
    {
        switch (gcGetComponentDataType(ROperand->dataType))
        {
        case gcSHADER_FLOAT_X1:
            slsOPERAND_CONSTANT_ChangeFloatFamilyDataType(NewDataType, &ROperand->u.constant);
            break;

        case gcSHADER_BOOLEAN_X1:
            slsOPERAND_CONSTANT_ChangeBooleanFamilyDataType(NewDataType, &ROperand->u.constant);
            break;

        case gcSHADER_INTEGER_X1:
            slsOPERAND_CONSTANT_ChangeIntegerFamilyDataType(NewDataType, &ROperand->u.constant);
            break;

        default:
            gcmASSERT(0);
        }

        ROperand->dataType  = NewDataType;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

void
slsROPERAND_CONSTANT_ConvScalarToVector(
    IN sloCOMPILER Compiler,
    IN gcSHADER_TYPE NewDataType,
    IN OUT slsROPERAND * ROperand
    )
{
    gctUINT     i;
    gctUINT8 vectorComponentCount;
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x NewDataType=%d ROperand=0x%x",
                  Compiler, NewDataType, ROperand);

    gcmASSERT(ROperand);
    gcmASSERT(gcIsScalarDataType(ROperand->dataType));
    gcmASSERT(!ROperand->isReg);

    vectorComponentCount = gcGetDataTypeComponentCount(NewDataType);

    switch (ROperand->dataType)
    {
    case gcSHADER_FLOAT_X1:
        for (i = 1; i < vectorComponentCount; i++)
        {
            ROperand->u.constant.values[i].floatValue = ROperand->u.constant.values[0].floatValue;
        }
        break;

    case gcSHADER_BOOLEAN_X1:
        for (i = 1; i < vectorComponentCount; i++)
        {
            ROperand->u.constant.values[i].boolValue = ROperand->u.constant.values[0].boolValue;
        }
        break;

    case gcSHADER_INTEGER_X1:
        for (i = 1; i < vectorComponentCount; i++)
        {
            ROperand->u.constant.values[i].intValue = ROperand->u.constant.values[0].intValue;
        }
        break;

    default:
        gcmASSERT(0);
    }

    ROperand->u.constant.valueCount = vectorComponentCount;

    ROperand->dataType = gcConvScalarToVectorDataType(ROperand->dataType, vectorComponentCount);

    status = slsROPERAND_ChangeDataTypeFamily(
                                             Compiler,
                                             0,
                                             0,
                                             gcvFALSE,
                                             NewDataType,
                                             ROperand);
    if (gcmIS_ERROR(status))
	{
		gcmFOOTER();
		return;
	}

    gcmFOOTER_NO();
}

gctBOOL
slsROPERAND_CONSTANT_IsAllVectorComponentsEqual(
    IN slsROPERAND * ROperand
    )
{
    gctUINT     i;

    gcmASSERT(ROperand);
    gcmASSERT(!ROperand->isReg);

    switch (gcGetVectorComponentDataType(ROperand->dataType))
    {
    case gcSHADER_FLOAT_X1:
        for (i = 1; i < ROperand->u.constant.valueCount; i++)
        {
            if (ROperand->u.constant.values[i].floatValue
                != ROperand->u.constant.values[0].floatValue)
            {
                return gcvFALSE;
            }
        }
        break;

    case gcSHADER_BOOLEAN_X1:
        for (i = 1; i < ROperand->u.constant.valueCount; i++)
        {
            if (ROperand->u.constant.values[i].boolValue
                != ROperand->u.constant.values[0].boolValue)
            {
                return gcvFALSE;
            }
        }
        break;

    case gcSHADER_INTEGER_X1:
        for (i = 1; i < ROperand->u.constant.valueCount; i++)
        {
            if (ROperand->u.constant.values[i].intValue
                != ROperand->u.constant.values[0].intValue)
            {
                return gcvFALSE;
            }
        }
        break;

    default:
        gcmASSERT(0);
    }

    return gcvTRUE;
}

gctCONST_STRING
slGetOpcodeName(
    IN sleOPCODE Opcode
    )
{
    switch (Opcode)
    {
    case slvOPCODE_ASSIGN:              return "assign";

    case slvOPCODE_ADD:                 return "add";
    case slvOPCODE_SUB:                 return "sub";
    case slvOPCODE_MUL:                 return "mul";
    case slvOPCODE_DIV:                 return "div";

    case slvOPCODE_TEXTURE_LOAD:        return "texture_load";
    case slvOPCODE_TEXTURE_LOAD_PROJ:   return "texture_load_proj";
    case slvOPCODE_TEXTURE_BIAS:        return "texture_bias";
    case slvOPCODE_TEXTURE_LOD:         return "texture_lod";

    case slvOPCODE_FLOAT_TO_INT:        return "float_to_int";
    case slvOPCODE_FLOAT_TO_BOOL:       return "float_to_bool";
    case slvOPCODE_INT_TO_BOOL:         return "int_to_bool";

    case slvOPCODE_INVERSE:             return "inverse";
    case slvOPCODE_LESS_THAN:           return "less_than";
    case slvOPCODE_LESS_THAN_EQUAL:     return "less_than_equal";
    case slvOPCODE_GREATER_THAN:        return "greater_than";
    case slvOPCODE_GREATER_THAN_EQUAL:  return "greater_than_equal";
    case slvOPCODE_EQUAL:               return "equal";
    case slvOPCODE_NOT_EQUAL:           return "not_equal";

    case slvOPCODE_ANY:                 return "any";
    case slvOPCODE_ALL:                 return "all";
    case slvOPCODE_NOT:                 return "not";

    case slvOPCODE_SIN:                 return "sin";
    case slvOPCODE_COS:                 return "cos";
    case slvOPCODE_TAN:                 return "tan";

    case slvOPCODE_ASIN:                return "asin";
    case slvOPCODE_ACOS:                return "acos";
    case slvOPCODE_ATAN:                return "atan";
    case slvOPCODE_ATAN2:               return "atan2";

    case slvOPCODE_POW:                 return "pow";
    case slvOPCODE_EXP2:                return "exp2";
    case slvOPCODE_LOG2:                return "log2";
    case slvOPCODE_SQRT:                return "sqrt";
    case slvOPCODE_INVERSE_SQRT:        return "inverse_sqrt";

    case slvOPCODE_ABS:                 return "abs";
    case slvOPCODE_SIGN:                return "sign";
    case slvOPCODE_FLOOR:               return "floor";
    case slvOPCODE_CEIL:                return "ceil";
    case slvOPCODE_FRACT:               return "fract";
    case slvOPCODE_MIN:                 return "min";
    case slvOPCODE_MAX:                 return "max";
    case slvOPCODE_SATURATE:            return "saturate";
    case slvOPCODE_STEP:                return "step";
    case slvOPCODE_DOT:                 return "dot";
    case slvOPCODE_CROSS:               return "cross";
    case slvOPCODE_NORMALIZE:           return "normalize";

    case slvOPCODE_JUMP:                return "jump";
    case slvOPCODE_CALL:                return "call";
    case slvOPCODE_RETURN:              return "return";
    case slvOPCODE_DISCARD:             return "discard";

    case slvOPCODE_DFDX:                return "dFdx";
    case slvOPCODE_DFDY:                return "dFdy";
    case slvOPCODE_FWIDTH:              return "fwidth";

    default:
        gcmASSERT(0);
        return "Invalid";
    }
}

gctCONST_STRING
slGetConditionName(
    IN sleCONDITION Condition
    )
{
    switch (Condition)
    {
    case slvCONDITION_EQUAL:                return "equal";
    case slvCONDITION_NOT_EQUAL:            return "not_equal";
    case slvCONDITION_LESS_THAN:            return "less_than";
    case slvCONDITION_LESS_THAN_EQUAL:      return "less_than_equal";
    case slvCONDITION_GREATER_THAN:         return "greater_than";
    case slvCONDITION_GREATER_THAN_EQUAL:   return "greater_than_equal";
    case slvCONDITION_XOR:                  return "xor";

    default:
        gcmASSERT(0);
        return "Invalid";
    }
}

sleCONDITION
slGetNotCondition(
    IN sleCONDITION Condition
    )
{
    switch (Condition)
    {
    case slvCONDITION_EQUAL:                return slvCONDITION_NOT_EQUAL;
    case slvCONDITION_NOT_EQUAL:
    case slvCONDITION_XOR:                  return slvCONDITION_EQUAL;
    case slvCONDITION_LESS_THAN:            return slvCONDITION_GREATER_THAN_EQUAL;
    case slvCONDITION_LESS_THAN_EQUAL:      return slvCONDITION_GREATER_THAN;
    case slvCONDITION_GREATER_THAN:         return slvCONDITION_LESS_THAN_EQUAL;
    case slvCONDITION_GREATER_THAN_EQUAL:   return slvCONDITION_LESS_THAN;

    default:
        gcmASSERT(0);
        return slvCONDITION_NOT_EQUAL;
    }
}

gctBOOL
slsROPERAND_IsFloatOrVecConstant(
    IN slsROPERAND * ROperand,
    IN gctFLOAT FloatValue
    )
{
    gcmASSERT(ROperand);

    if (ROperand->isReg) return gcvFALSE;

    switch (ROperand->dataType)
    {
    case gcSHADER_FLOAT_X1:
        return (ROperand->u.constant.values[0].floatValue == FloatValue);

    case gcSHADER_FLOAT_X2:
        return (ROperand->u.constant.values[0].floatValue == FloatValue
                && ROperand->u.constant.values[1].floatValue == FloatValue);

    case gcSHADER_FLOAT_X3:
        return (ROperand->u.constant.values[0].floatValue == FloatValue
                && ROperand->u.constant.values[1].floatValue == FloatValue
                && ROperand->u.constant.values[2].floatValue == FloatValue);

    case gcSHADER_FLOAT_X4:
        return (ROperand->u.constant.values[0].floatValue == FloatValue
                && ROperand->u.constant.values[1].floatValue == FloatValue
                && ROperand->u.constant.values[2].floatValue == FloatValue
                && ROperand->u.constant.values[3].floatValue == FloatValue);

    default: return gcvFALSE;
    }
}


static gctUINT
_GetLogicalOperandCount(
    IN slsDATA_TYPE * DataType
    )
{
    gctUINT     count = 0;
    slsNAME *   fieldName;

    gcmASSERT(DataType);

    if (DataType->elementType == slvTYPE_STRUCT)
    {
        gcmASSERT(DataType->fieldSpace);

        FOR_EACH_DLINK_NODE(&DataType->fieldSpace->names, slsNAME, fieldName)
        {
            gcmASSERT(fieldName->dataType);
            count += _GetLogicalOperandCount(fieldName->dataType);
        }
    }
    else
    {
        count = 1;
    }

    if (DataType->arrayLength > 0)
    {
        count *= DataType->arrayLength;
    }

    return count;
}

static gctSIZE_T
_GetLogicalOperandFieldOffset(
    IN slsDATA_TYPE * StructDataType,
    IN slsNAME * FieldName
    )
{
    gctSIZE_T   offset = 0;
    slsNAME *   fieldName;

    gcmASSERT(StructDataType);
    gcmASSERT(slsDATA_TYPE_IsStruct(StructDataType));
    gcmASSERT(FieldName);

    gcmASSERT(StructDataType->fieldSpace);

    FOR_EACH_DLINK_NODE(&StructDataType->fieldSpace->names, slsNAME, fieldName)
    {
        if (fieldName == FieldName) break;

        gcmASSERT(fieldName->dataType);
        offset += _GetLogicalOperandCount(fieldName->dataType);
    }

    gcmASSERT(fieldName == FieldName);

    return offset;
}

slsCOMPONENT_SELECTION
slGetDefaultComponentSelection(
    IN gcSHADER_TYPE DataType
    )
{
    switch (DataType)
    {
    case gcSHADER_FLOAT_X1:
    case gcSHADER_BOOLEAN_X1:
    case gcSHADER_INTEGER_X1:
        return ComponentSelection_X;

    case gcSHADER_FLOAT_X2:
    case gcSHADER_FLOAT_2X2:
    case gcSHADER_BOOLEAN_X2:
    case gcSHADER_INTEGER_X2:
        return ComponentSelection_XY;

    case gcSHADER_FLOAT_X3:
    case gcSHADER_FLOAT_3X3:
    case gcSHADER_BOOLEAN_X3:
    case gcSHADER_INTEGER_X3:
        return ComponentSelection_XYZ;

    case gcSHADER_FLOAT_X4:
    case gcSHADER_FLOAT_4X4:
    case gcSHADER_BOOLEAN_X4:
    case gcSHADER_INTEGER_X4:
    case gcSHADER_SAMPLER_1D:
    case gcSHADER_SAMPLER_2D:
    case gcSHADER_SAMPLER_3D:
    case gcSHADER_SAMPLER_CUBIC:
    case gcSHADER_SAMPLER_EXTERNAL_OES:
        return ComponentSelection_XYZW;

    default:
        gcmASSERT(0);
        return ComponentSelection_XYZW;
    }
}

static gcSHADER_TYPE
_ConvElementDataType(
    IN slsDATA_TYPE * DataType
    )
{
    gcmASSERT(DataType);

    switch (DataType->elementType)
    {
    case slvTYPE_BOOL:
        switch (slmDATA_TYPE_vectorSize_GET(DataType))
        {
        case 0: return gcSHADER_BOOLEAN_X1;
        case 2: return gcSHADER_BOOLEAN_X2;
        case 3: return gcSHADER_BOOLEAN_X3;
        case 4: return gcSHADER_BOOLEAN_X4;

        default:
            gcmASSERT(0);
            return gcSHADER_BOOLEAN_X4;
        }

    case slvTYPE_INT:
        switch (slmDATA_TYPE_vectorSize_GET(DataType))
        {
        case 0: return gcSHADER_INTEGER_X1;
        case 2: return gcSHADER_INTEGER_X2;
        case 3: return gcSHADER_INTEGER_X3;
        case 4: return gcSHADER_INTEGER_X4;

        default:
            gcmASSERT(0);
            return gcSHADER_INTEGER_X4;
        }

    case slvTYPE_FLOAT:
        switch (slmDATA_TYPE_matrixSize_GET(DataType))
        {
        case 0:
            switch (slmDATA_TYPE_vectorSize_GET(DataType))
            {
            case 0: return gcSHADER_FLOAT_X1;
            case 2: return gcSHADER_FLOAT_X2;
            case 3: return gcSHADER_FLOAT_X3;
            case 4: return gcSHADER_FLOAT_X4;

            default:
                gcmASSERT(0);
                return gcSHADER_FLOAT_X4;
            }

        case 2: return gcSHADER_FLOAT_2X2;
        case 3: return gcSHADER_FLOAT_3X3;
        case 4: return gcSHADER_FLOAT_4X4;

        default:
            gcmASSERT(0);
            return gcSHADER_FLOAT_4X4;
        }

    case slvTYPE_SAMPLER2D:
        return gcSHADER_SAMPLER_2D;

    case slvTYPE_SAMPLERCUBE:
        return gcSHADER_SAMPLER_CUBIC;

    case slvTYPE_SAMPLER1DARRAY:
    case slvTYPE_SAMPLER1DARRAYSHADOW:
        return gcSHADER_SAMPLER_2D;

    case slvTYPE_SAMPLER3D:
    case slvTYPE_SAMPLER2DARRAY:
    case slvTYPE_SAMPLER2DARRAYSHADOW:
        return gcSHADER_SAMPLER_3D;

    case slvTYPE_ISAMPLERCUBE:
        return gcSHADER_ISAMPLER_CUBIC;

    case slvTYPE_ISAMPLER2D:
        return gcSHADER_ISAMPLER_2D;

    case slvTYPE_ISAMPLER3D:
    case slvTYPE_ISAMPLER2DARRAY:
        return gcSHADER_ISAMPLER_3D;

    case slvTYPE_USAMPLERCUBE:
        return gcSHADER_USAMPLER_CUBIC;

    case slvTYPE_USAMPLER2D:
        return gcSHADER_USAMPLER_2D;

    case slvTYPE_USAMPLER3D:
    case slvTYPE_USAMPLER2DARRAY:
        return gcSHADER_USAMPLER_3D;

    case slvTYPE_SAMPLEREXTERNALOES:
        return gcSHADER_SAMPLER_EXTERNAL_OES;

    default:
        gcmASSERT(0);
        return gcSHADER_FLOAT_X4;
    }
}

static gcSHADER_PRECISION
_ConvElementDataPrecision(
    IN slsDATA_TYPE * DataType
    )
{
    gcmASSERT(DataType);

    switch (DataType->precision)
    {
    case slvPRECISION_DEFAULT:
        return gcSHADER_PRECISION_DEFAULT;
    case slvPRECISION_HIGH:
        return gcSHADER_PRECISION_HIGH;
    case slvPRECISION_MEDIUM:
        return gcSHADER_PRECISION_MEDIUM;
    case slvPRECISION_LOW:
        return gcSHADER_PRECISION_LOW;

    default:
        gcmASSERT(0);
        return gcSHADER_PRECISION_DEFAULT;
    }
}

static gceSTATUS
_ConvDataType(
    IN slsDATA_TYPE * DataType,
    OUT gcSHADER_TYPE * TargetDataTypes,
    IN OUT gctUINT * Start
    )
{
    gceSTATUS   status;
    gctUINT     count, i;
    slsNAME *   fieldName;

    gcmHEADER();

    gcmASSERT(DataType);
    gcmASSERT(TargetDataTypes);
    gcmASSERT(Start);

    count = (DataType->arrayLength > 0) ? DataType->arrayLength : 1;

    for (i = 0; i < count; i++)
    {
        if (DataType->elementType == slvTYPE_STRUCT)
        {
            gcmASSERT(DataType->fieldSpace);

            FOR_EACH_DLINK_NODE(&DataType->fieldSpace->names, slsNAME, fieldName)
            {
                gcmASSERT(fieldName->dataType);

                status = _ConvDataType(
                                    fieldName->dataType,
                                    TargetDataTypes,
                                    Start);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
        }
        else
        {
            TargetDataTypes[*Start] = _ConvElementDataType(DataType);
            (*Start)++;
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctBOOL
_IsTempRegQualifier(
    IN sltQUALIFIER Qualifier
    )
{
    switch (Qualifier)
    {
    case slvQUALIFIER_NONE:
    case slvQUALIFIER_CONST_IN:
    case slvQUALIFIER_IN:
    case slvQUALIFIER_OUT:
    case slvQUALIFIER_INOUT:

    case slvQUALIFIER_VARYING_OUT:
    case slvQUALIFIER_INVARIANT_VARYING_OUT:
    case slvQUALIFIER_FRAGMENT_OUT:
        return gcvTRUE;

    default:
        return gcvFALSE;
    }
}

static gceSTATUS
_AllocLogicalRegOrArray(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsNAME * Name,
    IN gctCONST_STRING Symbol,
    IN slsDATA_TYPE * DataType,
    IN gctINT16 parent,
    IN gctINT16 prevSibling,
    OUT gctINT16* ThisVarIndex,
    OUT gctINT16* FirstRegIndex,
    OUT slsLOGICAL_REG * LogicalRegs,
    IN OUT gctUINT * Start
    )
{
    gceSTATUS          status;
    sltQUALIFIER       qualifier;
    gcSHADER_TYPE      binaryDataType;
    gctSIZE_T          binaryDataTypeSize;
    gctUINT            logicalRegCount, i;
    gctREG_INDEX       tempRegIndex = 0;
    gcUNIFORM          uniform;
    gcATTRIBUTE        attribute;
    gcSHADER_PRECISION binaryPrecision;

    gcmHEADER_ARG("Compiler=0x%x CodeGenerator=0x%x Name=0x%x Symbol=%s "
                  "DataType=0x%x",
                  Compiler, CodeGenerator, Name, gcmOPT_STRING(Symbol),
                  DataType);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmVERIFY_ARGUMENT(Name);
    gcmASSERT(DataType);
    gcmASSERT(!slsDATA_TYPE_IsStruct(DataType));
    gcmASSERT(LogicalRegs);
    gcmASSERT(Start);

    qualifier           = Name->dataType->qualifier;

    if (Name->isBuiltIn)
    {
        gcmVERIFY_OK(slGetBuiltInVariableImplSymbol(Symbol, &Symbol, &qualifier));
    }

    binaryDataType      = _ConvElementDataType(DataType);
    binaryDataTypeSize  = gcGetDataTypeSize(binaryDataType);
    binaryPrecision = _ConvElementDataPrecision(DataType);

    logicalRegCount = (DataType->arrayLength > 0) ? DataType->arrayLength : 1;

    switch (qualifier)
    {
    case slvQUALIFIER_NONE:
    case slvQUALIFIER_CONST_IN:
    case slvQUALIFIER_IN:
    case slvQUALIFIER_OUT:
    case slvQUALIFIER_INOUT:
        tempRegIndex = slNewTempRegs(Compiler, logicalRegCount * binaryDataTypeSize);

        for (i = 0; i < logicalRegCount; i++)
        {
            slsLOGICAL_REG_InitializeTemp(
                                        LogicalRegs + *Start + i,
                                        qualifier,
                                        binaryDataType,
                                        tempRegIndex + (gctREG_INDEX)(i * binaryDataTypeSize));
        }

        if (Name->type == slvFUNC_NAME || Name->type == slvPARAMETER_NAME)
        {
            gctUINT8 argumentQualifier = gcvFUNCTION_INPUT;

            gcmASSERT(Name->context.function);

            switch (qualifier)
            {
            case slvQUALIFIER_NONE:
            case slvQUALIFIER_CONST_IN:
            case slvQUALIFIER_IN:
                argumentQualifier = gcvFUNCTION_INPUT;  break;
            case slvQUALIFIER_OUT:
                argumentQualifier = gcvFUNCTION_OUTPUT; break;
            case slvQUALIFIER_INOUT:
                argumentQualifier = gcvFUNCTION_INOUT;  break;
            }

            status = slNewFunctionArgument(Compiler,
                                           Name->context.function,
                                           binaryDataType,
                                           logicalRegCount,
                                           tempRegIndex,
                                           argumentQualifier);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }
        else
        {
            status = slNewVariable(Compiler,
                                   Name->lineNo,
                                   Name->stringNo,
                                   Symbol,
                                   binaryDataType,
                                   logicalRegCount,
                                   tempRegIndex,
                                   gcSHADER_VAR_CATEGORY_NORMAL,
                                   0,
                                   parent,
                                   prevSibling,
                                   ThisVarIndex);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }

        break;

    case slvQUALIFIER_UNIFORM:
        status = slNewUniform(Compiler,
                              Name->lineNo,
                              Name->stringNo,
                              Symbol,
                              binaryDataType,
                              binaryPrecision,
                              logicalRegCount,
                              &uniform);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }

        for (i = 0; i < logicalRegCount; i++)
        {
            slsLOGICAL_REG_InitializeUniform(LogicalRegs + *Start + i,
                                             qualifier,
                                             binaryDataType,
                                             uniform,
                                             (gctREG_INDEX)(i * binaryDataTypeSize));
        }
        break;

    case slvQUALIFIER_ATTRIBUTE:
    case slvQUALIFIER_VARYING_IN:
    case slvQUALIFIER_INVARIANT_VARYING_IN:
        status = slNewAttribute(Compiler,
                                Name->lineNo,
                                Name->stringNo,
                                Symbol,
                                binaryDataType,
                                logicalRegCount,
                                Name->context.useAsTextureCoord,
                                &attribute);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }

        for (i = 0; i < logicalRegCount; i++)
        {
            slsLOGICAL_REG_InitializeAttribute(LogicalRegs + *Start + i,
                                               qualifier,
                                               binaryDataType,
                                               attribute,
                                               (gctREG_INDEX)(i * binaryDataTypeSize));
        }
        break;

    case slvQUALIFIER_VARYING_OUT:
    case slvQUALIFIER_INVARIANT_VARYING_OUT:
    case slvQUALIFIER_FRAGMENT_OUT:
        tempRegIndex = slNewTempRegs(Compiler, logicalRegCount * binaryDataTypeSize);

        for (i = 0; i < logicalRegCount; i++)
        {
            slsLOGICAL_REG_InitializeTemp(LogicalRegs + *Start + i,
                                          qualifier,
                                          binaryDataType,
                                          tempRegIndex + (gctREG_INDEX)(i * binaryDataTypeSize));
        }

        status = slNewOutput(Compiler,
                             Name->lineNo,
                             Name->stringNo,
                             Symbol,
                             binaryDataType,
                             logicalRegCount,
                             tempRegIndex);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }

        if (qualifier == slvQUALIFIER_VARYING_OUT)
        {
            status = slNewVariable(Compiler,
                                   Name->lineNo,
                                   Name->stringNo,
                                   Symbol,
                                   binaryDataType,
                                   logicalRegCount,
                                   tempRegIndex,
                                   gcSHADER_VAR_CATEGORY_NORMAL,
                                   0,
                                   parent,
                                   prevSibling,
                                   ThisVarIndex);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }

        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    (*Start) += logicalRegCount;

    if (FirstRegIndex)
        *FirstRegIndex = tempRegIndex;

    gcmFOOTER_ARG("*LogicalRegs=0x%x *Start=%u", *LogicalRegs, *Start);
    return gcvSTATUS_OK;
}

static gceSTATUS _AllocStructElementAggregatedSymbol(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsDATA_TYPE * structDataType,
    IN gctUINT arrayIdx,
    IN gctCONST_STRING structSymbol,
    IN gctCONST_STRING fieldSymbol, /* this can be NULL */
    OUT gctSTRING *Symbol
    )
{
    gceSTATUS       status;
    gctSIZE_T       symbolLength = 0, fieldLength = 0, len;
    gctSTRING       symbol = gcvNULL;
    gctUINT         offset;
    gctPOINTER      pointer = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x codeGenerator=0x%x structDataType=0x%x arrayIdx=%d"
                  "structSymbol=%s fieldSymbol=%s Symbol=0x%x",
                  Compiler, CodeGenerator, structDataType, arrayIdx, structSymbol, fieldSymbol, Symbol);

    gcmVERIFY_OK(gcoOS_StrLen(structSymbol, &symbolLength));

    if (fieldSymbol)
        gcmVERIFY_OK(gcoOS_StrLen(fieldSymbol, &fieldLength));

    len = symbolLength + fieldLength + 20;

    status = sloCOMPILER_Allocate(Compiler,
                                  len,
                                  &pointer);

    if (gcmIS_ERROR(status))
    {
        if (Symbol) *Symbol = gcvNULL;

        gcmFOOTER();
        return status;
    }

    symbol = pointer;

    if (structDataType->arrayLength == 0)
    {
        offset = 0;
        if (fieldSymbol)
            gcmVERIFY_OK(gcoOS_PrintStrSafe(symbol, len,
                                            &offset,
                                            "%s.%s",
                                            structSymbol,
                                            fieldSymbol));
        else
            gcmVERIFY_OK(gcoOS_PrintStrSafe(symbol, len,
                                            &offset,
                                            "%s",
                                            structSymbol));

    }
    else
    {
        offset = 0;
        if (fieldSymbol)
            gcmVERIFY_OK(gcoOS_PrintStrSafe(symbol, len,
                                            &offset,
                                            "%s[%d].%s",
                                            structSymbol,
                                            arrayIdx,
                                            fieldSymbol));
        else
            gcmVERIFY_OK(gcoOS_PrintStrSafe(symbol, len,
                                            &offset,
                                            "%s[%d]",
                                            structSymbol,
                                            arrayIdx));
    }

    if (Symbol)
        *Symbol = symbol;

    gcmFOOTER_ARG("*Symbol=%s", *Symbol);
    return gcvSTATUS_OK;
}

static gceSTATUS _FreeStructElementAggregatedSymbol(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN gctSTRING symbol
    )
{
    gceSTATUS       status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x codeGenerator=0x%x symbol=%s",
                  Compiler, CodeGenerator, symbol);

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, symbol));

    gcmFOOTER();
    return status;
}

#if SUPPORT_STRUCT_ELEMENT_IN_VARIABLE
static gceSTATUS
_NewStructElementSymbol(
	IN sloCOMPILER Compiler,
	IN slsNAME * Name,
	IN gctCONST_STRING Symbol,
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
	gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    if (Name->dataType->qualifier == slvQUALIFIER_UNIFORM)
    {
    }
    else
    {
	    status = slNewVariable(Compiler,
                               Name->lineNo,
                               Name->stringNo,
	                           Symbol,
	                           DataType,
	                           Length,
	                           TempRegIndex,
                               varCategory,
                               numStructureElement,
                               parent,
                               prevSibling,
                               ThisVarIndex);
    }

    gcmFOOTER_NO();
	return status;
}

static gceSTATUS
_UpdateVariableTempReg(
	IN sloCOMPILER Compiler,
    IN slsNAME * Name,
	IN gctUINT varIndex,
	IN gctREG_INDEX newTempRegIndex
	)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    if (Name->dataType->qualifier == slvQUALIFIER_UNIFORM)
    {
    }
    else
    {
        status = slUpdateVariableTempReg(Compiler,
                                         Name->lineNo,
                                         Name->stringNo,
                                         varIndex,
                                         newTempRegIndex);
    }

    gcmFOOTER_NO();
	return status;
}
#endif

static gceSTATUS
_AllocLogicalRegs(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsNAME * Name,
    IN gctCONST_STRING Symbol,
    IN slsDATA_TYPE * DataType,
    IN gctINT16 parent,
    IN gctINT16 prevSibling,
    OUT gctINT16* ThisVarIndex,
    OUT gctINT16* FirstRegIndex,
    IN OUT slsLOGICAL_REG * LogicalRegs,
    IN OUT gctUINT * Start
    )
{
    gceSTATUS       status;
    gctUINT         count, i;
    slsNAME *       fieldName;
    gctSTRING       symbol = gcvNULL;
    gctINT16        mainIdx = -1, structEleParent;
#if SUPPORT_STRUCT_ELEMENT_IN_VARIABLE
    gctINT16        arrayElePrevSibling;
#endif
    gctINT16        structElePrevSibling;
    gctINT          regOfFirstArrayEle, regOfFirststructEle;
    gctINT16        regOfFirstMain, firstReg;
    gctUINT16       structEleCount;

    gcmHEADER_ARG("Compiler=0x%x codeGenerator=0x%x Name=0x%x Symbol=%s "
                  "DataType=0x%x",
                  Compiler, CodeGenerator, Name, Symbol, DataType);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmVERIFY_ARGUMENT(Name);
    gcmASSERT(DataType);
    gcmASSERT(LogicalRegs);
    gcmASSERT(Start);

    if (DataType->elementType == slvTYPE_STRUCT)
    {
        gcmASSERT(Name->dataType->fieldSpace);

        count = (DataType->arrayLength > 0) ? DataType->arrayLength : 1;
        slsDLINK_NODE_COUNT(&DataType->fieldSpace->names, structEleCount)

#if SUPPORT_STRUCT_ELEMENT_IN_VARIABLE
        status = _NewStructElementSymbol(Compiler,
                                         Name,
                                         Symbol,
                                         gcSHADER_FLOAT_X1, /* It is dummy for struct */
                                         count,
                                         (gctREG_INDEX)-1, /* Will resolve it when trasersing back */
                                         gcSHADER_VAR_CATEGORY_STRUCT,
                                         (count > 1) ? 0 : structEleCount,
                                         parent,
                                         prevSibling,
                                         &mainIdx);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }

        arrayElePrevSibling = -1;
#endif

        regOfFirstArrayEle = -1;
        for (i = 0; i < count; i++)
        {
            structEleParent = mainIdx;

#if SUPPORT_STRUCT_ELEMENT_IN_VARIABLE
            if (count > 1)
            {
                gcmVERIFY_OK(_AllocStructElementAggregatedSymbol(Compiler,
                                                           CodeGenerator,
                                                           DataType,
                                                           i,
                                                           Symbol,
                                                           gcvNULL,
                                                           &symbol));

                status = _NewStructElementSymbol(Compiler,
                                                 Name,
                                                 symbol,
                                                 gcSHADER_FLOAT_X1, /* It is dummy for struct */
                                                 1,
                                                 (gctREG_INDEX)-1, /* Will resolve it when trasersing back */
                                                 gcSHADER_VAR_CATEGORY_STRUCT,
                                                 structEleCount,
                                                 mainIdx,
                                                 arrayElePrevSibling,
                                                 &arrayElePrevSibling);

                if (gcmIS_ERROR(status))
                {
                    gcmFOOTER();
                    return status;
                }

                structEleParent = arrayElePrevSibling;

                gcmVERIFY_OK(_FreeStructElementAggregatedSymbol(Compiler, CodeGenerator, symbol));
            }
#endif

            structElePrevSibling = -1;
            regOfFirststructEle = -1;
            FOR_EACH_DLINK_NODE(&DataType->fieldSpace->names, slsNAME, fieldName)
            {
                gcmASSERT(fieldName->dataType);
                gcmVERIFY_OK(_AllocStructElementAggregatedSymbol(Compiler,
                                                           CodeGenerator,
                                                           DataType,
                                                           i,
                                                           Symbol,
                                                           fieldName->symbol,
                                                           &symbol));

                status = _AllocLogicalRegs(Compiler,
                                           CodeGenerator,
                                           Name,
                                           symbol,
                                           fieldName->dataType,
                                           structEleParent,
                                           structElePrevSibling,
                                           &structElePrevSibling,
                                           &firstReg,
                                           LogicalRegs,
                                           Start);

                if (gcmIS_ERROR(status))
                {
                    gcmFOOTER();
                    return status;
                }

                gcmVERIFY_OK(_FreeStructElementAggregatedSymbol(Compiler, CodeGenerator, symbol));

                if (regOfFirststructEle == -1)
                    regOfFirststructEle = firstReg;
            }

#if SUPPORT_STRUCT_ELEMENT_IN_VARIABLE
            gcmASSERT(regOfFirststructEle != -1);

            status = _UpdateVariableTempReg(Compiler,
                                            Name,
                                            structEleParent,
                                            (gctREG_INDEX)regOfFirststructEle);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
#endif

            if (regOfFirstArrayEle == -1)
                regOfFirstArrayEle = regOfFirststructEle;
        }

        regOfFirstMain = (gctUINT16)regOfFirstArrayEle;

#if SUPPORT_STRUCT_ELEMENT_IN_VARIABLE
        if (count > 1)
        {
            gcmASSERT(regOfFirstArrayEle != -1);

            status = _UpdateVariableTempReg(Compiler,
                                            Name,
                                            mainIdx,
                                            (gctREG_INDEX)regOfFirstArrayEle);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }
#endif
    }
    else
    {
        status = _AllocLogicalRegOrArray(Compiler,
                                         CodeGenerator,
                                         Name,
                                         Symbol,
                                         DataType,
                                         parent,
                                         prevSibling,
                                         &mainIdx,
                                         &regOfFirstMain,
                                         LogicalRegs,
                                         Start);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }
    }

    if (ThisVarIndex)
        *ThisVarIndex = mainIdx;

    if (FirstRegIndex)
        *FirstRegIndex = regOfFirstMain;

    gcmFOOTER_ARG("*LogicalRegs=0x%x *Start=%u", *LogicalRegs, *Start);
    return gcvSTATUS_OK;
}

gceSTATUS
slsNAME_CloneContext(
    IN sloCOMPILER Compiler,
    IN slsNAME * ActualParamName,
    IN slsNAME * FormalParamName
    )
{
    gceSTATUS   status;
    gctUINT     i;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x ActualParamName=0x%x FormalParamName=0x%x",
                  Compiler, ActualParamName, FormalParamName);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(ActualParamName);
    gcmVERIFY_ARGUMENT(FormalParamName);

    ActualParamName->context    = FormalParamName->context;

    status = sloCOMPILER_Allocate(
                                Compiler,
                                (gctSIZE_T)sizeof(slsLOGICAL_REG)
                                    * FormalParamName->context.logicalRegCount,
                                &pointer);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    ActualParamName->context.logicalRegs = pointer;

    for (i = 0; i < FormalParamName->context.logicalRegCount; i++)
    {
        ActualParamName->context.logicalRegs[i] = FormalParamName->context.logicalRegs[i];
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slsNAME_AllocLogicalRegs(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsNAME * Name
    )
{
    gceSTATUS           status;
    gctUINT             logicalRegCount;
    slsLOGICAL_REG *    logicalRegs = gcvNULL;
    gctUINT             start = 0;

    gcmHEADER_ARG("Compiler=0x%x CodeGenerator=0x%x Name=0x%x",
                  Compiler, CodeGenerator, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmVERIFY_ARGUMENT(Name);
    gcmASSERT(Name->dataType);
    gcmASSERT(Name->type == slvVARIABLE_NAME
                || Name->type == slvPARAMETER_NAME
                || Name->type == slvFUNC_NAME);

    if (Name->context.logicalRegCount != 0)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if (Name->type == slvPARAMETER_NAME && Name->u.parameterInfo.aliasName != gcvNULL)
    {
        status = slsNAME_CloneContext(Compiler,
                                      Name,
                                      Name->u.parameterInfo.aliasName);

        gcmFOOTER();
        return status;
    }

    logicalRegCount = _GetLogicalOperandCount(Name->dataType);
    gcmASSERT(logicalRegCount > 0);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsLOGICAL_REG) * logicalRegCount,
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        logicalRegs = pointer;

        status = _AllocLogicalRegs(
                                Compiler,
                                CodeGenerator,
                                Name,
                                Name->symbol,
                                Name->dataType,
                                -1,/* Assume it is not structure */
                                -1,/* Assume it is not structure */
                                gcvNULL,/* Do not need var index on the most top level */
                                gcvNULL,
                                logicalRegs,
                                &start);

        if (gcmIS_ERROR(status)) break;

        gcmASSERT(start == logicalRegCount);

        Name->context.logicalRegCount   = logicalRegCount;
        Name->context.logicalRegs       = logicalRegs;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (logicalRegs != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Free(Compiler, logicalRegs));
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
slsNAME_FreeLogicalRegs(
    IN sloCOMPILER Compiler,
    IN slsNAME * Name
    )
{
    gcmHEADER_ARG("Compiler=0x%x Name=0x%x", Compiler, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(Name);

    if (Name->context.logicalRegs != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Free(Compiler, Name->context.logicalRegs));
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctUINT
_GetLogicalArgCount(
    IN slsDATA_TYPE *DataType
    )
{
    gctUINT     count = 0;
    slsNAME *fieldName;

    gcmASSERT(DataType);

    if (DataType->elementType == slvTYPE_STRUCT) {
        gcmASSERT(DataType->fieldSpace);

        FOR_EACH_DLINK_NODE(&DataType->fieldSpace->names, slsNAME, fieldName) {
            gcmASSERT(fieldName->dataType);
            count += _GetLogicalArgCount(fieldName->dataType);
        }
    }
    else {
        count = slmDATA_TYPE_matrixSize_GET(DataType) ? slmDATA_TYPE_matrixSize_GET(DataType) : 1;
    }

    if (DataType->arrayLength > 0) {
        count *= DataType->arrayLength;
    }

    return count;
}

static gceSTATUS
_AllocateFuncResources(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsNAME * FuncName
    )
{
    gceSTATUS   status;
    slsNAME *paramName;
    gctUINT argCount;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(FuncName);

    if (FuncName->context.function != gcvNULL) { gcmFOOTER_NO(); return gcvSTATUS_OK; }

    status = slNewFunction(Compiler,
                           FuncName->lineNo,
                           FuncName->stringNo,
                           FuncName->symbol,
                           &FuncName->context.function);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    argCount = 0;
    /* count arguments for all parameters */
    FOR_EACH_DLINK_NODE(&FuncName->u.funcInfo.localSpace->names, slsNAME, paramName) {
       argCount += _GetLogicalArgCount(paramName->dataType);
    }

    /* Count arguments for return value */
    if (!slsDATA_TYPE_IsVoid(FuncName->dataType)) {
       argCount += _GetLogicalArgCount(FuncName->dataType);
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(Compiler,
                                  slvDUMP_CODE_GENERATOR,
                                  "<FUNCTION ARGUMENT COUNT: function name = \"%s\" "
                  "argument count = \"%d\" />",
                  FuncName->symbol,
                  argCount));

    status = gcFUNCTION_ReallocateArguments(FuncName->context.function, argCount);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Allocate registers for all parameters */
    FOR_EACH_DLINK_NODE(&FuncName->u.funcInfo.localSpace->names, slsNAME, paramName)
    {
        if (paramName->type != slvPARAMETER_NAME) break;

        paramName->context.function = FuncName->context.function;

        status = slsNAME_AllocLogicalRegs(Compiler,
                                          CodeGenerator,
                                          paramName);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    /* Allocate registers for return value */
    if (!slsDATA_TYPE_IsVoid(FuncName->dataType))
    {
        /* Return registers are output */
        FuncName->dataType->qualifier = slvQUALIFIER_OUT;

        status = slsNAME_AllocLogicalRegs(Compiler,
                                          CodeGenerator,
                                          FuncName);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineFuncBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_SET FuncBody
    )
{
    gceSTATUS       status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(FuncBody, slvIR_SET);
    gcmASSERT(FuncBody->funcName);

    gcmVERIFY_OK(sloCOMPILER_Dump(Compiler,
                                  slvDUMP_CODE_GENERATOR,
                                  "<FUNC_DEF line=\"%d\" string=\"%d\" name=\"%s\">",
                                  FuncBody->base.lineNo,
                                  FuncBody->base.stringNo,
                                  FuncBody->funcName->symbol));

    if (gcmIS_SUCCESS(gcoOS_StrCmp(FuncBody->funcName->symbol, "main")))
    {
        if (gcmIS_ERROR(sloCOMPILER_MainDefined(Compiler)))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(Compiler,
                                            FuncBody->funcName->lineNo,
                                            FuncBody->funcName->stringNo,
                                            slvREPORT_ERROR,
                                            "'main' function redefined"));

            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        CodeGenerator->currentFuncDefContext.isMain         = gcvTRUE;
        CodeGenerator->currentFuncDefContext.u.mainEndLabel = slNewLabel(Compiler);

        status = slBeginMainFunction(Compiler,
                                     FuncBody->base.lineNo,
                                     FuncBody->base.stringNo);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        CodeGenerator->currentFuncDefContext.isMain         = gcvFALSE;
        CodeGenerator->currentFuncDefContext.u.funcBody     = FuncBody;

        status = _AllocateFuncResources(Compiler,
                                        CodeGenerator,
                                        FuncBody->funcName);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = slBeginFunction(Compiler,
                                 FuncBody->base.lineNo,
                                 FuncBody->base.stringNo,
                                 FuncBody->funcName->context.function);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineFuncEnd(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_SET FuncBody,
    IN gctBOOL HasReturn
    )
{
    gceSTATUS           status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(FuncBody, slvIR_SET);
    gcmASSERT(FuncBody->funcName);

    if (CodeGenerator->currentFuncDefContext.isMain)
    {
        status = slSetLabel(
                            Compiler,
                            0,
                            0,
                            CodeGenerator->currentFuncDefContext.u.mainEndLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = slEndMainFunction(Compiler);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        gcmASSERT(CodeGenerator->currentFuncDefContext.u.funcBody == FuncBody);

        if (!HasReturn)
        {
            if (!slsDATA_TYPE_IsVoid(FuncBody->funcName->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                FuncBody->base.lineNo,
                                                FuncBody->base.stringNo,
                                                slvREPORT_WARN,
                                                "non-void function: '%s' must return a value",
                                                FuncBody->funcName->symbol));
            }

            status = slEmitAlwaysBranchCode(
                                            Compiler,
                                            FuncBody->base.lineNo,
                                            FuncBody->base.stringNo,
                                            slvOPCODE_RETURN,
                                            0);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }

        status = slEndFunction(Compiler, FuncBody->funcName->context.function);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "</FUNC_DEF>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
slsLOGICAL_REG_Dump(
    IN sloCOMPILER Compiler,
    IN slsLOGICAL_REG * LogicalReg
    )
{
    gctCONST_STRING     name;
    gctUINT8            i, component;
    const gctCHAR       componentNames[4] = {'x', 'y', 'z', 'w'};

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LogicalReg);

    switch (LogicalReg->qualifier)
    {
    case slvQUALIFIER_UNIFORM:
        name = gcGetUniformName(LogicalReg->u.uniform);
        break;

    case slvQUALIFIER_ATTRIBUTE:
        name = gcGetAttributeName(LogicalReg->u.attribute);
        break;

    default:
        name = "";
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "<LOGICAL_REG qualifier=\"%s\" dataType=\"%s\""
                                " name=\"%s\" regIndex=\"%d\" componentSelection=\"",
                                slGetQualifierName(LogicalReg->qualifier),
                                gcGetDataTypeName(LogicalReg->dataType),
                                name,
                                LogicalReg->regIndex));

    for (i = 0; i < LogicalReg->componentSelection.components; i++)
    {
        switch (i)
        {
        case 0: component = LogicalReg->componentSelection.x; break;
        case 1: component = LogicalReg->componentSelection.y; break;
        case 2: component = LogicalReg->componentSelection.z; break;
        case 3: component = LogicalReg->componentSelection.w; break;

        default:
            gcmASSERT(0);
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_CODE_GENERATOR,
                                    "%c",
                                    componentNames[component]));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "\" />"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DumpIndex(
    IN sloCOMPILER Compiler,
    IN gctCONST_STRING Name,
    IN slsINDEX * Index
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Name);
    gcmASSERT(Index);

    switch (Index->mode)
    {
    case slvINDEX_NONE:
        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvINDEX_REG:
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_CODE_GENERATOR,
                                    "<%s_REG_INDEX indexRegIndex=\"%d\" />",
                                    Name,
                                    Index->u.indexRegIndex));

        break;

    case slvINDEX_CONSTANT:
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_CODE_GENERATOR,
                                    "<%s_CONSTANT_INDEX index=\"%d\" />",
                                    Name,
                                    Index->u.constant));
        break;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
slsLOPERAND_Dump(
    IN sloCOMPILER Compiler,
    IN slsLOPERAND * LOperand
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LOperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "<LOPERAND dataType=\"%s\">",
                                gcGetDataTypeName(LOperand->dataType)));

    gcmVERIFY_OK(slsLOGICAL_REG_Dump(Compiler, &LOperand->reg));

    gcmVERIFY_OK(_DumpIndex(Compiler, "ARRAY", &LOperand->arrayIndex));

    gcmVERIFY_OK(_DumpIndex(Compiler, "MATRIX", &LOperand->matrixIndex));

    gcmVERIFY_OK(_DumpIndex(Compiler, "VECTOR", &LOperand->vectorIndex));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "</LOPERAND>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
slsROPERAND_Dump(
    IN sloCOMPILER Compiler,
    IN slsROPERAND * ROperand
    )
{
    gctUINT     i;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "<ROPERAND dataType=\"%s\">",
                                gcGetDataTypeName(ROperand->dataType)));

    if (ROperand->isReg)
    {
        gcmVERIFY_OK(slsLOGICAL_REG_Dump(Compiler, &ROperand->u.reg));
    }
    else
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_CODE_GENERATOR,
                                    "<CONSTANT dataType=\"%s\" valueCount=\"%d\">",
                                    gcGetDataTypeName(ROperand->u.constant.dataType),
                                    ROperand->u.constant.valueCount));

        for (i = 0; i < ROperand->u.constant.valueCount; i++)
        {
            gcmVERIFY_OK(sloCOMPILER_Dump(
                                        Compiler,
                                        slvDUMP_CODE_GENERATOR,
                                        "<VALUE bool=\"%s\" int=\"%d\" float=\"%f\" />",
                                        (ROperand->u.constant.values[i].boolValue) ?
                                            "true" : "false",
                                        ROperand->u.constant.values[i].intValue,
                                        ROperand->u.constant.values[i].floatValue));
        }

        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "</CONSTANT>"));

    }

    gcmVERIFY_OK(_DumpIndex(Compiler, "ARRAY", &ROperand->arrayIndex));

    gcmVERIFY_OK(_DumpIndex(Compiler, "MATRIX", &ROperand->matrixIndex));

    gcmVERIFY_OK(_DumpIndex(Compiler, "VECTOR", &ROperand->vectorIndex));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "</ROPERAND>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
slsIOPERAND_Dump(
    IN sloCOMPILER Compiler,
    IN slsIOPERAND * IOperand
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IOperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "<IOPERAND dataType=\"%s\" tempRegIndex=\"%d\" />",
                                gcGetDataTypeName(IOperand->dataType),
                                IOperand->tempRegIndex));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static slsCOMPONENT_SELECTION
_ReverseComponentSelection(
    IN slsCOMPONENT_SELECTION Source
    )
{
    slsCOMPONENT_SELECTION  result;

    result = ComponentSelection_XYZW;

    switch (Source.x)
    {
    case slvCOMPONENT_X: result.x = slvCOMPONENT_X; break;
    case slvCOMPONENT_Y: result.y = slvCOMPONENT_X; break;
    case slvCOMPONENT_Z: result.z = slvCOMPONENT_X; break;
    case slvCOMPONENT_W: result.w = slvCOMPONENT_X; break;

    default: gcmASSERT(0);
    }

    if (Source.components >= 2)
    {
        switch (Source.y)
        {
        case slvCOMPONENT_X: result.x = slvCOMPONENT_Y; break;
        case slvCOMPONENT_Y: result.y = slvCOMPONENT_Y; break;
        case slvCOMPONENT_Z: result.z = slvCOMPONENT_Y; break;
        case slvCOMPONENT_W: result.w = slvCOMPONENT_Y; break;

        default: gcmASSERT(0);
        }
    }

    if (Source.components >= 3)
    {
        switch (Source.z)
        {
        case slvCOMPONENT_X: result.x = slvCOMPONENT_Z; break;
        case slvCOMPONENT_Y: result.y = slvCOMPONENT_Z; break;
        case slvCOMPONENT_Z: result.z = slvCOMPONENT_Z; break;
        case slvCOMPONENT_W: result.w = slvCOMPONENT_Z; break;

        default: gcmASSERT(0);
        }
    }

    if (Source.components == 4)
    {
        switch (Source.w)
        {
        case slvCOMPONENT_X: result.x = slvCOMPONENT_W; break;
        case slvCOMPONENT_Y: result.y = slvCOMPONENT_W; break;
        case slvCOMPONENT_Z: result.z = slvCOMPONENT_W; break;
        case slvCOMPONENT_W: result.w = slvCOMPONENT_W; break;

        default: gcmASSERT(0);
        }
    }

    return result;
}

static slsCOMPONENT_SELECTION
_SwizzleComponentSelection(
    IN slsCOMPONENT_SELECTION Source1,
    IN slsCOMPONENT_SELECTION Source2
    )
{
    slsCOMPONENT_SELECTION  result = { 0 };

    result.components = Source1.components;

    switch (Source2.components)
    {
    case 1: Source2.w = Source2.z = Source2.y = Source2.x; break;
    case 2: Source2.w = Source2.z = Source2.y; break;
    case 3: Source2.w = Source2.z; break;
    case 4: break;

    default: gcmASSERT(0);
    }

    switch (Source1.x)
    {
    case slvCOMPONENT_X: result.x = Source2.x; break;
    case slvCOMPONENT_Y: result.x = Source2.y; break;
    case slvCOMPONENT_Z: result.x = Source2.z; break;
    case slvCOMPONENT_W: result.x = Source2.w; break;

    default: gcmASSERT(0);
    }

    if (Source1.components >= 2)
    {
        switch (Source1.y)
        {
        case slvCOMPONENT_X: result.y = Source2.x; break;
        case slvCOMPONENT_Y: result.y = Source2.y; break;
        case slvCOMPONENT_Z: result.y = Source2.z; break;
        case slvCOMPONENT_W: result.y = Source2.w; break;

        default: gcmASSERT(0);
        }
    }

    if (Source1.components >= 3)
    {
        switch (Source1.z)
        {
        case slvCOMPONENT_X: result.z = Source2.x; break;
        case slvCOMPONENT_Y: result.z = Source2.y; break;
        case slvCOMPONENT_Z: result.z = Source2.z; break;
        case slvCOMPONENT_W: result.z = Source2.w; break;

        default: gcmASSERT(0);
        }
    }

    if (Source1.components == 4)
    {
        switch (Source1.w)
        {
        case slvCOMPONENT_X: result.w = Source2.x; break;
        case slvCOMPONENT_Y: result.w = Source2.y; break;
        case slvCOMPONENT_Z: result.w = Source2.z; break;
        case slvCOMPONENT_W: result.w = Source2.w; break;

        default: gcmASSERT(0);
        }
    }

    return result;
}

static slsCOMPONENT_SELECTION
_ConvVectorIndexToComponentSelection(
    IN gctUINT VectorIndex
    )
{
    switch (VectorIndex)
    {
    case 0: return ComponentSelection_X;
    case 1: return ComponentSelection_Y;
    case 2: return ComponentSelection_Z;
    case 3: return ComponentSelection_W;

    default:
        gcmASSERT(0);
        return ComponentSelection_X;
    }
}

static gctUINT8
_ConvComponentToEnable(
    IN gctUINT8 Component
    )
{
    switch (Component)
    {
    case slvCOMPONENT_X: return gcSL_ENABLE_X;
    case slvCOMPONENT_Y: return gcSL_ENABLE_Y;
    case slvCOMPONENT_Z: return gcSL_ENABLE_Z;
    case slvCOMPONENT_W: return gcSL_ENABLE_W;

    default:
        gcmASSERT(0);
        return gcSL_ENABLE_X;
    }
}

static gctUINT8
_ConvComponentSelectionToEnable(
    IN slsCOMPONENT_SELECTION ComponentSelection
    )
{
    gctUINT8    enable;

    enable = _ConvComponentToEnable(ComponentSelection.x);

    if (ComponentSelection.components >= 2)
    {
        enable |= _ConvComponentToEnable(ComponentSelection.y);
    }

    if (ComponentSelection.components >= 3)
    {
        enable |= _ConvComponentToEnable(ComponentSelection.z);
    }

    if (ComponentSelection.components == 4)
    {
        enable |= _ConvComponentToEnable(ComponentSelection.w);
    }

    return enable;
}

static gceSTATUS
_ConvLOperandToTarget(
    IN sloCOMPILER Compiler,
    IN slsLOPERAND * LOperand,
    OUT gcsTARGET * Target,
    OUT slsCOMPONENT_SELECTION * ReversedComponentSelection
    )
{
    gctREG_INDEX            tempRegIndex;
    gctUINT8                enable = 0;
    gcSL_INDEXED            indexMode;
    gctREG_INDEX            indexRegIndex;
    slsCOMPONENT_SELECTION  reversedComponentSelection = { 0 };

    gcmHEADER();

    reversedComponentSelection.components = 0;

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LOperand);
    gcmASSERT(!gcIsMatrixDataType(LOperand->dataType));
    gcmASSERT(Target);
    gcmASSERT(ReversedComponentSelection);

    tempRegIndex    = LOperand->reg.regIndex;

    switch (LOperand->arrayIndex.mode)
    {
    case slvINDEX_NONE:
        indexMode       = gcSL_NOT_INDEXED;
        indexRegIndex   = 0;
        break;

    case slvINDEX_REG:
        gcmASSERT(LOperand->matrixIndex.mode != slvINDEX_REG);

        indexMode       = gcSL_INDEXED_X;
        indexRegIndex   = LOperand->arrayIndex.u.indexRegIndex;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (gcIsSamplerDataType(LOperand->dataType))
    {
        gcmASSERT(gcIsSamplerDataType(LOperand->reg.dataType));

        enable                      = gcGetDefaultEnable(LOperand->dataType);
        reversedComponentSelection  = slGetDefaultComponentSelection(LOperand->dataType);
    }
    else if (gcIsScalarDataType(LOperand->dataType))
    {
        if (gcIsScalarDataType(LOperand->reg.dataType))
        {
            enable                      = gcGetDefaultEnable(LOperand->dataType);
            reversedComponentSelection  = slGetDefaultComponentSelection(LOperand->dataType);
        }
        else
        {
            gcmASSERT(gcIsVectorDataType(LOperand->reg.dataType)
                        || gcIsMatrixDataType(LOperand->reg.dataType));

            switch (LOperand->vectorIndex.mode)
            {
            case slvINDEX_CONSTANT:
                reversedComponentSelection = _ConvVectorIndexToComponentSelection(
                                                        LOperand->vectorIndex.u.indexRegIndex);

                reversedComponentSelection = _SwizzleComponentSelection(
                                                        reversedComponentSelection,
                                                        LOperand->reg.componentSelection);

                enable = _ConvComponentSelectionToEnable(reversedComponentSelection);

                reversedComponentSelection = _ReverseComponentSelection(reversedComponentSelection);
                break;

            default:
                gcmASSERT(0);
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
                return gcvSTATUS_INVALID_ARGUMENT;
        }

            if (gcIsMatrixDataType(LOperand->reg.dataType))
            {
                switch (LOperand->matrixIndex.mode)
                {
                case slvINDEX_CONSTANT:
                    tempRegIndex += LOperand->matrixIndex.u.constant;
                    break;

                case slvINDEX_REG:
                    gcmASSERT(LOperand->arrayIndex.mode != slvINDEX_REG);

                    indexMode       = gcSL_INDEXED_X;
                    indexRegIndex   = LOperand->matrixIndex.u.indexRegIndex;
                    break;

                default:
                    gcmASSERT(0);
                    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
                    return gcvSTATUS_INVALID_ARGUMENT;
                }
            }
        }
    }
    else if (gcIsVectorDataType(LOperand->dataType))
    {
        gcmASSERT(gcIsVectorDataType(LOperand->reg.dataType)
                    || gcIsMatrixDataType(LOperand->reg.dataType));

        enable = _ConvComponentSelectionToEnable(LOperand->reg.componentSelection);

        reversedComponentSelection = _ReverseComponentSelection(LOperand->reg.componentSelection);

        if (gcIsMatrixDataType(LOperand->reg.dataType))
        {
            switch (LOperand->matrixIndex.mode)
            {
            case slvINDEX_CONSTANT:
                tempRegIndex += LOperand->matrixIndex.u.constant;
                break;

            case slvINDEX_REG:
                gcmASSERT(LOperand->arrayIndex.mode != slvINDEX_REG);

                indexMode       = gcSL_INDEXED_X;
                indexRegIndex   = LOperand->matrixIndex.u.indexRegIndex;
                break;

            default:
                gcmASSERT(0);
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }
    }
    else
    {
        gcmASSERT(0);
    }

    gcsTARGET_Initialize(
                        Target,
                        LOperand->dataType,
                        tempRegIndex,
                        enable,
                        indexMode,
                        indexRegIndex);

    *ReversedComponentSelection = reversedComponentSelection;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvLOperandToVectorComponentTarget(
    IN sloCOMPILER Compiler,
    IN slsLOPERAND * LOperand,
    IN gctUINT VectorIndex,
    OUT gcsTARGET * Target
    )
{
    gctREG_INDEX            tempRegIndex;
    gctUINT8                enable;
    gcSL_INDEXED            indexMode;
    gctREG_INDEX            indexRegIndex;
    slsCOMPONENT_SELECTION  componentSelection;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LOperand);
    gcmASSERT(gcIsVectorDataType(LOperand->dataType));
    gcmASSERT(Target);

    tempRegIndex    = LOperand->reg.regIndex;

    switch (LOperand->arrayIndex.mode)
    {
    case slvINDEX_NONE:
        indexMode       = gcSL_NOT_INDEXED;
        indexRegIndex   = 0;
        break;

    case slvINDEX_REG:
        gcmASSERT(LOperand->matrixIndex.mode != slvINDEX_REG);

        indexMode       = gcSL_INDEXED_X;
        indexRegIndex   = LOperand->arrayIndex.u.indexRegIndex;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmASSERT(gcIsVectorDataType(LOperand->reg.dataType)
                || gcIsMatrixDataType(LOperand->reg.dataType));

    componentSelection = _ConvVectorIndexToComponentSelection(VectorIndex);

    componentSelection = _SwizzleComponentSelection(
                                            componentSelection,
                                            LOperand->reg.componentSelection);

    enable = _ConvComponentSelectionToEnable(componentSelection);

    if (gcIsMatrixDataType(LOperand->reg.dataType))
    {
        switch (LOperand->matrixIndex.mode)
        {
        case slvINDEX_CONSTANT:
            tempRegIndex += LOperand->matrixIndex.u.constant;
            break;

        case slvINDEX_REG:
            gcmASSERT(LOperand->arrayIndex.mode != slvINDEX_REG);

            indexMode       = gcSL_INDEXED_X;
            indexRegIndex   = LOperand->matrixIndex.u.indexRegIndex;
            break;

        default:
            gcmASSERT(0);
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }
    }

    gcsTARGET_Initialize(
                        Target,
                        gcGetVectorComponentDataType(LOperand->dataType),
                        tempRegIndex,
                        enable,
                        indexMode,
                        indexRegIndex);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvLOperandToMatrixColumnTarget(
    IN sloCOMPILER Compiler,
    IN slsLOPERAND * LOperand,
    IN gctUINT MatrixIndex,
    OUT gcsTARGET * Target
    )
{
    gcSHADER_TYPE           dataType;
    gctREG_INDEX            tempRegIndex;
    gctUINT8                enable;
    gcSL_INDEXED            indexMode;
    gctREG_INDEX            indexRegIndex;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LOperand);
    gcmASSERT(gcIsMatrixDataType(LOperand->dataType));
    gcmASSERT(gcIsMatrixDataType(LOperand->reg.dataType));
    gcmASSERT(Target);

    dataType        = gcGetMatrixColumnDataType(LOperand->dataType);
    tempRegIndex    = (gctREG_INDEX) (LOperand->reg.regIndex + MatrixIndex);
    enable          = gcGetDefaultEnable(dataType);

    switch (LOperand->arrayIndex.mode)
    {
    case slvINDEX_NONE:
        indexMode       = gcSL_NOT_INDEXED;
        indexRegIndex   = 0;
        break;

    case slvINDEX_REG:
        gcmASSERT(LOperand->matrixIndex.mode != slvINDEX_REG);

        indexMode       = gcSL_INDEXED_X;
        indexRegIndex   = LOperand->arrayIndex.u.indexRegIndex;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcsTARGET_Initialize(
                        Target,
                        dataType,
                        tempRegIndex,
                        enable,
                        indexMode,
                        indexRegIndex);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvLOperandToMatrixComponentTarget(
    IN sloCOMPILER Compiler,
    IN slsLOPERAND * LOperand,
    IN gctUINT MatrixIndex,
    IN gctUINT VectorIndex,
    OUT gcsTARGET * Target
    )
{
    gcSHADER_TYPE           dataType;
    gctREG_INDEX            tempRegIndex;
    gctUINT8                enable;
    gcSL_INDEXED            indexMode;
    gctREG_INDEX            indexRegIndex;
    const gctUINT8          enableTable[4] =
                                {gcSL_ENABLE_X, gcSL_ENABLE_Y, gcSL_ENABLE_Z, gcSL_ENABLE_W};

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LOperand);
    gcmASSERT(gcIsMatrixDataType(LOperand->dataType));
    gcmASSERT(gcIsMatrixDataType(LOperand->reg.dataType));
    gcmASSERT(Target);

    dataType        = gcGetVectorComponentDataType(gcGetMatrixColumnDataType(LOperand->dataType));
    tempRegIndex    = (gctREG_INDEX) (LOperand->reg.regIndex + MatrixIndex);
    enable          = enableTable[VectorIndex];

    switch (LOperand->arrayIndex.mode)
    {
    case slvINDEX_NONE:
        indexMode       = gcSL_NOT_INDEXED;
        indexRegIndex   = 0;
        break;

    case slvINDEX_REG:
        gcmASSERT(LOperand->matrixIndex.mode != slvINDEX_REG);

        indexMode       = gcSL_INDEXED_X;
        indexRegIndex   = LOperand->arrayIndex.u.indexRegIndex;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcsTARGET_Initialize(
                        Target,
                        dataType,
                        tempRegIndex,
                        enable,
                        indexMode,
                        indexRegIndex);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctUINT8
_ConvComponentToSwizzle(
    IN gctUINT8 Component
    )
{
    switch (Component)
    {
    case slvCOMPONENT_X: return gcSL_SWIZZLE_X;
    case slvCOMPONENT_Y: return gcSL_SWIZZLE_Y;
    case slvCOMPONENT_Z: return gcSL_SWIZZLE_Z;
    case slvCOMPONENT_W: return gcSL_SWIZZLE_W;

    default:
        gcmASSERT(0);
        return gcSL_SWIZZLE_X;
    }
}

static gctUINT8
_ConvComponentSelectionToSwizzle(
    IN slsCOMPONENT_SELECTION ComponentSelection
    )
{
    gctUINT8 swizzle = _ConvComponentToSwizzle(ComponentSelection.x);

    swizzle |= (ComponentSelection.components >= 2)
            ?  ( _ConvComponentToSwizzle(ComponentSelection.y) << 2)
            :  ((swizzle & 0x03) << 2);

    swizzle |= (ComponentSelection.components >= 3)
            ?  (_ConvComponentToSwizzle(ComponentSelection.z) << 4)
            :  ((swizzle & 0x0C) << 2);

    swizzle |= (ComponentSelection.components >= 4)
            ?  (_ConvComponentToSwizzle(ComponentSelection.w) << 6)
            :  ((swizzle & 0x30) << 2);

    return swizzle;
}

static gceSOURCE_TYPE
_ConvQualifierToSourceType(
    IN sltQUALIFIER Qualifier
    )
{
    switch (Qualifier)
    {
    case slvQUALIFIER_NONE:
    case slvQUALIFIER_CONST_IN:
    case slvQUALIFIER_IN:
    case slvQUALIFIER_OUT:
    case slvQUALIFIER_INOUT:
    case slvQUALIFIER_VARYING_OUT:
    case slvQUALIFIER_INVARIANT_VARYING_OUT:
    case slvQUALIFIER_FRAGMENT_OUT:
        return gcvSOURCE_TEMP;

    case slvQUALIFIER_UNIFORM:
        return gcvSOURCE_UNIFORM;

    case slvQUALIFIER_ATTRIBUTE:
    case slvQUALIFIER_VARYING_IN:
    case slvQUALIFIER_INVARIANT_VARYING_IN:
        return gcvSOURCE_ATTRIBUTE;

    default:
        gcmASSERT(0);
        return gcvSOURCE_TEMP;
    }
}

static gceSTATUS
_ConvROperandToSourceReg(
    IN sloCOMPILER Compiler,
    IN slsROPERAND * ROperand,
    IN slsCOMPONENT_SELECTION ReversedComponentSelection,
    OUT gcsSOURCE * Source
    )
{
    gcsSOURCE_REG           sourceReg = { {0} };
    slsCOMPONENT_SELECTION  componentSelection;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);
    gcmASSERT(ROperand->isReg);
    gcmASSERT(!gcIsMatrixDataType(ROperand->dataType));
    gcmASSERT(Source);

    sourceReg.u.attribute   = ROperand->u.reg.u.attribute;
    sourceReg.regIndex      = ROperand->u.reg.regIndex;

    switch (ROperand->arrayIndex.mode)
    {
    case slvINDEX_NONE:
        sourceReg.indexMode     = gcSL_NOT_INDEXED;
        sourceReg.indexRegIndex = 0;
        break;

    case slvINDEX_REG:
        gcmASSERT(ROperand->matrixIndex.mode != slvINDEX_REG);

        sourceReg.indexMode     = gcSL_INDEXED_X;
        sourceReg.indexRegIndex = ROperand->arrayIndex.u.indexRegIndex;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (gcIsSamplerDataType(ROperand->dataType))
    {
        gcmASSERT(gcIsSamplerDataType(ROperand->u.reg.dataType));

        componentSelection = slGetDefaultComponentSelection(ROperand->dataType);

        componentSelection = _SwizzleComponentSelection(
                                                    ReversedComponentSelection,
                                                    componentSelection);

        sourceReg.swizzle = _ConvComponentSelectionToSwizzle(componentSelection);
    }
    else if (gcIsScalarDataType(ROperand->dataType))
    {
        if (gcIsScalarDataType(ROperand->u.reg.dataType))
        {
            componentSelection = slGetDefaultComponentSelection(ROperand->dataType);

            componentSelection = _SwizzleComponentSelection(
                                                        ReversedComponentSelection,
                                                        componentSelection);

            sourceReg.swizzle = _ConvComponentSelectionToSwizzle(componentSelection);
        }
        else
        {
            gcmASSERT(gcIsVectorDataType(ROperand->u.reg.dataType)
                        || gcIsMatrixDataType(ROperand->u.reg.dataType));

            switch (ROperand->vectorIndex.mode)
            {
            case slvINDEX_CONSTANT:
                componentSelection = _ConvVectorIndexToComponentSelection(
                                                ROperand->vectorIndex.u.indexRegIndex);

                componentSelection = _SwizzleComponentSelection(
                                                            componentSelection,
                                                            ROperand->u.reg.componentSelection);

                componentSelection = _SwizzleComponentSelection(
                                                            ReversedComponentSelection,
                                                            componentSelection);

                sourceReg.swizzle = _ConvComponentSelectionToSwizzle(componentSelection);
                break;

            default:
                gcmASSERT(0);
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
                return gcvSTATUS_INVALID_ARGUMENT;
            }

            if (gcIsMatrixDataType(ROperand->u.reg.dataType))
            {
                switch (ROperand->matrixIndex.mode)
                {
                case slvINDEX_CONSTANT:
                    sourceReg.regIndex += ROperand->matrixIndex.u.constant;
                    break;

                case slvINDEX_REG:
                    gcmASSERT(ROperand->arrayIndex.mode != slvINDEX_REG);

                    sourceReg.indexMode     = gcSL_INDEXED_X;
                    sourceReg.indexRegIndex = ROperand->matrixIndex.u.indexRegIndex;
                    break;

                default:
                    gcmASSERT(0);
                    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
                    return gcvSTATUS_INVALID_ARGUMENT;
                }
            }
        }
    }
    else if (gcIsVectorDataType(ROperand->dataType))
    {
        gcmASSERT(gcIsVectorDataType(ROperand->u.reg.dataType)
                    || gcIsMatrixDataType(ROperand->u.reg.dataType));

        componentSelection = _SwizzleComponentSelection(
                                                    ReversedComponentSelection,
                                                    ROperand->u.reg.componentSelection);

        sourceReg.swizzle = _ConvComponentSelectionToSwizzle(componentSelection);

        if (gcIsMatrixDataType(ROperand->u.reg.dataType))
        {
            switch (ROperand->matrixIndex.mode)
            {
            case slvINDEX_CONSTANT:
                sourceReg.regIndex += ROperand->matrixIndex.u.constant;
                break;

            case slvINDEX_REG:
                gcmASSERT(ROperand->arrayIndex.mode != slvINDEX_REG);

                sourceReg.indexMode     = gcSL_INDEXED_X;
                sourceReg.indexRegIndex = ROperand->matrixIndex.u.indexRegIndex;
                break;

            default:
                gcmASSERT(0);
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }
    }
    else
    {
        gcmASSERT(0);
    }

    gcsSOURCE_InitializeReg(
                            Source,
                            _ConvQualifierToSourceType(ROperand->u.reg.qualifier),
                            ROperand->dataType,
                            sourceReg);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvROperandToMatrixColumnSourceReg(
    IN sloCOMPILER Compiler,
    IN slsROPERAND * ROperand,
    IN gctUINT MatrixIndex,
    OUT gcsSOURCE * Source
    )
{
    gcsSOURCE_REG           sourceReg;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);
    gcmASSERT(ROperand->isReg);
    gcmASSERT(gcIsMatrixDataType(ROperand->dataType));
    gcmASSERT(gcIsMatrixDataType(ROperand->u.reg.dataType));
    gcmASSERT(Source);

    sourceReg.u.attribute   = ROperand->u.reg.u.attribute;
    sourceReg.regIndex      = (gctREG_INDEX) (ROperand->u.reg.regIndex + MatrixIndex);
    sourceReg.swizzle       = _ConvComponentSelectionToSwizzle(
                                            slGetDefaultComponentSelection(ROperand->dataType));

    switch (ROperand->arrayIndex.mode)
    {
    case slvINDEX_NONE:
        sourceReg.indexMode     = gcSL_NOT_INDEXED;
        sourceReg.indexRegIndex = 0;
        break;

    case slvINDEX_REG:
        gcmASSERT(ROperand->matrixIndex.mode != slvINDEX_REG);

        sourceReg.indexMode     = gcSL_INDEXED_X;
        sourceReg.indexRegIndex = ROperand->arrayIndex.u.indexRegIndex;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcsSOURCE_InitializeReg(
                            Source,
                            _ConvQualifierToSourceType(ROperand->u.reg.qualifier),
                            gcGetMatrixColumnDataType(ROperand->dataType),
                            sourceReg);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvROperandToSourceConstant(
    IN sloCOMPILER Compiler,
    IN slsROPERAND * ROperand,
    OUT gcsSOURCE * Source
    )
{
    gcsSOURCE_CONSTANT sourceConstant;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);
    gcmASSERT(!ROperand->isReg);
    gcmASSERT(gcIsScalarDataType(ROperand->dataType));
    gcmASSERT(Source);

    switch (ROperand->u.constant.dataType)
    {
    case gcSHADER_FLOAT_X1:
        sourceConstant.u.floatConstant  = ROperand->u.constant.values[0].floatValue;
        break;

    case gcSHADER_BOOLEAN_X1:
        sourceConstant.u.boolConstant   = ROperand->u.constant.values[0].boolValue;
        break;

    case gcSHADER_INTEGER_X1:
        sourceConstant.u.intConstant    = ROperand->u.constant.values[0].intValue;
        break;

    case gcSHADER_FLOAT_X2:
    case gcSHADER_FLOAT_X3:
    case gcSHADER_FLOAT_X4:
        gcmASSERT(ROperand->vectorIndex.mode == slvINDEX_CONSTANT);
        sourceConstant.u.floatConstant  =
                ROperand->u.constant.values[ROperand->vectorIndex.u.constant].floatValue;
        break;

    case gcSHADER_BOOLEAN_X2:
    case gcSHADER_BOOLEAN_X3:
    case gcSHADER_BOOLEAN_X4:
        gcmASSERT(ROperand->vectorIndex.mode == slvINDEX_CONSTANT);
        sourceConstant.u.boolConstant   =
                ROperand->u.constant.values[ROperand->vectorIndex.u.constant].boolValue;
        break;

    case gcSHADER_INTEGER_X2:
    case gcSHADER_INTEGER_X3:
    case gcSHADER_INTEGER_X4:
        gcmASSERT(ROperand->vectorIndex.mode == slvINDEX_CONSTANT);
        sourceConstant.u.intConstant    =
                ROperand->u.constant.values[ROperand->vectorIndex.u.constant].intValue;
        break;

    case gcSHADER_FLOAT_2X2:
    case gcSHADER_FLOAT_3X3:
    case gcSHADER_FLOAT_4X4:
        gcmASSERT(ROperand->matrixIndex.mode == slvINDEX_CONSTANT);
        gcmASSERT(ROperand->vectorIndex.mode == slvINDEX_CONSTANT);
        sourceConstant.u.floatConstant  =
                ROperand->u.constant.values[
                    ROperand->matrixIndex.u.constant
                        * gcGetMatrixDataTypeColumnCount(ROperand->u.constant.dataType)
                        + ROperand->vectorIndex.u.constant].floatValue;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcsSOURCE_InitializeConstant(Source, ROperand->dataType, sourceConstant);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvROperandToSpecialVectorSourceConstant(
    IN sloCOMPILER Compiler,
    IN slsROPERAND * ROperand,
    OUT gcsSOURCE * Source
    )
{
    gcsSOURCE_CONSTANT  sourceConstant;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);
    gcmASSERT(!ROperand->isReg);
    gcmASSERT(gcIsVectorDataType(ROperand->dataType));
    gcmASSERT(slsROPERAND_CONSTANT_IsAllVectorComponentsEqual(ROperand));
    gcmASSERT(Source);

    switch (gcGetVectorComponentDataType(ROperand->dataType))
    {
    case gcSHADER_FLOAT_X1:
        sourceConstant.u.floatConstant  = ROperand->u.constant.values[0].floatValue;
        break;

    case gcSHADER_BOOLEAN_X1:
        sourceConstant.u.boolConstant   = ROperand->u.constant.values[0].boolValue;
        break;

    case gcSHADER_INTEGER_X1:
        sourceConstant.u.intConstant    = ROperand->u.constant.values[0].intValue;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcsSOURCE_InitializeConstant(Source, ROperand->dataType, sourceConstant);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvROperandToVectorComponentSourceConstant(
    IN sloCOMPILER Compiler,
    IN slsROPERAND * ROperand,
    IN gctUINT VectorIndex,
    OUT gcsSOURCE * Source
    )
{
    gcSHADER_TYPE       dataType;
    gcsSOURCE_CONSTANT  sourceConstant;
    gctUINT             columnCount;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);
    gcmASSERT(!ROperand->isReg);
    gcmASSERT(gcIsVectorDataType(ROperand->dataType));
    gcmASSERT(ROperand->u.constant.valueCount > VectorIndex);
    gcmASSERT(Source);

    dataType = gcGetVectorComponentDataType(ROperand->dataType);

    if (gcIsMatrixDataType(ROperand->u.constant.dataType))
    {
        gcmASSERT(ROperand->dataType == gcGetMatrixColumnDataType(ROperand->u.constant.dataType));
        gcmASSERT(ROperand->matrixIndex.mode == slvINDEX_CONSTANT);

        columnCount = gcGetMatrixDataTypeColumnCount(ROperand->u.constant.dataType);

        switch (dataType)
        {
        case gcSHADER_FLOAT_X1:
            sourceConstant.u.floatConstant  = ROperand->u.constant.values[
                                                    ROperand->matrixIndex.u.constant * columnCount
                                                        + VectorIndex].floatValue;
            break;

        case gcSHADER_BOOLEAN_X1:
            sourceConstant.u.boolConstant   = ROperand->u.constant.values[
                                                    ROperand->matrixIndex.u.constant * columnCount
                                                        + VectorIndex].boolValue;
            break;

        case gcSHADER_INTEGER_X1:
            sourceConstant.u.intConstant    = ROperand->u.constant.values[
                                                    ROperand->matrixIndex.u.constant * columnCount
                                                        + VectorIndex].intValue;
            break;

        default:
            gcmASSERT(0);
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }
    }
    else
    {
        gcmASSERT(gcIsVectorDataType(ROperand->u.constant.dataType));

        switch (dataType)
        {
        case gcSHADER_FLOAT_X1:
            sourceConstant.u.floatConstant  = ROperand->u.constant.values[VectorIndex].floatValue;
            break;

        case gcSHADER_BOOLEAN_X1:
            sourceConstant.u.boolConstant   = ROperand->u.constant.values[VectorIndex].boolValue;
            break;

        case gcSHADER_INTEGER_X1:
            sourceConstant.u.intConstant    = ROperand->u.constant.values[VectorIndex].intValue;
            break;

        default:
            gcmASSERT(0);
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }
    }

    gcsSOURCE_InitializeConstant(Source, dataType, sourceConstant);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvROperandToMatrixComponentSourceConstant(
    IN sloCOMPILER Compiler,
    IN slsROPERAND * ROperand,
    IN gctUINT MatrixIndex,
    IN gctUINT VectorIndex,
    OUT gcsSOURCE * Source
    )
{
    gcSHADER_TYPE       dataType;
    gcsSOURCE_CONSTANT  sourceConstant;
    gctUINT             index;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);
    gcmASSERT(!ROperand->isReg);
    gcmASSERT(gcIsMatrixDataType(ROperand->dataType));
    gcmASSERT(ROperand->u.constant.valueCount > VectorIndex);
    gcmASSERT(Source);

    dataType    = gcGetVectorComponentDataType(gcGetMatrixColumnDataType(ROperand->dataType));
    index       = MatrixIndex * gcGetMatrixDataTypeColumnCount(ROperand->dataType) + VectorIndex;

    switch (dataType)
    {
    case gcSHADER_FLOAT_X1:
        sourceConstant.u.floatConstant  = ROperand->u.constant.values[index].floatValue;
        break;

    case gcSHADER_BOOLEAN_X1:
        sourceConstant.u.boolConstant   = ROperand->u.constant.values[index].boolValue;
        break;

    case gcSHADER_INTEGER_X1:
        sourceConstant.u.intConstant    = ROperand->u.constant.values[index].intValue;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcsSOURCE_InitializeConstant(Source, dataType, sourceConstant);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_SpecialGenAssignCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsLOPERAND * LOperand,
    IN slsROPERAND * ROperand
    )
{
    gceSTATUS               status = gcvSTATUS_OK;
    gcsTARGET               target;
    slsCOMPONENT_SELECTION  reversedComponentSelection;
    gcsSOURCE               source;
    gctUINT                 i, j;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LOperand);
    gcmASSERT(ROperand);
    gcmASSERT(LOperand->dataType == ROperand->dataType);

    if (gcIsScalarDataType(LOperand->dataType))
    {
        gcmONERROR(_ConvLOperandToTarget(
                                        Compiler,
                                        LOperand,
                                        &target,
                                        &reversedComponentSelection));

        if (ROperand->isReg)
        {
            gcmONERROR(_ConvROperandToSourceReg(
                                                Compiler,
                                                ROperand,
                                                reversedComponentSelection,
                                                &source));
        }
        else
        {
            gcmONERROR(_ConvROperandToSourceConstant(
                                                    Compiler,
                                                    ROperand,
                                                    &source));
        }

        gcmONERROR(slEmitAssignCode(
                                Compiler,
                                LineNo,
                                StringNo,
                                &target,
                                &source));
    }
    else if (gcIsVectorDataType(LOperand->dataType) || gcIsSamplerDataType(LOperand->dataType))
    {
        if (ROperand->isReg)
        {
            gcmONERROR(_ConvLOperandToTarget(
                                            Compiler,
                                            LOperand,
                                            &target,
                                            &reversedComponentSelection));

            gcmONERROR(_ConvROperandToSourceReg(
                                                Compiler,
                                                ROperand,
                                                reversedComponentSelection,
                                                &source));
            gcmONERROR(slEmitAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    &target,
                                    &source));
        }
        else
        {
            gcmASSERT(!gcIsSamplerDataType(LOperand->dataType));

            if (slsROPERAND_CONSTANT_IsAllVectorComponentsEqual(ROperand))
            {
                gcmONERROR(_ConvLOperandToTarget(
                                                Compiler,
                                                LOperand,
                                                &target,
                                                &reversedComponentSelection));

                gcmONERROR(_ConvROperandToSpecialVectorSourceConstant(
                                                                        Compiler,
                                                                        ROperand,
                                                                        &source));

                gcmONERROR(slEmitAssignCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            &target,
                                            &source));
            }
            else
            {
                for (i = 0; i < gcGetVectorDataTypeComponentCount(LOperand->dataType); i++)
                {
                    gcmONERROR(_ConvLOperandToVectorComponentTarget(
                                                                    Compiler,
                                                                    LOperand,
                                                                    i,
                                                                    &target));

                    gcmONERROR(_ConvROperandToVectorComponentSourceConstant(
                                                                            Compiler,
                                                                            ROperand,
                                                                            i,
                                                                            &source));

                    gcmONERROR(slEmitAssignCode(
                                                Compiler,
                                                LineNo,
                                                StringNo,
                                                &target,
                                                &source));
                }
            }
        }
    }
    else
    {
        gcmASSERT(gcIsMatrixDataType(LOperand->dataType));

        if (ROperand->isReg)
        {
            for (i = 0; i < gcGetMatrixDataTypeColumnCount(LOperand->dataType); i++)
            {
                gcmONERROR(_ConvLOperandToMatrixColumnTarget(
                                                            Compiler,
                                                            LOperand,
                                                            i,
                                                            &target));

                gcmONERROR(_ConvROperandToMatrixColumnSourceReg(
                                                                Compiler,
                                                                ROperand,
                                                                i,
                                                                &source));

                gcmONERROR(slEmitAssignCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            &target,
                                            &source));
            }
        }
        else
        {
            for (i = 0; i < gcGetMatrixDataTypeColumnCount(LOperand->dataType); i++)
            {
                for (j = 0; j < gcGetMatrixDataTypeRowCount(LOperand->dataType); j++)
                {
                    gcmONERROR(_ConvLOperandToMatrixComponentTarget(
                                                                    Compiler,
                                                                    LOperand,
                                                                    i,
                                                                    j,
                                                                    &target));

                    gcmONERROR(_ConvROperandToMatrixComponentSourceConstant(
                                                                            Compiler,
                                                                            ROperand,
                                                                            i,
                                                                            j,
                                                                            &source));


                    gcmONERROR(slEmitAssignCode(
                                                Compiler,
                                                LineNo,
                                                StringNo,
                                                &target,
                                                &source));
                }
            }
        }
    }
OnError:
    gcmFOOTER();
    return status;
}

static gceSTATUS
_GenIndexAddCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN gctREG_INDEX TargetIndexRegIndex,
    IN gctREG_INDEX SourceIndexRegIndex,
    IN slsROPERAND * ROperand
    );

gceSTATUS
slGenAssignSamplerCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsLOPERAND * LOperand,
    IN slsROPERAND * ROperand
    )
{
    gceSTATUS       status;
    slsLOPERAND     realLOperand;
    slsROPERAND     realROperand, baseROperand;
    gctREG_INDEX    samplerIndex;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LOperand);
    gcmASSERT(gcIsSamplerDataType(LOperand->dataType));
    gcmASSERT(ROperand);
    gcmASSERT(LOperand->dataType == ROperand->dataType);
    gcmASSERT(ROperand->isReg);

    realLOperand                = *LOperand;
    realLOperand.dataType       = realLOperand.reg.dataType = gcSHADER_INTEGER_X1;

    if (ROperand->u.reg.qualifier == slvQUALIFIER_UNIFORM)
    {
        status = slGetUniformSamplerIndex(
                                        Compiler,
                                        ROperand->u.reg.u.uniform,
                                        &samplerIndex);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        switch (ROperand->arrayIndex.mode)
        {
        case slvINDEX_NONE:
            slsROPERAND_InitializeIntOrIVecConstant(
                                                    &realROperand,
                                                    gcSHADER_INTEGER_X1,
                                                    (gctINT32)samplerIndex);

            status = slGenAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    &realLOperand,
                                    &realROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            break;

        case slvINDEX_CONSTANT:
            slsROPERAND_InitializeIntOrIVecConstant(
                                                    &realROperand,
                                                    gcSHADER_INTEGER_X1,
                                                    (gctINT32)(samplerIndex
                                                        + ROperand->arrayIndex.u.constant));

            status = slGenAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    &realLOperand,
                                    &realROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            break;

        case slvINDEX_REG:
            slsROPERAND_InitializeIntOrIVecConstant(
                                                    &baseROperand,
                                                    gcSHADER_INTEGER_X1,
                                                    (gctINT32)samplerIndex);

            status = _GenIndexAddCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    realLOperand.reg.regIndex,
                                    ROperand->arrayIndex.u.indexRegIndex,
                                    &baseROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            break;

        default:
            gcmASSERT(0);
        }
    }
    else
    {
        realROperand            = *ROperand;
        realROperand.dataType   = realROperand.u.reg.dataType   = gcSHADER_INTEGER_X1;

        status = slGenAssignCode(
                                Compiler,
                                LineNo,
                                StringNo,
                                &realLOperand,
                                &realROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slGenAssignCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsLOPERAND * LOperand,
    IN slsROPERAND * ROperand
    )
{
    gceSTATUS       status;
    slsROPERAND     newROperand;
    slsIOPERAND     intermIOperand;
    slsLOPERAND     intermLOperand;
    slsROPERAND     intermROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LOperand);
    gcmASSERT(LOperand->vectorIndex.mode != slvINDEX_REG);
    gcmASSERT(ROperand);
    gcmASSERT(ROperand->vectorIndex.mode != slvINDEX_REG);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<OPERATION line=\"%d\" string=\"%d\" type=\"assign\">",
                            LineNo,
                            StringNo));

    gcmVERIFY_OK(slsLOPERAND_Dump(Compiler, LOperand));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand));

    gcmASSERT(ROperand->isReg || (ROperand->arrayIndex.mode != slvINDEX_REG));

    if (!ROperand->isReg && (ROperand->matrixIndex.mode == slvINDEX_REG))
    {
        newROperand = *ROperand;
        newROperand.matrixIndex.mode = slvINDEX_NONE;

        slsIOPERAND_New(Compiler, &intermIOperand, ROperand->dataType);
        slsLOPERAND_InitializeUsingIOperand(&intermLOperand, &intermIOperand);

        status = _SpecialGenAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    &intermLOperand,
                                    &newROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);
        intermROperand.matrixIndex = ROperand->matrixIndex;

        status = _SpecialGenAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    LOperand,
                                    &intermROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        status = _SpecialGenAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    LOperand,
                                    ROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</OPERATION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvIOperandToTarget(
    IN sloCOMPILER Compiler,
    IN slsIOPERAND * IOperand,
    OUT gcsTARGET * Target
    )
{
    slsLOPERAND             lOperand;
    slsCOMPONENT_SELECTION  reversedComponentSelection;
    gceSTATUS               status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IOperand);
    gcmASSERT(!gcIsMatrixDataType(IOperand->dataType));
    gcmASSERT(Target);

    slsLOPERAND_InitializeUsingIOperand(&lOperand, IOperand);

    status = _ConvLOperandToTarget(
                                Compiler,
                                &lOperand,
                                Target,
                                &reversedComponentSelection);
    gcmFOOTER();
    return status;
}

static gceSTATUS
_ConvIOperandToVectorComponentTarget(
    IN sloCOMPILER Compiler,
    IN slsIOPERAND * IOperand,
    IN gctUINT VectorIndex,
    OUT gcsTARGET * Target
    )
{
    gceSTATUS       status;
    slsIOPERAND     componentIOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IOperand);
    gcmASSERT(gcIsVectorDataType(IOperand->dataType));
    gcmASSERT(Target);

    componentIOperand           = *IOperand;
    componentIOperand.dataType  = gcGetVectorComponentDataType(IOperand->dataType);

    status =  _ConvIOperandToTarget(
                                    Compiler,
                                    &componentIOperand,
                                    Target);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    switch (VectorIndex)
    {
    case 0: Target->enable = gcSL_ENABLE_X; break;
    case 1: Target->enable = gcSL_ENABLE_Y; break;
    case 2: Target->enable = gcSL_ENABLE_Z; break;
    case 3: Target->enable = gcSL_ENABLE_W; break;

    default: gcmASSERT(0);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_ConvIOperandToMatrixColumnTarget(
    IN sloCOMPILER Compiler,
    IN slsIOPERAND * IOperand,
    IN gctUINT MatrixIndex,
    OUT gcsTARGET * Target
    )
{
    slsIOPERAND     columnIOperand;
    gceSTATUS       status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IOperand);
    gcmASSERT(gcIsMatrixDataType(IOperand->dataType));
    gcmASSERT(Target);

    columnIOperand              = *IOperand;
    columnIOperand.dataType     = gcGetMatrixColumnDataType(IOperand->dataType);
    columnIOperand.tempRegIndex += (gctREG_INDEX) MatrixIndex;

    status = _ConvIOperandToTarget(
                                Compiler,
                                &columnIOperand,
                                Target);
    gcmFOOTER();
    return status;
}

static gceSTATUS
_ConvNormalROperandToSource(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsROPERAND * ROperand,
    OUT gcsSOURCE * Source
    )
{
    gceSTATUS               status = gcvSTATUS_OK;
    slsIOPERAND             intermIOperand;
    slsLOPERAND             intermLOperand;
    slsROPERAND             intermROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);
    gcmASSERT(!gcIsMatrixDataType(ROperand->dataType));
    gcmASSERT(Source);

    /* Assign the non-scalar constant operand to a new register operand */
    if (!ROperand->isReg && gcIsScalarDataType(ROperand->dataType))
    {
        gcmONERROR(_ConvROperandToSourceConstant(
                                                Compiler,
                                                ROperand,
                                                Source));
    }
    else if (!ROperand->isReg && gcIsVectorDataType(ROperand->dataType)
            && slsROPERAND_CONSTANT_IsAllVectorComponentsEqual(ROperand))
    {
        gcmONERROR(_ConvROperandToSpecialVectorSourceConstant(
                                                                Compiler,
                                                                ROperand,
                                                                Source));
    }
    else
    {
        if (!ROperand->isReg)
        {
            gcmASSERT(gcIsVectorDataType(ROperand->dataType));

            slsIOPERAND_New(Compiler, &intermIOperand, ROperand->dataType);
            slsLOPERAND_InitializeUsingIOperand(&intermLOperand, &intermIOperand);

            gcmONERROR(slGenAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    &intermLOperand,
                                    ROperand));

            slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);
            ROperand = &intermROperand;
        }

        gcmONERROR(_ConvROperandToSourceReg(
                                            Compiler,
                                            ROperand,
                                            slGetDefaultComponentSelection(ROperand->dataType),
                                            Source));
    }
OnError:
    gcmFOOTER();
    return status;
}

static gceSTATUS
_ConvNormalROperandToMatrixColumnSource(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsROPERAND * ROperand,
    IN gctUINT MatrixIndex,
    OUT gcsSOURCE * Source
    )
{
    gcSHADER_TYPE           dataType;
    slsROPERAND             columnROperand;
    gceSTATUS               status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ROperand);
    gcmASSERT(gcIsMatrixDataType(ROperand->dataType));
    gcmASSERT(ROperand->matrixIndex.mode == slvINDEX_NONE);
    gcmASSERT(Source);

    dataType = gcGetMatrixColumnDataType(ROperand->dataType);

    columnROperand                          = *ROperand;
    columnROperand.dataType                 = dataType;
    columnROperand.matrixIndex.mode         = slvINDEX_CONSTANT;
    columnROperand.matrixIndex.u.constant   = (gctREG_INDEX)MatrixIndex;

    status = _ConvNormalROperandToSource(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    &columnROperand,
                                    Source);
    gcmFOOTER();
    return status;
}

static gceSTATUS
_GenMatrixMulVectorCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsIOPERAND * IOperand,
    IN slsROPERAND * ROperand0,
    IN slsROPERAND * ROperand1
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
    )
{
    gceSTATUS   status;
    gcsTARGET   target;
    gcsSOURCE   source0;
    gcsSOURCE   source1;
    gctUINT     i;
    slsIOPERAND columnIOperand;
    slsROPERAND columnROperand1;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IOperand);
    gcmASSERT(ROperand0);
    gcmASSERT(ROperand1);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<OPERATION line=\"%d\" string=\"%d\" type=\"%s\">",
                            LineNo,
                            StringNo,
                            slGetOpcodeName(Opcode)));

    gcmVERIFY_OK(slsIOPERAND_Dump(Compiler, IOperand));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand0));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand1));

    switch (Opcode)
    {
    case slvOPCODE_ADD:
    case slvOPCODE_SUB:
    case slvOPCODE_MUL:
    case slvOPCODE_DIV:
        break;

    default: gcmASSERT(0);
    }

    if ((Opcode == slvOPCODE_MUL) &&
        gcIsVectorDataType(ROperand0->dataType) && gcIsMatrixDataType(ROperand1->dataType))
    {
        gcmASSERT(gcGetVectorDataTypeComponentCount(IOperand->dataType)
                    == gcGetVectorDataTypeComponentCount(ROperand0->dataType));
        gcmASSERT(gcGetVectorDataTypeComponentCount(IOperand->dataType)
                    == gcGetMatrixDataTypeColumnCount(ROperand1->dataType));

        status = _ConvNormalROperandToSource(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            ROperand0,
                                            &source0);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        for (i = 0; i < gcGetVectorDataTypeComponentCount(IOperand->dataType); i++)
        {
            gcmVERIFY_OK(_ConvIOperandToVectorComponentTarget(Compiler, IOperand, i, &target));

            status = _ConvNormalROperandToMatrixColumnSource(
                                                            Compiler,
                                                            LineNo,
                                                            StringNo,
                                                            ROperand1,
                                                            i,
                                                            &source1);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            status = slEmitCode2(
                                Compiler,
                                LineNo,
                                StringNo,
                                slvOPCODE_DOT,
                                &target,
                                &source0,
                                &source1);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
    }
    else if ((Opcode == slvOPCODE_MUL) &&
        gcIsMatrixDataType(ROperand0->dataType) && gcIsVectorDataType(ROperand1->dataType))
    {
        status = _GenMatrixMulVectorCode(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        IOperand,
                                        ROperand0,
                                        ROperand1);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else if ((Opcode == slvOPCODE_MUL) &&
        gcIsMatrixDataType(ROperand0->dataType) && gcIsMatrixDataType(ROperand1->dataType))
    {
        gcmASSERT(gcGetMatrixDataTypeColumnCount(IOperand->dataType)
                    == gcGetMatrixDataTypeColumnCount(ROperand0->dataType));
        gcmASSERT(gcGetMatrixDataTypeColumnCount(IOperand->dataType)
                    == gcGetMatrixDataTypeColumnCount(ROperand1->dataType));

        for (i = 0; i < gcGetMatrixDataTypeColumnCount(IOperand->dataType); i++)
        {
            slsIOPERAND_InitializeAsMatrixColumn(&columnIOperand, IOperand, i);
            slsROPERAND_InitializeAsMatrixColumn(&columnROperand1, ROperand1, i);

            status = _GenMatrixMulVectorCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            &columnIOperand,
                                            ROperand0,
                                            &columnROperand1);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
    }
    else
    {
        if (ROperand0->dataType == ROperand1->dataType)
        {
            if (gcIsScalarDataType(ROperand0->dataType) || gcIsVectorDataType(ROperand0->dataType))
            {
                status = _ConvIOperandToTarget(
                                                Compiler,
                                                IOperand,
                                                &target);
                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                status = _ConvNormalROperandToSource(
                                                    Compiler,
                                                    LineNo,
                                                    StringNo,
                                                    ROperand0,
                                                    &source0);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                status = _ConvNormalROperandToSource(
                                                    Compiler,
                                                    LineNo,
                                                    StringNo,
                                                    ROperand1,
                                                    &source1);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                status = slEmitCode2(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    Opcode,
                                    &target,
                                    &source0,
                                    &source1);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
            else
            {
                gcmASSERT(gcIsMatrixDataType(ROperand0->dataType));

                for (i = 0; i < gcGetMatrixDataTypeColumnCount(ROperand0->dataType); i++)
                {
                    gcmVERIFY_OK(_ConvIOperandToMatrixColumnTarget(Compiler, IOperand, i, &target));

                    status = _ConvNormalROperandToMatrixColumnSource(
                                                                    Compiler,
                                                                    LineNo,
                                                                    StringNo,
                                                                    ROperand0,
                                                                    i,
                                                                    &source0);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                    status = _ConvNormalROperandToMatrixColumnSource(
                                                                    Compiler,
                                                                    LineNo,
                                                                    StringNo,
                                                                    ROperand1,
                                                                    i,
                                                                    &source1);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                    status = slEmitCode2(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        Opcode,
                                        &target,
                                        &source0,
                                        &source1);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
            }
        }
        else
        {
            if (gcIsMatrixDataType(ROperand0->dataType))
            {
                gcmASSERT(gcIsScalarDataType(ROperand1->dataType));

                status = _ConvNormalROperandToSource(
                                                    Compiler,
                                                    LineNo,
                                                    StringNo,
                                                    ROperand1,
                                                    &source1);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                for (i = 0; i < gcGetMatrixDataTypeColumnCount(ROperand0->dataType); i++)
                {
                    gcmVERIFY_OK(_ConvIOperandToMatrixColumnTarget(Compiler, IOperand, i, &target));

                    status = _ConvNormalROperandToMatrixColumnSource(
                                                                    Compiler,
                                                                    LineNo,
                                                                    StringNo,
                                                                    ROperand0,
                                                                    i,
                                                                    &source0);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                    status = slEmitCode2(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        Opcode,
                                        &target,
                                        &source0,
                                        &source1);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
            }
            else if (gcIsMatrixDataType(ROperand1->dataType))
            {
                gcmASSERT(gcIsScalarDataType(ROperand0->dataType));

                status = _ConvNormalROperandToSource(
                                                    Compiler,
                                                    LineNo,
                                                    StringNo,
                                                    ROperand0,
                                                    &source0);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                for (i = 0; i < gcGetMatrixDataTypeColumnCount(ROperand1->dataType); i++)
                {
                    gcmVERIFY_OK(_ConvIOperandToMatrixColumnTarget(Compiler, IOperand, i, &target));

                    status = _ConvNormalROperandToMatrixColumnSource(
                                                                    Compiler,
                                                                    LineNo,
                                                                    StringNo,
                                                                    ROperand1,
                                                                    i,
                                                                    &source1);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                    status = slEmitCode2(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        Opcode,
                                        &target,
                                        &source0,
                                        &source1);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
            }
            else
            {
                gcmASSERT((gcIsVectorDataType(ROperand0->dataType)
                                && gcIsScalarDataType(ROperand1->dataType))
                        || (gcIsScalarDataType(ROperand0->dataType)
                                && gcIsVectorDataType(ROperand1->dataType)));

                status = _ConvIOperandToTarget(
                                                Compiler,
                                                IOperand,
                                                &target);
                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                status = _ConvNormalROperandToSource(
                                                    Compiler,
                                                    LineNo,
                                                    StringNo,
                                                    ROperand0,
                                                    &source0);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                status = _ConvNormalROperandToSource(
                                                    Compiler,
                                                    LineNo,
                                                    StringNo,
                                                    ROperand1,
                                                    &source1);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                status = slEmitCode2(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    Opcode,
                                    &target,
                                    &source0,
                                    &source1);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
        }
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</OPERATION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slGenGenericCode1(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleOPCODE Opcode,
    IN slsIOPERAND * IOperand,
    IN slsROPERAND * ROperand
    )
{
    gceSTATUS   status;
    gcsTARGET   target;
    gcsSOURCE   source;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IOperand);
    gcmASSERT(ROperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<OPERATION line=\"%d\" string=\"%d\" type=\"%s\">",
                            LineNo,
                            StringNo,
                            slGetOpcodeName(Opcode)));

    gcmVERIFY_OK(slsIOPERAND_Dump(Compiler, IOperand));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand));

    switch (Opcode)
    {
    case slvOPCODE_FLOAT_TO_INT:
    case slvOPCODE_FLOAT_TO_BOOL:
    case slvOPCODE_INT_TO_BOOL:

    case slvOPCODE_INVERSE:

    case slvOPCODE_ANY:
    case slvOPCODE_ALL:
    case slvOPCODE_NOT:

    case slvOPCODE_SIN:
    case slvOPCODE_COS:
    case slvOPCODE_TAN:

    case slvOPCODE_ASIN:
    case slvOPCODE_ACOS:
    case slvOPCODE_ATAN:

    case slvOPCODE_EXP2:
    case slvOPCODE_LOG2:
    case slvOPCODE_SQRT:
    case slvOPCODE_INVERSE_SQRT:

    case slvOPCODE_ABS:
    case slvOPCODE_SIGN:
    case slvOPCODE_FLOOR:
    case slvOPCODE_CEIL:
    case slvOPCODE_FRACT:
    case slvOPCODE_SATURATE:

    case slvOPCODE_NORMALIZE:

    case slvOPCODE_DFDX:
    case slvOPCODE_DFDY:
    case slvOPCODE_FWIDTH:
        break;

    default: gcmASSERT(0);
    }

    status = _ConvIOperandToTarget(
                                    Compiler,
                                    IOperand,
                                    &target);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _ConvNormalROperandToSource(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        ROperand,
                                        &source);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slEmitCode1(
                        Compiler,
                        LineNo,
                        StringNo,
                        Opcode,
                        &target,
                        &source);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</OPERATION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slGenGenericCode2(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleOPCODE Opcode,
    IN slsIOPERAND * IOperand,
    IN slsROPERAND * ROperand0,
    IN slsROPERAND * ROperand1
    )
{
    gceSTATUS   status;
    gcsTARGET   target;
    gcsSOURCE   source0;
    gcsSOURCE   source1;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IOperand);
    gcmASSERT(ROperand0);
    gcmASSERT(ROperand1);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<OPERATION line=\"%d\" string=\"%d\" type=\"%s\">",
                            LineNo,
                            StringNo,
                            slGetOpcodeName(Opcode)));

    gcmVERIFY_OK(slsIOPERAND_Dump(Compiler, IOperand));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand0));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand1));

    switch (Opcode)
    {
    case slvOPCODE_TEXTURE_LOAD:
    case slvOPCODE_TEXTURE_LOAD_PROJ:
    case slvOPCODE_TEXTURE_BIAS:
    case slvOPCODE_TEXTURE_LOD:

    case slvOPCODE_LESS_THAN:
    case slvOPCODE_LESS_THAN_EQUAL:
    case slvOPCODE_GREATER_THAN:
    case slvOPCODE_GREATER_THAN_EQUAL:
    case slvOPCODE_EQUAL:
    case slvOPCODE_NOT_EQUAL:

    case slvOPCODE_ATAN2:

    case slvOPCODE_POW:

    case slvOPCODE_MIN:
    case slvOPCODE_MAX:
    case slvOPCODE_STEP:
    case slvOPCODE_DOT:
    case slvOPCODE_CROSS:
        break;

    default: gcmASSERT(0);
    }

    status = _ConvIOperandToTarget(
                                    Compiler,
                                    IOperand,
                                    &target);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _ConvNormalROperandToSource(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        ROperand0,
                                        &source0);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _ConvNormalROperandToSource(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        ROperand1,
                                        &source1);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slEmitCode2(
                        Compiler,
                        LineNo,
                        StringNo,
                        Opcode,
                        &target,
                        &source0,
                        &source1);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</OPERATION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slGenTestJumpCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN gctLABEL Label,
    IN gctBOOL TrueJump,
    IN slsROPERAND * ROperand
    )
{
    gceSTATUS   status;
    gcsSOURCE   source;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(ROperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<TEST line=\"%d\" string=\"%d\" trueJump=\"%s\">",
                            LineNo,
                            StringNo,
                            TrueJump ? "true" : "false"));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand));

    status = _ConvNormalROperandToSource(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        ROperand,
                                        &source);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slEmitTestBranchCode(
                                Compiler,
                                LineNo,
                                StringNo,
                                slvOPCODE_JUMP,
                                Label,
                                TrueJump,
                                &source);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</TEST>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

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
    )
{
    gceSTATUS   status;
    gcsSOURCE   source0;
    gcsSOURCE   source1;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(ROperand0);
    gcmASSERT(ROperand1);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<CONDITION line=\"%d\" string=\"%d\""
                            " trueJump=\"%s\" compareType=\"%s\">",
                            LineNo,
                            StringNo,
                            TrueJump ? "true" : "false",
                            slGetConditionName(CompareCondition)));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand0));

    gcmVERIFY_OK(slsROPERAND_Dump(Compiler, ROperand1));

    status = _ConvNormalROperandToSource(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        ROperand0,
                                        &source0);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _ConvNormalROperandToSource(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        ROperand1,
                                        &source1);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }


    status = slEmitCompareBranchCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    slvOPCODE_JUMP,
                                    TrueJump ?
                                        CompareCondition : slGetNotCondition(CompareCondition),
                                    Label,
                                    &source0,
                                    &source1);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</CONDITION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenConditionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_EXPR CondExpr,
    IN gctLABEL Label,
    IN gctBOOL TrueJump
    );

gceSTATUS
sloIR_BINARY_EXPR_GenRelationalConditionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN gctLABEL Label,
    IN gctBOOL TrueJump
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;
    sleCONDITION            condition;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &leftParameters,
                                    gcvFALSE,
                                    gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmASSERT(leftParameters.operandCount == 1);

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmASSERT(rightParameters.operandCount == 1);

    /* Generate the condition code */
    switch (BinaryExpr->type)
    {
    case slvBINARY_GREATER_THAN:
        condition = slvCONDITION_GREATER_THAN;
        break;

    case slvBINARY_LESS_THAN:
        condition = slvCONDITION_LESS_THAN;
        break;

    case slvBINARY_GREATER_THAN_EQUAL:
        condition = slvCONDITION_GREATER_THAN_EQUAL;
        break;

    case slvBINARY_LESS_THAN_EQUAL:
        condition = slvCONDITION_LESS_THAN_EQUAL;
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status = slGenCompareJumpCode(
                                Compiler,
                                CodeGenerator,
                                BinaryExpr->exprBase.base.lineNo,
                                BinaryExpr->exprBase.base.stringNo,
                                Label,
                                TrueJump,
                                condition,
                                &leftParameters.rOperands[0],
                                &rightParameters.rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenMultiplyEqualityConditionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN gctLABEL Label,
    IN gctBOOL TrueJump,
    IN sleCONDITION CompareCondition,
    IN gctUINT OperandCount,
    IN gcSHADER_TYPE * DataTypes,
    IN slsROPERAND * ROperands0,
    IN slsROPERAND * ROperands1
    )
{
    gceSTATUS       status;
    gctUINT         i, j, k;
    slsROPERAND     rOperand0;
    slsROPERAND     rOperand1;
    gctLABEL        endLabel;
    gctLABEL        targetLabel;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(CompareCondition == slvCONDITION_EQUAL
                || CompareCondition == slvCONDITION_NOT_EQUAL);
    gcmASSERT(OperandCount >= 1);
    gcmASSERT(DataTypes);
    gcmASSERT(ROperands0);
    gcmASSERT(ROperands1);

    if (!TrueJump)  CompareCondition = slGetNotCondition(CompareCondition);

    if (CompareCondition == slvCONDITION_NOT_EQUAL)
    {
        for (i = 0; i < OperandCount; i++)
        {
            if (gcIsScalarDataType(DataTypes[i]))
            {
                status = slGenCompareJumpCode(
                                            Compiler,
                                            CodeGenerator,
                                            LineNo,
                                            StringNo,
                                            Label,
                                            gcvTRUE,
                                            CompareCondition,
                                            &ROperands0[i],
                                            &ROperands1[i]);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
            else if (gcIsVectorDataType(DataTypes[i]))
            {
                for (j = 0; j < gcGetVectorDataTypeComponentCount(DataTypes[i]); j++)
                {
                    slsROPERAND_InitializeAsVectorComponent(&rOperand0, &ROperands0[i], j);
                    slsROPERAND_InitializeAsVectorComponent(&rOperand1, &ROperands1[i], j);

                    status = slGenCompareJumpCode(
                                                Compiler,
                                                CodeGenerator,
                                                LineNo,
                                                StringNo,
                                                Label,
                                                gcvTRUE,
                                                CompareCondition,
                                                &rOperand0,
                                                &rOperand1);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
            }
            else
            {
                gcmASSERT(gcIsMatrixDataType(DataTypes[i]));

                for (j = 0; j < gcGetMatrixDataTypeColumnCount(DataTypes[i]); j++)
                {
                    for (k = 0; k < gcGetMatrixDataTypeColumnCount(DataTypes[i]); k++)
                    {
                        slsROPERAND_InitializeAsMatrixComponent(&rOperand0, &ROperands0[i], j, k);
                        slsROPERAND_InitializeAsMatrixComponent(&rOperand1, &ROperands1[i], j, k);

                        status = slGenCompareJumpCode(
                                                    Compiler,
                                                    CodeGenerator,
                                                    LineNo,
                                                    StringNo,
                                                    Label,
                                                    gcvTRUE,
                                                    CompareCondition,
                                                    &rOperand0,
                                                    &rOperand1);

                        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                    }
                }
            }
        }   /* for (i = 0; i < OperandCount; i++) */
    }
    else
    {
        endLabel = slNewLabel(Compiler);

        for (i = 0; i < OperandCount; i++)
        {
            if (gcIsScalarDataType(DataTypes[i]))
            {
                if (i == OperandCount - 1)
                {
                    targetLabel = Label;
                    TrueJump    = gcvTRUE;
                }
                else
                {
                    targetLabel = endLabel;
                    TrueJump    = gcvFALSE;
                }

                status = slGenCompareJumpCode(
                                            Compiler,
                                            CodeGenerator,
                                            LineNo,
                                            StringNo,
                                            targetLabel,
                                            TrueJump,
                                            CompareCondition,
                                            &ROperands0[i],
                                            &ROperands1[i]);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
            else if (gcIsVectorDataType(DataTypes[i]))
            {
                for (j = 0; j < gcGetVectorDataTypeComponentCount(DataTypes[i]); j++)
                {
                    if (i == OperandCount - 1
                        && j == (gctUINT) gcGetVectorDataTypeComponentCount(DataTypes[i]) - 1)
                    {
                        targetLabel = Label;
                        TrueJump    = gcvTRUE;
                    }
                    else
                    {
                        targetLabel = endLabel;
                        TrueJump    = gcvFALSE;
                    }

                    slsROPERAND_InitializeAsVectorComponent(&rOperand0, &ROperands0[i], j);
                    slsROPERAND_InitializeAsVectorComponent(&rOperand1, &ROperands1[i], j);

                    status = slGenCompareJumpCode(
                                                Compiler,
                                                CodeGenerator,
                                                LineNo,
                                                StringNo,
                                                targetLabel,
                                                TrueJump,
                                                CompareCondition,
                                                &rOperand0,
                                                &rOperand1);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
            }
            else
            {
                gcmASSERT(gcIsMatrixDataType(DataTypes[i]));

                for (j = 0; j < gcGetMatrixDataTypeColumnCount(DataTypes[i]); j++)
                {
                    for (k = 0; k < gcGetMatrixDataTypeColumnCount(DataTypes[i]); k++)
                    {
                        if (i == OperandCount - 1
                            && j == gcGetMatrixDataTypeColumnCount(DataTypes[i]) - 1
                            && k == gcGetMatrixDataTypeColumnCount(DataTypes[i]) - 1)
                        {
                            targetLabel = Label;
                            TrueJump    = gcvTRUE;
                        }
                        else
                        {
                            targetLabel = endLabel;
                            TrueJump    = gcvFALSE;
                        }

                        slsROPERAND_InitializeAsMatrixComponent(&rOperand0, &ROperands0[i], j, k);
                        slsROPERAND_InitializeAsMatrixComponent(&rOperand1, &ROperands1[i], j, k);

                        status = slGenCompareJumpCode(
                                                    Compiler,
                                                    CodeGenerator,
                                                    LineNo,
                                                    StringNo,
                                                    targetLabel,
                                                    TrueJump,
                                                    CompareCondition,
                                                    &rOperand0,
                                                    &rOperand1);

                        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                    }
                }
            }
        }   /* for (i = 0; i < OperandCount; i++) */

        /* end: */
        status = slSetLabel(
                            Compiler,
                            LineNo,
                            StringNo,
                            endLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenEqualityConditionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN gctLABEL Label,
    IN gctBOOL TrueJump
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;
    sleCONDITION            condition;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &leftParameters,
                                    gcvFALSE,
                                    gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Get the condition code */
    switch (BinaryExpr->type)
    {
    case slvBINARY_EQUAL:
        condition = slvCONDITION_EQUAL;
        break;

    case slvBINARY_NOT_EQUAL:
    case slvBINARY_XOR:
        condition = slvCONDITION_NOT_EQUAL;
        break;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (slsDATA_TYPE_IsScalar(BinaryExpr->leftOperand->dataType))
    {
        gcmASSERT(leftParameters.operandCount == 1);
        gcmASSERT(rightParameters.operandCount == 1);

        status = slGenCompareJumpCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->exprBase.base.lineNo,
                                    BinaryExpr->exprBase.base.stringNo,
                                    Label,
                                    TrueJump,
                                    condition,
                                    &leftParameters.rOperands[0],
                                    &rightParameters.rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        gcmASSERT(leftParameters.operandCount == rightParameters.operandCount);

        status = _GenMultiplyEqualityConditionCode(
                                                    Compiler,
                                                    CodeGenerator,
                                                    BinaryExpr->exprBase.base.lineNo,
                                                    BinaryExpr->exprBase.base.stringNo,
                                                    Label,
                                                    TrueJump,
                                                    condition,
                                                    leftParameters.operandCount,
                                                    leftParameters.dataTypes,
                                                    leftParameters.rOperands,
                                                    rightParameters.rOperands);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenAndConditionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN gctLABEL Label,
    IN gctBOOL TrueJump
    )
{
    gceSTATUS   status;
    gctLABEL    endLabel;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);

    if (TrueJump)
    {
        endLabel = slNewLabel(Compiler);

        /* jump end if !(left) */
        gcmASSERT(BinaryExpr->leftOperand);

        status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->leftOperand,
                                    endLabel,
                                    gcvFALSE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* jump label if (right) */
        gcmASSERT(BinaryExpr->rightOperand);

        status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->rightOperand,
                                    Label,
                                    gcvTRUE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* end: */
        status = slSetLabel(
                            Compiler,
                            BinaryExpr->exprBase.base.lineNo,
                            BinaryExpr->exprBase.base.stringNo,
                            endLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        /* jump label if !(left) */
        gcmASSERT(BinaryExpr->leftOperand);

        status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->leftOperand,
                                    Label,
                                    gcvFALSE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* jump label if !(right) */
        gcmASSERT(BinaryExpr->rightOperand);

        status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->rightOperand,
                                    Label,
                                    gcvFALSE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenOrConditionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN gctLABEL Label,
    IN gctBOOL TrueJump
    )
{
    gceSTATUS   status;
    gctLABEL    endLabel;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);

    if (TrueJump)
    {
        /* jump label if (left) */
        gcmASSERT(BinaryExpr->leftOperand);

        status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->leftOperand,
                                    Label,
                                    gcvTRUE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* jump label if (right) */
        gcmASSERT(BinaryExpr->rightOperand);

        status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->rightOperand,
                                    Label,
                                    gcvTRUE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        endLabel = slNewLabel(Compiler);

        /* jump end if (left) */
        gcmASSERT(BinaryExpr->leftOperand);

        status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->leftOperand,
                                    endLabel,
                                    gcvTRUE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* jump label if !(right) */
        gcmASSERT(BinaryExpr->rightOperand);

        status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    BinaryExpr->rightOperand,
                                    Label,
                                    gcvFALSE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* end: */
        status = slSetLabel(
                            Compiler,
                            BinaryExpr->exprBase.base.lineNo,
                            BinaryExpr->exprBase.base.stringNo,
                            endLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenConditionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_EXPR CondExpr,
    IN gctLABEL Label,
    IN gctBOOL TrueJump
    )
{
    gceSTATUS               status;
    sloIR_BINARY_EXPR       binaryExpr;
    sloIR_UNARY_EXPR        unaryExpr;
    slsGEN_CODE_PARAMETERS  condParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(CondExpr);

    switch (sloIR_OBJECT_GetType(&CondExpr->base))
    {
    case slvIR_BINARY_EXPR:
        binaryExpr = (sloIR_BINARY_EXPR)CondExpr;

        switch (binaryExpr->type)
        {
        case slvBINARY_GREATER_THAN:
        case slvBINARY_LESS_THAN:
        case slvBINARY_GREATER_THAN_EQUAL:
        case slvBINARY_LESS_THAN_EQUAL:
            status = sloIR_BINARY_EXPR_GenRelationalConditionCode(
                                                                Compiler,
                                                                CodeGenerator,
                                                                binaryExpr,
                                                                Label,
                                                                TrueJump);
            gcmFOOTER();
            return status;

        case slvBINARY_EQUAL:
        case slvBINARY_NOT_EQUAL:
        case slvBINARY_XOR:
            status = sloIR_BINARY_EXPR_GenEqualityConditionCode(
                                                            Compiler,
                                                            CodeGenerator,
                                                            binaryExpr,
                                                            Label,
                                                            TrueJump);
            gcmFOOTER();
            return status;

        case slvBINARY_AND:
            status = sloIR_BINARY_EXPR_GenAndConditionCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        binaryExpr,
                                                        Label,
                                                        TrueJump);
            gcmFOOTER();
            return status;

        case slvBINARY_OR:
            status = sloIR_BINARY_EXPR_GenOrConditionCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        binaryExpr,
                                                        Label,
                                                        TrueJump);
            gcmFOOTER();
            return status;

        default: break;
        }
        break;

    case slvIR_UNARY_EXPR:
        unaryExpr = (sloIR_UNARY_EXPR)CondExpr;
        gcmASSERT(unaryExpr->operand);

        switch (unaryExpr->type)
        {
        case slvUNARY_NOT:
            status = _GenConditionCode(
                                    Compiler,
                                    CodeGenerator,
                                    unaryExpr->operand,
                                    Label,
                                    !TrueJump);
            gcmFOOTER();
            return status;

        default: break;
        }
        break;

    default: break;
    }

    slsGEN_CODE_PARAMETERS_Initialize(&condParameters, gcvFALSE, gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &CondExpr->base,
                                &CodeGenerator->visitor,
                                &condParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slGenTestJumpCode(
                                Compiler,
                                CodeGenerator,
                                CondExpr->base.lineNo,
                                CondExpr->base.stringNo,
                                Label,
                                TrueJump,
                                &condParameters.rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsGEN_CODE_PARAMETERS_Finalize(&condParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slDefineSelectionBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN gctBOOL HasFalseOperand,
    OUT slsSELECTION_CONTEXT * SelectionContext
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(SelectionContext);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<SELECTION hasFalseOperand=\"%s\">",
                            HasFalseOperand ? "true" : "false"));

    SelectionContext->hasFalseOperand       = HasFalseOperand;
    SelectionContext->endLabel              = slNewLabel(Compiler);

    if (HasFalseOperand)
    {
        SelectionContext->beginLabelOfFalseOperand  = slNewLabel(Compiler);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slDefineSelectionEnd(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsSELECTION_CONTEXT * SelectionContext
    )
{
    gceSTATUS       status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(SelectionContext);

    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        SelectionContext->endLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</SELECTION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctLABEL
_GetSelectionConditionLabel(
    IN slsSELECTION_CONTEXT * SelectionContext
    )
{
    gcmASSERT(SelectionContext);

    return (SelectionContext->hasFalseOperand) ?
                SelectionContext->beginLabelOfFalseOperand : SelectionContext->endLabel;
}

gceSTATUS
slGenSelectionTestConditionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsSELECTION_CONTEXT * SelectionContext,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsROPERAND * ROperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(SelectionContext);
    gcmASSERT(ROperand);

    status = slGenTestJumpCode(
                            Compiler,
                            CodeGenerator,
                            LineNo,
                            StringNo,
                            _GetSelectionConditionLabel(SelectionContext),
                            gcvFALSE,
                            ROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

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
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(SelectionContext);
    gcmASSERT(ROperand0);
    gcmASSERT(ROperand1);

    status = slGenCompareJumpCode(
                                Compiler,
                                CodeGenerator,
                                LineNo,
                                StringNo,
                                _GetSelectionConditionLabel(SelectionContext),
                                gcvFALSE,
                                CompareCondition,
                                ROperand0,
                                ROperand1);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slDefineSelectionTrueOperandBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsSELECTION_CONTEXT * SelectionContext
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(SelectionContext);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<TRUE_OPERAND>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slDefineSelectionTrueOperandEnd(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsSELECTION_CONTEXT * SelectionContext,
    IN gctBOOL HasReturn
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(SelectionContext);

    if (SelectionContext->hasFalseOperand && !HasReturn)
    {
        status = slEmitAlwaysBranchCode(
                                        Compiler,
                                        0,
                                        0,
                                        slvOPCODE_JUMP,
                                        SelectionContext->endLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</TRUE_OPERAND>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slDefineSelectionFalseOperandBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsSELECTION_CONTEXT * SelectionContext
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(SelectionContext);
    gcmASSERT(SelectionContext->hasFalseOperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "<FALSE_OPERAND>"));

    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        SelectionContext->beginLabelOfFalseOperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slDefineSelectionFalseOperandEnd(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsSELECTION_CONTEXT * SelectionContext
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(SelectionContext);
    gcmASSERT(SelectionContext->hasFalseOperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                            Compiler,
                            slvDUMP_CODE_GENERATOR,
                            "</FALSE_OPERAND>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenMatrixMulVectorCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsIOPERAND * IOperand,
    IN slsROPERAND * ROperand0,
    IN slsROPERAND * ROperand1
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperands[6];
    slsROPERAND     rOperand0;
    slsROPERAND     rOperand1;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IOperand);
    gcmASSERT(ROperand0);
    gcmASSERT(ROperand1);

    /* mul t0, m[0], v.x */
    slsIOPERAND_New(Compiler, &intermIOperands[0], ROperand1->dataType);
    slsROPERAND_InitializeAsMatrixColumn(&rOperand0, ROperand0, 0);
    slsROPERAND_InitializeAsVectorComponent(&rOperand1, ROperand1, 0);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[0],
                                    &rOperand0,
                                    &rOperand1);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul t1, m[1], v.y */
    slsIOPERAND_New(Compiler, &intermIOperands[1], ROperand1->dataType);
    slsROPERAND_InitializeAsMatrixColumn(&rOperand0, ROperand0, 1);
    slsROPERAND_InitializeAsVectorComponent(&rOperand1, ROperand1, 1);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[1],
                                    &rOperand0,
                                    &rOperand1);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (ROperand0->dataType == gcSHADER_FLOAT_2X2)
    {
        /* add result, t0, t1 */
        slsROPERAND_InitializeUsingIOperand(&rOperand0, &intermIOperands[0]);
        slsROPERAND_InitializeUsingIOperand(&rOperand1, &intermIOperands[1]);

        status = slGenArithmeticExprCode(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        slvOPCODE_ADD,
                                        IOperand,
                                        &rOperand0,
                                        &rOperand1);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        /* add t2, t0, t1 */
        slsIOPERAND_New(Compiler, &intermIOperands[2], ROperand1->dataType);
        slsROPERAND_InitializeUsingIOperand(&rOperand0, &intermIOperands[0]);
        slsROPERAND_InitializeUsingIOperand(&rOperand1, &intermIOperands[1]);

        status = slGenArithmeticExprCode(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        slvOPCODE_ADD,
                                        &intermIOperands[2],
                                        &rOperand0,
                                        &rOperand1);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* mul t3, m[2], v.z */
        slsIOPERAND_New(Compiler, &intermIOperands[3], ROperand1->dataType);
        slsROPERAND_InitializeAsMatrixColumn(&rOperand0, ROperand0, 2);
        slsROPERAND_InitializeAsVectorComponent(&rOperand1, ROperand1, 2);

        status = slGenArithmeticExprCode(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        slvOPCODE_MUL,
                                        &intermIOperands[3],
                                        &rOperand0,
                                        &rOperand1);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (ROperand0->dataType == gcSHADER_FLOAT_3X3)
        {
            /* add result, t2, t3 */
            slsROPERAND_InitializeUsingIOperand(&rOperand0, &intermIOperands[2]);
            slsROPERAND_InitializeUsingIOperand(&rOperand1, &intermIOperands[3]);

            status = slGenArithmeticExprCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvOPCODE_ADD,
                                            IOperand,
                                            &rOperand0,
                                            &rOperand1);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            gcmASSERT(ROperand0->dataType == gcSHADER_FLOAT_4X4);

            /* add t4, t2, t3 */
            slsIOPERAND_New(Compiler, &intermIOperands[4], ROperand1->dataType);
            slsROPERAND_InitializeUsingIOperand(&rOperand0, &intermIOperands[2]);
            slsROPERAND_InitializeUsingIOperand(&rOperand1, &intermIOperands[3]);

            status = slGenArithmeticExprCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvOPCODE_ADD,
                                            &intermIOperands[4],
                                            &rOperand0,
                                            &rOperand1);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* mul t5, m[3], v.w */
            slsIOPERAND_New(Compiler, &intermIOperands[5], ROperand1->dataType);
            slsROPERAND_InitializeAsMatrixColumn(&rOperand0, ROperand0, 3);
            slsROPERAND_InitializeAsVectorComponent(&rOperand1, ROperand1, 3);

            status = slGenArithmeticExprCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvOPCODE_MUL,
                                            &intermIOperands[5],
                                            &rOperand0,
                                            &rOperand1);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* add result, t4, t5 */
            slsROPERAND_InitializeUsingIOperand(&rOperand0, &intermIOperands[4]);
            slsROPERAND_InitializeUsingIOperand(&rOperand1, &intermIOperands[5]);

            status = slGenArithmeticExprCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvOPCODE_ADD,
                                            IOperand,
                                            &rOperand0,
                                            &rOperand1);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* CODE_GENERATOR */
gceSTATUS
slsGEN_CODE_PARAMETERS_AllocateOperands(
    IN sloCOMPILER Compiler,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters,
    IN slsDATA_TYPE * DataType
    )
{
    gceSTATUS   status;
    gctUINT     start = 0;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Parameters);
    gcmASSERT(DataType);

    gcmASSERT(Parameters->operandCount == 0);
    gcmASSERT(Parameters->needLOperand || Parameters->needROperand);

    Parameters->operandCount = _GetLogicalOperandCount(DataType);
    gcmASSERT(Parameters->operandCount > 0);

    status = sloCOMPILER_Allocate(
                                Compiler,
                                (gctSIZE_T)sizeof(gcSHADER_TYPE) * Parameters->operandCount,
                                &pointer);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    Parameters->dataTypes = pointer;

    status = _ConvDataType(
                        DataType,
                        Parameters->dataTypes,
                        &start);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmASSERT(start == Parameters->operandCount);

    if (Parameters->needLOperand)
    {
        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsLOPERAND) * Parameters->operandCount,
                                    &pointer);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        Parameters->lOperands = pointer;
    }

    if (Parameters->needROperand)
    {
        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsROPERAND) * Parameters->operandCount,
                                    &pointer);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        Parameters->rOperands = pointer;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#define MAX_NAME_TABLE_SIZE         10
#define INVALID_VECTOR_INDEX        (0xffffffff)
#define MAX_LEVEL                   (0xffffffff)

typedef struct _slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS
{
    gctBOOL             usingSingleVectorIndex;

    gctUINT             vectorIndex;

    gctUINT             currentLevel;

    gctUINT             maxLevel;

    gctUINT             inputNameCount;

    slsNAME *           inputNameTable[MAX_NAME_TABLE_SIZE];

    gctUINT             outputNameCount;

    slsNAME *           outputNameTable[MAX_NAME_TABLE_SIZE];

    gctUINT             outputNameLevelTable[MAX_NAME_TABLE_SIZE];
}
slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS;

#define slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS_Initialize(parameters) \
    do \
    { \
        (parameters)->usingSingleVectorIndex    = gcvTRUE; \
        (parameters)->vectorIndex               = INVALID_VECTOR_INDEX; \
        (parameters)->currentLevel              = 0; \
        (parameters)->maxLevel                  = 0; \
        (parameters)->inputNameCount            = 0; \
        (parameters)->outputNameCount           = 0; \
    } \
    while (gcvFALSE)

static gctBOOL
_IsNameListEqual(
    IN gctUINT NameCount0,
    IN slsNAME * NameTable0[MAX_NAME_TABLE_SIZE],
    IN gctUINT NameCount1,
    IN slsNAME * NameTable1[MAX_NAME_TABLE_SIZE]
    )
{
    gctUINT     i;

    /* Verify the arguments. */
    gcmASSERT(NameTable0);
    gcmASSERT(NameTable1);

    if (NameCount0 != NameCount1)   return gcvFALSE;

    for (i = 0; i < NameCount0; i++)
    {
        if (NameTable0[i] != NameTable1[i]) return gcvFALSE;
    }

    return gcvTRUE;
}

static gctUINT
_FindNameInList(
    IN slsNAME * Name,
    IN gctUINT NameCount,
    IN slsNAME * NameTable[MAX_NAME_TABLE_SIZE]
    )
{
    gctUINT     i;

    /* Verify the arguments. */
    gcmASSERT(Name);
    gcmASSERT(NameTable);

    for (i = 0; i < NameCount; i++)
    {
        if (NameTable[i] == Name) return i;
    }

    return MAX_NAME_TABLE_SIZE;
}

static gceSTATUS
_AddNameToList(
    IN slsNAME * Name,
    IN gctUINT Level,
    IN OUT gctUINT * NameCount,
    IN OUT slsNAME * NameTable[MAX_NAME_TABLE_SIZE],
    IN OUT gctUINT NameLevelTable[MAX_NAME_TABLE_SIZE]
    )
{
    gctUINT     i;

    gcmHEADER();

    /* Verify the arguments. */
    gcmASSERT(Name);
    gcmASSERT(NameCount);
    gcmASSERT(NameTable);

    for (i = 0; i < *NameCount; i++)
    {
        if (Name == NameTable[i])
        {
            if (NameLevelTable != gcvNULL && Level < NameLevelTable[i])
            {
                NameLevelTable[i] = Level;
            }

            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }

    if (*NameCount == MAX_NAME_TABLE_SIZE)
    {
        gceSTATUS status = gcvSTATUS_BUFFER_TOO_SMALL;
        gcmFOOTER();
        return status;
    }

    NameTable[*NameCount]   = Name;

    if (NameLevelTable != gcvNULL) NameLevelTable[*NameCount] = Level;

    (*NameCount)++;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_AddNameToParameters(
    IN sloCOMPILER Compiler,
    IN slsNAME * Name,
    IN gctUINT VectorIndex,
    IN gctBOOL NeedLValue,
    IN gctBOOL NeedRValue,
    IN OUT slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS * Parameters
    )
{
    gceSTATUS   status;
    gctUINT     nameIndex;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Name);
    gcmASSERT(Parameters);

    do
    {
        if (Parameters->vectorIndex == INVALID_VECTOR_INDEX)
        {
            Parameters->vectorIndex = VectorIndex;
        }
        else
        {
            if (Parameters->vectorIndex != VectorIndex) break;
        }

        if (NeedRValue)
        {
            nameIndex = _FindNameInList(
                                        Name,
                                        Parameters->outputNameCount,
                                        Parameters->outputNameTable);

            if (nameIndex == MAX_NAME_TABLE_SIZE
                || Parameters->currentLevel < Parameters->outputNameLevelTable[nameIndex])
            {
                status = _AddNameToList(
                                        Name,
                                        Parameters->currentLevel,
                                        &Parameters->inputNameCount,
                                        Parameters->inputNameTable,
                                        gcvNULL);

                if (gcmIS_ERROR(status)) break;
            }
        }

        if (NeedLValue)
        {
            status = _AddNameToList(
                                    Name,
                                    Parameters->currentLevel,
                                    &Parameters->outputNameCount,
                                    Parameters->outputNameTable,
                                    Parameters->outputNameLevelTable);

            if (gcmIS_ERROR(status)) break;
        }

        Parameters->usingSingleVectorIndex = gcvTRUE;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    Parameters->usingSingleVectorIndex = gcvFALSE;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BASE_UsingSingleVectorIndex(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE Base,
    IN gctBOOL NeedLValue,
    IN gctBOOL NeedRValue,
    IN OUT slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    sloIR_SET               set             = gcvNULL;
    sloIR_VARIABLE          variable        = gcvNULL;
    sloIR_CONSTANT          constant        = gcvNULL;
    sloIR_UNARY_EXPR        unaryExpr       = gcvNULL;
    sloIR_BINARY_EXPR       binaryExpr      = gcvNULL;
    sloIR_SELECTION         selection       = gcvNULL;
    sloIR_POLYNARY_EXPR     polynaryExpr    = gcvNULL;
    sloIR_BASE              member;
    gctBOOL                 needLValue0 = gcvFALSE;
    gctBOOL                 needLValue1 = gcvFALSE;
    gctBOOL                 needRValue0 = gcvFALSE;
    gctBOOL                 needRValue1 = gcvFALSE;
    slsNAME *               name;
    gctUINT                 i, vectorIndex;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Base);
    gcmASSERT(Parameters);

    switch (sloIR_OBJECT_GetType(Base))
    {
    case slvIR_SET:
        set = (sloIR_SET)Base;

        FOR_EACH_DLINK_NODE(&set->members, struct _sloIR_BASE, member)
        {
            /* Check all members */
            status = sloIR_BASE_UsingSingleVectorIndex(
                                                        Compiler,
                                                        member,
                                                        gcvFALSE,
                                                        NeedRValue,
                                                        Parameters);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            if (!Parameters->usingSingleVectorIndex) break;
        }
        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_VARIABLE:
        variable = (sloIR_VARIABLE)Base;

        Parameters->usingSingleVectorIndex =
                    slsDATA_TYPE_IsScalar(variable->exprBase.dataType);
        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_CONSTANT:
        constant = (sloIR_CONSTANT)Base;

        Parameters->usingSingleVectorIndex =
                    slsDATA_TYPE_IsScalar(constant->exprBase.dataType);
        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_UNARY_EXPR:
        unaryExpr = (sloIR_UNARY_EXPR)Base;
        gcmASSERT(unaryExpr->operand);

        switch (unaryExpr->type)
        {
        case slvUNARY_FIELD_SELECTION:
            Parameters->usingSingleVectorIndex =
                    slsDATA_TYPE_IsScalar(unaryExpr->exprBase.dataType);
            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        case slvUNARY_COMPONENT_SELECTION:
            if (!slsDATA_TYPE_IsScalar(unaryExpr->exprBase.dataType)
                || sloIR_OBJECT_GetType(&unaryExpr->operand->base) != slvIR_VARIABLE)
            {
                Parameters->usingSingleVectorIndex = gcvFALSE;
                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }

            name = ((sloIR_VARIABLE)unaryExpr->operand)->name;
            gcmASSERT(name);

            vectorIndex = unaryExpr->u.componentSelection.x;

            status = _AddNameToParameters(
                                        Compiler,
                                        name,
                                        vectorIndex,
                                        NeedLValue,
                                        NeedRValue,
                                        Parameters);
            gcmFOOTER();
            return status;

        case slvUNARY_POST_INC:
        case slvUNARY_POST_DEC:
        case slvUNARY_PRE_INC:
        case slvUNARY_PRE_DEC:
            needLValue0 = gcvTRUE;
            needRValue0 = gcvTRUE;
            break;

        case slvUNARY_NEG:

        case slvUNARY_NOT:
            needLValue0 = gcvFALSE;
            needRValue0 = NeedRValue;
            break;

        default: gcmASSERT(0);
        }

        /* Check the operand */
        status = sloIR_BASE_UsingSingleVectorIndex(
                                                    Compiler,
                                                    &unaryExpr->operand->base,
                                                    needLValue0,
                                                    needRValue0,
                                                    Parameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_BINARY_EXPR:
        binaryExpr = (sloIR_BINARY_EXPR)Base;
        gcmASSERT(binaryExpr->leftOperand);
        gcmASSERT(binaryExpr->rightOperand);

        switch (binaryExpr->type)
        {
        case slvBINARY_SUBSCRIPT:
            if (!slsDATA_TYPE_IsBVecOrIVecOrVec(binaryExpr->leftOperand->dataType)
                || sloIR_OBJECT_GetType(&binaryExpr->leftOperand->base) != slvIR_VARIABLE
                || sloIR_OBJECT_GetType(&binaryExpr->rightOperand->base) != slvIR_CONSTANT)
            {
                Parameters->usingSingleVectorIndex = gcvFALSE;
                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }

            name = ((sloIR_VARIABLE)binaryExpr->leftOperand)->name;
            gcmASSERT(name);

            gcmASSERT(((sloIR_CONSTANT)binaryExpr->rightOperand)->valueCount == 1);
            vectorIndex = ((sloIR_CONSTANT)binaryExpr->rightOperand)->values[0].intValue;

            status = _AddNameToParameters(
                                        Compiler,
                                        name,
                                        vectorIndex,
                                        NeedLValue,
                                        NeedRValue,
                                        Parameters);
            gcmFOOTER();
            return status;

        case slvBINARY_ADD:
        case slvBINARY_SUB:
        case slvBINARY_MUL:
        case slvBINARY_DIV:

        case slvBINARY_GREATER_THAN:
        case slvBINARY_LESS_THAN:
        case slvBINARY_GREATER_THAN_EQUAL:
        case slvBINARY_LESS_THAN_EQUAL:

        case slvBINARY_EQUAL:
        case slvBINARY_NOT_EQUAL:

        case slvBINARY_AND:
        case slvBINARY_OR:
        case slvBINARY_XOR:
            needLValue0 = gcvFALSE;
            needRValue0 = NeedRValue;
            needLValue1 = gcvFALSE;
            needRValue1 = NeedRValue;
            break;

        case slvBINARY_SEQUENCE:
            needLValue0 = gcvFALSE;
            needRValue0 = gcvFALSE;
            needLValue1 = gcvFALSE;
            needRValue1 = NeedRValue;
            break;

        case slvBINARY_ASSIGN:
            needLValue0 = gcvTRUE;
            needRValue0 = gcvFALSE;
            needLValue1 = gcvFALSE;
            needRValue1 = gcvTRUE;
            break;

        case slvBINARY_MUL_ASSIGN:
        case slvBINARY_DIV_ASSIGN:
        case slvBINARY_ADD_ASSIGN:
        case slvBINARY_SUB_ASSIGN:
            needLValue0 = gcvTRUE;
            needRValue0 = gcvTRUE;
            needLValue1 = gcvFALSE;
            needRValue1 = gcvTRUE;
            break;

        default:
            gcmASSERT(0);
        }

        /* Check the left operand */
        status = sloIR_BASE_UsingSingleVectorIndex(
                                                    Compiler,
                                                    &binaryExpr->leftOperand->base,
                                                    needLValue0,
                                                    needRValue0,
                                                    Parameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (!Parameters->usingSingleVectorIndex) { gcmFOOTER_NO(); return gcvSTATUS_OK; }

        /* Check the right operand */
        status = sloIR_BASE_UsingSingleVectorIndex(
                                                    Compiler,
                                                    &binaryExpr->rightOperand->base,
                                                    needLValue1,
                                                    needRValue1,
                                                    Parameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_SELECTION:
        selection = (sloIR_SELECTION)Base;
        gcmASSERT(selection->condExpr);

        /* Check the condition expression */
        status = sloIR_BASE_UsingSingleVectorIndex(
                                                    Compiler,
                                                    &selection->condExpr->base,
                                                    gcvFALSE,
                                                    gcvTRUE,
                                                    Parameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (!Parameters->usingSingleVectorIndex) { gcmFOOTER_NO(); return gcvSTATUS_OK; }

        Parameters->currentLevel++;
        Parameters->maxLevel++;

        /* Check the true operand */
        if (selection->trueOperand != gcvNULL)
        {
            status = sloIR_BASE_UsingSingleVectorIndex(
                                                        Compiler,
                                                        selection->trueOperand,
                                                        gcvFALSE,
                                                        NeedRValue,
                                                        Parameters);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            if (!Parameters->usingSingleVectorIndex) { gcmFOOTER_NO(); return gcvSTATUS_OK; }

            for (i = 0; i < Parameters->outputNameCount; i++)
            {
                if (Parameters->outputNameLevelTable[i] == i)
                {
                    Parameters->outputNameLevelTable[i] = MAX_LEVEL;
                }
            }
        }

        /* Check the false operand */
        if (selection->falseOperand != gcvNULL)
        {
            status = sloIR_BASE_UsingSingleVectorIndex(
                                                        Compiler,
                                                        selection->falseOperand,
                                                        gcvFALSE,
                                                        NeedRValue,
                                                        Parameters);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            if (!Parameters->usingSingleVectorIndex) { gcmFOOTER_NO(); return gcvSTATUS_OK; }

            for (i = 0; i < Parameters->outputNameCount; i++)
            {
                if (Parameters->outputNameLevelTable[i] == i)
                {
                    Parameters->outputNameLevelTable[i] = MAX_LEVEL;
                }
            }
        }

        Parameters->currentLevel--;
        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_POLYNARY_EXPR:
        polynaryExpr = (sloIR_POLYNARY_EXPR)Base;

        if (polynaryExpr->type == slvPOLYNARY_FUNC_CALL
            && !polynaryExpr->funcName->isBuiltIn)
        {
            Parameters->usingSingleVectorIndex = gcvFALSE;
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        if (polynaryExpr->operands != gcvNULL)
        {
            /* Check all operands */
            status = sloIR_BASE_UsingSingleVectorIndex(
                                                        Compiler,
                                                        &polynaryExpr->operands->base,
                                                        gcvFALSE,
                                                        gcvTRUE,
                                                        Parameters);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    default:
        Parameters->usingSingleVectorIndex = gcvFALSE;
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
}

gctBOOL
sloIR_BASE_IsEqualExceptVectorIndex(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE Base0,
    IN sloIR_BASE Base1
    )
{
    sloIR_SET               set0, set1;
    sloIR_VARIABLE          variable0, variable1;
    sloIR_CONSTANT          constant0, constant1;
    sloIR_UNARY_EXPR        unaryExpr0, unaryExpr1;
    sloIR_BINARY_EXPR       binaryExpr0, binaryExpr1;
    sloIR_SELECTION         selection0, selection1;
    sloIR_POLYNARY_EXPR     polynaryExpr0, polynaryExpr1;
    sloIR_BASE              member0, member1;
    gctUINT                 i;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Base0);
    gcmASSERT(Base1);

    if (sloIR_OBJECT_GetType(Base0) != sloIR_OBJECT_GetType(Base1)) { gcmFOOTER_NO(); return gcvFALSE; }

    switch (sloIR_OBJECT_GetType(Base0))
    {
    case slvIR_SET:
        set0 = (sloIR_SET)Base0;
        set1 = (sloIR_SET)Base1;

        if (set0->type != set1->type) { gcmFOOTER_NO(); return gcvFALSE; }

        for (member0 = slsDLINK_LIST_First(&set0->members, struct _sloIR_BASE),
                member1 = slsDLINK_LIST_First(&set1->members, struct _sloIR_BASE);
            (slsDLINK_NODE *)member0 != &set0->members
                && (slsDLINK_NODE *)member1 != &set1->members;
            member0 = slsDLINK_NODE_Next(&member0->node, struct _sloIR_BASE),
                member1 = slsDLINK_NODE_Next(&member1->node, struct _sloIR_BASE))
        {
            if (!sloIR_BASE_IsEqualExceptVectorIndex(
                                                    Compiler,
                                                    member0,
                                                     member1)) { gcmFOOTER_NO(); return gcvFALSE; }
        }

        if ((slsDLINK_NODE *)member0 != &set0->members
            && (slsDLINK_NODE *)member1 != &set1->members) { gcmFOOTER_NO(); return gcvFALSE; }

        gcmFOOTER_NO();
        return gcvTRUE;

    case slvIR_VARIABLE:
        variable0 = (sloIR_VARIABLE)Base0;
        variable1 = (sloIR_VARIABLE)Base1;

        gcmFOOTER_NO();
        return (variable0->name == variable1->name);

    case slvIR_CONSTANT:
        constant0 = (sloIR_CONSTANT)Base0;
        constant1 = (sloIR_CONSTANT)Base1;

            if (constant0->valueCount != constant1->valueCount) { gcmFOOTER_NO(); return gcvFALSE; }

        for (i = 0; i < constant0->valueCount; i++)
        {
            if (constant0->values[i].intValue != constant1->values[i].intValue) { gcmFOOTER_NO(); return gcvFALSE; }
        }

        gcmFOOTER_NO(); return gcvTRUE;

    case slvIR_UNARY_EXPR:
        unaryExpr0 = (sloIR_UNARY_EXPR)Base0;
        unaryExpr1 = (sloIR_UNARY_EXPR)Base1;

            if (unaryExpr0->type != unaryExpr1->type) { gcmFOOTER_NO(); return gcvFALSE; }

        if (unaryExpr0->type == slvUNARY_FIELD_SELECTION
            && unaryExpr0->u.fieldName != unaryExpr1->u.fieldName) { gcmFOOTER_NO(); return gcvFALSE; }

        gcmFOOTER_NO();
        return sloIR_BASE_IsEqualExceptVectorIndex(
                                                Compiler,
                                                &unaryExpr0->operand->base,
                                                &unaryExpr1->operand->base);

    case slvIR_BINARY_EXPR:
        binaryExpr0 = (sloIR_BINARY_EXPR)Base0;
        binaryExpr1 = (sloIR_BINARY_EXPR)Base1;

        if (!sloIR_BASE_IsEqualExceptVectorIndex(
                                                Compiler,
                                                &binaryExpr0->leftOperand->base,
                                                 &binaryExpr1->leftOperand->base)) { gcmFOOTER_NO(); return gcvFALSE; }

        if (binaryExpr0->type != slvBINARY_SUBSCRIPT)
        {
            if (!sloIR_BASE_IsEqualExceptVectorIndex(
                                                    Compiler,
                                                    &binaryExpr0->rightOperand->base,
                                                    &binaryExpr1->rightOperand->base))
            {
                gcmFOOTER_NO(); return gcvFALSE;
            }
        }

        gcmFOOTER_NO(); return gcvTRUE;

    case slvIR_SELECTION:
        selection0 = (sloIR_SELECTION)Base0;
        selection1 = (sloIR_SELECTION)Base1;

        /* Check the condition expression */
        if (!sloIR_BASE_IsEqualExceptVectorIndex(
                                                Compiler,
                                                &selection0->condExpr->base,
                                                 &selection1->condExpr->base)) { gcmFOOTER_NO(); return gcvFALSE; }

        /* Check the true operand */
        if (selection0->trueOperand != gcvNULL)
        {
            if (selection1->trueOperand == gcvNULL) { gcmFOOTER_NO(); return gcvFALSE; }

            if (!sloIR_BASE_IsEqualExceptVectorIndex(
                                                    Compiler,
                                                    selection0->trueOperand,
                                                     selection1->trueOperand)) { gcmFOOTER_NO(); return gcvFALSE; }
        }
        else
        {
            if (selection1->trueOperand != gcvNULL) { gcmFOOTER_NO(); return gcvFALSE; }
        }

        /* Check the false operand */
        if (selection0->falseOperand != gcvNULL)
        {
            if (selection1->falseOperand == gcvNULL) { gcmFOOTER_NO(); return gcvFALSE; }

            if (!sloIR_BASE_IsEqualExceptVectorIndex(
                                                    Compiler,
                                                    selection0->falseOperand,
                                                     selection1->falseOperand)) { gcmFOOTER_NO(); return gcvFALSE; }
        }
        else
        {
            if (selection1->falseOperand != gcvNULL) { gcmFOOTER_NO(); return gcvFALSE; }
        }

        gcmFOOTER_NO(); return gcvTRUE;

    case slvIR_POLYNARY_EXPR:
        polynaryExpr0 = (sloIR_POLYNARY_EXPR)Base0;
        polynaryExpr1 = (sloIR_POLYNARY_EXPR)Base1;

            if (polynaryExpr0->type != polynaryExpr1->type) { gcmFOOTER_NO(); return gcvFALSE; }

        if (polynaryExpr0->operands != gcvNULL)
        {
            if (polynaryExpr1->operands == gcvNULL) { gcmFOOTER_NO(); return gcvFALSE; }

            if (!sloIR_BASE_IsEqualExceptVectorIndex(
                                                    Compiler,
                                                    &polynaryExpr0->operands->base,
                                                    &polynaryExpr1->operands->base))
            {
                gcmFOOTER_NO(); return gcvFALSE;
            }
        }
        else
        {
            if (polynaryExpr1->operands != gcvNULL) { gcmFOOTER_NO(); return gcvFALSE; }
        }

        gcmFOOTER_NO(); return gcvTRUE;

    default:
        gcmFOOTER_NO(); return gcvFALSE;
    }
}

typedef struct _slsCOMPARE_ALL_NAMES_COMPONENT_PARAMETERS
{
    gctUINT         nameCount;

    slsNAME * *     nameTable;

    gctUINT         vectorIndex0;

    gctUINT         vectorIndex1;

    gctBOOL         compareResults[MAX_NAME_TABLE_SIZE];
}
slsCOMPARE_ALL_NAMES_COMPONENT_PARAMETERS;

#define slsCOMPARE_ALL_NAMES_COMPONENT_PARAMETERS_Initialize( \
                                                            parameters, \
                                                            _nameCount, \
                                                            _nameTable, \
                                                            _vectorIndex0, \
                                                            _vectorIndex1) \
    do \
    { \
        gctUINT _i; \
        \
        (parameters)->nameCount     = (_nameCount); \
        (parameters)->nameTable     = (_nameTable); \
        (parameters)->vectorIndex0  = (_vectorIndex0); \
        (parameters)->vectorIndex1  = (_vectorIndex1); \
        \
        for (_i = 0; _i < (_nameCount); _i++) \
        { \
            (parameters)->compareResults[_i] = gcvFALSE; \
        } \
    } \
    while (gcvFALSE)

#define slsCOMPARE_ALL_NAMES_COMPONENT_PARAMETERS_ClearResults(parameters) \
    do \
    { \
        gctUINT _i; \
        \
        for (_i = 0; _i < (parameters)->nameCount; _i++) \
        { \
            (parameters)->compareResults[_i] = gcvFALSE; \
        } \
    } \
    while (gcvFALSE)

gctBOOL
sloIR_BASE_CompareAllNamesComponent(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE Base,
    IN OUT slsCOMPARE_ALL_NAMES_COMPONENT_PARAMETERS * Parameters,
    OUT gctBOOL * NeedClearResults
    )
{
    sloIR_VARIABLE          variable        = gcvNULL;
    sloIR_CONSTANT          constant        = gcvNULL;
    sloIR_UNARY_EXPR        unaryExpr       = gcvNULL;
    sloIR_BINARY_EXPR       binaryExpr      = gcvNULL;
    sloIR_POLYNARY_EXPR     polynaryExpr    = gcvNULL;
    gctUINT8                components[4];
    slsNAME *               name;
    gctUINT                 nameIndex;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Base);
    gcmASSERT(Parameters);
    gcmASSERT(NeedClearResults);

    *NeedClearResults = gcvFALSE;

    switch (sloIR_OBJECT_GetType(Base))
    {
    case slvIR_VARIABLE:
        variable = (sloIR_VARIABLE)Base;

        gcmFOOTER_NO();
        return slsDATA_TYPE_IsScalar(variable->exprBase.dataType);

    case slvIR_CONSTANT:
        constant = (sloIR_CONSTANT)Base;

            if (slsDATA_TYPE_IsScalar(constant->exprBase.dataType)) { gcmFOOTER_NO(); return gcvTRUE; }

        if (slsDATA_TYPE_IsBVecOrIVecOrVec(constant->exprBase.dataType))
        {
            if (slmDATA_TYPE_vectorSize_NOCHECK_GET(constant->exprBase.dataType) > Parameters->vectorIndex0
                && slmDATA_TYPE_vectorSize_NOCHECK_GET(constant->exprBase.dataType) > Parameters->vectorIndex1)
            {
                gcmFOOTER_NO();
                return (constant->values[Parameters->vectorIndex0].intValue
                            == constant->values[Parameters->vectorIndex1].intValue);
            }
        }
        gcmFOOTER_NO();
        return gcvFALSE;

    case slvIR_UNARY_EXPR:
        unaryExpr = (sloIR_UNARY_EXPR)Base;
        gcmASSERT(unaryExpr->operand);

            if (slsDATA_TYPE_IsScalar(unaryExpr->exprBase.dataType)) { gcmFOOTER_NO(); return gcvTRUE; }

        switch (unaryExpr->type)
        {
        case slvUNARY_FIELD_SELECTION:
            gcmFOOTER_NO();
            return gcvFALSE;

        case slvUNARY_COMPONENT_SELECTION:
            components[0] = unaryExpr->u.componentSelection.x;
            components[1] = unaryExpr->u.componentSelection.y;
            components[2] = unaryExpr->u.componentSelection.z;
            components[3] = unaryExpr->u.componentSelection.w;
            gcmFOOTER_NO();
            return (components[Parameters->vectorIndex0] == components[Parameters->vectorIndex1]);

        case slvUNARY_POST_INC:
        case slvUNARY_POST_DEC:
        case slvUNARY_PRE_INC:
        case slvUNARY_PRE_DEC:

        case slvUNARY_NEG:

        case slvUNARY_NOT:
            gcmFOOTER_NO();
            return sloIR_BASE_CompareAllNamesComponent(
                                                        Compiler,
                                                        &unaryExpr->operand->base,
                                                        Parameters,
                                                        NeedClearResults);

        default:
            gcmASSERT(0);
            gcmFOOTER_NO();
            return gcvFALSE;
        }

    case slvIR_BINARY_EXPR:
        binaryExpr = (sloIR_BINARY_EXPR)Base;
        gcmASSERT(binaryExpr->leftOperand);
        gcmASSERT(binaryExpr->rightOperand);

            if (slsDATA_TYPE_IsScalar(binaryExpr->exprBase.dataType)) { gcmFOOTER_NO(); return gcvTRUE; }

        switch (binaryExpr->type)
        {
        case slvBINARY_SUBSCRIPT:
            gcmFOOTER_NO();
            return gcvFALSE;

        case slvBINARY_ADD:
        case slvBINARY_SUB:
        case slvBINARY_MUL:
        case slvBINARY_DIV:

            if (!sloIR_BASE_CompareAllNamesComponent(
                                                    Compiler,
                                                    &binaryExpr->leftOperand->base,
                                                    Parameters,
                                                     NeedClearResults)) { gcmFOOTER_NO(); return gcvFALSE; }

            if (!sloIR_BASE_CompareAllNamesComponent(
                                                    Compiler,
                                                    &binaryExpr->rightOperand->base,
                                                    Parameters,
                                                     NeedClearResults)) { gcmFOOTER_NO(); return gcvFALSE; }

            gcmFOOTER_NO();
            return gcvTRUE;

        case slvBINARY_GREATER_THAN:
        case slvBINARY_LESS_THAN:
        case slvBINARY_GREATER_THAN_EQUAL:
        case slvBINARY_LESS_THAN_EQUAL:

        case slvBINARY_EQUAL:
        case slvBINARY_NOT_EQUAL:

        case slvBINARY_AND:
        case slvBINARY_OR:
        case slvBINARY_XOR:
            gcmFOOTER_NO();
            return gcvTRUE;

        case slvBINARY_SEQUENCE:
            if (!sloIR_BASE_CompareAllNamesComponent(
                                                    Compiler,
                                                    &binaryExpr->rightOperand->base,
                                                    Parameters,
                                                     NeedClearResults)) { gcmFOOTER_NO(); return gcvFALSE; }

            gcmFOOTER_NO();
            return gcvTRUE;

        case slvBINARY_ASSIGN:

        case slvBINARY_MUL_ASSIGN:
        case slvBINARY_DIV_ASSIGN:
        case slvBINARY_ADD_ASSIGN:
        case slvBINARY_SUB_ASSIGN:

            if (binaryExpr->type != slvBINARY_ASSIGN
                && !sloIR_BASE_CompareAllNamesComponent(
                                                        Compiler,
                                                        &binaryExpr->leftOperand->base,
                                                        Parameters,
                                                        NeedClearResults)) { gcmFOOTER_NO(); return gcvFALSE; }

            if (sloIR_OBJECT_GetType(&binaryExpr->leftOperand->base) == slvIR_VARIABLE)
            {
                name = ((sloIR_VARIABLE)binaryExpr->leftOperand)->name;
                gcmASSERT(name);

                nameIndex = _FindNameInList(
                                            name,
                                            Parameters->nameCount,
                                            Parameters->nameTable);

                if (nameIndex != MAX_NAME_TABLE_SIZE)
                {
                    Parameters->compareResults[nameIndex] =
                            sloIR_BASE_CompareAllNamesComponent(
                                                                Compiler,
                                                                &binaryExpr->rightOperand->base,
                                                                Parameters,
                                                                NeedClearResults);

                    gcmFOOTER_NO();
                    return Parameters->compareResults[nameIndex];
                }
            }

            gcmFOOTER_NO();
            return sloIR_BASE_CompareAllNamesComponent(
                                                    Compiler,
                                                    &binaryExpr->rightOperand->base,
                                                    Parameters,
                                                    NeedClearResults);

        default:
            gcmASSERT(0);
            gcmFOOTER_NO();
            return gcvFALSE;
        }

    case slvIR_POLYNARY_EXPR:
        polynaryExpr = (sloIR_POLYNARY_EXPR)Base;

        if (polynaryExpr->type == slvPOLYNARY_FUNC_CALL
            && !polynaryExpr->funcName->isBuiltIn)
        {
            *NeedClearResults = gcvTRUE;
            gcmFOOTER_NO();
            return gcvFALSE;
        }

        gcmFOOTER_NO();
        return slsDATA_TYPE_IsScalar(polynaryExpr->exprBase.dataType);

    default:
        *NeedClearResults = gcvTRUE;
        gcmFOOTER_NO();
        return gcvFALSE;
    }
}

gctBOOL
sloIR_SET_CompareAllNamesComponent(
    IN sloCOMPILER Compiler,
    IN sloIR_SET StatementSet,
    IN sloIR_BASE StopStatement,
    IN gctUINT NameCount,
    IN slsNAME * NameTable[MAX_NAME_TABLE_SIZE],
    IN gctUINT VectorIndex0,
    IN gctUINT VectorIndex1
    )
{
    slsCOMPARE_ALL_NAMES_COMPONENT_PARAMETERS   parameters;
    gctUINT                                     i;
    sloIR_BASE                                  statement;
    gctBOOL                                     needClearResults;

    slsCOMPARE_ALL_NAMES_COMPONENT_PARAMETERS_Initialize(
                                                        &parameters,
                                                        NameCount,
                                                        NameTable,
                                                        VectorIndex0,
                                                        VectorIndex1);

    FOR_EACH_DLINK_NODE(&StatementSet->members, struct _sloIR_BASE, statement)
    {
        if (statement == StopStatement) break;

        sloIR_BASE_CompareAllNamesComponent(Compiler, statement, &parameters, &needClearResults);

        if (needClearResults)
        {
            slsCOMPARE_ALL_NAMES_COMPONENT_PARAMETERS_ClearResults(&parameters);
        }
    }

    for (i = 0; i < parameters.nameCount; i++)
    {
        if (!parameters.compareResults[i]) return gcvFALSE;
    }

    return gcvTRUE;
}

typedef struct _slsSPECIAL_STATEMENT_CONTEXT
{
    gctBOOL                                     codeGenerated;

    sloIR_BASE                                  prevStatement;

    slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS     prevParameters;
}
slsSPECIAL_STATEMENT_CONTEXT;

#define slsSPECIAL_STATEMENT_CONTEXT_Initialize(context) \
    do \
    { \
        (context)->codeGenerated    = gcvFALSE; \
        (context)->prevStatement    = gcvNULL; \
        slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS_Initialize(&(context)->prevParameters); \
    } \
    while (gcvFALSE)

gceSTATUS
sloIR_SET_TryToGenSpecialStatementCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_SET StatementSet,
    IN sloIR_BASE Statement,
    IN OUT slsSPECIAL_STATEMENT_CONTEXT * StatementContext
    )
{
    gceSTATUS                                   status;
    slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS     parameters;
    gctUINT                                     i;
    slsNAME *                                   name;
    slsLOPERAND                                 lOperand, componentLOperand;
    slsROPERAND                                 rOperand, componentROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(StatementSet, slvIR_SET);
    gcmASSERT(Statement);
    gcmASSERT(StatementContext);

    do
    {
        /* Check the current statement */
        slsUSING_SINGLE_VECTOR_INDEX_PARAMETERS_Initialize(&parameters);

        status = sloIR_BASE_UsingSingleVectorIndex(
                                                Compiler,
                                                Statement,
                                                gcvFALSE,
                                                gcvFALSE,
                                                &parameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (!parameters.usingSingleVectorIndex
            || parameters.vectorIndex == INVALID_VECTOR_INDEX) break;

        if (parameters.maxLevel == 0) break;

        /* Check the previous statement */
        if (StatementContext->prevStatement == gcvNULL) break;

        gcmASSERT(StatementContext->prevParameters.usingSingleVectorIndex);
        gcmASSERT(StatementContext->prevParameters.vectorIndex != INVALID_VECTOR_INDEX);

        /* Compare the results */
        if (parameters.vectorIndex == StatementContext->prevParameters.vectorIndex) break;

        if (!_IsNameListEqual(
                            parameters.inputNameCount,
                            parameters.inputNameTable,
                            StatementContext->prevParameters.inputNameCount,
                            StatementContext->prevParameters.inputNameTable)) break;

        if (!_IsNameListEqual(
                            parameters.outputNameCount,
                            parameters.outputNameTable,
                            StatementContext->prevParameters.outputNameCount,
                            StatementContext->prevParameters.outputNameTable)) break;

        /* Compare the statements */
        if (!sloIR_BASE_IsEqualExceptVectorIndex(
                                                Compiler,
                                                Statement,
                                                StatementContext->prevStatement)) break;

        /* Check all input names */
        if (!sloIR_SET_CompareAllNamesComponent(
                                                Compiler,
                                                StatementSet,
                                                StatementContext->prevStatement,
                                                parameters.inputNameCount,
                                                parameters.inputNameTable,
                                                StatementContext->prevParameters.vectorIndex,
                                                parameters.vectorIndex)) break;

        /* Generate the special assign code */
        for (i = 0; i < parameters.outputNameCount; i++)
        {
            name = parameters.outputNameTable[i];
            gcmASSERT(slsDATA_TYPE_IsBVecOrIVecOrVec(name->dataType));
            gcmASSERT(name->context.logicalRegCount == 1);

            slsLOPERAND_Initialize(&lOperand, &name->context.logicalRegs[0]);
            slsROPERAND_InitializeReg(&rOperand, &name->context.logicalRegs[0]);

            slsLOPERAND_InitializeAsVectorComponent(
                                                    &componentLOperand,
                                                    &lOperand,
                                                    parameters.vectorIndex);
            slsROPERAND_InitializeAsVectorComponent(
                                                    &componentROperand,
                                                    &rOperand,
                                                    StatementContext->prevParameters.vectorIndex);

            status = slGenAssignCode(
                                    Compiler,
                                    Statement->lineNo,
                                    Statement->stringNo,
                                    &componentLOperand,
                                    &componentROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }

        StatementContext->codeGenerated = gcvTRUE;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    StatementContext->codeGenerated = gcvFALSE;

    if (parameters.usingSingleVectorIndex
        && parameters.vectorIndex != INVALID_VECTOR_INDEX)
    {
        StatementContext->prevStatement     = Statement;
        StatementContext->prevParameters    = parameters;
    }
    else
    {
        StatementContext->prevStatement = gcvNULL;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gctBOOL
sloIR_BASE_HasReturn(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE Statement
    )
{
    sloIR_SELECTION     selection;
    sloIR_SET           set;
    sloIR_BASE          member;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Statement);

    switch (sloIR_OBJECT_GetType(Statement))
    {
    case slvIR_JUMP:
        gcmFOOTER_NO();
        return (((sloIR_JUMP)Statement)->type == slvRETURN);

    case slvIR_SELECTION:
        selection = (sloIR_SELECTION)Statement;

        if (selection->trueOperand == gcvNULL
            || selection->falseOperand == gcvNULL) { gcmFOOTER_NO(); return gcvFALSE; }

        gcmFOOTER_NO();
        return sloIR_BASE_HasReturn(Compiler, selection->trueOperand)
                && sloIR_BASE_HasReturn(Compiler, selection->falseOperand);

    case slvIR_SET:
        set = (sloIR_SET)Statement;

            if (set->type != slvSTATEMENT_SET) { gcmFOOTER_NO(); return gcvFALSE; }

        FOR_EACH_DLINK_NODE(&set->members, struct _sloIR_BASE, member)
        {
            if (sloIR_BASE_HasReturn(Compiler, member)) { gcmFOOTER_NO(); return gcvTRUE; }
        }

        gcmFOOTER_NO();
        return gcvFALSE;

    default:
        gcmFOOTER_NO();
        return gcvFALSE;
    }
}

gceSTATUS
sloIR_SET_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_SET Set,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS                       status;
    gctBOOL                         isRoot;
    sloIR_BASE                      member;
    slsGEN_CODE_PARAMETERS          memberParameters;
    gctBOOL                         hasReturn = gcvFALSE;
    slsSPECIAL_STATEMENT_CONTEXT    specialStatementContext;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(Set, slvIR_SET);
    gcmASSERT(Parameters);

    switch (Set->type)
    {
    case slvDECL_SET:
        gcmVERIFY_OK(sloIR_SET_IsRoot(Compiler, Set, &isRoot));

        if (isRoot)
        {
            /* Generate the initalization code of all global variables */
            FOR_EACH_DLINK_NODE(&Set->members, struct _sloIR_BASE, member)
            {
                if (sloIR_OBJECT_GetType(member) == slvIR_BINARY_EXPR)
                {
                    slsGEN_CODE_PARAMETERS_Initialize(&memberParameters, gcvFALSE, gcvFALSE);

                    status = sloIR_OBJECT_Accept(
                                                Compiler,
                                                member,
                                                &CodeGenerator->visitor,
                                                &memberParameters);

                    slsGEN_CODE_PARAMETERS_Finalize(&memberParameters);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
            }
        }

        FOR_EACH_DLINK_NODE(&Set->members, struct _sloIR_BASE, member)
        {
            if (!isRoot || sloIR_OBJECT_GetType(member) != slvIR_BINARY_EXPR)
            {
                slsGEN_CODE_PARAMETERS_Initialize(&memberParameters, gcvFALSE, gcvFALSE);

                status = sloIR_OBJECT_Accept(
                                            Compiler,
                                            member,
                                            &CodeGenerator->visitor,
                                            &memberParameters);

                slsGEN_CODE_PARAMETERS_Finalize(&memberParameters);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
        }
        break;

    case slvSTATEMENT_SET:
        if (Set->funcName != gcvNULL)
        {
            status = _DefineFuncBegin(Compiler, CodeGenerator, Set);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            hasReturn = gcvFALSE;
        }

        slsSPECIAL_STATEMENT_CONTEXT_Initialize(&specialStatementContext);

        FOR_EACH_DLINK_NODE(&Set->members, struct _sloIR_BASE, member)
        {
            if (Set->funcName != gcvNULL)
            {
                if (sloIR_BASE_HasReturn(Compiler, member))
                {
                    hasReturn = gcvTRUE;
                }
            }

            if (sloCOMPILER_OptimizationEnabled(Compiler, slvOPTIMIZATION_SPECIAL))
            {
                status = sloIR_SET_TryToGenSpecialStatementCode(
                                                                Compiler,
                                                                CodeGenerator,
                                                                Set,
                                                                member,
                                                                &specialStatementContext);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                if (specialStatementContext.codeGenerated) continue;
            }

            slsGEN_CODE_PARAMETERS_Initialize(&memberParameters, gcvFALSE, gcvFALSE);

            status = sloIR_OBJECT_Accept(
                                        Compiler,
                                        member,
                                        &CodeGenerator->visitor,
                                        &memberParameters);

            slsGEN_CODE_PARAMETERS_Finalize(&memberParameters);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }

        if (Set->funcName != gcvNULL)
        {
            status = _DefineFuncEnd(Compiler, CodeGenerator, Set, hasReturn);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        break;

    case slvEXPR_SET:
        gcmASSERT(0);
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineUnrolledIterationBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsNAME * LoopIndexName,
    OUT slsITERATION_CONTEXT * CurrentIterationContext
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(LoopIndexName);
    gcmASSERT(CurrentIterationContext);

    CurrentIterationContext->prevContext    = CodeGenerator->currentIterationContext;
    CodeGenerator->currentIterationContext  = CurrentIterationContext;

    CurrentIterationContext->isUnrolled     = gcvTRUE;

    CurrentIterationContext->u.unrolledInfo.loopIndexName   = LoopIndexName;

    CurrentIterationContext->endLabel       = slNewLabel(Compiler);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineUnrolledIterationEnd(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);
    gcmASSERT(CodeGenerator->currentIterationContext->isUnrolled);

    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        CodeGenerator->currentIterationContext->endLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    CodeGenerator->currentIterationContext = CodeGenerator->currentIterationContext->prevContext;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineUnrolledIterationBodyBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sluCONSTANT_VALUE LoopIndexValue
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);
    gcmASSERT(CodeGenerator->currentIterationContext->isUnrolled);

    CodeGenerator->currentIterationContext->u.unrolledInfo.loopIndexValue   = LoopIndexValue;
    CodeGenerator->currentIterationContext->u.unrolledInfo.bodyEndLabel     = slNewLabel(Compiler);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineUnrolledIterationBodyEnd(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);
    gcmASSERT(CodeGenerator->currentIterationContext->isUnrolled);

    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        CodeGenerator->currentIterationContext->u.unrolledInfo.bodyEndLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineIterationBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN gctBOOL IsTestFirst,
    IN gctBOOL HasRestExpr,
    OUT slsITERATION_CONTEXT * CurrentIterationContext
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(CurrentIterationContext);

    CurrentIterationContext->prevContext    = CodeGenerator->currentIterationContext;
    CodeGenerator->currentIterationContext  = CurrentIterationContext;

    CurrentIterationContext->isUnrolled     = gcvFALSE;

    CurrentIterationContext->u.genericInfo.isTestFirst      = IsTestFirst;
    CurrentIterationContext->u.genericInfo.hasRestExpr      = HasRestExpr;
    CurrentIterationContext->u.genericInfo.loopBeginLabel   = slNewLabel(Compiler);

	if (HasRestExpr)
    {
        CurrentIterationContext->u.genericInfo.restBeginLabel = slNewLabel(Compiler);
    }

    CurrentIterationContext->endLabel       = slNewLabel(Compiler);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineIterationEnd(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);
    gcmASSERT(!CodeGenerator->currentIterationContext->isUnrolled);

    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        CodeGenerator->currentIterationContext->endLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    CodeGenerator->currentIterationContext = CodeGenerator->currentIterationContext->prevContext;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineIterationRestExprBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);
    gcmASSERT(!CodeGenerator->currentIterationContext->isUnrolled);
    gcmASSERT(CodeGenerator->currentIterationContext->u.genericInfo.hasRestExpr);

    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        CodeGenerator->currentIterationContext->u.genericInfo.restBeginLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}


static gceSTATUS
_DefineIterationBodyBegin(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);
    gcmASSERT(!CodeGenerator->currentIterationContext->isUnrolled);

    if (!CodeGenerator->currentIterationContext->u.genericInfo.hasRestExpr)
    {
        status = slSetLabel(
                            Compiler,
                            0,
                            0,
                            CodeGenerator->currentIterationContext->u.genericInfo.loopBeginLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_DefineIterationBodyEnd(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);
    gcmASSERT(!CodeGenerator->currentIterationContext->isUnrolled);

    if (CodeGenerator->currentIterationContext->u.genericInfo.isTestFirst)
    {
        status = slEmitAlwaysBranchCode(
                                        Compiler,
                                        0,
                                        0,
                                        slvOPCODE_JUMP,
                                        CodeGenerator->currentIterationContext->
                                            u.genericInfo.loopBeginLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctLABEL
_GetIterationContinueLabel(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gctLABEL theLabel;

    gcmHEADER();
    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);
    slmASSERT_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);

    if (CodeGenerator->currentIterationContext->isUnrolled)
    {
        theLabel = CodeGenerator->currentIterationContext->u.unrolledInfo.bodyEndLabel;
    }
    else
    {
		if (CodeGenerator->currentIterationContext->u.genericInfo.hasRestExpr)
		{
            theLabel = CodeGenerator->currentIterationContext->u.genericInfo.restBeginLabel;
		}
		else {
            theLabel = CodeGenerator->currentIterationContext->u.genericInfo.loopBeginLabel;
		}
    }
    gcmFOOTER_NO();
	return theLabel;
}

static gctLABEL
_GetIterationEndLabel(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);
    slmASSERT_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);

    gcmASSERT(CodeGenerator->currentIterationContext);

    gcmFOOTER_NO();
    return CodeGenerator->currentIterationContext->endLabel;
}

static gctBOOL
_IsUnrolledLoopIndexRecursively(
    IN sloCOMPILER Compiler,
    IN slsITERATION_CONTEXT * IterationContext,
    IN slsNAME * Name,
    OUT sluCONSTANT_VALUE * UnrolledLoopIndexValue
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(IterationContext);
    gcmASSERT(Name);
    gcmASSERT(UnrolledLoopIndexValue);

    if (IterationContext->isUnrolled && Name == IterationContext->u.unrolledInfo.loopIndexName)
    {
        *UnrolledLoopIndexValue = IterationContext->u.unrolledInfo.loopIndexValue;
        gcmFOOTER_NO();
        return gcvTRUE;
    }

    if (IterationContext->prevContext == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvFALSE;
    }
    else
    {
        gcmFOOTER_NO();
        return _IsUnrolledLoopIndexRecursively(
                                            Compiler,
                                            IterationContext->prevContext,
                                            Name,
                                            UnrolledLoopIndexValue);
    }
}

static gctBOOL
_IsUnrolledLoopIndex(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsNAME * Name,
    OUT sluCONSTANT_VALUE * UnrolledLoopIndexValue
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    gcmASSERT(Name);
    gcmASSERT(UnrolledLoopIndexValue);

    if (CodeGenerator->currentIterationContext == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvFALSE;
    }

    gcmFOOTER_NO();
    return _IsUnrolledLoopIndexRecursively(
                                        Compiler,
                                        CodeGenerator->currentIterationContext,
                                        Name,
                                        UnrolledLoopIndexValue);
}

typedef struct _slsITERATION_UNROLL_INFO
{
    gctBOOL             unrollable;

    slsNAME *           loopIndexName;

    sleCONDITION        condition;

    sluCONSTANT_VALUE   conditionConstantValue;

    sluCONSTANT_VALUE   initialConstantValue;

    sluCONSTANT_VALUE   incrementConstantValue;

    gctUINT             loopCount;
}
slsITERATION_UNROLL_INFO;

static gceSTATUS
_CheckAsUnrollableCondition(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN slsNAME_SPACE * ForSpace,
    OUT slsITERATION_UNROLL_INFO * IterationUnrollInfo
    )
{
    gceSTATUS               status;
    sloIR_CONSTANT          conditionConstant   = gcvNULL;
    slsGEN_CODE_PARAMETERS  conditionParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(ForSpace);
    gcmASSERT(IterationUnrollInfo);

    do
    {
        /* Check the loop index */
        if (sloIR_OBJECT_GetType(&BinaryExpr->leftOperand->base) != slvIR_VARIABLE) break;

        IterationUnrollInfo->loopIndexName = ((sloIR_VARIABLE)BinaryExpr->leftOperand)->name;

        if (IterationUnrollInfo->loopIndexName->mySpace != ForSpace) break;

        if (!slsDATA_TYPE_IsInt(IterationUnrollInfo->loopIndexName->dataType)
            && !slsDATA_TYPE_IsFloat(IterationUnrollInfo->loopIndexName->dataType))
        {
            break;
        }

        /* Check the relation operator */
        switch (BinaryExpr->type)
        {
        case slvBINARY_GREATER_THAN:
            IterationUnrollInfo->condition = slvCONDITION_GREATER_THAN;
            break;

        case slvBINARY_LESS_THAN:
            IterationUnrollInfo->condition = slvCONDITION_LESS_THAN;
            break;

        case slvBINARY_GREATER_THAN_EQUAL:
            IterationUnrollInfo->condition = slvCONDITION_GREATER_THAN_EQUAL;
            break;

        case slvBINARY_LESS_THAN_EQUAL:
            IterationUnrollInfo->condition = slvCONDITION_LESS_THAN_EQUAL;
            break;

        case slvBINARY_EQUAL:
            IterationUnrollInfo->condition = slvCONDITION_EQUAL;
            break;

        case slvBINARY_NOT_EQUAL:
            IterationUnrollInfo->condition = slvCONDITION_NOT_EQUAL;
            break;

        default:
            gcmASSERT(0);
            IterationUnrollInfo->condition = slvCONDITION_INVALID;
        }

        if (IterationUnrollInfo->condition == slvCONDITION_INVALID) break;

        /* Try to evaluate the condition operand */
        gcmASSERT(BinaryExpr->rightOperand);

        slsGEN_CODE_PARAMETERS_Initialize(
                                        &conditionParameters,
                                        gcvFALSE,
                                        gcvTRUE);

        conditionParameters.hint = slvEVALUATE_ONLY;

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &BinaryExpr->rightOperand->base,
                                    &CodeGenerator->visitor,
                                    &conditionParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        conditionConstant               = conditionParameters.constant;
        conditionParameters.constant    = gcvNULL;

        slsGEN_CODE_PARAMETERS_Finalize(&conditionParameters);

        if (conditionConstant == gcvNULL) break;

        if (!slsDATA_TYPE_IsInt(conditionConstant->exprBase.dataType)
            && !slsDATA_TYPE_IsFloat(conditionConstant->exprBase.dataType))
        {
            break;
        }

        IterationUnrollInfo->conditionConstantValue = conditionConstant->values[0];

        /* OK */
        IterationUnrollInfo->unrollable = gcvTRUE;

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &conditionConstant->exprBase.base));

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    IterationUnrollInfo->unrollable = gcvFALSE;

    if (conditionConstant != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &conditionConstant->exprBase.base));
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_CheckAsUnrollableInitializer(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsITERATION_UNROLL_INFO * IterationUnrollInfo
    )
{
    gceSTATUS               status;
    sloIR_CONSTANT          initialConstant = gcvNULL;
    slsGEN_CODE_PARAMETERS  initialParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(IterationUnrollInfo);

    do
    {
        /* Check the loop index */
        if (sloIR_OBJECT_GetType(&BinaryExpr->leftOperand->base) != slvIR_VARIABLE) break;

        if (((sloIR_VARIABLE)BinaryExpr->leftOperand)->name != IterationUnrollInfo->loopIndexName)
        {
            break;
        }

        /* Check the relation operator */
        if (BinaryExpr->type != slvBINARY_ASSIGN) break;

        /* Try to evaluate the initial operand */
        gcmASSERT(BinaryExpr->rightOperand);

        slsGEN_CODE_PARAMETERS_Initialize(
                                        &initialParameters,
                                        gcvFALSE,
                                        gcvTRUE);

        initialParameters.hint = slvEVALUATE_ONLY;

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &BinaryExpr->rightOperand->base,
                                    &CodeGenerator->visitor,
                                    &initialParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        initialConstant             = initialParameters.constant;
        initialParameters.constant  = gcvNULL;

        slsGEN_CODE_PARAMETERS_Finalize(&initialParameters);

        if (initialConstant == gcvNULL) break;

        if (!slsDATA_TYPE_IsInt(initialConstant->exprBase.dataType)
            && !slsDATA_TYPE_IsFloat(initialConstant->exprBase.dataType))
        {
            break;
        }

        IterationUnrollInfo->initialConstantValue = initialConstant->values[0];

        /* OK */
        IterationUnrollInfo->unrollable = gcvTRUE;

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &initialConstant->exprBase.base));

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    IterationUnrollInfo->unrollable = gcvFALSE;

    if (initialConstant != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &initialConstant->exprBase.base));
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_CheckAsUnrollableRestExpr1(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_UNARY_EXPR UnaryExpr,
    IN OUT slsITERATION_UNROLL_INFO * IterationUnrollInfo
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_IR_OBJECT(UnaryExpr, slvIR_UNARY_EXPR);
    gcmASSERT(IterationUnrollInfo);

    do
    {
        /* Check the loop index */
        if (sloIR_OBJECT_GetType(&UnaryExpr->operand->base) != slvIR_VARIABLE) break;

        if (((sloIR_VARIABLE)&UnaryExpr->operand->base)->name != IterationUnrollInfo->loopIndexName)
        {
            break;
        }

        /* Check the relation operator */
        if (UnaryExpr->type == slvUNARY_POST_INC || UnaryExpr->type == slvUNARY_PRE_INC)
        {
            if (slsDATA_TYPE_IsInt(IterationUnrollInfo->loopIndexName->dataType))
            {
                IterationUnrollInfo->incrementConstantValue.intValue = 1;
            }
            else
            {
                gcmASSERT(slsDATA_TYPE_IsFloat(IterationUnrollInfo->loopIndexName->dataType));

                IterationUnrollInfo->incrementConstantValue.floatValue = (gctFLOAT)1.0;
            }
        }
        else if (UnaryExpr->type == slvUNARY_POST_DEC || UnaryExpr->type == slvUNARY_PRE_DEC)
        {
            if (slsDATA_TYPE_IsInt(IterationUnrollInfo->loopIndexName->dataType))
            {
                IterationUnrollInfo->incrementConstantValue.intValue = -1;
            }
            else
            {
                gcmASSERT(slsDATA_TYPE_IsFloat(IterationUnrollInfo->loopIndexName->dataType));

                IterationUnrollInfo->incrementConstantValue.floatValue = (gctFLOAT)-1.0;
            }
        }
        else
        {
            break;
        }

        /* OK */
        IterationUnrollInfo->unrollable = gcvTRUE;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    IterationUnrollInfo->unrollable = gcvFALSE;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_CheckAsUnrollableRestExpr2(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsITERATION_UNROLL_INFO * IterationUnrollInfo
    )
{
    gceSTATUS               status;
    sloIR_CONSTANT          incrementConstant   = gcvNULL;
    slsGEN_CODE_PARAMETERS  incrementParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(IterationUnrollInfo);

    do
    {
        /* Check the loop index */
        if (sloIR_OBJECT_GetType(&BinaryExpr->leftOperand->base) != slvIR_VARIABLE) break;

        if (((sloIR_VARIABLE)BinaryExpr->leftOperand)->name != IterationUnrollInfo->loopIndexName)
        {
            break;
        }

        /* Try to evaluate the increment operand */
        gcmASSERT(BinaryExpr->rightOperand);

        slsGEN_CODE_PARAMETERS_Initialize(
                                        &incrementParameters,
                                        gcvFALSE,
                                        gcvTRUE);

        incrementParameters.hint = slvEVALUATE_ONLY;

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &BinaryExpr->rightOperand->base,
                                    &CodeGenerator->visitor,
                                    &incrementParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        incrementConstant               = incrementParameters.constant;
        incrementParameters.constant    = gcvNULL;

        slsGEN_CODE_PARAMETERS_Finalize(&incrementParameters);

        if (incrementConstant == gcvNULL) break;

        if (!slsDATA_TYPE_IsInt(incrementConstant->exprBase.dataType)
            && !slsDATA_TYPE_IsFloat(incrementConstant->exprBase.dataType))
        {
            break;
        }

        IterationUnrollInfo->incrementConstantValue = incrementConstant->values[0];

        /* Check the operator */
        if (BinaryExpr->type == slvBINARY_ADD_ASSIGN)
        {
            /* Nothing to do */
        }
        else if (BinaryExpr->type == slvBINARY_SUB_ASSIGN)
        {
            if (slsDATA_TYPE_IsInt(IterationUnrollInfo->loopIndexName->dataType))
            {
                IterationUnrollInfo->incrementConstantValue.intValue =
                        -IterationUnrollInfo->incrementConstantValue.intValue;
            }
            else
            {
                gcmASSERT(slsDATA_TYPE_IsFloat(IterationUnrollInfo->loopIndexName->dataType));

                IterationUnrollInfo->incrementConstantValue.floatValue =
                        -IterationUnrollInfo->incrementConstantValue.floatValue;
            }
        }
        else
        {
            break;
        }

        /* OK */
        IterationUnrollInfo->unrollable = gcvTRUE;

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &incrementConstant->exprBase.base));

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    IterationUnrollInfo->unrollable = gcvFALSE;

    if (incrementConstant != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &incrementConstant->exprBase.base));
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#if GC_ENABLE_LOADTIME_OPT
gctUINT  MAX_UNROLL_ITERATION_COUNT = 8;
#else
#define MAX_UNROLL_ITERATION_COUNT      3
#endif /* GC_ENABLE_LOADTIME_OPT */

static gceSTATUS
_CheckIterationCount(
    IN OUT slsITERATION_UNROLL_INFO * IterationUnrollInfo
    )
{
    gctUINT             i;
    sluCONSTANT_VALUE   loopIndexValue;
    gctBOOL             loopTestResult = gcvFALSE;

    gcmHEADER();

    loopIndexValue = IterationUnrollInfo->initialConstantValue;

    for (i = 0; gcvTRUE; i++)
    {
        /* Use the condition */
        if (slsDATA_TYPE_IsInt(IterationUnrollInfo->loopIndexName->dataType))
        {
            switch (IterationUnrollInfo->condition)
            {
            case slvCONDITION_EQUAL:
                loopTestResult = (loopIndexValue.intValue
                                    == IterationUnrollInfo->conditionConstantValue.intValue);
                break;

            case slvCONDITION_NOT_EQUAL:
                loopTestResult = (loopIndexValue.intValue
                                    != IterationUnrollInfo->conditionConstantValue.intValue);
                break;

            case slvCONDITION_LESS_THAN:
                loopTestResult = (loopIndexValue.intValue
                                    < IterationUnrollInfo->conditionConstantValue.intValue);
                break;

            case slvCONDITION_LESS_THAN_EQUAL:
                loopTestResult = (loopIndexValue.intValue
                                    <= IterationUnrollInfo->conditionConstantValue.intValue);
                break;

            case slvCONDITION_GREATER_THAN:
                loopTestResult = (loopIndexValue.intValue
                                    > IterationUnrollInfo->conditionConstantValue.intValue);
                break;

            case slvCONDITION_GREATER_THAN_EQUAL:
                loopTestResult = (loopIndexValue.intValue
                                    >= IterationUnrollInfo->conditionConstantValue.intValue);
                break;

            default:
                gcmASSERT(0);
            }
        }
        else
        {
            gcmASSERT(slsDATA_TYPE_IsFloat(IterationUnrollInfo->loopIndexName->dataType));

            switch (IterationUnrollInfo->condition)
            {
            case slvCONDITION_EQUAL:
                loopTestResult = (loopIndexValue.floatValue
                                    == IterationUnrollInfo->conditionConstantValue.floatValue);
                break;

            case slvCONDITION_NOT_EQUAL:
                loopTestResult = (loopIndexValue.floatValue
                                    != IterationUnrollInfo->conditionConstantValue.floatValue);
                break;

            case slvCONDITION_LESS_THAN:
                loopTestResult = (loopIndexValue.floatValue
                                    < IterationUnrollInfo->conditionConstantValue.floatValue);
                break;

            case slvCONDITION_LESS_THAN_EQUAL:
                loopTestResult = (loopIndexValue.floatValue
                                    <= IterationUnrollInfo->conditionConstantValue.floatValue);
                break;

            case slvCONDITION_GREATER_THAN:
                loopTestResult = (loopIndexValue.floatValue
                                    > IterationUnrollInfo->conditionConstantValue.floatValue);
                break;

            case slvCONDITION_GREATER_THAN_EQUAL:
                loopTestResult = (loopIndexValue.floatValue
                                    >= IterationUnrollInfo->conditionConstantValue.floatValue);
                break;

            default:
                gcmASSERT(0);
            }
        }

        if (!loopTestResult) break;

        /* Check the count */
        if (i >= MAX_UNROLL_ITERATION_COUNT)
        {
            IterationUnrollInfo->unrollable = gcvFALSE;

            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        /* Use the increment */
        if (slsDATA_TYPE_IsInt(IterationUnrollInfo->loopIndexName->dataType))
        {
            loopIndexValue.intValue += IterationUnrollInfo->incrementConstantValue.intValue;
        }
        else
        {
            gcmASSERT(slsDATA_TYPE_IsFloat(IterationUnrollInfo->loopIndexName->dataType));

            loopIndexValue.floatValue += IterationUnrollInfo->incrementConstantValue.floatValue;
        }
    }

    /* OK */
    IterationUnrollInfo->loopCount  = i;

    IterationUnrollInfo->unrollable = gcvTRUE;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_GenUnrolledCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_ITERATION Iteration,
    IN slsITERATION_UNROLL_INFO * IterationUnrollInfo,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    gctUINT                 i;
    slsGEN_CODE_PARAMETERS  loopBodyParameters;
    slsITERATION_CONTEXT    iterationContext;
    sluCONSTANT_VALUE       unrolledLoopIndexValue;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(IterationUnrollInfo);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand && !Parameters->needROperand);

    if (Iteration->loopBody == gcvNULL) { gcmFOOTER_NO(); return gcvSTATUS_OK; }

    status = _DefineUnrolledIterationBegin(
                                        Compiler,
                                        CodeGenerator,
                                        IterationUnrollInfo->loopIndexName,
                                        &iterationContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    unrolledLoopIndexValue  = IterationUnrollInfo->initialConstantValue;

    for (i = 0; i < IterationUnrollInfo->loopCount; i++)
    {
        status = _DefineUnrolledIterationBodyBegin(
                                                Compiler,
                                                CodeGenerator,
                                                unrolledLoopIndexValue);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Generate the code of the loop body */
        slsGEN_CODE_PARAMETERS_Initialize(&loopBodyParameters, gcvFALSE, gcvFALSE);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    Iteration->loopBody,
                                    &CodeGenerator->visitor,
                                    &loopBodyParameters);

        slsGEN_CODE_PARAMETERS_Finalize(&loopBodyParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = _DefineUnrolledIterationBodyEnd(
                                                Compiler,
                                                CodeGenerator);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Use the increment */
        if (slsDATA_TYPE_IsInt(IterationUnrollInfo->loopIndexName->dataType))
        {
            unrolledLoopIndexValue.intValue +=
                    IterationUnrollInfo->incrementConstantValue.intValue;
        }
        else
        {
            gcmASSERT(slsDATA_TYPE_IsFloat(IterationUnrollInfo->loopIndexName->dataType));

            unrolledLoopIndexValue.floatValue +=
                    IterationUnrollInfo->incrementConstantValue.floatValue;
        }
    }

    status = _DefineUnrolledIterationEnd(
                                        Compiler,
                                        CodeGenerator);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_TryToGenUnrolledCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters,
    OUT gctBOOL * IsUnrolled
    )
{
    gceSTATUS                   status = gcvSTATUS_OK;
    slsITERATION_UNROLL_INFO    iterationUnrollInfo;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(Parameters);
    gcmASSERT(IsUnrolled);

    do
    {
        if (Iteration->type != slvFOR) break;

        gcmASSERT(Iteration->forSpace);

        /* Check the condition part */
        if (Iteration->condExpr == gcvNULL
            || sloIR_OBJECT_GetType(&Iteration->condExpr->base) != slvIR_BINARY_EXPR) break;

        status = _CheckAsUnrollableCondition(
                                            Compiler,
                                            CodeGenerator,
                                            (sloIR_BINARY_EXPR)Iteration->condExpr,
                                            Iteration->forSpace,
                                            &iterationUnrollInfo);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (!iterationUnrollInfo.unrollable) break;

        /* Check the initial part */
        if (Iteration->forInitStatement == gcvNULL
            || sloIR_OBJECT_GetType(Iteration->forInitStatement) != slvIR_BINARY_EXPR) break;

        status = _CheckAsUnrollableInitializer(
                                            Compiler,
                                            CodeGenerator,
                                            (sloIR_BINARY_EXPR)Iteration->forInitStatement,
                                            &iterationUnrollInfo);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (!iterationUnrollInfo.unrollable) break;

        /* Check the rest part */
        if (Iteration->forRestExpr == gcvNULL) break;

        if (sloIR_OBJECT_GetType(&Iteration->forRestExpr->base) == slvIR_UNARY_EXPR)
        {
            status = _CheckAsUnrollableRestExpr1(
                                                Compiler,
                                                CodeGenerator,
                                                (sloIR_UNARY_EXPR)Iteration->forRestExpr,
                                                &iterationUnrollInfo);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            if (!iterationUnrollInfo.unrollable) break;
        }
        else if (sloIR_OBJECT_GetType(&Iteration->forRestExpr->base) == slvIR_BINARY_EXPR)
        {
            status = _CheckAsUnrollableRestExpr2(
                                                Compiler,
                                                CodeGenerator,
                                                (sloIR_BINARY_EXPR)Iteration->forRestExpr,
                                                &iterationUnrollInfo);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            if (!iterationUnrollInfo.unrollable) break;
        }
        else
        {
            break;
        }

        /* TODO: Check the loop body */

        /* Check the iteration count */
        status = _CheckIterationCount(&iterationUnrollInfo);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (!iterationUnrollInfo.unrollable) break;

        /* Generate the unrolled code */
        status = sloIR_ITERATION_GenUnrolledCode(
                                                Compiler,
                                                CodeGenerator,
                                                Iteration,
                                                &iterationUnrollInfo,
                                                Parameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* OK */
        *IsUnrolled = gcvTRUE;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *IsUnrolled = gcvFALSE;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_ITERATION_GenForCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsITERATION_CONTEXT    iterationContext;
    slsGEN_CODE_PARAMETERS  initParameters, restParameters, bodyParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(Parameters);
    gcmASSERT(Iteration->type == slvFOR);

	/********************************************************/
	/*                                                      */
	/*  for (initStmt; condExpr; restExpr)                  */
	/*     loopBody                                         */
	/*                                                      */
	/*  generate code:                                      */
	/*                                                      */
	/*     initStmt                                         */
	/*     JMP L1 if (!condExpr)                            */
	/*  L2:                                                 */
	/*     loopBody                                         */
	/*  L3:                     // continue jumps to here   */
	/*     restExpr                                         */
	/*     JMP L2 if (condExpr)                             */
	/*  L1:                                                 */
	/*                                                      */
	/********************************************************/


    /* The init part */
    if (Iteration->forInitStatement != gcvNULL)
    {
        slsGEN_CODE_PARAMETERS_Initialize(&initParameters, gcvFALSE, gcvFALSE);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    Iteration->forInitStatement,
                                    &CodeGenerator->visitor,
                                    &initParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Finalize(&initParameters);
    }

    status = _DefineIterationBegin(
                                Compiler,
                                CodeGenerator,
                                gcvTRUE,
                                (Iteration->forRestExpr != gcvNULL),
                                &iterationContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

	/* The condition part: jump to loopend if the condExpr is not true
	   JMP L1 if (!condExpr)    */
    if (Iteration->condExpr != gcvNULL)
    {
        status = _GenConditionCode(
                                Compiler,
                                CodeGenerator,
                                Iteration->condExpr,
                                _GetIterationEndLabel(Compiler, CodeGenerator),
                                gcvFALSE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    /* The loop body: body begin label */
    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        CodeGenerator->currentIterationContext->u.genericInfo.loopBeginLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    /* The body part */
    if (Iteration->loopBody != gcvNULL)
    {
        slsGEN_CODE_PARAMETERS_Initialize(&bodyParameters, gcvFALSE, gcvFALSE);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    Iteration->loopBody,
                                    &CodeGenerator->visitor,
                                    &bodyParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Finalize(&bodyParameters);
    }

    /* The rest part */
    if (Iteration->forRestExpr != gcvNULL)
    {
        status = _DefineIterationRestExprBegin(Compiler, CodeGenerator);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Initialize(&restParameters, gcvFALSE, gcvFALSE);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &Iteration->forRestExpr->base,
                                    &CodeGenerator->visitor,
                                    &restParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    /* The condition part: jump to loop begin if the exprCond is true
	    JMP L2 if (condExpr)   */
    if (Iteration->condExpr != gcvNULL)
    {
        status = _GenConditionCode(
                                Compiler,
                                CodeGenerator,
                                Iteration->condExpr,
                                CodeGenerator->currentIterationContext->
                                            u.genericInfo.loopBeginLabel,
                                gcvTRUE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        /* For the infinite for loop, 'for(;;)', we still need an unconditional branch to begin */
        status = slEmitAlwaysBranchCode(
                                        Compiler,
                                        0,
                                        0,
                                        slvOPCODE_JUMP,
                                        CodeGenerator->currentIterationContext->
                                            u.genericInfo.loopBeginLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = _DefineIterationEnd(
                                Compiler,
                                CodeGenerator);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_GenWhileCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsITERATION_CONTEXT    iterationContext;
    slsGEN_CODE_PARAMETERS  bodyParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(Parameters);
    gcmASSERT(Iteration->type == slvWHILE);

    status = _DefineIterationBegin(
                                Compiler,
                                CodeGenerator,
                                gcvTRUE,
                                gcvFALSE,
                                &iterationContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The loop body */
    status = _DefineIterationBodyBegin(Compiler, CodeGenerator);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The condition part */
    if (Iteration->condExpr != gcvNULL)
    {
        status = _GenConditionCode(
                                Compiler,
                                CodeGenerator,
                                Iteration->condExpr,
                                _GetIterationEndLabel(Compiler, CodeGenerator),
                                gcvFALSE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    if (Iteration->loopBody != gcvNULL)
    {
        slsGEN_CODE_PARAMETERS_Initialize(&bodyParameters, gcvFALSE, gcvFALSE);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    Iteration->loopBody,
                                    &CodeGenerator->visitor,
                                    &bodyParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Finalize(&bodyParameters);
    }

    status = _DefineIterationBodyEnd(Compiler, CodeGenerator);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _DefineIterationEnd(
                                Compiler,
                                CodeGenerator);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_GenDoWhileCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsITERATION_CONTEXT    iterationContext;
    slsGEN_CODE_PARAMETERS  bodyParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(Parameters);
    gcmASSERT(Iteration->type == slvDO_WHILE);

    status = _DefineIterationBegin(
                                Compiler,
                                CodeGenerator,
                                gcvFALSE,
                                gcvTRUE, /* Set HasRestExpr to TRUE because we need to */
                                         /* reuse this to correctly codegen 'continue' */
                                &iterationContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* do-while must have a loop begin label */
    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        CodeGenerator->currentIterationContext->u.genericInfo.loopBeginLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The loop body */
    if (Iteration->loopBody != gcvNULL)
    {
        slsGEN_CODE_PARAMETERS_Initialize(&bodyParameters, gcvFALSE, gcvFALSE);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    Iteration->loopBody,
                                    &CodeGenerator->visitor,
                                    &bodyParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Finalize(&bodyParameters);
    }

    /* Always look condition expression of do-while as rest of loop, so continue statement will branch to it. */
    /* This will not introduce redundant jumps or other side-effects since do-while naturally needs condition */
    /* judgement at the end of loop */
    status = slSetLabel(
                        Compiler,
                        0,
                        0,
                        CodeGenerator->currentIterationContext->u.genericInfo.restBeginLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The condition part */
    if (Iteration->condExpr != gcvNULL)
    {
        status = _GenConditionCode(
                                Compiler,
                                CodeGenerator,
                                Iteration->condExpr,
                                CodeGenerator->currentIterationContext->
                                            u.genericInfo.loopBeginLabel,
                                gcvTRUE);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = _DefineIterationEnd(
                                Compiler,
                                CodeGenerator);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS   status;
    gctBOOL     isUnrolled = gcvFALSE;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(Parameters);

    if (sloCOMPILER_OptimizationEnabled(Compiler, slvOPTIMIZATION_UNROLL_ITERATION))
    {
        status = sloIR_ITERATION_TryToGenUnrolledCode(
                                                    Compiler,
                                                    CodeGenerator,
                                                    Iteration,
                                                    Parameters,
                                                    &isUnrolled);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    if (isUnrolled)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    switch (Iteration->type)
    {
    case slvFOR:
        status = sloIR_ITERATION_GenForCode(
                                        Compiler,
                                        CodeGenerator,
                                        Iteration,
                                        Parameters);
            break;

    case slvWHILE:
        status = sloIR_ITERATION_GenWhileCode(
                                            Compiler,
                                            CodeGenerator,
                                            Iteration,
                                            Parameters);
            break;

    case slvDO_WHILE:
        status = sloIR_ITERATION_GenDoWhileCode(
                                            Compiler,
                                            CodeGenerator,
                                            Iteration,
                                            Parameters);
            break;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_JUMP_GenContinueCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_JUMP Jump,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Jump, slvIR_JUMP);
    gcmASSERT(Jump->type == slvCONTINUE);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand && !Parameters->needROperand);

    if (CodeGenerator->currentIterationContext == gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                    Compiler,
                                    Jump->base.lineNo,
                                    Jump->base.stringNo,
                                    slvREPORT_ERROR,
                                    "'continue' is only allowed within loops"));

        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status = slEmitAlwaysBranchCode(
                                    Compiler,
                                    Jump->base.lineNo,
                                    Jump->base.stringNo,
                                    slvOPCODE_JUMP,
                                    _GetIterationContinueLabel(Compiler, CodeGenerator));

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_JUMP_GenBreakCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_JUMP Jump,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Jump, slvIR_JUMP);
    gcmASSERT(Jump->type == slvBREAK);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand && !Parameters->needROperand);

    if (CodeGenerator->currentIterationContext == gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                    Compiler,
                                    Jump->base.lineNo,
                                    Jump->base.stringNo,
                                    slvREPORT_ERROR,
                                    "'break' is only allowed within loops"));

        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status = slEmitAlwaysBranchCode(
                                    Compiler,
                                    Jump->base.lineNo,
                                    Jump->base.stringNo,
                                    slvOPCODE_JUMP,
                                    _GetIterationEndLabel(Compiler, CodeGenerator));

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_JUMP_GenReturnCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_JUMP Jump,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsNAME *               funcName;
    slsGEN_CODE_PARAMETERS  returnExprParameters;
    gctUINT                 i;
    slsLOPERAND             lOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Jump, slvIR_JUMP);
    gcmASSERT(Jump->type == slvRETURN);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand && !Parameters->needROperand);

    if (CodeGenerator->currentFuncDefContext.isMain)
    {
        if (Jump->returnExpr != gcvNULL)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            Jump->base.lineNo,
                                            Jump->base.stringNo,
                                            slvREPORT_ERROR,
                                            "'main' function returning a value"));

            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        status = slEmitAlwaysBranchCode(
                                        Compiler,
                                        Jump->base.lineNo,
                                        Jump->base.stringNo,
                                        slvOPCODE_JUMP,
                                        CodeGenerator->currentFuncDefContext.u.mainEndLabel);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        funcName = CodeGenerator->currentFuncDefContext.u.funcBody->funcName;
        gcmASSERT(funcName);

        if (slsDATA_TYPE_IsVoid(funcName->dataType))
        {
            if (Jump->returnExpr != gcvNULL)
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                Jump->base.lineNo,
                                                Jump->base.stringNo,
                                                slvREPORT_ERROR,
                                                "'void' function: '%s' returning a value",
                                                funcName->symbol));

                gcmFOOTER_ARG("sttus=%s", gcvSTATUS_INVALID_ARGUMENT);
                return gcvSTATUS_INVALID_ARGUMENT;
            }
        }
        else if (Jump->returnExpr == gcvNULL)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            Jump->base.lineNo,
                                            Jump->base.stringNo,
                                            slvREPORT_WARN,
                                            "non-void function: '%s' must return a value",
                                            funcName->symbol));
        }
        else
        {
            if (!slsDATA_TYPE_IsEqual(funcName->dataType, Jump->returnExpr->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                Jump->base.lineNo,
                                                Jump->base.stringNo,
                                                slvREPORT_ERROR,
                                                "require the same typed return expression"));

                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
                return gcvSTATUS_INVALID_ARGUMENT;
            }

            /* Generate the code of the return expression */
            slsGEN_CODE_PARAMETERS_Initialize(
                                            &returnExprParameters,
                                            gcvFALSE,
                                            gcvTRUE);

            status = sloIR_OBJECT_Accept(
                                        Compiler,
                                        &Jump->returnExpr->base,
                                        &CodeGenerator->visitor,
                                        &returnExprParameters);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* Generate the assign code */
            gcmASSERT(returnExprParameters.operandCount == funcName->context.logicalRegCount);
            gcmASSERT(funcName->context.logicalRegs);

            for (i = 0; i < returnExprParameters.operandCount; i++)
            {
                slsLOPERAND_Initialize(
                                    &lOperand,
                                    funcName->context.logicalRegs + i);

                status = slGenAssignCode(
                                        Compiler,
                                        Jump->base.lineNo,
                                        Jump->base.stringNo,
                                        &lOperand,
                                        returnExprParameters.rOperands + i);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }

            slsGEN_CODE_PARAMETERS_Finalize(&returnExprParameters);
        }

        status = slEmitAlwaysBranchCode(
                                        Compiler,
                                        Jump->base.lineNo,
                                        Jump->base.stringNo,
                                        slvOPCODE_RETURN,
                                        0);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_JUMP_GenDiscardCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_JUMP Jump,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Jump, slvIR_JUMP);
    gcmASSERT(Jump->type == slvDISCARD);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand && !Parameters->needROperand);

    status = slEmitAlwaysBranchCode(
                                    Compiler,
                                    Jump->base.lineNo,
                                    Jump->base.stringNo,
                                    slvOPCODE_DISCARD,
                                    0);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_JUMP_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_JUMP Jump,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Jump, slvIR_JUMP);
    gcmASSERT(Parameters);

    switch (Jump->type)
    {
    case slvCONTINUE:
        status = sloIR_JUMP_GenContinueCode(
                                        Compiler,
                                        CodeGenerator,
                                        Jump,
                                        Parameters);
        break;

    case slvBREAK:
        status = sloIR_JUMP_GenBreakCode(
                                    Compiler,
                                    CodeGenerator,
                                    Jump,
                                    Parameters);
        break;

    case slvRETURN:
        status = sloIR_JUMP_GenReturnCode(
                                        Compiler,
                                        CodeGenerator,
                                        Jump,
                                        Parameters);
        break;

    case slvDISCARD:
        status = sloIR_JUMP_GenDiscardCode(
                                        Compiler,
                                        CodeGenerator,
                                        Jump,
                                        Parameters);
        break;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_VARIABLE_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_VARIABLE Variable,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS           status;
    gctUINT             i;
    sluCONSTANT_VALUE   unrolledLoopIndexValue;
    slsDATA_TYPE *      dataType;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Variable, slvIR_VARIABLE);
    gcmASSERT(Parameters);

    gcmASSERT(Variable->name);

    if (!Parameters->needLOperand && !Parameters->needROperand)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Check if it is the unrolled loop index */
    if (_IsUnrolledLoopIndex(
                            Compiler,
                            CodeGenerator,
                            Variable->name,
                            &unrolledLoopIndexValue))
    {
        gcmASSERT(!Parameters->needLOperand);

        if (Parameters->hint == slvEVALUATE_ONLY)
        {
            /* Create the data type */
            status = sloCOMPILER_CreateDataType(
                                                Compiler,
                                                slsDATA_TYPE_IsInt(Variable->exprBase.dataType) ?
                                                    T_INT : T_FLOAT,
                                                gcvNULL,
                                                &dataType);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            dataType->qualifier = slvQUALIFIER_CONST;

            /* Create the constant */
            status = sloIR_CONSTANT_Construct(
                                            Compiler,
                                            Variable->exprBase.base.lineNo,
                                            Variable->exprBase.base.stringNo,
                                            dataType,
                                            &Parameters->constant);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* Add the constant value */
            status = sloIR_CONSTANT_AddValues(
                                            Compiler,
                                            Parameters->constant,
                                            1,
                                            &unrolledLoopIndexValue);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            /* Allocate the operands */
            status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                            Compiler,
                                                            Parameters,
                                                            Variable->exprBase.dataType);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            gcmASSERT(Parameters->operandCount == 1);
            gcmASSERT(Parameters->dataTypes[0] == gcSHADER_FLOAT_X1
                        || Parameters->dataTypes[0] == gcSHADER_INTEGER_X1);

            slsROPERAND_InitializeConstant(
                                        &Parameters->rOperands[0],
                                        Parameters->dataTypes[0],
                                        1,
                                        &unrolledLoopIndexValue);
        }
    }
    else
    {
        if (Parameters->hint == slvEVALUATE_ONLY)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        /* Allocate all logical registers */
        status = slsNAME_AllocLogicalRegs(
                                        Compiler,
                                        CodeGenerator,
                                        Variable->name);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Allocate the operands */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        Variable->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (Parameters->needLOperand)
        {
            for (i = 0; i < Parameters->operandCount; i++)
            {
                slsLOPERAND_Initialize(
                                    Parameters->lOperands + i,
                                    Variable->name->context.logicalRegs + i);
            }
        }

        if (Parameters->needROperand)
        {
            for (i = 0; i < Parameters->operandCount; i++)
            {
                slsROPERAND_InitializeReg(
                                        Parameters->rOperands + i,
                                        Variable->name->context.logicalRegs + i);
            }
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_SetOperandConstants(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN sluCONSTANT_VALUE * Values,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters,
    IN OUT gctUINT * ValueStart,
    IN OUT gctUINT * Start
    )
{
    gceSTATUS       status;
    gctUINT         count, i;
    slsNAME *       fieldName;
    gcSHADER_TYPE   binaryDataType;
    gctUINT         componentCount;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(DataType);
    gcmASSERT(Values);
    gcmASSERT(Parameters);
    gcmASSERT(ValueStart);
    gcmASSERT(Start);

    count   = (DataType->arrayLength > 0) ? DataType->arrayLength : 1;

    for (i = 0; i < count; i++)
    {
        if (DataType->elementType == slvTYPE_STRUCT)
        {
            gcmASSERT(DataType->fieldSpace);

            FOR_EACH_DLINK_NODE(&DataType->fieldSpace->names, slsNAME, fieldName)
            {
                gcmASSERT(fieldName->dataType);

                status = _SetOperandConstants(
                                        Compiler,
                                        fieldName->dataType,
                                        Values,
                                        Parameters,
                                        ValueStart,
                                        Start);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
        }
        else
        {
            binaryDataType  = _ConvElementDataType(DataType);
            componentCount  = gcGetDataTypeComponentCount(binaryDataType);

            slsROPERAND_InitializeConstant(
                                        Parameters->rOperands + *Start,
                                        binaryDataType,
                                        componentCount,
                                        Values + *ValueStart);

            (*Start)++;
            (*ValueStart) += componentCount;
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_CONSTANT_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_CONSTANT Constant,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS   status;
    gctUINT     valueStart = 0, start = 0;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    if (!Parameters->needROperand)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if (Parameters->hint == slvEVALUATE_ONLY)
    {
        status = sloIR_CONSTANT_Clone(
                                    Compiler,
                                    Constant->exprBase.base.lineNo,
                                    Constant->exprBase.base.stringNo,
                                    Constant,
                                    &Parameters->constant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                    Compiler,
                                                    Parameters,
                                                    Constant->exprBase.dataType);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _SetOperandConstants(
                                Compiler,
                                Constant->exprBase.dataType,
                                Constant->values,
                                Parameters,
                                &valueStart,
                                &start);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmASSERT(valueStart == Constant->valueCount);
    gcmASSERT(start == Parameters->operandCount);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
_ConvertVecToAuxiScalarArray(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsROPERAND * VecOperand,
    IN slsNAME * scalarArrayName
    )
{
    gceSTATUS                   status;
    slsGEN_CODE_PARAMETERS      intermParameters;
    slsIOPERAND                 intermIOperand;
    slsLOPERAND                 intermLOperand;
    slsROPERAND                 intermROperand;
    slsDATA_TYPE                *scalarDataType;
    gcSHADER_TYPE               binaryDataType;
    gctSIZE_T                   binaryDataTypeSize;
    gctUINT                     logicalRegCount;
    gctREG_INDEX                tempRegIndex;
    slsCOMPONENT_SELECTION	    compSel;
    gctUINT                     i;

    gcmHEADER();

    binaryDataType      = scalarArrayName->context.logicalRegs->dataType;
    binaryDataTypeSize  = gcGetDataTypeSize(binaryDataType);
    logicalRegCount = scalarArrayName->context.logicalRegCount;
    tempRegIndex = scalarArrayName->context.logicalRegs->regIndex;

    status = slsDATA_TYPE_Clone(
                               Compiler, slvQUALIFIER_NONE,
                               scalarArrayName->dataType,
                               &scalarDataType);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    scalarDataType->arrayLength = 0;

    for (i = 0; i < logicalRegCount; i++)
    {
        slsGEN_CODE_PARAMETERS_Initialize(
                                        &intermParameters,
                                        gcvFALSE,
                                        gcvTRUE);

        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                    Compiler,
                                                    &intermParameters,
                                                    scalarDataType);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (i == 0)
            compSel = ComponentSelection_X;
        else if (i == 1)
            compSel = ComponentSelection_Y;
        else if (i == 2)
            compSel = ComponentSelection_Z;
        else if (i == 3)
            compSel = ComponentSelection_W;
        else
        {
            gcmASSERT(gcvFALSE);
            gcmFOOTER();
            return status;
        }

        slsIOPERAND_Initialize(&intermIOperand, binaryDataType, tempRegIndex + (gctREG_INDEX)(i * binaryDataTypeSize));
        slsLOPERAND_InitializeUsingIOperand(&intermLOperand, &intermIOperand);

        status = sloIR_ROperandComponentSelect(
                                               Compiler,
                                               VecOperand,
                                               compSel,
                                               &intermParameters.rOperands[0]);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        intermROperand = intermParameters.rOperands[0];

        status = slGenAssignCode(
                                Compiler,
                                0,
                                0,
                                &intermLOperand,
                                &intermROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Finalize(&intermParameters);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
_ConvertAuxiScalarArrayToVec(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN slsNAME * scalarArrayName,
    IN slsLOPERAND * VecOperand
    )
{
    gceSTATUS                   status;
    slsGEN_CODE_PARAMETERS      intermParameters;
    slsIOPERAND                 intermIOperand;
    slsLOPERAND                 intermLOperand;
    slsROPERAND                 intermROperand;
    slsDATA_TYPE                *scalarDataType;
    gcSHADER_TYPE               binaryDataType;
    gctSIZE_T                   binaryDataTypeSize;
    gctUINT                     logicalRegCount;
    gctREG_INDEX                tempRegIndex;
    slsCOMPONENT_SELECTION	    compSel;
    gctUINT                     i;

    gcmHEADER();

    binaryDataType      = scalarArrayName->context.logicalRegs->dataType;
    binaryDataTypeSize  = gcGetDataTypeSize(binaryDataType);
    logicalRegCount = scalarArrayName->context.logicalRegCount;
    tempRegIndex = scalarArrayName->context.logicalRegs->regIndex;

    status = slsDATA_TYPE_Clone(
                               Compiler, slvQUALIFIER_NONE,
                               scalarArrayName->dataType,
                               &scalarDataType);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    scalarDataType->arrayLength = 0;

    for (i = 0; i < logicalRegCount; i++)
    {
        slsGEN_CODE_PARAMETERS_Initialize(
                                        &intermParameters,
                                        gcvTRUE,
                                        gcvFALSE);

        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                    Compiler,
                                                    &intermParameters,
                                                    scalarDataType);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (i == 0)
            compSel = ComponentSelection_X;
        else if (i == 1)
            compSel = ComponentSelection_Y;
        else if (i == 2)
            compSel = ComponentSelection_Z;
        else if (i == 3)
            compSel = ComponentSelection_W;
        else
        {
            gcmASSERT(gcvFALSE);
            gcmFOOTER();
            return status;
        }

        slsIOPERAND_Initialize(&intermIOperand, binaryDataType, tempRegIndex + (gctREG_INDEX)(i * binaryDataTypeSize));
        slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

        status = sloIR_LOperandComponentSelect(
                                               Compiler,
                                               VecOperand,
                                               compSel,
                                               &intermParameters.lOperands[0]);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        intermLOperand = intermParameters.lOperands[0];

        status = slGenAssignCode(
                                Compiler,
                                0,
                                0,
                                &intermLOperand,
                                &intermROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Finalize(&intermParameters);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_GenFieldSelectionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_UNARY_EXPR UnaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  operandParameters;
    gctSIZE_T               operandFieldOffset = 0;
    gctUINT                 i;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(UnaryExpr, slvIR_UNARY_EXPR);
    gcmASSERT(UnaryExpr->type == slvUNARY_FIELD_SELECTION);
    gcmASSERT(Parameters);

    /* Generate the code of the operand */
    gcmASSERT(UnaryExpr->operand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &operandParameters,
                                    Parameters->needLOperand,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &UnaryExpr->operand->base,
                                &CodeGenerator->visitor,
                                &operandParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Copy all field operands */
    if (Parameters->needLOperand || Parameters->needROperand)
    {
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        UnaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        operandFieldOffset  = _GetLogicalOperandFieldOffset(
                                                            UnaryExpr->operand->dataType,
                                                            UnaryExpr->u.fieldName);
    }

    if (Parameters->needLOperand)
    {
        for (i = 0; i < Parameters->operandCount; i++)
        {
            Parameters->lOperands[i] = operandParameters.lOperands[operandFieldOffset + i];
        }
    }

    if (Parameters->needROperand)
    {
        for (i = 0; i < Parameters->operandCount; i++)
        {
            Parameters->rOperands[i] = operandParameters.rOperands[operandFieldOffset + i];
        }
    }

    slsGEN_CODE_PARAMETERS_Finalize(&operandParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctREG_INDEX
_ConvComponentToVectorIndex(
    IN gctUINT8 Component
    )
{
    switch (Component)
    {
    case slvCOMPONENT_X: return 0;
    case slvCOMPONENT_Y: return 1;
    case slvCOMPONENT_Z: return 2;
    case slvCOMPONENT_W: return 3;

    default:
        gcmASSERT(0);
        return 0;
    }
}

gceSTATUS
sloIR_ROperandComponentSelect(
    IN sloCOMPILER Compiler,
    IN slsROPERAND *From,
    IN slsCOMPONENT_SELECTION ComponentSelection,
    OUT slsROPERAND *To
    )
{
    gcmHEADER();

    gcmASSERT(From);
    gcmASSERT(To);

    *To = *From;

    To->dataType = gcGetVectorComponentSelectionDataType(From->dataType,
                                                         ComponentSelection.components);

    if (ComponentSelection.components == 1) {
       gcmASSERT(To->vectorIndex.mode == slvINDEX_NONE);
       To->vectorIndex.mode       = slvINDEX_CONSTANT;
       To->vectorIndex.u.constant = _ConvComponentToVectorIndex(ComponentSelection.x);
    }
    else {
       To->u.reg.componentSelection = _SwizzleComponentSelection(ComponentSelection,
                                                                 From->u.reg.componentSelection);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_LOperandComponentSelect(
    IN sloCOMPILER Compiler,
    IN slsLOPERAND *From,
    IN slsCOMPONENT_SELECTION ComponentSelection,
    OUT slsLOPERAND *To
    )
{
    gcmHEADER();

    gcmASSERT(From);
    gcmASSERT(To);

    *To = *From;

    To->dataType = gcGetVectorComponentSelectionDataType(From->dataType,
                                                         ComponentSelection.components);

    if (ComponentSelection.components == 1) {
       gcmASSERT(To->vectorIndex.mode == slvINDEX_NONE);
       To->vectorIndex.mode       = slvINDEX_CONSTANT;
       To->vectorIndex.u.constant = _ConvComponentToVectorIndex(ComponentSelection.x);
    }
    else {
       To->reg.componentSelection = _SwizzleComponentSelection(ComponentSelection,
                                                               From->reg.componentSelection);
    }
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_GenComponentSelectionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_UNARY_EXPR UnaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  operandParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(UnaryExpr, slvIR_UNARY_EXPR);
    gcmASSERT(UnaryExpr->type == slvUNARY_COMPONENT_SELECTION);
    gcmASSERT(Parameters);

    /* Generate the code of the operand */
    gcmASSERT(UnaryExpr->operand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &operandParameters,
                                    Parameters->needLOperand,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &UnaryExpr->operand->base,
                                &CodeGenerator->visitor,
                                &operandParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Make the swizzled operand */
    if (Parameters->needLOperand || Parameters->needROperand)
    {
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        UnaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    if (Parameters->needLOperand)
    {
        gcmASSERT(Parameters->operandCount == 1);

        Parameters->lOperands[0]            = operandParameters.lOperands[0];

        Parameters->lOperands[0].dataType   =
            gcGetVectorComponentSelectionDataType(
                                                operandParameters.lOperands[0].dataType,
                                                UnaryExpr->u.componentSelection.components);

        if (UnaryExpr->u.componentSelection.components == 1)
        {
            gcmASSERT(Parameters->lOperands[0].vectorIndex.mode == slvINDEX_NONE);

            Parameters->lOperands[0].vectorIndex.mode       = slvINDEX_CONSTANT;
            Parameters->lOperands[0].vectorIndex.u.constant =
                _ConvComponentToVectorIndex(UnaryExpr->u.componentSelection.x);
        }
        else
        {
            Parameters->lOperands[0].reg.componentSelection =
                _SwizzleComponentSelection(
                                            UnaryExpr->u.componentSelection,
                                            operandParameters.lOperands[0].reg.componentSelection);
        }
    }

    if (Parameters->needROperand)
    {
        gcmASSERT(Parameters->operandCount == 1);

        sloIR_ROperandComponentSelect(Compiler,
                                      &operandParameters.rOperands[0],
                                      UnaryExpr->u.componentSelection,
                                      &Parameters->rOperands[0]);
    }

    slsGEN_CODE_PARAMETERS_Finalize(&operandParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_GenIncOrDecCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_UNARY_EXPR UnaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    gctBOOL                 isPost, isInc;
    slsGEN_CODE_PARAMETERS  operandParameters;
    slsIOPERAND             intermIOperand;
    slsROPERAND             intermROperand;
    sluCONSTANT_VALUE       constantValue;
    slsROPERAND             constantROperand;
	gcSHADER_TYPE           componentDataType;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(UnaryExpr, slvIR_UNARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    switch (UnaryExpr->type)
    {
    case slvUNARY_POST_INC: isPost = gcvTRUE;   isInc = gcvTRUE;    break;
    case slvUNARY_POST_DEC: isPost = gcvTRUE;   isInc = gcvFALSE;   break;
    case slvUNARY_PRE_INC:  isPost = gcvFALSE;  isInc = gcvTRUE;    break;
    case slvUNARY_PRE_DEC:  isPost = gcvFALSE;  isInc = gcvFALSE;   break;

    default: gcmASSERT(0);
             gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
             return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Generate the code of the operand */
    gcmASSERT(UnaryExpr->operand);

    slsGEN_CODE_PARAMETERS_Initialize(&operandParameters, gcvTRUE, gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &UnaryExpr->operand->base,
                                &CodeGenerator->visitor,
                                &operandParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmASSERT(operandParameters.operandCount == 1);

    /* add/sub t0, operand, 1 */
    slsIOPERAND_New(Compiler, &intermIOperand, operandParameters.dataTypes[0]);

	componentDataType = gcGetComponentDataType(operandParameters.dataTypes[0]);

    switch (componentDataType)
    {
    case gcSHADER_FLOAT_X1:     constantValue.floatValue    = (gctFLOAT)1.0;    break;
    case gcSHADER_INTEGER_X1:   constantValue.intValue      = 1;                break;

    default: gcmASSERT(0);
        constantValue.floatValue    = (gctFLOAT)1.0;
    }

    slsROPERAND_InitializeConstant(
                                &constantROperand,
                                componentDataType,
                                1,
                                &constantValue);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    UnaryExpr->exprBase.base.lineNo,
                                    UnaryExpr->exprBase.base.stringNo,
                                    (isInc) ? slvOPCODE_ADD : slvOPCODE_SUB,
                                    &intermIOperand,
                                    &operandParameters.rOperands[0],
                                    &constantROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mov operand, t0 */
    slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

    status = slGenAssignCode(
                            Compiler,
                            UnaryExpr->exprBase.base.lineNo,
                            UnaryExpr->exprBase.base.stringNo,
                            &operandParameters.lOperands[0],
                            &intermROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        if (isPost)
        {
            /* sub/add t0, operand, 1 */
            status = slGenArithmeticExprCode(
                                            Compiler,
                                            UnaryExpr->exprBase.base.lineNo,
                                            UnaryExpr->exprBase.base.stringNo,
                                            (isInc) ? slvOPCODE_SUB : slvOPCODE_ADD,
                                            &intermIOperand,
                                            &operandParameters.rOperands[0],
                                            &constantROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* Return t0 */
            status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                            Compiler,
                                                            Parameters,
                                                            UnaryExpr->exprBase.dataType);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &intermIOperand);
        }
        else
        {
            /* Return the operand directly */
            slsGEN_CODE_PARAMETERS_MoveOperands(Parameters, &operandParameters);
        }
    }

    slsGEN_CODE_PARAMETERS_Finalize(&operandParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_GenNegCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_UNARY_EXPR UnaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  operandParameters;
    slsIOPERAND             intermIOperand;
    sluCONSTANT_VALUE       constantValue;
    slsROPERAND             constantROperand;
	gcSHADER_TYPE           componentDataType;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(UnaryExpr, slvIR_UNARY_EXPR);
    gcmASSERT(UnaryExpr->type == slvUNARY_NEG);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of the operand */
    gcmASSERT(UnaryExpr->operand);

    slsGEN_CODE_PARAMETERS_Initialize(&operandParameters, gcvFALSE, Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &UnaryExpr->operand->base,
                                &CodeGenerator->visitor,
                                &operandParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        gcmASSERT(operandParameters.operandCount == 1);

        /* sub t0, 0, operand */
        slsIOPERAND_New(Compiler, &intermIOperand, operandParameters.dataTypes[0]);

		componentDataType = gcGetComponentDataType(operandParameters.dataTypes[0]);

        switch (componentDataType)
        {
        case gcSHADER_FLOAT_X1:     constantValue.floatValue    = (gctFLOAT)0.0;    break;
        case gcSHADER_INTEGER_X1:   constantValue.intValue      = 0;                break;

        default: gcmASSERT(0);
            constantValue.floatValue    = (gctFLOAT)0.0;
        }

        slsROPERAND_InitializeConstant(
                                    &constantROperand,
                                    componentDataType,
                                    1,
                                    &constantValue);

        status = slGenArithmeticExprCode(
                                        Compiler,
                                        UnaryExpr->exprBase.base.lineNo,
                                        UnaryExpr->exprBase.base.stringNo,
                                        slvOPCODE_SUB,
                                        &intermIOperand,
                                        &constantROperand,
                                        &operandParameters.rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Return t0 */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        UnaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &intermIOperand);
    }

    slsGEN_CODE_PARAMETERS_Finalize(&operandParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_GenNotCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_UNARY_EXPR UnaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  operandParameters;
    slsIOPERAND             intermIOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(UnaryExpr, slvIR_UNARY_EXPR);
    gcmASSERT(UnaryExpr->type == slvUNARY_NOT);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of the operand */
    gcmASSERT(UnaryExpr->operand);

    slsGEN_CODE_PARAMETERS_Initialize(&operandParameters, gcvFALSE, Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &UnaryExpr->operand->base,
                                &CodeGenerator->visitor,
                                &operandParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        gcmASSERT(operandParameters.operandCount == 1);

        /* not t0, operand */
        slsIOPERAND_New(Compiler, &intermIOperand, operandParameters.dataTypes[0]);

        status = slGenGenericCode1(
                                Compiler,
                                UnaryExpr->exprBase.base.lineNo,
                                UnaryExpr->exprBase.base.stringNo,
                                slvOPCODE_NOT,
                                &intermIOperand,
                                &operandParameters.rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Return t0 */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        UnaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &intermIOperand);
    }

    slsGEN_CODE_PARAMETERS_Finalize(&operandParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_UNARY_EXPR UnaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  operandParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(UnaryExpr, slvIR_UNARY_EXPR);
    gcmASSERT(Parameters);

    /* Try to evaluate the operand */
    if (!Parameters->needLOperand && Parameters->needROperand)
    {
        gcmASSERT(UnaryExpr->operand);

        slsGEN_CODE_PARAMETERS_Initialize(
                                        &operandParameters,
                                        gcvFALSE,
                                        gcvTRUE);

        operandParameters.hint = slvEVALUATE_ONLY;

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &UnaryExpr->operand->base,
                                    &CodeGenerator->visitor,
                                    &operandParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (operandParameters.constant != gcvNULL)
        {
            status = sloIR_UNARY_EXPR_Evaluate(
                                            Compiler,
                                            UnaryExpr->type,
                                            operandParameters.constant,
                                            UnaryExpr->u.fieldName,
                                            &UnaryExpr->u.componentSelection,
                                            &Parameters->constant);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            operandParameters.constant  = gcvNULL;
        }

        slsGEN_CODE_PARAMETERS_Finalize(&operandParameters);

        if (Parameters->hint == slvEVALUATE_ONLY)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        if (Parameters->constant != gcvNULL)
        {
            status = sloIR_CONSTANT_GenCode(
                                        Compiler,
                                        CodeGenerator,
                                        Parameters->constant,
                                        Parameters);
            gcmFOOTER();
            return status;
        }
    }

    switch (UnaryExpr->type)
    {
    case slvUNARY_FIELD_SELECTION:
        status = sloIR_UNARY_EXPR_GenFieldSelectionCode(
                                                    Compiler,
                                                    CodeGenerator,
                                                    UnaryExpr,
                                                    Parameters);
        break;

    case slvUNARY_COMPONENT_SELECTION:
        status = sloIR_UNARY_EXPR_GenComponentSelectionCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        UnaryExpr,
                                                        Parameters);
        break;

    case slvUNARY_POST_INC:
    case slvUNARY_POST_DEC:
    case slvUNARY_PRE_INC:
    case slvUNARY_PRE_DEC:
        status = sloIR_UNARY_EXPR_GenIncOrDecCode(
                                                Compiler,
                                                CodeGenerator,
                                                UnaryExpr,
                                                Parameters);
        break;

    case slvUNARY_NEG:
        status = sloIR_UNARY_EXPR_GenNegCode(
                                        Compiler,
                                        CodeGenerator,
                                        UnaryExpr,
                                        Parameters);
        break;

    case slvUNARY_NOT:
        status = sloIR_UNARY_EXPR_GenNotCode(
                                        Compiler,
                                        CodeGenerator,
                                        UnaryExpr,
                                        Parameters);
        break;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Restore vec from scalar array if vect is component indexing. */
    if ((UnaryExpr->operand->base.vptr->type == slvIR_BINARY_EXPR) &&
        ((sloIR_BINARY_EXPR)(UnaryExpr->operand))->u.vec2Array)
    {
        status = _ConvertAuxiScalarArrayToVec(
                                             Compiler,
                                             CodeGenerator,
                                             ((sloIR_BINARY_EXPR)(UnaryExpr->operand))->u.vec2Array->scalarArrayName,
                                             &((sloIR_BINARY_EXPR)(UnaryExpr->operand))->u.vec2Array->vecOperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = sloCOMPILER_Free(
                                  Compiler,
                                  ((sloIR_BINARY_EXPR)(UnaryExpr->operand))->u.vec2Array);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        ((sloIR_BINARY_EXPR)(UnaryExpr->operand))->u.vec2Array = gcvNULL;
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
_GetConstantSubscriptCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN slsGEN_CODE_PARAMETERS * LeftParameters,
    IN slsGEN_CODE_PARAMETERS * RightParameters,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gctINT32        index;
    gctUINT         offset, i;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(LeftParameters);
    gcmASSERT(RightParameters);
    gcmASSERT(Parameters);

    gcmASSERT(RightParameters->operandCount == 1);
    gcmASSERT(RightParameters->dataTypes[0] == gcSHADER_INTEGER_X1);
    gcmASSERT(!RightParameters->rOperands[0].isReg);

    index = RightParameters->rOperands[0].u.constant.values[0].intValue;

    if (slsDATA_TYPE_IsBVecOrIVecOrVec(BinaryExpr->leftOperand->dataType))
    {
        gcmASSERT(Parameters->operandCount == 1);

        if (Parameters->needLOperand)
        {
            Parameters->lOperands[0] = LeftParameters->lOperands[0];

            Parameters->lOperands[0].dataType =
                            gcGetVectorComponentDataType(LeftParameters->lOperands[0].dataType);
            Parameters->lOperands[0].vectorIndex.mode       = slvINDEX_CONSTANT;
            Parameters->lOperands[0].vectorIndex.u.constant = (gctREG_INDEX) index;
        }

        if (Parameters->needROperand)
        {
            Parameters->rOperands[0] = LeftParameters->rOperands[0];

            Parameters->rOperands[0].dataType =
                            gcGetVectorComponentDataType(LeftParameters->rOperands[0].dataType);
            Parameters->rOperands[0].vectorIndex.mode       = slvINDEX_CONSTANT;
            Parameters->rOperands[0].vectorIndex.u.constant = (gctREG_INDEX) index;
        }
    }
    else if (slsDATA_TYPE_IsMat(BinaryExpr->leftOperand->dataType))
    {
        gcmASSERT(Parameters->operandCount == 1);

        if (Parameters->needLOperand)
        {
            Parameters->lOperands[0] = LeftParameters->lOperands[0];

            Parameters->lOperands[0].dataType =
                            gcGetMatrixColumnDataType(LeftParameters->lOperands[0].dataType);
            Parameters->lOperands[0].matrixIndex.mode       = slvINDEX_CONSTANT;
            Parameters->lOperands[0].matrixIndex.u.constant = (gctREG_INDEX) index;
        }

        if (Parameters->needROperand)
        {
            Parameters->rOperands[0] = LeftParameters->rOperands[0];

            Parameters->rOperands[0].dataType =
                            gcGetMatrixColumnDataType(LeftParameters->rOperands[0].dataType);
            Parameters->rOperands[0].matrixIndex.mode       = slvINDEX_CONSTANT;
            Parameters->rOperands[0].matrixIndex.u.constant = (gctREG_INDEX) index;
        }
    }
    else
    {
        gcmASSERT(slsDATA_TYPE_IsArray(BinaryExpr->leftOperand->dataType));

        offset = Parameters->operandCount * index;

        gcmASSERT(offset < LeftParameters->operandCount);

        if (Parameters->needLOperand)
        {
            for (i = 0; i < Parameters->operandCount; i++)
            {
                gcmASSERT(LeftParameters->lOperands[offset + i].arrayIndex.mode == slvINDEX_NONE);

                Parameters->lOperands[i] = LeftParameters->lOperands[offset + i];
            }
        }

        if (Parameters->needROperand)
        {
            for (i = 0; i < Parameters->operandCount; i++)
            {
                gcmASSERT(LeftParameters->rOperands[offset + i].arrayIndex.mode == slvINDEX_NONE);

                Parameters->rOperands[i] = LeftParameters->rOperands[offset + i];
            }
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenIndexAssignCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN gctREG_INDEX IndexRegIndex,
    IN slsROPERAND * ROperand
    )
{
    slsLOPERAND lOperand;

    slsLOPERAND_InitializeTempReg(
                                &lOperand,
                                slvQUALIFIER_NONE,
                                gcSHADER_INTEGER_X1,
                                IndexRegIndex);

    return slGenAssignCode(
                        Compiler,
                        LineNo,
                        StringNo,
                        &lOperand,
                        ROperand);
}

static gceSTATUS
_GenIndexAddCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN gctREG_INDEX TargetIndexRegIndex,
    IN gctREG_INDEX SourceIndexRegIndex,
    IN slsROPERAND * ROperand
    )
{
    slsIOPERAND iOperand;
    slsROPERAND rOperand;

    slsIOPERAND_Initialize(
                        &iOperand,
                        gcSHADER_INTEGER_X1,
                        TargetIndexRegIndex);

    slsROPERAND_InitializeTempReg(
                                &rOperand,
                                slvQUALIFIER_NONE,
                                gcSHADER_INTEGER_X1,
                                SourceIndexRegIndex);

    return slGenArithmeticExprCode(
                                Compiler,
                                LineNo,
                                StringNo,
                                slvOPCODE_ADD,
                                &iOperand,
                                &rOperand,
                                ROperand);
}

static gceSTATUS
_GenIndexScaleCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN gctREG_INDEX IndexRegIndex,
    IN slsROPERAND * ROperand,
    IN gctINT32 ElementDataTypeSize
    )
{
    slsIOPERAND         iOperand;
    slsROPERAND         rOperand;
    sluCONSTANT_VALUE   constantValue;

    slsIOPERAND_Initialize(
                        &iOperand,
                        gcSHADER_INTEGER_X1,
                        IndexRegIndex);

    constantValue.intValue = ElementDataTypeSize;

    slsROPERAND_InitializeConstant(
                                &rOperand,
                                gcSHADER_INTEGER_X1,
                                1,
                                &constantValue);

    return slGenArithmeticExprCode(
                                Compiler,
                                LineNo,
                                StringNo,
                                slvOPCODE_MUL,
                                &iOperand,
                                ROperand,
                                &rOperand);
}

gceSTATUS
_GetNonConstantSubscriptCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN slsGEN_CODE_PARAMETERS * LeftParameters,
    IN slsGEN_CODE_PARAMETERS * RightParameters,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS       status;
    gctREG_INDEX    indexRegIndex;
    gctUINT         i;
    gctINT32        elementDataTypeSize;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(LeftParameters);
    gcmASSERT(RightParameters);
    gcmASSERT(Parameters);

    gcmASSERT(sloIR_OBJECT_GetType(&BinaryExpr->rightOperand->base) != slvIR_CONSTANT);
    gcmASSERT(RightParameters->operandCount == 1);
    gcmASSERT(RightParameters->rOperands != gcvNULL && RightParameters->rOperands[0].isReg);

    if (slsDATA_TYPE_IsBVecOrIVecOrVec(BinaryExpr->leftOperand->dataType))
    {
        /*
           Vector components are dynamic indexed, so scalar array with vector size
           will be generated, any dynamic indexing on vector component will be
           replaced with scalar array. Vector will be restored from scalar array at
           other proper place.
        */

        slsNAME                     *scalarArrayName;
        slsDATA_TYPE                *arrayDataType;
        slsIOPERAND                 intermIOperand;
        slsLOPERAND                 intermLOperand;
        slsROPERAND                 intermROperand;
        gcSHADER_TYPE               binaryDataType;
        gctREG_INDEX                tempRegIndex;

        /* Create scalar array data type */
        status = sloCOMPILER_CreateArrayDataType(
                                                Compiler,
                                                BinaryExpr->leftOperand->dataType,
                                                slmDATA_TYPE_vectorSize_GET(BinaryExpr->leftOperand->dataType),
                                                &arrayDataType);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slmDATA_TYPE_vectorSize_SET(arrayDataType, 0);
        arrayDataType->qualifier = slvQUALIFIER_NONE;

        /* Create a symbol for auxiliary scalar array */
        status = sloCOMPILER_CreateAuxiliaryName(Compiler,
                                                 (BinaryExpr->leftOperand->base.vptr->type == slvIR_VARIABLE) ?
                                                 ((sloIR_VARIABLE)BinaryExpr->leftOperand)->name : gcvNULL,
                                                 BinaryExpr->exprBase.base.lineNo,
                                                 BinaryExpr->exprBase.base.stringNo,
                                                 arrayDataType, &scalarArrayName);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Alloc register for auxiliary symbol */
        status = slsNAME_AllocLogicalRegs(Compiler, CodeGenerator, scalarArrayName);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Initialize scalar array with vec component */
        if (LeftParameters->needLOperand)
            slsROPERAND_InitializeWithLOPERAND(&intermROperand, LeftParameters->lOperands);
        if (LeftParameters->needROperand)
            intermROperand = LeftParameters->rOperands[0];

        status = _ConvertVecToAuxiScalarArray(Compiler, CodeGenerator, &intermROperand, scalarArrayName);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Indexed access on scalar array */
        binaryDataType      = scalarArrayName->context.logicalRegs->dataType;
        tempRegIndex = scalarArrayName->context.logicalRegs->regIndex;
        if (RightParameters->rOperands[0].arrayIndex.mode != slvINDEX_NONE
            || RightParameters->rOperands[0].matrixIndex.mode != slvINDEX_NONE
            || RightParameters->rOperands[0].vectorIndex.mode != slvINDEX_NONE
            || !_IsTempRegQualifier(RightParameters->rOperands[0].u.reg.qualifier))
        {
            indexRegIndex = slNewTempRegs(Compiler, 1);

            status = _GenIndexAssignCode(
                                        Compiler,
                                        BinaryExpr->rightOperand->base.lineNo,
                                        BinaryExpr->rightOperand->base.stringNo,
                                        indexRegIndex,
                                        &RightParameters->rOperands[0]);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            indexRegIndex = RightParameters->rOperands[0].u.reg.regIndex;
        }

        if (Parameters->needLOperand)
        {
            for (i = 0; i < Parameters->operandCount; i++)
            {
                slsIOPERAND_Initialize(&intermIOperand, binaryDataType, tempRegIndex);
                slsLOPERAND_InitializeUsingIOperand(&intermLOperand, &intermIOperand);

                Parameters->lOperands[i] = intermLOperand;

                Parameters->lOperands[i].arrayIndex.mode            = slvINDEX_REG;
                Parameters->lOperands[i].arrayIndex.u.indexRegIndex = indexRegIndex;
            }
        }

        if (Parameters->needROperand)
        {
            for (i = 0; i < Parameters->operandCount; i++)
            {
                slsIOPERAND_Initialize(&intermIOperand, binaryDataType, tempRegIndex);
                slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

                Parameters->rOperands[i] = intermROperand;

                Parameters->rOperands[i].arrayIndex.mode            = slvINDEX_REG;
                Parameters->rOperands[i].arrayIndex.u.indexRegIndex = indexRegIndex;
            }
        }

        /* If vect is going to be written, we should record this vec->array */
        /* transformation, so we can trans them reversely later */
        if (LeftParameters->needLOperand)
        {
            gctPOINTER                  pointer = gcvNULL;

            status = sloCOMPILER_Allocate(
                                        Compiler,
                                        (gctSIZE_T)sizeof(slsVEC2ARRAY),
                                        &pointer);
            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            BinaryExpr->u.vec2Array = pointer;

            BinaryExpr->u.vec2Array->scalarArrayName = scalarArrayName;
            BinaryExpr->u.vec2Array->vecOperand = LeftParameters->lOperands[0];
        }
    }
    else if (slsDATA_TYPE_IsMat(BinaryExpr->leftOperand->dataType))
    {
        gcmASSERT(Parameters->operandCount == 1);

        if (LeftParameters->rOperands && LeftParameters->rOperands[0].arrayIndex.mode == slvINDEX_REG)
        {
            indexRegIndex = slNewTempRegs(Compiler, 1);

            status = _GenIndexAddCode(
                                    Compiler,
                                    BinaryExpr->rightOperand->base.lineNo,
                                    BinaryExpr->rightOperand->base.stringNo,
                                    indexRegIndex,
                                    LeftParameters->rOperands[0].arrayIndex.u.indexRegIndex,
                                    &RightParameters->rOperands[0]);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            LeftParameters->rOperands[0].arrayIndex.mode = slvINDEX_NONE;
        }
        else if (RightParameters->rOperands[0].arrayIndex.mode != slvINDEX_NONE
                    || RightParameters->rOperands[0].matrixIndex.mode != slvINDEX_NONE
                    || RightParameters->rOperands[0].vectorIndex.mode != slvINDEX_NONE
                    || !_IsTempRegQualifier(RightParameters->rOperands[0].u.reg.qualifier))
        {
            indexRegIndex = slNewTempRegs(Compiler, 1);

            status = _GenIndexAssignCode(
                                        Compiler,
                                        BinaryExpr->rightOperand->base.lineNo,
                                        BinaryExpr->rightOperand->base.stringNo,
                                        indexRegIndex,
                                        &RightParameters->rOperands[0]);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            indexRegIndex = RightParameters->rOperands[0].u.reg.regIndex;
        }

        if (Parameters->needLOperand)
        {
            Parameters->lOperands[0] = LeftParameters->lOperands[0];

            Parameters->lOperands[0].dataType =
                            gcGetMatrixColumnDataType(LeftParameters->lOperands[0].dataType);

            Parameters->lOperands[0].matrixIndex.mode               = slvINDEX_REG;
            Parameters->lOperands[0].matrixIndex.u.indexRegIndex    = indexRegIndex;
        }

        if (Parameters->needROperand)
        {
            Parameters->rOperands[0] = LeftParameters->rOperands[0];

            Parameters->rOperands[0].dataType =
                            gcGetMatrixColumnDataType(LeftParameters->rOperands[0].dataType);

            Parameters->rOperands[0].matrixIndex.mode               = slvINDEX_REG;
            Parameters->rOperands[0].matrixIndex.u.indexRegIndex    = indexRegIndex;
        }
    }
    else
    {
        gcmASSERT(slsDATA_TYPE_IsArray(BinaryExpr->leftOperand->dataType));

        if (Parameters->operandCount > 1 || slsDATA_TYPE_IsMat(BinaryExpr->exprBase.dataType))
        {
            indexRegIndex = slNewTempRegs(Compiler, 1);

            for (i = 0, elementDataTypeSize = 0; i < Parameters->operandCount; i++)
            {
                elementDataTypeSize += gcGetDataTypeSize(Parameters->dataTypes[i]);
            }

            status = _GenIndexScaleCode(
                                    Compiler,
                                    BinaryExpr->rightOperand->base.lineNo,
                                    BinaryExpr->rightOperand->base.stringNo,
                                    indexRegIndex,
                                    &RightParameters->rOperands[0],
                                    elementDataTypeSize);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else if (RightParameters->rOperands[0].arrayIndex.mode != slvINDEX_NONE
                    || RightParameters->rOperands[0].matrixIndex.mode != slvINDEX_NONE
                    || RightParameters->rOperands[0].vectorIndex.mode != slvINDEX_NONE
                    || !_IsTempRegQualifier(RightParameters->rOperands[0].u.reg.qualifier))
        {
            indexRegIndex = slNewTempRegs(Compiler, 1);

            status = _GenIndexAssignCode(
                                        Compiler,
                                        BinaryExpr->rightOperand->base.lineNo,
                                        BinaryExpr->rightOperand->base.stringNo,
                                        indexRegIndex,
                                        &RightParameters->rOperands[0]);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            indexRegIndex = RightParameters->rOperands[0].u.reg.regIndex;
        }

        if (Parameters->needLOperand)
        {
            for (i = 0; i < Parameters->operandCount; i++)
            {
                gcmASSERT(LeftParameters->lOperands[i].matrixIndex.mode == slvINDEX_NONE);

                Parameters->lOperands[i] = LeftParameters->lOperands[i];

                Parameters->lOperands[i].arrayIndex.mode            = slvINDEX_REG;
                Parameters->lOperands[i].arrayIndex.u.indexRegIndex = indexRegIndex;
            }
        }

        if (Parameters->needROperand)
        {
            for (i = 0; i < Parameters->operandCount; i++)
            {
                gcmASSERT(LeftParameters->rOperands[i].matrixIndex.mode == slvINDEX_NONE);

                Parameters->rOperands[i] = LeftParameters->rOperands[i];

                Parameters->rOperands[i].arrayIndex.mode            = slvINDEX_REG;
                Parameters->rOperands[i].arrayIndex.u.indexRegIndex = indexRegIndex;
            }
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenSubscriptCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &leftParameters,
                                    Parameters->needLOperand,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    Parameters->needLOperand || Parameters->needROperand);

    rightParameters.hint = slvGEN_INDEX_CODE;

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the subscripting code */
    if (Parameters->needLOperand || Parameters->needROperand)
    {
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        BinaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmASSERT(rightParameters.operandCount == 1);
        gcmASSERT(rightParameters.rOperands != gcvNULL);

        if (!rightParameters.rOperands[0].isReg)
        {
            status = _GetConstantSubscriptCode(
                                            Compiler,
                                            CodeGenerator,
                                            BinaryExpr,
                                            &leftParameters,
                                            &rightParameters,
                                            Parameters);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            status = _GetNonConstantSubscriptCode(
                                                Compiler,
                                                CodeGenerator,
                                                BinaryExpr,
                                                &leftParameters,
                                                &rightParameters,
                                                Parameters);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
    }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenArithmeticCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;
    gctUINT                 i;
    slsIOPERAND             iOperand;
    sleOPCODE               opcode;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &leftParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the arithmetic code */
    if (Parameters->needROperand)
    {
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        BinaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        for (i = 0; i < Parameters->operandCount; i++)
        {
            slsIOPERAND_New(Compiler, &iOperand, Parameters->dataTypes[i]);

            switch (BinaryExpr->type)
            {
            case slvBINARY_ADD: opcode = slvOPCODE_ADD; break;
            case slvBINARY_SUB: opcode = slvOPCODE_SUB; break;
            case slvBINARY_MUL: opcode = slvOPCODE_MUL; break;
            case slvBINARY_DIV: opcode = slvOPCODE_DIV; break;

            default:
                gcmASSERT(0);
                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }

            status = slGenArithmeticExprCode(
                                            Compiler,
                                            BinaryExpr->exprBase.base.lineNo,
                                            BinaryExpr->exprBase.base.stringNo,
                                            opcode,
                                            &iOperand,
                                            leftParameters.rOperands + i,
                                            rightParameters.rOperands + i);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            slsROPERAND_InitializeUsingIOperand(Parameters->rOperands + i, &iOperand);
        }
    }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenRelationalCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;
    slsIOPERAND             intermIOperand;
    sleOPCODE               opcode;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &leftParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        gcmASSERT(leftParameters.operandCount == 1);
        gcmASSERT(rightParameters.operandCount == 1);

        switch (BinaryExpr->type)
        {
        case slvBINARY_LESS_THAN:           opcode = slvOPCODE_LESS_THAN;           break;
        case slvBINARY_LESS_THAN_EQUAL:     opcode = slvOPCODE_LESS_THAN_EQUAL;     break;
        case slvBINARY_GREATER_THAN:        opcode = slvOPCODE_GREATER_THAN;        break;
        case slvBINARY_GREATER_THAN_EQUAL:  opcode = slvOPCODE_GREATER_THAN_EQUAL;  break;

        default:
            gcmASSERT(0);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        /* less-than/less-than-equal/greater-than/greater-than-equal t0, operand */
        slsIOPERAND_New(Compiler, &intermIOperand, gcSHADER_BOOLEAN_X1);

        status = slGenGenericCode2(
                                Compiler,
                                BinaryExpr->exprBase.base.lineNo,
                                BinaryExpr->exprBase.base.stringNo,
                                opcode,
                                &intermIOperand,
                                &leftParameters.rOperands[0],
                                &rightParameters.rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Return t0 */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        BinaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &intermIOperand);
    }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenEqualityCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;
    slsIOPERAND             intermIOperand;
    slsLOPERAND             intermLOperand;
    slsROPERAND             constROperand;
    sleOPCODE               opcode;
    sleCONDITION            condition;
    slsSELECTION_CONTEXT    selectionContext;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &leftParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        slsIOPERAND_New(Compiler, &intermIOperand, gcSHADER_BOOLEAN_X1);

        if (slsDATA_TYPE_IsScalar(BinaryExpr->leftOperand->dataType))
        {
            /* Get the opcode */
            switch (BinaryExpr->type)
            {
            case slvBINARY_EQUAL:       opcode = slvOPCODE_EQUAL;       break;
            case slvBINARY_NOT_EQUAL:
            case slvBINARY_XOR:         opcode = slvOPCODE_NOT_EQUAL;   break;

            default:
                gcmASSERT(0);
                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }

            /* equal/not-equal t0, operand */
            status = slGenGenericCode2(
                                    Compiler,
                                    BinaryExpr->exprBase.base.lineNo,
                                    BinaryExpr->exprBase.base.stringNo,
                                    opcode,
                                    &intermIOperand,
                                    &leftParameters.rOperands[0],
                                    &rightParameters.rOperands[0]);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            gcmASSERT(leftParameters.operandCount == rightParameters.operandCount);

            slsLOPERAND_InitializeUsingIOperand(&intermLOperand, &intermIOperand);

            /* Get the condition code */
            switch (BinaryExpr->type)
            {
            case slvBINARY_EQUAL:
                condition = slvCONDITION_EQUAL;
                break;

            case slvBINARY_NOT_EQUAL:
            case slvBINARY_XOR:
                condition = slvCONDITION_NOT_EQUAL;
                break;

            default:
                gcmASSERT(0);
                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }

            /* Selection Begin */
            status = slDefineSelectionBegin(
                                            Compiler,
                                            CodeGenerator,
                                            gcvTRUE,
                                            &selectionContext);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* Generate the code of the condition expression */
            status = _GenMultiplyEqualityConditionCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        BinaryExpr->exprBase.base.lineNo,
                                                        BinaryExpr->exprBase.base.stringNo,
                                                        _GetSelectionConditionLabel(&selectionContext),
                                                        gcvFALSE,
                                                        condition,
                                                        leftParameters.operandCount,
                                                        leftParameters.dataTypes,
                                                        leftParameters.rOperands,
                                                        rightParameters.rOperands);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* Generate the code of the true operand */
            status = slDefineSelectionTrueOperandBegin(
                                                    Compiler,
                                                    CodeGenerator,
                                                    &selectionContext);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            slsROPERAND_InitializeBoolOrBVecConstant(&constROperand, gcSHADER_BOOLEAN_X1, gcvTRUE);

            status = slGenAssignCode(
                                    Compiler,
                                    BinaryExpr->exprBase.base.lineNo,
                                    BinaryExpr->exprBase.base.stringNo,
                                    &intermLOperand,
                                    &constROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            status = slDefineSelectionTrueOperandEnd(
                                                    Compiler,
                                                    CodeGenerator,
                                                    &selectionContext,
                                                    gcvFALSE);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* Generate the code of the false operand */
            status = slDefineSelectionFalseOperandBegin(
                                                    Compiler,
                                                    CodeGenerator,
                                                    &selectionContext);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            slsROPERAND_InitializeBoolOrBVecConstant(&constROperand, gcSHADER_BOOLEAN_X1, gcvFALSE);

            status = slGenAssignCode(
                                    Compiler,
                                    BinaryExpr->exprBase.base.lineNo,
                                    BinaryExpr->exprBase.base.stringNo,
                                    &intermLOperand,
                                    &constROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            status = slDefineSelectionFalseOperandEnd(
                                                    Compiler,
                                                    CodeGenerator,
                                                    &selectionContext);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* Selection End */
            status = slDefineSelectionEnd(
                                        Compiler,
                                        CodeGenerator,
                                        &selectionContext);

        }

        /* Return t0 */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        BinaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &intermIOperand);
    }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenAndCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsSELECTION_CONTEXT    selectionContext = {0};
    slsGEN_CODE_PARAMETERS  rightParameters;
    slsIOPERAND             intermIOperand;
    slsLOPERAND             intermLOperand;
    slsROPERAND             constROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(BinaryExpr->leftOperand);
    gcmASSERT(BinaryExpr->rightOperand);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    if (Parameters->needROperand)
    {
        gcmASSERT(BinaryExpr->exprBase.dataType);

        /* Allocate the operand(s) */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        BinaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmASSERT(Parameters->operandCount == 1);

        slsIOPERAND_New(Compiler, &intermIOperand, Parameters->dataTypes[0]);
        slsLOPERAND_InitializeUsingIOperand(&intermLOperand, &intermIOperand);
        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &intermIOperand);
    }

    /* Selection Begin */
    status = slDefineSelectionBegin(
                                    Compiler,
                                    CodeGenerator,
                                    Parameters->needROperand,
                                    &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the condition expression */
    status = _GenConditionCode(
                            Compiler,
                            CodeGenerator,
                            BinaryExpr->leftOperand,
                            _GetSelectionConditionLabel(&selectionContext),
                            gcvFALSE);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the true operand */
    status = slDefineSelectionTrueOperandBegin(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        status = slGenAssignCode(
                                Compiler,
                                BinaryExpr->exprBase.base.lineNo,
                                BinaryExpr->exprBase.base.stringNo,
                                &intermLOperand,
                                &rightParameters.rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    status = slDefineSelectionTrueOperandEnd(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext,
                                            gcvFALSE);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the false operand */
    if (Parameters->needROperand)
    {
        status = slDefineSelectionFalseOperandBegin(
                                                Compiler,
                                                CodeGenerator,
                                                &selectionContext);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsROPERAND_InitializeBoolOrBVecConstant(&constROperand, gcSHADER_BOOLEAN_X1, gcvFALSE);

        status = slGenAssignCode(
                                Compiler,
                                BinaryExpr->exprBase.base.lineNo,
                                BinaryExpr->exprBase.base.stringNo,
                                &intermLOperand,
                                &constROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = slDefineSelectionFalseOperandEnd(
                                                Compiler,
                                                CodeGenerator,
                                                &selectionContext);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    /* Selection End */
    status = slDefineSelectionEnd(
                                Compiler,
                                CodeGenerator,
                                &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenOrCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsSELECTION_CONTEXT    selectionContext = {0};
    slsGEN_CODE_PARAMETERS  rightParameters;
    slsIOPERAND             intermIOperand;
    slsLOPERAND             intermLOperand;
    slsROPERAND             constROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(BinaryExpr->leftOperand);
    gcmASSERT(BinaryExpr->rightOperand);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    if (Parameters->needROperand)
    {
        gcmASSERT(BinaryExpr->exprBase.dataType);

        /* Allocate the operand(s) */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        BinaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmASSERT(Parameters->operandCount == 1);

        slsIOPERAND_New(Compiler, &intermIOperand, Parameters->dataTypes[0]);
        slsLOPERAND_InitializeUsingIOperand(&intermLOperand, &intermIOperand);
        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &intermIOperand);
    }

    /* Selection Begin */
    status = slDefineSelectionBegin(
                                    Compiler,
                                    CodeGenerator,
                                    Parameters->needROperand,
                                    &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the condition expression */
    status = _GenConditionCode(
                            Compiler,
                            CodeGenerator,
                            BinaryExpr->leftOperand,
                            _GetSelectionConditionLabel(&selectionContext),
                            gcvTRUE);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the true operand */
    status = slDefineSelectionTrueOperandBegin(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        status = slGenAssignCode(
                                Compiler,
                                BinaryExpr->exprBase.base.lineNo,
                                BinaryExpr->exprBase.base.stringNo,
                                &intermLOperand,
                                &rightParameters.rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    status = slDefineSelectionTrueOperandEnd(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext,
                                            gcvFALSE);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the false operand */
    if (Parameters->needROperand)
    {
        status = slDefineSelectionFalseOperandBegin(
                                                Compiler,
                                                CodeGenerator,
                                                &selectionContext);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsROPERAND_InitializeBoolOrBVecConstant(&constROperand, gcSHADER_BOOLEAN_X1, gcvTRUE);

        status = slGenAssignCode(
                                Compiler,
                                BinaryExpr->exprBase.base.lineNo,
                                BinaryExpr->exprBase.base.stringNo,
                                &intermLOperand,
                                &constROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = slDefineSelectionFalseOperandEnd(
                                                Compiler,
                                                CodeGenerator,
                                                &selectionContext);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    /* Selection End */
    status = slDefineSelectionEnd(
                                Compiler,
                                CodeGenerator,
                                &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenSequenceCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &leftParameters,
                                    gcvFALSE,
                                    gcvFALSE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(
                                    &rightParameters,
                                    gcvFALSE,
                                    Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        slsGEN_CODE_PARAMETERS_MoveOperands(Parameters, &rightParameters);
    }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenAssignCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;
    gctUINT                 i;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(&leftParameters, gcvTRUE, Parameters->needROperand);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(&rightParameters, gcvFALSE, gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the assign code */
    gcmASSERT(leftParameters.operandCount == rightParameters.operandCount);

    for (i = 0; i < leftParameters.operandCount; i++)
    {
        status = slGenAssignCode(
                                Compiler,
                                BinaryExpr->exprBase.base.lineNo,
                                BinaryExpr->exprBase.base.stringNo,
                                leftParameters.lOperands + i,
                                rightParameters.rOperands + i);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    if (Parameters->needROperand)
    {
        slsGEN_CODE_PARAMETERS_MoveOperands(Parameters, &leftParameters);
    }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenArithmeticAssignCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;
    gctUINT                 i;
    slsIOPERAND             intermIOperand;
    slsROPERAND             intermROperand;
    sleOPCODE               opcode;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    slsGEN_CODE_PARAMETERS_Initialize(&leftParameters, gcvTRUE, gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->leftOperand->base,
                                &CodeGenerator->visitor,
                                &leftParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the right operand */
    gcmASSERT(BinaryExpr->rightOperand);

    slsGEN_CODE_PARAMETERS_Initialize(&rightParameters, gcvFALSE, gcvTRUE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &BinaryExpr->rightOperand->base,
                                &CodeGenerator->visitor,
                                &rightParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the arithmetic assign code */
    gcmASSERT(leftParameters.operandCount == rightParameters.operandCount);

    for (i = 0; i < leftParameters.operandCount; i++)
    {
        /* Generate the arithmetic code */
        slsIOPERAND_New(Compiler, &intermIOperand, leftParameters.dataTypes[i]);

        switch (BinaryExpr->type)
        {
        case slvBINARY_ADD_ASSIGN: opcode = slvOPCODE_ADD; break;
        case slvBINARY_SUB_ASSIGN: opcode = slvOPCODE_SUB; break;
        case slvBINARY_MUL_ASSIGN: opcode = slvOPCODE_MUL; break;
        case slvBINARY_DIV_ASSIGN: opcode = slvOPCODE_DIV; break;

        default:
            gcmASSERT(0);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        status = slGenArithmeticExprCode(
                                        Compiler,
                                        BinaryExpr->exprBase.base.lineNo,
                                        BinaryExpr->exprBase.base.stringNo,
                                        opcode,
                                        &intermIOperand,
                                        leftParameters.rOperands + i,
                                        rightParameters.rOperands + i);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

        /* Generate the assign code */
        status = slGenAssignCode(
                                Compiler,
                                BinaryExpr->exprBase.base.lineNo,
                                BinaryExpr->exprBase.base.stringNo,
                                leftParameters.lOperands + i,
                                &intermROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    if (Parameters->needROperand)
    {
        slsGEN_CODE_PARAMETERS_MoveOperands(Parameters, &leftParameters);
    }

    slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
    slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  leftParameters, rightParameters;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);

    /* Try to evaluate the operands */
    if (!Parameters->needLOperand && Parameters->needROperand)
    {
        /* Try to evaluate the left operand */
        gcmASSERT(BinaryExpr->leftOperand);

        slsGEN_CODE_PARAMETERS_Initialize(
                                        &leftParameters,
                                        gcvFALSE,
                                        gcvTRUE);

        leftParameters.hint = slvEVALUATE_ONLY;

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &BinaryExpr->leftOperand->base,
                                    &CodeGenerator->visitor,
                                    &leftParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Try to evaluate the right operand */
        gcmASSERT(BinaryExpr->rightOperand);

        slsGEN_CODE_PARAMETERS_Initialize(
                                        &rightParameters,
                                        gcvFALSE,
                                        gcvTRUE);

        rightParameters.hint = slvEVALUATE_ONLY;

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &BinaryExpr->rightOperand->base,
                                    &CodeGenerator->visitor,
                                    &rightParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (leftParameters.constant != gcvNULL && rightParameters.constant != gcvNULL)
        {
            status = sloIR_BINARY_EXPR_Evaluate(
                                                Compiler,
                                                BinaryExpr->type,
                                                leftParameters.constant,
                                                rightParameters.constant,
                                                &Parameters->constant);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            leftParameters.constant     = gcvNULL;
            rightParameters.constant    = gcvNULL;
        }

        slsGEN_CODE_PARAMETERS_Finalize(&leftParameters);
        slsGEN_CODE_PARAMETERS_Finalize(&rightParameters);

        if (Parameters->hint == slvEVALUATE_ONLY)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        if (Parameters->constant != gcvNULL)
        {
            status = sloIR_CONSTANT_GenCode(
                                        Compiler,
                                        CodeGenerator,
                                        Parameters->constant,
                                        Parameters);

            gcmFOOTER();
            return status;
        }
    }

    switch (BinaryExpr->type)
    {
    case slvBINARY_SUBSCRIPT:
        status = sloIR_BINARY_EXPR_GenSubscriptCode(
                                                Compiler,
                                                CodeGenerator,
                                                BinaryExpr,
                                                Parameters);

        break;

    case slvBINARY_ADD:
    case slvBINARY_SUB:
    case slvBINARY_MUL:
    case slvBINARY_DIV:
        status = sloIR_BINARY_EXPR_GenArithmeticCode(
                                                Compiler,
                                                CodeGenerator,
                                                BinaryExpr,
                                                Parameters);

        break;

    case slvBINARY_GREATER_THAN:
    case slvBINARY_LESS_THAN:
    case slvBINARY_GREATER_THAN_EQUAL:
    case slvBINARY_LESS_THAN_EQUAL:
        status = sloIR_BINARY_EXPR_GenRelationalCode(
                                                Compiler,
                                                CodeGenerator,
                                                BinaryExpr,
                                                Parameters);

        break;

    case slvBINARY_EQUAL:
    case slvBINARY_NOT_EQUAL:
    case slvBINARY_XOR:
        status = sloIR_BINARY_EXPR_GenEqualityCode(
                                                Compiler,
                                                CodeGenerator,
                                                BinaryExpr,
                                                Parameters);

        break;

    case slvBINARY_AND:
        status = sloIR_BINARY_EXPR_GenAndCode(
                                            Compiler,
                                            CodeGenerator,
                                            BinaryExpr,
                                            Parameters);

        break;

    case slvBINARY_OR:
        status = sloIR_BINARY_EXPR_GenOrCode(
                                            Compiler,
                                            CodeGenerator,
                                            BinaryExpr,
                                            Parameters);

        break;

    case slvBINARY_SEQUENCE:
        status = sloIR_BINARY_EXPR_GenSequenceCode(
                                                Compiler,
                                                CodeGenerator,
                                                BinaryExpr,
                                                Parameters);

        break;

    case slvBINARY_ASSIGN:
        status = sloIR_BINARY_EXPR_GenAssignCode(
                                            Compiler,
                                            CodeGenerator,
                                            BinaryExpr,
                                            Parameters);

        break;

    case slvBINARY_MUL_ASSIGN:
    case slvBINARY_DIV_ASSIGN:
    case slvBINARY_ADD_ASSIGN:
    case slvBINARY_SUB_ASSIGN:
        status = sloIR_BINARY_EXPR_GenArithmeticAssignCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        BinaryExpr,
                                                        Parameters);

        break;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
        break;
    }

    /* Restore vec from scalar array if vect is component indexing. */
    if ((BinaryExpr->leftOperand->base.vptr->type == slvIR_BINARY_EXPR) &&
        ((sloIR_BINARY_EXPR)(BinaryExpr->leftOperand))->u.vec2Array)
    {
        status = _ConvertAuxiScalarArrayToVec(
                                             Compiler,
                                             CodeGenerator,
                                             ((sloIR_BINARY_EXPR)(BinaryExpr->leftOperand))->u.vec2Array->scalarArrayName,
                                             &((sloIR_BINARY_EXPR)(BinaryExpr->leftOperand))->u.vec2Array->vecOperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = sloCOMPILER_Free(
                                  Compiler,
                                  ((sloIR_BINARY_EXPR)(BinaryExpr->leftOperand))->u.vec2Array);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        ((sloIR_BINARY_EXPR)(BinaryExpr->leftOperand))->u.vec2Array = gcvNULL;
    }

    if ((BinaryExpr->rightOperand->base.vptr->type == slvIR_BINARY_EXPR) &&
        ((sloIR_BINARY_EXPR)(BinaryExpr->rightOperand))->u.vec2Array)
    {
        status = _ConvertAuxiScalarArrayToVec(
                                             Compiler,
                                             CodeGenerator,
                                             ((sloIR_BINARY_EXPR)(BinaryExpr->rightOperand))->u.vec2Array->scalarArrayName,
                                             &((sloIR_BINARY_EXPR)(BinaryExpr->rightOperand))->u.vec2Array->vecOperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = sloCOMPILER_Free(
                                  Compiler,
                                  ((sloIR_BINARY_EXPR)(BinaryExpr->rightOperand))->u.vec2Array);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        ((sloIR_BINARY_EXPR)(BinaryExpr->rightOperand))->u.vec2Array = gcvNULL;
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_SELECTION_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_SELECTION Selection,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    gctBOOL                 emptySelection;
    slsGEN_CODE_PARAMETERS  condParameters, trueParameters, falseParameters;
    slsSELECTION_CONTEXT    selectionContext = {0};
    gctBOOL                 trueOperandHasReturn;
    slsIOPERAND             iOperand;
    slsLOPERAND             lOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(Selection, slvIR_SELECTION);
    gcmASSERT(Selection->condExpr);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    gcoOS_ZeroMemory(&trueParameters, gcmSIZEOF(trueParameters));
    gcoOS_ZeroMemory(&falseParameters, gcmSIZEOF(falseParameters));

    if (Parameters->hint == slvEVALUATE_ONLY)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    emptySelection = (Selection->trueOperand == gcvNULL && Selection->falseOperand == gcvNULL);

    if (emptySelection)
    {
        gcmASSERT(!Parameters->needROperand);

        /* Only generate the code of the condition expression */
        slsGEN_CODE_PARAMETERS_Initialize(&condParameters, gcvFALSE, gcvFALSE);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &Selection->condExpr->base,
                                    &CodeGenerator->visitor,
                                    &condParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Finalize(&condParameters);

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Selection Begin */
    status = slDefineSelectionBegin(
                                    Compiler,
                                    CodeGenerator,
                                    (Selection->falseOperand != gcvNULL),
                                    &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Parameters->needROperand)
    {
        gcmASSERT(Selection->exprBase.dataType);

        /* Allocate the operand(s) */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        Selection->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmASSERT(Parameters->operandCount == 1);

        slsIOPERAND_New(Compiler, &iOperand, Parameters->dataTypes[0]);
        slsLOPERAND_InitializeUsingIOperand(&lOperand, &iOperand);
        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &iOperand);
    }

    /* Generate the code of the condition expression */
    status = _GenConditionCode(
                            Compiler,
                            CodeGenerator,
                            Selection->condExpr,
                            _GetSelectionConditionLabel(&selectionContext),
                            gcvFALSE);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the true operand */
    status = slDefineSelectionTrueOperandBegin(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Selection->trueOperand != gcvNULL)
    {
        slsGEN_CODE_PARAMETERS_Initialize(&trueParameters, gcvFALSE, Parameters->needROperand);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    Selection->trueOperand,
                                    &CodeGenerator->visitor,
                                    &trueParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (Parameters->needROperand)
        {
            status = slGenAssignCode(
                                    Compiler,
                                    Selection->trueOperand->lineNo,
                                    Selection->trueOperand->stringNo,
                                    &lOperand,
                                    &trueParameters.rOperands[0]);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
    }

    trueOperandHasReturn = (Selection->trueOperand != gcvNULL
                            && sloIR_BASE_HasReturn(Compiler, Selection->trueOperand));

    status = slDefineSelectionTrueOperandEnd(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext,
                                            trueOperandHasReturn);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of the false operand */
    if (Selection->falseOperand != gcvNULL)
    {
        status = slDefineSelectionFalseOperandBegin(
                                                Compiler,
                                                CodeGenerator,
                                                &selectionContext);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        slsGEN_CODE_PARAMETERS_Initialize(&falseParameters, gcvFALSE, Parameters->needROperand);

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    Selection->falseOperand,
                                    &CodeGenerator->visitor,
                                    &falseParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (Parameters->needROperand)
        {
            status = slGenAssignCode(
                                    Compiler,
                                    Selection->falseOperand->lineNo,
                                    Selection->falseOperand->stringNo,
                                    &lOperand,
                                    &falseParameters.rOperands[0]);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }

        status = slDefineSelectionFalseOperandEnd(
                                                Compiler,
                                                CodeGenerator,
                                                &selectionContext);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    /* Selection End */
    status = slDefineSelectionEnd(
                                Compiler,
                                CodeGenerator,
                                &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    if (Selection->trueOperand != gcvNULL)  slsGEN_CODE_PARAMETERS_Finalize(&trueParameters);
    if (Selection->falseOperand != gcvNULL) slsGEN_CODE_PARAMETERS_Finalize(&falseParameters);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenOperandsCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctBOOL NeedROperand,
    OUT gctUINT * OperandCount,
    OUT slsGEN_CODE_PARAMETERS * * OperandsParameters
    )
{
    gceSTATUS                   status;
    gctUINT                     operandCount;
    slsGEN_CODE_PARAMETERS *    operandsParameters;
    sloIR_EXPR                  operand;
    gctUINT                     i = 0;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount);
    gcmASSERT(OperandsParameters);

    if (PolynaryExpr->operands == gcvNULL)
    {
        *OperandCount       = 0;
        *OperandsParameters = gcvNULL;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    do
    {
        gctPOINTER pointer = gcvNULL;

        gcmVERIFY_OK(sloIR_SET_GetMemberCount(
                                            Compiler,
                                            PolynaryExpr->operands,
                                            &operandCount));

        gcmASSERT(operandCount > 0);

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsGEN_CODE_PARAMETERS) * operandCount,
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        operandsParameters = pointer;

        FOR_EACH_DLINK_NODE(&PolynaryExpr->operands->members, struct _sloIR_EXPR, operand)
        {
            slsGEN_CODE_PARAMETERS_Initialize(&operandsParameters[i], gcvFALSE, NeedROperand);

            status = sloIR_OBJECT_Accept(
                                        Compiler,
                                        &operand->base,
                                        &CodeGenerator->visitor,
                                        &operandsParameters[i]);

            if (gcmIS_ERROR(status)) break;

            i++;
        }

        if (gcmIS_ERROR(status)) break;

        *OperandCount       = operandCount;
        *OperandsParameters = operandsParameters;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *OperandCount       = 0;
    *OperandsParameters = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenOperandsCodeForFuncCall(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    OUT gctUINT * OperandCount,
    OUT slsGEN_CODE_PARAMETERS * * OperandsParameters
    )
{
    gceSTATUS                   status;
    gctUINT                     operandCount;
    slsGEN_CODE_PARAMETERS *    operandsParameters;
    sloIR_EXPR                  operand;
    gctUINT                     i = 0;
    slsNAME *                   paramName;
    gctBOOL                     needLOperand, needROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount);
    gcmASSERT(OperandsParameters);

    if (PolynaryExpr->operands == gcvNULL)
    {
        *OperandCount       = 0;
        *OperandsParameters = gcvNULL;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    do
    {
        gctPOINTER pointer = gcvNULL;

        gcmVERIFY_OK(sloIR_SET_GetMemberCount(
                                            Compiler,
                                            PolynaryExpr->operands,
                                            &operandCount));

        gcmASSERT(operandCount > 0);

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsGEN_CODE_PARAMETERS) * operandCount,
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        operandsParameters = pointer;

        for (paramName = slsDLINK_LIST_First(
                                &PolynaryExpr->funcName->u.funcInfo.localSpace->names, slsNAME),
                operand = slsDLINK_LIST_First(
                                &PolynaryExpr->operands->members, struct _sloIR_EXPR);
            (slsDLINK_NODE *)paramName != &PolynaryExpr->funcName->u.funcInfo.localSpace->names;
            paramName = slsDLINK_NODE_Next(&paramName->node, slsNAME),
                operand = slsDLINK_NODE_Next(&operand->base.node, struct _sloIR_EXPR))
        {
            if (paramName->type != slvPARAMETER_NAME) break;

            gcmASSERT((slsDLINK_NODE *)operand != &PolynaryExpr->operands->members);

            switch (paramName->dataType->qualifier)
            {
            case slvQUALIFIER_CONST_IN:
            case slvQUALIFIER_IN:
                needLOperand = gcvFALSE;
                needROperand = gcvTRUE;
                break;

            case slvQUALIFIER_OUT:
                needLOperand = gcvTRUE;
                needROperand = gcvFALSE;
                break;

            case slvQUALIFIER_INOUT:
                needLOperand = gcvTRUE;
                needROperand = gcvTRUE;
                break;

            default:
                gcmASSERT(0);
                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }

			if(i >= operandCount)
			{
				*OperandCount       = 0;
				*OperandsParameters = gcvNULL;
				gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
			}
            slsGEN_CODE_PARAMETERS_Initialize(&operandsParameters[i], needLOperand, needROperand);

            status = sloIR_OBJECT_Accept(
                                        Compiler,
                                        &operand->base,
                                        &CodeGenerator->visitor,
                                        &operandsParameters[i]);

            if (gcmIS_ERROR(status)) break;

            i++;
        }

        if (gcmIS_ERROR(status)) break;

        *OperandCount       = operandCount;
        *OperandsParameters = operandsParameters;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *OperandCount       = 0;
    *OperandsParameters = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_POLYNARY_EXPR_FinalizeOperandsParameters(
    IN sloCOMPILER Compiler,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters
    )
{
    gctUINT     i;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (OperandCount == 0)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    gcmASSERT(OperandsParameters);

    for (i = 0; i < OperandCount; i++)
    {
        slsGEN_CODE_PARAMETERS_Finalize(&OperandsParameters[i]);
    }

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, OperandsParameters));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}


static slsCOMPONENT_SELECTION
_GetComponentSelectionSlice(
    IN slsCOMPONENT_SELECTION ComponentSelection,
    IN gctUINT8 StartComponent,
    IN gctUINT8 SliceComponentCount
    )
{
    slsCOMPONENT_SELECTION resultComponentSelection = { 0 };

    gcmASSERT((StartComponent + SliceComponentCount) <= ComponentSelection.components);

    resultComponentSelection.components = SliceComponentCount;

    switch (StartComponent)
    {
    case 0:
        resultComponentSelection.x = ComponentSelection.x;
        if (SliceComponentCount > 1) resultComponentSelection.y = ComponentSelection.y;
        if (SliceComponentCount > 2) resultComponentSelection.z = ComponentSelection.z;
        if (SliceComponentCount > 3) resultComponentSelection.w = ComponentSelection.w;
        break;

    case 1:
        resultComponentSelection.x = ComponentSelection.y;
        if (SliceComponentCount > 1) resultComponentSelection.y = ComponentSelection.z;
        if (SliceComponentCount > 2) resultComponentSelection.z = ComponentSelection.w;
        break;

    case 2:
        resultComponentSelection.x = ComponentSelection.z;
        if (SliceComponentCount > 1) resultComponentSelection.y = ComponentSelection.w;
        break;

    case 3:
        resultComponentSelection.x = ComponentSelection.w;
        break;

    default:
        gcmASSERT(0);
    }

    return resultComponentSelection;
}

static void
_GetOperandConstantSlice(
    IN slsOPERAND_CONSTANT * OperandConstant,
    IN gctUINT8 StartComponent,
    IN gctUINT8 SliceComponentCount,
    IN gcSHADER_TYPE SliceDataType,
    OUT slsOPERAND_CONSTANT * ResultOperandConstant
    )
{
    gctUINT     i;

    /* Verify the arguments. */
    gcmASSERT(OperandConstant);
    gcmASSERT(ResultOperandConstant);
    gcmASSERT((StartComponent + SliceComponentCount) <= (gctUINT8)OperandConstant->valueCount);

    ResultOperandConstant->dataType     = SliceDataType;
    ResultOperandConstant->valueCount   = SliceComponentCount;

    for (i = 0; i < SliceComponentCount; i++)
    {
        ResultOperandConstant->values[i]    = OperandConstant->values[StartComponent + i];
    }
}

static void
_GetVectorROperandSlice(
    IN slsROPERAND * ROperand,
    IN gctUINT8 StartComponent,
    IN gctUINT8 RequiredComponentCount,
    OUT slsROPERAND * ROperandSlice,
    OUT gctUINT8 * SliceComponentCount
    )
{
    gctUINT8        sliceComponentCount;
    gcSHADER_TYPE   sliceDataType;

    /* Verify the arguments. */
    gcmASSERT(ROperand);
    gcmASSERT(gcIsVectorDataType(ROperand->dataType));
    gcmASSERT(ROperandSlice);
    gcmASSERT(SliceComponentCount);

    sliceComponentCount = gcGetVectorDataTypeComponentCount(ROperand->dataType) - StartComponent;

    if (sliceComponentCount > RequiredComponentCount)
    {
        sliceComponentCount = RequiredComponentCount;
    }

    sliceDataType = gcGetVectorSliceDataType(ROperand->dataType, sliceComponentCount);

    *ROperandSlice          = *ROperand;
    ROperandSlice->dataType = sliceDataType;

    if (sliceComponentCount == 1)
    {
        gcmASSERT(ROperandSlice->vectorIndex.mode == slvINDEX_NONE);

        ROperandSlice->vectorIndex.mode         = slvINDEX_CONSTANT;
        ROperandSlice->vectorIndex.u.constant   = (gctREG_INDEX)StartComponent;
    }
    else
    {
        if (ROperandSlice->isReg)
        {
            ROperandSlice->u.reg.componentSelection =
                        _GetComponentSelectionSlice(
                                                ROperand->u.reg.componentSelection,
                                                StartComponent,
                                                sliceComponentCount);
        }
        else
        {
            _GetOperandConstantSlice(
                                    &ROperand->u.constant,
                                    StartComponent,
                                    sliceComponentCount,
                                    sliceDataType,
                                    &ROperandSlice->u.constant);
        }
    }

    *SliceComponentCount = sliceComponentCount;
}

static gctBOOL
_GetROperandSlice(
    IN slsROPERAND * ROperand,
    IN OUT gctUINT8 * StartComponent,
    IN OUT gctUINT8 * RequiredComponentCount,
    OUT slsROPERAND * ROperandSlice,
    OUT gctUINT8 * SliceComponentCount
    )
{
    gctUINT8        componentCount, sliceComponentCount;
    gctUINT         matrixColumnCount, matrixIndex;
    slsROPERAND     matrixColumnROperand;

    /* Verify the arguments. */
    gcmASSERT(ROperand);
    gcmASSERT(StartComponent);
    gcmASSERT(RequiredComponentCount);
    gcmASSERT(*RequiredComponentCount > 0);
    gcmASSERT(ROperandSlice);

    if (gcIsScalarDataType(ROperand->dataType))
    {
        if (*StartComponent > 0) return gcvFALSE;

        *ROperandSlice = *ROperand;

        sliceComponentCount = 1;
    }
    else if (gcIsVectorDataType(ROperand->dataType))
    {
        componentCount = gcGetVectorDataTypeComponentCount(ROperand->dataType);

        if (*StartComponent > componentCount - 1) return gcvFALSE;

        _GetVectorROperandSlice(
                                ROperand,
                                *StartComponent,
                                *RequiredComponentCount,
                                ROperandSlice,
                                &sliceComponentCount);
    }
    else
    {
        gcmASSERT(gcIsMatrixDataType(ROperand->dataType));

        matrixColumnCount = gcGetMatrixDataTypeColumnCount(ROperand->dataType);

        if (*StartComponent > matrixColumnCount * matrixColumnCount - 1) return gcvFALSE;

        matrixIndex = *StartComponent / matrixColumnCount;

        slsROPERAND_InitializeAsMatrixColumn(
                                            &matrixColumnROperand,
                                            ROperand,
                                            matrixIndex);

        _GetVectorROperandSlice(
                                &matrixColumnROperand,
                                *StartComponent - (gctUINT8) (matrixIndex * matrixColumnCount),
                                *RequiredComponentCount,
                                ROperandSlice,
                                &sliceComponentCount);
    }

    (*StartComponent)           += sliceComponentCount;
    (*RequiredComponentCount)   -= sliceComponentCount;

    if (SliceComponentCount != gcvNULL) *SliceComponentCount = sliceComponentCount;

    return gcvTRUE;
}

static void
_GetVectorLOperandSlice(
    IN slsLOPERAND * LOperand,
    IN gctUINT8 StartComponent,
    IN gctUINT8 SliceComponentCount,
    OUT slsLOPERAND * LOperandSlice
    )
{
    gcSHADER_TYPE   sliceDataType;

    /* Verify the arguments. */
    gcmASSERT(LOperand);
    gcmASSERT(gcIsVectorDataType(LOperand->dataType));
    gcmASSERT(SliceComponentCount > 0);
    gcmASSERT(LOperandSlice);

    sliceDataType = gcGetVectorSliceDataType(LOperand->dataType, SliceComponentCount);

    *LOperandSlice          = *LOperand;
    LOperandSlice->dataType = sliceDataType;

    if (SliceComponentCount == 1)
    {
        gcmASSERT(LOperandSlice->vectorIndex.mode == slvINDEX_NONE);

        LOperandSlice->vectorIndex.mode         = slvINDEX_CONSTANT;
        LOperandSlice->vectorIndex.u.constant   = (gctREG_INDEX)StartComponent;
    }
    else
    {
        LOperandSlice->reg.componentSelection   =
                    _GetComponentSelectionSlice(LOperand->reg.componentSelection,
                                                StartComponent,
                                                SliceComponentCount);
    }
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenConstructScalarCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS                       status;
    gctUINT                         operandCount;
    slsGEN_CODE_PARAMETERS *        operandsParameters;
    gctBOOL                         treatFloatAsInt;
    gctUINT8                        startComponent = 0;
    gctUINT8                        requiredComponentCount = 1;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    status = sloIR_POLYNARY_EXPR_GenOperandsCode(
                                                Compiler,
                                                CodeGenerator,
                                                PolynaryExpr,
                                                Parameters->needROperand,
                                                &operandCount,
                                                &operandsParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmASSERT(operandCount == 1);

    treatFloatAsInt = (Parameters->hint == slvGEN_INDEX_CODE
                        || operandsParameters[0].treatFloatAsInt);

    if (Parameters->needROperand)
    {
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        PolynaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmASSERT(Parameters->operandCount == 1);

        _GetROperandSlice(
                        &operandsParameters[0].rOperands[0],
                        &startComponent,
                        &requiredComponentCount,
                        &Parameters->rOperands[0],
                        gcvNULL);

        status = slsROPERAND_ChangeDataTypeFamily(
                                                Compiler,
                                                PolynaryExpr->exprBase.base.lineNo,
                                                PolynaryExpr->exprBase.base.stringNo,
                                                treatFloatAsInt,
                                                Parameters->dataTypes[0],
                                                &Parameters->rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmVERIFY_OK(sloIR_POLYNARY_EXPR_FinalizeOperandsParameters(
                                                                Compiler,
                                                                operandCount,
                                                                operandsParameters));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

typedef struct _slsOPERANDS_LOCATION
{
    gctUINT         currentOperand;

    gctUINT8        startComponent;
}
slsOPERANDS_LOCATION;

#define slsOPERANDS_LOCATION_Initalize(location) \
    do \
    { \
        (location)->currentOperand  = 0; \
        (location)->startComponent  = 0; \
    } \
    while (gcvFALSE)

gceSTATUS
sloIR_POLYNARY_EXPR_GenVectorComponentAssignCode(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand,
    IN OUT slsOPERANDS_LOCATION * Location
    )
{
    gceSTATUS       status;
    gctUINT8        lOperandStartComponent = 0;
    gctUINT8        requiredComponentCount;
    slsROPERAND     rOperandSlice;
    gctUINT8        sliceComponentCount;
    slsLOPERAND     lOperand, lOperandSlice;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount > 0);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);
    gcmASSERT(gcIsVectorDataType(IOperand->dataType));
    gcmASSERT(Location);

    requiredComponentCount = gcGetVectorDataTypeComponentCount(IOperand->dataType);

    slsLOPERAND_InitializeUsingIOperand(&lOperand, IOperand);

    while (requiredComponentCount > 0)
    {
        gcmASSERT(Location->currentOperand < OperandCount);
        gcmASSERT(OperandsParameters[Location->currentOperand].operandCount == 1);

        if (!_GetROperandSlice(
                            &OperandsParameters[Location->currentOperand].rOperands[0],
                            &Location->startComponent,
                            &requiredComponentCount,
                            &rOperandSlice,
                            &sliceComponentCount))
        {
            Location->currentOperand++;
            Location->startComponent = 0;
            continue;
        }

        _GetVectorLOperandSlice(
                                &lOperand,
                                lOperandStartComponent,
                                sliceComponentCount,
                                &lOperandSlice);

        lOperandStartComponent += sliceComponentCount;

        status = slsROPERAND_ChangeDataTypeFamily(
                                                Compiler,
                                                PolynaryExpr->exprBase.base.lineNo,
                                                PolynaryExpr->exprBase.base.stringNo,
                                                OperandsParameters[
                                                    Location->currentOperand].treatFloatAsInt,
                                                lOperandSlice.dataType,
                                                &rOperandSlice);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = slGenAssignCode(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                &lOperandSlice,
                                &rOperandSlice);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenMatrixComponentAssignCode(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS               status;
    gctUINT                 matrixColumnCount, i;
    slsOPERANDS_LOCATION    location;
    slsIOPERAND             matrixColumnIOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount > 0);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);
    gcmASSERT(gcIsMatrixDataType(IOperand->dataType));

    matrixColumnCount = gcGetMatrixDataTypeColumnCount(IOperand->dataType);

    slsOPERANDS_LOCATION_Initalize(&location);

    for (i = 0; i < matrixColumnCount; i++)
    {
        matrixColumnIOperand.dataType       = gcGetMatrixColumnDataType(IOperand->dataType);
        matrixColumnIOperand.tempRegIndex   = (gctREG_INDEX) (IOperand->tempRegIndex + i);

        status = sloIR_POLYNARY_EXPR_GenVectorComponentAssignCode(
                                                                Compiler,
                                                                PolynaryExpr,
                                                                OperandCount,
                                                                OperandsParameters,
                                                                &matrixColumnIOperand,
                                                                &location);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenScalarToVectorAssignCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN gctBOOL TreatFloatAsInt,
    IN slsROPERAND * ScalarROperand,
    IN slsIOPERAND * VectorIOperand,
    OUT slsROPERAND * VectorROperand
    )
{
    gceSTATUS       status;
    slsROPERAND     scalarROperand;
    slsLOPERAND     vectorLOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ScalarROperand);
    gcmASSERT(VectorIOperand);
    gcmASSERT(VectorROperand);

    scalarROperand = *ScalarROperand;

    status = slsROPERAND_ChangeDataTypeFamily(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            TreatFloatAsInt,
                                            gcGetVectorComponentDataType(VectorIOperand->dataType),
                                            &scalarROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsLOPERAND_InitializeTempReg(
                                &vectorLOperand,
                                slvQUALIFIER_NONE,
                                scalarROperand.dataType,
                                VectorIOperand->tempRegIndex);

    status = slGenAssignCode(
                            Compiler,
                            LineNo,
                            StringNo,
                            &vectorLOperand,
                            &scalarROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(VectorROperand, VectorIOperand);

    VectorROperand->u.reg.componentSelection.w =
        VectorROperand->u.reg.componentSelection.z =
        VectorROperand->u.reg.componentSelection.y =
            VectorROperand->u.reg.componentSelection.x;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenConstructVectorCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS                       status;
    gctUINT                         operandCount;
    slsGEN_CODE_PARAMETERS *        operandsParameters;
    slsIOPERAND                     iOperand;
    slsOPERANDS_LOCATION            location;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    status = sloIR_POLYNARY_EXPR_GenOperandsCode(
                                                Compiler,
                                                CodeGenerator,
                                                PolynaryExpr,
                                                Parameters->needROperand,
                                                &operandCount,
                                                &operandsParameters);

    gcmASSERT(operandCount > 0);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    gcmASSERT(operandsParameters);
    if (operandsParameters == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Parameters->needROperand)
    {
        /* Allocate the operand(s) */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        PolynaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmASSERT(Parameters->operandCount == 1);

        if (operandCount == 1 && operandsParameters[0].operandCount == 1
            && gcIsScalarDataType(operandsParameters[0].dataTypes[0]))
        {
            if (!operandsParameters[0].rOperands[0].isReg)
            {
                /* Convert the scalar constant to the vector constant */
                Parameters->rOperands[0] = operandsParameters[0].rOperands[0];

                slsROPERAND_CONSTANT_ConvScalarToVector(
                                                    Compiler,
                                                    Parameters->dataTypes[0],
                                                    &Parameters->rOperands[0]);
            }
            else
            {
                /* Generate the special assignment */
                slsIOPERAND_New(Compiler, &iOperand, Parameters->dataTypes[0]);

                slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &iOperand);

                status = _GenScalarToVectorAssignCode(
                                                    Compiler,
                                                    PolynaryExpr->exprBase.base.lineNo,
                                                    PolynaryExpr->exprBase.base.stringNo,
                                                    operandsParameters[0].treatFloatAsInt,
                                                    &operandsParameters[0].rOperands[0],
                                                    &iOperand,
                                                    &Parameters->rOperands[0]);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
        }
        else
        {
            /* Generate the assignment(s) */
            slsIOPERAND_New(Compiler, &iOperand, Parameters->dataTypes[0]);

            slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &iOperand);

            slsOPERANDS_LOCATION_Initalize(&location);

            status = sloIR_POLYNARY_EXPR_GenVectorComponentAssignCode(
                                                                    Compiler,
                                                                    PolynaryExpr,
                                                                    operandCount,
                                                                    operandsParameters,
                                                                    &iOperand,
                                                                    &location);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
    }

    gcmVERIFY_OK(sloIR_POLYNARY_EXPR_FinalizeOperandsParameters(
                                                                Compiler,
                                                                operandCount,
                                                                operandsParameters));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenScalarToMatrixAssignCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsROPERAND * ScalarROperand,
    IN slsIOPERAND * MatrixIOperand
    )
{
    gceSTATUS       status;
    gctUINT         i, j;
    slsROPERAND     scalarROperand;
    slsLOPERAND     matrixLOperand, componentLOperand;
    slsROPERAND     zeroROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ScalarROperand);
    gcmASSERT(MatrixIOperand);

    scalarROperand = *ScalarROperand;

    status = slsROPERAND_ChangeDataTypeFamily(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            gcvFALSE,
                                            gcSHADER_FLOAT_X1,
                                            &scalarROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsLOPERAND_InitializeUsingIOperand(&matrixLOperand, MatrixIOperand);
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &zeroROperand,
                                                gcSHADER_FLOAT_X1,
                                                (gctFLOAT)0.0);

    for (i = 0; i < gcGetMatrixDataTypeColumnCount(MatrixIOperand->dataType); i++)
    {
        for (j = 0; j < gcGetMatrixDataTypeColumnCount(MatrixIOperand->dataType); j++)
        {
            slsLOPERAND_InitializeAsMatrixComponent(&componentLOperand, &matrixLOperand, i, j);

            if (i == j)
            {
                status = slGenAssignCode(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        &componentLOperand,
                                        &scalarROperand);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
            else
            {
                status = slGenAssignCode(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        &componentLOperand,
                                        &zeroROperand);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenMatrixToMatrixAssignCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsROPERAND * MatrixROperand,
    IN slsIOPERAND * MatrixIOperand
    )
{
    gceSTATUS       status;
    gctUINT         targetMatrixColumnCount, sourceMatrixColumnCount;
    gctUINT         i, j;
    slsLOPERAND     matrixLOperand, columnLOperand, columnSliceLOperand, componentLOperand;
    slsROPERAND     columnROperand, columnSliceROperand, oneROperand, zeroROperand;
    gctUINT8        sliceComponentCount;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(MatrixROperand);
    gcmASSERT(MatrixIOperand);

    sourceMatrixColumnCount = gcGetMatrixDataTypeColumnCount(MatrixROperand->dataType);
    targetMatrixColumnCount = gcGetMatrixDataTypeColumnCount(MatrixIOperand->dataType);

    slsLOPERAND_InitializeUsingIOperand(&matrixLOperand, MatrixIOperand);
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &oneROperand,
                                                gcSHADER_FLOAT_X1,
                                                (gctFLOAT)1.0);
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &zeroROperand,
                                                gcSHADER_FLOAT_X1,
                                                (gctFLOAT)0.0);

    for (i = 0; i < targetMatrixColumnCount; i++)
    {
        slsLOPERAND_InitializeAsMatrixColumn(&columnLOperand, &matrixLOperand, i);
        slsROPERAND_InitializeAsMatrixColumn(&columnROperand, MatrixROperand, i);

        if (targetMatrixColumnCount == sourceMatrixColumnCount)
        {
            status = slGenAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    &columnLOperand,
                                    &columnROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else if (targetMatrixColumnCount < sourceMatrixColumnCount)
        {
            _GetVectorROperandSlice(
                                    &columnROperand,
                                    0,
                                    (gctUINT8) targetMatrixColumnCount,
                                    &columnSliceROperand,
                                    &sliceComponentCount);

            gcmASSERT(sliceComponentCount == targetMatrixColumnCount);

            status = slGenAssignCode(
                                    Compiler,
                                    LineNo,
                                    StringNo,
                                    &columnLOperand,
                                    &columnSliceROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            if (i < sourceMatrixColumnCount)
            {
                _GetVectorLOperandSlice(
                                        &columnLOperand,
                                        0,
                                        (gctUINT8) sourceMatrixColumnCount,
                                        &columnSliceLOperand);

                status = slGenAssignCode(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        &columnSliceLOperand,
                                        &columnROperand);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

                j = sourceMatrixColumnCount;
            }
            else
            {
                j = 0;
            }

            for (; j < targetMatrixColumnCount; j++)
            {
                slsLOPERAND_InitializeAsMatrixComponent(&componentLOperand, &matrixLOperand, i, j);

                if (i == j)
                {
                    status = slGenAssignCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            &componentLOperand,
                                            &oneROperand);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
                else
                {
                    status = slGenAssignCode(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            &componentLOperand,
                                            &zeroROperand);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
            }   /* for */
        }   /* if */
    }   /* for */

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenConstructMatrixCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS                       status;
    gctUINT                         operandCount;
    slsGEN_CODE_PARAMETERS *        operandsParameters;
    slsIOPERAND                     iOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    status = sloIR_POLYNARY_EXPR_GenOperandsCode(
                                                Compiler,
                                                CodeGenerator,
                                                PolynaryExpr,
                                                Parameters->needROperand,
                                                &operandCount,
                                                &operandsParameters);

    gcmASSERT(operandCount > 0);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    gcmASSERT(operandsParameters);
    if (operandsParameters == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Parameters->needROperand)
    {
        /* Allocate the register(s) */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        PolynaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmASSERT(Parameters->operandCount == 1);

        slsIOPERAND_New(Compiler, &iOperand, Parameters->dataTypes[0]);

        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &iOperand);

        /* Generate the assignment(s) */
        if (operandCount == 1 && operandsParameters[0].operandCount == 1
            && gcIsScalarDataType(operandsParameters[0].dataTypes[0]))
        {
            status = _GenScalarToMatrixAssignCode(
                                                Compiler,
                                                PolynaryExpr->exprBase.base.lineNo,
                                                PolynaryExpr->exprBase.base.stringNo,
                                                &operandsParameters[0].rOperands[0],
                                                &iOperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else if (operandCount == 1 && operandsParameters[0].operandCount == 1
            && gcIsMatrixDataType(operandsParameters[0].dataTypes[0]))
        {
            status = _GenMatrixToMatrixAssignCode(
                                                Compiler,
                                                PolynaryExpr->exprBase.base.lineNo,
                                                PolynaryExpr->exprBase.base.stringNo,
                                                &operandsParameters[0].rOperands[0],
                                                &iOperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            status = sloIR_POLYNARY_EXPR_GenMatrixComponentAssignCode(
                                                                    Compiler,
                                                                    PolynaryExpr,
                                                                    operandCount,
                                                                    operandsParameters,
                                                                    &iOperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
    }

    gcmVERIFY_OK(sloIR_POLYNARY_EXPR_FinalizeOperandsParameters(
                                                                Compiler,
                                                                operandCount,
                                                                operandsParameters));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenConstructStructCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS                       status;
    gctUINT                         operandCount;
    slsGEN_CODE_PARAMETERS *        operandsParameters;
    gctUINT                         i, j, k;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    status = sloIR_POLYNARY_EXPR_GenOperandsCode(
                                                Compiler,
                                                CodeGenerator,
                                                PolynaryExpr,
                                                Parameters->needROperand,
                                                &operandCount,
                                                &operandsParameters);

    gcmASSERT(operandCount > 0);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    gcmASSERT(operandsParameters);
    if (operandsParameters == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Parameters->needROperand)
    {
        /* Allocate the register(s) */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(
                                                        Compiler,
                                                        Parameters,
                                                        PolynaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Copy all operands */
        for (i = 0, j = 0, k = 0; i < Parameters->operandCount; i++, k++)
        {
            if (k == operandsParameters[j].operandCount)
            {
                k = 0;
                j++;
            }

            gcmASSERT(j < operandCount);
            gcmASSERT(k < operandsParameters[j].operandCount);
            Parameters->rOperands[i] = operandsParameters[j].rOperands[k];
        }
    }

    gcmVERIFY_OK(sloIR_POLYNARY_EXPR_FinalizeOperandsParameters(
                                                                Compiler,
                                                                operandCount,
                                                                operandsParameters));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenBuiltInCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS                       status;
    gctUINT                         operandCount;
    slsGEN_CODE_PARAMETERS *        operandsParameters;
    slsIOPERAND                     iOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Generate the code of all operands */
    status = sloIR_POLYNARY_EXPR_GenOperandsCode(Compiler,
                                                 CodeGenerator,
                                                 PolynaryExpr,
                                                 Parameters->needROperand,
                                                 &operandCount,
                                                 &operandsParameters);

    gcmASSERT(operandCount > 0);

    if (Parameters->needROperand)
    {
        /* Allocate the register(s) */
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(Compiler,
                                                         Parameters,
                                                         PolynaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmASSERT(Parameters->operandCount == 1);

        slsIOPERAND_New(Compiler, &iOperand, Parameters->dataTypes[0]);

        slsROPERAND_InitializeUsingIOperand(&Parameters->rOperands[0], &iOperand);

        /* Generate the built-in code */
        status = slGenBuiltInFunctionCode(Compiler,
                                          CodeGenerator,
                                          PolynaryExpr,
                                          operandCount,
                                          operandsParameters,
                                          &iOperand,
                                          Parameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmVERIFY_OK(sloIR_POLYNARY_EXPR_FinalizeOperandsParameters(Compiler,
                                                                operandCount,
                                                                operandsParameters));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenFuncCallCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS                   status;
    gctUINT                     operandCount, i, j;
    slsGEN_CODE_PARAMETERS *    operandsParameters;
    slsNAME *                   paramName;
    slsLOPERAND                 lOperand;
    slsROPERAND                 rOperand;
    slsIOPERAND                 intermIOperand;
    slsLOPERAND                 intermLOperand;
    slsROPERAND                 intermROperand;
    gctLABEL                    funcLabel;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);
    gcmASSERT(!Parameters->needLOperand);

    /* Allocate the function resources */
    status = _AllocateFuncResources(Compiler,
                                    CodeGenerator,
                                    PolynaryExpr->funcName);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Generate the code of all operands */
    status = sloIR_POLYNARY_EXPR_GenOperandsCodeForFuncCall(Compiler,
                                                            CodeGenerator,
                                                            PolynaryExpr,
                                                            &operandCount,
                                                            &operandsParameters);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Set all 'in' arguments */
    i = 0;
    FOR_EACH_DLINK_NODE(&PolynaryExpr->funcName->u.funcInfo.localSpace->names, slsNAME, paramName)
    {
        if (paramName->type != slvPARAMETER_NAME) break;

        gcmASSERT(i < operandCount);

        switch (paramName->dataType->qualifier)
        {
        case slvQUALIFIER_CONST_IN:
        case slvQUALIFIER_IN:
        case slvQUALIFIER_INOUT:
            gcmASSERT(operandsParameters[i].needROperand);

            for (j = 0; j < operandsParameters[i].operandCount; j++)
            {
                slsLOPERAND_Initialize(
                                    &lOperand,
                                    paramName->context.logicalRegs + j);

                if (gcIsSamplerDataType(lOperand.dataType))
                {
                    status = slGenAssignSamplerCode(Compiler,
                                                    PolynaryExpr->exprBase.base.lineNo,
                                                    PolynaryExpr->exprBase.base.stringNo,
                                                    &lOperand,
                                                    operandsParameters[i].rOperands + j);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
                else
                {
                    status = slGenAssignCode(Compiler,
                                             PolynaryExpr->exprBase.base.lineNo,
                                             PolynaryExpr->exprBase.base.stringNo,
                                             &lOperand,
                                             operandsParameters[i].rOperands + j);

                    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
                }
            }

            break;
        }

        i++;
    }

    /* Generate the call code */
    status = slGetFunctionLabel(Compiler,
                                PolynaryExpr->funcName->context.function,
                                &funcLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slEmitAlwaysBranchCode(Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_CALL,
                                    funcLabel);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Get all 'out' arguments */
    i = 0;
    FOR_EACH_DLINK_NODE(&PolynaryExpr->funcName->u.funcInfo.localSpace->names, slsNAME, paramName)
    {
        if (paramName->type != slvPARAMETER_NAME) break;

        gcmASSERT(i < operandCount);

        switch (paramName->dataType->qualifier)
        {
        case slvQUALIFIER_OUT:
        case slvQUALIFIER_INOUT:
            gcmASSERT(operandsParameters[i].needLOperand);

            for (j = 0; j < operandsParameters[i].operandCount; j++)
            {
                slsROPERAND_InitializeReg(&rOperand,
                                          paramName->context.logicalRegs + j);

                status = slGenAssignCode(Compiler,
                                         PolynaryExpr->exprBase.base.lineNo,
                                         PolynaryExpr->exprBase.base.stringNo,
                                         operandsParameters[i].lOperands + j,
                                         &rOperand);

                if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
            }

            break;
        }

        i++;
    }

    /* Get the return value */
    if (Parameters->needROperand)
    {
        status = slsGEN_CODE_PARAMETERS_AllocateOperands(Compiler,
                                                         Parameters,
                                                         PolynaryExpr->exprBase.dataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        for (i = 0; i < Parameters->operandCount; i++)
        {
            /* Returned value needs to be stored to a temp register, optimizer will optimize this MOV if */
            /* it is redundant truely */
            slsIOPERAND_New(Compiler, &intermIOperand, (PolynaryExpr->funcName->context.logicalRegs + i)->dataType);
            slsLOPERAND_InitializeUsingIOperand(&intermLOperand, &intermIOperand);
            slsROPERAND_InitializeReg(&intermROperand,
                                      PolynaryExpr->funcName->context.logicalRegs + i);

            status = slGenAssignCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    &intermLOperand,
                                    &intermROperand);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);
            *(Parameters->rOperands + i) = intermROperand;
        }
    }

    gcmVERIFY_OK(sloIR_POLYNARY_EXPR_FinalizeOperandsParameters(Compiler,
                                                                operandCount,
                                                                operandsParameters));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_TryToEvaluate(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    sloIR_POLYNARY_EXPR     newPolynaryExpr;
    sloIR_EXPR              operand;
    slsGEN_CODE_PARAMETERS  operandParameters;
    sloIR_CONSTANT          operandConstant;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);

    if (PolynaryExpr->type == slvPOLYNARY_FUNC_CALL && !PolynaryExpr->funcName->isBuiltIn)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    /* Create a new polynary expression */
    status = sloIR_POLYNARY_EXPR_Construct(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        PolynaryExpr->type,
                                        PolynaryExpr->exprBase.dataType,
                                        PolynaryExpr->funcSymbol,
                                        &newPolynaryExpr);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmASSERT(PolynaryExpr->operands);

    status = sloIR_SET_Construct(
                                Compiler,
                                PolynaryExpr->operands->base.lineNo,
                                PolynaryExpr->operands->base.stringNo,
                                PolynaryExpr->operands->type,
                                &newPolynaryExpr->operands);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Try to evaluate all operands */
    FOR_EACH_DLINK_NODE(&PolynaryExpr->operands->members, struct _sloIR_EXPR, operand)
    {
        /* Try to evaluate the operand */
        slsGEN_CODE_PARAMETERS_Initialize(
                                        &operandParameters,
                                        gcvFALSE,
                                        gcvTRUE);

        operandParameters.hint = slvEVALUATE_ONLY;

        status = sloIR_OBJECT_Accept(
                                    Compiler,
                                    &operand->base,
                                    &CodeGenerator->visitor,
                                    &operandParameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        operandConstant             = operandParameters.constant;
        operandParameters.constant  = gcvNULL;

        slsGEN_CODE_PARAMETERS_Finalize(&operandParameters);

        if (operandConstant == gcvNULL) goto Exit;

        /* Add to the new polynary expression */
        gcmVERIFY_OK(sloIR_SET_AddMember(
                                        Compiler,
                                        newPolynaryExpr->operands,
                                        &operandConstant->exprBase.base));
    }

    if (newPolynaryExpr->type == slvPOLYNARY_FUNC_CALL)
    {
        status = sloCOMPILER_BindFuncCall(Compiler, newPolynaryExpr);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    /* Try to evaluate the new polynary expression */
    status = sloIR_POLYNARY_EXPR_Evaluate(
                                        Compiler,
                                        newPolynaryExpr,
                                        &Parameters->constant);

    if (gcmIS_SUCCESS(status) && Parameters->constant != gcvNULL)
    {
        newPolynaryExpr = gcvNULL;
    }

Exit:
    if (newPolynaryExpr != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &newPolynaryExpr->exprBase.base));
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_GenCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS       status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);

    /* Try to evaluate the polynary expression */
    if (!Parameters->needLOperand && Parameters->needROperand)
    {
        status = sloIR_POLYNARY_EXPR_TryToEvaluate(
                                                Compiler,
                                                CodeGenerator,
                                                PolynaryExpr,
                                                Parameters);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (Parameters->hint == slvEVALUATE_ONLY)
        {
            gcmFOOTER_KILL();
            return gcvSTATUS_OK;
        }

        if (Parameters->constant != gcvNULL)
        {
            status = sloIR_CONSTANT_GenCode(
                                        Compiler,
                                        CodeGenerator,
                                        Parameters->constant,
                                        Parameters);
            gcmFOOTER();
            return status;
        }
    }

    switch (PolynaryExpr->type)
    {
    case slvPOLYNARY_CONSTRUCT_FLOAT:
    case slvPOLYNARY_CONSTRUCT_INT:
    case slvPOLYNARY_CONSTRUCT_BOOL:
        status = sloIR_POLYNARY_EXPR_GenConstructScalarCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        PolynaryExpr,
                                                        Parameters);

        gcmFOOTER();
        return status;

    case slvPOLYNARY_CONSTRUCT_VEC2:
    case slvPOLYNARY_CONSTRUCT_VEC3:
    case slvPOLYNARY_CONSTRUCT_VEC4:
    case slvPOLYNARY_CONSTRUCT_BVEC2:
    case slvPOLYNARY_CONSTRUCT_BVEC3:
    case slvPOLYNARY_CONSTRUCT_BVEC4:
    case slvPOLYNARY_CONSTRUCT_IVEC2:
    case slvPOLYNARY_CONSTRUCT_IVEC3:
    case slvPOLYNARY_CONSTRUCT_IVEC4:
        status = sloIR_POLYNARY_EXPR_GenConstructVectorCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        PolynaryExpr,
                                                        Parameters);

        gcmFOOTER();
        return status;

    case slvPOLYNARY_CONSTRUCT_MAT2:
    case slvPOLYNARY_CONSTRUCT_MAT3:
    case slvPOLYNARY_CONSTRUCT_MAT4:
        status = sloIR_POLYNARY_EXPR_GenConstructMatrixCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        PolynaryExpr,
                                                        Parameters);

        gcmFOOTER();
        return status;

    case slvPOLYNARY_CONSTRUCT_STRUCT:
        status = sloIR_POLYNARY_EXPR_GenConstructStructCode(
                                                        Compiler,
                                                        CodeGenerator,
                                                        PolynaryExpr,
                                                        Parameters);

        gcmFOOTER();
        return status;

    case slvPOLYNARY_FUNC_CALL:
        gcmASSERT(PolynaryExpr->funcName);

        if (PolynaryExpr->funcName->isBuiltIn)
        {
            status = sloIR_POLYNARY_EXPR_GenBuiltInCode(
                                                    Compiler,
                                                    CodeGenerator,
                                                    PolynaryExpr,
                                                    Parameters);
        }
        else
        {
            status = sloIR_POLYNARY_EXPR_GenFuncCallCode(
                                                    Compiler,
                                                    CodeGenerator,
                                                    PolynaryExpr,
                                                    Parameters);
        }
        gcmFOOTER();
        return status;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
}

gceSTATUS
sloIR_AllocObjectPointerArrays(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gcSHADER  shader;
    gceSTATUS status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(CodeGenerator);
    gcmVERIFY_OK(sloCOMPILER_GetBinary(Compiler, &shader));

    if(CodeGenerator->attributeCount) {
      status = gcSHADER_ReallocateAttributes(shader, (gctSIZE_T)CodeGenerator->attributeCount);
          if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    if(CodeGenerator->uniformCount) {
      status = gcSHADER_ReallocateUniforms(shader, (gctSIZE_T)CodeGenerator->uniformCount);
          if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    if(CodeGenerator->variableCount) {
      status = gcSHADER_ReallocateVariables(shader, (gctSIZE_T)CodeGenerator->variableCount);
          if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    if(CodeGenerator->outputCount) {
      status = gcSHADER_ReallocateOutputs(shader, (gctSIZE_T)CodeGenerator->outputCount);
          if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    if(CodeGenerator->functionCount) {
      status = gcSHADER_ReallocateFunctions(shader, (gctSIZE_T)CodeGenerator->functionCount);
          if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCODE_GENERATOR_Construct(
    IN sloCOMPILER Compiler,
    OUT sloCODE_GENERATOR * CodeGenerator
    )
{
    gceSTATUS           status;
    sloCODE_GENERATOR   codeGenerator;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(CodeGenerator);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloCODE_GENERATOR),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        codeGenerator = pointer;

        /* Initialize the visitor */
        codeGenerator->visitor.object.type          = slvOBJ_CODE_GENERATOR;

        codeGenerator->visitor.visitSet             =
                        (sltVISIT_SET_FUNC_PTR)sloIR_SET_GenCode;

        codeGenerator->visitor.visitIteration       =
                        (sltVISIT_ITERATION_FUNC_PTR)sloIR_ITERATION_GenCode;

        codeGenerator->visitor.visitJump            =
                        (sltVISIT_JUMP_FUNC_PTR)sloIR_JUMP_GenCode;

        codeGenerator->visitor.visitVariable        =
                        (sltVISIT_VARIABLE_FUNC_PTR)sloIR_VARIABLE_GenCode;

        codeGenerator->visitor.visitConstant        =
                        (sltVISIT_CONSTANT_FUNC_PTR)sloIR_CONSTANT_GenCode;

        codeGenerator->visitor.visitUnaryExpr       =
                        (sltVISIT_UNARY_EXPR_FUNC_PTR)sloIR_UNARY_EXPR_GenCode;

        codeGenerator->visitor.visitBinaryExpr      =
                        (sltVISIT_BINARY_EXPR_FUNC_PTR)sloIR_BINARY_EXPR_GenCode;

        codeGenerator->visitor.visitSelection       =
                        (sltVISIT_SELECTION_FUNC_PTR)sloIR_SELECTION_GenCode;

        codeGenerator->visitor.visitPolynaryExpr    =
                        (sltVISIT_POLYNARY_EXPR_FUNC_PTR)sloIR_POLYNARY_EXPR_GenCode;

        /* Initialize other data members */
        codeGenerator->currentIterationContext      = gcvNULL;

        *CodeGenerator = codeGenerator;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *CodeGenerator = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCODE_GENERATOR_Destroy(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator
    )
{
    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(CodeGenerator, slvOBJ_CODE_GENERATOR);;

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, CodeGenerator));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slVSOuputPatch(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE* globalNameSpace
    )
{
    slsNAME* globalName;

    gcmHEADER();

    FOR_EACH_DLINK_NODE(&globalNameSpace->names, slsNAME, globalName)
    {
        if (globalName->dataType->qualifier == slvQUALIFIER_VARYING_OUT)
        {
            /* Yes, we find a declared output. We firstly need check whether
            ** it has been in output table. */
            gctSIZE_T nameLength = 0;
            gcSHADER shader;
            gcOUTPUT output;

            sloCOMPILER_GetBinary(Compiler, &shader);
            gcmVERIFY_OK(gcoOS_StrLen(globalName->symbol, &nameLength));
            gcSHADER_GetOutputByName(shader, globalName->symbol, nameLength, &output);

            /* We do not find this output in output table, so add it to
            ** table. */
            if (output == gcvNULL)
            {
                gcSHADER_TYPE   binaryDataType;
                gctSIZE_T       binaryDataTypeSize;
                gctUINT         logicalRegCount;
                gctREG_INDEX    tempRegIndex;

                binaryDataType      = _ConvElementDataType(globalName->dataType);
                binaryDataTypeSize  = gcGetDataTypeSize(binaryDataType);
                logicalRegCount = (globalName->dataType->arrayLength > 0) ?
                                                      globalName->dataType->arrayLength : 1;
                tempRegIndex = slNewTempRegs(Compiler, logicalRegCount * binaryDataTypeSize);

                slNewOutput(Compiler,
                            globalName->lineNo,
                            globalName->stringNo,
                            globalName->symbol,
                            binaryDataType,
                            logicalRegCount,
                            tempRegIndex);
            }
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
