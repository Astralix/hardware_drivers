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

#ifndef VIVANTE_NO_3D

#include "gc_hal_optimizer.h"

#define _WORKAROUND_FOR_MEMORY_CORRUPTION_  1
#define _DEBUG_FOR_MEMORY_CORRUPTION_       0

#if _DEBUG_FOR_MEMORY_CORRUPTION_
/*#include <stdio.h>*/
#endif

#define _REMOVE_UNUSED_FUNCTION_CODE_   0
#define _RECURSIVE_BUILD_DU_            1

#define _GC_OBJ_ZONE	gcvZONE_COMPILER

/* Number of temp registers needed for gcSHADER_TYPE. */
static const gctUINT _typeSize[] =
{
	1,	/* gcSHADER_FLOAT_X1 */
	1,	/* gcSHADER_FLOAT_X2 */
	1,	/* gcSHADER_FLOAT_X3 */
	1,	/* gcSHADER_FLOAT_X4 */
	2,	/* gcSHADER_FLOAT_2X2 */
	3,	/* gcSHADER_FLOAT_3X3 */
	4,	/* gcSHADER_FLOAT_4X4 */
	1,	/* gcSHADER_BOOLEAN_X1 */
	1,	/* gcSHADER_BOOLEAN_X2 */
	1,	/* gcSHADER_BOOLEAN_X3 */
	1,	/* gcSHADER_BOOLEAN_X4 */
	1,	/* gcSHADER_INTEGER_X1 */
	1,	/* gcSHADER_INTEGER_X2 */
	1,	/* gcSHADER_INTEGER_X3 */
	1,	/* gcSHADER_INTEGER_X4 */
	1,	/* gcSHADER_SAMPLER_1D */
	1,	/* gcSHADER_SAMPLER_2D */
	1,	/* gcSHADER_SAMPLER_3D */
	1,	/* gcSHADER_SAMPLER_CUBIC */
	1,	/* gcSHADER_FIXED_X1 */
	1,	/* gcSHADER_FIXED_X2 */
	1,	/* gcSHADER_FIXED_X3 */
	1,	/* gcSHADER_FIXED_X4 */
	1,	/* gcSHADER_IMAGE_2D for OCL*/
	1,	/* gcSHADER_IMAGE_3D for OCL*/
	1,	/* gcSHADER_SAMPLER for OCL*/
	2,	/* gcSHADER_FLOAT_2X3 */
	2,	/* gcSHADER_FLOAT_2X4 */
	3,	/* gcSHADER_FLOAT_3X2 */
	3,	/* gcSHADER_FLOAT_3X4 */
	4,	/* gcSHADER_FLOAT_4X2 */
	4,	/* gcSHADER_FLOAT_4X3 */
	1,	/* gcSHADER_ISAMPLER_2D */
	1,	/* gcSHADER_ISAMPLER_3D */
	1,	/* gcSHADER_ISAMPLER_CUBIC */
	1,	/* gcSHADER_USAMPLER_2D */
	1,	/* gcSHADER_USAMPLER_3D */
	1,	/* gcSHADER_USAMPLER_CUBIC */
	1,	/* gcSHADER_SAMPLER_EXTERNAL_OES */
};

const struct _gcSL_INSTRUCTION gcvSL_NOP_INSTR =
{gcSL_NOP, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/******************************************************************************\
|************************* Memory management Functions ************************|
|******************************************************************************/
gcmMEM_DeclareFSMemPool(struct _gcOPT_CODE, 		Code,				)
gcmMEM_DeclareFSMemPool(struct _gcOPT_LIST,			List,				)
gcmMEM_DeclareFSMemPool(struct _gcOPT_GLOBAL_USAGE, GlobalUsage,		)

gcmMEM_DeclareAFSMemPool(struct _gcOPT_CODE,		CodeArray,			)
gcmMEM_DeclareAFSMemPool(struct _gcOPT_FUNCTION,	FunctionArray,		)
gcmMEM_DeclareAFSMemPool(struct _gcOPT_TEMP,		TempArray,			)
gcmMEM_DeclareAFSMemPool(struct _gcOPT_TEMP_DEFINE,	TempDefineArray,	)

static gceSTATUS
_MemPoolInit(
	gcOPTIMIZER			Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcoOS				os = gcvNULL;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	gcmERR_RETURN(gcfMEM_InitFSMemPool(&Optimizer->codeMemPool,		os, 100, sizeof(struct _gcOPT_CODE)));
	gcmERR_RETURN(gcfMEM_InitFSMemPool(&Optimizer->listMemPool,		os, 500, sizeof(struct _gcOPT_LIST)));
	gcmERR_RETURN(gcfMEM_InitFSMemPool(&Optimizer->usageMemPool,	os,  50, sizeof(struct _gcOPT_GLOBAL_USAGE)));

	gcmERR_RETURN(gcfMEM_InitAFSMemPool(&Optimizer->codeArrayMemPool,			os, 300, sizeof(struct _gcOPT_CODE)));
	gcmERR_RETURN(gcfMEM_InitAFSMemPool(&Optimizer->functionArrayMemPool,		os,  10, sizeof(struct _gcOPT_FUNCTION)));
	gcmERR_RETURN(gcfMEM_InitAFSMemPool(&Optimizer->tempArrayMemPool,			os, 100, sizeof(struct _gcOPT_TEMP)));
	gcmERR_RETURN(gcfMEM_InitAFSMemPool(&Optimizer->tempDefineArrayMemPool,		os, 100, sizeof(struct _gcOPT_TEMP_DEFINE)));

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_MemPoolCleanup(
	gcOPTIMIZER			Optimizer
	)
{
	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	gcfMEM_FreeFSMemPool(&Optimizer->codeMemPool);
	gcfMEM_FreeFSMemPool(&Optimizer->listMemPool);
	gcfMEM_FreeFSMemPool(&Optimizer->usageMemPool);

	gcfMEM_FreeAFSMemPool(&Optimizer->codeArrayMemPool);
	gcfMEM_FreeAFSMemPool(&Optimizer->functionArrayMemPool);
	gcfMEM_FreeAFSMemPool(&Optimizer->tempArrayMemPool);
	gcfMEM_FreeAFSMemPool(&Optimizer->tempDefineArrayMemPool);

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

#if _DEBUG_FOR_MEMORY_CORRUPTION_
static void
_CheckList(
	IN gcOPT_LIST		InputList,
	IN gcOPT_CODE		Code
    )
{
	gcOPT_LIST			list;

	for (list = InputList; list; list = list->next)
	{
		if (list->index >= 0)
		{
            if (list->code == gcvNULL)
            {
                printf("Invalid list->code (index=%d).\n", list->index);
                printf("InputList=%p list=%p\n", InputList, list);
                printf("Code->id=%d Code->instruction.opcode=%d\n", Code->id, Code->instruction.opcode);
                printf("list->code->id=%d\n", list->code->id);
            }
        }
    }
}
#endif

/******************************************************************************\
|************************** List Supporting Functioins ************************|
\******************************************************************************/
gceSTATUS
gcOpt_AddIndexToList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gctINT			Index
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_LIST			list;

	gcmHEADER_ARG("Optimizer=%p Root=%p Index=%d", Optimizer, Root, Index);

	/* Walk all entries in the list. */
	for (list = *Root; list; list = list->next)
	{
		/* Does the current list entry matches the new one? */
		if (list->index == Index)
		{
			/* Success. */
			gcmFOOTER_ARG("*Root=%p", *Root);
			return gcvSTATUS_OK;
		}
	}

	/* Allocate a new gcOPT_LIST structure. */
	gcmERR_RETURN(_CAllocateList(Optimizer->listMemPool, &list));

	/* Initialize the gcOPT_LIST structure. */
	list->next    = *Root;
	list->index   = Index;
	list->code    = gcvNULL;

	/* Link the new gcOPT_LIST structure into the list. */
	*Root = list;

	/* Success. */
	gcmFOOTER_ARG("*Root=%p", *Root);
	return gcvSTATUS_OK;
}

gceSTATUS
gcOpt_ReplaceIndexInList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gctUINT			Index,
	IN gctUINT			NewIndex
	)
{
	gcOPT_LIST			list;

	gcmHEADER_ARG("Optimizer=%p Root=%p Index=%d NewIndex=%d",
					Optimizer, Root, Index, NewIndex);

	/* Walk all entries in the list. */
	for (list = *Root; list; list = list->next)
	{
		if (list->index == (gctINT) Index)
		{
			/* Found it, and replace it. */
			list->index = NewIndex;
			gcmFOOTER_ARG("*Root=%p", *Root);
			return gcvSTATUS_OK;
		}
	}

	gcmFOOTER_ARG("*Root=%p status=%d", *Root, gcvSTATUS_NO_MORE_DATA);
	return gcvSTATUS_NO_MORE_DATA;
}

gceSTATUS
gcOpt_DeleteIndexFromList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gctUINT			Index
	)
{
	gcOPT_LIST			list;
	gcOPT_LIST			prevList = gcvNULL;

	gcmHEADER_ARG("Optimizer=%p Root=%p Index=%d", Optimizer, Root, Index);

	/* Walk all entries in the list. */
	for (list = *Root; list; list = list->next)
	{
		if (list->index == (gctINT) Index)
		{
			break;
		}
		prevList = list;
	}

	if (list)
	{
		if (prevList)
		{
			prevList->next = list->next;
		}
		else
		{
			*Root = list->next;
		}

		/* Free the gcOPT_LIST structure. */
		gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
	}
	else
	{
		gcmFOOTER_ARG("*Root=%p status=%d", *Root, gcvSTATUS_NO_MORE_DATA);
		return gcvSTATUS_NO_MORE_DATA;
	}

	/* Success. */
	gcmFOOTER_ARG("*Root=%p", *Root);
	return gcvSTATUS_OK;
}

gceSTATUS
gcOpt_DestroyList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root
	)
{
	gcOPT_LIST			list, nextList;
    gceSTATUS           status;

	gcmHEADER_ARG("Optimizer=%p Root=%p", Optimizer, Root);

	gcmASSERT(Root);
    if (Root == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    for (list = *Root; list; list = nextList)
	{
		nextList = list->next;

		/* Free the gcOPT_LIST structure. */
		gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
	}

	*Root = gcvNULL;

	gcmFOOTER_ARG("*Root=%p", *Root);
	return gcvSTATUS_OK;
}

gceSTATUS
gcOpt_FreeList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	List
	)
{
    gceSTATUS           status;

	gcmHEADER_ARG("Optimizer=%p List=%p", Optimizer, List);

	gcmASSERT(List);
    if (List == gcvNULL)
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

	gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, *List));

	*List = gcvNULL;

	gcmFOOTER_ARG("*List=%p", *List);
	return gcvSTATUS_OK;
}

gceSTATUS
gcOpt_AddCodeToList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_LIST			list;
#if _WORKAROUND_FOR_MEMORY_CORRUPTION_
	gcOPT_LIST			list1;
#endif

	gcmHEADER_ARG("Optimizer=%p Root=%p Code=%p", Optimizer, Root, Code);

	/* Walk all entries in the list. */
	for (list = *Root; list; list = list->next)
	{
		/* Does the current list entry matches the new one? */
		if (list->code == Code)
		{
			/* Success. */
			gcmFOOTER_ARG("*Root=%p", *Root);
			return gcvSTATUS_OK;
		}
	}

	/* Allocate a new gcOPT_LIST structure. */
	gcmERR_RETURN(_CAllocateList(Optimizer->listMemPool, &list));

#if _WORKAROUND_FOR_MEMORY_CORRUPTION_
    /* Workaround for a weird memory corruption, maybe due to optimization. */
    list1 = list;
#endif

	/* Initialize the gcOPT_LIST structure. */
	list->next  = *Root;
	list->code  = Code;
    list->index = 0;

#if _WORKAROUND_FOR_MEMORY_CORRUPTION_
    /* Workaround for a weird memory corruption, maybe due to optimization. */
    if (list1->code != Code)
    {
#if _DEBUG_FOR_MEMORY_CORRUPTION_
        printf("list->code=%p\n", list->code);
        printf("list1->code=%p\n", list1->code);
        printf("list->code->id=%d\n", list->code->id);
        printf("(*Root)->code=%p\n", (*Root)->code);
#else
        list1->next  = *Root;
        list1->code  = Code;
        list1->index = 0;
#endif
    }
#endif

#if _DEBUG_FOR_MEMORY_CORRUPTION_
    _CheckList(list, Code);
#endif

	/* Link the new gcOPT_LIST structure into the list. */
#if _WORKAROUND_FOR_MEMORY_CORRUPTION_
	*Root = list1;
#else
	*Root = list;
#endif

	/* Success. */
	gcmFOOTER_ARG("*Root=%p", *Root);
	return gcvSTATUS_OK;
}

gceSTATUS
gcOpt_DeleteCodeFromList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
	)
{
	gcOPT_LIST			list;
	gcOPT_LIST			prevList = gcvNULL;

	gcmHEADER_ARG("Optimizer=%p Root=%p Code=%p", Optimizer, Root, Code);

	/* Walk all entries in the list. */
	for (list = *Root; list; list = list->next)
	{
		if (list->code == Code)
		{
			break;
		}
		prevList = list;
	}

	if (list)
	{
		if (prevList)
		{
			prevList->next = list->next;
		}
		else
		{
			*Root = list->next;
		}

		/* Free the gcOPT_LIST structure. */
		gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
	}
	else
	{
		gcmFOOTER_ARG("*Root=%p status=%d", *Root, gcvSTATUS_NO_MORE_DATA);
		return gcvSTATUS_NO_MORE_DATA;
	}

	/* Success. */
	gcmFOOTER_ARG("*Root=%p", *Root);
	return gcvSTATUS_OK;
}

gceSTATUS
gcOpt_ReplaceCodeInList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code,
	IN gcOPT_CODE		NewCode
	)
{
	gcOPT_LIST			list;

	gcmHEADER_ARG("Optimizer=%p Root=%p Code=%d NewCode=%d",
					Optimizer, Root, Code, NewCode);

	/* Walk all entries in the list. */
	for (list = *Root; list; list = list->next)
	{
		if (list->code == Code)
		{
			/* Found it, and replace it. */
			list->code = NewCode;
			gcmFOOTER_ARG("*Root=%p", *Root);
			return gcvSTATUS_OK;
		}
	}

	gcmFOOTER_ARG("*Root=%p status=%d", *Root, gcvSTATUS_NO_MORE_DATA);
	return gcvSTATUS_NO_MORE_DATA;
}

gceSTATUS
gcOpt_CheckCodeInList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
    )
{
    gcOPT_LIST			list;

	gcmHEADER_ARG("Optimizer=%p Root=%p Code=%d",
					Optimizer, Root, Code);

	/* Walk all entries in the list. */
	for (list = *Root; list; list = list->next)
	{
		if (list->code == Code)
		{
			/* Found it */
			gcmFOOTER_ARG("*Root=%p", *Root);
			return gcvSTATUS_TRUE;
		}
	}

	gcmFOOTER_ARG("*Root=%p status=%d", *Root, gcvSTATUS_NO_MORE_DATA);
	return gcvSTATUS_FALSE;
}

gceSTATUS
gcOpt_AddListToList(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_LIST		SrcList,
	IN OUT gcOPT_LIST *	Root
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_LIST			list;

	gcmHEADER_ARG("Optimizer=%p SrcList=%p Root=%p", Optimizer, SrcList, Root);

	for (list = SrcList; list; list = list->next)
	{
        if (list->index < 0)
        {
    		gcmERR_BREAK(gcOpt_AddIndexToList(Optimizer, Root, list->index));
        }
        else
        {
            gcmERR_BREAK(gcOpt_AddCodeToList(Optimizer, Root, list->code));
        }
	}

	/* Success. */
	gcmFOOTER_ARG("*Root=%p status=%d", *Root, status);
	return gcvSTATUS_OK;
}

