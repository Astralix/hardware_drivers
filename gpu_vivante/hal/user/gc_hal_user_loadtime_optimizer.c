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





#include "gc_hal_user_precomp.h"

#if GC_ENABLE_LOADTIME_OPT
#include "gc_hal_user_loadtime_optimizer.h"

#ifndef VIVANTE_NO_3D


#define _GC_OBJ_ZONE    gcvZONE_COMPILER
/*
**  Load Time Optimizer module.
**   load time optimizer is divided in two parts, compiler-time optimization
**   to find the load-time constants and do symbolic constant propagation and
**   folding, the purpose is to hoist the constant calculation out of the
**   shader code, put it into shader's data structure so it can be folded to
**   real constant at load time with minimum cost. The load time part maps the
**   constants specified by application to symbols in the shader saved symbolic
**   expression, after evaluating the expression, the constant value of the
**   expression is mapped to compiler genereated fake uniform.
**
**   the shader code may have branches usings the load time constant, in that
**   case we can two alternatives:
**     1. if there is only one or two branches using the load time constants,
**        we can multiversion the shader code to upto four versions and load
**        the correct version at load time depends on the load time value
**     2. if the multiversion is infeasible, and the benefit to remove the
**        branch is greater than the cost of load-time optimization, then the
**        shader code is marked as load-time otpimizable, at the load-time,
**        a light weight optimizer is invoked to optimize the code. The light
**        weight optimizer works on the machine code, currently only constant
**        propagation and dead code elimination are pplied to the code
**
**   The shader code may load texture using load-time constant, we don't know
**   the impact if the texture load is optimized as load time constant, since
**   the texure load is very complicated, cannot be easily modelled by CPU.
**   If we really want to optimize it, there two options:
**     1. model it with CPU code, basically using the c-model texture module
**        to get the texture value at load time
**     2. compose a small kernel code to have the GPU load the the texture and
**        get the return value from the kernel and use it for load-time constant
**   Both methods will have big overhead, and the benefit is unclear, because
**   the constant texture load may not have big latency as one expected, due to
**   the fact that the frequently accessed data may stay in the cache.
**
**
*/

gcsAllocator ltcAllocator = {gcoOS_Allocate, gcoOS_Free };

/* treat the Data and Key as temp register index with components
 * encoded in the value, check if the Index part are equal */
gctBOOL
CompareIndex (
     IN void *    Data,
     IN void *    Key
     )
{
    gctINT regIndex1 = ltcGetRegister((gctUINT)Data);
    gctINT regIndex2 = ltcGetRegister((gctUINT)Key);

    return  regIndex1 ==  regIndex2 ;
} /* CompareIndex */

/* treat the Data and Key as temp register index with components
 * encoded in the value, check if the Index part are equal, and
 * all components in the Key are included in Data's components
 */
gctBOOL
CompareIndexAndComponents (
     IN void *    Data,
     IN void *    Key
     )
{
    gctINT regIndex1      = ltcGetRegister((gctUINT)Data);
    gctINT regIndex2      = ltcGetRegister((gctUINT)Key);

    if (regIndex1 == regIndex2)
    {
        gctINT regComponents1 = ltcGetComponents((gctUINT)Data);
        gctINT regComponents2 = ltcGetComponents((gctUINT)Key);
        /* check if Data contains all components in Key */
        return ((regComponents1 ^ regComponents2) & regComponents2) == 0;
    }
    return gcvFALSE;
} /* CompareIndex */

gctBOOL
ComparePtr (
     IN void *    Data,
     IN void *    Key
     )
{
    return  Data == Key ;
} /* ComparePtr */


static gceSTATUS
_CreateListNode(
    IN void *             Data,
    IN gctAllocatorFunc   Allocator,
    OUT gcsListNode **    ListNode
    )
{
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    /* Allocate a new gcsCodeListCode structure. */
    gcmONERROR(
        Allocator(gcvNULL,
                       sizeof(gcsListNode),
                       &pointer));
    *ListNode = (gcsListNode*) pointer;
    (*ListNode)->data = Data;
    (*ListNode)->next = gcvNULL;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _CreateListNode */

gceSTATUS
_CleanList(
    IN gcsList *          List,
    IN gctBOOL            FreeData
    )
{
    gceSTATUS           status = gcvSTATUS_OK;
    gcsListNode *       curNode;
    gctDeallocatorFunc  Dealloc = List->allocator->deallocate;

    gcmHEADER();

    curNode = List->head;
    while (curNode)
    {
        gcsListNode *nextNode = curNode->next;
        if (Dealloc)
        {
            if (FreeData)
            {
                gcmONERROR(Dealloc(gcvNULL, curNode->data));
            }
            gcmONERROR(Dealloc(gcvNULL, curNode));
        }
        curNode = nextNode;
    }

    List->head = gcvNULL;
    List->tail = gcvNULL;
    List->count = 0;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _CleanList */

gcsListNode *
_FoundInList(
    IN gcsList *      List,
    IN void *         Key,
    IN compareFunc    compare
    )
{
    gcsListNode * curNode;

    gcmHEADER();

    curNode = List->head;
    while (curNode)
    {
        if (compare(curNode->data, Key))
        {
            gcmFOOTER_NO();
            return curNode;
        }

        curNode = curNode->next;
    }

    gcmFOOTER_NO();
    return gcvNULL;
} /* _FoundInList */

static
gceSTATUS
_AddToList(
    IN gcsList *          List,
    IN void *             Data
    )
{
    gceSTATUS status;
    gcsListNode *node;

    gcmHEADER();

    gcmVERIFY_ARGUMENT(List != gcvNULL);

    gcmONERROR(_CreateListNode( Data, List->allocator->allocate, &node));
    /* the list is empty, set the head and tail to the new node */
    if ( List->head == gcvNULL)
        List->head = List->tail = node;
    else
    {
        List->tail->next = node;
        List->tail = node;
    }

    List->count++;
    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _AddToList */

/* remove the Node from List, free the Node memory */
static
gceSTATUS
_RemoveFromList(
    IN gcsList *          List,
    IN gcsListNode *      Node
    )
{
    gceSTATUS           status = gcvSTATUS_OK;
    gcmHEADER();

    gcmVERIFY_ARGUMENT(List != gcvNULL && Node != gcvNULL);

    if (List->head == Node)
    {
        List->head = Node->next;
    }
    else
    {
        gcsListNode *curNode = List->head;
        while (curNode && curNode->next != Node)
            curNode = curNode->next;
        if (curNode)
        {
            /* found the next node is the Node to remove */
            gcmASSERT(curNode->next == Node);
            curNode->next = Node->next;
        }
        else {
            /* not found */
            gcmASSERT(gcvFALSE);
        }
    }

    /* free the Node */
    gcmONERROR(List->allocator->deallocate(gcvNULL, Node));
    List->count--;
    gcmASSERT(List->count >= 0);

OnError:
    gcmFOOTER();
    return status;
} /* _RemoveFromList */

static
gceSTATUS
_AddToCodeList(
    IN gctCodeList        List,
    IN gcOPT_CODE         Code
    )
{
    return _AddToList(List, Code);
} /* _AddToCodeList */

static
gceSTATUS
_AddToTempRegList(
     IN gctTempRegisterList    List,
     IN gctINT                 Index)
{
    gctCHAR  buffer[512];
    gctUINT  offset = 0;
    /* check if there is a node in the list which has the same temp
     * register Index
     */
    gcsListNode * node = _FoundInList(List, (void *)Index, CompareIndex);
    if (node != gcvNULL)
    {
        /* the temp register is already in the list, update it with new
         * components be merged with old ones
         */
        gctINT oldValue = (gctINT)node->data;
        gctINT newValue;

        gcmASSERT(ltcGetRegister(Index) == ltcGetRegister(oldValue));

        newValue = ltcRegisterWithComponents(ltcGetRegister(Index),
                      ltcGetComponents(oldValue) | ltcGetComponents(Index));
        node->data = (void *)newValue;
        if (gcDumpOption(gceLTC_DUMP_COLLECTING))
        {
            gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                "_AddToTempRegList(Index=%#x): update %#x to %#x", Index, oldValue, newValue));
            gcoOS_Print("%s", buffer);
        }
        return gcvSTATUS_OK;
    }
    else
    {
        if (gcDumpOption(gceLTC_DUMP_COLLECTING))
        {
            gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                "_AddToTempRegList(Index=%#x): added new index", Index));
            gcoOS_Print("%s", buffer);
        }
        /* not in the list, add a new one */
        return _AddToList(List, (void *)Index);
    } /* if */
} /* _AddToTempRegList */

