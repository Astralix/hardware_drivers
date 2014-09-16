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
**  Buffer management for OpenGL ES 2.0 driver.
*/

#include "gc_glsh_precomp.h"

/*******************************************************************************
**  InitializeBuffer
**
**  Initialize a new context with buffer management private variables.
**
**  Arguments:
**
**      GLContext Context
**          Pointer to a new GLContext object.
*/
void
_glshInitializeBuffer(
    GLContext Context
    )
{
    gcmASSERT(Context != gcvNULL);

    /* No buffers bound yet. */
    Context->arrayBuffer        = gcvNULL;
    Context->elementArrayBuffer = gcvNULL;

    /* Zero buffers. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->bufferObjects,
                                  gcmSIZEOF(Context->bufferObjects)));
}

/*******************************************************************************
**  DeleteBuffer
**
**  Delete a buffer.
**
**  Arguments:
**
**      GLContext Context
**          Pointer to a GLContext structure.
**
**      GLBuffer Buffer
**          Pointer to a GLBuffer object.
*/
void
_glshDeleteBuffer(
    GLContext Context,
    GLBuffer Buffer
    )
{
    gcmASSERT(Context != gcvNULL);
    gcmASSERT(Buffer  != gcvNULL);

    /* Remove the buffer from the object list. */
    _glshRemoveObject(&Context->bufferObjects, &Buffer->object);

    /* Delete any gcoINDEX object. */
    if (Buffer->index != gcvNULL)
    {
        gcmVERIFY_OK(gcoINDEX_Destroy(Buffer->index));
    }

    /* Delete any gcoSTREAM object. */
    if (Buffer->stream != gcvNULL)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(Buffer->stream));
    }

#if gcdUSE_TRIANGLE_STRIP_PATCH
    if (Buffer->subCount > 0)
    {
        GLint i;

        for (i = 0; i < Buffer->subCount; i++)
        {
            gcmVERIFY_OK(gcoINDEX_Destroy(Buffer->subs[i].patchIndex));
            Buffer->subs[i].patchIndex  = gcvNULL;
        }

        Buffer->subCount = 0;
    }
#endif

    /* Free the GLBuffer object. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(Context->os, Buffer));
}

/*******************************************************************************
**  _NewBuffer
**
**  Create a new GLBuffer object.
**
**  Arguments:
**
**      GLContext Context
**          Pointer to a GLContext structure.
**
**      GLuint Name
**          Numeric name for GLBuffer object.
**
**  Returns:
**
**      GLBuffer
**          Pointer to the new GLBuffer object or gcvNULL if there is an error.
*/
#if gcdNULL_DRIVER < 3
static GLBuffer
_NewBuffer(
    GLContext Context,
    GLuint Name
    )
{
    GLBuffer buffer = gcvNULL;
    gceSTATUS status;
    gctPOINTER pointer = gcvNULL;

    for (;;)
    {
        /* Allocate memory for the buffer object. */
        gcmERR_BREAK(gcoOS_Allocate(Context->os,
                                    gcmSIZEOF(struct _GLBuffer),
                                    &pointer));

        buffer = pointer;

        /* Create a new object. */
        if (!_glshInsertObject(&Context->bufferObjects,
                               &buffer->object,
                               GLObject_Buffer,
                               Name))
        {
            break;
        }

        /* The buffer object is not yet bound. */
        buffer->target  = 0;
        buffer->size    = 0;
        buffer->index   = gcvNULL;
        buffer->stream  = gcvNULL;
        buffer->mapped = gcvFALSE;
        buffer->bufferMapPointer = gcvNULL;
        buffer->boundType = 0;

#if gcdUSE_TRIANGLE_STRIP_PATCH
        buffer->stripPatchDirty = gcvTRUE;
        buffer->subCount        = 0;

        gcoOS_ZeroMemory(buffer->subs, sizeof(buffer->subs));
#endif

        /* Return the buffer. */
        return buffer;
    }

    /* Roll back. */
    if (buffer != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(Context->os, buffer));
    }

    /* Out of memory error. */
    gcmFATAL("%s(%d): Out of memory", __FUNCTION__, __LINE__);
    gl2mERROR(GL_OUT_OF_MEMORY);
    return gcvNULL;
}
#endif

