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


#include "gc_glsl_scanner.h"

#define T_RESERVED_KEYWORD		T_EOF
#define T_NOT_KEYWORD			T_IDENTIFIER

typedef struct _slsKEYWORD
{
	gctCONST_STRING	symbol;
	gctINT			token;
}
slsKEYWORD;

static slsKEYWORD KeywordTable[] =
{
	{"asm",					T_RESERVED_KEYWORD},
	{"attribute",				T_ATTRIBUTE},
	{"bool",				T_BOOL},
	{"break",				T_BREAK},
	{"bvec2",				T_BVEC2},
	{"bvec3",				T_BVEC3},
	{"bvec4",				T_BVEC4},
	{"cast",				T_RESERVED_KEYWORD},
	{"class",				T_RESERVED_KEYWORD},
	{"const",				T_CONST},
	{"continue",				T_CONTINUE},
	{"default",				T_RESERVED_KEYWORD},
	{"discard",				T_DISCARD},
	{"do",					T_DO},
	{"double",				T_RESERVED_KEYWORD},
	{"dvec2",				T_RESERVED_KEYWORD},
	{"dvec3",				T_RESERVED_KEYWORD},
	{"dvec4",				T_RESERVED_KEYWORD},
	{"else",				T_ELSE},
	{"enum",				T_RESERVED_KEYWORD},
	{"extern",				T_RESERVED_KEYWORD},
	{"external",				T_RESERVED_KEYWORD},
	{"fixed",				T_RESERVED_KEYWORD},
	{"flat",				T_RESERVED_KEYWORD},
	{"float",				T_FLOAT},
	{"for",					T_FOR},
	{"fvec2",				T_RESERVED_KEYWORD},
	{"fvec3",				T_RESERVED_KEYWORD},
	{"fvec4",				T_RESERVED_KEYWORD},
	{"goto",				T_RESERVED_KEYWORD},
	{"half",				T_RESERVED_KEYWORD},
	{"highp",				T_HIGH_PRECISION},
	{"hvec2",				T_RESERVED_KEYWORD},
	{"hvec3",				T_RESERVED_KEYWORD},
	{"hvec4",				T_RESERVED_KEYWORD},
	{"if",					T_IF},
	{"in",					T_IN},
	{"inline",				T_RESERVED_KEYWORD},
	{"inout",				T_INOUT},
	{"input",				T_RESERVED_KEYWORD},
	{"int",					T_INT},
	{"interface",				T_RESERVED_KEYWORD},
	{"invariant",				T_INVARIANT},
	{"isampler2D",				T_ISAMPLER2D},
	{"isampler2DArray",			T_ISAMPLER2DARRAY},
	{"isampler3D",				T_ISAMPLER3D},
	{"isamplerCube",			T_ISAMPLERCUBE},
	{"ivec2",				T_IVEC2},
	{"ivec3",				T_IVEC3},
	{"ivec4",				T_IVEC4},
	{"long",				T_RESERVED_KEYWORD},
	{"lowp",				T_LOW_PRECISION},
	{"mat2",				T_MAT2},
	{"mat2x3",				T_MAT2X3},
	{"mat2x4",				T_MAT2X4},
	{"mat3",				T_MAT3},
	{"mat3x2",				T_MAT3X2},
	{"mat3x4",				T_MAT3X4},
	{"mat4",				T_MAT4},
	{"mat4x2",				T_MAT4X2},
	{"mat4x3",				T_MAT4X3},
	{"mediump",				T_MEDIUM_PRECISION},
	{"namespace",			T_RESERVED_KEYWORD},
	{"noinline",			T_RESERVED_KEYWORD},
	{"out",				T_OUT},
	{"output",			T_RESERVED_KEYWORD},
	{"packed",			T_RESERVED_KEYWORD},
	{"precision",			T_PRECISION},
	{"public",			T_RESERVED_KEYWORD},
	{"return",			T_RETURN},
	{"sampler1D",			T_RESERVED_KEYWORD},
	{"sampler1DArray",		T_SAMPLER1DARRAY},
	{"sampler1DArrayShadow",	T_SAMPLER1DARRAYSHADOW},
	{"sampler1DShadow",		T_RESERVED_KEYWORD},
	{"sampler2D",			T_SAMPLER2D},
	{"sampler2DArray",		T_SAMPLER2DARRAY},
	{"sampler2DArrayShadow",	T_SAMPLER2DARRAYSHADOW},
	{"sampler2DRect",		T_RESERVED_KEYWORD},
	{"sampler2DRectShadow",		T_RESERVED_KEYWORD},
	{"sampler2DShadow",		T_RESERVED_KEYWORD},
	{"sampler3D",			T_SAMPLER3D},
	{"sampler3DRect",		T_RESERVED_KEYWORD},
	{"samplerCube",			T_SAMPLERCUBE},
	{"samplerExternalOES",		T_SAMPLEREXTERNALOES},

	{"short",			T_RESERVED_KEYWORD},
	{"sizeof",			T_RESERVED_KEYWORD},
	{"static",			T_RESERVED_KEYWORD},
	{"struct",			T_STRUCT},
	{"superp",			T_RESERVED_KEYWORD},
	{"switch",			T_RESERVED_KEYWORD},
	{"template",			T_RESERVED_KEYWORD},
	{"this",			T_RESERVED_KEYWORD},
	{"typedef",			T_RESERVED_KEYWORD},
	{"uniform",			T_UNIFORM},
	{"union",			T_RESERVED_KEYWORD},
	{"unsigned",			T_RESERVED_KEYWORD},
	{"usampler2D",			T_USAMPLER2D},
	{"usampler2DArray",		T_USAMPLER2DARRAY},
	{"usampler3D",			T_USAMPLER3D},
	{"usamplerCube",		T_USAMPLERCUBE},
	{"using",			T_RESERVED_KEYWORD},
	{"uvec2",			T_UVEC2},
	{"uvec3",			T_UVEC3},
	{"uvec4",			T_UVEC4},
	{"varying",			T_VARYING},
	{"vec2",			T_VEC2},
	{"vec3",			T_VEC3},
	{"vec4",			T_VEC4},
	{"void",			T_VOID},
	{"volatile",			T_RESERVED_KEYWORD},
	{"while",			T_WHILE}
};