/*
 * Get used components in Source for the Instruction, the return value
 * is in Enable format:
 *
 *     MOV   temp(1).yz, temp(0).xxw
 *
 * the components used in source 0 are x and w, so the return value
 * is (gcSL_ENABLE_X | gcSL_ENABLE_W)
 *
 */
gcSL_ENABLE
gcGetUsedComponents(
    IN gcSL_INSTRUCTION Instruction,
    IN gctINT           SourceNo
    )
{
    gctUINT16    enable = gcmSL_TARGET_GET(Instruction->temp, Enable);
    gctUINT16    source = SourceNo == 0 ? Instruction->source0 : Instruction->source1;
    gcSL_ENABLE  usedComponents = 0;
    gctINT       i;

    for (i=0; i < gcSL_COMPONENT_COUNT; i++)
    {
        if (gcmIsComponentEnabled(enable, i))
        {
            gcSL_SWIZZLE swizzle = 0;
            switch (i)
            {
            case 0:
                swizzle = (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX);
                break;
            case 1:
                swizzle = (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY);
                break;
            case 2:
                swizzle = (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ);
                break;
            case 3:
                swizzle = (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW);
                break;
            default:
                gcmASSERT(0);
                break;
            } /* switch */
            /* the component in swizzle is used, convert swizzle to enable */
            usedComponents |= 1 << swizzle;
        } /* if */
    } /* for */
    return usedComponents;
} /* gcGetUsedComponents */

/* return true if the FristCode and SecondCode in the same Basic Block:
 *
 *   1. There is no control flow instruction in between
 *   2. There is no jump or call to the code in between
 *
 */
gctBOOL
isCodeInSameBB(
    IN gcOPT_CODE FirstCode,
    IN gcOPT_CODE SecondCode
    )
{
    gcOPT_CODE curCode = FirstCode;

    gcmASSERT(FirstCode != SecondCode);
    for (; curCode != gcvNULL && curCode != SecondCode; curCode = curCode->next)
    {
        gcSL_OPCODE opcode = (gcSL_OPCODE)curCode->instruction.opcode;
        if (   opcode == gcSL_JMP
            || opcode == gcSL_CALL
            || opcode == gcSL_RET)
        {
            return gcvFALSE;
        }

        if (curCode->callers != gcvNULL && curCode != FirstCode)
        {
            /* has other instruction jump to this code */
            return gcvFALSE;
        }
    } /* for */

    if (curCode == SecondCode)
    {
        /* found the second code can be reached without meet any control flow */
        return gcvTRUE;
    }

    /* check if the first and second are reversed */
    curCode = SecondCode;

    for (; curCode != gcvNULL && curCode != FirstCode; curCode = curCode->next)
    {
        gcSL_OPCODE opcode = (gcSL_OPCODE)curCode->instruction.opcode;
        if (   opcode == gcSL_JMP
            || opcode == gcSL_CALL
            || opcode == gcSL_RET)
        {
            return gcvFALSE;
        }

        if (curCode->callers != gcvNULL && curCode != SecondCode)
        {
            /* has other instruction jump to this code */
            return gcvFALSE;
        }
    } /* for */

    if (curCode == FirstCode)
    {
        /* found the first code can be reached without meet any control flow */
        return gcvTRUE;
    }
    return gcvFALSE;
} /* isCodeInSameBB */

/* return true if the FristCode is dominated by the SecondCode */
gctBOOL
dominatedBy(
    IN gcOPT_CODE FirstCode,
    IN gcOPT_CODE SecondCode
    )
{
    /* now we only handle if the frist and second code are in
     * the same basic block
     */
    return    isCodeInSameBB(SecondCode, FirstCode);

} /* dominatedBy */

gctBOOL
isRedefKillsAllPrevDef(
      IN gcOPT_LIST    Dependencies,
      IN gcSL_ENABLE   EanbledComponents
      )
{
    gcOPT_LIST curDep = Dependencies;

    /* go through the dependencies list and find the first one with
     * at least one of same enabled component, then find the next one
     * with the enabled component, if the later one has some component
     * overlapped with the first one, check if the later one kills the
     * first one, if the later one doesn't have overlapped component
     * with the first one, set the later one as start of next round of
     * check
     *
     *        14: MOV             temp(14).xyz, uniform(5).xyz
     *        14: Users: 16
     *            N Def: 15
     *        15: ADD             temp(14).z, uniform(5).z, 1.000000
     *        15: Users: 16
     *            P Def: 14
     *        16: NORM            temp(15).xyz, temp(14).xyz
     *        16: Users: 17
     *            Src 0: 15, 14
     */
    for (curDep = Dependencies; curDep != gcvNULL; )
    {
        gcOPT_CODE   firstCode;
        gctUINT16    curDepIdx;
        gcOPT_LIST   nextDep;
        gcOPT_LIST   dep;
        gcOPT_LIST   nextRoundStart = gcvNULL;
        gcSL_ENABLE  usedComponents;

        nextDep = curDep->next;
        if (nextDep == gcvNULL)
            break;

        if (curDep->index < 0) /* depend on global/parameter/input, skip it */
        {
            curDep = nextDep;
            continue;
        }

        firstCode = curDep->code;
        curDepIdx = firstCode->instruction.tempIndex;
        dep = nextDep;
        usedComponents = gcmSL_TARGET_GET(firstCode->instruction.temp, Enable);

        usedComponents &= EanbledComponents;
        /* the user of the dependency may only use part of components */
        if (usedComponents == 0)
        {
            curDep = nextDep;
            continue;
        }

        /* go through the rest of the list to find if there is
           a dependency has the same index */

        for (; dep != gcvNULL; dep = dep->next)
        {
            /* found the next dependency has the same index
               as current dependency */
            gctUINT16    depIdx;

            if (dep->code == gcvNULL) /* depend on input */
                continue;

            depIdx = dep->code->instruction.tempIndex;
            /*  check if the dependencies are in the same basic block
             *  or defines different components
             */
            if (depIdx == curDepIdx)
            {
                /* check if any component is used in the both dependencies */
                gcSL_ENABLE  depUsedComponents =
                              gcmSL_TARGET_GET(dep->code->instruction.temp, Enable);

                depUsedComponents &= EanbledComponents;

                if ( (usedComponents & depUsedComponents) != 0)
                {
                    /* both dependencies define the same component,
                     * check if the later kills the first one */
                    if (!dominatedBy( firstCode, dep->code ))
                        return gcvFALSE;
                    else
                        continue;
                }
                else if (nextRoundStart == gcvNULL)
                {
                    /* the later dependency code should be the next round start */
                    nextRoundStart = dep;
                }
            }

        } /* for */
        /* set the next round start */
        curDep = (nextRoundStart == gcvNULL) ? curDep->next
                                             : nextRoundStart;
    } /* for */

    /* all definitions are killed by later redefinition */
    return gcvTRUE;
} /* isRedefKillsAllPrevDef */

