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


#include "gc_glsl_scanner.h"
#include "gc_glsl_ir.h"
#include "gc_glsl_built_ins.h"
#include "gc_glsl_gen_code.h"

/* Data type */
gceSTATUS
slsDATA_TYPE_Construct(
    IN sloCOMPILER Compiler,
    IN gctINT TokenType,
    IN slsNAME_SPACE * FieldSpace,
    OUT slsDATA_TYPE ** DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x TokenType=%d FieldSpace=0x%x",
                  Compiler, TokenType, FieldSpace);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(DataType);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsDATA_TYPE),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        dataType = pointer;

        dataType->qualifier     = slvQUALIFIER_NONE;

        /*dataType->precision     = slvPRECISION_DEFAULT;*/

        dataType->arrayLength   = 0;

        dataType->fieldSpace    = gcvNULL;

        switch (TokenType)
        {
        case T_VOID:
            dataType->elementType   = slvTYPE_VOID;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

        case T_FLOAT:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

        case T_INT:
            dataType->elementType   = slvTYPE_INT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

        case T_UINT:
            dataType->elementType   = slvTYPE_UINT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

        case T_BOOL:
            dataType->elementType   = slvTYPE_BOOL;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

        case T_VEC2:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 2);
            break;

        case T_VEC3:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 3);
            break;

        case T_VEC4:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 4);
            break;

        case T_BVEC2:
            dataType->elementType   = slvTYPE_BOOL;
	    slmDATA_TYPE_vectorSize_SET(dataType, 2);
            break;

        case T_BVEC3:
            dataType->elementType   = slvTYPE_BOOL;
	    slmDATA_TYPE_vectorSize_SET(dataType, 3);
            break;

        case T_BVEC4:
            dataType->elementType   = slvTYPE_BOOL;
	    slmDATA_TYPE_vectorSize_SET(dataType, 4);
            break;

        case T_IVEC2:
            dataType->elementType   = slvTYPE_INT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 2);
            break;

        case T_IVEC3:
            dataType->elementType   = slvTYPE_INT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 3);
            break;

        case T_IVEC4:
            dataType->elementType   = slvTYPE_INT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 4);
            break;

        case T_UVEC2:
            dataType->elementType   = slvTYPE_UINT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 2);
            break;

        case T_UVEC3:
            dataType->elementType   = slvTYPE_UINT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 3);
            break;

        case T_UVEC4:
            dataType->elementType   = slvTYPE_UINT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 4);
            break;

        case T_MAT2:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 2, 2);
            break;

        case T_MAT2X3:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 3, 2);
            break;

        case T_MAT2X4:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 4, 2);
            break;

        case T_MAT3:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 3, 3);
            break;

        case T_MAT3X2:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 2, 3);
            break;

        case T_MAT3X4:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 4, 3);
            break;

        case T_MAT4:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 4, 4);
            break;

        case T_MAT4X2:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 2, 4);
            break;

        case T_MAT4X3:
            dataType->elementType   = slvTYPE_FLOAT;
	    slmDATA_TYPE_matrixSize_SET(dataType, 3, 4);
            break;

        case T_SAMPLER2D:
            dataType->elementType   = slvTYPE_SAMPLER2D;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

        case T_SAMPLERCUBE:
            dataType->elementType   = slvTYPE_SAMPLERCUBE;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

	case T_SAMPLER3D:
            dataType->elementType   = slvTYPE_SAMPLER3D;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

	case T_SAMPLER1DARRAY:
            dataType->elementType   = slvTYPE_SAMPLER1DARRAY;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

	case T_SAMPLER2DARRAY:
            dataType->elementType   = slvTYPE_SAMPLER2DARRAY;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

	case T_SAMPLER1DARRAYSHADOW:
            dataType->elementType   = slvTYPE_SAMPLER1DARRAYSHADOW;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

	case T_SAMPLER2DARRAYSHADOW:
            dataType->elementType   = slvTYPE_SAMPLER2DARRAYSHADOW;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

        case T_STRUCT:
            gcmASSERT(FieldSpace);

            dataType->elementType   = slvTYPE_STRUCT;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            dataType->fieldSpace    = FieldSpace;
            break;

        case T_ISAMPLER2D:
            dataType->elementType   = slvTYPE_ISAMPLER2D;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

        case T_ISAMPLERCUBE:
            dataType->elementType   = slvTYPE_ISAMPLERCUBE;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

	case T_ISAMPLER3D:
            dataType->elementType   = slvTYPE_ISAMPLER3D;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

	case T_ISAMPLER2DARRAY:
            dataType->elementType   = slvTYPE_ISAMPLER2DARRAY;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

        case T_USAMPLER2D:
            dataType->elementType   = slvTYPE_USAMPLER2D;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

        case T_USAMPLERCUBE:
            dataType->elementType   = slvTYPE_USAMPLERCUBE;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
            break;

	case T_USAMPLER3D:
            dataType->elementType   = slvTYPE_USAMPLER3D;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

	case T_USAMPLER2DARRAY:
            dataType->elementType   = slvTYPE_USAMPLER2DARRAY;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

	case T_SAMPLEREXTERNALOES:
            dataType->elementType   = slvTYPE_SAMPLEREXTERNALOES;
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
	    break;

        default: gcmASSERT(0);
        }

        sloCOMPILER_GetDefaultPrecision(Compiler, dataType->elementType, &dataType->precision);

        *DataType = dataType;

        gcmFOOTER_ARG("*DataType=0x%x", *DataType);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *DataType = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
slsDATA_TYPE_ConstructArray(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * ElementDataType,
    IN gctUINT ArrayLength,
    OUT slsDATA_TYPE ** DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x ElementDataType=0x%x ArrayLength=%u",
                  Compiler, ElementDataType, ArrayLength);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(ElementDataType);
    gcmVERIFY_ARGUMENT(DataType);

    gcmASSERT(ElementDataType->arrayLength == 0);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsDATA_TYPE),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        dataType = pointer;

        *dataType               = *ElementDataType;
        dataType->arrayLength   = ArrayLength;

        *DataType = dataType;

        gcmFOOTER_ARG("*DataType=0x%x", *DataType);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *DataType = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
slsDATA_TYPE_ConstructElement(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * CompoundDataType,
    OUT slsDATA_TYPE ** DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x CompoundDataType=0x%x",
                  Compiler, CompoundDataType);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(CompoundDataType);
    gcmVERIFY_ARGUMENT(DataType);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsDATA_TYPE),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        dataType = pointer;

        *dataType               = *CompoundDataType;

        if (slsDATA_TYPE_IsBVecOrIVecOrVec(dataType))
        {
	    slmDATA_TYPE_vectorSize_SET(dataType, 0);
        }
        else if (slsDATA_TYPE_IsMat(dataType))
        {
	    slmDATA_TYPE_matrixColumnCount_SET(dataType, 0);
        }
        else if (slsDATA_TYPE_IsArray(dataType))
        {
            dataType->arrayLength   = 0;
        }
        else
        {
            gcmASSERT(0);
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        *DataType = dataType;

        gcmFOOTER_ARG("*DataType=0x%x", *DataType);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *DataType = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
slsDATA_TYPE_Clone(
    IN sloCOMPILER Compiler,
    IN sltQUALIFIER Qualifier,
    IN slsDATA_TYPE * Source,
    OUT slsDATA_TYPE ** DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x Qualifier=%d Source=0x%x",
                  Compiler, Qualifier, Source);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(DataType);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsDATA_TYPE),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        dataType = pointer;

        *dataType               = *Source;
        dataType->qualifier     = Qualifier;

        *DataType = dataType;

        gcmFOOTER_ARG("*DataType=0x%x", *DataType);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *DataType = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
