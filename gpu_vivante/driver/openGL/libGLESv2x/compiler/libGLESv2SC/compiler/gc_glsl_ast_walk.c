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


#include "gc_glsl_ast_walk.h"
#include "gc_glsl_gen_code.h"
#include "gc_glsl_emit_code.h"

static gceSTATUS
_CountFuncResources(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN slsNAME *FuncName
    )
{
    gceSTATUS       status = gcvSTATUS_OK;
    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x FuncName=0x%x",
                  Compiler, ObjectCounter, FuncName);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    gcmASSERT(FuncName);

    if(FuncName->context.isCounted) {
	gcmFOOTER_NO();
	return gcvSTATUS_OK;
    }

    ObjectCounter->functionCount++;
    FuncName->context.isCounted = gcvTRUE;

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_ITERATION_CountForCode(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  initParameters, restParameters, bodyParameters;
    slsGEN_CODE_PARAMETERS  condParameters;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Iteration=0x%x",
                  Compiler, ObjectCounter, Iteration);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(Iteration->type == slvFOR);

    /* The init part */
    if (Iteration->forInitStatement != gcvNULL) {
        status = sloIR_OBJECT_Accept(Compiler,
                                     Iteration->forInitStatement,
                                     &ObjectCounter->visitor,
                                     &initParameters);
        if (gcmIS_ERROR(status)) {
	   gcmFOOTER();
	   return status;
	}
    }

    /* The rest part */
    if (Iteration->forRestExpr != gcvNULL) {
        status = sloIR_OBJECT_Accept(Compiler,
                                     &Iteration->forRestExpr->base,
                                     &ObjectCounter->visitor,
                                     &restParameters);
        if (gcmIS_ERROR(status)) {
	   gcmFOOTER();
	   return status;
	}
    }

    /* The condition part */
    if (Iteration->condExpr != gcvNULL) {
       status = sloIR_OBJECT_Accept(Compiler,
                                    &Iteration->condExpr->base,
                                    &ObjectCounter->visitor,
                                    &condParameters);
        if (gcmIS_ERROR(status)) {
	   gcmFOOTER();
	   return status;
	}
    }

    /* The body part */
    if (Iteration->loopBody != gcvNULL) {
        status = sloIR_OBJECT_Accept(Compiler,
                                     Iteration->loopBody,
                                     &ObjectCounter->visitor,
                                     &bodyParameters);
        if (gcmIS_ERROR(status)) {
	   gcmFOOTER();
	   return status;
	}
    }
    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_CountWhileCode(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  bodyParameters, condParameters;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Iteration=0x%x",
                  Compiler, ObjectCounter, Iteration);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(Iteration->type == slvWHILE);

    /* The condition part */
    if (Iteration->condExpr != gcvNULL) {
       status = sloIR_OBJECT_Accept(Compiler,
                                    &Iteration->condExpr->base,
                                    &ObjectCounter->visitor,
                                    &condParameters);
        if (gcmIS_ERROR(status)) {
	   gcmFOOTER();
	   return status;
	}
    }
    /* The loop body */
    if (Iteration->loopBody != gcvNULL) {
        status = sloIR_OBJECT_Accept(Compiler,
                                     Iteration->loopBody,
                                     &ObjectCounter->visitor,
                                     &bodyParameters);
        if (gcmIS_ERROR(status)) {
	   gcmFOOTER();
	   return status;
	}
    }
    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_CountDoWhileCode(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS  status;
    slsGEN_CODE_PARAMETERS  bodyParameters, condParameters;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Iteration=0x%x",
                  Compiler, ObjectCounter, Iteration);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);
    gcmASSERT(Iteration->type == slvDO_WHILE);

    if (Iteration->loopBody != gcvNULL) {
        status = sloIR_OBJECT_Accept(Compiler,
                                     Iteration->loopBody,
                                     &ObjectCounter->visitor,
                                     &bodyParameters);
        if (gcmIS_ERROR(status)) {
	   gcmFOOTER();
	   return status;
	}
    }
    /* The condition part */
    if (Iteration->condExpr != gcvNULL) {
       status = sloIR_OBJECT_Accept(Compiler,
                                    &Iteration->condExpr->base,
                                    &ObjectCounter->visitor,
                                    &condParameters);
        if (gcmIS_ERROR(status)) {
	   gcmFOOTER();
	   return status;
	}
    }

    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_ITERATION_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_ITERATION Iteration,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Iteration=0x%x",
                  Compiler, ObjectCounter, Iteration);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Iteration, slvIR_ITERATION);

    switch (Iteration->type) {
    case slvFOR:
        status = sloIR_ITERATION_CountForCode(Compiler,
                                              ObjectCounter,
                                              Iteration,
                                              Parameters);
	break;

    case slvWHILE:
        status = sloIR_ITERATION_CountWhileCode(Compiler,
                                                 ObjectCounter,
                                                 Iteration,
                                                 Parameters);
	break;

    case slvDO_WHILE:
        status = sloIR_ITERATION_CountDoWhileCode(Compiler,
                                                  ObjectCounter,
                                                  Iteration,
                                                  Parameters);
	break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }
    if (gcmIS_ERROR(status)) {
       gcmFOOTER();
       return status;
    }
    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_JUMP_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_JUMP Jump,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Jump=0x%x",
                  Compiler, ObjectCounter, Jump);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Jump, slvIR_JUMP);
    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

