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




#include "gc_glff_precomp.h"

#define _GC_OBJ_ZONE glvZONE_TRACE

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  _UpdateXXXFlags
**
**  _UpdateXXXFlags set of functions updates flags such as zero and one.
**
**  INPUT:
**
**      Variable
**          Variable to be updated.
**
**  OUTPUT:
**
**      Nothing.
*/

static void _UpdateMutantFlags(
    glsMUTANT_PTR Variable
    )
{
    gcmHEADER_ARG("Variable=0x%x", Variable);

    Variable->zero = (Variable->value.i == 0);

    switch (Variable->type)
    {
    case glvINT:
        Variable->one = (Variable->value.i == 1);
        break;

    case glvFIXED:
        Variable->one = (Variable->value.x == gcvONE_X);
        break;

    case glvFLOAT:
        Variable->one = (Variable->value.f == 1.0f);
        break;

    default:
        gcmFATAL("_UpdateMutantFlags: invalid type %d", Variable->type);
    }
    gcmFOOTER_NO();
}

static void _UpdateVectorFlags(
    glsVECTOR_PTR Variable
    )
{
    gcmHEADER_ARG("Variable=0x%x", Variable);

    Variable->zero3
        = ((Variable->value[0].i == 0)
        && (Variable->value[1].i == 0)
        && (Variable->value[2].i == 0));

    Variable->zero4
        = ((Variable->value[3].i == 0)
        && (Variable->zero3));

    switch (Variable->type)
    {
    case glvINT:
        Variable->one3
            = ((Variable->value[0].i == 1)
            && (Variable->value[1].i == 1)
            && (Variable->value[2].i == 1));

        Variable->one4
            = ((Variable->value[3].i == 1)
            && (Variable->one3));
        break;

    case glvFIXED:
        Variable->one3
            = ((Variable->value[0].x == gcvONE_X)
            && (Variable->value[1].x == gcvONE_X)
            && (Variable->value[2].x == gcvONE_X));

        Variable->one4
            = ((Variable->value[3].x == gcvONE_X)
            && (Variable->one3));
        break;

    case glvFLOAT:
        Variable->one3
            = ((Variable->value[0].f == 1.0f)
            && (Variable->value[1].f == 1.0f)
            && (Variable->value[2].f == 1.0f));

        Variable->one4
            = ((Variable->value[3].f == 1.0f)
            && (Variable->one3));
        break;

    default:
        gcmFATAL("_UpdateVectorFlags: invalid type %d", Variable->type);
    }

    gcmFOOTER_NO();
}


/******************************************************************************\
***************** Value Conversion From/To OpenGL Enumerations *****************
\******************************************************************************/

/*******************************************************************************
**
**  glfConvertGLEnum
**
**  Converts OpenGL GL_xxx constants into intrernal enumerations.
**
**  INPUT:
**
**      Names
**      NameCount
**          Pointer and a count of the GL enumaration array.
**
**      Value
**      Type
**          GL enumeration value to be converted.
**
**  OUTPUT:
**
**      Result
**          Internal value corresponding to the GL enumeration.
*/

GLboolean glfConvertGLEnum(
    const GLenum* Names,
    GLint NameCount,
    const GLvoid* Value,
    gleTYPE Type,
    GLuint* Result
    )
{
    /* Convert the enum. */
    GLenum value = (Type == glvFLOAT)
        ? glmFLOAT2INT(* (GLfloat *) Value)
        :             (* (GLint   *) Value);

    /* Search the map for it. */
    GLint i;

    gcmHEADER_ARG("Names=0x%x NameCount=%d Value=0x%x Type=0x%04x Result=0x%x",
                    Names, NameCount, Value, Type, Result);

    for (i = 0; i < NameCount; i++)
    {
        if (Names[i] == value)
        {
            *Result = i;
            gcmFOOTER_ARG("return=%d", GL_TRUE);
            return GL_TRUE;
        }
    }

    gcmFOOTER_ARG("return=%d", GL_FALSE);

    /* Bad enum. */
    return GL_FALSE;
}

/*******************************************************************************
**
**  glfConvertGLboolean
**
**  Validates OpenGL GLboolean constants.
**
**  INPUT:
**
**      Value
**      Type
**          GL enumeration value to be converted.
**
**  OUTPUT:
**
**      Result
**          Internal value corresponding to the GL enumeration.
*/

GLboolean glfConvertGLboolean(
    const GLvoid* Value,
    gleTYPE Type,
    GLuint* Result
    )
{
    GLboolean result;
    static GLenum _BooleanNames[] =
    {
        GL_FALSE,
        GL_TRUE
    };

    gcmHEADER_ARG("Value=0x%x Type=0x%04x Result=0x%x", Value, Type, Result);

    result = glfConvertGLEnum(
        _BooleanNames,
        gcmCOUNTOF(_BooleanNames),
        Value,
        Type,
        Result
        );

    gcmFOOTER_ARG("return=%d", result);

    return result;
}


/******************************************************************************\
***************************** Value Type Converters ****************************
\******************************************************************************/

void glfGetFromBool(
    GLboolean Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=%d Value=0x%x Type=0x%04x", Variable, Value, Type);
    glfGetFromBoolArray(&Variable, 1, Value, Type);
    gcmFOOTER_NO();
}

void glfGetFromBoolArray(
    const GLboolean* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;
    gcmHEADER_ARG("Variable=%d Value=0x%x Type=0x%04x", Variables, Value, Type);

    for (i = 0; i < Count; i++)
    {
        switch (Type)
        {
        case glvBOOL:
            ((GLboolean *) Value) [i] = Variables[i];
            break;

        case glvINT:
            ((GLint *) Value) [i] = glmBOOL2INT(Variables[i]);
            break;

        case glvFIXED:
            ((GLfixed *) Value) [i] = glmBOOL2FIXED(Variables[i]);
            break;

        case glvFLOAT:
            ((GLfloat *) Value) [i] = glmBOOL2FLOAT(Variables[i]);
            break;

        default:
            gcmFATAL("glfGetFromIntArray: invalid type %d", Type);
        }
    }
    gcmFOOTER_NO();
}