/* check if the code's dependencies have a same temp variable as
 * dependency for more than one time, it can happen in following case:
 *
 *  001  if (cond)
 *  002    temp1 = ...
 *  003  else
 *  004    temp1 = ...
 *  005  temp2 = temp1;
 *
 *  The source0 of stmt 005 will have 002 and 004 as its depenencies,
 *  the target temp registers of both statements are temp1.
 *
 * There are other cases may depend on the same temp register but
 * different components, which we should not count them as multiple
 * dependencies:
 *
 *  001  temp1.xy = u1
 *  002  temp1.z = 0.0
 *  003  temp1.w = 1.0
 *  004  temp2.yzw = temp1.xwyz
 *
 *  In 004, temp2 has 001, 002, 003 as its source0's dependencies, and temp1
 *  are the target temp register for these 3 statements
 *
 *  The EnabledComponents are the components enabled in the instruction,
 *  in the examle above, 004 has yzw enabled
 *
 *  Need to handle this case:
 *
 *  005  temp1.xy = u1
 *  006  temp1.yz = 0.0
 *  007  temp1.w = 1.0
 *  008  temp2 = temp1
 *
 *  008 is dependen on 005, 006, 007, there is a overlap component temp1.y@005
 *  and temp1.y@006, however the temp1.y@005 is killed by temp1.y@006, we should
 *  be able to get the temp1@008 as <u1.x, 0.0, 0.0, 1.0>
 *
 */
static gctBOOL
_hasMultipleDependencyForSameTemp(
      IN gcOPT_LIST    Dependencies,
      IN gcSL_ENABLE   EanbledComponents
      )
{
    gcOPT_LIST curDep = Dependencies;
    for (curDep = Dependencies; curDep != gcvNULL; curDep = curDep->next )
    {
        gcOPT_CODE   depCode;
        gctUINT16    curDepIdx;
        gcOPT_LIST   nextDep;
        gcOPT_LIST   dep;
        gcSL_ENABLE  usedComponents;

        if (curDep->index < 0) /* depend on global/parameter/input, skip it */
            continue;

        depCode = curDep->code;
        curDepIdx = depCode->instruction.tempIndex;
        nextDep = curDep->next;
        dep = nextDep;
        usedComponents = gcmSL_TARGET_GET(depCode->instruction.temp, Enable);

        /* the user of the dependency may only use part of components */
        usedComponents &= EanbledComponents ;

        /* go through the rest of the list to find if there is
           a dependency has the same index */

        for (; dep != gcvNULL; dep = dep->next)
        {
            /* found the next dependency has the same index
               as current dependency */
            gctUINT16    depIdx;

            if (dep->code == gcvNULL) /* depend on input */
                continue;

            depIdx = dep->code->instruction.tempIndex;
            /* TODO: check if the dependencies are in the same basic block
             *       or defines different components
             */
            if (depIdx == curDepIdx)
            {
                /* check if any component is used in the both dependencies */
                gcSL_ENABLE  depUsedComponents =
                              gcmSL_TARGET_GET(dep->code->instruction.temp, Enable);

                depUsedComponents &= EanbledComponents;

                if ( (usedComponents & depUsedComponents) != 0)
                {
                    /* TODO: check if the conflict component is actually killed by later
                     * assignment */
                    if (!isRedefKillsAllPrevDef(Dependencies,
                                                usedComponents & depUsedComponents))
                        return gcvTRUE;
                }

                usedComponents |= depUsedComponents; /* merge used components */
            } /* if */
        } /* for */
    } /* for */
    return gcvFALSE;
} /* _hasMultipleDependencyForSameTemp */

/* check if the temp register 'index' is load time constant at Code */
gctBOOL
_isTempRegisterALoadtimeConstant(
       IN gcOPT_CODE           Code,
       IN gctUINT              SourceNo,
       IN gctUINT              Index,
       IN gctTempRegisterList  LTCTempRegList)
{
    gcSL_INSTRUCTION    inst = &Code->instruction;
    gctUINT16           target = inst->tempIndex;
    gcOPT_LIST          dependencies;
    gcSL_ENABLE         enable = gcGetUsedComponents(inst, SourceNo);
    gctINT              indexWithComponents =
                           (gctINT)ltcRegisterWithComponents(Index, enable);

    /* if the temp register is not in the known loadtime constant temp
       register list, then it is not a loadtime constant */
    if ( _FoundInList(LTCTempRegList,
                      (void *)indexWithComponents,
                      CompareIndexAndComponents) == gcvNULL)
    {
        return gcvFALSE;
    }

    /* if the Index is merged before or at the Code with different values,
       then it is not a loadtime constant */
    dependencies = (SourceNo == 0) ? Code->dependencies0
                                   : Code->dependencies1;
    /* TODO: check if the target temp register depends on itself indirectly:
     *
     *     28: ADD             temp(20).x, temp(0).x, 2.000000
     *     29: MOV             temp(0).x, temp(20).x
     *     30: ADD             temp(21).x, temp(19).x, 1.000000
     *     31: MOV             temp(19).x, temp(21).x
     *     32: JMP.L           28, temp(19).x, uniform(2).x
     */
    if (target == Index)
        return gcvFALSE;  /* the code is depend on itself */

    /* the source has multiple dependcencies, which can happen in this case:
        if (cond)
           temp1 = u0;  // stmt1
         else
           temp1 = u1;  // stmt2

         the temp1 now depends on stmt1 and stmt2's target (temp1) in
         following statement

         temp2 = temp1 + u2;
    */
    if (_hasMultipleDependencyForSameTemp(dependencies, enable))
        return gcvFALSE;

    /* now the temp register index is in the LTCTempRegList and not
       from multiple source, then it is a Loadtime constant in Code */
    return gcvTRUE;
} /* _isTempRegisterALoadtimeConstant */

/* for source in Code's SourceNo (0 or 1), check if it is a loadtime constant */
static gctBOOL
_isLoadtimeConastant(
       IN gcOPT_CODE           Code,
       IN gctINT               SourceNo,
       IN gctTempRegisterList  LTCTempRegList)
{
    /* a source is a loadtime constant if it is one of following:
         1. Compile-time constant value (gcSL_CONSTANT)
         2. Loadtime constant value (gcSL_UNIFORM), if indexed,
            the index must LTC itself
         3. temp register (gcSL_TEMP) which operands are LTC, if indexed,
            the index must LTC itself, and temp register is not depend on
            itself directly or indirectly
    */
    gcSL_INSTRUCTION        inst = &Code->instruction;
    gctUINT16               source;
    gctUINT16               index;
    gctBOOL                 sourceIsLTC = gcvFALSE;

    gcmASSERT(SourceNo == 0 || SourceNo == 1);
    source = SourceNo == 0 ? inst->source0 : inst->source1;

    if (gcmSL_SOURCE_GET(source, Type) == gcSL_CONSTANT)
    {
        sourceIsLTC = gcvTRUE;
    }
    else if (gcmSL_SOURCE_GET(source, Type) == gcSL_UNIFORM)
    {
        /* check if it is indexed and index is LTC */
        if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
        {
            index = SourceNo == 0 ? inst->source0Indexed : inst->source1Indexed;
            if (_isTempRegisterALoadtimeConstant(Code, SourceNo, index, LTCTempRegList))
                sourceIsLTC = gcvTRUE;
        }
        else sourceIsLTC = gcvTRUE;

    }
    else if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
    {
        /* the temp register should be LTC */
        index = SourceNo == 0 ? inst->source0Index : inst->source1Index;
        if (    gcmSL_INDEX_GET(index, ConstValue) == 0
             && _isTempRegisterALoadtimeConstant(Code, SourceNo, index, LTCTempRegList) )
        {
            if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
            {
                /* cannot handle indexed temp variable yet */
                sourceIsLTC = gcvFALSE;
            }
            else sourceIsLTC = gcvTRUE;
        }
    }
    return sourceIsLTC;
} /* _isLoadtimeConastant */

