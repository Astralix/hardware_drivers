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




/*******************************************************************************
**  Framebuffer management for OpenGL ES 2.0 driver.
**
**  Each attachment of a framebuffer object consists of a main owner, either a
**  renderbuffer or a texture.  For both of these owners, we also set aside a
**  render target if the following conditions are met:
**
**  - The IP supports tile status.
**  - The render buffer or texture is nicely aligned to a resolvable size.
*
**  The render target is then used to render into, minimizing bandwidth usage
**  and even allowing for MSAA.  This render target needs to be resolved into
**  the storage for the renderbuffer or texture when te frame buffer changes
**  binding.
**
**  If a render target is attched - special care must be taken if a texture or
**  renderbuffer has been changed while not bound to the active framebuffer.
**  Because we have a special render target, the renderbuffer or texture needs
**  to be copied into the render target if it is dirty.  Luckely, this is easy
**  to manage by using dirty bits.
*/

#include "gc_glsh_precomp.h"

/*******************************************************************************
**  InitializeFramebuffer
**
**  Initialize a new context with frame buffer object management private
**  variables.
**
**  Arguments:
**
**      GLContext Context
**          Pointer to a new GLContext object.
*/
void
_glshInitializeFramebuffer(
    GLContext Context
    )
{
    /* No framebuffers bound yet. */
    Context->framebuffer        = gcvNULL;
    Context->framebufferChanged = GL_FALSE;

    /* No frame buffer objects. */
    gcmVERIFY_OK(
        gcoOS_ZeroMemory(&Context->framebufferObjects,
                         gcmSIZEOF(Context->framebufferObjects)));

    /* Check whether IP has tile status support. */
    Context->hasTileStatus =
        gcoHAL_IsFeatureAvailable(Context->hal,
                                  gcvFEATURE_FAST_CLEAR) == gcvSTATUS_TRUE;

    /* Check whether IP has correct stencil support in depth-only mode. */
    Context->hasCorrectStencil =
        gcoHAL_IsFeatureAvailable(Context->hal,
                                  gcvFEATURE_CORRECT_STENCIL) == gcvSTATUS_TRUE;
}

/*******************************************************************************
**  _NewFramebuffer
**
**  Create a new GLFramebuffer object.
**
**  Arguments:
**
**      GLContext Context
**          Pointer to a GLContext structure.
**
**      GLuint Name
**          Numeric name for GLFramebuffer object.
**
**  Returns:
**
**      GLFramebuffer
**          Pointer to the new GLFramebuffer object or gcvNULL if there is an error.
*/
#if gcdNULL_DRIVER < 3
static GLFramebuffer
_NewFramebuffer(
    GLContext Context,
    GLuint Name
    )
{
    GLFramebuffer framebuffer = gcvNULL;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    /* Allocate memory for the framebuffer object. */
    gcmONERROR(
        gcoOS_Allocate(gcvNULL,
                       gcmSIZEOF(struct _GLFramebuffer),
                       &pointer));

    framebuffer = pointer;

    gcmONERROR(gcoOS_ZeroMemory(framebuffer, gcmSIZEOF(struct _GLFramebuffer)));

    /* Create a new object. */
    if (!_glshInsertObject(&Context->framebufferObjects,
                           &framebuffer->object,
                           GLObject_Framebuffer,
                           Name))
    {
        goto OnError;
    }

    /* The framebuffer object is not yet bound. */
    framebuffer->dirty         = GL_TRUE;
    framebuffer->completeness  = GL_NONE;
    framebuffer->needResolve   = GL_FALSE;
    framebuffer->eglUsed       = GL_FALSE;

    /* No color attachment yet. */
    framebuffer->color.object  = gcvNULL;
    framebuffer->color.surface = gcvNULL;
    framebuffer->color.offset  = 0;
    framebuffer->color.target  = gcvNULL;

    /* No depth attachment yet. */
    framebuffer->depth.object  = gcvNULL;
    framebuffer->depth.surface = gcvNULL;
    framebuffer->depth.offset  = 0;
    framebuffer->depth.target  = gcvNULL;

    /* No stencil attachment yet. */
    framebuffer->stencil.object  = gcvNULL;
    framebuffer->stencil.surface = gcvNULL;
    framebuffer->stencil.offset  = 0;
    framebuffer->stencil.target  = gcvNULL;

    /* Return the framebuffer. */
    return framebuffer;

OnError:
    if (framebuffer != gcvNULL)
    {
        /* Roll back. */
        gcmVERIFY_OK(
            gcmOS_SAFE_FREE(gcvNULL, framebuffer));
    }

    /* Out of memory error. */
    gcmTRACE(gcvLEVEL_ERROR, "_NewFramebuffer: Out of memory.");
    gl2mERROR(GL_OUT_OF_MEMORY);
    return gcvNULL;
}
#endif

void _glshReferenceObject(GLContext Context, GLObject Object)
{
    if (Object != gcvNULL)
    {
        gcmASSERT((Object->type == GLObject_Texture)
                  || (Object->type == GLObject_Renderbuffer)
                  );
        Object->reference++;
    }
}

void _glshDereferenceObject(GLContext Context, GLObject Object)
{
    if (Object == gcvNULL) return;

    gcmASSERT((Object->type == GLObject_Texture) || (Object->type == GLObject_Renderbuffer));

    if (Object->type == GLObject_Texture)
    {
        _glshDereferenceTexture(Context, (GLTexture)Object);
    }

    else if (Object->type == GLObject_Renderbuffer)
    {
        _glshDereferenceRenderbuffer(Context, (GLRenderbuffer)Object);
    }
}

/*******************************************************************************
**  DeleteFramebuffer
**
**  Delete a GLFramebuffer object.
**
**  Arguments:
**
**      GLContext Context
**          Pointer to a GLContext structure.
**
**      GLFramebuffer Framebuffer
**          Pointer to a GLFramebuffer object.
*/
void
_glshDeleteFramebuffer(
    GLContext Context,
    GLFramebuffer Framebuffer
    )
{
    gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                  "Deleting framebuffer %d",
                  Framebuffer->object.name);

    /* Remove the framebuffer from the object list. */
    _glshRemoveObject(&Context->framebufferObjects,
                      &Framebuffer->object);

    /* Destroy any render target created for the color attachment. */
    if (Framebuffer->color.target != gcvNULL)
    {
        gcmVERIFY_OK(
            gcoSURF_Destroy(Framebuffer->color.target));
    }

    /* Destroy any render target created for the depth attachment. */
    if (Framebuffer->depth.target != gcvNULL)
    {
        if (Framebuffer->stencil.target == Framebuffer->depth.target)
        {
            Framebuffer->stencil.target = gcvNULL;
        }

        gcmVERIFY_OK(
            gcoSURF_Destroy(Framebuffer->depth.target));
    }

    /* Stencil cannot have any attachmanent. */
    gcmASSERT(Framebuffer->stencil.target == gcvNULL);

    _glshDereferenceObject(Context, Framebuffer->color.object);
#if gldFBO_DATABASE
    if ((Framebuffer->color.object != gcvNULL)
        &&
        (Framebuffer->color.object->type == GLObject_Texture)
        )
    {
        ((GLTexture) Framebuffer->color.object)->owner = gcvNULL;
    }
#endif

    _glshDereferenceObject(Context, Framebuffer->depth.object);
#if gldFBO_DATABASE
    if ((Framebuffer->depth.object != gcvNULL)
        &&
        (Framebuffer->depth.object->type == GLObject_Texture)
        )
    {
        ((GLTexture) Framebuffer->depth.object)->owner = gcvNULL;
    }
#endif

    _glshDereferenceObject(Context, Framebuffer->stencil.object);
#if gldFBO_DATABASE
    if ((Framebuffer->stencil.object != gcvNULL)
        &&
        (Framebuffer->stencil.object->type == GLObject_Texture)
        )
    {
        ((GLTexture) Framebuffer->stencil.object)->owner = gcvNULL;
    }
#endif

#if gldFBO_DATABASE
    /* Remove all databases. */
    glshRemoveDatabase(Framebuffer);
#endif

    /* Free the memory. */
    gcmVERIFY_OK(
        gcmOS_SAFE_FREE(gcvNULL, Framebuffer));
}

/*******************************************************************************
**  glGenFramebuffers
**
**  Generate a number of framebuffer objects.
**
**  Arguments:
**
**      GLsizei Count
**          Number of framebuffers to generate.
**
**      GLuint * Framebuffers
**          Pointer to an array of GLuint values that will receive the names of
**          the generated buffers.
**
*/
GL_APICALL void GL_APIENTRY
glGenFramebuffers(
    GLsizei Count,
    GLuint * Framebuffers
    )
{
#if gcdNULL_DRIVER < 3
    GLFramebuffer framebuffer;
    GLsizei i;
	GLboolean goToEnd = gcvFALSE;

	glmENTER1(glmARGINT, Count)
	{
    /* Update the profiler. */
    glmPROFILE(context, GLES2_GENFRAMEBUFFERS, 0);

    /* Make sure count is not less than zero. */
    if (Count < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Loop while there are framebuffers to generate. */
    for (i = 0; i < Count; ++i)
    {
        /* Create a new framebuffer. */
        framebuffer = _NewFramebuffer(context, 0);
        if (framebuffer == gcvNULL)
        {
            goToEnd = gcvTRUE;
			break;
        }

        /* Return framebuffer name. */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_DRIVER,
                      "%s(%d): framebuffer object name is %u",
                      __FUNCTION__, __LINE__, framebuffer->object.name);
        Framebuffers[i] = framebuffer->object.name;
    }

	if (goToEnd)
		break;

	gcmDUMP_API("${ES20 glGenFramebuffers 0x%08X (0x%08X)",
                Count, Framebuffers);
    gcmDUMP_API_ARRAY(Framebuffers, Count);
    gcmDUMP_API("$}");

	}
	glmLEAVE();
#else
    while (Count < 0)
    {
        *Framebuffers++ = 2;
    }
#endif
}

