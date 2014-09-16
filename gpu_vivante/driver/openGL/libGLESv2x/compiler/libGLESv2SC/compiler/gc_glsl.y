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
#include "gc_glsl_parser.h"

#define YY_NO_UNISTD_H

#define YYPARSE_PARAM_DECL	sloCOMPILER
#define YYPARSE_PARAM		Compiler

#define YYLEX_PARAM			Compiler

#define YY_DECL				int yylex(YYSTYPE * pyylval, sloCOMPILER Compiler)

#define YYFPRINTF           yyfprintf

static int yyfprintf(FILE *file, const char * msg, ...)
{
    /* Do nothing */
    return 0;
}
%}

%pure_parser

%union
{
	slsLexToken					token;

	slsDeclOrDeclList			declOrDeclList;

	slsDLINK_LIST *				fieldDeclList;

	slsFieldDecl *				fieldDecl;

	slsDATA_TYPE *				dataType;

	sloIR_EXPR					expr;

	slsNAME	*					funcName;

	slsNAME	*					paramName;

	sloIR_SET					statements;

	sloIR_BASE					statement;

	slsSelectionStatementPair	selectionStatementPair;

	slsForExprPair				forExprPair;

	sloIR_POLYNARY_EXPR			funcCall;
}

/* tokens */
%token<token>		T_ATTRIBUTE T_CONST T_BOOL T_FLOAT T_INT
					T_BREAK T_CONTINUE T_DO T_ELSE T_FOR T_IF T_DISCARD T_RETURN
					T_BVEC2 T_BVEC3 T_BVEC4 T_IVEC2
					T_IVEC3 T_IVEC4 T_VEC2 T_VEC3 T_VEC4
					T_MAT2 T_MAT3 T_MAT4 T_IN T_OUT T_INOUT T_UNIFORM T_VARYING
					T_UINT T_UVEC2 T_UVEC3 T_UVEC4
					T_MAT2X3 T_MAT2X4 T_MAT3X2 T_MAT3X4 T_MAT4X2 T_MAT4X3
					T_SAMPLER2D T_SAMPLERCUBE
					T_SAMPLER3D
					T_SAMPLER1DARRAY T_SAMPLER2DARRAY T_SAMPLER1DARRAYSHADOW T_SAMPLER2DARRAYSHADOW
					T_ISAMPLER2D T_ISAMPLERCUBE T_ISAMPLER3D T_ISAMPLER2DARRAY
					T_USAMPLER2D T_USAMPLERCUBE T_USAMPLER3D T_USAMPLER2DARRAY
					T_SAMPLEREXTERNALOES
					T_STRUCT T_VOID T_WHILE

%token<token>		T_IDENTIFIER T_TYPE_NAME T_FLOATCONSTANT T_INTCONSTANT T_BOOLCONSTANT
					T_FIELD_SELECTION
					T_LEFT_OP T_RIGHT_OP
					T_INC_OP T_DEC_OP T_LE_OP T_GE_OP T_EQ_OP T_NE_OP
					T_AND_OP T_OR_OP T_XOR_OP T_MUL_ASSIGN T_DIV_ASSIGN T_ADD_ASSIGN
					T_MOD_ASSIGN T_LEFT_ASSIGN T_RIGHT_ASSIGN T_AND_ASSIGN T_XOR_ASSIGN T_OR_ASSIGN
					T_SUB_ASSIGN

%token<token>		T_INVARIANT T_HIGH_PRECISION T_MEDIUM_PRECISION T_LOW_PRECISION T_PRECISION

/* types */
%type<token>		'(' ')' '[' ']' '{' '}' '.' ',' ':' '=' ';' '!'
					'-' '~' '+' '*' '/' '%' '<' '>' '|' '^' '&' '?'

%type<token>		function_identifier constructor_identifier unary_operator assignment_operator
					precision_qualifier type_qualifier parameter_qualifier

%type<expr>			variable_identifier primary_expression postfix_expression integer_expression
					unary_expression multiplicative_expression additive_expression shift_expression
					relational_expression equality_expression and_expression
					exclusive_or_expression inclusive_or_expression logical_and_expression
					logical_xor_expression logical_or_expression conditional_expression
					assignment_expression initializer expression constant_expression
					condition conditionopt

