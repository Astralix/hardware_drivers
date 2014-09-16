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

#define _GC_OBJ_ZONE glvZONE_SHADER


/*******************************************************************************
** Vertex Shader generation defines.
*/

#define glmLIGHTING_UNIFORM(Shader, Name, Swizzle, LightIndex) \
    if (LightIndex == -1) \
    { \
        glmUNIFORM_DYNAMIC(Shader, Name, Swizzle, ShaderControl->rLightIndex); \
    } \
    else \
    { \
        glmUNIFORM_STATIC(Shader, Name, Swizzle, LightIndex); \
    }


/*******************************************************************************
** Indexing and swizzle sets used in loops.
*/

static gcSL_INDEXED g_ArrayIndex[] =
{
    gcSL_INDEXED_X,
    gcSL_INDEXED_Y,
    gcSL_INDEXED_Z,
    gcSL_INDEXED_W
};

static gcSL_SWIZZLE g_ArraySwizzle[] =
{
    gcSL_SWIZZLE_XXXX,
    gcSL_SWIZZLE_YYYY,
    gcSL_SWIZZLE_ZZZZ,
    gcSL_SWIZZLE_WWWW
};


/*******************************************************************************
** Vertex Shader internal parameters.
*/

typedef struct _glsVSCONTROL * glsVSCONTROL_PTR;
typedef struct _glsVSCONTROL
{
    /* Pointer to the exposed shader interface. */
    glsSHADERCONTROL_PTR i;

    /* Temporary register and label allocation indices. */
    glmDEFINE_TEMP(rLastAllocated);
    glmDEFINE_LABEL(lLastAllocated);

    /* Temporary result registers that are used in more then one place. */
    glmDEFINE_TEMP(rVtxInEyeSpace);
    glmDEFINE_TEMP(rNrmInEyeSpace)[2];

    /* Lighting. */
    gctINT outputCount;

    glmDEFINE_TEMP(rLighting)[2];
    glmDEFINE_TEMP(rLightIndex);
    gcFUNCTION funcLighting;

    glmDEFINE_TEMP(rVPpli);
    glmDEFINE_TEMP(rVPpliLength);
    glmDEFINE_TEMP(rNdotVPpli)[2];
    glmDEFINE_TEMP(rAttenuation);
    glmDEFINE_TEMP(rSpot);
    glmDEFINE_TEMP(rAmbient);
    glmDEFINE_TEMP(rDiffuse)[2];
    glmDEFINE_TEMP(rSpecular)[2];

    /* Uniforms. */
    glsUNIFORMWRAP_PTR uniforms[glvUNIFORM_VS_COUNT];

    /* Attributes. */
    glsATTRIBUTEWRAP_PTR attributes[glvATTRIBUTE_VS_COUNT];

    /* Varyings. */
    glmDEFINE_TEMP(vPosition);
    glmDEFINE_TEMP(vEyePosition);
    glmDEFINE_TEMP(vColor)[2];
    glmDEFINE_TEMP(vTexCoord)[glvMAX_TEXTURES];
    glmDEFINE_TEMP(vClipPlane)[glvMAX_CLIP_PLANES];
    glmDEFINE_TEMP(vPointSize);
    glmDEFINE_TEMP(vPointFade);
    glmDEFINE_TEMP(vPointSmooth);
}
glsVSCONTROL;


/*******************************************************************************
** Temporary allocation helper.
*/

static gctUINT16 _AllocateTemp(
    glsVSCONTROL_PTR ShaderControl
    )
{
    gctUINT16 result;
    gcmHEADER_ARG("ShaderControl=0x%x", ShaderControl);
    gcmASSERT(ShaderControl->rLastAllocated < 65535);
    result = ++ShaderControl->rLastAllocated;
    gcmFOOTER_ARG("%u", result);
    return result;
}


/*******************************************************************************
** Label allocation helper.
*/

static gctUINT _AllocateLabel(
    glsVSCONTROL_PTR ShaderControl
    )
{
    gctUINT result;
    gcmHEADER_ARG("ShaderControl=0x%x", ShaderControl);
    result = ++ShaderControl->lLastAllocated;
    gcmFOOTER_ARG("%u", result);
    return result;
}


/*******************************************************************************
** Constant setting callbacks.
*/

