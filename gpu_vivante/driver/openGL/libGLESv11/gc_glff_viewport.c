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

#define _GC_OBJ_ZONE glvZONE_VIEWPORT

/******************************************************************************\
********************** Individual State Setting Functions **********************
\******************************************************************************/

static gceSTATUS _ProgramViewport(
    glsCONTEXT_PTR Context
    )
{
    /* Determine viewport coordinates. */
    gctINT32 left
        = Context->viewportStates.viewportBox[0];

    gctINT32 top
        = Context->viewportStates.viewportBox[1];

    gctINT32 right
        = left
        + Context->viewportStates.viewportBox[2];

    gctINT32 bottom
        = top
        + Context->viewportStates.viewportBox[3];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Set uViewport dirty. */
    Context->vsUniformDirty.uViewportDirty = gcvTRUE;

    /* Set viewport. */
    status = gco3D_SetViewport(
        Context->hw, left, bottom, right, top
        );

    gcmFOOTER();
    return status;
}


/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  glfSetDefaultViewportStates
**
**  Set default viewport states.
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

gceSTATUS glfSetDefaultViewportStates(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Reset the clipped out flag. */
    Context->viewportStates.clippedOut = GL_FALSE;

    /* Invalidate the clipping box. */
    Context->viewportStates.recomputeClipping = GL_FALSE;
    Context->viewportStates.reprogramClipping = GL_TRUE;

    /* Disable scissor test. */
    Context->viewportStates.scissorTest = GL_FALSE;

    /* Set default viewport. */
    Context->viewportStates.viewportBox[0] = 0;
    Context->viewportStates.viewportBox[1] = 0;
    Context->viewportStates.viewportBox[2] = Context->drawWidth;
    Context->viewportStates.viewportBox[3] = Context->drawHeight;

    /* Set default clipped viewport. */
    Context->viewportStates.viewportClippedBox[0] = 0;
    Context->viewportStates.viewportClippedBox[1] = 0;
    Context->viewportStates.viewportClippedBox[2] = Context->drawWidth;
    Context->viewportStates.viewportClippedBox[3] = Context->drawHeight;

    /* Set default scissors. */
    Context->viewportStates.scissorBox[0] = 0;
    Context->viewportStates.scissorBox[1] = 0;
    Context->viewportStates.scissorBox[2] = Context->drawWidth;
    Context->viewportStates.scissorBox[3] = Context->drawHeight;

    /* Program viewport. */
    status = _ProgramViewport(Context);

    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  glfUpdateClipping
**
**  Update and program the clipping box.
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

gceSTATUS glfUpdateClpping(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x", Context);

    /* Recompute the clipping box. */
    if (Context->viewportStates.recomputeClipping)
    {
        /* Make shortcuts. */
        GLint* viewportClipped = Context->viewportStates.viewportClippedBox;
        GLint* scissorClipped  = Context->viewportStates.scissorClippedBox;
        GLint* scissors = Context->viewportStates.scissorBox;
        GLint* viewport = Context->viewportStates.viewportBox;

        GLint drawWidth  = (GLint)Context->drawWidth;
        GLint drawHeight = (GLint)Context->drawHeight;

        if (Context->frameBuffer != gcvNULL)
        {
            gcoSURF color, depth;

            /* Get color surface. */
            color = glfGetFramebufferSurface(&Context->frameBuffer->color);

            /* Get depth surface. */
            depth = glfGetFramebufferSurface(&Context->frameBuffer->depth);

            if (color != gcvNULL)
            {
                gcmONERROR(gcoSURF_GetSize(
                                  color,
                                  (gctUINT*)&drawWidth,
                                  (gctUINT*)&drawHeight,
                                  gcvNULL
                                  ));
            }

            if (depth != gcvNULL)
            {
                gcmONERROR(gcoSURF_GetSize(
                                  depth,
                                  (gctUINT*)&drawWidth,
                                  (gctUINT*)&drawHeight,
                                  gcvNULL
                                  ));
            }
        }

        /* Clip the viewport against the draw surface */
        viewportClipped[0] = gcmMAX(0, viewport[0]);

        viewportClipped[1] = gcmMAX(0, viewport[1]);

        viewportClipped[2] = gcmMIN(viewport[0] + viewport[2], drawWidth)
                           - viewportClipped[0];

        viewportClipped[3] = gcmMIN(viewport[1] + viewport[3], drawHeight)
                           - viewportClipped[1];

        /* Clip the scissor against the clipped viewport. */
        scissorClipped[0]  = gcmMAX(scissors[0], viewportClipped[0]);

        scissorClipped[1]  = gcmMAX(scissors[1], viewportClipped[1]);

        scissorClipped[2]  = gcmMIN(scissors[0] + scissors[2],
                                 viewportClipped[0] + viewportClipped[2])
                           - scissorClipped[0];

        scissorClipped[3]  = gcmMIN(scissors[1] + scissors[3],
                                 viewportClipped[1] + viewportClipped[3])
                           - scissorClipped[1];

        /* Reset the flag. */
        Context->viewportStates.recomputeClipping = GL_FALSE;

        /* Set to reprogram. */
        Context->viewportStates.reprogramClipping = GL_TRUE;
    }

    /* Determine the clipped out flag. */
    Context->viewportStates.clippedOut
        = Context->viewportStates.scissorTest
        && ((Context->viewportStates.scissorClippedBox[2] <= 0) ||
            (Context->viewportStates.scissorClippedBox[3] <= 0));

    /* Program the clipping box. */
    if (Context->viewportStates.reprogramClipping &&
        !Context->viewportStates.clippedOut)
    {
        /* Determine the source. */
        GLint* rect = Context->viewportStates.scissorTest
            ? Context->viewportStates.scissorClippedBox
            : Context->viewportStates.viewportClippedBox;

        /* Determine scissor coordinates. */
        gctINT32 left   = rect[0];
        gctINT32 top    = rect[1];
        gctINT32 right  = left + rect[2];
        gctINT32 bottom = top  + rect[3];

        /* Set viewport. */
        status = gco3D_SetScissors(Context->hw, left, top, right, bottom);

        /* Reset the flag. */
        Context->viewportStates.reprogramClipping = GL_FALSE;
    }

OnError:
    gcmFOOTER();
    /* Return result. */
    return status;
}


/*******************************************************************************
**
**  glfEnableScissorTest
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

GLenum glfEnableScissorTest(
    glsCONTEXT_PTR Context,
    GLboolean Enable
    )
{
    gcmHEADER_ARG("Context=0x%x Enable=%d", Context, Enable);

    Context->viewportStates.reprogramClipping = GL_TRUE;

    /* Set scissor test enable. */
    Context->viewportStates.scissorTest = Enable;

    gcmFOOTER_ARG("status=0x%04x", GL_NO_ERROR);
    /* Success. */
    return GL_NO_ERROR;
}


