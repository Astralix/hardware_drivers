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

typedef struct _slsEXTENSION_INFO
{
   gctSTRING str;
   sleEXTENSION  flag;
   gctBOOL   enable;
   gctBOOL   disable;
   gctBOOL   warn;
   gctBOOL   require;
}
slsEXTENSION_INFO;

static slsEXTENSION_INFO _DefinedExtensions[] =
{
   {"all",   slvEXTENSION_ALL, gcvFALSE, gcvTRUE, gcvTRUE, gcvFALSE},
   {"GL_OES_standard_derivatives", slvEXTENSION_STANDARD_DERIVATIVES, gcvTRUE, gcvTRUE, gcvFALSE, gcvFALSE},
   {"GL_EXT_texture_array", slvEXTENSION_TEXTURE_ARRAY, gcvTRUE, gcvTRUE, gcvFALSE, gcvFALSE},
   {"GL_OES_texture_3D", slvEXTENSION_TEXTURE_3D, gcvTRUE, gcvTRUE, gcvFALSE, gcvFALSE},
   {"GL_EXT_frag_depth", slvEXTENSION_FRAG_DEPTH, gcvTRUE, gcvTRUE, gcvFALSE, gcvFALSE},
   {"GL_OES_EGL_image_external", slvEXTENSION_EGL_IMAGE_EXTERNAL, gcvTRUE, gcvTRUE, gcvFALSE, gcvTRUE}
};

#define _sldDefinedExtensionsCount  (sizeof(_DefinedExtensions) / sizeof(slsEXTENSION_INFO))


/*******************************************************************************
**
**	ppoPREPROCESSOR_PreprocessingFile
**
*/
gceSTATUS
ppoPREPROCESSOR_PreprocessingFile(ppoPREPROCESSOR PP)
{
	ppoTOKEN	ntoken		= gcvNULL;

	ppoTOKEN	ntoken2		= gcvNULL;

	gceSTATUS	status		= gcvSTATUS_INVALID_ARGUMENT;

	gcmASSERT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);

	do{
		gcmONERROR(
			ppoPREPROCESSOR_PassEmptyLine(PP)
			);

		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
			);

		if (ntoken->type == ppvTokenType_EOF)
		{
			/*This is the end condition.*/
			gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

			return status;
		}
		else if (ntoken->poolString == PP->keyword->sharp)
		{
			/*#*/
			gcmONERROR(
				PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken2, !ppvICareWhiteSpace)
				);

			gcmONERROR(
				ppoINPUT_STREAM_UnGetToken(PP, &(PP->inputStream), ntoken2)
				);

			gcmONERROR(
				ppoINPUT_STREAM_UnGetToken(PP, &(PP->inputStream), ntoken)
				);

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)/*No more usage in this path.*/
				);
            ntoken = gcvNULL;


			if( ntoken2->poolString == PP->keyword->eof			||
				ntoken2->poolString == PP->keyword->newline		||
				ntoken2->poolString == PP->keyword->if_			||
				ntoken2->poolString == PP->keyword->ifdef		||
				ntoken2->poolString == PP->keyword->ifndef		||
				ntoken2->poolString == PP->keyword->pragma		||
				ntoken2->poolString == PP->keyword->error		||
				ntoken2->poolString == PP->keyword->line		||
				ntoken2->poolString == PP->keyword->version		||
				ntoken2->poolString == PP->keyword->extension		||
				ntoken2->poolString == PP->keyword->define		||
				ntoken2->poolString == PP->keyword->undef)
			{
				gcmONERROR(
					ppoTOKEN_Destroy(PP, ntoken2)
					);
                ntoken2 = gcvNULL;

				status = ppoPREPROCESSOR_Group(PP);

				if(status != gcvSTATUS_OK) return status;
			}
			else
			{
				/*Other legal directive or inlegal directive*/

				ppoPREPROCESSOR_Report(PP,
					slvREPORT_ERROR,
					"Not expected symbol here \"%s\"",
					ntoken2->poolString
					);

				gcmONERROR(
					ppoTOKEN_Destroy(PP, ntoken2)
					);
                ntoken2 = gcvNULL;

				return gcvSTATUS_INVALID_ARGUMENT;
			}
		}
		else
		{
			/*Text Line*/

			PP->otherStatementHasAlreadyAppeared = gcvTRUE;

			gcmONERROR(
				ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken)
				);

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);
            ntoken = gcvNULL;

			gcmONERROR(
				ppoPREPROCESSOR_Group(PP)
				);
		}
	}
	while(gcvTRUE);/*group by group.*/

OnError:
    if (ntoken != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
        ntoken = gcvNULL;
    }

    if (ntoken2 != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken2));
        ntoken2 = gcvNULL;
    }

    return status;
}

/*******************************************************************************
**
**	ppoPREPROCESSOR_Group
**
*/
gceSTATUS
ppoPREPROCESSOR_Group(ppoPREPROCESSOR PP)
{

	ppoTOKEN	ntoken		= gcvNULL;

	ppoTOKEN	ntoken2		= gcvNULL;

	gceSTATUS	status		= gcvSTATUS_INVALID_ARGUMENT;

    gcmHEADER_ARG("PP=0x%x", PP);

	gcmASSERT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);

	do
	{

		gcmONERROR(
			ppoPREPROCESSOR_PassEmptyLine(PP)
			);

		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
			);

		if(ntoken->type == ppvTokenType_EOF)
		{
			gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
            gcmFOOTER();
            return status;

		}

		if( ntoken->poolString	== PP->keyword->sharp
		&&	ntoken->hideSet		== gcvNULL)/*avoid # generated by macro*/
		{
			/*#*/

			gcmONERROR(
				PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken2, gcvFALSE)
				);

			gcmASSERT(ntoken2->hideSet == gcvNULL);

			gcmONERROR(
				ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken2)
				);

			gcmONERROR(
				ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken)
				);

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);
            ntoken = gcvNULL;

			if( ntoken2->poolString == PP->keyword->eof			||
				ntoken2->poolString == PP->keyword->newline		||
				ntoken2->poolString == PP->keyword->if_			||
				ntoken2->poolString == PP->keyword->ifdef		||
				ntoken2->poolString == PP->keyword->ifndef		||
				ntoken2->poolString == PP->keyword->pragma		||
				ntoken2->poolString == PP->keyword->error		||
				ntoken2->poolString == PP->keyword->line		||
				ntoken2->poolString == PP->keyword->version		||
				ntoken2->poolString == PP->keyword->extension	||
				ntoken2->poolString == PP->keyword->define		||
				ntoken2->poolString == PP->keyword->undef)
			{
				gcmONERROR(
					ppoTOKEN_Destroy(PP, ntoken2)
					);
                ntoken2 = gcvNULL;

				gcmONERROR(
					ppoPREPROCESSOR_GroupPart(PP)
					);
			}
            else if(ntoken2->poolString == PP->keyword->else_ ||
                ntoken2->poolString == PP->keyword->elif ||
                ntoken2->poolString == PP->keyword->endif)
			{
				/*this # should be part of another group.*/
				gcmONERROR(ppoTOKEN_Destroy(PP, ntoken2));

                gcmFOOTER();
				return status;

			}
            else if(!PP->doWeInValidArea)
            {
                gcmONERROR(ppoTOKEN_Destroy(PP, ntoken2));

                ntoken2 = gcvNULL;

                gcmONERROR(
                    ppoPREPROCESSOR_GroupPart(PP)
                    );

            }
            else
            {
                gcmONERROR(ppoTOKEN_Destroy(PP, ntoken2));

                gcmFOOTER();
                return status;
            }
		}
		else
		{
			/*Text Line*/

			gcmONERROR(
				ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken)
				);

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);
            ntoken = gcvNULL;

			gcmONERROR(
				ppoPREPROCESSOR_GroupPart(PP)
				);

		}
	}
	while(gcvTRUE);


