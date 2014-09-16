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


#include "gc_glsl_compiler.h"

/*******************************************************************************
***** Version Signature *******************************************************/

#define _gcmTXT2STR(t) #t
#define gcmTXT2STR(t) _gcmTXT2STR(t)
const char * _GLESV2SC_VERSION = "\n\0$VERSION$"
                                 gcmTXT2STR(gcvVERSION_MAJOR) "."
                                 gcmTXT2STR(gcvVERSION_MINOR) "."
                                 gcmTXT2STR(gcvVERSION_PATCH) ":"
                                 gcmTXT2STR(gcvVERSION_BUILD)
                                 "$\n";

#define gcdUSE_PRECOMPILED_SHADERS     1

#if gcdUSE_PRECOMPILED_SHADERS
typedef struct _lookup
{
    const gctUINT sourceSize;
    const char * source1;
    const char * source2;
    gceSTATUS (*function)(gcSHADER Shader);
}
lookup;

#include "gc_glsl_util.h"
#include "gc_glsl_precompiled_shaders.h"
#endif

#define gcdDUMP_SHADER  0  /* use VC_DUMP_SHADER_SOURCE=1 env variable to dump shader source */

static gctCONST_STRING shaderKindNames[] =
{
    "UNKNOWN",
    "VERTEX",
    "FRAGMENT",
    "CL",
    "PRECOMPILED"
};

gctCONST_STRING
gcmShaderName(
    IN gctINT ShaderKind
    )
{
    gcmASSERT(ShaderKind >= gcSHADER_TYPE_UNKNOWN &&  ShaderKind < gcSHADER_KIND_COUNT);
    return shaderKindNames[ShaderKind];
}


