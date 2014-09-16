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
#include "gc_hal_base.h"

static void
_GetColorSizes(
    GLContext Context,
    GLint * Red,
    GLint * Green,
    GLint * Blue,
    GLint * Alpha
    )
{
    gcoSURF                 surface;
    gceSURF_FORMAT          format;
    gcsSURF_FORMAT_INFO_PTR info[2];
    gceSTATUS               status;

    GLint red, green, blue, alpha;

    gcmHEADER_ARG("Context=0x%x", Context);

    surface = ( (Context->framebuffer != gcvNULL)
                &&
                (Context->framebuffer->color.surface != gcvNULL) )
            ? Context->framebuffer->color.surface
            : Context->draw;

    /* Query the format. */
    gcmONERROR(gcoSURF_GetFormat(surface, gcvNULL, &format));

    red = 0;
    green = 0;
    blue = 0;
    alpha = 0;

    /* Get format details. */
    if (gcmIS_SUCCESS(gcoSURF_QueryFormat(format, info)))
    {
        /* Assume it's an RGBA format. */
        gcmASSERT(info[0]->fmtClass == gcvFORMAT_CLASS_RGBA);

        if (!(info[0]->u.rgba.red.width & gcvCOMPONENT_DONTCARE))
        {
            red = info[0]->u.rgba.red.width & gcvCOMPONENT_WIDTHMASK;
        }

        if (!(info[0]->u.rgba.green.width & gcvCOMPONENT_DONTCARE))
        {
            green = info[0]->u.rgba.green.width & gcvCOMPONENT_WIDTHMASK;
        }

        if (!(info[0]->u.rgba.blue.width & gcvCOMPONENT_DONTCARE))
        {
            blue = info[0]->u.rgba.blue.width & gcvCOMPONENT_WIDTHMASK;
        }

        if (!(info[0]->u.rgba.alpha.width & gcvCOMPONENT_DONTCARE))
        {
            alpha = info[0]->u.rgba.alpha.width & gcvCOMPONENT_WIDTHMASK;
        }
    }
        /* Return component info. */
    if (Red != gcvNULL)
    {
       *Red = red;
    }

    if (Green != gcvNULL)
    {
        *Green = green;
    }

    if (Blue != gcvNULL)
    {
        *Blue = blue;
    }

    if (Alpha != gcvNULL)
    {
        *Alpha = alpha;
    }

OnError:
    gcmFOOTER_ARG("*Red=%d *Green=%d *Blue=%d *Alpha=%d", gcmOPT_VALUE(Red), gcmOPT_VALUE(Green), gcmOPT_VALUE(Blue), gcmOPT_VALUE(Alpha));
    return;

}

static void
_GetDepthSize(
    IN GLContext Context,
    OUT GLint * DepthBits
    )
{
    gcoSURF surface;
    gceSURF_FORMAT format;

    surface = ( (Context->framebuffer != gcvNULL)
                &&
                (Context->framebuffer->depth.surface != gcvNULL) )
            ? Context->framebuffer->depth.surface
            : Context->depth;

    if (surface == gcvNULL)
    {
        format = gcvSURF_UNKNOWN;
    }
    else
    {
        gcmVERIFY_OK(gcoSURF_GetFormat(surface, gcvNULL, &format));
    }

    switch (format)
    {
    case gcvSURF_UNKNOWN:
        *DepthBits = 0;
        break;

    case gcvSURF_D16:
        *DepthBits = 16;
        break;

    case gcvSURF_D24S8:
    case gcvSURF_D24X8:
        *DepthBits = 24;
        break;

    default:
        gcmFATAL("Invalid depth buffer format: %d", format);
    }
}

static void
_GetStencilSize(
    IN GLContext Context,
    OUT GLint * StencilBits
    )
{
    gcoSURF surface;
    gceSURF_FORMAT format;

    surface = ( (Context->framebuffer != gcvNULL)
                &&
                (Context->framebuffer->stencil.surface != gcvNULL) )
            ? Context->framebuffer->stencil.surface
            : Context->depth;

    if (surface == gcvNULL)
    {
        format = gcvSURF_UNKNOWN;
    }
    else
    {
        gcmVERIFY_OK(gcoSURF_GetFormat(surface, gcvNULL, &format));
    }

    switch (format)
    {
    case gcvSURF_UNKNOWN:
    case gcvSURF_D16:
    case gcvSURF_D24X8:
        *StencilBits = 0;
        break;

    case gcvSURF_D24S8:
        *StencilBits = 8;
        break;

    default:
        gcmFATAL("Invalid depth buffer format: %d", format);
    }
}

/*******************************************************************************
************************************************************** BOOLEAN states */

static void
_bGet_COLOR_WRITEMASK(
    GLContext Context,
    GLboolean * Values
    )
{
    Values[0] = Context->colorEnableRed;
    Values[1] = Context->colorEnableGreen;
    Values[2] = Context->colorEnableBlue;
    Values[3] = Context->colorEnableAlpha;
}

static void
_bGet_SAMPLE_COVERAGE_INVERT(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->sampleCoverageInvert;
}

static void
_bGet_DEPTH_WRITEMASK(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->depthMask;
}

static void
_bGet_SHADER_COMPILER(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = _glshLoadCompiler(Context);
}

static void
_bGet_BLEND(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->blendEnable;
}

static void
_bGet_CULL_FACE(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->cullEnable;
}

static void
_bGet_DEPTH_TEST(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->depthTest;
}

static void
_bGet_DITHER(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->ditherEnable;
}

static void
_bGet_SCISSOR_TEST(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->scissorEnable;
}

static void
_bGet_STENCIL_TEST(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->stencilEnable;
}

static void
_bGet_POLYGON_OFFSET_FILL(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->offsetEnable;
}

static void
_bGet_SAMPLE_ALPHA_TO_COVERAGE(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->sampleAlphaToCoverage;
}

static void
_bGet_SAMPLE_COVERAGE(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = Context->sampleCoverage;
}

static void
_bGet_TEXTURE_EXTERNAL_OES(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = gcvTRUE;
}

static void
_bGet_NUM_PROGRAM_BINARY_FORMATS_OES(
    GLContext Context,
    GLboolean * Value
    )
{
    *Value = gcvTRUE;
}

static void
_bGet_PROGRAM_BINARY_FORMATS_OES(
    GLContext Context,
    GLboolean * Value
    )
{
    /* How to return the format in boolean? */
    *Value = gcvTRUE;
}