OnError:
    if (ntoken != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
        ntoken = gcvNULL;
    }

    if (ntoken2 != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken2));
        ntoken2 = gcvNULL;
    }

    gcmFOOTER();
    return status;
}
/******************************************************************************\
**
**	ppoPREPROCESSOR_GroupPart
**
*/
gceSTATUS
ppoPREPROCESSOR_GroupPart(ppoPREPROCESSOR PP)
{
	ppoTOKEN	ntoken	= gcvNULL;

	ppoTOKEN	ntoken2 = gcvNULL;

	gceSTATUS	status	= gcvSTATUS_INVALID_ARGUMENT;

    gcmHEADER_ARG("PP=0x%x", PP);

	gcmASSERT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);

	status = ppoPREPROCESSOR_PassEmptyLine(PP); ppmCheckOK();

	status = PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, gcvFALSE);

	ppmCheckOK();

	if(ntoken->type == ppvTokenType_EOF)
	{
		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
        gcmFOOTER();
		return status;
	}

	if (ntoken->poolString == PP->keyword->sharp
	&&	ntoken->hideSet == gcvNULL)
	{
		/*#*/

		gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken2, !ppvICareWhiteSpace));

		gcmASSERT(ntoken2->hideSet == gcvNULL);

		/*hideSet should be gcvNULL*/

		gcmONERROR(ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken2));

		/*# EOF or NL*/

		if (ntoken2->type == ppvTokenType_EOF
		||	ntoken2->poolString == PP->keyword->newline)
		{
    		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
            ntoken = gcvNULL;
			gcmONERROR(ppoTOKEN_Destroy(PP, ntoken2));
            ntoken2 = gcvNULL;
            gcmFOOTER();
			return status;
		}

		/*# if/ifdef/ifndef*/

		if (ntoken2->poolString == PP->keyword->if_
		||	ntoken2->poolString == PP->keyword->ifdef
		||	ntoken2->poolString == PP->keyword->ifndef)
		{
      		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
            ntoken = gcvNULL;

			PP->otherStatementHasAlreadyAppeared = gcvTRUE;

			gcmONERROR(ppoTOKEN_Destroy(PP, ntoken2));
            gcmFOOTER();
			return ppoPREPROCESSOR_IfSection(PP);
		}

		/*	#pragma\error\line\version\extension\define\undef */

		if (ntoken2->poolString == PP->keyword->pragma
		||	ntoken2->poolString == PP->keyword->error
		||	ntoken2->poolString == PP->keyword->line
		||	ntoken2->poolString == PP->keyword->version
		||	ntoken2->poolString == PP->keyword->extension
		||	ntoken2->poolString == PP->keyword->define
		||	ntoken2->poolString == PP->keyword->undef)
		{
			if (ntoken2->poolString == PP->keyword->version)
			{
				if (gcvTRUE == PP->versionStatementHasAlreadyAppeared)
				{
					ppoPREPROCESSOR_Report(
						PP,
						slvREPORT_ERROR,
						"The version statement should appear only once.");

					gcmONERROR(gcvSTATUS_INVALID_DATA);
				}
				if (gcvTRUE == PP->otherStatementHasAlreadyAppeared)
				{
					ppoPREPROCESSOR_Report(
						PP,
						slvREPORT_ERROR,
						"The version statement should appear "
						"before any other statement except space and comment.");

					gcmONERROR(gcvSTATUS_INVALID_DATA);
				}
				PP->versionStatementHasAlreadyAppeared = gcvTRUE;
			}
			else
			{
				PP->otherStatementHasAlreadyAppeared = gcvTRUE;
			}
    		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
            ntoken = gcvNULL;
			gcmONERROR(ppoTOKEN_Destroy(PP, ntoken2));
            status = ppoPREPROCESSOR_ControlLine(PP);
            gcmFOOTER();
			return status;
		}
        else if( ntoken2->poolString == PP->keyword->else_ ||
            ntoken2->poolString == PP->keyword->elif ||
            ntoken2->poolString == PP->keyword->endif)
		{
			/*# something else,endif, else.*/
			status = ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken2); ppmCheckOK();
			status = ppoTOKEN_Destroy(PP, ntoken2); ppmCheckOK();
			status = ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken); ppmCheckOK();
			status = ppoTOKEN_Destroy(PP, ntoken); ppmCheckOK();
            gcmFOOTER();
			return 	status;
		}
        else
        {
			status = ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken2);
            ppmCheckOK();
			status = ppoTOKEN_Destroy(PP, ntoken2);
            ntoken2 = gcvNULL;
            ppmCheckOK();
			status = ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken);
            ppmCheckOK();
			status = ppoTOKEN_Destroy(PP, ntoken);
            ppmCheckOK();
            status = ppoPREPROCESSOR_TextLine(PP);
            gcmFOOTER();
            return status;
        }
	}
	else
	{
		/*Text Line*/
		gcmONERROR(ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, ntoken));
		status = ppoTOKEN_Destroy(PP, ntoken); ppmCheckOK();
        status = ppoPREPROCESSOR_TextLine(PP);
        gcmFOOTER();
		return status;
	}