/*******************************************************************************
**							   gcOpt_InitializeTempArray
********************************************************************************
**
**	Initialize temp registers' isGlobal, usage, isIndex.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_InitializeTempArray(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_TEMP			tempArray = Optimizer->tempArray;
	gcOPT_TEMP			temp;
	gctUINT16			source;
	gctUINT16			target;
	gctUINT16			index;
	gcOPT_CODE          code;
	gctUINT				tempCount = Optimizer->tempCount;
	gctUINT				i;
	gctBOOL				needToCheckGlobal = (Optimizer->functionCount > 0);

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	/* Reset isGlobal, usage, isIndex, and temp. */
	temp = tempArray;

    if (tempCount > 0 && temp == gcvNULL)
    {
        gcmASSERT(temp);
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

	for (i = 0; i < tempCount; i++, temp++)
	{
		temp->isGlobal = gcvFALSE;
		temp->usage = 0;
		temp->isIndex = gcvFALSE;
		temp->temp = (gctPOINTER) -1;
        temp->format = -1;
	}

	if (needToCheckGlobal)
	{
		if (Optimizer->outputCount > 0 && tempArray == gcvNULL)
        {
            gcmASSERT(tempArray);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        /* Set output registers as global. */
		for (i = 0; i < Optimizer->outputCount; i++)
		{
            /* output could be NULL if it is not used */
            if (Optimizer->outputs[i] == gcvNULL)
                continue;

			tempArray[Optimizer->outputs[i]->tempIndex].isGlobal = gcvTRUE;
		}
	}

	/* Set usage and isGlobal. */
    for (code = Optimizer->codeHead; code; code = code->next)
	{
        switch (code->instruction.opcode)
		{
		case gcSL_CALL:
		case gcSL_JMP:
		case gcSL_RET:
		case gcSL_NOP:
		case gcSL_KILL:
			/* Skip control flow instructions. */
			temp = gcvNULL;
			break;

		case gcSL_TEXBIAS:
		case gcSL_TEXGRAD:
		case gcSL_TEXLOD:
			/* Skip texture state instructions. */
			temp      = gcvNULL;
			break;

		case gcSL_STORE:
			/* Skip store instructions. */
			temp = gcvNULL;
			break;

        default:
			/* Get gcSL_TARGET field. */
			target = code->instruction.temp;

			/* Get pointer to temporary register. */
			temp = tempArray + code->instruction.tempIndex;

			/* Update register usage. */
			temp->usage |= gcmSL_TARGET_GET(target, Enable);

			if (needToCheckGlobal && temp->function == gcvNULL && !temp->isGlobal)
			{
				/* Check if the register is global. */
				if (temp->temp != (gctPOINTER) code->function)
				{
					if (temp->temp == (gctPOINTER) -1)
					{
						temp->temp = (gctPOINTER) code->function;
					}
					else
					{
						temp->isGlobal = gcvTRUE;
					}
				}
			}
			break;
		}

		/* If isIndex is needed, comment out the next line. */
		if (!needToCheckGlobal) continue;

		/* Determine usage of source0. */
		source = code->instruction.source0;

		if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
		{
			/* Get pointer to temporary register. */
			temp = tempArray + code->instruction.source0Index;

			if (temp->function == gcvNULL && !temp->isGlobal)
			{
				/* Check if the register is global. */
				if (temp->temp != (gctPOINTER) code->function)
				{
					if (temp->temp == (gctPOINTER) -1)
					{
						temp->temp = (gctPOINTER) code->function;
					}
					else
					{
						temp->isGlobal = gcvTRUE;
					}
				}
			}
		}

		if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
		{
			index = code->instruction.source0Indexed;

			/* Get pointer to temporary register. */
			temp = tempArray + gcmSL_INDEX_GET(index, Index);

			/* Mark the temporary register is used as an index. */
			temp->isIndex = gcvTRUE;

			if (temp->function == gcvNULL && !temp->isGlobal)
			{
				/* Check if the register is global. */
				if (temp->temp != (gctPOINTER) code->function)
				{
					if (temp->temp == (gctPOINTER) -1)
					{
						temp->temp = (gctPOINTER) code->function;
					}
					else
					{
						temp->isGlobal = gcvTRUE;
					}
				}
			}
		}


		/* Determine usage of source1. */
		source = code->instruction.source1;

		if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
		{
			/* Get pointer to temporary register. */
			temp = tempArray + code->instruction.source1Index;

			if (temp->function == gcvNULL && !temp->isGlobal)
			{
				/* Check if the register is global. */
				if (temp->temp != (gctPOINTER) code->function)
				{
					if (temp->temp == (gctPOINTER) -1)
					{
						temp->temp = (gctPOINTER) code->function;
					}
					else
					{
						temp->isGlobal = gcvTRUE;
					}
				}
			}

		}

		if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
		{
			index = code->instruction.source1Indexed;

			/* Get pointer to temporary register. */
			temp = tempArray + gcmSL_INDEX_GET(index, Index);

			/* Mark the temporary register is used as an index. */
			temp->isIndex = gcvTRUE;

			if (temp->function == gcvNULL && !temp->isGlobal)
			{
				/* Check if the register is global. */
				if (temp->temp != (gctPOINTER) code->function)
				{
					if (temp->temp == (gctPOINTER) -1)
					{
						temp->temp = (gctPOINTER) code->function;
					}
					else
					{
						temp->isGlobal = gcvTRUE;
					}
				}
			}
		}
	}

	if (needToCheckGlobal)
	{
		/* Add global temp registers to global list. */
		temp = tempArray;
		for (i = 0; i < tempCount; i++, temp++)
		{
			if (temp->isGlobal)
			{
				gcOPT_LIST list;
				gcmERR_RETURN(_CAllocateList(Optimizer->listMemPool, &list));

				/* Add to global variable list. */
				list->index = i;
				list->next = Optimizer->global;
				Optimizer->global = list;
			}
		}
	}

	/* Initialize arrayVariable. */
	if (Optimizer->shader->variableCount > 0)
	{
		gctUINT variableCount = Optimizer->shader->variableCount;
		gctUINT v;

		for (v = 0; v < variableCount; v++)
		{
			gcVARIABLE variable = Optimizer->shader->variables[v];

            if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
            {
			    gctUINT	size = variable->arraySize * _typeSize[variable->u.type];
			    gctUINT j;

			    gcmASSERT(variable->tempIndex + size <= tempCount);
			    temp = tempArray + variable->tempIndex;
			    for (j = 0; j < size; j++, temp++)
			    {
				    temp->arrayVariable = variable;
			    }
            }
		}
	}

	gcmFOOTER();
	return status;
}

/*******************************************************************************
**							   _BuildTempArray
********************************************************************************
**
**	Build optimizer's temp register array.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
static gceSTATUS
_BuildTempArray(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_CODE          code;
	gctUINT				i, tempCount;
	gcOPT_TEMP			tempArray = gcvNULL;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	/* Determine the number of temporary registers. */
	tempCount = 0;

	/* Check symbol table to find the maximal index. */
	if (Optimizer->shader->variableCount > 0)
	{
		gcSHADER shader = Optimizer->shader;
		gctUINT variableCount = shader->variableCount;

		for (i = 0; i < variableCount; i++)
		{
			gcVARIABLE variable = shader->variables[i];

            if (variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
            {
			    gctUINT	size = variable->arraySize * _typeSize[variable->u.type];
			    gctUINT end = variable->tempIndex + size;

			    if (end > tempCount) tempCount = end;
            }
		}
	}

    /* We need to check output for VS since VS can declare output but without
    ** using. */
    if ((Optimizer->shader->type == gcSHADER_TYPE_VERTEX) &&
        (Optimizer->shader->outputCount > 0))
	{
		gcSHADER shader = Optimizer->shader;
        gctUINT outputCount = shader->outputCount;

		for (i = 0; i < outputCount; i++)
		{
            gcOUTPUT output = shader->outputs[i];
			gctUINT	size;
			gctUINT end;

            /* output could be null if it is not used */
            if (output == gcvNULL)
                continue;

			size = output->arraySize * _typeSize[output->type];
			end = output->tempIndex + size;

			if (end > tempCount) tempCount = end;
		}
	}
    /******************** Check the argument temp index Function **********************/
    for (i = 0; i < Optimizer->shader->functionCount; ++i)
    {
        gcFUNCTION function = Optimizer->shader->functions[i];
        gctSIZE_T j;

        for (j = 0; j < function->argumentCount; ++j)
        {
            gctINT argIndex = function->arguments[j].index;

            if  (argIndex >= (gctINT) tempCount)
            {
                tempCount = argIndex + 1;
            }
        }
    }

    /*************** Check the argument temp index for Kernel Function *****************/
    for (i = 0; i < Optimizer->shader->kernelFunctionCount; ++i)
    {
        gcKERNEL_FUNCTION kernelFunction = Optimizer->shader->kernelFunctions[i];
        gctSIZE_T j;

        for (j = 0; j < kernelFunction->argumentCount; ++j)
        {
            gctINT argIndex = kernelFunction->arguments[j].index;
            if  (argIndex >= (gctINT) tempCount)
            {
                tempCount = argIndex + 1;
            }
        }
    }

    for (code = Optimizer->codeHead; code; code = code->next)
	{
        switch (code->instruction.opcode)
		{
		case gcSL_NOP:
		case gcSL_KILL:
		case gcSL_JMP:
		case gcSL_CALL:
		case gcSL_RET:
			/* Skip control flow instructions. */
			break;

		case gcSL_TEXBIAS:
		case gcSL_TEXGRAD:
		case gcSL_TEXLOD:
			/* Skip texture status instructions. */
			break;

		default:
			if ((gctUINT) code->instruction.tempIndex >= tempCount)
			{
				/* Adjust temporary register count. */
				tempCount = code->instruction.tempIndex + 1;
			}

			/* No need to check sources assuming all references are after setting. */
		}
	}

	if (Optimizer->tempArray == gcvNULL)
	{
        if (tempCount > 0)
        {
    		gcmERR_RETURN(_CAllocateTempArray(Optimizer->tempArrayMemPool, &tempArray, tempCount));
    	    Optimizer->tempArray = tempArray;
        }
	    Optimizer->tempCount = tempCount;
	}
	/* Set function for function arguments. */
	for (i = 0; i < Optimizer->functionCount; i++)
	{
		gcOPT_FUNCTION function = Optimizer->functionArray + i;
    	gcsFUNCTION_ARGUMENT_PTR argument;
		gctUINT j;

        if (function->argumentCount > 0 && tempArray == gcvNULL)
        {
            gcmASSERT(tempArray);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        argument = function->arguments;
		for (j = 0; j < function->argumentCount; j++, argument++)
		{
			gctUINT index = argument->index;

			gcmASSERT(tempArray[index].function == gcvNULL);
			tempArray[index].function = function;
			tempArray[index].argument = argument;
		}
	}

	/* Initialize temp registers' flags. */
	gcmERR_RETURN(gcOpt_InitializeTempArray(Optimizer));

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/*******************************************************************************
**							   gcOpt_ClearTempArray
********************************************************************************
**
**	Clear the lists in optimizer's temp register array.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
**
**		gcOPT_TEMP_DEFINE TempDefineArray
**			Pointer to a gcOPT_TEMP_DEFINE structure.
*/
gceSTATUS
gcOpt_ClearTempArray(
	IN gcOPTIMIZER			Optimizer,
	IN gcOPT_TEMP_DEFINE	TempDefineArray
	)
{
	gctUINT					i;
	gcOPT_TEMP_DEFINE		tempDefine;

	gcmHEADER_ARG("Optimizer=%p TempDefineArray=%p", Optimizer, TempDefineArray);

	tempDefine = TempDefineArray;
	for (i = 0; i < Optimizer->tempCount; i++, tempDefine++)
	{
		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &tempDefine->xDefines));
		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &tempDefine->yDefines));
		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &tempDefine->zDefines));
		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &tempDefine->wDefines));
	}

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/*******************************************************************************
**							   _DestroyTempArray
********************************************************************************
**
**	Destroy optimizer's temp register array.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
static gceSTATUS
_DestroyTempArray(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_TEMP			tempArray = Optimizer->tempArray;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	if (tempArray == gcvNULL)
	{
		gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}

	gcmVERIFY_OK(_FreeTempArray(Optimizer->tempArrayMemPool, Optimizer->tempArray));
	Optimizer->tempArray = gcvNULL;

	gcmFOOTER();
	return status;
}

/*******************************************************************************
**							   _BuildGlobalUsage
********************************************************************************
**
**	Build global usage for functions.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
static gceSTATUS
_BuildGlobalUsage(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_FUNCTION		functionArray = Optimizer->functionArray;
    gcOPT_FUNCTION      function;
	gcOPT_TEMP			tempArray = Optimizer->tempArray;
	gcOPT_TEMP			temp;
	gcOPT_LIST			list;
	gctUINT16			source;
	gctUINT16			index;
	gcOPT_CODE          code;
    gctBOOL             updated;
	gctUINT				i;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	if (Optimizer->global == gcvNULL)
	{
		gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}

    if ( Optimizer->functionCount > 0 &&
         tempArray == gcvNULL)
    {
        gcmASSERT(tempArray);
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    function = functionArray;
    for (i = 0; i < Optimizer->functionCount; i++, function++)
	{
		/* Reset tempInt, used as direction. */
		for (list = Optimizer->global; list; list = list->next)
		{
			tempArray[list->index].tempInt = -1;
		}

		/* Set direction in tempInt. */
        for (code = function->codeHead; code != gcvNULL && code != function->codeTail->next; code = code->next)
		{
            switch (code->instruction.opcode)
			{
			case gcSL_CALL:
			case gcSL_JMP:
			case gcSL_RET:
			case gcSL_NOP:
			case gcSL_KILL:
				/* Skip control flow instructions. */
				temp = gcvNULL;
				break;

			case gcSL_TEXBIAS:
			case gcSL_TEXGRAD:
			case gcSL_TEXLOD:
				/* Skip texture state instructions. */
				temp      = gcvNULL;
				break;

			default:
				/* Get pointer to temporary register. */
				temp = tempArray + code->instruction.tempIndex;

				if (temp->isGlobal)
				{
                    if (code->instruction.opcode != gcSL_STORE)
                    {
					    if (temp->tempInt == -1)
					    {
						    temp->tempInt = gcvFUNCTION_OUTPUT;
					    }
					    else if (temp->tempInt == gcvFUNCTION_INPUT)
					    {
						    temp->tempInt = gcvFUNCTION_INOUT;
					    }
                    }
                    else
                    {
					    if (temp->tempInt == -1)
					    {
						    temp->tempInt = gcvFUNCTION_INPUT;
					    }
					    else if (temp->tempInt == gcvFUNCTION_OUTPUT)
					    {
						    temp->tempInt = gcvFUNCTION_INOUT;
					    }
                    }
				}
				break;
			}

			/* Determine usage of source0. */
			source = code->instruction.source0;

			if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
			{
				/* Get pointer to temporary register. */
				temp = tempArray + code->instruction.source0Index;

				if (temp->isGlobal)
				{
					if (temp->tempInt == -1)
					{
						temp->tempInt = gcvFUNCTION_INPUT;
					}
					else if (temp->tempInt == gcvFUNCTION_OUTPUT)
					{
						temp->tempInt = gcvFUNCTION_INOUT;
					}
				}
			}

			if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
			{
				index = code->instruction.source0Indexed;

				/* Get pointer to temporary register. */
				temp = tempArray + gcmSL_INDEX_GET(index, Index);

				/* Mark the temporary register is used as an index. */
				temp->isIndex = gcvTRUE;

				if (temp->isGlobal)
				{
					if (temp->tempInt == -1)
					{
						temp->tempInt = gcvFUNCTION_INPUT;
					}
					else if (temp->tempInt == gcvFUNCTION_OUTPUT)
					{
						temp->tempInt = gcvFUNCTION_INOUT;
					}
				}
			}


			/* Determine usage of source1. */
			source = code->instruction.source1;

			if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
			{
				/* Get pointer to temporary register. */
				temp = tempArray + code->instruction.source1Index;

				if (temp->isGlobal)
				{
					if (temp->tempInt == -1)
					{
						temp->tempInt = gcvFUNCTION_INPUT;
					}
					else if (temp->tempInt == gcvFUNCTION_OUTPUT)
					{
						temp->tempInt = gcvFUNCTION_INOUT;
					}
				}

			}

			if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
			{
				index = code->instruction.source1Indexed;

				/* Get pointer to temporary register. */
				temp = tempArray + gcmSL_INDEX_GET(index, Index);

				/* Mark the temporary register is used as an index. */
				temp->isIndex = gcvTRUE;

				if (temp->isGlobal)
				{
					if (temp->tempInt == -1)
					{
						temp->tempInt = gcvFUNCTION_INPUT;
					}
					else if (temp->tempInt == gcvFUNCTION_OUTPUT)
					{
						temp->tempInt = gcvFUNCTION_INOUT;
					}
				}
			}
		}

		/* Add global usage list to the function. */
		for (list = Optimizer->global; list; list = list->next)
		{
			temp = tempArray + list->index;

			if (temp->tempInt != -1)
			{
				gcOPT_GLOBAL_USAGE usage;
				gcmERR_RETURN(_CAllocateGlobalUsage(Optimizer->usageMemPool, &usage));

				usage->index = list->index;
				usage->direction = (gceINPUT_OUTPUT) temp->tempInt;
				usage->next = function->globalUsage;
				function->globalUsage = usage;
			}
		}
	}

    /* Propagate global usage for nested calling. */
    function = functionArray;
    for (i = 0; i < Optimizer->functionCount; i++, function++)
	{
        function->updated = gcvTRUE;
    }

    do
    {
        updated = gcvFALSE;
        function = functionArray;
        for (i = 0; i < Optimizer->functionCount; i++, function++)
	    {
            gcOPT_LIST caller;
            gcOPT_FUNCTION callerFunction;
		    gcOPT_GLOBAL_USAGE usage, callerUsage;

            if (! function->updated)
            {
                continue;
            }

            function->updated = gcvFALSE;
            code = function->codeHead;
            for (caller = code->callers; caller; caller = caller->next)
            {
                callerFunction = caller->code->function;
                if (callerFunction)
                {
                    /* Not main function, propagate and merge global usage. */
                    for (usage = function->globalUsage; usage; usage = usage->next)
                    {
                        for (callerUsage = callerFunction->globalUsage;
                             callerUsage;
                             callerUsage = callerUsage->next)
                        {
                            if (callerUsage->index == usage->index)
                            {
                                if (callerUsage->direction != usage->direction)
                                {
                                    if (callerUsage->direction != gcvFUNCTION_INOUT)
                                    {
                                        callerUsage->direction = gcvFUNCTION_INOUT;

                                        updated = callerFunction->updated = gcvTRUE;
                                    }
                                }
                                break;
                            }
                        }

                        if (callerUsage == gcvNULL)
                        {
                            /* Add global usage. */
				            gcmERR_RETURN(_CAllocateGlobalUsage(Optimizer->usageMemPool, &callerUsage));

                            callerUsage->index = usage->index;
                            callerUsage->direction = usage->direction;
                            callerUsage->next = callerFunction->globalUsage;
				            callerFunction->globalUsage = callerUsage;

                            updated = callerFunction->updated = gcvTRUE;
                        }
                    }
                }
            }
        }
    }
    while (updated);

	gcmFOOTER();
	return status;
}