slsDATA_TYPE_Destory(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType
    )
{
    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x", Compiler, DataType);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(DataType);

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, DataType));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctCONST_STRING
_GetElementTypeName(
    IN sltELEMENT_TYPE ElementType
    )
{
    switch (ElementType)
    {
    case slvTYPE_VOID:          return "void";

    case slvTYPE_BOOL:          return "bool";
    case slvTYPE_INT:           return "int";
    case slvTYPE_UINT:          return "unsigned int";
    case slvTYPE_FLOAT:         return "float";

    case slvTYPE_SAMPLER2D:     return "sampler2D";
    case slvTYPE_SAMPLERCUBE:   return "samplerCube";

    case slvTYPE_SAMPLER3D:	return "sampler3D";
    case slvTYPE_SAMPLER1DARRAY: return "sampler1DArray";
    case slvTYPE_SAMPLER2DARRAY: return "sampler2DArray";
    case slvTYPE_SAMPLER1DARRAYSHADOW: return "sampler1DArrayShadow";
    case slvTYPE_SAMPLER2DARRAYSHADOW: return "sampler2DArrayShadow";

    case slvTYPE_ISAMPLER2D:     return "isampler2D";
    case slvTYPE_ISAMPLERCUBE:   return "isamplerCube";
    case slvTYPE_ISAMPLER3D:	 return "isampler3D";
    case slvTYPE_ISAMPLER2DARRAY: return "isampler2DArray";

    case slvTYPE_USAMPLER2D:     return "usampler2D";
    case slvTYPE_USAMPLERCUBE:   return "usamplerCube";
    case slvTYPE_USAMPLER3D:	 return "usampler3D";
    case slvTYPE_USAMPLER2DARRAY: return "usampler2DArray";

    case slvTYPE_SAMPLEREXTERNALOES: return "samplerExternalOES";

    case slvTYPE_STRUCT:        return "struct";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gctCONST_STRING
slGetQualifierName(
    IN sltQUALIFIER Qualifier
    )
{
    switch (Qualifier)
    {
    case slvQUALIFIER_NONE:                     return "none";

    case slvQUALIFIER_CONST:                    return "const";

    case slvQUALIFIER_UNIFORM:                  return "uniform";
    case slvQUALIFIER_ATTRIBUTE:                return "attribute";

    case slvQUALIFIER_VARYING_OUT:              return "varying";
    case slvQUALIFIER_VARYING_IN:               return "varying";
    case slvQUALIFIER_INVARIANT_VARYING_OUT:    return "invariant_varying";
    case slvQUALIFIER_INVARIANT_VARYING_IN:     return "invariant_varying";
    case slvQUALIFIER_FRAGMENT_OUT:             return "fragment_out";

    case slvQUALIFIER_CONST_IN:                 return "const_in";
    case slvQUALIFIER_IN:                       return "in";
    case slvQUALIFIER_OUT:                      return "out";
    case slvQUALIFIER_INOUT:                    return "inout";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

static gctCONST_STRING
_GetPrecisionName(
    IN sltPRECISION Precision
    )
{
    switch (Precision)
    {
    case slvPRECISION_DEFAULT:  return "default";

    case slvPRECISION_HIGH:     return "high";
    case slvPRECISION_MEDIUM:   return "medium";
    case slvPRECISION_LOW:      return "low";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
slsDATA_TYPE_Dump(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType
    )
{
    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x", Compiler, DataType);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(DataType);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<DATA_TYPE this=\"0x%x\" qualifier=\"%s\" precision=\"%s\""
                                " elementType=\"%s\" vectorSize=\"%d\""
                                " matrixSize=\"%d\" arrayLength=\"%d\" fieldSpace=\"0x%x\" />",
                                DataType,
                                slGetQualifierName(DataType->qualifier),
                                _GetPrecisionName(DataType->precision),
                                _GetElementTypeName(DataType->elementType),
                                slmDATA_TYPE_vectorSize_GET(DataType),
                                slmDATA_TYPE_matrixSize_GET(DataType),
                                DataType->arrayLength,
                                DataType->fieldSpace));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gctBOOL
slsDATA_TYPE_IsEqual(
    IN slsDATA_TYPE * DataType1,
    IN slsDATA_TYPE * DataType2
    )
{
    gctBOOL result;

    gcmHEADER_ARG("DataType1=0x%x DataType2=0x%x", DataType1, DataType2);

    gcmASSERT(DataType1);
    gcmASSERT(DataType2);

    result = (DataType1->elementType == DataType2->elementType)
              && (slmDATA_TYPE_vectorSize_NOCHECK_GET(DataType1) ==
                  slmDATA_TYPE_vectorSize_NOCHECK_GET(DataType2))
              && (slmDATA_TYPE_matrixRowCount_GET(DataType1) ==
                  slmDATA_TYPE_matrixRowCount_GET(DataType2))
              && (slmDATA_TYPE_matrixColumnCount_GET(DataType1) ==
                  slmDATA_TYPE_matrixColumnCount_GET(DataType2))
              && (DataType1->arrayLength == DataType2->arrayLength)
              && (DataType1->fieldSpace == DataType2->fieldSpace);

    gcmFOOTER_ARG("<return>=%d", result);
    return result;
}

gctSIZE_T
slsDATA_TYPE_GetSize(
    IN slsDATA_TYPE * DataType
    )
{
    gctSIZE_T   size = 0;
    slsNAME *   fieldName;

    gcmHEADER_ARG("DataType=0x%x", DataType);

    gcmASSERT(DataType);

    switch (DataType->elementType)
    {
    case slvTYPE_VOID:
        size = 0;
        break;

    case slvTYPE_BOOL:
    case slvTYPE_INT:
    case slvTYPE_UINT:
    case slvTYPE_FLOAT:
        size = 1;
        break;

    case slvTYPE_SAMPLER2D:
    case slvTYPE_SAMPLERCUBE:
    case slvTYPE_SAMPLER3D:
    case slvTYPE_SAMPLER1DARRAY:
    case slvTYPE_SAMPLER2DARRAY:
    case slvTYPE_SAMPLER1DARRAYSHADOW:
    case slvTYPE_SAMPLER2DARRAYSHADOW:
    case slvTYPE_ISAMPLER2D:
    case slvTYPE_ISAMPLERCUBE:
    case slvTYPE_ISAMPLER3D:
    case slvTYPE_ISAMPLER2DARRAY:
    case slvTYPE_USAMPLER2D:
    case slvTYPE_USAMPLERCUBE:
    case slvTYPE_USAMPLER3D:
    case slvTYPE_USAMPLER2DARRAY:
    case slvTYPE_SAMPLEREXTERNALOES:
        size = 4;
        break;

    case slvTYPE_STRUCT:
        gcmASSERT(DataType->fieldSpace);

        FOR_EACH_DLINK_NODE(&DataType->fieldSpace->names, slsNAME, fieldName)
        {
            gcmASSERT(fieldName->dataType);
            size += slsDATA_TYPE_GetSize(fieldName->dataType);
        }
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("<return>=%u", 0);
        return 0;
    }

    if (slmDATA_TYPE_vectorSize_GET(DataType) > 0)
    {
        size *= slmDATA_TYPE_vectorSize_NOCHECK_GET(DataType);
    }
    else if (slmDATA_TYPE_matrixSize_GET(DataType) > 0)
    {
        size *= slmDATA_TYPE_matrixColumnCount_GET(DataType) *
                slmDATA_TYPE_matrixRowCount_GET(DataType);
    }

    if (DataType->arrayLength > 0)
    {
        size *= DataType->arrayLength;
    }

    gcmFOOTER_ARG("<return>=%u", size);
    return size;
}

gctSIZE_T
slsDATA_TYPE_GetFieldOffset(
    IN slsDATA_TYPE * StructDataType,
    IN slsNAME * FieldName
    )
{
    gctSIZE_T   offset = 0;
    slsNAME *   fieldName;

    gcmHEADER_ARG("StructDataType=0x%x FieldName=0x%x", StructDataType, FieldName);

    gcmASSERT(StructDataType);
    gcmASSERT(slsDATA_TYPE_IsStruct(StructDataType));
    gcmASSERT(FieldName);

    gcmASSERT(StructDataType->fieldSpace);

    FOR_EACH_DLINK_NODE(&StructDataType->fieldSpace->names, slsNAME, fieldName)
    {
        if (fieldName == FieldName) break;

        gcmASSERT(fieldName->dataType);
        offset += slsDATA_TYPE_GetSize(fieldName->dataType);
    }

    gcmASSERT(fieldName == FieldName);

    gcmFOOTER_ARG("<return>=%u", offset);
    return offset;
}

gctBOOL
slsDATA_TYPE_IsAssignableAndComparable(
    IN slsDATA_TYPE * DataType
    )
{
    slsNAME *   fieldName;

    gcmHEADER_ARG("DataType=0x%x", DataType);

    gcmASSERT(DataType);

    if (slsDATA_TYPE_IsArray(DataType))
    {
        gcmFOOTER_ARG("<return>=%d", gcvFALSE);
        return gcvFALSE;
    }

    switch (DataType->elementType)
    {
    case slvTYPE_VOID:

    case slvTYPE_SAMPLER2D:
    case slvTYPE_SAMPLERCUBE:
    case slvTYPE_SAMPLER3D:
    case slvTYPE_SAMPLER1DARRAY:
    case slvTYPE_SAMPLER2DARRAY:
    case slvTYPE_SAMPLER1DARRAYSHADOW:
    case slvTYPE_SAMPLER2DARRAYSHADOW:
    case slvTYPE_ISAMPLER2D:
    case slvTYPE_ISAMPLERCUBE:
    case slvTYPE_ISAMPLER3D:
    case slvTYPE_ISAMPLER2DARRAY:
    case slvTYPE_USAMPLER2D:
    case slvTYPE_USAMPLERCUBE:
    case slvTYPE_USAMPLER3D:
    case slvTYPE_USAMPLER2DARRAY:
    case slvTYPE_SAMPLEREXTERNALOES:
        gcmFOOTER_ARG("<return>=%d", gcvFALSE);
        return gcvFALSE;

    case slvTYPE_BOOL:
    case slvTYPE_INT:
    case slvTYPE_UINT:
    case slvTYPE_FLOAT:
        gcmFOOTER_ARG("<return>=%d", gcvTRUE);
        return gcvTRUE;

    case slvTYPE_STRUCT:
        gcmASSERT(DataType->fieldSpace);

        FOR_EACH_DLINK_NODE(&DataType->fieldSpace->names, slsNAME, fieldName)
        {
            gcmASSERT(fieldName->dataType);
            if (!slsDATA_TYPE_IsAssignableAndComparable(fieldName->dataType))
            {
                gcmFOOTER_ARG("<return>=%d", gcvFALSE);
                return gcvFALSE;
            }
        }

         gcmFOOTER_ARG("<return>=%d", gcvTRUE);
        return gcvTRUE;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("<return>=%d", gcvFALSE);
        return gcvFALSE;
    }
}

/* Name and Name space. */
gceSTATUS
slsNAME_Construct(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE * MySpace,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleNAME_TYPE Type,
    IN slsDATA_TYPE * DataType,
    IN sltPOOL_STRING Symbol,
    IN gctBOOL IsBuiltIn,
    IN sleEXTENSION Extension,
    OUT slsNAME ** Name
    )
{
    gceSTATUS   status;
    slsNAME *   name;

    gcmHEADER_ARG("Compiler=0x%x MySpace=0x%x LineNo=%u StringNo=%u Type=%d "
                  "DataType=0x%x Symbol=0x%x IsBuiltIn=%d Extension=%d",
                  Compiler, MySpace, LineNo, StringNo, Type, DataType, Symbol,
                  IsBuiltIn, Extension);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(MySpace);
    gcmVERIFY_ARGUMENT(Symbol);
    gcmVERIFY_ARGUMENT(Name);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsNAME),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        name = pointer;

        name->mySpace       = MySpace;
        name->lineNo        = LineNo;
        name->stringNo      = StringNo;
        name->type          = Type;
        name->dataType      = DataType;
        name->symbol        = Symbol;
        name->isBuiltIn     = IsBuiltIn;
        name->extension     = Extension;

        switch (Type)
        {
        case slvVARIABLE_NAME:
            name->u.variableInfo.constant   = gcvNULL;
            break;

        case slvPARAMETER_NAME:
            name->u.parameterInfo.aliasName = gcvNULL;
            break;

        case slvFUNC_NAME:
            name->u.funcInfo.localSpace     = gcvNULL;
            name->u.funcInfo.isFuncDef      = gcvFALSE;
            name->u.funcInfo.funcBody       = gcvNULL;
            break;

        default:
            break;
        }

        name->context.logicalRegCount   = 0;
        name->context.logicalRegs       = gcvNULL;
        name->context.isCounted         = 0;
        name->context.useAsTextureCoord = gcvFALSE;
        name->context.function          = gcvNULL;

        *Name = name;

        gcmFOOTER_ARG("*Name=0x%x", *Name);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *Name = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
slsNAME_Destory(
    IN sloCOMPILER Compiler,
    IN slsNAME * Name
    )
{
    gcmHEADER_ARG("Compiler=0x%x Name=0x%x", Compiler, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(Name);

    slsNAME_FreeLogicalRegs(Compiler, Name);

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, Name));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctCONST_STRING
_GetNameTypeName(
    IN sleNAME_TYPE NameType
    )
{
    switch (NameType)
    {
    case slvVARIABLE_NAME:  return "variable";
    case slvPARAMETER_NAME: return "parameter";
    case slvFUNC_NAME:      return "function";
    case slvSTRUCT_NAME:    return "struct";
    case slvFIELD_NAME:     return "field";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
sloNAME_BindFuncBody(
    IN sloCOMPILER Compiler,
    IN slsNAME * FuncName,
    IN sloIR_SET FuncBody
    )
{
    gcmHEADER_ARG("Compiler=0x%x FuncName=0x%x FuncBody=0x%x",
                   Compiler, FuncName, FuncBody);

    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(FuncName);
    gcmASSERT(FuncName->type == slvFUNC_NAME);
    slmVERIFY_IR_OBJECT(FuncBody, slvIR_SET);

    FuncName->u.funcInfo.funcBody   = FuncBody;
    FuncBody->funcName              = FuncName;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloNAME_GetParamCount(
    IN sloCOMPILER Compiler,
    IN slsNAME * FuncName,
    OUT gctUINT * ParamCount
    )
{
    gctUINT     count = 0;
    slsNAME *   paramName;

    gcmHEADER_ARG("Compiler=0x%x FuncName=0x%x ParamCount=0x%x",
                   Compiler, FuncName, ParamCount);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(FuncName);
    gcmASSERT(FuncName->type == slvFUNC_NAME);
    gcmASSERT(FuncName->u.funcInfo.localSpace);
    gcmASSERT(ParamCount);

    FOR_EACH_DLINK_NODE(&FuncName->u.funcInfo.localSpace->names, slsNAME, paramName)
    {
        if (paramName->type != slvPARAMETER_NAME) break;

        count++;
    }

    *ParamCount = count;

    gcmFOOTER_ARG("*ParamCount=%u", *ParamCount);
    return gcvSTATUS_OK;
}

static gctBOOL
_IsSameFuncName(
    IN sloCOMPILER Compiler,
    IN slsNAME * FuncName1,
    IN slsNAME * FuncName2,
    OUT gctBOOL * AreAllParamQualifiersEqual
    )
{
    slsNAME *   paramName1;
    slsNAME *   paramName2;

    gcmHEADER_ARG("Compiler=0x%x FuncName1=0x%x FuncName2=0x%x AreAllParamQualifiersEqual=0x%x",
                   Compiler, FuncName1, FuncName2, AreAllParamQualifiersEqual);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(FuncName1);
    gcmASSERT(FuncName1->type == slvFUNC_NAME);
    gcmASSERT(FuncName1->u.funcInfo.localSpace);
    gcmASSERT(FuncName2);
    gcmASSERT(FuncName2->type == slvFUNC_NAME);
    gcmASSERT(FuncName2->u.funcInfo.localSpace);

    if (FuncName1->symbol != FuncName2->symbol)
    {
        gcmFOOTER_ARG("<return>=%d", gcvFALSE);
        return gcvFALSE;
    }

    if (AreAllParamQualifiersEqual != gcvNULL) *AreAllParamQualifiersEqual = gcvTRUE;

    for (paramName1 = (slsNAME *)FuncName1->u.funcInfo.localSpace->names.next,
            paramName2 = (slsNAME *)FuncName2->u.funcInfo.localSpace->names.next;
        (slsDLINK_NODE *)paramName1 != &FuncName1->u.funcInfo.localSpace->names
            && (slsDLINK_NODE *)paramName2 != &FuncName2->u.funcInfo.localSpace->names;
        paramName1 = (slsNAME *)((slsDLINK_NODE *)paramName1)->next,
            paramName2 = (slsNAME *)((slsDLINK_NODE *)paramName2)->next)
    {
        if (paramName1->type != slvPARAMETER_NAME || paramName2->type != slvPARAMETER_NAME) break;

        if (!slsDATA_TYPE_IsEqual(paramName1->dataType, paramName2->dataType))
        {
            gcmFOOTER_ARG("<return>=%d", gcvFALSE);
            return gcvFALSE;
        }

        if (AreAllParamQualifiersEqual != gcvNULL
            && paramName1->dataType->qualifier != paramName2->dataType->qualifier)
        {
            *AreAllParamQualifiersEqual = gcvFALSE;
        }
    }

    if ((slsDLINK_NODE *)paramName1 != &FuncName1->u.funcInfo.localSpace->names
        && paramName1->type == slvPARAMETER_NAME)
    {
        gcmFOOTER_ARG("<return>=%d", gcvFALSE);
        return gcvFALSE;
    }

    if ((slsDLINK_NODE *)paramName2 != &FuncName2->u.funcInfo.localSpace->names
        && paramName2->type == slvPARAMETER_NAME)
    {
        gcmFOOTER_ARG("<return>=%d", gcvFALSE);
        return gcvFALSE;
    }

    gcmFOOTER_ARG("<return>=%d", gcvTRUE);
    return gcvTRUE;
}

gceSTATUS
slsNAME_BindAliasParamNames(
    IN sloCOMPILER Compiler,
    IN OUT slsNAME * FuncDefName,
    IN slsNAME * FuncDeclName
    )
{
    slsNAME *   paramName;
    slsNAME *   aliasParamName;

    gcmHEADER_ARG("Compiler=0x%x FuncDefName=0x%x FuncDeclName=0x%x",
                   Compiler, FuncDefName, FuncDeclName);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(FuncDefName);
    gcmASSERT(FuncDefName->type == slvFUNC_NAME);
    gcmASSERT(FuncDefName->u.funcInfo.localSpace);
    gcmASSERT(FuncDeclName);
    gcmASSERT(FuncDeclName->type == slvFUNC_NAME);
    gcmASSERT(FuncDeclName->u.funcInfo.localSpace);

    gcmASSERT(_IsSameFuncName(Compiler, FuncDefName, FuncDeclName, gcvNULL));

    for (paramName = (slsNAME *)FuncDefName->u.funcInfo.localSpace->names.next,
            aliasParamName = (slsNAME *)FuncDeclName->u.funcInfo.localSpace->names.next;
        (slsDLINK_NODE *)paramName != &FuncDefName->u.funcInfo.localSpace->names
            && (slsDLINK_NODE *)aliasParamName != &FuncDeclName->u.funcInfo.localSpace->names;
        paramName = (slsNAME *)((slsDLINK_NODE *)paramName)->next,
            aliasParamName = (slsNAME *)((slsDLINK_NODE *)aliasParamName)->next)
    {
        if (paramName->type != slvPARAMETER_NAME
            || aliasParamName->type != slvPARAMETER_NAME) break;

        paramName->u.parameterInfo.aliasName = aliasParamName;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slsNAME_Dump(
    IN sloCOMPILER Compiler,
    IN slsNAME * Name
    )
{
    gcmHEADER_ARG("Compiler=0x%x Name=0x%x",
                   Compiler, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Name);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<NAME this=\"0x%x\" mySpace=\"0x%x\" line=\"%d\" string=\"%d\""
                                " type=\"%s\" dataType=\"0x%x\" symbol=\"%s\" isBuiltIn=\"%s\"",
                                Name,
                                Name->mySpace,
                                Name->lineNo,
                                Name->stringNo,
                                _GetNameTypeName(Name->type),
                                Name->dataType,
                                Name->symbol,
                                (Name->isBuiltIn)? "true" : "false"));

    switch (Name->type)
    {
    case slvVARIABLE_NAME:
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    " constant=\"0x%x\" />",
                                    Name->u.variableInfo.constant));
        break;

    case slvPARAMETER_NAME:
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    " aliasName=\"0x%x\" />",
                                    Name->u.parameterInfo.aliasName));
        break;

    case slvFUNC_NAME:
        gcmASSERT(Name->u.funcInfo.localSpace);

        /* gcmVERIFY_OK(slsNAME_SPACE_Dump(Compiler, Name->localSpace)); */

        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    " localSpace=\"0x%x\" isFuncDef=\"%s\" funcBody=\"0x%x\" />",
                                    Name->u.funcInfo.localSpace,
                                    (Name->u.funcInfo.isFuncDef)? "true" : "false",
                                    Name->u.funcInfo.funcBody));
        break;

    default:
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    " />"));
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slsNAME_SPACE_Construct(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE * Parent,
    OUT slsNAME_SPACE ** NameSpace
    )
{
    gceSTATUS       status;
    slsNAME_SPACE * nameSpace;

    gcmHEADER_ARG("Compiler=0x%x Parent=0x%x NameSpace=0x%x",
                   Compiler, Parent, NameSpace);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(NameSpace);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(Compiler,
                                      (gctSIZE_T)sizeof(slsNAME_SPACE),
                                      &pointer);

        if (gcmIS_ERROR(status)) break;

        nameSpace = pointer;

        nameSpace->parent = Parent;

        slsDLINK_LIST_Initialize(&nameSpace->names);

        slsDLINK_LIST_Initialize(&nameSpace->subSpaces);

        if (Parent != gcvNULL)
        {
            slsDLINK_LIST_InsertLast(&Parent->subSpaces, &nameSpace->node);

            /* Propogate default precision value from parent */
            nameSpace->defaultPrecision.floatPrecision = Parent->defaultPrecision.floatPrecision;
            nameSpace->defaultPrecision.intPrecision = Parent->defaultPrecision.intPrecision;
        }
        else
        {
            /* Just set it as no explicit precision is set */
            nameSpace->defaultPrecision.floatPrecision = slvPRECISION_DEFAULT;
            nameSpace->defaultPrecision.intPrecision = slvPRECISION_DEFAULT;
        }

        *NameSpace = nameSpace;

        gcmFOOTER_ARG("*NameSpace=0x%x", *NameSpace);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *NameSpace = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
slsNAME_SPACE_Destory(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE * NameSpace
    )
{
    slsNAME *       name;
    slsNAME_SPACE * subSpace;

    gcmHEADER_ARG("Compiler=0x%x NameSpace=0x%x",
                   Compiler, NameSpace);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(NameSpace);

    /* Destroy all names */
    while (!slsDLINK_LIST_IsEmpty(&NameSpace->names))
    {
        slsDLINK_LIST_DetachFirst(&NameSpace->names, slsNAME, &name);

        gcmVERIFY_OK(slsNAME_Destory(Compiler, name));
    }

    /* Destroy all sub name spaces */
    while (!slsDLINK_LIST_IsEmpty(&NameSpace->subSpaces))
    {
        slsDLINK_LIST_DetachFirst(&NameSpace->subSpaces, slsNAME_SPACE, &subSpace);

        gcmVERIFY_OK(slsNAME_SPACE_Destory(Compiler, subSpace));
    }

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, NameSpace));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slsNAME_SPACE_Dump(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE * NameSpace
    )
{
    slsNAME *       name;
    slsNAME_SPACE * subSpace;

    gcmHEADER_ARG("Compiler=0x%x NameSpace=0x%x",
                   Compiler, NameSpace);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(NameSpace);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<NAME_SPACE this=\"0x%x\" parent=\"0x%x\">",
                                NameSpace,
                                NameSpace->parent));

    /* Dump all names */
    FOR_EACH_DLINK_NODE(&NameSpace->names, slsNAME, name)
    {
        gcmVERIFY_OK(slsNAME_Dump(Compiler, name));
    }

    /* Dump all sub name spaces */
    FOR_EACH_DLINK_NODE(&NameSpace->subSpaces, slsNAME_SPACE, subSpace)
    {
        gcmVERIFY_OK(slsNAME_SPACE_Dump(Compiler, subSpace));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</NAME_SPACE>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slsNAME_SPACE_Search(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE * NameSpace,
    IN sltPOOL_STRING Symbol,
    IN gctBOOL Recursive,
    OUT slsNAME ** Name
    )
{
    slsNAME *       name;
    gceSTATUS       status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x NameSpace=0x%x Symbol=0x%x Recursive=%d Name=0x%x",
                   Compiler, NameSpace, Symbol, Recursive, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(NameSpace);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "NameSpace=%x", NameSpace);
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "*Symbol=%s", gcmOPT_STRING(Symbol));
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "Name=%x", gcmOPT_POINTER(Name));

    FOR_EACH_DLINK_NODE(&NameSpace->names, slsNAME, name)
    {
        if (name->symbol == Symbol)
        {
            if (name->extension != slvEXTENSION_NONE)
            {
                if (!sloCOMPILER_ExtensionEnabled(
                                                Compiler,
                                                name->extension))
                {
                    continue;
                }
            }

            *Name = name;

            gcmFOOTER_ARG("*Name=0x%x", *Name);
            return gcvSTATUS_OK;
        }
    }

    if (Recursive && NameSpace->parent != gcvNULL)
    {
        status = slsNAME_SPACE_Search(Compiler,
                                      NameSpace->parent,
                                      Symbol,
                                      Recursive,
                                      Name);
        gcmFOOTER_ARG("*Name=0x%x status=%d", *Name, status);
        return status;
    }
    else
    {
        *Name = gcvNULL;

        status = gcvSTATUS_NAME_NOT_FOUND;
        gcmFOOTER();
        return status;
    }
}

gceSTATUS
slsNAME_SPACE_CheckNewFuncName(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE * NameSpace,
    IN slsNAME * FuncName,
    OUT slsNAME ** FirstFuncName
    )
{
    gctUINT         paramCount;
    slsNAME *       name;
    gctBOOL         areAllParamQualifiersEqual;
    gceSTATUS       status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x NameSpace=0x%x FuncName=0x%x FirstFuncName=0x%x",
                   Compiler, NameSpace, FuncName, FirstFuncName);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(NameSpace);
    gcmASSERT(FuncName);

    if (gcmIS_SUCCESS(gcoOS_StrCmp(FuncName->symbol, "main")))
    {
        if (!slsDATA_TYPE_IsVoid(FuncName->dataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            FuncName->lineNo,
                                            FuncName->stringNo,
                                            slvREPORT_ERROR,
                                            "The return type of the function 'main'"
                                            " must be 'void'"));

            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        gcmVERIFY_OK(sloNAME_GetParamCount(
                                            Compiler,
                                            FuncName,
                                            &paramCount));

        if (paramCount != 0)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            FuncName->lineNo,
                                            FuncName->stringNo,
                                            slvREPORT_ERROR,
                                            "The function 'main' must have no parameters"));

            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        if (FirstFuncName != gcvNULL) *FirstFuncName = FuncName;

        gcmFOOTER();
        return gcvSTATUS_OK;
    }

    if (FirstFuncName != gcvNULL) *FirstFuncName = gcvNULL;

    FOR_EACH_DLINK_NODE(&NameSpace->names, slsNAME, name)
    {
        if (name != FuncName)
        {
            if (name->type == slvFUNC_NAME
                && _IsSameFuncName(Compiler, name, FuncName, &areAllParamQualifiersEqual))
            {
                if (name->u.funcInfo.isFuncDef == FuncName->u.funcInfo.isFuncDef)
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    FuncName->lineNo,
                                                    FuncName->stringNo,
                                                    slvREPORT_ERROR,
                                                    "%s function: '%s'",
                                                    (FuncName->u.funcInfo.isFuncDef)?
                                                        "redefined" : "redeclared",
                                                    FuncName->symbol));

                    status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
                    return status;
                }
                else if (!slsDATA_TYPE_IsEqual(name->dataType, FuncName->dataType))
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    FuncName->lineNo,
                                                    FuncName->stringNo,
                                                    slvREPORT_ERROR,
                                                    "the inconsistent return type of"
                                                    " function: '%s'",
                                                    FuncName->symbol));

                    status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
                    return status;
                }

                if (!areAllParamQualifiersEqual)
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    FuncName->lineNo,
                                                    FuncName->stringNo,
                                                    slvREPORT_ERROR,
                                                    "the function: '%s' has different"
                                                    " parameter qualifier(s)",
                                                    FuncName->symbol));

                    status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
                    return status;
                }

                if (FirstFuncName != gcvNULL && *FirstFuncName == gcvNULL)
                {
                    *FirstFuncName = name;
                }
            }
        }
        else
        {
            if (FirstFuncName != gcvNULL && *FirstFuncName == gcvNULL)
            {
                *FirstFuncName = name;
            }
        }
    }

    if (FirstFuncName)
    {
        gcmFOOTER_ARG("*FirstFuncName=0x%x", *FirstFuncName);
    }
    else
    {
        gcmFOOTER_NO();
    }
    return gcvSTATUS_OK;
}

static gctBOOL
_IsCorrespondingFuncName(
    IN sloCOMPILER Compiler,
    IN slsNAME * FuncName,
    IN sloIR_POLYNARY_EXPR PolynaryExpr
    )
{
    gctUINT     paramCount;
    slsNAME *   paramName;
    sloIR_EXPR  argument;

    gcmHEADER_ARG("Compiler=0x%x FuncName=0x%x PolynaryExpr=0x%x",
                   Compiler, FuncName, PolynaryExpr);

    gcmASSERT(FuncName);
    gcmASSERT(FuncName->type == slvFUNC_NAME);
    gcmASSERT(FuncName->u.funcInfo.localSpace);

    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(PolynaryExpr->type == slvPOLYNARY_FUNC_CALL);
    gcmASSERT(PolynaryExpr->funcSymbol);

    if (FuncName->symbol != PolynaryExpr->funcSymbol)
    {
        gcmFOOTER_ARG("<return>=%d", gcvFALSE);
        return gcvFALSE;
    }

    if (PolynaryExpr->operands == gcvNULL)
    {
        gcmVERIFY_OK(sloNAME_GetParamCount(
                                            Compiler,
                                            FuncName,
                                            &paramCount));

        gcmFOOTER_ARG("<return>=%d", (paramCount == 0));
        return (paramCount == 0);
    }

    for (paramName = (slsNAME *)FuncName->u.funcInfo.localSpace->names.next,
            argument = (sloIR_EXPR)PolynaryExpr->operands->members.next;
        (slsDLINK_NODE *)paramName != &FuncName->u.funcInfo.localSpace->names
            && (slsDLINK_NODE *)argument != &PolynaryExpr->operands->members;
        paramName = (slsNAME *)((slsDLINK_NODE *)paramName)->next,
            argument = (sloIR_EXPR)((slsDLINK_NODE *)argument)->next)
    {
        if (paramName->type != slvPARAMETER_NAME) break;

        if (!slsDATA_TYPE_IsEqual(paramName->dataType, argument->dataType))
        {
            gcmFOOTER_ARG("<return>=%d", gcvFALSE);
            return gcvFALSE;
        }
    }

    if (((slsDLINK_NODE *)paramName != &FuncName->u.funcInfo.localSpace->names
            && paramName->type == slvPARAMETER_NAME)
        || (slsDLINK_NODE *)argument != &PolynaryExpr->operands->members)
    {
        gcmFOOTER_ARG("<return>=%d", gcvFALSE);
        return gcvFALSE;
    }

    gcmFOOTER_ARG("<return>=%d", gcvTRUE);
    return gcvTRUE;
}

