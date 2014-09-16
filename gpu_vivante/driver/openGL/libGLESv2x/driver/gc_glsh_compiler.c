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

/* needed for loadtime optimization */
#include "gc_hal_optimizer.h"

/*#define SHADER_DUMP_DIRECTORY "data/shaders"*/

#ifdef SHADER_DUMP_DIRECTORY
static gceSTATUS
_ShaderDumpInit(
    IN gcoOS Os,
    IN gctINT ShaderType,
    OUT gctSTRING * Name,
    OUT gctSIZE_T_PTR NameSize
    )
{
    static gctUINT id = 1;
    gctSTRING name = gcvNULL;
    gctSIZE_T length;
    gctUINT offset;
    gctFILE file = gcvNULL;
    gceSTATUS status;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Name != gcvNULL);

    do
    {
        /* Get the length of the directory. */
        gcmERR_BREAK(gcoOS_StrLen(SHADER_DUMP_DIRECTORY, &length));
        gcmASSERT(length > 0);

        /* Allocate memory. */
        gcmERR_BREAK(gcoOS_Allocate(Os, length + 32, (gctPOINTER *) &name));

        /* Copy directory. */
        gcmERR_BREAK(
            gcoOS_StrCopySafe(name, length + 32, SHADER_DUMP_DIRECTORY));

        /* Append file separator. */
        if ((name[length] != '/') && (name[length] != '\\'))
        {
            gcmERR_BREAK(gcoOS_StrCatSafe(name, length + 32, "/"));
            ++length;
        }

        /* Loop to find an ID. */
        for (; id < 99999; ++id)
        {
            /* Append ID to filename. */
            offset = (gctUINT) length;

            gcmERR_BREAK(
                gcoOS_PrintStrSafe(name, length + 32, &offset, "%05d", id));

            gcmERR_BREAK(
                gcoOS_StrCatSafe(name, length + 32,
                                 (ShaderType == gcSHADER_TYPE_VERTEX)
                                 ? ".vert"
                                 : ".frag"));

            if (gcmIS_ERROR(gcoOS_Open(Os, name, gcvFILE_READ, &file)))
            {
                /* If the file does not exist, we are ready. */
                *Name = name;
                *NameSize = length + 32;

                /* Success. */
                return gcvSTATUS_OK;
            }

            /* Close the file. */
            gcmVERIFY_OK(gcoOS_Close(Os, file));
        }
    }
    while (gcvFALSE);

    if (name != gcvNULL)
    {
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(Os, name));
    }

    /* Return the status. */
    return status;
}

static gceSTATUS
_ShaderDumpDelete(
    IN gcoOS Os,
    IN gctSTRING Name
    )
{
    gceSTATUS = status;
    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Name != gcvNULL);

    /* Free the name buffer. */
    status = gcmOS_SAFE_FREE(Os, Name);
    return status;
}

static gceSTATUS
_ShaderDumpSource(
    IN gcoOS Os,
    IN gctSTRING Name,
    IN gctCONST_STRING Source,
    IN gctSIZE_T Size
    )
{
    gctFILE file = gcvNULL;
    gceSTATUS status;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Name != gcvNULL);
    gcmVERIFY_ARGUMENT(Source != gcvNULL);

    do
    {
        /* Open file. */
        gcmERR_BREAK(gcoOS_Open(Os, Name, gcvFILE_CREATE, &file));

        /* Write data. */
        gcmERR_BREAK(gcoOS_Write(Os, file, Size, Source));

        /* Close file. */
        gcmERR_BREAK(gcoOS_Close(Os, file));

        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (file != gcvNULL)
    {
        /* Roll back. */
        gcmVERIFY_OK(gcoOS_Close(Os, file));
    }

    /* Return the status. */
    return status;
}

static gceSTATUS
_ShaderDump(
    IN gcoOS Os,
    IN gctSTRING Name,
    IN gctSIZE_T NameSize,
    IN gcSHADER Shader
    )
{
    gceSTATUS status;
    gctSIZE_T bufferSize;
    gctSTRING buffer = gcvFALSE;
    gctFILE file = gcvNULL;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Name != gcvNULL);

    do
    {
        /* Get the shader size. */
        gcmERR_BREAK(gcSHADER_Save(Shader, gcvNULL, &bufferSize));

        /* Allocate memory to store shader */
        gcmERR_BREAK(gcoOS_Allocate(Os, bufferSize, (gctPOINTER *) &buffer));

        /* Save the shader binary. */
        gcmERR_BREAK(gcSHADER_Save(Shader, buffer, &bufferSize));

        /* Append the shader extension. */
        gcmERR_BREAK(gcoOS_StrCatSafe(Name, NameSize, ".gcSL"));

        /* Open the file. */
        gcmERR_BREAK(gcoOS_Open(Os, Name, gcvFILE_CREATE, &file));

        /* Write the binary shader. */
        gcmERR_BREAK(gcoOS_Write(Os, file, bufferSize, buffer));

        /* Close the file. */
        gcmERR_BREAK(gcoOS_Close(Os, file));

        /* Free the binary buffer. */
        gcmERR_BREAK(gcmOS_SAFE_FREE(Os, buffer));

        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (file != gcvNULL)
    {
        /* Roll back. */
        gcmVERIFY_OK(gcoOS_Close(Os, file));
    }

    if (buffer != gcvNULL)
    {
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(Os, buffer));
    }

    /* Return the status. */
    return status;
}

static gceSTATUS
_ShaderDumpError(
    IN gcoOS Os,
    IN gctSTRING Name,
    IN gctSIZE_T NameSize,
    IN gctSTRING ErrMessage,
    IN gctSIZE_T Length
    )
{
    gceSTATUS status;
    gctFILE file = gcvNULL;

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Os, gcvOBJ_OS);
    gcmVERIFY_ARGUMENT(Name != gcvNULL);
    gcmVERIFY_ARGUMENT(Length >= 0);

    do
    {
        /* Append the shader extension. */
        gcmERR_BREAK(gcoOS_StrCatSafe(Name, NameSize, ".elog"));

        /* Open the file. */
        gcmERR_BREAK(gcoOS_Open(Os, Name, gcvFILE_CREATE, &file));

        /* Write the binary shader. */
        if (ErrMessage != gcvNULL)
        {
            gcmERR_BREAK(gcoOS_Write(Os, file, Length, ErrMessage));
        }

        /* Close the file. */
        gcmERR_BREAK(gcoOS_Close(Os, file));

        /* Success. */
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    return status;
}
#endif /* SHADER_DUMP_DIRECTORY */

GLboolean
_glshCompileShader(
    GLContext Context,
    IN gctINT ShaderType,
    IN gctSIZE_T SourceSize,
    IN gctCONST_STRING Source,
    OUT gcSHADER * Binary,
    OUT gctSTRING * Log
    )
{
    gceSTATUS status;
#ifdef SHADER_DUMP_DIRECTORY
    gctSTRING shaderName;
    gctSIZE_T shaderNameSize;
#endif

    if (SourceSize == 0)
    {
        gcoOS_StrDup(gcvNULL,
                     "No source attached.",
                     Log);
        return GL_FALSE;
    }

    glmPROFILE(Context, GL_COMPILER_SHADER_LENGTH, SourceSize);
    glmPROFILE(Context, GL_COMPILER_START_TIME, 0);

    if (Context->compiler == gcvNULL)
    {
        if (!_glshLoadCompiler(Context))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            return GL_FALSE;
        }
    }

    /* Call the HAL compiler. */
#if VIVANTE_PROFILER
    gcoHAL_ProfileStart(Context->hal);
#endif

    status = (*Context->compiler)(Context->hal,
                                  ShaderType,
                                  SourceSize,
                                  Source,
                                  Binary,
                                  Log);
#if VIVANTE_PROFILER
    gcoHAL_ProfileEnd(Context->hal, "FRONT-END COMPILER");
#endif

#if !GC_ENABLE_LOADTIME_OPT
    if (gcmIS_SUCCESS(status))
    {
        /* Full optimization */
        gcmVERIFY_OK(gcSHADER_SetOptimizationOption(*Binary, gcvOPTIMIZATION_FULL));

        /* Optimize shader. */
#if VIVANTE_PROFILER
        gcoHAL_ProfileStart(Context->hal);
#endif
        status = gcOptimizeShader(*Binary, gcvNULL);
#if VIVANTE_PROFILER
        gcoHAL_ProfileEnd(Context->hal, "OPTIMIZING COMPILER");
#endif
    }
#endif /* GC_ENABLE_LOADTIME_OPT */

    glmPROFILE(Context, GL_COMPILER_END_TIME, 0);

#ifdef SHADER_DUMP_DIRECTORY
    gcmVERIFY_OK(_ShaderDumpInit(gcvNULL,
                                 (gctINT) ShaderType,
                                 (gctSTRING*) &shaderName,
                                 &shaderNameSize));

    gcmVERIFY_OK(_ShaderDumpSource(gcvNULL,
                                   shaderName,
                                   Source,
                                   SourceSize));
    if (*Binary != gcvNULL)
    {
        gcmVERIFY_OK(_ShaderDump(gcvNULL, shaderName, shaderNameSize, *Binary));
    }
    else
    {
        gcmVERIFY_OK(_ShaderDumpError(gcvNULL, shaderName, shaderNameSize, gcvNULL, 0));
    }

    gcmVERIFY_OK(_ShaderDumpDelete(gcvNULL, shaderName));
#endif

    return (status == gcvSTATUS_OK) ? GL_TRUE : GL_FALSE;
}

static void
_Count(
    IN GLProgram Program,
    IN gcSHADER Shader,
    IN GLint Count
    )
{
    GLint i;

    for (i = 0; i < Count; ++i)
    {
        gcUNIFORM uniform;
        gctSIZE_T length;
        gctCONST_STRING name;

        gcmVERIFY_OK(gcSHADER_GetUniform(Shader, i, &uniform));

        gcmVERIFY_OK(gcUNIFORM_GetName(uniform, &length, &name));

        if (name[0] == '#')
        {
            ++ Program->privateUniformCount;
        }
        else
        {
            ++ Program->uniformCount;

#if GC_ENABLE_LOADTIME_OPT
            if ((uniform->flags & gcvUNIFORM_LOADTIME_CONSTANT) != 0)
                ++ Program->ltcUniformCount;
#endif /* GC_ENABLE_LOADTIME_OPT */

            if ((GLsizei) length > Program->uniformMaxLength)
            {
                Program->uniformMaxLength = length;
            }
        }
    }
}

static gctUINT
_GetTypeSize(gcSHADER_TYPE type)
{
    if (type == gcSHADER_FLOAT_2X2)
        return 2;
    else if (type == gcSHADER_FLOAT_3X3)
        return 3;
    else if (type == gcSHADER_FLOAT_4X4)
        return 4;
    else
        return 1;
}

