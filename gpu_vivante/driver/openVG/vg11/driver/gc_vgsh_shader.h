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


#ifndef __gc_vgsh_shader_h_
#define __gc_vgsh_shader_h_

/*to be careful with the enum value, because the value should be equal to the GLObject_VertexShader, GLObject_FragmentShader
see defines in libGL.h*/
typedef enum _VGShaderType
{
    VGShaderType_Vertex = 1,
    VGShaderType_Fragment = 2
}_VGShaderType;

typedef struct _VGShader
{
    _VGShaderType  shaderType;
    VGint          sourceSize;
    char *		   compileLog;
    gcSHADER       binary;


	gctUINT16	   lLastAllocated;
	gctUINT16	   rLastAllocated;

	_vgUNIFORMWRAP		uniforms[vgvMAX_FRAG_UNIFORMS];
	gctUINT16			uniformCount;

	_vgATTRIBUTEWRAP	attributes[vgvMAX_ATTRIBUTES];
	gctUINT16			attributeCount;

	_vgSAMPLERWRAP		samplers[vgvMAX_SAMPLERS];
	gctUINT16			samplerCount;

	gctBOOL			resolveAttributes;

}_VGShader;


struct _VGProgram
{
    _VGShader   vertexShader;
    _VGShader   fragmentShader;

	gctSIZE_T	        statesSize;
	gctPOINTER	        states;
	gcsHINT_PTR			hints;
};

void _VGProgramCtor(gcoOS os, _VGProgram *program);
void _VGProgramDtor(gcoOS os, _VGProgram *program);


#endif /* __gc_vgsh_shader_h_ */