OnError:
    if (ntoken != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
        ntoken = gcvNULL;
    }

    if (ntoken2 != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken2));
        ntoken2 = gcvNULL;
    }

    gcmFOOTER();
    return status;
}
/*******************************************************************************
**
**	ppoPREPROCESSOR_IfSection
**
**
*/
gceSTATUS
ppoPREPROCESSOR_IfSection(ppoPREPROCESSOR PP)
{
	ppoTOKEN		ntoken			= gcvNULL;
	ppoTOKEN        newt            = gcvNULL;
	gctINT			evalresult		= 0;

	gctBOOL			legalfounded	= 0;

	gctBOOL			dmatch			= gcvFALSE;

	gctBOOL			pplegal_backup	= gcvFALSE;

	gceSTATUS		status			= gcvSTATUS_INVALID_ARGUMENT;

	gcmASSERT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);

	/*store PP's doWeInValidArea env vars*/

	gcmONERROR(
		PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, gcvFALSE)
		);

	gcmASSERT(
		ntoken->poolString == PP->keyword->if_		||
		ntoken->poolString == PP->keyword->ifdef	||
		ntoken->poolString == PP->keyword->ifndef);

	if( ntoken->poolString == PP->keyword->ifdef )
	{

		gcmONERROR(
			ppoTOKEN_Construct(PP, __FILE__, __LINE__, "Creat for ifdef.", &newt)
			);

		newt->hideSet		= gcvNULL;

		newt->poolString	= PP->keyword->defined;

		newt->type			= ppvTokenType_ID;

		gcmONERROR(
			ppoINPUT_STREAM_UnGetToken(PP, &(PP->inputStream), newt)
			);

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
		ntoken = gcvNULL;
		gcmONERROR(
			ppoTOKEN_Destroy(PP, newt)
			);
		newt = gcvNULL;

	}/*if( ntoken->poolString == PP->keyword->ifdef )*/
	else if(ntoken->poolString == PP->keyword->ifndef)
	{

		/*push defined back.*/

		gcmONERROR(
			ppoTOKEN_Construct(
			PP,
			__FILE__,
			__LINE__,
			"Creat for ifndef, defined.",
			&newt
			)
			);

		newt->hideSet = gcvNULL;

		newt->poolString = PP->keyword->defined;

		newt->type = ppvTokenType_ID;

		gcmONERROR(
			ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, newt)
			);

		gcmONERROR(
			ppoTOKEN_Destroy(PP, newt)
			);
		newt = gcvNULL;

		/*push ! back.*/

		gcmONERROR(
			ppoTOKEN_Construct(
			PP,
			__FILE__,
			__LINE__,
			"Creat for ifndef,!.",
			&newt
			)
			);

		newt->hideSet = gcvNULL;

		newt->poolString = PP->keyword->lanti;

		newt->type = ppvTokenType_PUNC;

		gcmONERROR(
			ppoINPUT_STREAM_UnGetToken(PP, &PP->inputStream, newt)
			);

		gcmONERROR(
			ppoTOKEN_Destroy(PP, newt)
			);
		newt = gcvNULL;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
		ntoken = gcvNULL;

	}
	else
	{
		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
		ntoken = gcvNULL;
	}
	/*else if(ntoken->poolString == PP->keyword->ifndef)*/

	pplegal_backup = PP->doWeInValidArea;

	if(PP->doWeInValidArea)
	{

		gcmONERROR(
			ppoPREPROCESSOR_Eval(PP, PP->keyword->newline, 0, &evalresult)
			);

		/*set enviroment variable doWeInValidArea.*/

		PP->doWeInValidArea = (PP->doWeInValidArea) && (!!evalresult);

		legalfounded =legalfounded || (gctBOOL)evalresult;

	}
	else
	{
		PP->doWeInValidArea = PP->doWeInValidArea;

		legalfounded = legalfounded;
	}

	gcmONERROR(
		ppoPREPROCESSOR_Group(PP)
		);

	/*set enviroment variable doWeInValidArea.*/

	PP->doWeInValidArea = pplegal_backup;

	while(1)
	{
		gcmONERROR(
			ppoPREPROCESSOR_PassEmptyLine(PP)
			);

		/*#else*/

		gcmONERROR(
			ppoPREPROCESSOR_MatchDoubleToken(
			PP,
			PP->keyword->sharp,
			PP->keyword->else_,
			&dmatch
			)
			);

		if(dmatch)
		{
			break;
		}

		/*#endif*/
		gcmONERROR(
			ppoPREPROCESSOR_MatchDoubleToken(
			PP,
			PP->keyword->sharp,
			PP->keyword->endif,
			&dmatch
			)
			);

		if(dmatch)
		{
			break;
		}

		/*#elif*/
		gcmONERROR(
			ppoPREPROCESSOR_MatchDoubleToken(
			PP,
			PP->keyword->sharp,
			PP->keyword->elif,
			&dmatch
			)
			);

		if(dmatch)
		{
			/*eval*/

			gcmONERROR(
				PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, gcvFALSE)
				);

			if(ntoken->poolString != PP->keyword->sharp)
			{

				gcmONERROR(
					ppoTOKEN_Destroy(PP, ntoken)
					);
				ntoken = gcvNULL;

				ppoPREPROCESSOR_Report(PP,
					slvREPORT_INTERNAL_ERROR,
					"This symbol should be #.");
			}

			if (ntoken != gcvNULL)
			{
				gcmONERROR(
					ppoTOKEN_Destroy(PP, ntoken)
					);
				ntoken = gcvNULL;
			}

			gcmONERROR(
				PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, gcvFALSE)
				);

			if(ntoken->poolString != PP->keyword->elif)
			{
				gcmONERROR(
					ppoTOKEN_Destroy(PP, ntoken)
					);
				ntoken = gcvNULL;

				ppoPREPROCESSOR_Report(PP,
					slvREPORT_INTERNAL_ERROR,
					"This symbol should be elif.");
			}
			else
			{
				if (ntoken != gcvNULL)
				{
					gcmONERROR(
						ppoTOKEN_Destroy(PP, ntoken)
						);
					ntoken = gcvNULL;
				}
			}

			pplegal_backup = PP->doWeInValidArea;

			if( PP->doWeInValidArea && !legalfounded)
			{

				gcmONERROR(
					ppoPREPROCESSOR_Eval(
					PP,
					PP->keyword->newline,
					0,
					&evalresult
					)
					);

				PP->doWeInValidArea = PP->doWeInValidArea && (!legalfounded) && (!!evalresult);

				legalfounded =legalfounded || (gctBOOL)evalresult;

			}
			else
			{
				PP->doWeInValidArea = PP->doWeInValidArea && (!legalfounded);

				legalfounded = legalfounded;
			}
			/*do not care the result of the evaluation*/

			gcmONERROR(
				ppoPREPROCESSOR_Group(PP)
				);

			/*backroll doWeInValidArea env*/

			PP->doWeInValidArea = pplegal_backup;
		}
		else
		{
			ppoPREPROCESSOR_Report(PP,
				slvREPORT_ERROR,
				"Expect #else, #endif, #elif.");

			return gcvSTATUS_INVALID_ARGUMENT;
		}
	}/*while(1)*/

	/*match #else or #endif*/

	gcmONERROR(
		ppoPREPROCESSOR_MatchDoubleToken(
		PP,
		PP->keyword->sharp,
		PP->keyword->else_,
		&dmatch)
		);

	if(dmatch)
	{
		/*match #else*/

		gcmONERROR(
			ppoPREPROCESSOR_ToEOL(PP)
			);

		/*set doWeInValidArea backup*/

		pplegal_backup = PP->doWeInValidArea;

		PP->doWeInValidArea = PP->doWeInValidArea && (!legalfounded);

		gcmONERROR(
			ppoPREPROCESSOR_Group(PP)
			);

		/*backroll doWeInValidArea env*/

		PP->doWeInValidArea = pplegal_backup;
	}

	/*match #endif*/

	gcmONERROR(
		ppoPREPROCESSOR_PassEmptyLine(PP)
		);

	gcmONERROR(
		ppoPREPROCESSOR_MatchDoubleToken(
		PP,
		PP->keyword->sharp,
		PP->keyword->endif,
		&dmatch)
		);

	if(!dmatch)
	{
		ppoPREPROCESSOR_Report(PP,
			slvREPORT_INTERNAL_ERROR,
			"Expect #endif.");

		return gcvSTATUS_INVALID_ARGUMENT;
	}

	return ppoPREPROCESSOR_ToEOL(PP);