static gceSTATUS
_ProcessUniforms(
    IN GLContext Context,
    IN GLProgram Program,
    IN gcSHADER Shader,
    IN GLint Count,
    IN GLboolean Duplicates,
    IN OUT GLint * Index,
    IN OUT GLint * PrivateIndex,
    IN OUT GLint * SamplerIndex,
    IN OUT GLuint * outSamplers
    )
{
    GLint i, samplers = 0;
    GLint prevShaderLastIndex = *Index;
    GLint prevShaderLastPrivateIndex = *PrivateIndex;
#if gldFBO_DATABASE
    GLint count = 0;
#endif

    for (i = 0; i < Count; ++i)
    {
        gcUNIFORM           uniform;
        gctCONST_STRING     name;
        GLUniform           slot;
        gcSHADER_TYPE       type;
        gcSHADER_PRECISION  prec;
        gctSIZE_T           length;
        gctSIZE_T           bytes;
		gceSTATUS           status;
        gctBOOL             isPrivateUniform = gcvFALSE;

        gcmONERROR(gcSHADER_GetUniform(Shader, i, &uniform));

		gcmONERROR(gcUNIFORM_GetName(uniform, gcvNULL, &name));

        if (name[0] == '#')
        {
            gcmASSERT(*PrivateIndex < Program->privateUniformCount);
            slot = &Program->privateUniforms[*PrivateIndex];
            isPrivateUniform = gcvTRUE;

#if GC_ENABLE_LOADTIME_OPT
            /* Get the uniform type. */
            gcmONERROR(gcUNIFORM_GetTypeEx(uniform, &type, &prec, &length));
#endif /* GC_ENABLE_LOADTIME_OPT */

            if (Duplicates)
            {
                GLint j;

                for (j = 0; j < prevShaderLastPrivateIndex; ++j)
                {
                    gctCONST_STRING name2;

                    gcmONERROR(gcUNIFORM_GetName(
                        Program->privateUniforms[j].uniform[0],
                        gcvNULL,
                        &name2));

                    if (gcmIS_SUCCESS(gcoOS_StrCmp(name, name2)))
                    {
                        slot = &Program->privateUniforms[j];
                        break;
                    }
                }
            }

#if !GC_ENABLE_LOADTIME_OPT
            if (slot->uniform[0] == gcvNULL)
            {
                slot->uniform[0] = uniform;

                ++ (*PrivateIndex);
            }
            else
            {
                slot->uniform[1] = uniform;

                -- (Program->privateUniformCount);
            }
            continue;
#endif /* GC_ENABLE_LOADTIME_OPT */
        }
        else
        {
#if gldFBO_DATABASE
            if (gcmIS_SUCCESS(gcoOS_StrCmp(name, "uniBonesM"))
                ||
                gcmIS_SUCCESS(gcoOS_StrCmp(name, "uniBonesT"))
                ||
                gcmIS_SUCCESS(gcoOS_StrCmp(name,
                                           "uniModelViewProjectionMatrix"))
                )
            {
                if (Context->samples == 1)
                {
                    if (++count == 3)
                    {
                        Context->dbEnable  = gcvTRUE;
                        Context->dbCounter = 2;
                    }
                }
            }
#endif
            gcmASSERT(*Index < Program->uniformCount);
            slot = &Program->uniforms[*Index];

            /* Get the uniform type. */
            gcmONERROR(gcUNIFORM_GetTypeEx(uniform, &type, &prec, &length));

            if (Duplicates)
            {
                GLint j;

                for (j = 0; j < prevShaderLastIndex; ++j)
                {
                    gctCONST_STRING name2;

                    gcmONERROR(gcUNIFORM_GetName(Program->uniforms[j].uniform[0],
                                                   gcvNULL,
                                                   &name2));

                    if (gcmIS_SUCCESS(gcoOS_StrCmp(name, name2)))
                    {
                        gcSHADER_TYPE type2;
                        gctSIZE_T length2;
                        gcSHADER_PRECISION prec2;

                        gcmONERROR(
                            gcUNIFORM_GetTypeEx(Program->uniforms[j].uniform[0],
                                              &type2,
                                              &prec2,
                                              &length2));

                        /* Failed to link for uniform! */
                        if (!((type == type2) && (length == length2) && (prec == prec2)))
                            return gcvSTATUS_UNIFORM_TYPE_MISMATCH;

                        slot = &Program->uniforms[j];
#if GC_ENABLE_LOADTIME_OPT
                        /* the app could call linkProgram multiple times */
                        gcmASSERT(   uniform->glUniformIndex == (gctUINT16)(gctINT16) -1
                                  || uniform->glUniformIndex ==  (gctUINT16)j );

                        uniform->glUniformIndex = (gctUINT16)j;
#endif /* GC_ENABLE_LOADTIME_OPT */
                        break;
                    }
                }
            }
        }

        switch (type)
        {
        case gcSHADER_SAMPLER_1D:
        case gcSHADER_SAMPLER_2D:
        case gcSHADER_SAMPLER_3D:
        case gcSHADER_SAMPLER_CUBIC:
        case gcSHADER_SAMPLER_EXTERNAL_OES:
            samplers += length;

            bytes = gcmSIZEOF(GLfloat) * length;

            while (length-- > 0)
            {
                gcmASSERT(*SamplerIndex < (GLint) gcmCOUNTOF(Program->sampleMap));
                Program->sampleMap[*SamplerIndex].type = type;
                Program->sampleMap[*SamplerIndex].unit = *SamplerIndex;
                ++ (*SamplerIndex);
            }

            break;

        case gcSHADER_FLOAT_X1:
        case gcSHADER_BOOLEAN_X1:
        case gcSHADER_INTEGER_X1:
            bytes = 1 * gcmSIZEOF(GLfloat) * length;
            break;

        case gcSHADER_FLOAT_X2:
        case gcSHADER_BOOLEAN_X2:
        case gcSHADER_INTEGER_X2:
            bytes = 2 * gcmSIZEOF(GLfloat) * length;
            break;

        case gcSHADER_FLOAT_X3:
        case gcSHADER_BOOLEAN_X3:
        case gcSHADER_INTEGER_X3:
            bytes = 3 * gcmSIZEOF(GLfloat) * length;
            break;

        case gcSHADER_FLOAT_X4:
        case gcSHADER_BOOLEAN_X4:
        case gcSHADER_INTEGER_X4:
            bytes = 4 * gcmSIZEOF(GLfloat) * length;
            break;

        case gcSHADER_FLOAT_2X2:
            bytes = 2 * 2 * gcmSIZEOF(GLfloat) * length;
            break;

        case gcSHADER_FLOAT_3X3:
            bytes = 3 * 3 * gcmSIZEOF(GLfloat) * length;
            break;

        case gcSHADER_FLOAT_4X4:
            bytes = 4 * 4 * gcmSIZEOF(GLfloat) * length;
            break;

        default:
            gcmFATAL("Unknown shader type %d!", type);
            bytes = 0;
        }

        if (slot->uniform[0] == gcvNULL)
        {
#if GC_ENABLE_LOADTIME_OPT
            gctUINT16 curIndex = isPrivateUniform ? (gctUINT16)(*PrivateIndex)++
                                                  : (gctUINT16)(*Index)++;
#endif /* GC_ENABLE_LOADTIME_OPT */
            /* Assign primary uniform. */
            slot->uniform[0] = uniform;

#if gldFBO_DATABASE
            /* Save size of uniform data. */
            slot->bytes = bytes;

            /* Allocate the data plus saved array. */
            gcmVERIFY_OK(gcoOS_Allocate(gcvNULL,
                                        bytes * 2,
                                        (gctPOINTER *) &slot->data));

            /* Set pointer to saved data. */
            slot->savedData = (gctUINT8_PTR) slot->data + bytes;
#else
            /* Allocate the data array. */
            gcmVERIFY_OK(gcoOS_Allocate(gcvNULL,
                                        bytes,
                                        (gctPOINTER *) &slot->data));
#endif

            gcmVERIFY_OK(gcoOS_ZeroMemory(slot->data, bytes));

            slot->dirty = GL_TRUE;
#if GC_ENABLE_LOADTIME_OPT
            /* the app could call linkProgram multiple times */
            gcmASSERT(   uniform->glUniformIndex == (gctUINT16)(gctINT16) -1
                      || uniform->glUniformIndex == curIndex );

            uniform->glUniformIndex = curIndex;
#else
            ++ (*Index);
#endif /* GC_ENABLE_LOADTIME_OPT */

        }
        else
        {
            /* Assign secondary uniform. */
            slot->uniform[1] = uniform;
#if GC_ENABLE_LOADTIME_OPT
            isPrivateUniform ? -- (Program->privateUniformCount)
                             : -- (Program->uniformCount);
#else
            -- (Program->uniformCount);
#endif /* GC_ENABLE_LOADTIME_OPT */
        }
    }
OnError:
    *outSamplers = samplers;
    return gcvSTATUS_OK;
}

static GLUniform
_glshFindUniform(
    IN GLProgram Program,
    IN gctCONST_STRING Name
    )
{
    GLint i;
	gceSTATUS       status;

    /* Walk all uniforms. */
    for (i = 0; i < Program->privateUniformCount; ++i)
    {
        GLUniform uniform = &Program->privateUniforms[i];
        gctCONST_STRING name;

        /* Get the uniform name. */
        gcmONERROR(gcUNIFORM_GetName(uniform->uniform[0], gcvNULL, &name));

        /* See if the uniform matches the requested name. */
        if (gcmIS_SUCCESS(gcoOS_StrCmp(Name, name)))
        {
            /* Match, return position. */
            return uniform;
        }
    }
OnError:
    return gcvNULL;
}

