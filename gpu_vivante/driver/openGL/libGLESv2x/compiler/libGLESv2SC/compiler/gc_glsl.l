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





%{
#include "gc_glsl_scanner.h"

static gctUINT CurrentLineNo		= 0;
static gctUINT CurrentStringNo		= 0;

void
slScanComment(
	IN sloCOMPILER Compiler
	);

#define YY_NO_UNISTD_H

/*#define YY_FATAL_ERROR(msg)	\
	slReport(0, 0, slvREPORT_FATAL_ERROR, msg)*/

#define YY_INPUT(buf, result, max_size) \
	{ \
		 result = slInput(max_size, buf); \
	}

#define YY_DECL \
	gctINT \
	sloCOMPILER_Scan( \
		IN sloCOMPILER Compiler, \
		OUT slsLexToken * Token \
		)

#define DUMP_SOURCE() \
	gcmVERIFY_OK(sloCOMPILER_Dump( \
							Compiler, \
							slvDUMP_SOURCE, \
							"<SOURCE line=\"%d\" string=\"%d\">'%s'</SOURCE>", \
							CurrentLineNo, \
							CurrentStringNo, \
							yytext))

#define DUMP_SOURCE_COMMENT_BEGIN() \
	gcmVERIFY_OK(sloCOMPILER_Dump( \
							Compiler, \
							slvDUMP_SOURCE, \
							"<SOURCE_COMMENT line=\"%d\" string=\"%d\">'%s", \
							CurrentLineNo, \
							CurrentStringNo, \
							yytext))

#define DUMP_SOURCE_COMMENT_END() \
	gcmVERIFY_OK(sloCOMPILER_Dump( \
							Compiler, \
							slvDUMP_SOURCE, \
							"'</SOURCE_COMMENT>"))

#define DUMP_SOURCE_CHAR(ch) \
	gcmVERIFY_OK(sloCOMPILER_Dump( \
							Compiler, \
							slvDUMP_SOURCE, \
							"%c", \
							ch))


#ifdef SL_SCAN_NO_ACTION

	#define SL_SCAN_IDENTIFIER()		T_IDENTIFIER
	#define SL_SCAN_UNSIGNED_INTEGER_TYPE()	T_UINT
	#define SL_SCAN_BOOLCONSTANT(value)	T_BOOLCONSTANT
	#define SL_SCAN_DEC_INTCONSTANT()	T_INTCONSTANT
	#define SL_SCAN_OCT_INTCONSTANT()	T_INTCONSTANT
	#define SL_SCAN_HEX_INTCONSTANT()	T_INTCONSTANT
	#define SL_SCAN_FLOATCONSTANT()		T_FLOATCONSTANT
	#define SL_SCAN_OPERATOR(type)		(type)
	#define SL_SCAN_FIELD_SELECTION()	T_FIELD_SELECTION

#else

	#define SL_SCAN_IDENTIFIER()		\
				slScanIdentifier(Compiler, CurrentLineNo, CurrentStringNo, yytext, Token)

	#define SL_SCAN_UNSIGNED_INTEGER_TYPE()		\
		slScanConvToUnsignedType(Compiler, CurrentLineNo, CurrentStringNo, yytext, Token)

	#define SL_SCAN_BOOLCONSTANT(value)	\
				slScanBoolConstant(Compiler, CurrentLineNo, CurrentStringNo, value, Token)

	#define SL_SCAN_DEC_INTCONSTANT()	\
				slScanDecIntConstant(Compiler, CurrentLineNo, CurrentStringNo, yytext, Token)

	#define SL_SCAN_OCT_INTCONSTANT()	\
				slScanOctIntConstant(Compiler, CurrentLineNo, CurrentStringNo, yytext, Token)

	#define SL_SCAN_HEX_INTCONSTANT()	\
				slScanHexIntConstant(Compiler, CurrentLineNo, CurrentStringNo, yytext, Token)

	#define SL_SCAN_FLOATCONSTANT()		\
				slScanFloatConstant(Compiler, CurrentLineNo, CurrentStringNo, yytext, Token)

	#define SL_SCAN_OPERATOR(type)		\
				slScanOperator(Compiler, CurrentLineNo, CurrentStringNo, yytext, type, Token)

	#define SL_SCAN_FIELD_SELECTION()	\
				slScanFieldSelection(Compiler, CurrentLineNo, CurrentStringNo, yytext, Token)

#endif /* SL_SCAN_NO_ACTION */

#define ECHO        do { /* Do nothing */ } while (gcvFALSE)
%}

