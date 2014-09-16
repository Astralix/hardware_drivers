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

void
_glshInitializeShader(
    GLContext Context
    )
{
    /* No active program. */
    Context->program      = gcvNULL;
    Context->programDirty = GL_FALSE;

    /* Zero objects. */
    gcmVERIFY_OK(gcoOS_ZeroMemory(&Context->shaderObjects,
                                  gcmSIZEOF(Context->shaderObjects)));
}

#if gcdNULL_DRIVER < 3
static GLsizei
_gcType2Bytes(
    gcSHADER_TYPE Type
    )
{
#if !GC_ENABLE_LOADTIME_OPT
    switch (Type)
    {
    case gcSHADER_FLOAT_X1:
        return 1 * sizeof(GLfloat);

    case gcSHADER_FLOAT_X2:
        return 2 * sizeof(GLfloat);

    case gcSHADER_FLOAT_X3:
        return 3 * sizeof(GLfloat);

    case gcSHADER_FLOAT_X4:
        return 4 * sizeof(GLfloat);

    case gcSHADER_FLOAT_2X2:
        return 2 * 2 * sizeof(GLfloat);

    case gcSHADER_FLOAT_3X3:
        return 3 * 3 * sizeof(GLfloat);

    case gcSHADER_FLOAT_4X4:
        return 4 * 4 * sizeof(GLfloat);

    case gcSHADER_BOOLEAN_X1:
    case gcSHADER_INTEGER_X1:
    case gcSHADER_SAMPLER_1D:
    case gcSHADER_SAMPLER_2D:
    case gcSHADER_SAMPLER_3D:
    case gcSHADER_SAMPLER_CUBIC:
        return 1 * sizeof(GLfloat);

    case gcSHADER_BOOLEAN_X2:
    case gcSHADER_INTEGER_X2:
        return 2 * sizeof(GLfloat);

    case gcSHADER_BOOLEAN_X3:
    case gcSHADER_INTEGER_X3:
        return 3 * sizeof(GLfloat);

    case gcSHADER_BOOLEAN_X4:
    case gcSHADER_INTEGER_X4:
        return 4 * sizeof(GLfloat);

    default:
        gcmFATAL("Unsupported type: %d", Type);
        return 0;
    }
#else
    gcmASSERT(Type < gcSHADER_TYPE_COUNT);
    return shader_type_info[Type].rows * shader_type_info[Type].components * sizeof(GLfloat);
#endif /* GC_ENABLE_LOADTIME_OPT */
}
#endif

/******************************************************************************\
|*                                                                    SHADERS *|
\******************************************************************************/

void
_glshDeleteShader(
    GLContext Context,
    GLShader Shader
    )
{
    /* Free any source data. */
    if (Shader->source != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->source));
    }

    /* Free any compile log. */
    if (Shader->compileLog != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->compileLog));
    }

    if (Shader->usageCount > 0)
    {
        /* Shader is in use - flag it for deletion. */
        gcmTRACE(gcvLEVEL_INFO,
                 "glDeleteShader: shader=%u flagged for deletion (usage %d)",
                 Shader->object.name,
                 Shader->usageCount);

        Shader->flagged = gcvTRUE;

        return;
    }

    /* Remove the shader from the object list. */
    _glshRemoveObject(&Context->shaderObjects, &Shader->object);

    /* Destroy any binary data. */
    if (Shader->binary != gcvNULL)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(Shader->binary));
    }

    /* Free the shader. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader));
}

GL_APICALL GLuint GL_APIENTRY
glCreateShader(
    GLenum type
    )
{
    GLContext context;
    GLObjectType objectType;
    GLShader shader;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("type=0x%04x", type);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_ARG("%d", 0);
        return 0;
    }

    glmPROFILE(context, GLES2_CREATESHADER, 0);

    switch (type)
    {
    case GL_VERTEX_SHADER:
        gcmTRACE(gcvLEVEL_VERBOSE, "glCreateShader: type=VERTEX_SHADER");
        objectType = GLObject_VertexShader;
        break;

    case GL_FRAGMENT_SHADER:
        gcmTRACE(gcvLEVEL_VERBOSE, "glCreateShader: type=FRAGMENT_SHADER");
        objectType = GLObject_FragmentShader;
        break;

    default:
        gcmTRACE(gcvLEVEL_ERROR, "glCreateShader: unknown type=0x%04X", type);
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_ARG("%d", 0);
        return 0;
    }

    /* Allocate GLShader structure. */
    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   gcmSIZEOF(struct _GLShader),
                                   &pointer)))
    {
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_ARG("%d", 0);
        return 0;
    }

    shader = pointer;

    /* Insert GLObject to hash tree. */
    if (!_glshInsertObject(&context->shaderObjects,
                           &shader->object,
                           objectType,
                           0))
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, shader));

        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_ARG("%d", 0);
        return 0;
    }

    /* Initialize GLShader structure. */
    shader->sourceSize    = 0;
    shader->source        = gcvNULL;
    shader->compileStatus = GL_FALSE;
    shader->compileLog    = gcvNULL;
    shader->binary        = gcvNULL;
    shader->usageCount    = 0;
    shader->flagged       = GL_FALSE;
    shader->dirty         = GL_FALSE;

    gcmTRACE(gcvLEVEL_VERBOSE, "glCreateShader ==> %u", shader->object.name);

    /* Return shader name. */
    gcmDUMP_API("${ES20 glCreateShader 0x%08X := 0x%08X}",
                type, shader->object.name);
    gcmFOOTER_ARG("%u", shader->object.name);
    return shader->object.name;
}

GL_APICALL void GL_APIENTRY
glDeleteShader(
    GLuint shader
    )
{
    GLContext context;
    GLShader object;

    gcmHEADER_ARG("shader=%u", shader);
    gcmDUMP_API("${ES20 glDeleteShader 0x%08X}", shader);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_DELETESHADER, 0);

    if (shader == 0)
    {
        gcmFOOTER_NO();
        return;
    }

    /* Find the object. */
    object = (GLShader) _glshFindObject(&context->shaderObjects, shader);

    if (object == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glDeleteShader: shader=%u is not a valid object",
                 shader);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure it is a shader object. */
    if ( (object->object.type != GLObject_VertexShader)
    &&   (object->object.type != GLObject_FragmentShader)
    )
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glDeleteShader: shader=%u is not a shader object",
                 shader);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Delete the shader. */
    _glshDeleteShader(context, object);

    gcmFOOTER_NO();
}

GL_APICALL GLboolean GL_APIENTRY
glIsShader(
    GLuint shader
    )
{
    GLContext context;
    GLObject object;
    GLboolean isShader;

    gcmHEADER_ARG("shader=%u", shader);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_ARG("return=%d", GL_FALSE);
        return GL_FALSE;
    }

    glmPROFILE(context, GLES2_ISSHADER, 0);

    /* Find the object. */
    object = _glshFindObject(&context->shaderObjects, shader);

    /* Object is a shader if the object is valid and is either of type
       VertexShader or FragmentShader. */
    isShader = (object != gcvNULL) &&
               ( (object->type == GLObject_VertexShader) ||
                 (object->type == GLObject_FragmentShader) );

    gcmTRACE(gcvLEVEL_VERBOSE, "glIsShader ==> %d", isShader);

    gcmDUMP_API("${ES20 glIsShader 0x%08X := 0x%08X}", shader, isShader);
    gcmFOOTER_ARG("%d", isShader);
    return isShader;
}

GL_APICALL void GL_APIENTRY
glShaderSource(
    GLuint shader,
    GLsizei count,
    const char ** string,
    const GLint * length
    )
{
    GLContext context;
    GLShader object;
    GLint i;
    char * p;
    gctSIZE_T size;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER_ARG("shader=%u count=%d string=0x%x, length=0x%x",
                  shader, count, string, length);
#if gcdDUMP_API
    gcmDUMP_API("${ES20 glShaderSource 0x%08X 0x%08X (0x%08X) (0x%08X)",
                shader, count, string, length);
    gcmDUMP_API_ARRAY(string, count);
    gcmDUMP_API_ARRAY(length, count);
    for (i = 0; i < count; ++i)
    {
        gctSIZE_T n;
        if ((length != gcvNULL) && (length[i] >= 0))
        {
            n = length[i];
        }
        else
        {
            n = 0;
        }
        gcmDUMP_API_DATA(string[i], n);
    }
    gcmDUMP_API("$}");
#endif

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_SHADERSOURCE, 0);

    /* Test for valid count. */
    if (count == 0)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glShaderSource: count can not be 0");

        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Test for valid string. */
    if (string == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glShaderSource: string cannot be NULL");

        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Find the object. */
    object = (GLShader) _glshFindObject(&context->shaderObjects, shader);

    if (object == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glShaderSource: shader=%u is not a valid object",
                 shader);

        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if ((object->object.type != GLObject_VertexShader)
    &&  (object->object.type != GLObject_FragmentShader)
    )
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glShaderSource: shader=%u is not a shader object",
                 shader);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Free any current source code. */
    if (object->source != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, object->source));
    }

    /* Determine the length of the source code. */
    if (object->binary != gcvNULL)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(object->binary));
        object->binary = gcvNULL;
    }

    /* Determine the length of the source code. */
    object->sourceSize = 0;

    for (i = 0; i < count; ++i)
    {
        if ( (length == gcvNULL) || (length[i] < 0) )
        {
            gctSIZE_T l;

            if (string[i] == gcvNULL)
            {
                continue;
            }

            gcmVERIFY_OK(gcoOS_StrLen(string[i], &l));

            object->sourceSize += l;
        }
        else
        {
            object->sourceSize += length[i];
        }
    }

    /* Allocate the string buffer. */
    if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                   object->sourceSize + 1,
                                   &pointer)))
    {
        gcmFATAL("%s(%d): Out of memory", __FUNCTION__, __LINE__);
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_NO();
        return;
    }

    object->source = pointer;

    /* Copy the entire source code. */
    p = object->source;

    for (i = 0; i < count; ++i)
    {
        if(string[i] == gcvNULL)
        {
            continue;
        }

        if ( (length == gcvNULL) || (length[i] < 0) )
        {
            gcmVERIFY_OK(gcoOS_StrLen(string[i], &size));
        }
        else
        {
            size = length[i];
        }

        if (size > 0)
        {
            gcmVERIFY_OK(gcoOS_MemCopy(p, string[i], size));
            p += size;
        }
    }

    /* Terminate source. */
    *p = '\0';

    gcmFOOTER_NO();
}

static void
glshCompileShader(
    GLContext Context,
    GLShader Shader
    )
{
    gctMD5_Byte blobCacheKey[BLOB_CACHE_KEY_SIZE];

    gcmHEADER_ARG("Context=0x%x Shader=0x%x", Context, Shader);

    /* Free any existing log. */
    if (Shader->compileLog != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Shader->compileLog));
    }

    /* Destroy any existing binary output. */
    if (Shader->binary != gcvNULL)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(Shader->binary));
        Shader->binary = gcvNULL;
    }

    /*
      Try to get the precompiled binary from the cache.
      If the shader is not cached compile and cache it
    */
    if (_glshBlobCacheGetShader(Context, Shader, blobCacheKey) != gcvSTATUS_OK)
    {
        /* Compile the shader. */
        Shader->compileStatus = _glshCompileShader(Context,
                                                   Shader->object.type,
                                                   Shader->sourceSize,
                                                   Shader->source,
                                                   &Shader->binary,
                                                   &Shader->compileLog);


        /* Cache the shader */
        if (Shader->compileStatus == GL_TRUE)
        {
            _glshBlobCacheSetShader(Shader, blobCacheKey);
        }
    }

    Shader->dirty = GL_TRUE;

    gcmFOOTER_NO();
    return;
}

GL_APICALL void GL_APIENTRY
glCompileShader(
    GLuint shader
    )
{
    GLShader shaderObject;

	glmENTER1(glmARGUINT, shader)
	{
    gcmDUMP_API("${ES20 glCompileShader 0x%08X}", shader);

    glmPROFILE(context, GLES2_COMPILESHADER, 0);

    /* Find the object. */
    shaderObject = (GLShader) _glshFindObject(&context->shaderObjects, shader);

    /* Make sure the object is valid. */
    if (shaderObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glCompileShader: shader=%u is not a valid object",
                 shader);
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Make sure the object is a shader. */
    if ((shaderObject->object.type != GLObject_VertexShader)
    &&  (shaderObject->object.type != GLObject_FragmentShader)
    )
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glCompileShader: shader=%u is not a shader object",
                 shader);
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    glshCompileShader(context, shaderObject);

	}
	glmLEAVE();
}

