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




#include "gc_glsh_precomp.h"

void
_glshInitializeVertex(
    IN GLContext Context
    )
{
    gctSIZE_T i;

    gcmHEADER_ARG("Context=0x%x", Context);

#if GL_USE_VERTEX_ARRAY
/* Construct the gcoVERTEXARRAY object. */
    if (gcmIS_ERROR(gcoVERTEXARRAY_Construct(Context->hal, &Context->vertex)))
    {
        /* Set error condition. */
        gl2mERROR(GL_OUT_OF_MEMORY);

        /* Return the status. */
        gcmFOOTER_ARG("%s", "GL_OUT_OF_MEMORY");
        return;
    }

    /* Initilaize the vertex array. */
    for (i = 0; i < gcmCOUNTOF(Context->vertexArrayGL); ++i)
    {
        Context->vertexArrayGL[i].type          = GL_FLOAT;
        Context->vertexArrayGL[i].stride        = 0;
        Context->vertexArrayGL[i].buffer        = gcvNULL;
        Context->vertexArray[i].enable          = gcvFALSE;
        Context->vertexArray[i].size            = 4;
        Context->vertexArray[i].format          = gcvVERTEX_FLOAT;
        Context->vertexArray[i].normalized      = gcvFALSE;
        Context->vertexArray[i].stride          = 0;
        Context->vertexArray[i].pointer         = gcvNULL;
        Context->vertexArray[i].stream          = gcvNULL;
        Context->vertexArray[i].linkage         = i;
        Context->vertexArray[i].genericValue[0] = 0.0f;
        Context->vertexArray[i].genericValue[1] = 0.0f;
        Context->vertexArray[i].genericValue[2] = 0.0f;
        Context->vertexArray[i].genericValue[3] = 1.0f;
    }
#else
    /* Zero out vertex array. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(Context->vertexArray,
                                  gcmSIZEOF(Context->vertexArray)));

    /* Initialize the generic vertices. */
    for (i = 0; i < gcmCOUNTOF(Context->genericVertex); ++i)
    {
        Context->genericVertex[i][0]    = 0.0f;
        Context->genericVertex[i][1]    = 0.0f;
        Context->genericVertex[i][2]    = 0.0f;
        Context->genericVertex[i][3]    = 1.0f;
        Context->genericVertexLength[i] = 4;
    }
#endif

    /* No array object bound yet. */
    Context->vertexArrayObject  = gcvNULL;

    /* Zero buffers. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->vertexArrayObjects,
                                  gcmSIZEOF(Context->vertexArrayObjects)));

    /* Success. */
    gcmFOOTER_NO();
}

void
_glshDeinitializeVertex(
    IN GLContext Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);

#if GL_USE_VERTEX_ARRAY
    if (Context->vertex != gcvNULL)
    {
        /* Destroy the gcoVERTEXARRAY object. */
        gcmVERIFY_OK(gcoVERTEXARRAY_Destroy(Context->vertex));
        Context->vertex = gcvNULL;
    }
#endif

    gcmFOOTER_NO();
}

/*******************************************************************************
**  glEnableVertexAttribArray
**
**  Enable a generic vertex attribute array.
**
***** PARAMETERS
**
**  index
**
**      Specifies the index of the generic vertex attribute to be enabled.
**
***** DESCRIPTION
**
**  glEnableVertexAttribArray enables the generic vertex attribute array
**  specified by <index>. By default, all client-side capabilities are disabled,
**  including all generic vertex attribute arrays. If enabled, the values in the
**  generic vertex attribute array will be accessed and used for rendering when
**  calls are made to vertex array commands such as glDrawArrays or
**  glDrawElements.
**
***** ERRORS
**
**  GL_INVALID_VALUE is generated if <index> is greater than or equal to
**  GL_MAX_VERTEX_ATTRIBS.
*/
GL_APICALL void GL_APIENTRY
glEnableVertexAttribArray(
    IN GLuint index
    )
{
#if gcdNULL_DRIVER < 3
#if GL_USE_VERTEX_ARRAY
    gcsVERTEXARRAY *vertexArray;
#else
    GLAttribute *vertexArray;
#endif

	glmENTER1(glmARGUINT, index)
	{
    gcmDUMP_API("${ES20 glEnableVertexAttribArray 0x%08X}", index);

    if(context->vertexArrayObject == gcvNULL)
    {
        vertexArray = context->vertexArray;
    }
    else
    {
        vertexArray = context->vertexArrayObject->vertexArray;
    }

    /* Count the call. */
    glmPROFILE(context, GLES2_ENABLEVERTEXATTRIBARRAY, 0);

    /* Quitely ignore -1 as an <index>. */
    if ((GLint) index == -1)
    {
        break;
    }

    /* Test for valid <index>. */
    if (index >= (GLuint) context->maxAttributes)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "index=%u is out of range [0..%u]",
                      index, context->maxAttributes - 1);

        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Enable the generic vertex attribute. */
    vertexArray[index].enable = gcvTRUE;

    /* Disable batch optmization. */
    context->batchDirty   = GL_TRUE;
    context->batchEnable |= 1 << index;

	}
	glmLEAVE();
#endif
}

/*******************************************************************************
**  glDisableVertexAttribArray
**
**  Disable a generic vertex attribute array.
**
***** PARAMETERS
**
**  index
**
**      Specifies the index of the generic vertex attribute to be disabled.
**
***** DESCRIPTION
**
**  glDisableVertexAttribArray disables the generic vertex attribute array
**  specified by <index>. By default, all client-side capabilities are disabled,
**  including all generic vertex attribute arrays. If enabled, the values in the
**  generic vertex attribute array will be accessed and used for rendering when
**  calls are made to vertex array commands such as glDrawArrays or
**  glDrawElements.
**
***** ERRORS
**
**  GL_INVALID_VALUE is generated if <index> is greater than or equal to
**  GL_MAX_VERTEX_ATTRIBS.
*/
GL_APICALL void GL_APIENTRY
glDisableVertexAttribArray(
    IN GLuint index
    )
{
#if gcdNULL_DRIVER < 3
#if GL_USE_VERTEX_ARRAY
    gcsVERTEXARRAY *vertexArray;
#else
    GLAttribute *vertexArray;
#endif

    glmENTER1(glmARGUINT, index)
	{
    gcmDUMP_API("${ES20 glDisableVertexAttribArray 0x%08X}", index);

    if(context->vertexArrayObject == gcvNULL)
    {
        vertexArray = context->vertexArray;
    }
    else
    {
        vertexArray = context->vertexArrayObject->vertexArray;
    }

    /* Count the call. */
    glmPROFILE(context, GLES2_DISABLEVERTEXATTRIBARRAY, 0);

    /* Quitely ignore -1 as an <index>. */
    if ((GLint) index == -1)
    {
        break;
    }

    /* Test for valid <index>. */
    if (index >= (GLuint) context->maxAttributes)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "index=%u is out of range [0..%u]",
                      index, context->maxAttributes - 1);

        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Disable the generic vertex attribute. */
    vertexArray[index].enable = gcvFALSE;

    /* Disable batch optmization. */
    context->batchDirty   = GL_TRUE;
    context->batchEnable &= ~(1 << index);

	}
	glmLEAVE();