%type<declOrDeclList>	init_declarator_list single_declaration

%type<dataType>		fully_specified_type type_specifier type_specifier_no_prec struct_specifier

%type<fieldDeclList>	struct_declarator_list

%type<fieldDecl>	struct_declarator

%type<funcName>		function_prototype function_declarator function_header_with_parameters
					function_header parameter_declarator parameter_type_specifier

%type<paramName>	parameter_declaration

%type<statements>	compound_statement compound_statement_no_new_scope statement_list

%type<statement>	declaration statement statement_no_new_scope simple_statement
					declaration_statement expression_statement selection_statement
					iteration_statement jump_statement for_init_statement

%type<selectionStatementPair>	selection_rest_statement

%type<forExprPair>	for_rest_statement

%type<funcCall>		function_call function_call_generic function_call_header_with_parameters
					function_call_header_no_parameters function_call_header

/* yacc rules */
%right T_IF T_ELSE

%start translation_unit

%%

variable_identifier :
	T_IDENTIFIER
		{ $$ = slParseVariableIdentifier(Compiler, &$1); }
	;

primary_expression :
	variable_identifier
		{ $$ = $1; }
	| T_INTCONSTANT
		{ $$ = slParseIntConstant(Compiler, &$1); }
	| T_FLOATCONSTANT
		{ $$ = slParseFloatConstant(Compiler, &$1); }
	| T_BOOLCONSTANT
		{ $$ = slParseBoolConstant(Compiler, &$1); }
	| '(' expression ')'
		{ $$ = $2; }
	;

postfix_expression :
	primary_expression
		{ $$ = $1; }
	| postfix_expression '[' integer_expression ']'
		{ $$ = slParseSubscriptExpr(Compiler, $1, $3); }
	| function_call
		{ $$ = slParseFuncCallExprAsExpr(Compiler, $1); }
	| postfix_expression '.' T_FIELD_SELECTION
		{ $$ = slParseFieldSelectionExpr(Compiler, $1, &$3); }
	| postfix_expression T_INC_OP
		{ $$ = slParseIncOrDecExpr(Compiler, gcvNULL, slvUNARY_POST_INC, $1); }
	| postfix_expression T_DEC_OP
		{ $$ = slParseIncOrDecExpr(Compiler, gcvNULL, slvUNARY_POST_DEC, $1); }
	;

integer_expression :
	expression
		{ $$ = $1; }
	;

function_call :
	function_call_generic
		{ $$ = $1; }
	;

function_call_generic :
	function_call_header_with_parameters ')'
		{ $$ = $1; }
	| function_call_header_no_parameters ')'
		{ $$ = $1; }
	;

function_call_header_no_parameters :
	function_call_header T_VOID
		{ $$ = $1; }
	| function_call_header
		{ $$ = $1; }
	;

function_call_header_with_parameters :
	function_call_header assignment_expression
		{ $$ = slParseFuncCallArgument(Compiler, $1, $2); }
	| function_call_header_with_parameters ',' assignment_expression
		{ $$ = slParseFuncCallArgument(Compiler, $1, $3); }
	;

function_call_header :
	function_identifier '('
		{ $$ = slParseFuncCallHeaderExpr(Compiler, &$1); }
	;

function_identifier :
	constructor_identifier
		{ $$ = $1; }
	| T_IDENTIFIER
		{ $$ = $1; }
	;

/* Grammar Note: Constructors look like functions, but lexical analysis recognized
	most of them as keywords. */

constructor_identifier :
	T_FLOAT
		{ $$ = $1; }
	| T_INT
		{ $$ = $1; }
	| T_BOOL
		{ $$ = $1; }
	| T_VEC2
		{ $$ = $1; }
	| T_VEC3
		{ $$ = $1; }
	| T_VEC4
		{ $$ = $1; }
	| T_BVEC2
		{ $$ = $1; }
	| T_BVEC3
		{ $$ = $1; }
	| T_BVEC4
		{ $$ = $1; }
	| T_IVEC2
		{ $$ = $1; }
	| T_IVEC3
		{ $$ = $1; }
	| T_IVEC4
		{ $$ = $1; }
	| T_MAT2
		{ $$ = $1; }
	| T_MAT3
		{ $$ = $1; }
	| T_MAT4
		{ $$ = $1; }
	| T_TYPE_NAME
		{ $$ = $1; }
	;