/*******************************************************************************
**
**  glfQueryViewportState
**
**  Queries viewport state values.
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
**
*/

GLboolean glfQueryViewportState(
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
    case GL_VIEWPORT:
        glfGetFromIntArray(
            Context->viewportStates.viewportBox,
            gcmCOUNTOF(Context->viewportStates.viewportBox),
            Value,
            Type
            );
        break;

    case GL_SCISSOR_BOX:
        glfGetFromIntArray(
            Context->viewportStates.scissorBox,
            gcmCOUNTOF(Context->viewportStates.scissorBox),
            Value,
            Type
            );
        break;

    case GL_SCISSOR_TEST:
        glfGetFromInt(
            Context->viewportStates.scissorTest,
            Value,
            Type
            );
        break;

    case GL_MAX_VIEWPORT_DIMS:
        {
            static GLint dimensions[] = { 2048, 2048 };

            glfGetFromIntArray(
                dimensions,
                gcmCOUNTOF(dimensions),
                Value,
                Type
                );
        }
        break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("status=%d", result);
    /* Return result. */
    return result;
}


/******************************************************************************\
**************************** Viewport Management Code **************************
\******************************************************************************/

#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_VIEWPORT

void glfViewport(
    GLint X,
    GLint Y,
    GLsizei Width,
    GLsizei Height
    )
{
    glmENTER4(glmARGINT, X, glmARGINT, Y, glmARGINT, Width, glmARGINT, Height)
    {
		gcmDUMP_API("${ES11 glViewport 0x%08X 0x%08X 0x%08X 0x%08X}", X, Y, Width, Height);

        glmPROFILE(context, GLES1_VIEWPORT, 0);
        if ((Width < 0) || (Height < 0))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Save viewport states. */
        context->viewportStates.viewportBox[0] = X;
        context->viewportStates.viewportBox[1] = Y;
        context->viewportStates.viewportBox[2] = Width;
        context->viewportStates.viewportBox[3] = Height;

        /* Program viewport. */
        gcmVERIFY_OK(_ProgramViewport(context));

        /* Invalidate the clipping box. */
        context->viewportStates.recomputeClipping = GL_TRUE;
    }
    glmLEAVE();
}

