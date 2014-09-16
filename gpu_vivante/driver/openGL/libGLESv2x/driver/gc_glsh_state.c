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

static void
_SetCulling(
    GLContext Context
    )
{
    if (Context->cullEnable)
    {
        if ((  (Context->cullMode  == GL_FRONT)
            && (Context->cullFront == GL_CCW)
            )
        ||  (   (Context->cullMode  == GL_BACK)
             && (Context->cullFront == GL_CW)
             )
        )
        {
            gcmVERIFY_OK(
                gco3D_SetCulling(Context->engine, gcvCULL_CW));
        }
        else
        {
            gcmVERIFY_OK(
                gco3D_SetCulling(Context->engine, gcvCULL_CCW));
        }
    }
    else
    {
        gcmVERIFY_OK(
            gco3D_SetCulling(Context->engine, gcvCULL_NONE));
    }
}

gceCOMPARE
_glshTranslateFunc(
    GLenum Func
    )
{
    switch (Func)
    {
    case GL_NEVER:
        return gcvCOMPARE_NEVER;

    case GL_ALWAYS:
        return gcvCOMPARE_ALWAYS;

    case GL_LESS:
        return gcvCOMPARE_LESS;

    case GL_LEQUAL:
        return gcvCOMPARE_LESS_OR_EQUAL;

    case GL_EQUAL:
        return gcvCOMPARE_EQUAL;

    case GL_GREATER:
        return gcvCOMPARE_GREATER;

    case GL_GEQUAL:
        return gcvCOMPARE_GREATER_OR_EQUAL;

    case GL_NOTEQUAL:
        return gcvCOMPARE_NOT_EQUAL;
    }

    return gcvCOMPARE_INVALID;
}

static gceSTENCIL_OPERATION
_glshTranslateOperation(
    GLenum Operation
    )
{
    switch (Operation)
    {
    case GL_KEEP:
        return gcvSTENCIL_KEEP;

    case GL_ZERO:
        return gcvSTENCIL_ZERO;

    case GL_REPLACE:
        return gcvSTENCIL_REPLACE;

    case GL_INCR:
        return gcvSTENCIL_INCREMENT_SATURATE;

    case GL_DECR:
        return gcvSTENCIL_DECREMENT_SATURATE;

    case GL_INVERT:
        return gcvSTENCIL_INVERT;

    case GL_INCR_WRAP:
        return gcvSTENCIL_INCREMENT;

    case GL_DECR_WRAP:
        return gcvSTENCIL_DECREMENT;
    }

    return gcvSTENCIL_OPERATION_INVALID;
}