static gceSTATUS
_CountVariableOrArray(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN slsNAME *Name,
    IN slsDATA_TYPE *DataType
    )
{
    sltQUALIFIER    qualifier;
    gctUINT         logicalRegCount;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Name=0x%x "
                  "DataType=0x%x",
                  Compiler, ObjectCounter, Name,
                  DataType);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    gcmVERIFY_ARGUMENT(Name);
    gcmASSERT(DataType);
    gcmASSERT(!slsDATA_TYPE_IsStruct(DataType));

    qualifier = Name->dataType->qualifier;

    logicalRegCount = (DataType->arrayLength > 0) ? DataType->arrayLength : 1;

    switch (qualifier) {
    case slvQUALIFIER_NONE:
    case slvQUALIFIER_CONST_IN:
    case slvQUALIFIER_IN:
    case slvQUALIFIER_OUT:
    case slvQUALIFIER_INOUT:
        if (Name->type == slvFUNC_NAME || Name->type == slvPARAMETER_NAME) {
/** SKIP THE COUNT FOR FUNCTION ARGUMENTS - DELAY TILL CODE GENERATION */
	    ;
        }
        else {
	    ObjectCounter->variableCount++;
	    Name->context.isCounted = gcvTRUE;
        }
        break;

    case slvQUALIFIER_UNIFORM:
	ObjectCounter->uniformCount++;
	Name->context.isCounted = gcvTRUE;
        break;

    case slvQUALIFIER_ATTRIBUTE:
    case slvQUALIFIER_VARYING_IN:
    case slvQUALIFIER_INVARIANT_VARYING_IN:
	ObjectCounter->attributeCount++;
	Name->context.isCounted = gcvTRUE;
        break;

    case slvQUALIFIER_VARYING_OUT:
    case slvQUALIFIER_INVARIANT_VARYING_OUT:
    case slvQUALIFIER_FRAGMENT_OUT:
	ObjectCounter->outputCount += logicalRegCount;
	Name->context.isCounted = gcvTRUE;

        if (qualifier == slvQUALIFIER_VARYING_OUT) {
	    ObjectCounter->variableCount++;
        }
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_ARGUMENT);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