const gctUINT KeywordCount = sizeof(KeywordTable) / sizeof(slsKEYWORD);

static gctINT
_SearchKeyword(
	IN gctCONST_STRING Symbol
	)
{
	gctINT		low, mid, high;
	gceSTATUS	result;

	low = 0;
	high = KeywordCount - 1;

	while (low <= high)
	{
		mid = (low + high) / 2;

		result = gcoOS_StrCmp(Symbol, KeywordTable[mid].symbol);

		if (result == gcvSTATUS_SMALLER)
		{
			high	= mid - 1;
		}
		else if (result == gcvSTATUS_LARGER)
		{
			low		= mid + 1;
		}
		else
		{
			gcmASSERT(gcmIS_SUCCESS(result));

			return KeywordTable[mid].token;
		}
	}

	return T_NOT_KEYWORD;
}

gctINT
slScanIdentifier(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Symbol,
	OUT slsLexToken * Token
	)
{
	gceSTATUS		status;
	gctINT			tokenType;
	sleSHADER_TYPE	shaderType;
	sltPOOL_STRING	symbolInPool;
	slsNAME *		typeName;

    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "LineNo=%u", LineNo);
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "StringNo=%u", StringNo);
    gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_COMPILER, "Symbol=%s", gcmOPT_STRING(Symbol));

	gcmASSERT(Token);

	gcmVERIFY_OK(sloCOMPILER_GetShaderType(Compiler, &shaderType));

	Token->lineNo	= LineNo;
	Token->stringNo	= StringNo;

	/* Check as a reserved keyword */
	tokenType = _SearchKeyword(Symbol);

	if (tokenType == T_RESERVED_KEYWORD)
	{
		Token->type = T_RESERVED_KEYWORD;

		gcmVERIFY_OK(sloCOMPILER_Report(
										Compiler,
										LineNo,
										StringNo,
										slvREPORT_ERROR,
										"reserved keyword : '%s'",
										Symbol));

		return T_RESERVED_KEYWORD;
	}
	else if (tokenType != T_NOT_KEYWORD)
	{
		Token->type = tokenType;

		switch (tokenType)
		{
		case T_CONST:				Token->u.qualifier	= slvQUALIFIER_CONST;				break;
		case T_UNIFORM:				Token->u.qualifier	= slvQUALIFIER_UNIFORM;				break;

		case T_ATTRIBUTE:
			if (shaderType != slvSHADER_TYPE_VERTEX)
			{
				gcmVERIFY_OK(sloCOMPILER_Report(
												Compiler,
												LineNo,
												StringNo,
												slvREPORT_ERROR,
												"'attribute' is only for the vertex shaders",
												Symbol));
			}

			Token->u.qualifier	= slvQUALIFIER_ATTRIBUTE;
			break;

		case T_VARYING:
			Token->u.qualifier	= (shaderType == slvSHADER_TYPE_VERTEX)?
					slvQUALIFIER_VARYING_OUT : slvQUALIFIER_VARYING_IN;
			break;

		case T_INVARIANT:
			Token->u.qualifier	= (shaderType == slvSHADER_TYPE_VERTEX)?
					slvQUALIFIER_INVARIANT_VARYING_OUT : slvQUALIFIER_INVARIANT_VARYING_IN;
			break;

		case T_IN:					Token->u.qualifier	= slvQUALIFIER_IN;					break;
		case T_OUT:					Token->u.qualifier	= slvQUALIFIER_OUT;					break;
		case T_INOUT:				Token->u.qualifier	= slvQUALIFIER_INOUT;				break;

		case T_HIGH_PRECISION:		Token->u.precision	= slvPRECISION_HIGH;				break;
		case T_MEDIUM_PRECISION:	Token->u.precision	= slvPRECISION_MEDIUM;				break;
		case T_LOW_PRECISION:		Token->u.precision	= slvPRECISION_LOW;					break;
		}

		gcmVERIFY_OK(sloCOMPILER_Dump(
									Compiler,
									slvDUMP_SCANNER,
									"<TOKEN line=\"%d\" string=\"%d\" type=\"keyword\" symbol=\"%s\" />",
									LineNo,
									StringNo,
									Symbol));

		return tokenType;
	}

	status = sloCOMPILER_AllocatePoolString(
											Compiler,
											Symbol,
											&symbolInPool);

	if (gcmIS_ERROR(status)) return T_EOF;

	if (sloCOMPILER_GetScannerState(Compiler) == slvSCANNER_NOMRAL)
	{
		/* Check as a type name */
		status = sloCOMPILER_SearchName(
										Compiler,
										symbolInPool,
										gcvTRUE,
										&typeName);

		if (status == gcvSTATUS_OK && typeName->type == slvSTRUCT_NAME)
		{
			Token->type			= T_TYPE_NAME;
			Token->u.typeName	= typeName;

			gcmVERIFY_OK(sloCOMPILER_Dump(
										Compiler,
										slvDUMP_SCANNER,
										"<TOKEN line=\"%d\" string=\"%d\" type=\"typeName\" symbol=\"%s\" />",
										LineNo,
										StringNo,
										symbolInPool));

			return T_TYPE_NAME;
		}
	}

	/* Treat as an identifier */
	Token->type			= T_IDENTIFIER;
	Token->u.identifier = symbolInPool;

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_SCANNER,
								"<TOKEN line=\"%d\" string=\"%d\" type=\"identifier\" symbol=\"%s\" />",
								LineNo,
								StringNo,
								Token->u.identifier));

	return T_IDENTIFIER;
}

