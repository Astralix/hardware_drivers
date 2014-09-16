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




#ifndef __gc_glff_cull_h_
#define __gc_glff_cull_h_

#ifdef __cplusplus
extern "C" {
#endif

gceSTATUS glfSetDefaultCullingStates(
	glsCONTEXT_PTR Context
	);

GLenum glfEnableCulling(
	glsCONTEXT_PTR Context,
	GLboolean Enable
	);

GLboolean glfQueryCullState(
	glsCONTEXT_PTR Context,
	GLenum Name,
	GLvoid* Value,
	gleTYPE Type
	);

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_cull_h_ */
