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
**  _QueryState
**
**  Simple state query main entry.
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

typedef GLboolean (* glfQUERYSTATE) (
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    );

static glfQUERYSTATE _Query[] =
{
    glfQueryTextureState,
    glfQueryVertexState,
    glfQueryFogState,
    glfQueryLightingState,
    glfQueryPointState,
    glfQueryLineState,
    glfQueryPixelState,
    glfQueryAlphaState,
    glfQueryCullState,
    glfQueryDepthState,
    glfQueryViewportState,
    glfQueryMatrixState,
    glfQueryBufferState,
    glfQueryMultisampleState,
    glfQueryMiscState
};

static GLboolean _QueryState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLuint i;
    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%04x",
                    Context, Name, Value, Type);

    for (i = 0; i < gcmCOUNTOF(_Query); i++)
    {
        if ((* _Query[i]) (Context, Name, Value, Type))
        {
            gcmFOOTER_ARG("return=%d", GL_TRUE);
            return GL_TRUE;
        }
    }

    gcmFOOTER_ARG("return=%d", GL_FALSE);
    return GL_FALSE;
}


/******************************************************************************\
************************** State Query Management Code *************************
\******************************************************************************/

/*******************************************************************************
**
**  glGetError
**
**  glGetError returns the value of the error flag. Each detectable error is
**  assigned a numeric code and symbolic name. When an error occurs, the error
**  flag is set to the appropriate error code value. No other errors are
**  recorded until glGetError is called, the error code is returned, and the
**  flag is reset to GL_NO_ERROR. If a call to glGetError returns GL_NO_ERROR,
**  there has been no detectable error since the last call to glGetError, or
**  since the GL was initialized.
**
**  To allow for distributed implementations, there may be several error flags.
**  If any single error flag has recorded an error, the value of that flag is
**  returned and that flag is reset to GL_NO_ERROR when glGetError is called.
**  If more than one flag has recorded an error, glGetError returns and clears
**  an arbitrary error flag value. Thus, glGetError should always be called in
**  a loop, until it returns GL_NO_ERROR, if all error flags are to be reset.
**
**  Initially, all error flags are set to GL_NO_ERROR.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      GL error code.
*/

GL_API GLenum GL_APIENTRY glGetError(
    void
    )
{
    glsCONTEXT_PTR context;
    GLenum error;

	glmDeclareTimeVar()
	gcmHEADER();
 	glmGetApiStartTime();

    /* Get current context. */
    context = GetCurrentContext();

    if (context == gcvNULL)
    {
        /* No current context. */
        gcmFOOTER_NO();
        return GL_NO_ERROR;
    }

    glmPROFILE(context, GLES1_GETERROR, 0);
    /* Save the error code. */
    error = context->error;

    /* Reset the error if any. */
    context->error = GL_NO_ERROR;

	gcmDUMP_API("${ES11 glGetError := 0x%08X}", error);

    glmGetApiEndTime(context);

    /* Return the last error. */
    gcmFOOTER_NO();
    return error;
}


