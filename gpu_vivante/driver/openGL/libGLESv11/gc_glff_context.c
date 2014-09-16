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
#include "gc_egl_common.h"

#define _GC_OBJ_ZONE    glvZONE_CONTEXT

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  _ConstructChipName
**
**  Construct chip name string.
**
**  INPUT:
**
**      ChipID
**          Chip ID.
**
**      ChipName
**          Pointer to the chip name string.
**
**  OUTPUT:
**
**      Nothing.
*/

static void _ConstructChipName(
    gctUINT32 ChipID,
    char* ChipName
    )
{
    gctINT i;
    gctBOOL foundID;

    /* Constant name postifx. */
    static const char postfix[] = " Graphics Engine";
    const gctINT postfixLength = gcmSIZEOF(postfix);

    gcmHEADER_ARG("ChipID=%u ChipName=0x%x", ChipID, ChipName);

    /* Append GC to the string. */
    *ChipName++ = 'G';
    *ChipName++ = 'C';

    /* Reset the ID found flag. */
    foundID = gcvFALSE;

    /* Translate the ID. */
    for (i = 0; i < 8; i++)
    {
        /* Get the current digit. */
        gctUINT8 digit = (gctUINT8) ((ChipID >> 28) & 0xFF);

        /* Append the digit to the string. */
        if (foundID || digit)
        {
            *ChipName++ = '0' + digit;
            foundID = gcvTRUE;
        }

        /* Advance to the next digit. */
        ChipID <<= 4;
    }

    /* Add the postfix. */
    for (i = 0; i < postfixLength; i++)
    {
        *ChipName++ = postfix[i];
    }
    gcmFOOTER_NO();
}


/*******************************************************************************
**
**  _AddSurface
**
**  Add surface to be dumped.
**
**  INPUT:
**
**      Dump
**          Pointer to the dumping object.
**
**      Surface
**          Pointer to the surface to add to dumping.
**
**  OUTPUT:
**
**      Nothing.
*/

#ifdef _DUMP_FILE
static void _AddSurface(
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
    gcmVERIFY_OK(gcoDUMP_AddSurface(
        Dump,
        width, height,
        format,
        address[0],
        stride * height
        ));

    /* Unlock the surface. */
    gcmVERIFY_OK(gcoSURF_Unlock(Surface, memory[0]));
}
#endif


/******************************************************************************\
********************** OpenGL ES 1.1 Context API Functions *********************
\******************************************************************************/

/*******************************************************************************
**
**  glfCreateContext
**
**  Create and initialize a context object.
**
**  INPUT:
**
**      Os
**          Pointer to an gcoOS object that needs to be destroyed.
**
**      Hal
**          Pointer to an gcoHAL object.
**
**      SharedContext
**          TBD.
**
**  OUTPUT:
**
**      Pointer to the new context object.
*/

