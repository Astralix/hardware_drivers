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
ppoTOKEN_Destroy(
    ppoPREPROCESSOR PP,
    ppoTOKEN Token
    )
{
    ppoHIDE_SET tmp, prev;

    gceSTATUS   status;

	if(Token == gcvNULL)
	{
		return gcvSTATUS_OK;
	}
    gcmASSERT(PP && PP->base.type == ppvOBJ_PREPROCESSOR && Token);

    tmp = (ppoHIDE_SET)Token->hideSet;

    while(tmp)
    {
        prev = (ppoHIDE_SET)tmp->base.node.prev;

        gcmONERROR(
            ppoHIDE_SET_Destroy(PP, tmp)
            );

        tmp = prev;
    }

    sloCOMPILER_Free(PP->compiler, Token);
	Token = gcvNULL;
    return gcvSTATUS_OK;

OnError:
	return status;
}

gceSTATUS
ppoTOKEN_STREAM_Destroy(
    ppoPREPROCESSOR PP,
    ppoTOKEN_STREAM TS
    )
{
    gceSTATUS status;

    ppoTOKEN t = (ppoTOKEN)TS;

    while(t)
    {
        ppoTOKEN p = (ppoTOKEN)t->inputStream.base.node.prev;

        gcmONERROR(
            ppoTOKEN_Destroy(PP, t)
            );

        t = p;
    }

    return gcvSTATUS_OK;

OnError:
	return status;
}
/*******************************TOKEN FindID***********************************/
gceSTATUS
ppoTOKEN_FindPoolString(
                        ppoPREPROCESSOR     PP,
                        ppoTOKEN_STREAM     TS,
                        gctSTRING               Name,
                        ppoTOKEN*           Finded)
{
    ppoTOKEN tmp = TS;
    while(tmp)
    {
        if(tmp->poolString == Name)
        {
            *Finded = tmp;
            return gcvSTATUS_OK;
        }
        tmp = (ppoTOKEN)tmp->inputStream.base.node.prev;
    }
    return gcvSTATUS_OK;
}

/******************************TOKEN_Creat*************************************/
gceSTATUS
ppoTOKEN_Construct(
                   ppoPREPROCESSOR      PP,
                   /*01*/gctCONST_STRING            File,
                   /*02*/gctINT     Line,
                   /*03*/gctCONST_STRING            MoreInfo,
                   /*04*/ppoTOKEN*      Created)
{
    ppoTOKEN    rt      = gcvNULL;
    gceSTATUS   status  = gcvSTATUS_INVALID_ARGUMENT;
    gctPOINTER  pointer = gcvNULL;
    gcmASSERT(PP && Created);

    gcmONERROR(
        sloCOMPILER_Allocate(
        PP->compiler,
        sizeof(struct _ppoTOKEN),
        &pointer)
        );

    if(status != gcvSTATUS_OK)
    {
        return status;
    }

    rt = pointer;

    gcoOS_MemFill(rt, 0, sizeof(*rt));
    /*00    isBase*/
    gcmONERROR(
        ppoINPUT_STREAM_Init(
        PP,
        /*00    youris*/    (void*)rt,
        /*01    file*/      File,
        /*02    line*/      Line,
        /*03    moreInfo*/  MoreInfo,
        /*04    istype*/    ppvOBJ_TOKEN,
        /*05    gettoken*/  ppoTOKEN_GetToken)
        );

    /*02    type*/
    rt->type = ppvTokenType_ERROR;
    /*03    hideSet*/
    rt->hideSet = gcvNULL;
    /*04    poolString*/
    rt->poolString = gcvNULL;
    /*05*/ rt->srcFileString = 0;
    /*06*/ rt->srcFileLine = 0;

    *Created = rt;
    return gcvSTATUS_OK;
OnError:
	if (rt != gcvNULL)
	{
		gcmVERIFY_OK(sloCOMPILER_Free(PP->compiler, rt));
        rt = gcvNULL;
	}
	return status;
}
/*******************    ppoTOKEN GetToken *************************************/
gceSTATUS
ppoTOKEN_GetToken(
                  ppoPREPROCESSOR       PP,
                  ppoINPUT_STREAM* IS,
                  ppoTOKEN*     Token,
                  gctBOOL           WhiteSpace)
{
    gcmASSERT(IS && (*IS)->base.type == ppvOBJ_TOKEN);

    *Token = (ppoTOKEN)*IS;
    *IS = (void*)(*IS)->base.node.prev;

    (*Token)->inputStream.base.node.prev = gcvNULL;
    (*Token)->inputStream.base.node.next = gcvNULL;
    return  gcvSTATUS_OK;
}
/******************************************************************************\
TOKEN ColonToken
Colon a token, the base, should be clear.
\******************************************************************************/
gceSTATUS
ppoTOKEN_Colon(
               ppoPREPROCESSOR      PP,
               ppoTOKEN Token,
               gctCONST_STRING      File,
               gctINT       Line,
               gctCONST_STRING      MoreInfo,
               ppoTOKEN*    Coloned)
{
    gceSTATUS   status = gcvSTATUS_OK;
    gcmASSERT(Token && Token->inputStream.base.type == ppvOBJ_TOKEN);

    /*00*/
    status = ppoTOKEN_Construct(
        PP,
        File,
        Line,
        MoreInfo,
        Coloned);

    if(status != gcvSTATUS_OK){
        return status;
    }

    /*02    type*/
    (*Coloned)->type = Token->type;
    /*03    hideSet*/
    status = ppoHIDE_SET_LIST_Colon(PP, Token->hideSet, &((*Coloned)->hideSet));
    if(status != gcvSTATUS_OK)
    {
        return status;
    }
    /*04    poolString*/
    (*Coloned)->poolString = Token->poolString;
    return gcvSTATUS_OK;
}

