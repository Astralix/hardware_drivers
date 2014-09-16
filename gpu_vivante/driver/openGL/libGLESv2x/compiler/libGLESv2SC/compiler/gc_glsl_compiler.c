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
#include "gc_hal_user.h"
#include "gc_glsl_preprocessor.h"
#include "gc_glsl_parser.h"
#include "gc_glsl_ast_walk.h"
#include "gc_glsl_emit_code.h"

#define _sldLanguageType  gcmCC('E', 'S', '\0', '\0')
static gctUINT32 _slCompilerVersion[2] = { _sldLanguageType,
			                   gcmCC('\0','\0','\1','\1')};

/* sloCOMPILER object. */
struct _sloCOMPILER
{
    slsOBJECT                   object;

    gcoHAL                      hal;

    sleSHADER_TYPE              shaderType;

    gcSHADER                    binary;

    gctSTRING                   log;

    gctUINT                     logBufSize;

    slsDLINK_LIST               memoryPool;

    struct
    {
        gctUINT16               errorCount;

        gctUINT16               warnCount;

        slsHASH_TABLE           stringPool;

        sltOPTIMIZATION_OPTIONS optimizationOptions;

        sltEXTENSIONS           extensions;

        sltDUMP_OPTIONS         dumpOptions;

        sleSCANNER_STATE        scannerState;

        gctUINT                 stringCount;

        gctCONST_STRING *       strings;

        gctUINT                 currentLineNo;

        gctUINT                 currentStringNo;

        gctUINT                 currentCharNo;

        slsDLINK_LIST           dataTypes;

        slsNAME_SPACE *         builtinSpace;

        slsNAME_SPACE *         globalSpace;

        slsNAME_SPACE *         currentSpace;

        sloIR_SET               rootSet;

        gctUINT                 tempRegCount;

        gctUINT                 labelCount;

        gctBOOL                 loadingBuiltIns;

        gctBOOL                 mainDefined;
    }
    context;

    sloPREPROCESSOR             preprocessor;

    sloCODE_EMITTER             codeEmitter;
};

typedef struct _slsPOOL_STRING_NODE
{
    slsDLINK_NODE       node;

    sltPOOL_STRING      string;
}
slsPOOL_STRING_NODE;

static gcsATOM_PTR  CompilerLockRef = gcvNULL;
static gctPOINTER   CompilerLock;

gceSTATUS
sloCOMPILER_Construct(
    IN gcoHAL Hal,
    IN sleSHADER_TYPE ShaderType,
    OUT sloCOMPILER * Compiler
    )
{
    gceSTATUS status;
    sloCOMPILER compiler = gcvNULL;
    gctINT32 reference;

    gcmHEADER_ARG("Hal=0x%x ShaderType=%d", Hal, ShaderType);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(Compiler);

    do
    {
        gctPOINTER pointer = gcvNULL;

        if (CompilerLockRef == gcvNULL)
        {
            /* Create a new reference counter. */
            gcmERR_BREAK(gcoOS_AtomConstruct(gcvNULL, &CompilerLockRef));
        }

        /* Increment the reference counter */
        gcmERR_BREAK(gcoOS_AtomIncrement(gcvNULL, CompilerLockRef, &reference));

        if (reference == 0)
        {
            /* Create a global lock. */
            status = gcoOS_CreateMutex(gcvNULL, &CompilerLock);

            if (gcmIS_ERROR(status))
            {
                CompilerLock = gcvNULL;
                break;
            }
        }

        /* Allocate memory for sloCOMPILER object */
        status = gcoOS_Allocate(
                                gcvNULL,
                                sizeof(struct _sloCOMPILER),
                                &pointer
                                );

        if (gcmIS_ERROR(status))
        {
            compiler = gcvNULL;
            break;
        }

        compiler = pointer;

        /* Initialize members */
        compiler->object.type               = slvOBJ_COMPILER;
        compiler->hal                       = Hal;
        compiler->shaderType                = ShaderType;
        compiler->binary                    = gcvNULL;
        compiler->log                       = gcvNULL;
        compiler->logBufSize                = 0;

        slsDLINK_LIST_Initialize(&compiler->memoryPool);

        compiler->context.errorCount    = 0;
        compiler->context.warnCount = 0;

        compiler->context.tempRegCount  = 0;
        compiler->context.labelCount    = 0;

        compiler->context.loadingBuiltIns   = gcvFALSE;
        compiler->context.mainDefined       = gcvFALSE;

        slsHASH_TABLE_Initialize(&compiler->context.stringPool);

        compiler->context.stringCount       = 0;
        compiler->context.strings           = gcvNULL;

        slsDLINK_LIST_Initialize(&compiler->context.dataTypes);

        /* Create built-in name space */
        status = slsNAME_SPACE_Construct(
                                        compiler,
                                        gcvNULL,
                                        &compiler->context.builtinSpace);

        if (gcmIS_ERROR(status))
        {
            gcmASSERT(compiler->context.builtinSpace == gcvNULL);
            break;
        }

        compiler->context.currentSpace = compiler->context.builtinSpace;

        /* Create global name space */
        status = slsNAME_SPACE_Construct(
                                        compiler,
                                        compiler->context.builtinSpace,
                                        &compiler->context.globalSpace);

        if (gcmIS_ERROR(status))
        {
            gcmASSERT(compiler->context.globalSpace == gcvNULL);
            break;
        }

        /* Create IR root */
        status = sloIR_SET_Construct(
                                    compiler,
                                    1,
                                    0,
                                    slvDECL_SET,
                                    &compiler->context.rootSet);

        if (gcmIS_ERROR(status))
        {
            gcmASSERT(compiler->context.rootSet == gcvNULL);
            break;
        }

#ifndef SL_SCAN_NO_PREPROCESSOR
        /* Create the preprocessor */
        status = sloPREPROCESSOR_Construct(
                                        compiler,
                                        &compiler->preprocessor);

        if (gcmIS_ERROR(status)) break;
#endif

        /* Create the code emitter */
        status = sloCODE_EMITTER_Construct(
                                        compiler,
                                        &compiler->codeEmitter);

        if (gcmIS_ERROR(status)) break;

        *Compiler = compiler;

        gcmFOOTER_ARG("*Compiler=0x%x", *Compiler);
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (compiler != gcvNULL) sloCOMPILER_Destroy(compiler);

    *Compiler = gcvNULL;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_Destroy(
    IN sloCOMPILER Compiler
    )
{
    slsDATA_TYPE *          dataType;
    slsDLINK_LIST *         poolStringBucket;
    slsPOOL_STRING_NODE *   poolStringNode;
    gctINT32                reference = 0;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    /* Decrement the reference counter */
    gcmVERIFY_OK(gcoOS_AtomDecrement(gcvNULL, CompilerLockRef, &reference));

    if (reference == 1)
    {
        /* Delete the global lock */
        gcmVERIFY_OK(gcoOS_DeleteMutex(gcvNULL, CompilerLock));

        /* Destroy the reference counter */
        gcmVERIFY_OK(gcoOS_AtomDestroy(gcvNULL, CompilerLockRef));

        CompilerLockRef = gcvNULL;
    }

    if (Compiler->codeEmitter != gcvNULL)
    {
        gcmVERIFY_OK(sloCODE_EMITTER_Destroy(Compiler, Compiler->codeEmitter));
    }

#ifndef SL_SCAN_NO_PREPROCESSOR
    if (Compiler->preprocessor != gcvNULL)
    {
        gcmVERIFY_OK(sloPREPROCESSOR_Destroy(Compiler, Compiler->preprocessor));
    }
#endif

    if (Compiler->binary != gcvNULL)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(Compiler->binary));
    }

    if (Compiler->log != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Compiler->log));
    }

    /* Destory the whole IR tree */
    if (Compiler->context.rootSet != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &Compiler->context.rootSet->base));
    }

    /* Destroy built-in name space */
    if (Compiler->context.builtinSpace != gcvNULL)
    {
        gcmVERIFY_OK(slsNAME_SPACE_Destory(Compiler, Compiler->context.builtinSpace));
    }

    /* Destroy data types */
    while (!slsDLINK_LIST_IsEmpty(&Compiler->context.dataTypes))
    {
        slsDLINK_LIST_DetachFirst(&Compiler->context.dataTypes, slsDATA_TYPE, &dataType);

        gcmVERIFY_OK(slsDATA_TYPE_Destory(Compiler, dataType));
    }

    /* Destory string pool */
    FOR_EACH_HASH_BUCKET(&Compiler->context.stringPool, poolStringBucket)
    {
        while (!slsDLINK_LIST_IsEmpty(poolStringBucket))
        {
            slsDLINK_LIST_DetachFirst(poolStringBucket, slsPOOL_STRING_NODE, &poolStringNode);

            gcmVERIFY_OK(sloCOMPILER_Free(Compiler, poolStringNode));
        }
    }

    /* Empty the memory pool only if errors occur. */
    if (Compiler->context.errorCount > 0)
    {
        gcmVERIFY_OK(sloCOMPILER_EmptyMemoryPool(Compiler));
    }

    /* Free compiler struct */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Compiler));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_GetShaderType(
    IN sloCOMPILER Compiler,
    OUT sleSHADER_TYPE * ShaderType
    )
{
    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ShaderType);

    *ShaderType = Compiler->shaderType;

    gcmFOOTER_ARG("*ShaderType=%d", *ShaderType);
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_GetBinary(
    IN sloCOMPILER Compiler,
    OUT gcSHADER * Binary
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Binary);

    *Binary = Compiler->binary;

    if (Compiler->binary == gcvNULL)
    {
		status = gcvSTATUS_INVALID_DATA;
        gcmFOOTER();
        return status;
    }
    else
    {
        gcmFOOTER_ARG("*Binary=0x%x", *Binary);
        return gcvSTATUS_OK;
    }
}