gctINT
slScanConvToUnsignedType(
IN sloCOMPILER Compiler,
IN gctUINT LineNo,
IN gctUINT StringNo,
IN gctSTRING Symbol,
OUT slsLexToken * Token
)
{
	gcmASSERT(Token);
	Token->lineNo	= LineNo;
	Token->stringNo	= StringNo;

	/* Check as a reserved keyword */
	switch(_SearchKeyword(Symbol)) {
	case T_INT:
		Token->type = T_UINT;
		break;

	default:
		gcmASSERT(0);
		return T_EOF;
	}

	gcmVERIFY_OK(sloCOMPILER_Dump(Compiler,
				      slvDUMP_SCANNER,
				      "<TOKEN line=\"%d\" string=\"%d\" type=\"keyword\" symbol=\"%s\" />",
				      LineNo,
				      StringNo,
				      Symbol));
	return T_UINT;
}

gctINT
slScanBoolConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctBOOL Value,
	OUT slsLexToken * Token
	)
{
	gcmASSERT(Token);

	Token->lineNo			= LineNo;
	Token->stringNo			= StringNo;
	Token->type			= T_BOOLCONSTANT;
	Token->u.constant.boolValue	= Value;

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_SCANNER,
								"<TOKEN line=\"%d\" string=\"%d\" type=\"boolConstant\" value=\"%s\" />",
								LineNo,
								StringNo,
								(Token->u.constant.boolValue)? "true" : "false"));

	return T_BOOLCONSTANT;
}

