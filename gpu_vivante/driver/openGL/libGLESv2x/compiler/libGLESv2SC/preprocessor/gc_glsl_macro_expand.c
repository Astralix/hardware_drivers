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
**  ppoPREPROCESSOR_MacroExpand
*/

gceSTATUS
ppoPREPROCESSOR_MacroExpand(
                            ppoPREPROCESSOR     PP,
                            ppoINPUT_STREAM     *IS,
                            ppoTOKEN            *Head,
                            ppoTOKEN            *End,
                            gctBOOL             *AnyExpanationHappened)
{
    gceSTATUS status = gcvSTATUS_INVALID_DATA;
    ppoTOKEN *headtail = gcvNULL;
    ppoTOKEN *expanded_headtail = gcvNULL;
    gctBOOL match_case = gcvFALSE;
    ppoTOKEN id = gcvNULL;
    ppoMACRO_SYMBOL ms = gcvNULL;
    gctPOINTER pointer = gcvNULL;

    if ((*IS) == gcvNULL)
    {
        *AnyExpanationHappened  = gcvFALSE;
        *Head       = gcvNULL;
        *End        = gcvNULL;

        return gcvSTATUS_OK;
    }

    gcmONERROR(
        ppoPREPROCESSOR_MacroExpand_0_SelfContain(PP, IS, Head, End, AnyExpanationHappened, &match_case, &id)
        );

    if (match_case == gcvTRUE) return gcvSTATUS_OK;

    gcmONERROR(
        ppoPREPROCESSOR_MacroExpand_1_NotMacroSymbol(PP, IS, Head, End, AnyExpanationHappened, &match_case, id, &ms)
        );

    if (match_case == gcvTRUE)  return gcvSTATUS_OK;


    gcmONERROR(
        ppoPREPROCESSOR_MacroExpand_2_NoFormalArgs(PP, IS, Head, End, AnyExpanationHappened, &match_case, id, ms)
        );

    if (match_case == gcvTRUE)  return gcvSTATUS_OK;

    gcmONERROR(
        ppoPREPROCESSOR_MacroExpand_3_NoMoreTokenInIS(PP, IS, Head, End, AnyExpanationHappened, &match_case, id)
        );

    if (match_case == gcvTRUE)  return gcvSTATUS_OK;

    gcmONERROR(
        ppoPREPROCESSOR_MacroExpand_4_NoRealArg(PP, IS, Head, End, AnyExpanationHappened, &match_case, id)
        );

    if (match_case == gcvTRUE)  return gcvSTATUS_OK;

    status = sloCOMPILER_Allocate(
        PP->compiler,
        sizeof(ppoTOKEN)*2*(ms->argc),
        &pointer
        );

    if (gcmIS_ERROR(status))
        return status;

    headtail = pointer;

    gcoOS_MemFill((gctPOINTER)headtail, 0, sizeof(ppoTOKEN) * 2 * (ms->argc));

    gcmONERROR(sloCOMPILER_Allocate(
        PP->compiler,
        sizeof(ppoTOKEN)*2*(ms->argc),
        (void*)&expanded_headtail));

    gcoOS_MemFill((gctPOINTER)expanded_headtail, 0, sizeof(ppoTOKEN) * 2 * (ms->argc));

    gcmONERROR(
        ppoPREPROCESSOR_MacroExpand_5_BufferRealArgs(PP, IS, headtail, id, ms)
        );

    if (match_case == gcvTRUE)  return gcvSTATUS_OK;

    gcmONERROR(
        ppoPREPROCESSOR_MacroExpand_6_ExpandHeadTail(PP, IS, headtail, expanded_headtail, id, ms)
        );

    gcmONERROR(
        ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList(PP, IS, Head, End, AnyExpanationHappened,expanded_headtail, id, ms)
        );

    gcmONERROR(
        sloCOMPILER_Free(PP->compiler, headtail)
        );

    gcmONERROR(
        sloCOMPILER_Free(PP->compiler, expanded_headtail)
        );

    return gcvSTATUS_OK;

OnError:
	if (headtail != gcvNULL)
	{
		gcmVERIFY_OK(sloCOMPILER_Free(PP->compiler, headtail));
		headtail = gcvNULL;
	}

	if (expanded_headtail != gcvNULL)
	{
		gcmVERIFY_OK(sloCOMPILER_Free(PP->compiler, expanded_headtail));
		expanded_headtail = gcvNULL;
	}
	return status;
}