OnError:
	if (ntoken != gcvNULL)
	{
		gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
		ntoken = gcvNULL;
	}
	if (newt != gcvNULL)
	{
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, newt));
		newt = gcvNULL;
	}
	return status;
}

/******************************************************************************\
Defined
Parse out the id in or not in ().
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_Defined(ppoPREPROCESSOR PP, gctSTRING* Return)
{
	ppoTOKEN	ntoken		= gcvNULL;

	gceSTATUS	status		= gcvSTATUS_INVALID_ARGUMENT;

	gcmASSERT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);

	gcmONERROR(
		PP->inputStream->GetToken(
		PP,
		&(PP->inputStream),
		&ntoken,
		!ppvICareWhiteSpace)
		);

	if(ntoken->poolString == PP->keyword->lpara)
	{

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
        ntoken = gcvNULL;

		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
			);

		if(ntoken->type != ppvTokenType_ID){

			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
				"Expect and id after the defined(.");

			gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

			return gcvSTATUS_INVALID_ARGUMENT;

		}

		*Return = ntoken->poolString;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
        ntoken = gcvNULL;

		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
			);

		if(ntoken->poolString != PP->keyword->rpara) {

			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
				"Expect a ) after defined(id .");

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);

			return gcvSTATUS_INVALID_ARGUMENT;

		}

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

	}
	else
	{

		if(ntoken->type != ppvTokenType_ID){

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);

			return gcvSTATUS_INVALID_ARGUMENT;

		}

		*Return = ntoken->poolString;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

	}

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
Args Macro Expand
In this function, we treat the HeadIn as a Input Stream, we inputStream doWeInValidArea to do
Macro Expanation use current Macro Context.
And the expanded token stream inputStream store in HeadOut and EndOut.
If the HeadIn inputStream gcvNULL, then then HeadOut and EndOut will be gcvNULL, too.
The Outs counld be gcvNULL, when the expanation inputStream just NOTHING.

WARINIG!!!
Every node in the HeadIn should not be released outside.
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_ArgsMacroExpand_AddTokenToOut(
								ppoPREPROCESSOR PP,
								ppoTOKEN	InHead,
								ppoTOKEN	InEnd,
								ppoTOKEN	*OutHead,
								ppoTOKEN	*OutEnd)
{
	if(*OutHead == gcvNULL)
	{
		gcmASSERT(*OutEnd == gcvNULL);

		*OutHead = InHead;
		*OutEnd = InEnd;

		InHead->inputStream.base.node.next = gcvNULL;
		InEnd->inputStream.base.node.prev = gcvNULL;
	}
	else
	{
		gcmASSERT(*OutEnd != gcvNULL);

		(*OutEnd)->inputStream.base.node.prev = (void*)InHead;

		InHead->inputStream.base.node.next = (void*)(*OutEnd);
		InEnd->inputStream.base.node.prev = gcvNULL;

		(*OutEnd) = InEnd;
	}

	return gcvSTATUS_OK;
}

gceSTATUS
ppoPREPROCESSOR_ArgsMacroExpand_LinkBackToIS(
								ppoPREPROCESSOR PP,
								ppoTOKEN	*IS,
								ppoTOKEN	*InHead,
								ppoTOKEN	*InEnd)
{
	if((*IS) == gcvNULL)
	{
		*IS = *InHead;

		(*InEnd)->inputStream.base.node.prev = gcvNULL;
	}
	else
	{
		(*IS)->inputStream.base.node.next = (void*)(*InEnd);
		(*InEnd)->inputStream.base.node.prev = (void*)(*IS);
		(*InHead)->inputStream.base.node.next = gcvNULL;
		(*IS) = (*InHead);
	}

	return gcvSTATUS_OK;
}
gceSTATUS
ppoPREPROCESSOR_ArgsMacroExpand(
								ppoPREPROCESSOR PP,
								ppoTOKEN	*IS,
								ppoTOKEN	*OutHead,
								ppoTOKEN	*OutEnd)
{
	gceSTATUS status = gcvSTATUS_INVALID_DATA;

	*OutHead = gcvNULL;
	*OutEnd  = gcvNULL;

	if( *IS == gcvNULL )	return gcvSTATUS_OK;

	while((*IS))
	{
		/*
		more input-token, more parsing-action.
		which is diff with text line.
		*/

		if ((*IS)->type == ppvTokenType_ID)
		{
			/*
			do macro expand
			*/
			gctBOOL any_expanation_happened = gcvFALSE;

			ppoTOKEN expanded_id_head = gcvNULL;
			ppoTOKEN expanded_id_end = gcvNULL;

			status = ppoPREPROCESSOR_MacroExpand(
					PP,
					(ppoINPUT_STREAM*)IS,
					&expanded_id_head,
					&expanded_id_end,
					&any_expanation_happened);
			if(status != gcvSTATUS_OK) return status;

			gcmASSERT(
				(expanded_id_head == gcvNULL && expanded_id_end == gcvNULL)
				||
				(expanded_id_head != gcvNULL && expanded_id_end != gcvNULL));

			if (any_expanation_happened == gcvTRUE)
			{
				if (expanded_id_head != gcvNULL)
				{
					/*link expanded_id back to IS*/
					status = ppoPREPROCESSOR_ArgsMacroExpand_LinkBackToIS(
						PP,
						IS,
						&expanded_id_head,
						&expanded_id_end);
					if(status != gcvSTATUS_OK) return status;
				}
				/*else, the id expand to nothing.*/
			}
			else
			{
				if(expanded_id_head != gcvNULL)
				{
					/*
					add to *Out*
					*/
					status = ppoPREPROCESSOR_ArgsMacroExpand_AddTokenToOut(
								PP,
								expanded_id_head,
								expanded_id_end,
								OutHead,
								OutEnd);
					if(status != gcvSTATUS_OK) return status;
				}
				else
				{
					gcmASSERT(0);
				}
			}
		}
		else
		{
			/*
			not id, just add to *Out*
			*/
			ppoTOKEN ntoken = (*IS);

			*IS = (ppoTOKEN)((*IS)->inputStream.base.node.prev);

			status = ppoPREPROCESSOR_ArgsMacroExpand_AddTokenToOut(
				PP,
				ntoken,
				ntoken,
				OutHead,
				OutEnd);
			if(status != gcvSTATUS_OK) return status;
		}
	}/*while((*IS))*/

	return gcvSTATUS_OK;
}