#define SL_INT_MAX      2147483647

static gctINT32
StringToIntConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING String,
	IN gctINT Base,
	IN OUT gctINT * Index
	)
{
	gctINT orgIndex = *Index;
    gctBOOL error = gcvFALSE;
	gctINT32 result = 0, digit = 0;
	gctCHAR ch;

	gcmASSERT(String);
	gcmASSERT(Index);
	gcmASSERT((Base == 8) || (Base == 10) || (Base == 16));

	for (; gcvTRUE; (*Index)++)
	{
		ch = String[*Index];

		switch (Base)
		{
		case 8:
			if ((ch >= '0') && (ch <= '7')) digit = ch - '0';
			else goto EXIT;
			break;

		case 10:
			if ((ch >= '0') && (ch <= '9')) digit = ch - '0';
			else goto EXIT;
			break;

		case 16:
			if ((ch >= 'a') && (ch <= 'f'))			digit = ch - 'a' + 10;
			else if ((ch >= 'A') && (ch <= 'F'))	digit = ch - 'A' + 10;
			else if ((ch >= '0') && (ch <= '9'))	digit = ch - '0';
			else goto EXIT;
			break;

		default: gcmASSERT(0);
		}

		if (error) continue;

		if (result > ((SL_INT_MAX - digit) / Base))
		{
            error = gcvTRUE;

			gcmVERIFY_OK(sloCOMPILER_Report(
											Compiler,
											LineNo,
											StringNo,
											slvREPORT_ERROR,
											"too large %s integer: %s",
											(Base == 8)?
												"octal" : ((Base == 10)? "decimal" : "hexadecimal"),
											String + orgIndex));
        }
		else
		{
            result = result * Base + digit;
        }
    }

EXIT:
    return result;
}

gctINT
slScanDecIntConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	OUT slsLexToken * Token
	)
{
    gctINT index = 0;

	gcmASSERT(Token);

	Token->lineNo			= LineNo;
	Token->stringNo			= StringNo;
	Token->type			= T_INTCONSTANT;
	Token->u.constant.intValue	= StringToIntConstant(Compiler, LineNo, StringNo, Text, 10, &index);
    gcmASSERT(Text[index] == '\0');

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_SCANNER,
								"<TOKEN line=\"%d\" string=\"%d\" type=\"intConstant\""
								" format=\"decimal\" value=\"%d\" />",
								LineNo,
								StringNo,
								Token->u.constant.intValue));

	return T_INTCONSTANT;
}