%option outfile="gc_glsl_scanner.c"
%option never-interactive
%option nounput

%x SCAN_FIELD_SELECTION
%x SCAN_UNSIGNED_INTEGER_TYPE


identifier					{nodigit}({nodigit}|{digit})*
nodigit						[_A-Za-z]
digit						[0-9]

decimal_constant				{nozero_digit}{digit}*
octal_constant					0{octal_digit}*
hexadecimal_constant				0[Xx]{hexadecimal_digit}+

nozero_digit					[1-9]
octal_digit					[0-7]
hexadecimal_digit				[0-9A-Fa-f]

floating_constant				{floating_constant_1}|{floating_constant_2}
floating_constant_1				{fractional_constant}{exponent_part}?
floating_constant_2				{digit_sequence}{exponent_part}

fractional_constant				{fractional_constant_1}|{fractional_constant_2}
fractional_constant_1				{digit_sequence}?"."{digit_sequence}
fractional_constant_2				{digit_sequence}"."

exponent_part					[eE]{sign}?{digit_sequence}
sign								[+-]
digit_sequence					{digit}+

signed_integer_type				int
white_sp_nl                                     [ \t\n]+
%%

"true"						{ DUMP_SOURCE(); return SL_SCAN_BOOLCONSTANT(gcvTRUE); }
"false"						{ DUMP_SOURCE(); return SL_SCAN_BOOLCONSTANT(gcvFALSE); }

"unsigned"{white_sp_nl}+			{ DUMP_SOURCE();
						  BEGIN(SCAN_UNSIGNED_INTEGER_TYPE); }
<SCAN_UNSIGNED_INTEGER_TYPE>{signed_integer_type} { DUMP_SOURCE();
                                                    BEGIN(INITIAL);
						    return SL_SCAN_UNSIGNED_INTEGER_TYPE(); }
{identifier}					{ DUMP_SOURCE(); return SL_SCAN_IDENTIFIER(); }

{decimal_constant}				{ DUMP_SOURCE(); return SL_SCAN_DEC_INTCONSTANT(); }
{octal_constant}				{ DUMP_SOURCE(); return SL_SCAN_OCT_INTCONSTANT(); }
{hexadecimal_constant}				{ DUMP_SOURCE(); return SL_SCAN_HEX_INTCONSTANT(); }

{floating_constant}				{ DUMP_SOURCE(); return SL_SCAN_FLOATCONSTANT(); }

"<<"								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_LEFT_OP); }
">>"								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_RIGHT_OP); }
"++"								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_INC_OP); }
"--"								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_DEC_OP); }
"<="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_LE_OP); }
">="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_GE_OP); }
"=="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_EQ_OP); }
"!="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_NE_OP); }
"&&"								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_AND_OP); }
"||"								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_OR_OP); }
"^^"								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_XOR_OP); }
"*="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_MUL_ASSIGN); }
"/="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_DIV_ASSIGN); }
"+="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_ADD_ASSIGN); }
"%="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_MOD_ASSIGN); }
"<<="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_LEFT_ASSIGN); }
">>="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_RIGHT_ASSIGN); }
"&="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_AND_ASSIGN); }
"^="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_XOR_ASSIGN); }
"|="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_OR_ASSIGN); }
"-="								{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(T_SUB_ASSIGN); }