/*******************************************************************************
************************************************************** INTEGER states */

static void
_iGet_RED_BITS(
    GLContext Context,
    GLint * Value
    )
{
    _GetColorSizes(Context, Value, gcvNULL, gcvNULL, gcvNULL);
}

static void
_iGet_GREEN_BITS(
    GLContext Context,
    GLint * Value
    )
{
    _GetColorSizes(Context, gcvNULL, Value, gcvNULL, gcvNULL);
}

static void
_iGet_BLUE_BITS(
    GLContext Context,
    GLint * Value
    )
{
    _GetColorSizes(Context, gcvNULL, gcvNULL, Value, gcvNULL);
}

static void
_iGet_ALPHA_BITS(
    GLContext Context,
    GLint * Value
    )
{
    _GetColorSizes(Context, gcvNULL, gcvNULL, gcvNULL, Value);
}

static void
_iGet_DEPTH_BITS(
    GLContext Context,
    GLint * Value
    )
{
    _GetDepthSize(Context, Value);
}

static void
_iGet_STENCIL_BITS(
    GLContext Context,
    GLint * Value
    )
{
    _GetStencilSize(Context, Value);
}

static void
_iGet_SUBPIXEL_BITS(
    GLContext Context,
    GLint * Value
    )
{
    *Value = 4;
}

static void
_iGet_MAX_TEXTURE_SIZE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = gcmMAX(Context->maxTextureWidth, Context->maxTextureHeight);
}

static void
_iGet_MAX_3D_TEXTURE_SIZE_OES(
    GLContext Context,
    GLint * Value
    )
{
    gctUINT maxWidth, maxHeight, maxDepth;

    gcmVERIFY_OK(gcoHAL_QueryTextureCaps(Context->hal,
                                         &maxWidth,
                                         &maxHeight,
                                         &maxDepth,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL));

    *Value = (maxDepth > 0) ? gcmMAX(gcmMAX(maxWidth, maxHeight),maxDepth) : 0;
}

static void
_iGet_MAX_CUBE_MAP_TEXTURE_SIZE(
    GLContext Context,
    GLint * Value
    )
{
    gctUINT maxWidth, maxHeight;
    gctBOOL cubic;

    gcmVERIFY_OK(gcoHAL_QueryTextureCaps(Context->hal,
                                         &maxWidth,
                                         &maxHeight,
                                         gcvNULL,
                                         &cubic,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL));

    *Value = cubic ? gcmMAX(maxWidth, maxHeight) : 0;
}

static void
_iGet_MAX_VIEWPORT_DIMS(
    GLContext Context,
    GLint * Values
    )
{
    Values[0] = Context->maxWidth;
    Values[1] = Context->maxHeight;
}

static void
_iGet_MAX_VERTEX_ATTRIBS(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->maxAttributes;
}

static void
_iGet_MAX_COMBINED_TEXTURE_IMAGE_UNITS(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->vertexSamplers + Context->fragmentSamplers;
}

static void
_iGet_MAX_VERTEX_TEXTURE_IMAGE_UNITS(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->vertexSamplers;
}

static void
_iGet_MAX_TEXTURE_IMAGE_UNITS(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->fragmentSamplers;
}

static void
_iGet_MAX_VERTEX_UNIFORM_VECTORS(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->vertexUniforms;
}

static void
_iGet_MAX_FRAGMENT_UNIFORM_VECTORS(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->fragmentUniforms;
}

static void
_iGet_MAX_RENDERBUFFER_SIZE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = gcmMAX(Context->maxWidth, Context->maxHeight);
}
static void
_iGet_ARRAY_BUFFER_BINDING(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->arrayBuffer == gcvNULL)
           ? 0
           : Context->arrayBuffer->object.name;
}

static void
_iGet_ELEMENT_ARRAY_BUFFER_BINDING(
    GLContext Context,
    GLint * Value
    )
{
    GLBuffer *elementArrayBuffer;

    if(Context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &Context->elementArrayBuffer;
    }
    else
    {
        elementArrayBuffer = &Context->vertexArrayObject->elementArrayBuffer;
    }

    *Value = (*elementArrayBuffer == gcvNULL)
           ? 0
           : (*elementArrayBuffer)->object.name;
}

static void
_iGet_VERTEX_ARRAY_BINDING_OES(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->vertexArrayObject == gcvNULL)
           ? 0
           : Context->vertexArrayObject->object.name;
}

static void
_iGet_VIEWPORT(
    GLContext Context,
    GLint * Values
    )
{
    Values[0] = Context->viewportX;
    Values[1] = Context->viewportY;
    Values[2] = Context->viewportWidth;
    Values[3] = Context->viewportHeight;
}

static void
_iGet_CULL_FACE_MODE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->cullMode;
}

static void
_iGet_FRONT_FACE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->cullFront;
}

static void
_iGet_TEXTURE_BINDING_2D(
    GLContext Context,
    GLint * Value
    )
{
    GLTexture texture = Context->texture2D[Context->textureUnit];

    *Value = (texture == gcvNULL)
           ? 0
           : texture->object.name;
}

static void
_iGet_TEXTURE_BINDING_3D_OES(
    GLContext Context,
    GLint * Value
    )
{
    GLTexture texture = Context->texture3D[Context->textureUnit];

    *Value = (texture == gcvNULL)
           ? 0
           : texture->object.name;
}

static void
_iGet_TEXTURE_BINDING_CUBE_MAP(
    GLContext Context,
    GLint * Value
    )
{
    GLTexture texture = Context->textureCube[Context->textureUnit];

    *Value = (texture == gcvNULL)
           ? 0
           : texture->object.name;
}

static void
_iGet_TEXTURE_BINDING_EXTERNAL_OES(
    GLContext Context,
    GLint * Value
    )
{
    GLTexture texture = Context->textureExternal[Context->textureUnit];

    *Value = (texture == gcvNULL)
           ? 0
           : texture->object.name;
}

static void
_iGet_ACTIVE_TEXTURE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = GL_TEXTURE0 + Context->textureUnit;
}

static void
_iGet_STENCIL_WRITEMASK(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->stencilMaskFront;
}

static void
_iGet_STENCIL_BACK_WRITEMASK(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->stencilMaskBack;
}