static gceSTATUS
_BuildCodeList(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
    gcSHADER			shader = Optimizer->shader;
    gcSL_INSTRUCTION	shaderCode = shader->code;
    gctUINT             codeCount = shader->codeCount + 1;
    gcOPT_CODE          code, codePrev, codeNext;
	gctUINT				i;

    gcmHEADER_ARG("Optimizer=0x%x", Optimizer);

	/* Allocate an array for code list. */
    /* Allocate one more code for the extra RET for main. */
	gcmERR_RETURN(_CAllocateCodeArray(
        Optimizer->codeArrayMemPool,
        &Optimizer->codeHead, codeCount));

	/* Build code list. */
    codePrev = gcvNULL;
    code = Optimizer->codeHead;
    for (i = 0; i < codeCount; i++)
    {
        /* Initialization. */
        code->id            = i;
        code->function      = gcvNULL;
        code->callers       = gcvNULL;
        code->callee        = gcvNULL;
        code->tempDefine    = gcvNULL;
        code->dependencies0 = gcvNULL;
        code->dependencies1 = gcvNULL;
        code->users         = gcvNULL;
        code->prevDefines   = gcvNULL;
        code->nextDefines   = gcvNULL;
#if GC_ENABLE_LOADTIME_OPT
        code->ltcArrayIdx   = -1;
#endif /* GC_ENABLE_LOADTIME_OPT */

        /* Copy instruction. */
        code->instruction   = *shaderCode;

        /* Set links. */
        code->prev          = codePrev;
        codeNext            = code + 1;
        code->next          = codeNext;

        /* Move to next instruction. */
        shaderCode++;
        codePrev = code;
        code = codeNext;
    }

    /* Set the extra RET code. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(&codePrev->instruction, gcvINSTR_SIZE));
    codePrev->instruction.opcode = gcSL_RET;
    codePrev->next = gcvNULL;
    Optimizer->codeTail = codePrev;

    /* Initialize caller and callee. */
    for (code = Optimizer->codeHead; code; code = code->next)
    {
        if (code->instruction.opcode == gcSL_CALL
        ||  code->instruction.opcode == gcSL_JMP)
        {
            /* Add caller and callee. */
            gctUINT jumpTarget = code->instruction.tempIndex;
            gcOPT_CODE codeTarget = Optimizer->codeHead + jumpTarget;

            gcmASSERT(codeTarget->id == jumpTarget);
            code->callee = codeTarget;
            if (jumpTarget < code->id)
            {
                code->backwardJump = gcvTRUE;
            }

            gcmERR_RETURN(gcOpt_AddCodeToList(Optimizer, &codeTarget->callers, code));
        }
    }

    gcmFOOTER();
	return status;
}

void
gcOpt_UpdateCodeId(
	IN gcOPTIMIZER		Optimizer
	)
{
    gcOPT_CODE          code;
	gctUINT				i;

    for (i = 0, code = Optimizer->codeHead;
         /*i < Optimizer->codeCount &&*/ code;
         i++, code = code->next)
    {
        if (code->id != i)
        {
            if (code->callers)
            {
                gcOPT_LIST list;

                for (list = code->callers; list; list = list->next)
                {
                    gcOPT_CODE caller = list->code;
                    gcmASSERT(caller->instruction.opcode == gcSL_CALL
                           || caller->instruction.opcode == gcSL_JMP);
                    /* Cannot check this due to removeNOP. */
                    /*gcmASSERT(caller->instruction.tempIndex == code->id);*/
                    caller->instruction.tempIndex = (gctUINT16) i;
                }
            }

            code->id = i;
        }
    }
    /*gcmASSERT(i == Optimizer->codeCount && code == gcvNULL);*/
}

/*******************************************************************************
**							gcOpt_MoveCodeListBefore
********************************************************************************
**
**	Move a piece of code to another location.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
**
**		gcOPT_CODE SrcCodeHead
**			Start point of instructions to be moved.
**
**		gcOPT_CODE SrcCodeTail
**			End point of instructions to be moved.
**
**		gcOPT_CODE DestCode
**			Destination code.
*/
void
gcOpt_MoveCodeListBefore(
	IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       SrcCodeHead,
    IN gcOPT_CODE       SrcCodeTail,
    IN gcOPT_CODE       DestCode
	)
{
    /* Unlink the list from the original place. */
    if (SrcCodeTail->next)
    {
        SrcCodeTail->next->prev = SrcCodeHead->prev;
    }
    else
    {
        gcmASSERT(Optimizer->codeTail == SrcCodeTail);
        Optimizer->codeTail = SrcCodeHead->prev;
    }
    if (SrcCodeHead->prev)
    {
        SrcCodeHead->prev->next = SrcCodeTail->next;
    }
    else
    {
        gcmASSERT(Optimizer->codeHead == SrcCodeHead);
        Optimizer->codeHead = SrcCodeTail->next;
    }

    /* Link the list to destination place. */
    SrcCodeHead->prev = DestCode->prev;
    SrcCodeTail->next = DestCode;

    /* Update destination links. */
    if (DestCode->prev)
    {
        DestCode->prev->next = SrcCodeHead;
    }
    else
    {
        gcmASSERT(Optimizer->codeHead == DestCode);
        Optimizer->codeHead = SrcCodeHead;
    }
    DestCode->prev = SrcCodeTail;
}

void
gcOpt_MoveCodeListAfter(
	IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       SrcCodeHead,
    IN gcOPT_CODE       SrcCodeTail,
    IN gcOPT_CODE       DestCode
	)
{
    /* Unlink the list from the original place. */
    if (SrcCodeTail->next)
    {
        SrcCodeTail->next->prev = SrcCodeHead->prev;
    }
    else
    {
        gcmASSERT(Optimizer->codeTail == SrcCodeTail);
        Optimizer->codeTail = SrcCodeHead->prev;
    }
    if (SrcCodeHead->prev)
    {
        SrcCodeHead->prev->next = SrcCodeTail->next;
    }
    else
    {
        gcmASSERT(Optimizer->codeHead == SrcCodeHead);
        Optimizer->codeHead = SrcCodeTail->next;
    }

    /* Link the list to destination place. */
    SrcCodeHead->prev = DestCode;
    SrcCodeTail->next = DestCode->next;

    /* Update destination links. */
    if (DestCode->next)
    {
        DestCode->next->prev = SrcCodeTail;
    }
    else
    {
        gcmASSERT(Optimizer->codeTail == DestCode);
        Optimizer->codeTail = SrcCodeTail;
    }
    DestCode->next = SrcCodeHead;
}

/*******************************************************************************
**							gcOpt_CopyCodeListAfter
********************************************************************************
**
**	Copy a piece of code to another location.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
**
**		gcOPT_CODE SrcCodeHead
**			Start point of code to be moved.
**
**		gcOPT_CODE SrcCodeTail
**			End point of code to be moved.
**
**		gcOPT_CODE DestCode
**			Destination code.
*/
gceSTATUS
gcOpt_CopyCodeListAfter(
	IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       SrcCodeHead,
    IN gcOPT_CODE       SrcCodeTail,
    IN gcOPT_CODE       DestCode
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
    gcOPT_CODE          srcCode, code = gcvNULL;
    gcOPT_CODE          codeNext = DestCode->next;
    gcOPT_CODE          srcCodePrev = SrcCodeHead->prev;
    gcOPT_CODE          destCodeNext = DestCode->next;
    gcOPT_CODE          codePrev;

    gcmHEADER_ARG("Optimizer=0x%x SrcCodeHead=0x%x SrcCodeTail=0x%x DestCode=0x%x",
                   Optimizer, SrcCodeHead, SrcCodeTail, DestCode);

    /* The following update is done in gcOpt_ExpandFunctions temporarily to avoid assertion. */
    /* Need to update code id for checking. */
    /*gcOpt_UpdateCodeId(Optimizer);*/

    /* Copy code. */
    /* Use prev pointers as temp storage to link srcCode and code. */
    for (srcCode = SrcCodeTail; srcCode != gcvNULL && srcCode != srcCodePrev; srcCode = codePrev)
    {
        codePrev = srcCode->prev;
        if (Optimizer->freeCodeList)
        {
            code = Optimizer->freeCodeList;
            Optimizer->freeCodeList = code->next;
        }
        else
        {
	        /* Allocate a new gcOPT_CODE structure. */
	        gcmERR_RETURN(_CAllocateCode(Optimizer->codeMemPool, &code));
        }

        code->function      = DestCode->function;
        /*code->callers       = gcvNULL;
        code->callee        = gcvNULL;
        code->tempDefine    = gcvNULL;
        code->dependencies0 = gcvNULL;
        code->dependencies1 = gcvNULL;
        code->users         = gcvNULL;
        code->prevDefines   = gcvNULL;
        code->nextDefines   = gcvNULL;*/
        code->instruction   = srcCode->instruction;

        if (code->instruction.opcode == gcSL_CALL)
        {
            /* Function definition should not be part of copied code. */
            gcmASSERT(srcCode->callee->id < SrcCodeHead->id || srcCode->callee->id > SrcCodeTail->id);
            code->callee = srcCode->callee;
            gcmERR_RETURN(gcOpt_AddCodeToList(Optimizer, &srcCode->callee->callers, code));
        }

        /* Set temp links between srcCode and code using prev. */
        srcCode->prev = code;
        code->prev = srcCode;

        /* Set links. */
        code->next          = codeNext;
        codeNext            = code;
    }
    DestCode->next = code;

    /* Set callers and callee for JUMP instructions. */
    for (code = DestCode->next; code != gcvNULL&&code != destCodeNext; code = code->next)
    {
        if (code->instruction.opcode == gcSL_JMP)
        {
            srcCode = code->prev;
            if (srcCode->callee->id >= SrcCodeHead->id && srcCode->callee->id <= SrcCodeTail->id)
            {
                /* Target is inside the copied code--add code to the real callee's callers list. */
                code->callee = srcCode->callee->prev;
                gcmERR_RETURN(gcOpt_AddCodeToList(Optimizer, &code->callee->callers, code));
            }
            else
            {
                /* Target is outside the copied code--add code to callee's callers list. */
                code->callee = srcCode->callee;
                gcmERR_RETURN(gcOpt_AddCodeToList(Optimizer, &code->callee->callers, code));
            }
        }
    }

    /* Restore prev pointers back. */
    codePrev = srcCodePrev;
    for (code = SrcCodeHead; code != gcvNULL && code != SrcCodeTail->next; code = code->next)
    {
        code->prev = codePrev;
        codePrev = code;
    }

    codePrev = DestCode;
    for (code = DestCode->next; code != gcvNULL && code != destCodeNext; code = code->next)
    {
        code->prev = codePrev;
        codePrev = code;
    }
    if (destCodeNext)
    {
        destCodeNext->prev = codePrev;
    }

    /* Note that no housekeeping work done here. */
    /* The caller is responsible to update dependencies. */

    gcmFOOTER();
    return gcvSTATUS_OK;
}

gceSTATUS
gcOpt_RemoveCodeList(
	IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       CodeHead,
    IN gcOPT_CODE       CodeTail
	)
{
    gcOPT_CODE          code;

    gcmHEADER_ARG("Optimizer=0x%x CodeHead=0x%x CodeTail=0x%x",
                   Optimizer, CodeHead, CodeTail);

    /* Unlink the list from the original place. */
    if (CodeHead->prev)
    {
        CodeHead->prev->next = CodeTail->next;
    }
    else
    {
        gcmASSERT(Optimizer->codeHead == CodeHead);
        Optimizer->codeHead = CodeTail->next;
    }
    if (CodeTail->next)
    {
        CodeTail->next->prev = CodeHead->prev;
    }
    else
    {
        gcmASSERT(Optimizer->codeTail == CodeTail);
        Optimizer->codeTail = CodeHead->prev;
    }

    /* Remove reference from callers. */
    for (code = CodeHead; code != gcvNULL && code != CodeTail->next; code = code->next)
    {
        if (code->callee)
        {
            gcOPT_LIST list;
            gcOPT_LIST prevList = gcvNULL;
            gceSTATUS  status;

            for (list = code->callee->callers; list; list = list->next)
            {
                if (list->code == code)
                {
                    break;
                }
                prevList = list;
            }

			if (! list)
			{
                gcmASSERT(list);
                status = gcvSTATUS_INVALID_ARGUMENT;
                gcmFOOTER();
                return status;
            }

            if (prevList)
            {
                prevList->next = list->next;
            }
            else
            {
                code->callee->callers = list->next;
            }

	        /* Free the gcOPT_LIST structure. */
	        gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
        }
    }

    /* Add the list to the freeCodeList. */
    CodeTail->next = Optimizer->freeCodeList;
    Optimizer->freeCodeList = CodeHead;

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

/*******************************************************************************
**							   _BuildFunctionArray
********************************************************************************
**
**	Build optimizer's function array.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
static gceSTATUS
_BuildFunctionArray(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcSHADER			shader = Optimizer->shader;
	gcOPT_FUNCTION		functionArray;
	gcOPT_FUNCTION		function;
#if __SUPPORT_OCL__
	gcFUNCTION          shaderFunction;
    gcKERNEL_FUNCTION   kernelFunction;
	gctUINT				i, j, k;
#else
	gctUINT				i;
#endif
    gcOPT_CODE          codeHead = Optimizer->codeHead;
    gcOPT_CODE          code;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	gcmASSERT(Optimizer->main == gcvNULL);

	/* Allocate main program data structure. */
	if (Optimizer->main == gcvNULL)
	{
		gcmERR_RETURN(_CAllocateFunctionArray(Optimizer->functionArrayMemPool, &Optimizer->main, 1));
	}

	gcmASSERT(Optimizer->functionArray == gcvNULL);

	if (Optimizer->functionCount == 0)
	{
		gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}

	/* Allocate function array data structure. */
	gcmERR_RETURN(_CAllocateFunctionArray(Optimizer->functionArrayMemPool, &functionArray, Optimizer->functionCount));

	Optimizer->functionArray = functionArray;

	/* Initialize functionArray's data struture. */
	function = functionArray;

#if __SUPPORT_OCL__
    /* Merge both function array. */
    i = j = 0;
    shaderFunction = shader->functionCount ? shader->functions[0] : gcvNULL;
    kernelFunction = shader->kernelFunctionCount ? shader->kernelFunctions[0] : gcvNULL;
    for (k = 0; k < Optimizer->functionCount; k++, function++)
    {
        if (j >= shader->kernelFunctionCount
        ||  (i < shader->functionCount  && shaderFunction->codeStart < kernelFunction->codeStart))
        {
		    function->shaderFunction = shaderFunction;
		    function->argumentCount = shaderFunction->argumentCount;
		    function->arguments = shaderFunction->arguments;
		    function->globalUsage = gcvNULL;

            /* At beginning, codeHead is sequential in an array. */
		    function->codeHead = codeHead + shaderFunction->codeStart;
		    function->codeTail = codeHead + shaderFunction->codeStart + shaderFunction->codeCount - 1;

            /* Set function pointer. */
		    for (code = function->codeHead;
                 code != gcvNULL && code != function->codeTail->next;
                 code = code->next)
		    {
			    code->function = function;
		    }

            /* Move to next shader function. */
            i++;
            if (i < shader->functionCount)
            {
                shaderFunction = shader->functions[i];
            }
        }
        else
        {
            gcmASSERT(i >= shader->functionCount ||
                      shaderFunction->codeStart > kernelFunction->codeStart);

		    function->kernelFunction = kernelFunction;
		    function->argumentCount = kernelFunction->argumentCount;
		    function->arguments = kernelFunction->arguments;
		    function->globalUsage = gcvNULL;

            /* At beginning, codeHead is sequential in an array. */
		    function->codeHead = codeHead + kernelFunction->codeStart;
            function->codeTail = codeHead + kernelFunction->codeEnd - 1;

            /* Set function pointer. */
		    for (code = function->codeHead;
                 code != gcvNULL && code != function->codeTail->next;
                 code = code->next)
		    {
			    code->function = function;
		    }

            /* Move to next shader function. */
            j++;
            if (j < shader->kernelFunctionCount)
            {
                kernelFunction = shader->kernelFunctions[j];
            }
        }
    }
#else
	for (i = 0; i < Optimizer->functionCount; i++, function++)
	{
		gcFUNCTION shaderFunction = shader->functions[i];

		function->shaderFunction = shaderFunction;
		function->argumentCount = shaderFunction->argumentCount;
		function->arguments = shaderFunction->arguments;
		function->globalUsage = gcvNULL;

        /* At beginning, codeHead is sequential in an array. */
		function->codeHead = codeHead + shaderFunction->codeStart;
		function->codeTail  = codeHead + shaderFunction->codeStart + shaderFunction->codeCount - 1;

        /* Set function pointer. */
		for (code = function->codeHead;
             code != gcvNULL && code != function->codeTail->next;
             code = code->next)
		{
			code->function = function;
		}
	}
#endif

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/*******************************************************************************
**							   _DestroyFunctionArray
********************************************************************************
**
**	Destroy optimizer's function array.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
static gceSTATUS
_DestroyFunctionArray(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_TEMP			tempArray = Optimizer->tempArray;
	gcOPT_FUNCTION		functionArray = Optimizer->functionArray;
	gcOPT_FUNCTION		function;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	if (Optimizer->main)
	{
		gcmVERIFY_OK(_FreeFunctionArray(Optimizer->functionArrayMemPool, Optimizer->main));

		Optimizer->main = gcvNULL;
	}

	if (functionArray == gcvNULL)
	{
		gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}

	for (function = functionArray + Optimizer->functionCount - 1;
		 function >= functionArray;
		 function--)
	{
		/* Free global variable usage list. */
		while (function->globalUsage)
		{
			gcOPT_GLOBAL_USAGE usage = function->globalUsage;
			function->globalUsage = usage->next;
			gcmVERIFY_OK(_FreeGlobalUsage(Optimizer->usageMemPool, usage));
		}

		if (tempArray)
		{
            gcsFUNCTION_ARGUMENT_PTR argument;
			gctUINT i;

			/* Update temps' function for function arguments. */
            argument = function->arguments;
			for (i = 0; i < function->argumentCount; i++, argument++)
			{
				gctUINT index = argument->index;

				gcmASSERT(tempArray[index].function == function);
				tempArray[index].function = gcvNULL;
                tempArray[index].argument = gcvNULL;
			}
		}
	}

	gcmVERIFY_OK(_FreeFunctionArray(Optimizer->functionArrayMemPool, functionArray));

	Optimizer->functionArray = gcvNULL;

	gcmFOOTER();
	return status;
}

