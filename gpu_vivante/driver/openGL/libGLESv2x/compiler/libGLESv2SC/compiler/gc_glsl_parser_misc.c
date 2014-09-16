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


#include "gc_glsl_parser.h"

extern int
yyparse(
    void *
    );

extern void
yyrestart(
    gctPOINTER
    );

extern int
yylex(
    YYSTYPE * pyylval,
    sloCOMPILER Compiler
    )
{
    int tokenType;

    gcmHEADER();

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    tokenType = sloCOMPILER_Scan(Compiler, &(pyylval->token));

    switch (tokenType)
    {
    case T_BOOL:
    case T_FLOAT:
    case T_INT:
    case T_UINT:
    case T_BVEC2:
    case T_BVEC3:
    case T_BVEC4:
    case T_IVEC2:
    case T_IVEC3:
    case T_IVEC4:
    case T_UVEC2:
    case T_UVEC3:
    case T_UVEC4:
    case T_VEC2:
    case T_VEC3:
    case T_VEC4:
    case T_MAT2:
    case T_MAT2X3:
    case T_MAT2X4:
    case T_MAT3:
    case T_MAT3X2:
    case T_MAT3X4:
    case T_MAT4:
    case T_MAT4X2:
    case T_MAT4X3:
    case T_SAMPLER2D:
    case T_SAMPLERCUBE:

    case T_SAMPLER3D:
    case T_SAMPLER1DARRAY:
    case T_SAMPLER2DARRAY:
    case T_SAMPLER1DARRAYSHADOW:
    case T_SAMPLER2DARRAYSHADOW:

    case T_ISAMPLER2D:
    case T_ISAMPLERCUBE:
    case T_ISAMPLER3D:
    case T_ISAMPLER2DARRAY:

    case T_USAMPLER2D:
    case T_USAMPLERCUBE:
    case T_USAMPLER3D:
    case T_USAMPLER2DARRAY:
    case T_SAMPLEREXTERNALOES:

    case T_VOID:
    case T_TYPE_NAME:
        gcmVERIFY_OK(sloCOMPILER_SetScannerState(Compiler, slvSCANNER_AFTER_TYPE));
        break;

    default:
        gcmVERIFY_OK(sloCOMPILER_SetScannerState(Compiler, slvSCANNER_NOMRAL));
        break;
    }

    gcmFOOTER_ARG("%d", tokenType);
    return tokenType;
}

gceSTATUS
sloCOMPILER_LoadBuiltIns(
    IN sloCOMPILER Compiler
    )
{
	gceSTATUS		status;
	sleSHADER_TYPE	shaderType;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);

    gcmVERIFY_OK(sloCOMPILER_LoadingBuiltIns(Compiler, gcvTRUE));

    /* Load all kind of built-ins */
    gcmVERIFY_OK(sloCOMPILER_GetShaderType(Compiler, &shaderType));

    status = slLoadBuiltIns(Compiler, shaderType);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    gcmVERIFY_OK(sloCOMPILER_LoadingBuiltIns(Compiler, gcvFALSE));

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

gceSTATUS
sloCOMPILER_Parse(
    IN sloCOMPILER Compiler,
    IN gctUINT StringCount,
    IN gctCONST_STRING Strings[]
    )
{
    gceSTATUS       status = gcvSTATUS_OK;
#ifdef SL_SCAN_ONLY
    slsLexToken     token;
#endif

    gcmHEADER_ARG("Compiler=0x%x StringCount=%u Strings=0x%x",
                  Compiler, StringCount, Strings);

    status = sloCOMPILER_Lock(Compiler);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    status = sloCOMPILER_MakeCurrent(Compiler, StringCount, Strings);

	if (gcmIS_ERROR(status))
    {
        gcmVERIFY_OK(sloCOMPILER_Unlock(Compiler));
        gcmFOOTER();
        return status;
    }

#ifdef SL_SCAN_ONLY
    while (sloCOMPILER_Scan(Compiler, &token) != T_EOF);
#else
    yyrestart(gcvNULL);

	if (yyparse(Compiler) != 0)
    {
		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmVERIFY_OK(sloCOMPILER_Unlock(Compiler));
        gcmFOOTER();
        return status;
    }
#endif

    gcmVERIFY_OK(sloCOMPILER_Unlock(Compiler));

    gcmFOOTER();
	return status;
}

static gceSTATUS
_CheckErrorAsConstantExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR Expr,
    OUT sloIR_CONSTANT * Constant
    )
{
	gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x Expr=0x%x Constant=0x%x",
                  Compiler, Expr, Constant);

    gcmASSERT(Expr);
    gcmASSERT(Constant);

    if (sloIR_OBJECT_GetType(&Expr->base) != slvIR_CONSTANT)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Expr->base.lineNo,
                                        Expr->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a constant expression"));

        *Constant = gcvNULL;

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    *Constant = (sloIR_CONSTANT)Expr;

    gcmFOOTER_ARG("*Constant=0x%x", *Constant);
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorAsLValueExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR Expr
    )
{
    sloIR_UNARY_EXPR    unaryExpr;
	gceSTATUS           status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x Expr=0x%x",
                  Compiler, Expr);

    gcmASSERT(Expr);
    gcmASSERT(Expr->dataType);

    switch (Expr->dataType->qualifier)
    {
    case slvQUALIFIER_CONST:
    case slvQUALIFIER_UNIFORM:
    case slvQUALIFIER_ATTRIBUTE:
    case slvQUALIFIER_VARYING_IN:
    case slvQUALIFIER_INVARIANT_VARYING_IN:
    case slvQUALIFIER_CONST_IN:

        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Expr->base.lineNo,
                                        Expr->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a l-value expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (sloIR_OBJECT_GetType(&Expr->base) == slvIR_UNARY_EXPR)
    {
        unaryExpr = (sloIR_UNARY_EXPR)Expr;

        if (unaryExpr->type == slvUNARY_COMPONENT_SELECTION
            && slIsRepeatedComponentSelection(&unaryExpr->u.componentSelection))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            Expr->base.lineNo,
                                            Expr->base.stringNo,
                                            slvREPORT_ERROR,
                                            "The l-value expression select repeated"
                                            " components or swizzles"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_EvaluateExprToArrayLength(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR Expr,
    OUT gctUINT * ArrayLength
    )
{
    gceSTATUS       status;
    sloIR_CONSTANT  constant;

    gcmHEADER_ARG("Compiler=0x%x Expr=0x%x ArrayLength=0x%x",
                  Compiler, Expr, ArrayLength);

    gcmASSERT(Expr);
    gcmASSERT(ArrayLength);

    *ArrayLength = 0;

    status = _CheckErrorAsConstantExpr(
                                    Compiler,
                                    Expr,
                                    &constant);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    if (constant->exprBase.dataType == gcvNULL
        || !slsDATA_TYPE_IsInt(constant->exprBase.dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Expr->base.lineNo,
                                        Expr->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an integral constant expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
    }

    if (constant->valueCount > 1
        || constant->values == gcvNULL
        || constant->values[0].intValue <= 0)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Expr->base.lineNo,
                                        Expr->base.stringNo,
                                        slvREPORT_ERROR,
                                        "the array length must be greater than zero"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
    }

    *ArrayLength = (gctUINT)constant->values[0].intValue;

    gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &constant->exprBase.base));

    gcmFOOTER_ARG("*ArrayLength=0x%x", *ArrayLength);
	return gcvSTATUS_OK;
}

sloIR_EXPR
slParseVariableIdentifier(
    IN sloCOMPILER Compiler,
    IN slsLexToken * Identifier
    )
{
    gceSTATUS       status;
    slsNAME *       name;
    sloIR_CONSTANT  constant;
    sloIR_VARIABLE  variable;
    sloIR_EXPR      expr;

    gcmHEADER_ARG("Compiler=0x%x Identifier=0x%x",
                  Compiler, Identifier);

    gcmASSERT(Identifier);

    status = sloCOMPILER_SearchName(
                                    Compiler,
                                    Identifier->u.identifier,
                                    gcvTRUE,
                                    &name);

    if (status != gcvSTATUS_OK)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Identifier->lineNo,
                                        Identifier->stringNo,
                                        slvREPORT_ERROR,
                                        "undefined identifier: '%s'",
                                        Identifier->u.identifier));

        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    switch (name->type)
    {
    case slvVARIABLE_NAME:
    case slvPARAMETER_NAME:
        break;

    case slvFUNC_NAME:
    case slvSTRUCT_NAME:
    case slvFIELD_NAME:
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Identifier->lineNo,
                                        Identifier->stringNo,
                                        slvREPORT_ERROR,
                                        "'%s' isn't a variable",
                                        name->symbol));

        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;

	default:
		gcmASSERT(0);
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    if (name->type == slvVARIABLE_NAME && name->u.variableInfo.constant != gcvNULL)
    {
        status = sloIR_CONSTANT_Clone(
                                    Compiler,
                                    Identifier->lineNo,
                                    Identifier->stringNo,
                                    name->u.variableInfo.constant,
                                    &constant);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}

        expr = &constant->exprBase;
    }
    else
    {
        status = sloIR_VARIABLE_Construct(
                                        Compiler,
                                        Identifier->lineNo,
                                        Identifier->stringNo,
                                        name,
                                        &variable);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}

        expr = &variable->exprBase;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<VARIABLE_IDENTIFIER line=\"%d\" string=\"%d\" name=\"%s\" />",
                                Identifier->lineNo,
                                Identifier->stringNo,
                                Identifier->u.identifier));

    gcmFOOTER_ARG("<return>=0x%x", expr);
	return expr;
}

sloIR_EXPR
slParseIntConstant(
    IN sloCOMPILER Compiler,
    IN slsLexToken * IntConstant
    )
{
    gceSTATUS           status;
    slsDATA_TYPE *      dataType;
    sloIR_CONSTANT      constant;
    sluCONSTANT_VALUE   value;

    gcmHEADER_ARG("Compiler=0x%x IntConstant=0x%x",
                  Compiler, IntConstant);

    gcmASSERT(IntConstant);

    /* Create the data type */
    status = sloCOMPILER_CreateDataType(
                                        Compiler,
                                        T_INT,
                                        gcvNULL,
                                        &dataType);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    dataType->qualifier = slvQUALIFIER_CONST;

    /* Create the constant */
    status = sloIR_CONSTANT_Construct(
                                    Compiler,
                                    IntConstant->lineNo,
                                    IntConstant->stringNo,
                                    dataType,
                                    &constant);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    /* Add the constant value */
    value.intValue = IntConstant->u.constant.intValue;

    status = sloIR_CONSTANT_AddValues(
                                    Compiler,
                                    constant,
                                    1,
                                    &value);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<INT_CONSTANT line=\"%d\" string=\"%d\" value=\"%d\" />",
                                IntConstant->lineNo,
                                IntConstant->stringNo,
                                IntConstant->u.constant.intValue));

    gcmFOOTER_ARG("<return>=0x%x", &constant->exprBase);
	return &constant->exprBase;
}

sloIR_EXPR
slParseFloatConstant(
    IN sloCOMPILER Compiler,
    IN slsLexToken * FloatConstant
    )
{
    gceSTATUS           status;
    slsDATA_TYPE *      dataType;
    sloIR_CONSTANT      constant;
    sluCONSTANT_VALUE   value;

    gcmHEADER_ARG("Compiler=0x%x FloatConstant=0x%x",
                  Compiler, FloatConstant);

    gcmASSERT(FloatConstant);

    /* Create the data type */
    status = sloCOMPILER_CreateDataType(
                                        Compiler,
                                        T_FLOAT,
                                        gcvNULL,
                                        &dataType);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    dataType->qualifier = slvQUALIFIER_CONST;

    /* Create the constant */
    status = sloIR_CONSTANT_Construct(
                                    Compiler,
                                    FloatConstant->lineNo,
                                    FloatConstant->stringNo,
                                    dataType,
                                    &constant);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    /* Add the constant value */
    value.floatValue = FloatConstant->u.constant.floatValue;

    status = sloIR_CONSTANT_AddValues(
                                    Compiler,
                                    constant,
                                    1,
                                    &value);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<FLOAT_CONSTANT line=\"%d\" string=\"%d\" value=\"%f\" />",
                                FloatConstant->lineNo,
                                FloatConstant->stringNo,
                                FloatConstant->u.constant.floatValue));

    gcmFOOTER_ARG("<return>=0x%x", &constant->exprBase);
	return &constant->exprBase;
}

sloIR_EXPR
slParseBoolConstant(
    IN sloCOMPILER Compiler,
    IN slsLexToken * BoolConstant
    )
{
    gceSTATUS           status;
    slsDATA_TYPE *      dataType;
    sloIR_CONSTANT      constant;
    sluCONSTANT_VALUE   value;

    gcmHEADER_ARG("Compiler=0x%x BoolConstant=0x%x",
                  Compiler, BoolConstant);

    gcmASSERT(BoolConstant);

    /* Create the data type */
    status = sloCOMPILER_CreateDataType(
                                        Compiler,
                                        T_BOOL,
                                        gcvNULL,
                                        &dataType);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    dataType->qualifier = slvQUALIFIER_CONST;

    /* Create the constant */
    status = sloIR_CONSTANT_Construct(
                                    Compiler,
                                    BoolConstant->lineNo,
                                    BoolConstant->stringNo,
                                    dataType,
                                    &constant);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    /* Add the constant value */
    value.boolValue = BoolConstant->u.constant.boolValue;

    status = sloIR_CONSTANT_AddValues(
                                    Compiler,
                                    constant,
                                    1,
                                    &value);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<BOOL_CONSTANT line=\"%d\" string=\"%d\" value=\"%s\" />",
                                BoolConstant->lineNo,
                                BoolConstant->stringNo,
                                (BoolConstant->u.constant.boolValue)? "true" : "false"));

    gcmFOOTER_ARG("<return>=0x%x", &constant->exprBase);
	return &constant->exprBase;
}

gceSTATUS
_CheckErrorForSubscriptExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
    gctINT32        index;

	gceSTATUS       status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, RightOperand);

    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);

    /* Check the left operand */
    if (!slsDATA_TYPE_IsBVecOrIVecOrVec(LeftOperand->dataType)
        && !slsDATA_TYPE_IsMat(LeftOperand->dataType)
        && !slsDATA_TYPE_IsArray(LeftOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an array or matrix or vector typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    /* Check the right operand */
    if (!slsDATA_TYPE_IsInt(RightOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        RightOperand->base.lineNo,
                                        RightOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar integer expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    /* Check constant index range */
    if (sloIR_OBJECT_GetType(&RightOperand->base) == slvIR_CONSTANT)
    {
        gcmASSERT(((sloIR_CONSTANT)RightOperand)->valueCount == 1);

        index = ((sloIR_CONSTANT)RightOperand)->values[0].intValue;

        if (index < 0)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            RightOperand->base.lineNo,
                                            RightOperand->base.stringNo,
                                            slvREPORT_ERROR,
                                            "require a nonnegative index"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}

        if (slsDATA_TYPE_IsBVecOrIVecOrVec(LeftOperand->dataType))
        {
            if ((gctUINT8) index >= slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                RightOperand->base.lineNo,
                                                RightOperand->base.stringNo,
                                                slvREPORT_ERROR,
                                                "the index exceeds the vector type size"));

				status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
				return status;
			}
        }
        else if (slsDATA_TYPE_IsMat(LeftOperand->dataType))
        {
            if (index >= slmDATA_TYPE_matrixColumnCount_GET(LeftOperand->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                RightOperand->base.lineNo,
                                                RightOperand->base.stringNo,
                                                slvREPORT_ERROR,
                                                "the index exceeds the matrix type size"));

				status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
				return status;
            }
        }
        else
        {
            gcmASSERT(slsDATA_TYPE_IsArray(LeftOperand->dataType));

            if ((gctUINT)index >= LeftOperand->dataType->arrayLength)
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                RightOperand->base.lineNo,
                                                RightOperand->base.stringNo,
                                                slvREPORT_ERROR,
                                                "the index exceeds the array type size"));

				status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
				return status;
            }
        }
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