static gceSTATUS
_RemoveTempComponentsFromLTCTempRegList(
       IN gctTempRegisterList  LTCTempRegList,
       IN gctINT               TempIndex,
       IN gcSL_ENABLE          Components)
{
    gceSTATUS           status = gcvSTATUS_OK;
    gctINT              indexWithComponents =
                           (gctINT)ltcRegisterWithComponents(TempIndex, Components);
    gcsListNode *       node;

    gcmHEADER();

    node = _FoundInList(LTCTempRegList, (void *)indexWithComponents, CompareIndex);

    if (node != gcvNULL)
    {
        /* the temp register is already in the list, update it with new
         * components be removing the components in the node
         */
        gctCHAR  buffer[512];
        gctUINT  offset = 0;
        gctINT       oldValue = (gctINT)node->data;
        gctINT       newValue;
        gcSL_ENABLE  updatedComponets;

        gcmASSERT(ltcGetRegister(indexWithComponents) == ltcGetRegister(oldValue));

        updatedComponets = ltcGetComponents(oldValue) & ~Components;
        if (updatedComponets == gcSL_ENABLE_NONE)
        {
            /* now after remove the killed components, there is no
             * loadtime constant component left in the temp register,
             * remove the temp register from the list
             */
            _RemoveFromList(LTCTempRegList, node);
            if (gcDumpOption(gceLTC_DUMP_COLLECTING))
            {
                gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                    "%s(TempIndex=%d)", __FUNCTION__, TempIndex));
                gcoOS_Print("%s", buffer);
            }
        }
        else
        {
            /* update the node with new value */
            newValue = ltcRegisterWithComponents(ltcGetRegister(indexWithComponents),
                                                 updatedComponets);
            node->data = (void *)newValue;
            if (gcDumpOption(gceLTC_DUMP_COLLECTING))
            {
                gcmVERIFY_OK(gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
                    "%s(TempIndex=%d): update %#x to %#x", __FUNCTION__, TempIndex,
                    oldValue, newValue));
                gcoOS_Print("%s", buffer);
            }
        }
    }

    gcmFOOTER();
    return status;
} /* _RemoveTempComponentsFromLTCTempRegList */

static gceSTATUS
_RemoveTargetFromLTCTempRegList(
       IN gcOPT_CODE           Code,
       IN gctTempRegisterList  LTCTempRegList)
{
    gcSL_INSTRUCTION    inst = &Code->instruction;
    gctUINT16           target = inst->tempIndex;
    gcSL_ENABLE         enable = (gcSL_ENABLE) gcmSL_TARGET_GET(inst->temp,
                                                                Enable);

    return _RemoveTempComponentsFromLTCTempRegList(LTCTempRegList,
                                                   target, enable);
} /* _RemoveTargetFromLTCTempRegList */


static gceSTATUS
_findLoadtimeConstantInFunction(
    IN gcOPTIMIZER          Optimizer,
    IN gcOPT_FUNCTION       Function
    )
{
    gceSTATUS            status = gcvSTATUS_OK;
    gcOPT_CODE           code;
    gcSL_INSTRUCTION     inst;
    gctUINT16            target;
    gctUINT16            source;
    gctUINT16            enabled;

    /* reset global and argument temp register. */
    {
        gcsFUNCTION_ARGUMENT_PTR argument;
        gcOPT_GLOBAL_USAGE usage;
        gctUINT a;

        /* remove constantness for all input arguments. */
        argument = Function->arguments;
        for (a = 0; a < Function->argumentCount; a++, argument++)
        {
            /* Check input arguments. */
            if (argument->qualifier != gcvFUNCTION_OUTPUT)
            {
                _RemoveTempComponentsFromLTCTempRegList(&Optimizer->theLTCTempRegList,
                                                        Function->arguments[a].index,
                                                        gcSL_ENABLE_XYZW);
            }
        }

        /* remove constantness for all global variables. */
        for (usage = Function->globalUsage; usage; usage = usage->next)
        {
            _RemoveTempComponentsFromLTCTempRegList(&Optimizer->theLTCTempRegList,
                                                    usage->index,
                                                    gcSL_ENABLE_XYZW);
        }
    }

    /* Walk through all the instructions, finding instructions uses LTC as sources. */
    for (code = Function->codeHead;
         code != Function->codeTail->next;
         code = code->next)
    {
        if (gcDumpOption(gceLTC_DUMP_COLLECTING))
            dbg_dumpCode(code);

        /* Get instruction. */
        inst = &code->instruction;
        enabled = gcmSL_TARGET_GET(inst->temp, Enable);
        /* Get target. */
        target = inst->tempIndex;
        source = inst->source0;
        if (gcmSL_SOURCE_GET(source, Type) != gcSL_NONE
            && !_isLoadtimeConastant(code, 0, &Optimizer->theLTCTempRegList) )
        {
            /* the source0 is not loadtime constant, kill the temp register
             * in theLTCTempRegList
             */
            _RemoveTargetFromLTCTempRegList(code, &Optimizer->theLTCTempRegList);
            continue;
        }

        source = inst->source1;
        if (gcmSL_SOURCE_GET(source, Type) != gcSL_NONE
            && !_isLoadtimeConastant(code, 1, &Optimizer->theLTCTempRegList) )
        {
            /* the source1 is not loadtime constant, kill the temp register
             * in theLTCTempRegList
             */
            _RemoveTargetFromLTCTempRegList(code, &Optimizer->theLTCTempRegList);
            continue;
        }

        /* all sources are loadtime constant, now find if it is an instructions
           we cannot fold it at loadtime to constant: texture load, kill, barrier,
           load, store, etc.
         */
        switch (inst->opcode)
        {
        case gcSL_MOV:
            /* mov instruction */
        case gcSL_RCP:
        case gcSL_ABS:
        case gcSL_FLOOR:
        case gcSL_CEIL:
        case gcSL_FRAC:
        case gcSL_LOG:
        case gcSL_RSQ:
        case gcSL_SAT:
        case gcSL_NORM:

        case gcSL_SIN:
        case gcSL_COS:
        case gcSL_TAN:
        case gcSL_EXP:
        case gcSL_SIGN:
        case gcSL_SQRT:
        case gcSL_ACOS:
        case gcSL_ASIN:
        case gcSL_ATAN:
            /* One-operand computational instruction. */
        case gcSL_ADD:
        case gcSL_SUB:
        case gcSL_MUL:
        case gcSL_MAX:
        case gcSL_MIN:
        case gcSL_POW:
        case gcSL_STEP:

        case gcSL_DP3:
        case gcSL_DP4:
        case gcSL_CROSS:
            /* Two-operand computational instruction. */

            /* all these operators are supported in
             * current constant folding routine
             */
            _AddToCodeList(&Optimizer->theLTCCodeList, code);

            /* add to LTC temp register list if is not already in it,
             * update it otherwise
             */
            _AddToTempRegList(&Optimizer->theLTCTempRegList,
                              ltcRegisterWithComponents(target, enabled));
            break;

        case gcSL_SET:
        case gcSL_DSX:
        case gcSL_DSY:
        case gcSL_FWIDTH:
        case gcSL_DIV:
        case gcSL_MOD:
        case gcSL_AND_BITWISE:
        case gcSL_OR_BITWISE:
        case gcSL_XOR_BITWISE:
        case gcSL_NOT_BITWISE:
        case gcSL_LSHIFT:
        case gcSL_RSHIFT:
        case gcSL_ROTATE:
        case gcSL_BITSEL:
        case gcSL_LEADZERO:
        case gcSL_ADDLO:
        case gcSL_MULLO:
        case gcSL_CONV:
        case gcSL_MULHI:
        case gcSL_CMP:
        case gcSL_I2F:
        case gcSL_F2I:
        case gcSL_ADDSAT:
        case gcSL_SUBSAT:
        case gcSL_MULSAT:
            /* these operators are not supported by constant folding
               need to add handling for them */
        case gcSL_CALL:
        case gcSL_JMP:
        case gcSL_RET:
        case gcSL_NOP:
        case gcSL_KILL:
                /* skip control flow. */
        case gcSL_TEXBIAS:
        case gcSL_TEXGRAD:
        case gcSL_TEXLOD:
                /* Skip texture state instructions. */
        case gcSL_TEXLD:
        case gcSL_TEXLDP:
                /* Skip texture sample instructions. */
        case gcSL_LOAD:
        case gcSL_STORE:
        case gcSL_BARRIER:
        case gcSL_GETEXP:
        case gcSL_GETMANT:
            /* although the sources are loadtime constant, but since we
             * don't process the result, the target is no longer a loadtime
             * constant, so we need to remove it from Loadtime temp register
             * list
             */
            _RemoveTargetFromLTCTempRegList(code, &Optimizer->theLTCTempRegList);
            continue;
        }

        /* special handle  control flow,  */
    }
    return status;
} /* _findLoadtimeConstantInFunction */

