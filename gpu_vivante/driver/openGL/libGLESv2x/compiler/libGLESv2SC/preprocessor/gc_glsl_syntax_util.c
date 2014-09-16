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

/******************************************************************************\
ToEOL
In	:	Any pos in the text.
Out	:	To the pos just pass the first NL following, or EOF.
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_ToEOL(ppoPREPROCESSOR PP)
{
	ppoTOKEN	ntoken = gcvNULL;
    gctBOOL     toTheEnd = gcvFALSE;

	gceSTATUS	status = gcvSTATUS_INVALID_ARGUMENT;

    toTheEnd = PP->toLineEnd;
    if(!PP->doWeInValidArea)
    {
        PP->toLineEnd = gcvTRUE;
        PP->skipLine = PP->currentSourceFileLineNumber;
    }

	gcmONERROR(
		PP->inputStream->GetToken(
		PP,
		&(PP->inputStream),
		&ntoken,
		!ppvICareWhiteSpace)
		);

	while(
		ntoken->poolString != PP->keyword->eof &&
		ntoken->poolString != PP->keyword->newline)
	{

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

		gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, gcvFALSE));

	}

    PP->toLineEnd = toTheEnd;

    gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

    /* Success. */
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    return status;
}
/******************************************************************************\
MatchDoubleToken
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_MatchDoubleToken(
	ppoPREPROCESSOR	PP,
	gctSTRING	NotWSStr1,
	gctSTRING	NotWSStr2,
	gctBOOL*	Match)
{
	ppoTOKEN	ntoken1 = gcvNULL;
	ppoTOKEN	ntoken2 = gcvNULL;
	gceSTATUS	status = gcvSTATUS_INVALID_ARGUMENT;

	gcmONERROR(
		ppoPREPROCESSOR_PassEmptyLine(PP)
		);

	gcmONERROR(
		PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken1, !ppvICareWhiteSpace)
		);

	gcmONERROR(
		PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken2, !ppvICareWhiteSpace)
		);

	gcmONERROR(
		ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken2)
		);

	gcmONERROR(
		ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken1)
		);

	if(	ntoken1->poolString == NotWSStr1 &&
		ntoken2->poolString == NotWSStr2)
	{
		*Match = gcvTRUE;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken2)
			);
        ntoken2 = gcvNULL;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken1)
			);

		return gcvSTATUS_OK;
	}
	else
	{
		*Match = gcvFALSE;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken2)
			);
        ntoken2 = gcvNULL;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken1)
			);

		return gcvSTATUS_OK;
	}

OnError:
	if (ntoken1 != gcvNULL)
	{
		gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken1));
		ntoken1 = gcvNULL;
	}
	if (ntoken2 != gcvNULL)
	{
		gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken2));
		ntoken2 = gcvNULL;
	}
	return status;
}


/******************************************************************************\
PassEmptyLine
In	:	Client should keep the read pos at the begin of a new line.
Out :	Server will put the pos to the next line which has some symbol
		other than NL as the first symbol in that line, or EOF.
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_PassEmptyLine(ppoPREPROCESSOR PP)
{
	ppoTOKEN	ntoken = gcvNULL;
	gceSTATUS	status = gcvSTATUS_INVALID_ARGUMENT;
	gcmASSERT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);


	gcmONERROR(
		PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
		);

	while(ntoken->type != ppvTokenType_EOF && ntoken->poolString == PP->keyword->newline)
	{
		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
        ntoken = gcvNULL;

		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, gcvFALSE)
			);
	}

	gcmONERROR(
		ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken)
		);

	gcmONERROR(
		ppoTOKEN_Destroy(PP, ntoken)
		);

	return gcvSTATUS_OK;

OnError:
    if (ntoken != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
        ntoken = gcvNULL;
    }
	return status;


}





/******************************************************************************\
Define Buffer RList
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_Define_BufferReplacementList(ppoPREPROCESSOR PP, ppoTOKEN* RList)
{
	ppoTOKEN	ntoken	= gcvNULL;
	ppoTOKEN	colon	= gcvNULL;
	ppoTOKEN	lastone	= gcvNULL;
	gceSTATUS	status	= gcvSTATUS_INVALID_ARGUMENT;

    gcmHEADER_ARG("PP=0x%x RList=0x%x", PP, RList);

	gcmASSERT(RList);

	*RList = gcvNULL;

	status = PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace);			ppmCheckOK();

	while(
		ntoken->poolString != PP->keyword->eof		&&
		ntoken->poolString != PP->keyword->newline)
	{
		colon = ntoken;

		/*Add this formal para to the para stack.*/
		if(*RList)
		{
			lastone->inputStream.base.node.prev	= (void*)colon;
			colon->inputStream.base.node.next		= (void*)lastone;
			colon->inputStream.base.node.prev		= gcvNULL;
			lastone = colon;
		}
		else
		{
			*RList = colon;
			lastone = colon;
		}
		/*Get a token, check.*/
		gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace));

	}/*while*/

	gcmONERROR(
		ppoINPUT_STREAM_UnGetToken(PP, &(PP->inputStream), ntoken)
		);

	gcmONERROR(
		ppoTOKEN_Destroy(PP, ntoken)
		);

    gcmFOOTER_NO();
	return gcvSTATUS_OK;

