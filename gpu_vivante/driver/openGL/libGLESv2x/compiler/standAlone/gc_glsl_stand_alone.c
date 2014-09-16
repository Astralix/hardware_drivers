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


#ifndef _CRT_SECURE_NO_DEPRECATE
#   define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef _WIN32
#include <windows.h>
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gc_hal.h"
#include "gc_hal_driver.h"
#include "gc_hal_types.h"
#include "gc_hal_compiler.h"

static gctINT
GetShaderType(
    IN gctCONST_STRING FileName
    )
{
    gctCONST_STRING ext;

    gcmASSERT(FileName);

    ext = strrchr(FileName, '.');

    if (ext != gcvNULL)
    {
        if (gcmIS_SUCCESS(gcoOS_StrNCmp(ext, ".frag", 4))) return gcSHADER_TYPE_FRAGMENT;
        if (gcmIS_SUCCESS(gcoOS_StrNCmp(ext, ".vert", 4))) return gcSHADER_TYPE_VERTEX;
    }

    return gcSHADER_TYPE_FRAGMENT;
}

static gctBOOL
ReadSource(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    OUT gctSIZE_T * SourceSize,
    OUT gctSTRING * Source
    )
{
    gceSTATUS   status;
    gctFILE     file;
    gctUINT32   count;
    gctSIZE_T   byteRead;
    gctSTRING   source;

    gcmASSERT(FileName);

    status = gcoOS_Open(Os, FileName, gcvFILE_READ, &file);

    if (gcmIS_ERROR(status))
    {
        printf("*ERROR* Failed to open input file: %s\n", FileName);
        return gcvFALSE;
    }

    gcmVERIFY_OK(gcoOS_Seek(Os, file, 0, gcvFILE_SEEK_END));
    gcmVERIFY_OK(gcoOS_GetPos(Os, file, &count));

    status = gcoOS_Allocate(Os, count + 1, (gctPOINTER *) &source);
    if (!gcmIS_SUCCESS(status))
    {
        printf("*ERROR* Not enough memory\n");
        gcmVERIFY_OK(gcoOS_Close(Os, file));
        return gcvFALSE;
    }

    gcmVERIFY_OK(gcoOS_SetPos(Os, file, 0));
    status = gcoOS_Read(Os, file, count, source, &byteRead);
    if (!gcmIS_SUCCESS(status) || byteRead != count)
    {
        printf("*ERROR* Failed to open input file: %s\n", FileName);
        gcmVERIFY_OK(gcoOS_Close(Os, file));
        return gcvFALSE;
    }

    source[count] = '\0';
    gcmVERIFY_OK(gcoOS_Close(Os, file));

    *SourceSize = count;
    *Source = source;
    return gcvTRUE;
}

static gctBOOL
OutputShaderData(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    IN gctSIZE_T Size,
    IN gctCONST_STRING Data
    )
{
    gceSTATUS   status;
    gctSTRING   dataFileName;
    gctSIZE_T   length;
    gctFILE     file;

    gcmASSERT(FileName);

    gcmVERIFY_OK(gcoOS_StrLen(FileName, &length));
    length += 6;
    gcmVERIFY_OK(gcoOS_Allocate(Os, length, (gctPOINTER *) &dataFileName));
    gcmVERIFY_OK(gcoOS_StrCopySafe(dataFileName, length, FileName));
    gcmVERIFY_OK(gcoOS_StrCatSafe(dataFileName, length, ".gcSL"));

    status = gcoOS_Open(Os, dataFileName, gcvFILE_CREATE, &file);

    if (gcmIS_ERROR(status))
    {
        gcoOS_Free(Os, dataFileName);
        printf("*ERROR* Failed to open the data file: %s\n", dataFileName);
        return gcvFALSE;
    }
    gcoOS_Free(Os, dataFileName);

    gcmASSERT(file);
    status = gcoOS_Write(Os, file, Size, Data);

    if (!gcmIS_SUCCESS(status))
    {
        printf("*ERROR* Failed to write the data file: %s\n", dataFileName);
        goto ErrorExit;
    }

    gcmVERIFY_OK(gcoOS_Close(Os, file));
    return gcvTRUE;

ErrorExit:
    gcmVERIFY_OK(gcoOS_Close(Os, file));
    return gcvFALSE;
}

