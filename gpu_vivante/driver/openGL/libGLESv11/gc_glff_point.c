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
********************** Individual State Setting Functions **********************
\******************************************************************************/

static GLenum _SetMinimumPointSize(
    glsCONTEXT_PTR Context,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLenum result;

    glsMUTANT temp;
    gcmHEADER_ARG("Context=0x%x Value=0x%x Type=0x%04x", Context, Value, Type);
    glfSetMutant(&temp, Value, Type);

    /* Cannot be negative. */
    if (glfIntFromMutant(&temp) < 0)
    {
        result = GL_INVALID_VALUE;
    }
    else
    {
        Context->pointStates.clampFrom = temp;

        /* Set uPointSize dirty. */
        Context->vsUniformDirty.uPointSizeDirty = gcvTRUE;

        result = GL_NO_ERROR;
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLenum _SetMaximumPointSize(
    glsCONTEXT_PTR Context,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLenum result;

    glsMUTANT temp;
    gcmHEADER_ARG("Context=0x%x Value=0x%x Type=0x%04x", Context, Value, Type);
    glfSetMutant(&temp, Value, Type);

    /* Cannot be negative. */
    if (glfIntFromMutant(&temp) < 0)
    {
        result = GL_INVALID_VALUE;
    }
    else
    {
        Context->pointStates.clampTo = temp;

        /* Set uPointSize dirty. */
        Context->vsUniformDirty.uPointSizeDirty = gcvTRUE;

        result = GL_NO_ERROR;
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}

static GLenum _SetDistanceAttenuation(
    glsCONTEXT_PTR Context,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Context=0x%x Value=0x%x Type=0x%04x", Context, Value, Type);
    glfSetVector3(&Context->pointStates.attenuation, Value, Type);

    /* Set uPointAttenuation dirty. */
    Context->vsUniformDirty.uPointAttenuationDirty = gcvTRUE;

    gcmFOOTER_ARG("return=%d", GL_NO_ERROR);
    return GL_NO_ERROR;
}

static GLenum _SetFadeThresholdSize(
    glsCONTEXT_PTR Context,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLenum result;

    glsMUTANT temp;
    gcmHEADER_ARG("Context=0x%x Value=0x%x Type=0x%04x", Context, Value, Type);
    glfSetMutant(&temp, Value, Type);

    /* Make sure the value isn't negative. */
    if (glfIntFromMutant(&temp) < 0)
    {
        result = GL_INVALID_VALUE;
    }
    else
    {
        /* Set the variable. */
        Context->pointStates.fadeThrdshold = temp;

        /* Set uPointSize dirty. */
        Context->vsUniformDirty.uPointSizeDirty = gcvTRUE;

        result = GL_NO_ERROR;
    }

    gcmFOOTER_ARG("return=%d", result);
    return result;
}


/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  _SetPointParameter
**
**  Specify parameters for point rasterization. _SetPointParameter assigns
**  values to point parameters. _SetPointParameter takes two arguments. Name,
**  specifies which of several parameters will be modified. Value, specifies
**  what value or values will be assigned to the specified parameter.
**
**  INPUT:
**
**      Context
**          Pointer to the current context object.
**
**      Name
**          Specifies the parameter to be updated. Can be either
**          GL_POINT_SIZE_MIN, GL_POINT_SIZE_MAX, GL_POINT_FADE_THRESHOLD_SIZE,
**          or GL_POINT_DISTANCE_ATTENUATION.
**
**      Value
**          Specifies the value that Name will be set to.
**
**      Type
**          Specifies the type of Value.
**
**      ValueArraySize
**          Specifies the size of Value.
**
**  OUTPUT:
**
**      Nothing.
*/

static GLenum _SetPointParameter(
    glsCONTEXT_PTR Context,
    GLenum Name,
    const GLvoid* Value,
    gleTYPE Type,
    gctUINT32 ValueArraySize
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x Name=0x%x Value=0x%x Type=0x%x Count=%d",
                  Context, Name, Value, Type, ValueArraySize);

    if (ValueArraySize > 1)
    {
        switch (Name)
        {
        case GL_POINT_DISTANCE_ATTENUATION:
            result = _SetDistanceAttenuation(Context, Value, Type);
		    gcmFOOTER_ARG("%d", result);
		    return result;
            break;
        }
    }

    switch (Name)
    {
    case GL_POINT_SIZE_MIN:
        result = _SetMinimumPointSize(Context, Value, Type);
        break;

    case GL_POINT_SIZE_MAX:
        result = _SetMaximumPointSize(Context, Value, Type);
        break;

    case GL_POINT_FADE_THRESHOLD_SIZE:
        result = _SetFadeThresholdSize(Context, Value, Type);
        break;

    default:
        result = GL_INVALID_ENUM;
        break;
    }

    gcmFOOTER_ARG("%d", result);
    return result;
}


/*******************************************************************************
**
**  glfSetDefaultPointStates
**
**  Set default point states.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
*/

gceSTATUS glfSetDefaultPointStates(
    glsCONTEXT_PTR Context
    )
{
    GLenum result;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        static gltFRACTYPE value0 = glvFRACZERO;
        static gltFRACTYPE value1 = glvFRACONE;
        static gltFRACTYPE value256 = glvFRAC256;
        static gltFRACTYPE vec1000[] =
            { glvFRACONE, glvFRACZERO, glvFRACZERO, glvFRACZERO };

        /* Default hint. */
        Context->pointStates.hint = GL_DONT_CARE;

        /* Proram point size. */
        glmERR_BREAK(glmTRANSLATEHALSTATUS(gco3D_SetPointSizeEnable(
            Context->hw,
            gcvFALSE
            )));

        /* Disable point sprite. */
        glmERR_BREAK(glfEnablePointSprite(
            Context,
            GL_FALSE
            ));

        /* Default minimum point size. */
        glmERR_BREAK(_SetMinimumPointSize(
            Context,
            &value0,
            glvFRACTYPEENUM
            ));

        /* Default maximum point size. */
        glmERR_BREAK(_SetMaximumPointSize(
            Context,
            &value256,
            glvFRACTYPEENUM
            ));

        /* Set default distance attenuation. */
        glmERR_BREAK(_SetDistanceAttenuation(
            Context,
            &vec1000,
            glvFRACTYPEENUM
            ));

        /* Set fade threshold size. */
        glmERR_BREAK(_SetFadeThresholdSize(
            Context,
            &value1,
            glvFRACTYPEENUM
            ));
    }
    while (gcvFALSE);

    status = glmTRANSLATEGLRESULT(result);

    gcmFOOTER();

    return status;
}


/*******************************************************************************
**
**  glfEnablePointSprite
**
**  Enable point sprite.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Enable
**          Enable flag.
**
**  OUTPUT:
**
**      Nothing.
*/

GLenum glfEnablePointSprite(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);
    /* Invalidate point sprite state. */
    Context->pointStates.spriteDirty = GL_TRUE;

    /* Set point sprite state. */
    Context->hashKey.hashPointSpriteEnabled =
    Context->pointStates.spriteEnable = Enable;

    gcmFOOTER_ARG("return=%d", GL_NO_ERROR);

    return GL_NO_ERROR;
}


