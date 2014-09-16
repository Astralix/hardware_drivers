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




/*
**  Common gcSL optimizer module.
*/

#include "gc_hal_user_precomp.h"

#ifndef VIVANTE_NO_3D

#include "gc_hal_optimizer.h"

#define _GC_OBJ_ZONE    gcvZONE_COMPILER

/******************************************************************************\
|*************************** Optimization Functioins **************************|
|******************************************************************************|
|* All the functions in this file optimize the input shader without changing  *|
|* the functionality of the shader.  All the underlying utilities, including  *|
|* housekeeping functions are in gcOptUtil.c.                                 *|
\******************************************************************************/
static gceSTATUS
_RemoveFunctionUnreachableCode(
    IN gcOPTIMIZER Optimizer,
    IN gcOPT_FUNCTION Function
    )
{
    gctUINT codeRemoved;
    gcOPT_CODE code;
    gcOPT_CODE codeNext;
    gceSTATUS status = gcvSTATUS_OK;

    do
    {
        code = Function->codeHead;
        codeNext = Function->codeTail->next;
        codeRemoved = 0;

        do
        {
            if (code->instruction.opcode == gcSL_JMP)
            {
                gcOPT_CODE codeJump;
                gcOPT_CODE codeTarget;

                if (gcmSL_TARGET_GET(code->instruction.temp, Condition) != gcSL_ALWAYS)
                {
                    code  = code->next;
                    continue;
                }

                codeJump = code;
                codeTarget = code->callee;

                code = code->next;
                while (code != codeNext &&code !=gcvNULL && code->callers == gcvNULL)
                {
                    if (code->instruction.opcode != gcSL_NOP)
                    {
                        gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));
                        codeRemoved++;
                    }
                    code = code->next;
                }

                /* Check if the jump is redundant. */
                if (codeTarget == code)
                {
                    gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, codeJump));
                    codeRemoved++;
                }
            }
            else if (code->instruction.opcode == gcSL_RET)
            {
                code = code->next;
                while (code != codeNext && code !=gcvNULL && code->callers == gcvNULL)
                 {
                    if (code->instruction.opcode != gcSL_NOP)
                    {
                        gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));
                        codeRemoved++;
                    }
                    code = code->next;
                }
            }
            else
            {
                code = code->next;
            }
        }
        while (code != codeNext && code !=gcvNULL );

        if (codeRemoved > 0)
        {
            status = gcvSTATUS_CHANGED;
        }
    }
    while (codeRemoved > 0);

    return status;
}

/*******************************************************************************
**                          gcOpt_RemoveDeadCode
********************************************************************************
**
**  Remove unused code.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_RemoveDeadCode(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT codeRemoved = 0;
    gctUINT i;
    gcOPT_CODE code;
    gctBOOL bTexldIsRemoved = gcvFALSE;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    /* Pass 1: remove un-reachable instructions. */
    status = _RemoveFunctionUnreachableCode(Optimizer, Optimizer->main);

    for (i = 0; i < Optimizer->functionCount; i++)
    {
        gceSTATUS oStatus =
            _RemoveFunctionUnreachableCode(Optimizer,
                                           Optimizer->functionArray + i);
        if (oStatus == gcvSTATUS_CHANGED)
        {
            status = gcvSTATUS_CHANGED;
        }
    }

    if (status == gcvSTATUS_CHANGED)
        codeRemoved++;

    /* Pass 2: remove unused instructions. */
    for (code = Optimizer->codeTail; code; code = code->prev)
    {
        switch (code->instruction.opcode)
        {
        case gcSL_CALL:
        case gcSL_RET:
        case gcSL_NOP:
        case gcSL_KILL:
            /* Skip control flow instructions. */
            break;

        case gcSL_TEXBIAS:
        case gcSL_TEXGRAD:
        case gcSL_TEXLOD:
            if (bTexldIsRemoved)
            {
                bTexldIsRemoved = gcvFALSE;

                gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));
                codeRemoved++;
            }
            break;

        case gcSL_BARRIER:
            /* Skip barrier instruction. */
            break;

        case gcSL_JMP:
            if (! code->backwardJump)
            {
                gcOPT_CODE codeBtwn = code->callee->prev;
                gctBOOL redundant = gcvTRUE;

                /* Check if the jump jumps to next non-NOP instruction. */
                for (; codeBtwn !=gcvNULL && codeBtwn != code; codeBtwn = codeBtwn->prev)
                {
                    if (codeBtwn->instruction.opcode != gcSL_NOP)
                    {
                        redundant = gcvFALSE;
                        break;
                    }
                }
                if (redundant)
                {
                    /* The jump is redundant. */
                    gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));
                    codeRemoved++;
                }
            }
            break;

        default:
            if (code->users == gcvNULL)
            {
                if (code->instruction.opcode == gcSL_TEXLD ||
                    code->instruction.opcode == gcSL_TEXLDP)
                {
                    if (code->prev)
                    {
                        if (code->prev->instruction.opcode == gcSL_TEXBIAS ||
                            code->prev->instruction.opcode == gcSL_TEXLOD ||
                            code->prev->instruction.opcode == gcSL_TEXGRAD)
                        {
                            bTexldIsRemoved = gcvTRUE;
                        }
                    }
                }

                /* The target is not used. */
                /* Change the instruction to NOP. */
                gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));

                codeRemoved++;
            }
            break;
        }
    }

    if (codeRemoved != 0)
    {
        DUMP_OPTIMIZER("Removed dead code from the shader", Optimizer);
        gcmFOOTER();
        return status;
    }

    gcmFOOTER();
    return status;
}

static gctUINT16
_GetSwizzle(
    IN gctUINT16 Swizzle,
    IN gctUINT16 Source
    )
{
    /* Select on swizzle. */
    switch ((gcSL_SWIZZLE) Swizzle)
    {
    case gcSL_SWIZZLE_X:
        /* Select swizzle x. */
        return gcmSL_SOURCE_GET(Source, SwizzleX);

    case gcSL_SWIZZLE_Y:
        /* Select swizzle y. */
        return gcmSL_SOURCE_GET(Source, SwizzleY);

    case gcSL_SWIZZLE_Z:
        /* Select swizzle z. */
        return gcmSL_SOURCE_GET(Source, SwizzleZ);

    case gcSL_SWIZZLE_W:
        /* Select swizzle w. */
        return gcmSL_SOURCE_GET(Source, SwizzleW);

    default:
        return (gctUINT16) 0xFFFF;
    }
}

static gctUINT8
_ConvertEnable2Swizzle(
    IN gctUINT32 Enable
    )
{
    switch (Enable)
    {
    case gcSL_ENABLE_X:
        return gcSL_SWIZZLE_XXXX;

    case gcSL_ENABLE_Y:
        return gcSL_SWIZZLE_YYYY;

    case gcSL_ENABLE_Z:
        return gcSL_SWIZZLE_ZZZZ;

    case gcSL_ENABLE_W:
        return gcSL_SWIZZLE_WWWW;

    case gcSL_ENABLE_XY:
        return gcSL_SWIZZLE_XYYY;

    case gcSL_ENABLE_XZ:
        return gcSL_SWIZZLE_XZZZ;

    case gcSL_ENABLE_XW:
        return gcSL_SWIZZLE_XWWW;

    case gcSL_ENABLE_YZ:
        return gcSL_SWIZZLE_YZZZ;

    case gcSL_ENABLE_YW:
        return gcSL_SWIZZLE_YWWW;

    case gcSL_ENABLE_ZW:
        return gcSL_SWIZZLE_ZWWW;

    case gcSL_ENABLE_XYZ:
        return gcSL_SWIZZLE_XYZZ;

    case gcSL_ENABLE_XYW:
        return gcSL_SWIZZLE_XYWW;

    case gcSL_ENABLE_XZW:
        return gcSL_SWIZZLE_XZWW;

    case gcSL_ENABLE_YZW:
        return gcSL_SWIZZLE_YZWW;

    case gcSL_ENABLE_XYZW:
        return gcSL_SWIZZLE_XYZW;
    }

    gcmFATAL("ERROR: Invalid enable 0x%04X", Enable);
    return gcSL_SWIZZLE_XYZW;
}

static gctUINT8
_ConvertSwizzle2Enable(
    IN gcSL_SWIZZLE X,
    IN gcSL_SWIZZLE Y,
    IN gcSL_SWIZZLE Z,
    IN gcSL_SWIZZLE W
    )
{
    static gctUINT8 _enable[] =
    {
        gcSL_ENABLE_X,
        gcSL_ENABLE_Y,
        gcSL_ENABLE_Z,
        gcSL_ENABLE_W
    };

    /* Return combined enables for each swizzle. */
    return _enable[X] | _enable[Y] | _enable[Z] | _enable[W];
}

static gctBOOL
_IsSingleChannelEnabled(IN gctUINT32 Enable)
{
    return ((Enable == gcSL_ENABLE_X) ||
            (Enable == gcSL_ENABLE_Y) ||
            (Enable == gcSL_ENABLE_Z) ||
            (Enable == gcSL_ENABLE_W));
}