gctINT
slScanOctIntConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	OUT slsLexToken * Token
	)
{
    gctINT index = 1;

	gcmASSERT(Token);

	Token->lineNo			= LineNo;
	Token->stringNo			= StringNo;
	Token->type			= T_INTCONSTANT;
	Token->u.constant.intValue	= StringToIntConstant(Compiler, LineNo, StringNo, Text, 8, &index);
    gcmASSERT(Text[index] == '\0');

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_SCANNER,
								"<TOKEN line=\"%d\" string=\"%d\" type=\"intConstant\""
								" format=\"octal\" value=\"%d\" />",
								LineNo,
								StringNo,
								Token->u.constant.intValue));

	return T_INTCONSTANT;
}

gctINT
slScanHexIntConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	OUT slsLexToken * Token
	)
{
    gctINT index = 2;

	gcmASSERT(Token);

	Token->lineNo			= LineNo;
	Token->stringNo			= StringNo;
	Token->type			= T_INTCONSTANT;
	Token->u.constant.intValue	= StringToIntConstant(Compiler, LineNo, StringNo, Text, 16, &index);
    gcmASSERT(Text[index] == '\0');

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_SCANNER,
								"<TOKEN line=\"%d\" string=\"%d\" type=\"intConstant\""
								" format=\"hexadecimal\" value=\"%d\" />",
								LineNo,
								StringNo,
								Token->u.constant.intValue));

	return T_INTCONSTANT;
}

gctINT
slScanFloatConstant(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	OUT slsLexToken * Token
	)
{
	gcmASSERT(Token);

	Token->lineNo			= LineNo;
	Token->stringNo			= StringNo;
	Token->type			= T_FLOATCONSTANT;

	gcmVERIFY_OK(gcoOS_StrToFloat(Text, &Token->u.constant.floatValue));

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_SCANNER,
								"<TOKEN line=\"%d\" string=\"%d\" type=\"floatConstant\""
								" value=\"%f\" />",
								LineNo,
								StringNo,
								Token->u.constant.floatValue));

	return T_FLOATCONSTANT;
}

gctINT
slScanOperator(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Text,
	IN gctINT tokenType,
	OUT slsLexToken * Token
	)
{
	gcmASSERT(Token);

	Token->lineNo			= LineNo;
	Token->stringNo			= StringNo;
	Token->type				= tokenType;
	Token->u.operator		= tokenType;

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_SCANNER,
								"<TOKEN line=\"%d\" string=\"%d\""
								" type=\"operator\" symbol=\"%s\" />",
								LineNo,
								StringNo,
								Text));

	return tokenType;
}

gctINT
slScanFieldSelection(
	IN sloCOMPILER Compiler,
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN gctSTRING Symbol,
	OUT slsLexToken * Token
	)
{
	gceSTATUS		status;
	sltPOOL_STRING	symbolInPool;

	gcmASSERT(Token);

	Token->lineNo	= LineNo;
	Token->stringNo	= StringNo;

	status = sloCOMPILER_AllocatePoolString(
											Compiler,
											Symbol,
											&symbolInPool);

	if (gcmIS_ERROR(status)) return T_EOF;

	Token->type = T_FIELD_SELECTION;
	Token->u.fieldSelection = symbolInPool;

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_SCANNER,
								"<TOKEN line=\"%d\" string=\"%d\" type=\"fieldSelection\" symbol=\"%s\" />",
								LineNo,
								StringNo,
								Token->u.fieldSelection));

	return T_FIELD_SELECTION;
}

int yywrap(void)
{
	return 1;
}