GL_APICALL void GL_APIENTRY
glGetShaderiv(
    GLuint shader,
    GLenum pname,
    GLint *params
    )
{
    GLContext context;
    GLShader shaderObject;

    gcmHEADER_ARG("shader=%u pname=0x%04x", shader, pname);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETSHADERIV, 0);

    /* Find the object. */
    shaderObject = (GLShader) _glshFindObject(&context->shaderObjects, shader);

    /* Make sure the object is valid. */
    if (shaderObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
            "%s(%d): shader=%u is not a valid object", __FUNCTION__, __LINE__, shader);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a shader. */
    if ( (shaderObject->object.type != GLObject_VertexShader) &&
         (shaderObject->object.type != GLObject_FragmentShader) )
    {
        gcmTRACE(gcvLEVEL_WARNING,
            "%s(%d): shader=%u is not a shader object", __FUNCTION__, __LINE__, shader);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Dispatch on pname. */
    switch (pname)
    {
    case GL_SHADER_TYPE:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetShaderiv: shader=%u pname=SHADER_TYPE",
                 shader);

        if (shaderObject->object.type == GLObject_VertexShader)
        {
            *params = GL_VERTEX_SHADER;
        }
        else
        {
            *params = GL_FRAGMENT_SHADER;
        }
        break;

    case GL_DELETE_STATUS:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetShaderiv: shader=%u pname=DELETE_STATUS",
                 shader);

        *params = shaderObject->flagged;
        break;

    case GL_COMPILE_STATUS:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetShaderiv: shader=%u pname=COMPILE_STATUS",
                 shader);

        *params = shaderObject->compileStatus;
        break;

    case GL_INFO_LOG_LENGTH:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetShaderiv: shader=%u pname=INFO_LOG_LENGTH",
                 shader);

        if (shaderObject->compileLog == gcvNULL)
        {
            *params = 0;
        }
        else
        {
            gcmVERIFY_OK(gcoOS_StrLen(shaderObject->compileLog,
                                      (gctSIZE_T *) params));
        }
        *params += 1;
        break;

    case GL_SHADER_SOURCE_LENGTH:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetShaderiv: shader=%u pname=SHADER_SOURCE_LENGTH",
                 shader);

        *params = shaderObject->sourceSize;

        if(*params)
        {
            (*params) ++;
        }
        break;

    default:
        gcmFATAL("%s(%d): unknown pname=0x%04X", __FUNCTION__, __LINE__, pname);
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    gcmDUMP_API("${ES20 glGetShaderiv 0x%08X 0x%08X (0x%08X)",
                shader, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*params=%d", *params);
}

GL_APICALL void GL_APIENTRY
glGetShaderInfoLog(
    GLuint shader,
    GLsizei bufsize,
    GLsizei * length,
    char * infolog
    )
{
    GLContext context;
    GLShader shaderObject;
    gctSIZE_T size;
	gceSTATUS status;

    gcmHEADER_ARG("shader=%u bufsize=%d", shader, bufsize);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETSHADERINFOLOG, 0);

    /* Find the object. */
    shaderObject = (GLShader) _glshFindObject(&context->shaderObjects, shader);

    /* Make sure the object is valid. */
    if (shaderObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetShaderiv: shader=%u is not a valid object",
                 shader);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a shader. */
    if ( (shaderObject->object.type != GLObject_VertexShader) &&
         (shaderObject->object.type != GLObject_FragmentShader) )
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetShaderiv: shader=%u is not a shader object",
                 shader);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (shaderObject->compileLog == gcvNULL)
    {
        size = 0;
    }
    else
    {
        gcmVERIFY_OK(gcoOS_StrLen(shaderObject->compileLog, &size));
        if (size > (gctSIZE_T) bufsize)
        {
            size = bufsize;
        }

        if ((size > 0) && (infolog != gcvNULL))
        {
            gcmONERROR(gcoOS_MemCopy(infolog,
                                       shaderObject->compileLog,
                                       size));
        }
    }

    infolog[size] = '\0';

    if (length != gcvNULL)
    {
        *length = size;
    }

    gcmDUMP_API("${ES20 glGetShaderInfoLog 0x%08X 0x%08X (0x%08X) (0x%08X)",
                shader, bufsize, length, infolog);
    gcmDUMP_API_ARRAY(length, 1);
    gcmDUMP_API_DATA(infolog, size);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*length=%d", gcmOPT_VALUE(length));
	return;
OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
}

GL_APICALL void GL_APIENTRY
glGetShaderSource(
    GLuint  shader,
    GLsizei bufsize,
    GLsizei *length,
    char    *source
    )
{
    GLContext context;
    GLShader object;
    GLsizei size;
	gceSTATUS status;

    gcmHEADER_ARG("shader=%u bufsize=%d", shader, bufsize);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETSHADERSOURCE, 0);

    /* Find the shader object. */
    object = (GLShader) _glshFindObject(&context->shaderObjects, shader);

    /* Make sure the object is valid. */
    if (object == gcvNULL)
    {
        gcmFATAL("%s(%d): shader=%u is not a valid object", __FUNCTION__, __LINE__, shader);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a shader. */
    if ((object->object.type != GLObject_VertexShader) &&
        (object->object.type != GLObject_FragmentShader))
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetShaderSource: shader=%u is not a shader object",
                 shader);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (bufsize < 0)
    {
        gcmTRACE(gcvLEVEL_WARNING, "glGetShaderSource: bufsize(%d)<0", bufsize);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Get information of the source code. */
    size = (bufsize == 0) ? 0
         : (object->sourceSize < (bufsize - 1)) ? object->sourceSize
                                                : (bufsize - 1);
    if (length != gcvNULL)
    {
        *length = size;
    }

    if (source != gcvNULL)
    {
        if (size > 0)
        {
            gcmONERROR(gcoOS_MemCopy(source, object->source, size));
        }

        source[size] = '\0';
    }

    gcmDUMP_API("${ES20 glGetShaderSource 0x%08X 0x%08X (0x%08X) (0x%08X)",
                shader, bufsize, length, source);
    gcmDUMP_API_ARRAY(length, 1);
    gcmDUMP_API_DATA(source, size);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*length=%d", gcmOPT_VALUE(length));
	return;
OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
}

/******************************************************************************\
|******************************************************************* PROGRAMS *|
\******************************************************************************/

static void
_DeleteUniforms(
    GLProgram Program
    )
{
    if (Program->uniforms != gcvNULL)
    {
        GLint i;

        for (i = 0; i < Program->uniformCount; ++i)
        {
            if (Program->uniforms[i].data != gcvNULL)
            {
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL,
                                        Program->uniforms[i].data));
            }
        }

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->uniforms));

        Program->uniformCount = 0;
    }
}

void
_glshDeleteProgram(
    GLContext Context,
    GLProgram Program
    )
{
    /* Remove the program from the object list. */
    _glshRemoveObject(&Context->shaderObjects, &Program->object);

    /* Unbind any vertex shader. */
    if (Program->vertexShader != gcvNULL)
    {
        if ( (--Program->vertexShader->usageCount == 0)
             && Program->vertexShader->flagged )
        {
            /* The vertex shader was flagged, so delete it now. */
            _glshDeleteShader(Context, Program->vertexShader);
        }
    }

    /* Destroy the vertex shader binary. */
    if (Program->vertexShaderBinary != gcvNULL)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(Program->vertexShaderBinary));
    }

    /* Unbind any fragment shader. */
    if (Program->fragmentShader != gcvNULL)
    {
        if ( (--Program->fragmentShader->usageCount == 0)
             && Program->fragmentShader->flagged )
        {
            /* The fragment shader was flagged, so delete it now. */
            _glshDeleteShader(Context, Program->fragmentShader);
        }
    }

    /* Destroy the fragment shader binary. */
    if (Program->fragmentShaderBinary != gcvNULL)
    {
        gcmVERIFY_OK(gcSHADER_Destroy(Program->fragmentShaderBinary));
    }

    /* Free any link log. */
    if (Program->infoLog != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->infoLog));
    }

    /* Free any states. */
    if (Program->states != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->states));
    }

    /* Free any hints. */
    if (Program->hints != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->hints));
    }

    /* Free attribute pointers. */
    if (Program->attributePointers != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->attributePointers));
        Program->attributePointers = gcvNULL;
    }

    /* Free attribute bindings. */
    while (Program->attributeBinding != gcvNULL)
    {
        GLBinding binding = Program->attributeBinding;
        Program->attributeBinding = binding->next;

        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, binding->name));
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, binding));
    }

    /* Free attribute linkage. */
    if (Program->attributeLinkage != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->attributeLinkage));
    }

    /* Free attribute location. */
    if (Program->attributeLocation != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->attributeLocation));
    }

    /* Free attribute map. */
    if (Program->attributeMap != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->attributeMap));
    }

    /* Free any uniform buffer. */
    _DeleteUniforms(Program);

    if (Program->privateUniforms != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->privateUniforms));
    }

    /* Free the program. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program));
}

GL_APICALL GLuint GL_APIENTRY
glCreateProgram(
    void
    )
{
    GLContext context;
    GLProgram program = gcvNULL;
    GLuint i;
    gctPOINTER pointer = gcvNULL;

    gcmHEADER();

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_ARG("%u", 0);
        return 0;
    }

    glmPROFILE(context, GLES2_CREATEPROGRAM, 0);

    do
    {
        /* Allocate GLProgram structure. */
        if (gcmIS_ERROR(gcoOS_Allocate(gcvNULL,
                                       gcmSIZEOF(struct _GLProgram),
                                       &pointer)))
        {
            break;
        }

        program = pointer;
        gcmVERIFY_OK(gcoOS_ZeroMemory(program,
                                      gcmSIZEOF(struct _GLProgram)));

        /* Insert GLObject to hash tree. */
        if (!_glshInsertObject(&context->shaderObjects,
                               &program->object,
                               GLObject_Program,
                               0))
        {
            break;
        }

        /* Initialize GLProgram structure. */
        program->vertexShader         = gcvNULL;
        program->vertexShaderBinary   = gcvNULL;
        program->fragmentShader       = gcvNULL;
        program->fragmentShaderBinary = gcvNULL;
        program->linkStatus           = GL_FALSE;
        program->infoLog              = gcvNULL;
        program->validateStatus       = GL_FALSE;
        program->statesSize           = 0;
        program->states               = gcvNULL;
        program->hints                = gcvNULL;
        program->flagged              = GL_FALSE;
        program->attributeDirty       = GL_FALSE;
        program->attributeCount       = 0;
        program->attributeMaxLength   = 0;
        program->attributePointers    = gcvNULL;
        program->attributeBinding     = gcvNULL;
        program->attributeLinkage     = gcvNULL;
        program->attributeLocation    = gcvNULL;
        program->attributeMap         = gcvNULL;
        program->ltcUniformCount      = 0;
        program->uniformCount         = 0;
        program->uniformMaxLength     = 0;
        program->uniforms             = gcvNULL;
        program->privateUniformCount  = 0;
        program->privateUniforms      = gcvNULL;
        program->vertexSamplers       = 0;
        program->fragmentSamplers     = 0;

        gcmVERIFY_OK(gcoOS_ZeroMemory(program->sampleMap,
                                      gcmSIZEOF(program->sampleMap)));

        if (gcmIS_ERROR(
                gcoOS_Allocate(gcvNULL,
                               context->maxAttributes * gcmSIZEOF(GLuint),
                               &pointer)))
        {
            gcmFATAL("%s(%d): Cannot create attributeLinkage", __FUNCTION__, __LINE__);
            break;
        }

        program->attributeLinkage = pointer;

        if (gcmIS_ERROR(
                gcoOS_Allocate(gcvNULL,
                               context->maxAttributes * gcmSIZEOF(GLLocation),
                               &pointer)))
        {
            gcmFATAL("%s(%d): Cannot create attrributeLocation", __FUNCTION__, __LINE__);
            break;
        }

        program->attributeLocation = pointer;

        if (gcmIS_ERROR(
                gcoOS_Allocate(gcvNULL,
                               context->maxAttributes * gcmSIZEOF(GLuint),
                               &pointer)))
        {
            gcmFATAL("%s(%d): Cannot create attrributeMap", __FUNCTION__, __LINE__);
            break;
        }

        program->attributeMap = pointer;

        for (i = 0; i < context->maxAttributes; ++i)
        {
            program->attributeLinkage[i]            = (GLuint) 0xFFFFFFFF;
            program->attributeMap[i]                = (GLuint) 0xFFFFFFFF;
            program->attributeLocation[i].attribute = gcvNULL;
        }

        /* Return shader name. */
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glCreateProgram ==> %u",
                 program->object.name);

        gcmDUMP_API("${ES20 glCreateProgram := 0x%08X}", program->object.name);
        gcmFOOTER_ARG("%u", program->object.name);
        return program->object.name;
    }
    while (gcvFALSE);

    if (program != gcvNULL)
    {
        /* Remove the object from the linked list. */
        _glshRemoveObject(&context->shaderObjects, &program->object);

        if (program->attributeLinkage != gcvNULL)
        {
            /* Free the attribute linkage. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, program->attributeLinkage));
        }

        if (program->attributeLocation != gcvNULL)
        {
            /* Free the attribute location. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, program->attributeLocation));
        }

        if (program->attributeMap != gcvNULL)
        {
            /* Free the attribute map. */
            gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, program->attributeMap));
        }

        /* Free the program. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, program));
    }

    gl2mERROR(GL_OUT_OF_MEMORY);
    gcmFOOTER_ARG("%u", 0);
    return 0;
}

GL_APICALL void GL_APIENTRY
glDeleteProgram(
    GLuint program
    )
{
    GLContext context;
    GLProgram object;

    gcmHEADER_ARG("program=%u", program);
    gcmDUMP_API("${ES20 glDeleteProgram 0x%08X}", program);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_DELETEPROGRAM, 0);

    if (program == 0)
    {
        gcmFOOTER_NO();
        return;
    }

    object = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    if (object == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glDeleteProgram: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (object->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glDeleteProgram: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if ( context->program == object )
    {
        object->flagged = gcvTRUE;
    }
    else
    {
        _glshDeleteProgram(context, object);
    }

    gcmFOOTER_NO();
}

GL_APICALL GLboolean GL_APIENTRY
glIsProgram(
    GLuint program
    )
{
    GLContext context;
    GLObject object;
    GLboolean isProgram;

    gcmHEADER_ARG("program=%u", program);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_ARG("%u", GL_FALSE);
        return GL_FALSE;
    }

    glmPROFILE(context, GLES2_ISPROGRAM, 0);

    /* Find the object. */
    object = _glshFindObject(&context->shaderObjects, program);

    /* Object is a program if the object is valid and has type Program. */
    isProgram = (object != gcvNULL) && (object->type == GLObject_Program);

    gcmTRACE(gcvLEVEL_VERBOSE, "glIsProgram ==> %d", isProgram);

    gcmDUMP_API("${ES20 glIsProgram 0x%08X := 0x%08X}", program, isProgram);
    gcmFOOTER_ARG("%d", isProgram);
    return isProgram;
}