/*******************************************************************************
**                          gcOpt_OptimizeMOVInstructions
********************************************************************************
**
**  Optimize MOV Instruction.
**      Remove unnecessary MOV instructions while honoring gcSL's restriction--
**      target cannot be the same as either of the source operands.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_OptimizeMOVInstructions(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcOPT_TEMP tempArray = Optimizer->tempArray;
    gctUINT16 enable;
    gctUINT codeRemoved;
    gcOPT_CODE code, depCode, iterCode;
    gcOPT_LIST list;
    gctUINT16 codeEnable;
    gctUINT16 depCodeEnable;
    gctBOOL bSelfMoveGenerated;
    const gctUINT enableCount[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    /* Pass 1: Check if a MOV instruction can be removed by replacing the dependant instructions' target
               with the target of the MOV instruction. */
    codeRemoved = 0;
    for (code = Optimizer->codeTail; code; code = code->prev)
    {
        /* Test for MOV instruction. */
        if (code->instruction.opcode != gcSL_MOV) continue;

        /* Skip MOV without dependancy. */
        if (code->dependencies0 == gcvNULL) continue;

        /* Skip MOV with multiple dependancies. */
        if (code->dependencies0->next != gcvNULL) continue;

        /* Skip function arguments in callers. */
        if (tempArray[code->instruction.tempIndex].function != gcvNULL)
        {
            if (tempArray[code->instruction.tempIndex].function != code->function)
            {
                if (tempArray[code->instruction.tempIndex].argument->qualifier == gcvFUNCTION_OUTPUT)
                {
                    continue;
                }
            }
        }

        /* Skip union. */
        if (tempArray[code->instruction.tempIndex].format == -2) continue;
        if (gcmSL_SOURCE_GET(code->instruction.source0, Type) == gcSL_TEMP &&
            tempArray[code->instruction.source0Index].format == -2) continue;

        /* Make sure the source is temp register and not indexed. */
        if ((gcmSL_SOURCE_GET(code->instruction.source0, Type) != gcSL_TEMP)
        ||  (gcmSL_SOURCE_GET(code->instruction.source0, Indexed) != gcSL_NOT_INDEXED))
        {
            continue;
        }

        /* Skip global register. */
        if (code->dependencies0->index < 0) continue;

        depCode = code->dependencies0->code;

        /* Skip if dependant instruction is a CALL instruction. */
        if (depCode->instruction.opcode == gcSL_CALL) continue;

        /* Skip if dependant instruction has different target format. */
        if (tempArray[depCode->instruction.tempIndex].format !=
            tempArray[code->instruction.tempIndex].format) continue;

        /* Skip if dependant instruction has different enable count. */
        codeEnable    = gcmSL_TARGET_GET(code->instruction.temp, Enable);
        depCodeEnable = gcmSL_TARGET_GET(depCode->instruction.temp, Enable);
        if (enableCount[codeEnable] != enableCount[depCodeEnable]) continue;

        /* Make sure the source is the same as the target of the dependant instruction. */
        /* Make sure the target of the current and the dependant instructions are not indexed. */
        if ((code->instruction.source0Index != depCode->instruction.tempIndex)
        ||  (gcmSL_TARGET_GET(code->instruction.temp, Indexed) != gcSL_NOT_INDEXED)
        ||  (gcmSL_TARGET_GET(depCode->instruction.temp, Indexed) != gcSL_NOT_INDEXED))
        {
            continue;
        }

        /* Special handling for LOAD. */
        /* The swizzle of input data for LOAD is always xyzw, */
        /* so not all enables in MOV instruction can be optimized. */
        if (depCode->instruction.opcode == gcSL_LOAD && codeEnable != depCodeEnable)
        {
            /* Assume data loaded are packed from x. */
            /* TODO - need to generalize if needed. */

            /* This is not true after MOV optimization. */
            /* gcmASSERT(depCodeEnable & gcSL_ENABLE_X); */

            if (depCodeEnable << 1 != codeEnable
            &&  depCodeEnable << 2 != codeEnable
            &&  depCodeEnable << 3 != codeEnable)
            {
                continue;
            }
        }

        /* Skip MOV with dependant that has multiple users. */
        if (depCode->users->next != gcvNULL) continue;

        /* gcSL does not allow target temp register the same as any of the source temp register. */
        bSelfMoveGenerated = gcvFALSE;
        if ((code->instruction.tempIndex == depCode->instruction.source0Index) &&
            (gcmSL_SOURCE_GET(depCode->instruction.source0, Type) == gcSL_TEMP))
        {
            if (depCode->instruction.opcode == gcSL_MOV)
            {
                if (_ConvertEnable2Swizzle(gcmSL_TARGET_GET(code->instruction.temp, Enable)) !=
                    (depCode->instruction.source0 >> 8))
                {
                    continue;
                }

                bSelfMoveGenerated = gcvTRUE;
            }
            else
                continue;
        }

        if ((code->instruction.tempIndex == depCode->instruction.source1Index) &&
            (gcmSL_SOURCE_GET(depCode->instruction.source1, Type) == gcSL_TEMP))
        {
            continue;
        }

        /* Make sure the dependency is swizzling the entire register usage. */
        if (_ConvertEnable2Swizzle(gcmSL_TARGET_GET(depCode->instruction.temp, Enable)) != (code->instruction.source0 >> 8))
        {
            continue;
        }

        /* Skip MOV with prev user after dependant instruction. */
        if (code->prevDefines)
        {
            for (list = code->prevDefines; list; list = list->next)
            {
                gcOPT_CODE pdCode;
                gcOPT_LIST ulist;

                if (list->index < 0) continue;

                pdCode = list->code;
                if (pdCode->id > depCode->id && pdCode->id < code->id) break;

                for (ulist = pdCode->users; ulist; ulist = ulist->next)
                {
                    if (ulist->index < 0) continue;
                    if (ulist->code->id > depCode->id && ulist->code->id < code->id) break;
                }

                if (ulist) break;
            }

            if (list) continue;
        }

        /* Make sure there is no jump in or out between code and depCode. */
        for (iterCode = code; iterCode != depCode; iterCode = iterCode->prev)
        {
            if (iterCode->callers || iterCode->instruction.opcode == gcSL_JMP)
            {
                break;
            }
        }
        if (iterCode != depCode)
        {
            continue;
        }

        /* Update data flow. */
        gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &code->dependencies0, depCode));
        gcmASSERT(code->dependencies0 == gcvNULL);
        gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &depCode->users, code));
        gcmASSERT(depCode->users == gcvNULL);
        depCode->users = code->users;
        code->users = gcvNULL;
        for (list = depCode->users; list; list = list->next)
        {
            gcOPT_CODE userCode;

            if (list->index < 0) continue;   /* Skip output and global dependency. */

            userCode = list->code;
            gcOpt_ReplaceCodeInList(Optimizer, &userCode->dependencies0, code, depCode);
            gcOpt_ReplaceCodeInList(Optimizer, &userCode->dependencies1, code, depCode);
        }

        /* Merge code into depCode. */
        /*depCode->instruction.temp        = code->instruction.temp;*/
        enable = gcmSL_TARGET_GET(code->instruction.temp, Enable);
        depCode->instruction.temp = gcmSL_TARGET_SET(depCode->instruction.temp, Enable, enable);
        depCode->instruction.tempIndex   = code->instruction.tempIndex;
        depCode->instruction.tempIndexed = code->instruction.tempIndexed;

        /* Update nextDefines and prevDefines. */
        if (depCode->nextDefines)
        {
            for (list = depCode->nextDefines; list; list = list->next)
            {
    			gcOpt_DeleteCodeFromList(Optimizer, &list->code->prevDefines, depCode);
            }
            if (depCode->prevDefines)
            {
                /* Add nextDefines to all prevDefines. */
                for (list = depCode->prevDefines; list; list = list->next)
                {
                    if (list->index == 0)
                    {
                        gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, depCode->nextDefines, &list->code->nextDefines));
                    }
                }
            }
    		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &depCode->nextDefines));
        }
        if (code->nextDefines)
        {
            gcmASSERT(depCode->nextDefines == gcvNULL);
            depCode->nextDefines = code->nextDefines;
            code->nextDefines = gcvNULL;
            for (list = depCode->nextDefines; list; list = list->next)
            {
                gcOpt_ReplaceCodeInList(Optimizer, &list->code->prevDefines, code, depCode);
            }
        }
        if (depCode->prevDefines)
        {
            for (list = depCode->prevDefines; list; list = list->next)
            {
                if (list->index == 0)
                {
        			gcOpt_DeleteCodeFromList(Optimizer, &list->code->nextDefines, depCode);
                }
            }
            if (depCode->nextDefines)
            {
                /* Add prevDefines to all nextDefines. */
                for (list = depCode->nextDefines; list; list = list->next)
                {
                    gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, depCode->prevDefines, &list->code->prevDefines));
                }
            }
    		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &depCode->prevDefines));
        }
        if (code->prevDefines)
        {
            gcmASSERT(depCode->prevDefines == gcvNULL);
            depCode->prevDefines = code->prevDefines;
            code->prevDefines = gcvNULL;
            for (list = depCode->prevDefines; list; list = list->next)
            {
                if (list->index == 0)
                {
                    gcOpt_ReplaceCodeInList(Optimizer, &list->code->nextDefines, code, depCode);
                }
            }
        }

        gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));
        /*code->instruction = gcvSL_NOP_INSTR;*/
        codeRemoved++;

        if (bSelfMoveGenerated)
        {
            /* Update DU info */
            for (list = depCode->dependencies0; list; list = list->next)
	        {
                gcOPT_LIST ulist;

                if (list->index < 0)
                    continue;

                for (ulist = depCode->users; ulist; ulist = ulist->next)
                {
                    if (ulist->index < 0)
                        continue;

                    if (gcOpt_CheckCodeInList(Optimizer, &ulist->code->dependencies0, depCode) == gcvSTATUS_TRUE)
                        gcOpt_AddCodeToList(Optimizer, &ulist->code->dependencies0, list->code);
                    if (gcOpt_CheckCodeInList(Optimizer, &ulist->code->dependencies1, depCode) == gcvSTATUS_TRUE)
                        gcOpt_AddCodeToList(Optimizer, &ulist->code->dependencies1, list->code);

                    gcOpt_AddCodeToList(Optimizer, &list->code->users, ulist->code);
                }
	        }

            gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, depCode));
            codeRemoved++;
        }
    }

    if (codeRemoved > 0)
    {
        status = gcvSTATUS_CHANGED;
    }

    /* Pass 2: Check if a MOV instruction can be removed by replacing the users' source
               with the source of the MOV instruction. */
    codeRemoved = 0;
    for (code = Optimizer->codeHead; code; code = code->next)
    {
        gctUINT16 source;
        gcOPT_LIST nlist;
        gcOPT_CODE reDefinedCode;
        gctBOOL usageRemoved = gcvFALSE;
        gctBOOL propagateUniform = gcvFALSE;

        /* Test for MOV instruction. */
        if (code->instruction.opcode != gcSL_MOV) continue;

        /* Skip if the target is a variable. */
        if (Optimizer->tempArray[code->instruction.tempIndex].arrayVariable)
        {
            gcVARIABLE variable = Optimizer->tempArray[code->instruction.tempIndex].arrayVariable;

            if (variable->arraySize > 1) continue;
            if (variable->u.type != gcSHADER_FLOAT_X1 &&
                variable->u.type != gcSHADER_INTEGER_X1) continue;
        }

        /* Skip if the source is a constant (already done in constant propagation. */
        if (gcmSL_SOURCE_GET(code->instruction.source0, Type) == gcSL_CONSTANT) continue;

        /* Skip function arguments in callers. */
        if (gcmSL_SOURCE_GET(code->instruction.source0, Type) == gcSL_TEMP &&
            tempArray[code->instruction.source0Index].function != gcvNULL)
        {
            if (tempArray[code->instruction.source0Index].function != code->function)
            {
                if (tempArray[code->instruction.source0Index].argument->qualifier == gcvFUNCTION_INPUT)
                {
                    continue;
                }
            }
        }

        if (code->users == gcvNULL) continue;

        /* Skip union. */
        if (tempArray[code->instruction.tempIndex].format == -2) continue;
        if (gcmSL_SOURCE_GET(code->instruction.source0, Type) == gcSL_TEMP &&
            tempArray[code->instruction.source0Index].format == -2) continue;

        /* Find the closest dependency temp's redefined pc. */
        reDefinedCode = Optimizer->codeTail;
        for (list = code->dependencies0; list; list = list->next)
        {
            /* Skip input arguments. */
            if (list->index < 0) break;

            /* Skip if depends on future change in a loop. */
            depCode = list->code;
            if (depCode->id >= code->id) break;

            for (nlist = depCode->nextDefines; nlist; nlist = nlist->next)
            {
                gcOPT_CODE nDefCode;

                gcmASSERT(nlist->index == 0);
                nDefCode = nlist->code;
                if (nDefCode->id < reDefinedCode->id && nDefCode->id > code->id)
                {
                    reDefinedCode = nDefCode;
                }
            }
        }

        if (list) continue;     /* Skip when future dependency exists. */

        if (gcmSL_SOURCE_GET(code->instruction.source0, Type) == gcSL_UNIFORM)
        {
            propagateUniform = gcvTRUE;
        }

        for (list = code->users; list; list = list->next)
        {
            gcOPT_CODE userCode;
            gctBOOL canReplace = gcvFALSE;
            gctBOOL cannotReplace = gcvFALSE;

            /* Skip output or global dependency. */
            if (list->index < 0) break;

            /* Skip if any user is over the source's next define. */
            userCode = list->code;
            if (userCode->id >= reDefinedCode->id) break;

            switch (userCode->instruction.opcode)
            {
            case gcSL_CALL:
                /* No inter-procedural propagation. */
            case gcSL_TEXBIAS:
            case gcSL_TEXGRAD:
            case gcSL_TEXLOD:
                /* Skip texture state instructions. */
            case gcSL_TEXLD:
            case gcSL_TEXLDP:
                /* Skip texture sample instructions. */

                cannotReplace = gcvTRUE;
            }

            if (cannotReplace) break;

            if (userCode->dependencies0 &&
                userCode->dependencies0->code == code &&
                userCode->dependencies0->next == gcvNULL &&
                gcmSL_SOURCE_GET(userCode->instruction.source0, Type) == gcSL_TEMP)
            {
                /* If source is a uniform, check if it causes two constants in an instruction. */
                if (propagateUniform &&
                    gcmSL_SOURCE_GET(userCode->instruction.source1, Type) == gcSL_CONSTANT)
                {
                    break;
                }

                canReplace = gcvTRUE;
            }
            if (userCode->dependencies1 &&
                userCode->dependencies1->code == code &&
                userCode->dependencies1->next == gcvNULL &&
                gcmSL_SOURCE_GET(userCode->instruction.source1, Type) == gcSL_TEMP)
            {
                /* If source is a uniform, check if it causes two constants in an instruction. */
                if (propagateUniform &&
                    gcmSL_SOURCE_GET(userCode->instruction.source0, Type) == gcSL_CONSTANT)
                {
                    break;
                }

                canReplace = gcvTRUE;
            }
            if (!canReplace) break;
        }

        if (list) continue;     /* Skip when not all users can be replaced. */

        source = code->instruction.source0;

        for (list = code->users; list; list = nlist)
        {
            gcOPT_CODE userCode;
            gctUINT16 source1;
            gcOPT_LIST ulist, depList;
            gctBOOL canRemoveUsage = gcvTRUE;

            nlist = list->next;

            gcmASSERT(list->index == 0);

            /* Get the user code. */
            userCode = list->code;

            /* gcSL does not allow target temp register the same as any of the source temp register. */
            if ((gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP) &&
                (userCode->instruction.tempIndex == code->instruction.source0Index))
            {
                continue;
            }

            if (userCode->dependencies0 &&
                userCode->dependencies0->code == code &&
                userCode->dependencies0->next == gcvNULL &&
                gcmSL_TARGET_GET(code->instruction.temp, Indexed) == gcSL_NOT_INDEXED &&
                gcmSL_SOURCE_GET(userCode->instruction.source0, Indexed) == gcSL_NOT_INDEXED)
            {
                /* Get source operands. */
                gctUINT16 source0 = userCode->instruction.source0;

                /* Test if source0 is the target of the MOV. */
                gcmASSERT(gcmSL_SOURCE_GET(source0, Type) == gcSL_TEMP &&
                          userCode->instruction.source0Index == code->instruction.tempIndex);

                /* Change userCode's source0 to code's source0. */
                source0 = gcmSL_SOURCE_SET(source0, Type, gcmSL_SOURCE_GET(source, Type));
                source0 = gcmSL_SOURCE_SET(source0, Indexed, gcmSL_SOURCE_GET(source, Indexed));
                source0 = gcmSL_SOURCE_SET(source0, Format, gcmSL_SOURCE_GET(source, Format));
                source0 = gcmSL_SOURCE_SET(source0, SwizzleX, _GetSwizzle(gcmSL_SOURCE_GET(source0, SwizzleX), source));
                source0 = gcmSL_SOURCE_SET(source0, SwizzleY, _GetSwizzle(gcmSL_SOURCE_GET(source0, SwizzleY), source));
                source0 = gcmSL_SOURCE_SET(source0, SwizzleZ, _GetSwizzle(gcmSL_SOURCE_GET(source0, SwizzleZ), source));
                source0 = gcmSL_SOURCE_SET(source0, SwizzleW, _GetSwizzle(gcmSL_SOURCE_GET(source0, SwizzleW), source));

                userCode->instruction.source0        = source0;
                userCode->instruction.source0Index   = code->instruction.source0Index;
                userCode->instruction.source0Indexed = code->instruction.source0Indexed;

                /* Update data flow. */
                gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &userCode->dependencies0, code));

                if (code->dependencies0)
                {
                    gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, code->dependencies0, &userCode->dependencies0));
                    for (ulist = code->dependencies0; ulist; ulist = ulist->next)
                    {
                        if (ulist->index >= 0)
                        {
                            gcmVERIFY_OK(gcOpt_AddCodeToList(Optimizer, &ulist->code->users, userCode));
                        }
                    }
                }
            }

            if (userCode->dependencies1 &&
                userCode->dependencies1->code == code &&
                userCode->dependencies1->next == gcvNULL &&
                gcmSL_TARGET_GET(code->instruction.temp, Indexed) == gcSL_NOT_INDEXED &&
                gcmSL_SOURCE_GET(userCode->instruction.source1, Indexed) == gcSL_NOT_INDEXED)
            {
                /* Get source operands. */
                source1 = userCode->instruction.source1;

                /* Test if source1 is the target of the MOV. */
                gcmASSERT(gcmSL_SOURCE_GET(source1, Type) == gcSL_TEMP &&
                          userCode->instruction.source1Index == code->instruction.tempIndex);

                /* Change userCode's source0 to code's source0. */
                source1 = gcmSL_SOURCE_SET(source1, Type, gcmSL_SOURCE_GET(source, Type));
                source1 = gcmSL_SOURCE_SET(source1, Indexed, gcmSL_SOURCE_GET(source, Indexed));
                source1 = gcmSL_SOURCE_SET(source1, Format, gcmSL_SOURCE_GET(source, Format));
                source1 = gcmSL_SOURCE_SET(source1, SwizzleX, _GetSwizzle(gcmSL_SOURCE_GET(source1, SwizzleX), source));
                source1 = gcmSL_SOURCE_SET(source1, SwizzleY, _GetSwizzle(gcmSL_SOURCE_GET(source1, SwizzleY), source));
                source1 = gcmSL_SOURCE_SET(source1, SwizzleZ, _GetSwizzle(gcmSL_SOURCE_GET(source1, SwizzleZ), source));
                source1 = gcmSL_SOURCE_SET(source1, SwizzleW, _GetSwizzle(gcmSL_SOURCE_GET(source1, SwizzleW), source));

                userCode->instruction.source1        = source1;
                userCode->instruction.source1Index   = code->instruction.source0Index;
                userCode->instruction.source1Indexed = code->instruction.source0Indexed;

                /* Update data flow. */
                gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &userCode->dependencies1, code));

                if (code->dependencies0)
                {
                    gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, code->dependencies0, &userCode->dependencies1));
                    for (ulist = code->dependencies0; ulist; ulist = ulist->next)
                    {
                        if (ulist->index >= 0)
                        {
                            gcmVERIFY_OK(gcOpt_AddCodeToList(Optimizer, &ulist->code->users, userCode));
                        }
                    }
                }
            }

            /* It is possible that both sources use the same temp register, so before we can */
            /* delete usage for this instruction against MOV instruction, we need assure that */
            /* all dependencies on two sources has been cleaned up with this MOV instruction */
            for (depList = userCode->dependencies0; depList; depList = depList->next)
            {
                if (depList->code == code)
                {
                    canRemoveUsage = gcvFALSE;
                    break;
                }
            }

            if (canRemoveUsage)
            {
                for (depList = userCode->dependencies1; depList; depList = depList->next)
                {
                    if (depList->code == code)
                    {
                        canRemoveUsage = gcvFALSE;
                        break;
                    }
                }
            }

            /* Yes, we can remove safely now */
            if (canRemoveUsage)
            {
                gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &code->users, userCode));
                usageRemoved = gcvTRUE;
            }
        }

        if (code->users == gcvNULL)
        {
            /* No more user for the MOV instruction, change it to NOP. */
            gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));

        }
        else if (usageRemoved)
        {
            /* Try to diminish channel */

            gctUINT16 remainderEnable = 0;
            gctUINT16 source;
            gctBOOL canDiminishChannel = gcvTRUE;

            for (list = code->users; list; list = list->next)
            {
                gcOPT_CODE userCode;
                gcOPT_LIST depList;
                gctUINT16 thisEnable;

                /* Get the user code. */
                userCode = list->code;
                for (depList = userCode->dependencies0; depList; depList = depList->next)
                {
                    /* source0 has a usage from MOV instruction */
                    if (depList->code == code)
                    {
                        source = userCode->instruction.source0;

                        /* Since currently our DFA is not very accurate on channels, we only */
                        /* consider single-channel case conservationally */
                        thisEnable = _ConvertSwizzle2Enable((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
                                                            (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
                                                            (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
                                                            (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW));

                        if (!_IsSingleChannelEnabled(thisEnable))
                        {
                            canDiminishChannel = gcvFALSE;
                            break;
                        }
                        else
                        {
                            remainderEnable |= thisEnable;
                        }
                    }
                }

                for (depList = userCode->dependencies1; depList; depList = depList->next)
                {
                    source = userCode->instruction.source1;

                    /* source1 has a usage from MOV instruction */
                    if (depList->code == code)
                    {
                        source = userCode->instruction.source1;

                        /* Since currently our DFA is not very accurate on channels, we only */
                        /* consider single-channel case conservationally */
                        thisEnable = _ConvertSwizzle2Enable((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
                                                            (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
                                                            (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
                                                            (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW));

                        if (!_IsSingleChannelEnabled(thisEnable))
                        {
                            canDiminishChannel = gcvFALSE;
                            break;
                        }
                        else
                        {
                            remainderEnable |= thisEnable;
                        }
                    }
                }
            }

            remainderEnable &= gcmSL_TARGET_GET(code->instruction.temp, Enable);
            if (remainderEnable && canDiminishChannel)
            {
                /* Ok, we can remove other channels rather than channels enabled by remainderEnable */

                gctUINT16 disable = ~remainderEnable;
                gctUINT16 swizzle;

                code->instruction.temp = gcmSL_TARGET_SET(code->instruction.temp, Enable, remainderEnable);

                source = code->instruction.source0;

                if (remainderEnable & gcSL_ENABLE_X)
                    swizzle = gcmSL_SOURCE_GET(source, SwizzleX);
                else if (remainderEnable & gcSL_ENABLE_Y)
                    swizzle = gcmSL_SOURCE_GET(source, SwizzleY);
                else if (remainderEnable & gcSL_ENABLE_Z)
                    swizzle = gcmSL_SOURCE_GET(source, SwizzleZ);
                else
                    swizzle = gcmSL_SOURCE_GET(source, SwizzleW);

                source = (disable & gcSL_ENABLE_X) ? gcmSL_SOURCE_SET(source, SwizzleX, swizzle) : source;
                source = (disable & gcSL_ENABLE_Y) ? gcmSL_SOURCE_SET(source, SwizzleY, swizzle) : source;
                source = (disable & gcSL_ENABLE_Z) ? gcmSL_SOURCE_SET(source, SwizzleZ, swizzle) : source;
                source = (disable & gcSL_ENABLE_W) ? gcmSL_SOURCE_SET(source, SwizzleW, swizzle) : source;

                code->instruction.source0 = source;
            }
        }

        codeRemoved++;
    }

    if (codeRemoved > 0)
    {
        status = gcvSTATUS_CHANGED;
    }

    /* Pass 3: Check if a MOV instruction can be removed because it is the same
               as the previous MOV instruction. */
    codeRemoved = 0;
    for (code = Optimizer->codeTail; code; code = code->prev)
    {
        gcOPT_LIST nlist;

        /* Test for MOV instruction. */
        if (code->instruction.opcode != gcSL_MOV) continue;

        /* Skip if there are callers. */
        if (code->callers) break;

        /* Skip if no prevDefines or more than one preDefines. */
        if (code->prevDefines == gcvNULL) continue;
        if (code->prevDefines->next) continue;

        /* Skip if prevDefine is not exact the same instruction. */
        depCode = code->prevDefines->code;
        if (depCode == gcvNULL) continue;
        if (depCode->instruction.opcode != code->instruction.opcode) continue;
        if (depCode->instruction.temp != code->instruction.temp) continue;
        if (depCode->instruction.tempIndex != code->instruction.tempIndex) continue;
        if (depCode->instruction.tempIndexed != code->instruction.tempIndexed) continue;
        if (depCode->instruction.source0 != code->instruction.source0) continue;
        if (depCode->instruction.source0Index != code->instruction.source0Index) continue;
        if (depCode->instruction.source0Indexed != code->instruction.source0Indexed) continue;

        /* Skip if there is any jump out/in in between. */
        for (iterCode = depCode->next; iterCode != code; iterCode = iterCode->next)
        {
            if (iterCode->callers) break;
            if (iterCode->instruction.opcode == gcSL_JMP ||
                iterCode->instruction.opcode == gcSL_CALL) break;
        }
        if (iterCode != code) continue;

        /* Skip if the dependencies are not the same. */
        if (depCode->dependencies0)
        {
            if (code->dependencies0 == gcvNULL) continue;

            for (list = depCode->dependencies0; list; list = list->next)
            {
                for (nlist = code->dependencies0; nlist; nlist = nlist->next)
                {
                    if (nlist->index != list->index) continue;
                    if (nlist->index >= 0 && nlist->code == list->code) break;
                }

                if (nlist == gcvNULL) break;
            }

            if (list) continue;

            for (list = code->dependencies0; list; list = list->next)
            {
                for (nlist = depCode->dependencies0; nlist; nlist = nlist->next)
                {
                    if (nlist->index != list->index) continue;
                    if (nlist->index >= 0 && nlist->code == list->code) break;
                }

                if (nlist == gcvNULL) break;
            }

            if (list) continue;
        }
        else if (code->dependencies0) continue;

        /* Remove code since it is redundant. */
        gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));
        /*code->instruction = gcvSL_NOP_INSTR;*/
        codeRemoved++;
    }

    if (codeRemoved > 0)
    {
        status = gcvSTATUS_CHANGED;
    }

    if (status == gcvSTATUS_CHANGED)
    {
        /* Rebuild flow graph. */
    	gcmONERROR(gcOpt_RebuildFlowGraph(Optimizer));

        DUMP_OPTIMIZER("Optimize MOV instructions in the shader", Optimizer);
    }

    gcmFOOTER();
    return status;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

static gctBOOL
_IsTempModified(
    IN gcOPTIMIZER Optimizer,
    IN gctUINT Index,
    IN gcOPT_CODE Code1,
    IN gcOPT_CODE Code2
    )
{
    gcOPT_CODE code;

    for (code = Code1->next; code != gcvNULL && code != Code2; code = code->next)
    {
        switch (code->instruction.opcode)
        {
        case gcSL_CALL:
            /* Check global and output. */
            {
            /* Add dependencies for input and output arguments. */
            gcOPT_FUNCTION function = code->callee->function;
            gcsFUNCTION_ARGUMENT_PTR argument;
            gcOPT_GLOBAL_USAGE usage;
            gctUINT a;

            /* Add dependencies for all input arguments, and add users for all output arguments. */
            argument = function->arguments;
            for (a = 0; a < function->argumentCount; a++, argument++)
            {
                /* Check output arguments. */
                if (argument->qualifier != gcvFUNCTION_INPUT)
                {
                    if (function->arguments[a].index == Index) return gcvTRUE;
                }
            }

            /* Add global variable dependencies and users. */
            for (usage = function->globalUsage; usage; usage = usage->next)
            {
                if (usage->direction != gcvFUNCTION_INPUT)
                {
                    if (usage->index == Index) return gcvTRUE;
                }
            }
            }
            break;

        case gcSL_RET:
        case gcSL_JMP:
            /* Should not happen because it is checked before. */
            gcmASSERT(0);
            break;

        case gcSL_NOP:
        case gcSL_KILL:
        case gcSL_TEXBIAS:
        case gcSL_TEXGRAD:
        case gcSL_TEXLOD:
            /* Skip no temp change instructions. */
            break;

        default:
            if (code->instruction.tempIndex == Index) return gcvTRUE;

            /* Since the value of indexed reg is unknown, just be cautious. */
            if (gcmSL_TARGET_GET(code->instruction.temp, Indexed) != gcSL_NOT_INDEXED) return gcvTRUE;

            break;
        }
    }

    return gcvFALSE;
}

static gctBOOL
_IsSourceModified(
    IN gcOPTIMIZER Optimizer,
    IN gctUINT16 Source,
    IN gctUINT SourceIndex,
    IN gctUINT SourceIndexed,
    IN gcOPT_LIST Dependencies,
    IN gcOPT_CODE Code1,
    IN gcOPT_CODE Code2
    )
{
    gcOPT_LIST list, dlist;
    gctBOOL needDetailedCheck = gcvFALSE;

    for (list = Dependencies; list; list = list->next)
    {
        if (list->index < 0)
        {
            needDetailedCheck = gcvTRUE;
            continue;
        }
        for (dlist = list->code->nextDefines; dlist; dlist = dlist->next)
        {
            if (dlist->index == 0)
            {
                gctUINT id = dlist->code->id;
                if (id > Code1->id && id < Code2->id) return gcvTRUE;
            }
        }
    }

    if (! needDetailedCheck) return gcvFALSE;

    if (gcmSL_SOURCE_GET(Source, Type) == gcSL_TEMP)
    {
        if (_IsTempModified(Optimizer, SourceIndex, Code1, Code2)) return gcvTRUE;
    }

    if (gcmSL_SOURCE_GET(Source, Indexed) == gcSL_NOT_INDEXED) return gcvFALSE;

    if (_IsTempModified(Optimizer, SourceIndexed, Code1, Code2)) return gcvTRUE;

    return gcvFALSE;
}

static gctBOOL
_IsADDForMADD(
    IN gcOPTIMIZER Optimizer,
    IN gcOPT_CODE Code,
    IN gctUINT16 Source,
    IN gcOPT_LIST Dependencies
    )
{
    gcOPT_CODE code, depCode;
    gctUINT16 enable;

    /* Check if the source is direct temp register. */
    if (gcmSL_SOURCE_GET(Source, Type) != gcSL_TEMP) return gcvFALSE;
    if (gcmSL_SOURCE_GET(Source, Indexed) != gcSL_NOT_INDEXED) return gcvFALSE;

    /* Check if only one dependency. */
    if (Dependencies == gcvNULL) return gcvFALSE;
    if (Dependencies->next != gcvNULL) return gcvFALSE;

    /* Check if the dependent code is MUL. */
    if (Dependencies->index < 0) return gcvFALSE;
    depCode = Dependencies->code;
    if (depCode->instruction.opcode != gcSL_MUL) return gcvFALSE;
    if (gcmSL_TARGET_GET(depCode->instruction.temp, Indexed) != gcSL_NOT_INDEXED) return gcvFALSE;

    /* Check if the enables are the same. */
    enable = gcmSL_TARGET_GET(Code->instruction.temp, Enable);
    if (gcmSL_TARGET_GET(depCode->instruction.temp, Enable) != enable) return gcvFALSE;

    /* Check if depCode has only one user, code. */
    gcmASSERT(depCode->users != gcvNULL);
    if (depCode->users == gcvNULL)  return gcvFALSE;
    if (depCode->users->next != gcvNULL)  return gcvFALSE;
    gcmASSERT(depCode->users->code == Code);

    /* Check if the swizzle matches the enable. */
    if ((enable & gcSL_ENABLE_X) && gcmSL_SOURCE_GET(Source, SwizzleX) != gcSL_SWIZZLE_X) return gcvFALSE;
    if ((enable & gcSL_ENABLE_Y) && gcmSL_SOURCE_GET(Source, SwizzleY) != gcSL_SWIZZLE_Y) return gcvFALSE;
    if ((enable & gcSL_ENABLE_Z) && gcmSL_SOURCE_GET(Source, SwizzleZ) != gcSL_SWIZZLE_Z) return gcvFALSE;
    if ((enable & gcSL_ENABLE_W) && gcmSL_SOURCE_GET(Source, SwizzleW) != gcSL_SWIZZLE_W) return gcvFALSE;

    /* Check if any branch-out instruction or jump-in point between depPc and Pc. */
    /* This should not happen, but be cautious. */
    gcmASSERT(depCode->id < Code->id);
    if (depCode->callers != gcvNULL) return gcvFALSE;
    for (code = depCode->next;code != gcvNULL && code != Code; code = code->next)
    {
        if (code->callers != gcvNULL) return gcvFALSE;
        if (code->instruction.opcode == gcSL_JMP &&
            (code->callee->id < depCode->id || code->callee->id > Code->id)) return gcvFALSE;
        /* Not likely, but be cautious. */
        if (code->instruction.opcode == gcSL_RET) return gcvFALSE;
    }

    /* Check if MUL instruction can be moved to right before the ADD instruction. */
    /* Check if source0 may be modified between depPc and Pc. */
    if (depCode->dependencies0 != gcvNULL &&
        _IsSourceModified(Optimizer, depCode->instruction.source0, depCode->instruction.source0Index,
                          depCode->instruction.source0Indexed, depCode->dependencies0, depCode, Code)) return gcvFALSE;

    /* Check if source1 may be modified between depPc and Pc. */
    if (depCode->dependencies1 != gcvNULL &&
        _IsSourceModified(Optimizer, depCode->instruction.source1, depCode->instruction.source1Index,
                          depCode->instruction.source1Indexed, depCode->dependencies1, depCode, Code)) return gcvFALSE;

    /* TODO - Check if ADD instruction can be moved to right after the MUL instruction. */

    return gcvTRUE;
}

/*******************************************************************************
**                          gcOpt_OptimizeMULADDInstructions
********************************************************************************
**
**  Optimize MUL and ADD Instructions for MADD instructions.
**      Move MUL and ADD instructions with MADD instruction pattern to be together
**      for backend linker to convert them into MADD instructions.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_OptimizeMULADDInstructions(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcOPT_CODE code, codeMUL;
    gctUINT codeMoved;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    /* Check if one of a ADD instruction's dependency is only one MUL instruction,
       and the MUL's target's enable is the same as the ADD's dependency's enable.
       Then, move the MUL instruction to right before ADD or move ADD instruction
       to right after MUL instruction. */
    do
    {
        codeMoved = 0;
        for (code = Optimizer->codeTail; code; code = code->prev)
        {
            /* Check for ADD/SUB instruction. */
            if (code->instruction.opcode != gcSL_ADD &&
                code->instruction.opcode != gcSL_SUB) continue;

            /* Check if the ADD instruction can be combined with MUL to MADD instruction. */
            if (_IsADDForMADD(Optimizer, code, code->instruction.source0, code->dependencies0))
            {
                /* Check if MUL is right before ADD. */
                if (code->dependencies0->code == code->prev)
                    continue;

                /* Check if dependencies1 is also MUL and right before ADD. */
                if (_IsADDForMADD(Optimizer, code, code->instruction.source1, code->dependencies1))
                {
                    /* Check if MUL is right before ADD. */
                    if (code->dependencies1->code == code->prev)
                        continue;
                }

                /* Find one candidate. */
                codeMUL = code->dependencies0->code;
                gcOpt_MoveCodeListBefore(Optimizer, codeMUL, codeMUL, code);
                codeMoved++;
            }
            else if (_IsADDForMADD(Optimizer, code, code->instruction.source1, code->dependencies1))
            {
                /* Check if MUL is right before ADD. */
                if (code->dependencies1->code == code->prev)
                    continue;

                /* Find one candidate. */
                codeMUL = code->dependencies1->code;
                gcOpt_MoveCodeListBefore(Optimizer, codeMUL, codeMUL, code);
                codeMoved++;
            }
        }

        if (codeMoved > 0)
        {
            status = gcvSTATUS_CHANGED;
        }
    }
    while (codeMoved > 0);

    if (status == gcvSTATUS_CHANGED)
    {
        DUMP_OPTIMIZER("Optimize MUL and ADD for MADD instructions in the shader", Optimizer);
        gcmFOOTER();
        return status;
    }

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                          _ExpandOneFunctionCall
********************************************************************************
**
**  Expand one functon into the caller, and remove the function if specified.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
**
**      gcOPT_FUNCTION function
**          Pointer to a gcOPT_FUNCTION structure.
**
**      gctUINT caller
**          Caller location.
**
**      gctBOOL removeFunction
**          Whether to remove the function.
*/
static gceSTATUS
_ExpandOneFunctionCall(
    IN gcOPTIMIZER Optimizer,
    IN gcOPT_FUNCTION Function,
    IN gcOPT_CODE Caller,
    IN gctBOOL WillRemoveFunction
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcOPT_CODE codeNext = Caller->next;
    gcOPT_CODE code;

    gcmHEADER_ARG("Optimizer=0x%x Function=0x%x Caller=0x%x WillRemoveFunction=%d",
                   Optimizer, Function, Caller, WillRemoveFunction);

    /* After expansion, the CALL instruction will be after the function block. */
    if (WillRemoveFunction)
    {
        /* Expand the function by moving the code to after caller. */
        gcOpt_MoveCodeListAfter(Optimizer,
                                    Function->codeHead,
                                    Function->codeTail,
                                    Caller);
        Function->codeHead = gcvNULL;
        Function->codeTail = gcvNULL;
    }
    else
    {
        /* Expand the function by copying the code to after caller. */
        status = gcOpt_CopyCodeListAfter(Optimizer,
                                    Function->codeHead,
                                    Function->codeTail,
                                    Caller);
    }

    if (status != gcvSTATUS_OK)
    {
        gcmFOOTER();
        return status;
    }

    /* Change the caller to NOP. */
    Caller->instruction = gcvSL_NOP_INSTR;

    /* If the last instruction of the function is RET, change it to NOP. */
    code = codeNext->prev;
    if (code->instruction.opcode == gcSL_RET)
    {
        code->instruction = gcvSL_NOP_INSTR;
    }

    /* Change the rest RET instructions to JMP. */
    for (code = code->prev; code != gcvNULL && code != Caller; code = code->prev)
    {
        if (code->instruction.opcode == gcSL_RET)
        {
            code->instruction.opcode = gcSL_JMP;
            code->instruction.tempIndex = (gctUINT16) codeNext->id;

            /* Set caller and callee. */
            code->callee = codeNext;
            gcmERR_RETURN(gcOpt_AddCodeToList(Optimizer, &codeNext->callers, code));
        }
    }

    /*DUMP_OPTIMIZER("Expand a function of the shader", Optimizer);*/
    gcmFOOTER();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**                          gcOpt_ExpandFunctions
********************************************************************************
**
**  Expand functons that are called once or short but with many arguments.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_ExpandFunctions(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT functionRemoved = 0;
    gctINT i;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    if (Optimizer->functionCount == 0)
    {
        gcmFOOTER();
        return status;
    }

    /* Id's will be used to calculate functions' codeCount. */
    gcOpt_UpdateCodeId(Optimizer);

    for (i = Optimizer->functionCount - 1; i >= 0; i--)
    {
        gcOPT_FUNCTION function = Optimizer->functionArray + i;
        gctUINT realCallerCount = 0;
        gcOPT_LIST caller;
        gcOPT_CODE codeCaller;
#if __SUPPORT_OCL__
        gctBOOL isMainKernelFunction = gcvFALSE;
#endif

        /* Calculate the real caller count. */
        for (caller = function->codeHead->callers;
             caller != gcvNULL;
             caller = caller->next)
        {
            if (caller->code->instruction.opcode == gcSL_CALL)
            {
                realCallerCount++;
            }
        }

        /* Check if function is unnecessary. */
        /* Function is called only once or is short but with many arguments. */
        if (realCallerCount > 1 &&
            function->codeTail->id - function->codeHead->id > function->argumentCount + 1)
        {
            continue;
        }

        if (realCallerCount > 0)
        {
            do
            {
                /* The following call should be in gcOpt_copyCodeListAfter. */
                /* Move here to avoid an assertion, which should be removed later after testing. */
                /* Need to update code id for checking. */
                gcOpt_UpdateCodeId(Optimizer);

                realCallerCount--;

                /* Expand the function. */
                /* Note that hintArray is rebuilt after each function expansion. */
                for (caller = function->codeHead->callers;
                     caller != gcvNULL;
                     caller = caller->next)
                {
                    if (caller->code->instruction.opcode == gcSL_CALL) break;
                }

                gcmASSERT(caller);

                if (caller == gcvNULL)
                {
                    return gcvSTATUS_INVALID_ARGUMENT;
                }

                codeCaller = caller->code;
                codeCaller->callee = gcvNULL;

#if __SUPPORT_OCL__
                if (function->kernelFunction && codeCaller->function == gcvNULL)
                {
                    isMainKernelFunction = gcvTRUE;
                }
#endif

                /* Remove caller from the caller list. */
		        gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer,
                                &function->codeHead->callers,
                                codeCaller));

                gcmONERROR(_ExpandOneFunctionCall(Optimizer,
                                function,
                                codeCaller,
                                (realCallerCount == 0) /* willRemoveFunction */));
            }
            while (realCallerCount > 0);
        }

#if __SUPPORT_OCL__
        if (isMainKernelFunction)
        {
            gcmASSERT(Optimizer->main->shaderFunction == gcvNULL);
            gcmASSERT(Optimizer->main->kernelFunction == gcvNULL);
            Optimizer->main->kernelFunction = function->kernelFunction;
            Optimizer->isMainMergeWithKerenel = gcvTRUE;
        }
#endif

        gcmONERROR(gcOpt_DeleteFunction(Optimizer, function));

        functionRemoved++;
    }

    if (functionRemoved)
    {
        /* Code optimzed. */
        DUMP_OPTIMIZER("Expand unnecessary functions in the shader", Optimizer);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_CHANGED);
        return gcvSTATUS_CHANGED;
    }
    else
    {
        /* Code unchanged. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                          gcOpt_PropagateConstants
********************************************************************************
**
**  Propagate constants.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_PropagateConstants(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcOPT_CODE code;
    gctUINT constPropagated = 0;
    gctUINT16 source, source1;
    gctUINT16 sourceUint = 0;
    gctUINT16 sourceIndex = 0;
    gctUINT16 sourceIndexed = 0;
    gctUINT32 value = 0, value0, value1;
    gcSL_FORMAT format0, format1;
    gctBOOL needToPropagate;
    gcOPT_LIST list, nextList;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    /* for each constant, propagate it. */
    for (code = Optimizer->codeHead; code; code = code->next)
    {
        needToPropagate = gcvFALSE;

        switch (code->instruction.opcode)
        {
        case gcSL_MOV:
            source = code->instruction.source0;
            if (gcmSL_SOURCE_GET(source, Type) == gcSL_CONSTANT)
            {
                gcmASSERT(code->dependencies0 == gcvNULL);
                needToPropagate = gcvTRUE;
                sourceUint      = code->instruction.source0;
                sourceIndex     = code->instruction.source0Index;
                sourceIndexed   = code->instruction.source0Indexed;
            }
            break;

#if __SUPPORT_OCL__
#endif

        case gcSL_SAT:
        case gcSL_RCP:
        case gcSL_RSQ:
        case gcSL_LOG:
        case gcSL_FRAC:
        case gcSL_FLOOR:
        case gcSL_CEIL:
            /* One-operand float computational instruction. */
            source = code->instruction.source0;
            if (gcmSL_SOURCE_GET(source, Type) != gcSL_CONSTANT) break;
            gcmASSERT(code->dependencies0 == gcvNULL);
            format0 = (gcSL_FORMAT) gcmSL_SOURCE_GET(source, Format);

            /* Assemble the 32-bit value. */
            value0 = (code->instruction.source0Index & 0xFFFF) | (code->instruction.source0Indexed << 16);

            if (format0 == gcSL_FLOAT)
            {
                gctFLOAT f, f0;

                f = f0 = gcoMATH_UIntAsFloat(value0);
                sourceUint = code->instruction.source0;

                switch (code->instruction.opcode)
                {
                case gcSL_SAT:
                    f = f0 > 1.0f ? 1.0f : f < 0.0f ? 0.0f : f0;
                    break;
                case gcSL_RCP:
                    f = gcoMATH_Divide(1.0f, f0);
                    break;
                case gcSL_RSQ:
                    f = gcoMATH_ReciprocalSquareRoot(f0);
                    break;
                case gcSL_LOG:
                    f = gcoMATH_Log2(f0);
                    break;
                case gcSL_FRAC:
                    f = gcoMATH_Add(f0, -gcoMATH_Floor(f0));
                    break;
                case gcSL_FLOOR:
                    f = gcoMATH_Floor(f0);
                    break;
                case gcSL_CEIL:
                    f = gcoMATH_Ceiling(f0);
                    break;
                }
                value = gcoMATH_FloatAsUInt(f);
                sourceIndex = value & 0xFFFF;
                sourceIndexed = (value >> 16) & 0xFFFF;

                needToPropagate = gcvTRUE;
            }
            else
            {
                gcmASSERT(format0 == gcSL_FLOAT);
            }
            break;

        case gcSL_ABS:
            /* One-operand computational instruction. */
            source = code->instruction.source0;
            if (gcmSL_SOURCE_GET(source, Type) != gcSL_CONSTANT) break;
            gcmASSERT(code->dependencies0 == gcvNULL);
            format0 = (gcSL_FORMAT) gcmSL_SOURCE_GET(source, Format);

            /* Assemble the 32-bit value. */
            value0 = (code->instruction.source0Index & 0xFFFF) | (code->instruction.source0Indexed << 16);

            if (format0 == gcSL_FLOAT)
            {
                gctFLOAT f, f0;

                f = f0 = gcoMATH_UIntAsFloat(value0);
                sourceUint = code->instruction.source0;

                switch (code->instruction.opcode)
                {
                case gcSL_ABS:
                    f = f0 > 0.0f ? f0 : -f0;
                    break;
                }
                value = gcoMATH_FloatAsUInt(f);
                sourceIndex = value & 0xFFFF;
                sourceIndexed = (value >> 16) & 0xFFFF;

                needToPropagate = gcvTRUE;
            }
            else if (format0 == gcSL_INTEGER)
            {
                gctINT i, i0;

                i = i0 = *(gctINT *) &value0;
                sourceUint = code->instruction.source0;

                switch (code->instruction.opcode)
                {
                case gcSL_ABS:
                    i = gcmABS(i0);
                    break;
                }
                value = *(gctUINT *) &i;
                sourceIndex = value & 0xFFFF;
                sourceIndexed = (value >> 16) & 0xFFFF;

                needToPropagate = gcvTRUE;
            }
            else if (format0 == gcSL_UINT32)
            {
                gctUINT i, i0;

                i = i0 = *(gctUINT *) &value0;
                sourceUint = code->instruction.source0;

                switch (code->instruction.opcode)
                {
                case gcSL_ABS:
                    i = i0;
                    break;
                }
                value = *(gctUINT *) &i;
                sourceIndex = value & 0xFFFF;
                sourceIndexed = (value >> 16) & 0xFFFF;

                needToPropagate = gcvTRUE;
            }
            else
            {
                /* Error. */
                gcmFOOTER();
                return gcvSTATUS_INVALID_DATA;
            }
            break;

#if __SUPPORT_OCL__
        case gcSL_AND_BITWISE:
        case gcSL_OR_BITWISE:
        case gcSL_XOR_BITWISE:
        case gcSL_NOT_BITWISE:
        case gcSL_LSHIFT:
        case gcSL_RSHIFT:
        case gcSL_ROTATE:
        case gcSL_DIV:
        case gcSL_MOD:
            /* Two-operand integer computational instruction. */
            break;
#endif

        case gcSL_POW:
            /* Two-operand float computational instruction. */

        case gcSL_ADD:
        case gcSL_SUB:
        case gcSL_MUL:
        case gcSL_MAX:
        case gcSL_MIN:
            /* Two-operand computational instruction. */
            source  = code->instruction.source0;
            format0 = (gcSL_FORMAT) gcmSL_SOURCE_GET(source, Format);
            source1 = code->instruction.source1;
            format1 = (gcSL_FORMAT) gcmSL_SOURCE_GET(source1, Format);

            if ((gcmSL_SOURCE_GET(source,  Type) != (gctUINT16) gcSL_CONSTANT)
            &&  (gcmSL_SOURCE_GET(source1, Type) != (gctUINT16) gcSL_CONSTANT)
            )
            {
                break;
            }

            /* Assemble the 32-bit value. */
            value0 = (code->instruction.source0Index & 0xFFFF) | (code->instruction.source0Indexed << 16);
            value1 = (code->instruction.source1Index & 0xFFFF) | (code->instruction.source1Indexed << 16);

            if (gcmSL_SOURCE_GET(source, Type) != gcSL_CONSTANT || gcmSL_SOURCE_GET(source1, Type) != gcSL_CONSTANT)
            {
                if (code->instruction.opcode == gcSL_ADD)
                {
                    if (gcmSL_SOURCE_GET(source, Type) == gcSL_CONSTANT)
                    {
                        if (format0 == gcSL_FLOAT)
                        {
                            float f = gcoMATH_UIntAsFloat(value0);
                            if (f != 0.0f) break;

                            /* It is equivalent to MOV instruction. */
                            code->instruction.opcode         = gcSL_MOV;
                            code->instruction.source0        = code->instruction.source1;
                            code->instruction.source0Index   = code->instruction.source1Index;
                            code->instruction.source0Indexed = code->instruction.source1Indexed;
                            code->instruction.source1        = 0;
                            code->instruction.source1Index   = 0;
                            code->instruction.source1Indexed = 0;
                        }
                        else if (format0 == gcSL_INTEGER)
                        {
                            gctINT i = *(gctINT *) &value0;
                            if (i != 0) break;

                            /* It is equivalent to MOV instruction. */
                            code->instruction.opcode         = gcSL_MOV;
                            code->instruction.source0        = code->instruction.source1;
                            code->instruction.source0Index   = code->instruction.source1Index;
                            code->instruction.source0Indexed = code->instruction.source1Indexed;
                            code->instruction.source1        = 0;
                            code->instruction.source1Index   = 0;
                            code->instruction.source1Indexed = 0;
                        }

                        /* Update dataFlow. */
                        gcmASSERT(code->dependencies0 == gcvNULL);
                        code->dependencies0 = code->dependencies1;
                        code->dependencies1 = gcvNULL;
                    }
                    else
                    {
                        if (format1 == gcSL_FLOAT)
                        {
                            float f = gcoMATH_UIntAsFloat(value1);
                            if (f != 0.0f) break;

                            /* It is equivalent to MOV instruction. */
                            code->instruction.opcode         = gcSL_MOV;
                            code->instruction.source1        = 0;
                            code->instruction.source1Index   = 0;
                            code->instruction.source1Indexed = 0;
                        }
                        else if (format1 == gcSL_INTEGER)
                        {
                            gctINT i = *(gctINT *) &value1;
                            if (i != 0) break;

                            /* It is equivalent to MOV instruction. */
                            code->instruction.opcode         = gcSL_MOV;
                            code->instruction.source1        = 0;
                            code->instruction.source1Index   = 0;
                            code->instruction.source1Indexed = 0;
                        }
                    }

                    /* The result is not constant. */
                    break;
                }
                else if (code->instruction.opcode == gcSL_MUL)
                {
                    if (gcmSL_SOURCE_GET(source, Type) == gcSL_CONSTANT)
                    {
                        if (format0 == gcSL_FLOAT)
                        {
                            float f = gcoMATH_UIntAsFloat(value0);
                            if (f == 0.0f)
                            {
                                /* The result is 0. */
                                sourceUint = code->instruction.source0;
                                value = gcoMATH_FloatAsUInt(f);
                                sourceIndex = value & 0xFFFF;
                                sourceIndexed = (value >> 16) & 0xFFFF;

                                needToPropagate = gcvTRUE;

                                /* Remove dependencies0. */
                                if (code->dependencies0)
                                {
                                    gcmVERIFY_OK(gcOpt_DestroyCodeDependency(Optimizer,
                                                        &code->dependencies0,
                                                        code));
                                }
                            }
                            else if (f == 1.0f)
                            {
                                /* It is equivalent to MOV instruction. */
                                code->instruction.opcode         = gcSL_MOV;
                                code->instruction.source0        = code->instruction.source1;
                                code->instruction.source0Index   = code->instruction.source1Index;
                                code->instruction.source0Indexed = code->instruction.source1Indexed;
                                code->instruction.source1        = 0;
                                code->instruction.source1Index   = 0;
                                code->instruction.source1Indexed = 0;

                                /* Update dataFlow. */
                                gcmASSERT(code->dependencies0 == gcvNULL);
                                code->dependencies0 = code->dependencies1;
                                code->dependencies1 = gcvNULL;
                            }
                        }
                        else if (format0 == gcSL_INTEGER)
                        {
                            gctINT i = *(gctINT *) &value0;
                            if (i == 0)
                            {
                                /* The result is 0. */
                                sourceUint = code->instruction.source0;
                                sourceIndex = value0 & 0xFFFF;
                                sourceIndexed = (value0 >> 16) & 0xFFFF;

                                needToPropagate = gcvTRUE;

                                /* Remove dependencies0. */
                                if (code->dependencies0)
                                {
                                    gcmVERIFY_OK(gcOpt_DestroyCodeDependency(Optimizer,
                                                        &code->dependencies0,
                                                        code));
                                }
                            }
                            else if (i == 1)
                            {
                                /* It is equivalent to MOV instruction. */
                                code->instruction.opcode         = gcSL_MOV;
                                code->instruction.source0        = code->instruction.source1;
                                code->instruction.source0Index   = code->instruction.source1Index;
                                code->instruction.source0Indexed = code->instruction.source1Indexed;
                                code->instruction.source1        = 0;
                                code->instruction.source1Index   = 0;
                                code->instruction.source1Indexed = 0;

                                /* Update dataFlow. */
                                gcmASSERT(code->dependencies0 == gcvNULL);
                                code->dependencies0 = code->dependencies1;
                                code->dependencies1 = gcvNULL;
                            }
                        }
                    }
                    else
                    {
                        if (format1 == gcSL_FLOAT)
                        {
                            float f = gcoMATH_UIntAsFloat(value1);
                            if (f == 0.0f)
                            {
                                /* The result is 0. */
                                sourceUint = code->instruction.source1;
                                value = gcoMATH_FloatAsUInt(f);
                                sourceIndex = value & 0xFFFF;
                                sourceIndexed = (value >> 16) & 0xFFFF;

                                needToPropagate = gcvTRUE;

                                /* Remove dependencies1. */
                                if (code->dependencies1)
                                {
                                    gcmVERIFY_OK(gcOpt_DestroyCodeDependency(Optimizer,
                                                        &code->dependencies1,
                                                        code));
                                }
                            }
                            else if (f == 1.0f)
                            {
                                /* It is equivalent to MOV instruction. */
                                code->instruction.opcode         = gcSL_MOV;
                                code->instruction.source1        = 0;
                                code->instruction.source1Index   = 0;
                                code->instruction.source1Indexed = 0;
                            }
                        }
                        else if (format1 == gcSL_INTEGER)
                        {
                            gctINT i = *(gctINT *) &value1;
                            if (i == 0)
                            {
                                /* The result is 0. */
                                sourceUint = code->instruction.source1;
                                sourceIndex = value1 & 0xFFFF;
                                sourceIndexed = (value1 >> 16) & 0xFFFF;

                                needToPropagate = gcvTRUE;

                                /* Remove dependencies1. */
                                if (code->dependencies1)
                                {
                                    gcmVERIFY_OK(gcOpt_DestroyCodeDependency(Optimizer,
                                                        &code->dependencies1,
                                                        code));
                                }
                            }
                            else if (i == 1)
                            {
                                /* It is equivalent to MOV instruction. */
                                code->instruction.opcode         = gcSL_MOV;
                                code->instruction.source1        = 0;
                                code->instruction.source1Index   = 0;
                                code->instruction.source1Indexed = 0;
                            }
                        }
                    }
                    break;
                }
                else if (code->instruction.opcode == gcSL_SUB)
                {
                    if (gcmSL_SOURCE_GET(source1, Type) != gcSL_CONSTANT) break;

                    if (format1 == gcSL_FLOAT)
                    {
                        float f = gcoMATH_UIntAsFloat(value1);
                        if (f != 0.0f) break;

                        /* It is equivalent to MOV instruction. */
                        code->instruction.opcode = gcSL_MOV;
                        code->instruction.source1 = 0;
                        code->instruction.source1Index = 0;
                        code->instruction.source1Indexed = 0;
                    }
                    else if (format1 == gcSL_INTEGER)
                    {
                        gctINT i = *(gctINT *) &value1;
                        if (i != 0) break;

                        /* It is equivalent to MOV instruction. */
                        code->instruction.opcode = gcSL_MOV;
                        code->instruction.source1 = 0;
                        code->instruction.source1Index = 0;
                        code->instruction.source1Indexed = 0;
                    }

                    /* The result is not constant. */
                    break;
                }
                else if (code->instruction.opcode == gcSL_POW)
                {
                    if (gcmSL_SOURCE_GET(source, Type) == gcSL_CONSTANT)
                    {
                        float f;
                        gcmASSERT(format0 == gcSL_FLOAT);
                        f = gcoMATH_UIntAsFloat(value0);
                        if (f != 0.0f && f != 1.0f) break;

                        /* The result is f. */
                        sourceUint = code->instruction.source0;
                        value = gcoMATH_FloatAsUInt(f);
                        sourceIndex = value & 0xFFFF;
                        sourceIndexed = (value >> 16) & 0xFFFF;

                        needToPropagate = gcvTRUE;

                        /* Remove dependencies1. */
                        if (code->dependencies1)
                        {
                            gcmVERIFY_OK(gcOpt_DestroyCodeDependency(Optimizer,
                                                &code->dependencies1,
                                                code));
                        }
                    }
                    else
                    {
                        float f;
                        gcmASSERT(format1 == gcSL_FLOAT);
                        f = gcoMATH_UIntAsFloat(value1);
                        if (f == 0.0f)
                        {
                            /* The result is 1. */
                            sourceUint = code->instruction.source1;
                            f = 1.0f;
                            value = gcoMATH_FloatAsUInt(f);
                            sourceIndex = value & 0xFFFF;
                            sourceIndexed = (value >> 16) & 0xFFFF;

                            needToPropagate = gcvTRUE;

                            /* Remove dependencies1. */
                            if (code->dependencies1)
                            {
                                gcmVERIFY_OK(gcOpt_DestroyCodeDependency(Optimizer,
                                                    &code->dependencies1,
                                                    code));
                            }
                        }
                        else if (f == 1.0f)
                        {
                            /* It is equivalent to MOV instruction. */
                            code->instruction.opcode         = gcSL_MOV;
                            code->instruction.source1        = 0;
                            code->instruction.source1Index   = 0;
                            code->instruction.source1Indexed = 0;
                        }
                    }
                    break;
                }
                else break;
            }

            gcmASSERT(code->dependencies0 == gcvNULL &&
                      code->dependencies1 == gcvNULL);

            if (format0 == gcSL_FLOAT || format1 == gcSL_FLOAT)
            {
                float f = 0.0f, f0 = 0.0f, f1;

                if (format0 == gcSL_FLOAT)
                {
                    f0 = gcoMATH_UIntAsFloat(value0);
                    sourceUint = code->instruction.source0;
                }
                else if (format0 == gcSL_INTEGER)
                {
                    f0 = (float) value0;
                }
                else
                {
                    /* TODO - error. */
                }

                if (format1 == gcSL_FLOAT)
                {
                    f1 = gcoMATH_UIntAsFloat(value1);
                    sourceUint = code->instruction.source1;
                }
                else if (format1 == gcSL_INTEGER)
                {
                    f1 = (float) value1;
                }
                else
                {
                    /* Error. */
                    gcmFOOTER();
                    return gcvSTATUS_INVALID_DATA;
                }

                switch (code->instruction.opcode)
                {
                case gcSL_ADD:
                    f = gcoMATH_Add(f0, f1); break;
                case gcSL_SUB:
                    f = gcoMATH_Add(f0, -f1); break;
                case gcSL_MUL:
                    f = gcoMATH_Multiply(f0, f1); break;
                case gcSL_MAX:
                    f = f0 > f1 ? f0 : f1; break;
                case gcSL_MIN:
                    f = f0 < f1 ? f0 : f1; break;
                case gcSL_POW:
                    f = gcoMATH_Power(f0, f1); break;
                }
                value = gcoMATH_FloatAsUInt(f);
                sourceIndex = value & 0xFFFF;
                sourceIndexed = (value >> 16) & 0xFFFF;

                needToPropagate = gcvTRUE;
            }
            else if (format0 == gcSL_INTEGER || format1 == gcSL_INTEGER)
            {
                if (format0 == gcSL_INTEGER)
                {
                    sourceUint = code->instruction.source0;
                }
                else if (format1 == gcSL_INTEGER)
                {
                    sourceUint = code->instruction.source1;
                }

                switch (code->instruction.opcode)
                {
                case gcSL_ADD:
                    value = value0 + value1; break;
                case gcSL_SUB:
                    value = value0 - value1; break;
                case gcSL_MUL:
                    value = value0 * value1; break;
                case gcSL_MAX:
                    value = value0 > value1 ? value0 : value1; break;
                case gcSL_MIN:
                    value = value0 < value1 ? value0 : value1; break;
                case gcSL_POW:
                    value = gcoMATH_Float2Int(
                                gcoMATH_Power(gcoMATH_Int2Float(value0),
                                              gcoMATH_Int2Float(value1)));
                    break;
                }
                sourceIndex = value & 0xFFFF;
                sourceIndexed = (value >> 16) & 0xFFFF;

                needToPropagate = gcvTRUE;
            }
            else
            {
                if (format0 > gcSL_FLOAT128 || format1 > gcSL_FLOAT128)
                {
                    /* Error. */
                    gcmFOOTER();
                    return gcvSTATUS_INVALID_DATA;
                }
                else
                {
                    /* TODO: Need to support more data type for OCL ... */
                }
            }

            break;


        default:
            break;
        }

        if (!needToPropagate) continue;

        if (code->instruction.opcode != gcSL_MOV)
        {
            /* Convert the instruction to MOV instruction. */
            code->instruction.opcode = gcSL_MOV;
            code->instruction.source0 = sourceUint;
            code->instruction.source0Index = sourceIndex;
            code->instruction.source0Indexed = sourceIndexed;
            code->instruction.source1 = 0;
            code->instruction.source1Index = 0;
            code->instruction.source1Indexed = 0;
        }

        for (list = code->users; list; list = nextList)
        {
            gcOPT_CODE codeUser;
            gcOPT_LIST depList;
            gctBOOL cannotReplace = gcvFALSE;

            nextList = list->next;

            /* Skip output. */
            if (list->index < 0) continue;

            codeUser = list->code;
            switch (codeUser->instruction.opcode)
            {
            case gcSL_CALL:
                /* No inter-procedural propagation. */
            case gcSL_TEXBIAS:
            case gcSL_TEXGRAD:
            case gcSL_TEXLOD:
                /* Skip texture state instructions. */
            case gcSL_TEXLD:
            case gcSL_TEXLDP:
                /* Skip texture sample instructions. */

            case gcSL_STORE1:
                /* Skip store1 because it may cause two constants in an instruction. */

                cannotReplace = gcvTRUE;
            }

            if (cannotReplace) continue;

            depList = codeUser->dependencies0;
            if (depList && depList->code == code && depList->next == gcvNULL &&
                gcmSL_SOURCE_GET(codeUser->instruction.source0, Type) == gcSL_TEMP)
            {
                /* Change temp usage to constant. */
                codeUser->instruction.source0 = sourceUint;
                codeUser->instruction.source0Index = sourceIndex;
                codeUser->instruction.source0Indexed = sourceIndexed;

                /* Remove dependency and user. */
                gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &code->users, codeUser));
                gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &codeUser->dependencies0, code));

                constPropagated++;
            }

            depList = codeUser->dependencies1;
            if (depList && depList->code == code && depList->next == gcvNULL &&
                gcmSL_SOURCE_GET(codeUser->instruction.source1, Type) == gcSL_TEMP)
            {
                /* Change temp usage to constant. */
                codeUser->instruction.source1 = sourceUint;
                codeUser->instruction.source1Index = sourceIndex;
                codeUser->instruction.source1Indexed = sourceIndexed;

                /* Remove dependency and user. */
                /* source1 may be the same as source0, so do not verify it. */
                gcOpt_DeleteCodeFromList(Optimizer, &code->users, codeUser);
                gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &codeUser->dependencies1, code));

                constPropagated++;
            }
        }
    }
    if (constPropagated > 0)
    {
        status = gcvSTATUS_CHANGED;
    }

    if (status == gcvSTATUS_CHANGED)
    {
        DUMP_OPTIMIZER("Propagated constants in the shader", Optimizer);
        gcmFOOTER();
        return status;
    }

    gcmFOOTER();
    return status;
}

