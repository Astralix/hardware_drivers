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


#ifndef __gc_glsl_ast_walk_h_
#define __gc_glsl_ast_walk_h_

#include "gc_glsl_ir.h"


/* sloOBJECT_COUNTER */
struct _sloOBJECT_COUNTER
{
	slsVISITOR	visitor;
        gctUINT         attributeCount;
        gctUINT         uniformCount;
        gctUINT         variableCount;
        gctUINT         outputCount;
        gctUINT         functionCount;
};

typedef struct _sloOBJECT_COUNTER *sloOBJECT_COUNTER;

gceSTATUS
sloOBJECT_COUNTER_Construct(
	IN sloCOMPILER Compiler,
	OUT sloOBJECT_COUNTER * ObjectCounter
	);

gceSTATUS
sloOBJECT_COUNTER_Destroy(
	IN sloCOMPILER Compiler,
	IN sloOBJECT_COUNTER ObjectCounter
	);
#endif /* __gc_glsl_ast_walk_h_ */