void glfGetFromInt(
    GLint Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=%d Value=0x%x Type=0x%04x", Variable, Value, Type);
    glfGetFromIntArray(&Variable, 1, Value, Type);
    gcmFOOTER_NO();
}

void glfGetFromIntArray(
    const GLint* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;

    gcmHEADER_ARG("Variables=0x%x Count=%d Value=0x%x Type=0x%04x", Variables, Count, Value, Type);

    for (i = 0; i < Count; i++)
    {
        switch (Type)
        {
        case glvBOOL:
            ((GLboolean *) Value) [i] = glmINT2BOOL(Variables[i]);
            break;

        case glvINT:
            ((GLint *) Value) [i] = Variables[i];
            break;

        case glvFIXED:
            ((GLfixed *) Value) [i] = glmINT2FIXED(Variables[i]);
            break;

        case glvFLOAT:
            ((GLfloat *) Value) [i] = glmINT2FLOAT(Variables[i]);
            break;

        default:
            gcmFATAL("glfGetFromIntArray: invalid type %d", Type);
        }
    }
    gcmFOOTER_NO();
}

void glfGetFromFixed(
    GLfixed Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=%d Value=0x%x Type=0x%04x", Variable, Value, Type);
    glfGetFromFixedArray(&Variable, 1, Value, Type);
    gcmFOOTER_NO();
}

void glfGetFromFixedArray(
    const GLfixed* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;

    gcmHEADER_ARG("Variables=0x%x Count=%d Value=0x%x Type=0x%04x", Variables, Count, Value, Type);

    for (i = 0; i < Count; i++)
    {
        switch (Type)
        {
        case glvBOOL:
            ((GLboolean *) Value) [i] = glmFIXED2BOOL(Variables[i]);
            break;

        case glvINT:
            ((GLint *) Value) [i] = glmFIXED2INT(Variables[i]);
            break;

        case glvNORM:
            ((GLint *) Value) [i] = glmFIXED2NORM(Variables[i]);
            break;

        case glvFIXED:
            ((GLfixed *) Value) [i] = Variables[i];
            break;

        case glvFLOAT:
            ((GLfloat *) Value) [i] = glmFIXED2FLOAT(Variables[i]);
            break;

        default:
            gcmFATAL("glfGetFromFixedArray: invalid type %d", Type);
        }
    }
    gcmFOOTER_NO();
}

void glfGetFromFloat(
    GLfloat Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=%f Value=0x%x Type=0x%04x", Variable, Value, Type);
    glfGetFromFloatArray(&Variable, 1, Value, Type);
    gcmFOOTER_NO();
}

void glfGetFromFloatArray(
    const GLfloat* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;
    gcmHEADER_ARG("Variables=%f Count=%d Value=0x%x Type=0x%04x", Variables, Count, Value, Type);

    for (i = 0; i < Count; i++)
    {
        switch (Type)
        {
        case glvBOOL:
            ((GLboolean *) Value) [i] = glmFLOAT2BOOL(Variables[i]);
            break;

        case glvINT:
            ((GLint *) Value) [i] = glmFLOAT2INT(Variables[i]);
            break;

        case glvNORM:
            ((GLint *) Value) [i] = glmFLOAT2NORM(Variables[i]);
            break;

        case glvFIXED:
            ((GLfixed *) Value) [i] = glmFLOAT2FIXED(Variables[i]);
            break;

        case glvFLOAT:
            ((GLfloat *) Value) [i] = Variables[i];
            break;

        default:
            gcmFATAL("glfGetFromFloatArray: invalid type %d", Type);
        }
    }
    gcmFOOTER_NO();
}

void glfGetFromEnum(
    GLenum Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=0x%04x Value=0x%x Type=0x%04x", Variable, Value, Type);
    glfGetFromEnumArray(&Variable, 1, Value, Type);
    gcmFOOTER_NO();
}

void glfGetFromEnumArray(
    const GLenum* Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;

    gcmHEADER_ARG("Variables=0x%x Count=%d Value=0x%x Type=0x%04x", Variables, Count, Value, Type);

    for (i = 0; i < Count; i++)
    {
        switch (Type)
        {
        case glvBOOL:
            ((GLboolean *) Value) [i] = glmINT2BOOL(Variables[i]);
            break;

        case glvINT:
            ((GLint *) Value) [i] = Variables[i];
            break;

        case glvFIXED:
            ((GLfixed *) Value) [i] = Variables[i];
            break;

        case glvFLOAT:
            ((GLfloat *) Value) [i] = glmINT2FLOAT(Variables[i]);
            break;

        default:
            gcmFATAL("glfGetFromEnumArray: invalid type %d", Type);
        }
    }
    gcmFOOTER_NO();
}

void glfGetFromMutable(
    gluMUTABLE Variable,
    gleTYPE VariableType,
    GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=0x%x VariableType=0x%04x Value=0x%x Type=0x%04x",
                    Variable, VariableType, Value, Type);
    glfGetFromMutableArray(&Variable, VariableType, 1, Value, Type);
    gcmFOOTER_NO();
}