static gctBOOL
_EvaluateChecking(
    IN gcSL_INSTRUCTION Code,
    OUT gctBOOL * CheckingResult
    )
{
    gctUINT16 source;
    gctUINT16 target;
    gcSL_FORMAT format0, format1;
    gctUINT32 value0, value1;

    source = Code->source0;
    if (gcmSL_SOURCE_GET(source, Type) != gcSL_CONSTANT) return gcvFALSE;

    format0 = (gcSL_FORMAT) gcmSL_SOURCE_GET(source, Format);

    source = Code->source1;
    if (gcmSL_SOURCE_GET(source, Type) != gcSL_CONSTANT) return gcvFALSE;

    format1 = (gcSL_FORMAT) gcmSL_SOURCE_GET(source, Format);

    /* Assemble the 32-bit values. */
    value0 = (Code->source0Index & 0xFFFF) | (Code->source0Indexed << 16);
    value1 = (Code->source1Index & 0xFFFF) | (Code->source1Indexed << 16);

    if (format0 == gcSL_FLOAT || format1 == gcSL_FLOAT)
    {
        float f0, f1;

        if (format0 == gcSL_FLOAT)
        {
            f0 = gcoMATH_UIntAsFloat(value0);
        }
        else if (format0 == gcSL_INTEGER)
        {
            f0 = gcoMATH_UInt2Float(value0);
        }
        else
        {
            /* Error. */
            return gcvFALSE;
        }

        if (format1 == gcSL_FLOAT)
        {
            f1 = gcoMATH_UIntAsFloat(value1);
        }
        else if (format1 == gcSL_INTEGER)
        {
            f1 = gcoMATH_UInt2Float(value1);
        }
        else
        {
            /* Error. */
            return gcvFALSE;
        }

        target = Code->temp;

        switch (gcmSL_TARGET_GET(target, Condition))
        {
        case gcSL_ALWAYS:
            /* Error. */ return gcvFALSE;
        case gcSL_NOT_EQUAL:
            *CheckingResult = (f0 != f1); break;
        case gcSL_LESS_OR_EQUAL:
            *CheckingResult = (f0 <= f1); break;
        case gcSL_LESS:
            *CheckingResult = (f0 < f1); break;
        case gcSL_EQUAL:
            *CheckingResult = (f0 == f1); break;
        case gcSL_GREATER:
            *CheckingResult = (f0 > f1); break;
        case gcSL_GREATER_OR_EQUAL:
            *CheckingResult = (f0 >= f1); break;
        case gcSL_AND:
        case gcSL_OR:
        case gcSL_XOR:
            /* TODO - Error. */
            return gcvFALSE;
        case gcSL_NOT_ZERO:
            *CheckingResult = (f0 != 0.0f); break;
        }
    }
    else
    {
        target = Code->temp;

        switch (gcmSL_TARGET_GET(target, Condition))
        {
        case gcSL_ALWAYS:
            /* Error. */ return gcvFALSE;
        case gcSL_NOT_EQUAL:
            *CheckingResult = (value0 != value1); break;
        case gcSL_LESS_OR_EQUAL:
            *CheckingResult = (value0 <= value1); break;
        case gcSL_LESS:
            *CheckingResult = (value0 < value1); break;
        case gcSL_EQUAL:
            *CheckingResult = (value0 == value1); break;
        case gcSL_GREATER:
            *CheckingResult = (value0 > value1); break;
        case gcSL_GREATER_OR_EQUAL:
            *CheckingResult = (value0 >= value1); break;
        case gcSL_AND:
            *CheckingResult = (value0 & value1); break;
        case gcSL_OR:
            *CheckingResult = (value0 | value1); break;
        case gcSL_XOR:
            *CheckingResult = (value0 ^ value1); break;
        case gcSL_NOT_ZERO:
            *CheckingResult = (value0 != 0); break;
        }
    }

    return gcvTRUE;
}