static void
_glshCleanProgramAttributes(
    GLContext Context,
    GLProgram Program
)
{
    GLuint i;

    if (Program->privateUniforms != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->privateUniforms));
    }

    /* Free any link log. */
    if (Program->infoLog != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->infoLog));
    }

    /* Free any states. */
    if (Program->states != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->states));
        Program->statesSize = 0;
    }

    /* Free any hints. */
    if (Program->hints != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->hints));
    }

    /* Zero existing attribute buffer. */
    Program->attributeCount = 0;

    /* Free attribute pointers. */
    if (Program->attributePointers != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, Program->attributePointers));
    }

    for (i = 0; i < Context->maxAttributes; ++i)
    {
        Program->attributeLinkage[i]            = (GLuint) -1;
        Program->attributeMap[i]                = (GLuint) -1;
        Program->attributeLocation[i].attribute = gcvNULL;
    }

    /* Free any uniform buffer. */
    _DeleteUniforms(Program);
}

GL_APICALL void GL_APIENTRY
glLinkProgram(
    GLuint program
    )
{
    GLProgram programObject;

	glmENTER1(glmARGUINT, program)
	{
    gcmDUMP_API("${ES20 glLinkProgram 0x%08X}", program);

    glmPROFILE(context, GLES2_LINKPROGRAM, 0);

    /* Find the program object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glLinkProgram: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glLinkProgram: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    /* Make sure the program has the shaders attached. */
    if (programObject->vertexShader == gcvNULL)
    {
        programObject->linkStatus = GL_FALSE;
        gcmVERIFY_OK(gcoOS_StrDup(gcvNULL,
                                  "No vertex shader attached.",
                                  &programObject->infoLog));
        break;
    }

    if (programObject->fragmentShader == gcvNULL)
    {
        programObject->linkStatus = GL_FALSE;
        gcmVERIFY_OK(gcoOS_StrDup(gcvNULL,
                                  "No fragment shader attached.",
                                  &programObject->infoLog));
        break;
    }

    /* Make sure the vertex shader is compiled. */
    if (programObject->vertexShader->binary == gcvNULL)
    {
        /* Compile the vertex shader. */
        glshCompileShader(context, programObject->vertexShader);

        if (!programObject->vertexShader->compileStatus)
        {
            /* Error compiling the vertex shader. */
            programObject->linkStatus = GL_FALSE;
            gcmVERIFY_OK(gcoOS_StrDup(gcvNULL,
                                      "Vertex shader did not compile.",
                                      &programObject->infoLog));
            break;
        }
    }

    /* Make sure the fragment shader is compiled. */
    if (programObject->fragmentShader->binary == gcvNULL)
    {
        /* Compile the fragment shader. */
        glshCompileShader(context, programObject->fragmentShader);

        if (!programObject->fragmentShader->compileStatus)
        {
            /* Error compiling the fragment shader. */
            programObject->linkStatus = GL_FALSE;
            gcmVERIFY_OK(gcoOS_StrDup(gcvNULL,
                                      "Fragment shader did not compile.",
                                      &programObject->infoLog));
            break;
        }
    }

    /* check if the program is already linked and the shaders are not dirty
     * then we need not to link it again, some applications may link the
     * program more than once
     */
    if (programObject->linkStatus == GL_TRUE &&
        programObject->states != gcvNULL &&
        !(programObject->vertexShader->dirty || programObject->fragmentShader->dirty) &&
        !programObject->attributeDirty)
    {
        break;
    }

#if GC_ENABLE_LOADTIME_OPT
    if (programObject->attributeDirty &&
        ((programObject->vertexShaderBinary && programObject->vertexShaderBinary->ltcUniformCount != 0) ||
         (programObject->fragmentShaderBinary && programObject->fragmentShaderBinary->ltcUniformCount != 0)))
    {
        /* need to redo the optimization to get the LTC expressions */
        programObject->vertexShader->dirty = GL_TRUE;
    }
#endif /* GC_ENABLE_LOADTIME_OPT */

    _glshCleanProgramAttributes(context, programObject);

    /* Link the shaders. */
    programObject->linkStatus = _glshLinkShaders(context, programObject);

	}
	glmLEAVE();
}

GL_APICALL void GL_APIENTRY
glGetProgramiv(
    GLuint program,
    GLenum pname,
    GLint * params
    )
{
    GLContext context;
    GLProgram programObject;
	gceSTATUS status;

    gcmHEADER_ARG("program=%u pname=0x%04x", program, pname);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETPROGRAMIV, 0);

    /* Find the object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetProgramiv: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetProgramiv: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Dispatch on pname. */
    switch (pname)
    {
    case GL_DELETE_STATUS:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=DELETE_STATUS",
                 program);

        *params = programObject->flagged;
        break;

    case GL_LINK_STATUS:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=LINK_STATUS",
                 program);

        *params = programObject->linkStatus;
        break;

    case GL_INFO_LOG_LENGTH:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=INFO_LOG_LENGTH",
                 program);

        if (programObject->infoLog == gcvNULL)
        {
            *params = 0;
        }
        else
        {
            gcmONERROR(gcoOS_StrLen(programObject->infoLog,
                                    (gctSIZE_T *) params));
            *params += 1;
        }
        break;

    case GL_ACTIVE_ATTRIBUTES:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=ACTIVE_ATTRIBUTES",
                 program);

        *params = programObject->attributeCount;
        break;

    case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=ACTIVE_ATTRIBUTE_MAX_LENGTH",
                 program);

        *params = programObject->attributeMaxLength + 1;
        break;

    case GL_ACTIVE_UNIFORMS:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=ACTIVE_UNIFORMS",
                 program);

        *params = programObject->uniformCount - programObject->ltcUniformCount;
        break;

    case GL_ACTIVE_UNIFORM_MAX_LENGTH:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=ACTIVE_UNIFORM_MAX_LENGTH",
                 program);

        *params = programObject->uniformMaxLength + 1;
        break;

    case GL_VALIDATE_STATUS:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=VALIDATE_STATUS",
                 program);

        *params = programObject->validateStatus;
        break;

    case GL_ATTACHED_SHADERS:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=ATTACHED_SHADERS",
                 program);

        *params = 0;
        if (programObject->vertexShader != gcvNULL)
        {
            *params += 1;
        }
        if (programObject->fragmentShader != gcvNULL)
        {
            *params += 1;
        }
        break;

    case GL_PROGRAM_BINARY_LENGTH_OES:
        gcmTRACE(gcvLEVEL_VERBOSE,
                 "glGetProgramiv: program=%u pname=PROGRAM_BINARY_LENGTH_OES",
                 program);

        if (programObject->linkStatus)
        {
            gceSTATUS status;
            status = gcSaveProgram(programObject->vertexShaderBinary,
                                   programObject->fragmentShaderBinary,
                                   programObject->statesSize,
                                   programObject->states,
                                   programObject->hints,
                                   gcvNULL,
                                   (gctSIZE_T *) params);

            if (gcmIS_ERROR(status))
            {
                gl2mERROR(GL_INVALID_OPERATION);
                gcmFOOTER_NO();
                return;
            }
        }
        else
        {
            gl2mERROR(GL_INVALID_OPERATION);
            gcmFOOTER_NO();
            return;
        }
        break;

    default:
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetProgramiv: unknown pname=0x%04X",
                 pname);
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    gcmDUMP_API("${ES20 glGetProgramiv 0x%08X 0x%08X (0x%08X)",
                program, pname, params);
    gcmDUMP_API_ARRAY(params, 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*params=%d", *params);
	return;
OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
}

static const GLenum gc2glType[] =
{
    GL_FLOAT,               /* gcUNIFORM_FLOAT_X1 */
    GL_FLOAT_VEC2,          /* gcUNIFORM_FLOAT_X2 */
    GL_FLOAT_VEC3,          /* gcUNIFORM_FLOAT_X3 */
    GL_FLOAT_VEC4,          /* gcUNIFORM_FLOAT_X4 */
    GL_FLOAT_MAT2,          /* gcUNIFORM_FLOAT_2X2 */
    GL_FLOAT_MAT3,          /* gcUNIFORM_FLOAT_3X3 */
    GL_FLOAT_MAT4,          /* gcUNIFORM_FLOAT_4X4 */
    GL_BOOL,                /* gcUNIFORM_BOOLEAN_X1 */
    GL_BOOL_VEC2,           /* gcUNIFORM_BOOLEAN_X2 */
    GL_BOOL_VEC3,           /* gcUNIFORM_BOOLEAN_X3 */
    GL_BOOL_VEC4,           /* gcUNIFORM_BOOLEAN_X4 */
    GL_INT,                 /* gcUNIFORM_INTEGER_X1 */
    GL_INT_VEC2,            /* gcUNIFORM_INTEGER_X2 */
    GL_INT_VEC3,            /* gcUNIFORM_INTEGER_X3 */
    GL_INT_VEC4,            /* gcUNIFORM_INTEGER_X4 */
    0,                      /* gcUNIFORM_SAMPLER_1D */
    GL_SAMPLER_2D,          /* gcUNIFORM_SAMPLER_2D */
    0,                      /* gcUNIFORM_SAMPLER_3D */
    GL_SAMPLER_CUBE,        /* gcUNIFORM_SAMPLER_CUBIC */
    GL_FIXED,               /* gcSHADER_FIXED_X1 */
    0,                      /* gcSHADER_FIXED_X2 */
    0,                      /* gcSHADER_FIXED_X3 */
    0,                      /* gcSHADER_FIXED_X4 */
    0,                      /* gcSHADER_IMAGE_2D */
    0,                      /* gcSHADER_IMAGE_3D */
    0,                      /* gcSHADER_SAMPLER */
    0,                      /* gcSHADER_FLOAT_2X3 */
    0,                      /* gcSHADER_FLOAT_2X4 */
    0,                      /* gcSHADER_FLOAT_3X2 */
    0,                      /* gcSHADER_FLOAT_3X4 */
    0,                      /* gcSHADER_FLOAT_4X2 */
    0,                      /* gcSHADER_FLOAT_4X3 */
    0,                      /* gcSHADER_ISAMPLER_2D */
    0,                      /* gcSHADER_ISAMPLER_3D */
    0,                      /* gcSHADER_ISAMPLER_CUBIC */
    0,                      /* gcSHADER_USAMPLER_2D */
    0,                      /* gcSHADER_USAMPLER_3D */
    0,                      /* gcSHADER_USAMPLER_CUBIC */
    GL_SAMPLER_EXTERNAL_OES,/* gcSHADER_SAMPLER_EXTERNAL_OES */

};

