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


#include "gc_glsh_precomp.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <KHR/khrvivante.h>

void
_glshInitializeRenderbuffer(
    GLContext Context
    )
{
    /* No renderbuffers bound yet. */
    Context->renderbuffer = gcvNULL;

    /* No frame buffer objects. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->renderbufferObjects,
                                  gcmSIZEOF(Context->renderbufferObjects)));
}

#if gcdNULL_DRIVER < 3
static GLRenderbuffer
_NewRenderbuffer(
    GLContext Context,
    GLuint Name
    )
{
    GLRenderbuffer renderbuffer;
    gctPOINTER pointer = gcvNULL;

    /* Allocate memory for the renderbuffer object. */
    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   gcmSIZEOF(struct _GLRenderbuffer),
                                   &pointer)))
    {
        /* Out of memory error. */
        gcmFATAL("%s(%d): Out of memory", __FUNCTION__, __LINE__);
        gl2mERROR(GL_OUT_OF_MEMORY);
        return gcvNULL;
    }

    renderbuffer = pointer;

    /* Create a new object. */
    if (!_glshInsertObject(&Context->renderbufferObjects,
                           &renderbuffer->object,
                           GLObject_Renderbuffer,
                           Name))
    {
        /* Roll back. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, renderbuffer));

        /* Out of memory error. */
        gcmFATAL("%s(%d): Out of memory", __FUNCTION__, __LINE__);
        gl2mERROR(GL_OUT_OF_MEMORY);
        return gcvNULL;
    }

    /* The renderbuffer object is not yet bound. */
    renderbuffer->width       = 0;
    renderbuffer->height      = 0;
    renderbuffer->format      = GL_NONE;
    renderbuffer->surface     = gcvNULL;
    renderbuffer->combined    = gcvNULL;
    renderbuffer->eglUsed     = GL_FALSE;

    renderbuffer->object.reference = 1;

    /* Return the renderbuffer. */
    return renderbuffer;
}
#endif

void
_glshDereferenceRenderbuffer(
    GLContext Context,
    GLRenderbuffer Renderbuffer
    )
{
    gcmASSERT((Renderbuffer->object.reference - 1) >= 0);

    if (--Renderbuffer->object.reference == 0)
    {
        if (Renderbuffer->surface != gcvNULL)
        {
            if (Renderbuffer->combined == gcvNULL)
            {
                gcmVERIFY_OK(gcoSURF_Destroy(Renderbuffer->surface));
            }
            else
            {
                Renderbuffer->combined->combined = gcvNULL;
            }
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Renderbuffer));
    }
}

void
_glshDeleteRenderbuffer(
    GLContext Context,
    GLRenderbuffer Renderbuffer
    )
{
    /* Remove the renderbuffer from the object list. */
    _glshRemoveObject(&Context->renderbufferObjects, &Renderbuffer->object);

    _glshDereferenceRenderbuffer(Context, Renderbuffer);
}

GL_APICALL void GL_APIENTRY
glGenRenderbuffers(
    GLsizei n,
    GLuint *renderbuffers
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLRenderbuffer renderbuffer;
    GLsizei i;

    gcmHEADER_ARG("n=%d", n);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    if (n < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Loop while there are renderbuffers to generate. */
    for (i = 0; i < n; ++i)
    {
        /* Create a new renderbuffer. */
        renderbuffer = _NewRenderbuffer(context, 0);
        if (renderbuffer == gcvNULL)
        {
            gcmFOOTER_NO();
            return;
        }

        /* Return Renderbuffer name. */
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGenRenderbuffers: ==> %u",
                 renderbuffer->object.name);
        renderbuffers[i] = renderbuffer->object.name;
    }

    gcmDUMP_API("${ES20 glGenRenderbuffers 0x%08X (0x%08X)", n, renderbuffers);
    gcmDUMP_API_ARRAY(renderbuffers, n);
    gcmDUMP_API("$}");
    gcmFOOTER_NO();
#else
    while (n-- > 0)
    {
        *renderbuffers++ = 1;
    }
#endif
}