sloIR_EXPR
slParseSubscriptExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
    gceSTATUS           status;
    sloIR_CONSTANT      resultConstant;
    sloIR_BINARY_EXPR   binaryExpr;

    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, RightOperand);


	if (LeftOperand == gcvNULL || RightOperand == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    /* Check error */
    status = _CheckErrorForSubscriptExpr(
                                        Compiler,
                                        LeftOperand,
                                        RightOperand);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    /* Constant calculation */
    if (sloIR_OBJECT_GetType(&LeftOperand->base) == slvIR_CONSTANT
        && sloIR_OBJECT_GetType(&RightOperand->base) == slvIR_CONSTANT)
    {
        status = sloIR_BINARY_EXPR_Evaluate(
                                            Compiler,
                                            slvBINARY_SUBSCRIPT,
                                            (sloIR_CONSTANT)LeftOperand,
                                            (sloIR_CONSTANT)RightOperand,
                                            &resultConstant);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}

        gcmFOOTER_ARG("<return>=0x%x", &resultConstant->exprBase);
		return &resultConstant->exprBase;
    }

    /* Create the binary expression */
    status = sloIR_BINARY_EXPR_Construct(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        slvBINARY_SUBSCRIPT,
                                        LeftOperand,
                                        RightOperand,
                                        &binaryExpr);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<SUBSCRIPT_EXPR line=\"%d\" string=\"%d\" />",
                                LeftOperand->base.lineNo,
                                LeftOperand->base.stringNo));

    gcmFOOTER_ARG("<return>=0x%x", &binaryExpr->exprBase);
	return &binaryExpr->exprBase;
}

static gceSTATUS
_CheckErrorAsScalarConstructor(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr
    )
{
    gctUINT     operandCount;
    sloIR_EXPR  operand;

	gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x PolynaryExpr=0x%x",
                  Compiler, PolynaryExpr);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);

    if (PolynaryExpr->operands == gcvNULL)
    {
        operandCount = 0;
    }
    else
    {
        gcmVERIFY_OK(sloIR_SET_GetMemberCount(
                                            Compiler,
                                            PolynaryExpr->operands,
                                            &operandCount));
    }

    if (operandCount != 1)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        slvREPORT_ERROR,
                                        "require one expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    operand = slsDLINK_LIST_First(&PolynaryExpr->operands->members, struct _sloIR_EXPR);
    gcmASSERT(operand);
    gcmASSERT(operand->dataType);

    if (!slsDATA_TYPE_IsBoolOrBVec(operand->dataType)
        && !slsDATA_TYPE_IsIntOrIVec(operand->dataType)
        && !slsDATA_TYPE_IsFloatOrVecOrMat(operand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        operand->base.lineNo,
                                        operand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an boolean or integer or floating-point"
                                        " typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorAsVectorOrMatrixConstructor(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN gctBOOL IsVectorConstructor
    )
{
    gctUINT     operandCount;
    sloIR_EXPR  operand;
    gctSIZE_T   operandDataSizes = 0;
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x PolynaryExpr=0x%x IsVectorConstructor=%d",
                  Compiler, PolynaryExpr, IsVectorConstructor);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);

    if (PolynaryExpr->operands == gcvNULL)
    {
        operandCount = 0;
    }
    else
    {
        gcmVERIFY_OK(sloIR_SET_GetMemberCount(
                                            Compiler,
                                            PolynaryExpr->operands,
                                            &operandCount));
    }

    if (operandCount == 0)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        slvREPORT_ERROR,
                                        "require at least one expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    FOR_EACH_DLINK_NODE(&PolynaryExpr->operands->members, struct _sloIR_EXPR, operand)
    {
        gcmASSERT(operand);
        gcmASSERT(operand->dataType);

        if (!slsDATA_TYPE_IsBoolOrBVec(operand->dataType)
            && !slsDATA_TYPE_IsIntOrIVec(operand->dataType)
            && !slsDATA_TYPE_IsFloatOrVecOrMat(operand->dataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            operand->base.lineNo,
                                            operand->base.stringNo,
                                            slvREPORT_ERROR,
                                            "require any boolean or integer or floating-point"
                                            " typed expression"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}

        if (operandDataSizes >= slsDATA_TYPE_GetSize(PolynaryExpr->exprBase.dataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            PolynaryExpr->exprBase.base.lineNo,
                                            PolynaryExpr->exprBase.base.stringNo,
                                            slvREPORT_ERROR,
                                            "too many expressions in the constructor"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}

        operandDataSizes += slsDATA_TYPE_GetSize(operand->dataType);
    }

    if (operandCount == 1)
    {
        operand = slsDLINK_LIST_First(&PolynaryExpr->operands->members, struct _sloIR_EXPR);
        gcmASSERT(operand);

        if (IsVectorConstructor)
        {
            if (slsDATA_TYPE_IsScalar(operand->dataType))
            {
                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
        }
        else
        {
            if (slsDATA_TYPE_IsScalar(operand->dataType) || slsDATA_TYPE_IsMat(operand->dataType))
            {
                gcmFOOTER_NO();
                return gcvSTATUS_OK;
            }
        }
    }

    if (operandDataSizes < slsDATA_TYPE_GetSize(PolynaryExpr->exprBase.dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        slvREPORT_ERROR,
                                        "require more expressions"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorAsStructConstructor(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr
    )
{
    gctUINT         operandCount;
    sloIR_EXPR      operand;
    slsDATA_TYPE *  structDataType;
    slsNAME *       fieldName;
    gceSTATUS       status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x PolynaryExpr=0x%x",
                  Compiler, PolynaryExpr);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);

    if (PolynaryExpr->operands == gcvNULL)
    {
        operandCount = 0;
    }
    else
    {
        gcmVERIFY_OK(sloIR_SET_GetMemberCount(
                                            Compiler,
                                            PolynaryExpr->operands,
                                            &operandCount));
    }

    if (operandCount == 0)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        PolynaryExpr->exprBase.base.lineNo,
                                        PolynaryExpr->exprBase.base.stringNo,
                                        slvREPORT_ERROR,
                                        "require at least one expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    structDataType = PolynaryExpr->exprBase.dataType;
    gcmASSERT(structDataType);
    gcmASSERT(slsDATA_TYPE_IsStruct(structDataType));

    for (fieldName = slsDLINK_LIST_First(&structDataType->fieldSpace->names, slsNAME),
            operand = slsDLINK_LIST_First(&PolynaryExpr->operands->members, struct _sloIR_EXPR);
        (slsDLINK_NODE *)fieldName != &structDataType->fieldSpace->names;
        fieldName = slsDLINK_NODE_Next(&fieldName->node, slsNAME),
            operand = slsDLINK_NODE_Next(&operand->base.node, struct _sloIR_EXPR))
    {
        if ((slsDLINK_NODE *)operand == &PolynaryExpr->operands->members)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            PolynaryExpr->exprBase.base.lineNo,
                                            PolynaryExpr->exprBase.base.stringNo,
                                            slvREPORT_ERROR,
                                            "require more expressions"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}

        if (!slsDATA_TYPE_IsEqual(fieldName->dataType, operand->dataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            operand->base.lineNo,
                                            operand->base.stringNo,
                                            slvREPORT_ERROR,
                                            "require the same typed expression"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}
	}

    if ((slsDLINK_NODE *)operand != &PolynaryExpr->operands->members)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        operand->base.lineNo,
                                        operand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "too many expressions"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorAsFuncCall(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR PolynaryExpr
    )
{
    gceSTATUS       status;
    gctUINT         operandCount, paramCount;
    sloIR_EXPR      operand;
    slsNAME *       paramName;

    gcmHEADER_ARG("Compiler=0x%x PolynaryExpr=0x%x",
                  Compiler, PolynaryExpr);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(PolynaryExpr->funcName);
    gcmASSERT(PolynaryExpr->funcName->u.funcInfo.localSpace);

    if (PolynaryExpr->operands == gcvNULL)
    {
        operandCount = 0;
    }
    else
    {
        gcmVERIFY_OK(sloIR_SET_GetMemberCount(
                                            Compiler,
                                            PolynaryExpr->operands,
                                            &operandCount));
    }

    if (operandCount == 0)
    {
        gcmVERIFY_OK(sloNAME_GetParamCount(
                                            Compiler,
                                            PolynaryExpr->funcName,
                                            &paramCount));

        if (paramCount != 0)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            PolynaryExpr->exprBase.base.lineNo,
                                            PolynaryExpr->exprBase.base.stringNo,
                                            slvREPORT_ERROR,
                                            "require %d argument(s)",
                                            paramCount));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}
		else
		{
            gcmFOOTER_NO();
			return gcvSTATUS_OK;
		}
	}

    for (paramName = slsDLINK_LIST_First(
                            &PolynaryExpr->funcName->u.funcInfo.localSpace->names, slsNAME),
            operand = slsDLINK_LIST_First(&PolynaryExpr->operands->members, struct _sloIR_EXPR);
        (slsDLINK_NODE *)paramName != &PolynaryExpr->funcName->u.funcInfo.localSpace->names;
        paramName = slsDLINK_NODE_Next(&paramName->node, slsNAME),
            operand = slsDLINK_NODE_Next(&operand->base.node, struct _sloIR_EXPR))
    {
        if (paramName->type != slvPARAMETER_NAME) break;

        if ((slsDLINK_NODE *)operand == &PolynaryExpr->operands->members)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            PolynaryExpr->exprBase.base.lineNo,
                                            PolynaryExpr->exprBase.base.stringNo,
                                            slvREPORT_ERROR,
                                            "require more arguments"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}

        if (!slsDATA_TYPE_IsEqual(paramName->dataType, operand->dataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            operand->base.lineNo,
                                            operand->base.stringNo,
                                            slvREPORT_ERROR,
                                            "require the same typed argument"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}

        switch (paramName->dataType->qualifier)
        {
        case slvQUALIFIER_OUT:
        case slvQUALIFIER_INOUT:
            status = _CheckErrorAsLValueExpr(Compiler, operand);

			if (gcmIS_ERROR(status))
            {
                gcmFOOTER();
                return status;
            }

            break;
        }
    }

    if ((slsDLINK_NODE *)operand != &PolynaryExpr->operands->members)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        operand->base.lineNo,
                                        operand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "too many arguments"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

sloIR_EXPR
slParseFuncCallExprAsExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR FuncCall
    )
{
    gceSTATUS       status;
    sloIR_CONSTANT  constant;

    gcmHEADER_ARG("Compiler=0x%x FuncCall=0x%x",
                  Compiler, FuncCall);

	if (FuncCall == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    switch (FuncCall->type)
    {
    case slvPOLYNARY_CONSTRUCT_FLOAT:
    case slvPOLYNARY_CONSTRUCT_INT:
    case slvPOLYNARY_CONSTRUCT_UINT:
    case slvPOLYNARY_CONSTRUCT_BOOL:
        status = _CheckErrorAsScalarConstructor(Compiler, FuncCall);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case slvPOLYNARY_CONSTRUCT_VEC2:
    case slvPOLYNARY_CONSTRUCT_VEC3:
    case slvPOLYNARY_CONSTRUCT_VEC4:
    case slvPOLYNARY_CONSTRUCT_BVEC2:
    case slvPOLYNARY_CONSTRUCT_BVEC3:
    case slvPOLYNARY_CONSTRUCT_BVEC4:
    case slvPOLYNARY_CONSTRUCT_IVEC2:
    case slvPOLYNARY_CONSTRUCT_IVEC3:
    case slvPOLYNARY_CONSTRUCT_IVEC4:
    case slvPOLYNARY_CONSTRUCT_UVEC2:
    case slvPOLYNARY_CONSTRUCT_UVEC3:
    case slvPOLYNARY_CONSTRUCT_UVEC4:
        status = _CheckErrorAsVectorOrMatrixConstructor(Compiler, FuncCall, gcvTRUE);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case slvPOLYNARY_CONSTRUCT_MAT2:
    case slvPOLYNARY_CONSTRUCT_MAT2X3:
    case slvPOLYNARY_CONSTRUCT_MAT2X4:
    case slvPOLYNARY_CONSTRUCT_MAT3:
    case slvPOLYNARY_CONSTRUCT_MAT3X2:
    case slvPOLYNARY_CONSTRUCT_MAT3X4:
    case slvPOLYNARY_CONSTRUCT_MAT4:
    case slvPOLYNARY_CONSTRUCT_MAT4X2:
    case slvPOLYNARY_CONSTRUCT_MAT4X3:
        status = _CheckErrorAsVectorOrMatrixConstructor(Compiler, FuncCall, gcvFALSE);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case slvPOLYNARY_CONSTRUCT_STRUCT:
        status = _CheckErrorAsStructConstructor(Compiler, FuncCall);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case slvPOLYNARY_FUNC_CALL:
        status = sloCOMPILER_BindFuncCall(Compiler, FuncCall);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        status = _CheckErrorAsFuncCall(Compiler, FuncCall);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    default:
        gcmASSERT(0);
    }

    /* Try to evaluate it */
    status = sloIR_POLYNARY_EXPR_Evaluate(
                                        Compiler,
                                        FuncCall,
                                        &constant);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmFOOTER_ARG("<return>=0x%x", (constant != gcvNULL)? &constant->exprBase : &FuncCall->exprBase);
	return (constant != gcvNULL)? &constant->exprBase : &FuncCall->exprBase;
}

gceSTATUS
_ParseComponentSelection(
    IN sloCOMPILER Compiler,
    IN gctUINT8 VectorSize,
    IN slsLexToken * FieldSelection,
    OUT slsCOMPONENT_SELECTION * ComponentSelection
    )
{
    gctUINT8        i;
    gctUINT8        nameSets[4];
    sleCOMPONENT    components[4];
	gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x VectorSize=%u FieldSelection=0x%x ComponentSelection=0x%x",
                  Compiler, VectorSize, FieldSelection, ComponentSelection);


    gcmASSERT(VectorSize <= 4);
    gcmASSERT(FieldSelection);
    gcmASSERT(FieldSelection->u.fieldSelection);
    gcmASSERT(ComponentSelection);

    for (i = 0; FieldSelection->u.fieldSelection[i] != '\0'; i++)
    {
        if(i >= 4)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            FieldSelection->lineNo,
                                            FieldSelection->stringNo,
                                            slvREPORT_ERROR,
                                            "more than 4 components are selected : \"%s\"",
                                            FieldSelection->u.fieldSelection));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
        }

        switch (FieldSelection->u.fieldSelection[i])
        {
        case 'x': nameSets[i] = 0; components[i] = slvCOMPONENT_X; break;
        case 'y': nameSets[i] = 0; components[i] = slvCOMPONENT_Y; break;
        case 'z': nameSets[i] = 0; components[i] = slvCOMPONENT_Z; break;
        case 'w': nameSets[i] = 0; components[i] = slvCOMPONENT_W; break;

        case 'r': nameSets[i] = 1; components[i] = slvCOMPONENT_X; break;
        case 'g': nameSets[i] = 1; components[i] = slvCOMPONENT_Y; break;
        case 'b': nameSets[i] = 1; components[i] = slvCOMPONENT_Z; break;
        case 'a': nameSets[i] = 1; components[i] = slvCOMPONENT_W; break;

        case 's': nameSets[i] = 2; components[i] = slvCOMPONENT_X; break;
        case 't': nameSets[i] = 2; components[i] = slvCOMPONENT_Y; break;
        case 'p': nameSets[i] = 2; components[i] = slvCOMPONENT_Z; break;
        case 'q': nameSets[i] = 2; components[i] = slvCOMPONENT_W; break;

        default:
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            FieldSelection->lineNo,
                                            FieldSelection->stringNo,
                                            slvREPORT_ERROR,
                                            "invalid component name: '%c'",
                                            FieldSelection->u.fieldSelection[i]));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
        }
    }

    ComponentSelection->components = i;

    for (i = 1; i < ComponentSelection->components; i++)
    {
        if (nameSets[i] != nameSets[0])
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            FieldSelection->lineNo,
                                            FieldSelection->stringNo,
                                            slvREPORT_ERROR,
                                            "the component name: '%c'"
                                            " do not come from the same set",
                                            FieldSelection->u.fieldSelection[i]));

            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }
    }

    for (i = 0; i < ComponentSelection->components; i++)
    {
        if ((gctUINT8)components[i] >= VectorSize)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            FieldSelection->lineNo,
                                            FieldSelection->stringNo,
                                            slvREPORT_ERROR,
                                            "the component: '%c' beyond the specified vector type",
                                            FieldSelection->u.fieldSelection[i]));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
        }

        switch (i)
        {
        case 0: ComponentSelection->x = components[0]; break;
        case 1: ComponentSelection->y = components[1]; break;
        case 2: ComponentSelection->z = components[2]; break;
        case 3: ComponentSelection->w = components[3]; break;

        default:
            gcmASSERT(0);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static slePOLYNARY_EXPR_TYPE
_ConvToPolynaryExprType(
    IN gctINT TokenType
    )
{
    switch (TokenType)
    {
    case T_FLOAT:   return slvPOLYNARY_CONSTRUCT_FLOAT;
    case T_INT:     return slvPOLYNARY_CONSTRUCT_INT;
    case T_UINT:     return slvPOLYNARY_CONSTRUCT_UINT;
    case T_BOOL:    return slvPOLYNARY_CONSTRUCT_BOOL;
    case T_VEC2:    return slvPOLYNARY_CONSTRUCT_VEC2;
    case T_VEC3:    return slvPOLYNARY_CONSTRUCT_VEC3;
    case T_VEC4:    return slvPOLYNARY_CONSTRUCT_VEC4;
    case T_BVEC2:   return slvPOLYNARY_CONSTRUCT_BVEC2;
    case T_BVEC3:   return slvPOLYNARY_CONSTRUCT_BVEC3;
    case T_BVEC4:   return slvPOLYNARY_CONSTRUCT_BVEC4;
    case T_IVEC2:   return slvPOLYNARY_CONSTRUCT_IVEC2;
    case T_IVEC3:   return slvPOLYNARY_CONSTRUCT_IVEC3;
    case T_IVEC4:   return slvPOLYNARY_CONSTRUCT_IVEC4;
    case T_UVEC2:   return slvPOLYNARY_CONSTRUCT_UVEC2;
    case T_UVEC3:   return slvPOLYNARY_CONSTRUCT_UVEC3;
    case T_UVEC4:   return slvPOLYNARY_CONSTRUCT_UVEC4;
    case T_MAT2:    return slvPOLYNARY_CONSTRUCT_MAT2;
    case T_MAT2X3:    return slvPOLYNARY_CONSTRUCT_MAT2X3;
    case T_MAT2X4:    return slvPOLYNARY_CONSTRUCT_MAT2X4;
    case T_MAT3:    return slvPOLYNARY_CONSTRUCT_MAT3;
    case T_MAT3X2:    return slvPOLYNARY_CONSTRUCT_MAT3X2;
    case T_MAT3X4:    return slvPOLYNARY_CONSTRUCT_MAT3X4;
    case T_MAT4:    return slvPOLYNARY_CONSTRUCT_MAT4;
    case T_MAT4X2:    return slvPOLYNARY_CONSTRUCT_MAT4X2;
    case T_MAT4X3:    return slvPOLYNARY_CONSTRUCT_MAT4X3;

    default:
        gcmASSERT(0);
        return slvPOLYNARY_CONSTRUCT_FLOAT;
    }
}

sloIR_POLYNARY_EXPR
slParseFuncCallHeaderExpr(
    IN sloCOMPILER Compiler,
    IN slsLexToken * FuncIdentifier
    )
{
    gceSTATUS               status;
    slePOLYNARY_EXPR_TYPE   exprType;
    slsDATA_TYPE *          dataType    = gcvNULL;
    sltPOOL_STRING          funcSymbol  = gcvNULL;
    sloIR_POLYNARY_EXPR     polynaryExpr;

    gcmHEADER_ARG("Compiler=0x%x FuncIdentifier=0x%x",
                  Compiler, FuncIdentifier);

    gcmASSERT(FuncIdentifier);

    switch (FuncIdentifier->type)
    {
    case T_FLOAT:
    case T_INT:
    case T_UINT:
    case T_BOOL:
    case T_VEC2:
    case T_VEC3:
    case T_VEC4:
    case T_BVEC2:
    case T_BVEC3:
    case T_BVEC4:
    case T_IVEC2:
    case T_IVEC3:
    case T_IVEC4:
    case T_UVEC2:
    case T_UVEC3:
    case T_UVEC4:
    case T_MAT2:
    case T_MAT2X3:
    case T_MAT2X4:
    case T_MAT3:
    case T_MAT3X2:
    case T_MAT3X4:
    case T_MAT4:
    case T_MAT4X2:
    case T_MAT4X3:
        exprType    = _ConvToPolynaryExprType(FuncIdentifier->type);

        status = sloCOMPILER_CreateDataType(
                                            Compiler,
                                            FuncIdentifier->type,
                                            gcvNULL,
                                            &dataType);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        dataType->qualifier = slvQUALIFIER_CONST;

        break;

    case T_TYPE_NAME:
        exprType    = slvPOLYNARY_CONSTRUCT_STRUCT;

        status = sloCOMPILER_CloneDataType(
                                        Compiler,
                                        slvQUALIFIER_CONST,
                                        FuncIdentifier->u.typeName->dataType,
                                        &dataType);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case T_IDENTIFIER:
        exprType    = slvPOLYNARY_FUNC_CALL;

        funcSymbol  = FuncIdentifier->u.identifier;

        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    /* Create polynary expression */
    status = sloIR_POLYNARY_EXPR_Construct(
                                        Compiler,
                                        FuncIdentifier->lineNo,
                                        FuncIdentifier->stringNo,
                                        exprType,
                                        dataType,
                                        funcSymbol,
                                        &polynaryExpr);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<FUNC_CALL_HEADER type=\"%s\" line=\"%d\" string=\"%d\" />",
                                slGetIRPolynaryExprTypeName(exprType),
                                FuncIdentifier->lineNo,
                                FuncIdentifier->stringNo));

    gcmFOOTER_ARG("<return>=0x%x", polynaryExpr);
    return polynaryExpr;
}