static gcSHADER
CompileFile(
    IN gcoOS Os,
    IN gctCONST_STRING FileName,
    IN gcoHAL Hal,
    IN gctUINT Option,
    IN gctBOOL DumpLog,
    IN gctBOOL DumpCompiledShader
    )
{
    gceSTATUS   status;
    gctINT      shaderType;
    gctSIZE_T   sourceSize;
    gctSTRING   source          = gcvNULL;
    gcSHADER    binary;
    gctSTRING   log             = gcvNULL;
    gctSIZE_T   bufferSize;
    gctSTRING   buffer;
    gctSTRING   dataFileName    = gcvNULL;
    gctSIZE_T   length;
    gctFILE     logFile         = gcvNULL;

    gcmASSERT(FileName);

    shaderType = GetShaderType(FileName);

    if (!ReadSource(Os, FileName, &sourceSize, &source)) return gcvNULL;

    status = gcCompileShader(Hal,
                             shaderType,
                             sourceSize, source,
                             &binary,
                             &log);

    if (log != gcvNULL)
    {
        printf("<LOG>\n");
        printf("%s", log);
        printf("</LOG>\n");
    }

    if (gcmIS_ERROR(status))
    {
        gcmASSERT(binary == gcvNULL);
        binary = gcvNULL;

        printf("*ERROR* Failed to compile %s (error: %d)\n", FileName, status);
        goto Exit;
    }

    gcmASSERT(binary != gcvNULL);

    if (DumpLog)
    {
        gcmVERIFY_OK(gcoOS_StrLen(FileName, &length));
        length += 5;
        gcmVERIFY_OK(gcoOS_Allocate(Os, length, (gctPOINTER *) &dataFileName));
        gcmVERIFY_OK(gcoOS_StrCopySafe(dataFileName, length, FileName));
        gcmVERIFY_OK(gcoOS_StrCatSafe(dataFileName, length, ".log"));

        status = gcoOS_Open(Os, dataFileName, gcvFILE_CREATETEXT, &logFile);
        if (gcmIS_ERROR(status))
        {
            logFile = gcvNULL;

            printf("*ERROR* Failed to open the log file: %s\n", dataFileName);
        }
        gcoOS_Free(Os, dataFileName);
    }

    gcmVERIFY_OK(gcSHADER_SetOptimizationOption(binary, Option));

    status = gcOptimizeShader(binary, logFile);

    if (!gcmIS_SUCCESS(status))
    {
        printf("*ERROR* Failed to optimize %s (error: %d)\n", FileName, status);
    }

    if (logFile != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_Close(Os, logFile));
    }

    if (DumpCompiledShader)
    {
        status = gcSHADER_Save(binary, gcvNULL, &bufferSize);
        if (gcmIS_ERROR(status))
        {
            printf("*ERROR* Failed to get the buffer size of the shader\n");
            goto Exit;
        }

        status = gcoOS_Allocate(Os, bufferSize, (gctPOINTER *) &buffer);
        if (!gcmIS_SUCCESS(status))
        {
            printf("*ERROR* Not enough memory\n");
            goto Exit;
        }

        status = gcSHADER_Save(binary, buffer, &bufferSize);
        if (status != gcvSTATUS_OK)
        {
            printf("*ERROR* Failed to get the buffer size of the shader\n");
            gcoOS_Free(Os, buffer);
            goto Exit;
        }

        OutputShaderData(Os, FileName, bufferSize, buffer);

        gcoOS_Free(Os, buffer);
    }


Exit:
    if (DumpLog && log != gcvNULL)
    {
        printf("**************** Compile Log ****************");
        printf("%s", log);
        printf("*********************************************");
    }

    if (log != gcvNULL) gcoOS_Free(Os, log);

    if (source != gcvNULL) gcoOS_Free(Os, source);

    return binary;
}

int
main(
    int argc,
    char * argv[]
    )
{
    gctBOOL         dumpLog      = gcvFALSE;
    gctSTRING       fileName[2]  = { gcvNULL, gcvNULL };
    gcSHADER        shaders[2]   = { gcvNULL, gcvNULL };
    gctINT          i;
    gcoOS           os           = gcvNULL;
    gcoHAL          hal          = gcvNULL;
    gceSTATUS       result;
    gctUINT         option       = 1;   /* no optimization */
    char            outFile[128] = { '\0' };
    char            logVSFile[128] = { '\0' };
    char            logFSFile[128] = { '\0' };

    printf("vCompile version 0.8, Copyright (c) 2005-2011, Vivante Corporation\n\n");

#ifdef _WIN32
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) /*| _CRTDBG_CHECK_ALWAYS_DF*/ | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
    /* _CrtSetBreakAlloc(79); */
#endif

#if gcdDEBUG
    gcoOS_SetDebugLevel(gcvLEVEL_VERBOSE);
    gcoOS_SetDebugZone(gcvZONE_COMPILER);
