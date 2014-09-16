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

static GLenum _ProgramCulling(
    glsCONTEXT_PTR Context
    )
{
    gceCULL mode;
    GLenum result;
    gcmHEADER_ARG("Context=0x%x", Context);

    if (Context->cullStates.enabled)
    {
        if (Context->cullStates.cullFace == GL_FRONT)
        {
            if (Context->cullStates.frontFace == GL_CCW)
            {
                mode = gcvCULL_CW;
            }
            else
            {
                mode = gcvCULL_CCW;
            }
        }
        else if (Context->cullStates.cullFace == GL_BACK)
        {
            if (Context->cullStates.frontFace == GL_CCW)
            {
                mode = gcvCULL_CCW;
            }
            else
            {
                mode = gcvCULL_CW;
            }
        }
        else
        {
            mode = gcvCULL_NONE;
        }
    }
    else
    {
        mode = gcvCULL_NONE;
    }

    result = glmTRANSLATEHALSTATUS(gco3D_SetCulling(Context->hw, mode));
    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _SetFrontFace(
    glsCONTEXT_PTR Context,
    GLenum FrontFace
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x FrontFace=0x%04x", Context, FrontFace);

    switch (FrontFace)
    {
    case GL_CW:
    case GL_CCW:
        /* Set the query state. */
        Context->cullStates.frontFace = FrontFace;

        /* Determine front facing flag. */
        Context->hashKey.hashClockwiseFront =
        Context->cullStates.clockwiseFront = (FrontFace == GL_CCW);

        /* Program the states. */
        result = _ProgramCulling(Context);
        break;

    default:
        result = GL_INVALID_ENUM;
    }

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}

static GLenum _SetCullFace(
    glsCONTEXT_PTR Context,
    GLenum CullFace
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x CullFace=0x%04x", Context, CullFace);

    switch (CullFace)
    {
    case GL_FRONT:
    case GL_BACK:
    case GL_FRONT_AND_BACK:
        Context->cullStates.cullFace = CullFace;
        result = _ProgramCulling(Context);
        break;

    default:
        result = GL_INVALID_ENUM;
    }

    gcmFOOTER_ARG("return=0x%04x", result);

    return result;
}


/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  glfSetDefaultCullingStates
**
**  Set default culling states.
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

gceSTATUS glfSetDefaultCullingStates(
    glsCONTEXT_PTR Context
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x", Context);
    Context->cullStates.enabled   = GL_FALSE;
    Context->cullStates.frontFace = GL_CCW;
    Context->cullStates.cullFace  = GL_BACK;

    do
    {
        glmERR_BREAK(_SetFrontFace(Context, GL_CCW));

        glmERR_BREAK(_ProgramCulling(Context));
    }
    while (gcvFALSE);

    result = glmTRANSLATEGLRESULT(result);
    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}


/*******************************************************************************
**
**  glfEnableCulling
**
**  Enable culling.
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

GLenum glfEnableCulling(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    GLenum result;
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);
    Context->cullStates.enabled = Enable;
    result = _ProgramCulling(Context);
    gcmFOOTER_ARG("return=0x%04x", result);
    return result;
}


/*******************************************************************************
**
**  glfQueryCullState
**
**  Queries culling state values.
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

GLboolean glfQueryCullState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;
    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%04x", Context, Name, Value, Type);

    switch (Name)
    {
    case GL_CULL_FACE:
        glfGetFromInt(
            Context->cullStates.enabled,
            Value,
            Type
            );
        break;

    case GL_CULL_FACE_MODE:
        glfGetFromEnum(
            Context->cullStates.cullFace,
            Value,
            Type
            );
        break;

    case GL_FRONT_FACE:
        glfGetFromEnum(
            Context->cullStates.frontFace,
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
********************* OpenGL Fragment State Management Code ********************
\******************************************************************************/

/*******************************************************************************
**
**  glFrontFace
**
**  In a scene composed entirely of opaque closed surfaces, back-facing polygons
**  are never visible. Eliminating (culling) these invisible polygons has the
**  obvious benefit of speeding up the rendering of the image. To enable and
**  disable culling, call glEnable and glDisable with argument GL_CULL_FACE.
**  Culling is initially disabled.
**
**  The projection of a polygon to window coordinates is said to have clockwise
**  winding if an imaginary object following the path from its first vertex,
**  its second vertex, and so on, to its last vertex, and finally back to its
**  first vertex, moves in a clockwise direction about the interior of the
**  polygon. The polygon's winding is said to be counterclockwise if the
**  imaginary object following the same path moves in a counterclockwise
**  direction about the interior of the polygon. glFrontFace specifies whether
**  polygons with clockwise winding in window coordinates, or counterclockwise
**  winding in window coordinates, are taken to be front-facing. Passing GL_CCW
**  to mode selects counterclockwise polygons as front-facing. GL_CW selects
**  clockwise polygons as front-facing. By default, counterclockwise polygons
**  are taken to be front-facing.
**
**  INPUT:
**
**      Mode
**          Specifies the orientation of front-facing polygons.
**          GL_CW and GL_CCW are accepted. The initial value is GL_CCW.
**
**  OUTPUT:
**
**      Nothing.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_LIGHT

GL_API void GL_APIENTRY glFrontFace(
    GLenum Mode
    )
{
    glmENTER1(glmARGENUM, Mode)
    {
		gcmDUMP_API("${ES11 glFrontFace 0x%08X}", Mode);

		glmPROFILE(context, GLES1_FRONTFACE, 0);
        glmERROR(_SetFrontFace(context, Mode));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glCullFace
**
**  glCullFace specifies whether front- or back-facing polygons are culled
**  (as specified by mode) when culling is enabled. To enable and disable
**  culling, call glEnable and glDisable with argument GL_CULL_FACE.
**  Culling is initially disabled.
**
**  INPUT:
**
**      Mode
**          Specifies whether front- or back-facing polygons are culled.
**          Symbolic constants GL_FRONT, GL_BACK, and GL_FRONT_AND_BACK
**          are accepted. The initial value is GL_BACK.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_POLIGON

GL_API void GL_APIENTRY glCullFace(
    GLenum Mode
    )
{
    glmENTER1(glmARGENUM, Mode)
    {
		gcmDUMP_API("${ES11 glCullFace 0x%08X}", Mode);

		glmPROFILE(context, GLES1_CULLFACE, 0);
        glmERROR(_SetCullFace(context, Mode));
    }
    glmLEAVE();
}