gceSTATUS
slsNAME_SPACE_BindFuncName(
    IN sloCOMPILER Compiler,
    IN slsNAME_SPACE * NameSpace,
    IN OUT sloIR_POLYNARY_EXPR PolynaryExpr
    )
{
    gceSTATUS       status;
    slsNAME *       name;

    gcmHEADER_ARG("Compiler=0x%x NameSpace=0x%x PolynaryExpr=0x%x",
                   Compiler, NameSpace, PolynaryExpr);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(NameSpace);

    FOR_EACH_DLINK_NODE(&NameSpace->names, slsNAME, name)
    {
        if (name->type == slvFUNC_NAME
            && _IsCorrespondingFuncName(Compiler, name, PolynaryExpr))
        {
            if (name->extension != slvEXTENSION_NONE)
            {
                if (!sloCOMPILER_ExtensionEnabled(
                                                Compiler,
                                                name->extension))
                {
                    continue;
                }
            }

            status = sloCOMPILER_CloneDataType(
                                            Compiler,
                                            slvQUALIFIER_CONST,
                                            name->dataType,
                                            &PolynaryExpr->exprBase.dataType);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }

            PolynaryExpr->funcName  = name;

            gcmFOOTER_ARG("PolynaryExpr=0x%x", PolynaryExpr);
            return gcvSTATUS_OK;
        }
    }

    if (NameSpace->parent != gcvNULL)
    {
        status = slsNAME_SPACE_BindFuncName(Compiler,
                                            NameSpace->parent,
                                            PolynaryExpr);
        gcmFOOTER();
        return status;
    }

    gcmVERIFY_OK(sloCOMPILER_Report(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvREPORT_ERROR,
                                    "function: '%s' hasn't the corresponding declaration",
                                    PolynaryExpr->funcSymbol));

    status = gcvSTATUS_INVALID_ARGUMENT;
    gcmFOOTER();
    return status;
}

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
    )
{
    gceSTATUS status;
    slsNAME * name;

    gcmHEADER_ARG("Compiler=0x%x NameSpace=0x%x LineNo=%u StringNo=%u "
                  "Type=%d DataType=0x%x Symbol=0x%x IsBuiltIn=%d Extension=0x%x Name=0x%x",
                  Compiler, NameSpace, LineNo, StringNo,
                  Type, DataType, Symbol, IsBuiltIn, Extension, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(NameSpace);

    if (Type != slvFUNC_NAME && DataType != gcvNULL && slsDATA_TYPE_IsVoid(DataType))
    {
        if (Type != slvPARAMETER_NAME || Symbol[0] != '\0')
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvREPORT_ERROR,
                                            "\"%s\" can not use the void type",
                                            Symbol));
        }
        else
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvREPORT_ERROR,
                                            "the parameter declaration can not use the void type"));
        }

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    if (Type != slvFUNC_NAME && !(Type == slvPARAMETER_NAME && Symbol[0] == '\0'))
    {
        status = slsNAME_SPACE_Search(
                                    Compiler,
                                    NameSpace,
                                    Symbol,
                                    gcvFALSE,
                                    &name);

        if (status == gcvSTATUS_OK)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvREPORT_ERROR,
                                            "redefined identifier: '%s'",
                                            Symbol));

            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
    }

    /* Create a new name */
    do
    {
        status = slsNAME_Construct(
                                    Compiler,
                                    NameSpace,
                                    LineNo,
                                    StringNo,
                                    Type,
                                    DataType,
                                    Symbol,
                                    IsBuiltIn,
                                    Extension,
                                    &name);

        if (gcmIS_ERROR(status)) break;

        slsDLINK_LIST_InsertLast(&NameSpace->names, &name->node);

        if (Name != gcvNULL) *Name = name;

        gcmFOOTER_ARG("*Name=0x%x", gcmOPT_POINTER(Name));
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (Name != gcvNULL) *Name = gcvNULL;

    gcmFOOTER();
    return status;
}

/* sloIR_BASE object. */
static gctCONST_STRING
_GetIRObjectTypeName(
    IN sleIR_OBJECT_TYPE IRObjectType
    )
{
    switch (IRObjectType)
    {
    case slvIR_SET:             return "IR_SET";
    case slvIR_ITERATION:       return "IR_ITERATION";
    case slvIR_JUMP:            return "IR_JUMP";
    case slvIR_VARIABLE:        return "IR_VARIABLE";
    case slvIR_CONSTANT:        return "IR_CONSTANT";
    case slvIR_UNARY_EXPR:      return "IR_UNARY_EXPR";
    case slvIR_BINARY_EXPR:     return "IR_BINARY_EXPR";
    case slvIR_SELECTION:       return "IR_SELECTION";
    case slvIR_POLYNARY_EXPR:   return "IR_POLYNARY_EXPR";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
sloIR_BASE_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(This);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_BASE line=\"%d\" string=\"%d\" realType=\"%s\" />",
                                This->lineNo,
                                This->stringNo,
                                _GetIRObjectTypeName(This->vptr->type)));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* sloIR_SET object. */
gceSTATUS
sloIR_SET_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_SET       set = (sloIR_SET)This;
    slsDLINK_LIST * members;
    sloIR_BASE      member;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(set, slvIR_SET);

    members = &set->members;

    while (!slsDLINK_LIST_IsEmpty(members))
    {
        slsDLINK_LIST_DetachFirst(members, struct _sloIR_BASE, &member);

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, member));
    }

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, set));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* sloIR_BASE object. */
static gctCONST_STRING
_GetIRSetTypeName(
    IN sleSET_TYPE SetType
    )
{
    switch (SetType)
    {
    case slvDECL_SET:       return "declSet";
    case slvSTATEMENT_SET:  return "statementSet";
    case slvEXPR_SET:       return "exprSet";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
sloIR_SET_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_SET   set = (sloIR_SET)This;
    sloIR_BASE  member;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(set, slvIR_SET);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_SET this=\"0x%x\" line=\"%d\" string=\"%d\""
                                " type=\"%s\" asFunc=\"%s\">",
                                set,
                                set->base.lineNo,
                                set->base.stringNo,
                                _GetIRSetTypeName(set->type),
                                (set->funcName == gcvNULL)? "none" : set->funcName->symbol));

    if (set->funcName != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- Function Name -->"));

        gcmVERIFY_OK(slsNAME_Dump(Compiler, set->funcName));
    }

    FOR_EACH_DLINK_NODE(&set->members, struct _sloIR_BASE, member)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, member));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_SET>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_SET_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    gceSTATUS   status = gcvSTATUS_OK;
    sloIR_SET   set = (sloIR_SET)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x Visitor=0x%x Parameters=0x%x",
                   Compiler, This, Visitor, Parameters);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(set, slvIR_SET);
    gcmASSERT(Visitor);

    if (Visitor->visitSet == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = Visitor->visitSet(Compiler, Visitor, set, Parameters);

    gcmFOOTER();
    return status;
}

static slsVTAB s_setVTab =
{
    slvIR_SET,
    sloIR_SET_Destroy,
    sloIR_SET_Dump,
    sloIR_SET_Accept
};

gceSTATUS
sloIR_SET_Construct(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleSET_TYPE Type,
    OUT sloIR_SET * Set
    )
{
    gceSTATUS status;
    sloIR_SET set;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%u StringNo=%u Type=%d Set=0x%x",
                   Compiler, LineNo, StringNo, Type, Set);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Set);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_SET),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        set = pointer;

        sloIR_BASE_Initialize(&set->base, &s_setVTab, LineNo, StringNo);

        set->type       = Type;

        slsDLINK_LIST_Initialize(&set->members);

        set->funcName   = gcvNULL;

        *Set = set;

        gcmFOOTER_ARG("*Set=0x%x", *Set);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *Set = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_SET_AddMember(
    IN sloCOMPILER Compiler,
    IN sloIR_SET Set,
    IN sloIR_BASE Member
    )
{
    gcmHEADER_ARG("Compiler=0x%x Set=0x%x Member=0x%x",
                   Compiler, Set, Member);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Set, slvIR_SET);

    slsDLINK_LIST_InsertLast(&Set->members, &Member->node);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_SET_GetMemberCount(
    IN sloCOMPILER Compiler,
    IN sloIR_SET Set,
    OUT gctUINT * MemberCount
    )
{
    gctUINT     count = 0;
    sloIR_BASE  member;

    gcmHEADER_ARG("Compiler=0x%x Set=0x%x MemberCount=0x%x",
                   Compiler, Set, MemberCount);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Set, slvIR_SET);

    FOR_EACH_DLINK_NODE(&Set->members, struct _sloIR_BASE, member)
    {
        count++;
    }

    *MemberCount = count;

    gcmFOOTER_ARG("*MemberCount=%u", *MemberCount);
    return gcvSTATUS_OK;
}

/* sloIR_ITERATION object. */
gceSTATUS
sloIR_ITERATION_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_ITERATION iteration = (sloIR_ITERATION)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(iteration, slvIR_ITERATION);

    if (iteration->condExpr != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &iteration->condExpr->base));
    }

    if (iteration->loopBody != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, iteration->loopBody));
    }

    if (iteration->forInitStatement != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, iteration->forInitStatement));
    }

    if (iteration->forRestExpr != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &iteration->forRestExpr->base));
    }

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, iteration));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctCONST_STRING
_GetIRIterationTypeName(
    IN sleITERATION_TYPE IterationType
    )
{
    switch (IterationType)
    {
    case slvFOR:        return "for";
    case slvWHILE:      return "while";
    case slvDO_WHILE:   return "do-while";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
sloIR_ITERATION_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_ITERATION iteration = (sloIR_ITERATION)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                  Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(iteration, slvIR_ITERATION);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_ITERATION line=\"%d\" string=\"%d\" type=\"%s\"",
                                iteration->base.lineNo,
                                iteration->base.stringNo,
                                _GetIRIterationTypeName(iteration->type)));

    if (iteration->forSpace != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    " forSpace=\"0x%x\"",
                                    iteration->forSpace));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                ">"));

    if (iteration->condExpr != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- Condition Expression -->"));

        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &iteration->condExpr->base));
    }

    if (iteration->loopBody != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- Loop Body -->"));

        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, iteration->loopBody));
    }

    if (iteration->forInitStatement != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- For Init Statement -->"));

        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, iteration->forInitStatement));
    }

    if (iteration->forRestExpr != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- For Rest Expression -->"));

        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &iteration->forRestExpr->base));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_ITERATION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    gceSTATUS       status    = gcvSTATUS_OK;
    sloIR_ITERATION iteration = (sloIR_ITERATION)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x Visitor=0x%x Parameters=0x%x",
                  Compiler, This, Visitor, Parameters);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(iteration, slvIR_ITERATION);
    gcmASSERT(Visitor);

    if (Visitor->visitIteration == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = Visitor->visitIteration(Compiler, Visitor, iteration, Parameters);

    gcmFOOTER();
    return status;
}

static slsVTAB s_iterationVTab =
{
    slvIR_ITERATION,
    sloIR_ITERATION_Destroy,
    sloIR_ITERATION_Dump,
    sloIR_ITERATION_Accept
};

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
    )
{
    gceSTATUS           status;
    sloIR_ITERATION     iteration;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%u StringNo=%u Type=%d CondExpr=0x%x LoopBody=0x%x "
                  "ForSpace=%d ForInitStatement=0x%x ForRestExpr=0x%x Iteration=0x%x",
                  Compiler, LineNo, StringNo, Type, CondExpr, LoopBody,
                  ForSpace, ForInitStatement, ForRestExpr, Iteration);


    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_ITERATION),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        iteration = pointer;

        sloIR_BASE_Initialize(&iteration->base, &s_iterationVTab, LineNo, StringNo);

        iteration->type             = Type;
        iteration->condExpr         = CondExpr;
        iteration->loopBody         = LoopBody;
        iteration->forSpace         = ForSpace;
        iteration->forInitStatement = ForInitStatement;
        iteration->forRestExpr      = ForRestExpr;

        *Iteration = iteration;

        gcmFOOTER_ARG("*Iteration=0x%x", *Iteration);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *Iteration = gcvNULL;

    gcmFOOTER();
    return status;
}

/* sloIR_JUMP object. */
gceSTATUS
sloIR_JUMP_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_JUMP  jump = (sloIR_JUMP)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(jump, slvIR_JUMP);

    if (jump->returnExpr != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &jump->returnExpr->base));
    }

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, jump));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gctCONST_STRING
slGetIRJumpTypeName(
    IN sleJUMP_TYPE JumpType
    )
{
    switch (JumpType)
    {
    case slvCONTINUE:   return "continue";
    case slvBREAK:      return "break";
    case slvRETURN:     return "return";
    case slvDISCARD:    return "discard";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
sloIR_JUMP_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_JUMP  jump = (sloIR_JUMP)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(jump, slvIR_JUMP);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_JUMP line=\"%d\" string=\"%d\" type=\"%s\">",
                                jump->base.lineNo,
                                jump->base.stringNo,
                                slGetIRJumpTypeName(jump->type)));

    if (jump->type == slvRETURN)
    {
        if (jump->returnExpr != gcvNULL)
        {
            gcmVERIFY_OK(sloCOMPILER_Dump(
                                        Compiler,
                                        slvDUMP_IR,
                                        "<!-- Return Expression -->"));

            gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &jump->returnExpr->base));
        }
    }
    else
    {
        gcmASSERT(jump->returnExpr == gcvNULL);
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_JUMP>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_JUMP_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    gceSTATUS   status = gcvSTATUS_OK;
    sloIR_JUMP  jump = (sloIR_JUMP)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x Visitor=0x%x Parameters=0x%x",
                   Compiler, This, Visitor, Parameters);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(jump, slvIR_JUMP);
    gcmASSERT(Visitor);

    if (Visitor->visitJump == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status =  Visitor->visitJump(Compiler, Visitor, jump, Parameters);

    gcmFOOTER();
    return status;
}

static slsVTAB s_jumpVTab =
{
    slvIR_JUMP,
    sloIR_JUMP_Destroy,
    sloIR_JUMP_Dump,
    sloIR_JUMP_Accept
};

gceSTATUS
sloIR_JUMP_Construct(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleJUMP_TYPE Type,
    IN sloIR_EXPR ReturnExpr,
    OUT sloIR_JUMP * Jump
    )
{
    gceSTATUS       status;
    sloIR_JUMP      jump;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%u StringNo=%u Type=%d ReturnExpr=0x%x Jump=0x%x",
                   Compiler, LineNo, StringNo, Type, ReturnExpr, Jump);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_JUMP),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        jump = pointer;

        sloIR_BASE_Initialize(&jump->base, &s_jumpVTab, LineNo, StringNo);

        jump->type          = Type;
        jump->returnExpr    = ReturnExpr;

        *Jump = jump;

        gcmFOOTER_ARG("*Jump=0x%x", *Jump);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *Jump = gcvNULL;

     gcmFOOTER();
    return status;
}

/* sloIR_VARIABLE object. */
gceSTATUS
sloIR_VARIABLE_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_VARIABLE  variable = (sloIR_VARIABLE)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(variable, slvIR_VARIABLE);

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, variable));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_VARIABLE_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_VARIABLE  variable = (sloIR_VARIABLE)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(variable, slvIR_VARIABLE);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_VARIABLE line=\"%d\" string=\"%d\">",
                                variable->exprBase.base.lineNo,
                                variable->exprBase.base.stringNo));

    gcmASSERT(variable->name);
    gcmVERIFY_OK(slsNAME_Dump(Compiler, variable->name));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_VARIABLE>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_VARIABLE_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    gceSTATUS       status   = gcvSTATUS_OK;
    sloIR_VARIABLE  variable = (sloIR_VARIABLE)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x Visitor=0x%x Parameters=0x%x",
                   Compiler, This, Visitor, Parameters);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(variable, slvIR_VARIABLE);
    gcmASSERT(Visitor);

    if (Visitor->visitVariable == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = Visitor->visitVariable(Compiler, Visitor, variable, Parameters);

    gcmFOOTER();
    return status;
}

static slsVTAB s_variableVTab =
{
    slvIR_VARIABLE,
    sloIR_VARIABLE_Destroy,
    sloIR_VARIABLE_Dump,
    sloIR_VARIABLE_Accept
};

gceSTATUS
sloIR_VARIABLE_Construct(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsNAME * Name,
    OUT sloIR_VARIABLE * Variable
    )
{
    gceSTATUS status;
    sloIR_VARIABLE variable;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%u StringNo=%u Name=0x%x Variable=0x%x",
                   Compiler, LineNo, StringNo, Name, Variable);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Name);

    do
    {
        gctPOINTER pointer = gcvNULL;

        if (Name->dataType == gcvNULL)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvREPORT_ERROR,
                                            "'%s' has no data type",
                                            Name->symbol));

            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_VARIABLE),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        variable = pointer;

        sloIR_EXPR_Initialize(&variable->exprBase, &s_variableVTab, LineNo, StringNo, Name->dataType);

        variable->name              = Name;

        *Variable = variable;

        gcmFOOTER_ARG("*Variable=0x%x", *Variable);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *Variable = gcvNULL;

    gcmFOOTER();
    return status;
}

/* sloIR_CONSTANT object. */
gceSTATUS
sloIR_CONSTANT_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_CONSTANT  constant = (sloIR_CONSTANT)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(constant, slvIR_CONSTANT);

    if (constant->valueCount > 0)
    {
        gcmASSERT(constant->values);

        gcmVERIFY_OK(sloCOMPILER_Free(Compiler, constant->values));
    }

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, constant));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_CONSTANT_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    gctUINT         i;
    sloIR_CONSTANT  constant = (sloIR_CONSTANT)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x",
                   Compiler, This);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(constant, slvIR_CONSTANT);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_CONSTANT line=\"%d\" string=\"%d\" dataType=\"0x%x\""
                                " valueCount=\"%d\" value=\"0x%x\" >",
                                constant->exprBase.base.lineNo,
                                constant->exprBase.base.stringNo,
                                constant->exprBase.dataType,
                                constant->valueCount,
                                constant->values));

    for (i = 0; i < constant->valueCount; i++)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<VALUE bool=\"%s\" int=\"%d\" float=\"%f\" />",
                                    (constant->values[i].boolValue) ? "true" : "false",
                                    constant->values[i].intValue,
                                    constant->values[i].floatValue));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_CONSTANT>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_CONSTANT_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    gceSTATUS       status   = gcvSTATUS_OK;
    sloIR_CONSTANT  constant = (sloIR_CONSTANT)This;

    gcmHEADER_ARG("Compiler=0x%x This=0x%x Visitor=0x%x Parameters=0x%x",
                   Compiler, This, Visitor, Parameters);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(constant, slvIR_CONSTANT);
    gcmASSERT(Visitor);

    if (Visitor->visitConstant == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = Visitor->visitConstant(Compiler, Visitor, constant, Parameters);
    gcmFOOTER();
    return status;
}