gceSTATUS
sloCOMPILER_GetHAL(
    IN sloCOMPILER Compiler,
    OUT gcoHAL * Hal
    )
{
    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Hal != gcvNULL);

    *Hal = Compiler->hal;

    gcmFOOTER_ARG("*Hal=0x%x", *Hal);
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_LoadingBuiltIns(
    IN sloCOMPILER Compiler,
    IN gctBOOL LoadingBuiltIns
    )
{
    gcmHEADER_ARG("Compiler=0x%x LoadingBuiltIns=%d",
                  Compiler, LoadingBuiltIns);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    Compiler->context.loadingBuiltIns = LoadingBuiltIns;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_MainDefined(
    IN sloCOMPILER Compiler
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (Compiler->context.mainDefined)
    {
        status = gcvSTATUS_INVALID_REQUEST;
        gcmFOOTER();
        return status;
    }

    Compiler->context.mainDefined = gcvTRUE;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#define LOG_BUF_RESERVED_SIZE   1024

gceSTATUS
sloCOMPILER_AddLog(
    IN sloCOMPILER Compiler,
    IN gctCONST_STRING Log
    )
{
    gceSTATUS       status;
    gctSIZE_T       length;
    gctUINT         requiredLogBufSize;
    gctSTRING       newLog;

    gcmHEADER_ARG("Compiler=0x%x Log=0x%x",
                  Compiler, Log);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Log);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, Log);

    gcmVERIFY_OK(gcoOS_StrLen(Log, &length));

    requiredLogBufSize = (gctUINT)length + 1;

    if (Compiler->logBufSize > 0)
    {
        gcmVERIFY_OK(gcoOS_StrLen(Compiler->log, &length));

        requiredLogBufSize += (gctUINT)length;
    }

    if (requiredLogBufSize > Compiler->logBufSize)
    {
        gctPOINTER pointer = gcvNULL;

        requiredLogBufSize += LOG_BUF_RESERVED_SIZE;

        status = gcoOS_Allocate(
                                gcvNULL,
                                (gctSIZE_T)requiredLogBufSize,
                                &pointer);
        if (gcmIS_ERROR(status))
        {
            gcmFOOTER();
            return status;
        }

        newLog = pointer;

        if (Compiler->logBufSize > 0)
        {
            gcmVERIFY_OK(gcoOS_StrCopySafe(newLog, requiredLogBufSize, Compiler->log));
            gcmVERIFY_OK(gcoOS_StrCatSafe(newLog, requiredLogBufSize, Log));

            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Compiler->log));
        }
        else
        {
            gcmASSERT(Compiler->log == gcvNULL);

            gcmVERIFY_OK(gcoOS_StrCopySafe(newLog, requiredLogBufSize, Log));
        }

        Compiler->log           = newLog;
        Compiler->logBufSize    = requiredLogBufSize;
    }
    else
    {
        gcmVERIFY_OK(gcoOS_StrCatSafe(Compiler->log, Compiler->logBufSize, Log));
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#define MAX_SINGLE_LOG_LENGTH       1024

gceSTATUS
sloCOMPILER_VOutputLog(
    IN sloCOMPILER Compiler,
    IN gctCONST_STRING Message,
    IN gctARGUMENTS Arguments
    )
{
    char *buffer;
    gctUINT offset = 0;
	gceSTATUS status;
    gctSIZE_T bytes;

    gcmHEADER_ARG("Compiler=0x%x Message=0x%x Arguments=0x%x",
                  Compiler, Message, Arguments);

    bytes = (MAX_SINGLE_LOG_LENGTH + 1) * gcmSIZEOF(char);

	gcmONERROR(gcoOS_Allocate(gcvNULL, bytes, (gctPOINTER*) &buffer));

    gcmVERIFY_OK(gcoOS_PrintStrVSafe(buffer, bytes,
                                     &offset,
                                     Message,
                                     Arguments));

    buffer[bytes - 1] = '\0';

    status = sloCOMPILER_AddLog(Compiler, buffer);

	gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, buffer));