OnError:
	if (ntoken != gcvNULL)
	{
		ppoTOKEN_Destroy(PP, ntoken);
		ntoken = gcvNULL;
	}

    gcmFOOTER();
	return status;

}

/******************************************************************************\
Define BufferArgs
In	:	This function inputStream called just after the name of #define
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_Define_BufferArgs(ppoPREPROCESSOR PP, ppoTOKEN* args, gctINT* argc)
{
	ppoTOKEN ntoken = gcvNULL;
	ppoTOKEN colon	= gcvNULL;
	ppoTOKEN lastone	= gcvNULL;
	gctBOOL	 boolean= gcvFALSE;
	gceSTATUS	status = gcvSTATUS_INVALID_ARGUMENT;

    gcmHEADER_ARG("PP=0x%x args=0x%x argc=0x%x", PP, args, argc);

	gcmASSERT( (*args) == gcvNULL && (*argc) == 0 );

	while(1)
	{
		status = PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace);

		if(status != gcvSTATUS_OK)
        {
            gcmFOOTER();
            return status;
        }
		/*Get One Para*/
		if(ntoken->type != ppvTokenType_ID)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR, "Id is expected.");
			gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
		}
		else
		{
			/*Check if this para inputStream already in the list*/
			ppoTOKEN_STREAM_FindID(PP, *args,ntoken->poolString, &boolean);
			if(boolean)
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
					"The formal para name should not be the same.%s.",ntoken->poolString);
				gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
			}

			colon = ntoken;

			/*Add this formal para to the para stack.*/
			if(*args)
			{
				if (lastone == gcvNULL)
                    gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);

                lastone->inputStream.base.node.prev = (void*)colon;
				colon->inputStream.base.node.prev = gcvNULL;
				colon->inputStream.base.node.next = (void*)lastone;
				lastone = colon;
				++(*argc);
			}
			else{
				*args = colon;
				lastone = *args;
				++(*argc);
			}
		}

		gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace));

		if(ntoken->poolString != PP->keyword->comma)
		{
			if(ntoken->poolString != PP->keyword->rpara)
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
					"Need a ) here.");
				gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
			}
			else
			{
                status = ppoTOKEN_Destroy(PP, ntoken);
                if (gcmIS_SUCCESS(status))
                {
                    gcmFOOTER_NO();
                    return gcvSTATUS_OK;
                }
                else
                {
                    gcmFOOTER();
                    return status;
                }
			}
		}

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
        ntoken = gcvNULL;

	}/*while(1)*/