/*******************************************************************************
**
**  glGetXXX
**
**  Returns values for simple state variables in GL. 'Value' is a symbolic
**  constant indicating the state variable to be returned, and 'Data' is
**  a pointer to an array of the indicated type in which to place the returned
**  data.
**
**  INPUT:
**
**      Value
**          Specifies the parameter value to be returned. The symbolic constants
**          in the list below are accepted.
**
**  OUTPUT:
**
**      Data
**          Returns the value or values of the specified parameter.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_QUERY

GL_API void GL_APIENTRY glGetBooleanv(
    GLenum Value,
    GLboolean* Data
    )
{
    glmENTER2(glmARGENUM, Value, glmARGPTR, Data)
    {
        glmPROFILE(context, GLES1_GETBOOLEANV, 0);
        if (!_QueryState(context, Value, Data, glvBOOL))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

		/* TODO, Size unknown; */
		gcmDUMP_API("${ES11 glGetBooleanv 0x%08X (0x%08X)}", Value, Data);
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetIntegerv(
    GLenum Value,
    GLint* Data
    )
{
    glmENTER2(glmARGENUM, Value, glmARGPTR, Data)
    {
        glmPROFILE(context, GLES1_GETINTEGERV, 0);
        if (!_QueryState(context, Value, Data, glvINT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

		/* TODO, Size unknown; */
		gcmDUMP_API("${ES11 glGetIntegerv 0x%08X (0x%08X)", Value, Data);
		gcmDUMP_API_ARRAY(Data, 1);
		gcmDUMP_API("$}");
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetFixedv(
    GLenum Value,
    GLfixed* Data
    )
{
    glmENTER2(glmARGENUM, Value, glmARGPTR, Data)
    {
        glmPROFILE(context, GLES1_GETFIXEDV, 0);
        if (!_QueryState(context, Value, Data, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

		/* TODO, Size unknown; */
		gcmDUMP_API("${ES11 glGetFixedv 0x%08X (0x%08X)}", Value, Data);
		gcmDUMP_API_ARRAY(Data, 1);
		gcmDUMP_API("$}");
	}
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetFixedvOES(
    GLenum Value,
    GLfixed* Data
    )
{
    glmENTER2(glmARGENUM, Value, glmARGPTR, Data)
    {
        if (!_QueryState(context, Value, Data, glvFIXED))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

		/* TODO, Size unknown; */
		gcmDUMP_API("${ES11 glGetFixedvOES 0x%08X (0x%08X)}", Value, Data);
    }
    glmLEAVE();
}

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glGetFloatv(
    GLenum Value,
    GLfloat* Data
    )
{
    glmENTER2(glmARGENUM, Value, glmARGPTR, Data)
    {
        glmPROFILE(context, GLES1_GETFLOATV, 0);
        if (!_QueryState(context, Value, Data, glvFLOAT))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

		/* TODO, Size unknown; */
		gcmDUMP_API("${ES11 glGetFloatv 0x%08X (0x%08X)}", Value, Data);
    }
    glmLEAVE();
}
#endif


/*******************************************************************************
**
**  glGetPointerv
**
**  glGetPointer returns pointer information. Name is a symbolic constant
**  indicating the pointer to be returned, and Params is a pointer to a location
**  in which to place the returned data.
**
**  INPUT:
**
**      Name
**          Specifies the array or buffer pointer to be returned.
**
**  OUTPUT:
**
**      Params
**          Returns the pointer value specified by Name.
*/

GL_API void GL_APIENTRY glGetPointerv(
    GLenum Name,
    GLvoid** Params
    )
{
    glmENTER2(glmARGENUM, Name, glmARGPTR, Params)
    {
        glmPROFILE(context, GLES1_GETPOINTERV, 0);
        switch (Name)
        {
        case GL_VERTEX_ARRAY_POINTER:
            *Params = (GLvoid *) context->aPositionInfo.pointer;
            break;

        case GL_NORMAL_ARRAY_POINTER:
            *Params = (GLvoid *) context->aNormalInfo.pointer;
            break;

        case GL_COLOR_ARRAY_POINTER:
            *Params = (GLvoid *) context->aColorInfo.pointer;
            break;

        case GL_TEXTURE_COORD_ARRAY_POINTER:
            *Params = (GLvoid *) context->texture.activeClientSampler->
                                 aTexCoordInfo.pointer;
            break;

        case GL_POINT_SIZE_ARRAY_POINTER_OES:
            *Params = (GLvoid *) context->aPointSizeInfo.pointer;
            break;

        case GL_MATRIX_INDEX_ARRAY_POINTER_OES:
            *Params = (GLvoid *) context->aMatrixIndexInfo.pointer;
            break;

        case GL_WEIGHT_ARRAY_POINTER_OES:
            *Params = (GLvoid *) context->aWeightInfo.pointer;
            break;

        default:
            glmERROR(GL_INVALID_ENUM);
        }

		/* TODO, Size unknown; */
		gcmDUMP_API("${ES11 glGetPointerv 0x%08X (0x%08X)}", Name, Params);
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glGetString
**
**  glGetString returns a pointer to a static string describing some aspect
**  of the current GL connection.
**
**  INPUT:
**
**      Name
**          Specifies a symbolic constant, one of GL_VENDOR, GL_RENDERER,
**          GL_VERSION, or GL_EXTENSIONS.
**
**  OUTPUT:
**
**      String describing the current GL connection.
*/

GL_API const GLubyte * GL_APIENTRY glGetString(
    GLenum Name
    )
{
    GLubyte * pointer = gcvNULL;

    glmENTER1(glmARGENUM, Name)
    {
        glmPROFILE(context, GLES1_GETSTRING, 0);
        switch (Name)
        {
        case GL_VENDOR:
            pointer = context->chipInfo.vendor;
            break;

        case GL_RENDERER:
            pointer = context->chipInfo.renderer;
            break;

        case GL_VERSION:
            pointer = context->chipInfo.version;
            break;

        case GL_EXTENSIONS:
            pointer = (GLubyte*)context->chipInfo.extensions;
            break;

        default:
            glmERROR(GL_INVALID_ENUM);
            /* For QNX, it's mandatory to return gcvNULL. */
        }

		/* TODO, Size unknown; */
#if gcdDUMP
		{
			gctSIZE_T len;
			gcoOS_StrLen((gctCONST_STRING)pointer, &len);
			gcmDUMP_API("${ES11 glGetString 0x%08X :=", Name);
			gcmDUMP_API_DATA(pointer, len);
			gcmDUMP_API("$}");
		}
#endif
    }
    glmLEAVE();

    return pointer;
}