/*******************************************************************************
**							   gcOpt_DeleteFunction
********************************************************************************
**
**	Delete one function from optimizer's function array.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
**
**		gcOPT_FUNCTION Function
**			Pointer to a gcOPT_FUNCTION structure to be deleted.
*/
gceSTATUS
gcOpt_DeleteFunction(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_FUNCTION	Function
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
    gcOPT_CODE          code;
	gcOPT_TEMP			tempArray = Optimizer->tempArray;
	gcOPT_FUNCTION		functionArray = Optimizer->functionArray;
	gctUINT				fIndex = (gctINT) (Function - functionArray);
	gctUINT				i, j;
    gcsFUNCTION_ARGUMENT_PTR argument;

	gcmHEADER_ARG("Optimizer=%p Function=%p", Optimizer, Function);

	gcmASSERT(functionArray);
	gcmASSERT(fIndex < Optimizer->functionCount);

	/* Update temps' function for function arguments. */
    argument = Function->arguments;
	for (j = 0; j < Function->argumentCount; j++, argument++)
	{
		gctUINT index = argument->index;

		gcmASSERT(tempArray[index].function == Function);
		tempArray[index].function = gcvNULL;
		tempArray[index].argument = gcvNULL;
	}

	/* Free global variable usage list. */
	while (Function->globalUsage)
	{
		gcOPT_GLOBAL_USAGE usage = Function->globalUsage;
		Function->globalUsage = usage->next;
		gcmVERIFY_OK(_FreeGlobalUsage(Optimizer->usageMemPool, usage));
	}

	/* Remove the function. */
    if (Function->codeHead)
    {
        gcOpt_RemoveCodeList(Optimizer, Function->codeHead, Function->codeTail);
    }

    for (j = fIndex; j < Optimizer->functionCount - 1; j++)
	{
        gcsFUNCTION_ARGUMENT_PTR argument;

		functionArray[j] = functionArray[j + 1];

		/* Update temps' function for function arguments. */
        argument = functionArray[j].arguments;
		for (i = 0; i < functionArray[j].argumentCount; i++, argument++)
		{
			gctUINT index = argument->index;

			gcmASSERT(tempArray[index].function == functionArray + j + 1);
			tempArray[index].function = functionArray + j;
            tempArray[index].argument = argument;
		}

        /* Update code's and callee's function */
        for (code = Optimizer->codeHead; code; code = code->next)
        {
            if (code->instruction.opcode == gcSL_CALL)
            {
                if (code->callee->function == &functionArray[j + 1])
                    code->callee->function = &functionArray[j];
            }

            if (code->function == &functionArray[j + 1])
                code->function = &functionArray[j];
        }
	}
	Optimizer->functionCount--;
	if (Optimizer->functionCount == 0)
	{
		/* Free the empty function array. */
		gcmVERIFY_OK(_FreeFunctionArray(Optimizer->functionArrayMemPool, functionArray));
		Optimizer->functionArray = gcvNULL;
	}

	/* Rebuild flow graph. */
	gcmVERIFY_OK(gcOpt_RebuildFlowGraph(Optimizer));

	gcmFOOTER();
	return status;
}

/*******************************************************************************
**							gcOpt_CopyInShader
********************************************************************************
**
**	Copy shader data into optimizer, and add an extra instruction, RET, at the
**  end of main program to avoid ambiguity and memory corruption (ABW and ABR).
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
**
**		gcSHADER Shader
**			Pointer to a gcSHADER object holding information about the compiled shader.
*/
gceSTATUS
gcOpt_CopyInShader(
	IN gcOPTIMIZER		Optimizer,
	IN gcSHADER			Shader
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcFUNCTION *		shaderFunctions = Shader->functions;
	gctBOOL				mainLast = gcvTRUE;

	gcmHEADER_ARG("Optimizer=%p Shader=%p", Optimizer, Shader);

	/*  Sort shader's functions and locate the main program. */
	if (Shader->functionCount > 0)
	{
		gctINT i, j;
		gcFUNCTION function;

		/* Check and sort functions to make sure they are in order. */
		for (i = Shader->functionCount - 1; i > 0; i--)
		{
			gctBOOL functionInOrder = gcvTRUE;
			for (j = 0; j < i; j++)
			{
				if (shaderFunctions[j]->codeStart > shaderFunctions[j + 1]->codeStart)
				{
					/* Out of order.  Swap it. */
					gcFUNCTION temp = shaderFunctions[j];
					gctUINT tempLabel = temp->label;
					gctUINT tempLabel1 = shaderFunctions[j + 1]->label;
					shaderFunctions[j] = shaderFunctions[j + 1];
					shaderFunctions[j + 1] = temp;
					shaderFunctions[j]->label = (gctUINT16) tempLabel;
					shaderFunctions[j + 1]->label = (gctUINT16) tempLabel1;
					functionInOrder = gcvFALSE;
				}
			}
			if (functionInOrder) break;
		}

		function = Shader->functions[Shader->functionCount - 1];
		if (function->codeStart + function->codeCount == Shader->codeCount)
		{
			mainLast = gcvFALSE;
		}
	}

#if __SUPPORT_OCL__
	/*  Sort shader's functions and locate the main program. */
	if (Shader->kernelFunctionCount > 0)
	{
		gctINT i, j;
    	gcKERNEL_FUNCTION * kernelFunctions = Shader->kernelFunctions;

		/* Check and sort functions to make sure they are in order. */
		for (i = Shader->kernelFunctionCount - 1; i > 0; i--)
		{
			gctBOOL functionInOrder = gcvTRUE;
			for (j = 0; j < i; j++)
			{
				if (kernelFunctions[j]->codeStart > kernelFunctions[j + 1]->codeStart)
				{
					/* Out of order.  Swap it. */
					gcKERNEL_FUNCTION temp = kernelFunctions[j];
					gctUINT tempLabel = temp->label;
					gctUINT tempLabel1 = kernelFunctions[j + 1]->label;
					kernelFunctions[j] = kernelFunctions[j + 1];
					kernelFunctions[j + 1] = temp;
					kernelFunctions[j]->label = (gctUINT16) tempLabel;
					kernelFunctions[j + 1]->label = (gctUINT16) tempLabel1;
					functionInOrder = gcvFALSE;
				}
			}
			if (functionInOrder) break;
		}

        if (mainLast)
        {
		    gcKERNEL_FUNCTION kernelFunction = Shader->kernelFunctions[Shader->kernelFunctionCount - 1];
            if (kernelFunction->codeEnd == Shader->codeCount)
		    {
			    mainLast = gcvFALSE;
		    }
        }
	}
#endif

	/* Initialize Optimizer. */
	Optimizer->shader			= Shader;
#if __SUPPORT_OCL__
	Optimizer->functionCount	= Shader->functionCount + Shader->kernelFunctionCount;
#else
	Optimizer->functionCount	= Shader->functionCount;
#endif
	Optimizer->outputCount		= Shader->outputCount;
	Optimizer->outputs			= Shader->outputs;

    /* Copy codes/instructions from shader to code list. */
	gcmERR_RETURN(_BuildCodeList(Optimizer));

	/* Allocate main and function array data structure. */
	gcmERR_RETURN(_BuildFunctionArray(Optimizer));

	if (mainLast)
	{
        /* Set main's codeHead and codeTail. */
        if (Optimizer->functionCount == 0)
        {
            Optimizer->main->codeHead = Optimizer->codeHead;
        }
        else
        {
            Optimizer->main->codeHead = Optimizer->functionArray[Optimizer->functionCount - 1].codeTail->next;
        }
        Optimizer->main->codeTail = Optimizer->codeTail;
	}
	else
	{
		gctUINT i;
		gcOPT_FUNCTION functionArray = Optimizer->functionArray;
        gcOPT_CODE codeRET;
        gcOPT_CODE codeNext;
        gcOPT_CODE code;

		/* Locate the last instruction of main program. */
		for (i = Optimizer->functionCount - 1; i > 0; i--)
		{
            if (functionArray[i].codeHead != functionArray[i - 1].codeTail->next)
				break;
		}

        /* Move the RET instruction from last to after main's last instruction. */
        codeRET  = Optimizer->codeTail;
        codeNext = functionArray[i].codeHead;

        /* Unlink codeRET from the end of codeHead. */
        codeRET->prev->next = gcvNULL;
        Optimizer->codeTail = codeRET->prev;

        /* Insert codeRET to before codeNext. */
        codeRET->next = codeNext;
        codeRET->prev = codeNext->prev;
        codeNext->prev->next = codeRET;
        codeNext->prev = codeRET;

        /* Set main's codeHead and codeTail. */
        if (i == 0)
        {
            Optimizer->main->codeHead = Optimizer->codeHead;
        }
        else
        {
            Optimizer->main->codeHead = functionArray[i - 1].codeTail->next;
        }
        Optimizer->main->codeTail = codeRET;

        /* Update code's id from codeRET. */
        codeRET->id = codeNext->id;
        for (code = codeNext; code; code = code->next)
        {
            if (code->callers)
            {
                gcOPT_LIST list;
                for (list = code->callers; list; list = list->next)
                {
                    gcOPT_CODE caller = list->code;
                    gcmASSERT(caller->instruction.opcode == gcSL_CALL
                           || caller->instruction.opcode == gcSL_JMP);
                    gcmASSERT(caller->instruction.tempIndex == code->id);
                    caller->instruction.tempIndex++;
                }
            }
            code->id++;
        }
	}

	/* Shader's labels should be empty. */
	gcmASSERT(Shader->labels == gcvNULL);

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/*******************************************************************************
**							gcOpt_CopyOutShader
********************************************************************************
**
**	Copy shader data from optimizer back to shader, and change the extra RET
**  instruction to NOP instruction.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
**
**		gcSHADER Shader
**			Pointer to a gcSHADER object holding information about the compiled shader.
*/
gceSTATUS
gcOpt_CopyOutShader(
	IN gcOPTIMIZER		Optimizer,
	IN gcSHADER			Shader
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcoOS				os = gcvNULL;
	gctUINT				codeCount = Optimizer->codeTail->id + 1;
    gcOPT_CODE          srcCode;
	gcSL_INSTRUCTION	code;
	gctUINT				pc;
	gctUINT				i;

	gcmHEADER_ARG("Optimizer=%p Shader=%p", Optimizer, Shader);


	if (Shader->codeCount != codeCount)
	{
	    gctPOINTER pointer = gcvNULL;
        gctSIZE_T Bytes;

		/* Free the old code array. */
		gcmVERIFY_OK(gcmOS_SAFE_FREE(os, Shader->code));

		/* Allocate new code array for the shader. */
        Bytes = (gctSIZE_T)((gctSIZE_T)codeCount * gcvINSTR_SIZE);
		gcmERR_RETURN(gcoOS_Allocate(os,
							Bytes,
							&pointer));

		/* Switch over to new code array. */
		Shader->code = pointer;
		Shader->codeCount = codeCount;
		Shader->lastInstruction = codeCount;
	}

    code = Shader->code;
    for (srcCode = Optimizer->codeHead; srcCode; srcCode = srcCode->next, code++)
    {
        *code = srcCode->instruction;
    }

	/* Copy the function data back. */
	if (Optimizer->functionCount == 0)
	{
		if (Shader->functionCount > 0)
		{
			/* All functions are expanded, so delete the functions. */
			for (i = 0; i < Shader->functionCount; i++)
			{
				gcFUNCTION shaderFunction = Shader->functions[i];

				if (shaderFunction->arguments)
				{
					/* Free argument array. */
					gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction->arguments));
				}
				gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction));
			}

			/* Free the function array. */
			gcmVERIFY_OK(gcmOS_SAFE_FREE(os, Shader->functions));

			Shader->functionCount = 0;
		}
#if __SUPPORT_OCL__
		if (Shader->kernelFunctionCount > 0)
		{
            gcOPT_FUNCTION function;
            gcKERNEL_FUNCTION kernelFunction;

            gcmASSERT(Optimizer->isMainMergeWithKerenel);

			/* All functions are expanded except the main kernel function. */
			for (i = 0; i < Shader->kernelFunctionCount; i++)
			{
				kernelFunction = Shader->kernelFunctions[i];

                if (kernelFunction == Optimizer->main->kernelFunction)
                {
                    continue;
                }

				if (kernelFunction->arguments)
				{
					/* Free argument array. */
					gcmVERIFY_OK(gcmOS_SAFE_FREE(os, kernelFunction->arguments));
				}
				gcmVERIFY_OK(gcmOS_SAFE_FREE(os, kernelFunction));
			}

			/* Free the function array. */
            function = Optimizer->main;
            kernelFunction = function->kernelFunction;
            Shader->kernelFunctions[0] = kernelFunction;
	        kernelFunction->label = (gctUINT16) (~0);
            kernelFunction->codeStart = function->codeHead->id;
            kernelFunction->codeCount = function->codeTail->id - function->codeHead->id + 1;
            kernelFunction->codeEnd   = function->codeTail->id + 1;
            kernelFunction->isMain    = gcvTRUE;

			Shader->kernelFunctionCount = 1;
		}
#endif
	}
	else
	{
#if __SUPPORT_OCL__
		gcOPT_FUNCTION function = Optimizer->functionArray;
		gcFUNCTION shaderFunction;
		gcKERNEL_FUNCTION kernelFunction;
		gctUINT j = 0, k = 0;
		gctUINT j1 = 0, k1 = 0;

        shaderFunction = Shader->functionCount ? Shader->functions[0] : gcvNULL;
        kernelFunction = Shader->kernelFunctionCount ? Shader->kernelFunctions[0] : gcvNULL;
		gcmASSERT(Optimizer->functionCount <= Shader->functionCount + Shader->kernelFunctionCount);

		for (i = 0; i < Optimizer->functionCount; i++, function++)
		{
			gcmASSERT(i <= j + k);
            if (function->shaderFunction)
            {
			    while (shaderFunction != gcvNULL && shaderFunction != function->shaderFunction)
			    {
				    /* Free the expanded function. */
				    if (shaderFunction->arguments)
				    {
					    /* Free argument array. */
					    gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction->arguments));
				    }
				    gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction));
				    Shader->functions[j] = gcvNULL;

				    j++;
				    gcmASSERT(j < Shader->functionCount);
				    shaderFunction = Shader->functions[j];
			    }
			    gcmASSERT(shaderFunction != gcvNULL);
			    /* Copy new function data back to Shader. */
			    gcmASSERT(shaderFunction->argumentCount == function->argumentCount);
			    gcmASSERT(shaderFunction->arguments == function->arguments);
			    if (i == j)
			    {
				    gcmASSERT(shaderFunction->label == (gctUINT16) (~0 - i));
			    }
			    else
			    {
				    shaderFunction->label = (gctUINT16) (~0 - i);
			    }
			    if (j != j1)
			    {
				    Shader->functions[j1] = shaderFunction;
				    Shader->functions[j] = gcvNULL;
			    }
                shaderFunction->codeStart = function->codeHead->id;
                shaderFunction->codeCount = function->codeTail->id - function->codeHead->id + 1;
			    j++;
                j1++;
                shaderFunction = j < Shader->functionCount ? Shader->functions[j] : gcvNULL;
            }
            else
            {
                gcmASSERT(function->kernelFunction);
			    while (kernelFunction != gcvNULL && kernelFunction != function->kernelFunction)
			    {
                    if (kernelFunction != Optimizer->main->kernelFunction)
                    {
				        /* Free the expanded function. */
				        if (kernelFunction->arguments)
				        {
					        /* Free argument array. */
					        gcmVERIFY_OK(gcmOS_SAFE_FREE(os, kernelFunction->arguments));
				        }
				        gcmVERIFY_OK(gcmOS_SAFE_FREE(os, kernelFunction));
				        Shader->kernelFunctions[k] = gcvNULL;
                    }

				    k++;
				    gcmASSERT(k < Shader->kernelFunctionCount);
				    kernelFunction = Shader->kernelFunctions[k];
			    }
			    gcmASSERT(kernelFunction != gcvNULL);
			    /* Copy new function data back to Shader. */
			    gcmASSERT(kernelFunction->argumentCount == function->argumentCount);
			    gcmASSERT(kernelFunction->arguments == function->arguments);
			    if (i == k)
			    {
				    gcmASSERT(kernelFunction->label == (gctUINT16) (~0 - i));
			    }
			    else
			    {
				    kernelFunction->label = (gctUINT16) (~0 - i);
			    }
			    if (k != k1)
			    {
				    Shader->kernelFunctions[k1] = kernelFunction;
				    Shader->kernelFunctions[k] = gcvNULL;
			    }
                kernelFunction->codeStart = function->codeHead->id;
                kernelFunction->codeCount = function->codeTail->id - function->codeHead->id + 1;
                kernelFunction->codeEnd = function->codeTail->id + 1;
                k++;
			    k1++;
                kernelFunction = k < Shader->kernelFunctionCount ? Shader->kernelFunctions[k] : gcvNULL;
            }

            gcmASSERT(i == j1 + k1 - 1);
		}

        gcmASSERT(Optimizer->functionCount == j1 + k1);

		/* Free the rest of shader functions. */
		while (j < Shader->functionCount)
		{
			shaderFunction = Shader->functions[j];

			/* Free the expanded function. */
			if (shaderFunction->arguments)
			{
				/* Free argument array. */
				gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction->arguments));
			}
			gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction));
			Shader->functions[j] = gcvNULL;

			j++;
		}

        if (j1 == 0 && Shader->functionCount > 0)
        {
			/* Free the function array. */
			gcmVERIFY_OK(gcmOS_SAFE_FREE(os, Shader->functions));
        }

		Shader->functionCount = j1;

		/* Free the rest of shader kernel functions. */
		while (k < Shader->kernelFunctionCount)
		{
			kernelFunction = Shader->kernelFunctions[k];

            if (kernelFunction != Optimizer->main->kernelFunction)
            {
			    /* Free the expanded function. */
			    if (kernelFunction->arguments)
			    {
				    /* Free argument array. */
				    gcmVERIFY_OK(gcmOS_SAFE_FREE(os, kernelFunction->arguments));
			    }
			    gcmVERIFY_OK(gcmOS_SAFE_FREE(os, kernelFunction));
            }
			Shader->kernelFunctions[k] = gcvNULL;

			k++;
		}

        function = Optimizer->main;
        kernelFunction = function->kernelFunction;
        if (kernelFunction)
        {
	        Shader->kernelFunctions[k1] = kernelFunction;
	        kernelFunction->label = (gctUINT16) (~0 - i);
            kernelFunction->codeStart = function->codeHead->id;
            kernelFunction->codeCount = function->codeTail->id - function->codeHead->id + 1;
            kernelFunction->codeEnd   = function->codeTail->id + 1;
            kernelFunction->isMain    = gcvTRUE;

		    Shader->kernelFunctionCount = k1 + 1;
        }
        else
        {
		    Shader->kernelFunctionCount = k1;
        }