/*******************************************************************************
**
**  glfQueryPointState
**
**  Queries point state values.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Name
**          Specifies the symbolic name of the state to get.
**
**      Type
**          Data format.
**
**  OUTPUT:
**
**      Value
**          Points to the data.
*/

GLboolean glfQueryPointState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;

    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Name, Value, Type);

    switch (Name)
    {
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_SMOOTH_POINT_SIZE_RANGE:
        {
            static GLint sizeRange[] = { 1, 256 };
            glfGetFromIntArray(
                sizeRange,
                gcmCOUNTOF(sizeRange),
                Value,
                Type
                );
        }
        break;

    case GL_POINT_DISTANCE_ATTENUATION:
        glfGetFromVector3(
            &Context->pointStates.attenuation,
            Value,
            Type
            );
        break;

    case GL_POINT_FADE_THRESHOLD_SIZE:
        glfGetFromMutant(
            &Context->pointStates.fadeThrdshold,
            Value,
            Type
            );
        break;

    case GL_POINT_SIZE:
        glfGetFromMutable(
            Context->aPointSizeInfo.currValue.value[0],
            Context->aPointSizeInfo.currValue.type,
            Value,
            Type
            );
        break;

    case GL_POINT_SIZE_MAX:
        glfGetFromMutant(
            &Context->pointStates.clampTo,
            Value,
            Type
            );
        break;

    case GL_POINT_SIZE_MIN:
        glfGetFromMutant(
            &Context->pointStates.clampFrom,
            Value,
            Type
            );
        break;

    case GL_POINT_SMOOTH:
        glfGetFromInt(
            Context->pointStates.smooth,
            Value,
            Type
            );
        break;

    case GL_POINT_SMOOTH_HINT:
        glfGetFromEnum(
            Context->pointStates.hint,
            Value,
            Type
            );
        break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("return=%d", result);
    /* Return result. */
    return result;
}