void glfGetFromMutableArray(
    const gluMUTABLE_PTR Variables,
    gleTYPE VariableType,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;
    gcmHEADER_ARG("Variables=0x%x VariableType=0x%04x Count=%d Value=0x%x Type=0x%04x",
                    Variables, VariableType, Count, Value, Type);

    for (i = 0; i < Count; i++)
    {
        GLvoid* value = gcvNULL;

        switch (Type)
        {
        case glvBOOL:
            value = & ((GLboolean *) Value) [i];
            break;

        case glvINT:
        case glvNORM:
            value = & ((GLint *) Value) [i];
            break;

        case glvFIXED:
            value = & ((GLfixed *) Value) [i];
            break;

        case glvFLOAT:
            value = & ((GLfloat *) Value) [i];
            break;

        default:
            gcmFATAL("glfGetFromMutableArray: invalid type %d", Type);
        }

        switch (VariableType)
        {
        case glvINT:
            glfGetFromInt(Variables[i].i, value, Type);
            break;

        case glvFIXED:
            glfGetFromFixed(Variables[i].x, value, Type);
            break;

        case glvFLOAT:
            glfGetFromFloat(Variables[i].f, value, Type);
            break;

        default:
            gcmFATAL("glfGetFromMutableArray: invalid source type %d", VariableType);
        }
    }
    gcmFOOTER_NO();
}

void glfGetFromMutant(
    const glsMUTANT_PTR Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x",
                    Variable, Value, Type);
    glfGetFromMutantArray(Variable, 1, Value, Type);
    gcmFOOTER_NO();
}

void glfGetFromMutantArray(
    const glsMUTANT_PTR Variables,
    GLint Count,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;
    gcmHEADER_ARG("Variables=0x%x Count=%d Value=0x%x Type=0x%04x",
                    Variables, Count, Value, Type);

    for (i = 0; i < Count; i++)
    {
        switch (Type)
        {
        case glvBOOL:
            glfGetFromMutable(
                Variables[i].value,
                Variables[i].type,
                & ((GLboolean *) Value) [i],
                Type
                );
            break;

        case glvINT:
        case glvNORM:
            glfGetFromMutable(
                Variables[i].value,
                Variables[i].type,
                & ((GLint *) Value) [i],
                Type
                );
            break;

        case glvFIXED:
            glfGetFromMutable(
                Variables[i].value,
                Variables[i].type,
                & ((GLfixed *) Value) [i],
                Type
                );
            break;

        case glvFLOAT:
            glfGetFromMutable(
                Variables[i].value,
                Variables[i].type,
                & ((GLfloat *) Value) [i],
                Type
                );
            break;

        default:
            gcmFATAL("glfGetFromMutableArray: invalid type %d", Type);
        }
    }
    gcmFOOTER_NO();
}

void glfGetFromVector3(
    const glsVECTOR_PTR Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x",
                    Variable, Value, Type);

    switch (Type)
    {
    case glvBOOL:
        for (i = 0; i < 3; i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLboolean *) Value) [i],
                Type
                );
        }
        break;

    case glvINT:
        for (i = 0; i < 3; i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLint *) Value) [i],
                Type
                );
        }
        break;

    case glvFIXED:
        for (i = 0; i < 3; i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLfixed *) Value) [i],
                Type
                );
        }
        break;

    case glvFLOAT:
        for (i = 0; i < 3; i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLfloat *) Value) [i],
                Type
                );
        }
        break;

    default:
        gcmFATAL("glfGetFromVector3: invalid type %d", Type);
    }
    gcmFOOTER_NO();
}

void glfGetFromVector4(
    const glsVECTOR_PTR Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLuint i;
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x",
                    Variable, Value, Type);

    switch (Type)
    {
    case glvBOOL:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLboolean *) Value) [i],
                Type
                );
        }
        break;

    case glvINT:
    case glvNORM:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLint *) Value) [i],
                Type
                );
        }
        break;

    case glvFIXED:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLfixed *) Value) [i],
                Type
                );
        }
        break;

    case glvFLOAT:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLfloat *) Value) [i],
                Type
                );
        }
        break;

    default:
        gcmFATAL("glfGetFromVector4: invalid type %d", Type);
    }
    gcmFOOTER_NO();
}

void glfGetFromMatrix(
    const glsMATRIX_PTR Variable,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLuint i;
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x",
                    Variable, Value, Type);

    switch (Type)
    {
    case glvBOOL:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLboolean *) Value) [i],
                Type
                );
        }
        break;

    case glvINT:
    case glvNORM:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLint *) Value) [i],
                Type
                );
        }
        break;

    case glvFIXED:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLfixed *) Value) [i],
                Type
                );
        }
        break;

    case glvFLOAT:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            glfGetFromMutable(
                Variable->value[i],
                Variable->type,
                & ((GLfloat *) Value) [i],
                Type
                );
        }
        break;

    default:
        gcmFATAL("glfGetFromMatrix: invalid type %d", Type);
    }
    gcmFOOTER_NO();
}


/******************************************************************************\
**************************** Get Values From Raw Input *************************
\******************************************************************************/

GLfixed glfFixedFromRaw(
    const GLvoid* Variable,
    gleTYPE Type
    )
{
    GLfixed result=0;
    gcmHEADER_ARG("Variable=0x%x Type=0x%04x", Variable, Type);
    switch (Type)
    {
    case glvINT:
        result = glmINT2FIXED(* (GLint *) Variable);
        break;

    case glvFIXED:
        result = * (GLfixed *) Variable;
        break;

    case glvFLOAT:
        result = glmFLOAT2FIXED(* (GLfloat *) Variable);
        break;

    default:
        gcmFATAL("glfFixedFromRaw: invalid type %d", Type);
        break;
    }
	gcmFOOTER_ARG("return=0x%d08x", result);
    return result;
}

GLfloat glfFloatFromRaw(
    const GLvoid* Variable,
    gleTYPE Type
    )
{
    GLfloat result = 0.0;
    gcmHEADER_ARG("Variable=0x%x Type=0x%04x", Variable, Type);
    switch (Type)
    {
    case glvINT:
        result = glmINT2FLOAT(* (GLint *) Variable);
        break;
    case glvFIXED:
        result = glmFIXED2FLOAT(* (GLfixed *) Variable);
        break;
    case glvFLOAT:
        result = * (GLfloat *) Variable;
        break;

    default:
        gcmFATAL("glfFloatFromRaw: invalid type %d", Type);
        break;
    }
    gcmFOOTER_ARG("return=%f", result);
    return result;
}