gceSTATUS
ppoPREPROCESSOR_MacroExpand_0_SelfContain(
    ppoPREPROCESSOR     PP,
    ppoINPUT_STREAM     *IS,
    ppoTOKEN            *Head,
    ppoTOKEN            *End,
    gctBOOL             *AnyExpanationHappened,
    gctBOOL             *MatchCase,
    ppoTOKEN            *ID)
{
    gceSTATUS status = gcvSTATUS_INVALID_DATA;

    ppoTOKEN id = gcvNULL;

    gctBOOL inhs = gcvFALSE;

    gcmONERROR(
        (*IS)->GetToken(PP, IS, &id, !ppvICareWhiteSpace)
        );

    gcmASSERT(id && id->type == ppvTokenType_ID);

    *ID = id;

    gcmTRACE(gcvLEVEL_VERBOSE, "ME : Begin to process %s.", id->poolString);

    gcmONERROR(
        ppoHIDE_SET_LIST_ContainSelf(PP, id, &inhs)
        );

    if (inhs == gcvTRUE)
    {
        gcmTRACE(gcvLEVEL_VERBOSE, "ME : self-contain.",id->poolString);

        *Head = id;
        *End  = id;
        *AnyExpanationHappened = gcvFALSE;
        *MatchCase = gcvTRUE;

        return gcvSTATUS_OK;
    }
    else
    {
        gcmTRACE(gcvLEVEL_VERBOSE, "ME : not self-contain.",id->poolString);

        *Head = gcvNULL;
        *End  = gcvNULL;
        *AnyExpanationHappened = gcvFALSE;
        *MatchCase = gcvFALSE;

        return gcvSTATUS_OK;
    }
OnError:
    if (id != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, id));
        id = gcvNULL;
    }
    return status;
}



gceSTATUS
ppoPREPROCESSOR_MacroExpand_1_NotMacroSymbol(
    ppoPREPROCESSOR     PP,
    ppoINPUT_STREAM     *IS,
    ppoTOKEN            *Head,
    ppoTOKEN            *End,
    gctBOOL             *AnyExpanationHappened,
    gctBOOL             *MatchCase,
    ppoTOKEN            ID,
    ppoMACRO_SYMBOL     *MS
    )
{

    gceSTATUS status = gcvSTATUS_INVALID_DATA;

    ppoTOKEN id = ID;

    ppoMACRO_SYMBOL ms = gcvNULL;

    gcmONERROR(
        ppoMACRO_MANAGER_GetMacroSymbol(PP, PP->macroManager, id->poolString, &ms)
        );

    *MS = ms;

    if (ms == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_VERBOSE,"ME : %s is not defined.",id->poolString);

        *Head = id;
        *End  = id;
        *AnyExpanationHappened = gcvFALSE;
        *MatchCase = gcvTRUE;

        return gcvSTATUS_OK;
    }
    else
    {
        gcmTRACE(gcvLEVEL_VERBOSE,"ME : %s is defined.",id->poolString);

        *Head = gcvNULL;
        *End  = gcvNULL;
        *AnyExpanationHappened = gcvFALSE;
        *MatchCase = gcvFALSE;

        return gcvSTATUS_OK;
    }
OnError:
	return status;
}