gceSTATUS
ppoPREPROCESSOR_TextLine_Handle_FILE_LINE_VERSION(
	ppoPREPROCESSOR PP,
	gctSTRING What
	)
{
	ppoTOKEN newtoken = gcvNULL;

	gceSTATUS status = gcvSTATUS_INVALID_DATA;

	char* creat_str = gcvNULL;

	char	numberbuffer [128];

	gctUINT	offset = 0;

	gcoOS_MemFill(numberbuffer, 0, 128);

	if (What == PP->keyword->_file_)
	{

		creat_str = "ppoPREPROCESSOR_TextLine : Creat a new token to substitute __FILE__";
		gcoOS_PrintStrSafe(numberbuffer, gcmSIZEOF(numberbuffer), &offset, "%d\0", PP->currentSourceFileStringNumber);

	}
	else if (What == PP->keyword->_line_)
	{
		creat_str = "ppoPREPROCESSOR_TextLine : Creat a new token to substitute __LINE__";
		gcoOS_PrintStrSafe(numberbuffer, gcmSIZEOF(numberbuffer), &offset, "%d\0", PP->currentSourceFileLineNumber);


	}
	else if (What == PP->keyword->_version_)
	{
		creat_str = "ppoPREPROCESSOR_TextLine : Creat a new token to substitute __VERSION__";
		gcoOS_PrintStrSafe(numberbuffer, gcmSIZEOF(numberbuffer), &offset, "%d\0", PP->version);
	}
	else if(What == PP->keyword->gl_es)
	{
		creat_str = "ppoPREPROCESSOR_TextLine : Creat a new token to substitute GL_ES";
		gcoOS_PrintStrSafe(numberbuffer, gcmSIZEOF(numberbuffer), &offset, "%d\0", 1);
	}
	else
	{
		gcmASSERT(0);
	}

	gcmONERROR(
		ppoTOKEN_Construct(
		PP,
		__FILE__,
		__LINE__,
		creat_str,
		&newtoken));

	gcmONERROR(
		sloCOMPILER_AllocatePoolString(
		PP->compiler,
		numberbuffer,
		&(newtoken->poolString))
		);

	newtoken->hideSet	= gcvNULL;

	newtoken->type		= ppvTokenType_INT;

	/*newtoken->integer	= PP->currentSourceFileStringNumber;*/

	gcmONERROR(
		ppoPREPROCESSOR_AddToOutputStreamOfPP(PP, newtoken)
		);

	gcmONERROR(
		ppoTOKEN_Destroy(PP, newtoken)
		);

	return gcvSTATUS_OK;

OnError:
	if (newtoken != gcvNULL)
	{
		gcmVERIFY_OK(ppoTOKEN_Destroy(PP, newtoken));
	}
	return status;

}
/*******************************************************************************
**
**	ppoPREPROCESSOR_TextLine
**
*/

gceSTATUS
ppoPREPROCESSOR_TextLine_AddToInputAfterMacroExpand(
	ppoPREPROCESSOR PP,
	ppoTOKEN expanded_id_head,
	ppoTOKEN expanded_id_end
	)
{
	if(expanded_id_head)
	{
		gcmASSERT(expanded_id_end != gcvNULL);

		PP->inputStream->base.node.next = (void*)expanded_id_end;
		expanded_id_end->inputStream.base.node.prev = (void*)PP->inputStream;

		PP->inputStream = (void*)expanded_id_head;
		expanded_id_head->inputStream.base.node.next = gcvNULL;
	}

	return gcvSTATUS_OK;
}

gceSTATUS
ppoPREPROCESSOR_TextLine_CheckSelfContainAndIsMacroOrNot(
	ppoPREPROCESSOR PP,
	ppoTOKEN  ThisToken,
	gctBOOL	*TokenIsSelfContain,
	ppoMACRO_SYMBOL *TheMacroSymbolOfThisId)
{
	gceSTATUS status;

	gcmONERROR(
		ppoHIDE_SET_LIST_ContainSelf(PP, ThisToken, TokenIsSelfContain)
		);

	gcmONERROR(
		ppoMACRO_MANAGER_GetMacroSymbol(
		PP,
		PP->macroManager,
		ThisToken->poolString,
		TheMacroSymbolOfThisId)
		);

	return gcvSTATUS_OK;

OnError:
	return status;

}
gceSTATUS
ppoPREPROCESSOR_TextLine(ppoPREPROCESSOR PP)
{
	gceSTATUS status = gcvSTATUS_INVALID_ARGUMENT;

	ppoTOKEN ntoken = gcvNULL;

	gctBOOL doWeInValidArea;

	gcmASSERT(PP && PP->base.type == ppvOBJ_PREPROCESSOR);

	doWeInValidArea	= PP->doWeInValidArea;

	if(doWeInValidArea == gcvFALSE)
	{
		return ppoPREPROCESSOR_ToEOL(PP);
	}

	gcmONERROR(
		ppoPREPROCESSOR_PassEmptyLine(PP)
		);

	gcmONERROR(
		PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
		);

	/*just check The first token of a line.*/

	while (
		ntoken->poolString != PP->keyword->eof
		&&
		!(ntoken->poolString == PP->keyword->sharp && ntoken->hideSet == gcvNULL))
	{

		/*check next token*/

		while (
			ntoken->poolString != PP->keyword->eof
			&&
			ntoken->poolString != PP->keyword->newline)
		{
			/*pre-defined macro, should not be editable.*/

			if (ntoken->poolString == PP->keyword->_file_
			||	ntoken->poolString == PP->keyword->_line_
			||	ntoken->poolString == PP->keyword->_version_
			||	ntoken->poolString == PP->keyword->gl_es)
			{
				gcmONERROR(
					ppoPREPROCESSOR_TextLine_Handle_FILE_LINE_VERSION(PP, ntoken->poolString)
					);

				gcmONERROR(
					ppoTOKEN_Destroy(PP, ntoken)
					);
                ntoken = gcvNULL;
			}
			else if(ntoken->type == ppvTokenType_ID)
			{
				/*Check the hide set of this ID.*/

				gctBOOL token_is_self_contain = gcvFALSE;

				ppoMACRO_SYMBOL the_macro_symbol_of_this_id = gcvNULL;

				gcmONERROR(ppoPREPROCESSOR_TextLine_CheckSelfContainAndIsMacroOrNot(
					PP,
					ntoken,
					&token_is_self_contain,
					&the_macro_symbol_of_this_id));

				if(token_is_self_contain || the_macro_symbol_of_this_id == gcvNULL)
				{
					gcmONERROR(ppoPREPROCESSOR_AddToOutputStreamOfPP(PP, ntoken));

					gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
                    ntoken = gcvNULL;
				}
				else
				{
					ppoTOKEN head = gcvNULL;
					ppoTOKEN end  = gcvNULL;
					gctBOOL any_expanation_happened = gcvFALSE;

					gcmONERROR(ppoINPUT_STREAM_UnGetToken(PP, &(PP->inputStream), ntoken));

					gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
                    ntoken = gcvNULL;

					gcmONERROR(ppoPREPROCESSOR_MacroExpand(
						PP,
						&(PP->inputStream),
						&head,
						&end,
						&any_expanation_happened));

					gcmASSERT(
						( head == gcvNULL && end == gcvNULL )
						||
						( head != gcvNULL && end != gcvNULL ));

					if(gcvTRUE == any_expanation_happened)
					{
						gcmONERROR(ppoPREPROCESSOR_TextLine_AddToInputAfterMacroExpand(
							PP,
							head,
							end));
					}
					else
					{
						gcmASSERT(head == end);

						if(head != gcvNULL)
						{
							gcmONERROR(ppoPREPROCESSOR_AddToOutputStreamOfPP(
								PP,
								head));
						}
					}
				}/*if(token_is_self_contain || the_macro_symbol_of_this_id == gcvNULL)*/
			}/*else if(ntoken->type == ppvTokenType_ID)*/
			else
			{
				/*Not ID*/
				gcmONERROR( ppoPREPROCESSOR_AddToOutputStreamOfPP(PP, ntoken) );

				gcmONERROR( ppoTOKEN_Destroy(PP, ntoken) );
                ntoken = gcvNULL;

			}

			gcmONERROR(
				PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
				);

		}/*internal while*/

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
        ntoken = gcvNULL;

		gcmONERROR(
			ppoPREPROCESSOR_PassEmptyLine(PP)
			);

		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
			);

	}/*extenal while*/

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

