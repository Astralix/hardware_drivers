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
ppoBYTE_INPUT_STREAM_GetChar_Phase_0(
    IN  ppoPREPROCESSOR         PP,
    IN  ppoBYTE_INPUT_STREAM    BIS,
    OUT gctSTRING               New
    );

gceSTATUS
ppoBYTE_INPUT_STREAM_UnGetChar_Phase_0(
    IN  ppoPREPROCESSOR PP
    );

gceSTATUS
ppoBYTE_INPUT_STREAM_GetChar_Phase_1(
    IN  ppoPREPROCESSOR PP,
    IN  ppoBYTE_INPUT_STREAM Bis,
    OUT char* Pc
    );

gceSTATUS
ppoBYTE_INPUT_STREAM_UnGetChar_Phase_1(
    IN  ppoPREPROCESSOR PP
    );

gceSTATUS
ppoBYTE_INPUT_STREAM_GetChar_Phase_2(
    IN  ppoPREPROCESSOR PP,
    IN  ppoBYTE_INPUT_STREAM Bis,
    OUT char* Pc
    );

gceSTATUS
ppoBYTE_INPUT_STREAM_UnGetChar_Phase_2(
    IN  ppoPREPROCESSOR PP
    );

#define ppoBYTE_INPUT_STREAM_GetChar    ppoBYTE_INPUT_STREAM_GetChar_Phase_2
#define ppoBYTE_INPUT_STREAM_UnGetChar  ppoBYTE_INPUT_STREAM_UnGetChar_Phase_2




gceSTATUS
ppoINPUT_STREAM_Dump(
    ppoPREPROCESSOR     PP,
    ppoINPUT_STREAM     IS
    )
{
    gcmASSERT(PP && IS);

    switch (IS->base.type)
    {

    case ppvOBJ_TOKEN:

        return ppoTOKEN_STREAM_Dump(PP, (ppoTOKEN_STREAM)IS);

    case ppvOBJ_BYTE_INPUT_STREAM:

        return ppoBYTE_INPUT_STREAM_Dump(PP, IS);

    default:

        gcmVERIFY_OK(sloCOMPILER_Report(
            PP->compiler,
            1,
            0,
            slvREPORT_INTERNAL_ERROR,
            "This is not a inputstream object."
            ));

        return gcvSTATUS_INVALID_DATA;
    }
}


/******************************************************************************\
Dump
\******************************************************************************/
gceSTATUS
ppoBYTE_INPUT_STREAM_Dump(
    ppoPREPROCESSOR PP,
    ppoINPUT_STREAM IS)
{
    ppoBYTE_INPUT_STREAM    local_bis   = (ppoBYTE_INPUT_STREAM)IS;
    gctCONST_STRING         p           = gcvNULL;
    gctINT                  c           = 0;
    gceSTATUS               status      = gcvSTATUS_INVALID_DATA;

    gcmASSERT(IS && IS->base.type == ppvOBJ_BYTE_INPUT_STREAM);

    gcmONERROR(
        sloCOMPILER_Dump(
            PP->compiler,
            slvDUMP_PREPROCESSOR,
            "<ByteInputStream fileNumber=\"%d\" byteCount=\"%d\" />",
            local_bis->inputStringNumber,
            local_bis->count
            )
        );

    gcmONERROR(
        ppoBASE_Dump(PP, (ppoBASE)IS)
        );

    p = &(local_bis->src[0]);

    while(c < local_bis->count)
    {
        if(c == local_bis->curpos)
        {
            if(*p != '\n')
            {
                gcmONERROR(
                    sloCOMPILER_Dump(
                        PP->compiler,
                        slvDUMP_PREPROCESSOR,
                        "<Char nextReadPosition=\"%c\" />",
                        *p
                        )
                    );
            }
            else
            {
                gcmONERROR(
                    sloCOMPILER_Dump(
                        PP->compiler,
                        slvDUMP_PREPROCESSOR,
                        "<Char NextReadPosition=\"NewLine\" />"
                        )
                    );

            }
        }
        else
        {
            if(*p != '\n')
            {
                gcmONERROR(
                    sloCOMPILER_Dump(
                        PP->compiler,
                        slvDUMP_PREPROCESSOR,
                        "<Char inputStream=\"%c\" />",
                        *p
                        )
                    );
            }
            else
            {
                gcmONERROR(
                    sloCOMPILER_Dump(
                        PP->compiler,
                        slvDUMP_PREPROCESSOR,
                        "<Char inputStream=\"NewLine\" />",
                        *p
                        )
                    );
            }
        }
        ++c;
        ++p;
    }

    gcmONERROR(
        sloCOMPILER_Dump(
            PP->compiler,
            slvDUMP_PREPROCESSOR,
            "</ByteInputStream>"
            )
        );


    if(IS->base.node.prev)
    {
        return
            ppoINPUT_STREAM_Dump(
                PP,
                (ppoINPUT_STREAM)(IS->base.node.prev)
                );
    }

    return gcvSTATUS_OK;

OnError:
	return status;
}


