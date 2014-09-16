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

/*******************************************************************************
**
**
*/
gceSTATUS   ppoHIDE_SET_Dump(
    ppoPREPROCESSOR     PP,
    ppoHIDE_SET HS)
{
    gceSTATUS   status ;

    gcmONERROR(
        sloCOMPILER_Dump(
            PP->compiler,
            slvDUMP_PREPROCESSOR,
            "<HideSet>"
            )
        );

    ppoBASE_Dump(PP, (ppoBASE)HS);

    gcmONERROR(
        sloCOMPILER_Dump(
            PP->compiler,
            slvDUMP_PREPROCESSOR,
            "<NameHided poolString=\"%s\" />",
            HS->macroName
            )
        );

    gcmONERROR(
        sloCOMPILER_Dump(
            PP->compiler,
            slvDUMP_PREPROCESSOR,
            "</HideSet>"
            )
        );

    if(HS->base.node.prev)
    {
        return ppoHIDE_SET_Dump(PP, (ppoHIDE_SET)HS->base.node.prev);
    }
    else
    {
        return gcvSTATUS_OK;
    }

OnError:
	return status;
}

gceSTATUS
ppoHIDE_SET_Destroy(
    ppoPREPROCESSOR PP,
    ppoHIDE_SET     HS
    )
{
    return sloCOMPILER_Free(PP->compiler, HS);
}