GLboolean
_glshLinkProgramAttributes(
    GLContext Context,
    IN OUT GLProgram Program
    )
{
    gcSHADER vertexShader, fragmentShader;
    GLuint i, j, location, index, map;
    gctSIZE_T length;
    gctSIZE_T vertexUniforms, fragmentUniforms;
    GLBinding binding;
    gctPOINTER pointer = gcvNULL;
    gcSHADER_TYPE attrType;
	gceSTATUS       status;

    vertexShader = Program->vertexShaderBinary;
    fragmentShader = Program->fragmentShaderBinary;

    /* Get the number of vertex attributes. */
    gcmVERIFY_OK(gcSHADER_GetAttributeCount(vertexShader,
                                            &Program->attributeCount));

    /* Zero out enabled attributes. */
    Program->attributeEnable = 0;

    if (Program->attributeCount > 0)
    {
        /* Allocate the pointer array for the vertex attributes. */
        if (gcmIS_ERROR(
                gcoOS_Allocate(gcvNULL,
                               Program->attributeCount * sizeof(gcATTRIBUTE),
                               &pointer)))
        {
            /* Error. */
            gcmFATAL("LinkShaders: gcoOS_Allocate failed");
            return GL_FALSE;
        }

        Program->attributePointers = pointer;

        /* Zero out the maximum length of any attribute name. */
        Program->attributeMaxLength = 0;

        /* Query all attributes. */
        for (i = index = location = 0; i < Program->attributeCount; ++i)
        {
            gcATTRIBUTE attribute;

            /* Get the gcATTRIBUTE object pointer. */
            gcmVERIFY_OK(gcSHADER_GetAttribute(vertexShader, i, &attribute));

            if (attribute != gcvNULL)
            {
                /* Save the attribute pointer. */
                Program->attributePointers[index++] = attribute;

                /* Get the attribute name. */
                gcmVERIFY_OK(gcATTRIBUTE_GetName(attribute, &length, gcvNULL));

                if (length > Program->attributeMaxLength)
                {
                    /* Update maximum length of attribute name. */
                    Program->attributeMaxLength = length;
                }

                /* Get the attribute array length. */
                gcmVERIFY_OK(gcATTRIBUTE_GetType(attribute, &attrType, &length));
                length *= _GetTypeSize(attrType);

                for (j = 0; j < length; ++j, ++location)
                {
                    if (location >= Context->maxAttributes)
                    {
                        gcmFATAL("%s(%d): Too many attributes",
                                 __FUNCTION__, __LINE__);
                        return GL_FALSE;
                    }

                    /* Set attribute location. */
                    Program->attributeLocation[location].attribute = attribute;
                    Program->attributeLocation[location].index     = j;
                }
            }
        }

        /* Set actual number of attributes. */
        Program->attributeCount = index;

        /* Zero out linkage and unused locaotions. */
        for (i = map = 0; i < Context->maxAttributes; ++i)
        {
            Program->attributeLinkage[i]           = (GLuint) -1;
            Program->attributeLocation[i].assigned = GL_FALSE;

            if (i >= location)
            {
                Program->attributeLocation[i].attribute = gcvNULL;
                Program->attributeLocation[i].index     = -1;
            }
            else
            {
                gctBOOL enable = gcvFALSE;

                /* Check whether attribute is enabled or not. */
                if (gcmIS_SUCCESS(gcATTRIBUTE_IsEnabled(Program->attributeLocation[i].attribute, &enable))
                &&  enable
                )
                {
                    Program->attributeEnable |= 1 << i;
                    Program->attributeMap[i]  = map++;
                }
            }
        }

        /* Walk all bindings and assign the attributes. */
        for (binding = Program->attributeBinding;
             binding != gcvNULL;
             binding = binding->next)
        {
            gctSIZE_T bindingLength;

            /* Get the length of the binding attribute. */
            gcmVERIFY_OK(gcoOS_StrLen(binding->name, &bindingLength));

            /* Walk all attribute locations. */
            for (i = 0; i < Context->maxAttributes; ++i)
            {
                gcATTRIBUTE attribute;
                gctCONST_STRING name;

                /* Ignore if location is unavailable or not the start of an
                   array. */
                if (Program->attributeLocation[i].index != 0)
                {
                    continue;
                }

                attribute = Program->attributeLocation[i].attribute;

                /* Compare the name of the binding with the attribute. */
                gcmVERIFY_OK(gcATTRIBUTE_GetName(attribute, &length, &name));

                if ((bindingLength == length)
                &&  gcmIS_SUCCESS(gcoOS_MemCmp(binding->name, name, length))
                )
                {
                    /* Get the length of the attribute array. */
                    gcmVERIFY_OK(gcATTRIBUTE_GetType(attribute,
                                                     gcvNULL,
                                                     &length));

                    /* Test for overflow. */
                    if (binding->index + length > Context->maxAttributes)
                    {
                        gcmFATAL("Binding for %s overflows.", binding->name);
                        return GL_FALSE;
                    }

                    /* Copy the binding. */
                    for (j = 0; j < length; ++j)
                    {
                        /* Make sure the binding is not yet taken. */
                        if (Program->attributeLinkage[binding->index + j] != (GLuint) -1)
                        {
                            gcmFATAL("Binding for %s occupied.", binding->name);
                            return GL_FALSE;
                        }

                        /* Assign the binding. */
                        Program->attributeLinkage[binding->index + j] = i + j;
                        Program->attributeLocation[i + j].assigned    = GL_TRUE;
                    }

                    break;
                }
            }
        }

        /* Walk all unassigned attributes. */
        for (i = 0; i < Context->maxAttributes; ++i)
        {
            if (Program->attributeLocation[i].assigned
            ||  (Program->attributeLocation[i].attribute == gcvNULL)
            )
            {
                continue;
            }

            gcmVERIFY_OK(
                gcATTRIBUTE_GetType(Program->attributeLocation[i].attribute,
                                    &attrType,
                                    &length));
            length *= _GetTypeSize(attrType);

            for (index = 0; index < Context->maxAttributes; ++index)
            {
                for (j = 0; j < length; ++j)
                {
                    if (Program->attributeLinkage[index + j] != (GLuint) -1)
                    {
                        break;
                    }
                }

                if (j == length)
                {
                    break;
                }
            }

            if (index == Context->maxAttributes)
            {
                gcmFATAL("No room for attribute %d (x%d)", i, length);
                return GL_FALSE;
            }

            for (j = 0; j < length; ++j)
            {
                Program->attributeLinkage[index + j]       = i + j;
                Program->attributeLocation[i + j].assigned = GL_TRUE;
            }
        }
    }

    /***************************************************************************
    ** Uniform management.
    */

    /* Get the number of uniform attributes. */
    gcmONERROR(gcSHADER_GetUniformCount(vertexShader,   &vertexUniforms));
    gcmONERROR(gcSHADER_GetUniformCount(fragmentShader, &fragmentUniforms));

    /* Count visible and private uniforms. */
    Program->vertexCount         = vertexUniforms;
    Program->vertexSamplers      = 0;
    Program->fragmentSamplers    = 0;
    Program->ltcUniformCount     = 0;
    Program->uniformCount        = 0;
    Program->privateUniformCount = 0;
    Program->uniformMaxLength    = 0;

    _Count(Program, vertexShader,   vertexUniforms);
    _Count(Program, fragmentShader, fragmentUniforms);

    if (Program->uniformCount > 0)
    {
        /* Allocate the array of _GLUniform structures. */
        gctSIZE_T bytes = Program->uniformCount * gcmSIZEOF(struct _GLUniform);

        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                       bytes,
                                       &pointer)))
        {
            /* Error. */
            gcmFATAL("%s(%d): gcoOS_Allocate failed", __FUNCTION__, __LINE__);
            return GL_FALSE;
        }

        Program->uniforms = pointer;

        gcmVERIFY_OK(gcoOS_ZeroMemory(Program->uniforms, bytes));
    }

    if (Program->privateUniformCount > 0)
    {
        /* Allocate the array of _GLUniform structures. */
        gctSIZE_T bytes = Program->privateUniformCount
                        * gcmSIZEOF(struct _GLUniform);

        if (gcmIS_ERROR(
                gcoOS_Allocate(gcvNULL,
                               bytes,
                               &pointer)))
        {
            /* Error. */
            gcmFATAL("%s(%d): gcoOS_Allocate failed", __FUNCTION__, __LINE__);
            return GL_FALSE;
        }

        Program->privateUniforms = pointer;

        gcmVERIFY_OK(gcoOS_ZeroMemory(Program->privateUniforms, bytes));
    }

    /* Process any uniforms. */
    if (Program->uniformCount + Program->privateUniformCount > 0)
    {
        gctINT uniformIndex = 0;
        gctINT privateIndex = 0;
        gctINT samplerIndex = 0;
        gctUINT vsSamplesCount = 0;
        gctINT vssamplerIndex = 8;
        gctUINT samplers = 0;
        gceSTATUS status;

                /* Verify the arguments. */
        gcmVERIFY_OK(gcoHAL_QueryTextureCaps(Context->hal,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL,
                                         &vsSamplesCount,
                                         gcvNULL));
        if (vsSamplesCount == 16)
        {
            vssamplerIndex = 16;
        }
        /* Process vertex uniforms. */
        status = _ProcessUniforms(Context,
                                  Program,
                                  vertexShader,
                                  vertexUniforms,
                                  GL_FALSE,
                                  &uniformIndex,
                                  &privateIndex,
                                  &vssamplerIndex,
                                  &samplers);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFATAL("%s(%d): gcLinkShaders failed status=%d", __FUNCTION__, __LINE__, status);
            return GL_FALSE;
        }

        Program->vertexSamplers = samplers;

        /* Process fragment uniforms. */
        status = _ProcessUniforms(Context,
                                  Program,
                                  fragmentShader,
                                  fragmentUniforms,
                                  GL_TRUE,
                                  &uniformIndex,
                                  &privateIndex,
                                  &samplerIndex,
                                  &samplers);

        if (gcmIS_ERROR(status))
        {
            /* Error. */
            gcmFATAL("%s(%d): gcLinkShaders failed status=%d", __FUNCTION__, __LINE__, status);
            return GL_FALSE;
        }

        Program->fragmentSamplers = samplers;
    }

    Program->depthRangeNear = _glshFindUniform(Program, "#DepthRange.near");
    Program->depthRangeFar  = _glshFindUniform(Program, "#DepthRange.far");
    Program->depthRangeDiff = _glshFindUniform(Program, "#DepthRange.diff");
    Program->height         = _glshFindUniform(Program, "#Height");

#if VIVANTE_PROFILER
    gcoHAL_ProfileEnd(Context->hal, "BACK-END LINKER");
#endif
    /* Success. */
    return GL_TRUE;
OnError:
	return GL_FALSE;
}

GLboolean
_glshLinkShaders(
    GLContext Context,
    IN OUT GLProgram Program
    )
{
    gcSHADER vertexShader, fragmentShader;
    gceSTATUS status;

    /* Test if any of the shaders is dirty. */
	if (Program->vertexShader->dirty ||
        Program->fragmentShader->dirty ||
        (Program->vertexShaderBinary &&
         gcSHADER_CheckValidity(Program->vertexShaderBinary) != gcvSTATUS_OK) ||
        (Program->fragmentShaderBinary &&
         gcSHADER_CheckValidity(Program->fragmentShaderBinary) != gcvSTATUS_OK))
	{
	    if (Program->vertexShaderBinary == gcvNULL)
	    {
            /* Construct a new vertex shader. */
            gcmONERROR(gcSHADER_Construct(Context->hal,
                                          gcSHADER_TYPE_VERTEX,
                                          &Program->vertexShaderBinary));
        }

    	/* Copy the vertex shader binary from the shader to the program. */
	    gcmONERROR(gcSHADER_Copy(Program->vertexShaderBinary,
	                             Program->vertexShader->binary));

        /* Vertex shader is no longer dirty. */
	    Program->vertexShader->dirty = GL_FALSE;

        if (Program->fragmentShaderBinary == gcvNULL)
        {
            /* Construct a new fragment shader. */
            gcmONERROR(gcSHADER_Construct(Context->hal,
                                          gcSHADER_TYPE_FRAGMENT,
                                          &Program->fragmentShaderBinary));
        }

    	/* Copy the fragment shader binary from the shader to the program. */
    	gcmONERROR(gcSHADER_Copy(Program->fragmentShaderBinary,
	                             Program->fragmentShader->binary));

        /* Fragment shader is no longer dirty. */
	    Program->fragmentShader->dirty = GL_FALSE;
	}

    /* Get pointer to shader binaries. */
	vertexShader   = Program->vertexShaderBinary;
	fragmentShader = Program->fragmentShaderBinary;

    /* Call the HAL backend linker. */
#if VIVANTE_PROFILER
    gcoHAL_ProfileStart(Context->hal);
#endif
    status = gcLinkShaders(vertexShader,
                           fragmentShader,
                           (gceSHADER_FLAGS)
                           ( gcvSHADER_DEAD_CODE
                           | gcvSHADER_RESOURCE_USAGE
                           | gcvSHADER_OPTIMIZER
                           | gcvSHADER_USE_GL_Z
                           | gcvSHADER_USE_GL_POINT_COORD
                           | gcvSHADER_USE_GL_POSITION
                           | gcvSHADER_LOADTIME_OPTIMIZER
                           ),
                           &Program->statesSize,
                           &Program->states,
                           &Program->hints);

    if (gcmIS_ERROR(status))
    {
        /* Error. */
        gcmFATAL("%s(%d): gcLinkShaders failed status=%d", __FUNCTION__, __LINE__, status);
        return GL_FALSE;
    }
    return  _glshLinkProgramAttributes(Context,
    				       Program);

OnError:
    gcmFATAL("%s(%d): gcSHADER out of memory", __FUNCTION__, __LINE__);
    return gcvFALSE;
}