static gceSTATUS
_CountVariable(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN slsNAME *Name,
    IN slsDATA_TYPE *DataType
    )
{
    gceSTATUS	status;
    gctUINT	count, i;
    slsNAME	*fieldName;

    gcmHEADER_ARG("Compiler=0x%x objectCounter=0x%x Name=0x%x "
                  "DataType=0x%x",
                  Compiler, ObjectCounter, Name, DataType);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    gcmVERIFY_ARGUMENT(Name);
    gcmASSERT(DataType);

    if (DataType->elementType == slvTYPE_STRUCT) {
        count = (DataType->arrayLength > 0) ? DataType->arrayLength : 1;

        gcmASSERT(Name->dataType->fieldSpace);
        for (i = 0; i < count; i++) {
            FOR_EACH_DLINK_NODE(&DataType->fieldSpace->names, slsNAME, fieldName) {
                gcmASSERT(fieldName->dataType);

                status = _CountVariable(Compiler,
                                        ObjectCounter,
                                        Name,
                                        fieldName->dataType);

                if (gcmIS_ERROR(status)) {
                    gcmFOOTER();
                    return status;
                }
            }
        }
    }
    else {
        status = _CountVariableOrArray(Compiler,
                                       ObjectCounter,
                                       Name,
                                       DataType);

        if (gcmIS_ERROR(status)) {
            gcmFOOTER();
            return status;
        }
    }

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS
slsNAME_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN slsNAME *Name
    )
{
    gceSTATUS  status;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Name=0x%x",
                  Compiler, ObjectCounter, Name);

    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    gcmVERIFY_ARGUMENT(Name);
    gcmASSERT(Name->dataType);
    gcmASSERT(Name->type == slvVARIABLE_NAME
                || Name->type == slvPARAMETER_NAME
                || Name->type == slvFUNC_NAME);

    if (Name->context.isCounted)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    if (Name->type == slvPARAMETER_NAME && Name->u.parameterInfo.aliasName != gcvNULL) {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    do {
        status = _CountVariable(Compiler,
                                ObjectCounter,
                                Name,
                                Name->dataType);
        if (gcmIS_ERROR(status)) break;

        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    } while (gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_VARIABLE_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_VARIABLE Variable,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS  status;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Variable=0x%x",
                  Compiler, ObjectCounter, Variable);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Variable, slvIR_VARIABLE);
    gcmASSERT(Parameters);

    gcmASSERT(Variable->name);

        /* Allocate all logical registers */
    status = slsNAME_Count(Compiler,
                           ObjectCounter,
                           Variable->name);
    if (gcmIS_ERROR(status)) {
	gcmFOOTER();
	return status;
    }

    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_SET_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_SET Set,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS               status;
    sloIR_BASE              member;
    slsGEN_CODE_PARAMETERS  memberParameters;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Set=0x%x",
                  Compiler, ObjectCounter, Set);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    slmVERIFY_IR_OBJECT(Set, slvIR_SET);

    switch (Set->type) {
    case slvDECL_SET:
        FOR_EACH_DLINK_NODE(&Set->members, struct _sloIR_BASE, member) {
            /* Count through members in the set */
            status = sloIR_OBJECT_Accept(Compiler,
                                         member,
                                         &ObjectCounter->visitor,
                                         &memberParameters);
            if (gcmIS_ERROR(status)) {
	       gcmFOOTER();
	       return status;
	    }
        }
        break;

    case slvSTATEMENT_SET:
        if (Set->funcName != gcvNULL) {
            status = _CountFuncResources(Compiler,
                                         ObjectCounter,
                                         Set->funcName);
            if (gcmIS_ERROR(status)) {
	       gcmFOOTER();
	       return status;
	    }
        }

        FOR_EACH_DLINK_NODE(&Set->members, struct _sloIR_BASE, member) {
            status = sloIR_OBJECT_Accept(Compiler,
                                         member,
                                         &ObjectCounter->visitor,
                                         &memberParameters);
            if (gcmIS_ERROR(status)) {
	       gcmFOOTER();
	       return status;
	    }
        }
        break;

    case slvEXPR_SET:
        gcmASSERT(0);
        break;

    default:
        gcmASSERT(0);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_INVALID_DATA);
        return gcvSTATUS_INVALID_DATA;
    }

    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_CONSTANT_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_CONSTANT Constant,
    IN OUT slsGEN_CODE_PARAMETERS * Parameters
    )
{
    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Constant=0x%x",
                  Compiler, ObjectCounter, Constant);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_IR_OBJECT(Constant, slvIR_CONSTANT);

    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_UNARY_EXPR_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_UNARY_EXPR UnaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS  operandParameters;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x UnaryExpr=0x%x",
                  Compiler, ObjectCounter, UnaryExpr);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    slmVERIFY_IR_OBJECT(UnaryExpr, slvIR_UNARY_EXPR);

    /* Generate the code of the operand */
    gcmASSERT(UnaryExpr->operand);

    status = sloIR_OBJECT_Accept(Compiler,
                                 &UnaryExpr->operand->base,
                                 &ObjectCounter->visitor,
                                 &operandParameters);
    gcmFOOTER();
    return status;
}

