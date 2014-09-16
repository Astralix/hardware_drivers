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


#include "gc_glsl_parser.h"
#include "gc_glsl_built_ins.h"

/* Basic Built-In Types */
static gctINT BasicBuiltInTypes[] =
{
    T_BOOL,
    T_INT,
    T_FLOAT,
    T_UINT,

    T_BVEC2,
    T_BVEC3,
    T_BVEC4,

    T_IVEC2,
    T_IVEC3,
    T_IVEC4,

    T_UVEC2,
    T_UVEC3,
    T_UVEC4,

    T_VEC2,
    T_VEC3,
    T_VEC4,

    T_MAT2,
    T_MAT2X3,
    T_MAT2X4,

    T_MAT3,
    T_MAT3X2,
    T_MAT3X4,

    T_MAT4,
    T_MAT4X2,
    T_MAT4X3,

    T_SAMPLER2D,
    T_SAMPLERCUBE,
    T_SAMPLER3D,
    T_SAMPLER1DARRAY,
    T_SAMPLER2DARRAY,
    T_SAMPLER1DARRAYSHADOW,
    T_SAMPLER2DARRAYSHADOW,

    T_ISAMPLER2D,
    T_ISAMPLERCUBE,
    T_ISAMPLER3D,
    T_ISAMPLER2DARRAY,

    T_USAMPLER2D,
    T_USAMPLERCUBE,
    T_USAMPLER3D,
    T_USAMPLER2DARRAY,
    T_SAMPLEREXTERNALOES
};

static gctUINT  BasicBuiltInTypeCount   = sizeof(BasicBuiltInTypes) / sizeof(gctINT);

typedef struct _slsBASIC_BUILT_IN_TYPE_INFO
{
    gctINT              type;

    slsDATA_TYPE *      normalDataType;

    slsDATA_TYPE *      inDataType;
}
slsBASIC_BUILT_IN_TYPE_INFO;

static gceSTATUS
_ConstructBasicBuiltInTypeInfos(
    IN sloCOMPILER Compiler,
    OUT slsBASIC_BUILT_IN_TYPE_INFO * * BasicBuiltInTypeInfos
   )
{
    gceSTATUS                       status;
    slsBASIC_BUILT_IN_TYPE_INFO *   basicBuiltInTypeInfos = gcvNULL;
    gctUINT                         i;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(BasicBuiltInTypeInfos);

    do
    {
        gctPOINTER pointer = gcvNULL;

        status = sloCOMPILER_Allocate(
                                    Compiler,
                                    (gctSIZE_T)sizeof(slsBASIC_BUILT_IN_TYPE_INFO)
                                        * BasicBuiltInTypeCount,
                                    &pointer);

        if (gcmIS_ERROR(status)) break;

        basicBuiltInTypeInfos = pointer;

        for (i = 0; i < BasicBuiltInTypeCount; i++)
        {
            basicBuiltInTypeInfos[i].type = BasicBuiltInTypes[i];

            status = sloCOMPILER_CreateDataType(
                                                Compiler,
                                                basicBuiltInTypeInfos[i].type,
                                                gcvNULL,
                                                &basicBuiltInTypeInfos[i].normalDataType);

            if (gcmIS_ERROR(status)) break;

            status = sloCOMPILER_CreateDataType(
                                                Compiler,
                                                basicBuiltInTypeInfos[i].type,
                                                gcvNULL,
                                                &basicBuiltInTypeInfos[i].inDataType);

            if (gcmIS_ERROR(status)) break;

            basicBuiltInTypeInfos[i].inDataType->qualifier = slvQUALIFIER_IN;
        }

        if (gcmIS_ERROR(status)) break;

        *BasicBuiltInTypeInfos = basicBuiltInTypeInfos;

        gcmFOOTER_ARG("*BasicBuiltInTypeInfos=0x%x", *BasicBuiltInTypeInfos);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (basicBuiltInTypeInfos != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Free(Compiler, basicBuiltInTypeInfos));
    }

    *BasicBuiltInTypeInfos = gcvNULL;

    gcmFOOTER();
    return status;
}

static gceSTATUS
_DestroyBasicBuiltInTypeInfos(
    IN sloCOMPILER Compiler,
    IN slsBASIC_BUILT_IN_TYPE_INFO * BasicBuiltInTypeInfos
    )
{
    gcmHEADER_ARG("Compiler=0x%x BasicBuiltInTypeInfos=0x%x",
                  Compiler, BasicBuiltInTypeInfos);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(BasicBuiltInTypeInfos);

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, BasicBuiltInTypeInfos));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static slsBASIC_BUILT_IN_TYPE_INFO *
_GetBasicBuiltInTypeInfo(
    IN slsBASIC_BUILT_IN_TYPE_INFO * BasicBuiltInTypeInfos,
    IN gctINT Type
    )
{
    gctUINT i;

    for (i = 0; i < BasicBuiltInTypeCount; i++)
    {
        if (BasicBuiltInTypeInfos[i].type == Type)
        {
            return &BasicBuiltInTypeInfos[i];
        }
    }

    return gcvNULL;
}

/* Default Precision Declarations */
typedef struct _slsDEFAULT_PRECISION_DECL
{
    sltPRECISION        precision;

    gctINT              type;
}
slsDEFAULT_PRECISION_DECL;

static slsDEFAULT_PRECISION_DECL    VSDefaultPrecisionDecls[] =
{
    /* Default Precision Declarations */
    {slvPRECISION_HIGH,     T_INT},
    {slvPRECISION_HIGH,     T_FLOAT},
    {slvPRECISION_LOW,      T_SAMPLER2D},
    {slvPRECISION_LOW,      T_SAMPLERCUBE},
    {slvPRECISION_LOW,      T_SAMPLEREXTERNALOES}
};

static gctUINT VSDefaultPrecisionDeclCount =
                    sizeof(VSDefaultPrecisionDecls) / sizeof(slsDEFAULT_PRECISION_DECL);

static slsDEFAULT_PRECISION_DECL    FSDefaultPrecisionDecls[] =
{
    /* Default Precision Declarations */
    {slvPRECISION_MEDIUM,   T_INT},
    {slvPRECISION_LOW,      T_SAMPLER2D},
    {slvPRECISION_LOW,      T_SAMPLERCUBE},
    {slvPRECISION_LOW,      T_SAMPLEREXTERNALOES}
};

static gctUINT FSDefaultPrecisionDeclCount =
                    sizeof(FSDefaultPrecisionDecls) / sizeof(slsDEFAULT_PRECISION_DECL);