/*******************************************************************************
**  glBindFramebuffer
**
**  Bind a framebuffer to the specified target or unbind the current framebuffer
**  from the specified target.
**
**  Arguments:
**
**      GLenum Target
**          Target to bind the buffer to.  This must be GL_FRAMEBUFFER.
**
**      GLuint Framebuffer
**          Framebuffer to bind or 0 to unbind the current framebuffer.
**
*/
GL_APICALL void GL_APIENTRY
glBindFramebuffer(
    GLenum Target,
    GLuint Framebuffer
    )
{
#if gcdNULL_DRIVER < 3
    GLFramebuffer object;
	gceSTATUS status;

	glmENTER2(glmARGENUM, Target, glmARGUINT, Framebuffer)
	{
    gcmDUMP_API("${ES20 glBindFramebuffer 0x%08X 0x%08X}", Target, Framebuffer);

    /* Update the profiler. */
    glmPROFILE(context, GLES2_BINDFRAMEBUFFER, 0);

    /* Make sure target has the right value. */
    if (Target != GL_FRAMEBUFFER)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid target 0x%04X",
                      __FUNCTION__, __LINE__, Target);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    if (Framebuffer == 0)
    {
        /* Remove current binding. */
        object = gcvNULL;
    }
    else
    {
        /* Find the object. */
        object = (GLFramebuffer) _glshFindObject(&context->framebufferObjects,
                                                 Framebuffer);

        if (object == gcvNULL)
        {
            /* Create a new framebuffer. */
            object = _NewFramebuffer(context, Framebuffer);
            if (object == gcvNULL)
            {
                break;
            }
        }

        /* Mark attached color textures as flushable. */
        if ((object->color.object != gcvNULL)
        &&  (object->color.object->type == GLObject_Texture)
        )
        {
            /* Texture object needs to be flushed next time it is used. */
            ((GLTexture) object->color.object)->needFlush = GL_TRUE;

            if (((GLTexture) object->color.object)->fromImage)
            {
                /* Flush texture to update any other textures that may have
                   aliased the same eglImage. */
                context->textureFlushNeeded = gcvTRUE;
            }
        }

        /* Mark attached depth textures as flushable. */
        if ((object->depth.object != gcvNULL)
        &&  (object->depth.object->type == GLObject_Texture)
        )
        {
            /* Texture object needs to be flushed next time it is used. */
            ((GLTexture) object->depth.object)->needFlush = GL_TRUE;

            if (((GLTexture) object->depth.object)->fromImage)
            {
                /* Flush texture to update any other textures that may have
                   aliased the same eglImage. */
                context->textureFlushNeeded = gcvTRUE;
            }
        }
    }

    /* Only proceed if the framebuffer is different than the current binding. */
    if (context->framebuffer != object)
    {
        /* Unbind any current framebuffer. */
        if (context->framebuffer != gcvNULL)
        {
            /* If it is used outside gl, sync the pixels of the surface. */
            if (context->framebuffer->eglUsed)
            {
                if (context->framebuffer->color.surface != gcvNULL)
                {
                    gcmONERROR(gcoSURF_Resolve(context->framebuffer->color.surface,
                                               context->framebuffer->color.surface));
                }
                if (context->framebuffer->depth.surface != gcvNULL)
                {
                    gcmONERROR(gcoSURF_Resolve(context->framebuffer->depth.surface,
                                               context->framebuffer->depth.surface));
                }
            }

            if ((context->framebuffer->color.target != gcvNULL)
            &&  context->framebuffer->needResolve
            )
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                              "Resolving color target 0x%x into texture %d",
                              context->framebuffer->color.target,
                              context->framebuffer->color.object->name);

                /* Quick check for texture surface changed case. */
                if(context->framebuffer->color.object->type == GLObject_Texture)
                {
                    GLTexture texture = (GLTexture)context->framebuffer->color.object;

                    if (texture->target == GL_TEXTURE_2D)
                    {
                        gcoSURF surface = gcvNULL;
                        gcmONERROR(gcoTEXTURE_GetMipMap(texture->texture,
                                                        context->framebuffer->color.level,
                                                        &surface));

                        if (surface != gcvNULL)
                        {
                            context->framebuffer->color.surface = surface;
                        }
                    }
                }

                /* Set orientation. */
                gcmONERROR(
                    gcoSURF_SetOrientation(context->framebuffer->color.surface,
                                           gcvORIENTATION_BOTTOM_TOP));

                /* Resolve color render target into texture. */
                gcmONERROR(
                    gcoSURF_Resolve(context->framebuffer->color.target,
                                    context->framebuffer->color.surface));
            }

            if ((context->framebuffer->depth.target != gcvNULL)
            &&  context->framebuffer->needResolve
            )
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                              "Resolving depth target 0x%x into texture %d",
                              context->framebuffer->depth.target,
                              context->framebuffer->depth.object->name);

                /* Set orientation. */
                gcmONERROR(
                    gcoSURF_SetOrientation(context->framebuffer->depth.surface,
                                           gcvORIENTATION_BOTTOM_TOP));

                /* Resolve depth render target into texture. */
                gcmONERROR(
                    gcoSURF_Resolve(context->framebuffer->depth.target,
                                    context->framebuffer->depth.surface));
            }

            context->framebuffer->needResolve = gcvFALSE;
        }

        /* Set new render targets. */
        context->framebuffer        = object;
        context->framebufferChanged = GL_TRUE;

        /* Check for uploading into the render target. */
        if ((object != gcvNULL)
        &&  (object->color.object != gcvNULL)
        &&  (object->color.object->type == GLObject_Texture)
        &&  (object->color.target != gcvNULL)
        )
        {
            if ((((GLTexture) object->color.object)->dirty)
            ||  (  (((GLTexture) object->color.object)->renderingFb != 0)
                && (((GLTexture) object->color.object)->renderingFb != Framebuffer)
                )
            )
            {
                gcmONERROR(
                    gcoSURF_DisableTileStatus(object->color.target, gcvTRUE));

                gcmONERROR(
                    gcoSURF_Resolve(object->color.surface,
                                    object->color.target));

                ((GLTexture) object->color.object)->dirty = GL_FALSE;
            }
        }

        /* Check for uploading into the depth buffer. */
        if ((object != gcvNULL)
        &&  (object->depth.object != gcvNULL)
        &&  (object->depth.object->type == GLObject_Texture)
        &&  (object->depth.target != gcvNULL)
        )
        {
            if (((GLTexture) object->depth.object)->dirty)
            {
                gcmONERROR(
                    gcoSURF_DisableTileStatus(object->depth.target, gcvTRUE));

                gcmONERROR(
                    gcoSURF_Resolve(object->depth.surface,
                                    object->depth.target));

                ((GLTexture) object->depth.object)->dirty = GL_FALSE;
            }
        }
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    }
    glmLEAVE();
    return;

OnError:
    gl2mERROR(GL_INVALID_OPERATION);
    gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

/*******************************************************************************
**  glDeleteFramebuffers
**
**  Delete a specified number of framebuffers.
**
**  Arguments:
**
**      GLsizei Count
**          Number of framebuffers to delete.
**
**      const GLUint * Framebuffers
**          Pointer to the list of framebuffers to delete.
**
*/
GL_APICALL void GL_APIENTRY
glDeleteFramebuffers(
    GLsizei Count,
    const GLuint * Framebuffers
    )
{
#if gcdNULL_DRIVER < 3
    GLsizei i;
    GLFramebuffer framebuffer;

	glmENTER2(glmARGINT, Count, glmARGPTR, Framebuffers)
	{
    gcmDUMP_API("${ES20 glDeleteFramebuffers 0x%08X (0x%08X)",
                Count, Framebuffers);
    gcmDUMP_API_ARRAY(Framebuffers, Count);
    gcmDUMP_API("$}");

    /* Update the profiler. */
    glmPROFILE(context, GLES2_DELETEFRAMEBUFFERS, 0);

    /* Make sure count is not less than 0. */
    if (Count < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    for (i = 0; i < Count; ++i)
    {
        /* Find the specified framebufer. */
        framebuffer = (GLFramebuffer) _glshFindObject(&context->framebufferObjects,
                                                      Framebuffers[i]);

        /* Make sure this is a valid framebuffer. */
        if ((framebuffer == gcvNULL)
        ||  (framebuffer->object.type != GLObject_Framebuffer)
        )
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "%s(%d): Object %d is not a framebuffer",
                          __FUNCTION__, __LINE__, Framebuffers[i]);

            /*gl2mERROR(GL_INVALID_VALUE);*/
            break;
        }

        /* If this is the current framebuffer, unbind it. */
        if (context->framebuffer == framebuffer)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            /* Disable batch optmization. */
            context->batchDirty = GL_TRUE;
        }

        /* Delete the framebuffer. */
        _glshDeleteFramebuffer(context, framebuffer);
    }

	}
	glmLEAVE();
