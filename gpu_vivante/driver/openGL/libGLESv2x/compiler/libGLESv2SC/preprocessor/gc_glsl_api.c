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


#include "gc_glsl_preprocessor_int.h"

gceSTATUS
sloPREPROCESSOR_Construct(
    IN  sloCOMPILER     Compiler,
    OUT sloPREPROCESSOR *Preprocessor
    )
{
    gceSTATUS status = gcvSTATUS_INVALID_DATA;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */

    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    gcmASSERT(Preprocessor);

    status = ppoPREPROCESSOR_Construct(Compiler, (ppoPREPROCESSOR*)Preprocessor);

    if (gcmIS_SUCCESS(status))
    {
        gcmFOOTER_ARG("*Preprocessor=0x%x", Preprocessor);
    }
    else
    {
        gcmFOOTER();
    }
    return status;

}

gceSTATUS
sloPREPROCESSOR_Destroy(
    IN sloCOMPILER Compiler,
    IN sloPREPROCESSOR Preprocessor
    )
{
    gceSTATUS status = gcvSTATUS_INVALID_DATA;
    ppoPREPROCESSOR PP = Preprocessor;

    gcmHEADER_ARG("Compiler=0x%x Preprocessor=0x%x", Compiler, Preprocessor);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmVERIFY_ARGUMENT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);

    status = ppoPREPROCESSOR_Destroy(PP);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloPREPROCESSOR_SetSourceStrings(
    IN sloPREPROCESSOR Preprocessor,
    IN gctUINT StringCount,
    IN gctCONST_STRING Strings[]
    )
{
    gceSTATUS           status  = gcvSTATUS_INVALID_ARGUMENT;
    ppoPREPROCESSOR     PP      = (ppoPREPROCESSOR)Preprocessor;

    gcmHEADER_ARG("Preprocessor=0x%x StringCount=%u Strings=0x%x",
                  Preprocessor, StringCount, Strings);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);
    gcmVERIFY_ARGUMENT(StringCount > 0);
    gcmVERIFY_ARGUMENT(Strings);

    status = ppoPREPROCESSOR_SetSourceStrings(PP,
                                              Strings,
                                              gcvNULL,
                                              StringCount);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloPREPROCESSOR_Parse(
    IN      sloPREPROCESSOR     Preprocessor,
    IN      gctINT              MaxSize,
    OUT     gctSTRING           Buffer,
    OUT     gctINT              *ActualSize
    )
{
    gceSTATUS status;

    ppoPREPROCESSOR PP = Preprocessor;

    gcmHEADER_ARG("Preprocessor=0x%x MaxSize=%u Buffer=0x%x",
                  Preprocessor, MaxSize, Buffer);

    /* Verify the arguments. */
    gcmVERIFY_ARGUMENT(PP);
    gcmVERIFY_ARGUMENT(Buffer);
    gcmVERIFY_ARGUMENT(ActualSize);

    MaxSize--;/*for the trail white space.*/

    MaxSize--;/*for the trail \0*/

    if (MaxSize < ppvMAX_PPTOKEN_CHAR_NUMBER)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
            PP->compiler,
            1,
            0,
            slvREPORT_INTERNAL_ERROR,
            "sloPREPROCESSOR_Parse : The output buffer is too small."
            "please set to more than %d",
            ppvMAX_PPTOKEN_CHAR_NUMBER + 2
            ));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    status = ppoPREPROCESSOR_Parse(PP, Buffer, MaxSize, ActualSize);

    if(status != gcvSTATUS_OK)
    {
        gcmFOOTER();
        return status;
    }

    gcmVERIFY_OK(gcoOS_StrCatSafe(Buffer, MaxSize, " "))    ;

    (*ActualSize)++;

    gcmFOOTER_ARG("*ActualSize=%u", *ActualSize);
    return gcvSTATUS_OK;
}

/*******************************************************************************
*/

gceSTATUS
sloCOMPILER_SetDebug(
    sloCOMPILER Compiler,
    gctBOOL     Debug
    )
{
    gcmHEADER_ARG("Compiler=0x%x Debug=%d",
                  Compiler, Debug);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_SetOptimize(
    sloCOMPILER Compiler,
    gctBOOL     Optimize
    )
{
    gcmHEADER_ARG("Compiler=0x%x Optimize=%d",
                  Compiler, Optimize);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_SetVersion(
    sloCOMPILER Compiler,
    gctINT Line
    )
{
    gcmHEADER_ARG("Compiler=0x%x Line=%d",
                  Compiler, Line);

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}