sloIR_POLYNARY_EXPR
slParseFuncCallArgument(
    IN sloCOMPILER Compiler,
    IN sloIR_POLYNARY_EXPR FuncCall,
    IN sloIR_EXPR Argument
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("Compiler=0x%x FuncCall=0x%x Argument=0x%x",
                  Compiler, FuncCall, Argument);

    if (FuncCall == gcvNULL || Argument == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    if (FuncCall->operands == gcvNULL)
    {
        status = sloIR_SET_Construct(
                                    Compiler,
                                    Argument->base.lineNo,
                                    Argument->base.stringNo,
                                    slvEXPR_SET,
                                    &FuncCall->operands);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }
	}

    gcmASSERT(FuncCall->operands);
    gcmVERIFY_OK(sloIR_SET_AddMember(
                                    Compiler,
                                    FuncCall->operands,
                                    &Argument->base));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<FUNC_CALL_ARGUMENT />"));

    gcmFOOTER_ARG("<return>=0x%x", FuncCall);
	return FuncCall;
}

sloIR_EXPR
slParseFieldSelectionExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR Operand,
    IN slsLexToken * FieldSelection
    )
{
    gceSTATUS               status;
    sleUNARY_EXPR_TYPE      exprType;
    slsNAME *               fieldName = gcvNULL;
    slsCOMPONENT_SELECTION  componentSelection;
    sloIR_CONSTANT          resultConstant;
    sloIR_UNARY_EXPR        unaryExpr;

    gcmHEADER_ARG("Compiler=0x%x Operand=0x%x FieldSelection=0x%x",
                  Compiler, Operand, FieldSelection);

    gcmASSERT(FieldSelection);

    if (Operand == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    if (slsDATA_TYPE_IsStruct(Operand->dataType))
    {
        gcmASSERT(Operand->dataType->fieldSpace);

        exprType = slvUNARY_FIELD_SELECTION;

        status = slsNAME_SPACE_Search(
                                    Compiler,
                                    Operand->dataType->fieldSpace,
                                    FieldSelection->u.fieldSelection,
                                    gcvFALSE,
                                    &fieldName);

        if (status != gcvSTATUS_OK)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            FieldSelection->lineNo,
                                            FieldSelection->stringNo,
                                            slvREPORT_ERROR,
                                            "unknown field: '%s'",
                                            FieldSelection->u.fieldSelection));
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        gcmASSERT(fieldName->type == slvFIELD_NAME);
    }
    else if (slsDATA_TYPE_IsBVecOrIVecOrVec(Operand->dataType))
    {
        exprType = slvUNARY_COMPONENT_SELECTION;

        status = _ParseComponentSelection(Compiler,
                                          slmDATA_TYPE_vectorSize_NOCHECK_GET(Operand->dataType),
                                          FieldSelection,
                                          &componentSelection);

        if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }
    }
    else
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Operand->base.lineNo,
                                        Operand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a struct or vector typed expression"));

        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    /* Constant calculation */
    if (sloIR_OBJECT_GetType(&Operand->base) == slvIR_CONSTANT)
    {
        status = sloIR_UNARY_EXPR_Evaluate(
                                            Compiler,
                                            exprType,
                                            (sloIR_CONSTANT)Operand,
                                            fieldName,
                                            &componentSelection,
                                            &resultConstant);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}

        gcmFOOTER_ARG("<return>=0x%x", &resultConstant->exprBase);
		return &resultConstant->exprBase;
	}

    /* Create unary expression */
    status = sloIR_UNARY_EXPR_Construct(
                                        Compiler,
                                        Operand->base.lineNo,
                                        Operand->base.stringNo,
                                        exprType,
                                        Operand,
                                        fieldName,
                                        &componentSelection,
                                        &unaryExpr);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<UNARY_EXPR type=\"%s\" line=\"%d\" string=\"%d\""
                                " fieldSelection=\"%s\" />",
                                slGetIRUnaryExprTypeName(exprType),
                                Operand->base.lineNo,
                                Operand->base.stringNo,
                                FieldSelection->u.fieldSelection));

    gcmFOOTER_ARG("<return>=0x%x", &unaryExpr->exprBase);
	return &unaryExpr->exprBase;
}

gceSTATUS
_CheckErrorForIncOrDecExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR Operand
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x Operand=0x%x",
                  Compiler, Operand);

    gcmASSERT(Operand);
    gcmASSERT(Operand->dataType);

    /* Check the operand */
    status = _CheckErrorAsLValueExpr(Compiler, Operand);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    if (!slsDATA_TYPE_IsIntOrIVec(Operand->dataType)
        && !slsDATA_TYPE_IsFloatOrVecOrMat(Operand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Operand->base.lineNo,
                                        Operand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an integer or floating-point typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

sloIR_EXPR
slParseIncOrDecExpr(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN sleUNARY_EXPR_TYPE ExprType,
    IN sloIR_EXPR Operand
    )
{
    gceSTATUS           status;
    gctUINT             lineNo;
    gctUINT             stringNo;
    sloIR_UNARY_EXPR    unaryExpr;

    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x ExprType=%d Operand=0x%x",
                  Compiler, StartToken, ExprType, Operand);

	if (Operand == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    if (StartToken != gcvNULL)
    {
        lineNo      = StartToken->lineNo;
        stringNo    = StartToken->stringNo;
    }
    else
    {
        lineNo      = Operand->base.lineNo;
        stringNo    = Operand->base.stringNo;
    }

    /* Check Error */
    status = _CheckErrorForIncOrDecExpr(
                                        Compiler,
                                        Operand);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    /* Create unary expression */
    status = sloIR_UNARY_EXPR_Construct(
                                        Compiler,
                                        lineNo,
                                        stringNo,
                                        ExprType,
                                        Operand,
                                        gcvNULL,
                                        gcvNULL,
                                        &unaryExpr);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<UNARY_EXPR type=\"%s\" line=\"%d\" string=\"%d\" />",
                                slGetIRUnaryExprTypeName(ExprType),
                                lineNo,
                                stringNo));

    gcmFOOTER_ARG("<return>=0x%x", &unaryExpr->exprBase);
    return &unaryExpr->exprBase;
}