/*control_line***************************************************************************/
gceSTATUS
ppoPREPROCESSOR_ControlLine(ppoPREPROCESSOR PP)
{
	ppoTOKEN	ntoken = gcvNULL;

	gceSTATUS	status = gcvSTATUS_INVALID_ARGUMENT;

	gctBOOL		doWeInValidArea = PP->doWeInValidArea;

	if(doWeInValidArea == gcvFALSE)
	{

		return ppoPREPROCESSOR_ToEOL(PP);

	}

	gcmONERROR(
		PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
		);

	if(ntoken ->poolString == PP->keyword->define)
	{
		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

		return ppoPREPROCESSOR_Define(PP);
	}
	else if (ntoken ->poolString == PP->keyword->undef)
	{
		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

		return ppoPREPROCESSOR_Undef(PP);
	}
	else if (ntoken ->poolString == PP->keyword->error)
	{
		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

		return ppoPREPROCESSOR_Error(PP);
	}
	else if (ntoken ->poolString == PP->keyword->pragma)
	{
		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

		return ppoPREPROCESSOR_Pragma(PP);
	}
	else if(ntoken ->poolString == PP->keyword->extension)
	{
		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

		return ppoPREPROCESSOR_Extension(PP);
	}
	else if(ntoken ->poolString == PP->keyword->version)
	{
		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

		return ppoPREPROCESSOR_Version(PP);
	}
	else if (ntoken ->poolString == PP->keyword->line)
	{
		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

		return ppoPREPROCESSOR_Line(PP);
	}
	else
	{
        gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
		return gcvSTATUS_INVALID_ARGUMENT;
	}

OnError:
	return status;
}

/******************************************************************************\
Version
\******************************************************************************/
gceSTATUS ppoPREPROCESSOR_Version(ppoPREPROCESSOR PP)
{
	gctBOOL doWeInValidArea = PP->doWeInValidArea;
    gceSTATUS   status = gcvSTATUS_INVALID_DATA;

	if(doWeInValidArea == gcvTRUE)
	{
		/*Now we just support 100*/
		ppoTOKEN	ntoken = gcvNULL;

		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
			);

		if(ntoken->type != ppvTokenType_INT)
		{
			ppoPREPROCESSOR_Report(PP,
				slvREPORT_ERROR,
				"Expect a number afer the #version.");

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);

            status = gcvSTATUS_INVALID_DATA;
			return status;
		}
		if(ntoken->poolString != PP->keyword->version_100)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
				"Expect 100 afer the #version.",
				PP->currentSourceFileStringNumber,
				PP->currentSourceFileLineNumber);

			gcmONERROR(
				ppoTOKEN_Destroy(PP, ntoken)
				);

            status = gcvSTATUS_INVALID_DATA;
			return status;
		}
		else
		{
			sloCOMPILER_SetVersion(PP->compiler, 100);
		}

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

        status = gcvSTATUS_OK;
		return status;
	}
	else
	{
        status = ppoPREPROCESSOR_ToEOL(PP);
		return status;
	}