GL_APICALL void GL_APIENTRY
glGetActiveAttrib(
    GLuint program,
    GLuint index,
    GLsizei bufsize,
    GLsizei * length,
    GLint * size,
    GLenum * type,
    char * name
    )
{
    GLContext context;
    GLProgram programObject;
    gcATTRIBUTE attribute;
    gctSIZE_T attributeNameLength;
    gctCONST_STRING attributeName;
    gcSHADER_TYPE attributeType = gcSHADER_FLOAT_X1;
	gceSTATUS status;

    gcmHEADER_ARG("program=%u index=%u bufsize=%d", program, index, bufsize);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETACTIVEATTRIB, 0);

    /* Find the object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetActiveAttrib: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetActiveAttrib: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the attribute index is in range. */
    if (index >= (GLuint) programObject->attributeCount)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetActiveAttrib: index=%u is out of range",
                 index);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Get gcATTRIBUTE object pointer. */
    attribute = programObject->attributePointers[index];

    if (attribute == gcvNULL)
    {
        gcmFATAL("%s(%d): Attribute %d not used - why is it here?",
                 __FUNCTION__, __LINE__, index);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Get name of attribute. */
    gcmONERROR(gcATTRIBUTE_GetName(attribute,
                                     &attributeNameLength,
                                     &attributeName));

    /* Make sure it fits within the buffer. */
    if (attributeNameLength > (gctSIZE_T) bufsize - 1)
    {
        attributeNameLength = gcmMAX(bufsize - 1, 0);
    }

    if (name != gcvNULL)
    {
        if (attributeNameLength > 0)
        {
            /* Copy the name to the buffer. */
            gcmONERROR(gcoOS_MemCopy(name, attributeName, attributeNameLength));
        }

        name[attributeNameLength] = '\0';
    }

    if (length != gcvNULL)
    {
        /* Return number of characters copied, excluding the null terminator. */
        *length = attributeNameLength;
    }

    if (size != gcvNULL)
    {
        /* Get the type of the attribute. */
        gcmONERROR(gcATTRIBUTE_GetType(attribute,
                                         &attributeType,
                                         (gctSIZE_T *) size));
    }

    /* Convert type to GL enumeration. */
    if (type != gcvNULL)
    {
        *type = gc2glType[attributeType];
    }

    gcmDUMP_API("${ES20 glGetActiveAttrib 0x%08X 0x%08X 0x%08X (0x%08X) "
                "(0x%08X) (0x%08X) (0x%08X)",
                program, index, bufsize, length, size, type, name);
    gcmDUMP_API_ARRAY(length, 1);
    gcmDUMP_API_ARRAY(size, 1);
    gcmDUMP_API_ARRAY(type, 1);
    gcmDUMP_API_DATA(name, attributeNameLength);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*length=%d *size=%d *type=0x%04x name=0x%x",
                  gcmOPT_VALUE(length), gcmOPT_VALUE(size), gcmOPT_VALUE(type), name);

	gcmTRACE(gcvLEVEL_VERBOSE, "name=%s", gcmOPT_STRING(name));
	return;
OnError:
    gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
}

GL_APICALL int GL_APIENTRY
glGetAttribLocation(
    GLuint program,
    const char * name
    )
{
    GLContext context;
    GLProgram programObject;
    gctSIZE_T length = 0;
    GLuint i;
    gctSIZE_T attributeLength;
    gctCONST_STRING attributeName;

    gcmHEADER_ARG("program=%u name=0x%x", program, name);

	gcmTRACE(gcvLEVEL_VERBOSE, "name=%s", name);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_ARG("%d", -1);
        return -1;
    }

    glmPROFILE(context, GLES2_GETATTRIBLOCATION, 0);

    if (name == gcvNULL)
    {
        gcmFOOTER_ARG("%d", -1);
        return -1;
    }

    /* Find the object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetAttribLocation: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("%d", -1);
        return -1;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetAttribLocation: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_ARG("%d", -1);
        return -1;
    }

    if (!programObject->linkStatus)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_ARG("%d", -1);
        return -1;
    }

    gcmVERIFY_OK(gcoOS_StrLen(name, &length));

    /* Walk all attributes. */
    for (i = 0; i < context->maxAttributes; ++i)
    {
        GLint linkage = programObject->attributeLinkage[i];

        if (linkage == -1)
        {
            continue;
        }

        /* Get the attribute name. */
        gcmVERIFY_OK(
            gcATTRIBUTE_GetName(
                programObject->attributeLocation[linkage].attribute,
                &attributeLength,
                &attributeName));

        /* See if the attribute matches the requested name. */
        if ((length == attributeLength)
        &&  gcmIS_SUCCESS(gcoOS_MemCmp(name, attributeName, length))
        )
        {
            /* Match, return position. */
            gcmDUMP_API("${ES20 glGetAttribLocation 0x%08X (0x%08X) := 0x%08X",
                        program, name, i);
            gcmDUMP_API_DATA(name, 0);
            gcmDUMP_API("$}");
            gcmFOOTER_ARG("%d", i);
            return i;
        }
    }

    /* Attribute not found. */
    gcmFOOTER_ARG("%d", -1);
    return -1;
}

GLboolean glshSetProgram(GLContext Context, GLProgram Program)
{
    /* If the program is already current, do nothing. */
    if (Program == Context->program)
    {
        return GL_FALSE;
    }

    /* Delete current program if flagged for deletion. */
    if ((Context->program != gcvNULL) && Context->program->flagged)
    {
        _glshDeleteProgram(Context, Context->program);
    }

    /* Save current program. */
    Context->programDirty = GL_TRUE;
    Context->program      = Program;

    /* Disable batch optmization. */
    Context->batchDirty = GL_TRUE;

    return GL_TRUE;
}

GL_APICALL void GL_APIENTRY
glUseProgram(
    GLuint program
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLProgram programObject;

    gcmHEADER_ARG("program=%u", program);
    gcmDUMP_API("${ES20 glUseProgram 0x%08X}", program);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_USEPROGRAM, 0);

    if (program == 0)
    {
        /* Delete current program if flagged for deletion */
        if ( context->program && context->program->flagged )
        {
            _glshDeleteProgram(context, context->program);
        }
        /* Remove current program. */
        context->program = gcvNULL;
        gcmFOOTER_NO();
        return;
    }

    /* Find the object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glUseProgram: program=%u is not a valid object",
                 program);

        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glUseProgram: program=%u is not a program object",
                 program);

        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (!programObject->linkStatus)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glUseProgram: program=%u is not successfully linked",
                 program);

        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (programObject->statesSize == 0)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glUseProgram: program=%u cannot be supported by the hardware",
                 program);

        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (glshSetProgram(context, programObject))
    {
        glmPROFILE(context, GL_PROGRAM_IN_USE_BEGIN, 1);
        glmPROFILE(context, GL_PROGRAM_VERTEX_SHADER,
                   (gctUINT) context->program->vertexShaderBinary);
        glmPROFILE(context, GL_PROGRAM_FRAGMENT_SHADER,
                   (gctUINT) context->program->fragmentShaderBinary);
        glmPROFILE(context, GL_PROGRAM_IN_USE_END, 1);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glValidateProgram(
    GLuint program
    )
{
    GLContext context;
    GLProgram programObject;

    gcmHEADER_ARG("program=%u", program);
    gcmDUMP_API("${ES20 glValidateProgram 0x%08X}", program);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_VALIDATEPROGRAM, 0);

    /* Find the program object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glValidateProgram: program=%u is not a valid object", program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glValidateProgram: program=%u is not a program object", program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Free any existing log. */
    if (programObject->infoLog != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, programObject->infoLog));
    }

    programObject->validateStatus = (programObject->statesSize > 0);

    if ((programObject->vertexShader == gcvNULL)
    ||  !programObject->vertexShader->compileStatus
    )
    {
        programObject->validateStatus = GL_FALSE;
    }

    if ((programObject->fragmentShader == gcvNULL)
    ||  !programObject->fragmentShader->compileStatus
    )
    {
        programObject->validateStatus = GL_FALSE;
    }

    gcmFOOTER_NO();
}

GL_APICALL void GL_APIENTRY
glGetAttachedShaders(
    GLuint program,
    GLsizei maxcount,
    GLsizei *count,
    GLuint *shaders
    )
{
    GLContext context;
    GLProgram programObject;
    GLsizei n;

    gcmHEADER_ARG("program=%u maxcount=%d", program, maxcount);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETATTACHEDSHADERS, 0);

    /* Find the program object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetAttachedShaders: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetAttachedShaders: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Get the attached shader objects. */
    n = 0;

    if ( shaders != gcvNULL )
    {
        if ( (maxcount > n) && (programObject->vertexShader != gcvNULL) )
        {
            shaders[n] = programObject->vertexShader->object.name;
            n++;
        }

        if ( (maxcount > n) && (programObject->fragmentShader != gcvNULL) )
        {
            shaders[n] = programObject->fragmentShader->object.name;
            n++;
        }
    }

    if (count != gcvNULL)
    {
        *count = n;
    }

    gcmDUMP_API("${ES20 glGetAttachedShaders 0x%08X 0x%08X (0x%08X) (0x%08X)",
                program, maxcount, count, shaders);
    gcmDUMP_API_ARRAY(count, 1);
    gcmDUMP_API_ARRAY(shaders, n);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*count=%d", gcmOPT_VALUE(count));
}

GL_APICALL void GL_APIENTRY
glGetProgramInfoLog(
    GLuint  program,
    GLsizei bufsize,
    GLsizei *length,
    char    *infolog
    )
{
    GLContext context;
    GLProgram programObject;
    gctSIZE_T size;
	gceSTATUS status;

    gcmHEADER_ARG("program=%u bufsize=%d", program, bufsize);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETPROGRAMINFOLOG, 0);

    /* Find the program object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetProgramInfoLog: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetProgramInfoLog: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Get the log information. */
    if (programObject->infoLog == gcvNULL)
    {
        size = 0;
    }
    else
    {
        gcmONERROR(gcoOS_StrLen(programObject->infoLog, &size));
        if (size > (gctSIZE_T) bufsize)
        {
            size = bufsize;
        }

        if (infolog != gcvNULL)
        {
            gcmONERROR(gcoOS_MemCopy(infolog, programObject->infoLog, size));
        }
    }

    if (infolog != gcvNULL)
    {
        infolog[size] = '\0';
    }

    if (length != gcvNULL )
    {
        *length = size;
    }

    gcmDUMP_API("${ES20 glGetProgramInfoLog 0x%08X 0x%08X (0x%08X) (0x%08X)",
                program, bufsize, length, infolog);
    gcmDUMP_API_ARRAY(length, 1);
    gcmDUMP_API_DATA(infolog, size);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*length=%d", gcmOPT_VALUE(length));
	return;
OnError:
	gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
}