gceSTATUS
ppoPREPROCESSOR_MacroExpand_2_NoFormalArgs(
    ppoPREPROCESSOR     PP,
    ppoINPUT_STREAM     *IS,
    ppoTOKEN            *Head,
    ppoTOKEN            *End,
    gctBOOL             *AnyExpanationHappened,
    gctBOOL             *MatchCase,
    ppoTOKEN            ID,
    ppoMACRO_SYMBOL     MS)
{
    gceSTATUS status = gcvSTATUS_INVALID_DATA;

    ppoTOKEN id = ID;

    ppoMACRO_SYMBOL ms = MS;

    ppoTOKEN replacement_list = gcvNULL;

    gcmHEADER_ARG("PP=0x%x IS=0x%x Head=0x%x End=0x%x AnyExpanationHappened=0x%x MatchCase=0x%x ID=0x%x MS=0x%x",
                  PP, IS, Head, End, AnyExpanationHappened, MatchCase, ID, MS);

    gcmASSERT(ms != gcvNULL);

    if (ms->argc == 0)
    {
        gcmTRACE(gcvLEVEL_VERBOSE, "ME : macro %s has no arg(s).",id->poolString);

        if (ms->replacementList == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_VERBOSE, "ME : macro %s, has no replacement-list.",id->poolString);

            *Head = gcvNULL;
            *End  = gcvNULL;
            *AnyExpanationHappened = gcvTRUE;
            *MatchCase = gcvTRUE;

            status = ppoTOKEN_Destroy(PP,id);

            if (gcmIS_SUCCESS(status))
            {
                gcmFOOTER_ARG("*Head=0x%x *End=0x%x *AnyExpanationHappened=%d *MatchCase=%d",
                              *Head, *End, *AnyExpanationHappened, *MatchCase);
                return gcvSTATUS_OK;
            }
            else
            {
                gcmFOOTER();
                return status;
            }
        }

        gcmTRACE(gcvLEVEL_VERBOSE, "ME : macro %s, has replacement-list.",id->poolString);

        gcmTRACE(gcvLEVEL_VERBOSE, "ME : macro %s, colon replacement-list.",id->poolString);

        gcmONERROR(ppoTOKEN_ColonTokenList(
            PP,
            ms->replacementList,
            __FILE__,
            __LINE__,
            "ME : colon replacementList",
            &replacement_list)
            );

        *Head = replacement_list;

        gcmTRACE(gcvLEVEL_VERBOSE, "ME : macro %s, add hs.",id->poolString);

        while(replacement_list)
        {
            gcmASSERT(replacement_list->hideSet == gcvNULL);

            ppoHIDE_SET_LIST_Append(
                PP,
                replacement_list,
                id);

            ppoHIDE_SET_AddHS(PP, replacement_list, id->poolString);

            if((void*)replacement_list->inputStream.base.node.prev == gcvNULL)
            {
                *End = replacement_list;
            }

            replacement_list = (ppoTOKEN)replacement_list->inputStream.base.node.prev;
        }

        *AnyExpanationHappened = gcvTRUE;

        *MatchCase = gcvTRUE;

        status = ppoTOKEN_Destroy(PP,id);

        if (gcmIS_SUCCESS(status))
        {
            gcmFOOTER_ARG("*Head=0x%x *End=0x%x *AnyExpanationHappened=%d *MatchCase=%d",
                          *Head, *End, *AnyExpanationHappened, *MatchCase);
            return gcvSTATUS_OK;
        }
        else
        {
            gcmFOOTER();
            return status;
        }

    }
    else/*if (ms->argc == 0)*/
    {
        *Head   = gcvNULL;
        *End    = gcvNULL;
        *AnyExpanationHappened = gcvFALSE;
        *MatchCase = gcvFALSE;

        gcmFOOTER_ARG("*Head=0x%x *End=0x%x *AnyExpanationHappened=%d *MatchCase=%d",
                      *Head, *End, *AnyExpanationHappened, *MatchCase);
        return gcvSTATUS_OK;
    }

OnError:
    gcmFOOTER();
	return status;
}



gceSTATUS
ppoPREPROCESSOR_MacroExpand_3_NoMoreTokenInIS(
    ppoPREPROCESSOR     PP,
    ppoINPUT_STREAM     *IS,
    ppoTOKEN            *Head,
    ppoTOKEN            *End,
    gctBOOL             *AnyExpanationHappened,
    gctBOOL             *MatchCase,
    ppoTOKEN            ID
    )
{
    ppoTOKEN id = ID;

    if ((*IS) == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_VERBOSE, "ME : %s, no real arg list.", id->poolString);

        *Head = id;
        *End  = id;
        *AnyExpanationHappened = gcvFALSE;
        *MatchCase = gcvTRUE;
    }
    else
    {
        gcmTRACE(gcvLEVEL_VERBOSE, "ME : %s, has real arg list.", id->poolString);

        *AnyExpanationHappened = gcvFALSE;
        *MatchCase = gcvFALSE;
    }

    return gcvSTATUS_OK;
}