/******************************************************************************\
**************************** Get Values From Mutables **************************
\******************************************************************************/

GLboolean glfBoolFromMutable(
    gluMUTABLE Variable,
    gleTYPE Type
    )
{
    GLboolean result;
    gcmHEADER_ARG("Variable=0x%x Type=0x%04x", Variable, Type);
    glfGetFromMutable(Variable, Type, &result, glvBOOL);
    gcmFOOTER_ARG("return=%d", result);
    return result;
}

GLint glfIntFromMutable(
    gluMUTABLE Variable,
    gleTYPE Type
    )
{
    GLint result;
    gcmHEADER_ARG("Variable=0x%x Type=0x%04x", Variable, Type);
    glfGetFromMutable(Variable, Type, &result, glvINT);
    gcmFOOTER_ARG("return=%d", result);
    return result;
}

GLfixed glfFixedFromMutable(
    gluMUTABLE Variable,
    gleTYPE Type
    )
{
    GLfixed result;
    gcmHEADER_ARG("Variable=0x%x Type=0x%04x", Variable, Type);
    glfGetFromMutable(Variable, Type, &result, glvFIXED);
    gcmFOOTER_ARG("return=0x%08x", result);
    return result;
}

GLfloat glfFloatFromMutable(
    gluMUTABLE Variable,
    gleTYPE Type
    )
{
    GLfloat result;
    gcmHEADER_ARG("Variable=0x%x Type=0x%04x", Variable, Type);
    glfGetFromMutable(Variable, Type, &result, glvFLOAT);
    gcmFOOTER_ARG("return=%f", result);
    return result;
}


/******************************************************************************\
**************************** Get Values From Mutants ***************************
\******************************************************************************/

GLboolean glfBoolFromMutant(
    const glsMUTANT_PTR Variable
    )
{
    GLboolean result;
    gcmHEADER_ARG("Variable=0x%x", Variable);
    glfGetFromMutant(Variable, &result, glvBOOL);
    gcmFOOTER_ARG("return=%d", result);
    return result;
}

GLint glfIntFromMutant(
    const glsMUTANT_PTR Variable
    )
{
    GLint result;
    gcmHEADER_ARG("Variable=0x%x", Variable);
    glfGetFromMutant(Variable, &result, glvINT);
    gcmFOOTER_ARG("return=%d", result);
    return result;
}

GLfixed glfFixedFromMutant(
    const glsMUTANT_PTR Variable
    )
{
    GLfixed result;
    gcmHEADER_ARG("Variable=0x%x", Variable);
    glfGetFromMutant(Variable, &result, glvFIXED);
    gcmFOOTER_ARG("return=0x%08x", result);
    return result;
}

GLfloat glfFloatFromMutant(
    const glsMUTANT_PTR Variable
    )
{
    GLfloat result;
    gcmHEADER_ARG("Variable=0x%x", Variable);
    glfGetFromMutant(Variable, &result, glvFLOAT);
    gcmFOOTER_ARG("return=%f", result);
    return result;
}


/******************************************************************************\
***************************** Set Values To Mutants ****************************
\******************************************************************************/

void glfSetIntMutant(
    glsMUTANT_PTR Variable,
    GLint Value
    )
{
    gcmHEADER_ARG("Variable=0x%x Value=%d", Variable, Value);
    /* Set the type. */
    Variable->type = glvINT;

    /* Set the value. */
    Variable->value.i = Value;

    /* Update special value flags. */
    _UpdateMutantFlags(Variable);

    gcmFOOTER_NO();
}

void glfSetFixedMutant(
    glsMUTANT_PTR Variable,
    GLfixed Value
    )
{
    gcmHEADER_ARG("Variable=0x%x Value=0x%08x", Variable, Value);
    /* Set the type. */
    Variable->type = glvFIXED;

    /* Set the value. */
    Variable->value.x = Value;

    /* Update special value flags. */
    _UpdateMutantFlags(Variable);

    gcmFOOTER_NO();
}

void glfSetFloatMutant(
    glsMUTANT_PTR Variable,
    GLfloat Value
    )
{
    gcmHEADER_ARG("Variable=0x%x Value=%f", Variable, Value);
    /* Set the type. */
    Variable->type = glvFLOAT;

    /* Set the value. */
    Variable->value.f = Value;

    /* Update special value flags. */
    _UpdateMutantFlags(Variable);

    gcmFOOTER_NO();
}

void glfSetMutant(
    glsMUTANT_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x", Variable, Value, Type);
    /* Set the type. */
    Variable->type = Type;

    /* Set the value. */
    switch (Type)
    {
    case glvINT:
        Variable->value.i = * (GLint *) Value;
        break;

    case glvFIXED:
        Variable->value.x = * (GLfixed *) Value;
        break;

    case glvFLOAT:
        Variable->value.f = * (GLfloat *) Value;
        break;

    default:
        gcmFATAL("glfSetMutant: invalid type %d", Type);
    }

    /* Update special value flags. */
    _UpdateMutantFlags(Variable);

    gcmFOOTER_NO();
}

void glfSetClampedMutant(
    glsMUTANT_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x", Variable, Value, Type);
    /* Set the type. */
    Variable->type = Type;

    /* Set the value. */
    switch (Type)
    {
    case glvFIXED:
        Variable->value.x = glmFIXEDCLAMP_0_TO_1(* (GLfixed *) Value);
        break;

    case glvFLOAT:
        Variable->value.f = glmFLOATCLAMP_0_TO_1(* (GLfloat *) Value);
        break;

    default:
        gcmFATAL("glfSetClampedMutant: invalid type %d", Type);
    }

    /* Update special value flags. */
    _UpdateMutantFlags(Variable);

    gcmFOOTER_NO();
}


