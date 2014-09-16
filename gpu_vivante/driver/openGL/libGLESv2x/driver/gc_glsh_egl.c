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
#include "EGL/egl.h"
#include <KHR/khrvivante.h>
#include "gc_egl_common.h"

#define _gcmTXT2STR(t) #t
#define gcmTXT2STR(t) _gcmTXT2STR(t)
const gctSTRING _MD5_VERSION = ""
                            gcmTXT2STR(gcvVERSION_BUILD)
                            gcmTXT2STR(gcvVERSION_PATCH)
                            gcmTXT2STR(gcvVERSION_MINOR)
                            gcmTXT2STR(gcvVERSION_MAJOR)
                            "$";

static void *
veglCreateContext(
    gcoOS Os,
    gcoHAL Hal,
    gctPOINTER SharedContext
    )
{
    GLContext context = gcvNULL;
    gco3D engine;
	gcoHARDWARE hw;
    gctUINT offset;
    gctPOINTER pointer = gcvNULL;
    gctSIZE_T extensionStringLength;
    gceSTATUS status;
    gctSTRING defaultExtensions = "GL_OES_compressed_ETC1_RGB8_texture"
                                   " "
                                  "GL_OES_compressed_paletted_texture"
                                  " "
                                  "GL_OES_EGL_image"
                                  " "
                                  "GL_OES_depth24"
                                  " "
                                  "GL_OES_element_index_uint"
                                  " "
                                  "GL_OES_fbo_render_mipmap"
                                  " "
                                  "GL_OES_fragment_precision_high"
                                  " "
                                  "GL_OES_rgb8_rgba8"
                                  " "
                                  "GL_OES_stencil1"
                                  " "
                                  "GL_OES_stencil4"
                                  " "
                                  /*"GL_OES_texture_3D"*/
                                  /*" "*/
                                  "GL_OES_texture_npot"
                                  " "
                                  "GL_OES_vertex_half_float"
                                  " "
                                  "GL_OES_depth_texture"
                                  " "
                                  "GL_OES_packed_depth_stencil"
                                  " "
                                  "GL_OES_standard_derivatives"
                                  " "
                                  "GL_OES_get_program_binary"
                                  " "
                                  "GL_EXT_texture_compression_dxt1"
                                  " "
                                  "GL_EXT_texture_format_BGRA8888"
                                  " "
                                  "GL_IMG_read_format"
                                  " "
                                  "GL_EXT_blend_minmax"
                                  " "
                                  "GL_EXT_read_format_bgra"
                                  " "
                                  "GL_EXT_multi_draw_arrays"
                                  " "
                                  /*"GL_OES_vertex_array_object"
                                  " "*/
                                  "GL_APPLE_texture_format_BGRA8888"
                                  " "
                                  "GL_APPLE_texture_max_level"
                                  " "
                                  "GL_ARM_rgba8"
                                  " "
                                  "GL_EXT_frag_depth"
                                  " "
                                  "GL_EXT_texture_compression_s3tc"
                                  " "
                                  "GL_VIV_shader_binary"
                                  " "
                                  "GL_VIV_timestamp"
                                  " "
                                  "GL_OES_mapbuffer"
                                  " "
                                  "GL_OES_EGL_image_external"
                                  " "
                                  "GL_EXT_discard_framebuffer"
                                  ;

    gcmHEADER_ARG("Os=0x%x Hal=0x%x SharedContext=0x%x",
                  Os, Hal, SharedContext);

    gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

    /* Allocate GLContext structure. */
    if (gcmIS_ERROR(gcoOS_Allocate(Os,
                                   gcmSIZEOF(struct _GLContext),
                                   &pointer)))
    {
        gcmFOOTER_ARG("0x%x", gcvNULL);
        return gcvNULL;
    }

    context = pointer;

    /* Query gco3D object pointer. */
    gcmONERROR(gcoHAL_Get3DEngine(Hal, &engine));

 	/* Query gcoHARDWARE object pointer. */
	gcmONERROR(gcoHAL_Get3DHardware(Hal, &hw));

    /* Initialize GLContext structure. */
    context->os     = Os;
    context->hal    = Hal;
    context->engine = engine;
	context->hw     = hw;
    context->read   = gcvNULL;

    /* Initialize error. */
    context->error = GL_NO_ERROR;

    /* Compiler. */
    context->dll      = gcvNULL;
    context->compiler = gcvNULL;

    /* Query hardware support. */
    gcmVERIFY_OK(gcoHAL_QueryTextureCaps(Hal,
                                         &context->maxTextureWidth,
                                         &context->maxTextureHeight,
                                         gcvNULL,
                                         gcvNULL,
                                         gcvNULL,
                                         &context->vertexSamplers,
                                         &context->fragmentSamplers));

    /* Query hardware support. */
    gcmVERIFY_OK(gcoHAL_QueryTextureMaxAniso(Hal,
                                             &context->maxAniso));

    gcmVERIFY_OK(gcoHAL_QueryTargetCaps(Hal,
                                        &context->maxWidth,
                                        &context->maxHeight,
                                        gcvNULL,
                                        gcvNULL));

    context->draw       = gcvNULL;
    context->drawWidth  = context->maxWidth;
    context->drawHeight = context->maxHeight;
    context->width      = context->drawWidth;
    context->height     = context->drawHeight;

    gcmVERIFY_OK(gcoHAL_QueryStreamCaps(Hal,
                                        &context->maxAttributes,
                                        gcvNULL,
                                        gcvNULL,
                                        gcvNULL));

    gcmVERIFY_OK(gcoHAL_QueryShaderCaps(Hal,
                                        &context->vertexUniforms,
                                        &context->fragmentUniforms,
                                        &context->maxVaryings));

    /* Query chip model. */
    gcmVERIFY_OK(gcoHAL_QueryChipIdentity(Hal,
                                          &context->model,
                                          &context->revision,
                                          gcvNULL,
                                          gcvNULL));
    offset = 0;
    gcmVERIFY_OK(
        gcoOS_PrintStrSafe(context->renderer,
                           gcmSIZEOF(context->renderer),
                           &offset,
                           "GC%x core",
                           context->model));

    /* Set Extension string. */
    gcmONERROR(gcoOS_StrLen(defaultExtensions, &extensionStringLength));

    /* Allocate more to make space for non-default extensions. */
    extensionStringLength += 200;

    gcmONERROR(gcoOS_Allocate(gcvNULL,
                              extensionStringLength,
                              (gctPOINTER *) &context->extensionString));
    offset = 0;
    gcmVERIFY_OK(
        gcoOS_PrintStrSafe(context->extensionString,
                           extensionStringLength,
                           &offset,
                           defaultExtensions));


    gcmONERROR(gcoHAL_QueryChipIdentity(
            Hal,
            &context->chipModel,
            &context->chipRevision,
            gcvNULL,
            gcvNULL
            ));

    if ((context->chipModel == gcv880 && context->chipRevision >= 0x5100 && context->chipRevision < 0x5130)
       || (context->chipModel == gcv1000 && context->chipRevision >= 0x5100 && context->chipRevision < 0x5130)
       || (context->chipModel == gcv2000 && context->chipRevision >= 0x5100 && context->chipRevision < 0x5130)
       || (context->chipModel == gcv4000 && context->chipRevision >= 0x5200 && context->chipRevision < 0x5220)
    )
    {
        context->patchStrip = gcvTRUE;
    }
    else
    {
        context->patchStrip = gcvFALSE;
    }

    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_VERTEX_10_10_10_2) == gcvSTATUS_TRUE)
    {
        gcmVERIFY_OK(
            gcoOS_StrCatSafe(context->extensionString,
                             extensionStringLength,
                             " GL_OES_vertex_type_10_10_10_2"));
    }

    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_10_10_10_2) == gcvSTATUS_TRUE)
    {
        gcmVERIFY_OK(
            gcoOS_StrCatSafe(context->extensionString,
                             extensionStringLength,
                             " GL_EXT_texture_type_2_10_10_10_REV"));
    }

    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_ANISOTROPIC_FILTERING) == gcvSTATUS_TRUE)
    {
        gcmVERIFY_OK(
            gcoOS_StrCatSafe(context->extensionString,
                             extensionStringLength,
                             " GL_EXT_texture_filter_anisotropic"));
    }

    if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_FLOAT_HALF_FLOAT) == gcvSTATUS_TRUE)
    {
        gcmVERIFY_OK(
            gcoOS_StrCatSafe(context->extensionString,
                             extensionStringLength,
                             " GL_OES_texture_float"
                             " GL_OES_texture_half_float"
                             " GL_OES_texture_half_float_linear"));
    }

    /* Set HAL API to OpenGL. */
    gcmVERIFY_OK(gco3D_SetAPI(engine, gcvAPI_OPENGL));

    /* Initialize context. */
    _glshInitializeBuffer(context);
    _glshInitializeClear(context);
    _glshInitializeFramebuffer(context);
    _glshInitializeShader(context);
    _glshInitializeState(context);
    _glshInitializeTexture(context);
    _glshInitializeVertex(context);
    _glshInitializeRenderbuffer(context);
    _glshInitializeDraw(context);