#endif
}

/*******************************************************************************
**  glIsFramebuffer
**
**  Check whether the specified framebuffer is a framebuffer object or not.
**
**  Arguments:
**
**      GLuint Framebuffer
**          Framebuffer name to test.
**
**  Returns:
**
**      GLboolean
**          GL_TRUE if the specified framebuffer is indeed a framebuffer object.
**          GL_FALSE otherwise.
**
*/
GL_APICALL GLboolean GL_APIENTRY
glIsFramebuffer(
    GLuint Framebuffer
    )
{
#if gcdNULL_DRIVER < 3
    GLObject object;
    GLboolean isFramebuffer = GL_FALSE;

	glmENTER1(glmARGUINT, Framebuffer)
	{
    /* Update the profiler. */
    glmPROFILE(context, GLES2_ISFRAMEBUFFER, 0);

    /* Find the object. */
    object = _glshFindObject(&context->framebufferObjects, Framebuffer);

    /* Test if the object is indeed a framebuffer object. */
    isFramebuffer = (object != gcvNULL) && (object->type == GLObject_Framebuffer);

    gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                  "%s(%d): %d => %s",
                  __FUNCTION__, __LINE__, Framebuffer,
                  isFramebuffer ? "GL_TRUE" : "GL_FALSE");

    gcmDUMP_API("${ES20 glIsFramebuffer 0x%08X := 0x%08X}",
                Framebuffer, isFramebuffer);
	}
	glmLEAVE1(glmARGINT, isFramebuffer);

	/* Return the framebuffer status. */
	return isFramebuffer;
#else
    return (Framebuffer == 2) ? GL_TRUE : GL_FALSE;
#endif
}

void
_MergeDepthAndStencil(
    GLContext Context
    )
{
    GLRenderbuffer depth, stencil;

    depth   = (GLRenderbuffer) Context->framebuffer->depth.object;
    stencil = (GLRenderbuffer) Context->framebuffer->stencil.object;

    /* Need both depth and stencil to combine. */
    if ((depth   == gcvNULL)
    ||  (stencil == gcvNULL)
    )
    {
        return;
    }

    /* GL_OES_packed_depth_stencil. */
    if (depth == stencil)
    {
        return;
    }

    /* Both need to be of renderbuffer type. */
    if ((depth->object.type   != GLObject_Renderbuffer)
    ||  (stencil->object.type != GLObject_Renderbuffer)
    )
    {
        return;
    }

    /* Already bound together? */
    if (depth->bound   && (depth->combined   == stencil)
    &&  stencil->bound && (stencil->combined == depth)
    )
    {
        return;
    }

    if ((depth->width  != stencil->width)
    ||  (depth->height != stencil->height)
    )
    {
        Context->framebuffer->dirty        = GL_FALSE;
        Context->framebuffer->completeness =
            GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;

        return;
    }

    if (depth->bound
    &&  (depth->combined != gcvNULL)
    &&  (depth->combined != stencil)
    )
    {
        Context->framebuffer->dirty        = GL_FALSE;
        Context->framebuffer->completeness = GL_FRAMEBUFFER_UNSUPPORTED;

        return;
    }

    if (stencil->bound
    &&  (stencil->combined != gcvNULL)
    &&  (stencil->combined != depth)
    )
    {
        Context->framebuffer->dirty        = GL_FALSE;
        Context->framebuffer->completeness = GL_FRAMEBUFFER_UNSUPPORTED;

        return;
    }

    if ((depth->surface != gcvNULL) && (stencil->surface != depth->surface))
    {
        /* Delete depth surface. */
        gcmVERIFY_OK(gcoSURF_Destroy(depth->surface));
    }

    /* Combine depth and stencil renderbuffers. */
    depth->surface                      = stencil->surface;
    Context->framebuffer->depth.surface = stencil->surface;
    depth->combined                     = stencil;
    stencil->combined                   = depth;
}