/*******************************************************************************
**  _DuplicateBufferData
**
**  Copy vetex data to index of a buffer, or inverse.
**
**  Arguments:
**
**      GLContext Context
**          Pointer to a GLContext structure.
**
**      GLBuffer Buffer
**          Pointer to a GLBuffer object.
**
**      gctBOOL ArrayToElement
**          Data copy direction.
**
*/
#if gcdNULL_DRIVER < 3
static gceSTATUS
_DuplicateBufferData(
    GLContext Context,
    GLBuffer Buffer,
    gctBOOL ArrayToElement
    )
{
    gceSTATUS  status = gcvSTATUS_OK;
    gctPOINTER address = gcvNULL;
    gctBOOL streamLocked = gcvFALSE;
    gctBOOL indexLocked  = gcvFALSE;

    gcmASSERT(Context != gcvNULL);
    gcmASSERT(Buffer  != gcvNULL);

    if (Buffer->size <= 0)
    {
        return status;
    }

    if (ArrayToElement)
    {
        if (Buffer->stream == gcvNULL)
        {
            return status;
        }

        /* Construct an index object if not present. */
        if (!Buffer->index)
        {
            gcmONERROR(gcoINDEX_Construct(Context->hal, &Buffer->index));
        }

        /* Get memory of array buffer. */
        gcmONERROR(gcoSTREAM_Lock(Buffer->stream, &address, gcvNULL));

        if (address != gcvNULL)
        {
            streamLocked = gcvTRUE;

            /* Upload to index. */
            gcmONERROR(gcoINDEX_Upload(Buffer->index, address, Buffer->size));
        }
    }
    else
    {
        if (Buffer->index == gcvNULL)
        {
            return status;
        }

        /* Construct a stream if no present. */
        if (Buffer->stream == gcvNULL)
        {
            gcmONERROR(gcoSTREAM_Construct(Context->hal, &Buffer->stream));
        }

        /* Reserve enough memory. */
        gcmONERROR(gcoSTREAM_Reserve(Buffer->stream, Buffer->size));

        /* Get memory of index buffer. */
        gcmONERROR(gcoINDEX_Lock(Buffer->index, gcvNULL, &address));

        if (address != gcvNULL)
        {
            indexLocked = gcvTRUE;

            /* Upload data into stream. */
            gcmONERROR(gcoSTREAM_Upload(Buffer->stream,
                                      address,
                                      0,
                                      Buffer->size,
                                      Buffer->usage == GL_DYNAMIC_DRAW));
        }
    }

OnError:
    /* Unlock the stream object. */
    if (streamLocked)
    {
        gcmVERIFY_OK(gcoSTREAM_Unlock(Buffer->stream));
    }

    /* Unlock the index object. */
    if (indexLocked)
    {
        gcmVERIFY_OK(gcoINDEX_Unlock(Buffer->index));
    }

    return status;
}
#endif
/*******************************************************************************
**  glGenBuffers
**
**  Generate a number of buffers.
**
**  Arguments:
**
**      GLsizei n
**          Number of buffers to generate.
**
**      GLuint * buffers
**          Pointer to an array of GLuint values that will receive the names of
**          the generated buffers.
**
*/
GL_APICALL void GL_APIENTRY
glGenBuffers(
    GLsizei n,
    GLuint * buffers
    )
{
#if gcdNULL_DRIVER < 3
    GLsizei i;
	GLboolean goToEnd = GL_FALSE;

	glmENTER1(glmARGINT, n)
    {

    glmPROFILE(context, GLES2_GENBUFFERS, 0);

    /* Test for invalid value. */
    if (n < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Loop while there are buffers to generate. */
    for (i = 0; i < n; ++i)
    {
        /* Create a new GLbuffer object. */
        GLBuffer buffer = _NewBuffer(context, 0);
        if (buffer == gcvNULL)
        {
			goToEnd = GL_TRUE;
            break;
        }

        /* Return the buffer name. */
        gcmTRACE_ZONE(gcvLEVEL_VERBOSE, gcvZONE_BUFFER,
                      "%s(%d): buffer object name %u",
                      __FUNCTION__, __LINE__, buffer->object.name);
        buffers[i] = buffer->object.name;
    }

	if (goToEnd)
		break;

    gcmDUMP_API("${ES20 glGenBuffers 0x%08X (0x%08X)", n, buffers);
    gcmDUMP_API_ARRAY(buffers, n);
    gcmDUMP_API("$}");

	}
	glmLEAVE();
#else
    while (n-- > 0)
    {
        *buffers++ = 3;
    }
#endif
}

/*******************************************************************************
**  glBindBuffer
**
**  Bind a buffer to the specified target or unbind the current buffer from the
**  specified target.
**
**  Arguments:
**
**      GLenum target
**          Target to bind the buffer to.  Can be one of the following values:
**              GL_ARRAY_BUFFER
**                  Bind the buffer to an array of vertex values.
**              GL_ELEMENT_ARRAY_BUFFER
**                  Bind the buffer to an array of element indices.
**
**      GLuint buffer
**          Buffer to bind or 0 to unbind the current buffer.
**
*/
GL_APICALL void GL_APIENTRY
glBindBuffer(
    GLenum target,
    GLuint buffer
    )
{
#if gcdNULL_DRIVER < 3
    GLBuffer object;
    gceSTATUS status;
    GLBuffer *elementArrayBuffer;
	GLboolean goToEnd = GL_FALSE;

	glmENTER2(glmARGENUM, target, glmARGUINT, buffer)
    {
    gcmDUMP_API("${ES20 glBindBuffer 0x%08X 0x%08X}", target, buffer);

    glmPROFILE(context, GLES2_BINDBUFFER, 0);

    if(context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &context->elementArrayBuffer;
    }
    else
    {
        elementArrayBuffer = &context->vertexArrayObject->elementArrayBuffer;
    }

    if (buffer == 0)
    {
        /* Remove current binding. */
        object = gcvNULL;
    }
    else
    {
        /* Find the object. */
        object = (GLBuffer) _glshFindObject(&context->bufferObjects, buffer);

        if (object == gcvNULL)
        {
            /* Create a new buffer. */
            object = _NewBuffer(context, buffer);
            if (object == gcvNULL)
            {
                break;
            }
        }

        /* Unbind buffer from currently bound target. */
        switch (object->target)
        {
        case GL_ARRAY_BUFFER:
            context->arrayBuffer = gcvNULL;
            break;

        case GL_ELEMENT_ARRAY_BUFFER:
            *elementArrayBuffer = gcvNULL;
            break;
        }

        /* Set target for buffer. */
        object->target = target;
    }

    /* Set buffer object for target. */
    switch (target)
    {
    case GL_ARRAY_BUFFER:
        /* Unbind current buffer. */
        if (context->arrayBuffer != gcvNULL)
        {
            context->arrayBuffer->target = GL_NONE;
        }

        /* Bind new bufer. */
        context->arrayBuffer = object;

        if (object != gcvNULL)
        {
            if (object->stream == gcvNULL)
            {
                /* Create gcoSTREAM object. */
                status = gcoSTREAM_Construct(context->hal, &object->stream);

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s(%d): gcoSTREAM_Construct returned %d(%s)",
                             __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
                    gl2mERROR(GL_OUT_OF_MEMORY);
					goToEnd = gcvTRUE;
                    break;
                }
            }

            /* If bound to index before, and first time bound to vertex. */
            if ((object->boundType & gldBOUNDTYPE_INDEX) &&
                !(object->boundType & gldBOUNDTYPE_VERTEX)
                )
            {
                status = _DuplicateBufferData(context, object, gcvFALSE);

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s(%d): _DuplicateBufferData returned %d",
                             __FUNCTION__, __LINE__, status);
                    gl2mERROR(GL_OUT_OF_MEMORY);
					goToEnd = gcvTRUE;
                    break;
                }
            }

            /* Set bound type. */
            object->boundType |= gldBOUNDTYPE_VERTEX;
        }
        break;

    case GL_ELEMENT_ARRAY_BUFFER:
        /* Unbind current buffer. */
        if (*elementArrayBuffer != gcvNULL)
        {
            (*elementArrayBuffer)->target = GL_NONE;
        }

        /* Bind new bufer. */
        *elementArrayBuffer = object;

        if (object != gcvNULL)
        {
            if (object->index == gcvNULL)
            {
                /* Create gcoINDEX object. */
                status = gcoINDEX_Construct(context->hal, &object->index);

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s(%d): gcoINDEX_Construct returned %d(%s)",
                             __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
                    gl2mERROR(GL_OUT_OF_MEMORY);
					goToEnd = gcvTRUE;
                    break;
                }
            }

            /* If bound to vertex before, and first time bound to index. */
            if ((object->boundType & gldBOUNDTYPE_VERTEX) &&
                !(object->boundType & gldBOUNDTYPE_INDEX)
               )
            {
                status = _DuplicateBufferData(context, object, gcvTRUE);

                if (gcmIS_ERROR(status))
                {
                    gcmFATAL("%s(%d): _DuplicateBufferData returned %d",
                             __FUNCTION__, __LINE__, status);
                    gl2mERROR(GL_OUT_OF_MEMORY);
					goToEnd = gcvTRUE;
                    break;
                }
            }

            /* Set bound type. */
            object->boundType |= gldBOUNDTYPE_INDEX;
        }
        break;

    default:
        gcmFATAL("%s(%d): Invalid target 0x%04X",
                 __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_OPERATION);
		goToEnd = gcvTRUE;
        break;
    }

	if (goToEnd)
		break;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glBufferData(
    GLenum target,
    GLsizeiptr size,
    const void* data,
    GLenum usage
    )
{
#if gcdNULL_DRIVER < 3
    GLBuffer buffer = gcvNULL;
    gceSTATUS status;
    GLBuffer *elementArrayBuffer;
	GLboolean goToEnd = gcvFALSE;

	glmENTER4(glmARGENUM, target, glmARGINT, size,
              glmARGPTR, data, glmARGENUM, usage)
    {
    gcmDUMP_API("${ES20 glBufferData 0x%08X 0x%08X (0x%08X) 0x%08X",
                target, size, data, usage);
    gcmDUMP_API_DATA(data, size);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_BUFFERDATA, 0);

    if (size < 0)
    {
        gcmTRACE(gcvLEVEL_WARNING, "glBufferData: invalid size %ld", size);
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if(context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &context->elementArrayBuffer;
    }
    else
    {
        elementArrayBuffer = &context->vertexArrayObject->elementArrayBuffer;
    }

    if (size == 0)
    {
        break;
	}

    switch (target)
    {
    case GL_ARRAY_BUFFER:
        buffer = context->arrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glBufferData: no active buffer for target: %s",
                     "GL_ARRAY_BUFFER");
            gl2mERROR(GL_INVALID_OPERATION);
            goToEnd = gcvTRUE;
        }
        break;

    case GL_ELEMENT_ARRAY_BUFFER:
        buffer = *elementArrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glBufferData: no active buffer for target: %s",
                     "GL_ELEMENT_ARRAY_BUFFER");
            gl2mERROR(GL_INVALID_OPERATION);
			goToEnd = gcvTRUE;
        }
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X",
                 __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_OPERATION);
        goToEnd = gcvTRUE;
		break;
    }

	if (goToEnd)
		break;

    /* If had been bound to vertex.*/
    if (buffer->boundType & gldBOUNDTYPE_VERTEX)
    {
        if (buffer->stream != gcvNULL)
        {
            status = gcoSTREAM_Destroy(buffer->stream);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): gcoSTREAM_Destroy returned %d",
                          __FUNCTION__, __LINE__, status);
                gl2mERROR(GL_OUT_OF_MEMORY);
                gcmFOOTER_NO();
                return;
            }

            buffer->stream = gcvNULL;
        }

        status = gcoSTREAM_Construct(context->hal,
                                     &buffer->stream);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoSTREAM_Construct returned %d(%s)",
                     __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }

        status = gcoSTREAM_Reserve(buffer->stream, size);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoSTREAM_Reserve returned %d(%s)",
                     __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }

        status = gcoSTREAM_Upload(buffer->stream,
                                  data,
                                  0,
                                  size,
                                  usage == GL_DYNAMIC_DRAW);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoSTREAM_Upload returned %d(%s)",
                     __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }
    }

    /* If had been bound to index.*/
    if (buffer->boundType & gldBOUNDTYPE_INDEX)
    {
        if (buffer->index != gcvNULL)
        {
            status = gcoINDEX_Destroy(buffer->index);

            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): gcoINDEX_Destroy returned %d(%s)",
                         __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
                gl2mERROR(GL_OUT_OF_MEMORY);
                break;
            }

            buffer->index = gcvNULL;
        }

        status = gcoINDEX_Construct(context->hal, &buffer->index);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoINDEX_Construct returned %d(%s)",
                     __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }

        status = gcoINDEX_Upload(buffer->index, data, size);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoINDEX_Upload returned %d(%s)",
                     __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }
    }

    buffer->usage = usage;
    buffer->size  = size;
    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