static gceSTATUS _Set_uColor(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        &Context->aColorInfo.currValue,
        valueArray,
        1
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uNormal(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
          Uniform,
          &Context->aNormalInfo.currValue,
          valueArray,
          1
          );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uModelView(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * 4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromMatrix(
          Uniform,
          Context->modelViewMatrix,
          valueArray,
          1, 4, 4           /* One 4x4 matrix. */
          );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uModelViewInverse3x3Transposed(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[3 * 3];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromMatrix(
          Uniform,
          glfGetModelViewInverse3x3TransposedMatrix(Context),
          valueArray,
          1, 3, 3           /* One 3x3 matrix. */
          );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uModelViewProjection(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * 4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    if (!Context->modelViewProjectionMatrix.dirty)
    {
        status = gcvSTATUS_OK;
        gcmFOOTER();
        return status;
    }

    Context->modelViewProjectionMatrix.dirty = GL_FALSE;

    status = glfSetUniformFromMatrix(
          Uniform,
          glfGetModelViewProjectionMatrix(Context),
          valueArray,
          1, 4, 4           /* One 4x4 matrix. */
          );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uProjection(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * 4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromMatrix(
          Uniform,
          glfGetConvertedProjectionMatrix(Context),
          valueArray,
          1, 4, 4           /* One 4x4 matrix. */
          );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uEcm(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
          Uniform,
          &Context->lightingStates.Ecm,
          valueArray,
          1
          );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uAcm(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
          Uniform,
          &Context->lightingStates.Acm,
          valueArray,
          1
          );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uDcm(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        &Context->lightingStates.Dcm,
        valueArray,
        1
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uAcs(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        &Context->lightingStates.Acs,
        valueArray,
        1
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uSrm(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromMutants(
        Uniform,
        &Context->lightingStates.Srm,
        gcvNULL,
        gcvNULL,
        gcvNULL,
        &valueArray,
        1
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uScm(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        &Context->lightingStates.Scm,
        valueArray,
        1
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uPpli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * glvMAX_LIGHTS];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        Context->lightingStates.Ppli,
        valueArray,
        glvMAX_LIGHTS
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uKi(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[3 * glvMAX_LIGHTS];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromMutants(
        Uniform,
        Context->lightingStates.K0i,
        Context->lightingStates.K1i,
        Context->lightingStates.K2i,
        gcvNULL,
        valueArray,
        glvMAX_LIGHTS
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uSrli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[glvMAX_LIGHTS];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromMutants(
        Uniform,
        Context->lightingStates.Srli,
        gcvNULL,
        gcvNULL,
        gcvNULL,
        valueArray,
        glvMAX_LIGHTS
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uAcli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * glvMAX_LIGHTS];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        Context->lightingStates.Acli,
        valueArray,
        glvMAX_LIGHTS
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uDcli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * glvMAX_LIGHTS];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        Context->lightingStates.Dcli,
        valueArray,
        glvMAX_LIGHTS
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uScli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * glvMAX_LIGHTS];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        Context->lightingStates.Scli,
        valueArray,
        glvMAX_LIGHTS
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uTexMatrix(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    GLint i, x, y;
    gltFRACTYPE valueArray[4 * 4 * glvMAX_TEXTURES];
    gltFRACTYPE* value = valueArray;
    gltFRACTYPE matrix[4 * 4];
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    if (!Context->texture.matrixDirty)
    {
        status = gcvSTATUS_OK;
        gcmFOOTER();
        return status;
    }

    Context->texture.matrixDirty = GL_FALSE;

    for (i = 0; i < Context->texture.pixelSamplers; i++)
    {
        glsMATRIXSTACK_PTR stack =
            &Context->matrixStackArray[glvTEXTURE_MATRIX_0 + i];

        glfGetFromMatrix(
            stack->topMatrix, matrix, glvFRACTYPEENUM
            );

        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                *value++ = matrix[y + x * 4];
            }
        }
    }

    status = gcUNIFORM_SetFracValue(
          Uniform,
          4 * Context->texture.pixelSamplers,
          valueArray
          );

    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uClipPlane(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * glvMAX_CLIP_PLANES];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        Context->clipPlane,
        valueArray,
        glvMAX_CLIP_PLANES
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uPointAttenuation(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        &Context->pointStates.attenuation,
        valueArray,
        1
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uPointSize(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromFractions(
        Uniform,
        glfFracFromMutable(
            Context->aPointSizeInfo.currValue.value[0],
            Context->aPointSizeInfo.currValue.type
            ),
        glfFracFromMutant(&Context->pointStates.clampFrom),
        glfFracFromMutant(&Context->pointStates.clampTo),
        glfFracFromMutant(&Context->pointStates.fadeThrdshold)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uViewport(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromFractions(
        Uniform,

        /* ViewportScaleX */
        glmINT2FRAC(
            Context->viewportStates.viewportBox[2] / 2
            ),

        /* ViewportOriginX */
        glmINT2FRAC(
            Context->viewportStates.viewportBox[0] +
            Context->viewportStates.viewportBox[2] / 2
            ),

        /* ViewportScaleY */
        glmINT2FRAC(
            Context->viewportStates.viewportBox[3] / 2
            ),

        /* ViewportOriginY */
        glmINT2FRAC(
            Context->viewportStates.viewportBox[1] +
            Context->viewportStates.viewportBox[3] / 2
            )
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uMatrixPalette(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gctUINT i, x, y;
    glsMATRIX_PTR paletteMatrix;
    gltFRACTYPE valueArray[4 * 4 * glvMAX_PALETTE_MATRICES];
    gltFRACTYPE* value = valueArray;
    gltFRACTYPE matrix[4 * 4];
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    for (i = 0; i < glvMAX_PALETTE_MATRICES; i += 1)
    {
        /* Get the top matrix. */
        paletteMatrix
            = Context->matrixStackArray[glvPALETTE_MATRIX_0 + i].topMatrix;

        /* Get the matrix values. */
        glfGetFromMatrix(
            paletteMatrix, matrix, glvFRACTYPEENUM
            );

        /* Copy the values in reversed order. */
        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                *value++ = matrix[x * 4 + y];
            }
        }
    }

    status = gcUNIFORM_SetFracValue(
          Uniform,
          4 * glvMAX_PALETTE_MATRICES,
          valueArray
          );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Set_uMatrixPaletteInverse(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gctUINT i, x, y;
    glsDEPENDENTMATRIX_PTR matrixArray;
    gltFRACTYPE valueArray[3 * 3 * glvMAX_PALETTE_MATRICES];
    gltFRACTYPE* value = valueArray;
    gltFRACTYPE matrix[4 * 4];
    gltFRACTYPE* row;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    /* Query the current inverse palette matrix array. */
    matrixArray = glfGetMatrixPaletteInverse(Context);

    for (i = 0; i < glvMAX_PALETTE_MATRICES; i += 1)
    {
        glfGetFromMatrix(
            &matrixArray[i].matrix, matrix, glvFRACTYPEENUM
            );

        /* Transpose the matrix at the same time. */
        for (x = 0; x < 3; x++)
        {
            row = &matrix[x * 4];

            for (y = 0; y < 3; y++)
            {
                *value++ = row[y];
            }
        }
    }

    status = gcUNIFORM_SetFracValue(
          Uniform,
          3 * glvMAX_PALETTE_MATRICES,
          valueArray
          );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Set_uAcmAcli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * glvMAX_LIGHTS];
    glsVECTOR   vAcmAcli[glvMAX_LIGHTS];
    GLfloat     vec[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLint       i;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    for (i = 0; i < glvMAX_LIGHTS; i++)
    {
        if (Context->lightingStates.materialEnabled &&
            !Context->aColorInfo.streamEnabled)
        {
            glfMulVector4(&Context->aColorInfo.currValue,
                          &Context->lightingStates.Acli[i],
                          &vAcmAcli[i]);
        }
        else if (!Context->lightingStates.materialEnabled)
        {
            glfMulVector4(&Context->lightingStates.Acm,
                          &Context->lightingStates.Acli[i],
                          &vAcmAcli[i]);
        }
        else
        {
            glfSetVector4(&vAcmAcli[i], vec, glvFLOAT);
        }
    }

    status = glfSetUniformFromVectors(
          Uniform,
          vAcmAcli,
          valueArray,
          glvMAX_LIGHTS
          );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Set_uVPpli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    GLint       i;
    gltFRACTYPE valueArray[4 * glvMAX_LIGHTS];
    glsVECTOR   vPpli[glvMAX_LIGHTS];
    GLfloat     vec[] = {0.0f, 0.0f, 1.0f, 0.0f};
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    for (i = 0; i < glvMAX_LIGHTS; i++)
    {
        if (Context->lightingStates.Directional[i])
        {
            glfNorm3Vector4f(
                &Context->lightingStates.Ppli[i], &vPpli[i]
                );
        }
        else
        {
            glfSetVector4(&vPpli[i], vec, glvFLOAT);
        }
    }

    status = glfSetUniformFromVectors(
          Uniform,
          vPpli,
          valueArray,
          glvMAX_LIGHTS
          );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Set_uDcmDcli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4 * glvMAX_LIGHTS];
    glsVECTOR   vDcmDcli[glvMAX_LIGHTS];
    GLfloat     vec[] = {0.0f, 0.0f, 0.0f, 1.0f};
    GLint       i;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    for (i = 0; i < glvMAX_LIGHTS; i++)
    {
        if (Context->lightingStates.materialEnabled &&
            !Context->aColorInfo.streamEnabled)
        {
            glfMulVector4(&Context->aColorInfo.currValue,
                          &Context->lightingStates.Dcli[i],
                          &vDcmDcli[i]);
        }
        else if (!Context->lightingStates.materialEnabled)
        {
            glfMulVector4(&Context->lightingStates.Dcm,
                          &Context->lightingStates.Dcli[i],
                          &vDcmDcli[i]);
        }
        else
        {
            glfSetVector4(&vDcmDcli[i], vec, glvFLOAT);
        }
    }

    status = glfSetUniformFromVectors(
          Uniform,
          vDcmDcli,
          valueArray,
          glvMAX_LIGHTS
          );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Set_uCrli(
	glsCONTEXT_PTR Context,
	gcUNIFORM Uniform
	)
{
	gltFRACTYPE valueArray[glvMAX_LIGHTS];

	return glfSetUniformFromMutants(
		Uniform,
        Context->lightingStates.uCrli180,
		gcvNULL,
		gcvNULL,
		gcvNULL,
		valueArray,
		glvMAX_LIGHTS
		);
}

static gceSTATUS _Set_uCosCrli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[glvMAX_LIGHTS];
    glsMUTANT   mCosCrli[glvMAX_LIGHTS];
    GLint       i;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    for (i = 0; i < glvMAX_LIGHTS; i++)
    {
        glfCosMutant(&Context->lightingStates.Crli[i],
                     &mCosCrli[i]);
    }

    status = glfSetUniformFromMutants(
          Uniform,
          mCosCrli,
          gcvNULL,
          gcvNULL,
          gcvNULL,
          valueArray,
          glvMAX_LIGHTS
          );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Set_uNormedSdli(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{

    gltFRACTYPE valueArray[4 * glvMAX_LIGHTS];
    glsVECTOR   vNormedSdli[glvMAX_LIGHTS];
    GLint       i;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    for (i = 0; i < glvMAX_LIGHTS; i++)
    {
        glfNorm3Vector4f(&Context->lightingStates.Sdli[i],
                        &vNormedSdli[i]);
    }

    status = glfSetUniformFromVectors(
          Uniform,
          vNormedSdli,
          valueArray,
          glvMAX_LIGHTS
          );

    gcmFOOTER();

    return status;
}


/*******************************************************************************
** Uniform access helpers.
*/

static gceSTATUS _Using_uColor(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
          ShaderControl->i,
          "uColor",
          gcSHADER_FRAC_X4,
          1,
          _Set_uColor,
          &Context->vsUniformDirty.uColorDirty,
          &glmUNIFORM_WRAP(VS, uColor)
          );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uNormal(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uNormal",
        gcSHADER_FRAC_X3,
        1,
        _Set_uNormal,
        &Context->vsUniformDirty.uNormalDirty,
        &glmUNIFORM_WRAP(VS, uNormal)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uModelView(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uModelView",
        gcSHADER_FRAC_X4,
        4,
        _Set_uModelView,
        &Context->vsUniformDirty.uModelViewDirty,
        &glmUNIFORM_WRAP(VS, uModelView)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uModelViewInverse3x3Transposed(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uModelViewInverse3x3Transposed",
        gcSHADER_FRAC_X3,
        3,
        _Set_uModelViewInverse3x3Transposed,
        &Context->vsUniformDirty.uModelViewInverse3x3TransposedDirty,
        &glmUNIFORM_WRAP(VS, uModelViewInverse3x3Transposed)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uModelViewProjection(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uModelViewProjection",
        gcSHADER_FRAC_X4,
        4,
        _Set_uModelViewProjection,
        &Context->vsUniformDirty.uModeViewProjectionDirty,
        &glmUNIFORM_WRAP(VS, uModelViewProjection)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uProjection(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uProjection",
        gcSHADER_FRAC_X4,
        4,
        _Set_uProjection,
        &Context->vsUniformDirty.uProjectionDirty,
        &glmUNIFORM_WRAP(VS, uProjection)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uEcm(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uEcm",
        gcSHADER_FRAC_X4,
        1,
        _Set_uEcm,
        &Context->vsUniformDirty.uEcmDirty,
        &glmUNIFORM_WRAP(VS, uEcm)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uAcm(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uAcm",
        gcSHADER_FRAC_X4,
        1,
        _Set_uAcm,
        &Context->vsUniformDirty.uAcmDirty,
        &glmUNIFORM_WRAP(VS, uAcm)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uDcm(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uDcm",
        gcSHADER_FRAC_X4,
        1,
        _Set_uDcm,
        &Context->vsUniformDirty.uDcmDirty,
        &glmUNIFORM_WRAP(VS, uDcm)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uAcs(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uAcs",
        gcSHADER_FRAC_X4,
        1,
        _Set_uAcs,
        &Context->vsUniformDirty.uAcsDirty,
        &glmUNIFORM_WRAP(VS, uAcs)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uSrm(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uSrm",
        gcSHADER_FRAC_X1,
        1,
        _Set_uSrm,
        &Context->vsUniformDirty.uSrmDirty,
        &glmUNIFORM_WRAP(VS, uSrm)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uScm(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uScm",
        gcSHADER_FRAC_X4,
        1,
        _Set_uScm,
        &Context->vsUniformDirty.uScmDirty,
        &glmUNIFORM_WRAP(VS, uScm)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uPpli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uPpli",
        gcSHADER_FRAC_X4,
        glvMAX_LIGHTS,
        _Set_uPpli,
        &Context->vsUniformDirty.uPpliDirty,
        &glmUNIFORM_WRAP(VS, uPpli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uKi(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uKi",
        gcSHADER_FRAC_X3,
        glvMAX_LIGHTS,
        _Set_uKi,
        &Context->vsUniformDirty.uKiDirty,
        &glmUNIFORM_WRAP(VS, uKi)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uSrli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uSrli",
        gcSHADER_FRAC_X1,
        glvMAX_LIGHTS,
        _Set_uSrli,
        &Context->vsUniformDirty.uSrliDirty,
        &glmUNIFORM_WRAP(VS, uSrli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uAcli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uAcli",
        gcSHADER_FRAC_X4,
        glvMAX_LIGHTS,
        _Set_uAcli,
        &Context->vsUniformDirty.uAcliDirty,
        &glmUNIFORM_WRAP(VS, uAcli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uDcli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uDcli",
        gcSHADER_FRAC_X4,
        glvMAX_LIGHTS,
        _Set_uDcli,
        &Context->vsUniformDirty.uDcliDirty,
        &glmUNIFORM_WRAP(VS, uDcli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uScli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uScli",
        gcSHADER_FRAC_X4,
        glvMAX_LIGHTS,
        _Set_uScli,
        &Context->vsUniformDirty.uScliDirty,
        &glmUNIFORM_WRAP(VS, uScli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uAcmAcli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uAcmAcli",
        gcSHADER_FRAC_X4,
        glvMAX_LIGHTS,
        _Set_uAcmAcli,
        &Context->vsUniformDirty.uAcmAcliDirty,
        &glmUNIFORM_WRAP(VS, uAcmAcli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uVPpli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uVPpli",
        gcSHADER_FRAC_X4,
        glvMAX_LIGHTS,
        _Set_uVPpli,
        &Context->vsUniformDirty.uVPpliDirty,
        &glmUNIFORM_WRAP(VS, uVPpli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uDcmDcli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uDcmDcli",
        gcSHADER_FRAC_X4,
        glvMAX_LIGHTS,
        _Set_uDcmDcli,
        &Context->vsUniformDirty.uDcmDcliDirty,
        &glmUNIFORM_WRAP(VS, uDcmDcli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uCrli(
	glsCONTEXT_PTR Context,
	glsVSCONTROL_PTR ShaderControl
	)
{
	return glfUsingUniform(
		ShaderControl->i,
		"uCrli",
		gcSHADER_FRAC_X1,
		glvMAX_LIGHTS,
		_Set_uCrli,
        &Context->vsUniformDirty.uCrli180Dirty,
		&glmUNIFORM_WRAP(VS, uCrli)
		);
}

static gceSTATUS _Using_uCosCrli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uCosCrli",
        gcSHADER_FRAC_X1,
        glvMAX_LIGHTS,
        _Set_uCosCrli,
        &Context->vsUniformDirty.uCosCrliDirty,
        &glmUNIFORM_WRAP(VS, uCosCrli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uNormedSdli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uNormedSdli",
        gcSHADER_FRAC_X4,
        glvMAX_LIGHTS,
        _Set_uNormedSdli,
        &Context->vsUniformDirty.uNormedSdliDirty,
        &glmUNIFORM_WRAP(VS, uNormedSdli)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uTexCoord(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsing_uTexCoord(
        ShaderControl->i,
        &Context->vsUniformDirty.uTexCoordDirty,
        &glmUNIFORM_WRAP(VS, uTexCoord)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uTexMatrix(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uTexMatrix",
        gcSHADER_FRAC_X4,
        4 * Context->texture.pixelSamplers,
        _Set_uTexMatrix,
        &Context->vsUniformDirty.uTexMatrixDirty,
        &glmUNIFORM_WRAP(VS, uTexMatrix)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uClipPlane(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uClipPlane",
        gcSHADER_FRAC_X4,
        glvMAX_CLIP_PLANES,
        _Set_uClipPlane,
        &Context->vsUniformDirty.uClipPlaneDirty,
        &glmUNIFORM_WRAP(VS, uClipPlane)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uPointAttenuation(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uPointAttenuation",
        gcSHADER_FRAC_X4,
        1,
        _Set_uPointAttenuation,
        &Context->vsUniformDirty.uPointAttenuationDirty,
        &glmUNIFORM_WRAP(VS, uPointAttenuation)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uPointSize(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uPointSize",
        gcSHADER_FRAC_X4,
        1,
        _Set_uPointSize,
        &Context->vsUniformDirty.uPointSizeDirty,
        &glmUNIFORM_WRAP(VS, uPointSize)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uViewport(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uViewport",
        gcSHADER_FRAC_X4,
        1,
        _Set_uViewport,
        &Context->vsUniformDirty.uViewportDirty,
        &glmUNIFORM_WRAP(VS, uViewport)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uMatrixPalette(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uMatrixPalette",
        gcSHADER_FRAC_X4,
        4 * glvMAX_PALETTE_MATRICES,
        _Set_uMatrixPalette,
        &Context->vsUniformDirty.uMatrixPaletteDirty,
        &glmUNIFORM_WRAP(VS, uMatrixPalette)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uMatrixPaletteInverse(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingUniform(
        ShaderControl->i,
        "uMatrixPaletteInverse",
        gcSHADER_FRAC_X3,
        3 * glvMAX_PALETTE_MATRICES,
        _Set_uMatrixPaletteInverse,
        &Context->vsUniformDirty.uMatrixPaletteInverseDirty,
        &glmUNIFORM_WRAP(VS, uMatrixPaletteInverse)
        );
    gcmFOOTER();
    return status;
}


/*******************************************************************************
** Attribute access helpers.
*/

static gceSTATUS _Using_aPosition(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    glsATTRIBUTEINFO_PTR info;
    gctINT binding;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);


    /* Define a shortcut to the attribute descriptor. */
    if (Context->drawClearRectEnabled)
    {
        info = &Context->aPositionDrawClearRectInfo;
        binding = gldATTRIBUTE_DRAWCLEAR_POSITION;
    }
    else if (Context->drawTexOESEnabled)
    {
        info = &Context->aPositionDrawTexInfo;
        binding = gldATTRIBUTE_DRAWTEX_POSITION;
    }
    else
    {
        info = &Context->aPositionInfo;
        binding = gldATTRIBUTE_POSITION;
    }

    status = glfUsingAttribute(
        ShaderControl->i,
        "aPosition",
        info->attributeType,
        1,
        gcvFALSE,
        info,
        &glmATTRIBUTE_WRAP(VS, aPosition),
        binding
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_aNormal(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    glsATTRIBUTEINFO_PTR info = &Context->aNormalInfo;

    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingAttribute(
        ShaderControl->i,
        "aNormal",
        info->attributeType,
        1,
        gcvFALSE,
        info,
        &glmATTRIBUTE_WRAP(VS, aNormal),
        gldATTRIBUTE_NORMAL
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_aColor(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    glsATTRIBUTEINFO_PTR info = &Context->aColorInfo;

    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingAttribute(
        ShaderControl->i,
        "aColor",
        info->attributeType,
        1,
        gcvFALSE,
        info,
        &glmATTRIBUTE_WRAP(VS, aColor),
        gldATTRIBUTE_COLOR
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_aTexCoord(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT Sampler
    )
{
    static gctCONST_STRING aName[] =
    {
        "aTexCoord0",
        "aTexCoord1",
        "aTexCoord2",
        "aTexCoord3"
    };

    /* Make a shortcut to the sampler. */
    glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[Sampler];

    /* Make a shortcut to the attribute descriptor. */
    glsATTRIBUTEINFO_PTR info = Context->drawTexOESEnabled
        ? &sampler->aTexCoordDrawTexInfo
        : &sampler->aTexCoordInfo;

    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=%d", Context, ShaderControl, Sampler);

    status = glfUsingAttribute(
        ShaderControl->i,
        aName[Sampler],
        info->attributeType,
        1,
        gcvTRUE,
        info,
        &glmATTRIBUTE_WRAP_INDEXED(VS, aTexCoord0, Sampler),
        (Context->drawTexOESEnabled
        ? (gldATTRIBUTE_DRAWTEX_TEXCOORD0 + Sampler)
        : (gldATTRIBUTE_TEXCOORD0 + Sampler))
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_aPointSize(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    glsATTRIBUTEINFO_PTR info = &Context->aPointSizeInfo;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    status = glfUsingAttribute(
        ShaderControl->i,
        "aPointSize",
        info->attributeType,
        1,
        gcvFALSE,
        info,
        &glmATTRIBUTE_WRAP(VS, aPointSize),
        gldATTRIBUTE_POINT_SIZE
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_aMatrixIndex(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    glsATTRIBUTEINFO_PTR info = &Context->aMatrixIndexInfo;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    status = glfUsingAttribute(
        ShaderControl->i,
        "aMatrixIndex",
        info->attributeType,
        1,
        gcvFALSE,
        info,
        &glmATTRIBUTE_WRAP(VS, aMatrixIndex),
        gldATTRIBUTE_INDEX
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_aMatrixWeight(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    glsATTRIBUTEINFO_PTR info = &Context->aWeightInfo;

    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    status = glfUsingAttribute(
        ShaderControl->i,
        "aMatrixWeight",
        info->attributeType,
        1,
        gcvFALSE,
        info,
        &glmATTRIBUTE_WRAP(VS, aMatrixWeight),
        gldATTRIBUTE_WEIGHT
        );
    gcmFOOTER();
    return status;
}


/*******************************************************************************
** Output access helpers.
*/

static gceSTATUS _Assign_vPosition(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctUINT16 TempRegister
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x TempRegister=%u", Context, ShaderControl, TempRegister);
    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        "#Position",
        gcSHADER_FLOAT_X4,
        1,
        TempRegister
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Assign_vEyePosition(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctUINT16 TempRegister
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x TempRegister=%u", Context, ShaderControl, TempRegister);
    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        "vEyePosition",
        gcSHADER_FLOAT_X1,
        1,
        TempRegister
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Assign_vColor(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT OutputIndex
    )
{
    static gctCONST_STRING vName[] =
    {
        "vColor0",
        "vColor1"
    };

    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x OutputIndex=%d", Context, ShaderControl, OutputIndex);
    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        vName[OutputIndex],
        gcSHADER_FLOAT_X4,
        1,
        ShaderControl->vColor[OutputIndex]
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Assign_vTexCoord(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT Sampler
    )
{
    static gctCONST_STRING vName[] =
    {
        "vTexCoord0",
        "vTexCoord1",
        "vTexCoord2",
        "vTexCoord3"
    };
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=%d", Context, ShaderControl, Sampler);

    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        vName[Sampler],
        Context->texture.sampler[Sampler].coordType,
        1,
        ShaderControl->vTexCoord[Sampler]
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Assign_vClipPlane(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT ClipPlane
    )
{
    static gctCONST_STRING vName[] =
    {
        "vClipPlane0",
        "vClipPlane1",
        "vClipPlane2",
        "vClipPlane3",
        "vClipPlane4",
        "vClipPlane5"
    };
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x ClipPlane=%d", Context, ShaderControl, ClipPlane);

    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        vName[ClipPlane],
        gcSHADER_FLOAT_X1,
        1,
        ShaderControl->vClipPlane[ClipPlane]
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Assign_vPointSize(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctUINT16 TempRegister
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x TempRegister=%u", Context, ShaderControl, TempRegister);

    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        "#PointSize",
        gcSHADER_FLOAT_X1,
        1,
        TempRegister
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Assign_vPointFade(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctUINT16 TempRegister
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x TempRegister=%u", Context, ShaderControl, TempRegister);

    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        "vPointFade",
        gcSHADER_FLOAT_X1,
        1,
        TempRegister
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Assign_vPointSmooth(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctUINT16 TempRegister
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x TempRegister=%u", Context, ShaderControl, TempRegister);

    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        "vPointSmooth",
        gcSHADER_FLOAT_X3,
        1,
        TempRegister
        );

    gcmFOOTER();
    return status;
}


/*******************************************************************************
*   Check special case to expand VS shader to compute lighting
*   even if light source number greater than 4.
*/
static gctBOOL _checkExpandLighting(
    glsCONTEXT_PTR Context
    )
{
    GLint   i;
    gctBOOL bForceExpand = gcvTRUE;

    gcmHEADER_ARG("Context=0x%x", Context);

    if (Context->lightingStates.lightCount <= 4)
    {
        gcmFOOTER_ARG("%d", gcvFALSE);
        return gcvFALSE;
    }

    for (i = 0; i < glvMAX_LIGHTS; i++)
    {
        if (Context->lightingStates.lightEnabled[i])
        {
            if (!Context->lightingStates.Directional[i])
            {
                bForceExpand = gcvFALSE;
                break;
            }
        }
    }

    gcmFOOTER_ARG("%d", bForceExpand);
    return bForceExpand;
}


/*******************************************************************************
**
**  _Pos2Eye
**
**  Transform the incoming position to the eye space coordinates using matrix
**  palette.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _Pos2Eye(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Already computed? */
        if (ShaderControl->rVtxInEyeSpace != 0)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Compute using matrix palette? */
        if (Context->matrixPaletteEnabled)
        {
            GLint i, count;

            /* Allocate temporaries. */
            glmALLOCATE_LOCAL_TEMP(temp1);

            /* Allocate attributes. */
            glmUSING_ATTRIBUTE(aPosition);
            glmUSING_ATTRIBUTE(aMatrixIndex);
            glmUSING_ATTRIBUTE(aMatrixWeight);

            /* Allocate palette matrices. */
            glmUSING_UNIFORM(uMatrixPalette);

            /* Convert indices to matrix row offsets: */
            /* temp1.x = aMatrixIndex[0] * 4 */
            /* temp1.y = aMatrixIndex[1] * 4 */
            /* temp1.z = aMatrixIndex[2] * 4 */
            glmOPCODE(MUL, temp1, XYZ);
                glmATTRIBUTE(VS, aMatrixIndex, XYZZ);
                glmCONST(4);

            /* Determine the number of iterations. */
            count = gcmMAX(
                Context->aMatrixIndexInfo.components,
                Context->aWeightInfo.components
                );

            gcmASSERT((count >= 1) && (count <= glvMAX_VERTEX_UNITS));

            /* Transform using matrix palette. */
            for (i = 0; i < count; i++)
            {
                /* Allocate temporaries. */
                glmALLOCATE_LOCAL_TEMP(temp2);
                glmALLOCATE_LOCAL_TEMP(temp3);

                /* dp4 temp2.x, aPosition, uMatrixPalette[index + 0] */
                glmOPCODE(DP4, temp2, X);
                    glmATTRIBUTE(VS, aPosition, XYZW);
                    glmUNIFORM_DYNAMIC_MATRIX(VS, uMatrixPalette, XYZW, temp1, 0, g_ArrayIndex[i]);

                /* dp4 temp2.y, aPosition, uMatrixPalette[index + 1] */
                glmOPCODE(DP4, temp2, Y);
                    glmATTRIBUTE(VS, aPosition, XYZW);
                    glmUNIFORM_DYNAMIC_MATRIX(VS, uMatrixPalette, XYZW, temp1, 1, g_ArrayIndex[i]);

                /* dp4 temp2.z, aPosition, uMatrixPalette[index + 2] */
                glmOPCODE(DP4, temp2, Z);
                    glmATTRIBUTE(VS, aPosition, XYZW);
                    glmUNIFORM_DYNAMIC_MATRIX(VS, uMatrixPalette, XYZW, temp1, 2, g_ArrayIndex[i]);

                /* dp4 temp2.w, aPosition, uMatrixPalette[index + e] */
                glmOPCODE(DP4, temp2, W);
                    glmATTRIBUTE(VS, aPosition, XYZW);
                    glmUNIFORM_DYNAMIC_MATRIX(VS, uMatrixPalette, XYZW, temp1, 3, g_ArrayIndex[i]);

                /* temp3 = temp2 * aMatrixWeight[i] */
                glmOPCODE(MUL, temp3, XYZW);
                    glmTEMP(temp2, XYZW);
                    glmATTRIBUTEV(VS, aMatrixWeight, g_ArraySwizzle[i]);

                /* Assume the result of the current pass if the accumulator
                   is not yet allocated. */
                if (ShaderControl->rVtxInEyeSpace == 0)
                {
                    ShaderControl->rVtxInEyeSpace = temp3;
                }
                else
                {
                    /* Allocate a new temp. */
                    glmALLOCATE_LOCAL_TEMP(temp4);

                    /* temp4 = rVtxInEyeSpace + temp3 */
                    glmOPCODE(ADD, temp4, XYZW);
                        glmTEMP(ShaderControl->rVtxInEyeSpace, XYZW);
                        glmTEMP(temp3, XYZW);

                    /* Update result register. */
                    ShaderControl->rVtxInEyeSpace = temp4;
                }
            }
        }
        else
        {
            /* Allocate a temporary for the vertex in eye space. */
            glmALLOCATE_TEMP(ShaderControl->rVtxInEyeSpace);

            /* Allocate position attribute. */
            glmUSING_ATTRIBUTE(aPosition);

            if (Context->modelViewMatrix->identity)
            {
                /* rVtxInEyeSpace = aPosition */
                glmOPCODE(MOV, ShaderControl->rVtxInEyeSpace, XYZW);
                    glmATTRIBUTE(VS, aPosition, XYZW);
            }
            else
            {
                /* Allocate resources. */
                glmUSING_UNIFORM(uModelView);

                /* dp4 rVtxInEyeSpace.x, aPosition, uModelView[0] */
                glmOPCODE(DP4, ShaderControl->rVtxInEyeSpace, X);
                    glmATTRIBUTE(VS, aPosition, XYZW);
                    glmUNIFORM_STATIC(VS, uModelView, XYZW, 0);

                /* dp4 rVtxInEyeSpace.y, aPosition, uModelView[1] */
                glmOPCODE(DP4, ShaderControl->rVtxInEyeSpace, Y);
                    glmATTRIBUTE(VS, aPosition, XYZW);
                    glmUNIFORM_STATIC(VS, uModelView, XYZW, 1);

                /* dp4 rVtxInEyeSpace.z, aPosition, uModelView[2] */
                glmOPCODE(DP4, ShaderControl->rVtxInEyeSpace, Z);
                    glmATTRIBUTE(VS, aPosition, XYZW);
                    glmUNIFORM_STATIC(VS, uModelView, XYZW, 2);

                /* dp4 rVtxInEyeSpace.w, aPosition, uModelView[3] */
                glmOPCODE(DP4, ShaderControl->rVtxInEyeSpace, W);
                    glmATTRIBUTE(VS, aPosition, XYZW);
                    glmUNIFORM_STATIC(VS, uModelView, XYZW, 3);
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _Pos2ClipWithPalette
**
**  Transform the incoming position to the clip coordinate space using matrix
**  palette.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _Pos2ClipWithPalette(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Already computed? */
        if (ShaderControl->vPosition != 0)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Compute position in eye space coordinates. */
        gcmERR_BREAK(_Pos2Eye(Context, ShaderControl));

        /* Allocate position register. */
        glmALLOCATE_TEMP(ShaderControl->vPosition);

        /* Allocate projection matrix. */
        glmUSING_UNIFORM(uProjection);

        /* dp4 vPosition.x, rVtxInEyeSpace, uProjection[0] */
        glmOPCODE(DP4, ShaderControl->vPosition, X);
            glmTEMP(ShaderControl->rVtxInEyeSpace, XYZW);
            glmUNIFORM_STATIC(VS, uProjection, XYZW, 0);

        /* dp4 vPosition.y, rVtxInEyeSpace, uProjection[1] */
        glmOPCODE(DP4, ShaderControl->vPosition, Y);
            glmTEMP(ShaderControl->rVtxInEyeSpace, XYZW);
            glmUNIFORM_STATIC(VS, uProjection, XYZW, 1);

        /* dp4 vPosition.z, rVtxInEyeSpace, uProjection[2] */
        glmOPCODE(DP4, ShaderControl->vPosition, Z);
            glmTEMP(ShaderControl->rVtxInEyeSpace, XYZW);
            glmUNIFORM_STATIC(VS, uProjection, XYZW, 2);

        /* dp4 vPosition.w, rVtxInEyeSpace, uProjection[3] */
        glmOPCODE(DP4, ShaderControl->vPosition, W);
            glmTEMP(ShaderControl->rVtxInEyeSpace, XYZW);
            glmUNIFORM_STATIC(VS, uProjection, XYZW, 3);
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _Pos2ClipWithModelViewProjection
**
**  Transform the incoming position to the clip coordinate space using the
**  product of model view and projection matrices.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _Pos2ClipWithModelViewProjection(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Already computed? */
        if (ShaderControl->vPosition != 0)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Allocate a temporary for the position output. */
        glmALLOCATE_TEMP(ShaderControl->vPosition);

        /* Allocate position attribute. */
        glmUSING_ATTRIBUTE(aPosition);

        /* Allocate projection matrix. */
        glmUSING_UNIFORM(uModelViewProjection);

        /* dp4 vPosition.x, aPosition, uModelViewProjection[0] */
        glmOPCODE(DP4, ShaderControl->vPosition, X);
            glmATTRIBUTE(VS, aPosition, XYZW);
            glmUNIFORM_STATIC(VS, uModelViewProjection, XYZW, 0);

        /* dp4 vPosition.y, aPosition, uModelViewProjection[1] */
        glmOPCODE(DP4, ShaderControl->vPosition, Y);
            glmATTRIBUTE(VS, aPosition, XYZW);
            glmUNIFORM_STATIC(VS, uModelViewProjection, XYZW, 1);

        /* dp4 vPosition.z, aPosition, uModelViewProjection[2] */
        glmOPCODE(DP4, ShaderControl->vPosition, Z);
            glmATTRIBUTE(VS, aPosition, XYZW);
            glmUNIFORM_STATIC(VS, uModelViewProjection, XYZW, 2);

        /* dp4 vPosition.w, aPosition, uModelViewProjection[3] */
        glmOPCODE(DP4, ShaderControl->vPosition, W);
            glmATTRIBUTE(VS, aPosition, XYZW);
            glmUNIFORM_STATIC(VS, uModelViewProjection, XYZW, 3);
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _Pos2Clip
**
**  Transform the incoming position to the clip coordinate space.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _Pos2Clip(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Already computed? */
        if (ShaderControl->vPosition != 0)
        {
            status = gcvSTATUS_OK;
            break;
        }

        if (Context->drawTexOESEnabled || Context->drawClearRectEnabled)
        {
            /* Allocate a temporary for the position output. */
            glmALLOCATE_TEMP(ShaderControl->vPosition);

            /* Allocate position attribute. */
            glmUSING_ATTRIBUTE(aPosition);

            /* vPosition = aPosition */
            glmOPCODE(MOV, ShaderControl->vPosition, XYZW);
                glmATTRIBUTE(VS, aPosition, XYZW);
        }
        else if (Context->matrixPaletteEnabled)
        {
            status = _Pos2ClipWithPalette(Context, ShaderControl);
        }
        else
        {
            status = _Pos2ClipWithModelViewProjection(Context, ShaderControl);
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _Normal2EyeWithPalette
**
**  Transform the incoming normal to the eye space coordinates using matrix
**  palette.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _Normal2EyeWithPalette(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        GLint i, count;

        /* Allocate temporaries. */
        glmDEFINE_TEMP(temp1);
        glmDEFINE_TEMP(temp2);

        /* Already computed? */
        if (ShaderControl->rNrmInEyeSpace[0] != 0)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Determine the number of iterations. */
        count = gcmMAX(
            Context->aMatrixIndexInfo.components,
            Context->aWeightInfo.components
            );

        gcmASSERT((count >= 1) && (count <= glvMAX_VERTEX_UNITS));

        /* Allocate temporaries. */
        glmALLOCATE_TEMP(temp1);
        glmALLOCATE_TEMP(temp2);

        /* Allocate attributes. */
        glmUSING_ATTRIBUTE(aMatrixIndex);
        glmUSING_ATTRIBUTE(aMatrixWeight);

        /* Allocate palette matrices. */
        glmUSING_UNIFORM(uMatrixPaletteInverse);

        /* temp1.x = aMatrixIndex[0] * 3 */
        /* temp1.y = aMatrixIndex[1] * 3 */
        /* temp1.z = aMatrixIndex[2] * 3 */
        glmOPCODE(MUL, temp1, XYZ);
            glmATTRIBUTE(VS, aMatrixIndex, XYZZ);
            glmCONST(3);

        /* Normal comes from the stream? */
        if (Context->aNormalInfo.streamEnabled)
        {
            /* Allocate resources. */
            glmUSING_ATTRIBUTE(aNormal);

            /* temp2 = aNormal */
            glmOPCODE(MOV, temp2, XYZ);
                glmATTRIBUTE(VS, aNormal, XYZZ);
        }

        /* Normal comes from the constant. */
        else
        {
            /* Allocate resources. */
            glmUSING_UNIFORM(uNormal);

            /* temp2 = uNormal */
            glmOPCODE(MOV, temp2, XYZ);
                glmUNIFORM(VS, uNormal, XYZZ);
        }

        /* Transform using matrix palette. */
        for (i = 0; i < count; i++)
        {
            /* Allocate temporaries. */
            glmALLOCATE_LOCAL_TEMP(temp3);
            glmALLOCATE_LOCAL_TEMP(temp4);

            /* temp3.x = dot(temp2, uMatrixPaletteInverse[index + 0]) */
            glmOPCODE(DP3, temp3, X);
                glmTEMP(temp2, XYZZ);
                glmUNIFORM_DYNAMIC_MATRIX(VS, uMatrixPaletteInverse, XYZZ, temp1, 0, g_ArrayIndex[i]);

            /* temp3.y = dot(temp2, uMatrixPaletteInverse[index + 1]) */
            glmOPCODE(DP3, temp3, Y);
                glmTEMP(temp2, XYZZ);
                glmUNIFORM_DYNAMIC_MATRIX(VS, uMatrixPaletteInverse, XYZZ, temp1, 1, g_ArrayIndex[i]);

            /* temp3.z = dot(temp2, uMatrixPaletteInverse[index + 2]) */
            glmOPCODE(DP3, temp3, Z);
                glmTEMP(temp2, XYZZ);
                glmUNIFORM_DYNAMIC_MATRIX(VS, uMatrixPaletteInverse, XYZZ, temp1, 2, g_ArrayIndex[i]);

            /* temp4 = temp3 * aMatrixWeight[i] */
            glmOPCODE(MUL, temp4, XYZ);
                glmTEMP(temp3, XYZZ);
                glmATTRIBUTEV(VS, aMatrixWeight, g_ArraySwizzle[i]);

            /* Assume the result of the current pass if the accumulator
               is not yet allocated. */
            if (ShaderControl->rNrmInEyeSpace[0] == 0)
            {
                ShaderControl->rNrmInEyeSpace[0] = temp4;
            }
            else
            {
                /* Allocate a new temp. */
                glmALLOCATE_LOCAL_TEMP(temp5);

                /* temp5 = rNrmInEyeSpace + temp4 */
                glmOPCODE(ADD, temp5, XYZ);
                    glmTEMP(ShaderControl->rNrmInEyeSpace[0], XYZZ);
                    glmTEMP(temp4, XYZZ);

                /* Update result register. */
                ShaderControl->rNrmInEyeSpace[0] = temp5;
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _Normal2EyeWithModelViewInv
**
*   Transform normal to eye space using model-view inverse matrix.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _Normal2EyeWithModelViewInv(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Allocate a temp. */
        glmALLOCATE_LOCAL_TEMP(temp);

        /* Normal comes from the stream? */
        if (Context->aNormalInfo.streamEnabled)
        {
            /* Allocate normal attribute. */
            glmUSING_ATTRIBUTE(aNormal);

            /* temp = aNormal */
            glmOPCODE(MOV, temp, XYZ);
                glmATTRIBUTE(VS, aNormal, XYZZ);
        }

        /* Normal comes from the constant. */
        else
        {
            /* Allocate normal uniform. */
            glmUSING_UNIFORM(uNormal);

            /* temp = uNormal */
            glmOPCODE(MOV, temp, XYZ);
                glmUNIFORM(VS, uNormal, XYZZ);
        }

        if (glfGetModelViewInverse3x3TransposedMatrix(Context)->identity)
        {
            /* Set the result. */
            ShaderControl->rNrmInEyeSpace[0] = temp;
        }
        else
        {
            /* Allocate the result. */
            glmALLOCATE_TEMP(ShaderControl->rNrmInEyeSpace[0]);

            /* Allocate matrix uniform. */
            glmUSING_UNIFORM(uModelViewInverse3x3Transposed);

            /* rNrmInEyeSpace[0].x = dot(temp, uModelViewInverse3x3Transposed[0]) */
            glmOPCODE(DP3, ShaderControl->rNrmInEyeSpace[0], X);
                glmTEMP(temp, XYZZ);
                glmUNIFORM_STATIC(VS, uModelViewInverse3x3Transposed, XYZZ, 0);

            /* rNrmInEyeSpace[0].y = dot(temp, uModelViewInverse3x3Transposed[1]) */
            glmOPCODE(DP3, ShaderControl->rNrmInEyeSpace[0], Y);
                glmTEMP(temp, XYZZ);
                glmUNIFORM_STATIC(VS, uModelViewInverse3x3Transposed, XYZZ, 1);

            /* rNrmInEyeSpace[0].z = dot(temp, uModelViewInverse3x3Transposed[2]) */
            glmOPCODE(DP3, ShaderControl->rNrmInEyeSpace[0], Z);
                glmTEMP(temp, XYZZ);
                glmUNIFORM_STATIC(VS, uModelViewInverse3x3Transposed, XYZZ, 2);
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _Normal2Eye
**
**  Transform normal to eye space.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _Normal2Eye(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Already computed? */
        if (ShaderControl->rNrmInEyeSpace[0])
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Matrix palette enabled? */
        if (Context->matrixPaletteEnabled)
        {
            gcmERR_BREAK(_Normal2EyeWithPalette(Context, ShaderControl));
        }
        else
        {
            gcmERR_BREAK(_Normal2EyeWithModelViewInv(Context, ShaderControl));
        }

        /* Rescale normal. */
        if (Context->rescaleNormal &&
            !glfGetModelViewInverse3x3TransposedMatrix(Context)->identity)
        {
            /* Allocate temporaries. */
            glmALLOCATE_LOCAL_TEMP(temp1);
            glmALLOCATE_LOCAL_TEMP(temp2);

            /* Save current normal value. */
            glmDEFINE_TEMP(prevNrmInEyeSpace) = ShaderControl->rNrmInEyeSpace[0];

            /* Allocate new normal register. */
            glmALLOCATE_TEMP(ShaderControl->rNrmInEyeSpace[0]);

            /* Allocate resources. */
            glmUSING_UNIFORM(uModelViewInverse3x3Transposed);

            /* temp1.x = m31^2 + m32^2 + m33^2 */
            glmOPCODE(DP3, temp1, X);
                glmUNIFORM_STATIC(VS, uModelViewInverse3x3Transposed, XYZZ, 2);
                glmUNIFORM_STATIC(VS, uModelViewInverse3x3Transposed, XYZZ, 2);

            /* temp2.x = scale factor */
            glmOPCODE(RSQ, temp2, X);
                glmTEMP(temp1, XXXX);

            /* Rescale the normal. */
            glmOPCODE(MUL, ShaderControl->rNrmInEyeSpace[0], XYZ);
                glmTEMP(prevNrmInEyeSpace, XYZZ);
                glmTEMP(temp2, XXXX);
        }

        /* Normalize the normal. */
        if (Context->normalizeNormal)
        {
            /* Save current normal value. */
            glmDEFINE_TEMP(prevNrmInEyeSpace) = ShaderControl->rNrmInEyeSpace[0];

            /* Allocate new normal register. */
            glmALLOCATE_TEMP(ShaderControl->rNrmInEyeSpace[0]);

            /* rNrmInEyeSpace[0] = norm(rNrmInEyeSpace[0]) */
            glmOPCODE(NORM, ShaderControl->rNrmInEyeSpace[0], XYZ);
                glmTEMP(prevNrmInEyeSpace, XYZZ);
        }

        /* Compute the negated normal for two-sided lighting. */
        if (ShaderControl->outputCount == 2)
        {
            glmALLOCATE_TEMP(ShaderControl->rNrmInEyeSpace[1]);

            /* Compute the reversed normal. */
            glmOPCODE(SUB, ShaderControl->rNrmInEyeSpace[1], XYZ);
                glmCONST(0);
                glmTEMP(ShaderControl->rNrmInEyeSpace[0], XYZZ);
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _ComputePointSize
**
**  Compute point size. Per OpenGL ES spec:
**
**                                     1
**  derived_size' = size * ------------------------- = size * scale_factor
**                         SQRT(a + b * d + c * d^2)
**
**  derived_size = clamp(derived_size')
**
**  where a, b, and c - distance attenuation function coefficients;
**        d           - eye-coordinate distance from the eye to the vertex;
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _ComputePointSize(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Define a label. */
        glmDEFINE_LABEL(lblZero);

        /* Define temporaries. */
        glmDEFINE_TEMP(temp1);
        glmDEFINE_TEMP(temp2);
        glmDEFINE_TEMP(temp3);
        glmDEFINE_TEMP(temp4);
        glmDEFINE_TEMP(temp5);
        glmDEFINE_TEMP(temp6);

        /* Allocate the label. */
        glmALLOCATE_LABEL(lblZero);

        /* Compute current position in eye space. */
        gcmERR_BREAK(_Pos2Eye(Context, ShaderControl));

        /* Allocate temporaries. */
        glmALLOCATE_TEMP(temp1);
        glmALLOCATE_TEMP(temp2);
        glmALLOCATE_TEMP(temp3);
        glmALLOCATE_TEMP(temp4);
        glmALLOCATE_TEMP(temp5);
        glmALLOCATE_TEMP(temp6);

        /* Allocate resources. */
        glmUSING_UNIFORM(uPointAttenuation);
        glmUSING_UNIFORM(uPointSize);

        /* temp1 = (1, d, d^2) */
        {
            /* temp1.z = d^2 */
            glmOPCODE(DP3, temp1, YZ);
                glmTEMP(ShaderControl->rVtxInEyeSpace, XYZZ);
                glmTEMP(ShaderControl->rVtxInEyeSpace, XYZZ);

            /* Make sure Z is not zero, otherwize RSQ will produce an INF. */
            glmOPCODE_BRANCH(JMP, EQUAL, lblZero);
                glmTEMP(temp1, ZZZZ);
                glmCONST(0);

            {
                /* temp1.y = 1 / sqrt(d^2) */
                glmOPCODE(RSQ, temp2, Y);
                    glmTEMP(temp1, ZZZZ);

                /* temp2.z = temp1.z */
                glmOPCODE(MOV, temp2, Z);
                    glmTEMP(temp1, ZZZZ);

                /* temp1.y = d */
                glmOPCODE(MUL, temp1, Y);
                    glmTEMP(temp2, YYYY);
                    glmTEMP(temp2, ZZZZ);
            }

            /* Define label. */
            glmLABEL(lblZero);

            /* temp1.x = 1 */
            glmOPCODE(MOV, temp1, X);
                glmCONST(1);
        }

        /* temp2.x = a + b * d + c * d^2 */
        glmOPCODE(DP3, temp2, X);
            glmUNIFORM(VS, uPointAttenuation, XYZZ);
            glmTEMP(temp1, XYZZ);

        /* temp3.x = scale_factor */
        glmOPCODE(RSQ, temp3, X);
            glmTEMP(temp2, XXXX);

        /* Point size is comming from the stream. */
        if (Context->aPointSizeInfo.streamEnabled)
        {
            glmUSING_ATTRIBUTE(aPointSize);

            /* temp4.x = derived_size' */
            glmOPCODE(MUL, temp4, X);
                glmATTRIBUTE(VS, aPointSize, XXXX);
                glmTEMP(temp3, XXXX);
        }

        /* Point size is a constant value. */
        else
        {
            /* temp4.x = derived_size' */
            glmOPCODE(MUL, temp4, X);
                glmUNIFORM(VS, uPointSize, XXXX);
                glmTEMP(temp3, XXXX);
        }

        /* Clamp by the lower boundary (uPointSize.y). */
        glmOPCODE(MAX, temp5, X);
            glmUNIFORM(VS, uPointSize, YYYY);
            glmTEMP(temp4, XXXX);

        /* Clamp by the upper boundary (uPointSize.z) = derived_size. */
        glmOPCODE(MIN, temp6, X);
            glmUNIFORM(VS, uPointSize, ZZZZ);
            glmTEMP(temp5, XXXX);

#if USE_SUPER_SAMPLING
        /* Special patch for GC500 with super-sampling enabled:
           multiply point size by 2. */
        if (Context->chipModel == gcv500)
        {
            gctUINT samples;

            if (gcmIS_SUCCESS(gcoSURF_GetSamples(Context->draw, &samples))
            &&  (samples > 1)
            )
            {
                /* temp6 *= 2 */
                glmOPCODE(MUL, temp6, X);
                    glmTEMP(temp6, XXXX);
                    glmCONST(2);
            }
        }
#endif

        if (Context->multisampleStates.enabled)
        {
            /* Allocate temporaries. */
            glmALLOCATE_LOCAL_TEMP(temp7);
            glmALLOCATE_LOCAL_TEMP(temp8);
            glmALLOCATE_LOCAL_LABEL(lblDontFade);

            /* Allocate varyings. */
            glmALLOCATE_TEMP(ShaderControl->vPointSize);
            glmALLOCATE_TEMP(ShaderControl->vPointFade);

            /* Compare against the threshold. */
            glmOPCODE(MAX, ShaderControl->vPointSize, X);
                glmUNIFORM(VS, uPointSize, WWWW);
                glmTEMP(temp6, XXXX);

            /* Initialize fade factor to 1. */
            glmOPCODE(MOV, ShaderControl->vPointFade, X);
                glmCONST(1);

            /* if (fadeThrdshold <= derived_size) goto lblDontFade. */
            glmOPCODE_BRANCH(JMP, LESS_OR_EQUAL, lblDontFade);
                glmUNIFORM(VS, uPointSize, WWWW);
                glmTEMP(temp6, XXXX);

            {
                /* temp7.x = 1 / fadeThrdshold. */
                glmOPCODE(RCP, temp7, X);
                    glmUNIFORM(VS, uPointSize, WWWW);

                /* temp8.x = derived_size / fadeThrdshold. */
                glmOPCODE(MUL, temp8, X);
                    glmTEMP(temp6, XXXX);
                    glmTEMP(temp7, XXXX);

                /* vPointFade.x = (derived_size / fadeThrdshold) ^ 2. */
                glmOPCODE(MUL, ShaderControl->vPointFade, X);
                    glmTEMP(temp8, XXXX);
                    glmTEMP(temp8, XXXX);
            }

            /* Define label. */
            glmLABEL(lblDontFade);
        }
        else
        {
            /* Assign point size varying. */
            ShaderControl->vPointSize = temp6;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _ComputePointSmooth
**
**  Compute point smooth.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _ComputePointSmooth(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Allocate temporaries. */
        glmALLOCATE_LOCAL_TEMP(temp1);
        glmALLOCATE_LOCAL_TEMP(temp2);
        glmALLOCATE_LOCAL_TEMP(temp3);

        glmALLOCATE_LOCAL_TEMP(temp4);
        glmALLOCATE_LOCAL_TEMP(temp5);
        /* Allocate point smooth varying. */
        glmALLOCATE_TEMP(ShaderControl->vPointSmooth);

        /* Allocate resources. */
        glmUSING_UNIFORM(uViewport);

        /* Do W correction for position. */
        glmOPCODE(RCP, temp1, X);
            glmTEMP(ShaderControl->vPosition, WWWW);

        glmOPCODE(MUL, temp2, XY);
            glmTEMP(temp1, XXXX);
            glmTEMP(ShaderControl->vPosition, XYYY);

        /* PointScreenX (vPointSmooth.x)
              = PositionX * ViewportScaleX + ViewportOriginX */
        glmOPCODE(MUL, temp3, X);
            glmTEMP(temp2, XXXX);
            glmUNIFORM(VS, uViewport, XXXX);

        glmOPCODE(ADD, ShaderControl->vPointSmooth, X);
            glmTEMP(temp3, XXXX);
            glmUNIFORM(VS, uViewport, YYYY);

        /* PointScreenY (vPointSmooth.y)
              = PositionY * ViewportScaleY + ViewportOriginY */
        glmOPCODE(MUL, temp4, X);
            glmTEMP(temp2, YYYY);
            glmUNIFORM(VS, uViewport, ZZZZ);

        glmOPCODE(ADD, ShaderControl->vPointSmooth, Y);
            glmTEMP(temp4, XXXX);
            glmUNIFORM(VS, uViewport, WWWW);

        /* temp3.x = the radius of the point. */
        glmOPCODE(MUL, temp5, X);
            glmTEMP(ShaderControl->vPointSize, XXXX);
            glmCONST(0.5f);

        /* vPointSmooth.z = radius^2. */
        glmOPCODE(MUL, ShaderControl->vPointSmooth, Z);
            glmTEMP(temp5, XXXX);
            glmTEMP(temp5, XXXX);
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightGeneral
**
**  Generate general lighting component of the lighting formula: object emission
**  and general ambient lighting. Per OpenGL ES spec:
**
**      c = Ecm + Acm * Acs,
**
**  where Ecm - emissive color of material;
**        Acm - ambient color of material;
**        Acs - ambient color of scene.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
*/

static gceSTATUS _lightGeneral(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /***************************************************************************
        ** Per OpenGL ES spec:
        **   When color material tracking is enabled, by glEnable with
        **   COLOR_MATERIAL, the ambient (Acm) property of both the front and back
        **   material are immediately set to the value of the current color, and
        **   will track changes to the current color resulting from either the
        **   glColor commands or drawing vertex arrays with the color array enabled.
        */

        /* Allocate function result register. */
        glmALLOCATE_TEMP(ShaderControl->rLighting[0]);

        /***********************************************************************
        ** BLOCK1: Acs == 0
        */
        if (Context->lightingStates.Acs.zero3)
        {
            /*******************************************************************
            ** BLOCK2: Ecm == 0, (Acs == 0)
            */
            if (Context->lightingStates.Ecm.zero3)
            {
                /* rLighting[0] = 0 */
                glmOPCODE(MOV, ShaderControl->rLighting[0], XYZ);
                    glmCONST(0);
            }
            /*
            ** BLOCK2: Ecm == 0, (Acs == 0)
            *******************************************************************/

            /*******************************************************************
            ** BLOCK3: Ecm != 0, (Acs == 0)
            */
            else
            {
                /* Allocate resources. */
                glmUSING_UNIFORM(uEcm);

                /* rLighting[0] = Ecm */
                glmOPCODE(MOV, ShaderControl->rLighting[0], XYZ);
                    glmUNIFORM(VS, uEcm, XYZZ);
            }
            /*
            ** BLOCK3: Ecm != 0, (Acs == 0)
            *******************************************************************/
        }
        /*
        ** BLOCK1: Acs == 0
        ***********************************************************************/

        /***********************************************************************
        ** BLOCK4: Acs != 0
        */
        else
        {
            /*******************************************************************
            ** BLOCK5: Ecm == 0, (Acs != 0)
            */
            if (Context->lightingStates.Ecm.zero3)
            {
                /***************************************************************
                ** BLOCK6: Acm = Color, ((Ecm == 0) and (Acs != 0)).
                */
                if (Context->lightingStates.materialEnabled)
                {
                    /* Color from stream? */
                    if (Context->aColorInfo.streamEnabled)
                    {
                        /* Allocate resources. */
                        glmUSING_UNIFORM(uAcs);
                        glmUSING_ATTRIBUTE(aColor);

                        /* rLighting[0] = aColor * Acs */
                        glmOPCODE(MUL, ShaderControl->rLighting[0], XYZ);
                            glmATTRIBUTE(VS, aColor, XYZZ);
                            glmUNIFORM(VS, uAcs, XYZZ);
                    }

                    /* Color from a constant. */
                    else
                    {
                        /* Color == 0 ? */
                        if (Context->aColorInfo.currValue.zero3)
                        {
                            /* rLighting[0] = 0 */
                            glmOPCODE(MOV, ShaderControl->rLighting[0], XYZ);
                                glmCONST(0);
                        }
                        else
                        {
                            /* Allocate a temp. */
                            glmALLOCATE_LOCAL_TEMP(temp);

                            /* Allocate resources. */
                            glmUSING_UNIFORM(uAcs);
                            glmUSING_UNIFORM(uColor);

                            /* temp = uColor */
                            glmOPCODE(MOV, temp, XYZ);
                                glmUNIFORM(VS, uColor, XYZZ);

                            /* rLighting[0] = temp * Acs */
                            glmOPCODE(MUL, ShaderControl->rLighting[0], XYZ);
                                glmTEMP(temp, XYZZ);
                                glmUNIFORM(VS, uAcs, XYZZ);
                        }
                    }
                }
                /*
                ** BLOCK6: Acm = Color, ((Ecm == 0) and (Acs != 0)).
                ***************************************************************/

                /***************************************************************
                ** BLOCK7: Acm = as is, ((Ecm == 0) and (Acs != 0)).
                */
                else
                {
                    /* (Ecm == 0) and (Acm == 0) and (Acs != 0) */
                    if (Context->lightingStates.Acm.zero3)
                    {
                        /* rLighting[0] = 0 */
                        glmOPCODE(MOV, ShaderControl->rLighting[0], XYZ);
                            glmCONST(0);
                    }

                    /* (Ecm == 0) and (Acm != 0) and (Acs != 0) */
                    else
                    {
                        /* Allocate a temp. */
                        glmALLOCATE_LOCAL_TEMP(temp);

                        /* Allocate resources. */
                        glmUSING_UNIFORM(uAcm);
                        glmUSING_UNIFORM(uAcs);

                        /* temp = uAcm */
                        glmOPCODE(MOV, temp, XYZ);
                            glmUNIFORM(VS, uAcm, XYZZ);

                        /* rLighting[0] = temp * Acs */
                        glmOPCODE(MUL, ShaderControl->rLighting[0], XYZ);
                            glmTEMP(temp, XYZZ);
                            glmUNIFORM(VS, uAcs, XYZZ);
                    }
                }
                /*
                ** BLOCK7: Acm = as is, ((Ecm == 0) and (Acs != 0)).
                ***************************************************************/
            }
            /*
            ** BLOCK5: Ecm == 0, (Acs != 0)
            *******************************************************************/

            /*******************************************************************
            ** BLOCK8: Ecm != 0, (Acs != 0)
            */
            else
            {
                /***************************************************************
                ** BLOCK9: Acm = Color, ((Ecm != 0) and (Acs != 0)).
                */
                if (Context->lightingStates.materialEnabled)
                {
                    /* Color from stream? */
                    if (Context->aColorInfo.streamEnabled)
                    {
                        /* Allocate a temp. */
                        glmALLOCATE_LOCAL_TEMP(temp);

                        /* Allocate resources. */
                        glmUSING_UNIFORM(uEcm);
                        glmUSING_UNIFORM(uAcs);
                        glmUSING_ATTRIBUTE(aColor);

                        /* temp = aColor * Acs */
                        glmOPCODE(MUL, temp, XYZ);
                            glmATTRIBUTE(VS, aColor, XYZZ);
                            glmUNIFORM(VS, uAcs, XYZZ);

                        /* rLighting[0] = Ecm + temp */
                        glmOPCODE(ADD, ShaderControl->rLighting[0], XYZ);
                            glmUNIFORM(VS, uEcm, XYZZ);
                            glmTEMP(temp, XYZZ);
                    }

                    /* Color from a constant. */
                    else
                    {
                        /* Color == 0 ? */
                        if (Context->aColorInfo.currValue.zero3)
                        {
                            /* Allocate resources. */
                            glmUSING_UNIFORM(uEcm);

                            /* rLighting[0] = Ecm */
                            glmOPCODE(MOV, ShaderControl->rLighting[0], XYZ);
                                glmUNIFORM(VS, uEcm, XYZZ);
                        }
                        else
                        {
                            /* Allocate temporaries. */
                            glmALLOCATE_LOCAL_TEMP(temp1);
                            glmALLOCATE_LOCAL_TEMP(temp2);

                            /* Allocate resources. */
                            glmUSING_UNIFORM(uEcm);
                            glmUSING_UNIFORM(uAcs);
                            glmUSING_UNIFORM(uColor);

                            /* temp1 = uColor */
                            glmOPCODE(MOV, temp1, XYZ);
                                glmUNIFORM(VS, uColor, XYZZ);

                            /* temp2 = temp1 * Acs */
                            glmOPCODE(MUL, temp2, XYZ);
                                glmTEMP(temp1, XYZZ);
                                glmUNIFORM(VS, uAcs, XYZZ);

                            /* rLighting[0] = Ecm + temp2 */
                            glmOPCODE(ADD, ShaderControl->rLighting[0], XYZ);
                                glmUNIFORM(VS, uEcm, XYZZ);
                                glmTEMP(temp2, XYZZ);
                        }
                    }
                }
                /*
                ** BLOCK9: Acm = Color, ((Ecm != 0) and (Acs != 0)).
                ***************************************************************/

                /***************************************************************
                ** BLOCK10: Acm = as is, ((Ecm != 0) and (Acs != 0)).
                */
                else
                {
                    /* (Ecm != 0) and (Acm == 0) and (Acs != 0) */
                    if (Context->lightingStates.Acm.zero3)
                    {
                        /* Allocate resources. */
                        glmUSING_UNIFORM(uEcm);

                        /* rLighting[0] = Ecm */
                        glmOPCODE(MOV, ShaderControl->rLighting[0], XYZ);
                            glmUNIFORM(VS, uEcm, XYZZ);
                    }

                    /* (Ecm != 0) and (Acm != 0) and (Acs != 0) */
                    else
                    {
                        /* Allocate temporaries. */
                        glmALLOCATE_LOCAL_TEMP(temp1);
                        glmALLOCATE_LOCAL_TEMP(temp2);

                        /* Allocate resources. */
                        glmUSING_UNIFORM(uAcm);
                        glmUSING_UNIFORM(uAcs);
                        glmUSING_UNIFORM(uEcm);

                        /* temp1 = uAcm */
                        glmOPCODE(MOV, temp1, XYZ);
                            glmUNIFORM(VS, uAcm, XYZZ);

                        /* temp2 = temp1 * Acs */
                        glmOPCODE(MUL, temp2, XYZ);
                            glmTEMP(temp1, XYZZ);
                            glmUNIFORM(VS, uAcs, XYZZ);

                        /* rLighting[0] = Ecm + temp2 */
                        glmOPCODE(ADD, ShaderControl->rLighting[0], XYZ);
                            glmUNIFORM(VS, uEcm, XYZZ);
                            glmTEMP(temp2, XYZZ);
                    }
                }
                /*
                ** BLOCK10: Acm = as is, ((Ecm != 0) and (Acs != 0)).
                ***************************************************************/
            }
            /*
            ** BLOCK8: Ecm != 0, (Acs != 0)
            *******************************************************************/
        }
        /*
        ** BLOCK4: (Acs != 0)
        ***********************************************************************/

        /* Allocate resources. */
        glmUSING_UNIFORM(uDcm);

        /* Accordingly to OpenGL ES spec, alpha component comes from alpha value
           associated with Dcm. */
        glmOPCODE(MOV, ShaderControl->rLighting[0], W);
            glmUNIFORM(VS, uDcm, WWWW);

        /* Copy the value to the second color output for two-sided lighting. */
        if (ShaderControl->outputCount == 2)
        {
            /* Allocate the second color output. */
            glmALLOCATE_TEMP(ShaderControl->rLighting[1]);

            /* Copy the value. */
            glmOPCODE(MOV, ShaderControl->rLighting[1], XYZW);
                glmTEMP(ShaderControl->rLighting[0], XYZW);
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightDetermineVPpli
**
**  Generate VPpli vector as it is used in multiple places within the
**  lighting formula. Per OpenGL ES spec:
**
**      Let VPpli be the unit vector that points from point V to point Ppli.
**      Note that:
**          - if V has a zero w coordinate, results of lighting are
**            undefined accordingly to the OpenGL ES spec;
**          - if V has a non-zero w coordinate and Ppli has a zero w
**            coordinate, then VPpli is the unit vector corresponding to the
**            direction specified by the x, y, and z coordinates of Ppli;
**          - if V and Ppli both have non-zero w coordinates, then VPpli
**            is the unit vector obtained by normalizing the direction
**            corresponding to (Ppli - V).
**
**  The notes can be interpreted as:
**
**      if (V.w == 0)
**      {
**          // Results are undefined.
**      }
**
**      else
**      {
**          if (P.w == 0)
**          {
**              VPpli = (Ppli.x, Ppli.y, Ppli.z)
**          }
**          else
**          {
**              VPpli = (Ppli.x - V.x, Ppli.y - V.y, Ppli.z - V.z)
**          }
**      }
**
**  The pseudo-code can be rewritten as:
**
**      VPpli = Ppli - V * Ppli.w'
**      Ppli.w' = (Ppli.w == 0)? 0 : 1
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Current light.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _lightDetermineVPpli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d", Context, ShaderControl, LightIndex);

    do
    {
        if (ShaderControl->rVPpli == 0)
        {
            /* Allocate local resources. */
            glmALLOCATE_LOCAL_TEMP(temp1);
            glmALLOCATE_LOCAL_TEMP(temp2);
            glmALLOCATE_LOCAL_TEMP(temp3);

            /* Allocate the result registers. */
            glmALLOCATE_TEMP(ShaderControl->rVPpli);
            glmALLOCATE_TEMP(ShaderControl->rVPpliLength);

            /* Allocate resources. */
            glmUSING_UNIFORM(uPpli);
            glmUSING_UNIFORM(uVPpli);

            /*******************************************************************
            ** Determine VPpli vector.
            */

            /* Set preliminary VPpli vector value:
               temp1 = Ppli[LightIndex]. */
            glmOPCODE(MOV, temp1, XYZW);
                glmLIGHTING_UNIFORM(VS, uPpli, XYZW, LightIndex);

            /* For generic function we have to put condition in place. */
            if (LightIndex < 0)
            {
                /* Define a label. */
                glmDEFINE_LABEL(lblDirectional);

                /* Compute current position in eye space. */
                gcmERR_BREAK(_Pos2Eye(Context, ShaderControl));

                /* Allocate a label. */
                glmALLOCATE_LABEL(lblDirectional);

                /* if (temp1.w == 0) goto lblDirectional. */
                glmOPCODE_BRANCH(JMP, EQUAL, lblDirectional);
                    glmTEMP(temp1, WWWW);
                    glmCONST(0);

                {
                    /* Compute VPpli:
                       temp1 = temp1 - V. */
                    glmOPCODE(MOV, temp2, XYZW);
                        glmTEMP(temp1, XYZW);
                    glmOPCODE(SUB, temp1, XYZ);
                        glmTEMP(temp2, XYZZ);
                        glmTEMP(ShaderControl->rVtxInEyeSpace, XYZZ);
                }

                /* Define label. */
                glmLABEL(lblDirectional);
            }
            else
            {
                if (!Context->lightingStates.Directional[LightIndex])
                {
                    /* Compute current position in eye space. */
                    gcmERR_BREAK(_Pos2Eye(Context, ShaderControl));

                    /* Compute VPpli:
                       temp1 = temp1 - V. */
                    glmOPCODE(MOV, temp2, XYZW);
                        glmTEMP(temp1, XYZW);
                    glmOPCODE(SUB, temp1, XYZ);
                        glmTEMP(temp2, XYZZ);
                        glmTEMP(ShaderControl->rVtxInEyeSpace, XYZZ);
                }
            }

            if ((LightIndex >= 0) &&
                Context->lightingStates.Directional[LightIndex])
            {
                    glmOPCODE(MOV, ShaderControl->rVPpli, XYZ);
                        glmLIGHTING_UNIFORM(VS, uVPpli, XYZZ, LightIndex);
            }
            else
            {
                /***************************************************************
                ** Normalize VPpli vector (make it into a unit vector).
                **
                **                VPpli
                ** VPpli.norm = ---------
                **              ||VPpli||
                **
                */

                /* Dot product of the same vector gives a square of its length. */
                glmOPCODE(DP3, temp2, X);
                    glmTEMP(temp1, XYZZ);
                    glmTEMP(temp1, XYZZ);

                /* Compute 1 / ||VPpli|| */
                glmOPCODE(RSQ, temp3, X);
                    glmTEMP(temp2, XXXX);

                /* Normalize the vector. */
                glmOPCODE(MUL, ShaderControl->rVPpli, XYZ);
                    glmTEMP(temp1, XYZZ);
                    glmTEMP(temp3, XXXX);

                /* Compute the length of the vector. */
                glmOPCODE(MUL, ShaderControl->rVPpliLength, X);
                    glmTEMP(temp2, XXXX);
                    glmTEMP(temp3, XXXX);
            }
        }
        else
        {
            status = gcvSTATUS_OK;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightNormDotVPpli
**
**  Generate the dot product between the norm vector and VPpli vector as it is
**  used in multiple places within the lighting formula.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Current light.
**
**      OutputIndex
**          Color output index (0 or 1).
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _lightNormDotVPpli(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex,
    gctINT OutputIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d OutputIndex=%d",
                    Context, ShaderControl, LightIndex, OutputIndex);

    do
    {
        if (ShaderControl->rNdotVPpli[OutputIndex] == 0)
        {
            /* Define a temporary. */
            glmDEFINE_TEMP(temp);

            /* Transform normal to eye space. */
            gcmERR_BREAK(_Normal2Eye(
                Context, ShaderControl
                ));

            /* Determnine VPpli unit vector. */
            gcmERR_BREAK(_lightDetermineVPpli(
                Context, ShaderControl, LightIndex
                ));

            /* Allocate temporaries. */
            glmALLOCATE_TEMP(temp);
            glmALLOCATE_TEMP(ShaderControl->rNdotVPpli[OutputIndex]);

            /* temp.x = dot(n, VPpli) */
            glmOPCODE(DP3, temp, X);
                glmTEMP(ShaderControl->rNrmInEyeSpace[OutputIndex], XYZZ);
                glmTEMP(ShaderControl->rVPpli, XYZZ);

            /* If normal is normalized, "dp3 normal, vp" will be in [-1, 1]
               Use SAT to replace MAX, and expect back-end optimise it. */
            if (Context->normalizeNormal)
            {
                /* rNdotVPpli[OutputIndex] = sat(dot(n, VPpli), 0). */
                glmOPCODE(SAT, ShaderControl->rNdotVPpli[OutputIndex], X);
                    glmTEMP(temp, XXXX);
            }
            else
            {
                /* rNdotVPpli[OutputIndex] = max(dot(n, VPpli), 0). */
                glmOPCODE(MAX, ShaderControl->rNdotVPpli[OutputIndex], X);
                    glmTEMP(temp, XXXX);
                    glmCONST(0);
            }
        }
        else
        {
            status = gcvSTATUS_OK;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightAttenuation
**
**  Generate light attenuation factor. Per OpenGL ES spec:
**
**  If Ppli's w != 0:
**
**                                1
**      ATTi = --------------------------------------------
**             K0i + K1i * ||VPpli|| + K2i * ||VPpli|| ** 2
**
**  If Ppli's w == 0:
**
**      ATTi = 1
**
**  where K0i, K1i, K2i - user-defined constants associated with the light;
**        V             - current vertex;
**        Ppli          - position of light i;
**        ||VPpli||     - distance between the vertex and the light.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Current light.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _lightAttenuation(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d",
                    Context, ShaderControl, LightIndex);

    do
    {
        /* Per-light optimization. */
        if (
                (
                    (LightIndex >= 0) &&

                    (
                        /* Attenuation = 1 for directional lights. */
                        Context->lightingStates.Directional[LightIndex] ||

                        /* Attenuation = 1 if K0i == 1 and K1i == 0 and K2i == 0. */
                        (Context->lightingStates.K0i[LightIndex].one  &&
                         Context->lightingStates.K1i[LightIndex].zero &&
                         Context->lightingStates.K2i[LightIndex].zero)
                    )
                )

                /* Already computed. */
                || (ShaderControl->rAttenuation != 0)
            )
        {
            status = gcvSTATUS_OK;
        }
        else
        {
            glmDEFINE_TEMP(temp1);
            glmDEFINE_TEMP(temp2);

            /* Determnine VPpli unit vector. */
            gcmERR_BREAK(_lightDetermineVPpli(Context, ShaderControl, LightIndex));

            /* Allocate temporaries. */
            glmALLOCATE_TEMP(temp1);
            glmALLOCATE_TEMP(temp2);
            glmALLOCATE_TEMP(ShaderControl->rAttenuation);

            /* Allocate resources. */
            glmUSING_UNIFORM(uKi);

            /* temp1.x = 1 */
            glmOPCODE(MOV, temp1, X);
                glmCONST(1);

            /* temp1.y = len(VPpli) */
            glmOPCODE(MOV, temp1, Y);
                glmTEMP(ShaderControl->rVPpliLength, XXXX);

            /* temp1.z = len(VPpli)^2 */
            glmOPCODE(MUL, temp1, Z);
                glmTEMP(ShaderControl->rVPpliLength, XXXX);
                glmTEMP(ShaderControl->rVPpliLength, XXXX);

            /* temp2.x = K0i + K1i * ||VPpli|| + K2i * ||VPpli||^2 */
            glmOPCODE(DP3, temp2, X);
                glmLIGHTING_UNIFORM(VS, uKi, XYZZ, LightIndex);
                glmTEMP(temp1, XYZZ);

            /* rAttenuation = rcp(temp2.x) */
            glmOPCODE(RCP, ShaderControl->rAttenuation, X);
                glmTEMP(temp2, XXXX);
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightSpot
**
**  Generate light spot factor. Per OpenGL ES spec:
**
**  If (Crli != 180) and (PpliV dot S'dli >= cos(Crli)):
**
**      SPOTi = (PpliV dot S'dli) ** Srli
**
**  If (Crli != 180) and (PpliV dot S'dli < cos(Crli)):
**
**      SPOTi = 0
**
**  If (Crli == 180):
**
**      SPOTi = 1
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Current light.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _lightSpot(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d",
                    Context, ShaderControl, LightIndex);

    do
    {
        /* Per-light optimization. */
        if (
                (
                    (LightIndex >= 0) &&

                    /* Spot = 1, if Crli == 180. */
                    Context->lightingStates.Crli180[LightIndex]
                )

                /* Already computed. */
                || (ShaderControl->rSpot != 0)
           )
        {
            status = gcvSTATUS_OK;
        }
        else
        {
            /* Define a label. */
            glmDEFINE_LABEL(lblEnd);

            /* Define temporaries. */
            glmDEFINE_TEMP(temp1);
            glmDEFINE_TEMP(temp2);
            glmDEFINE_TEMP(temp3);
            glmDEFINE_TEMP(temp4);
            glmDEFINE_TEMP(temp5);

            /* Determnine VPpli unit vector. */
            gcmERR_BREAK(_lightDetermineVPpli(Context, ShaderControl, LightIndex));

            /* Allocate the label. */
            glmALLOCATE_LABEL(lblEnd);

            /* Allocate the temporary registers. */
            glmALLOCATE_TEMP(temp1);
            glmALLOCATE_TEMP(temp2);
            glmALLOCATE_TEMP(temp3);
            glmALLOCATE_TEMP(temp4);
            glmALLOCATE_TEMP(temp5);
            glmALLOCATE_TEMP(ShaderControl->rSpot);

            /* Allocate resources. */
            glmUSING_UNIFORM(uNormedSdli);
            glmUSING_UNIFORM(uCosCrli);
            glmUSING_UNIFORM(uSrli);

            /***************************************************************************
            ** Hardware cannot compute exponents with an arbitrary base. We have to do
            ** a transformation to be able to compute the formula. By definition:
            **           logB(A)
            **      A = B
            ** substitute B with 2, we get:
            **           log2(A)
            **      A = 2
            ** raise both parts of the equation to the power of Srli, we get:
            **       Srli    Srli * log2(A)
            **      A     = 2
            ** where:
            **      A = PpliV dot S'dli.
            ** to summarize:
            **                       Srli    Srli * log2(PpliV dot S'dli)
            **      (PpliV dot S'dli)     = 2
            */

            /* Get normalized S'dli.  */
            glmOPCODE(MOV, temp1, XYZ);
                glmLIGHTING_UNIFORM(VS, uNormedSdli, XYZZ, LightIndex);

            /*
            **  Compute dot(PpliV, S'dli) and cos(Crli).
            */

            if (LightIndex < 0)
            {
                /* Define temporaries. */
                glmDEFINE_TEMP(temp6);

                /* Allocate the temporary registers. */
                glmALLOCATE_TEMP(temp6);

                /* Allocate resources. */
                glmUSING_UNIFORM(uCrli);

	    		/* rSpot = 1 */
    			glmOPCODE(MOV, ShaderControl->rSpot, X);
		    		glmCONST(1);

			    /* temp2 = 1 */
			    glmOPCODE(MOV, temp6, X);
				    glmCONST(1);

                /*
                    CASE : if Crli is 180, rSpot = 1.0
                */
			    glmOPCODE_BRANCH(JMP, EQUAL, lblEnd);
				    glmTEMP(temp6, XXXX);
				    glmLIGHTING_UNIFORM(VS, uCrli, XXXX, LightIndex);
            }

            /* Negate VPli. */
            glmOPCODE(SUB, temp2, XYZ);
                glmCONST(0);
                glmTEMP(ShaderControl->rVPpli, XYZZ);

            /* dot(PpliV, S'dli). */
            glmOPCODE(DP3, temp3, X);
                glmTEMP(temp1, XYZZ);
                glmTEMP(temp2, XYZZ);

            /* max(dot(PpliV, S'dli), 0). */
            glmOPCODE(MAX, temp4, X);
                glmTEMP(temp3, XXXX);
                glmCONST(0);

            /* Get cos(Crli). */
            glmOPCODE(MOV, temp5, X);
                glmLIGHTING_UNIFORM(VS, uCosCrli, XXXX, LightIndex);

            /* rSpot = 0 */
            glmOPCODE(MOV, ShaderControl->rSpot, X);
                glmCONST(0);

            /* if (max(dot(PpliV, S'dli), 0) < cos(Crli)) goto lblZero. */
            glmOPCODE_BRANCH(JMP, LESS, lblEnd);
                glmTEMP(temp4, XXXX);
                glmTEMP(temp5, XXXX);

            {
                /*
                **  CASE:   max(dot(PpliV, S'dli), 0) >= cos(Crli)
                **
                **                                   Srli
                **  rSpot = max(dot(PpliV, S'dli), 0)
                */
                glmOPCODE(POW, ShaderControl->rSpot, X);
                    glmTEMP(temp4, XXXX);
                    glmLIGHTING_UNIFORM(VS, uSrli, XXXX, LightIndex);
            }

            glmLABEL(lblEnd);
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightAmbient
**
**  Generate ambient factor for the current light. Per OpenGL ES spec:
**
**      AMBIENTi = Acm * Acli
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Current light.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _lightAmbient(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d",
                    Context, ShaderControl, LightIndex);

    do
    {
        /* Per-light optimization. */
        if (
                (
                    (LightIndex >= 0) &&

                    /* Ambient = 0, if Acli == 0 */
                    Context->lightingStates.Acli[LightIndex].zero3
                )

                /* Already computed. */
                || (ShaderControl->rAmbient != 0)
           )
        {
            status = gcvSTATUS_OK;
        }
        else
        {
            /*******************************************************************
            ** Per OpenGL ES spec:
            **   When color material tracking is enabled, by glEnable with
            **   COLOR_MATERIAL, the ambient (Acm) property of both the front
            **   and back material are immediately set to the value of the
            **   current color, and will track changes to the current color
            **   resulting from either the glColor commands or drawing vertex
            **   arrays with the color array enabled.
            */

            if (Context->lightingStates.materialEnabled)
            {
                /* Color from stream? */
                if (Context->aColorInfo.streamEnabled)
                {
                    /* Allocate resources. */
                    glmUSING_UNIFORM(uAcli);
                    glmUSING_ATTRIBUTE(aColor);

                    /* Allocate the result. */
                    glmALLOCATE_TEMP(ShaderControl->rAmbient);

                    /* rAmbient = aColor * Acli */
                    glmOPCODE(MUL, ShaderControl->rAmbient, XYZ);
                        glmATTRIBUTE(VS, aColor, XYZZ);
                        glmLIGHTING_UNIFORM(VS, uAcli, XYZZ, LightIndex);
                }

                /* Color from a constant. */
                else
                {
                    /* Color == 0 ? */
                    if (Context->aColorInfo.currValue.zero3)
                    {
                        /* rAmbient = 0 */
                        status = gcvSTATUS_OK;
                    }
                    else
                    {
                        /* Allocate temporaries. */
                        glmALLOCATE_TEMP(ShaderControl->rAmbient);

                        /* Allocate resources. */
                        glmUSING_UNIFORM(uAcmAcli);

                        /* rAmbient = pre-calculated (aColor * Acli) */
                        glmOPCODE(MOV, ShaderControl->rAmbient, XYZ);
                            glmLIGHTING_UNIFORM(VS, uAcmAcli, XYZZ, LightIndex);
                    }
                }
            }
            else
            {
                if (Context->lightingStates.Acm.zero3)
                {
                    /* rAmbient = 0 */
                    status = gcvSTATUS_OK;
                }
                else
                {
                    /* Allocate temporaries. */
                    glmALLOCATE_TEMP(ShaderControl->rAmbient);

                    /* Allocate resources. */
                    glmUSING_UNIFORM(uAcmAcli);

                    /* rAmbient = pre-calculated (uAcm * Acli) */
                    glmOPCODE(MOV, ShaderControl->rAmbient, XYZ);
                        glmLIGHTING_UNIFORM(VS, uAcmAcli, XYZZ, LightIndex);
                }
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightDiffuse
**
**  Compute diffuse component of the formula. Per OpenGL ES spec:
**
**      DIFFUSEi = (n dot VPpli) * (Dcm * Dcli),
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Current light.
**
**      OutputIndex
**          Color output index (0 or 1).
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _lightDiffuse(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex,
    gctINT OutputIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d OutputIndex=%d",
                    Context, ShaderControl, LightIndex, OutputIndex);

    do
    {
        /* Per-light optimization. */
        if (
                (LightIndex >= 0) &&

                /* Diffuse = 0, if Dcli == 0 */
                Context->lightingStates.Dcli[LightIndex].zero3
           )
        {
            status = gcvSTATUS_OK;
        }
        else
        {
            /*******************************************************************
            ** Per OpenGL ES spec:
            **   When color material tracking is enabled, by glEnable with
            **   COLOR_MATERIAL, the diffuse (Dcm) property of both the front
            **   and back material are immediately set to the value of the
            **   current color, and will track changes to the current color
            **   resulting from either the glColor commands or drawing vertex
            **   arrays with the color array enabled.
            */

            /* Allocate temporary registers. */
            if (Context->lightingStates.materialEnabled)
            {
                /* Color from stream? */
                if (Context->aColorInfo.streamEnabled)
                {
                    glmDEFINE_TEMP(temp);

                    /* Determine dot product between norm vector and VPpli. */
                    gcmERR_BREAK(_lightNormDotVPpli(
                        Context, ShaderControl, LightIndex, OutputIndex
                        ));

                    /* Allocate temporaries. */
                    glmALLOCATE_TEMP(temp);
                    glmALLOCATE_TEMP(ShaderControl->rDiffuse[OutputIndex]);

                    /* Allocate resources. */
                    glmUSING_UNIFORM(uDcli);
                    glmUSING_ATTRIBUTE(aColor);

                    /* temp = aColor * Dcli */
                    glmOPCODE(MUL, temp, XYZ);
                        glmATTRIBUTE(VS, aColor, XYZZ);
                        glmLIGHTING_UNIFORM(VS, uDcli, XYZZ, LightIndex);

                    /* rDiffuse[OutputIndex] = rNdotVPpli * temp */
                    glmOPCODE(MUL, ShaderControl->rDiffuse[OutputIndex], XYZ);
                        glmTEMP(ShaderControl->rNdotVPpli[OutputIndex], XXXX);
                        glmTEMP(temp, XYZZ);
                }

                /* Color from a constant. */
                else
                {
                    /* Color == 0 ? */
                    if (Context->aColorInfo.currValue.zero3)
                    {
                        /* rDiffuse[OutputIndex] = 0 */
                        status = gcvSTATUS_OK;
                    }
                    else
                    {
                        /* Determine dot product between norm vector and VPpli. */
                        gcmERR_BREAK(_lightNormDotVPpli(
                            Context, ShaderControl, LightIndex, OutputIndex
                            ));

                        /* Allocate temporaries. */
                        glmALLOCATE_TEMP(ShaderControl->rDiffuse[OutputIndex]);

                        /* Allocate resources. */
                        glmUSING_UNIFORM(uDcmDcli);

                        /* rDiffuse[OutputIndex] = rNdotVPpli[OutputIndex] * uColor * Dcli */
                        glmOPCODE(MUL, ShaderControl->rDiffuse[OutputIndex], XYZ);
                            glmTEMP(ShaderControl->rNdotVPpli[OutputIndex], XXXX);
                            glmLIGHTING_UNIFORM(VS, uDcmDcli, XYZZ, LightIndex);
                    }
                }
            }
            else
            {
                if (Context->lightingStates.Dcm.zero3)
                {
                    /* rDiffuse[OutputIndex] = 0 */
                    status = gcvSTATUS_OK;
                }
                else
                {
                    /* Determine dot product between norm vector and VPpli. */
                    gcmERR_BREAK(_lightNormDotVPpli(
                        Context, ShaderControl, LightIndex, OutputIndex
                        ));

                    /* Allocate temporaries. */
                    glmALLOCATE_TEMP(ShaderControl->rDiffuse[OutputIndex]);

                    /* Allocate resources. */
                    glmUSING_UNIFORM(uDcmDcli);

                    /* rDiffuse[OutputIndex] = rNdotVPpli[OutputIndex] * uDcm * Dcli */
                    glmOPCODE(MUL, ShaderControl->rDiffuse[OutputIndex], XYZ);
                        glmTEMP(ShaderControl->rNdotVPpli[OutputIndex], XXXX);
                        glmLIGHTING_UNIFORM(VS, uDcmDcli, XYZZ, LightIndex);
                }
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightSpecular
**
**  Compute specular component of the formula. Per OpenGL ES spec:
**
**      SPECULARi = (Fi) * ((n dot H'i) ** Srm) * (Scm * Scli),
**
**  where
**
**      Fi = 1, if (n dot VPpli) != 0
**         = 0 otherwise
**
**      Hi = halfway vector = VPpli + (0, 0, 1)
**
**      H'i = normalized(Hi)
**
**  OpenGL spec defines:
**
**      Hi = VPpli + VPe, where VPe is a unit vector between the vertex and the
**      eye position.
**
**  OpenGL ES always assumes an infinite viewer, which in unit vector space
**  means (0, 0, 1).
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Current light.
**
**      OutputIndex
**          Color output index (0 or 1).
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _lightSpecular(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex,
    gctINT OutputIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d OutputIndex=%d",
                    Context, ShaderControl, LightIndex, OutputIndex);

    do
    {
        /* Per-light optimization. */
        if (
                (LightIndex >= 0) &&

                /* Specular = 0, if Scm == 0 or Scli == 0 */
                (
                    Context->lightingStates.Scm.zero3 ||
                    Context->lightingStates.Scli[LightIndex].zero3
                )
           )
        {
            status = gcvSTATUS_OK;
        }
        else
        {
            glmDEFINE_LABEL(lblZero);

            /* Determine dot product between norm vector and VPpli. */
            gcmERR_BREAK(_lightNormDotVPpli(
                Context, ShaderControl, LightIndex, OutputIndex
                ));

            /* Allocate resources. */
            glmALLOCATE_LABEL(lblZero);
            glmALLOCATE_TEMP(ShaderControl->rSpecular[OutputIndex]);

            /* rSpecular[OutputIndex] = 0 */
            glmOPCODE(MOV, ShaderControl->rSpecular[OutputIndex], XYZ);
                glmCONST(0);

            /* Function Fi: if (max(dot(n, VPpli), 0) == 0) */
            glmOPCODE_BRANCH(JMP, EQUAL, lblZero);
                glmTEMP(ShaderControl->rNdotVPpli[OutputIndex], XXXX);
                glmCONST(0);

            {
                /*
                **  CASE:   max(dot(n, VPpli), 0) != 0
                **
                **                                     Srm
                **  rSpecular = Scm * Scli * (n dot H'i)
                */

                /* Srm == 0 --> no need to compute the dot product. */
                if (Context->lightingStates.Srm.zero)
                {
                    glmALLOCATE_LOCAL_TEMP(temp);

                    /* Allocate resources. */
                    glmUSING_UNIFORM(uScm);
                    glmUSING_UNIFORM(uScli);

                    glmOPCODE(MOV, temp, XYZ);
                        glmUNIFORM(VS, uScm, XYZZ);

                    glmOPCODE(MUL, ShaderControl->rSpecular[OutputIndex], XYZ);
                        glmTEMP(temp, XYZZ);
                        glmLIGHTING_UNIFORM(VS, uScli, XYZZ, LightIndex);
                }

                /* Srm != 0 */
                else
                {
                    /* Define temporaries. */
                    glmDEFINE_TEMP(temp1);
                    glmDEFINE_TEMP(temp2);
                    glmDEFINE_TEMP(temp3);
                    glmDEFINE_TEMP(temp4);
                    glmDEFINE_TEMP(temp5);
                    glmDEFINE_TEMP(temp6);

                    /* Transform normal to eye space. */
                    gcmERR_BREAK(_Normal2Eye(Context, ShaderControl));

                    /* Determnine VPpli unit vector. */
                    gcmERR_BREAK(_lightDetermineVPpli(Context, ShaderControl, LightIndex));

                    /* Allocate temporaries. */
                    glmALLOCATE_TEMP(temp1);
                    glmALLOCATE_TEMP(temp2);
                    glmALLOCATE_TEMP(temp3);
                    glmALLOCATE_TEMP(temp4);
                    glmALLOCATE_TEMP(temp5);
                    glmALLOCATE_TEMP(temp6);

                    /* Allocate resources. */
                    glmUSING_UNIFORM(uSrm);
                    glmUSING_UNIFORM(uScm);
                    glmUSING_UNIFORM(uScli);

                    /*
                    **  Compute halfway vector:
                    **      temp1 = VPpli + (0, 0, 1).
                    */
                    glmOPCODE(MOV, temp1, XYZ);
                        glmTEMP(ShaderControl->rVPpli, XYZZ);

                    glmOPCODE(ADD, temp1, Z);
                        glmTEMP(ShaderControl->rVPpli, ZZZZ);
                        glmCONST(1);

                    /* Normalize halfway vector. */
                    glmOPCODE(NORM, temp2, XYZ);
                        glmTEMP(temp1, XYZZ);

                    /* temp3.x = max(dot(n, H'i), 0). */
                    glmOPCODE(DP3, temp3, X);
                        glmTEMP(ShaderControl->rNrmInEyeSpace[OutputIndex], XYZZ);
                        glmTEMP(temp2, XYZZ);

                    glmOPCODE(MAX, temp4, X);
                        glmTEMP(temp3, XXXX);
                        glmCONST(0);

                    /* temp5.x = max(dot(n, H'i), 0) ^ Srm */
                    glmOPCODE(POW, temp5, X);
                        glmTEMP(temp4, XXXX);
                        glmUNIFORM(VS, uSrm, XXXX);

                    /* Multuply by Scm. */
                    glmOPCODE(MUL, temp6, XYZ);
                        glmTEMP(temp5, XXXX);
                        glmUNIFORM(VS, uScm, XYZZ);

                    /* Multuply by Scli. */
                    glmOPCODE(MUL, ShaderControl->rSpecular[OutputIndex], XYZ);
                        glmTEMP(temp6, XYZZ);
                        glmLIGHTING_UNIFORM(VS, uScli, XYZZ, LightIndex);
                }
            }

            glmLABEL(lblZero);
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _lightGetPerLightResult
**
**  Complete the result by putting all components together.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Current light.
**
**      OutputIndex
**          Color output index (0 or 1).
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _lightGetPerLightResult(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex,
    gctINT OutputIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d OutputIndex=%d",
                    Context, ShaderControl, LightIndex, OutputIndex);

    do
    {
        /* Define the accumulator. */
        glmDEFINE_TEMP(accumulator) = 0;
        glmDEFINE_TEMP(temp) = 0;
        glmDEFINE_TEMP(tempOut) = 0;
        glmALLOCATE_TEMP(temp);
        glmALLOCATE_TEMP(tempOut);
        glmOPCODE(MOV, tempOut, XYZW);
            glmTEMP(ShaderControl->rLighting[OutputIndex], XYZW);
        /* Initialize with ambient component. */
        if (ShaderControl->rAmbient)
        {
            glmALLOCATE_TEMP(accumulator);
            glmOPCODE(MOV, accumulator, XYZ);
                glmTEMP(ShaderControl->rAmbient, XYZZ);
        }

        /* Add diffuse component. */
        if (ShaderControl->rDiffuse[OutputIndex])
        {
            if (accumulator)
            {
                glmOPCODE(MOV, temp, XYZW);
                    glmTEMP(accumulator, XYZW);
                glmOPCODE(ADD, accumulator, XYZ);
                    glmTEMP(temp, XYZZ);
                    glmTEMP(ShaderControl->rDiffuse[OutputIndex], XYZZ);
            }
            else
            {
                glmALLOCATE_TEMP(accumulator);
                glmOPCODE(MOV, accumulator, XYZ);
                    glmTEMP(ShaderControl->rDiffuse[OutputIndex], XYZZ);
            }
        }

        /* Add specular component. */
        if (ShaderControl->rSpecular[OutputIndex])
        {
            if (accumulator)
            {
                glmOPCODE(MOV, temp, XYZW);
                    glmTEMP(accumulator, XYZW);
                glmOPCODE(ADD, accumulator, XYZ);
                    glmTEMP(temp, XYZZ);
                    glmTEMP(ShaderControl->rSpecular[OutputIndex], XYZZ);
            }
            else
            {
                glmALLOCATE_TEMP(accumulator);
                glmOPCODE(MOV, accumulator, XYZ);
                    glmTEMP(ShaderControl->rSpecular[OutputIndex], XYZZ);
            }
        }

        /* Multiply by the spot factor. */
        if (ShaderControl->rSpot)
        {
            glmOPCODE(MOV, temp, XYZW);
                glmTEMP(accumulator, XYZW);
            glmOPCODE(MUL, accumulator, XYZ);
                glmTEMP(ShaderControl->rSpot, XXXX);
                glmTEMP(temp, XYZZ);
        }

        /* Multiply by the attenuation factor. */
        if (ShaderControl->rAttenuation)
        {
            glmOPCODE(MOV, temp, XYZW);
                glmTEMP(accumulator, XYZW);
            glmOPCODE(MUL, accumulator, XYZ);
                glmTEMP(ShaderControl->rAttenuation, XXXX);
                glmTEMP(temp, XYZZ);
        }

        /* Add to the final result. */
        glmOPCODE(ADD, ShaderControl->rLighting[OutputIndex], XYZ);
            glmTEMP(tempOut, XYZZ);
            glmTEMP(accumulator, XYZZ);
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _GenerateLightingFormula
**
**  Generate light-dependent part of the formula.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      LightIndex
**          Light in interest. For a generic version set to -1.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _GenerateLightingFormula(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT i;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d",
                    Context, ShaderControl, LightIndex);

    /* Reset temporary registers. */
    ShaderControl->rVPpli        = 0;
    ShaderControl->rVPpliLength  = 0;
    ShaderControl->rNdotVPpli[0] = 0;
    ShaderControl->rNdotVPpli[1] = 0;
    ShaderControl->rAttenuation  = 0;
    ShaderControl->rSpot         = 0;
    ShaderControl->rAmbient      = 0;
    ShaderControl->rDiffuse[0]   = 0;
    ShaderControl->rDiffuse[1]   = 0;
    ShaderControl->rSpecular[0]  = 0;
    ShaderControl->rSpecular[1]  = 0;

    /* Compute the lighting color. */
    for (i = 0; i < ShaderControl->outputCount; i++)
    {
        /* Compute light ambient component of the formula. */
        gcmERR_BREAK(_lightAmbient(
            Context, ShaderControl, LightIndex
            ));

        /* Compute diffuse component of the formula. */
        gcmERR_BREAK(_lightDiffuse(
            Context, ShaderControl, LightIndex, i
            ));

        /* Compute specular component of the formula. */
        gcmERR_BREAK(_lightSpecular(
            Context, ShaderControl, LightIndex, i
            ));

        /* Verify the state of ambient, diffuse and specular factors. */
        if ((ShaderControl->rAmbient == 0) &&
            (ShaderControl->rDiffuse[i] == 0) &&
            (ShaderControl->rSpecular[i] == 0))
        {
            /* Ambient, diffuse and specular factors are all zeros,
               the result for the current light becomes a zero as well. */
            continue;
        }

        /* Compute light attenuation factor (ATTi). */
        gcmERR_BREAK(_lightAttenuation(
            Context, ShaderControl, LightIndex
            ));

        /* Compute light spot factor (SPOTi). */
        gcmERR_BREAK(_lightSpot(
            Context, ShaderControl, LightIndex
            ));

        /* Finalize the result for the current light. */
        gcmERR_BREAK(_lightGetPerLightResult(
            Context, ShaderControl, LightIndex, i
            ));
    }

    gcmFOOTER();
    /* Return status. */
    return status;
}

static gceSTATUS _CallGenericLightingFunction(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl,
    gctINT LightIndex
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x LightIndex=%d",
                    Context, ShaderControl, LightIndex);

    do
    {
        gctUINT label;

        /* Set the index of the light to be worked on. */
        glmOPCODE(MOV, ShaderControl->rLightIndex, X);
            glmCONST(LightIndex);

        /* Generate the call instruction. */
        gcmERR_BREAK(gcFUNCTION_GetLabel(
            ShaderControl->funcLighting,
            &label
            ));

        gcmERR_BREAK(gcSHADER_AddOpcodeConditional(
            ShaderControl->i->shader,
            gcSL_CALL,
            gcSL_ALWAYS,
            label
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _GetOuputColorFromLighting
**
**  Compute vertex color based on lighting equation.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _GetOuputColorFromLighting(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        gctINT i;
        gctUINT lightIndex;
        gctBOOL bExpandLighting = _checkExpandLighting(Context);

        /* Generate general lighting component of the lighting formula. */
        gcmERR_BREAK(_lightGeneral(Context, ShaderControl));

        /* Use generic lighting function when there are more then 4 lights. */
        if (Context->lightingStates.useFunction && !bExpandLighting)
        {
            /* Allocate light index register. */
            glmALLOCATE_TEMP(ShaderControl->rLightIndex);

            /* Create lighting function. */
            glmADD_FUNCTION(funcLighting);

            /* Begin function. */
            glmBEGIN_FUNCTION(funcLighting);

            /* Generate generic light formula. */
            gcmERR_BREAK(_GenerateLightingFormula(Context, ShaderControl, -1));

            /* Add the return instruction. */
            glmRET();

            /* End function. */
            glmEND_FUNCTION(funcLighting);

            /* Iterate through lights. */
            for (lightIndex = 0; lightIndex < glvMAX_LIGHTS; lightIndex++)
            {
                if (Context->lightingStates.lightEnabled[lightIndex])
                {
                    gcmERR_BREAK(_CallGenericLightingFunction(
                        Context,
                        ShaderControl,
                        lightIndex
                        ));
                }
            }
        }
        else
        {
            /* Iterate through lights. */
            for (lightIndex = 0; lightIndex < glvMAX_LIGHTS; lightIndex++)
            {
                if (Context->lightingStates.lightEnabled[lightIndex])
                {
                    /* Inline an optimized version. */
                    gcmERR_BREAK(_GenerateLightingFormula(
                        Context,
                        ShaderControl,
                        lightIndex
                        ));
                }
            }
        }

        /* Assign varying. */
        if (gcmIS_SUCCESS(status))
        {
            for (i = 0; i < ShaderControl->outputCount; i++)
            {
                /* Allocate color varying. */
                glmALLOCATE_TEMP(ShaderControl->vColor[i]);

                /* Clamp. */
                glmOPCODE(SAT, ShaderControl->vColor[i], XYZW);
                    glmTEMP(ShaderControl->rLighting[i], XYZW);
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _GetOuputColorFromInput
**
**  Determine the proper input color source and set it to the output.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _GetOuputColorFromInput(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Get the color from the stream if enabled. */
        if ((Context->aColorInfo.streamEnabled) &&
            (!Context->drawClearRectEnabled)
            )
        {
            /* Allocate a temp. */
            glmALLOCATE_LOCAL_TEMP(temp);

            /* Allocate color varying. */
            glmALLOCATE_TEMP(ShaderControl->vColor[0]);

            /* Allocate color attribute. */
            glmUSING_ATTRIBUTE(aColor);

            /* Move to the output. */
            glmOPCODE(MOV, temp, XYZW);
                glmATTRIBUTE(VS, aColor, XYZW);

            /* Clamp. */
            glmOPCODE(SAT, ShaderControl->vColor[0], XYZW);
                glmTEMP(temp, XYZW);
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _PrepareFog
**
**  Compute the eye space vertex position and pass it to the fragment shader.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _ProcessFog(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Compute current position in eye space. */
        gcmERR_BREAK(_Pos2Eye(Context, ShaderControl));

        /* Allocate eye position register. */
        glmALLOCATE_TEMP(ShaderControl->vEyePosition);

        /* We take the "easy way out" per spec, only need Z. */
        glmOPCODE(MOV, ShaderControl->vEyePosition, X);
            glmTEMP(ShaderControl->rVtxInEyeSpace, ZZZZ);
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  _ProcessDepthBias
**
**  Add the DepthBias to the vertex in clip space.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _ProcessDepthBias(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Allocate temporaries. */
        glmALLOCATE_LOCAL_TEMP(temp1);
        glmALLOCATE_LOCAL_TEMP(temp2);
        glmALLOCATE_LOCAL_TEMP(temp3);

        /* In hardware implementation, depth bias is added after the
           this (z + w) / (2 x w) transformation is done. So here we need to
           find such a depth bias value that will end up with the same result.
        */

        /* find 2 * depth bias */
        glmOPCODE(MUL, temp1, X);
            glmCONST(Context->hashKey.hashDepthBias);
            glmCONST(2.0);

        /* find 2 * depth bias * w */
        glmOPCODE( MUL, temp2, X);
            glmTEMP(ShaderControl->vPosition, WWWW);
            glmTEMP(temp1, XXXX);

        /* Add Depth bias to the vPosition.z. */
        glmOPCODE(ADD, temp3, X);
            glmTEMP(ShaderControl->vPosition, ZZZZ);
            glmTEMP(temp2, XXXX);

        /* mov temp to position. */
        glmOPCODE(MOV, ShaderControl->vPosition, Z);
            glmTEMP(temp3, XXXX);

    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  _TransformTextureCoordinates
**
**  Transform texture coordinates as necessary.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TransformTextureCoordinates(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        glmDEFINE_TEMP(reflection) = 0;

        gctBOOL haveTexture    = gcvFALSE;
        gctBOOL needNormal     = gcvFALSE;
        gctBOOL needReflection = gcvFALSE;

        GLint i;

        if (Context->drawClearRectEnabled)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Scan all stages and determine control flags. */
        for (i = 0; i < Context->texture.pixelSamplers; i++)
        {
            glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[i];

            /* Skip the stage if disabled. */
            if (!sampler->stageEnabled)
                continue;

            /* Have at least one stage enabled. */
            haveTexture = gcvTRUE;

            /* Skip the stage if coordinate generation is disabled. */
            if (!sampler->genEnable)
                continue;

            /* Coordinate generation needs the current normal. */
            needNormal = gcvTRUE;

            /* Skip if the generation mode is not reflection. */
            if (sampler->genMode != glvREFLECTION)
                continue;

            /* Mark reflection as needed. */
            needReflection = gcvTRUE;
        }

        /* All stages are disabled? */
        if (!haveTexture)
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* Generate the current normal if needed. */
        if (needNormal)
        {
            /* Transform normal to eye space. */
            gcmERR_BREAK(_Normal2Eye(
                Context, ShaderControl
                ));
        }

        /* Generate the reflection vector if needed. */
        if (needReflection)
        {
            /* Define unit vectors. */
            glmDEFINE_TEMP(unitNormal);
            glmDEFINE_TEMP(unitPosition);

            /* Allocate temporaries. */
            glmALLOCATE_LOCAL_TEMP(temp1);
            glmALLOCATE_LOCAL_TEMP(temp2);
            glmALLOCATE_LOCAL_TEMP(temp3);

            /**********************************************************
            ** Reflection vector is computed using the following      **
            ** formula:                                               **
            **                                                        **
            ** r = u - 2 * n * (n DOT u)                              **
            **                                                        **
            ** where u is a unit vector pointing from the origin to   **
            **         the vertex (in eye coordinates);               **
            **       n is the current transformed normal in eye       **
            **         coordinates.                                   **
            **                                                        **
             **********************************************************/

            /***********************************************************
            ** Normalize the normal vector.
            */

            if (Context->normalizeNormal)
            {
                /* Already normalized. */
                unitNormal = ShaderControl->rNrmInEyeSpace[0];
            }
            else
            {
                /* Allocate new temp. */
                glmALLOCATE_TEMP(unitNormal);

                /* unitNormal = norm(rNrmInEyeSpace[0]) */
                glmOPCODE(NORM, unitNormal, XYZ);
                    glmTEMP(ShaderControl->rNrmInEyeSpace[0], XYZZ);
            }

            /***********************************************************
            ** Normalize the position vector.
            */

            /* Allocate new temp. */
            glmALLOCATE_TEMP(unitPosition);
            if (Context->chipModel >= gcv1000 || Context->chipModel == gcv880)
			{
                /* Allocate temporaries. */
                glmALLOCATE_LOCAL_TEMP(temp4);
                glmALLOCATE_LOCAL_TEMP(temp5);

                /* mov postion to temp. */
                glmOPCODE(MOV, temp5, XYZW);
                    glmTEMP(ShaderControl->vPosition, XYZW);

                /* get position ( Z + W ) / 2. */
                glmOPCODE(ADD, temp4, Z);
                    glmTEMP(ShaderControl->vPosition, ZZZZ);
				    glmTEMP(ShaderControl->vPosition, WWWW);

                glmOPCODE(MUL, temp5, Z);
                    glmTEMP(temp4, ZZZZ);
                    glmCONST(0.5);

                /* Normalize the position vector. */
                glmOPCODE(NORM, unitPosition, XYZ);
                    glmTEMP(temp5, XYZZ);
			}
			else
			{
                /* Normalize the position vector. */
                glmOPCODE(NORM, unitPosition, XYZ);
                    glmTEMP(ShaderControl->vPosition, XYZZ);
			}
            /***********************************************************
            ** Find (n DOT u) = (normal DOT vPosition.norm).
            */

            glmOPCODE(DP3, temp1, X);
                glmTEMP(unitNormal, XYZZ);
                glmTEMP(unitPosition, XYZZ);

            /***********************************************************
            ** Find (2 * normal * (normal DOT vPosition.norm)).
            */

            glmOPCODE(MUL, temp2, XYZ);
                glmTEMP(unitNormal, XYZZ);
                glmTEMP(temp1, XXXX);

            glmOPCODE(MUL, temp3, XYZ);
                glmTEMP(temp2, XYZZ);
                glmCONST(2);

            /***********************************************************
            ** Find the reflection vector.
            */

            /* Allocate the vector. */
            glmALLOCATE_TEMP(reflection);

            /* Compute reflection. */
            glmOPCODE(SUB, reflection, XYZ);
                glmTEMP(unitPosition, XYZZ);
                glmTEMP(temp3, XYZZ);
        }

        /* Determine the texture coordinates. */
        for (i = 0; i < Context->texture.pixelSamplers; i++)
        {
            glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[i];

            /* Skip the stage if disabled. */
            if (!sampler->stageEnabled)
                continue;

            /* Coordinate generation enabled (GL_OES_texture_cube_map) ? */
            if (sampler->genEnable)
            {
                glsMATRIXSTACK_PTR stack;

                /* Allocate texture coordinate register. */
                glmALLOCATE_TEMP(ShaderControl->vTexCoord[i]);

                /* Normal vector based coordinates? */
                if (sampler->genMode == glvTEXNORMAL)
                {
                    glmOPCODE(MOV, ShaderControl->vTexCoord[i], XYZ);
                        glmTEMP(ShaderControl->rNrmInEyeSpace[0], XYZZ);
                }

                /* No, reflection vector based. */
                else
                {
                    glmOPCODE(MOV, ShaderControl->vTexCoord[i], XYZ);
                        glmTEMP(reflection, XYZZ);
                }

                /* Set Q to 1.0. */
                glmOPCODE(MOV, ShaderControl->vTexCoord[i], W);
                    glmCONST(1);

                /* Get a shortcut to the transformation matrix. */
                stack = &Context->matrixStackArray[glvTEXTURE_MATRIX_0 + i];

                /* Transform texture coordinates. */
                if (!stack->topMatrix->identity)
                {
                    /* Define a pre-transform texture coordinate temp register. */
                    glmDEFINE_TEMP(preXformTexCoord);

                    /* Allocate texture transformation matrix. */
                    glmUSING_UNIFORM(uTexMatrix);

                    /* Set the pre-transform register. */
                    preXformTexCoord = ShaderControl->vTexCoord[i];

                    /* Allocate transformed texture coordinate register. */
                    glmALLOCATE_TEMP(ShaderControl->vTexCoord[i]);

                    /* vTexCoord[i].x = dp4(preXformTexCoord, uTexMatrix[i * 4 + 0]) */
                    glmOPCODE(DP4, ShaderControl->vTexCoord[i], X);
                        glmTEMP(preXformTexCoord, XYZW);
                        glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 0);

                    /* vTexCoord[i].y = dp4(preXformTexCoord, uTexMatrix[i * 4 + 1]) */
                    glmOPCODE(DP4, ShaderControl->vTexCoord[i], Y);
                        glmTEMP(preXformTexCoord, XYZW);
                        glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 1);

                    /* vTexCoord[i].z = dp4(preXformTexCoord, uTexMatrix[i * 4 + 2]) */
                    glmOPCODE(DP4, ShaderControl->vTexCoord[i], Z);
                        glmTEMP(preXformTexCoord, XYZW);
                        glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 2);
                }
            }

            /* DrawTex stream: retrieve texture coordinate from the stream
               without transformation. */
            else if (Context->drawTexOESEnabled)
            {
                /* Allocate texture coordinate register. */
                glmALLOCATE_TEMP(ShaderControl->vTexCoord[i]);

                /* Define texture stream. */
                glmUSING_INDEXED_ATTRIBUTE(aTexCoord, i);

                /* Get the stream value (always 2 components for DrawTex). */
                glmOPCODE(MOV, ShaderControl->vTexCoord[i], XY);
                    glmATTRIBUTE_INDEXED(VS, aTexCoord0, i, XYYY);
            }

            /* Retrieve texture coordinate from the coordinate stream. */
            else if (sampler->aTexCoordInfo.streamEnabled)
            {
                /* Get a shortcut to the transformation matrix. */
                glsMATRIXSTACK_PTR stack =
                    &Context->matrixStackArray[glvTEXTURE_MATRIX_0 + i];

                /* Define texture stream. */
                glmUSING_INDEXED_ATTRIBUTE(aTexCoord, i);

                /* Transform texture coordinates. */
                if (stack->topMatrix->identity)
                {
                    /* Allocate texture coordinate register. */
                    glmALLOCATE_TEMP(ShaderControl->vTexCoord[i]);

                    /* Copy two components form input to output. */
                    if (sampler->coordType == gcSHADER_FLOAT_X2)
                    {
                        glmOPCODE(MOV, ShaderControl->vTexCoord[i], XY);
                            glmATTRIBUTE_INDEXED(VS, aTexCoord0, i, XYYY);
                    }

                    /* Copy three components form input to output. */
                    else if (sampler->coordType == gcSHADER_FLOAT_X3)
                    {
                        glmOPCODE(MOV, ShaderControl->vTexCoord[i], XYZ);
                            glmATTRIBUTE_INDEXED(VS, aTexCoord0, i, XYZZ);
                    }

                    /* Copy four components form input to output. */
                    else
                    {
                        /* Allocate a temp. */
                        glmALLOCATE_LOCAL_TEMP(temp);
                        glmALLOCATE_LOCAL_TEMP(temp1);

                        /* Make sure it's four. */
                        gcmASSERT(sampler->coordType == gcSHADER_FLOAT_X4);

                        /* Get the stream value. */
                        glmOPCODE(MOV, ShaderControl->vTexCoord[i], XYZW);
                            glmATTRIBUTE_INDEXED(VS, aTexCoord0, i, XYZW);

                        /* Make homogeneous vector. */
                        glmOPCODE(RCP, temp, X);
                            glmTEMP(ShaderControl->vTexCoord[i], WWWW);

                        glmOPCODE(MOV, temp1, XYZW);
                            glmTEMP(ShaderControl->vTexCoord[i], XYZW);

                        glmOPCODE(MUL, ShaderControl->vTexCoord[i], XYZW);
                            glmTEMP(temp1, XYZW);
                            glmTEMP(temp, XXXX);
                    }
                }
                else
                {
                    /* Allocate a temp. */
                    glmALLOCATE_LOCAL_TEMP(temp1);

                    /* Allocate texture coordinate varying. */
                    glmALLOCATE_TEMP(ShaderControl->vTexCoord[i]);

                    /* Allocate texture transformation matrix. */
                    glmUSING_UNIFORM(uTexMatrix);

                    /* Case with two components (x, y).*/
                    if (sampler->coordType == gcSHADER_FLOAT_X2)
                    {
                        /* Get the stream value. */
                        glmOPCODE(MOV, temp1, XYZW);
                            glmATTRIBUTE_INDEXED(VS, aTexCoord0, i, XYZW);

                        /* vTexCoord[i].x = dp4(temp1, uTexMatrix[i * 4 + 0]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], X);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 0);

                        /* vTexCoord[i].y = dp4(temp1, uTexMatrix[i * 4 + 1]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], Y);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 1);
                    }

                    /* Case with three components (x, y, z). */
                    else if (sampler->coordType == gcSHADER_FLOAT_X3)
                    {
                        /* Get the stream value. */
                        glmOPCODE(MOV, temp1, XYZW);
                            glmATTRIBUTE_INDEXED(VS, aTexCoord0, i, XYZW);

                        /* vTexCoord[i].x = dp4(temp1, uTexMatrix[i * 4 + 0]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], X);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 0);

                        /* vTexCoord[i].y = dp4(temp1, uTexMatrix[i * 4 + 1]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], Y);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 1);

                        /* vTexCoord[i].z = dp4(temp1, uTexMatrix[i * 4 + 2]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], Z);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 2);
                    }

                    /* Case with four components (x, y, z, w). */
                    else
                    {
                        /* Allocate a temp. */
                        glmALLOCATE_LOCAL_TEMP(temp2);
                        glmALLOCATE_LOCAL_TEMP(temp3);
                        /* Make sure it's four. */
                        gcmASSERT(sampler->coordType == gcSHADER_FLOAT_X4);

                        /* Get the stream value. */
                        glmOPCODE(MOV, temp1, XYZW);
                            glmATTRIBUTE_INDEXED(VS, aTexCoord0, i, XYZW);

                        /* Make homogeneous vector. */
                        glmOPCODE(RCP, temp2, X);
                            glmTEMP(temp1, WWWW);

                        glmOPCODE(MOV, temp3, XYZW);
                            glmTEMP(temp1, XYZW);

                        glmOPCODE(MUL, temp1, XYZW);
                            glmTEMP(temp3, XYZW);
                            glmTEMP(temp2, XXXX);

                        /* vTexCoord[i].x = dp4(temp1, uTexMatrix[i * 4 + 0]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], X);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 0);

                        /* vTexCoord[i].y = dp4(temp1, uTexMatrix[i * 4 + 1]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], Y);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 1);

                        /* vTexCoord[i].z = dp4(temp1, uTexMatrix[i * 4 + 2]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], Z);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 2);

                        /* vTexCoord[i].w = dp4(temp1, uTexMatrix[i * 4 + 3]) */
                        glmOPCODE(DP4, ShaderControl->vTexCoord[i], W);
                            glmTEMP(temp1, XYZW);
                            glmUNIFORM_STATIC(VS, uTexMatrix, XYZW, i * 4 + 3);
                    }
                }
            }

            /* Retrieve texture coordinate from the constant. */
            else
            {
                /* We only send constant texture coordinates through if
                   point sprite is enabled because in this case the
                   coordinate has to be properly interpolated. */
                if (Context->pointStates.spriteActive)
                {
                    /* Allocate uniform. */
                    glmUSING_UNIFORM(uTexCoord);

                    /* Allocate texture coordinate register. */
                    glmALLOCATE_TEMP(ShaderControl->vTexCoord[i]);

                    /* Copy four components form input to output. */
                    glmOPCODE(MOV, ShaderControl->vTexCoord[i], XYZW);
                        glmUNIFORM_STATIC(VS, uTexCoord, XYZW, i);
                }
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TransformClipPlane
**
**  Transform clip plane values.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TransformClipPlane(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        GLint i;

        /* Ignore clip planes if DrawTex extension is in use. */
        if (Context->drawTexOESEnabled)
        {
            break;
        }

        /* Ignore clip planes if DrawClearRect is in use. */
        if (Context->drawClearRectEnabled)
        {
            break;
        }

        for (i = 0; i < glvMAX_CLIP_PLANES; i++)
        {
            if (Context->clipPlaneEnabled[i])
            {
                /* Compute current position in eye space. */
                gcmERR_BREAK(_Pos2Eye(Context, ShaderControl));

                /* Allocate clip plane varyinga temporary for the input. */
                glmALLOCATE_TEMP(ShaderControl->vClipPlane[i]);

                /* Allocate resources. */
                glmUSING_UNIFORM(uClipPlane);

                /* Transform clip plane. */
                glmOPCODE(DP4, ShaderControl->vClipPlane[i], X);
                    glmTEMP(ShaderControl->rVtxInEyeSpace, XYZW);
                    glmUNIFORM_STATIC(VS, uClipPlane, XYZW, i);
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _MapVaryings
**
**  Map all used varyings.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _MapVaryings(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    GLint i;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        glmASSIGN_OUTPUT(vPosition);
        glmASSIGN_OUTPUT(vEyePosition);
        glmASSIGN_INDEXED_OUTPUT(vColor, 0);
        glmASSIGN_INDEXED_OUTPUT(vColor, 1);
        glmASSIGN_OUTPUT(vPointSize);
        glmASSIGN_OUTPUT(vPointFade);
        glmASSIGN_OUTPUT(vPointSmooth);

        for (i = 0; i < glvMAX_TEXTURES; i++)
        {
            if (ShaderControl->vTexCoord[i] != 0)
            {
                glmASSIGN_INDEXED_OUTPUT(vTexCoord, i);
            }
        }

        if (gcmIS_ERROR(status))
        {
            break;
        }

        for (i = 0; i < glvMAX_CLIP_PLANES; i++)
        {
            if (ShaderControl->vClipPlane[i] != 0)
            {
                glmASSIGN_INDEXED_OUTPUT(vClipPlane, i);
            }
        }

        if (gcmIS_ERROR(status))
        {
            break;
        }
    }
    while (gcvFALSE);
    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  _InitializeVS
**
**  INitialize common things for a vertex shader.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _InitializeVS(
    glsCONTEXT_PTR Context,
    glsVSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    do
    {
        /* Allocate the ModelViewProjection uniform. */
        glmUSING_UNIFORM(uModelViewProjection);

        /* Allocate the TexMatrix uniform. */
        glmUSING_UNIFORM(uTexMatrix);
    }
    while (gcvFALSE);
    gcmFOOTER();
    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  glfGenerateVSFixedFunction
**
**  Generate Vertex Shader code.
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

gceSTATUS glfGenerateVSFixedFunction(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status;
    /* Define internal shader structure. */
    glsVSCONTROL vsControl;
    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        gcoOS_ZeroMemory(&vsControl, sizeof(glsVSCONTROL));
        vsControl.i = &Context->currProgram->vs;

        if (vsControl.i->shader == gcvNULL)
        {
            /* Shader may have been deleted. Construct again. */
            gcmONERROR(gcSHADER_Construct(
                Context->hal,
                gcSHADER_TYPE_VERTEX,
                &vsControl.i->shader
                ));
        }

        /* Determine the number of color outputs. */
        vsControl.outputCount
            = (Context->lightingStates.lightingEnabled
            && Context->lightingStates.doTwoSidedlighting)
                ? 2
                : 1;

        /* Initialize the vertex shader. */
        gcmONERROR(_InitializeVS(Context, &vsControl));

        /* Determine the output position. */
        gcmONERROR(_Pos2Clip(Context, &vsControl));

        /* Determine point output parameters. */
        if (Context->pointStates.pointPrimitive)
        {
            /*******************************************************************
            ** The points can be rendered in three different ways:
            **
            ** 1. Point sprites (point.sprite).
            **    When enabled, the antialiased enable state (point.smooth)
            **    is ignored. The exact vertex position is used as the center
            **    and the point size as the side length of the square. Texture
            **    coordinates are replaced for units with GL_COORD_REPLACE_OES
            **    enabled. The result is a square.
            **
            **    The hardware computes and substitutes the texture coordinates,
            **    therefore there is nothing special left for the shader to do.
            **
            ** 2. Antialiased point (point.smooth).
            **    Use exact vertex position as the center and point size
            **    as the diameter of the smooth point with blended edge.
            **
            **    Shader needs to compute alpha values to make a circle out
            **    of the rasterized square.
            **
            ** 3. Non-antialiased point (not sprite and not smooth).
            **    Use integer vertex position as the center and point size
            **    as the side length of the resulting square.
            **
            **    The hardware adjusts the center, nothing special for the
            **    shader to do.
            **
            */

            /* Compute point size. */
            gcmONERROR(_ComputePointSize(Context, &vsControl));

            /* Compute point smooth. */
            if (Context->pointStates.smooth
                && !Context->pointStates.spriteEnable)
            {
                gcmONERROR(_ComputePointSmooth(Context, &vsControl));
            }
        }

        /* Determine output color. */
        if (Context->lightingStates.lightingEnabled && !Context->drawTexOESEnabled)
        {
            gcmONERROR(_GetOuputColorFromLighting(Context, &vsControl));
        }
        else
        {
            gcmONERROR(_GetOuputColorFromInput(Context, &vsControl));
        }

        /* Pass the eye space position if fog is enabled. */
        if (Context->fogStates.enabled && !Context->drawTexOESEnabled)
        {
            gcmONERROR(_ProcessFog(Context, &vsControl));
        }

        if (Context->hashKey.hashDepthBiasFixEnabled)
        {
            gcmONERROR(_ProcessDepthBias(Context, &vsControl));
        }

        /* Determine texture coordinates. */
        gcmONERROR(_TransformTextureCoordinates(Context, &vsControl));

        /* Calculate clip plane equation. */
        gcmONERROR(_TransformClipPlane(Context, &vsControl));

        /* Map varyings. */
        gcmONERROR(_MapVaryings(Context, &vsControl));

        /* Pack the shader. */
        gcmONERROR(gcSHADER_Pack(vsControl.i->shader));

#if !GC_ENABLE_LOADTIME_OPT
        /* Optimize the shader. */
        gcmONERROR(gcOptimizeShader(vsControl.i->shader, gcvNULL));
#endif  /* GC_ENABLE_LOADTIME_OPT */

    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;

OnError:
    /* Destroy partially generated shader. */
    if (vsControl.i->shader)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(vsControl.i->shader));
        vsControl.i->shader = gcvNULL;
    }

    gcmFOOTER();
    /* Return status. */
    return status;
}