/******************************************************************************\
***************************** Set Values To Vectors ****************************
\******************************************************************************/

void glfSetIntVector4(
    glsVECTOR_PTR Variable,
    GLint X,
    GLint Y,
    GLint Z,
    GLint W
    )
{
    gcmHEADER_ARG("Variable=0x%x X=%d Y=%d Z=%d W=%d", Variable, X, Y, Z, W);
    /* Set the type. */
    Variable->type = glvINT;

    /* Set components. */
    Variable->value[0].i = X;
    Variable->value[1].i = Y;
    Variable->value[2].i = Z;
    Variable->value[3].i = W;

    /* Update special value flags. */
    _UpdateVectorFlags(Variable);
    gcmFOOTER_NO();
}

void glfSetFixedVector4(
    glsVECTOR_PTR Variable,
    GLfixed X,
    GLfixed Y,
    GLfixed Z,
    GLfixed W
    )
{
    gcmHEADER_ARG("Variable=0x%x X=0x%08x Y=0x%08x Z=0x%08x W=0x%08x", Variable, X, Y, Z, W);
    /* Set the type. */
    Variable->type = glvFIXED;

    /* Set components. */
    Variable->value[0].x = X;
    Variable->value[1].x = Y;
    Variable->value[2].x = Z;
    Variable->value[3].x = W;

    /* Update special value flags. */
    _UpdateVectorFlags(Variable);
    gcmFOOTER_NO();
}

void glfSetFloatVector4(
    glsVECTOR_PTR Variable,
    GLfloat X,
    GLfloat Y,
    GLfloat Z,
    GLfloat W
    )
{
    gcmHEADER_ARG("Variable=0x%x X=%f Y=%f Z=%f W=%f", Variable, X, Y, Z, W);
    /* Set the type. */
    Variable->type = glvFLOAT;

    /* Set components. */
    Variable->value[0].f = X;
    Variable->value[1].f = Y;
    Variable->value[2].f = Z;
    Variable->value[3].f = W;

    /* Update special value flags. */
    _UpdateVectorFlags(Variable);
    gcmFOOTER_NO();
}

void glfSetVector3(
    glsVECTOR_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLint i;

    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x", Variable, Value, Type);

    /* Set the type. */
    Variable->type = Type;

    /* Set the value. */
    switch (Type)
    {
    case glvINT:
        for (i = 0; i < 3; i++)
        {
            Variable->value[i].i = ((GLint *) Value) [i];
        }
        break;

    case glvFIXED:
        for (i = 0; i < 3; i++)
        {
            Variable->value[i].x = ((GLfixed *) Value) [i];
        }
        break;

    case glvFLOAT:
        for (i = 0; i < 3; i++)
        {
            Variable->value[i].f = ((GLfloat *) Value) [i];
        }
        break;

    default:
        gcmFATAL("glfSetVector3: invalid type %d", Type);
    }

    /* Set the fourth component to zero. */
    Variable->value[3].i = 0;

    /* Update special value flags. */
    _UpdateVectorFlags(Variable);

    gcmFOOTER_NO();
}

void glfSetVector4(
    glsVECTOR_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLuint i;
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x", Variable, Value, Type);

    /* Set the type. */
    Variable->type = Type;

    /* Set the value. */
    switch (Type)
    {
    case glvINT:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            Variable->value[i].i = ((GLint *) Value) [i];
        }
        break;

    case glvFIXED:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            Variable->value[i].x = ((GLfixed *) Value) [i];
        }
        break;

    case glvFLOAT:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            Variable->value[i].f = ((GLfloat *) Value) [i];
        }
        break;

    default:
        gcmFATAL("glfSetVector4: invalid type %d", Type);
    }

    /* Update special value flags. */
    _UpdateVectorFlags(Variable);

    gcmFOOTER_NO();
}

void glfSetClampedVector4(
    glsVECTOR_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLuint i;
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x", Variable, Value, Type);

    /* Set the type. */
    Variable->type = Type;

    /* Set the value. */
    switch (Type)
    {
    case glvFIXED:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            Variable->value[i].x
                = glmFIXEDCLAMP_0_TO_1(((GLfixed *) Value) [i]);
        }
        break;

    case glvFLOAT:
        for (i = 0; i < gcmCOUNTOF(Variable->value); i++)
        {
            Variable->value[i].f
                = glmFLOATCLAMP_0_TO_1(((GLfloat *) Value) [i]);
        }
        break;

    default:
        gcmFATAL("glfSetVector4: invalid type %d", Type);
    }

    /* Update special value flags. */
    _UpdateVectorFlags(Variable);

    gcmFOOTER_NO();
}