#endif
}

/*******************************************************************************
**  glVertexAttribPointer
**
**  Define an array of generic vertex attribute data.
**
***** PARAMETERS
**
**  index
**
**      Specifies the index of the generic vertex attribute to be modified.
**
**  size
**
**      Specifies the number of components per generic vertex attribute. Must be
**      1, 2, 3, or 4. The initial value is 4.
**
**  type
**
**      Specifies the data type of each component in the array. Symbolic
**      constants GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT,
**      GL_FIXED, or GL_FLOAT are accepted. The initial value is GL_FLOAT.
**
**  normalized
**
**      Specifies whether fixed-point data values should be normalized (GL_TRUE)
**      or converted directly as fixed-point values (GL_FALSE) when they are
**      accessed.
**
**  stride
**
**      Specifies the byte offset between consecutive generic vertex attributes.
**      If stride is 0, the generic vertex attributes are understood to be
**      tightly packed in the array. The initial value is 0.
**
**  pointer
**
**      Specifies a pointer to the first component of the first generic vertex
**      attribute in the array. The initial value is 0.
**
***** DESCRIPTION
**
**  glVertexAttribPointer specifies the location and data format of the array of
**  generic vertex attributes at index <index> to use when rendering. <size>
**  specifies the number of components per attribute and must be 1, 2, 3, or 4.
**  <type> specifies the data type of each component, and <stride> specifies the
**  byte stride from one attribute to the next, allowing vertices and attributes
**  to be packed into a single array or stored in separate arrays. If set to
**  GL_TRUE, <normalized> indicates that values stored in an integer format are
**  to be mapped to the range [-1,1] (for signed values) or [0,1] (for unsigned
**  values) when they are accessed and converted to floating point. Otherwise,
**  values will be converted to floats directly without normalization.
**
**  If a non-zero named buffer object is bound to the GL_ARRAY_BUFFER target
**  (see glBindBuffer) while a generic vertex attribute array is specified,
**  <pointer> is treated as a byte offset into the buffer object's data store.
**  Also, the buffer object binding (GL_ARRAY_BUFFER_BINDING) is saved as
**  generic vertex attribute array client-side state
**  (GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING) for index <index>.
**
**  When a generic vertex attribute array is specified, <size>, <type>,
**  <normalized>, <stride>, and <pointer> are saved as client-side state, in
**  addition to the current vertex array buffer object binding.
**
**  To enable and disable a generic vertex attribute array, call
**  glEnableVertexAttribArray and glDisableVertexAttribArray with <index>. If
**  enabled, the generic vertex attribute array is used when glDrawArrays or
**  glDrawElements is called.
**
***** NOTES
**
**  Each generic vertex attribute array is initially disabled and isn't accessed
**  when glDrawElements or glDrawArrays is called.
**
**  glVertexAttribPointer is typically implemented on the client side.
**
***** ERRORS
**
**  GL_INVALID_ENUM is generated if <type> is not an accepted value.
**
**  GL_INVALID_VALUE is generated if <index> is greater than or equal to
**  GL_MAX_VERTEX_ATTRIBS.
**
**  GL_INVALID_VALUE is generated if <size> is not 1, 2, 3, or 4.
**
**  GL_INVALID_VALUE is generated if <stride> is negative.
*/
GL_APICALL void GL_APIENTRY
glVertexAttribPointer(
    GLuint index,
    GLint size,
    GLenum type,
    GLboolean normalized,
    GLsizei stride,
    const GLvoid * pointer
    )
{
#if gcdNULL_DRIVER < 3
    GLsizei autoStride = 0;
    gceVERTEX_FORMAT format = gcvVERTEX_BYTE;
#if GL_USE_VERTEX_ARRAY
    gcoSTREAM stream;
    gcsVERTEXARRAY *vertexArray;
    GLVertex *vertexArrayGL;
#else
    GLAttribute *vertexArray;
#endif
	GLboolean goToEnd = GL_FALSE;

	glmENTER6(glmARGUINT, index, glmARGINT, size, glmARGENUM, type,
		      glmARGINT, normalized, glmARGINT, stride, glmARGPTR, pointer)
	{
    gcmDUMP_API("${ES20 glVertexAttribPointer 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X}",
                index, size, type, normalized, stride, pointer);

    if(context->vertexArrayObject == gcvNULL)
    {
        vertexArray = context->vertexArray;
#if GL_USE_VERTEX_ARRAY
        vertexArrayGL = context->vertexArrayGL;
#endif
    }
    else
    {
        if(context->arrayBuffer == gcvNULL && pointer != gcvNULL)
        {
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }

        vertexArray = context->vertexArrayObject->vertexArray;
#if GL_USE_VERTEX_ARRAY
        vertexArrayGL = context->vertexArrayObject->vertexArrayGL;
#endif
    }

    /* Count the call. */
    glmPROFILE(context, GLES2_VERTEXATTRIBPOINTER, 0);

    /* Quitely ignore -1 as an <index>. */
    if ((GLint) index == -1)
    {
        break;
    }

    /* Test for valid <size>. */
    if ((size < 1) || (size > 4))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "size=%d is out of range [1..4]",
                      size);

        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Test for valid <index>. */
    if (index >= (GLuint) context->maxAttributes)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "index=%u is out of range [0..%u]",
                      index, context->maxAttributes - 1);

        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Test for valid <stride>. */
    if (stride < 0)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "Negative stride=%d is invalid",
                      stride);

        gl2mERROR(GL_INVALID_VALUE);
		break;
    }

    /* Compute stride for specified <type> and <size>. */
    switch (type)
    {
    case GL_BYTE:
        autoStride = size * gcmSIZEOF(GLbyte);
        format     = gcvVERTEX_BYTE;
        break;

    case GL_UNSIGNED_BYTE:
        autoStride = size * gcmSIZEOF(GLubyte);
        format     = gcvVERTEX_UNSIGNED_BYTE;
        break;

    case GL_SHORT:
        autoStride = size * gcmSIZEOF(GLshort);
        format     = gcvVERTEX_SHORT;
        break;

    case GL_UNSIGNED_SHORT:
        autoStride = size * gcmSIZEOF(GLushort);
        format     = gcvVERTEX_UNSIGNED_SHORT;
        break;

    case GL_FIXED:
        autoStride = size * gcmSIZEOF(GLfixed);
        format     = gcvVERTEX_FIXED;
        break;

    case GL_FLOAT:
        autoStride = size * gcmSIZEOF(GLfloat);
        format     = gcvVERTEX_FLOAT;
        break;

    case GL_HALF_FLOAT_OES:
        autoStride = size * (gcmSIZEOF(GLfloat) / 2);
        format     = gcvVERTEX_HALF;
        break;

    case GL_UNSIGNED_INT_10_10_10_2_OES:
    case GL_INT_10_10_10_2_OES:
        if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_VERTEX_10_10_10_2) != gcvSTATUS_TRUE)
        {
            /* Invalid type. */
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                          "type=0x%04x is invalid",
                          type);

            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = GL_TRUE;
			break;
        }

        /* Regardless of size being 3 or 4, the container size is 4. */
        autoStride = 4 * gcmSIZEOF(GLubyte);
        format     = (type == GL_UNSIGNED_INT_10_10_10_2_OES) ?
                        gcvVERTEX_UNSIGNED_INT_10_10_10_2 :
                        gcvVERTEX_INT_10_10_10_2;

        /* Test for <size> being 3 or 4. */
        if ((size < 3) || (size > 4))
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                          "size=%d is out of range [3..4]",
                          size);

            gl2mERROR(GL_INVALID_VALUE);
            goToEnd = GL_TRUE;
			break;
        }
        break;

    default:
        /* Invalid type. */
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "type=0x%04x is invalid",
                      type);

        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    if (stride > 0)
    {
        /* Override automatic stride. */
        autoStride = stride;
    }

