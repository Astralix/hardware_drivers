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


/*******************************************************************************
** Shader generation helpers.
*/

gceSTATUS glfSetUniformFromFractions(
    IN gcUNIFORM Uniform,
    IN gltFRACTYPE X,
    IN gltFRACTYPE Y,
    IN gltFRACTYPE Z,
    IN gltFRACTYPE W
    )
{
    gltFRACTYPE vector[4];
    gceSTATUS status;
    gcmHEADER_ARG("Uniform=0x%x X=%f Y=%f Z=%f W=%f", Uniform, X, Y, Z, W);

    vector[0] = X;
    vector[1] = Y;
    vector[2] = Z;
    vector[3] = W;

    status = gcUNIFORM_SetFracValue(Uniform, 1, vector);

    gcmFOOTER();

    return status;
}

gceSTATUS glfSetUniformFromMutants(
    IN gcUNIFORM Uniform,
    IN glsMUTANT_PTR MutantX,
    IN glsMUTANT_PTR MutantY,
    IN glsMUTANT_PTR MutantZ,
    IN glsMUTANT_PTR MutantW,
    IN gltFRACTYPE* ValueArray,
    IN gctUINT Count
    )
{
    gctUINT i;
    gltFRACTYPE* vector = ValueArray;
    gceSTATUS status;
    gcmHEADER_ARG("Uniform=0x%x MutantX=0x%x MutantY=0x%x MutantZ=0x%x MutantW=0x%x "
                    "ValueArray=0x%x Count=%d",
                    Uniform, MutantX, MutantY, MutantZ, MutantW, ValueArray, Count);

    for (i = 0; i < Count; i++)
    {
        if (MutantX != gcvNULL)
        {
            glfGetFromMutant(
                &MutantX[i],
                vector++,
                glvFRACTYPEENUM
                );
        }

        if (MutantY != gcvNULL)
        {
            glfGetFromMutant(
                &MutantY[i],
                vector++,
                glvFRACTYPEENUM
                );
        }

        if (MutantZ != gcvNULL)
        {
            glfGetFromMutant(
                &MutantZ[i],
                vector++,
                glvFRACTYPEENUM
                );
        }

        if (MutantW != gcvNULL)
        {
            glfGetFromMutant(
                &MutantW[i],
                vector++,
                glvFRACTYPEENUM
                );
        }
    }

    status = gcUNIFORM_SetFracValue(Uniform, Count, ValueArray);
    gcmFOOTER();

    return status;
}

gceSTATUS glfSetUniformFromVectors(
    IN gcUNIFORM Uniform,
    IN glsVECTOR_PTR Vector,
    IN gltFRACTYPE* ValueArray,
    IN gctUINT Count
    )
{
    gctUINT i;
    gltFRACTYPE* vector = ValueArray;
    gceSTATUS status;
    gcmHEADER_ARG("Uniform=0x%x Vector=0x%x ValueArray=0x%x Count=%d",
                    Uniform, Vector, ValueArray, Count);

    for (i = 0; i < Count; i++)
    {
        glfGetFromVector4(
            &Vector[i],
            vector,
            glvFRACTYPEENUM
            );

        vector += 4;
    }

    status = gcUNIFORM_SetFracValue(Uniform, Count, ValueArray);
    gcmFOOTER();

    return status;
}

gceSTATUS glfSetUniformFromMatrix(
    IN gcUNIFORM Uniform,
    IN glsMATRIX_PTR Matrix,
    IN gltFRACTYPE* ValueArray,
    IN gctUINT MatrixCount,
    IN gctUINT ColumnCount,
    IN gctUINT RowCount
    )
{
    gctUINT i, x, y;
    gltFRACTYPE matrix[4 * 4];
    gltFRACTYPE* value = ValueArray;
    gceSTATUS status;

    gcmHEADER_ARG("Uniform=0x%x Matrix=0x%x ValueArray=0x%x MatrixCount=%d "
                    "ColumnCount=%d RowCount=%d",
                    Uniform, Matrix, ValueArray, MatrixCount, ColumnCount, RowCount);

    for (i = 0; i < MatrixCount; i++)
    {
        glfGetFromMatrix(
            &Matrix[i],
            matrix,
            glvFRACTYPEENUM
            );

        for (y = 0; y < RowCount; y++)
        {
            for (x = 0; x < ColumnCount; x++)
            {
                *value++ = matrix[y + x * 4];
            }
        }
    }

    status = gcUNIFORM_SetFracValue(Uniform, MatrixCount * RowCount, ValueArray);
    gcmFOOTER();

    return status;
}