static gceSTATUS
_CheckErrorForPosOrNegExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR Operand
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x Operand=0x%x",
                  Compiler, Operand);

    gcmASSERT(Operand);
    gcmASSERT(Operand->dataType);

    /* Check the operand */
    if (!slsDATA_TYPE_IsIntOrIVec(Operand->dataType)
        && !slsDATA_TYPE_IsFloatOrVecOrMat(Operand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Operand->base.lineNo,
                                        Operand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an integer or floating-point typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorForNotExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR Operand
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x Operand=0x%x",
                  Compiler, Operand);

    gcmASSERT(Operand);
    gcmASSERT(Operand->dataType);

    /* Check the operand */
    if (!slsDATA_TYPE_IsBool(Operand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Operand->base.lineNo,
                                        Operand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar boolean expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

sloIR_EXPR
slParseNormalUnaryExpr(
    IN sloCOMPILER Compiler,
    IN slsLexToken * Operator,
    IN sloIR_EXPR Operand
    )
{
    gceSTATUS           status;
    sleUNARY_EXPR_TYPE  exprType = slvUNARY_NEG;
    sloIR_CONSTANT      resultConstant;
    sloIR_UNARY_EXPR    unaryExpr;

    gcmHEADER_ARG("Compiler=0x%x Operator=0x%x Operand=0x%x",
                  Compiler, Operator, Operand);

    gcmASSERT(Operator);

	if (Operand == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    switch (Operator->u.operator)
    {
    case '-':
        exprType = slvUNARY_NEG;
        /* Fall through. */

    case '+':
        status = _CheckErrorForPosOrNegExpr(
                                            Compiler,
                                            Operand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case '!':
        exprType = slvUNARY_NOT;

        status = _CheckErrorForNotExpr(
                                    Compiler,
                                    Operand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case '~':
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Operator->lineNo,
                                        Operator->stringNo,
                                        slvREPORT_ERROR,
                                        "reserved unary operator '~'"));

        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;

	default:
		gcmASSERT(0);
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

	if (Operator->u.operator == '+')
	{
        /* No action needed for '+' operator. */
        gcmFOOTER_ARG("<return>=0x%x", Operand);
		return Operand;
	}

    /* Constant calculation */
    if (sloIR_OBJECT_GetType(&Operand->base) == slvIR_CONSTANT)
    {
        status = sloIR_UNARY_EXPR_Evaluate(
                                            Compiler,
                                            exprType,
                                            (sloIR_CONSTANT)Operand,
                                            gcvNULL,
                                            gcvNULL,
                                            &resultConstant);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}

        gcmFOOTER_ARG("<return>=0x%x", &resultConstant->exprBase);
		return &resultConstant->exprBase;
	}

    /* Create unary expression */
    status = sloIR_UNARY_EXPR_Construct(
                                        Compiler,
                                        Operator->lineNo,
                                        Operator->stringNo,
                                        exprType,
                                        Operand,
                                        gcvNULL,
                                        gcvNULL,
                                        &unaryExpr);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<UNARY_EXPR type=\"%c\" line=\"%d\" string=\"%d\" />",
                                Operator->u.operator,
                                Operator->lineNo,
                                Operator->stringNo));

    gcmFOOTER_ARG("<return>=0x%x", &unaryExpr->exprBase);
	return &unaryExpr->exprBase;
}

static gctCONST_STRING
_GetBinaryOperatorName(
    IN gctINT TokenType
    )
{
    switch (TokenType)
    {
    case '*':               return "*";
    case '/':               return "/";
    case '%':               return "%";

    case '+':               return "+";
    case '-':               return "-";

    case T_LEFT_OP:         return "<<";
    case T_RIGHT_OP:        return ">>";

    case '<':               return "<";
    case '>':               return ">";

    case T_LE_OP:           return "<=";
    case T_GE_OP:           return ">=";

    case T_EQ_OP:           return "==";
    case T_NE_OP:           return "!=";

    case '&':               return "&";
    case '^':               return "^";
    case '|':               return "|";

    case T_AND_OP:          return "&&";
    case T_XOR_OP:          return "^^";
    case T_OR_OP:           return "||";

    case ',':               return ",";

    case '=':               return "=";

    case T_MUL_ASSIGN:      return "*=";
    case T_DIV_ASSIGN:      return "/=";
    case T_MOD_ASSIGN:      return "%=";
    case T_ADD_ASSIGN:      return "+=";
    case T_SUB_ASSIGN:      return "-=";
    case T_LEFT_ASSIGN:     return "<<=";
    case T_RIGHT_ASSIGN:    return ">>=";
    case T_AND_ASSIGN:      return "&=";
    case T_XOR_ASSIGN:      return "^=";
    case T_OR_ASSIGN:       return "|=";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

static gceSTATUS
_CheckErrorForArithmeticExpr(
    IN sloCOMPILER Compiler,
    IN gctBOOL IsMul,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x IsMul=%d LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, IsMul, LeftOperand, RightOperand);

    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);

    /* Check the operands */
    if (!slsDATA_TYPE_IsIntOrIVec(LeftOperand->dataType)
        && !slsDATA_TYPE_IsFloatOrVecOrMat(LeftOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an integer or floating-point typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (!slsDATA_TYPE_IsIntOrIVec(RightOperand->dataType)
        && !slsDATA_TYPE_IsFloatOrVecOrMat(RightOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        RightOperand->base.lineNo,
                                        RightOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an integer or floating-point typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (!slsDATA_TYPE_IsEqual(LeftOperand->dataType, RightOperand->dataType))
    {
        if (slsDATA_TYPE_IsInt(LeftOperand->dataType))
        {
            if (!slsDATA_TYPE_IsIVec(RightOperand->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                RightOperand->base.lineNo,
                                                RightOperand->base.stringNo,
                                                slvREPORT_ERROR,
                                                "require an integer typed expression"));

				status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
				return status;
            }
        }
        else if (slsDATA_TYPE_IsIVec(LeftOperand->dataType))
        {
            if (!slsDATA_TYPE_IsInt(RightOperand->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                RightOperand->base.lineNo,
                                                RightOperand->base.stringNo,
                                                slvREPORT_ERROR,
                                                "require an int or ivec%d expression",
                                                slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType)));

				status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
				return status;
            }
        }
        else if (slsDATA_TYPE_IsFloat(LeftOperand->dataType))
        {
            if (!slsDATA_TYPE_IsVecOrMat(RightOperand->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                RightOperand->base.lineNo,
                                                RightOperand->base.stringNo,
                                                slvREPORT_ERROR,
                                                "require a float or vec or mat expression"));

				status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
				return status;
            }
        }
        else if (slsDATA_TYPE_IsVec(LeftOperand->dataType))
        {
            if (!IsMul)
            {
                if (!slsDATA_TYPE_IsFloat(RightOperand->dataType))
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    RightOperand->base.lineNo,
                                                    RightOperand->base.stringNo,
                                                    slvREPORT_ERROR,
                                                    "require a float or vec%d expression",
                                                    slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType)));

					status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
					return status;
                }
            }
            else
            {
                if (!slsDATA_TYPE_IsFloat(RightOperand->dataType)
                    && !(slsDATA_TYPE_IsMat(RightOperand->dataType)
                        && slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType)
                        == slmDATA_TYPE_matrixRowCount_GET(RightOperand->dataType)))
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    RightOperand->base.lineNo,
                                                    RightOperand->base.stringNo,
                                                    slvREPORT_ERROR,
                                                    "require a float or vec%d or mat%d expression",
                                                    slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType),
                                                    slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType)));

					status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
					return status;
                }
            }
        }
        else if (slsDATA_TYPE_IsMat(LeftOperand->dataType))
        {
            if (!IsMul)
            {
                if (!slsDATA_TYPE_IsFloat(RightOperand->dataType))
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    RightOperand->base.lineNo,
                                                    RightOperand->base.stringNo,
                                                    slvREPORT_ERROR,
                                                    "require a float or mat%d expression",
                                                    LeftOperand->dataType->matrixSize));

					status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
					return status;
                }
            }
            else
            {
                if (!slsDATA_TYPE_IsFloat(RightOperand->dataType)
                    && !(slsDATA_TYPE_IsVec(RightOperand->dataType)
                        && slmDATA_TYPE_matrixColumnCount_GET(LeftOperand->dataType)
                        == slmDATA_TYPE_vectorSize_NOCHECK_GET(RightOperand->dataType)))
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    RightOperand->base.lineNo,
                                                    RightOperand->base.stringNo,
                                                    slvREPORT_ERROR,
                                                    "require a float or vec%d or mat%d expression",
                                                    LeftOperand->dataType->matrixSize,
                                                    LeftOperand->dataType->matrixSize));

					status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
					return status;
                }
            }
        }
        else
        {
            gcmASSERT(0);
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
        }
    }

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorForRelationalExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, RightOperand);

    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);

    /* Check the operands */
    if (!slsDATA_TYPE_IsInt(LeftOperand->dataType)
        && !slsDATA_TYPE_IsFloat(LeftOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar integer or scalar floating-point expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (slsDATA_TYPE_IsInt(LeftOperand->dataType)
        && !slsDATA_TYPE_IsInt(RightOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        RightOperand->base.lineNo,
                                        RightOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar integer expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
    }
    else if (slsDATA_TYPE_IsFloat(LeftOperand->dataType)
        && !slsDATA_TYPE_IsFloat(RightOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        RightOperand->base.lineNo,
                                        RightOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar floating-point expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorForEqualityExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, RightOperand);

    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);

    /* Check the operands */
    if (!slsDATA_TYPE_IsAssignableAndComparable(LeftOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require any typed expression except arrays, structures"
                                        " containing arrays, sampler types, and structures"
                                        " containing sampler types"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (!slsDATA_TYPE_IsEqual(LeftOperand->dataType, RightOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        RightOperand->base.lineNo,
                                        RightOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a matching typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorForLogicalExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, RightOperand);

    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);

    /* Check the operands */
    if (!slsDATA_TYPE_IsBool(LeftOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar boolean expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (!slsDATA_TYPE_IsBool(RightOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        RightOperand->base.lineNo,
                                        RightOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar boolean expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorForSequenceExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, RightOperand);

    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

sloIR_EXPR
slParseNormalBinaryExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN slsLexToken * Operator,
    IN sloIR_EXPR RightOperand
    )
{
    gceSTATUS           status;
    sleBINARY_EXPR_TYPE exprType = (sleBINARY_EXPR_TYPE) 0;
    sloIR_CONSTANT      resultConstant;
    sloIR_BINARY_EXPR   binaryExpr;

    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x Operator=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, Operator, RightOperand);

    gcmASSERT(Operator);

	if (LeftOperand == gcvNULL || RightOperand == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    switch (Operator->u.operator)
    {
    case '%':
    case T_LEFT_OP:
    case T_RIGHT_OP:
    case '&':
    case '^':
    case '|':
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Operator->lineNo,
                                        Operator->stringNo,
                                        slvREPORT_ERROR,
                                        "reserved binary operator '%s'",
                                        _GetBinaryOperatorName(Operator->u.operator)));

        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;

    case '*':
    case '/':
    case '+':
    case '-':
        switch (Operator->u.operator)
        {
        case '*': exprType = slvBINARY_MUL; break;
        case '/': exprType = slvBINARY_DIV; break;
        case '+': exprType = slvBINARY_ADD; break;
        case '-': exprType = slvBINARY_SUB; break;
        }

        status = _CheckErrorForArithmeticExpr(
                                            Compiler,
                                            (Operator->u.operator == '*'),
                                            LeftOperand,
                                            RightOperand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case '<':
    case '>':
    case T_LE_OP:
    case T_GE_OP:
        switch (Operator->u.operator)
        {
        case '<':       exprType = slvBINARY_LESS_THAN;             break;
        case '>':       exprType = slvBINARY_GREATER_THAN;          break;
        case T_LE_OP:   exprType = slvBINARY_LESS_THAN_EQUAL;       break;
        case T_GE_OP:   exprType = slvBINARY_GREATER_THAN_EQUAL;    break;
        }

        status = _CheckErrorForRelationalExpr(
                                            Compiler,
                                            LeftOperand,
                                            RightOperand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case T_EQ_OP:
    case T_NE_OP:
        switch (Operator->u.operator)
        {
        case T_EQ_OP: exprType = slvBINARY_EQUAL;       break;
        case T_NE_OP: exprType = slvBINARY_NOT_EQUAL;   break;
        }

        status = _CheckErrorForEqualityExpr(
                                        Compiler,
                                        LeftOperand,
                                        RightOperand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case T_AND_OP:
    case T_XOR_OP:
    case T_OR_OP:
        switch (Operator->u.operator)
        {
        case T_AND_OP:  exprType = slvBINARY_AND;   break;
        case T_XOR_OP:  exprType = slvBINARY_XOR;   break;
        case T_OR_OP:   exprType = slvBINARY_OR;    break;
        }

        status = _CheckErrorForLogicalExpr(
                                        Compiler,
                                        LeftOperand,
                                        RightOperand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case ',':
        exprType = slvBINARY_SEQUENCE;

        status = _CheckErrorForSequenceExpr(
                                            Compiler,
                                            LeftOperand,
                                            RightOperand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        if (sloIR_OBJECT_GetType(&LeftOperand->base) == slvIR_CONSTANT)
        {
            gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &LeftOperand->base));

            gcmFOOTER_ARG("<return>=0x%x", RightOperand);
			return RightOperand;
		}

        break;

	default:
		gcmASSERT(0);
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    /* Constant calculation */
    if (sloIR_OBJECT_GetType(&LeftOperand->base) == slvIR_CONSTANT
        && sloIR_OBJECT_GetType(&RightOperand->base) == slvIR_CONSTANT)
    {
        status = sloIR_BINARY_EXPR_Evaluate(
                                            Compiler,
                                            exprType,
                                            (sloIR_CONSTANT)LeftOperand,
                                            (sloIR_CONSTANT)RightOperand,
                                            &resultConstant);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}

        gcmFOOTER_ARG("<return>=0x%x", &resultConstant->exprBase);
		return &resultConstant->exprBase;
	}

    /* Create binary expression */
    status = sloIR_BINARY_EXPR_Construct(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        exprType,
                                        LeftOperand,
                                        RightOperand,
                                        &binaryExpr);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<BINARY_EXPR type=\"%s\" line=\"%d\" string=\"%d\" />",
                                _GetBinaryOperatorName(Operator->u.operator),
                                LeftOperand->base.lineNo,
                                LeftOperand->base.stringNo));

    gcmFOOTER_ARG("<return>=0x%x", &binaryExpr->exprBase);
	return &binaryExpr->exprBase;
}

static gceSTATUS
_CheckErrorForSelectionExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR CondExpr,
    IN sloIR_EXPR TrueOperand,
    IN sloIR_EXPR FalseOperand
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x CondExpr=0x%x TrueOperand=0x%x FalseOperand=0x%x",
                  Compiler, CondExpr, TrueOperand, FalseOperand);

    gcmASSERT(CondExpr);
    gcmASSERT(CondExpr->dataType);
    gcmASSERT(TrueOperand);
    gcmASSERT(TrueOperand->dataType);
    gcmASSERT(FalseOperand);
    gcmASSERT(FalseOperand->dataType);

    /* Check the operands */
    if (!slsDATA_TYPE_IsBool(CondExpr->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        CondExpr->base.lineNo,
                                        CondExpr->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar boolean expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (slsDATA_TYPE_IsArray(TrueOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        TrueOperand->base.lineNo,
                                        TrueOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a non-array expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (!slsDATA_TYPE_IsEqual(TrueOperand->dataType, FalseOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        FalseOperand->base.lineNo,
                                        FalseOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a matching typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

sloIR_EXPR
slParseSelectionExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR CondExpr,
    IN sloIR_EXPR TrueOperand,
    IN sloIR_EXPR FalseOperand
    )
{
    gceSTATUS       status;
    sloIR_SELECTION selection;
    sloIR_CONSTANT  condConstant;
    gctBOOL         condValue;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x CondExpr=0x%x TrueOperand=0x%x FalseOperand=0x%x",
                  Compiler, CondExpr, TrueOperand, FalseOperand);

	if (CondExpr == gcvNULL || TrueOperand == gcvNULL || FalseOperand == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    /* Check error */
    status = _CheckErrorForSelectionExpr(
                                        Compiler,
                                        CondExpr,
                                        TrueOperand,
                                        FalseOperand);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    /* Constant calculation */
    if (sloIR_OBJECT_GetType(&CondExpr->base) == slvIR_CONSTANT)
    {
        condConstant = (sloIR_CONSTANT)CondExpr;
        gcmASSERT(condConstant->valueCount == 1);
        gcmASSERT(condConstant->values);

        condValue = condConstant->values[0].boolValue;

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &CondExpr->base));

        if (condValue)
        {
            gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &FalseOperand->base));
            gcmFOOTER_ARG("<return>=0x%x", TrueOperand);
            return TrueOperand;
        }
        else
        {
            gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &TrueOperand->base));
            gcmFOOTER_ARG("<return>=0x%x", FalseOperand);
            return FalseOperand;
        }
    }

    /* Create the selection */
    status = sloCOMPILER_CloneDataType(
                                    Compiler,
                                    slvQUALIFIER_CONST,
                                    TrueOperand->dataType,
                                    &dataType);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    status = sloIR_SELECTION_Construct(
                                    Compiler,
                                    CondExpr->base.lineNo,
                                    CondExpr->base.stringNo,
                                    dataType,
                                    CondExpr,
                                    &TrueOperand->base,
                                    &FalseOperand->base,
                                    &selection);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<SELECTION_EXPR line=\"%d\" string=\"%d\" condExpr=\"0x%x\""
                                " TrueOperand=\"0x%x\" FalseOperand=\"0x%x\" />",
                                CondExpr->base.lineNo,
                                CondExpr->base.stringNo,
                                CondExpr,
                                TrueOperand,
                                FalseOperand));

    gcmFOOTER_ARG("<return>=0x%x", &selection->exprBase);
	return &selection->exprBase;
}

static gceSTATUS
_CheckErrorForAssignmentExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, RightOperand);

    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand->dataType);

    /* Check the left operand */
    status = _CheckErrorAsLValueExpr(Compiler, LeftOperand);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    /* Check the operands */
    if (!slsDATA_TYPE_IsAssignableAndComparable(LeftOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require any typed expression except arrays, structures"
                                        " containing arrays, sampler types, and structures"
                                        " containing sampler types"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (!slsDATA_TYPE_IsEqual(LeftOperand->dataType, RightOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        RightOperand->base.lineNo,
                                        RightOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a matching typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_CheckErrorForArithmeticAssignmentExpr(
    IN sloCOMPILER Compiler,
    IN gctBOOL IsMul,
    IN sloIR_EXPR LeftOperand,
    IN sloIR_EXPR RightOperand
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Compiler=0x%x IsMul=%d LeftOperand=0x%x RightOperand=0x%x",
                  Compiler, IsMul, LeftOperand, RightOperand);

    gcmASSERT(LeftOperand);
    gcmASSERT(LeftOperand->dataType);
    gcmASSERT(RightOperand);
    gcmASSERT(RightOperand->dataType);

    /* Check the left operand */
    status = _CheckErrorAsLValueExpr(Compiler, LeftOperand);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    /* Check the operands */
    if (!slsDATA_TYPE_IsIntOrIVec(LeftOperand->dataType)
        && !slsDATA_TYPE_IsFloatOrVecOrMat(LeftOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an integer or floating-point typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (!slsDATA_TYPE_IsIntOrIVec(RightOperand->dataType)
        && !slsDATA_TYPE_IsFloatOrVecOrMat(RightOperand->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        RightOperand->base.lineNo,
                                        RightOperand->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require an integer or floating-point typed expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    if (!slsDATA_TYPE_IsEqual(LeftOperand->dataType, RightOperand->dataType))
    {
        if (slsDATA_TYPE_IsInt(LeftOperand->dataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            RightOperand->base.lineNo,
                                            RightOperand->base.stringNo,
                                            slvREPORT_ERROR,
                                            "require a scalar integer expression"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
        }
        else if (slsDATA_TYPE_IsIVec(LeftOperand->dataType))
        {
            if (!slsDATA_TYPE_IsInt(RightOperand->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                RightOperand->base.lineNo,
                                                RightOperand->base.stringNo,
                                                slvREPORT_ERROR,
                                                "require an int or ivec%d expression",
                                                slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType)));

				status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
				return status;
            }
        }
        else if (slsDATA_TYPE_IsFloat(LeftOperand->dataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            RightOperand->base.lineNo,
                                            RightOperand->base.stringNo,
                                            slvREPORT_ERROR,
                                            "require a scalar float expression"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
        }
        else if (slsDATA_TYPE_IsVec(LeftOperand->dataType))
        {
            if (!IsMul)
            {
                if (!slsDATA_TYPE_IsFloat(RightOperand->dataType))
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    RightOperand->base.lineNo,
                                                    RightOperand->base.stringNo,
                                                    slvREPORT_ERROR,
                                                    "require a float or vec%d expression",
                                                    slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType)));

					status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
					return status;
                }
            }
            else
            {
                if (!slsDATA_TYPE_IsFloat(RightOperand->dataType)
                    && !(slsDATA_TYPE_IsMat(RightOperand->dataType)
                        && slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType)
                        == slmDATA_TYPE_matrixRowCount_GET(RightOperand->dataType)))
                {
                    gcmVERIFY_OK(sloCOMPILER_Report(
                                                    Compiler,
                                                    RightOperand->base.lineNo,
                                                    RightOperand->base.stringNo,
                                                    slvREPORT_ERROR,
                                                    "require a float or vec%d or mat%d expression",
                                                    slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType),
                                                    slmDATA_TYPE_vectorSize_NOCHECK_GET(LeftOperand->dataType)));

					status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
					return status;
                }
            }
        }
        else if (slsDATA_TYPE_IsMat(LeftOperand->dataType))
        {
            if (!slsDATA_TYPE_IsFloat(RightOperand->dataType))
            {
                gcmVERIFY_OK(sloCOMPILER_Report(
                                                Compiler,
                                                RightOperand->base.lineNo,
                                                RightOperand->base.stringNo,
                                                slvREPORT_ERROR,
                                                "require a float or mat%d expression",
                                                LeftOperand->dataType->matrixSize));

				status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
				return status;
			}
		}
		else {
			gcmASSERT(0);
			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}


sloIR_EXPR
slParseAssignmentExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR LeftOperand,
    IN slsLexToken * Operator,
    IN sloIR_EXPR RightOperand
    )
{
    gceSTATUS           status;
    sleBINARY_EXPR_TYPE exprType = (sleBINARY_EXPR_TYPE) 0;
    sloIR_BINARY_EXPR   binaryExpr;

    gcmHEADER_ARG("Compiler=0x%x LeftOperand=0x%x Operator=0x%x RightOperand=0x%x",
                  Compiler, LeftOperand, Operator, RightOperand);

    gcmASSERT(Operator);

	if (LeftOperand == gcvNULL || RightOperand == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    switch (Operator->u.operator)
    {
    case T_MOD_ASSIGN:
    case T_LEFT_ASSIGN:
    case T_RIGHT_ASSIGN:
    case T_AND_ASSIGN:
    case T_XOR_ASSIGN:
    case T_OR_ASSIGN:
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Operator->lineNo,
                                        Operator->stringNo,
                                        slvREPORT_ERROR,
                                        "reserved binary operator '%s'",
                                        _GetBinaryOperatorName(Operator->u.operator)));

        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;

    case '=':
        exprType = slvBINARY_ASSIGN;

        status = _CheckErrorForAssignmentExpr(
                                            Compiler,
                                            LeftOperand,
                                            RightOperand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

    case T_MUL_ASSIGN:
    case T_DIV_ASSIGN:
    case T_ADD_ASSIGN:
    case T_SUB_ASSIGN:
        switch (Operator->u.operator)
        {
        case T_MUL_ASSIGN:  exprType = slvBINARY_MUL_ASSIGN;    break;
        case T_DIV_ASSIGN:  exprType = slvBINARY_DIV_ASSIGN;    break;
        case T_ADD_ASSIGN:  exprType = slvBINARY_ADD_ASSIGN;    break;
        case T_SUB_ASSIGN:  exprType = slvBINARY_SUB_ASSIGN;    break;
        }

        status = _CheckErrorForArithmeticAssignmentExpr(
                                                        Compiler,
                                                        (Operator->u.operator == T_MUL_ASSIGN),
                                                        LeftOperand,
                                                        RightOperand);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }

        break;

	default:
		gcmASSERT(0);
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    /* Create binary expression */
    status = sloIR_BINARY_EXPR_Construct(
                                        Compiler,
                                        LeftOperand->base.lineNo,
                                        LeftOperand->base.stringNo,
                                        exprType,
                                        LeftOperand,
                                        RightOperand,
                                        &binaryExpr);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<BINARY_EXPR type=\"%s\" line=\"%d\" string=\"%d\" />",
                                _GetBinaryOperatorName(Operator->u.operator),
                                LeftOperand->base.lineNo,
                                LeftOperand->base.stringNo));

    gcmFOOTER_ARG("<return>=0x%x", &binaryExpr->exprBase);
	return &binaryExpr->exprBase;
}

