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

#define _GC_OBJ_ZONE    glvZONE_CLIP

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  _SetClipPlane
**
**  Set the clip plane equation.
**
**  INPUT:
**
**      Context
**          Pointer to the current context object.
**
**      Plane
**          Specifies the plane number
**
**      Equation
**          Specifies a pointer to the values of clip plane
**
**      Type
**          Specifies the type of Value.
**
**  OUTPUT:
**
**      Nothing.
*/

static void _SetClipPlane(
    glsCONTEXT_PTR Context,
    GLint Plane,
    const GLvoid* Equation,
    gleTYPE Type
    )
{
    /* Get model view inverse transposed matrix. */
    glsMATRIX_PTR matrix;

    gcmHEADER_ARG("Context=0x%x Plane=%d Equation=0x%x Type=0x%04x", Context, Plane, Equation, Type);

    matrix = glfGetModelViewInverse4x4TransposedMatrix(Context);

    /* Set clip plane vector. */
    glfSetVector4(&Context->clipPlane[Plane], Equation, Type);

    /* Transform clip plane to the eye space. */
    if (!matrix->identity)
    {
        glfMultiplyVector4ByMatrix4x4(
            &Context->clipPlane[Plane],
            matrix,
            &Context->clipPlane[Plane]
            );
    }

    /* Set uClipPlane dirty. */
    Context->vsUniformDirty.uClipPlaneDirty = gcvTRUE;

    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  glfSetDefaultDepthStates
**
**  Set default depth states.
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

gceSTATUS glfSetDefaultClipPlaneStates(
    glsCONTEXT_PTR Context
    )
{
    gctUINT32 i;
    gcmHEADER_ARG("Context=0x%x", Context);

    for (i = 0; i < glvMAX_CLIP_PLANES; ++i)
    {
        glfSetFracVector4(
            &Context->clipPlane[i],
            glvFRACZERO,
            glvFRACZERO,
            glvFRACZERO,
            glvFRACZERO
            );
    }

    gcmFOOTER_ARG("return=%s", "gcvSTATUS_OK");

    return gcvSTATUS_OK;
}


/******************************************************************************\
******************** OpenGL Clip Plane State Management Code *******************
\******************************************************************************/

/*******************************************************************************
**
**  glClipPlane
**
**  Geometry is always clipped against the boundaries of a six-plane frustum in
**  x, y, and z. glClipPlane allows the specification of additional planes, not
**  necessarily perpendicular to the x, y, or z axis, against which all geometry
**  is clipped. To determine the maximum number of additional clipping planes,
**  call glGet with argument GL_MAX_CLIP_PLANES. All implementations support at
**  least one such clipping planes. Because the resulting clipping region is the
**  intersection of the defined half-spaces, it is always convex.
**
**  glClipPlane specifies a half-space using a four-component plane equation.
**  When glClipPlane is called, equation is transformed by the inverse of the
**  modelview matrix and stored in the resulting eye coordinates. Subsequent
**  changes to the modelview matrix have no effect on the stored plane-equation
**  components. If the dot product of the eye coordinates of a vertex with the
**  stored plane equation components is positive or zero, the vertex is in with
**  respect to that clipping plane. Otherwise, it is out.
**
**  To enable and disable clipping planes, call glEnable and glDisable with the
**  argument GL_CLIP_PLANEi, where i is the plane number.
**
**  All clipping planes are initially defined as (0, 0, 0, 0) in eye coordinates
**  and are disabled.
**
**  INPUT:
**
**      Plane
**          Specifies which clipping plane is being positioned. Symbolic names
**          of the form GL_CLIP_PLANEi where 0 < i < GL_MAX_CLIP_PLANES are
**          accepted.
**
**      Equation
**          Specifies the address of an array of four fixed-point or
**          floating-point values. These values are interpreted as a plane
**          equation.
**
**  OUTPUT:
**
**      Nothing.
*/

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glClipPlanef(
    GLenum Plane,
    const GLfloat* Equation
    )
{
    glmENTER2(glmARGENUM, Plane, glmARGPTR, Equation)
    {
        GLint plane = Plane - GL_CLIP_PLANE0;

		gcmDUMP_API("${ES11 glClipPlanef 0x%08X (0x%08x)", Plane, Equation);
		gcmDUMP_API_ARRAY(Equation, 4);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_CLIPPLANEF, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        _SetClipPlane(context, plane, Equation, glvFLOAT);
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glClipPlanefOES(
    GLenum Plane,
    const GLfloat* Equation
    )
{
    glmENTER2(glmARGENUM, Plane, glmARGPTR, Equation)
    {
        GLint plane = Plane - GL_CLIP_PLANE0;

		gcmDUMP_API("${ES11 glClipPlanefOES 0x%08X (0x%08x)", Plane, Equation);
		gcmDUMP_API_ARRAY(Equation, 4);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_CLIPPLANEF, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        _SetClipPlane(context, plane, Equation, glvFLOAT);
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glClipPlanefIMG(
    GLenum p,
    const GLfloat *eqn
    )
{
    glmENTER2(glmARGENUM, p, glmARGPTR, eqn)
    {
        GLint plane = p - GL_CLIP_PLANE0;

		gcmDUMP_API("${ES11 glClipPlanefIMG 0x%08X (0x%08x)", p, eqn);
		gcmDUMP_API_ARRAY(eqn, 4);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_CLIPPLANEF, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        _SetClipPlane(context, plane, eqn, glvFLOAT);
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glClipPlanex(
    GLenum Plane,
    const GLfixed* Equation
    )
{
    glmENTER2(glmARGENUM, Plane, glmARGPTR, Equation)
    {
        GLint plane = Plane - GL_CLIP_PLANE0;

		gcmDUMP_API("${ES11 glClipPlanex 0x%08X (0x%08x)", Plane, Equation);
		gcmDUMP_API_ARRAY(Equation, 4);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_CLIPPLANEX, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        _SetClipPlane(context, plane, Equation, glvFIXED);
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glClipPlanexOES(
    GLenum Plane,
    const GLfixed* Equation
    )
{
    glmENTER2(glmARGENUM, Plane, glmARGPTR, Equation)
    {
        GLint plane = Plane - GL_CLIP_PLANE0;

		gcmDUMP_API("${ES11 glClipPlanexOES 0x%08X (0x%08x)", Plane, Equation);
		gcmDUMP_API_ARRAY(Equation, 4);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_CLIPPLANEX, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        _SetClipPlane(context, plane, Equation, glvFIXED);
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glClipPlanexIMG(
    GLenum p,
    const GLfixed* eqn
    )
{
    glmENTER2(glmARGENUM, p, glmARGPTR, eqn)
    {
        GLint plane = p - GL_CLIP_PLANE0;

		gcmDUMP_API("${ES11 glClipPlanexIMG 0x%08X (0x%08x)", p, eqn);
		gcmDUMP_API_ARRAY(eqn, 4);
		gcmDUMP_API("$}");

		glmPROFILE(context, GLES1_CLIPPLANEX, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        _SetClipPlane(context, plane, eqn, glvFIXED);
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glGetClipPlane
**
**  glGetClipPlane returns in equation the four coefficients of the plane
**  equation for plane.
**
**  INPUT:
**
**      Plane
**          Specifies which clipping plane is being positioned. Symbolic names
**          of the form GL_CLIP_PLANEi where 0 < i < GL_MAX_CLIP_PLANES are
**          accepted.
**
**      Equation
**          Specifies the address of an array of four fixed-point or
**          floating-point values. These values are interpreted as a plane
**          equation.
**
**  OUTPUT:
**
**      Nothing.
*/

#undef  _GC_OBJ_ZONE
#define _GC_OBJ_ZONE    glvZONE_QUERY

#ifndef COMMON_LITE
GL_API void GL_APIENTRY glGetClipPlanef(
    GLenum Plane,
    GLfloat Equation[4]
    )
{
    glmENTER2(glmARGENUM, Plane, glmARGPTR, Equation)
    {
        GLint plane = Plane - GL_CLIP_PLANE0;

		glmPROFILE(context, GLES1_GETCLIPPLANEF, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        glfGetFromVector4(
            &context->clipPlane[plane],
            Equation,
            glvFLOAT
            );

		gcmDUMP_API("${ES11 glGetClipPlanef 0x%08X (0x%08x)", Plane, Equation);
		gcmDUMP_API_ARRAY(Equation, 4);
		gcmDUMP_API("$}");
	}
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetClipPlanefOES(
    GLenum Plane,
    GLfloat Equation[4]
    )
{
    glmENTER2(glmARGENUM, Plane, glmARGPTR, Equation)
    {
        GLint plane = Plane - GL_CLIP_PLANE0;

		glmPROFILE(context, GLES1_GETCLIPPLANEF, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        glfGetFromVector4(
            &context->clipPlane[plane],
            Equation,
            glvFLOAT
            );

		gcmDUMP_API("${ES11 glGetClipPlanefOES 0x%08X (0x%08x)", Plane, Equation);
		gcmDUMP_API_ARRAY(Equation, 4);
		gcmDUMP_API("$}");
    }
    glmLEAVE();
}
#endif

GL_API void GL_APIENTRY glGetClipPlanex(
    GLenum Plane,
    GLfixed Equation[4]
    )
{
    glmENTER2(glmARGENUM, Plane, glmARGPTR, Equation)
    {
        GLint plane = Plane - GL_CLIP_PLANE0;

		glmPROFILE(context, GLES1_GETCLIPPLANEX, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        glfGetFromVector4(
            &context->clipPlane[plane],
            Equation,
            glvFIXED
            );

		gcmDUMP_API("${ES11 glGetClipPlanex 0x%08X (0x%08x)", Plane, Equation);
		gcmDUMP_API_ARRAY(Equation, 4);
		gcmDUMP_API("$}");
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glGetClipPlanexOES(
    GLenum Plane,
    GLfixed Equation[4]
    )
{
    glmENTER2(glmARGENUM, Plane, glmARGPTR, Equation)
    {
        GLint plane = Plane - GL_CLIP_PLANE0;

		glmPROFILE(context, GLES1_GETCLIPPLANEX, 0);
        if ((plane < 0) || (plane >= glvMAX_CLIP_PLANES))
        {
            /* Invalid number. */
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        glfGetFromVector4(
            &context->clipPlane[plane],
            Equation,
            glvFIXED
            );

		gcmDUMP_API("${ES11 glGetClipPlanexOES 0x%08X (0x%08x)", Plane, Equation);
		gcmDUMP_API_ARRAY(Equation, 4);
		gcmDUMP_API("$}");
    }
    glmLEAVE();
}
