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

static GLenum _SetCurrentNormal(
    glsCONTEXT_PTR Context,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Context=0x%x Value=0x%x", Context, Value);
    glfSetVector4(
        &Context->aNormalInfo.currValue,
        Value,
        Type
        );

    /* Set uNormal dirty. */
    Context->vsUniformDirty.uNormalDirty = gcvTRUE;

    gcmFOOTER_ARG("result=%d", GL_NO_ERROR);
    return GL_NO_ERROR;
}

static GLenum _SetCurrentColor(
    glsCONTEXT_PTR Context,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    gcmHEADER_ARG("Context=0x%x Value=0x%x Type=0x%04x", Context, Value, Type);
    glfSetVector4(
        &Context->aColorInfo.currValue,
        Value,
        Type
        );

    Context->aColorInfo.dirty = GL_TRUE;

    Context->hashKey.hashColorCurrValueZero =
        Context->aColorInfo.currValue.zero3;

    /* Set uColor dirty. */
    Context->vsUniformDirty.uColorDirty = gcvTRUE;
    Context->fsUniformDirty.uColorDirty = gcvTRUE;

    gcmFOOTER_ARG("result=%d", GL_NO_ERROR);
    return GL_NO_ERROR;
}

static GLenum _SetCurrentPointSize(
    glsCONTEXT_PTR Context,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLenum result;
    gltFRACTYPE size = glfFracFromRaw(Value, Type);
    gcmHEADER_ARG("Context=0x%x Value=0x%x Type=0x%04x", Context, Value, Type);

    if (size <= glvFRACZERO)
    {
        result = GL_INVALID_VALUE;
    }
    else
    {
        glfSetVector4(
            &Context->aPointSizeInfo.currValue,
            Value,
            Type
            );

        /* Set uPointSize dirty. */
        Context->vsUniformDirty.uPointSizeDirty = gcvTRUE;
        result = GL_NO_ERROR;
    }

    gcmFOOTER_ARG("result=0x%04x", result);
    return result;
}

static GLenum _SetCurrentTexCoord(
    glsCONTEXT_PTR Context,
    GLenum Target,
    const GLvoid* Value,
    gleTYPE Type
    )
{
    GLenum result;
    GLint index = Target - GL_TEXTURE0;
    gcmHEADER_ARG("Context=0x%x Target=0x%04x Value=0x%x Type=0x%04x",
                    Context, Target, Value, Type);

    if ((index < 0) || (index >= Context->texture.pixelSamplers))
    {
        result = GL_INVALID_ENUM;
    }
    else
    {
        glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[index];

        /* Set texture coordinates for the query function. */
        glfSetVector4(
            &sampler->queryCoord,
            Value,
            Type
            );

        /* Determine homogeneous coordinates. */
        glfSetHomogeneousVector4(
            &sampler->homogeneousCoord,
            Value,
            Type
            );

        /* Schedule to be recomputed. */
        sampler->recomputeCoord = GL_TRUE;

        /* Set uTexCoord dirty. */
        Context->vsUniformDirty.uTexCoordDirty = gcvTRUE;
        Context->fsUniformDirty.uTexCoordDirty = gcvTRUE;

        result = GL_NO_ERROR;
    }

    gcmFOOTER_ARG("result=0x%04x", result);
    return result;
}


/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  glfSetStreamParameters
**
**  Configure specified input stream.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Stream
**          Pointer to the stream structure to be initialized
**
**      Type
**          Component data type.
**
**      Components
**          Number of components per vertex.
**
**      Stride
**          OpenGL stream stride.
**
**      Normalize
**          Set to true to enable value normalization.
**
**      Pointer
**          Pointer to the stream data.
**
**      Buffer
**          Pointer to the buffer object.
**
**      BindingType
**          Buffer binding type.
**
**  OUTPUT:
**
**      Nothing.
*/