static gctCONST_STRING
_GetNonStructTypeName(
    IN gctINT TokenType
    )
{
    switch (TokenType)
    {
    case T_VOID:        return "void";
    case T_FLOAT:       return "float";
    case T_INT:         return "int";
    case T_UINT:        return "unsigned int";
    case T_BOOL:        return "bool";
    case T_VEC2:        return "vec2";
    case T_VEC3:        return "vec3";
    case T_VEC4:        return "vec4";
    case T_BVEC2:       return "bvec2";
    case T_BVEC3:       return "bvec3";
    case T_BVEC4:       return "bvec4";
    case T_IVEC2:       return "ivec2";
    case T_IVEC3:       return "ivec3";
    case T_IVEC4:       return "ivec4";
    case T_UVEC2:       return "uvec2";
    case T_UVEC3:       return "uvec3";
    case T_UVEC4:       return "uvec4";
    case T_MAT2:        return "mat2";
    case T_MAT3:        return "mat3";
    case T_MAT4:        return "mat4";
    case T_SAMPLER2D:   return "sampler2D";
    case T_SAMPLERCUBE: return "samplerCube";

    case T_SAMPLER3D:	return "sampler3D";
    case T_SAMPLER1DARRAY: return "sampler1DArray";
    case T_SAMPLER2DARRAY: return "sampler2DArray";
    case T_SAMPLER1DARRAYSHADOW: return "sampler1DArrayShadow";
    case T_SAMPLER2DARRAYSHADOW: return "sampler2DArrayShadow";

    case T_ISAMPLER2D:   return "isampler2D";
    case T_ISAMPLERCUBE: return "isamplerCube";
    case T_ISAMPLER3D:	return "isampler3D";
    case T_ISAMPLER2DARRAY: return "isampler2DArray";

    case T_USAMPLER2D:   return "usampler2D";
    case T_USAMPLERCUBE: return "usamplerCube";
    case T_USAMPLER3D:	return "usampler3D";
    case T_USAMPLER2DARRAY: return "usampler2DArray";
    case T_SAMPLEREXTERNALOES: return "samplerExternalOES";

    default:
        gcmASSERT(0);
        return "invalid";
    }
}

static gceSTATUS
_ParseNonArrayVariableDecl(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier
    )
{
    gceSTATUS           status;
    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x",
                  Compiler, DataType, Identifier);


    gcmASSERT(DataType);
    gcmASSERT(Identifier);

    if (DataType->qualifier == slvQUALIFIER_CONST)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Identifier->lineNo,
                                        Identifier->stringNo,
                                        slvREPORT_ERROR,
                                        "require the initializer for the 'const' variable: '%s'",
                                        Identifier->u.identifier));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    status = sloCOMPILER_CreateName(
                                    Compiler,
                                    Identifier->lineNo,
                                    Identifier->stringNo,
                                    slvVARIABLE_NAME,
                                    DataType,
                                    Identifier->u.identifier,
                                    slvEXTENSION_NONE,
                                    gcvNULL);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER();
        return status;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<VARIABLE_DECL line=\"%d\" string=\"%d\" name=\"%s\" />",
                                Identifier->lineNo,
                                Identifier->stringNo,
                                Identifier->u.identifier));

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_ParseArrayVariableDecl(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR ArrayLengthExpr
    )
{
    gceSTATUS       status;
    gctUINT         arrayLength;
    slsDATA_TYPE *  arrayDataType;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x ArrayLengthExpr=0x%x",
                  Compiler, DataType, Identifier, ArrayLengthExpr);


    gcmASSERT(DataType);
    gcmASSERT(Identifier);
    gcmASSERT(ArrayLengthExpr);

    switch (DataType->qualifier)
    {
    case slvQUALIFIER_CONST:
    case slvQUALIFIER_ATTRIBUTE:
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        Identifier->lineNo,
                                        Identifier->stringNo,
                                        slvREPORT_ERROR,
                                        "cannot declare the array: '%s' with the '%s' qualifier",
                                        Identifier->u.identifier,
                                        slGetQualifierName(DataType->qualifier)));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    status = _EvaluateExprToArrayLength(
                                        Compiler,
                                        ArrayLengthExpr,
                                        &arrayLength);

	if (gcmIS_ERROR(status)) {
        gcmFOOTER();
		return status;
	}

    status = sloCOMPILER_CreateArrayDataType(
                                            Compiler,
                                            DataType,
                                            arrayLength,
                                            &arrayDataType);

	if (gcmIS_ERROR(status)) {
        gcmFOOTER();
		return status;
	}

    status = sloCOMPILER_CreateName(
                                    Compiler,
                                    Identifier->lineNo,
                                    Identifier->stringNo,
                                    slvVARIABLE_NAME,
                                    arrayDataType,
                                    Identifier->u.identifier,
                                    slvEXTENSION_NONE,
                                    gcvNULL);

	if (gcmIS_ERROR(status)) {
        gcmFOOTER();
		return status;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<VARIABLE_DECL line=\"%d\" string=\"%d\" name=\"%s\" />",
                                Identifier->lineNo,
                                Identifier->stringNo,
                                Identifier->u.identifier));

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

slsDeclOrDeclList
slParseNonArrayVariableDecl(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier
    )
{
    gceSTATUS           status;
    slsDeclOrDeclList   declOrDeclList;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x",
                  Compiler, DataType, Identifier);


    gcmASSERT(Identifier);

    declOrDeclList.dataType         = DataType;
    declOrDeclList.initStatement    = gcvNULL;
    declOrDeclList.initStatements   = gcvNULL;

	if (declOrDeclList.dataType == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
        return declOrDeclList;
    }

    status = _ParseNonArrayVariableDecl(
                                        Compiler,
                                        DataType,
                                        Identifier);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
        return declOrDeclList;
    }

    gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
	return declOrDeclList;
}

slsDeclOrDeclList
slParseArrayVariableDecl(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR ArrayLengthExpr
    )
{
    gceSTATUS           status;
    slsDeclOrDeclList   declOrDeclList;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x ArrayLengthExpr=0x%x",
                  Compiler, DataType, Identifier, ArrayLengthExpr);

    gcmASSERT(Identifier);

    declOrDeclList.dataType         = DataType;
    declOrDeclList.initStatement    = gcvNULL;
    declOrDeclList.initStatements   = gcvNULL;

	if (declOrDeclList.dataType == gcvNULL || ArrayLengthExpr == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
        return declOrDeclList;
    }

    status = _ParseArrayVariableDecl(
                                    Compiler,
                                    DataType,
                                    Identifier,
                                    ArrayLengthExpr);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
		return declOrDeclList;
	}

    gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
	return declOrDeclList;
}

slsDeclOrDeclList
slParseNonArrayVariableDecl2(
    IN sloCOMPILER Compiler,
    IN slsDeclOrDeclList DeclOrDeclList,
    IN slsLexToken * Identifier
    )
{
    gceSTATUS           status;
    slsDeclOrDeclList   declOrDeclList;

    gcmHEADER_ARG("Compiler=0x%x DeclOrDeclList=0x%x Identifier=0x%x",
                  Compiler, DeclOrDeclList, Identifier);

    gcmASSERT(Identifier);

    declOrDeclList = DeclOrDeclList;

	if (declOrDeclList.dataType == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
        return declOrDeclList;
    }

    status = _ParseNonArrayVariableDecl(
                                        Compiler,
                                        declOrDeclList.dataType,
                                        Identifier);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
		return declOrDeclList;
	}

    gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
	return declOrDeclList;
}

slsDeclOrDeclList
slParseArrayVariableDecl2(
    IN sloCOMPILER Compiler,
    IN slsDeclOrDeclList DeclOrDeclList,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR ArrayLengthExpr
    )
{
    gceSTATUS           status;
    slsDeclOrDeclList   declOrDeclList;

    gcmHEADER_ARG("Compiler=0x%x DeclOrDeclList=0x%x Identifier=0x%x ArrayLengthExpr=0x%x",
                  Compiler, DeclOrDeclList, Identifier, ArrayLengthExpr);

    gcmASSERT(Identifier);

    declOrDeclList = DeclOrDeclList;

	if (declOrDeclList.dataType == gcvNULL || ArrayLengthExpr == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
        return declOrDeclList;
    }

    status = _ParseArrayVariableDecl(
                                    Compiler,
                                    declOrDeclList.dataType,
                                    Identifier,
                                    ArrayLengthExpr);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
		return declOrDeclList;
	}

    gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
	return declOrDeclList;
}

