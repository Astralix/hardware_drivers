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




#ifndef __gc_glff_buffer_h_
#define __gc_glff_buffer_h_

#ifdef __cplusplus
extern "C" {
#endif

#if gcdUSE_TRIANGLE_STRIP_PATCH
#    define  gldINDEX_MAX_SUBS      64
#endif

/******************************************************************************\
*************************** Possible buffer bindings. **************************
\******************************************************************************/

typedef enum _gleBUFFERBINDINGS
{
	glvARRAYBUFFER = 0,
	glvELEMENTBUFFER,
	glvVERTEXBUFFER,
	glvNORMALBUFFER,
	glvCOLORBUFFER,
	glvPOINTSIZEBUFFER,
	glvTEX0COORDBUFFER,
	glvTEX1COORDBUFFER,
	glvTEX2COORDBUFFER,
	glvTEX3COORDBUFFER,
	glvMATRIXBUFFER,
	glvWEIGHTBUFFER,
	glvTEX0COORDBUFFER_AUX,
	glvTEX1COORDBUFFER_AUX,
	glvTEX2COORDBUFFER_AUX,
	glvTEX3COORDBUFFER_AUX,

	/* Number of possible bindings. */
	glvTOTALBINDINGS
}
gleBUFFERBINDINGS;


/******************************************************************************\
******************************** Buffer structure. *****************************
\******************************************************************************/

#if gcdUSE_TRIANGLE_STRIP_PATCH
struct _GLSubBuffer
{
    gcoINDEX            patchIndex;
    GLint               start;
    GLint               count;
};
#endif

typedef struct _glsBUFFER * glsBUFFER_PTR;
typedef struct _glsBUFFER
{
	gctBOOL					bound;
	glsNAMEDOBJECT_PTR*		binding[glvTOTALBINDINGS];
	GLint					size;
	GLenum					usage;
	gcoINDEX				index;
	gcoSTREAM				stream;
	gctBOOL					boundAtLeastOnce;
	gctUINT					positionCount;
    gctBOOL                 mapped;
    gctPOINTER              bufferMapPointer;

#if gcdUSE_TRIANGLE_STRIP_PATCH
    struct _GLSubBuffer     subs[gldINDEX_MAX_SUBS];
    gctINT                  subCount;
    gctBOOL                 stripPatchDirty;
#endif
}
glsBUFFER;


/******************************************************************************\
*********************************** Buffer API. ********************************
\******************************************************************************/

GLboolean glfQueryBufferState(
	glsCONTEXT_PTR Context,
	GLenum Name,
	GLvoid* Value,
	gleTYPE Type
	);

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_buffer_h_ */