static void
_iGet_STENCIL_CLEAR_VALUE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = Context->clearStencil;
}

static void
_iGet_SCISSOR_BOX(
    GLContext Context,
    GLint * Values
    )
{
    Values[0] = Context->scissorX;
    Values[1] = Context->scissorY;
    Values[2] = Context->scissorWidth;
    Values[3] = Context->scissorHeight;
}

static void
_iGet_STENCIL_FUNC(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilFuncFront;
}

static void
_iGet_STENCIL_VALUE_MASK(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilMaskFront;
}

static void
_iGet_STENCIL_REF(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilRefFront;
}

static void
_iGet_STENCIL_FAIL(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilOpFailFront;
}

static void
_iGet_STENCIL_PASS_DEPTH_FAIL(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilOpDepthFailFront;
}

static void
_iGet_STENCIL_PASS_DEPTH_PASS(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilOpDepthPassFront;
}

static void
_iGet_STENCIL_BACK_FUNC(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilFuncBack;
}

static void
_iGet_STENCIL_BACK_VALUE_MASK(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilMaskBack;
}

static void
_iGet_STENCIL_BACK_REF(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilRefBack;
}

static void
_iGet_STENCIL_BACK_FAIL(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilOpFailBack;
}

static void
_iGet_STENCIL_BACK_PASS_DEPTH_FAIL(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilOpDepthFailBack;
}

static void
_iGet_STENCIL_BACK_PASS_DEPTH_PASS(
    GLContext context,
    GLint *param
    )
{
    *param = context->stencilOpDepthPassBack;
}

static void
_iGet_DEPTH_FUNC(
    GLContext context,
    GLint *param
    )
{
    *param = context->depthFunc;
}

static void
_iGet_BLEND_SRC_RGB(
    GLContext context,
    GLint *param
    )
{
    *param = context->blendFuncSourceRGB;
}

static void
_iGet_BLEND_SRC_ALPHA(
    GLContext context,
    GLint *param
    )
{
    *param = context->blendFuncSourceAlpha;
}

static void
_iGet_BLEND_DST_RGB(
    GLContext context,
    GLint *param
    )
{
    *param = context->blendFuncTargetRGB;
}

static void
_iGet_BLEND_DST_ALPHA(
    GLContext context,
    GLint *param
    )
{
    *param = context->blendFuncTargetAlpha;
}

static void
_iGet_BLEND_EQUATION_RGB(
    GLContext context,
    GLint *param
    )
{
    *param = context->blendModeRGB;
}

static void
_iGet_BLEND_EQUATION_ALPHA(
    GLContext context,
    GLint *param
    )
{
    *param = context->blendModeAlpha;
}

static void
_iGet_UNPACK_ALIGNMENT(
    GLContext context,
    GLint *param
    )
{
    *param = context->unpackAlignment;
}

static void
_iGet_PACK_ALIGNMENT(
    GLContext context,
    GLint *param
    )
{
    *param = context->packAlignment;
}

static void
_iGet_CURRENT_PROGRAM(
    GLContext context,
    GLint *param
    )
{
    *param = (context->program == gcvNULL)
           ? 0
           : context->program->object.name;
}

static void
_iGet_GENERATE_MIPMAP_HINT(
    GLContext Context,
    GLint * Parameters
    )
{
    *Parameters = Context->mipmapHint;
}

static void
_iGet_FRAGMENT_SHADER_DERIVATIVE_HINT_OES(
    GLContext Context,
    GLint * Parameters
    )
{
    *Parameters = Context->fragShaderDerivativeHint;
}

static void
_iGet_SAMPLE_BUFFERS(
    GLContext context,
    GLint *param
    )
{
    /* TODO */
    *param = 0;
}

static void
_iGet_SAMPLES(
    GLContext context,
    GLint *param
    )
{
    /* TODO */
    *param = 0;
}

static void
_iGet_COMPRESSED_TEXTURE_FORMATS(
    GLContext context,
    GLint *params
    )
{
    params[0] = GL_PALETTE4_RGBA4_OES;
    params[1] = GL_PALETTE4_RGB5_A1_OES;
    params[2] = GL_PALETTE4_R5_G6_B5_OES;
    params[3] = GL_PALETTE4_RGB8_OES;
    params[4] = GL_PALETTE4_RGBA8_OES;
    params[5] = GL_PALETTE8_RGBA4_OES;
    params[6] = GL_PALETTE8_RGB5_A1_OES;
    params[7] = GL_PALETTE8_R5_G6_B5_OES;
    params[8] = GL_PALETTE8_RGB8_OES;
    params[9] = GL_PALETTE8_RGBA8_OES;

    params[10] = GL_ETC1_RGB8_OES;
    params[11] = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    params[12] = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    params[13] = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    params[14] = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
}

static void
_iGet_NUM_COMPRESSED_TEXTURE_FORMATS(
    GLContext context,
    GLint *param
    )
{
    *param = 15;
}

static void
_iGet_SHADER_BINARY_FORMATS(
    GLContext context,
    GLint *param
    )
{
    param[0] = GL_SHADER_BINARY_VIV;
}

static void
_iGet_NUM_SHADER_BINARY_FORMATS(
    GLContext context,
    GLint *param
    )
{
    *param = 1;
}

static void
_iGet_MAX_VARYING_VECTORS(
    GLContext context,
    GLint *param
    )
{
    *param = context->maxVaryings;
}

static void
_iGet_IMPLEMENTATION_COLOR_READ_TYPE(
    GLContext context,
    GLint *param
    )
{
    *param = GL_UNSIGNED_BYTE;
}

static void
_iGet_IMPLEMENTATION_COLOR_READ_FORMAT(
    GLContext context,
    GLint *param
    )
{
    *param = GL_RGBA;
}

static void
_iGet_RENDERBUFFER_BINDING(
    GLContext Context,
    GLint * Parameter
    )
{
    *Parameter = (Context->renderbuffer == gcvNULL)
               ? 0
               : Context->renderbuffer->object.name;
}

static void
_iGet_FRAMEBUFFER_BINDING(
    GLContext Context,
    GLint * Parameter
    )
{
    *Parameter = (Context->framebuffer == gcvNULL)
               ? 0
               : Context->framebuffer->object.name;
}