static gceSTATUS
_ParseVariableDeclWithInitializer(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR Initializer,
    OUT sloIR_EXPR * InitExpr
    )
{
    gceSTATUS           status;
    slsNAME *           name;
    sloIR_VARIABLE      variable;
    sloIR_BINARY_EXPR   binaryExpr;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x Initializer=0x%x InitExpr=0x%x",
                  Compiler, DataType, Identifier, Initializer, InitExpr);

    gcmASSERT(DataType);
    gcmASSERT(Identifier);
    gcmASSERT(Initializer);
    gcmASSERT(InitExpr);

    /* Create the name */
    status = sloCOMPILER_CreateName(
                                    Compiler,
                                    Identifier->lineNo,
                                    Identifier->stringNo,
                                    slvVARIABLE_NAME,
                                    DataType,
                                    Identifier->u.identifier,
                                    slvEXTENSION_NONE,
                                    &name);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER();
		return status;
	}

    if (DataType->qualifier == slvQUALIFIER_CONST)
    {
        status = _CheckErrorAsConstantExpr(Compiler, Initializer, &name->u.variableInfo.constant);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER();
			return status;
		}

        *InitExpr = Initializer;
    }
    else
    {
        /* Create the variable */
        status = sloIR_VARIABLE_Construct(
                                        Compiler,
                                        Identifier->lineNo,
                                        Identifier->stringNo,
                                        name,
                                        &variable);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER();
			return status;
		}

        status = _CheckErrorForAssignmentExpr(
                                            Compiler,
                                            &variable->exprBase,
                                            Initializer);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER();
			return status;
		}

        /* Create the assigement expression */
        status = sloIR_BINARY_EXPR_Construct(
                                            Compiler,
                                            Identifier->lineNo,
                                            Identifier->stringNo,
                                            slvBINARY_ASSIGN,
                                            &variable->exprBase,
                                            Initializer,
                                            &binaryExpr);

		if (gcmIS_ERROR(status))
		{
            gcmFOOTER();
			return status;
		}

        *InitExpr = &binaryExpr->exprBase;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<VARIABLE_DECL_WITH_INITIALIZER line=\"%d\" string=\"%d\""
                                " dataType=\"0x%x\" identifier=\"%s\" initializer=\"0x%x\" />",
                                Identifier->lineNo,
                                Identifier->stringNo,
                                DataType,
                                Identifier->u.identifier,
                                Initializer));

    gcmFOOTER_ARG("*InitExpr=0x%x", *InitExpr);
	return gcvSTATUS_OK;
}

slsDeclOrDeclList
slParseVariableDeclWithInitializer(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR Initializer
    )
{
    gceSTATUS           status;
    slsDeclOrDeclList   declOrDeclList;
    sloIR_EXPR          initExpr;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x Initializer=0x%x",
                  Compiler, DataType, Identifier, Initializer);

    gcmASSERT(Identifier);

    declOrDeclList.dataType         = DataType;
    declOrDeclList.initStatement    = gcvNULL;
    declOrDeclList.initStatements   = gcvNULL;

    if (declOrDeclList.dataType == gcvNULL || Initializer == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
		return declOrDeclList;
    }

    status = _ParseVariableDeclWithInitializer(
                                            Compiler,
                                            declOrDeclList.dataType,
                                            Identifier,
                                            Initializer,
                                            &initExpr);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
		return declOrDeclList;
	}

    declOrDeclList.initStatement = &initExpr->base;

    gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
	return declOrDeclList;
}

slsDeclOrDeclList
slParseVariableDeclWithInitializer2(
    IN sloCOMPILER Compiler,
    IN slsDeclOrDeclList DeclOrDeclList,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR Initializer
    )
{
    gceSTATUS           status;
    slsDeclOrDeclList   declOrDeclList;
    sloIR_EXPR          initExpr;
    sloIR_BASE          initStatement;

    gcmHEADER_ARG("Compiler=0x%x DeclOrDeclList=0x%x Identifier=0x%x Initializer=0x%x",
                  Compiler, DeclOrDeclList, Identifier, Initializer);

    gcmASSERT(Identifier);

    declOrDeclList = DeclOrDeclList;

	if (declOrDeclList.dataType == gcvNULL || Initializer == gcvNULL)
	{
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
		return declOrDeclList;
	}

    if (declOrDeclList.initStatement != gcvNULL)
    {
        gcmASSERT(declOrDeclList.initStatements == gcvNULL);

        status = sloIR_SET_Construct(
                                    Compiler,
                                    declOrDeclList.initStatement->lineNo,
                                    declOrDeclList.initStatement->stringNo,
                                    slvDECL_SET,
                                    &declOrDeclList.initStatements);

		if (gcmIS_ERROR(status))
		{
			gcmASSERT(declOrDeclList.initStatements == gcvNULL);
            gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
			return declOrDeclList;
		}

        gcmVERIFY_OK(sloIR_SET_AddMember(
                                        Compiler,
                                        declOrDeclList.initStatements,
                                        declOrDeclList.initStatement));

        declOrDeclList.initStatement = gcvNULL;
    }

    status = _ParseVariableDeclWithInitializer(
                                            Compiler,
                                            declOrDeclList.dataType,
                                            Identifier,
                                            Initializer,
                                            &initExpr);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
		return declOrDeclList;
	}

    initStatement = &initExpr->base;

    if (declOrDeclList.initStatements != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_SET_AddMember(
                                        Compiler,
                                        declOrDeclList.initStatements,
                                        initStatement));
    }
    else
    {
        gcmASSERT(declOrDeclList.initStatement == gcvNULL);

        declOrDeclList.initStatement = initStatement;
    }

    gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
	return declOrDeclList;
}

slsNAME *
slParseFuncHeader(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier
    )
{
    gceSTATUS       status;
    slsNAME *       name;
    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x",
                  Compiler, DataType, Identifier);

    gcmASSERT(Identifier);

	if (DataType == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    status = sloCOMPILER_CreateName(
                                    Compiler,
                                    Identifier->lineNo,
                                    Identifier->stringNo,
                                    slvFUNC_NAME,
                                    DataType,
                                    Identifier->u.identifier,
                                    slvEXTENSION_NONE,
                                    &name);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    status = sloCOMPILER_CreateNameSpace(
                                        Compiler,
                                        &name->u.funcInfo.localSpace);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<FUNCTION line=\"%d\" string=\"%d\" name=\"%s\">",
                                Identifier->lineNo,
                                Identifier->stringNo,
                                Identifier->u.identifier));

    gcmFOOTER_ARG("<return>=0x%x", name);
	return name;
}

sloIR_BASE
slParseFuncDecl(
    IN sloCOMPILER Compiler,
    IN slsNAME * FuncName
    )
{
    gceSTATUS   status;

    gcmHEADER_ARG("Compiler=0x%x FuncName=0x%x",
                  Compiler, FuncName);

	if (FuncName == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    sloCOMPILER_PopCurrentNameSpace(Compiler, gcvNULL);

    FuncName->u.funcInfo.isFuncDef = gcvFALSE;

    status = sloCOMPILER_CheckNewFuncName(
                                        Compiler,
                                        FuncName,
                                        gcvNULL);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "</FUNCTION>"));

    gcmFOOTER_ARG("<return>=%s", "<nil>");
	return gcvNULL;
}

sloIR_BASE
slParseDeclaration(
    IN sloCOMPILER Compiler,
    IN slsDeclOrDeclList DeclOrDeclList
    )
{
    gcmHEADER_ARG("Compiler=0x%x DeclOrDeclList=0x%x",
                  Compiler, DeclOrDeclList);

    if (DeclOrDeclList.initStatement != gcvNULL)
    {
        gcmASSERT(DeclOrDeclList.initStatements == gcvNULL);

        gcmFOOTER_ARG("<return>=0x%x", DeclOrDeclList.initStatement);
		return DeclOrDeclList.initStatement;
    }
    else if (DeclOrDeclList.initStatements != gcvNULL)
    {
        gcmASSERT(DeclOrDeclList.initStatement == gcvNULL);

        gcmFOOTER_ARG("<return>=0x%x", &DeclOrDeclList.initStatements->base);
		return &DeclOrDeclList.initStatements->base;
    }
    else
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
    }
}

sloIR_BASE
slParseDefaultPrecisionQualifier(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN slsLexToken * PrecisionQualifier,
    IN slsDATA_TYPE * DataType
    )
{
    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x PrecisionQualifier=0x%x DataType=0x%x",
                  Compiler, StartToken, PrecisionQualifier, DataType);

    gcmASSERT(StartToken);
    gcmASSERT(PrecisionQualifier);

	if (DataType == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    sloCOMPILER_SetDefaultPrecision(Compiler,
                                      DataType->elementType,
                                      PrecisionQualifier->u.precision);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<DEFAULT_PRECISION line=\"%d\" string=\"%d\""
                                " precision=\"%d\" dataType=\"0x%x\" />",
                                StartToken->lineNo,
                                StartToken->stringNo,
                                PrecisionQualifier->u.precision,
                                DataType));

    gcmFOOTER_ARG("<return>=%s", "<nil>");
	return gcvNULL;
}

void
slParseCompoundStatementBegin(
    IN sloCOMPILER Compiler
    )
{
    gceSTATUS       status;
    slsNAME_SPACE * nameSpace;

    gcmHEADER_ARG("Compiler=0x%x",
                  Compiler);

    status = sloCOMPILER_CreateNameSpace(
                                        Compiler,
                                        &nameSpace);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_NO();
		return;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<COMPOUND_STATEMENT>"));
    gcmFOOTER_NO();
}

sloIR_SET
slParseCompoundStatementEnd(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN sloIR_SET Set
    )
{
    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x Set=0x%x",
                  Compiler, StartToken, Set);

    gcmASSERT(StartToken);

	if (Set == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    sloCOMPILER_PopCurrentNameSpace(Compiler, gcvNULL);

    Set->base.lineNo    = StartToken->lineNo;
    Set->base.stringNo  = StartToken->stringNo;

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "</COMPOUND_STATEMENT>"));

    gcmFOOTER_ARG("<return>=0x%x", Set);
	return Set;
}

void
slParseCompoundStatementNoNewScopeBegin(
    IN sloCOMPILER Compiler
    )
{
    gcmHEADER_ARG("Compiler=0x%x",
                  Compiler);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<COMPOUND_STATEMENT_NO_NEW_SCOPE>"));
    gcmFOOTER_NO();
}

sloIR_SET
slParseCompoundStatementNoNewScopeEnd(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN sloIR_SET Set
    )
{
    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x Set=0x%x",
                  Compiler, StartToken, Set);

    gcmASSERT(StartToken);

	if (Set == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    Set->base.lineNo    = StartToken->lineNo;
    Set->base.stringNo  = StartToken->stringNo;

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "</COMPOUND_STATEMENT_NO_NEW_SCOPE>"));

    gcmFOOTER_ARG("<return>=0x%x", Set);
	return Set;
}

sloIR_SET
slParseStatementList(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE Statement
    )
{
    gceSTATUS status;
    sloIR_SET set;

    gcmHEADER_ARG("Compiler=0x%x Statement=0x%x",
                  Compiler, Statement);

    status = sloIR_SET_Construct(
                                Compiler,
                                0,
                                0,
                                slvSTATEMENT_SET,
                                &set);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    if (Statement != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_SET_AddMember(
                                        Compiler,
                                        set,
                                        Statement));
    }

    gcmFOOTER_ARG("<return>=0x%x", set);
	return set;
}

sloIR_SET
slParseStatementList2(
    IN sloCOMPILER Compiler,
    IN sloIR_SET Set,
    IN sloIR_BASE Statement
    )
{
    gcmHEADER_ARG("Compiler=0x%x Set=0x%x Statement=0x%x",
                  Compiler, Set, Statement);

    if (Set != gcvNULL && Statement != gcvNULL)
    {
        gcmVERIFY_OK(sloIR_SET_AddMember(
                                        Compiler,
                                        Set,
                                        Statement));
    }

    gcmFOOTER_ARG("<return>=0x%x", Set);
	return Set;
}

sloIR_BASE
slParseExprAsStatement(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR Expr
    )
{
    gcmHEADER_ARG("Compiler=0x%x Expr=0x%x",
                  Compiler, Expr);

    if (Expr == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<EXPRESSION_STATEMENT expr=\"0x%x\" />",
                                Expr));

    gcmFOOTER_ARG("<return>=0x%x", &Expr->base);
	return &Expr->base;
}

sloIR_BASE
slParseCompoundStatementAsStatement(
    IN sloCOMPILER Compiler,
    IN sloIR_SET CompoundStatement
    )
{
    gcmHEADER_ARG("Compiler=0x%x CompoundStatement=0x%x",
                  Compiler, CompoundStatement);

	if (CompoundStatement == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<STATEMENT compoundStatement=\"0x%x\" />",
                                CompoundStatement));

    gcmFOOTER_ARG("<return>=0x%x", &CompoundStatement->base);
	return &CompoundStatement->base;
}

sloIR_BASE
slParseCompoundStatementNoNewScopeAsStatementNoNewScope(
    IN sloCOMPILER Compiler,
    IN sloIR_SET CompoundStatementNoNewScope
    )
{
    gcmHEADER_ARG("Compiler=0x%x CompoundStatementNoNewScope=0x%x",
                  Compiler, CompoundStatementNoNewScope);

	if (CompoundStatementNoNewScope == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<STATEMENT_NO_NEW_SCOPE compoundStatementNoNewScope=\"0x%x\" />",
                                CompoundStatementNoNewScope));

    gcmFOOTER_ARG("<return>=0x%x", &CompoundStatementNoNewScope->base);
	return &CompoundStatementNoNewScope->base;
}

slsSelectionStatementPair
slParseSelectionRestStatement(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE TrueStatement,
    IN sloIR_BASE FalseStatement
    )
{
    slsSelectionStatementPair pair;
    gcmHEADER_ARG("Compiler=0x%x TrueStatement=0x%x FalseStatement=0x%x",
                   Compiler, TrueStatement, FalseStatement);

    pair.trueStatement  = TrueStatement;
    pair.falseStatement = FalseStatement;

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<SELECTION_REST_STATEMENT trueStatement=\"0x%x\""
                                " falseStatement=\"0x%x\" />",
                                TrueStatement,
                                FalseStatement));

    gcmFOOTER_ARG("<return>=0x%x", pair);
	return pair;
}

static gceSTATUS
_CheckErrorForCondExpr(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR CondExpr
    )
{
	gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x CondExpr=0x%x",
                  Compiler, CondExpr);

    gcmASSERT(CondExpr);
    gcmASSERT(CondExpr->dataType);

    /* Check the operand */
    if (!slsDATA_TYPE_IsBool(CondExpr->dataType))
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        CondExpr->base.lineNo,
                                        CondExpr->base.stringNo,
                                        slvREPORT_ERROR,
                                        "require a scalar boolean expression"));

		status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
		return status;
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