void glfSetStreamParameters(
    glsCONTEXT_PTR Context,
    glsATTRIBUTEINFO_PTR Stream,
    GLenum Type,
    GLint Components,
    GLsizei Stride,
    gctBOOL Normalize,
    const GLvoid* Pointer,
    const glsNAMEDOBJECT_PTR Buffer,
    gleBUFFERBINDINGS BindingType
    )
{
    /* NOTE: (Components == 0) may come from glMatrixIndexPointerOES
       and glWeightPointerOES. */

    static gcSHADER_TYPE attributeType[] =
    {
        (gcSHADER_TYPE) 0,
        gcSHADER_FRAC_X1,
        gcSHADER_FRAC_X2,
        gcSHADER_FRAC_X3,
        gcSHADER_FRAC_X4
    };

    static gcSHADER_TYPE varyingType[] =
    {
        (gcSHADER_TYPE) 0,
        gcSHADER_FLOAT_X1,
        gcSHADER_FLOAT_X2,
        gcSHADER_FLOAT_X3,
        gcSHADER_FLOAT_X4
    };

    static gcSL_SWIZZLE varyingSwizzle[] =
    {
        (gcSL_SWIZZLE) 0,
        gcSL_SWIZZLE_XXXX,
        gcSL_SWIZZLE_XYYY,
        gcSL_SWIZZLE_XYZZ,
        gcSL_SWIZZLE_XYZW
    };

    gceVERTEX_FORMAT format;
    GLsizei componentSize;
    GLboolean normalize;
    gcsVERTEXARRAY_PTR vertexPtr;

    gcmHEADER_ARG("Context=0x%x Stream=0x%x Type=0x%04x Components=%d Stride=%d "
                    "Normalize=%d Pointer=0x%x Buffer=0%0x BindingType=0x%04x",
                    Context, Stream, Type, Components, Stride, Normalize,
                    Pointer, Buffer, BindingType);

    /* Get internal format and component size. */
    switch (Type)
    {
#if !defined(COMMON_LITE)
    case GL_FLOAT:
        format = gcvVERTEX_FLOAT;
        componentSize = sizeof(GLfloat);
        break;
#endif

    case GL_HALF_FLOAT_OES:
        format = gcvVERTEX_HALF;
        componentSize = sizeof(GLshort);
        break;

    case GL_FIXED:
        format = gcvVERTEX_FIXED;
        componentSize = sizeof(GLfixed);
        break;

    case GL_SHORT:
        format = gcvVERTEX_SHORT;
        componentSize = sizeof(GLshort);
        break;

    case GL_BYTE:
        format = gcvVERTEX_BYTE;
        componentSize = sizeof(GLbyte);
        break;

    case GL_UNSIGNED_BYTE:
        format = gcvVERTEX_UNSIGNED_BYTE;
        componentSize = sizeof(GLubyte);
        break;

    default:
        gcmFATAL("_GetStreamParameters: invalid type %d", Type);
        gcmFOOTER_NO();
        return;
    }

    /* Determine whether attribute normalization is required. */
    normalize
        = Normalize
        && (Type != GL_FIXED)
        && (Type != GL_FLOAT)
        && (Type != GL_HALF_FLOAT_OES);

    /* Mark stream dirty. */
    Stream->dirty = GL_TRUE;

    /* Set stream parameters. */
    Stream->format         = format;
    Stream->normalize      = normalize;
    Stream->components     = Components;
    Stream->attributeType  = attributeType[Components];
    Stream->varyingType    = varyingType[Components];
    Stream->varyingSwizzle = varyingSwizzle[Components];
    Stream->stride         = Stride? Stride : Components * componentSize;
    Stream->pointer        = Pointer;
    Stream->buffer         = Buffer;
    Stream->attributeSize  = Components * componentSize;

    /* Set buffer binding to the stream. */
    if (Buffer)
    {
        /* Cast the object. */
        glsBUFFER_PTR object = (glsBUFFER_PTR) Buffer->object;

        /* Set binding. */
        object->binding[BindingType] = &Stream->buffer;
    }

    /* Store OpenGL parameters. */
    Stream->queryFormat = Type;
    Stream->queryStride = Stride;

    /* Determing index into vertex array attribute array. */
    switch (BindingType)
    {
    case glvVERTEXBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_POSITION];
        break;

    case glvNORMALBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_NORMAL];
        break;

    case glvCOLORBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_COLOR];
        break;

    case glvPOINTSIZEBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_POINT_SIZE];
        break;

    case glvTEX0COORDBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_TEXCOORD0];
        break;

    case glvTEX1COORDBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_TEXCOORD1];
        break;

    case glvTEX2COORDBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_TEXCOORD2];
        break;

    case glvTEX3COORDBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_TEXCOORD3];
        break;

    case glvMATRIXBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_INDEX];
        break;

    case glvWEIGHTBUFFER:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_WEIGHT];
        break;

    case glvTEX0COORDBUFFER_AUX:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_DRAWTEX_TEXCOORD0];
        break;

    case glvTEX1COORDBUFFER_AUX:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_DRAWTEX_TEXCOORD1];
        break;

    case glvTEX2COORDBUFFER_AUX:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_DRAWTEX_TEXCOORD2];
        break;

    case glvTEX3COORDBUFFER_AUX:
        vertexPtr = &Context->attributeArray[gldATTRIBUTE_DRAWTEX_TEXCOORD3];
        break;

    default:
        if (Stream == &Context->aPositionDrawClearRectInfo)
        {
            vertexPtr = &Context->attributeArray[gldATTRIBUTE_DRAWCLEAR_POSITION];
        }
        else
        {
            vertexPtr = &Context->attributeArray[gldATTRIBUTE_DRAWTEX_POSITION];
        }
        break;
}

    /* Get vertex array attribute information. */
    vertexPtr->size       = Components;
    vertexPtr->format     = format;
    vertexPtr->normalized = normalize;
    vertexPtr->stride     = (Stride != 0) ? Stride
                                          : (Components * componentSize);
    vertexPtr->pointer    = Pointer;
    gcmFOOTER_NO();
    return;
}

/*******************************************************************************
**
**  glfInitializeStreams
**
**  Initialize context streams.
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

gceSTATUS glfInitializeStreams(
    glsCONTEXT_PTR Context
    )
{
    static gltFRACTYPE vec1000[] =
    {
        glvFRACONE, glvFRACZERO, glvFRACZERO, glvFRACZERO
    };

    static gltFRACTYPE vec0010[] =
    {
        glvFRACZERO, glvFRACZERO, glvFRACONE, glvFRACZERO
    };

    static gltFRACTYPE vec0001[] =
    {
        glvFRACZERO, glvFRACZERO, glvFRACZERO, glvFRACONE
    };

    static gltFRACTYPE vec1111[] =
    {
        glvFRACONE, glvFRACONE, glvFRACONE, glvFRACONE
    };

    GLenum result;
    GLint i;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        glmERR_BREAK(_SetCurrentNormal(Context, vec0010, glvFRACTYPEENUM));
        glmERR_BREAK(_SetCurrentColor(Context, vec1111, glvFRACTYPEENUM));
        glmERR_BREAK(_SetCurrentPointSize(Context, vec1000, glvFRACTYPEENUM));

        /* Initialize vertex array info. */
        glfSetStreamParameters(
            Context,
            &Context->aPositionInfo,
            glvFRACGLTYPEENUM,
            4,
            0,
            gcvFALSE,
            gcvNULL,
            gcvNULL,
            glvVERTEXBUFFER
            );

        /* Initialize color array info. */
        glfSetStreamParameters(
            Context,
            &Context->aColorInfo,
            glvFRACGLTYPEENUM,
            4,
            0,
            gcvTRUE,
            gcvNULL,
            gcvNULL,
            glvCOLORBUFFER
            );

        /* Initialize normal array info. */
        glfSetStreamParameters(
            Context,
            &Context->aNormalInfo,
            glvFRACGLTYPEENUM,
            3,
            0,
            gcvTRUE,
            gcvNULL,
            gcvNULL,
            glvNORMALBUFFER
            );

        /* Initialize point size array info. */
        glfSetStreamParameters(
            Context,
            &Context->aPointSizeInfo,
            glvFRACGLTYPEENUM,
            1,
            0,
            gcvFALSE,
            gcvNULL,
            gcvNULL,
            glvPOINTSIZEBUFFER
            );

        for (i = 0; i < Context->texture.pixelSamplers; i++)
        {
            glmERR_BREAK(_SetCurrentTexCoord(
                Context, GL_TEXTURE0 + i, vec0001, glvFRACTYPEENUM
                ));

            /* Initialize texcoord array info. */
            glfSetStreamParameters(
                Context,
                &Context->texture.sampler[i].aTexCoordInfo,
                glvFRACGLTYPEENUM,
                4,
                0,
                gcvFALSE,
                gcvNULL,
                gcvNULL,
                (gleBUFFERBINDINGS) (glvTEX0COORDBUFFER + i)
                );
        }
    }
    while (gcvFALSE);

    status = glmTRANSLATEGLRESULT(result);

    gcmFOOTER();

    return status;
}