GLboolean
_glshLoadCompiler(
    IN GLContext Context
    )
{
#if gcdSTATIC_LINK
    Context->compiler = gcCompileShader;
#else
    if (Context->dll == gcvNULL)
    {
        do
        {
            gceSTATUS status;
            union gluVARIANT
            {
                gceSTATUS           (*compiler)(IN gcoHAL Hal,
                                    IN gctINT ShaderType,
                                    IN gctSIZE_T SourceSize,
                                    IN gctCONST_STRING Source,
                                    OUT gcSHADER * Binary,
                                    OUT gctSTRING * Log);

                gctPOINTER ptr;
            }
            compiler;

            status = gcoOS_LoadLibrary(gcvNULL,
#if defined(__APPLE__)
                                       "libGLSLC.dylib",
#elif defined(LINUXEMULATOR)
                                       "libGLSLC.so",
#else
                                       "libGLSLC",
#endif
                                       &Context->dll);
            if (gcmIS_ERROR(status))
            {
                break;
            }

            gcmERR_BREAK(gcoOS_GetProcAddress(gcvNULL,
                                              Context->dll,
                                              "gcCompileShader",
                                              &compiler.ptr));

            Context->compiler = compiler.compiler;
        }
        while (gcvFALSE);
    }
#endif

    return (Context->compiler != gcvNULL);
}

void
_glshReleaseCompiler(
    IN GLContext Context
    )
{
#if !gcdSTATIC_LINK
    if (Context->dll != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_FreeLibrary(gcvNULL, Context->dll));

        Context->dll = gcvNULL;
    }
#endif

    Context->compiler = gcvNULL;
}

#if GC_ENABLE_LOADTIME_OPT

static gceSTATUS _EvaluateLoadtimeConstantExpresion(GLContext, gcSHADER, gctUINT,  LTCValue *);


gcSHADER_TYPE_INFO shader_type_info[] =
{
    {gcSHADER_FLOAT_X1, "gcSHADER_FLOAT_X1", gcSHADER_FLOAT_X1, 1, 1},		    /* 0x00 */
    {gcSHADER_FLOAT_X2,	"gcSHADER_FLOAT_X2", gcSHADER_FLOAT_X1, 2, 1},	  		/* 0x01 */
    {gcSHADER_FLOAT_X3,	"gcSHADER_FLOAT_X3", gcSHADER_FLOAT_X1, 3, 1},			/* 0x02 */
    {gcSHADER_FLOAT_X4,	"gcSHADER_FLOAT_X4", gcSHADER_FLOAT_X1, 4, 1},			/* 0x03 */
    {gcSHADER_FLOAT_2X2, "gcSHADER_FLOAT_2X2", gcSHADER_FLOAT_X1, 2, 2},		/* 0x04 */
    {gcSHADER_FLOAT_3X3, "gcSHADER_FLOAT_3X3", gcSHADER_FLOAT_X1, 3, 3},		/* 0x05 */
    {gcSHADER_FLOAT_4X4, "gcSHADER_FLOAT_4X4", gcSHADER_FLOAT_X1, 4, 4},		/* 0x06 */
    {gcSHADER_BOOLEAN_X1, "gcSHADER_BOOLEAN_X1", gcSHADER_BOOLEAN_X1, 1, 1},	/* 0x07 */
    {gcSHADER_BOOLEAN_X2, "gcSHADER_BOOLEAN_X2", gcSHADER_BOOLEAN_X1, 2, 1},	/* 0x08 */
    {gcSHADER_BOOLEAN_X3, "gcSHADER_BOOLEAN_X3", gcSHADER_BOOLEAN_X1, 3, 1},	/* 0x09 */
    {gcSHADER_BOOLEAN_X4, "gcSHADER_BOOLEAN_X4", gcSHADER_BOOLEAN_X1, 4, 1},	/* 0x0A */
    {gcSHADER_INTEGER_X1, "gcSHADER_INTEGER_X1", gcSHADER_INTEGER_X1, 1, 1},	/* 0x0B */
    {gcSHADER_INTEGER_X2, "gcSHADER_INTEGER_X2", gcSHADER_INTEGER_X1, 2, 1},	/* 0x0C */
    {gcSHADER_INTEGER_X3, "gcSHADER_INTEGER_X3", gcSHADER_INTEGER_X1, 3, 1},	/* 0x0D */
    {gcSHADER_INTEGER_X4, "gcSHADER_INTEGER_X4", gcSHADER_INTEGER_X1, 4, 1}, 	/* 0x0E */
    {gcSHADER_SAMPLER_1D, "gcSHADER_SAMPLER_1D", gcSHADER_SAMPLER_1D, 1, 1}, 	/* 0x0F */
    {gcSHADER_SAMPLER_2D, "gcSHADER_SAMPLER_2D", gcSHADER_SAMPLER_2D, 1, 1},	/* 0x10 */
    {gcSHADER_SAMPLER_3D, "gcSHADER_SAMPLER_3D", gcSHADER_SAMPLER_3D, 1, 1},	/* 0x11 */
    {gcSHADER_SAMPLER_CUBIC, "gcSHADER_SAMPLER_CUBIC", gcSHADER_SAMPLER_CUBIC, 1, 1},	/* 0x12 */
    {gcSHADER_FIXED_X1, "gcSHADER_FIXED_X1", gcSHADER_FIXED_X1, 1, 1},			/* 0x13 */
    {gcSHADER_FIXED_X2,	"gcSHADER_FIXED_X2", gcSHADER_FIXED_X1, 2, 1},			/* 0x14 */
    {gcSHADER_FIXED_X3,	"gcSHADER_FIXED_X3", gcSHADER_FIXED_X1, 3, 1},			/* 0x15 */
    {gcSHADER_FIXED_X4,	"gcSHADER_FIXED_X4", gcSHADER_FIXED_X1, 4, 1},			/* 0x16 */
    {gcSHADER_IMAGE_2D, "gcSHADER_IMAGE_2D", gcSHADER_IMAGE_2D, 1, 1},			/* 0x17 */  /* For OCL. */
    {gcSHADER_IMAGE_3D,	"gcSHADER_IMAGE_3D", gcSHADER_IMAGE_3D, 1, 1},			/* 0x18 */  /* For OCL. */
    {gcSHADER_SAMPLER,	"gcSHADER_SAMPLER", gcSHADER_SAMPLER, 1, 1}, 			/* 0x19 */  /* For OCL. */
    {gcSHADER_FLOAT_2X3, "gcSHADER_FLOAT_2X3", gcSHADER_FLOAT_X1, 2, 3},		/* 0x1A */
    {gcSHADER_FLOAT_2X4, "gcSHADER_FLOAT_2X4", gcSHADER_FLOAT_X1, 2, 4},		/* 0x1B */
    {gcSHADER_FLOAT_3X2, "gcSHADER_FLOAT_3X2", gcSHADER_FLOAT_X1, 3, 2},		/* 0x1C */
    {gcSHADER_FLOAT_3X4, "gcSHADER_FLOAT_3X4", gcSHADER_FLOAT_X1, 3, 4},		/* 0x1D */
    {gcSHADER_FLOAT_4X2, "gcSHADER_FLOAT_4X2", gcSHADER_FLOAT_X1, 4, 2},		/* 0x1E */
    {gcSHADER_FLOAT_4X3, "gcSHADER_FLOAT_4X3", gcSHADER_FLOAT_X1, 4, 3},		/* 0x1F */
    {gcSHADER_ISAMPLER_2D, "gcSHADER_ISAMPLER_2D", gcSHADER_ISAMPLER_2D, 1, 1},	/* 0x20 */
    {gcSHADER_ISAMPLER_3D, "gcSHADER_ISAMPLER_3D", gcSHADER_ISAMPLER_3D, 1, 1},	/* 0x21 */
    {gcSHADER_ISAMPLER_CUBIC, "gcSHADER_ISAMPLER_CUBIC", gcSHADER_ISAMPLER_CUBIC, 1, 1},/* 0x22 */
    {gcSHADER_USAMPLER_2D,	"gcSHADER_USAMPLER_2D", gcSHADER_USAMPLER_2D, 1, 1},	/* 0x23 */
    {gcSHADER_USAMPLER_3D,	"gcSHADER_USAMPLER_3D", gcSHADER_USAMPLER_3D, 1, 1},	/* 0x24 */
    {gcSHADER_USAMPLER_CUBIC, "gcSHADER_USAMPLER_CUBIC", gcSHADER_USAMPLER_CUBIC, 1, 1},/* 0x25 */
    {gcSHADER_SAMPLER_EXTERNAL_OES,	"gcSHADER_SAMPLER_EXTERNAL_OES", gcSHADER_SAMPLER_EXTERNAL_OES, 1, 1}	/* 0x26 */
};

/* return the swizzle for Component in the Source */
static
gctUINT
_SwizzleSourceComponent(
    IN gctUINT Source,
    IN gctUINT Component
    )
{
    /* Select on swizzle. */
    switch ((gcSL_SWIZZLE) Component)
    {
    case gcSL_SWIZZLE_X:
        /* Select swizzle x. */
        return gcmSL_SOURCE_GET(Source, SwizzleX);

    case gcSL_SWIZZLE_Y:
        /* Select swizzle y. */
        return gcmSL_SOURCE_GET(Source, SwizzleY);

    case gcSL_SWIZZLE_Z:
        /* Select swizzle z. */
        return gcmSL_SOURCE_GET(Source, SwizzleZ);

    case gcSL_SWIZZLE_W:
        /* Select swizzle w. */
        return gcmSL_SOURCE_GET(Source, SwizzleW);

    default:
        gcmASSERT(gcvFALSE);
        return (gctUINT16) 0xFFFF;
    }
} /* _SwizzleSourceComponent */