void glfSetHomogeneousVector4(
    glsVECTOR_PTR Variable,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Variable=0x%x Value=0x%x Type=0x%04x", Variable, Value, Type);
    /* Set the type. */
    Variable->type = Type;

    /* Set the value. */
    switch (Type)
    {
    case glvINT:
        {
            GLint* value = ((GLint *) Value);

            if ((value[3] != 0) && (value[3] != 1))
            {
                Variable->value[0].i = gcoMATH_DivideInt(value[0], value[3]);
                Variable->value[1].i = gcoMATH_DivideInt(value[1], value[3]);
                Variable->value[2].i = gcoMATH_DivideInt(value[2], value[3]);
                Variable->value[3].i = 1;
            }
            else
            {
                Variable->value[0].i = value[0];
                Variable->value[1].i = value[1];
                Variable->value[2].i = value[2];
                Variable->value[3].i = value[3];
            }
        }
        break;

    case glvFIXED:
        {
            GLfixed* value = ((GLfixed *) Value);

            if ((value[3] != gcvZERO_X) && (value[3] != gcvONE_X))
            {
                Variable->value[0].x = gcmXDivide(value[0], value[3]);
                Variable->value[1].x = gcmXDivide(value[1], value[3]);
                Variable->value[2].x = gcmXDivide(value[2], value[3]);
                Variable->value[3].x = gcvONE_X;
            }
            else
            {
                Variable->value[0].x = value[0];
                Variable->value[1].x = value[1];
                Variable->value[2].x = value[2];
                Variable->value[3].x = value[3];
            }
        }
        break;

    case glvFLOAT:
        {
            GLfloat* value = ((GLfloat *) Value);

            if ((value[3] != 0.0f) && (value[3] != 1.0f))
            {
                Variable->value[0].f = gcoMATH_Divide(value[0], value[3]);
                Variable->value[1].f = gcoMATH_Divide(value[1], value[3]);
                Variable->value[2].f = gcoMATH_Divide(value[2], value[3]);
                Variable->value[3].f = 1.0f;
            }
            else
            {
                Variable->value[0].f = value[0];
                Variable->value[1].f = value[1];
                Variable->value[2].f = value[2];
                Variable->value[3].f = value[3];
            }
        }
        break;

    default:
        gcmFATAL("glfSetHomogeneousVector4: invalid type %d", Type);
    }

    /* Update special value flags. */
    _UpdateVectorFlags(Variable);

    gcmFOOTER_NO();
}

void glfSetVector4FromMutants(
    glsVECTOR_PTR Variable,
    const glsMUTANT_PTR X,
    const glsMUTANT_PTR Y,
    const glsMUTANT_PTR Z,
    const glsMUTANT_PTR W
    )
{
    gcmHEADER_ARG("Variable=0x%x X=0x%x Y=0x%x Z=0x%x W=0x%x", Variable, X, Y, Z, W);
    /* Get type from the first mutant. */
    Variable->type = X->type;

    /* Set components. */
    glfGetFromMutant(X, &Variable->value[0], Variable->type);
    glfGetFromMutant(Y, &Variable->value[1], Variable->type);
    glfGetFromMutant(Z, &Variable->value[2], Variable->type);
    glfGetFromMutant(W, &Variable->value[3], Variable->type);

    /* Update special value flags. */
    _UpdateVectorFlags(Variable);
    gcmFOOTER_NO();
}


/******************************************************************************\
*********************** Operations on Mutants and Vectors **********************
\******************************************************************************/

void glfMulMutant(
    const glsMUTANT_PTR Variable1,
    const gltFRACTYPE Variable2,
    glsMUTANT_PTR Result
    )
{
    gltFRACTYPE variable1 = glfFracFromMutant(Variable1);
    gltFRACTYPE result = glmFRACMULTIPLY(variable1, Variable2);
    gcmHEADER_ARG("Variable1=0x%x Variable2=%f Result=0x%x", Variable1, Variable2, Result);
    glfSetFracMutant(Result, result);
    gcmFOOTER_NO();
}

void glfCosMutant(
    const glsMUTANT_PTR Mutant,
    glsMUTANT_PTR Result
    )
{
    gcmHEADER_ARG("Mutant=0x%x Result=0x%x", Mutant, Result);
    /* Dispatch base on the type. */
    switch (Mutant->type)
    {
    case glvFIXED:
        {
            /* Convert to randians. */
            GLfixed radians = gcmXMultiply(Mutant->value.x, glvFIXEDPIOVER180);

            /* Compute cos. */
            GLfixed cosine = glfCosX(radians);

            /* Set the value. */
            glfSetFixedMutant(Result, cosine);
        }
        break;

    case glvFLOAT:
        {
            /* Convert to randians. */
            GLfloat radians = Mutant->value.f * glvFLOATPIOVER180;

            /* Compute cos. */
            GLfloat cosine = gcoMATH_Cosine(radians);

            /* Set the value. */
            glfSetFloatMutant(Result, cosine);
        }
        break;

    default:
        gcmFATAL("glfCosMutant: invalid type %d", Mutant->type);
    }
    gcmFOOTER_NO();
}

void glfAddVector4(
    const glsVECTOR_PTR Variable1,
    const glsVECTOR_PTR Variable2,
    glsVECTOR_PTR Result
    )
{
    GLint i;

    gcmHEADER_ARG("Variable1=0x%x Variable2=0x%x Result=0x%x", Variable1, Variable2, Result);

    /* Determine the output type. */
    if (Variable1->type == Variable2->type)
    {
        Result->type = Variable1->type;
    }

    else if ((Variable1->type == glvFLOAT) ||
             (Variable2->type == glvFLOAT))
    {
        Result->type = glvFLOAT;
    }

    else if ((Variable1->type == glvFIXED) ||
             (Variable2->type == glvFIXED))
    {
        Result->type = glvFIXED;
    }

    else
    {
        Result->type = glvINT;
    }

    /* Dispatch base on the type. */
    switch (Result->type)
    {
    case glvINT:
        {
            GLint vec1[4];
            GLint vec2[4];
            GLint result[4];

            glfGetFromVector4(Variable1, vec1, glvINT);
            glfGetFromVector4(Variable2, vec2, glvINT);

            for (i = 0; i < 4; i++)
            {
                result[i] = vec1[i] + vec2[i];
            }

            glfSetVector4(Result, result, glvINT);
        }
        break;

    case glvFIXED:
        {
            GLfixed vec1[4];
            GLfixed vec2[4];
            GLfixed result[4];

            glfGetFromVector4(Variable1, vec1, glvFIXED);
            glfGetFromVector4(Variable2, vec2, glvFIXED);

            for (i = 0; i < 4; i++)
            {
                result[i] = vec1[i] + vec2[i];
            }

            glfSetVector4(Result, result, glvFIXED);
        }
        break;

    case glvFLOAT:
        {
            GLfloat vec1[4];
            GLfloat vec2[4];
            GLfloat result[4];

            glfGetFromVector4(Variable1, vec1, glvFLOAT);
            glfGetFromVector4(Variable2, vec2, glvFLOAT);

            for (i = 0; i < 4; i++)
            {
                result[i] = vec1[i] + vec2[i];
            }

            glfSetVector4(Result, result, glvFLOAT);
        }
        break;

    default:
        gcmFATAL("glfAddVector4: invalid type %d", Result->type);
    }
    gcmFOOTER_NO();
}