static void
_gcsListInit(
    IN gcsList *list
    )
{
    list->head = list->tail = gcvNULL;
    list->count = 0;
    list->allocator = &ltcAllocator;
}

/***************************************************************
**
**  find LTC in a function
**  for each code in function
**    if (all sources are ((in set_of_LTC_registers
**                          or  is_constant_or_uniform )
**                  && is not depend on mutiple value))
**    {
**      set_of_LTC_registers += code's target;
**
**      list_of_LTC_code += code;
**    }
**
**  // find LDC in program
**  list_of_LTC_code = {};
**  set_of_LTC_registers = {};
**  find LTC in main
**  for each used function
**      find LTC in the function
*/
static gceSTATUS
_FindLoadtimeConstant(
    IN gcOPTIMIZER Optimizer
    )
{
    gctSIZE_T            i;
    gceSTATUS            status;

    gcmHEADER();

    /* initialize the code list and temp register list */
    _gcsListInit(&Optimizer->theLTCTempRegList);
    _gcsListInit(&Optimizer->theLTCCodeList);

    /* Find loadtime constant for main. */
    gcmERR_RETURN(_findLoadtimeConstantInFunction(Optimizer, Optimizer->main));

    /* Find loadtime constant for each function. */
    for (i = 0; i < Optimizer->functionCount; i++)
    {
        gcmERR_RETURN(_findLoadtimeConstantInFunction(Optimizer, Optimizer->functionArray + i));
    }

    gcmFOOTER();
    return status;
} /* _FindLoadtimeConstant */

/* static */ gctBOOL
_isSourceSwizzled(gctUINT16 source)
{
    return (gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, Swizzle) != gcSL_SWIZZLE_XYZW;
}

/* return true if the code in the Loadtime expression only consists of MOV instructions */
static gctBOOL
_isLTCExpressionOnlySimpleMoves(
    IN gcOPT_CODE      Code
    )
{
    gcSL_INSTRUCTION        inst = &Code->instruction;
    gctUINT16                source;

    if (inst->opcode == gcSL_MOV)
    {
        /* go through its source to check if it is also MOV or constant/uniform */
        source = inst->source0;
        if (gcmSL_SOURCE_GET(source, Type) == gcSL_CONSTANT)
        {
            return gcvTRUE;
        }
        else if (gcmSL_SOURCE_GET(source, Type) == gcSL_UNIFORM)
        {
            /* since the code is already a LTC expression, we don't need
               to check its index */
            return gcvTRUE;
        }
    } /* if */

    return gcvFALSE;
} /* _isLTCExpressionOnlySimpleMoves */

static gcSHADER_TYPE
_MapTargetFormatToShaderType(
    IN  gcOPT_CODE                Code,
    OUT gctINT                    ComponentMap[4]
    )
{
    gctUINT16      enabled = gcmSL_TARGET_GET(Code->instruction.temp, Enable);
    gctINT         usedComponents = 0;
    gcSHADER_TYPE  type = gcSHADER_FLOAT_X1;
    gcSL_FORMAT    format;

    /* if the result is temp.yw, we should only create vector of two
     * of component type, e.g. float2 or int2, and map the y => x,
     * w => y
     */
    if (enabled & gcSL_ENABLE_X)
        ComponentMap[0] = usedComponents++;
    if (enabled & gcSL_ENABLE_Y)
        ComponentMap[1] = usedComponents++;
    if (enabled & gcSL_ENABLE_Z)
        ComponentMap[2] = usedComponents++;
    if (enabled & gcSL_ENABLE_W)
        ComponentMap[3] = usedComponents++;

    /* get uniform type from target format field */
    format = (gcSL_FORMAT)gcmSL_TARGET_GET(Code->instruction.temp, Format);
    switch (format)
    {
    case gcSL_FLOAT:
    case gcSL_FLOAT16:
        type = (usedComponents == 4) ? gcSHADER_FLOAT_X4 :
               (usedComponents == 3) ? gcSHADER_FLOAT_X3 :
               (usedComponents == 2) ? gcSHADER_FLOAT_X2 :
                                       gcSHADER_FLOAT_X1;
        break;
    case gcSL_INTEGER:
    case gcSL_UINT32:
    case gcSL_INT8:
    case gcSL_UINT8:
    case gcSL_INT16:
    case gcSL_UINT16:
        type = (usedComponents == 4) ? gcSHADER_INTEGER_X4 :
               (usedComponents == 3) ? gcSHADER_INTEGER_X3 :
               (usedComponents == 2) ? gcSHADER_INTEGER_X2 :
                                       gcSHADER_INTEGER_X1;
        break;
    case gcSL_BOOLEAN:
        type = (usedComponents == 4) ? gcSHADER_BOOLEAN_X4 :
               (usedComponents == 3) ? gcSHADER_BOOLEAN_X3 :
               (usedComponents == 2) ? gcSHADER_BOOLEAN_X2 :
                                       gcSHADER_BOOLEAN_X1;
        break;
    default:
        gcmASSERT(gcvFALSE);
    } /* switch */

    return type;
} /* _MapTargetFormatToShaderType */

