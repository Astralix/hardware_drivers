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

#define _GC_OBJ_ZONE    glvZONE_CLEAR

#define USE_3D_RENDERING_CLEAR          1

#if USE_3D_RENDERING_CLEAR
static GLboolean
_ClearRect(
    glsCONTEXT_PTR Context,
    GLbitfield Mask
    )
{
    /* Define result. */
    GLboolean result = GL_TRUE;
    gcmHEADER_ARG("Context=0x%x Mask=%u", Context, Mask);

    do
    {
        /* Scissor bounding box coordinate. */
        gltFRACTYPE Xs, Ys, Ws, Hs;

        /* Viewport bounding box size. */
        gltFRACTYPE widthViewport, heightViewport;

        /* Normalized coordinates. */
        gltFRACTYPE normLeft, normTop, normWidth, normHeight;

        /* Rectangle coordinates. */
        gltFRACTYPE left, top, right, bottom;
        gltFRACTYPE vertexBuffer[4 * 3];

        /* Color states. */
        GLboolean   colorMask[4];

        /* Depth states. */
        gltFRACTYPE depthClear = glvFRACZERO;
        GLenum      depthFunc = 0;
        GLboolean   depthWriteMask = GL_FALSE;

        /* Stencil states. */
        GLint       stencilClear = 0;
        GLenum      stencilFunc = GL_ALWAYS;
        GLint       stencilRef = 0;
        GLuint      stencilWriteMask = 0xffff;

        GLuint      stencilMask = 0xffff;
        GLenum      stencilFail = 0;
        GLenum      stencilzFail = 0;
        GLenum      stencilzPass = 0;

        GLboolean   logicOp;
        GLboolean   alphaTest;
        GLboolean   alphaBlend;
        GLboolean   depthTest;
        GLboolean   stencilTest;
        GLboolean   cull;
		GLboolean	matPalette;

        /* Get color states. */
        colorMask[0] = Context->colorMask[0];
        colorMask[1] = Context->colorMask[1];
        colorMask[2] = Context->colorMask[2];
        colorMask[3] = Context->colorMask[3];

        logicOp     = Context->logicOp.enabled;
        alphaTest   = Context->alphaStates.testEnabled;
        alphaBlend  = Context->alphaStates.blendEnabled;
        depthTest   = Context->depthStates.testEnabled;
        stencilTest = Context->stencilStates.testEnabled;
        cull        = Context->cullStates.enabled;
		matPalette	= Context->matrixPaletteEnabled;

        /* Program color states. */
        /* for clear color case, the color mask should affect the clear result according spec */
        if (!(Mask & GL_COLOR_BUFFER_BIT))
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        }

        if (Context->depth)
        {
            /* Get depth states. */
            depthClear = glfFracFromMutant(
                &Context->depthStates.clearValue
                );

            depthWriteMask = Context->depthStates.depthMask;

            glfQueryDepthState(
                Context,
                GL_DEPTH_FUNC,
                &depthFunc,
                glvINT
                );

            /* Get stencil states. */
            stencilClear = Context->stencilStates.clearValue;

            stencilWriteMask = Context->stencilStates.writeMask;

            glfQueryDepthState(
                Context,
                GL_STENCIL_FUNC,
                &stencilFunc,
                glvINT
                );

            stencilRef  = Context->stencilStates.reference;
            stencilMask = Context->stencilStates.valueMask;

            glfQueryDepthState(
                Context,
                GL_STENCIL_FAIL,
                &stencilFail,
                glvINT
                );

            glfQueryDepthState(
                Context,
                GL_STENCIL_PASS_DEPTH_FAIL,
                &stencilzFail,
                glvINT
                );

            glfQueryDepthState(
                Context,
                GL_STENCIL_PASS_DEPTH_PASS,
                &stencilzPass,
                glvINT
                );

            /* Always pass depth test. */
            glDepthFunc(GL_ALWAYS);

            if (Mask & GL_DEPTH_BUFFER_BIT)
            {
                /* Depth test: accept new depth. */
                glDepthMask(GL_TRUE);
                glEnable(GL_DEPTH_TEST);
            }
            else
            {
                /* Not clearing depth, disable writing. */
                glDepthMask(GL_FALSE);
            }

            /* Always pass stencil test. */
            glStencilFunc(GL_ALWAYS, stencilClear, 0xFFFF);

            if (Mask & GL_STENCIL_BUFFER_BIT)
            {
                /* Clearing, replace with ref value(clear value). */
                glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
                glStencilMask(0xFFFF);
                glEnable(GL_STENCIL_TEST);
            }
            else
            {
                /* Not clearing, diable writing. */
                glStencilMask(0x0000);
            }
        }

        /* disable some states to render. */
        glDisable(GL_COLOR_LOGIC_OP);
        glDisable(GL_ALPHA_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);

		if (Context->viewportStates.scissorTest)
		{
			widthViewport  = glmINT2FRAC(Context->viewportStates.viewportBox[2]);
			heightViewport = glmINT2FRAC(Context->viewportStates.viewportBox[3]);

			Xs  = glmINT2FRAC(Context->viewportStates.scissorBox[0]
				    - Context->viewportStates.viewportBox[0]);
			Ys  = glmINT2FRAC(Context->viewportStates.scissorBox[1]
				    - Context->viewportStates.viewportBox[1]);
			Ws  = glmINT2FRAC(Context->viewportStates.scissorBox[2]);
			Hs  = glmINT2FRAC(Context->viewportStates.scissorBox[3]);

			/* Normalize coordinates. */
			normLeft   = glmFRACDIVIDE(Xs, widthViewport);
			normTop    = glmFRACDIVIDE(Ys, heightViewport);
			normWidth  = glmFRACDIVIDE(Ws, widthViewport);
			normHeight = glmFRACDIVIDE(Hs, heightViewport);
		}
		else
		{
			/* Full viewport rendering. */
			normLeft   = (GLfixed)0.0f;
			normTop    = (GLfixed)0.0f;
			normWidth  = (GLfixed)1.0f;
			normHeight = (GLfixed)1.0f;
		}
        /* Transform coordinates. */
        left   = glmFRACMULTIPLY(normLeft,   glvFRACTWO) - glvFRACONE;
        top    = glmFRACMULTIPLY(normTop,    glvFRACTWO) - glvFRACONE;
        right  = glmFRACMULTIPLY(normWidth,  glvFRACTWO) + left;
        bottom = glmFRACMULTIPLY(normHeight, glvFRACTWO) + top;

        /* Create two triangles. */
        vertexBuffer[ 0] = left;
        vertexBuffer[ 1] = top;
        vertexBuffer[ 2] = depthClear;

        vertexBuffer[ 3] = right;
        vertexBuffer[ 4] = top;
        vertexBuffer[ 5] = depthClear;

        vertexBuffer[ 6] = left;
        vertexBuffer[ 7] = bottom;
        vertexBuffer[ 8] = depthClear;

        vertexBuffer[ 9] = right;
        vertexBuffer[10] = bottom;
        vertexBuffer[11] = depthClear;

        /* Update flags. */
        Context->hashKey.hashClearRectEnabled = 1;
        Context->drawClearRectEnabled = GL_TRUE;

        /* Set stream parameters. */
        glfSetStreamParameters(
            Context,
            &Context->aPositionDrawClearRectInfo,
            glvFRACGLTYPEENUM,
            3,
            3 * sizeof (gltFRACTYPE),
            gcvFALSE,
            &vertexBuffer,
            gcvNULL,
            glvTOTALBINDINGS
            );

		Context->matrixPaletteEnabled = GL_FALSE;

        /* Draw the clear rect. */
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		Context->matrixPaletteEnabled = matPalette;

        /* Restore flags. */
        Context->hashKey.hashClearRectEnabled = 0;
        Context->drawClearRectEnabled = GL_FALSE;

        /* Restore color states. */
        if (!(Mask & GL_COLOR_BUFFER_BIT))
        {
            glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
        }

        if (Context->depth)
        {
            /* Restore depth states. */
            glDepthFunc(depthFunc);
            glDepthMask(depthWriteMask);

            /* Restore stencil states. */
            glStencilFunc(stencilFunc, stencilRef, stencilMask);
            glStencilOp(stencilFail, stencilzFail, stencilzPass);
            glStencilMask(stencilWriteMask);

        }

        /* Restore changed states. */
        if (logicOp)
        {
            glEnable(GL_COLOR_LOGIC_OP);
        }

        if (alphaTest)
        {
            glEnable(GL_ALPHA_TEST);
        }

        if (alphaBlend)
        {
            glEnable(GL_BLEND);
        }

        if (!depthTest)
        {
            glDisable(GL_DEPTH_TEST);
        }

        if (!stencilTest)
        {
            glDisable(GL_STENCIL_TEST);
        }

        if (cull)
        {
            glEnable(GL_CULL_FACE);
        }
    }while (gcvFALSE);

    gcmFOOTER_ARG("return=%d", result);

    /* Return status. */
    return result;
}
#endif