gctBOOL
_isComponentNeededForInstruction(
       IN gctUINT                    Component,
       IN gcSL_INSTRUCTION           Instruction
       )
{
    gcSL_OPCODE opcode = Instruction->opcode;
    gcmASSERT(Component < 4);  /* Component shoudl be in {0, 1, 2, 3} */

    switch (opcode)
    {
    case gcSL_DP3:
        return Component < 3;  /* use x, y, z */
    case gcSL_DP4:
        return Component < 4;  /* use x, y, z, w */
    case gcSL_NORM:
        return Component < 3;  /* use x, y, z */
    case gcSL_CROSS:
        return Component < 3;  /* use x, y, z */
    default:
        return gcvFALSE;
    }
}

static gctBOOL
_isElementEnabled(
     IN gctINT         Enable,
     IN gctUINT        element
     )
{
    gcmASSERT(element < LTC_MAX_ELEMENTS);
    return Enable & (1 << element);
}

/* get value from Instructions' SourceId, the value can be from
    1. Compile-time constant
    2. Load-time constant (uniform)
    3. Load-time evaluated value, it should be in some temp register
       which is stored in Results
*/
static gceSTATUS
_GetSourceValue(
       IN GLContext                  Context,
       IN gcSHADER                   Shader,
       IN gcSL_INSTRUCTION           Instruction,
       IN gctINT                     SourceId,
       IN LTCValue *                 Results,
       IN OUT LTCValue *             SourceValue)
{
    gceSTATUS               status = gcvSTATUS_OK;
    gctUINT16               source;
    gctUINT16               index;
    gcSL_FORMAT             format;
    gctUINT32               value;
    GLProgram               program = Context->program;
    gctINT                  i;

    gcmHEADER();

    gcmASSERT(SourceId == 0 || SourceId == 1);

    /* get source and its format */
    SourceValue->sourceInfo = source =
         (SourceId == 0) ? Instruction->source0 : Instruction->source1;
    format = (gcSL_FORMAT) gcmSL_SOURCE_GET(source, Format);
    SourceValue->elementType = format;

    /* get enabled value channel from Instruction's enable field */
    SourceValue->enable = gcmSL_TARGET_GET(Instruction->temp, Enable);

    /* set value type */

    switch (gcmSL_SOURCE_GET(source, Type))
    {
    case gcSL_CONSTANT:
    {
        /* get values one component at a time*/
        for (i=0; i<LTC_MAX_ELEMENTS; i++)
        {
            /* value is constant, it is stored in SourceIndex and
             * SourceINdexed fields */
            value = (SourceId == 0) ? (Instruction->source0Index & 0xFFFF)
                                      | (Instruction->source0Indexed << 16)
                                    : (Instruction->source1Index & 0xFFFF)
                                      | (Instruction->source1Indexed << 16);
            if (format == gcSL_FLOAT)
            {
                /* float value */
                float f = gcoMATH_UIntAsFloat(value);
                SourceValue->v[i].f32 = f;
            }
            else if (format == gcSL_INTEGER)
            {
                /* integer value */
                SourceValue->v[i].i32 = value;
            }
            else
            {
                /* Error. */
                status = gcvSTATUS_INVALID_DATA;
                break;
            }
        }
        break;
    }
    case gcSL_UNIFORM:
    {
        /* the uniform can be in the form of float4x4 b[10], we may access b[7] row 3,
           which is represented as

                sourceIndex: (3<<14 | (b's uniform index)
                sourceIndexed: (4*7) = 28  (gcSL_NOT_INDEXED)

           if it is accessed by variable i:  b[i] row 3, then the index i is assigned to
           some temp register, e.g. temp(12).x, the representation looks like this:

             MOV temp(12).x, i

                sourceIndex: (3<<14 | (b's uniform index)
                sourceIndexed: 12  (gcSL_INDEXED_X)

           Note: sometime the frontend doesn't generate the constValue and constant index
           in the way described above, but the sum of constValue and constant index is
           the offset of accessed element

        */

        /* get the uniform index */
        gctINT uniformIndex = gcmSL_INDEX_GET((SourceId == 0) ? Instruction->source0Index
                                                              : Instruction->source1Index, Index);
        gctINT constOffset = gcmSL_INDEX_GET((SourceId == 0) ? Instruction->source0Index
                                                              : Instruction->source1Index, ConstValue);
        gctINT indexedOffset = (SourceId == 0) ? Instruction->source0Indexed
                                                : Instruction->source1Indexed;
        gctINT              combinedOffset = 0;
        gcUNIFORM           uniform = Shader->uniforms[uniformIndex];
        gctCONST_STRING     uniformName;
        GLUniform           glUniform = gcvNULL;
        gctFLOAT *          data;

        gcmASSERT(uniformIndex < (gctINT)Shader->uniformCount);

        /* Get the uniform name. */
        gcmONERROR(gcUNIFORM_GetName(uniform, gcvNULL, &uniformName));

        /* find the uniform data in the  program */
        /* Walk all uniforms. */
        if ('#' == uniformName[0])
        {
            /* private (built-in) uniform */
            for (i = 0; i < program->privateUniformCount ; ++i)
            {
                GLUniform u = &program->privateUniforms[i];
                gctCONST_STRING name;

                /* Get the uniform name. */
                gcmONERROR(gcUNIFORM_GetName(u->uniform[0], gcvNULL, &name));

                /* See if the uniform matches the requested name. */
                if (gcmIS_SUCCESS(gcoOS_StrCmp(uniformName, name)))
                {
                    /* Match, return position. */
                    glUniform = u;
                    break;
                }
            }
        }
        else
        {
            for (i = 0; i < program->uniformCount; ++i)
            {
                GLUniform u = &program->uniforms[i];
                gctCONST_STRING name;

                /* Get the uniform name. */
                gcmONERROR(gcUNIFORM_GetName(u->uniform[0], gcvNULL, &name));

                /* See if the uniform matches the requested name. */
                if (gcmIS_SUCCESS(gcoOS_StrCmp(uniformName, name)))
                {
                    /* Match, return position. */
                    glUniform = u;
                    break;
                }
            }
        }
        if (glUniform == gcvNULL)
        {
            /* the uniform is not found in program */
            status = gcvSTATUS_INVALID_DATA;
            break;
        }

        if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
        {
            LTCValue *            indexedValue;
            gcSL_FORMAT           indexedTempFormat;

            /* check if the index is out of bounds */
            indexedValue = Results + indexedOffset;
            indexedTempFormat =  (gcSL_FORMAT) gcmSL_SOURCE_GET(indexedValue->sourceInfo, Format);

            switch (gcmSL_SOURCE_GET(source, Indexed))
            {
            case gcSL_INDEXED_X:
                indexedOffset = (indexedTempFormat == gcSL_FLOAT) ? (gctINT)indexedValue->v[0].f32
                                                                  : indexedValue->v[0].i16;
                break;
            case gcSL_INDEXED_Y:
                indexedOffset = (indexedTempFormat == gcSL_FLOAT) ? (gctINT)indexedValue->v[1].f32
                                                                  : indexedValue->v[1].i16;
                break;
            case gcSL_INDEXED_Z:
                indexedOffset = (indexedTempFormat == gcSL_FLOAT) ? (gctINT)indexedValue->v[2].f32
                                                                  : indexedValue->v[2].i16;
                break;
            case gcSL_INDEXED_W:
                indexedOffset = (indexedTempFormat == gcSL_FLOAT) ? (gctINT)indexedValue->v[3].f32
                                                                  : indexedValue->v[3].i16;
                break;
            default:
                break;
            }
        }

        /* check if the combined indexed offset is out of bounds of the uniform */
        combinedOffset = constOffset + indexedOffset;
        if (combinedOffset >= uniform->arraySize * shader_type_info[uniform->type].rows )
        {
            /* the access is out of bounds */
            status = gcvSTATUS_INVALID_DATA;
            break;
        }

        data = (gctFLOAT *)glUniform->data +
                  combinedOffset * shader_type_info[uniform->type].components;

        gcmASSERT(data != gcvNULL);

        /* get the data from GLUniform one component at a time */
        for (i=0; i<LTC_MAX_ELEMENTS; i++)
        {
             gctUINT sourceComponent;

            /* get the swizzled source component for channel i
             * (corresponding to target ith component) */
            sourceComponent = _SwizzleSourceComponent(source, (gctUINT16)i);

            if (format == gcSL_FLOAT)
            {
                /* float value */
                SourceValue->v[i].f32 = data[sourceComponent];
            }
            else if (format == gcSL_INTEGER)
            {
                /* integer value */
                SourceValue->v[i].i32 = ((gctINT *)data)[sourceComponent];
            }
            else
            {
                /* Error. */
                status = gcvSTATUS_INVALID_DATA;
                break;
            }
        }
        break;
    }
    case gcSL_TEMP:
    {
        LTCValue *            tempValue;
        gcmASSERT(gcmSL_SOURCE_GET(source, Indexed) == gcSL_NOT_INDEXED);

        /* the temp register and its indexed value should both be LTC */
        index = (SourceId == 0) ? Instruction->source0Index
                                : Instruction->source1Index;
        tempValue = Results + index;

        /* get the data from tempVale one component at a time */
        for (i=0; i<LTC_MAX_ELEMENTS; i++)
        {
            gctUINT sourceComponent;
            /* get the swizzled source component for channel i
             * (corresponding to target ith component) */
            sourceComponent = _SwizzleSourceComponent(source, i);

            if (format == gcSL_FLOAT)
            {
                /* float value */
                SourceValue->v[i].f32 = tempValue->v[sourceComponent].f32;
            }
            else if (format == gcSL_INTEGER)
            {
                /* integer value */
                SourceValue->v[i].i32 = tempValue->v[sourceComponent].i32;
            }
            else
            {
                /* Error. */
                status = gcvSTATUS_INVALID_DATA;
                break;
            }
        } /* for */
    }
    default:
        break;
    } /* switch */

 OnError:
    /* error handling */
    gcmASSERT(gcvSTATUS_INVALID_DATA != status);
    /* Return the status. */
    gcmFOOTER();
   return status;
} /* _GetSourceValue */


/*******************************************************************************
**                          gcEvaluateLoadtimeConstantExpresions
********************************************************************************
**
**  evaluate the LTC expression with the unitform input
**
**
*/
gceSTATUS
gcEvaluateLoadtimeConstantExpresions(
    IN GLContext                  Context,
    IN gcSHADER                   Shader
    )
{
    gceSTATUS               status = gcvSTATUS_OK;
    /*gcSL_INSTRUCTION        ltcExpressions = Shader->ltcExpressions; */
    LTCValue *              results = gcvNULL;
    int                     i;

    gcmHEADER();

    /* do nothing if there is no Loadtime Constant Expression */
    if (Shader->ltcUniformCount == 0)
    {
        gcmFOOTER();
        return status;
    }


    gcmASSERT(Shader->ltcInstructionCount > 0 &&
              Shader->ltcExpressions != gcvNULL);

    /* allocate result array */
    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              sizeof(LTCValue)*Shader->ltcInstructionCount,
                              (gctPOINTER *) &results));

    /* evaluate all intructions */
    for (i = 0; i < (gctINT)Shader->ltcInstructionCount; i++)
    {
        results[i].instructionIndex = i;
        gcmONERROR(_EvaluateLoadtimeConstantExpresion(Context,
                                                      Shader,
                                                      i,
                                                      results));
    } /* for */

    /* free result array */
    gcmONERROR(gcoOS_Free(gcvNULL, results));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
} /* gcEvaluateLoadtimeConstantExpresions */