/*******************************************************************************
**                            _CreateDummyUniformInfo
********************************************************************************
**
**  Create and add a dummy uniform with the Code's target type to shader
**
**  INPUT:
**
**      gcOPTIMIZER    Optimizer
**         Pointer to a gcOPTIMIZER structure
**
**      gcOPT_CODE     Code
**         the Code is final node in LTC expression, the result is
**         used by normal shader instruction
**
**  OUTPUT:
**
**      gcUNIFORM **     DummyUniformPtr
**          Pointer to created dummy uniform.
*/
static gceSTATUS
_CreateDummyUniformInfo(
    IN gcOPTIMIZER               Optimizer,
    IN gctCodeList               LTCCodeList,
    IN gcOPT_CODE                Code,
    OUT gcUNIFORM *              DummyUniformPtr,
    OUT gctINT                   ComponentMap[4]
    )
{
    gceSTATUS                 status;
    gcUNIFORM                 uniform;
    gctCHAR                   name[24];
    gcSHADER_TYPE             type;
    gctUINT                   offset;
    static gctINT             dummyUniformCount = 0;

    gcmHEADER();

    /* construct dummy uniform name */
    offset = 0;
    gcmVERIFY_OK(
        gcoOS_PrintStrSafe(name,
                           gcmSIZEOF(name),
                           &offset,
                           "#__dummy_uniform_%d", /* private (internal)
                                                   * uniform starts with '#' */
                           dummyUniformCount++));
    /* add uniform */
    type = _MapTargetFormatToShaderType(Code, ComponentMap);
    gcmONERROR(gcSHADER_AddUniform(Optimizer->shader, name,
                                   type, 1 /* Length */, &uniform));
    uniform->flags |= gcvUNIFORM_LOADTIME_CONSTANT;
    *DummyUniformPtr = uniform;
    gcmFOOTER();
    return status;

OnError:
    *DummyUniformPtr = gcvNULL;
    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _CreateDummyUniformInfo */

/* return -1 if not found, otherwise return the mapped value */
static gctUINT32
_FindInSimpleMap(
     IN SimpleMap *Map,
     IN gctUINT32    Key
     )
{
    while (Map)
    {
        if (Map->key == Key)
            return Map->val;
        Map = Map->next;
    }
    return (gctUINT32)(-1);
} /* _FindInSimpleMap */

/* Add a pair <Key, Val> to the Map head, the user should be aware that the
 * map pointer is always changed when adding a new node :
 *
 *   _AddNodeToSimpleMap(&theMap, key, val);
 *
 */
static
gceSTATUS
_AddNodeToSimpleMap(
     IN SimpleMap **   Map,
     IN gctUINT32      Key,
     IN gctUINT32      Val
     )
{
    gceSTATUS                 status = gcvSTATUS_OK;
    SimpleMap *               nodePtr;
    gcmHEADER();

    /* allocate memory */
    gcmONERROR(ltcAllocator.allocate(gcvNULL, sizeof(SimpleMap), (gctPOINTER *)&nodePtr));

    nodePtr->key  = Key;
    nodePtr->val  = Val;
    nodePtr->next = *Map;  /* always put the new node at the front */

    *Map = nodePtr;

OnError:
    gcmFOOTER();
    return status;
} /* _AddNodeToSimpleMap */


gceSTATUS
_DestroySimpleMap(
     IN SimpleMap *    Map
     )
{
    gceSTATUS                 status = gcvSTATUS_OK;
    SimpleMap *               nodePtr;
    gcmHEADER();

    while (Map)
    {
        nodePtr = Map->next;
        gcmONERROR(ltcAllocator.deallocate(gcvNULL, Map));
        Map = nodePtr;
    }

OnError:
    gcmFOOTER();
    return status;
} /* _DestroySimpleMap */

static SimpleMap * tempRegisterMap = gcvNULL;

static
gceSTATUS
_DestroyTempRegisterMap()
{
   gceSTATUS  status =  _DestroySimpleMap(tempRegisterMap);
   tempRegisterMap = gcvNULL;
   return status;
} /* _DestroytempRegisterMap */

/*****************************************************************************
**                            _CreateMoveAndChangeDependencies
******************************************************************************
**
**  The Code's result is a loadtime constant which is used by instruction which
**  is not LTC expression itself, a dummy uniform is created for the Code, now
**  we need to transform the Code to use the dummy uniform:
**
**     target.yw = expr
**
**  transform to:
**
**     MOV target.yw, dummy_uniform._x_y
**
**  and change Code's dependency list
**
**  INPUT:
**
**        gcOPTIMIZER             Optimizer
**            Pointer to a gcOPTIMIZER structure.
**
**      gcUniform *             DummyUniform
**         Pointer to a dummy uniform created for the Code
**
**      gcOPT_CODE              Code
**         Pointer to Loadtime Constant Code
**
**  OUTPUT:
**
**      NONE
**
**  Note:
**
**    an alternative or "better" way to do it may be to change the user of the
**    Code directly if the Code result is not an output, but it is more
**    complicated, we need to deal with indexed part also
**
*/
static gceSTATUS
_CreateMoveAndChangeDependencies(
    IN gcOPTIMIZER             Optimizer,
    IN gctUINT                 DummyUniformIndex,
    IN gcOPT_CODE              Code,
    IN gctINT                  ComponentMap[4]
    )
{
    gceSTATUS            status = gcvSTATUS_OK;
    gcSL_INSTRUCTION     inst = &Code->instruction;
    gctUINT16            source;
    gctUINT16            index;
    gctUINT16            indexRegister;
    gcOPT_LIST           list;
    gcSL_SWIZZLE         swizzle;
    gcmHEADER();

    /* update Code's dependencies list */
    while ((list = Code->dependencies0) != gcvNULL)
    {
        Code->dependencies0 = list->next;

        if (list->index >= 0)
        {
            gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer,
                                                  &list->code->users, Code));
        }

        /* Free the gcOPT_LIST structure. */
        gcmVERIFY_OK(gcOpt_FreeList(Optimizer, &list));
    } /* while */

    while ((list = Code->dependencies1) != gcvNULL)
    {
        Code->dependencies1 = list->next;

        /* It is possible that both sources are the same. */
        if (list->index >= 0)
        {
            gcOpt_DeleteCodeFromList(Optimizer, &list->code->users, Code);
        }

        /* Free the gcOPT_LIST structure. */
        gcmVERIFY_OK(gcOpt_FreeList(Optimizer, &list));
    } /* while */

    /* change the Code to MOV target, dummy_uniform */
    inst->opcode = gcSL_MOV;

    swizzle = (  ComponentMap[0] << 0 | ComponentMap[1] << 2
               | ComponentMap[2] << 4 | ComponentMap[3] << 6);
    /* Create the source. */
    source = gcmSL_SOURCE_SET(0, Type,    gcSL_UNIFORM)
           | gcmSL_SOURCE_SET(0, Indexed, gcSL_NOT_INDEXED)
           | gcmSL_SOURCE_SET(0, Swizzle, swizzle)
           | gcmSL_SOURCE_SET(0, Format, gcmSL_TARGET_GET(inst->temp, Format));

    /* Create the index. */
    index = gcmSL_INDEX_SET(0, Index, DummyUniformIndex);

    indexRegister = 0;

    /* Update source0 operand. */
    inst->source0        = source;
    inst->source0Index   = index;
    inst->source0Indexed = indexRegister;

    /* set source 1 to proper value */
    source = gcmSL_SOURCE_SET(0, Type,    gcSL_NONE)
           | gcmSL_SOURCE_SET(0, Indexed, gcSL_NOT_INDEXED);
    inst->source1        = source;

    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _CreateMoveAndChangeDependencies */

/* bit 0: source 0
 * bit 1: source 1
 * bit 2: source 0 Indexed
 * bit 3: source 1 Indexed
 */
#define isSourceProcessed(Map, Index, SourceId)       \
    ((Map)[Index] & (1 << SourceId))
#define setSourceProcessed(Map, Index, SourceId)      \
    ((Map)[Index] |= (1 << SourceId))
#define isSourceIndexProcessed(Map, Index, SourceId)  \
    ((Map)[Index] & (1 << ((SourceId)+2)))
#define setSourceIndexProcessed(Map, Index, SourceId) \
    ((Map)[Index] |=  (1 << ((SourceId)+2)))