static gceSTATUS
_LoadDefaultPrecisionDecls(
    IN sloCOMPILER Compiler,
    IN slsBASIC_BUILT_IN_TYPE_INFO * BasicBuiltInTypeInfos,
    IN gctUINT DefaultPrecisionDeclCount,
    IN slsDEFAULT_PRECISION_DECL * DefaultPrecisionDecls
    )
{
    gcmHEADER_ARG("Compiler=0x%x BasicBuiltInTypeInfos=0x%x "
                  "DefaultPrecisionDeclCount=%u DefaultPrecisionDecls=0x%x",
                  Compiler, BasicBuiltInTypeInfos, DefaultPrecisionDeclCount,
                  DefaultPrecisionDecls);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(BasicBuiltInTypeInfos);
    gcmVERIFY_ARGUMENT(DefaultPrecisionDeclCount > 0);
    gcmVERIFY_ARGUMENT(DefaultPrecisionDecls);

    /* TODO: Not implemented yet */

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/* Built-In Constant */
static gceSTATUS
_LoadBuiltInConstants(
    IN sloCOMPILER Compiler
    )
{
    gceSTATUS           status;
    gctUINT             i;
    gctUINT             maxVertexAttribs            = 8;
    gctUINT             maxVertexUniformVectors     = 128;
    gctUINT             maxVaryingVectors           = 8;
    gctUINT             maxVertexTextureImageUnits  = 0;
    gctUINT             maxTextureImageUnits        = 8;
    gctUINT             maxFragmentUniformVectors   = 16;
    gctUINT             maxDrawBuffers              = 1;

    struct
    {
        gctCONST_STRING     symbol;

        gctUINT             value;
    }
    constantInfos[8];

    slsDATA_TYPE *      dataType;
    sloIR_CONSTANT      constant;
    sluCONSTANT_VALUE   value;
    sltPOOL_STRING      variableSymbol;
    slsNAME *           variableName;
    gcoHAL              hal;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    /* Grab the HAL object. */
    gcmVERIFY_OK(
        sloCOMPILER_GetHAL(Compiler, &hal));

    /* Setup the constant values */
    gcmVERIFY_OK(
        gcoHAL_QueryStreamCaps(hal,
                               &maxVertexAttribs,
                               gcvNULL,
                               gcvNULL,
                               gcvNULL));

    gcmVERIFY_OK(
        gcoHAL_QueryShaderCaps(hal,
                               &maxVertexUniformVectors,
                               &maxFragmentUniformVectors,
                               &maxVaryingVectors));

    gcmVERIFY_OK(
        gcoHAL_QueryTextureCaps(hal,
                                gcvNULL,
                                gcvNULL,
                                gcvNULL,
                                gcvNULL,
                                gcvNULL,
                                &maxVertexTextureImageUnits,
                                &maxTextureImageUnits));

    constantInfos[0].symbol     = "gl_MaxVertexAttribs";
    constantInfos[0].value      = maxVertexAttribs;

    constantInfos[1].symbol     = "gl_MaxVertexUniformVectors";
    constantInfos[1].value      = maxVertexUniformVectors;

    constantInfos[2].symbol     = "gl_MaxVaryingVectors";
    constantInfos[2].value      = maxVaryingVectors;

    constantInfos[3].symbol     = "gl_MaxVertexTextureImageUnits";
    constantInfos[3].value      = maxVertexTextureImageUnits;

    constantInfos[4].symbol     = "gl_MaxCombinedTextureImageUnits";
    constantInfos[4].value      = maxVertexTextureImageUnits + maxTextureImageUnits;

    constantInfos[5].symbol     = "gl_MaxTextureImageUnits";
    constantInfos[5].value      = maxTextureImageUnits;

    constantInfos[6].symbol     = "gl_MaxFragmentUniformVectors";
    constantInfos[6].value      = maxFragmentUniformVectors;

    constantInfos[7].symbol     = "gl_MaxDrawBuffers";
    constantInfos[7].value      = maxDrawBuffers;

    /* Create the data type */
    status = sloCOMPILER_CreateDataType(
                                        Compiler,
                                        T_INT,
                                        gcvNULL,
                                        &dataType);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    dataType->qualifier = slvQUALIFIER_CONST;
    dataType->precision = slvPRECISION_MEDIUM;

    for (i = 0; i < 8; i++)
    {
        /* Create the constant */
        status = sloIR_CONSTANT_Construct(
                                        Compiler,
                                        0,
                                        0,
                                        dataType,
                                        &constant);

        if (gcmIS_ERROR(status)) break;

        /* Add the constant value */
        value.intValue = (gctINT32)constantInfos[i].value;

        status = sloIR_CONSTANT_AddValues(
                                        Compiler,
                                        constant,
                                        1,
                                        &value);

        if (gcmIS_ERROR(status)) break;

        gcmVERIFY_OK(sloCOMPILER_AddExternalDecl(Compiler, &constant->exprBase.base));

        /* Create the variable name */
        status = sloCOMPILER_AllocatePoolString(
                                                Compiler,
                                                constantInfos[i].symbol,
                                                &variableSymbol);

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_CreateName(
                                        Compiler,
                                        0,
                                        0,
                                        slvVARIABLE_NAME,
                                        dataType,
                                        variableSymbol,
                                        slvEXTENSION_NONE,
                                        &variableName);

        if (gcmIS_ERROR(status)) break;

        variableName->u.variableInfo.constant = constant;
    }

    gcmFOOTER();
    return status;
}

/* Built-In Uniform State */
static gceSTATUS
_LoadBuiltInUniformState(
    IN sloCOMPILER Compiler,
    IN slsBASIC_BUILT_IN_TYPE_INFO * BasicBuiltInTypeInfos
    )
{
    gceSTATUS               status;
    gctUINT                 i;
    slsNAME_SPACE *         fieldNameSpace;
    slsDATA_TYPE *          fieldDataType;
    const gctCONST_STRING   fields[3] = {"near", "far", "diff"};
    sltPOOL_STRING          fieldSymbol;
    slsDATA_TYPE *          structDataType;
    sltPOOL_STRING          structSymbol;
    sltPOOL_STRING          variableSymbol;

    gcmHEADER_ARG("Compiler=0x%x BasicBuiltInTypeInfos=0x%x",
                  Compiler, BasicBuiltInTypeInfos);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(BasicBuiltInTypeInfos);

    do
    {
        /* Create the struct field: near, far, diff */
        status = sloCOMPILER_CreateNameSpace(
                                            Compiler,
                                            &fieldNameSpace);

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_CreateDataType(
                                            Compiler,
                                            T_FLOAT,
                                            gcvNULL,
                                            &fieldDataType);

        if (gcmIS_ERROR(status)) break;

        fieldDataType->precision = slvPRECISION_HIGH;

        for (i = 0; i < 3; i++)
        {
            status = sloCOMPILER_AllocatePoolString(
                                                    Compiler,
                                                    fields[i],
                                                    &fieldSymbol);

            if (gcmIS_ERROR(status)) break;

            status = sloCOMPILER_CreateName(
                                            Compiler,
                                            0,
                                            0,
                                            slvFIELD_NAME,
                                            fieldDataType,
                                            fieldSymbol,
                                            slvEXTENSION_NONE,
                                            gcvNULL);

            if (gcmIS_ERROR(status)) break;
        }

        if (gcmIS_ERROR(status)) break;

        sloCOMPILER_PopCurrentNameSpace(Compiler, gcvNULL);

        /* Create the struct type: gl_DepthRangeParameters */
        status = sloCOMPILER_CreateDataType(
                                            Compiler,
                                            T_STRUCT,
                                            fieldNameSpace,
                                            &structDataType);

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_AllocatePoolString(
                                                Compiler,
                                                "gl_DepthRangeParameters",
                                                &structSymbol);

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_CreateName(
                                        Compiler,
                                        0,
                                        0,
                                        slvSTRUCT_NAME,
                                        structDataType,
                                        structSymbol,
                                        slvEXTENSION_NONE,
                                        gcvNULL);

        if (gcmIS_ERROR(status)) break;

        /* Create the uniform variable: gl_DepthRange */
        status = sloCOMPILER_AllocatePoolString(
                                                Compiler,
                                                "gl_DepthRange",
                                                &variableSymbol);

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_CreateName(
                                        Compiler,
                                        0,
                                        0,
                                        slvVARIABLE_NAME,
                                        structDataType,
                                        variableSymbol,
                                        slvEXTENSION_NONE,
                                        gcvNULL);

        if (gcmIS_ERROR(status)) break;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmFOOTER();
    return status;
}

/* Built-In Variables */
typedef struct _slsBUILT_IN_VARIABLE
{
    sleEXTENSION        extension;

    gctCONST_STRING     symbol;

    sltPRECISION        precision;

    gctINT              type;

    gctUINT             arrayLength;
}
slsBUILT_IN_VARIABLE;

static slsBUILT_IN_VARIABLE VSBuiltInVariables[] =
{
    {slvEXTENSION_NONE, "gl_Position",         slvPRECISION_HIGH,      T_VEC4,     0},
    {slvEXTENSION_NONE, "gl_PointSize",        slvPRECISION_MEDIUM,    T_FLOAT,    0}
};

static gctUINT VSBuiltInVariableCount =
                    sizeof(VSBuiltInVariables) / sizeof(slsBUILT_IN_VARIABLE);

static slsBUILT_IN_VARIABLE FSBuiltInVariables[] =
{
    {slvEXTENSION_NONE, "gl_FragCoord",        slvPRECISION_MEDIUM,    T_VEC4,     0},
    {slvEXTENSION_NONE, "gl_FrontFacing",      slvPRECISION_DEFAULT,   T_BOOL,     0},
    {slvEXTENSION_NONE, "gl_FragColor",        slvPRECISION_MEDIUM,    T_VEC4,     0},
    {slvEXTENSION_NONE, "gl_FragData",         slvPRECISION_MEDIUM,    T_VEC4,     1},
    {slvEXTENSION_NONE, "gl_PointCoord",       slvPRECISION_MEDIUM,    T_VEC2,     0},
    {slvEXTENSION_FRAG_DEPTH, "gl_FragDepthEXT", slvPRECISION_MEDIUM,  T_FLOAT,    0}
};

static gctUINT FSBuiltInVariableCount =
                    sizeof(FSBuiltInVariables) / sizeof(slsBUILT_IN_VARIABLE);

static gceSTATUS
_LoadBuiltInVariables(
    IN sloCOMPILER Compiler,
    IN slsBASIC_BUILT_IN_TYPE_INFO * BasicBuiltInTypeInfos,
    IN gctUINT BuiltInVariableCount,
    IN slsBUILT_IN_VARIABLE * BuiltInVariables
    )
{
    gceSTATUS                       status = gcvSTATUS_OK;
    gctUINT                         i;
    slsBASIC_BUILT_IN_TYPE_INFO *   basicBuiltInTypeInfo;
    slsDATA_TYPE *                  dataType;
    sltPOOL_STRING                  symbolInPool;

    gcmHEADER_ARG("Compiler=0x%x BasicBuiltInTypeInfos=0x%x "
                  "BuiltInVariableCount=%u BuiltInVariables=0x%x",
                  Compiler, BasicBuiltInTypeInfos, BuiltInVariableCount,
                  BuiltInVariables);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(BasicBuiltInTypeInfos);
    gcmVERIFY_ARGUMENT(BuiltInVariableCount > 0);
    gcmVERIFY_ARGUMENT(BuiltInVariables);

    for (i = 0; i < BuiltInVariableCount; i++)
    {
        /* Setup the data type */
        if (BuiltInVariables[i].precision != slvPRECISION_DEFAULT)
        {
            status = sloCOMPILER_CreateDataType(
                                                Compiler,
                                                BuiltInVariables[i].type,
                                                gcvNULL,
                                                &dataType);

            if (gcmIS_ERROR(status)) break;

            dataType->precision = BuiltInVariables[i].precision;
        }
        else
        {
            basicBuiltInTypeInfo = _GetBasicBuiltInTypeInfo(
                                                            BasicBuiltInTypeInfos,
                                                            BuiltInVariables[i].type);

            gcmASSERT(basicBuiltInTypeInfo);

            if (basicBuiltInTypeInfo == gcvNULL)
                break;

            dataType = basicBuiltInTypeInfo->normalDataType;
        }

        if (BuiltInVariables[i].arrayLength != 0)
        {
            status = sloCOMPILER_CreateArrayDataType(
                                                    Compiler,
                                                    dataType,
                                                    BuiltInVariables[i].arrayLength,
                                                    &dataType);

            if (gcmIS_ERROR(status)) break;
        }

        /* Create the variable name */
        status = sloCOMPILER_AllocatePoolString(
                                                Compiler,
                                                BuiltInVariables[i].symbol,
                                                &symbolInPool);

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_CreateName(
                                        Compiler,
                                        0,
                                        0,
                                        slvVARIABLE_NAME,
                                        dataType,
                                        symbolInPool,
                                        BuiltInVariables[i].extension,
                                        gcvNULL);

        if (gcmIS_ERROR(status)) break;
    }

    gcmFOOTER();
    return status;
}

/* Built-In Functions */
typedef struct _slsBUILT_IN_FUNCTION
{
    sleEXTENSION        extension;

    gctCONST_STRING     symbol;

    gctINT              returnType;

    gctUINT             paramCount;

    gctINT              paramTypes[slmMAX_BUILT_IN_PARAMETER_COUNT];
}
slsBUILT_IN_FUNCTION;

static slsBUILT_IN_FUNCTION VSBuiltInFunctions[] =
{
    /* Texture Lookup Functions */
    {slvEXTENSION_NONE,     "texture2DLod",         T_VEC4,     3, {T_SAMPLER2D,    T_VEC2,     T_FLOAT}},
    {slvEXTENSION_NONE,     "texture2DProjLod",     T_VEC4,     3, {T_SAMPLER2D,    T_VEC3,     T_FLOAT}},
    {slvEXTENSION_NONE,     "texture2DProjLod",     T_VEC4,     3, {T_SAMPLER2D,    T_VEC4,     T_FLOAT}},
    {slvEXTENSION_NONE,     "textureCubeLod",       T_VEC4,     3, {T_SAMPLERCUBE,  T_VEC3,     T_FLOAT}},

    /* 3D Texture Lookup Functions */
    {slvEXTENSION_TEXTURE_3D, "texture3DLod",         T_VEC4,   3, {T_SAMPLER3D,    T_VEC3,     T_FLOAT}},
    {slvEXTENSION_TEXTURE_3D, "texture3DProjLod",     T_VEC4,   3, {T_SAMPLER3D,    T_VEC4,     T_FLOAT}},

    /* Texture array Lookup Functions */
    {slvEXTENSION_TEXTURE_ARRAY, "texture1DArrayLod", T_VEC4,   3, {T_SAMPLER1DARRAY,    T_VEC2,     T_FLOAT}},
    {slvEXTENSION_TEXTURE_ARRAY, "texture2DArrayLod", T_VEC4,   3, {T_SAMPLER2DARRAY,    T_VEC3,     T_FLOAT}},
    {slvEXTENSION_TEXTURE_ARRAY, "shadow1DArrayLod", T_VEC4,    3, {T_SAMPLER1DARRAYSHADOW, T_VEC3,  T_FLOAT}}
};

static gctUINT VSBuiltInFunctionCount =
                    sizeof(VSBuiltInFunctions) / sizeof(slsBUILT_IN_FUNCTION);

static slsBUILT_IN_FUNCTION FSBuiltInFunctions[] =
{
    /* Texture Lookup Functions */
    {slvEXTENSION_NONE,     "texture2D",            T_VEC4,     3, {T_SAMPLER2D,    T_VEC2,     T_FLOAT}},
    {slvEXTENSION_NONE,     "texture2DProj",        T_VEC4,     3, {T_SAMPLER2D,    T_VEC3,     T_FLOAT}},
    {slvEXTENSION_NONE,     "texture2DProj",        T_VEC4,     3, {T_SAMPLER2D,    T_VEC4,     T_FLOAT}},
    {slvEXTENSION_NONE,     "textureCube",          T_VEC4,     3, {T_SAMPLERCUBE,  T_VEC3,     T_FLOAT}},

    /* 3D Texture Lookup Functions */
    {slvEXTENSION_TEXTURE_3D,     "texture3D",      T_VEC4,     3, {T_SAMPLER3D,    T_VEC3,     T_FLOAT}},
    {slvEXTENSION_TEXTURE_3D,     "texture3DProj",  T_VEC4,     3, {T_SAMPLER3D,    T_VEC4,     T_FLOAT}},

    /* Texture array Lookup Functions */
    {slvEXTENSION_TEXTURE_ARRAY, "texture1DArray", T_VEC4,     3, {T_SAMPLER1DARRAY,    T_VEC2,     T_FLOAT}},
    {slvEXTENSION_TEXTURE_ARRAY, "texture2DArray", T_VEC4,     3, {T_SAMPLER2DARRAY,    T_VEC3,     T_FLOAT}},
    {slvEXTENSION_TEXTURE_ARRAY, "shadow1DArray",  T_VEC4,     3, {T_SAMPLER1DARRAYSHADOW, T_VEC3,  T_FLOAT}},

    /* Derivative Functions */
    {slvEXTENSION_STANDARD_DERIVATIVES,     "dFdx",     T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "dFdx",     T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "dFdx",     T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "dFdx",     T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_STANDARD_DERIVATIVES,     "dFdy",     T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "dFdy",     T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "dFdy",     T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "dFdy",     T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_STANDARD_DERIVATIVES,     "fwidth",   T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "fwidth",   T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "fwidth",   T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_STANDARD_DERIVATIVES,     "fwidth",   T_VEC4,     1, {T_VEC4}}
};

static gctUINT FSBuiltInFunctionCount =
                    sizeof(FSBuiltInFunctions) / sizeof(slsBUILT_IN_FUNCTION);

static slsBUILT_IN_FUNCTION CommonBuiltInFunctions[] =
{
    /* Angle and Trigonometry Functions */
    {slvEXTENSION_NONE,     "radians",              T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "radians",              T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "radians",              T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "radians",              T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "degrees",              T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "degrees",              T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "degrees",              T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "degrees",              T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "sin",                  T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "sin",                  T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "sin",                  T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "sin",                  T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "cos",                  T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "cos",                  T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "cos",                  T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "cos",                  T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "tan",                  T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "tan",                  T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "tan",                  T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "tan",                  T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "asin",                 T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "asin",                 T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "asin",                 T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "asin",                 T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "acos",                 T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "acos",                 T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "acos",                 T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "acos",                 T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "atan",                 T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "atan",                 T_VEC2,     2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "atan",                 T_VEC3,     2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "atan",                 T_VEC4,     2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "atan",                 T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "atan",                 T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "atan",                 T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "atan",                 T_VEC4,     1, {T_VEC4}},

    /* Exponential Functions */
    {slvEXTENSION_NONE,     "pow",                  T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "pow",                  T_VEC2,     2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "pow",                  T_VEC3,     2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "pow",                  T_VEC4,     2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "exp",                  T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "exp",                  T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "exp",                  T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "exp",                  T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "log",                  T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "log",                  T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "log",                  T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "log",                  T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "exp2",                 T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "exp2",                 T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "exp2",                 T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "exp2",                 T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "log2",                 T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "log2",                 T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "log2",                 T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "log2",                 T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "sqrt",                 T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "sqrt",                 T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "sqrt",                 T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "sqrt",                 T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "inversesqrt",          T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "inversesqrt",          T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "inversesqrt",          T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "inversesqrt",          T_VEC4,     1, {T_VEC4}},

    /* Common Functions */
    {slvEXTENSION_NONE,     "abs",                  T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "abs",                  T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "abs",                  T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "abs",                  T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "sign",                 T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "sign",                 T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "sign",                 T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "sign",                 T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "floor",                T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "floor",                T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "floor",                T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "floor",                T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "ceil",                 T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "ceil",                 T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "ceil",                 T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "ceil",                 T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "fract",                T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "fract",                T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "fract",                T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "fract",                T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "mod",                  T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "mod",                  T_VEC2,     2, {T_VEC2,         T_FLOAT}},
    {slvEXTENSION_NONE,     "mod",                  T_VEC3,     2, {T_VEC3,         T_FLOAT}},
    {slvEXTENSION_NONE,     "mod",                  T_VEC4,     2, {T_VEC4,         T_FLOAT}},
    {slvEXTENSION_NONE,     "mod",                  T_VEC2,     2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "mod",                  T_VEC3,     2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "mod",                  T_VEC4,     2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "min",                  T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "min",                  T_VEC2,     2, {T_VEC2,         T_FLOAT}},
    {slvEXTENSION_NONE,     "min",                  T_VEC3,     2, {T_VEC3,         T_FLOAT}},
    {slvEXTENSION_NONE,     "min",                  T_VEC4,     2, {T_VEC4,         T_FLOAT}},
    {slvEXTENSION_NONE,     "min",                  T_VEC2,     2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "min",                  T_VEC3,     2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "min",                  T_VEC4,     2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "max",                  T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "max",                  T_VEC2,     2, {T_VEC2,         T_FLOAT}},
    {slvEXTENSION_NONE,     "max",                  T_VEC3,     2, {T_VEC3,         T_FLOAT}},
    {slvEXTENSION_NONE,     "max",                  T_VEC4,     2, {T_VEC4,         T_FLOAT}},
    {slvEXTENSION_NONE,     "max",                  T_VEC2,     2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "max",                  T_VEC3,     2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "max",                  T_VEC4,     2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "clamp",                T_FLOAT,    3, {T_FLOAT,        T_FLOAT,    T_FLOAT}},
    {slvEXTENSION_NONE,     "clamp",                T_VEC2,     3, {T_VEC2,         T_FLOAT,    T_FLOAT}},
    {slvEXTENSION_NONE,     "clamp",                T_VEC3,     3, {T_VEC3,         T_FLOAT,    T_FLOAT}},
    {slvEXTENSION_NONE,     "clamp",                T_VEC4,     3, {T_VEC4,         T_FLOAT,    T_FLOAT}},
    {slvEXTENSION_NONE,     "clamp",                T_VEC2,     3, {T_VEC2,         T_VEC2,     T_VEC2}},
    {slvEXTENSION_NONE,     "clamp",                T_VEC3,     3, {T_VEC3,         T_VEC3,     T_VEC3}},
    {slvEXTENSION_NONE,     "clamp",                T_VEC4,     3, {T_VEC4,         T_VEC4,     T_VEC4}},

    {slvEXTENSION_NONE,     "mix",                  T_FLOAT,    3, {T_FLOAT,        T_FLOAT,    T_FLOAT}},
    {slvEXTENSION_NONE,     "mix",                  T_VEC2,     3, {T_VEC2,         T_VEC2,     T_FLOAT}},
    {slvEXTENSION_NONE,     "mix",                  T_VEC3,     3, {T_VEC3,         T_VEC3,     T_FLOAT}},
    {slvEXTENSION_NONE,     "mix",                  T_VEC4,     3, {T_VEC4,         T_VEC4,     T_FLOAT}},
    {slvEXTENSION_NONE,     "mix",                  T_VEC2,     3, {T_VEC2,         T_VEC2,     T_VEC2}},
    {slvEXTENSION_NONE,     "mix",                  T_VEC3,     3, {T_VEC3,         T_VEC3,     T_VEC3}},
    {slvEXTENSION_NONE,     "mix",                  T_VEC4,     3, {T_VEC4,         T_VEC4,     T_VEC4}},

    {slvEXTENSION_NONE,     "step",                 T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "step",                 T_VEC2,     2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "step",                 T_VEC3,     2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "step",                 T_VEC4,     2, {T_VEC4,         T_VEC4}},
    {slvEXTENSION_NONE,     "step",                 T_VEC2,     2, {T_FLOAT,        T_VEC2}},
    {slvEXTENSION_NONE,     "step",                 T_VEC3,     2, {T_FLOAT,        T_VEC3}},
    {slvEXTENSION_NONE,     "step",                 T_VEC4,     2, {T_FLOAT,        T_VEC4}},

    {slvEXTENSION_NONE,     "smoothstep",           T_FLOAT,    3, {T_FLOAT,        T_FLOAT,    T_FLOAT}},
    {slvEXTENSION_NONE,     "smoothstep",           T_VEC2,     3, {T_VEC2,         T_VEC2,     T_VEC2}},
    {slvEXTENSION_NONE,     "smoothstep",           T_VEC3,     3, {T_VEC3,         T_VEC3,     T_VEC3}},
    {slvEXTENSION_NONE,     "smoothstep",           T_VEC4,     3, {T_VEC4,         T_VEC4,     T_VEC4}},
    {slvEXTENSION_NONE,     "smoothstep",           T_VEC2,     3, {T_FLOAT,        T_FLOAT,    T_VEC2}},
    {slvEXTENSION_NONE,     "smoothstep",           T_VEC3,     3, {T_FLOAT,        T_FLOAT,    T_VEC3}},
    {slvEXTENSION_NONE,     "smoothstep",           T_VEC4,     3, {T_FLOAT,        T_FLOAT,    T_VEC4}},

    /* Geometric Functions */
    {slvEXTENSION_NONE,     "length",               T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "length",               T_FLOAT,    1, {T_VEC2}},
    {slvEXTENSION_NONE,     "length",               T_FLOAT,    1, {T_VEC3}},
    {slvEXTENSION_NONE,     "length",               T_FLOAT,    1, {T_VEC4}},

    {slvEXTENSION_NONE,     "distance",             T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "distance",             T_FLOAT,    2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "distance",             T_FLOAT,    2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "distance",             T_FLOAT,    2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "dot",                  T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "dot",                  T_FLOAT,    2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "dot",                  T_FLOAT,    2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "dot",                  T_FLOAT,    2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "cross",                T_VEC3,     2, {T_VEC3,         T_VEC3}},

    {slvEXTENSION_NONE,     "normalize",            T_FLOAT,    1, {T_FLOAT}},
    {slvEXTENSION_NONE,     "normalize",            T_VEC2,     1, {T_VEC2}},
    {slvEXTENSION_NONE,     "normalize",            T_VEC3,     1, {T_VEC3}},
    {slvEXTENSION_NONE,     "normalize",            T_VEC4,     1, {T_VEC4}},

    {slvEXTENSION_NONE,     "faceforward",          T_FLOAT,    3, {T_FLOAT,        T_FLOAT,    T_FLOAT}},
    {slvEXTENSION_NONE,     "faceforward",          T_VEC2,     3, {T_VEC2,         T_VEC2,     T_VEC2}},
    {slvEXTENSION_NONE,     "faceforward",          T_VEC3,     3, {T_VEC3,         T_VEC3,     T_VEC3}},
    {slvEXTENSION_NONE,     "faceforward",          T_VEC4,     3, {T_VEC4,         T_VEC4,     T_VEC4}},

    {slvEXTENSION_NONE,     "reflect",              T_FLOAT,    2, {T_FLOAT,        T_FLOAT}},
    {slvEXTENSION_NONE,     "reflect",              T_VEC2,     2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "reflect",              T_VEC3,     2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "reflect",              T_VEC4,     2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "refract",              T_FLOAT,    3, {T_FLOAT,        T_FLOAT,    T_FLOAT}},
    {slvEXTENSION_NONE,     "refract",              T_VEC2,     3, {T_VEC2,         T_VEC2,     T_FLOAT}},
    {slvEXTENSION_NONE,     "refract",              T_VEC3,     3, {T_VEC3,         T_VEC3,     T_FLOAT}},
    {slvEXTENSION_NONE,     "refract",              T_VEC4,     3, {T_VEC4,         T_VEC4,     T_FLOAT}},

    /* Matrix Functions */
    {slvEXTENSION_NONE,     "matrixCompMult",       T_MAT2,     2, {T_MAT2,         T_MAT2}},
    {slvEXTENSION_NONE,     "matrixCompMult",       T_MAT3,     2, {T_MAT3,         T_MAT3}},
    {slvEXTENSION_NONE,     "matrixCompMult",       T_MAT4,     2, {T_MAT4,         T_MAT4}},

    /* Vector Relational Functions */
    {slvEXTENSION_NONE,     "lessThan",             T_BVEC2,    2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "lessThan",             T_BVEC3,    2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "lessThan",             T_BVEC4,    2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "lessThan",             T_BVEC2,    2, {T_IVEC2,        T_IVEC2}},
    {slvEXTENSION_NONE,     "lessThan",             T_BVEC3,    2, {T_IVEC3,        T_IVEC3}},
    {slvEXTENSION_NONE,     "lessThan",             T_BVEC4,    2, {T_IVEC4,        T_IVEC4}},

    {slvEXTENSION_NONE,     "lessThanEqual",        T_BVEC2,    2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "lessThanEqual",        T_BVEC3,    2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "lessThanEqual",        T_BVEC4,    2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "lessThanEqual",        T_BVEC2,    2, {T_IVEC2,        T_IVEC2}},
    {slvEXTENSION_NONE,     "lessThanEqual",        T_BVEC3,    2, {T_IVEC3,        T_IVEC3}},
    {slvEXTENSION_NONE,     "lessThanEqual",        T_BVEC4,    2, {T_IVEC4,        T_IVEC4}},

    {slvEXTENSION_NONE,     "greaterThan",          T_BVEC2,    2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "greaterThan",          T_BVEC3,    2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "greaterThan",          T_BVEC4,    2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "greaterThan",          T_BVEC2,    2, {T_IVEC2,        T_IVEC2}},
    {slvEXTENSION_NONE,     "greaterThan",          T_BVEC3,    2, {T_IVEC3,        T_IVEC3}},
    {slvEXTENSION_NONE,     "greaterThan",          T_BVEC4,    2, {T_IVEC4,        T_IVEC4}},

    {slvEXTENSION_NONE,     "greaterThanEqual",     T_BVEC2,    2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "greaterThanEqual",     T_BVEC3,    2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "greaterThanEqual",     T_BVEC4,    2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "greaterThanEqual",     T_BVEC2,    2, {T_IVEC2,        T_IVEC2}},
    {slvEXTENSION_NONE,     "greaterThanEqual",     T_BVEC3,    2, {T_IVEC3,        T_IVEC3}},
    {slvEXTENSION_NONE,     "greaterThanEqual",     T_BVEC4,    2, {T_IVEC4,        T_IVEC4}},

    {slvEXTENSION_NONE,     "equal",                T_BVEC2,    2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "equal",                T_BVEC3,    2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "equal",                T_BVEC4,    2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "equal",                T_BVEC2,    2, {T_IVEC2,        T_IVEC2}},
    {slvEXTENSION_NONE,     "equal",                T_BVEC3,    2, {T_IVEC3,        T_IVEC3}},
    {slvEXTENSION_NONE,     "equal",                T_BVEC4,    2, {T_IVEC4,        T_IVEC4}},

    {slvEXTENSION_NONE,     "equal",                T_BVEC2,    2, {T_BVEC2,        T_BVEC2}},
    {slvEXTENSION_NONE,     "equal",                T_BVEC3,    2, {T_BVEC3,        T_BVEC3}},
    {slvEXTENSION_NONE,     "equal",                T_BVEC4,    2, {T_BVEC4,        T_BVEC4}},

    {slvEXTENSION_NONE,     "notEqual",             T_BVEC2,    2, {T_VEC2,         T_VEC2}},
    {slvEXTENSION_NONE,     "notEqual",             T_BVEC3,    2, {T_VEC3,         T_VEC3}},
    {slvEXTENSION_NONE,     "notEqual",             T_BVEC4,    2, {T_VEC4,         T_VEC4}},

    {slvEXTENSION_NONE,     "notEqual",             T_BVEC2,    2, {T_IVEC2,        T_IVEC2}},
    {slvEXTENSION_NONE,     "notEqual",             T_BVEC3,    2, {T_IVEC3,        T_IVEC3}},
    {slvEXTENSION_NONE,     "notEqual",             T_BVEC4,    2, {T_IVEC4,        T_IVEC4}},

    {slvEXTENSION_NONE,     "notEqual",             T_BVEC2,    2, {T_BVEC2,        T_BVEC2}},
    {slvEXTENSION_NONE,     "notEqual",             T_BVEC3,    2, {T_BVEC3,        T_BVEC3}},
    {slvEXTENSION_NONE,     "notEqual",             T_BVEC4,    2, {T_BVEC4,        T_BVEC4}},

    {slvEXTENSION_NONE,     "any",                  T_BOOL,     1, {T_BVEC2}},
    {slvEXTENSION_NONE,     "any",                  T_BOOL,     1, {T_BVEC3}},
    {slvEXTENSION_NONE,     "any",                  T_BOOL,     1, {T_BVEC4}},

    {slvEXTENSION_NONE,     "all",                  T_BOOL,     1, {T_BVEC2}},
    {slvEXTENSION_NONE,     "all",                  T_BOOL,     1, {T_BVEC3}},
    {slvEXTENSION_NONE,     "all",                  T_BOOL,     1, {T_BVEC4}},

    {slvEXTENSION_NONE,     "not",                  T_BVEC2,    1, {T_BVEC2}},
    {slvEXTENSION_NONE,     "not",                  T_BVEC3,    1, {T_BVEC3}},
    {slvEXTENSION_NONE,     "not",                  T_BVEC4,    1, {T_BVEC4}},

    /* Texture Lookup Functions */
    {slvEXTENSION_NONE,     "texture2D",            T_VEC4,     2, {T_SAMPLER2D,    T_VEC2}},
    {slvEXTENSION_NONE,     "texture2DProj",        T_VEC4,     2, {T_SAMPLER2D,    T_VEC3}},
    {slvEXTENSION_NONE,     "texture2DProj",        T_VEC4,     2, {T_SAMPLER2D,    T_VEC4}},
    {slvEXTENSION_NONE,     "textureCube",          T_VEC4,     2, {T_SAMPLERCUBE,  T_VEC3}},

    {slvEXTENSION_EGL_IMAGE_EXTERNAL,     "texture2D",            T_VEC4,     2, {T_SAMPLEREXTERNALOES,    T_VEC2}},
    {slvEXTENSION_EGL_IMAGE_EXTERNAL,     "texture2DProj",        T_VEC4,     2, {T_SAMPLEREXTERNALOES,    T_VEC3}},
    {slvEXTENSION_EGL_IMAGE_EXTERNAL,     "texture2DProj",        T_VEC4,     2, {T_SAMPLEREXTERNALOES,    T_VEC4}},

    /* 3D Texture Lookup Functions */
    {slvEXTENSION_TEXTURE_3D, "texture3D",          T_VEC4,     2, {T_SAMPLER3D,    T_VEC3}},
    {slvEXTENSION_TEXTURE_3D, "texture3DProj",      T_VEC4,     2, {T_SAMPLER3D,    T_VEC4}},

    /* Texture array Lookup Functions */
    {slvEXTENSION_TEXTURE_ARRAY, "texture1DArray",  T_VEC4,     2, {T_SAMPLER1DARRAY,  T_VEC2}},
    {slvEXTENSION_TEXTURE_ARRAY, "texture2DArray",  T_VEC4,     2, {T_SAMPLER2DARRAY,  T_VEC3}},
    {slvEXTENSION_TEXTURE_ARRAY, "shadow1DArray",   T_VEC4,     2, {T_SAMPLER1DARRAYSHADOW,   T_VEC3}},
    {slvEXTENSION_TEXTURE_ARRAY, "shadow2DArray",   T_VEC4,     2, {T_SAMPLER2DARRAYSHADOW,   T_VEC4}}
};

static gctUINT CommonBuiltInFunctionCount =
                    sizeof(CommonBuiltInFunctions) / sizeof(slsBUILT_IN_FUNCTION);

static gceSTATUS
_LoadBuiltInFunctions(
    IN sloCOMPILER Compiler,
    IN slsBASIC_BUILT_IN_TYPE_INFO * BasicBuiltInTypeInfos,
    IN gctUINT BuiltInFunctionCount,
    IN slsBUILT_IN_FUNCTION * BuiltInFunctions
    )
{
    gceSTATUS                       status = gcvSTATUS_OK;
    gctUINT                         i, j;
    sltPOOL_STRING                  symbolInPool;
    slsBASIC_BUILT_IN_TYPE_INFO *   basicBuiltInTypeInfo;
    slsNAME *                       funcName = gcvNULL;
    slsNAME *                       paramName = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x BasicBuiltInTypeInfos=0x%x "
                  "BuiltInFunctionCount=%u BuiltInFunctions=0x%x",
                  Compiler, BasicBuiltInTypeInfos, BuiltInFunctionCount,
                  BuiltInFunctions);


    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(BasicBuiltInTypeInfos);
    gcmVERIFY_ARGUMENT(BuiltInFunctionCount > 0);
    gcmVERIFY_ARGUMENT(BuiltInFunctions);

    for (i = 0; i < BuiltInFunctionCount; i++)
    {
        /* Create function name */
        basicBuiltInTypeInfo = _GetBasicBuiltInTypeInfo(
                                                        BasicBuiltInTypeInfos,
                                                        BuiltInFunctions[i].returnType);

        gcmASSERT(basicBuiltInTypeInfo);

        if (basicBuiltInTypeInfo == gcvNULL)
            break;

        status = sloCOMPILER_AllocatePoolString(
                                                Compiler,
                                                BuiltInFunctions[i].symbol,
                                                &symbolInPool);

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_CreateName(
                                        Compiler,
                                        0,
                                        0,
                                        slvFUNC_NAME,
                                        basicBuiltInTypeInfo->normalDataType,
                                        symbolInPool,
                                        BuiltInFunctions[i].extension,
                                        &funcName);

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_CreateNameSpace(
                                            Compiler,
                                            &funcName->u.funcInfo.localSpace);

        if (gcmIS_ERROR(status)) break;

        for (j = 0; j < BuiltInFunctions[i].paramCount; j++)
        {
            /* Create parameter name */
            basicBuiltInTypeInfo = _GetBasicBuiltInTypeInfo(
                                                            BasicBuiltInTypeInfos,
                                                            BuiltInFunctions[i].paramTypes[j]);

            gcmASSERT(basicBuiltInTypeInfo);

            if (basicBuiltInTypeInfo == gcvNULL)
                break;

            status = sloCOMPILER_CreateName(
                                            Compiler,
                                            0,
                                            0,
                                            slvPARAMETER_NAME,
                                            basicBuiltInTypeInfo->inDataType,
                                            "",
                                            slvEXTENSION_NONE,
                                            &paramName);

            if (gcmIS_ERROR(status)) break;
        }

        if (gcmIS_ERROR(status)) break;

        sloCOMPILER_PopCurrentNameSpace(Compiler, gcvNULL);

        funcName->u.funcInfo.isFuncDef = gcvFALSE;
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
slLoadBuiltIns(
    IN sloCOMPILER Compiler,
    IN sleSHADER_TYPE ShaderType
    )
{
    gceSTATUS                       status;
    slsBASIC_BUILT_IN_TYPE_INFO *   basicBuiltInTypeInfos;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    status = _ConstructBasicBuiltInTypeInfos(
                                            Compiler,
                                            &basicBuiltInTypeInfos);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    do
    {
        /* Load default precision declarations */
        if (ShaderType == slvSHADER_TYPE_VERTEX)
        {
            status = _LoadDefaultPrecisionDecls(
                                                Compiler,
                                                basicBuiltInTypeInfos,
                                                VSDefaultPrecisionDeclCount,
                                                VSDefaultPrecisionDecls);

            if (gcmIS_ERROR(status)) break;
        }
        else
        {
            status = _LoadDefaultPrecisionDecls(
                                                Compiler,
                                                basicBuiltInTypeInfos,
                                                FSDefaultPrecisionDeclCount,
                                                FSDefaultPrecisionDecls);

            if (gcmIS_ERROR(status)) break;
        }

        /* Load built-in constants */
        status = _LoadBuiltInConstants(Compiler);

        if (gcmIS_ERROR(status)) break;

        /* Load built-in uniform state */
        status = _LoadBuiltInUniformState(
                                        Compiler,
                                        basicBuiltInTypeInfos);

        if (gcmIS_ERROR(status)) break;

        /* Load built-in variables */
        if (ShaderType == slvSHADER_TYPE_VERTEX)
        {
            status = _LoadBuiltInVariables(
                                            Compiler,
                                            basicBuiltInTypeInfos,
                                            VSBuiltInVariableCount,
                                            VSBuiltInVariables);

            if (gcmIS_ERROR(status)) break;
        }
        else
        {
            status = _LoadBuiltInVariables(
                                            Compiler,
                                            basicBuiltInTypeInfos,
                                            FSBuiltInVariableCount,
                                            FSBuiltInVariables);

            if (gcmIS_ERROR(status)) break;
        }

        /* Load built-in functions */
        if (ShaderType == slvSHADER_TYPE_VERTEX)
        {
            status = _LoadBuiltInFunctions(
                                            Compiler,
                                            basicBuiltInTypeInfos,
                                            VSBuiltInFunctionCount,
                                            VSBuiltInFunctions);

            if (gcmIS_ERROR(status)) break;
        }
        else
        {
            status = _LoadBuiltInFunctions(
                                            Compiler,
                                            basicBuiltInTypeInfos,
                                            FSBuiltInFunctionCount,
                                            FSBuiltInFunctions);

            if (gcmIS_ERROR(status)) break;
        }

        status = _LoadBuiltInFunctions(
                                        Compiler,
                                        basicBuiltInTypeInfos,
                                        CommonBuiltInFunctionCount,
                                        CommonBuiltInFunctions);

        if (gcmIS_ERROR(status)) break;

        /* Return */
        gcmVERIFY_OK(_DestroyBasicBuiltInTypeInfos(Compiler, basicBuiltInTypeInfos));

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    gcmVERIFY_OK(_DestroyBasicBuiltInTypeInfos(Compiler, basicBuiltInTypeInfos));

    gcmFOOTER();
    return status;
}

/* Built-In Variables */
typedef struct _slsBUILT_IN_VARIABLE_INFO
{
    gctCONST_STRING     symbol;

    gctCONST_STRING     implSymbol;

    sltQUALIFIER        implQualifier;
}
slsBUILT_IN_VARIABLE_INFO;

const slsBUILT_IN_VARIABLE_INFO BuiltInVariableInfos[] =
{
    /* Vertex Shader Variables */
    {"gl_Position",             "#Position",            slvQUALIFIER_VARYING_OUT},
    {"gl_PointSize",            "#PointSize",           slvQUALIFIER_VARYING_OUT},

    /* Fragment Shader Variables */
    {"gl_FragCoord",            "#Position",            slvQUALIFIER_VARYING_IN},
    {"gl_FrontFacing",          "#FrontFacing",         slvQUALIFIER_VARYING_IN},
    {"gl_FragColor",            "#Color",               slvQUALIFIER_FRAGMENT_OUT},
    {"gl_FragData",             "#Color",               slvQUALIFIER_FRAGMENT_OUT},
    {"gl_PointCoord",           "#PointCoord",          slvQUALIFIER_VARYING_IN},
    {"gl_FragDepthEXT",         "#Depth",               slvQUALIFIER_FRAGMENT_OUT},

    /* Built-In Uniforms */
    {"gl_DepthRange.near",      "#DepthRange.near",     slvQUALIFIER_UNIFORM},
    {"gl_DepthRange.far",       "#DepthRange.far",      slvQUALIFIER_UNIFORM},
    {"gl_DepthRange.diff",      "#DepthRange.diff",     slvQUALIFIER_UNIFORM}
};

const gctUINT BuiltInVariableCount =
                    sizeof(BuiltInVariableInfos) / sizeof(slsBUILT_IN_VARIABLE_INFO);

gceSTATUS
slGetBuiltInVariableImplSymbol(
    IN gctCONST_STRING Symbol,
    OUT gctCONST_STRING * ImplSymbol,
    OUT sltQUALIFIER * ImplQualifier
    )
{
    gctUINT     i;

    gcmHEADER_ARG("Symbol=0x%x ImplSymbol=0x%x ImplQualifier=0x%x",
                  Symbol, ImplSymbol, ImplQualifier);

    /* Verify the arguments. */
    gcmASSERT(Symbol);
    gcmASSERT(ImplSymbol);
    gcmASSERT(ImplQualifier);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "Symbol=%s", gcmOPT_STRING(Symbol));
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "ImplSymbol=%x", gcmOPT_POINTER(ImplSymbol));
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "*ImplQualifier=%u", gcmOPT_VALUE(ImplQualifier));

    for (i = 0; i < BuiltInVariableCount; i++)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(BuiltInVariableInfos[i].symbol, Symbol)))
        {
            *ImplSymbol     = BuiltInVariableInfos[i].implSymbol;
            *ImplQualifier  = BuiltInVariableInfos[i].implQualifier;
            break;
        }
    }

    gcmASSERT(i < BuiltInVariableCount);

    gcmFOOTER_ARG("*ImplSymbol=%u *ImplQualifier=%u", *ImplSymbol, *ImplQualifier);
    return gcvSTATUS_OK;
}

