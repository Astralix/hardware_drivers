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


#ifndef __gc_glsl_preprocessor_h_
#define __gc_glsl_preprocessor_h_

#include "gc_glsl_compiler.h"

typedef struct	_ppoPREPROCESSOR*	sloPREPROCESSOR;

gceSTATUS
sloPREPROCESSOR_Construct(
	IN sloCOMPILER Compiler,
	OUT sloPREPROCESSOR * PP
	);

gceSTATUS
sloPREPROCESSOR_Destroy(
	IN sloCOMPILER Compiler,
	IN sloPREPROCESSOR PP
	);

gceSTATUS
sloPREPROCESSOR_SetSourceStrings(
	IN sloPREPROCESSOR PP,
	IN gctUINT StringCount,
	IN gctCONST_STRING Strings[]
);

gceSTATUS
sloPREPROCESSOR_Parse(
	IN sloPREPROCESSOR PP,
	IN gctINT MaxSize,
	OUT gctSTRING Buffer,
	OUT gctINT * ActualSize
	);


#define ppvMAX_MACRO_ARGS_NUMBER			64
#define	ppvMAX_PPTOKEN_CHAR_NUMBER			1000
#define ppvCHAR_EOF							(char) (unsigned char) 0xFF

#if ppvMAX_PPTOKEN_CHAR_NUMBER < 8
#error	Please set ppvMAX_PPTOKEN_CHAR_NUMBER >= 8.
#endif

#endif /* __gc_glsl_preprocessor_h_ */
