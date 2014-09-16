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


#ifndef __gc_glsl_parser_h_
#define __gc_glsl_parser_h_

#include "gc_glsl_scanner.h"
#include "gc_glsl_built_ins.h"

sloIR_EXPR
slParseVariableIdentifier(
	IN sloCOMPILER Compiler,
	IN slsLexToken * Identifier
	);

sloIR_EXPR
slParseIntConstant(
	IN sloCOMPILER Compiler,
	IN slsLexToken * IntConstant
	);

sloIR_EXPR
slParseFloatConstant(
	IN sloCOMPILER Compiler,
	IN slsLexToken * FloatConstant
	);

sloIR_EXPR
slParseBoolConstant(
	IN sloCOMPILER Compiler,
	IN slsLexToken * BoolConstant
	);

sloIR_EXPR
slParseSubscriptExpr(
	IN sloCOMPILER Compiler,
	IN sloIR_EXPR LeftOperand,
	IN sloIR_EXPR RightOperand
	);

sloIR_EXPR
slParseFuncCallExprAsExpr(
	IN sloCOMPILER Compiler,
	IN sloIR_POLYNARY_EXPR FuncCall
	);

sloIR_POLYNARY_EXPR
slParseFuncCallHeaderExpr(
	IN sloCOMPILER Compiler,
	IN slsLexToken * FuncIdentifier
	);

sloIR_POLYNARY_EXPR
slParseFuncCallArgument(
	IN sloCOMPILER Compiler,
	IN sloIR_POLYNARY_EXPR FuncCall,
	IN sloIR_EXPR Argument
	);

sloIR_EXPR
slParseFieldSelectionExpr(
	IN sloCOMPILER Compiler,
	IN sloIR_EXPR Operand,
	IN slsLexToken * FieldSelection
	);

sloIR_EXPR
slParseIncOrDecExpr(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN sleUNARY_EXPR_TYPE ExprType,
	IN sloIR_EXPR Operand
	);

sloIR_EXPR
slParseNormalUnaryExpr(
	IN sloCOMPILER Compiler,
	IN slsLexToken * Operator,
	IN sloIR_EXPR Operand
	);

sloIR_EXPR
slParseNormalBinaryExpr(
	IN sloCOMPILER Compiler,
	IN sloIR_EXPR LeftOperand,
	IN slsLexToken * Operator,
	IN sloIR_EXPR RightOperand
	);

sloIR_EXPR
slParseSelectionExpr(
	IN sloCOMPILER Compiler,
	IN sloIR_EXPR CondExpr,
	IN sloIR_EXPR TrueOperand,
	IN sloIR_EXPR FalseOperand
	);

sloIR_EXPR
slParseAssignmentExpr(
	IN sloCOMPILER Compiler,
	IN sloIR_EXPR LeftOperand,
	IN slsLexToken * Operator,
	IN sloIR_EXPR RightOperand
	);

slsDeclOrDeclList
slParseNonArrayVariableDecl(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType,
	IN slsLexToken * Identifier
	);

slsDeclOrDeclList
slParseArrayVariableDecl(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType,
	IN slsLexToken * Identifier,
	IN sloIR_EXPR ArrayLengthExpr
	);

slsDeclOrDeclList
slParseNonArrayVariableDecl2(
	IN sloCOMPILER Compiler,
	IN slsDeclOrDeclList DeclOrDeclList,
	IN slsLexToken * Identifier
	);

slsDeclOrDeclList
slParseArrayVariableDecl2(
	IN sloCOMPILER Compiler,
	IN slsDeclOrDeclList DeclOrDeclList,
	IN slsLexToken * Identifier,
	IN sloIR_EXPR ArrayLengthExpr
	);

slsDeclOrDeclList
slParseVariableDeclWithInitializer(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType,
	IN slsLexToken * Identifier,
	IN sloIR_EXPR Initializer
	);

slsDeclOrDeclList
slParseVariableDeclWithInitializer2(
	IN sloCOMPILER Compiler,
	IN slsDeclOrDeclList DeclOrDeclList,
	IN slsLexToken * Identifier,
	IN sloIR_EXPR Initializer
	);

slsNAME *
slParseFuncHeader(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType,
	IN slsLexToken * Identifier
	);

sloIR_BASE
slParseFuncDecl(
	IN sloCOMPILER Compiler,
	IN slsNAME * FuncName
	);

sloIR_BASE
slParseDeclaration(
	IN sloCOMPILER Compiler,
	IN slsDeclOrDeclList DeclOrDeclList
	);

sloIR_BASE
slParseDefaultPrecisionQualifier(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN slsLexToken * PrecisionQualifier,
	IN slsDATA_TYPE * DataType
	);

void
slParseCompoundStatementBegin(
	IN sloCOMPILER Compiler
	);

sloIR_SET
slParseCompoundStatementEnd(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN sloIR_SET Set
	);

void
slParseCompoundStatementNoNewScopeBegin(
	IN sloCOMPILER Compiler
	);

sloIR_SET
slParseCompoundStatementNoNewScopeEnd(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN sloIR_SET Set
	);

sloIR_SET
slParseStatementList(
	IN sloCOMPILER Compiler,
	IN sloIR_BASE Statement
	);

sloIR_SET
slParseStatementList2(
	IN sloCOMPILER Compiler,
	IN sloIR_SET Set,
	IN sloIR_BASE Statement
	);