unary_expression :
	postfix_expression
		{ $$ = $1; }
	| T_INC_OP unary_expression
		{ $$ = slParseIncOrDecExpr(Compiler, &$1, slvUNARY_PRE_INC, $2); }
	| T_DEC_OP unary_expression
		{ $$ = slParseIncOrDecExpr(Compiler, &$1, slvUNARY_PRE_DEC, $2); }
	| unary_operator unary_expression
		{ $$ = slParseNormalUnaryExpr(Compiler, &$1, $2); }
	;

/* Grammar Note: No traditional style type casts. */

unary_operator:
	'+'
		{ $$ = $1; }
	| '-'
		{ $$ = $1; }
	| '!'
		{ $$ = $1; }
	| '~'	/* reserved */
		{ $$ = $1; }
	;

/* Grammar Note: No '*' or '&' unary ops. Pointers are not supported. */

multiplicative_expression :
	unary_expression
		{ $$ = $1; }
	| multiplicative_expression '*' unary_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	| multiplicative_expression '/' unary_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	| multiplicative_expression '%' unary_expression	/* reserved */
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

additive_expression :
	multiplicative_expression
		{ $$ = $1; }
	| additive_expression '+' multiplicative_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	| additive_expression '-' multiplicative_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

shift_expression :
	additive_expression
		{ $$ = $1; }
	| shift_expression T_LEFT_OP additive_expression	/* reserved */
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	| shift_expression T_RIGHT_OP additive_expression	/* reserved */
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

relational_expression :
	shift_expression
		{ $$ = $1; }
	| relational_expression '<' shift_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	| relational_expression '>' shift_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	| relational_expression T_LE_OP shift_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	| relational_expression T_GE_OP shift_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

equality_expression:
	relational_expression
		{ $$ = $1; }
	| equality_expression T_EQ_OP relational_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	| equality_expression T_NE_OP relational_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

and_expression :
	equality_expression
		{ $$ = $1; }
	| and_expression '&' equality_expression	/* reserved */
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

exclusive_or_expression :
	and_expression
		{ $$ = $1; }
	| exclusive_or_expression '^' and_expression	/* reserved */
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

inclusive_or_expression :
	exclusive_or_expression
		{ $$ = $1; }
	| inclusive_or_expression '|' exclusive_or_expression	/* reserved */
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

logical_and_expression :
	inclusive_or_expression
		{ $$ = $1; }
	| logical_and_expression T_AND_OP inclusive_or_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

logical_xor_expression :
	logical_and_expression
		{ $$ = $1; }
	| logical_xor_expression T_XOR_OP logical_and_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

logical_or_expression :
	logical_xor_expression
		{ $$ = $1; }
	| logical_or_expression T_OR_OP logical_xor_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

conditional_expression :
	logical_or_expression
		{ $$ = $1; }
	| logical_or_expression '?' expression ':' assignment_expression
		{ $$ = slParseSelectionExpr(Compiler, $1, $3, $5); }
	;

assignment_expression :
	conditional_expression
		{ $$ = $1; }
	| unary_expression assignment_operator assignment_expression
		{ $$ = slParseAssignmentExpr(Compiler, $1, &$2, $3); }
	;

assignment_operator:
	'='
		{ $$ = $1; }
	| T_MUL_ASSIGN
		{ $$ = $1; }
	| T_DIV_ASSIGN
		{ $$ = $1; }
	| T_MOD_ASSIGN		/* reserved */
		{ $$ = $1; }
	| T_ADD_ASSIGN
		{ $$ = $1; }
	| T_SUB_ASSIGN
		{ $$ = $1; }
	| T_LEFT_ASSIGN		/* reserved */
		{ $$ = $1; }
	| T_RIGHT_ASSIGN	/* reserved */
		{ $$ = $1; }
	| T_AND_ASSIGN		/* reserved */
		{ $$ = $1; }
	| T_XOR_ASSIGN		/* reserved */
		{ $$ = $1; }
	| T_OR_ASSIGN		/* reserved */
		{ $$ = $1; }
	;