static slsVTAB s_constantVTab =
{
    slvIR_CONSTANT,
    sloIR_CONSTANT_Destroy,
    sloIR_CONSTANT_Dump,
    sloIR_CONSTANT_Accept
};

gceSTATUS
sloIR_CONSTANT_Construct(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsDATA_TYPE * DataType,
    OUT sloIR_CONSTANT * Constant
    )
{
    gceSTATUS       status;
    sloIR_CONSTANT  constant;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%u StringNo=%u DataType=0x%x Constant=0x%x",
                   Compiler, LineNo, StringNo, DataType, Constant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(DataType);
    gcmASSERT(DataType->qualifier == slvQUALIFIER_CONST);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_CONSTANT),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        constant = pointer;

        sloIR_EXPR_Initialize(&constant->exprBase, &s_constantVTab, LineNo, StringNo, DataType);

        constant->valueCount        = 0;
        constant->values            = gcvNULL;

        *Constant = constant;

        gcmFOOTER_ARG("*Constant=0x%x", *Constant);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *Constant = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_CONSTANT_Clone(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sloIR_CONSTANT Source,
    OUT sloIR_CONSTANT * Constant
    )
{
    gceSTATUS           status;
    sluCONSTANT_VALUE * values = gcvNULL;
    sloIR_CONSTANT      constant;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%u StringNo=%u Source=0x%x Constant=0x%x",
                   Compiler, LineNo, StringNo, Source, Constant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Source, slvIR_CONSTANT);

    do
    {
        gctPOINTER pointer = gcvNULL;

        if (Source->valueCount > 0)
        {
            status = sloCOMPILER_Allocate(
                                        Compiler,
                                        (gctSIZE_T)sizeof(sluCONSTANT_VALUE) * Source->valueCount,
                                        &pointer);

            if (gcmIS_ERROR(status)) break;

            values = pointer;

            gcoOS_MemCopy(
                        values,
                        Source->values,
                        (gctSIZE_T)sizeof(sluCONSTANT_VALUE) * Source->valueCount);
        }

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_CONSTANT),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        constant = pointer;

        sloIR_EXPR_Initialize(
                            &constant->exprBase,
                            &s_constantVTab,
                            LineNo,
                            StringNo,
                            Source->exprBase.dataType);

        constant->valueCount        = Source->valueCount;
        constant->values            = values;

        *Constant = constant;

        gcmFOOTER_ARG("*Constant=0x%x", *Constant);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (values != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Free(Compiler, values));
    }

    *Constant = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_CONSTANT_AddValues(
    IN sloCOMPILER Compiler,
    IN sloIR_CONSTANT Constant,
    IN gctUINT ValueCount,
    IN sluCONSTANT_VALUE * Values
    )
{
    gceSTATUS           status;
    sluCONSTANT_VALUE * newValues = gcvNULL;
    gctUINT             i;

    gcmHEADER_ARG("Compiler=0x%x Constant=0x%x ValueCount=%u Values=0x%x",
                   Compiler, Constant, ValueCount, Values);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(ValueCount > 0);

    do
    {
        gctPOINTER pointer = gcvNULL;

        if (Constant->valueCount == 0)
        {
            gcmASSERT(Constant->values == gcvNULL);

            status = sloCOMPILER_Allocate(
                                        Compiler,
                                        (gctSIZE_T)sizeof(sluCONSTANT_VALUE) * ValueCount,
                                        &pointer);

            if (gcmIS_ERROR(status)) break;

            newValues = pointer;

            for (i = 0; i < ValueCount; i++)
            {
                newValues[i] = Values[i];
            }

            Constant->valueCount    = ValueCount;
            Constant->values        = newValues;
        }
        else
        {
            gcmASSERT(Constant->values);

            status = sloCOMPILER_Allocate(
                                        Compiler,
                                        (gctSIZE_T)sizeof(sluCONSTANT_VALUE) *
                                            (Constant->valueCount + ValueCount),
                                        &pointer);

            if (gcmIS_ERROR(status)) break;

            newValues = pointer;

            gcoOS_MemCopy(
                        newValues,
                        Constant->values,
                        (gctSIZE_T)sizeof(sluCONSTANT_VALUE) * Constant->valueCount);

            for (i = 0; i < ValueCount; i++)
            {
                newValues[Constant->valueCount + i] = Values[i];
            }

            gcmVERIFY_OK(sloCOMPILER_Free(Compiler, Constant->values));

            Constant->valueCount    += ValueCount;
            Constant->values        = newValues;
        }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (newValues != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Free(Compiler, newValues));
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_CONSTANT_GetBoolValue(
    IN sloCOMPILER Compiler,
    IN sloIR_CONSTANT Constant,
    IN gctUINT ValueNo,
    OUT sluCONSTANT_VALUE * Value
    )
{
    gcmHEADER_ARG("Compiler=0x%x Constant=0x%x ValueNo=%u Value=0x%x",
                   Compiler, Constant, ValueNo, Value);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(Constant->valueCount > ValueNo);
    gcmASSERT(Value);

    switch (Constant->exprBase.dataType->elementType)
    {
    case slvTYPE_BOOL:
        Value->boolValue = Constant->values[ValueNo].boolValue;
        break;

    case slvTYPE_INT:
        Value->boolValue = slmI2B(Constant->values[ValueNo].intValue);
        break;

    case slvTYPE_UINT:
        Value->boolValue = slmUI2B(Constant->values[ValueNo].uintValue);
        break;

    case slvTYPE_FLOAT:
        Value->boolValue = slmF2B(Constant->values[ValueNo].floatValue);
        break;

    default:
        gcmASSERT(0);
    }

    gcmFOOTER_ARG("*Value=%d", Value->boolValue);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_CONSTANT_GetIntValue(
    IN sloCOMPILER Compiler,
    IN sloIR_CONSTANT Constant,
    IN gctUINT ValueNo,
    OUT sluCONSTANT_VALUE * Value
    )
{
    gcmHEADER_ARG("Compiler=0x%x Constant=0x%x ValueNo=%u Value=0x%x",
                   Compiler, Constant, ValueNo, Value);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(Constant->valueCount > ValueNo);
    gcmASSERT(Value);

    switch (Constant->exprBase.dataType->elementType)
    {
    case slvTYPE_BOOL:
        Value->intValue = slmB2I(Constant->values[ValueNo].boolValue);
        break;

    case slvTYPE_INT:
        Value->intValue = Constant->values[ValueNo].intValue;
        break;

    case slvTYPE_UINT:
        Value->intValue = slmUI2I(Constant->values[ValueNo].uintValue);
        break;

    case slvTYPE_FLOAT:
        Value->intValue = slmF2I(Constant->values[ValueNo].floatValue);
        break;

    default:
        gcmASSERT(0);
    }

    gcmFOOTER_ARG("*Value=%d", Value->intValue);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_CONSTANT_GetUIntValue(
    IN sloCOMPILER Compiler,
    IN sloIR_CONSTANT Constant,
    IN gctUINT ValueNo,
    OUT sluCONSTANT_VALUE * Value
    )
{
    gcmHEADER_ARG("Compiler=0x%x Constant=0x%x ValueNo=%u Value=0x%x",
                   Compiler, Constant, ValueNo, Value);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(Constant->valueCount > ValueNo);
    gcmASSERT(Value);

    switch (Constant->exprBase.dataType->elementType)
    {
    case slvTYPE_BOOL:
        Value->uintValue = slmB2I(Constant->values[ValueNo].boolValue);
        break;

    case slvTYPE_INT:
        Value->uintValue = slmI2UI(Constant->values[ValueNo].intValue);
        break;

    case slvTYPE_UINT:
        Value->uintValue = Constant->values[ValueNo].uintValue;
        break;

    case slvTYPE_FLOAT:
        Value->uintValue = slmF2UI(Constant->values[ValueNo].floatValue);
        break;

    default:
        gcmASSERT(0);
    }

    gcmFOOTER_ARG("*Value=%u", Value->uintValue);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_CONSTANT_GetFloatValue(
    IN sloCOMPILER Compiler,
    IN sloIR_CONSTANT Constant,
    IN gctUINT ValueNo,
    OUT sluCONSTANT_VALUE * Value
    )
{
    gcmHEADER_ARG("Compiler=0x%x Constant=0x%x ValueNo=%u Value=0x%x",
                   Compiler, Constant, ValueNo, Value);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(Constant->valueCount > ValueNo);
    gcmASSERT(Value);

    switch (Constant->exprBase.dataType->elementType)
    {
    case slvTYPE_BOOL:
        Value->floatValue = slmB2F(Constant->values[ValueNo].boolValue);
        break;

    case slvTYPE_INT:
        Value->floatValue = slmI2F(Constant->values[ValueNo].intValue);
        break;

    case slvTYPE_UINT:
        Value->floatValue = slmUI2F(Constant->values[ValueNo].uintValue);
        break;

    case slvTYPE_FLOAT:
        Value->floatValue = Constant->values[ValueNo].floatValue;
        break;

    default:
        gcmASSERT(0);
    }

    gcmFOOTER_ARG("*Value=%f", Value->floatValue);
    return gcvSTATUS_OK;
}

static gceSTATUS
_EvaluateConstantValues(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN OUT gctUINT * Offset,
    IN OUT sluCONSTANT_VALUE * Values,
    IN sltEVALUATE_FUNC_PTR Evaluate
    )
{
    gceSTATUS       status;
    slsDATA_TYPE    subType;
    gctUINT8        i;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Offset=0x%x Values=0x%x Evaluate=0x%x",
                  Compiler, DataType, Offset, Values, Evaluate);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(DataType);
    gcmASSERT(Offset);
    gcmASSERT(Values);
    gcmASSERT(Evaluate);

    if (DataType->arrayLength != 0)
    {
        subType             = *DataType;
        subType.arrayLength = 0;

        for (i = 0; i < DataType->arrayLength; i++)
        {
            status = _EvaluateConstantValues(Compiler,
                                             &subType,
                                             Offset,
                                             Values,
                                             Evaluate);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }
    }
    else if (slmDATA_TYPE_matrixSize_GET(DataType) != 0)
    {
        subType             = *DataType;
        slmDATA_TYPE_matrixSize_SET(&subType, 0, 0);

        for (i = 0; i < slmDATA_TYPE_matrixColumnCount_GET(DataType) * slmDATA_TYPE_matrixRowCount_GET(DataType); i++)
        {
            status = _EvaluateConstantValues(Compiler,
                                             &subType,
                                             Offset,
                                             Values,
                                             Evaluate);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }
    }
    else if (slmDATA_TYPE_vectorSize_GET(DataType) != 0)
    {
        subType             = *DataType;
        slmDATA_TYPE_vectorSize_SET(&subType, 0);

        for (i = 0; i < slmDATA_TYPE_vectorSize_NOCHECK_GET(DataType); i++)
        {
            status = _EvaluateConstantValues(Compiler,
                                             &subType,
                                             Offset,
                                             Values,
                                             Evaluate);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }
    }
    else
    {
        switch (DataType->elementType)
        {
        case slvTYPE_BOOL:
        case slvTYPE_INT:
        case slvTYPE_UINT:
        case slvTYPE_FLOAT:
            status = (*Evaluate)(DataType->elementType, Values + *Offset);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }

            (*Offset)++;
            break;

        default:
            gcmASSERT(0);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
    }

    gcmFOOTER_ARG("*Offset=%f *Values=%f", *Offset, Values->intValue);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_CONSTANT_Evaluate(
    IN sloCOMPILER Compiler,
    IN OUT sloIR_CONSTANT Constant,
    IN sltEVALUATE_FUNC_PTR Evaluate
    )
{
    gceSTATUS   status;
    gctUINT     offset = 0;

    gcmHEADER_ARG("Compiler=0x%x Constant=0x%x Evaluate=0x%x",
                   Compiler, Constant, Evaluate);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(Evaluate);

    gcmASSERT(Constant->exprBase.dataType);
    gcmASSERT(Constant->values);

/*klc*/
if(Constant->exprBase.base.lineNo == 331) {
   status = gcvSTATUS_OK;
}
    status = _EvaluateConstantValues(
                                Compiler,
                                Constant->exprBase.dataType,
                                &offset,
                                Constant->values,
                                Evaluate);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

/*KLC */
if(offset != Constant->valueCount) {
    gcmASSERT(offset == Constant->valueCount);
}

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_Mat_Mul_Mat(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gctUINT8    col, row, i;
    gctUINT8    matrixSize;
    gctUINT8    matrixRowCount, matrixColumnCount;
    gctFLOAT    resultMatrix[16] = {0};

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                   Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsMat(LeftConstant->exprBase.dataType));
    gcmASSERT(slsDATA_TYPE_IsEqual(
                                LeftConstant->exprBase.dataType,
                                RightConstant->exprBase.dataType));

    matrixRowCount = slmDATA_TYPE_matrixRowCount_GET(LeftConstant->exprBase.dataType);
    matrixColumnCount = slmDATA_TYPE_matrixColumnCount_GET(RightConstant->exprBase.dataType);

    matrixSize = slmDATA_TYPE_matrixColumnCount_GET(LeftConstant->exprBase.dataType);
    gcmASSERT(LeftConstant->valueCount == (gctUINT) (matrixRowCount * matrixSize));
    gcmASSERT(RightConstant->valueCount == (gctUINT) (matrixSize * matrixColumnCount));

    /* Calculate the result */
    for (col = 0; col < matrixColumnCount; col++)
    {
        for (row = 0; row < matrixRowCount; row++)
        {
            for (i = 0; i < matrixSize; i++)
            {
                resultMatrix[col * matrixRowCount + row] +=
                        LeftConstant->values[i * matrixRowCount + row].floatValue
                            * RightConstant->values[col * matrixSize + i].floatValue;
            }
        }
    }

    /* Save the result back */
    for (i = 0; i < matrixSize * matrixSize; i++)
    {
        LeftConstant->values[i].floatValue = resultMatrix[i];
    }

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &RightConstant->exprBase.base));

    *ResultConstant = LeftConstant;

    gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_Vec_Mul_Mat(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gctUINT8    i, j;
    gctUINT8    vectorSize;
    gctUINT8    matrixColumnCount;
    gctFLOAT    resultVector[4];

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                   Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsVec(LeftConstant->exprBase.dataType));
    gcmASSERT(slsDATA_TYPE_IsMat(RightConstant->exprBase.dataType));
    gcmASSERT(slmDATA_TYPE_vectorSize_GET(LeftConstant->exprBase.dataType)
                == slmDATA_TYPE_matrixRowCount_GET(RightConstant->exprBase.dataType));

    vectorSize = slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftConstant->exprBase.dataType);

    gcmASSERT(LeftConstant->valueCount == (gctUINT) vectorSize);
    gcmASSERT(RightConstant->valueCount ==
             (gctUINT) (vectorSize * slmDATA_TYPE_matrixColumnCount_GET(RightConstant->exprBase.dataType)));

    /* Calculate the result */
    matrixColumnCount = slmDATA_TYPE_matrixColumnCount_GET(RightConstant->exprBase.dataType);
    for (i = 0; i < matrixColumnCount; i++)
    {
        resultVector[i] = 0;

        for (j = 0; j < vectorSize; j++)
        {
            resultVector[i] +=
                    LeftConstant->values[j].floatValue
                        * RightConstant->values[i * vectorSize + j].floatValue;
        }
    }

    /* Save the result back */
    RightConstant->exprBase.base.lineNo     = LeftConstant->exprBase.base.lineNo;
    RightConstant->exprBase.base.stringNo   = LeftConstant->exprBase.base.stringNo;
    for (i = 0; i < matrixColumnCount; i++)
    {
        RightConstant->values[i].floatValue = resultVector[i];
    }

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &LeftConstant->exprBase.base));

    *ResultConstant = RightConstant;

    gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_Mat_Mul_Vec(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gctUINT8    i, j;
    gctUINT8    vectorSize;
    gctUINT8    matrixRowCount;
    gctFLOAT    resultVector[4];

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                   Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsMat(LeftConstant->exprBase.dataType));
    gcmASSERT(slsDATA_TYPE_IsVec(RightConstant->exprBase.dataType));
    gcmASSERT(slmDATA_TYPE_matrixColumnCount_GET(LeftConstant->exprBase.dataType)
                == slmDATA_TYPE_vectorSize_GET(RightConstant->exprBase.dataType));

    vectorSize = slmDATA_TYPE_vectorSize_NOCHECK_GET(RightConstant->exprBase.dataType);

    gcmASSERT(LeftConstant->valueCount ==
              (gctUINT) slmDATA_TYPE_matrixColumnCount_GET(LeftConstant->exprBase.dataType) * vectorSize);
    gcmASSERT(RightConstant->valueCount == (gctUINT) vectorSize);

    /* Calculate the result */
    matrixRowCount = slmDATA_TYPE_matrixRowCount_GET(LeftConstant->exprBase.dataType);
    for (i = 0; i < matrixRowCount; i++)
    {
        resultVector[i] = 0;

        for (j = 0; j < vectorSize; j++)
        {
            resultVector[i] +=
                    LeftConstant->values[j * matrixRowCount + i].floatValue
                        * RightConstant->values[j].floatValue;
        }
    }

    /* Save the result back */
    for (i = 0; i < matrixRowCount; i++)
    {
        LeftConstant->values[i].floatValue = resultVector[i];
    }

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &RightConstant->exprBase.base));

    *ResultConstant = LeftConstant;

    gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_ArithmeticOperateBySameTypes(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gctUINT     i;
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                  Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsEqual(
                                LeftConstant->exprBase.dataType,
                                RightConstant->exprBase.dataType));
    gcmASSERT(RightConstant->valueCount == LeftConstant->valueCount);

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
    if (slsDATA_TYPE_IsInt(LeftConstant->exprBase.dataType)
        || slsDATA_TYPE_IsFloat(LeftConstant->exprBase.dataType))
    {
        gcmASSERT(LeftConstant->valueCount == 1);
    }
    else if (slsDATA_TYPE_IsIVec(LeftConstant->exprBase.dataType)
        || slsDATA_TYPE_IsVec(LeftConstant->exprBase.dataType))
    {
        gcmASSERT(LeftConstant->valueCount == slmDATA_TYPE_vectorSize_GET(LeftConstant->exprBase.dataType));
    }
    else if (slsDATA_TYPE_IsMat(LeftConstant->exprBase.dataType))
    {
        gcmASSERT(LeftConstant->valueCount ==
                        (gctUINT) (slmDATA_TYPE_matrixColumnCount_GET(LeftConstant->exprBase.dataType)
                            * slmDATA_TYPE_matrixRowCount_GET(LeftConstant->exprBase.dataType)));
    }
    else
    {
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
#endif /* gcmIS_DEBUG(gcdDEBUG_ASSERT) */

    if (ExprType == slvBINARY_MUL && slsDATA_TYPE_IsMat(LeftConstant->exprBase.dataType))
    {
         status = _sloIR_CONSTANT_Mat_Mul_Mat(
                                        Compiler,
                                        ExprType,
                                        LeftConstant,
                                        RightConstant,
                                        ResultConstant);

        gcmFOOTER_ARG("*ResultConstant=0x%x status=%d", *ResultConstant, status);
        return status;
    }

    switch (LeftConstant->exprBase.dataType->elementType)
    {
    case slvTYPE_INT:
        for (i = 0; i < LeftConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                LeftConstant->values[i].intValue += RightConstant->values[i].intValue;
                break;

            case slvBINARY_SUB:
                LeftConstant->values[i].intValue -= RightConstant->values[i].intValue;
                break;

            case slvBINARY_MUL:
                LeftConstant->values[i].intValue *= RightConstant->values[i].intValue;
                break;

            case slvBINARY_DIV:
                LeftConstant->values[i].intValue /= RightConstant->values[i].intValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    case slvTYPE_UINT:
        for (i = 0; i < LeftConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                LeftConstant->values[i].uintValue += RightConstant->values[i].uintValue;
                break;

            case slvBINARY_SUB:
                LeftConstant->values[i].uintValue -= RightConstant->values[i].uintValue;
                break;

            case slvBINARY_MUL:
                LeftConstant->values[i].uintValue *= RightConstant->values[i].uintValue;
                break;

            case slvBINARY_DIV:
                LeftConstant->values[i].uintValue /= RightConstant->values[i].uintValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    case slvTYPE_FLOAT:
        for (i = 0; i < LeftConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                LeftConstant->values[i].floatValue += RightConstant->values[i].floatValue;
                break;

            case slvBINARY_SUB:
                LeftConstant->values[i].floatValue -= RightConstant->values[i].floatValue;
                break;

            case slvBINARY_MUL:
                LeftConstant->values[i].floatValue *= RightConstant->values[i].floatValue;
                break;

            case slvBINARY_DIV:
                LeftConstant->values[i].floatValue /= RightConstant->values[i].floatValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    default:
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &RightConstant->exprBase.base));

    *ResultConstant = LeftConstant;

    gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_Scalar_ArithmeticOperate_VectorOrMatrix(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gctUINT     i;
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                  Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);

    gcmASSERT(LeftConstant->valueCount == 1);

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
    if (slsDATA_TYPE_IsIVec(RightConstant->exprBase.dataType)
        || slsDATA_TYPE_IsVec(RightConstant->exprBase.dataType))
    {
        gcmASSERT(RightConstant->valueCount == slmDATA_TYPE_vectorSize_GET(RightConstant->exprBase.dataType));
    }
    else if (slsDATA_TYPE_IsMat(RightConstant->exprBase.dataType))
    {
        gcmASSERT(RightConstant->valueCount ==
                        (gctUINT) (slmDATA_TYPE_matrixColumnCount_GET(RightConstant->exprBase.dataType)
                            * slmDATA_TYPE_matrixRowCount_GET(RightConstant->exprBase.dataType)));
    }
    else
    {
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
#endif /* gcmIS_DEBUG(gcdDEBUG_ASSERT) */

    switch (RightConstant->exprBase.dataType->elementType)
    {
    case slvTYPE_INT:
        for (i = 0; i < RightConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                RightConstant->values[i].intValue =
                        LeftConstant->values[0].intValue + RightConstant->values[i].intValue;
                break;

            case slvBINARY_SUB:
                RightConstant->values[i].intValue =
                        LeftConstant->values[0].intValue - RightConstant->values[i].intValue;
                break;

            case slvBINARY_MUL:
                RightConstant->values[i].intValue =
                        LeftConstant->values[0].intValue * RightConstant->values[i].intValue;
                break;

            case slvBINARY_DIV:
                RightConstant->values[i].intValue =
                        LeftConstant->values[0].intValue / RightConstant->values[i].intValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    case slvTYPE_UINT:
        for (i = 0; i < RightConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                RightConstant->values[i].uintValue =
                        LeftConstant->values[0].uintValue + RightConstant->values[i].uintValue;
                break;

            case slvBINARY_SUB:
                RightConstant->values[i].uintValue =
                        LeftConstant->values[0].uintValue - RightConstant->values[i].uintValue;
                break;

            case slvBINARY_MUL:
                RightConstant->values[i].uintValue =
                        LeftConstant->values[0].uintValue * RightConstant->values[i].uintValue;
                break;

            case slvBINARY_DIV:
                RightConstant->values[i].uintValue =
                        LeftConstant->values[0].uintValue / RightConstant->values[i].uintValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    case slvTYPE_FLOAT:
        for (i = 0; i < RightConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                RightConstant->values[i].floatValue =
                        LeftConstant->values[0].floatValue + RightConstant->values[i].floatValue;
                break;

            case slvBINARY_SUB:
                RightConstant->values[i].floatValue =
                        LeftConstant->values[0].floatValue - RightConstant->values[i].floatValue;
                break;

            case slvBINARY_MUL:
                RightConstant->values[i].floatValue =
                        LeftConstant->values[0].floatValue * RightConstant->values[i].floatValue;
                break;

            case slvBINARY_DIV:
                RightConstant->values[i].floatValue =
                        LeftConstant->values[0].floatValue / RightConstant->values[i].floatValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    default:
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    RightConstant->exprBase.base.lineNo     = LeftConstant->exprBase.base.lineNo;
    RightConstant->exprBase.base.stringNo   = LeftConstant->exprBase.base.stringNo;

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &LeftConstant->exprBase.base));

    *ResultConstant = RightConstant;

    gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_VectorOrMatrix_ArithmeticOperate_Scalar(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gctUINT     i;
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                  Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
    if (slsDATA_TYPE_IsIVec(LeftConstant->exprBase.dataType)
        || slsDATA_TYPE_IsVec(LeftConstant->exprBase.dataType))
    {
        gcmASSERT(LeftConstant->valueCount == slmDATA_TYPE_vectorSize_GET(LeftConstant->exprBase.dataType));
    }
    else if (slsDATA_TYPE_IsMat(LeftConstant->exprBase.dataType))
    {
        gcmASSERT(LeftConstant->valueCount ==
                        (gctUINT) (slmDATA_TYPE_matrixColumnCount_GET(LeftConstant->exprBase.dataType)
                            * slmDATA_TYPE_matrixRowCount_GET(LeftConstant->exprBase.dataType)));
    }
    else
    {
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
#endif /* gcmIS_DEBUG(gcdDEBUG_ASSERT) */

    gcmASSERT(RightConstant->valueCount == 1);


    switch (LeftConstant->exprBase.dataType->elementType)
    {
    case slvTYPE_INT:
        for (i = 0; i < LeftConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                LeftConstant->values[i].intValue += RightConstant->values[0].intValue;
                break;

            case slvBINARY_SUB:
                LeftConstant->values[i].intValue -= RightConstant->values[0].intValue;
                break;

            case slvBINARY_MUL:
                LeftConstant->values[i].intValue *= RightConstant->values[0].intValue;
                break;

            case slvBINARY_DIV:
                LeftConstant->values[i].intValue /= RightConstant->values[0].intValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    case slvTYPE_UINT:
        for (i = 0; i < LeftConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                LeftConstant->values[i].uintValue += RightConstant->values[0].uintValue;
                break;

            case slvBINARY_SUB:
                LeftConstant->values[i].uintValue -= RightConstant->values[0].uintValue;
                break;

            case slvBINARY_MUL:
                LeftConstant->values[i].uintValue *= RightConstant->values[0].uintValue;
                break;

            case slvBINARY_DIV:
                LeftConstant->values[i].uintValue /= RightConstant->values[0].uintValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    case slvTYPE_FLOAT:
        for (i = 0; i < LeftConstant->valueCount; i++)
        {
            switch (ExprType)
            {
            case slvBINARY_ADD:
                LeftConstant->values[i].floatValue += RightConstant->values[0].floatValue;
                break;

            case slvBINARY_SUB:
                LeftConstant->values[i].floatValue -= RightConstant->values[0].floatValue;
                break;

            case slvBINARY_MUL:
                LeftConstant->values[i].floatValue *= RightConstant->values[0].floatValue;
                break;

            case slvBINARY_DIV:
                LeftConstant->values[i].floatValue /= RightConstant->values[0].floatValue;
                break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }
        }
        break;

    default:
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &RightConstant->exprBase.base));

    *ResultConstant = LeftConstant;

    gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_Subscript(
    IN sloCOMPILER Compiler,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS       status;
    gctREG_INDEX    index;
    gctUINT8        i;

    gcmHEADER_ARG("Compiler=0x%x LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                  Compiler, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);

    gcmASSERT(slsDATA_TYPE_IsInt(RightConstant->exprBase.dataType));
    index = (gctREG_INDEX)RightConstant->values[0].intValue;

    status = sloCOMPILER_CloneDataType(
                                    Compiler,
                                    LeftConstant->exprBase.dataType->qualifier,
                                    LeftConstant->exprBase.dataType,
                                    &LeftConstant->exprBase.dataType);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    if (slsDATA_TYPE_IsBVecOrIVecOrVec(LeftConstant->exprBase.dataType))
    {
        LeftConstant->values[0] = LeftConstant->values[index];

        LeftConstant->valueCount = 1;
        slmDATA_TYPE_vectorSize_SET(LeftConstant->exprBase.dataType, 0);
    }
    else
    {
	gctUINT8 rowCount;

        gcmASSERT(slsDATA_TYPE_IsMat(LeftConstant->exprBase.dataType));

	rowCount = slmDATA_TYPE_matrixRowCount_GET(LeftConstant->exprBase.dataType);
        for (i = 0; i < rowCount; i++)
        {
            LeftConstant->values[i] =
                LeftConstant->values[index * rowCount + i];
        }

        LeftConstant->valueCount = rowCount;
	slmDATA_TYPE_vectorSize_SET(LeftConstant->exprBase.dataType, rowCount);
    }

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &RightConstant->exprBase.base));

    *ResultConstant = LeftConstant;

    gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_ArithmeticOperate(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                  Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    if (slsDATA_TYPE_IsEqual(LeftConstant->exprBase.dataType, RightConstant->exprBase.dataType))
    {
        status = _sloIR_CONSTANT_ArithmeticOperateBySameTypes(
                                                            Compiler,
                                                            ExprType,
                                                            LeftConstant,
                                                            RightConstant,
                                                            ResultConstant);
        gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                       *ResultConstant, status);
        return status;
    }
    else
    {
        if (slsDATA_TYPE_IsInt(LeftConstant->exprBase.dataType))
        {
            gcmASSERT(slsDATA_TYPE_IsIVec(RightConstant->exprBase.dataType));

            status = _sloIR_CONSTANT_Scalar_ArithmeticOperate_VectorOrMatrix(
                                                                        Compiler,
                                                                        ExprType,
                                                                        LeftConstant,
                                                                        RightConstant,
                                                                        ResultConstant);
            gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                           *ResultConstant, status);
            return status;
        }
        else if (slsDATA_TYPE_IsIVec(LeftConstant->exprBase.dataType))
        {
            gcmASSERT(slsDATA_TYPE_IsInt(RightConstant->exprBase.dataType));

            status = _sloIR_CONSTANT_VectorOrMatrix_ArithmeticOperate_Scalar(
                                                                        Compiler,
                                                                        ExprType,
                                                                        LeftConstant,
                                                                        RightConstant,
                                                                        ResultConstant);
            gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                           *ResultConstant, status);
            return status;
        }
        else if (slsDATA_TYPE_IsFloat(LeftConstant->exprBase.dataType))
        {
            gcmASSERT(slsDATA_TYPE_IsVecOrMat(RightConstant->exprBase.dataType));

            status = _sloIR_CONSTANT_Scalar_ArithmeticOperate_VectorOrMatrix(
                                                                        Compiler,
                                                                        ExprType,
                                                                        LeftConstant,
                                                                        RightConstant,
                                                                        ResultConstant);
            gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                           *ResultConstant, status);
            return status;
        }
        else if (slsDATA_TYPE_IsVec(LeftConstant->exprBase.dataType))
        {
            if (ExprType != slvBINARY_MUL)
            {
                gcmASSERT(slsDATA_TYPE_IsFloat(RightConstant->exprBase.dataType));

                status = _sloIR_CONSTANT_VectorOrMatrix_ArithmeticOperate_Scalar(
                                                                            Compiler,
                                                                            ExprType,
                                                                            LeftConstant,
                                                                            RightConstant,
                                                                            ResultConstant);
                gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                               *ResultConstant, status);
                return status;
            }
            else
            {
                if (slsDATA_TYPE_IsFloat(RightConstant->exprBase.dataType))
                {
                    status = _sloIR_CONSTANT_VectorOrMatrix_ArithmeticOperate_Scalar(
                                                                                Compiler,
                                                                                ExprType,
                                                                                LeftConstant,
                                                                                RightConstant,
                                                                                ResultConstant);
                    gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                                   *ResultConstant, status);
                    return status;
                }
                else
                {
                    gcmASSERT(slsDATA_TYPE_IsMat(RightConstant->exprBase.dataType)
                        && slmDATA_TYPE_vectorSize_GET(LeftConstant->exprBase.dataType)
                                    == slmDATA_TYPE_matrixRowCount_GET(RightConstant->exprBase.dataType));

                    status = _sloIR_CONSTANT_Vec_Mul_Mat(Compiler,
                                                         ExprType,
                                                         LeftConstant,
                                                         RightConstant,
                                                         ResultConstant);
                    gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                                   *ResultConstant, status);
                    return status;
                }
            }
        }
        else if (slsDATA_TYPE_IsMat(LeftConstant->exprBase.dataType))
        {
            if (ExprType != slvBINARY_MUL)
            {
                gcmASSERT(slsDATA_TYPE_IsFloat(RightConstant->exprBase.dataType));

                status = _sloIR_CONSTANT_VectorOrMatrix_ArithmeticOperate_Scalar(
                                                                            Compiler,
                                                                            ExprType,
                                                                            LeftConstant,
                                                                            RightConstant,
                                                                            ResultConstant);
                gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                               *ResultConstant, status);
                return status;
            }
            else
            {
                if (slsDATA_TYPE_IsFloat(RightConstant->exprBase.dataType))
                {
                    status = _sloIR_CONSTANT_VectorOrMatrix_ArithmeticOperate_Scalar(
                                                                                Compiler,
                                                                                ExprType,
                                                                                LeftConstant,
                                                                                RightConstant,
                                                                                ResultConstant);
                    gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                                   *ResultConstant, status);
                    return status;
                }
                else
                {
                    gcmASSERT(slsDATA_TYPE_IsVec(RightConstant->exprBase.dataType)
                        && slmDATA_TYPE_matrixColumnCount_GET(LeftConstant->exprBase.dataType)
                                == slmDATA_TYPE_vectorSize_GET(RightConstant->exprBase.dataType));

                    status = _sloIR_CONSTANT_Mat_Mul_Vec(
                                                    Compiler,
                                                    ExprType,
                                                    LeftConstant,
                                                    RightConstant,
                                                    ResultConstant);
                    gcmFOOTER_ARG("*ResultConstant=0x%x status=%d",
                                   *ResultConstant, status);
                    return status;
                }
            }
        }
        else
        {
            gcmASSERT(0);

            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
    }
}

static gceSTATUS
_sloIR_CONSTANT_RelationalOperate(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS           status;
    slsDATA_TYPE *      resultDataType;
    sloIR_CONSTANT      resultConstant;
    sluCONSTANT_VALUE   value;

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                  Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsInt(LeftConstant->exprBase.dataType)
            || slsDATA_TYPE_IsFloat(LeftConstant->exprBase.dataType));
    gcmASSERT(slsDATA_TYPE_IsEqual(
                                LeftConstant->exprBase.dataType,
                                RightConstant->exprBase.dataType));

    /* Calculate the value */
    if (slsDATA_TYPE_IsInt(LeftConstant->exprBase.dataType))
    {
        switch (ExprType)
        {
        case slvBINARY_GREATER_THAN:
            value.boolValue =
                    (LeftConstant->values[0].intValue > RightConstant->values[0].intValue);
            break;

        case slvBINARY_LESS_THAN:
            value.boolValue =
                    (LeftConstant->values[0].intValue < RightConstant->values[0].intValue);
            break;

        case slvBINARY_GREATER_THAN_EQUAL:
            value.boolValue =
                    (LeftConstant->values[0].intValue >= RightConstant->values[0].intValue);
            break;

        case slvBINARY_LESS_THAN_EQUAL:
            value.boolValue =
                    (LeftConstant->values[0].intValue <= RightConstant->values[0].intValue);

            break;

        default:
            gcmASSERT(0);

            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
    }
    else
    {
        switch (ExprType)
        {
        case slvBINARY_GREATER_THAN:
            value.boolValue =
                    (LeftConstant->values[0].floatValue > RightConstant->values[0].floatValue);
            break;

        case slvBINARY_LESS_THAN:
            value.boolValue =
                    (LeftConstant->values[0].floatValue < RightConstant->values[0].floatValue);
            break;

        case slvBINARY_GREATER_THAN_EQUAL:
            value.boolValue =
                    (LeftConstant->values[0].floatValue >= RightConstant->values[0].floatValue);
            break;

        case slvBINARY_LESS_THAN_EQUAL:
            value.boolValue =
                    (LeftConstant->values[0].floatValue <= RightConstant->values[0].floatValue);

            break;

        default:
            gcmASSERT(0);

            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
    }

    do
    {
        /* Create the const bool data type */
        status = sloCOMPILER_CreateDataType(
                                            Compiler,
                                            T_BOOL,
                                            gcvNULL,
                                            &resultDataType);

        if (gcmIS_ERROR(status)) break;

        resultDataType->qualifier = slvQUALIFIER_CONST;

        /* Create the constant */
        status = sloIR_CONSTANT_Construct(
                                        Compiler,
                                        LeftConstant->exprBase.base.lineNo,
                                        LeftConstant->exprBase.base.stringNo,
                                        resultDataType,
                                        &resultConstant);

        if (gcmIS_ERROR(status)) break;

        /* Add the constant value */
        status = sloIR_CONSTANT_AddValues(
                                        Compiler,
                                        resultConstant,
                                        1,
                                        &value);

        if (gcmIS_ERROR(status)) break;

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &LeftConstant->exprBase.base));
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &RightConstant->exprBase.base));

        *ResultConstant = resultConstant;

        gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *ResultConstant = gcvNULL;

    gcmFOOTER();
    return status;
}