GL_APICALL void GL_APIENTRY
glAttachShader(
    GLuint program,
    GLuint shader
    )
{
    GLContext context;
    GLProgram programObject;
    GLShader shaderObject;

    gcmHEADER_ARG("program=%u shader=%u", program, shader);
    gcmDUMP_API("${ES20 glAttachShader 0x%08X 0x%08X}", program, shader);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_ATTACHSHADER, 0);

    /* Find the program object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glAttachShader: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetShaderiv: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Find the shader object. */
    shaderObject = (GLShader) _glshFindObject(&context->shaderObjects, shader);

    /* Make sure the object is valid. */
    if (shaderObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glAttachShader: shader=%u is not a valid object",
                 shader);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Dispatch on shader object type. */
    switch (shaderObject->object.type)
    {
    case GLObject_VertexShader:
        if (programObject->vertexShader != gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glAttachShader: program=%u already has a vertex shader "
                     "attached.",
                     program);
            gl2mERROR(GL_INVALID_OPERATION);
            gcmFOOTER_NO();
            return;
        }

        programObject->vertexShader = shaderObject;
        programObject->vertexShader->dirty = GL_TRUE;
        break;

    case GLObject_FragmentShader:
        if (programObject->fragmentShader != gcvNULL)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glAttachShader: program=%u already has a fragment shader "
                     "attached.",
                     program);
            gl2mERROR(GL_INVALID_OPERATION);
            gcmFOOTER_NO();
            return;
        }

        programObject->fragmentShader = shaderObject;
        programObject->fragmentShader->dirty = GL_TRUE;
        break;

    default:
        gcmTRACE(gcvLEVEL_WARNING,
                 "glAttachShader: shader=%u is not a shader object",
                 shader);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Increment the usage count for the shader object. */
    shaderObject->usageCount++;

    /* Disable batch optmization. */
    context->batchDirty = GL_TRUE;

    gcmFOOTER_NO();
}

GL_APICALL void GL_APIENTRY
glDetachShader(
    GLuint program,
    GLuint shader
    )
{
    GLContext context;
    GLProgram programObject;
    GLShader shaderObject;

    gcmHEADER_ARG("program=%u shader=%u", program, shader);
    gcmDUMP_API("${ES20 glDetachShader 0x%08X 0x%08X}", program, shader);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_DETACHSHADER, 0);

    /* Find the program object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glDetachShader: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glDetachShader: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Find the shader object. */
    shaderObject = (GLShader) _glshFindObject(&context->shaderObjects, shader);

    /* Make sure the object is valid. */
    if (shaderObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glDetachShader: shader=%u is not a valid object",
                 shader);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Dispatch on shader object type. */
    switch (shaderObject->object.type)
    {
    case GLObject_VertexShader:
        if (programObject->vertexShader != shaderObject)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glDetachShader: shader=%u is not attached to program=%u",
                     shader,
                     program);
            gl2mERROR(GL_INVALID_OPERATION);
            gcmFOOTER_NO();
            return;
        }

        programObject->vertexShader = gcvNULL;
        break;

    case GLObject_FragmentShader:
        if (programObject->fragmentShader != shaderObject)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "glDetachShader: shader=%u is not attached to program=%u",
                     shader,
                     program);
            gl2mERROR(GL_INVALID_OPERATION);
            gcmFOOTER_NO();
            return;
        }

        programObject->fragmentShader = gcvNULL;
        break;

    default:
        gcmTRACE(gcvLEVEL_WARNING,
                 "glAttachShader: shader=%u is not a shader object",
                 shader);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Decrement the usage count for the shader object. */
    if ( (--shaderObject->usageCount == 0) && shaderObject->flagged)
    {
        /* Shader was flagged for deletion, delete it now. */
        _glshDeleteShader(context, shaderObject);
    }

    gcmFOOTER_NO();
}

GL_APICALL void GL_APIENTRY
glBindAttribLocation(
    GLuint program,
    GLuint index,
    const char *name
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLProgram programObject;
    GLBinding binding = gcvNULL;

    gcmHEADER_ARG("program=%u index=%u name=0x%x", program, index, name);
	gcmTRACE(gcvLEVEL_VERBOSE, "name=%s", name);
    gcmDUMP_API("${ES20 glBindAttribLocation 0x%08X 0x%08X (0x%08X)",
                program, index, name);
    gcmDUMP_API_DATA(name, 0);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_BINDATTRIBLOCATION, 0);

    if (name == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    if ((name[0] == 'g') && (name[1] == 'l') && (name[2] == '_'))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    if (programObject == gcvNULL)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (programObject->object.type != GLObject_Program)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (index >= (GLuint) context->maxAttributes)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    for (binding = programObject->attributeBinding;
         binding != gcvNULL;
         binding = binding->next)
    {
        if (gcmIS_SUCCESS(gcoOS_StrCmp(binding->name, name)))
        {
            binding->index = index;
            programObject->attributeDirty = GL_TRUE;
            gcmFOOTER_NO();
            return;
        }
    }

    do
    {
        gceSTATUS status;
        gctPOINTER pointer = gcvNULL;

        gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                    gcmSIZEOF(struct _GLBinding),
                                    &pointer));

        binding = pointer;

        gcmERR_BREAK(gcoOS_StrDup(gcvNULL, name, &binding->name));

        binding->index                  = index;
        binding->next                   = programObject->attributeBinding;
        programObject->attributeBinding = binding;
        programObject->attributeDirty   = GL_TRUE;

        /* Disable batch optmization. */
        context->batchDirty = GL_TRUE;

        gcmFOOTER_NO();
        return;
    }
    while (gcvFALSE);

    if (binding != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, binding));
    }

    gl2mERROR(GL_OUT_OF_MEMORY);

    gcmFOOTER_NO();
#endif
}

/******************************************************************************\
|*                                                                   UNIFORMS *|
\******************************************************************************/

GL_APICALL void GL_APIENTRY
glGetActiveUniform(
    GLuint program,
    GLuint index,
    GLsizei bufsize,
    GLsizei * length,
    GLint * size,
    GLenum * type,
    char * name
    )
{
    GLContext context;
    GLProgram programObject;
    gcUNIFORM uniform;
    gctSIZE_T uniformNameLength;
    gctCONST_STRING uniformName;
    gcSHADER_TYPE uniformType = gcSHADER_FLOAT_X1;
	gceSTATUS status;

    gcmHEADER_ARG("program=%u index=%u bufsize=%d", program, index, bufsize);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETACTIVEUNIFORM, 0);

    /* Find the object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetActiveUniform: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetActiveUniform: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the uniform index is in range. */
    if (index >= (GLuint) programObject->uniformCount)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Get gcUNIFORM object pointer. */
    uniform = programObject->uniforms[index].uniform[0];

    /* Get name of uniform. */
    gcmONERROR(gcUNIFORM_GetName(uniform,
                                   &uniformNameLength,
                                   &uniformName));

    /* Make sure it fits within the buffer. */
    if (uniformNameLength > (gctSIZE_T) bufsize - 1)
    {
        uniformNameLength = gcmMAX(bufsize - 1, 0);
    }

    if (name != gcvNULL)
    {
        if (uniformNameLength > 0)
        {
            /* Copy the name to the buffer. */
            gcmONERROR(gcoOS_MemCopy(name, uniformName, uniformNameLength));
        }

        name[uniformNameLength] = '\0';
    }

    if (length != gcvNULL)
    {
        /* Return number of characters copied excluding the null character. */
        *length = uniformNameLength;
    }

    if (size != gcvNULL)
    {
        /* Get the type of the uniform. */
        gcmONERROR((gcUNIFORM_GetType(uniform, &uniformType, (gctSIZE_T *) size)));
    }

    /* Convert type to GL enumeration. */
    if (type != gcvNULL)
    {
        *type = gc2glType[uniformType];
    }

    gcmDUMP_API("${ES20 glGetActiveUniform 0x%08X 0x%08X 0x%08X (0x%08X) "
                "(0x%08X) (0x%08X) (0x%08X)",
                program, index, bufsize, length, size, type, name);
    gcmDUMP_API_ARRAY(length, 1);
    gcmDUMP_API_ARRAY(size, 1);
    gcmDUMP_API_ARRAY(type, 1);
    gcmDUMP_API_DATA(name, uniformNameLength + 1);
    gcmDUMP_API("$}");
    gcmFOOTER_ARG("*length=%d *size=%d *type=0x%04x name=0x%x",
                  gcmOPT_VALUE(length), gcmOPT_VALUE(size), gcmOPT_VALUE(type), name);
	gcmTRACE(gcvLEVEL_VERBOSE, "name=%s", gcmOPT_STRING(name));
	return;
OnError:
    gl2mERROR(GL_INVALID_OPERATION);
	gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
}

GL_APICALL int GL_APIENTRY
glGetUniformLocation(
    GLuint program,
    const char * name
    )
{
    GLContext context;
    GLProgram programObject;
    gctSIZE_T length;
    GLint i, index = 0;
    gctSIZE_T uniformLength;
    gctCONST_STRING uniformName;
	GLboolean bStructureArray = gcvFALSE;

    gcmHEADER_ARG("program=%u name=0x%x", program, name);
	gcmTRACE(gcvLEVEL_VERBOSE, "name=%s", name);

    if (name == gcvNULL)
    {
        gcmFOOTER_ARG("return=%d", -1);
        return -1;
    }

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_ARG("return=%d", -1);
        return -1;
    }

    glmPROFILE(context, GLES2_GETUNIFORMLOCATION, 0);

    /* Make sure the name does not start with "gl_". */
    if (gcmIS_SUCCESS(gcoOS_MemCmp(name, "gl_", 3)))
    {
        gcmFOOTER_ARG("return=%d", -1);
        return -1;
    }

    /* Find the object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects,
                                                program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetUniformLocation: program=%u is not a valid object",
                 program);
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("return=%d", -1);
        return -1;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetUniformLocation: program=%u is not a program object",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_ARG("return=%d", -1);
        return -1;
    }

    if (!programObject->linkStatus)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetUniformLocation: program=%u is not linked correctly",
                 program);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_ARG("return=%d", -1);
        return -1;
    }

    /* Get the length of the requested uniform. */
    for (length = 0; name[length] != '\0'; ++length)
    {
        if (name[length] == '[')
        {
            break;
        }
    }

    /* Special case arrays. */
    if (name[length] == '[')
    {
        i = length + 1;
        while ((name[i] >= '0') && (name[i] <= '9'))
        {
            index = index * 10 + name[i++] - '0';
        }

        if ((name[i] == ']') && (name[i + 1] == '.') )
        {
            /* According to GLES2.0 Spec, name can be a member of a structure array */
            bStructureArray = gcvTRUE;
        }
        else if ((name[i] != ']') || (name[i + 1] != '\0'))
        {
            gcmFATAL("%s(%d): Invalid array subscript: %s", __FUNCTION__, __LINE__, name);
            gl2mERROR(GL_INVALID_OPERATION);
            gcmFOOTER_ARG("return=%d", -1);
            return -1;
        }
    }
	if ( bStructureArray )
    {
        /*
         * Special handle structure array, since each member of structure element
         * is a separate uniform, This is different with normal array, which take
         * each array as a uniform, needs save the index value to locate the right
         * offset address in _SetUniforms.
         *
         */
        gcmVERIFY_OK(gcoOS_StrLen(name, &length));
        index = 0;
    }
    /* Walk all uniform. */
    for (i = 0; i < programObject->uniformCount; i++)
    {
        /* Get the uniform name. */
        gcmVERIFY_OK(gcUNIFORM_GetName(programObject->uniforms[i].uniform[0],
                                       &uniformLength,
                                       &uniformName));

        /* See if the uniform matches the requested name. */
        if ((length == uniformLength)
        &&  gcmIS_SUCCESS(gcoOS_MemCmp(name, uniformName, length))
        )
        {
            /* Match, return position. */
            gcmDUMP_API("${ES20 glGetUniformLocation 0x%08X (0x%08X) := 0x%08X",
                        program, name, i + (index << 16));
            gcmDUMP_API_DATA(name, 0);
            gcmDUMP_API("$}");
            gcmFOOTER_ARG("return=%d", i + (index << 16));
            return i + (index << 16);
        }
    }

    /* Uniform not found. */
    gcmFOOTER_ARG("return=%d", -1);
    return -1;
}

#if gcdNULL_DRIVER < 3
static void
_Float2Bool(
    GLfloat * Destination,
    const GLfloat * Source,
    GLsizei Count
    )
{
    GLsizei i;

    for (i = 0; i < Count; ++i)
    {
        Destination[i] = (Source[i] == 0.0f) ? 0.0f : 1.0f;
    }
}


static void
_Int2Bool(
    GLfloat * Destination,
    const GLint * Source,
    GLsizei Count
    )
{
    GLsizei i;

    for (i = 0; i < Count; ++i)
    {
        Destination[i] = (Source[i] == 0) ? 0.0f : 1.0f;
    }
}

