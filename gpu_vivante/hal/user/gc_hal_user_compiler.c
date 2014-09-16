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




/*
**  Common gcSL compiler module.
*/

#include "gc_hal_user_precomp.h"

#ifndef VIVANTE_NO_3D

/* Zone used for header/footer. */
#define _GC_OBJ_ZONE    gcvZONE_COMPILER

/*******************************************************************************
**  gcSHADER_SetCompilerVersion
**
**  Set the compiler version of a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctINT *Version
**          Pointer to a two word version
*/
gceSTATUS
gcSHADER_SetCompilerVersion(
    IN gcSHADER Shader,
    IN gctUINT32 *Version
    )
{
    gcmHEADER_ARG("Shader=0x%x Version=0x%x", Shader, Version);

/* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Version != gcvNULL);

    Shader->compilerVersion[0] = Version[0];
    Shader->compilerVersion[1] = Version[1];

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_CheckValidity
**
**  Check validity for a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
*/
gceSTATUS
gcSHADER_CheckValidity(
    IN gcSHADER Shader
    )
{
    gctSIZE_T instIdx;
    gcSL_INSTRUCTION code;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    for (instIdx = 0; instIdx < Shader->codeCount; instIdx ++)
    {
        code = &Shader->code[instIdx];

        /* Dst index can not be equal to source index */
        if (code->source0Index == code->tempIndex ||
            code->source1Index == code->tempIndex)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcSHADER_GetCompilerVersion
**
**  Get the compiler version of a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctUINT32_PTR *CompilerVersion.
**          Pointer to a variable for the returned compilerVersion pointer
*/
gceSTATUS
gcSHADER_GetCompilerVersion(
    IN gcSHADER Shader,
    OUT gctUINT32_PTR *CompilerVersion
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    *CompilerVersion = Shader->compilerVersion;
    gcmFOOTER_ARG("*CompilerVersion=0x%x", *CompilerVersion);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_SetConstantMemorySize
**
**  Set the constant memory address space size of a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T ConstantMemorySize
**          Constant memory size in bytes
**
**      gctCHAR *ConstantMemoryBuffer
**          Constant memory buffer
*/
gceSTATUS
gcSHADER_SetConstantMemorySize(
    IN gcSHADER Shader,
    IN gctSIZE_T ConstantMemorySize,
    IN gctCHAR * ConstantMemoryBuffer
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Shader=0x%x ConstantMemorySize=%u ConstantMemoryBuffer=0x%x",
                  Shader, ConstantMemorySize, ConstantMemoryBuffer);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    Shader->constantMemorySize = ConstantMemorySize;
    if(Shader->constantMemorySize) {
       gctPOINTER pointer;

       if(Shader->constantMemoryBuffer) {
          gcmOS_SAFE_FREE(gcvNULL, Shader->constantMemoryBuffer);
       }
       /* Allocate a new buffer to store the constants */
       status = gcoOS_Allocate(gcvNULL,
                               gcmSIZEOF(gctCHAR) * ConstantMemorySize,
                               &pointer);

       if (gcmIS_ERROR(status)) {
          /* Error. */
          gcmFATAL("gcSHADER_SetConstantMemorySize: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
          gcmFOOTER();
          return status;
       }
       Shader->constantMemoryBuffer = pointer;
       status = gcoOS_MemCopy(Shader->constantMemoryBuffer,
                              ConstantMemoryBuffer,
                              ConstantMemorySize);
       if (gcmIS_ERROR(status)) {
          /* Error. */
          gcmFATAL("gcSHADER_SetConstantMemorySize: gcoOS_MemCopy failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
          gcmFOOTER();
          return status;
       }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetConstantMemorySize
**
**  Get the gcSHADER object's constant memory size.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctSIZE_T *ConstantMemorySize.
**          Pointer to a returned shader constant memory size.
**
**      gctCHAR **ConstantMemoryBuffer.
**          Pointer to a variable for returned shader constant memory buffer.
*/
gceSTATUS
gcSHADER_GetConstantMemorySize(
    IN gcSHADER Shader,
    OUT gctSIZE_T *ConstantMemorySize,
    OUT gctCHAR **ConstantMemoryBuffer
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    *ConstantMemorySize = Shader->constantMemorySize;
    *ConstantMemoryBuffer = Shader->constantMemoryBuffer;
    gcmFOOTER_ARG("*ConstantMemorySize=%d *ConstantMemoryBuffer=0x%x",
                  *ConstantMemorySize, *ConstantMemoryBuffer);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_SetPrivateMemorySize
**
**  Set the private memory address space size of a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T PrivateMemorySize
**          Private memory size in bytes
*/
gceSTATUS
gcSHADER_SetPrivateMemorySize(
    IN gcSHADER Shader,
    IN gctSIZE_T PrivateMemorySize
    )
{
    gcmHEADER_ARG("Shader=0x%x PrivateMemorySize=%u", Shader, PrivateMemorySize);

/* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    Shader->privateMemorySize = PrivateMemorySize;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetPrivateMemorySize
**
**  Get the gcSHADER object's private memory size.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctSIZE_T *PrivateMemorySize.
**          Pointer to a returned shader private memory size.
*/
gceSTATUS
gcSHADER_GetPrivateMemorySize(
    IN gcSHADER Shader,
    OUT gctSIZE_T *PrivateMemorySize
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    *PrivateMemorySize = Shader->privateMemorySize;
    gcmFOOTER_ARG("*PrivateMemorySize=%d", *PrivateMemorySize);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_SetLocalMemorySize
**
**  Set the local memory address space size of a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T LocalMemorySize
**          Local memory size in bytes
*/
gceSTATUS
gcSHADER_SetLocalMemorySize(
    IN gcSHADER Shader,
    IN gctSIZE_T LocalMemorySize
    )
{
    gcmHEADER_ARG("Shader=0x%x LocalMemorySize=%u", Shader, LocalMemorySize);

/* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    Shader->localMemorySize = LocalMemorySize;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetLocalMemorySize
**
**  Get the gcSHADER object's local memory size.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctSIZE_T * LocalMemorySize.
**          Pointer to a returned shader local memory size.
*/
gceSTATUS
gcSHADER_GetLocalMemorySize(
    IN gcSHADER Shader,
    OUT gctSIZE_T *LocalMemorySize
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    *LocalMemorySize = Shader->localMemorySize;
    gcmFOOTER_ARG("*LocalMemorySize=%d", *LocalMemorySize);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_SetMaxKernelFunctionArgs
**
**  Set the maximum number of kernel function arguments of a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T MaxKernelFunctionArgs
**          Maximum number of kernel function arguments
*/
gceSTATUS
gcSHADER_SetMaxKernelFunctionArgs(
    IN gcSHADER Shader,
    IN gctUINT32 MaxKernelFunctionArgs
    )
{
    gcmHEADER_ARG("Shader=0x%x MaxKernelFunctionArgs=%u", Shader, MaxKernelFunctionArgs);

/* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    Shader->maxKernelFunctionArgs = (gctUINT16) MaxKernelFunctionArgs;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetType
**
**  Get the gcSHADER object's type.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctINT *Type.
**          Pointer to a returned shader type.
*/
gceSTATUS
gcSHADER_GetType(
    IN gcSHADER Shader,
    OUT gctINT *Type
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

/* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    *Type = Shader->type;
    gcmFOOTER_ARG("*Type=%d", *Type);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_Construct
**
**  Construct a new gcSHADER object.
**
**  INPUT:
**
**      gcoOS Hal
**          Pointer to an gcoHAL object.
**
**      gctINT ShaderType
**          Type of gcSHADER object to cerate.  'ShaderType' can be one of the
**          following:
**
**              gcSHADER_TYPE_VERTEX    Vertex shader.
**              gcSHADER_TYPE_FRAGMENT  Fragment shader.
**              gcSHADER_TYPE_CL  OpenCL shader.
**        gcSHADER_TYPE_PRECOMPILED
**
**  OUTPUT:
**
**      gcSHADER * Shader
**          Pointer to a variable receiving the gcSHADER object pointer.
*/
gceSTATUS
gcSHADER_Construct(
    IN gcoHAL Hal,
    IN gctINT ShaderType,
    OUT gcSHADER * Shader
    )
{
    gcSHADER shader;
    gceSTATUS status;
    gctINT vertexBase, fragmentBase;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("ShaderType=%d", ShaderType);

    /* Verify the arguments. */
    gcmDEBUG_VERIFY_ARGUMENT(Shader != gcvNULL);

    status = gcoHARDWARE_QuerySamplerBase(gcvNULL,
                                          &vertexBase,
                                          gcvNULL,
                                          &fragmentBase);
    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFATAL("gcSHADER_Construct: gcoHARDWARE_QuerySamplerBase failed status=%s", gcoOS_DebugStatus2Name(status));
        gcmFOOTER();
        return status;
    }

    /* Allocate the gcSTATUS object structure. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(struct _gcSHADER),
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFATAL("gcSHADER_Construct: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
        gcmFOOTER();
        return status;
    }

    shader = pointer;

    /* Initialize gcSHADER object. */
    shader->object.type = gcvOBJ_SHADER;

    shader->compilerVersion[0] = gcmCC('E','S','\0','\0') | (ShaderType << 16);
    shader->compilerVersion[1] = gcmCC('\0','\0','\0','\1');

    shader->maxKernelFunctionArgs = 0;
    shader->constantMemorySize = 0;
    shader->constantMemoryBuffer = gcvNULL;
    shader->privateMemorySize = 0;
    shader->localMemorySize = 0;

    /* Reset all atributes, uniforms, outputs, and code. */
    shader->type                = ShaderType;
    shader->attributeArrayCount = 0;
    shader->attributeCount      = 0;
    shader->attributes          = gcvNULL;
    shader->uniformArrayCount   = 0;
    shader->uniformCount        = 0;
    shader->uniforms            = gcvNULL;
    shader->samplerIndex        = (ShaderType == gcSHADER_TYPE_VERTEX)
                                  ? vertexBase
                                  : fragmentBase;
    shader->outputArrayCount    = 0;
    shader->outputCount         = 0;
    shader->outputs             = gcvNULL;
    shader->variableArrayCount  = 0;
    shader->variableCount       = 0;
    shader->variables           = gcvNULL;
    shader->functionArrayCount  = 0;
    shader->functionCount       = 0;
    shader->functions           = gcvNULL;
    shader->currentFunction     = gcvNULL;
    shader->kernelFunctionArrayCount  = 0;
    shader->kernelFunctionCount       = 0;
    shader->kernelFunctions           = gcvNULL;
    shader->currentKernelFunction     = gcvNULL;
    shader->codeCount           = 0;
    shader->lastInstruction     = 0;
    shader->instrIndex          = gcSHADER_OPCODE;
    shader->labels              = gcvNULL;
    shader->code                = gcvNULL;
    shader->loadUsers           = gcvNULL;
#if GC_ENABLE_LOADTIME_OPT
    shader->ltcUniformCount     = 0;
    shader->ltcUniformBegin     = 0;
    shader->ltcCodeUniformIndex = gcvNULL;
    shader->ltcInstructionCount = 0;
    shader->ltcExpressions      = gcvNULL;
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Full optimization by default */
    shader->optimizationOption  = gcvOPTIMIZATION_FULL;

    /* Return gcSHADER object pointer. */
    *Shader = shader;

    /* Success. */
    gcmFOOTER_ARG("*Shader=0x%x", *Shader);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_Clean
**
**  Clean up memory used in a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      Nothing.
*/
static gceSTATUS
_gcSHADER_Clean(
    IN gcSHADER Shader
    )
{
    gctUINT i, j;

    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Free any attributes. */
    if (Shader->attributes != gcvNULL)
    {
        for (i = 0; i < Shader->attributeCount; i++)
        {
            /* Free the gcSHADER_ATTRIBUTE structure. */
            if (Shader->attributes[i] != gcvNULL)
            {
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->attributes[i]));
            }
        }

        /* Free the array of gcSHADER_ATTRIBUTE pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->attributes));
        Shader->attributeArrayCount = 0;
        Shader->attributeCount = 0;
    }

    /* Free any uniforms. */
    if (Shader->uniforms != gcvNULL)
    {
        for (i = 0; i < Shader->uniformCount; i++)
        {
	    if(!Shader->uniforms[i]) continue;
            /* Free the gcSHADER_UNIFORM structure. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->uniforms[i]));
        }

        /* Free the array of gcSHADER_UNIFORM pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->uniforms));
        Shader->uniformArrayCount = 0;
        Shader->uniformCount = 0;
    }

    /* Free any variables. */
    if (Shader->variables != gcvNULL)
    {
        for (i = 0; i < Shader->variableCount; i++)
        {
            /* Free the gcSHADER_UNIFORM structure. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->variables[i]));
        }

        /* Free the array of gcSHADER_UNIFORM pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->variables));
        Shader->variableArrayCount = 0;
        Shader->variableCount = 0;
    }

    /* Free any outputs. */
    if (Shader->outputs != gcvNULL)
    {
        for (i = 0; i < Shader->outputCount; i++)
        {
            if (Shader->outputs[i] != gcvNULL)
            {
                /* Free the gcSHADER_OUTPUT structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->outputs[i]));
            }
        }

        /* Free the array of gcSHADER_OUTPUT pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->outputs));
        Shader->outputArrayCount = 0;
        Shader->outputCount = 0;
    }

    /* Free any functions. */
    if (Shader->functions != gcvNULL)
    {
        for (i = 0; i < Shader->functionCount; i++)
        {
            if (Shader->functions[i] != gcvNULL)
            {
                if (Shader->functions[i]->arguments != gcvNULL)
                {
                    /* Free the gcSHADER_FUNCTION_ARGUMENT structure. */
                    gcmVERIFY_OK(
                        gcmOS_SAFE_FREE(gcvNULL, Shader->functions[i]->arguments));
                }

                /* Free the gcSHADER_FUNCTION structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->functions[i]));
            }
        }

        /* Free the array of gcSHADER_FUNCTION pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->functions));
        Shader->functionArrayCount = 0;
        Shader->functionCount = 0;
    }

    /* Free any kernel functions. */
    if (Shader->kernelFunctions != gcvNULL)
    {
        for (i = 0; i < Shader->kernelFunctionCount; i++)
        {
            if (Shader->kernelFunctions[i] != gcvNULL)
            {
                if (Shader->kernelFunctions[i]->arguments != gcvNULL)
                {
                    /* Free the gcSHADER_FUNCTION_ARGUMENT structure. */
                    gcmVERIFY_OK(
                        gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]->arguments));
                }

                /* Free any uniforms. */
                if (Shader->kernelFunctions[i]->uniformArguments != gcvNULL)
                {
                    for (j = 0; j < Shader->kernelFunctions[i]->uniformArgumentCount; j++)
                    {
                        /* Free the gcUNIFORM structure. */
                        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]->uniformArguments[j]));
                    }

                    /* Free the array of gcUNIFORM pointers. */
                    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]->uniformArguments));
                    Shader->kernelFunctions[i]->uniformArgumentCount = 0;
                }

                /* Free any variables. */
                if (Shader->kernelFunctions[i]->variables != gcvNULL)
                {
                    for (j = 0; j < Shader->kernelFunctions[j]->variableCount; j++)
                    {
                        /* Free the gcVARIABLE structure. */
                        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]->variables[j]));
                    }

                    /* Free the array of gcVARIABLE pointers. */
                    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]->variables));
                    Shader->kernelFunctions[i]->variableCount = 0;
                }

                /* Free any image samplers. */
                if (Shader->kernelFunctions[i]->imageSamplers != gcvNULL)
                {
                     /* Free the array of gcsIMAGE_SAMPLER pointers. */
                    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]->imageSamplers));
                    Shader->kernelFunctions[i]->imageSamplerCount = 0;
                }

                /* Free any properties. */
                if (Shader->kernelFunctions[i]->properties != gcvNULL)
                {
                    /* Free the gcKERNEL_FUNCTION_PROPERTY structure. */
                    gcmVERIFY_OK(
                        gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]->properties));
                    Shader->kernelFunctions[i]->propertyCount = 0;
                }

                /* Free any property values. */
                if (Shader->kernelFunctions[i]->propertyValues != gcvNULL)
                {
                    /* Free the gctUINT array. */
                    gcmVERIFY_OK(
                        gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]->propertyValues));
                    Shader->kernelFunctions[i]->propertyValueCount = 0;
                }

                /* Free the gcSHADER_FUNCTION structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions[i]));
            }
        }

        /* Free the array of gcSHADER_FUNCTION pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions));
        Shader->kernelFunctionCount = 0;
    }

    /* Free any labels. */
    if (Shader->labels != gcvNULL)
    {
        while (Shader->labels != gcvNULL)
        {
            gcSHADER_LABEL label = Shader->labels;
            Shader->labels = label->next;

            while (label->referenced != gcvNULL)
            {
                /* Remove gcSHADER_LINK structure from head of list. */
                gcSHADER_LINK link = label->referenced;
                label->referenced = link->next;

                /* Free the gcSHADER_LINK structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, link));
            }

            /* Free the gcSHADER_LABEL structure. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, label));
        }
    }

    /* Free the code buffer. */
    if (Shader->code != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->code));
    }

    /* Free the load users. */
    if (Shader->loadUsers != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->loadUsers));
    }

#if GC_ENABLE_LOADTIME_OPT
    /* Free Loadtime Optimization related data */

    /* LTC uniform index */
    if (Shader->ltcCodeUniformIndex != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->ltcCodeUniformIndex));
    }

    /* LTC expressions */
    if (Shader->ltcExpressions != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->ltcExpressions));
    }
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_Destroy
**
**  Destroy a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_Destroy(
    IN gcSHADER Shader
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    _gcSHADER_Clean(Shader);

    /* Mark the gcSHADER object as unknown. */
    Shader->object.type = gcvOBJ_UNKNOWN;

    /* Free the gcSHADER object structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/** SHADER binary file header format: -
       Word 1:  gcmCC('S', 'H', 'D', 'R') shader binary file signature
       Word 2:  ('\od' '\od' '\od' '\od') od = octal digits; binary file version
       Word 3:  ('\E' '\S' '\0' '\0') | gcSHADER_TYPE_VERTEX << 16   or
                ('E' 'S' '\0' '\0') | gcSHADER_TYPE_FRAGMENT << 16 or
                ('C' 'L' '\0' '\0') | gcSHADER_TYPE_CL << 16
       Word 4: ('\od' '\od' '\od' '\od') od = octal digits; compiler version
       Word 5: ('\0' '\0' '\0' '\1') gcSL version
       Word 6: size of shader binary file in bytes excluding this header */
#define _gcdShaderBinaryHeaderSize   (6 * 4)  /*Shader binary file header size in bytes*/

gceSTATUS
gcSHADER_Copy(
    IN gcSHADER Shader,
    IN gcSHADER Source
    )
{
    gceSTATUS status;
    gctSIZE_T i, bytes;
    gcSHADER_LABEL srcLabel, label;
    gcSHADER_LINK srcReference, reference;

    gcmHEADER_ARG("Shader=0x%x Source=0x%x", Shader, Source);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(Source, gcvOBJ_SHADER);

    /* Free the resources. */
    gcmONERROR(_gcSHADER_Clean(Shader));

    /* Copy the shader type. */
    Shader->type               = Source->type;
    Shader->optimizationOption = Source->optimizationOption;
    Shader->compilerVersion[0] = Source->compilerVersion[0];
    Shader->compilerVersion[1] = Source->compilerVersion[1];

    /***** Attributes *********************************************************/

    if (Source->attributeCount > 0)
    {
        /* Allocate the attribute array. */
        gcmONERROR(gcoOS_Allocate(
            gcvNULL,
            Source->attributeCount * gcmSIZEOF(gcATTRIBUTE),
            (gctPOINTER *) &Shader->attributes));

        /* Zero the memory. */
        gcmONERROR(gcoOS_ZeroMemory(
            Shader->attributes,
            Source->attributeCount * gcmSIZEOF(gcATTRIBUTE)));

        /* Copy all attributes. */
        for (i = 0; i < Source->attributeCount; i++)
        {
            /* CHeck if we have a valid source attribute. */
            if (Source->attributes[i] != gcvNULL)
            {
                /* Get the number of bytes for this attribute, based on the
                ** name. */
                if ((gctINT) Source->attributes[i]->nameLength < 0)
                {
                    /* Predefined name. */
                    bytes = gcmOFFSETOF(_gcATTRIBUTE, name);
                }
                else
                {
                    /* User name. */
                    bytes = gcmOFFSETOF(_gcATTRIBUTE, name)
                          + Source->attributes[i]->nameLength
                          + 1;
                }

                /* Allocate the attribute. */
                gcmONERROR(gcoOS_Allocate(
                    gcvNULL,
                    bytes,
                    (gctPOINTER *) &Shader->attributes[i]));

                /* Copy the attribute. */
                gcmONERROR(gcoOS_MemCopy(Shader->attributes[i],
                                         Source->attributes[i],
                                         bytes));
            }
        }
    }

    /* Copy the attribute count. */
    Shader->attributeArrayCount = Source->attributeCount;
    Shader->attributeCount      = Source->attributeCount;

    /***** Uniforms ***********************************************************/

    if (Source->uniformCount > 0)
    {
        /* Allocate the uniform array. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  Source->uniformCount * gcmSIZEOF(gcUNIFORM),
                                  (gctPOINTER *) &Shader->uniforms));

        /* Zero the memory. */
        gcmONERROR(gcoOS_ZeroMemory(
            Shader->uniforms,
            Source->uniformCount * gcmSIZEOF(gcUNIFORM)));

        /* Copy all uniforms. */
        for (i = 0; i < Source->uniformCount; i++)
        {
            /* CHeck if we have a valid source uniform. */
            if (Source->uniforms[i] != gcvNULL)
            {
                /* Get the number of bytes for this uniform. */
                bytes = gcmOFFSETOF(_gcUNIFORM, name)
                      + Source->uniforms[i]->nameLength
                      + 1;

                /* Allocate the uniform. */
                gcmONERROR(gcoOS_Allocate(gcvNULL,
                                          bytes,
                                          (gctPOINTER *) &Shader->uniforms[i]));

                /* Copy the uniform. */
                gcmONERROR(gcoOS_MemCopy(Shader->uniforms[i],
                                         Source->uniforms[i],
                                         bytes));
            }
            else
            {
                /* Not a valid uniform. */
                Shader->uniforms[i] = gcvNULL;
            }
        }
    }

    /* Copy the uniform count. */
    Shader->uniformArrayCount = Source->uniformCount;
    Shader->uniformCount      = Source->uniformCount;
    Shader->samplerIndex      = Source->samplerIndex;

    /***** Outputs ************************************************************/

    if (Source->outputCount > 0)
    {
        /* Allocate the output array. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  Source->outputCount * gcmSIZEOF(gcOUTPUT),
                                  (gctPOINTER *) &Shader->outputs));

        /* Zero the memory. */
        gcmONERROR(gcoOS_ZeroMemory(
            Shader->outputs,
            Source->outputCount * gcmSIZEOF(gcOUTPUT)));

        /* Copy all outputs. */
        for (i = 0; i < Source->outputCount; i++)
        {
            /* CHeck if we have a valid source output. */
            if (Source->outputs[i] != gcvNULL)
            {
                /* Get the number of bytes for this output, based on the
                ** name. */
                if ((gctINT) Source->outputs[i]->nameLength < 0)
                {
                    /* Predefined name. */
                    bytes = gcmOFFSETOF(_gcOUTPUT, name);
                }
                else
                {
                    /* User name. */
                    bytes = gcmOFFSETOF(_gcOUTPUT, name)
                          + Source->outputs[i]->nameLength
                          + 1;
                }

                /* Allocate the output. */
                gcmONERROR(gcoOS_Allocate(gcvNULL,
                                          bytes,
                                          (gctPOINTER *) &Shader->outputs[i]));

                /* Copy the output. */
                gcmONERROR(gcoOS_MemCopy(Shader->outputs[i],
                                         Source->outputs[i],
                                         bytes));
            }
            else
            {
                /* Not a valid output. */
                Shader->outputs[i] = gcvNULL;
            }
        }
    }

    /* Copy the output count. */
    Shader->outputArrayCount = Source->outputCount;
    Shader->outputCount      = Source->outputCount;

    /***** Variables **********************************************************/

    if (Source->variableCount > 0)
    {
        /* Allocate the variable array. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  Source->variableCount * gcmSIZEOF(gcVARIABLE),
                                  (gctPOINTER *) &Shader->variables));

        /* Zero the memory. */
        gcmONERROR(gcoOS_ZeroMemory(
            Shader->variables,
            Source->variableCount * gcmSIZEOF(gcVARIABLE)));

        /* Copy all variables. */
        for (i = 0; i < Source->variableCount; i++)
        {
            /* CHeck if we have a valid source variable. */
            if (Source->variables[i] != gcvNULL)
            {
                /* Get the number of bytes for this variable. */
                bytes = gcmOFFSETOF(_gcVARIABLE, name)
                      + Source->variables[i]->nameLength
                      + 1;

                /* Allocate the variable. */
                gcmONERROR(gcoOS_Allocate(
                    gcvNULL,
                    bytes,
                    (gctPOINTER *) &Shader->variables[i]));

                /* Copy the variable. */
                gcmONERROR(gcoOS_MemCopy(Shader->variables[i],
                                         Source->variables[i],
                                         bytes));
            }
            else
            {
                /* Not a valid variable. */
                Shader->variables[i] = gcvNULL;
            }
        }
    }

    /* Copy the variable count. */
    Shader->variableArrayCount = Source->variableCount;
    Shader->variableCount      = Source->variableCount;

    /***** Functions **********************************************************/

    if (Source->functionCount > 0)
    {
        /* Allocate the function array. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  Source->functionCount * gcmSIZEOF(gcFUNCTION),
                                  (gctPOINTER *) &Shader->functions));

        /* Zero the memory. */
        gcmONERROR(gcoOS_ZeroMemory(
            Shader->functions,
            Source->functionCount * gcmSIZEOF(gcFUNCTION)));

        /* Copy all functions. */
        for (i = 0; i < Source->functionCount; i++)
        {
            /* CHeck if we have a valid source function. */
            if (Source->functions[i] != gcvNULL)
            {
                /* Get the number of bytes for this function. */
                bytes = gcmOFFSETOF(_gcsFUNCTION, name)
                      + Source->functions[i]->nameLength
                      + 1;

                /* Allocate the function. */
                gcmONERROR(gcoOS_Allocate(
                    gcvNULL,
                    bytes,
                    (gctPOINTER *) &Shader->functions[i]));

                /* Copy the function. */
                gcmONERROR(gcoOS_MemCopy(Shader->functions[i],
                                         Source->functions[i],
                                         bytes));

                /* Copy the argument count. */
                Shader->functions[i]->argumentArrayCount =
                    Source->functions[i]->argumentCount;
                Shader->functions[i]->argumentCount      =
                    Source->functions[i]->argumentCount;

                /* Allocate the arguments. */
                if (Shader->functions[i]->argumentCount > 0)
                {
                    /* Get the number of bytes for the arguments. */
                    bytes = Shader->functions[i]->argumentCount
                          * gcmSIZEOF(gcsFUNCTION_ARGUMENT);

                    /* Allocate the arguments. */
                    gcmONERROR(gcoOS_Allocate(
                        gcvNULL,
                        bytes,
                        (gctPOINTER *) &Shader->functions[i]->arguments));

                    /* Copy the arguments. */
                    gcmONERROR(gcoOS_MemCopy(Shader->functions[i]->arguments,
                                             Source->functions[i]->arguments,
                                             bytes));
                }
                else
                {
                    /* No arguments. */
                    Shader->functions[i]->arguments = gcvNULL;
                }

                /* Determine the current function. */
                if (Source->currentFunction == Source->functions[i])
                {
                    Shader->currentFunction = Shader->functions[i];
                }
            }
            else
            {
                /* Not a valid variable. */
                Shader->variables[i] = gcvNULL;
            }
        }
    }

    /* Copy the function count. */
    Shader->functionArrayCount = Source->functionCount;
    Shader->functionCount      = Source->functionCount;
    Shader->currentFunction    = Source->currentFunction;

    /***** Labels *************************************************************/

    /* Copy all labels. */
    for (srcLabel = Source->labels;
         srcLabel != gcvNULL;
         srcLabel = srcLabel->next
    )
    {
        /* Allocate the label. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  gcmSIZEOF(struct _gcSHADER_LABEL),
                                  (gctPOINTER *) &label));

        /* Copy the label. */
        label->next       = Shader->labels;
        label->label      = srcLabel->label;
        label->defined    = srcLabel->defined;
        label->referenced = gcvNULL;
        Shader->labels    = label;

        /* Copy all references. */
        for (srcReference = srcLabel->referenced;
             srcReference != gcvNULL;
             srcReference = srcReference->next
        )
        {
            /* Allocate the reference. */
            gcmONERROR(gcoOS_Allocate(gcvNULL,
                                      gcmSIZEOF(struct _gcSHADER_LINK),
                                      (gctPOINTER *) &reference));

            /* Copy the reference. */
            reference->next       = label->referenced;
            reference->referenced = srcReference->referenced;
            label->referenced     = reference;
        }
    }

    /***** Code ***************************************************************/

    /* Copy the code count. */
    Shader->codeCount       = Source->codeCount;
    Shader->lastInstruction = Source->lastInstruction;
    Shader->instrIndex      = Source->instrIndex;

    /* Copy the instructions. */
    if (Shader->codeCount > 0)
    {
        /* Get the number of bytes for all the instructions. */
        bytes = Shader->codeCount * gcmSIZEOF(struct _gcSL_INSTRUCTION);

        /* Allocate the instructions. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  bytes,
                                  (gctPOINTER *) &Shader->code));

        /* Copy the instructions. */
        gcmONERROR(gcoOS_MemCopy(Shader->code, Source->code, bytes));
    }

#if GC_ENABLE_LOADTIME_OPT
    /* Loadtime Optimization related data */

    /* LTC uniform index */
    Shader->ltcUniformCount = Source->ltcUniformCount;

    Shader->ltcUniformBegin = Source->ltcUniformBegin;

    /* LTC expressions */
    Shader->ltcInstructionCount = Source->ltcInstructionCount;

    if (Shader->ltcInstructionCount > 0)
    {
        bytes = sizeof(*Shader->ltcCodeUniformIndex) * Shader->ltcInstructionCount;

        /* Allocate memory for  ltcCodeUniformIndex inside the gcSHADER object. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  bytes,
                                  (gctPOINTER *) &Shader->ltcCodeUniformIndex));

        /* Copy the ltcCodeUniformIndex. */
        gcmONERROR(gcoOS_MemCopy(Shader->ltcCodeUniformIndex,
                                 Source->ltcCodeUniformIndex,
                                 bytes));
        bytes = sizeof(*Shader->ltcExpressions) * Shader->ltcInstructionCount;
        /* Allocate memory for  ltcExpressions inside the gcSHADER object. */
        gcmONERROR(gcoOS_Allocate(gcvNULL,
                                  bytes,
                                  (gctPOINTER *) &Shader->ltcExpressions));

        /* Copy the ltcExpressions. */
        gcmONERROR(gcoOS_MemCopy(Shader->ltcExpressions,
                                 Source->ltcExpressions,
                                 bytes));
    }
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcSHADER_LoadHeader
**
**  Load a gcSHADER object from a binary buffer.  The binary buffer is layed out
**  as follows:
**      // Six word header
**      // Signature, must be 'S','H','D','R'.
**      gctINT8             signature[4];
**      gctUINT32           binFileVersion;
**      gctUINT32           compilerVersion[2];
**      gctUINT32           gcSLVersion;
**      gctUINT32           binarySize;
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**          Shader type will be returned if type in shader object is not gcSHADER_TYPE_PRECOMPILED
**
**      gctPOINTER Buffer
**          Pointer to a binary buffer containing the shader data to load.
**
**      gctSIZE_T BufferSize
**          Number of bytes inside the binary buffer pointed to by 'Buffer'.
**
**  OUTPUT:
**      nothing
**
*/
gceSTATUS
gcSHADER_LoadHeader(
    IN gcSHADER     Shader,
    IN gctPOINTER   Buffer,
    IN gctSIZE_T    BufferSize,
    OUT gctUINT32 * ShaderVersion
    )
{
    gceOBJECT_TYPE * signature;
    gctSIZE_T bytes;
    gctUINT32 *version;
    gctUINT32 *size;
    gctUINT8 *bytePtr;

    gcmHEADER_ARG("Shader=0x%x Buffer=0x%x BufferSize=%lu",
                  Shader, Buffer, BufferSize);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Buffer != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(BufferSize > 0);

    /* Load the Header */
    bytes = BufferSize;
    if(bytes < _gcdShaderBinaryHeaderSize) {
        /* Invalid file format */
        gcmFATAL("gcSHADER_LoadHeader: Invalid shader binary file format");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Verify the signature. */
    signature = (gceOBJECT_TYPE *) Buffer;
    if (*signature != gcvOBJ_SHADER) {
        /* Signature mismatch. */
        gcmFATAL("gcSHADER_LoadHeader: Signature does not match with 'SHDR'");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Word 2: binary file version # */
    version = (gctUINT32 *) (signature + 1);
    *ShaderVersion = *version;
    /* we may need to check if the major version is changed, we should be able to
     * break backward compatibility between major versions */
    if(*version > gcdSL_SHADER_BINARY_FILE_VERSION) {
#if gcdDEBUG
        /* for now, we don't support shader file format forward compatibility */
        gctUINT8 *inputVerPtr, *curVerPtr;
        gctUINT32 curVer[1] = {gcdSL_SHADER_BINARY_FILE_VERSION};

        inputVerPtr = (gctUINT8 *) version;
        curVerPtr = (gctUINT8 *) curVer;
        gcmFATAL("gcSHADER_LoadHeader: shader binary file's version of %u.%u.%u:%u "
                 "is not compatible with current version %u.%u.%u:%u\n"
                 "Please recompile source",
                 inputVerPtr[0], inputVerPtr[1], inputVerPtr[2], inputVerPtr[3],
                 curVerPtr[0], curVerPtr[1], curVerPtr[2], curVerPtr[3]);
#endif
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }


    /* Word 3: language type and shader type */
    version = (gctUINT32 *) (version + 1);
    bytePtr = (gctUINT8 *) version;
    if (Shader->type == gcSHADER_TYPE_PRECOMPILED) {
        Shader->type = *version >> 16;
    }
    else {
        if((gctUINT32)Shader->type != (*version >> 16)) {
           gcmFATAL("gcSHADER_LoadHeader: expected \"%s\" shader type does not exist in binary",
                    Shader->type == gcSHADER_TYPE_VERTEX ? "vertex" :
                    (Shader->type == gcSHADER_TYPE_FRAGMENT ? "fragment" : "unknown"));
           gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
           return gcvSTATUS_INVALID_DATA;
        }
    }
    switch(Shader->type) {
    case gcSHADER_TYPE_VERTEX:
    case gcSHADER_TYPE_FRAGMENT:
        if(bytePtr[0] != 'E' || bytePtr[1] != 'S') {
            gcmFATAL("gcSHADER_LoadHeader: Invalid compiler type \"%c%c\"", bytePtr[0], bytePtr[1]);
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
            return gcvSTATUS_INVALID_DATA;
        }
        break;

    case gcSHADER_TYPE_CL:
        if(bytePtr[0] != 'C' || bytePtr[1] != 'L') {
            gcmFATAL("gcSHADER_LoadHeader: Invalid compiler type \"%c%c\"", bytePtr[0], bytePtr[1]);
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
            return gcvSTATUS_INVALID_DATA;
        }
        break;

    default:
        gcmFATAL("gcSHADER_LoadHeader: Invalid shader type %d", Shader->type);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }
    Shader->compilerVersion[0] = *version;

    /* Word 4: compiler version */
    version = (gctUINT32 *) (version + 1);
    if(*version < Shader->compilerVersion[1]) {
#if gcdDEBUG
        gctUINT8 *inputVerPtr, *curVerPtr;

        inputVerPtr = (gctUINT8 *) version;
        curVerPtr = (gctUINT8 *) Shader->compilerVersion[1];
        gcmFATAL("gcSHADER_LoadHeader: shader binary file's compiler version of %u.%u.%u:%u "
                 "is older than current version %u.%u.%u:%u\n"
                 "Please recompile source",
                 inputVerPtr[0], inputVerPtr[1], inputVerPtr[2], inputVerPtr[3],
                 curVerPtr[0], curVerPtr[1], curVerPtr[2], curVerPtr[3]);
#endif
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }
    Shader->compilerVersion[1] = *version;

    /* Word 5: gcSL version */
    version = (gctUINT32 *) (version + 1);
    if(*version > gcdSL_IR_VERSION) {
#if gcdDEBUG
        gctUINT8 *inputVerPtr, *curVerPtr;
        gctUINT32 curVer[1] = {gcdSL_IR_VERSION};

        inputVerPtr = (gctUINT8 *) version;
        curVerPtr = (gctUINT8 *) curVer;
        gcmFATAL("gcSHADER_LoadHeader: shader binary file's gcSL version of %u.%u.%u:%u "
                 "is newer than current version %u.%u.%u:%u",
                 inputVerPtr[0], inputVerPtr[1], inputVerPtr[2], inputVerPtr[3],
                 curVerPtr[0], curVerPtr[1], curVerPtr[2], curVerPtr[3]);
#endif
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Word 6: size of binary excluding header */
    bytes -= _gcdShaderBinaryHeaderSize;
    size = (gctUINT32 *) (version + 1);
    if(bytes != *size) {
        /* binary size mismatch. */
        gcmFATAL("gcSHADER_LoadHeader: shader binary size %u does not match actual file size %u",
                 bytes, *size);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }
    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_LoadKernel
**
**  Load a kernel function given by name into gcSHADER object
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSTRING KernelName
**          Pointer to a kernel function name
**
**  OUTPUT:
**      nothing
**
*/
gceSTATUS
gcSHADER_LoadKernel(
    IN gcSHADER Shader,
    IN gctSTRING KernelName
    )
{
    gctSIZE_T i, j;
    gctUINT32 maxKernelFunctionArgs;
    gcKERNEL_FUNCTION kernelFunction = gcvNULL;
    gctUINT begin, end;

    gcmHEADER_ARG("Shader=0x%x KernelName=%s", Shader, KernelName);

    if (Shader->kernelFunctionCount > 0) {

        for (i = 0; i < Shader->kernelFunctionCount; i++) {
            if (Shader->kernelFunctions[i] == gcvNULL) continue;

            if (gcmIS_SUCCESS(gcoOS_StrCmp(Shader->kernelFunctions[i]->name, KernelName)))
            {
                kernelFunction = Shader->kernelFunctions[i];
                break;
            }
        }

        if (kernelFunction == gcvNULL) {
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
            return gcvSTATUS_INVALID_DATA;
        }

        Shader->currentKernelFunction = kernelFunction;
        Shader->localMemorySize = kernelFunction->localMemorySize;
        maxKernelFunctionArgs = Shader->maxKernelFunctionArgs;

        if (maxKernelFunctionArgs > 0) {

            /* Check array count */
            if (Shader->uniformArrayCount <= Shader->uniformCount + maxKernelFunctionArgs) {
                /* Reallocate a new array of object pointers. */
                gcmVERIFY_OK(gcSHADER_ReallocateUniforms(Shader, Shader->uniformCount + maxKernelFunctionArgs));
            }

            /* Shift shader uniforms down */
            for (i = Shader->uniformCount; i > 0; i--) {
                Shader->uniforms[i+maxKernelFunctionArgs-1] = Shader->uniforms[i-1];
            }

            Shader->uniformCount += maxKernelFunctionArgs;

            /* Load kernel uniforms arguments into shader uniforms */
            for (i = 0; i < kernelFunction->uniformArgumentCount; i++) {
                Shader->uniforms[i] = kernelFunction->uniformArguments[i];
            }

            /* Clean up unused uniforms */
            if (kernelFunction->uniformArgumentCount < maxKernelFunctionArgs) {
                for (i = kernelFunction->uniformArgumentCount; i < maxKernelFunctionArgs; i++) {
                    Shader->uniforms[i] = gcvNULL;
					Shader->uniformCount--;
                }
            }
        }

        /* Remove instructions for loading uniforms of other kernel functions */
        for (i = 0; i < Shader->kernelFunctionCount; i++) {

            kernelFunction = Shader->kernelFunctions[i];

            if (kernelFunction == gcvNULL) continue;
            if (kernelFunction == Shader->currentKernelFunction) continue;

            begin = kernelFunction->codeEnd;
            end   = kernelFunction->codeStart + kernelFunction->codeCount;

            for (j=begin; j<end; j++) {
                gcmVERIFY_OK(gcoOS_ZeroMemory(&Shader->code[j], sizeof(struct _gcSL_INSTRUCTION)));
            }
        }
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_Load
**
**  Load a gcSHADER object from a binary buffer.  The binary buffer is layed out
**  as follows:
**      // Six word header
**      // Signature, must be 'S','H','D','R'.
**      gctINT8             signature[4];
**      gctUINT32           binFileVersion;
**      gctUINT32           compilerVersion[2];
**      gctUINT32           gcSLVersion;
**      gctUINT32           binarySize;
**
**
**      // Number of attributes.  Can be 0, in which case the next field will be
**      // uniformCount.
**      gctINT16                attributeCount;
**
**      // attributeCount structures.  Each structure is followed by the
**      // attribute name, without the trailing '\0'.  Each structure is
**      // optionally padded with 0x00 to align to a 16-bit boundary.
**      struct _GCSL_BINARY_ATTRIBUTE {
**          gctINT8         componentCount;
**          gctINT8         isTexture;
**          gctINT16            nameLength;
**          char            name[];
**      } attributes[];
**
**      // Number of uniforms.  Can be 0, in which case the next field will be
**      outputCount.
**      gctINT16                uniformCount
**
**      // uniformCount structures.  Each structure is followed by the uniform
**      // name, without the trailing '\0'.  Each structure is optionally padded
**      // with 0x00 to align to a 16-bit boundary.
**      struct _GCSL_BINARY_UNIFORM {
**          gctUINT16           index;
**          gctUINT8            type;
**          gctINT16            count;
**          gctINT16            nameLength;
**          char            name[];
**      } uniforms[];
**
**      // Number of outputs.  Must be at least 1.
**      gctINT16                outputCount
**
**      // outputCount structures.  Each structure is followed by the output
**      // name, without the trailing '\0'.  Each structure is optionally padded
**      // with 0x00 to align to a 16-bit boundary.
**      struct _GCSL_BINARY_OUTPUT {
**          gctINT8         componentCount;
**          gctINT8         filler;     // align next field to 16-bit boundary
**          gctUINT16           tempIndex;
**          gctINT16            nameLength;
**          char            name[];
**      } outputs[];
**
**      // Number of instructions in code.
**      gctINT16                codeCount;
**
**      // The code buffer.
**      glSL_INSTRUCTION    code[];
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctPOINTER Buffer
**          Pointer to a binary buffer containing the shader data to load.
**
**      gctSIZE_T BufferSize
**          Number of bytes inside the binary buffer pointed to by 'Buffer'.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_Load(
    IN gcSHADER Shader,
    IN gctPOINTER Buffer,
    IN gctSIZE_T BufferSize
    )
{
    gctSIZE_T           bytes;
    gctUINT16           *count;
    gceSTATUS           status;
    gcBINARY_ATTRIBUTE  binaryAttribute;
    gctUINT             i;
    gctUINT             j;
    gctSIZE_T           length, binarySize;
    gcATTRIBUTE         attribute;
    gcBINARY_UNIFORM    binaryUniform;
    gcUNIFORM           uniform;
    gcBINARY_OUTPUT     binaryOutput;
    gcOUTPUT            output;
    gcBINARY_VARIABLE   binaryVariable;
    gcVARIABLE          variable;
    gcBINARY_FUNCTION   binaryFunction;
    gcFUNCTION          function;
    gcBINARY_ARGUMENT   binaryArgument;
    gcsFUNCTION_ARGUMENT_PTR argument;
    gcSL_INSTRUCTION    code;
#if GC_ENABLE_LOADTIME_OPT
    gctUINT8 *          curPos;
#endif /* GC_ENABLE_LOADTIME_OPT */
    gctPOINTER          pointer = gcvNULL;
    gctUINT32           shaderVersion;

    gcmHEADER_ARG("Shader=0x%x Buffer=0x%x BufferSize=%lu",
                  Shader, Buffer, BufferSize);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Buffer != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(BufferSize > 0);

    if(!Shader) {
       gcmFATAL("gcSHADER_Load: null shader handle passed");
       gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
       return gcvSTATUS_INVALID_ARGUMENT;
    }

    _gcSHADER_Clean(Shader);

    /* Load the Header */
    status = gcSHADER_LoadHeader(Shader, Buffer, BufferSize, &shaderVersion);
    if(gcmIS_ERROR(status)) {
       gcmFOOTER();
       return status;
    }

    bytes = BufferSize - _gcdShaderBinaryHeaderSize;

    /************************************************************************/
    /*                                  attributes                          */
    /************************************************************************/

    /* Get the attribute count. */
    count  = (gctUINT16 *) ((gctUINT8 *)Buffer + _gcdShaderBinaryHeaderSize);

    if (bytes < sizeof(gctUINT16))
    {
        /* Invalid attribute count. */
        gcmFATAL("gcSHADER_Load: Invalid attributeCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Save the attribute count. */
    Shader->attributeCount = *count;

    /* Point to the first attribute. */
    binaryAttribute = (gcBINARY_ATTRIBUTE) (count + 1);
    bytes          -= sizeof(gctUINT16);

    if (Shader->attributeCount > 0)
    {
        /* Allocate the array of gcATTRIBUTE structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->attributeCount * sizeof(gcATTRIBUTE),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->attributeCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
            gcmFOOTER();
            return status;
        }

        Shader->attributes = pointer;

        /* Parse all attributes. */
        for (i = 0; i < Shader->attributeCount; i++)
        {
            /* Get the length of the attribute name. */
            length = (bytes < sizeof(struct _gcBINARY_ATTRIBUTE))
                         ? 0
                         : binaryAttribute->nameLength;

            /* Test for special length. */
            if ((gctINT) length < 0)
            {
                length = 0;
            }

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_ATTRIBUTE, name) + length,
                                  2);

            if (bytes < binarySize)
            {
                /* Roll back. */
                Shader->attributeCount = i;

                /* Invalid attribute. */
                gcmFATAL("gcSHADER_Load: Invalid attribute");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the attribute inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                    gcmOFFSETOF(_gcATTRIBUTE, name) + length + 1,
                                    &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->attributeCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
                gcmFOOTER();
                return status;
            }

            attribute = pointer;

            /* Copy attribute to the gcSHADER object. */
            Shader->attributes[i]     = attribute;
            attribute->object.type    = gcvOBJ_ATTRIBUTE;
            attribute->index          = (gctUINT16) i;
            attribute->type           = (gcSHADER_TYPE) binaryAttribute->type;
            attribute->isTexture      = binaryAttribute->isTexture;
            attribute->enabled        = gcvTRUE;
            attribute->arraySize      = binaryAttribute->arraySize;
            attribute->inputIndex     = -1;
            attribute->nameLength     = binaryAttribute->nameLength;
            attribute->name[length]   = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(attribute->name,
                                       binaryAttribute->name,
                                       length));
            }

            /* Point to next attribute. */
            binaryAttribute = (gcBINARY_ATTRIBUTE)
                              ((gctINT8 *) binaryAttribute + binarySize);
            bytes          -= binarySize;
        }
    }

    /************************************************************************/
    /*                              uniforms                                */
    /************************************************************************/

    /* Get the uniform count. */
    count = (gctUINT16 *) binaryAttribute;

    if (bytes < sizeof(gctUINT16))
    {
        /* Invalid uniform count. */
        gcmFATAL("gcSHADER_Load: Invalid uniformCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Save the uniform count. */
    Shader->uniformCount = *count;

    /* Point to the first uniform. */
    binaryUniform = (gcBINARY_UNIFORM) (count + 1);
    bytes        -= sizeof(gctUINT16);

    if (Shader->uniformCount > 0)
    {
        gctINT   previousVersionAdjustment = 0;

        if (shaderVersion == gcdSL_SHADER_BINARY_BEFORE_LTC_FILE_VERSION)
        {
#if GC_ENABLE_LOADTIME_OPT
            /* fields which are not in the version */
            previousVersionAdjustment = sizeof(binaryUniform->flags)
                                     + sizeof(binaryUniform->glUniformIndex);;
#endif /* GC_ENABLE_LOADTIME_OPT */
        }

        /* Allocate the array of gcUNIFORM structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->uniformCount * sizeof(gcUNIFORM),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->uniformCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
            gcmFOOTER();
            return status;
        }

        Shader->uniforms = pointer;

        /* Parse all uniforms. */
        for (i = 0; i < Shader->uniformCount; i++)
        {
            /* Get the length of the uniform name. */
            length = (bytes < sizeof(struct _gcBINARY_UNIFORM))
                         ? 0
                         : binaryUniform->nameLength;

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_UNIFORM, name) + length,
                                 2);

            if (bytes < (binarySize - previousVersionAdjustment))
            {
                /* Roll back. */
                Shader->uniformCount = i;

                /* Invalid uniform. */
                gcmFATAL("gcSHADER_Load: Invalid uniform");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the uniform inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   gcmOFFSETOF(_gcUNIFORM, name) + length + 1,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->uniformCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
                gcmFOOTER();
                return status;
            }

            uniform = pointer;

            /* Copy uniform to the gcSHADER object. */
            Shader->uniforms[i]       = uniform;
            uniform->object.type      = gcvOBJ_UNIFORM;
            uniform->index            = (gctUINT16) i;
            uniform->type             = (gcSHADER_TYPE) binaryUniform->type;
            if (shaderVersion == gcdSL_SHADER_BINARY_BEFORE_LTC_FILE_VERSION)
            {
#if GC_ENABLE_LOADTIME_OPT
                uniform->flags            = 0;
                uniform->glUniformIndex   = (gctUINT16)(gctINT16) -1;
#endif /* GC_ENABLE_LOADTIME_OPT */
            }
            else
            {
#if GC_ENABLE_LOADTIME_OPT
                uniform->flags            = (gceUNIFORM_FLAGS)binaryUniform->flags;
                uniform->glUniformIndex   = binaryUniform->glUniformIndex;
#endif /* GC_ENABLE_LOADTIME_OPT */
            }
            uniform->format       = 0;
            uniform->isPointer    = 0;
            uniform->arraySize    = binaryUniform->arraySize;
            uniform->nameLength   = length;

            /* Copy name. */
            uniform->name[length] = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(uniform->name,
                                       binaryUniform->name - previousVersionAdjustment,
                                       length));
            }

            uniform->physical     = -1;
            uniform->address      = ~0U;

            switch (uniform->type)
            {
            case gcSHADER_SAMPLER_1D:
            case gcSHADER_SAMPLER_2D:
            case gcSHADER_SAMPLER_3D:
            case gcSHADER_SAMPLER_CUBIC:

            case gcSHADER_ISAMPLER_2D:
            case gcSHADER_ISAMPLER_3D:
            case gcSHADER_ISAMPLER_CUBIC:
            case gcSHADER_USAMPLER_2D:
            case gcSHADER_USAMPLER_3D:
            case gcSHADER_USAMPLER_CUBIC:
            case gcSHADER_SAMPLER_EXTERNAL_OES:
               uniform->physical = Shader->samplerIndex++;
               break;

            default:
               break;
            }

            /* substract fields which are not in the file version */
            binarySize -= previousVersionAdjustment;

            /* Point to next uniform. */
            binaryUniform = (gcBINARY_UNIFORM)
                            ((gctUINT8 *) binaryUniform + binarySize);
            bytes        -= binarySize;
        }
    }

    /************************************************************************/
    /*                                  outputs                             */
    /************************************************************************/

    /* Get the output count. */
    count = (gctUINT16 *) binaryUniform;

    if ( (bytes < sizeof(gctUINT16)) || (*count <= 0))
    {
        /* Invalid output count. */
        gcmFATAL("gcSHADER_Load: Invalid outputCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the first output. */
    binaryOutput = (gcBINARY_OUTPUT) (count + 1);
    bytes       -= sizeof(gctUINT16);

    /* Save the output count. */
    Shader->outputCount = *count;

    /* Allocate the array of gcOUTPUT structure pointers. */
    status = gcoOS_Allocate(gcvNULL,
                           Shader->outputCount * sizeof(gcOUTPUT),
                           &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Roll back. */
        Shader->outputCount = 0;

        /* Error. */
        gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
        gcmFOOTER();
        return status;
    }

    Shader->outputs = pointer;

    /* Parse all outputs. */
    for (i = 0; i < Shader->outputCount; i++)
    {
        /* Get the length of the output name. */
        length = (bytes < sizeof(struct _gcBINARY_OUTPUT))
                     ? 0
                     : binaryOutput->nameLength;

        /* Test for special length. */
        if ((gctINT) length < 0)
        {
            length = 0;
        }

        /* Compute the number of bytes required. */
        binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_OUTPUT, name) + length, 2);

        if (bytes < binarySize)
        {
            /* Roll back. */
            Shader->outputCount = i;

            /* Invalid output. */
            gcmFATAL("gcSHADER_Load: Invalid output");
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
            return gcvSTATUS_INVALID_DATA;
        }

        /* Allocate memory for the output inside the gcSHADER object. */
        status = gcoOS_Allocate(gcvNULL,
                               gcmOFFSETOF(_gcOUTPUT, name) + length + 1,
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->outputCount = i;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
            gcmFOOTER();
            return status;
        }

        output = pointer;

        /* Copy output to shader. */
        Shader->outputs[i]   = output;
        output->object.type  = gcvOBJ_OUTPUT;
        output->type         = (gcSHADER_TYPE) binaryOutput->type;
        output->arraySize    = binaryOutput->arraySize;
        output->tempIndex    = binaryOutput->tempIndex;
        output->physical     = gcvFALSE;
        output->nameLength   = binaryOutput->nameLength;
        output->name[length] = '\0';

        if (length > 0)
        {
            gcmVERIFY_OK(gcoOS_MemCopy(output->name, binaryOutput->name, length));
        }

        /* Point to next output. */
        binaryOutput = (gcBINARY_OUTPUT)
                       ((gctUINT8 *) binaryOutput + binarySize);
        bytes       -= binarySize;
    }

    /* Get the variable count. */
    count = (gctUINT16 *) binaryOutput;

    if (bytes < sizeof(gctUINT16))
    {
        /* Invalid variable count. */
        gcmFATAL("gcSHADER_Load: Invalid variableCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the first variable. */
    binaryVariable = (gcBINARY_VARIABLE) (count + 1);
    bytes       -= sizeof(gctUINT16);

    /* Save the variable count. */
    Shader->variableCount = *count;

    if (*count > 0)
    {
        gctINT   previousVersionAdjustment = 0;
        if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
        {
            /* fields which are not in the version */
            previousVersionAdjustment = sizeof(binaryVariable->varCategory) +
                                        sizeof(binaryVariable->firstChild) +
                                        sizeof(binaryVariable->nextSibling) +
                                        sizeof(binaryVariable->prevSibling) +
                                        sizeof(binaryVariable->parent);
        }

        /* Allocate the array of gcVARIABLE structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->variableCount * sizeof(gcVARIABLE),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->variableCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->variables = pointer;

        /* Parse all variables. */
        for (i = 0; i < Shader->variableCount; i++)
        {
            /* Get the length of the variable name. */
            length = (bytes < sizeof(struct _gcBINARY_VARIABLE))
                         ? 0
                         : binaryVariable->nameLength;

            /* Test for special length. */
            if ((gctINT) length < 0)
            {
                length = 0;
            }

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_VARIABLE, name) + length, 2);

            if (bytes < binarySize - previousVersionAdjustment)
            {
                /* Roll back. */
                Shader->variableCount = i;

                /* Invalid variable. */
                gcmFATAL("gcSHADER_Load: Invalid variable");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the variable inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   gcmOFFSETOF(_gcVARIABLE, name) + length + 1,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->variableCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                gcmFOOTER();
                return status;
            }

            variable = pointer;

            /* Copy variable to shader. */
            Shader->variables[i]   = variable;
            variable->object.type  = gcvOBJ_VARIABLE;

            if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
            {
                variable->varCategory = gcSHADER_VAR_CATEGORY_NORMAL;
                variable->firstChild = -1;
                variable->nextSibling = -1;
                variable->prevSibling = -1;
                variable->parent = -1;
                variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
            }
            else
            {
            variable->varCategory = binaryVariable->varCategory;
            variable->firstChild = binaryVariable->firstChild;
            variable->nextSibling = binaryVariable->nextSibling;
            variable->prevSibling = binaryVariable->prevSibling;
            variable->parent = binaryVariable->parent;

            if (binaryVariable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
            else
                variable->u.numStructureElement = binaryVariable->u.numStructureElement;
            }

            variable->arraySize    = binaryVariable->arraySize;
            variable->tempIndex    = binaryVariable->tempIndex;
            variable->nameLength   = binaryVariable->nameLength;
            variable->name[length] = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(variable->name, binaryVariable->name - previousVersionAdjustment, length));
            }

            /* substract fields which are not in the file version */
            binarySize -= previousVersionAdjustment;

            /* Point to next variable. */
            binaryVariable = (gcBINARY_VARIABLE)
                           ((gctUINT8 *) binaryVariable + binarySize);
            bytes       -= binarySize;
        }
    }

    /************************************************************************/
    /*                              functions                               */
    /************************************************************************/

    /* Get the function count. */
    count = (gctUINT16 *) binaryVariable;

    if (bytes < sizeof(gctUINT16))
    {
        /* Invalid function count. */
        gcmFATAL("gcSHADER_Load: Invalid functionCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the first function. */
    binaryFunction = (gcBINARY_FUNCTION) (count + 1);
    bytes       -= sizeof(gctUINT16);

    /* Save the function count. */
    Shader->functionCount = *count;

    if (*count > 0)
    {
        /* Allocate the array of gcFUNCTION structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                                Shader->functionCount * sizeof(gcFUNCTION),
                                &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->functionCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->functions = pointer;

        /* Parse all functions. */
        for (i = 0; i < Shader->functionCount; i++)
        {
            /* Get the length of the function name. */
            length = (bytes < sizeof(struct _gcBINARY_FUNCTION))
                         ? 0
                         : binaryFunction->nameLength;

            /* Test for special length. */
            if ((gctINT) length < 0)
            {
                length = 0;
            }

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_FUNCTION, name) + length, 2);

            if (bytes < binarySize)
            {
                /* Roll back. */
                Shader->functionCount = i;

                /* Invalid function. */
                gcmFATAL("gcSHADER_Load: Invalid function");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the function inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   gcmOFFSETOF(_gcsFUNCTION, name) + length + 1,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->functionCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                gcmFOOTER();
                return status;
            }

            function = pointer;

            /* Copy function to shader. */
            Shader->functions[i]   = function;
            function->object.type  = gcvOBJ_FUNCTION;

            function->argumentCount = binaryFunction->argumentCount;
            function->arguments     = gcvNULL;
            function->variableCount = binaryFunction->variableCount;
            function->variables     = gcvNULL;
            function->codeStart     = binaryFunction->codeStart;
            function->codeCount     = binaryFunction->codeCount;
            function->label         = binaryFunction->label;
            function->nameLength    = binaryFunction->nameLength;
            function->name[length]  = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(function->name, binaryFunction->name, length));
            }

            /* Point to first argument. */
            binaryArgument = (gcBINARY_ARGUMENT)
                           ((gctUINT8 *) binaryFunction + binarySize);
            bytes         -= binarySize;

            /* Function arguments */
            if (function->argumentCount > 0)
            {
                binarySize = function->argumentCount * sizeof(gcsFUNCTION_ARGUMENT);

                if (bytes < binarySize)
                {
                    /* Roll back. */
                    Shader->functionCount = i;
                    function->argumentCount = 0;

                    /* Invalid argument. */
                    gcmFATAL("gcSHADER_Load: Invalid argument");
                    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                    return gcvSTATUS_INVALID_DATA;
                }

                /* Allocate the array of gcsFUNCTION_ARGUMENT structures. */
                status = gcoOS_Allocate(gcvNULL, binarySize, &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->functionCount = i;
                    function->argumentCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                function->arguments = pointer;

                for (j=0; j<function->argumentCount; j++)
                {
                    argument = &function->arguments[j];

                    argument->index     = binaryArgument->index;
                    argument->enable    = binaryArgument->enable;
                    argument->qualifier = binaryArgument->qualifier;

                    /* Point to next argument. */
                    binaryArgument++;
                    bytes -= sizeof(gcsFUNCTION_ARGUMENT);
                }
            }

            /* Point to first variable. */
            binaryVariable = (gcBINARY_VARIABLE) binaryArgument;

            /* Function variables */
            if (function->variableCount > 0)
            {
                gctINT   previousVersionAdjustment = 0;
                if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
                {
                    /* fields which are not in the version */
                    previousVersionAdjustment = sizeof(binaryVariable->varCategory) +
                                                sizeof(binaryVariable->firstChild) +
                                                sizeof(binaryVariable->nextSibling) +
                                                sizeof(binaryVariable->prevSibling) +
                                                sizeof(binaryVariable->parent);
                }

                /* Allocate the array of gcVARIABLE structure pointers. */
                status = gcoOS_Allocate(gcvNULL,
                                       function->variableCount * sizeof(gcVARIABLE),
                                       &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->functionCount = i;
                    function->variableCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                function->variables = pointer;

                /* Parse all variables. */
                for (j = 0; j < function->variableCount; j++)
                {
                    /* Get the length of the variable name. */
                    length = (bytes < sizeof(struct _gcBINARY_VARIABLE))
                                 ? 0
                                 : binaryVariable->nameLength;

                    /* Test for special length. */
                    if ((gctINT) length < 0)
                    {
                        length = 0;
                    }

                    /* Compute the number of bytes required. */
                    binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_VARIABLE, name) + length, 2);

                    if (bytes < binarySize - previousVersionAdjustment)
                    {
                        /* Roll back. */
                        Shader->functionCount = i;
                        function->variableCount = j;

                        /* Invalid variable. */
                        gcmFATAL("gcSHADER_Load: Invalid variable");
                        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                        return gcvSTATUS_INVALID_DATA;
                    }

                    /* Allocate memory for the variable inside the gcSHADER object. */
                    status = gcoOS_Allocate(gcvNULL,
                                           gcmOFFSETOF(_gcVARIABLE, name) + length + 1,
                                           &pointer);

                    if (gcmIS_ERROR(status))
                    {
                        /* Roll back. */
                        Shader->functionCount = i;
                        function->variableCount = j;

                        /* Error. */
                        gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                        gcmFOOTER();
                        return status;
                    }

                    variable = pointer;

                    /* Copy variable to shader. */
                    function->variables[j]   = variable;
                    variable->object.type  = gcvOBJ_VARIABLE;

                    if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
                    {
                        variable->varCategory = gcSHADER_VAR_CATEGORY_NORMAL;
                        variable->firstChild = -1;
                        variable->nextSibling = -1;
                        variable->prevSibling = -1;
                        variable->parent = -1;
                        variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
                    }
                    else
                    {
                    variable->varCategory = binaryVariable->varCategory;
                    variable->firstChild = binaryVariable->firstChild;
                    variable->nextSibling = binaryVariable->nextSibling;
                    variable->prevSibling = binaryVariable->prevSibling;
                    variable->parent = binaryVariable->parent;

                    if (binaryVariable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                        variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
                    else
                        variable->u.numStructureElement = binaryVariable->u.numStructureElement;
                    }

                    variable->arraySize    = binaryVariable->arraySize;
                    variable->tempIndex    = binaryVariable->tempIndex;
                    variable->nameLength   = binaryVariable->nameLength;
                    variable->name[length] = '\0';

                    if (length > 0)
                    {
                        gcmVERIFY_OK(gcoOS_MemCopy(variable->name, binaryVariable->name - previousVersionAdjustment, length));
                    }

                    /* substract fields which are not in the file version */
                    binarySize -= previousVersionAdjustment;

                    /* Point to next variable. */
                    binaryVariable = (gcBINARY_VARIABLE)
                                   ((gctUINT8 *) binaryVariable + binarySize);
                    bytes       -= binarySize;
                }
            }

            /* Point to next function. */
            binaryFunction = (gcBINARY_FUNCTION) binaryVariable;
        }
    }

    /************************************************************************/
    /*                                  code                                */
    /************************************************************************/

    /* Get the code count. */
    count = (gctUINT16 *)binaryFunction;

    if ( (bytes < sizeof(gctUINT16)) || (*count <= 0))
    {
        /* Invalid code count. */
        gcmFATAL("gcSHADER_Load: Invalid codeCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the code. */
    code   = (gcSL_INSTRUCTION) (count + 1);
    bytes -= sizeof(gctUINT16);

    /* Save the code count. */
    Shader->lastInstruction = Shader->codeCount = *count;

    /* Compute the number of bytes. */
    binarySize = Shader->codeCount * sizeof(struct _gcSL_INSTRUCTION);

    if (bytes < binarySize)
    {
        /* Roll back. */
        Shader->lastInstruction = Shader->codeCount = 0;

        /* Invalid code count. */
        gcmFATAL("gcSHADER_Load: Invalid codeCount");
        gcmFOOTER();
        return gcvSTATUS_INVALID_DATA;
    }

    /* Allocate memory for the code inside the gcSHADER object. */
    status = gcoOS_Allocate(gcvNULL,
                           binarySize,
                           &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Roll back. */
        Shader->lastInstruction = Shader->codeCount = 0;

        /* Error. */
        gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
        gcmFOOTER();
        return status;
    }

    Shader->code = pointer;

    /* Copy the code into the gcSHADER object. */
    gcmVERIFY_OK(gcoOS_MemCopy(Shader->code, code, binarySize));


    if (shaderVersion > gcdSL_SHADER_BINARY_BEFORE_LTC_FILE_VERSION)
    {
#if GC_ENABLE_LOADTIME_OPT
        /* Loadtime Optimization related data */

        /* LTC uniform index */
        curPos = (gctUINT8 *)code + binarySize;
        bytes -= binarySize;  /* remaining bytes */
        Shader->ltcUniformCount = *(gctINT *)curPos;

        curPos += sizeof(Shader->ltcUniformCount);
        bytes -= sizeof(Shader->ltcUniformCount);  /* remaining bytes */
        Shader->ltcUniformBegin = *(gctUINT *) curPos;

        curPos += sizeof(Shader->ltcUniformBegin);
        bytes -= sizeof(Shader->ltcUniformBegin);
        Shader->ltcInstructionCount = *(gctUINT *) curPos;

        curPos += sizeof(Shader->ltcInstructionCount); /* points to ltcCodeUniformIndex */
        bytes -= sizeof(Shader->ltcInstructionCount);  /* remaining bytes */
        binarySize = sizeof(*Shader->ltcCodeUniformIndex) * Shader->ltcInstructionCount;

        if (binarySize > 0)
        {
            /* Allocate memory for  ltcCodeUniformIndex inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   binarySize,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->ltcUniformCount = 0;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
                gcmFOOTER();
                return status;
            }

            gcmVERIFY_OK(gcoOS_MemCopy(pointer, curPos, binarySize));
            Shader->ltcCodeUniformIndex = (gctINT *)pointer;
        }
        else
        {
            Shader->ltcCodeUniformIndex = gcvNULL;
        }

        curPos += binarySize;                           /* points to ltcExpressions */
        bytes -= binarySize;                            /* remaining bytes */
        binarySize = sizeof(*Shader->ltcExpressions) * Shader->ltcInstructionCount;
        if (binarySize > 0)
        {
            /* Allocate memory for  ltcExpressions inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   binarySize,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->ltcUniformCount = 0;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
                gcmFOOTER();
                return status;
            }

            gcmVERIFY_OK(gcoOS_MemCopy(pointer, curPos, binarySize));
            Shader->ltcExpressions = (gcSL_INSTRUCTION)pointer;
        }
        else
        {
            Shader->ltcExpressions = gcvNULL;
        }
#endif /* GC_ENABLE_LOADTIME_OPT */
    }
    else
    {
#if GC_ENABLE_LOADTIME_OPT
        Shader->ltcUniformCount = 0;
        Shader->ltcExpressions = gcvNULL;
        Shader->ltcUniformBegin = 0;
        Shader->ltcInstructionCount = 0;
        Shader->ltcCodeUniformIndex = gcvNULL;
#endif /* GC_ENABLE_LOADTIME_OPT */
    }/* if */

    if (bytes > binarySize)
    {
        /* Error. */
        gcmFATAL("gcSHADER_Load: %u extraneous bytes in the binary file.",
              bytes - binarySize);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_Save
**
**  Save a gcSHADER object including header information to a binary buffer.
**  SHADER binary header format: -
**     Word 1:  ('S' 'H' 'D' 'R') : signature
**     Word 2:  ('\od' '\od' '\od' '\od') od = octal digits; binary file version
**     Word 3:  ('E' 'S' '\0' '\0') | gcSHADER_TYPE_VERTEX   or
**              ('E' 'S' '\0' '\0') | gcSHADER_TYPE_FRAGMENT or
**              ('C' 'L' '\0' '\0') | gcSHADER_TYPE_CL
**     Word 4: ('\od' '\od' '\od' '\od') od = octal digits; compiler version
**     Word 5: ('\1' '\0' '\0' '\0') gcSL version
**     Word 6: size of shader binary file in bytes excluding this header
**
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctPOINTER Buffer
**          Pointer to a binary buffer to be used as storage for the gcSHADER
**          object.  If 'Buffer' is gcvNULL, the gcSHADER object will not be saved,
**          but the number of bytes required to hold the binary output for the
**          gcSHADER object will be returned.
**
**      gctSIZE_T * BufferSize
**          Pointer to a variable holding the number of bytes allocated in
**          'Buffer'.  Only valid if 'Buffer' is not gcvNULL.
**
**  OUTPUT:
**
**      gctSIZE_T * BufferSize
**          Pointer to a variable receiving the number of bytes required to hold
**          the binary form of the gcSHADER object.
*/
gceSTATUS
gcSHADER_Save(
    IN gcSHADER Shader,
    IN gctPOINTER Buffer,
    IN OUT gctSIZE_T * BufferSize
    )
{
    gctSIZE_T   bytes, i, j, nameLength;
    gctUINT8    *buffer;
    gctSIZE_T   outputCount     = 0;
    gctSIZE_T   attributeCount  = 0;
    gctSIZE_T   uniformCount    = 0;
    gctSIZE_T   variableCount   = 0;
    gctSIZE_T   functionCount   = 0;

    gcmHEADER_ARG("Shader=0x%x Buffer=0x%x BufferSize=0x%x", Shader, Buffer, BufferSize);

    if (Shader == gcvNULL)
    {
    /* Return required number of bytes. */
        *BufferSize = 0;
        gcmFOOTER_ARG("*BufferSize=%d", *BufferSize);
        return gcvSTATUS_OK;
    }

    /************************************************************************/
    /*                  Compute size of binary buffer.                      */
    /************************************************************************/

    /* File Header. */
    bytes = _gcdShaderBinaryHeaderSize;

    /* Attributes. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->attributeCount; i++)
    {
        if (Shader->attributes[i] == gcvNULL) continue;

        attributeCount ++;

        nameLength = Shader->attributes[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_ATTRIBUTE, name) + gcmALIGN(nameLength, 2);
    }

    /* Uniforms. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->uniformCount; i++)
    {
        if (Shader->uniforms[i] == gcvNULL) continue;

        uniformCount ++;

        nameLength = Shader->uniforms[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_UNIFORM, name) + gcmALIGN(nameLength, 2);
    }

    /* Outputs. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->outputCount; i++)
    {
        if (Shader->outputs[i] == gcvNULL) continue;

        outputCount ++;

        nameLength = Shader->outputs[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_OUTPUT, name) + gcmALIGN(nameLength, 2);
    }

    /* Global Variables. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->variableCount; i++)
    {
        if (Shader->variables[i] == gcvNULL) continue;

        variableCount ++;

        nameLength = Shader->variables[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + gcmALIGN(nameLength, 2);
    }

    /* Functions. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->functionCount; i++)
    {
        if (Shader->functions[i] == gcvNULL) continue;

        functionCount ++;

        nameLength = Shader->functions[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_FUNCTION, name) + gcmALIGN(nameLength, 2);

        /* Function arguments */
        bytes += Shader->functions[i]->argumentCount * sizeof(struct _gcBINARY_ARGUMENT);

        /* Function variables */
        for (j=0; j<Shader->functions[i]->variableCount; j++)
        {
            if (Shader->functions[i]->variables[j] == gcvNULL) continue;

            nameLength = Shader->functions[i]->variables[j]->nameLength;

            if ((long) nameLength < 0)
            {
                nameLength = 0;
            }

            bytes += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + gcmALIGN(nameLength, 2);
        }
    }

    /* Code. */
    bytes += sizeof(gctUINT16);
    bytes += Shader->codeCount * sizeof(struct _gcSL_INSTRUCTION);

#if GC_ENABLE_LOADTIME_OPT
    /* Loadtime Optimization related data */

    bytes += sizeof(Shader->ltcUniformCount) +
             sizeof(Shader->ltcUniformBegin) +
             sizeof(*Shader->ltcCodeUniformIndex) * Shader->ltcInstructionCount +
             sizeof(Shader->ltcInstructionCount) +
             sizeof(*Shader->ltcExpressions) * Shader->ltcInstructionCount;
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Return required number of bytes if Buffer is gcvNULL. */
    if (Buffer == gcvNULL)
    {
        *BufferSize = bytes;
        gcmFOOTER_ARG("*BufferSize=%d", *BufferSize);
        return gcvSTATUS_OK;
    }

    /* Make sure the buffer is large enough. */
    if (*BufferSize < bytes)
    {
        *BufferSize = bytes;
        gcmFOOTER_ARG("*BufferSize=%d status=%d",
                        *BufferSize, gcvSTATUS_BUFFER_TOO_SMALL);
        return gcvSTATUS_BUFFER_TOO_SMALL;
    }

    /* Store number of bytes returned. */
    *BufferSize = bytes;

    /************************************************************************/
    /*                      Fill the binary buffer.                         */
    /************************************************************************/

    buffer = Buffer;

    /* Header */
    /* Word 1: signature 'S' 'H' 'D' 'R' */
    *(gceOBJECT_TYPE *) buffer = gcvOBJ_SHADER;
    buffer += sizeof(gceOBJECT_TYPE);

    /* Word 2: binary file version # */
    *(gctUINT32 *) buffer = gcdSL_SHADER_BINARY_FILE_VERSION;
    buffer += sizeof(gctUINT32);

    /* Word 3: language type and shader type */
    *(gctUINT32 *) buffer = Shader->compilerVersion[0];
    buffer += sizeof(gctUINT32);

    /* Word 4: compiler version */
    *(gctUINT32 *) buffer = Shader->compilerVersion[1];
    buffer += sizeof(gctUINT32);

    /* Word 5: gcSL version */
    *(gctUINT32 *) buffer = gcdSL_IR_VERSION;
    buffer += sizeof(gctUINT32);

    /* Word 6: size of binary excluding header */
    *(gctUINT32 *) buffer = bytes - _gcdShaderBinaryHeaderSize;
    buffer += sizeof(gctUINT32);

    /* Attribute count. */
    *(gctUINT16 *) buffer = (gctUINT16) attributeCount;
    buffer += sizeof(gctUINT16);

    /* Attributes. */
    for (i = 0; i < Shader->attributeCount; i++)
    {
        gcATTRIBUTE attribute;
        gcBINARY_ATTRIBUTE binary;

        if (Shader->attributes[i] == gcvNULL) continue;

        /* Point to source and binary attributes. */
        attribute = Shader->attributes[i];
        binary    = (gcBINARY_ATTRIBUTE) buffer;

        /* Fill in binary attribute. */
        binary->type       = attribute->type;
        binary->isTexture  = (gctINT8)  attribute->isTexture;
        binary->arraySize  = (gctINT16) attribute->arraySize;
        binary->nameLength = (gctINT16) attribute->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, attribute->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_ATTRIBUTE, name) + bytes;
    }

    /* Uniform count. */
    *(gctUINT16 *) buffer = (gctUINT16) uniformCount;
    buffer += sizeof(gctUINT16);

    /* Uniforms. */
    for (i = 0; i < Shader->uniformCount; i++)
    {
        gcUNIFORM uniform;
        gcBINARY_UNIFORM binary;

        if (Shader->uniforms[i] == gcvNULL) continue;

        /* Point to source and binary uniforms. */
        uniform = Shader->uniforms[i];
        binary  = (gcBINARY_UNIFORM) buffer;

        /* Fill in binary uniform. */
        binary->type       = uniform->type;
        binary->arraySize  = (gctINT16) uniform->arraySize;
        binary->nameLength = (gctINT16) uniform->nameLength;
#if GC_ENABLE_LOADTIME_OPT
        binary->flags      = (gctINT16) uniform->flags;
        binary->glUniformIndex   = uniform->glUniformIndex;
#endif /* GC_ENABLE_LOADTIME_OPT */

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, uniform->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_UNIFORM, name) + bytes;
    }

    /* Output count. */
    *(gctUINT16 *) buffer = (gctUINT16) outputCount;
    buffer += sizeof(gctUINT16);

    /* Outputs. */
    for (i = 0; i < Shader->outputCount; i++)
    {
        gcOUTPUT output;
        gcBINARY_OUTPUT binary;

        if (Shader->outputs[i] == gcvNULL) continue;

        /* Point to source and binary outputs. */
        output = Shader->outputs[i];
        binary = (gcBINARY_OUTPUT) buffer;

        /* Fill in binary output. */
        binary->type       = output->type;
        binary->arraySize  = (gctINT8) output->arraySize;
        binary->tempIndex  = output->tempIndex;
        binary->nameLength = (gctINT16) output->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, output->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_OUTPUT, name) + bytes;
    }

    /* Global variable count. */
    *(gctUINT16 *) buffer = (gctUINT16) Shader->variableCount;
    buffer += sizeof(gctUINT16);

    /* Global variables. */
    for (i = 0; i < Shader->variableCount; i++)
    {
        gcVARIABLE variable;
        gcBINARY_VARIABLE binary;

        if (Shader->variables[i] == gcvNULL) continue;

        /* Point to source and binary variables. */
        variable = Shader->variables[i];
        binary = (gcBINARY_VARIABLE) buffer;

        /* Fill in binary variable. */
        binary->varCategory = variable->varCategory;
        binary->firstChild = variable->firstChild;
        binary->nextSibling = variable->nextSibling;
        binary->prevSibling = variable->prevSibling;
        binary->parent = variable->parent;

        if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
            binary->u.type = (gcSHADER_TYPE) variable->u.type;
        else
            binary->u.numStructureElement = (gctUINT8)variable->u.numStructureElement;

        binary->arraySize  = (gctINT8) variable->arraySize;
        binary->tempIndex  = variable->tempIndex;
        binary->nameLength = (gctINT16) variable->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, variable->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + bytes;
    }

    /* Function count. */
    *(gctUINT16 *) buffer = (gctUINT16) functionCount;
    buffer += sizeof(gctUINT16);

    /* Functions. */
    for (i = 0; i < Shader->functionCount; i++)
    {
        gcFUNCTION function;
        gcBINARY_FUNCTION binary;

        if (Shader->functions[i] == gcvNULL) continue;

        /* Point to source and binary functions. */
        function = Shader->functions[i];
        binary = (gcBINARY_FUNCTION) buffer;

        /* Fill in binary function. */
        binary->argumentCount       = (gctINT16) function->argumentCount;
        binary->variableCount       = (gctINT16) function->variableCount;
        binary->codeStart           = (gctUINT16) function->codeStart;
        binary->codeCount           = (gctUINT16) function->codeCount;
        binary->label               = function->label;
        binary->nameLength          = (gctINT16) function->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, function->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_FUNCTION, name) + bytes;

        /* Function arguments */
        for (j = 0; j < function->argumentCount; j++)
        {
            gcsFUNCTION_ARGUMENT *argument;
            gcBINARY_ARGUMENT binary;

            /* Point to source and binary arguments. */
            argument = &function->arguments[j];
            binary = (gcBINARY_ARGUMENT) buffer;

            /* Fill in binary argument. */
            binary->index      = argument->index;
            binary->enable     = argument->enable;
            binary->qualifier  = argument->qualifier;

            /* Adjust buffer pointer. */
            buffer += sizeof(struct _gcBINARY_ARGUMENT);
        }

        /* Function variables */
        for (j = 0; j < function->variableCount; j++)
        {
            gcVARIABLE variable;
            gcBINARY_VARIABLE binary;

            if (function->variables[j] == gcvNULL) continue;

            /* Point to source and binary variables. */
            variable = function->variables[j];
            binary = (gcBINARY_VARIABLE) buffer;

            /* Fill in binary variable. */
            binary->varCategory = variable->varCategory;
            binary->firstChild = variable->firstChild;
            binary->nextSibling = variable->nextSibling;
            binary->prevSibling = variable->prevSibling;
            binary->parent = variable->parent;

            if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                binary->u.type = (gcSHADER_TYPE) variable->u.type;
            else
                binary->u.numStructureElement = (gctUINT8)variable->u.numStructureElement;

            binary->arraySize  = (gctINT8) variable->arraySize;
            binary->tempIndex  = variable->tempIndex;
            binary->nameLength = (gctINT16) variable->nameLength;

            if (binary->nameLength > 0)
            {
                /* Compute number of bytes to copy. */
                bytes = gcmALIGN(binary->nameLength, 2);

                /* Copy name. */
                gcmVERIFY_OK(gcoOS_MemCopy(binary->name, variable->name, bytes));
            }
            else
            {
                bytes = 0;
            }

            /* Adjust buffer pointer. */
            buffer += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + bytes;
        }
    }

    /* Code count. */
    *(gctUINT16 *) buffer = (gctUINT16) Shader->codeCount;
    buffer += sizeof(gctUINT16);

    /* Copy the code. */
    bytes = Shader->codeCount * sizeof(struct _gcSL_INSTRUCTION);
    if (bytes > 0) gcmVERIFY_OK(gcoOS_MemCopy(buffer, Shader->code, bytes));
    buffer += bytes;

#if GC_ENABLE_LOADTIME_OPT
    /* Loadtime Optimization related data */

    /* LTC uniform index */
    *(gctINT *) buffer = Shader->ltcUniformCount;
    buffer += sizeof(Shader->ltcUniformCount);

    *(gctUINT *) buffer = Shader->ltcUniformBegin;
    buffer += sizeof(Shader->ltcUniformBegin);

    *(gctUINT *) buffer = Shader->ltcInstructionCount;
    buffer += sizeof(Shader->ltcInstructionCount);

    bytes = sizeof(*Shader->ltcCodeUniformIndex) * Shader->ltcInstructionCount;
    if (bytes > 0) gcmVERIFY_OK(gcoOS_MemCopy(buffer, Shader->ltcCodeUniformIndex, bytes));
    buffer += bytes;

    /* LTC expressions */
    bytes = sizeof(*Shader->ltcExpressions) * Shader->ltcInstructionCount;
    if (bytes > 0) gcmVERIFY_OK(gcoOS_MemCopy(buffer, Shader->ltcExpressions, bytes));
    buffer += bytes;
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Success. */
    gcmFOOTER_ARG("*BufferSize=%lu", *BufferSize);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_LoadEx
**
**  Load a gcSHADER object from a binary buffer.  The binary buffer is layed out
**  as follows:
**      // Six word header
**      // Signature, must be 'S','H','D','R'.
**      gctINT8             signature[4];
**      gctUINT32           binFileVersion;
**      gctUINT32           compilerVersion[2];
**      gctUINT32           gcSLVersion;
**      gctUINT32           binarySize;
**
**
**      // Number of attributes.  Can be 0, in which case the next field will be
**      // uniformCount.
**      gctINT16                attributeCount;
**
**      // attributeCount structures.  Each structure is followed by the
**      // attribute name, without the trailing '\0'.  Each structure is
**      // optionally padded with 0x00 to align to a 16-bit boundary.
**      struct _GCSL_BINARY_ATTRIBUTE {
**          gctINT8         componentCount;
**          gctINT8         isTexture;
**          gctINT16            nameLength;
**          char            name[];
**      } attributes[];
**
**      // Number of uniforms.  Can be 0, in which case the next field will be
**      outputCount.
**      gctINT16                uniformCount
**
**      // uniformCount structures.  Each structure is followed by the uniform
**      // name, without the trailing '\0'.  Each structure is optionally padded
**      // with 0x00 to align to a 16-bit boundary.
**      struct _GCSL_BINARY_UNIFORM {
**          gctUINT16           index;
**          gctUINT8            type;
**          gctINT16            count;
**          gctINT16            nameLength;
**          char            name[];
**      } uniforms[];
**
**      // Number of outputs.  Can be zero.
**      gctINT16                outputCount
**
**      // outputCount structures.  Each structure is followed by the output
**      // name, without the trailing '\0'.  Each structure is optionally padded
**      // with 0x00 to align to a 16-bit boundary.
**      struct _GCSL_BINARY_OUTPUT {
**          gctINT8         type;
**          gctINT8         arraySize;
**          gctUINT16       tempIndex;
**          gctINT16        nameLength;
**          char            name[];
**      } outputs[];
**
**      // Number of variables.  Can be zero.
**      gctINT16                variableCount
**
**      // variableCount structures.  Each structure is followed by the variable
**      // name, without the trailing '\0'.  Each structure is optionally padded
**      // with 0x00 to align to a 16-bit boundary.
**      struct _GCSL_BINARY_VARIABLE {
**          gctINT8         type;
**          gctINT8         arraySize;
**          gctUINT16       tempIndex;
**          gctINT16        nameLength;
**          char            name[];
**      } variables[];
**
**      // Number of functions.  Can be zero.
**      gctINT16                functionCount
**
**      // functionCount structures.  Each structure is followed by the function
**      // name, without the trailing '\0'.  Each structure is optionally padded
**      // with 0x00 to align to a 16-bit boundary.
**      // Next follows function arguments and variables for the function
**      // followed by the next function
**      struct _gcBINARY_FUNCTION {
**          gctINT16                    argumentCount;
**          gctINT16                    variableCount;
**          gctUINT16                    codeStart;
**          gctUINT16                    codeCount;
**            gctUINT16                    label;
**            gctINT16                    nameLength;
**            char                        name[1];
**        }
**        functions[];
**
**      struct _gcBINARY_ARGUMENT
**        {
**            gctUINT16                    index;
**            gctUINT8                    enable;
**            gctUINT8                    qualifier;
**        }
**        arguments;
**
**      // kernelFunctionCount structures.  Each structure is followed by the kernel function
**      // name, without the trailing '\0'.  Each structure is optionally padded
**      // with 0x00 to align to a 16-bit boundary.
**      // Next follows:
**      //       arguments       gcBINARY_ARGUMENT
**      //       uniforms        gcBINARY_UNIFORM_EX
**      //       image samplers  gcBINARY_IMAGE_SAMPLER
**      //       variables       gcBINARY_VARIABLE
**      //       properties      gcBINARY_KERNEL_FUNCTION_PROPERTY
**      //       property values gctINT32
**      // for the kernel function followed by the next kernel function
**
**      struct _gcBINARY_KERNEL_FUNCTION {
**            gctINT16                argumentCount;
**            gctUINT16                label;
**            gctUINT32                localMemorySize;
**            gctINT16                uniformArgumentCount;
**            gctINT16                samplerIndex;
**            gctINT16                imageSamplerCount;
**            gctINT16                variableCount;
**            gctINT16                propertyCount;
**            gctINT16                propertyValueCount;
**            gctUINT16                codeStart;
**            gctUINT16                codeCount;
**            gctUINT16                codeEnd;
**            gctINT16                nameLength;
**            char                    name[1];
**        }
**        kernelFunctions[];
**
**      // Number of instructions in code.
**      gctINT16                codeCount;
**
**      // The code buffer.
**      glSL_INSTRUCTION    code[];
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctPOINTER Buffer
**          Pointer to a binary buffer containing the shader data to load.
**
**      gctSIZE_T BufferSize
**          Number of bytes inside the binary buffer pointed to by 'Buffer'.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_LoadEx(
    IN gcSHADER Shader,
    IN gctPOINTER Buffer,
    IN gctSIZE_T BufferSize
    )
{
    gctSIZE_T bytes;
    gctUINT16 * count;
    gctUINT16 * privateMemorySize;
    gctUINT16 * constantMemorySize;
    gctUINT   * optimizationOption;
    gctUINT16 * maxKernelFunctionArgs;
    gceSTATUS status;
    gcBINARY_ATTRIBUTE binaryAttribute;
    gctUINT i, j;
    gctSIZE_T length, binarySize;
    gcATTRIBUTE attribute;
    gcBINARY_UNIFORM_EX binaryUniform;
    gcUNIFORM uniform;
    gcBINARY_OUTPUT binaryOutput;
    gcOUTPUT output;
    gcBINARY_VARIABLE binaryVariable;
    gcVARIABLE variable;
    gcBINARY_FUNCTION binaryFunction;
    gcFUNCTION function;
    gcBINARY_ARGUMENT binaryArgument;
    gcsFUNCTION_ARGUMENT_PTR argument;
    gcBINARY_KERNEL_FUNCTION binaryKernelFunc;
    gcKERNEL_FUNCTION kernelFunction;
    gcBINARY_KERNEL_FUNCTION_PROPERTY binaryKernelFuncProp;
    gcsKERNEL_FUNCTION_PROPERTY_PTR kernelFuncProp;
    gcBINARY_IMAGE_SAMPLER binaryImageSampler;
    gcsIMAGE_SAMPLER_PTR imageSampler;
    gctINT_PTR binaryPropertyValue;

    gcSL_INSTRUCTION code;
#if GC_ENABLE_LOADTIME_OPT
    gctUINT8 *       curPos;
#endif /* GC_ENABLE_LOADTIME_OPT */
    gctPOINTER pointer = gcvNULL;
    gctUINT32           shaderVersion;

    gcmHEADER_ARG("Shader=0x%x Buffer=0x%x BufferSize=%lu",
                  Shader, Buffer, BufferSize);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Buffer != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(BufferSize > 0);

    if(!Shader) {
       gcmFATAL("gcSHADER_Load: null shader handle passed");
       gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
       return gcvSTATUS_INVALID_ARGUMENT;
    }

    _gcSHADER_Clean(Shader);

    /* Load the Header */
    status = gcSHADER_LoadHeader(Shader, Buffer, BufferSize, &shaderVersion);
    if(gcmIS_ERROR(status)) {
       gcmFOOTER();
       return status;
    }

    bytes = BufferSize - _gcdShaderBinaryHeaderSize;

    /************************************************************************/
    /*                              attributes                              */
    /************************************************************************/

    /* Get the attribute count. */
    count  = (gctUINT16 *) ((gctUINT8 *)Buffer + _gcdShaderBinaryHeaderSize);

    if (bytes < sizeof(gctUINT16))
    {
        /* Invalid attribute count. */
        gcmFATAL("gcSHADER_Load: Invalid attributeCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Save the attribute count. */
    Shader->attributeCount = *count;

    /* Point to the first attribute. */
    binaryAttribute = (gcBINARY_ATTRIBUTE) (count + 1);
    bytes          -= sizeof(gctUINT16);

    if (Shader->attributeCount > 0)
    {
        /* Allocate the array of gcATTRIBUTE structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->attributeCount * sizeof(gcATTRIBUTE),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->attributeCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->attributes = pointer;

        /* Parse all attributes. */
        for (i = 0; i < Shader->attributeCount; i++)
        {
            /* Get the length of the attribute name. */
            length = (bytes < sizeof(struct _gcBINARY_ATTRIBUTE))
                         ? 0
                         : binaryAttribute->nameLength;

            /* Test for special length. */
            if ((gctINT) length < 0)
            {
                length = 0;
            }

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_ATTRIBUTE, name) + length,
                                  2);

            if (bytes < binarySize)
            {
                /* Roll back. */
                Shader->attributeCount = i;

                /* Invalid attribute. */
                gcmFATAL("gcSHADER_Load: Invalid attribute");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the attribute inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                    gcmOFFSETOF(_gcATTRIBUTE, name) + length + 1,
                                    &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->attributeCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                gcmFOOTER();
                return status;
            }

            attribute = pointer;

            /* Copy attribute to the gcSHADER object. */
            Shader->attributes[i]     = attribute;
            attribute->object.type    = gcvOBJ_ATTRIBUTE;
            attribute->index          = (gctUINT16) i;
            attribute->type           = (gcSHADER_TYPE) binaryAttribute->type;
            attribute->isTexture      = binaryAttribute->isTexture;
            attribute->enabled        = gcvTRUE;
            attribute->arraySize      = binaryAttribute->arraySize;
            attribute->inputIndex     = -1;
            attribute->nameLength     = binaryAttribute->nameLength;
            attribute->name[length]   = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(attribute->name,
                                       binaryAttribute->name,
                                       length));
            }

            /* Point to next attribute. */
            binaryAttribute = (gcBINARY_ATTRIBUTE)
                              ((gctINT8 *) binaryAttribute + binarySize);
            bytes          -= binarySize;
        }
    }

    /************************************************************************/
    /*                                  uniforms                            */
    /************************************************************************/

    /* Get the uniform count. */
    count = (gctUINT16 *) binaryAttribute;

    if (bytes < sizeof(gctUINT16))
    {
        /* Invalid uniform count. */
        gcmFATAL("gcSHADER_Load: Invalid uniformCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Save the uniform count. */
    Shader->uniformCount = *count;

    /* Point to the first uniform. */
    binaryUniform = (gcBINARY_UNIFORM_EX) (count + 1);
    bytes        -= sizeof(gctUINT16);

    if (Shader->uniformCount > 0)
    {
        gctINT   previousVersionAdjustment = 0;

        if (shaderVersion == gcdSL_SHADER_BINARY_BEFORE_LTC_FILE_VERSION)
        {
#if GC_ENABLE_LOADTIME_OPT
            /* fields which are not in the version */
            previousVersionAdjustment = sizeof(binaryUniform->glUniformIndex);
#endif /* GC_ENABLE_LOADTIME_OPT */
        }

        /* Allocate the array of gcUNIFORM structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->uniformCount * sizeof(gcUNIFORM),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->uniformCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->uniforms = pointer;
        Shader->uniformArrayCount = Shader->uniformCount;

        /* Parse all uniforms. */
        for (i = 0; i < Shader->uniformCount; i++)
        {
            /* Get the length of the uniform name. */
            length = (bytes < sizeof(struct _gcBINARY_UNIFORM_EX))
                         ? 0
                         : binaryUniform->nameLength;

            /* Compute the number of bytes required for current version of the strcut. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_UNIFORM_EX, name) + length,
                                 2);

            if (bytes < (binarySize - previousVersionAdjustment) )
            {
                /* Roll back. */
                Shader->uniformCount = i;

                /* Invalid uniform. */
                gcmFATAL("gcSHADER_Load: Invalid uniform");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the uniform inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   gcmOFFSETOF(_gcUNIFORM, name) + length + 1,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->uniformCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                gcmFOOTER();
                return status;
            }

            uniform = pointer;

            /* Copy uniform to the gcSHADER object. */
            Shader->uniforms[i]   = uniform;
            uniform->object.type  = gcvOBJ_UNIFORM;
            uniform->index        = binaryUniform->index;
            uniform->type         = (gcSHADER_TYPE) binaryUniform->type;
            uniform->flags        = binaryUniform->flags;
            uniform->format       = binaryUniform->format;
            uniform->isPointer    = binaryUniform->isPointer;
            uniform->arraySize    = binaryUniform->arraySize;
            uniform->nameLength   = length;

            if (shaderVersion == gcdSL_SHADER_BINARY_BEFORE_LTC_FILE_VERSION)
            {
#if GC_ENABLE_LOADTIME_OPT
                uniform->glUniformIndex   = (gctUINT16)(gctINT16) -1;
#endif /* GC_ENABLE_LOADTIME_OPT */
            }
            else
            {
#if GC_ENABLE_LOADTIME_OPT
                uniform->glUniformIndex   = binaryUniform->glUniformIndex;
#endif /* GC_ENABLE_LOADTIME_OPT */
            }

            /* Copy name. */
            uniform->name[length] = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(uniform->name,
                                       binaryUniform->name - previousVersionAdjustment,
                                       length));
            }

            /* substract fields which are not in the file version */
            binarySize -= previousVersionAdjustment;

            /* Point to next uniform. */
            binaryUniform = (gcBINARY_UNIFORM_EX)
                            ((gctUINT8 *) binaryUniform + binarySize);
            bytes        -= binarySize;
        }
    }

    /************************************************************************/
    /*                                  outputs                             */
    /************************************************************************/

    /* Get the output count. */
    count = (gctUINT16 *) binaryUniform;

    if ( (bytes < sizeof(gctUINT16)) ||
         ((*count <= 0) && (Shader->type != gcSHADER_TYPE_CL)))
    {
        /* Invalid output count. */
        gcmFATAL("gcSHADER_Load: Invalid outputCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the first output. */
    binaryOutput = (gcBINARY_OUTPUT) (count + 1);
    bytes       -= sizeof(gctUINT16);

    /* Save the output count. */
    Shader->outputCount = *count;

    if (*count > 0) {

        /* Allocate the array of gcOUTPUT structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->outputCount * sizeof(gcOUTPUT),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->outputCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->outputs = pointer;

        /* Parse all outputs. */
        for (i = 0; i < Shader->outputCount; i++)
        {
            /* Get the length of the output name. */
            length = (bytes < sizeof(struct _gcBINARY_OUTPUT))
                         ? 0
                         : binaryOutput->nameLength;

            /* Test for special length. */
            if ((gctINT) length < 0)
            {
                length = 0;
            }

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_OUTPUT, name) + length, 2);

            if (bytes < binarySize)
            {
                /* Roll back. */
                Shader->outputCount = i;

                /* Invalid output. */
                gcmFATAL("gcSHADER_Load: Invalid output");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the output inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   gcmOFFSETOF(_gcOUTPUT, name) + length + 1,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->outputCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                gcmFOOTER();
                return status;
            }

            output = pointer;

            /* Copy output to shader. */
            Shader->outputs[i]   = output;
            output->object.type  = gcvOBJ_OUTPUT;
            output->type         = (gcSHADER_TYPE) binaryOutput->type;
            output->arraySize    = binaryOutput->arraySize;
            output->tempIndex    = binaryOutput->tempIndex;
            output->physical     = gcvFALSE;
            output->nameLength   = binaryOutput->nameLength;
            output->name[length] = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(output->name, binaryOutput->name, length));
            }

            /* Point to next output. */
            binaryOutput = (gcBINARY_OUTPUT)
                           ((gctUINT8 *) binaryOutput + binarySize);
            bytes       -= binarySize;
        }
    }

    /************************************************************************/
    /*                          global variables                            */
    /************************************************************************/

    /* Get the variable count. */
    count = (gctUINT16 *) binaryOutput;

    if ( (bytes < sizeof(gctUINT16)) ||
         ((*count <= 0) && (Shader->type != gcSHADER_TYPE_CL)))
    {
        /* Invalid variable count. */
        gcmFATAL("gcSHADER_Load: Invalid variableCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the first variable. */
    binaryVariable = (gcBINARY_VARIABLE) (count + 1);
    bytes       -= sizeof(gctUINT16);

    /* Save the variable count. */
    Shader->variableCount = *count;

    if (*count > 0) {

        gctINT   previousVersionAdjustment = 0;
        if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
        {
            /* fields which are not in the version */
            previousVersionAdjustment = sizeof(binaryVariable->varCategory) +
                                        sizeof(binaryVariable->firstChild) +
                                        sizeof(binaryVariable->nextSibling) +
                                        sizeof(binaryVariable->prevSibling) +
                                        sizeof(binaryVariable->parent);
        }

        /* Allocate the array of gcVARIABLE structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->variableCount * sizeof(gcVARIABLE),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->variableCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->variables = pointer;

        /* Parse all variables. */
        for (i = 0; i < Shader->variableCount; i++)
        {
            /* Get the length of the variable name. */
            length = (bytes < sizeof(struct _gcBINARY_VARIABLE))
                         ? 0
                         : binaryVariable->nameLength;

            /* Test for special length. */
            if ((gctINT) length < 0)
            {
                length = 0;
            }

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_VARIABLE, name) + length, 2);

            if (bytes < binarySize - previousVersionAdjustment)
            {
                /* Roll back. */
                Shader->variableCount = i;

                /* Invalid variable. */
                gcmFATAL("gcSHADER_Load: Invalid variable");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the variable inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   gcmOFFSETOF(_gcVARIABLE, name) + length + 1,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->variableCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                gcmFOOTER();
                return status;
            }

            variable = pointer;

            /* Copy variable to shader. */
            Shader->variables[i]   = variable;
            variable->object.type  = gcvOBJ_VARIABLE;

            if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
            {
                variable->varCategory = gcSHADER_VAR_CATEGORY_NORMAL;
                variable->firstChild = -1;
                variable->nextSibling = -1;
                variable->prevSibling = -1;
                variable->parent = -1;
                variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
            }
            else
            {
            variable->varCategory = binaryVariable->varCategory;
            variable->firstChild = binaryVariable->firstChild;
            variable->nextSibling = binaryVariable->nextSibling;
            variable->prevSibling = binaryVariable->prevSibling;
            variable->parent = binaryVariable->parent;

            if (binaryVariable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
            else
                variable->u.numStructureElement = binaryVariable->u.numStructureElement;
            }

            variable->arraySize    = binaryVariable->arraySize;
            variable->tempIndex    = binaryVariable->tempIndex;
            variable->nameLength   = binaryVariable->nameLength;
            variable->name[length] = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(variable->name, binaryVariable->name - previousVersionAdjustment, length));
            }

            /* substract fields which are not in the file version */
            binarySize -= previousVersionAdjustment; 

            /* Point to next variable. */
            binaryVariable = (gcBINARY_VARIABLE)
                           ((gctUINT8 *) binaryVariable + binarySize);
            bytes       -= binarySize;
        }
    }

    /************************************************************************/
    /*                              functions                               */
    /************************************************************************/

    /* Get the function count. */
    count = (gctUINT16 *) binaryVariable;

    if ( (bytes < sizeof(gctUINT16)) ||
         ((*count <= 0) && (Shader->type != gcSHADER_TYPE_CL)))
    {
        /* Invalid function count. */
        gcmFATAL("gcSHADER_Load: Invalid functionCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the first function. */
    binaryFunction = (gcBINARY_FUNCTION) (count + 1);
    bytes       -= sizeof(gctUINT16);

    /* Save the function count. */
    Shader->functionCount = *count;

    if (*count > 0) {

        /* Allocate the array of gcFUNCTION structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->functionCount * sizeof(gcFUNCTION),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->functionCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->functions = pointer;

        /* Parse all functions. */
        for (i = 0; i < Shader->functionCount; i++)
        {
            /* Get the length of the function name. */
            length = (bytes < sizeof(struct _gcBINARY_FUNCTION))
                         ? 0
                         : binaryFunction->nameLength;

            /* Test for special length. */
            if ((gctINT) length < 0)
            {
                length = 0;
            }

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_FUNCTION, name) + length, 2);

            if (bytes < binarySize)
            {
                /* Roll back. */
                Shader->functionCount = i;

                /* Invalid function. */
                gcmFATAL("gcSHADER_Load: Invalid function");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the function inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   gcmOFFSETOF(_gcsFUNCTION, name) + length + 1,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->functionCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                gcmFOOTER();
                return status;
            }

            function = pointer;

            /* Copy function to shader. */
            Shader->functions[i]   = function;
            function->object.type  = gcvOBJ_FUNCTION;

            function->argumentCount = binaryFunction->argumentCount;
            function->arguments     = gcvNULL;
            function->variableCount = binaryFunction->variableCount;
            function->variables     = gcvNULL;
            function->codeStart     = binaryFunction->codeStart;
            function->codeCount     = binaryFunction->codeCount;
            function->label         = binaryFunction->label;
            function->nameLength    = binaryFunction->nameLength;
            function->name[length]  = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(function->name, binaryFunction->name, length));
            }

            /* Point to first argument. */
            binaryArgument = (gcBINARY_ARGUMENT)
                           ((gctUINT8 *) binaryFunction + binarySize);
            bytes         -= binarySize;

            /* Function arguments */
            if (function->argumentCount > 0) {

                binarySize = function->argumentCount * sizeof(gcsFUNCTION_ARGUMENT);

                if (bytes < binarySize)
                {
                    /* Roll back. */
                    Shader->functionCount = i;
                    function->argumentCount = 0;

                    /* Invalid argument. */
                    gcmFATAL("gcSHADER_Load: Invalid argument");
                    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                    return gcvSTATUS_INVALID_DATA;
                }

                /* Allocate the array of gcsFUNCTION_ARGUMENT structures. */
                status = gcoOS_Allocate(gcvNULL, binarySize, &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->functionCount = i;
                    function->argumentCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                function->arguments = pointer;

                for (j=0; j<function->argumentCount; j++) {
                    argument = &function->arguments[j];

                    argument->index     = binaryArgument->index;
                    argument->enable    = binaryArgument->enable;
                    argument->qualifier = binaryArgument->qualifier;

                    /* Point to next argument. */
                    binaryArgument++;
                    bytes -= sizeof(gcsFUNCTION_ARGUMENT);
                }
            }

            /* Point to first variable. */
            binaryVariable = (gcBINARY_VARIABLE) binaryArgument;

            /* Function variables */
            if (function->variableCount > 0) {

                gctINT   previousVersionAdjustment = 0;
                if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
                {
                    /* fields which are not in the version */
                    previousVersionAdjustment = sizeof(binaryVariable->varCategory) +
                                                sizeof(binaryVariable->firstChild) +
                                                sizeof(binaryVariable->nextSibling) +
                                                sizeof(binaryVariable->prevSibling) +
                                                sizeof(binaryVariable->parent);
                }

                /* Allocate the array of gcVARIABLE structure pointers. */
                status = gcoOS_Allocate(gcvNULL,
                                       function->variableCount * sizeof(gcVARIABLE),
                                       &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->functionCount = i;
                    function->variableCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                function->variables = pointer;

                /* Parse all variables. */
                for (j = 0; j < function->variableCount; j++)
                {
                    /* Get the length of the variable name. */
                    length = (bytes < sizeof(struct _gcBINARY_VARIABLE))
                                 ? 0
                                 : binaryVariable->nameLength;

                    /* Test for special length. */
                    if ((gctINT) length < 0)
                    {
                        length = 0;
                    }

                    /* Compute the number of bytes required. */
                    binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_VARIABLE, name) + length, 2);

                    if (bytes < binarySize - previousVersionAdjustment)
                    {
                        /* Roll back. */
                        Shader->functionCount = i;
                        function->variableCount = j;

                        /* Invalid variable. */
                        gcmFATAL("gcSHADER_Load: Invalid variable");
                        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                        return gcvSTATUS_INVALID_DATA;
                    }

                    /* Allocate memory for the variable inside the gcSHADER object. */
                    status = gcoOS_Allocate(gcvNULL,
                                           gcmOFFSETOF(_gcVARIABLE, name) + length + 1,
                                           &pointer);

                    if (gcmIS_ERROR(status))
                    {
                        /* Roll back. */
                        Shader->functionCount = i;
                        function->variableCount = j;

                        /* Error. */
                        gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                        gcmFOOTER();
                        return status;
                    }

                    variable = pointer;

                    /* Copy variable to shader. */
                    function->variables[j]   = variable;
                    variable->object.type  = gcvOBJ_VARIABLE;

                    if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
                    {
                        variable->varCategory = gcSHADER_VAR_CATEGORY_NORMAL;
                        variable->firstChild = -1;
                        variable->nextSibling = -1;
                        variable->prevSibling = -1;
                        variable->parent = -1;
                        variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
                    }
                    else
                    {
                    variable->varCategory = binaryVariable->varCategory;
                    variable->firstChild = binaryVariable->firstChild;
                    variable->nextSibling = binaryVariable->nextSibling;
                    variable->prevSibling = binaryVariable->prevSibling;
                    variable->parent = binaryVariable->parent;

                    if (binaryVariable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                        variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
                    else
                        variable->u.numStructureElement = binaryVariable->u.numStructureElement;
                    }

                    variable->arraySize    = binaryVariable->arraySize;
                    variable->tempIndex    = binaryVariable->tempIndex;
                    variable->nameLength   = binaryVariable->nameLength;
                    variable->name[length] = '\0';

                    if (length > 0)
                    {
                        gcmVERIFY_OK(gcoOS_MemCopy(variable->name, binaryVariable->name - previousVersionAdjustment, length));
                    }

                    /* substract fields which are not in the file version */
                    binarySize -= previousVersionAdjustment; 

                    /* Point to next variable. */
                    binaryVariable = (gcBINARY_VARIABLE)
                                   ((gctUINT8 *) binaryVariable + binarySize);
                    bytes       -= binarySize;
                }
            }

            /* Point to next function. */
            binaryFunction = (gcBINARY_FUNCTION) binaryVariable;
        }
    }

    /************************************************************************/
    /*                          kernel functions                            */
    /************************************************************************/

    /* Get the max kernel function args. */
    maxKernelFunctionArgs = (gctUINT16 *) binaryFunction;

    if ( (bytes < sizeof(gctUINT16)) ||
         ((*maxKernelFunctionArgs > 0) && (Shader->type != gcSHADER_TYPE_CL)))
    {
        /* Invalid  max kernel function args */
        gcmFATAL("gcSHADER_Load: Invalid max kernel function args");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Save the max kernel function args. */
    Shader->maxKernelFunctionArgs = *maxKernelFunctionArgs;

    /* Get the kernel function count. */
    count = (gctUINT16 *) (maxKernelFunctionArgs + 1);
    bytes       -= sizeof(gctUINT16);

    if ( (bytes < sizeof(gctUINT16)) ||
         ((*count > 0) && (Shader->type != gcSHADER_TYPE_CL)))
    {
        /* Invalid function count. */
        gcmFATAL("gcSHADER_Load: Invalid functionCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the first kernel function. */
    binaryKernelFunc = (gcBINARY_KERNEL_FUNCTION) (count + 1);
    bytes       -= sizeof(gctUINT16);

    /* Save the kernel function count. */
    Shader->kernelFunctionCount = *count;

    if (*count > 0) {

        /* Allocate the array of gcKERNEL_FUNCTION structure pointers. */
        status = gcoOS_Allocate(gcvNULL,
                               Shader->kernelFunctionCount * sizeof(gcKERNEL_FUNCTION),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->kernelFunctionCount = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->kernelFunctions = pointer;

        /* Parse all kernel functions. */
        for (i = 0; i < Shader->kernelFunctionCount; i++)
        {
            /* Get the length of the kernel function name. */
            length = (bytes < sizeof(struct _gcBINARY_KERNEL_FUNCTION))
                         ? 0
                         : binaryKernelFunc->nameLength;

            /* Test for special length. */
            if ((gctINT) length < 0)
            {
                length = 0;
            }

            /* Compute the number of bytes required. */
            binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_KERNEL_FUNCTION, name) + length, 2);

            if (bytes < binarySize)
            {
                /* Roll back. */
                Shader->kernelFunctionCount = i;

                /* Invalid function. */
                gcmFATAL("gcSHADER_Load: Invalid kernel function");
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                return gcvSTATUS_INVALID_DATA;
            }

            /* Allocate memory for the kernel function inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   gcmOFFSETOF(_gcsKERNEL_FUNCTION, name) + length + 1,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->kernelFunctionCount = i;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                gcmFOOTER();
                return status;
            }

            kernelFunction = pointer;

            /* Copy kernel function to shader. */
            Shader->kernelFunctions[i]   = kernelFunction;
            kernelFunction->object.type  = gcvOBJ_KERNEL_FUNCTION;
            kernelFunction->shader       = Shader;

            kernelFunction->argumentCount        = binaryKernelFunc->argumentCount;
            kernelFunction->arguments            = gcvNULL;
            kernelFunction->label                = binaryKernelFunc->label;
            gcoOS_MemCopy(&kernelFunction->localMemorySize, &binaryKernelFunc->localMemorySize, sizeof(gctUINT32));
            kernelFunction->uniformArgumentCount = binaryKernelFunc->uniformArgumentCount;
            kernelFunction->uniformArguments     = gcvNULL;
            kernelFunction->samplerIndex         = binaryKernelFunc->samplerIndex;
            kernelFunction->imageSamplerCount    = binaryKernelFunc->imageSamplerCount;
            kernelFunction->imageSamplers        = gcvNULL;
            kernelFunction->variableCount        = binaryKernelFunc->variableCount;
            kernelFunction->variables            = gcvNULL;
            kernelFunction->propertyCount        = binaryKernelFunc->propertyCount;
            kernelFunction->properties           = gcvNULL;
            kernelFunction->propertyValueCount   = binaryKernelFunc->propertyValueCount;
            kernelFunction->propertyValues       = gcvNULL;
            kernelFunction->codeStart            = binaryKernelFunc->codeStart;
            kernelFunction->codeCount            = binaryKernelFunc->codeCount;
            kernelFunction->codeEnd              = binaryKernelFunc->codeEnd;
            kernelFunction->isMain               = binaryKernelFunc->isMain;
            kernelFunction->nameLength           = binaryKernelFunc->nameLength;
            kernelFunction->name[length]         = '\0';

            if (length > 0)
            {
                gcmVERIFY_OK(gcoOS_MemCopy(kernelFunction->name, binaryKernelFunc->name, length));
            }

            /* Point to first argument. */
            binaryArgument = (gcBINARY_ARGUMENT)
                           ((gctUINT8 *) binaryKernelFunc + binarySize);
            bytes         -= binarySize;

            /* Kernel function arguments */
            if (kernelFunction->argumentCount > 0) {

                binarySize = kernelFunction->argumentCount * sizeof(gcsFUNCTION_ARGUMENT);

                if (bytes < binarySize)
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->argumentCount = 0;

                    /* Invalid argument. */
                    gcmFATAL("gcSHADER_Load: Invalid argument");
                    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                    return gcvSTATUS_INVALID_DATA;
                }

                /* Allocate the array of gcsFUNCTION_ARGUMENT structures. */
                status = gcoOS_Allocate(gcvNULL, binarySize, &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->argumentCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                kernelFunction->arguments = pointer;

                for (j=0; j<kernelFunction->argumentCount; j++) {
                    argument = &kernelFunction->arguments[j];

                    argument->index     = binaryArgument->index;
                    argument->enable    = binaryArgument->enable;
                    argument->qualifier = binaryArgument->qualifier;

                    /* Point to next argument. */
                    binaryArgument++;
                    bytes -= sizeof(gcsFUNCTION_ARGUMENT);
                }
            }

            /* Point to first uniform. */
            binaryUniform = (gcBINARY_UNIFORM_EX) binaryArgument;

            /* Kernel function uniform arguments */
            if (kernelFunction->uniformArgumentCount > 0) {

                /* Allocate the array of gcUNIFORM structure pointers. */
                status = gcoOS_Allocate(gcvNULL,
                                       kernelFunction->uniformArgumentCount * sizeof(gcUNIFORM),
                                       &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->uniformArgumentCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                kernelFunction->uniformArguments = pointer;

                /* Parse all variables. */
                for (j = 0; j < kernelFunction->uniformArgumentCount; j++)
                {
                    /* Get the length of the uniform argument name. */
                    length = (bytes < sizeof(struct _gcBINARY_UNIFORM_EX))
                                 ? 0
                                 : binaryUniform->nameLength;

                    /* Test for special length. */
                    if ((gctINT) length < 0)
                    {
                        length = 0;
                    }

                    /* Compute the number of bytes required. */
                    binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_UNIFORM_EX, name) + length, 2);

                    if (bytes < binarySize)
                    {
                        /* Roll back. */
                        Shader->kernelFunctionCount = i;
                        kernelFunction->uniformArgumentCount = j;

                        /* Invalid variable. */
                        gcmFATAL("gcSHADER_Load: Invalid uniform argument");
                        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                        return gcvSTATUS_INVALID_DATA;
                    }

                    /* Allocate memory for the uniform argument inside the gcSHADER object. */
                    status = gcoOS_Allocate(gcvNULL,
                                           gcmOFFSETOF(_gcUNIFORM, name) + length + 1,
                                           &pointer);

                    if (gcmIS_ERROR(status))
                    {
                        /* Roll back. */
                        Shader->kernelFunctionCount = i;
                        kernelFunction->uniformArgumentCount = j;

                        /* Error. */
                        gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                        gcmFOOTER();
                        return status;
                    }

                    uniform = pointer;

                    /* Copy uniform argument to shader. */
                    kernelFunction->uniformArguments[j]   = uniform;
                    uniform->object.type  = gcvOBJ_UNIFORM;
                    uniform->index        = binaryUniform->index;
                    uniform->type         = (gcSHADER_TYPE) binaryUniform->type;
                    uniform->flags        = binaryUniform->flags;
                    uniform->format       = binaryUniform->format;
                    uniform->isPointer    = binaryUniform->isPointer;
                    uniform->arraySize    = binaryUniform->arraySize;
                    uniform->nameLength   = length;
                    uniform->name[length] = '\0';
#if GC_ENABLE_LOADTIME_OPT
                    uniform->flags            = (gceUNIFORM_FLAGS)binaryUniform->flags;
                    uniform->glUniformIndex   = binaryUniform->glUniformIndex;
#endif /* GC_ENABLE_LOADTIME_OPT */

                    if (length > 0)
                    {
                        gcmVERIFY_OK(gcoOS_MemCopy(uniform->name, binaryUniform->name, length));
                    }

                    /* Point to next uniform argument. */
                    binaryUniform = (gcBINARY_UNIFORM_EX)
                                   ((gctUINT8 *) binaryUniform + binarySize);
                    bytes       -= binarySize;
                }
            }

            /* Point to first image sampler. */
            binaryImageSampler = (gcBINARY_IMAGE_SAMPLER) binaryUniform;

            /* Kernel function image samplers */
            if (kernelFunction->imageSamplerCount > 0) {

                binarySize = kernelFunction->imageSamplerCount * sizeof(struct _gcBINARY_IMAGE_SAMPLER);

                if (bytes < binarySize)
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->imageSamplerCount = 0;

                    /* Invalid argument. */
                    gcmFATAL("gcSHADER_Load: Invalid image sampler");
                    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                    return gcvSTATUS_INVALID_DATA;
                }

                /* Allocate the array of gcsIMAGE_SAMPLER_PTR structures. */
                status = gcoOS_Allocate(gcvNULL, binarySize, &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->imageSamplerCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                kernelFunction->imageSamplers = pointer;

                for (j=0; j<kernelFunction->imageSamplerCount; j++) {
                    imageSampler = &kernelFunction->imageSamplers[j];

                    imageSampler->isConstantSamplerType    = binaryImageSampler->isConstantSamplerType;
                    imageSampler->imageNum     = binaryImageSampler->imageNum;
                    imageSampler->samplerType  = binaryImageSampler->samplerType;

                    /* Point to next image sampler. */
                    binaryImageSampler++;
                    bytes -= sizeof(struct _gcBINARY_IMAGE_SAMPLER);
                }
            }

            /* Point to first variable. */
            binaryVariable = (gcBINARY_VARIABLE) binaryImageSampler;

            /* Kernel function variables */
            if (kernelFunction->variableCount > 0) {

                gctINT   previousVersionAdjustment = 0;
                if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
                {
                    /* fields which are not in the version */
                    previousVersionAdjustment = sizeof(binaryVariable->varCategory) +
                                                sizeof(binaryVariable->firstChild) +
                                                sizeof(binaryVariable->nextSibling) +
                                                sizeof(binaryVariable->prevSibling) +
                                                sizeof(binaryVariable->parent);
                }

                /* Allocate the array of gcVARIABLE structure pointers. */
                status = gcoOS_Allocate(gcvNULL,
                                       kernelFunction->variableCount * sizeof(gcVARIABLE),
                                       &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->variableCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                kernelFunction->variables = pointer;

                /* Parse all variables. */
                for (j = 0; j < kernelFunction->variableCount; j++)
                {
                    /* Get the length of the variable name. */
                    length = (bytes < sizeof(struct _gcBINARY_VARIABLE))
                                 ? 0
                                 : binaryVariable->nameLength;

                    /* Test for special length. */
                    if ((gctINT) length < 0)
                    {
                        length = 0;
                    }

                    /* Compute the number of bytes required. */
                    binarySize = gcmALIGN(gcmOFFSETOF(_gcBINARY_VARIABLE, name) + length, 2);

                    if (bytes < binarySize - previousVersionAdjustment)
                    {
                        /* Roll back. */
                        Shader->kernelFunctionCount = i;
                        kernelFunction->variableCount = j;

                        /* Invalid variable. */
                        gcmFATAL("gcSHADER_Load: Invalid variable");
                        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                        return gcvSTATUS_INVALID_DATA;
                    }

                    /* Allocate memory for the variable inside the gcSHADER object. */
                    status = gcoOS_Allocate(gcvNULL,
                                           gcmOFFSETOF(_gcVARIABLE, name) + length + 1,
                                           &pointer);

                    if (gcmIS_ERROR(status))
                    {
                        /* Roll back. */
                        Shader->kernelFunctionCount = i;
                        kernelFunction->variableCount = j;

                        /* Error. */
                        gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                        gcmFOOTER();
                        return status;
                    }

                    variable = pointer;

                    /* Copy variable to shader. */
                    kernelFunction->variables[j]   = variable;
                    variable->object.type  = gcvOBJ_VARIABLE;

                    if (shaderVersion <= gcdSL_SHADER_BINARY_BEFORE_STRUCT_SYMBOL_FILE_VERSION)
                    {
                        variable->varCategory = gcSHADER_VAR_CATEGORY_NORMAL;
                        variable->firstChild = -1;
                        variable->nextSibling = -1;
                        variable->prevSibling = -1;
                        variable->parent = -1;
                        variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
                    }
                    else
                    {
                    variable->varCategory = binaryVariable->varCategory;
                    variable->firstChild = binaryVariable->firstChild;
                    variable->nextSibling = binaryVariable->nextSibling;
                    variable->prevSibling = binaryVariable->prevSibling;
                    variable->parent = binaryVariable->parent;

                    if (binaryVariable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                        variable->u.type = (gcSHADER_TYPE) binaryVariable->u.type;
                    else
                        variable->u.numStructureElement = binaryVariable->u.numStructureElement;
                    }

                    variable->arraySize    = binaryVariable->arraySize;
                    variable->tempIndex    = binaryVariable->tempIndex;
                    variable->nameLength   = binaryVariable->nameLength;
                    variable->name[length] = '\0';

                    if (length > 0)
                    {
                        gcmVERIFY_OK(gcoOS_MemCopy(variable->name, binaryVariable->name - previousVersionAdjustment, length));
                    }

                    /* substract fields which are not in the file version */
                    binarySize -= previousVersionAdjustment; 

                    /* Point to next variable. */
                    binaryVariable = (gcBINARY_VARIABLE)
                                   ((gctUINT8 *) binaryVariable + binarySize);
                    bytes       -= binarySize;
                }
            }

            /* Point to first kernel function property. */
            binaryKernelFuncProp = (gcBINARY_KERNEL_FUNCTION_PROPERTY) binaryVariable;

            /* Kernel function properties */
            if (kernelFunction->propertyCount > 0) {

                binarySize = kernelFunction->propertyCount * sizeof(gcsKERNEL_FUNCTION_PROPERTY);

                if (bytes < binarySize)
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->propertyCount = 0;

                    /* Invalid argument. */
                    gcmFATAL("gcSHADER_Load: Invalid kernel function property");
                    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                    return gcvSTATUS_INVALID_DATA;
                }

                /* Allocate the array of gcsKERNEL_FUNCTION_PROPERTY_PTR structures. */
                status = gcoOS_Allocate(gcvNULL, binarySize, &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->propertyCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                kernelFunction->properties = pointer;

                for (j=0; j<kernelFunction->propertyCount; j++) {
                    kernelFuncProp = &kernelFunction->properties[j];

                    gcoOS_MemCopy(&kernelFuncProp->propertyType, &binaryKernelFuncProp->propertyType,  sizeof(gctUINT32));
                    gcoOS_MemCopy(&kernelFuncProp->propertySize, &binaryKernelFuncProp->propertySize,  sizeof(gctUINT32));

                    /* Point to next kernel function property. */
                    binaryKernelFuncProp++;
                    bytes -= sizeof(gcsKERNEL_FUNCTION_PROPERTY);
                }
            }

            /* Point to first kernel function property value. */
            binaryPropertyValue = (gctINT32_PTR) binaryKernelFuncProp;

            /* Kernel function properties */
            if (kernelFunction->propertyValueCount > 0) {

                binarySize = kernelFunction->propertyValueCount * sizeof(gctINT32);

                if (bytes < binarySize)
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->propertyValueCount = 0;

                    /* Invalid argument. */
                    gcmFATAL("gcSHADER_Load: Invalid kernel function property value");
                    gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
                    return gcvSTATUS_INVALID_DATA;
                }

                /* Allocate the array of gctINT32 */
                status = gcoOS_Allocate(gcvNULL, binarySize, &pointer);

                if (gcmIS_ERROR(status))
                {
                    /* Roll back. */
                    Shader->kernelFunctionCount = i;
                    kernelFunction->propertyValueCount = 0;

                    /* Error. */
                    gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
                    gcmFOOTER();
                    return status;
                }

                kernelFunction->propertyValues = pointer;

                for (j=0; j<kernelFunction->propertyValueCount; j++) {
                    gcoOS_MemCopy(&kernelFunction->propertyValues[j], &binaryPropertyValue[j], sizeof(gctUINT32));
                }

                /* Point to next property value. */
                binaryPropertyValue = (gctINT32_PTR)
                               ((gctUINT8 *) binaryPropertyValue + binarySize);
                bytes       -= binarySize;

            }

            /* Point to next kernel function. */
            binaryKernelFunc = (gcBINARY_KERNEL_FUNCTION) binaryPropertyValue;
        }
    }

    /************************************************************************/
    /*      private/constant memory size and optimization option            */
    /************************************************************************/

    /* Get the private memory size. */
    privateMemorySize = (gctUINT16 *) binaryKernelFunc;

    if (bytes < sizeof(gctUINT16))
    {
        /* Invalid private memory size. */
        gcmFATAL("gcSHADER_Load: Invalid private memory size");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    Shader->privateMemorySize = *privateMemorySize;

    /* Point to the constant memory size. */
    constantMemorySize  = (gctUINT16 *) (privateMemorySize + 1);
    bytes -= sizeof(gctUINT16);

    if (bytes < sizeof(gctUINT16))
    {
        /* Invalid constant memory size. */
        gcmFATAL("gcSHADER_Load: Invalid constant memory size");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    Shader->constantMemorySize = *constantMemorySize;
    bytes -= sizeof(gctUINT16);

    if (Shader->constantMemorySize > 0) {

        if (bytes < Shader->constantMemorySize)
        {
            /* Invalid constant memory buffer. */
            gcmFATAL("gcSHADER_Load: Invalid constant memory buffer");
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
            return gcvSTATUS_INVALID_DATA;
        }

        status = gcoOS_Allocate(gcvNULL,
                                Shader->constantMemorySize,
                                &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Roll back. */
            Shader->constantMemorySize = 0;

            /* Error. */
            gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
            gcmFOOTER();
            return status;
        }

        Shader->constantMemoryBuffer = pointer;
        pointer = (gctUINT16 *) (constantMemorySize + 1);
        gcmVERIFY_OK(gcoOS_MemCopy(Shader->constantMemoryBuffer, pointer, Shader->constantMemorySize));
        constantMemorySize = (gctUINT16 *) ((gctUINT) constantMemorySize + Shader->constantMemorySize);
        bytes -= Shader->constantMemorySize;
    }

    /* Point to the optimization option. */
    optimizationOption  = (gctUINT *) (constantMemorySize + 1);

    if (bytes < sizeof(gctUINT))
    {
        /* Invalid optimization option. */
        gcmFATAL("gcSHADER_Load: Invalid optimization option");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    Shader->optimizationOption = *optimizationOption;

    /* Point to the code count. */
    count   = (gctUINT16 *) (optimizationOption + 1);
    bytes -= sizeof(gctUINT);

      /************************************************************************/
     /*                                                                 code */
    /************************************************************************/

    /* Get the code count. */
    count = (gctUINT16 *) count;

    if ( (bytes < sizeof(gctUINT16)) || (*count <= 0))
    {
        /* Invalid code count. */
        gcmFATAL("gcSHADER_Load: Invalid codeCount");
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Point to the code. */
    code   = (gcSL_INSTRUCTION) (count + 1);
    bytes -= sizeof(gctUINT16);

    /* Save the code count. */
    Shader->lastInstruction = Shader->codeCount = *count;

    /* Compute the number of bytes. */
    binarySize = Shader->codeCount * sizeof(struct _gcSL_INSTRUCTION);

    if (bytes < binarySize)
    {
        /* Roll back. */
        Shader->lastInstruction = Shader->codeCount = 0;

        /* Invalid code count. */
        gcmFATAL("gcSHADER_Load: Invalid codeCount");
        gcmFOOTER();
        return gcvSTATUS_INVALID_DATA;
    }

    /* Allocate memory for the code inside the gcSHADER object. */
    status = gcoOS_Allocate(gcvNULL,
                           binarySize,
                           &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Roll back. */
        Shader->lastInstruction = Shader->codeCount = 0;

        /* Error. */
        gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d", status);
        gcmFOOTER();
        return status;
    }

    Shader->code = pointer;

    /* Copy the code into the gcSHADER object. */
    gcmVERIFY_OK(gcoOS_MemCopy(Shader->code, code, binarySize));

    if (shaderVersion > gcdSL_SHADER_BINARY_BEFORE_LTC_FILE_VERSION)
    {
#if GC_ENABLE_LOADTIME_OPT
        /* Loadtime Optimization related data */

        /* LTC uniform index */
        curPos = (gctUINT8 *)code + binarySize;
        bytes -= binarySize;  /* remaining bytes */
        Shader->ltcUniformCount = *(gctINT *)curPos;

        curPos += sizeof(Shader->ltcUniformCount);
        bytes -= sizeof(Shader->ltcUniformCount);  /* remaining bytes */
        Shader->ltcUniformBegin = *(gctUINT *) curPos;

        curPos += sizeof(Shader->ltcUniformBegin);
        bytes -= sizeof(Shader->ltcUniformBegin);
        Shader->ltcInstructionCount = *(gctUINT *) curPos;

        curPos += sizeof(Shader->ltcInstructionCount); /* points to ltcCodeUniformIndex */
        bytes -= sizeof(Shader->ltcInstructionCount);  /* remaining bytes */
        binarySize = sizeof(*Shader->ltcCodeUniformIndex) * Shader->ltcInstructionCount;

        if (binarySize > 0)
        {
            /* Allocate memory for  ltcCodeUniformIndex inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   binarySize,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->ltcUniformCount = 0;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
                gcmFOOTER();
                return status;
            }

            gcmVERIFY_OK(gcoOS_MemCopy(pointer, curPos, binarySize));
            Shader->ltcCodeUniformIndex = (gctINT *)pointer;
        }
        else
        {
            Shader->ltcCodeUniformIndex = gcvNULL;
        }

        curPos += binarySize;                           /* points to ltcExpressions */
        bytes -= binarySize;                            /* remaining bytes */
        binarySize = sizeof(*Shader->ltcExpressions) * Shader->ltcInstructionCount;
        if (binarySize > 0)
        {
            /* Allocate memory for  ltcExpressions inside the gcSHADER object. */
            status = gcoOS_Allocate(gcvNULL,
                                   binarySize,
                                   &pointer);

            if (gcmIS_ERROR(status))
            {
                /* Roll back. */
                Shader->ltcUniformCount = 0;

                /* Error. */
                gcmFATAL("gcSHADER_Load: gcoOS_Allocate failed status=%d(%s)", status, gcoOS_DebugStatus2Name(status));
                gcmFOOTER();
                return status;
            }

            gcmVERIFY_OK(gcoOS_MemCopy(pointer, curPos, binarySize));
            Shader->ltcExpressions = (gcSL_INSTRUCTION)pointer;
        }
#endif /* GC_ENABLE_LOADTIME_OPT */
    }
    else
    {
#if GC_ENABLE_LOADTIME_OPT
        Shader->ltcUniformCount = 0;
        Shader->ltcExpressions = gcvNULL;
        Shader->ltcUniformBegin = 0;
        Shader->ltcInstructionCount = 0;
        Shader->ltcCodeUniformIndex = gcvNULL;
#endif /* GC_ENABLE_LOADTIME_OPT */
    } /* if */

    if (bytes > binarySize)
    {
        /* Error. */
        gcmFATAL("gcSHADER_Load: %u extraneous bytes in the binary file.",
              bytes - binarySize);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_Save
**
**  Save a gcSHADER object including header information to a binary buffer.
**  SHADER binary header format: -
**     Word 1:  ('S' 'H' 'D' 'R') : signature
**     Word 2:  ('\od' '\od' '\od' '\od') od = octal digits; binary file version
**     Word 3:  ('E' 'S' '\0' '\0') | gcSHADER_TYPE_VERTEX   or
**              ('E' 'S' '\0' '\0') | gcSHADER_TYPE_FRAGMENT or
**              ('C' 'L' '\0' '\0') | gcSHADER_TYPE_CL
**     Word 4: ('\od' '\od' '\od' '\od') od = octal digits; compiler version
**     Word 5: ('\1' '\0' '\0' '\0') gcSL version
**     Word 6: size of shader binary file in bytes excluding this header
**
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctPOINTER Buffer
**          Pointer to a binary buffer to be used as storage for the gcSHADER
**          object.  If 'Buffer' is gcvNULL, the gcSHADER object will not be saved,
**          but the number of bytes required to hold the binary output for the
**          gcSHADER object will be returned.
**
**      gctSIZE_T * BufferSize
**          Pointer to a variable holding the number of bytes allocated in
**          'Buffer'.  Only valid if 'Buffer' is not gcvNULL.
**
**  OUTPUT:
**
**      gctSIZE_T * BufferSize
**          Pointer to a variable receiving the number of bytes required to hold
**          the binary form of the gcSHADER object.
*/
gceSTATUS
gcSHADER_SaveEx(
    IN gcSHADER Shader,
    IN gctPOINTER Buffer,
    IN OUT gctSIZE_T * BufferSize
    )
{
    gctSIZE_T bytes, i, j, nameLength;
    gctUINT8 * buffer;
    gctSIZE_T outputCount = 0;
    gctSIZE_T attributeCount = 0;
    gctSIZE_T uniformCount = 0;
    gctSIZE_T variableCount = 0;
    gctSIZE_T functionCount = 0;
    gctSIZE_T kernelFunctionCount = 0;

    gcmHEADER_ARG("Shader=0x%x Buffer=0x%x BufferSize=0x%x", Shader, Buffer, BufferSize);

    if (Shader == gcvNULL)
    {
    /* Return required number of bytes. */
        *BufferSize = 0;
        gcmFOOTER_ARG("*BufferSize=%d", *BufferSize);
        return gcvSTATUS_OK;
    }

      /************************************************************************/
     /*                                       Compute size of binary buffer. */
    /************************************************************************/

    /* File Header. */
    bytes = _gcdShaderBinaryHeaderSize;

    /* Attributes. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->attributeCount; i++)
    {
        if (Shader->attributes[i] == gcvNULL) continue;

        attributeCount ++;

        nameLength = Shader->attributes[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_ATTRIBUTE, name) + gcmALIGN(nameLength, 2);
    }

    /* Uniforms. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->uniformCount; i++)
    {
        if (Shader->uniforms[i] == gcvNULL) continue;

        uniformCount ++;

        nameLength = Shader->uniforms[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_UNIFORM_EX, name) + gcmALIGN(nameLength, 2);
    }

    /* Outputs. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->outputCount; i++)
    {
        if (Shader->outputs[i] == gcvNULL) continue;

        outputCount ++;

        nameLength = Shader->outputs[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_OUTPUT, name) + gcmALIGN(nameLength, 2);
    }

    /* Global Variables. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->variableCount; i++)
    {
        if (Shader->variables[i] == gcvNULL) continue;

        variableCount ++;

        nameLength = Shader->variables[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + gcmALIGN(nameLength, 2);
    }

    /* Functions. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->functionCount; i++)
    {
        if (Shader->functions[i] == gcvNULL) continue;

        functionCount ++;

        nameLength = Shader->functions[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_FUNCTION, name) + gcmALIGN(nameLength, 2);

        /* Function arguments */
        bytes += Shader->functions[i]->argumentCount * sizeof(struct _gcBINARY_ARGUMENT);

        /* Function variables */
        for (j = 0; j < Shader->functions[i]->variableCount; j++)
        {
            if (Shader->functions[i]->variables[j] == gcvNULL) continue;

            nameLength = Shader->functions[i]->variables[j]->nameLength;

            if ((long) nameLength < 0)
            {
                nameLength = 0;
            }

            bytes += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + gcmALIGN(nameLength, 2);
        }
    }

    /* Max kernel function arguments. */
    bytes += sizeof(gctUINT16);

    /* Kernel functions. */
    bytes += sizeof(gctUINT16);

    for (i = 0; i < Shader->kernelFunctionCount; i++)
    {
        if (Shader->kernelFunctions[i] == gcvNULL) continue;

        kernelFunctionCount ++;

        nameLength = Shader->kernelFunctions[i]->nameLength;

        if ((long) nameLength < 0)
        {
            nameLength = 0;
        }

        bytes += gcmOFFSETOF(_gcBINARY_KERNEL_FUNCTION, name) + gcmALIGN(nameLength, 2);

        /* Kernel function arguments */
        bytes += Shader->kernelFunctions[i]->argumentCount * sizeof(struct _gcBINARY_ARGUMENT);

        /* Kernel function uniform arguments. */
        for (j = 0; j < Shader->kernelFunctions[i]->uniformArgumentCount; j++)
        {
            if (Shader->kernelFunctions[i]->uniformArguments[j] == gcvNULL) continue;

            nameLength = Shader->kernelFunctions[i]->uniformArguments[j]->nameLength;

            if ((long) nameLength < 0)
            {
                nameLength = 0;
            }

            bytes += gcmOFFSETOF(_gcBINARY_UNIFORM_EX, name) + gcmALIGN(nameLength, 2);
        }

        /* Kernel function image samplers */
        bytes += Shader->kernelFunctions[i]->imageSamplerCount * sizeof(struct _gcBINARY_IMAGE_SAMPLER);

        /* Kernel function variables */
        for (j = 0; j < Shader->kernelFunctions[i]->variableCount; j++)
        {
            if (Shader->kernelFunctions[i]->variables[j] == gcvNULL) continue;

            nameLength = Shader->kernelFunctions[i]->variables[j]->nameLength;

            if ((long) nameLength < 0)
            {
                nameLength = 0;
            }

            bytes += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + gcmALIGN(nameLength, 2);
        }

        /* Kernel function properties */
        bytes += Shader->kernelFunctions[i]->propertyCount * sizeof(struct _gcBINARY_KERNEL_FUNCTION_PROPERTY);

        /* Kernel function property values */
        bytes += Shader->kernelFunctions[i]->propertyValueCount * sizeof(gctINT32);

    }

    /* Private memory size. */
    bytes += sizeof(gctUINT16);

    /* Constant memory size. */
    bytes += sizeof(gctUINT16);

    /* Constant memory buffer. */
    bytes += Shader->constantMemorySize;

    /* Optimization option. */
    bytes += sizeof(gctUINT);

    /* Code. */
    bytes += sizeof(gctUINT16);
    bytes += Shader->codeCount * sizeof(struct _gcSL_INSTRUCTION);

#if GC_ENABLE_LOADTIME_OPT
    /* Loadtime Optimization related data */
    bytes += sizeof(Shader->ltcUniformCount) +
             sizeof(Shader->ltcUniformBegin) +
             sizeof(*Shader->ltcCodeUniformIndex) * Shader->ltcInstructionCount +
             sizeof(Shader->ltcInstructionCount) +
             sizeof(*Shader->ltcExpressions) * Shader->ltcInstructionCount;
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Return required number of bytes if Buffer is gcvNULL. */
    if (Buffer == gcvNULL)
    {
        *BufferSize = bytes;
        gcmFOOTER_ARG("*BufferSize=%d", *BufferSize);
        return gcvSTATUS_OK;
    }

    /* Make sure the buffer is large enough. */
    if (*BufferSize < bytes)
    {
        *BufferSize = bytes;
        gcmFOOTER_ARG("*BufferSize=%d status=%d",
                        *BufferSize, gcvSTATUS_BUFFER_TOO_SMALL);
        return gcvSTATUS_BUFFER_TOO_SMALL;
    }

    /* Store number of bytes returned. */
    *BufferSize = bytes;

      /************************************************************************/
     /*                                              Fill the binary buffer. */
    /************************************************************************/

    buffer = Buffer;

    /* Header */
    /* Word 1: signature 'S' 'H' 'D' 'R' */
    *(gceOBJECT_TYPE *) buffer = gcvOBJ_SHADER;
    buffer += sizeof(gceOBJECT_TYPE);

    /* Word 2: binary file version # */
    *(gctUINT32 *) buffer = gcdSL_SHADER_BINARY_FILE_VERSION;
    buffer += sizeof(gctUINT32);

    /* Word 3: language type and shader type */
    *(gctUINT32 *) buffer = Shader->compilerVersion[0];
    buffer += sizeof(gctUINT32);

    /* Word 4: compiler version */
    *(gctUINT32 *) buffer = Shader->compilerVersion[1];
    buffer += sizeof(gctUINT32);

    /* Word 5: gcSL version */
    *(gctUINT32 *) buffer = gcdSL_IR_VERSION;
    buffer += sizeof(gctUINT32);

    /* Word 6: size of binary excluding header */
    *(gctUINT32 *) buffer = bytes - _gcdShaderBinaryHeaderSize;
    buffer += sizeof(gctUINT32);

    /* Attribute count. */
    *(gctUINT16 *) buffer = (gctUINT16) attributeCount;
    buffer += sizeof(gctUINT16);

    /* Attributes. */
    for (i = 0; i < Shader->attributeCount; i++)
    {
        gcATTRIBUTE attribute;
        gcBINARY_ATTRIBUTE binary;

        if (Shader->attributes[i] == gcvNULL) continue;

        /* Point to source and binary attributes. */
        attribute = Shader->attributes[i];
        binary    = (gcBINARY_ATTRIBUTE) buffer;

        /* Fill in binary attribute. */
        binary->type       = attribute->type;
        binary->isTexture  = (gctINT8)  attribute->isTexture;
        binary->arraySize  = (gctINT16) attribute->arraySize;
        binary->nameLength = (gctINT16) attribute->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, attribute->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_ATTRIBUTE, name) + bytes;
    }

    /* Uniform count. */
    *(gctUINT16 *) buffer = (gctUINT16) uniformCount;
    buffer += sizeof(gctUINT16);

    /* Uniforms. */
    for (i = 0; i < Shader->uniformCount; i++)
    {
        gcUNIFORM uniform;
        gcBINARY_UNIFORM_EX binary;

        if (Shader->uniforms[i] == gcvNULL) continue;

        /* Point to source and binary uniforms. */
        uniform = Shader->uniforms[i];
        binary  = (gcBINARY_UNIFORM_EX) buffer;

        /* Fill in binary uniform. */
        binary->type       = uniform->type;
        binary->index      = uniform->index;
        binary->arraySize  = (gctINT16) uniform->arraySize;
        binary->flags      = (gctUINT16) uniform->flags;
        binary->format     = (gctUINT16) uniform->format;
        binary->isPointer  = (gctINT16) uniform->isPointer;
        binary->nameLength = (gctINT16) uniform->nameLength;
#if GC_ENABLE_LOADTIME_OPT
        binary->flags            = (gceUNIFORM_FLAGS)uniform->flags;
        binary->glUniformIndex   = uniform->glUniformIndex;
#endif /* GC_ENABLE_LOADTIME_OPT */

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, uniform->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_UNIFORM_EX, name) + bytes;
    }

    /* Output count. */
    *(gctUINT16 *) buffer = (gctUINT16) outputCount;
    buffer += sizeof(gctUINT16);

    /* Outputs. */
    for (i = 0; i < Shader->outputCount; i++)
    {
        gcOUTPUT output;
        gcBINARY_OUTPUT binary;

        if (Shader->outputs[i] == gcvNULL) continue;

        /* Point to source and binary outputs. */
        output = Shader->outputs[i];
        binary = (gcBINARY_OUTPUT) buffer;

        /* Fill in binary output. */
        binary->type       = output->type;
        binary->arraySize  = (gctINT8) output->arraySize;
        binary->tempIndex  = output->tempIndex;
        binary->nameLength = (gctINT16) output->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, output->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_OUTPUT, name) + bytes;
    }

    /* Global variable count. */
    *(gctUINT16 *) buffer = (gctUINT16) variableCount;
    buffer += sizeof(gctUINT16);

    /* Global variables. */
    for (i = 0; i < Shader->variableCount; i++)
    {
        gcVARIABLE variable;
        gcBINARY_VARIABLE binary;

        if (Shader->variables[i] == gcvNULL) continue;

        /* Point to source and binary variables. */
        variable = Shader->variables[i];
        binary = (gcBINARY_VARIABLE) buffer;

        /* Fill in binary variable. */
        binary->varCategory = variable->varCategory;
        binary->firstChild = variable->firstChild;
        binary->nextSibling = variable->nextSibling;
        binary->prevSibling = variable->prevSibling;
        binary->parent = variable->parent;

        if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
            binary->u.type = (gcSHADER_TYPE) variable->u.type;
        else
            binary->u.numStructureElement = (gctUINT8)variable->u.numStructureElement;

        binary->arraySize  = (gctINT8) variable->arraySize;
        binary->tempIndex  = variable->tempIndex;
        binary->nameLength = (gctINT16) variable->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, variable->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + bytes;
    }

    /* Function count. */
    *(gctUINT16 *) buffer = (gctUINT16) functionCount;
    buffer += sizeof(gctUINT16);

    /* Functions. */
    for (i = 0; i < Shader->functionCount; i++)
    {
        gcFUNCTION function;
        gcBINARY_FUNCTION binary;

        if (Shader->functions[i] == gcvNULL) continue;

        /* Point to source and binary functions. */
        function = Shader->functions[i];
        binary = (gcBINARY_FUNCTION) buffer;

        /* Fill in binary function. */
        binary->argumentCount       = (gctINT16) function->argumentCount;
        binary->variableCount       = (gctINT16) function->variableCount;
        binary->codeStart           = (gctUINT16) function->codeStart;
        binary->codeCount           = (gctUINT16) function->codeCount;
        binary->label               = function->label;
        binary->nameLength          = (gctINT16) function->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, function->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_FUNCTION, name) + bytes;

        /* Function arguments */
        for (j = 0; j < function->argumentCount; j++)
        {
            gcsFUNCTION_ARGUMENT *argument;
            gcBINARY_ARGUMENT binary;

            /* Point to source and binary arguments. */
            argument = &function->arguments[j];
            binary = (gcBINARY_ARGUMENT) buffer;

            /* Fill in binary argument. */
            binary->index      = argument->index;
            binary->enable     = argument->enable;
            binary->qualifier  = argument->qualifier;

            /* Adjust buffer pointer. */
            buffer += sizeof(struct _gcBINARY_ARGUMENT);
        }

        /* Function variables */
        for (j = 0; j < function->variableCount; j++)
        {
            gcVARIABLE variable;
            gcBINARY_VARIABLE binary;

            if (function->variables[j] == gcvNULL) continue;

            /* Point to source and binary variables. */
            variable = function->variables[j];
            binary = (gcBINARY_VARIABLE) buffer;

            /* Fill in binary variable. */
            binary->varCategory = variable->varCategory;
            binary->firstChild = variable->firstChild;
            binary->nextSibling = variable->nextSibling;
            binary->prevSibling = variable->prevSibling;
            binary->parent = variable->parent;

            if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                binary->u.type = (gcSHADER_TYPE) variable->u.type;
            else
                binary->u.numStructureElement = (gctUINT8)variable->u.numStructureElement;

            binary->arraySize  = (gctINT8) variable->arraySize;
            binary->tempIndex  = variable->tempIndex;
            binary->nameLength = (gctINT16) variable->nameLength;

            if (binary->nameLength > 0)
            {
                /* Compute number of bytes to copy. */
                bytes = gcmALIGN(binary->nameLength, 2);

                /* Copy name. */
                gcmVERIFY_OK(gcoOS_MemCopy(binary->name, variable->name, bytes));
            }
            else
            {
                bytes = 0;
            }

            /* Adjust buffer pointer. */
            buffer += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + bytes;
        }
    }

    /* Max kernel function args. */
    *(gctUINT16 *) buffer = (gctUINT16) Shader->maxKernelFunctionArgs;
    buffer += sizeof(gctUINT16);

    /* Kernel function count. */
    *(gctUINT16 *) buffer = (gctUINT16) kernelFunctionCount;
    buffer += sizeof(gctUINT16);

    /* Kernel Functions. */
    for (i = 0; i < Shader->kernelFunctionCount; i++)
    {
        gcKERNEL_FUNCTION kernelFunction;
        gcBINARY_KERNEL_FUNCTION binary;

        if (Shader->kernelFunctions[i] == gcvNULL) continue;

        /* Point to source and binary kernel functions. */
        kernelFunction = Shader->kernelFunctions[i];
        binary = (gcBINARY_KERNEL_FUNCTION) buffer;

        /* Fill in binary function. */
        binary->argumentCount       = (gctINT16) kernelFunction->argumentCount;
        binary->label               = kernelFunction->label;
        gcoOS_MemCopy(&binary->localMemorySize, &kernelFunction->localMemorySize, sizeof(gctUINT32));
        binary->uniformArgumentCount = (gctINT16) kernelFunction->uniformArgumentCount;
        binary->samplerIndex        = (gctINT16) kernelFunction->samplerIndex;
        binary->imageSamplerCount   = (gctINT16) kernelFunction->imageSamplerCount;
        binary->variableCount       = (gctINT16) kernelFunction->variableCount;
        binary->propertyCount       = (gctINT16) kernelFunction->propertyCount;
        binary->propertyValueCount  = (gctINT16) kernelFunction->propertyValueCount;
        binary->codeStart           = (gctUINT16) kernelFunction->codeStart;
        binary->codeCount           = (gctUINT16) kernelFunction->codeCount;
        binary->codeEnd             = (gctUINT16) kernelFunction->codeEnd;
        binary->isMain              = (gctUINT16) kernelFunction->isMain;
        binary->nameLength          = (gctINT16) kernelFunction->nameLength;

        if (binary->nameLength > 0)
        {
            /* Compute number of bytes to copy. */
            bytes = gcmALIGN(binary->nameLength, 2);

            /* Copy name. */
            gcmVERIFY_OK(gcoOS_MemCopy(binary->name, kernelFunction->name, bytes));
        }
        else
        {
            bytes = 0;
        }

        /* Adjust buffer pointer. */
        buffer += gcmOFFSETOF(_gcBINARY_KERNEL_FUNCTION, name) + bytes;

        /* Kernel function arguments */
        for (j = 0; j < kernelFunction->argumentCount; j++)
        {
            gcsFUNCTION_ARGUMENT *argument;
            gcBINARY_ARGUMENT binary;

            /* Point to source and binary arguments. */
            argument = &kernelFunction->arguments[j];
            binary = (gcBINARY_ARGUMENT) buffer;

            /* Fill in binary argument. */
            binary->index      = argument->index;
            binary->enable     = argument->enable;
            binary->qualifier  = argument->qualifier;

            /* Adjust buffer pointer. */
            buffer += sizeof(struct _gcBINARY_ARGUMENT);
        }

        /* Kernel function uniform arguments */
        for (j = 0; j < kernelFunction->uniformArgumentCount; j++)
        {
            gcUNIFORM uniform;
            gcBINARY_UNIFORM_EX binary;

            if (kernelFunction->uniformArguments[j] == gcvNULL) continue;

            /* Point to source and binary uniforms. */
            uniform = kernelFunction->uniformArguments[j];
            binary = (gcBINARY_UNIFORM_EX) buffer;

            /* Fill in binary uniform. */
            binary->type       = uniform->type;
            binary->index      = uniform->index;
            binary->arraySize  = (gctINT16) uniform->arraySize;
            binary->flags      = (gctUINT16) uniform->flags;
            binary->format     = (gctUINT16) uniform->format;
            binary->isPointer  = (gctINT16) uniform->isPointer;
            binary->nameLength = (gctINT16) uniform->nameLength;
#if GC_ENABLE_LOADTIME_OPT
            binary->flags            = (gceUNIFORM_FLAGS)uniform->flags;
            binary->glUniformIndex   = uniform->glUniformIndex;
#endif /* GC_ENABLE_LOADTIME_OPT */

            if (binary->nameLength > 0)
            {
                /* Compute number of bytes to copy. */
                bytes = gcmALIGN(binary->nameLength, 2);

                /* Copy name. */
                gcmVERIFY_OK(gcoOS_MemCopy(binary->name, uniform->name, bytes));
            }
            else
            {
                bytes = 0;
            }

            /* Adjust buffer pointer. */
            buffer += gcmOFFSETOF(_gcBINARY_UNIFORM_EX, name) + bytes;
        }

        /* Kernel function image samplers */
        for (j = 0; j < kernelFunction->imageSamplerCount; j++)
        {
            gcsIMAGE_SAMPLER_PTR imageSampler;
            gcBINARY_IMAGE_SAMPLER binary;

            /* Point to source and binary imageSamplers. */
            imageSampler = &kernelFunction->imageSamplers[j];
            binary = (gcBINARY_IMAGE_SAMPLER) buffer;

            /* Fill in binary imageSampler. */
            binary->isConstantSamplerType = (gctUINT8) imageSampler->isConstantSamplerType;
            binary->imageNum              = imageSampler->imageNum;
            binary->samplerType           = imageSampler->samplerType;

            /* Adjust buffer pointer. */
            buffer += sizeof(struct _gcBINARY_IMAGE_SAMPLER);
        }

        /* Kernel function variables */
        for (j = 0; j < kernelFunction->variableCount; j++)
        {
            gcVARIABLE variable;
            gcBINARY_VARIABLE binary;

            if (kernelFunction->variables[j] == gcvNULL) continue;

            /* Point to source and binary variables. */
            variable = kernelFunction->variables[j];
            binary = (gcBINARY_VARIABLE) buffer;

            /* Fill in binary variable. */
            binary->varCategory = variable->varCategory;
            binary->firstChild = variable->firstChild;
            binary->nextSibling = variable->nextSibling;
            binary->prevSibling = variable->prevSibling;
            binary->parent = variable->parent;

            if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
                binary->u.type = (gcSHADER_TYPE) variable->u.type;
            else
                binary->u.numStructureElement = (gctUINT8)variable->u.numStructureElement;

            binary->arraySize  = (gctINT8) variable->arraySize;
            binary->tempIndex  = variable->tempIndex;
            binary->nameLength = (gctINT16) variable->nameLength;

            if (binary->nameLength > 0)
            {
                /* Compute number of bytes to copy. */
                bytes = gcmALIGN(binary->nameLength, 2);

                /* Copy name. */
                gcmVERIFY_OK(gcoOS_MemCopy(binary->name, variable->name, bytes));
            }
            else
            {
                bytes = 0;
            }

            /* Adjust buffer pointer. */
            buffer += gcmOFFSETOF(_gcBINARY_VARIABLE, name) + bytes;
        }

        /* Kernel function properties */
        for (j = 0; j < kernelFunction->propertyCount; j++)
        {
            gcsKERNEL_FUNCTION_PROPERTY_PTR property;
            gcBINARY_KERNEL_FUNCTION_PROPERTY binary;

            /* Point to source and binary properties. */
            property = &kernelFunction->properties[j];
            binary = (gcBINARY_KERNEL_FUNCTION_PROPERTY) buffer;

            /* Fill in binary property. */
            gcoOS_MemCopy(&binary->propertyType, &property->propertyType, sizeof(gctUINT32));
            gcoOS_MemCopy(&binary->propertySize, &property->propertySize, sizeof(gctUINT32));

            /* Adjust buffer pointer. */
            buffer += sizeof(struct _gcBINARY_KERNEL_FUNCTION_PROPERTY);
        }

        /* Kernel function property values */
        for (j = 0; j < kernelFunction->propertyValueCount; j++)
        {
            gcoOS_MemCopy(&((gctINT_PTR)buffer)[j], &kernelFunction->propertyValues[j], sizeof(gctUINT32));

        }
        /* Adjust buffer pointer. */
        buffer += sizeof(gctINT32) * kernelFunction->propertyValueCount;
    }

    /* Private memory size. */
    *(gctUINT16 *) buffer = (gctUINT16) Shader->privateMemorySize;
    buffer += sizeof(gctUINT16);

    /* Constant memory size. */
    *(gctUINT16 *) buffer = (gctUINT16) Shader->constantMemorySize;
    buffer += sizeof(gctUINT16);

    /* Constant memory buffer. */
    if (Shader->constantMemoryBuffer != gcvNULL) {
        gcmVERIFY_OK(gcoOS_MemCopy(buffer, Shader->constantMemoryBuffer, Shader->constantMemorySize));
        buffer += Shader->constantMemorySize;
    }

    /* Optimization option. */
    *(gctUINT *) buffer = (gctUINT) Shader->optimizationOption;
    buffer += sizeof(gctUINT);

    /* Code count. */
    *(gctUINT16 *) buffer = (gctUINT16) Shader->codeCount;
    buffer += sizeof(gctUINT16);

    /* Copy the code. */
    bytes = Shader->codeCount * sizeof(struct _gcSL_INSTRUCTION);
    if (bytes > 0) gcmVERIFY_OK(gcoOS_MemCopy(buffer, Shader->code, bytes));
    buffer += bytes;

#if GC_ENABLE_LOADTIME_OPT
    /* Loadtime Optimization related data */

    /* LTC uniform index */
    *(gctINT *) buffer = Shader->ltcUniformCount;
    buffer += sizeof(Shader->ltcUniformCount);

    *(gctUINT *) buffer = Shader->ltcUniformBegin;
    buffer += sizeof(Shader->ltcUniformBegin);

    /* LTC expressions */
    *(gctUINT *) buffer = Shader->ltcInstructionCount;
    buffer += sizeof(Shader->ltcInstructionCount);

    bytes = sizeof(*Shader->ltcCodeUniformIndex) * Shader->ltcInstructionCount;
    if (bytes > 0) gcmVERIFY_OK(gcoOS_MemCopy(buffer, Shader->ltcCodeUniformIndex, bytes));
    buffer += bytes;

    bytes = sizeof(*Shader->ltcExpressions) * Shader->ltcInstructionCount;
    if (bytes > 0) gcmVERIFY_OK(gcoOS_MemCopy(buffer, Shader->ltcExpressions, bytes));
    buffer += bytes;
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Success. */
    gcmFOOTER_ARG("*BufferSize=%lu", *BufferSize);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_ReallocateAttributes
**
**  Reallocate an array of pointers to gcATTRIBUTE objects.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcSHADER_ReallocateAttributes(
    IN gcSHADER Shader,
    IN gctSIZE_T Count
    )
{
    gcATTRIBUTE * attributes;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Count=%lu", Shader, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Count < Shader->attributeCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == Shader->attributeArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcATTRIBUTE) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    attributes = pointer;

    if (Shader->attributes != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(attributes,
                                   Shader->attributes,
                                   gcmSIZEOF(gcATTRIBUTE)
                                   * Shader->attributeCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->attributes));
    }

    /* Set new gcATTRIBTE object pointer. */
    Shader->attributeArrayCount = Count;
    Shader->attributes          = attributes;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddAttribute
**
**  Add an attribute to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctCONST_STRING Name
**          Name of the attribute to add.
**
**      gcSHADER_TYPE Type
**          Type of the attribute to add.
**
**      gctSIZE_T Length
**          Array length of the attribute to add.  'Length' must be at least 1.
**
**      gctBOOL IsTexture
**          gcvTRUE if the attribute is used as a texture coordinate, gcvFALSE if not.
**
**  OUTPUT:
**
**      gcATTRIBUTE * Attribute
**          Pointer to a variable receiving the gcATTRIBUTE object pointer.
*/
gceSTATUS
gcSHADER_AddAttribute(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN gctBOOL IsTexture,
    OUT gcATTRIBUTE * Attribute
    )
{
    gctSIZE_T nameLength, bytes;
    gcATTRIBUTE attribute;
    gceSTATUS status;
    gctBOOL copyName;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%lu IsTexture=%d",
                  Shader, Name, Type, Length, IsTexture);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Check array count. */
    if (Shader->attributeArrayCount <= Shader->attributeCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcSHADER_ReallocateAttributes(Shader, Shader->attributeCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Get the length of the name. */
    gcmVERIFY_OK(gcoOS_StrLen(Name, &nameLength));

    if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#Position")))
    {
        nameLength = gcSL_POSITION;
        bytes      = gcmOFFSETOF(_gcATTRIBUTE, name);
        copyName   = gcvFALSE;
    }
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#FrontFacing")))
    {
        nameLength = gcSL_FRONT_FACING;
        bytes      = gcmOFFSETOF(_gcATTRIBUTE, name);
        copyName   = gcvFALSE;
    }
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#PointCoord")))
    {
        nameLength = gcSL_POINT_COORD;
        bytes      = gcmOFFSETOF(_gcATTRIBUTE, name);
        copyName   = gcvFALSE;
    }
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#Position.w")))
    {
        nameLength = gcSL_POSITION_W;
        bytes      = gcmOFFSETOF(_gcATTRIBUTE, name);
        copyName   = gcvFALSE;
    }
    else
    {
        /* Compute the number of bytes required for the gcATTRIBUTE object. */
        bytes    = gcmOFFSETOF(_gcATTRIBUTE, name) + nameLength + 1;
        copyName = gcvTRUE;
    }

    /* Allocate the gcATTRIBUTE object. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    attribute = pointer;

    /* Initialize the gcATTRIBUTE object. */
    attribute->object.type = gcvOBJ_ATTRIBUTE;
    attribute->index       = (gctUINT16) Shader->attributeCount;
    attribute->type        = Type;
    attribute->arraySize   = Length;
    attribute->isTexture   = IsTexture;
    attribute->enabled     = gcvTRUE;
    attribute->nameLength  = nameLength;

    if (copyName)
    {
        /* Copy the attribute name. */
        gcmVERIFY_OK(gcoOS_MemCopy(attribute->name, Name, nameLength + 1));
    }

    /* Set new gcATTRIBTE object pointer. */
    Shader->attributes[Shader->attributeCount++] = attribute;

    if (Attribute != gcvNULL)
    {
        /* Return the gcATTRIBUTE object pointer. */
        *Attribute = attribute;
    }

    gcmFOOTER_ARG("*Attribute=0x%x", gcmOPT_POINTER(Attribute));
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetAttributeCount
**
**  Get the number of attributes for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctSIZE_T * Count
**          Pointer to a variable receiving the number of attributes.
*/
gceSTATUS
gcSHADER_GetAttributeCount(
    IN gcSHADER Shader,
    OUT gctSIZE_T * Count
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    /* Return attribute count. */
    *Count = Shader->attributeCount;

    /* Success. */
    gcmFOOTER_ARG("*Count=%lu", *Count);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetAttribute
**
**  Get the gcATTRIBUTE object poniter for an indexed attribute for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT Index
**          Index of attribute to retreive the name for.
**
**  OUTPUT:
**
**      gcATTRIBUTE * Attribute
**          Pointer to a variable receiving the gcATTRIBUTE object pointer.
*/
gceSTATUS
gcSHADER_GetAttribute(
    IN gcSHADER Shader,
    IN gctUINT Index,
    OUT gcATTRIBUTE * Attribute
    )
{
    gcmHEADER_ARG("Shader=0x%x Index=%u", Shader, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Index < Shader->attributeCount);
    gcmDEBUG_VERIFY_ARGUMENT(Attribute != gcvNULL);

    /* Return the gcATTRIBUTE object pointer. */
    *Attribute = Shader->attributes[Index];

    /* Success. */
    gcmFOOTER_ARG("*Attribute=0x%x", *Attribute);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_ReallocateUniforms
**
**  Reallocate an array of pointers to gcUNIFORM objects.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcSHADER_ReallocateUniforms(
    IN gcSHADER Shader,
    IN gctSIZE_T Count
    )
{
    gcUNIFORM * uniforms;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Count=%lu", Shader, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Count < Shader->uniformCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == Shader->uniformArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcUNIFORM) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    uniforms = pointer;

    if (Shader->uniforms != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(uniforms,
                                   Shader->uniforms,
                                   gcmSIZEOF(gcUNIFORM)
                                   * Shader->uniformCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->uniforms));
    }

    /* Set new gcUNIFORM object pointer. */
    Shader->uniformArrayCount = Count;
    Shader->uniforms          = uniforms;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddUniform
**
**  Add an uniform to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctCONST_STRING Name
**          Name of the uniform to add.
**
**      gcSHADER_TYPE Type
**          Type of the uniform to add.
**
**      gctSIZE_T Length
**          Array length of the uniform to add.  'Length' must be at least 1.
**
**  OUTPUT:
**
**      gcUNIFORM * Uniform
**          Pointer to a variable receiving the gcUNIFORM object pointer.
*/
gceSTATUS
gcSHADER_AddUniform(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    OUT gcUNIFORM * Uniform
    )
{
    gctSIZE_T nameLength=0, bytes;
    gcUNIFORM uniform;
    gceSTATUS status;
    gctPOINTER pointer;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%lu",
                  Shader, Name, Type, Length);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Check array count. */
    if (Shader->uniformArrayCount <= Shader->uniformCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcSHADER_ReallocateUniforms(Shader, Shader->uniformCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Get the length of the name. */
    gcmVERIFY_OK(gcoOS_StrLen(Name, &nameLength));

    /* Compute the number of bytes required for the gcUNIFORM object. */
    bytes = gcmOFFSETOF(_gcUNIFORM, name) + nameLength + 1;

    /* Allocate the gcUNIFORM object. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    uniform = pointer;

    /* Initialize the gcUNIFORM object. */
    uniform->object.type  = gcvOBJ_UNIFORM;
    uniform->index        = (gctUINT16) (Shader->maxKernelFunctionArgs + Shader->uniformCount);
    uniform->type         = Type;
    uniform->precision    = gcSHADER_PRECISION_DEFAULT;
    uniform->arraySize    = Length;
    uniform->format       = gcSL_FLOAT;
    uniform->isPointer    = gcvFALSE;
    uniform->nameLength   = nameLength;
    uniform->physical     = -1;
    uniform->address      = ~0U;
#if GC_ENABLE_LOADTIME_OPT
    uniform->flags        = 0;
    uniform->glUniformIndex = (gctUINT16)-1;
#endif /* GC_ENABLE_LOADTIME_OPT */

    switch (Type)
    {
    case gcSHADER_SAMPLER_1D:
    case gcSHADER_SAMPLER_2D:
    case gcSHADER_SAMPLER_3D:
    case gcSHADER_SAMPLER_CUBIC:

    case gcSHADER_ISAMPLER_2D:
    case gcSHADER_ISAMPLER_3D:
    case gcSHADER_ISAMPLER_CUBIC:
    case gcSHADER_USAMPLER_2D:
    case gcSHADER_USAMPLER_3D:
    case gcSHADER_USAMPLER_CUBIC:
    case gcSHADER_SAMPLER_EXTERNAL_OES:
        uniform->physical = Shader->samplerIndex++;
        break;

    default:
        break;
    }

    /* Copy the uniform name. */
    gcmVERIFY_OK(gcoOS_MemCopy(uniform->name, Name, nameLength + 1));

    /* Set new gcUNIFORM object pointer. */
    Shader->uniforms[Shader->uniformCount++] = uniform;

    if (Uniform != gcvNULL)
    {
        /* Return the gcUNIFORM object pointer. */
        *Uniform = uniform;
    }

    gcmFOOTER_ARG("*Uniform=0x%x", gcmOPT_POINTER(Uniform));
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddUniformEx
**
**  Add an uniform to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctCONST_STRING Name
**          Name of the uniform to add.
**
**      gcSHADER_TYPE Type
**          Type of the uniform to add.
**
**      gcSHADER_PRECISION precision
**          Precision of the uniform to add.
**
**      gctSIZE_T Length
**          Array length of the uniform to add.  'Length' must be at least 1.
**
**  OUTPUT:
**
**      gcUNIFORM * Uniform
**          Pointer to a variable receiving the gcUNIFORM object pointer.
*/
gceSTATUS
gcSHADER_AddUniformEx(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gcSHADER_PRECISION precision,
    IN gctSIZE_T Length,
    OUT gcUNIFORM * Uniform
    )
{
    gctSIZE_T nameLength=0, bytes;
    gcUNIFORM uniform;
    gceSTATUS status;
    gctPOINTER pointer;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%lu",
                  Shader, Name, Type, Length);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Check array count. */
    if (Shader->uniformArrayCount <= Shader->uniformCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcSHADER_ReallocateUniforms(Shader, Shader->uniformCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Get the length of the name. */
    gcmVERIFY_OK(gcoOS_StrLen(Name, &nameLength));

    /* Compute the number of bytes required for the gcUNIFORM object. */
    bytes = gcmOFFSETOF(_gcUNIFORM, name) + nameLength + 1;

    /* Allocate the gcUNIFORM object. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    uniform = pointer;

    /* Initialize the gcUNIFORM object. */
    uniform->object.type  = gcvOBJ_UNIFORM;
    uniform->index        = (gctUINT16) (Shader->maxKernelFunctionArgs + Shader->uniformCount);
    uniform->type         = Type;
    uniform->precision    = precision;
    uniform->arraySize    = Length;
    uniform->format       = gcSL_FLOAT;
    uniform->isPointer    = gcvFALSE;
    uniform->nameLength   = nameLength;
    uniform->physical     = -1;
    uniform->address      = ~0U;
#if GC_ENABLE_LOADTIME_OPT
    uniform->flags        = 0;
    uniform->glUniformIndex = (gctUINT16)-1;
#endif /* GC_ENABLE_LOADTIME_OPT */

    switch (Type)
    {
    case gcSHADER_SAMPLER_1D:
    case gcSHADER_SAMPLER_2D:
    case gcSHADER_SAMPLER_3D:
    case gcSHADER_SAMPLER_CUBIC:

    case gcSHADER_ISAMPLER_2D:
    case gcSHADER_ISAMPLER_3D:
    case gcSHADER_ISAMPLER_CUBIC:
    case gcSHADER_USAMPLER_2D:
    case gcSHADER_USAMPLER_3D:
    case gcSHADER_USAMPLER_CUBIC:
    case gcSHADER_SAMPLER_EXTERNAL_OES:
        uniform->physical = Shader->samplerIndex++;
        break;

    default:
        break;
    }

    /* Copy the uniform name. */
    gcmVERIFY_OK(gcoOS_MemCopy(uniform->name, Name, nameLength + 1));

    /* Set new gcUNIFORM object pointer. */
    Shader->uniforms[Shader->uniformCount++] = uniform;

    if (Uniform != gcvNULL)
    {
        /* Return the gcUNIFORM object pointer. */
        *Uniform = uniform;
    }

    gcmFOOTER_ARG("*Uniform=0x%x", gcmOPT_POINTER(Uniform));
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetUniformCount
**
**  Get the number of uniforms for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctSIZE_T * Count
**          Pointer to a variable receiving the number of uniforms.
*/
gceSTATUS
gcSHADER_GetUniformCount(
    IN gcSHADER Shader,
    OUT gctSIZE_T * Count
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    if (Shader == gcvNULL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    /* Return uniform count. */
    *Count = Shader->uniformCount;

    /* Success. */
    gcmFOOTER_ARG("*Count=%lu", *Count);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetKernelFucntion
**
**  Get the gcKERNEL_FUNCTION object pointer for an indexed kernel function for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT Index
**          Index of kernel function to retreive the name for.
**
**  OUTPUT:
**
**      gcKERNEL_FUNCTION * KernelFunction
**          Pointer to a variable receiving the gcKERNEL_FUNCTION object pointer.
*/
gceSTATUS
gcSHADER_GetKernelFunction(
    IN gcSHADER Shader,
    IN gctUINT Index,
    OUT gcKERNEL_FUNCTION * KernelFunction
    )
{
    gcmHEADER_ARG("Shader=0x%x Index=%u", Shader, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Index < Shader->kernelFunctionCount);
    gcmDEBUG_VERIFY_ARGUMENT(KernelFunction != gcvNULL);

    /* Return the gcKERNEL_FUNCTION object pointer. */
    *KernelFunction = Shader->kernelFunctions[Index];

    /* Success. */
    gcmFOOTER_ARG("*KernelFunction=0x%x", *KernelFunction);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetKernelFucntionByName
**
**  Get the gcKERNEL_FUNCTION object pointer for an named kernel function for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT KernelName
**          Name of kernel function to retreive the name for.
**
**  OUTPUT:
**
**      gcKERNEL_FUNCTION * KernelFunction
**          Pointer to a variable receiving the gcKERNEL_FUNCTION object pointer.
*/
gceSTATUS
gcSHADER_GetKernelFunctionByName(
    IN gcSHADER Shader,
    IN gctSTRING KernelName,
    OUT gcKERNEL_FUNCTION * KernelFunction
    )
{
    gctSIZE_T i;

    gcmHEADER_ARG("Shader=0x%x Index=%u", Shader, KernelName);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(KernelFunction != gcvNULL);

    /* Return the gcKERNEL_FUNCTION object pointer. */
    if(Shader->kernelFunctionCount > 0)
        for (i = 0; i < Shader->kernelFunctionCount; i++) {
            if (Shader->kernelFunctions[i] == gcvNULL) continue;

            if (gcmIS_SUCCESS(gcoOS_StrCmp(Shader->kernelFunctions[i]->name, KernelName)))
            {
                *KernelFunction = Shader->kernelFunctions[i];
                break;
            }
        }

    /* Success. */
    gcmFOOTER_ARG("*KernelFunction=0x%x", *KernelFunction);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetKernelFunctionCount
**
**  Get the number of kernel functions for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctSIZE_T * Count
**          Pointer to a variable receiving the number of kernel functions.
*/
gceSTATUS
gcSHADER_GetKernelFunctionCount(
    IN gcSHADER Shader,
    OUT gctSIZE_T * Count
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    /* Return uniform count. */
    *Count = Shader->kernelFunctionCount;

    /* Success. */
    gcmFOOTER_ARG("*Count=%lu", *Count);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetUniform
**
**  Get the gcUNIFORM object pointer for an indexed uniform for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT Index
**          Index of uniform to retreive the name for.
**
**  OUTPUT:
**
**      gcUNIFORM * Uniform
**          Pointer to a variable receiving the gcUNIFORM object pointer.
*/
gceSTATUS
gcSHADER_GetUniform(
    IN gcSHADER Shader,
    IN gctUINT Index,
    OUT gcUNIFORM * Uniform
    )
{
    gcmHEADER_ARG("Shader=0x%x Index=%u", Shader, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Index < Shader->uniformCount);
    gcmDEBUG_VERIFY_ARGUMENT(Uniform != gcvNULL);

    /* Return the gcUNIFORM object pointer. */
    *Uniform = Shader->uniforms[Index];

    /* Success. */
    gcmFOOTER_ARG("*Uniform=0x%x", *Uniform);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_ReallocateOutputs
**
**  Reallocate an array of pointers to gcOUTPUT objects.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcSHADER_ReallocateOutputs(
    IN gcSHADER Shader,
    IN gctSIZE_T Count
    )
{
    gcOUTPUT * outputs;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Count=%lu", Shader, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Count < Shader->outputCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == Shader->outputArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcOUTPUT) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    outputs = pointer;

    if (Shader->outputs != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(outputs,
                                   Shader->outputs,
                                   gcmSIZEOF(gcOUTPUT)
                                   * Shader->outputCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->outputs));
    }

    /* Set new gcOUTPUT object pointer. */
    Shader->outputArrayCount = Count;
    Shader->outputs          = outputs;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOutput
**
**  Add an output to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctCONST_STRING Name
**          Name of the output to add.
**
**      gcSHADER_TYPE Type
**          Type of the output to add.
**
**      gctSIZE_T Length
**          Array length of the output to add.  'Length' must be at least 1.
**
**      gctUINT16 TempRegister
**          Temporary register index that holds the output value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOutput(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN gctUINT16 TempRegister
    )
{
    gctSIZE_T nameLength, bytes, i;
    gcOUTPUT output;
    gceSTATUS status;
    gctBOOL copyName;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%lu TempRegister=%u",
                  Shader, Name, Type, Length, TempRegister);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Check array count. */
    if (Shader->outputArrayCount < Shader->outputCount + Length)
    {
        /* Reallocate a new array of object pointers. */
        status = gcSHADER_ReallocateOutputs(Shader, Shader->outputCount + Length + 9);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Test for position name. */
    if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#Position")))
    {
        nameLength = gcSL_POSITION;
        bytes      = gcmOFFSETOF(_gcOUTPUT, name);
        copyName   = gcvFALSE;
    }

    /* Test for point size name. */
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#PointSize")))
    {
        nameLength = gcSL_POINT_SIZE;
        bytes      = gcmOFFSETOF(_gcOUTPUT, name);
        copyName   = gcvFALSE;
    }

    /* Test for color name. */
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#Color")))
    {
        nameLength = gcSL_COLOR;
        bytes      = gcmOFFSETOF(_gcOUTPUT, name);
        copyName   = gcvFALSE;
    }

    /* Test for front facing name. */
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#FrontFacing")))
    {
        nameLength = gcSL_FRONT_FACING;
        bytes      = gcmOFFSETOF(_gcOUTPUT, name);
        copyName   = gcvFALSE;
    }

    /* Test for point coordinate name. */
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#PointCoord")))
    {
        nameLength = gcSL_POINT_COORD;
        bytes      = gcmOFFSETOF(_gcOUTPUT, name);
        copyName   = gcvFALSE;
    }

    /* Test for position.w name. */
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#Position.w")))
    {
        nameLength = gcSL_POSITION_W;
        bytes      = gcmOFFSETOF(_gcOUTPUT, name);
        copyName   = gcvFALSE;
    }

    /* Test for frag depth name. */
    else if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, "#Depth")))
    {
        nameLength = gcSL_DEPTH;
        bytes      = gcmOFFSETOF(_gcOUTPUT, name);
        copyName   = gcvFALSE;
    }

    else
    {
        /* Get the length of the name. */
        gcmVERIFY_OK(gcoOS_StrLen(Name, &nameLength));

        /* Compute the number of bytes required for the gcOUTPUT object. */
        bytes      = gcmOFFSETOF(_gcOUTPUT, name) + nameLength + 1;

        /* Mark the name to be copied. */
        copyName   = gcvTRUE;
    }

    /* Generate gcOUTPUT object for each array entry. */
    for (i = 0; i < Length; ++i)
    {
        /* Allocate the gcOUTPUT object. */
        status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        output = pointer;

        /* Initialize the gcOUTPUT object. */
        output->object.type = gcvOBJ_OUTPUT;
        output->type        = Type;
        output->arraySize   = Length;
        output->tempIndex   = TempRegister;
        output->physical    = gcvFALSE;
        output->nameLength  = nameLength;

        if (copyName)
        {
            /* Copy the output name. */
            gcmVERIFY_OK(gcoOS_MemCopy(output->name, Name, nameLength + 1));
        }

        /* Set new gcOUTPUT object pointer. */
        Shader->outputs[Shader->outputCount++] = output;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOutputIndexed
**
**  Add an indexed output to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctCONST_STRING Name
**          Name of the output to add.
**
**      gctINT Index
**          Index of output to add.
**
**      gctUINT16 TempRegister
**          Temporary register index that holds the output value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOutputIndexed(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,
    IN gctSIZE_T Index,
    IN gctUINT16 TempIndex
    )
{
    gctSIZE_T i;

    gcmHEADER_ARG("Shader=0x%x Name=%s Index=%lu TempIndex=%d",
                  Shader, Name, Index, TempIndex);

    for (i = 0; i < Shader->outputCount; ++i)
    {
        gcOUTPUT output = Shader->outputs[i];

        if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, output->name)))
        {
            if (Index >= output->arraySize)
            {
                gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_INDEX);
                return gcvSTATUS_INVALID_INDEX;
            }

            output = Shader->outputs[i + Index];
            output->tempIndex = TempIndex;

            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
    }

    gcmFOOTER_ARG("status=%d", gcvSTATUS_NAME_MISMATCH);
    return gcvSTATUS_NAME_MISMATCH;
}

/*******************************************************************************
**  gcSHADER_GetOutputCount
**
**  Get the number of outputs for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctSIZE_T * Count
**          Pointer to a variable receiving the number of outputs.
*/
gceSTATUS
gcSHADER_GetOutputCount(
    IN gcSHADER Shader,
    OUT gctSIZE_T * Count
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    /* Return output count. */
    *Count = Shader->outputCount;

    /* Success. */
    gcmFOOTER_ARG("*Count=%lu", *Count);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetOutput
**
**  Get the gcOUTPUT object pointer for an indexed output for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT Index
**          Index of output to retrieve.
**
**  OUTPUT:
**
**      gcOUTPUT * Output
**          Pointer to a variable receiving the gcOUTPUT object pointer.
*/
gceSTATUS
gcSHADER_GetOutput(
    IN gcSHADER Shader,
    IN gctUINT Index,
    OUT gcOUTPUT * Output
    )
{
    gcmHEADER_ARG("Shader=0x%x Index=%u", Shader, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Index < Shader->outputCount);
    gcmDEBUG_VERIFY_ARGUMENT(Output != gcvNULL);

    /* Return the gcOUTPUT object pointer. */
    *Output = Shader->outputs[Index];

    /* Success. */
    gcmFOOTER_ARG("*Output=0x%x", *Output);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**							   gcSHADER_GetOutputByName
********************************************************************************
**
**	Get the gcOUTPUT object pointer for this shader by output name.
**
**	INPUT:
**
**		gcSHADER Shader
**			Pointer to a gcSHADER object.
**
**		gctSTRING name
**			Name of output to retrieve.
**
**      gctSIZE_T nameLength
**          Length of name to retrieve
**
**	OUTPUT:
**
**		gcOUTPUT * Output
**			Pointer to a variable receiving the gcOUTPUT object pointer.
*/
gceSTATUS
gcSHADER_GetOutputByName(
	IN gcSHADER Shader,
	IN gctSTRING name,
    IN gctSIZE_T nameLength,
	OUT gcOUTPUT * Output
	)
{
    gctSIZE_T idx;

    gcmHEADER_ARG("Shader=0x%x Index=%s", Shader, name);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Output != gcvNULL);

    *Output = gcvNULL;
    for (idx = 0; idx < Shader->outputCount; idx++)
    {
        gcOUTPUT output = Shader->outputs[idx];

        if ((output->nameLength == nameLength)
            &&  gcmIS_SUCCESS(gcoOS_MemCmp(output->name,
                              name,
                              nameLength))
            )
        {
            break;
        }
    }

    if (idx < Shader->outputCount)
        *Output = Shader->outputs[idx];

    /* Success. */
    gcmFOOTER_ARG("*Output=0x%x", *Output);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_ReallocateVariables
**
**  Reallocate an array of pointers to gcVARIABLE objects.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcSHADER_ReallocateVariables(
    IN gcSHADER Shader,
    IN gctSIZE_T Count
    )
{
    gcVARIABLE * variables;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Count=%lu", Shader, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Count < Shader->variableCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == Shader->variableArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcVARIABLE) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    variables = pointer;

    if (Shader->variables != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(variables,
                                   Shader->variables,
                                   gcmSIZEOF(gcVARIABLE)
                                   * Shader->variableCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->variables));
    }

    /* Set new gcVARIABLE object pointer. */
    Shader->variableArrayCount = Count;
    Shader->variables          = variables;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddVariable
**
**  Add a variable to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctCONST_STRING Name
**          Name of the variable to add.
**
**      gcSHADER_TYPE Type
**          Type of the variable to add.
**
**      gctSIZE_T Length
**          Array length of the variable to add.  'Length' must be at least 1.
**
**      gctUINT16 TempRegister
**          Temporary register index that holds the variable value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddVariable(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN gctUINT16 TempRegister
    )
{
    gctSIZE_T nameLength=0, bytes;
    gcVARIABLE variable;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%lu TempRegister=%u",
                  Shader, Name, Type, Length, TempRegister);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Check array count. */
    if (Shader->variableArrayCount <= Shader->variableCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcSHADER_ReallocateVariables(Shader, Shader->variableCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Get the length of the name. */
    gcmVERIFY_OK(gcoOS_StrLen(Name, &nameLength));

    /* Compute the number of bytes required for the gcOUTPUT object. */
    bytes = gcmOFFSETOF(_gcVARIABLE, name) + nameLength + 1;

    /* Allocate the gcVARIABLE object. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    variable = pointer;

    /* Initialize the gcVARIABLE object. */
    variable->object.type = gcvOBJ_VARIABLE;
    variable->varCategory = gcSHADER_VAR_CATEGORY_NORMAL;
    variable->firstChild = -1;
    variable->nextSibling = -1;
    variable->prevSibling = -1;
    variable->parent = -1;
    variable->u.type      = Type;
    variable->arraySize   = Length;
    variable->tempIndex   = TempRegister;
    variable->nameLength  = nameLength;

    /* Copy the variable name. */
    gcmVERIFY_OK(gcoOS_MemCopy(variable->name, Name, nameLength + 1));

    /* Set new gcVARIABLE object pointer. */
    Shader->variables[Shader->variableCount++] = variable;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddVariableEx
**
**  Add a variable to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctCONST_STRING Name
**          Name of the variable to add.
**
**      gcSHADER_TYPE Type
**          Type of the variable to add.
**
**      gctSIZE_T Length
**          Array length of the variable to add.  'Length' must be at least 1.
**
**      gctUINT16 TempRegister
**          Temporary register index that holds the variable value.
**
**      gcSHADER_VAR_CATEGORY varCategory
**          Variable category, normal or struct.
**
**      gctUINT16 numStructureElement
**          If struct, its element number.
**
**      gctINT16 parent
**          If struct, parent index in gcSHADER.variables.
**
**      gctINT16 preSibling
**          If struct, previous sibling index in gcSHADER.variables.
**
**      gctINT16* ThisVarIndex
**          Returned value about variable index in gcSHADER.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddVariableEx(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    IN gctUINT16 TempRegister,
    IN gcSHADER_VAR_CATEGORY varCategory,
    IN gctUINT16 numStructureElement,
    IN gctINT16 parent,
    IN gctINT16 prevSibling,
    OUT gctINT16* ThisVarIndex
    )
{
    gctSIZE_T nameLength=0, bytes;
    gcVARIABLE variable;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;
    gctINT16 thisIdx;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%lu TempRegister=%u \
                   varCategory=%d numStructureElement=%d parent=%d \
                   prevSibling=%d ThisIndex=%d",
                  Shader, Name, Type, Length, TempRegister, varCategory,
                  numStructureElement, parent, prevSibling, ThisVarIndex);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Check array count. */
    if (Shader->variableArrayCount <= Shader->variableCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcSHADER_ReallocateVariables(Shader, Shader->variableCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Get the length of the name. */
    gcmVERIFY_OK(gcoOS_StrLen(Name, &nameLength));

    /* Compute the number of bytes required for the gcOUTPUT object. */
    bytes = gcmOFFSETOF(_gcVARIABLE, name) + nameLength + 1;

    /* Allocate the gcVARIABLE object. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    variable = pointer;

    /* Initialize the gcVARIABLE object. */
    variable->object.type = gcvOBJ_VARIABLE;
    variable->varCategory = varCategory;

    if (varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
        variable->u.type = Type;
    else
        variable->u.numStructureElement = numStructureElement;

    /* Link variables */
    thisIdx = (gctINT16)Shader->variableCount;
    variable->parent = parent;
    if (parent != -1)
    {
        if (Shader->variables[parent]->firstChild == -1)
            Shader->variables[parent]->firstChild = thisIdx;
        else
        {
            gctINT curIdx, preIdx;
            curIdx = Shader->variables[parent]->firstChild;
            preIdx = -1;
            while (curIdx != -1)
            {
                preIdx = curIdx;
                curIdx = Shader->variables[curIdx]->nextSibling;
            }
            gcmASSERT(preIdx != -1);
            Shader->variables[preIdx]->nextSibling = thisIdx;
        }
    }

    variable->prevSibling = prevSibling;
    if (prevSibling != -1)
        Shader->variables[prevSibling]->nextSibling = thisIdx;

    variable->nextSibling = -1;
    variable->firstChild = -1;

    variable->arraySize   = Length;
    variable->tempIndex   = TempRegister;
    variable->nameLength  = nameLength;

    /* Copy the variable name. */
    gcmVERIFY_OK(gcoOS_MemCopy(variable->name, Name, nameLength + 1));

    /* Set new gcVARIABLE object pointer. */
    Shader->variables[Shader->variableCount++] = variable;

    /* Return this index */
    if (ThisVarIndex)
        *ThisVarIndex = thisIdx;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_UpdateVariable
********************************************************************************
**
**  Update a variable to a gcSHADER object.
**
**  INPUT:
**
**		gcSHADER Shader
**			Pointer to a gcSHADER object.
**
**		gctUINT Index
**			Index of variable to retrieve.
**
**		gceVARIABLE_UPDATE_FLAGS flag
**			Flag which property of variable will be updated.
**
**      gctUINT16 newValue
**          New value to update.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_UpdateVariable(
    IN gcSHADER Shader,
    IN gctUINT Index,
    IN gceVARIABLE_UPDATE_FLAGS flag,
    IN gctUINT16 newValue
    )
{
    gcVARIABLE   variable;

    gcmHEADER_ARG("Shader=0x%x Index=%u flag=%d, newValue=%d", Shader, Index, flag, newValue);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Index < Shader->variableCount);

    gcmVERIFY_OK(gcSHADER_GetVariable(Shader, Index, &variable));

    switch (flag)
    {
    case gceVARIABLE_UPDATE_TEMPREG:
        variable->tempIndex = newValue;
        break;
    case gceVARIABLE_UPDATE_NOUPDATE:
    default:
        break;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
/*******************************************************************************
**  gcSHADER_GetVariableCount
**
**  Get the number of variables for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      gctSIZE_T * Count
**          Pointer to a variable receiving the number of variables.
*/
gceSTATUS
gcSHADER_GetVariableCount(
    IN gcSHADER Shader,
    OUT gctSIZE_T * Count
    )
{
    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    /* Return output count. */
    *Count = Shader->variableCount;

    /* Success. */
    gcmFOOTER_ARG("*Count=%lu", *Count);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_GetVariable
**
**  Get the gcVARIABLE object pointer for an indexed variable for this shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT Index
**          Index of variable to retrieve.
**
**  OUTPUT:
**
**      gcVARIABLE * Variable
**          Pointer to a variable receiving the gcVARIABLE object pointer.
*/
gceSTATUS
gcSHADER_GetVariable(
    IN gcSHADER Shader,
    IN gctUINT Index,
    OUT gcVARIABLE * Variable
    )
{
    gcmHEADER_ARG("Shader=0x%x Index=%u", Shader, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Index < Shader->variableCount);
    gcmDEBUG_VERIFY_ARGUMENT(Variable != gcvNULL);

    /* Return the gcOUTPUT object pointer. */
    *Variable = Shader->variables[Index];

    /* Success. */
    gcmFOOTER_ARG("*Variable=0x%x", *Variable);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**   _ExpandCode
**
**  Expand the code size in the gcSHADER object by another 32 instructions.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
_ExpandCode(
    IN gcSHADER Shader
    )
{
    gctSIZE_T bytes;
    gceSTATUS status;
    gcSL_INSTRUCTION code;
    gctPOINTER pointer = gcvNULL;

    /* Allocate 32 extra instruction slots. */
    bytes = (Shader->codeCount + 32) * sizeof(struct _gcSL_INSTRUCTION);

    /* Allocate the memory. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        return status;
    }

    code = pointer;

    if (Shader->code != gcvNULL)
    {
        /* Copy the current code. */
        gcmVERIFY_OK(gcoOS_MemCopy(code,
                               Shader->code,
                               Shader->codeCount *
                               sizeof(struct _gcSL_INSTRUCTION)));

        /* Free the current code buffer. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->code));
    }

    /* Zero the new instruction slots (we might not write in all). */
    gcmVERIFY_OK(gcoOS_ZeroMemory(code + Shader->codeCount,
                              32 * sizeof(struct _gcSL_INSTRUCTION)));

    /* Adjust the array counters. */
    Shader->codeCount += 32;
    Shader->code       = code;

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOpcode
**
**  Add an opcode to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_OPCODE Opcode
**          Opcode to add.
**
**      gctUINT16 TempRegister
**          Temporary register index that acts as the target of the opcode.
**
**      gctUINT8 Enable
**          Write enable value for the temporary register that acts as the
**          target of the opcode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOpcode(
    IN gcSHADER Shader,
    IN gcSL_OPCODE Opcode,
    IN gctUINT16 TempRegister,
    IN gctUINT8 Enable,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Opcode=%d TempRegister=%u Enable=%u Format=%d",
                  Shader, Opcode, TempRegister, Enable, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->lastInstruction++;
    }

    /* Did we reach the end of the allocated instruction array? */
    if (Shader->lastInstruction >= Shader->codeCount)
    {
        /* Allocate 32 extra instruction slots. */
        status = _ExpandCode(Shader);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    /* Initialize the opcode portion of the instruction. */
    code->opcode      = Opcode;
    code->temp        = (gctUINT16) (Enable | (Format << 12));
    code->tempIndex   = TempRegister;
    code->tempIndexed = 0;

    /* Move to SOURCE0 operand. */
    Shader->instrIndex = gcSHADER_SOURCE0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOpcode2
**
**  Add an opcode and condition to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_OPCODE Opcode
**          Opcode to add.
**
**      gcSL_CONDITION Condition
**          Condition of the opcode.
**
**      gctUINT16 TempRegister
**          Temporary register index that acts as the target of the opcode.
**
**      gctUINT8 Enable
**          Write enable value for the temporary register that acts as the
**          target of the opcode.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOpcode2(
    IN gcSHADER Shader,
    IN gcSL_OPCODE Opcode,
    IN gcSL_CONDITION Condition,
    IN gctUINT16 TempRegister,
    IN gctUINT8 Enable,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Opcode=%d Condition=%d TempRegister=%u "
                  "Enable=%u Format=%d",
                  Shader, Opcode, Condition, TempRegister, Enable, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->lastInstruction++;
    }

    /* Did we reach the end of teh allocated instruction array? */
    if (Shader->lastInstruction >= Shader->codeCount)
    {
        /* Allocate 32 extra instruction slots. */
        status = _ExpandCode(Shader);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Point to the current instrcution. */
    code = Shader->code + Shader->lastInstruction;

    /* Initialize the opcode portion of the instruction. */
    code->opcode      = Opcode;
    code->temp        = gcmSL_TARGET_SET(0, Condition, Condition)
                      | gcmSL_TARGET_SET(0, Enable,    Enable)
                      | gcmSL_TARGET_SET(0, Format,    Format);
    code->tempIndex   = TempRegister;
    code->tempIndexed = 0;

    /* Move to SOURCE0 operand. */
    Shader->instrIndex = gcSHADER_SOURCE0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOpcodeIndexed
**
**  Add an opcode to a gcSHADER object that writes to an dynamically indexed
**  target.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_OPCODE Opcode
**          Opcode to add.
**
**      gctUINT16 TempRegister
**          Temporary register index that acts as the target of the opcode.
**
**      gctUINT8 Enable
**          Write enable bits  for the temporary register that acts as the
**          target of the opcode.
**
**      gcSL_INDEXED Mode
**          Location of the dynamic index inside the temporary register.  Valid
**          values can be:
**
**              gcSL_INDEXED_X - Use x component of the temporary register.
**              gcSL_INDEXED_Y - Use y component of the temporary register.
**              gcSL_INDEXED_Z - Use z component of the temporary register.
**              gcSL_INDEXED_W - Use w component of the temporary register.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOpcodeIndexed(
    IN gcSHADER Shader,
    IN gcSL_OPCODE Opcode,
    IN gctUINT16 TempRegister,
    IN gctUINT8 Enable,
    IN gcSL_INDEXED Mode,
    IN gctUINT16 IndexRegister,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Opcode=%d TempRegister=%u Enable=%u Mode=%d "
                  "IndexRegister=%u Format=%d",
                  Shader, Opcode, TempRegister, Enable, Mode, IndexRegister,
                  Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->lastInstruction++;
    }

    /* Did we reach the end of teh allocated instruction array? */
    if (Shader->lastInstruction >= Shader->codeCount)
    {
        /* Allocate 32 extra instruction slots. */
        status = _ExpandCode(Shader);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Point to the current instrcution. */
    code = Shader->code + Shader->lastInstruction;

    /* Initialize the opcode portion of the instruction. */
    code->opcode      = Opcode;
    code->temp        = (gctUINT16) (Enable | (Mode << 4) | (Format << 12));
    code->tempIndex   = TempRegister;
    code->tempIndexed = IndexRegister;

    /* Move to SOURCE0 operand. */
    Shader->instrIndex = gcSHADER_SOURCE0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOpcodeConditionIndexed
**
**  Add an opcode to a gcSHADER object that writes to an dynamically indexed
**  target.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_OPCODE Opcode
**          Opcode to add.
**
**      gcSL_CONDITION Condition
**          Condition to check.
**
**      gctUINT16 TempRegister
**          Temporary register index that acts as the target of the opcode.
**
**      gctUINT8 Enable
**          Write enable bits  for the temporary register that acts as the
**          target of the opcode.
**
**      gcSL_INDEXED Indexed
**          Location of the dynamic index inside the temporary register.  Valid
**          values can be:
**
**              gcSL_INDEXED_X - Use x component of the temporary register.
**              gcSL_INDEXED_Y - Use y component of the temporary register.
**              gcSL_INDEXED_Z - Use z component of the temporary register.
**              gcSL_INDEXED_W - Use w component of the temporary register.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOpcodeConditionIndexed(
    IN gcSHADER Shader,
    IN gcSL_OPCODE Opcode,
    IN gcSL_CONDITION Condition,
    IN gctUINT16 TempRegister,
    IN gctUINT8 Enable,
    IN gcSL_INDEXED Indexed,
    IN gctUINT16 IndexRegister,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Opcode=%d Condition=%u TempRegister=%u Enable=%u Indexed=%d "
                  "IndexRegister=%u Format=%d",
                  Shader, Opcode, Condition, TempRegister, Enable, Indexed, IndexRegister,
                  Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->lastInstruction++;
    }

    /* Did we reach the end of teh allocated instruction array? */
    if (Shader->lastInstruction >= Shader->codeCount)
    {
        /* Allocate 32 extra instruction slots. */
        status = _ExpandCode(Shader);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Point to the current instrcution. */
    code = Shader->code + Shader->lastInstruction;

    /* Initialize the opcode portion of the instruction. */
    code->opcode      = Opcode;
    code->temp        = gcmSL_TARGET_SET(0, Condition, Condition)
                      | gcmSL_TARGET_SET(0, Enable,    Enable)
                      | gcmSL_TARGET_SET(0, Indexed,   Indexed)
                      | gcmSL_TARGET_SET(0, Format,    Format);
    code->tempIndex   = TempRegister;
    code->tempIndexed = IndexRegister;

    /* Move to SOURCE0 operand. */
    Shader->instrIndex = gcSHADER_SOURCE0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  _FindLabel
**
**  Find a label inside the gcSHADER object or create one if the label could not
**  be found.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gctUINT Label
**          Label identifier.
**
**  OUTPUT:
**
**      gcSHADER_LABEL * ShaderLabel
**          Pointer to a variable receiving the pointer to the gcSHADER_LABEL
**          structure representing the requested label.
*/
gceSTATUS
_FindLabel(
    IN gcSHADER Shader,
    IN gctUINT Label,
    OUT gcSHADER_LABEL * ShaderLabel
    )
{
    gcSHADER_LABEL label;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Label=%u", Shader, Label);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(ShaderLabel != gcvNULL);

    /* Walk all defines shader labels to find the requested label. */
    for (label = Shader->labels; label != gcvNULL; label = label->next)
    {
        if (label->label == Label)
        {
            /* Label matches, return pointer to gcSHADER_LABEL structure. */
            *ShaderLabel = label;

            /* Success. */
            gcmFOOTER_ARG("*ShaderLabel=0x%x", *ShaderLabel);
            return gcvSTATUS_OK;
        }
    }

    /* Allocate a new gcSHADER_LABEL structure.  */
    status = gcoOS_Allocate(gcvNULL,
                           sizeof(struct _gcSHADER_LABEL),
                           &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    label = pointer;

    /* Initialize the gcSHADER_LABEL structure. */
    label->next       = Shader->labels;
    label->label      = Label;
    label->defined    = ~0U;
    label->referenced = gcvNULL;

    /* Move gcSHADER_LABEL structure to head of list. */
    Shader->labels = label;

    /* Return pointer to the gcSHADER_LABEL structure. */
    *ShaderLabel = label;

    /* Success. */
    gcmFOOTER_ARG("*ShaderLabel=0x%x", *ShaderLabel);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOpcodeConditional
**
**  Add an conditional jump or call opcode to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_OPCODE Opcode
**          Opcode to add.
**
**      gcSL_CONDITION Condition
**          Condition that needs to evaluate to gcvTRUE in order for the opcode to
**          execute.
**
**      gctUINT Label
**          Target label if 'Condition' evaluates to gcvTRUE.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOpcodeConditional(
    IN gcSHADER Shader,
    IN gcSL_OPCODE Opcode,
    IN gcSL_CONDITION Condition,
    IN gctUINT Label
    )
{
    gcSL_INSTRUCTION code;
    gceSTATUS status;
    gcSHADER_LABEL label;
    gcSHADER_LINK link;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Opcode=%d Condition=%d Label=%u",
                  Shader, Opcode, Condition, Label);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->lastInstruction++;
    }

    /* Did we reach the end of teh allocated instruction array? */
    if (Shader->lastInstruction >= Shader->codeCount)
    {
        /* Allocate 32 extra instruction slots. */
        status = _ExpandCode(Shader);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Point to the current instrcution. */
    code = Shader->code + Shader->lastInstruction;

    /* Initialize the opcode portion of the instruction. */
    code->opcode      = Opcode;
    code->temp        = (gctUINT16) (Condition << 8);
    code->tempIndex   = (gctUINT16) Label;

    if ( (Opcode == gcSL_JMP) || (Opcode == gcSL_CALL) )
    {
        /* Find or create the label. */
        status = _FindLabel(Shader, Label, &label);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        /* Allocate a new gcSHADER_LINK structure. */
        status = gcoOS_Allocate(gcvNULL,
                               sizeof(struct _gcSHADER_LINK),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        link = pointer;

        /* Initialize the gcSHADER_LINK structure. */
        link->next       = label->referenced;
        link->referenced = Shader->lastInstruction;

        /* Move gcSHADER_LINK structure to head of list. */
        label->referenced = link;
    }

    /* Move to SOURCE0 operand. */
    Shader->instrIndex = gcSHADER_SOURCE0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOpcodeConditionalFormatted
**
**  Add an conditional jump or call opcode to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_OPCODE Opcode
**          Opcode to add.
**
**      gcSL_CONDITION Condition
**          Condition that needs to evaluate to gcvTRUE in order for the opcode to
**          execute.
**
**      gcSL_FORMAT Format
**          Format of conditional operands
**
**      gctUINT Label
**          Target label if 'Condition' evaluates to gcvTRUE.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOpcodeConditionalFormatted(
    IN gcSHADER Shader,
    IN gcSL_OPCODE Opcode,
    IN gcSL_CONDITION Condition,
    IN gcSL_FORMAT Format,
    IN gctUINT Label
    )
{
    gcSL_INSTRUCTION code;
    gceSTATUS status;
    gcSHADER_LABEL label;
    gcSHADER_LINK link;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Opcode=%d Condition=%d Format=%u Label=%u",
                  Shader, Opcode, Condition, Format, Label);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->lastInstruction++;
    }

    /* Did we reach the end of teh allocated instruction array? */
    if (Shader->lastInstruction >= Shader->codeCount)
    {
        /* Allocate 32 extra instruction slots. */
        status = _ExpandCode(Shader);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Point to the current instrcution. */
    code = Shader->code + Shader->lastInstruction;

    /* Initialize the opcode portion of the instruction. */
    code->opcode      = Opcode;
    code->temp        = gcmSL_TARGET_SET(0, Condition, Condition)
                      | gcmSL_TARGET_SET(0, Format, Format);
    code->tempIndex   = (gctUINT16) Label;

    if ( (Opcode == gcSL_JMP) || (Opcode == gcSL_CALL) )
    {
        /* Find or create the label. */
        status = _FindLabel(Shader, Label, &label);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        /* Allocate a new gcSHADER_LINK structure. */
        status = gcoOS_Allocate(gcvNULL,
                               sizeof(struct _gcSHADER_LINK),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        link = pointer;

        /* Initialize the gcSHADER_LINK structure. */
        link->next       = label->referenced;
        link->referenced = Shader->lastInstruction;

        /* Move gcSHADER_LINK structure to head of list. */
        label->referenced = link;
    }

    /* Move to SOURCE0 operand. */
    Shader->instrIndex = gcSHADER_SOURCE0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddOpcodeConditionalFormattedEnable
**
**  Add an conditional jump or call opcode to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_OPCODE Opcode
**          Opcode to add.
**
**      gcSL_CONDITION Condition
**          Condition that needs to evaluate to gcvTRUE in order for the opcode to
**          execute.
**
**      gcSL_FORMAT Format
**          Format of conditional operands
**
**      gctUINT8 Enable
**          Write enable value for the target of the opcode.
**
**      gctUINT Label
**          Target label if 'Condition' evaluates to gcvTRUE.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddOpcodeConditionalFormattedEnable(
    IN gcSHADER Shader,
    IN gcSL_OPCODE Opcode,
    IN gcSL_CONDITION Condition,
    IN gcSL_FORMAT Format,
    IN gctUINT8 Enable,
    IN gctUINT Label
    )
{
    gcSL_INSTRUCTION code;
    gceSTATUS status;
    gcSHADER_LABEL label;
    gcSHADER_LINK link;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Opcode=%d Condition=%d "
                  "Enable=%u Format=%d Label=%u",
                  Shader, Opcode, Condition, Enable, Format, Label);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->lastInstruction++;
    }

    /* Did we reach the end of teh allocated instruction array? */
    if (Shader->lastInstruction >= Shader->codeCount)
    {
        /* Allocate 32 extra instruction slots. */
        status = _ExpandCode(Shader);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Point to the current instrcution. */
    code = Shader->code + Shader->lastInstruction;

    /* Initialize the opcode portion of the instruction. */
    code->opcode      = Opcode;
    code->temp        = gcmSL_TARGET_SET(0, Condition, Condition)
                      | gcmSL_TARGET_SET(0, Enable,    Enable)
                      | gcmSL_TARGET_SET(0, Format, Format);
    code->tempIndex   = (gctUINT16) Label;

    if ( (Opcode == gcSL_JMP) || (Opcode == gcSL_CALL) )
    {
        /* Find or create the label. */
        status = _FindLabel(Shader, Label, &label);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        /* Allocate a new gcSHADER_LINK structure. */
        status = gcoOS_Allocate(gcvNULL,
                               sizeof(struct _gcSHADER_LINK),
                               &pointer);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }

        link = pointer;

        /* Initialize the gcSHADER_LINK structure. */
        link->next       = label->referenced;
        link->referenced = Shader->lastInstruction;

        /* Move gcSHADER_LINK structure to head of list. */
        label->referenced = link;
    }

    /* Move to SOURCE0 operand. */
    Shader->instrIndex = gcSHADER_SOURCE0;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddLabel
**
**  Define a label at the current instruction of a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT Label
**          Label to define.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddLabel(
    IN gcSHADER Shader,
    IN gctUINT Label
    )
{
    gcSHADER_LABEL label;
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Label=%u", Shader, Label);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->instrIndex = gcSHADER_OPCODE;
        Shader->lastInstruction++;
    }

    /* Find or create the label. */
    status = _FindLabel(Shader, Label, &label);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    /* Set definition of the label. */
    label->defined = Shader->lastInstruction;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddSource
**
**  Add a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_TYPE Type
**          Type of the source operand.
**
**      gctUINT16 SourceIndex
**          Index of the source operand.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSource(
    IN gcSHADER Shader,
    IN gcSL_TYPE Type,
    IN gctUINT16 SourceIndex,
    IN gctUINT8 Swizzle,
    IN gcSL_FORMAT Format
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Type=%d SourceIndex=%u Swizzle=%u Format=%d",
                  Shader, Type, SourceIndex, Swizzle, Format);

    /* Use AddSourceIndexed with addressing mode gcSL_NOT_INDEXED. */
    status = gcSHADER_AddSourceIndexed(Shader,
                                       Type,
                                       SourceIndex,
                                       Swizzle,
                                       gcSL_NOT_INDEXED,
                                       0,
                                       Format);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcSHADER_AddSourceIndexed
**
**  Add a dynamically indexed source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcSL_TYPE Type
**          Type of the source operand.
**
**      gctUINT16 SourceIndex
**          Index of the source operand.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gcSL_INDEXED Mode
**          Addressing mode for the index.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceIndexed(
    IN gcSHADER Shader,
    IN gcSL_TYPE Type,
    IN gctUINT16 SourceIndex,
    IN gctUINT8 Swizzle,
    IN gcSL_INDEXED Mode,
    IN gctUINT16 IndexRegister,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;
    gctUINT16 source;

    gcmHEADER_ARG("Shader=0x%x Type=%d SourceIndex=%u Swizzle=%u Mode=%d "
                  "IndexRegister=%u Format=%d",
                  Shader, Type, SourceIndex, Swizzle, Mode, IndexRegister,
                  Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    source = gcmSL_SOURCE_SET(0, Swizzle, Swizzle)
           | gcmSL_SOURCE_SET(0, Type,    Type)
           | gcmSL_SOURCE_SET(0, Indexed, Mode)
           | gcmSL_SOURCE_SET(0, Format,  Format);

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        code->source0        = source;
        code->source0Index   = SourceIndex;
        code->source0Indexed = IndexRegister;

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    case gcSHADER_SOURCE1:
        /* Update source1 operand. */
        code->source1        = source;
        code->source1Index   = SourceIndex;
        code->source1Indexed = IndexRegister;

        /* Move to next instruction. */
        Shader->instrIndex = gcSHADER_OPCODE;
        Shader->lastInstruction++;
        break;

    default:
        /* Invalid data. */
        gcmFOOTER_NO();
        return gcvSTATUS_INVALID_DATA;
    }

    /* Suuccess. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddSourceAttribute
**
**  Add an attribute as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcATTRIBUTE Attribute
**          Pointer to a gcATTRIBUTE object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gctINT Index
**          Static index into the attribute in case the attribute is a matrix
**          or array.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceAttribute(
    IN gcSHADER Shader,
    IN gcATTRIBUTE Attribute,
    IN gctUINT8 Swizzle,
    IN gctINT Index
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Attribute=0x%x Swizzle=%u Index=%d",
                  Shader, Attribute, Swizzle, Index);

    /* Use AddSourceAttributeIndexed with addressing mode gcSL_NOT_INDEXED. */
    status = gcSHADER_AddSourceAttributeIndexed(Shader,
                                                Attribute,
                                                Swizzle,
                                                Index,
                                                gcSL_NOT_INDEXED,
                                                0);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcSHADER_AddSourceAttributeIndexed
**
**  Add an indexed attribute as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcATTRIBUTE Attribute
**          Pointer to a gcATTRIBUTE object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gctINT Index
**          Static index into the attribute in case the attribute is a matrix
**          or array.
**
**      gcSL_INDEXED Mode
**          Addressing mode of the dynamic index.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceAttributeIndexed(
    IN gcSHADER Shader,
    IN gcATTRIBUTE Attribute,
    IN gctUINT8 Swizzle,
    IN gctINT Index,
    IN gcSL_INDEXED Mode,
    IN gctUINT16 IndexRegister
    )
{
    gcSL_INSTRUCTION code;
    gctUINT16 source;
    gctUINT16 index;
    gctUINT16 indexRegister;

    gcmHEADER_ARG("Shader=0x%x Attribute=0x%x Swizzle=%u Index=%d Mode=%d "
                  "IndexRegister=%u",
                  Shader, Attribute, Swizzle, Index, Mode, IndexRegister);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(Attribute, gcvOBJ_ATTRIBUTE);

    /* Cteate the source. */
    source = gcmSL_SOURCE_SET(0, Type, gcSL_ATTRIBUTE)
           | gcmSL_SOURCE_SET(0, Indexed, Mode)
           | gcmSL_SOURCE_SET(0, Swizzle, Swizzle);

    /* Create the index. */
    index = gcmSL_INDEX_SET(0, Index,      Attribute->index)
          | gcmSL_INDEX_SET(0, ConstValue, Index);

    gcmASSERT((Mode == gcSL_NOT_INDEXED) || (Index < 4));

    indexRegister = (Mode == gcSL_NOT_INDEXED) ? (Index & ~3) : IndexRegister;

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        code->source0        = source;
        code->source0Index   = index;
        code->source0Indexed = indexRegister;

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    case gcSHADER_SOURCE1:
        /* Update source1 operand. */
        code->source1        = source;
        code->source1Index   = index;
        code->source1Indexed = indexRegister;

        /* Move to next instruction. */
        Shader->instrIndex = gcSHADER_OPCODE;
        Shader->lastInstruction++;
        break;

    default:
        gcmFOOTER_NO();
        /* Invalid data. */
        return gcvSTATUS_INVALID_DATA;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddSourceUniform
**
**  Add a uniform as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gctINT Index
**          Static index into the uniform in case the uniform is a matrix or
**          array.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceUniform(
    IN gcSHADER Shader,
    IN gcUNIFORM Uniform,
    IN gctUINT8 Swizzle,
    IN gctINT Index
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Uniform=0x%x Swizzle=%u Index=%d",
                  Shader, Uniform, Swizzle, Index);

    /* Use AddSourceUniformIndexed with addressing mode gcSL_NOT_INDEXED. */
    status = gcSHADER_AddSourceUniformIndexed(Shader,
                                              Uniform,
                                              Swizzle,
                                              Index,
                                              gcSL_NOT_INDEXED,
                                              0);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcSHADER_AddSourceUniformIndexed
**
**  Add an indexed uniform as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gctINT Index
**          Static index into the uniform in case the uniform is a matrix or
**          array.
**
**      gcSL_INDEXED Mode
**          Addressing mode of the dynamic index.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceUniformIndexed(
    IN gcSHADER Shader,
    IN gcUNIFORM Uniform,
    IN gctUINT8 Swizzle,
    IN gctINT Index,
    IN gcSL_INDEXED Mode,
    IN gctUINT16 IndexRegister
    )
{
    gcSL_INSTRUCTION code;
    gctUINT16 source;
    gctUINT16 index;
    gctUINT16 indexRegister;

    gcmHEADER_ARG("Shader=0x%08x Uniform=0x%08x Swizzle=%u Index=%d Mode=%d "
                  "IndexRegister=%u",
                  Shader, Uniform, Swizzle, Index, Mode, IndexRegister);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    /* Create the source. */
    source = gcmSL_SOURCE_SET(0, Type,    gcSL_UNIFORM)
           | gcmSL_SOURCE_SET(0, Indexed, Mode)
           | gcmSL_SOURCE_SET(0, Swizzle, Swizzle);

    /* Create the index. */
    index = gcmSL_INDEX_SET(0, Index,      Uniform->index)
          | gcmSL_INDEX_SET(0, ConstValue, Index);

    gcmASSERT((Mode == gcSL_NOT_INDEXED) || (Index < 4));

    indexRegister = (Mode == gcSL_NOT_INDEXED) ? (Index & ~3) : IndexRegister;

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        code->source0        = source;
        code->source0Index   = index;
        code->source0Indexed = indexRegister;

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    case gcSHADER_SOURCE1:
        /* Update source1 operand. */
        code->source1        = source;
        code->source1Index   = index;
        code->source1Indexed = indexRegister;

        /* Move to next instruction. */
        Shader->instrIndex = gcSHADER_OPCODE;
        Shader->lastInstruction++;
        break;

    default:
        /* Invalid data. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}


/*******************************************************************************
**  gcSHADER_AddSourceSamplerIndexed
**
**  Add a "0-based" indexed sampler as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gcSL_INDEXED Mode
**          Addressing mode of the dynamic index.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceSamplerIndexed(
    IN gcSHADER Shader,
    IN gctUINT8 Swizzle,
    IN gcSL_INDEXED Mode,
    IN gctUINT16 IndexRegister
    )
{
    gcSL_INSTRUCTION code;
    gctUINT16 source;

    gcmHEADER_ARG("Shader=0x%x Swizzle=%u Mode=%d IndexRegister=%u",
                  Shader, Swizzle, Mode, IndexRegister);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Mode != gcSL_NOT_INDEXED);

    /* Create the source. */
    source = gcmSL_SOURCE_SET(0, Type,    gcSL_SAMPLER)
           | gcmSL_SOURCE_SET(0, Indexed, Mode)
           | gcmSL_SOURCE_SET(0, Swizzle, Swizzle);

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    gcmASSERT(Shader->instrIndex == gcSHADER_SOURCE0);

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        code->source0        = *(gctUINT16 *) &source;
        code->source0Index   = 0;
        code->source0Indexed = IndexRegister;

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    default:
        /* Invalid data. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddSourceAttributeFormatted
**
**  Add a formatted attribute as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcATTRIBUTE Attribute
**          Pointer to a gcATTRIBUTE object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gctINT Index
**          Static index into the attribute in case the attribute is a matrix
**          or array.
**
**    gcSL_FORMAT Format
**        Format of attribute value
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceAttributeFormatted(
    IN gcSHADER Shader,
    IN gcATTRIBUTE Attribute,
    IN gctUINT8 Swizzle,
    IN gctINT Index,
    IN gcSL_FORMAT Format
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Attribute=0x%x Swizzle=%u Index=%d Format=%u",
                  Shader, Attribute, Swizzle, Index, Format);

    /* Use AddSourceAttributeIndexed with addressing mode gcSL_NOT_INDEXED. */
    status = gcSHADER_AddSourceAttributeIndexedFormatted(Shader,
                                                     Attribute,
                                                     Swizzle,
                                                     Index,
                                                     gcSL_NOT_INDEXED,
                                                     0,
                                                     Format);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcSHADER_AddSourceAttributeIndexedFormatted
**
**  Add a formatted indexed attribute as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcATTRIBUTE Attribute
**          Pointer to a gcATTRIBUTE object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gctINT Index
**          Static index into the attribute in case the attribute is a matrix
**          or array.
**
**      gcSL_INDEXED Mode
**          Addressing mode of the dynamic index.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**    gcSL_FORMAT Format
**        Format of indexed attribute value
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceAttributeIndexedFormatted(
    IN gcSHADER Shader,
    IN gcATTRIBUTE Attribute,
    IN gctUINT8 Swizzle,
    IN gctINT Index,
    IN gcSL_INDEXED Mode,
    IN gctUINT16 IndexRegister,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;
    gctUINT16 source;
    gctUINT16 index;
    gctUINT16 indexRegister;

    gcmHEADER_ARG("Shader=0x%x Attribute=0x%x Swizzle=%u Index=%d Mode=%d "
                  "IndexRegister=%u Format=%u",
                  Shader, Attribute, Swizzle, Index, Mode, IndexRegister, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(Attribute, gcvOBJ_ATTRIBUTE);

    /* Cteate the source. */
    source = gcmSL_SOURCE_SET(0, Type, gcSL_ATTRIBUTE)
           | gcmSL_SOURCE_SET(0, Indexed, Mode)
           | gcmSL_SOURCE_SET(0, Format,  Format)
           | gcmSL_SOURCE_SET(0, Swizzle, Swizzle);

    /* Create the index. */
    index = gcmSL_INDEX_SET(0, Index,      Attribute->index)
          | gcmSL_INDEX_SET(0, ConstValue, Index);

    gcmASSERT((Mode == gcSL_NOT_INDEXED) || (Index < 4));

    indexRegister = (Mode == gcSL_NOT_INDEXED) ? (Index & ~3) : IndexRegister;

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        code->source0        = source;
        code->source0Index   = index;
        code->source0Indexed = indexRegister;

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    case gcSHADER_SOURCE1:
        /* Update source1 operand. */
        code->source1        = source;
        code->source1Index   = index;
        code->source1Indexed = indexRegister;

        /* Move to next instruction. */
        Shader->instrIndex = gcSHADER_OPCODE;
        Shader->lastInstruction++;
        break;

    default:
        gcmFOOTER_NO();
        /* Invalid data. */
        return gcvSTATUS_INVALID_DATA;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddSourceUniformFormatted
**
**  Add a formatted uniform as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gctINT Index
**          Static index into the uniform in case the uniform is a matrix or
**          array.
**
**    gcSL_FORMAT Format
**        Format of UNIFORM value
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceUniformFormatted(
    IN gcSHADER Shader,
    IN gcUNIFORM Uniform,
    IN gctUINT8 Swizzle,
    IN gctINT Index,
    IN gcSL_FORMAT Format
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Uniform=0x%x Swizzle=%u Index=%d Format=%u",
                  Shader, Uniform, Swizzle, Index, Format);

    /* Use AddSourceUniformIndexed with addressing mode gcSL_NOT_INDEXED. */
    status = gcSHADER_AddSourceUniformIndexedFormatted(Shader,
                                                       Uniform,
                                                       Swizzle,
                                                       Index,
                                                       gcSL_NOT_INDEXED,
                                                       0,
                                                       Format);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**  gcSHADER_AddSourceUniformIndexedFormatted
**
**  Add a formatted indexed uniform as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gctINT Index
**          Static index into the uniform in case the uniform is a matrix or
**          array.
**
**      gcSL_INDEXED Mode
**          Addressing mode of the dynamic index.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**    gcSL_FORMAT Format
**        Format of uniform value
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceUniformIndexedFormatted(
    IN gcSHADER Shader,
    IN gcUNIFORM Uniform,
    IN gctUINT8 Swizzle,
    IN gctINT Index,
    IN gcSL_INDEXED Mode,
    IN gctUINT16 IndexRegister,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;
    gctUINT16 source;
    gctUINT16 index;
    gctUINT16 indexRegister;

    gcmHEADER_ARG("Shader=0x%08x Uniform=0x%08x Swizzle=%u Index=%d Mode=%d "
                  "IndexRegister=%u Format=%u",
                  Shader, Uniform, Swizzle, Index, Mode, IndexRegister, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    /* Create the source. */
    source = gcmSL_SOURCE_SET(0, Type,    gcSL_UNIFORM)
           | gcmSL_SOURCE_SET(0, Indexed, Mode)
           | gcmSL_SOURCE_SET(0, Format,  Format)
           | gcmSL_SOURCE_SET(0, Swizzle, Swizzle);

    /* Create the index. */
    index = gcmSL_INDEX_SET(0, Index,      Uniform->index)
          | gcmSL_INDEX_SET(0, ConstValue, Index);

    gcmASSERT((Mode == gcSL_NOT_INDEXED) || (Index < 4));

    indexRegister = (Mode == gcSL_NOT_INDEXED) ? (Index & ~3) : IndexRegister;

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        code->source0        = source;
        code->source0Index   = index;
        code->source0Indexed = indexRegister;

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    case gcSHADER_SOURCE1:
        /* Update source1 operand. */
        code->source1        = source;
        code->source1Index   = index;
        code->source1Indexed = indexRegister;

        /* Move to next instruction. */
        Shader->instrIndex = gcSHADER_OPCODE;
        Shader->lastInstruction++;
        break;

    default:
        /* Invalid data. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}


/*******************************************************************************
**  gcSHADER_AddSourceSamplerIndexedFormatted
**
**  Add a "0-based" formatted indexed sampler as a source operand to a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT8 Swizzle
**          x, y, z, and w swizzle values packed into one 8-bit value.
**
**      gcSL_INDEXED Mode
**          Addressing mode of the dynamic index.
**
**      gctUINT16 IndexRegister
**          Temporary register index that holds the dynamic index.
**
**    gcSL_FORMAT Format
**        Format of sampler value
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceSamplerIndexedFormatted(
    IN gcSHADER Shader,
    IN gctUINT8 Swizzle,
    IN gcSL_INDEXED Mode,
    IN gctUINT16 IndexRegister,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;
    gctUINT16 source;

    gcmHEADER_ARG("Shader=0x%x Swizzle=%u Mode=%d IndexRegister=%u Format=%u",
                  Shader, Swizzle, Mode, IndexRegister, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Mode != gcSL_NOT_INDEXED);

    /* Create the source. */
    source = gcmSL_SOURCE_SET(0, Type,    gcSL_SAMPLER)
           | gcmSL_SOURCE_SET(0, Indexed, Mode)
           | gcmSL_SOURCE_SET(0, Format,  Format)
           | gcmSL_SOURCE_SET(0, Swizzle, Swizzle);

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    gcmASSERT(Shader->instrIndex == gcSHADER_SOURCE0);

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        code->source0        = *(gctUINT16 *) &source;
        code->source0Index   = 0;
        code->source0Indexed = IndexRegister;

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    default:
        /* Invalid data. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_AddSourceConstant
**
**  Add a constant floating point value as a source operand to a gcSHADER
**  object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctFLOAT Constant
**          Floating point constant.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_AddSourceConstant(
    IN gcSHADER Shader,
    IN gctFLOAT Constant
    )
{
    gcSL_INSTRUCTION code;
    gcuFLOAT_UINT32 constant;

    gcmHEADER_ARG("Shader=0x%x Constant=%f", Shader, Constant);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        constant.f           = Constant;
        code->source0        = gcSL_CONSTANT;
        code->source0Index   = (gctUINT16) (constant.u & 0xFFFF);
        code->source0Indexed = (gctUINT16) (constant.u >> 16);

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    case gcSHADER_SOURCE1:
        /* Update source1 operand. */
        constant.f           = Constant;
        code->source1        = gcSL_CONSTANT;
        code->source1Index   = (gctUINT16) (constant.u & 0xFFFF);
        code->source1Indexed = (gctUINT16) (constant.u >> 16);

        /* Move to next instruction. */
        Shader->instrIndex = gcSHADER_OPCODE;
        Shader->lastInstruction++;
        break;

    default:
        /* Invalid data. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Suuccess. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**    gcSHADER_AddSourceConstantFormatted
**
**    Add a formatted constant value as a source operand to a gcSHADER object
**
**    INPUT:
**
**        gcSHADER Shader
**            Pointer to a gcSHADER object.
**
**        void *Constant
**            Pointer to constant value (32 bits).
**
**        gcSL_FORMAT Format
**            Format of constant value
**
**    OUTPUT:
**
**        Nothing.
*/
gceSTATUS
gcSHADER_AddSourceConstantFormatted(
    IN gcSHADER Shader,
    IN void *Constant,
    IN gcSL_FORMAT Format
    )
{
    gcSL_INSTRUCTION code;

    gcmHEADER_ARG("Shader=0x%x Constant=0x%x Format=%d", Shader, Constant, Format);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Point to the current instruction. */
    code = Shader->code + Shader->lastInstruction;

    switch (Shader->instrIndex)
    {
    case gcSHADER_SOURCE0:
        /* Update source0 operand. */
        code->source0        = gcmSL_SOURCE_SET(0, Type,    gcSL_CONSTANT)
                             | gcmSL_SOURCE_SET(0, Format,  Format);
        code->source0Index   = ((gctUINT16 *) Constant)[0];
        code->source0Indexed = ((gctUINT16 *) Constant)[1];

        /* Move to source1 operand. */
        Shader->instrIndex = gcSHADER_SOURCE1;
        break;

    case gcSHADER_SOURCE1:
        /* Update source1 operand. */
        code->source1        = gcmSL_SOURCE_SET(0, Type,    gcSL_CONSTANT)
                             | gcmSL_SOURCE_SET(0, Format,  Format);
        code->source1Index   = ((gctUINT16 *) Constant)[0];
        code->source1Indexed = ((gctUINT16 *) Constant)[1];

        /* Move to next instruction. */
        Shader->instrIndex = gcSHADER_OPCODE;
        Shader->lastInstruction++;
        break;

    default:
        /* Invalid data. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    /* Suuccess. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_Pack
**
**  Pack a dynamically created gcSHADER object by trimming the allocated arrays
**  and resolving all the labeling.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_Pack(
    IN gcSHADER Shader
    )
{
    gcSHADER_LABEL label;
    gcSHADER_LINK link;

    gcmHEADER_ARG("Shader=0x%x", Shader);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        /* Move to start of a new instruction. */
        Shader->lastInstruction++;
    }

    /* Update total number of instructions. */
    Shader->codeCount = Shader->lastInstruction;

    /* Loop while we have labels to resolve. */
    while (Shader->labels != gcvNULL)
    {
        /* Remove gcSHADER_LABEL structure from head of list. */
        label          = Shader->labels;
        Shader->labels = label->next;

        if (label->defined == ~0U)
        {
            /* Make sure the label is defined. */
            gcmFATAL("Label %u has not been defined.", label->label);
        }
        else
        {
            /* Loop while we have references to this label. */
            while (label->referenced != gcvNULL)
            {
                /* Remove gcSHADER_LINK structure from head of list. */
                link              = label->referenced;
                label->referenced = link->next;

                /* Update the reference with the correct label location. */
                Shader->code[link->referenced].tempIndex =
                    (gctUINT16) label->defined;

                /* Free the gcSHADER_LINK structure. */
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, link));
            }
        }

        /* Free the gcSHADER_LABEL structure. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, label));
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcSHADER_SetOptimizationOption
**
**  Set optimization option of a gcSHADER object.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctUINT OptimizationOption
**          Optimization option.  Can be one of the following:
**
**              0       - No optimization.
**              1       - Full optimization.
**              Others  - Testing patterns.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcSHADER_SetOptimizationOption(
    IN gcSHADER Shader,
    IN gctUINT OptimizationOption
    )
{
    gcmHEADER_ARG("Shader=0x%x OptimizationOption=%d",
                  Shader, OptimizationOption);

    if (OptimizationOption == 1)
        Shader->optimizationOption = gcvOPTIMIZATION_FULL;
     else
        Shader->optimizationOption = OptimizationOption;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcATTRIBUTE_GetType
**
**  Get the type and array length of a gcATTRIBUTE object.
**
**  INPUT:
**
**      gcATTRIBUTE Attribute
**          Pointer to a gcATTRIBUTE object.
**
**  OUTPUT:
**
**      gcSHADER_TYPE * Type
**          Pointer to a variable receiving the type of the attribute.  'Type'
**          can be gcvNULL, in which case no type will be returned.
**
**      gctSIZE_T * ArrayLength
**          Pointer to a variable receiving the length of the array if the
**          attribute was declared as an array.  If the attribute was not
**          declared as an array, the array length will be 1.  'ArrayLength' can
**          be gcvNULL, in which case no array length will be returned.
*/
gceSTATUS
gcATTRIBUTE_GetType(
    IN gcATTRIBUTE Attribute,
    OUT gcSHADER_TYPE * Type,
    OUT gctSIZE_T * ArrayLength
    )
{
    gcmHEADER_ARG("Attribute=0x%x Type=0x%x ArrayLength=0x%x", Attribute, Type, ArrayLength);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Attribute, gcvOBJ_ATTRIBUTE);

    if (Type != gcvNULL)
    {
        /* Return attribute type. */
        *Type = Attribute->type;
    }

    if (ArrayLength != gcvNULL)
    {
        /* Return attribute array length. */
        *ArrayLength = Attribute->arraySize;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  _PredefinedName
**
**  Convert a predefined name into an ASCII string.
**
**  INPUT:
**
**      gctSIZE_T Length
**          The predefined name.
**
**  OUTPUT:
**
**      Nothing.
**
**  RETURN VALUE:
**
**      gctCONST_STRING
**          Pointer to the predefined name or gcvNULL if 'Length' does not  specify
**          a predefined name.
*/
gctCONST_STRING
_PredefinedName(
    IN gctSIZE_T Length
    )
{
    switch (Length)
    {
    case gcSL_POSITION:
        return "gcSL_POSITION";

    case gcSL_POINT_SIZE:
        return "gcSL_POINT_SIZE";

    case gcSL_COLOR:
        return "gcSL_COLOR";

    case gcSL_FRONT_FACING:
        return "gcSL_FRONT_FACING";

    case gcSL_POINT_COORD:
        return "gcSL_POINT_COORD";
    }

    /* Not a predefined name. */
    return gcvNULL;
}

/*******************************************************************************
**  gcATTRIBUTE_GetName
**
**  Get the name of a gcATTRIBUTE object.
**
**  INPUT:
**
**      gcATTRIBUTE Attribute
**          Pointer to a gcATTRIBUTE object.
**
**  OUTPUT:
**
**      gctSIZE_T * Length
**          Pointer to a variable receiving the length of the attribute name.
**          'Length' can be gcvNULL, in which case no length is returned.
**
**      gctCONST_STRING * Name
**          Pointer to a variable receiving the pointer to the name of the
**          attribute.  'Name' can be gcvNULL in which case no name is returned.
*/
gceSTATUS
gcATTRIBUTE_GetName(
    IN gcATTRIBUTE Attribute,
    OUT gctSIZE_T * Length,
    OUT gctCONST_STRING * Name
    )
{
    gctSIZE_T length;
    gctCONST_STRING name;

    gcmHEADER_ARG("Attribute=0x%x Length=0x%x Name=0x%x", Attribute, Length, Name);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Attribute, gcvOBJ_ATTRIBUTE);

    /* Convert predefined names into ASCII. */
    name = _PredefinedName(Attribute->nameLength);

    if (name == gcvNULL)
    {
        /* Not a predefined name. */
        length = Attribute->nameLength;
        name   = Attribute->name;
    }
    else
    {
        /* Compute length of predefined name. */
        gcmVERIFY_OK(gcoOS_StrLen(name, &length));
    }

    if (Length != gcvNULL)
    {
        /* Return length of name. */
        *Length = length;
    }

    if (Name != gcvNULL)
    {
        /* Return pointer to name. */
        *Name = name;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcATTRIBUTE_IsEnabled
**
**  Query the enabled state of a gcATTRIBUTE object.
**
**  INPUT:
**
**      gcATTRIBUTE Attribute
**          Pointer to a gcATTRIBUTE object.
**
**  OUTPUT:
**
**      gctBOOL * Enabled
**          Pointer to a variable receiving the enabled state of the attribute.
*/
gceSTATUS
gcATTRIBUTE_IsEnabled(
    IN gcATTRIBUTE Attribute,
    OUT gctBOOL * Enabled
    )
{
    gcmHEADER_ARG("Attribute=0x%x Enabled=0x%x", Attribute, Enabled);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Attribute, gcvOBJ_ATTRIBUTE);

    if (Enabled != gcvNULL)
    {
        /* Return enabled state. */
        *Enabled = Attribute->enabled;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcUNIFORM_GetType
**
**  Get the type and array length of a gcUNIFORM object.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**  OUTPUT:
**
**      gcSHADER_TYPE * Type
**          Pointer to a variable receiving the type of the uniform.  'Type' can
**          be gcvNULL, in which case no type will be returned.
**
**      gctSIZE_T * ArrayLength
**          Pointer to a variable receiving the length of the array if the
**          uniform was declared as an array.  If the uniform was not declared
**          as an array, the array length will be 1.  'ArrayLength' can be gcvNULL,
**          in which case no array length will be returned.
*/
gceSTATUS
gcUNIFORM_GetType(
    IN gcUNIFORM Uniform,
    OUT gcSHADER_TYPE * Type,
    OUT gctSIZE_T * ArrayLength
    )
{
    gcmHEADER_ARG("Uniform=0x%x Type=%d, ArrayLength", Uniform, Type, ArrayLength);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    if (Type != gcvNULL)
    {
        /* Return uniform type. */
        *Type = Uniform->type;
    }

    if (ArrayLength != gcvNULL)
    {
        /* Return uniform array length. */
        *ArrayLength = Uniform->arraySize;
    }

    /* Success. */
    gcmFOOTER_ARG("*Type=%d *ArrayLength=%lu",
                  gcmOPT_VALUE(Type), gcmOPT_VALUE(ArrayLength));

    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcUNIFORM_GetTypeEx
**
**  Get the type and array length of a gcUNIFORM object.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**  OUTPUT:
**
**      gcSHADER_TYPE * Type
**          Pointer to a variable receiving the type of the uniform.  'Type' can
**          be gcvNULL, in which case no type will be returned.
**
**		gcSHADER_PRECISION * Precision
**			Pointer to a variable receiving the precision of the uniform.  'Precision' can
**			be gcvNULL, in which case no type will be returned.
**
**      gctSIZE_T * ArrayLength
**          Pointer to a variable receiving the length of the array if the
**          uniform was declared as an array.  If the uniform was not declared
**          as an array, the array length will be 1.  'ArrayLength' can be gcvNULL,
**          in which case no array length will be returned.
*/
gceSTATUS
gcUNIFORM_GetTypeEx(
    IN gcUNIFORM Uniform,
    OUT gcSHADER_TYPE * Type,
    OUT gcSHADER_PRECISION * Precision,
    OUT gctSIZE_T * ArrayLength
    )
{
    gcmHEADER_ARG("Uniform=0x%x Type=%d, ArrayLength", Uniform, Type, ArrayLength);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    if (Type != gcvNULL)
    {
        /* Return uniform type. */
        *Type = Uniform->type;
    }

    if (Precision != gcvNULL)
    {
        *Precision = Uniform->precision;
    }

    if (ArrayLength != gcvNULL)
    {
        /* Return uniform array length. */
        *ArrayLength = Uniform->arraySize;
    }

    /* Success. */
    gcmFOOTER_ARG("*Type=%d *ArrayLength=%lu",
                  gcmOPT_VALUE(Type), gcmOPT_VALUE(ArrayLength));

    return gcvSTATUS_OK;
}

/*******************************************************************************
**                              gcUNIFORM_GetFlags
********************************************************************************
**
**    Get the type and array length of a gcUNIFORM object.
**
**    INPUT:
**
**        gcUNIFORM Uniform
**            Pointer to a gcUNIFORM object.
**
**    OUTPUT:
**
**        gceUNIFORM_FLAGS * Flags
**            Pointer to a variable receiving the flags of the uniform.
**
*/
gceSTATUS
gcUNIFORM_GetFlags(
    IN gcUNIFORM Uniform,
    OUT gceUNIFORM_FLAGS * Flags
    )
{
    gcmHEADER_ARG("Uniform=0x%x Flags=0x%x", Uniform, Flags);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    if (Flags != gcvNULL)
    {
        /* Return uniform flags. */
        *Flags = Uniform->flags;
        gcmFOOTER_ARG("*Flags=%d", *Flags);
    }
    else
    {
        gcmFOOTER_NO();
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**                              gcUNIFORM_SetFlags
********************************************************************************
**
**    Set the flags of a gcUNIFORM object.
**
**    INPUT:
**
**        gcUNIFORM Uniform
**            Pointer to a gcUNIFORM object.
**
**        gceUNIFORM_FLAGS Flags
**            Flags of the uniform to be set.
**
**    OUTPUT:
**            Nothing.
**
*/
gceSTATUS
gcUNIFORM_SetFlags(
    IN gcUNIFORM Uniform,
    IN gceUNIFORM_FLAGS Flags
    )
{
    gcmHEADER_ARG("Uniform=0x%x Flags=0x%x", Uniform, Flags);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    Uniform->flags = Flags;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcUNIFORM_GetName
**
**  Get the name of a gcUNIFORM object.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**  OUTPUT:
**
**      gctSIZE_T * Length
**          Pointer to a variable receiving the length of the uniform name.
**          'Length' can be gcvNULL, in which case no length will be returned.
**
**      gctCONST_STRING * Name
**          Pointer to a variable receiving the pointer to the uniform name.
**          'Name' can be gcvNULL, in which case no name will be returned.
*/
gceSTATUS
gcUNIFORM_GetName(
    IN gcUNIFORM Uniform,
    OUT gctSIZE_T * Length,
    OUT gctCONST_STRING * Name
    )
{
    gcmHEADER_ARG("Uniform=0x%x Length=0x%x Name=0x%x", Uniform, Length, Name);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    if (Length != gcvNULL)
    {
        /* Return length of name. */
        *Length = Uniform->nameLength;
    }

    if (Name != gcvNULL)
    {
        /* Return pointer to name. */
        *Name = Uniform->name;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcUNIFORM_GetSampler
**
**  Get the physical sampler number for a sampler gcUNIFORM object.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**  OUTPUT:
**
**      gctUINT32 * Sampler
**          Pointer to a variable receiving the physical sampler.
*/
gceSTATUS
gcUNIFORM_GetSampler(
    IN gcUNIFORM Uniform,
    OUT gctUINT32 * Sampler
    )
{
    gcmHEADER_ARG("Uniform=0x%x Sampler=0x%x", Uniform, Sampler);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    /* Make sure the uniform is a sampler. */
    if ( (Uniform->type != gcSHADER_SAMPLER_1D) &&
         (Uniform->type != gcSHADER_SAMPLER_2D) &&
         (Uniform->type != gcSHADER_SAMPLER_3D) &&
         (Uniform->type != gcSHADER_SAMPLER_CUBIC) &&
         (Uniform->type != gcSHADER_ISAMPLER_2D) &&
         (Uniform->type != gcSHADER_ISAMPLER_3D) &&
         (Uniform->type != gcSHADER_ISAMPLER_CUBIC) &&
         (Uniform->type != gcSHADER_USAMPLER_2D) &&
         (Uniform->type != gcSHADER_USAMPLER_3D) &&
         (Uniform->type != gcSHADER_USAMPLER_CUBIC) &&
         (Uniform->type != gcSHADER_SAMPLER_EXTERNAL_OES))
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_GLOBAL_TYPE_MISMATCH);
        return gcvSTATUS_GLOBAL_TYPE_MISMATCH;
    }

    if (Sampler != gcvNULL)
    {
        /* Return physical sampler. */
        *Sampler = Uniform->physical;
    }

    gcmFOOTER_ARG("*Sampler=%d", gcmOPT_VALUE(Sampler));

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcUNIFORM_GetFormat
**
**  Get the type and array length of a gcUNIFORM object.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**  OUTPUT:
**
**      gcSL_FORMAT * Format
**          Pointer to a variable receiving the format of element of the uniform.
**          'Type' can be gcvNULL, in which case no type will be returned.
**
**      gctBOOL * IsPointer
**          Pointer to a variable receiving the state wheter the uniform is a pointer.
**          'IsPointer' can be gcvNULL, in which case no array length will be returned.
*/
gceSTATUS
gcUNIFORM_GetFormat(
    IN gcUNIFORM Uniform,
    OUT gcSL_FORMAT * Format,
    OUT gctBOOL * IsPointer
    )
{
    gcmHEADER_ARG("Uniform=0x%x Format=0x%x IsPointer=0x%x", Uniform, Format, IsPointer);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    if (Format != gcvNULL)
    {
        /* Return uniform format. */
        *Format = Uniform->format;
    }

    if (IsPointer != gcvNULL)
    {
        /* Return uniform pointer designation */
        *IsPointer = Uniform->isPointer;
        gcmFOOTER_ARG("*Format=%d *IsPointer=%d", Uniform->format, Uniform->isPointer);
    }
    else
    {
        gcmFOOTER_NO();
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcUNIFORM_SetFormat
**
**  Set the format and isPointer of a uniform.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gcSL_FORMAT Format
**          Format of element of the uniform shaderType.
**
**      gctBOOL IsPointer
**          Wheter the uniform is a pointer.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcUNIFORM_SetFormat(
    IN gcUNIFORM Uniform,
    IN gcSL_FORMAT Format,
    IN gctBOOL IsPointer
    )
{
    gcmHEADER_ARG("Uniform=0x%x Format=%d IsPointer=%d",
                  Uniform, Format, IsPointer);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);

    Uniform->format = Format;
    Uniform->isPointer = IsPointer;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcUNIFORM_SetValue
**
**  Set the value of a uniform in integer.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gctSIZE_T Count
**          Number of entries to program if the uniform has been declared as an
**          array.
**
**      const gctINT * Value
**          Pointer to a buffer holding the integer values for the uniform.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcUNIFORM_SetValue(
    IN gcUNIFORM Uniform,
    IN gctSIZE_T Count,
    IN const gctINT * Value
    )
{
#if gcdNULL_DRIVER < 2
    gceSTATUS status;
    gctSIZE_T columns, rows;

    gcmHEADER_ARG("Uniform=0x%x Count=%lu", Uniform, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);
    gcmDEBUG_VERIFY_ARGUMENT(Count > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Value != gcvNULL);

    rows = gcmMIN((gctINT) Count, Uniform->arraySize);

    /* Dispatch on uniform type. */
    switch (Uniform->type)
    {
    case gcSHADER_INTEGER_X1:
    case gcSHADER_BOOLEAN_X1:
        /* Value. */
        columns = 1;
        break;

    case gcSHADER_INTEGER_X2:
    case gcSHADER_BOOLEAN_X2:
        /* 2-component vector. */
        columns = 2;
        break;

    case gcSHADER_INTEGER_X3:
    case gcSHADER_BOOLEAN_X3:
        /* 3-component vector. */
        columns = 3;
        break;

    case gcSHADER_INTEGER_X4:
    case gcSHADER_BOOLEAN_X4:
        /* 4-component vector. */
        columns = 4;
        break;

    case gcSHADER_IMAGE_2D:
    case gcSHADER_IMAGE_3D:
    case gcSHADER_SAMPLER:
        /* Value. */
        columns = 1;
        break;

    default:
        gcmBREAK();
        columns = 0;
    }

    /* Program the uniform. */
    status = gcoHARDWARE_ProgramUniform(Uniform->address,
                                        columns, rows,
                                        (gctPOINTER) Value,
                                        gcvFALSE);

    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}

/*******************************************************************************
**  gcUNIFORM_SetValueX
**
**  Set the value of a uniform in fixed point.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gctSIZE_T Count
**          Number of entries to program if the uniform has been declared as an
**          array.
**
**      const gctFIXED_POINT * Value
**          Pointer to a buffer holding the fixed point values for the uniform.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcUNIFORM_SetValueX(
    IN gcUNIFORM Uniform,
    IN gctSIZE_T Count,
    IN gctFIXED_POINT * Value
    )
{
#if gcdNULL_DRIVER < 2
    gceSTATUS status;
    gctSIZE_T columns, rows;

    gcmHEADER_ARG("Uniform=0x%x Count=%lu Value=0x%x", Uniform, Count, Value);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);
    gcmDEBUG_VERIFY_ARGUMENT(Count > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Value != gcvNULL);

    rows = gcmMIN((gctINT) Count, Uniform->arraySize);

    /* Dispatch on uniform type. */
    switch (Uniform->type)
    {
    case gcSHADER_FIXED_X1:
        /* Value. */
        columns = 1;
        break;

    case gcSHADER_FIXED_X2:
        /* 2-component vector. */
        columns = 2;
        break;

    case gcSHADER_FIXED_X3:
        /* 3-component vector. */
        columns = 3;
        break;

    case gcSHADER_FIXED_X4:
        /* 4-component vector. */
        columns = 4;
        break;

    default:
        gcmBREAK();
        columns = 0;
    }

    /* Program the uniform. */
    status = gcoHARDWARE_ProgramUniform(Uniform->address,
                                        columns, rows,
                                        (gctPOINTER) Value,
                                        gcvTRUE);

    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}

/*******************************************************************************
**  gcUNIFORM_SetValueF
**
**  Set the value of a uniform in floating point.
**
**  INPUT:
**
**      gcUNIFORM Uniform
**          Pointer to a gcUNIFORM object.
**
**      gctSIZE_T Count
**          Number of entries to program if the uniform has been declared as an
**          array.
**
**      const gctFLOAT * Value
**          Pointer to a buffer holding the floating point values for the
**          uniform.
**
**  OUTPUT:
**
**      Nothing.
*/
gceSTATUS
gcUNIFORM_SetValueF(
    IN gcUNIFORM Uniform,
    IN gctSIZE_T Count,
    IN const gctFLOAT * Value
    )
{
#if gcdNULL_DRIVER < 2
    gceSTATUS status;
    gctSIZE_T columns, rows;

    gcmHEADER_ARG("Uniform=0x%x Count=%lu Value=0x%x", Uniform, Count, Value);

    /* Verify the argiments. */
    gcmVERIFY_OBJECT(Uniform, gcvOBJ_UNIFORM);
    gcmDEBUG_VERIFY_ARGUMENT(Count > 0);
    gcmDEBUG_VERIFY_ARGUMENT(Value != gcvNULL);

    rows = gcmMIN((gctINT) Count, Uniform->arraySize);

    /* Dispatch on uniform type. */
    switch (Uniform->type)
    {
    case gcSHADER_FLOAT_X4:
        /* 4-component vector. */
        columns = 4;
        break;

    case gcSHADER_FLOAT_4X4:
        /* 4 x 4 matrix. */
        columns = 4;
        rows   *= 4;
        break;

    case gcSHADER_FLOAT_X1:
        /* Value. */
        columns = 1;
        break;

    case gcSHADER_FLOAT_X2:
        /* 2-component vector. */
        columns = 2;
        break;

    case gcSHADER_FLOAT_X3:
        /* 3-component vector. */
        columns = 3;
        break;

    case gcSHADER_FLOAT_2X2:
        /* 2 x 2 matrix. */
        columns = 2;
        rows   *= 2;
        break;

    case gcSHADER_FLOAT_3X3:
        /* 3 x 3 matrix. */
        columns = 3;
        rows   *= 3;
        break;

    default:
        gcmBREAK();
        columns = 0;
    }

    /* Program the uniform. */
    status = gcoHARDWARE_ProgramUniform(Uniform->address,
                                        columns, rows,
                                        (gctPOINTER) Value,
                                        gcvFALSE);

    gcmFOOTER();
    return status;
#else
    return gcvSTATUS_OK;
#endif
}

/*******************************************************************************
**  gcOUTPUT_GetType
**
**  Get the type and array length of a gcOUTPUT object.
**
**  INPUT:
**
**      gcOUTPUT Output
**          Pointer to a gcOUTPUT object.
**
**  OUTPUT:
**
**      gcSHADER_TYPE * Type
**          Pointer to a variable receiving the type of the output.  'Type' can
**          be gcvNULL, in which case no type will be returned.
**
**      gctSIZE_T * ArrayLength
**          Pointer to a variable receiving the length of the array if the
**          output was declared as an array.  If the output was not declared
**          as an array, the array length will be 1.  'ArrayLength' can be gcvNULL,
**          in which case no array length will be returned.
*/
gceSTATUS
gcOUTPUT_GetType(
    IN gcOUTPUT Output,
    OUT gcSHADER_TYPE * Type,
    OUT gctSIZE_T * ArrayLength
    )
{
    gcmHEADER_ARG("Output=0x%x", Output);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Output, gcvOBJ_OUTPUT);

    if (Type != gcvNULL)
    {
        /* Return output type. */
        *Type = Output->type;
        gcmFOOTER_ARG("*Type=%d", gcmOPT_VALUE(Type));
    }
    else
    {
        gcmFOOTER_NO();
    }

    if (ArrayLength != gcvNULL)
    {
        /* Return output array length. */
        *ArrayLength = Output->arraySize;
        gcmFOOTER_ARG("*ArrayLength=%lu", gcmOPT_VALUE(ArrayLength));
    }
    else
    {
        gcmFOOTER_NO();
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcOUTPUT_GetIndex
**
**  Get the index of a gcOUTPUT object.
**
**  INPUT:
**
**      gcOUTPUT Output
**          Pointer to a gcOUTPUT object.
**
**  OUTPUT:
**
**      gctUINT * Index
**          Pointer to a variable receiving the temporary register index of the
**          output.  'Index' can be gcvNULL,. in which case no index will be
**          returned.
*/
gceSTATUS
gcOUTPUT_GetIndex(
    IN gcOUTPUT Output,
    OUT gctUINT * Index
    )
{
    gcmHEADER_ARG("Output=0x%x", Output);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Output, gcvOBJ_OUTPUT);

    if (Index != gcvNULL)
    {
        /* Return output temporary register index. */
        *Index = Output->tempIndex;
    }

    gcmFOOTER_ARG("*Index=%u", gcmOPT_VALUE(Index));
    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcOUTPUT_GetName
**
**  Get the name of a gcOUTPUT object.
**
**  INPUT:
**
**      gcOUTPUT Output
**          Pointer to a gcOUTPUT object.
**
**  OUTPUT:
**
**      gctSIZE_T * Length
**          Pointer to a variable receiving the length of the output name.
**          'Length' can be gcvNULL, in which case no length will be returned.
**
**      gctCONST_STRING * Name
**          Pointer to a variable receiving the pointer to the output name.
**          'Name' can be gcvNULL, in which case no name will be returned.
*/
gceSTATUS
gcOUTPUT_GetName(
    IN gcOUTPUT Output,
    OUT gctSIZE_T * Length,
    OUT gctCONST_STRING * Name
    )
{
    gctSIZE_T length;
    gctCONST_STRING name;

    gcmHEADER_ARG("Output=0x%x Length=0x%x Name=0x%x", Output, Length, Name);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Output, gcvOBJ_OUTPUT);

    /* Convert predefined names into ASCII. */
    name = _PredefinedName(Output->nameLength);

    if (name == gcvNULL)
    {
        /* Not a predefined name. */
        length = Output->nameLength;
        name   = Output->name;
    }
    else
    {
        /* Compute length of predefined name. */
        gcmVERIFY_OK(gcoOS_StrLen(name, &length));
    }

    if (Length != gcvNULL)
    {
        /* Return length of name. */
        *Length = length;
    }

    if (Name != gcvNULL)
    {
        /* Return pointer to name. */
        *Name = name;
    }

    /* Success. */
    gcmFOOTER_ARG("*Length=%lu *Name=0x%x",
                  gcmOPT_VALUE(Length), gcmOPT_POINTER(Name));
    return gcvSTATUS_OK;
}

/*******************************************************************************
*********************************************************** F U N C T I O N S **
*******************************************************************************/

/*******************************************************************************
**  gcSHADER_ReallocateFunctions
**
**  Reallocate an array of pointers to gcFUNCTION objects.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcSHADER_ReallocateFunctions(
    IN gcSHADER Shader,
    IN gctSIZE_T Count
    )
{
    gcFUNCTION * functions;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Count=%lu", Shader, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Count < Shader->functionCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == Shader->functionArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcFUNCTION) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    functions = pointer;

    if (Shader->functions != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(functions,
                                   Shader->functions,
                                   gcmSIZEOF(gcFUNCTION)
                                   * Shader->functionCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->functions));
    }

    /* Set new gcFUNCTION object pointer. */
    Shader->functionArrayCount = Count;
    Shader->functions          = functions;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

/*******************************************************************************
*********************************************K E R N E L    F U N C T I O N S **
*******************************************************************************/

/*******************************************************************************
**  gcSHADER_ReallocateKernelFunctions
**
**  Reallocate an array of pointers to gcKERNEL_FUNCTION objects.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcSHADER_ReallocateKernelFunctions(
    IN gcSHADER Shader,
    IN gctSIZE_T Count
    )
{
    gcKERNEL_FUNCTION * kernelFunctions;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Count=%lu", Shader, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    if (Count < Shader->kernelFunctionCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == Shader->kernelFunctionArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcKERNEL_FUNCTION) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    kernelFunctions = pointer;

    if (Shader->kernelFunctions != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(kernelFunctions,
                                   Shader->kernelFunctions,
                                   gcmSIZEOF(gcKERNEL_FUNCTION)
                                   * Shader->kernelFunctionCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->kernelFunctions));
    }

    /* Set new gcFUNCTION object pointer. */
    Shader->kernelFunctionArrayCount = Count;
    Shader->kernelFunctions          = kernelFunctions;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

gceSTATUS
gcSHADER_AddFunction(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,
    OUT gcFUNCTION * Function
    )
{
    gceSTATUS status;
    gctSIZE_T length = 0;
    gctSIZE_T bytes;
    gcFUNCTION function = gcvNULL;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Name=%s", Shader, Name);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Name != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(Function != gcvNULL);

    /* Check array count. */
    if (Shader->functionArrayCount <= Shader->functionCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcSHADER_ReallocateFunctions(Shader, Shader->functionCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Get the length of the name. */
    gcmVERIFY_OK(gcoOS_StrLen(Name, &length));

    /* Compute the number of bytes required for the gcFUNCTION object. */
    bytes = gcmOFFSETOF(_gcsFUNCTION, name) + length + 1;

    /* Allocate the gcFUNCTION object. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    function = pointer;

    /* Initialize the gcFUNCTION object. */
    function->object.type        = gcvOBJ_FUNCTION;

    function->argumentArrayCount = 0;
    function->argumentCount      = 0;
    function->arguments          = gcvNULL;

    function->variableCount      = 0;
    function->variables          = gcvNULL;

    function->label              = (gctUINT16) (~0 - Shader->kernelFunctionCount - Shader->functionCount);

    function->codeStart          = 0;
    function->codeCount          = 0;

    /* Copy the function name. */
    function->nameLength         = length;
    gcmVERIFY_OK(gcoOS_MemCopy(function->name, Name, length + 1));

    /* Set new gcFUNCTION object pointer. */
    Shader->functions[Shader->functionCount++] = function;

    /* Return the gcFUNCTION object pointer. */
    *Function = function;

    /* Success. */
    gcmFOOTER_ARG("*Function=0x%x", *Function);
    return gcvSTATUS_OK;
}

gceSTATUS
gcSHADER_AddKernelFunction(
    IN gcSHADER Shader,
    IN gctCONST_STRING Name,

    OUT gcKERNEL_FUNCTION * KernelFunction
    )
{
    gceSTATUS status;
    gctSIZE_T length = 0;
    gctSIZE_T bytes;
    gcKERNEL_FUNCTION kernelFunction = gcvNULL;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x Name=%s", Shader, Name);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmDEBUG_VERIFY_ARGUMENT(Name != gcvNULL);
    gcmDEBUG_VERIFY_ARGUMENT(KernelFunction != gcvNULL);

    /* Check array count. */
    if (Shader->kernelFunctionArrayCount <= Shader->kernelFunctionCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcSHADER_ReallocateKernelFunctions(Shader, Shader->kernelFunctionCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Get the length of the name. */
    gcmVERIFY_OK(gcoOS_StrLen(Name, &length));

    /* Compute the number of bytes required for the gcKERNEL_FUNCTION object. */
    bytes = gcmOFFSETOF(_gcsKERNEL_FUNCTION, name) + length + 1;

    /* Allocate the gcKERNEL_FUNCTION object. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    kernelFunction = pointer;

    /* Initialize the gcKERNEL_FUNCTION object. */
    kernelFunction->object.type = gcvOBJ_KERNEL_FUNCTION;

    kernelFunction->shader = Shader;

    kernelFunction->argumentArrayCount = 0;
    kernelFunction->argumentCount      = 0;
    kernelFunction->arguments          = gcvNULL;
    kernelFunction->variableCount      = 0;
    kernelFunction->variables          = gcvNULL;

    kernelFunction->localMemorySize = 0;

    /* Uniform args to kernel function */
    kernelFunction->uniformArgumentArrayCount = 0;
    kernelFunction->uniformArgumentCount      = 0;
    kernelFunction->uniformArguments          = gcvNULL;
    kernelFunction->samplerIndex = 0;

    /* Image-Sampler associations */
    kernelFunction->imageSamplerArrayCount = 0;
    kernelFunction->imageSamplerCount      = 0;
    kernelFunction->imageSamplers          = gcvNULL;

    /*kernel function property */
    kernelFunction->propertyArrayCount = 0;
    kernelFunction->propertyCount      = 0;
    kernelFunction->properties         = gcvNULL;
    kernelFunction->propertyValueArrayCount = 0;
    kernelFunction->propertyValueCount      = 0;
    kernelFunction->propertyValues          = gcvNULL;

    kernelFunction->label              = (gctUINT16) (~0 - Shader->kernelFunctionCount - Shader->functionCount);

    kernelFunction->codeStart = 0;
    kernelFunction->codeCount = 0;
    kernelFunction->codeEnd   = 0;

    kernelFunction->isMain    = gcvFALSE;

    /* Copy the function name. */
    kernelFunction->nameLength         = length;
    gcmVERIFY_OK(gcoOS_MemCopy(kernelFunction->name, Name, length + 1));

    /* Set new gcFUNCTION object pointer. */
    Shader->kernelFunctions[Shader->kernelFunctionCount++] = kernelFunction;

    /* Return the gcFUNCTION object pointer. */
    *KernelFunction = kernelFunction;

    /* Success. */
    gcmFOOTER_ARG("*KernelFunction=0x%x", *KernelFunction);
    return gcvSTATUS_OK;
}

gceSTATUS
gcSHADER_BeginFunction(
    IN gcSHADER Shader,
    IN gcFUNCTION Function
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Function=0x%x", Shader, Function);

    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(Function, gcvOBJ_FUNCTION);

    Shader->currentFunction = Function;

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        Shader->instrIndex       = gcSHADER_OPCODE;
        Shader->lastInstruction += 1;
    }

    Function->codeStart = Shader->lastInstruction;

    status = gcSHADER_AddLabel(Shader, Function->label);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcSHADER_EndFunction(
    IN gcSHADER Shader,
    IN gcFUNCTION Function
    )
{
    gcmHEADER_ARG("Shader=0x%x Function=0x%x", Shader, Function);

    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(Function, gcvOBJ_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Shader->currentFunction == Function);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        Shader->instrIndex       = gcSHADER_OPCODE;
        Shader->lastInstruction += 1;
    }

    Function->codeCount = Shader->lastInstruction - Function->codeStart;

    Shader->currentFunction = gcvNULL;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcSHADER_BeginKernelFunction(
    IN gcSHADER Shader,
    IN gcKERNEL_FUNCTION KernelFunction
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x KernelFunction=0x%x", Shader, KernelFunction);

    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    Shader->currentKernelFunction = KernelFunction;

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        Shader->instrIndex       = gcSHADER_OPCODE;
        Shader->lastInstruction += 1;
    }

    KernelFunction->codeStart = Shader->lastInstruction;

    status = gcSHADER_AddLabel(Shader, KernelFunction->label);

    gcmFOOTER();
    return status;
}

gceSTATUS
gcSHADER_EndKernelFunction(
    IN gcSHADER Shader,
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctSIZE_T LocalMemorySize
    )
{
    gcmHEADER_ARG("Shader=0x%x KernelFunction=0x%x LocalMemorySize=%u",
                  Shader, KernelFunction, LocalMemorySize);

    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Shader->currentKernelFunction == KernelFunction);

    if (Shader->instrIndex != gcSHADER_OPCODE)
    {
        Shader->instrIndex       = gcSHADER_OPCODE;
        Shader->lastInstruction += 1;
    }

    KernelFunction->codeCount = Shader->lastInstruction - KernelFunction->codeStart;
    KernelFunction->localMemorySize = LocalMemorySize;

    Shader->currentKernelFunction = gcvNULL;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcFUNCTION_ReallocateArguments
**
**  Reallocate an array of gcsFUNCTION_ARGUMENT objects.
**
**  INPUT:
**
**      gcFUNCTION Function
**          Pointer to a gcFUNCTION object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcFUNCTION_ReallocateArguments(
    IN gcFUNCTION Function,
    IN gctSIZE_T Count
    )
{
    gcsFUNCTION_ARGUMENT_PTR arguments;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Function=0x%x Count=%lu", Function, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Function, gcvOBJ_FUNCTION);

    if (Count < Function->argumentCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == Function->argumentArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcsFUNCTION_ARGUMENT) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    arguments = pointer;

    if (Function->arguments != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(arguments,
                                   Function->arguments,
                                   gcmSIZEOF(gcsFUNCTION_ARGUMENT)
                                   * Function->argumentCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Function->arguments));
    }

    /* Set new gcFUNCTION object pointer. */
    Function->argumentArrayCount = Count;
    Function->arguments          = arguments;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

gceSTATUS
gcFUNCTION_AddArgument(
    IN gcFUNCTION Function,
    IN gctUINT16 TempIndex,
    IN gctUINT8 Enable,
    IN gctUINT8 Qualifier
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Function=0x%x TempIndex=%u Enable=%u Qualifier=%u",
                  Function, TempIndex, Enable, Qualifier);

    gcmVERIFY_OBJECT(Function, gcvOBJ_FUNCTION);

    if (Function->argumentArrayCount <= Function->argumentCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcFUNCTION_ReallocateArguments(Function, Function->argumentCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    Function->arguments[Function->argumentCount].index     = TempIndex;
    Function->arguments[Function->argumentCount].enable    = Enable;
    Function->arguments[Function->argumentCount].qualifier = Qualifier;

    Function->argumentCount++;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcFUNCTION_GetArgument(
    IN gcFUNCTION Function,
    IN gctUINT16 Index,
    OUT gctUINT16_PTR Temp,
    OUT gctUINT8_PTR Enable,
    OUT gctUINT8_PTR Swizzle
    )
{
    gcmHEADER_ARG("Function=0x%x Index=%u", Function, Index);

    gcmVERIFY_OBJECT(Function, gcvOBJ_FUNCTION);

    if (Index >= Function->argumentCount)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Temp != gcvNULL)
    {
        *Temp = Function->arguments[Index].index;
    }

    if (Enable != gcvNULL)
    {
        *Enable = Function->arguments[Index].enable;
    }

    if (Swizzle != gcvNULL)
    {
        switch (Function->arguments[Index].enable)
        {
        case 0x1: *Swizzle = gcSL_SWIZZLE_XXXX; break;
        case 0x2: *Swizzle = gcSL_SWIZZLE_YYYY; break;
        case 0x4: *Swizzle = gcSL_SWIZZLE_ZZZZ; break;
        case 0x8: *Swizzle = gcSL_SWIZZLE_WWWW; break;
        case 0x3: *Swizzle = gcSL_SWIZZLE_XYYY; break;
        case 0x6: *Swizzle = gcSL_SWIZZLE_YZZZ; break;
        case 0xC: *Swizzle = gcSL_SWIZZLE_ZWWW; break;
        case 0x7: *Swizzle = gcSL_SWIZZLE_XYZZ; break;
        case 0xE: *Swizzle = gcSL_SWIZZLE_YZWW; break;
        case 0xF: *Swizzle = gcSL_SWIZZLE_XYZW; break;
        default:  gcmFATAL("Oops!");
        }
    }

    if ((Temp != gcvNULL) && (Enable != gcvNULL) && (Swizzle != gcvNULL))
    {
        gcmFOOTER_ARG("*Temp=%u *Enable=%u *Swizzle=%u",
                      gcmOPT_VALUE(Temp), gcmOPT_VALUE(Enable),
                      gcmOPT_VALUE(Swizzle));
    }
    else
    {
        gcmFOOTER_NO();
    }
    return gcvSTATUS_OK;
}

gceSTATUS
gcFUNCTION_GetLabel(
    IN gcFUNCTION Function,
    OUT gctUINT_PTR Label
    )
{
    gcmHEADER_ARG("Function=0x%x", Function);

    gcmVERIFY_OBJECT(Function, gcvOBJ_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Label != gcvNULL);

    *Label = Function->label;

    gcmFOOTER_ARG("*Label=%u", *Label);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_GetName
**
**  Get the name of a gcKERNEL_FUNCTION object.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**  OUTPUT:
**
**      gctSIZE_T * Length
**          Pointer to a variable receiving the length of the kernel function name.
**          'Length' can be gcvNULL, in which case no length will be returned.
**
**      gctCONST_STRING * Name
**          Pointer to a variable receiving the pointer to the kernel function name.
**          'Name' can be gcvNULL, in which case no name will be returned.
*/
gceSTATUS
gcKERNEL_FUNCTION_GetName(
    IN gcKERNEL_FUNCTION KernelFunction,
    OUT gctSIZE_T * Length,
    OUT gctCONST_STRING * Name
    )
{
    gcmHEADER_ARG("KernelFunction=0x%x Length=0x%x Name=0x%x", KernelFunction, Length, Name);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    if (Length != gcvNULL)
    {
        /* Return length of name. */
        *Length = KernelFunction->nameLength;
    }

    if (Name != gcvNULL)
    {
        /* Return pointer to name. */
        *Name = KernelFunction->name;
    }

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_ReallocateUniformArguments
**
**  Reallocate an array of pointers to gcUNIFORM objects.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcKERNEL_FUNCTION_ReallocateUniformArguments(
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctSIZE_T Count
    )
{
    gcUNIFORM * uniformArguments;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("KernelFunction=0x%x Count=%lu", KernelFunction, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    if (Count < KernelFunction->uniformArgumentCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == KernelFunction->uniformArgumentArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcUNIFORM) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    uniformArguments = pointer;

    if (KernelFunction->uniformArguments != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(uniformArguments,
                                   KernelFunction->uniformArguments,
                                   gcmSIZEOF(gcUNIFORM)
                                   * KernelFunction->uniformArgumentCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, KernelFunction->uniformArguments));
    }

    /* Set new gcUNIFORM object pointer. */
    KernelFunction->uniformArgumentArrayCount = Count;
    KernelFunction->uniformArguments          = uniformArguments;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_AddUniformArgument
**
**  Add an uniform arugment to a gcKERNEL_FUNCTION object.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**      gctCONST_STRING Name
**          Name of the uniform to add.
**
**      gcSHADER_TYPE Type
**          Type of the uniform to add.
**
**      gctSIZE_T Length
**          Array length of the uniform to add.  'Length' must be at least 1.
**
**  OUTPUT:
**
**      gcUNIFORM * UniformArgument
**          Pointer to a variable receiving the gcUNIFORM object pointer.
*/
gceSTATUS
gcKERNEL_FUNCTION_AddUniformArgument(
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctCONST_STRING Name,
    IN gcSHADER_TYPE Type,
    IN gctSIZE_T Length,
    OUT gcUNIFORM * UniformArgument
    )
{
    gctSIZE_T nameLength=0, bytes;
    gcUNIFORM uniformArgument;
    gceSTATUS status;
    gctPOINTER pointer;

    gcmHEADER_ARG("KernelFunction=0x%x Name=%s Type=%d Length=%lu",
                  KernelFunction, Name, Type, Length);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    /* Check array count. */
    if (KernelFunction->uniformArgumentArrayCount <= KernelFunction->uniformArgumentCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcKERNEL_FUNCTION_ReallocateUniformArguments(KernelFunction,
                                                              KernelFunction->uniformArgumentCount + 10);
        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    /* Get the length of the name. */
    gcmVERIFY_OK(gcoOS_StrLen(Name, &nameLength));

    /* Compute the number of bytes required for the gcUNIFORM object. */
    bytes = gcmOFFSETOF(_gcUNIFORM, name) + nameLength + 1;

    /* Allocate the gcUNIFORM object. */
    status = gcoOS_Allocate(gcvNULL, bytes, &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    uniformArgument = pointer;

    /* Initialize the gcUNIFORM object. */
    uniformArgument->object.type  = gcvOBJ_UNIFORM;

    uniformArgument->index        = (gctUINT16) KernelFunction->uniformArgumentCount;
    uniformArgument->type         = Type;
    uniformArgument->arraySize    = Length;
    uniformArgument->format       = gcSL_FLOAT;
    uniformArgument->isPointer    = gcvFALSE;
    uniformArgument->nameLength   = nameLength;
    uniformArgument->physical     = -1;
    uniformArgument->address      = ~0U;
#if GC_ENABLE_LOADTIME_OPT
    uniformArgument->flags        = 0;
    uniformArgument->glUniformIndex = (gctUINT16)-1;
#endif /* GC_ENABLE_LOADTIME_OPT */

    switch (Type)
    {
    case gcSHADER_SAMPLER_1D:
    case gcSHADER_SAMPLER_2D:
    case gcSHADER_SAMPLER_3D:
    case gcSHADER_SAMPLER_CUBIC:

    case gcSHADER_ISAMPLER_2D:
    case gcSHADER_ISAMPLER_3D:
    case gcSHADER_ISAMPLER_CUBIC:
    case gcSHADER_USAMPLER_2D:
    case gcSHADER_USAMPLER_3D:
    case gcSHADER_USAMPLER_CUBIC:
    case gcSHADER_SAMPLER_EXTERNAL_OES:
        uniformArgument->physical = KernelFunction->shader->samplerIndex + KernelFunction->samplerIndex++;
        break;

    default:
        break;
    }

    /* Copy the uniform name. */
    gcmVERIFY_OK(gcoOS_MemCopy(uniformArgument->name, Name, nameLength + 1));

    /* Set new gcUNIFORM object pointer. */
    KernelFunction->uniformArguments[KernelFunction->uniformArgumentCount++] = uniformArgument;

    if (UniformArgument != gcvNULL)
    {
        /* Return the gcUNIFORM object pointer. */
        *UniformArgument = uniformArgument;
        gcmFOOTER_ARG("*UniformArgument=0x%x", *UniformArgument);
    }
    else
    {
        gcmFOOTER_NO();
    }

    /* Success. */
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_GetUniformArgumentCount
**
**  Get the number of uniform arguments for this kernel function.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**  OUTPUT:
**
**      gctSIZE_T * Count
**          Pointer to a variable receiving the number of uniform arguments.
*/
gceSTATUS
gcKERNEL_FUNCTION_GetUniformArgumentCount(
    IN gcKERNEL_FUNCTION KernelFunction,
    OUT gctSIZE_T * Count
    )
{
    gcmHEADER_ARG("KernelFunction=0x%x", KernelFunction);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    /* Return uniform count. */
    *Count = KernelFunction->uniformArgumentCount;

    /* Success. */
    gcmFOOTER_ARG("*Count=%lu", *Count);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_GetUniformArgument
**
**  Get the gcUNIFORM object pointer for an indexed uniform for this kernel function.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**      gctUINT Index
**          Index of uniform to retreive the name for.
**
**  OUTPUT:
**
**      gcUNIFORM * UniformArgument
**          Pointer to a variable receiving the gcUNIFORM object pointer.
*/
gceSTATUS
gcKERNEL_FUNCTION_GetUniformArgument(
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctUINT Index,
    OUT gcUNIFORM * UniformArgument
    )
{
    gcmHEADER_ARG("KernelFunction=0x%x Index=%u", KernelFunction, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Index < KernelFunction->uniformArgumentCount);
    gcmDEBUG_VERIFY_ARGUMENT(UniformArgument != gcvNULL);

    /* Return the gcUNIFORM object pointer. */
    *UniformArgument = KernelFunction->uniformArguments[Index];

    /* Success. */
    gcmFOOTER_ARG("*UniformArgument=0x%x", *UniformArgument);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_ReallocateImageSamplers
**
**  Reallocate an array of pointers to image sampler pair.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcKERNEL_FUNCTION_ReallocateImageSamplers(
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctSIZE_T Count
    )
{
    gcsIMAGE_SAMPLER_PTR imageSamplers;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("KernelFunction=0x%x Count=%lu", KernelFunction, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    if (Count < KernelFunction->imageSamplerCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == KernelFunction->imageSamplerArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of image sampler pairs. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcsIMAGE_SAMPLER) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    imageSamplers = pointer;

    if (KernelFunction->imageSamplers != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(imageSamplers,
                                   KernelFunction->imageSamplers,
                                   gcmSIZEOF(gcsIMAGE_SAMPLER)
                                   * KernelFunction->imageSamplerCount));

        /* Free the current array of image sampler pairs. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, KernelFunction->imageSamplers));
    }

    /* Set new image sampler pair. */
    KernelFunction->imageSamplerArrayCount = Count;
    KernelFunction->imageSamplers          = imageSamplers;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

gceSTATUS
gcKERNEL_FUNCTION_GetPropertyCount(
    IN gcKERNEL_FUNCTION KernelFunction,
    OUT gctSIZE_T * Count
    )
{
    gcmHEADER_ARG("KernelFunction=0x%x", KernelFunction);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    *Count = KernelFunction->propertyCount;

    /* Success. */
    gcmFOOTER_ARG("*Count=%lu", *Count);
    return gcvSTATUS_OK;
}

gceSTATUS
gcKERNEL_FUNCTION_GetProperty(
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctUINT Index,
    OUT gctSIZE_T * propertySize,
    OUT gctINT * propertyType,
    OUT gctINT * propertyValues
    )
{
    gctSIZE_T i, valueIndex;

    gcmHEADER_ARG("KernelFunction=0x%x Index=%u", KernelFunction, Index);

    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Index < KernelFunction->propertyCount);

    if (propertySize)
        *propertySize = KernelFunction->properties[Index].propertySize;

    if (propertyType)
        *propertyType = KernelFunction->properties[Index].propertyType;


    valueIndex = 0;
    for(i = 0; i < Index; i++)
    {
        valueIndex += KernelFunction->properties[i].propertySize;
    }

    if(propertyValues)
    {
        gcmVERIFY_OK(gcoOS_MemCopy(propertyValues,
            &KernelFunction->propertyValues[valueIndex],
            gcmSIZEOF(gctINT) * KernelFunction->properties[Index].propertySize));
    }

    if ((propertySize != gcvNULL) || (propertyType != gcvNULL) || (propertyValues != gcvNULL))
    {
        /* Success. */
        gcmFOOTER_ARG("*propertySizee=%u *propertyType=%d *propertyValues=%d",
            gcmOPT_VALUE(propertySize), gcmOPT_VALUE(propertyType),
            gcmOPT_VALUE(propertyValues));
    }
    else
    {
        gcmFOOTER_NO();
    }

    return gcvSTATUS_OK;

}

gceSTATUS
gcKERNEL_FUNCTION_ReallocateKernelFunctionProperties(
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctSIZE_T Count,
    IN gctBOOL isPropertyValue
    )
{
    gcsKERNEL_FUNCTION_PROPERTY_PTR properties;
    gctINT_PTR propertyValues;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("KernelFunction=0x%x Count=%lu", KernelFunction, Count);

    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    if(!isPropertyValue)
    {
        if (Count < KernelFunction->propertyCount)
        {
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        if (Count == KernelFunction->propertyArrayCount)
        {
            gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
            return gcvSTATUS_OK;
        }

        status = gcoOS_Allocate(gcvNULL,
            gcmSIZEOF(gcsKERNEL_FUNCTION_PROPERTY) * Count,
            &pointer);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }

        properties = pointer;

        if (KernelFunction->properties != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_MemCopy(properties,
                KernelFunction->properties,
                gcmSIZEOF(gcsKERNEL_FUNCTION_PROPERTY)
                * KernelFunction->propertyCount));

            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, KernelFunction->properties));
        }

        KernelFunction->propertyArrayCount    = Count;
        KernelFunction->properties            = properties;
    }
    else
    {
        if (Count < KernelFunction->propertyValueCount)
        {
            gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
            return gcvSTATUS_INVALID_ARGUMENT;
        }

        if (Count == KernelFunction->propertyValueArrayCount)
        {
            gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
            return gcvSTATUS_OK;
        }

        status = gcoOS_Allocate(gcvNULL,
            gcmSIZEOF(gctINT) * Count,
            &pointer);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }

        propertyValues = pointer;

        if (KernelFunction->propertyValues != gcvNULL)
        {
            gcmVERIFY_OK(gcoOS_MemCopy(propertyValues,
                KernelFunction->propertyValues,
                gcmSIZEOF(gctINT) * KernelFunction->propertyValueCount));

            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, KernelFunction->properties));
        }

        KernelFunction->propertyValueArrayCount = Count;
        KernelFunction->propertyValues = propertyValues;
    }

    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

gceSTATUS
gcKERNEL_FUNCTION_AddKernelFunctionProperties(
        IN gcKERNEL_FUNCTION KernelFunction,
        IN gctINT propertyType,
        IN gctSIZE_T propertySize,
        IN gctINT * values
        )
{
    gceSTATUS status;

    gcmHEADER_ARG("KernelFunction=0x%x",
                  KernelFunction);

    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    if (KernelFunction->propertyArrayCount <= KernelFunction->propertyCount)
    {
        status = gcKERNEL_FUNCTION_ReallocateKernelFunctionProperties(KernelFunction,
            KernelFunction->propertyCount + 10, gcvFALSE);
        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }
    }

    KernelFunction->properties[KernelFunction->propertyCount].propertySize = propertySize;
    KernelFunction->properties[KernelFunction->propertyCount].propertyType = propertyType;

    KernelFunction->propertyCount++;

    if (KernelFunction->propertyValueArrayCount <= KernelFunction->propertyValueCount)
    {
        status = gcKERNEL_FUNCTION_ReallocateKernelFunctionProperties(KernelFunction,
            propertySize + 10, gcvTRUE);
        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }
    }

    gcmVERIFY_OK(gcoOS_MemCopy(&KernelFunction->propertyValues[KernelFunction->propertyValueCount],
        values,
        gcmSIZEOF(gctINT) * propertySize));

    KernelFunction->propertyValueCount += propertySize;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

}

/*******************************************************************************
**  gcKERNEL_FUNCTION_AddImageSampler
**
**  Add an Image Sampler pair to a gcKERNEL_FUNCTION object.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**      gctUINT8    ImageNum;
**          Kernel function argument # associated with the image passed to the kernel function
**
**      gctBOOL        IsConstantSamplerType;
**          Sampler type is a constant variable inside the program
**
**    gctUINT32    SamplerType;
**          Either a kernel function argument # associated with the sampler type passed in as
**          a kernel function argument
**            or
**          defined as a constant variable as determined from IsConstantSamplerType
**          Array length of the uniform to add.  'Length' must be at least 1.
*/
gceSTATUS
gcKERNEL_FUNCTION_AddImageSampler(
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctUINT8 ImageNum,
    IN gctBOOL IsConstantSamplerType,
    IN gctUINT32 SamplerType
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("KernelFunction=0x%x ImageNum=%u IsConstantSamplerType=%d SamplerType=%lu",
                  KernelFunction, ImageNum, IsConstantSamplerType, SamplerType);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    /* Check array count. */
    if (KernelFunction->imageSamplerArrayCount <= KernelFunction->imageSamplerCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcKERNEL_FUNCTION_ReallocateImageSamplers(KernelFunction,
                                                           KernelFunction->imageSamplerCount + 10);
        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    KernelFunction->imageSamplers[KernelFunction->imageSamplerCount].imageNum  = ImageNum;
    KernelFunction->imageSamplers[KernelFunction->imageSamplerCount].isConstantSamplerType = IsConstantSamplerType;
    KernelFunction->imageSamplers[KernelFunction->imageSamplerCount].samplerType = SamplerType;

    KernelFunction->imageSamplerCount++;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_GetImageSamplerCount
**
**  Get the number of image sampler pairs for this kernel function.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**  OUTPUT:
**
**      gctSIZE_T * Count
**          Pointer to a variable receiving the number of image sampler pairs.
*/
gceSTATUS
gcKERNEL_FUNCTION_GetImageSamplerCount(
    IN gcKERNEL_FUNCTION KernelFunction,
    OUT gctSIZE_T * Count
    )
{
    gcmHEADER_ARG("KernelFunction=0x%x", KernelFunction);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Count != gcvNULL);

    /* Return image sampler pair count. */
    *Count = KernelFunction->imageSamplerCount;

    /* Success. */
    gcmFOOTER_ARG("*Count=%lu", *Count);
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_GetImageSampler
**
**  Get the indexed image sampler pair for this kernel function.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION KernelFunction
**          Pointer to a gcKERNEL_FUNCTION object.
**
**      gctUINT Index
**          Index into the image sampler pair array
**
**  OUTPUT:
**
**     gctUINT8    *ImageNum;
**          Pointer to variable to receive Kernel function argument # associated with the image
**
**      gctBOOL    *IsConstantSamplerType;
**          Pointer to varaible to indicate whethger sampler type is a constant variable
**
**    gctUINT32 *SamplerType;
**          Pointer to sampler type
*/
gceSTATUS
gcKERNEL_FUNCTION_GetImageSampler(
    IN gcKERNEL_FUNCTION KernelFunction,
    IN gctUINT Index,
    OUT gctUINT8 *ImageNum,
    OUT gctBOOL *IsConstantSamplerType,
    OUT gctUINT32 *SamplerType
    )
{
    gcmHEADER_ARG("KernelFunction=0x%x Index=%u", KernelFunction, Index);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Index < KernelFunction->imageSamplerCount);

    if(ImageNum) {
       *ImageNum = KernelFunction->imageSamplers[Index].imageNum;
    }
    if(IsConstantSamplerType) {
       *IsConstantSamplerType = KernelFunction->imageSamplers[Index].isConstantSamplerType;
    }
    if(SamplerType) {
       *SamplerType = KernelFunction->imageSamplers[Index].samplerType;
    }

    /* Success. */
    if ((ImageNum != gcvNULL) || (IsConstantSamplerType != gcvNULL) || (SamplerType != gcvNULL))
    {
        gcmFOOTER_ARG("*ImageNum=%u *IsConstantSamplerType=%d *SamplerType=%u",
                      gcmOPT_VALUE(ImageNum), gcmOPT_VALUE(IsConstantSamplerType),
                      gcmOPT_VALUE(SamplerType));
    }
    else
    {
        gcmFOOTER_NO();
    }
    return gcvSTATUS_OK;
}

/*******************************************************************************
**  gcKERNEL_FUNCTION_ReallocateArguments
**
**  Reallocate an array of gcsFUNCTION_ARGUMENT objects in a kernel function.
**
**  INPUT:
**
**      gcKERNEL_FUNCTION Function
**          Pointer to a gcKERNEL_FUNCTION object.
**
**      gctSIZE_T Count
**          Array count to reallocate.  'Count' must be at least 1.
*/
gceSTATUS
gcKERNEL_FUNCTION_ReallocateArguments(
    IN gcKERNEL_FUNCTION Function,
    IN gctSIZE_T Count
    )
{
    gcsFUNCTION_ARGUMENT_PTR arguments;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Function=0x%x Count=%lu", Function, Count);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Function, gcvOBJ_KERNEL_FUNCTION);

    if (Count < Function->argumentCount)
    {
        /* Error. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Count == Function->argumentArrayCount)
    {
        /* No action needed. */
        gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
        return gcvSTATUS_OK;
    }

    /* Allocate a new array of object pointers. */
    status = gcoOS_Allocate(gcvNULL,
                            gcmSIZEOF(gcsFUNCTION_ARGUMENT) * Count,
                            &pointer);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFOOTER();
        return status;
    }

    arguments = pointer;

    if (Function->arguments != gcvNULL)
    {
        /* Copy the current object pointers. */
        gcmVERIFY_OK(gcoOS_MemCopy(arguments,
                                   Function->arguments,
                                   gcmSIZEOF(gcsFUNCTION_ARGUMENT)
                                   * Function->argumentCount));

        /* Free the current array of object pointers. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Function->arguments));
    }

    /* Set new gcFUNCTION object pointer. */
    Function->argumentArrayCount = Count;
    Function->arguments          = arguments;

    /* Success. */
    gcmFOOTER_ARG("status=%d", gcvSTATUS_OK);
    return gcvSTATUS_OK;
}

gceSTATUS
gcKERNEL_FUNCTION_AddArgument(
    IN gcKERNEL_FUNCTION Function,
    IN gctUINT16 TempIndex,
    IN gctUINT8 Enable,
    IN gctUINT8 Qualifier
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Function=0x%x TempIndex=%u Enable=%u Qualifier=%u",
                  Function, TempIndex, Enable, Qualifier);

    gcmVERIFY_OBJECT(Function, gcvOBJ_KERNEL_FUNCTION);

    if (Function->argumentArrayCount <= Function->argumentCount)
    {
        /* Reallocate a new array of object pointers. */
        status = gcKERNEL_FUNCTION_ReallocateArguments(Function, Function->argumentCount + 10);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFOOTER();
            return status;
        }
    }

    Function->arguments[Function->argumentCount].index     = TempIndex;
    Function->arguments[Function->argumentCount].enable    = Enable;
    Function->arguments[Function->argumentCount].qualifier = Qualifier;

    Function->argumentCount++;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
gcKERNEL_FUNCTION_GetArgument(
    IN gcKERNEL_FUNCTION Function,
    IN gctUINT16 Index,
    OUT gctUINT16_PTR Temp,
    OUT gctUINT8_PTR Enable,
    OUT gctUINT8_PTR Swizzle
    )
{
    gcmHEADER_ARG("Function=0x%x Index=%u", Function, Index);

    gcmVERIFY_OBJECT(Function, gcvOBJ_KERNEL_FUNCTION);

    if (Index >= Function->argumentCount)
    {
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    if (Temp != gcvNULL)
    {
        *Temp = Function->arguments[Index].index;
    }

    if (Enable != gcvNULL)
    {
        *Enable = Function->arguments[Index].enable;
    }

    if (Swizzle != gcvNULL)
    {
        switch (Function->arguments[Index].enable)
        {
        case 0x1: *Swizzle = gcSL_SWIZZLE_XXXX; break;
        case 0x2: *Swizzle = gcSL_SWIZZLE_YYYY; break;
        case 0x4: *Swizzle = gcSL_SWIZZLE_ZZZZ; break;
        case 0x8: *Swizzle = gcSL_SWIZZLE_WWWW; break;
        case 0x3: *Swizzle = gcSL_SWIZZLE_XYYY; break;
        case 0x6: *Swizzle = gcSL_SWIZZLE_YZZZ; break;
        case 0xC: *Swizzle = gcSL_SWIZZLE_ZWWW; break;
        case 0x7: *Swizzle = gcSL_SWIZZLE_XYZZ; break;
        case 0xE: *Swizzle = gcSL_SWIZZLE_YZWW; break;
        case 0xF: *Swizzle = gcSL_SWIZZLE_XYZW; break;
        default:  gcmFATAL("Oops!");
        }
    }

    if ((Temp != gcvNULL) || (Enable != gcvNULL) || (Swizzle != gcvNULL))
    {
        gcmFOOTER_ARG("*Temp=%u *Enable=%u *Swizzle=%u",
                      gcmOPT_VALUE(Temp), gcmOPT_VALUE(Enable),
                      gcmOPT_VALUE(Swizzle));
    }
    else
    {
        gcmFOOTER_NO();
    }
    return gcvSTATUS_OK;
}

gceSTATUS
gcKERNEL_FUNCTION_GetLabel(
    IN gcKERNEL_FUNCTION Function,
    OUT gctUINT_PTR Label
    )
{
    gcmHEADER_ARG("Function=0x%x", Function);

    gcmVERIFY_OBJECT(Function, gcvOBJ_KERNEL_FUNCTION);
    gcmDEBUG_VERIFY_ARGUMENT(Label != gcvNULL);

    *Label = Function->label;

    gcmFOOTER_ARG("*Label=%u", *Label);
    return gcvSTATUS_OK;
}

gceSTATUS
gcKERNEL_FUNCTION_SetCodeEnd(
    IN gcKERNEL_FUNCTION KernelFunction
    )
{
    gcSHADER shader;

    gcmHEADER_ARG("KernelFunction=0x%x", KernelFunction);

    gcmVERIFY_OBJECT(KernelFunction, gcvOBJ_KERNEL_FUNCTION);

    gcmASSERT(KernelFunction->shader);
    shader = KernelFunction->shader;
    gcmDEBUG_VERIFY_ARGUMENT(shader->currentKernelFunction == KernelFunction);

    if (shader->instrIndex != gcSHADER_OPCODE)
    {
        shader->instrIndex       = gcSHADER_OPCODE;
        shader->lastInstruction += 1;
    }

    KernelFunction->codeEnd = shader->lastInstruction;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
#endif /*VIVANTE_NO_3D*/