GLboolean
gcIsUniformALoadtimeConstant(
    IN GLUniform   Uniform
                     )
{
   return (Uniform->uniform[0]->flags & gcvUNIFORM_LOADTIME_CONSTANT) != 0;
} /* isLoadtimeConstant */

/*******************************************************************************
**                          gcComputeLoadtimeConstant
********************************************************************************
**
**  compute the loadtime constant in the Context
**
**
*/
gceSTATUS
gcComputeLoadtimeConstant(
    IN GLContext Context
    )
{
    gceSTATUS               status = gcvSTATUS_OK;
    GLProgram               program = Context->program;
    gcSHADER                vertexShaderBinary = program->vertexShaderBinary;
    gcSHADER                fragmentShaderBinary = program->fragmentShaderBinary;

    gcmHEADER();

    gcmONERROR(gcEvaluateLoadtimeConstantExpresions(Context, vertexShaderBinary));
    gcmONERROR(gcEvaluateLoadtimeConstantExpresions(Context, fragmentShaderBinary));

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
} /* gcComputeLoadtimeConstant */

static gceSTATUS
_EvaluateUnioperator(
        IN gcSL_OPCODE    Opcode,
        IN LTCValue *     Source0Value,
        IN OUT LTCValue * ResultValue)
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT    i;
    gcmHEADER();

    /* handle operations which need multiple components together first */
    if (Opcode == gcSL_NORM)
    {
        /* gcSL_NORM:

           sqrt_s0 = sqrt(source0_x*source0_x + source0_y*source0_y + source0_z*source0_z)

           target_x = source0_x / sqrt_s0;
           target_y = source0_y / sqrt_s0;
           target_z = source0_z / sqrt_s0;
        */
        gctUINT sourceComponent_x;
        gctUINT sourceComponent_y;
        gctUINT sourceComponent_z;

        gcmASSERT((Source0Value->enable == gcSL_ENABLE_XYZ) &&
                  (ResultValue->enable == gcSL_ENABLE_XYZ)  );

        /* Get swizzled components */
        sourceComponent_x = _SwizzleSourceComponent(Source0Value->sourceInfo,
                                                    gcSL_SWIZZLE_X);
        sourceComponent_y = _SwizzleSourceComponent(Source0Value->sourceInfo,
                                                    gcSL_SWIZZLE_Y);
        sourceComponent_z = _SwizzleSourceComponent(Source0Value->sourceInfo,
                                                    gcSL_SWIZZLE_Z);

        if (ResultValue->elementType == gcSL_FLOAT)
        {
            /* get 3 components value */
            gctFLOAT source_x = Source0Value->v[sourceComponent_x].f32;
            gctFLOAT source_y = Source0Value->v[sourceComponent_y].f32;
            gctFLOAT source_z = Source0Value->v[sourceComponent_z].f32;
            gctFLOAT sqrt_s0 = gcoMATH_SquareRoot( source_x * source_x +
                                                   source_y * source_y +
                                                   source_z * source_z );

            if (sqrt_s0 == 0.0f)
            {
                /* x,y,z are all zero, the normal of the vector is NaN  */
                ResultValue->v[0].f32 = FLOAT_NaN;
                ResultValue->v[1].f32 = FLOAT_NaN;
                ResultValue->v[2].f32 = FLOAT_NaN;
            }
            else
            {
                /* evaluate 3 components value */
                ResultValue->v[0].f32 = source_x / sqrt_s0;
                ResultValue->v[1].f32 = source_y / sqrt_s0;
                ResultValue->v[2].f32 = source_z / sqrt_s0;
            } /* if */
        }
        else if (ResultValue->elementType == gcSL_INTEGER)
        {
            /* get 3 components value */
            gctINT source_x = Source0Value->v[sourceComponent_x].i32;
            gctINT source_y = Source0Value->v[sourceComponent_y].i32;
            gctINT source_z = Source0Value->v[sourceComponent_z].i32;
            gctINT sqrt_s0 = (gctINT) gcoMATH_SquareRoot( (gctFLOAT)(source_x * source_x +
                                                          source_y * source_y +
                                                          source_z * source_z) );

            if (sqrt_s0 == 0)
            {
                /* x,y,z are all zero, the normal of the vector is NaN  */
                ResultValue->v[0].i32 = INT32_MAX;
                ResultValue->v[1].i32 = INT32_MAX;
                ResultValue->v[2].i32 = INT32_MAX;
            }
            else
            {
                /* evaluate 3 components value */
                ResultValue->v[0].i32 = source_x / sqrt_s0;
                ResultValue->v[1].i32 = source_y / sqrt_s0;
                ResultValue->v[2].i32 = source_z / sqrt_s0;
            } /* if */
        }
        else
        {
            /* Error. */
            gcmFOOTER();
            return gcvSTATUS_INVALID_DATA;
        } /* if */
        gcmFOOTER();
        return status;
    } /* if */

    for (i=0; i<LTC_MAX_ELEMENTS; i++)
    {
        gctUINT sourceComponent;
        gctConstantValue v;
        if (!_isElementEnabled(Source0Value->enable, i)) {
            continue;
        }
        sourceComponent = _SwizzleSourceComponent(Source0Value->sourceInfo, i);
        v = Source0Value->v[sourceComponent]; /* get swizzled component */

        if (ResultValue->elementType == gcSL_FLOAT)
        {
            gctFLOAT f, f0;

            f = f0 = v.f32;

            switch (Opcode)
            {
            case gcSL_ABS:
                f = f0 > 0.0f ? f0 : -f0;
                break;
            case gcSL_RCP:
                if (f0 == 0.0f)
                {
                    f = FLOAT_NaN;
                }
                else
                {
                    f = gcoMATH_Divide(1.0f, f0);
                }
                break;
            case gcSL_FLOOR:
                f = gcoMATH_Floor(f0);
                break;
            case gcSL_CEIL:
                f = gcoMATH_Ceiling(f0);
                break;
            case gcSL_FRAC:
                f = gcoMATH_Add(f0, -gcoMATH_Floor(f0));
                break;
            case gcSL_LOG:
                f = gcoMATH_Log2(f0);
                break;
            case gcSL_RSQ:
                if (f0 == 0.0f || isF32NaN(f0))
                    f = FLOAT_NaN;
                else
                    f = gcoMATH_ReciprocalSquareRoot(f0);
                break;
            case gcSL_SAT:
                f = f0 < 0.0f ? 0.0f : (f > 1.0f ? 1.0f : f0);
                break;
            case gcSL_SIN:
                f = gcoMATH_Sine(f0);
                break;
            case gcSL_COS:
                f = gcoMATH_Cosine(f0);
                break;
            case gcSL_TAN:
                 f = gcoMATH_Tangent(f0);
               break;
            case gcSL_EXP:
                f = gcoMATH_Exp(f0);
                break;
            case gcSL_SIGN:
                f = (f0 > 0.0f) ? 1.0f :
                                 (f0 < 0.0f) ? -1.0f : 0.0f;
                break;
            case gcSL_SQRT:
                f = gcoMATH_SquareRoot(f0);
                break;
            case gcSL_ACOS:
                f = gcoMATH_ArcCosine(f0);
                break;
            case gcSL_ASIN:
                f = gcoMATH_ArcSine(f0);
                break;
            case gcSL_ATAN:
                f = gcoMATH_ArcTangent(f0);
                break;
            default:
                gcmASSERT(gcvFALSE);
                break;
            } /* switch */
            ResultValue->v[i].f32 = f;
        }
        else if (ResultValue->elementType == gcSL_INTEGER)
        {
            gctINT i, i0;

            i = i0 = v.i32;

            switch (Opcode)
            {
            case gcSL_ABS:
                i = gcmABS(i0);
                break;
            case gcSL_RCP:
                if (i0 == 0)
                {
                    /* for openGL, we should not generate NaN */
                    i = INT32_MAX;
                }
                else
                {
                    i = gcoMATH_Float2Int(gcoMATH_Divide(1.0f, gcoMATH_Int2Float(i0)));
                }
                break;
            case gcSL_FLOOR:
                i = i0;
                break;
            case gcSL_CEIL:
                i = i0;
                break;
            case gcSL_FRAC:
                i = 0;
                break;
            case gcSL_LOG:
                i = gcoMATH_Float2Int(gcoMATH_Log2(gcoMATH_Int2Float(i0)));
                break;
            case gcSL_RSQ:
                i = gcoMATH_Float2Int(
                        gcoMATH_ReciprocalSquareRoot(
                            gcoMATH_Int2Float(i0)));
                break;
            case gcSL_SAT:
                i = i0 < 0 ? 0 : (i0 > 1 ? 1 : i0);
                break;
            default:
                gcmASSERT(gcvFALSE);
                break;
            } /* switch */

            ResultValue->v[i].i32 = i;
        }
        else
        {
            /* Error. */
            return gcvSTATUS_INVALID_DATA;
        } /* if */
    } /* for */

    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _EvaluateUnioperator */

