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
**	Include file the defines the front- and back-end compilers, as well as the
**	objects they use.
*/

#ifndef __gc_hal_optimizer_h_
#define __gc_hal_optimizer_h_
#ifndef VIVANTE_NO_3D

#include "gc_hal_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define __DUMP_OPTIMIZER__			0

#define __SUPPORT_OCL__             1

/*******************************************************************************
**							gcOptimizer Constants
*******************************************************************************/
#define gcvOPT_INPUT_REGISTER		-1
#define gcvOPT_OUTPUT_REGISTER		-2
#define gcvOPT_GLOBAL_REGISTER		-3

/*******************************************************************************
**							gcOptimizer Data Structures
*******************************************************************************/
typedef enum _gceSHADER_OPTIMIZATION
{
    /*  No optimization. */
	gcvOPTIMIZATION_NONE,

    /*  Flow graph construction. */
	gcvOPTIMIZATION_CONSTRUCTION                = 1 << 0,

    /*  Dead code elimination. */
	gcvOPTIMIZATION_DEAD_CODE                   = 1 << 1,

    /*  Redundant move instruction elimination. */
	gcvOPTIMIZATION_REDUNDANT_MOVE              = 1 << 2,

    /*  Inline expansion. */
	gcvOPTIMIZATION_INLINE_EXPANSION            = 1 << 3,

    /*  Constant propagation. */
	gcvOPTIMIZATION_CONSTANT_PROPAGATION        = 1 << 4,

    /*  Redundant bounds/checking elimination. */
	gcvOPTIMIZATION_REDUNDANT_CHECKING          = 1 << 5,

    /*  Loop invariant movement. */
	gcvOPTIMIZATION_LOOP_INVARIANT              = 1 << 6,

    /*  Induction variable removal. */
	gcvOPTIMIZATION_INDUCTION_VARIABLE          = 1 << 7,

    /*  Common subexpression elimination. */
	gcvOPTIMIZATION_COMMON_SUBEXPRESSION        = 1 << 8,

    /*  Control flow/banch optimization. */
	gcvOPTIMIZATION_CONTROL_FLOW                = 1 << 9,

    /*  Vector component operation merge. */
	gcvOPTIMIZATION_VECTOR_INSTRUCTION_MERGE    = 1 << 10,

    /*  Algebra simplificaton. */
	gcvOPTIMIZATION_ALGEBRAIC_SIMPLIFICATION    = 1 << 11,

    /*  Pattern matching and replacing. */
	gcvOPTIMIZATION_PATTERN_MATCHING            = 1 << 12,

    /*  Interprocedural constant propagation. */
	gcvOPTIMIZATION_IP_CONSTANT_PROPAGATION     = 1 << 13,

    /*  Interprecedural register optimization. */
	gcvOPTIMIZATION_IP_REGISTRATION             = 1 << 14,

    /*  Optimization option number. */
	gcvOPTIMIZATION_OPTION_NUMBER               = 1 << 15,

	/*  Loadtime constant. */
    gcvOPTIMIZATION_LOADTIME_CONSTANT           = 1 << 16,

    /*  MAD instruction optimization. */
	gcvOPTIMIZATION_MAD_INSTRUCTION             = 1 << 17,

    /*  Special optimization for LOAD SW workaround. */
	gcvOPTIMIZATION_LOAD_SW_WORKAROUND          = 1 << 18,

    /*  Full optimization. */
    /*  Note that gcvOPTIMIZATION_LOAD_SW_WORKAROUND is off. */
	gcvOPTIMIZATION_FULL                        = 0x7FFBFFFF,

	/* Optimization Unit Test flag. */
    gcvOPTIMIZATION_UNIT_TEST                   = 1 << 31
}
gceSHADER_OPTIMIZATION;

#define gcdHasOptimization(Flags, Opt) ((Flags & Opt) == Opt)

typedef struct _gcOPTIMIZER *			gcOPTIMIZER;
typedef struct _gcOPT_LIST *			gcOPT_LIST;
typedef struct _gcOPT_TEMP_DEFINE *		gcOPT_TEMP_DEFINE;
typedef struct _gcOPT_CODE *			gcOPT_CODE;
typedef struct _gcOPT_TEMP *			gcOPT_TEMP;
typedef struct _gcOPT_GLOBAL_USAGE *	gcOPT_GLOBAL_USAGE;
typedef struct _gcOPT_FUNCTION *		gcOPT_FUNCTION;

/*******************************************************************************
**							Data Flow Data Structures
*******************************************************************************/
/* Structure that defines the linked list of dependencies. */
struct _gcOPT_LIST
{
	/* Pointer to next dependent register. */
	gcOPT_LIST					next;

	/* Index of instruction. */
	/*		-1:		input variable/register. */
	/*		-2:		output variable/register. */
	/*		-3:		global variable/register. */
	/*	others:		instruction pointer. */
	gctINT						index;