#if GL_USE_VERTEX_ARRAY
    /* Get current gcoSTREAM object. */
    if (context->arrayBuffer != gcvNULL)
    {
        stream = context->arrayBuffer->stream;
    }
    else
    {
        stream = gcvNULL;
    }

    /* Fill in vertex array. */
    vertexArrayGL[index].type     = type;
    vertexArrayGL[index].buffer   = context->arrayBuffer;
    vertexArrayGL[index].stride   = stride;
    vertexArray[index].size       = size;
    vertexArray[index].format     = format;
    vertexArray[index].normalized = normalized;
    vertexArray[index].stride     = autoStride;
    vertexArray[index].pointer    = pointer;
    vertexArray[index].stream     = stream;

#else
    vertexArray[index].size       = size;
    vertexArray[index].type       = type;
    vertexArray[index].normalized = normalized;
    vertexArray[index].stride     = autoStride;
    vertexArray[index].buffer     = context->arrayBuffer;

    if (context->arrayBuffer == gcvNULL)
    {
        vertexArray[index].ptr    = pointer;
        vertexArray[index].offset = 0;
    }
    else
    {
        gcmVERIFY_OK(gcoSTREAM_SetStride(context->arrayBuffer->stream, autoStride));
        gcmASSERT(autoStride > 0);
        vertexArray[index].ptr    = gcvNULL;
        vertexArray[index].offset = (GLsizeiptr) pointer;
    }
#endif

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;
#if GL_USE_VERTEX_ARRAY
    if (stream != gcvNULL)
    {
        context->batchArray &= ~(1 << index);
    }
    else
    {
        context->batchArray |= 1 << index;
    }
#endif

	}
	glmLEAVE();
#endif
}

