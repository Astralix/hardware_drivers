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




#ifndef __gc_glff_fog_h_
#define __gc_glff_fog_h_

#ifdef __cplusplus
extern "C" {
#endif

gceSTATUS glfSetDefaultFogStates(
	glsCONTEXT_PTR Context
	);

GLenum glfEnableFog(
	glsCONTEXT_PTR Context,
	GLboolean Enable
	);

void glfUpdateLinearFactors(
	glsCONTEXT_PTR Context
	);

void glfUpdateExpFactors(
	glsCONTEXT_PTR Context
	);

void glfUpdateExp2Factors(
	glsCONTEXT_PTR Context
	);

GLboolean glfQueryFogState(
	glsCONTEXT_PTR Context,
	GLenum Name,
	GLvoid* Value,
	gleTYPE Type
	);

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_fog_h_ */