    /* Pointer to code. */
    gcOPT_CODE                  code;
};

struct _gcOPT_TEMP_DEFINE
{
	gcOPT_LIST					xDefines;
	gcOPT_LIST					yDefines;
	gcOPT_LIST					zDefines;
	gcOPT_LIST					wDefines;
};

struct _gcOPT_CODE
{
    /* Next and Prev for linked list. */
    gcOPT_CODE                  next;
    gcOPT_CODE                  prev;

    /* Number sequence of the code. */
    gctUINT                     id;

    /* Instruction. */
    struct _gcSL_INSTRUCTION    instruction;

    /* Pointer to the function to which this code belongs. */
	gcOPT_FUNCTION				function;

	/* Callers/jumpers to this instruction. */
	gcOPT_LIST					callers;

    /* Call/jump target code. */
    gcOPT_CODE                  callee;

    /* Flags for data flow construction. */
    gctBOOL                     backwardJump;
    gctBOOL                     handled;

#if GC_ENABLE_LOADTIME_OPT
    /* the index to Loadtime Expression array */
    gctINT                      ltcArrayIdx;
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Temp define info for label. */
    gcOPT_TEMP_DEFINE           tempDefine;

	/* Dependencies for the instruction. */
	gcOPT_LIST					dependencies0;
	gcOPT_LIST					dependencies1;

	/* Users of the instruction. */
	gcOPT_LIST					users;

	/* Previous define instructions. */
	gcOPT_LIST					prevDefines;

	/* Next define instructions. */
	gcOPT_LIST					nextDefines;
};

/* Structure that defines the entire life and dependency for a shader. */
struct _gcOPT_TEMP
{
	/* In-use flag. */
	gctBOOL						inUse;

	/* Is-global flag. */
	gctBOOL						isGlobal;

	/* True if the register is used as an index. */
	gctBOOL						isIndex;

	/* Usage flags for the temporary register. */
	gctUINT8					usage;

	/* Data format for the temporary register. */
	gctINT						format;     /* -1: unknown, -2: union. */

	/* Array variable if the register is part of the array. */
	gcVARIABLE					arrayVariable;

	/* Pointer to the function if used as that function's argument. */
	gcOPT_FUNCTION				function;
	gcsFUNCTION_ARGUMENT_PTR	argument;

	/* Temp interger. */
	gctINT						tempInt;

	/* Temp pointer. */
	gctPOINTER					temp;
};

/* Structure that save the usage of global variables in function. */
struct _gcOPT_GLOBAL_USAGE
{
	/* Pointer to next global usage. */
	gcOPT_GLOBAL_USAGE			next;

	/* Global register index. */
	gctUINT						index;

	/* Usage direction in function. */
	gceINPUT_OUTPUT				direction;
};

struct _gcOPT_FUNCTION
{
    /* Code list. */
    gcOPT_CODE                  codeHead;
    gcOPT_CODE                  codeTail;

	/* Global variable usages. */
	gcOPT_GLOBAL_USAGE			globalUsage;

	/* Pointer to shader function. */
	gcFUNCTION					shaderFunction;

	/* Pointer to shader function. */
	gcKERNEL_FUNCTION			kernelFunction;

	/* Number of arguments. */
	gctSIZE_T					argumentCount;

	/* Pointer to shader's function's arguments. */
	gcsFUNCTION_ARGUMENT_PTR	arguments;

    /* Updated for global usage. */
    gctBOOL                     updated;
};

#if GC_ENABLE_LOADTIME_OPT

/* allocator/deallocator function pointer */
typedef gceSTATUS (*gctAllocatorFunc)(
    IN gcoOS Os,
    IN gctSIZE_T Bytes,
    OUT gctPOINTER * Memory
    );

typedef gceSTATUS (*gctDeallocatorFunc)(
    IN gcoOS Os,
    IN gctPOINTER Memory
    );

typedef gctBOOL (*compareFunc) (
     IN void *    data,
     IN void *    key
     );

typedef struct _gcsListNode gcsListNode;
struct _gcsListNode
{
    gcsListNode *       next;
    void *              data;
};

typedef struct _gcsAllocator
{
    gctAllocatorFunc    allocate;
    gctDeallocatorFunc  deallocate;
} gcsAllocator;

typedef struct _gcsList
{
    gcsListNode  *head;
    gcsListNode  *tail;
    gctINT        count;
    gcsAllocator *allocator;
} gcsList;

/* simple map structure */
typedef struct _SimpleMap SimpleMap;
struct _SimpleMap
{
    gctUINT32  key;
    gctUINT32  val;
    SimpleMap  *next;
};

/*  link list structure for code list */
typedef gcsList gcsCodeList;
typedef gcsCodeList * gctCodeList;
typedef gcsListNode gcsCodeListNode;