OnError:
	if (ntoken != gcvNULL)
	{
		gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
		ntoken = gcvNULL;
	}
    gcmFOOTER();
	return status;
}
/******************************************************************************\
Buffer Actual Args
When meet a macro call in texline, we call this function to collect
the actual arguments of macro call.
(___,___,___)
__is what this function parse.
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_BufferActualArgs(ppoPREPROCESSOR PP, ppoINPUT_STREAM *IS, ppoTOKEN* Head, ppoTOKEN* End)
{
	ppoTOKEN			ntoken		= gcvNULL;
	gctINT				locallevel	= 0;
	gceSTATUS			status		= gcvSTATUS_INVALID_DATA;

    gcmHEADER_ARG("PP=0x%x IS=0x%x Head=0x%x End=0x%x", PP, IS, Head, End);

	*Head	= gcvNULL;
	*End	= gcvNULL;

	gcmASSERT(IS);

	if((*IS) == gcvNULL)
	{
		ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR, "unexpected end of file.");
        status = gcvSTATUS_INVALID_DATA;
        gcmFOOTER();
		return status;
	}

	status = (*IS)->GetToken(PP, IS, &ntoken, !ppvICareWhiteSpace);
    if(status != gcvSTATUS_OK)
    {
        gcmFOOTER();
        return status;
    }

	while(1)
	{
		if(ntoken->poolString == PP->keyword->eof)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
				"unexpected end of file.");

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);

            status = gcvSTATUS_INVALID_DATA;
            gcmFOOTER();
			return status;
		}
		else	if(ntoken->poolString == PP->keyword->rpara && locallevel == 0)
		{
			if(*IS)
			{
				gcmONERROR(
					ppoINPUT_STREAM_UnGetToken(PP, IS, ntoken)
					);

                status = ppoTOKEN_Destroy(PP, ntoken);
                if (gcmIS_SUCCESS(status))
                {
                    gcmFOOTER_NO();
                    return gcvSTATUS_OK;
                }
                else
                {
                    gcmFOOTER();
                    return status;
                }
			}
			else
			{
				ntoken->inputStream.base.node.prev = gcvNULL;

				ntoken->inputStream.base.node.next = gcvNULL;

				(*IS)  = (ppoINPUT_STREAM)ntoken;

                gcmFOOTER_NO();
				return gcvSTATUS_OK;
			}
		}
		else if(ntoken->poolString == PP->keyword->comma && locallevel == 0)
		{
			if(*IS)
			{
				gcmONERROR(
					ppoINPUT_STREAM_UnGetToken(PP, IS, ntoken)
					);

				gcmONERROR(
					ppoTOKEN_Destroy(PP, ntoken)
					);
                ntoken = gcvNULL;
			}
			else
			{
				ntoken->inputStream.base.node.prev = gcvNULL;

				ntoken->inputStream.base.node.next = gcvNULL;

				(*IS)  = (ppoINPUT_STREAM)ntoken;
			}

            gcmFOOTER_NO();
			return gcvSTATUS_OK;

		}
		else if(ntoken->poolString == PP->keyword->lpara)
		{
			++locallevel;
		}
		else if(ntoken->poolString == PP->keyword->rpara)
		{
			--locallevel;
		}

		if((*Head) == gcvNULL)
		{
			*End = *Head = ntoken;
		}
		else
		{
			(*End)->inputStream.base.node.prev = (void*)ntoken;

			ntoken->inputStream.base.node.prev = gcvNULL;

			ntoken->inputStream.base.node.next = (void*)(*End);

			*End = ntoken;
		}

		if(*IS)
		{
			status = (*IS)->GetToken(PP, IS, &ntoken, !ppvICareWhiteSpace);			ppmCheckOK();
		}
		else
		{
			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);

			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR, "unexpected end of file.");

            status = gcvSTATUS_INVALID_DATA;
            gcmFOOTER();
			return status;
		}
	}/*while(1)*/
OnError:
    if (ntoken != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
        ntoken = gcvNULL;
    }
    gcmFOOTER();
	return status;
}