gceSTATUS
sloIR_BINARY_EXPR_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_BINARY_EXPR BinaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS               status;
    slsGEN_CODE_PARAMETERS leftParameters, rightParameters;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x BinaryExpr=0x%x",
                  Compiler, ObjectCounter, BinaryExpr);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    slmVERIFY_IR_OBJECT(BinaryExpr, slvIR_BINARY_EXPR);
    gcmASSERT(Parameters);

    /* Count through the left operand */
    gcmASSERT(BinaryExpr->leftOperand);

    status = sloIR_OBJECT_Accept(Compiler,
                                 &BinaryExpr->leftOperand->base,
                                 &ObjectCounter->visitor,
                                 &leftParameters);
    if (gcmIS_ERROR(status)) {
       gcmFOOTER();
       return status;
    }

    /* Count through the right operand */
    status = sloIR_OBJECT_Accept(Compiler,
                                 &BinaryExpr->rightOperand->base,
                                 &ObjectCounter->visitor,
                                 &rightParameters);
    if (gcmIS_ERROR(status)) {
       gcmFOOTER();
       return status;
    }
    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_SELECTION_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_SELECTION Selection,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS               status;
    gctBOOL                 emptySelection;
    slsGEN_CODE_PARAMETERS  condParameters, trueParameters, falseParameters;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x Selection=0x%x",
                  Compiler, ObjectCounter, Selection);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    slmVERIFY_IR_OBJECT(Selection, slvIR_SELECTION);
    gcmASSERT(Selection->condExpr);
    gcmASSERT(Parameters);

    emptySelection = (Selection->trueOperand == gcvNULL && Selection->falseOperand == gcvNULL);

    if (emptySelection) {
        status = sloIR_OBJECT_Accept(Compiler,
                                     &Selection->condExpr->base,
                                     &ObjectCounter->visitor,
                                     &condParameters);
        if (gcmIS_ERROR(status)) {
           gcmFOOTER();
           return status;
        }
        gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
        return gcvSTATUS_OK;
    }

    status = sloIR_OBJECT_Accept(Compiler,
                                 &Selection->condExpr->base,
                                 &ObjectCounter->visitor,
                                 &condParameters);
    if (gcmIS_ERROR(status)) {
       gcmFOOTER();
       return status;
    }

    if (Selection->trueOperand != gcvNULL) {
        status = sloIR_OBJECT_Accept(Compiler,
                                     Selection->trueOperand,
                                     &ObjectCounter->visitor,
                                     &trueParameters);
        if (gcmIS_ERROR(status)) {
           gcmFOOTER();
           return status;
        }
    }

    /* Generate the code of the false operand */
    if (Selection->falseOperand != gcvNULL) {
        status = sloIR_OBJECT_Accept(Compiler,
                                     Selection->falseOperand,
                                     &ObjectCounter->visitor,
                                     &falseParameters);
        if (gcmIS_ERROR(status)) {
           gcmFOOTER();
           return status;
        }
    }
    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloIR_POLYNARY_EXPR_Count(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter,
    IN sloIR_POLYNARY_EXPR PolynaryExpr,
    IN OUT slsGEN_CODE_PARAMETERS *Parameters
    )
{
    gceSTATUS       status = gcvSTATUS_OK;
    slsGEN_CODE_PARAMETERS operandsParameters;
    sloIR_EXPR operand;

    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x PolynaryExpr=0x%x",
                  Compiler, ObjectCounter, PolynaryExpr);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);
    slmVERIFY_IR_OBJECT(PolynaryExpr, slvIR_POLYNARY_EXPR);
    gcmASSERT(Parameters);

    if (PolynaryExpr->type == slvPOLYNARY_FUNC_CALL) {
    /* Allocate the function resources */
        if (!PolynaryExpr->funcName->isBuiltIn) {
           status = _CountFuncResources(Compiler,
                                        ObjectCounter,
                                        PolynaryExpr->funcName);
           if (gcmIS_ERROR(status)) {
              gcmFOOTER();
              return status;
           }
	}
    }
    if (PolynaryExpr->operands == gcvNULL) {
        gcmFOOTER();
        return gcvSTATUS_OK;
    }

    FOR_EACH_DLINK_NODE(&PolynaryExpr->operands->members, struct _sloIR_EXPR, operand) {
            status = sloIR_OBJECT_Accept(Compiler,
                                         &operand->base,
                                         &ObjectCounter->visitor,
                                         &operandsParameters);
           if (gcmIS_ERROR(status)) {
              gcmFOOTER();
              return status;
           }
    }
    gcmFOOTER_ARG("*Parameters=0x%x", *Parameters);
    return gcvSTATUS_OK;
}

