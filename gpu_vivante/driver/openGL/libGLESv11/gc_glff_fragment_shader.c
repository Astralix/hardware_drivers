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

#define _GC_OBJ_ZONE glvZONE_FRAGMENT

/*******************************************************************************
** Strings for debugging.
*/

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
static char* g_functionName[] =
{
    "REPLACE",
    "MODULATE",
    "DECAL",
    "BLEND",
    "ADD",
    "COMBINE"
};

static char* g_combineFunctionName[] =
{
    "REPLACE",
    "MODULATE",
    "ADD",
    "ADD SIGNED",
    "INTERPOLATE",
    "SUBTRACT",
    "DOT3RGB",
    "DOT3RGBA"
};

static char* g_passName[] =
{
    "COLOR",
    "ALPHA"
};

static char* g_sourceName[] =
{
    "TEXTURE",
    "CONSTANT",
    "PRIMARY COLOR",
    "PREVIOUS"
};

static char* g_operandName[] =
{
    "ALPHA",
    "1-ALPHA",
    "COLOR",
    "1-COLOR"
};

#endif


/*******************************************************************************
** Vertex Shader internal parameters.
*/

typedef struct _glsFSCONTROL * glsFSCONTROL_PTR;
typedef struct _glsFSCONTROL
{
    /* Pointer to the exposed shader interface. */
    glsSHADERCONTROL_PTR i;

    /* Postpone clamping in case if the color is not used anymore.
       Hardware does the clamping of the output by default. */
    gctBOOL clampColor;

    /* Temporary register and label allocation indices. */
    glmDEFINE_TEMP(rColor);
    glmDEFINE_TEMP(rLastAllocated);
    glmDEFINE_LABEL(lLastAllocated);

    /* Uniforms. */
    glsUNIFORMWRAP_PTR uniforms[glvUNIFORM_FS_COUNT];

    /* Attributes. */
    glsATTRIBUTEWRAP_PTR attributes[glvATTRIBUTE_FS_COUNT];

    /* Outputs. */
    glmDEFINE_TEMP(oPrevColor);
    glmDEFINE_TEMP(oColor);
}
glsFSCONTROL;


/*******************************************************************************
** Define texture function pointer arrays.
*/

typedef gceSTATUS (* glfDOCOMBINETEXTUREFUNCTION) (
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    );

typedef struct _glsCOMBINEFUNCTION * glsCOMBINEFUNCTION_PTR;
typedef struct _glsCOMBINEFUNCTION
{
    glfDOCOMBINETEXTUREFUNCTION function;
    GLboolean haveArg[3];
}
glsCOMBINEFUNCTION;

#define glmDECLARECOMBINETEXTUREFUNCTION(Name) \
    static gceSTATUS Name( \
        glsCONTEXT_PTR Context, \
        glsFSCONTROL_PTR ShaderControl, \
        gctUINT16_PTR Arguments, \
        glsCOMBINEFLOW_PTR CombineFlow \
        )