gceSTATUS
ppoPREPROCESSOR_MacroExpand_4_NoRealArg(
                                        ppoPREPROCESSOR     PP,
                                        ppoINPUT_STREAM     *IS,
                                        ppoTOKEN            *Head,
                                        ppoTOKEN            *End,
                                        gctBOOL             *AnyExpanationHappened,
                                        gctBOOL             *MatchCase,
                                        ppoTOKEN            ID
                                        )
{
    gceSTATUS status;

    ppoTOKEN id = ID;

    ppoTOKEN ahead = gcvNULL;

    gcmONERROR(
        (*IS)->GetToken(PP, IS, &ahead, !ppvICareWhiteSpace)
        );

    if (ahead->poolString != PP->keyword->lpara)
    {
        gcmTRACE(gcvLEVEL_VERBOSE, "ME : %s, no real arg.", id->poolString);

        *Head = id;
        *End  = id;
        *AnyExpanationHappened = gcvFALSE;
        *MatchCase = gcvTRUE;

        gcmONERROR(
            ppoINPUT_STREAM_UnGetToken(PP, (ppoINPUT_STREAM *)IS, ahead));

        gcmONERROR(ppoTOKEN_Destroy(PP, ahead));

        return gcvSTATUS_OK;
    }
    else
    {
        gcmTRACE(gcvLEVEL_VERBOSE, "ME : %s, real arg.", id->poolString);

        *MatchCase = gcvFALSE;
        *AnyExpanationHappened = gcvFALSE;

        gcmONERROR(
            ppoINPUT_STREAM_UnGetToken(PP, IS, ahead)
            );

        return gcvSTATUS_OK;
    }
OnError:
    if (ahead != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ahead));
        ahead = gcvNULL;
    }
	return status;
}



gceSTATUS
ppoPREPROCESSOR_MacroExpand_5_BufferRealArgs(
    ppoPREPROCESSOR     PP,
    ppoINPUT_STREAM     *IS,
    ppoTOKEN            *HeadTail,
    ppoTOKEN            ID,
    ppoMACRO_SYMBOL     MS
    )
{
    gceSTATUS status;

    ppoTOKEN id = ID;

    ppoMACRO_SYMBOL ms = MS;

    ppoTOKEN ahead = gcvNULL;

    gctINT real_argc;

    gcmONERROR(
        (*IS)->GetToken(PP, IS, &ahead, !ppvICareWhiteSpace)
        );

    gcmTRACE(gcvLEVEL_VERBOSE, "ME : %s, buffer real-arg(s).", id->poolString);

    real_argc = 0;

    while (ahead->poolString != PP->keyword->rpara)
    {
        gcmONERROR(ppoTOKEN_Destroy(PP, ahead));
        ahead = gcvNULL;

        if (real_argc < ms->argc)
        {
            gcmTRACE(gcvLEVEL_VERBOSE, "ME : %s, buffer the %dth arg.", id->poolString, real_argc);

            ppoPREPROCESSOR_BufferActualArgs(
                PP,
                IS,
                &(HeadTail[real_argc*2]),
                &(HeadTail[real_argc*2+1])
                );

            gcmASSERT(
                ((HeadTail[real_argc*2] == gcvNULL) && (HeadTail[real_argc*2 + 1] == gcvNULL))
                ||
                ((HeadTail[real_argc*2] != gcvNULL) && (HeadTail[real_argc*2 + 1] != gcvNULL))
                );

            ++ real_argc;
        }
        else
        {
            ppoTOKEN no_use_head = gcvNULL;
            ppoTOKEN no_use_end  = gcvNULL;

            ppoPREPROCESSOR_BufferActualArgs(
                PP,
                IS,
                &no_use_head,
                &no_use_end
                );

            ++ real_argc;
        }

        if (*IS)
        {
            (*IS)->GetToken(PP, IS, &ahead, !ppvICareWhiteSpace);
        }
        else
        {
            ppoPREPROCESSOR_Report(
                PP,
                slvREPORT_ERROR,
                "unexpected end of file when expand the macro %s.",
                id->poolString);

            return gcvSTATUS_INVALID_DATA;
        }

        if (ahead->poolString != PP->keyword->rpara &&
            ahead->poolString != PP->keyword->comma)
        {

            if(ahead->poolString == PP->keyword->eof)
            {

                ppoPREPROCESSOR_Report(
                    PP,
                    slvREPORT_ERROR,
                    " unexpected end of file when expand the macro %s.",
                    id->poolString);
            }
            else
            {
                ppoPREPROCESSOR_Report(
                    PP,
                    slvREPORT_ERROR,
                    " unexpected token when expand the macro %s.",
                    id->poolString);
            }

            gcmONERROR(
                ppoTOKEN_Destroy(PP, id)
                );

            gcmONERROR(
                ppoTOKEN_Destroy(PP, ahead)
                );

            return gcvSTATUS_INVALID_DATA;
        }
    }/*while (ahead->poolString != PP->keyword->rpara)*/

    gcmONERROR(
        ppoTOKEN_Destroy(PP, ahead)
        );
    ahead = gcvNULL;

    if(real_argc < ms->argc)
    {
        ppoPREPROCESSOR_Report(
            PP,
            slvREPORT_WARN,
            "not enough actual parameters for macro '%s'.",
            id->poolString);
    }

    if(real_argc > ms->argc)
    {

        ppoPREPROCESSOR_Report(
            PP,
            slvREPORT_ERROR,
            "too many actual parameters for macro '%s'.",
            id->poolString);
    }

    gcmTRACE(gcvLEVEL_VERBOSE, "ME : %s,finish real-arg(s)-buffering.", id->poolString);

    return gcvSTATUS_OK;
OnError:
    if (ahead != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ahead));
        ahead = gcvNULL;
    }
	return status;
}