static void
_Int2Float(
    GLfloat * Destination,
    const GLint * Source,
    GLsizei Count
    )
{
    GLsizei i;

    for (i = 0; i < Count; ++i)
    {
        Destination[i] = (GLfloat) Source[i];
    }
}

static void
_Float2Int(
    GLint * Destination,
    const GLfloat * Source,
    GLsizei Count
    )
{
    GLsizei i;

    for (i = 0; i < Count; ++i)
    {
        Destination[i] = (GLint) Source[i];
    }
}

#if GC_ENABLE_LOADTIME_OPT


void
_dumpGLUniform(
    GLUniform      Uniform,
    gcSHADER_TYPE  Type,
    GLsizei        Count,
    GLint          Index
    )
{
    gctINT    rows        = shader_type_info[Type].rows;
    gctINT    components  = shader_type_info[Type].components;
    GLfloat*  data        = (GLfloat *) Uniform->data + Index;
    gctINT    i;
    gctBOOL   singleValue = (rows*components*Count == 1);
    gctCHAR   buffer[512];
	gctUINT   offset      = 0;
    gctBOOL   printArrayIndex = 0;
    gctINT    arrayIndex = Index / (rows * components);

    gcmASSERT(Uniform != gcvNULL && Type < gcSHADER_TYPE_COUNT);
    /* print uniform type */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "uniform %s", shader_type_info[Type].name));

    /* print array size if it has one */
    if (Uniform->uniform[0]->arraySize > 1)
    {
	    gcmVERIFY_OK(
		    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						       "[%d]", Uniform->uniform[0]->arraySize));
        printArrayIndex = gcvTRUE;
    }

    /* print uniform name */
	gcmVERIFY_OK(
		gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   " %s", Uniform->uniform[0]->name));

    /* print index value */
    if (printArrayIndex)
    {
        gcmVERIFY_OK(
		    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						   "[%d]", arrayIndex));
    }

    gcmVERIFY_OK(
	    gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset," = "));

    switch (Type)
    {
    case gcSHADER_FLOAT_X1:
    case gcSHADER_FLOAT_X2:
    case gcSHADER_FLOAT_X3:
    case gcSHADER_FLOAT_X4:
    case gcSHADER_FLOAT_2X2:
    case gcSHADER_FLOAT_3X3:
    case gcSHADER_FLOAT_4X4:
    case gcSHADER_BOOLEAN_X1:
    case gcSHADER_BOOLEAN_X2:
    case gcSHADER_BOOLEAN_X3:
    case gcSHADER_BOOLEAN_X4:
    case gcSHADER_INTEGER_X1:
    case gcSHADER_INTEGER_X2:
    case gcSHADER_INTEGER_X3:
    case gcSHADER_INTEGER_X4:

        if (!singleValue)
	        gcmVERIFY_OK(
		        gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						           "{ \n"));

        for (i=0; i < Count; i ++)
        {
            int j;
            if (Count > 1)
                gcmVERIFY_OK(
		            gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						           "\t\t{"));

            /* print rows */
            for (j=0; j < rows; j++)
            {
                gctINT k;
                if (rows > 1)
                   gcmVERIFY_OK(
		               gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						           "\t\t\t{"));
                /* print print components */
                for (k = 0; k < components; k++)
                {
                    gcmVERIFY_OK(
		               gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						           " %10.6f", *data++));
                    if (k < components-1)
                       gcmVERIFY_OK(
		                   gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						           ","));
                }
                if (rows > 1)
                    gcmVERIFY_OK(
		                gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						           "  },\n"));
            }

            if (Count > 1)
            {
                gcmVERIFY_OK(
		            gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						           "} "));
                if (i != Count -1)
                   gcmVERIFY_OK(
		               gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
						           ",\n\t\t"));
            }
	        gcoOS_Print("%s", buffer);
            offset = 0;
        }
        if (!singleValue)
            gcoOS_Print("};\n");
        else
            gcoOS_Print(";\n");

        break;

    case gcSHADER_SAMPLER_2D:
    case gcSHADER_SAMPLER_CUBIC:
    case gcSHADER_SAMPLER_EXTERNAL_OES:
        gcmASSERT(Index == 0);
        gcmVERIFY_OK(
            gcoOS_PrintStrSafe(buffer, gcmSIZEOF(buffer), &offset,
				           " %6.4f;", *data));
        gcoOS_Print("%s", buffer);
        break;

    default:
        break;
    }

    return;
} /* _dumpGLUniform */

#endif /* GC_ENABLE_LOADTIME_OPT */

static void
_SetUniforms(
    GLContext Context,
    GLint Location,
    gcSHADER_TYPE Type,
    GLsizei Count,
    const void * Values
    )
{
    GLint location, index;
    GLProgram program;
    GLUniform uniform;
    GLenum error = GL_NO_ERROR;

    if (Values == gcvNULL)
    {
        return;
    }

    /* Get the current program. */
    program = Context->program;

    if (program == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING, "_SetUniforms: No active program");
        gl2mERROR(GL_INVALID_OPERATION);
        return;
    }

    location = Location & 0xFFFF;
    index    = Location >> 16;

    /* Make sure the uniform location is in range. */
    if ( (location < 0) || (location >= program->uniformCount) || (index < 0) )
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "_SetUniforms: location=%d is out of range",
                 location);
        gl2mERROR(GL_INVALID_VALUE);
        return;
    }

    /* Point to the GLUniform structure. */
    uniform = &program->uniforms[location];
    gcmASSERT(uniform != gcvNULL);

    error = _SetUniformData(program,
                            uniform,
                            Type,
                            Count,
                            index,
                            Values);

    if (error == GL_NO_ERROR)
    {
        /* Disable batch optmization. */
        Context->batchDirty = GL_TRUE;
    }
}
#endif