/*******************************************************************************
**  glFramebufferTexture2D
**
**  Attach a texture to the currently bound framebuffer.
**
**  Arguments:
**
**      GLenum Target
**          This must be GL_FRAMEBUFFER.
**
**      GLenum Attachment
**          Attachment to attach.  This can be one of the following:
**
**              GL_COLOR_ATTACHMENT0
**                  Attach a color attachment.
**              GL_DEPTH_ATTACHMENT
**                  Attach a depth attachment.
**              GL_STENCIL_ATTACHMENT
**                  Attach a stencil attachment.
**
**      GLenum Textarget
**          This must be GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP.
**
**      GLuint Texture
**          Name of the texture to attach.
**
**      GLint Level
**          This must be 0.
*/
GL_APICALL void GL_APIENTRY
glFramebufferTexture2D(
    GLenum Target,
    GLenum Attachment,
    GLenum Textarget,
    GLuint Texture,
    GLint Level
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture object;
    gcoSURF surface = gcvNULL, target = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
    gceTEXTURE_FACE face = gcvFACE_NONE;
    gctUINT32 offset = 0;
    gceSURF_TYPE type;
    gceSURF_FORMAT format = gcvSURF_UNKNOWN;
    gctUINT width, height, depth;
    gctBOOL redo = gcvFALSE;
    gctBOOL goToEnd = gcvFALSE;

    glmENTER5(glmARGENUM, Target, glmARGENUM, Attachment, glmARGENUM, Textarget,
        glmARGUINT, Texture, glmARGINT, Level)
    {
    gcmDUMP_API("${ES20 glFramebufferTexture2D 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X}",
                Target, Attachment, Textarget, Texture, Level);

    /* Update the profiler. */
    glmPROFILE(context, GLES2_FRAMEBUFFERTEXTURE2D, 0);

    /* Make sure target has the right value. */
    if (Target != GL_FRAMEBUFFER)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid target %s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Target));

        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Make sure there is a framebuffer object bound. */
    if (context->framebuffer == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): No framebuffer bound",
                      __FUNCTION__, __LINE__);

        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Find texture object. */
    if (Texture == 0)
    {
        /* Remove current attachment. */
        object  = gcvNULL;
        surface = gcvNULL;
    }
    else
    {
        /* Find the texture object. */
        object = (GLTexture) _glshFindObject(&context->textureObjects, Texture);

        if (object == gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "%s(%d): Texture %u not found",
                          __FUNCTION__, __LINE__, Texture);

            gl2mERROR(GL_INVALID_OPERATION);
            break;
        }

        if (Level != 0)
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "%s(%d): Invalid level=%d",
                          __FUNCTION__, __LINE__, Level);

            gl2mERROR(GL_INVALID_VALUE);
            break;
        }

        do
        {
#if gldFBO_DATABASE
            /* Save object owner. */
            object->owner = context->framebuffer;
#endif

            switch (object->target)
            {
            case GL_TEXTURE_2D:
                if (Textarget != GL_TEXTURE_2D)
                {
                    gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_FRAMEBUFFER,
                                  "%s(%d): Invalid textarget=%s for TEXTURE_2D object",
                                  __FUNCTION__, __LINE__, _glshGetEnum(Textarget));

                    gl2mERROR(GL_INVALID_OPERATION);
                    goToEnd = gcvTRUE;
                    break;
                }

                status = gcoTEXTURE_GetMipMap(object->texture,
                                              Level,
                                              &surface);
                offset = 0;
                break;

            case GL_TEXTURE_CUBE_MAP:
                switch (Textarget)
                {
                case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                    face = gcvFACE_POSITIVE_X;
                    break;

                case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                    face = gcvFACE_NEGATIVE_X;
                    break;

                case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                    face = gcvFACE_POSITIVE_Y;
                    break;

                case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                    face = gcvFACE_NEGATIVE_Y;
                    break;

                case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                    face = gcvFACE_POSITIVE_Z;
                    break;

                case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                    face = gcvFACE_NEGATIVE_Z;
                    break;

                default:
                    gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                                  "%s(%d): Invalid textarget=%s for TEXTURE_CUBE_MAP object",
                                  __FUNCTION__, __LINE__, _glshGetEnum(Textarget));

                    gl2mERROR(GL_INVALID_OPERATION);
                    goToEnd = gcvTRUE;
                    break;
                }
                if (goToEnd)
                    break;

                status = gcoTEXTURE_GetMipMapFace(object->texture,
                                                  Level,
                                                  face,
                                                  &surface,
                                                  &offset);
                break;

            default:
                gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_FRAMEBUFFER,
                              "%s(%d): Invalid textarget=%s for TEXTURE object",
                              __FUNCTION__, __LINE__, _glshGetEnum(Textarget));

                gl2mERROR(GL_INVALID_OPERATION);
                goToEnd = gcvTRUE;
                break;
            }

            if (goToEnd)
                break;

            if (gcmIS_ERROR(status))
            {
                surface = gcvNULL;
            }

            else if (redo)
            {
                redo = gcvFALSE;
            }

            else
            {
                gctBOOL unaligned = gcoTEXTURE_IsRenderable(object->texture, Level) == gcvSTATUS_NOT_ALIGNED;

                gcmONERROR(
                    gcoSURF_GetSize(surface, &width, &height, &depth));

                /* We create a new render target for IPs that support tile status,
                ** when the textures are not properly aligned, or when multi-sampling
                ** is turned on. */
                if ((  (context->hasTileStatus || (context->drawSamples > 1))
                    && (width > 128)
                    && (height > 128)
                    && (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_TILED_READ) != gcvSTATUS_TRUE)
                    )
                ||  unaligned
                ||  object->fromImage
                )
                {
                    gcmVERIFY_OK(
                        gcoSURF_GetFormat(surface, &type, &format));

                    if ((Attachment == GL_STENCIL_ATTACHMENT)
                    &&  (format == gcvSURF_D24S8)
                    &&  (context->framebuffer->depth.surface == surface)
                    )
                    {
                        target  = context->framebuffer->depth.target;
                        surface = context->framebuffer->depth.surface;
                        break;
                    }

                    else
                    if ((Attachment == GL_DEPTH_ATTACHMENT)
                    &&  (format == gcvSURF_D24S8)
                    &&  (context->framebuffer->stencil.surface == surface)
                    )
                    {
                        target  = context->framebuffer->stencil.target;
                        surface = context->framebuffer->stencil.surface;
                        break;
                    }

                    switch (format)
                    {
                    case gcvSURF_D16:
                    case gcvSURF_D24X8:
                    case gcvSURF_D24S8:
                    case gcvSURF_D32:
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
                        if (Attachment == GL_COLOR_ATTACHMENT0)
                        {
                            gcmTRACE_ZONE(gcvLEVEL_WARNING,
                                          gldZONE_FRAMEBUFFER,
                                          "%s(%d): Depth texture attached to color",
                                          __FUNCTION__, __LINE__, _glshGetEnum(Attachment));
                        }
#endif

                        type = unaligned ? gcvSURF_DEPTH_NO_TILE_STATUS
                                         : gcvSURF_DEPTH;
                        break;

                    default:
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
                        if (Attachment != GL_COLOR_ATTACHMENT0)
                        {
                            gcmTRACE_ZONE(gcvLEVEL_WARNING,
                                          gldZONE_FRAMEBUFFER,
                                          "%s(%d): Color texture attached to depth",
                                          __FUNCTION__, __LINE__, _glshGetEnum(Attachment));
                        }
#endif

                        type = unaligned ? gcvSURF_RENDER_TARGET_NO_TILE_STATUS
                                         : gcvSURF_RENDER_TARGET;
                        break;
                    }

                    if (gcmIS_SUCCESS(
                            gcoSURF_Construct(context->hal,
                                              width, height, depth,
                                              type,
                                              format,
                                              gcvPOOL_DEFAULT,
                                              &target)))
                    {
                    }
                }

                if (target == gcvNULL)
                {
                    /* Render directly into the mipmap. */
                    gcmVERIFY_OK(
                        gcoTEXTURE_RenderIntoMipMap(object->texture, Level));

                    /* Re-get the surface - it might have been changed. */
                    redo = gcvTRUE;
                }
            }
        }
        while (redo);

        if (goToEnd)
            break;
    }

    /* Dispatch on attachment. */
    switch (Attachment)
    {
    case GL_COLOR_ATTACHMENT0:
#if gldFBO_DATABASE
        if ((object == gcvNULL)
            &&
            (context->framebuffer->color.object != gcvNULL)
            )
        {
            ((GLTexture) context->framebuffer->color.object)->owner = gcvNULL;
        }
#endif

        if ((context->framebuffer->color.target != gcvNULL)
        &&  (context->framebuffer->color.target != target)
        )
        {
            /* Resolve the surface before destroying, if it needs to. */
            if ( context->framebuffer->needResolve )
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                              "Resolving color target 0x%x into texture %d",
                              context->framebuffer->color.target,
                              context->framebuffer->color.object->name);

                /* Set orientation. */
                gcmONERROR(
                    gcoSURF_SetOrientation(context->framebuffer->color.surface,
                                           gcvORIENTATION_BOTTOM_TOP));

                /* Resolve color render target into texture. */
                gcmONERROR(
                    gcoSURF_Resolve(context->framebuffer->color.target,
                                    context->framebuffer->color.surface));

                context->framebuffer->needResolve = 0;
            }

            gcmVERIFY_OK(gcoSURF_Destroy(context->framebuffer->color.target));
        }

        _glshDereferenceObject(context, context->framebuffer->color.object);
        /* Set color attachment. */
        context->framebuffer->color.object  = (Texture == 0) ? gcvNULL : &object->object;
        _glshReferenceObject(context, context->framebuffer->color.object);
        context->framebuffer->color.surface = (Texture == 0) ? gcvNULL : surface;

        /*
         * If surface offset is changed, we should update render target address
         * through gcoHARDWARE_FlushTarget.
         */
        if (context->framebuffer->color.offset != offset && surface != gcvNULL)
        {
            /*
             * Set offset to surface object, which will be used by gcoHARDWARE_FlushTarget
             * to send correct address(Phy_Addr+Offset) to HARDWARE.
             */
            gcoSURF_SetOffset(surface, offset);

            /*
             * If surface offset is changed, We have to unset target to force
             * the HARDWARE update render target address.
             */
            gco3D_UnsetTarget(context->engine, surface);
        }
        context->framebuffer->color.offset  = (Texture == 0) ? 0 : offset;
        context->framebuffer->color.target  = target;

        context->framebuffer->color.level   = Level;

        /* We should resove surface to target, I will optimize this to only resolve if we is must
           Currently we make the ICS gallary save pic function works.
        */
        if(context->framebuffer->color.surface != gcvNULL &&
           context->framebuffer->color.target != gcvNULL)
        {
            /* Set orientation. */
            gcmVERIFY_OK(
                gcoSURF_SetOrientation(context->framebuffer->color.surface,
                                       gcvORIENTATION_BOTTOM_TOP));

            gcmVERIFY_OK(
                gcoSURF_SetOrientation(context->framebuffer->color.target,
                                       gcvORIENTATION_BOTTOM_TOP));

            /* Resolve color render target into texture. */
            gcmVERIFY_OK(
                gcoSURF_Resolve(context->framebuffer->color.surface,
                                context->framebuffer->color.target));
        }

        context->framebuffer->dirty         = GL_TRUE;
        break;

    case GL_DEPTH_ATTACHMENT:
#if gldFBO_DATABASE
        if ((object == gcvNULL)
            &&
            (context->framebuffer->depth.object != gcvNULL)
            )
        {
            ((GLTexture) context->framebuffer->depth.object)->owner = gcvNULL;
        }
#endif

        if ((context->framebuffer->depth.target != gcvNULL)
        &&  (context->framebuffer->depth.target != target)
        &&  (context->framebuffer->depth.target !=
             context->framebuffer->stencil.target)
        )
        {
            /* Resolve the surface before destroying, if it needs to. */
            if ( context->framebuffer->needResolve )
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                              "Resolving depth target 0x%x into texture %d",
                              context->framebuffer->depth.target,
                              context->framebuffer->depth.object->name);

                /* Set orientation. */
                gcmONERROR(
                    gcoSURF_SetOrientation(context->framebuffer->depth.surface,
                                           gcvORIENTATION_BOTTOM_TOP));

                /* Resolve depth render target into texture. */
                gcmONERROR(
                    gcoSURF_Resolve(context->framebuffer->depth.target,
                                    context->framebuffer->depth.surface));

                context->framebuffer->needResolve = 0;
            }

            gcmVERIFY_OK(gcoSURF_Destroy(context->framebuffer->depth.target));
        }

        _glshDereferenceObject(context, context->framebuffer->depth.object);
        /* Set depth attachment. */
        context->framebuffer->depth.object  = (Texture == 0) ? gcvNULL
                                                             : &object->object;

        _glshReferenceObject(context, context->framebuffer->depth.object);

        context->framebuffer->depth.surface = (Texture == 0) ? gcvNULL
                                                             : surface;
        context->framebuffer->depth.offset  = (Texture == 0) ? 0
                                                             : offset;
        context->framebuffer->depth.target  = target;
        context->framebuffer->dirty         = GL_TRUE;
        break;

    case GL_STENCIL_ATTACHMENT:
#if gldFBO_DATABASE
        if ((object == gcvNULL)
            &&
            (context->framebuffer->stencil.object != gcvNULL)
            )
        {
            ((GLTexture) context->framebuffer->stencil.object)->owner = gcvNULL;
        }