OnError:
	return status;
}
/******************************************************************************\
**
**	ppoPREPROCESSOR_Line
**
*/
gceSTATUS ppoPREPROCESSOR_Line(ppoPREPROCESSOR PP)
{
	gctBOOL doWeInValidArea	= 0;
	gctINT	line	= 0;
	gctINT	string	= 0;
	gceSTATUS	status = gcvSTATUS_INVALID_DATA;
    ppoTOKEN    ntoken = gcvNULL;

	gcmASSERT(PP);

	doWeInValidArea	= PP->doWeInValidArea;

	line	= PP->currentSourceFileLineNumber;

	string	= PP->currentSourceFileStringNumber;

	if(doWeInValidArea)
	{
		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
			);

		if(ntoken->type != ppvTokenType_INT)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
				"Expect integer-line-number after #line.");

			gcmONERROR(gcvSTATUS_INVALID_DATA);
		}

		/*process line number*/
		gcmONERROR(
			ppoPREPROCESSOR_EvalInt(
			PP,
			ntoken,
			&line)
			);

		if(line < 0)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
				"Expect positive integer-line-number after #line.");

			gcmONERROR(gcvSTATUS_INVALID_DATA);
		}

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
        ntoken = gcvNULL;

		gcmONERROR(
			PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace)
			);

		if(ntoken->type != ppvTokenType_EOF && ntoken->type != ppvTokenType_NL)
		{
			if(ntoken->type != ppvTokenType_INT)
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
					"Expect  source-string-number after #line.",
					PP->currentSourceFileStringNumber,
					PP->currentSourceFileLineNumber);

				gcmONERROR(gcvSTATUS_INVALID_DATA);
			}
			else
			{
				/*process poolString number*/
				gcmONERROR(
					ppoPREPROCESSOR_EvalInt(
					PP,
					ntoken,
					&string)
					);



				if(string < 0)
				{
					ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
						"Expect none negative source-string-number"
						" after #line.");
					gcmONERROR(gcvSTATUS_INVALID_DATA);
				}
			}
		}
		else
		{
			gcmONERROR(
				ppoINPUT_STREAM_UnGetToken(PP, &(PP->inputStream), ntoken)
				);
		}

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);
        ntoken = gcvNULL;

	}
	status = ppoPREPROCESSOR_ToEOL(PP);

	if(status != gcvSTATUS_OK){return status;}

	PP->currentSourceFileStringNumber = string;

	PP->currentSourceFileLineNumber = line + 1;

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
Extension
\******************************************************************************/
gceSTATUS ppoPREPROCESSOR_Extension(ppoPREPROCESSOR PP)
{
	gctBOOL doWeInValidArea = PP->doWeInValidArea;
    gceSTATUS   status = gcvSTATUS_INVALID_DATA;
    ppoTOKEN    ntoken = gcvNULL;
    ppoTOKEN    feature = gcvNULL;
    ppoTOKEN    behavior = gcvNULL;

    gcmHEADER_ARG("PP=0x%x", PP);

	if(doWeInValidArea == gcvTRUE)
	{
		gceSTATUS	status = gcvSTATUS_INVALID_DATA;
		gctUINT  i;
		gctINT	extensionIndex = -1;


		status = PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace);ppmCheckOK();

		if(ntoken->type != ppvTokenType_ID)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR, "Expect extension name here.");
			gcmONERROR(gcvSTATUS_INVALID_DATA);
		}

		feature = ntoken;


		for(i=0; i < _sldDefinedExtensionsCount; i++) {

		   if(gcmIS_SUCCESS(gcoOS_StrCmp(feature->poolString, _DefinedExtensions[i].str))) {
		      extensionIndex = i;
		      break;
 		   }
		}
	  	if(extensionIndex < 0) {
		   ppoPREPROCESSOR_Report(PP,slvREPORT_WARN,"Extension : %s is not provided by this compiler.", feature->poolString);
		}

		ntoken = gcvNULL;
		status = PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace);ppmCheckOK();

		if(ntoken->poolString != PP->keyword->colon)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR, "Expect : here.");
			gcmONERROR(gcvSTATUS_INVALID_DATA);
		}

		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
		ntoken = gcvNULL;

		gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace));

		if( ntoken->poolString != PP->keyword->require	&&
			ntoken->poolString != PP->keyword->enable	&&
			ntoken->poolString != PP->keyword->warn		&&
			ntoken->poolString != PP->keyword->disable)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
				"Expect 'require' or 'enable' or 'warn' or 'disable' here.");
			gcmONERROR(gcvSTATUS_INVALID_DATA);
		}

		behavior = ntoken;
		ntoken = gcvNULL;

		gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace));

		if(ntoken->poolString != PP->keyword->newline && ntoken->poolString != PP->keyword->eof)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,"Expect 'New Line' or 'End of File' here.");

			gcmONERROR(gcvSTATUS_INVALID_DATA);
		}


		/*sematic checking*/

		/*require*/
		if(behavior->poolString == PP->keyword->require)
		{
			if(feature->poolString == PP->keyword->all)
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,"Expect all's behavior should be warn or disable.");
				gcmONERROR(gcvSTATUS_INVALID_DATA);
			}
			else if (extensionIndex >= 0 && _DefinedExtensions[extensionIndex].require == gcvTRUE)
			{
				gcmVERIFY_OK(sloCOMPILER_EnableExtension(PP->compiler,
									 _DefinedExtensions[extensionIndex].flag,
									 gcvTRUE));
			}
			else if(extensionIndex < 0 || _DefinedExtensions[extensionIndex].require == gcvFALSE)
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,"Extension %s does not support 'require'.",
						       feature->poolString);
				gcmONERROR(gcvSTATUS_INVALID_DATA);
			}
		}

		/*enable*/
		if( behavior->poolString == PP->keyword->enable)
		{
			if(feature->poolString == PP->keyword->all)
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,"Expect all's behavior should be warn or disable.");
				gcmONERROR(gcvSTATUS_INVALID_DATA);

			}
			else if (extensionIndex >= 0 && _DefinedExtensions[extensionIndex].enable == gcvTRUE)
			{
				gcmVERIFY_OK(sloCOMPILER_EnableExtension(PP->compiler,
									 _DefinedExtensions[extensionIndex].flag,
									 gcvTRUE));
			}
			else
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_WARN,"Extension %s does not support 'enable'.",
					feature->poolString);
			}
		}

		/*warn*/
		if( behavior->poolString == PP->keyword->warn)
		{
			if(extensionIndex < 0 || _DefinedExtensions[extensionIndex].warn == gcvFALSE)
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_WARN,"Extension %s does not support 'warn'.",
					feature->poolString);
			}
		}

		/*disable*/
		if( behavior->poolString == PP->keyword->disable)
		{
			if(extensionIndex >= 0 && _DefinedExtensions[extensionIndex].disable == gcvTRUE)
			{
				gcmVERIFY_OK(sloCOMPILER_EnableExtension(PP->compiler,
									 _DefinedExtensions[extensionIndex].flag,
									 gcvFALSE));
			}
			else
			{
				ppoPREPROCESSOR_Report(PP,slvREPORT_WARN,"Extension %s does not support 'disable'.",
					feature->poolString);
			}
		}



		gcmONERROR(
			ppoTOKEN_Destroy(PP, feature)
			);
        feature = gcvNULL;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, behavior)
			);
        behavior = gcvNULL;

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

        gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}
	else
	{
        status = ppoPREPROCESSOR_ToEOL(PP);
        gcmFOOTER();
		return status;
	}

OnError:
    if (feature != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, feature));
        feature = gcvNULL;
    }
    if (behavior != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, behavior));
        behavior = gcvNULL;
    }
    if (ntoken != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
        ntoken = gcvNULL;
    }
    gcmFOOTER();
	return status;
}
/******************************************************************************\
Error
\******************************************************************************/
gceSTATUS ppoPREPROCESSOR_Error(ppoPREPROCESSOR PP)
{
	gctBOOL doWeInValidArea = PP->doWeInValidArea;
    gceSTATUS status = gcvSTATUS_INVALID_DATA;

	if(doWeInValidArea == gcvTRUE)
	{
		ppoTOKEN	ntoken = gcvNULL;

		ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
			"Error(str:%d,lin:%d): "
			"Meet #error with:",
			PP->currentSourceFileStringNumber,
			PP->currentSourceFileLineNumber);

		gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace));

		while(ntoken->poolString != PP->keyword->newline && ntoken->poolString != PP->keyword->eof)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR, "%s ", ntoken->poolString);

			gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

			gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace));
		}

		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

		ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR, "");
        status = gcvSTATUS_INVALID_DATA;
		return status;
	}
	else
	{
        status = ppoPREPROCESSOR_ToEOL(PP);
		return status;
	}

