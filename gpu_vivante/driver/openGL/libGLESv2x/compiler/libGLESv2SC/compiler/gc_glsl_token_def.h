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


#ifndef __gc_glsl_token_def_h_
#define __gc_glsl_token_def_h_
/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_ATTRIBUTE = 258,
     T_CONST = 259,
     T_BOOL = 260,
     T_FLOAT = 261,
     T_INT = 262,
     T_BREAK = 263,
     T_CONTINUE = 264,
     T_DO = 265,
     T_ELSE = 266,
     T_FOR = 267,
     T_IF = 268,
     T_DISCARD = 269,
     T_RETURN = 270,
     T_BVEC2 = 271,
     T_BVEC3 = 272,
     T_BVEC4 = 273,
     T_IVEC2 = 274,
     T_IVEC3 = 275,
     T_IVEC4 = 276,
     T_VEC2 = 277,
     T_VEC3 = 278,
     T_VEC4 = 279,
     T_MAT2 = 280,
     T_MAT3 = 281,
     T_MAT4 = 282,
     T_IN = 283,
     T_OUT = 284,
     T_INOUT = 285,
     T_UNIFORM = 286,
     T_VARYING = 287,
     T_UINT = 288,
     T_UVEC2 = 289,
     T_UVEC3 = 290,
     T_UVEC4 = 291,
     T_MAT2X3 = 292,
     T_MAT2X4 = 293,
     T_MAT3X2 = 294,
     T_MAT3X4 = 295,
     T_MAT4X2 = 296,
     T_MAT4X3 = 297,
     T_SAMPLER2D = 298,
     T_SAMPLERCUBE = 299,
     T_SAMPLER3D = 300,
     T_SAMPLER1DARRAY = 301,
     T_SAMPLER2DARRAY = 302,
     T_SAMPLER1DARRAYSHADOW = 303,
     T_SAMPLER2DARRAYSHADOW = 304,
     T_ISAMPLER2D = 305,
     T_ISAMPLERCUBE = 306,
     T_ISAMPLER3D = 307,
     T_ISAMPLER2DARRAY = 308,
     T_USAMPLER2D = 309,
     T_USAMPLERCUBE = 310,
     T_USAMPLER3D = 311,
     T_USAMPLER2DARRAY = 312,
     T_SAMPLEREXTERNALOES = 313,
     T_STRUCT = 314,
     T_VOID = 315,
     T_WHILE = 316,
     T_IDENTIFIER = 317,
     T_TYPE_NAME = 318,
     T_FLOATCONSTANT = 319,
     T_INTCONSTANT = 320,
     T_BOOLCONSTANT = 321,
     T_FIELD_SELECTION = 322,
     T_LEFT_OP = 323,
     T_RIGHT_OP = 324,
     T_INC_OP = 325,
     T_DEC_OP = 326,
     T_LE_OP = 327,
     T_GE_OP = 328,
     T_EQ_OP = 329,
     T_NE_OP = 330,
     T_AND_OP = 331,
     T_OR_OP = 332,
     T_XOR_OP = 333,
     T_MUL_ASSIGN = 334,
     T_DIV_ASSIGN = 335,
     T_ADD_ASSIGN = 336,
     T_MOD_ASSIGN = 337,
     T_LEFT_ASSIGN = 338,
     T_RIGHT_ASSIGN = 339,
     T_AND_ASSIGN = 340,
     T_XOR_ASSIGN = 341,
     T_OR_ASSIGN = 342,
     T_SUB_ASSIGN = 343,
     T_INVARIANT = 344,
     T_HIGH_PRECISION = 345,
     T_MEDIUM_PRECISION = 346,
     T_LOW_PRECISION = 347,
     T_PRECISION = 348
   };
#endif
#define T_ATTRIBUTE 258
#define T_CONST 259
#define T_BOOL 260
#define T_FLOAT 261
#define T_INT 262
#define T_BREAK 263
#define T_CONTINUE 264
#define T_DO 265
#define T_ELSE 266
#define T_FOR 267
#define T_IF 268
#define T_DISCARD 269
#define T_RETURN 270
#define T_BVEC2 271
#define T_BVEC3 272
#define T_BVEC4 273
#define T_IVEC2 274
#define T_IVEC3 275
#define T_IVEC4 276
#define T_VEC2 277
#define T_VEC3 278
#define T_VEC4 279
#define T_MAT2 280
#define T_MAT3 281
#define T_MAT4 282
#define T_IN 283
#define T_OUT 284
#define T_INOUT 285
#define T_UNIFORM 286
#define T_VARYING 287
#define T_UINT 288
#define T_UVEC2 289
#define T_UVEC3 290
#define T_UVEC4 291
#define T_MAT2X3 292
#define T_MAT2X4 293
#define T_MAT3X2 294
#define T_MAT3X4 295
#define T_MAT4X2 296
#define T_MAT4X3 297
#define T_SAMPLER2D 298
#define T_SAMPLERCUBE 299
#define T_SAMPLER3D 300
#define T_SAMPLER1DARRAY 301
#define T_SAMPLER2DARRAY 302
#define T_SAMPLER1DARRAYSHADOW 303
#define T_SAMPLER2DARRAYSHADOW 304
#define T_ISAMPLER2D 305
#define T_ISAMPLERCUBE 306
#define T_ISAMPLER3D 307
#define T_ISAMPLER2DARRAY 308
#define T_USAMPLER2D 309
#define T_USAMPLERCUBE 310
#define T_USAMPLER3D 311
#define T_USAMPLER2DARRAY 312
#define T_SAMPLEREXTERNALOES 313
#define T_STRUCT 314
#define T_VOID 315
#define T_WHILE 316
#define T_IDENTIFIER 317
#define T_TYPE_NAME 318
#define T_FLOATCONSTANT 319
#define T_INTCONSTANT 320
#define T_BOOLCONSTANT 321
#define T_FIELD_SELECTION 322
#define T_LEFT_OP 323
#define T_RIGHT_OP 324
#define T_INC_OP 325
#define T_DEC_OP 326
#define T_LE_OP 327
#define T_GE_OP 328
#define T_EQ_OP 329
#define T_NE_OP 330
#define T_AND_OP 331
#define T_OR_OP 332
#define T_XOR_OP 333
#define T_MUL_ASSIGN 334
#define T_DIV_ASSIGN 335
#define T_ADD_ASSIGN 336
#define T_MOD_ASSIGN 337
#define T_LEFT_ASSIGN 338
#define T_RIGHT_ASSIGN 339
#define T_AND_ASSIGN 340
#define T_XOR_ASSIGN 341
#define T_OR_ASSIGN 342
#define T_SUB_ASSIGN 343
#define T_INVARIANT 344
#define T_HIGH_PRECISION 345
#define T_MEDIUM_PRECISION 346
#define T_LOW_PRECISION 347
#define T_PRECISION 348




#if !defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 25 "gc_glsl.y"
typedef union YYSTYPE {
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
} YYSTYPE;
/* Line 1275 of yacc.c.  */
#line 251 "gc_glsl_token_def.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





#endif /* __gc_glsl_token_def_h_ */