gceSTATUS glfUsingUniform(
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN glfUNIFORMSET UniformSet,
    IN gctBOOL_PTR DirtyBit,
    IN glsUNIFORMWRAP_PTR* UniformWrap
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("ShaderControl=0x%x Name=0x%x Type=0x%04x Length=%u "
                    "UniformSet=0x%x UniformWrap=0x%x",
                    ShaderControl, Name, Type, Length, UniformSet, UniformWrap);

    do
    {
        /* Allocate if not yet allocated. */
        if (*UniformWrap == gcvNULL)
        {
            gctSIZE_T index;
            glsUNIFORMWRAP wrap;

            /* Query the number of uniforms in the shader. */
            gcmERR_BREAK(gcSHADER_GetUniformCount(
                ShaderControl->shader,
                &index
                ));

            /* Allocate a new uniform. */
            gcmERR_BREAK(gcSHADER_AddUniform(
                ShaderControl->shader,
                Name,
                Type,
                Length,
                &wrap.uniform
                ));

            /* Set the callback function. */
            wrap.set = UniformSet;

            /* Set dirty bit address. */
            wrap.dirty = DirtyBit;

            /* Add to the allocated uniform array. */
            ShaderControl->uniforms[index] = wrap;

            /* Set uniform quick access pointer. */
            *UniformWrap = &ShaderControl->uniforms[index];
        }

        /* Return success otherwise. */
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

gceSTATUS glfUsingAttribute(
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN gctBOOL IsTexture,
    IN glsATTRIBUTEINFO_PTR AttributeInfo,
    IN glsATTRIBUTEWRAP_PTR* AttributeWrap,
    IN gctINT Binding
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("ShaderControl=0x%x Name=0x%x Type=0x%04x Length=%u "
                    "IsTexture=%d AttributeInfo=0x%x AttributeWrap=0x%x",
                    ShaderControl, Name, Type, Length, IsTexture, AttributeInfo, AttributeWrap);

    do
    {
        /* Allocate if not yet allocated. */
        if (*AttributeWrap == gcvNULL)
        {
            gctSIZE_T index;
            glsATTRIBUTEWRAP wrap;

            /* Query the number of attributes in the shader. */
            gcmERR_BREAK(gcSHADER_GetAttributeCount(
                ShaderControl->shader,
                &index
                ));

            /* Allocate a new attribute. */
            gcmERR_BREAK(gcSHADER_AddAttribute(
                ShaderControl->shader,
                Name,
                Type,
                Length,
                IsTexture,
                &wrap.attribute
                ));

            /* Set stream information. */
            wrap.info = AttributeInfo;
            wrap.binding = Binding;

            /* Add to the allocated uniform array. */
            ShaderControl->attributes[index] = wrap;

            /* Set attribute quick access pointer. */
            *AttributeWrap = &ShaderControl->attributes[index];
        }

        /* Return success otherwise. */
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

gceSTATUS glfUsingVarying(
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN gctBOOL IsTexture,
    IN glsATTRIBUTEWRAP_PTR* AttributeWrap
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("ShaderControl=0x%x Name=0x%x Type=0x%04x Length=%u "
                    "IsTexture=%d AttributeWrap=0x%x",
                    ShaderControl, Name, Type, Length, IsTexture, AttributeWrap);

    status = glfUsingAttribute(
        ShaderControl,
        Name,
        Type,
        Length,
        IsTexture,
        gcvFALSE,
        AttributeWrap,
        -1
        );
    gcmFOOTER();

    return status;
}


/*******************************************************************************
**  uTexCoord constant is used in both VS and FS, define its functions here.
*/

static gceSTATUS _Set_uTexCoord(
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
        glsTEXTURESAMPLER_PTR sampler = &Context->texture.sampler[i];

        /* Only do the enabled stages. */
        if (sampler->stageEnabled)
        {
            /* Compute constant texture coordinnate. */
            if (sampler->recomputeCoord)
            {
                /* Get a shortcut to the matrix stack. */
                glsMATRIXSTACK_PTR stack =
                    &Context->matrixStackArray[glvTEXTURE_MATRIX_0 + i];

                /* Transform texture coordinate. */
                glfMultiplyVector4ByMatrix4x4(
                    &sampler->homogeneousCoord,
                    stack->topMatrix,
                    &sampler->aTexCoordInfo.currValue
                    );

                /* Reset the flag. */
                sampler->recomputeCoord = GL_FALSE;
            }

            glfGetFromVector4(
                &sampler->aTexCoordInfo.currValue,
                (gluMUTABLE_PTR) vector,
                glvFRACTYPEENUM
                );
        }

        vector += 4;
    }

    status = gcUNIFORM_SetFracValue(Uniform, glvMAX_TEXTURES, valueArray);
    gcmFOOTER();
    return status;
}

gceSTATUS glfUsing_uTexCoord(
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctBOOL_PTR DirtyBit,
    OUT glsUNIFORMWRAP_PTR* UniformWrap
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("ShaderControl=0x%x UniformWrap=0x%x", ShaderControl, UniformWrap);
    status = glfUsingUniform(
        ShaderControl,
        "uTexCoord",
        gcSHADER_FRAC_X4,
        glvMAX_TEXTURES,
        _Set_uTexCoord,
        DirtyBit,
        UniformWrap
        );
    gcmFOOTER();
    return status;
}


/*******************************************************************************
**
**  _LoadUniforms
**
**  Set the values for program uniforms.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      ShaderControl
**          Pointer to the shader control.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _LoadUniforms(
    IN glsCONTEXT_PTR Context,
    IN glsSHADERCONTROL_PTR ShaderControl,
    IN gctBOOL FlushALL
    )
{
    gceSTATUS status;
    gctSIZE_T i, uniformCount;
    gcmHEADER_ARG("Context=0x%x ShaderControl=0x%x", Context, ShaderControl);

    /* Query the number of attributes. */
    gcmONERROR(gcSHADER_GetUniformCount(
        ShaderControl->shader,
        &uniformCount
        ));

    /* Iterate though the attributes. */
    for (i = 0; i < uniformCount; i++)
    {
        /* Get current uniform. */
        glsUNIFORMWRAP_PTR wrap = &ShaderControl->uniforms[i];

        /* Set the uniform. */
        if (wrap->set != gcvNULL && (FlushALL || *(wrap->dirty)))
        {
            gcmONERROR((*wrap->set) (Context, wrap->uniform));

            *(wrap->dirty) = gcvFALSE;
        }
    }

OnError:
    gcmFOOTER();
    /* Return status. */
    return status;
}


#if gcmIS_DEBUG(gcdDEBUG_TRACE)
/*******************************************************************************
**
**  glfPrintStates
**
**  Debug printing of various states.
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

void glfPrintStates(
    IN glsCONTEXT_PTR Context
    )
{


}
#endif


/*******************************************************************************
**
**  glfLoadShader
**
**  Generate and load vertex and pixel shaders fixed functions.
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

gceSTATUS glfLoadShader(
    IN glsCONTEXT_PTR Context
    )
{
    gceSTATUS status;
    glsPROGRAMINFO_PTR program = gcvNULL;
    gcmHEADER_ARG("Context=0x%x ", Context);

    do
    {
        /* Update everything that may affect the hash key. */
        glfUpdateMatrixStates(Context);

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        /* Debug printing of various states. */
        glfPrintStates(Context);
#endif

        /* Locate the program entry. */
        gcmONERROR(glfGetHashedProgram(Context, &program));

        if (program != Context->currProgram)
        {
            Context->currProgram = program;

            /* Create new program if necessary. */
            if (Context->currProgram->programSize == 0
                || Context->currProgram->vs.shader == gcvNULL
                || Context->currProgram->fs.shader == gcvNULL)
            {
                /* Generate shaders. */
                gcmONERROR(glfGenerateVSFixedFunction(Context));
                gcmONERROR(glfGenerateFSFixedFunction(Context));

                /* Link the program. */
                gcmONERROR(gcLinkShaders(
                    Context->currProgram->vs.shader,
                    Context->currProgram->fs.shader,
                    (gceSHADER_FLAGS)
                    ( 0
                    | gcvSHADER_DEAD_CODE
                    | gcvSHADER_RESOURCE_USAGE
                    | gcvSHADER_OPTIMIZER
                    ),
                    &Context->currProgram->programSize,
                    &Context->currProgram->programBuffer,
                    &Context->currProgram->hints
                    ));
            }

            /* Send states to hardware. */
            gcmONERROR(gcLoadShaders(
                Context->hal,
                Context->currProgram->programSize,
                Context->currProgram->programBuffer,
                Context->currProgram->hints
                ));

            /* Load uniforms. */
            gcmONERROR(_LoadUniforms(Context, &Context->currProgram->vs, gcvTRUE));
            gcmONERROR(_LoadUniforms(Context, &Context->currProgram->fs, gcvTRUE));
        }
        else
        {
            /* Load uniforms. */
            gcmERR_BREAK(_LoadUniforms(Context, &Context->currProgram->vs, gcvFALSE));

	        /* For fragment shader, if clearing using shader, the clear color needs to be force flushed. */
    	    gcmERR_BREAK(_LoadUniforms(Context, &Context->currProgram->fs, Context->drawClearRectEnabled));
        }
    }
    while (gcvFALSE);

    /* Return status. */
    gcmFOOTER();
    return status;

OnError:
    /* Reset currProgram, as it may have been partially created due to an error. */
    if (Context->currProgram != gcvNULL)
    {
        if (Context->currProgram->vs.shader)
        {
            gcmVERIFY_OK(gcSHADER_Destroy(Context->currProgram->vs.shader));
            Context->currProgram->vs.shader = gcvNULL;
        }

        if (Context->currProgram->fs.shader)
        {
            gcmVERIFY_OK(gcSHADER_Destroy(Context->currProgram->fs.shader));
            Context->currProgram->fs.shader = gcvNULL;
        }

        Context->currProgram = gcvNULL;
    }
    gcmFOOTER();
    /* Return status. */
    return status;
}