#if gcdUSE_TRIANGLE_STRIP_PATCH
    buffer->stripPatchDirty = gcvTRUE;
#endif
	}
	glmLEAVE();
#endif
}

GL_APICALL void GL_APIENTRY
glBufferSubData(
    GLenum target,
    GLintptr offset,
    GLsizeiptr size,
    const void *data
    )
{
#if gcdNULL_DRIVER < 3
    GLBuffer buffer = gcvNULL;
    gceSTATUS status;
    GLBuffer *elementArrayBuffer;
	GLboolean goToEnd = gcvFALSE;

    glmENTER4(glmARGENUM, target, glmARGINT, offset,
              glmARGPTR, size, glmARGPTR, data)
	{
    gcmDUMP_API("${ES20 glBufferSubData 0x%08X 0x%08X 0x%08X (0x%08X)",
                target, offset, size, data);
    gcmDUMP_API_DATA(data, size);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_BUFFERSUBDATA, 0);

    /* Verify the arguments. */
    if ( (offset < 0) || (size < 0) )
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if (size == 0)
    {
        break;
    }

    if(context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &context->elementArrayBuffer;
    }
    else
    {
        elementArrayBuffer = &context->vertexArrayObject->elementArrayBuffer;
    }

    switch (target)
    {
    case GL_ARRAY_BUFFER:
        buffer = context->arrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glBufferSubData: no active buffer for target: %s",
                     "GL_ARRAY_BUFFER");
            gl2mERROR(GL_INVALID_OPERATION);
            goToEnd = gcvTRUE;
        }
        break;

    case GL_ELEMENT_ARRAY_BUFFER:
        buffer = *elementArrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glBufferSubData: no active buffer for target: %s",
                     "GL_ELEMENT_ARRAY_BUFFER");
            gl2mERROR(GL_INVALID_OPERATION);
            goToEnd = gcvTRUE;
        }
        break;

    default:
        gcmFATAL("glBufferSubData: Invalid target: 0x%04X", target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = gcvTRUE;
		break;
    }

    if (goToEnd)
		break;

    if(buffer->mapped)
    {

        gcmFATAL("%s(%d): Invalid target: 0x%04X",
                 __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if (offset + size > buffer->size)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* If had been bound to vertex.*/
    if (buffer->boundType & gldBOUNDTYPE_VERTEX)
    {
        status = gcoSTREAM_Upload(buffer->stream,
                                  data,
                                  offset,
                                  size,
                                  buffer->usage == GL_DYNAMIC_DRAW);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoSTREAM_Upload returned %d",
                     __FUNCTION__, __LINE__, status);
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }
    }

    /* If had been bound to index.*/
    if (buffer->boundType & gldBOUNDTYPE_INDEX)
    {
        status = gcoINDEX_UploadOffset(buffer->index, offset, data, size);

        if (gcmIS_ERROR(status))
        {
            gcmFATAL("%s(%d): gcoINDEX_UploadOffset returned %d(%s)",
                     __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
            gl2mERROR(GL_OUT_OF_MEMORY);
            break;
        }
    }

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

#if gcdUSE_TRIANGLE_STRIP_PATCH
    buffer->stripPatchDirty = gcvTRUE;
#endif
    }
	glmLEAVE();