#else
		gcOPT_FUNCTION function = Optimizer->functionArray;
		gcFUNCTION shaderFunction = Shader->functions[0];
		gctUINT j = 0;

		gcmASSERT(Optimizer->functionCount <= Shader->functionCount);
		for (i = 0; i < Optimizer->functionCount; i++, function++)
		{
			gcmASSERT(i <= j);
			while (shaderFunction != gcvNULL && shaderFunction != function->shaderFunction)
			{
				/* Free the expanded function. */
				if (shaderFunction->arguments)
				{
					/* Free argument array. */
					gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction->arguments));
				}
				gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction));
				Shader->functions[j] = gcvNULL;

				j++;
				gcmASSERT(j < Shader->functionCount);
				shaderFunction = Shader->functions[j];
			}
			gcmASSERT(shaderFunction != gcvNULL);
			/* Copy new function data back to Shader. */
			gcmASSERT(shaderFunction->argumentCount == function->argumentCount);
			gcmASSERT(shaderFunction->arguments == function->arguments);
			if (i == j)
			{
				gcmASSERT(shaderFunction->label == (gctUINT16) (~0 - i));
			}
			else
			{
				Shader->functions[i] = shaderFunction;
				Shader->functions[j] = gcvNULL;
				shaderFunction->label = (gctUINT16) (~0 - i);
			}
            shaderFunction->codeStart = function->codeHead->id;
            shaderFunction->codeCount = function->codeTail->id - function->codeHead->id + 1;
			j++;
			shaderFunction = Shader->functions[j];
		}

		/* Free the rest of shader functions. */
		while (j < Shader->functionCount)
		{
			shaderFunction = Shader->functions[j];

			/* Free the expanded function. */
			if (shaderFunction->arguments)
			{
				/* Free argument array. */
				gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction->arguments));
			}
			gcmVERIFY_OK(gcmOS_SAFE_FREE(os, shaderFunction));
			Shader->functions[j] = gcvNULL;

			j++;
		}

		Shader->functionCount = Optimizer->functionCount;
#endif
	}

	/* Locate the last instruction of main program. */
    pc = Optimizer->main->codeTail->id;

	/* Change the last instruction of main program to NOP if it is RET. */
	code = Shader->code + pc;
	if (code->opcode == gcSL_RET)
	{
		/* Make the instruction NOP. */
        *code = gcvSL_NOP_INSTR;
	}
	else
	{
		gcmASSERT(code->opcode == gcSL_RET);
	}

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/*******************************************************************************
**							_PackMainProgram
********************************************************************************
**
**	Set an instruction as removed by changing it to NOP instruction
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
**
**      gcOPT_CODE       code
**          Pointer to a code to be removed
**
**  OUTPUT:
**
**  If successfully changed, return TRUE, else FALSE
*/
static gctBOOL
_RemoveInstByChangeToNOP(
    IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       code
    )
{
    gctBOOL				changeCode = gcvFALSE;

    if (code->instruction.opcode != gcSL_NOP)
    {
        /* For jump, we need update its target's info before we change it to
        ** NOP. */
        if (code->instruction.opcode == gcSL_JMP)
        {
            gcOPT_LIST caller;
            gcmASSERT(code->callee);

            /* Callee's caller. */
            for (caller = code->callee->callers; caller != gcvNULL; caller = caller->next)
            {
                if (caller->code == code)
                {
                    gcOpt_DeleteCodeFromList(Optimizer,
                                             &code->callee->callers,
                                             caller->code);
                    break;
                }
            }

            /* Callee. */
            code->callee = gcvNULL;
        }

	    code->instruction = gcvSL_NOP_INSTR;
	    changeCode = gcvTRUE;
    }

    return changeCode;
}

/*******************************************************************************
**							_PackMainProgram
********************************************************************************
**
**	Pact main program if main program is not in one chunk, and initialize
**	main program's codeStart, etc.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
static gceSTATUS
_PackMainProgram(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_FUNCTION		functionArray = Optimizer->functionArray;
	gcOPT_FUNCTION		function;
    gcOPT_CODE          code;
    gcOPT_CODE          codeNext;
	gctUINT				i;
	gctINT				k;
	gctBOOL				changeCode = gcvFALSE;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	if (Optimizer->functionCount == 0)
	{
		/* Initialize main program data struture. */
		gcmASSERT(functionArray == gcvNULL);
        gcmASSERT(Optimizer->main->codeHead == Optimizer->codeHead);
        gcmASSERT(Optimizer->main->codeTail  == Optimizer->codeTail);

		gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}

	/* Remove unreachable code, especially for jump-out-of-bound JMP in functions. */
	/* And check to make sure no jump-out-of-bound JMP in functions. */
	for (i = 0; i < Optimizer->functionCount; i++)
	{
		function  = functionArray + i;
        code = function->codeHead;
        codeNext = function->codeTail->next;

        do
        {
            if (code->instruction.opcode == gcSL_JMP)
            {
                gcmASSERT(code->instruction.tempIndex >= function->codeHead->id
                    && code->instruction.tempIndex <= function->codeTail->id);

			    if (gcmSL_TARGET_GET(code->instruction.temp, Condition) != gcSL_ALWAYS)
                {
                    code = code->next;
                    continue;
                }

                code = code->next;
                while (code != gcvNULL && code != codeNext && code->callers == gcvNULL)
			    {
                    changeCode = _RemoveInstByChangeToNOP(Optimizer, code);
                    code = code->next;
			    }
            }
            else if (code->instruction.opcode == gcSL_RET)
            {
                code = code->next;
                while (code != gcvNULL && code != codeNext && code->callers == gcvNULL)
			    {
                    changeCode = _RemoveInstByChangeToNOP(Optimizer, code);
                    code = code->next;
			    }
			}
            else
            {
                code = code->next;
            }
        }
        while (code != gcvNULL && code != codeNext);
	}

	/* Remove unused functions. */
	for (k = Optimizer->functionCount - 1; k >= 0; k--)
	{
		function = functionArray + k;

		if (function->codeHead->callers == gcvNULL)
		{
			/* Function is not used at all. */
			/* Remove the function. */
			gcOPT_TEMP tempArray = Optimizer->tempArray;
            gcsFUNCTION_ARGUMENT_PTR argument;
			gctUINT j;

            /* Free the code list in the function. */
#if _REMOVE_UNUSED_FUNCTION_CODE_
            gcOpt_RemoveCodeList(Optimizer, function->codeHead, function->codeTail);
#else
            {
            for (code = function->codeHead; code != gcvNULL && code != function->codeTail->next; code = code->next)
            {
                if(code->instruction.opcode == gcSL_CALL)
                {
                    /* Remove from callee's caller list */
                    gcOpt_DeleteCodeFromList(Optimizer,
                                &code->callee->callers,
                                code);
                    code->callee = gcvNULL;
                }
                else
                {
                    /* All other instruction is inside this function, remove caller and callee */
                    gcOPT_LIST caller;
                    gcOPT_LIST nextCaller;

                    for(caller = code->callers ;caller != gcvNULL;)
                    {
                        nextCaller = caller->next;
                        _FreeList(Optimizer->listMemPool, caller);
                        caller = nextCaller;
                    }

                    code->callee = gcvNULL;
                    code->callers = gcvNULL;
                }

                code->instruction = gcvSL_NOP_INSTR;
                code->function = gcvNULL;
            }
            if (k > 0 && function->codeHead->prev == functionArray[k - 1].codeTail)
            {
                functionArray[k - 1].codeTail = function->codeTail;
            }
            else if (function->codeTail->next == Optimizer->main->codeHead)
            {
                if (k == 0)
                {
                    Optimizer->main->codeHead = Optimizer->codeHead;
                }
                else if (function->codeHead->prev != functionArray[k - 1].codeTail)
                {
                    Optimizer->main->codeHead = functionArray[k - 1].codeTail->next;
                }
                else
                {
                    Optimizer->main->codeHead = function->codeHead;
                }
            }
            }
#endif

			/* Update temps' function for function arguments. */
            argument = function->arguments;
			for (j = 0; j < function->argumentCount; j++, argument++)
			{
				gctUINT index = argument->index;

				gcmASSERT(tempArray[index].function == function);
				tempArray[index].function = gcvNULL;
                tempArray[index].argument = gcvNULL;
			}

			for (j = k; j < Optimizer->functionCount - 1; j++)
			{
				functionArray[j] = functionArray[j + 1];

				/* Update temps' function for function arguments. */
                argument = functionArray[j].arguments;
				for (i = 0; i < functionArray[j].argumentCount; i++, argument++)
				{
					gctUINT index = argument->index;

					gcmASSERT(tempArray[index].function == functionArray + j + 1);
					tempArray[index].function = functionArray + j;
					tempArray[index].argument = argument;
				}
			}
			Optimizer->functionCount--;
			if (Optimizer->functionCount == 0)
			{
				/* Free the empty function array. */
				gcmVERIFY_OK(_FreeFunctionArray(Optimizer->functionArrayMemPool, functionArray));
				Optimizer->functionArray = gcvNULL;
			}

			changeCode = gcvTRUE;
		}
	}

	if (changeCode)
	{
		/* Update code id. */
		gcOpt_UpdateCodeId(Optimizer);

        /* Update function pointer. */
        function = functionArray;
        for (i = 0; i < Optimizer->functionCount; i++, function++)
        {
            for (code = function->codeHead; code != gcvNULL && code != function->codeTail->next; code = code->next)
            {
                code->function = function;
            }
        }

        changeCode = gcvFALSE;
	}

	if (Optimizer->functionCount == 0)
	{
		/* Initialize main program data struture. */
		gcmASSERT(Optimizer->functionArray == gcvNULL);
        gcmASSERT(Optimizer->main->codeHead == Optimizer->codeHead);
        gcmASSERT(Optimizer->main->codeTail  == Optimizer->codeTail);
        /*Optimizer->main->codeHead = Optimizer->codeHead;
        Optimizer->main->codeTail  = Optimizer->codeTail;*/

		gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}

    /* Skip the functions after main. */
	function = functionArray + Optimizer->functionCount - 1;
    for (k = Optimizer->functionCount - 1; k >= 0; k--, function--)
    {
        if (function->codeHead->id < Optimizer->main->codeHead->id)
        {
            break;
        }
    }

    if (k >= 0)
    {
    	gcOPT_FUNCTION functionPrev;

        gcmASSERT(Optimizer->main->codeHead == function->codeTail->next);
        gcmASSERT(Optimizer->main->codeTail  ==
            ((k == (gctINT) Optimizer->functionCount - 1) ?
            Optimizer->codeTail :
                    functionArray[k + 1].codeHead->prev));
        for (; k > 0; k--, function = functionPrev)
        {
            functionPrev = function - 1;
            if (function->codeHead->prev != functionPrev->codeTail)
            {
                /* Move the code in the gap to in front of main. */
                gcOPT_CODE codeHead = functionPrev->codeTail->next;
                gcOpt_MoveCodeListBefore(Optimizer,
                    codeHead,
                    function->codeHead->prev,
                    Optimizer->main->codeHead);
                gcmASSERT(function->codeHead->prev == functionPrev->codeTail
                       && function->codeHead == functionPrev->codeTail->next);
                Optimizer->main->codeHead = codeHead;

                changeCode = gcvTRUE;
            }
        }

        /* Handle the first function. */
        if (function->codeHead->prev)
        {
            /* Move the code before function to in front of main. */
            gcOPT_CODE codeHead = Optimizer->codeHead;
            gcOpt_MoveCodeListBefore(Optimizer,
                codeHead,
                function->codeHead->prev,
                Optimizer->main->codeHead);
            gcmASSERT(function->codeHead->prev == gcvNULL);
            Optimizer->main->codeHead = codeHead;

            changeCode = gcvTRUE;
        }
    }
#if !_REMOVE_UNUSED_FUNCTION_CODE_
    else
    {
        if (Optimizer->main->codeHead != Optimizer->codeHead)
        {
            Optimizer->main->codeHead = Optimizer->codeHead;
        }
        gcmASSERT(Optimizer->main->codeTail == functionArray[0].codeHead->prev);
    }
#endif

	if (changeCode)
	{
		/* Update code id. */
		gcOpt_UpdateCodeId(Optimizer);
	}

	gcmFOOTER();
	return status;
}

/******************************************************************************\
|********************* Data Flow Graph Supporting Functioins ******************|
\******************************************************************************/

static gceSTATUS
_SetDefineInput(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gctINT			Index
	)
{
    gcmASSERT(Index < 0);
	if (*Root == gcvNULL) {
		return gcOpt_AddIndexToList(Optimizer, Root, Index);
	}
    else
    {
        gcmASSERT(*Root == gcvNULL);
        return gcvSTATUS_INVALID_ARGUMENT;
    }
}

static gceSTATUS
_SetTempDefineInput(
	IN gcOPTIMIZER			Optimizer,
	IN gcOPT_TEMP_DEFINE	TempDefine,
	IN gctUINT				Enable,
	IN gctINT				Index
	)
{
	gceSTATUS				status = gcvSTATUS_OK;
    gcmHEADER_ARG("Optimizer=0x%x TempDefine=0x%x Enable=%u Index=%d",
                   Optimizer, TempDefine, Enable, Index);

	/* Set temp defines according to enable. */
	if (Enable & gcSL_ENABLE_X)
	{
		gcmERR_RETURN(_SetDefineInput(Optimizer, &TempDefine->xDefines, Index));
	}
	if (Enable & gcSL_ENABLE_Y)
	{
		gcmERR_RETURN(_SetDefineInput(Optimizer, &TempDefine->yDefines, Index));
	}
	if (Enable & gcSL_ENABLE_Z)
	{
		gcmERR_RETURN(_SetDefineInput(Optimizer, &TempDefine->zDefines, Index));
	}
	if (Enable & gcSL_ENABLE_W)
	{
		gcmERR_RETURN(_SetDefineInput(Optimizer, &TempDefine->wDefines, Index));
	}

    gcmFOOTER();
	return status;
}

static gceSTATUS
_AddFunctionInputDefine(
	IN gcOPTIMIZER			Optimizer,
	IN gcOPT_FUNCTION		Function,
	IN gcOPT_TEMP_DEFINE	TempDefineArray
	)
{
	gceSTATUS				status = gcvSTATUS_OK;
	gcsFUNCTION_ARGUMENT_PTR argument;
	gcOPT_TEMP				tempArray = Optimizer->tempArray;
	gcOPT_GLOBAL_USAGE		usage;
	gctUINT					i;

    gcmHEADER_ARG("Optimizer=0x%x Function=0x%x TempDefineArray=0x%x",
                   Optimizer, Function, TempDefineArray);

	/* Add define for all input arguments. */
	argument = Function->arguments;
	for (i = 0; i < Function->argumentCount; i++, argument++)
	{
		/* Set output arguments. */
		if (argument->qualifier != gcvFUNCTION_OUTPUT)
		{
			gcmERR_RETURN(_SetTempDefineInput(Optimizer,
								&TempDefineArray[argument->index],
								argument->enable,
								gcvOPT_INPUT_REGISTER));
		}
	}

	/* Add define for all global variables. */
	for (usage = Function->globalUsage; usage; usage = usage->next)
	{
		gctUINT index = usage->index;

		gcmERR_RETURN(_SetTempDefineInput(Optimizer,
								&TempDefineArray[index],
								tempArray[index].usage,
								gcvOPT_GLOBAL_REGISTER));
	}

    gcmFOOTER();
	return status;
}

static gceSTATUS
_SetDefineList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_LIST			list;

    gcmHEADER_ARG("Optimizer=0x%x Root=0x%x Code=0x%x",
                   Optimizer, Root, Code);

	if (*Root == gcvNULL) {
        status = gcOpt_AddCodeToList(Optimizer, Root, Code);
        gcmFOOTER();
		return status;
	}

	for (list = *Root; list; list = list->next)
	{
        if (list->code == Code)
        {
            gcmFOOTER_NO();
	        return gcvSTATUS_OK;
        }

		if (list->index >= 0)
		{
			gcmERR_RETURN(gcOpt_AddCodeToList(Optimizer, &list->code->nextDefines, Code));
		}
	}

	if (Code->prevDefines == gcvNULL)
	{
		Code->prevDefines = *Root;
		*Root = gcvNULL;
        status = gcOpt_AddCodeToList(Optimizer, Root, Code);
        gcmFOOTER();
		return status;
	}

	gcmERR_RETURN(gcOpt_AddListToList(Optimizer, *Root, &Code->prevDefines));

	/* Free all entries in the list except one. */
	while ((list = (*Root)->next) != gcvNULL)
	{
		(*Root)->next = list->next;

		/* Free the gcOPT_LIST structure. */
		gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
	}

	/* Set code. */
	(*Root)->code  = Code;
	(*Root)->index = 0;

	/* Success. */
    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_SetTempDefine(
	IN gcOPTIMIZER			Optimizer,
	IN gcOPT_TEMP_DEFINE	TempDefine,
	IN gctUINT				Enable,
	IN gcOPT_CODE			Code
	)
{
	gceSTATUS				status = gcvSTATUS_OK;

    gcmHEADER_ARG("Optimizer=0x%x TempDefine=0x%x Enable=%u Code=0x%x",
                   Optimizer, TempDefine, Enable, Code);

	/* Set temp defines according to enable. */
	if (Enable & gcSL_ENABLE_X)
	{
		gcmERR_RETURN(_SetDefineList(Optimizer, &TempDefine->xDefines, Code));
	}
	if (Enable & gcSL_ENABLE_Y)
	{
		gcmERR_RETURN(_SetDefineList(Optimizer, &TempDefine->yDefines, Code));
	}
	if (Enable & gcSL_ENABLE_Z)
	{
		gcmERR_RETURN(_SetDefineList(Optimizer, &TempDefine->zDefines, Code));
	}
	if (Enable & gcSL_ENABLE_W)
	{
		gcmERR_RETURN(_SetDefineList(Optimizer, &TempDefine->wDefines, Code));
	}

    gcmFOOTER();
	return status;
}