OnError:
    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_OutputLog(
    IN sloCOMPILER Compiler,
    IN gctCONST_STRING Message,
    IN ...
    )
{
    gceSTATUS    status;
    gctARGUMENTS arguments;

    gcmHEADER_ARG("Compiler=0x%x Message=0x%x ...",
                  Compiler, Message);

    gcmARGUMENTS_START(arguments, Message);
    status = sloCOMPILER_VOutputLog(Compiler,
                                    Message,
                                    arguments);
    gcmARGUMENTS_END(arguments);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_GenCode(
    IN sloCOMPILER Compiler
    )
{
    gceSTATUS               status;
    sloCODE_GENERATOR       codeGenerator = gcvNULL;
    sloOBJECT_COUNTER       objectCounter = gcvNULL;
    slsGEN_CODE_PARAMETERS  parameters;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (Compiler->context.rootSet == gcvNULL)
    {
		status = gcvSTATUS_INVALID_DATA;
        gcmFOOTER();
        return status;
    }

    gcmONERROR(sloCODE_GENERATOR_Construct(Compiler, &codeGenerator));

    /* Count the objects */

    gcmONERROR(sloOBJECT_COUNTER_Construct(Compiler, &objectCounter));

    gcmONERROR(sloIR_OBJECT_Accept(Compiler,
                                   &Compiler->context.rootSet->base,
                                   &objectCounter->visitor,
                                   &parameters));

    codeGenerator->attributeCount= objectCounter->attributeCount;
    codeGenerator->uniformCount  = objectCounter->uniformCount;
    codeGenerator->variableCount = objectCounter->variableCount;
    codeGenerator->outputCount   = objectCounter->outputCount;
    codeGenerator->functionCount = objectCounter->functionCount;

    gcmVERIFY_OK(sloOBJECT_COUNTER_Destroy(Compiler, objectCounter));
    objectCounter = gcvNULL;

    gcmVERIFY_OK(sloCOMPILER_Dump(Compiler,
                                  slvDUMP_CODE_GENERATOR,
                                  "<PROGRAM>\n"
                  "<OBJECT COUNT: attributes = \"%d\" "
                  "uniforms = \"%d\" variables = \"%d\" "
                  "outputs = \"%d\" functions = \"%d\" />",
                  codeGenerator->attributeCount,
                  codeGenerator->uniformCount,
                  codeGenerator->variableCount,
                  codeGenerator->outputCount,
                  codeGenerator->functionCount));

    gcmONERROR(sloIR_AllocObjectPointerArrays(Compiler,
                                              codeGenerator));

    slsGEN_CODE_PARAMETERS_Initialize(&parameters, gcvFALSE, gcvFALSE);

    status = sloIR_OBJECT_Accept(
                                Compiler,
                                &Compiler->context.rootSet->base,
                                &codeGenerator->visitor,
                                &parameters);

    status = sloCOMPILER_BackPatch(Compiler);


    slsGEN_CODE_PARAMETERS_Finalize(&parameters);

    gcmVERIFY_OK(sloCODE_GENERATOR_Destroy(Compiler, codeGenerator));
    codeGenerator = gcvNULL;

    gcmONERROR(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_CODE_GENERATOR,
                                "</PROGRAM>"));

    /* Check if 'main' function defined */
    if (!Compiler->context.mainDefined)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        0,
                                        0,
                                        slvREPORT_ERROR,
                                        "'main' function undefined"));

		status = gcvSTATUS_INVALID_DATA;
        gcmFOOTER();
        return status;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    if (objectCounter != gcvNULL)
    {
        gcmVERIFY_OK(sloOBJECT_COUNTER_Destroy(Compiler, objectCounter));
        objectCounter = gcvNULL;
    }
    if (codeGenerator != gcvNULL)
    {
        gcmVERIFY_OK(sloCODE_GENERATOR_Destroy(Compiler, codeGenerator));
        codeGenerator = gcvNULL;
    }
    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_Compile(
    IN sloCOMPILER Compiler,
    IN sltOPTIMIZATION_OPTIONS OptimizationOptions,
    IN sltDUMP_OPTIONS DumpOptions,
    IN gctUINT StringCount,
    IN gctCONST_STRING Strings[],
    OUT gcSHADER * Binary,
    OUT gctSTRING * Log
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("Compiler=0x%x OptimizationOptions=%u DumpOptions=%u "
                  "StringCount=%u Strings=0x%x Binary=0x%x Log=0x%x",
                  Compiler, OptimizationOptions, DumpOptions,
                  StringCount, Strings, Binary, Log);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    gcmASSERT(Binary != gcvNULL);

    *Binary = gcvNULL;

    Compiler->context.optimizationOptions   = OptimizationOptions;
    Compiler->context.extensions            = slvEXTENSION_NONE;
    Compiler->context.dumpOptions           = DumpOptions;
    Compiler->context.scannerState          = slvSCANNER_NOMRAL;

    do
    {
        /* Load the built-ins */
        status = sloCOMPILER_LoadBuiltIns(Compiler);

        if (gcmIS_ERROR(status)) break;


        /* Set the global scope as current */
        Compiler->context.currentSpace = Compiler->context.globalSpace;

        /* Parse the source string */
        status = sloCOMPILER_Parse(
                                    Compiler,
                                    StringCount,
                                    Strings);

        if (gcmIS_ERROR(status)) break;

        gcmVERIFY_OK(sloCOMPILER_DumpIR(Compiler));

        if (Compiler->context.errorCount > 0)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Construct the binary */
        status = gcSHADER_Construct(
                                    Compiler->hal,
                                    Compiler->shaderType,
                                    &Compiler->binary);
        if (gcmIS_ERROR(status)) break;

        status = gcSHADER_SetCompilerVersion(Compiler->binary,
					     sloCOMPILER_GetVersion(Compiler->shaderType));
        if (gcmIS_ERROR(status)) break;

        /* Generate the code */
        status = sloCOMPILER_GenCode(Compiler);

        if (gcmIS_ERROR(status)) break;

        if (Compiler->context.errorCount > 0)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        /* Pack the binary */
        status = gcSHADER_Pack(Compiler->binary);

        if (gcmIS_ERROR(status)) break;

        /* Return */
        *Binary             = Compiler->binary;
        Compiler->binary    = gcvNULL;

        if (Log != gcvNULL)
        {
            *Log                = Compiler->log;
            Compiler->log       = gcvNULL;
        }

        gcmFOOTER_ARG("*Binary=%lu *Log=0x%x",
                      *Binary, gcmOPT_POINTER(Log));
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (Log != gcvNULL)
    {
        *Log                = Compiler->log;
        Compiler->log       = gcvNULL;
    }

    gcmFOOTER();
    return status;
}

#define BUFFER_SIZE     1024

gceSTATUS
sloCOMPILER_Preprocess(
    IN sloCOMPILER Compiler,
    IN sltOPTIMIZATION_OPTIONS OptimizationOptions,
    IN sltDUMP_OPTIONS DumpOptions,
    IN gctUINT StringCount,
    IN gctCONST_STRING Strings[],
    OUT gctSTRING * Log
    )
{
    gceSTATUS   status;
    gctBOOL     locked = gcvFALSE;
#ifndef SL_SCAN_NO_PREPROCESSOR
    gctCHAR     buffer[BUFFER_SIZE];
    gctINT      actualSize;
#endif

    gcmHEADER_ARG("Compiler=0x%x OptimizationOptions=%u DumpOptions=%u "
                  "StringCount=%u Strings=0x%x Log=0x%x",
                  Compiler, OptimizationOptions, DumpOptions,
                  StringCount, Strings, Log);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    Compiler->context.optimizationOptions   = OptimizationOptions;
    Compiler->context.dumpOptions           = DumpOptions;

    do
    {
        status = sloCOMPILER_Lock(Compiler);
        locked = gcvTRUE;

        if (gcmIS_ERROR(status)) break;

        status = sloCOMPILER_MakeCurrent(Compiler, StringCount, Strings);

        if (gcmIS_ERROR(status)) break;

 #ifndef SL_SCAN_NO_PREPROCESSOR
        /* Preprocess the source string */
        while (gcvTRUE)
        {
            status = sloPREPROCESSOR_Parse(
                                        sloCOMPILER_GetPreprocessor(Compiler),
                                        BUFFER_SIZE,
                                        buffer,
                                        &actualSize);

            if (gcmIS_ERROR(status)) break;

            if (actualSize == 0) break;

            gcmVERIFY_OK(sloCOMPILER_OutputLog(
                                            Compiler,
                                            "<PP_TOKEN line=\"%d\" string=\"%d\" text=\"%s\" />",
                                            sloCOMPILER_GetCurrentLineNo(Compiler),
                                            sloCOMPILER_GetCurrentStringNo(Compiler),
                                            buffer));
        }
#endif

        locked = gcvFALSE;
        gcmVERIFY_OK(sloCOMPILER_Unlock(Compiler));

        /* Return */
        if (Log != gcvNULL)
        {
            *Log                = Compiler->log;
            Compiler->log       = gcvNULL;
        }

        gcmFOOTER_ARG("*Log=0x%x", gcmOPT_POINTER(Log));
        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (locked)
    {
        gcmVERIFY_OK(sloCOMPILER_Unlock(Compiler));
    }

    if (Log != gcvNULL)
    {
        *Log                = Compiler->log;
        Compiler->log       = gcvNULL;
    }


    gcmFOOTER();
    return status;
}

/* Dump IR data */
gceSTATUS
sloCOMPILER_DumpIR(
    IN sloCOMPILER Compiler
    )
{
    slsDATA_TYPE *          dataType;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (!(Compiler->context.dumpOptions & slvDUMP_IR))
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "<IR>"));

    /* Dump all data types */
    FOR_EACH_DLINK_NODE(&Compiler->context.dataTypes, slsDATA_TYPE, dataType)
    {
        gcmVERIFY_OK(slsDATA_TYPE_Dump(Compiler, dataType));
    }

    /* Dump all name spaces and names */
    if (Compiler->context.globalSpace != gcvNULL)
    {
        gcmVERIFY_OK(slsNAME_SPACE_Dump(Compiler, Compiler->context.globalSpace));
    }

    /* Dump syntax tree */
    if (Compiler->context.rootSet != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_OBJECT_Dump(Compiler, &Compiler->context.rootSet->base));
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_IR,
                                "</IR>"));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_Lock(
    IN sloCOMPILER Compiler
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    if (CompilerLock == gcvNULL)
    {
		status = gcvSTATUS_INVALID_OBJECT;
        gcmFOOTER();
        return status;
    }

    status = gcoOS_AcquireMutex(gcvNULL, CompilerLock, gcvINFINITE);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_Unlock(
    IN sloCOMPILER Compiler
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    if (CompilerLock == gcvNULL)
    {
		status = gcvSTATUS_INVALID_OBJECT;
        gcmFOOTER();
        return status;
    }

    status = gcoOS_ReleaseMutex(gcvNULL, CompilerLock);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_SetScannerState(
    IN sloCOMPILER Compiler,
    IN sleSCANNER_STATE State
    )
{
    gcmHEADER_ARG("Compiler=0x%x State=%d", Compiler, State);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    Compiler->context.scannerState = State;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

sleSCANNER_STATE
sloCOMPILER_GetScannerState(
    IN sloCOMPILER Compiler
    )
{
    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    gcmFOOTER_ARG("<return>=%d", Compiler->context.scannerState);
    return Compiler->context.scannerState;
}

sloPREPROCESSOR
sloCOMPILER_GetPreprocessor(
    IN sloCOMPILER Compiler
    )
{
    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    gcmFOOTER_ARG("<return>=0x%x", Compiler->preprocessor);
    return Compiler->preprocessor;
}

sloCODE_EMITTER
sloCOMPILER_GetCodeEmitter(
    IN sloCOMPILER Compiler
    )
{
    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    gcmFOOTER_ARG("<return>=0x%x", Compiler->codeEmitter);
    return Compiler->codeEmitter;
}

gctBOOL
sloCOMPILER_OptimizationEnabled(
    IN sloCOMPILER Compiler,
    IN sleOPTIMIZATION_OPTION OptimizationOption
    )
{
    gcmHEADER_ARG("Compiler=0x%x OptimizationOption=%d",
                  Compiler, OptimizationOption);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    gcmFOOTER_ARG("<return>=%d", Compiler->context.optimizationOptions & OptimizationOption);
    return (Compiler->context.optimizationOptions & OptimizationOption);
}

gctBOOL
sloCOMPILER_ExtensionEnabled(
    IN sloCOMPILER Compiler,
    IN sleEXTENSION Extension
    )
{
    gcmHEADER_ARG("Compiler=0x%x Extension=%d",
                  Compiler, Extension);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    gcmFOOTER_ARG("<return>=%d", Compiler->context.extensions & Extension);
    return (Compiler->context.extensions & Extension);
}

/* Enable extension */
gceSTATUS
sloCOMPILER_EnableExtension(
    IN sloCOMPILER Compiler,
    IN sleEXTENSION Extension,
    IN gctBOOL Enable
    )
{
    gcmHEADER_ARG("Compiler=0x%x Extension=%d Enable=%d",
                  Compiler, Extension, Enable);

    /* Verify the arguments. */
    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    if (Enable)
    {
        Compiler->context.extensions |= Extension;
    }
    else
    {
        Compiler->context.extensions &= ~Extension;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

#define MAX_ERROR   100

gceSTATUS
sloCOMPILER_VReport(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleREPORT_TYPE Type,
    IN gctCONST_STRING Message,
    IN gctARGUMENTS Arguments
    )
{
    gcmHEADER_ARG("Compiler=0x%x LineNo=%d StringNo=%d "
                  "Type=%d Message=0x%x Arguments=0x%x",
                  Compiler, LineNo, StringNo,
                  Type, Message, Arguments);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    switch (Type)
    {
    case slvREPORT_FATAL_ERROR:
    case slvREPORT_INTERNAL_ERROR:
    case slvREPORT_ERROR:
        if (Compiler->context.errorCount >= MAX_ERROR)
        {
            gcmFOOTER_NO();
            return gcvSTATUS_OK;
        }
        break;
    default: break;
    }

    if (LineNo > 0)
    {
        sloCOMPILER_OutputLog(Compiler, "(%d:%d) : ", LineNo, StringNo);
    }

    switch (Type)
    {
    case slvREPORT_FATAL_ERROR:
        Compiler->context.errorCount = MAX_ERROR;
        sloCOMPILER_OutputLog(Compiler, "fatal error : ");
        break;

    case slvREPORT_INTERNAL_ERROR:
        Compiler->context.errorCount++;
        sloCOMPILER_OutputLog(Compiler, "internal error : ");
        break;

    case slvREPORT_ERROR:
        Compiler->context.errorCount++;
        sloCOMPILER_OutputLog(Compiler, "error : ");
        break;

    case slvREPORT_WARN:
        Compiler->context.warnCount++;
        sloCOMPILER_OutputLog(Compiler, "warning : ");
        break;

    default:
        gcmASSERT(0);
    }

    sloCOMPILER_VOutputLog(Compiler, Message, Arguments);

    sloCOMPILER_OutputLog(Compiler, "\n");

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_Report(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleREPORT_TYPE Type,
    IN gctCONST_STRING Message,
    IN ...
    )
{
    gceSTATUS    status;
    gctARGUMENTS arguments;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%d StringNo=%d "
                  "Type=%d Message=0x%x ...",
                  Compiler, LineNo, StringNo,
                  Type, Message);

    gcmARGUMENTS_START(arguments, Message);
    status = sloCOMPILER_VReport(Compiler,
                                 LineNo,
                                 StringNo,
                                 Type,
                                 Message,
                                 arguments);
    gcmARGUMENTS_END(arguments);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_Dump(
    IN sloCOMPILER Compiler,
    IN sleDUMP_OPTION DumpOption,
    IN gctCONST_STRING Message,
    IN ...
    )
{
    gceSTATUS    status;
    gctARGUMENTS arguments;

    gcmHEADER_ARG("Compiler=0x%x DumpOption=%d Message=0x%x ...",
                  Compiler, DumpOption, Message);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (!(Compiler->context.dumpOptions & DumpOption))
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    gcmARGUMENTS_START(arguments, Message);
    status = sloCOMPILER_VOutputLog(Compiler, Message, arguments);
    gcmARGUMENTS_END(arguments);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_Allocate(
    IN sloCOMPILER Compiler,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    )
{
    gceSTATUS       status;
    gctSIZE_T       bytes;
    slsDLINK_NODE * node;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x Bytes=%lu", Compiler, Bytes);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    bytes = Bytes + sizeof(slsDLINK_NODE);

    status = gcoOS_Allocate(
                        gcvNULL,
                        bytes,
                        &pointer);

    if (gcmIS_ERROR(status))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        0,
                                        0,
                                        slvREPORT_FATAL_ERROR,
                                        "not enough memory"));

        gcmFOOTER();
        return status;
    }

    node = pointer;

    /* Add node into the memory pool */
    slsDLINK_LIST_InsertLast(&Compiler->memoryPool, node);

    *Memory = (gctPOINTER)(node + 1);

    gcmFOOTER_ARG("status=%d *Memory=0x%x", status, *Memory);
    return status;
}

gceSTATUS
sloCOMPILER_Free(
    IN sloCOMPILER Compiler,
    IN gctPOINTER Memory
    )
{
    slsDLINK_NODE * node;
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x Memory=0x%x", Compiler, Memory);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    node = (slsDLINK_NODE *)Memory - 1;

    /* Detach node from the memory pool */
    slsDLINK_NODE_Detach(node);

    status = gcmOS_SAFE_FREE(gcvNULL, node);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_EmptyMemoryPool(
    IN sloCOMPILER Compiler
    )
{
    slsDLINK_NODE * node;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    /* Free all unfreed memory block in the pool */
    while (!slsDLINK_LIST_IsEmpty(&Compiler->memoryPool))
    {
        slsDLINK_LIST_DetachFirst(&Compiler->memoryPool, slsDLINK_NODE, &node);

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, node));
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_AllocatePoolString(
    IN sloCOMPILER Compiler,
    IN gctCONST_STRING String,
    OUT sltPOOL_STRING * PoolString
    )
{
    gceSTATUS               status;
    slsDLINK_NODE *         bucket;
    gctSIZE_T               length;
    slsPOOL_STRING_NODE *   node;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x String=0x%x",
                  Compiler, String);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    bucket = slsHASH_TABLE_Bucket(
                                &Compiler->context.stringPool,
                                slmBUCKET_INDEX(slHashString(String)));

    FOR_EACH_DLINK_NODE(bucket, slsPOOL_STRING_NODE, node)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(node->string, String)))
        {
            *PoolString = node->string;

            gcmFOOTER_ARG("*PoolString=0x%x", *PoolString);
            return gcvSTATUS_OK;
        }
    }

    gcmVERIFY_OK(gcoOS_StrLen(String, &length));

    status = sloCOMPILER_Allocate(
                                Compiler,
                                (gctSIZE_T)sizeof(slsPOOL_STRING_NODE) + length + 1,
                                &pointer);
    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    node = pointer;

    node->string = (sltPOOL_STRING)((gctINT8 *)node + sizeof(slsPOOL_STRING_NODE));

    gcmVERIFY_OK(gcoOS_StrCopySafe(node->string, length + 1, String));

    slsDLINK_LIST_InsertFirst(bucket, &node->node);

    *PoolString = node->string;

    gcmFOOTER_ARG("*PoolString=0x%x", *PoolString);
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_GetChar(
    IN sloCOMPILER Compiler,
    OUT gctINT_PTR Char
    )
{
    gcmHEADER_ARG("Compiler=0x%x Char=0x%x",
                  Compiler, Char);

    gcmASSERT(Compiler);
    gcmASSERT(Compiler->context.strings);

    if (Compiler->context.strings[Compiler->context.currentStringNo][Compiler->context.currentCharNo] != '\0')
    {
        *Char = Compiler->context.strings[Compiler->context.currentStringNo][Compiler->context.currentCharNo++];
    }
    else if (Compiler->context.currentStringNo == Compiler->context.stringCount)
    {
        *Char = T_EOF;
    }
    else
    {
        gcmASSERT(Compiler->context.currentStringNo < Compiler->context.stringCount);

        Compiler->context.currentStringNo++;
        Compiler->context.currentCharNo = 0;

        while (Compiler->context.currentStringNo < Compiler->context.stringCount)
        {
            if (Compiler->context.strings[Compiler->context.currentStringNo][0] != '\0')
            {
                break;
            }

            Compiler->context.currentStringNo++;
        }

        if (Compiler->context.currentStringNo == Compiler->context.stringCount)
        {
            *Char = T_EOF;
        }
        else
        {
            gcmASSERT(Compiler->context.currentStringNo < Compiler->context.stringCount);
            *Char = Compiler->context.strings[Compiler->context.currentStringNo][Compiler->context.currentCharNo++];
        }
    }

    gcmVERIFY_OK(sloCOMPILER_SetCurrentStringNo(
                                                Compiler,
                                                Compiler->context.currentStringNo));

    gcmVERIFY_OK(sloCOMPILER_SetCurrentLineNo(
                                            Compiler,
                                            Compiler->context.currentLineNo));

    if (*Char == '\n')
    {
        Compiler->context.currentLineNo++;
    }

    gcmFOOTER_ARG("*Char=0x%x", *Char);
    return gcvSTATUS_OK;
}

static sloCOMPILER CurrentCompiler  = gcvNULL;

gceSTATUS
sloCOMPILER_MakeCurrent(
    IN sloCOMPILER Compiler,
    IN gctUINT StringCount,
    IN gctCONST_STRING Strings[]
    )
{
#ifndef SL_SCAN_NO_PREPROCESSOR
    gceSTATUS   status;
#endif

    gcmHEADER_ARG("Compiler=0x%x StringCount=%d Strings=0x%x",
                  Compiler, StringCount, Strings);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    Compiler->context.stringCount       = StringCount;
    Compiler->context.strings           = Strings;
    Compiler->context.currentLineNo     = 1;
    Compiler->context.currentStringNo   = 0;
    Compiler->context.currentCharNo     = 0;

    CurrentCompiler                     = Compiler;

#ifndef SL_SCAN_NO_PREPROCESSOR
    status = sloPREPROCESSOR_SetSourceStrings(
                                            Compiler->preprocessor,
                                            StringCount,
                                            Strings);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }
#endif

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gctINT
slInput(
    IN gctINT MaxSize,
    OUT gctSTRING Buffer
    )
{
#ifdef SL_SCAN_NO_PREPROCESSOR

    gctINT ch;

    gcmHEADER_ARG("MaxSize=0x%x Buffer=0x%x",
                  MaxSize, Buffer);

    gcmASSERT(CurrentCompiler);

    gcmVERIFY_OK(sloCOMPILER_GetChar(CurrentCompiler, &ch));

    if (ch == T_EOF)
    {
        gcmFOOTER_ARG("<return>=%d", 0);
        return 0;
    }

    Buffer[0] = (gctCHAR)ch;

    gcmFOOTER_ARG("<return>=%d", 1);
    return 1;

#else

    gceSTATUS   status;
    gctINT      actualSize;

    gcmHEADER_ARG("MaxSize=0x%x Buffer=0x%x",
                  MaxSize, Buffer);

    status = sloPREPROCESSOR_Parse(
                                sloCOMPILER_GetPreprocessor(CurrentCompiler),
                                MaxSize,
                                Buffer,
                                &actualSize);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%d", 0);
        return 0;
    }

    gcmFOOTER_ARG("<return>=%d", actualSize);
    return actualSize;

#endif
}

gctPOINTER
slMalloc(
    IN gctSIZE_T Bytes
    )
{
    gceSTATUS status;
    gctSIZE_T_PTR memory;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Bytes=%u", Bytes);

    status = sloCOMPILER_Allocate(CurrentCompiler,
                                  Bytes + gcmSIZEOF(gctSIZE_T_PTR),
                                  &pointer);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return gcvNULL;
    }

    memory    = pointer;
    memory[0] = Bytes;

    gcmFOOTER_ARG("<return>=%d", &memory[1]);
    return &memory[1];
}