sloIR_BASE
slParseSelectionStatement(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN sloIR_EXPR CondExpr,
    IN slsSelectionStatementPair SelectionStatementPair
    )
{
    gceSTATUS       status;
    sloIR_SELECTION selection;
    sloIR_CONSTANT  condConstant;
    gctBOOL         condValue;
    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x CondExpr=0x%x SelectionStatementPair=0x%x",
                  Compiler, StartToken, CondExpr, SelectionStatementPair);

    gcmASSERT(StartToken);

	if (CondExpr == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    /* Check error */
    status = _CheckErrorForCondExpr(
                                    Compiler,
                                    CondExpr);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    /* Constant calculation */
    if (sloIR_OBJECT_GetType(&CondExpr->base) == slvIR_CONSTANT)
    {
        condConstant = (sloIR_CONSTANT)CondExpr;
        gcmASSERT(condConstant->valueCount == 1);
        gcmASSERT(condConstant->values);

        condValue = condConstant->values[0].boolValue;

        gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, &CondExpr->base));

        if (condValue)
        {
            if (SelectionStatementPair.falseStatement != gcvNULL)
            {
                gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, SelectionStatementPair.falseStatement));
            }

            gcmFOOTER_ARG("<return>=0x%x", SelectionStatementPair.trueStatement);
			return SelectionStatementPair.trueStatement;

        }
        else
        {
            if (SelectionStatementPair.trueStatement != gcvNULL)
            {
                gcmVERIFY_OK(sloIR_OBJECT_Destroy(Compiler, SelectionStatementPair.trueStatement));
            }

            gcmFOOTER_ARG("<return>=0x%x", SelectionStatementPair.trueStatement);
			return SelectionStatementPair.falseStatement;
        }
    }

    /* Create the selection */
    status = sloIR_SELECTION_Construct(
                                    Compiler,
                                    StartToken->lineNo,
                                    StartToken->stringNo,
                                    gcvNULL,
                                    CondExpr,
                                    SelectionStatementPair.trueStatement,
                                    SelectionStatementPair.falseStatement,
                                    &selection);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<SELECTION_STATEMENT line=\"%d\" string=\"%d\" condExpr=\"0x%x\""
                                " trueStatement=\"0x%x\" falseStatement=\"0x%x\" />",
                                StartToken->lineNo,
                                StartToken->stringNo,
                                CondExpr,
                                SelectionStatementPair.trueStatement,
                                SelectionStatementPair.falseStatement));

    gcmFOOTER_ARG("<return>=0x%x", &selection->exprBase.base);
	return &selection->exprBase.base;
}

void
slParseWhileStatementBegin(
    IN sloCOMPILER Compiler
    )
{
    gceSTATUS       status;
    slsNAME_SPACE * nameSpace;

    gcmHEADER_ARG("Compiler=0x%x",
                  Compiler);

    status = sloCOMPILER_CreateNameSpace(
                                        Compiler,
                                        &nameSpace);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_NO();
		return;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<WHILE_STATEMENT>"));
    gcmFOOTER_NO();
}

sloIR_BASE
slParseWhileStatementEnd(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN sloIR_EXPR CondExpr,
    IN sloIR_BASE LoopBody
    )
{
    gceSTATUS           status;
    sloIR_ITERATION     iteration;

    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x CondExpr=0x%x LoopBody=0x%x",
                  Compiler, StartToken, CondExpr, LoopBody);

    gcmASSERT(StartToken);

    sloCOMPILER_PopCurrentNameSpace(Compiler, gcvNULL);

    /* Check error */
    if (CondExpr == gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        StartToken->lineNo,
                                        StartToken->stringNo,
                                        slvREPORT_ERROR,
                                        "while statement has no condition"));

        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    status = _CheckErrorForCondExpr(
                                    Compiler,
                                    CondExpr);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    /* Create the iteration */
    status = sloIR_ITERATION_Construct(
                                    Compiler,
                                    StartToken->lineNo,
                                    StartToken->stringNo,
                                    slvWHILE,
                                    CondExpr,
                                    LoopBody,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    &iteration);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "</WHILE_STATEMENT>"));

    gcmFOOTER_ARG("<return>=0x%x", &iteration->base);
	return &iteration->base;
}

sloIR_BASE
slParseDoWhileStatement(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN sloIR_BASE LoopBody,
    IN sloIR_EXPR CondExpr
    )
{
    gceSTATUS       status;
    sloIR_ITERATION iteration;

    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x LoopBody=0x%x CondExpr=0x%x",
                  Compiler, StartToken, LoopBody, CondExpr);

    gcmASSERT(StartToken);

    if (CondExpr == gcvNULL)
    {
        gcmVERIFY_OK(sloCOMPILER_Report(
                                        Compiler,
                                        StartToken->lineNo,
                                        StartToken->stringNo,
                                        slvREPORT_ERROR,
                                        "do-while statement has no condition"));

        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    status = _CheckErrorForCondExpr(
                                    Compiler,
                                    CondExpr);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    status = sloIR_ITERATION_Construct(
                                    Compiler,
                                    StartToken->lineNo,
                                    StartToken->stringNo,
                                    slvDO_WHILE,
                                    CondExpr,
                                    LoopBody,
                                    gcvNULL,
                                    gcvNULL,
                                    gcvNULL,
                                    &iteration);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<DO_WHILE_STATEMENT line=\"%d\" string=\"%d\""
                                " condExpr=\"0x%x\" LoopBody=\"0x%x\" />",
                                StartToken->lineNo,
                                StartToken->stringNo,
                                CondExpr,
                                LoopBody));

    gcmFOOTER_ARG("<return>=0x%x", &iteration->base);
	return &iteration->base;
}

void
slParseForStatementBegin(
    IN sloCOMPILER Compiler)
{
    gceSTATUS       status;
    slsNAME_SPACE * nameSpace;

    gcmHEADER_ARG("Compiler=0x%x",
                  Compiler);

    status = sloCOMPILER_CreateNameSpace(
                                        Compiler,
                                        &nameSpace);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_NO();
		return;
	}

	gcmVERIFY_OK(sloCOMPILER_Dump(
								Compiler,
								slvDUMP_PARSER,
								"<FOR_STATEMENT>"));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<FOR_STATEMENT>"));

    gcmFOOTER_NO();
}

sloIR_BASE
slParseForStatementEnd(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN sloIR_BASE ForInitStatement,
    IN slsForExprPair ForExprPair,
    IN sloIR_BASE LoopBody
    )
{
    gceSTATUS       status;
    sloIR_ITERATION iteration;
    slsNAME_SPACE * forSpace = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x ForInitStatement=0x%x ForExprPair=0x%x LoopBody=0x%x",
                  Compiler, StartToken, ForInitStatement, ForExprPair, LoopBody);

    gcmASSERT(StartToken);

    sloCOMPILER_PopCurrentNameSpace(Compiler, &forSpace);

    if (ForExprPair.condExpr != gcvNULL)
    {
        status = _CheckErrorForCondExpr(
                                        Compiler,
                                        ForExprPair.condExpr);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
            return gcvNULL;
        }
	}

    status = sloIR_ITERATION_Construct(
                                    Compiler,
                                    StartToken->lineNo,
                                    StartToken->stringNo,
                                    slvFOR,
                                    ForExprPair.condExpr,
                                    LoopBody,
                                    forSpace,
                                    ForInitStatement,
                                    ForExprPair.restExpr,
                                    &iteration);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "</FOR_STATEMENT>"));

    gcmFOOTER_ARG("<return>=0x%x", &iteration->base);
	return &iteration->base;
}

sloIR_EXPR
slParseCondition(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR Initializer
)
{
    sloIR_EXPR      initExpr = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x Initializer=0x%x",
                  Compiler, DataType, Identifier, Initializer);

    gcmASSERT(Identifier);

	if (DataType == gcvNULL || Initializer == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(
        _ParseVariableDeclWithInitializer(Compiler,
                                          DataType,
                                          Identifier,
                                          Initializer,
                                          &initExpr));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<CONDITION line=\"%d\" string=\"%d\""
                                " dataType=\"0x%x\" identifier=\"%s\" initializer=\"0x%x\" />",
                                Identifier->lineNo,
                                Identifier->stringNo,
                                DataType,
                                Identifier->u.identifier,
                                Initializer));

    gcmFOOTER_ARG("<return>=0x%x", initExpr);
	return initExpr;
}

slsForExprPair
slParseForRestStatement(
    IN sloCOMPILER Compiler,
    IN sloIR_EXPR CondExpr,
    IN sloIR_EXPR RestExpr
    )
{
    slsForExprPair pair;

    gcmHEADER_ARG("Compiler=0x%x CondExpr=0x%x RestExpr=0x%x",
                  Compiler, CondExpr, RestExpr);

    pair.condExpr   = CondExpr;
    pair.restExpr   = RestExpr;

    gcmFOOTER_ARG("<return>=0x%x", pair);
	return pair;
}

gceSTATUS
_CheckErrorForJump(
    IN sloCOMPILER Compiler,
    IN gctUINT LineNo,
    IN gctUINT StringNo,
    IN sleJUMP_TYPE Type,
    IN sloIR_EXPR ReturnExpr
    )
{
    sleSHADER_TYPE  shaderType;
	gceSTATUS       status = gcvSTATUS_OK;

    gcmHEADER_ARG("Compiler=0x%x LineNo=%u StringNo=%u Type=%d ReturnExpr=0x%x",
                   Compiler, LineNo, StringNo, Type, ReturnExpr);

    if (Type == slvDISCARD)
    {
        gcmVERIFY_OK(sloCOMPILER_GetShaderType(Compiler, &shaderType));

        if (shaderType != slvSHADER_TYPE_FRAGMENT)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            LineNo,
                                            StringNo,
                                            slvREPORT_ERROR,
                                            "'discard' is only allowed "
                                            "within the fragment shaders"));

			status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

sloIR_BASE
slParseJumpStatement(
    IN sloCOMPILER Compiler,
    IN sleJUMP_TYPE Type,
    IN slsLexToken * StartToken,
    IN sloIR_EXPR ReturnExpr
    )
{
    gceSTATUS       status;
    sloIR_JUMP      jump;

    gcmHEADER_ARG("Compiler=0x%x Type=%d StartToken=0x%x ReturnExpr=0x%x",
                  Compiler, Type, StartToken, ReturnExpr);

    gcmASSERT(StartToken);

    status = _CheckErrorForJump(
                                Compiler,
                                StartToken->lineNo,
                                StartToken->stringNo,
                                Type,
                                ReturnExpr);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    status = sloIR_JUMP_Construct(
                                Compiler,
                                StartToken->lineNo,
                                StartToken->stringNo,
                                Type,
                                ReturnExpr,
                                &jump);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<JUMP line=\"%d\" string=\"%d\""
                                " type=\"%s\" returnExpr=\"0x%x\" />",
                                StartToken->lineNo,
                                StartToken->stringNo,
                                slGetIRJumpTypeName(Type),
                                ReturnExpr));

    gcmFOOTER_ARG("<return>=0x%x", &jump->base);
	return &jump->base;
}

void
slParseExternalDecl(
    IN sloCOMPILER Compiler,
    IN sloIR_BASE Decl
    )
{
    gcmHEADER_ARG("Compiler=0x%x Decl=0x%x",
                  Compiler, Decl);

	if (Decl == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    gcmVERIFY_OK(sloCOMPILER_AddExternalDecl(Compiler, Decl));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<EXTERNAL_DECL decl=\"0x%x\" />",
                                Decl));

    gcmFOOTER_NO();
}

void
slParseFuncDef(
    IN sloCOMPILER Compiler,
    IN slsNAME * FuncName,
    IN sloIR_SET Statements
    )
{
    gceSTATUS   status;
    slsNAME *   firstFuncName;

    gcmHEADER_ARG("Compiler=0x%x FuncName=0x%x Statements=0x%x",
                  Compiler, FuncName, Statements);

	if (FuncName == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    if (Statements == gcvNULL)
    {
        status = sloIR_SET_Construct(
                                    Compiler,
                                    FuncName->lineNo,
                                    FuncName->stringNo,
                                    slvSTATEMENT_SET,
                                    &Statements);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_NO();
            return;
        }
	}

    sloCOMPILER_PopCurrentNameSpace(Compiler, gcvNULL);

    FuncName->u.funcInfo.isFuncDef = gcvTRUE;

    status = sloCOMPILER_CheckNewFuncName(
                                        Compiler,
                                        FuncName,
                                        &firstFuncName);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_NO();
        return;
    }

    gcmASSERT(firstFuncName);
    if (firstFuncName == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    if (FuncName != firstFuncName)
    {
        status = slsNAME_BindAliasParamNames(Compiler, FuncName, firstFuncName);

		if (gcmIS_ERROR(status))
        {
            gcmFOOTER_NO();
            return;
        }
    }

    gcmVERIFY_OK(sloNAME_BindFuncBody(
                                    Compiler,
                                    firstFuncName,
                                    Statements));

    gcmVERIFY_OK(sloCOMPILER_AddExternalDecl(Compiler, &Statements->base));

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "</FUNCTION>"));
    gcmFOOTER_NO();
}

slsNAME *
slParseParameterList(
    IN sloCOMPILER Compiler,
    IN slsNAME * FuncName,
    IN slsNAME * ParamName
    )
{
    gcmHEADER_ARG("Compiler=0x%x FuncName=0x%x ParamName=0x%x",
                  Compiler, FuncName, ParamName);

    gcmFOOTER_ARG("<return>=0x%x", FuncName);
	return FuncName;
}

slsDeclOrDeclList
slParseTypeDecl(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType
    )
{
    slsDeclOrDeclList declOrDeclList;

    declOrDeclList.dataType         = DataType;
    declOrDeclList.initStatement    = gcvNULL;
    declOrDeclList.initStatements   = gcvNULL;

    return declOrDeclList;
}

slsDeclOrDeclList
slParseInvariantDecl(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN slsLexToken * Identifier
    )
{
    slsDeclOrDeclList declOrDeclList;

    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x Identifier=0x%x",
                  Compiler, StartToken, Identifier);

    gcmASSERT(StartToken);
    gcmASSERT(Identifier);

    declOrDeclList.dataType         = gcvNULL;
    declOrDeclList.initStatement    = gcvNULL;
    declOrDeclList.initStatements   = gcvNULL;

    /* TODO: Not implemented yet */

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<INVARIANT_DECL line=\"%d\" string=\"%d\" identifier=\"%s\" />",
                                StartToken->lineNo,
                                StartToken->stringNo,
                                Identifier->u.identifier));

    gcmFOOTER_ARG("<return>=0x%x", declOrDeclList);
	return declOrDeclList;
}

slsNAME *
slParseNonArrayParameterDecl(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier
    )
{
    gceSTATUS       status;
    slsNAME *       name;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x",
                  Compiler, DataType, Identifier);

	if (DataType == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

	if (slsDATA_TYPE_IsVoid(DataType) && Identifier == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    status = sloCOMPILER_CreateName(
                                    Compiler,
                                    (Identifier != gcvNULL)? Identifier->lineNo : 0,
                                    (Identifier != gcvNULL)? Identifier->stringNo : 0,
                                    slvPARAMETER_NAME,
                                    DataType,
                                    (Identifier != gcvNULL)? Identifier->u.identifier : "",
                                    slvEXTENSION_NONE,
                                    &name);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<PARAMETER_DECL dataType=\"0x%x\" name=\"%s\" />",
                                DataType,
                                (Identifier != gcvNULL)? Identifier->u.identifier : ""));

    gcmFOOTER_ARG("<return>=0x%x", name);
	return name;
}