#define glmCOMBINETEXTUREFUNCTIONENTRY(Name, Arg0, Arg1, Arg2) \
    {Name, {GL_##Arg0, GL_##Arg1, GL_##Arg2}}

glmDECLARECOMBINETEXTUREFUNCTION(_TexCombFuncReplace);
glmDECLARECOMBINETEXTUREFUNCTION(_TexCombFuncModulate);
glmDECLARECOMBINETEXTUREFUNCTION(_TexCombFuncAdd);
glmDECLARECOMBINETEXTUREFUNCTION(_TexCombFuncAddSigned);
glmDECLARECOMBINETEXTUREFUNCTION(_TexCombFuncInterpolate);
glmDECLARECOMBINETEXTUREFUNCTION(_TexCombFuncSubtract);
glmDECLARECOMBINETEXTUREFUNCTION(_TexCombFuncDot3RGB);
glmDECLARECOMBINETEXTUREFUNCTION(_TexCombFuncDot3RGBA);

static glsCOMBINEFUNCTION _CombineTextureFunctions[] =
{
    glmCOMBINETEXTUREFUNCTIONENTRY(_TexCombFuncReplace,     TRUE, FALSE, FALSE),
    glmCOMBINETEXTUREFUNCTIONENTRY(_TexCombFuncModulate,    TRUE, TRUE,  FALSE),
    glmCOMBINETEXTUREFUNCTIONENTRY(_TexCombFuncAdd,         TRUE, TRUE,  FALSE),
    glmCOMBINETEXTUREFUNCTIONENTRY(_TexCombFuncAddSigned,   TRUE, TRUE,  FALSE),
    glmCOMBINETEXTUREFUNCTIONENTRY(_TexCombFuncInterpolate, TRUE, TRUE,  TRUE),
    glmCOMBINETEXTUREFUNCTIONENTRY(_TexCombFuncSubtract,    TRUE, TRUE,  FALSE),
    glmCOMBINETEXTUREFUNCTIONENTRY(_TexCombFuncDot3RGB,     TRUE, TRUE,  FALSE),
    glmCOMBINETEXTUREFUNCTIONENTRY(_TexCombFuncDot3RGBA,    TRUE, TRUE,  FALSE)
};

typedef gceSTATUS (* glfDOTEXTUREFUNCTION) (
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    glsTEXTURESAMPLER_PTR Sampler,
    gctUINT SamplerNumber
    );

#define glmDECLARETEXTUREFUNCTION(Name) \
    static gceSTATUS Name( \
        glsCONTEXT_PTR Context, \
        glsFSCONTROL_PTR ShaderControl, \
        glsTEXTURESAMPLER_PTR Sampler, \
        gctUINT SamplerNumber \
        )

#define glmTEXTUREFUNCTIONENTRY(Name) \
    Name

glmDECLARETEXTUREFUNCTION(_TexFuncReplace);
glmDECLARETEXTUREFUNCTION(_TexFuncModulate);
glmDECLARETEXTUREFUNCTION(_TexFuncDecal);
glmDECLARETEXTUREFUNCTION(_TexFuncBlend);
glmDECLARETEXTUREFUNCTION(_TexFuncAdd);
glmDECLARETEXTUREFUNCTION(_TexFuncCombine);

static glfDOTEXTUREFUNCTION _TextureFunctions[] =
{
    glmTEXTUREFUNCTIONENTRY(_TexFuncReplace),
    glmTEXTUREFUNCTIONENTRY(_TexFuncModulate),
    glmTEXTUREFUNCTIONENTRY(_TexFuncDecal),
    glmTEXTUREFUNCTIONENTRY(_TexFuncBlend),
    glmTEXTUREFUNCTIONENTRY(_TexFuncAdd),
    glmTEXTUREFUNCTIONENTRY(_TexFuncCombine)
};


/*******************************************************************************
** Temporary allocation helper.
*/

static gctUINT16 _AllocateTemp(
    glsFSCONTROL_PTR ShaderControl
    )
{
    gctUINT16 result;
    gcmHEADER_ARG("ShaderControl=0x%x", ShaderControl);
    gcmASSERT(ShaderControl->rLastAllocated < 65535);
    result = ++ShaderControl->rLastAllocated;
    gcmFOOTER_ARG("return=%u", result);
    return result;
}


/*******************************************************************************
** Label allocation helper.
*/

static gctUINT _AllocateLabel(
    glsFSCONTROL_PTR ShaderControl
    )
{
    gctUINT result;
    gcmHEADER_ARG("ShaderControl=0x%x", ShaderControl);
    result = ++ShaderControl->lLastAllocated;
    gcmFOOTER_ARG("return=%u", result);
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

    if (Context->drawClearRectEnabled)
    {
        Context->aColorInfo.dirty = GL_TRUE;

        status = glfSetUniformFromVectors(
            Uniform,
            &Context->clearColor,
            valueArray,
            1
            );
        gcmFOOTER();
        return status;
    }
    else
    {
        if (!Context->aColorInfo.dirty)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }

        Context->aColorInfo.dirty = GL_FALSE;

        status =  glfSetUniformFromVectors(
            Uniform,
            &Context->aColorInfo.currValue,
            valueArray,
            1
            );
        gcmFOOTER();
        return status;
    }
}

static gceSTATUS _Set_uFogFactors(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    if (Context->fogStates.mode == glvLINEARFOG)
    {
        /* Update the factors. */
        glfUpdateLinearFactors(Context);

        /* Set the uniform. */
        status = glfSetUniformFromFractions(
            Uniform,
            Context->fogStates.linearFactor0,
            Context->fogStates.linearFactor1,
            0, 0
            );
    }

    else if (Context->fogStates.mode == glvEXPFOG)
    {
        /* Update the factors. */
        glfUpdateExpFactors(Context);

        /* Set the uniform. */
        status = glfSetUniformFromFractions(
            Uniform,
            Context->fogStates.expFactor,
            0, 0, 0
            );
    }

    else
    {
        /* Update the factors. */
        glfUpdateExp2Factors(Context);

        /* Set the uniform. */
        status = glfSetUniformFromFractions(
            Uniform,
            Context->fogStates.exp2Factor,
            0, 0, 0
            );
    }

    gcmFOOTER();

    /* Return status. */
    return status;
}

static gceSTATUS _Set_uFogColor(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    gltFRACTYPE valueArray[4];
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);
    status = glfSetUniformFromVectors(
        Uniform,
        &Context->fogStates.color,
        valueArray,
        1
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Set_uTexColor(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    GLint i;
    gltFRACTYPE valueArray[4 * glvMAX_TEXTURES];
    gltFRACTYPE* vector = valueArray;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    for (i = 0; i < Context->texture.pixelSamplers; i++)
    {
        glfGetFromVector4(
            &Context->texture.sampler[i].constColor,
            (gluMUTABLE_PTR) vector,
            glvFRACTYPEENUM
            );

        vector += 4;
    }

    status = gcUNIFORM_SetFracValue(
        Uniform,
        Context->texture.pixelSamplers,
        valueArray
        );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Set_uTexCombScale(
    glsCONTEXT_PTR Context,
    gcUNIFORM Uniform
    )
{
    GLint i;
    gltFRACTYPE valueArray[4 * glvMAX_TEXTURES];
    gltFRACTYPE* vector = valueArray;
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x Uniform=0x%x", Context, Uniform);

    for (i = 0; i < Context->texture.pixelSamplers; i++)
    {
        vector[0] =
        vector[1] =
        vector[2] = glfFracFromMutant(
            &Context->texture.sampler[i].combColor.scale
            );

        vector[3] = glfFracFromMutant(
            &Context->texture.sampler[i].combAlpha.scale
            );

        vector += 4;
    }

    status = gcUNIFORM_SetFracValue(
        Uniform,
        Context->texture.pixelSamplers,
        valueArray
        );

    gcmFOOTER();

    return status;
}


/*******************************************************************************
** Uniform access helpers.
*/

static gceSTATUS _Using_uColor(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
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
        &Context->fsUniformDirty.uColorDirty,
        &glmUNIFORM_WRAP(FS, uColor)
        );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Using_uFogFactors(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    status = glfUsingUniform(
        ShaderControl->i,
        "uFogFactors",
        gcSHADER_FRAC_X4,
        1,
        _Set_uFogFactors,
        &Context->fsUniformDirty.uFogFactorsDirty,
        &glmUNIFORM_WRAP(FS, uFogFactors)
        );

    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uFogColor(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    status = glfUsingUniform(
        ShaderControl->i,
        "uFogColor",
        gcSHADER_FRAC_X4,
        1,
        _Set_uFogColor,
        &Context->fsUniformDirty.uFogColorDirty,
        &glmUNIFORM_WRAP(FS, uFogColor)
        );

    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uTexColor(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    status = glfUsingUniform(
        ShaderControl->i,
        "uTexColor",
        gcSHADER_FRAC_X4,
        Context->texture.pixelSamplers,
        _Set_uTexColor,
        &Context->fsUniformDirty.uTexColorDirty,
        &glmUNIFORM_WRAP(FS, uTexColor)
        );

    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uTexCombScale(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    status = glfUsingUniform(
        ShaderControl->i,
        "uTexCombScale",
        gcSHADER_FRAC_X4,
        Context->texture.pixelSamplers,
        _Set_uTexCombScale,
        &Context->fsUniformDirty.uTexCombScaleDirty,
        &glmUNIFORM_WRAP(FS, uTexCombScale)
        );

    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_uTexSampler(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctINT Sampler
    )
{
    gceSTATUS status;

    static gctCONST_STRING uName[] =
    {
        "uTexSampler0",
        "uTexSampler1",
        "uTexSampler2",
        "uTexSampler3"
    };


    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=%d", Context, ShaderControl, Sampler);

    do
    {
        gcmERR_BREAK(glfUsingUniform(
            ShaderControl->i,
            uName[Sampler],
            gcSHADER_SAMPLER_2D,
            1,
            gcvNULL,
            gcvNULL,
            &glmUNIFORM_WRAP_INDEXED(FS, uTexSampler0, Sampler)
            ));

        ShaderControl->i->texture[Sampler]
            = glmUNIFORM_WRAP_INDEXED(FS, uTexSampler0, Sampler);
    }
    while (gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _Using_uTexCoord(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsing_uTexCoord(
        ShaderControl->i,
        &Context->fsUniformDirty.uTexCoordDirty,
        &glmUNIFORM_WRAP(FS, uTexCoord)
        );
    gcmFOOTER();
    return status;
}


/*******************************************************************************
** Varying access helpers.
*/

static gceSTATUS _Using_vPosition(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingVarying(
        ShaderControl->i,
        "#Position",
        gcSHADER_FLOAT_X4,
        1,
        gcvFALSE,
        &glmATTRIBUTE_WRAP(FS, vPosition)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_vEyePosition(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingVarying(
        ShaderControl->i,
        "vEyePosition",
        gcSHADER_FLOAT_X1,
        1,
        gcvFALSE,
        &glmATTRIBUTE_WRAP(FS, vEyePosition)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_vColor(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctINT InputIndex
    )
{
    static gctCONST_STRING vName[] =
    {
        "vColor0",
        "vColor1"
    };
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x InputIndex=%d", Context, ShaderControl, InputIndex);

    status = glfUsingVarying(
        ShaderControl->i,
        vName[InputIndex],
        gcSHADER_FLOAT_X4,
        1,
        gcvFALSE,
        &glmATTRIBUTE_WRAP_INDEXED(FS, vColor0, InputIndex)
        );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Using_vTexCoord(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
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

    status = glfUsingVarying(
        ShaderControl->i,
        vName[Sampler],
        Context->texture.sampler[Sampler].coordType,
        1,
        gcvTRUE,
        &glmATTRIBUTE_WRAP_INDEXED(FS, vTexCoord0, Sampler)
        );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Using_vClipPlane(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
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

    status = glfUsingVarying(
        ShaderControl->i,
        vName[ClipPlane],
        gcSHADER_FLOAT_X1,
        1,
        gcvTRUE,
        &glmATTRIBUTE_WRAP_INDEXED(FS, vClipPlane0, ClipPlane)
        );

    gcmFOOTER();

    return status;
}

static gceSTATUS _Using_vPointFade(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    status = glfUsingVarying(
        ShaderControl->i,
        "vPointFade",
        gcSHADER_FLOAT_X1,
        1,
        gcvFALSE,
        &glmATTRIBUTE_WRAP(FS, vPointFade)
        );

    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_vPointSmooth(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingVarying(
        ShaderControl->i,
        "vPointSmooth",
        gcSHADER_FLOAT_X3,
        1,
        gcvFALSE,
        &glmATTRIBUTE_WRAP(FS, vPointSmooth)
        );
    gcmFOOTER();
    return status;
}

static gceSTATUS _Using_vFace(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    status = glfUsingVarying(
        ShaderControl->i,
        "#FrontFacing",
        gcSHADER_BOOLEAN_X1,
        1,
        gcvFALSE,
        &glmATTRIBUTE_WRAP(FS, vFace)
        );
    gcmFOOTER();
    return status;
}


/*******************************************************************************
** Output assigning helpers.
*/

static gceSTATUS _Assign_oColor(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16 TempRegister
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x TempRegister=%d", Context, ShaderControl, TempRegister);
    status = gcSHADER_AddOutput(
        ShaderControl->i->shader,
        "#Color",
        gcSHADER_FLOAT_X4,
        1,
        TempRegister
        );
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  _ClampColor
**
**  Clamp color if needed.
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

static gceSTATUS _ClampColor(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Clamp color if needed. */
        if (!ShaderControl->clampColor)
        {
            break;
        }

        /* Allocate a temp for previous color value. */
        ShaderControl->oPrevColor = ShaderControl->oColor;

        /* Allocate color output. */
        glmALLOCATE_TEMP(ShaderControl->oColor);

        /* Clamp. */
        glmOPCODE(SAT, ShaderControl->oColor, XYZW);
            glmTEMP(ShaderControl->oPrevColor, XYZW);

        /* Reset the clamping flag. */
        ShaderControl->clampColor = gcvFALSE;
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _ProcessClipPlane
**
**  Kill pixels if the corresponding value (Ax + By + Cz + Dw) computed
**  by Vertex Shader is less then zero.
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

static gceSTATUS _ProcessClipPlane(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
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

        /* Igore clip planes if DrawClearRect is in use. */
        if (Context->drawClearRectEnabled)
        {
            break;
        }

        for (i = 0; i < glvMAX_CLIP_PLANES; i++)
        {
            if (Context->clipPlaneEnabled[i])
            {
                /* Allocate resources. */
                glmUSING_INDEXED_VARYING(vClipPlane, i);

                /* if (vClipPlane[i] < 0) discard */
                glmOPCODE_BRANCH(KILL, LESS, 0);
                    glmVARYING_INDEXED(FS, vClipPlane0, i, XXXX);
                    glmCONST(0);
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
**  _GetInitialColor
**
**  Determine preliminary color output.
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

static gceSTATUS _GetInitialColor(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Allocate color registers. */
        glmALLOCATE_TEMP(ShaderControl->rColor);
        glmALLOCATE_TEMP(ShaderControl->oColor);

        /* Two-sided lighting case. */
        if (Context->lightingStates.lightingEnabled &&
            Context->lightingStates.doTwoSidedlighting)
        {
            /* Allocate a label. */
            glmALLOCATE_LOCAL_LABEL(lblFrontFace);

            /* Allocate the varyings. */
            glmUSING_INDEXED_VARYING(vColor, 0);
            glmUSING_INDEXED_VARYING(vColor, 1);
            glmUSING_VARYING(vFace);

            /* Assume front face. */
            glmOPCODE(MOV, ShaderControl->rColor, XYZW);
                glmVARYING(FS, vColor0, XYZW);

            /* If the front face specified as clockwise (GL_CW), then we pick
               the front color whenever the area is negative. */
            if (Context->cullStates.clockwiseFront)
            {
                /* if (face == 1) goto lblFrontFace. */
                glmOPCODE_BRANCH(JMP, EQUAL, lblFrontFace);
                    glmVARYING(FS, vFace, XXXX);
                    glmCONST(1);
            }

            /* If the front face specified as counterclockwise (GL_CCW),
               then we pick the front color whenever the area is positive. */
            else
            {
                /* if (face == 1) goto lblFrontFace. */
                glmOPCODE_BRANCH(JMP, EQUAL, lblFrontFace);
                    glmVARYING(FS, vFace, XXXX);
                    glmCONST(0);
            }

            {
                /* Back face. */
                glmOPCODE(MOV, ShaderControl->rColor, XYZW);
                    glmVARYING(FS, vColor1, XYZW);
            }

            /* Define label. */
            glmLABEL(lblFrontFace);
        }

        /* Single-sided lighting or color stream. */
        else if (((Context->lightingStates.lightingEnabled  && !Context->drawTexOESEnabled) ||
                 Context->aColorInfo.streamEnabled) &&
                 (!Context->drawClearRectEnabled))
        {
            /* Allocate the varying. */
            glmUSING_INDEXED_VARYING(vColor, 0);

            /* Copy the value. */
            glmOPCODE(MOV, ShaderControl->rColor, XYZW);
                glmVARYING(FS, vColor0, XYZW);
        }

        /* Color comes from the constant. */
        else
        {
            /* Allocate the uniform. */
            glmUSING_UNIFORM(uColor);

            /* Copy the value. */
            glmOPCODE(MOV, ShaderControl->rColor, XYZW);
                glmUNIFORM(FS, uColor, XYZW);
        }

        /* Copy the value. */
        glmOPCODE(MOV, ShaderControl->oColor, XYZW);
            glmTEMP(ShaderControl->rColor, XYZW);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _GetArgumentSource
**
**  Retrieve specified argument source a texture operation.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Source
**          Specifies the source of the argument.
**
**      SamplerNumber
**          Sampler number.
**
**  OUTPUT:
**
**      SourceRegister
**          Allocated source.
*/

static gceSTATUS _GetArgumentSource(
    IN glsCONTEXT_PTR Context,
    IN glsFSCONTROL_PTR ShaderControl,
    IN gleCOMBINESOURCE Source,
    IN gctUINT SamplerNumber,
    OUT gctUINT16_PTR SourceRegister
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Source=0x%04x SamplerNumber=%u SourceRegister=%u",
        Context, ShaderControl, Source, SamplerNumber, SourceRegister);

    do
    {
        /* Allocate a temporary for the argument. */
        glmALLOCATE_LOCAL_TEMP(temp);

        /* Argument from texture. */
        if (Source == glvTEXTURE)
        {
            /* Get a shortcut to the sampler. */
            glsTEXTURESAMPLER_PTR sampler
                = &Context->texture.sampler[SamplerNumber];

            /* Allocate sampler. */
            glmUSING_INDEXED_VARYING(uTexSampler, SamplerNumber);

            /* Coordinate from stream? */
            if (Context->drawTexOESEnabled ||
                sampler->aTexCoordInfo.streamEnabled ||
                sampler->genEnable ||
                Context->pointStates.spriteActive)
            {
                /* Allocate varying. */
                glmUSING_INDEXED_VARYING(vTexCoord, SamplerNumber);

                /* Flip texture coordinate for point sprite. */
                if (Context->pointStates.spriteActive)
                {
                    glmALLOCATE_LOCAL_TEMP(coord);

                    glmOPCODE(MOV, coord, XYZW);
                        glmVARYINGV_INDEXED(FS, vTexCoord0, SamplerNumber, sampler->coordSwizzle);

                    /* Flip Y coordinate. */
                    glmOPCODE(SUB, coord, Y);
                        glmCONST(1.0f);
                        glmVARYINGV_INDEXED(FS, vTexCoord0, SamplerNumber, sampler->coordSwizzle);

                    /* Load texture. */
                    glmOPCODE(TEXLD, temp, XYZW);
                        glmUNIFORM_INDEXED(FS, uTexSampler0, SamplerNumber, XYZW);
                        glmTEMP(coord, XYYY);
                }
                else
                {
                    /* Load texture. */
                    glmOPCODE(TEXLD, temp, XYZW);
                        glmUNIFORM_INDEXED(FS, uTexSampler0, SamplerNumber, XYZW);
                        glmVARYINGV_INDEXED(FS, vTexCoord0, SamplerNumber, sampler->coordSwizzle);
                }
            }
            else
            {
                /* Allocate uniform. */
                glmUSING_UNIFORM(uTexCoord);

                /* Load texture. */
                glmOPCODE(TEXLD, temp, XYZW);
                    glmUNIFORM_INDEXED(FS, uTexSampler0, SamplerNumber, XYZW);
                    glmUNIFORM_STATIC(FS, uTexCoord, XYZW, SamplerNumber);
            }
        }

        /* Argument from constant color. */
        else if (Source == glvCONSTANT)
        {
            /* Allocate the constant color uniform. */
            glmUSING_UNIFORM(uTexColor);

            /* Move to the argument register. */
            glmOPCODE(MOV, temp, XYZW);
                glmUNIFORM_STATIC(FS, uTexColor, XYZW, SamplerNumber);
        }

        /* Argument from primary fragment color. */
        else if (Source == glvCOLOR)
        {
            /* Copy predetermined value. */
            glmOPCODE(MOV, temp, XYZW);
                glmTEMP(ShaderControl->rColor, XYZW);
        }

        /* Argument from previous stage result. */
        else if (Source == glvPREVIOUS)
        {
            /* Clamp if needed. */
            if (ShaderControl->clampColor)
            {
                /* Move to the argument register with clamping. */
                glmOPCODE(SAT, temp, XYZW);
                    glmTEMP(ShaderControl->oColor, XYZW);
            }
            else
            {
                /* Move to the argument register. */
                glmOPCODE(MOV, temp, XYZW);
                    glmTEMP(ShaderControl->oColor, XYZW);
            }
        }

        /* Assign the result. */
        *SourceRegister = temp;
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _GetArgumentOperand
**
**  Retrieve specified argument for a texture operation.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Operand
**          Specifies the type of the operand.
**
**      SamplerNumber
**          Sampler number.
**
**      SourceRegister
**          The source register number.
**
**  OUTPUT:
**
**      ArgumentRegister
**          Allocated argument.
*/

static gceSTATUS _GetArgumentOperand(
    IN glsCONTEXT_PTR Context,
    IN glsFSCONTROL_PTR ShaderControl,
    IN gleCOMBINEOPERAND Operand,
    IN gctUINT SamplerNumber,
    IN gctUINT16 SourceRegister,
    OUT gctUINT16_PTR ArgumentRegister
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Operand=0x%04x SamplerNumber=%u "
                    "SourceRegister=%u ArgumentRegister=%u",
                    Context, ShaderControl, Operand, SamplerNumber, SourceRegister, ArgumentRegister);

    do
    {
        /* Define a temporary for the argument. */
        glmDEFINE_TEMP(temp);

        /* Argument = Csrc. */
        if (Operand == glvSRCCOLOR)
        {
            /* Set the result. */
            temp = SourceRegister;
        }

        /* Argument = 1 - Csrc. */
        else if (Operand == glvSRCCOLORINV)
        {
            /* Allocate the final argument. */
            glmALLOCATE_TEMP(temp);

            /* temp = 1 - source. */
            glmOPCODE(SUB, temp, XYZW);
                glmCONST(1);
                glmTEMP(SourceRegister, XYZW);
        }

        /* Argument = Asrc. */
        else if (Operand == glvSRCALPHA)
        {
            /* Allocate the final argument. */
            glmALLOCATE_TEMP(temp);

            /* temp2 = temp1.w */
            glmOPCODE(MOV, temp, XYZW);
                glmTEMP(SourceRegister, WWWW);
        }

        /* Argument = 1 - Asrc. */
        else
        {
            gcmASSERT(Operand == glvSRCALPHAINV);

            /* Allocate the final argument. */
            glmALLOCATE_TEMP(temp);

            /* temp2 = 1 - temp1.w */
            glmOPCODE(SUB, temp, XYZW);
                glmCONST(1);
                glmTEMP(SourceRegister, WWWW);
        }

        /* Assign the result. */
        *ArgumentRegister = temp;
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _GetCombineSources
**
**  Retrieve sources required by the selected texture combine function.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      SamplerNumber
**          Sampler number.
**
**      Combine
**          Pointer to the combine function descriptor.
**
**  OUTPUT:
**
**      Sources[4]
**          Array with allocated sources.
*/

static gceSTATUS _GetCombineSources(
    IN glsCONTEXT_PTR Context,
    IN glsFSCONTROL_PTR ShaderControl,
    IN gctUINT SamplerNumber,
    IN glsTEXTURECOMBINE_PTR Combine,
    OUT gctUINT16 Sources[4]
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT i;

    /* Make a shortcut to the current combine function. */
    glsCOMBINEFUNCTION_PTR combineFunction =
        &_CombineTextureFunctions[Combine->function];

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x SamplerNumber=%u Combine=0x%x Sources=0x%x",
        Context, ShaderControl, SamplerNumber, Combine, Sources);


    /* Loop through 3 possible arguments. */
    for (i = 0; i < 3; i++)
    {
        /* Argument needed? */
        if (combineFunction->haveArg[i])
        {
            /* Get current source type. */
            gleCOMBINESOURCE source = Combine->source[i];

            /* Make a shortcut to the current source register. */
            gctUINT16_PTR sourceRegister = &Sources[source];

            /* Not yet allocated? */
            if (*sourceRegister == 0)
            {
                gcmERR_BREAK(_GetArgumentSource(
                    Context,
                    ShaderControl,
                    source,
                    SamplerNumber,
                    sourceRegister
                    ));
            }
        }
    }

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _GetCombineArguments
**
**  Retrieve arguments required by the selected texture combine function.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      SamplerNumber
**          Sampler number.
**
**      Combine
**          Pointer to the array of combine function descriptors.
**
**      Sources[4]
**          Array with allocated sources.
**
**      ArgumentMap[4][4]
**          Map between allocated sources and arguments.
**
**  OUTPUT:
**
**      Arguments[3]
**          Arguments for the current function.
*/

static gceSTATUS _GetCombineArguments(
    IN glsCONTEXT_PTR Context,
    IN glsFSCONTROL_PTR ShaderControl,
    IN gctUINT SamplerNumber,
    IN glsTEXTURECOMBINE_PTR Combine,
    IN gctUINT16 Sources[4],
    IN gctUINT16 ArgumentMap[4][4],
    OUT gctUINT16 Arguments[3]
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT i;

    /* Make a shortcut to the current combine function. */
    glsCOMBINEFUNCTION_PTR combineFunction =
        &_CombineTextureFunctions[Combine->function];

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x SamplerNumber=%u Combine=0x%x "
                    "Sources=0x%x ArgumentMap=0x%x Arguments=0x%x",
                    Context, ShaderControl, SamplerNumber, Combine, Sources, ArgumentMap, Arguments);

    /* Loop through 3 possible arguments. */
    for (i = 0; i < 3; i++)
    {
        /* Argument needed? */
        if (combineFunction->haveArg[i])
        {
            /* Get current source and operand type. */
            gleCOMBINESOURCE  source  = Combine->source[i];
            gleCOMBINEOPERAND operand = Combine->operand[i];

            /* Make a shortcut to the current argument register. */
            gctUINT16_PTR argumentRegister =
                &ArgumentMap[source][operand];

            /* Not yet allocated? */
            if (*argumentRegister == 0)
            {
                gcmERR_BREAK(_GetArgumentOperand(
                    Context,
                    ShaderControl,
                    operand,
                    SamplerNumber,
                    Sources[source],
                    argumentRegister
                    ));
            }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
            /* Report source allocation. */
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_TEXTURE,
                "           Source %d: from %s(reg=%d),",
                i,
                g_sourceName[source], Sources[source]
                );

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_TEXTURE,
                "                      take %s(reg=%d).",
                g_operandName[operand], *argumentRegister
                );
#endif

            /* Set the output. */
            Arguments[i] = *argumentRegister;
        }
    }

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexCombFuncReplace
**
**  REPLACE texture combine function:
**
**      oColor = Arg0
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Arguments
**          Pointer to the array of input arguments.
**
**      CombineFlow
**          Data flow control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexCombFuncReplace(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Arguments=%u CombineFlow=0x%x",
                    Context, ShaderControl, Arguments, CombineFlow);

    do
    {
        glmOPCODEV(MOV, ShaderControl->oColor, CombineFlow->targetEnable);
            glmTEMPV(Arguments[0], CombineFlow->argSwizzle);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexCombFuncModulate
**
**  MODULATE texture combine function:
**
**      oColor = Arg0 * Arg1
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Arguments
**          Pointer to the array of input arguments.
**
**      CombineFlow
**          Data flow control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexCombFuncModulate(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Arguments=%u CombineFlow=0x%x",
                    Context, ShaderControl, Arguments, CombineFlow);

    do
    {
        glmOPCODEV(MUL, ShaderControl->oColor, CombineFlow->targetEnable);
            glmTEMPV(Arguments[0], CombineFlow->argSwizzle);
            glmTEMPV(Arguments[1], CombineFlow->argSwizzle);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexCombFuncAdd
**
**  ADD texture combine function:
**
**      oColor = Arg0 + Arg1
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Arguments
**          Pointer to the array of input arguments.
**
**      CombineFlow
**          Data flow control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexCombFuncAdd(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Arguments=%u CombineFlow=0x%x",
                    Context, ShaderControl, Arguments, CombineFlow);

    do
    {
        glmOPCODEV(ADD, ShaderControl->oColor, CombineFlow->targetEnable);
            glmTEMPV(Arguments[0], CombineFlow->argSwizzle);
            glmTEMPV(Arguments[1], CombineFlow->argSwizzle);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexCombFuncAddSigned
**
**  ADD_SIGNED texture combine function:
**
**      oColor = Arg0 + Arg1 - 0.5
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Arguments
**          Pointer to the array of input arguments.
**
**      CombineFlow
**          Data flow control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexCombFuncAddSigned(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Arguments=%u CombineFlow=0x%x",
                    Context, ShaderControl, Arguments, CombineFlow);

    do
    {
        /* Allocate a temp. */
        glmALLOCATE_LOCAL_TEMP(temp);

        glmOPCODEV(ADD, temp, CombineFlow->tempEnable);
            glmTEMPV(Arguments[0], CombineFlow->argSwizzle);
            glmTEMPV(Arguments[1], CombineFlow->argSwizzle);

        glmOPCODEV(SUB, ShaderControl->oColor, CombineFlow->targetEnable);
            glmTEMPV(temp, CombineFlow->tempSwizzle);
            glmCONST(0.5f);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexCombFuncInterpolate
**
**  INTERPOLATE texture combine function:
**
**      oColor = Arg0 * Arg2 + Arg1 * (1 ? Arg2)
**             = Arg0 * Arg2 + Arg1 - Arg1 * Arg2
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Arguments
**          Pointer to the array of input arguments.
**
**      CombineFlow
**          Data flow control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexCombFuncInterpolate(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Arguments=%u CombineFlow=0x%x",
                    Context, ShaderControl, Arguments, CombineFlow);

    do
    {
        /* Allocate temporaries. */
        glmALLOCATE_LOCAL_TEMP(temp1);
        glmALLOCATE_LOCAL_TEMP(temp2);
        glmALLOCATE_LOCAL_TEMP(temp3);

        /* temp1 = Arg0 * Arg2. */
        glmOPCODEV(MUL, temp1, CombineFlow->tempEnable);
            glmTEMPV(Arguments[0], CombineFlow->argSwizzle);
            glmTEMPV(Arguments[2], CombineFlow->argSwizzle);

        /* temp2 = Arg0 * Arg2 + Arg1. */
        glmOPCODEV(ADD, temp2, CombineFlow->tempEnable);
            glmTEMPV(temp1, CombineFlow->tempSwizzle);
            glmTEMPV(Arguments[1], CombineFlow->argSwizzle);

        /* temp3 = Arg1 * Arg2 */
        glmOPCODEV(MUL, temp3, CombineFlow->tempEnable);
            glmTEMPV(Arguments[1], CombineFlow->argSwizzle);
            glmTEMPV(Arguments[2], CombineFlow->argSwizzle);

        /* Finally compute the result. */
        glmOPCODEV(SUB, ShaderControl->oColor, CombineFlow->targetEnable);
            glmTEMPV(temp2, CombineFlow->tempSwizzle);
            glmTEMPV(temp3, CombineFlow->tempSwizzle);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexCombFuncSubtract
**
**  SUBTRACT texture combine function:
**
**      oColor = Arg0 - Arg1
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Arguments
**          Pointer to the array of input arguments.
**
**      CombineFlow
**          Data flow control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexCombFuncSubtract(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Arguments=%u CombineFlow=0x%x",
                    Context, ShaderControl, Arguments, CombineFlow);

    do
    {
        glmOPCODEV(SUB, ShaderControl->oColor, CombineFlow->targetEnable);
            glmTEMPV(Arguments[0], CombineFlow->argSwizzle);
            glmTEMPV(Arguments[1], CombineFlow->argSwizzle);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexCombFuncDot3RGB
**
**  DOT3_RGB texture combine function:
**
**      oColor.xyz = 4 * ( (Arg0r - 0.5) * (Arg1r - 0.5) +
**                         (Arg0g - 0.5) * (Arg1g - 0.5) +
**                         (Arg0b - 0.5) * (Arg1b - 0.5) )
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Arguments
**          Pointer to the array of input arguments.
**
**      CombineFlow
**          Data flow control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexCombFuncDot3RGB(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Arguments=%u CombineFlow=0x%x",
                    Context, ShaderControl, Arguments, CombineFlow);

    do
    {
        /* Allocate temporaries. */
        glmALLOCATE_LOCAL_TEMP(temp1);
        glmALLOCATE_LOCAL_TEMP(temp2);
        glmALLOCATE_LOCAL_TEMP(temp3);

        /* temp1 = Arg0 - 0.5 */
        glmOPCODE(SUB, temp1, XYZ);
            glmTEMP(Arguments[0], XYZZ);
            glmCONST(0.5f);

        /* temp2 = Arg1 - 0.5 */
        glmOPCODE(SUB, temp2, XYZ);
            glmTEMP(Arguments[1], XYZZ);
            glmCONST(0.5f);

        /* temp3 = dp3(temp1, temp2) */
        glmOPCODE(DP3, temp3, X);
            glmTEMP(temp1, XYZZ);
            glmTEMP(temp2, XYZZ);

        /* oColor = dp3 * 4 */
        glmOPCODEV(MUL, ShaderControl->oColor, CombineFlow->targetEnable);
            glmTEMP(temp3, XXXX);
            glmCONST(4);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexCombFuncDot3RGBA
**
**  DOT3_RGBA texture combine function:
**
**      oColor.xyzw = 4 * ( (Arg0r - 0.5) * (Arg1r - 0.5) +
**                          (Arg0g - 0.5) * (Arg1g - 0.5) +
**                          (Arg0b - 0.5) * (Arg1b - 0.5) )
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Arguments
**          Pointer to the array of input arguments.
**
**      CombineFlow
**          Data flow control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexCombFuncDot3RGBA(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    gctUINT16_PTR Arguments,
    glsCOMBINEFLOW_PTR CombineFlow
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Arguments=%u CombineFlow=0x%x",
                    Context, ShaderControl, Arguments, CombineFlow);

    do
    {
        /* Allocate temporaries. */
        glmALLOCATE_LOCAL_TEMP(temp1);
        glmALLOCATE_LOCAL_TEMP(temp2);
        glmALLOCATE_LOCAL_TEMP(temp3);

        /* temp1 = Arg0 - 0.5 */
        glmOPCODE(SUB, temp1, XYZ);
            glmTEMP(Arguments[0], XYZZ);
            glmCONST(0.5f);

        /* temp2 = Arg1 - 0.5 */
        glmOPCODE(SUB, temp2, XYZ);
            glmTEMP(Arguments[1], XYZZ);
            glmCONST(0.5f);

        /* temp3 = dp3(temp1, temp2) */
        glmOPCODE(DP3, temp3, X);
            glmTEMP(temp1, XYZZ);
            glmTEMP(temp2, XYZZ);

        /* oColor = dp3 * 4 */
        glmOPCODE(MUL, ShaderControl->oColor, XYZW);
            glmTEMP(temp3, XXXX);
            glmCONST(4);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}

static gctBOOL _CheckDoClamp(
    glsFSCONTROL_PTR ShaderControl,
    IN glsTEXTURECOMBINE_PTR Combine,
    IN gctUINT CombineCount
    )
{
    gctBOOL doClamp = gcvFALSE;

    gcmHEADER_ARG("ShaderControl=0x%x Combine=0x%x CombineCount=%u",
                    ShaderControl, Combine, CombineCount);

    switch (Combine[0].function)
    {
    case glvCOMBINEADD       :
    case glvCOMBINEADDSIGNED :
    case glvCOMBINESUBTRACT  :
    case glvCOMBINEDOT3RGB   :
    case glvCOMBINEDOT3RGBA  :
        doClamp = gcvTRUE;
		break;
    default:
        break;
    }

    if (doClamp || CombineCount == 1)
    {
        gcmFOOTER_ARG("%d", doClamp);
        return doClamp;
    }

    switch (Combine[1].function)
    {
    case glvCOMBINEADD       :
    case glvCOMBINEADDSIGNED :
    case glvCOMBINESUBTRACT  :
    case glvCOMBINEDOT3RGB   :
    case glvCOMBINEDOT3RGBA  :
        doClamp = gcvTRUE;
		break;
    default:
        break;
    }

    gcmFOOTER_ARG("%d", doClamp);
    return doClamp;
}

/*******************************************************************************
**
**  _TexFuncCombineComponent
**
**  Perform preconfigured texture combine function.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      SamplerNumber
**          Sampler number.
**
**      TargetEnable
**          Format dependent target enable.
**
**      Combine
**          Pointer to the array of combine function descriptors.
**
**      CombineCount
**          Number of combine function descriptors in the array.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexFuncCombineComponent(
    IN glsCONTEXT_PTR Context,
    IN glsFSCONTROL_PTR ShaderControl,
    IN gctUINT SamplerNumber,
    IN gcSL_ENABLE TargetEnable,
    IN glsTEXTURECOMBINE_PTR Combine,
    IN gctUINT CombineCount
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x SamplerNumber=%u TargetEnable=0x%04x "
                    "Combine=0x%x CombineCount=%u",
                    Context, ShaderControl, SamplerNumber, TargetEnable, Combine, CombineCount);

    do
    {
        gctUINT i;

        /* Determine the scaling enable flag. */
        gctBOOL doScale = (CombineCount == 1)
            ? !Combine[0].scale.one
            : (!Combine[0].scale.one || !Combine[1].scale.one);

        /* There are four different types of sources defined. The algorithm
           will reuse a source instead of allocating a new one if the same
           source is used more then once. Therefore, we only need up to 4
           source registers for both color and alpha functions. */
        gctUINT16 sources[4];

        /* Argument map between argument and source type. */
        gctUINT16 argumentMap[4][4];

        /* Reset sources and argument map. */
        gcoOS_ZeroMemory(sources, sizeof(sources));
        gcoOS_ZeroMemory(argumentMap, sizeof(argumentMap));

        /* Loop through combine descriptors. */
        for (i = 0; i < CombineCount; i++)
        {
            /* Arguments for the current function. */
            gctUINT16 arguments[3];

            /* Make a shortcut to the current combine function descriptor. */
            glsTEXTURECOMBINE_PTR combine = &Combine[i];

            /* Make a shortcut to the current combine function. */
            glsCOMBINEFUNCTION_PTR combineFunction =
                &_CombineTextureFunctions[combine->function];

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
            /* Report texture function. */
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_TEXTURE,
                "         Pass=%s,",
                g_passName[i]
                );

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_TEXTURE,
                "             function=%s, scale factor=%.5f.",
                g_combineFunctionName[combine->function],
                glfFloatFromMutant(&combine->scale)
                );
#endif

            /* Generate function only if it produces a result. */
            if (combine->combineFlow->targetEnable & TargetEnable)
            {
                /* Allocate sources. */
                gcmERR_BREAK(_GetCombineSources(
                    Context,
                    ShaderControl,
                    SamplerNumber,
                    combine,
                    sources
                    ));

                /* Fetch source arguments. */
                gcmERR_BREAK(_GetCombineArguments(
                    Context,
                    ShaderControl,
                    SamplerNumber,
                    combine,
                    sources,
                    argumentMap,
                    arguments
                    ));

                /* Save the current color as previous. */
                ShaderControl->oPrevColor = ShaderControl->oColor;

                /* Allocate new color register. */
                glmALLOCATE_TEMP(ShaderControl->oColor);

                /* Generate function. */
                gcmERR_BREAK((*combineFunction->function)(
                    Context,
                    ShaderControl,
                    arguments,
                    combine->combineFlow
                    ));

                /* Copy the leftover part of the color. */
                if (combine->combineFlow->targetEnable == gcSL_ENABLE_XYZ)
                {
                    glmOPCODE(MOV, ShaderControl->oColor, W);
                        glmTEMP(ShaderControl->oPrevColor, WWWW);
                }
                else if (combine->combineFlow->targetEnable == gcSL_ENABLE_W)
                {
                    glmOPCODE(MOV, ShaderControl->oColor, XYZ);
                        glmTEMP(ShaderControl->oPrevColor, XYZZ);
                }
            }
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
            else
            {
                gcmTRACE_ZONE(
                    gcvLEVEL_VERBOSE, gcvZONE_TEXTURE,
                    "         Skipped."
                    );
            }
#endif

            /* Accordingly to the spec, the alpha result is ignored if RGB
               is set to be computed using DOT3RGBA function. */
            if (combine->function == glvCOMBINEDOT3RGBA)
            {
                break;
            }
        }

        /* Scale the result. */
        if (doScale)
        {
            /* Save the current color as previous. */
            ShaderControl->oPrevColor = ShaderControl->oColor;

            /* Allocate new color register. */
            glmALLOCATE_TEMP(ShaderControl->oColor);

            /* Allocate scale constant. */
            glmUSING_UNIFORM(uTexCombScale);

            glmOPCODE(MUL, ShaderControl->oColor, XYZW);
                glmTEMP(ShaderControl->oPrevColor, XYZW);
                glmUNIFORM_STATIC(FS, uTexCombScale, XYZW, SamplerNumber);
        }

        /* Schedule to clamp the final result. */
        ShaderControl->clampColor = doScale
                                    ? gcvTRUE
                                    : _CheckDoClamp(ShaderControl,
                                                    Combine,
                                                    CombineCount);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexFuncReplace
**
**  REPLACE texture function:
**
**      oColor.color = Csrc
**      oColor.alpha = Asrc
**
**      We are using REPLACE function to implement the formula:
**
**          oColor = Arg0
**
**              where Arg0 = source texture
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Sampler
**          Pointer to a sampler descriptor.
**
**      SamplerNumber
**          Sampler number.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexFuncReplace(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    glsTEXTURESAMPLER_PTR Sampler,
    gctUINT SamplerNumber
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=0x%x SamplerNumber=%u",
                    Context, ShaderControl, Sampler, SamplerNumber);

    do
    {
        static glsTEXTURECOMBINE combine =
        {
            glvCOMBINEREPLACE,

            /* Arg0                  Arg1               Arg2 */
            {glvTEXTURE,  (gleCOMBINESOURCE)  0, (gleCOMBINESOURCE)  0},
            {glvSRCCOLOR, (gleCOMBINEOPERAND) 0, (gleCOMBINEOPERAND) 0},

            /* Scale. */
            glmFIXEDMUTANTONE,

            /* Combine data flow control structure. */
            gcvNULL
        };

        /* Texture format defines the data flow. */
        combine.combineFlow = &Sampler->binding->combineFlow;

        /* Generate function. */
        gcmERR_BREAK(_TexFuncCombineComponent(
            Context,
            ShaderControl,
            SamplerNumber,
            Sampler->binding->combineFlow.targetEnable,
            &combine,
            1
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexFuncModulate
**
**  MODULATE texture function:
**
**      oColor.color = Cprev * Csrc
**      oColor.alpha = Aprev * Asrc
**
**      We are using MODULATE function to implement the formula:
**
**          oColor = Arg0 * Arg1
**
**              where Arg0 = previous result,
**                    Arg1 = texture value.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Sampler
**          Pointer to a sampler descriptor.
**
**      SamplerNumber
**          Sampler number.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexFuncModulate(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    glsTEXTURESAMPLER_PTR Sampler,
    gctUINT SamplerNumber
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=0x%x SamplerNumber=%u",
                    Context, ShaderControl, Sampler, SamplerNumber);

    do
    {
        static glsTEXTURECOMBINE combine =
        {
            glvCOMBINEMODULATE,

            /* Arg0          Arg1              Arg2 */
            {glvPREVIOUS, glvTEXTURE,  (gleCOMBINESOURCE)  0},
            {glvSRCCOLOR, glvSRCCOLOR, (gleCOMBINEOPERAND) 0},

            /* Scale. */
            glmFIXEDMUTANTONE,

            /* Combine data flow control structure. */
            gcvNULL
        };

        /* Texture format defines the data flow. */
        combine.combineFlow = &Sampler->binding->combineFlow;

        /* Generate function. */
        gcmERR_BREAK(_TexFuncCombineComponent(
            Context,
            ShaderControl,
            SamplerNumber,
            Sampler->binding->combineFlow.targetEnable,
            &combine,
            1
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexFuncDecal
**
**  DECAL texture function:
**
**      RGB format:
**
**          oColor.color = Csrc
**          oColor.alpha = Aprev
**
**          We are using REPLACE function to implement the formula:
**
**              oColor = Arg0
**
**                  where Arg0 = Csrc
**
**      RGBA format:
**
**          oColor.color = Cprev * (1 - Asrc) + Csrc * Asrc
**          oColor.alpha = Aprev
**
**          We are using INTERPOLATE combine function to implement the formula:
**
**              oColor.color = Arg0 * Arg2 + Arg1 * (1 ? Arg2)
**
**                  where Arg0 = Csrc
**                        Arg1 = Cprev
**                        Arg2 = Asrc
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Sampler
**          Pointer to a sampler descriptor.
**
**      SamplerNumber
**          Sampler number.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexFuncDecal(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    glsTEXTURESAMPLER_PTR Sampler,
    gctUINT SamplerNumber
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=0x%x SamplerNumber=%u",
                    Context, ShaderControl, Sampler, SamplerNumber);

    do
    {
        if (Sampler->binding->format == GL_RGB)
        {
            gcmERR_BREAK(_TexFuncReplace(
                Context,
                ShaderControl,
                Sampler,
                SamplerNumber
                ));
        }
        else if ((Sampler->binding->format == GL_RGBA) ||
                 (Sampler->binding->format == GL_BGRA_EXT))
        {
            static glsCOMBINEFLOW dataFlow =
            {
                gcSL_ENABLE_XYZ,        /* Target enable. */
                gcSL_ENABLE_XYZ,        /* Temp enable. */
                gcSL_SWIZZLE_XYZZ,      /* Temp swizzle. */
                gcSL_SWIZZLE_XYZZ       /* Source argument swizzle. */
            };

            static glsTEXTURECOMBINE combine =
            {
                glvCOMBINEINTERPOLATE,

                /* Arg0       Arg1         Arg2 */
                {glvTEXTURE,  glvPREVIOUS, glvTEXTURE},
                {glvSRCCOLOR, glvSRCCOLOR, glvSRCALPHA},

                /* Scale. */
                glmFIXEDMUTANTONE,

                /* Combine data flow control structure. */
                &dataFlow
            };

            /* Generate function. */
            gcmERR_BREAK(_TexFuncCombineComponent(
                Context,
                ShaderControl,
                SamplerNumber,
                gcSL_ENABLE_XYZW,
                &combine,
                1
                ));
        }
        else
        {
            /* For the rest of the formats the result is undefined. */
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
**  _TexFuncBlend
**
**  BLEND texture function:
**
**      oColor.color = Cprev * (1 - Csrc) + Cconst * Csrc
**
**          We are using INTERPOLATE combine function to implement the formula:
**
**          oColor.color = Arg0 * Arg2 + Arg1 * (1 ? Arg2)
**
**              where Arg0 = Cconst
**                    Arg1 = Cprev
**                    Arg2 = Csrc
**
**      oColor.alpha = Aprev * Asrc
**
**          We are using MODULATE combine function to implement the formula:
**
**          oColor.color = Arg0 * Arg1
**
**              where Arg0 = Aprev
**                    Arg1 = Asrc
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Sampler
**          Pointer to a sampler descriptor.
**
**      SamplerNumber
**          Sampler number.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexFuncBlend(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    glsTEXTURESAMPLER_PTR Sampler,
    gctUINT SamplerNumber
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=0x%x SamplerNumber=%u",
                    Context, ShaderControl, Sampler, SamplerNumber);

    do
    {
        static glsCOMBINEFLOW colorDataFlow =
        {
            gcSL_ENABLE_XYZ,        /* Target enable. */
            gcSL_ENABLE_XYZ,        /* Temp enable. */
            gcSL_SWIZZLE_XYZZ,      /* Temp swizzle. */
            gcSL_SWIZZLE_XYZZ       /* Source argument swizzle. */
        };

        static glsCOMBINEFLOW alphaDataFlow =
        {
            gcSL_ENABLE_W,          /* Target enable. */
            gcSL_ENABLE_X,          /* Temp enable. */
            gcSL_SWIZZLE_XXXX,      /* Temp swizzle. */
            gcSL_SWIZZLE_WWWW       /* Source argument swizzle. */
        };

        static glsTEXTURECOMBINE combine[] =
        {
            /* RGB function. */
            {
                glvCOMBINEINTERPOLATE,

                /* Arg0       Arg1         Arg2 */
                {glvCONSTANT, glvPREVIOUS, glvTEXTURE},
                {glvSRCCOLOR, glvSRCCOLOR, glvSRCCOLOR},

                /* Scale. */
                glmFIXEDMUTANTONE,

                /* Combine data flow control structure. */
                &colorDataFlow
            },

            /* Alpha function. */
            {
                glvCOMBINEMODULATE,

                /* Arg0       Arg1         Arg2 */
                {glvPREVIOUS, glvTEXTURE,  (gleCOMBINESOURCE)  0},
                {glvSRCALPHA, glvSRCALPHA, (gleCOMBINEOPERAND) 0},

                /* Scale. */
                glmFIXEDMUTANTONE,

                /* Combine data flow control structure. */
                &alphaDataFlow
            }
        };

        /* Generate function. */
        gcmERR_BREAK(_TexFuncCombineComponent(
            Context,
            ShaderControl,
            SamplerNumber,
            Sampler->binding->combineFlow.targetEnable,
            combine,
            gcmCOUNTOF(combine)
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexFuncAdd
**
**  ADD texture function:
**
**      oColor.color = Cprev + Csrc
**
**          We are using ADD combine function to implement the formula:
**
**          oColor.color = Arg0 + Arg1
**
**              where Arg0 = Cprev
**                    Arg1 = Csrc
**
**      oColor.alpha = Aprev * Asrc
**
**          We are using MODULATE combine function to implement the formula:
**
**          oColor.color = Arg0 * Arg1
**
**              where Arg0 = Aprev
**                    Arg1 = Asrc
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Sampler
**          Pointer to a sampler descriptor.
**
**      SamplerNumber
**          Sampler number.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexFuncAdd(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    glsTEXTURESAMPLER_PTR Sampler,
    gctUINT SamplerNumber
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=0x%x SamplerNumber=%u",
                    Context, ShaderControl, Sampler, SamplerNumber);

    do
    {
        static glsCOMBINEFLOW colorDataFlow =
        {
            gcSL_ENABLE_XYZ,        /* Target enable. */
            gcSL_ENABLE_XYZ,        /* Temp enable. */
            gcSL_SWIZZLE_XYZZ,      /* Temp swizzle. */
            gcSL_SWIZZLE_XYZZ       /* Source argument swizzle. */
        };

        static glsCOMBINEFLOW alphaDataFlow =
        {
            gcSL_ENABLE_W,          /* Target enable. */
            gcSL_ENABLE_X,          /* Temp enable. */
            gcSL_SWIZZLE_XXXX,      /* Temp swizzle. */
            gcSL_SWIZZLE_WWWW       /* Source argument swizzle. */
        };

        static glsTEXTURECOMBINE combine[] =
        {
            /* RGB function. */
            {
                glvCOMBINEADD,

                /* Arg0       Arg1         Arg2 */
                {glvPREVIOUS, glvTEXTURE,  (gleCOMBINESOURCE)  0},
                {glvSRCCOLOR, glvSRCCOLOR, (gleCOMBINEOPERAND) 0},

                /* Scale. */
                glmFIXEDMUTANTONE,

                /* Combine data flow control structure. */
                &colorDataFlow
            },

            /* Alpha function. */
            {
                glvCOMBINEMODULATE,

                /* Arg0       Arg1         Arg2 */
                {glvPREVIOUS, glvTEXTURE,  (gleCOMBINESOURCE)  0},
                {glvSRCALPHA, glvSRCALPHA, (gleCOMBINEOPERAND) 0},

                /* Scale. */
                glmFIXEDMUTANTONE,

                /* Combine data flow control structure. */
                &alphaDataFlow
            }
        };

        /* Generate function. */
        gcmERR_BREAK(_TexFuncCombineComponent(
            Context,
            ShaderControl,
            SamplerNumber,
            Sampler->binding->combineFlow.targetEnable,
            combine,
            gcmCOUNTOF(combine)
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _TexFuncCombine
**
**  COMBINE texture function.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the current shader object.
**
**      Sampler
**          Pointer to a sampler descriptor.
**
**      SamplerNumber
**          Sampler number.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _TexFuncCombine(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl,
    glsTEXTURESAMPLER_PTR Sampler,
    gctUINT SamplerNumber
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x Sampler=0x%x SamplerNumber=%u",
                    Context, ShaderControl, Sampler, SamplerNumber);

    do
    {
        glsTEXTURECOMBINE combine[2];

        /* Set combine functions. */
        combine[0] = Sampler->combColor;
        combine[1] = Sampler->combAlpha;

        /* Generate function. */
        gcmERR_BREAK(_TexFuncCombineComponent(
            Context,
            ShaderControl,
            SamplerNumber,
            gcSL_ENABLE_XYZW,
            combine,
            gcmCOUNTOF(combine)
            ));
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


#if gcmIS_DEBUG(gcdDEBUG_TRACE)
/*******************************************************************************
**
**  _GetTextureFormatName
**
**  Return the string name of the specified texture format.
**
**  INPUT:
**
**      Format
**          Enumerated GL name.
**
**  OUTPUT:
**
**      String name.
*/

static char* _GetTextureFormatName(
    GLenum Format
    )
{
    gcmHEADER_ARG("Format=0x%04x", Format);

    if ((Format >= GL_ALPHA) && (Format <= GL_LUMINANCE_ALPHA))
    {
        static char* _textureFormats[] =
        {
            "GL_ALPHA",
            "GL_RGB",
            "GL_RGBA",
            "GL_LUMINANCE",
            "GL_LUMINANCE_ALPHA"
        };

        gcmFOOTER_ARG("result=0x%x", _textureFormats[Format - GL_ALPHA]);

        return _textureFormats[Format - GL_ALPHA];
    }
    else
    {
        gcmASSERT(Format == GL_BGRA_EXT);
        gcmFOOTER_ARG("result=0x%x", "GL_BGRA_EXT");
        return "GL_BGRA_EXT";
    }
}
#endif


/*******************************************************************************
**
**  _ProcessTexture
**
**  Generate texture stage functions for enabled samplers.
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

static gceSTATUS _ProcessTexture(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    GLint i;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    if (Context->drawClearRectEnabled)
    {
        gcmFOOTER();
        return status;
    }

    for (i = 0; i < Context->texture.pixelSamplers; i++)
    {
        glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[i];

        if (sampler->stageEnabled)
        {
            gcmASSERT(sampler->binding != gcvNULL);

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
            /* Report texture function. */
            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_TEXTURE,
                "[FS11] Stage=%d, format=%s,",
                i,
                _GetTextureFormatName(sampler->binding->format)
                );

            gcmTRACE_ZONE(
                gcvLEVEL_VERBOSE, gcvZONE_TEXTURE,
                "       texture function=%s.",
                g_functionName[sampler->function]
                );
#endif

            /* Generate function. */
            gcmERR_BREAK((*_TextureFunctions[sampler->function])(
                Context,
                ShaderControl,
                sampler,
                i
                ));
        }
    }

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _ProcessFog
**
**  Fog blends a fog color with a rasterized fragment's post-texturing
**  color using a blending factor. The blending factor is computed and passed
**  on to FS from VS. If Cr represents a rasterized fragment’s R, G, or B
**  value, then the corresponding value produced by fog is
**
**      C = f * Cr + (1 - f) * Cf,
**
**  The rasterized fragment’s A value is not changed by fog blending.
**  The R, G, B, and A values of Cf are specified by calling Fog with name
**  equal to FOG_COLOR.
**
**  We compute the color using transformed formula:
**
**      C = f * Cr + (1 - f) * Cf = f * Cr + Cf - f * Cf
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
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Allocate temporaries. */
        glmALLOCATE_LOCAL_TEMP(eyeDistance);
        glmALLOCATE_LOCAL_TEMP(fogFactor);
        glmALLOCATE_LOCAL_TEMP(temp1);
        glmALLOCATE_LOCAL_TEMP(temp2);
        glmALLOCATE_LOCAL_TEMP(temp3);

        /* Allocate resources. */
        glmUSING_UNIFORM(uFogFactors);
        glmUSING_UNIFORM(uFogColor);
        glmUSING_VARYING(vEyePosition);

        /***********************************************************************
        ** Compute the distance from the eye to the fragment. Per spec there
        ** is a proper way to do it by computing the actual distance from the
        ** fragment to the eye and then there is also an easy way out - to take
        ** Z value of the fragment position for the distance. It seems most
        ** vendors take the easy way, so we'll do the same here.
        */

        /* Take the absolute Z value. */
        glmOPCODE(ABS, eyeDistance, X);
            glmVARYING(FS, vEyePosition, XXXX);

        /***********************************************************************
        ** Compute the fog factor.
        */

        if (Context->fogStates.mode == glvLINEARFOG)
        {
#if defined(COMMON_LITE)
            /*******************************************************************
            ** In COMMON_LIGHT profile with fixed point we cannot precompute
            ** linear coefficients because of precision loss, which will
            ** make the conformance test fail.
            **
            ** factor = (end - c) / range
            **
            ** uFogFactors.x = end
            ** uFogFactors.y = range
            */

            /* Allocate temporaries. */
            glmALLOCATE_LOCAL_TEMP(_temp1);
            glmALLOCATE_LOCAL_TEMP(_temp2);
            glmALLOCATE_LOCAL_TEMP(_temp3);

            /* _temp1.x = fogEnd - c */
            glmOPCODE(SUB, _temp1, X);
                glmUNIFORM(FS, uFogFactors, XXXX);
                glmTEMP(eyeDistance, XXXX);

            /* _temp2.x = 1 / fogRange */
            glmOPCODE(RCP, _temp2, X);
                glmUNIFORM(FS, uFogFactors, YYYY);

            /* _temp3.x = _temp1.x * _temp2.x */
            glmOPCODE(MUL, _temp3, X);
                glmTEMP(_temp1, XXXX);
                glmTEMP(_temp2, XXXX);

            /* fogFactor = sat(_temp3.x) */
            glmOPCODE(SAT, fogFactor, X);
                glmTEMP(_temp3, XXXX);
#else
            /*******************************************************************
            ** factor = c * [(1 / (s - e))] + [e / (e - s)]
            **
            ** uFogFactors.x = [(1 / (s - e))]
            ** uFogFactors.y = [ e / (e - s) ]
            */

            /* Allocate temporaries. */
            glmALLOCATE_LOCAL_TEMP(_temp1);
            glmALLOCATE_LOCAL_TEMP(_temp2);

            /* _temp1.x = c * [(1 / (s - e))] */
            glmOPCODE(MUL, _temp1, X);
                glmTEMP(eyeDistance, XXXX);
                glmUNIFORM(FS, uFogFactors, XXXX);

            /* _temp2.x = c * [(1 / (s - e))] + [e / (e - s) */
            glmOPCODE(ADD, _temp2, X);
                glmTEMP(_temp1, XXXX);
                glmUNIFORM(FS, uFogFactors, YYYY);

            /* fogFactor = sat(_temp3.x) */
            glmOPCODE(SAT, fogFactor, X);
                glmTEMP(_temp2, XXXX);
#endif
        }
        else
        {
            /*******************************************************************
            **            -(density * c)
            ** factor = e
            **
            ** or
            **            -(density * c)^2
            ** factor = e
            **
            ** uFogFactors.x = density
            */

            /* Allocate temporaries. */
            glmALLOCATE_LOCAL_TEMP(_temp1);
            glmDEFINE_TEMP        (_temp2);
            glmALLOCATE_LOCAL_TEMP(_temp3);
            glmALLOCATE_LOCAL_TEMP(_temp4);

            /* Allocate resources. */
            glmUSING_UNIFORM(uFogFactors);

            /* _temp1.x = density * c */
            glmOPCODE(MUL, _temp1, X);
                glmUNIFORM(FS, uFogFactors, XXXX);
                glmTEMP(eyeDistance, XXXX);

            /* Non-squared case? */
            if (Context->fogStates.mode == glvEXPFOG)
            {
                /* Yes, just assign the register. */
                _temp2 = _temp1;
            }
            else
            {
                /* Allocate another temp. */
                glmALLOCATE_TEMP(_temp2);

                /* _temp2.x = _temp1.x * _temp1.x */
                glmOPCODE(MUL, _temp2, X);
                    glmTEMP(_temp1, XXXX);
                    glmTEMP(_temp1, XXXX);
            }

            /* _temp3.x = -_temp2.x */
            glmOPCODE(SUB, _temp3, X);
                glmCONST(0);
                glmTEMP(_temp2, XXXX);

            /* _temp4.x = exp(_temp3.x) */
            glmOPCODE(EXP, _temp4, X);
                glmTEMP(_temp3, XXXX);

            /* fogFactor = sat(_temp4.x) */
            glmOPCODE(SAT, fogFactor, X);
                glmTEMP(_temp4, XXXX);
        }

        /***********************************************************************
        ** Blend the colors.
        */

        /* Clamp color. */
        gcmERR_BREAK(_ClampColor(Context, ShaderControl));

        /* Allocate a temp for previous color value. */
        ShaderControl->oPrevColor = ShaderControl->oColor;

        /* Allocate color output. */
        glmALLOCATE_TEMP(ShaderControl->oColor);

        /* temp1 = f * Cr. */
        glmOPCODE(MUL, temp1, XYZ);
            glmTEMP(fogFactor, XXXX);
            glmTEMP(ShaderControl->oPrevColor, XYZZ);

        /* temp2 = f * Cr + Cf. */
        glmOPCODE(ADD, temp2, XYZ);
            glmTEMP(temp1, XYZZ);
            glmUNIFORM(FS, uFogColor, XYZZ);

        /* temp3 = f * Cf */
        glmOPCODE(MUL, temp3, XYZ);
            glmTEMP(fogFactor, XXXX);
            glmUNIFORM(FS, uFogColor, XYZZ);

        /* Finally compute the result. */
        glmOPCODE(SUB, ShaderControl->oColor, XYZ);
            glmTEMP(temp2, XYZZ);
            glmTEMP(temp3, XYZZ);

        /* Copy the alpha component. */
        glmOPCODE(MOV, ShaderControl->oColor, W);
            glmTEMP(ShaderControl->oPrevColor, WWWW);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _ProcessPointSmooth
**
**  Compute the alpha value for antialiased points.
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

static gceSTATUS _ProcessPointSmooth(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        GLint i;

        /* Allocate temporaries. */
        glmALLOCATE_LOCAL_TEMP(temp1);
        glmALLOCATE_LOCAL_TEMP(temp2);

        /* Allocate resources. */
        glmUSING_VARYING(vPosition);
        glmUSING_VARYING(vPointSmooth);

        /* Compute the distance from the center of the point to the center of
           the fragment that is currently being rendered. */
        glmOPCODE(SUB, temp1, XY);
            glmVARYING(FS, vPosition, XYYY);
            glmVARYING(FS, vPointSmooth, XYYY);

        /* Zero out Z so that we can use DP3 on X and Y. */
        glmOPCODE(MOV, temp1, Z);
            glmCONST(0);

        /* Reset the resulting alpha. */
        glmOPCODE(MOV, temp2, X);
            glmCONST(0);

        /* Iterate through all 4 corners of the current fragment. */
        for (i = 0; i < 4; i++)
        {
            /* Allocate resources. */
            glmALLOCATE_LOCAL_LABEL(lblCornerOut);
            glmALLOCATE_LOCAL_TEMP(temp3);

            /* Top left corner of the current fragment. */
            if (i == 0)
            {
                glmOPCODE(MOV, temp3, XYZW);
                    glmTEMP(temp1, XYZW);
                glmOPCODE(SUB, temp1, XY);
                    glmTEMP(temp3, XYYY);
                    glmCONST(0.5f);
            }

            /* Top right corner of the current fragment. */
            else if (i == 1)
            {
                glmOPCODE(MOV, temp3, XYZW);
                    glmTEMP(temp1, XYZW);
                glmOPCODE(ADD, temp1, X);
                    glmTEMP(temp3, XXXX);
                    glmCONST(1);
            }

            /* Bottom right corner of the current fragment. */
            else if (i == 2)
            {
                glmOPCODE(MOV, temp3, XYZW);
                    glmTEMP(temp1, XYZW);
                glmOPCODE(ADD, temp1, Y);
                    glmTEMP(temp3, YYYY);
                    glmCONST(1);
            }

            /* Bottom left corner of the current fragment. */
            else
            {
                glmOPCODE(MOV, temp3, XYZW);
                    glmTEMP(temp1, XYZW);
                glmOPCODE(SUB, temp1, X);
                    glmTEMP(temp3, XXXX);
                    glmCONST(1);
            }

            /* temp3.x = the squared distance between the center of the point
                         and the current corner of the fragment. */
            glmOPCODE(DP3, temp3, X);
                glmTEMP(temp1, XYZZ);
                glmTEMP(temp1, XYZZ);

            /* If the square of the point's radius (vPointSmooth.z) is less
               or equal then the square of the distance between the center
               of the point and the current corner of the fragment (temp3.x),
               then the corner is outside the circle. */
            glmOPCODE_BRANCH(JMP, LESS_OR_EQUAL, lblCornerOut);
                glmVARYING(FS, vPointSmooth, ZZZZ);
                glmTEMP(temp3, XXXX);

            {
                /* Add the corner's contribution to the alpha value. */
                glmOPCODE(MOV, temp3, XYZW);
                    glmTEMP(temp2, XYZW);
                glmOPCODE(ADD, temp2, X);
                    glmTEMP(temp3, XXXX);
                    glmCONST(0.25f);
            }

            /* Define label. */
            glmLABEL(lblCornerOut);
        }

        /* Set the resulting alpha. */
        glmOPCODE(MOV, ShaderControl->oColor, W);
            glmTEMP(temp2, XXXX);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _ApplyMultisampling
**
**  Apply multisampling fade factor to the final alpha value.
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

static gceSTATUS _ApplyMultisampling(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Allocate resources. */
        glmUSING_VARYING(vPointFade);

        /* Clamp color. */
        gcmERR_BREAK(_ClampColor(Context, ShaderControl));

        /* Allocate a temp for previous color value. */
        ShaderControl->oPrevColor = ShaderControl->oColor;

        /* Allocate color output. */
        glmALLOCATE_TEMP(ShaderControl->oColor);

        /* Compute the final alpha. */
        glmOPCODE(MUL, ShaderControl->oColor, W);
            glmTEMP(ShaderControl->oPrevColor, WWWW);
            glmVARYING(FS, vPointFade, XXXX);

        /* Copy the color components. */
        glmOPCODE(MOV, ShaderControl->oColor, XYZ);
            glmTEMP(ShaderControl->oPrevColor, XYZZ);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _RoundResult
**
**  Patch for conformance test where it fails in RGB8 mode because of absence
**  of the proper conversion from 32-bit floating point FS output to 8-bit RGB
**  frame buffer. At the time the problem was identified, GC500 was already
**  taped out, so we fix the problem by rouding in the shader:
**
**      oColor.color = ( INT(oColor.color * 255 + 0.5) ) / 256
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

static gceSTATUS _RoundResult(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
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

        /* Clamp color. */
        gcmERR_BREAK(_ClampColor(Context, ShaderControl));

        /* Allocate a temp for previous color value. */
        ShaderControl->oPrevColor = ShaderControl->oColor;

        /* Allocate color output. */
        glmALLOCATE_TEMP(ShaderControl->oColor);

        /* Convert to 0..255 range. */
        glmOPCODE(MUL, temp1, XYZ);
            glmTEMP(ShaderControl->oPrevColor, XYZZ);
            glmCONST(255.0f);

        /* Round. */
        glmOPCODE(ADD, temp2, XYZ);
            glmTEMP(temp1, XYZZ);
            glmCONST(0.5f);

        /* Remove the fraction. */
        glmOPCODE(FLOOR, temp3, XYZ);
            glmTEMP(temp2, XYZZ);

        /* Convert back to 0..1 range. */
        glmOPCODE(MUL, ShaderControl->oColor, XYZ);
            glmTEMP(temp3, XYZZ);
            glmCONST(1.0f / 256.0f);

        /* Copy the alpha component. */
        glmOPCODE(MOV, ShaderControl->oColor, W);
            glmTEMP(ShaderControl->oPrevColor, WWWW);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _MapOutput
**
**  Map color output.
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

static gceSTATUS _MapOutput(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    do
    {
        /* Assign output. */
        glmASSIGN_OUTPUT(oColor);
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/*******************************************************************************
**
**  _InitializeFS
**
**  INitialize common things for a fragment shader.
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

static gceSTATUS _InitializeFS(
    glsCONTEXT_PTR Context,
    glsFSCONTROL_PTR ShaderControl
    )
{
    gceSTATUS status = gcvSTATUS_OK;
gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);
    do
    {
        /* Allocate the Color uniform. */
        glmUSING_UNIFORM(uColor);
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  glfGenerateFSFixedFunction
**
**  Generate Fragment Shader code.
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

gceSTATUS glfGenerateFSFixedFunction(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status;
    /* Define internal shader structure. */
    glsFSCONTROL fsControl;
    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        gcoOS_ZeroMemory(&fsControl, sizeof(glsFSCONTROL));
        fsControl.i = &Context->currProgram->fs;

        if (fsControl.i->shader == gcvNULL)
        {
            /* Shader may have been deleted. Construct again. */
            gcmONERROR(gcSHADER_Construct(
                Context->hal,
                gcSHADER_TYPE_FRAGMENT,
                &fsControl.i->shader
                ));
        }

        /* Initialize the fragment shader. */
        gcmERR_BREAK(_InitializeFS(Context, &fsControl));

        /* Handle clip plane first. */
        gcmERR_BREAK(_ProcessClipPlane(Context, &fsControl));

        /* Determine preliminary color output. */
        gcmERR_BREAK(_GetInitialColor(Context, &fsControl));

        /* Process texture. */
        gcmERR_BREAK(_ProcessTexture(Context, &fsControl));

        /* Compute the fog color. */
        if (Context->fogStates.enabled && !Context->drawTexOESEnabled)
        {
            gcmERR_BREAK(_ProcessFog(Context, &fsControl));
        }

        /* Process points. */
        if (Context->pointStates.pointPrimitive)
        {
            /* Compute point smooth. */
            if (Context->pointStates.smooth &&
                !Context->pointStates.spriteEnable)
            {
                gcmERR_BREAK(_ProcessPointSmooth(Context, &fsControl));
            }

            /* Apply multisampling alpha. */
            if (Context->multisampleStates.enabled)
            {
                gcmERR_BREAK(_ApplyMultisampling(Context, &fsControl));
            }
        }

        /* Round the result. */
        if (Context->fsRoundingEnabled)
        {
            gcmERR_BREAK(_RoundResult(Context, &fsControl));
        }

        /* Map output. */
        gcmERR_BREAK(_MapOutput(Context, &fsControl));

        /* Pack the shader. */
        gcmERR_BREAK(gcSHADER_Pack(fsControl.i->shader));

#if !GC_ENABLE_LOADTIME_OPT
        /* Optimize the shader. */
        gcmERR_BREAK(gcOptimizeShader(fsControl.i->shader, gcvNULL));
#endif  /* GC_ENABLE_LOADTIME_OPT */
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return status. */
    return status;

OnError:
    /* Destroy partially generated shader. */
    if (fsControl.i->shader)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(fsControl.i->shader));
        fsControl.i->shader = gcvNULL;
    }

    gcmFOOTER();
    /* Return status. */
    return status;
}