#endif
}

GL_APICALL GLboolean GL_APIENTRY
glIsBuffer(
    GLuint buffer
    )
{
#if gcdNULL_DRIVER < 3
    GLObject object;
    GLboolean isBuffer = GL_FALSE;

    glmENTER1(glmARGUINT, buffer)
	{
    glmPROFILE(context, GLES2_ISBUFFER, 0);

    /* Find the object. */
    object = _glshFindObject(&context->bufferObjects, buffer);

    isBuffer = (object != gcvNULL) && (object->type == GLObject_Buffer);

    gcmTRACE(gcvLEVEL_VERBOSE,
             "glIsBuffer ==> %s",
             isBuffer ? "GL_TRUE" : "GL_FALSE");

    gcmDUMP_API("${ES20 glIsBuffer 0x%08X := 0x%08X}", buffer, isBuffer);
	}
	glmLEAVE1(glmARGINT, isBuffer);
    return isBuffer;
#else
    return (buffer == 3) ? GL_TRUE : GL_FALSE;
#endif
}

GL_APICALL void GL_APIENTRY
glDeleteBuffers(
    GLsizei n,
    const GLuint *buffers
    )
{
#if gcdNULL_DRIVER < 3
    GLsizei i;
    GLuint j;
    GLBuffer buffer;
    GLBuffer *elementArrayBuffer;
#if GL_USE_VERTEX_ARRAY
    gcsVERTEXARRAY *vertexArray;
    GLVertex *vertexArrayGL;
#else
    GLAttribute *vertexArray;
#endif

	glmENTER2(glmARGINT, n, glmARGPTR, buffers)
	{
    gcmDUMP_API("${ES20 glDeleteBuffers 0x%08X (0x%08X)", n, buffers);
    gcmDUMP_API_ARRAY(buffers, n);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_DELETEBUFFERS, 0);

    if (n < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    if(context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &context->elementArrayBuffer;
        vertexArray = context->vertexArray;
#if GL_USE_VERTEX_ARRAY
        vertexArrayGL = context->vertexArrayGL;
#endif
    }
    else
    {
        elementArrayBuffer = &(context->vertexArrayObject->elementArrayBuffer);
        vertexArray = context->vertexArrayObject->vertexArray;
#if GL_USE_VERTEX_ARRAY
        vertexArrayGL = context->vertexArrayObject->vertexArrayGL;
#endif
    }

    for (i = 0; i < n; i++)
    {
        if (buffers[i] == 0)
        {
            continue;
        }

        buffer = (GLBuffer) _glshFindObject(&context->bufferObjects, buffers[i]);

        if ( (buffer == gcvNULL) || (buffer->object.type != GLObject_Buffer) )
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glDeleteBuffers: Object %d is not a buffer",
                     buffers[i]);
            gl2mERROR(GL_INVALID_VALUE);
            break;
        }

        /* Remove the buffer from any bound vertex array. */
        /* The delete strategy of object is wrong! */
        for (j = 0; j < context->maxAttributes; ++j)
        {
#if GL_USE_VERTEX_ARRAY
            if (vertexArrayGL[j].buffer == buffer)
            {
                vertexArrayGL[j].buffer = gcvNULL;
                vertexArray[j].stream   = gcvNULL;

                /* Disable batch optmization. */
                context->batchDirty = GL_TRUE;
            }
#else
            if (vertexArray[j].buffer == buffer)
            {
                vertexArray[j].buffer = gcvNULL;

                /* Disable batch optmization. */
                context->batchDirty = GL_TRUE;
            }
#endif
        }

        /* Unbind from currently bound target. */
        switch (buffer->target)
        {
        case GL_ARRAY_BUFFER:
            context->arrayBuffer = gcvNULL;
            break;

        case GL_ELEMENT_ARRAY_BUFFER:
            *elementArrayBuffer = gcvNULL;
            break;
        }

        _glshDeleteBuffer(context, buffer);
    }

	}
	glmLEAVE();