expression :
	assignment_expression
		{ $$ = $1; }
	| expression ',' assignment_expression
		{ $$ = slParseNormalBinaryExpr(Compiler, $1, &$2, $3); }
	;

constant_expression :
	conditional_expression
		{ $$ = $1; }
	;

declaration :
	function_prototype ';'
		{ $$ = slParseFuncDecl(Compiler, $1); }
	| init_declarator_list ';'
		{ $$ = slParseDeclaration(Compiler, $1); }
	| T_PRECISION precision_qualifier type_specifier_no_prec ';'
		{ $$ = slParseDefaultPrecisionQualifier(Compiler, &$1, &$2, $3); }
	;

function_prototype :
	function_declarator ')'
		{ $$ = $1; }
	;

function_declarator :
	function_header
		{ $$ = $1; }
	| function_header_with_parameters
		{ $$ = $1; }
	;

function_header_with_parameters :
	function_header parameter_declaration
		{ $$ = slParseParameterList(Compiler, $1, $2); }
	| function_header_with_parameters ',' parameter_declaration
		{ $$ = slParseParameterList(Compiler, $1, $3); }
	;

function_header :
	fully_specified_type T_IDENTIFIER '('
		{ $$ = slParseFuncHeader(Compiler, $1, &$2); }
	;

parameter_declarator :
	type_specifier T_IDENTIFIER
		{ $$ = slParseNonArrayParameterDecl(Compiler, $1, &$2); }
	| type_specifier T_IDENTIFIER '[' constant_expression ']'
		{ $$ = slParseArrayParameterDecl(Compiler, $1, &$2, $4); }
	;

parameter_declaration :
	type_qualifier parameter_qualifier parameter_declarator
		{ $$ = slParseQualifiedParameterDecl(Compiler, &$1, &$2, $3); }
	| parameter_qualifier parameter_declarator
		{ $$ = slParseQualifiedParameterDecl(Compiler, gcvNULL, &$1, $2); }
	| type_qualifier parameter_qualifier parameter_type_specifier
		{ $$ = slParseQualifiedParameterDecl(Compiler, &$1, &$2, $3); }
	| parameter_qualifier parameter_type_specifier
		{ $$ = slParseQualifiedParameterDecl(Compiler, gcvNULL, &$1, $2); }
	;

parameter_qualifier :
	/* empty */
		{ $$ = slParseEmptyParameterQualifier(Compiler); }
	| T_IN
		{ $$ = $1; }
	| T_OUT
		{ $$ = $1; }
	| T_INOUT
		{ $$ = $1; }
	;

parameter_type_specifier :
	type_specifier
		{ $$ = slParseNonArrayParameterDecl(Compiler, $1, gcvNULL); }
	| type_specifier '[' constant_expression ']'
		{ $$ = slParseArrayParameterDecl(Compiler, $1, gcvNULL, $3); }
	;

init_declarator_list :
	single_declaration
		{ $$ = $1; }
	| init_declarator_list ',' T_IDENTIFIER
		{ $$ = slParseNonArrayVariableDecl2(Compiler, $1, &$3); }
	| init_declarator_list ',' T_IDENTIFIER '[' constant_expression ']'
		{ $$ = slParseArrayVariableDecl2(Compiler, $1, &$3, $5); }
	| init_declarator_list ',' T_IDENTIFIER '=' initializer
		{ $$ = slParseVariableDeclWithInitializer2(Compiler, $1, &$3, $5); }
	;

single_declaration :
	fully_specified_type
		{ $$ = slParseTypeDecl(Compiler, $1); }
	| fully_specified_type T_IDENTIFIER
		{ $$ = slParseNonArrayVariableDecl(Compiler, $1, &$2); }
	| fully_specified_type T_IDENTIFIER '[' constant_expression ']'
		{ $$ = slParseArrayVariableDecl(Compiler, $1, &$2, $4); }
	| fully_specified_type T_IDENTIFIER '=' initializer
		{ $$ = slParseVariableDeclWithInitializer(Compiler, $1, &$2, $4); }
	| T_INVARIANT T_IDENTIFIER		/* Vertex only. */
		{ $$ = slParseInvariantDecl(Compiler, &$1, &$2); }
	;