/*******************************************************************************
**                          gcOpt_RemoveRedundantCheckings
********************************************************************************
**
**  Remove redundant checkings.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_RemoveRedundantCheckings(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gcOPT_CODE code;
    gctBOOL checkingResult = gcvFALSE;
    gctUINT changeCount = 0;
    gctUINT i;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    for (code = Optimizer->codeHead; code; code = code->next)
    {
        if (code->instruction.opcode == gcSL_JMP)
        {
            if (gcmSL_TARGET_GET(code->instruction.temp, Condition) != gcSL_ALWAYS)
            {
                if (_EvaluateChecking(&code->instruction, &checkingResult))
                {
                    changeCount++;
                    if (checkingResult)
                    {
                        /* Change the conditional jump to unconditional jump. */
                        code->instruction.temp = gcmSL_TARGET_SET(code->instruction.temp, Condition, gcSL_ALWAYS);

                        /* Remove sources. */
                        code->instruction.source0 = code->instruction.source0Index = code->instruction.source0Indexed = 0;
                        code->instruction.source1 = code->instruction.source1Index = code->instruction.source1Indexed = 0;
                    }
                    else
                    {
                        /* Change the checking/jump instruction to NOP. */
                        gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, code));
                    }
                }
            }
        }
    }

    if (changeCount)
    {
        _RemoveFunctionUnreachableCode(Optimizer, Optimizer->main);
        for (i = 0; i < Optimizer->functionCount; i++)
        {
            _RemoveFunctionUnreachableCode(Optimizer, Optimizer->functionArray + i);
        }
    }

    if (changeCount)
    {
        /* Rebuild flow graph. */
    	gcmONERROR(gcOpt_RebuildFlowGraph(Optimizer));

        DUMP_OPTIMIZER("Removed redundant checkings from the shader", Optimizer);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_CHANGED);
        return gcvSTATUS_CHANGED;
    }
    else
    {
        /* Code unchanged. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                          gcOpt_MergeVectorInstructions
********************************************************************************
**
**  Merge separate vector instructions into one vector instruction.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_MergeVectorInstructions(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT codeRemoved = 0;
    gcOPT_TEMP tempArray = Optimizer->tempArray;
	gcOPT_TEMP temp;
    gcOPT_CODE code, endCode, iterCode;
    gcOPT_LIST list, list1;
    gctUINT16 enable, enable1;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    gcOpt_UpdateCodeId(Optimizer);

    /* Phase I: merge MOV instructions for the same vector. */
    for (code = Optimizer->codeTail; code; code = code->prev)
    {
        /* Test for MOV instruction. */
        if (code->instruction.opcode != gcSL_MOV) continue;

        if (! code->users) continue;

		/* Get pointer to temporary register. */
		temp = tempArray + code->instruction.tempIndex;

        enable = gcmSL_TARGET_GET(code->instruction.temp, Enable);

        /* Skip if all used components are enabled. */
        if (enable == temp->usage) continue;

        /* Check if users have the multiple dependencies that set different components. */
        endCode = Optimizer->codeTail;
        for (list = code->users; list; list = list->next)
        {
            if (list->code && list->code->id < endCode->id)
            {
                endCode = list->code;
            }
        }

        for (list = code->nextDefines; list; list = list->next)
        {
            if (list->code && list->code->id < endCode->id)
            {
                endCode = list->code;
            }
        }

        /* Find candidates that can be merged. */
        for (iterCode = code->next; iterCode != endCode; iterCode = iterCode->next)
        {
            /* Stop if any jump in/out. */
            if (iterCode->callers) break;
            if (iterCode->instruction.opcode == gcSL_JMP ||
                iterCode->instruction.opcode == gcSL_CALL) break;

            /* Check if iterCode is a candidate. */
            if (iterCode->instruction.opcode != gcSL_MOV) continue;
            if (iterCode->instruction.tempIndex != code->instruction.tempIndex) continue;
            if (iterCode->instruction.source0Index != code->instruction.source0Index) continue;
            if (gcmSL_SOURCE_GET(code->instruction.source0, Type) !=
                gcmSL_SOURCE_GET(iterCode->instruction.source0, Type)) continue;
            if (gcmSL_TARGET_GET(code->instruction.temp, Indexed) !=
                gcmSL_TARGET_GET(iterCode->instruction.temp, Indexed)) continue;
            if (iterCode->instruction.tempIndexed != code->instruction.tempIndexed) continue;
            if (gcmSL_SOURCE_GET(code->instruction.source0, Indexed) !=
                gcmSL_SOURCE_GET(iterCode->instruction.source0, Indexed)) continue;
            if (iterCode->instruction.source0Indexed != code->instruction.source0Indexed) continue;

            /* Check if the dest has been changed after code. */
            for (list = iterCode->prevDefines; list; list = list->next)
            {
                if (list->code && list->code->id >= code->id) break;

                /* Check if the dest is used after code. */
                for (list1 = list->code->users; list1; list1 = list1->next)
                {
                    if (list1->code && list1->code->id > code->id) break;
                }
            }
            if (list) continue;

            /* Check if the source is redefined after code */
            for (list = iterCode->dependencies0; list; list = list->next)
            {
                if (list->code && list->code->id > code->id) break;
            }
            if (list) continue;

            enable1 = gcmSL_TARGET_GET(iterCode->instruction.temp, Enable);
            gcmASSERT((enable & enable1) == 0);

            /* Merge iterCode into code. */
            enable |= enable1;
            code->instruction.temp = gcmSL_TARGET_SET(code->instruction.temp, Enable, enable);

            if (enable1 & gcSL_ENABLE_X)
            {
                code->instruction.source0 = gcmSL_SOURCE_SET(code->instruction.source0,
                                                             SwizzleX,
                                                             gcmSL_SOURCE_GET(iterCode->instruction.source0, SwizzleX));
            }
            if (enable1 & gcSL_ENABLE_Y)
            {
                code->instruction.source0 = gcmSL_SOURCE_SET(code->instruction.source0,
                                                             SwizzleY,
                                                             gcmSL_SOURCE_GET(iterCode->instruction.source0, SwizzleY));
            }
            if (enable1 & gcSL_ENABLE_Z)
            {
                code->instruction.source0 = gcmSL_SOURCE_SET(code->instruction.source0,
                                                             SwizzleZ,
                                                             gcmSL_SOURCE_GET(iterCode->instruction.source0, SwizzleZ));
            }
            if (enable1 & gcSL_ENABLE_W)
            {
                code->instruction.source0 = gcmSL_SOURCE_SET(code->instruction.source0,
                                                             SwizzleW,
                                                             gcmSL_SOURCE_GET(iterCode->instruction.source0, SwizzleW));
            }

            /* Update data flow. */
            if (iterCode->users)
            {
                for (list = iterCode->users; list; list = list->next)
                {
                    gcOPT_CODE userCode;

                    if (list->index < 0) continue;   /* Skip output and global dependency. */

                    userCode = list->code;
                    gcOpt_ReplaceCodeInList(Optimizer, &userCode->dependencies0, iterCode, code);
                    gcOpt_ReplaceCodeInList(Optimizer, &userCode->dependencies1, iterCode, code);
                }
                gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, iterCode->users, &code->users));
                gcmVERIFY_OK(gcOpt_FreeList(Optimizer, &iterCode->users));
            }

            if (iterCode->dependencies0)
            {
                for (list = iterCode->dependencies0; list; list = list->next)
                {
                    if (list->index < 0) continue;   /* Skip output and global dependency. */

                    gcOpt_ReplaceCodeInList(Optimizer, &list->code->users, iterCode, code);
                }

                gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, iterCode->dependencies0, &code->dependencies0));
                gcmVERIFY_OK(gcOpt_FreeList(Optimizer, &iterCode->dependencies0));
            }

            if (iterCode->nextDefines)
            {
                gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, iterCode->nextDefines, &code->nextDefines));
                gcmVERIFY_OK(gcOpt_FreeList(Optimizer, &iterCode->nextDefines));
            }

            if (iterCode->prevDefines)
            {
                gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, iterCode->prevDefines, &code->prevDefines));
                gcmVERIFY_OK(gcOpt_FreeList(Optimizer, &iterCode->prevDefines));
            }

            gcmVERIFY_OK(gcOpt_ChangeCodeToNOP(Optimizer, iterCode));
            /*iterCode->instruction = gcvSL_NOP_INSTR;*/
            codeRemoved++;
        }
    }

    if (codeRemoved != 0)
    {
        /* Rebuild flow graph. */
    	gcmONERROR(gcOpt_RebuildFlowGraph(Optimizer));

        DUMP_OPTIMIZER("Merged vector instructions in the shader", Optimizer);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_CHANGED);
        return gcvSTATUS_CHANGED;
    }
    else
    {
        /* Code unchanged. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                          gcOpt_LoadSWWorkaround
********************************************************************************
**
**  Special optimization for LOAD SW workaround.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_LoadSWWorkaround(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT codeMoved = 0;
    gcOPT_TEMP tempArray = Optimizer->tempArray;
	gcOPT_TEMP temp;
    gcOPT_CODE code, nextLoadCode, endCode, lastUserCode, iterCode;
    gcOPT_LIST list, list1;
    gctUINT nextLoadCodeId, id;
	gctINT * loadUsers = gcvNULL;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    if (Optimizer->shader->loadUsers)
    {
		/* Free the old load users. */
		gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Optimizer->shader->loadUsers));
    }

    gcOpt_UpdateCodeId(Optimizer);

    /* Move users before next LOAD instruction if possible. */
    for (code = Optimizer->codeHead; code; code = nextLoadCode)
    {
        nextLoadCode = code->next;

        /* Test for MOV instruction. */
        if (code->instruction.opcode != gcSL_LOAD) continue;

        /* Skip if no users. */
        if (! code->users) continue;

        /* Skip if indexed. */
        if (gcmSL_TARGET_GET(code->instruction.temp, Indexed) != gcSL_NOT_INDEXED) continue;

		/* Get pointer to temporary register. */
		temp = tempArray + code->instruction.tempIndex;

        /* Skip if target is part of an array. */
        if (temp->arrayVariable && temp->arrayVariable->arraySize > 1) continue;

        /* Skip if not all used components are enabled. */
        if (gcmSL_TARGET_GET(code->instruction.temp, Enable) != temp->usage) continue;

        /* Find next LOAD instruction. */
        for (nextLoadCode = code->next; nextLoadCode; nextLoadCode = nextLoadCode->next)
        {
            if (nextLoadCode->instruction.opcode == gcSL_LOAD) break;
        }
        if (nextLoadCode == gcvNULL) break;

        /* Check if all users can be moved before next LOAD instruction. */
        nextLoadCodeId = nextLoadCode->id;
        lastUserCode = code;
        for (list = code->users; list; list = list->next)
        {
            /* Skip output and global registers. */
            if (list->index < 0) break;

            /* Stop if any jump in/out. */
            iterCode = list->code;
            if (iterCode->callers) break;
            if (iterCode->instruction.opcode == gcSL_JMP ||
                iterCode->instruction.opcode == gcSL_CALL) break;

            /* Skip MULSAT and ADDSAT if they are together. */
            if (iterCode->instruction.opcode == gcSL_MULSAT)
            {
                if (iterCode->next &&
                    (iterCode->next->instruction.opcode == gcSL_ADDSAT ||
                     iterCode->next->instruction.opcode == gcSL_SUBSAT)) break;
            }
            else if (iterCode->instruction.opcode == gcSL_ADDSAT ||
                     iterCode->instruction.opcode == gcSL_SUBSAT)
            {
                if (iterCode->prev &&
                    iterCode->prev->instruction.opcode == gcSL_MULSAT) break;
            }

            /* If the user is gcSL_LOAD, it can be only the nextLoadCode. */
            /* Stop if gcSL_LOAD for now. */
            if (iterCode->instruction.opcode == gcSL_LOAD /*&&
                iterCode != nextLoadCode*/) break;

            if (iterCode->id > lastUserCode->id)
            {
                lastUserCode = list->code;
            }

            /* Skip if users have prevDefines after nextLoadCode. */
            for (list1 = iterCode->prevDefines; list1; list1 = list1->next)
            {
                if (list1->code && list1->code->id >= nextLoadCodeId) break;
            }
            if (list1) break;

            /* Skip if users have other dependencies after nextLoadCode. */
            for (list1 = iterCode->dependencies0; list1; list1 = list1->next)
            {
                if (list1->code && list1->code->id >= nextLoadCodeId) break;
            }
            if (list1) break;
            for (list1 = iterCode->dependencies1; list1; list1 = list1->next)
            {
                if (list1->code && list1->code->id >= nextLoadCodeId) break;
            }
            if (list1) break;
        }
        if (list) continue;

        /* Check if there is any jump in/out between code and lastUserCode. */
        endCode = lastUserCode->next;
        for (iterCode = code->next; iterCode != endCode; iterCode = iterCode->next)
        {
            /* Stop if any jump in/out. */
            if (iterCode->callers) break;
            if (iterCode->instruction.opcode == gcSL_JMP ||
                iterCode->instruction.opcode == gcSL_CALL) break;
        }
        if (iterCode != endCode) continue;

        /* Move users before nextLoadCode. */
        for (list = code->users; list; list = list->next)
        {
            iterCode = list->code;
            if (iterCode->id > nextLoadCodeId)
            {
                gcOpt_MoveCodeListBefore(Optimizer, iterCode, iterCode, nextLoadCode);
                lastUserCode = iterCode;
            }
        }

        /* Update code id because the checkings depend on it. */
        /*gcOpt_UpdateCodeId(Optimizer);*/
        for (id = code->id + 1, iterCode = code->next;
             iterCode != endCode;
             id++, iterCode = iterCode->next)
        {
            if (iterCode->id != id)
            {
                iterCode->id = id;
            }
        }
        codeMoved++;

        if (Optimizer->shader->loadUsers == gcvNULL)
        {
            gctSIZE_T codeCount = Optimizer->codeTail->id + 1;
    	    gctPOINTER pointer = gcvNULL;
            gctSIZE_T bytes;
            gctUINT i;

		    /* Allocate new loadUsers. */
            bytes = (gctSIZE_T)(codeCount * gcmSIZEOF(gctINT));
		    gcmONERROR(gcoOS_Allocate(gcvNULL, bytes, &pointer));

		    loadUsers = Optimizer->shader->loadUsers = pointer;

            /* Initialize all entries to -1. */
            for (i = 0; i < codeCount; i++)
            {
                loadUsers[i] = -1;
            }
        }

        /* Save last user info in loadUsers for SW workaround in BE. */
        loadUsers[code->id] = lastUserCode->id;
    }

    if (codeMoved != 0)
    {
        DUMP_OPTIMIZER("Moved user instructions for LOAD the shader", Optimizer);
        gcmFOOTER_ARG("status=%d", gcvSTATUS_CHANGED);
        return gcvSTATUS_CHANGED;
    }
    else
    {
        /* Code unchanged. */
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                          gcOpt_OptimizeBranches
********************************************************************************
**
**  Optimize branches.
**
**  IN:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_OptimizeBranches(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    /* Optimize adjacent JMPs. */
    /* TODO */

    DUMP_OPTIMIZER("Optimized branches in the shader", Optimizer);
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                          gcOpt_RemoveCommonExpression
********************************************************************************
**
**  Remove common sub-expressions.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_RemoveCommonExpression(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    /* identify common sub-expressions and remove duplicate copies. */
    /* TODO */

    DUMP_OPTIMIZER("Removed common sub-expressions from the shader", Optimizer);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                          gcOpt_MoveLoopInvariants
********************************************************************************
**
**  Move loop invariants out of the loops.
**
**  INPUT:
**
**      gcOPTIMIZER Optimizer
**          Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_MoveLoopInvariants(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

    /* identify common sub-expressions and move them out of loops. */
    /* TODO */

    DUMP_OPTIMIZER("Moved loop invariants out of loops in the shader", Optimizer);

    gcmFOOTER();
    return status;
}

/*******************************************************************************
**                              gcOptimizeShader
********************************************************************************
**
**  Optimize a shader.
**
**  INPUT:
**
**      gcSHADER Shader
**          Pointer to a gcSHADER object holding information about the compiled
**          shader.
**
**      gctFILE LogFile
**          Pointer to an open FILE object.
*/
gceSTATUS
gcOptimizeShader(
    IN gcSHADER Shader,
    IN gctFILE LogFile
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    gceSHADER_OPTIMIZATION option;
    gcOPTIMIZER optimizer = gcvNULL;

    gcmHEADER_ARG("Shader=0x%x LogFile=0x%x", Shader, LogFile);

    /* Verify the arguments. */
    gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);

    /* Sanity check. */
    if (Shader->codeCount == 0)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

#if !__SUPPORT_OCL__
    if (Shader->type == gcSHADER_TYPE_CL)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }
#endif

    option = (gceSHADER_OPTIMIZATION) Shader->optimizationOption;

    DUMP_SHADER(LogFile, "Incoming shader", Shader);
    /*gcOpt_GenShader(Shader, LogFile);*/

    if (option == gcvOPTIMIZATION_NONE)
    {
        gcmFOOTER_NO();
        return gcvSTATUS_OK;
    }

    DUMP_MESSAGE(gcvNULL, LogFile, "gcOptimizeShader: start\n");

    do
    {
        /* Construct the optimizer. */
        gcmERR_BREAK(gcOpt_ConstructOptimizer(Shader, &optimizer));

        optimizer->logFile = LogFile;

        DUMP_OPTIMIZER("After preprocessing", optimizer);

        if (option != gcvOPTIMIZATION_NONE &&
            !gcdHasOptimization(option, gcvOPTIMIZATION_UNIT_TEST))
        {
            /* The normal full optimization mode. */
            status = gcvSTATUS_OK;

            /* TODO - need to add more features and adjust the sequence. */

            if (optimizer->functionCount > 0)
            {
                gctBOOL changed = gcvFALSE;

                if (gcdHasOptimization(option, gcvOPTIMIZATION_DEAD_CODE))
                {
                    /* Remove dead code. */
                    gcmERR_BREAK(gcOpt_RemoveDeadCode(optimizer));
                    if (status == gcvSTATUS_CHANGED) changed = gcvTRUE;
                }

                if (gcdHasOptimization(option, gcvOPTIMIZATION_REDUNDANT_MOVE))
                {
                    /* Optimize MOV instructions for arguments and outputs. */
                    gcmERR_BREAK(gcOpt_OptimizeMOVInstructions(optimizer));
                    if (status == gcvSTATUS_CHANGED) changed = gcvTRUE;
                }

                if (gcdHasOptimization(option, gcvOPTIMIZATION_CONSTANT_PROPAGATION))
                {
                    /* Propagate constants. */
                    gcmERR_BREAK(gcOpt_PropagateConstants(optimizer));

                    if (status == gcvSTATUS_CHANGED)
                    {
                        changed = gcvTRUE;

                        /* Remove dead code. */
                        gcmERR_BREAK(gcOpt_RemoveDeadCode(optimizer));

                        if (gcdHasOptimization(option, gcvOPTIMIZATION_REDUNDANT_CHECKING))
                        {
                            /* Remove redundant checking. */
                            gcmERR_BREAK(gcOpt_RemoveRedundantCheckings(optimizer));
                        }
                    }
                }

                if (gcdHasOptimization(option, gcvOPTIMIZATION_DEAD_CODE))
                {
                    /* Remove dead code. */
                    gcmERR_BREAK(gcOpt_RemoveDeadCode(optimizer));
                    if (status == gcvSTATUS_CHANGED) changed = gcvTRUE;
                }

                if (changed)
                {
                    /* Get accurate function codeCount after optimizaton. */
                    gcmERR_BREAK(gcOpt_RemoveNOP(optimizer));
                }

                if (gcdHasOptimization(option, gcvOPTIMIZATION_INLINE_EXPANSION))
                {
                    /* Expand functions used only once or short but with many
                    ** arguments. */
                    gcmERR_BREAK(gcOpt_ExpandFunctions(optimizer));

                    if (status == gcvSTATUS_CHANGED)
                    {
                        /* Remove dead code. */
                        gcmERR_BREAK(gcOpt_RemoveDeadCode(optimizer));

                        /* Optimize MOV instructions for arguments and outputs. */
                        gcOpt_OptimizeMOVInstructions(optimizer);
                    }
                }
            }

            do
            {
                if (gcdHasOptimization(option, gcvOPTIMIZATION_DEAD_CODE))
                {
                    /* Remove dead code. */
                    gcmERR_BREAK(gcOpt_RemoveDeadCode(optimizer));
                }

                if (gcdHasOptimization(option, gcvOPTIMIZATION_REDUNDANT_MOVE))
                {
                    /* Optimize MOV instructions. */
                    gcmERR_BREAK(gcOpt_OptimizeMOVInstructions(optimizer));
                }

                if (gcdHasOptimization(option, gcvOPTIMIZATION_CONSTANT_PROPAGATION))
                {
                    /* Propagate constants. */
                    gcmERR_BREAK(gcOpt_PropagateConstants(optimizer));
                }

                if (status == gcvSTATUS_CHANGED)
                {
                    /* Remove dead code. */
                    gcmERR_BREAK(gcOpt_RemoveDeadCode(optimizer));

                    if (gcdHasOptimization(option, gcvOPTIMIZATION_REDUNDANT_CHECKING))
                    {
                        /* Remove redundant checking. */
                        gcmERR_BREAK(gcOpt_RemoveRedundantCheckings(optimizer));
                    }
                }
            }
            while (status == gcvSTATUS_CHANGED);

            gcmERR_BREAK(status);

            if (gcdHasOptimization(option, gcvOPTIMIZATION_REDUNDANT_MOVE))
            {
                /* Optimize MOV instructions. */
                gcmERR_BREAK(gcOpt_OptimizeMOVInstructions(optimizer));
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_DEAD_CODE))
            {
                /* Remove dead code. */
                gcmERR_BREAK(gcOpt_RemoveDeadCode(optimizer));
            }

            /* Remove NOP instructions from the shader. */
            gcmERR_BREAK(gcOpt_RemoveNOP(optimizer));

            if (gcdHasOptimization(option, gcvOPTIMIZATION_VECTOR_INSTRUCTION_MERGE))
            {
                /* Merge separate vector instructions into one vector instruction. */
                gcmERR_BREAK(gcOpt_MergeVectorInstructions(optimizer));
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_MAD_INSTRUCTION))
            {
                /* Optimize MUL and ADD for MADD instructions. */
                gcmERR_BREAK(gcOpt_OptimizeMULADDInstructions(optimizer));
            }

#if GC_ENABLE_LOADTIME_OPT
            if (gcdHasOptimization(option, gcvOPTIMIZATION_LOADTIME_CONSTANT))
            {
                gcmERR_BREAK(gcOPT_OptimizeLoadtimeConstant(optimizer));
                if (status == gcvSTATUS_CHANGED)
                {
                    /* Optimize MOV instructions. */
                    gcmERR_BREAK(gcOpt_OptimizeMOVInstructions(optimizer));

                    /* Remove dead code. */
                    gcmERR_BREAK(gcOpt_RemoveDeadCode(optimizer));
                }
            }
#endif  /* GC_ENABLE_LOADTIME_OPT */

            /* This has to be the last step because it is for BE SW workaround. */
            if (gcdHasOptimization(option, gcvOPTIMIZATION_LOAD_SW_WORKAROUND))
            {
                /* Remove NOP instructions from the shader. */
                gcmERR_BREAK(gcOpt_RemoveNOP(optimizer));

                /* Move users in front of next LOAD for LOAD SW workaround. */
                gcmERR_BREAK(gcOpt_LoadSWWorkaround(optimizer));
            }
        }
        else if (gcdHasOptimization(option, gcvOPTIMIZATION_UNIT_TEST))
        {
            /* Unit test for specified optimization feature. */
            if (gcdHasOptimization(option, gcvOPTIMIZATION_CONSTRUCTION))
            {
                status = gcvSTATUS_OK;
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_DEAD_CODE))
            {
                status = gcOpt_RemoveDeadCode(optimizer);
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_REDUNDANT_MOVE))
            {
                status = gcOpt_OptimizeMOVInstructions(optimizer);
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_INLINE_EXPANSION))
            {
                status = gcOpt_ExpandFunctions(optimizer);
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_CONSTANT_PROPAGATION))
            {
                status = gcOpt_PropagateConstants(optimizer);
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_REDUNDANT_CHECKING))
            {
                status = gcOpt_PropagateConstants(optimizer);
                status = gcOpt_RemoveRedundantCheckings(optimizer);
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_MAD_INSTRUCTION))
            {
                /* Optimize MUL and ADD for MADD instructions. */
                gcmERR_BREAK(gcOpt_OptimizeMULADDInstructions(optimizer));
            }

            if (gcdHasOptimization(option, gcvOPTIMIZATION_LOAD_SW_WORKAROUND))
            {
                /* Merge separate vector instructions into one vector instruction. */
                gcmERR_BREAK(gcOpt_LoadSWWorkaround(optimizer));
            }
        }

        /* Remove NOP instructions from the shader. */
        gcmERR_BREAK(gcOpt_RemoveNOP(optimizer));

        gcmERR_BREAK(gcOpt_CopyOutShader(optimizer, Shader));
    }
    while (gcvFALSE);

    if (optimizer != gcvNULL)
    {
        /* Free optimizer. */
        gcmVERIFY_OK(gcOpt_DestroyOptimizer(optimizer));
        optimizer = gcvNULL;
    }

    DUMP_SHADER(LogFile, "Final shader.", Shader);
    DUMP_MESSAGE(gcvNULL, LogFile, "gcOptimizeShader: end\n");
    /*gcOpt_GenShader(Shader, LogFile);*/

    /* Return the status. */
    gcmFOOTER();
    return status;
}
#endif /* VIVANTE_NO_3D */