/*******************************************************************************
**                            _CloneLTCExpressionToShader
********************************************************************************
**
**  Clone the instructions in LTCCodeList to shader's ltcExpressions
**  changes ltcExpressions' register number to the same as the instruction
**  index in the array if the register number is never seen before, otherwise
**  reuse the previous mapped index
**
**    001  mov  temp(10), c10
**    002  add  temp(3), temp(10), c1
**    003  mul  temp(3), temp(3), c2
**
**  ==>
**
**    001  mov  temp(1), c10
**    002  add  temp(2), temp(1), c1
**    003  mul  temp(2), temp(2), c2   // the result is assigned to both
**                                     // location: 002 and 003
**
**  INPUT:
**
**      gcOPTIMIZER    Optimizer
**         Pointer to a gcOPTIMIZER structure
**
**      gctCodeList    LTCCodeList
**         Pointer to Loadtime Constant Code List
**
**  OUTPUT:
**
**      NONE
**
*/
static gceSTATUS
_CloneLTCExpressionToShader(
    IN gcOPTIMIZER               Optimizer,
    IN gctCodeList               LTCCodeList
    )
{
    gceSTATUS            status = gcvSTATUS_OK;
    gcSL_INSTRUCTION     inst_list;
    gctINT               i;
    gcsCodeListNode *    codeNode;
    gcSL_INSTRUCTION     inst;
    gctUINT32            mappedIndex;
    gcOPT_LIST           users;
    gcOPT_CODE           code;
    gctUINT16            temp;
    gctCHAR *            processedSourceMap;  /* used to record which source in the
                                               * instruction is already processed */
    gcmHEADER();

    gcmASSERT(LTCCodeList->count != 0);

    gcmONERROR(ltcAllocator.allocate(gcvNULL,
                              sizeof(struct _gcSL_INSTRUCTION)*LTCCodeList->count,
                              (gctPOINTER *)&inst_list));

    gcmONERROR(ltcAllocator.allocate(gcvNULL,
                              sizeof(gctCHAR)*LTCCodeList->count,
                              (gctPOINTER *)&processedSourceMap));
    gcmONERROR(gcoOS_ZeroMemory(processedSourceMap,
                              sizeof(gctCHAR)*LTCCodeList->count));

    /* set shader's ltcExpressions */
    Optimizer->shader->ltcExpressions = inst_list;

    codeNode = LTCCodeList->head;
    for (i=0; i<LTCCodeList->count; i++)
    {
        gcmASSERT(codeNode != gcvNULL);
        code = (gcOPT_CODE)codeNode->data;

        /* copy the instruction */
        inst_list[i] = code->instruction;
        /* record the index to the ltc array to code */
        code->ltcArrayIdx = i;   /* the ltcArrayIdx starts from 1 */

        codeNode = codeNode->next;
    }

    /* fixup the temp registers in the inst_list so it will be easier to
     * calculate later:
     *   we map the 1st instruction's result to temp[0], change all its
     *   users to use temp[0], map 2nd instruction result to temp[1] if
     *   the temp register is a new one, and change all its user to use
     *   temp[1], and so on, ...
    */

    codeNode = LTCCodeList->head;
    for (i=0; i<LTCCodeList->count; i++)
    {
        inst = &inst_list[i];
        code = (gcOPT_CODE)codeNode->data;
        /* check the temp register */
        temp = inst->tempIndex;
        /* we don't handle indexed temp register for now */
        gcmASSERT(inst->tempIndexed == gcSL_NOT_INDEXED);

        /* map the temp register to an index in range [0, i) */
        mappedIndex = _FindInSimpleMap(tempRegisterMap, temp);
        if (mappedIndex == (gctUINT32)-1) /* not in the map, add a new one */
        {
            mappedIndex = i;
            gcmONERROR(_AddNodeToSimpleMap(&tempRegisterMap,
                                           temp, mappedIndex));
        }

        /* change the tempIndex to i */
        inst->tempIndex = (gctUINT16)mappedIndex;
        /* find all the uses of the temp by going through its users */
        users = code->users;
        for (users = code->users; users; users = users->next)
        {
            gctINT ltcArrayIdx;
            /* skip output/global/parameter variable usage */
            if (users->index < 0) continue;
            gcmASSERT(users->code != gcvNULL);
            ltcArrayIdx = users->code->ltcArrayIdx;
            if (ltcArrayIdx != -1)
            {
                gcSL_INSTRUCTION  use_inst;
                gctINT            j;
                /* the user's code is also in LTCCodeList,
                 * change its use to mappedIndex
                 */
                gcmASSERT(   ltcArrayIdx >= 0
                          && ltcArrayIdx < LTCCodeList->count);

                use_inst = &inst_list[ltcArrayIdx];

                for (j = 0; j < 2; j++)   /* iterate through the sources */
                {
                    gctUINT16 source = (j == 0) ? use_inst->source0
                                                : use_inst->source1;
                    if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP )
                    {
                        /* check the source index register */
                        gctUINT16 * index = (j == 0) ? &use_inst->source0Index
                                                     : &use_inst->source1Index;
                        if (*index == temp &&
                            !isSourceProcessed(processedSourceMap,
                                           ltcArrayIdx, j) )
                        {
                            /* the user uses the temp, change it to mappedIndex */
                            *index = (gctUINT16)mappedIndex;
                            setSourceProcessed(processedSourceMap,
                                               ltcArrayIdx, j);
                        }
                        /* check the source indexed register */
                        if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
                        {
                            index = (j == 0) ? &use_inst->source0Indexed
                                             : &use_inst->source1Indexed;
                            if (*index == temp &&
                                !isSourceIndexProcessed(processedSourceMap,
                                                          ltcArrayIdx, j))
                            {
                                /* the user uses the temp for its indexed register,
                                 * change it to mappedIndex */
                                *index = (gctUINT16)mappedIndex;
                                setSourceIndexProcessed(processedSourceMap,
                                                          ltcArrayIdx, j);
                            }
                        } /* if */
                    }
                    else if (gcmSL_SOURCE_GET(source, Type) == gcSL_UNIFORM )
                    {
                        /* check the source indexed register */
                        if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
                        {
                            gctUINT16 * index = (j == 0) ? &use_inst->source0Indexed
                                                         : &use_inst->source1Indexed;
                            if (*index == temp &&
                                !isSourceIndexProcessed(processedSourceMap,
                                                          ltcArrayIdx, j))
                            {
                                /* the user uses the temp for its indexed register,
                                 * change it to mappedIndex */
                                *index = (gctUINT16)mappedIndex;
                                setSourceIndexProcessed(processedSourceMap,
                                                          ltcArrayIdx, j);
                            }
                        } /* if */
                    } /* if */
                } /* for */
            } /* if */
        } /* for */
        codeNode = codeNode->next;
    } /* for */

    /* check sanity of the fixup: every temp register source and its index should be fixed */
    for (i=0; i<LTCCodeList->count; i++)
    {
        gctINT            j;
        inst = &inst_list[i];
        for (j = 0; j < 2; j++)   /* iterate through the sources */
        {
            gctUINT16 source = (j == 0) ? inst->source0
                                        : inst->source1;
            if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP )
            {
                /* check the source index register */
                if (!isSourceProcessed(processedSourceMap, i, j) )
                {
                        gcmASSERT(gcvFALSE);
                }
                /* check the source indexed register */
                if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
                {
                    if (!isSourceIndexProcessed(processedSourceMap, i, j))
                    {
                        gcmASSERT(gcvFALSE);
                    }
                } /* if */
            }
            else if (gcmSL_SOURCE_GET(source, Type) == gcSL_UNIFORM )
            {
                /* check the source indexed register */
                if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
                {
                    if (!isSourceIndexProcessed(processedSourceMap, i, j))
                    {
                        gcmASSERT(gcvFALSE);
                    }
                } /* if */
            } /* if */
        } /* for */
    } /* for */

    /* clean up the processed source map */
    gcmONERROR(ltcAllocator.deallocate(gcvNULL, processedSourceMap));
    /* cleanup the temp register map */
    gcmONERROR(_DestroyTempRegisterMap());

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
} /* _CloneLTCExpressionToShader */