#endif

        if ((context->framebuffer->stencil.target != gcvNULL)
        &&  (context->framebuffer->stencil.target != target)
        &&  (context->framebuffer->stencil.target !=
             context->framebuffer->depth.target)
        )
        {
            gcmVERIFY_OK(gcoSURF_Destroy(context->framebuffer->stencil.target));
        }

        _glshDereferenceObject(context, context->framebuffer->stencil.object);
        /* Set stencil attachment. */
        context->framebuffer->stencil.object  = (Texture == 0) ? gcvNULL
                                                               : &object->object;

        _glshReferenceObject(context, context->framebuffer->stencil.object);

        context->framebuffer->stencil.surface = (Texture == 0) ? gcvNULL
                                                               : surface;
        context->framebuffer->stencil.offset  = (Texture == 0) ? 0
                                                               : offset;
        context->framebuffer->stencil.target  = target;
        context->framebuffer->dirty           = GL_TRUE;
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid attachment=%s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Attachment));

        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = gcvTRUE;
    }

	if (goToEnd)
		break;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    }
	glmLEAVE();
	return;

OnError:
    gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
	glmGetApiEndTime(context);
#endif
}

GL_APICALL void GL_APIENTRY
glFramebufferTexture3DOES(
    GLenum Target,
    GLenum Attachment,
    GLenum Textarget,
    GLuint Texture,
    GLint Level,
    GLint Zoffset
    )
{
#if gcdNULL_DRIVER < 3
    GLTexture object;
    gcoSURF surface = gcvNULL, target = gcvNULL;
    gceSTATUS status = gcvSTATUS_OK;
    gctUINT32 offset = 0;
    gceSURF_TYPE type;
    gceSURF_FORMAT format;
    gctUINT width, height, depth;
    gctBOOL redo = gcvFALSE;
	gctBOOL goToEnd = gcvFALSE;

	glmENTER6(glmARGENUM, Target, glmARGENUM, Attachment, glmARGENUM, Textarget,
		      glmARGUINT, Texture, glmARGINT, Level, glmARGINT, Zoffset)
	{

    gcmDUMP_API("${ES20 glFramebufferTexture3DOES 0x%08X 0x%08X 0x%08X 0x%08X "
                "0x%08X 0x%08X}",
                Target, Attachment, Textarget, Texture, Level,Zoffset);

    /* Update the profiler. */
    glmPROFILE(context, GLES2_FRAMEBUFFERTEXTURE3DOES, 0);

    /* Make sure target has the right value. */
    if (Target != GL_FRAMEBUFFER)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid target %s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Target));

        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Make sure there is a framebuffer object bound. */
    if (context->framebuffer == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): No framebuffer bound",
                      __FUNCTION__, __LINE__);

        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Find texture object. */
    if (Texture == 0)
    {
        /* Remove current attachment. */
        object  = gcvNULL;
        surface = gcvNULL;
    }
    else
    {
        /* Find the texture object. */
        object = (GLTexture) _glshFindObject(&context->textureObjects, Texture);

        if (object == gcvNULL)
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "%s(%d): Texture %u not found",
                          __FUNCTION__, __LINE__, Texture);

            gl2mERROR(GL_INVALID_VALUE);
            break;
        }

        if (Level < 0)
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "%s(%d): Invalid level=%d",
                          __FUNCTION__, __LINE__, Level);

            gl2mERROR(GL_INVALID_VALUE);
            break;
		}

        do
        {
            switch (object->target)
            {
            case GL_TEXTURE_3D_OES:
                if (Textarget != GL_TEXTURE_3D_OES)
                {
                    gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_FRAMEBUFFER,
                                  "%s(%d): Invalid textarget=%s for TEXTURE_3D object",
                                  __FUNCTION__, __LINE__, _glshGetEnum(Textarget));

                    gl2mERROR(GL_INVALID_OPERATION);
                    goToEnd = gcvTRUE;
					break;
                }

                /* We need a function to give the slice offset in 3D texture */
                gcmASSERT(0);
                status = gcvSTATUS_INVALID_ARGUMENT;
                /* This offset should be slice offset in 3D texture */
                offset = 0;
                break;

            default:
                gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_FRAMEBUFFER,
                              "%s(%d): Invalid textarget=%s for TEXTURE_3D object",
                              __FUNCTION__, __LINE__, Textarget);

                gl2mERROR(GL_INVALID_OPERATION);
                goToEnd = gcvTRUE;
				break;
            }

			if (goToEnd)
				break;

            if (gcmIS_ERROR(status))
            {
                surface = gcvNULL;
            }

            else if (redo)
            {
                redo = gcvFALSE;
            }

            else
            {
                gctBOOL unaligned = gcoTEXTURE_IsRenderable(object->texture, Level) == gcvSTATUS_NOT_ALIGNED;

                gcmONERROR(
                    gcoSURF_GetSize(surface, &width, &height, &depth));

                /* We create a new render target for IPs that support tile status,
                ** when the textures are not properly aligned, or when multi-sampling
                ** is turned on. */
                if ((  (context->hasTileStatus || (context->drawSamples > 1))
                    && (width > 128) && (height > 128)
                    )
                ||  unaligned
                ||  object->fromImage
                )
                {
                    gcmVERIFY_OK(
                        gcoSURF_GetFormat(surface, &type, &format));

                    if ((Attachment == GL_STENCIL_ATTACHMENT)
                    &&  (format == gcvSURF_D24S8)
                    &&  (context->framebuffer->depth.surface == surface)
                    )
                    {
                        target  = context->framebuffer->depth.target;
                        surface = context->framebuffer->depth.surface;
                        break;
                    }

                    else
                    if ((Attachment == GL_DEPTH_ATTACHMENT)
                    &&  (format == gcvSURF_D24S8)
                    &&  (context->framebuffer->stencil.surface == surface)
                    )
                    {
                        target  = context->framebuffer->stencil.target;
                        surface = context->framebuffer->stencil.surface;
                        break;
                    }

                    switch (format)
                    {
                    case gcvSURF_D16:
                    case gcvSURF_D24X8:
                    case gcvSURF_D24S8:
                    case gcvSURF_D32:
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
                        if (Attachment == GL_COLOR_ATTACHMENT0)
                        {
                            gcmTRACE_ZONE(gcvLEVEL_WARNING,
                                          gldZONE_FRAMEBUFFER,
                                          "%s(%d): Depth texture attached to color",
                                          __FUNCTION__, __LINE__, _glshGetEnum(Attachment));
                        }
#endif

                        type = unaligned ? gcvSURF_DEPTH_NO_TILE_STATUS
                                         : gcvSURF_DEPTH;
                        break;

                    default:
#if gcmIS_DEBUG(gcdDEBUG_TRACE)
                        if (Attachment != GL_COLOR_ATTACHMENT0)
                        {
                            gcmTRACE_ZONE(gcvLEVEL_WARNING,
                                          gldZONE_FRAMEBUFFER,
                                          "%s(%d): Color texture attached to depth",
                                          __FUNCTION__, __LINE__, _glshGetEnum(Attachment));
                        }
#endif

                        type = unaligned ? gcvSURF_RENDER_TARGET_NO_TILE_STATUS
                                         : gcvSURF_RENDER_TARGET;
                        break;
                    }

                    if (gcmIS_SUCCESS(
                            gcoSURF_Construct(context->hal,
                                              width, height, 1,
                                              type,
                                              format,
                                              gcvPOOL_DEFAULT,
                                              &target)))
                    {
                        /* Type is not depth. */
                        if ((type & ~gcvSURF_NO_TILE_STATUS) != gcvSURF_DEPTH)
                        {
                            gcmONERROR(
                                gcoSURF_SetSamples(target,
                                                   context->drawSamples));
                        }
                    }
                }

                if (target == gcvNULL)
                {
                    /* Render directly into the mipmap. */
                    gcmVERIFY_OK(
                        gcoTEXTURE_RenderIntoMipMap(object->texture, Level));

                    /* Re-get the surface - it might have been changed. */
                    redo = gcvTRUE;
                }
            }
        }
        while (redo);

		if (goToEnd)
			break;
    }

    /* Dispatch on attachment. */
    switch (Attachment)
    {
    case GL_COLOR_ATTACHMENT0:
        if ((context->framebuffer->color.target != gcvNULL)
        &&  (context->framebuffer->color.target != target)
        )
        {
            /* Resolve the surface before destroying, if it needs to. */
            if ( context->framebuffer->needResolve )
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                              "Resolving color target 0x%x into texture %d",
                              context->framebuffer->color.target,
                              context->framebuffer->color.object->name);

                /* Set orientation. */
                gcmVERIFY_OK(
                    gcoSURF_SetOrientation(context->framebuffer->color.surface,
                                           gcvORIENTATION_BOTTOM_TOP));

                /* Resolve color render target into texture. */
                gcmONERROR(
                    gcoSURF_Resolve(context->framebuffer->color.target,
                                    context->framebuffer->color.surface));

                context->framebuffer->needResolve = 0;
            }

            gcmVERIFY_OK(gcoSURF_Destroy(context->framebuffer->color.target));
        }

        /* Set color attachment. */
        context->framebuffer->color.object  = (Texture == 0) ? gcvNULL
                                                             : &object->object;
        context->framebuffer->color.surface = (Texture == 0) ? gcvNULL
                                                             : surface;
        context->framebuffer->color.offset  = (Texture == 0) ? 0
                                                             : offset;
        context->framebuffer->color.target  = target;

        context->framebuffer->dirty         = GL_TRUE;
        break;

    case GL_DEPTH_ATTACHMENT:
        if ((context->framebuffer->depth.target != gcvNULL)
        &&  (context->framebuffer->depth.target != target)
        &&  (context->framebuffer->depth.target !=
             context->framebuffer->stencil.target)
        )
        {
            /* Resolve the surface before destroying, if it needs to. */
            if ( context->framebuffer->needResolve )
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                              "Resolving depth target 0x%x into texture %d",
                              context->framebuffer->depth.target,
                              context->framebuffer->depth.object->name);

                /* Set orientation. */
                gcmVERIFY_OK(
                    gcoSURF_SetOrientation(context->framebuffer->depth.surface,
                                           gcvORIENTATION_BOTTOM_TOP));

                /* Resolve depth render target into texture. */
                gcmONERROR(
                    gcoSURF_Resolve(context->framebuffer->depth.target,
                                    context->framebuffer->depth.surface));

                context->framebuffer->needResolve = 0;
            }

            gcmVERIFY_OK(gcoSURF_Destroy(context->framebuffer->depth.target));
        }

        /* Set depth attachment. */
        context->framebuffer->depth.object  = (Texture == 0) ? gcvNULL
                                                             : &object->object;
        context->framebuffer->depth.surface = (Texture == 0) ? gcvNULL
                                                             : surface;
        context->framebuffer->depth.offset  = (Texture == 0) ? 0
                                                             : offset;
        context->framebuffer->depth.target  = target;
        context->framebuffer->dirty         = GL_TRUE;
        break;

    case GL_STENCIL_ATTACHMENT:
        if ((context->framebuffer->stencil.target != gcvNULL)
        &&  (context->framebuffer->stencil.target != target)
        &&  (context->framebuffer->stencil.target !=
             context->framebuffer->depth.target)
        )
        {
            gcmVERIFY_OK(gcoSURF_Destroy(context->framebuffer->stencil.target));
        }

        /* Set stencil attachment. */
        context->framebuffer->stencil.object  = (Texture == 0) ? gcvNULL
                                                               : &object->object;
        context->framebuffer->stencil.surface = (Texture == 0) ? gcvNULL
                                                               : surface;
        context->framebuffer->stencil.offset  = (Texture == 0) ? 0
                                                               : offset;
        context->framebuffer->stencil.target  = target;
        context->framebuffer->dirty           = GL_TRUE;
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_ERROR, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid attachment=%s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Attachment));

        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = gcvTRUE;
		break;
    }
	if (goToEnd)
		break;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;
    }
	glmLEAVE();
	return;