#if _RECURSIVE_BUILD_DU_
static gceSTATUS
_AddUserRecusive(
    IN gcOPTIMIZER		Optimizer,
	IN gcOPT_CODE		Code,
    IN gcOPT_CODE		defCode
    )
{
    gceSTATUS			status = gcvSTATUS_OK;
    gcOPT_CODE          code;
    gcOPT_LIST          preDef;

    gcmHEADER_ARG("Optimizer=0x%x Code=0x%x", Optimizer, Code);

    code = defCode;
    if (code != gcvNULL)
    {
        preDef = code->prevDefines;
        while (preDef)
        {
            code = preDef->code;
            if (code != gcvNULL)
            {
                gcmERR_BREAK(gcOpt_AddCodeToList(Optimizer, &code->users, Code));

                /* No need go on if we have found an exact def. Otherwise, go into further */
                if (gcmSL_TARGET_GET(code->instruction.temp, Indexed) != gcSL_NOT_INDEXED)
                    gcmERR_BREAK(_AddUserRecusive(Optimizer, Code, code));
            }

            preDef = preDef->next;
        }
    }

    gcmFOOTER();
	return status;
}
#endif

static gceSTATUS
_AddUser(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_LIST		InputList,
	IN gcOPT_CODE		Code,
    IN gctBOOL bForSuccessiveReg
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_LIST			list;
    gcOPT_CODE          code;

    gcmHEADER_ARG("Optimizer=0x%x InputList=0x%x Code=0x%x",
                   Optimizer, InputList, Code);

	/* Add the input list to Root. */
	for (list = InputList; list; list = list->next)
	{
		if (list->index >= 0)
		{
            gcmERR_BREAK(gcOpt_AddCodeToList(Optimizer, &list->code->users, Code));

            /* Successive registers, such as for array/matrix, we use very conservative solution that uses */
            /* all defs before */
            if (bForSuccessiveReg && (gcmSL_TARGET_GET(list->code->instruction.temp, Indexed) != gcSL_NOT_INDEXED))
            {
                code = list->code;

#if _RECURSIVE_BUILD_DU_
                gcmERR_BREAK(_AddUserRecusive(Optimizer, Code, code));
#else
                while (code != gcvNULL && code->prevDefines)
                {
                    code = code->prevDefines->code;
                    if (code)
                    {
                        gcmERR_BREAK(gcOpt_AddCodeToList(Optimizer, &code->users, Code));

                        /* No need go on since we have found an exact def. */
                        if (gcmSL_TARGET_GET(code->instruction.temp, Indexed) == gcSL_NOT_INDEXED)
                            break;
                    }
                }
#endif
            }
		}
	}

    gcmFOOTER();
	return status;
}

#if _RECURSIVE_BUILD_DU_
static gceSTATUS
_AddTempDependencyRecusive(
    IN gcOPTIMIZER		Optimizer,
    IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		code
    )
{
    gceSTATUS			status = gcvSTATUS_OK;
    gcOPT_LIST          preDef;
    gctINT              index;

    gcmHEADER_ARG("Optimizer=0x%x Root=%p Code=0x%x", Optimizer, Root, code);

    if (code != gcvNULL)
    {
        preDef = code->prevDefines;
        while (preDef)
        {
            code = preDef->code;
            index = preDef->index;
            if (code != gcvNULL)
            {
                if (index < 0)
                {
		            gcmERR_BREAK(gcOpt_AddIndexToList(Optimizer, Root, index));
                }
                else
                {
                    gcmERR_BREAK(gcOpt_AddCodeToList(Optimizer, Root, code));
                }

                /* No need go on if we have found an exact def. Otherwise, go into further */
                if (gcmSL_TARGET_GET(code->instruction.temp, Indexed) != gcSL_NOT_INDEXED)
                    gcmERR_BREAK(_AddTempDependencyRecusive(Optimizer, Root, code));
            }

            preDef = preDef->next;
        }
    }

    gcmFOOTER();
	return status;
}
#endif

static gceSTATUS
_AddTempDependency(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_LIST		SrcList,
	IN OUT gcOPT_LIST *	Root,
    IN gctBOOL bForSuccessiveReg
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_LIST			list;
    gcOPT_CODE          code;

	gcmHEADER_ARG("Optimizer=%p SrcList=%p Root=%p", Optimizer, SrcList, Root);

	for (list = SrcList; list; list = list->next)
	{
        if (list->index < 0)
        {
    		gcmERR_BREAK(gcOpt_AddIndexToList(Optimizer, Root, list->index));
        }
        else
        {
            gcmERR_BREAK(gcOpt_AddCodeToList(Optimizer, Root, list->code));
        }

        /* Successive registers, such as for array/matrix, we use very conservative solution that uses */
        /* all defs before */
        if (bForSuccessiveReg && (gcmSL_TARGET_GET(list->code->instruction.temp, Indexed) != gcSL_NOT_INDEXED))
        {
            code = list->code;

#if _RECURSIVE_BUILD_DU_
            gcmERR_BREAK(_AddTempDependencyRecusive(Optimizer, Root, code));
#else
            while (code != gcvNULL && code->prevDefines)
            {
                if (code->prevDefines->index < 0)
                {
    		        gcmERR_BREAK(gcOpt_AddIndexToList(Optimizer, Root, code->prevDefines->index));
                }
                else
                {
                    gcmERR_BREAK(gcOpt_AddCodeToList(Optimizer, Root, code->prevDefines->code));
                }

                /* No need go on since we have found an exact def. */
                if (gcmSL_TARGET_GET(code->prevDefines->code->instruction.temp, Indexed) == gcSL_NOT_INDEXED)
                    break;

                code = code->prevDefines->code;
            }
#endif
        }
	}

	/* Success. */
	gcmFOOTER_ARG("*Root=%p status=%d", *Root, status);
	return gcvSTATUS_OK;
}

static gceSTATUS
_AddTempUsage(
	IN gcOPTIMIZER			Optimizer,
	IN gcOPT_TEMP_DEFINE	TempDefine,
	IN gctUINT				enable,
	IN OUT gcOPT_LIST *		Root,
	IN gcOPT_CODE			Code,
    IN gctBOOL              bForSuccessiveReg
	)
{
	gceSTATUS				status = gcvSTATUS_OK;

    gcmHEADER_ARG("Optimizer=0x%x TempDefine=0x%x enable=0x%x Root=0x%x Code=0x%x",
                   Optimizer, TempDefine, enable, Root, Code);

	/* Set temp defines according to enable. */
	if (enable & gcSL_ENABLE_X)
	{
		/* Add dependencies. */
		if (Root)
		{
			gcmERR_RETURN(_AddTempDependency(Optimizer, TempDefine->xDefines, Root, bForSuccessiveReg));
		}

		/* Add user. */
		gcmERR_RETURN(_AddUser(Optimizer, TempDefine->xDefines, Code, bForSuccessiveReg));
	}
	if (enable & gcSL_ENABLE_Y)
	{
		/* Add dependencies. */
		if (Root)
		{
			gcmERR_RETURN(_AddTempDependency(Optimizer, TempDefine->yDefines, Root, bForSuccessiveReg));
		}

		/* Add user. */
		gcmERR_RETURN(_AddUser(Optimizer, TempDefine->yDefines, Code, bForSuccessiveReg));
	}
	if (enable & gcSL_ENABLE_Z)
	{
		/* Add dependencies. */
		if (Root)
		{
			gcmERR_RETURN(_AddTempDependency(Optimizer, TempDefine->zDefines, Root, bForSuccessiveReg));
		}

		/* Add user. */
		gcmERR_RETURN(_AddUser(Optimizer, TempDefine->zDefines, Code, bForSuccessiveReg));
	}
	if (enable & gcSL_ENABLE_W)
	{
		/* Add dependencies. */
		if (Root)
		{
			gcmERR_RETURN(_AddTempDependency(Optimizer, TempDefine->wDefines, Root, bForSuccessiveReg));
		}

		/* Add user. */
		gcmERR_RETURN(_AddUser(Optimizer, TempDefine->wDefines, Code, bForSuccessiveReg));
	}

    gcmFOOTER();
	return status;
}

static gceSTATUS
_AddOutputUser(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_LIST		InputList,
	IN gctINT			Index
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_LIST			list;
    gcOPT_CODE          code;

    gcmHEADER_ARG("Optimizer=0x%x InputList=0x%x Index=%d",
                   Optimizer, InputList, Index);

	/* Add the input list to Root. */
	for (list = InputList; list; list = list->next)
	{
		if (list->index >= 0)
		{
			gcmERR_BREAK(gcOpt_AddIndexToList(Optimizer, &list->code->users, Index));

            /* No need go on since we have found an exact def. */
            if (gcmSL_TARGET_GET(list->code->instruction.temp, Indexed) == gcSL_NOT_INDEXED)
                continue;

            /* All appeared output defs must be add usage at end of program */
            code = list->code;
            while (code != gcvNULL && code->prevDefines)
            {
                code = code->prevDefines->code;
                if (code)
                {
                    gcmERR_BREAK(gcOpt_AddIndexToList(Optimizer, &code->users, Index));

                    /* No need go on since we have found an exact def. */
                    if (gcmSL_TARGET_GET(code->instruction.temp, Indexed) == gcSL_NOT_INDEXED)
                        break;
                }
            }
		}
	}

    gcmFOOTER();
	return status;
}

static gceSTATUS
_AddTempOutputUsage(
	IN gcOPTIMIZER			Optimizer,
	IN gcOPT_TEMP_DEFINE	TempDefine,
	IN gctUINT				enable,
	IN gctINT				Index
	)
{
	gceSTATUS				status = gcvSTATUS_OK;

    gcmHEADER_ARG("Optimizer=0x%x TempDefine=0x%x enable=%u Index=%u",
                   Optimizer, TempDefine, enable, Index);

	/* Set temp defines according to enable. */
	if (enable & gcSL_ENABLE_X)
	{
		/* Add user. */
		gcmERR_RETURN(_AddOutputUser(Optimizer, TempDefine->xDefines, Index));
	}
	if (enable & gcSL_ENABLE_Y)
	{
		/* Add user. */
		gcmERR_RETURN(_AddOutputUser(Optimizer, TempDefine->yDefines, Index));
	}
	if (enable & gcSL_ENABLE_Z)
	{
		/* Add user. */
		gcmERR_RETURN(_AddOutputUser(Optimizer, TempDefine->zDefines, Index));
	}
	if (enable & gcSL_ENABLE_W)
	{
		/* Add user. */
		gcmERR_RETURN(_AddOutputUser(Optimizer, TempDefine->wDefines, Index));
	}

    gcmFOOTER();
	return status;
}

static gceSTATUS
_AddFunctionOutputUsage(
	IN gcOPTIMIZER			Optimizer,
	IN gcOPT_FUNCTION		Function,
	IN gcOPT_TEMP_DEFINE	TempDefineArray
	)
{
	gceSTATUS				status = gcvSTATUS_OK;
	gctUINT i;

    gcmHEADER_ARG("Optimizer=0x%x Function=0x%x TempDefineArray=0x%x",
                   Optimizer, Function, TempDefineArray);

	/* Add output dependancy. */
	if (Function == gcvNULL)
	{
		/* Main program. */
		for (i = 0; i < Optimizer->outputCount; i++)
		{
            gctUINT regIdx;
		    gctUINT size;
		    gctUINT	endIndex;

            /* output could be NULL if it is not used */
            if (Optimizer->outputs[i] == gcvNULL)
                continue;

            size = _typeSize[Optimizer->outputs[i]->type];
		    endIndex = Optimizer->outputs[i]->tempIndex + Optimizer->outputs[i]->arraySize * size;

		    for (regIdx = Optimizer->outputs[i]->tempIndex; regIdx < endIndex; regIdx++)
		    {
		        gcmERR_RETURN(_AddTempOutputUsage(Optimizer,
								        &TempDefineArray[regIdx],
								        gcSL_ENABLE_XYZW,
								        gcvOPT_OUTPUT_REGISTER));
            }
		}
	}
	else
	{
		gcsFUNCTION_ARGUMENT_PTR argument;
		gcOPT_TEMP tempArray = Optimizer->tempArray;
		gcOPT_GLOBAL_USAGE usage;

		/* Function. */
		/* Add users for all output arguments. */
		argument = Function->arguments;
		for (i = 0; i < Function->argumentCount; i++, argument++)
		{
			/* Set output arguments. */
			if (argument->qualifier != gcvFUNCTION_INPUT)
			{
				gcmERR_RETURN(_AddTempOutputUsage(Optimizer,
									&TempDefineArray[argument->index],
									argument->enable,
									gcvOPT_OUTPUT_REGISTER));
			}
		}

		/* Add users for all global variables. */
		for (usage = Function->globalUsage; usage; usage = usage->next)
		{
			gctUINT index = usage->index;

			gcmERR_RETURN(_AddTempOutputUsage(Optimizer,
									&TempDefineArray[index],
									tempArray[index].usage,
									gcvOPT_GLOBAL_REGISTER));
		}
	}

    gcmVERIFY_OK(gcOpt_ClearTempArray(Optimizer, TempDefineArray));

    gcmFOOTER();
	return status;
}

gceSTATUS
gcOpt_DestroyCodeDependency(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
	)
{
	gcOPT_LIST			list, nextList;

	gcmHEADER_ARG("Optimizer=%p Root=%p Code=%p",
					Optimizer, Root, Code);

	for (list = *Root; list; list = nextList)
	{
		nextList = list->next;

		if (list->index >= 0)
		{
			gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &list->code->users, Code));
		}

		/* Free the gcOPT_LIST structure. */
		gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
	}

	*Root = gcvNULL;

	gcmFOOTER_ARG("*Root=%p", *Root);
	return gcvSTATUS_OK;
}

static gceSTATUS
_MergeTempDefineArray(
	IN gcOPTIMIZER			Optimizer,
    IN gcOPT_TEMP_DEFINE	SrcTempDefineArray,
	OUT gcOPT_TEMP_DEFINE *	DestTempDefineArray
	)
{
	gceSTATUS				status = gcvSTATUS_OK;
	gcOPT_TEMP_DEFINE		srcTempDefine;
	gcOPT_TEMP_DEFINE		destTempDefine;
	gctUINT					i;

    gcmHEADER_ARG("Optimizer=0x%x SrcTempDefineArray=0x%x",
                   Optimizer, SrcTempDefineArray);

	/* Allocate new array if needed. */
    if (*DestTempDefineArray == gcvNULL && Optimizer->tempCount > 0)
    {
    	gcmERR_RETURN(_CAllocateTempDefineArray(Optimizer->tempDefineArrayMemPool,
            DestTempDefineArray, Optimizer->tempCount));
    }

    srcTempDefine = SrcTempDefineArray;
    destTempDefine = *DestTempDefineArray;
    for (i = 0; i < Optimizer->tempCount; i++, srcTempDefine++, destTempDefine++)
    {
	    gcmERR_BREAK(gcOpt_AddListToList(Optimizer, srcTempDefine->xDefines, &destTempDefine->xDefines));
	    gcmERR_BREAK(gcOpt_AddListToList(Optimizer, srcTempDefine->yDefines, &destTempDefine->yDefines));
	    gcmERR_BREAK(gcOpt_AddListToList(Optimizer, srcTempDefine->zDefines, &destTempDefine->zDefines));
	    gcmERR_BREAK(gcOpt_AddListToList(Optimizer, srcTempDefine->wDefines, &destTempDefine->wDefines));
    }

    gcmFOOTER();
	return status;
}

static gctUINT
_GetEnableFromSwizzles(
	IN gcSL_SWIZZLE		SwizzleX,
	IN gcSL_SWIZZLE		SwizzleY,
	IN gcSL_SWIZZLE		SwizzleZ,
	IN gcSL_SWIZZLE		SwizzleW
	)
{
	gctUINT				enable = 0;

	static const gcSL_ENABLE swizzleToEnable[] =
	{
		gcSL_ENABLE_X, gcSL_ENABLE_Y, gcSL_ENABLE_Z, gcSL_ENABLE_W
	};

	/* Calulate enable from swizzles. */
	if ((SwizzleX == gcSL_SWIZZLE_X) &&
		(SwizzleY == gcSL_SWIZZLE_Y) &&
		(SwizzleZ == gcSL_SWIZZLE_Z) &&
		(SwizzleW == gcSL_SWIZZLE_W))
	{
		enable = gcSL_ENABLE_XYZW;
	}
	else
	{
		/* Enable the x swizzle. */
		enable = swizzleToEnable[SwizzleX];

		/* Only continue of the other swizzles are different. */
		if ((SwizzleY != SwizzleX) ||
			(SwizzleZ != SwizzleX) ||
			(SwizzleW != SwizzleX))
		{
			/* Enable the y swizzle. */
			enable |= swizzleToEnable[SwizzleY];

			/* Only continue of the other swizzles are different. */
			if ( (SwizzleZ != SwizzleY) || (SwizzleW != SwizzleY) )
			{
				/* Enable the z swizzle. */
				enable |= swizzleToEnable[SwizzleZ];

				/* Only continue of the other swizzle are different. */
				if (SwizzleW != SwizzleZ)
				{
					/* Enable the w swizzle. */
					enable |= swizzleToEnable[SwizzleW];
				}
			}
		}
	}

	return enable;
}

static gctBOOL _needSuccessiveReg(
    IN gcOPTIMIZER			Optimizer,
    IN gctUINT		        regIndex
    )
{
    gctBOOL ret = gcvFALSE;

    if (Optimizer->tempArray[regIndex].arrayVariable)
    {
        ret = (Optimizer->tempArray[regIndex].arrayVariable->arraySize > 1) ||
              IS_MATRIX_TYPE(Optimizer->tempArray[regIndex].arrayVariable->u.type);
    }

    return ret;
}