static void
_iGet_DEPTH_RANGE(
    GLContext Context,
    GLint * Values
    )
{
    GLint depth = 0;
    GLint max;

    _GetDepthSize(Context, &depth);

    max = (1 << depth) - 1;

    Values[0] = (GLint) (Context->depthNear * max);
    Values[1] = (GLint) (Context->depthFar  * max);
}

static void
_iGet_BLEND_COLOR(
    GLContext Context,
    GLint * Values
    )
{
    GLint red = 0, green = 0, blue = 0, alpha = 0;

    _GetColorSizes(Context, &red, &green, &blue, &alpha);

    Values[0] = (GLint) (Context->blendColorRed   * (GLint)((1 << red)   - 1));
    Values[1] = (GLint) (Context->blendColorGreen * (GLint)((1 << green) - 1));
    Values[2] = (GLint) (Context->blendColorBlue  * (GLint)((1 << blue)  - 1));
    Values[3] = (GLint) (Context->blendColorAlpha * (GLint)((1 << alpha) - 1));
}

static void
_iGet_COLOR_CLEAR_VALUE(
    GLContext Context,
    GLint * Values
    )
{
    GLint red = 0, green = 0, blue = 0, alpha = 0;

    _GetColorSizes(Context, &red, &green, &blue, &alpha);

    Values[0] = (GLint) (Context->clearRed   * (GLint)((1 << red)   - 1));
    Values[1] = (GLint) (Context->clearGreen * (GLint)((1 << green) - 1));
    Values[2] = (GLint) (Context->clearBlue  * (GLint)((1 << blue)  - 1));
    Values[3] = (GLint) (Context->clearAlpha * (GLint)((1 << alpha) - 1));
}

static void
_iGet_DEPTH_CLEAR_VALUE(
    GLContext Context,
    GLint * Values
    )
{
    GLint depth = 0;

    _GetDepthSize(Context, &depth);

    *Values = (GLint) (Context->clearDepth * (GLint)((1 << depth) - 1));
}

static void
_iGet_TIMESTAMP_VIV0(
    GLContext Context,
    GLint * Parameter
    )
{
    gcmVERIFY_OK(
        gcoHAL_GetTimerTime(Context->hal,
                            GL_TIMESTAMP_VIV0 - GL_TIMESTAMP_VIV0,
                            Parameter));
}

static void
_iGet_TIMESTAMP_VIV1(
    GLContext Context,
    GLint * Parameter
    )
{
    gcmVERIFY_OK(
        gcoHAL_GetTimerTime(Context->hal,
                            GL_TIMESTAMP_VIV1 - GL_TIMESTAMP_VIV0,
                            Parameter));
}

static void
_iGet_TIMESTAMP_VIV2(
    GLContext Context,
    GLint * Parameter
    )
{
    gcmVERIFY_OK(
        gcoHAL_GetTimerTime(Context->hal,
                            GL_TIMESTAMP_VIV2 - GL_TIMESTAMP_VIV0,
                            Parameter));
}

static void
_iGet_TIMESTAMP_VIV3(
    GLContext Context,
    GLint * Parameter
    )
{
    gcmVERIFY_OK(
        gcoHAL_GetTimerTime(Context->hal,
                            GL_TIMESTAMP_VIV3 - GL_TIMESTAMP_VIV0,
                            Parameter));
}

static void
_iGet_TIMESTAMP_VIV4(
    GLContext Context,
    GLint * Parameter
    )
{
    gcmVERIFY_OK(
        gcoHAL_GetTimerTime(Context->hal,
                            GL_TIMESTAMP_VIV4 - GL_TIMESTAMP_VIV0,
                            Parameter));
}

static void
_iGet_TIMESTAMP_VIV5(
    GLContext Context,
    GLint * Parameter
    )
{
    gcmVERIFY_OK(
        gcoHAL_GetTimerTime(Context->hal,
                            GL_TIMESTAMP_VIV5 - GL_TIMESTAMP_VIV0,
                            Parameter));
}

static void
_iGet_TIMESTAMP_VIV6(
    GLContext Context,
    GLint * Parameter
    )
{
    gcmVERIFY_OK(
        gcoHAL_GetTimerTime(Context->hal,
                            GL_TIMESTAMP_VIV6 - GL_TIMESTAMP_VIV0,
                            Parameter));
}

static void
_iGet_TIMESTAMP_VIV7(
    GLContext Context,
    GLint * Parameter
    )
{
    gcmVERIFY_OK(
        gcoHAL_GetTimerTime(Context->hal,
                            GL_TIMESTAMP_VIV7 - GL_TIMESTAMP_VIV0,
                            Parameter));
}

static void
_iGet_BLEND(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->blendEnable) ? 1 : 0;
}

static void
_iGet_CULL_FACE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->cullEnable) ? 1 : 0;
}

static void
_iGet_DEPTH_TEST(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->depthTest) ? 1 : 0;
}

static void
_iGet_DITHER(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->ditherEnable) ? 1 : 0;
}

static void
_iGet_SCISSOR_TEST(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->scissorEnable) ? 1 : 0;
}

static void
_iGet_STENCIL_TEST(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->stencilEnable) ? 1 : 0;
}

static void
_iGet_POLYGON_OFFSET_FILL(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->offsetEnable) ? 1 : 0;
}

static void
_iGet_SAMPLE_ALPHA_TO_COVERAGE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->sampleAlphaToCoverage) ? 1 : 0;
}

static void
_iGet_SAMPLE_COVERAGE(
    GLContext Context,
    GLint * Value
    )
{
    *Value = (Context->sampleCoverage) ? 1 : 0;
}

static void
_iGet_TEXTURE_EXTERNAL_OES(
    GLContext Context,
    GLint * Value
    )
{
    *Value = 1;
}

static void
_iGet_NUM_PROGRAM_BINARY_FORMATS_OES(
    GLContext Context,
    GLint * Value
    )
{
    *Value = 1;
}

static void
_iGet_PROGRAM_BINARY_FORMATS_OES(
    GLContext Context,
    GLint * Value
    )
{
    *Value = GL_PROGRAM_BINARY_VIV;
}

/*******************************************************************************
************************************************************** Float states */

static void
_fGet_DEPTH_CLEAR_VALUE(
    GLContext Context,
    GLfloat * Values
    )
{
    *Values = Context->clearDepth;
}

static void
_fGet_DEPTH_RANGE(
    GLContext context,
    GLfloat *params
    )
{
    params[0] = context->depthNear;
    params[1] = context->depthFar;
}