OnError:
	return status;
}
/******************************************************************************\
Pragma
\******************************************************************************/
gceSTATUS ppoPREPROCESSOR_Pragma(ppoPREPROCESSOR PP)
{
	return ppoPREPROCESSOR_ToEOL(PP);
}
/******************************************************************************\
Undef
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_Undef(ppoPREPROCESSOR PP)
{
	gctSTRING			name	= gcvNULL;
	ppoTOKEN			ntoken	= gcvNULL;
	ppoMACRO_SYMBOL		ms		= gcvNULL;
	gceSTATUS			status  = gcvSTATUS_INVALID_ARGUMENT;
	gctBOOL				doWeInValidArea	= PP->doWeInValidArea;

	if(doWeInValidArea == gcvFALSE)
	{
		return ppoPREPROCESSOR_ToEOL(PP);
	}
	status = PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace);
	if(status != gcvSTATUS_OK)
	{
		return status;
	}
	if(ntoken->type != ppvTokenType_ID){
		ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
			"Error(%d,%d) : #undef should followed by id.", PP->currentSourceFileStringNumber, PP->currentSourceFileLineNumber);

		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

		return gcvSTATUS_INVALID_ARGUMENT;
	}

	if(ntoken->poolString == PP->keyword->gl_es   ||
		ntoken->poolString == PP->keyword->_line_ ||
		ntoken->poolString == PP->keyword->_file_ ||
		ntoken->poolString == PP->keyword->_version_)
	{
		ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
			"Error(%d,%d) : Can not #undef builtin marcro %s.",
			PP->currentSourceFileStringNumber,
			PP->currentSourceFileLineNumber,
			ntoken->poolString);

		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

		return gcvSTATUS_INVALID_ARGUMENT;
	}

	name = ntoken->poolString;

	gcmONERROR(
		ppoMACRO_MANAGER_GetMacroSymbol(
		PP,
		PP->macroManager,
		name,
		&ms)
		);

	if(!ms || ms->undefined == gcvTRUE)
	{
		ppoPREPROCESSOR_Report(
			PP,
			slvREPORT_WARN,
			"#undef a undefined id.");

		gcmONERROR(
			ppoTOKEN_Destroy(PP, ntoken)
			);

		return gcvSTATUS_OK;
	}

	ms->undefined = gcvTRUE;

    gcmONERROR(
	ppoMACRO_MANAGER_DestroyMacroSymbol(
		PP,
        PP->macroManager,
        ms));

	gcmONERROR(
		ppoTOKEN_Destroy(PP, ntoken)
		);

	return gcvSTATUS_OK;

OnError:
    if (ntoken != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
        ntoken =  gcvNULL;
    }
	return status;
}

/******************************************************************************\
Define
\******************************************************************************/
gceSTATUS
ppoPREPROCESSOR_Define(ppoPREPROCESSOR PP)
{
	gctSTRING			name		= gcvNULL;
	gctINT				argc		= 0;
	ppoTOKEN			argv		= gcvNULL;
	ppoTOKEN			rlst		= gcvNULL;
	ppoTOKEN			ntoken		= gcvNULL;
	ppoMACRO_SYMBOL		ms			= gcvNULL;
	gceSTATUS			status		= gcvSTATUS_INVALID_ARGUMENT;
	gctBOOL				doWeInValidArea		= PP->doWeInValidArea;
	gctBOOL				redefined	= gcvFALSE;
    gctBOOL				redefError	= gcvFALSE;
	ppoTOKEN			ntokenNext	= gcvNULL;
	ppoTOKEN			mstokenNext	= gcvNULL;

	if(doWeInValidArea == gcvFALSE)
	{
		return ppoPREPROCESSOR_ToEOL(PP);
	}

	gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, !ppvICareWhiteSpace));


	if(ntoken->type != ppvTokenType_ID){
		ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
			"Error(%d,%d) : #define should followed by id.",
			PP->currentSourceFileStringNumber,
			PP->currentSourceFileLineNumber);

		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));

		return gcvSTATUS_INVALID_ARGUMENT;
	}

	/*01 name*/

	name = ntoken->poolString;

	gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
    ntoken = gcvNULL;

	if(	name == PP->keyword->_line_ ||
		name == PP->keyword->_version_ ||
		name == PP->keyword->_file_)
	{
		ppoPREPROCESSOR_Report(PP,slvREPORT_WARN,
			"No Effect with re-defining of %s.",
			name);
		return ppoPREPROCESSOR_ToEOL(PP);
	}

	/*check if preceded by GL_ or __*/
	if(!gcoOS_StrNCmp(name , "GL_",3))
	{
		ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
			"GL_ is reserved to used by feature.");
	}
	if(!gcoOS_StrNCmp(name , "__",2))
	{
		ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
			"__ is reserved to used by the compiler.");
	}

	gcmONERROR(
		ppoMACRO_MANAGER_GetMacroSymbol(PP, PP->macroManager, name, &ms)
		);

	if(ms != gcvNULL)
	{
		redefined = gcvTRUE;
	}

	gcmONERROR(PP->inputStream->GetToken(PP, &(PP->inputStream), &ntoken, ppvICareWhiteSpace));

	if(ntoken->poolString == PP->keyword->lpara)
	{
		/*macro with (arguments-opt)*/

		/*collect argv*/

		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
		ntoken = gcvNULL;

		gcmONERROR(ppoPREPROCESSOR_Define_BufferArgs(PP, &argv, &argc));
		if(argc == 0)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_WARN, "No argument in () of macro.");
			gcmASSERT(argv);
		}
	}
	else if(ntoken->type != ppvTokenType_WS)
	{
		if(ntoken->type != ppvTokenType_NL)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR, "White Space or New Line inputStream expected.");
		}
		else
		{
			/*NL*/
			gcmONERROR(
				ppoINPUT_STREAM_UnGetToken(PP, &(PP->inputStream), ntoken)
				);

		}

		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
		ntoken = gcvNULL;
	}
	else
	{
		gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
		ntoken = gcvNULL;
	}

	/*replacement list*/

	gcmONERROR(
		ppoPREPROCESSOR_Define_BufferReplacementList(PP, &rlst)
		);

	if(redefined)
	{
		if( argc != ms->argc)
		{
			ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
								"Can not redefine defined macro %s.",name);
			redefError = gcvTRUE;
		}
		else
		{
				ntokenNext = rlst;
				mstokenNext = ms->replacementList;

				while(ntokenNext || mstokenNext)
				{
					if(/* One of token is NULL */
					(ntokenNext != mstokenNext && (mstokenNext == gcvNULL || ntokenNext == gcvNULL)) ||
					/* Different replacement list */
					(!gcmIS_SUCCESS(gcoOS_StrCmp(ntokenNext->poolString,mstokenNext->poolString))))
				{
					ppoPREPROCESSOR_Report(PP,slvREPORT_ERROR,
						"Can not redefine defined macro %s.",name);
					redefError = gcvTRUE;
					break;
				}

				ntokenNext = (ppoTOKEN)ntokenNext->inputStream.base.node.prev;
				mstokenNext = (ppoTOKEN)mstokenNext->inputStream.base.node.prev;
			}
		}

		while(argv)
		{
			ntokenNext = (ppoTOKEN)argv->inputStream.base.node.prev;
			gcmONERROR(ppoTOKEN_Destroy(PP, argv));
			argv = ntokenNext;
		}

		while(rlst)
		{
			ntokenNext = (ppoTOKEN)rlst->inputStream.base.node.prev;
			gcmONERROR(ppoTOKEN_Destroy(PP, rlst));
			rlst = ntokenNext;
		}

		if(redefError)
			return gcvSTATUS_INVALID_ARGUMENT;

		return gcvSTATUS_OK;
	}

	/*make ms*/
	gcmONERROR(ppoMACRO_SYMBOL_Construct(
		(void*)PP,
		__FILE__,
		__LINE__,
		"ppoPREPROCESSOR_PPDefine : find a macro name, \
		prepare to add a macro in the cpp's mac manager.",
		name,
		argc,
		argv,
		rlst,
		&ms));

	return ppoMACRO_MANAGER_AddMacroSymbol(
		PP,
		PP->macroManager,
		ms);

OnError:
    if (ntoken != gcvNULL)
    {
        gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
        ntoken = gcvNULL;
    }
	return status;
}