#if VIVANTE_PROFILER
    _glshInitializeProfiler(context);
#endif
#if defined(GC_USE_GL)
    _glshInitializeGL(context);
#endif

    /* Return GLContext structure. */
    gcmFOOTER_ARG("0x%x", context);
    return context;

OnError:
    /* Free the GLContext structure. */
    if(context != gcvNULL)
    {
    	gcmVERIFY_OK(gcmOS_SAFE_FREE(context->os, context));
    }

    gcmFOOTER_ARG("0x%x", context);
    return gcvNULL;
}

static void
_DestroyObjects(
    GLContext Context,
    GLObjectList * List
    )
{
    GLObject object, next;
    int i;

    /* Free all attached named objects. */
    for (i = 0; i < gldNAMED_OBJECTS_HASH; ++i)
    {
        /* Loop until this hash entry is empty. */
        for (object = List->objects[i]; object != gcvNULL; object = next)
        {
            /* Get next named object. */
            next = object->next;

            /* Dispatch in object type. */
            switch (object->type)
            {
            case GLObject_VertexShader:
            case GLObject_FragmentShader:
                /* Delete a shader object. */
                _glshDeleteShader(Context, (GLShader) object);
                break;

            case GLObject_Program:
                /* Delete a program object. */
                _glshDeleteProgram(Context, (GLProgram) object);
                break;

            case GLObject_Texture:
                /* Delete a texture object. */
                _glshDeleteTexture(Context, (GLTexture) object);
                break;

            case GLObject_Buffer:
                /* Delete a buffer object. */
                _glshDeleteBuffer(Context, (GLBuffer) object);
                break;

            case GLObject_Framebuffer:
                /* Delete a framebuffer object. */
                _glshDeleteFramebuffer(Context, (GLFramebuffer) object);
                break;

            case GLObject_Renderbuffer:
                /* Delete a renderbuffer object. */
                _glshDeleteRenderbuffer(Context, (GLRenderbuffer) object);
                break;

            case GLObject_VertexArray:
                _glshDeleteVertexArrayObject(Context, (GLVertexArray) object);
                break;

            default:
                gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRIVER,
                              "%s(%d): invalid type for object=0x%x",
                              __FUNCTION__, __LINE__, object);
            }
        }
    }
}