gceSTATUS
ppoPREPROCESSOR_MacroExpand_6_ExpandHeadTail(
    ppoPREPROCESSOR     PP,
    ppoINPUT_STREAM     *IS,
    ppoTOKEN            *HeadTail,
    ppoTOKEN            *ExpandHeadTail,
    ppoTOKEN            ID,
    ppoMACRO_SYMBOL     MS
    )
{
    gctINT real_argc = 0;
    ppoMACRO_SYMBOL ms = MS;

    while (real_argc < ms->argc)
    {
        gcmASSERT(
            ((HeadTail[real_argc*2] == gcvNULL) && (HeadTail[real_argc*2 + 1] == gcvNULL))
            ||
            ((HeadTail[real_argc*2] != gcvNULL) && (HeadTail[real_argc*2 + 1] != gcvNULL))
            );

        if(HeadTail[real_argc*2] != gcvNULL)
        {
            HeadTail[real_argc*2+1]->inputStream.base.node.prev = gcvNULL;

            gcmTRACE(gcvLEVEL_VERBOSE,  "ME : expand the %dth real-arg ", real_argc);

            ppoPREPROCESSOR_ArgsMacroExpand(
                PP,
                &(HeadTail[real_argc*2]),
                &(ExpandHeadTail[real_argc*2]),
                &(ExpandHeadTail[real_argc*2+1])
                );

            gcmASSERT(HeadTail[real_argc*2] == gcvNULL);

            gcmTRACE(gcvLEVEL_VERBOSE, "ME : the %dth real-arg expanded.", real_argc);
        }
        else
        {
            gcmTRACE(gcvLEVEL_VERBOSE, "ME : no real-arg at %d",real_argc);

            gcmASSERT(
                ExpandHeadTail[real_argc*2] == gcvNULL
                &&
                ExpandHeadTail[real_argc*2+1] == gcvNULL
                );
        }

        ++real_argc;
    }

    gcmTRACE(gcvLEVEL_VERBOSE, "ME : check the HeadTail buffer");

    return gcvSTATUS_OK;
}


gceSTATUS
ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList_GetPositionOfNode(
    ppoPREPROCESSOR     PP,
    ppoTOKEN            RPNode,
    gctINT              *Position,
    ppoTOKEN            *FormalArg
    )
{
    *Position = -1;
    while(*FormalArg)
    {
        ++(*Position);

        if ( (*FormalArg)->poolString == RPNode->poolString)
            break;

        (*FormalArg) = (ppoTOKEN)(*FormalArg)->inputStream.base.node.prev;
    }

    return gcvSTATUS_OK;
}