void *
glfCreateContext(
    gcoOS Os,
    gcoHAL Hal,
    gctPOINTER SharedContext
    )
{
    gceSTATUS status;
    gctUINT offset;
    glsCONTEXT_PTR context = gcvNULL;
    gctSIZE_T extensionStringLength;
    gctSTRING defaultExtensions   = "GL_OES_blend_equation_separate"
		                            " "
		                            "GL_OES_blend_func_separate"
		                            " "
		                            "GL_OES_blend_subtract"
		                            " "
                                    "GL_OES_byte_coordinates"
                                    " "
                                    "GL_OES_compressed_ETC1_RGB8_texture"
                                    " "
                                    "GL_OES_compressed_paletted_texture"
                                    " "
                                    "GL_OES_draw_texture"
                                    " "
                                    "GL_OES_extended_matrix_palette"
                                    " "
                                    "GL_OES_fixed_point"
                                    " "
                                    "GL_OES_framebuffer_object"
                                    " "
                                    "GL_OES_matrix_get"
                                    " "
                                    "GL_OES_matrix_palette"
                                    " "
                                    "GL_OES_point_size_array"
                                    " "
                                    "GL_OES_point_sprite"
                                    " "
                                    "GL_OES_query_matrix"
                                    " "
                                    "GL_OES_read_format"
                                    " "
                                    "GL_OES_single_precision"
                                    " "
		                            "GL_OES_stencil_wrap"
		                            " "
                                    "GL_OES_texture_cube_map"
                                    " "
                                    /*"GL_OES_texture_env_crossbar"
                                    " "*/
		                            "GL_OES_texture_mirrored_repeat"
		                            " "
                                    "GL_OES_EGL_image"
                                    " "
                                    "GL_OES_depth24"
                                    " "
		                            "GL_OES_element_index_uint"
		                            " "
		                            "GL_OES_fbo_render_mipmap"
		                            " "
                                    "GL_OES_mapbuffer"
                                    " "
                                    "GL_OES_rgb8_rgba8"
                                    " "
                                    "GL_OES_stencil1"
                                    " "
                                    "GL_OES_stencil4"
                                    " "
                                    "GL_OES_stencil8"
                                    " "
                                    /*"GL_OES_texture_3D"
                                    " "*/
                                    /*"GL_OES_texture_npot"
                                    " "*/
                                    "GL_OES_vertex_half_float"
                                    " "
                                    "GL_OES_packed_depth_stencil"
		                            " "
                                    "GL_EXT_texture_compression_dxt1"
                                    " "
                                    "GL_EXT_texture_format_BGRA8888"
                                    " "
		                            "GL_IMG_read_format"
		                            " "
                                    "GL_IMG_user_clip_plane"
                                    " "
		                            "GL_APPLE_texture_2D_limited_npot"
		                            " "
		                            "GL_EXT_texture_lod_bias"
		                            " "
		                            "GL_EXT_blend_minmax"
		                            " "
		                            "GL_EXT_read_format_bgra"
		                            " "
                                    "GL_EXT_multi_draw_arrays"
                                    " "
                                    "GL_OES_EGL_sync"
                                    " "
		                            "GL_APPLE_framebuffer_multisample"
		                            " "
		                            "GL_APPLE_texture_format_BGRA8888"
		                            " "
		                            "GL_APPLE_texture_max_level"
		                            " "
		                            "GL_ARM_rgba8"
		                            " "
                                    /*"GL_OES_EGL_image_external"
		                            " "*/
                                    "GL_VIV_direct_texture"
                                    " "
                                    "GL_VIV_composition"
                                    " "
                                    "GL_EXT_texture_compression_s3tc"
									" "
                                    /* MM06 depends on this. */
									"GL_ARB_vertex_buffer_object"
						            " "
                                    /* MM06 depends on this. */
						            "GL_ARB_multitexture"
                                    ;

    gcmHEADER_ARG("Os=0x%x Hal=0x%x SharedContext=0x%x",
                  Os, Hal, SharedContext);

    do
    {
        gctPOINTER pointer = gcvNULL;

        gcmERR_BREAK(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

        /* Allocate the context structure. */
        gcmERR_BREAK(gcoOS_Allocate(
            Os,
            gcmSIZEOF(glsCONTEXT),
            &pointer
            ));

        context = pointer;

        /* Init context structure. */
        gcoOS_ZeroMemory(context, gcmSIZEOF(glsCONTEXT));

        /* Set API pointers. */
        context->hal = Hal;

		/* Set OS object. */
		context->os  = Os;

        /* Save shared context pointer. */
        context->shared = SharedContext;

        /* Query chip identity. */
        gcmERR_BREAK(gcoHAL_QueryChipIdentity(
            Hal,
            &context->chipModel,
            &context->chipRevision,
            gcvNULL,
            gcvNULL
            ));

        /* Test chip ID. */
        if (context->chipModel == 0)
        {
            status = gcvSTATUS_NOT_SUPPORTED;
            break;
        }

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

        /* Set Sub VBO sync cap to false by default. */
        context->bSyncSubVBO = gcvFALSE;

        /* Construct chip name. */
        _ConstructChipName(context->chipModel, context->chipName);

        /* Initialize driver strings. */
        context->chipInfo.vendor     = (GLubyte*) "Vivante Corporation";
        context->chipInfo.renderer   = (GLubyte*) context->chipName;
        context->chipInfo.version    = (GLubyte*) glvGLESDRIVERNAME;

        /* Set Extension string. */
        gcmONERROR(gcoOS_StrLen(defaultExtensions, &extensionStringLength));

        /* Allocate more to make space for non-default extensions. */
        extensionStringLength += 100;

        gcmONERROR(
            gcoOS_Allocate(gcvNULL,
                           extensionStringLength,
                           (gctPOINTER*)&context->chipInfo.extensions));

        offset = 0;
        gcmVERIFY_OK(
            gcoOS_PrintStrSafe(context->chipInfo.extensions,
                               extensionStringLength,
                               &offset,
                               defaultExtensions));

        if (gcoHAL_IsFeatureAvailable(gcvNULL, gcvFEATURE_TEXTURE_ANISOTROPIC_FILTERING)
            == gcvSTATUS_TRUE)
        {
            gcmVERIFY_OK(
                gcoOS_StrCatSafe(context->chipInfo.extensions,
                                 extensionStringLength,
                                 " GL_EXT_texture_filter_anisotropic"));
        }

        /* Determine whether the frament processor is available. */
        context->useFragmentProcessor = gcoHAL_IsFeatureAvailable(
            Hal, gcvFEATURE_FRAGMENT_PROCESSOR
            ) == gcvSTATUS_TRUE;

        /* Check whether IP has correct stencil support in depth-only mode. */
        context->hasCorrectStencil = gcoHAL_IsFeatureAvailable(
            Hal, gcvFEATURE_CORRECT_STENCIL
            ) == gcvSTATUS_TRUE;

        /* Check whether IP has tile status support. */
        context->hasTileStatus = gcoHAL_IsFeatureAvailable(
            Hal, gcvFEATURE_FAST_CLEAR
            ) == gcvSTATUS_TRUE;

        /* Check whether IP has logicOp support. */
        context->hwLogicOp = gcoHAL_IsFeatureAvailable(
            Hal, gcvFEATURE_LOGIC_OP
            ) == gcvSTATUS_TRUE;

        /* Get the 3D engine pointer. */
        gcmERR_BREAK(gcoHAL_Get3DEngine(
            Hal, &context->hw
            ));

        /* Get the target maximum size. */
        gcmERR_BREAK(gcoHAL_QueryTargetCaps(
            Hal, &context->maxWidth, &context->maxHeight, gcvNULL, gcvNULL
            ));

        /* Get the anisotropic texture max value. */
        gcmERR_BREAK(gcoHAL_QueryTextureMaxAniso(
            Hal, &context->maxAniso
            ));

        /* Get the texture max dimensions. */
        gcmERR_BREAK(gcoHAL_QueryTextureCaps(
            Hal, &context->maxTextureWidth, &context->maxTextureHeight,
            gcvNULL, gcvNULL, gcvNULL, gcvNULL, gcvNULL
            ));

        /* Set API type to OpenGL. */
        gcmERR_BREAK(gco3D_SetAPI(context->hw, gcvAPI_OPENGL));

        /* Initialize the drawing system. */
        gcmERR_BREAK(glfInitializeDraw(context));

#if gcdANDROID_UNALIGNED_LINEAR_COMPOSITION_ADJUST
        /* Get the pid */
        context->pid = gcoOS_GetCurrentProcessID();
#endif
    }
    while (GL_FALSE);

    /* Free the context buffer if error. */
    if (gcmIS_ERROR(status) && (context != gcvNULL))
    {
        /* Free the context. */
        gcmOS_SAFE_FREE(Os, context);

        /* Reset the pointer. */
        context = gcvNULL;
    }

#if VIVANTE_PROFILER
    if (context != gcvNULL)
    {
        _glffInitializeProfiler(context);
    }
#endif


    /* Return GLContext structure. */
    gcmFOOTER_ARG("context=0x%x", context);
    return context;

OnError:
    /* Free the GLContext structure. */
    gcmVERIFY_OK(gcmOS_SAFE_FREE(context->os, context));

    gcmFOOTER_ARG("context=0x%x", context);
    return gcvNULL;
}


/*******************************************************************************
**
**  glfDestroyContext
**
**  Destroy a context object.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Not zero if successfully destroyed.
*/

static EGLBoolean
glfDestroyContext(
    void * Context
    )
{
    gceSTATUS status = gcvSTATUS_OK, last;
    glsCONTEXT_PTR context = (glsCONTEXT_PTR) Context;
    gcmHEADER_ARG("Context=0x%x ", Context);

    /* Free the temporary bitmap. */
    gcmCHECK_STATUS(glfInitializeTempBitmap(context, gcvSURF_UNKNOWN, 0, 0));

    /* Destroy hash tables. */
    gcmCHECK_STATUS(glfFreeHashTable(context));

    /* Destroy texture management object. */
    gcmCHECK_STATUS(glfDestroyTexture(context));

    /* Free matrix stacks. */
    gcmCHECK_STATUS(glfFreeMatrixStack(context));

    /* Destroy buffer list. */
    gcmCHECK_STATUS(glfDestroyNamedObjectList(context, &context->bufferList));

    /* Destroy render buffer list. */
    gcmCHECK_STATUS(glfDestroyNamedObjectList(context, &context->renderBufferList));

    /* Destroy frame buffer list. */
    gcmCHECK_STATUS(glfDestroyNamedObjectList(context, &context->frameBufferList));

    /* Shut down the drawing system. */
    gcmCHECK_STATUS(glfDeinitializeDraw(context));

    /* Free up the render target and depth surface. */
    gcmCHECK_STATUS(gco3D_SetTarget(context->hw, gcvNULL));
    gcmCHECK_STATUS(gco3D_SetDepth(context->hw, gcvNULL));

#if VIVANTE_PROFILER
    _glffDestroyProfiler(context);
#endif

    if (context->chipInfo.extensions != gcvNULL)
    {
        gcmVERIFY_OK(gcmOS_SAFE_FREE(context->os, context->chipInfo.extensions));
    }

#if defined(ANDROID) && gcdDEFER_RESOLVES
    if (context->importedRT != gcvNULL)
    {
        /* Free the surf, but not the vidmem node.

           TO DO: don't free the vidmem node.
         */
        gcmOS_SAFE_FREE(gcvNULL, context->importedRT);
    }
#endif

    /* Destroy the context object. */
    gcmCHECK_STATUS(gcmOS_SAFE_FREE(gcvNULL, context));

    /* Assert if error. */
    if (gcmIS_ERROR(status))
    {
        gcmFATAL("glfDestroyContext failed.");
        gcmFOOTER_ARG("result=%d", EGL_FALSE);
        return EGL_FALSE;
    }
    else
    {
        gcmFOOTER_ARG("result=%d", EGL_TRUE);
        return EGL_TRUE;
    }
}


/*******************************************************************************
**
**  glfSetContext
**
**  Set current context to the specified one.
**
**  INPUT:
**
**      Context
**          Pointer to the context to be set as current.
**
**      Draw
**          Pointer to the surface to be used for drawing.
**
**      Read
**          Pointer to the surface to be used for reading.
**
**      Depth
**          Pointer to the surface to be used as depth buffer.
**
**  OUTPUT:
**
**      Nothing.
*/

gctBOOL
glfSetContext(
    void * Context,
    gcoSURF Draw,
    gcoSURF Read,
    gcoSURF Depth
    )
{
    gceSTATUS status;
    glsCONTEXT_PTR context = (glsCONTEXT_PTR) Context;

    gcmHEADER_ARG("Context=0x%x Draw=0x%x Read=0x%x Depth=0x%x",
                  Context, Draw, Read, Depth);

    do
    {
        gceSURF_FORMAT drawFormat;
        gctUINT drawWidth = 0, drawHeight = 0;
        glsCONTEXT_PTR currentContext;

        gcmERR_BREAK(gcoHAL_SetHardwareType(gcvNULL, gcvHARDWARE_3D));

        currentContext = GetCurrentContext();

#ifdef _DUMP_FILE
        /* Dumping. */
        {
            gcoDUMP dump;
            if (context == gcvNULL)
            {
                if (currentContext != gcvNULL)
                {
                    gcmVERIFY_OK(gcoHAL_GetDump(currentContext->hal, &dump));
                    gcmVERIFY_OK(gcoDUMP_Control(dump, gcvNULL));
                }
            }
            else
            {
                gcmVERIFY_OK(gcoHAL_GetDump(context->hal, &dump));
                gcmVERIFY_OK(gcoDUMP_Control(dump, _DUMP_FILE));
                _AddSurface(dump, Draw);
                _AddSurface(dump, Depth);
                gcmVERIFY_OK(gcoDUMP_FrameBegin(dump));
            }
        }
#endif

        /* Reset the target and the depth buffer. */
        if ((currentContext != gcvNULL) && (context != gcvNULL))
        {
            if (Draw == gcvNULL)
            {
                gcmERR_BREAK(gco3D_SetTarget(currentContext->hw, gcvNULL));
            }

            if (Depth == gcvNULL)
            {
                gcmERR_BREAK(gco3D_SetDepthMode(currentContext->hw, gcvDEPTH_NONE));

                gcmERR_BREAK(gco3D_SetDepth(currentContext->hw, gcvNULL));
            }
        }

        if (context == gcvNULL)
        {
            status = gcvSTATUS_OK;
            break;
        }

        if (Draw != gcvNULL)
        {
            gcmVERIFY_OK(gcoSURF_SetOrientation(Draw, gcvORIENTATION_BOTTOM_TOP));

            /* Get surface information. */
            gcmERR_BREAK(gcoSURF_GetSize(Draw, &drawWidth, &drawHeight, gcvNULL));
            gcmERR_BREAK(gcoSURF_GetFormat(Draw, gcvNULL, &drawFormat));
            gcmERR_BREAK(gcoSURF_QueryFormat(drawFormat, context->drawFormatInfo));
            gcmERR_BREAK(gcoSURF_GetSamples(Draw, &context->drawSamples));
        }
        if (Read != gcvNULL)
        {
            gcmVERIFY_OK(gcoSURF_SetOrientation(Read, gcvORIENTATION_BOTTOM_TOP));
        }
        if (Depth != gcvNULL)
        {
            gcmVERIFY_OK(gcoSURF_SetOrientation(Depth, gcvORIENTATION_BOTTOM_TOP));
        }

        /* Store the draw size. */
        context->drawWidth  = context->effectiveWidth  = drawWidth;
        context->drawHeight = context->effectiveHeight = drawHeight;

        /* Copy surfaces into context. */
        context->draw  = Draw;
        context->read  = Read;
        context->depth = Depth;

        /* Set surfaces into hardware context. */
        gco3D_SetTarget(context->hw, Draw);

        if (Depth != gcvNULL)
        {
            gco3D_SetDepthMode(
                    context->hw,
                    context->depthStates.depthMode
                    );
        }

        gco3D_SetDepth(context->hw, Depth);

        if ((Draw == gcvNULL)
         && (Read == gcvNULL)
         && (Depth == gcvNULL))
        {
            /* Everything that's needed to be done for this case,
               has been done till this point. */
            break;
        }

        /* Set defaults if this is a new context. */
        if (!context->initialized)
        {
#if defined(ANDROID) && gcdDEFER_RESOLVES
            /* Create a surface with dummy values and no vidmem,
               to be used as a RT in SF (only).
               It will be populated with actual values from
               a 3D app and used in a remote resolve.

               TO DO: Query draw surface linearity to
                      differentiate and do this for SF only.
             */
            gcmERR_BREAK(gcoSURF_Construct(
                            gcvNULL,
                            16,
                            4,
                            1,
                            gcvSURF_TEXTURE_NO_VIDMEM,
                            gcvSURF_A8R8G8B8,
                            gcvPOOL_DEFAULT,
                            &context->importedRT));
#endif
            /* fsRoundingEnabled enables a patch for conformance test where it
               fails in RGB8 mode because of absence of the proper conversion
               from 32-bit floating point PS output to 8-bit RGB frame buffer.
               At the time the problem was identified, GC500 was already taped
               out, so we fix the problem by rouding in the shader. */
            context->hashKey.hashFSRoundingEnabled =
            context->fsRoundingEnabled =
                (
                    (context->chipModel == gcv500) &&
                    (drawWidth  == 48) &&
                    (drawHeight == 48) &&
                    (!context->drawFormatInfo[0]->interleaved) &&
                    ( context->drawFormatInfo[0]->fmtClass == gcvFORMAT_CLASS_RGBA) &&
                    ( context->drawFormatInfo[0]->u.rgba.red.width   == 8) &&
                    ( context->drawFormatInfo[0]->u.rgba.green.width == 8) &&
                    ( context->drawFormatInfo[0]->u.rgba.blue.width  == 8)
                );

            /* Initialize buffer list. */
            gcmERR_BREAK(glfConstructNamedObjectList(
                context,
                &context->bufferList,
                sizeof(glsBUFFER)
                ));

            /* Initialize render buffer list. */
            gcmERR_BREAK(glfConstructNamedObjectList(
                context,
                &context->renderBufferList,
                sizeof(glsRENDER_BUFFER)
                ));

            /* Initialize frame buffer list. */
            gcmERR_BREAK(glfConstructNamedObjectList(
                context,
                &context->frameBufferList,
                sizeof(glsFRAME_BUFFER)
                ));

            /* Construct objects. */
            gcmERR_BREAK(glfInitializeHashTable(context));
            gcmERR_BREAK(glfInitializeTexture(context));
            gcmERR_BREAK(glfInitializeStreams(context));
            gcmERR_BREAK(glfInitializeMatrixStack(context));

            /* Initialize default states. */
            gcmERR_BREAK(glfSetDefaultMiscStates(context));
            gcmERR_BREAK(glfSetDefaultCullingStates(context));
            gcmERR_BREAK(glfSetDefaultAlphaStates(context));
            gcmERR_BREAK(glfSetDefaultDepthStates(context));
            gcmERR_BREAK(glfSetDefaultLightingStates(context));
            gcmERR_BREAK(glfSetDefaultPointStates(context));
            gcmERR_BREAK(glfSetDefaultFogStates(context));
            gcmERR_BREAK(glfSetDefaultLineStates(context));
            gcmERR_BREAK(glfSetDefaultPixelStates(context));
            gcmERR_BREAK(glfSetDefaultClipPlaneStates(context));
            gcmERR_BREAK(glfSetDefaultViewportStates(context));

            /* Mark as initialized. */
            context->initialized = gcvTRUE;
        }

        /* Readjust viewport and scissor. */
        glfViewport(context->viewportStates.viewportBox[0],
                   context->viewportStates.viewportBox[1],
                   context->viewportStates.viewportBox[2],
                   context->viewportStates.viewportBox[3]);

        glfScissor(context->viewportStates.scissorBox[0],
                  context->viewportStates.scissorBox[1],
                  context->viewportStates.scissorBox[2],
                  context->viewportStates.scissorBox[3]);
    }
    while (GL_FALSE);

    /* Return result. */
    gcmFOOTER();
    return gcmIS_SUCCESS(status);
}


/*******************************************************************************
**
**  glfFlushContext/glfFlush/glfFinish
**
**  Context flushing functions.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing.
*/

static EGLBoolean
glfFlushContext(
    void * Context
    )
{
    gcmHEADER_ARG("Context=0x%x ", Context);
    glFlush();
    gcmFOOTER_ARG("result=%d", EGL_TRUE);
    return EGL_TRUE;
}

static void
glfFlush(
    void
    )
{
    gcmHEADER();
    glFlush();
    gcmFOOTER_NO();
}

static void
glfFinish(
    void
    )
{
    gcmHEADER();
    glFinish();
    gcmFOOTER_NO();
}

/*******************************************************************************
**
**  glFlush
**
**  Different GL implementations buffer commands in several different locations,
**  including network buffers and the graphics accelerator itself. glFlush
**  empties all of these buffers, causing all issued commands to be executed
**  as quickly as they are accepted by the actual rendering engine. Though this
**  execution may not be completed in any particular time period, it does
**  complete in finite time.
**
**  Because any GL program might be executed over a network, or on an
**  accelerator that buffers commands, all programs should call glFlush
**  whenever they count on having all of their previously issued commands
**  completed. For example, call glFlush before waiting for user input that
**  depends on the generated image.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glFlush(
    void
    )
{
    glmENTER()
    {
		gcmDUMP_API("${ES11 glFlush}");

		glmPROFILE(context, GLES1_FLUSH, 0);
        /* Flush the frame buffer. */
        if (gcmIS_ERROR(gcoSURF_Flush(context->draw)))
		{
			glmERROR(GL_INVALID_OPERATION);
			break;
		}

        /* Commit command buffer. */
        if (gcmIS_ERROR(gcoHAL_Commit(context->hal, gcvFALSE)))
		{
			glmERROR(GL_INVALID_OPERATION);
			break;
		}
    }
    glmLEAVE();
}