/* Grammar Note: No 'enum', or 'typedef'. */

fully_specified_type :
	type_specifier
		{ $$ = $1; }
	| type_qualifier type_specifier
		{ $$ = slParseFullySpecifiedType(Compiler, &$1, $2); }
	;

type_qualifier :
	T_CONST
		{ $$ = $1; }
	| T_ATTRIBUTE		/* Vertex only. */
		{ $$ = $1; }
	| T_VARYING
		{ $$ = $1; }
	| T_INVARIANT T_VARYING
		{ $$ = $1; }
	| T_UNIFORM
		{ $$ = $1; }
	;

type_specifier :
	type_specifier_no_prec
		{ $$ = $1; }
	| precision_qualifier type_specifier_no_prec
		{ $$ = slParseTypeSpecifier(Compiler, &$1, $2); }
	;

type_specifier_no_prec:
	T_VOID
		{ $$ = slParseNonStructType(Compiler, &$1, T_VOID); }
	| T_FLOAT
		{ $$ = slParseNonStructType(Compiler, &$1, T_FLOAT); }
	| T_INT
		{ $$ = slParseNonStructType(Compiler, &$1, T_INT); }
	| T_UINT
		{ $$ = slParseNonStructType(Compiler, &$1, T_UINT); }
	| T_BOOL
		{ $$ = slParseNonStructType(Compiler, &$1, T_BOOL); }
	| T_VEC2
		{ $$ = slParseNonStructType(Compiler, &$1, T_VEC2); }
	| T_VEC3
		{ $$ = slParseNonStructType(Compiler, &$1, T_VEC3); }
	| T_VEC4
		{ $$ = slParseNonStructType(Compiler, &$1, T_VEC4); }
	| T_BVEC2
		{ $$ = slParseNonStructType(Compiler, &$1, T_BVEC2); }
	| T_BVEC3
		{ $$ = slParseNonStructType(Compiler, &$1, T_BVEC3); }
	| T_BVEC4
		{ $$ = slParseNonStructType(Compiler, &$1, T_BVEC4); }
	| T_IVEC2
		{ $$ = slParseNonStructType(Compiler, &$1, T_IVEC2); }
	| T_IVEC3
		{ $$ = slParseNonStructType(Compiler, &$1, T_IVEC3); }
	| T_IVEC4
		{ $$ = slParseNonStructType(Compiler, &$1, T_IVEC4); }
	| T_UVEC2
		{ $$ = slParseNonStructType(Compiler, &$1, T_UVEC2); }
	| T_UVEC3
		{ $$ = slParseNonStructType(Compiler, &$1, T_UVEC3); }
	| T_UVEC4
		{ $$ = slParseNonStructType(Compiler, &$1, T_UVEC4); }
	| T_MAT2
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT2); }
	| T_MAT2X3
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT2X3); }
	| T_MAT2X4
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT2X4); }
	| T_MAT3
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT3); }
	| T_MAT3X2
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT3X2); }
	| T_MAT3X4
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT3X4); }
	| T_MAT4
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT4); }
	| T_MAT4X2
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT4X2); }
	| T_MAT4X3
		{ $$ = slParseNonStructType(Compiler, &$1, T_MAT4X3); }
	| T_SAMPLER2D
		{ $$ = slParseNonStructType(Compiler, &$1, T_SAMPLER2D); }
	| T_SAMPLERCUBE
		{ $$ = slParseNonStructType(Compiler, &$1, T_SAMPLERCUBE); }
	| struct_specifier
		{ $$ = slParseStructType(Compiler, $1); }
	| T_TYPE_NAME
		{ $$ = slParseNamedType(Compiler, &$1); }
	| T_SAMPLER3D
		{ $$ = slParseNonStructType(Compiler, &$1, T_SAMPLER3D); }
	| T_SAMPLER1DARRAY
		{ $$ = slParseNonStructType(Compiler, &$1, T_SAMPLER1DARRAY); }
	| T_SAMPLER2DARRAY
		{ $$ = slParseNonStructType(Compiler, &$1, T_SAMPLER2DARRAY); }
	| T_SAMPLER1DARRAYSHADOW
		{ $$ = slParseNonStructType(Compiler, &$1, T_SAMPLER1DARRAYSHADOW); }
	| T_SAMPLER2DARRAYSHADOW
		{ $$ = slParseNonStructType(Compiler, &$1, T_SAMPLER2DARRAYSHADOW); }
	| T_ISAMPLER2D
		{ $$ = slParseNonStructType(Compiler, &$1, T_ISAMPLER2D); }
	| T_ISAMPLERCUBE
		{ $$ = slParseNonStructType(Compiler, &$1, T_ISAMPLERCUBE); }
	| T_ISAMPLER3D
		{ $$ = slParseNonStructType(Compiler, &$1, T_ISAMPLER3D); }
	| T_ISAMPLER2DARRAY
		{ $$ = slParseNonStructType(Compiler, &$1, T_ISAMPLER2DARRAY); }
	| T_USAMPLER2D
		{ $$ = slParseNonStructType(Compiler, &$1, T_USAMPLER2D); }
	| T_USAMPLERCUBE
		{ $$ = slParseNonStructType(Compiler, &$1, T_USAMPLERCUBE); }
	| T_USAMPLER3D
		{ $$ = slParseNonStructType(Compiler, &$1, T_USAMPLER3D); }
	| T_USAMPLER2DARRAY
		{ $$ = slParseNonStructType(Compiler, &$1, T_USAMPLER2DARRAY); }
	| T_SAMPLEREXTERNALOES
		{ $$ = slParseNonStructType(Compiler, &$1, T_SAMPLEREXTERNALOES); }
	;