static gceSTATUS
_sloIR_CONSTANT_EqualityOperate(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gctUINT             i;
    gceSTATUS           status;
    slsDATA_TYPE *      resultDataType;
    sloIR_CONSTANT      resultConstant;
    sluCONSTANT_VALUE   value;

    gcmHEADER_ARG("Compiler=0x%x ExprType=%d LeftConstant=0x%x RightConstant=0x%x ResultConstant=0x%x",
                  Compiler, ExprType, LeftConstant, RightConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsAssignableAndComparable(LeftConstant->exprBase.dataType));
    gcmASSERT(slsDATA_TYPE_IsEqual(
                                LeftConstant->exprBase.dataType,
                                RightConstant->exprBase.dataType));
    gcmASSERT(LeftConstant->valueCount == RightConstant->valueCount);

    value.boolValue = gcvTRUE;

    for (i = 0; i < LeftConstant->valueCount; i++)
    {
        if (LeftConstant->values[i].intValue != RightConstant->values[i].intValue)
        {
            value.boolValue = gcvFALSE;
            break;
        }
    }

    switch (ExprType)
    {
    case slvBINARY_EQUAL:
        break;

    case slvBINARY_NOT_EQUAL:
        value.boolValue = !value.boolValue;
        break;

    default:
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    do
    {
        /* Create the const bool data type */
        status = sloCOMPILER_CreateDataType(
                                            Compiler,
                                            T_BOOL,
                                            gcvNULL,
                                            &resultDataType);

        if (gcmIS_ERROR(status)) break;

        resultDataType->qualifier = slvQUALIFIER_CONST;

        /* Create the constant */
        status = sloIR_CONSTANT_Construct(
                                        Compiler,
                                        LeftConstant->exprBase.base.lineNo,
                                        LeftConstant->exprBase.base.stringNo,
                                        resultDataType,
                                        &resultConstant);

        if (gcmIS_ERROR(status)) break;

        /* Add the constant value */
        status = sloIR_CONSTANT_AddValues(
                                        Compiler,
                                        resultConstant,
                                        1,
                                        &value);

        if (gcmIS_ERROR(status)) break;

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &LeftConstant->exprBase.base));
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &RightConstant->exprBase.base));

        *ResultConstant = resultConstant;

        gcmFOOTER_ARG("*ResultConstant=0x%x", *ResultConstant);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *ResultConstant = gcvNULL;

    gcmFOOTER();
    return status;
}