static gceSTATUS
_EvaluateBioperator(
        IN gcSL_OPCODE    Opcode,
        IN LTCValue *     Source0Value,
        IN LTCValue *     Source1Value,
        OUT LTCValue *    ResultValue)
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT source0Component_x;
    gctUINT source0Component_y;
    gctUINT source0Component_z;
    gctUINT source0Component_w;
    gctUINT source1Component_x;
    gctUINT source1Component_y;
    gctUINT source1Component_z;
    gctUINT source1Component_w;
    gctINT  i;

    gcmHEADER();

    /* handle operations which need multiple components together first */
    switch (Opcode)
    {
    case gcSL_DP3:
        /* gcSL_DP3:

           target_x = target_y = target_z =
               source0_x*source1_x + source0_y*source1_y + source0_z*source1_z;

        */

        gcmASSERT(Source0Value->enable == gcSL_ENABLE_XYZ &&
                  Source1Value->enable == gcSL_ENABLE_XYZ &&
                  ResultValue->enable == gcSL_ENABLE_XYZ);

        /* Get swizzled components */
        source0Component_x =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_X);
        source0Component_y =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_Y);
        source0Component_z =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_Z);

        source1Component_x =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_X);
        source1Component_y =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_Y);
        source1Component_z =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_Z);

        if (ResultValue->elementType == gcSL_FLOAT)
        {
            /* get 3 components value */
            gctFLOAT source0_x = Source0Value->v[source0Component_x].f32;
            gctFLOAT source0_y = Source0Value->v[source0Component_y].f32;
            gctFLOAT source0_z = Source0Value->v[source0Component_z].f32;

            gctFLOAT source1_x = Source1Value->v[source1Component_x].f32;
            gctFLOAT source1_y = Source1Value->v[source1Component_y].f32;
            gctFLOAT source1_z = Source1Value->v[source1Component_z].f32;

            ResultValue->v[0].f32 =
                ResultValue->v[1].f32 =
                ResultValue->v[2].f32 =
                               ( source0_x * source1_x +
                                 source0_y * source1_y +
                                 source0_z * source1_z );
        }
        else if (ResultValue->elementType  == gcSL_INTEGER)
        {
            /* get 3 components value */
            gctINT source0_x = Source0Value->v[source0Component_x].i32;
            gctINT source0_y = Source0Value->v[source0Component_y].i32;
            gctINT source0_z = Source0Value->v[source0Component_z].i32;

            gctINT source1_x = Source1Value->v[source1Component_x].i32;
            gctINT source1_y = Source1Value->v[source1Component_y].i32;
            gctINT source1_z = Source1Value->v[source1Component_z].i32;

            ResultValue->v[0].i32 =
                ResultValue->v[1].i32 =
                ResultValue->v[2].i32 = ( source0_x * source1_x +
                                          source0_y * source1_y +
                                          source0_z * source1_z );
        }
        else
        {
            /* Error. */
            status = gcvSTATUS_INVALID_DATA;
        } /* if */
        gcmFOOTER();
        return status;
    case gcSL_DP4:
        /* gcSL_DP4:

           target_x = target_y = target_z = target_w
               source0_x*source1_x + source0_y*source1_y +
               source0_z*source1_z + source0_w*source1_w ;

        */

        gcmASSERT(Source0Value->enable == gcSL_ENABLE_XYZW &&
                  Source1Value->enable == gcSL_ENABLE_XYZW &&
                  ResultValue->enable == gcSL_ENABLE_XYZW);

        /* Get swizzled components */
        source0Component_x =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_X);
        source0Component_y =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_Y);
        source0Component_z =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_Z);
        source0Component_w =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_W);

        source1Component_x =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_X);
        source1Component_y =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_Y);
        source1Component_z =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_Z);
        source1Component_w =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_W);

        if (ResultValue->elementType == gcSL_FLOAT)
        {
            /* get 4 components value */
            gctFLOAT source0_x = Source0Value->v[source0Component_x].f32;
            gctFLOAT source0_y = Source0Value->v[source0Component_y].f32;
            gctFLOAT source0_z = Source0Value->v[source0Component_z].f32;
            gctFLOAT source0_w = Source0Value->v[source0Component_w].f32;

            gctFLOAT source1_x = Source1Value->v[source1Component_x].f32;
            gctFLOAT source1_y = Source1Value->v[source1Component_y].f32;
            gctFLOAT source1_z = Source1Value->v[source1Component_z].f32;
            gctFLOAT source1_w = Source1Value->v[source1Component_w].f32;

            ResultValue->v[0].f32 =
                ResultValue->v[1].f32 =
                ResultValue->v[2].f32 =
                ResultValue->v[3].f32 =
                               ( source0_x * source1_x +
                                 source0_y * source1_y +
                                 source0_z * source1_z +
                                 source0_w * source1_w);
        }
        else if (ResultValue->elementType == gcSL_INTEGER)
        {
            /* get 4 components value */
            gctINT source0_x = Source0Value->v[source0Component_x].i32;
            gctINT source0_y = Source0Value->v[source0Component_y].i32;
            gctINT source0_z = Source0Value->v[source0Component_z].i32;
            gctINT source0_w = Source0Value->v[source0Component_w].i32;

            gctINT source1_x = Source1Value->v[source1Component_x].i32;
            gctINT source1_y = Source1Value->v[source1Component_y].i32;
            gctINT source1_z = Source1Value->v[source1Component_z].i32;
            gctINT source1_w = Source1Value->v[source1Component_w].i32;

            ResultValue->v[0].i32 =
                ResultValue->v[1].i32 =
                ResultValue->v[2].i32 =
                ResultValue->v[3].i32 = ( source0_x * source1_x +
                                          source0_y * source1_y +
                                          source0_z * source1_z +
                                          source0_w * source1_w);
        }
        else
        {
            /* Error. */
            status = gcvSTATUS_INVALID_DATA;
        } /* if */
        gcmFOOTER();
        return status;
    case gcSL_CROSS:
        /* gcSL_CROSS:

           target_x = source0_y*source1_z - source0_z*source1_y;
           target_y = source0_z*source1_x - source0_x*source1_z;
           target_z = source0_x*source1_y - source0_y*source1_x;
        */

        gcmASSERT(Source0Value->enable == gcSL_ENABLE_XYZ &&
                  Source1Value->enable == gcSL_ENABLE_XYZ &&
                  ResultValue->enable == gcSL_ENABLE_XYZ);

        /* Get swizzled components */
        source0Component_x =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_X);
        source0Component_y =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_Y);
        source0Component_z =
            _SwizzleSourceComponent(Source0Value->sourceInfo, gcSL_SWIZZLE_Z);

        source1Component_x =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_X);
        source1Component_y =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_Y);
        source1Component_z =
            _SwizzleSourceComponent(Source1Value->sourceInfo, gcSL_SWIZZLE_Z);

        if (ResultValue->elementType == gcSL_FLOAT)
        {
            /* get 4 components value */
            gctFLOAT source0_x = Source0Value->v[source0Component_x].f32;
            gctFLOAT source0_y = Source0Value->v[source0Component_y].f32;
            gctFLOAT source0_z = Source0Value->v[source0Component_z].f32;

            gctFLOAT source1_x = Source1Value->v[source1Component_x].f32;
            gctFLOAT source1_y = Source1Value->v[source1Component_y].f32;
            gctFLOAT source1_z = Source1Value->v[source1Component_z].f32;

            ResultValue->v[0].f32 = source0_y*source1_z - source0_z*source1_y;
            ResultValue->v[1].f32 = source0_z*source1_x - source0_x*source1_z;
            ResultValue->v[2].f32 = source0_x*source1_y - source0_y*source1_x;
        }
        else if (ResultValue->elementType == gcSL_INTEGER)
        {
            /* get 3 components value */
            gctINT source0_x = Source0Value->v[source0Component_x].i32;
            gctINT source0_y = Source0Value->v[source0Component_y].i32;
            gctINT source0_z = Source0Value->v[source0Component_z].i32;

            gctINT source1_x = Source1Value->v[source1Component_x].i32;
            gctINT source1_y = Source1Value->v[source1Component_y].i32;
            gctINT source1_z = Source1Value->v[source1Component_z].i32;

            ResultValue->v[0].i32 = source0_y*source1_z - source0_z*source1_y;
            ResultValue->v[1].i32 = source0_z*source1_x - source0_x*source1_z;
            ResultValue->v[2].i32 = source0_x*source1_y - source0_y*source1_x;
        }
        else
        {
            /* Error. */
            status = gcvSTATUS_INVALID_DATA;
        } /* if */

        gcmFOOTER();
        return status;
    default:
        break;
    }
    for (i=0; i<LTC_MAX_ELEMENTS; i++)
    {
        gctUINT source0Component, source1Component;
        gctConstantValue v0, v1;

        if (!_isElementEnabled(Source0Value->enable, i)) {
            continue;
        }
        source0Component =
            _SwizzleSourceComponent(Source0Value->sourceInfo, i);
        /* get swizzled component */
        v0 = Source0Value->v[source0Component];

        source1Component =
            _SwizzleSourceComponent(Source1Value->sourceInfo, i);
        /* get swizzled component */
        v1 = Source1Value->v[source1Component];

        if (ResultValue->elementType == gcSL_FLOAT)
        {
            gctFLOAT f, f0, f1;

            f = f0 = v0.f32;
            f1 = v1.f32;

            switch (Opcode)
            {
            case gcSL_ADD:
                f = gcoMATH_Add(f0, f1);
                break;
            case gcSL_SUB:
                f = gcoMATH_Add(f0, -f1);
                break;
            case gcSL_MUL:
                f = gcoMATH_Multiply(f0, f1);
                break;
            case gcSL_MAX:
                f = f0 > f1 ? f0 : f1;
                break;
            case gcSL_MIN:
                f = f0 < f1 ? f0 : f1;
                break;
            case gcSL_POW:
                f = gcoMATH_Power(f0, f1);
                break;
            case gcSL_STEP:
                f = f0 > f1 ? 0.0f : 1.0f;
                break;

            default:
                gcmASSERT(gcvFALSE);
                break;
            } /* switch */
            ResultValue->v[i].f32 = f;
        }
        else if (ResultValue->elementType == gcSL_INTEGER)
        {
            gctINT i, i0, i1;

            i = i0 = v0.i32;
            i1 = v1.i32;

            switch (Opcode)
            {
            case gcSL_ADD:
                i = i0 + i1; break;
            case gcSL_SUB:
                i = i0 - i1; break;
            case gcSL_MUL:
                i = i0 * i1; break;
            case gcSL_MAX:
                i = i0 > i1 ? i0 : i1; break;
            case gcSL_MIN:
                i = i0 < i1 ? i0 : i1; break;
            case gcSL_POW:
                i = gcoMATH_Float2Int(
                            gcoMATH_Power(gcoMATH_Int2Float(i0),
                                          gcoMATH_Int2Float(i1)));
                break;
            default:
                gcmASSERT(gcvFALSE);
                break;
            } /* switch */

            ResultValue->v[i].i32 = i;
        }
        else
        {
            /* Error. */
            return gcvSTATUS_INVALID_DATA;
        } /* if */
    } /* for */

    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _EvaluateBioperator */

/* copy enabled components in ResultValue to the InstructionIndex
 * and ResultLocation in the ResultArray
 */