/***************************************************************
** convert
**
**   target = expr
**
** to
**
**   dummy_uniform = expr;
**   mov target, dummy_uniform
**
** create a dummy uniform with code's result type
** change the instruction to move instruction:
**   move target, dummy_uniform
**
** remove code form code dependencies' user list
** change code's dependencies to dummy_uniform
**
*********************************************************************/
gceSTATUS
_FoldLoadtimeConstant(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS            status = gcvSTATUS_OK;
    gctINT *             ltcCodeUniformIndex;
    gctINT               i;

    gcOPT_CODE           code;
    gctUINT              dummyUniformCount = 0;
    /* the loadtime constant dummy uniforms always start after the user specified ones */
    gctUINT              ltcUniformBegin = Optimizer->shader->uniformCount;

    gctINT               codeIndex;
    gcsCodeListNode *    codeNode;
    gctINT               activeLTCInstCount = 0;

    gcmHEADER();

    if (Optimizer->theLTCCodeList.count == 0)
    {
        gcmFOOTER();
        return status;
    }

    /* clone the ltc expressions to shader */
    gcmONERROR(_CloneLTCExpressionToShader(Optimizer, &Optimizer->theLTCCodeList));

    /* create ltcCodeUniformIndex array */
    gcmONERROR(ltcAllocator.allocate(gcvNULL,
                   sizeof(gctINT) * Optimizer->theLTCCodeList.count,
                   (gctPOINTER *)&ltcCodeUniformIndex));

    if (gcDumpOption(gceLTC_DUMP_COLLECTING))
    {
        codeNode = Optimizer->theLTCCodeList.head;
        gcoOS_Print("Collected LTC expressions, count=%d", Optimizer->theLTCCodeList.count);
        for (i=0; i<Optimizer->theLTCCodeList.count; i++)
        {
            dbg_dumpIR(&Optimizer->shader->ltcExpressions[i], ((gcOPT_CODE)(codeNode->data))->id);
            codeNode = codeNode->next;
        }
    }

    /* initialize the array */
    for (i=0; i<Optimizer->theLTCCodeList.count; i++)
        ltcCodeUniformIndex[i] = -1;

    codeNode = Optimizer->theLTCCodeList.head;
    codeIndex = 0;    /* the index number for codeNode in theTLCCodeList */
    /* go through each code in the theLTCCodeList to decide if we need
       to create a uniform for it */
    while (codeNode)
    {
        gcOPT_LIST user;
        code = (gcOPT_CODE)(codeNode->data);
        for (user = code->users; user; user = user->next)
        {
            /* check if the user is output/global variable, otherwise
               check if the user is not in theLTCCodeList, if not, it means
               the result of the code is used by operation which is not LTC,
               generate a dummy uniform for the result. Replace the user's code
               to use the dummy unform
            */
            if (   user->index < 0
                || _FoundInList(&Optimizer->theLTCCodeList, user->code, ComparePtr) == gcvNULL)
            {


                /* check if the LTC expression is as simple as a string of MOV instruction,
                   no need to creat another uniform if the value is moved from uniform or
                   constant
                */
                if (!_isLTCExpressionOnlySimpleMoves(code))
                {
                    gctINT    componentMap[4] = {0, 0, 0, 0};
                    /* check if the dummy uniform is already created */
                    if (ltcCodeUniformIndex[codeIndex] == -1)
                    {
                        /* not created yet, we need to create a dummy uniform
                         * for the expression */
                        gcUNIFORM  aDummyUniformPtr;
                        gcmONERROR(_CreateDummyUniformInfo(Optimizer,
                                                           &Optimizer->theLTCCodeList,
                                                           code,
                                                           &aDummyUniformPtr,
                                                           componentMap));

                        /* map the codeIndex to uniform index */
                        ltcCodeUniformIndex[codeIndex] =
                                  aDummyUniformPtr->index;

                        dummyUniformCount++;
                    }

                    /* convert target = expr to MOV target, dummy_uniform */
                    _CreateMoveAndChangeDependencies(
                                  Optimizer,
                                  ltcCodeUniformIndex[codeIndex],
                                  code,
                                  componentMap);
                    activeLTCInstCount = codeIndex + 1;
                }
                else
                {
                    /* TODO: propagate the value to its users */

                }/* if */
            } /* if */
        } /* for */
        codeIndex++;
        codeNode = codeNode->next;
    } /* while */

    /* fill the shader with loadtime constant evaluation info */
    Optimizer->shader->ltcUniformCount = dummyUniformCount;
    Optimizer->shader->ltcUniformBegin = ltcUniformBegin;

    Optimizer->shader->ltcCodeUniformIndex = ltcCodeUniformIndex;
    Optimizer->shader->ltcInstructionCount = activeLTCInstCount;
    gcmASSERT(activeLTCInstCount <= Optimizer->theLTCCodeList.count);

    if ( Optimizer->shader->ltcUniformCount != 0)
    {
        /* notify the caller status changed */
        status = gcvSTATUS_CHANGED;
        DUMP_OPTIMIZER("After Loadtime Constant Folding", Optimizer);
    }
    else
    {
        /* remove allocated storage if there is no dummy uniform created */
        if (Optimizer->shader->ltcCodeUniformIndex != gcvNULL)
        {
            gcmONERROR(gcoOS_Free(gcvNULL,
                                  Optimizer->shader->ltcCodeUniformIndex));
            Optimizer->shader->ltcCodeUniformIndex = gcvNULL;
        }
        if (Optimizer->shader->ltcExpressions != gcvNULL)
        {
            gcmONERROR(gcoOS_Free(gcvNULL,
                                  Optimizer->shader->ltcExpressions));
            Optimizer->shader->ltcExpressions      = gcvNULL;
            Optimizer->shader->ltcInstructionCount = 0;
        }
    } /* if */

OnError:
    gcmFOOTER();
    return status;
}  /* _FoldLoadtimeConstant */


gceSTATUS
gcOPT_OptimizeLoadtimeConstant(
    IN gcOPTIMIZER Optimizer
    )
{
    gceSTATUS            status = gcvSTATUS_OK;
    gcmHEADER();

    gcmONERROR(_FindLoadtimeConstant(Optimizer));
    gcmONERROR(_FoldLoadtimeConstant(Optimizer));
    /* cleanup the LTC expressions, remove unneeded stuff */
    /* TODO: _CleanupLoadtimeConstantData() */
    _CleanList(&Optimizer->theLTCTempRegList, gcvFALSE);
    _CleanList(&Optimizer->theLTCCodeList, gcvFALSE);

OnError:
    gcmFOOTER();
    return status;
} /* gcOPT_OptimizeLoadtimeConstant */

#endif /* VIVANTE_NO_3D */

#else
/* interface stub to shut off link error */
gceSTATUS
gcOPT_OptimizeLoadtimeConstant(
    IN gcOPTIMIZER Optimizer
    )
{
    return gcvSTATUS_OK;
} /* gcOPT_OptimizeLoadtimeConstant */

#endif /* GC_ENABLE_LOADTIME_OPT */