gceSTATUS
sloOBJECT_COUNTER_Construct(
    IN sloCOMPILER Compiler,
    OUT sloOBJECT_COUNTER *ObjectCounter
    )
{
    gceSTATUS           status;
    sloOBJECT_COUNTER   objectCounter;
    gctPOINTER pointer;

    gcmHEADER_ARG("Compiler=0x%x", Compiler);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    gcmASSERT(ObjectCounter);



    status = sloCOMPILER_Allocate(Compiler,
                                  (gctSIZE_T)sizeof(struct _sloOBJECT_COUNTER),
                                  &pointer);
    if (gcmIS_ERROR(status)) {
       gcmFOOTER();
       return status;
    }

    objectCounter = pointer;

    /* Initialize the visitor */
    objectCounter->visitor.object.type          = slvOBJ_OBJECT_COUNTER;

    objectCounter->visitor.visitSet             =
                    (sltVISIT_SET_FUNC_PTR)sloIR_SET_Count;

    objectCounter->visitor.visitIteration       =
                    (sltVISIT_ITERATION_FUNC_PTR)sloIR_ITERATION_Count;

    objectCounter->visitor.visitJump            =
                    (sltVISIT_JUMP_FUNC_PTR)sloIR_JUMP_Count;

    objectCounter->visitor.visitVariable        =
                    (sltVISIT_VARIABLE_FUNC_PTR)sloIR_VARIABLE_Count;

    objectCounter->visitor.visitConstant        =
                    (sltVISIT_CONSTANT_FUNC_PTR)sloIR_CONSTANT_Count;

    objectCounter->visitor.visitUnaryExpr       =
                    (sltVISIT_UNARY_EXPR_FUNC_PTR)sloIR_UNARY_EXPR_Count;

    objectCounter->visitor.visitBinaryExpr      =
                    (sltVISIT_BINARY_EXPR_FUNC_PTR)sloIR_BINARY_EXPR_Count;

    objectCounter->visitor.visitSelection       =
                    (sltVISIT_SELECTION_FUNC_PTR)sloIR_SELECTION_Count;

    objectCounter->visitor.visitPolynaryExpr    =
                    (sltVISIT_POLYNARY_EXPR_FUNC_PTR)sloIR_POLYNARY_EXPR_Count;

    /* Initialize other data members */
    objectCounter->attributeCount= 0;
    objectCounter->uniformCount = 0;
    objectCounter->variableCount = 0;
    objectCounter->outputCount = 0;
    objectCounter->functionCount = 0;

    *ObjectCounter = objectCounter;
    gcmFOOTER_ARG("*ObjectCounter=0x%x", *ObjectCounter);
    return gcvSTATUS_OK;
}

gceSTATUS
sloOBJECT_COUNTER_Destroy(
    IN sloCOMPILER Compiler,
    IN sloOBJECT_COUNTER ObjectCounter
    )
{
    gcmHEADER_ARG("Compiler=0x%x ObjectCounter=0x%x",
                  Compiler, ObjectCounter);
    /* Verify the arguments. */
    slmVERIFY_OBJECT(Compiler, slvOBJ_COMPILER);
    slmVERIFY_OBJECT(ObjectCounter, slvOBJ_OBJECT_COUNTER);;

    gcmVERIFY_OK(sloCOMPILER_Free(Compiler, ObjectCounter));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}