#endif
}

/* Buffer Parameter Query Function: By Vincent. */
GL_APICALL void GL_APIENTRY
glGetBufferParameteriv(
    GLenum target,
    GLenum pname,
    GLint  *params
    )
{
#if gcdNULL_DRIVER < 3
    GLBuffer buffer = gcvNULL;
    GLBuffer *elementArrayBuffer;
	GLboolean goToEnd = gcvFALSE;

	glmENTER2(glmARGENUM, target, glmARGENUM, pname)
    {

    if(context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &context->elementArrayBuffer;
    }
    else
    {
        elementArrayBuffer = &context->vertexArrayObject->elementArrayBuffer;
    }

    glmPROFILE(context, GLES2_GETBUFFERPARAMETERIV, 0);

    switch (target)
    {
    case GL_ARRAY_BUFFER:
        buffer = context->arrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glGetBufferParameteriv: no active buffer for target: %s",
                     "GL_ARRAY_BUFFER");
            gl2mERROR(GL_INVALID_OPERATION);
            goToEnd = gcvTRUE;
        }
        break;

    case GL_ELEMENT_ARRAY_BUFFER:
        buffer = *elementArrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glGetBufferParameteriv: no active buffer for target: %s",
                     "GL_ELEMENT_ARRAY_BUFFER");
            gl2mERROR(GL_INVALID_OPERATION);
            goToEnd = gcvTRUE;
        }
        break;

    default:
        gcmFATAL("%s(%d): Invalid target: 0x%04X",
                 __FUNCTION__, __LINE__, target);
        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = gcvTRUE;
		break;
    }

	if (goToEnd)
		break;

    switch (pname)
    {
    case GL_BUFFER_SIZE:
        *params = buffer->size;
        break;

    case GL_BUFFER_USAGE:
        *params = buffer->usage;
        break;

    case GL_BUFFER_ACCESS_OES:
        *params = GL_WRITE_ONLY_OES;
        break;

    case GL_BUFFER_MAPPED_OES:
        *params = buffer->mapped;
        break;

    default:
        gcmFATAL("%s(%d): Invalid parameter: 0x%04X",
                 __FUNCTION__, __LINE__, pname);
        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = gcvTRUE;
        break;
    }

	if (goToEnd)
		break;

    gcmDUMP_API("${ES20 glGetBufferParameteriv 0x%08X 0x%08X (0x%08X)",
                target, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
	}
	glmLEAVE1(glmARGINT, *params);