static gceSTATUS
_SetValues(
       IN OUT LTCValue * ResultsArray,
       IN LTCValue *     ResultValue,
       IN gctUINT        InstructionIndex,
       IN gctUINT        ResultLocation
       )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctINT    i;

    gcmHEADER();

    ResultsArray[InstructionIndex].elementType = ResultValue->elementType;
    ResultsArray[InstructionIndex].enable |= ResultValue->enable;
    ResultsArray[InstructionIndex].sourceInfo = ResultValue->sourceInfo;
    ResultsArray[InstructionIndex].instructionIndex = InstructionIndex;
    if (ResultLocation != InstructionIndex)
    {
        ResultsArray[ResultLocation].elementType = ResultValue->elementType;
        ResultsArray[ResultLocation].enable |= ResultValue->enable;
        ResultsArray[ResultLocation].sourceInfo = ResultValue->sourceInfo;
        ResultsArray[ResultLocation].instructionIndex = InstructionIndex;
    }

    for (i=0; i<LTC_MAX_ELEMENTS; i++)
    {
        if (!_isElementEnabled(ResultValue->enable, i)) {
            continue;
        }
        if (ResultValue->elementType == gcSL_FLOAT)
        {
            ResultsArray[InstructionIndex].v[i].f32 = ResultValue->v[i].f32;
            if (ResultLocation != InstructionIndex)
            {
                ResultsArray[ResultLocation].v[i].f32 = ResultValue->v[i].f32;
            }
        }
        else if (ResultValue->elementType == gcSL_INTEGER)
        {
            ResultsArray[InstructionIndex].v[i].i32 = ResultValue->v[i].i32;
            if (ResultLocation != InstructionIndex)
            {
                ResultsArray[ResultLocation].v[i].i32 = ResultValue->v[i].i32;
            }
        }
        else
        {
            gcmASSERT(gcvFALSE);
        }
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _SetValues */

gctINT
_enabledComponents(
        IN gcSL_ENABLE       Enable
        )
{
    gctINT n = 0;
    if (0 != (Enable & gcSL_ENABLE_X))
        n++;

    if (0 != (Enable & gcSL_ENABLE_Y))
        n++;

    if (0 != (Enable & gcSL_ENABLE_Z))
        n++;

    if (0 != (Enable & gcSL_ENABLE_W))
        n++;

    return n;
} /* _enabledComponents */

/* store the Value to Shader's uniform at LtcUniformIndex */
static gceSTATUS
_StoreValueToDummyUniform(
       IN LTCValue *            Value,
       IN GLContext             Context,
       IN gcSHADER              Shader,
       IN gctINT                LtcUniformIndex
       )
{
    gceSTATUS      status = gcvSTATUS_OK;
    GLProgram      program;
    GLUniform      glUniform;
    gctINT         glUniformIndex;
    gctINT         i = 0;
    gcSL_ENABLE    enable = Value->enable;
    gctBOOL        isLTCUniformPrivate;
    GLenum         error = GL_NO_ERROR;

    gcmHEADER();

    gcmASSERT(   LtcUniformIndex >= 0
              && LtcUniformIndex < (gctINT) Shader->uniformCount
              && Shader->uniforms[LtcUniformIndex]->glUniformIndex != (gctUINT16) -1 );

    glUniformIndex = Shader->uniforms[LtcUniformIndex]->glUniformIndex;
    program = Context->program;
    isLTCUniformPrivate =  (Shader->uniforms[LtcUniformIndex]->name[0] == '#');
    glUniform = isLTCUniformPrivate ? &program->privateUniforms[glUniformIndex]
                                    : &program->uniforms[glUniformIndex];

    gcmASSERT(glUniform->uniform[0] == Shader->uniforms[LtcUniformIndex]);

    if (Value->elementType == gcSL_FLOAT)
    {
        gctFLOAT  f32[4];
        if (0 != (enable & gcSL_ENABLE_X))
            f32[i++] = Value->v[0].f32;

        if (0 != (enable & gcSL_ENABLE_Y))
            f32[i++] = Value->v[1].f32;

        if (0 != (enable & gcSL_ENABLE_Z))
            f32[i++] = Value->v[2].f32;

        if (0 != (enable & gcSL_ENABLE_W))
            f32[i++] = Value->v[3].f32;

        error = _SetUniformData(program,
                                glUniform,
                                glUniform->uniform[0]->type,
                                1  /* Count */,
                                0  /* Index */,
                                f32);
    }
    else if (Value->elementType == gcSL_INTEGER)
    {
        gctINT  i32[4];
        if (0 != (enable & gcSL_ENABLE_X))
            i32[i++] = Value->v[0].i32;

        if (0 != (enable & gcSL_ENABLE_Y))
            i32[i++] = Value->v[1].i32;

        if (0 != (enable & gcSL_ENABLE_Z))
            i32[i++] = Value->v[2].i32;

        if (0 != (enable & gcSL_ENABLE_W))
            i32[i++] = Value->v[3].i32;

        error = _SetUniformData(program,
                                glUniform,
                                glUniform->uniform[0]->type,
                                1  /* Count */,
                                0  /* Index */,
                                i32);
    }
    else
    {
        gcmASSERT(gcvFALSE);
    } /* if */

    if (error == GL_NO_ERROR)
    {
       gcmASSERT(error == GL_NO_ERROR);
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _StoreValueToDummyUniform */


static void
_dumpLTCValue(
       IN LTCValue *            Value,
       IN OUT gctSTRING         Buffer,
       IN gctUINT               BufferLen,
       IN OUT gctUINT *         Offset
       )
{
    gctINT i = 0;

    gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferLen, Offset, "[ "));

    for (;i<LTC_MAX_ELEMENTS; i++)
    {
		gcmVERIFY_OK(
			gcoOS_PrintStrSafe(Buffer, BufferLen, Offset,
							   "%10.6f", Value->v[i].f32));
        if (i != LTC_MAX_ELEMENTS -1 )
		    gcmVERIFY_OK(
			    gcoOS_PrintStrSafe(Buffer, BufferLen, Offset,
							       ", "));
    }
    gcmVERIFY_OK(gcoOS_PrintStrSafe(Buffer, BufferLen, Offset, " ]"));
}

/***************************************************************************
**                   _EvaluateLoadtimeConstantExpresion
****************************************************************************
**
**   For instruction number 'InstructionIndex' in the Shader, evaluate the
**   instruction and store the evaluated constant value into Results
**
*/
static gceSTATUS
_EvaluateLoadtimeConstantExpresion(
       IN GLContext             Context,
       IN gcSHADER              Shader,
       IN gctUINT               InstructionIndex,
       IN LTCValue *            Results
       )
{
    gceSTATUS               status = gcvSTATUS_OK;
    gcSL_INSTRUCTION        instruction;
    gctINT                  ltcUniformIndex;
    gctINT                  sources = 0;    /* number of sources */
    gcmHEADER();

    gcmASSERT(InstructionIndex <= Shader->ltcInstructionCount);

    instruction = &Shader->ltcExpressions[InstructionIndex];
    ltcUniformIndex = Shader->ltcCodeUniformIndex[InstructionIndex];

    /* for each load time constant, mark it. */
    do
    {
        /* get the source0 and source1 */
        LTCValue  source0Value;
        LTCValue  source1Value;
        LTCValue  resultValue;
        gctUINT    resultLocation;

        if (gcDumpOption(gceLTC_DUMP_EVALUATION))
            dbg_dumpIR(instruction, InstructionIndex);

        gcmONERROR(_GetSourceValue(Context, Shader,
                                   instruction, 0,
                                   Results, &source0Value));

        gcmONERROR(_GetSourceValue(Context, Shader,
                                   instruction, 1,
                                   Results, &source1Value));
        /* setup the enable for the result value */
        resultValue.enable = gcmSL_TARGET_GET(instruction->temp, Enable);
        resultValue.elementType = gcmSL_TARGET_GET(instruction->temp, Format);

        /* get result location from the instruction temp index */
        resultLocation = instruction->tempIndex;
        /* if the result is indexed, we should put the result value in the
           instruction's value location:

           for example,

             005  ADD temp(0), U1, U2

           the result will be put in location {0} and {5} both places

           and

            006  MOV temp(0).1, temp(1)

           the result will only be put at {6}, we do not handle indexed
           register
        */
        if (gcmSL_TARGET_GET(instruction->temp, Indexed) != gcSL_NOT_INDEXED)
        {
            resultLocation = InstructionIndex;
        }

        switch (instruction->opcode)
        {
        case gcSL_MOV:
            /* mov instruction */
            resultValue = source0Value;
            _SetValues(Results, &resultValue,
                       InstructionIndex, resultLocation);
            sources = 1;
            break;
        case gcSL_RCP:
        case gcSL_ABS:
        case gcSL_FLOOR:
        case gcSL_CEIL:
        case gcSL_FRAC:
        case gcSL_LOG:
        case gcSL_RSQ:
        case gcSL_SAT:
        case gcSL_NORM:
        case gcSL_SIN:
        case gcSL_COS:
        case gcSL_TAN:
        case gcSL_EXP:
        case gcSL_SIGN:
        case gcSL_SQRT:
        case gcSL_ACOS:
        case gcSL_ASIN:
        case gcSL_ATAN:
             /* One-operand computational instruction. */
            gcmONERROR(_EvaluateUnioperator(instruction->opcode,
                                            &source0Value,
                                            &resultValue));
            _SetValues(Results, &resultValue,
                       InstructionIndex, resultLocation);
            sources = 1;
            break;

        case gcSL_ADD:
        case gcSL_SUB:
        case gcSL_MUL:
        case gcSL_MAX:
        case gcSL_MIN:
        case gcSL_POW:
        case gcSL_DP3:
        case gcSL_DP4:
        case gcSL_CROSS:
        case gcSL_STEP:
            /* Two-operand computational instruction. */
            gcmONERROR(_EvaluateBioperator(instruction->opcode,
                                           &source0Value,
                                           &source1Value,
                                           &resultValue));
            _SetValues(Results, &resultValue,
                       InstructionIndex, resultLocation);
            sources = 2;
            break;

        case gcSL_SET:
        case gcSL_DSX:
        case gcSL_DSY:
        case gcSL_FWIDTH:
        case gcSL_DIV:
        case gcSL_MOD:
        case gcSL_AND_BITWISE:
        case gcSL_OR_BITWISE:
        case gcSL_XOR_BITWISE:
        case gcSL_NOT_BITWISE:
        case gcSL_LSHIFT:
        case gcSL_RSHIFT:
        case gcSL_ROTATE:
        case gcSL_BITSEL:
        case gcSL_LEADZERO:
        case gcSL_ADDLO:
        case gcSL_MULLO:
        case gcSL_CONV:
        case gcSL_MULHI:
        case gcSL_CMP:
        case gcSL_I2F:
        case gcSL_F2I:
        case gcSL_ADDSAT:
        case gcSL_SUBSAT:
        case gcSL_MULSAT:
            /* these operators are not supported by constant folding
               need to add handling for them */

        case gcSL_CALL:
        case gcSL_JMP:
        case gcSL_RET:
        case gcSL_NOP:
        case gcSL_KILL:

        case gcSL_TEXBIAS:
        case gcSL_TEXGRAD:
        case gcSL_TEXLOD:

        case gcSL_TEXLD:
        case gcSL_TEXLDP:

        default:
            gcmASSERT(gcvFALSE);
            break;
        }
        if (sources != 0 && gcDumpOption(gceLTC_DUMP_EVALUATION))
        {
            gctCHAR  buffer[512];
            gctUINT  offset = 0;

            /* dump source 0 */
            gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                            "    source0 = "));
            _dumpLTCValue(&source0Value, buffer, gcmSIZEOF(buffer), &offset);
	        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER, "%s", buffer);
            offset = 0;

            /* dump source 1 */
            if (sources > 1)
            {
                gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                               "    source1 = "));
                _dumpLTCValue(&source1Value, buffer, gcmSIZEOF(buffer), &offset);
	            gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER, "%s", buffer);
                offset = 0;
            }

            /* dump result */
            gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                                            "    result  = "));
            _dumpLTCValue(&resultValue, buffer, gcmSIZEOF(buffer), &offset);
	        gcmTRACE_ZONE(gcvLEVEL_INFO, gcvZONE_COMPILER, "%s", buffer);
            offset = 0;
        }

        /* check if the result need to store to dummy uniform */
        if (ltcUniformIndex != -1)
        {
            /* the instruction's result is a dummy uniform value ,
             * need to store the value to the uniform */
            gcmONERROR(_StoreValueToDummyUniform(&resultValue,
                                                 Context,
                                                 Shader,
                                                 ltcUniformIndex));
        }
        /*  dump evaluated uniform value */
    } while (gcvFALSE);

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _EvaluateLoadtimeConstantExpresion */

#endif /* GC_ENABLE_LOADTIME_OPT */