gceSTATUS
ppoBYTE_INPUT_STREAM_Release(
	ppoPREPROCESSOR		    PP,
	ppoBYTE_INPUT_STREAM	BIS)
{
    gcmHEADER_ARG("PP=0x%x BIS=0x%x",
                  PP, BIS);

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/*
Test tip:
1.Empty input string.
2.Edit the processing of newline.
3.gcvNULL appear in input stream
*/
/*********************************************ppoINPUT_STREAM_Init*******************************/
gceSTATUS
ppoINPUT_STREAM_Init(
    /*00*/  ppoPREPROCESSOR     PP,
    /*01*/  ppoINPUT_STREAM     YouCreated,
    /*02*/  gctCONST_STRING     File,
    /*03*/  gctUINT             Line,
    /*04*/  gctCONST_STRING     MoreInfo,
    /*05*/  ppeOBJECT_TYPE      Type,
    /*06*/  gceSTATUS           (*GetToken) (ppoPREPROCESSOR, ppoINPUT_STREAM*, ppoTOKEN*,gctBOOL)
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("PP=0x%x YouCreated=0x%x File=0x%x Line=%d MoreInfo=0x%x Type=%d GetToken=0x%x",
                  PP, YouCreated, File, Line, MoreInfo, Type, GetToken);

    gcmASSERT(YouCreated && File && Line && MoreInfo && Type && GetToken);

	/*01*/
	YouCreated->GetToken = GetToken;

	/*00*/
	gcmASSERT( ppvOBJ_BYTE_INPUT_STREAM == Type || ppvOBJ_TOKEN == Type );

	status =  ppoBASE_Init(
		PP,
		(ppoBASE)YouCreated,
		File,
		Line,
		MoreInfo,
		Type);

    gcmFOOTER();
    return status;
}


gceSTATUS
ppoINPUT_STREAM_UnGetToken(
    ppoPREPROCESSOR		PP,
    ppoINPUT_STREAM*	Is,
    ppoTOKEN			Token)
{
	gceSTATUS	status		= gcvSTATUS_INVALID_ARGUMENT;
	ppoTOKEN	newtoken	= gcvNULL;

    gcmHEADER_ARG("PP=0x%x IS=0x%x Token=0x%x",
                  PP, Is, Token);

    gcmASSERT(Is && Token);

    if (Token == gcvNULL)
    {
        gcmFOOTER();
    	return status;
    }

	if(Token->type == ppvTokenType_EOF)
	{
        gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}
	status = ppoTOKEN_Colon(
		PP,
		Token,
		__FILE__,
		__LINE__,
		"Dump and push on the inputStream of cpp.",
		&newtoken);

	ppmCheckOK();

	(*Is)->base.node.next = (void*)newtoken;
	newtoken->inputStream.base.node.prev = (void*)(*Is);
	newtoken->inputStream.base.node.next = gcvNULL;
	(*Is) = (void*)newtoken;

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}


gceSTATUS
ppoBYTE_INPUT_STREAM_Construct(
                           /*00*/   ppoPREPROCESSOR     PP,
                           /*01*/   ppoBYTE_INPUT_STREAM        Prev,
                           /*02*/   ppoBYTE_INPUT_STREAM        Next,
                           /*03*/   gctCONST_STRING             File,
                           /*04*/   gctINT                      Line,
                           /*05*/   gctCONST_STRING             MoreInfo,
                           /*06*/   gctCONST_STRING             Src,
                           /*07*/   const gctINT                InputStringNumber,
                           /*08*/   const gctINT                Count,
                           /*09*/   ppoBYTE_INPUT_STREAM*       Created
    )
{
    ppoBYTE_INPUT_STREAM    rt      = gcvNULL;
    gceSTATUS               status  = gcvSTATUS_INVALID_ARGUMENT;
    gctPOINTER pointer = gcvNULL;
    gcmASSERT(File  && Line && MoreInfo && Src && Created);


    status =
        sloCOMPILER_Allocate(
            PP->compiler,
            sizeof(struct _ppoBYTE_INPUT_STREAM),
            &pointer
            );

    if(status != gcvSTATUS_OK)
    {
        ppoPREPROCESSOR_Report(PP,slvREPORT_FATAL_ERROR,"ppoPREPROCESSOR_CBIS_Creat : Failed to alloc BIS.");
        return gcvSTATUS_OUT_OF_MEMORY;
    }

    rt = pointer;

    /*00:   isBase*/
    status = ppoINPUT_STREAM_Init(
        PP,
        /*00    youris*/    (ppoINPUT_STREAM)rt,
        /*01    file*/      File,
        /*02    line*/      Line,
        /*03    moreInfo*/  MoreInfo,
        /*04    istype*/    ppvOBJ_BYTE_INPUT_STREAM,
        /*05    gettoken*/  ppoBYTE_INPUT_STREAM_GetToken
        );
    if(status != gcvSTATUS_OK)
    {
        if (rt != gcvNULL)
        {
            sloCOMPILER_Free(PP->compiler, rt);
            rt = gcvNULL;
        }
        return status;
    }

    rt->inputStream.base.node.prev = (void*)Prev;
    rt->inputStream.base.node.next =    (void*)Next;

    if(Prev)
    {

        Prev->inputStream.base.node.next = (void*)rt;

    }
    if(Next)
    {

        Next->inputStream.base.node.prev = (void*)rt;

    }
    /*01:   src*/
    rt->src     = Src;
    /*02:   pcCur*/
    rt->curpos  = 0;
    /*03:   count*/
    rt->count   = Count;
    /*04:   inputStringNumber*/
    rt->inputStringNumber = InputStringNumber;

    *Created = rt;
    return gcvSTATUS_OK;
}

/*Alphabet, Number, _*/
gctBOOL ppoPREPROCESSOR_isalnum_(char c)
{
    return  (c >= 'A' && c <= 'Z')
        ||
        (c >= 'a' && c <= 'z' )
        ||
        c == '_'
        ||
        (c >= '0' && c <= '9');
}
/*Alphabet, _*/
gctBOOL ppoPREPROCESSOR_isal_(char c)
{
    return  (c >= 'A' && c <= 'Z')
        ||
        (c >= 'a' && c <= 'z' )
        ||
        c == '_';
}

/*HEX Number*/
gctBOOL
ppoPREPROCESSOR_ishexnum(char c)
{
    return
        ( (c>='A') && (c<='F') )
        ||
        ( (c>='a') && (c<='f') )
        ||
        ( (c>='0') && (c<='9') )
        ;
}

/*OCT*/
gctBOOL
ppoPREPROCESSOR_isoctnum(char c)
{
    return c>='0' && c<='7';
}

/*Number*/
gctBOOL
ppoPREPROCESSOR_isnum(char c)
{
    return c>='0' && c<='9';
}

/*White Space*/
gctBOOL
ppoPREPROCESSOR_isws(char c)
{
    return
		(c == '\t')	||	/*Horizental Table*/
		(c == '\v')	||	/*Vertical Table*/
		(c == 12)	||	/*Form Feed*/
		(c == 13)	||	/*Carrige Return*/
		(c == 32);		/*Space*/
}

/*Newline*/
gctBOOL
ppoPREPROCESSOR_isnl(char c)
{
    return c == '\n';
}

gctBOOL
ppoPREPROCESSOR_islegalchar(char c)
{
    return
        c == ppvCHAR_EOF
        ||
        ppoPREPROCESSOR_isnum(c)
        ||
        ppoPREPROCESSOR_isal_(c)
        ||
        ppoPREPROCESSOR_isws(c)
        ||
        ppoPREPROCESSOR_ispunc(c)
        ||
        ppoPREPROCESSOR_isnl(c)
        ;
}

/*Set string*/
gceSTATUS
ppoPREPROCESSOR_setnext(
           ppoPREPROCESSOR      PP,
           char     c,
           gctSTRING        cb,
           gctINT*      pcblen)
{
    gcmASSERT(PP);
    if( (*pcblen) >= ppvMAX_PPTOKEN_CHAR_NUMBER)
    {
        ppoPREPROCESSOR_Report(
            PP,
            slvREPORT_ERROR,
            "The limitation of the length of one token inputStream : %d, "
            "please contact your compiler provider to get help.",
            ppvMAX_PPTOKEN_CHAR_NUMBER);
        return  gcvSTATUS_INVALID_DATA;
    }
    else
    {
        cb[*pcblen] = c;
        ++(*pcblen);
        return gcvSTATUS_OK;
    }
}

/*Not Single Punctuator*/
gctBOOL
ppoPREPROCESSOR_isnspunc(char c)
{
    return
        /*00*/c == '+' ||   /*  +=, ++  */
        /*01*/c == '-' ||   /*  -=, --  */
        /*02*/c == '<' ||   /*  <<, <=, <<= */
        /*03*/c == '>' ||   /*  >>, >=, >>= */
        /*04*/c == '*' ||   /*  *=  */
        /*05*/c == '&' ||   /*  &&, &=  */
        /*06*/c == '=' ||   /*  ==  */
        /*07*/c == '!' ||   /*  !=  */
        /*08*/c == '^' ||   /*  ^=  */
        /*09*/c == '|' ||   /*  ||, |=  */
        /*10*/c == '/' ||   /*  /=, / *  */
        /*11*/c == '%'      /*  %=  */
        ;
}

/*Punctuator*/
gctBOOL
ppoPREPROCESSOR_ispunc(char c)
{
    /*24+1*/
    return
        c == '.' || c == '+' || c == '-' || c == '/' || c == '*' || c == '%' || c == '<' || c == '>' || c == '[' || c == ']' ||
        c == '(' || c == ')' || c == '{' || c == '}' || c == '^' || c == '|' || c == '&' || c == '~' || c == '=' || c == '!' ||
        c == ':' || c == ';' || c == ',' || c == '?'
        ||
        c == '#'
        ||
        c == '\\'
        ;
}

#define ppmBIS_GetToken_CheckOK()\
    do\
{\
    if(status != gcvSTATUS_OK)\
{\
    PP->mm->Free(PP, PP->mm, (ppoBASE*)&hsem);\
}\
    ppmCheckOK();\
}\
    while(0)



struct _ppoInfomationForBISRollBack
{
    ppoBYTE_INPUT_STREAM    from_here_to_the_end;
    gctINT                  reset_here_curpos_to;
};

typedef struct _ppoInfomationForBISRollBack*
ppoInfomationForBISRollBack;

gceSTATUS
ppoInfomationForBISRollBack_RollBackBISList(
    ppoPREPROCESSOR PP,
    ppoInfomationForBISRollBack Info
    )
{
    ppoBYTE_INPUT_STREAM    here = Info->from_here_to_the_end;

    gcmASSERT(
        ((ppoINPUT_STREAM)here)->base.type == ppvOBJ_BYTE_INPUT_STREAM
        );

    here->curpos = Info->reset_here_curpos_to;

    here = (ppoBYTE_INPUT_STREAM)here->inputStream.base.node.prev;

    while(here)
    {

        gcmASSERT(
            ((ppoINPUT_STREAM)here)->base.type == ppvOBJ_BYTE_INPUT_STREAM
            );

        here->curpos = 0;

		here = (ppoBYTE_INPUT_STREAM)here->inputStream.base.node.prev;
    }

    PP->currentSourceFileStringNumber = Info->from_here_to_the_end->inputStringNumber;

    return gcvSTATUS_OK;

}

gceSTATUS
ppoBYTE_INPUT_STREAM_GetToken(
                              ppoPREPROCESSOR       PP,
                              ppoINPUT_STREAM*  Is,
                              ppoTOKEN*         Token,
                              gctBOOL               WhiteSpace)
{

    gceSTATUS               status  = gcvSTATUS_INVALID_ARGUMENT;

    ppoTOKEN                ntoken  = gcvNULL;

    char                    c       = ppvCHAR_EOF;

    gctINT                  cblen   = 0;

    char                    cb[ppvMAX_PPTOKEN_CHAR_NUMBER+1];

    ppoBYTE_INPUT_STREAM    bis     = (ppoBYTE_INPUT_STREAM)*Is;

    ppoInfomationForBISRollBack rollbackinfo = gcvNULL;

    gctPOINTER                  pointer = gcvNULL;


    gcmASSERT(
        bis
        &&
        ((ppoINPUT_STREAM)bis)->base.type == ppvOBJ_BYTE_INPUT_STREAM);

    /*alloc a new token*/
    gcmONERROR(
        ppoTOKEN_Construct(
        PP,
        /*01    file*/  __FILE__,
        /*02    line*/  __LINE__,
        /*03    moreInfo*/ "ppoPREPROCESSOR_CBIS_GetToken : Creat sematic value.",
        &ntoken)
        );

    /*reset ntoken's type*/
    ntoken->type = ppvTokenType_ERROR;

    /*get first char.*/

    do
    {
        gcmONERROR(
            ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c)
            );

        if(ppoPREPROCESSOR_isws(c) && WhiteSpace == gcvTRUE)
        {

            ntoken->type    = ppvTokenType_WS;

            ntoken->poolString      = PP->keyword->ws;

            *Token          = ntoken;

            (*Token)->inputStream.base.node.prev = gcvNULL;

            (*Token)->inputStream.base.node.next = gcvNULL;

            return gcvSTATUS_OK;

        }

        if(c == ppvCHAR_EOF)
        {
            ntoken->type = ppvTokenType_EOF;

            ntoken->poolString  = PP->keyword->eof;

            *Token = ntoken;

            (*Token)->inputStream.base.node.prev = gcvNULL;

            (*Token)->inputStream.base.node.next = gcvNULL;

            return gcvSTATUS_OK;
        }

    }
    while(ppoPREPROCESSOR_isws(c));

    if(PP->toLineEnd && (PP->currentSourceFileLineNumber == PP->skipLine))
    {
        while(c != '\n' && c != ppvCHAR_EOF)
        {
            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
        }

        if(c == '\n')
        {
            ntoken->type = ppvTokenType_NL;
            ntoken->poolString = PP->keyword->newline;
        }
        else
        {
            ntoken->type = ppvTokenType_EOF;
            ntoken->poolString = PP->keyword->eof;
        }
        *Token = ntoken;
        (*Token)->inputStream.base.node.prev = gcvNULL;
        (*Token)->inputStream.base.node.next = gcvNULL;
        return gcvSTATUS_OK;
    }

    if (ppoPREPROCESSOR_islegalchar(c))
    {
        /*in-doWeInValidArea gctCONST_STRING/
        ntoken->type = ppvTokenType_NOT_IN_LEGAL_CHAR_SET;

        ppoPREPROCESSOR_Report(
        PP,
        slvREPORT_WARN,
        "illegal Character found: %c, hex value inputStream %xh.",
        c, c);

        gcmONERROR(
        ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c)
        );

        }
        else
        ******************************************************************************
        New Line
        ******************************************************************************/

        if(ppoPREPROCESSOR_isnl(c))
        {
            ntoken->type    = ppvTokenType_NL;

            ntoken->poolString      = PP->keyword->newline;

            *Token          = ntoken;

            (*Token)->inputStream.base.node.prev = gcvNULL;

            (*Token)->inputStream.base.node.next = gcvNULL;

            PP->iAmFollowingAComment = gcvFALSE;

            return gcvSTATUS_OK;
        }
        /******************************************************************************\
        ID
        \******************************************************************************/
        else if(ppoPREPROCESSOR_isal_(c))
        {

            ntoken->type = ppvTokenType_ID;

            while(ppoPREPROCESSOR_isalnum_(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
            {

                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));

            }

            ntoken->hideSet = gcvNULL;

        }
        /******************************************************************************\
        PPNumber
        \******************************************************************************/
        else if (ppoPREPROCESSOR_isnum(c))
        {

            /*number,0 preceding*/
            if(c == '0' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
            {
                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));

                if((c == 'x' || c == 'X') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                {
                    /*
                    0x or
                    0X
                    */
                    ntoken->type =  ppvTokenType_ERROR;

                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));

                    while(ppoPREPROCESSOR_ishexnum(c) &&    ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                    {

                        ntoken->type =  ppvTokenType_INT;

                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));

                    }

                }/*Hex*/
                else if(c == '.' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                {
                    ntoken->type =  ppvTokenType_FLOAT;
                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                    {
                        /*0.digit-sequence*/
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    }
                    if((c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                    {
                        /*0.digit-sequence E/e */
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if((c == '+' || c == '-') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                        {
                            /*0.digit-sequence E/e +/-*/
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                            {
                                /*0.digit-sequence E/e +/- digit-sequence*/
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                        else
                        {
                            /*0.digit-sequence E/e digit-sequence*/
                            while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                            {
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                    }/*if((c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )*/
                }/*else if(c == '.')*/
                else if( (c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                {
                    /*0 E/e*/
                    ntoken->type =  ppvTokenType_FLOAT;
                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    if((c == '+' || c == '-') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                    {
                        /*0 E/e +/-*/
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                        {
                            /*0 E/e +/- digit-sequence*/
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    else
                    {
                        /*0 E/e digit-sequence*/
                        while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                        {
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                }
                else
                {
                    /*Maybe Oct*/
                    gctBOOL has_none_oct_digit = gcvFALSE;
                    gctINT  first_none_oct_digit_pos_in_cb = -1;

                    gcmONERROR(
                        sloCOMPILER_Allocate(
                        PP->compiler,
                        sizeof(struct _ppoInfomationForBISRollBack),
                        &pointer)
                        );

                    rollbackinfo = pointer;

                    rollbackinfo->from_here_to_the_end = (ppoBYTE_INPUT_STREAM)gcvNULL;

                    rollbackinfo->reset_here_curpos_to = -1;

                    while(ppoPREPROCESSOR_isoctnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                    {
                        /* 0 oct-digit-sequence */
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    }

                    while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                    {
                        /*0 digit-sequence*/
                        if(first_none_oct_digit_pos_in_cb == -1)
                        {
                            first_none_oct_digit_pos_in_cb = cblen-1;
                        }

                        if(rollbackinfo->from_here_to_the_end == gcvNULL)
                        {
                            rollbackinfo->from_here_to_the_end =
                                PP->lastGetcharPhase0IsFromThisBis;

                            gcmASSERT(
                                (rollbackinfo->from_here_to_the_end->curpos - 1) >= 0
                                );

                            rollbackinfo->reset_here_curpos_to =
                                rollbackinfo->from_here_to_the_end->curpos - 1;
                        }

                        has_none_oct_digit = gcvTRUE;

                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    }

                    if(c == '.' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                    {
                        ntoken->type =  ppvTokenType_FLOAT;
                        /*digit-sequence .*/
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0){
                            /*digit-sequence . digit-sequence*/
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                        /*digit-sequence . digit-sequence E/e */
                        if((c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                        {
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            if((c == '+' || c == '-') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                            {
                                /*digit-sequence . digit-sequence E/e +/-*/
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                                while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                                {
                                    /*digit-sequence . digit-sequence E/e +/- digit-sequence*/
                                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                                }
                            }
                            else
                            {
                                /*digit-sequence . digit-sequence E/e digit-sequence*/
                                while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                                {
                                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                                }
                            }
                        }/*if((c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )*/

                    }/*if(c == '.')*/
                    else if( (c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                    {
                        ntoken->type =  ppvTokenType_FLOAT;
                        /*digit-sequence E/e*/
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if((c == '+' || c == '-') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                        {
                            /*digit-sequence E/e +/-*/
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                            {
                                /*digit-sequence E/e +/- digit-sequence*/
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                        else
                        {
                            /*digit-sequence E/e digit-sequence*/
                            while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                            {
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                    }
                    else
                    {
                        /*
                        0 maybe-oct-digit-sequence, maybe-none-oct-digit-sequende
                        or
                        0 none-digit-sequende
                        */
                        if(has_none_oct_digit == gcvTRUE)
                        {

                            gcmONERROR(
                                ppoInfomationForBISRollBack_RollBackBISList(
                                PP,
                                rollbackinfo)
                                );

                            cblen = first_none_oct_digit_pos_in_cb;

                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));

                        }
                        ntoken->type =  ppvTokenType_INT;
                    }
                    if (rollbackinfo != gcvNULL ) {
                        status = sloCOMPILER_Free(PP->compiler, rollbackinfo);
                        rollbackinfo = gcvNULL;
                    }
                }/*Maybe OCT*/
            }/*number,0 preceding*/
            else
            {
                /*number but not 0 preceding*/
                ntoken->type =  ppvTokenType_INT;
                while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                {
                    /*digit-sequence*/
                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                }
                if(c == '.' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                {
                    ntoken->type =  ppvTokenType_FLOAT;
                    /*digit-sequence .*/
                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0){
                        /*digit-sequence . digit-sequence*/
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    }
                    /*digit-sequence . digit-sequence E/e */
                    if((c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if((c == '+' || c == '-') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                        {
                            /*digit-sequence . digit-sequence E/e +/-*/
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                            {
                                /*digit-sequence . digit-sequence E/e +/- digit-sequence*/
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                        else
                        {
                            /*digit-sequence . digit-sequence E/e digit-sequence*/
                            while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                            {
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                    }/*if((c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )*/
                }/*if(c == '.')*/
                else if( (c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                {
                    ntoken->type =  ppvTokenType_FLOAT;
                    /*digit-sequence E/e*/
                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    if((c == '+' || c == '-') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                    {
                        /*digit-sequence E/e +/-*/
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                        {
                            /*digit-sequence E/e +/- digit-sequence*/
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    else
                    {
                        /*digit-sequence E/e digit-sequence*/
                        while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                        {
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                }/*else if( (c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)*/
            }/*number,~0 preceding*/
        }
        /**********************************************************************\
        Punctuator
        \**********************************************************************/
        else if(ppoPREPROCESSOR_ispunc(c))
        {
            ntoken->type = ppvTokenType_PUNC;
            if(c == '.' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0)
            {
                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                if(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                {
                    /*.digit-sequence*/
                    ntoken->type =  ppvTokenType_FLOAT;
                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    }
                    /*.digit-sequence E/e */
                    if((c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if((c == '+' || c == '-') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )
                        {
                            /*.digit-sequence E/e +/-*/
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                            {
                                /*0.digit-sequence E/e +/- digit-sequence*/
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                        else
                        {
                            /*.digit-sequence E/e digit-sequence*/
                            while(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)
                            {
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                    }/*if((c == 'E' || c == 'e') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0 )*/
                }/*if(ppoPREPROCESSOR_isnum(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen)==0)*/
            }/*if(c == '.' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0)*/
            else
            {
                /*not .*/
                ntoken->type = ppvTokenType_PUNC;
                if(ppoPREPROCESSOR_isnspunc(c) && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0)
                {
                    /*00    +   */
                    if(c == '+'){
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if( (c == '=' || c == '+') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0 ){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    /*01    -   */
                    else if(c == '-'){
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if( (c == '=' || c == '-') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    /*02    <   */
                    else if(c == '<')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                        else if (c == '<' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                    }
                    /*03    >   */
                    else if(c == '>')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                        else if (c == '>' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                        }
                    }
                    /*04    *   */
                    else if(c == '*')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    /*05    &   */
                    else if(c == '&')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if((c == '&' || c == '=') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    /*06    =   */
                    else if(c == '=')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    /*07    !   */
                    else if(c == '!')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    /*08    ^   */
                    else if(c == '^')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if((c == '^' || c == '=') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    /*09    |   */
                    else if(c == '|')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if((c == '|' || c == '=') && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    /*10    /   */
                    else if(c == '/')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                        else if(c == '*')
                        {
                            /*comment*/
                            gctBOOL legalenv = PP->doWeInValidArea;
                            PP->doWeInValidArea = gcvFALSE;/*accept not doWeInValidArea character.*/
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            while(1)
                            {
                                if(c == ppvCHAR_EOF)
                                {
                                    ppoPREPROCESSOR_Report(
                                        PP,
                                        slvREPORT_ERROR,
                                        "Unexpected end of file, maybe"
                                        "you forget */.");

                                    gcmONERROR(
                                        sloCOMPILER_Free(PP->compiler, ntoken)
                                        );

                                    return gcvSTATUS_INVALID_DATA;
                                }
                                if (c == '*')
                                {
                                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));

                                    if(c == '/')
                                    {
                                        PP->doWeInValidArea = legalenv;

                                        gcmONERROR(
                                            sloCOMPILER_Free(PP->compiler, ntoken)
                                            );

                                        return ppoBYTE_INPUT_STREAM_GetToken(PP, Is, Token, WhiteSpace);
                                    }
                                }
                                else
                                {
                                    gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                                }
                            }
                        }
                        else if(c == '/')
                        {
                            /*commonet*/
                            gctBOOL legalenv = PP->doWeInValidArea;
                            PP->doWeInValidArea = gcvFALSE;/*accept not doWeInValidArea character.*/

                            PP->iAmFollowingAComment = gcvTRUE;

                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));



                            while(c != '\n' && c != ppvCHAR_EOF)
                            {
                                gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                            }
                            PP->doWeInValidArea = legalenv;
                            if(c == '\n')
                            {
                                ntoken->type = ppvTokenType_NL;
                                ntoken->poolString = PP->keyword->newline;
                            }
                            else
                            {
                                ntoken->type = ppvTokenType_EOF;
                                ntoken->poolString = PP->keyword->eof;
                            }
                            *Token = ntoken;
                            (*Token)->inputStream.base.node.prev = gcvNULL;
                            (*Token)->inputStream.base.node.next = gcvNULL;
                            return gcvSTATUS_OK;
                        }
                    }
                    /*11    %   */
                    else if(c == '%')
                    {
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        if(c == '=' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                            gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                        }
                    }
                    else
                    {
                        ppoPREPROCESSOR_Report(
                            PP,
                            slvREPORT_INTERNAL_ERROR,
                            "ppoPREPROCESSOR_CBIS_GetToken : Unhandled a not single punctuator %c.",c);
                        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
                    }
                }/*not single punctuator.*/
                else
                {
                    ntoken->type = ppvTokenType_PUNC;
                    /*signle charactor.*/
                    if(ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0){
                        gcmONERROR(ppoBYTE_INPUT_STREAM_GetChar(PP, bis, &c));
                    }
                }/*signle charactor.*/
            }/*if(c == '.' && ppoPREPROCESSOR_setnext(PP,c, cb, &cblen) == 0)*/
        }/*Punctuator*/
        else
        {
            ppoPREPROCESSOR_Report(PP,
                slvREPORT_INTERNAL_ERROR,
                "ppoPREPROCESSOR_CBIS_GetToken : Should not go into this path.");

            gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
        }
        /*end scanning*/
    }

    gcmONERROR(ppoBYTE_INPUT_STREAM_UnGetChar(PP));

    cb[cblen] = '\0';

    if( ntoken->type == ppvTokenType_ERROR )
    {
        ppoPREPROCESSOR_Report(
            PP,
            slvREPORT_INTERNAL_ERROR,
            "ppoBYTE_INPUT_STREAM_GetToken : Unhandle the type of the token genetated : %s",
            cb);

        gcmONERROR(gcvSTATUS_INVALID_ARGUMENT);
    }

    status =
        sloCOMPILER_AllocatePoolString(
        PP->compiler,
        cb,
        &(ntoken->poolString
        )
        );

    if(status != gcvSTATUS_OK)
    {

        ppoPREPROCESSOR_Report(
            PP,
            slvREPORT_INTERNAL_ERROR,
            "ppoBYTE_INPUT_STREAM_GetToken : Failed to add the literal to a string manager");
        if (ntoken != gcvNULL)
        {
            gcmONERROR(ppoTOKEN_Destroy(PP, ntoken));
            ntoken = gcvNULL;
        }

        return status;

    }

    *Token = ntoken;

    (*Token)->inputStream.base.node.prev = gcvNULL;

    (*Token)->inputStream.base.node.next = gcvNULL;

    return gcvSTATUS_OK;

OnError:
	if (ntoken != gcvNULL)
	{
		gcmVERIFY_OK(ppoTOKEN_Destroy(PP, ntoken));
		ntoken = gcvNULL;
	}

	if (rollbackinfo != gcvNULL)
	{
		gcmVERIFY_OK(sloCOMPILER_Free(PP->compiler, rollbackinfo));
		rollbackinfo = gcvNULL;
	}
	return status;
}


/******************************************************************************\
To provide a single stream
\******************************************************************************/
gceSTATUS
ppoBYTE_INPUT_STREAM_GetChar_Phase_0(
    IN      ppoPREPROCESSOR         PP,
    IN      ppoBYTE_INPUT_STREAM    Bis,
    OUT     char*               Pc
    )
{
    /*CASE 1: Handle the end of the list of BIS*/
    if(Bis == gcvNULL)
    {
        *Pc = ppvCHAR_EOF;

        PP->lastGetcharPhase0IsFromThisBis = gcvNULL;

        return gcvSTATUS_OK;

    }

    /*CASE 2: When this BIS inputStream not meet the last gctCONST_STRING*/
    if(Bis->curpos < Bis->count)
    {
        *Pc = Bis->src[Bis->curpos];

        ++(Bis->curpos);



        PP->lastGetcharPhase0IsFromThisBis = Bis;

        if(*Pc == '\n')
        {
            PP->currentSourceFileLineNumber++;
        }
        else if(
            (!ppoPREPROCESSOR_islegalchar(*Pc))
            &&
            PP->doWeInValidArea)
        {
            ppoPREPROCESSOR_Report(
                PP,
                slvREPORT_ERROR,
                "illegal character : '%c', hex value is %xh",
                *Pc, *Pc);

            gcmTRACE(
                gcvLEVEL_ERROR,
                "ppoBYTE_INPUT_STREAM_GetCharLowLevel: "
                "illegal character. : '%c', hex value is %xh",
                *Pc, *Pc);

            return gcvSTATUS_INVALID_DATA;
        }

        return gcvSTATUS_OK;
    }

    return ppoBYTE_INPUT_STREAM_GetChar_Phase_0(
        PP,
        (ppoBYTE_INPUT_STREAM)(Bis->inputStream.base.node.prev),
        Pc);
}


gceSTATUS
ppoBYTE_INPUT_STREAM_UnGetChar_Phase_0(IN   ppoPREPROCESSOR PP)
{

    ppoBYTE_INPUT_STREAM bis = PP->lastGetcharPhase0IsFromThisBis;

    if(bis == gcvNULL)
    {
        return gcvSTATUS_OK;
    }

    gcmASSERT(
        bis->curpos != 0
        &&
        bis->curpos <= bis->count
        );

    --(bis->curpos);

    if(bis->src[bis->curpos] == '\n')
    {
        --(PP->currentSourceFileLineNumber);
    }

    return gcvSTATUS_OK;

}
/******************************************************************************\
Move \r\n to \n, and move \r to WS
\******************************************************************************/
 gceSTATUS
ppoBYTE_INPUT_STREAM_GetChar_Phase_1(
    IN  ppoPREPROCESSOR         PP,
    IN  ppoBYTE_INPUT_STREAM    Bis,
    OUT char*               Pc
    )
{

    char c1,c2;

    gceSTATUS status = gcvSTATUS_INVALID_DATA;

    gcmONERROR(
        ppoBYTE_INPUT_STREAM_GetChar_Phase_0(
        PP,
        Bis,
        &c1)
        );

    if(c1 == '\r')
    {

        gcmONERROR(
            ppoBYTE_INPUT_STREAM_GetChar_Phase_0(
            PP,
            Bis,
            &c2)
            );

        if(c2 == '\n')
        {
            *Pc = '\n';

            return  gcvSTATUS_OK;
        }
        else
        {
            gcmONERROR(
                ppoBYTE_INPUT_STREAM_UnGetChar_Phase_0(PP)
                );

            /*

            '\r' should be '\n'
            qizhuang.liu, 2009-06-24, when debug BUG 613.

            */
            *Pc = '\n';

            return  gcvSTATUS_OK;
        }

    }
    else
    {

        *Pc = c1;

        return  gcvSTATUS_OK;

    }

OnError:
	return status;
}
/******************************************************************************\
To remove \ and  \n
\******************************************************************************/
 gceSTATUS
ppoBYTE_INPUT_STREAM_GetChar_Phase_2(
    IN      ppoPREPROCESSOR         PP,
    IN      ppoBYTE_INPUT_STREAM    Bis,
    OUT     char*               Pc
    )
{

    char c1,c2;

    gceSTATUS status = gcvSTATUS_INVALID_DATA;

    gctBOOL  i_am_following_a_comment = PP->iAmFollowingAComment;

    gcmONERROR(
        ppoBYTE_INPUT_STREAM_GetChar_Phase_1(
        PP,
        Bis,
        &c1
        )
        );

    if(c1 == '\\')
    {

        gcmONERROR(
            ppoBYTE_INPUT_STREAM_GetChar_Phase_1(
                PP,
                Bis,
                &c2
                )
            );

        if(c2 == '\n')
        {
            if(gcvTRUE == i_am_following_a_comment)
            {
                *Pc = '\n';

                ppoPREPROCESSOR_Report(
                    PP,
                    slvREPORT_WARN,
                    "single-line comment contains line-continuation character,"
                    " ignore the line-continuation character");

                return  gcvSTATUS_OK;
            }
            else
            {
                return ppoBYTE_INPUT_STREAM_GetChar_Phase_1(PP, Bis, Pc);
            }
        }
        else
        {

            gcmONERROR(
                ppoBYTE_INPUT_STREAM_UnGetChar_Phase_0(PP)
                );

            *Pc = '\\';

            return  gcvSTATUS_OK;

        }

    }
    else
    {

        *Pc = c1;

        return  gcvSTATUS_OK;

    }

OnError:
	return status;
}

gceSTATUS
ppoBYTE_INPUT_STREAM_UnGetChar_Phase_1(
                                       IN   ppoPREPROCESSOR PP
                                       )
{
    return ppoBYTE_INPUT_STREAM_UnGetChar_Phase_0(PP);
}
gceSTATUS
ppoBYTE_INPUT_STREAM_UnGetChar_Phase_2(
                                       IN   ppoPREPROCESSOR PP
                                       )
{
    return ppoBYTE_INPUT_STREAM_UnGetChar_Phase_1(PP);
}