static void
_fGet_LINE_WIDTH(
    GLContext context,
    GLfloat *params
    )
{
    *params = context->lineWidth;
}

static void
_fGet_POLYGON_OFFSET_FACTOR(
    GLContext context,
    GLfloat *params
    )
{
    *params = context->offsetFactor;
}

static void
_fGet_POLYGON_OFFSET_UNITS(
    GLContext context,
    GLfloat *params
    )
{
    *params = context->offsetUnits;
}

static void
_fGet_SAMPLE_COVERAGE_VALUE(
    GLContext context,
    GLfloat *params
    )
{
    *params = context->sampleCoverageValue;
}

static void
_fGet_COLOR_CLEAR_VALUE(
    GLContext context,
    GLfloat *params
    )
{
    params[0] = context->clearRed;
    params[1] = context->clearGreen;
    params[2] = context->clearBlue;
    params[3] = context->clearAlpha;
}

static void
_fGet_BLEND_COLOR(
    GLContext Context,
    GLfloat * Values
    )
{
    Values[0] = Context->blendColorRed;
    Values[1] = Context->blendColorGreen;
    Values[2] = Context->blendColorBlue;
    Values[3] = Context->blendColorAlpha;
}

static void
_fGet_ALIASED_POINT_SIZE_RANGE(
    GLContext Context,
    GLfloat * Values
    )
{
    Values[0] = 0.0f;
    Values[1] = 2048.0f;
}

static void
_fGet_ALIASED_LINE_WIDTH_RANGE(
    GLContext Context,
    GLfloat * Values
    )
{
    Values[0] = Context->lineWidthRange[0];
    Values[1] = Context->lineWidthRange[1];
}

static void
_fGet_MAX_TEXTURE_MAX_ANISOTROPY_EXT(
    GLContext Context,
    GLfloat * Values
    )
{
    Values[0] = (GLfloat)Context->maxAniso;
}

static void
_fGet_BLEND(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->blendEnable) ? 1.0f : 0.0f;
}

static void
_fGet_CULL_FACE(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->cullEnable) ? 1.0f : 0.0f;
}

static void
_fGet_DEPTH_TEST(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->depthTest) ? 1.0f : 0.0f;
}

static void
_fGet_DITHER(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->ditherEnable) ? 1.0f : 0.0f;
}

static void
_fGet_SCISSOR_TEST(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->scissorEnable) ? 1.0f : 0.0f;
}

static void
_fGet_STENCIL_TEST(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->stencilEnable) ? 1.0f : 0.0f;
}

static void
_fGet_POLYGON_OFFSET_FILL(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->offsetEnable) ? 1.0f : 0.0f;
}

static void
_fGet_SAMPLE_ALPHA_TO_COVERAGE(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->sampleAlphaToCoverage) ? 1.0f : 0.0f;
}

static void
_fGet_SAMPLE_COVERAGE(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (Context->sampleCoverage) ? 1.0f : 0.0f;
}

static void
_fGet_TEXTURE_EXTERNAL_OES(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = 1.0f;
}

static void
_fGet_NUM_PROGRAM_BINARY_FORMATS_OES(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = 1.0f;
}

static void
_fGet_PROGRAM_BINARY_FORMATS_OES(
    GLContext Context,
    GLfloat * Value
    )
{
    *Value = (GLfloat)GL_PROGRAM_BINARY_VIV;
}

/*******************************************************************************
*((************************************************************* STATE tables */

struct _GLStateGetter
{
    GLenum  name;
    GLuint  count;
    void    (*getBoolean)(GLContext Context, GLboolean * Values);
    void    (*getFloat)(GLContext Context, GLfloat * Values);
    void    (*getInteger)(GLContext Context, GLint * Values);
};

typedef struct _GLStateGetter * GLStateGetter;

#define MAX_PCOUNT  15