/* Built-In Functions */
gctBOOL
slIsTextureLookupFunction(
    IN gctCONST_STRING Symbol
    )
{
    gcmASSERT(Symbol);

    return (gcmIS_SUCCESS(gcoOS_StrNCmp(Symbol, "texture", 7)));
}

static gceSTATUS
_GenTexture2DCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2 || OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (OperandCount == 3)
    {
        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_TEXTURE_BIAS,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[2].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture2DLodCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[2].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture2DProjCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2 || OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (OperandCount == 3)
    {
        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_TEXTURE_BIAS,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[2].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD_PROJ,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture2DProjLodCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[2].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD_PROJ,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture3DCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2 || OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (OperandCount == 3)
    {
        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_TEXTURE_BIAS,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[2].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture3DLodCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[2].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture3DProjCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2 || OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (OperandCount == 3)
    {
        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_TEXTURE_BIAS,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[2].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD_PROJ,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture3DProjLodCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[2].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD_PROJ,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenAccessLayerCode(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN slsROPERAND *Coord,
    IN slsCOMPONENT_SELECTION ComponentSelection,
    OUT slsIOPERAND *Layer
)
{
    gceSTATUS status;
    slsROPERAND intermROperand[1];
    slsROPERAND constantHalf[1];
    slsROPERAND constantZero[1];

    gcmHEADER();

    sloIR_ROperandComponentSelect(Compiler,
                                  Coord,
                                  ComponentSelection,
                                  intermROperand);

    slsIOPERAND_New(Compiler, Layer, intermROperand->dataType);

    slsROPERAND_InitializeFloatOrVecOrMatConstant(constantHalf,
                                                  gcSHADER_FLOAT_X1,
						  (gctFLOAT)0.5);

    status = slGenArithmeticExprCode(Compiler,
                                     LineNo,
                                     StringNo,
                                     slvOPCODE_ADD,
                                     Layer,
                                     constantHalf,
                                     intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(intermROperand, Layer);
    status = slGenGenericCode1(Compiler,
                               LineNo,
                               StringNo,
                               slvOPCODE_FLOOR,
                               Layer,
                               intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeFloatOrVecOrMatConstant(constantZero,
                                                  gcSHADER_FLOAT_X1,
						  (gctFLOAT)0.0);
    status = slGenGenericCode2(Compiler,
                               LineNo,
                               StringNo,
                               slvOPCODE_MAX,
                               Layer,
                               constantZero,
                               intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture1DArrayCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;
    slsROPERAND intermROperand[1];
    slsLOPERAND intermLOperand[1];
    slsIOPERAND layerOperand[1];
    slsIOPERAND iOperand[1];
    slsROPERAND rOperand[1];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2 || OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (OperandCount == 3)
    {
        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_TEXTURE_BIAS,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[2].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = _GenAccessLayerCode(Compiler,
                                 PolynaryExpr->exprBase.base.lineNo,
                                 PolynaryExpr->exprBase.base.stringNo,
                                 &OperandsParameters[1].rOperands[0],
                                 ComponentSelection_Y,
                                 layerOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsIOPERAND_New(Compiler, iOperand, OperandsParameters[1].rOperands[0].dataType);
    slsLOPERAND_InitializeUsingIOperand(intermLOperand, iOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             &OperandsParameters[1].rOperands[0]);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    sloIR_LOperandComponentSelect(Compiler,
                                  intermLOperand,
                                  ComponentSelection_Y,
                                  intermLOperand);

    slsROPERAND_InitializeUsingIOperand(intermROperand, layerOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(rOperand, iOperand);
    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            rOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture1DArrayLodCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;
    slsROPERAND intermROperand[1];
    slsLOPERAND intermLOperand[1];
    slsIOPERAND layerOperand[1];
    slsIOPERAND iOperand[1];
    slsROPERAND rOperand[1];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[2].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _GenAccessLayerCode(Compiler,
                                 PolynaryExpr->exprBase.base.lineNo,
                                 PolynaryExpr->exprBase.base.stringNo,
                                 &OperandsParameters[1].rOperands[0],
                                 ComponentSelection_Y,
                                 layerOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsIOPERAND_New(Compiler, iOperand, OperandsParameters[1].rOperands[0].dataType);
    slsLOPERAND_InitializeUsingIOperand(intermLOperand, iOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             &OperandsParameters[1].rOperands[0]);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    sloIR_LOperandComponentSelect(Compiler,
                                  intermLOperand,
                                  ComponentSelection_Y,
                                  intermLOperand);

    slsROPERAND_InitializeUsingIOperand(intermROperand, layerOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(rOperand, iOperand);
    status = slGenGenericCode2(Compiler,
                               PolynaryExpr->exprBase.base.lineNo,
                               PolynaryExpr->exprBase.base.stringNo,
                               slvOPCODE_TEXTURE_LOAD,
                               IOperand,
                               &OperandsParameters[0].rOperands[0],
			       rOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture2DArrayCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;
    slsROPERAND intermROperand[1];
    slsLOPERAND intermLOperand[1];
    slsIOPERAND layerOperand[1];
    slsIOPERAND iOperand[1];
    slsROPERAND rOperand[1];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2 || OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (OperandCount == 3)
    {
        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_TEXTURE_BIAS,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[2].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = _GenAccessLayerCode(Compiler,
                                 PolynaryExpr->exprBase.base.lineNo,
                                 PolynaryExpr->exprBase.base.stringNo,
                                 &OperandsParameters[1].rOperands[0],
                                 ComponentSelection_Z,
                                 layerOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsIOPERAND_New(Compiler, iOperand, OperandsParameters[1].rOperands[0].dataType);
    slsLOPERAND_InitializeUsingIOperand(intermLOperand, iOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             &OperandsParameters[1].rOperands[0]);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    sloIR_LOperandComponentSelect(Compiler,
                                  intermLOperand,
                                  ComponentSelection_Z,
                                  intermLOperand);

    slsROPERAND_InitializeUsingIOperand(intermROperand, layerOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(rOperand, iOperand);
    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            rOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTexture2DArrayLodCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;
    slsROPERAND intermROperand[1];
    slsLOPERAND intermLOperand[1];
    slsIOPERAND layerOperand[1];
    slsIOPERAND iOperand[1];
    slsROPERAND rOperand[1];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[2].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _GenAccessLayerCode(Compiler,
                                 PolynaryExpr->exprBase.base.lineNo,
                                 PolynaryExpr->exprBase.base.stringNo,
                                 &OperandsParameters[1].rOperands[0],
                                 ComponentSelection_Z,
                                 layerOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsIOPERAND_New(Compiler, iOperand, OperandsParameters[1].rOperands[0].dataType);
    slsLOPERAND_InitializeUsingIOperand(intermLOperand, iOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             &OperandsParameters[1].rOperands[0]);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    sloIR_LOperandComponentSelect(Compiler,
                                  intermLOperand,
                                  ComponentSelection_Z,
                                  intermLOperand);

    slsROPERAND_InitializeUsingIOperand(intermROperand, layerOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(rOperand, iOperand);
    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            rOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenShadow1DArrayCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;
    slsROPERAND intermROperand[1];
    slsLOPERAND intermLOperand[1];
    slsIOPERAND layerOperand[1];
    slsIOPERAND iOperand[1];
    slsROPERAND rOperand[1];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2 || OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (OperandCount == 3)
    {
        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_TEXTURE_BIAS,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[2].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    status = _GenAccessLayerCode(Compiler,
                                 PolynaryExpr->exprBase.base.lineNo,
                                 PolynaryExpr->exprBase.base.stringNo,
                                 &OperandsParameters[1].rOperands[0],
                                 ComponentSelection_Y,
                                 layerOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsIOPERAND_New(Compiler, iOperand, OperandsParameters[1].rOperands[0].dataType);
    slsLOPERAND_InitializeUsingIOperand(intermLOperand, iOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             &OperandsParameters[1].rOperands[0]);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    sloIR_LOperandComponentSelect(Compiler,
                                  intermLOperand,
                                  ComponentSelection_Y,
                                  intermLOperand);

    slsROPERAND_InitializeUsingIOperand(intermROperand, layerOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(rOperand, iOperand);
    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            rOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenShadow1DArrayLodCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;
    slsROPERAND intermROperand[1];
    slsLOPERAND intermLOperand[1];
    slsIOPERAND layerOperand[1];
    slsIOPERAND iOperand[1];
    slsROPERAND rOperand[1];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[2].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = _GenAccessLayerCode(Compiler,
                                 PolynaryExpr->exprBase.base.lineNo,
                                 PolynaryExpr->exprBase.base.stringNo,
                                 &OperandsParameters[1].rOperands[0],
                                 ComponentSelection_Y,
                                 layerOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsIOPERAND_New(Compiler, iOperand, OperandsParameters[1].rOperands[0].dataType);
    slsLOPERAND_InitializeUsingIOperand(intermLOperand, iOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             &OperandsParameters[1].rOperands[0]);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    sloIR_LOperandComponentSelect(Compiler,
                                  intermLOperand,
                                  ComponentSelection_Y,
                                  intermLOperand);

    slsROPERAND_InitializeUsingIOperand(intermROperand, layerOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(rOperand, iOperand);
    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            rOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenShadow2DArrayCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;
    slsROPERAND intermROperand[1];
    slsLOPERAND intermLOperand[1];
    slsIOPERAND layerOperand[1];
    slsIOPERAND iOperand[1];
    slsROPERAND rOperand[1];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = _GenAccessLayerCode(Compiler,
                                 PolynaryExpr->exprBase.base.lineNo,
                                 PolynaryExpr->exprBase.base.stringNo,
                                 &OperandsParameters[1].rOperands[0],
                                 ComponentSelection_Z,
                                 layerOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsIOPERAND_New(Compiler, iOperand, OperandsParameters[1].rOperands[0].dataType);
    slsLOPERAND_InitializeUsingIOperand(intermLOperand, iOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             &OperandsParameters[1].rOperands[0]);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    sloIR_LOperandComponentSelect(Compiler,
                                  intermLOperand,
                                  ComponentSelection_Z,
                                  intermLOperand);

    slsROPERAND_InitializeUsingIOperand(intermROperand, layerOperand);
    status = slGenAssignCode(Compiler,
                             PolynaryExpr->exprBase.base.lineNo,
                             PolynaryExpr->exprBase.base.stringNo,
                             intermLOperand,
                             intermROperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(rOperand, iOperand);
    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TEXTURE_LOAD,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            rOperand);
    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTextureCubeCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    return _GenTexture2DCode(
                            Compiler,
                            CodeGenerator,
                            PolynaryExpr,
                            OperandCount,
                            OperandsParameters,
                            IOperand);
}

static gceSTATUS
_GenTextureCubeLodCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    return _GenTexture2DLodCode(
                                Compiler,
                                CodeGenerator,
                                PolynaryExpr,
                                OperandCount,
                                OperandsParameters,
                                IOperand);
}

static gceSTATUS
_GenSinCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_SIN,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenCosCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_COS,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenTanCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_TAN,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenAsinCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_ASIN,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenAcosCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_ACOS,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenAtanCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (OperandCount == 1)
    {
        status = slGenGenericCode1(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_ATAN,
                                IOperand,
                                &OperandsParameters[0].rOperands[0]);
    }
    else
    {
        gcmASSERT(OperandCount == 2);

        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_ATAN2,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[1].rOperands[0]);
    }

    gcmFOOTER();
    return status;
}

static gceSTATUS
_GenRadiansCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS           status;
    slsROPERAND         constantROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);
    gcmASSERT(gcGetComponentDataType(IOperand->dataType) == gcSHADER_FLOAT_X1);

    /* mul result, degrees, _PI / 180 */
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &constantROperand,
                                                gcSHADER_FLOAT_X1,
                                                _PI / (gctFLOAT)180.0);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &constantROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenDegreesCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS           status;
    slsROPERAND         constantROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);
    gcmASSERT(gcGetComponentDataType(IOperand->dataType) == gcSHADER_FLOAT_X1);

    /* mul result, radians, 180 / _PI */
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &constantROperand,
                                                gcSHADER_FLOAT_X1,
                                                (gctFLOAT)180.0 / _PI);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &constantROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenPow0Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS           status;
    slsLOPERAND         lOperand;
    slsROPERAND         constantROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);
    gcmASSERT(gcGetComponentDataType(IOperand->dataType) == gcSHADER_FLOAT_X1);

    /* mov result, 1.0 */
    slsLOPERAND_InitializeUsingIOperand(&lOperand, IOperand);
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &constantROperand,
                                                IOperand->dataType,
                                                (gctFLOAT)1.0);

    status = slGenAssignCode(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            &lOperand,
                            &constantROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenPow1Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS           status;
    slsLOPERAND         lOperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* mov result, x */
    slsLOPERAND_InitializeUsingIOperand(&lOperand, IOperand);

    status = slGenAssignCode(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            &lOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenPow2Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS           status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* mul result, x, x */
    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenPow3Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperand;
    slsROPERAND     intermROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* mul t0, x, x */
    slsIOPERAND_New(Compiler, &intermIOperand, OperandsParameters[0].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul result, t0, x */
    slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &intermROperand,
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenPow4Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperand;
    slsROPERAND     intermROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* mul t0, x, x */
    slsIOPERAND_New(Compiler, &intermIOperand, OperandsParameters[0].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul result, t0, t0 */
    slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &intermROperand,
                                    &intermROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenPow5Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperands[2];
    slsROPERAND     intermROperands[2];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* mul t0, x, x */
    slsIOPERAND_New(Compiler, &intermIOperands[0], OperandsParameters[0].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[0],
                                    &OperandsParameters[0].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul t1, t0, t0 */
    slsIOPERAND_New(Compiler, &intermIOperands[1], OperandsParameters[0].dataTypes[0]);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[0], &intermIOperands[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[1],
                                    &intermROperands[0],
                                    &intermROperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul result, t1, x */
    slsROPERAND_InitializeUsingIOperand(&intermROperands[1], &intermIOperands[1]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &intermROperands[1],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenPow6Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperands[2];
    slsROPERAND     intermROperands[2];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* mul t0, x, x */
    slsIOPERAND_New(Compiler, &intermIOperands[0], OperandsParameters[0].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[0],
                                    &OperandsParameters[0].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul t1, t0, t0 */
    slsIOPERAND_New(Compiler, &intermIOperands[1], OperandsParameters[0].dataTypes[0]);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[0], &intermIOperands[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[1],
                                    &intermROperands[0],
                                    &intermROperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul result, t1, t0 */
    slsROPERAND_InitializeUsingIOperand(&intermROperands[1], &intermIOperands[1]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &intermROperands[1],
                                    &intermROperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenPow8Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND *   intermIOperands;
    slsROPERAND *   intermROperands;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

	status = gcoOS_Allocate(gcvNULL,
			                        2 * sizeof(slsIOPERAND),
									(gctPOINTER *)&intermIOperands);
	if (gcmIS_ERROR(status))
	{
        gcmFOOTER();
		return status;
	}

	status = gcoOS_Allocate(gcvNULL,
			                        2 * sizeof(slsROPERAND),
									(gctPOINTER *)&intermROperands);
	if (gcmIS_ERROR(status))
	{
		gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, intermIOperands));
        gcmFOOTER();
		return status;
	}

    /* mul t0, x, x */
    slsIOPERAND_New(Compiler, intermIOperands, OperandsParameters[0].dataTypes[0]);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    intermIOperands,
                                    &OperandsParameters[0].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]));

    /* mul t1, t0, t0 */
    slsIOPERAND_New(Compiler, intermIOperands + 1, OperandsParameters[0].dataTypes[0]);
    slsROPERAND_InitializeUsingIOperand(intermROperands, intermIOperands);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    intermIOperands + 1,
                                    intermROperands,
                                    intermROperands));

    /* mul result, t1, t1 */
    slsROPERAND_InitializeUsingIOperand(intermROperands + 1, intermIOperands + 1);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    intermROperands + 1,
                                    intermROperands + 1));

	gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, intermIOperands));
	gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, intermROperands));

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
	gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, intermIOperands));
	gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, intermROperands));
    /* Return the status. */
    gcmFOOTER();
    return status;

}

typedef gceSTATUS
(* sltGEN_POW_N_CODE_FUNC_PTR)(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    );

#define POW_N_COUNT     9

const sltGEN_POW_N_CODE_FUNC_PTR        GenPowNCodeTable[POW_N_COUNT] =
{
    _GenPow0Code,
    _GenPow1Code,
    _GenPow2Code,
    _GenPow3Code,
    _GenPow4Code,
    _GenPow5Code,
    _GenPow6Code,
    gcvNULL,
    _GenPow8Code
};

static gceSTATUS
_GenPowCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS                   status;
    gctUINT                     i;
    sltGEN_POW_N_CODE_FUNC_PTR  genCode = gcvNULL;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (sloCOMPILER_OptimizationEnabled(Compiler, slvOPTIMIZATION_CALCULATION))
    {
        for (i = 0; i < POW_N_COUNT; i++)
        {
            if (slsROPERAND_IsFloatOrVecConstant(&OperandsParameters[1].rOperands[0], (gctFLOAT)i))
            {
                genCode = GenPowNCodeTable[i];
                break;
            }
        }
    }

    if (genCode != gcvNULL)
    {
        status = (*genCode)(
                            Compiler,
                            CodeGenerator,
                            PolynaryExpr,
                            OperandCount,
                            OperandsParameters,
                            IOperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_POW,
                                IOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[1].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#define _LOG2_E             ((float)1.44269504088896340736)
#define _RCP_OF_LOG2_E      ((float)0.69314718055994530942)

static gceSTATUS
_GenExpCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsROPERAND     constantROperand;
    slsIOPERAND     intermIOperand;
    slsROPERAND     intermROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* mul t0, x, _LOG2_E */
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &constantROperand,
                                                gcSHADER_FLOAT_X1,
                                                _LOG2_E);

    slsIOPERAND_New(Compiler, &intermIOperand, OperandsParameters[0].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &constantROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* exp2 result, t0 */
    slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_EXP2,
                            IOperand,
                            &intermROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenLogCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsROPERAND     constantROperand;
    slsIOPERAND     intermIOperand;
    slsROPERAND     intermROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* log2 t0, x */
    slsIOPERAND_New(Compiler, &intermIOperand, OperandsParameters[0].dataTypes[0]);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_LOG2,
                            &intermIOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul result, t0, _RCP_OF_LOG2_E */
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &constantROperand,
                                                gcSHADER_FLOAT_X1,
                                                _RCP_OF_LOG2_E);

    slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &intermROperand,
                                    &constantROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenExp2Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_EXP2,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenLog2Code(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_LOG2,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenSqrtCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_SQRT,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenInverseSqrtCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_INVERSE_SQRT,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenAbsCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_ABS,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenSignCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_SIGN,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenFloorCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_FLOOR,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenCeilCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_CEIL,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenFractCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_FRACT,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gctFLOAT
_Mod(
     IN gctFLOAT Operand0,
     IN gctFLOAT Operand1
     )
{
    return Operand0 - Operand1 * (gctFLOAT)(gctINT)(Operand0 / Operand1);
}

static gceSTATUS
_EvaluateMod(
    IN sloCOMPILER Compiler,
    IN gctUINT OperandCount,
    IN sloIR_CONSTANT * OperandConstants,
    IN OUT sloIR_CONSTANT ResultConstant
    )
{
    gceSTATUS           status;
    gctUINT             i, componentCount;
    sluCONSTANT_VALUE   values[4];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandConstants);

    gcmASSERT(slsDATA_TYPE_IsFloatOrVec(OperandConstants[0]->exprBase.dataType));
    gcmASSERT(slsDATA_TYPE_IsEqual(
                                ResultConstant->exprBase.dataType,
                                OperandConstants[0]->exprBase.dataType));

    componentCount = (slmDATA_TYPE_vectorSize_GET(OperandConstants[0]->exprBase.dataType) == 0) ?
                        1 : slmDATA_TYPE_vectorSize_NOCHECK_GET(OperandConstants[0]->exprBase.dataType);

    for (i = 0; i < componentCount; i++)
    {
        if (slsDATA_TYPE_IsFloat(OperandConstants[1]->exprBase.dataType))
        {
            values[i].floatValue = _Mod(
                                        OperandConstants[0]->values[i].floatValue,
                                        OperandConstants[1]->values[0].floatValue);
        }
        else
        {
            values[i].floatValue = _Mod(
                                        OperandConstants[0]->values[i].floatValue,
                                        OperandConstants[1]->values[i].floatValue);
        }
    }

    status = sloIR_CONSTANT_AddValues(
                                    Compiler,
                                    ResultConstant,
                                    componentCount,
                                    values);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenModCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperands[3];
    slsROPERAND     intermROperands[3];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* div t0, x (operand0), y (operand1) */
    slsIOPERAND_New(Compiler, &intermIOperands[0], OperandsParameters[0].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_DIV,
                                    &intermIOperands[0],
                                    &OperandsParameters[0].rOperands[0],
                                    &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* floor t1, t0 */
    slsIOPERAND_New(Compiler, &intermIOperands[1], intermIOperands[0].dataType);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[0], &intermIOperands[0]);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_FLOOR,
                            &intermIOperands[1],
                            &intermROperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul t2, y (operand1), t1 */
    slsIOPERAND_New(Compiler, &intermIOperands[2], intermIOperands[1].dataType);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[1], &intermIOperands[1]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[2],
                                    &OperandsParameters[1].rOperands[0],
                                    &intermROperands[1]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* sub result, x (operand0), t2 */
    slsROPERAND_InitializeUsingIOperand(&intermROperands[2], &intermIOperands[2]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    IOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &intermROperands[2]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenMinCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_MIN,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenMaxCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_MAX,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenClampCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperand;
    slsROPERAND     intermROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (sloCOMPILER_OptimizationEnabled(Compiler, slvOPTIMIZATION_CALCULATION)
        && slsROPERAND_IsFloatOrVecConstant(&OperandsParameters[1].rOperands[0], 0.0)
        && slsROPERAND_IsFloatOrVecConstant(&OperandsParameters[2].rOperands[0], 1.0))
    {
        status = slGenGenericCode1(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_SATURATE,
                                IOperand,
                                &OperandsParameters[0].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        /* max t0, opd0, opd1 */
        slsIOPERAND_New(Compiler, &intermIOperand, IOperand->dataType);

        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_MAX,
                                &intermIOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[1].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* min result, t0, opd2 */
        slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_MIN,
                                IOperand,
                                &intermROperand,
                                &OperandsParameters[2].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenMixCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperands[2];
    slsROPERAND     intermROperands[2];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* sub t0, y, x */
    slsIOPERAND_New(Compiler, &intermIOperands[0], OperandsParameters[0].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    &intermIOperands[0],
                                    &OperandsParameters[1].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul t1, a, t0 */
    slsIOPERAND_New(Compiler, &intermIOperands[1], OperandsParameters[0].dataTypes[0]);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[0], &intermIOperands[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[1],
                                    &OperandsParameters[2].rOperands[0],
                                    &intermROperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* add result, t1, x */
    slsROPERAND_InitializeUsingIOperand(&intermROperands[1], &intermIOperands[1]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_ADD,
                                    IOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &intermROperands[1]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenStepCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_STEP,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenSmoothStepCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperands[7];
    slsROPERAND     intermROperands[7];
    slsROPERAND     constantROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* sub t0, x (operand2), edge0 (operand0) */
    slsIOPERAND_New(Compiler, &intermIOperands[0], OperandsParameters[2].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    &intermIOperands[0],
                                    &OperandsParameters[2].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* sub t1, edge1 (operand1), edge0 (operand0) */
    slsIOPERAND_New(Compiler, &intermIOperands[1], OperandsParameters[1].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    &intermIOperands[1],
                                    &OperandsParameters[1].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* div t2, t0, t1 */
    slsIOPERAND_New(Compiler, &intermIOperands[2], intermIOperands[0].dataType);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[0], &intermIOperands[0]);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[1], &intermIOperands[1]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_DIV,
                                    &intermIOperands[2],
                                    &intermROperands[0],
                                    &intermROperands[1]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* sat t3, t2 */
    slsIOPERAND_New(Compiler, &intermIOperands[3], intermIOperands[2].dataType);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[2], &intermIOperands[2]);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_SATURATE,
                            &intermIOperands[3],
                            &intermROperands[2]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul t4, t3, t3 */
    slsIOPERAND_New(Compiler, &intermIOperands[4], intermIOperands[3].dataType);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[3], &intermIOperands[3]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[4],
                                    &intermROperands[3],
                                    &intermROperands[3]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* add t5, t3, t3   ==> mul t5, t3, 2 */
    slsIOPERAND_New(Compiler, &intermIOperands[5], intermIOperands[3].dataType);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_ADD,
                                    &intermIOperands[5],
                                    &intermROperands[3],
                                    &intermROperands[3]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* sub t6, 3.0, t5 */
    slsIOPERAND_New(Compiler, &intermIOperands[6], intermIOperands[5].dataType);
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &constantROperand,
                                                gcSHADER_FLOAT_X1,
                                                (gctFLOAT)3.0);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[5], &intermIOperands[5]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    &intermIOperands[6],
                                    &constantROperand,
                                    &intermROperands[5]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul result, t4, t6 */
    slsROPERAND_InitializeUsingIOperand(&intermROperands[4], &intermIOperands[4]);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[6], &intermIOperands[6]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    IOperand,
                                    &intermROperands[4],
                                    &intermROperands[6]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenLengthCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperand;
    slsROPERAND     intermROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    if (gcIsScalarDataType(OperandsParameters[0].dataTypes[0]))
    {
        /* abs result, x */
        status = slGenGenericCode1(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_ABS,
                                IOperand,
                                &OperandsParameters[0].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        /* dot t0, x, x */
        slsIOPERAND_New(Compiler, &intermIOperand, gcSHADER_FLOAT_X1);

        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_DOT,
                                &intermIOperand,
                                &OperandsParameters[0].rOperands[0],
                                &OperandsParameters[0].rOperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* sqrt result, t0 */
        slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);

        status = slGenGenericCode1(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_SQRT,
                                IOperand,
                                &intermROperand);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenDistanceCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperands[2];
    slsROPERAND     intermROperands[2];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* sub t0, p0, p1 */
    slsIOPERAND_New(Compiler, &intermIOperands[0], OperandsParameters[0].dataTypes[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    &intermIOperands[0],
                                    &OperandsParameters[0].rOperands[0],
                                    &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    slsROPERAND_InitializeUsingIOperand(&intermROperands[0], &intermIOperands[0]);

    if (gcIsScalarDataType(OperandsParameters[0].dataTypes[0]))
    {
        /* abs result, t0 */
        status = slGenGenericCode1(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_ABS,
                                IOperand,
                                &intermROperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }
    else
    {
        /* dot t1, t0, t0 */
        slsIOPERAND_New(Compiler, &intermIOperands[1], gcSHADER_FLOAT_X1);

        status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_DOT,
                                &intermIOperands[1],
                                &intermROperands[0],
                                &intermROperands[0]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        /* sqrt result, t1 */
        slsROPERAND_InitializeUsingIOperand(&intermROperands[1], &intermIOperands[1]);

        status = slGenGenericCode1(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_SQRT,
                                IOperand,
                                &intermROperands[1]);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenDotCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_DOT,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenCrossCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_CROSS,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenNormalizeCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_NORMALIZE,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenFaceForwardCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS               status;
    slsIOPERAND             intermIOperand;
    slsROPERAND             intermROperand;
    slsLOPERAND             intermLOperand;
    slsSELECTION_CONTEXT    selectionContext;
    slsROPERAND             zeroROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* dot t0, Nref (operand2), I (operand1) */
    slsIOPERAND_New(Compiler, &intermIOperand, gcSHADER_FLOAT_X1);

    status = slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_DOT,
                                &intermIOperand,
                                &OperandsParameters[2].rOperands[0],
                                &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The selection begin*/
    status = slDefineSelectionBegin(
                                    Compiler,
                                    CodeGenerator,
                                    gcvTRUE,
                                    &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The condition part: t0 < 0.0 */
    slsROPERAND_InitializeUsingIOperand(&intermROperand, &intermIOperand);
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &zeroROperand,
                                                gcSHADER_FLOAT_X1,
                                                (gctFLOAT)0.0);

    status = slGenSelectionCompareConditionCode(
                                                Compiler,
                                                CodeGenerator,
                                                &selectionContext,
                                                PolynaryExpr->exprBase.base.lineNo,
                                                PolynaryExpr->exprBase.base.stringNo,
                                                slvCONDITION_LESS_THAN,
                                                &intermROperand,
                                                &zeroROperand);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The true part */
    status = slDefineSelectionTrueOperandBegin(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mov result, N (operand0) */
    slsLOPERAND_InitializeUsingIOperand(&intermLOperand, IOperand);

    status = slGenAssignCode(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            &intermLOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }


    status = slDefineSelectionTrueOperandEnd(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext,
                                            gcvFALSE);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The false part */
    status = slDefineSelectionFalseOperandBegin(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* sub result, 0.0, N (operand0) */
    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    IOperand,
                                    &zeroROperand,
                                    &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    status = slDefineSelectionFalseOperandEnd(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* The selection end */
    status = slDefineSelectionEnd(
                                Compiler,
                                CodeGenerator,
                                &selectionContext);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenReflectCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    slsIOPERAND     intermIOperands[3];
    slsROPERAND     intermROperands[3];

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    /* dot t0, N (operand1), I (operand0) */
    slsIOPERAND_New(Compiler, &intermIOperands[0], gcSHADER_FLOAT_X1);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_DOT,
                            &intermIOperands[0],
                            &OperandsParameters[1].rOperands[0],
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* add t1, t0, t0   ==> mul t1, 2.0, t0 */
    slsIOPERAND_New(Compiler, &intermIOperands[1], gcSHADER_FLOAT_X1);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[0], &intermIOperands[0]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_ADD,
                                    &intermIOperands[1],
                                    &intermROperands[0],
                                    &intermROperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* mul t2, t1, N (operand1) */
    slsIOPERAND_New(Compiler, &intermIOperands[2], OperandsParameters[1].dataTypes[0]);
    slsROPERAND_InitializeUsingIOperand(&intermROperands[1], &intermIOperands[1]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    &intermIOperands[2],
                                    &intermROperands[1],
                                    &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* sub result, I (operand0), t2 */
    slsROPERAND_InitializeUsingIOperand(&intermROperands[2], &intermIOperands[2]);

    status = slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    IOperand,
                                    &OperandsParameters[0].rOperands[0],
                                    &intermROperands[2]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenRefractCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS               status;
    slsIOPERAND *           intermIOperands = gcvNULL;
    slsROPERAND *           intermROperands = gcvNULL;
    slsLOPERAND             intermLOperand;
    slsSELECTION_CONTEXT    selectionContext;
    slsROPERAND             oneROperand, zeroROperand;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 3);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    gcmONERROR(gcoOS_Allocate(gcvNULL, 11 * sizeof(slsIOPERAND), (gctPOINTER*)&intermIOperands));
	gcmONERROR(gcoOS_Allocate(gcvNULL, 11 * sizeof(slsROPERAND), (gctPOINTER*)&intermROperands));

    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &oneROperand,
                                                gcSHADER_FLOAT_X1,
                                                (gctFLOAT)1.0);

    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &zeroROperand,
                                                gcSHADER_FLOAT_X1,
                                                (gctFLOAT)0.0);

    /* dot t0, N (operand1), I (operand0) */
    slsIOPERAND_New(Compiler, intermIOperands, gcSHADER_FLOAT_X1);

    gcmONERROR(slGenGenericCode2(
                                Compiler,
                                PolynaryExpr->exprBase.base.lineNo,
                                PolynaryExpr->exprBase.base.stringNo,
                                slvOPCODE_DOT,
                                intermIOperands,
                                &OperandsParameters[1].rOperands[0],
                                &OperandsParameters[0].rOperands[0]));

    /* mul t1, t0, t0 */
    slsIOPERAND_New(Compiler, intermIOperands + 1, gcSHADER_FLOAT_X1);
    slsROPERAND_InitializeUsingIOperand(intermROperands, intermIOperands);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    intermIOperands + 1,
                                    intermROperands,
                                    intermROperands));

    /* sub t2, 1.0, t1 */
    slsIOPERAND_New(Compiler, intermIOperands + 2, gcSHADER_FLOAT_X1);
    slsROPERAND_InitializeUsingIOperand(intermROperands + 1, intermIOperands + 1);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    intermIOperands + 2,
                                    &oneROperand,
                                    intermROperands + 1));

    /* mul t3, eta (operand2), eta (operand2) */
    slsIOPERAND_New(Compiler, intermIOperands + 3, gcSHADER_FLOAT_X1);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    intermIOperands + 3,
                                    &OperandsParameters[2].rOperands[0],
                                    &OperandsParameters[2].rOperands[0]));

    /* mul t4, t3, t2 */
    slsIOPERAND_New(Compiler, intermIOperands + 4, gcSHADER_FLOAT_X1);
    slsROPERAND_InitializeUsingIOperand(intermROperands + 3, intermIOperands + 3);
    slsROPERAND_InitializeUsingIOperand(intermROperands + 2, intermIOperands + 2);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    intermIOperands + 4,
                                    intermROperands + 3,
                                    intermROperands + 2));

    /* sub t5, 1.0, t4 */
    slsIOPERAND_New(Compiler, intermIOperands + 5, gcSHADER_FLOAT_X1);
    slsROPERAND_InitializeUsingIOperand(intermROperands + 4, intermIOperands + 4);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    intermIOperands + 5,
                                    &oneROperand,
                                    intermROperands + 4));

    /* The selection begin*/
    gcmONERROR(slDefineSelectionBegin(
                                    Compiler,
                                    CodeGenerator,
                                    gcvTRUE,
                                    &selectionContext));

    /* The condition part: t5 < 0.0 */
    slsROPERAND_InitializeUsingIOperand(intermROperands + 5, intermIOperands + 5);

    gcmONERROR(slGenSelectionCompareConditionCode(
                                                Compiler,
                                                CodeGenerator,
                                                &selectionContext,
                                                PolynaryExpr->exprBase.base.lineNo,
                                                PolynaryExpr->exprBase.base.stringNo,
                                                slvCONDITION_LESS_THAN,
                                                intermROperands + 5,
                                                &zeroROperand));

    /* The true part */
    gcmONERROR(slDefineSelectionTrueOperandBegin(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext));

    /* mov result, 0.0 */
    slsLOPERAND_InitializeUsingIOperand(&intermLOperand, IOperand);
    slsROPERAND_InitializeFloatOrVecOrMatConstant(
                                                &zeroROperand,
                                                IOperand->dataType,
                                                (gctFLOAT)0.0);

    gcmONERROR(slGenAssignCode(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            &intermLOperand,
                            &zeroROperand));

    gcmONERROR(slDefineSelectionTrueOperandEnd(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext,
                                            gcvFALSE));

    /* The false part */
    gcmONERROR(slDefineSelectionFalseOperandBegin(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext));

    /* mul t6, eta (operand2), I (operand0) */
    slsIOPERAND_New(Compiler, intermIOperands + 6, OperandsParameters[0].dataTypes[0]);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    intermIOperands + 6,
                                    &OperandsParameters[2].rOperands[0],
                                    &OperandsParameters[0].rOperands[0]));

    /* mul t7, eta (operand2), t0 */
    slsIOPERAND_New(Compiler, intermIOperands + 7, gcSHADER_FLOAT_X1);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    intermIOperands + 7,
                                    &OperandsParameters[2].rOperands[0],
                                    intermROperands));

    /* sqrt t8, t5 */
    slsIOPERAND_New(Compiler, intermIOperands + 8, gcSHADER_FLOAT_X1);

    gcmONERROR(slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_SQRT,
                            intermIOperands + 8,
                            intermROperands + 5));

    /* add t9, t7, t8 */
    slsIOPERAND_New(Compiler, intermIOperands + 9, gcSHADER_FLOAT_X1);
    slsROPERAND_InitializeUsingIOperand(intermROperands + 7, intermIOperands + 7);
    slsROPERAND_InitializeUsingIOperand(intermROperands + 8, intermIOperands + 8);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_ADD,
                                    intermIOperands + 9,
                                    intermROperands + 7,
                                    intermROperands + 8));

    /* mul t10, t9, N (operand1) */
    slsIOPERAND_New(Compiler, intermIOperands + 10, OperandsParameters[1].dataTypes[0]);
    slsROPERAND_InitializeUsingIOperand(intermROperands + 9, intermIOperands + 9);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_MUL,
                                    intermIOperands + 10,
                                    intermROperands + 9,
                                    &OperandsParameters[1].rOperands[0]));

    /* sub result, t6, t10 */
    slsROPERAND_InitializeUsingIOperand(intermROperands + 6, intermIOperands + 6);
    slsROPERAND_InitializeUsingIOperand(intermROperands + 10, intermIOperands + 10);

    gcmONERROR(slGenArithmeticExprCode(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    slvOPCODE_SUB,
                                    IOperand,
                                    intermROperands + 6,
                                    intermROperands + 10));

    gcmONERROR(slDefineSelectionFalseOperandEnd(
                                            Compiler,
                                            CodeGenerator,
                                            &selectionContext));

    /* The selection end */
    gcmONERROR(slDefineSelectionEnd(
                                Compiler,
                                CodeGenerator,
                                &selectionContext));

    if (intermIOperands != gcvNULL)
	{
		gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, intermIOperands));
	}
	if (intermROperands != gcvNULL)
	{
		gcmONERROR(gcmOS_SAFE_FREE(gcvNULL, intermROperands));
	}

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (intermIOperands != gcvNULL)
	{
		gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, intermIOperands));
	}
	if (intermROperands != gcvNULL)
	{
		gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, intermROperands));
	}
    gcmFOOTER();
	return status;
}