void glfScissor(
    GLint X,
    GLint Y,
    GLsizei Width,
    GLsizei Height
    )
{
    glmENTER4(glmARGINT, X, glmARGINT, Y, glmARGINT, Width, glmARGINT, Height)
    {
		gcmDUMP_API("${ES11 glScissor 0x%08X 0x%08X 0x%08X 0x%08X}", X, Y, Width, Height);

		glmPROFILE(context, GLES1_SCISSOR, 0);
        if ((Width < 0) || (Height < 0))
        {
            /* Invalid value. */
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Save scissor states. */
        context->viewportStates.scissorBox[0] = X;
        context->viewportStates.scissorBox[1] = Y;
        context->viewportStates.scissorBox[2] = Width;
        context->viewportStates.scissorBox[3] = Height;

        /* Invalidate the clipping box. */
        context->viewportStates.recomputeClipping = GL_TRUE;
    }
    glmLEAVE();
}

/*******************************************************************************
**
**  glViewport
**
**  glViewport specifies the affine transformation of x and y from normalized
**  device coordinates to window coordinates. Let (xnd, ynd) be normalized
**  device coordinates. Then the window coordinates (xw, yw) are computed as
**  follows:
**
**      Xw = ( Xnd + 1 ) ^ width / 2 + x
**      Yw = ( Ynd + 1 ) ^ height / 2 + y
**
**  Viewport width and height are silently clamped to a range that depends on
**  the implementation. To query this range, call glGetInteger with argument
**  GL_MAX_VIEWPORT_DIMS.
**
**  INPUT:
**
**      X, Y
**          Specify the lower left corner of the viewport rectangle, in pixels.
**          The initial value is (0, 0).
**
**      Width, Height
**          Specify the width and height of the viewport. When a GL context is
**          first attached to a surface (e.g. window), width and height are set
**          to the dimensions of that surface.
**
**  OUTPUT:
**
**      Nothing.
*/
GL_API void GL_APIENTRY glViewport(
    GLint X,
    GLint Y,
    GLsizei Width,
    GLsizei Height
    )
{
    glfViewport(X, Y, Width, Height);
}


/*******************************************************************************
**
**  glScissor
**
**  glScissor defines a rectangle, called the scissor box, in window
**  coordinates. The first two arguments, x and y, specify the lower left corner
**  of the box. width and height specify the width and height of the box.
**
**  To enable and disable the scissor test, call glEnable and glDisable with
**  argument GL_SCISSOR_TEST. The scissor test is initially disabled. While
**  scissor test is enabled, only pixels that lie within the scissor box can be
**  modified by drawing commands. Window coordinates have integer values at
**  the shared corners of frame buffer pixels. glScissor(0, 0, 1, 1) allows
**  modification of only the lower left pixel in the window, and
**  glScissor(0, 0, 0, 0) doesn't allow modification of any pixels
**  in the window.
**
**  When the scissor test is disabled, it is as though the scissor box includes
**  the entire window.
**
**  INPUT:
**
**      X, Y
**          Specify the lower left corner of the scissor box, in pixels.
**          The initial value is (0, 0).
**
**      Width, Height
**          Specify the width and height of the scissor box. When a GL context
**          is first attached to a surface (e.g. window), width and height are
**          set to the dimensions of that surface.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glScissor(
    GLint X,
    GLint Y,
    GLsizei Width,
    GLsizei Height
    )
{
    glfScissor(X, Y, Width, Height);
}
