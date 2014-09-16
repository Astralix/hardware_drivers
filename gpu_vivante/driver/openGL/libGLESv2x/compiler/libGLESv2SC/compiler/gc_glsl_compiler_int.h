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


#ifndef __gc_glsl_compiler_int_h_
#define __gc_glsl_compiler_int_h_

#include "gc_glsl_compiler.h"
#include "gc_glsl_preprocessor.h"

/* Debug macros */
/*#define SL_SCAN_ONLY*/
/*#define SL_SCAN_NO_ACTION*/
/*#define SL_SCAN_NO_PREPROCESSOR*/

gceSTATUS
sloCOMPILER_Lock(
	IN sloCOMPILER Compiler
	);

gceSTATUS
sloCOMPILER_Unlock(
	IN sloCOMPILER Compiler
	);

gceSTATUS
sloCOMPILER_EmptyMemoryPool(
	IN sloCOMPILER Compiler
	);

/* Scanner State */
typedef enum _sleSCANNER_STATE
{
	slvSCANNER_NOMRAL		= 0,

	slvSCANNER_AFTER_TYPE
}
sleSCANNER_STATE;

gceSTATUS
sloCOMPILER_SetScannerState(
	IN sloCOMPILER Compiler,
	IN sleSCANNER_STATE State
	);

sleSCANNER_STATE
sloCOMPILER_GetScannerState(
	IN sloCOMPILER Compiler
	);

sloPREPROCESSOR
sloCOMPILER_GetPreprocessor(
	IN sloCOMPILER Compiler
	);

sloCODE_EMITTER
sloCOMPILER_GetCodeEmitter(
	IN sloCOMPILER Compiler
	);

gctBOOL
sloCOMPILER_OptimizationEnabled(
	IN sloCOMPILER Compiler,
	IN sleOPTIMIZATION_OPTION OptimizationOption
	);

gctBOOL
sloCOMPILER_ExtensionEnabled(
	IN sloCOMPILER Compiler,
	IN sleEXTENSION Extension
	);

gceSTATUS
sloCOMPILER_AddLog(
	IN sloCOMPILER Compiler,
	IN gctCONST_STRING Log
	);

gceSTATUS
sloCOMPILER_GetChar(
	IN sloCOMPILER Compiler,
	OUT gctINT_PTR Char
	);

gceSTATUS
sloCOMPILER_MakeCurrent(
	IN sloCOMPILER Compiler,
	IN gctUINT StringCount,
	IN gctCONST_STRING Strings[]
	);

gctINT
slInput(
	IN gctINT MaxSize,
	OUT gctSTRING Buffer
	);

gctPOINTER
slMalloc(
	IN gctSIZE_T Bytes
	);

gctPOINTER
slRealloc(
	IN gctPOINTER Memory,
	IN gctSIZE_T NewBytes
	);

void
slFree(
	IN gctPOINTER Memory
	);

void
slReport(
	IN gctUINT LineNo,
	IN gctUINT StringNo,
	IN sleREPORT_TYPE Type,
	IN gctSTRING Message,
	IN ...
	);

gceSTATUS
sloCOMPILER_LoadingBuiltIns(
	IN sloCOMPILER Compiler,
	IN gctBOOL LoadingBuiltIns
	);

gceSTATUS
sloCOMPILER_MainDefined(
	IN sloCOMPILER Compiler
	);

gceSTATUS
sloCOMPILER_LoadBuiltIns(
	IN sloCOMPILER Compiler
	);

gceSTATUS
sloCOMPILER_Parse(
	IN sloCOMPILER Compiler,
	IN gctUINT StringCount,
	IN gctCONST_STRING Strings[]
	);

gceSTATUS
sloCOMPILER_DumpIR(
	IN sloCOMPILER Compiler
	);

#endif /* __gc_glsl_compiler_int_h_ */