/*******************************************************************************
**  glVertexAttrib
**
**  Specify the value of a generic vertex attribute.
**
***** PARAMETERS
**
**  index
**
**      Specifies the index of the generic vertex attribute to be modified.
**
**  v0, v1, v2, v3
**
**      Specifies the new values to be used for the specified vertex attribute.
**
**  v
**
**      Specifies a pointer to an array of values to be used for the generic
**      vertex attribute.
**
***** DESCRIPTION
**
**  The glVertexAttrib family of entry points allows an application to pass
**  generic vertex attributes in numbered locations.
**
**  Generic attributes are defined as four-component values that are organized
**  into an array. The first entry of this array is numbered 0, and the size of
**  the array is specified by the implementation-dependent symbolic constant
**  GL_MAX_VERTEX_ATTRIBS. Individual elements of this array can be modified
**  with a glVertexAttrib call that specifies the index of the element to be
**  modified and a value for that element.
**
**  These commands can be used to specify one, two, three, or all four
**  components of the generic vertex attribute specified by <index>. A 1 in the
**  name of the command indicates that only one value is passed, and it will be
**  used to modify the first component of the generic vertex attribute. The
**  second and third components will be set to 0, and the fourth component will
**  be set to 1. Similarly, a 2 in the name of the command indicates that values
**  are provided for the first two components, the third component will be set
**  to 0, and the fourth component will be set to 1. A 3 in the name of the
**  command indicates that values are provided for the first three components
**  and the fourth component will be set to 1, whereas a 4 in the name indicates
**  that values are provided for all four components.
**
**  The letter f indicates that the arguments are of type float. When v is
**  appended to the name, the commands can take a pointer to an array of floats.
**
**  OpenGL ES Shading Language attribute variables are allowed to be of type
**  mat2, mat3, or mat4. Attributes of these types may be loaded using the
**  glVertexAttrib entry points. Matrices must be loaded into successive generic
**  attribute slots in column major order, with one column of the matrix in each
**  generic attribute slot.
**
**  A user-defined attribute variable declared in a vertex shader can be bound
**  to a generic attribute index by calling glBindAttribLocation. This allows an
**  application to use descriptive variable names in a vertex shader. A
**  subsequent change to the specified generic vertex attribute will be
**  immediately reflected as a change to the corresponding attribute variable in
**  the vertex shader.
**
**  The binding between a generic vertex attribute index and a user-defined
**  attribute variable in a vertex shader is part of the state of a program
**  object, but the current value of the generic vertex attribute is not. The
**  value of each generic vertex attribute is part of current state and it is
**  maintained even if a different program object is used.
**
**  An application may freely modify generic vertex attributes that are not
**  bound to a named vertex shader attribute variable. These values are simply
**  maintained as part of current state and will not be accessed by the vertex
**  shader. If a generic vertex attribute bound to an attribute variable in a
**  vertex shader is not updated while the vertex shader is executing, the
**  vertex shader will repeatedly use the current value for the generic vertex
**  attribute.
**
***** NOTES
**
**  It is possible for an application to bind more than one attribute name to
**  the same generic vertex attribute index. This is referred to as aliasing,
**  and it is allowed only if just one of the aliased attribute variables is
**  active in the vertex shader, or if no path through the vertex shader
**  consumes more than one of the attributes aliased to the same location.
**  OpenGL implementations are not required to do error checking to detect
**  aliasing, they are allowed to assume that aliasing will not occur, and they
**  are allowed to employ optimizations that work only in the absence of
**  aliasing.
**
***** ERRORS
**
**  GL_INVALID_VALUE is generated if <index> is greater than or equal to
**  GL_MAX_VERTEX_ATTRIBS.
*/
#if gcdNULL_DRIVER < 3
static gctBOOL
_glshVertrxAttrib(
    IN GLuint Index,
    IN GLfloat V0,
    IN GLfloat V1,
    IN GLfloat V2,
    IN GLfloat V3,
    IN GLint FunctionIndex,
    OUT GLContext * Context
    )
{
    GLContext context;

    /* Get current context. */
    gcmASSERT(Context != gcvNULL);
    *Context = (context = _glshGetCurrentContext());
    if (context == gcvNULL)
    {
        return gcvTRUE;
    }

    /* Count the call. */
    glmPROFILE(context, FunctionIndex, 0);

    /* Test for valid <index>. */
    if (Index >= (GLuint) context->maxAttributes)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "index=%u is out of range [0..%u]",
                      Index, context->maxAttributes - 1);

        return gcvFALSE;
    }

    /* Set generic vertex attribute values. */
#if GL_USE_VERTEX_ARRAY
    context->vertexArray[Index].genericValue[0] = V0;
    context->vertexArray[Index].genericValue[1] = V1;
    context->vertexArray[Index].genericValue[2] = V2;
    context->vertexArray[Index].genericValue[3] = V3;

    /* Tmp solution for VAO generic value */
    if(context->vertexArrayObject != gcvNULL)
    {
        context->vertexArrayObject->vertexArray[Index].genericValue[0] =
            context->vertexArray[Index].genericValue[0];
        context->vertexArrayObject->vertexArray[Index].genericValue[1] =
            context->vertexArray[Index].genericValue[1];
        context->vertexArrayObject->vertexArray[Index].genericValue[2] =
            context->vertexArray[Index].genericValue[2];
        context->vertexArrayObject->vertexArray[Index].genericValue[3] =
            context->vertexArray[Index].genericValue[3];
    }
#else
    context->genericVertex[Index][0]    = V0;
    context->genericVertex[Index][1]    = V1;
    context->genericVertex[Index][2]    = V2;
    context->genericVertex[Index][3]    = V3;
    context->genericVertexLength[Index] = 4;
#endif

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    return gcvTRUE;
}
#endif