"("									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('('); }
")"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(')'); }
"["									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('['); }
"]"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(']'); }
"{"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('{'); }
"}"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('}'); }
"."									{ BEGIN(SCAN_FIELD_SELECTION);
										DUMP_SOURCE(); return SL_SCAN_OPERATOR('.'); }
","									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(','); }
":"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(':'); }
"="									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('='); }
";"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR(';'); }
"!"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('!'); }
"-"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('-'); }
"~"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('~'); }
"+"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('+'); }
"*"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('*'); }
"/"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('/'); }
"%"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('%'); }
"<"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('<'); }
">"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('>'); }
"|"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('|'); }
"^"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('^'); }
"&"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('&'); }
"?"									{ DUMP_SOURCE(); return SL_SCAN_OPERATOR('?'); }

<SCAN_FIELD_SELECTION>{identifier}	{ BEGIN(INITIAL);
										DUMP_SOURCE(); return SL_SCAN_FIELD_SELECTION(); }

<*>[ \t\n\r\f\v]					{ DUMP_SOURCE(); /* Ignore */ }

<*>"/*"								{ DUMP_SOURCE_COMMENT_BEGIN(); slScanComment(Compiler); }
<*>"/""/".*							{ DUMP_SOURCE(); /* Ignore */ }

<*>.								{ DUMP_SOURCE();
										sloCOMPILER_Report(
														Compiler,
														CurrentLineNo,
														CurrentStringNo,
														slvREPORT_ERROR,
														"unexpected character: '%s'", yytext); }

<*><<EOF>>							{ yy_delete_buffer(YY_CURRENT_BUFFER); yyterminate(); }

%%

void
slScanComment(
	IN sloCOMPILER Compiler
	)
{
	gctINT ch;

	while (gcvTRUE)
	{
		while ((ch = input()) != '*' && ch != EOF) DUMP_SOURCE_CHAR(ch);

		if (ch == EOF)
		{
			sloCOMPILER_Report(
							Compiler,
							CurrentLineNo,
							CurrentStringNo,
							slvREPORT_ERROR,
							"invalid comment: unexpected end of file");

			break;
		}

		DUMP_SOURCE_CHAR(ch);

		while ((ch = input()) == '*') DUMP_SOURCE_CHAR(ch);

		if (ch == EOF)
		{
			sloCOMPILER_Report(
							Compiler,
							CurrentLineNo,
							CurrentStringNo,
							slvREPORT_ERROR,
							"invalid comment: unexpected end of file");

			break;
		}
		else if (ch == '/')
		{
			DUMP_SOURCE_CHAR(ch);
			break;
		}
		else
		{
			DUMP_SOURCE_CHAR(ch);
		}
	}

	DUMP_SOURCE_COMMENT_END();
}

gceSTATUS
sloCOMPILER_SetCurrentLineNo(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo
	)
{
	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

	CurrentLineNo	= LineNo;

	return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_SetCurrentStringNo(
	IN sloCOMPILER Compiler,
	IN gctUINT StringNo
	)
{
	/* Verify the arguments. */
	slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

	CurrentStringNo	= StringNo;

	return gcvSTATUS_OK;
}

gctUINT
sloCOMPILER_GetCurrentLineNo(
	IN sloCOMPILER Compiler
	)
{
	/* Verify the arguments. */
	slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

	return CurrentLineNo;
}

gctUINT
sloCOMPILER_GetCurrentStringNo(
	IN sloCOMPILER Compiler
	)
{
	/* Verify the arguments. */
	slmASSERT_OBJECT(Compiler, slvOBJ_COMPILER);

	return CurrentStringNo;
}

extern void
yyerror(
	char *msg
	)
{
	slReport(
			CurrentLineNo,
			CurrentStringNo,
			slvREPORT_ERROR,
			msg);

	yy_delete_buffer(YY_CURRENT_BUFFER);
}