#define GET_BOOLEAN(tag, count) \
    { GL_ ## tag, count, _bGet_ ## tag, gcvNULL, gcvNULL }
#define GET_INTEGER(tag, count) \
    { GL_ ## tag, count, gcvNULL, gcvNULL, _iGet_ ## tag}
#define GET_FLOAT(tag, count) \
    { GL_ ## tag, count, gcvNULL, _fGet_ ## tag, gcvNULL }
#define GET_INTEGER_FLOAT(tag, count) \
    { GL_ ## tag, count, gcvNULL, _fGet_ ## tag, _iGet_ ## tag }
#define GET_BOOLEAN_INTEGER_FLOAT(tag, count) \
    { GL_ ## tag, count, _bGet_ ## tag, _fGet_ ## tag, _iGet_ ## tag }

static struct _GLStateGetter _stateGetters[] =
{
    /* Only getBoolean */
    GET_BOOLEAN(COLOR_WRITEMASK, 4),
    GET_BOOLEAN(SAMPLE_COVERAGE_INVERT, 1),
    GET_BOOLEAN(DEPTH_WRITEMASK, 1),
    GET_BOOLEAN(SHADER_COMPILER, 1),
	GET_BOOLEAN(BLEND, 1),
    GET_BOOLEAN(CULL_FACE, 1),
    GET_BOOLEAN(DEPTH_TEST, 1),
    GET_BOOLEAN(DITHER, 1),
    GET_BOOLEAN(SCISSOR_TEST, 1),
    GET_BOOLEAN(STENCIL_TEST, 1),
    GET_BOOLEAN(POLYGON_OFFSET_FILL, 1),
    GET_BOOLEAN(SAMPLE_ALPHA_TO_COVERAGE, 1),
    GET_BOOLEAN(SAMPLE_COVERAGE, 1),
    GET_BOOLEAN(TEXTURE_EXTERNAL_OES, 1),

    /* Only getInteger */
    GET_INTEGER(RED_BITS, 1),
    GET_INTEGER(GREEN_BITS, 1),
    GET_INTEGER(BLUE_BITS, 1),
    GET_INTEGER(ALPHA_BITS, 1),
    GET_INTEGER(DEPTH_BITS, 1),
    GET_INTEGER(STENCIL_BITS, 1),
    GET_INTEGER(SUBPIXEL_BITS, 1),
    GET_INTEGER(MAX_TEXTURE_SIZE, 1),
    GET_INTEGER(MAX_CUBE_MAP_TEXTURE_SIZE, 1),
    GET_INTEGER(MAX_VIEWPORT_DIMS, 2),
    GET_INTEGER(MAX_VERTEX_ATTRIBS, 1),
    GET_INTEGER(MAX_COMBINED_TEXTURE_IMAGE_UNITS, 1),
    GET_INTEGER(MAX_VERTEX_TEXTURE_IMAGE_UNITS, 1),
    GET_INTEGER(MAX_TEXTURE_IMAGE_UNITS, 1),
    GET_INTEGER(MAX_VERTEX_UNIFORM_VECTORS, 1),
    GET_INTEGER(MAX_FRAGMENT_UNIFORM_VECTORS, 1),
    GET_INTEGER(MAX_RENDERBUFFER_SIZE, 2),
    GET_INTEGER(ARRAY_BUFFER_BINDING, 1),
    GET_INTEGER(ELEMENT_ARRAY_BUFFER_BINDING, 1),
    GET_INTEGER(VIEWPORT, 4),
    GET_INTEGER(CULL_FACE_MODE, 1),
    GET_INTEGER(FRONT_FACE, 1),
    GET_INTEGER(TEXTURE_BINDING_2D, 1),
    GET_INTEGER(TEXTURE_BINDING_CUBE_MAP, 1),
    GET_INTEGER(TEXTURE_BINDING_EXTERNAL_OES, 1),
    GET_INTEGER(ACTIVE_TEXTURE, 1),
    GET_INTEGER(SCISSOR_BOX, 4),
    GET_INTEGER(STENCIL_CLEAR_VALUE, 1),
    GET_INTEGER(STENCIL_WRITEMASK, 1),
    GET_INTEGER(STENCIL_BACK_WRITEMASK, 1),
    GET_INTEGER(STENCIL_FUNC, 1),
    GET_INTEGER(STENCIL_VALUE_MASK, 1),
    GET_INTEGER(STENCIL_REF, 1),
    GET_INTEGER(STENCIL_FAIL, 1),
    GET_INTEGER(STENCIL_PASS_DEPTH_FAIL, 1),
    GET_INTEGER(STENCIL_PASS_DEPTH_PASS, 1),
    GET_INTEGER(STENCIL_BACK_FUNC, 1),
    GET_INTEGER(STENCIL_BACK_VALUE_MASK, 1),
    GET_INTEGER(STENCIL_BACK_REF, 1),
    GET_INTEGER(STENCIL_BACK_FAIL, 1),
    GET_INTEGER(STENCIL_BACK_PASS_DEPTH_FAIL, 1),
    GET_INTEGER(STENCIL_BACK_PASS_DEPTH_PASS, 1),
    GET_INTEGER(DEPTH_FUNC, 1),
    GET_INTEGER(BLEND_SRC_RGB, 1),
    GET_INTEGER(BLEND_SRC_ALPHA, 1),
    GET_INTEGER(BLEND_DST_RGB, 1),
    GET_INTEGER(BLEND_DST_ALPHA, 1),
    GET_INTEGER(BLEND_EQUATION_RGB, 1),
    GET_INTEGER(BLEND_EQUATION_ALPHA, 1),
    GET_INTEGER(UNPACK_ALIGNMENT, 1),
    GET_INTEGER(PACK_ALIGNMENT, 1),
    GET_INTEGER(CURRENT_PROGRAM, 1),
    GET_INTEGER(GENERATE_MIPMAP_HINT, 1),
    GET_INTEGER(FRAGMENT_SHADER_DERIVATIVE_HINT_OES, 1),
    GET_INTEGER(SAMPLE_BUFFERS, 1),
    GET_INTEGER(SAMPLES, 1),
    GET_INTEGER(COMPRESSED_TEXTURE_FORMATS, 15),
    GET_INTEGER(NUM_COMPRESSED_TEXTURE_FORMATS, 1),
    GET_INTEGER(SHADER_BINARY_FORMATS, 0),
    GET_INTEGER(NUM_SHADER_BINARY_FORMATS, 1),
    GET_INTEGER(MAX_VARYING_VECTORS, 1),
    GET_INTEGER(IMPLEMENTATION_COLOR_READ_TYPE, 1),
    GET_INTEGER(IMPLEMENTATION_COLOR_READ_FORMAT, 1),
    GET_INTEGER(RENDERBUFFER_BINDING, 1),
    GET_INTEGER(FRAMEBUFFER_BINDING, 1),
    GET_INTEGER(TIMESTAMP_VIV0, 1),
    GET_INTEGER(TIMESTAMP_VIV1, 1),
    GET_INTEGER(TIMESTAMP_VIV2, 1),
    GET_INTEGER(TIMESTAMP_VIV3, 1),
    GET_INTEGER(TIMESTAMP_VIV4, 1),
    GET_INTEGER(TIMESTAMP_VIV5, 1),
    GET_INTEGER(TIMESTAMP_VIV6, 1),
    GET_INTEGER(TIMESTAMP_VIV7, 1),
    GET_INTEGER(TEXTURE_BINDING_3D_OES, 1),
    GET_INTEGER(MAX_3D_TEXTURE_SIZE_OES, 1),
    GET_INTEGER(VERTEX_ARRAY_BINDING_OES,1 ),

    /* Only getFloat */
    GET_FLOAT(LINE_WIDTH, 1),
    GET_FLOAT(POLYGON_OFFSET_FACTOR, 1),
    GET_FLOAT(POLYGON_OFFSET_UNITS, 1),
    GET_FLOAT(SAMPLE_COVERAGE_VALUE, 1),
    GET_FLOAT(ALIASED_POINT_SIZE_RANGE, 2),
    GET_FLOAT(ALIASED_LINE_WIDTH_RANGE, 2),
    GET_FLOAT(MAX_TEXTURE_MAX_ANISOTROPY_EXT, 1),

    /* Both getInteger and getFloat */
    GET_INTEGER_FLOAT(DEPTH_RANGE, 2),
    GET_INTEGER_FLOAT(BLEND_COLOR, 4),
    GET_INTEGER_FLOAT(COLOR_CLEAR_VALUE, 4),
    GET_INTEGER_FLOAT(DEPTH_CLEAR_VALUE, 1),
    GET_INTEGER_FLOAT(BLEND, 1),
    GET_INTEGER_FLOAT(CULL_FACE, 1),
    GET_INTEGER_FLOAT(DEPTH_TEST, 1),
    GET_INTEGER_FLOAT(DITHER, 1),
    GET_INTEGER_FLOAT(SCISSOR_TEST, 1),
    GET_INTEGER_FLOAT(STENCIL_TEST, 1),
    GET_INTEGER_FLOAT(POLYGON_OFFSET_FILL, 1),
    GET_INTEGER_FLOAT(SAMPLE_ALPHA_TO_COVERAGE, 1),
    GET_INTEGER_FLOAT(SAMPLE_COVERAGE, 1),
    GET_INTEGER_FLOAT(TEXTURE_EXTERNAL_OES, 1),

    /* All three. */
    GET_BOOLEAN_INTEGER_FLOAT(NUM_PROGRAM_BINARY_FORMATS_OES, 1),
    GET_BOOLEAN_INTEGER_FLOAT(PROGRAM_BINARY_FORMATS_OES, 1),

    /* End of array */
    { GL_NONE, 0, gcvNULL, gcvNULL, gcvNULL },
};

static void
_CommonGetv(
    GLenum Type,
    GLenum Name,
    void * Values
    )
{
    GLContext context;
    GLuint i;
    GLStateGetter p;
    GLboolean created = GL_FALSE;

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        context = _glshCreateContext();
        if (context == gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_QUERY,
                          "Cannot create context");
            return;
        }

        created = GL_TRUE;
    }

    for (p = _stateGetters; p->name != GL_NONE; ++p)
    {
        if (p->name == Name)
        {
            break;
        }
    }

    if (p->name == GL_NONE)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_QUERY,
                      "Unknown Name=%04X", Name);
        gl2mERROR(GL_INVALID_ENUM);
    }

    else
    {
        switch (Type)
        {
        case GL_BOOL:
            if (p->getBoolean != gcvNULL)
            {
                p->getBoolean(context, (GLboolean *) Values);
            }

            else
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_QUERY,
                              "Converting state %04X into GLboolean",
                              p->name);

                if (p->getInteger != gcvNULL)
                {
                    GLint values[MAX_PCOUNT];
                    p->getInteger(context, values);

                    for (i = 0; i < p->count; ++i)
                    {
                        ((GLboolean *) Values)[i] = (values[i] == 0)
                                                  ? GL_FALSE
                                                  : GL_TRUE;
                    }
                }

                else
                if (p->getFloat != gcvNULL)
                {
                    GLfloat values[MAX_PCOUNT];

                    p->getFloat(context, values);

                    for (i = 0; i < p->count; ++i)
                    {
                        ((GLboolean *) Values)[i] = (values[i] == 0.0f)
                                                  ? GL_FALSE
                                                  : GL_TRUE;
                    }
                }
            }
            break;

        case GL_FLOAT:
            if (p->getFloat != gcvNULL)
            {
                p->getFloat(context, (GLfloat *) Values);
            }

            else
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_QUERY,
                              "Converting state %04X into GLfloat",
                              p->name);

                if (p->getInteger != gcvNULL)
                {
                    GLint values[MAX_PCOUNT];

                    p->getInteger(context, values);

                    for (i = 0; i < p->count; ++i)
                    {
                        ((GLfloat *) Values)[i] = (GLfloat) values[i];
                    }
                }

                else
                if (p->getBoolean != gcvNULL)
                {
                    GLboolean values[MAX_PCOUNT];

                    p->getBoolean(context, values);

                    for (i = 0; i < p->count; ++i)
                    {
                        ((GLfloat *) Values)[i] = values[i] ? 1.0f : 0.0f;
                    }
                }
            }
            break;

        case GL_INT:
            if (p->getInteger != gcvNULL)
            {
                p->getInteger(context, (GLint *) Values);
            }

            else
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_QUERY,
                              "Converting state %04X into GLint",
                              p->name);

                if (p->getFloat != gcvNULL)
                {
                    GLfloat values[MAX_PCOUNT];

                    p->getFloat(context, values);

                    for(i = 0; i < p->count; ++i)
                    {
                        ((GLint *) Values)[i] = (GLint) (values[i] + 0.5f);
                    }
                }

                if (p->getBoolean != gcvNULL)
                {
                    GLboolean values[MAX_PCOUNT];

                    p->getBoolean(context, values);

                    for (i = 0; i < p->count; ++i)
                    {
                        ((GLint *) Values)[i] = values[i] ? 1 : 0;
                    }
                }
            }
            break;

        default:
            gcmFATAL("No conversion found for parameter name %04X", p->name);
        }
    }

    if (created)
    {
        _glshDestroyContext(context);
    }
}

