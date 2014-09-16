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




#ifndef __gc_glff_renderbuffer_h_
#define __gc_glff_renderbuffer_h_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************\
****************************** EGL image attributes. ***************************
\******************************************************************************/

typedef struct _glsEGL_IMAGE_ATTRIBUTES * glsEGL_IMAGE_ATTRIBUTES_PTR;
typedef struct _glsEGL_IMAGE_ATTRIBUTES
{
    gcoSURF                 surface;
    /* On Android, we use 'appSurface' to resolve into "surface" for CPU apps. */
    gcoSURF                 appSurface;
    /* On Android, we use 'privHandle' to find out information about compositor clients. */
    gctPOINTER              privHandle;
    gceSURF_FORMAT          format;
    gctUINT                 width;
    gctUINT                 height;
    gctINT                  stride;
    gctINT                  level;
    gctPOINTER              pixel;
}
glsEGL_IMAGE_ATTRIBUTES;


/******************************************************************************\
******************************** Buffer structure. *****************************
\******************************************************************************/

typedef struct _glsRENDER_BUFFER * glsRENDER_BUFFER_PTR;
typedef struct _glsRENDER_BUFFER
{
    gctBOOL                 bound;

    GLsizei                 width;
    GLsizei                 height;
    GLenum                  format;

    gcoSURF                 surface;

    glsRENDER_BUFFER_PTR    combined;
    GLint                   count;
}
glsRENDER_BUFFER;


/******************************************************************************\
*********************************** Buffer API. ********************************
\******************************************************************************/

gceSTATUS glfGetEGLImageAttributes(
    khrEGL_IMAGE_PTR Image,
    glsEGL_IMAGE_ATTRIBUTES_PTR Attributes
    );

EGLenum
glfCreateImageRenderBuffer(
    GLenum RenderBuffer,
    gctPOINTER Image
    );

#ifdef __cplusplus
}
#endif

#endif /* __gc_glff_renderbuffer_h_ */
