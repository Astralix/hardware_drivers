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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 1 "gc_glsl.y"

#include "gc_glsl_parser.h"
#ifndef FILE
#define FILE		void
#endif

#ifndef stderr
#define stderr		gcvNULL
#endif


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


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

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
/* Line 191 of yacc.c.  */
#line 311 "gc_glsl_parser.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 323 "gc_glsl_parser.c"

#if !defined (yyoverflow) || YYERROR_VERBOSE

#ifndef YYFREE
#  define YYFREE slFree
# endif
#ifndef YYMALLOC
#  define YYMALLOC slMalloc
# endif

/* The parser invokes alloca or malloc; define the necessary symbols.  */

#ifdef YYSTACK_USE_ALLOCA
#if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC slMalloc
#  endif
# else
#if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC slMalloc
#  else
#ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

#ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#if defined (__STDC__) || defined (__cplusplus)
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
#ifndef YYCOPY
#if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  92
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1494

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  118
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  77
/* YYNRULES -- Number of rules. */
#define YYNRULES  240
/* YYNRULES -- Number of states. */
#define YYNSTATES  363

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   348

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   105,     2,     2,     2,   111,   116,     2,
      94,    95,   109,   108,   101,   106,   100,   110,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   102,   104,
     112,   103,   113,   117,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    96,     2,    97,   115,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    98,   114,    99,   107,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    17,    19,
      24,    26,    30,    33,    36,    38,    40,    43,    46,    49,
      51,    54,    58,    61,    63,    65,    67,    69,    71,    73,
      75,    77,    79,    81,    83,    85,    87,    89,    91,    93,
      95,    97,    99,   102,   105,   108,   110,   112,   114,   116,
     118,   122,   126,   130,   132,   136,   140,   142,   146,   150,
     152,   156,   160,   164,   168,   170,   174,   178,   180,   184,
     186,   190,   192,   196,   198,   202,   204,   208,   210,   214,
     216,   222,   224,   228,   230,   232,   234,   236,   238,   240,
     242,   244,   246,   248,   250,   252,   256,   258,   261,   264,
     269,   272,   274,   276,   279,   283,   287,   290,   296,   300,
     303,   307,   310,   311,   313,   315,   317,   319,   324,   326,
     330,   337,   343,   345,   348,   354,   359,   362,   364,   367,
     369,   371,   373,   376,   378,   380,   383,   385,   387,   389,
     391,   393,   395,   397,   399,   401,   403,   405,   407,   409,
     411,   413,   415,   417,   419,   421,   423,   425,   427,   429,
     431,   433,   435,   437,   439,   441,   443,   445,   447,   449,
     451,   453,   455,   457,   459,   461,   463,   465,   467,   469,
     471,   473,   475,   477,   478,   485,   486,   492,   494,   497,
     501,   503,   507,   509,   514,   516,   518,   520,   522,   524,
     526,   528,   530,   532,   535,   536,   541,   543,   545,   548,
     549,   554,   556,   559,   561,   564,   570,   574,   576,   578,
     583,   584,   591,   599,   600,   608,   610,   612,   614,   615,
     618,   622,   625,   628,   631,   635,   638,   640,   643,   645,
     647
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
     192,     0,    -1,    62,    -1,   119,    -1,    65,    -1,    64,
      -1,    66,    -1,    94,   146,    95,    -1,   120,    -1,   121,
      96,   122,    97,    -1,   123,    -1,   121,   100,    67,    -1,
     121,    70,    -1,   121,    71,    -1,   146,    -1,   124,    -1,
     126,    95,    -1,   125,    95,    -1,   127,    60,    -1,   127,
      -1,   127,   144,    -1,   126,   101,   144,    -1,   128,    94,
      -1,   129,    -1,    62,    -1,     6,    -1,     7,    -1,     5,
      -1,    22,    -1,    23,    -1,    24,    -1,    16,    -1,    17,
      -1,    18,    -1,    19,    -1,    20,    -1,    21,    -1,    25,
      -1,    26,    -1,    27,    -1,    63,    -1,   121,    -1,    70,
     130,    -1,    71,   130,    -1,   131,   130,    -1,   108,    -1,
     106,    -1,   105,    -1,   107,    -1,   130,    -1,   132,   109,
     130,    -1,   132,   110,   130,    -1,   132,   111,   130,    -1,
     132,    -1,   133,   108,   132,    -1,   133,   106,   132,    -1,
     133,    -1,   134,    68,   133,    -1,   134,    69,   133,    -1,
     134,    -1,   135,   112,   134,    -1,   135,   113,   134,    -1,
     135,    72,   134,    -1,   135,    73,   134,    -1,   135,    -1,
     136,    74,   135,    -1,   136,    75,   135,    -1,   136,    -1,
     137,   116,   136,    -1,   137,    -1,   138,   115,   137,    -1,
     138,    -1,   139,   114,   138,    -1,   139,    -1,   140,    76,
     139,    -1,   140,    -1,   141,    78,   140,    -1,   141,    -1,
     142,    77,   141,    -1,   142,    -1,   142,   117,   146,   102,
     144,    -1,   143,    -1,   130,   145,   144,    -1,   103,    -1,
      79,    -1,    80,    -1,    82,    -1,    81,    -1,    88,    -1,
      83,    -1,    84,    -1,    85,    -1,    86,    -1,    87,    -1,
     144,    -1,   146,   101,   144,    -1,   143,    -1,   149,   104,
      -1,   157,   104,    -1,    93,   163,   162,   104,    -1,   150,
      95,    -1,   152,    -1,   151,    -1,   152,   154,    -1,   151,
     101,   154,    -1,   159,    62,    94,    -1,   161,    62,    -1,
     161,    62,    96,   147,    97,    -1,   160,   155,   153,    -1,
     155,   153,    -1,   160,   155,   156,    -1,   155,   156,    -1,
      -1,    28,    -1,    29,    -1,    30,    -1,   161,    -1,   161,
      96,   147,    97,    -1,   158,    -1,   157,   101,    62,    -1,
     157,   101,    62,    96,   147,    97,    -1,   157,   101,    62,
     103,   171,    -1,   159,    -1,   159,    62,    -1,   159,    62,
      96,   147,    97,    -1,   159,    62,   103,   171,    -1,    89,
      62,    -1,   161,    -1,   160,   161,    -1,     4,    -1,     3,
      -1,    32,    -1,    89,    32,    -1,    31,    -1,   162,    -1,
     163,   162,    -1,    60,    -1,     6,    -1,     7,    -1,    33,
      -1,     5,    -1,    22,    -1,    23,    -1,    24,    -1,    16,
      -1,    17,    -1,    18,    -1,    19,    -1,    20,    -1,    21,
      -1,    34,    -1,    35,    -1,    36,    -1,    25,    -1,    37,
      -1,    38,    -1,    26,    -1,    39,    -1,    40,    -1,    27,
      -1,    41,    -1,    42,    -1,    43,    -1,    44,    -1,   164,
      -1,    63,    -1,    45,    -1,    46,    -1,    47,    -1,    48,
      -1,    49,    -1,    50,    -1,    51,    -1,    52,    -1,    53,
      -1,    54,    -1,    55,    -1,    56,    -1,    57,    -1,    58,
      -1,    90,    -1,    91,    -1,    92,    -1,    -1,    59,    62,
      98,   165,   167,    99,    -1,    -1,    59,    98,   166,   167,
      99,    -1,   168,    -1,   167,   168,    -1,   161,   169,   104,
      -1,   170,    -1,   169,   101,   170,    -1,    62,    -1,    62,
      96,   147,    97,    -1,   144,    -1,   148,    -1,   175,    -1,
     174,    -1,   172,    -1,   181,    -1,   182,    -1,   185,    -1,
     191,    -1,    98,    99,    -1,    -1,    98,   176,   180,    99,
      -1,   178,    -1,   174,    -1,    98,    99,    -1,    -1,    98,
     179,   180,    99,    -1,   173,    -1,   180,   173,    -1,   104,
      -1,   146,   104,    -1,    13,    94,   146,    95,   183,    -1,
     173,    11,   173,    -1,   173,    -1,   146,    -1,   159,    62,
     103,   171,    -1,    -1,    61,   186,    94,   184,    95,   177,
      -1,    10,   173,    61,    94,   146,    95,   104,    -1,    -1,
      12,   187,    94,   188,   190,    95,   177,    -1,   181,    -1,
     172,    -1,   184,    -1,    -1,   189,   104,    -1,   189,   104,
     146,    -1,     9,   104,    -1,     8,   104,    -1,    15,   104,
      -1,    15,   146,   104,    -1,    14,   104,    -1,   193,    -1,
     192,   193,    -1,   194,    -1,   148,    -1,   149,   178,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   128,   128,   133,   135,   137,   139,   141,   146,   148,
     150,   152,   154,   156,   161,   166,   171,   173,   178,   180,
     185,   187,   192,   197,   199,   207,   209,   211,   213,   215,
     217,   219,   221,   223,   225,   227,   229,   231,   233,   235,
     237,   242,   244,   246,   248,   255,   257,   259,   261,   268,
     270,   272,   274,   279,   281,   283,   288,   290,   292,   297,
     299,   301,   303,   305,   310,   312,   314,   319,   321,   326,
     328,   333,   335,   340,   342,   347,   349,   354,   356,   361,
     363,   368,   370,   375,   377,   379,   381,   383,   385,   387,
     389,   391,   393,   395,   400,   402,   407,   412,   414,   416,
     421,   426,   428,   433,   435,   440,   445,   447,   452,   454,
     456,   458,   464,   465,   467,   469,   474,   476,   481,   483,
     485,   487,   492,   494,   496,   498,   500,   507,   509,   514,
     516,   518,   520,   522,   527,   529,   534,   536,   538,   540,
     542,   544,   546,   548,   550,   552,   554,   556,   558,   560,
     562,   564,   566,   568,   570,   572,   574,   576,   578,   580,
     582,   584,   586,   588,   590,   592,   594,   596,   598,   600,
     602,   604,   606,   608,   610,   612,   614,   616,   618,   620,
     625,   627,   629,   635,   634,   639,   638,   645,   646,   650,
     655,   657,   662,   664,   669,   674,   679,   681,   688,   690,
     692,   694,   696,   701,   704,   703,   710,   712,   717,   720,
     719,   726,   728,   733,   735,   740,   745,   747,   754,   756,
     762,   761,   765,   768,   767,   774,   776,   781,   784,   788,
     790,   795,   797,   799,   801,   803,   810,   811,   815,   816,
     821
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_ATTRIBUTE", "T_CONST", "T_BOOL",
  "T_FLOAT", "T_INT", "T_BREAK", "T_CONTINUE", "T_DO", "T_ELSE", "T_FOR",
  "T_IF", "T_DISCARD", "T_RETURN", "T_BVEC2", "T_BVEC3", "T_BVEC4",
  "T_IVEC2", "T_IVEC3", "T_IVEC4", "T_VEC2", "T_VEC3", "T_VEC4", "T_MAT2",
  "T_MAT3", "T_MAT4", "T_IN", "T_OUT", "T_INOUT", "T_UNIFORM", "T_VARYING",
  "T_UINT", "T_UVEC2", "T_UVEC3", "T_UVEC4", "T_MAT2X3", "T_MAT2X4",
  "T_MAT3X2", "T_MAT3X4", "T_MAT4X2", "T_MAT4X3", "T_SAMPLER2D",
  "T_SAMPLERCUBE", "T_SAMPLER3D", "T_SAMPLER1DARRAY", "T_SAMPLER2DARRAY",
  "T_SAMPLER1DARRAYSHADOW", "T_SAMPLER2DARRAYSHADOW", "T_ISAMPLER2D",
  "T_ISAMPLERCUBE", "T_ISAMPLER3D", "T_ISAMPLER2DARRAY", "T_USAMPLER2D",
  "T_USAMPLERCUBE", "T_USAMPLER3D", "T_USAMPLER2DARRAY",
  "T_SAMPLEREXTERNALOES", "T_STRUCT", "T_VOID", "T_WHILE", "T_IDENTIFIER",
  "T_TYPE_NAME", "T_FLOATCONSTANT", "T_INTCONSTANT", "T_BOOLCONSTANT",
  "T_FIELD_SELECTION", "T_LEFT_OP", "T_RIGHT_OP", "T_INC_OP", "T_DEC_OP",
  "T_LE_OP", "T_GE_OP", "T_EQ_OP", "T_NE_OP", "T_AND_OP", "T_OR_OP",
  "T_XOR_OP", "T_MUL_ASSIGN", "T_DIV_ASSIGN", "T_ADD_ASSIGN",
  "T_MOD_ASSIGN", "T_LEFT_ASSIGN", "T_RIGHT_ASSIGN", "T_AND_ASSIGN",
  "T_XOR_ASSIGN", "T_OR_ASSIGN", "T_SUB_ASSIGN", "T_INVARIANT",
  "T_HIGH_PRECISION", "T_MEDIUM_PRECISION", "T_LOW_PRECISION",
  "T_PRECISION", "'('", "')'", "'['", "']'", "'{'", "'}'", "'.'", "','",
  "':'", "'='", "';'", "'!'", "'-'", "'~'", "'+'", "'*'", "'/'", "'%'",
  "'<'", "'>'", "'|'", "'^'", "'&'", "'?'", "$accept",
  "variable_identifier", "primary_expression", "postfix_expression",
  "integer_expression", "function_call", "function_call_generic",
  "function_call_header_no_parameters",
  "function_call_header_with_parameters", "function_call_header",
  "function_identifier", "constructor_identifier", "unary_expression",
  "unary_operator", "multiplicative_expression", "additive_expression",
  "shift_expression", "relational_expression", "equality_expression",
  "and_expression", "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_xor_expression",
  "logical_or_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "constant_expression", "declaration", "function_prototype",
  "function_declarator", "function_header_with_parameters",
  "function_header", "parameter_declarator", "parameter_declaration",
  "parameter_qualifier", "parameter_type_specifier",
  "init_declarator_list", "single_declaration", "fully_specified_type",
  "type_qualifier", "type_specifier", "type_specifier_no_prec",
  "precision_qualifier", "struct_specifier", "@1", "@2",
  "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "declaration_statement", "statement", "simple_statement",
  "compound_statement", "@3", "statement_no_new_scope",
  "compound_statement_no_new_scope", "@4", "statement_list",
  "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "iteration_statement", "@5",
  "@6", "for_init_statement", "conditionopt", "for_rest_statement",
  "jump_statement", "translation_unit", "external_declaration",
  "function_definition", 0
};
#endif

#ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,    40,    41,    91,    93,   123,   125,
      46,    44,    58,    61,    59,    33,    45,   126,    43,    42,
      47,    37,    60,    62,   124,    94,    38,    63
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,   118,   119,   120,   120,   120,   120,   120,   121,   121,
     121,   121,   121,   121,   122,   123,   124,   124,   125,   125,
     126,   126,   127,   128,   128,   129,   129,   129,   129,   129,
     129,   129,   129,   129,   129,   129,   129,   129,   129,   129,
     129,   130,   130,   130,   130,   131,   131,   131,   131,   132,
     132,   132,   132,   133,   133,   133,   134,   134,   134,   135,
     135,   135,   135,   135,   136,   136,   136,   137,   137,   138,
     138,   139,   139,   140,   140,   141,   141,   142,   142,   143,
     143,   144,   144,   145,   145,   145,   145,   145,   145,   145,
     145,   145,   145,   145,   146,   146,   147,   148,   148,   148,
     149,   150,   150,   151,   151,   152,   153,   153,   154,   154,
     154,   154,   155,   155,   155,   155,   156,   156,   157,   157,
     157,   157,   158,   158,   158,   158,   158,   159,   159,   160,
     160,   160,   160,   160,   161,   161,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     162,   162,   162,   162,   162,   162,   162,   162,   162,   162,
     163,   163,   163,   165,   164,   166,   164,   167,   167,   168,
     169,   169,   170,   170,   171,   172,   173,   173,   174,   174,
     174,   174,   174,   175,   176,   175,   177,   177,   178,   179,
     178,   180,   180,   181,   181,   182,   183,   183,   184,   184,
     186,   185,   185,   187,   185,   188,   188,   189,   189,   190,
     190,   191,   191,   191,   191,   191,   192,   192,   193,   193,
     194
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     3,     1,     4,
       1,     3,     2,     2,     1,     1,     2,     2,     2,     1,
       2,     3,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     2,     2,     2,     1,     1,     1,     1,     1,
       3,     3,     3,     1,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       5,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     3,     1,     2,     2,     4,
       2,     1,     1,     2,     3,     3,     2,     5,     3,     2,
       3,     2,     0,     1,     1,     1,     1,     4,     1,     3,
       6,     5,     1,     2,     5,     4,     2,     1,     2,     1,
       1,     1,     2,     1,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     6,     0,     5,     1,     2,     3,
       1,     3,     1,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     0,     4,     1,     1,     2,     0,
       4,     1,     2,     1,     2,     5,     3,     1,     1,     4,
       0,     6,     7,     0,     7,     1,     1,     1,     0,     2,
       3,     2,     2,     2,     3,     2,     1,     2,     1,     1,
       2
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,   130,   129,   140,   137,   138,   144,   145,   146,   147,
     148,   149,   141,   142,   143,   153,   156,   159,   133,   131,
     139,   150,   151,   152,   154,   155,   157,   158,   160,   161,
     162,   163,   166,   167,   168,   169,   170,   171,   172,   173,
     174,   175,   176,   177,   178,   179,     0,   136,   165,     0,
     180,   181,   182,     0,   239,     0,     0,   102,   112,     0,
     118,   122,     0,   127,   134,     0,   164,     0,   236,   238,
       0,   185,   132,   126,     0,   209,    97,   240,   100,   112,
     113,   114,   115,     0,   103,     0,   112,     0,    98,   123,
     128,   135,     1,   237,   183,     0,     0,   208,     0,   104,
     109,   111,   116,     0,   119,   105,     0,     0,     0,     0,
       0,   187,    99,   140,   137,   138,     0,     0,     0,   223,
       0,     0,     0,   144,   145,   146,   147,   148,   149,   141,
     142,   143,   153,   156,   159,   220,     2,   165,     5,     4,
       6,     0,     0,     0,   204,   213,    47,    46,    48,    45,
       3,     8,    41,    10,    15,     0,     0,    19,     0,    23,
      49,     0,    53,    56,    59,    64,    67,    69,    71,    73,
      75,    77,    79,    81,    94,     0,   195,     0,   198,   211,
     197,   196,     0,   199,   200,   201,   202,   106,     0,   108,
     110,     0,     0,    27,    25,    26,    31,    32,    33,    34,
      35,    36,    28,    29,    30,    37,    38,    39,    40,    49,
      96,     0,   194,   125,     0,   192,     0,   190,   186,   188,
     232,   231,     0,     0,     0,   235,   233,     0,     0,    42,
      43,     0,   203,     0,    12,    13,     0,     0,    17,    16,
       0,    18,    20,    22,    84,    85,    87,    86,    89,    90,
      91,    92,    93,    88,    83,     0,    44,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   214,   210,
     212,     0,     0,     0,   121,   124,   184,     0,     0,   189,
       0,     0,     0,   234,     0,     7,     0,     0,    14,    11,
      21,    82,    50,    51,    52,    55,    54,    57,    58,    62,
      63,    60,    61,    65,    66,    68,    70,    72,    74,    76,
      78,     0,    95,     0,   117,   120,     0,   191,     0,   226,
     225,   228,     0,   218,     0,     0,   205,     9,     0,   107,
     193,     0,   227,     0,     0,   217,   215,     0,     0,    80,
       0,   229,     0,     0,     0,   207,   221,   206,   222,   230,
     224,   216,   219
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,   150,   151,   152,   297,   153,   154,   155,   156,   157,
     158,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   255,   175,   211,
     176,   177,    56,    57,    58,   100,    84,    85,   101,    59,
      60,    61,    62,    63,    64,    65,    66,   108,    95,   110,
     111,   216,   217,   213,   178,   179,   180,   181,   233,   356,
     357,    98,   182,   183,   184,   346,   335,   185,   228,   223,
     331,   343,   344,   186,    67,    68,    69
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -283
static const short yypact[] =
{
    1310,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,
    -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,
    -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,
    -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,
    -283,  -283,  -283,  -283,  -283,  -283,   -45,  -283,  -283,     2,
    -283,  -283,  -283,    38,  -283,   -85,   -63,   -38,    12,    -2,
    -283,    24,  1371,  -283,  -283,  1431,  -283,  1232,  -283,  -283,
      16,  -283,  -283,  -283,  1431,   -29,  -283,  -283,  -283,    27,
    -283,  -283,  -283,    93,  -283,  1371,   106,    82,  -283,   -68,
    -283,  -283,  -283,  -283,  -283,  1371,    41,  -283,   448,  -283,
    -283,  -283,   -48,  1371,   -58,  -283,   954,   954,  1371,    84,
    1047,  -283,  -283,    53,    55,    56,    47,    48,   448,  -283,
      59,    50,   836,    61,    62,    65,    66,    67,    68,    69,
      70,    71,    73,    74,    75,  -283,    77,    78,  -283,  -283,
    -283,   954,   954,   954,    79,  -283,  -283,  -283,  -283,  -283,
    -283,  -283,   -34,  -283,  -283,    81,   -28,   861,    80,  -283,
      10,   954,    28,     4,   -46,   -52,    29,    57,    88,    90,
     104,   103,   -66,  -283,  -283,     7,  -283,   101,  -283,  -283,
    -283,  -283,   236,  -283,  -283,  -283,  -283,    86,   954,  -283,
    -283,   954,   954,  -283,  -283,  -283,  -283,  -283,  -283,  -283,
    -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,
    -283,   109,  -283,  -283,  1135,   111,    22,  -283,  -283,  -283,
    -283,  -283,   149,   117,   954,  -283,  -283,    23,   119,  -283,
    -283,   -24,  -283,   448,  -283,  -283,   954,   150,  -283,  -283,
     954,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,
    -283,  -283,  -283,  -283,  -283,   954,  -283,   954,   954,   954,
     954,   954,   954,   954,   954,   954,   954,   954,   954,   954,
     954,   954,   954,   954,   954,   954,   954,   954,  -283,  -283,
    -283,   954,   121,   122,  -283,  -283,  -283,   954,    84,  -283,
     126,   650,   -23,  -283,   743,  -283,   342,   124,   115,  -283,
    -283,  -283,  -283,  -283,  -283,    28,    28,     4,     4,   -46,
     -46,   -46,   -46,   -52,   -52,    29,    57,    88,    90,   104,
     103,   -20,  -283,   125,  -283,  -283,   127,  -283,   954,  -283,
    -283,   743,   448,   115,   161,   130,  -283,  -283,   954,  -283,
    -283,   -21,  -283,   123,   131,   217,  -283,   132,   554,  -283,
     129,   954,   554,   448,   954,  -283,  -283,  -283,  -283,   115,
    -283,  -283,  -283
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,  -283,
    -283,  -283,   -73,  -283,  -129,  -122,  -145,  -126,   -41,   -37,
     -42,   -36,   -10,    -9,  -283,  -104,  -107,  -283,  -119,  -181,
       8,     9,  -283,  -283,  -283,   135,   186,   218,   200,  -283,
    -283,  -282,   -33,   -56,   -47,   252,  -283,  -283,  -283,   201,
    -105,  -283,    20,  -188,    19,  -117,  -269,  -283,  -283,   -40,
     256,  -283,    83,    26,  -283,  -283,   -18,  -283,  -283,  -283,
    -283,  -283,  -283,  -283,  -283,   247,  -283
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -102
static const short yytable[] =
{
     212,   222,   210,   227,   284,   219,    90,   282,    54,    55,
     283,   275,   334,    75,   187,     1,     2,    70,    91,    76,
     264,   265,   262,   263,   231,    86,   105,    96,   106,   102,
       1,     2,    78,   209,    72,   107,   234,   235,   191,   109,
      80,    81,    82,    18,    19,   192,    86,   102,   188,   334,
     242,   276,   109,    71,   109,    80,    81,    82,    18,    19,
     266,   267,   236,    79,    73,   280,   237,   239,   229,   230,
      97,   295,   332,   240,   350,    54,    55,   277,   277,   355,
     277,   277,   338,   355,   210,   212,    89,   210,   256,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,    87,
     323,    83,    88,   268,   269,   292,   326,  -101,   277,   219,
     260,   278,   261,   254,    94,   209,    83,   298,   209,   309,
     310,   311,   312,   288,   277,    72,   289,   293,    50,    51,
      52,   305,   306,   300,    80,    81,    82,   257,   258,   259,
     307,   308,   313,   314,   104,   112,   215,   -27,   301,   -25,
     -26,   220,   221,   224,   225,   -31,   -32,   321,   109,   -33,
     -34,   -35,   -36,   -28,   -29,   -30,   362,   -37,   -38,   -39,
     322,   -24,   -40,   270,   243,   333,   238,   210,   232,   280,
     273,   274,   281,   210,   302,   303,   304,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   271,   272,    76,   285,   287,   209,   341,
     290,   291,   333,   294,   209,   345,   277,   299,   324,   325,
     328,   337,   339,   347,   340,   348,   352,   351,   353,   315,
     317,   349,   359,   358,   316,   354,   361,   318,   189,     1,
       2,   113,   114,   115,   116,   117,   118,   212,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,   133,   134,   319,    99,   320,    18,    19,    20,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,   135,   136,   137,
     138,   139,   140,   190,   103,    74,   141,   142,   327,   214,
     329,    77,   360,   342,    93,     0,   296,   330,     0,     0,
       0,     0,     0,     0,     0,    49,    50,    51,    52,    53,
     143,     0,     0,     0,   144,   279,     0,     0,     0,     0,
     145,   146,   147,   148,   149,     1,     2,   113,   114,   115,
     116,   117,   118,     0,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,   135,   136,   137,   138,   139,   140,     0,
       0,     0,   141,   142,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    49,    50,    51,    52,    53,   143,     0,     0,     0,
     144,   336,     0,     0,     0,     0,   145,   146,   147,   148,
     149,     1,     2,   113,   114,   115,   116,   117,   118,     0,
     119,   120,   121,   122,   123,   124,   125,   126,   127,   128,
     129,   130,   131,   132,   133,   134,     0,     0,     0,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,   135,
     136,   137,   138,   139,   140,     0,     0,     0,   141,   142,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    49,    50,    51,
      52,    53,   143,     0,     0,     0,   144,     0,     0,     0,
       0,     0,   145,   146,   147,   148,   149,     1,     2,   113,
     114,   115,   116,   117,   118,     0,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   129,   130,   131,   132,
     133,   134,     0,     0,     0,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,   135,   136,   137,   138,   139,
     140,     0,     0,     0,   141,   142,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    49,    50,    51,    52,    53,   143,     0,
       0,     0,    75,     1,     2,   113,   114,   115,   145,   146,
     147,   148,   149,     0,     0,     0,   123,   124,   125,   126,
     127,   128,   129,   130,   131,   132,   133,   134,     0,     0,
       0,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,     0,   136,   137,   138,   139,   140,     0,     0,     0,
     141,   142,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    49,
      50,    51,    52,    53,   143,     0,     1,     2,   113,   114,
     115,     0,     0,     0,   145,   146,   147,   148,   149,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,   133,
     134,     0,     0,     0,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,     0,   136,   137,   138,   139,   140,
       0,     0,     0,   141,   142,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    83,    50,    51,    52,     0,   143,     0,     0,
       0,   193,   194,   195,     0,     0,     0,     0,   146,   147,
     148,   149,   196,   197,   198,   199,   200,   201,   202,   203,
     204,   205,   206,   207,     0,     0,   193,   194,   195,     0,
       0,     0,     0,     0,     0,     0,     0,   196,   197,   198,
     199,   200,   201,   202,   203,   204,   205,   206,   207,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   136,   208,
     138,   139,   140,     0,     0,     0,   141,   142,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   241,     0,   136,   208,   138,   139,   140,     0,     0,
     143,   141,   142,     0,     0,     0,     0,     0,     0,     0,
     226,   146,   147,   148,   149,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   143,     0,     0,     0,   193,
     194,   195,     0,     0,     0,     0,   146,   147,   148,   149,
     196,   197,   198,   199,   200,   201,   202,   203,   204,   205,
     206,   207,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   136,   208,   138,   139,
     140,     0,     0,     0,   141,   142,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   143,     0,
       0,     0,     3,     4,     5,     0,     0,     0,     0,   146,
     147,   148,   149,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,     0,     0,     0,     0,     0,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,     0,     0,
      48,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    50,    51,    52,
       3,     4,     5,     0,     0,     0,   218,     0,     0,     0,
       0,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,     0,     0,     0,     0,     0,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,     0,     0,    48,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,     0,     0,
       0,     0,    92,     0,   286,     1,     2,     3,     4,     5,
       0,     0,     0,     0,     0,     0,     0,     0,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
       0,     0,     0,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,     0,     0,    48,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     1,     2,     3,     4,     5,     0,     0,
       0,    49,    50,    51,    52,    53,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,     0,     0,
       0,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,     0,     0,    48,     0,     0,     3,     4,     5,     0,
       0,     0,     0,     0,     0,     0,     0,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    49,
      50,    51,    52,    53,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,     0,     0,    48,     0,     3,     4,     5,     0,
       0,     0,     0,     0,     0,     0,     0,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,     0,
       0,    50,    51,    52,    20,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,     0,     0,    48
};

static const short yycheck[] =
{
     107,   118,   106,   122,   192,   110,    62,   188,     0,     0,
     191,    77,   294,    98,    62,     3,     4,    62,    65,   104,
      72,    73,    68,    69,   143,    58,    94,    74,    96,    85,
       3,     4,    95,   106,    32,   103,    70,    71,    96,    95,
      28,    29,    30,    31,    32,   103,    79,   103,    96,   331,
     157,   117,   108,    98,   110,    28,    29,    30,    31,    32,
     112,   113,    96,   101,    62,   182,   100,    95,   141,   142,
      99,    95,    95,   101,    95,    67,    67,   101,   101,   348,
     101,   101,   102,   352,   188,   192,    62,   191,   161,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,   101,
     281,    89,   104,    74,    75,   224,   287,    95,   101,   214,
     106,   104,   108,   103,    98,   188,    89,   236,   191,   264,
     265,   266,   267,   101,   101,    32,   104,   104,    90,    91,
      92,   260,   261,   240,    28,    29,    30,   109,   110,   111,
     262,   263,   268,   269,    62,   104,    62,    94,   255,    94,
      94,   104,   104,    94,   104,    94,    94,   276,   214,    94,
      94,    94,    94,    94,    94,    94,   354,    94,    94,    94,
     277,    94,    94,   116,    94,   294,    95,   281,    99,   296,
      76,    78,    96,   287,   257,   258,   259,   260,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,   271,   272,
     273,   274,   275,   115,   114,   104,    97,    96,   281,   328,
      61,    94,   331,    94,   287,   332,   101,    67,    97,    97,
      94,    97,    97,    62,    97,    95,    95,   104,    11,   270,
     272,   338,   351,   104,   271,   103,   353,   273,   103,     3,
       4,     5,     6,     7,     8,     9,    10,   354,    12,    13,
      14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,   274,    79,   275,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,   103,    86,    53,    70,    71,   288,   108,
     291,    55,   352,   331,    67,    -1,   233,   291,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    89,    90,    91,    92,    93,
      94,    -1,    -1,    -1,    98,    99,    -1,    -1,    -1,    -1,
     104,   105,   106,   107,   108,     3,     4,     5,     6,     7,
       8,     9,    10,    -1,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      -1,    -1,    -1,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    -1,
      -1,    -1,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    89,    90,    91,    92,    93,    94,    -1,    -1,    -1,
      98,    99,    -1,    -1,    -1,    -1,   104,   105,   106,   107,
     108,     3,     4,     5,     6,     7,     8,     9,    10,    -1,
      12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    -1,    -1,    -1,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    -1,    -1,    -1,    70,    71,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,    90,    91,
      92,    93,    94,    -1,    -1,    -1,    98,    -1,    -1,    -1,
      -1,    -1,   104,   105,   106,   107,   108,     3,     4,     5,
       6,     7,     8,     9,    10,    -1,    12,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    -1,    -1,    -1,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    -1,    -1,    -1,    70,    71,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    89,    90,    91,    92,    93,    94,    -1,
      -1,    -1,    98,     3,     4,     5,     6,     7,   104,   105,
     106,   107,   108,    -1,    -1,    -1,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    -1,    -1,
      -1,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    62,    63,    64,    65,    66,    -1,    -1,    -1,
      70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    89,
      90,    91,    92,    93,    94,    -1,     3,     4,     5,     6,
       7,    -1,    -1,    -1,   104,   105,   106,   107,   108,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    -1,    -1,    -1,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    -1,    62,    63,    64,    65,    66,
      -1,    -1,    -1,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    89,    90,    91,    92,    -1,    94,    -1,    -1,
      -1,     5,     6,     7,    -1,    -1,    -1,    -1,   105,   106,
     107,   108,    16,    17,    18,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    -1,    -1,     5,     6,     7,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    62,    63,
      64,    65,    66,    -1,    -1,    -1,    70,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    60,    -1,    62,    63,    64,    65,    66,    -1,    -1,
      94,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     104,   105,   106,   107,   108,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    94,    -1,    -1,    -1,     5,
       6,     7,    -1,    -1,    -1,    -1,   105,   106,   107,   108,
      16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
      26,    27,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    62,    63,    64,    65,
      66,    -1,    -1,    -1,    70,    71,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    94,    -1,
      -1,    -1,     5,     6,     7,    -1,    -1,    -1,    -1,   105,
     106,   107,   108,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    -1,    -1,    -1,    -1,    -1,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    -1,    -1,
      63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    90,    91,    92,
       5,     6,     7,    -1,    -1,    -1,    99,    -1,    -1,    -1,
      -1,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    -1,    -1,    -1,    -1,    -1,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    -1,    -1,    63,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    90,    91,    92,    -1,    -1,
      -1,    -1,     0,    -1,    99,     3,     4,     5,     6,     7,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      -1,    -1,    -1,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      58,    59,    60,    -1,    -1,    63,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,     7,    -1,    -1,
      -1,    89,    90,    91,    92,    93,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    -1,    -1,
      -1,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    -1,    -1,    63,    -1,    -1,     5,     6,     7,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    89,
      90,    91,    92,    93,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    -1,    63,    -1,     5,     6,     7,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    16,    17,    18,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    -1,
      -1,    90,    91,    92,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    -1,    -1,    63
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     5,     6,     7,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    63,    89,
      90,    91,    92,    93,   148,   149,   150,   151,   152,   157,
     158,   159,   160,   161,   162,   163,   164,   192,   193,   194,
      62,    98,    32,    62,   163,    98,   104,   178,    95,   101,
      28,    29,    30,    89,   154,   155,   160,   101,   104,    62,
     161,   162,     0,   193,    98,   166,   162,    99,   179,   154,
     153,   156,   161,   155,    62,    94,    96,   103,   165,   161,
     167,   168,   104,     5,     6,     7,     8,     9,    10,    12,
      13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    61,    62,    63,    64,    65,
      66,    70,    71,    94,    98,   104,   105,   106,   107,   108,
     119,   120,   121,   123,   124,   125,   126,   127,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   146,   148,   149,   172,   173,
     174,   175,   180,   181,   182,   185,   191,    62,    96,   153,
     156,    96,   103,     5,     6,     7,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    63,   130,
     143,   147,   144,   171,   167,    62,   169,   170,    99,   168,
     104,   104,   173,   187,    94,   104,   104,   146,   186,   130,
     130,   146,    99,   176,    70,    71,    96,   100,    95,    95,
     101,    60,   144,    94,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,   103,   145,   130,   109,   110,   111,
     106,   108,    68,    69,    72,    73,   112,   113,    74,    75,
     116,   115,   114,    76,    78,    77,   117,   101,   104,    99,
     173,    96,   147,   147,   171,    97,    99,    96,   101,   104,
      61,    94,   146,   104,    94,    95,   180,   122,   146,    67,
     144,   144,   130,   130,   130,   132,   132,   133,   133,   134,
     134,   134,   134,   135,   135,   136,   137,   138,   139,   140,
     141,   146,   144,   147,    97,    97,   147,   170,    94,   172,
     181,   188,    95,   146,   159,   184,    99,    97,   102,    97,
      97,   146,   184,   189,   190,   173,   183,    62,    95,   144,
      95,   104,    95,    11,   103,   174,   177,   178,   104,   146,
     177,   173,   171
};

#if !defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if !defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if !defined (YYSIZE_T)
#if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if !defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

#ifndef YYFPRINTF
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

#ifndef yystrlen
#if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

#ifndef yystpcpy
#if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
#ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  /* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
#ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 129 "gc_glsl.y"
    { yyval.expr = slParseVariableIdentifier(Compiler, &yyvsp[0].token); ;}
    break;

  case 3:
#line 134 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 4:
#line 136 "gc_glsl.y"
    { yyval.expr = slParseIntConstant(Compiler, &yyvsp[0].token); ;}
    break;

  case 5:
#line 138 "gc_glsl.y"
    { yyval.expr = slParseFloatConstant(Compiler, &yyvsp[0].token); ;}
    break;

  case 6:
#line 140 "gc_glsl.y"
    { yyval.expr = slParseBoolConstant(Compiler, &yyvsp[0].token); ;}
    break;

  case 7:
#line 142 "gc_glsl.y"
    { yyval.expr = yyvsp[-1].expr; ;}
    break;

  case 8:
#line 147 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 9:
#line 149 "gc_glsl.y"
    { yyval.expr = slParseSubscriptExpr(Compiler, yyvsp[-3].expr, yyvsp[-1].expr); ;}
    break;

  case 10:
#line 151 "gc_glsl.y"
    { yyval.expr = slParseFuncCallExprAsExpr(Compiler, yyvsp[0].funcCall); ;}
    break;

  case 11:
#line 153 "gc_glsl.y"
    { yyval.expr = slParseFieldSelectionExpr(Compiler, yyvsp[-2].expr, &yyvsp[0].token); ;}
    break;

  case 12:
#line 155 "gc_glsl.y"
    { yyval.expr = slParseIncOrDecExpr(Compiler, gcvNULL, slvUNARY_POST_INC, yyvsp[-1].expr); ;}
    break;

  case 13:
#line 157 "gc_glsl.y"
    { yyval.expr = slParseIncOrDecExpr(Compiler, gcvNULL, slvUNARY_POST_DEC, yyvsp[-1].expr); ;}
    break;

  case 14:
#line 162 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 15:
#line 167 "gc_glsl.y"
    { yyval.funcCall = yyvsp[0].funcCall; ;}
    break;

  case 16:
#line 172 "gc_glsl.y"
    { yyval.funcCall = yyvsp[-1].funcCall; ;}
    break;

  case 17:
#line 174 "gc_glsl.y"
    { yyval.funcCall = yyvsp[-1].funcCall; ;}
    break;

  case 18:
#line 179 "gc_glsl.y"
    { yyval.funcCall = yyvsp[-1].funcCall; ;}
    break;

  case 19:
#line 181 "gc_glsl.y"
    { yyval.funcCall = yyvsp[0].funcCall; ;}
    break;

  case 20:
#line 186 "gc_glsl.y"
    { yyval.funcCall = slParseFuncCallArgument(Compiler, yyvsp[-1].funcCall, yyvsp[0].expr); ;}
    break;

  case 21:
#line 188 "gc_glsl.y"
    { yyval.funcCall = slParseFuncCallArgument(Compiler, yyvsp[-2].funcCall, yyvsp[0].expr); ;}
    break;

  case 22:
#line 193 "gc_glsl.y"
    { yyval.funcCall = slParseFuncCallHeaderExpr(Compiler, &yyvsp[-1].token); ;}
    break;

  case 23:
#line 198 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 24:
#line 200 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 25:
#line 208 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 26:
#line 210 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 27:
#line 212 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 28:
#line 214 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 29:
#line 216 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 30:
#line 218 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 31:
#line 220 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 32:
#line 222 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 33:
#line 224 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 34:
#line 226 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 35:
#line 228 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 36:
#line 230 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 37:
#line 232 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 38:
#line 234 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 39:
#line 236 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 40:
#line 238 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 41:
#line 243 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 42:
#line 245 "gc_glsl.y"
    { yyval.expr = slParseIncOrDecExpr(Compiler, &yyvsp[-1].token, slvUNARY_PRE_INC, yyvsp[0].expr); ;}
    break;

  case 43:
#line 247 "gc_glsl.y"
    { yyval.expr = slParseIncOrDecExpr(Compiler, &yyvsp[-1].token, slvUNARY_PRE_DEC, yyvsp[0].expr); ;}
    break;

  case 44:
#line 249 "gc_glsl.y"
    { yyval.expr = slParseNormalUnaryExpr(Compiler, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 45:
#line 256 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 46:
#line 258 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 47:
#line 260 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 48:
#line 262 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 49:
#line 269 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 50:
#line 271 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 51:
#line 273 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 52:
#line 275 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 53:
#line 280 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 54:
#line 282 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 55:
#line 284 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 56:
#line 289 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 57:
#line 291 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 58:
#line 293 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 59:
#line 298 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 60:
#line 300 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 61:
#line 302 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 62:
#line 304 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 63:
#line 306 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 64:
#line 311 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 65:
#line 313 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 66:
#line 315 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 67:
#line 320 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 68:
#line 322 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 69:
#line 327 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 70:
#line 329 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 71:
#line 334 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 72:
#line 336 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 73:
#line 341 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 74:
#line 343 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 75:
#line 348 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 76:
#line 350 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 77:
#line 355 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 78:
#line 357 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 79:
#line 362 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 80:
#line 364 "gc_glsl.y"
    { yyval.expr = slParseSelectionExpr(Compiler, yyvsp[-4].expr, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 81:
#line 369 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 82:
#line 371 "gc_glsl.y"
    { yyval.expr = slParseAssignmentExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 83:
#line 376 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 84:
#line 378 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 85:
#line 380 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 86:
#line 382 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 87:
#line 384 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 88:
#line 386 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 89:
#line 388 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 90:
#line 390 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 91:
#line 392 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 92:
#line 394 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 93:
#line 396 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 94:
#line 401 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 95:
#line 403 "gc_glsl.y"
    { yyval.expr = slParseNormalBinaryExpr(Compiler, yyvsp[-2].expr, &yyvsp[-1].token, yyvsp[0].expr); ;}
    break;

  case 96:
#line 408 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 97:
#line 413 "gc_glsl.y"
    { yyval.statement = slParseFuncDecl(Compiler, yyvsp[-1].funcName); ;}
    break;

  case 98:
#line 415 "gc_glsl.y"
    { yyval.statement = slParseDeclaration(Compiler, yyvsp[-1].declOrDeclList); ;}
    break;

  case 99:
#line 417 "gc_glsl.y"
    { yyval.statement = slParseDefaultPrecisionQualifier(Compiler, &yyvsp[-3].token, &yyvsp[-2].token, yyvsp[-1].dataType); ;}
    break;

  case 100:
#line 422 "gc_glsl.y"
    { yyval.funcName = yyvsp[-1].funcName; ;}
    break;

  case 101:
#line 427 "gc_glsl.y"
    { yyval.funcName = yyvsp[0].funcName; ;}
    break;

  case 102:
#line 429 "gc_glsl.y"
    { yyval.funcName = yyvsp[0].funcName; ;}
    break;

  case 103:
#line 434 "gc_glsl.y"
    { yyval.funcName = slParseParameterList(Compiler, yyvsp[-1].funcName, yyvsp[0].paramName); ;}
    break;

  case 104:
#line 436 "gc_glsl.y"
    { yyval.funcName = slParseParameterList(Compiler, yyvsp[-2].funcName, yyvsp[0].paramName); ;}
    break;

  case 105:
#line 441 "gc_glsl.y"
    { yyval.funcName = slParseFuncHeader(Compiler, yyvsp[-2].dataType, &yyvsp[-1].token); ;}
    break;

  case 106:
#line 446 "gc_glsl.y"
    { yyval.funcName = slParseNonArrayParameterDecl(Compiler, yyvsp[-1].dataType, &yyvsp[0].token); ;}
    break;

  case 107:
#line 448 "gc_glsl.y"
    { yyval.funcName = slParseArrayParameterDecl(Compiler, yyvsp[-4].dataType, &yyvsp[-3].token, yyvsp[-1].expr); ;}
    break;

  case 108:
#line 453 "gc_glsl.y"
    { yyval.paramName = slParseQualifiedParameterDecl(Compiler, &yyvsp[-2].token, &yyvsp[-1].token, yyvsp[0].funcName); ;}
    break;

  case 109:
#line 455 "gc_glsl.y"
    { yyval.paramName = slParseQualifiedParameterDecl(Compiler, gcvNULL, &yyvsp[-1].token, yyvsp[0].funcName); ;}
    break;

  case 110:
#line 457 "gc_glsl.y"
    { yyval.paramName = slParseQualifiedParameterDecl(Compiler, &yyvsp[-2].token, &yyvsp[-1].token, yyvsp[0].funcName); ;}
    break;

  case 111:
#line 459 "gc_glsl.y"
    { yyval.paramName = slParseQualifiedParameterDecl(Compiler, gcvNULL, &yyvsp[-1].token, yyvsp[0].funcName); ;}
    break;

  case 112:
#line 464 "gc_glsl.y"
    { yyval.token = slParseEmptyParameterQualifier(Compiler); ;}
    break;

  case 113:
#line 466 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 114:
#line 468 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 115:
#line 470 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 116:
#line 475 "gc_glsl.y"
    { yyval.funcName = slParseNonArrayParameterDecl(Compiler, yyvsp[0].dataType, gcvNULL); ;}
    break;

  case 117:
#line 477 "gc_glsl.y"
    { yyval.funcName = slParseArrayParameterDecl(Compiler, yyvsp[-3].dataType, gcvNULL, yyvsp[-1].expr); ;}
    break;

  case 118:
#line 482 "gc_glsl.y"
    { yyval.declOrDeclList = yyvsp[0].declOrDeclList; ;}
    break;

  case 119:
#line 484 "gc_glsl.y"
    { yyval.declOrDeclList = slParseNonArrayVariableDecl2(Compiler, yyvsp[-2].declOrDeclList, &yyvsp[0].token); ;}
    break;

  case 120:
#line 486 "gc_glsl.y"
    { yyval.declOrDeclList = slParseArrayVariableDecl2(Compiler, yyvsp[-5].declOrDeclList, &yyvsp[-3].token, yyvsp[-1].expr); ;}
    break;

  case 121:
#line 488 "gc_glsl.y"
    { yyval.declOrDeclList = slParseVariableDeclWithInitializer2(Compiler, yyvsp[-4].declOrDeclList, &yyvsp[-2].token, yyvsp[0].expr); ;}
    break;

  case 122:
#line 493 "gc_glsl.y"
    { yyval.declOrDeclList = slParseTypeDecl(Compiler, yyvsp[0].dataType); ;}
    break;

  case 123:
#line 495 "gc_glsl.y"
    { yyval.declOrDeclList = slParseNonArrayVariableDecl(Compiler, yyvsp[-1].dataType, &yyvsp[0].token); ;}
    break;

  case 124:
#line 497 "gc_glsl.y"
    { yyval.declOrDeclList = slParseArrayVariableDecl(Compiler, yyvsp[-4].dataType, &yyvsp[-3].token, yyvsp[-1].expr); ;}
    break;

  case 125:
#line 499 "gc_glsl.y"
    { yyval.declOrDeclList = slParseVariableDeclWithInitializer(Compiler, yyvsp[-3].dataType, &yyvsp[-2].token, yyvsp[0].expr); ;}
    break;

  case 126:
#line 501 "gc_glsl.y"
    { yyval.declOrDeclList = slParseInvariantDecl(Compiler, &yyvsp[-1].token, &yyvsp[0].token); ;}
    break;

  case 127:
#line 508 "gc_glsl.y"
    { yyval.dataType = yyvsp[0].dataType; ;}
    break;

  case 128:
#line 510 "gc_glsl.y"
    { yyval.dataType = slParseFullySpecifiedType(Compiler, &yyvsp[-1].token, yyvsp[0].dataType); ;}
    break;

  case 129:
#line 515 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 130:
#line 517 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 131:
#line 519 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 132:
#line 521 "gc_glsl.y"
    { yyval.token = yyvsp[-1].token; ;}
    break;

  case 133:
#line 523 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 134:
#line 528 "gc_glsl.y"
    { yyval.dataType = yyvsp[0].dataType; ;}
    break;

  case 135:
#line 530 "gc_glsl.y"
    { yyval.dataType = slParseTypeSpecifier(Compiler, &yyvsp[-1].token, yyvsp[0].dataType); ;}
    break;

  case 136:
#line 535 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_VOID); ;}
    break;

  case 137:
#line 537 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_FLOAT); ;}
    break;

  case 138:
#line 539 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_INT); ;}
    break;

  case 139:
#line 541 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_UINT); ;}
    break;

  case 140:
#line 543 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_BOOL); ;}
    break;

  case 141:
#line 545 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_VEC2); ;}
    break;

  case 142:
#line 547 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_VEC3); ;}
    break;

  case 143:
#line 549 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_VEC4); ;}
    break;

  case 144:
#line 551 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_BVEC2); ;}
    break;

  case 145:
#line 553 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_BVEC3); ;}
    break;

  case 146:
#line 555 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_BVEC4); ;}
    break;

  case 147:
#line 557 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_IVEC2); ;}
    break;

  case 148:
#line 559 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_IVEC3); ;}
    break;

  case 149:
#line 561 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_IVEC4); ;}
    break;

  case 150:
#line 563 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_UVEC2); ;}
    break;

  case 151:
#line 565 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_UVEC3); ;}
    break;

  case 152:
#line 567 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_UVEC4); ;}
    break;

  case 153:
#line 569 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT2); ;}
    break;

  case 154:
#line 571 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT2X3); ;}
    break;

  case 155:
#line 573 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT2X4); ;}
    break;

  case 156:
#line 575 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT3); ;}
    break;

  case 157:
#line 577 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT3X2); ;}
    break;

  case 158:
#line 579 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT3X4); ;}
    break;

  case 159:
#line 581 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT4); ;}
    break;

  case 160:
#line 583 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT4X2); ;}
    break;

  case 161:
#line 585 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_MAT4X3); ;}
    break;

  case 162:
#line 587 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_SAMPLER2D); ;}
    break;

  case 163:
#line 589 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_SAMPLERCUBE); ;}
    break;

  case 164:
#line 591 "gc_glsl.y"
    { yyval.dataType = slParseStructType(Compiler, yyvsp[0].dataType); ;}
    break;

  case 165:
#line 593 "gc_glsl.y"
    { yyval.dataType = slParseNamedType(Compiler, &yyvsp[0].token); ;}
    break;

  case 166:
#line 595 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_SAMPLER3D); ;}
    break;

  case 167:
#line 597 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_SAMPLER1DARRAY); ;}
    break;

  case 168:
#line 599 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_SAMPLER2DARRAY); ;}
    break;

  case 169:
#line 601 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_SAMPLER1DARRAYSHADOW); ;}
    break;

  case 170:
#line 603 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_SAMPLER2DARRAYSHADOW); ;}
    break;

  case 171:
#line 605 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_ISAMPLER2D); ;}
    break;

  case 172:
#line 607 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_ISAMPLERCUBE); ;}
    break;

  case 173:
#line 609 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_ISAMPLER3D); ;}
    break;

  case 174:
#line 611 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_ISAMPLER2DARRAY); ;}
    break;

  case 175:
#line 613 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_USAMPLER2D); ;}
    break;

  case 176:
#line 615 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_USAMPLERCUBE); ;}
    break;

  case 177:
#line 617 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_USAMPLER3D); ;}
    break;

  case 178:
#line 619 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_USAMPLER2DARRAY); ;}
    break;

  case 179:
#line 621 "gc_glsl.y"
    { yyval.dataType = slParseNonStructType(Compiler, &yyvsp[0].token, T_SAMPLEREXTERNALOES); ;}
    break;

  case 180:
#line 626 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 181:
#line 628 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 182:
#line 630 "gc_glsl.y"
    { yyval.token = yyvsp[0].token; ;}
    break;

  case 183:
#line 635 "gc_glsl.y"
    { slParseStructDeclBegin(Compiler); ;}
    break;

  case 184:
#line 637 "gc_glsl.y"
    { yyval.dataType = slParseStructDeclEnd(Compiler, &yyvsp[-5].token, &yyvsp[-4].token); ;}
    break;

  case 185:
#line 639 "gc_glsl.y"
    { slParseStructDeclBegin(Compiler); ;}
    break;

  case 186:
#line 641 "gc_glsl.y"
    { yyval.dataType = slParseStructDeclEnd(Compiler, &yyvsp[-4].token, gcvNULL); ;}
    break;

  case 189:
#line 651 "gc_glsl.y"
    { slParseTypeSpecifiedFieldDeclList(Compiler, yyvsp[-2].dataType, yyvsp[-1].fieldDeclList); ;}
    break;

  case 190:
#line 656 "gc_glsl.y"
    { yyval.fieldDeclList = slParseFieldDeclList(Compiler, yyvsp[0].fieldDecl); ;}
    break;

  case 191:
#line 658 "gc_glsl.y"
    { yyval.fieldDeclList = slParseFieldDeclList2(Compiler, yyvsp[-2].fieldDeclList, yyvsp[0].fieldDecl); ;}
    break;

  case 192:
#line 663 "gc_glsl.y"
    { yyval.fieldDecl = slParseFieldDecl(Compiler, &yyvsp[0].token, gcvNULL); ;}
    break;

  case 193:
#line 665 "gc_glsl.y"
    { yyval.fieldDecl = slParseFieldDecl(Compiler, &yyvsp[-3].token, yyvsp[-1].expr); ;}
    break;

  case 194:
#line 670 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 195:
#line 675 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 196:
#line 680 "gc_glsl.y"
    { yyval.statement = slParseCompoundStatementAsStatement(Compiler, yyvsp[0].statements); ;}
    break;

  case 197:
#line 682 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 198:
#line 689 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 199:
#line 691 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 200:
#line 693 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 201:
#line 695 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 202:
#line 697 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 203:
#line 702 "gc_glsl.y"
    { yyval.statements = gcvNULL; ;}
    break;

  case 204:
#line 704 "gc_glsl.y"
    { slParseCompoundStatementBegin(Compiler); ;}
    break;

  case 205:
#line 706 "gc_glsl.y"
    { yyval.statements = slParseCompoundStatementEnd(Compiler, &yyvsp[-3].token, yyvsp[-1].statements); ;}
    break;

  case 206:
#line 711 "gc_glsl.y"
    { yyval.statement = slParseCompoundStatementNoNewScopeAsStatementNoNewScope(Compiler, yyvsp[0].statements); ;}
    break;

  case 207:
#line 713 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 208:
#line 718 "gc_glsl.y"
    { yyval.statements = gcvNULL; ;}
    break;

  case 209:
#line 720 "gc_glsl.y"
    { slParseCompoundStatementNoNewScopeBegin(Compiler); ;}
    break;

  case 210:
#line 722 "gc_glsl.y"
    { yyval.statements = slParseCompoundStatementNoNewScopeEnd(Compiler, &yyvsp[-3].token, yyvsp[-1].statements); ;}
    break;

  case 211:
#line 727 "gc_glsl.y"
    { yyval.statements = slParseStatementList(Compiler, yyvsp[0].statement); ;}
    break;

  case 212:
#line 729 "gc_glsl.y"
    { yyval.statements = slParseStatementList2(Compiler, yyvsp[-1].statements, yyvsp[0].statement); ;}
    break;

  case 213:
#line 734 "gc_glsl.y"
    { yyval.statement = gcvNULL; ;}
    break;

  case 214:
#line 736 "gc_glsl.y"
    { yyval.statement = slParseExprAsStatement(Compiler, yyvsp[-1].expr); ;}
    break;

  case 215:
#line 741 "gc_glsl.y"
    { yyval.statement = slParseSelectionStatement(Compiler, &yyvsp[-4].token, yyvsp[-2].expr, yyvsp[0].selectionStatementPair); ;}
    break;

  case 216:
#line 746 "gc_glsl.y"
    { yyval.selectionStatementPair = slParseSelectionRestStatement(Compiler, yyvsp[-2].statement, yyvsp[0].statement); ;}
    break;

  case 217:
#line 748 "gc_glsl.y"
    { yyval.selectionStatementPair = slParseSelectionRestStatement(Compiler, yyvsp[0].statement, gcvNULL); ;}
    break;

  case 218:
#line 755 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 219:
#line 757 "gc_glsl.y"
    { yyval.expr = slParseCondition(Compiler, yyvsp[-3].dataType, &yyvsp[-2].token, yyvsp[0].expr); ;}
    break;

  case 220:
#line 762 "gc_glsl.y"
    { slParseWhileStatementBegin(Compiler); ;}
    break;

  case 221:
#line 764 "gc_glsl.y"
    { yyval.statement = slParseWhileStatementEnd(Compiler, &yyvsp[-5].token, yyvsp[-2].expr, yyvsp[0].statement); ;}
    break;

  case 222:
#line 766 "gc_glsl.y"
    { yyval.statement = slParseDoWhileStatement(Compiler, &yyvsp[-6].token, yyvsp[-5].statement, yyvsp[-2].expr); ;}
    break;

  case 223:
#line 768 "gc_glsl.y"
    { slParseForStatementBegin(Compiler); ;}
    break;

  case 224:
#line 770 "gc_glsl.y"
    { yyval.statement = slParseForStatementEnd(Compiler, &yyvsp[-6].token, yyvsp[-3].statement, yyvsp[-2].forExprPair, yyvsp[0].statement); ;}
    break;

  case 225:
#line 775 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 226:
#line 777 "gc_glsl.y"
    { yyval.statement = yyvsp[0].statement; ;}
    break;

  case 227:
#line 782 "gc_glsl.y"
    { yyval.expr = yyvsp[0].expr; ;}
    break;

  case 228:
#line 784 "gc_glsl.y"
    { yyval.expr = gcvNULL; ;}
    break;

  case 229:
#line 789 "gc_glsl.y"
    { yyval.forExprPair = slParseForRestStatement(Compiler, yyvsp[-1].expr, gcvNULL); ;}
    break;

  case 230:
#line 791 "gc_glsl.y"
    { yyval.forExprPair = slParseForRestStatement(Compiler, yyvsp[-2].expr, yyvsp[0].expr); ;}
    break;

  case 231:
#line 796 "gc_glsl.y"
    { yyval.statement = slParseJumpStatement(Compiler, slvCONTINUE, &yyvsp[-1].token, gcvNULL); ;}
    break;

  case 232:
#line 798 "gc_glsl.y"
    { yyval.statement = slParseJumpStatement(Compiler, slvBREAK, &yyvsp[-1].token, gcvNULL); ;}
    break;

  case 233:
#line 800 "gc_glsl.y"
    { yyval.statement = slParseJumpStatement(Compiler, slvRETURN, &yyvsp[-1].token, gcvNULL); ;}
    break;

  case 234:
#line 802 "gc_glsl.y"
    { yyval.statement = slParseJumpStatement(Compiler, slvRETURN, &yyvsp[-2].token, yyvsp[-1].expr); ;}
    break;

  case 235:
#line 804 "gc_glsl.y"
    { yyval.statement = slParseJumpStatement(Compiler, slvDISCARD, &yyvsp[-1].token, gcvNULL); ;}
    break;

  case 239:
#line 817 "gc_glsl.y"
    { slParseExternalDecl(Compiler, yyvsp[0].statement); ;}
    break;

  case 240:
#line 822 "gc_glsl.y"
    { slParseFuncDef(Compiler, yyvsp[-1].funcName, yyvsp[0].statements); ;}
    break;


    }

/* Line 1000 of yacc.c.  */
#line 3009 "gc_glsl_parser.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 825 "gc_glsl.y"