/*******************************************************************************
**
**  glfQueryVertexState
**
**  Queries vertex state values.
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

GLboolean glfQueryVertexState(
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
#if defined(GL_MAX_ELEMENTS_INDICES) && defined(GL_MAX_ELEMENTS_VERTICES)
    case GL_MAX_ELEMENTS_INDICES:
    case GL_MAX_ELEMENTS_VERTICES:
        {
            gctUINT maxIndex;
            gcoINDEX_QueryCaps(gcvNULL, gcvNULL, gcvNULL, &maxIndex);

            glfGetFromInt(
                maxIndex,
                Value,
                Type
                );
        }
        break;
#endif

    case GL_CURRENT_COLOR:
        if (Type == glvINT)
        {
            /* Normalize when converting to INT. */
            Type = glvNORM;
        }

        glfGetFromVector4(
            &Context->aColorInfo.currValue,
            Value,
            Type
            );
        break;

    case GL_CURRENT_NORMAL:
        glfGetFromVector3(
            &Context->aNormalInfo.currValue,
            Value,
            Type
            );
        break;

    case GL_CURRENT_TEXTURE_COORDS:
        glfGetFromVector4(
            &Context->texture.activeSampler->queryCoord,
            Value,
            Type
            );
        break;

    case GL_NORMALIZE:
        glfGetFromInt(
            Context->normalizeNormal,
            Value,
            Type
            );
        break;

    case GL_RESCALE_NORMAL:
        glfGetFromInt(
            Context->rescaleNormal,
            Value,
            Type
            );
        break;

    case GL_COLOR_ARRAY:
        glfGetFromInt(
            Context->aColorInfo.streamEnabled,
            Value,
            Type
            );
        break;

    case GL_COLOR_ARRAY_SIZE:
        glfGetFromInt(
            Context->aColorInfo.components,
            Value,
            Type
            );
        break;

    case GL_COLOR_ARRAY_STRIDE:
        glfGetFromInt(
            Context->aColorInfo.queryStride,
            Value,
            Type
            );
        break;

    case GL_COLOR_ARRAY_TYPE:
        glfGetFromEnum(
            Context->aColorInfo.queryFormat,
            Value,
            Type
            );
        break;

    case GL_NORMAL_ARRAY:
        glfGetFromInt(
            Context->aNormalInfo.streamEnabled,
            Value,
            Type
            );
        break;

    case GL_NORMAL_ARRAY_STRIDE:
        glfGetFromInt(
            Context->aNormalInfo.queryStride,
            Value,
            Type
            );
        break;

    case GL_NORMAL_ARRAY_TYPE:
        glfGetFromEnum(
            Context->aNormalInfo.queryFormat,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_COORD_ARRAY:
        glfGetFromInt(
            Context->texture.activeClientSampler->aTexCoordInfo.streamEnabled,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_COORD_ARRAY_SIZE:
        glfGetFromInt(
            Context->texture.activeClientSampler->aTexCoordInfo.components,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_COORD_ARRAY_STRIDE:
        glfGetFromInt(
            Context->texture.activeClientSampler->aTexCoordInfo.queryStride,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_COORD_ARRAY_TYPE:
        glfGetFromEnum(
            Context->texture.activeClientSampler->aTexCoordInfo.queryFormat,
            Value,
            Type
            );
        break;

    case GL_VERTEX_ARRAY:
        glfGetFromInt(
            Context->aPositionInfo.streamEnabled,
            Value,
            Type
            );
        break;

    case GL_VERTEX_ARRAY_SIZE:
        glfGetFromInt(
            Context->aPositionInfo.components,
            Value,
            Type
            );
        break;

    case GL_VERTEX_ARRAY_STRIDE:
        glfGetFromInt(
            Context->aPositionInfo.queryStride,
            Value,
            Type
            );
        break;

    case GL_VERTEX_ARRAY_TYPE:
        glfGetFromEnum(
            Context->aPositionInfo.queryFormat,
            Value,
            Type
            );
        break;

    case GL_POINT_SIZE_ARRAY_OES:
        glfGetFromInt(
            Context->aPointSizeInfo.streamEnabled,
            Value,
            Type
            );
        break;

    case GL_POINT_SIZE_ARRAY_STRIDE_OES:
        glfGetFromInt(
            Context->aPointSizeInfo.queryStride,
            Value,
            Type
            );
        break;

    case GL_POINT_SIZE_ARRAY_TYPE_OES:
        glfGetFromEnum(
            Context->aPointSizeInfo.queryFormat,
            Value,
            Type
            );
        break;

    case GL_MATRIX_INDEX_ARRAY_OES:
        glfGetFromInt(
            Context->aMatrixIndexInfo.streamEnabled,
            Value,
            Type
            );
        break;

    case GL_MATRIX_INDEX_ARRAY_SIZE_OES:
        glfGetFromInt(
            Context->aMatrixIndexInfo.components,
            Value,
            Type
            );
        break;

    case GL_MATRIX_INDEX_ARRAY_STRIDE_OES:
        glfGetFromInt(
            Context->aMatrixIndexInfo.queryStride,
            Value,
            Type
            );
        break;

    case GL_MATRIX_INDEX_ARRAY_TYPE_OES:
        glfGetFromEnum(
            Context->aMatrixIndexInfo.queryFormat,
            Value,
            Type
            );
        break;

    case GL_WEIGHT_ARRAY_OES:
        glfGetFromInt(
            Context->aWeightInfo.streamEnabled,
            Value,
            Type
            );
        break;

    case GL_WEIGHT_ARRAY_SIZE_OES:
        glfGetFromInt(
            Context->aWeightInfo.components,
            Value,
            Type
            );
        break;

    case GL_WEIGHT_ARRAY_STRIDE_OES:
        glfGetFromInt(
            Context->aWeightInfo.queryStride,
            Value,
            Type
            );
        break;

    case GL_WEIGHT_ARRAY_TYPE_OES:
        glfGetFromEnum(
            Context->aWeightInfo.queryFormat,
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
************************* OpenGL Vertex Management Code ************************
\******************************************************************************/

/*******************************************************************************
**
**  glNormal
**
**  The current normal is set to the given coordinates whenever glNormal is
**  issued. Byte, short, or integer arguments are converted to floating-point
**  with a linear mapping that maps the most positive representable integer
**  value to 1.0, and the most negative representable integer value to -1.0.
**
**  Normals specified with glNormal need not have unit length. If GL_NORMALIZE
**  is enabled, then normals of any length specified with glNormal are
**  normalized after transformation. If GL_RESCALE_NORMAL is enabled, normals
**  are scaled by a scaling factor derived from the modelview matrix.
**  GL_RESCALE_NORMAL requires that the originally specified normals were of
**  unit length, and that the modelview matrix contain only uniform scales for
**  proper results. To enable and disable normalization, call glEnable and
**  glDisable with either GL_NORMALIZE or GL_RESCALE_NORMAL.
**  Normalization is initially disabled.
**
**  INPUT:
**
**      X, Y, Z
**          Specify the x, y, and z coordinates of the new current normal.
**          The initial value is (0, 0, 1).
**
**  OUTPUT:
**
**      Nothing.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_STREAM

GL_API void GL_APIENTRY glNormal3x(
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
    glmENTER3(glmARGFIXED, X, glmARGFIXED, Y, glmARGFIXED, Z)
    {
        GLfixed normal[4];

		gcmDUMP_API("${ES11 glNormal3x 0x%08X 0x%08X 0x%08X}", X, Y, Z);

        glmPROFILE(context, GLES1_NORMAL3X, 0);

        normal[0] = X;
        normal[1] = Y;
        normal[2] = Z;
        normal[3] = gcvONE_X;

        glmERROR(_SetCurrentNormal(context, normal, glvFIXED));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glNormal3xOES(
    GLfixed X,
    GLfixed Y,
    GLfixed Z
    )
{
    glmENTER3(glmARGFIXED, X, glmARGFIXED, Y, glmARGFIXED, Z)
    {
        GLfixed normal[4];

		gcmDUMP_API("${ES11 glNormal3xOES 0x%08X 0x%08X 0x%08X}", X, Y, Z);

        normal[0] = X;
        normal[1] = Y;
        normal[2] = Z;
        normal[3] = gcvONE_X;

        glmERROR(_SetCurrentNormal(context, normal, glvFIXED));
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glNormal3f(
    GLfloat X,
    GLfloat Y,
    GLfloat Z
    )
{
    glmENTER3(glmARGFLOAT, X, glmARGFLOAT, Y, glmARGFLOAT, Z)
    {
        GLfloat normal[4];

		gcmDUMP_API("${ES11 glNormal3f 0x%08X 0x%08X 0x%08X}", *(GLuint*)&X, *(GLuint*)&Y, *(GLuint*)&Z);

		glmPROFILE(context, GLES1_NORMAL3F, 0);
        normal[0] = X;
        normal[1] = Y;
        normal[2] = Z;
        normal[3] = 1.0f;

        glmERROR(_SetCurrentNormal(context, normal, glvFLOAT));
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glColor
**
**  The GL stores a current four-valued RGBA color. glColor sets a new
**  four-valued RGBA color.
**
**  Current color values are stored in fixed-point or floating-point. In case
**  the values are stored in floating-point, the mantissa and exponent sizes
**  are unspecified.
**
**  Neither fixed-point nor floating-point values are clamped to the range
**  [0, 1] before the current color is updated. However, color components are
**  clamped to this range before they are interpolated or written into
**  the color buffer.
**
**  INPUT:
**
**      Red, Green, Blue, Alpha
**          Specify new red, green, blue, and alpha values for the current
**          color. The initial value is (1, 1, 1, 1).
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glColor4x(
    GLfixed Red,
    GLfixed Green,
    GLfixed Blue,
    GLfixed Alpha
    )
{
    glmENTER4(glmARGFIXED, Red, glmARGFIXED, Green,
             glmARGFIXED, Blue, glmARGFIXED, Alpha)
    {
        GLfixed color[4];

		gcmDUMP_API("${ES11 glColor4x 0x%08X 0x%08X 0x%08X 0x%08X}", Red, Green, Blue, Alpha);

        glmPROFILE(context, GLES1_COLOR4X, 0);

        color[0] = Red;
        color[1] = Green;
        color[2] = Blue;
        color[3] = Alpha;

        glmERROR(_SetCurrentColor(context, color, glvFIXED));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glColor4xOES(
    GLfixed Red,
    GLfixed Green,
    GLfixed Blue,
    GLfixed Alpha
    )
{
    glmENTER4(glmARGFIXED, Red, glmARGFIXED, Green,
             glmARGFIXED, Blue, glmARGFIXED, Alpha)
    {
        GLfixed color[4];

		gcmDUMP_API("${ES11 glColor4xOES 0x%08X 0x%08X 0x%08X 0x%08X}", Red, Green, Blue, Alpha);

        color[0] = Red;
        color[1] = Green;
        color[2] = Blue;
        color[3] = Alpha;

        glmERROR(_SetCurrentColor(context, color, glvFIXED));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glColor4ub(
    GLubyte Red,
    GLubyte Green,
    GLubyte Blue,
    GLubyte Alpha
    )
{
    glmENTER4(glmARGUINT, Red, glmARGUINT, Green,
             glmARGUINT, Blue, glmARGUINT, Alpha)
    {
        gltFRACTYPE color[4];

		gcmDUMP_API("${ES11 glColor4ub 0x%08X 0x%08X 0x%08X 0x%08X}", Red, Green, Blue, Alpha);

        glmPROFILE(context, GLES1_COLOR4UB, 0);
        color[0] = glmFRACMULTIPLY(Red,   glvFRACONEOVER255);
        color[1] = glmFRACMULTIPLY(Green, glvFRACONEOVER255);
        color[2] = glmFRACMULTIPLY(Blue,  glvFRACONEOVER255);
        color[3] = glmFRACMULTIPLY(Alpha, glvFRACONEOVER255);

        glmERROR(_SetCurrentColor(context, color, glvFRACTYPEENUM));
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glColor4f(
    GLfloat Red,
    GLfloat Green,
    GLfloat Blue,
    GLfloat Alpha
    )
{
    glmENTER4(glmARGFLOAT, Red, glmARGFLOAT, Green,
             glmARGFLOAT, Blue, glmARGFLOAT, Alpha)
    {
        GLfloat color[4];

		gcmDUMP_API("${ES11 glColor4f 0x%08X 0x%08X 0x%08X 0x%08X}",
			*(GLuint*)&Red, *(GLuint*)&Green, *(GLuint*)&Blue, *(GLuint*)&Alpha);

        glmPROFILE(context, GLES1_COLOR4F, 0);
        color[0] = Red;
        color[1] = Green;
        color[2] = Blue;
        color[3] = Alpha;

        glmERROR(_SetCurrentColor(context, color, glvFLOAT));
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glPointSize
**
**  glPointSize specifies the rasterized diameter of both aliased and
**  antialiased points. Using a point size other than 1 has different effects,
**  depending on whether point antialiasing is enabled. To enable and disable
**  point antialiasing, call glEnable and glDisable with argument
**  GL_POINT_SMOOTH. Point antialiasing is initially disabled.
**
**  If point antialiasing is disabled, the actual size is determined by rounding
**  the supplied size to the nearest integer. (If the rounding results in the
**  value 0, it is as if the point size were 1.) If the rounded size is odd,
**  then the center point (X, Y) of the pixel fragment that represents the point
**  is computed as
**                      ( [ Xw ] + 1/2, [ Yw ] + 1/2 )
**  where w subscripts indicate window coordinates. All pixels that lie within
**  the square grid of the rounded size centered at (X, Y) make up the fragment.
**  If the size is even, the center point is
**                      ( [ Xw + 1/2 ] , [ Yw + 1/2 ] )
**  and the rasterized fragment's centers are the half-integer window
**  coordinates within the square of the rounded size centered at (X, Y). All
**  pixel fragments produced in rasterizing a nonantialiased point are assigned
**  the same associated data, that of the vertex corresponding to the point.
**
**  If antialiasing is enabled, then point rasterization produces a fragment for
**  each pixel square that intersects the region lying within the circle having
**  diameter equal to the current point size and centered at the point's
**  (Xw, Yw). The coverage value for each fragment is the window coordinate area
**  of the intersection of the circular region with the corresponding pixel
**  square. This value is saved and used in the final rasterization step. The
**  data associated with each fragment is the data associated with the point
**  being rasterized.
**
**  Not all sizes are supported when point antialiasing is enabled. If an
**  unsupported size is requested, the nearest supported size is used. Only size
**  1 is guaranteed to be supported; others depend on the implementation.
**  To query the range of supported sizes, call glGet with the argument
**  GL_SMOOTH_POINT_SIZE_RANGE. For aliased points, query the supported ranges
**  glGet with the argument GL_ALIASED_POINT_SIZE_RANGE.
**
**  A non-antialiased point size may be clamped to an implementation-dependent
**  maximum. Although this maximum cannot be queried, it must be no less than
**  the maximum value for antialiased points, rounded to the nearest integer
**  value.
**
**  INPUT:
**
**      Size
**          Specifies the diameter of rasterized points. The initial value is 1.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_FOG

#if !defined(COMMON_LITE)
GL_API void GL_APIENTRY glPointSize(
    GLfloat Size
    )
{
    glmENTER1(glmARGFLOAT, Size)
    {
        GLfloat pointSize[4];

		gcmDUMP_API("${ES11 glPointSize 0x%08X}", *(GLuint*)&Size);

        glmPROFILE(context, GLES1_POINTSIZE, 0);

        pointSize[0] = Size;
        pointSize[1] = 0.0f;
        pointSize[2] = 0.0f;
        pointSize[3] = 0.0f;

        glmERROR(_SetCurrentPointSize(context, pointSize, glvFLOAT));
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glPointSizex(
    GLfixed Size
    )
{
    glmENTER1(glmARGFIXED, Size)
    {
        GLfixed pointSize[4];

		gcmDUMP_API("${ES11 glPointSizex 0x%08X}", Size);

        glmPROFILE(context, GLES1_POINTSIZEX, 0);
        pointSize[0] = Size;
        pointSize[1] = 0;
        pointSize[2] = 0;
        pointSize[3] = 0;

        glmERROR(_SetCurrentPointSize(context, pointSize, glvFIXED));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glPointSizexOES(
    GLfixed Size
    )
{
    glmENTER1(glmARGFIXED, Size)
    {
        GLfixed pointSize[4];

		gcmDUMP_API("${ES11 glPointSizexOES 0x%08X}", Size);

        pointSize[0] = Size;
        pointSize[1] = 0;
        pointSize[2] = 0;
        pointSize[3] = 0;

        glmERROR(_SetCurrentPointSize(context, pointSize, glvFIXED));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glMultiTexCoord
**
**  glMultiTexCoord specifies the four texture coordinates as (s, t, r, q).
**
**  The current texture coordinates are part of the data that is associated
**  with each vertex.
**
**  It is always the case that GL_TEXTUREi = GL_TEXTURE0 + i.
**
**  INPUT:
**
**      GLenum Target
**          Specifies texture unit whose coordinates should be modified.
**          The number of texture units is implementation dependent, but must
**          be at least one. Must be one of GL_TEXTUREi, where
**          0 <= i < GL_MAX_TEXTURE_UNITS, which is an implementation-dependent
**          value.
**
**      S, T, R, Q
**          Specify s, t, r, and q texture coordinates for target texture unit.
**          The initial value is (0, 0, 0, 1).
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_TEXTURE

GL_API void GL_APIENTRY glMultiTexCoord4x(
    GLenum Target,
    GLfixed S,
    GLfixed T,
    GLfixed R,
    GLfixed Q
    )
{
    glmENTER5(glmARGENUM, Target, glmARGFIXED, S, glmARGFIXED, T,
              glmARGFIXED, R, glmARGFIXED, Q)
    {
        GLfixed texCoord[4];

		gcmDUMP_API("${ES11 glMultiTexCoord4x 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}", Target, S, T, R, Q);

        glmPROFILE(context, GLES1_MULTITEXCOORD4X, 0);

        texCoord[0] = S;
        texCoord[1] = T;
        texCoord[2] = R;
        texCoord[3] = Q;

        glmERROR(_SetCurrentTexCoord(
            context, Target, texCoord, glvFIXED
            ));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glMultiTexCoord4xOES(
    GLenum Target,
    GLfixed S,
    GLfixed T,
    GLfixed R,
    GLfixed Q
    )
{
    glmENTER5(glmARGENUM, Target, glmARGFIXED, S, glmARGFIXED, T,
              glmARGFIXED, R, glmARGFIXED, Q)
    {
        GLfixed texCoord[4];

		gcmDUMP_API("${ES11 glMultiTexCoord4xOES 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}", Target, S, T, R, Q);

        texCoord[0] = S;
        texCoord[1] = T;
        texCoord[2] = R;
        texCoord[3] = Q;

        glmERROR(_SetCurrentTexCoord(
            context, Target, texCoord, glvFIXED
            ));
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glMultiTexCoord4f(
    GLenum Target,
    GLfloat S,
    GLfloat T,
    GLfloat R,
    GLfloat Q
    )
{
    glmENTER5(glmARGENUM, Target,
             glmARGFLOAT, S, glmARGFLOAT, T, glmARGFLOAT, R, glmARGFLOAT, Q)
    {
        GLfloat texCoord[4];

		gcmDUMP_API("${ES11 glMultiTexCoord4f 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
			Target, *(GLuint*)&S, *(GLuint*)&T, *(GLuint*)&R, *(GLuint*)&Q);

        glmPROFILE(context, GLES1_MULTITEXCOORD4F, 0);
        texCoord[0] = S;
        texCoord[1] = T;
        texCoord[2] = R;
        texCoord[3] = Q;

        glmERROR(_SetCurrentTexCoord(
            context, Target, texCoord, glvFLOAT
            ));
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glVertexPointer
**
**  glVertexPointer specifies the location and data of an array of vertex
**  coordinates to use when rendering. size specifies the number of coordinates
**  per vertex and type the data type of the coordinates. stride specifies the
**  byte stride from one vertex to the next allowing vertices and attributes to
**  be packed into a single array or stored in separate arrays. (Single-array
**  storage may be more efficient on some implementations.)
**
**  When a vertex array is specified, size, type, stride, and pointer are saved
**  as client-side state.
**
**  If the vertex array is enabled, it is used when glDrawArrays, or
**  glDrawElements is called. To enable and disable the vertex array, call
**  glEnableClientState and glDisableClientState with the argument
**  GL_VERTEX_ARRAY. The vertex array is initially disabled and isn't accessed
**  when glDrawArrays or glDrawElements is called.
**
**  Use glDrawArrays to construct a sequence of primitives (all of the same
**  type) from prespecified vertex and vertex attribute arrays. Use
**  glDrawElements to construct a sequence of primitives by indexing vertices
**  and vertex attributes.
**
**  INPUT:
**
**      GLint Size
**          Specifies the number of coordinates per vertex. Must be 2, 3, or
**          4. The initial value is 4.
**
**      GLenum Type
**          Specifies the data type of each vertex coordinate in the array.
**          Symbolic constants GL_BYTE, GL_SHORT, and GL_FIXED, are accepted.
**          However, the initial value is GL_FLOAT.
**          The common profile accepts the symbolic constant GL_FLOAT as well.
**
**      GLsizei Stride
**          Specifies the byte offset between consecutive vertices. If stride
**          is 0, the vertices are understood to be tightly packed in the
**          array. The initial value is 0.
**
**      const GLvoid* Pointer
**          Specifies a pointer to the first coordinate of the first vertex
**          in the array. The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_STREAM

GL_API void GL_APIENTRY glVertexPointer(
    GLint Size,
    GLenum Type,
    GLsizei Stride,
    const GLvoid* Pointer
    )
{
    glmENTER4(glmARGINT, Size, glmARGENUM, Type,
             glmARGINT, Stride, glmARGPTR, Pointer)
    {
		gcmDUMP_API("${ES11 glVertexPointer 0x%08X 0x%08X 0x%08X 0x%08X}", Size, Type, Stride, Pointer);

		glmPROFILE(context, GLES1_VERTEXPOINTER, 0);

        /* Validate size. */
        if ((Size < 2) || (Size > 4)
        /* Validate stride. */
         || (Stride < 0))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Validate type. */
        if (
                (Type != GL_BYTE)
            &&  (Type != GL_SHORT)
            &&  (Type != GL_FIXED)
            &&  (Type != GL_HALF_FLOAT_OES)

#if !defined(COMMON_LITE)
            &&  (Type != GL_FLOAT)
#endif
            )
        {
            /* Invalid enum. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set stream parameters. */
        glfSetStreamParameters(
            context,
            &context->aPositionInfo,
            Type,
            Size,
            Stride,
            gcvFALSE,
            Pointer,
            context->arrayBuffer,
            glvVERTEXBUFFER
            );
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glNormalPointer
**
**  glNormalPointer specifies the location and data of an array of normals to
**  use when rendering. type specifies the data type of the normal coordinates
**  and stride gives the byte stride from one normal to the next, allowing
**  vertices and attributes to be packed into a single array or stored in
**  separate arrays. (Single-array storage may be more efficient on some
**  implementations.) When a normal array is specified, type, stride, and
**  pointer are saved as client-side state.
**
**  If the normal array is enabled, it is used when glDrawArrays or
**  glDrawElements is called. To enable and disable the normal array, call
**  glEnableClientState and glDisableClientState with the argument
**  GL_NORMAL_ARRAY. The normal array is initially disabled and isn't accessed
**  when glDrawArrays or glDrawElements is called.
**
**  Use glDrawArrays to construct a sequence of primitives (all of the same
**  type) from prespecified vertex and vertex attribute arrays. Use
**  glDrawElements to construct a sequence of primitives by indexing vertices
**  and vertex attributes.
**
**  INPUT:
**
**      Type
**          Specifies the data type of each coordinate in the array.
**          Symbolic constants GL_BYTE, GL_SHORT, and GL_FIXED are accepted.
**          However, the initial value is GL_FLOAT.
**          The common profile accepts the symbolic constant GL_FLOAT as well.
**
**      Stride
**          Specifies the byte offset between consecutive normals. If stride is
**          0, the normals are understood to be tightly packed in the array.
**          The initial value is 0.
**
**      Pointer
**          Specifies a pointer to the first coordinate of the first normal in
**          the array. The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glNormalPointer(
    GLenum Type,
    GLsizei Stride,
    const GLvoid* Pointer
    )
{
    glmENTER3(glmARGENUM, Type,
             glmARGINT, Stride, glmARGPTR, Pointer)
    {
        glmPROFILE(context, GLES1_NORMALPOINTER, 0);

		gcmDUMP_API("${ES11 glNormalPointer 0x%08X 0x%08X 0x%08X}", Type, Stride, Pointer);

        /* Validate stride. */
        if (Stride < 0)
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

		/* Validate type. */
        if (
                (Type != GL_BYTE)
            &&  (Type != GL_SHORT)
            &&  (Type != GL_FIXED)
            &&  (Type != GL_HALF_FLOAT_OES)

#if !defined(COMMON_LITE)
            &&  (Type != GL_FLOAT)
#endif
            )
        {
            /* Invalid enum. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set stream parameters. */
        glfSetStreamParameters(
            context,
            &context->aNormalInfo,
            Type,
            3,
            Stride,
            gcvTRUE,
            Pointer,
            context->arrayBuffer,
            glvNORMALBUFFER
            );
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glColorPointer
**
**  glColorPointer specifies the location and data of an array of color
**  components to use when rendering. size specifies the number of components
**  per color, and must be 4. type specifies the data type of each color
**  component, and stride specifies the byte stride from one color to the next
**  allowing vertices and attributes to be packed into a single array or stored
**  in separate arrays. (Single-array storage may be more efficient on some
**  implementations.)
**
**  When a color array is specified, size, type, stride, and pointer are saved
**  as client-side state.
**
**  If the color array is enabled, it is used when glDrawArrays, or
**  glDrawElements is called. To enable and disable the color array, call
**  glEnableClientState and glDisableClientState with the argument
**  GL_COLOR_ARRAY. The color array is initially disabled and isn't accessed
**  when glDrawArrays or glDrawElements is called.
**
**  Use glDrawArrays to construct a sequence of primitives (all of the same
**  type) from prespecified vertex and vertex attribute arrays. Use
**  glDrawElements to construct a sequence of primitives by indexing vertices
**  and vertex attributes.
**
**  INPUT:
**
**      Size
**          Specifies the number of components per color. Must be 4.
**          The initial value is 4.
**
**      Type
**          Specifies the data type of each color component in the array.
**          Symbolic constants GL_UNSIGNED_BYTE and GL_FIXED are accepted.
**          However, the initial value is GL_FLOAT. The common profile accepts
**          the symbolic constant GL_FLOAT as well.
**
**      Stride
**          Specifies the byte offset between consecutive colors. If stride
**          is 0, the colors are understood to be tightly packed in the array.
**          The initial value is 0.
**
**      Pointer
**          Specifies a pointer to the first component of the first color
**          element in the array. The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glColorPointer(
    GLint Size,
    GLenum Type,
    GLsizei Stride,
    const GLvoid* Pointer
    )
{
    glmENTER4(glmARGINT, Size, glmARGENUM, Type,
             glmARGINT, Stride, glmARGPTR, Pointer)
    {
		gcmDUMP_API("${ES11 glColorPointer 0x%08X 0x%08X 0x%08X 0x%08X}", Size, Type, Stride, Pointer);

        glmPROFILE(context, GLES1_COLORPOINTER, 0);
        /* Validate size. */
        if ((Size != 4)
        /* Validate stride. */
         || (Stride < 0))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Validate type. */
        if (
                (Type != GL_UNSIGNED_BYTE)
            &&  (Type != GL_FIXED)
            &&  (Type != GL_HALF_FLOAT_OES)

#if !defined(COMMON_LITE)
            &&  (Type != GL_FLOAT)
#endif
            )
        {
            /* Invalid enum. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set stream parameters. */
        glfSetStreamParameters(
            context,
            &context->aColorInfo,
            Type,
            4,
            Stride,
            gcvTRUE,
            Pointer,
            context->arrayBuffer,
            glvCOLORBUFFER
            );
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glPointSizePointerOES
**
**  glPointSizePointer specifies the location and data of an array of point
**  sizes to use when rendering points. type the data type of the coordinates.
**  stride specifies the byte stride from one point size to the next, allowing
**  vertices and attributes to be packed into a single array or stored in
**  separate arrays. (Single-array storage may be more efficient on some
**  implementations.)
**
**  The point sizes supplied in the point size arrays will be the sizes used to
**  render both points and point sprites.
**
**  Distance-based attenuation works in conjunction with
**  GL_POINT_SIZE_ARRAY_OES. If distance-based attenuation is enabled
**  the point size from the point size array will be attenuated as
**  defined by glPointParameter, to compute the final point size.
**
**  When a point size array is specified, type, stride, and pointer are saved
**  as client-side state.
**
**  If the point size array is enabled, it is used to control the sizes used to
**  render points and point sprites. To enable and disable the point size array,
**  call glEnableClientState and glDisableClientState with the argument
**  GL_POINT_SIZE_ARRAY_OES. The point size array is initially disabled.
**
**  INPUT:
**
**      Type
**          Specifies the data type of each point size in the array. Symbolic
**          constant GL_FIXED is accepted. However, the common profile also
**          accepts the symbolic constant GL_FLOAT as well. The initial value
**          is GL_FIXED for the common lite profile, or GL_FLOAT for the common
**          profile.
**
**      Stride
**          Specifies the byte offset between consecutive point sizes.
**          If stride is 0, the point sizes are understood to be tightly
**          packed in the array. The initial value is 0.
**
**      Pointer
**          Specifies a pointer to the point size of the first vertex in
**          the array. The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_EXTENTION

GL_API void GL_APIENTRY glPointSizePointerOES(
    GLenum Type,
    GLsizei Stride,
    const GLvoid* Pointer
    )
{
    glmENTER3(glmARGENUM, Type, glmARGINT, Stride, glmARGPTR, Pointer)
    {
		gcmDUMP_API("${ES11 glPointSizePointerOES 0x%08X 0x%08X 0x%08X}", Type, Stride, Pointer);

        /* Validate stride. */
        if (Stride < 0)
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Validate type. */
        if (
                (Type != GL_FIXED)
            &&  (Type != GL_HALF_FLOAT_OES)

#if !defined(COMMON_LITE)
            &&  (Type != GL_FLOAT)
#endif
            )
        {
            /* Invalid enum. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set stream parameters. */
        glfSetStreamParameters(
            context,
            &context->aPointSizeInfo,
            Type,
            1,
            Stride,
            gcvFALSE,
            Pointer,
            context->arrayBuffer,
            glvPOINTSIZEBUFFER
            );
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glMatrixIndexPointerOES (GL_OES_matrix_palette)
**
**  glMatrixIndexPointer specifies the location and data of an array of matrix
**  indices to use when rendering. size specifies the number of matrix indices
**  per vertex and type the data type of the coordinates. stride specifies the
**  byte stride from one matrix index to the next, allowing vertices and
**  attributes to be packed into a single array or stored in separate arrays.
**  (Single-array storage may be more efficient on some implementations.)
**
**  These matrices indices are used to blend corresponding matrices for a given
**  vertex.
**
**  When a matrix index array is specified, size, type, stride, and pointer are
**  saved as client-side state.
**
**  If the matrix index array is enabled, it is used when glDrawArrays, or
**  glDrawElements is called. To enable and disable the vertex array, call
**  glEnableClientState and glDisableClientState with the argument
**  GL_MATRIX_INDEX_ARRAY_OES. The matrix index array is initially disabled and
**  isn't accessed when glDrawArrays or glDrawElements is called.
**
**  Use glDrawArrays to construct a sequence of primitives (all of the same
**  type) from prespecified vertex and vertex attribute arrays. Use
**  glDrawElements to construct a sequence of primitives by indexing vertices
**  and vertex attributes.
**
**  INPUT:
**
**      Size
**          Specifies the number of matrix indices per vertex. Must be less
**          than or equal to GL_MAX_VERTEX_UNITS_OES. The initial value is 0.
**
**      Type
**          Specifies the data type of each matrix index in the array.
**          Symbolic constant GL_UNSIGNED_BYTE is accepted.
**          The initial value is GL_UNSIGNED_BYTE.
**
**      Stride
**          Specifies the byte offset between consecutive matrix indices.
**          If stride is 0, the matrix indices are understood to be tightly
**          packed in the array. The initial value is 0.
**
**      Pointer
**          Specifies a pointer to the first matrix index of the first vertex
**          in the array. The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glMatrixIndexPointerOES(
    GLint Size,
    GLenum Type,
    GLsizei Stride,
    const GLvoid* Pointer
    )
{
    glmENTER4(glmARGINT, Size, glmARGENUM, Type,
             glmARGINT, Stride, glmARGPTR, Pointer)
    {
		gcmDUMP_API("${ES11 glMatrixIndexPointerOES 0x%08X 0x%08X 0x%08X 0x%08X}", Size, Type, Stride, Pointer);

        /* Validate size. */
        if ((Size < 0) || (Size > glvMAX_VERTEX_UNITS)
        /* Validate stride. */
         || (Stride < 0))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Validate type. */
        if (Type != GL_UNSIGNED_BYTE)
        {
            /* Invalid enum. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set stream parameters. */
        glfSetStreamParameters(
            context,
            &context->aMatrixIndexInfo,
            Type,
            Size,
            Stride,
            gcvFALSE,
            Pointer,
            context->arrayBuffer,
            glvMATRIXBUFFER
            );

        /* Update the hash key. */
        context->hashKey.hashMatrixIndexStreamComponents = Size;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glWeightPointerOES (GL_OES_matrix_palette)
**
**  glWeightPointer specifies the location and data of an array of weights to
**  use when rendering. size specifies the number of weights per vertex and type
**  the data type of the coordinates. stride specifies the byte stride from one
**  weight to the next allowing vertices and attributes to be packed into a
**  single array or stored in separate arrays. (Single-array storage may be more
**  efficient on some implementations.)
**
**  These weights are used to blend corresponding matrices for a given vertex.
**
**  When a weight array is specified, size, type, stride, and pointer are saved
**  as client-side state.
**
**  If the weight array is enabled, it is used when glDrawArrays, or
**  glDrawElements is called. To enable and disable the vertex array, call
**  glEnableClientState and glDisableClientState with the argument
**  GL_WEIGHT_ARRAY_OES. The weight array is initially disabled and isn't
**  accessed when glDrawArrays or glDrawElements is called.
**
**  Use glDrawArrays to construct a sequence of primitives (all of the same
**  type) from prespecified vertex and vertex attribute arrays. Use
**  glDrawElements to construct a sequence of primitives by indexing vertices
**  and vertex attributes.
**
**  INPUT:
**
**      Size
**          Specifies the number of weights per vertex. Must be less than or
**          equal to GL_MAX_VERTEX_UNITS_OES. The initial value is 0.
**
**      Type
**          Specifies the data type of each weight in the array. Symbolic
**          constant GL_FIXED is accepted. However, the common profile also
**          accepts the symbolic constant GL_FLOAT as well. The initial value
**          is GL_FIXED for the common lite profile, or GL_FLOAT for the common
**          profile.
**
**      Stride
**          Specifies the byte offset between consecutive weights. If stride
**          is 0, the weights are understood to be tightly packed in the array.
**          The initial value is 0.
**
**      Pointer
**          Specifies a pointer to the first weight of the first vertex in
**          the array. The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glWeightPointerOES(
    GLint Size,
    GLenum Type,
    GLsizei Stride,
    const GLvoid* Pointer
    )
{
    glmENTER4(glmARGINT, Size, glmARGENUM, Type,
             glmARGINT, Stride, glmARGPTR, Pointer)
    {
		gcmDUMP_API("${ES11 glWeightPointerOES 0x%08X 0x%08X 0x%08X 0x%08X}", Size, Type, Stride, Pointer);

        /* Validate size. */
        if ((Size < 0) || (Size > glvMAX_VERTEX_UNITS)
        /* Validate stride. */
         || (Stride < 0))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Validate type. */
        if (
                (Type != GL_FIXED)
            &&  (Type != GL_HALF_FLOAT_OES)

#if !defined(COMMON_LITE)
            &&  (Type != GL_FLOAT)
#endif
            )
        {
            /* Invalid enum. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Set stream parameters. */
        glfSetStreamParameters(
            context,
            &context->aWeightInfo,
            Type,
            Size,
            Stride,
            gcvFALSE,
            Pointer,
            context->arrayBuffer,
            glvWEIGHTBUFFER
            );

        /* Update the hash key. */
        context->hashKey.hashWeightStreamComponents = Size;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glTexCoordPointer
**
**  glTexCoordPointer specifies the location and data of an array of texture
**  coordinates to use when rendering. size specifies the number of coordinates
**  per element, and must be 2, 3, or 4. type specifies the data type of each
**  texture coordinate and stride specifies the byte stride from one array
**  element to the next allowing vertices and attributes to be packed into
**  a single array or stored in separate arrays. (Single-array storage may be
**  more efficient on some implementations.)
**
**  When a texture coordinate array is specified, size, type, stride, and
**  pointer are saved as client-side state.
**
**  If the texture coordinate array is enabled, it is used when glDrawArrays,
**  or glDrawElements is called. To enable and disable the texture coordinate
**  array for the client-side active texture unit, call glEnableClientState and
**  glDisableClientState with the argument GL_TEXTURE_COORD_ARRAY. The texture
**  coordinate array is initially disabled for all client-side active texture
**  units and isn't accessed when glDrawArrays or glDrawElements is called.
**
**  Use glDrawArrays to construct a sequence of primitives (all of the same
**  type) from prespecified vertex and vertex attribute arrays. Use
**  glDrawElements to construct a sequence of primitives by indexing vertices
**  and vertex attributes.
**
**  INPUT:
**
**      Size
**          Specifies the number of coordinates per array element. Must be
**          2, 3 or 4. The initial value is 4.
**
**      Type
**          Specifies the data type of each texture coordinate. Symbolic
**          constants GL_BYTE, GL_SHORT, and GL_FIXED are accepted.
**          However, the initial value is GL_FLOAT. The common profile
**          accepts the symbolic constant GL_FLOAT as well.
**
**      Stride
**          Specifies the byte offset between consecutive array elements.
**          If stride is 0, the array elements are understood to be tightly
**          packed. The initial value is 0.
**
**      Pointer
**          Specifies a pointer to the first coordinate of the first element
**          in the array. The initial value is 0.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_TEXTURE

GL_API void GL_APIENTRY glTexCoordPointer(
    GLint Size,
    GLenum Type,
    GLsizei Stride,
    const GLvoid* Pointer
    )
{
    glmENTER4(glmARGUINT, Size, glmARGENUM, Type,
              glmARGUINT, Stride, glmARGPTR, Pointer)
    {
        glsTEXTURESAMPLER_PTR sampler;

		gcmDUMP_API("${ES11 glTexCoordPointer 0x%08X 0x%08X 0x%08X 0x%08X}", Size, Type, Stride, Pointer);

        glmPROFILE(context, GLES1_TEXCOORDPOINTER, 0);

        /* Validate size. */
        if ((Size < 2) || (Size > 4)
        /* Validate stride. */
         || (Stride < 0))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Validate type. */
        if (
#if !defined(COMMON_LITE)
            	(Type != GL_FLOAT) &&
#endif
                (Type != GL_SHORT)
            &&  (Type != GL_FIXED)
            &&  (Type != GL_HALF_FLOAT_OES)
            &&  (Type != GL_BYTE)
            )
        {
            /* Invalid enum. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Shortcut to the active sampler. */
        sampler = context->texture.activeClientSampler;

        /* Set stream parameters. */
        glfSetStreamParameters(
            context,
            &sampler->aTexCoordInfo,
            Type,
            Size,
            Stride,
            gcvFALSE,
            Pointer,
            context->arrayBuffer,
            (gleBUFFERBINDINGS) (glvTEX0COORDBUFFER + sampler->index)
            );
    }
    glmLEAVE();
}
