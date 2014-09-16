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

#define _GC_OBJ_ZONE    glvZONE_BUFFER

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  _ClearBindings
**
**  Clear all current buffer bindings.
**
**  INPUT:
**
**      Object
**          Pointer to the object to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/

static void _ClearBindings(
    IN gctPOINTER Object
    )
{
    gctUINT32 i;

    /* Cast the object. */
    glsBUFFER_PTR object = (glsBUFFER_PTR) Object;

    gcmHEADER_ARG("Object=0x%x", Object);

    /* Reset the bound target. */
    object->bound            = gcvFALSE;
    object->boundAtLeastOnce = gcvFALSE;

    /* Reset bound streams. */
    for (i = 0; i < glvTOTALBINDINGS; i++)
    {
        if (object->binding[i])
        {
            *object->binding[i] = gcvNULL;
            object->binding[i]  = gcvNULL;
        }
    }
    gcmFOOTER_NO();
}

/*******************************************************************************
**
**  _ClearMapping
**
**  INPUT:
**
**      Object
**          Pointer to the object to be destroyed.
**
**  OUTPUT:
**
**      Nothing.
*/
static void _ClearMapping(
    IN gctPOINTER Object
    )
{
    glsBUFFER_PTR object = (glsBUFFER_PTR) Object;

    /* reset map status */
    object->mapped = gcvFALSE;
    object->bufferMapPointer = gcvNULL;
}


/*******************************************************************************
**
**  _DeleteBuffer
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

static gceSTATUS _DeleteBuffer(
    IN glsCONTEXT_PTR Context,
    IN gctPOINTER Object
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Context=0x%x Object=0x%x", Context, Object);

    do
    {
        /* Cast the object. */
        glsBUFFER_PTR object = (glsBUFFER_PTR) Object;

        /* Clear bindings. */
        _ClearBindings(Object);

        /* Clear mapping */
        _ClearMapping(Object);

        /* Destroy index object. */
        if (object->index)
        {
            gcoINDEX_Destroy(object->index);
            object->index = gcvNULL;
        }

        /* Destroy stream object. */
        if (object->stream)
        {
            gcoSTREAM_Destroy(object->stream);
            object->stream = gcvNULL;
        }

#if gcdUSE_TRIANGLE_STRIP_PATCH
        if (object->subCount > 0)
        {
            GLint i;

            for (i = 0; i < object->subCount; i++)
            {
                gcoINDEX_Destroy(object->subs[i].patchIndex);
                object->subs[i].patchIndex  = gcvNULL;
            }

            object->subCount = 0;
        }
#endif
        /* Reset the fields. */
        object->size = 0;
        object->usage = GL_STATIC_DRAW;
    }
    while (gcvFALSE);

    gcmFOOTER();

    return status;
}

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
    glsCONTEXT_PTR Context,
    glsBUFFER_PTR  Buffer,
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

#if gcdUSE_TRIANGLE_STRIP_PATCH
    Buffer->stripPatchDirty = gcvTRUE;
#endif

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
**
**  _CreateBuffer
**
**  Buffer constructor.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Buffer
**          Name of the buffer to create.
**
**  OUTPUT:
**
**      Wrapper
**          Points to the created named object.
*/

static gceSTATUS _CreateBuffer(
    IN glsCONTEXT_PTR Context,
    IN gctUINT32 Buffer,
    OUT glsNAMEDOBJECT_PTR * Wrapper
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x Buffer=%u Wrapper=0x%x",
                  Context, Buffer, Wrapper);

    do
    {
        glsBUFFER_PTR object;
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
            &shared->bufferList,
            Buffer,
            _DeleteBuffer,
            Wrapper
            ));

        /* Get a pointer to the new buffer. */
        object = (glsBUFFER_PTR) (*Wrapper)->object;

        /* Init the buffer to the defaults. */
        gcoOS_ZeroMemory(object, sizeof(glsBUFFER));
        object->usage = GL_STATIC_DRAW;
        object->mapped = gcvFALSE;
        object->bufferMapPointer = gcvNULL;

#if gcdUSE_TRIANGLE_STRIP_PATCH
        object->stripPatchDirty = gcvTRUE;
        object->subCount        = 0;

        gcoOS_ZeroMemory(object->subs, sizeof(object->subs));
#endif
    }
    while (gcvFALSE);

    gcmFOOTER();

    /* Return result. */
    return status;
}


/*******************************************************************************
**
**  glfQueryBufferState
**
**  Queries alpha state values.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      Name
**          Specifies the symbolic name of the state to get.
**
**      Type
**          Data format.
**
**  OUTPUT:
**
**      Value
**          Points to the data.
*/