static EGLBoolean
veglDestroyContext(
    void * Context
    )
{
    GLContext context = (GLContext) Context;

    gcmHEADER_ARG("Context=0x%x", Context);

    /* Deinitialize context. */
    _glshDeinitializeDraw(context);
    _glshDeinitializeVertex(context);

    /* Free all attached named objects. */
    _DestroyObjects(context, &context->bufferObjects);
    _DestroyObjects(context, &context->shaderObjects);
    _DestroyObjects(context, &context->textureObjects);
    _DestroyObjects(context, &context->framebufferObjects);
    _DestroyObjects(context, &context->renderbufferObjects);
    _DestroyObjects(context, &context->vertexArrayObjects);

    if (context->default2D.texture != gcvNULL)
    {
        gcoTEXTURE_Destroy(context->default2D.texture);
    }

    if (context->default3D.texture != gcvNULL)
    {
        gcoTEXTURE_Destroy(context->default3D.texture);
    }

    if (context->defaultCube.texture != gcvNULL)
    {
        gcoTEXTURE_Destroy(context->defaultCube.texture);
    }

    if (context->defaultExternal.texture != gcvNULL)
    {
        gcoTEXTURE_Destroy(context->defaultExternal.texture);
    }

    /* Free compiler. */
    _glshReleaseCompiler(context);

#if VIVANTE_PROFILER
    /* Destroy the profiler. */
    _glshDestroyProfiler(context);
#endif

    if (context->extensionString != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(context->os, context->extensionString));
    }

    /* Free the GLContext structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(context->os, context));

    /* Success. */
    gcmFOOTER_NO();
    return EGL_TRUE;
}

static EGLBoolean
veglFlushContext(
    void * Context
    )
{
    /* What do we need to flush? */
    return EGL_TRUE;
}

#ifdef _DUMP_FILE
static void
_AddSurface(
    gcoDUMP Dump,
    gcoSURF Surface
    )
{
    gctUINT32 address[3] = {0};
    gctPOINTER memory[3] = {gcvNULL};
    gctUINT width, height;
    gctINT stride;
    gceSURF_FORMAT format;

    /* Lock the surface. */
    gcmVERIFY_OK(gcoSURF_Lock(Surface, address, memory));

    /* Get the size of the surface. */
    gcmVERIFY_OK(gcoSURF_GetAlignedSize(Surface, &width, &height, &stride));

    /* Get the format of the surface. */
    gcmVERIFY_OK(gcoSURF_GetFormat(Surface, gcvNULL, &format));

    /* Dump the surface. */
    gcmVERIFY_OK(gcoDUMP_AddSurface(Dump,
                                    width, height,
                                    format,
                                    address[0],
                                    stride * height));

    /* Unlock the surface. */
    gcmVERIFY_OK(gcoSURF_Unlock(Surface, memory[0]));
}
#endif