#else
    *params = 0;
#endif
}

/*******************************************************************************
**
**  glMapBufferOES
**
**  INPUT:
**
**      Target
**          Specifies the target buffer object. The symbolic constant must be
**          GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER.
**
**      Access
**          Specifies the access mode of a buffer object. Accepted
**          values is only GL_WRITE_ONLY_OES
**
**  OUTPUT:
**
**      see descriptions.
*/

GL_APICALL void* GL_APIENTRY
glMapBufferOES (
    GLenum target,
    GLenum access
    )
{
#if gcdNULL_DRIVER < 3
    GLBuffer buffer = gcvNULL;
    GLBuffer *elementArrayBuffer;
    gceSTATUS status;
	GLboolean goToEnd = gcvFALSE;
	void *result = gcvNULL;

	glmENTER2(glmARGENUM, target, glmARGENUM, access)
	{
    gcmDUMP_API("${ES20 glMapBufferOES 0x%08X 0x%08X}", target, access);

    if(access != GL_WRITE_ONLY_OES)
    {
        gcmFATAL("glMapBufferOES: Invalid access: 0x%04X", access);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    if(context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &context->elementArrayBuffer;
    }
    else
    {
        elementArrayBuffer = &context->vertexArrayObject->elementArrayBuffer;
    }
    glmPROFILE(context, GLES2_GLMAPBUFFEROES, 0);

    switch (target)
    {
    case GL_ARRAY_BUFFER:
        buffer = context->arrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glMapBufferOES: no active buffer for target: %s",
                     "GL_ARRAY_BUFFER");
        }
        break;

    case GL_ELEMENT_ARRAY_BUFFER:
        buffer = *elementArrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glMapBufferOES: no active buffer for target: %s",
                     "GL_ELEMENT_ARRAY_BUFFER");
        }
        break;

    default:
        gcmFATAL("glMapBufferOES: Invalid target: 0x%04X", target);
        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = gcvTRUE;
        break;
    }

	if (goToEnd)
		break;

    if(buffer == gcvNULL)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if(buffer->mapped)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if(buffer->size == 0)
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        break;
    }
    if(target == GL_ARRAY_BUFFER)
    {
        status = gcoSTREAM_CPUCacheOperation(buffer->stream, gcvCACHE_INVALIDATE);
        if (gcmIS_ERROR(status))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            break;

        }

        gcoSTREAM_Lock(buffer->stream,
            &buffer->bufferMapPointer,
            gcvNULL);
    }
    if(target == GL_ELEMENT_ARRAY_BUFFER)
    {
        gcoINDEX_Lock(buffer->index, gcvNULL, &buffer->bufferMapPointer);
    }

    buffer->mapped = gcvTRUE;
	result = buffer->bufferMapPointer;

	}
	glmLEAVE();

    return result;

