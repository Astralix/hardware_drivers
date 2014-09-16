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

static void _UpdateLighting(
    glsCONTEXT_PTR Context,
    GLint Light,
    GLboolean LightEnable
    )
{
    gcmHEADER_ARG("Context=0x%x Light=%d LightEnable=%d",
                    Context, Light, LightEnable);
    if (LightEnable)
    {
        /* Update light count. */
        if (!Context->lightingStates.lightEnabled[Light])
        {
            Context->lightingStates.lightCount++;
        }
    }
    else
    {
        /* Update light count. */
        if (Context->lightingStates.lightEnabled[Light])
        {
            Context->lightingStates.lightCount--;
        }
    }

    /* Use generic lighting function when there are more
       then 4 lights. */
    Context->lightingStates.useFunction =
        (Context->lightingStates.lightCount > 4);

    gcmFOOTER_NO();
}

static GLenum _SetState(
    glsCONTEXT_PTR Context,
    GLenum State,
    GLboolean Enable
    )
{
    GLenum result = GL_NO_ERROR;

    gcmHEADER_ARG("Context=0x%x State=0x%04x Enable=%d",
                    Context, State, Enable);

    switch (State)
    {
    case GL_STENCIL_TEST:
        result = glfEnableStencilTest(Context, Enable);
        break;
    case GL_TEXTURE_2D:
        result = glfEnableTexturing(Context, Enable);
        break;
    case GL_ALPHA_TEST:
        result = glfEnableAlphaTest(Context, Enable);
        break;

    case GL_BLEND:
        result = glfEnableAlphaBlend(Context, Enable);
        break;

    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    case GL_CLIP_PLANE3:
    case GL_CLIP_PLANE4:
    case GL_CLIP_PLANE5:
        {
            gctUINT index = State - GL_CLIP_PLANE0;
            glmSETHASH_1BIT(hashClipPlaneEnabled, Enable, index);
            Context->clipPlaneEnabled[index] = Enable;
        }
        break;

    case GL_COLOR_LOGIC_OP:
        result = glfEnableLogicOp(Context, Enable);
        break;

    case GL_COLOR_MATERIAL:
        Context->hashKey.hashMaterialEnabled =
        Context->lightingStates.materialEnabled = Enable;
        break;

    case GL_CULL_FACE:
        result = glfEnableCulling(Context, Enable);
        break;

    case GL_DEPTH_TEST:
        result = glfEnableDepthTest(Context, Enable);
        break;

    case GL_DITHER:
        result = glfEnableDither(Context, Enable);
        break;

    case GL_FOG:
        result = glfEnableFog(Context, Enable);
        break;

    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
        {
            gctUINT index = State - GL_LIGHT0;
            _UpdateLighting(Context, index, Enable);
            glmSETHASH_1BIT(hashLightEnabled, Enable, index);
            Context->lightingStates.lightEnabled[index] = Enable;
        }
        break;

    case GL_LIGHTING:
        Context->hashKey.hashLightingEnabled =
        Context->lightingStates.lightingEnabled = Enable;
        break;

    case GL_LINE_SMOOTH:
        Context->lineStates.smooth = Enable;
        break;

    case GL_MATRIX_PALETTE_OES:
        Context->hashKey.hashMatrixPaletteEnabled =
        Context->matrixPaletteEnabled = Enable;
        break;

    case GL_MULTISAMPLE:
        gcmVERIFY_OK(glfEnableMultisampling(Context, Enable));
        break;

    case GL_NORMALIZE:
        Context->hashKey.hashNormalizeNormal =
        Context->normalizeNormal = Enable;
        break;

    case GL_POINT_SMOOTH:
        Context->hashKey.hashPointSmoothEnabled =
        Context->pointStates.smooth = Enable;
        break;

    case GL_POINT_SPRITE_OES:
        result = glfEnablePointSprite(Context, Enable);
        break;

    case GL_POLYGON_OFFSET_FILL:
        result = glfEnablePolygonOffsetFill(Context, Enable);
        break;

    case GL_RESCALE_NORMAL:
        Context->hashKey.hashRescaleNormal =
        Context->rescaleNormal = Enable;
        break;

    case GL_SAMPLE_ALPHA_TO_COVERAGE:
        Context->multisampleStates.alphaToCoverage = Enable;
        break;

    case GL_SAMPLE_ALPHA_TO_ONE:
        Context->multisampleStates.alphaToOne = Enable;
        break;

    case GL_SAMPLE_COVERAGE:
        Context->multisampleStates.coverage = Enable;
        break;

    case GL_SCISSOR_TEST:
        result = glfEnableScissorTest(Context, Enable);
        break;

    case GL_TEXTURE_CUBE_MAP_OES:
        result = glfEnableCubeTexturing(Context, Enable);
        break;

    case GL_TEXTURE_EXTERNAL_OES:
        result = glfEnableExternalTexturing(Context, Enable);
        break;

    case GL_TEXTURE_GEN_STR_OES:
        result = glfEnableCoordGen(Context, Enable);
        break;

    default:
        result = GL_INVALID_ENUM;
        break;
    }

    gcmFOOTER_ARG("return=0x%04x", result);
    /* Return the result. */
    return result;
}