precision_qualifier :
	T_HIGH_PRECISION
		{ $$ = $1; }
	| T_MEDIUM_PRECISION
		{ $$ = $1; }
	| T_LOW_PRECISION
		{ $$ = $1; }
	;

struct_specifier:
	T_STRUCT T_IDENTIFIER '{'
			{ slParseStructDeclBegin(Compiler); }
		struct_declaration_list '}'
		{ $$ = slParseStructDeclEnd(Compiler, &$1, &$2); }
	| T_STRUCT '{'
			{ slParseStructDeclBegin(Compiler); }
		struct_declaration_list '}'
		{ $$ = slParseStructDeclEnd(Compiler, &$1, gcvNULL); }
	;

struct_declaration_list :
	struct_declaration
	| struct_declaration_list struct_declaration
	;

struct_declaration :
	type_specifier struct_declarator_list ';'
		{ slParseTypeSpecifiedFieldDeclList(Compiler, $1, $2); }
	;

struct_declarator_list :
	struct_declarator
		{ $$ = slParseFieldDeclList(Compiler, $1); }
	| struct_declarator_list ',' struct_declarator
		{ $$ = slParseFieldDeclList2(Compiler, $1, $3); }
	;

struct_declarator :
	T_IDENTIFIER
		{ $$ = slParseFieldDecl(Compiler, &$1, gcvNULL); }
	| T_IDENTIFIER '[' constant_expression ']'
		{ $$ = slParseFieldDecl(Compiler, &$1, $3); }
	;

initializer :
	assignment_expression
		{ $$ = $1; }
	;

declaration_statement :
	declaration
		{ $$ = $1; }
	;

statement :
	compound_statement
		{ $$ = slParseCompoundStatementAsStatement(Compiler, $1); }
	| simple_statement
		{ $$ = $1; }
	;

/* Grammar Note: No labeled statements; 'goto' is not supported. */

simple_statement :
	declaration_statement
		{ $$ = $1; }
	| expression_statement
		{ $$ = $1; }
	| selection_statement
		{ $$ = $1; }
	| iteration_statement
		{ $$ = $1; }
	| jump_statement
		{ $$ = $1; }
	;