#endif
}

GL_APICALL GLboolean GL_APIENTRY
glUnmapBufferOES (
    GLenum target
    )
{
#if gcdNULL_DRIVER < 3

    GLBuffer buffer = gcvNULL;
    GLBuffer *elementArrayBuffer;
    gceSTATUS status;
    GLboolean result = gcvFALSE;
	GLboolean goToEnd = gcvFALSE;

	glmENTER1(glmARGENUM, target)
	{
    gcmDUMP_API("${ES20 glMapBufferOES 0x%08X}", target);

    if(context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &context->elementArrayBuffer;
    }
    else
    {
        elementArrayBuffer = &context->vertexArrayObject->elementArrayBuffer;
    }
    glmPROFILE(context, GLES2_GLUNMAPBUFFEROES, 0);

    switch (target)
    {
    case GL_ARRAY_BUFFER:
        buffer = context->arrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glUnmapBufferOES: no active buffer for target: %s",
                     "GL_ARRAY_BUFFER");
        }
        break;

    case GL_ELEMENT_ARRAY_BUFFER:
        buffer = *elementArrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glUnmapBufferOES: no active buffer for target: %s",
                     "GL_ELEMENT_ARRAY_BUFFER");
        }
        break;

    default:
        gcmFATAL("glUnmapBufferOES: Invalid target: 0x%04X", target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = gcvTRUE;
		break;
    }

	if (goToEnd)
		break;

    if(buffer == gcvNULL)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if(buffer->mapped)
    {
        if(target == GL_ARRAY_BUFFER)
        {
            status = gcoSTREAM_CPUCacheOperation(buffer->stream, gcvCACHE_FLUSH);
            if (gcmIS_ERROR(status))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                break;
            }
        }
        if(target == GL_ELEMENT_ARRAY_BUFFER)
        {
            gcoINDEX_Unlock(buffer->index);
        }
        buffer->bufferMapPointer = gcvNULL;
        buffer->mapped = gcvFALSE;
		result = gcvTRUE;
		break;
    }

    gl2mERROR(GL_INVALID_OPERATION);
	}
	glmLEAVE();
	return result;

