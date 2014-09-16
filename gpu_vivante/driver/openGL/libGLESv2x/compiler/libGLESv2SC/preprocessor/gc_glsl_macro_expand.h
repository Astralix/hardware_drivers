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


#ifndef __gc_glsl_macro_expand_h_
#define	__gc_glsl_macro_expand_h_

gceSTATUS
ppoPREPROCESSOR_MacroExpand(
							ppoPREPROCESSOR		PP,
							ppoINPUT_STREAM		*IS,
							ppoTOKEN			*Head,
							ppoTOKEN			*End,
							gctBOOL				*AnyExpanationHappened
							);

gceSTATUS
ppoPREPROCESSOR_MacroExpand_0_SelfContain(
	ppoPREPROCESSOR		PP,
	ppoINPUT_STREAM		*IS,
	ppoTOKEN			*Head,
	ppoTOKEN			*End,
	gctBOOL				*AnyExpanationHappened,
	gctBOOL				*MatchCase,
	ppoTOKEN			*ID
	);

gceSTATUS
ppoPREPROCESSOR_MacroExpand_1_NotMacroSymbol(
	ppoPREPROCESSOR		PP,
	ppoINPUT_STREAM		*IS,
	ppoTOKEN			*Head,
	ppoTOKEN			*End,
	gctBOOL				*AnyExpanationHappened,
	gctBOOL				*MatchCase,
	ppoTOKEN			ID,
	ppoMACRO_SYMBOL		*MS
	);

gceSTATUS
ppoPREPROCESSOR_MacroExpand_2_NoFormalArgs(
	ppoPREPROCESSOR		PP,
	ppoINPUT_STREAM		*IS,
	ppoTOKEN			*Head,
	ppoTOKEN			*End,
	gctBOOL				*AnyExpanationHappened,
	gctBOOL				*MatchCase,
	ppoTOKEN			ID,
	ppoMACRO_SYMBOL		MS
	);

gceSTATUS
ppoPREPROCESSOR_MacroExpand_3_NoMoreTokenInIS(
	ppoPREPROCESSOR		PP,
	ppoINPUT_STREAM		*IS,
	ppoTOKEN			*Head,
	ppoTOKEN			*End,
	gctBOOL				*AnyExpanationHappened,
	gctBOOL				*MatchCase,
	ppoTOKEN			ID
	);

gceSTATUS
ppoPREPROCESSOR_MacroExpand_4_NoRealArg(
										ppoPREPROCESSOR		PP,
										ppoINPUT_STREAM		*IS,
										ppoTOKEN			*Head,
										ppoTOKEN			*End,
										gctBOOL				*AnyExpanationHappened,
										gctBOOL				*MatchCase,
										ppoTOKEN			ID
										);

gceSTATUS
ppoPREPROCESSOR_MacroExpand_5_BufferRealArgs(
	ppoPREPROCESSOR		PP,
	ppoINPUT_STREAM		*IS,
	ppoTOKEN			*HeadTail,
	ppoTOKEN			ID,
	ppoMACRO_SYMBOL		MS
	);

gceSTATUS
ppoPREPROCESSOR_MacroExpand_6_ExpandHeadTail(
	ppoPREPROCESSOR		PP,
	ppoINPUT_STREAM		*IS,
	ppoTOKEN			*HeadTail,
	ppoTOKEN			*ExpandHeadTail,
	ppoTOKEN			ID,
	ppoMACRO_SYMBOL		MS
	);

gceSTATUS
ppoPREPROCESSOR_MacroExpand_7_ParseReplacementList(
	ppoPREPROCESSOR		PP,
	ppoINPUT_STREAM		*IS,
	ppoTOKEN			*Head,
	ppoTOKEN			*End,
	gctBOOL				*AnyExpanationHappened,
	ppoTOKEN			*ExpandedHeadTail,
	ppoTOKEN			ID,
	ppoMACRO_SYMBOL		MS
	);

#endif /* __gc_glsl_macro_expand_h_ */