compound_statement :
	'{' '}'
		{ $$ = gcvNULL; }
	| '{'
			{ slParseCompoundStatementBegin(Compiler); }
		statement_list '}'
		{ $$ = slParseCompoundStatementEnd(Compiler, &$1, $3); }
	;

statement_no_new_scope :
	compound_statement_no_new_scope
		{ $$ = slParseCompoundStatementNoNewScopeAsStatementNoNewScope(Compiler, $1); }
	| simple_statement
		{ $$ = $1; }
	;

compound_statement_no_new_scope :
	'{' '}'
		{ $$ = gcvNULL; }
	| '{'
			{ slParseCompoundStatementNoNewScopeBegin(Compiler); }
		statement_list '}'
		{ $$ = slParseCompoundStatementNoNewScopeEnd(Compiler, &$1, $3); }
	;

statement_list :
	statement
		{ $$ = slParseStatementList(Compiler, $1); }
	| statement_list statement
		{ $$ = slParseStatementList2(Compiler, $1, $2); }
	;

expression_statement :
	';'
		{ $$ = gcvNULL; }
	| expression ';'
		{ $$ = slParseExprAsStatement(Compiler, $1); }
	;

selection_statement :
	T_IF '(' expression ')' selection_rest_statement
		{ $$ = slParseSelectionStatement(Compiler, &$1, $3, $5); }
	;

selection_rest_statement :
	statement T_ELSE statement
		{ $$ = slParseSelectionRestStatement(Compiler, $1, $3); }
	| statement						%prec T_IF
		{ $$ = slParseSelectionRestStatement(Compiler, $1, gcvNULL); }
	;

/* Grammar Note: No 'switch'. Switch statements not supported. */

condition :
	expression
		{ $$ = $1; }
	| fully_specified_type T_IDENTIFIER '=' initializer
		{ $$ = slParseCondition(Compiler, $1, &$2, $4); }
	;

iteration_statement :
	T_WHILE
			{ slParseWhileStatementBegin(Compiler); }
		'(' condition ')' statement_no_new_scope
		{ $$ = slParseWhileStatementEnd(Compiler, &$1, $4, $6); }
	| T_DO statement T_WHILE '(' expression ')' ';'
		{ $$ = slParseDoWhileStatement(Compiler, &$1, $2, $5); }
	| T_FOR
			{ slParseForStatementBegin(Compiler); }
		'(' for_init_statement for_rest_statement ')' statement_no_new_scope
		{ $$ = slParseForStatementEnd(Compiler, &$1, $4, $5, $7); }
	;

for_init_statement :
	expression_statement
		{ $$ = $1; }
	| declaration_statement
		{ $$ = $1; }
	;

conditionopt :
	condition
		{ $$ = $1; }
	| /* empty */
		{ $$ = gcvNULL; }
	;

for_rest_statement :
	conditionopt ';'
		{ $$ = slParseForRestStatement(Compiler, $1, gcvNULL); }
	| conditionopt ';' expression
		{ $$ = slParseForRestStatement(Compiler, $1, $3); }
	;

jump_statement :
	T_CONTINUE ';'
		{ $$ = slParseJumpStatement(Compiler, slvCONTINUE, &$1, gcvNULL); }
	| T_BREAK ';'
		{ $$ = slParseJumpStatement(Compiler, slvBREAK, &$1, gcvNULL); }
	| T_RETURN ';'
		{ $$ = slParseJumpStatement(Compiler, slvRETURN, &$1, gcvNULL); }
	| T_RETURN expression ';'
		{ $$ = slParseJumpStatement(Compiler, slvRETURN, &$1, $2); }
	| T_DISCARD ';'		/* Fragment shader only. */
		{ $$ = slParseJumpStatement(Compiler, slvDISCARD, &$1, gcvNULL); }
	;

/* Grammar Note: No 'goto'. Gotos are not supported. */

translation_unit :
	external_declaration
	| translation_unit external_declaration
	;

external_declaration :
	function_definition
	| declaration
		{ slParseExternalDecl(Compiler, $1); }
	;

function_definition :
	function_prototype compound_statement_no_new_scope
		{ slParseFuncDef(Compiler, $1, $2); }
	;

%%