gceSTATUS
ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList_MergePastingTokenName(
    ppoPREPROCESSOR     PP,
    ppoTOKEN            InHead,
    ppoTOKEN            InReplaceList,
    OUT sltPOOL_STRING * PoolString
    )
{
    gceSTATUS status;

    gctSIZE_T size;

    gctSIZE_T newSize = 0;

    char newString[256];

    ppoTOKEN tmprplst = InReplaceList;

    newString[0] = '\0';

    gcmONERROR(gcoOS_StrLen(InHead->poolString, &newSize));
    gcmONERROR(gcoOS_StrCatSafe(newString,256,InHead->poolString));

    while(tmprplst)
    {
        gcmONERROR(gcoOS_StrLen(tmprplst->poolString, &size));

        newSize += size;
        if(newSize + 1 > 256)
            return gcvSTATUS_TOO_COMPLEX;

        gcmONERROR(gcoOS_StrCatSafe(newString,256,tmprplst->poolString));

        tmprplst = (ppoTOKEN)tmprplst->inputStream.base.node.prev;
    }

    status = sloCOMPILER_AllocatePoolString(PP->compiler,
        newString,
        PoolString);

OnError:
    return status;
}

gceSTATUS
ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList_AddToOut(
    ppoPREPROCESSOR     PP,
    ppoTOKEN            InHead,
    ppoTOKEN            InEnd,
    ppoTOKEN            *OutHead,
    ppoTOKEN            *OutEnd
    )
{
    gcmASSERT(
        (InHead == gcvNULL && InEnd == gcvNULL)
        ||
        (InHead != gcvNULL && InEnd != gcvNULL));

    if(InHead == gcvNULL)
    {
        return gcvSTATUS_OK;
    }

    if ((*OutHead) == gcvNULL)
    {
        *OutHead = InHead;
        *OutEnd  = InEnd;

        InHead->inputStream.base.node.next = gcvNULL;
        if(InEnd)  InEnd->inputStream.base.node.prev  = gcvNULL;
    }
    else
    {
        (*OutEnd)->inputStream.base.node.prev = (void*)InHead;
        InHead->inputStream.base.node.next = (void*)(*OutEnd);
        (*OutEnd) = InEnd;
        InEnd->inputStream.base.node.prev = gcvNULL;
    }

    return gcvSTATUS_OK;
}