static gceSTATUS
_sloIR_CONSTANT_LogicalOperate(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE ExprType,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);
    gcmASSERT(ResultConstant);

    gcmASSERT(LeftConstant->exprBase.dataType);
    gcmASSERT(RightConstant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsBool(LeftConstant->exprBase.dataType));
    gcmASSERT(slsDATA_TYPE_IsBool(RightConstant->exprBase.dataType));

    switch (ExprType)
    {
    case slvBINARY_AND:
        LeftConstant->values[0].boolValue =
                (LeftConstant->values[0].boolValue && RightConstant->values[0].boolValue);
        break;

    case slvBINARY_OR:
        LeftConstant->values[0].boolValue =
                (LeftConstant->values[0].boolValue || RightConstant->values[0].boolValue);
        break;

    case slvBINARY_XOR:
        LeftConstant->values[0].boolValue =
                (LeftConstant->values[0].boolValue != RightConstant->values[0].boolValue);
        break;

    default:
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &RightConstant->exprBase.base));

    *ResultConstant = LeftConstant;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_sloIR_CONSTANT_SelectField(
    IN sloCOMPILER Compiler,
    IN sloIR_CONSTANT Constant,
    IN slsNAME * FieldName,
    OUT sloIR_CONSTANT * FieldConstant
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;
    sloIR_CONSTANT  fieldConstant;
    gctSIZE_T       size, offset;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(Constant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsStruct(Constant->exprBase.dataType));
    gcmASSERT(FieldName);
    gcmASSERT(FieldName->dataType);
    gcmASSERT(FieldConstant);

    do
    {
        status = sloCOMPILER_CloneDataType(
                                        Compiler,
                                        slvQUALIFIER_NONE,
                                        FieldName->dataType,
                                        &dataType);

        if (gcmIS_ERROR(status)) break;

        dataType->qualifier = slvQUALIFIER_CONST;

        status = sloIR_CONSTANT_Construct(
                                        Compiler,
                                        Constant->exprBase.base.lineNo,
                                        Constant->exprBase.base.stringNo,
                                        dataType,
                                        &fieldConstant);

        if (gcmIS_ERROR(status)) break;

        size = slsDATA_TYPE_GetSize(FieldName->dataType);
        gcmASSERT(size > 0);

        offset = slsDATA_TYPE_GetFieldOffset(Constant->exprBase.dataType, FieldName);

        status = sloIR_CONSTANT_AddValues(
                                        Compiler,
                                        fieldConstant,
                                        size,
                                        Constant->values + offset);

        if (gcmIS_ERROR(status)) break;

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &Constant->exprBase.base));

        *FieldConstant = fieldConstant;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *FieldConstant = gcvNULL;

    gcmFOOTER();
    return status;
}

static gceSTATUS
_sloIR_CONSTANT_SelectComponents(
    IN sloCOMPILER Compiler,
    IN sloIR_CONSTANT Constant,
    IN slsCOMPONENT_SELECTION * ComponentSelection,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS           status;
    gctUINT8            i;
    sluCONSTANT_VALUE   values[4];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);
    gcmASSERT(Constant->exprBase.dataType);
    gcmASSERT(slsDATA_TYPE_IsBVecOrIVecOrVec(Constant->exprBase.dataType));
    gcmASSERT(ComponentSelection);
    gcmASSERT(slmDATA_TYPE_vectorSize_GET(Constant->exprBase.dataType) >= ComponentSelection->components);
    gcmASSERT(ResultConstant);

    /* Swizzle the components */
    for (i = 0; i < ComponentSelection->components; i++)
    {
        switch (i)
        {
        case 0: values[i] = Constant->values[ComponentSelection->x]; break;
        case 1: values[i] = Constant->values[ComponentSelection->y]; break;
        case 2: values[i] = Constant->values[ComponentSelection->z]; break;
        case 3: values[i] = Constant->values[ComponentSelection->w]; break;

        default: gcmASSERT(0); break;
        }
    }

    /* Change to the result constant */
    status = sloCOMPILER_CloneDataType(
                                    Compiler,
                                    slvQUALIFIER_CONST,
                                    Constant->exprBase.dataType,
                                    &Constant->exprBase.dataType);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slmDATA_TYPE_vectorSize_SET(Constant->exprBase.dataType,
        (ComponentSelection->components == 1) ? 0 : ComponentSelection->components);

    Constant->valueCount = ComponentSelection->components;

    for (i = 0; i < ComponentSelection->components; i++)
    {
        Constant->values[i] = values[i];
    }

    *ResultConstant = Constant;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* sloIR_UNARY_EXPR object. */
gceSTATUS
sloIR_UNARY_EXPR_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_UNARY_EXPR    unaryExpr = (sloIR_UNARY_EXPR)This;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(unaryExpr, slvIR_UNARY_EXPR);

    gcmASSERT(unaryExpr->operand);
    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &unaryExpr->operand->base));

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, unaryExpr));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

slsCOMPONENT_SELECTION  ComponentSelection_X =
        {1, slvCOMPONENT_X, slvCOMPONENT_X, slvCOMPONENT_X, slvCOMPONENT_X};

slsCOMPONENT_SELECTION  ComponentSelection_Y =
        {1, slvCOMPONENT_Y, slvCOMPONENT_Y, slvCOMPONENT_Y, slvCOMPONENT_Y};

slsCOMPONENT_SELECTION  ComponentSelection_Z =
        {1, slvCOMPONENT_Z, slvCOMPONENT_Z, slvCOMPONENT_Z, slvCOMPONENT_Z};

slsCOMPONENT_SELECTION  ComponentSelection_W =
        {1, slvCOMPONENT_W, slvCOMPONENT_W, slvCOMPONENT_W, slvCOMPONENT_W};

slsCOMPONENT_SELECTION  ComponentSelection_XY =
        {2, slvCOMPONENT_X, slvCOMPONENT_Y, slvCOMPONENT_Y, slvCOMPONENT_Y};

slsCOMPONENT_SELECTION  ComponentSelection_XYZ =
        {3, slvCOMPONENT_X, slvCOMPONENT_Y, slvCOMPONENT_Z, slvCOMPONENT_Z};

slsCOMPONENT_SELECTION  ComponentSelection_XYZW =
        {4, slvCOMPONENT_X, slvCOMPONENT_Y, slvCOMPONENT_Z, slvCOMPONENT_W};

gctBOOL
slIsRepeatedComponentSelection(
    IN slsCOMPONENT_SELECTION * ComponentSelection
    )
{
    gctUINT8    components[4];
    gctUINT8    i, j;

    gcmASSERT(ComponentSelection);

    components[0] = ComponentSelection->x;
    components[1] = ComponentSelection->y;
    components[2] = ComponentSelection->z;
    components[3] = ComponentSelection->w;

    for (i = 0; i < ComponentSelection->components - 1; i++)
    {
        for (j = i + 1; j < ComponentSelection->components; j++)
        {
            if (components[i] == components[j]) return gcvTRUE;
        }
    }

    return gcvFALSE;
}

gctCONST_STRING
slGetIRUnaryExprTypeName(
    IN sleUNARY_EXPR_TYPE UnaryExprType
    )
{
    switch (UnaryExprType)
    {
    case slvUNARY_FIELD_SELECTION:      return ".";
    case slvUNARY_COMPONENT_SELECTION:  return ".";

    case slvUNARY_POST_INC:             return "x++";
    case slvUNARY_POST_DEC:             return "x--";
    case slvUNARY_PRE_INC:              return "++x";
    case slvUNARY_PRE_DEC:              return "--x";
    case slvUNARY_NEG:                  return "-";
    case slvUNARY_NOT:                  return "!";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
sloIR_UNARY_EXPR_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_UNARY_EXPR    unaryExpr = (sloIR_UNARY_EXPR)This;
    gctUINT8            i, component;
    const gctCHAR       componentNames[4] = {'x', 'y', 'z', 'w'};
    gceSTATUS           status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(unaryExpr, slvIR_UNARY_EXPR);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_UNARY_EXPR line=\"%d\" string=\"%d\""
                                " dataType=\"0x%x\" type=\"%s\">",
                                unaryExpr->exprBase.base.lineNo,
                                unaryExpr->exprBase.base.stringNo,
                                unaryExpr->exprBase.dataType,
                                slGetIRUnaryExprTypeName(unaryExpr->type)));

    gcmASSERT(unaryExpr->operand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<!-- Operand -->"));

    gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &unaryExpr->operand->base));

    switch (unaryExpr->type)
    {
    case slvUNARY_FIELD_SELECTION:
        gcmASSERT(unaryExpr->u.fieldName);

        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- Field -->"));

        gcmVERIFY_OK(slsNAME_Dump(Compiler, unaryExpr->u.fieldName));

        break;

    case slvUNARY_COMPONENT_SELECTION:
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<COMPONMENT_SELECTION value=\""));

        for (i = 0; i < unaryExpr->u.componentSelection.components; i++)
        {
            switch (i)
            {
            case 0: component = unaryExpr->u.componentSelection.x; break;
            case 1: component = unaryExpr->u.componentSelection.y; break;
            case 2: component = unaryExpr->u.componentSelection.z; break;
            case 3: component = unaryExpr->u.componentSelection.w; break;

            default:
                gcmASSERT(0);

                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }

            gcmVERIFY_OK(sloCOMPILER_Dump(
                                        Compiler,
                                        slvDUMP_IR,
                                        "%c",
                                        componentNames[component]));
        }

        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "\" />"));

        break;

    default:
        break;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_UNARY_EXPR>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    sloIR_UNARY_EXPR    unaryExpr = (sloIR_UNARY_EXPR)This;
    gceSTATUS           status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(unaryExpr, slvIR_UNARY_EXPR);
    gcmASSERT(Visitor);

    if (Visitor->visitUnaryExpr == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = Visitor->visitUnaryExpr(Compiler, Visitor, unaryExpr, Parameters);

    gcmFOOTER();
    return status;
}

static slsVTAB s_unaryExprVTab =
{
    slvIR_UNARY_EXPR,
    sloIR_UNARY_EXPR_Destroy,
    sloIR_UNARY_EXPR_Dump,
    sloIR_UNARY_EXPR_Accept
};

static gceSTATUS
_GetUnaryExprDataType(
    IN sloCOMPILER Compiler,
    IN sleUNARY_EXPR_TYPE Type,
    IN sloIR_EXPR Operand,
    IN slsNAME * FieldName,
    IN slsCOMPONENT_SELECTION * ComponentSelection,
    OUT slsDATA_TYPE * * DataType
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Operand);
    gcmASSERT(DataType);

    switch (Type)
    {
    case slvUNARY_FIELD_SELECTION:
        gcmASSERT(FieldName);

        if (FieldName == gcvNULL)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        if (FieldName->dataType == gcvNULL)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        status = sloCOMPILER_CloneDataType(
                                        Compiler,
                                        Operand->dataType->qualifier,
                                        FieldName->dataType,
                                        DataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvUNARY_COMPONENT_SELECTION:
        gcmASSERT(ComponentSelection);
		if (ComponentSelection == gcvNULL)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        gcmASSERT(slsDATA_TYPE_IsBVecOrIVecOrVec(Operand->dataType));

        status = sloCOMPILER_CloneDataType(
                                        Compiler,
                                        Operand->dataType->qualifier,
                                        Operand->dataType,
                                        DataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (ComponentSelection->components == 1)
        {
            slmDATA_TYPE_vectorSize_SET(*DataType, 0);
        }
        else
        {
            slmDATA_TYPE_vectorSize_SET(*DataType, ComponentSelection->components);
        }

        break;

    case slvUNARY_POST_INC:
    case slvUNARY_POST_DEC:
    case slvUNARY_PRE_INC:
    case slvUNARY_PRE_DEC:

    case slvUNARY_NEG:
    case slvUNARY_NOT:
        status = sloCOMPILER_CloneDataType(
                                        Compiler,
                                        slvQUALIFIER_CONST,
                                        Operand->dataType,
                                        DataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    default: gcmASSERT(0);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_Construct(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleUNARY_EXPR_TYPE Type,
    IN sloIR_EXPR Operand,
    IN slsNAME * FieldName,
    IN slsCOMPONENT_SELECTION * ComponentSelection,
    OUT sloIR_UNARY_EXPR * UnaryExpr
    )
{
    gceSTATUS           status = gcvSTATUS_OK;
    slsDATA_TYPE *      dataType;
    sloIR_UNARY_EXPR    unaryExpr;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Operand);
    gcmASSERT(UnaryExpr);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = _GetUnaryExprDataType(
                                    Compiler,
                                    Type,
                                    Operand,
                                    FieldName,
                                    ComponentSelection,
                                    &dataType);

        if (gcmIS_ERROR(status)) break;

        gcmASSERT(dataType);

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_UNARY_EXPR),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        unaryExpr = pointer;

        sloIR_EXPR_Initialize(&unaryExpr->exprBase, &s_unaryExprVTab, LineNo, StringNo, dataType);

        unaryExpr->type     = Type;
        unaryExpr->operand  = Operand;

        switch (Type)
        {
        case slvUNARY_FIELD_SELECTION:
            gcmASSERT(FieldName);
            unaryExpr->u.fieldName          = FieldName;
            break;

        case slvUNARY_COMPONENT_SELECTION:
            gcmASSERT(ComponentSelection);
            if (ComponentSelection == gcvNULL)
            {
                gcmFOOTER();
                return status;
            }

            unaryExpr->u.componentSelection = *ComponentSelection;
            break;

        default:
            break;
        }

        *UnaryExpr = unaryExpr;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *UnaryExpr = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
_NegConstantValue(
    IN sltELEMENT_TYPE Type,
    IN OUT sluCONSTANT_VALUE * Value
    )
{
    gceSTATUS   status;

    gcmHEADER();

    gcmASSERT(Value);

    switch (Type)
    {
    case slvTYPE_INT:
        Value->intValue = -Value->intValue;
        break;

    case slvTYPE_UINT:
        Value->uintValue = -((gctINT32) Value->uintValue);
        break;

    case slvTYPE_FLOAT:
        Value->floatValue = -Value->floatValue;
        break;

    default:
        gcmASSERT(0);

        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
_NotConstantValue(
    IN sltELEMENT_TYPE Type,
    IN OUT sluCONSTANT_VALUE * Value
    )
{
    gceSTATUS   status;

    gcmHEADER();

    gcmASSERT(Value);

    switch (Type)
    {
    case slvTYPE_BOOL:
        Value->boolValue = !Value->boolValue;
        break;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_Evaluate(
    IN sloCOMPILER Compiler,
    IN sleUNARY_EXPR_TYPE Type,
    IN sloIR_CONSTANT Constant,
    IN slsNAME * FieldName,
    IN slsCOMPONENT_SELECTION * ComponentSelection,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS               status;
    sltEVALUATE_FUNC_PTR    evaluate;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);

    switch (Type)
    {
    case slvUNARY_FIELD_SELECTION:
        status = _sloIR_CONSTANT_SelectField(
                                        Compiler,
                                        Constant,
                                        FieldName,
                                        ResultConstant);
        gcmFOOTER();
        return status;

    case slvUNARY_COMPONENT_SELECTION:
        status = _sloIR_CONSTANT_SelectComponents(
                                                Compiler,
                                                Constant,
                                                ComponentSelection,
                                                ResultConstant);
        gcmFOOTER();
        return status;

    case slvUNARY_NEG:
        evaluate = &_NegConstantValue;
        break;

    case slvUNARY_NOT:
        evaluate = &_NotConstantValue;
        break;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    do
    {
        status = sloIR_CONSTANT_Evaluate(
                                        Compiler,
                                        Constant,
                                        evaluate);

        if (gcmIS_ERROR(status)) break;

        *ResultConstant = Constant;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *ResultConstant = gcvNULL;

    gcmFOOTER();
    return status;
}

/* sloIR_BINARY_EXPR object. */
gceSTATUS
sloIR_BINARY_EXPR_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_BINARY_EXPR   binaryExpr = (sloIR_BINARY_EXPR)This;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(binaryExpr, slvIR_BINARY_EXPR);

    gcmASSERT(binaryExpr->leftOperand);
    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &binaryExpr->leftOperand->base));

    gcmASSERT(binaryExpr->rightOperand);
    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &binaryExpr->rightOperand->base));

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, binaryExpr));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctCONST_STRING
_GetIRBinaryExprTypeName(
    IN sleBINARY_EXPR_TYPE BinaryExprType
    )
{
    switch (BinaryExprType)
    {
    case slvBINARY_SUBSCRIPT:           return "subscript";

    case slvBINARY_ADD:                 return "add";
    case slvBINARY_SUB:                 return "sub";
    case slvBINARY_MUL:                 return "mul";
    case slvBINARY_DIV:                 return "div";

    case slvBINARY_GREATER_THAN:        return "greater_than";
    case slvBINARY_LESS_THAN:           return "less_than";
    case slvBINARY_GREATER_THAN_EQUAL:  return "greater_than_equal";
    case slvBINARY_LESS_THAN_EQUAL:     return "less_than_equal";

    case slvBINARY_EQUAL:               return "equal";
    case slvBINARY_NOT_EQUAL:           return "not_equal";

    case slvBINARY_AND:                 return "and";
    case slvBINARY_OR:                  return "or";
    case slvBINARY_XOR:                 return "xor";

    case slvBINARY_SEQUENCE:            return "sequence";

    case slvBINARY_ASSIGN:              return "assign";

    case slvBINARY_MUL_ASSIGN:          return "mul_assign";
    case slvBINARY_DIV_ASSIGN:          return "div_assign";
    case slvBINARY_ADD_ASSIGN:          return "add_assign";
    case slvBINARY_SUB_ASSIGN:          return "sub_assign";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
sloIR_BINARY_EXPR_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_BINARY_EXPR   binaryExpr = (sloIR_BINARY_EXPR)This;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(binaryExpr, slvIR_BINARY_EXPR);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_BINARY_EXPR line=\"%d\" string=\"%d\""
                                " dataType=\"0x%x\" type=\"%s\">",
                                binaryExpr->exprBase.base.lineNo,
                                binaryExpr->exprBase.base.stringNo,
                                binaryExpr->exprBase.dataType,
                                _GetIRBinaryExprTypeName(binaryExpr->type)));

    gcmASSERT(binaryExpr->leftOperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<!-- Left Operand -->"));

    gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &binaryExpr->leftOperand->base));

    gcmASSERT(binaryExpr->rightOperand);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<!-- Right Operand -->"));

    gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &binaryExpr->rightOperand->base));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_BINARY_EXPR>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    sloIR_BINARY_EXPR   binaryExpr = (sloIR_BINARY_EXPR)This;
    gceSTATUS           status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(binaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Visitor);

    if (Visitor->visitBinaryExpr == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = Visitor->visitBinaryExpr(Compiler, Visitor, binaryExpr, Parameters);

    gcmFOOTER();
    return status;
}