#endif

    for (i = 1; i < argc; i++)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(argv[i], "-l")))
        {
            dumpLog = gcvTRUE;
        }
        else if (gcmIS_SUCCESS(gcoOS_StrCmp(argv[i], "-O0")))
        {
            /* Currently, optimization level is either FULL or NONE */
            option = 0;     /* no optimization */
        }
        else if (gcmIS_SUCCESS(gcoOS_StrCmp(argv[i], "-O")))
        {
            option = 1;     /* full optimization */
        }
        else if (gcmIS_SUCCESS(gcoOS_StrCmp(argv[i], "-OT")))
        {
            /* For optimization unit test */
            if (i++ == argc)
            {
                printf("*ERROR* Optimization testing pattern not provided.\n");
                return 1;
            }
            else
            {
                gctINT testPattern;

                gcmVERIFY_OK(gcoOS_StrToInt(argv[i], (gctINT *)&testPattern));
                if (testPattern < 0)
                {
                    printf("*ERROR* Unknown optimization testing pattern.\n");
                    return 1;
                }
                option = testPattern;
            }
        }
        else
        {
            if (fileName[0] == gcvNULL) fileName[0] = argv[i];
            else if (fileName[1] == gcvNULL) fileName[1] = argv[i];
            else
            {
                printf("*ERROR* Too many shaders.\n");
                return 1;
            }
        }
    }

    if (fileName[0] == gcvNULL)
    {
        printf("Usage: %s [-l] [-O0] shaderFileName [shaderFileName]\n", argv[0]);
        printf("  -l   Generate log file.\n"
               "  -O0  Disable optimizations.\n"
               "\n"
               "If only one shader is specified, that shader will be compiled into a .gcSL\n"
               "file.  If two shaders are specified, those shaders will be compiled and\n"
               "linked into a .gcPGM file. With two shaders, the vertex shader file needs\n"
               "to be the first.\n");
        return 0;
    }

    result = gcoOS_Construct(gcvNULL, &os);
    if (result != gcvSTATUS_OK)
    {
        printf("*ERROR* Failed to construct a new gcoOS object\n");
        return 1;
    }

    result = gcoHAL_Construct(gcvNULL, os, &hal);
    if (result != gcvSTATUS_OK)
    {
        printf("*ERROR* Failed to construct a new gcoHAL object\n");
        goto ErrorExit;
    }

    /* Dump compile log only when one shader is present */
    shaders[0] = CompileFile(os, fileName[0], hal, option, dumpLog && (fileName[1] == gcvNULL), fileName[1] == gcvNULL );

    if (shaders[0] == gcvNULL)
    {
        goto ErrorExit;
    }

    if (fileName[1] != gcvNULL)
    {
        gctSIZE_T programBufferSize = 0;
        gctPOINTER programBuffer = gcvNULL;
        gcsHINT_PTR hints = gcvNULL;
        gceSTATUS status;
        gctPOINTER binary = gcvNULL;
        gctSIZE_T binarySize = 0;
        FILE * f;
        gctSTRING p;

        gcoOS_StrCopySafe(outFile, gcmSIZEOF(outFile), fileName[0]);
        p = strrchr(outFile, '.');
        gcoOS_StrCopySafe(p, gcmSIZEOF(outFile) - (p - outFile), ".gcPGM");

        gcoOS_StrCopySafe(logVSFile, gcmSIZEOF(logVSFile), fileName[0]);
        gcoOS_StrCatSafe(logVSFile, gcmSIZEOF(logVSFile), ".log");

        gcoOS_StrCopySafe(logFSFile, gcmSIZEOF(logFSFile), fileName[1]);
        gcoOS_StrCatSafe(logFSFile, gcmSIZEOF(logFSFile), ".log");

        shaders[1] = CompileFile(os, fileName[1], hal, option, gcvFALSE, gcvFALSE);
        if (shaders[1] == gcvNULL)
        {
            goto ErrorExit;
        }

        if ( dumpLog)
        {
            gcoOS_SetDebugShaderFiles(logVSFile, logFSFile);
        }
        status = gcLinkShaders(shaders[0],
                               shaders[1],
                               gcvSHADER_DEAD_CODE
                               | gcvSHADER_RESOURCE_USAGE
                               | gcvSHADER_OPTIMIZER
                               | gcvSHADER_USE_GL_Z
                               | gcvSHADER_USE_GL_POSITION
                               | gcvSHADER_USE_GL_FACE,
                               &programBufferSize,
                               &programBuffer,
                               &hints);
        if ( dumpLog)
        {
            gcoOS_SetDebugShaderFiles(gcvNULL, gcvNULL);
        }

        if (gcmIS_ERROR(status))
        {
            printf("*ERROR* gcLinkShaders returned errror %d\n", status);
        }
        else
        {
            int ret;
            status = gcSaveProgram(shaders[0],
                                   shaders[1],
                                   programBufferSize,
                                   programBuffer,
                                   hints,
                                   &binary,
                                   &binarySize);

            if (gcmIS_ERROR(status))
            {
                printf("*ERROR* gcSaveShaders returned errror %d\n", status);
            }

            f = fopen(outFile, "wb");
            ret = fwrite(binary, binarySize, 1, f);
            if (ret);
            fclose(f);
        }

        if (programBuffer != gcvNULL) gcoOS_Free(os, programBuffer);
        if (hints         != gcvNULL) gcoOS_Free(os, hints);
        if (binary        != gcvNULL) gcoOS_Free(os, binary);
    }

    gcSHADER_Destroy(shaders[0]);
    if (shaders[1] != gcvNULL) gcSHADER_Destroy(shaders[1]);
    gcoHAL_Destroy(hal);
    gcoOS_Destroy(os);
    return 0;

ErrorExit:
    if (shaders[0] != gcvNULL) gcSHADER_Destroy(shaders[0]);
    if (shaders[1] != gcvNULL) gcSHADER_Destroy(shaders[1]);
    if (gcvNULL != hal) gcoHAL_Destroy(hal);
    if (gcvNULL != os) gcoOS_Destroy(os);

    return 1;
}