static gctBOOL
veglSetContext(
    void * Context,
    gcoSURF Draw,
    gcoSURF Read,
    gcoSURF Depth
    )
{
    GLContext context = (GLContext) Context;
    gceSTATUS status;
	gco3D currentEngine = gcvNULL;
	gcoHARDWARE currentHW = gcvNULL;

    gcmHEADER_ARG("Context=0x%x Draw=0x%x Read=0x%x Depth=0x%x",
                  Context, Draw, Read, Depth);

#ifdef _DUMP_FILE
    {
        gcoDUMP dump;

        if (context == NULL)
        {
            context = glshGetCurrentContext();
        }

        gcmVERIFY_OK(gcoHAL_GetDump(context->hal, &dump));

        gcmVERIFY_OK(gcoDUMP_Control(dump, (Context == gcvNULL) ? gcvNULL
                                                                : _DUMP_FILE));

        if (Context != gcvNULL)
        {
            _AddSurface(dump, Draw);
            _AddSurface(dump, Depth);
            gcmVERIFY_OK(gcoDUMP_FrameBegin(dump));
        }
    }
#endif
	gcmONERROR(gcoHAL_Query3DEngine(gcvNULL, &currentEngine));
	gcmONERROR(gcoHAL_Get3DHardware(gcvNULL, &currentHW));

    if (Context == gcvNULL)
    {
		gcmFOOTER_NO();
        return EGL_TRUE;
    }

	/* If there's framebuffer bound, ignore current face. */
	if (context->framebuffer != gcvNULL)
	{
	    /* Save surfaces. */
	    context->draw  = Draw;
	    context->read  = Read;
	    context->depth = Depth;
		gcmFOOTER_NO();
		return EGL_TRUE;
	}

    gcmONERROR(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

	/* Set HAL API to OpenGL. */
	gcmVERIFY_OK(gco3D_SetAPI(context->engine, gcvAPI_OPENGL));

    /* Set orientation for surfaces. */
    if (Draw != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_SetOrientation(Draw, gcvORIENTATION_BOTTOM_TOP));
    }
    if (Read != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_SetOrientation(Read, gcvORIENTATION_BOTTOM_TOP));
    }
    if (Depth != gcvNULL)
    {
        gcmVERIFY_OK(gcoSURF_SetOrientation(Depth, gcvORIENTATION_BOTTOM_TOP));
    }

    /* Set render target to HAL. */
    gcmVERIFY_OK(gco3D_SetTarget(context->engine, Draw));
    gcmVERIFY_OK(gco3D_SetDepth(context->engine, Depth));

    if (Depth == gcvNULL)
    {
        context->depthFormat = gcvSURF_UNKNOWN;
    }
    else
    {
        gcmVERIFY_OK(gcoSURF_GetFormat(Depth, gcvNULL, &context->depthFormat));
    }

    if (Draw != gcvNULL)
    {
        /* Save size of surface. */
        gcmONERROR(gcoSURF_GetSize(Draw,
                                   (gctUINT_PTR) &context->width,
                                   (gctUINT_PTR) &context->height,
                                   gcvNULL));

        context->drawWidth  = context->width;
        context->drawHeight = context->height;

        if (context->draw == gcvNULL)
        {
            /* Set initial viewport if none existed. */
            context->viewportX      = context->viewportY = 0;
            context->viewportWidth  = context->width;
            context->viewportHeight = context->height;
        }

        gcmTRACE_ZONE(gcvLEVEL_INFO, gldZONE_DRIVER,
                      "%s(%d): Draw size: %dx%d",
                      __FUNCTION__, __LINE__,
                      context->drawWidth,
                      context->drawHeight);

        context->viewDirty = GL_TRUE;

        /* Set initial scissor. */
        context->scissorWidth  = gcmMIN(context->scissorWidth,
                                        context->viewportWidth);
        context->scissorHeight = gcmMIN(context->scissorHeight,
                                        context->viewportHeight);

        /* Query surface samples. */
        gcmVERIFY_OK(gcoSURF_GetSamples(Draw, &context->samples));
        context->drawSamples = context->samples;
        context->antiAlias   = (context->samples > 1) ? GL_TRUE : GL_FALSE;
    }

    if (Depth != gcvNULL)
    {
        gceSURF_TYPE type;
        gceSURF_FORMAT format;

        /* Get depth format. */
        gcmVERIFY_OK(gcoSURF_GetFormat(Depth, &type, &format));

        /* Enable early-depth. */
        gcmVERIFY_OK(gco3D_SetEarlyDepth(context->engine, gcvTRUE));
    }

    /* Save surfaces. */
    context->draw  = Draw;
    context->read  = Read;
    context->depth = Depth;

    /* Detect resolve limitations for render target. */
    if ((Draw != gcvNULL) || (Depth != gcvNULL))
    {
        gceTILING tiling;

        /* Get tiling. */
        gcmVERIFY_OK(gcoSURF_GetTiling((Draw != gcvNULL)
                                       ? Draw : Depth,
                                       &tiling));

        switch (tiling)
        {
        case gcvMULTI_SUPERTILED:
            context->resolveMaskX      = 0x3F;
            context->resolveMaskY      = 0x7F;
            context->resolveMaskWidth  = 0x0F;
            context->resolveMaskHeight = 0x07;
            break;

        case gcvSUPERTILED:
            context->resolveMaskX      = 0x3F;
            context->resolveMaskY      = 0x3F;
            context->resolveMaskWidth  = 0x0F;
            context->resolveMaskHeight = 0x03;
            break;

        case gcvMULTI_TILED:
            context->resolveMaskX      = 0x03;
            context->resolveMaskY      = 0x07;
            context->resolveMaskWidth  = 0x0F;
            context->resolveMaskHeight = 0x07;
            break;

        case gcvTILED:
        default:
            context->resolveMaskX      = 0x03;
            context->resolveMaskY      = 0x03;
            context->resolveMaskWidth  = 0x0F;
            context->resolveMaskHeight = 0x03;
            break;
        }
    }

    /* Reprogram gco3D and gcoHARDWARE objects. */
    _glshInitializeObjectStates(context);

    /* Success. */
    gcmFOOTER_NO();
    return EGL_TRUE;

OnError:
	gcmVERIFY_OK(gcoHAL_Set3DEngine(gcvNULL, currentEngine));
	gcmVERIFY_OK(gcoHAL_Set3DHardware(gcvNULL, currentHW));

    gcmFOOTER();
    return EGL_FALSE;
}

GLContext
_glshGetCurrentContext(
    void
    )
{
    GLContext context;

    gcmHEADER();

    context = veglGetCurrentAPIContext();
    gcmASSERT(context != gcvNULL);

    gcmFOOTER_ARG("0x%x", context);
    return context;
}

GLContext
_glshCreateContext(
    void
    )
{
    gcoOS os   = gcvNULL;
    gcoHAL hal = gcvNULL;
    gceSTATUS status;
    GLContext context;

    gcmHEADER();

    gcmONERROR(gcoOS_Construct(gcvNULL, &os));
    gcmONERROR(gcoHAL_Construct(gcvNULL, os, &hal));
    context = veglCreateContext(os, hal, gcvNULL);
    if (context != gcvNULL)
    {
        gcmFOOTER_ARG("0x%x", context);
        return context;
    }

OnError:
    if (hal != gcvNULL)
    {
        gcmVERIFY_OK(gcoHAL_Destroy(hal));
    }

    if (os != gcvNULL)
    {
        gcmVERIFY_OK(gcoOS_Destroy(os));
    }

    gcmFOOTER_ARG("0x%x", gcvNULL);
    return gcvNULL;
}

void
_glshDestroyContext(
    GLContext Context
    )
{
    gcoOS  os;
    gcoHAL hal;

    gcmHEADER_ARG("Context=0x%x", Context);

    if (Context != gcvNULL)
    {
        os  = Context->os;
        hal = Context->hal;

        veglDestroyContext(Context);

        gcmVERIFY_OK(gcoHAL_Destroy(hal));
        gcmVERIFY_OK(gcoOS_Destroy(os));
    }

    gcmFOOTER_NO();
}