gceSTATUS
ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList(
    ppoPREPROCESSOR     PP,
    ppoINPUT_STREAM     *IS,
    ppoTOKEN            *Head,
    ppoTOKEN            *End,
    gctBOOL             *AnyExpanationHappened,
    ppoTOKEN            *ExpandedHeadTail,
    ppoTOKEN            ID,
    ppoMACRO_SYMBOL     MS
    )
{

    ppoTOKEN id = ID;

    ppoMACRO_SYMBOL ms = MS;

    ppoTOKEN replacement_list = gcvNULL;

    ppoTOKEN *expandedheadtail = ExpandedHeadTail;

    gceSTATUS status = gcvSTATUS_INVALID_DATA;

    gctBOOL pasting = gcvFALSE;

    gcmTRACE(gcvLEVEL_VERBOSE, "ME : begin to expand replacement-list.");

    ppoTOKEN_ColonTokenList(
        PP,
        ms->replacementList,
        __FILE__,
        __LINE__,
        "ppoPREPROCESSOR_MacroExpand : Colon the replacement list.",
        &replacement_list
        );

    while(replacement_list)
    {
        gcmTRACE(gcvLEVEL_VERBOSE, "ME : add to rp-list %s's hs.", replacement_list->poolString);

        ppoHIDE_SET_AddHS(PP, replacement_list, id->poolString);

        if (replacement_list->type == ppvTokenType_ID)
        {
            gctINT whereisarg = -1;

            ppoTOKEN search_formal_arg = ms->argv;

            status = ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList_GetPositionOfNode(
                PP,
                replacement_list,
                &whereisarg,
                &search_formal_arg);

            if(status != gcvSTATUS_OK) return status;

            gcmTRACE(gcvLEVEL_VERBOSE,
                "ME : find the position(%d) in argv to rp-list %s's hs.",
                whereisarg,
                replacement_list->poolString);

            gcmASSERT(whereisarg < ms->argc);

            if(search_formal_arg)
            {
                ppoTOKEN tmphead = gcvNULL;
                ppoTOKEN tmpend  = gcvNULL;

                if(expandedheadtail[whereisarg*2 + 1])
                {
                    (expandedheadtail[whereisarg*2 + 1])->inputStream.base.node.prev = gcvNULL;
                }

                gcmTRACE(gcvLEVEL_VERBOSE, "ME : rp-list node : %s, is a formal arg.", replacement_list->poolString);

                ppoTOKEN_ColonTokenList(
                    PP,
                    expandedheadtail[whereisarg*2],
                    __FILE__,
                    __LINE__,
                    "ppoPREPROCESSOR_MacroExpand : Creat a list of the tokenlist expanded out by the actual arguments.",
                    &tmphead
                    );

                tmpend = tmphead;

                gcmTRACE(gcvLEVEL_VERBOSE, "ME : add hs to output nodes.");

                while(tmpend)
                {
                    ppoHIDE_SET_LIST_Append(
                        PP,
                        tmpend,
                        id);

                    ppoHIDE_SET_AddHS(PP, tmpend, id->poolString);

                    if(tmpend->inputStream.base.node.prev == gcvNULL)
                    {
                        break;
                    }

                    tmpend = (ppoTOKEN)tmpend->inputStream.base.node.prev;
                }


                gcmTRACE(gcvLEVEL_VERBOSE,
                    "ME : replacementList : add the expanded node to output.",
                    replacement_list->poolString);

                if(pasting)
                {
                    status = ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList_MergePastingTokenName(
                        PP,
                        (*End),
                        tmphead,
                        &(*End)->poolString
                        );

                    pasting = gcvFALSE;
                }
                else
                {
                    status = ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList_AddToOut(
                        PP,
                        tmphead,
                        tmpend,
                        Head,
                        End
                        );
                }

                if(status != gcvSTATUS_OK) return status;


                do
                {
                    ppoTOKEN tmp = replacement_list;

                    replacement_list = (ppoTOKEN)replacement_list->inputStream.base.node.prev;

                    gcmONERROR(ppoTOKEN_Destroy(PP, tmp));
                }
                while(gcvFALSE);

                continue;

            }/*not found in argv*/
            else
            {
                gcmTRACE(
                    gcvLEVEL_VERBOSE,
                    "ME : replacementList node: %s,inputStream not a formal arg.",
                    replacement_list->poolString);
            }
        }/*if(replacementList->type == ppvTokenType_ID)*/
        else
        {
            if(pasting)
            {
                status = ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList_MergePastingTokenName(
                    PP,
                    (*End),
                    replacement_list,
                    &(*End)->poolString
                    );

                pasting = gcvFALSE;
                replacement_list = (ppoTOKEN)replacement_list->inputStream.base.node.prev;
                continue;
            }
            else if(replacement_list->poolString == PP->keyword->sharp &&
                (ppoTOKEN)replacement_list->inputStream.base.node.prev != gcvNULL)
            {
                ppoTOKEN tmprplst = replacement_list;
                tmprplst = (ppoTOKEN)tmprplst->inputStream.base.node.prev;

                /* ## */
                if(tmprplst->poolString == PP->keyword->sharp &&
                    tmprplst->inputStream.base.node.prev != gcvNULL)
                {
                    replacement_list = (ppoTOKEN)tmprplst->inputStream.base.node.prev;
                    pasting = gcvTRUE;
                    continue;
                }
            }
        }

        gcmTRACE(gcvLEVEL_VERBOSE,
            "ME : node in replacementList(%s0,not an ID or not a formal arg.",
            replacement_list->poolString);
        do
        {
            ppoTOKEN tmprplst = replacement_list;

            if( (*Head) == gcvNULL )
            {
                *Head = replacement_list;
                *End  = replacement_list;
            }
            else
            {
                (*End)->inputStream.base.node.prev = (void *)replacement_list;

                replacement_list->inputStream.base.node.next    = (void *)(*End);

                (*End) = replacement_list;
            }

            replacement_list = (ppoTOKEN)tmprplst->inputStream.base.node.prev;
        }
        while(gcvFALSE);

    }/*while(replacementList)*/

    if(*End)
    {
        (*End)->inputStream.base.node.prev = gcvNULL;
    }

    *AnyExpanationHappened = gcvTRUE;

    return gcvSTATUS_OK;

OnError:
	return status;
}