GLenum
_SetUniformData(
    GLProgram        Program,
    GLUniform        Uniform,
    gcSHADER_TYPE    Type,
    GLsizei          Count,
    GLint            Index,
    const void *     Values
    )
{
    gctSIZE_T           length;
    gctUINT32           sampler;
    gcSHADER_TYPE       type;
    GLenum              error = GL_NO_ERROR;

    if (Values == gcvNULL)
    {
        return error;
    }

    gcmASSERT(Uniform != gcvNULL);

    if (gcmIS_ERROR(gcUNIFORM_GetType(Uniform->uniform[0],
                                      &type,
                                      &length)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		return error;
	}

    if ( (length == 1) && (Count > 1) )
    {
        gl2mERROR(GL_INVALID_OPERATION);
        return error;
    }

    if ((gctSIZE_T) Count > length)
    {
        Count = length;
    }

    switch (type)
    {
    case gcSHADER_FLOAT_X1:
    case gcSHADER_FLOAT_X2:
    case gcSHADER_FLOAT_X3:
    case gcSHADER_FLOAT_X4:
    case gcSHADER_FLOAT_2X2:
    case gcSHADER_FLOAT_3X3:
    case gcSHADER_FLOAT_4X4:
        if (type == Type)
        {
            switch (type)
            {
            case gcSHADER_FLOAT_2X2:
                Index *= 4;
                break;

            case gcSHADER_FLOAT_3X3:
                Index *= 9;
                break;

            case gcSHADER_FLOAT_4X4:
                Index *= 16;
                break;

            default:
                break;
            }

            gcmVERIFY_OK(gcoOS_MemCopy((GLfloat *) Uniform->data + Index,
                                       Values,
                                       Count * _gcType2Bytes(Type)));
        }
        else
        {
            error = GL_INVALID_OPERATION;
        }
        break;

    case gcSHADER_BOOLEAN_X1:
        gcmASSERT(Index == 0);
        switch (Type)
        {
        case gcSHADER_BOOLEAN_X1:
        case gcSHADER_INTEGER_X1:
            _Int2Bool(Uniform->data, Values, Count);
            break;

        case gcSHADER_FLOAT_X1:
            _Float2Bool(Uniform->data, Values, Count);
            break;

        default:
            error = GL_INVALID_OPERATION;
            break;
        }
        break;

    case gcSHADER_BOOLEAN_X2:
        gcmASSERT(Index == 0);
        switch (Type)
        {
        case gcSHADER_BOOLEAN_X2:
        case gcSHADER_INTEGER_X2:
            _Int2Bool(Uniform->data, Values, 2 * Count);
            break;

        case gcSHADER_FLOAT_X2:
            _Float2Bool(Uniform->data, Values, 2 * Count);
            break;

        default:
            error = GL_INVALID_OPERATION;
            break;
        }
        break;

    case gcSHADER_BOOLEAN_X3:
        gcmASSERT(Index == 0);
        switch (Type)
        {
        case gcSHADER_BOOLEAN_X3:
        case gcSHADER_INTEGER_X3:
            _Int2Bool(Uniform->data, Values, 3 * Count);
            break;

        case gcSHADER_FLOAT_X3:
            _Float2Bool(Uniform->data, Values, 3 * Count);
            break;

        default:
            error = GL_INVALID_OPERATION;
            break;
        }
        break;

    case gcSHADER_BOOLEAN_X4:
        gcmASSERT(Index == 0);
        switch (Type)
        {
        case gcSHADER_BOOLEAN_X4:
        case gcSHADER_INTEGER_X4:
            _Int2Bool(Uniform->data, Values, 4 * Count);
            break;

        case gcSHADER_FLOAT_X4:
            _Float2Bool(Uniform->data, Values, 4 * Count);
            break;

        default:
            error = GL_INVALID_OPERATION;
            break;
        }
        break;

    case gcSHADER_INTEGER_X1:
        gcmASSERT(Index == 0);
        if (type == Type)
        {
            _Int2Float(Uniform->data, Values, 1 * Count);
        }
        else
        {
            error = GL_INVALID_OPERATION;
        }
        break;

    case gcSHADER_INTEGER_X2:
        gcmASSERT(Index == 0);
        if (type == Type)
        {
            _Int2Float(Uniform->data, Values, 2 * Count);
        }
        else
        {
            error = GL_INVALID_OPERATION;
        }
        break;

    case gcSHADER_INTEGER_X3:
        gcmASSERT(Index == 0);
        if (type == Type)
        {
            _Int2Float(Uniform->data, Values, 3 * Count);
        }
        else
        {
            error = GL_INVALID_OPERATION;
        }
        break;

    case gcSHADER_INTEGER_X4:
        gcmASSERT(Index == 0);
        if (type == Type)
        {
            _Int2Float(Uniform->data, Values, 4 * Count);
        }
        else
        {
            error = GL_INVALID_OPERATION;
        }
        break;

    case gcSHADER_SAMPLER_2D:
    case gcSHADER_SAMPLER_CUBIC:
    case gcSHADER_SAMPLER_EXTERNAL_OES:
        gcmASSERT(Index == 0);

        /* Currently,we have problem of sampler array, will fix later */
        if(Index > 0)
        {
            break;
        }

        if (Type == gcSHADER_INTEGER_X1)
        {
            _Int2Float(Uniform->data, Values, Count);

            /* Get sampler. */
            gcmVERIFY_OK(gcUNIFORM_GetSampler(Uniform->uniform[0], &sampler));

            /* Map sampler. */
            Program->sampleMap[sampler].unit = *(const GLint *) Values;
        }
        else
        {
            error = GL_INVALID_OPERATION;
        }
        break;

    default:
        error = GL_INVALID_OPERATION;
        break;
    }

    if (error == GL_NO_ERROR)
    {
        Uniform->dirty = GL_TRUE;

#if gldFBO_DATABASE
        Uniform->dbDirty = GL_TRUE;
#endif

#if GC_ENABLE_LOADTIME_OPT
        if (gcDumpOption(gceLTC_DUMP_UNIFORM))
        {
            _dumpGLUniform(Uniform, type, Count, Index);
        }
#endif /* GC_ENABLE_LOADTIME_OPT */
    }
    else
    {
        gl2mERROR(error);
    }

    return error;
}

GL_APICALL void GL_APIENTRY
glUniform1f(
    GLint location,
    GLfloat x
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d x=%f", location, x);
    gcmDUMP_API("${ES20 glUniform1f 0x%08X 0x%08X}",
                location, *(GLuint*) &x);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM1F, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_FLOAT_X1, 1, &x);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform1fv(
    GLint location,
    GLsizei count,
    const GLfloat * v
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d v=0x%x", location, count, v);
    gcmDUMP_API("${ES20 glUniform1fv 0x%08X 0x%08X (0x%08X)",
                location, count, v);
    gcmDUMP_API_ARRAY(v, count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM1FV, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_FLOAT_X1, count, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform2f(
    GLint location,
    GLfloat x,
    GLfloat y
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d x=%f y=%f", location, x, y);
    gcmDUMP_API("${ES20 glUniform2f 0x%08X 0x%08X 0x%08X}",
                location, *(GLuint*) &x, *(GLuint*) &y);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM2F, 0);

    if (location != -1)
    {
        GLfloat v[2];
        v[0] = x;
        v[1] = y;

        _SetUniforms(context, location, gcSHADER_FLOAT_X2, 1, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform2fv(
    GLint location,
    GLsizei count,
    const GLfloat *v
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d v=0x%x", location, count, v);
    gcmDUMP_API("${ES20 glUniform2fv 0x%08X 0x%08X (0x%08X)",
                location, count, v);
    gcmDUMP_API_ARRAY(v, 2 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM2FV, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_FLOAT_X2, count, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform3f(
    GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d x=%f y=%f z=%f", location, x, y, z);
    gcmDUMP_API("${ES20 glUniform3f 0x%08X 0x%08X 0x%08X 0x%08X}",
                location, *(GLuint*) &x, *(GLuint*) &y, *(GLuint*) &z);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM3F, 0);

    if (location != -1)
    {
        GLfloat v[3];
        v[0] = x;
        v[1] = y;
        v[2] = z;

        _SetUniforms(context, location, gcSHADER_FLOAT_X3, 1, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform3fv(
    GLint location,
    GLsizei count,
    const GLfloat * v
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d v=0x%x", location, count, v);
    gcmDUMP_API("${ES20 glUniform3fv 0x%08X 0x%08X (0x%08X)",
                location, count, v);
    gcmDUMP_API_ARRAY(v, 3 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM3FV, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_FLOAT_X3, count, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform4f(
    GLint location,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat w
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d x=%f y=%f z=%f w=%f", location, x, y, z, w);
    gcmDUMP_API("${ES20 glUniform4f 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
                location, *(GLuint*) &x, *(GLuint*) &y, *(GLuint*) &z,
                *(GLuint*) &w);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM4F, 0);

    if (location != -1)
    {
        GLfloat v[4];
        v[0] = x;
        v[1] = y;
        v[2] = z;
        v[3] = w;

        _SetUniforms(context, location, gcSHADER_FLOAT_X4, 1, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform4fv(
    GLint location,
    GLsizei count,
    const GLfloat *v
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d v=0x%x", location, count, v);
    gcmDUMP_API("${ES20 glUniform4fv 0x%08X 0x%08X (0x%08X)",
                location, count, v);
    gcmDUMP_API_ARRAY(v, 4 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM4FV, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_FLOAT_X4, count, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniformMatrix2fv(
    GLint location,
    GLsizei count,
    GLboolean transpose,
    const GLfloat * value
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d transpose=%d value=0x%x",
                  location, count, transpose, value);
    gcmDUMP_API("${ES20 glUniformMatrix2fv 0x%08X 0x%08X 0x%08X (0x%08X)",
                location, count, transpose, value);
    gcmDUMP_API_ARRAY(value, 4 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORMMATRIX2FV, 0);

    /* Transpose is not supported in OpenGL ES 2.0. */
    if (transpose)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_FLOAT_2X2, count, value);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniformMatrix3fv(
    GLint location,
    GLsizei count,
    GLboolean transpose,
    const GLfloat * value
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d transpose=%d value=0x%x",
                  location, count, transpose, value);
    gcmDUMP_API("${ES20 glUniformMatrix3fv 0x%08X 0x%08X 0x%08X (0x%08X)",
                location, count, transpose, value);
    gcmDUMP_API_ARRAY(value, 9 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORMMATRIX3FV, 0);

    /* Transpose is not supported in OpenGL ES 2.0. */
    if (transpose)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_FLOAT_3X3, count, value);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniformMatrix4fv(
    GLint location,
    GLsizei count,
    GLboolean transpose,
    const GLfloat * value
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d transpose=%d value=0x%x",
                  location, count, transpose, value);
    gcmDUMP_API("${ES20 glUniformMatrix4fv 0x%08X 0x%08X 0x%08X (0x%08X)",
                location, count, transpose, value);
    gcmDUMP_API_ARRAY(value, 16 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORMMATRIX4FV, 0);

    /* Transpose is not supported in OpenGL ES 2.0. */
    if (transpose)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_FLOAT_4X4, count, value);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform1i(
    GLint location,
    GLint x
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d x=%d", location, x);
    gcmDUMP_API("${ES20 glUniform1i 0x%08X 0x%08X}", location, x);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM1I, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_INTEGER_X1, 1, &x);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform1iv(
    GLint location,
    GLsizei count,
    const GLint * v
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d v=0x%x", location, count, v);
    gcmDUMP_API("${ES20 glUniform1iv 0x%08X 0x%08X (0x%08X)",
                location, count, v);
    gcmDUMP_API_ARRAY(v, count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM1IV, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_INTEGER_X1, count, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform2i(
    GLint location,
    GLint x,
    GLint y
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d x=%d y=%d", location, x, y);
    gcmDUMP_API("${ES20 glUniform2i 0x%08X 0x%08X 0x%08X}",
                location, x, y);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM2I, 0);

    if (location != -1)
    {
        GLint v[2];
        v[0] = x;
        v[1] = y;

        _SetUniforms(context, location, gcSHADER_INTEGER_X2, 1, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform2iv(
    GLint location,
    GLsizei count,
    const GLint * v
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d v=0x%x", location, count, v);
    gcmDUMP_API("${ES20 glUniform2iv 0x%08X 0x%08X (0x%08X)",
                location, count, v);
    gcmDUMP_API_ARRAY(v, 2 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM2IV, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_INTEGER_X2, count, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform3i(
    GLint location,
    GLint x,
    GLint y,
    GLint z
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d x=%d y=%d z=%d", location, x, y, z);
    gcmDUMP_API("${ES20 glUniform3i 0x%08X 0x%08X 0x%08X 0x%08X}",
                location, x, y, z);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM3I, 0);

    if (location != -1)
    {
        GLint v[3];
        v[0] = x;
        v[1] = y;
        v[2] = z;

        _SetUniforms(context, location, gcSHADER_INTEGER_X3, 1, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform3iv(
    GLint location,
    GLsizei count,
    const GLint * v
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d v=0x%x", location, count, v);
    gcmDUMP_API("${ES20 glUniform3iv 0x%08X 0x%08X (0x%08X)",
                location, count, v);
    gcmDUMP_API_ARRAY(v, 3 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM3IV, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_INTEGER_X3, count, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform4i(
    GLint location,
    GLint x,
    GLint y,
    GLint z,
    GLint w
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d x=%d y=%d z=%d w=%d", location, x, y, z, w);
    gcmDUMP_API("${ES20 glUniform4i 0x%08X 0x%08X 0x%08X 0x%08X 0x%08X}",
                location, x, y, z, w);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM4I, 0);

    if (location != -1)
    {
        GLint v[4];
        v[0] = x;
        v[1] = y;
        v[2] = z;
        v[3] = w;

        _SetUniforms(context, location, gcSHADER_INTEGER_X4, 1, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glUniform4iv(
    GLint location,
    GLsizei count,
    const GLint * v
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER_ARG("location=%d count=%d v=0x%x", location, count, v);
    gcmDUMP_API("${ES20 glUniform4iv 0x%08X 0x%08X (0x%08X)",
                location, count, v);
    gcmDUMP_API_ARRAY(v, 4 * count);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_UNIFORM4IV, 0);

    if (location != -1)
    {
        _SetUniforms(context, location, gcSHADER_INTEGER_X4, count, v);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glGetUniformfv(
    GLuint program,
    GLint location,
    GLfloat * params
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLProgram object;
    GLUniform uniform;
    gcSHADER_TYPE type;
    gctSIZE_T length;
    GLsizei bytes;
	gceSTATUS status;

    gcmHEADER_ARG("program=%u location=%i params=0x%x",
                  program, location, params);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETUNIFORMFV, 0);

    if (params == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    object = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    if (object == gcvNULL)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (object->object.type != GLObject_Program)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (!object->linkStatus)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if ( (location < 0) || (location >= object->uniformCount) )
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    uniform = &object->uniforms[location];
    gcmASSERT(uniform != gcvNULL);

    gcmONERROR(gcUNIFORM_GetType(uniform->uniform[0], &type, &length));

    switch (type)
    {
    case gcSHADER_FLOAT_X1:
    case gcSHADER_FLOAT_X2:
    case gcSHADER_FLOAT_X3:
    case gcSHADER_FLOAT_X4:
    case gcSHADER_FLOAT_2X2:
    case gcSHADER_FLOAT_3X3:
    case gcSHADER_FLOAT_4X4:
    case gcSHADER_BOOLEAN_X1:
    case gcSHADER_BOOLEAN_X2:
    case gcSHADER_BOOLEAN_X3:
    case gcSHADER_BOOLEAN_X4:
    case gcSHADER_INTEGER_X1:
    case gcSHADER_INTEGER_X2:
    case gcSHADER_INTEGER_X3:
    case gcSHADER_INTEGER_X4:
        bytes = _gcType2Bytes(type);
        break;

    default:
        gcmFATAL("glGetUniformfv: Invalid uniform type %d", type);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    gcmASSERT(uniform->data != gcvNULL);

    gcmONERROR(gcoOS_MemCopy(params, uniform->data, bytes));

    gcmDUMP_API("${ES20 glGetUniformfv 0x%08X 0x%08X (0x%08X)",
                program, location, params);
    gcmDUMP_API_ARRAY(params, bytes / 4);
    gcmDUMP_API("$}");
    gcmFOOTER_NO();
	return;
OnError:
    gl2mERROR(GL_INVALID_OPERATION);
    gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
#endif
}

GL_APICALL void GL_APIENTRY
glGetUniformiv(
    GLuint program,
    GLint location,
    GLint * params
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLProgram object;
    GLUniform uniform;
    gcSHADER_TYPE type;
    gctSIZE_T length;
    GLsizei count;

    gcmHEADER_ARG("program=%u location=%d", program, location);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETUNIFORMIV, 0);

    if (params == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    object = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    if (object == gcvNULL)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    if (object->object.type != GLObject_Program)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (!object->linkStatus)
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if ( (location < 0) || (location >= object->uniformCount) )
    {
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    uniform = &object->uniforms[location];
    gcmASSERT(uniform != gcvNULL);

    if (gcmIS_ERROR(gcUNIFORM_GetType(uniform->uniform[0], &type, &length)))
	{
		gl2mERROR(GL_INVALID_OPERATION);
		gcmFOOTER_ARG("error=0x%04x", GL_INVALID_OPERATION);
		return;
	}

    switch (type)
    {
    case gcSHADER_BOOLEAN_X1:
    case gcSHADER_INTEGER_X1:
    case gcSHADER_SAMPLER_1D:
    case gcSHADER_SAMPLER_2D:
    case gcSHADER_SAMPLER_3D:
    case gcSHADER_SAMPLER_CUBIC:
        count = 1;
        break;

    case gcSHADER_BOOLEAN_X2:
    case gcSHADER_INTEGER_X2:
        count = 2;
        break;

    case gcSHADER_BOOLEAN_X3:
    case gcSHADER_INTEGER_X3:
        count = 3;
        break;

    case gcSHADER_BOOLEAN_X4:
    case gcSHADER_INTEGER_X4:
        count = 4;
        break;

    default:
        gcmFATAL("%s(%d): Invalid uniform type %d", __FUNCTION__, __LINE__, type);
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    gcmASSERT(uniform->data != gcvNULL);
    _Float2Int(params, uniform->data, count);

    gcmDUMP_API("${ES20 glGetUniformiv 0x%08X 0x%08X (0x%08X)",
                program, location, params);
    gcmDUMP_API_ARRAY(params, count);
    gcmDUMP_API("$}");
    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glShaderBinary(
    GLint n,
    const GLuint *shaders,
    GLenum binaryformat,
    const void *binary,
    GLint length
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLShader shaderObject;
    gcSHADER vertexShader = gcvNULL;
    gcSHADER fragmentShader = gcvNULL;
    gceSTATUS status;
    GLint i;

    gcmHEADER_ARG("n=%d shaders=0x%x binaryformat=%d binary=0x%x length=%d",
                    n, shaders, binaryformat, binary, length);
    gcmDUMP_API("${ES20 glShaderBinary 0x%08X (0x%08X) 0x%08X (0x%08X) 0x%08X",
                n, shaders, binaryformat, binary, length);
    gcmDUMP_API_ARRAY(shaders, n);
    gcmDUMP_API_ARRAY(binary, length);
    gcmDUMP_API("$}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    if (binaryformat != GL_SHADER_BINARY_VIV &&
        binaryformat != GL_PROGRAM_BINARY_VIV) {
        gl2mERROR(GL_INVALID_ENUM);
        gcmFOOTER_NO();
        return;
    }

    if ((binary == gcvNULL) || (length == 0))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    for(i=0; i < n; i++) {
       /* Find the object. */
       shaderObject = (GLShader) _glshFindObject(&context->shaderObjects, shaders[i]);

       /* Make sure the object is valid. */
       if (shaderObject == gcvNULL) {
          gcmTRACE(gcvLEVEL_WARNING,
                   "glShaderBinary: shaders[%d]=%u is not a valid object",
                   i, shaders[i]);

          gl2mERROR(GL_INVALID_VALUE);
          gcmFOOTER_NO();
          return;
       }

       /* Make sure the object is a shader. */
       switch(shaderObject->object.type) {
       case GLObject_VertexShader:
          if(vertexShader) {
             gcmTRACE(gcvLEVEL_WARNING,
                      "glShaderBinary: shaders[%d]=%u is a duplicate vertex shader handle",
                      i, shaders[i]);
             gl2mERROR(GL_INVALID_OPERATION);
             gcmFOOTER_NO();
             return;
          }
          if(shaderObject->binary == gcvNULL) {
             status = gcSHADER_Construct(context->hal, GLObject_VertexShader, &shaderObject->binary);
             if(gcmIS_ERROR(status)) {
                gl2mERROR(GL_INVALID_OPERATION);
                gcmFOOTER_NO();
                return;
             }
          }
          vertexShader = shaderObject->binary;
          break;

       case GLObject_FragmentShader:
          if(fragmentShader) {
             gcmTRACE(gcvLEVEL_WARNING,
                      "glShaderBinary: shaders[%d]=%u is a duplicate fragment shader handle",
                      i, shaders[i]);
             gl2mERROR(GL_INVALID_OPERATION);
             gcmFOOTER_NO();
             return;
          }
          if(shaderObject->binary == gcvNULL) {
             status = gcSHADER_Construct(context->hal, GLObject_FragmentShader, &shaderObject->binary);
             if(gcmIS_ERROR(status)) {
                gl2mERROR(GL_INVALID_OPERATION);
                gcmFOOTER_NO();
                return;
             }
          }
          fragmentShader = shaderObject->binary;
          break;

       default:
          gcmTRACE(gcvLEVEL_WARNING,
                   "glShaderBinary: shaders[%d]=%u is not a shader object",
                   i, shaders[i]);

          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }
    }
    if(vertexShader == gcvNULL && fragmentShader == gcvNULL) {
       gcmTRACE(gcvLEVEL_WARNING,
                "glShaderBinary: no valid shader handle is supplied");
       gl2mERROR(GL_INVALID_OPERATION);
       gcmFOOTER_NO();
       return;
    }

    if (binaryformat == GL_SHADER_BINARY_VIV) {
       gcSHADER        shader;
       gcSHADER        found = gcvNULL;
       gctUINT32_PTR   compilerVersion;
       gctINT          shaderType;
       gctUINT32       shaderVersion;

       glmPROFILE(context, GLES2_SHADERBINARY, 0);
       status = gcSHADER_Construct(context->hal, gcSHADER_TYPE_PRECOMPILED, &shader);
       if(gcmIS_ERROR(status)) {
          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }
       gcSHADER_GetCompilerVersion(vertexShader? vertexShader: fragmentShader, &compilerVersion);
       gcSHADER_SetCompilerVersion(shader, compilerVersion);
       status = gcSHADER_LoadHeader(shader, (gctPOINTER) binary, length, &shaderVersion);
       if(gcmIS_ERROR(status)) {
          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }
       gcSHADER_GetType(shader, &shaderType);
       if(shaderType == GLObject_VertexShader && vertexShader) {
          found = vertexShader;
       }
       else if(shaderType == GLObject_FragmentShader && fragmentShader) {
          found = fragmentShader;
       }

       status = gcSHADER_Destroy(shader);
       if(gcmIS_ERROR(status)) {
          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }

       if(!found) {
          gcmTRACE(gcvLEVEL_WARNING,
                   "glShaderBinary: no valid shader handle is supplied for \"%s\" shader",
                   shaderType == GLObject_VertexShader ? "vertex" :
                   shaderType == GLObject_FragmentShader ? "fragment" : "unknown");
          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }
       status = gcSHADER_Load(found, (gctPOINTER) binary, length);
       if(gcmIS_ERROR(status)) {
          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }
    }
    else {  /*program binary */
       if (binaryformat != GL_PROGRAM_BINARY_VIV) {
          gl2mERROR(GL_INVALID_ENUM);
          gcmFOOTER_NO();
          return;
       }

       if(vertexShader == gcvNULL) {
          gcmTRACE(gcvLEVEL_WARNING,
                   "glShaderBinary: missing valid vertex shader handle for loading of program binary");
          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }

       if(fragmentShader == gcvNULL) {
          gcmTRACE(gcvLEVEL_WARNING,
                   "glShaderBinary: missing valid fragment shader handle for loading of program binary");
          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }

       glmPROFILE(context, GLES2_PROGRAMBINARYOES, 0);
       status = gcLoadProgram((gctPOINTER) binary,
                              length,
                          vertexShader,
                          fragmentShader,
                          gcvNULL,
                          gcvNULL,
                          gcvNULL);
       if(gcmIS_ERROR(status)) {
          gl2mERROR(GL_INVALID_OPERATION);
          gcmFOOTER_NO();
          return;
       }
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glReleaseShaderCompiler(
    void
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;

    gcmHEADER();
    gcmDUMP_API("${ES20 glReleaseShaderCompiler}");

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_RELEASESHADERCOMPILER, 0);

    if (context->compiler != gcvNULL)
    {
        _glshReleaseCompiler(context);
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glGetProgramBinaryOES(
    GLuint program,
    GLsizei bufSize,
    GLsizei *length,
    GLenum *binaryFormat,
    GLvoid *binary
    )
{
#if gcdNULL_DRIVER < 3
    GLContext context;
    GLProgram programObject;
    GLsizei binarySize;
    gceSTATUS status;

    gcmHEADER_ARG("program=%u bufSize=%d length=0x%x binaryFormat=0x%x binary=0x%x",
                    program, bufSize, length, binaryFormat, binary);

    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    glmPROFILE(context, GLES2_GETPROGRAMBINARYOES, 0);

    /* Find the object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetProgramBinaryOES: program=%u is not a valid object",
                 program);

        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glUseProgram: program=%u is not a program object",
                 program);

        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if (!programObject->linkStatus)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glUseProgram: program=%u is not successfully linked",
                 program);

        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_NO();
        return;
    }

    if ((binary == gcvNULL) || (bufSize == 0))
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_NO();
        return;
    }

    /* Get program size. */
    status = gcSaveProgram(programObject->vertexShaderBinary,
                           programObject->fragmentShaderBinary,
                           programObject->statesSize,
                           programObject->states,
                           programObject->hints,
                           gcvNULL,
                           (gctSIZE_T *) &binarySize);

    if (gcmIS_ERROR(status) || (binarySize > bufSize))
    {
        gl2mERROR(GL_INVALID_OPERATION);
        if (length)
        {
            *length = 0;
        }

        gcmFOOTER_NO();
        return;
    }

    /* Copy program to binary buffer. */
    status = gcSaveProgram(programObject->vertexShaderBinary,
                           programObject->fragmentShaderBinary,
                           programObject->statesSize,
                           programObject->states,
                           programObject->hints,
                           &binary,
                           (gctSIZE_T *) &binarySize);

    if (gcmIS_ERROR(status))
    {
        gl2mERROR(GL_INVALID_OPERATION);
    }
    else {
        if (length)
        {
            *length = binarySize;
        }

        if (binaryFormat)
        {
            *binaryFormat = GL_PROGRAM_BINARY_VIV;
        }
    }

    gcmFOOTER_NO();
#endif
}

GL_APICALL void GL_APIENTRY
glProgramBinaryOES(
    GLuint program,
    GLenum binaryFormat,
    const GLvoid *binary,
    GLint length
    )
{
#if gcdNULL_DRIVER < 3
    GLProgram programObject;
    gceSTATUS status;

	glmENTER4(glmARGINT, program, glmARGINT, binaryFormat, glmARGPTR, binary, glmARGINT, length)
	{
    gcmDUMP_API("${ES20 glProgramBinaryOES 0x%08X 0x%08X (0x%08X) 0x%08X",
                program, binaryFormat, binary, length);
    gcmDUMP_API_ARRAY(binary, length);
    gcmDUMP_API("$}");

    glmPROFILE(context, GLES2_PROGRAMBINARYOES, 0);

    if (binaryFormat != GL_PROGRAM_BINARY_VIV)
    {
        gl2mERROR(GL_INVALID_ENUM);
        break;
    }

    if ((binary == gcvNULL) || (length == 0))
    {
        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Find the object. */
    programObject = (GLProgram) _glshFindObject(&context->shaderObjects, program);

    /* Make sure the object is valid. */
    if (programObject == gcvNULL)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glGetProgramBinaryOES: program=%u is not a valid object",
                 program);

        gl2mERROR(GL_INVALID_VALUE);
        break;
    }

    /* Make sure the object is a program. */
    if (programObject->object.type != GLObject_Program)
    {
        gcmTRACE(gcvLEVEL_WARNING,
                 "glUseProgram: program=%u is not a program object",
                 program);

        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    if(programObject->vertexShader &&
       programObject->vertexShader->binary == programObject->vertexShaderBinary) {
       programObject->vertexShaderBinary = gcvNULL;
    }
    if(programObject->vertexShaderBinary == gcvNULL) {
       status = gcSHADER_Construct(context->hal, GLObject_VertexShader, &programObject->vertexShaderBinary);
       if(gcmIS_ERROR(status)) {
          gl2mERROR(GL_INVALID_OPERATION);
          break;
       }
    }

    if(programObject->fragmentShader &&
       programObject->fragmentShader->binary == programObject->fragmentShaderBinary) {
       programObject->fragmentShaderBinary = gcvNULL;
    }
    if(programObject->fragmentShaderBinary == gcvNULL) {
       status = gcSHADER_Construct(context->hal, GLObject_FragmentShader, &programObject->fragmentShaderBinary);
       if(gcmIS_ERROR(status)) {
          gl2mERROR(GL_INVALID_OPERATION);
          break;
       }
    }

    _glshCleanProgramAttributes(context, programObject);
/** A temporary workaround to link the shader ***/
    status = gcLoadProgram((gctPOINTER) binary,
                           length,
                       programObject->vertexShaderBinary,
                       programObject->fragmentShaderBinary,
                           gcvNULL,
                           gcvNULL,
                           gcvNULL);

    if(gcmIS_ERROR(status)) {
        gl2mERROR(GL_INVALID_OPERATION);
        break;
    }

    status = gcLinkShaders(programObject->vertexShaderBinary,
                           programObject->fragmentShaderBinary,
                           (gceSHADER_FLAGS)
                           ( gcvSHADER_RESOURCE_USAGE
                           | gcvSHADER_USE_GL_Z
                           | gcvSHADER_USE_GL_POINT_COORD
                           | gcvSHADER_USE_GL_POSITION
                           ),
                           &programObject->statesSize,
                           &programObject->states,
                           &programObject->hints);

    if (gcmIS_ERROR(status) &&
        (status != gcvSTATUS_OUT_OF_RESOURCES))
    {
        /* Error. */
        gcmFATAL("%s(%d): gcLinkShaders failed status=%d", __FUNCTION__, __LINE__, status);
        programObject->linkStatus = GL_FALSE;
        break;
    }


    programObject->linkStatus = _glshLinkProgramAttributes(context, programObject);
	}
	glmLEAVE();
#endif
}
