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




#include "gc_glff_precomp.h"

#define _GC_OBJ_ZONE glvZONE_TRACE

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  _DeleteRenderBuffer
**
**  Buffer destructor.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Object
**          Pointer to the object to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/

static gceSTATUS _DeleteRenderBuffer(
    IN glsCONTEXT_PTR Context,
    IN gctPOINTER Object
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x Object=0x%x", Context, Object);

    do
    {
        /* Cast the object. */
        glsRENDER_BUFFER_PTR object = (glsRENDER_BUFFER_PTR) Object;

        /* Is the render buffer surface allocated? */
        if (object->surface != gcvNULL)
        {
            /* Is this a combined render buffer? */
            if (object->combined == gcvNULL)
            {
                /* No, delete the surface. */
                gcmERR_BREAK(gcoSURF_Destroy(object->surface));
                object->surface = gcvNULL;
            }
            else
            {
                /* Yes, 'decombine'. */
                object->combined->combined = gcvNULL;
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER();

    return status;
}


/*******************************************************************************
**
**  _CreateRenderBuffer
**
**  Render buffer constructor.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      RenderBuffer
**          Name of the render buffer to create.
**
**  OUTPUT:
**
**      Wrapper
**          Points to the created named object.
*/

static gceSTATUS _CreateRenderBuffer(
    IN glsCONTEXT_PTR Context,
    IN gctUINT32 RenderBuffer,
    OUT glsNAMEDOBJECT_PTR * Wrapper
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x RenderBuffer=%u Wrapper=0x%x", Context, RenderBuffer, Wrapper);

    do
    {
        glsRENDER_BUFFER_PTR object;
        glsCONTEXT_PTR shared;

        /* Map shared context. */
#if gcdRENDER_THREADS
        shared = (Context->shared != gcvNULL) ? Context->shared : Context;
#else
        shared = Context;
#endif

        /* Attempt to allocate a new buffer. */
        gcmERR_BREAK(glfCreateNamedObject(
            Context,
            &shared->renderBufferList,
            RenderBuffer,
            _DeleteRenderBuffer,
            Wrapper
            ));

        /* Get a pointer to the new buffer. */
        object = (glsRENDER_BUFFER_PTR) (*Wrapper)->object;

        /* Init the buffer to the defaults. */
        gcoOS_ZeroMemory(object, sizeof(glsRENDER_BUFFER));
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return result. */
    return status;
}


/*******************************************************************************
**
**  _ConvertBufferFormat
**
**  Convert OpenGL render buffer format into HAL format.
**
**  INPUT:
**
**      Format
**          Specifies the format of the pixel data.
**
**      HalType
**          HAL type.
**
**      HalFormat
**          HAL format.
**
**  OUTPUT:
**
**      Returns GL_FALSE for unsupported formats.
*/

static gctBOOL _ConvertBufferFormat(
    GLenum Format,
    gceSURF_TYPE * HalType,
    gceSURF_FORMAT * HalFormat
    )
{
    /* Assume success. */
    gctBOOL result = gcvTRUE;

    gcmHEADER_ARG("Format=0x%04x HalType=0x%x HalFormat=0x%x", Format, HalType, HalFormat);

    /* Dispatch on format. */
    switch (Format)
    {
    case GL_RGB8_OES:
        * HalType   = gcvSURF_RENDER_TARGET;
        * HalFormat = gcvSURF_X8R8G8B8;
        break;

    case GL_RGBA8_OES:
        * HalType   = gcvSURF_RENDER_TARGET;
        * HalFormat = gcvSURF_A8R8G8B8;
        break;

    case GL_RGB565_OES:
        * HalType   = gcvSURF_RENDER_TARGET;
        * HalFormat = gcvSURF_R5G6B5;
        break;

    case GL_RGBA4_OES:
        * HalType   = gcvSURF_RENDER_TARGET;
        * HalFormat = gcvSURF_A4R4G4B4;
        break;

    case GL_RGB5_A1_OES:
        * HalType   = gcvSURF_RENDER_TARGET;
        * HalFormat = gcvSURF_A1R5G5B5;
        break;

    case GL_DEPTH_COMPONENT16_OES:
        * HalType   = gcvSURF_DEPTH;
        * HalFormat = gcvSURF_D16;
        break;

	case GL_DEPTH_COMPONENT24_OES:
        * HalType   = gcvSURF_DEPTH;
        * HalFormat = gcvSURF_D24X8;
        break;

	case GL_DEPTH24_STENCIL8_OES:
	case GL_STENCIL_INDEX1_OES:
    case GL_STENCIL_INDEX4_OES:
    case GL_STENCIL_INDEX8_OES:
        * HalType   = gcvSURF_DEPTH;
        * HalFormat = gcvSURF_D24S8;
        break;

    default:
        /* Not supported format. */;
        result = gcvFALSE;
    }

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  _ConvertImageType
**
**  Convert EGL image type.
**
**  INPUT:
**
**      Format
**          Specifies the format of the pixel data.
**
**      HalType
**          HAL type.
**
**  OUTPUT:
**
**      Returns GL_FALSE for unsupported formats.
*/

static gctBOOL _ConvertImageType(
    GLenum Format,
    gceSURF_TYPE * HalType
    )
{
    /* Assume success. */
    gctBOOL result = gcvTRUE;

    gcmHEADER_ARG("Format=0x%04x HalType=0x%x", Format, HalType);

    /* Dispatch on format. */
    switch (Format)
    {
    case gcvSURF_R5G6B5:
    case gcvSURF_A4R4G4B4:
    case gcvSURF_A1R5G5B5:
        * HalType = gcvSURF_RENDER_TARGET;
        break;

    case gcvSURF_D16:
    case gcvSURF_D24X8:
    case gcvSURF_D24S8:
        * HalType   = gcvSURF_DEPTH;
        break;

    default:
        /* Not supported format. */;
        result = gcvFALSE;
    }

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}


/******************************************************************************\
********************************* Internal API *********************************
\******************************************************************************/

/*******************************************************************************
**
**  glfGetEGLImageAttributes
**
**  Get attributes of the specified EGL image.
**
**  INPUT:
**
**      Image
**          Specifies an eglImage.
**
**  OUTPUT
**
**      Surface
**          Return gcoSURF object of the texture.
**
**      Format
**          Return format of the texture.
**
**      Width
**          Return width of the texture.
**
**      Height
**          Return height of the texture.
**
**      Stride
**          Return stride of the texture.
**
**      Level
**          Return level of a mipmap.
**
**      Pixel
**          Return pointer of texture pixel.
**
*/

gceSTATUS glfGetEGLImageAttributes(
    khrEGL_IMAGE_PTR Image,
    glsEGL_IMAGE_ATTRIBUTES_PTR Attributes
    )
{
    gceSTATUS status;
    gcmHEADER_ARG("Image=0x%x Attributes=0x%x", Image, Attributes);

    do
    {
        /* Validate the image. */
        if ((Image == gcvNULL) || (Image->magic != KHR_EGL_IMAGE_MAGIC_NUM))
        {
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }

        Attributes->privHandle = gcvNULL;

        /* Get texture attributes. */
        switch (Image->type)
        {
        case KHR_IMAGE_TEXTURE_2D:
        case KHR_IMAGE_TEXTURE_CUBE:
        case KHR_IMAGE_RENDER_BUFFER:
#if defined(ANDROID)
        case KHR_IMAGE_ANDROID_NATIVE_BUFFER:
            Attributes->privHandle = Image->u.androidNativeBuffer.privHandle;
#endif
            if (Image->surface == gcvNULL)
            {
                status = gcvSTATUS_INVALID_ARGUMENT;
                break;
            }

            /* Query width and height from source. */
            gcmERR_BREAK(gcoSURF_GetSize(
                Image->surface, &Attributes->width, &Attributes->height, gcvNULL
                ));

            /* Query source surface format. */
            gcmERR_BREAK(gcoSURF_GetFormat(
                Image->surface, gcvNULL, &Attributes->format
                ));

            /* Query srouce surface stride. */
            gcmERR_BREAK(gcoSURF_GetAlignedSize(
                Image->surface, gcvNULL, gcvNULL, &Attributes->stride
                ));

            Attributes->surface = Image->surface;
            Attributes->level   = 0;
            Attributes->pixel   = gcvNULL;
            break;

        case KHR_IMAGE_PIXMAP:
            Attributes->surface = Image->surface;
            Attributes->stride  = Image->u.pixmap.stride;
            Attributes->level   = 0;
            Attributes->width   = Image->u.pixmap.width;
            Attributes->height  = Image->u.pixmap.height;
            Attributes->format  = Image->u.pixmap.format;
            Attributes->pixel   = Image->u.pixmap.address;

            /* Success. */
            status = gcvSTATUS_OK;
            break;

        default:
            status = gcvSTATUS_INVALID_ARGUMENT;
            break;
        }
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return status. */
    return status;
}


/******************************************************************************\
**************************** Buffer Management Code ****************************
\******************************************************************************/

/*******************************************************************************
**
**  glGenRenderbuffersOES (OES_framebuffer_object)
**
**  Returns 'Count' renderbuffer object names in 'RenderBuffers'. There is no
**  guarantee that the names form a contiguous set of integers; however, it is
**  guaranteed that none of the returned names was in use immediately before
**  the call to glGenRenderbuffers.
**
**  Renderbuffer object names returned by a call to glGenRenderbuffers are not
**  returned by subsequent calls, unless they are first deleted with
**  glDeleteRenderbuffers.
**
**  No renderbuffer objects are associated with the returned renderbuffer object
**  names until they are first bound by calling glBindRenderbuffer.
**
**  ERRORS:
**
**      GL_INVALID_VALUE
**          if Count is negative.
**
**  INPUT:
**
**      Count
**          Specifies the number of buffer object names to be generated.
**
**      RenderBuffers
**          Specifies an array in which the generated buffer object names
**          are stored.
**
**  OUTPUT:
**
**      RenderBuffers
**          An array filled with generated buffer names.
*/
#ifdef _GC_OBJ_ZONE
#undef _GC_OBJ_ZONE
#endif
#define _GC_OBJ_ZONE    glvZONE_BUFFER

GL_API void GL_APIENTRY glGenRenderbuffersOES(
    GLsizei Count,
    GLuint * RenderBuffers
    )
{
    glmENTER2(glmARGINT, Count, glmARGPTR, RenderBuffers)
    {
        GLsizei i;
        glsNAMEDOBJECT_PTR wrapper;

        /* Validate count. */
        if (Count < 0)
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Don't do anything if RenderBuffers is NULL. */
        if (RenderBuffers == gcvNULL)
        {
            break;
        }

        /* Generate buffers. */
        for (i = 0; i < Count; i++)
        {
            /* Create a new wrapper. */
            if (gcmIS_SUCCESS(_CreateRenderBuffer(context, 0, &wrapper)))
            {
                RenderBuffers[i] = wrapper->name;
            }
            else
            {
                RenderBuffers[i] = 0;
            }
        }

		gcmDUMP_API("${ES11 glGenRenderbuffersOES 0x%08X (0x%08X)", Count, RenderBuffers);
		gcmDUMP_API_ARRAY(RenderBuffers, Count);
		gcmDUMP_API("$}");
	}
    glmLEAVE();
}


/*******************************************************************************
**
**  glDeleteRenderbuffersOES (OES_framebuffer_object)
**
**  Deletes 'Count' renderbuffer objects named by the elements of the array
**  RenderBuffers. After a renderbuffer object is deleted, it has no contents,
**  and its name is free for reuse (for example by glGenRenderbuffers).
**
**  If a renderbuffer object that is currently bound is deleted, the binding
**  reverts to 0 (the absence of any renderbuffer object). Additionally, special
**  care must be taken when deleting a renderbuffer object if the image of the
**  renderbuffer is attached to a framebuffer object. In this case, if the
**  deleted renderbuffer object is attached to the currently bound framebuffer
**  object, it is automatically detached.  However, attachments to any other
**  framebuffer objects are the responsibility of the application.
**
**  glDeleteRenderbuffers silently ignores 0's and names that do not correspond
**  to existing renderbuffer objects.
**
**  ERRORS:
**
**      GL_INVALID_VALUE
**          if Count is negative.
**
**  INPUT:
**
**      Count
**          Specifies the number of buffer objects to be deleted.
**
**      Buffers
**          Specifies an array of buffer objects to be deleted.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glDeleteRenderbuffersOES(
    GLsizei Count,
    const GLuint * RenderBuffers
    )
{
    glmENTER2(glmARGINT, Count, glmARGPTR, RenderBuffers)
    {
        GLsizei i;
        glsCONTEXT_PTR shared;

        /* Map shared context. */
#if gcdRENDER_THREADS
        shared = (context->shared != gcvNULL) ? context->shared : context;
#else
        shared = context;
#endif

        /* Validate count. */
        if (Count < 0)
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Don't do anything if Buffers is NULL. */
        if (RenderBuffers == gcvNULL)
        {
            break;
        }

        gcmDUMP_API("${ES11 glDeleteRenderbuffersOES 0x%08X (0x%08X)", Count, RenderBuffers);
        gcmDUMP_API_ARRAY(RenderBuffers, Count);
        gcmDUMP_API("$}");

        /* Iterate through the buffer names. */
        for (i = 0; i < Count; i++)
        {
            glsNAMEDOBJECT_PTR wrapper = glfFindNamedObject(&context->renderBufferList, RenderBuffers[i]);

            if (gcvNULL == wrapper)
            {
                continue;
            }

            /* Unbound it from the current FBO */
            if (gcvNULL != context->frameBuffer)
            {
                if (context->frameBuffer->color.object == wrapper)
                {
                    if (gcvNULL != context->frameBuffer->color.target)
                    {
                        gcmVERIFY_OK(gcoSURF_Destroy(
                            context->frameBuffer->color.target
                            ));
                    }

                    glfDereferenceNamedObject(context, wrapper);

                    context->frameBuffer->color.texture = gcvFALSE;
                    context->frameBuffer->color.object  = gcvNULL;
                    context->frameBuffer->color.surface = gcvNULL;
                    context->frameBuffer->color.offset  = 0;
                    context->frameBuffer->color.target  = gcvNULL;
                    context->frameBuffer->dirty         = GL_TRUE;
                }

                if (context->frameBuffer->depth.object == wrapper)
                {
                    if (gcvNULL != context->frameBuffer->depth.target)
                    {
                        gcmVERIFY_OK(gcoSURF_Destroy(
                            context->frameBuffer->depth.target
                            ));
                    }

                    glfDereferenceNamedObject(context, wrapper);

                    context->frameBuffer->depth.texture = gcvFALSE;
                    context->frameBuffer->depth.object  = gcvNULL;
                    context->frameBuffer->depth.surface = gcvNULL;
                    context->frameBuffer->depth.offset  = 0;
                    context->frameBuffer->depth.target  = gcvNULL;
                    context->frameBuffer->dirty         = GL_TRUE;
                }

                if (context->frameBuffer->stencil.object == wrapper)
                {
                    if (gcvNULL != context->frameBuffer->stencil.target)
                    {
                        gcmVERIFY_OK(gcoSURF_Destroy(
                            context->frameBuffer->stencil.target
                            ));
                    }

                    glfDereferenceNamedObject(context, wrapper);

                    context->frameBuffer->stencil.texture = gcvFALSE;
                    context->frameBuffer->stencil.object  = gcvNULL;
                    context->frameBuffer->stencil.surface = gcvNULL;
                    context->frameBuffer->stencil.offset  = 0;
                    context->frameBuffer->stencil.target  = gcvNULL;
                    context->frameBuffer->dirty           = GL_TRUE;
                }
            }

            /* Unbound the RBO from current immediately */
            if (context->renderBuffer == (glsRENDER_BUFFER_PTR)wrapper->object)
            {
                context->renderBuffer = gcvNULL;
            }

            if (gcmIS_ERROR(glfDeleteNamedObject(
                context, &shared->renderBufferList, RenderBuffers[i]
                )))
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glIsRenderbufferOES (OES_framebuffer_object)
**
**  glIsRenderbuffer returns GL_TRUE if RenderBuffer is currently the name of
**  a RenderBuffer object. If RenderBuffer is zero, or is a non-zero value that
**  is not currently the name of a RenderBuffer object, or if an error occurs,
**  glIsRenderbuffer returns GL_FALSE.
**
**  A name returned by glGenRenderbuffers, but not yet associated with a
**  RenderBuffer object by calling glBindRenderbuffer, is not the name of
**  a RenderBuffer object.
**
**  INPUT:
**
**      RenderBuffer
**          Specifies a value that may be the name of a RenderBuffer object.
**
**  OUTPUT:
**
**      GL_TRUE or GL_FALSE (see above description.)
*/

GL_API GLboolean GL_APIENTRY glIsRenderbufferOES(
    GLuint RenderBuffer
    )
{
    GLboolean result = GL_FALSE;

    glmENTER1(glmARGUINT, RenderBuffer)
    {
        result = glfFindNamedObject(&context->renderBufferList, RenderBuffer)
               ? GL_TRUE
               : GL_FALSE;

		gcmDUMP_API("${ES11 glIsRenderbufferOES 0x%08X := 0x%08X}", RenderBuffer, result);
    }
    glmLEAVE();

    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  glBindRenderbufferOES (OES_framebuffer_object)
**
**  A renderbuffer is a data storage object containing a single image of a
**  renderable internal format.  A renderbuffer's image may be attached to
**  a framebuffer object to use as a destination for rendering and as a source
**  for reading.
**
**  glBindRenderbuffer lets you create or use a named renderbuffer object.
**  Calling glBindRenderbuffer with target set to GL_RENDERBUFFER and
**  renderbuffer set to the name of the new renderbuffer object binds
**  the renderbuffer object name. When a renderbuffer object is bound,
**  the previous binding is automatically broken.
**
**  Renderbuffer object names are unsigned integers. The value zero is reserved,
**  but there is no default renderbuffer object. Instead, renderbuffer set to
**  zero effectively unbinds any renderbuffer object previously bound.
**  Renderbuffer object names and the corresponding renderbuffer object contents
**  are local to the shared object space of the current GL rendering context.
**
**  You may use glGenRenderbuffers to generate a set of new renderbuffer object
**  names.
**
**  The state of a renderbuffer object immediately after it is first bound is
**  a zero-sized memory buffer with format GL_RGBA4 and zero-sized red, green,
**  blue, alpha, depth, and stencil pixel depths.
**
**  While a non-zero renderbuffer object name is bound, GL operations on target
**  GL_RENDERBUFFER affect the bound renderbuffer object, and queries of target
**  GL_RENDERBUFFER return state from the bound renderbuffer object. While
**  renderbuffer object name zero is bound, as in the initial state, attempts
**  to modify or query state on target GL_RENDERBUFFER generates an
**  GL_INVALID_OPERATION error.
**
**  A renderbuffer object binding created with glBindRenderbuffer remains active
**  until a different renderbuffer object name is bound, or until the bound
**  renderbuffer object is deleted with glDeleteRenderbuffers.
**
**  INPUT:
**
**      Target
**          Specifies the target to which the buffer object is bound.
**          The symbolic constant must be GL_ARRAY_BUFFER or
**          GL_ELEMENT_ARRAY_BUFFER.
**
**      Buffer
**          Specifies the name of a buffer object.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glBindRenderbufferOES(
    GLenum Target,
    GLuint RenderBuffer
    )
{
    glmENTER2(glmARGENUM, Target, glmARGUINT, RenderBuffer)
    {
        glsRENDER_BUFFER_PTR object;
        glsNAMEDOBJECT_PTR wrapper;
        glsCONTEXT_PTR shared;

		gcmDUMP_API("${ES11 glBindRenderbufferOES 0x%08X 0x%08X}",
                    Target, RenderBuffer);

        /* Map shared context. */
#if gcdRENDER_THREADS
        shared = (context->shared != gcvNULL) ? context->shared : context;
#else
        shared = context;
#endif

        /* Verify the target. */
        if (Target != GL_RENDERBUFFER_OES)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Remove current binding? */
        if (RenderBuffer == 0)
        {
            object = gcvNULL;
        }
        else
        {
            /* Find the object. */
            wrapper = glfFindNamedObject(&shared->renderBufferList, RenderBuffer);

            /* No such object yet? */
            if (wrapper == gcvNULL)
            {
                /* Create a new RenderBuffer. */
                if (gcmIS_ERROR(_CreateRenderBuffer(context, RenderBuffer, &wrapper)))
                {
                    glmERROR(GL_OUT_OF_MEMORY);
                    break;
                }
            }

            /* Get the object. */
            object = (glsRENDER_BUFFER_PTR) wrapper->object;
        }

        context->renderBuffer     = object;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glRenderbufferStorageOES (OES_framebuffer_object)
**
**  glRenderbufferStorage establishes the data storage, format, and dimensions
**  of a renderbuffer object's image. Any existing data store for the
**  renderbuffer is deleted and the contents of the new data store
**  are undefined.
**
**  An implementation may vary its allocation of internal component resolution
**  based on any glRenderbufferStorage parameter (except target), but the
**  allocation and chosen internal format must not be a function of any other
**  state and cannot be changed once they are established. The actual resolution
**  in bits of each component of the allocated image can be queried with
**  glGetRenderbufferParameteriv.
**
**  INPUT:
**
**      Target
**          Specifies the RenderBuffer target. The symbolic constant must be
**          GL_RENDERBUFFER_OES.
**
**      InternalFormat
**          Specifies the color-renderable, depth-renderable, or stencil-
**          renderable format of the RenderBuffer. Must be one of the
**          following symbolic constants:
**              GL_RGBA4,
**              GL_RGB565,
**              GL_RGB5_A1,
**              GL_DEPTH_COMPONENT16, or
**              GL_STENCIL_INDEX8.
**
**      Width
**          Specifies the width of the renderbuffer in pixels.
**
**      Height
**          Specifies the height of the renderbuffer in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/


GL_API void GL_APIENTRY glRenderbufferStorageOES(
    GLenum Target,
    GLenum InternalFormat,
    GLsizei Width,
    GLsizei Height
    )
{
    glmENTER4(glmARGENUM, Target, glmARGENUM, InternalFormat,
              glmARGINT, Width, glmARGINT, Height)
    {
        gceSTATUS status;
        gceSURF_TYPE type = gcvSURF_TYPE_UNKNOWN;
        gceSURF_FORMAT format = gcvSURF_UNKNOWN;

		gcmDUMP_API("${ES11 glRenderbufferStorageOES 0x%08X 0x%08X 0x%08X 0x%08X}",
			Target, InternalFormat, Width, Height);

        /* Verify the target. */
        if (Target != GL_RENDERBUFFER_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Make sure we have a render buffer bound. */
        if (context->renderBuffer == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Validate the requested size. */
        if (((gctUINT) Width  > context->maxWidth) ||
            ((gctUINT) Height > context->maxHeight))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Convert the format. */
        if (!_ConvertBufferFormat(InternalFormat, &type, &format))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Is there a storage already allocated? */
        if (context->renderBuffer->surface != gcvNULL)
        {
            /* Free the surface. */
            status = gcoSURF_Destroy(context->renderBuffer->surface);

            /* Failed? */
            if (gcmIS_ERROR(status))
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Reset. */
            context->renderBuffer->surface = gcvNULL;
        }

        /* Construct the surface. */
        status = gcoSURF_Construct(
            context->hal,
            Width, Height, 1,
            type, format,
            gcvPOOL_DEFAULT,
            &context->renderBuffer->surface
            );

        /* Failed? */
        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Set proper samples. */
        status = gcoSURF_SetSamples(
            context->renderBuffer->surface,
            context->drawSamples
            );

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }

        /* Set render buffer parameters. */
        context->renderBuffer->width  = Width;
        context->renderBuffer->height = Height;
        context->renderBuffer->format = InternalFormat;
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glGetRenderbufferParameterivOES (OES_framebuffer_object)
**
**  glGetRenderbufferParameteriv returns in params a selected parameter of the
**  currently bound renderbuffer object.
**
**  INPUT:
**
**      Target
**          Specifies the RenderBuffer target. The symbolic constant must be
**          GL_RENDERBUFFER_OES.
**
**      InternalFormat
**          Specifies the color-renderable, depth-renderable, or stencil-
**          renderable format of the RenderBuffer. Must be one of the
**          following symbolic constants:
**              GL_RGBA4,
**              GL_RGB565,
**              GL_RGB5_A1,
**              GL_DEPTH_COMPONENT16, or
**              GL_STENCIL_INDEX8.
**
**      Width
**          Specifies the width of the renderbuffer in pixels.
**
**      Height
**          Specifies the height of the renderbuffer in pixels.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glGetRenderbufferParameterivOES(
    GLenum Target,
    GLenum Name,
    GLint * Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Name, glmARGPTR, Params)
    {
        gceSURF_FORMAT          format;
        gcsSURF_FORMAT_INFO_PTR info[2];
        gceSTATUS               status;

        /* Verify the target. */
        if (Target != GL_RENDERBUFFER_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Make sure we have a render buffer bound. */
        if (context->renderBuffer == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Determine the current format. */
        if (context->renderBuffer->surface != gcvNULL)
        {
            gcmERR_BREAK(gcoSURF_GetFormat(
                context->renderBuffer->surface, gcvNULL, &format
                ));

            gcmERR_BREAK(gcoSURF_QueryFormat(
                format, info
                ));
        }
        else
        {
            format = gcvSURF_UNKNOWN;
            info[0] = info[1] = gcvNULL;
        }

        /* Dispatch on the parameter name. */
        switch (Name)
        {
        case GL_RENDERBUFFER_WIDTH_OES:
            * Params = context->renderBuffer->width;
            break;

        case GL_RENDERBUFFER_HEIGHT_OES:
            * Params = context->renderBuffer->height;
            break;

        case GL_RENDERBUFFER_INTERNAL_FORMAT_OES:
            * Params = context->renderBuffer->format;
            break;

        case GL_RENDERBUFFER_RED_SIZE_OES:
            * Params = ((info[0] != gcvNULL) &&
                        (info[0]->fmtClass == gcvFORMAT_CLASS_RGBA))
                    ? info[0]->u.rgba.red.width & gcvCOMPONENT_WIDTHMASK
                    : 0;
            break;

        case GL_RENDERBUFFER_GREEN_SIZE_OES:
            * Params = ((info[0] != gcvNULL) &&
                        (info[0]->fmtClass == gcvFORMAT_CLASS_RGBA))
                    ? info[0]->u.rgba.green.width & gcvCOMPONENT_WIDTHMASK
                    : 0;
            break;

        case GL_RENDERBUFFER_BLUE_SIZE_OES:
            * Params = ((info[0] != gcvNULL) &&
                        (info[0]->fmtClass == gcvFORMAT_CLASS_RGBA))
                    ? info[0]->u.rgba.blue.width & gcvCOMPONENT_WIDTHMASK
                    : 0;
            break;

        case GL_RENDERBUFFER_ALPHA_SIZE_OES:
            * Params = ((info[0] != gcvNULL) &&
                        (info[0]->fmtClass == gcvFORMAT_CLASS_RGBA))
                    ? info[0]->u.rgba.alpha.width & gcvCOMPONENT_WIDTHMASK
                    : 0;
            break;

        case GL_RENDERBUFFER_DEPTH_SIZE_OES:
            * Params = ((info[0] != gcvNULL) &&
                        (info[0]->fmtClass == gcvFORMAT_CLASS_DEPTH))
                    ? info[0]->u.depth.depth.width & gcvCOMPONENT_WIDTHMASK
                    : 0;
            break;

        case GL_RENDERBUFFER_STENCIL_SIZE_OES:
            * Params = ((info[0] != gcvNULL) &&
                        (info[0]->fmtClass == gcvFORMAT_CLASS_DEPTH))
                    ? info[0]->u.depth.stencil.width & gcvCOMPONENT_WIDTHMASK
                    : 0;
            break;

        default:
            glmERROR(GL_INVALID_ENUM);
        }

		gcmDUMP_API("${ES11 glGetRenderbufferParameterivOES 0x%08X 0x%08X (0x%08X)", Target, Name, Params);
		gcmDUMP_API_ARRAY(Params, 1);
		gcmDUMP_API("$}");
	}
    glmLEAVE();
}


/*******************************************************************************
**
**  glEGLImageTargetRenderbufferStorageOES (GL_OES_EGL_image)
**
**  Establishes the data storage, format, and dimensions of a renderbuffer
**  object's image, using the parameters and storage associated with the
**  eglImageOES <image>.  Assuming no errors are generated in this process,
**  the resulting renderbuffer will be an EGLImage target of the specifed
**  eglImageOES <image>. The renderbuffer will remain an EGLImage target
**  until it is deleted or respecified by a call to RenderbufferStorageOES.
**  As a result of this referencing operation, all of the pixel data in the
**  <buffer> used as the EGLImage source resource (i.e., the <buffer> parameter
**  passed to the CreateImageOES command that returned <image>) will become
**  undefined.
**
**  <target> must be RENDERBUFFER_OES, and <image> must be the handle of
**  a valid EGLImage resource, cast into the type eglImageOES.
**
**  If the GL is unable to create a renderbuffer using the specified
**  eglImageOES, the error INVALID_OPERATION is generated.  If <image> does not
**  refer to a valid eglImageOES object, the error INVALID_VALUE is generated.
**
**  ERRORS:
**
**      GL_INVALID_ENUM
**          - if Target is not RENDERBUFFER_OES.
**
**      GL_INVALID_OPERATION
**          - if the specified image cannot be operated on;
**
**  INPUT:
**
**      Target
**          Specifies the target texture. Must be GL_TEXTURE_2D.
**
**      Image
**          Specifies the source eglImage handle, cast to GLeglImageOES.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES(
    GLenum Target,
    GLeglImageOES Image
    )
{
    glmENTER2(glmARGENUM, Target, glmARGPTR, Image)
    {
        gceSTATUS status;
        khrEGL_IMAGE_PTR image;
        glsEGL_IMAGE_ATTRIBUTES attributes;
        gceSURF_TYPE type = gcvSURF_TYPE_UNKNOWN;

		gcmDUMP_API("${ES11 glEGLImageTargetRenderbufferStorageOES 0x%08X 0x%08X}", Target, Image);

        if (Target != GL_RENDERBUFFER_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Get the eglImage. */
        image = (khrEGL_IMAGE_PTR) Image;

        /* Get texture attributes from eglImage. */
        status = glfGetEGLImageAttributes(image, &attributes);

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Validate the size. */
        if ((attributes.width  > context->maxWidth) ||
            (attributes.height > context->maxHeight))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        if (!_ConvertImageType(attributes.format, &type))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Is there a storage already allocated? */
        if (context->renderBuffer->surface != gcvNULL)
        {
            /* Free the surface. */
            status = gcoSURF_Destroy(context->renderBuffer->surface);

            /* Failed? */
            if (gcmIS_ERROR(status))
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Reset. */
            context->renderBuffer->surface = gcvNULL;
        }

        /* Set render buffer parameters. */
        context->renderBuffer->width  = attributes.width;
        context->renderBuffer->height = attributes.height;

        /* Is there a surface associated with the image? */
        if (attributes.surface)
        {
            /* Yes, assume the surface. */
            context->renderBuffer->surface = attributes.surface;
            gcoSURF_ReferenceSurface(attributes.surface);
        }
        else
        {
            /* No, create a wrapper. */
            status = gcoSURF_Construct(
                context->hal, attributes.width, attributes.height, 1,
                type, attributes.format, gcvPOOL_USER,
                &context->renderBuffer->surface
                );

            if (gcmIS_ERROR(status))
            {
                glmERROR(GL_OUT_OF_MEMORY);
                break;
            }

            status = gcoSURF_MapUserSurface(
                context->renderBuffer->surface,
                0, attributes.pixel, ~0U
                );

            if (gcmIS_ERROR(status))
            {
                glmERROR(GL_OUT_OF_MEMORY);
                break;
            }
        }

        /* Set proper samples. */
        status = gcoSURF_SetSamples(
            context->renderBuffer->surface,
            context->drawSamples
            );

        if (gcmIS_ERROR(status))
        {
            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  veglCreateImageRenderBuffer (EGL_KHR_gl_renderbuffer_image)
**
**  Initialize the specified EGL image with the surface from the specified
**  render buffer object.
**
**  INPUT:
**
**      RenderBuffer
**          Source render buffer object name.
**
**      Image
**          EGL image to initialize.
**
**  OUTPUT:
**
**      Nothing.
*/

EGLenum
glfCreateImageRenderBuffer(
    GLenum RenderBuffer,
    gctPOINTER Image
    )
{
    EGLenum result;
    gcmHEADER_ARG("RenderBuffer=0x%04x Image=0x%x", RenderBuffer, Image);

    do
    {
        glsCONTEXT_PTR context;
        glsNAMEDOBJECT_PTR wrapper;
        glsRENDER_BUFFER_PTR object;
        gcoSURF surface;
        khrEGL_IMAGE * image;
        gctINT32 referenceCount;
        glsCONTEXT_PTR shared;

        /* Get the context pointer. */
        context = GetCurrentContext();
        if (context == gcvNULL)
        {
            result = EGL_BAD_ALLOC;
            break;
        }

        /* Map shared context. */
#if gcdRENDER_THREADS
        shared = (context->shared != gcvNULL) ? context->shared : context;
#else
        shared = context;
#endif

        /* Verify the render buffer name. */
        if (RenderBuffer == 0)
        {
            result = EGL_BAD_PARAMETER;
            break;
        }

        /* Find the object. */
        wrapper = glfFindNamedObject(&shared->renderBufferList, RenderBuffer);

        /* No such object yet? */
        if (wrapper == gcvNULL)
        {
            result = EGL_BAD_PARAMETER;
            break;
        }

        /* Get the object. */
        object = (glsRENDER_BUFFER_PTR) wrapper->object;

        /* Get render buffer surface. */
        surface = object->surface;

        if (surface == gcvNULL)
        {
            result = EGL_BAD_ACCESS;
            break;
        }

        /* Get source surface reference count. */
        gcmVERIFY_OK(gcoSURF_QueryReferenceCount(surface, &referenceCount));

        /* Test if surface is a sibling of any eglImage. */
        if (referenceCount > 1)
        {
            result = EGL_BAD_PARAMETER;
            break;
        }

        image = (khrEGL_IMAGE *) Image;

        /* Set EGL Image info. */
        image->magic   = KHR_EGL_IMAGE_MAGIC_NUM;
        image->type    = KHR_IMAGE_RENDER_BUFFER;
        image->surface = surface;

        /* Success. */
        result = EGL_SUCCESS;
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("result=0x%04x", result);
    /* Return result. */
    return result;
}