/******************************************************************************\
Colon   Token   List
\******************************************************************************/
gceSTATUS
ppoTOKEN_ColonTokenList(
                        ppoPREPROCESSOR     PP,
                        ppoTOKEN    SrcTLst,
                        gctCONST_STRING     File,
                        gctINT      Line,
                        gctCONST_STRING     MoreInfo,
                        ppoTOKEN*   ColonedHead)
{
    gceSTATUS   status = gcvSTATUS_OK;

    if(!SrcTLst)
    {
        *ColonedHead = gcvNULL;
        return gcvSTATUS_OK;
    }

    status = ppoTOKEN_Colon(PP, SrcTLst, File, Line, MoreInfo, ColonedHead);
    if(status != gcvSTATUS_OK)
    {
        return status;
    }
    if(SrcTLst->inputStream.base.node.prev)
    {
        return ppoTOKEN_ColonTokenList(
            PP,
            (void*)(SrcTLst->inputStream.base.node.prev),
            File,
            Line,
            MoreInfo,
            (void*)&((*ColonedHead)->inputStream.base.node.prev));
    }
    else
    {
        (*ColonedHead)->inputStream.base.node.prev = gcvNULL;
        return gcvSTATUS_OK;
    }
}
/********************** TOKEN DumpTokenList ***********************************/
gceSTATUS
ppoTOKEN_Dump(
              ppoPREPROCESSOR       PP,
              ppoTOKEN    Token)
{
    ppoTOKEN local = gcvNULL;

    gcmASSERT(Token && Token->inputStream.base.type == ppvOBJ_TOKEN);

    local = Token;

    sloCOMPILER_Dump(PP->compiler, slvDUMP_PREPROCESSOR, "<Token ");

    if(local->poolString == PP->keyword->newline)
    {
        sloCOMPILER_Dump(PP->compiler, slvDUMP_PREPROCESSOR, " poolString=\"NewLine\">");
    }
    else
    {
        sloCOMPILER_Dump(PP->compiler, slvDUMP_PREPROCESSOR, " poolString=\"%s\">",
            local->poolString);
    }

    if(local->hideSet)
    {
        ppoHIDE_SET_Dump(PP, local->hideSet);
    }

    sloCOMPILER_Dump(PP->compiler, slvDUMP_PREPROCESSOR, "</Token>");


    if(Token->inputStream.base.node.prev)
    {
        return ((ppoINPUT_STREAM)(Token->inputStream.base.node.prev))->Dump(PP, (ppoINPUT_STREAM)Token->inputStream.base.node.prev);
    }

    return gcvSTATUS_OK ;
}

gceSTATUS
ppoTOKEN_STREAM_Dump(
                     ppoPREPROCESSOR PP,
                     ppoTOKEN_STREAM TS
                     )
{
    return ppoTOKEN_Dump(PP, (ppoTOKEN)TS);
}

/******************************************************************************\
IsInIdTokenList
\******************************************************************************/
gceSTATUS
ppoTOKEN_STREAM_FindID(
                       ppoPREPROCESSOR      PP,ppoTOKEN TokenList, gctSTRING ID, gctBOOL* FindOrNot)
{
    ppoTOKEN tmp = TokenList;
    *FindOrNot = gcvFALSE;
    while(tmp)
    {
        gcmASSERT(tmp->type == ppvTokenType_ID);
        if(tmp->poolString == ID)
        {
            *FindOrNot = gcvTRUE;
            return gcvSTATUS_OK;
        }
        tmp = (void*)(tmp->inputStream.base.node.prev);
    }
    return gcvSTATUS_OK;
}