/*  link list structure for code list */
typedef gcsList gcsTempRegisterList;
typedef gcsTempRegisterList * gctTempRegisterList;
typedef gcsListNode gcsTempRegisterListNode;

#define ltcRegisterWithComponents(RegIndex, Components)             \
          (((Components) << 16) | (gctUINT16)(RegIndex))

#define ltcGetRegister(Value)   ((gctUINT16)((Value) & 0xff))

#define ltcGetComponents(Value) ((gctUINT16)((Value) >> 16))

#define ltcRemoveComponents(Value, Components)                 \
          (ltcRegisterWithComponents(ltcGetRegister(Value),    \
           ltcGetComponents(Value) & ~(Components)))

#define ltcAddComponents(Value, Components)                    \
          (ltcRegisterWithComponents(ltcGetRegister(Value),    \
           ltcGetComponents(Value) | (Components)))

#define ltcHasComponents(Value, Components)                    \
          (ltcGetComponents(Value) & (Components)))

typedef struct _gcsLTCExpression gcsLTCExpression;

struct _gcsLTCExpression
{
    gctINT             flag;
    gctFLOAT *         theValue;     /* the pointer to the calculated constant value */
    gctINT             codeCount;    /* number of instructions in the expr */
    gcSL_INSTRUCTION   code_list;
    gcsLTCExpression * next;
} ;


typedef struct _gcsDummyUniformInfo
{
   gctINT                 shaderKind;   /* vertex or fragment shader */
   gcUNIFORM *            uniform;      /* pointer to the load-time constant uniform */
   gcsLTCExpression *     expr;         /* the expression used to evaluate the uniform value at
									       load-time */
   gctINT                 codeIndex;    /* the code index in LTCExpression for the dummy uniform */
   struct _gcsDummyUniformInfo *  next;
} gcsDummyUniformInfo;

gceSTATUS
gcOPT_OptimizeLoadtimeConstant(
    IN gcOPTIMIZER Optimizer
    );

extern gcsAllocator ltcAllocator;
#endif /* GC_ENABLE_LOADTIME_OPT */


struct _gcOPTIMIZER
{
	/* Pointer to the gcSHADER object. */
	gcSHADER					shader;

	/* Number of instructons in shader. */
	gctUINT						codeCount;

    /* Code list. */
    gcOPT_CODE                  codeHead;
    gcOPT_CODE                  codeTail;

    /* Code list. */
    gcOPT_CODE                  freeCodeList;

	/* Number of temporary registers. */
	gctUINT						tempCount;

	/* Temporary registers. */
	gcOPT_TEMP					tempArray;

	/* Main program. */
	gcOPT_FUNCTION				main;

	/* Number of functions. */
	gctUINT						functionCount;

	/* Function array. */
	gcOPT_FUNCTION				functionArray;

	/* Current function being processed. */
	gcOPT_FUNCTION				currentFunction;

	/* Global variables. */
	gcOPT_LIST					global;

	/* Points back to shader's outputs. */
	gctSIZE_T					outputCount;
	gcOUTPUT *					outputs;

    /* Kernel function is merged with main. */
    gctBOOL                     isMainMergeWithKerenel;

#if GC_ENABLE_LOADTIME_OPT
    gcsTempRegisterList         theLTCTempRegList;
    gcsCodeList                 theLTCCodeList;
#endif /* GC_ENABLE_LOADTIME_OPT */

	/* Memory pools. */
	gcsMEM_FS_MEM_POOL			codeMemPool;
	gcsMEM_FS_MEM_POOL			listMemPool;
	gcsMEM_FS_MEM_POOL			usageMemPool;

	gcsMEM_AFS_MEM_POOL			functionArrayMemPool;
	gcsMEM_AFS_MEM_POOL			codeArrayMemPool;
	gcsMEM_AFS_MEM_POOL			tempArrayMemPool;
	gcsMEM_AFS_MEM_POOL			tempDefineArrayMemPool;

	/* Log file. */
	gctFILE						logFile;
};

#define gcvINSTR_SIZE	gcmSIZEOF(struct _gcSL_INSTRUCTION)

extern const struct _gcSL_INSTRUCTION gcvSL_NOP_INSTR;

/*******************************************************************************
**							gcOptimizer Supporting Functions
*******************************************************************************/

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
	IN gcSHADER Shader,
	OUT gcOPTIMIZER * Optimizer
	);

