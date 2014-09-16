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


#ifndef __gc_glsl_scanner_h_
#define __gc_glsl_scanner_h_

#include "gc_glsl_compiler_int.h"
#include "gc_glsl_ir.h"

typedef struct _slsLexToken
{
	gctUINT		lineNo;

	gctUINT		stringNo;

	gctINT		type;

	union
	{
		sluCONSTANT_VALUE  constant;

		sltPOOL_STRING	identifier;

		sltQUALIFIER	qualifier;

		sltPRECISION	precision;

		gctINT		operator;

		slsNAME *	typeName;

		sltPOOL_STRING	fieldSelection;
	}
	u;
}
slsLexToken;

typedef struct _slsDeclOrDeclList
{
	slsDATA_TYPE *	dataType;

	sloIR_BASE		initStatement;

	sloIR_SET		initStatements;
}
slsDeclOrDeclList;

typedef struct _slsFieldDecl
{
	slsDLINK_NODE	node;

	slsNAME *		field;

	gctUINT			arrayLength;
}
slsFieldDecl;

typedef struct _slsSelectionStatementPair
{
	sloIR_BASE	trueStatement;

	sloIR_BASE	falseStatement;
}
slsSelectionStatementPair;

typedef struct _slsForExprPair
{
	sloIR_EXPR	condExpr;

	sloIR_EXPR	restExpr;
}
slsForExprPair;

#include "gc_glsl_token_def.h"

#ifndef T_EOF
#define T_EOF		0
#endif

gctINT
sloCOMPILER_Scan(
	IN sloCOMPILER Compiler,
	OUT slsLexToken * Token
	);

gctINT
slScanBoolConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctBOOL Value,
	OUT slsLexToken * Token
	);

gctINT
slScanIdentifier(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Symbol,
	OUT slsLexToken * Token
	);

gctINT
slScanConvToUnsignedType(
IN sloCOMPILER Compiler,
IN gctUINT LineNo,
IN gctUINT StringNo,
IN gctSTRING Symbol,
OUT slsLexToken * Token
);

gctINT
slScanDecIntConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	OUT slsLexToken * Token
	);

gctINT
slScanOctIntConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	OUT slsLexToken * Token
	);

gctINT
slScanHexIntConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	OUT slsLexToken * Token
	);

gctINT
slScanFloatConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	OUT slsLexToken * Token
	);

gctINT
slScanOperator(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	IN gctINT tokenType,
	OUT slsLexToken * Token
	);

gctINT
slScanFieldSelection(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Symbol,
	OUT slsLexToken * Token
	);

int
yylex(
	YYSTYPE * pyylval,
	sloCOMPILER Compiler
	);

void
yyerror(
	char *msg
	);

#ifndef YY_STACK_USED
#define YY_STACK_USED 0
#endif
#ifndef YY_ALWAYS_INTERACTIVE
#define YY_ALWAYS_INTERACTIVE 0
#endif
#ifndef YY_MAIN
#define YY_MAIN 0
#endif

#endif /* __gc_glsl_scanner_h_ */