OnError:
    gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
	glmGetApiEndTime(context);
#endif
}

/*******************************************************************************
**  glFramebufferRenderbuffer
**
**  Attach a renderbuffer to the currently bound framebuffer.
**
**  Arguments:
**
**      GLenum Target
**          This must be GL_FRAMEBUFFER.
**
**      GLenum Attachment
**          Attachment to attach.  This can be one of the following:
**
**              GL_COLOR_ATTACHMENT0
**                  Attach a color attachment.
**              GL_DEPTH_ATTACHMENT
**                  Attach a depth attachment.
**              GL_STENCIL_ATTACHMENT
**                  Attach a stencil attachment.
**
**      GLenum Renderbuffertarget
**          This must be GL_RENDERBUFFER.
**
**      GLuint Renderbuffer
**          Name of the renderbuffer to attach.
*/
GL_APICALL void GL_APIENTRY
glFramebufferRenderbuffer(
    GLenum Target,
    GLenum Attachment,
    GLenum Renderbuffertarget,
    GLuint Renderbuffer
    )
{
#if gcdNULL_DRIVER < 3
    GLRenderbuffer render;
	gceSTATUS status;
	GLboolean goToEnd = gcvFALSE;

	glmENTER4(glmARGENUM, Target, glmARGENUM, Attachment,
		      glmARGENUM, Renderbuffertarget, glmARGUINT, Renderbuffer)
	{
    gcmDUMP_API("${ES20 glFramebufferRenderbuffer 0x%08X 0x%08X 0x%08X 0x%08X}",
                Target, Attachment, Renderbuffertarget, Renderbuffer);

    /* Update the profiler. */
    glmPROFILE(context, GLES2_FRAMEBUFFERRENDERBUFFER, 0);

    /* Make sure target has the right value. */
    if (Target != GL_FRAMEBUFFER)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid target %s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Target));

        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Make sure there is a framebuffer object bound. */
    if (context->framebuffer == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): No framebuffer bound",
                      __FUNCTION__, __LINE__);

        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Make sure renderbuffertarget has the right value. */
    if (Renderbuffertarget != GL_RENDERBUFFER)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid renderbuffertarget %s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Renderbuffertarget));

        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Find the renderbuffer. */
    render = (GLRenderbuffer) _glshFindObject(&context->renderbufferObjects,
                                              Renderbuffer);

    /* Test if the render buffer was found. */
    if ((render == gcvNULL) && (Renderbuffer != 0))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Renderbuffer %u not found",
                      __FUNCTION__, __LINE__, Renderbuffer);

        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }


	if((Renderbuffer != 0) && (render != context->renderbuffer) && (render->format == GL_NONE))
	{
		gl2mERROR(GL_INVALID_OPERATION);
        break;
	}
    /* Dispatch on attachment. */
    switch (Attachment)
    {
    case GL_COLOR_ATTACHMENT0:
        /* Delete any render target created for the color attachment. */
        if (context->framebuffer->color.target != gcvNULL)
        {
            /* Resolve the surface before destroying, if it needs to. */
            if ( context->framebuffer->needResolve)
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                              "Resolving depth target 0x%x into texture %d",
                              context->framebuffer->color.target,
                              context->framebuffer->color.object->name);

                /* Set orientation. */
                gcmONERROR(
                    gcoSURF_SetOrientation(context->framebuffer->color.surface,
                                           gcvORIENTATION_BOTTOM_TOP));

                /* Resolve depth render target into texture. */
                gcmONERROR(
                    gcoSURF_Resolve(context->framebuffer->color.target,
                                    context->framebuffer->color.surface));

                context->framebuffer->needResolve = 0;
            }

            gcmVERIFY_OK(
                gcoSURF_Destroy(context->framebuffer->color.target));
        }

        _glshDereferenceObject(context, context->framebuffer->color.object);
        /* Copy renderbuffer to color attachment. */
        context->framebuffer->color.object  = (render == gcvNULL)
                                            ? gcvNULL
                                            : &render->object;

        _glshReferenceObject(context, context->framebuffer->color.object);

        context->framebuffer->color.surface = (render == gcvNULL)
                                            ? gcvNULL
                                            : render->surface;
        context->framebuffer->color.offset  = 0;
        context->framebuffer->color.target  = gcvNULL;
        context->framebuffer->dirty         = GL_TRUE;
        context->framebuffer->eglUsed = (render == gcvNULL)
                                      ? GL_FALSE
                                      : render->eglUsed;
        break;

    case GL_DEPTH_ATTACHMENT:
        /* Delete any render target created for the depth attachment. */
        if (context->framebuffer->depth.target != gcvNULL)
        {
            /* Resolve the surface before destroying, if it needs to. */
            if ( context->framebuffer->needResolve )
            {
                gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                              "Resolving depth target 0x%x into texture %d",
                              context->framebuffer->depth.target,
                              context->framebuffer->depth.object->name);

                /* Set orientation. */
                gcmONERROR(
                    gcoSURF_SetOrientation(context->framebuffer->depth.surface,
                                           gcvORIENTATION_BOTTOM_TOP));

                /* Resolve depth render target into texture. */
                gcmONERROR(
                    gcoSURF_Resolve(context->framebuffer->depth.target,
                                    context->framebuffer->depth.surface));

                context->framebuffer->needResolve = 0;
            }

            gcmVERIFY_OK(
                gcoSURF_Destroy(context->framebuffer->depth.target));
        }

        _glshDereferenceObject(context, context->framebuffer->depth.object);

        /* Copy renderbuffer to depth attachment. */
        context->framebuffer->depth.object  = (render == gcvNULL)
                                            ? gcvNULL
                                            : &render->object;

        _glshReferenceObject(context, context->framebuffer->depth.object);

        context->framebuffer->depth.surface = (render == gcvNULL)
                                            ? gcvNULL
                                            : render->surface;
        context->framebuffer->depth.offset  = 0;
        context->framebuffer->depth.target  = gcvNULL;
        context->framebuffer->dirty         = GL_TRUE;
        context->framebuffer->eglUsed = (render == gcvNULL)
                                      ? GL_FALSE
                                      : render->eglUsed;

        /* Merge the depth and stencil buffers. */
        _MergeDepthAndStencil(context);

        if (render != gcvNULL)
        {
            /* Mark the renderbuffer as bound. */
            render->bound = gcvTRUE;
        }
        break;

    case GL_STENCIL_ATTACHMENT:
        gcmASSERT(context->framebuffer->stencil.target == gcvNULL);

        _glshDereferenceObject(context, context->framebuffer->stencil.object);
        /* Copy renderbuffer to stencil attachment. */
        context->framebuffer->stencil.object  = (render == gcvNULL)
                                              ? gcvNULL
                                              : &render->object;

        _glshReferenceObject(context, context->framebuffer->stencil.object);

        context->framebuffer->stencil.surface = (render == gcvNULL)
                                              ? gcvNULL
                                              : render->surface;
        context->framebuffer->stencil.offset  = 0;
        context->framebuffer->stencil.target  = gcvNULL;
        context->framebuffer->dirty           = GL_TRUE;
        context->framebuffer->eglUsed = (render == gcvNULL)
                                      ? GL_FALSE
                                      : render->eglUsed;

        /* Merge the depth and stencil buffers. */
        _MergeDepthAndStencil(context);

        if (render != gcvNULL)
        {
            /* Mark the renderbuffer as bound. */
            render->bound = gcvTRUE;
        }
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid attachment %s",
                      __FUNCTION__, __LINE__, Attachment);

        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = gcvTRUE;
		break;
    }

	if (goToEnd)
		break;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
    return;