/*******************************************************************************
**                              gcCompileShader
********************************************************************************
**
**  Compile a shader.
**
**  INPUT:
**
**      gcoOS Hal
**          Pointer to an gcoHAL object.
**
**      gctINT ShaderType
**          Shader type to compile.  Can be one of the following values:
**
**              gcSHADER_TYPE_VERTEX
**                  Compile a vertex shader.
**
**              gcSHADER_TYPE_FRAGMENT
**                  Compile a fragment shader.
**
**		gcSHADER_TYPE_PRECOMPILED
**                  Precompiled shader
**
**      gctSIZE_T SourceSize
**          Size of the source buffer in bytes.
**
**      gctCONST_STRING Source
**          Pointer to the buffer containing the shader source code.
**
**  OUTPUT:
**
**      gcSHADER * Binary
**          Pointer to a variable receiving the pointer to a gcSHADER object
**          containg the compiled shader code.
**
**      gctSTRING * Log
**          Pointer to a variable receiving a string pointer containging the
**          compile log.
*/
gceSTATUS
gcCompileShader(
    IN gcoHAL Hal,
    IN gctINT ShaderType,
    IN gctSIZE_T SourceSize,
    IN gctCONST_STRING Source,
    OUT gcSHADER * Binary,
    OUT gctSTRING * Log
    )
{
    gceSTATUS status;
    sloCOMPILER compiler = gcvNULL;
    static gctBOOL dumpShader =
#if gcdDUMP_SHADER
                                 gcvTRUE;
#else
                                 gcvFALSE;
#endif
    static gctBOOL firstTime = 1;

    gcmHEADER_ARG("Hal=0x%x ShaderType=%d SourceSize=%lu Source=0x%x",
                  Hal, ShaderType, SourceSize, Source);

    if (firstTime)
    {
        /* check environment variable GC_DUMP_SHADER_SOURCE */
        char* p = gcvNULL;
        gcoOS_GetEnv(gcvNULL,
                    "VC_DUMP_SHADER_SOURCE",
                    &p);
        if (p)
        {
            if (gcmIS_SUCCESS(gcoOS_StrCmp(p, "1")) ||
                gcmIS_SUCCESS(gcoOS_StrCmp(p, "on")) ||
                gcmIS_SUCCESS(gcoOS_StrCmp(p, "ON")) )
            {
                dumpShader = gcvTRUE;
            }
            else
            {
                dumpShader = gcvFALSE;
            }
        }
        firstTime = 0;
    }

    if (dumpShader)
    {
        gctCHAR buffer[512];
        gctUINT offset = 0;
        gctSIZE_T n;
        gcmTRACE(gcvLEVEL_ERROR, "===== [ Incoming %s shader source ] =====", gcmShaderName(ShaderType));
        for (n = 0; n < SourceSize; ++n)
        {
            if ((Source[n] == '\n')
            ||  (offset == gcmSIZEOF(buffer) - 2)
            )
            {
                if (Source[n] != '\n')
                {
                    /* take care the last char */
                    buffer[offset++] = Source[n];
                }
                buffer[offset] = '\0';
                gcmTRACE(gcvLEVEL_ERROR, "%s", buffer);
                offset = 0;
            }
            else
            {
                /* don't print \r, which may be from a DOS format file */
                if (Source[n] != '\r')
                    buffer[offset++] = Source[n];
            }
        }
        if (offset != 0)
        {
            buffer[offset] = '\0';
            gcmTRACE(gcvLEVEL_ERROR, "%s", buffer);
        }
    }

    if(ShaderType == gcSHADER_TYPE_PRECOMPILED)
    {
	    /* Construct a new gcSHADER object. */
	    gcmONERROR(gcSHADER_Construct(Hal,
	                                  ShaderType,
	                                  Binary));

        /* A precompiled shader */
        gcmONERROR(gcSHADER_SetCompilerVersion(*Binary,
                                               sloCOMPILER_GetVersion(ShaderType)));
        gcmONERROR(gcSHADER_Load(*Binary,
                                 (gctPOINTER) Source,
                                 SourceSize));
    }
    else
    {
#if gcdUSE_PRECOMPILED_SHADERS
        if (SourceSize >= 604)
        {
            const char *p, *q;
            lookup * lookup;
            gctBOOL useSource1;

            /* Look up any hand compiled shader. */
            for (lookup = compiledShaders; lookup->function != gcvNULL; ++lookup)
            {
                if (SourceSize < lookup->sourceSize)
                {
                    continue;
                }

                p = Source;
                q = lookup->source1;
                useSource1 = gcvTRUE;

                while (*p)
                {
                    if (*p == *q)
                    {
                        p++;
                        q++;
                    }
                    else if (*p == ' ' || *p == '\r' || *p == '\n' || *p == '\t')
                    {
                        p++;
                    }
                    else if (*q == ' ' || *q == '\r' || *q == '\n' || *q == '\t')
                    {
                        q++;
                    }
                    else if (*q == '\0' && useSource1 && lookup->source2)
                    {
                        q = lookup->source2;
                        useSource1 = gcvFALSE;
                    }
                    else
                    {
                        break;
                    }
                }

                if (*p == '\0' && *q == '\0')
                {
				    /* Construct a new gcSHADER object. */
				    gcmONERROR(gcSHADER_Construct(Hal,
				                                  ShaderType,
				                                  Binary));

                    /* Create the shader dynamically. */
                    gcmONERROR((*lookup->function)(*Binary));

                    /* Success. */
                    gcmFOOTER_ARG("*Binary=0x%x *Log=0x%x", *Binary, *Log);
                    return gcvSTATUS_OK;
                }
            }
        }
#endif /* gcdUSE_PRECOMPILED_SHADERS */
        {
            gcmONERROR(sloCOMPILER_Construct(Hal,
                                             (sleSHADER_TYPE) ShaderType,
                                             &compiler));

            gcmONERROR(sloCOMPILER_Compile(compiler,
                                           slvOPTIMIZATION_ALL,
                                           slmDUMP_OPTIONS,
                                           1,
                                           &Source,
                                           Binary,
                                           Log));

            gcmONERROR(sloCOMPILER_Destroy(compiler));
        }
    }

    /* Success. */
    gcmFOOTER_ARG("*Binary=0x%x *Log=0x%x", *Binary, *Log);
    return gcvSTATUS_OK;

OnError:
    if (compiler != gcvNULL)
    {
        /* Delete sloCOMPILER object. */
       gcmVERIFY_OK(sloCOMPILER_Destroy(compiler));
    }

    if (*Binary)
    {
        /* Destroy bad binary. */
        gcmVERIFY_OK(gcSHADER_Destroy(*Binary));
        *Binary = gcvNULL;
    }

    /* Return the status. */
    gcmFOOTER();
    return status;
}