/*******************************************************************************
**
**  glClear
**
**  glClear sets the bitplane area of the window to values previously selected
**  by glClearColor, glClearDepth, and glClearStencil.
**
**  The pixel ownership test, the scissor test, dithering, and the color buffer
**  masks affect the operation of glClear. The scissor box bounds the cleared
**  region. Alpha function, blend function, logical operation, stenciling,
**  texture mapping, and depth-buffering are ignored by glClear.
**
**  glClear takes a single argument that is the bitwise OR of several values
**  indicating which buffer is to be cleared.
**
**  The values are as follows:
**      GL_COLOR_BUFFER_BIT   - Indicates the color buffer.
**      GL_DEPTH_BUFFER_BIT   - Indicates the depth buffer.
**      GL_STENCIL_BUFFER_BIT - Indicates the stencil buffer.
**
**  The value to which each buffer is cleared depends on the setting of the
**  clear value for that buffer.
**
**  INPUT:
**
**      Mask
**          Bitwise OR of masks that indicate the buffers to be cleared.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glClear(
    GLbitfield Mask
    )
{
    glmENTER1(glmARGHEX, Mask)
    {
        gceSTATUS status = gcvSTATUS_OK;

		gcmDUMP_API("${ES11 glClear 0x%08X}", Mask);

		glmPROFILE(context, GLES1_CLEAR, 0);

        /* Update frame buffer. */
        gcmERR_BREAK(glfUpdateFrameBuffer(context));

        do
        {
#if USE_3D_RENDERING_CLEAR
            /* For some cases clear can't be done by hw, using 3D rendering */
            /* to do clear will avoid driver use software to do clear.      */
            /* Such optimization may be put in the hal to benefit all APIs. */
            if (context->viewportStates.scissorTest)
            {
	            /* Temporary hack to qualify this feature. */
                if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_SUPER_TILED))
	            {
                    if ((context->viewportStates.scissorBox[0] & 0x3F) ||
                        (context->viewportStates.scissorBox[1] & 0x3F) ||
                        (context->viewportStates.scissorBox[2] & 0x3F) ||
                        (context->viewportStates.scissorBox[3] & 0x7F))
                    {
					    if (_ClearRect(context, Mask))
					    {
						    break;
					    }
                    }
				}
                else
                if ((context->viewportStates.scissorBox[0] & 0x03) ||
                    (context->viewportStates.scissorBox[1] & 0x03) ||
                    (context->viewportStates.scissorBox[2] & 0x0F) ||
                    (context->viewportStates.scissorBox[3] & 0x03))
                {
                    if (_ClearRect(context, Mask))
                    {
                        break;
                    }
                }
            }
