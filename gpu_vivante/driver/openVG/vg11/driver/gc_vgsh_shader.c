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


#include "gc_vgsh_precomp.h"

/*
ColorType
0.0 s_nonpre
1.0 l_nonpre
2.0 s_pre
3.0 l_pre
*/


void _VGProgramCtor(gcoOS os, _VGProgram *program)
{
	gcmHEADER_ARG("os=0x%x program=0x%x", os, program);

	program->statesSize         = 0;
	program->states             = gcvNULL;
	program->hints              = gcvNULL;

	gcoOS_ZeroMemory(&program->vertexShader, sizeof(_VGShader));
	gcoOS_ZeroMemory(&program->fragmentShader, sizeof(_VGShader));

	gcmFOOTER_NO();
}

void _VGProgramDtor(gcoOS os, _VGProgram *program)
{
	gcmHEADER_ARG("os=0x%x program=0x%x", os, program);

    if (program->vertexShader.binary)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(program->vertexShader.binary));
    }

	if (program->vertexShader.compileLog)
	{
		gcmVERIFY_OK(gcmOS_SAFE_FREE(os, program->vertexShader.compileLog));
	}

    if (program->fragmentShader.binary)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(program->fragmentShader.binary));
    }

	if (program->fragmentShader.compileLog)
	{
		gcmVERIFY_OK(gcmOS_SAFE_FREE(os, program->fragmentShader.compileLog));
	}

	if (program->states)
	{
		gcmVERIFY_OK(gcmOS_SAFE_FREE(os, program->states));
	}

	if (program->hints != gcvNULL)
	{
		gcmVERIFY_OK(gcmOS_SAFE_FREE(os, program->hints));
	}

	gcmFOOTER_NO();
}