static slsVTAB s_binaryExprVTab =
{
    slvIR_BINARY_EXPR,
    sloIR_BINARY_EXPR_Destroy,
    sloIR_BINARY_EXPR_Dump,
    sloIR_BINARY_EXPR_Accept
};

static gceSTATUS
_GetArithmeticExprDataType(
    IN sloCOMPILER Compiler,
    IN gctBOOL IsMul,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand,
    OUT slsDATA_TYPE * * DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  exprDataType;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);
    gcmASSERT(DataType);

    if (slsDATA_TYPE_IsEqual(LeftOperand->dataType, RightOperand->dataType))
    {
        exprDataType = LeftOperand->dataType;
    }
    else
    {
        if (slsDATA_TYPE_IsInt(LeftOperand->dataType))
        {
            gcmASSERT(slsDATA_TYPE_IsIVec(RightOperand->dataType));

            exprDataType = RightOperand->dataType;
        }
        else if (slsDATA_TYPE_IsIVec(LeftOperand->dataType))
        {
            gcmASSERT(slsDATA_TYPE_IsInt(RightOperand->dataType));

            exprDataType = LeftOperand->dataType;
        }
        else if (slsDATA_TYPE_IsFloat(LeftOperand->dataType))
        {
            gcmASSERT(slsDATA_TYPE_IsVecOrMat(RightOperand->dataType));

            exprDataType = RightOperand->dataType;
        }
        else if (slsDATA_TYPE_IsVec(LeftOperand->dataType))
        {
            if (!IsMul)
            {
                gcmASSERT(slsDATA_TYPE_IsFloat(RightOperand->dataType));

                exprDataType = LeftOperand->dataType;
            }
            else
            {
                if (slsDATA_TYPE_IsFloat(RightOperand->dataType))
                {
                    exprDataType = LeftOperand->dataType;
                }
                else
                {
                    gcmASSERT(slsDATA_TYPE_IsMat(RightOperand->dataType)
                        && slmDATA_TYPE_vectorSize_GET(LeftOperand->dataType)
                        == slmDATA_TYPE_matrixRowCount_GET(RightOperand->dataType));

                    exprDataType = LeftOperand->dataType;
                }
            }
        }
        else if (slsDATA_TYPE_IsMat(LeftOperand->dataType))
        {
            if (!IsMul)
            {
                gcmASSERT(slsDATA_TYPE_IsFloat(RightOperand->dataType));

                exprDataType = LeftOperand->dataType;
            }
            else
            {
                if (slsDATA_TYPE_IsFloat(RightOperand->dataType))
                {
                    exprDataType = LeftOperand->dataType;
                }
                else
                {
                    gcmASSERT(slsDATA_TYPE_IsVec(RightOperand->dataType)
                        && slmDATA_TYPE_matrixColumnCount_GET(LeftOperand->dataType)
                        == slmDATA_TYPE_vectorSize_GET(RightOperand->dataType));

                    exprDataType = RightOperand->dataType;
                }
            }
        }
        else
        {
            gcmASSERT(0);

            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
    }

    status = sloCOMPILER_CloneDataType(
                                    Compiler,
                                    slvQUALIFIER_CONST,
                                    exprDataType,
                                    DataType);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GetBinaryExprDataType(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE Type,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand,
    OUT slsDATA_TYPE * * DataType
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);
    gcmASSERT(DataType);

    switch (Type)
    {
    case slvBINARY_SUBSCRIPT:
        status = sloCOMPILER_CreateElementDataType(
                                                Compiler,
                                                LeftOperand->dataType,
                                                DataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvBINARY_ADD:
    case slvBINARY_SUB:
    case slvBINARY_MUL:
    case slvBINARY_DIV:
        status = _GetArithmeticExprDataType(
                                            Compiler,
                                            (Type == slvBINARY_MUL),
                                            LeftOperand,
                                            RightOperand,
                                            DataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvBINARY_GREATER_THAN:
    case slvBINARY_LESS_THAN:
    case slvBINARY_GREATER_THAN_EQUAL:
    case slvBINARY_LESS_THAN_EQUAL:

    case slvBINARY_EQUAL:
    case slvBINARY_NOT_EQUAL:

    case slvBINARY_AND:
    case slvBINARY_OR:
    case slvBINARY_XOR:
        status = sloCOMPILER_CreateDataType(
                                            Compiler,
                                            T_BOOL,
                                            gcvNULL,
                                            DataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvBINARY_SEQUENCE:
        status = sloCOMPILER_CloneDataType(
                                        Compiler,
                                        slvQUALIFIER_CONST,
                                        RightOperand->dataType,
                                        DataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvBINARY_ASSIGN:

    case slvBINARY_MUL_ASSIGN:
    case slvBINARY_DIV_ASSIGN:
    case slvBINARY_ADD_ASSIGN:
    case slvBINARY_SUB_ASSIGN:
        status = sloCOMPILER_CloneDataType(
                                        Compiler,
                                        slvQUALIFIER_CONST,
                                        LeftOperand->dataType,
                                        DataType);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    default: gcmASSERT(0);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_BINARY_EXPR_Construct(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleBINARY_EXPR_TYPE Type,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand,
    OUT sloIR_BINARY_EXPR * BinaryExpr
    )
{
    gceSTATUS           status;
    slsDATA_TYPE *      dataType;
    sloIR_BINARY_EXPR   binaryExpr;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(LeftOperand);
    gcmASSERT(RightOperand);
    gcmASSERT(BinaryExpr);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = _GetBinaryExprDataType(
                                        Compiler,
                                        Type,
                                        LeftOperand,
                                        RightOperand,
                                        &dataType);

        if (gcmIS_ERROR(status)) break;

        gcmASSERT(dataType);

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_BINARY_EXPR),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        binaryExpr = pointer;

        sloIR_EXPR_Initialize(&binaryExpr->exprBase, &s_binaryExprVTab, LineNo, StringNo, dataType);

        binaryExpr->type            = Type;
        binaryExpr->leftOperand     = LeftOperand;
        binaryExpr->rightOperand    = RightOperand;
        binaryExpr->u.vec2Array     = gcvNULL;

        *BinaryExpr = binaryExpr;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *BinaryExpr = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_BINARY_EXPR_Evaluate(
    IN sloCOMPILER Compiler,
    IN sleBINARY_EXPR_TYPE Type,
    IN sloIR_CONSTANT LeftConstant,
    IN sloIR_CONSTANT RightConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(LeftConstant, slvIR_CONSTANT);
    slmVERIFY_IR_OBJECT(RightConstant, slvIR_CONSTANT);

    switch (Type)
    {
    case slvBINARY_SUBSCRIPT:
        status = _sloIR_CONSTANT_Subscript(
                                        Compiler,
                                        LeftConstant,
                                        RightConstant,
                                        ResultConstant);
        gcmFOOTER();
        return status;

    case slvBINARY_ADD:
    case slvBINARY_SUB:
    case slvBINARY_MUL:
    case slvBINARY_DIV:
        status = _sloIR_CONSTANT_ArithmeticOperate(
                                                Compiler,
                                                Type,
                                                LeftConstant,
                                                RightConstant,
                                                ResultConstant);
        gcmFOOTER();
        return status;

    case slvBINARY_GREATER_THAN:
    case slvBINARY_LESS_THAN:
    case slvBINARY_GREATER_THAN_EQUAL:
    case slvBINARY_LESS_THAN_EQUAL:
        status = _sloIR_CONSTANT_RelationalOperate(
                                                Compiler,
                                                Type,
                                                LeftConstant,
                                                RightConstant,
                                                ResultConstant);
        gcmFOOTER();
        return status;

    case slvBINARY_EQUAL:
    case slvBINARY_NOT_EQUAL:
        status = _sloIR_CONSTANT_EqualityOperate(
                                            Compiler,
                                            Type,
                                            LeftConstant,
                                            RightConstant,
                                            ResultConstant);
        gcmFOOTER();
        return status;

    case slvBINARY_AND:
    case slvBINARY_OR:
    case slvBINARY_XOR:
        status = _sloIR_CONSTANT_LogicalOperate(
                                            Compiler,
                                            Type,
                                            LeftConstant,
                                            RightConstant,
                                            ResultConstant);
        gcmFOOTER();
        return status;

    case slvBINARY_SEQUENCE:
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &LeftConstant->exprBase.base));

        *ResultConstant = RightConstant;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    default:
        gcmASSERT(0);
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }
}

/* sloIR_SELECTION object. */
gceSTATUS
sloIR_SELECTION_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_SELECTION selection = (sloIR_SELECTION)This;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(selection, slvIR_SELECTION);

    gcmASSERT(selection->condExpr);
    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &selection->condExpr->base));

    if (selection->trueOperand != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, selection->trueOperand));
    }

    if (selection->falseOperand != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, selection->falseOperand));
    }

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, selection));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_SELECTION_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_SELECTION selection = (sloIR_SELECTION)This;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(selection, slvIR_SELECTION);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_SELECTION line=\"%d\" string=\"%d\" dataType=\"0x%x\">",
                                selection->exprBase.base.lineNo,
                                selection->exprBase.base.stringNo,
                                selection->exprBase.dataType));

    gcmASSERT(selection->condExpr);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<!-- Condition Expression -->"));

    gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &selection->condExpr->base));

    if (selection->trueOperand != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- True Operand -->"));

        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, selection->trueOperand));
    }

    if (selection->falseOperand != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- False Operand -->"));

        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, selection->falseOperand));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_SELECTION>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_SELECTION_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    sloIR_SELECTION selection = (sloIR_SELECTION)This;
    gceSTATUS       status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(selection, slvIR_SELECTION);
    gcmASSERT(Visitor);

    if (Visitor->visitSelection == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = Visitor->visitSelection(Compiler, Visitor, selection, Parameters);

    gcmFOOTER();
    return status;
}

static slsVTAB s_selectionVTab =
{
    slvIR_SELECTION,
    sloIR_SELECTION_Destroy,
    sloIR_SELECTION_Dump,
    sloIR_SELECTION_Accept
};

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
    )
{
    gceSTATUS           status;
    sloIR_SELECTION     selection;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(CondExpr);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_SELECTION),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        selection = pointer;

        sloIR_EXPR_Initialize(&selection->exprBase, &s_selectionVTab, LineNo, StringNo, DataType);

        selection->condExpr     = CondExpr;
        selection->trueOperand  = TrueOperand;
        selection->falseOperand = FalseOperand;

        *Selection = selection;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *Selection = gcvNULL;

    gcmFOOTER();
    return status;
}