gctPOINTER
slRealloc(
    IN gctPOINTER Memory,
    IN gctSIZE_T NewBytes
    )
{
    gceSTATUS status;
    gctSIZE_T_PTR memory = gcvNULL;
    gctPOINTER pointer = gcvNULL;
    gcmHEADER_ARG("Memory=0x%x NewBytes=%u",
                  Memory, NewBytes);

    do
    {
        gcmERR_BREAK(
            sloCOMPILER_Allocate(CurrentCompiler,
                                 NewBytes + gcmSIZEOF(gctSIZE_T_PTR),
                                 &pointer));

        memory    = pointer;
        memory[0] = NewBytes;

        gcmERR_BREAK(
            gcoOS_MemCopy(memory + 1,
                          Memory,
                          ((gctSIZE_T_PTR) Memory)[-1]));

        gcmERR_BREAK(
            sloCOMPILER_Free(CurrentCompiler,
                             &((gctSIZE_T_PTR) Memory)[-1]));

        gcmFOOTER_ARG("<return>=%d", &memory[1]);
        return &memory[1];
    }
    while (gcvFALSE);

    if (memory != gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Free(CurrentCompiler, memory));
    }

    gcmFOOTER_ARG("<return>=%d", gcvNULL);
    return gcvNULL;
}