GL_APICALL GLenum GL_APIENTRY
glGetError(
    void
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLenum error;

    gcmHEADER();

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_ARG("0x%04x", GL_INVALID_OPERATION);
        return GL_INVALID_OPERATION;
    }

    glmPROFILE(context, GLES2_GETERROR, 0);

    error = context->error;
    context->error = GL_NO_ERROR;

    switch (error)
    {
    case GL_NO_ERROR:
        break;

    case GL_INVALID_ENUM:
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_QUERY,
                      "%s(%d): INVALID_ENUM",
                      __FUNCTION__, __LINE__);
        break;

    case GL_INVALID_VALUE:
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_QUERY,
                      "%s(%d): INVALID_VALUE",
                      __FUNCTION__, __LINE__);
        break;

    case GL_INVALID_OPERATION:
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_QUERY,
                      "%s(%d): INVALID_OPERATION",
                      __FUNCTION__, __LINE__);
        break;

    case GL_OUT_OF_MEMORY:
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_QUERY,
                      "%s(%d): OUT_OF_MEMORY",
                      __FUNCTION__, __LINE__);
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_QUERY,
                      "%s(%d): %04X", __FUNCTION__, __LINE__, error);
        break;
    }

    gcmDUMP_API("${ES20 glGetError := 0x%08X}", error);
    gcmFOOTER_ARG("0x%04x", error);
    return error;
#else
    return GL_NO_ERROR;
#endif
}

