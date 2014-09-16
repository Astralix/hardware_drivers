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




#ifndef __gc_glff_framebuffer_h_
#define __gc_glff_framebuffer_h_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
******************************** Buffer structure. *****************************
\******************************************************************************/

typedef struct _glsFRAME_BUFFER_ATTACHMENT * glsFRAME_BUFFER_ATTACHMENT_PTR;
typedef struct _glsFRAME_BUFFER_ATTACHMENT
{
	gctBOOL						texture;
	gctPOINTER					object;
	gcoSURF						surface;
	gctUINT32					offset;
	gcoSURF						target;
}
glsFRAME_BUFFER_ATTACHMENT;

typedef struct _glsFRAME_BUFFER * glsFRAME_BUFFER_PTR;
typedef struct _glsFRAME_BUFFER
{
	gctBOOL						dirty;
	GLenum						completeness;
	gctBOOL						needResolve;

	glsFRAME_BUFFER_ATTACHMENT	color;
	glsFRAME_BUFFER_ATTACHMENT	depth;
	glsFRAME_BUFFER_ATTACHMENT	stencil;
}
glsFRAME_BUFFER;


/******************************************************************************\
*********************************** Buffer API. ********************************
\******************************************************************************/

GLenum glfIsFramebufferComplete(
	IN glsCONTEXT_PTR Context
	);

gcoSURF glfGetFramebufferSurface(
	glsFRAME_BUFFER_ATTACHMENT_PTR Attachment
	);

gceSTATUS glfUpdateFrameBuffer(
    IN glsCONTEXT_PTR Context
    );

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_framebuffer_h_ */