static gceSTATUS
_glshTranslateBlendFunc(
    IN GLenum BlendFuncGL,
    OUT gceBLEND_FUNCTION * BlendFunc
    )
{
    switch (BlendFuncGL)
    {
    case GL_ZERO:
        *BlendFunc = gcvBLEND_ZERO;
        break;

    case GL_ONE:
        *BlendFunc = gcvBLEND_ONE;
        break;

    case GL_SRC_COLOR:
        *BlendFunc = gcvBLEND_SOURCE_COLOR;
        break;

    case GL_ONE_MINUS_SRC_COLOR:
        *BlendFunc = gcvBLEND_INV_SOURCE_COLOR;
        break;

    case GL_DST_COLOR:
        *BlendFunc = gcvBLEND_TARGET_COLOR;
        break;

    case GL_ONE_MINUS_DST_COLOR:
        *BlendFunc = gcvBLEND_INV_TARGET_COLOR;
        break;

    case GL_SRC_ALPHA:
        *BlendFunc = gcvBLEND_SOURCE_ALPHA;
        break;

    case GL_ONE_MINUS_SRC_ALPHA:
        *BlendFunc = gcvBLEND_INV_SOURCE_ALPHA;
        break;

    case GL_DST_ALPHA:
        *BlendFunc = gcvBLEND_TARGET_ALPHA;
        break;

    case GL_ONE_MINUS_DST_ALPHA:
        *BlendFunc = gcvBLEND_INV_TARGET_ALPHA;
        break;

    case GL_CONSTANT_COLOR:
        *BlendFunc = gcvBLEND_CONST_COLOR;
        break;

    case GL_ONE_MINUS_CONSTANT_COLOR:
        *BlendFunc = gcvBLEND_INV_CONST_COLOR;
        break;

    case GL_CONSTANT_ALPHA:
        *BlendFunc = gcvBLEND_CONST_ALPHA;
        break;

    case GL_ONE_MINUS_CONSTANT_ALPHA:
        *BlendFunc = gcvBLEND_INV_CONST_ALPHA;
        break;

    case GL_SRC_ALPHA_SATURATE:
        *BlendFunc = gcvBLEND_SOURCE_ALPHA_SATURATE;
        break;

    default:
        gcmTRACE(gcvLEVEL_WARNING,
                 "%s: unknown enum 0x%04X",
                 __FUNCTION__, BlendFuncGL);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_glshTranslateBlendMode(
    IN GLenum BlendModeGL,
    OUT gceBLEND_MODE * BlendMode
    )
{
    switch (BlendModeGL)
    {
    case GL_FUNC_ADD:
        *BlendMode = gcvBLEND_ADD;
        break;

    case GL_FUNC_SUBTRACT:
        *BlendMode = gcvBLEND_SUBTRACT;
        break;

    case GL_FUNC_REVERSE_SUBTRACT:
        *BlendMode = gcvBLEND_REVERSE_SUBTRACT;
        break;

    case GL_MIN_EXT:
        *BlendMode = gcvBLEND_MIN;
        break;

    case GL_MAX_EXT:
        *BlendMode = gcvBLEND_MAX;
        break;

    default:
        gcmTRACE(gcvLEVEL_WARNING,
                 "%s: unknown enum 0x%04X",
                 __FUNCTION__, BlendModeGL);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return gcvSTATUS_OK;
}

static gceSTATUS
_SetLineWidth(
    GLContext Context,
    GLfloat LineWidth
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x LineWidth=%f", Context, LineWidth);

    do
    {
        /* Validate the width. */
        if (LineWidth <= 0.0f)
        {
            gcmFOOTER_ARG("LineWidth=%f", LineWidth);
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        /* Make sure the width is within the supported range. */
        if (LineWidth < Context->lineWidthRange[0])
        {
            LineWidth = Context->lineWidthRange[0];
        }

        if (LineWidth > Context->lineWidthRange[1])
        {
            LineWidth = Context->lineWidthRange[1];
        }

        /* Set the line width for the hardware. */
        gcmONERROR(
            gco3D_SetAALineWidth(Context->engine,
                                 gcoMATH_Floor(LineWidth + 0.5f)));
    }
    while (gcvFALSE);

OnError:
    gcmFOOTER();
    return status;
}

/* Same idea as _glshInitializeState, but with a context that already has valid state. */
void
_glshInitializeObjectStates(
    GLContext Context
    )
{
    gceSTATUS status;
    gceBLEND_MODE rgbMode = gcvBLEND_ADD, aMode = gcvBLEND_ADD;
    gceBLEND_FUNCTION rgbFunc = gcvBLEND_ZERO, aFunc = gcvBLEND_ZERO;
    GLfloat depthBias;

    /* Initialize states. */
    gcmVERIFY_OK(
        gco3D_EnableBlending(Context->engine, Context->blendEnable));

    _glshTranslateBlendFunc(Context->blendFuncSourceRGB,   &rgbFunc);
    _glshTranslateBlendFunc(Context->blendFuncSourceAlpha, &aFunc);
    gcmVERIFY_OK(gco3D_SetBlendFunction(Context->engine,
                                        gcvBLEND_SOURCE,
                                        rgbFunc,
                                        aFunc));

    _glshTranslateBlendFunc(Context->blendFuncTargetRGB,   &rgbFunc);
    _glshTranslateBlendFunc(Context->blendFuncTargetAlpha, &aFunc);
    gcmVERIFY_OK(gco3D_SetBlendFunction(Context->engine,
                                        gcvBLEND_TARGET,
                                        rgbFunc,
                                        aFunc));

    _glshTranslateBlendMode(Context->blendModeRGB,   &rgbMode);
    _glshTranslateBlendMode(Context->blendModeAlpha, &aMode);
    gcmVERIFY_OK(
        gco3D_SetBlendMode(Context->engine, rgbMode, aMode));

    gcmVERIFY_OK(
        gco3D_SetBlendColorF(Context->engine,
                             Context->blendColorRed,
                             Context->blendColorGreen,
                             Context->blendColorBlue,
                             Context->blendColorAlpha));

    _SetCulling(Context);

    gcmVERIFY_OK(
        gco3D_EnableDepthWrite(Context->engine, Context->depthMask));

    Context->depthDirty  = GL_TRUE;
    Context->ditherDirty = GL_TRUE;
    Context->viewDirty   = GL_TRUE;

    gcmONERROR(
        gco3D_SetStencilReference(Context->engine,
                                  (gctUINT8) Context->stencilRefFront, gcvTRUE));
    gcmONERROR(
        gco3D_SetStencilReference(Context->engine,
                                  (gctUINT8) Context->stencilRefFront, gcvFALSE));

    gcmVERIFY_OK(
        gco3D_SetStencilCompare(Context->engine,
                                gcvSTENCIL_FRONT,
                                _glshTranslateFunc(Context->stencilFuncFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilCompare(Context->engine,
                                gcvSTENCIL_BACK,
                                _glshTranslateFunc(Context->stencilFuncBack)));
    gcmONERROR(
        gco3D_SetStencilMask(Context->engine,
                             (gctUINT8) Context->stencilMaskFront));
    gcmONERROR(
        gco3D_SetStencilWriteMask(Context->engine,
                                  (gctUINT8) Context->stencilWriteMask));
    gcmVERIFY_OK(
        gco3D_SetStencilFail(Context->engine,
                             gcvSTENCIL_FRONT,
                             Context->frontFail =
                                _glshTranslateOperation(
                                    Context->stencilOpFailFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilFail(Context->engine,
                             gcvSTENCIL_BACK,
                             Context->backFail =
                                 _glshTranslateOperation(
                                     Context->stencilOpFailBack)));
    gcmVERIFY_OK(
        gco3D_SetStencilDepthFail(Context->engine,
                                  gcvSTENCIL_FRONT,
                                  Context->frontZFail =
                                      _glshTranslateOperation(
                                          Context->stencilOpDepthFailFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilDepthFail(Context->engine,
                                  gcvSTENCIL_BACK,
                                  Context->backZFail =
                                      _glshTranslateOperation(
                                          Context->stencilOpDepthFailBack)));
    gcmVERIFY_OK(
        gco3D_SetStencilPass(Context->engine,
                             gcvSTENCIL_FRONT,
                             Context->frontZPass =
                                 _glshTranslateOperation(
                                     Context->stencilOpDepthPassFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilPass(Context->engine,
                             gcvSTENCIL_BACK,
                             Context->backZPass =
                                 _glshTranslateOperation(
                                     Context->stencilOpDepthPassBack)));

    gcmVERIFY_OK(
        gco3D_SetColorWrite(Context->engine, Context->colorWrite));

    depthBias = gcoMATH_Divide(Context->offsetUnits, 65535.f);
    gcmONERROR(
        gco3D_SetDepthScaleBiasF(Context->engine,
                                 Context->offsetFactor,
                                 depthBias));

    gcmVERIFY_OK(
        _SetLineWidth(Context, Context->lineWidth));

OnError:
    return;
}

void
_glshInitializeState(
    GLContext Context
    )
{
    gceSTATUS status;
    gceBLEND_MODE rgbMode = gcvBLEND_ADD, aMode;
    gceBLEND_FUNCTION rgbFunc = gcvBLEND_ZERO, aFunc = gcvBLEND_ZERO;

    gcmHEADER_ARG("Context=0x%x", Context);

    /* Initialize states. */
    gcmVERIFY_OK(
        gco3D_SetLastPixelEnable(Context->engine, gcvFALSE));

    Context->blendEnable = GL_FALSE;
    gcmVERIFY_OK(
        gco3D_EnableBlending(Context->engine, Context->blendEnable));

    Context->blendFuncSourceRGB   = GL_ONE;
    Context->blendFuncSourceAlpha = GL_ONE;
    _glshTranslateBlendFunc(Context->blendFuncSourceRGB,   &rgbFunc);
    _glshTranslateBlendFunc(Context->blendFuncSourceAlpha, &aFunc);
    gcmVERIFY_OK(gco3D_SetBlendFunction(Context->engine,
                                        gcvBLEND_SOURCE,
                                        rgbFunc,
                                        aFunc));

    Context->blendFuncTargetRGB   = GL_ZERO;
    Context->blendFuncTargetAlpha = GL_ZERO;
    _glshTranslateBlendFunc(Context->blendFuncTargetRGB,   &rgbFunc);
    _glshTranslateBlendFunc(Context->blendFuncTargetAlpha, &aFunc);
    gcmVERIFY_OK(gco3D_SetBlendFunction(Context->engine,
                                        gcvBLEND_TARGET,
                                        rgbFunc,
                                        aFunc));

    Context->blendModeRGB   = GL_FUNC_ADD;
    Context->blendModeAlpha = GL_FUNC_ADD;
    _glshTranslateBlendMode(Context->blendModeRGB,   &rgbMode);
    _glshTranslateBlendMode(Context->blendModeAlpha, &aMode);
    gcmVERIFY_OK(
        gco3D_SetBlendMode(Context->engine, rgbMode, aMode));

    Context->blendColorRed   = 0.0f;
    Context->blendColorGreen = 0.0f;
    Context->blendColorBlue  = 0.0f;
    Context->blendColorAlpha = 0.0f;
    gcmVERIFY_OK(
        gco3D_SetBlendColorF(Context->engine,
                             Context->blendColorRed,
                             Context->blendColorGreen,
                             Context->blendColorBlue,
                             Context->blendColorAlpha));

    gcmVERIFY_OK(
        gco3D_SetAlphaTest(Context->engine, gcvFALSE));

    Context->cullEnable = GL_FALSE;
    Context->cullMode   = GL_BACK;
    Context->cullFront  = GL_CCW;
    _SetCulling(Context);

    Context->depthMask = GL_TRUE;
    gcmVERIFY_OK(
        gco3D_EnableDepthWrite(Context->engine, Context->depthMask));

    Context->depthTest  = GL_FALSE;
    Context->depthFunc  = GL_LESS;
    Context->depthNear  = 0.0f;
    Context->depthFar   = 1.0f;
    Context->depthDirty = GL_TRUE;

    Context->ditherEnable = GL_TRUE;
    Context->ditherDirty  = GL_TRUE;

    Context->viewportX      = 0;
    Context->viewportY      = 0;
    Context->viewportWidth  = Context->drawWidth;
    Context->viewportHeight = Context->drawHeight;
    Context->scissorEnable  = GL_FALSE;
    Context->scissorX       = 0;
    Context->scissorY       = 0;
    Context->scissorWidth   = Context->drawWidth;
    Context->scissorHeight  = Context->drawHeight;
    Context->viewDirty      = GL_TRUE;

    Context->stencilEnable           = GL_FALSE;
    Context->stencilRefFront         = 0;
    Context->stencilRefBack          = 0;
    Context->stencilFuncFront        = GL_ALWAYS;
    Context->stencilFuncBack         = GL_ALWAYS;
    Context->stencilMaskFront        = ~0U;
    Context->stencilMaskBack         = ~0U;
    Context->stencilWriteMask        = ~0U;
    Context->stencilOpFailFront      = GL_KEEP;
    Context->stencilOpFailBack       = GL_KEEP;
    Context->stencilOpDepthFailFront = GL_KEEP;
    Context->stencilOpDepthFailBack  = GL_KEEP;
    Context->stencilOpDepthPassFront = GL_KEEP;
    Context->stencilOpDepthPassBack  = GL_KEEP;
    gcmONERROR(
        gco3D_SetStencilReference(Context->engine,
                                  (gctUINT8) Context->stencilRefFront, gcvTRUE));
    gcmONERROR(
        gco3D_SetStencilReference(Context->engine,
                                  (gctUINT8) Context->stencilRefFront, gcvFALSE));
    gcmVERIFY_OK(
        gco3D_SetStencilCompare(Context->engine,
                                gcvSTENCIL_FRONT,
                                _glshTranslateFunc(Context->stencilFuncFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilCompare(Context->engine,
                                gcvSTENCIL_BACK,
                                _glshTranslateFunc(Context->stencilFuncBack)));
    gcmONERROR(
        gco3D_SetStencilMask(Context->engine,
                             (gctUINT8) Context->stencilMaskFront));
    gcmONERROR(
        gco3D_SetStencilWriteMask(Context->engine,
                                  (gctUINT8) Context->stencilWriteMask));
    gcmVERIFY_OK(
        gco3D_SetStencilFail(Context->engine,
                             gcvSTENCIL_FRONT,
                             Context->frontFail =
                                _glshTranslateOperation(
                                    Context->stencilOpFailFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilFail(Context->engine,
                             gcvSTENCIL_BACK,
                             Context->backFail =
                                 _glshTranslateOperation(
                                     Context->stencilOpFailBack)));
    gcmVERIFY_OK(
        gco3D_SetStencilDepthFail(Context->engine,
                                  gcvSTENCIL_FRONT,
                                  Context->frontZFail =
                                      _glshTranslateOperation(
                                          Context->stencilOpDepthFailFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilDepthFail(Context->engine,
                                  gcvSTENCIL_BACK,
                                  Context->backZFail =
                                      _glshTranslateOperation(
                                          Context->stencilOpDepthFailBack)));
    gcmVERIFY_OK(
        gco3D_SetStencilPass(Context->engine,
                             gcvSTENCIL_FRONT,
                             Context->frontZPass =
                                 _glshTranslateOperation(
                                     Context->stencilOpDepthPassFront)));
    gcmVERIFY_OK(
        gco3D_SetStencilPass(Context->engine,
                             gcvSTENCIL_BACK,
                             Context->backZPass =
                                 _glshTranslateOperation(
                                     Context->stencilOpDepthPassBack)));

    Context->colorEnableRed   = GL_TRUE;
    Context->colorEnableGreen = GL_TRUE;
    Context->colorEnableBlue  = GL_TRUE;
    Context->colorEnableAlpha = GL_TRUE;
    Context->colorWrite       = 0xF;
    gcmVERIFY_OK(
        gco3D_SetColorWrite(Context->engine, Context->colorWrite));

    Context->offsetEnable = GL_FALSE;
    Context->offsetFactor = 0.0f;
    Context->offsetUnits  = 0.0f;
    gcmONERROR(
        gco3D_SetDepthScaleBiasF(Context->engine,
                                 Context->offsetFactor,
                                 Context->offsetUnits));

    Context->sampleAlphaToCoverage = GL_FALSE;
    Context->sampleCoverage        = GL_FALSE;
    Context->sampleCoverageValue   = 1.0f;
    Context->sampleCoverageInvert  = GL_FALSE;

    /* Initialize alignment. */
    Context->packAlignment = Context->unpackAlignment = 4;

    /* Initialize PA. */
    gcmVERIFY_OK(
        gco3D_SetFill(Context->engine, gcvFILL_SOLID));
    gcmVERIFY_OK(
        gco3D_SetShading(Context->engine, gcvSHADING_SMOOTH));

    /* Initialize line width. */
    Context->lineWidth = 1.0f;

    Context->lineWidthRange[0] = 1.0f;
    Context->lineWidthRange[1] = 1.0f;

    if (gcoHAL_IsFeatureAvailable(Context->hal,
                                  gcvFEATURE_WIDE_LINE) == gcvSTATUS_TRUE)
    {
        /* Wide lines are supported. */
        Context->lineWidthRange[1] = 8192.0f;

    }

    /* Set the hardware states. */
    gcmVERIFY_OK(
        gco3D_SetAntiAliasLine(Context->engine, gcvTRUE));

    gcmVERIFY_OK(
        _SetLineWidth(Context, Context->lineWidth));

    /* Initialize hint. */
    Context->mipmapHint = GL_DONT_CARE;
    Context->fragShaderDerivativeHint = GL_DONT_CARE;

    /* Initialize flush state. */
    Context->textureFlushNeeded = gcvFALSE;

    gcmFOOTER_NO();
    return;

OnError:
    gcmFOOTER();
    return;
}

void
glshProgramDither(
    IN GLContext Context
    )
{
    GLboolean   dither;

    /* Test if dither is dirty. */
    if (Context->ditherDirty)
    {
        /* Get dither value. */
        dither = Context->ditherEnable;

        /* Test if alpha blending is enabled. */
        if (Context->blendEnable)
        {
            /* Disable dither when alpha blending is enabled. */
            dither = GL_FALSE;
        }

        /* Send dither state to HAL. */
        gco3D_EnableDither(Context->engine, dither);

        /* Dither has been programmed. */
        Context->ditherDirty = GL_FALSE;
    }
}

GL_APICALL void GL_APIENTRY
glViewport(
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("x=%d y=%d width=%ld height=%ld", x, y, width, height);
    gcmDUMP_API("${ES20 glViewport 0x%08X 0x%08X 0x%08X 0x%08X}",
                x, y, width, height);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_VIEWPORT, 0);

    if ((width < 0) || (height < 0))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Negative size %dx%d",
                      __FUNCTION__, __LINE__, width, height);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if ((context->viewportX != x)
     || (context->viewportY != y)
     || (context->viewportWidth != (GLuint)width)
     || (context->viewportHeight != (GLuint)height))
    {
        context->viewportX      = x;
        context->viewportY      = y;
        context->viewportWidth  = width;
        context->viewportHeight = height;

        context->viewDirty = GL_TRUE;

        /* Disable batch optmization. */
        context->batchDirty = GL_TRUE;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glPixelStorei(
    GLenum pname,
    GLint param
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("pname=0x%04x param=%d", pname, param);
    gcmDUMP_API("${ES20 glPixelStorei 0x%08X 0x%08X}", pname, param);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_PIXELSTOREI, 0);

    switch (pname)
    {
    case GL_UNPACK_ALIGNMENT:
        context->unpackAlignment = param;
        break;

    case GL_PACK_ALIGNMENT:
        context->packAlignment = param;
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown pname=0x%04x",
                      __FUNCTION__, __LINE__, pname);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glDisable(
    GLenum cap
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("cap=0x%04x", cap);
    gcmDUMP_API("${ES20 glDisable 0x%08X}", cap);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_DISABLE, 0);

    switch (cap)
    {
    case GL_BLEND:
        context->blendEnable = GL_FALSE;
        context->ditherDirty = GL_TRUE;
        if (gcmIS_ERROR(gco3D_EnableBlending(context->engine, gcvFALSE)))
		{
			gl2mERROR(GL_INVALID_OPERATION);
			gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		}
        break;

    case GL_CULL_FACE:
        context->cullEnable = GL_FALSE;
        if (gcmIS_ERROR(gco3D_SetCulling(context->engine, gcvCULL_NONE)))
		{
			gl2mERROR(GL_INVALID_OPERATION);
			gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		}
        break;

    case GL_DEPTH_TEST:
        context->depthTest  = GL_FALSE;
        context->depthDirty = GL_TRUE;
        break;

    case GL_DITHER:
        context->ditherEnable = GL_FALSE;
        context->ditherDirty  = GL_TRUE;
        break;

    case GL_STENCIL_TEST:
        context->stencilEnable = GL_FALSE;
        context->depthDirty    = GL_TRUE;
        break;

    case GL_SCISSOR_TEST:
        context->scissorEnable = GL_FALSE;
        context->viewDirty     = GL_TRUE;
        break;

    case GL_POLYGON_OFFSET_FILL:
        context->offsetEnable = GL_FALSE;
        if (gcmIS_ERROR(gco3D_SetDepthScaleBiasF(context->engine, 0.0f, 0.0f)))
		{
			gl2mERROR(GL_INVALID_OPERATION);
			gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		}
        break;

    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        context->sampleAlphaToCoverage = GL_FALSE;
        break;

    case GL_SAMPLE_COVERAGE:
        context->sampleCoverage = GL_FALSE;
        break;

    case GL_TIMESTAMP_VIV0:
    case GL_TIMESTAMP_VIV1:
    case GL_TIMESTAMP_VIV2:
    case GL_TIMESTAMP_VIV3:
    case GL_TIMESTAMP_VIV4:
    case GL_TIMESTAMP_VIV5:
    case GL_TIMESTAMP_VIV6:
    case GL_TIMESTAMP_VIV7:
        if (gcmIS_ERROR(
            gcoHAL_SetTimer(context->hal,
                            cap - GL_TIMESTAMP_VIV0,
                            gcvFALSE)))
		{
			gl2mERROR(GL_INVALID_OPERATION);
			gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		}
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown cap=0x%04x",
                      __FUNCTION__, __LINE__, cap);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glEnable(
    GLenum cap
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLfloat depthBias = 0.f;

    gcmHEADER_ARG("cap=0x%04x", cap);
    gcmDUMP_API("${ES20 glEnable 0x%08X}", cap);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_ENABLE, 0);

    switch (cap)
    {
    case GL_BLEND:
        context->blendEnable = GL_TRUE;
        context->ditherDirty = GL_TRUE;
        gcmVERIFY_OK(gco3D_EnableBlending(context->engine, gcvTRUE));
        break;

    case GL_CULL_FACE:
        context->cullEnable = GL_TRUE;
        _SetCulling(context);
        break;

    case GL_DEPTH_TEST:
        context->depthTest  = GL_TRUE;
        context->depthDirty = GL_TRUE;
        break;

    case GL_DITHER:
        context->ditherEnable = GL_TRUE;
        context->ditherDirty  = GL_TRUE;
        break;

    case GL_SCISSOR_TEST:
        context->scissorEnable = GL_TRUE;
        context->viewDirty     = GL_TRUE;
        break;

    case GL_STENCIL_TEST:
        context->stencilEnable = GL_TRUE;
        context->depthDirty    = GL_TRUE;
        break;

    case GL_POLYGON_OFFSET_FILL:
        context->offsetEnable = GL_TRUE;
        depthBias = gcoMATH_Divide(context->offsetUnits, 65535.f);

        if (gcmIS_ERROR(
            gco3D_SetDepthScaleBiasF(context->engine,
                                     context->offsetFactor,
                                     depthBias)))
		{
			gl2mERROR(GL_INVALID_OPERATION);
			gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		}
        break;

    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        context->sampleAlphaToCoverage = GL_TRUE;
        break;

    case GL_SAMPLE_COVERAGE:
        context->sampleCoverage = GL_TRUE;
        break;

    case GL_TIMESTAMP_VIV0:
    case GL_TIMESTAMP_VIV1:
    case GL_TIMESTAMP_VIV2:
    case GL_TIMESTAMP_VIV3:
    case GL_TIMESTAMP_VIV4:
    case GL_TIMESTAMP_VIV5:
    case GL_TIMESTAMP_VIV6:
    case GL_TIMESTAMP_VIV7:
        if (gcmIS_ERROR(
            gcoHAL_SetTimer(context->hal,
                            cap - GL_TIMESTAMP_VIV0,
                            gcvTRUE)))
		{
			gl2mERROR(GL_INVALID_OPERATION);
			gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		}
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown cap=0x%04x",
                      __FUNCTION__, __LINE__, cap);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glBlendFunc(
    GLenum sfactor,
    GLenum dfactor
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceBLEND_FUNCTION blendSource, blendTarget;
	gceSTATUS status;

    gcmHEADER_ARG("sfactor=0x%04x dfactor=0x%04x", sfactor, dfactor);
    gcmDUMP_API("${ES20 glBlendFunc 0x%08X 0x%08X}", sfactor, dfactor);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_BLENDFUNC, 0);

    if (gcmIS_ERROR(_glshTranslateBlendFunc(sfactor, &blendSource)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown sfactor=0x%04x",
                      __FUNCTION__, __LINE__, sfactor);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if (gcmIS_ERROR(_glshTranslateBlendFunc(dfactor, &blendTarget)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown dfactor=0x%04x",
                      __FUNCTION__, __LINE__, dfactor);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    context->blendFuncSourceRGB   = sfactor;
    context->blendFuncSourceAlpha = sfactor;
    context->blendFuncTargetRGB   = dfactor;
    context->blendFuncTargetAlpha = dfactor;

    gcmONERROR(
        gco3D_SetBlendFunction(context->engine,
                               gcvBLEND_SOURCE,
                               blendSource, blendSource));

    gcmONERROR(
        gco3D_SetBlendFunction(context->engine,
                               gcvBLEND_TARGET,
                               blendTarget, blendTarget));

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
	return;
OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

GL_APICALL void GL_APIENTRY
glBlendFuncSeparate(
    GLenum srcRGB,
    GLenum dstRGB,
    GLenum srcAlpha,
    GLenum dstAlpha
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceBLEND_FUNCTION blendSourceRGB, blendSourceAlpha;
    gceBLEND_FUNCTION blendTargetRGB, blendTargetAlpha;
	gceSTATUS status;

    gcmHEADER_ARG("srcRGB=0x%04x dstRGB=0x%04x srcAlpha=0x%04x dstAlpha=0x%04x",
                  srcRGB, dstRGB, srcAlpha, dstAlpha);
    gcmDUMP_API("${ES20 glBlendFuncSeparate 0x%08X 0x%08X 0x%08X 0x%08X}",
                srcRGB, dstRGB, srcAlpha, dstAlpha);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_BLENDFUNCSEPARATE, 0);

    if (gcmIS_ERROR(_glshTranslateBlendFunc(srcRGB, &blendSourceRGB)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown srcRGB=0x%04x",
                      __FUNCTION__, __LINE__, srcRGB);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if (gcmIS_ERROR(_glshTranslateBlendFunc(dstRGB, &blendTargetRGB)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown dstRGB=0x%04x",
                      __FUNCTION__, __LINE__, dstRGB);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if (gcmIS_ERROR(_glshTranslateBlendFunc(srcAlpha, &blendSourceAlpha)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown srcAlpha=0x%04x",
                      __FUNCTION__, __LINE__, srcAlpha);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if (gcmIS_ERROR(_glshTranslateBlendFunc(dstAlpha, &blendTargetAlpha)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown dstAlpha=0x%04x",
                      __FUNCTION__, __LINE__, dstAlpha);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    context->blendFuncSourceRGB   = srcRGB;
    context->blendFuncSourceAlpha = srcAlpha;
    context->blendFuncTargetRGB   = dstRGB;
    context->blendFuncTargetAlpha = dstAlpha;

    gcmONERROR(
        gco3D_SetBlendFunction(context->engine,
                               gcvBLEND_SOURCE,
                               blendSourceRGB, blendSourceAlpha));

    gcmONERROR(
        gco3D_SetBlendFunction(context->engine,
                               gcvBLEND_TARGET,
                               blendTargetRGB, blendTargetAlpha));

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
	return;
OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

GL_APICALL void GL_APIENTRY
glBlendColor(
    GLclampf red,
    GLclampf green,
    GLclampf blue,
    GLclampf alpha
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("red=%f green=%f blue=%f alpha=%f", red, green, blue, alpha);
    gcmDUMP_API("${ES20 glBlendColor 0x%08X 0x%08X 0x%08X 0x%08X}",
                *(GLuint*) &red, *(GLuint*) &green, *(GLuint*) &blue,
                *(GLuint*) &alpha);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_BLENDCOLOR, 0);

    context->blendColorRed   = gcmMAX(0.0f, gcmMIN(1.0f, red));
    context->blendColorGreen = gcmMAX(0.0f, gcmMIN(1.0f, green));
    context->blendColorBlue  = gcmMAX(0.0f, gcmMIN(1.0f, blue));
    context->blendColorAlpha = gcmMAX(0.0f, gcmMIN(1.0f, alpha));

    if (gcmIS_ERROR(
        gco3D_SetBlendColorF(context->engine,
                             context->blendColorRed,
                             context->blendColorGreen,
                             context->blendColorBlue,
                             context->blendColorAlpha)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		return;
	}

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glBlendEquation(
    GLenum mode
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceBLEND_MODE blendMode;

    gcmHEADER_ARG("mode=0x%04x", mode);
    gcmDUMP_API("${ES20 glBlendEquation 0x%08X}", mode);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_BLENDEQUATION, 0);

    if (gcmIS_ERROR(_glshTranslateBlendMode(mode, &blendMode)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown mode=0x%04x",
                      __FUNCTION__, __LINE__, mode);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    context->blendModeRGB   = mode;
    context->blendModeAlpha = mode;

    if (gcmIS_ERROR(
        gco3D_SetBlendMode(context->engine, blendMode, blendMode)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		return;
	}

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glBlendEquationSeparate(
    GLenum modeRGB,
    GLenum modeAlpha
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceBLEND_MODE blendModeRGB, blendModeAlpha;

    gcmHEADER_ARG("modeRGB=%d modeAlpha=%d", modeRGB, modeAlpha);
    gcmDUMP_API("${ES20 glBlendEquationSeparate 0x%08X 0x%08X}",
                modeRGB, modeAlpha);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_BLENDEQUATIONSEPARATE, 0);

    if (gcmIS_ERROR(_glshTranslateBlendMode(modeRGB, &blendModeRGB)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown modeRGB=0x%04x",
                      __FUNCTION__, __LINE__, modeRGB);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if (gcmIS_ERROR(_glshTranslateBlendMode(modeAlpha, &blendModeAlpha)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown modeAlpha=0x%04x",
                      __FUNCTION__, __LINE__, modeAlpha);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    context->blendModeRGB   = modeRGB;
    context->blendModeAlpha = modeAlpha;

    if (gcmIS_ERROR(
        gco3D_SetBlendMode(context->engine, blendModeRGB, blendModeAlpha)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		return;
	}

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glDepthFunc(
    GLenum func
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceCOMPARE compare;

    gcmHEADER_ARG("func=0x%04x", func);
    gcmDUMP_API("${ES20 glDepthFunc 0x%08X}", func);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_DEPTHFUNC, 0);

    compare = _glshTranslateFunc(func);
    if (compare == gcvCOMPARE_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown func=0x%04x",
                      __FUNCTION__, __LINE__, func);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    context->depthFunc  = func;
    context->depthDirty = GL_TRUE;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glDepthMask(
    GLboolean flag
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("flag=%d", flag);
    gcmDUMP_API("${ES20 glDepthMask 0x%08X}", flag);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_DEPTHMASK, 0);

    context->depthMask = flag;

    if (gcmIS_ERROR(
        gco3D_EnableDepthWrite(context->engine, flag)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		return;
	}

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glDepthRangef(
    GLclampf zNear,
    GLclampf zFar
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("zNear=%f zFar=%f", zNear, zFar);
    gcmDUMP_API("${ES20 glDepthRangef 0x%08X 0x%08X}",
                *(GLuint*) &zNear, *(GLuint*) &zFar);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_DEPTHRANGEF, 0);

    context->depthNear  = gcmMAX(0.0f, gcmMIN(1.0f, zNear));
    context->depthFar   = gcmMAX(0.0f, gcmMIN(1.0f, zFar));
    context->depthDirty = GL_TRUE;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glFrontFace(
    GLenum mode
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("mode=0x%04x", mode);
    gcmDUMP_API("${ES20 glFrontFace 0x%08X}", mode);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_FRONTFACE, 0);

    context->cullFront = mode;

    if (context->cullEnable)
    {
        _SetCulling(context);
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glStencilFunc(
    GLenum func,
    GLint ref,
    GLuint mask
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceCOMPARE compare;
	gceSTATUS status;

    gcmHEADER_ARG("func=0x%04x ref=%d mask=0x%x", func, ref, mask);
    gcmDUMP_API("${ES20 glStencilFunc 0x%08X 0x%08X 0x%08X}",
                func, ref, mask);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_STENCILFUNC, 0);

    compare = _glshTranslateFunc(func);
    if (compare == gcvCOMPARE_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown func=0x%04x",
                      __FUNCTION__, __LINE__, func);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    context->stencilFuncFront = func;
    context->stencilFuncBack = func;
    context->stencilRefFront  = ref;
    context->stencilMaskFront = mask;
    context->stencilRefBack  = ref;
    context->stencilMaskBack = mask;

    gcmONERROR(
        gco3D_SetStencilCompare(context->engine, gcvSTENCIL_FRONT, compare));
    gcmONERROR(
        gco3D_SetStencilCompare(context->engine, gcvSTENCIL_BACK, compare));
    gcmONERROR(
        gco3D_SetStencilReference(context->engine, (gctUINT8) ref, gcvTRUE));
    gcmONERROR(
        gco3D_SetStencilReference(context->engine, (gctUINT8) ref, gcvFALSE));
    gcmONERROR(
        gco3D_SetStencilMask(context->engine, (gctUINT8) mask));

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	gcmFOOTER_NO();
	return;

OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

GL_APICALL void GL_APIENTRY
glStencilFuncSeparate(
    GLenum face,
    GLenum func,
    GLint ref,
    GLuint mask
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceCOMPARE compare;
	gceSTATUS status;

    gcmHEADER_ARG("face=0x%04x func=0x%04x ref=%d mask=0x%x",
                  face, func, ref, mask);
    gcmDUMP_API("${ES20 glStencilFuncSeparate 0x%08X 0x%08X 0x%08X 0x%08X}",
                face, func, ref, mask);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_STENCILFUNCSEPARATE, 0);

    compare = _glshTranslateFunc(func);
    if (compare == gcvCOMPARE_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown func=0x%04x",
                      __FUNCTION__, __LINE__, func);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if ((face == GL_FRONT) || (face == GL_FRONT_AND_BACK))
    {
        context->stencilFuncFront = func;
        context->stencilRefFront  = ref;
        context->stencilMaskFront = mask;
        context->stencilWriteMask = mask;

        gcmONERROR(
            gco3D_SetStencilCompare(context->engine,
                                    (context->cullFront == GL_CW)
                                    ? gcvSTENCIL_FRONT
                                    : gcvSTENCIL_BACK,
                                    compare));
        gcmONERROR(
            gco3D_SetStencilReference(context->engine, (gctUINT8) ref, (context->cullFront == GL_CW)||(face == GL_FRONT_AND_BACK)));
        gcmONERROR(
            gco3D_SetStencilMask(context->engine, (gctUINT8) mask));
        gcmONERROR(
            gco3D_SetStencilWriteMask(context->engine, (gctUINT8) mask));
    }

    if ((face == GL_BACK) || (face == GL_FRONT_AND_BACK))
    {
        context->stencilFuncBack  = func;
        context->stencilRefBack   = ref;
        context->stencilMaskBack  = mask;
        context->stencilWriteMask = mask;

        gcmONERROR(
            gco3D_SetStencilCompare(context->engine,
                                    (context->cullFront == GL_CW)
                                    ? gcvSTENCIL_BACK
                                    : gcvSTENCIL_FRONT,
                                    compare));
        gcmONERROR(
            gco3D_SetStencilReference(context->engine, (gctUINT8) ref, (context->cullFront != GL_CW)||(face == GL_FRONT_AND_BACK)));
        gcmONERROR(
            gco3D_SetStencilMask(context->engine, (gctUINT8) mask));
        gcmONERROR(
            gco3D_SetStencilWriteMask(context->engine, (gctUINT8) mask));
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
	return;
OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

GL_APICALL void GL_APIENTRY
glStencilMask(
    GLuint mask
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("mask=0x%x", mask);
    gcmDUMP_API("${ES20 glStencilMask 0x%08X}", mask);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_STENCILMASK, 0);

    context->stencilMaskFront = mask;
    context->stencilWriteMask = mask;

    if (gcmIS_ERROR(
        gco3D_SetStencilWriteMask(context->engine, (gctUINT8) mask)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		return;
	}

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glStencilMaskSeparate(
    GLenum face,
    GLuint mask
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
	gceSTATUS status;

    gcmHEADER_ARG("face=0x%04x mask=0x%x", face, mask);
    gcmDUMP_API("${ES20 glStencilMaskSeparate 0x%08X 0x%08X}", face, mask);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_STENCILMASKSEPARATE, 0);

    if ((face == GL_FRONT) || (face == GL_FRONT_AND_BACK))
    {
        context->stencilMaskFront = mask;
        context->stencilWriteMask = mask;

        gcmONERROR(
            gco3D_SetStencilMask(context->engine, (gctUINT8) mask));
        gcmONERROR(
            gco3D_SetStencilWriteMask(context->engine, (gctUINT8) mask));
    }

    if ((face == GL_BACK) || (face == GL_FRONT_AND_BACK))
    {
        context->stencilMaskBack  = mask;
        context->stencilWriteMask = mask;

        gcmONERROR(
            gco3D_SetStencilMask(context->engine, (gctUINT8) mask));
        gcmONERROR(
            gco3D_SetStencilWriteMask(context->engine, (gctUINT8) mask));
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
	return;

OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

GL_APICALL void GL_APIENTRY
glStencilOp(
    GLenum fail,
    GLenum zfail,
    GLenum zpass
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceSTENCIL_OPERATION opFail, opDepthFail, opDepthPass;
	gceSTATUS status;

    gcmHEADER_ARG("fail=0x%04x zfail=0x%04x zpass=0x%04x", fail, zfail, zpass);
    gcmDUMP_API("${ES20 glStencilOp 0x%08X 0x%08X 0x%08X}", fail, zfail, zpass);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_STENCILOP, 0);

    opFail = _glshTranslateOperation(fail);
    if (opFail == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown fail=0x%04x",
                      __FUNCTION__, __LINE__, fail);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    opDepthFail = _glshTranslateOperation(zfail);
    if (opDepthFail == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown zfail=0x%04x",
                      __FUNCTION__, __LINE__, zfail);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    opDepthPass = _glshTranslateOperation(zpass);
    if (opDepthPass == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown zpass=0x%04x",
                      __FUNCTION__, __LINE__, zpass);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    context->stencilOpFailFront      = fail;
    context->stencilOpDepthFailFront = zfail;
    context->stencilOpDepthPassFront = zpass;

    gcmONERROR(
        gco3D_SetStencilFail(context->engine,
                             gcvSTENCIL_FRONT,
                             context->frontFail = opFail));
    gcmONERROR(
        gco3D_SetStencilFail(context->engine,
                             gcvSTENCIL_BACK,
                             context->backFail = opFail));
    gcmONERROR(
        gco3D_SetStencilDepthFail(context->engine,
                                  gcvSTENCIL_FRONT,
                                  context->frontZFail = opDepthFail));
    gcmONERROR(
        gco3D_SetStencilDepthFail(context->engine,
                                  gcvSTENCIL_BACK,
                                  context->backZFail = opDepthFail));
    gcmONERROR(
        gco3D_SetStencilPass(context->engine,
                             gcvSTENCIL_FRONT,
                             context->frontZPass = opDepthPass));
    gcmONERROR(
        gco3D_SetStencilPass(context->engine,
                             gcvSTENCIL_BACK,
                             context->backZPass = opDepthPass));

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
	return;

OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

GL_APICALL void GL_APIENTRY
glStencilOpSeparate(
    GLenum face,
    GLenum fail,
    GLenum zfail,
    GLenum zpass
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceSTENCIL_OPERATION opFail, opDepthFail, opDepthPass;
	gceSTATUS status;

    gcmHEADER_ARG("face=0x%04x fail=0x%04x zfail=0x%04x zpass=0x%04x",
                  face, fail, zfail, zpass);
    gcmDUMP_API("${ES20 glStencilOpSeparate 0x%08X 0x%08X 0x%08X 0x%08X}",
                face, fail, zfail, zpass);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_STENCILOPSEPARATE, 0);

    opFail = _glshTranslateOperation(fail);
    if (opFail == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown fail=0x%04x",
                      __FUNCTION__, __LINE__, fail);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    opDepthFail = _glshTranslateOperation(zfail);
    if (opDepthFail == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown zfail=0x%04x",
                      __FUNCTION__, __LINE__, zfail);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    opDepthPass = _glshTranslateOperation(zpass);
    if (opDepthPass == gcvSTENCIL_OPERATION_INVALID)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown zpass=0x%04x",
                      __FUNCTION__, __LINE__, zpass);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if ((face == GL_FRONT) || (face == GL_FRONT_AND_BACK))
    {
        context->stencilOpFailFront      = fail;
        context->stencilOpDepthFailFront = zfail;
        context->stencilOpDepthPassFront = zpass;

        gcmONERROR(
            gco3D_SetStencilFail(context->engine,
                                 context->cullFront == GL_CW ? gcvSTENCIL_FRONT : gcvSTENCIL_BACK,
                                 context->frontFail = opFail));
        gcmONERROR(
            gco3D_SetStencilDepthFail(context->engine,
                                      context->cullFront == GL_CW ? gcvSTENCIL_FRONT : gcvSTENCIL_BACK,
                                      context->frontZFail = opDepthFail));
        gcmONERROR(
            gco3D_SetStencilPass(context->engine,
                                 context->cullFront == GL_CW ? gcvSTENCIL_FRONT : gcvSTENCIL_BACK,
                                 context->frontZPass = opDepthPass));
    }

    if ((face == GL_BACK) || (face == GL_FRONT_AND_BACK))
    {
        context->stencilOpFailBack      = fail;
        context->stencilOpDepthFailBack = zfail;
        context->stencilOpDepthPassBack = zpass;

        gcmONERROR(
            gco3D_SetStencilFail(context->engine,
                                 context->cullFront == GL_CW ? gcvSTENCIL_BACK : gcvSTENCIL_FRONT,
                                 context->backFail = opFail));
        gcmONERROR(
            gco3D_SetStencilDepthFail(context->engine,
                                      context->cullFront == GL_CW ? gcvSTENCIL_BACK : gcvSTENCIL_FRONT,
                                      context->backZFail = opDepthFail));
        gcmONERROR(
            gco3D_SetStencilPass(context->engine,
                                 context->cullFront == GL_CW ? gcvSTENCIL_BACK : gcvSTENCIL_FRONT,
                                 context->backZPass = opDepthPass));
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
	return;
OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

GL_APICALL void GL_APIENTRY
glScissor(
    GLint x,
    GLint y,
    GLsizei width,
    GLsizei height
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("x=%d y=%d width=%ld height=%ld", x, y, width, height);
    gcmDUMP_API("${ES20 glScissor 0x%08X 0x%08X 0x%08X 0x%08X}",
                x, y, width, height);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_SCISSOR, 0);

    /* Save scissor box. */
    context->scissorX      = x;
    context->scissorY      = y;
    context->scissorWidth  = width;
    context->scissorHeight = height;

    context->viewDirty = GL_TRUE;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glColorMask(
    GLboolean red,
    GLboolean green,
    GLboolean blue,
    GLboolean alpha
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("red=%d green=%d blue=%d alpha=%d", red, green, blue, alpha);
    gcmDUMP_API("${ES20 glColorMask 0x%08X 0x%08X 0x%08X 0x%08X}",
                red, green, blue, alpha);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_COLORMASK, 0);

    context->colorEnableRed   = red;
    context->colorEnableGreen = green;
    context->colorEnableBlue  = blue;
    context->colorEnableAlpha = alpha;

    context->colorWrite = (red   ? 0x1 : 0x0)
                        | (green ? 0x2 : 0x0)
                        | (blue  ? 0x4 : 0x0)
                        | (alpha ? 0x8 : 0x0);

    if (gcmIS_ERROR(
        gco3D_SetColorWrite(context->engine, context->colorWrite)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		return;
	}

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glCullFace(
    GLenum mode
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("mode=0x%04x", mode);
    gcmDUMP_API("${ES20 glCullFace 0x%08X}", mode);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_CULLFACE, 0);

    switch (mode)
    {
    case GL_FRONT:
    case GL_BACK:
    case GL_FRONT_AND_BACK:
        context->cullMode = mode;

        if (context->cullEnable)
        {
            _SetCulling(context);
        }
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown mode=0x%04x",
                      __FUNCTION__, __LINE__, mode);

        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glPolygonOffset(
    GLfloat factor,
    GLfloat units
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("factor=%f units=%f", factor, units);
    gcmDUMP_API("${ES20 glPolygonOffset 0x%08X 0x%08X}",
                *(GLuint*) &factor, *(GLuint*) &units);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_POLYGONOFFSET, 0);

    /* Save state values. */
    context->offsetFactor = factor;
    context->offsetUnits  = units;

    if (context->offsetEnable)
    {
        units = gcoMATH_Divide(units, 65535.f);

        /* Set depth offset in hardware. */
        if (gcmIS_ERROR(
            gco3D_SetDepthScaleBiasF(context->engine, factor, units)))
		{
			gl2mERROR(GL_INVALID_OPERATION);
			gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
            return;
		}
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glSampleCoverage(
    GLclampf value,
    GLboolean invert
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("value=%f invert=%d", value, invert);
    gcmDUMP_API("${ES20 glSampleCoverage 0x%08X 0x%08X}",
                *(GLuint*) &value, invert);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_SAMPLECOVERAGE, 0);

    context->sampleCoverageValue  = value;
    context->sampleCoverageInvert = invert;

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glLineWidth(
    GLfloat width
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceSTATUS status;

    gcmHEADER_ARG("width=%f", width);
    gcmDUMP_API("${ES20 glLineWidth 0x%08X}", *(GLuint*) &width);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_LINEWIDTH, 0);

    status = _SetLineWidth(context, width);

    if (gcmIS_SUCCESS(status))
    {
        context->lineWidth = width;
    }
    else
    {
        gl2mERROR(GL_INVALID_VALUE);
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER();
#endif
}

GL_APICALL void GL_APIENTRY
glHint(
    GLenum target,
    GLenum mode
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("target=0x%04x mode=0x%04x", target, mode);
    gcmDUMP_API("${ES20 glHint 0x%08X 0x%08X}", target, mode);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_HINT, 0);

    switch (target)
    {
    case GL_GENERATE_MIPMAP_HINT:
        switch (mode)
        {
        case GL_FASTEST:
        case GL_NICEST:
        case GL_DONT_CARE:
            context->mipmapHint = mode;
            break;

        default:
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                          "%s(%d): Unknown mode=0x%04x",
                          __FUNCTION__, __LINE__, mode);

            gl2mERROR(GL_INVALID_ENUM);
            break;
        }
        break;

    case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
        switch (mode)
        {
        case GL_FASTEST:
        case GL_NICEST:
        case GL_DONT_CARE:
            context->fragShaderDerivativeHint = mode;
            break;

        default:
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                          "%s(%d): Unknown mode=0x%04x",
                          __FUNCTION__, __LINE__, mode);

            gl2mERROR(GL_INVALID_ENUM);
            break;
        }
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_STATE,
                      "%s(%d): Unknown target=0x%04x",
                      __FUNCTION__, __LINE__, target);

        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    gcmFOOTER_NO();
#endif
}