static void
_PostOrderVariable(
    IN gcSHADER Shader,
    IN gcVARIABLE rootVariable,
    IN gcVARIABLE firstVariable,
    IN gctBOOL_PTR StartCalc,
    OUT gctUINT *Start,
    OUT gctUINT *End)
{
    gctUINT          start = 0xffffffff, end = 0;
    gcVARIABLE       var;
    gctINT16        varIndex;

    if (!*StartCalc && (rootVariable == firstVariable))
        *StartCalc = gcvTRUE;

    if (rootVariable->firstChild != -1)
    {
        gcmASSERT (rootVariable->varCategory == gcSHADER_VAR_CATEGORY_STRUCT);

        varIndex = rootVariable->firstChild;
        while (varIndex != -1)
        {
            gctUINT          startTemp = 0, endTemp = 0;

            var = Shader->variables[varIndex];

            if (!*StartCalc && (var == firstVariable))
                *StartCalc = gcvTRUE;

            _PostOrderVariable(Shader, var, firstVariable, StartCalc, &startTemp, &endTemp);

            if (*StartCalc)
            {
                /* Currently, only select the maximum range, but
                   we can improve it to get separated ranges for
                   more accurate analysis later */
                if (startTemp < start) start = startTemp;
                if (endTemp > end) end = endTemp;
            }

            varIndex = var->nextSibling;
        }
    }

    /* Process root */
    if (rootVariable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL)
    {
        if (*StartCalc)
        {
            start = rootVariable->tempIndex;
            end = rootVariable->tempIndex + rootVariable->arraySize * _typeSize[rootVariable->u.type];
        }
    }
    else
    {
        /* Nothing to do since start and end have been cal when visiting its children. */
    }

    *Start = start;
    *End = end;
}

gceSTATUS
_GetVariableIndexingRange(
    IN gcSHADER Shader,
    IN gcVARIABLE variable,
    IN gctBOOL whole,
    OUT gctUINT *Start,
    OUT gctUINT *End)
{
    gceSTATUS        status = gcvSTATUS_OK;
    gctINT           root = -1;
    gcVARIABLE       var;
    gctBOOL          bStartCalc;

    /* Find arrayed root for this variable */
    var = variable;
    while (var)
    {
        if (var->parent != -1)
        {
            gctINT parent = var->parent;
            var = Shader->variables[var->parent];

            /* Find the out-most array */
            if (var->arraySize > 1)
                root = parent;
        }
        else
            var = gcvNULL;
    }

    /* To control whole or part of struct element array */
    if (whole)
        bStartCalc = gcvTRUE;
    else
        bStartCalc = gcvFALSE;

    /* Post order to calc range */
    _PostOrderVariable(Shader, (root == -1) ? variable : Shader->variables[root],
                       variable, &bStartCalc, Start, End);

    return status;
}

static gceSTATUS
_BuildDataFlowForCode(
	IN gcOPTIMIZER			Optimizer,
	IN gcOPT_CODE			Code,
	IN gcOPT_TEMP_DEFINE	TempDefineArray
	)
{
	gceSTATUS				status = gcvSTATUS_OK;
    gcSL_INSTRUCTION		code = &Code->instruction;
    gcOPT_TEMP				tempArray = Optimizer->tempArray;
	gctUINT16				target;
	gctUINT16				source;
	gctUINT16				index;
	gctUINT8				format;
	gctBOOL					targetIsTemp = gcvFALSE;

    gcmHEADER_ARG("Optimizer=0x%x Code=0x%x TempDefineArray=0x%x",
                   Optimizer, Code, TempDefineArray);

	/* Dispatch on opcode. */
	switch (code->opcode)
	{
	case gcSL_CALL:
		{
		/* Add dependencies for input and output arguments. */
		gcOPT_FUNCTION function = Code->callee->function;
		gcsFUNCTION_ARGUMENT_PTR argument;
		gcOPT_GLOBAL_USAGE usage;
		gctUINT i;

		/* Add dependencies for all input arguments, and add users for all output arguments. */
		argument = function->arguments;
		for (i = 0; i < function->argumentCount; i++, argument++)
		{
			gctUINT indexA = argument->index;
			gctUINT enable = argument->enable;

			/* Set input arguments. */
			if (argument->qualifier != gcvFUNCTION_OUTPUT)
			{
				gcmERR_RETURN(_AddTempUsage(Optimizer,
									&TempDefineArray[indexA],
									enable,
									&Code->dependencies0,
									Code,
                                    _needSuccessiveReg(Optimizer, indexA)));
			}

			/* Set output arguments. */
			if (argument->qualifier != gcvFUNCTION_INPUT)
			{
				gcmERR_RETURN(_SetTempDefine(Optimizer,
									&TempDefineArray[indexA],
									enable,
									Code));
			}
		}

		/* Add global variable dependencies and users. */
		for (usage = function->globalUsage; usage; usage = usage->next)
		{
			gctUINT indexT = usage->index;
			gctUINT enable = Optimizer->tempArray[indexT].usage;

			if (usage->direction != gcvFUNCTION_OUTPUT)
			{
				gcmERR_RETURN(_AddTempUsage(Optimizer,
									&TempDefineArray[indexT],
									enable,
									&Code->dependencies0,
									Code,
                                    _needSuccessiveReg(Optimizer, indexT)));
			}
			if (usage->direction != gcvFUNCTION_INPUT)
			{
				gcmERR_RETURN(_SetTempDefine(Optimizer,
									&TempDefineArray[indexT],
									enable,
									Code));
			}
		}
		}
		break;

	case gcSL_RET:
		/* Function returns, so add output dependancy and clean up TempDefineArray. */
		gcmVERIFY_OK(_AddFunctionOutputUsage(Optimizer,
									Code->function,
									TempDefineArray));

        gcmFOOTER();
		return gcvSTATUS_OK;

	case gcSL_STORE:
		/* Get gcSL_TARGET field. */
		target = code->temp;

        /* Target is actually source, so add usage. */
		gcmERR_RETURN(_AddTempUsage(Optimizer,
							&TempDefineArray[code->tempIndex],
							gcmSL_TARGET_GET(target, Enable),
							&Code->dependencies1,
							Code,
                            _needSuccessiveReg(Optimizer, code->tempIndex)));

	    if (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
	    {
		    gctUINT enable = 0;

		    switch (gcmSL_TARGET_GET(target, Indexed))
		    {
		    case gcSL_INDEXED_X:
			    enable = gcSL_ENABLE_X; break;
		    case gcSL_INDEXED_Y:
			    enable = gcSL_ENABLE_Y; break;
		    case gcSL_INDEXED_Z:
			    enable = gcSL_ENABLE_Z; break;
		    case gcSL_INDEXED_W:
			    enable = gcSL_ENABLE_W; break;
		    }

		    index = code->tempIndexed;

		    gcmERR_RETURN(_AddTempUsage(Optimizer,
							    &TempDefineArray[gcmSL_INDEX_GET(index, Index)],
							    enable,
							    &Code->dependencies1,
							    Code,
                                _needSuccessiveReg(Optimizer, gcmSL_INDEX_GET(index, Index))));

		    /* Mark all array temp registers as inputs. */
		    {
			    gctUINT i;

			    if (Optimizer->tempArray[code->tempIndex].arrayVariable)
			    {
				    gcVARIABLE variable = Optimizer->tempArray[code->tempIndex].arrayVariable;

                    gctUINT startIndex, endIndex;
                    gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                    _GetVariableIndexingRange(Optimizer->shader, variable, gcvFALSE,
                                              &startIndex, &endIndex);
                    gcmASSERT(startIndex == variable->tempIndex);

				    for (i = startIndex; i < endIndex; i++)
				    {
					    gcmERR_RETURN(_AddTempUsage(Optimizer,
							    &TempDefineArray[i],
							    gcmSL_TARGET_GET(target, Enable),
							    &Code->dependencies1,
							    Code,
                                _needSuccessiveReg(Optimizer, i)));
				    }
			    }
			    else
			    {
				    gcmASSERT(Optimizer->tempArray[code->tempIndex].arrayVariable);
			    }
		    }
	    }

        /* Add output usage. */
        gcmERR_RETURN(gcOpt_AddIndexToList(Optimizer, &Code->users, gcvOPT_OUTPUT_REGISTER));
		break;

	case gcSL_STORE1:
	case gcSL_ATOMADD:
	case gcSL_ATOMSUB:
	case gcSL_ATOMXCHG:
	case gcSL_ATOMCMPXCHG:
	case gcSL_ATOMMIN:
	case gcSL_ATOMMAX:
	case gcSL_ATOMOR:
	case gcSL_ATOMAND:
	case gcSL_ATOMXOR:
        /* Add output usage. */
        gcmERR_RETURN(gcOpt_AddIndexToList(Optimizer, &Code->users, gcvOPT_OUTPUT_REGISTER));
		break;

	case gcSL_JMP:
	case gcSL_NOP:
	case gcSL_KILL:
		/* Skip control flow instructions. */
		break;

	case gcSL_TEXBIAS:
	case gcSL_TEXGRAD:
	case gcSL_TEXLOD:
		/* Skip texture state instructions. */
		break;

	default:
		/* Get gcSL_TARGET field. */
		target = code->temp;

		/* Set instruction to definitions. */
		gcmERR_RETURN(_SetTempDefine(Optimizer,
									&TempDefineArray[code->tempIndex],
									gcmSL_TARGET_GET(target, Enable),
									Code));

        /* Set temp format. */
        format = gcmSL_TARGET_GET(target, Format);
        if (tempArray[code->tempIndex].format == -1)
        {
            tempArray[code->tempIndex].format = format;
        }
        else
        {
            if (tempArray[code->tempIndex].format != format)
            {
                if (tempArray[code->tempIndex].format != -2)
                {
                    if (format == gcSL_FLOAT || tempArray[code->tempIndex].format == gcSL_FLOAT)
                    {
                        tempArray[code->tempIndex].format = -2;
                    }
                }
            }
        }

        if (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
        {
		    gctUINT i;

		    if (Optimizer->tempArray[code->tempIndex].arrayVariable)
		    {
			    gcVARIABLE variable = Optimizer->tempArray[code->tempIndex].arrayVariable;

                gctUINT startIndex, endIndex;
                gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                _GetVariableIndexingRange(Optimizer->shader, variable, gcvFALSE,
                                          &startIndex, &endIndex);
                gcmASSERT(startIndex == variable->tempIndex);

			    for (i = startIndex; i < endIndex; i++)
			    {
				    gcmERR_RETURN(_SetTempDefine(Optimizer,
									&TempDefineArray[i],
									gcmSL_TARGET_GET(target, Enable),
									Code));
                    tempArray[i].format = format;
			    }
		    }
		    else
		    {
			    gcmASSERT(Optimizer->tempArray[code->source0Index].arrayVariable);
		    }
        }

		targetIsTemp = gcvTRUE;

		/* TODO - need to handle indexed mode? */

		break;
	}

	/* Determine usage of source0. */
	source = code->source0;

	if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
	{
		gctUINT enable = _GetEnableFromSwizzles((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW));

		if (targetIsTemp && code->source0Index == code->tempIndex)
		{
			gcmASSERT(code->source0Index != code->tempIndex);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}

		gcmERR_RETURN(_AddTempUsage(Optimizer,
							&TempDefineArray[code->source0Index],
							enable,
							&Code->dependencies0,
							Code,
                            _needSuccessiveReg(Optimizer, code->source0Index)));
	}

	if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
	{
		gctUINT enable = 0;

		switch (gcmSL_SOURCE_GET(source, Indexed))
		{
		case gcSL_INDEXED_X:
			enable = gcSL_ENABLE_X; break;
		case gcSL_INDEXED_Y:
			enable = gcSL_ENABLE_Y; break;
		case gcSL_INDEXED_Z:
			enable = gcSL_ENABLE_Z; break;
		case gcSL_INDEXED_W:
			enable = gcSL_ENABLE_W; break;
		}

		index = code->source0Indexed;

		gcmERR_RETURN(_AddTempUsage(Optimizer,
							&TempDefineArray[gcmSL_INDEX_GET(index, Index)],
							enable,
							&Code->dependencies0,
							Code,
                            _needSuccessiveReg(Optimizer, gcmSL_INDEX_GET(index, Index))));

		/* If the source is temp register, mark all array temp registers as inputs. */
		if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
		{
			gctUINT i;
			enable = _GetEnableFromSwizzles((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW));

			if (Optimizer->tempArray[code->source0Index].arrayVariable)
			{
				gcVARIABLE variable = Optimizer->tempArray[code->source0Index].arrayVariable;

                gctUINT startIndex, endIndex;
                gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                _GetVariableIndexingRange(Optimizer->shader, variable, gcvFALSE,
                                          &startIndex, &endIndex);
                gcmASSERT(startIndex == variable->tempIndex);

				for (i = startIndex; i < endIndex; i++)
				{
					gcmERR_RETURN(_AddTempUsage(Optimizer,
							&TempDefineArray[i],
							enable,
							&Code->dependencies0,
							Code,
                            _needSuccessiveReg(Optimizer, i)));
				}
			}
			else
			{
				gcmASSERT(Optimizer->tempArray[code->source0Index].arrayVariable);
			}
		}
	}

	/* TODO - Special case for TEXLD? */

	/* Determine usage of source1. */
	source = code->source1;

	if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
	{
		gctUINT enable = _GetEnableFromSwizzles((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW));

		if (targetIsTemp && code->source1Index == code->tempIndex)
		{
			gcmASSERT(code->source1Index != code->tempIndex);
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
			return status;
		}

		gcmERR_RETURN(_AddTempUsage(Optimizer,
							&TempDefineArray[code->source1Index],
							enable,
							&Code->dependencies1,
							Code,
                            _needSuccessiveReg(Optimizer, code->source1Index)));
	}

	if (gcmSL_SOURCE_GET(source, Indexed) != gcSL_NOT_INDEXED)
	{
		gctUINT enable = 0;

		switch (gcmSL_SOURCE_GET(source, Indexed))
		{
		case gcSL_INDEXED_X:
			enable = gcSL_ENABLE_X; break;
		case gcSL_INDEXED_Y:
			enable = gcSL_ENABLE_Y; break;
		case gcSL_INDEXED_Z:
			enable = gcSL_ENABLE_Z; break;
		case gcSL_INDEXED_W:
			enable = gcSL_ENABLE_W; break;
		}

		index = code->source1Indexed;

		gcmERR_RETURN(_AddTempUsage(Optimizer,
							&TempDefineArray[gcmSL_INDEX_GET(index, Index)],
							enable,
							&Code->dependencies1,
							Code,
                            _needSuccessiveReg(Optimizer, gcmSL_INDEX_GET(index, Index))));

		/* If the source is temp register, mark all array temp registers as inputs. */
		if (gcmSL_SOURCE_GET(source, Type) == gcSL_TEMP)
		{
			gctUINT i;
			enable = _GetEnableFromSwizzles((gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleX),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleY),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleZ),
							(gcSL_SWIZZLE) gcmSL_SOURCE_GET(source, SwizzleW));

			if (Optimizer->tempArray[code->source1Index].arrayVariable)
			{
				gcVARIABLE variable = Optimizer->tempArray[code->source1Index].arrayVariable;

                gctUINT startIndex, endIndex;
                gcmASSERT(variable->varCategory == gcSHADER_VAR_CATEGORY_NORMAL);
                _GetVariableIndexingRange(Optimizer->shader, variable, gcvFALSE,
                                          &startIndex, &endIndex);
                gcmASSERT(startIndex == variable->tempIndex);

				for (i = startIndex; i < endIndex; i++)
				{
					gcmERR_RETURN(_AddTempUsage(Optimizer,
							&TempDefineArray[i],
							enable,
							&Code->dependencies1,
							Code,
                            _needSuccessiveReg(Optimizer, i)));
				}
			}
			else
			{
				gcmASSERT(Optimizer->tempArray[code->source1Index].arrayVariable);
			}
		}
	}

	/* Determine usage of dst index register. */
    target = code->temp;

	if (gcmSL_TARGET_GET(target, Indexed) != gcSL_NOT_INDEXED)
	{
		/* Mark usage for index register */
		gctUINT enable = 0;

		switch (gcmSL_TARGET_GET(target, Indexed))
		{
		case gcSL_INDEXED_X:
			enable = gcSL_ENABLE_X; break;
		case gcSL_INDEXED_Y:
			enable = gcSL_ENABLE_Y; break;
		case gcSL_INDEXED_Z:
			enable = gcSL_ENABLE_Z; break;
		case gcSL_INDEXED_W:
			enable = gcSL_ENABLE_W; break;
		}

		index = code->tempIndexed;

		gcmERR_RETURN(_AddTempUsage(Optimizer,
							&TempDefineArray[gcmSL_INDEX_GET(index, Index)],
							enable,
							&Code->dependencies1,
							Code,
                            _needSuccessiveReg(Optimizer, gcmSL_INDEX_GET(index, Index))));
	}

    gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

static gceSTATUS
_BuildFunctionFlowGraph(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_FUNCTION	Function
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_TEMP_DEFINE	tempDefineArray = gcvNULL;
    gcOPT_CODE          code;

    gcmHEADER_ARG("Optimizer=0x%x Function=0x%x",
                   Optimizer, Function);
	/* Allocate array of lists to save where temp registers are defined. */
	if (Optimizer->tempCount)
	{
		gcmERR_RETURN(_CAllocateTempDefineArray(Optimizer->tempDefineArrayMemPool, &tempDefineArray, Optimizer->tempCount));
	}

	/* Add input and global dependancies. */
	if (Function != Optimizer->main)
	{
		gcmVERIFY_OK(_AddFunctionInputDefine(Optimizer, Function, tempDefineArray));
	}

	/* Build data flow. */
    for (code = Function->codeHead;
         code != Function->codeTail->next;
         code = code->next)
    {
        if (code->callers)
        {
            if (code->tempDefine)
            {
    		    gcmERR_RETURN(_MergeTempDefineArray(Optimizer,
										code->tempDefine, &tempDefineArray));
            }
		    gcmERR_RETURN(_MergeTempDefineArray(Optimizer,
										tempDefineArray, &code->tempDefine));
        }

		gcmERR_RETURN(_BuildDataFlowForCode(Optimizer, code, tempDefineArray));

        if (code->instruction.opcode == gcSL_JMP)
        {
            gcOPT_CODE codeDest = code->callee;

            if (! code->backwardJump)
            {
    		    gcmERR_RETURN(_MergeTempDefineArray(Optimizer,
										tempDefineArray, &codeDest->tempDefine));
            }
            else if (! code->handled)
            {
                code->handled = gcvTRUE;
                for (code = code->prev;
                     code != gcvNULL && code != codeDest->prev;
                     code = code->prev)
                {
                    /* Reset handled flag for the codes in between. */
                    if (code->backwardJump)
                    {
                        gcmASSERT(code->instruction.opcode == gcSL_JMP ? code->handled : 1);
                        code->handled = gcvFALSE;
                    }
                }

                continue;
            }

            /* Clear tempDefines if the jump is unconditional. */
            if (gcmSL_TARGET_GET(code->instruction.temp, Condition) == gcSL_ALWAYS)
            {
                gcmVERIFY_OK(gcOpt_ClearTempArray(Optimizer, tempDefineArray));
            }
        }
    }

	/* Output dependancies are already added by RET instruction (main program has extra RET instruction). */

	/* tempDefineArray's lists are freed in _AddOutputUsage. */
	/* Free tempDefineArray. */
	if (tempDefineArray)
	{
		gcmVERIFY_OK(_FreeTempDefineArray(Optimizer->tempDefineArrayMemPool, tempDefineArray));
	}

    /* Free tempDefines in codes. */
    for (code = Function->codeHead;
         code != gcvNULL && code != Function->codeTail->next;
         code = code->next)
    {
        if (code->tempDefine)
        {
            gcmVERIFY_OK(gcOpt_ClearTempArray(Optimizer, code->tempDefine));
    		gcmVERIFY_OK(_FreeTempDefineArray(Optimizer->tempDefineArrayMemPool, code->tempDefine));
            code->tempDefine = gcvNULL;
        }
    }

    gcmFOOTER();
	return status;
}

/*******************************************************************************
**							gcOpt_BuildFlowGraph
********************************************************************************
**
**	Build control flow and data flow.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a Optimizer structure.
*/
gceSTATUS
gcOpt_BuildFlowGraph(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
	gcOPT_FUNCTION		functionArray = Optimizer->functionArray;
	gctUINT				i;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
	/* Sanity check. */
	if (Optimizer->functionCount == 0)
	{
		gcmASSERT(Optimizer->functionArray == gcvNULL);
        gcmASSERT(Optimizer->main->codeHead == Optimizer->codeHead);
        gcmASSERT(Optimizer->main->codeTail  == Optimizer->codeTail);
        gcmASSERT(Optimizer->codeHead->id == 0);
        /*gcmASSERT(Optimizer->codeTail->id == Optimizer->codeCount - 1);*/
	}
	else
	{
        gcOPT_CODE prevCode = gcvNULL;
        gctBOOL findMain = gcvFALSE;
        gctBOOL mainFirst = gcvFALSE;

		gcmASSERT(functionArray);
        if (functionArray == gcvNULL)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        for (i = 0; i < Optimizer->functionCount; i++)
		{
            if (functionArray[i].codeHead->prev == prevCode)
            {
                gcmASSERT(! prevCode || prevCode->next == functionArray[i].codeHead);
    			prevCode = functionArray[i].codeTail;
            }
            else
            {
                gcmASSERT(! findMain);
                findMain = gcvTRUE;
                gcmASSERT(! prevCode || prevCode->next == Optimizer->main->codeHead);
                if (! prevCode)
                {
                    mainFirst = gcvTRUE;
                }
                gcmASSERT(Optimizer->main->codeHead->prev == prevCode);
    			prevCode = Optimizer->main->codeTail;
                i--;  /* Recheck the function again. */
            }
		}

        if (! prevCode)
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            gcmFOOTER();
            return status;
        }

        if (findMain)
		{
			gcmASSERT(prevCode == Optimizer->codeTail);
			gcmASSERT(prevCode->next == gcvNULL);
			if (mainFirst)
			{
				gcmASSERT(Optimizer->main->codeHead == Optimizer->codeHead);
				gcmASSERT(prevCode->next == gcvNULL);
			}
		}
		else
		{
			gcmASSERT(Optimizer->main->codeHead->prev == prevCode);
			gcmASSERT(prevCode->next == Optimizer->main->codeHead);
			gcmASSERT(Optimizer->main->codeTail == Optimizer->codeTail);
		}
	}
#endif

	if (Optimizer->functionCount > 0)
	{
		gcmERR_RETURN(_BuildGlobalUsage(Optimizer));
	}

	/* Build data flow for main. */
	gcmERR_RETURN(_BuildFunctionFlowGraph(Optimizer, Optimizer->main));

	/* Build data flow for each function. */
	for (i = 0; i < Optimizer->functionCount; i++)
	{
		gcmERR_RETURN(_BuildFunctionFlowGraph(Optimizer, functionArray + i));
	}

	/*DUMP_OPTIMIZER("Build data flow for the shader", Optimizer);*/
	gcmFOOTER();
	return status;
}