/*******************************************************************************
**							gcOpt_DestroyOptimizer
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
	IN OUT gcOPTIMIZER Optimizer
	);

gceSTATUS
gcOpt_CopyInShader(
	IN gcOPTIMIZER Optimizer,
	IN gcSHADER Shader
	);

gceSTATUS
gcOpt_CopyOutShader(
	IN gcOPTIMIZER Optimizer,
	IN gcSHADER Shader
	);

gceSTATUS
gcOpt_RebuildFlowGraph(
	IN gcOPTIMIZER Optimizer
	);

gceSTATUS
gcOpt_RemoveNOP(
	IN gcOPTIMIZER Optimizer
	);

void
gcOpt_MoveCodeListBefore(
	IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       SrcCodeFirst,
    IN gcOPT_CODE       SrcCodeLast,
    IN gcOPT_CODE       DestCode
	);

void
gcOpt_MoveCodeListAfter(
	IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       SrcCodeFirst,
    IN gcOPT_CODE       SrcCodeLast,
    IN gcOPT_CODE       DestCode
	);

gceSTATUS
gcOpt_CopyCodeListAfter(
	IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       SrcCodeFirst,
    IN gcOPT_CODE       SrcCodeLast,
    IN gcOPT_CODE       DestCode
	);

gceSTATUS
gcOpt_RemoveCodeList(
	IN gcOPTIMIZER		Optimizer,
    IN gcOPT_CODE       CodeFirst,
    IN gcOPT_CODE       CodeLast
	);

gceSTATUS
gcOpt_ChangeCodeToNOP(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_CODE       Code
	);

gceSTATUS
gcOpt_DestroyCodeDependency(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
	);

gceSTATUS
gcOpt_DeleteFunction(
	IN gcOPTIMIZER Optimizer,
	IN gcOPT_FUNCTION Function
	);

void
gcOpt_UpdateCodeId(
	IN gcOPTIMIZER		Optimizer
	);

gceSTATUS
gcOpt_AddIndexToList(
	IN gcOPTIMIZER Optimizer,
	IN OUT gcOPT_LIST * Root,
	IN gctINT Index
	);

gceSTATUS
gcOpt_ReplaceIndexInList(
	IN gcOPTIMIZER Optimizer,
	IN OUT gcOPT_LIST * Root,
	IN gctUINT Index,
	IN gctUINT NewIndex
	);

gceSTATUS
gcOpt_DeleteIndexFromList(
	IN gcOPTIMIZER Optimizer,
	IN OUT gcOPT_LIST * Root,
	IN gctUINT Index
	);

gceSTATUS
gcOpt_FreeList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	List
	);

gceSTATUS
gcOpt_AddCodeToList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
	);

gceSTATUS
gcOpt_ReplaceCodeInList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code,
	IN gcOPT_CODE		NewCode
	);

gceSTATUS
gcOpt_CheckCodeInList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
    );

gceSTATUS
gcOpt_DeleteCodeFromList(
	IN gcOPTIMIZER		Optimizer,
	IN OUT gcOPT_LIST *	Root,
	IN gcOPT_CODE		Code
	);

gceSTATUS
gcOpt_AddListToList(
	IN gcOPTIMIZER		Optimizer,
	IN gcOPT_LIST		SrcList,
	IN OUT gcOPT_LIST *	Root
	);

gceSTATUS
gcOpt_DestroyList(
	IN gcOPTIMIZER Optimizer,
	IN OUT gcOPT_LIST * Root
	);

/*******************************************************************************
**								gcOptimizer Logging
*******************************************************************************/
void
gcOpt_Dump(
	IN gctFILE File,
	IN gctCONST_STRING Text,
	IN gcOPTIMIZER Optimizer,
	IN gcSHADER Shader
	);

void
gcOpt_DumpMessage(
	IN gcoOS Os,
	IN gctFILE File,
	IN gctSTRING Message
	);

#if __DUMP_OPTIMIZER__
#define DUMP_OPTIMIZER(Text, Optimizer)		gcOpt_Dump(Optimizer->logFile, Text, Optimizer, gcvNULL)
#define DUMP_SHADER(File, Text, Shader)		gcOpt_Dump(File, Text, gcvNULL, Shader)
#define DUMP_MESSAGE(Os, File, Message)		gcOpt_DumpMessage(Os, File, Message)
#else
#define DUMP_OPTIMIZER(Text, Optimizer)     do { } while (gcvFALSE)
#define DUMP_SHADER(File, Text, Shader)     do { } while (gcvFALSE)
#define DUMP_MESSAGE(Os, File, Message)     do { } while (gcvFALSE)
#endif

/* dump instruction to stdout */
void dbg_dumpIR(gcSL_INSTRUCTION Inst,gctINT n);
/* dump optimizer to stdout */
void dbg_dumpOptimizer(gcOPTIMIZER Optimizer);
/* dump shader to stdout */
void dbg_dumpShader(gcSHADER Shader);

void dbg_dumpCode(gcOPT_CODE Code);

void
gcOpt_GenShader(
	IN gcSHADER Shader,
	IN gctFILE File
	);

#ifdef __cplusplus
}
#endif

#endif /* VIVANTE_NO_3D */
#endif /* __gc_hal_optimizer_h_ */