#endif
}

GL_APICALL void GL_APIENTRY
glGetBufferPointervOES (
    GLenum target,
    GLenum pname,
    GLvoid ** params
    )
{
#if gcdNULL_DRIVER < 3

    GLBuffer buffer = gcvNULL;
    GLBuffer *elementArrayBuffer;
	GLboolean goToEnd = gcvFALSE;

	glmENTER3(glmARGENUM, target, glmARGENUM, pname, glmARGPTR, *params)
	{

    if(pname != GL_BUFFER_MAP_POINTER_OES)
    {
        gcmFATAL("glGetBufferPointervOES: Invalid pname: 0x%04X", pname);
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    if(context->vertexArrayObject == gcvNULL)
    {
        elementArrayBuffer = &context->elementArrayBuffer;
    }
    else
    {
        elementArrayBuffer = &context->vertexArrayObject->elementArrayBuffer;
    }

    glmPROFILE(context, GLES2_GLGETBUFFERPOINTERVOES, 0);

    switch (target)
    {
    case GL_ARRAY_BUFFER:
        buffer = context->arrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glGetBufferPointervOES: no active buffer for target: %s",
                     "GL_ARRAY_BUFFER");
        }
        break;

    case GL_ELEMENT_ARRAY_BUFFER:
        buffer = *elementArrayBuffer;

        if (buffer == gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glGetBufferPointervOES: no active buffer for target: %s",
                     "GL_ELEMENT_ARRAY_BUFFER");
        }
        break;

    default:
        gcmFATAL("glGetBufferPointervOES: Invalid target: 0x%04X", target);
        gl2mERROR(GL_INVALID_ENUM);
        goToEnd = gcvTRUE;
		break;
    }
	if (goToEnd)
		break;

    if(buffer == gcvNULL)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        break;;
    }

    if(buffer->mapped)
        *params = buffer->bufferMapPointer;
    else
        *params = (GLvoid*)gcvNULL;

    gcmDUMP_API("${ES20 glGetBufferPointervOES 0x%08X 0x%08X (0x%08X)",
        target, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
	}
	glmLEAVE1(glmARGINT, *params);

#endif
}