void
_glshFlush(
    IN GLContext Context,
    IN GLboolean Stall
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%0x Stall=%d", Context, Stall);

    gcmONERROR(gcoSURF_Flush(Context->draw));

    gcmONERROR(gcoHAL_Commit(Context->hal, Stall ? gcvTRUE : gcvFALSE));

    gcmFOOTER_NO();
    return;

OnError:
    gl2mERROR(GL_INVALID_OPERATION);
    gcmFOOTER();
}

/* Computes a 32 byte key for a Shader */
void
_glshComputeBlobCacheKey(
    GLShader Shader,
    gctMD5_Byte* DigestMD5
    )
{
    gctSTRING versionString;
    gcsMD5_State state;
    gctSIZE_T i;

    /* Find MD5 */
    gcoMD5_Init(&state);
    gcoMD5_Append(&state, (const gctMD5_Byte *)Shader->source, Shader->sourceSize);
    gcoMD5_Finish(&state, DigestMD5);

    /*
       append version to output, it is optimistic (assumes version string
       will not exceed 16 bytes) even if it exists build number will always
       be at the begining.
    */
    versionString = _MD5_VERSION;
    i = BLOB_CACHE_MD5_SIZE;
    while (*versionString != '$' && i < BLOB_CACHE_KEY_SIZE)
    {
        DigestMD5[i++] = *versionString;
        versionString++;
    }

    /* Append zeros if there is still space left */
    while (i < BLOB_CACHE_KEY_SIZE)
    {
        DigestMD5[i++] = 0;
    }
}

gceSTATUS
_glshBlobCacheGetShader(
    GLContext Context,
    GLShader Shader,
    gctMD5_Byte* BlobCacheKey
    )
{
#if gcdBLOB_CACHE_ENABLED
    gceSTATUS status;
    gctSTRING buffer = gcvNULL;
    gctSIZE_T allocatedBufferSize = BLOB_CACHE_MAX_BINARY_SIZE;
    EGLsizeiANDROID realBufferSize;
    do
    {
        /* Check if BlobCacheKey is OK. */
        if (BlobCacheKey == gcvNULL)
        {
           break;
        }

        /* Allocate memory to store shader. */
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, allocatedBufferSize, (gctPOINTER *) &buffer));

        /* Zero Fill Memory as much as header size to avoid any unlucky residual allocation. */
        gcoOS_MemFill(buffer, 0, 6*4);

        /* Compute Blob Cache Key. */
        _glshComputeBlobCacheKey (Shader, BlobCacheKey);

        /* Get the cached binary. */
        realBufferSize = veglGetBlobCache(BlobCacheKey,
                                          BLOB_CACHE_KEY_SIZE,
                                          buffer,
                                          allocatedBufferSize);

        /* Check if cache get is not successful. */
        if (realBufferSize == 0)
        {
            break;
        }

        /* Alternative to using the _glshCompileShader.
        gcmERR_BREAK(gcSHADER_Construct(Context->hal, Shader->object.type, &(Shader->binary)));
        gcmERR_BREAK(gcSHADER_Load(Shader->binary, (gctPOINTER) buffer, realBufferSize));
        Shader->compileStatus = GL_TRUE;
        */

        /* Compile the shader.         */
        Shader->compileStatus = _glshCompileShader(Context,
                                                   gcSHADER_TYPE_PRECOMPILED,
                                                   realBufferSize,
                                                   buffer,
                                                   &Shader->binary,
                                                   &Shader->compileLog);

        /* Free buffer. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, buffer));

        return gcvSTATUS_OK;
    }
    while (gcvFALSE);

    if (buffer != gcvNULL)
    {
        /* Free buffer. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, buffer));
    }
#endif
    return gcvSTATUS_NOT_FOUND;
}


void
_glshBlobCacheSetShader(
    GLShader Shader,
    gctMD5_Byte* BlobCacheKey
    )
{
#if gcdBLOB_CACHE_ENABLED
    gceSTATUS status;
    gctSIZE_T bufferSize;
    gctSTRING buffer = gcvNULL;

    do
    {
        /* Check for valid binary. */
        if (Shader == gcvNULL)
        {
            break;
        }

        /* Check is BlobCacheKey is OK. */
        if (BlobCacheKey == gcvNULL)
        {
           break;
        }

        /* Get the shader size. */
        gcmERR_BREAK(gcSHADER_Save(Shader->binary, gcvNULL, &bufferSize));

        /* Assert that the shader binary is no more than our max size. */
        gcmASSERT(bufferSize < BLOB_CACHE_MAX_BINARY_SIZE);

        /* Allocate memory to store shader. */
        gcmERR_BREAK(gcoOS_Allocate(gcvNULL, bufferSize, (gctPOINTER *) &buffer));

        /* Save the shader binary. */
        gcmERR_BREAK(gcSHADER_Save(Shader->binary, buffer, &bufferSize));

        /* Cache the binary. */
        veglSetBlobCache(BlobCacheKey, BLOB_CACHE_KEY_SIZE, buffer, bufferSize);

        /* Break and free buffer. */
        break;
    }
    while (gcvFALSE);

    if (buffer != gcvNULL)
    {
        /* Free buffer. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, buffer));
    }
#endif
}


static void
veglFlush(
    void
    )
{
    GLContext context;

    gcmHEADER();

    context = _glshGetCurrentContext();
    if (context != gcvNULL)
    {
        _glshFlush(context, GL_FALSE);
    }

    gcmFOOTER_NO();
}

static void
veglFinish(
    void
    )
{
    GLContext context;

    gcmHEADER();

    context = _glshGetCurrentContext();
    if (context != gcvNULL)
    {
        _glshFlush(context, GL_TRUE);
    }

    gcmFOOTER_NO();
}

static EGLenum
veglCreateImageTexture(
    EGLenum Target,
    gctINT Texture,
    gctINT Level,
    gctINT Depth,
    gctPOINTER Image
    )
{
    GLTexture texture = gcvNULL;
    gctINT32 referenceCount = 0;
    GLContext context;
    gcoSURF surface;
    khrEGL_IMAGE * image = gcvNULL;
    khrIMAGE_TYPE type;
    gceTEXTURE_FACE face = gcvFACE_NONE;
    gctUINT32 offset = 0;

    gcmHEADER_ARG("Target=0x%04x Texture=0x%08x Level=%d Depth=%d Image=0x%x",
                  Target, Texture, Level, Depth, Image);

    /* Create image from default texture isn't allowed. */
    if (Texture == 0)
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    context = _glshGetCurrentContext();

    if (context == gcvNULL)
    {
		gcmFOOTER_ARG("0x%04x", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    /* Find the texture object by name. */
    texture = (GLTexture) _glshFindObject(&context->textureObjects, Texture);

    /* Test texture object. */
    if ((texture == gcvNULL)
    ||  (texture->texture == gcvNULL)
    )
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    /* Test target parameter. */
    switch(Target)
    {
    case EGL_GL_TEXTURE_2D_KHR:
        face = gcvFACE_NONE;
        type = KHR_IMAGE_TEXTURE_2D;
        break;

    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
        face = gcvFACE_POSITIVE_X;
        type = KHR_IMAGE_TEXTURE_CUBE;
        break;

    case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
        face = gcvFACE_NEGATIVE_X;
        type = KHR_IMAGE_TEXTURE_CUBE;
        break;

    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
        face = gcvFACE_POSITIVE_Y;
        type = KHR_IMAGE_TEXTURE_CUBE;
        break;

    case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
        face = gcvFACE_NEGATIVE_Y;
        type = KHR_IMAGE_TEXTURE_CUBE;
        break;

    case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
        face = gcvFACE_POSITIVE_Z;
        type = KHR_IMAGE_TEXTURE_CUBE;
        break;

    case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
        face = gcvFACE_NEGATIVE_Z;
        type = KHR_IMAGE_TEXTURE_CUBE;
        break;

    default:
        gcmFOOTER_ARG("0x%04x", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    /* Get the surface of a level */
    if (gcmIS_ERROR(gcoTEXTURE_GetMipMapFace(texture->texture,
                                             Level,
                                             face,
                                             &surface,
                                             &offset)))
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    if (surface == gcvNULL)
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_ACCESS);
        return EGL_BAD_ACCESS;
    }

    /* Get source surface reference count. */
    gcmVERIFY_OK(
        gcoSURF_QueryReferenceCount(surface,
                                    &referenceCount));

    /* Test if surface is a sibling of any eglImage. */
    if (referenceCount > 1)
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_ACCESS);
        return EGL_BAD_ACCESS;
    }

    image = (khrEGL_IMAGE*) Image;

    /* Set EGL Image info. */
    image->magic             = KHR_EGL_IMAGE_MAGIC_NUM;
    image->type              = type;
    image->surface           = surface;
    image->u.texture.level   = Level;
    image->u.texture.face    = face;
    image->u.texture.depth   = Depth;
    image->u.texture.offset  = offset;
    image->u.texture.texture = Texture;
    image->u.texture.object  = texture->texture;

    gcmVERIFY_OK(
        gcoSURF_SetResolvability(surface, gcvFALSE));

    gcmFOOTER_ARG("0x%04x", EGL_SUCCESS);

    return EGL_SUCCESS;
}