OnError:
    gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
	glmGetApiEndTime(context);
#endif
}

/*******************************************************************************
**  IsFramebufferComplete
**
**  Check whether the currently bound framebuffer is complete or not.
**
**  Arguments:
**
**      GLContext Context
**          Pointer to the GLContext structure.
**
**  Returns:
**
**      GLenum
**          The completess of the frame buffer.  Can be one of the following:
**
**              GL_FRAMEBUFFER_COMPLETE
**                  The framebuffer is complete.
**              GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
**                  One of the attachments to the framebuffer is incomplete.
**              GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
**                  There are no attachments to the framebuffer.
**              GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
**                  The attachments have different dimensions.
**              GL_FRAMEBUFFER_UNSUPPORTED
**                  The attachments cannot be used together.
*/
GLenum
_glshIsFramebufferComplete(
    GLContext Context
    )
{
    gceSURF_FORMAT format[3];
    gcsSURF_FORMAT_INFO_PTR info[2];
    gceSTATUS status;
    GLint count = 0, i;
    gctUINT width[3], height[3];

    /* We are complete if there is no bound framebuffer. */
    if (Context->framebuffer == gcvNULL)
    {
        return GL_FRAMEBUFFER_COMPLETE;
    }

    do
    {
        if (!Context->framebuffer->dirty)
        {
            /* The framebuffer is not dirty, so we already have a valid
            ** completeness. */
            break;
        }

        /* Test for incomplete attachment. */
        Context->framebuffer->completeness =
            GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;

        /* Test color attachment for completeness. */
        if (Context->framebuffer->color.object != gcvNULL)
        {
            /* Test for storage. */
            if (Context->framebuffer->color.surface == gcvNULL)
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "Framebuffer %d: Color attachment has no storage",
                              Context->framebuffer->object.name);
                Context->framebuffer->completeness =
                    GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
                break;
            }

            /* Get the format of color attachment. */
            gcmERR_BREAK(
                gcoSURF_GetFormat(Context->framebuffer->color.surface,
                                  gcvNULL,
                                  &format[count]));

            gcmERR_BREAK(
                gcoSURF_QueryFormat(format[count], info));

            /* Only (A)RGB surfaces are supported for color attachments. */
            if (info[0]->fmtClass != gcvFORMAT_CLASS_RGBA)
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "Framebuffer %d: Color attachment has invalid "
                              "format class",
                              Context->framebuffer->object.name);
                break;
            }

            /* Get the dimension of the color attachment. */
            gcmERR_BREAK(
                gcoSURF_GetSize(Context->framebuffer->color.surface,
                                &width[count], &height[count],
                                gcvNULL));

            ++count;
        }

        /* Test depth attachment for completeness. */
        if (Context->framebuffer->depth.object != gcvNULL)
        {
            /* Test for storage. */
            if (Context->framebuffer->depth.surface == gcvNULL)
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "Framebuffer %d: Depth attachment has no storage",
                              Context->framebuffer->object.name);
                Context->framebuffer->completeness =
                    GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
                break;
            }

            /* Get the format of depth attachment. */
            gcmERR_BREAK(
                gcoSURF_GetFormat(Context->framebuffer->depth.surface,
                                  gcvNULL,
                                  &format[count]));

            gcmERR_BREAK(
                gcoSURF_QueryFormat(format[count], info));

            /* Only (S)D surfaces are supported for depth attachments. */
            if ((info[0]->fmtClass != gcvFORMAT_CLASS_DEPTH)
            ||  (info[0]->u.depth.depth.width == 0)
            )
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "Framebuffer %d: Depth attachment has invalid "
                              "format class",
                              Context->framebuffer->object.name);
                break;
            }

            /* Get the dimension of the depth attachment. */
            gcmERR_BREAK(
                gcoSURF_GetSize(Context->framebuffer->depth.surface,
                                &width[count], &height[count],
                                gcvNULL));

            ++count;
        }

        /* Test stencil attachment for completeness. */
        if (Context->framebuffer->stencil.object != gcvNULL)
        {
            /* Test for storage. */
            if (Context->framebuffer->stencil.surface == gcvNULL)
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "Framebuffer %d: Stencil attachment has no "
                              "storage",
                              Context->framebuffer->object.name);
                break;
            }

            /* Get the format of stencil attachment. */
            gcmERR_BREAK(
                gcoSURF_GetFormat(Context->framebuffer->stencil.surface,
                                  gcvNULL,
                                  &format[count]));

            gcmERR_BREAK(
                gcoSURF_QueryFormat(format[count], info));

            /* Only (S)D surfaces are supported for depth attachments. */
            if ((info[0]->fmtClass != gcvFORMAT_CLASS_DEPTH)
            ||  (info[0]->u.depth.stencil.width == 0)
            )
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "Framebuffer %d: Stencil attachment has invalid "
                              "format class",
                              Context->framebuffer->object.name);
                break;
            }

            /* Get the dimension of the stencil attachment. */
            gcmERR_BREAK(
                gcoSURF_GetSize(Context->framebuffer->stencil.surface,
                                &width[count], &height[count],
                                gcvNULL));

            ++count;
        }

        /* Test for missing attachment. */
        if (count == 0)
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "Framebuffer %d: Missing attachments",
                          Context->framebuffer->object.name);

            Context->framebuffer->completeness =
                GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
            break;
        }

        /* Test for incorrect dimensions between attachments. */
        for (i = count - 1; i > 0; --i)
        {
            if ((width[0] != width[i]) || (height[0] != height[i]))
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "Framebuffer %d: Dimensions of attachments are "
                              "different (%dx%d vs %dx%d)",
                              Context->framebuffer->object.name,
                              width[0], height[0],
                              width[i], height[i]);

                Context->framebuffer->completeness =
                    GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
                break;
            }
        }

        if (i > 0)
        {
            break;
        }

        /* Test for unsupported depth & stencil combination. */
        if ((Context->framebuffer->depth.object   != gcvNULL)
        &&  (Context->framebuffer->stencil.object != gcvNULL)
        )
        {
            GLint depth = (count == 2) ? 0 : 1;
            gcmASSERT(depth + 1 < count);

            if (format[depth] != format[depth + 1])
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "Framebuffer %d: Depth and stencil attachments "
                              "have different formats (%d vs %d)",
                              Context->framebuffer->object.name,
                              format[depth],
                              format[depth + 1]);

                Context->framebuffer->completeness =
                    GL_FRAMEBUFFER_UNSUPPORTED;
                break;
            }
        }

        /* Framebuffer is complete. */
        Context->framebuffer->completeness = GL_FRAMEBUFFER_COMPLETE;
    }
    while (gcvFALSE);

    /* Return the completeness of the framebuffer. */
    return Context->framebuffer->completeness;
}

/*******************************************************************************
**  glCheckFramebufferStatus
**
**  Check whether the currently bound framebuffer is complete or not.  A
**  complete framebuffer can be used to render into.
**
**  Arguments:
**
**      GLenum Target
**          This must be GL_FRAMEBUFFER.
**
**  Returns:
**
**      GLenum
**          The completess of the frame buffer or 0 if there is an error.  Can
**          be one of the following:
**
**              GL_FRAMEBUFFER_COMPLETE
**                  The framebuffer is complete.
**              GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
**                  One of the attachments to the framebuffer is incomplete.
**              GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
**                  There are no attachments to the framebuffer.
**              GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
**                  The attachments have different dimensions.
**              GL_FRAMEBUFFER_UNSUPPORTED
**                  The attachments cannot be used together.
*/
GL_APICALL GLenum GL_APIENTRY
glCheckFramebufferStatus(
    GLenum Target
    )
{
#if gcdNULL_DRIVER < 3
    GLenum status = 0;

   	glmENTER1(glmARGENUM, Target)
	{

    /* Update the profiler. */
    glmPROFILE(context, GLES2_CHECKFRAMEBUFFERSTATUS, 0);

    /* Make sure target has the right value. */
    if (Target != GL_FRAMEBUFFER)
    {
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Check if the current framebuffer is complete or not. */
    status = _glshIsFramebufferComplete(context);

    gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_FRAMEBUFFER,
                  "%s(%d) status is %s",
                  __FUNCTION__, __LINE__, _glshGetEnum(status));

    gcmDUMP_API("${ES20 glCheckFramebufferStatus 0x%08X := 0x%08X}",
                Target, status);

	}
	glmLEAVE1(glmARGINT, status);

	/* Return the completeness. */
	return status;

#else
    return GL_FRAMEBUFFER_COMPLETE;
#endif
}