static GLboolean _SetClientState(
    glsCONTEXT_PTR Context,
    GLenum State,
    GLboolean Enable
    )
{
    GLboolean result = GL_TRUE;

    gcmHEADER_ARG("Context=0x%x State=0x%04x Enable=%d",
                    Context, State, Enable);

    switch (State)
    {
    case GL_COLOR_ARRAY:
        Context->hashKey.hashColorStreamEnabled =
        Context->aColorInfo.streamEnabled = Enable;
        break;

    case GL_MATRIX_INDEX_ARRAY_OES:
        Context->hashKey.hashMatrixIndexStreamEnabled =
        Context->aMatrixIndexInfo.streamEnabled = Enable;
        break;

    case GL_NORMAL_ARRAY:
        Context->hashKey.hashNormalStreamEnabled =
        Context->aNormalInfo.streamEnabled = Enable;
        break;

    case GL_POINT_SIZE_ARRAY_OES:
        Context->hashKey.hashPointSizeStreamEnabled =
        Context->aPointSizeInfo.streamEnabled = Enable;
        break;

    case GL_TEXTURE_COORD_ARRAY:
        {
            glsTEXTURESAMPLER_PTR sampler = Context->texture.activeClientSampler;
            glmSETHASH_1BIT(hashTexCoordStreamEnabled, Enable, sampler->index);
            sampler->aTexCoordInfo.streamEnabled = Enable;
        }
        break;

    case GL_VERTEX_ARRAY:
        Context->aPositionInfo.streamEnabled = Enable;
        break;

    case GL_WEIGHT_ARRAY_OES:
        Context->hashKey.hashWeightStreamEnabled =
        Context->aWeightInfo.streamEnabled = Enable;
        break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("return=%d", result);
    /* Return the result. */
    return result;
}


/******************************************************************************\
***************************** State Management Code ****************************
\******************************************************************************/

/*******************************************************************************
**
**  glEnable / glDisable
**
**  glEnableClientState and glDisableClientState enable or disable individual
**  client-side capabilities. By default, all client-side capabilities are
**  disabled. Both glEnableClientState and glDisableClientState take a single
**  argument, array, which can assume one of the following values:
**
**      GL_COLOR_ARRAY
**          If enabled, the color array is enabled for writing and used during
**          rendering when glDrawArrays, or glDrawElements is called.
**          See glColorPointer.
**
**      GL_NORMAL_ARRAY
**          If enabled, the normal array is enabled for writing and used during
**          rendering when glDrawArrays, or glDrawElements is called.
**          See glNormalPointer.
**
**      GL_TEXTURE_COORD_ARRAY
**          If enabled, the texture coordinate array is enabled for writing and
**          used during rendering when glDrawArrays, or glDrawElements is
**          called. See glTexCoordPointer.
**
**      GL_VERTEX_ARRAY
**          If enabled, the vertex array is enabled for writing and used during
**          rendering when glDrawArrays, or glDrawElements is called.
**          See glVertexPointer.
**
**  INPUT:
**
**      State
**          Specifies the capability to enable or disable. Symbolic constants
**          GL_COLOR_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY,
**          and GL_VERTEX_ARRAY are accepted.
**
**  OUTPUT:
**
**      Nothing.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_ENABLE

GL_API void GL_APIENTRY glEnable(
    GLenum State
    )
{
    glmENTER1(glmARGENUM, State)
    {
		gcmDUMP_API("${ES11 glEnable 0x%08X}", State);

		glmPROFILE(context, GLES1_ENABLE, 0);
        glmERROR(_SetState(context, State, GL_TRUE));
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glDisable(
    GLenum State
    )
{
    glmENTER1(glmARGENUM, State)
    {
		gcmDUMP_API("${ES11 glDisable 0x%08X}", State);

        glmPROFILE(context, GLES1_DISABLE, 0);
        glmERROR(_SetState(context, State, GL_FALSE));
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glIsEnabled
**
**  The glIsEnabled function tests whether a capability is enabled.
**
**  INPUT:
**
**      State
**          Specifies the capability to check.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API GLboolean GL_APIENTRY glIsEnabled(
    GLenum State
    )
{
    GLboolean result = GL_FALSE;

    glmENTER1(glmARGENUM, State)
    {
        glmPROFILE(context, GLES1_ISENABLED, 0);
        switch (State)
        {
        case GL_ALPHA_TEST:
            result = context->alphaStates.testEnabled;
            break;

        case GL_BLEND:
            result = context->alphaStates.blendEnabled;
            break;

        case GL_CLIP_PLANE0:
        case GL_CLIP_PLANE1:
        case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3:
        case GL_CLIP_PLANE4:
        case GL_CLIP_PLANE5:
            result = context->clipPlaneEnabled[State - GL_CLIP_PLANE0];
            break;

        case GL_COLOR_ARRAY:
            result = context->aColorInfo.streamEnabled;
            break;

        case GL_COLOR_LOGIC_OP:
            result = context->logicOp.enabled;
            break;

        case GL_COLOR_MATERIAL:
            result = context->lightingStates.materialEnabled;
            break;

        case GL_CULL_FACE:
            result = context->cullStates.enabled;
            break;

        case GL_DEPTH_TEST:
            result = context->depthStates.testEnabled;
            break;

        case GL_DITHER:
            result = context->dither;
            break;

        case GL_FOG:
            result = context->fogStates.enabled;
            break;

        case GL_LIGHT0:
        case GL_LIGHT1:
        case GL_LIGHT2:
        case GL_LIGHT3:
        case GL_LIGHT4:
        case GL_LIGHT5:
        case GL_LIGHT6:
        case GL_LIGHT7:
            result = context->lightingStates.lightEnabled[State - GL_LIGHT0];
            break;

        case GL_LIGHTING:
            result = context->lightingStates.lightingEnabled;
            break;

        case GL_LINE_SMOOTH:
            result = context->lineStates.smooth;
            break;

        case GL_MATRIX_INDEX_ARRAY_OES:
            result = context->aMatrixIndexInfo.streamEnabled;
            break;

        case GL_MATRIX_PALETTE_OES:
            result = context->matrixPaletteEnabled;
            break;

        case GL_MULTISAMPLE:
            result = context->multisampleStates.enabled;
            break;

        case GL_NORMAL_ARRAY:
            result = context->aNormalInfo.streamEnabled;
            break;

        case GL_NORMALIZE:
            result = context->normalizeNormal;
            break;

        case GL_POINT_SIZE_ARRAY_OES:
            result = context->aPointSizeInfo.streamEnabled;
            break;

        case GL_POINT_SMOOTH:
            result = context->pointStates.smooth;
            break;

        case GL_POINT_SPRITE_OES:
            result = context->pointStates.spriteEnable;
            break;

        case GL_POLYGON_OFFSET_FILL:
            result = context->depthStates.polygonOffsetFill;
            break;

        case GL_RESCALE_NORMAL:
            result = context->rescaleNormal;
            break;

        case GL_SAMPLE_ALPHA_TO_COVERAGE:
            result = context->multisampleStates.alphaToCoverage;
            break;

        case GL_SAMPLE_ALPHA_TO_ONE:
            result = context->multisampleStates.alphaToOne;
            break;

        case GL_SAMPLE_COVERAGE:
            result = context->multisampleStates.coverage;
            break;

        case GL_SCISSOR_TEST:
            result = context->viewportStates.scissorTest;
            break;

        case GL_STENCIL_TEST:
            result = context->stencilStates.testEnabled;
            break;

        case GL_TEXTURE_2D:
            result = context->texture.activeSampler->enableTexturing;
            break;

        case GL_TEXTURE_CUBE_MAP_OES:
            result = context->texture.activeSampler->enableCubeTexturing;
            break;

       case GL_TEXTURE_EXTERNAL_OES:
            result = context->texture.activeSampler->enableExternalTexturing;
            break;

        case GL_TEXTURE_GEN_STR_OES:
            result = context->texture.activeSampler->genEnable;
            break;

        case GL_TEXTURE_COORD_ARRAY:
            result = context->texture.activeClientSampler->aTexCoordInfo.streamEnabled;
            break;

        case GL_VERTEX_ARRAY:
            result = context->aPositionInfo.streamEnabled;
            break;

        case GL_WEIGHT_ARRAY_OES:
            result = context->aWeightInfo.streamEnabled;
        }

		gcmDUMP_API("${ES11 glIsEnabled 0x%08X := 0x%08X}", State, result);
	}
    glmLEAVE();

    return result;
}


/*******************************************************************************
**
**  glEnableClientState / glDisableClientState
**
**  glEnableClientState and glDisableClientState enable or disable individual
**  client-side capabilities. By default, all client-side capabilities are
**  disabled. Both glEnableClientState and glDisableClientState take a single
**  argument, array, which can assume one of the following values:
**
**      GL_COLOR_ARRAY
**          If enabled, the color array is enabled for writing and used during
**          rendering when glDrawArrays, or glDrawElements is called.
**          See glColorPointer.
**
**      GL_NORMAL_ARRAY
**          If enabled, the normal array is enabled for writing and used during
**          rendering when glDrawArrays, or glDrawElements is called.
**          See glNormalPointer.
**
**      GL_TEXTURE_COORD_ARRAY
**          If enabled, the texture coordinate array is enabled for writing and
**          used during rendering when glDrawArrays, or glDrawElements is
**          called. See glTexCoordPointer.
**
**      GL_VERTEX_ARRAY
**          If enabled, the vertex array is enabled for writing and used during
**          rendering when glDrawArrays, or glDrawElements is called.
**          See glVertexPointer.
**
**  INPUT:
**
**      State
**          Specifies the capability to enable or disable. Symbolic constants
**          GL_COLOR_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY,
**          and GL_VERTEX_ARRAY are accepted.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glEnableClientState(
    GLenum State
    )
{
    glmENTER1(glmARGENUM, State)
    {
		gcmDUMP_API("${ES11 glEnableClientState 0x%08X}", State);

        glmPROFILE(context, GLES1_ENABLECLIENTSTATE, 0);
        if (!_SetClientState(context, State, GL_TRUE))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}

GL_API void GL_APIENTRY glDisableClientState(
    GLenum State
    )
{
    glmENTER1(glmARGENUM, State)
    {
		gcmDUMP_API("${ES11 glDisableClientState 0x%08X}", State);

        glmPROFILE(context, GLES1_DISABLECLIENTSTATE, 0);
        if (!_SetClientState(context, State, GL_FALSE))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }
    }
    glmLEAVE();
}