slsNAME *
slParseArrayParameterDecl(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR ArrayLengthExpr
    )
{
    gceSTATUS       status;
    gctUINT         arrayLength;
    slsDATA_TYPE *  arrayDataType;
    slsNAME *       name;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x Identifier=0x%x ArrayLengthExpr=0x%x",
                  Compiler, DataType, Identifier, ArrayLengthExpr);

	if (DataType == gcvNULL || ArrayLengthExpr == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    status = _EvaluateExprToArrayLength(
                                        Compiler,
                                        ArrayLengthExpr,
                                        &arrayLength);

	if (gcmIS_ERROR(status)) {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    status = sloCOMPILER_CreateArrayDataType(
                                            Compiler,
                                            DataType,
                                            arrayLength,
                                            &arrayDataType);

	if (gcmIS_ERROR(status)) {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    status = sloCOMPILER_CreateName(
                                    Compiler,
                                    (Identifier != gcvNULL)? Identifier->lineNo : 0,
                                    (Identifier != gcvNULL)? Identifier->stringNo : 0,
                                    slvPARAMETER_NAME,
                                    arrayDataType,
                                    (Identifier != gcvNULL)? Identifier->u.identifier : "",
                                    slvEXTENSION_NONE,
                                    &name);

	if (gcmIS_ERROR(status)) {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<PARAMETER_DECL dataType=\"0x%x\" name=\"%s\" />",
                                DataType,
                                (Identifier != gcvNULL)? Identifier->u.identifier : ""));

    gcmFOOTER_ARG("<return>=0x%x", name);
	return name;
}

slsLexToken
slParseEmptyParameterQualifier(
    IN sloCOMPILER Compiler
    )
{
    slsLexToken token;

    gcmHEADER_ARG("Compiler=0x%x",
                  Compiler);

    token.lineNo        = 0;
    token.stringNo      = 0;
    token.type          = T_IN;
    token.u.qualifier   = slvQUALIFIER_IN;

    gcmFOOTER_ARG("<return>=0x%x", token);
	return token;
}

slsNAME *
slParseQualifiedParameterDecl(
    IN sloCOMPILER Compiler,
    IN slsLexToken * TypeQualifier,
    IN slsLexToken * ParameterQualifier,
    IN slsNAME * ParameterDecl
    )
{
    sltQUALIFIER qualifier;

    gcmHEADER_ARG("Compiler=0x%x TypeQualifier=0x%x ParameterQualifier=0x%x ParameterDecl=0x%x",
                  Compiler, TypeQualifier, ParameterQualifier, ParameterDecl);

    gcmASSERT(ParameterQualifier);

	if (ParameterDecl == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmASSERT(ParameterDecl->dataType);

    if (TypeQualifier != gcvNULL)
    {
        switch (TypeQualifier->u.qualifier)
        {
        case slvQUALIFIER_CONST:
            qualifier = slvQUALIFIER_CONST_IN;
            break;

        default:
            qualifier = ParameterQualifier->u.qualifier;

            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            TypeQualifier->lineNo,
                                            TypeQualifier->stringNo,
                                            slvREPORT_ERROR,
                                            "invalid parameter qualifier: '%s'",
                                            slGetQualifierName(TypeQualifier->u.qualifier)));
        }

        if (qualifier == slvQUALIFIER_CONST_IN
            && ParameterQualifier->u.qualifier != slvQUALIFIER_IN)
        {
            qualifier = ParameterQualifier->u.qualifier;

            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            TypeQualifier->lineNo,
                                            TypeQualifier->stringNo,
                                            slvREPORT_ERROR,
                                            "the const qualifier cannot be used with out or inout"));
        }
    }
    else
    {
        qualifier = ParameterQualifier->u.qualifier;
    }

    ParameterDecl->dataType->qualifier = qualifier;

    gcmFOOTER_ARG("<return>=0x%x", ParameterDecl);
	return ParameterDecl;
}

slsDATA_TYPE *
slParseFullySpecifiedType(
    IN sloCOMPILER Compiler,
    IN slsLexToken * TypeQualifier,
    IN slsDATA_TYPE * DataType
    )
{
    gctBOOL     mustAtGlobalNameSpace, atGlobalNameSpace;
    gcmHEADER_ARG("Compiler=0x%x TypeQualifier=0x%x DataType=0x%x",
                  Compiler, TypeQualifier, DataType);

    gcmASSERT(TypeQualifier);

	if (DataType == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    switch (TypeQualifier->u.qualifier)
    {
    case slvQUALIFIER_UNIFORM:

        mustAtGlobalNameSpace = gcvTRUE;

        break;

    case slvQUALIFIER_ATTRIBUTE:
        if (!slsDATA_TYPE_IsFloatOrVecOrMat(DataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            TypeQualifier->lineNo,
                                            TypeQualifier->stringNo,
                                            slvREPORT_ERROR,
                                            "the 'attribute' qualifier can be used only with"
                                            " the data types: 'float', 'vec2', 'vec3',"
                                            " 'vec4', 'mat2', 'mat3', and 'mat4'"));

            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}

        mustAtGlobalNameSpace = gcvTRUE;

        break;

    case slvQUALIFIER_VARYING_OUT:
    case slvQUALIFIER_VARYING_IN:
    case slvQUALIFIER_INVARIANT_VARYING_OUT:
    case slvQUALIFIER_INVARIANT_VARYING_IN:
        if (DataType->elementType != slvTYPE_FLOAT)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            TypeQualifier->lineNo,
                                            TypeQualifier->stringNo,
                                            slvREPORT_ERROR,
                                            "the 'varying' qualifier can be used only with"
                                            " the data types: 'float', 'vec2', 'vec3',"
                                            " 'vec4', 'mat2', 'mat3', and 'mat4', or arrays of these"));

            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}

        mustAtGlobalNameSpace = gcvTRUE;

        break;

    default:
        mustAtGlobalNameSpace = gcvFALSE;
    }

    if (mustAtGlobalNameSpace)
    {
        gcmVERIFY_OK(sloCOMPILER_AtGlobalNameSpace(Compiler, &atGlobalNameSpace));

        if (!atGlobalNameSpace)
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            TypeQualifier->lineNo,
                                            TypeQualifier->stringNo,
                                            slvREPORT_ERROR,
                                            "the %s qualifier can be used to declare"
                                            " global variables",
                                            slGetQualifierName(TypeQualifier->u.qualifier)));

            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
        }
    }

    DataType->qualifier = TypeQualifier->u.qualifier;

    gcmFOOTER_ARG("<return>=0x%x", DataType);
	return DataType;
}

slsDATA_TYPE *
slParseTypeSpecifier(
    IN sloCOMPILER Compiler,
    IN slsLexToken * PrecisionQualifier,
    IN slsDATA_TYPE * DataType
    )
{
    gcmHEADER_ARG("Compiler=0x%x PrecisionQualifier=0x%x DataType=0x%x",
                  Compiler, PrecisionQualifier, DataType);

    gcmASSERT(PrecisionQualifier);

	if (DataType == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    DataType->precision = PrecisionQualifier->u.precision;

    gcmFOOTER_ARG("<return>=0x%x", DataType);
    return DataType;
}

slsDATA_TYPE *
slParseNonStructType(
    IN sloCOMPILER Compiler,
    IN slsLexToken * Token,
    IN gctINT TokenType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x Token=0x%x TokenType=%d",
                  Compiler, Token, TokenType);

    gcmASSERT(Token);

    status = sloCOMPILER_CreateDataType(
                                    Compiler,
                                    TokenType,
                                    gcvNULL,
                                    &dataType);

	if (gcmIS_ERROR(status)) {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<DATA_TYPE line=\"%d\" string=\"%d\" name=\"%s\" />",
                                Token->lineNo,
                                Token->stringNo,
                                _GetNonStructTypeName(TokenType)));

    gcmFOOTER_ARG("<return>=0x%x", dataType);
	return dataType;
}

slsDATA_TYPE *
slParseStructType(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * StructType
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x StructType=0x%x",
                  Compiler, StructType);

	if (StructType == gcvNULL)
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    status = sloCOMPILER_CloneDataType(
                                    Compiler,
                                    slvQUALIFIER_NONE,
                                    StructType,
                                    &dataType);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmFOOTER_ARG("<return>=0x%x", dataType);
	return dataType;
}

slsDATA_TYPE *
slParseNamedType(
    IN sloCOMPILER Compiler,
    IN slsLexToken * TypeName
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;

    gcmHEADER_ARG("Compiler=0x%x TypeName=0x%x",
                  Compiler, TypeName);

    gcmASSERT(TypeName);
    gcmASSERT(TypeName->u.typeName);

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<DATA_TYPE line=\"%d\" string=\"%d\" name=\"%s\" />",
                                TypeName->lineNo,
                                TypeName->stringNo,
                                TypeName->u.typeName->symbol));


    status = sloCOMPILER_CloneDataType(
                                    Compiler,
                                    slvQUALIFIER_NONE,
                                    TypeName->u.typeName->dataType,
                                    &dataType);

	if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
        return gcvNULL;
    }

    gcmFOOTER_ARG("<return>=0x%x", dataType);
	return dataType;
}

void
slParseStructDeclBegin(
    IN sloCOMPILER Compiler
    )
{
    gceSTATUS       status;
    slsNAME_SPACE * nameSpace;

    gcmHEADER_ARG("Compiler=0x%x",
                  Compiler);

    status = sloCOMPILER_CreateNameSpace(
                                        Compiler,
                                        &nameSpace);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_NO();
		return;
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<STRUCT_DECL>"));
    gcmFOOTER_NO();
}

slsDATA_TYPE *
slParseStructDeclEnd(
    IN sloCOMPILER Compiler,
    IN slsLexToken * StartToken,
    IN slsLexToken * Identifier
    )
{
    gceSTATUS       status;
    slsDATA_TYPE *  dataType;
    slsNAME_SPACE * prevNameSpace = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x StartToken=0x%x Identifier=0x%x",
                  Compiler, StartToken, Identifier);

    gcmASSERT(StartToken);

    status = sloCOMPILER_PopCurrentNameSpace(Compiler, &prevNameSpace);
    if (gcmIS_ERROR(status)) {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}
    status = sloCOMPILER_CreateDataType(
                                        Compiler,
                                        T_STRUCT,
                                        prevNameSpace,
                                        &dataType);

	if (gcmIS_ERROR(status)) {
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    if (Identifier != gcvNULL)
    {
        status = sloCOMPILER_CreateName(
                                        Compiler,
                                        Identifier->lineNo,
                                        Identifier->stringNo,
                                        slvSTRUCT_NAME,
                                        dataType,
                                        Identifier->u.identifier,
                                        slvEXTENSION_NONE,
                                        gcvNULL);

		if (gcmIS_ERROR(status)) {
            gcmFOOTER_ARG("<return>=%s", "<nil>");
			return gcvNULL;
		}
    }

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "</STRUCT_DECL>"));

    gcmFOOTER_ARG("<return>=0x%x", dataType);
	return dataType;
}

void
slParseTypeSpecifiedFieldDeclList(
    IN sloCOMPILER Compiler,
    IN slsDATA_TYPE * DataType,
    IN slsDLINK_LIST * FieldDeclList
    )
{
    gceSTATUS       status;
    slsFieldDecl *  fieldDecl;

    gcmHEADER_ARG("Compiler=0x%x DataType=0x%x FieldDeclList=0x%x",
                  Compiler, DataType, FieldDeclList);

	if (DataType == gcvNULL || FieldDeclList == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    FOR_EACH_DLINK_NODE(FieldDeclList, slsFieldDecl, fieldDecl)
    {
        if (slsDATA_TYPE_IsVoid(DataType))
        {
            gcmVERIFY_OK(sloCOMPILER_Report(
                                            Compiler,
                                            fieldDecl->field->lineNo,
                                            fieldDecl->field->stringNo,
                                            slvREPORT_ERROR,
                                            "'%s' can not use the void type",
                                            fieldDecl->field->symbol));

            break;
        }

        if (fieldDecl->arrayLength == 0)
        {
            fieldDecl->field->dataType = DataType;
        }
        else
        {
            status = sloCOMPILER_CreateArrayDataType(
                                                    Compiler,
                                                    DataType,
                                                    fieldDecl->arrayLength,
                                                    &fieldDecl->field->dataType);

            if (gcmIS_ERROR(status))
            {
                fieldDecl->field->dataType = DataType;
                break;
            }
        }
    }

    while (!slsDLINK_LIST_IsEmpty(FieldDeclList))
    {
        slsDLINK_LIST_DetachFirst(FieldDeclList, slsFieldDecl, &fieldDecl);

        gcmVERIFY_OK(sloCOMPILER_Free(Compiler, fieldDecl));
    }

	gcmVERIFY_OK(sloCOMPILER_Free(Compiler, FieldDeclList));

    gcmFOOTER_NO();
}

slsDLINK_LIST *
slParseFieldDeclList(
    IN sloCOMPILER Compiler,
    IN slsFieldDecl * FieldDecl
    )
{
    gceSTATUS       status;
    slsDLINK_LIST * fieldDeclList;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("Compiler=0x%x FieldDecl=0x%x",
                  Compiler, FieldDecl);

    status = sloCOMPILER_Allocate(
                                Compiler,
                                (gctSIZE_T)sizeof(slsDLINK_LIST),
                                &pointer);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    fieldDeclList = pointer;

    slsDLINK_LIST_Initialize(fieldDeclList);

    if (FieldDecl != gcvNULL)
    {
        slsDLINK_LIST_InsertLast(fieldDeclList, &FieldDecl->node);
    }

    gcmFOOTER_ARG("<return>=0x%x", fieldDeclList);
	return fieldDeclList;
}

slsDLINK_LIST *
slParseFieldDeclList2(
    IN sloCOMPILER Compiler,
    IN slsDLINK_LIST * FieldDeclList,
    IN slsFieldDecl * FieldDecl
    )
{
    gcmHEADER_ARG("Compiler=0x%x FieldDeclList=0x%x FieldDecl=0x%x",
                  Compiler, FieldDeclList, FieldDecl);

    if (FieldDeclList != gcvNULL && FieldDecl != gcvNULL)
    {
        slsDLINK_LIST_InsertLast(FieldDeclList, &FieldDecl->node);
    }

    gcmFOOTER_ARG("<return>=0x%x", FieldDeclList);
	return FieldDeclList;
}

slsFieldDecl *
slParseFieldDecl(
    IN sloCOMPILER Compiler,
    IN slsLexToken * Identifier,
    IN sloIR_EXPR ArrayLengthExpr
    )
{
    gceSTATUS       status;
    slsNAME *       field;
    slsFieldDecl *  fieldDecl;
    gctPOINTER pointer = gcvNULL;
    gcmHEADER_ARG("Compiler=0x%x Identifier=0x%x ArrayLengthExpr=0x%x",
                  Compiler, Identifier, ArrayLengthExpr);

    gcmASSERT(Identifier);

    status = sloCOMPILER_CreateName(
                                    Compiler,
                                    Identifier->lineNo,
                                    Identifier->stringNo,
                                    slvFIELD_NAME,
                                    gcvNULL,
                                    Identifier->u.identifier,
                                    slvEXTENSION_NONE,
                                    &field);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    status = sloCOMPILER_Allocate(
                                Compiler,
                                (gctSIZE_T)sizeof(slsFieldDecl),
                                &pointer);

	if (gcmIS_ERROR(status))
	{
        gcmFOOTER_ARG("<return>=%s", "<nil>");
		return gcvNULL;
	}

    fieldDecl = pointer;

    fieldDecl->field    = field;

    if (ArrayLengthExpr == gcvNULL)
    {
        fieldDecl->arrayLength  = 0;
    }
    else
    {
        status = _EvaluateExprToArrayLength(
                                            Compiler,
                                            ArrayLengthExpr,
                                            &fieldDecl->arrayLength);

        if (gcmIS_ERROR(status))
        {
            gcmASSERT(fieldDecl->arrayLength == 0);

            gcmFOOTER_ARG("<return>=0x%x", fieldDecl);
			return fieldDecl;
		}
	}

    gcmVERIFY_OK(sloCOMPILER_Dump(
                                Compiler,
                                slvDUMP_PARSER,
                                "<FIELD line=\"%d\" string=\"%d\" name=\"%s\" />",
                                Identifier->lineNo,
                                Identifier->stringNo,
                                Identifier->u.identifier));

    gcmFOOTER_ARG("<return>=0x%x", fieldDecl);
	return fieldDecl;
}