sloIR_BASE
slParseExprAsStatement(
	IN sloCOMPILER Compiler,
	IN sloIR_EXPR Expr
	);

sloIR_BASE
slParseCompoundStatementAsStatement(
	IN sloCOMPILER Compiler,
	IN sloIR_SET CompoundStatement
	);

sloIR_BASE
slParseCompoundStatementNoNewScopeAsStatementNoNewScope(
	IN sloCOMPILER Compiler,
	IN sloIR_SET CompoundStatementNoNewScope
	);

slsSelectionStatementPair
slParseSelectionRestStatement(
	IN sloCOMPILER Compiler,
	IN sloIR_BASE TrueStatement,
	IN sloIR_BASE FalseStatement
	);

sloIR_BASE
slParseSelectionStatement(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN sloIR_EXPR CondExpr,
	IN slsSelectionStatementPair SelectionStatementPair
	);

void
slParseWhileStatementBegin(
	IN sloCOMPILER Compiler
	);

sloIR_BASE
slParseWhileStatementEnd(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN sloIR_EXPR CondExpr,
	IN sloIR_BASE LoopBody
	);

sloIR_BASE
slParseDoWhileStatement(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN sloIR_BASE LoopBody,
	IN sloIR_EXPR CondExpr
	);

void
slParseForStatementBegin(
	IN sloCOMPILER Compiler
	);

sloIR_BASE
slParseForStatementEnd(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN sloIR_BASE ForInitStatement,
	IN slsForExprPair ForExprPair,
	IN sloIR_BASE LoopBody
	);

sloIR_EXPR
slParseCondition(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType,
	IN slsLexToken * Identifier,
	IN sloIR_EXPR Initializer
	);

slsForExprPair
slParseForRestStatement(
	IN sloCOMPILER Compiler,
	IN sloIR_EXPR CondExpr,
	IN sloIR_EXPR RestExpr
	);

sloIR_BASE
slParseJumpStatement(
	IN sloCOMPILER Compiler,
	IN sleJUMP_TYPE Type,
	IN slsLexToken * StartToken,
	IN sloIR_EXPR ReturnExpr
	);

void
slParseExternalDecl(
	IN sloCOMPILER Compiler,
	IN sloIR_BASE Decl
	);

void
slParseFuncDef(
	IN sloCOMPILER Compiler,
	IN slsNAME * FuncName,
	IN sloIR_SET Statements
	);

slsNAME	*
slParseParameterList(
	IN sloCOMPILER Compiler,
	IN slsNAME * FuncName,
	IN slsNAME * ParamName
	);

slsDeclOrDeclList
slParseTypeDecl(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType
	);

slsDeclOrDeclList
slParseInvariantDecl(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN slsLexToken * Identifier
	);

slsNAME	*
slParseNonArrayParameterDecl(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType,
	IN slsLexToken * Identifier
	);

slsNAME	*
slParseArrayParameterDecl(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType,
	IN slsLexToken * Identifier,
	IN sloIR_EXPR ArrayLengthExpr
	);

slsLexToken
slParseEmptyParameterQualifier(
	IN sloCOMPILER Compiler
	);

slsNAME	*
slParseQualifiedParameterDecl(
	IN sloCOMPILER Compiler,
	IN slsLexToken * TypeQualifier,
	IN slsLexToken * ParameterQualifier,
	IN slsNAME * ParameterDecl
	);

slsDATA_TYPE *
slParseFullySpecifiedType(
	IN sloCOMPILER Compiler,
	IN slsLexToken * TypeQualifier,
	IN slsDATA_TYPE * DataType
	);

slsDATA_TYPE *
slParseTypeSpecifier(
	IN sloCOMPILER Compiler,
	IN slsLexToken * PrecisionQualifier,
	IN slsDATA_TYPE * DataType
	);

slsDATA_TYPE *
slParseNonStructType(
	IN sloCOMPILER Compiler,
	IN slsLexToken * Token,
	IN gctINT TokenType
	);

slsDATA_TYPE *
slParseStructType(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * StructType
	);

slsDATA_TYPE *
slParseNamedType(
	IN sloCOMPILER Compiler,
	IN slsLexToken * TypeName
	);

void
slParseStructDeclBegin(
	IN sloCOMPILER Compiler
	);

slsDATA_TYPE *
slParseStructDeclEnd(
	IN sloCOMPILER Compiler,
	IN slsLexToken * StartToken,
	IN slsLexToken * Identifier
	);

void
slParseTypeSpecifiedFieldDeclList(
	IN sloCOMPILER Compiler,
	IN slsDATA_TYPE * DataType,
	IN slsDLINK_LIST * FieldDeclList
	);

slsDLINK_LIST *
slParseFieldDeclList(
	IN sloCOMPILER Compiler,
	IN slsFieldDecl * FieldDecl
	);

slsDLINK_LIST *
slParseFieldDeclList2(
	IN sloCOMPILER Compiler,
	IN slsDLINK_LIST * FieldDeclList,
	IN slsFieldDecl * FieldDecl
	);

slsFieldDecl *
slParseFieldDecl(
	IN sloCOMPILER Compiler,
	IN slsLexToken * Identifier,
	IN sloIR_EXPR ArrayLengthExpr
	);

#endif /* __gc_glsl_parser_h_ */