/*******************************************************************************
**
**
*/
gceSTATUS   ppoHIDE_SET_Construct(
    ppoPREPROCESSOR             PP,
    gctCONST_STRING     File,
    gctUINT             Line,
    gctCONST_STRING     MoreInfo,
    gctSTRING           MacroName,
    ppoHIDE_SET*        New)
{
    ppoHIDE_SET rt      = gcvNULL;
    gceSTATUS   status  = gcvSTATUS_INVALID_ARGUMENT;
    gctPOINTER pointer = gcvNULL;

    gcmASSERT(
        PP
        && File
        && Line
        && MoreInfo
        && MacroName
        && New
        );

    status = sloCOMPILER_Allocate(
            PP->compiler,
            sizeof(struct _ppoHIDE_SET),
            &pointer
            );

    rt = pointer;

    if( (gcvNULL == rt) || (status != gcvSTATUS_OK))
	{
		ppoPREPROCESSOR_Report(
			PP,
			slvREPORT_INTERNAL_ERROR,
			"ppoHIDE_SET_Construct : Failed to alloc a HideSet object."
			);

		status = gcvSTATUS_OUT_OF_MEMORY;
		return status;
	}

    gcoOS_MemFill(rt, 0, sizeof(struct _ppoHIDE_SET));

    if( gcvNULL == rt)
    {
        ppoPREPROCESSOR_Report(
            PP,
            slvREPORT_INTERNAL_ERROR,
            "ppoHIDE_SET_Construct : Failed to alloc a HideSet object."
            );

        return gcvSTATUS_OUT_OF_MEMORY;
    }

    /*00    base*/
    status = ppoBASE_Init(
        PP,
        (ppoBASE)rt,
        File,
        Line,
        MoreInfo,
        ppvOBJ_HIDE_SET);

    if(status != gcvSTATUS_OK)
    {
        ppoPREPROCESSOR_Report(
            PP,
            slvREPORT_INTERNAL_ERROR,
            "ppoHIDE_SET_Construct : Failed to init the base of a HideSet object.");

        return gcvSTATUS_HEAP_CORRUPTED;
    }

    /*01    macame*/
    rt->macroName = MacroName;

    *New = rt;

    return gcvSTATUS_OK;
}
/*******************************************************************************
**
**
*/
gceSTATUS   ppoHIDE_SET_LIST_ContainSelf    (
    ppoPREPROCESSOR     PP,
    ppoTOKEN    Token ,
    gctBOOL*     Yes)
{
    gctSTRING       poolString  =  gcvNULL;

    ppoHIDE_SET     hideSet     =  gcvNULL;

    gcmASSERT(Token && Token->inputStream.base.type == ppvOBJ_TOKEN);

    poolString  =  Token->poolString;

    hideSet     =  Token->hideSet;

    while(hideSet)
    {
        if(hideSet->macroName == poolString)
        {
            *Yes = gcvTRUE;
            return gcvSTATUS_OK;
        }
        hideSet = (void*)(hideSet->base.node.prev);
    }
    *Yes = gcvFALSE;
    return gcvSTATUS_OK;
}
/*AddtoHS******************************************************************************/
gceSTATUS   ppoHIDE_SET_AddHS(
    ppoPREPROCESSOR     PP,
    ppoTOKEN    Token,
    gctSTRING       MacName)
{
    gceSTATUS   status      = gcvSTATUS_INVALID_ARGUMENT;

    ppoHIDE_SET hideSet     = gcvNULL;

    gcmASSERT(Token && Token->inputStream.base.type == ppvOBJ_TOKEN);

    status = ppoHIDE_SET_Construct(
        PP,
        __FILE__,
        __LINE__,
        "Creat hideSet node to add a new hideSet-node.",
         MacName,
         &hideSet);
    if(status != gcvSTATUS_OK)
    {
        return status;
    }

    if(Token->hideSet)
    {
        Token->hideSet->base.node.next = (void*)hideSet;
        hideSet->base.node.prev = (void*)(Token->hideSet);
        hideSet->base.node.next = gcvNULL;
        Token->hideSet = hideSet;
        return gcvSTATUS_OK;
    }
    else
    {
       Token->hideSet = hideSet;
       return gcvSTATUS_OK;
    }
}
gceSTATUS   ppoHIDE_SET_LIST_Append(
    ppoPREPROCESSOR     PP,
    ppoTOKEN    TarToken,
    ppoTOKEN    SrcToken)
{
    ppoHIDE_SET hideSet     = gcvNULL;
    gcmASSERT(
        TarToken &&
        SrcToken &&
        TarToken->inputStream.base.type == ppvOBJ_TOKEN &&
        SrcToken->inputStream.base.type == ppvOBJ_TOKEN);

    hideSet = SrcToken->hideSet;
    while(hideSet)
    {
        ppoHIDE_SET_AddHS(
            PP,
            TarToken,
            hideSet->macroName);

        hideSet = (ppoHIDE_SET)hideSet->base.node.prev;
    }
    return gcvSTATUS_OK;
}
/*ColonHideSetList******************************************************************************/
gceSTATUS
ppoHIDE_SET_LIST_Colon(
    ppoPREPROCESSOR     PP,ppoHIDE_SET src, ppoHIDE_SET* out)
{
    gceSTATUS   status  = gcvSTATUS_INVALID_ARGUMENT;
    ppoHIDE_SET cur     = gcvNULL;
    ppoHIDE_SET sub     = gcvNULL;
    if(src)
    {
        gcmASSERT(src->base.type == ppvOBJ_HIDE_SET);

        status = ppoHIDE_SET_Construct(
            PP,
            __FILE__,
            __LINE__,
            "Creat for dump a stack of HS node.",
            src->macroName,
            &cur);
        if(status != gcvSTATUS_OK)
        {
            return status;
        }

        if(src->base.node.prev)
        {
            status = ppoHIDE_SET_LIST_Colon(PP,(ppoHIDE_SET)(src->base.node.prev), &sub);
            if(status != gcvSTATUS_OK)
            {
                return status;
            }
        }
        else
        {
            sub = gcvNULL;
        }
        cur->base.node.prev = (void*)sub;
        if(sub)
        {
            sub->base.node.next = (void*)cur;
        }
        *out = cur;
        return gcvSTATUS_OK;
    }
    else
    {
        *out = gcvNULL;
        return gcvSTATUS_OK;
    }
}