/*******************************************************************************
**  glGetFramebufferAttachmentParameteriv
**
**  Query information about an attachment of a framebuffer.
**
**  Arguments:
**
**      GLenum Target
**          This must be GL_FRAMEBUFFER.
**
**      GLenum Attachment
**          Attachment to query.  This can be one of the following:
**
**              GL_COLOR_ATTACHMENT0
**                  Query information about the color attachment.
**              GL_DEPTH_ATTACHMENT
**                  Query information about the depth attachment.
**              GL_STENCIL_ATTACHMENT
**                  Query information about the stencil attachment.
**
**      GLenum Name
**          Parameter name to query.  This can be one of the following:
**
**              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE
**                  Query the object type of the attachment.
**              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME
**                  Query the object name of the attachment.
**              GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL
**                  Query the texture level of the attachment.  The attachment
**                  has to be a texture object and the returned level is always
**                  0.
**              GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE
**                  Query the cube map face of the attachment.  The attachment
**                  has to be a cube map texture object.
**
**      GLint * Params
**          Pointer to a variable that will receive the information.
*/
GL_APICALL void GL_APIENTRY
glGetFramebufferAttachmentParameteriv(
    GLenum Target,
    GLenum Attachment,
    GLenum Name,
    GLint * Params
    )
{
#if gcdNULL_DRIVER < 3
    GLAttachment * attach = gcvNULL;
    GLTexture texture;
	GLboolean goToEnd = gcvFALSE;

	glmENTER3(glmARGENUM, Target, glmARGENUM, Attachment, glmARGENUM, Name)
	{

    /* Update the profiler. */
    glmPROFILE(context, GLES2_GETFRAMEBUFFERATTACHMENTPARAMETERIV, 0);

    /* Make sure target has the right value. */
    if (Target != GL_FRAMEBUFFER)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid target %s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Target));

        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    /* Make sure we have a bound framebuffer. */
    if (context->framebuffer == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): No current framebuffer attached",
                      __FUNCTION__, __LINE__);

        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Get requested attachment. */
    switch (Attachment)
    {
    case GL_COLOR_ATTACHMENT0:
        /* Get color attachment. */
        attach = &context->framebuffer->color;
        break;

    case GL_DEPTH_ATTACHMENT:
        /* Get depth attachment. */
        attach = &context->framebuffer->depth;
        break;

    case GL_STENCIL_ATTACHMENT:
        /* Get stencil attachment. */
        attach = &context->framebuffer->stencil;
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid attachment %s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Attachment));

        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = gcvTRUE;
		break;
	}

	if (goToEnd)
		break;

    /* Parse name. */
    switch (Name)
    {
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
        /* Test for no attachment. */
        if (attach->object == gcvNULL)
        {
            *Params = GL_NONE;
        }

        /* Test for renderbufefr attachment. */
        else if (attach->object->type == GLObject_Renderbuffer)
        {
            *Params = GL_RENDERBUFFER;
        }

        /* Test for texture attachment. */
        else if (attach->object->type == GLObject_Texture)
        {
            *Params = GL_TEXTURE;
        }

        else
        {
            gcmFATAL("Invalid object type: %d", attach->object->type);
        }

        break;

    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
        /* Return attachment name. */
		if(attach->object == gcvNULL)
		{
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME: "
                          "No attached");

            context->error = GL_INVALID_ENUM;
            goToEnd = gcvTRUE;
			break;
		}
		else
		{
			*Params = attach->object->name;
		}
        break;

    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
        /* Return attached texture level. */
        if ((attach->object == gcvNULL)
        ||  (attach->object->type != GLObject_Texture)
        )
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL: "
                          "No texture attached");

            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = gcvTRUE;
			break;
        }

        /* This has to be zero. */
        *Params = 0;
        break;

    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
        /* Return attached texture cube map face. */
        if ((attach->object == gcvNULL)
        ||   (attach->object->type != GLObject_Texture)
        )
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE: "
                          "No texture attached");

            gl2mERROR(GL_INVALID_ENUM);
            goToEnd = gcvTRUE;
			break;
        }
        /* Get pointer to GLTexture object. */
        texture = (GLTexture) attach->object;

        /* Test if texture is a cube map. */
        if (texture->target == GL_TEXTURE_CUBE_MAP)
        {
            gcoSURF surface;
            gctUINT32 slice;

            /* Get the slice size for the cube map. */
            if (gcmIS_SUCCESS(
                    gcoTEXTURE_GetMipMapFace(texture->texture,
                                             0,
                                             gcvFACE_NEGATIVE_X,
                                             &surface,
                                             &slice)))
            {
                /* Compute the face by dividing the offset by the slice size. */
                *Params = GL_TEXTURE_CUBE_MAP_POSITIVE_X
                        + attach->offset / slice;
            }
            else
            {
                /* Not a complete cube map. */
                *Params = 0;
            }
        }
        else
        {
            /* Not a cube map. */
            *Params = 0;
        }
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                      "%s(%d): Invalid name %s",
                      __FUNCTION__, __LINE__, _glshGetEnum(Name));

        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = gcvTRUE;
		break;
    }
	if (goToEnd)
		break;

    gcmDUMP_API("${ES20 glGetFramebufferAttachmentParameteriv 0x%08X 0x%08X "
                "0x%08X (0x%08X)",
                Target, Attachment, Name, Params);
    gcmDUMP_API_ARRAY(Params, 1);
    gcmDUMP_API("$}");

	}
	glmLEAVE();
#else
    *Params = 0;
#endif
}

gcoSURF
_glshGetFramebufferSurface(
    GLAttachment * Attachment
    )
{
    if (Attachment == gcvNULL)
    {
        return gcvNULL;
    }

    if (Attachment->target != gcvNULL)
    {
        return Attachment->target;
    }

    return Attachment->surface;
}

/*******************************************************************************
**  glDiscardFramebufferEXT
**
**  Provide a means for discarding portions of every pixels in particular
**  buffer, effectively leaving its contents undefined. It's a hint.
**
**  Arguments:
**
**      GLenum target
**          This must be GL_FRAMEBUFFER.
**
**      GLsizei numAttachments
**          Indicate how many attachments are supplied in the
**          attachments list.
**
**      const GLenum * attachments
**          Must be GL_COLOR_EXT, GL_DEPTH_EXT or GL_STENCIL_EXT.
**
*/
GL_APICALL void GL_APIENTRY
glDiscardFramebufferEXT(
    GLenum Target,
    GLsizei NumAttachments,
    const GLenum *Attachments
    )
{
#if gcdNULL_DRIVER < 3
    GLsizei i;

	glmENTER3(glmARGENUM, Target, glmARGINT, NumAttachments, glmARGPTR, Attachments)
	{
        gcmDUMP_API("${ES20 glDiscardFramebufferEXT 0x%08X 0x%08X (0x%08X)",
                    Target, NumAttachments, Attachments);
        gcmDUMP_API_ARRAY(Attachments, NumAttachments);
        gcmDUMP_API("$}");

        /* Update the profiler. */
        glmPROFILE(context, GLES2_DISCARDFRAMEBUFFEREXT, 0);

        /* Make sure target has the right value. */
        if (Target != GL_FRAMEBUFFER)
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "%s(%d): Invalid target %s",
                          __FUNCTION__, __LINE__, _glshGetEnum(Target));

            gl2mERROR(GL_INVALID_ENUM);
            break;
        }

        /* Make sure number of attachments not less than zero. */
        if (NumAttachments < 0)
        {
            gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                          "%s(%d): Invalid numAttachment %d",
                          __FUNCTION__, __LINE__, NumAttachments);

            gl2mERROR(GL_INVALID_VALUE);
            break;
        }

        /* Make sure attachments list are valid. */
        for (i = 0; i < NumAttachments; ++i)
        {
            if ((Attachments[i] != GL_COLOR_EXT) ||
                (Attachments[i] != GL_DEPTH_EXT) ||
                (Attachments[i] != GL_STENCIL_EXT)
               )
            {
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_FRAMEBUFFER,
                              "%s(%d): Invalid numAttachment 0x%x",
                              __FUNCTION__, __LINE__, Attachments[i]);

                gl2mERROR(GL_INVALID_ENUM);
                break;
            }
        }

        /* We have nothing to discard so far, ignore silently. */
    }
    glmLEAVE();
	return;
#endif
}