#endif

            if ((Mask & GL_COLOR_BUFFER_BIT) != 0)
            {
                gcoSURF draw;

                /* Determine the target surface. */
                if (context->frameBuffer == gcvNULL)
                {
                    draw = context->draw;
                }
                else
                {
                    if (glfIsFramebufferComplete(context) != GL_FRAMEBUFFER_COMPLETE_OES)
                    {
                        glmERROR(GL_INVALID_FRAMEBUFFER_OPERATION_OES);
                        break;
                    }

                    draw = glfGetFramebufferSurface(&context->frameBuffer->color);
                }

                /* Do we have a surface to draw on? */
                if (draw != gcvNULL)
                {
                    /* Clear color buffer. */
                    if (context->viewportStates.scissorTest)
                    {
                        /* Determine scissor coordinates. */
                        gctINT left
                            = context->viewportStates.scissorBox[0];

                        gctINT top
                            = context->viewportStates.scissorBox[1];

                        gctINT right
                            = left
                            + context->viewportStates.scissorBox[2];

                        gctINT bottom
                            = top
                            + context->viewportStates.scissorBox[3];

                        /* Clear the scissor area. */
                        gcmERR_BREAK(gcoSURF_ClearRect(
                            draw, left, top, right, bottom, gcvCLEAR_COLOR
                            ));
                    }
                    else
                    {
                        gcmERR_BREAK(gcoSURF_Clear(
                            draw, gcvCLEAR_COLOR
                            ));
                    }
                }
            }

            if ((Mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0)
            {
                gcoSURF depth;
                gctUINT flags = 0;

                /* Compute masks for clearing. */
                if (Mask & GL_DEPTH_BUFFER_BIT)
                {
                    flags |= gcvCLEAR_DEPTH;
                }

                if (Mask & GL_STENCIL_BUFFER_BIT)
                {
                    flags |= gcvCLEAR_STENCIL;
                }

                /* Update stencil states. */
                gcmERR_BREAK(glfUpdateStencil(context));

                /* Determine the target surface. */
                if (context->frameBuffer == gcvNULL)
                {
                    depth = context->depth;
                }
                else
                {
                    if (glfIsFramebufferComplete(context) != GL_FRAMEBUFFER_COMPLETE_OES)
                    {
                        glmERROR(GL_INVALID_FRAMEBUFFER_OPERATION_OES);
                        break;
                    }

                    depth = glfGetFramebufferSurface(&context->frameBuffer->depth);
                }

                /* Do we have a surface to draw on? */
                if (depth != gcvNULL)
                {
                    /* Clear depth buffer. */
                    if (context->viewportStates.scissorTest)
                    {
                        /* Determine scissor coordinates. */
                        gctINT left
                            = context->viewportStates.scissorBox[0];

                        gctINT top
                            = context->viewportStates.scissorBox[1];

                        gctINT right
                            = left
                            + context->viewportStates.scissorBox[2];

                        gctINT bottom
                            = top
                            + context->viewportStates.scissorBox[3];

                        /* Clear the scissor area. */
                        gcmERR_BREAK(gcoSURF_ClearRect(
                            depth, left, top, right, bottom, flags
                            ));
                    }
                    else
                    {
                        gcmERR_BREAK(gcoSURF_Clear(
                            depth, flags
                            ));
                    }
                }
            }
        }
        while (gcvFALSE);

        /* Error? */
        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }
    }
    glmLEAVE();
}