static EGLenum
veglCreateImageRenderbuffer(
    EGLenum Renderbuffer,
    gctPOINTER Image
    )
{
    GLContext context;
    GLRenderbuffer object;
    gcoSURF surface = gcvNULL;
    khrEGL_IMAGE * image = gcvNULL;
    gctINT32 referenceCount = 0;

    gcmHEADER_ARG("Renderbuffer=%u Image=0x%x", Renderbuffer, Image);

    context = _glshGetCurrentContext();

    if ((context == gcvNULL) || (Renderbuffer == 0))
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    /* Find the object. */
    object = (GLRenderbuffer) _glshFindObject(&context->renderbufferObjects,
                                              Renderbuffer);

    /* Invalid render buffer object. */
    if (object == gcvNULL)
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_PARAMETER);
        return EGL_BAD_PARAMETER;
    }

    /* Get render buffer surface. */
    surface = object->surface;
    if (!surface)
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_ACCESS);
        return EGL_BAD_ACCESS;
    }

    /* Get source surface reference count. */
    gcoSURF_QueryReferenceCount(surface, &referenceCount);

    /* Test if surface is a sibling of any eglImage. */
    if (referenceCount > 1)
    {
        gcmFOOTER_ARG("0x%04x", EGL_BAD_ACCESS);
        return EGL_BAD_ACCESS;
    }

    image = (khrEGL_IMAGE*)Image;

    /* Set EGL Image info. */
    image->magic   = KHR_EGL_IMAGE_MAGIC_NUM;
    image->type    = KHR_IMAGE_RENDER_BUFFER;
    image->surface = surface;

    object->eglUsed = GL_TRUE;

    gcmFOOTER_ARG("0x%04x", EGL_SUCCESS);
    return EGL_SUCCESS;
}