GL_APICALL void GL_APIENTRY
glBindRenderbuffer(
    GLenum target,
    GLuint renderbuffer
    )
{
#if gcdNULL_DRIVER < 3
    GLRenderbuffer object;

	glmENTER2(glmARGENUM, target, glmARGUINT, renderbuffer)
	{
    gcmDUMP_API("${ES20 glBindRenderbuffer 0x%08X 0x%08X}",
                target, renderbuffer);

    glmPROFILE(context, GLES2_BINDRENDERBUFFER, 0);

    if (target != GL_RENDERBUFFER)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glBindRenderbuffer: Invalid target 0x%04X",
                 target);
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if (renderbuffer == 0)
    {
        /* Remove current binding. */
        object = gcvNULL;
    }
    else
    {
        /* Find the object. */
        object = (GLRenderbuffer) _glshFindObject(
            &context->renderbufferObjects,
            renderbuffer);

        if (object == gcvNULL)
        {
            /* Create a new renderbuffer. */
            object = _NewRenderbuffer(context, renderbuffer);
            if (object == gcvNULL)
            {
                break;
            }
        }
    }

    context->renderbuffer = object;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glDeleteRenderbuffers(
    GLsizei n,
    const GLuint *renderbuffers
    )
{
#if gcdNULL_DRIVER < 3
    GLsizei i;
    GLRenderbuffer renderbuffer;

	glmENTER2(glmARGINT, n, glmARGPTR, renderbuffers)
	{
    gcmDUMP_API("${ES20 glDeleteRenderbuffers 0x%08X (0x%08X)",
                n, renderbuffers);
    gcmDUMP_API_ARRAY(renderbuffers, n);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_DELETERENDERBUFFERS, 0);

    if (n < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    for (i = 0; i < n; i++)
    {
        renderbuffer = (GLRenderbuffer) _glshFindObject(&context->renderbufferObjects,
                                                   renderbuffers[i]);

        if ( (renderbuffer == gcvNULL) ||
            (renderbuffer->object.type != GLObject_Renderbuffer) )
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glDeleteRenderbuffers: Object %d is not a renderbuffer",
                     renderbuffers[i]);
            gl2mERROR(GL_INVALID_VALUE);
            break;
        }

        /* Remove if the renderbuffer is bound to current framebuffer. */
        if (gcvNULL != context->framebuffer)
        {
            GLint type = 0;
            GLint name = 0;

            /* Color0. */
            glGetFramebufferAttachmentParameteriv(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                &type);

            if (GL_RENDERBUFFER == type)
            {
				glGetFramebufferAttachmentParameteriv(
					GL_FRAMEBUFFER,
					GL_COLOR_ATTACHMENT0,
					GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
					&name);
				if(renderbuffers[i] == (GLuint) name)
				{
					glFramebufferRenderbuffer(GL_FRAMEBUFFER,
											  GL_COLOR_ATTACHMENT0,
											  GL_RENDERBUFFER,
											  0);

					/* Disable batch optmization. */
					context->batchDirty = GL_TRUE;
				}
            }

            /* Depth. */
            glGetFramebufferAttachmentParameteriv(
                GL_FRAMEBUFFER,
                GL_DEPTH_ATTACHMENT,
                GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                &type);


            if (GL_RENDERBUFFER == type)
            {
				glGetFramebufferAttachmentParameteriv(
					GL_FRAMEBUFFER,
					GL_DEPTH_ATTACHMENT,
					GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
					&name);
				if(renderbuffers[i] == (GLuint) name)
				{
					glFramebufferRenderbuffer(GL_FRAMEBUFFER,
											  GL_DEPTH_ATTACHMENT,
											  GL_RENDERBUFFER,
											  0);

					/* Disable batch optmization. */
					context->batchDirty = GL_TRUE;
				}
            }

            /* Stencil. */
            glGetFramebufferAttachmentParameteriv(
                GL_FRAMEBUFFER,
                GL_STENCIL_ATTACHMENT,
                GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                &type);


            if (GL_RENDERBUFFER == type)
            {
				glGetFramebufferAttachmentParameteriv(
					GL_FRAMEBUFFER,
					GL_STENCIL_ATTACHMENT,
					GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
					&name);
				if(renderbuffers[i] == (GLuint) name)
				{
					glFramebufferRenderbuffer(GL_FRAMEBUFFER,
											  GL_STENCIL_ATTACHMENT,
											  GL_RENDERBUFFER,
											  0);

					/* Disable batch optmization. */
					context->batchDirty = GL_TRUE;
				}
            }
        }

        if (context->renderbuffer == renderbuffer)
        {
            context->renderbuffer = gcvNULL;

            /* Disable batch optmization. */
            context->batchDirty = GL_TRUE;
        }

        _glshDeleteRenderbuffer(context, renderbuffer);
    }

	}
	glmLEAVE();