void glfMulVector4(
    const glsVECTOR_PTR Variable1,
    const glsVECTOR_PTR Variable2,
    glsVECTOR_PTR Result
    )
{
    GLint i;

    gcmHEADER_ARG("Variable1=0x%x Variable2=0x%x Result=0x%x", Variable1, Variable2, Result);

    /* Determine the output type. */
    if (Variable1->type == Variable2->type)
    {
        Result->type = Variable1->type;
    }

    else if ((Variable1->type == glvFLOAT) ||
             (Variable2->type == glvFLOAT))
    {
        Result->type = glvFLOAT;
    }

    else if ((Variable1->type == glvFIXED) ||
             (Variable2->type == glvFIXED))
    {
        Result->type = glvFIXED;
    }

    else
    {
        Result->type = glvINT;
    }

    /* Dispatch base on the type. */
    switch (Result->type)
    {
    case glvINT:
        {
            GLint vec1[4];
            GLint vec2[4];
            GLint result[4];

            glfGetFromVector4(Variable1, vec1, glvINT);
            glfGetFromVector4(Variable2, vec2, glvINT);

            for (i = 0; i < 4; i++)
            {
                result[i] = vec1[i] * vec2[i];
            }

            glfSetVector4(Result, result, glvINT);
        }
        break;

    case glvFIXED:
        {
            GLfixed vec1[4];
            GLfixed vec2[4];
            GLfixed result[4];

            glfGetFromVector4(Variable1, vec1, glvFIXED);
            glfGetFromVector4(Variable2, vec2, glvFIXED);

            for (i = 0; i < 4; i++)
            {
                result[i] = gcmXMultiply(vec1[i], vec2[i]);
            }

            glfSetVector4(Result, result, glvFIXED);
        }
        break;

    case glvFLOAT:
        {
            GLfloat vec1[4];
            GLfloat vec2[4];
            GLfloat result[4];

            glfGetFromVector4(Variable1, vec1, glvFLOAT);
            glfGetFromVector4(Variable2, vec2, glvFLOAT);

            for (i = 0; i < 4; i++)
            {
                result[i] = vec1[i] * vec2[i];
            }

            glfSetVector4(Result, result, glvFLOAT);
        }
        break;

    default:
        gcmFATAL("glfMulVector4: invalid type %d", Result->type);
    }
    gcmFOOTER_NO();
}

void glfNorm3Vector4(
    const glsVECTOR_PTR Vector,
    glsVECTOR_PTR Result
    )
{
    gcmHEADER_ARG("Vector=0x%x Result=0x%x", Vector, Result);
    /* Dispatch base on the type. */
    switch (Vector->type)
    {
    case glvFIXED:
        {
            /* Compute normal. */
            GLfixed squareSum
                = gcmXMultiply(Vector->value[0].x, Vector->value[0].x)
                + gcmXMultiply(Vector->value[1].x, Vector->value[1].x)
                + gcmXMultiply(Vector->value[2].x, Vector->value[2].x);

            GLfixed norm = glfRSQX(squareSum);

            /* Multiply vector by normal. */
            GLfixed x = gcmXMultiply(Vector->value[0].x, norm);
            GLfixed y = gcmXMultiply(Vector->value[1].x, norm);
            GLfixed z = gcmXMultiply(Vector->value[2].x, norm);
            GLfixed w = 0;

            /* Set the value. */
            glfSetFixedVector4(Result, x, y, z, w);
        }
        break;

    case glvFLOAT:
        {
            /* Compute normal. */
            GLfloat squareSum
                = Vector->value[0].f * Vector->value[0].f
                + Vector->value[1].f * Vector->value[1].f
                + Vector->value[2].f * Vector->value[2].f;

            GLfloat norm = (squareSum < 0)
                ? squareSum
                : gcoMATH_Divide(1.0f, gcoMATH_SquareRoot(squareSum));

            /* Multiply vector by normal. */
            GLfloat x = Vector->value[0].f * norm;
            GLfloat y = Vector->value[1].f * norm;
            GLfloat z = Vector->value[2].f * norm;
            GLfloat w = 0.0f;

            /* Set the value. */
            glfSetFloatVector4(Result, x, y, z, w);
        }
        break;

    default:
        gcmFATAL("glfNorm3Vector4: invalid type %d", Vector->type);
    }
    gcmFOOTER_NO();
}

/* Normalize vector3 to vector4.                */
/* Using float to do intermedia calculation.    */
void glfNorm3Vector4f(
    const glsVECTOR_PTR Vector,
    glsVECTOR_PTR Result
    )
{
    gcmHEADER_ARG("Vector=0x%x Result=0x%x", Vector, Result);
    /* Dispatch base on the type. */
    switch (Vector->type)
    {
    case glvFIXED:
    case glvFLOAT:
        {
            GLfloat vec[4];
            GLfloat squareSum, norm;
            GLfloat x, y, z, w;

            /* Convert vector to float type. */
            glfGetFromVector4(Vector, vec, glvFLOAT);

            /* Compute normal. */
            squareSum
                = vec[0] * vec[0]
                + vec[1] * vec[1]
                + vec[2] * vec[2];

            norm = (squareSum < 0)
                ? squareSum
                : gcoMATH_Divide(1.0f, gcoMATH_SquareRoot(squareSum));

            /* Multiply vector by normal. */
            x = vec[0] * norm;
            y = vec[1] * norm;
            z = vec[2] * norm;
            w = 0.0f;

            /* Set the value. */
            glfSetFloatVector4(Result, x, y, z, w);
        }
        break;

    default:
        gcmFATAL("glfNorm3Vector4f: invalid type %d", Vector->type);
    }
    gcmFOOTER_NO();
}