static gceSTATUS
_GenMatrixCompMultCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS       status;
    gctUINT         i;
    slsIOPERAND     columnIOperand;
    slsROPERAND     columnROperand0, columnROperand1;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    for (i = 0; i < gcGetMatrixDataTypeColumnCount(IOperand->dataType); i++)
    {
        slsIOPERAND_InitializeAsMatrixColumn(
                                            &columnIOperand,
                                            IOperand,
                                            i);

        slsROPERAND_InitializeAsMatrixColumn(
                                            &columnROperand0,
                                            &OperandsParameters[0].rOperands[0],
                                            i);

        slsROPERAND_InitializeAsMatrixColumn(
                                            &columnROperand1,
                                            &OperandsParameters[1].rOperands[0],
                                            i);

        status = slGenArithmeticExprCode(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        slvOPCODE_MUL,
                                        &columnIOperand,
                                        &columnROperand0,
                                        &columnROperand1);

        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenLessThanCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_LESS_THAN,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenLessThanEqualCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_LESS_THAN_EQUAL,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenGreaterThanCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_GREATER_THAN,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenGreaterThanEqualCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_GREATER_THAN_EQUAL,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenEqualCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_EQUAL,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenNotEqualCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 2);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode2(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_NOT_EQUAL,
                            IOperand,
                            &OperandsParameters[0].rOperands[0],
                            &OperandsParameters[1].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenAnyCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_ANY,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenAllCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_ALL,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenNotCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_NOT,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenDFdxCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_DFDX,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenDFdyCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_DFDY,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_GenFwidthCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    )
{
    gceSTATUS   status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(OperandCount == 1);
    gcmASSERT(OperandsParameters);
    gcmASSERT(IOperand);

    status = slGenGenericCode1(
                            Compiler,
                            PolynaryExpr->exprBase.base.lineNo,
                            PolynaryExpr->exprBase.base.stringNo,
                            slvOPCODE_FWIDTH,
                            IOperand,
                            &OperandsParameters[0].rOperands[0]);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

typedef gceSTATUS
(* sltBUILT_IN_EVALUATE_FUNC_PTR)(
    IN sloCOMPILER Compiler,
    IN gctUINT OperandCount,
    IN sloIR_CONSTANT * OperandConstants,
    IN OUT sloIR_CONSTANT ResultConstant
    );

typedef gceSTATUS
(* sltBUILT_IN_GEN_CODE_FUNC_PTR)(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand
    );

typedef struct _slsBUILT_IN_FUNCTION_INFO
{
    gctCONST_STRING                 symbol;

    gctBOOL                         treatFloatAsInt;

    sltBUILT_IN_EVALUATE_FUNC_PTR   evaluate;

    sltBUILT_IN_GEN_CODE_FUNC_PTR   genCode;
}
slsBUILT_IN_FUNCTION_INFO;

const slsBUILT_IN_FUNCTION_INFO     BuiltInFunctionInfos[] =
{
    /* Texture Lookup Functions */
    {"texture2DLod",        gcvFALSE,   gcvNULL,                    _GenTexture2DLodCode},
    {"texture2DProjLod",    gcvFALSE,   gcvNULL,                    _GenTexture2DProjLodCode},
    {"textureCubeLod",      gcvFALSE,   gcvNULL,                    _GenTextureCubeLodCode},

    {"texture2D",           gcvFALSE,   gcvNULL,                    _GenTexture2DCode},
    {"texture2DProj",       gcvFALSE,   gcvNULL,                    _GenTexture2DProjCode},
    {"textureCube",         gcvFALSE,   gcvNULL,                    _GenTextureCubeCode},

    {"texture3DLod",        gcvFALSE,   gcvNULL,                    _GenTexture3DLodCode},
    {"texture3DProjLod",    gcvFALSE,   gcvNULL,                    _GenTexture3DProjLodCode},

    {"texture3D",           gcvFALSE,   gcvNULL,                    _GenTexture3DCode},
    {"texture3DProj",       gcvFALSE,   gcvNULL,                    _GenTexture3DProjCode},

    {"texture1DArray",      gcvFALSE,   gcvNULL,                    _GenTexture1DArrayCode},
    {"texture2DArray",      gcvFALSE,   gcvNULL,                    _GenTexture2DArrayCode},
    {"shadow1DArray",       gcvFALSE,   gcvNULL,                    _GenShadow1DArrayCode},
    {"shadow2DArray",       gcvFALSE,   gcvNULL,                    _GenShadow2DArrayCode},

    {"texture1DArrayLod",   gcvFALSE,   gcvNULL,                    _GenTexture1DArrayLodCode},
    {"texture2DArrayLod",   gcvFALSE,   gcvNULL,                    _GenTexture2DArrayLodCode},
    {"shadow1DArrayLod",    gcvFALSE,   gcvNULL,                    _GenShadow1DArrayLodCode},

    /* Angle and Trigonometry Functions */
    {"radians",             gcvFALSE,   gcvNULL,                    _GenRadiansCode},
    {"degrees",             gcvFALSE,   gcvNULL,                    _GenDegreesCode},
    {"sin",                 gcvFALSE,   gcvNULL,                    _GenSinCode},
    {"cos",                 gcvFALSE,   gcvNULL,                    _GenCosCode},
    {"tan",                 gcvFALSE,   gcvNULL,                    _GenTanCode},
    {"asin",                gcvFALSE,   gcvNULL,                    _GenAsinCode},
    {"acos",                gcvFALSE,   gcvNULL,                    _GenAcosCode},
    {"atan",                gcvFALSE,   gcvNULL,                    _GenAtanCode},

    /* Exponential Functions */
    {"pow",                 gcvFALSE,   gcvNULL,                    _GenPowCode},
    {"exp",                 gcvFALSE,   gcvNULL,                    _GenExpCode},
    {"log",                 gcvFALSE,   gcvNULL,                    _GenLogCode},
    {"exp2",                gcvFALSE,   gcvNULL,                    _GenExp2Code},
    {"log2",                gcvFALSE,   gcvNULL,                    _GenLog2Code},
    {"sqrt",                gcvFALSE,   gcvNULL,                    _GenSqrtCode},
    {"inversesqrt",         gcvFALSE,   gcvNULL,                    _GenInverseSqrtCode},

    /* Common Functions */
    {"abs",                 gcvFALSE,   gcvNULL,                    _GenAbsCode},
    {"sign",                gcvTRUE,    gcvNULL,                    _GenSignCode},
    {"floor",               gcvTRUE,    gcvNULL,                    _GenFloorCode},
    {"ceil",                gcvTRUE,    gcvNULL,                    _GenCeilCode},
    {"fract",               gcvFALSE,   gcvNULL,                    _GenFractCode},
    {"mod",                 gcvFALSE,   _EvaluateMod,               _GenModCode},
    {"min",                 gcvFALSE,   gcvNULL,                    _GenMinCode},
    {"max",                 gcvFALSE,   gcvNULL,                    _GenMaxCode},
    {"clamp",               gcvFALSE,   gcvNULL,                    _GenClampCode},
    {"mix",                 gcvFALSE,   gcvNULL,                    _GenMixCode},
    {"step",                gcvTRUE,    gcvNULL,                    _GenStepCode},
    {"smoothstep",          gcvFALSE,   gcvNULL,                    _GenSmoothStepCode},

    /* Geometric Functions */
    {"length",              gcvFALSE,   gcvNULL,                    _GenLengthCode},
    {"distance",            gcvFALSE,   gcvNULL,                    _GenDistanceCode},
    {"dot",                 gcvFALSE,   gcvNULL,                    _GenDotCode},
    {"cross",               gcvFALSE,   gcvNULL,                    _GenCrossCode},
    {"normalize",           gcvFALSE,   gcvNULL,                    _GenNormalizeCode},
    {"faceforward",         gcvFALSE,   gcvNULL,                    _GenFaceForwardCode},
    {"reflect",             gcvFALSE,   gcvNULL,                    _GenReflectCode},
    {"refract",             gcvFALSE,   gcvNULL,                    _GenRefractCode},

    /* Matrix Functions */
    {"matrixCompMult",      gcvFALSE,   gcvNULL,                    _GenMatrixCompMultCode},

    /* Vector Relational Functions */
    {"lessThan",            gcvFALSE,   gcvNULL,                    _GenLessThanCode},
    {"lessThanEqual",       gcvFALSE,   gcvNULL,                    _GenLessThanEqualCode},
    {"greaterThan",         gcvFALSE,   gcvNULL,                    _GenGreaterThanCode},
    {"greaterThanEqual",    gcvFALSE,   gcvNULL,                    _GenGreaterThanEqualCode},
    {"equal",               gcvFALSE,   gcvNULL,                    _GenEqualCode},
    {"notEqual",            gcvFALSE,   gcvNULL,                    _GenNotEqualCode},
    {"any",                 gcvFALSE,   gcvNULL,                    _GenAnyCode},
    {"all",                 gcvFALSE,   gcvNULL,                    _GenAllCode},
    {"not",                 gcvFALSE,   gcvNULL,                    _GenNotCode},

    {"dFdx",                gcvFALSE,   gcvNULL,                    _GenDFdxCode},
    {"dFdy",                gcvFALSE,   gcvNULL,                    _GenDFdyCode},
    {"fwidth",              gcvFALSE,   gcvNULL,                    _GenFwidthCode}
};

const gctUINT BuiltInFunctionCount =
                    sizeof(BuiltInFunctionInfos) / sizeof(slsBUILT_IN_FUNCTION_INFO);


gceSTATUS
slEvaluateBuiltInFunction(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN sloIR_CONSTANT * OperandConstants,
    OUT sloIR_CONSTANT * ResultConstant
    )
{
    gceSTATUS                       status;
    gctUINT                         i;
    sltBUILT_IN_EVALUATE_FUNC_PTR   evaluate = gcvNULL;
    sloIR_CONSTANT                  resultConstant;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(PolynaryExpr->type == slvPOLYNARY_FUNC_CALL);
    gcmASSERT(PolynaryExpr->funcName != gcvNULL);
    gcmASSERT(PolynaryExpr->funcName->isBuiltIn);
    gcmASSERT(OperandCount > 0);
    gcmASSERT(OperandConstants);
    gcmASSERT(ResultConstant);

    *ResultConstant = gcvNULL;

    for (i = 0; i < BuiltInFunctionCount; i++)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(BuiltInFunctionInfos[i].symbol, PolynaryExpr->funcSymbol)))
        {
            evaluate = BuiltInFunctionInfos[i].evaluate;
            break;
        }
    }

    gcmASSERT(i < BuiltInFunctionCount);

    /* TODO */
    if (evaluate == gcvNULL) { gcmFOOTER_NO(); return gcvSTATUS_OK; }

    /* Create the constant */
    PolynaryExpr->exprBase.dataType->qualifier = slvQUALIFIER_CONST;

    status = sloIR_CONSTANT_Construct(
                                    Compiler,
                                    PolynaryExpr->exprBase.base.lineNo,
                                    PolynaryExpr->exprBase.base.stringNo,
                                    PolynaryExpr->exprBase.dataType,
                                    &resultConstant);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    /* Evaluate the built-in */
    status = (*evaluate)(
                        Compiler,
                        OperandCount,
                        OperandConstants,
                        resultConstant);

    if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

    *ResultConstant = resultConstant;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slGenBuiltInFunctionCode(
    IN sloCOMPILER Compiler,
    IN sloCODE_GENERATOR CodeGenerator,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctUINT OperandCount,
    IN slsGEN_CODE_PARAMETERS * OperandsParameters,
    IN slsIOPERAND * IOperand,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gctUINT                         i;
    sltBUILT_IN_GEN_CODE_FUNC_PTR   genCode = gcvNULL;
    gceSTATUS                       status;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(PolynaryExpr->type == slvPOLYNARY_FUNC_CALL);
    gcmASSERT(PolynaryExpr->funcName != gcvNULL);
    gcmASSERT(PolynaryExpr->funcName->isBuiltIn);
    gcmASSERT(IOperand);
    gcmASSERT(Parameters);

    for (i = 0; i < BuiltInFunctionCount; i++)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(BuiltInFunctionInfos[i].symbol, PolynaryExpr->funcSymbol)))
        {
            Parameters->treatFloatAsInt = BuiltInFunctionInfos[i].treatFloatAsInt;
            genCode                     = BuiltInFunctionInfos[i].genCode;
            break;
        }
    }

    gcmASSERT(i < BuiltInFunctionCount);

    gcmASSERT(genCode);

    if (genCode == gcvNULL)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    status = (*genCode)(
                    Compiler,
                    CodeGenerator,
                    PolynaryExpr,
                    OperandCount,
                    OperandsParameters,
                    IOperand);
    gcmFOOTER();
    return status;
}