#endif
}

GL_APICALL GLboolean GL_APIENTRY
glIsRenderbuffer(
    GLuint renderbuffer
    )
{
#if gcdNULL_DRIVER < 3
    GLObject object;
    GLboolean isRenderbuffer = GL_FALSE;

	glmENTER1(glmARGUINT, renderbuffer)
	{
    glmPROFILE(context, GLES2_ISRENDERBUFFER, 0);

    /* Find the object. */
    object = _glshFindObject(&context->renderbufferObjects, renderbuffer);

    isRenderbuffer = (object != gcvNULL) &&
                     (object->type == GLObject_Renderbuffer);

    gcmTRACE(gcvLEVEL_VERBOSE,
             "glIsRenderbuffer ==> %s",
             isRenderbuffer ? "GL_TRUE" : "GL_FALSE");

    gcmDUMP_API("${ES20 glIsRenderbuffer 0x%08X := 0x%08X}",
                renderbuffer, isRenderbuffer);
	}
	glmLEAVE1(glmARGINT, isRenderbuffer);
    return isRenderbuffer;
#else
    return (renderbuffer == 1) ? GL_TRUE : GL_FALSE;
#endif
}

GL_APICALL void GL_APIENTRY
glRenderbufferStorage(
    GLenum target,
    GLenum internalformat,
    GLsizei width,
    GLsizei height
    )
{
#if gcdNULL_DRIVER < 3
    gceSTATUS status;
    gceSURF_TYPE type = gcvSURF_TYPE_UNKNOWN;
    gceSURF_FORMAT format = gcvSURF_UNKNOWN;
	GLboolean goToEnd = GL_FALSE;

	glmENTER4(glmARGENUM, target, glmARGENUM, internalformat, glmARGINT, width, glmARGINT, height)
	{
    gcmDUMP_API("${ES20 glRenderbufferStorage 0x%08X 0x%08X 0x%08X 0x%08X}",
                target, internalformat, width, height);

    glmPROFILE(context, GLES2_RENDERBUFFERSTORAGE, 0);

    if (target != GL_RENDERBUFFER)
    {
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    if (context->renderbuffer == gcvNULL)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if ( ((gctUINT) width  > context->maxWidth) ||
         ((gctUINT) height > context->maxHeight) )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    context->renderbuffer->width  = width;
    context->renderbuffer->height = height;
    context->renderbuffer->format = internalformat;

    switch (internalformat)
    {
    case GL_RGB8_OES:
        type   = gcvSURF_RENDER_TARGET;
        format = gcvSURF_X8R8G8B8;
        break;

    case GL_RGBA8_OES:
        type   = gcvSURF_RENDER_TARGET;
        format = gcvSURF_A8R8G8B8;
        break;

    case GL_RGB565:
        type   = gcvSURF_RENDER_TARGET;
        format = gcvSURF_R5G6B5;
        break;

    case GL_RGBA4:
        type   = gcvSURF_RENDER_TARGET;
        format = gcvSURF_A4R4G4B4;
        break;

    case GL_RGB5_A1:
        type   = gcvSURF_RENDER_TARGET;
        format = gcvSURF_A1R5G5B5;
        break;

    case GL_DEPTH_COMPONENT16:
        type   = gcvSURF_DEPTH;
        format = gcvSURF_D16;
        break;

    case GL_DEPTH_COMPONENT24_OES:
        type   = gcvSURF_DEPTH;
        format = gcvSURF_D24X8;
        break;

    case GL_DEPTH24_STENCIL8_OES:
    case GL_STENCIL_INDEX1_OES:
    case GL_STENCIL_INDEX4_OES:
    case GL_STENCIL_INDEX8:
        type   = gcvSURF_DEPTH;
        format = gcvSURF_D24S8;
        break;

    default:
        gl2mERROR(GL_INVALID_VALUE);
        goToEnd = GL_TRUE;
		break;
    }
	if (goToEnd)
		break;

    if (context->renderbuffer->surface != gcvNULL)
    {
        if (context->renderbuffer->combined == gcvNULL)
        {
            gcmVERIFY_OK(gcoSURF_Destroy(context->renderbuffer->surface));
        }
        else
        {
            context->renderbuffer->combined->combined = gcvNULL;
        }
    }

    status = gcoSURF_Construct(context->hal,
                               width, height, 1,
                               type, format,
                               gcvPOOL_DEFAULT,
                               &context->renderbuffer->surface);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        break;
    }

    status = gcoSURF_SetSamples(context->renderbuffer->surface,
                                context->drawSamples);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        break;
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glGetRenderbufferParameteriv(
    GLenum target,
    GLenum pname,
    GLint * params
    )
{
#if gcdNULL_DRIVER < 3
    gceSURF_FORMAT          format = gcvSURF_UNKNOWN;
    gcsSURF_FORMAT_INFO_PTR info[2];
    gceSTATUS               status;
	GLboolean               goToEnd = GL_FALSE;

	glmENTER2(glmARGENUM, target, glmARGENUM, pname)
	{

    glmPROFILE(context, GLES2_GETRENDERBUFFERPARAMETERIV, 0);

    if (target != GL_RENDERBUFFER)
    {
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    if (context->renderbuffer == gcvNULL)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if (context->renderbuffer->surface != gcvNULL)
    {
        gcmONERROR(gcoSURF_GetFormat(context->renderbuffer->surface,
                                       gcvNULL,
                                       &format));

        gcmONERROR(gcoSURF_QueryFormat(format, info));
    }
    else
    {
        format = gcvSURF_UNKNOWN;
        info[0] = info[1] = gcvNULL;
    }

    switch (pname)
    {
    case GL_RENDERBUFFER_WIDTH:
        *params = context->renderbuffer->width;
        break;

    case GL_RENDERBUFFER_HEIGHT:
        *params = context->renderbuffer->height;
        break;

    case GL_RENDERBUFFER_INTERNAL_FORMAT:
        *params = context->renderbuffer->format;
        break;

    case GL_RENDERBUFFER_RED_SIZE:
        *params = ( (info[0] != gcvNULL) &&
                    (info[0]->fmtClass == gcvFORMAT_CLASS_RGBA) )
                ? info[0]->u.rgba.red.width & gcvCOMPONENT_WIDTHMASK
                : 0;
        break;

    case GL_RENDERBUFFER_GREEN_SIZE:
        *params = ( (info[0] != gcvNULL) &&
                    (info[0]->fmtClass == gcvFORMAT_CLASS_RGBA) )
                ? info[0]->u.rgba.green.width & gcvCOMPONENT_WIDTHMASK
                : 0;
        break;

    case GL_RENDERBUFFER_BLUE_SIZE:
        *params = ( (info[0] != gcvNULL) &&
                    (info[0]->fmtClass == gcvFORMAT_CLASS_RGBA) )
                ? info[0]->u.rgba.blue.width & gcvCOMPONENT_WIDTHMASK
                : 0;
        break;

    case GL_RENDERBUFFER_ALPHA_SIZE:
        *params = ( (info[0] != gcvNULL) &&
                    (info[0]->fmtClass == gcvFORMAT_CLASS_RGBA) )
                ? info[0]->u.rgba.alpha.width & gcvCOMPONENT_WIDTHMASK
                : 0;
        break;

    case GL_RENDERBUFFER_DEPTH_SIZE:
        *params = ( (info[0] != gcvNULL) &&
                    (info[0]->fmtClass == gcvFORMAT_CLASS_DEPTH) )
                ? info[0]->u.depth.depth.width & gcvCOMPONENT_WIDTHMASK
                : 0;
        break;

    case GL_RENDERBUFFER_STENCIL_SIZE:
        *params = ( (info[0] != gcvNULL) &&
                    (info[0]->fmtClass == gcvFORMAT_CLASS_DEPTH) )
                ? info[0]->u.depth.stencil.width & gcvCOMPONENT_WIDTHMASK
                : 0;
        break;

    default:
OnError:
        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = GL_TRUE;
        break;
    }
	if (goToEnd)
		break;

    gcmDUMP_API("${ES20 glGetRenderbufferParameteriv 0x%08X 0x%08X (0x%08X)",
                target, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");

	}
	glmLEAVE1(glmARGINT, *params);
#else
    *params = 0;
#endif
}

#if gcdNULL_DRIVER < 3
static gceSTATUS
_GetRenderBufferAttribFromImage(
    khrEGL_IMAGE_PTR image,
    gctUINT* width,
    gctUINT* height,
    gceSURF_FORMAT* format,
    gcoSURF* surface,
    gctPOINTER* address
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    switch (image->type)
    {
    case KHR_IMAGE_TEXTURE_2D:
    case KHR_IMAGE_TEXTURE_CUBE:
    case KHR_IMAGE_RENDER_BUFFER:
        *surface = image->surface;

        gcmERR_BREAK(
            gcoSURF_GetSize(image->surface,
                            width,
                            height,
                            gcvNULL));

        gcmERR_BREAK(
            gcoSURF_GetFormat(image->surface,
                              gcvNULL,
                              format));
        break;

    case KHR_IMAGE_PIXMAP:
        *width   = image->u.pixmap.width;
        *height  = image->u.pixmap.height;
        *format  = image->u.pixmap.format;
        *surface = image->surface;
        *address = image->u.pixmap.address;
        break;

    default:
        status = gcvSTATUS_INVALID_ARGUMENT;
        break;
    }

    return status;
}
#endif

GL_APICALL void GL_APIENTRY
glEGLImageTargetRenderbufferStorageOES(
    GLenum target,
    GLeglImageOES image
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    gceSTATUS status;
    gceSURF_TYPE type;
    gctUINT width, height;
    gceSURF_FORMAT format;
    khrEGL_IMAGE_PTR eglImage = (khrEGL_IMAGE_PTR)(image);
    gcoSURF surface = gcvNULL;
    gctPOINTER address = gcvNULL;

    gcmHEADER_ARG("target=0x%04x image=0x%x", target, image);
    gcmDUMP_API("${ES20 glEGLImageTargetRenderbufferStorageOES 0x%08X 0x%08X}",
                target, image);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    if (target != GL_RENDERBUFFER)
    {
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if (context->renderbuffer == gcvNULL)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if ((eglImage == gcvNULL)
    ||  (eglImage->magic != KHR_EGL_IMAGE_MAGIC_NUM)
    )
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    status = _GetRenderBufferAttribFromImage(eglImage,
                                             &width,
                                             &height,
                                             &format,
                                             &surface,
                                             &address);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if ((width  > context->maxWidth)
    ||  (height > context->maxHeight)
    )
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    context->renderbuffer->width  = width;
    context->renderbuffer->height = height;

    switch (format)
    {
    case gcvSURF_R5G6B5:
    case gcvSURF_A4R4G4B4:
    case gcvSURF_A1R5G5B5:
    case gcvSURF_A8R8G8B8:
    case gcvSURF_X8R8G8B8:
        type = gcvSURF_RENDER_TARGET;
        break;

    case gcvSURF_D16:
    case gcvSURF_D24X8:
    case gcvSURF_D24S8:
        type = gcvSURF_DEPTH;
        break;

    default:
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (surface)
    {
        context->renderbuffer->surface = surface;
        gcoSURF_ReferenceSurface(surface);

        status = gcoSURF_SetSamples(context->renderbuffer->surface,
                                    context->drawSamples);

        if (gcmIS_ERROR(status))
        {
            gl2mERROR(GL_OUT_OF_MEMORY);
            gcmFOOTER_NO();
            return;
        }
    }
    else
    {
        status = gcoSURF_Construct(context->hal,
                                   width, height, 1,
                                   type, format,
                                   gcvPOOL_USER,
                                   &context->renderbuffer->surface);

        if (gcmIS_ERROR(status))
        {
            gl2mERROR(GL_OUT_OF_MEMORY);
            gcmFOOTER_NO();
            return;
        }

        status = gcoSURF_MapUserSurface(context->renderbuffer->surface,
                                        0,
                                        address,
                                        ~0U);

        if (gcmIS_ERROR(status))
        {
            gl2mERROR(GL_OUT_OF_MEMORY);
            gcmFOOTER_NO();
            return;
        }
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
#endif
}