static veglLOOKUP
_glaLookup[] =
{
    eglMAKE_LOOKUP(glActiveTexture),
    eglMAKE_LOOKUP(glAttachShader),
    eglMAKE_LOOKUP(glBindAttribLocation),
    eglMAKE_LOOKUP(glBindBuffer),
    eglMAKE_LOOKUP(glBindFramebuffer),
    eglMAKE_LOOKUP(glBindRenderbuffer),
    eglMAKE_LOOKUP(glBindTexture),
    eglMAKE_LOOKUP(glBindVertexArrayOES),
    eglMAKE_LOOKUP(glBlendColor),
    eglMAKE_LOOKUP(glBlendEquation),
    eglMAKE_LOOKUP(glBlendEquationSeparate),
    eglMAKE_LOOKUP(glBlendFunc),
    eglMAKE_LOOKUP(glBlendFuncSeparate),
    eglMAKE_LOOKUP(glBufferData),
    eglMAKE_LOOKUP(glBufferSubData),
    eglMAKE_LOOKUP(glCheckFramebufferStatus),
    eglMAKE_LOOKUP(glClear),
    eglMAKE_LOOKUP(glClearColor),
    eglMAKE_LOOKUP(glClearDepthf),
    eglMAKE_LOOKUP(glClearStencil),
    eglMAKE_LOOKUP(glColorMask),
    eglMAKE_LOOKUP(glCompileShader),
    eglMAKE_LOOKUP(glCompressedTexImage2D),
    eglMAKE_LOOKUP(glCompressedTexImage3DOES),
    eglMAKE_LOOKUP(glCompressedTexSubImage2D),
    eglMAKE_LOOKUP(glCompressedTexSubImage3DOES),
    eglMAKE_LOOKUP(glCopyTexImage2D),
    eglMAKE_LOOKUP(glCopyTexSubImage2D),
    eglMAKE_LOOKUP(glCopyTexSubImage3DOES),
    eglMAKE_LOOKUP(glCreateProgram),
    eglMAKE_LOOKUP(glCreateShader),
    eglMAKE_LOOKUP(glCullFace),
    eglMAKE_LOOKUP(glDeleteBuffers),
    eglMAKE_LOOKUP(glDeleteFramebuffers),
    eglMAKE_LOOKUP(glDeleteProgram),
    eglMAKE_LOOKUP(glDeleteRenderbuffers),
    eglMAKE_LOOKUP(glDeleteShader),
    eglMAKE_LOOKUP(glDeleteTextures),
    eglMAKE_LOOKUP(glDeleteVertexArraysOES),
    eglMAKE_LOOKUP(glDepthFunc),
    eglMAKE_LOOKUP(glDepthMask),
    eglMAKE_LOOKUP(glDepthRangef),
    eglMAKE_LOOKUP(glDetachShader),
    eglMAKE_LOOKUP(glDisable),
    eglMAKE_LOOKUP(glDisableVertexAttribArray),
    eglMAKE_LOOKUP(glDrawArrays),
    eglMAKE_LOOKUP(glDrawElements),
    eglMAKE_LOOKUP(glEGLImageTargetRenderbufferStorageOES),
    eglMAKE_LOOKUP(glEGLImageTargetTexture2DOES),
    eglMAKE_LOOKUP(glEnable),
    eglMAKE_LOOKUP(glEnableVertexAttribArray),
    eglMAKE_LOOKUP(glFinish),
    eglMAKE_LOOKUP(glFlush),
    eglMAKE_LOOKUP(glFramebufferRenderbuffer),
    eglMAKE_LOOKUP(glFramebufferTexture2D),
    eglMAKE_LOOKUP(glFramebufferTexture3DOES),
    eglMAKE_LOOKUP(glFrontFace),
    eglMAKE_LOOKUP(glGenBuffers),
    eglMAKE_LOOKUP(glGenFramebuffers),
    eglMAKE_LOOKUP(glGenRenderbuffers),
    eglMAKE_LOOKUP(glGenTextures),
    eglMAKE_LOOKUP(glGenVertexArraysOES),
    eglMAKE_LOOKUP(glGenerateMipmap),
    eglMAKE_LOOKUP(glGetActiveAttrib),
    eglMAKE_LOOKUP(glGetActiveUniform),
    eglMAKE_LOOKUP(glGetAttachedShaders),
    eglMAKE_LOOKUP(glGetAttribLocation),
    eglMAKE_LOOKUP(glGetBooleanv),
    eglMAKE_LOOKUP(glGetBufferParameteriv),
    eglMAKE_LOOKUP(glGetError),
    eglMAKE_LOOKUP(glGetFloatv),
    eglMAKE_LOOKUP(glGetFramebufferAttachmentParameteriv),
    eglMAKE_LOOKUP(glGetIntegerv),
    eglMAKE_LOOKUP(glGetProgramInfoLog),
    eglMAKE_LOOKUP(glGetProgramiv),
    eglMAKE_LOOKUP(glGetRenderbufferParameteriv),
    eglMAKE_LOOKUP(glGetShaderInfoLog),
    eglMAKE_LOOKUP(glGetShaderPrecisionFormat),
    eglMAKE_LOOKUP(glGetShaderSource),
    eglMAKE_LOOKUP(glGetShaderiv),
    eglMAKE_LOOKUP(glGetString),
    eglMAKE_LOOKUP(glGetTexParameterfv),
    eglMAKE_LOOKUP(glGetTexParameteriv),
    eglMAKE_LOOKUP(glGetUniformLocation),
    eglMAKE_LOOKUP(glGetUniformfv),
    eglMAKE_LOOKUP(glGetUniformiv),
    eglMAKE_LOOKUP(glGetVertexAttribPointerv),
    eglMAKE_LOOKUP(glGetVertexAttribfv),
    eglMAKE_LOOKUP(glGetVertexAttribiv),
    eglMAKE_LOOKUP(glHint),
    eglMAKE_LOOKUP(glIsBuffer),
    eglMAKE_LOOKUP(glIsEnabled),
    eglMAKE_LOOKUP(glIsFramebuffer),
    eglMAKE_LOOKUP(glIsProgram),
    eglMAKE_LOOKUP(glIsRenderbuffer),
    eglMAKE_LOOKUP(glIsShader),
    eglMAKE_LOOKUP(glIsTexture),
    eglMAKE_LOOKUP(glIsVertexArrayOES),
    eglMAKE_LOOKUP(glLineWidth),
    eglMAKE_LOOKUP(glLinkProgram),
    eglMAKE_LOOKUP(glPixelStorei),
    eglMAKE_LOOKUP(glPolygonOffset),
    eglMAKE_LOOKUP(glReadPixels),
    eglMAKE_LOOKUP(glReleaseShaderCompiler),
    eglMAKE_LOOKUP(glRenderbufferStorage),
    eglMAKE_LOOKUP(glSampleCoverage),
    eglMAKE_LOOKUP(glScissor),
    eglMAKE_LOOKUP(glShaderBinary),
    eglMAKE_LOOKUP(glShaderSource),
    eglMAKE_LOOKUP(glStencilFunc),
    eglMAKE_LOOKUP(glStencilFuncSeparate),
    eglMAKE_LOOKUP(glStencilMask),
    eglMAKE_LOOKUP(glStencilMaskSeparate),
    eglMAKE_LOOKUP(glStencilOp),
    eglMAKE_LOOKUP(glStencilOpSeparate),
    eglMAKE_LOOKUP(glTexImage2D),
    eglMAKE_LOOKUP(glTexImage3DOES),
    eglMAKE_LOOKUP(glTexParameterf),
    eglMAKE_LOOKUP(glTexParameterfv),
    eglMAKE_LOOKUP(glTexParameteri),
    eglMAKE_LOOKUP(glTexParameteriv),
    eglMAKE_LOOKUP(glTexSubImage2D),
    eglMAKE_LOOKUP(glTexSubImage3DOES),
    eglMAKE_LOOKUP(glTexDirectVIVMap),
    eglMAKE_LOOKUP(glTexDirectVIV),
    eglMAKE_LOOKUP(glTexDirectInvalidateVIV),
    eglMAKE_LOOKUP(glUniform1f),
    eglMAKE_LOOKUP(glUniform1fv),
    eglMAKE_LOOKUP(glUniform1i),
    eglMAKE_LOOKUP(glUniform1iv),
    eglMAKE_LOOKUP(glUniform2f),
    eglMAKE_LOOKUP(glUniform2fv),
    eglMAKE_LOOKUP(glUniform2i),
    eglMAKE_LOOKUP(glUniform2iv),
    eglMAKE_LOOKUP(glUniform3f),
    eglMAKE_LOOKUP(glUniform3fv),
    eglMAKE_LOOKUP(glUniform3i),
    eglMAKE_LOOKUP(glUniform3iv),
    eglMAKE_LOOKUP(glUniform4f),
    eglMAKE_LOOKUP(glUniform4fv),
    eglMAKE_LOOKUP(glUniform4i),
    eglMAKE_LOOKUP(glUniform4iv),
    eglMAKE_LOOKUP(glUniformMatrix2fv),
    eglMAKE_LOOKUP(glUniformMatrix3fv),
    eglMAKE_LOOKUP(glUniformMatrix4fv),
    eglMAKE_LOOKUP(glUseProgram),
    eglMAKE_LOOKUP(glValidateProgram),
    eglMAKE_LOOKUP(glVertexAttrib1f),
    eglMAKE_LOOKUP(glVertexAttrib1fv),
    eglMAKE_LOOKUP(glVertexAttrib2f),
    eglMAKE_LOOKUP(glVertexAttrib2fv),
    eglMAKE_LOOKUP(glVertexAttrib3f),
    eglMAKE_LOOKUP(glVertexAttrib3fv),
    eglMAKE_LOOKUP(glVertexAttrib4f),
    eglMAKE_LOOKUP(glVertexAttrib4fv),
    eglMAKE_LOOKUP(glVertexAttribPointer),
    eglMAKE_LOOKUP(glViewport),
    eglMAKE_LOOKUP(glMapBufferOES),
    eglMAKE_LOOKUP(glUnmapBufferOES),
    eglMAKE_LOOKUP(glGetBufferPointervOES),
    eglMAKE_LOOKUP(glDiscardFramebufferEXT),
    { gcvNULL, gcvNULL }
};

/* Dispatch table. */
GL_APICALL veglDISPATCH
GLESv2_DISPATCH_TABLE =
{
    /* createContext            */  veglCreateContext,
    /* destroyContext           */  veglDestroyContext,
    /* setContext               */  veglSetContext,
    /* flushContext             */  veglFlushContext,

    /* flush                    */  veglFlush,
    /* finish                   */  veglFinish,

    /* setBuffer                */  gcvNULL,
    /* getClientBuffer          */  gcvNULL,

    /* createImageTexture       */  veglCreateImageTexture,
    /* createImageRenderbuffer  */  veglCreateImageRenderbuffer,
    /* createImageParentImage   */  gcvNULL,
    /* bindTexImage             */  gcvNULL,

#if VIVANTE_PROFILER
    /* profiler                 */  _glshProfiler,
#else
    /* profiler                 */  gcvNULL,
#endif
    /* lookup                   */  _glaLookup,

    /* queryHWVG                */  gcvNULL,

    /* renderThread             */  gcvNULL,
};