void
slFree(
    IN gctPOINTER Memory
    )
{
    gcmHEADER_ARG("Memory=0x%x", Memory);

    if (Memory != gcvNULL)
    {
        gcmVERIFY_OK(
            sloCOMPILER_Free(CurrentCompiler,
                             &((gctSIZE_T_PTR) Memory)[-1]));
    }

    gcmFOOTER_NO();
}

void
slReport(
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleREPORT_TYPE Type,
    IN gctSTRING Message,
    IN ...
    )
{
    gctARGUMENTS arguments;

    gcmHEADER_ARG("LineNo=%d StringNo=%d Type=%d Message=0x%x ...",
                  LineNo, StringNo, Type, Message);

    /* TODO: ... */
    gcmASSERT(CurrentCompiler);

    gcmARGUMENTS_START(arguments, Message);
    gcmVERIFY_OK(sloCOMPILER_VReport(CurrentCompiler,
                                     LineNo,
                                     StringNo,
                                     Type,
                                     Message,
                                     arguments));
    gcmARGUMENTS_END(arguments);
    gcmFOOTER_NO();
}

gceSTATUS
sloCOMPILER_CreateDataType(
    IN sloCOMPILER Compiler,
    IN gctINT TokenType,
    IN slsNAME_SPACE * FieldSpace,
    OUT slsDATA_TYPE ** DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x TokenType=%d FieldSpace=0x%x",
               Compiler, TokenType, FieldSpace);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    status = slsDATA_TYPE_Construct(
                                    Compiler,
                                    TokenType,
                                    FieldSpace,
                                    &dataType);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    slsDLINK_LIST_InsertLast(&Compiler->context.dataTypes, &dataType->node);

    *DataType = dataType;

    gcmFOOTER_ARG("*DataType=0x%x", *DataType);
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_CreateArrayDataType(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * ElementDataType,
    IN gctUINT ArrayLength,
    OUT slsDATA_TYPE ** DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x ElementDataType=0x%x ArrayLength=%u",
                  Compiler, ElementDataType, ArrayLength);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    status = slsDATA_TYPE_ConstructArray(
                                    Compiler,
                                    ElementDataType,
                                    ArrayLength,
                                    &dataType);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    slsDLINK_LIST_InsertLast(&Compiler->context.dataTypes, &dataType->node);

    *DataType = dataType;


    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_CreateElementDataType(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * CompoundDataType,
    OUT slsDATA_TYPE ** DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x CompoundDataType=0x%x",
                  Compiler, CompoundDataType);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    status = slsDATA_TYPE_ConstructElement(
                                    Compiler,
                                    CompoundDataType,
                                    &dataType);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    slsDLINK_LIST_InsertLast(&Compiler->context.dataTypes, &dataType->node);

    *DataType = dataType;
    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_CloneDataType(
    IN sloCOMPILER Compiler,
    IN sltQUALIFIER Qualifier,
    IN slsDATA_TYPE * Source,
    OUT slsDATA_TYPE ** DataType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x Qualifier=%d Source=0x%x",
                  Compiler, Qualifier, Source);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    status = slsDATA_TYPE_Clone(
                                Compiler,
                                Qualifier,
                                Source,
                                &dataType);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    slsDLINK_LIST_InsertLast(&Compiler->context.dataTypes, &dataType->node);

    *DataType = dataType;

    gcmFOOTER_ARG("*DataType=0x%x", *DataType);
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_CreateName(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleNAME_TYPE Type,
    IN slsDATA_TYPE * DataType,
    IN sltPOOL_STRING Symbol,
    IN sleEXTENSION Extension,
    OUT slsNAME ** Name
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%d StringNo=%d "
                  "Type=%d DataType=0x%x Symbol=0x%x "
                  "Extension=%d Name=0x%x",
                  Compiler, LineNo, StringNo,
                  Type, DataType, Symbol,
                  Extension, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (!Compiler->context.loadingBuiltIns
        && gcmIS_SUCCESS(gcoOS_StrNCmp(Symbol, "gl_", 3)))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        LineNo,
                                        StringNo,
                                        slvREPORT_ERROR,
                                        "The identifier: '%s' starting with 'gl_' is reserved",
                                        Symbol));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    status = slsNAME_SPACE_CreateName(
                                    Compiler,
                                    Compiler->context.currentSpace,
                                    LineNo,
                                    StringNo,
                                    Type,
                                    DataType,
                                    Symbol,
                                    Compiler->context.loadingBuiltIns,
                                    Extension,
                                    Name);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_CreateAuxiliaryName(
	IN sloCOMPILER Compiler,
    IN slsNAME* refName,
    IN gctUINT LineNo,
	IN gctUINT StringNo,
    IN slsDATA_TYPE * DataType,
	OUT slsNAME ** Name
	)
{
    gceSTATUS                   status = gcvSTATUS_OK;
    sltPOOL_STRING              auxiArraySymbol, tempSymbol;
    gctPOINTER                  pointer = gcvNULL;
    slsNAME*                    name = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x refName=0x%x LineNo=%u StringNo=%u "
                  "DataType=0x%x Name=0x%x",
                  Compiler, refName, LineNo, StringNo,
                  DataType, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (refName)
    {
        gctSIZE_T   symbolSize;

        gcoOS_StrLen(refName->symbol, &symbolSize);
        status = gcoOS_Allocate(
                                gcvNULL,
                                (gctSIZE_T)symbolSize+16,
                                &pointer);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        tempSymbol = pointer;

        gcoOS_StrCopySafe(tempSymbol, symbolSize+1, refName->symbol);
        gcoOS_StrCatSafe(tempSymbol, symbolSize+16, "_scalarArray");

        status = sloCOMPILER_AllocatePoolString(
                                            Compiler,
                                            tempSymbol,
                                            &auxiArraySymbol);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = slsNAME_SPACE_Search(
                                    Compiler,
                                    refName->mySpace,
                                    auxiArraySymbol,
                                    gcvFALSE,
                                    &name);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (name == gcvNULL)
        {
            status = slsNAME_SPACE_CreateName(Compiler,
                                             refName->mySpace,
                                             refName->lineNo,
                                             refName->stringNo,
                                             refName->type,
                                             DataType,
                                             auxiArraySymbol,
                                             gcvFALSE,
                                             refName->extension,
                                             &name);
            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }

        gcoOS_Free(gcvNULL, pointer);
        pointer= gcvNULL;
    }
    else
    {
        gctUINT      offset = 0;
        gctUINT64   curTime;

        status = gcoOS_Allocate(
                                gcvNULL,
                                (gctSIZE_T)256,
                                &pointer);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        tempSymbol = pointer;

        gcoOS_GetTime(&curTime);
        gcoOS_PrintStrSafe(tempSymbol, 256, &offset, "%u_scalarArray", curTime);

        status = sloCOMPILER_AllocatePoolString(
                                            Compiler,
                                            tempSymbol,
                                            &auxiArraySymbol);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        status = slsNAME_SPACE_Search(
                                    Compiler,
                                    Compiler->context.currentSpace,
                                    auxiArraySymbol,
                                    gcvFALSE,
                                    &name);
        if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }

        if (name == gcvNULL)
        {
            status = slsNAME_SPACE_CreateName(Compiler,
                                             Compiler->context.currentSpace,
                                             LineNo,
                                             StringNo,
                                             slvVARIABLE_NAME,
                                             DataType,
                                             auxiArraySymbol,
                                             gcvFALSE,
                                             slvEXTENSION_NONE,
                                             &name);
            if (gcmIS_ERROR(status)) { gcmFOOTER(); return status; }
        }

        gcoOS_Free(gcvNULL, pointer);
        pointer= gcvNULL;
    }

    if (Name != gcvNULL) *Name = name;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_SearchName(
    IN sloCOMPILER Compiler,
    IN sltPOOL_STRING Symbol,
    IN gctBOOL Recursive,
    OUT slsNAME ** Name
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x Symbol=0x%x "
                  "Recursive=%d Name=0x%x",
                  Compiler, Symbol, Recursive, Name);

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "Symbol=%s", gcmOPT_STRING(Symbol));
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "Name=%x", gcmOPT_POINTER(Name));

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    status = slsNAME_SPACE_Search(
                                Compiler,
                                Compiler->context.currentSpace,
                                Symbol,
                                Recursive,
                                Name);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_CheckNewFuncName(
    IN sloCOMPILER Compiler,
    IN slsNAME * FuncName,
    OUT slsNAME ** FirstFuncName
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x FuncName=0x%x FirstFuncName=0x%x",
                  Compiler, FuncName, FirstFuncName);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(FuncName);

    status = slsNAME_SPACE_CheckNewFuncName(
                                        Compiler,
                                        Compiler->context.globalSpace,
                                        FuncName,
                                        FirstFuncName);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_CreateNameSpace(
    IN sloCOMPILER Compiler,
    OUT slsNAME_SPACE ** NameSpace
    )
{
    gceSTATUS       status;
    slsNAME_SPACE * nameSpace;

    gcmHEADER_ARG("Compiler=0x%x NameSpace=0x%x",
                  Compiler, NameSpace);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(NameSpace);

    status = slsNAME_SPACE_Construct(
                                    Compiler,
                                    Compiler->context.currentSpace,
                                    &nameSpace);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    Compiler->context.currentSpace = nameSpace;

    *NameSpace = nameSpace;

    gcmFOOTER_ARG("*NameSpace=0x%x", *NameSpace);
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_PopCurrentNameSpace(
    IN sloCOMPILER Compiler,
    OUT slsNAME_SPACE ** PrevNameSpace
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x PrevNameSpace=0x%x",
                  Compiler, PrevNameSpace);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (Compiler->context.currentSpace == gcvNULL ||
        Compiler->context.currentSpace->parent == gcvNULL)
    {
		status = gcvSTATUS_INTERFACE_ERROR;
        gcmFOOTER();
        return status;
    }

    if (PrevNameSpace != gcvNULL)
    {
        *PrevNameSpace = Compiler->context.currentSpace;
    }

    Compiler->context.currentSpace = Compiler->context.currentSpace->parent;

    gcmFOOTER_ARG("*PrevNameSpace=0x%x", gcmOPT_POINTER(PrevNameSpace));
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_AtGlobalNameSpace(
    IN sloCOMPILER Compiler,
    OUT gctBOOL * AtGlobalNameSpace
    )
{
    gcmHEADER_ARG("Compiler=0x%x AtGlobalNameSpace=0x%x",
                  Compiler, AtGlobalNameSpace);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(AtGlobalNameSpace);
    gcmASSERT(Compiler->context.currentSpace);
    gcmASSERT(Compiler->context.globalSpace);

    *AtGlobalNameSpace = (Compiler->context.globalSpace == Compiler->context.currentSpace);

    gcmFOOTER_ARG("*AtGlobalNameSpace=%d", *AtGlobalNameSpace);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_SET_IsRoot(
    IN sloCOMPILER Compiler,
    IN sloIR_SET Set,
    OUT gctBOOL * IsRoot
    )
{
    gcmHEADER_ARG("Compiler=0x%x Set=0x%x",
                  Compiler, Set);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Set, slvIR_SET);
    gcmASSERT(IsRoot);

    *IsRoot = (Set == Compiler->context.rootSet);

    gcmFOOTER_ARG("*IsRoot=%d", *IsRoot);
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_AddExternalDecl(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE ExternalDecl
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("Compiler=0x%x ExternalDecl=0x%x",
                  Compiler, ExternalDecl);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    status = sloIR_SET_AddMember(
                            Compiler,
                            Compiler->context.rootSet,
                            ExternalDecl);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_BASE_UseAsTextureCoord(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE Base
    )
{
    gceSTATUS               status;
    sloIR_SET               set             = gcvNULL;
    sloIR_UNARY_EXPR        unaryExpr       = gcvNULL;
    sloIR_BINARY_EXPR       binaryExpr      = gcvNULL;
    sloIR_SELECTION         selection       = gcvNULL;
    sloIR_POLYNARY_EXPR     polynaryExpr    = gcvNULL;
    sloIR_BASE              member;

    gcmHEADER_ARG("Compiler=0x%x Base=0x%x",
                  Compiler, Base);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(Base);

    switch (sloIR_OBJECT_GetType(Base))
    {
    case slvIR_SET:
        set = (sloIR_SET)Base;

        FOR_EACH_DLINK_NODE(&set->members, struct _sloIR_BASE, member)
        {
            /* Setup all members */
            status = sloIR_BASE_UseAsTextureCoord(
                                                Compiler,
                                                member);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_VARIABLE:
        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_UNARY_EXPR:
        unaryExpr = (sloIR_UNARY_EXPR)Base;
        gcmASSERT(unaryExpr->operand);

        switch (unaryExpr->type)
        {
        case slvUNARY_COMPONENT_SELECTION:
            /* Setup the operand */
            status = sloIR_BASE_UseAsTextureCoord(
                                                Compiler,
                                                &unaryExpr->operand->base);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }

            break;
        default: break;
        }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_BINARY_EXPR:
        binaryExpr = (sloIR_BINARY_EXPR)Base;
        gcmASSERT(binaryExpr->leftOperand);
        gcmASSERT(binaryExpr->rightOperand);

        switch (binaryExpr->type)
        {
        case slvBINARY_SUBSCRIPT:
            /* Setup the left operand */
            status = sloIR_BASE_UseAsTextureCoord(
                                                Compiler,
                                                &binaryExpr->leftOperand->base);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }

            break;
        default: break;
        }

        switch (binaryExpr->type)
        {
        case slvBINARY_SEQUENCE:
            /* Setup the right operand */
            status = sloIR_BASE_UseAsTextureCoord(
                                                Compiler,
                                                &binaryExpr->rightOperand->base);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }

            gcmFOOTER_NO();
            return gcvSTATUS_OK;

        default:
            break;
        }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_SELECTION:
        selection = (sloIR_SELECTION)Base;
        gcmASSERT(selection->condExpr);

        /* Setup the true operand */
        if (selection->trueOperand != gcvNULL)
        {
            status = sloIR_BASE_UseAsTextureCoord(
                                                Compiler,
                                                selection->trueOperand);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }

        /* Setup the false operand */
        if (selection->falseOperand != gcvNULL)
        {
            status = sloIR_BASE_UseAsTextureCoord(
                                                Compiler,
                                                selection->falseOperand);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    case slvIR_POLYNARY_EXPR:
        polynaryExpr = (sloIR_POLYNARY_EXPR)Base;

        if (polynaryExpr->type != slvPOLYNARY_FUNC_CALL
            && polynaryExpr->operands != gcvNULL)
        {
            /* Setup all operands */
            status = sloIR_BASE_UseAsTextureCoord(
                                                Compiler,
                                                &polynaryExpr->operands->base);

            if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }
        }

        gcmFOOTER_NO();
        return gcvSTATUS_OK;

    default:
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
}

gceSTATUS
sloCOMPILER_BindFuncCall(
    IN sloCOMPILER Compiler,
    IN OUT sloIR_POLYNARY_EXPR PolynaryExpr
    )
{
    gceSTATUS   status;
    sloIR_BASE  argument;

    gcmHEADER_ARG("Compiler=0x%x PolynaryExpr=0x%x",
                  Compiler, PolynaryExpr);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(PolynaryExpr->type == slvPOLYNARY_FUNC_CALL);

    /* Bind the function name */
    status = slsNAME_SPACE_BindFuncName(
                                        Compiler,
                                        Compiler->context.globalSpace,
                                        PolynaryExpr);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    /* Setup the texture coordinate */
    if (PolynaryExpr->type == slvPOLYNARY_FUNC_CALL
        && PolynaryExpr->funcName->isBuiltIn
        && slIsTextureLookupFunction(PolynaryExpr->funcSymbol))
    {
        gcmASSERT(PolynaryExpr->operands);

        argument = slsDLINK_LIST_First(&PolynaryExpr->operands->members, struct _sloIR_BASE);
        argument = slsDLINK_NODE_Next(&argument->node, struct _sloIR_BASE);

        status = sloIR_BASE_UseAsTextureCoord(Compiler, argument);
		if (gcmIS_ERROR(status))
		{
			gcmFOOTER();
			return status;
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gctREG_INDEX
slNewTempRegs(
    IN sloCOMPILER Compiler,
    IN gctUINT RegCount
    )
{
    gctREG_INDEX    regIndex;

    gcmHEADER_ARG("Compiler=0x%x RegCount=%u",
                  Compiler, RegCount);

    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    regIndex = (gctREG_INDEX) Compiler->context.tempRegCount;

    Compiler->context.tempRegCount += RegCount;

    gcmFOOTER_ARG("<return>=%u", regIndex);
    return regIndex;
}

gctUINT32 *
sloCOMPILER_GetVersion(
	IN sleSHADER_TYPE ShaderType
)
{
  _slCompilerVersion[0] = _sldLanguageType | (ShaderType << 16);
  return _slCompilerVersion;
}

gctLABEL
slNewLabel(
    IN sloCOMPILER Compiler
    )
{
    gctLABEL    label;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

    label = 1 + Compiler->context.labelCount;

    Compiler->context.labelCount++;

    gcmFOOTER_ARG("<return>=%u", label);
    return label;
}

gceSTATUS
sloCOMPILER_BackPatch(
    IN sloCOMPILER Compiler
    )
{
    gcmHEADER();
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    /* Issue on dEQP test dEQP-GLES2.functional.shaders.linkage.varying_4:
    ** Per GLSL spec, VS can declare varying output but not written within
    ** shader, then fragment shader can use this varying input from VS with
    ** undefined value. So we need add this type of varying output of VS in
    ** output table. We defer determination of whether this type of output of
    ** VS needs really to be removed to linkage time of BE. */
    if (Compiler->shaderType == slvSHADER_TYPE_VERTEX)
    {
        slsNAME_SPACE* globalNameSpace = Compiler->context.globalSpace;
        slVSOuputPatch(Compiler, globalNameSpace);
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_SetDefaultPrecision(
	IN sloCOMPILER Compiler,
	IN sltELEMENT_TYPE typeToSet,
    IN sltPRECISION precision
	)
{
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x typeToSet=0x%x precision=0x%x",
                  Compiler, typeToSet, precision);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (typeToSet == slvTYPE_FLOAT)
    {
        Compiler->context.currentSpace->defaultPrecision.floatPrecision =
            (enum _slePRECISION)precision;
    }
    else if (typeToSet == slvTYPE_INT)
    {
        Compiler->context.currentSpace->defaultPrecision.intPrecision =
            (enum _slePRECISION)precision;
    }
    else
    {
        /* TODO: Not implemented yet */
    }

    gcmFOOTER();
    return status;
}

gceSTATUS
sloCOMPILER_GetDefaultPrecision(
	IN sloCOMPILER Compiler,
	IN sltELEMENT_TYPE typeToGet,
    sltPRECISION *precision
	)
{
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x typeToSet=0x%x precision=0x%x",
                  Compiler, typeToGet, precision);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    if (typeToGet == slvTYPE_FLOAT)
    {
        *precision = Compiler->context.currentSpace->defaultPrecision.floatPrecision;
    }
    else if (typeToGet == slvTYPE_INT)
    {
        *precision = Compiler->context.currentSpace->defaultPrecision.intPrecision;
    }
    else
    {
        *precision = (sltPRECISION)slvPRECISION_DEFAULT;
    }

    gcmFOOTER();
    return status;
}

