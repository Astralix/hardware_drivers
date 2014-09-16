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


#ifndef __gc_glsh_util_
#define __gc_glsh_util_

#define _SWIZZLE_X		gcSL_SWIZZLE_XXXX
#define _SWIZZLE_Y		gcSL_SWIZZLE_YYYY
#define _SWIZZLE_Z		gcSL_SWIZZLE_ZZZZ
#define _SWIZZLE_W		gcSL_SWIZZLE_WWWW
#define _SWIZZLE_XY		gcSL_SWIZZLE_XYYY
#define _SWIZZLE_XYZ	gcSL_SWIZZLE_XYZZ
#define _SWIZZLE_XYZW	gcSL_SWIZZLE_XYZW
#define _SWIZZLE_YZW	gcSL_SWIZZLE_YZWW
#define _SWIZZLE_ZW		gcSL_SWIZZLE_ZWWW

#define ADD_ATTRIBUTE(name,type) \
	gcmONERROR(gcSHADER_AddAttribute(Shader, \
									#name, \
									gcSHADER_##type, \
									1, \
									gcvFALSE, \
									&name))

#define ADD_ATTRIBUTE_TX(name,type) \
	gcmONERROR(gcSHADER_AddAttribute(Shader, \
									#name, \
									gcSHADER_##type, \
									1, \
									gcvTRUE, \
									&name))

#define ADD_ATTRIBUTE_ARRAY(name,type,length) \
	gcmONERROR(gcSHADER_AddAttribute(Shader, \
									#name, \
									gcSHADER_##type, \
									length, \
									gcvFALSE, \
									&name))

#define ADD_ATTRIBUTE_ARRAY_TX(name,type,length) \
	gcmONERROR(gcSHADER_AddAttribute(Shader, \
									#name, \
									gcSHADER_##type, \
									length, \
									gcvTRUE, \
									&name))

#define ADD_UNIFORM(name,type) \
	gcmONERROR(gcSHADER_AddUniform(Shader, \
								  #name, \
								  gcSHADER_##type, \
								  1, \
								  &name))

#define ADD_UNIFORM_ARRAY(name,type,length) \
	gcmONERROR(gcSHADER_AddUniform(Shader, \
								  #name, \
								  gcSHADER_##type, \
								  length, \
								  &name))

#define POSITION(reg) \
	gcmONERROR(gcSHADER_AddOutput(Shader, \
								 "#Position", \
								 gcSHADER_FLOAT_X4, \
								 1, \
								 reg))

#define COLOR(reg) \
	gcmONERROR(gcSHADER_AddOutput(Shader, \
								 "#Color", \
								 gcSHADER_FLOAT_X4, \
								 1, \
								 reg))

#define ADD_OUTPUT(name,type) \
	gcmONERROR(gcSHADER_AddOutput(Shader, \
								 #name, \
								 gcSHADER_##type, \
								 1, \
								 (gctUINT16) -1))

#define ADD_OUTPUT_ARRAY(name,type,length) \
	gcmONERROR(gcSHADER_AddOutput(Shader, \
								 #name, \
								 gcSHADER_##type, \
								 length, \
								 (gctUINT16) -1))

#define LABEL(lbl) \
	gcmONERROR(gcSHADER_AddLabel(Shader, lbl))

#define OPCODE(op,reg,enable) \
	gcmONERROR(gcSHADER_AddOpcode(Shader, \
									gcSL_##op, \
									reg, \
									gcSL_ENABLE_##enable, \
									gcSL_FLOAT))

#define OPCODE_COND_ENABLE(op,cond,reg,enable) \
	gcmONERROR(gcSHADER_AddOpcode2(Shader, \
									 gcSL_##op, \
									 gcSL_##cond, \
									 reg, \
									 gcSL_ENABLE_##enable, \
									 gcSL_FLOAT))

#define OPCODE_COND(op,cond,target) \
	gcmONERROR(gcSHADER_AddOpcodeConditional(Shader, \
											gcSL_##op, \
											gcSL_##cond, \
											target))

#define UNIFORM(name,swizzle) \
	gcmONERROR(gcSHADER_AddSourceUniform(Shader, \
										name, \
										_SWIZZLE_##swizzle, \
										0))