GLboolean glfQueryBufferState(
    glsCONTEXT_PTR Context,
    GLenum Name,
    GLvoid* Value,
    gleTYPE Type
    )
{
    GLboolean result = GL_TRUE;

    gcmHEADER_ARG("Context=0x%x Name=0x%04x Value=0x%x Type=0x%04x",
                  Context, Name, Value, Type);

    switch (Name)
    {
    case GL_ARRAY_BUFFER_BINDING:
        glfGetFromInt(
            (Context->arrayBuffer == gcvNULL)
                ? 0
                : Context->arrayBuffer->name,
            Value,
            Type
            );
        break;

    case GL_ELEMENT_ARRAY_BUFFER_BINDING:
        glfGetFromInt(
            (Context->elementArrayBuffer == gcvNULL)
                ? 0
                : Context->elementArrayBuffer->name,
            Value,
            Type
            );
        break;

    case GL_VERTEX_ARRAY_BUFFER_BINDING:
        glfGetFromInt(
            (Context->aPositionInfo.buffer == gcvNULL)
                ? 0
                : Context->aPositionInfo.buffer->name,
            Value,
            Type
            );
        break;

    case GL_NORMAL_ARRAY_BUFFER_BINDING:
        glfGetFromInt(
            (Context->aNormalInfo.buffer == gcvNULL)
                ? 0
                : Context->aNormalInfo.buffer->name,
            Value,
            Type
            );
        break;

    case GL_COLOR_ARRAY_BUFFER_BINDING:
        glfGetFromInt(
            (Context->aColorInfo.buffer == gcvNULL)
                ? 0
                : Context->aColorInfo.buffer->name,
            Value,
            Type
            );
        break;

    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING:
        {
            glsTEXTURESAMPLER_PTR sampler =
                Context->texture.activeClientSampler;

            glfGetFromInt(
                (sampler->aTexCoordInfo.buffer == gcvNULL)
                    ? 0
                    : sampler->aTexCoordInfo.buffer->name,
                Value,
                Type
                );
        }
        break;

    case GL_POINT_SIZE_ARRAY_BUFFER_BINDING_OES:
        glfGetFromInt(
            (Context->aPointSizeInfo.buffer == gcvNULL)
                ? 0
                : Context->aPointSizeInfo.buffer->name,
            Value,
            Type
            );
        break;

    case GL_MATRIX_INDEX_ARRAY_BUFFER_BINDING_OES:
        glfGetFromInt(
            (Context->aMatrixIndexInfo.buffer == gcvNULL)
                ? 0
                : Context->aMatrixIndexInfo.buffer->name,
            Value,
            Type
            );
        break;

    case GL_WEIGHT_ARRAY_BUFFER_BINDING_OES:
        glfGetFromInt(
            (Context->aWeightInfo.buffer == gcvNULL)
                ? 0
                : Context->aWeightInfo.buffer->name,
            Value,
            Type
            );
        break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("return=%d", result);

    /* Return result. */
    return result;
}


/******************************************************************************\
**************************** Buffer Management Code ****************************
\******************************************************************************/

/*******************************************************************************
**
**  glGenBuffers
**
**  glGenBuffers returns 'Count' buffer object names in buffers. There is no
**  guarantee that the names form a contiguous set of integers; however, it is
**  guaranteed that none of the returned names was in use immediately before
**  the call to glGenBuffers.
**
**  Buffer object names returned by a call to glGenBuffers are not returned by
**  subsequent calls, unless they are first deleted with glDeleteBuffers.
**
**  No buffer objects are associated with the returned buffer object names until
**  they are first bound by calling glBindBuffer.
**
**  INPUT:
**
**      Count
**          Specifies the number of buffer object names to be generated.
**
**      Buffers
**          Specifies an array in which the generated buffer object names
**          are stored.
**
**  OUTPUT:
**
**      Buffers
**          An array filled with generated buffer names.
*/

GL_API void GL_APIENTRY glGenBuffers(
    GLsizei Count,
    GLuint* Buffers
    )
{
    glmENTER2(glmARGINT, Count, glmARGPTR, Buffers)
    {
        GLsizei i;
        glsNAMEDOBJECT_PTR wrapper;

		glmPROFILE(context, GLES1_GENBUFFERS, 0);

        /* Validate count. */
        if (Count < 0)
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Don't do anything if Buffers is NULL. */
        if (Buffers == gcvNULL)
        {
            break;
        }

        /* Generate buffers. */
        for (i = 0; i < Count; i++)
        {
            /* Create a new wrapper. */
            if (gcmIS_SUCCESS(_CreateBuffer(context, 0, &wrapper)))
            {
                Buffers[i] = wrapper->name;
            }
            else
            {
                Buffers[i] = 0;
            }
        }

		gcmDUMP_API("${ES11 glGenBuffers 0x%08X (0x%08X)", Count, Buffers);
		gcmDUMP_API_ARRAY(Buffers, Count);
		gcmDUMP_API("$}");
	}
    glmLEAVE();
}


/*******************************************************************************
**
**  glDeleteBuffers
**
**  glDeleteBuffers deletes Count buffer objects named by the elements of the
**  array 'Buffers'. After a buffer object is deleted, it has no contents,
**  and its name is free for reuse (for example by glGenBuffers).
**
**  If a buffer object that is currently bound is deleted, the binding reverts
**  to 0 (the absence of any buffer object, which reverts to client memory
**  usage).
**
**  glDeleteBuffers silently ignores 0's and names that do not correspond to
**  existing buffer objects.
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

GL_API void GL_APIENTRY glDeleteBuffers(
    GLsizei Count,
    const GLuint* Buffers
    )
{
    glmENTER2(glmARGINT, Count, glmARGPTR, Buffers)
    {
        GLsizei i;
        glsCONTEXT_PTR shared;

        gcmDUMP_API("${ES11 glDeleteBuffers 0x%08X (0x%08X)", Count, Buffers);
        gcmDUMP_API_ARRAY(Buffers, Count);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_DELETEBUFFERS, 0);

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
        if (Buffers == gcvNULL)
        {
            break;
        }

        /* Iterate through the buffer names. */
        for (i = 0; i < Count; i++)
        {
            glsNAMEDOBJECT_PTR wrapper = glfFindNamedObject(&shared->bufferList, Buffers[i]);
            if (gcvNULL == wrapper)
            {
                continue;
            }

            _ClearBindings(wrapper->object);

            if (gcmIS_ERROR(glfDeleteNamedObject(
                context, &shared->bufferList, Buffers[i]
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
**  glIsBuffer
**
**  glIsBuffer returns GL_TRUE if buffer is currently the name of a buffer
**  object. If buffer is zero, or is a non-zero value that is not currently
**  the name of a buffer object, or if an error occurs, glIsBuffer returns
**  GL_FALSE.
**
**  A name returned by glGenBuffers, but not yet associated with a buffer
**  object by calling glBindBuffer, is not the name of a buffer object.
**
**  INPUT:
**
**      Buffer
**          Specifies a value that may be the name of a Buffer object.
**
**  OUTPUT:
**
**      GL_TRUE or GL_FALSE (see above description.)
*/

GL_API GLboolean GL_APIENTRY glIsBuffer(
    GLuint Buffer
    )
{
    GLboolean result = GL_FALSE;

    glmENTER1(glmARGUINT, Buffer)
    {
        glsNAMEDOBJECT_PTR wrapper;
        glsCONTEXT_PTR shared;

		glmPROFILE(context, GLES1_ISBUFFER, 0);

        /* Map shared context. */
#if gcdRENDER_THREADS
        shared = (context->shared != gcvNULL) ? context->shared : context;
#else
        shared = context;
#endif

        /* Find the object. */
        wrapper = glfFindNamedObject(&shared->bufferList, Buffer);

        /* Must exist. */
        if (wrapper)
        {
            glsBUFFER_PTR object = (glsBUFFER_PTR) wrapper->object;

            /* Must also be bound at least once. */
            if (object->boundAtLeastOnce)
            {
                result = GL_TRUE;
            }
        }

		gcmDUMP_API("${ES11 glIsBuffer 0x%08X := 0x%08X}", Buffer, result);
    }
    glmLEAVE();

    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  glBindBuffer
**
**  glBindBuffer lets you create or use a named buffer object. Calling
**  glBindBuffer with target set to GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
**  and buffer set to the name of the new buffer object binds the buffer object
**  name to the target.
**
**  When a buffer object is bound to a target, the previous binding for that
**  target is automatically broken.
**
**  Buffer object names are unsigned integers. The value zero is reserved, but
**  there is no default buffer object for each buffer object target. Instead,
**  buffer set to zero effectively unbinds any buffer object previously bound,
**  and restores client memory usage for that buffer object target.
**
**  Buffer object names and the corresponding buffer object contents are local
**  to the shared display-list space (see glXCreateContext) of the current
**  GL rendering context; two rendering contexts share buffer object names only
**  if they also share display lists.
**
**  You may use glGenBuffers to generate a set of new buffer object names.
**
**  The state of a buffer object immediately after it is first bound is an
**  unmapped zero-sized memory buffer with READ_WRITE access and STATIC_DRAW
**  usage.
**
**  While a non-zero buffer object name is bound, GL operations on the target to
**  which it is bound affect the bound buffer object, and queries of the target
**  to which it is bound return state from the bound buffer object. While buffer
**  object name zero is bound, as in the initial state, attempts to modify or
**  query state on the target to which it is bound generates an
**  INVALID_OPERATION error.
**
**  When vertex array pointer state is changed, for example by a call to
**  glNormalPointer, the current buffer object binding (GL_ARRAY_BUFFER_BINDING)
**  is copied into the corresponding client state for the vertex array type
**  being changed, for example GL_NORMAL_ARRAY_BUFFER_BINDING.
**
**  While a non-zero buffer object is bound to the GL_ARRAY_BUFFER target, the
**  vertex array pointer parameter that is traditionally interpreted as
**  a pointer to client-side memory is instead interpreted as an offset within
**  the buffer object measured in basic machine units.
**
**  While a non-zero buffer object is bound to the GL_ARRAY_ELEMENT_BUFFER
**  target, the indices parameter of glDrawElements, glDrawRangeElements, or
**  glMultiDrawElements that is traditionally interpreted as a pointer to
**  client-side memory is instead interpreted as an offset within the
**  buffer object measured in basic machine units.
**
**  A buffer object binding created with glBindBuffer remains active until
**  a different buffer object name is bound to the same target, or until the
**  bound buffer object is deleted with glDeleteBuffers.
**
**  Once created, a named buffer object may be re-bound to any target as often
**  as needed. However, the GL implementation may make choices about how to
**  optimize the storage of a buffer object based on its initial binding target.
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

GL_API void GL_APIENTRY glBindBuffer(
    GLenum Target,
    GLuint Buffer
    )
{
    glmENTER2(glmARGENUM, Target, glmARGUINT, Buffer)
    {
        gceSTATUS status;
        glsNAMEDOBJECT_PTR* target;
        gleBUFFERBINDINGS bindingType;
        glsNAMEDOBJECT_PTR wrapper;
        glsCONTEXT_PTR shared;

        gcmDUMP_API("${ES11 glBindBuffer 0x%08X 0x%08X}", Target, Buffer);

        glmPROFILE(context, GLES1_BINDBUFFER, 0);

        /* Map shared context. */
#if gcdRENDER_THREADS
        shared = (context->shared != gcvNULL) ? context->shared : context;
#else
        shared = context;
#endif

        /* Translate the target. */
        if (Target == GL_ARRAY_BUFFER)
        {
            target = &context->arrayBuffer;
            bindingType = glvARRAYBUFFER;
        }
        else if (Target == GL_ELEMENT_ARRAY_BUFFER)
        {
            target = &context->elementArrayBuffer;
            bindingType = glvELEMENTBUFFER;
        }
        else
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Find the object. */
        wrapper = glfFindNamedObject(&shared->bufferList, Buffer);

        /* Create if does not exist yet. */
        if ((wrapper == gcvNULL) && Buffer)
        {
            /* Create a new wrapper. */
            if (gcmIS_ERROR(_CreateBuffer(context, Buffer, &wrapper)))
            {
                gcmFATAL("glBindBuffer failed to generate a named object.");
                break;
            }
        }

        /* Unbind the current buffer if different. */
        if (*target && (*target != wrapper))
        {
            /* Cast the object. */
            glsBUFFER_PTR object = (glsBUFFER_PTR) (*target)->object;

            /* Reset the bound target. */
            object->bound = gcvFALSE;

            /* Reset binding to the target. */
            object->binding[bindingType] = gcvNULL;
            *target = gcvNULL;
        }

        /* Bind the new buffer. */
        if (wrapper != gcvNULL)
        {
            /* Cast the object. */
            glsBUFFER_PTR object = (glsBUFFER_PTR) wrapper->object;

            object->bound = gcvTRUE;
            object->binding[bindingType] = target;
            *target = wrapper;

            object->boundAtLeastOnce = gcvTRUE;

            /* copy array to element or vice versa if needed*/
            status = _DuplicateBufferData(context, object, GL_ELEMENT_ARRAY_BUFFER==Target);
            if (gcmIS_ERROR(status))
            {
                gcmFATAL("%s(%d): _DuplicateBufferData returned %d(%s)",
                         __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
                glmERROR(GL_OUT_OF_MEMORY);
                break;
            }
        }
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glBufferData
**
**  glBufferData creates a new data store for the buffer object currently bound
**  to target. Any pre-existing data store is deleted. The new data store is
**  created with the specified size in bytes and usage. If data is not gcvNULL,
**  the data store is initialized with data from this pointer. In its initial
**  state, the new data store is not mapped, it has a gcvNULL mapped pointer,
**  and its mapped access is GL_READ_WRITE.
**
**  'Usage' is a hint to the GL implementation as to how a buffer object's data
**  store will be accessed. This enables the GL implementation to make more
**  intelligent decisions that may significantly impact buffer object
**  performance. It does not, however, constrain the actual usage of the data
**  store. 'Usage' can be broken down into two parts: first, the frequency of
**  access (modification and usage), and second, the nature of that access.
**
**  INPUT:
**
**      Target
**          Specifies the target buffer object. The symbolic constant must be
**          GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER.
**
**      Size
**          Specifies the size in bytes of the buffer object's new data store.
**
**      Data
**          Specifies a pointer to data that will be copied into the data store
**          for initialization, or gcvNULL if no data is to be copied.
**
**      Usage
**          Specifies the expected application usage pattern of the data store.
**          Accepted values are GL_STATIC_DRAW and GL_DYNAMIC_DRAW.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glBufferData(
    GLenum Target,
    GLsizeiptr Size,
    const GLvoid* Data,
    GLenum Usage
    )
{
    glmENTER4(glmARGENUM, Target, glmARGINT, Size,
              glmARGPTR, Data, glmARGENUM, Usage)
    {
        glsNAMEDOBJECT_PTR wrapper;
        gceSTATUS status = gcvSTATUS_OK;

        gcmDUMP_API("${ES11 glBufferData 0x%08X 0x%08X (0x%08X) 0x%08X",
            Target, Size, Data, Usage);
        gcmDUMP_API_DATA(Data, Size);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_BUFFERDATA, 0);

        /* Translate the target. */
        if (Target == GL_ARRAY_BUFFER)
        {
            wrapper = context->arrayBuffer;
        }
        else if (Target == GL_ELEMENT_ARRAY_BUFFER)
        {
            wrapper = context->elementArrayBuffer;
        }
        else
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Validate usage. */
        if ((Usage != GL_STATIC_DRAW) && (Usage != GL_DYNAMIC_DRAW))
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Validate the size. */
        if (Size < 0)
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        /* Don't do anything if nothing is bound. */
        if (wrapper == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        do
        {
            /* Cast the object. */
            glsBUFFER_PTR object = (glsBUFFER_PTR) wrapper->object;
            gcmASSERT(object != gcvNULL);

            /* Set new parameters. */
            object->size  = Size;
            object->usage = Usage;

#if gcdUSE_TRIANGLE_STRIP_PATCH
            object->stripPatchDirty = gcvTRUE;
#endif
            /* clear map state */
            _ClearMapping(object);

            /* Don't allocate buffer objects if the buffer size is 0. */
            if (Size == 0)
            {
                break;
            }

            /* If currently or ever bound as array buffer */
            if (GL_ARRAY_BUFFER == Target || object->stream)
            {
                /* Destroy the old stream. */
                if (object->stream)
                {
                    gcmERR_BREAK(gcoSTREAM_Destroy(object->stream));
                    object->stream = gcvNULL;
                }

                /* Allocate the stream object. */
                gcmERR_BREAK(gcoSTREAM_Construct(
                    context->hal,
                    &object->stream
                    ));

                /* Allocate the buffer. */
                gcmERR_BREAK(gcoSTREAM_Reserve(
                    object->stream,
                    Size
                    ));

                /* Is the data provided? */
                if (Data != gcvNULL)
                {
                    gcmERR_BREAK(gcoSTREAM_Upload(
                        object->stream,
                        Data,
                        0,
                        Size,
                        Usage == GL_DYNAMIC_DRAW
                        ));
                }
            }

            /* If currently or ever bound as element array buffer */
            if (GL_ELEMENT_ARRAY_BUFFER == Target || object->index)
            {
                /* Destroy index buffer. */
                if (object->index)
                {
                    gcmERR_BREAK(gcoINDEX_Destroy(object->index));
                    object->index = gcvNULL;
                }

                /* Allocate the index object. */
                gcmERR_BREAK(gcoINDEX_Construct(
                    context->hal,
                    &object->index
                    ));

                /* Allocate the buffer. */
                gcmERR_BREAK(gcoINDEX_Upload(
                    object->index,
                    Data,
                    Size
                    ));
            }
        }
        while (gcvFALSE);

        /* Verify the result. */
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
**  glBufferSubData
**
**  glBufferSubData redefines some or all of the data store for the buffer
**  object currently bound to target. Data starting at byte offset offset and
**  extending for size bytes is copied to the data store from the memory pointed
**  to by data.  An error is thrown if offset and size together define a range
**  beyond the bounds of the buffer object's data store.
**
**  INPUT:
**
**      Target
**          Specifies the target buffer object. The symbolic constant must be
**          GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER.
**
**      Offset
**          Specifies the offset into the buffer object's data store where data
**          replacement will begin, measured in bytes.
**
**      Size
**          Specifies the size in bytes of the data store region being replaced.
**
**      Data
**          Specifies a pointer to the new data that will be copied into
**          the data store.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glBufferSubData(
    GLenum Target,
    GLintptr Offset,
    GLsizeiptr Size,
    const GLvoid* Data
    )
{
    glmENTER4(glmARGENUM, Target, glmARGUINT, Offset,
              glmARGINT, Size, glmARGPTR, Data)
    {
        glsNAMEDOBJECT_PTR wrapper;
        glsBUFFER_PTR object;
        gceSTATUS status = gcvSTATUS_OK;

        gcmDUMP_API("${ES11 glBufferSubData 0x%08X 0x%08X 0x%08X (0x%08x)",
            Target, Offset, Size, Data);
        gcmDUMP_API_DATA(Data, Size);
        gcmDUMP_API("$}");

        glmPROFILE(context, GLES1_BUFFERSUBDATA, 0);

        /* Translate the target. */
        if (Target == GL_ARRAY_BUFFER)
        {
            wrapper = context->arrayBuffer;
        }
        else if (Target == GL_ELEMENT_ARRAY_BUFFER)
        {
            wrapper = context->elementArrayBuffer;
        }
        else
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Don't do anything if nothing is bound. */
        if (wrapper == gcvNULL)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Cast the object. */
        object = (glsBUFFER_PTR) wrapper->object;
        gcmASSERT(object != gcvNULL);

        /* Validate input data buffer. */
        if ((Offset < 0) || (Size < 0) ||
            (Offset + Size > object->size))
        {
            glmERROR(GL_INVALID_VALUE);
            break;
        }

        if(object->mapped)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Don't do anything if no data is provided. */
        if ((Data == gcvNULL) || (Size == 0))
        {
            break;
        }

        if (object->stream)
        {
            /* Upload data. */
            gcmERR_BREAK(gcoSTREAM_Upload(
                object->stream,
                Data,
                Offset,
                Size,
                object->usage == GL_DYNAMIC_DRAW
                ));
        }

        if (object->index)
        {
            /* If upload whole index, we create a new one. */
            if (Offset == 0 && Size == object->size)
            {
                gcmVERIFY_OK(gcoINDEX_Destroy(object->index));
                object->index = gcvNULL;

                /* Allocate the index object. */
                gcmVERIFY_OK(gcoINDEX_Construct(
                    context->hal,
                    &object->index
                    ));

                if (object->index == gcvNULL)
                {
                    break;
                }

                gcmVERIFY_OK(gcoINDEX_Upload(
                    object->index,
                    Data,
                    Size
                    ));
            }
            else
            {
                /* Stall if only upload a sub-range. */
                if (context->bSyncSubVBO)
                {
                    gcmERR_BREAK(gcoHAL_Commit(gcvNULL, gcvTRUE));
                }

                /* Upload data. */
                gcmVERIFY_OK(gcoINDEX_UploadOffset(
                    object->index,
                    Offset,
                    Data,
                    Size
                    ));
            }
        }
#if gcdUSE_TRIANGLE_STRIP_PATCH
        object->stripPatchDirty = gcvTRUE;
#endif
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glGetBufferParameteriv
**
**  glGetBufferParameteriv returns in data a selected parameter of the buffer
**  object specified by target.
**
**  INPUT:
**
**      Target
**          Specifies the target buffer object. The symbolic constant must be
**          GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER.
**
**      Value
**          Specifies the symbolic name of a buffer object parameter. Accepted
**          values are GL_BUFFER_ACCESS, GL_BUFFER_MAPPED, GL_BUFFER_SIZE,
**          or GL_BUFFER_USAGE.
**
**      Data
**          Returns the requested parameter.
**
**  OUTPUT:
**
**      See description.
*/

GL_API void GL_APIENTRY
glGetBufferParameteriv(
    GLenum Target,
    GLenum Value,
    GLint* Data
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Value, glmARGPTR, Data)
    {
        glsBUFFER_PTR object;

		glmPROFILE(context, GLES1_GETBUFFERPARAMETERIV, 0);

        /* Translate the target. */
        if (Target == GL_ARRAY_BUFFER)
        {
            /* Don't do anything if nothing is bound. */
            if (context->arrayBuffer == gcvNULL)
            {
                break;
            }

            /* Cast the object. */
            object = (glsBUFFER_PTR) context->arrayBuffer->object;
        }
        else if (Target == GL_ELEMENT_ARRAY_BUFFER)
        {
            /* Don't do anything if nothing is bound. */
            if (context->elementArrayBuffer == gcvNULL)
            {
                break;
            }

            /* Cast the object. */
            object = (glsBUFFER_PTR) context->elementArrayBuffer->object;
        }
        else
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Dispatch. */
        switch (Value)
        {
        case GL_BUFFER_SIZE:
            *Data = object->size;
            break;

        case GL_BUFFER_USAGE:
            *Data = object->usage;
            break;

        case GL_BUFFER_ACCESS_OES:
            *Data = GL_WRITE_ONLY_OES;
            break;

        case GL_BUFFER_MAPPED_OES:
            *Data = object->mapped;
            break;

        default:
            glmERROR(GL_INVALID_ENUM);
            break;
        }

		gcmDUMP_API("${ES11 glGetBufferParameteriv 0x%08X 0x%08x (0x%08X)",
                    Target, Value, Data);
		gcmDUMP_API_DATA(Data, 1);
		gcmDUMP_API("$}");
	}
    glmLEAVE();
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

GL_API void* GL_APIENTRY
glMapBufferOES (
    GLenum Target,
    GLenum Access
    )
{
    glmENTER2(glmARGENUM, Target, glmARGENUM, Access)
    {
        glsBUFFER_PTR object;
        gceSTATUS status = gcvSTATUS_OK;

        gcmDUMP_API("${ES11 glMapBufferOES 0x%08X 0x%08X}", Target, Access);

        glmPROFILE(context, GLES1_GLMAPBUFFEROES, 0);

        /* Check the access value. */
        if (Access != GL_WRITE_ONLY_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Translate the target. */
        if (Target == GL_ARRAY_BUFFER)
        {
            /* Don't do anything if nothing is bound. */
            if (context->arrayBuffer == gcvNULL)
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Cast the object. */
            object = (glsBUFFER_PTR) context->arrayBuffer->object;
        }
        else if (Target == GL_ELEMENT_ARRAY_BUFFER)
        {
            /* Don't do anything if nothing is bound. */
            if (context->elementArrayBuffer == gcvNULL)
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Cast the object. */
            object = (glsBUFFER_PTR) context->elementArrayBuffer->object;
        }
        else
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Check buffer if already mapped. */
        if (object->mapped)
        {
            glmERROR(GL_INVALID_OPERATION);
            break;
        }

        /* Check buffer if created with the size bigger than zero. */
        if (object->size == 0)
        {
            glmERROR(GL_OUT_OF_MEMORY);
            break;
        }

        if(Target == GL_ARRAY_BUFFER)
        {
            /* Set mapping status and return buffer client pointer. */
            gcmERR_BREAK(gcoSTREAM_CPUCacheOperation(object->stream,
                gcvCACHE_INVALIDATE));

            gcmERR_BREAK(gcoSTREAM_Lock(object->stream,
                &object->bufferMapPointer,
                gcvNULL));
        }
        if(Target == GL_ELEMENT_ARRAY_BUFFER)
        {
            gcmERR_BREAK(gcoINDEX_Lock(object->index,
                gcvNULL,
                &object->bufferMapPointer));
        }

        object->mapped = gcvTRUE;
        return object->bufferMapPointer;
    }
    glmLEAVE();
    return gcvNULL;
}

GL_API GLboolean GL_APIENTRY
glUnmapBufferOES (
    GLenum Target
    )
{
    glmENTER1(glmARGENUM, Target)
    {
        glsBUFFER_PTR object;
        gceSTATUS status = gcvSTATUS_OK;

        gcmDUMP_API("${ES11 glUnmapBufferOES 0x%08X}", Target);

        glmPROFILE(context, GLES1_GLUNMAPBUFFEROES, 0);

        /* Translate the target. */
        if (Target == GL_ARRAY_BUFFER)
        {
            /* Don't do anything if nothing is bound. */
            if (context->arrayBuffer == gcvNULL)
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Cast the object. */
            object = (glsBUFFER_PTR) context->arrayBuffer->object;
        }
        else if (Target == GL_ELEMENT_ARRAY_BUFFER)
        {
            /* Don't do anything if nothing is bound. */
            if (context->elementArrayBuffer == gcvNULL)
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Cast the object. */
            object = (glsBUFFER_PTR) context->elementArrayBuffer->object;
        }
        else
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Check buffer if already mapped. */
        if (object->mapped)
        {
            if(Target == GL_ARRAY_BUFFER)
            {
                gcmERR_BREAK(gcoSTREAM_CPUCacheOperation(object->stream,
                    gcvCACHE_FLUSH));
                gcmERR_BREAK(gcoSTREAM_Unlock(object->stream));
                /* Array was updated, duplicate it to element */
                _DuplicateBufferData(context, object, gcvTRUE);
            }
            if(Target == GL_ELEMENT_ARRAY_BUFFER)
            {
                gcmERR_BREAK(gcoINDEX_Unlock(object->index));
                /* Element was updated, duplicate it to array */
                _DuplicateBufferData(context, object, gcvFALSE);
            }

            _ClearMapping(object);
            return gcvTRUE;
        }

        /* Generate a invalid operation if buffer is unmapped status. */
        glmERROR(GL_INVALID_OPERATION);
    }
    glmLEAVE();
    return gcvFALSE;
}

GL_API void GL_APIENTRY
glGetBufferPointervOES (
    GLenum Target,
    GLenum Value,
    GLvoid ** Params
    )
{
    glmENTER3(glmARGENUM, Target, glmARGENUM, Value, glmARGPTR, *Params)
    {
        glsBUFFER_PTR object;

        glmPROFILE(context, GLES1_GLGETBUFFERPOINTERVOES, 0);

        if (Value != GL_BUFFER_MAP_POINTER_OES)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (Params == gcvNULL)
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        /* Translate the target. */
        if (Target == GL_ARRAY_BUFFER)
        {
            /* Don't do anything if nothing is bound. */
            if (context->arrayBuffer == gcvNULL)
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Cast the object. */
            object = (glsBUFFER_PTR) context->arrayBuffer->object;
        }
        else if (Target == GL_ELEMENT_ARRAY_BUFFER)
        {
            /* Don't do anything if nothing is bound. */
            if (context->elementArrayBuffer == gcvNULL)
            {
                glmERROR(GL_INVALID_OPERATION);
                break;
            }

            /* Cast the object. */
            object = (glsBUFFER_PTR) context->elementArrayBuffer->object;
        }
        else
        {
            glmERROR(GL_INVALID_ENUM);
            break;
        }

        if (object->mapped)
        {
            *Params = object->bufferMapPointer;
        }
        else
        {
            *Params = (GLvoid*)gcvNULL;
        }


		gcmDUMP_API("${ES11 glGetBufferPointervOES 0x%08X 0x%08x (0x%08X)",
                    Target, Value, *Params);
		gcmDUMP_API_DATA(*Params, sizeof(GLvoid*));
		gcmDUMP_API("$}");
	}
    glmLEAVE();
}



GL_API void GL_APIENTRY glGenBuffersARB(
   GLsizei Count,
   GLuint* Buffers
   )
{
    glGenBuffers(Count,Buffers);
}

GL_API void GL_APIENTRY glDeleteBuffersARB(
    GLsizei Count,
    const GLuint* Buffers
    )
{
    glDeleteBuffers(Count,Buffers);
}

GL_API void GL_APIENTRY glBindBufferARB(
    GLenum Target,
    GLuint Buffer
    )
{
    glBindBuffer(Target,Buffer);
}

GL_API void GL_APIENTRY glBufferDataARB(
    GLenum Target,
    GLsizeiptr Size,
    const GLvoid* Data,
    GLenum Usage
    )
{
    glBufferData(Target, Size,Data, Usage);
}

GL_API void GL_APIENTRY glBufferSubDataARB(
    GLenum Target,
    GLintptr Offset,
    GLsizeiptr Size,
    const GLvoid* Data
    )
{
    glBufferSubData(Target,Offset,Size,Data);
}

GL_API void GL_APIENTRY glGetBufferParameterivARB(
    GLenum Target,
    GLenum Value,
    GLint* Data
    )
{
    glGetBufferParameteriv(Target,Value,Data);
}