/******************************************************************************\
***************************** Point Management Code ****************************
\******************************************************************************/

/*******************************************************************************
**
**  glPointParameter
**
**  Specify parameters for point rasterization. glPointParameter assigns values
**  to point parameters. glPointParameter takes two arguments. Name, specifies
**  which of several parameters will be modified. Value, specifies what value
**  or values will be assigned to the specified parameter.
**
**  INPUT:
**
**      Name
**          Specifies the parameter to be updated. Can be either
**          GL_POINT_SIZE_MIN, GL_POINT_SIZE_MAX, GL_POINT_FADE_THRESHOLD_SIZE,
**          or GL_POINT_DISTANCE_ATTENUATION.
**
**      Value
**          Specifies the value that Name will be set to.
**
**  OUTPUT:
**
**      Nothing.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_FOG

#if !defined(COMMON_LITE)
GL_API void GL_APIENTRY glPointParameterf(
    GLenum Name,
    GLfloat Value
    )
{
    glmENTER2(glmARGENUM, Name, glmARGFLOAT, Value)
    {
		gcmDUMP_API("${ES11 glPointParameterf 0x%08X 0x%08x}", Name, *(GLuint*)&Value);

        glmPROFILE(context, GLES1_POINTPARAMETERF, 0);
        glmERROR(_SetPointParameter(context, Name, &Value, glvFLOAT, 1));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glPointParameterfv(
    GLenum Name,
    const GLfloat* Value
    )
{
    glmENTER2(glmARGENUM, Name, glmARGPTR, Value)
    {
		gcmDUMP_API("${ES11 glPointParameterfv 0x%08X (0x%08x)", Name, Value);
		gcmDUMP_API_ARRAY(Value, 3);
		gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_POINTPARAMETERFV, 0);
        glmERROR(_SetPointParameter(context, Name, Value, glvFLOAT, 3));
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glPointParameterx(
    GLenum Name,
    GLfixed Value
    )
{
    glmENTER2(glmARGENUM, Name, glmARGFIXED, Value)
    {
		gcmDUMP_API("${ES11 glPointParameterx 0x%08X 0x%08x}", Name, Value);

		glmPROFILE(context, GLES1_POINTPARAMETERX, 0);
        glmERROR(_SetPointParameter(context, Name, &Value, glvFIXED, 1));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glPointParameterxOES(
    GLenum Name,
    GLfixed Value
    )
{
    glmENTER2(glmARGENUM, Name, glmARGFIXED, Value)
    {
		gcmDUMP_API("${ES11 glPointParameterxOES 0x%08X 0x%08x}", Name, Value);

		glmERROR(_SetPointParameter(context, Name, &Value, glvFIXED, 1));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glPointParameterxv(
    GLenum Name,
    const GLfixed* Value
    )
{
    glmENTER2(glmARGENUM, Name, glmARGPTR, Value)
    {
		gcmDUMP_API("${ES11 glPointParameterxv 0x%08X (0x%08x)", Name, Value);
		gcmDUMP_API_ARRAY(Value, 3);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_POINTPARAMETERXV, 0);
        glmERROR(_SetPointParameter(context, Name, Value, glvFIXED, 3));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glPointParameterxvOES(
    GLenum Name,
    const GLfixed* Value
    )
{
    glmENTER2(glmARGENUM, Name, glmARGPTR, Value)
    {
		gcmDUMP_API("${ES11 glPointParameterxvOES 0x%08X (0x%08x)", Name, Value);
		gcmDUMP_API_ARRAY(Value, 3);
		gcmDUMP_API("$}");

		glmERROR(_SetPointParameter(context, Name, Value, glvFIXED, 3));
    }
    glmLEAVE();
}