GL_APICALL const GLubyte * GL_APIENTRY
glGetString(
    GLenum name
    )
{
    GLContext context;
    char * result;

    GLboolean created = GL_FALSE;

    gcmHEADER_ARG("name=0x%04x", name);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        context = _glshCreateContext();
        if (context == gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_QUERY,
                          "Cannot create context");
            gcmFOOTER_ARG("%s", "");
            return gcvNULL;
        }

        created = GL_TRUE;
    }

    glmPROFILE(context, GLES2_GETSTRING, 0);

    switch (name)
    {
    case GL_VENDOR:
        result = "Vivante Corporation";
        break;

    case GL_RENDERER:
        result = context->renderer;
        break;

    case GL_VERSION:
        result = "OpenGL ES 2.0";
        break;

    case GL_SHADING_LANGUAGE_VERSION:
        result = "OpenGL ES GLSL ES 1.00";
        break;

    case GL_EXTENSIONS:
        result = context->extensionString;
        break;

    default:
#ifndef __QNXNTO__
		/* QNX redir library asks for non-standard enums.
		   Don't die out fatally on that. */
        gcmFATAL("%s(%d): Unknown name=0x%04x", __FUNCTION__, __LINE__, name);
#endif
        if (context != gcvNULL)
        {
            gl2mERROR(GL_INVALID_ENUM);
        }
        /* For QNX, it's mandatory to return gcvNULL. */
        result = gcvNULL;
    }

    if (created)
    {
        _glshDestroyContext(context);
    }

    gcmDUMP_API("${ES20 glGetString 0x%08X :=", name);
    gcmDUMP_API_DATA(result, 0);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("%s", gcmOPT_STRING(result));
    return (GLubyte *) result;
}

GL_APICALL void GL_APIENTRY
glGetIntegerv(
    GLenum pname,
    GLint * params
    )
{
    GLContext context;

    gcmHEADER_ARG("pname=0x%04x", pname);

    context = _glshGetCurrentContext();
    if (context != gcvNULL)
    {
        glmPROFILE(context, GLES2_GETINTEGERV, 0);
    }

    _CommonGetv(GL_INT, pname, params);

    gcmDUMP_API("${ES20 glGetIntegerv 0x%08X (0x%08X)", pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*params=%d", gcmOPT_VALUE(params));
}

GL_APICALL void GL_APIENTRY
glGetBooleanv(
    GLenum pname,
    GLboolean * params
    )
{
    GLContext context;

    gcmHEADER_ARG("pname=0x%04x", pname);

    context = _glshGetCurrentContext();
    if (context != gcvNULL)
    {
        glmPROFILE(context, GLES2_GETBOOLEANV, 0);
    }

    _CommonGetv(GL_BOOL, pname, params);

    gcmDUMP_API("${ES20 glGetBooleanv 0x%08X (0x%08X)", pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*params=%d", gcmOPT_VALUE(params));
}

GL_APICALL void GL_APIENTRY
glGetFloatv(
    GLenum pname,
    GLfloat * params
    )
{
    GLContext context;

    gcmHEADER_ARG("pname=0x%04x", pname);

    context = _glshGetCurrentContext();
    if (context != gcvNULL)
    {
        glmPROFILE(context, GLES2_GETFLOATV, 0);
    }

    _CommonGetv(GL_FLOAT, pname, params);

    gcmDUMP_API("${ES20 glGetFloatv 0x%08X (0x%08X)", pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*params=%f", gcmOPT_VALUE(params));
}

GL_APICALL GLboolean GL_APIENTRY
glIsEnabled(
    GLenum cap
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLboolean enabled;

    gcmHEADER_ARG("cap=0x%04x", cap);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_ARG("%d", GL_FALSE);
        return GL_FALSE;
    }

    glmPROFILE(context, GLES2_ISENABLED, 0);

    switch (cap)
    {
    case GL_BLEND:
        enabled = context->blendEnable;
        break;

    case GL_CULL_FACE:
        enabled = context->cullEnable;
        break;

    case GL_DEPTH_TEST:
        enabled = context->depthTest;
        break;

    case GL_DITHER:
        enabled = context->ditherEnable;
        break;

    case GL_SCISSOR_TEST:
        enabled = context->scissorEnable;
        break;

    case GL_STENCIL_TEST:
        enabled = context->stencilEnable;
        break;

    case GL_POLYGON_OFFSET_FILL:
        enabled = context->offsetEnable;
        break;

    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        enabled = context->sampleAlphaToCoverage;
        break;

    case GL_SAMPLE_COVERAGE:
        enabled = context->sampleCoverage;
        break;

    case GL_TEXTURE_EXTERNAL_OES:
        enabled = gcvTRUE;
        break;

    default:
        gcmFATAL("%s(%d): Unknown cap=0x%04x", __FUNCTION__, __LINE__, cap);
        gl2mERROR(GL_INVALID_ENUM);
        enabled = GL_FALSE;
        break;
    }

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gldZONE_QUERY,
                  "%s(%d): enable is %s", __FUNCTION__, __LINE__, enabled ? "GL_TRUE" : "GL_FALSE");

    gcmDUMP_API("${ES20 glIsEnabled 0x%08X := 0x%08X}", cap, enabled);
    gcmFOOTER_ARG("%d", enabled);
    return enabled;
#else
    return GL_FALSE;
#endif
}

GL_APICALL void GL_APIENTRY
glGetShaderPrecisionFormat(
    GLenum shadertype,
    GLenum precisiontype,
    GLint *range,
    GLint *precision
    )
{
    GLContext context;

    gcmHEADER_ARG("shardertype=0x%04x precisiontype=0x%04x",
                  shadertype, precisiontype);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETSHADERPRECISIONFORMAT, 0);

    if ((shadertype != GL_VERTEX_SHADER) && (shadertype != GL_FRAGMENT_SHADER))
    {
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    switch (precisiontype)
    {
    case GL_LOW_FLOAT:
    case GL_MEDIUM_FLOAT:
    case GL_HIGH_FLOAT:
        range[0]   = -127;
        range[1]   =  127;
        *precision =  -24;
        break;

    case GL_LOW_INT:
    case GL_MEDIUM_INT:
    case GL_HIGH_INT:
        range[0]   = -24;
        range[1]   =  24;
        *precision =   0;
        break;

    default:
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    gcmDUMP_API("${ES20 glGetShaderPrecisionFormat 0x%08X 0x%08X (0x%08X) (0x%08X)",
                shadertype, precisiontype, range, precision);
    gcmDUMP_API_ARRAY(range, 2);
    gcmDUMP_API_ARRAY(precision, 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*range=%d,%d *precision=%d", range[0], range[1], *precision);
}