GL_APICALL void GL_APIENTRY
glVertexAttrib1f(
    GLuint index,
    GLfloat v0
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context = gcvNULL;

    gcmHEADER_ARG("index=%u v0=%f", index, v0);
    gcmDUMP_API("${ES20 glVertexAttrib1f 0x%08X 0x%08X}",
                index, *(GLuint *) &v0);

    /* Route through common function. */
    if (!_glshVertrxAttrib(index,
                           v0, 0.0f, 0.0f, 1.0f,
                           GLES2_VERTEXATTRIB1F,
                           &context))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glVertexAttrib2f(
    GLuint index,
    GLfloat v0,
    GLfloat v1
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context = gcvNULL;

    gcmHEADER_ARG("index=%u v0=%f v1=%f", index, v0, v1);
    gcmDUMP_API("${ES20 glVertexAttrib2f 0x%08X 0x%08X 0x%08X}",
                index, *(GLuint *) &v0, *(GLuint *) &v1);

    /* Route through common function. */
    if (!_glshVertrxAttrib(index,
                           v0, v1, 0.0f, 1.0f,
                           GLES2_VERTEXATTRIB2F,
                           &context))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glVertexAttrib3f(
    GLuint index,
    GLfloat v0,
    GLfloat v1,
    GLfloat v2
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context = gcvNULL;

    gcmHEADER_ARG("index=%u v0=%f v1=%f v2=%f", index, v0, v1, v2);
    gcmDUMP_API("${ES20 glVertexAttrib3f 0x%08X 0x%08X 0x%08X 0x%08X}",
                index, *(GLuint *) &v0, *(GLuint *) &v1, *(GLuint *) &v2);

    /* Route through common function. */
    if (!_glshVertrxAttrib(index,
                           v0, v1, v2, 1.0f,
                           GLES2_VERTEXATTRIB3F,
                           &context))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glVertexAttrib4f(
    GLuint index,
    GLfloat v0,
    GLfloat v1,
    GLfloat v2,
    GLfloat v3
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context = gcvNULL;

    gcmHEADER_ARG("index=%u v0=%f v1=%f v2=%f v3=%f", index, v0, v1, v2, v3);
    gcmDUMP_API("${ES20 glVertexAttrib4f 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
                index, *(GLuint *) &v0, *(GLuint *) &v1, *(GLuint *) &v2,
                *(GLuint *) &v3);

    /* Route through common function. */
    if (!_glshVertrxAttrib(index,
                           v0, v1, v2, v3,
                           GLES2_VERTEXATTRIB4F,
                           &context))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glVertexAttrib1fv(
    GLuint index,
    const GLfloat * v
    )
{
#if gcdNULL_DRIVER < 3
    static const GLfloat vDefault[1] = { 0.0f };
    const GLfloat * vPtr = (v == gcvNULL) ? vDefault : v;
    GLContext context = gcvNULL;

    gcmHEADER_ARG("index=%u v=0x%x", index, v);
    gcmDUMP_API("${ES20 glVertexAttrib1fv 0x%08X (0x%08X)", index, v);
    gcmDUMP_API_ARRAY(v, 1);
    gcmDUMP_API("$}");

    /* Route through common function. */
    if (!_glshVertrxAttrib(index,
                           vPtr[0], 0.0f, 0.0f, 1.0f,
                           GLES2_VERTEXATTRIB1FV,
                           &context))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glVertexAttrib2fv(
    GLuint index,
    const GLfloat * v
    )
{
#if gcdNULL_DRIVER < 3
    static const GLfloat vDefault[2] = { 0.0f, 0.0f };
    const GLfloat * vPtr = (v == gcvNULL) ? vDefault : v;
    GLContext context = gcvNULL;

    gcmHEADER_ARG("index=%u v=0x%x", index, v);
    gcmDUMP_API("${ES20 glVertexAttrib2fv 0x%08X (0x%08X)", index, v);
    gcmDUMP_API_ARRAY(v, 2);
    gcmDUMP_API("$}");

    /* Route through common function. */
    if (!_glshVertrxAttrib(index,
                           vPtr[0], vPtr[1], 0.0f, 1.0f,
                           GLES2_VERTEXATTRIB2FV,
                           &context))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glVertexAttrib3fv(
    GLuint index,
    const GLfloat * v
    )
{
#if gcdNULL_DRIVER < 3
    static const GLfloat vDefault[3] = { 0.0f, 0.0f, 0.0f };
    const GLfloat * vPtr = (v == gcvNULL) ? vDefault : v;
    GLContext context = gcvNULL;

    gcmHEADER_ARG("index=%u v=0x%x", index, v);
    gcmDUMP_API("${ES20 glVertexAttrib3fv 0x%08X (0x%08X)", index, v);
    gcmDUMP_API_ARRAY(v, 3);
    gcmDUMP_API("$}");

    /* Route through common function. */
    if (!_glshVertrxAttrib(index,
                           vPtr[0], vPtr[1], vPtr[2], 1.0f,
                           GLES2_VERTEXATTRIB3FV,
                           &context))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glVertexAttrib4fv(
    GLuint index,
    const GLfloat * v
    )
{
#if gcdNULL_DRIVER < 3
    static const GLfloat vDefault[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    const GLfloat * vPtr = (v == gcvNULL) ? vDefault : v;
    GLContext context = gcvNULL;

    gcmHEADER_ARG("index=%u v=0x%x", index, v);
    gcmDUMP_API("${ES20 glVertexAttrib4fv 0x%08X (0x%08X)", index, v);
    gcmDUMP_API_ARRAY(v, 4);
    gcmDUMP_API("$}");

    /* Route through common function. */
    if (!_glshVertrxAttrib(index,
                           vPtr[0], vPtr[1], vPtr[2], vPtr[3],
                           GLES2_VERTEXATTRIB4FV,
                           &context))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    gcmFOOTER_NO();
#endif
}

/*******************************************************************************
**  glGetVertexAttribiv
**  glGetVertexAttribfv
**
**  Return a generic vertex attribute parameter.
**
***** PARAMETERS
**
**  index
**
**      Specifies the generic vertex attribute parameter to be queried.
**
**  pname
**
**      Specifies the symbolic name of the vertex attribute parameter to be
**      queried. Accepted values are GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,
**      GL_VERTEX_ATTRIB_ARRAY_ENABLED, GL_VERTEX_ATTRIB_ARRAY_SIZE,
**      GL_VERTEX_ATTRIB_ARRAY_STRIDE, GL_VERTEX_ATTRIB_ARRAY_TYPE,
**      GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, or GL_CURRENT_VERTEX_ATTRIB.
**
**  params
**
**      Returns the requested data.
**
***** DESCRIPTION
**
**  glGetVertexAttrib returns in <params> the value of a generic vertex
**  attribute parameter. The generic vertex attribute to be queried is specified
**  by <index>, and the parameter to be queried is specified by <pname>.
**
**  The accepted parameter names are as follows:
**
**  GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING
**
**      <params> returns a single value, the name of the buffer object currently
**      bound to the binding point corresponding to generic vertex attribute
**      array index. If no buffer object is bound, 0 is returned. The initial
**      value is 0.
**
**  GL_VERTEX_ATTRIB_ARRAY_ENABLED
**
**      <params> returns a single value that is non-zero (true) if the vertex
**      attribute array for <index> is enabled and 0 (false) if it is disabled.
**      The initial value is GL_FALSE.
**
**  GL_VERTEX_ATTRIB_ARRAY_SIZE
**
**      <params> returns a single value, the size of the vertex attribute array
**      for <index>. The size is the number of values for each element of the
**      vertex attribute array, and it will be 1, 2, 3, or 4. The initial value
**      is 4.
**
**  GL_VERTEX_ATTRIB_ARRAY_STRIDE
**
**      <params> returns a single value, the array stride for (number of bytes
**      between successive elements in) the vertex attribute array for <index>.
**      A value of 0 indicates that the array elements are stored sequentially
**      in memory. The initial value is 0.
**
**  GL_VERTEX_ATTRIB_ARRAY_TYPE
**
**      <params> returns a single value, a symbolic constant indicating the
**      array type for the vertex attribute array for <index>. Possible values
**      are GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_FIXED,
**      and GL_FLOAT. The initial value is GL_FLOAT.
**
**  GL_VERTEX_ATTRIB_ARRAY_NORMALIZED
**
**      <params> returns a single value that is non-zero (true) if fixed-point
**      data types for the vertex attribute array indicated by <index> are
**      normalized when they are converted to floating point, and 0 (false)
**      otherwise. The initial value is GL_FALSE.
**
**  GL_CURRENT_VERTEX_ATTRIB
**
**      <params> returns four values that represent the current value for the
**      generic vertex attribute specified by <index>. The initial value is
**      (0,0,0,1).
**
**  All of the parameters except GL_CURRENT_VERTEX_ATTRIB represent client-side
**  state.
**
***** NOTES
**
**  If an error is generated, no change is made to the contents of <params>.
**
***** ERRORS
**
**  GL_INVALID_ENUM is generated if <pname> is not an accepted value.
**
**  GL_INVALID_VALUE is generated if <index> is greater than or equal to
**  GL_MAX_VERTEX_ATTRIBS.
*/
GL_APICALL void GL_APIENTRY
glGetVertexAttribiv(
    GLuint index,
    GLenum pname,
    GLint * params
    )
{
#if gcdNULL_DRIVER < 3
    GLint data[4];
    GLint n = 1;
#if GL_USE_VERTEX_ARRAY
    gcsVERTEXARRAY *vertexArray;
    GLVertex *vertexArrayGL;
#else
    GLAttribute *vertexArray;
#endif
	GLboolean goToEnd = GL_FALSE;

	glmENTER2(glmARGUINT, index, glmARGENUM, pname)
	{

    if(context->vertexArrayObject == gcvNULL)
    {
        vertexArray = context->vertexArray;
#if GL_USE_VERTEX_ARRAY
        vertexArrayGL = context->vertexArrayGL;
#endif
    }
    else
    {
        vertexArray = context->vertexArrayObject->vertexArray;
#if GL_USE_VERTEX_ARRAY
        vertexArrayGL = context->vertexArrayObject->vertexArrayGL;
#endif
    }

    /* Count the call. */
    glmPROFILE(context, GLES2_GETVERTEXATTRIBIV, 0);

    /* Test for valid <index>. */
    if (index >= (GLuint) context->maxAttributes)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "index=%u is out of range [0..%u]",
                      index, context->maxAttributes - 1);

        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Get the vertex attribute parameter. */
    switch (pname)
    {
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
#if GL_USE_VERTEX_ARRAY
        data[0] = (vertexArrayGL[index].buffer != gcvNULL)
                ? vertexArrayGL[index].buffer->object.name
                : 0;
#else
        data[0] = (vertexArray[index].buffer != gcvNULL)
                ? vertexArray[index].buffer->object.name
                : 0;
#endif
        break;

    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        data[0] = vertexArray[index].enable;
        break;

    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
        data[0] = vertexArray[index].size;
        break;

    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
#if GL_USE_VERTEX_ARRAY
        data[0] = vertexArrayGL[index].stride;
#else
        data[0] = vertexArray[index].stride;
#endif
        break;

    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
#if GL_USE_VERTEX_ARRAY
        data[0] = vertexArrayGL[index].type;
#else
        data[0] = vertexArray[index].type;
#endif
        break;

    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
        data[0] = vertexArray[index].normalized;
        break;

    case GL_CURRENT_VERTEX_ATTRIB:
#if GL_USE_VERTEX_ARRAY
        data[0] = (GLint) context->vertexArray[index].genericValue[0];
        data[1] = (GLint) context->vertexArray[index].genericValue[1];
        data[2] = (GLint) context->vertexArray[index].genericValue[2];
        data[3] = (GLint) context->vertexArray[index].genericValue[3];
#else
        data[0] = (GLint) context->genericVertex[index][0];
        data[1] = (GLint) context->genericVertex[index][1];
        data[2] = (GLint) context->genericVertex[index][2];
        data[3] = (GLint) context->genericVertex[index][3];
#endif
        n       = 4;
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "pname=0x%04x is invalid",
                 pname);

        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = GL_TRUE;
        break;
    }
	if (goToEnd)
		break;

    if (params != gcvNULL)
    {
        /* Copy the vertex attribute parameter to <params>. */
        gcmVERIFY_OK(gcoOS_MemCopy(params, data, n * gcmSIZEOF(GLint)));
    }

    gcmDUMP_API("${ES20 glGetVertexAttribiv 0x%08X 0x%08X (0x%08X)",
                index, pname, params);
    gcmDUMP_API_ARRAY(params, n);
    gcmDUMP_API("$}");
	}
	glmLEAVE1(glmARGINT, *params);

#else
    *params = 0;
#endif
}

GL_APICALL void GL_APIENTRY
glGetVertexAttribfv(
    GLuint index,
    GLenum pname,
    GLfloat * params
    )
{
#if gcdNULL_DRIVER < 3
    GLfloat data[4];
    GLint n = 1;
#if GL_USE_VERTEX_ARRAY
    gcsVERTEXARRAY *vertexArray;
    GLVertex *vertexArrayGL;
#else
    GLAttribute *vertexArray;
#endif
	GLboolean goToEnd = GL_FALSE;

	glmENTER2(glmARGUINT, index, glmARGENUM, pname)
	{
    if(context->vertexArrayObject == gcvNULL)
    {
        vertexArray = context->vertexArray;
#if GL_USE_VERTEX_ARRAY
        vertexArrayGL = context->vertexArrayGL;
#endif
    }
    else
    {
        vertexArray = context->vertexArrayObject->vertexArray;
#if GL_USE_VERTEX_ARRAY
        vertexArrayGL = context->vertexArrayObject->vertexArrayGL;
#endif
    }

    /* Count the call. */
    glmPROFILE(context, GLES2_GETVERTEXATTRIBFV, 0);

    /* Test for valid <index>. */
    if (index >= (GLuint) context->maxAttributes)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "index=%u is out of range [0..%u]",
                      index, context->maxAttributes - 1);

        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Get the vertex attribute parameter. */
    switch (pname)
    {
    case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
#if GL_USE_VERTEX_ARRAY
        data[0] = (vertexArrayGL[index].buffer != gcvNULL)
                ? (GLfloat) vertexArrayGL[index].buffer->object.name
                : 0.0f;
#else
        data[0] = (vertexArray[index].buffer != gcvNULL)
                ? (GLfloat)vertexArray[index].buffer->object.name
                : 0.0f;
#endif
        break;

    case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
        data[0] = (GLfloat) vertexArray[index].enable;
        break;

    case GL_VERTEX_ATTRIB_ARRAY_SIZE:
        data[0] = (GLfloat) vertexArray[index].size;
        break;

    case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
#if GL_USE_VERTEX_ARRAY
        data[0] = (GLfloat) vertexArrayGL[index].stride;
#else
        data[0] = (GLfloat) vertexArray[index].stride;
#endif
        break;

    case GL_VERTEX_ATTRIB_ARRAY_TYPE:
#if GL_USE_VERTEX_ARRAY
        data[0] = (GLfloat) vertexArrayGL[index].type;
#else
        data[0] = (GLfloat) vertexArray[index].type;
#endif
        break;

    case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
        data[0] = (GLfloat) vertexArray[index].normalized;
        break;

    case GL_CURRENT_VERTEX_ATTRIB:
#if GL_USE_VERTEX_ARRAY
        data[0] = context->vertexArray[index].genericValue[0];
        data[1] = context->vertexArray[index].genericValue[1];
        data[2] = context->vertexArray[index].genericValue[2];
        data[3] = context->vertexArray[index].genericValue[3];
#else
        data[0] = context->genericVertex[index][0];
        data[1] = context->genericVertex[index][1];
        data[2] = context->genericVertex[index][2];
        data[3] = context->genericVertex[index][3];
#endif
        n       = 4;
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "pname=0x%04x is invalid",
                 pname);

        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    if (params != gcvNULL)
    {
        /* Copy the vertex attribute parameter to <params>. */
        gcmVERIFY_OK(gcoOS_MemCopy(params, data, n * gcmSIZEOF(GLfloat)));
    }

    gcmDUMP_API("${ES20 glGetVertexAttribfv 0x%08X 0x%08X (0x%08X)",
                index, pname, params);
    gcmDUMP_API_ARRAY(params, n);
    gcmDUMP_API("$}");
	}
	glmLEAVE1(glmARGFLOAT, *params);
#else
    *params = 0;
#endif
}

/*******************************************************************************
**  glGetVertexAttribPointerv
**
**  Return the address of the specified generic vertex attribute pointer.
**
***** PARAMETERS
**
**  index
**
**      Specifies the generic vertex attribute parameter to be returned.
**
**  pname
**
**      Specifies the symbolic name of the generic vertex attribute parameter to
**      be returned. Must be GL_VERTEX_ATTRIB_ARRAY_POINTER.
**
**  pointer
**
**      Returns the pointer value.
**
*****  DESCRIPTION
**
**  glGetVertexAttribPointerv returns pointer information. <index> is the
**  generic vertex attribute to be queried, <pname> is a symbolic constant
**  indicating the pointer to be returned, and <params> is a pointer to a
**  location in which to place the returned data.
**
**  If a non-zero named buffer object was bound to the GL_ARRAY_BUFFER target
**  (see glBindBuffer) when the desired pointer was previously specified, the
**  pointer returned is a byte offset into the buffer object's data store.
**
***** NOTES
**
**  The pointer returned is client-side state.
**
**  The initial value for each pointer is 0.
**
***** ERRORS
**
**  GL_INVALID_ENUM is generated if <pname> is not an accepted value.
**
**  GL_INVALID_VALUE is generated if <index> is greater than or equal to
**  GL_MAX_VERTEX_ATTRIBS.
*/
GL_APICALL void GL_APIENTRY
glGetVertexAttribPointerv(
    IN GLuint index,
    IN GLenum pname,
    OUT GLvoid ** pointer
    )
{
#if gcdNULL_DRIVER < 3

#if GL_USE_VERTEX_ARRAY
    gcsVERTEXARRAY *vertexArray;
#else
    GLAttribute *vertexArray;
#endif

	glmENTER2(glmARGUINT, index, glmARGENUM, pname)
	{

    if(context->vertexArrayObject == gcvNULL)
    {
        vertexArray = context->vertexArray;
    }
    else
    {
        vertexArray = context->vertexArrayObject->vertexArray;
    }

    /* Count the call. */
    glmPROFILE(context, GLES2_GETVERTEXATTRIBPOINTERV, 0);

    /* Test for valid <index>. */
    if (index >= (GLuint) context->maxAttributes)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "index=%u is out of range [0..%u]",
                      index, context->maxAttributes - 1);

        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Test for valid <pname>. */
    if (pname != GL_VERTEX_ATTRIB_ARRAY_POINTER)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_VERTEX,
                      "PName=0x%x is invalid",
                      pname);

        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    if (pointer != gcvNULL)
    {
#if GL_USE_VERTEX_ARRAY
        *pointer = (void *) vertexArray[index].pointer;
#else
        *pointer = (void *) vertexArray[index].ptr;
#endif
    }

    gcmDUMP_API("${ES20 glGetVertexAttribPointerv 0x%08X 0x%08X (0x%08X)",
                index, pname, pointer);
    gcmDUMP_API_ARRAY(pointer, 1);
    gcmDUMP_API("$}");
	}
	glmLEAVE1(glmARGPTR, *pointer);

#else
    *pointer = gcvNULL;
#endif
}

GL_APICALL void GL_APIENTRY
glBindVertexArrayOES(
    GLuint array
    )
{
#if gcdNULL_DRIVER < 3
    GLVertexArray object;

	glmENTER1(glmARGINT,array)
	{
    gcmDUMP_API("${ES20 glBindVertexArrayOES 0x%08X}",array);

    glmPROFILE(context, GLES2_BINDVERTEXARRAYOES, 0);

    if (array == 0)
    {
        /* Remove current binding. */
        object = gcvNULL;
        /* Unbind buffer from currently bound target. */
        if(context->vertexArrayObject != gcvNULL)
        {
            context->vertexArrayObject->target = GL_NONE;
            context->vertexArrayObject = gcvNULL;
        }
    }
    else
    {
        /* Find the object. */
        object = (GLVertexArray) _glshFindObject(&context->vertexArrayObjects, array);

        if (object == gcvNULL)
        {
            /* Only the created VAO could been bind */
            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Unbind buffer from currently bound target. */
        if(context->vertexArrayObject != gcvNULL &&
            context->vertexArrayObject->object.name != object->object.name)
        {
            context->vertexArrayObject->target = GL_NONE;
        }

        object->target = GL_VERTEX_ARRAY_BINDING_OES;
        context->vertexArrayObject = object;

        /* Tmp solution for generic color, that we copy from context */
#if GL_USE_VERTEX_ARRAY
        {
            GLint i,j;
            for(i = 0; i < gldVERTEX_ELEMENT_COUNT ;i++)
            {
                for(j = 0; j < 4; j++)
                {
                    context->vertexArrayObject->vertexArray[i].genericValue[j] =
                        context->vertexArray[i].genericValue[j];
                }
            }
        }
#endif
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

#if gcdNULL_DRIVER < 3
static GLVertexArray
_NewVertexArray(
    GLContext Context,
    GLuint Name
    )
{
    GLVertexArray array = gcvNULL;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;
#if GL_USE_VERTEX_ARRAY
    GLint i;
#endif

    for (;;)
    {
        /* Allocate memory for the array object. */
        gcmERR_BREAK(gcoOS_Allocate(Context->os,
                                    gcmSIZEOF(struct _GLVertexArray),
                                    &pointer));

        array = pointer;

        gcmERR_BREAK(gcoOS_ZeroMemory(array,gcmSIZEOF(struct _GLVertexArray)));

        /* Create a new array object. */
        if (!_glshInsertObject(&Context->vertexArrayObjects,
                               &array->object,
                               GLObject_VertexArray,
                               Name))
        {
            break;
        }

        /* The array object is not yet bound. */
        array->target  = GL_NONE;

#if GL_USE_VERTEX_ARRAY
        for(i = 0; i < 16; i++)
        {
            array->vertexArray[i].size = 4;
            array->vertexArray[i].genericValue[0] = 0.0f;
            array->vertexArray[i].genericValue[1] = 0.0f;
            array->vertexArray[i].genericValue[2] = 0.0f;
            array->vertexArray[i].genericValue[3] = 1.0f;
        }
#endif
        /* Return the buffer. */
        return array;
    }

    /* Roll back. */
    if (array != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(Context->os, array));
    }

    /* Out of memory error. */
    gcmFATAL("%s(%d): Out of memory", __FUNCTION__, __LINE__);
    gl2mERROR(GL_OUT_OF_MEMORY);
    return gcvNULL;
}
#endif

void
_glshDereferenceVertexArrayObject(
    GLContext Context,
    GLVertexArray array
)
{
    gcmASSERT((array->object.reference - 1) >= 0);

    if (--array->object.reference == 0)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, array));
    }
}

void
_glshDeleteVertexArrayObject(
    GLContext Context,
    GLVertexArray array
)
{
    /****Comment the code which with empty if statement***/
    /*GLsizei i;*/

    _glshRemoveObject(&Context->vertexArrayObjects, &array->object);

    /*if(array->elementArrayBuffer != gcvNULL)*/
    /*{*/
        /*_glshDereferenceBufferObject(Context, object->elementArrayBuffer);*/
    /*}*/

    /*for(i = 0; i < gldVERTEX_ELEMENT_COUNT ; i++)*/
/*    {*/
/*#if GL_USE_VERTEX_ARRAY*/
        /*if(array->vertexArrayGL[i].buffer)*/
        /*{*/
            /*_glshDereferenceBufferObject(Context, array->vertexArrayGL[i].buffer);*/
        /*}*/
/*#else*/
        /*if(array->vertexArray[i].buffer)*/
        /*{*/
            /*_glshDereferenceBufferObject(Context, array->vertexArray[i].buffer);*/
        /*}*/
/*#endif*/
    /*}*/

    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, array));
}

GL_APICALL void GL_APIENTRY
glDeleteVertexArraysOES (
    GLsizei n,
    const GLuint *arrays
)
{
#if gcdNULL_DRIVER < 3
    GLVertexArray object;
    GLsizei i;

	glmENTER2(glmARGINT, n, glmARGPTR, arrays)
	{
    gcmDUMP_API("${ES20 glDeleteVertexArraysOES 0x%08X (0x%08X)}", n, arrays);
    gcmDUMP_API_ARRAY(arrays, n);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_DELETEVERTEXARRAYOES, 0);

    if (n < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    for (i = 0; i < n ; ++i)
    {
        if (arrays[i] == 0)
        {
            continue;
        }

        object = (GLVertexArray) _glshFindObject(&context->vertexArrayObjects,
                                                 arrays[i]);

        /* Make sure this is a valid vertexarray. */
        if ((object == gcvNULL)
        ||  (object->object.type != GLObject_VertexArray)
        )
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "%s(%d): Object %d is not a vertexarray object",
                          __FUNCTION__, __LINE__, arrays[i]);

            gl2mERROR(GL_INVALID_VALUE);
            break;
        }

        /* If this is the current vertex array object, unbind it. */
        if (context->vertexArrayObject == object)
        {
            glBindVertexArrayOES(0);

            /* Disable batch optmization. */
            context->batchDirty = GL_TRUE;
        }

        _glshDeleteVertexArrayObject(context, object);
    }

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glGenVertexArraysOES (
    GLsizei n,
    GLuint *arrays
)
{
#if gcdNULL_DRIVER < 3
    GLsizei i;

	glmENTER1(glmARGINT, n)
	{
    glmPROFILE(context, GLES2_GENVERTEXARRAYOES, 0);

    /* Test for invalid value. */
    if (n < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Loop while there are buffers to generate. */
    for (i = 0; i < n; ++i)
    {
        /* Create a new GLbuffer object. */
        GLVertexArray array = _NewVertexArray(context, 0);
        if (array == gcvNULL)
        {
            break;
        }

        /* Return the buffer name. */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                      "%s(%d): array object name %u",
                      __FUNCTION__, __LINE__, array->object.name);
        arrays[i] = array->object.name;
    }

    gcmDUMP_API("${ES20 glGenVertexArraysOES 0x%08X (0x%08X)", n, arrays);
    gcmDUMP_API_ARRAY(arrays, n);
    gcmDUMP_API("$}");
	}
	glmLEAVE();
#else
    while (n-- > 0)
    {
        *arrays++ = 4;
    }
#endif
}

GL_APICALL GLboolean GL_APIENTRY
glIsVertexArrayOES (
    GLuint array
)
{
#if gcdNULL_DRIVER < 3
    GLObject object;
    GLboolean isArray = GL_FALSE;

	glmENTER1(glmARGUINT, array)
	{
    glmPROFILE(context, GLES2_ISVERTEXARRAYOES, 0);

    /* Find the object. */
    object = _glshFindObject(&context->vertexArrayObjects, array);

    isArray = (object != gcvNULL) && (object->type == GLObject_VertexArray);

    gcmTRACE(gcvLEVEL_VERBOSE,
             "glIsVertexArrayOES ==> %s",
             isArray ? "GL_TRUE" : "GL_FALSE");

    gcmDUMP_API("${ES20 glIsVertexArrayOES 0x%08X := 0x%08X}", array, isArray);
	}
	glmLEAVE1(glmARGINT, isArray);
    return isArray;
#else
    return (array == 4) ? GL_TRUE : GL_FALSE;
#endif
}