/* sloIR_POLYNARY_EXPR object. */
gceSTATUS
sloIR_POLYNARY_EXPR_Destroy(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_POLYNARY_EXPR polynaryExpr = (sloIR_POLYNARY_EXPR)This;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(polynaryExpr, slvIR_POLYNARY_EXPR);

    if (polynaryExpr->operands != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &polynaryExpr->operands->base));
    }

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, polynaryExpr));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gctCONST_STRING
slGetIRPolynaryExprTypeName(
    IN slePOLYNARY_EXPR_TYPE PolynaryExprType
    )
{
    switch (PolynaryExprType)
    {
    case slvPOLYNARY_CONSTRUCT_FLOAT:   return "construct_float";
    case slvPOLYNARY_CONSTRUCT_INT:     return "construct_int";
    case slvPOLYNARY_CONSTRUCT_UINT:     return "construct_unsigned_int";
    case slvPOLYNARY_CONSTRUCT_BOOL:    return "construct_bool";
    case slvPOLYNARY_CONSTRUCT_VEC2:    return "construct_vec2";
    case slvPOLYNARY_CONSTRUCT_VEC3:    return "construct_vec3";
    case slvPOLYNARY_CONSTRUCT_VEC4:    return "construct_vec4";
    case slvPOLYNARY_CONSTRUCT_BVEC2:   return "construct_bvec2";
    case slvPOLYNARY_CONSTRUCT_BVEC3:   return "construct_bvec3";
    case slvPOLYNARY_CONSTRUCT_BVEC4:   return "construct_bvec4";
    case slvPOLYNARY_CONSTRUCT_IVEC2:   return "construct_ivec2";
    case slvPOLYNARY_CONSTRUCT_IVEC3:   return "construct_ivec3";
    case slvPOLYNARY_CONSTRUCT_IVEC4:   return "construct_ivec4";
    case slvPOLYNARY_CONSTRUCT_UVEC2:   return "construct_uvec2";
    case slvPOLYNARY_CONSTRUCT_UVEC3:   return "construct_uvec3";
    case slvPOLYNARY_CONSTRUCT_UVEC4:   return "construct_uvec4";
    case slvPOLYNARY_CONSTRUCT_MAT2:    return "construct_mat2";
    case slvPOLYNARY_CONSTRUCT_MAT2X3:  return "construct_mat2x3";
    case slvPOLYNARY_CONSTRUCT_MAT2X4:  return "construct_mat2x4";
    case slvPOLYNARY_CONSTRUCT_MAT3:    return "construct_mat3";
    case slvPOLYNARY_CONSTRUCT_MAT3X2:  return "construct_mat3x2";
    case slvPOLYNARY_CONSTRUCT_MAT3X4:  return "construct_mat3x4";
    case slvPOLYNARY_CONSTRUCT_MAT4:    return "construct_mat4";
    case slvPOLYNARY_CONSTRUCT_MAT4X2:  return "construct_mat4x2";
    case slvPOLYNARY_CONSTRUCT_MAT4X3:  return "construct_mat4x3";
    case slvPOLYNARY_CONSTRUCT_STRUCT:  return "construct_struct";

    case slvPOLYNARY_FUNC_CALL:         return "function_call";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

gceSTATUS
sloIR_POLYNARY_EXPR_Dump(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This
    )
{
    sloIR_POLYNARY_EXPR polynaryExpr = (sloIR_POLYNARY_EXPR)This;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(polynaryExpr, slvIR_POLYNARY_EXPR);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR_POLYNARY_EXPR line=\"%d\" string=\"%d\""
                                " dataType=\"0x%x\" type=\"%s\"",
                                polynaryExpr->exprBase.base.lineNo,
                                polynaryExpr->exprBase.base.stringNo,
                                polynaryExpr->exprBase.dataType,
                                slGetIRPolynaryExprTypeName(polynaryExpr->type)));

    if (polynaryExpr->type == slvPOLYNARY_FUNC_CALL)
    {
        gcmASSERT(polynaryExpr->funcName);

        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    " funcSymbol=\"%s\">",
                                    polynaryExpr->funcSymbol));
    }
    else
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    ">"));
    }

    if (polynaryExpr->funcName != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- Function Name -->"));

        gcmVERIFY_OK(slsNAME_Dump(Compiler, polynaryExpr->funcName));
    }

    if (polynaryExpr->operands != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Dump(
                                    Compiler,
                                    slvDUMP_IR,
                                    "<!-- Operands -->"));

        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &polynaryExpr->operands->base));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR_POLYNARY_EXPR>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_Accept(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE This,
    IN slsVISITOR * Visitor,
    IN OUT gctPOINTER Parameters
    )
{
    sloIR_POLYNARY_EXPR polynaryExpr = (sloIR_POLYNARY_EXPR)This;
    gceSTATUS           status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(polynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Visitor);

    if (Visitor->visitPolynaryExpr == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    status = Visitor->visitPolynaryExpr(Compiler, Visitor, polynaryExpr, Parameters);

    gcmFOOTER();
    return status;
}

static slsVTAB s_polynaryExprVTab =
{
    slvIR_POLYNARY_EXPR,
    sloIR_POLYNARY_EXPR_Destroy,
    sloIR_POLYNARY_EXPR_Dump,
    sloIR_POLYNARY_EXPR_Accept
};

gceSTATUS
sloIR_POLYNARY_EXPR_Construct(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slePOLYNARY_EXPR_TYPE Type,
    IN slsDATA_TYPE * DataType,
    IN sltPOOL_STRING FuncSymbol,
    OUT sloIR_POLYNARY_EXPR * PolynaryExpr
    )
{
    gceSTATUS           status;
    sloIR_POLYNARY_EXPR polynaryExpr;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(struct _sloIR_POLYNARY_EXPR),
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        polynaryExpr = pointer;

        sloIR_EXPR_Initialize(&polynaryExpr->exprBase, &s_polynaryExprVTab, LineNo, StringNo, DataType);

        polynaryExpr->type          = Type;
        polynaryExpr->funcSymbol    = FuncSymbol;
        polynaryExpr->funcName      = gcvNULL;
        polynaryExpr->operands      = gcvNULL;

        *PolynaryExpr = polynaryExpr;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *PolynaryExpr = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_POLYNARY_EXPR_ConstructScalarConstant(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS           status;
    sloIR_CONSTANT      resultConstant, operandConstant;
    sloIR_EXPR          operand;
    sluCONSTANT_VALUE   value;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(ResultConstant);

    gcmASSERT(PolynaryExpr->operands);

    operand = slsDLINK_LIST_First(&PolynaryExpr->operands->members, struct _sloIR_EXPR);
    gcmASSERT(operand);

    /*init value*/
    value.floatValue = (gctFLOAT)0.0;

    do
    {
        /* Check if the operand is constant */
        if (sloIR_OBJECT_GetType(&operand->base) != slvIR_CONSTANT)
            break;

        operandConstant = (sloIR_CONSTANT)operand;

        /* Create the constant */
        PolynaryExpr->exprBase.dataType->qualifier = slvQUALIFIER_CONST;

        status = sloIR_CONSTANT_Construct(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        PolynaryExpr->exprBase.dataType,
                                        &resultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Get the converted constant value */
        switch (PolynaryExpr->exprBase.dataType->elementType)
        {
        case slvTYPE_BOOL:
            gcmVERIFY_OK(sloIR_CONSTANT_GetBoolValue(
                                                    Compiler,
                                                    operandConstant,
                                                    0,
                                                    &value));
            break;

        case slvTYPE_INT:
            gcmVERIFY_OK(sloIR_CONSTANT_GetIntValue(
                                                    Compiler,
                                                    operandConstant,
                                                    0,
                                                    &value));
            break;

        case slvTYPE_UINT:
            gcmVERIFY_OK(sloIR_CONSTANT_GetUIntValue(
                                                    Compiler,
                                                    operandConstant,
                                                    0,
                                                    &value));
            break;

        case slvTYPE_FLOAT:
            gcmVERIFY_OK(sloIR_CONSTANT_GetFloatValue(
                                                    Compiler,
                                                    operandConstant,
                                                    0,
                                                    &value));
            break;

        default:
            gcmASSERT(0);

        }

        /* Add the constant value */
        status = sloIR_CONSTANT_AddValues(
                                        Compiler,
                                        resultConstant,
                                        1,
                                        &value);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &PolynaryExpr->exprBase.base));

        *ResultConstant = resultConstant;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *ResultConstant = gcvNULL;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctBOOL
_AreAllOperandsConstant(
    IN sloIR_POLYNARY_EXPR PolynaryExpr
    )
{
    sloIR_EXPR          operand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(PolynaryExpr->operands);

    FOR_EACH_DLINK_NODE(&PolynaryExpr->operands->members, struct _sloIR_EXPR, operand)
    {
        if (sloIR_OBJECT_GetType(&operand->base) != slvIR_CONSTANT)
        {
            gcmFOOTER_NO();
            return gcvFALSE;
        }
    }

    gcmFOOTER_NO();
    return gcvTRUE;
}

static gceSTATUS
_SetVectorConstantValuesByOneScalarValue(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN sloIR_CONSTANT OperandConstant,
    IN OUT sloIR_CONSTANT ResultConstant
    )
{
    gceSTATUS           status;
    gctUINT             i;
    sluCONSTANT_VALUE   value;

    gcmHEADER_ARG("Compiler=0x%x PolynaryExpr=0x%x OperandConstant=0x%x ResultConstant=0x%x",
                  Compiler, PolynaryExpr, OperandConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);

    /* init value */
    value.floatValue = (gctFLOAT)0.0;

    /* Get the converted constant value */
    switch (ResultConstant->exprBase.dataType->elementType)
    {
    case slvTYPE_BOOL:
        gcmVERIFY_OK(sloIR_CONSTANT_GetBoolValue(
                                                Compiler,
                                                OperandConstant,
                                                0,
                                                &value));
        break;

    case slvTYPE_INT:
        gcmVERIFY_OK(sloIR_CONSTANT_GetIntValue(
                                                Compiler,
                                                OperandConstant,
                                                0,
                                                &value));
        break;

    case slvTYPE_UINT:
        gcmVERIFY_OK(sloIR_CONSTANT_GetUIntValue(
                                                Compiler,
                                                OperandConstant,
                                                0,
                                                &value));
        break;

    case slvTYPE_FLOAT:
        gcmVERIFY_OK(sloIR_CONSTANT_GetFloatValue(
                                                Compiler,
                                                OperandConstant,
                                                0,
                                                &value));
        break;

    default:
        gcmASSERT(0);
    }

    for (i = 0; i < slsDATA_TYPE_GetSize(ResultConstant->exprBase.dataType); i++)
    {
        /* Add the constant value */
        status = sloIR_CONSTANT_AddValues(
                                        Compiler,
                                        ResultConstant,
                                        1,
                                        &value);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }
    }

    gcmFOOTER_ARG("ResultConstant=0x%x", ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_SetMatrixConstantValuesByOneScalarValue(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN sloIR_CONSTANT OperandConstant,
    IN OUT sloIR_CONSTANT ResultConstant
    )
{
    gceSTATUS           status;
    gctUINT             matrixColumnCount, i, j;
    sluCONSTANT_VALUE   value, valueZero;

    gcmHEADER_ARG("Compiler=0x%x PolynaryExpr=0x%x OperandConstant=0x%x ResultConstant=0x%x",
                  Compiler, PolynaryExpr, OperandConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);

    /* init value */
    value.floatValue = (gctFLOAT)0.0;

    /* Get the converted constant value */
    switch (ResultConstant->exprBase.dataType->elementType)
    {
    case slvTYPE_BOOL:
        gcmVERIFY_OK(sloIR_CONSTANT_GetBoolValue(
                                                Compiler,
                                                OperandConstant,
                                                0,
                                                &value));
        break;

    case slvTYPE_INT:
        gcmVERIFY_OK(sloIR_CONSTANT_GetIntValue(
                                                Compiler,
                                                OperandConstant,
                                                0,
                                                &value));
        break;

    case slvTYPE_UINT:
        gcmVERIFY_OK(sloIR_CONSTANT_GetUIntValue(
                                                Compiler,
                                                OperandConstant,
                                                0,
                                                &value));
        break;

    case slvTYPE_FLOAT:
        gcmVERIFY_OK(sloIR_CONSTANT_GetFloatValue(
                                                Compiler,
                                                OperandConstant,
                                                0,
                                                &value));
        break;

    default:
        gcmASSERT(0);
    }

    valueZero.floatValue = (gctFLOAT)0.0;

    gcmASSERT(slmDATA_TYPE_IsSquareMat(ResultConstant->exprBase.dataType));
    matrixColumnCount = slmDATA_TYPE_matrixColumnCount_GET(ResultConstant->exprBase.dataType);
    gcmASSERT(2 <= matrixColumnCount && matrixColumnCount <= 4);

    for (i = 0; i < matrixColumnCount; i++)
    {
        for (j = 0; j < matrixColumnCount; j++)
        {
            if (i == j)
            {
                /* Add the specified constant value */
                status = sloIR_CONSTANT_AddValues(
                                                Compiler,
                                                ResultConstant,
                                                1,
                                                &value);

                if (gcmIS_ERROR(status))
                {
                    gcmFOOTER();
                    return status;
                }
            }
            else
            {
                /* Add the zero constant value */
                status = sloIR_CONSTANT_AddValues(
                                                Compiler,
                                                ResultConstant,
                                                1,
                                                &valueZero);

                if (gcmIS_ERROR(status))
                {
                    gcmFOOTER();
                    return status;
                }
            }
        }
    }

    gcmFOOTER_ARG("ResultConstant=0x%x", ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_SetMatrixConstantValuesByOneMatrixValue(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN sloIR_CONSTANT OperandConstant,
    IN OUT sloIR_CONSTANT ResultConstant
    )
{
    gceSTATUS           status;
    gctUINT             matrixColumnCount, operandMatrixColumnCount, i, j;
    sluCONSTANT_VALUE   value, valueZero, valueOne;

    gcmHEADER_ARG("Compiler=0x%x PolynaryExpr=0x%x OperandConstant=0x%x ResultConstant=0x%x",
                  Compiler, PolynaryExpr, OperandConstant, ResultConstant);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);

    valueZero.floatValue    = (gctFLOAT)0.0;
    valueOne.floatValue     = (gctFLOAT)1.0;

    /* init value */
    value.floatValue        = (gctFLOAT)0.0;

    matrixColumnCount = slmDATA_TYPE_matrixColumnCount_GET(ResultConstant->exprBase.dataType);
    gcmASSERT(2 <= matrixColumnCount && matrixColumnCount <= 4);

    operandMatrixColumnCount = slmDATA_TYPE_matrixColumnCount_GET(OperandConstant->exprBase.dataType);
    gcmASSERT(2 <= operandMatrixColumnCount && operandMatrixColumnCount <= 4);

    for (i = 0; i < matrixColumnCount; i++)
    {
        for (j = 0; j < matrixColumnCount; j++)
        {
            if (i < operandMatrixColumnCount && j < operandMatrixColumnCount)
            {
                /* Get the converted constant value */
                switch (ResultConstant->exprBase.dataType->elementType)
                {
                case slvTYPE_BOOL:
                    gcmVERIFY_OK(sloIR_CONSTANT_GetBoolValue(
                                                            Compiler,
                                                            OperandConstant,
                                                            i * operandMatrixColumnCount + j,
                                                            &value));
                    break;

                case slvTYPE_INT:
                    gcmVERIFY_OK(sloIR_CONSTANT_GetIntValue(
                                                            Compiler,
                                                            OperandConstant,
                                                            i * operandMatrixColumnCount + j,
                                                            &value));
                    break;

                case slvTYPE_UINT:
                    gcmVERIFY_OK(sloIR_CONSTANT_GetUIntValue(
                                                            Compiler,
                                                            OperandConstant,
                                                            i * operandMatrixColumnCount + j,
                                                            &value));
                    break;

                case slvTYPE_FLOAT:
                    gcmVERIFY_OK(sloIR_CONSTANT_GetFloatValue(
                                                            Compiler,
                                                            OperandConstant,
                                                            i * operandMatrixColumnCount + j,
                                                            &value));
                    break;

                default:
                    gcmASSERT(0);
                }

                /* Add the specified constant value */
                status = sloIR_CONSTANT_AddValues(
                                                Compiler,
                                                ResultConstant,
                                                1,
                                                &value);

                if (gcmIS_ERROR(status))
                {
                    gcmFOOTER();
                    return status;
                }
            }
            else if (i == j)
            {
                /* Add the one constant value */
                status = sloIR_CONSTANT_AddValues(
                                                Compiler,
                                                ResultConstant,
                                                1,
                                                &valueOne);

                if (gcmIS_ERROR(status))
                {
                    gcmFOOTER();
                    return status;
                }
            }
            else
            {
                /* Add the zero constant value */
                status = sloIR_CONSTANT_AddValues(
                                                Compiler,
                                                ResultConstant,
                                                1,
                                                &valueZero);

                if (gcmIS_ERROR(status))
                {
                    gcmFOOTER();
                    return status;
                }
            }
        }
    }

    gcmFOOTER_ARG("ResultConstant=0x%x", ResultConstant);
    return gcvSTATUS_OK;
}

static gceSTATUS
_SetVectorOrMatrixConstantValues(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctBOOL IsVectorConstant,
    IN OUT sloIR_CONSTANT ResultConstant
    )
{
    gceSTATUS           status;
    gctUINT             operandCount, i, valueCount = 0;
    sloIR_EXPR          operand;
    sloIR_CONSTANT      operandConstant;
    sluCONSTANT_VALUE   value;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(PolynaryExpr->operands);

    gcmVERIFY_OK(sloIR_SET_GetMemberCount(
                                        Compiler,
                                        PolynaryExpr->operands,
                                        &operandCount));

    /* init value */
    value.floatValue = (gctFLOAT)0.0;

    if (operandCount == 1)
    {
        operand = slsDLINK_LIST_First(&PolynaryExpr->operands->members, struct _sloIR_EXPR);
        gcmASSERT(operand);

        gcmASSERT(sloIR_OBJECT_GetType(&operand->base) == slvIR_CONSTANT);
        operandConstant = (sloIR_CONSTANT)operand;

        if (IsVectorConstant)
        {
            status = _SetVectorConstantValuesByOneScalarValue(
                                                            Compiler,
                                                            PolynaryExpr,
                                                            operandConstant,
                                                            ResultConstant);
        }
        else if (slsDATA_TYPE_IsScalar(operandConstant->exprBase.dataType))
        {
            status = _SetMatrixConstantValuesByOneScalarValue(
                                                            Compiler,
                                                            PolynaryExpr,
                                                            operandConstant,
                                                            ResultConstant);
        }
        else
        {
            gcmASSERT(slsDATA_TYPE_IsMat(operandConstant->exprBase.dataType));

            status = _SetMatrixConstantValuesByOneMatrixValue(
                                                            Compiler,
                                                            PolynaryExpr,
                                                            operandConstant,
                                                            ResultConstant);
        }

        gcmFOOTER();
        return status;
    }

    FOR_EACH_DLINK_NODE(&PolynaryExpr->operands->members, struct _sloIR_EXPR, operand)
    {
        gcmASSERT(sloIR_OBJECT_GetType(&operand->base) == slvIR_CONSTANT);
        operandConstant = (sloIR_CONSTANT)operand;

        for (i = 0; i < slsDATA_TYPE_GetSize(operandConstant->exprBase.dataType); i++)
        {
            /* Get the converted constant value */
            switch (ResultConstant->exprBase.dataType->elementType)
            {
            case slvTYPE_BOOL:
                gcmVERIFY_OK(sloIR_CONSTANT_GetBoolValue(
                                                        Compiler,
                                                        operandConstant,
                                                        i,
                                                        &value));
                break;

            case slvTYPE_INT:
                gcmVERIFY_OK(sloIR_CONSTANT_GetIntValue(
                                                        Compiler,
                                                        operandConstant,
                                                        i,
                                                        &value));
                break;

            case slvTYPE_UINT:
                gcmVERIFY_OK(sloIR_CONSTANT_GetUIntValue(
                                                        Compiler,
                                                        operandConstant,
                                                        i,
                                                        &value));
                break;

            case slvTYPE_FLOAT:
                gcmVERIFY_OK(sloIR_CONSTANT_GetFloatValue(
                                                        Compiler,
                                                        operandConstant,
                                                        i,
                                                        &value));
                break;

            default:
                gcmASSERT(0);
            }

            /* Add the constant value */
            status = sloIR_CONSTANT_AddValues(
                                            Compiler,
                                            ResultConstant,
                                            1,
                                            &value);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

            /* Move to the next component */
            valueCount++;
            if (valueCount == slsDATA_TYPE_GetSize(ResultConstant->exprBase.dataType))
            {
                goto Exit;
            }
        }
    }

    gcmASSERT(0);

Exit:
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_ConstructVectorOrMatrixConstant(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctBOOL IsVectorConstant,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS           status;
    sloIR_CONSTANT      resultConstant;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(ResultConstant);

    gcmASSERT(PolynaryExpr->operands);

    do
    {
        /* Check if all operands are constant */
        if (!_AreAllOperandsConstant(PolynaryExpr)) break;

        /* Create the constant */
        PolynaryExpr->exprBase.dataType->qualifier = slvQUALIFIER_CONST;

        status = sloIR_CONSTANT_Construct(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        PolynaryExpr->exprBase.dataType,
                                        &resultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Convert all operand constant values */
        status = _SetVectorOrMatrixConstantValues(
                                                Compiler,
                                                PolynaryExpr,
                                                IsVectorConstant,
                                                resultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &PolynaryExpr->exprBase.base));

        *ResultConstant = resultConstant;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *ResultConstant = gcvNULL;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_ConstructStructConstant(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS           status;
    sloIR_CONSTANT      resultConstant;
    sloIR_EXPR          operand;
    sloIR_CONSTANT      operandConstant;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(ResultConstant);

    gcmASSERT(PolynaryExpr->operands);

    do
    {
        /* Check if all operands are constant */
        if (!_AreAllOperandsConstant(PolynaryExpr)) break;

        /* Create the constant */
        PolynaryExpr->exprBase.dataType->qualifier = slvQUALIFIER_CONST;

        status = sloIR_CONSTANT_Construct(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        PolynaryExpr->exprBase.dataType,
                                        &resultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* Set all operand constant values */
        FOR_EACH_DLINK_NODE(&PolynaryExpr->operands->members, struct _sloIR_EXPR, operand)
        {
            gcmASSERT(sloIR_OBJECT_GetType(&operand->base) == slvIR_CONSTANT);
            operandConstant = (sloIR_CONSTANT)operand;

            /* Add the constant value */
            status = sloIR_CONSTANT_AddValues(
                                            Compiler,
                                            resultConstant,
                                            operandConstant->valueCount,
                                            operandConstant->values);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &PolynaryExpr->exprBase.base));

        *ResultConstant = resultConstant;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *ResultConstant = gcvNULL;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_EvaluateBuiltIn(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS           status;
    sloIR_EXPR          operand;
    gctUINT             i;
    sloIR_CONSTANT      operandConstants[slmMAX_BUILT_IN_PARAMETER_COUNT];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(ResultConstant);

    gcmASSERT(PolynaryExpr->operands);

    do
    {
        /* Check if all operands are constant */
        if (!_AreAllOperandsConstant(PolynaryExpr)) break;

        /* Collect all operand constants */
        i = 0;
        FOR_EACH_DLINK_NODE(&PolynaryExpr->operands->members, struct _sloIR_EXPR, operand)
        {
            gcmASSERT(sloIR_OBJECT_GetType(&operand->base) == slvIR_CONSTANT);
            operandConstants[i] = (sloIR_CONSTANT)operand;

            i++;
        }

        gcmASSERT(i <= slmMAX_BUILT_IN_PARAMETER_COUNT);

        /* Try to evaluate the built-in */
        status = slEvaluateBuiltInFunction(
                                        Compiler,
                                        PolynaryExpr,
                                        i,
                                        operandConstants,
                                        ResultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (*ResultConstant != gcvNULL)
        {
            gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &PolynaryExpr->exprBase.base));
        }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    *ResultConstant = gcvNULL;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_Evaluate(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS           status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(ResultConstant);

    switch (PolynaryExpr->type)
    {
    case slvPOLYNARY_CONSTRUCT_FLOAT:
    case slvPOLYNARY_CONSTRUCT_INT:
    case slvPOLYNARY_CONSTRUCT_UINT:
    case slvPOLYNARY_CONSTRUCT_BOOL:
        status = sloIR_POLYNARY_EXPR_ConstructScalarConstant(
                                                            Compiler,
                                                            PolynaryExpr,
                                                            ResultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvPOLYNARY_CONSTRUCT_VEC2:
    case slvPOLYNARY_CONSTRUCT_VEC3:
    case slvPOLYNARY_CONSTRUCT_VEC4:
    case slvPOLYNARY_CONSTRUCT_BVEC2:
    case slvPOLYNARY_CONSTRUCT_BVEC3:
    case slvPOLYNARY_CONSTRUCT_BVEC4:
    case slvPOLYNARY_CONSTRUCT_IVEC2:
    case slvPOLYNARY_CONSTRUCT_IVEC3:
    case slvPOLYNARY_CONSTRUCT_IVEC4:
    case slvPOLYNARY_CONSTRUCT_UVEC2:
    case slvPOLYNARY_CONSTRUCT_UVEC3:
    case slvPOLYNARY_CONSTRUCT_UVEC4:
        status = sloIR_POLYNARY_EXPR_ConstructVectorOrMatrixConstant(
                                                                    Compiler,
                                                                    PolynaryExpr,
                                                                    gcvTRUE,
                                                                    ResultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvPOLYNARY_CONSTRUCT_MAT2:
    case slvPOLYNARY_CONSTRUCT_MAT2X3:
    case slvPOLYNARY_CONSTRUCT_MAT2X4:
    case slvPOLYNARY_CONSTRUCT_MAT3:
    case slvPOLYNARY_CONSTRUCT_MAT3X2:
    case slvPOLYNARY_CONSTRUCT_MAT3X4:
    case slvPOLYNARY_CONSTRUCT_MAT4:
    case slvPOLYNARY_CONSTRUCT_MAT4X2:
    case slvPOLYNARY_CONSTRUCT_MAT4X3:
        status = sloIR_POLYNARY_EXPR_ConstructVectorOrMatrixConstant(
                                                                    Compiler,
                                                                    PolynaryExpr,
                                                                    gcvFALSE,
                                                                    ResultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvPOLYNARY_CONSTRUCT_STRUCT:
        status = sloIR_POLYNARY_EXPR_ConstructStructConstant(
                                                            Compiler,
                                                            PolynaryExpr,
                                                            ResultConstant);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        break;

    case slvPOLYNARY_FUNC_CALL:
        gcmASSERT(PolynaryExpr->funcName);

        if (PolynaryExpr->funcName->isBuiltIn)
        {
            status = sloIR_POLYNARY_EXPR_EvaluateBuiltIn(
                                                        Compiler,
                                                        PolynaryExpr,
                                                        ResultConstant);

            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }
        else
        {
            *ResultConstant = gcvNULL;
        }

        break;

    default:
        gcmASSERT(0);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