void glfHomogeneousVector4(
    const glsVECTOR_PTR Vector,
    glsVECTOR_PTR Result
    )
{
    gcmHEADER_ARG("Vector=0x%x Result=0x%x", Vector, Result);
    /* Dispatch base on the type. */
    switch (Vector->type)
    {
    case glvFIXED:
        {
            GLfixed x, y, z, w;

            /* Make vector homogeneous. */
            if ((Vector->value[3].x != gcvZERO_X) &&
                (Vector->value[3].x != gcvONE_X))
            {
                x = gcmXDivide( Vector->value[0].x, Vector->value[3].x);
                y = gcmXDivide( Vector->value[1].x, Vector->value[3].x);
                z = gcmXDivide( Vector->value[2].x, Vector->value[3].x);
                w = gcvONE_X;
            }
            else
            {
                x = Vector->value[0].x;
                y = Vector->value[1].x;
                z = Vector->value[2].x;
                w = Vector->value[3].x;
            }

            /* Set the value. */
            glfSetFixedVector4(Result, x, y, z, w);
        }
        break;

    case glvFLOAT:
        {
            GLfloat x, y, z, w;

            /* Make vector homogeneous. */
            if ((Vector->value[3].f != 0.0f) &&
                (Vector->value[3].f != 1.0f))
            {
                x = gcoMATH_Divide(Vector->value[0].f, Vector->value[3].f);
                y = gcoMATH_Divide(Vector->value[1].f, Vector->value[3].f);
                z = gcoMATH_Divide(Vector->value[2].f, Vector->value[3].f);
                w = 1.0f;
            }
            else
            {
                x = Vector->value[0].f;
                y = Vector->value[1].f;
                z = Vector->value[2].f;
                w = Vector->value[3].f;
            }

            /* Set the value. */
            glfSetFloatVector4(Result, x, y, z, w);
        }
        break;

    default:
        gcmFATAL("glfHomogeneousVector4: invalid type %d", Vector->type);
    }
    gcmFOOTER_NO();
}


/******************************************************************************\
**************** Debug Printing for Mutants, Vectors and Matrices **************
\******************************************************************************/

#if gcmIS_DEBUG(gcdDEBUG_TRACE)

void glfPrintMatrix3x3(
    gctSTRING Name,
    glsMATRIX_PTR Matrix
    )
{
    gctUINT y;
    gctFLOAT values[4 * 4];

    gcmHEADER_ARG("Name=0x%x Matrix=0x%x", Name, Matrix);

    glfGetFromMatrix(Matrix, values, glvFLOAT);

    gcmTRACE(
        gcvLEVEL_INFO,
        "%s:", Name
        );

    for (y = 0; y < 3; y += 1)
    {
        gcmTRACE(
            gcvLEVEL_INFO,
            "  %.6f, %.6f, %.6f",
            values[y + 0 * 4],
            values[y + 1 * 4],
            values[y + 2 * 4]
            );
    }
    gcmFOOTER_NO();
}

void glfPrintMatrix4x4(
    gctSTRING Name,
    glsMATRIX_PTR Matrix
    )
{
    gctUINT y;
    gctFLOAT values[4 * 4];

    gcmHEADER_ARG("Name=0x%x Matrix=0x%x", Name, Matrix);

    glfGetFromMatrix(Matrix, values, glvFLOAT);

    gcmTRACE(
        gcvLEVEL_INFO,
        "%s:", Name
        );

    for (y = 0; y < 4; y += 1)
    {
        gcmTRACE(
            gcvLEVEL_INFO,
            "  %.6f, %.6f, %.6f, %.6f",
            values[y + 0 * 4],
            values[y + 1 * 4],
            values[y + 2 * 4],
            values[y + 3 * 4]
            );
    }
    gcmFOOTER_NO();
}

void glfPrintVector3(
    gctSTRING Name,
    glsVECTOR_PTR Vector
    )
{
    gctFLOAT values[3];

    gcmHEADER_ARG("Name=0x%x Vector=0x%x", Name, Vector);

    glfGetFromVector3(Vector, values, glvFLOAT);

    gcmTRACE(
        gcvLEVEL_INFO,
        "%s: %.6f, %.6f, %.6f",
        Name, values[0], values[1], values[2]
        );

    gcmFOOTER_NO();
}

void glfPrintVector4(
    gctSTRING Name,
    glsVECTOR_PTR Vector
    )
{
    gctFLOAT values[4];

    gcmHEADER_ARG("Name=0x%x Vector=0x%x", Name, Vector);

    glfGetFromVector4(Vector, values, glvFLOAT);

    gcmTRACE(
        gcvLEVEL_INFO,
        "%s: %.6f, %.6f, %.6f, %.6f",
        Name, values[0], values[1], values[2], values[3]
        );

    gcmFOOTER_NO();
}

void glfPrintMutant(
    gctSTRING Name,
    glsMUTANT_PTR Mutant
    )
{
    gctFLOAT value;
    gcmHEADER_ARG("Name=0x%x Mutant=0x%x", Name, Mutant);

    glfGetFromMutant(Mutant, &value, glvFLOAT);

    gcmTRACE(
        gcvLEVEL_INFO,
        "%s: %.6f",
        Name, value
        );

    gcmFOOTER_NO();
}

#endif