#define UNIFORM_ARRAY(name,swizzle,index) \
	gcmONERROR(gcSHADER_AddSourceUniform(Shader, \
										name, \
										_SWIZZLE_##swizzle, \
										index))

#define UNIFORM_ARRAY_INDEX(name,swizzle,index,reg,mode) \
	gcmONERROR(gcSHADER_AddSourceUniformIndexed(Shader, \
											   name, \
											   _SWIZZLE_##swizzle, \
											   index, \
											   gcSL_INDEXED_##mode, \
											   reg))

#define SAMPLER(name) \
	do { \
		gctUINT32 index; \
		gcmONERROR(gcUNIFORM_GetSampler(name, &index)); \
		CONST((gctFLOAT) index); \
	} while (gcvFALSE)

#define SAMPLER_INDEX(swizzle,reg,mode) \
	gcmONERROR(gcSHADER_AddSourceSamplerIndexed(Shader, \
												  _SWIZZLE_##swizzle, \
												  gcSL_INDEXED_##mode, \
												  reg))

#define ATTRIBUTE(name,swizzle) \
	gcmONERROR(gcSHADER_AddSourceAttribute(Shader, \
										  name, \
										  _SWIZZLE_##swizzle, \
										  0))

#define ATTRIBUTE_ARRAY(name,swizzle,index) \
	gcmONERROR(gcSHADER_AddSourceAttribute(Shader, \
										  name, \
										  _SWIZZLE_##swizzle, \
										  index))

#define ATTRIBUTE_ARRAY_INDEX(name,swizzle,index,reg,mode) \
	gcmONERROR(gcSHADER_AddSourceAttributeIndexed(Shader, \
												 name, \
												 _SWIZZLE_##swizzle, \
												 index, \
												 gcSL_INDEXED_##mode, \
												 reg))
#ifdef CONST
#	undef CONST
#endif

#define CONST(c) \
	gcmONERROR(gcSHADER_AddSourceConstant(Shader, c))

#define TEMP(reg, swizzle) \
	gcmONERROR(gcSHADER_AddSource(Shader, \
								 gcSL_TEMP, \
								 reg, \
								 _SWIZZLE_##swizzle, \
								 gcSL_FLOAT))

#define OUTPUT(name,temp) \
	gcmONERROR(gcSHADER_AddOutputIndexed(Shader, \
										#name, \
										0, \
										temp))

#define OUTPUT_ARRAY(name,temp,index) \
	gcmONERROR(gcSHADER_AddOutputIndexed(Shader, \
										#name, \
										index, \
										temp))

#define PACK() \
	gcmONERROR(gcSHADER_Pack(Shader))

#define ADD_FUNCTION(func) \
	gcmONERROR(gcSHADER_AddFunction(Shader, #func, &func))

#define ADD_FUNCTION_ARGUMENT(func,temp,enable,qualifier) \
	gcmONERROR(gcFUNCTION_AddArgument(func, temp, gcSL_ENABLE_##enable, qualifier))

#define BEGIN_FUNCTION(func) \
	gcmONERROR(gcSHADER_BeginFunction(Shader, func))

#define END_FUNCTION(func) \
	gcmONERROR(gcSHADER_EndFunction(Shader, func))

#define FUNCTION_INPUT(func,index) \
	do { \
		gctUINT16 temp; \
		gctUINT8 enable; \
		gcmONERROR(gcFUNCTION_GetArgument(func, \
											index, \
											&temp, \
											&enable, \
											gcvNULL)); \
		gcmONERROR(gcSHADER_AddOpcode(Shader, \
										gcSL_MOV, \
										temp, \
										enable, \
										gcSL_FLOAT)); \
	} while (gcvFALSE)

#define CALL(func,cond) \
	do { \
		gctUINT label; \
		gcmONERROR(gcFUNCTION_GetLabel(func, &label)); \
		gcmONERROR(gcSHADER_AddOpcodeConditional(Shader, \
												   gcSL_CALL, \
												   gcSL_##cond, \
												   label)); \
	} while (gcvFALSE)

#define FUNCTION_OUTPUT(func,index) \
	gcmONERROR(gcSHADER_AddSource(Shader, \
								 gcSL_TEMP, \
								 gcFUNCTION_GetTemp(func, index), \
								 gcFUNCTION_GetSwizzle(func, index), \
								 gcSL_FLOAT))

#endif /* __gc_glsh_util_ */