/*******************************************************************************
**
**  glFinish
**
**  glFinish does not return until the effects of all previously called GL
**  commands are complete. Such effects include all changes to GL state,
**  all changes to connection state, and all changes to the frame buffer
**  contents.
**
**  INPUT:
**
**      Nothing.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glFinish(
    void
    )
{
    glmENTER()
    {
		gcmDUMP_API("${ES11 glFinish}");

		glmPROFILE(context, GLES1_FINISH, 0);
        /* Flush the cache. */
        if (gcmIS_ERROR(gcoSURF_Flush(context->draw)))
		{
			glmERROR(GL_INVALID_OPERATION);
			break;
		}

        /* Commit command buffer. */
        if (gcmIS_ERROR(gcoHAL_Commit(context->hal, gcvTRUE)))
		{
			glmERROR(GL_INVALID_OPERATION);
			break;
		}
    }
    glmLEAVE();
}

glsCONTEXT_PTR
GetCurrentContext(void)
{
    glsCONTEXT_PTR context;
    gcmHEADER();
    context = veglGetCurrentAPIContext();
    gcmFOOTER_ARG("result=0x%x", context);
    return context;
}

static veglLOOKUP _glaLookup[] =
{
    eglMAKE_LOOKUP(glActiveTexture),
    eglMAKE_LOOKUP(glAlphaFuncx),
    eglMAKE_LOOKUP(glAlphaFuncxOES),
    eglMAKE_LOOKUP(glBindBuffer),
    eglMAKE_LOOKUP(glBindFramebufferOES),
    eglMAKE_LOOKUP(glBindRenderbufferOES),
    eglMAKE_LOOKUP(glBindTexture),
    eglMAKE_LOOKUP(glBlendFunc),
	eglMAKE_LOOKUP(glBlendFuncSeparateOES),
	eglMAKE_LOOKUP(glBlendEquationOES),
	eglMAKE_LOOKUP(glBlendEquationSeparateOES),
    eglMAKE_LOOKUP(glBufferData),
    eglMAKE_LOOKUP(glBufferSubData),
    eglMAKE_LOOKUP(glCheckFramebufferStatusOES),
    eglMAKE_LOOKUP(glClear),
    eglMAKE_LOOKUP(glClearColorx),
    eglMAKE_LOOKUP(glClearColorxOES),
    eglMAKE_LOOKUP(glClearDepthx),
    eglMAKE_LOOKUP(glClearDepthxOES),
    eglMAKE_LOOKUP(glClearStencil),
    eglMAKE_LOOKUP(glClientActiveTexture),
    eglMAKE_LOOKUP(glClipPlanex),
    eglMAKE_LOOKUP(glClipPlanexOES),
    eglMAKE_LOOKUP(glClipPlanexIMG),
    eglMAKE_LOOKUP(glColor4ub),
    eglMAKE_LOOKUP(glColor4x),
    eglMAKE_LOOKUP(glColor4xOES),
    eglMAKE_LOOKUP(glColorMask),
    eglMAKE_LOOKUP(glColorPointer),
    eglMAKE_LOOKUP(glCompressedTexImage2D),
    eglMAKE_LOOKUP(glCompressedTexSubImage2D),
    eglMAKE_LOOKUP(glCopyTexImage2D),
    eglMAKE_LOOKUP(glCopyTexSubImage2D),
    eglMAKE_LOOKUP(glCullFace),
    eglMAKE_LOOKUP(glCurrentPaletteMatrixOES),
    eglMAKE_LOOKUP(glDeleteBuffers),
    eglMAKE_LOOKUP(glDeleteFramebuffersOES),
    eglMAKE_LOOKUP(glDeleteRenderbuffersOES),
    eglMAKE_LOOKUP(glDeleteTextures),
    eglMAKE_LOOKUP(glDepthFunc),
    eglMAKE_LOOKUP(glDepthMask),
    eglMAKE_LOOKUP(glDepthRangex),
    eglMAKE_LOOKUP(glDepthRangexOES),
    eglMAKE_LOOKUP(glDisable),
    eglMAKE_LOOKUP(glDisableClientState),
    eglMAKE_LOOKUP(glDrawArrays),
    eglMAKE_LOOKUP(glDrawElements),
    eglMAKE_LOOKUP(glDrawTexiOES),
    eglMAKE_LOOKUP(glDrawTexivOES),
    eglMAKE_LOOKUP(glDrawTexsOES),
    eglMAKE_LOOKUP(glDrawTexsvOES),
    eglMAKE_LOOKUP(glDrawTexxOES),
    eglMAKE_LOOKUP(glDrawTexxvOES),
    eglMAKE_LOOKUP(glEGLImageTargetRenderbufferStorageOES),
    eglMAKE_LOOKUP(glEGLImageTargetTexture2DOES),
    eglMAKE_LOOKUP(glEnable),
    eglMAKE_LOOKUP(glEnableClientState),
    eglMAKE_LOOKUP(glFinish),
    eglMAKE_LOOKUP(glFlush),
    eglMAKE_LOOKUP(glFogx),
    eglMAKE_LOOKUP(glFogxOES),
    eglMAKE_LOOKUP(glFogxv),
    eglMAKE_LOOKUP(glFogxvOES),
    eglMAKE_LOOKUP(glFramebufferRenderbufferOES),
    eglMAKE_LOOKUP(glFramebufferTexture2DOES),
    eglMAKE_LOOKUP(glFrontFace),
    eglMAKE_LOOKUP(glFrustumx),
    eglMAKE_LOOKUP(glFrustumxOES),
    eglMAKE_LOOKUP(glGenBuffers),
    eglMAKE_LOOKUP(glGenerateMipmapOES),
    eglMAKE_LOOKUP(glGenFramebuffersOES),
    eglMAKE_LOOKUP(glGenRenderbuffersOES),
    eglMAKE_LOOKUP(glGenTextures),
    eglMAKE_LOOKUP(glGetBooleanv),
    eglMAKE_LOOKUP(glGetBufferParameteriv),
    eglMAKE_LOOKUP(glGetClipPlanex),
    eglMAKE_LOOKUP(glGetClipPlanexOES),
    eglMAKE_LOOKUP(glGetError),
    eglMAKE_LOOKUP(glGetFixedv),
    eglMAKE_LOOKUP(glGetFixedvOES),
    eglMAKE_LOOKUP(glGetFramebufferAttachmentParameterivOES),
    eglMAKE_LOOKUP(glGetIntegerv),
    eglMAKE_LOOKUP(glGetLightxv),
    eglMAKE_LOOKUP(glGetLightxvOES),
    eglMAKE_LOOKUP(glGetMaterialxv),
    eglMAKE_LOOKUP(glGetMaterialxvOES),
    eglMAKE_LOOKUP(glGetPointerv),
    eglMAKE_LOOKUP(glGetRenderbufferParameterivOES),
    eglMAKE_LOOKUP(glGetString),
    eglMAKE_LOOKUP(glGetTexEnviv),
    eglMAKE_LOOKUP(glGetTexEnvxv),
    eglMAKE_LOOKUP(glGetTexEnvxvOES),
    eglMAKE_LOOKUP(glGetTexGenivOES),
    eglMAKE_LOOKUP(glGetTexGenxvOES),
    eglMAKE_LOOKUP(glGetTexParameteriv),
    eglMAKE_LOOKUP(glGetTexParameterxv),
    eglMAKE_LOOKUP(glGetTexParameterxvOES),
    eglMAKE_LOOKUP(glHint),
    eglMAKE_LOOKUP(glIsBuffer),
    eglMAKE_LOOKUP(glIsEnabled),
    eglMAKE_LOOKUP(glIsFramebufferOES),
    eglMAKE_LOOKUP(glIsRenderbufferOES),
    eglMAKE_LOOKUP(glIsTexture),
    eglMAKE_LOOKUP(glLightModelx),
    eglMAKE_LOOKUP(glLightModelxOES),
    eglMAKE_LOOKUP(glLightModelxv),
    eglMAKE_LOOKUP(glLightModelxvOES),
    eglMAKE_LOOKUP(glLightx),
    eglMAKE_LOOKUP(glLightxOES),
    eglMAKE_LOOKUP(glLightxv),
    eglMAKE_LOOKUP(glLightxvOES),
    eglMAKE_LOOKUP(glLineWidthx),
    eglMAKE_LOOKUP(glLineWidthxOES),
    eglMAKE_LOOKUP(glLoadIdentity),
    eglMAKE_LOOKUP(glLoadMatrixx),
    eglMAKE_LOOKUP(glLoadMatrixxOES),
    eglMAKE_LOOKUP(glLoadPaletteFromModelViewMatrixOES),
    eglMAKE_LOOKUP(glLogicOp),
    eglMAKE_LOOKUP(glMaterialx),
    eglMAKE_LOOKUP(glMaterialxOES),
    eglMAKE_LOOKUP(glMaterialxv),
    eglMAKE_LOOKUP(glMaterialxvOES),
    eglMAKE_LOOKUP(glMatrixIndexPointerOES),
    eglMAKE_LOOKUP(glMatrixMode),
    eglMAKE_LOOKUP(glMultMatrixx),
    eglMAKE_LOOKUP(glMultMatrixxOES),
    eglMAKE_LOOKUP(glMultiDrawArraysEXT),
    eglMAKE_LOOKUP(glMultiDrawElementsEXT),
    eglMAKE_LOOKUP(glMultiTexCoord4x),
    eglMAKE_LOOKUP(glMultiTexCoord4xOES),
    eglMAKE_LOOKUP(glNormal3x),
    eglMAKE_LOOKUP(glNormal3xOES),
    eglMAKE_LOOKUP(glNormalPointer),
    eglMAKE_LOOKUP(glOrthox),
    eglMAKE_LOOKUP(glOrthoxOES),
    eglMAKE_LOOKUP(glPixelStorei),
    eglMAKE_LOOKUP(glPointParameterx),
    eglMAKE_LOOKUP(glPointParameterxOES),
    eglMAKE_LOOKUP(glPointParameterxv),
    eglMAKE_LOOKUP(glPointParameterxvOES),
    eglMAKE_LOOKUP(glPointSizePointerOES),
    eglMAKE_LOOKUP(glPointSizex),
    eglMAKE_LOOKUP(glPointSizexOES),
    eglMAKE_LOOKUP(glPolygonOffsetx),
    eglMAKE_LOOKUP(glPolygonOffsetxOES),
    eglMAKE_LOOKUP(glPopMatrix),
    eglMAKE_LOOKUP(glPushMatrix),
    eglMAKE_LOOKUP(glQueryMatrixxOES),
    eglMAKE_LOOKUP(glReadPixels),
    eglMAKE_LOOKUP(glRenderbufferStorageOES),
    eglMAKE_LOOKUP(glRotatex),
    eglMAKE_LOOKUP(glRotatexOES),
    eglMAKE_LOOKUP(glSampleCoveragex),
    eglMAKE_LOOKUP(glSampleCoveragexOES),
    eglMAKE_LOOKUP(glScalex),
    eglMAKE_LOOKUP(glScalexOES),
    eglMAKE_LOOKUP(glScissor),
    eglMAKE_LOOKUP(glShadeModel),
    eglMAKE_LOOKUP(glStencilFunc),
    eglMAKE_LOOKUP(glStencilMask),
    eglMAKE_LOOKUP(glStencilOp),
    eglMAKE_LOOKUP(glTexCoordPointer),
    eglMAKE_LOOKUP(glTexDirectInvalidateVIV),
    eglMAKE_LOOKUP(glTexDirectVIV),
    eglMAKE_LOOKUP(glTexDirectVIVMap),
    eglMAKE_LOOKUP(glTexEnvi),
    eglMAKE_LOOKUP(glTexEnviv),
    eglMAKE_LOOKUP(glTexEnvx),
    eglMAKE_LOOKUP(glTexEnvxOES),
    eglMAKE_LOOKUP(glTexEnvxv),
    eglMAKE_LOOKUP(glTexEnvxvOES),
    eglMAKE_LOOKUP(glTexGeniOES),
    eglMAKE_LOOKUP(glTexGenivOES),
    eglMAKE_LOOKUP(glTexGenxOES),
    eglMAKE_LOOKUP(glTexGenxvOES),
    eglMAKE_LOOKUP(glTexImage2D),
    eglMAKE_LOOKUP(glTexParameteri),
    eglMAKE_LOOKUP(glTexParameteriv),
    eglMAKE_LOOKUP(glTexParameterx),
    eglMAKE_LOOKUP(glTexParameterxOES),
    eglMAKE_LOOKUP(glTexParameterxv),
    eglMAKE_LOOKUP(glTexParameterxvOES),
    eglMAKE_LOOKUP(glTexSubImage2D),
    eglMAKE_LOOKUP(glTranslatex),
    eglMAKE_LOOKUP(glTranslatexOES),
    eglMAKE_LOOKUP(glVertexPointer),
    eglMAKE_LOOKUP(glViewport),
    eglMAKE_LOOKUP(glWeightPointerOES),
	eglMAKE_LOOKUP(glBindBufferARB),
	eglMAKE_LOOKUP(glBufferDataARB),
	eglMAKE_LOOKUP(glBufferSubDataARB),
	eglMAKE_LOOKUP(glDeleteBuffersARB),
	eglMAKE_LOOKUP(glGenBuffersARB),
	eglMAKE_LOOKUP(glGetBufferParameterivARB),
#ifndef COMMON_LITE
    eglMAKE_LOOKUP(glAlphaFunc),
    eglMAKE_LOOKUP(glClearColor),
    eglMAKE_LOOKUP(glClearDepthf),
    eglMAKE_LOOKUP(glClearDepthfOES),
    eglMAKE_LOOKUP(glClipPlanef),
    eglMAKE_LOOKUP(glClipPlanefOES),
    eglMAKE_LOOKUP(glClipPlanefIMG),
    eglMAKE_LOOKUP(glColor4f),
    eglMAKE_LOOKUP(glDepthRangef),
    eglMAKE_LOOKUP(glDepthRangefOES),
    eglMAKE_LOOKUP(glDrawTexfOES),
    eglMAKE_LOOKUP(glDrawTexfvOES),
    eglMAKE_LOOKUP(glFogf),
    eglMAKE_LOOKUP(glFogfv),
    eglMAKE_LOOKUP(glFrustumf),
    eglMAKE_LOOKUP(glFrustumfOES),
    eglMAKE_LOOKUP(glGetClipPlanef),
    eglMAKE_LOOKUP(glGetClipPlanefOES),
    eglMAKE_LOOKUP(glGetFloatv),
    eglMAKE_LOOKUP(glGetLightfv),
    eglMAKE_LOOKUP(glGetMaterialfv),
    eglMAKE_LOOKUP(glGetTexEnvfv),
    eglMAKE_LOOKUP(glGetTexGenfvOES),
    eglMAKE_LOOKUP(glGetTexParameterfv),
    eglMAKE_LOOKUP(glLightModelf),
    eglMAKE_LOOKUP(glLightModelfv),
    eglMAKE_LOOKUP(glLightf),
    eglMAKE_LOOKUP(glLightfv),
    eglMAKE_LOOKUP(glLineWidth),
    eglMAKE_LOOKUP(glLoadMatrixf),
    eglMAKE_LOOKUP(glMaterialf),
    eglMAKE_LOOKUP(glMaterialfv),
    eglMAKE_LOOKUP(glMultMatrixf),
    eglMAKE_LOOKUP(glMultiTexCoord4f),
    eglMAKE_LOOKUP(glNormal3f),
    eglMAKE_LOOKUP(glOrthof),
    eglMAKE_LOOKUP(glOrthofOES),
    eglMAKE_LOOKUP(glPointParameterf),
    eglMAKE_LOOKUP(glPointParameterfv),
    eglMAKE_LOOKUP(glPointSize),
    eglMAKE_LOOKUP(glPolygonOffset),
    eglMAKE_LOOKUP(glRotatef),
    eglMAKE_LOOKUP(glSampleCoverage),
    eglMAKE_LOOKUP(glScalef),
    eglMAKE_LOOKUP(glTexEnvf),
    eglMAKE_LOOKUP(glTexEnvfv),
    eglMAKE_LOOKUP(glTexGenfOES),
    eglMAKE_LOOKUP(glTexGenfvOES),
    eglMAKE_LOOKUP(glTexParameterf),
    eglMAKE_LOOKUP(glTexParameterfv),
    eglMAKE_LOOKUP(glTranslatef),
#endif

    eglMAKE_LOOKUP(glMapBufferOES),
    eglMAKE_LOOKUP(glUnmapBufferOES),
    eglMAKE_LOOKUP(glGetBufferPointervOES),
    { gcvNULL, gcvNULL }
};

GL_API veglDISPATCH
#ifdef COMMON_LITE
GLES_CL_DISPATCH_TABLE
#else
GLES_CM_DISPATCH_TABLE
#endif
=
{
    /* createContext            */  glfCreateContext,
    /* destroyContext           */  glfDestroyContext,
    /* setContext               */  glfSetContext,
    /* flushContext             */  glfFlushContext,

    /* flush                    */  glfFlush,
    /* finish                   */  glfFinish,

    /* setBuffer                */  gcvNULL,
    /* getClientBuffer          */  gcvNULL,

    /* createImageTexture       */  glfCreateImageTexture,
    /* createImageRenderbuffer  */  glfCreateImageRenderBuffer,
    /* createImageParentImage   */  gcvNULL,
    /* bindTexImage             */  glfBindTexImage,

#if VIVANTE_PROFILER
    /* profiler                 */  _glffProfiler,
#else
    /* profiler                 */  gcvNULL,
#endif
    /* lookup                   */  _glaLookup,

    /* queryHWVG                */  gcvNULL,

#if gcdRENDER_THREADS
    /* renderThread             */  glfRenderThread,
#else
    /* renderThread             */  gcvNULL,
#endif
};