/*******************************************************************************
**							gcOpt_DestroyFlowGraph
********************************************************************************
**
**	Free control flow and data flow data.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_DestroyFlowGraph(
	IN gcOPTIMIZER		Optimizer
	)
{
	gcOPT_CODE			code;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	if (Optimizer->functionArray != gcvNULL)
	{
		gcOPT_FUNCTION functionArray = Optimizer->functionArray;
		gcOPT_FUNCTION function;

		function = functionArray + Optimizer->functionCount - 1;
		for (; function >= functionArray; function--)
		{
			/* Free global variable usage list. */
			while (function->globalUsage)
			{
				gcOPT_GLOBAL_USAGE usage = function->globalUsage;
				function->globalUsage = usage->next;
				gcmVERIFY_OK(_FreeGlobalUsage(Optimizer->usageMemPool, usage));
			}
		}
	}

	/* Free data flow. */
    for (code = Optimizer->codeHead; code; code = code->next)
	{
        code->handled = gcvFALSE;
		if (code->dependencies0)
		{
			gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &code->dependencies0));
		}

		if (code->dependencies1)
		{
			gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &code->dependencies1));
		}

		if (code->users)
		{
			gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &code->users));
		}

		if (code->prevDefines)
		{
			gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &code->prevDefines));
		}

		if (code->nextDefines)
		{
			gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &code->nextDefines));
		}
	}

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/*******************************************************************************
**							gcOpt_RebuildFlowGraph
********************************************************************************
**
**	Rebuild flow graph.
**		This is used after functionCount change or code change that does not
**			update flow graph properly.
**		Assume tempCount is not changed.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_RebuildFlowGraph(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	/* Free flow graph. */
	gcmVERIFY_OK(gcOpt_DestroyFlowGraph(Optimizer));

	/* Update code id. */
	gcOpt_UpdateCodeId(Optimizer);

	/* Rebuild flow graph. */
	gcmONERROR(gcOpt_BuildFlowGraph(Optimizer));

	/* DUMP_OPTIMIZER("Update the flow graph of the shader", Optimizer); */
	gcmFOOTER();
	return status;

OnError:
    /* Return the status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**							gcOpt_ConstructOptimizer
********************************************************************************
**
**	Construct a new gcOPTIMIZER structure.
**
**	INPUT:
**
**		gcSHADER Shader
**			Pointer to a gcSHADER object holding information about the compiled shader.
**
**	OUT:
**
**		gcOPTIMIZER * Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_ConstructOptimizer(
	IN gcSHADER			Shader,
	OUT gcOPTIMIZER *	Optimizer
	)
{
	gceSTATUS status;
	gcOPTIMIZER optimizer = gcvNULL;
    gctPOINTER pointer = gcvNULL;

	gcmHEADER_ARG("Shader=0x%x", Shader);

	/* Verify the arguments. */
	gcmVERIFY_OBJECT(Shader, gcvOBJ_SHADER);
	gcmVERIFY_ARGUMENT(Optimizer != gcvNULL);

	/* Allocate optimizer data structure. */
	gcmONERROR(gcoOS_Allocate(gcvNULL,
							  gcmSIZEOF(struct _gcOPTIMIZER),
							  &pointer));
    optimizer = pointer;

	gcmONERROR(gcoOS_ZeroMemory(optimizer, gcmSIZEOF(struct _gcOPTIMIZER)));

	gcmONERROR(_MemPoolInit(optimizer));

	gcmONERROR(gcOpt_CopyInShader(optimizer, Shader));

	/* Build temp register array. */
	gcmONERROR(_BuildTempArray(optimizer));

	/* Pack main program if necessary. */
	gcmONERROR(_PackMainProgram(optimizer));

	/* Build flow graph. */
	gcmONERROR(gcOpt_BuildFlowGraph(optimizer));

	*Optimizer = optimizer;
	gcmFOOTER_ARG("*Optimizer=0x%x", *Optimizer);
	return gcvSTATUS_OK;

OnError:
	if (optimizer != gcvNULL)
	{
		/* Destroy the optimizer. */
		gcOpt_DestroyOptimizer(optimizer);
	}

	/* Return the status. */
	gcmFOOTER();
	return status;
}

/*******************************************************************************
**							   gcOpt_DestroyOptimizer
********************************************************************************
**
**	Destroy a gcOPTIMIZER structure.
**
**	IN OUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_DestroyOptimizer(
	IN OUT gcOPTIMIZER	Optimizer
	)
{
	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	if (Optimizer == gcvNULL)
	{
		gcmFOOTER_NO();
		return gcvSTATUS_OK;
	}

	/* Free main and function array. */
	gcmVERIFY_OK(_DestroyFunctionArray(Optimizer));

	/* Free temp reg array. */
	gcmVERIFY_OK(_DestroyTempArray(Optimizer));

	/* Free global variable list. */
	if (Optimizer->global)
	{
		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &Optimizer->global));
	}

	/* Free code flow graph. */
	gcmVERIFY_OK(gcOpt_DestroyFlowGraph(Optimizer));

    /* Free code list. */
    gcOpt_RemoveCodeList(Optimizer, Optimizer->codeHead, Optimizer->codeTail);

	/* Free memory pools. */
	_MemPoolCleanup(Optimizer);

	/* Free optimizer data structure. */
	gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Optimizer));

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

/******************************************************************************\
|******************** Supporting Functions for Optimization *******************|
\******************************************************************************/

/*******************************************************************************
**							gcOpt_RemoveNOP
********************************************************************************
**
**	Remove NOP instructions from the shader.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
*/
gceSTATUS
gcOpt_RemoveNOP(
	IN gcOPTIMIZER		Optimizer
	)
{
	gceSTATUS			status = gcvSTATUS_OK;
    gcOPT_CODE          code, codePrev;
    gcOPT_CODE          nextNonNOPCode = gcvNULL;
    gctUINT				i;

	gcmHEADER_ARG("Optimizer=%p", Optimizer);

	/* Update codeHead and codeTail of main and fucntions. */
    if (Optimizer->main->codeHead->instruction.opcode == gcSL_NOP)
    {
        code = Optimizer->main->codeHead;
        do
        {
            code = code->next;
        }
        while (code != gcvNULL && code->instruction.opcode == gcSL_NOP);
		gcmASSERT(code != gcvNULL);
        gcmASSERT(code->id <= Optimizer->main->codeTail->id);
        Optimizer->main->codeHead = code;
    }

    if (Optimizer->main->codeTail->instruction.opcode == gcSL_NOP)
    {
        code = Optimizer->main->codeTail;
        do
        {
            code = code->prev;
        }
        while (code != gcvNULL && code->instruction.opcode == gcSL_NOP);
		gcmASSERT(code != gcvNULL);
        gcmASSERT(code->id >= Optimizer->main->codeHead->id);
        Optimizer->main->codeTail = code;
    }

    if (Optimizer->functionCount > 0)
    {
	    gcOPT_FUNCTION function;

        function = Optimizer->functionArray;
        for (i = 0; i < Optimizer->functionCount; i++, function++)
        {
            if (function->codeHead->instruction.opcode == gcSL_NOP)
            {
                code = function->codeHead;
                do
                {
                    code = code->next;
                }
                while (code != gcvNULL && code->instruction.opcode == gcSL_NOP);
				gcmASSERT(code != gcvNULL);
                gcmASSERT(code->id <= function->codeTail->id);
                function->codeHead = code;
            }

            if (function->codeTail->instruction.opcode == gcSL_NOP)
            {
                code = function->codeTail;
                do
                {
                    code = code->prev;
                }
                while (code != gcvNULL && code->instruction.opcode == gcSL_NOP);
				gcmASSERT(code != gcvNULL);
                gcmASSERT(code->id >= function->codeHead->id);
                function->codeTail = code;
            }
        }
    }

    /* Remove NOP codes and update callers. */
    for (code = Optimizer->codeTail; code; code = codePrev)
    {
        codePrev = code->prev;
        if (code->instruction.opcode != gcSL_NOP)
        {
            nextNonNOPCode = code;
        }
        else
        {
            if (code->callers)
            {
			    gcOPT_LIST caller;
                gcOPT_LIST lastCaller = gcvNULL;

                /* Update NOP instruction's callers. */
                if (! nextNonNOPCode)
                {
                    gcmASSERT(nextNonNOPCode);
                    status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
                    return status;
                }

		        for (caller = code->callers;
			         caller != gcvNULL;
			         caller = caller->next)
		        {
                    gcmASSERT(caller->code->callee == code);
			        caller->code->callee = nextNonNOPCode;
			        lastCaller = caller;
		        }

                if (! lastCaller)
                {
                    gcmASSERT(lastCaller);
                    status = gcvSTATUS_INVALID_ARGUMENT;
                    gcmFOOTER();
                    return status;
                }

                /* Move caller list from code to nextNonNOPCode. */
		        lastCaller->next = nextNonNOPCode->callers;
		        nextNonNOPCode->callers = code->callers;
		        code->callers = gcvNULL;
            }

            gcOpt_RemoveCodeList(Optimizer, code, code);
        }
    }

	/* Update code id. */
	gcOpt_UpdateCodeId(Optimizer);

	DUMP_OPTIMIZER("Removed NOP instructions from the shader", Optimizer);
	gcmFOOTER();
	return status;
}

/*******************************************************************************
**							gcOpt_ChangeCodeToNOP
********************************************************************************
**
**	Change one code to NOP.
**
**	INPUT:
**
**		gcOPTIMIZER Optimizer
**			Pointer to a gcOPTIMIZER structure.
**
**		gcSL_INSTRUCTION Code
**			Pointer to code array.
*/
gceSTATUS
gcOpt_ChangeCodeToNOP(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_CODE       Code
	)
{
    gcSL_INSTRUCTION	instr = &Code->instruction;
	gcOPT_LIST			list;

	gcmHEADER_ARG("Optimizer=%p Code=%p", Optimizer, Code);

	/* Update jumpTarget's caller list if needed. */
	if (instr->opcode == gcSL_JMP || instr->opcode == gcSL_CALL)
	{
		gcOPT_LIST caller, prevCaller = gcvNULL;

		/* Remove Index from caller list. */
        for (caller = Code->callee->callers;
			 caller != gcvNULL;
			 caller = caller->next)
		{
			if (caller->code == Code)
			{
				if (prevCaller)
				{
					prevCaller->next = caller->next;
				}
				else
				{
					Code->callee->callers = caller->next;
				}

				gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, caller));
				break;
			}
			prevCaller = caller;
		}
        Code->callee = gcvNULL;
	}

	/* Update previous and next define lists. */
	for (list = Code->prevDefines; list; list = list->next)
	{
		if (list->index >= 0)
		{
			gcOpt_DeleteCodeFromList(Optimizer, &list->code->nextDefines, Code);
		}
	}

	for (list = Code->nextDefines; list; list = list->next)
	{
		gcmASSERT(list->index >= 0);
		gcOpt_DeleteCodeFromList(Optimizer, &list->code->prevDefines, Code);
	}

	if (Code->prevDefines && Code->nextDefines)
	{
		for (list = Code->prevDefines; list; list = list->next)
		{
			if (list->index >= 0)
			{
				gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, Code->nextDefines, &list->code->nextDefines));
			}
		}

		for (list = Code->nextDefines; list; list = list->next)
		{
			gcmASSERT(list->index >= 0);
			gcmVERIFY_OK(gcOpt_AddListToList(Optimizer, Code->prevDefines, &list->code->prevDefines));
		}
	}

	/* Clean up the data flow of the instruction. */
	while ((list = Code->users) != gcvNULL)
	{
		Code->users = list->next;

		if (list->index >= 0)
		{
            /* Cannot add gcmVERIFY_OK here. */
			gcOpt_DeleteCodeFromList(Optimizer, &list->code->dependencies0, Code);
			gcOpt_DeleteCodeFromList(Optimizer, &list->code->dependencies1, Code);
		}

		/* Free the gcOPT_LIST structure. */
		gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
	}

	while ((list = Code->dependencies0) != gcvNULL)
	{
		Code->dependencies0 = list->next;

		if (list->index >= 0)
		{
			gcmVERIFY_OK(gcOpt_DeleteCodeFromList(Optimizer, &list->code->users, Code));
		}

		/* Free the gcOPT_LIST structure. */
		gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
	}

	while ((list = Code->dependencies1) != gcvNULL)
	{
		Code->dependencies1 = list->next;

		/* It is possible that both sources are the same. */
		if (list->index >= 0)
		{
			gcOpt_DeleteCodeFromList(Optimizer, &list->code->users, Code);
		}

		/* Free the gcOPT_LIST structure. */
		gcmVERIFY_OK(_FreeList(Optimizer->listMemPool, list));
	}

	if (Code->nextDefines)
	{
		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &Code->nextDefines));
	}

	if (Code->prevDefines)
	{
		gcmVERIFY_OK(gcOpt_DestroyList(Optimizer, &Code->prevDefines));
	}

	/* Make the instruction NOP. */
	Code->instruction = gcvSL_NOP_INSTR;

	gcmFOOTER_NO();
	return gcvSTATUS_OK;
}

#endif /* VIVANTE_NO_3D */
