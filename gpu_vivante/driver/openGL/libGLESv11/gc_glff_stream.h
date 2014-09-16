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




#ifndef __gc_glff_vertex_h_
#define __gc_glff_vertex_h_

#ifdef __cplusplus
extern "C" {
#endif

#define gldSTREAM_POOL_SIZE      128
#define gldSTREAM_GROUP_SIZE     16
#define gldSTREAM_SIGNAL_NUM \
    ( \
        (gldSTREAM_POOL_SIZE + gldSTREAM_GROUP_SIZE - 1) / gldSTREAM_GROUP_SIZE \
    )

typedef struct _glsSTREAM * glsSTREAM_PTR;
typedef struct _glsSTREAM
{
    gctINT                  attributeCount;
    gcoVERTEX               vertex;
}
glsSTREAM;

typedef struct _glsSTREAMINFO * glsSTREAMINFO_PTR;
typedef struct _glsSTREAMINFO
{
    glsATTRIBUTEINFO_PTR    attribute;
    gcoSTREAM               stream;
}
glsSTREAMINFO;

void glfSetStreamParameters(
    glsCONTEXT_PTR Context,
    glsATTRIBUTEINFO_PTR Stream,
    GLenum Type,
    GLint Components,
    GLsizei Stride,
    gctBOOL Normalize,
    const GLvoid* Pointer,
    const glsNAMEDOBJECT_PTR Buffer,
    gleBUFFERBINDINGS BindingType
    );

gceSTATUS glfInitializeStreams(
    glsCONTEXT_PTR Context
    );

GLboolean glfQueryVertexState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    );

void _FreeStream(
    IN glsCONTEXT_PTR Context,
    IN glsSTREAM_PTR Stream
    );

gceSTATUS _BuildStream(
    IN glsCONTEXT_PTR   Context,
    IN GLint            First,
    IN GLsizei          Count,
    IN GLsizei          IndexCount,
    IN gceINDEX_TYPE    IndexType,
    IN const GLvoid     *IndexBuffer,
    OUT glsSTREAM_PTR   Stream,
    OUT GLint           *Start
    );


#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_vertex_h_ */
