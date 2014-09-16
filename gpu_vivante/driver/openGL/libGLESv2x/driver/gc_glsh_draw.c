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

/* Define the sizes for the dynamic arrays. */
#define gldDYNAMIC_ARRAY_SIZE       (64 << 10)
#define gldDYNAMIC_ARRAY_COUNT      2
#define gldDYNAMIC_ELEMENT_SIZE     (16 << 10)
#define gldDYNAMIC_ELEMENT_COUNT    2

#define gldBATCH_OPTIMIZATION       2

void
_glshInitializeDraw(
    GLContext Context
    )
{
    gceSTATUS status;

#if !GL_USE_VERTEX_ARRAY
    /* Initialize vertex stuff. */
    Context->vertex            = gcvNULL;
    Context->dynamicArray      = gcvNULL;
    Context->dynamicElement    = gcvNULL;
    Context->lastPrimitiveType = (gcePRIMITIVE) -1;

    /* Construct gcoVERTEX object. */
    gcmONERROR(gcoVERTEX_Construct(Context->hal,
                                   &Context->vertex));

    /* Construct gcoSTREAM object for dynamic arrays. */
    gcmONERROR(gcoSTREAM_Construct(Context->hal,
                                   &Context->dynamicArray));

    /* Mark it as dynamic. */
    gcmONERROR(gcoSTREAM_SetDynamic(Context->dynamicArray,
                                    gldDYNAMIC_ARRAY_SIZE,
                                    gldDYNAMIC_ARRAY_COUNT));

    /* Construct gcoINDEX object for dynamic elements. */
    gcmONERROR(gcoINDEX_Construct(Context->hal,
                                  &Context->dynamicElement));

    /* Mark it as dynamic. */
    gcmONERROR(gcoINDEX_SetDynamic(Context->dynamicElement,
                                   gldDYNAMIC_ELEMENT_SIZE,
                                   gldDYNAMIC_ELEMENT_COUNT));
#endif

    /* Initialize last primitive type. */
    Context->lastPrimitiveType = (gcePRIMITIVE) -1;

    /* Zero the bounded texture sampler. */
    gcmONERROR(gcoOS_ZeroMemory(Context->bounded, gcmSIZEOF(Context->bounded)));

    /* Initialize batch. */
    Context->batchDirty     = GL_TRUE;
    Context->batchMode      = GL_NONE;
    Context->batchFirst     = 0;
    Context->batchCount     = 0;
    Context->batchIndexType = GL_NONE;
    Context->batchPointer   = gcvNULL;
    Context->batchEnable    = 0;
    Context->batchArray     = 0;
    Context->batchSkip      = 0;

    return;

OnError:
    /* Destroy everything just created. */
    _glshDeinitializeDraw(Context);

    /* Error. */
    gl2mERROR(GL_OUT_OF_MEMORY);
}

void _glshDeinitializeDraw(
    GLContext Context
    )
{
#if !GL_USE_VERTEX_ARRAY
    /* Destroy the gcoVERTEX object. */
    if (Context->vertex != gcvNULL)
    {
        gcmVERIFY_OK(gcoVERTEX_Destroy(Context->vertex));
    }

    /* Destroy the dynamic arrays gcoSTREAM object. */
    if (Context->dynamicArray != gcvNULL)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(Context->dynamicArray));
    }

    /* Destroy the dynamic elements gcoINDEX object. */
    if (Context->dynamicElement != gcvNULL)
    {
        gcmVERIFY_OK(gcoINDEX_Destroy(Context->dynamicElement));
    }
#endif
}

#if gcdDUMP_API
static void
_DumpVertices(
    IN GLContext Context,
    IN GLint First,
    IN GLsizei Count
    )
{
    GLuint i;
#if GL_USE_VERTEX_ARRAY
    gcsVERTEXARRAY *vertexArray;
#else
    GLAttribute *vertexArray;
#endif

    if(Context->vertexArrayObject == gcvNULL)
    {
        vertexArray = Context->vertexArray;
    }
    else
    {
        vertexArray = Context->vertexArrayObject->vertexArray;
    }

    /* Walk all vertices. */
    for (i = 0; i < Context->maxAttributes; ++i)
    {
#if GL_USE_VERTEX_ARRAY
        if ((vertexArray[i].enable)
        &&  (vertexArray[i].stream == gcvNULL)
        &&  (vertexArray[i].pointer != gcvNULL)
        )
        {
            gcmDUMP_API_DATA((gctUINT8_PTR) vertexArray[i].pointer
                             + First * vertexArray[i].stride,
                             Count * vertexArray[i].stride);
        }
#else
        if (vertexArray[i].enable
        &&  (vertexArray[i].ptr != gcvNULL)
        )
        {
            gcmDUMP_API_DATA((gctUINT8_PTR) vertexArray[i].ptr
                             + First * vertexArray[i].stride,
                             Count * vertexArray[i].stride);
        }
#endif
    }
}
#endif

#if gcdNULL_DRIVER < 3
static gceSTATUS
_DrawPrimitives(
    IN GLContext Context,
    IN gcePRIMITIVE PrimitiveType,
    IN GLint First,
    IN GLsizei PrimitiveCount,
    IN gctBOOL Indexed
    )
{
    gceSTATUS status;
    gcoSURF depth;
    gctUINT width, height;
    gctINT depthBytes;
    gcoSURF savedZ = gcvNULL;
    gctUINT32_PTR savedZLogical = gcvNULL, savedZPtr;
    gcoSURF stencilZ = gcvNULL;
    gctUINT32_PTR stencilZLogical = gcvNULL, stencilZPtr;
    gcoSURF failedZ = gcvNULL;
    gctUINT32_PTR failedZLogical = gcvNULL, failedZPtr;
    gctINT offset, i;
    GLenum origCullMode = 0;
    GLboolean revertCullMode = GL_FALSE;
    gctPOINTER pointer[3] = {gcvNULL};

    /*
        Plans for acceleration:

        1)  Resolve depth into save texture.
        2)  Resolve depth into failed stencil buffer.
        3)  Render triangle as-is.

        4)  Change stencil to ZERO,ZERO,INCR_SAT and write=~0.
        5)  Set colorwrite to 0.
        6)  Render triangle into failed stencil buffer.

        7)  Change stencil to KEEP,KEEP,KEEP, mask=~0, NON_ZERO.
        8)  Disable depth write.
        9)  Set color write to 0xE.
        10) Reprogram pixel shader to:
                mul r1.xy, r0.xy, c<size>.xy
                texld r1, r1xy, s<save>
        11) Program texture with save texture.
        12) Render triangle into depth buffer (as a render target).

        13) Reset stencil, color, and depth, pixel shader, texture,
            color/depth write.

        Issues:

        1)  HAL need "patch" function with texture and constant input.
        2)  Need a "shader reset" function that uses the hints to just reprogram
            the 2 overwritten pixel shader instructions.
        3)  Make sure we can set a depth buffer as a render target (with tiling,
            copression, etc.).
    */

    /* Test if stencil is enabled and either front or back is not ALWAYS. */
    if (Context->stencilEnable
    &&  Context->depthTest
    &&  Context->depthMask
    &&  (!Context->hasCorrectStencil)
    &&  (  (Context->stencilFuncFront != GL_ALWAYS)
        || (Context->stencilFuncBack  != GL_ALWAYS)
        )
    )
    {
        /* Stencil bug work around.  We actually get a perfect stencil buffer,
        ** however, the depth buffer is wrong when stencil fails. */
        depth = (Context->framebuffer == gcvNULL)
              ? Context->depth
              : (Context->framebuffer->depth.target == gcvNULL)
              ? Context->framebuffer->depth.surface
              : Context->framebuffer->depth.target;
    }
    else
    {
        /* No need to work around the hardware bug. */
        depth = gcvNULL;
    }

    if (depth == gcvNULL)
    {
        if (Indexed)
        {
            /* Render indexed primitives. */
            gcmONERROR(
                gco3D_DrawIndexedPrimitives(Context->engine,
                                            PrimitiveType,
                                            0,
                                            First,
                                            PrimitiveCount));
        }
        else
        {
            /* Render primitives. */
            gcmONERROR(
                gco3D_DrawPrimitives(Context->engine,
                                     PrimitiveType,
                                     First,
                                     PrimitiveCount));
        }

        /* Success. */
        return gcvSTATUS_OK;
    }

    /* TODO: We only support a fan of 1 triangle. */
    gcmASSERT(  (PrimitiveType  != gcvPRIMITIVE_TRIANGLE_FAN)
             || (PrimitiveCount != 1)
             );

    /* Disable the tile status on the current depth buffer. */
    gcmONERROR(
        gcoSURF_DisableTileStatus(depth, gcvTRUE));

    /* Get the current size of the depth buffer. */
    gcmONERROR(
        gcoSURF_GetAlignedSize(depth, gcvNULL, &height, &depthBytes));

    depthBytes *= height;

    gcmONERROR(
        gcoSURF_GetSize(depth, &width, &height, gcvNULL));

    /* Create saved depth buffer and lock it. */
    status = gcoSURF_Construct(Context->hal,
                               width, height, 1,
                               gcvSURF_DEPTH_NO_TILE_STATUS,
                               gcvSURF_D24S8,
                               gcvPOOL_DEFAULT,
                               &savedZ);

    /* Out of memory - stall pipe and retry. */
    if (status == gcvSTATUS_OUT_OF_MEMORY)
    {
        gcmONERROR(
            gcoHAL_Commit(Context->hal, gcvTRUE));

        gcmONERROR(
            gcoSURF_Construct(Context->hal,
                               width, height, 1,
                               gcvSURF_DEPTH_NO_TILE_STATUS,
                               gcvSURF_D24S8,
                               gcvPOOL_DEFAULT,
                               &savedZ));
    }

    /* If MultiSample happens */
    gcmONERROR(gcoSURF_SetSamples(savedZ,Context->samples));

    gcmONERROR(gcoSURF_SetOrientation(savedZ, gcvORIENTATION_BOTTOM_TOP));

    gcmONERROR(gcoSURF_Lock(savedZ, gcvNULL, pointer));

    savedZLogical = pointer[0];

    /* Create stencil buffer. */
    status = gcoSURF_Construct(Context->hal,
                               width, height, 1,
                               gcvSURF_DEPTH_NO_TILE_STATUS,
                               gcvSURF_D24S8,
                               gcvPOOL_DEFAULT,
                               &stencilZ);

    /* Out of memory - stall pipe and retry. */
    if (status == gcvSTATUS_OUT_OF_MEMORY)
    {
        gcmONERROR(
            gcoHAL_Commit(Context->hal, gcvTRUE));

        gcmONERROR(
            gcoSURF_Construct(Context->hal,
                              width, height, 1,
                              gcvSURF_DEPTH_NO_TILE_STATUS,
                              gcvSURF_D24S8,
                              gcvPOOL_DEFAULT,
                              &stencilZ));
    }

    /* If MultiSample happens */
    gcmONERROR(gcoSURF_SetSamples(stencilZ,Context->samples));

    gcmONERROR(
        gcoSURF_SetOrientation(stencilZ, gcvORIENTATION_BOTTOM_TOP));

    gcmONERROR(gcoSURF_Lock(stencilZ, gcvNULL, pointer));

    stencilZLogical = pointer[0];

    /* Create stencil fail depth buffer. */
    status = gcoSURF_Construct(Context->hal,
                               width, height, 1,
                               gcvSURF_DEPTH_NO_TILE_STATUS,
                               gcvSURF_D24S8,
                               gcvPOOL_DEFAULT,
                               &failedZ);

    /* Out of memory - stall pipe and retry. */
    if (status == gcvSTATUS_OUT_OF_MEMORY)
    {
        gcmONERROR(
            gcoHAL_Commit(Context->hal, gcvTRUE));

        gcmONERROR(
            gcoSURF_Construct(Context->hal,
                              width, height, 1,
                              gcvSURF_DEPTH_NO_TILE_STATUS,
                              gcvSURF_D24S8,
                              gcvPOOL_DEFAULT,
                              &failedZ));
    }
    /* If MultiSample happens */
    gcmONERROR(gcoSURF_SetSamples(failedZ,Context->samples));

    gcmONERROR(
        gcoSURF_SetOrientation(failedZ, gcvORIENTATION_BOTTOM_TOP));

    gcmONERROR(gcoSURF_Lock(failedZ, gcvNULL, pointer));

    failedZLogical = pointer[0];

    /* Loop through all primitives. */
    for (offset = 0; PrimitiveCount > 0; --PrimitiveCount)
    {
        gcmONERROR(
            gcoSURF_CPUCacheOperation(savedZ, gcvCACHE_FLUSH));

        gcmONERROR(
            gcoSURF_CPUCacheOperation(stencilZ, gcvCACHE_FLUSH));

        gcmONERROR(
            gcoSURF_CPUCacheOperation(failedZ, gcvCACHE_FLUSH));

        /* for triangle strip case, when we split it to single triangles
         * the odd offset triangles' vertexes are not match the spec order,
         * it cause the face is reverted, if the cull is enabled, hw will cull
         * the wrong face, so we need add a workaround for it to revert the cull mode
         * to avoid cull the wrong face. */
        if (PrimitiveType == gcvPRIMITIVE_TRIANGLE_STRIP &&
            Context->cullEnable &&
            (offset%2)==1)
        {
            if (Context->cullMode  == GL_FRONT)
            {
                revertCullMode = GL_TRUE;
                origCullMode = GL_FRONT;
                glCullFace(GL_BACK);
            }
            else if (Context->cullMode == GL_BACK)
            {
                revertCullMode = GL_TRUE;
                origCullMode = GL_BACK;
                glCullFace(GL_FRONT);
            }
        }


        /* Copy depth buffer to saved depth buffer. */
        gcmONERROR(
            gcoSURF_Resolve(depth, savedZ));

        /* Copy depth buffer to stencil depth buffer. */
        gcmONERROR(
            gcoSURF_Resolve(depth, stencilZ));

        /* Copy depth buffer to failed depth buffer. */
        gcmONERROR(
            gcoSURF_Resolve(depth, failedZ));

        /* Set stencil buffer as depth buffer. */
        gcmONERROR(
            gco3D_SetDepth(Context->engine, stencilZ));

        /* Render 1 triangle. */
        if (Indexed)
        {
            /* Render indexed primitives. */
            gcmONERROR(
                gco3D_DrawIndexedPrimitives(Context->engine,
                                            PrimitiveType,
                                            0,
                                            First + offset,
                                            1));
        }
        else
        {
            /* Render primitives. */
            gcmONERROR(
                gco3D_DrawPrimitives(Context->engine,
                                     PrimitiveType,
                                     First + offset,
                                     1));
        }

        /* Flush the stencil surface. */
        gcmONERROR(
            gcoSURF_Flush(stencilZ));

        /* Set failed depth buffer. */
        gcmONERROR(
            gco3D_SetDepth(Context->engine, failedZ));

        /* Disable color write. */
        gcmONERROR(
            gco3D_SetColorWrite(Context->engine, 0));

        /* Set stencil to NONE,NONE,INCR_SAT. */
        gcmONERROR(
            gco3D_SetStencilPass(Context->engine,
                                 gcvSTENCIL_FRONT,
                                 gcvSTENCIL_ZERO));
        gcmONERROR(
            gco3D_SetStencilPass(Context->engine,
                                 gcvSTENCIL_BACK,
                                 gcvSTENCIL_ZERO));
        gcmONERROR(
            gco3D_SetStencilDepthFail(Context->engine,
                                      gcvSTENCIL_FRONT,
                                      gcvSTENCIL_ZERO));
        gcmONERROR(
            gco3D_SetStencilDepthFail(Context->engine,
                                      gcvSTENCIL_BACK,
                                      gcvSTENCIL_ZERO));
        gcmONERROR(
            gco3D_SetStencilFail(Context->engine,
                                 gcvSTENCIL_FRONT,
                                 gcvSTENCIL_INCREMENT_SATURATE));
        gcmONERROR(
            gco3D_SetStencilFail(Context->engine,
                                 gcvSTENCIL_BACK,
                                 gcvSTENCIL_INCREMENT_SATURATE));

        /* Set stencil write mask to 0xFF. */
        gcmONERROR(
            gco3D_SetStencilWriteMask(Context->engine, 0xFF));

        /* Render 1 triangle. */
        if (Indexed)
        {
            /* Render indexed primitives. */
            gcmONERROR(
                gco3D_DrawIndexedPrimitives(Context->engine,
                                            PrimitiveType,
                                            0,
                                            First + offset,
                                            1));
        }
        else
        {
            /* Render primitives. */
            gcmONERROR(
                gco3D_DrawPrimitives(Context->engine,
                                     PrimitiveType,
                                     First + offset,
                                     1));
        }

        /* Flush the failed depth surface. */
        gcmONERROR(
            gcoSURF_Flush(failedZ));

        /* Commit and stall the hardware. */
        gcmONERROR(
            gcoHAL_Commit(Context->hal, gcvTRUE));

        /* Loop through all pixels. */
        savedZPtr   = savedZLogical;
        stencilZPtr = stencilZLogical;
        failedZPtr  = failedZLogical;
        for (i = 0; i < depthBytes; i += 4)
        {
            if (*failedZPtr & 0xFF)
            {
                *stencilZPtr = (*stencilZPtr & 0x000000FF)
                             | (*savedZPtr   & 0xFFFFFF00);
            }

            ++ savedZPtr;
            ++ stencilZPtr;
            ++ failedZPtr;
        }

        gcmONERROR(
            gcoSURF_CPUCacheOperation(stencilZ, gcvCACHE_CLEAN));

        /* set back the reverted cull mode */
        if (revertCullMode == GL_TRUE)
        {
            revertCullMode = GL_FALSE;
            glCullFace(origCullMode);
        }

        /* Resolve stencil depth buffer to depth buffer. */
        gcmONERROR(
            gcoSURF_Resolve(stencilZ, depth));

        /* Reset depth buffer. */
        gcmONERROR(
            gco3D_SetDepth(Context->engine, depth));

        /* Reset color write. */
        gcmONERROR(
            gco3D_SetColorWrite(Context->engine, Context->colorWrite));

        /* Reset stencil. */
        gcmONERROR(
            gco3D_SetStencilPass(Context->engine,
                                 gcvSTENCIL_FRONT,
                                 Context->frontZPass));
        gcmONERROR(
            gco3D_SetStencilPass(Context->engine,
                                 gcvSTENCIL_BACK,
                                 Context->backZPass));
        gcmONERROR(
            gco3D_SetStencilDepthFail(Context->engine,
                                      gcvSTENCIL_FRONT,
                                      Context->frontZFail));
        gcmONERROR(
            gco3D_SetStencilDepthFail(Context->engine,
                                      gcvSTENCIL_BACK,
                                      Context->backZFail));
        gcmONERROR(
            gco3D_SetStencilFail(Context->engine,
                                 gcvSTENCIL_FRONT,
                                 Context->frontFail));
        gcmONERROR(
            gco3D_SetStencilFail(Context->engine,
                                 gcvSTENCIL_BACK,
                                 Context->backFail));
        gcmONERROR(
            gco3D_SetStencilWriteMask(Context->engine,
                                      (gctUINT8) Context->stencilWriteMask));

        /* Next primitive. */
        switch (PrimitiveType)
        {
        default:
            offset += 1;
            break;

        case gcvPRIMITIVE_LINE_LIST:
            offset += 2;
            break;

        case gcvPRIMITIVE_TRIANGLE_LIST:
            offset += 3;
            break;
        }
    }

    /* Success. */
    status = gcvSTATUS_OK;

OnError:
    if (savedZ != gcvNULL)
    {
        if (savedZLogical != gcvNULL)
        {
            /* Unlock the saved depth buffer. */
            gcmVERIFY_OK(
                gcoSURF_Unlock(savedZ, savedZLogical));
        }

        /* Destroy saved depth buffer. */
        gcmVERIFY_OK(
            gcoSURF_Destroy(savedZ));
    }

    if (stencilZ != gcvNULL)
    {
        if (stencilZLogical != gcvNULL)
        {
            /* Unlock the stencil depth buffer. */
            gcmVERIFY_OK(
                gcoSURF_Unlock(stencilZ, stencilZLogical));
        }

        /* Destroy stencil depth buffer. */
        gcmVERIFY_OK(
            gcoSURF_Destroy(stencilZ));
    }

    if (failedZ != gcvNULL)
    {
        if (failedZLogical != gcvNULL)
        {
            /* Unlock the failed depth buffer. */
            gcmVERIFY_OK(
                gcoSURF_Unlock(failedZ, failedZLogical));
        }

        /* Destroy failed depth buffer. */
        gcmVERIFY_OK(
            gcoSURF_Destroy(failedZ));
    }

    if ((depth != gcvNULL) && gcmIS_ERROR(status))
    {
        /* Reset depth buffer. */
        gcmVERIFY_OK(
            gco3D_SetDepth(Context->engine, depth));

        /* Reset color write. */
        gcmVERIFY_OK(
            gco3D_SetColorWrite(Context->engine, Context->colorWrite));

        /* Reset stencil. */
        gcmVERIFY_OK(
            gco3D_SetStencilPass(Context->engine,
                                 gcvSTENCIL_FRONT,
                                 Context->frontZPass));
        gcmVERIFY_OK(
            gco3D_SetStencilPass(Context->engine,
                                 gcvSTENCIL_BACK,
                                 Context->backZPass));
        gcmVERIFY_OK(
            gco3D_SetStencilDepthFail(Context->engine,
                                      gcvSTENCIL_FRONT,
                                      Context->frontZFail));
        gcmVERIFY_OK(
            gco3D_SetStencilDepthFail(Context->engine,
                                      gcvSTENCIL_BACK,
                                      Context->backZFail));
        gcmVERIFY_OK(
            gco3D_SetStencilFail(Context->engine,
                                 gcvSTENCIL_FRONT,
                                 Context->frontFail));
        gcmVERIFY_OK(
            gco3D_SetStencilFail(Context->engine,
                                 gcvSTENCIL_BACK,
                                 Context->backFail));
        gcmVERIFY_OK(
            gco3D_SetStencilWriteMask(Context->engine,
                                      (gctUINT8) Context->stencilWriteMask));
    }

    /* Return the status. */
    return status;
}

static gctBOOL
_glshMode2Type(
    IN GLenum Mode,
    IN GLsizei Count,
    OUT gcePRIMITIVE * PrimitiveType,
    OUT gctUINT_PTR PrimitiveCount
    )
{
    gcmASSERT(PrimitiveType != gcvNULL);
    gcmASSERT(PrimitiveCount != gcvNULL);

    switch (Mode)
    {
    case GL_POINTS:
        *PrimitiveType  = gcvPRIMITIVE_POINT_LIST;
        *PrimitiveCount = (gctUINT) Count;
        break;

    case GL_LINES:
        *PrimitiveType  = gcvPRIMITIVE_LINE_LIST;
        *PrimitiveCount = (gctUINT) Count / 2;
        break;

    case GL_LINE_LOOP:
        *PrimitiveType  = gcvPRIMITIVE_LINE_LOOP;
		*PrimitiveCount = (gctUINT) Count;
        break;

    case GL_LINE_STRIP:
        *PrimitiveType  = gcvPRIMITIVE_LINE_STRIP;
        *PrimitiveCount = (gctUINT) Count - 1;
        break;

    case GL_TRIANGLES:
        *PrimitiveType  = gcvPRIMITIVE_TRIANGLE_LIST;
        *PrimitiveCount = (gctUINT) Count / 3;
        break;

    case GL_TRIANGLE_STRIP:
        *PrimitiveType  = gcvPRIMITIVE_TRIANGLE_STRIP;
        *PrimitiveCount = (gctUINT) Count - 2;
        break;

    case GL_TRIANGLE_FAN:
        *PrimitiveType  = gcvPRIMITIVE_TRIANGLE_FAN;
        *PrimitiveCount = (gctUINT) Count - 2;
        break;

    case 0x7 /*GL_QUADS*/:
        *PrimitiveType  = gcvPRIMITIVE_RECTANGLE;
        *PrimitiveCount = (gctUINT) Count / 4;
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Mode=0x%04x is invalid", Mode);
        return gcvFALSE;
    }

    return gcvTRUE;
}
#endif

#if !GL_USE_VERTEX_ARRAY
static gceSTATUS
_gl2gcFormat(
    IN GLenum Format,
    OUT gceVERTEX_FORMAT * VertexFormat
    )
{
    gcmASSERT(VertexFormat != gcvNULL);

    switch (Format)
    {
    case GL_BYTE:
        *VertexFormat = gcvVERTEX_BYTE;
        break;

    case GL_UNSIGNED_BYTE:
        *VertexFormat = gcvVERTEX_UNSIGNED_BYTE;
        break;

    case GL_UNSIGNED_INT_10_10_10_2_OES:
        *VertexFormat = gcvVERTEX_UNSIGNED_INT_10_10_10_2;
        break;

    case GL_INT_10_10_10_2_OES:
        *VertexFormat = gcvVERTEX_INT_10_10_10_2;
        break;

    case GL_SHORT:
        *VertexFormat = gcvVERTEX_SHORT;
        break;

    case GL_UNSIGNED_SHORT:
        *VertexFormat = gcvVERTEX_UNSIGNED_SHORT;
        break;

    case GL_INT:
        *VertexFormat = gcvVERTEX_INT;
        break;

    case GL_UNSIGNED_INT:
        *VertexFormat = gcvVERTEX_UNSIGNED_INT;
        break;

    case GL_FIXED:
        *VertexFormat = gcvVERTEX_FIXED;
        break;

    case GL_FLOAT:
        *VertexFormat = gcvVERTEX_FLOAT;
        break;

    case GL_HALF_FLOAT_OES:
        *VertexFormat = gcvVERTEX_HALF;
        break;

    default:
        gcmTRACE(gcvLEVEL_ERROR, "%s: Unsupported format 0x%04X", __FUNCTION__, Format);
        return gcvSTATUS_INVALID_ARGUMENT;
    }

    return gcvSTATUS_OK;
}

static GLsizei
_gl2bytes(
    GLenum Format
    )
{
    switch (Format)
    {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;

    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_HALF_FLOAT_OES:
        return 2;

    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FIXED:
    case GL_FLOAT:
        return 4;
    }

    gcmFATAL("%s(%d): Unsupported format 0x%04X", __FUNCTION__, __LINE__, Format);
    return 0;
}
#endif

#if gcdNULL_DRIVER < 3
#if gcdUSE_TRIANGLE_STRIP_PATCH
static GLboolean
_PatchIndex(
    IN GLContext Context,
    const GLvoid * Indices,
    GLenum Mode,
    GLenum Type,
    GLsizei Count,
    const  GLvoid * * patchedIndices,
    GLenum * patchedMode,
    GLsizei * patchedCount,
    gcoINDEX * patchedIndex,
    gctBOOL  * newIndices
    )
{
    GLvoid  *  indices     = gcvNULL;
    gcoINDEX   indexObject = gcvNULL;
    GLBuffer   boundIndex  = gcvNULL;
    GLubyte *  base        = gcvNULL;
    GLint      elementSize = 0;
    GLubyte *  start       = gcvNULL;
    GLint      triangles   = 0;
    gceSTATUS  status      = gcvSTATUS_OK;
    GLint      sub         = 0;
    GLint      i = 0, j = 0;

    gcmASSERT(patchedIndices != gcvNULL);
    gcmASSERT(patchedMode    != gcvNULL);
    gcmASSERT(patchedCount   != gcvNULL);
    gcmASSERT(patchedIndex   != gcvNULL);

    *patchedIndices = Indices;
    *patchedMode    = Mode;
    *patchedCount   = Count;
    *patchedIndex   = Context->elementArrayBuffer != gcvNULL
                      ? Context->elementArrayBuffer->index : gcvNULL;
    *newIndices     = gcvFALSE;

    if (Mode != GL_TRIANGLE_STRIP)
    {
        return GL_FALSE;
    }

    switch(Type)
    {
    case GL_UNSIGNED_BYTE:
        elementSize = sizeof(GLubyte);
        break;

    case GL_UNSIGNED_SHORT:
        elementSize = sizeof(GLushort);
        break;

    case GL_UNSIGNED_INT:
        elementSize = sizeof(GLuint);
        break;

    default:
        return GL_FALSE;
        break;
    }

    boundIndex = Context->elementArrayBuffer;

    /* If has index object. */
    if (boundIndex)
    {

        if (boundIndex->index == gcvNULL)
        {
            return GL_FALSE;
        }

        /* Reset subs if dirty. */
        if (boundIndex->stripPatchDirty)
        {
            for (i = 0; i < boundIndex->subCount; i++)
            {
                gcoINDEX_Destroy(boundIndex->subs[i].patchIndex);

                boundIndex->subs[i].patchIndex = gcvNULL;
                boundIndex->subs[i].start      = 0;
                boundIndex->subs[i].count      = 0;
            }

            boundIndex->stripPatchDirty = gcvFALSE;
            boundIndex->subCount        = 0;
        }

        /* Find pre-patched index. */
        for (i = 0; i < boundIndex->subCount; i++)
        {
            if ( ((GLint)Indices ==  boundIndex->subs[i].start) &&
                 (Count <= boundIndex->subs[i].count))
            {
                *patchedIndices = 0;
                *patchedMode    = GL_TRIANGLES;
                *patchedCount   = (Count -2)* 3;
                *patchedIndex   = boundIndex->subs[i].patchIndex;

                return GL_FALSE;
            }
        }

        /* Create an index object if has available slot. */
        if (i < gldINDEX_MAX_SUBS)
        {
            sub = i;

            /* Create patched index object. */
            status = gcoINDEX_Construct(Context->hal,
                                        &(boundIndex->subs[sub].patchIndex));
            if (gcmIS_ERROR(status))
            {
                return GL_FALSE;
            }

            boundIndex->subCount++;

            indexObject = boundIndex->subs[sub].patchIndex;
        }

        status =  gcoINDEX_Lock(boundIndex->index,
                                gcvNULL,
                                (GLvoid**)&base);

        if (gcmIS_ERROR(status))
        {
            return GL_FALSE;
        }
    }

    /* calc triangles for strip batch. */
    triangles = Count - 2;

    /* Calculate src start address. */
    if (boundIndex != gcvNULL)
    {
        start     = base + (GLuint)Indices;
    }
    else
    {
        start     = (GLubyte *)Indices;
    }

    status = gcoOS_Allocate(Context->os,
                            triangles * 3 * elementSize,
                            &indices);

    if (gcmIS_ERROR(status))
    {
        return GL_FALSE;
    }

    switch(Type)
    {
    case GL_UNSIGNED_BYTE:
        {
            const GLubyte *src;
            GLubyte       *dst;
            GLint         i, j;

            {
                src = (GLubyte *)start;
                dst = (GLubyte *)indices;
                for (i = 0, j = 0; i < triangles; i++)
                {
                    dst[j * 3]     = src[(i % 2) == 0 ? i : i + 1];
                    dst[j * 3 + 1] = src[(i % 2) == 0 ? i + 1 : i];
                    dst[j * 3 + 2] = src[i + 2];

                    j++;
                }
            }
        }
        break;

    case GL_UNSIGNED_SHORT:
        {
            const GLushort *src;
            GLushort       *dst;

            {
                src = (GLushort *)start;
                dst = (GLushort *)indices;
                for (i = 0, j = 0; i < triangles; i++)
                {
                    dst[j * 3]     = src[(i % 2) == 0 ? i : i + 1];
                    dst[j * 3 + 1] = src[(i % 2) == 0 ? i + 1 : i];
                    dst[j * 3 + 2] = src[i + 2];

                    j++;
                }
            }
        }
        break;

    case GL_UNSIGNED_INT:
        {
            const GLuint *src;
            GLuint       *dst;

            {
                src = (GLuint *)start;
                dst = (GLuint *)indices;
                for (i = 0, j = 0; i < triangles; i++)
                {
                    dst[j * 3]     = src[(i % 2) == 0 ? i : i + 1];
                    dst[j * 3 + 1] = src[(i % 2) == 0 ? i + 1 : i];
                    dst[j * 3 + 2] = src[i + 2];

                    j++;
                }
            }
        }
        break;

    default:
        break;
    }

    /* Patch mode, count, and start index. */
    *patchedMode    = GL_TRIANGLES;
    *patchedCount   = (Count - 2) * 3;

    if (indexObject != gcvNULL)
    {
        gcoINDEX_Upload(boundIndex->subs[sub].patchIndex,
                        indices,
                        triangles * 3 * elementSize);

        boundIndex->subs[sub].start = (GLint)Indices;
        boundIndex->subs[sub].count = Count;

        /* return new object and offset. */
        *patchedIndices = (GLvoid *)0;
        *patchedIndex   = boundIndex->subs[sub].patchIndex;

        gcmVERIFY_OK(gcoOS_Free(Context->os, indices));
    }
    else
    {
        *patchedIndices = indices;
        *patchedIndex   = gcvNULL;
        *newIndices     = gcvTRUE;
    }

    return GL_TRUE;
}
#endif
#endif

static gceSTATUS
_SetView(
    IN GLContext Context
    )
{
    GLint vpLeft, vpTop, vpRight, vpBottom;
    GLint scLeft, scTop, scRight, scBottom;
    gceSTATUS status;

    vpLeft   = Context->viewportX;
    vpTop    = Context->viewportY;
    vpRight  = Context->viewportX + Context->viewportWidth;
    vpBottom = Context->viewportY + Context->viewportHeight;

    if (Context->scissorEnable)
    {
        scLeft   = Context->scissorX;
        scTop    = Context->scissorY;
        scRight  = Context->scissorX + Context->scissorWidth;
        scBottom = Context->scissorY + Context->scissorHeight;
    }
    else
    {
        scLeft   = 0;
        scTop    = 0;
        scRight  = Context->width;
        scBottom = Context->height;
    }

    /* Clip scissor with viewport. */
    if (scLeft   < vpLeft  ) scLeft   = vpLeft;
    if (scTop    < vpTop   ) scTop    = vpTop;
    if (scRight  > vpRight ) scRight  = vpRight;
    if (scBottom > vpBottom) scBottom = vpBottom;

    /* Clip scissor with target. */
    if (scLeft   < 0              ) scLeft   = 0;
    if (scTop    < 0              ) scTop    = 0;
    if (scRight  > Context->width ) scRight  = Context->width;
    if (scBottom > Context->height) scBottom = Context->height;

    if ((scLeft >= scRight) || (scTop >= scBottom))
    {
        return gcvSTATUS_SKIP;
    }

    do
    {
        gcmERR_BREAK(gco3D_SetViewport(Context->engine,
                                       vpLeft,
                                       vpBottom,
                                       vpRight,
                                       vpTop));

        gcmERR_BREAK(gco3D_SetScissors(Context->engine,
                                       scLeft,
                                       scTop,
                                       scRight,
                                       scBottom));

        Context->viewDirty = GL_FALSE;

        status = gcvSTATUS_OK;
    }
    while (gcvFALSE);

    return status;
}

static void
_SetDepthMode(
    GLContext Context
    )
{
    gceDEPTH_MODE depth;
    gceCOMPARE compare;
    gceSTENCIL_MODE stencil;

    if ((  Context->depthTest
        || Context->stencilEnable
        )
    &&  (  (Context->framebuffer == gcvNULL)
        || (Context->framebuffer->depth.surface != gcvNULL)
        )
    )
    {
        gceSURF_FORMAT format = gcvSURF_UNKNOWN;

        if (Context->framebuffer == gcvNULL)
        {
            format = Context->depthFormat;
        }
        else
        {
            gcmVERIFY_OK(
                gcoSURF_GetFormat(Context->framebuffer->depth.surface,
                                  gcvNULL,
                                  &format));
        }

        depth = gcvDEPTH_Z;

        compare = Context->depthTest
                  ? _glshTranslateFunc(Context->depthFunc)
                  : gcvCOMPARE_ALWAYS;

        stencil = (  Context->stencilEnable
                  && (format == gcvSURF_D24S8)
                  )
                  ? gcvSTENCIL_DOUBLE_SIDED
                  : gcvSTENCIL_NONE;
    }
    else
    {
        depth   = gcvDEPTH_NONE;
        compare = gcvCOMPARE_ALWAYS;
        stencil = gcvSTENCIL_NONE;
    }

    gcmVERIFY_OK(gco3D_SetDepthMode(Context->engine, depth));
    gcmVERIFY_OK(gco3D_SetDepthCompare(Context->engine, compare));
    gcmVERIFY_OK(gco3D_SetStencilMode(Context->engine, stencil));
}

GLboolean
_glshFrameBuffer(
    IN GLContext Context
    )
{
    GLboolean doView = GL_FALSE;
    gceSTATUS status;

    /* Test framebuffer object. */
    if (Context->framebufferChanged || Context->depthDirty ||
        (Context->framebuffer && Context->framebuffer->dirty))
    {
		gcmVERIFY_OK(gcoSURF_Flush(Context->draw));

        Context->framebufferChanged = GL_FALSE;
        Context->depthDirty         = GL_FALSE;
        Context->programDirty       = GL_TRUE;

        _SetDepthMode(Context);

        if (Context->framebuffer == gcvNULL)
        {
            gceDEPTH_MODE mode = (Context->depth == gcvNULL) ?
                                    gcvDEPTH_NONE : gcvDEPTH_Z;

            gcmVERIFY_OK(gco3D_SetTarget(Context->engine, Context->draw));
            gcmVERIFY_OK(gco3D_SetDepth(Context->engine, Context->depth));
            gcmVERIFY_OK(gco3D_SetDepthRangeF(Context->engine,
                                              mode,
                                              Context->depthNear,
                                              Context->depthFar));

            gcmVERIFY_OK(gcoSURF_GetSamples(Context->draw, &Context->samples));

            Context->width  = Context->drawWidth;
            Context->height = Context->drawHeight;

            gcmVERIFY_OK(gcoHAL_SetDepthOnly(Context->hal, gcvFALSE));

            doView = GL_TRUE;
        }
        else
        {
            gcoSURF color, depth;

            if (_glshIsFramebufferComplete(Context) != GL_FRAMEBUFFER_COMPLETE)
            {
                gl2mERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
                return GL_FALSE;
            }

            Context->framebuffer->dirty = GL_FALSE;

            /* Get color surface. */
            color = _glshGetFramebufferSurface(&Context->framebuffer->color);

            /* Get depth surface. */
            depth = _glshGetFramebufferSurface(&Context->framebuffer->depth);

            /* Set the render target. */
            gcmVERIFY_OK(
                gco3D_SetTarget(Context->engine, color));

            /* Set the depth buffer. */
            gcmVERIFY_OK(
                gco3D_SetDepth(Context->engine, depth));

            if (color != gcvNULL)
            {
                gcmVERIFY_OK(
                    gcoSURF_SetOrientation(color,
                                           gcvORIENTATION_BOTTOM_TOP));

                gcmONERROR(
                    gcoSURF_GetSize(color,
                                    (gctUINT_PTR) &Context->width,
                                    (gctUINT_PTR) &Context->height,
                                    gcvNULL));

                gcmVERIFY_OK(
                    gcoSURF_GetSamples(color, &Context->samples));

                gcmVERIFY_OK(
                    gcoHAL_SetDepthOnly(Context->hal, gcvFALSE));

                if (Context->framebuffer->color.object->type == GLObject_Texture)
                {
                    /* Texture object needs to be flushed next time it is used. */
                    ((GLTexture) Context->framebuffer->color.object)->needFlush = GL_TRUE;

                    /* Save color attachment rendering framebuffer. */
                    ((GLTexture) Context->framebuffer->color.object)->renderingFb
                        = Context->framebuffer->object.name;
                }
            }
            else
            {
                gcmONERROR(
                    gcoSURF_GetSize(depth,
                                    (gctUINT_PTR) &Context->width,
                                    (gctUINT_PTR) &Context->height,
                                    gcvNULL));

                gcmVERIFY_OK(
                    gcoHAL_SetDepthOnly(Context->hal, gcvTRUE));
            }

            if (depth != gcvNULL)
            {
                gcmVERIFY_OK(
                    gco3D_SetDepthRangeF(Context->engine,
                                         gcvDEPTH_Z,
                                         Context->depthNear,
                                         Context->depthFar));

                gcmVERIFY_OK(
                    gcoSURF_SetOrientation(depth,
                                           gcvORIENTATION_BOTTOM_TOP));

                gcmVERIFY_OK(
                    gcoSURF_GetSamples(depth,
                                       &Context->samples));

                if (Context->framebuffer->depth.object->type == GLObject_Texture)
                {
                    /* Texture object needs to be flushed next time it is used. */
                    ((GLTexture) Context->framebuffer->depth.object)->needFlush = GL_TRUE;
                }
            }

            Context->framebuffer->needResolve = GL_TRUE;

            doView = GL_TRUE;
        }
    }

    if (doView || Context->viewDirty)
    {
        if (_SetView(Context) == gcvSTATUS_SKIP)
        {
            return GL_FALSE;
        }
    }

    /* Test for dirty dithering. */
    if (Context->ditherDirty)
    {
        /* Program dithering. */
        glshProgramDither(Context);
    }

    return GL_TRUE;

OnError:
    return GL_FALSE;
}

#if gcdNULL_DRIVER < 3
static gceSTATUS
_SetShaders(
    IN GLContext Context,
    IN gcePRIMITIVE PrimitiveType
    )
{
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x PrimitiveType=%d", Context, PrimitiveType);

    glmPROFILE(Context, GL_SHADER_OBJECT, (gctUINT32)Context->program->states);

    if ((PrimitiveType == gcvPRIMITIVE_POINT_LIST)
    && (Context->program->hints->vsHasPointSize == 0))
    {
        /* If primitive type is GL_POINTS and gl_PointSize is not set,
           then return error. */
        gcmONERROR(gcvSTATUS_INVALID_DATA);
    }

    gcmONERROR(
        gco3D_SetAllEarlyDepthModes(Context->engine,
                                    Context->program->hints->psHasFragDepthOut));

    /* Check if program is dirty. */
    if (Context->programDirty)
    {
        /* Load program. */
        gcmONERROR(gcLoadShaders(Context->hal,
                                 Context->program->statesSize,
                                 Context->program->states,
                                 Context->program->hints));
#if GC_ENABLE_LOADTIME_OPT
        Context->programDirty = GL_FALSE;
#endif /* GC_ENABLE_LOADTIME_OPT */
    }

    /* Check if primitive type has changed. */
    else if (Context->lastPrimitiveType != PrimitiveType)
    {
        /* Reprogram hints based on primitive type. */
        gcmONERROR(gcLoadShaders(Context->hal,
                                 0,
                                 gcvNULL,
                                 Context->program->hints));
    }

    /* Check if primitive type has changed from or to a POINT_LIST. */
    if ((Context->lastPrimitiveType != PrimitiveType)
    && (  (Context->lastPrimitiveType == (gcePRIMITIVE) -1)
       || (Context->lastPrimitiveType ==  gcvPRIMITIVE_POINT_LIST)
       || (             PrimitiveType ==  gcvPRIMITIVE_POINT_LIST)
       )
    )
    {
        /* Enable point size for points. */
        gcmONERROR(
            gco3D_SetPointSizeEnable(Context->engine,
                                     PrimitiveType == gcvPRIMITIVE_POINT_LIST));

        /* Enable point sprites for points. */
        gcmONERROR(
            gco3D_SetPointSprite(Context->engine,
                                 PrimitiveType == gcvPRIMITIVE_POINT_LIST));
    }

    /* Save current primitive type. */
    Context->lastPrimitiveType = PrimitiveType;

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    /* Return the error. */
   gcmFOOTER();
   return status;
}

static gctBOOL
_MapTextures(
    IN GLContext Context
    )
{
    GLuint i;
    GLint dirty = 0;
    gceSTATUS status;
    GLboolean ditherEnabled = GL_FALSE;
    gctBOOL results = gcvTRUE;
    gctBOOL programHasSamplers = (Context->program->vertexSamplers + Context->program->fragmentSamplers) > 0;

    if (!programHasSamplers)
    {
        /* Nothing to map. */
        return gcvTRUE;
    }


    /* Map all required samplers for the current program. */
    if ((Context->program->vertexSamplers      > Context->vertexSamplers)
        || (Context->program->fragmentSamplers > Context->fragmentSamplers))
    {
        /* Too many samplers requested. */
        return gcvFALSE;
    }

    /* init the result as false when having samplers */
    if((Context->program->vertexSamplers + Context->program->fragmentSamplers) > 0)
    {
        results = gcvFALSE;
    }

    /* Init the result as false. */
    results = gcvFALSE;

    for (i = 0;
        (i < (Context->vertexSamplers + Context->fragmentSamplers)) &&
             programHasSamplers;
        ++i)
    {
#if gldFBO_DATABASE
        /* Get the bound texture. */
        GLTexture texture = Context->program->textures[i];
#else
        /* Get the linked texture sampler number. */
        GLint unit = Context->program->sampleMap[i].unit;
        GLTexture texture;

        /* Dispatch on the sampler type. */
        switch (Context->program->sampleMap[i].type)
        {
        case gcSHADER_SAMPLER_2D:
            /* Get the 2D sampler. */
            texture = Context->texture2D[unit];

            if (texture == gcvNULL)
            {
                /* Revert to default sampler. */
                texture = &Context->default2D;
            }
            break;

        case gcSHADER_SAMPLER_3D:
            /* Get the 3D sampler */
            texture = Context->texture3D[unit];

            if(texture == gcvNULL)
            {
                /* Revert to default sampler */
                texture = &Context->default3D;
            }
            break;

        case gcSHADER_SAMPLER_CUBIC:
            /* Get the Cubic sampler. */
            texture = Context->textureCube[unit];

            if (texture == gcvNULL)
            {
                /* Revert to default sampler. */
                texture = &Context->defaultCube;
            }
            break;
        case gcSHADER_SAMPLER_EXTERNAL_OES:
            /* Get the Cubic sampler. */
            texture = Context->textureExternal[unit];

            if (texture == gcvNULL)
            {
                /* Revert to default sampler. */
                texture = &Context->defaultExternal;
            }
            break;

        default:
            /* Invalid sampler. */
            Context->bounded[i] = gcvNULL;
            continue;
        }

        /* Check for NULL textures or invalid target. */
        if ((texture == gcvNULL)
            ||  (texture->texture == gcvNULL)
            ||  (  (texture->target != GL_TEXTURE_2D)
            && (texture->target != GL_TEXTURE_CUBE_MAP)
            && (texture->target != GL_TEXTURE_3D_OES)
            && (texture->target != GL_TEXTURE_EXTERNAL_OES)
            )
            )
        {
            texture = gcvNULL;
        }
#endif

        results = gcvTRUE;

        /* Check for invalid texture. */
        if (texture == gcvNULL)
        {
            /* Disable the sampler unit. */
            gcmONERROR(gcoTEXTURE_Disable(Context->hal, i));
        }
        else
        {
#if defined(ANDROID) && (ANDROID_SDK_VERSION >= 7)
            if (texture->source != gcvNULL)
            {
                gcoSURF mipmap = gcvNULL;

                /* Get the first LOD. */
                gcmONERROR(gcoTEXTURE_GetMipMap(
                    texture->texture,
                    0,
                    &mipmap
                    ));

                /* Try resolve for texture uploading first. */
                status = gcoSURF_Resolve(texture->source, mipmap);

                if (gcmIS_SUCCESS(status))
                {
                    gcmONERROR(
                        gcoHAL_Commit(Context->hal,
                                      gcvTRUE));
                }

                if (texture->target == GL_TEXTURE_EXTERNAL_OES)
                {
                    /* Reset external source surface. */
                    texture->source = gcvNULL;
                }

                gcmONERROR(status);

                texture->needFlush = gcvTRUE;
            }
#endif

            /* Check for a change in the bounded sampler, or any changed
            ** states. */
            if ((Context->bounded[i] != texture)
                ||  texture->dirty
                )
            {
                /* Set maxLod */
                texture->states.lodMax = texture->maxLevel << 16;

                /* Bind sampler to the hardware. */
                gcmONERROR(gcoTEXTURE_BindTexture(texture->texture,
                                                  texture->target ==
                                                  GL_TEXTURE_EXTERNAL_OES,
                                                  i,
                                                  &texture->states));
            }

            /* Check if the texture data has changed. */
            if (texture->needFlush
                || Context->textureFlushNeeded)
            {
                /* Check if we already flushed the texture cache. */
                if (dirty++ == 0)
                {
                    /* Flush the texture cache. */
                    gcmONERROR(gcoTEXTURE_Flush(texture->texture));
                }
            }

            /* Transform planar-sourced texture. */
            if (texture->direct.dirty)
            {
                /* Filter blit to the temporary buffer to create
                a power of two sized texture. */
                if (texture->direct.temp != gcvNULL)
                {
                    gcmONERROR(gcoSURF_FilterBlit(
                        texture->direct.source,
                        texture->direct.temp,
                        gcvNULL, gcvNULL, gcvNULL
                        ));

                    /* Stall the hardware. */
                    gcmONERROR(gcoHAL_Commit(Context->hal, gcvTRUE));

                    /* Create the first LOD. */
                    gcmONERROR(gcoSURF_Resolve(
                        texture->direct.temp,
                        texture->direct.texture[0]
                    ));
                }
                else
                {
                    gcmONERROR(gcoSURF_Flush(texture->direct.texture[0]));

                    gcmONERROR(gco3D_Semaphore(Context->engine,
                        gcvWHERE_RASTER,
                        gcvWHERE_PIXEL,
                        gcvHOW_SEMAPHORE_STALL));
                    if (Context->ditherEnable)
                    {
                        /*disable the dither for resolve*/
                        gcmONERROR(gco3D_EnableDither(Context->engine, gcvFALSE));

                        ditherEnabled = GL_TRUE;
                    }

                    /* Create the first LOD. */
                    gcmONERROR(gcoSURF_Resolve(
                        texture->direct.source,
                        texture->direct.texture[0]
                    ));

                    if (Context->ditherEnable)
                    {
                        /* restore the dither */
                        gcmONERROR(gco3D_EnableDither(Context->engine, gcvTRUE));

                        ditherEnabled = GL_FALSE;
                    }
                }

                /* wait all the pixels done */
                gcmONERROR(gco3D_Semaphore(Context->engine,
                    gcvWHERE_RASTER,
                    gcvWHERE_PIXEL,
                    gcvHOW_SEMAPHORE_STALL));

                /* Reset planar dirty flag. */
                texture->direct.dirty = gcvFALSE;
            }
        }

        /* Save the bounded sampler. */
        Context->bounded[i] = texture;
    }

    if(programHasSamplers)
    {
        for(i = 0; i < 32; ++i)
        {
            GLTexture texture = Context->bounded[i];
            if(texture != gcvNULL)
            {
                /* Mark texture as no longer dirty. */
                texture->dirty = gcvFALSE;
                /* Mark the texture as flushed. */
                texture->needFlush = gcvFALSE;
            }
        }

        /* Reset texture flush flag. */
        Context->textureFlushNeeded = gcvFALSE;
    }
    /* Success. */
    return results;

OnError:
    /* restore the dither, if it was disabled */
    if (ditherEnabled == GL_TRUE)
    {
        gcmONERROR(gco3D_EnableDither(Context->engine, gcvTRUE));
    }

    /* Error. */
    return gcvFALSE;
}

static GLboolean
_FlushUniforms(
    IN GLContext Context
    )
{
    GLProgram program = Context->program;
    GLint i;
#if GC_ENABLE_LOADTIME_OPT
    GLboolean loadtimeUniformEvaluated = gcvFALSE;
    GLfloat   value;
#endif /* GC_ENABLE_LOADTIME_OPT */
    GLUniform privateUniform;

    if (program == gcvNULL)
    {
        return GL_FALSE;
    }

    privateUniform = program->depthRangeNear;
    if (privateUniform != gcvNULL)
    {
#if GC_ENABLE_LOADTIME_OPT
        value = Context->depthNear;
        if (   privateUniform->dirty
            || *(GLfloat *)privateUniform->data !=  value)
        {
            privateUniform->dirty = gcvFALSE;
            *(GLfloat *)privateUniform->data = value;
            gcmVERIFY_OK(gcUNIFORM_SetValueF(privateUniform->uniform[0],
                                             1,
                                             &value));

            if (privateUniform->uniform[1] != gcvNULL)
            {
                gcmVERIFY_OK(
                    gcUNIFORM_SetValueF(privateUniform->uniform[1],
                                        1,
                                        &value));
            }
        }
#else
        gcmVERIFY_OK(gcUNIFORM_SetValueF(program->depthRangeNear->uniform[0],
                                         1,
                                         &Context->depthNear));

        if (program->depthRangeNear->uniform[1] != gcvNULL)
        {
            gcmVERIFY_OK(
                gcUNIFORM_SetValueF(program->depthRangeNear->uniform[1],
                                    1,
                                    &Context->depthNear));
        }
#endif /* GC_ENABLE_LOADTIME_OPT */
    }

    privateUniform = program->depthRangeFar;
    if (privateUniform != gcvNULL)
    {
#if GC_ENABLE_LOADTIME_OPT
        value = Context->depthFar;
        if (   privateUniform->dirty
            || *(GLfloat *)privateUniform->data !=  value)
        {
            privateUniform->dirty = gcvFALSE;
            *(GLfloat *)privateUniform->data = value;
            gcmVERIFY_OK(gcUNIFORM_SetValueF(privateUniform->uniform[0],
                                             1,
                                             &value));

            if (privateUniform->uniform[1] != gcvNULL)
            {
                gcmVERIFY_OK(
                    gcUNIFORM_SetValueF(privateUniform->uniform[1],
                                        1,
                                        &value));
            }
        }
#else
        gcmVERIFY_OK(gcUNIFORM_SetValueF(program->depthRangeFar->uniform[0],
                                         1,
                                         &Context->depthFar));

        if (program->depthRangeFar->uniform[1] != gcvNULL)
        {
            gcmVERIFY_OK(gcUNIFORM_SetValueF(program->depthRangeFar->uniform[1],
                                             1,
                                             &Context->depthFar));
        }
#endif /* GC_ENABLE_LOADTIME_OPT */
    }

    privateUniform = program->depthRangeDiff;
    if (privateUniform != gcvNULL)
    {
#if GC_ENABLE_LOADTIME_OPT
        value = Context->depthFar - Context->depthNear;
        if (   privateUniform->dirty
            || *(GLfloat *)privateUniform->data !=  value)
        {
            privateUniform->dirty = gcvFALSE;
            *(GLfloat *)privateUniform->data = value;
            gcmVERIFY_OK(gcUNIFORM_SetValueF(privateUniform->uniform[0],
                                             1,
                                             &value));

            if (privateUniform->uniform[1] != gcvNULL)
            {
                gcmVERIFY_OK(
                    gcUNIFORM_SetValueF(privateUniform->uniform[1],
                                        1,
                                        &value));
            }
        }
#else
        gctFLOAT diff = Context->depthFar - Context->depthNear;

        gcmVERIFY_OK(gcUNIFORM_SetValueF(program->depthRangeDiff->uniform[0],
                                         1,
                                         &diff));

        if (program->depthRangeDiff->uniform[1] != gcvNULL)
        {
            gcmVERIFY_OK(
                gcUNIFORM_SetValueF(program->depthRangeDiff->uniform[1],
                                    1,
                                    &diff));
        }
#endif /* GC_ENABLE_LOADTIME_OPT */
    }

    privateUniform = program->height;
    if (privateUniform != gcvNULL)
    {
#if GC_ENABLE_LOADTIME_OPT
        value = (GLfloat) Context->height;
        if (   privateUniform->dirty
            || *(GLfloat *)privateUniform->data !=  value)
        {
            privateUniform->dirty = gcvFALSE;
            *(GLfloat *)privateUniform->data = value;
            gcmVERIFY_OK(gcUNIFORM_SetValueF(privateUniform->uniform[0],
                                             1,
                                             &value));

            if (privateUniform->uniform[1] != gcvNULL)
            {
                gcmVERIFY_OK(
                    gcUNIFORM_SetValueF(privateUniform->uniform[1],
                                        1,
                                        &value));
            }
        }
#else
        gctFLOAT height = (GLfloat) Context->height;

        gcmVERIFY_OK(gcUNIFORM_SetValueF(program->height->uniform[0],
                                         1,
                                         &height));

        if (program->height->uniform[1] != gcvNULL)
        {
            gcmVERIFY_OK(gcUNIFORM_SetValueF(program->height->uniform[1],
                                             1,
                                             &height));
        }
#endif /* GC_ENABLE_LOADTIME_OPT */
    }

    for (i = 0; i < program->uniformCount; ++i)
    {
        if (program->uniforms[i].dirty || Context->programDirty)
        {
            gcSHADER_TYPE type;
            gctSIZE_T length;
            GLint j;

            if (gcmIS_ERROR(gcUNIFORM_GetType(program->uniforms[i].uniform[0],
                                              &type,
                                              &length)))
            {
                return GL_FALSE;
            }
#if GC_ENABLE_LOADTIME_OPT
			/*  do load time optimization if the uniform is
             * marked as loadtime constant */
			if (  !loadtimeUniformEvaluated
                && gcIsUniformALoadtimeConstant(&program->uniforms[i]))
			{
                /* gcmASSERT(program->uniforms[i].uniform[0]->glUniformIndex == (gctUINT16) i); */
                loadtimeUniformEvaluated = gcvTRUE;
                /* compute all loadtime constants */
				gcmVERIFY_OK(gcComputeLoadtimeConstant(Context));
                /* still need to flush the value to hardware */
			}
#endif /* GC_ENABLE_LOADTIME_OPT */

            switch (type)
            {
            case gcSHADER_FLOAT_X1:
            case gcSHADER_FLOAT_X2:
            case gcSHADER_FLOAT_X3:
            case gcSHADER_FLOAT_X4:
            case gcSHADER_FLOAT_2X2:
            case gcSHADER_FLOAT_3X3:
            case gcSHADER_FLOAT_4X4:
                for (j = 0; j < 2; ++j)
                {
                    if ((program->uniforms[i].uniform[j] != gcvNULL)
                    &&  gcmIS_ERROR(
                            gcUNIFORM_SetValueF(program->uniforms[i].uniform[j],
                                                length,
                                                program->uniforms[i].data))
                    )
                    {
                        return GL_FALSE;
                    }
                }
                break;

            case gcSHADER_BOOLEAN_X1:
            case gcSHADER_BOOLEAN_X2:
            case gcSHADER_BOOLEAN_X3:
            case gcSHADER_BOOLEAN_X4:
            case gcSHADER_INTEGER_X1:
            case gcSHADER_INTEGER_X2:
            case gcSHADER_INTEGER_X3:
            case gcSHADER_INTEGER_X4:
                for (j = 0; j < 2; ++j)
                {
                    if ((program->uniforms[i].uniform[j] != gcvNULL)
                    &&  gcmIS_ERROR(
                            gcUNIFORM_SetValue(program->uniforms[i].uniform[j],
                                               length,
                                               program->uniforms[i].data))
                    )
                    {
                        return GL_FALSE;
                    }
                }
                break;

            default:
                break;
            }

            program->uniforms[i].dirty = GL_FALSE;
        }
    }
#if GC_ENABLE_LOADTIME_OPT
    for (i = 0; i < program->privateUniformCount; ++i)
    {
		/*  do load time optimization if the uniform is
         * marked as loadtime constant */
		if (gcIsUniformALoadtimeConstant(&program->privateUniforms[i]))
		{
    		if (  !loadtimeUniformEvaluated )
            {
                loadtimeUniformEvaluated = gcvTRUE;
                /* compute all loadtime constants */
			    gcmVERIFY_OK(gcComputeLoadtimeConstant(Context));
                /* still need to flush the value to hardware */
            }
		}
        else
        {
            continue; /* other private uniforms are already processed */
        }

        if (program->privateUniforms[i].dirty || Context->programDirty)
        {
            gcSHADER_TYPE type;
            gctSIZE_T length;
            GLint j;

            if (gcmIS_ERROR(gcUNIFORM_GetType(program->privateUniforms[i].uniform[0],
                                              &type,
                                              &length)))
            {
                return GL_FALSE;
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
                for (j = 0; j < 2; ++j)
                {
                    if ((program->privateUniforms[i].uniform[j] != gcvNULL)
                    &&  gcmIS_ERROR(
                            gcUNIFORM_SetValueF(program->privateUniforms[i].uniform[j],
                                                length,
                                                program->privateUniforms[i].data))
                    )
                    {
                        return GL_FALSE;
                    }
                }
                break;

            case gcSHADER_BOOLEAN_X1:
            case gcSHADER_BOOLEAN_X2:
            case gcSHADER_BOOLEAN_X3:
            case gcSHADER_BOOLEAN_X4:
            case gcSHADER_INTEGER_X1:
            case gcSHADER_INTEGER_X2:
            case gcSHADER_INTEGER_X3:
            case gcSHADER_INTEGER_X4:
                for (j = 0; j < 2; ++j)
                {
                    if ((program->privateUniforms[i].uniform[j] != gcvNULL)
                    &&  gcmIS_ERROR(
                            gcUNIFORM_SetValue(program->privateUniforms[i].uniform[j],
                                               length,
                                               program->privateUniforms[i].data))
                    )
                    {
                        return GL_FALSE;
                    }
                }
                break;

            default:
                break;
            }

            program->privateUniforms[i].dirty = GL_FALSE;
        }
    }
#endif /* GC_ENABLE_LOADTIME_OPT */

#if !GC_ENABLE_LOADTIME_OPT
    Context->programDirty = GL_FALSE;
#endif /* GC_ENABLE_LOADTIME_OPT */

    return GL_TRUE;
}
#endif

#if !GL_USE_VERTEX_ARRAY
static gceSTATUS
_ConstructVertex(
    IN GLContext Context,
    IN GLint First,
    IN GLsizei Count,
    IN gcoVERTEX Vertex,
    OUT gcoSTREAM Streams[gldVERTEX_ELEMENT_COUNT],
    OUT gctINT_PTR StreamIndex
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    GLuint i;
    gceVERTEX_FORMAT vertexFormat;

    GLAttribute *contexVertexArray;

    if(Context->vertexArrayObject == gcvNULL)
    {
        contexVertexArray = Context->vertexArray;
    }
    else
    {
        contexVertexArray = Context->vertexArrayObject->vertexArray;
    }

    for (i = 0; i < Context->maxAttributes; ++i)
    {
        GLint linkage;
        gcATTRIBUTE attribute;
        gctBOOL enabled = gcvFALSE;
        gcSHADER_TYPE type = (gcSHADER_TYPE) 0;
        gctSIZE_T length = 0;
        GLint components;
        GLAttribute * vertexArray = &contexVertexArray[i];

        linkage = Context->program->attributeLinkage[i];

        if (linkage == -1)
        {
            continue;
        }

        attribute = Context->program->attributeLocation[linkage].attribute;
        gcmASSERT(attribute != gcvNULL);

        gcmERR_BREAK(gcATTRIBUTE_IsEnabled(attribute, &enabled));

        if (!enabled)
        {
            continue;
        }

        gcmERR_BREAK(gcATTRIBUTE_GetType(attribute, &type, &length));

        switch (type)
        {
        case gcSHADER_FLOAT_X1:
            /* Fall through. */
        case gcSHADER_BOOLEAN_X1:
            /* Fall through. */
        case gcSHADER_INTEGER_X1:
            /* Fall through. */
            components = 1;
            break;

        case gcSHADER_FLOAT_X2:
            /* Fall through. */
        case gcSHADER_BOOLEAN_X2:
            /* Fall through. */
        case gcSHADER_INTEGER_X2:
            /* Fall through. */
            components = 2;
            break;

        case gcSHADER_FLOAT_X3:
            /* Fall through. */
        case gcSHADER_BOOLEAN_X3:
            /* Fall through. */
        case gcSHADER_INTEGER_X3:
            /* Fall through. */
            components = 3;
            break;

        case gcSHADER_FLOAT_X4:
            /* Fall through. */
        case gcSHADER_BOOLEAN_X4:
            /* Fall through. */
        case gcSHADER_INTEGER_X4:
            /* Fall through. */
            components = 4;
            break;

        default:
            gcmFATAL("!! TODO !!");
            return gcvSTATUS_TOO_COMPLEX;
        }

#if gcmIS_DEBUG(gcdDEBUG_TRACE)
        if (vertexArray->size > components)
        {
            gcmTRACE(gcvLEVEL_WARNING,
                     "WARNING: Vertex defines more attributes (%d) than"
                     " required (%d).",
                     vertexArray->size,
                     components);
        }
#endif

        if ( (vertexArray->ptr == gcvNULL) && vertexArray->enable )
        {
            if (vertexArray->buffer == gcvNULL)
            {
                status = gcvSTATUS_INVALID_DATA;
                break;
            }

            gcmERR_BREAK(_gl2gcFormat(vertexArray->type, &vertexFormat));

            gcmERR_BREAK(
                gcoVERTEX_EnableAttribute(Vertex,
                                          linkage,
                                          vertexFormat,
                                          vertexArray->normalized,
                                          vertexArray->size,
                                          vertexArray->buffer->stream,
                                          vertexArray->offset,
                                          vertexArray->stride));
        }
        else
        {
            gcoSTREAM stream;
            GLenum vertexType = vertexArray->type;
            GLsizei streamStride;
            GLint generics, start = 0;

            gcmERR_BREAK(gcoSTREAM_Construct(Context->hal, &stream));

            Streams[*StreamIndex] = stream;
            *StreamIndex += 1;

            if (vertexArray->ptr == gcvNULL)
            {
                switch (vertexArray->size)
                {
                case 4:
                    if (Context->genericVertex[i][3] != 1.0f)
                    {
                        components = generics = 4;
                        break;
                    }
                    /* Fall through. */

                case 3:
                    if (Context->genericVertex[i][2] != 0.0f)
                    {
                        components = generics = 3;
                        break;
                    }
                    /* Fall through. */

                case 2:
                    if (Context->genericVertex[i][1] != 0.0f)
                    {
                        components = generics = 2;
                        break;
                    }
                    /* Fall through. */

                default:
                    components = generics = 1;
                    break;
                }
            }
            else if (vertexArray->size < components)
            {
                switch (vertexArray->size)
                {
                case 1:
                    if (Context->genericVertex[i][1] != 0.0f)
                    {
                        start    = 1;
                        generics = components - 1;
                        break;
                    }
                    /* Fall through. */

                case 2:
                    if (Context->genericVertex[i][2] != 0.0f)
                    {
                        start    = 2;
                        generics = components - 2;
                        break;
                    }
                    /* Fall through. */

                case 3:
                    if (Context->genericVertex[i][3] != 1.0f)
                    {
                        start    = 3;
                        generics = components - 3;
                        break;
                    }
                    /* Fall through. */

                default:
                    start      = 0;
                    generics   = 0;
                    components = vertexArray->size;
                    break;
                }
            }
            else
            {
                generics = 0;
            }

            streamStride = components * _gl2bytes(vertexType);

            if (generics == 0)
            {
                if (streamStride == vertexArray->stride)
                {
                    gcmERR_BREAK(
                        gcoSTREAM_Upload(stream,
                                         vertexArray->ptr,
                                         0,
                                         (First + Count) * streamStride,
                                         gcvFALSE));
                }
                else
                {
                    GLint v;
                    const char * ptr = (const char *) vertexArray->ptr
                                     + First * vertexArray->stride;
                    gctUINT32 offset = First * streamStride;

                    gcmERR_BREAK(
                        gcoSTREAM_Reserve(stream,
                                          (First + Count) * streamStride));

                    for (v = First; v < First + Count; ++v)
                    {
                        gcmERR_BREAK(gcoSTREAM_Upload(stream,
                                                      ptr,
                                                      offset,
                                                      streamStride,
                                                      gcvFALSE));

                        ptr    += vertexArray->stride;
                        offset += streamStride;
                    }
                }
            }
            else
            {
                GLsizei v;
                const char * ptr = (const char *) vertexArray->ptr;

                gcmERR_BREAK(gcoSTREAM_Reserve(stream,
                                               streamStride * (First + Count)));

                for (v = First; v < First + Count; ++v)
                {
                    if (start != 0)
                    {
                        gcmERR_BREAK(
                            gcoSTREAM_Upload(stream,
                                             ptr + v * vertexArray->stride,
                                             v * streamStride,
                                             start * _gl2bytes(vertexType),
                                             gcvFALSE));
                    }

                    switch (vertexType)
                    {
                    case GL_FLOAT:
                        gcmERR_BREAK(
                            gcoSTREAM_Upload(stream,
                                             &Context->genericVertex[i][start],
                                             v * streamStride
                                                 + start * gcmSIZEOF(GLfloat),
                                             generics * gcmSIZEOF(GLfloat),
                                             gcvFALSE));
                        break;

                    default:
                        gcmFATAL("!!TODO!!");
                    }

                    if (gcmIS_ERROR(status))
                    {
                        break;
                    }
                }
            }

            gcmERR_BREAK(gcoSTREAM_SetStride(stream, streamStride));

            gcmERR_BREAK(_gl2gcFormat(vertexType, &vertexFormat));

            gcmERR_BREAK(gcoVERTEX_EnableAttribute(Vertex,
                                                   linkage,
                                                   vertexFormat,
                                                   vertexArray->normalized,
                                                   components,
                                                   stream,
                                                   vertexArray->offset,
                                                   streamStride));
        }
    }

    return status;
}

static gceSTATUS
_ConstructDynamicVertex(
    IN GLContext Context,
    IN GLint First,
    IN GLsizei Count
    )
{
    gcsSTREAM_INFO info[gldVERTEX_ELEMENT_COUNT];
    gctUINT count = 0, i;
    gctSIZE_T bytes, stride;
    GLuint linkage;
    gcATTRIBUTE attribute;
    gctBOOL enabled = gcvFALSE;
    gcSHADER_TYPE type = (gcSHADER_TYPE) 0;
    gctSIZE_T length = 0;
    gctINT components;
    gceSTATUS status;
    gceVERTEX_FORMAT vertexFormat;

    GLAttribute *vertexArray;

    if(Context->vertexArrayObject == gcvNULL)
    {
        vertexArray = Context->vertexArray;
    }
    else
    {
        vertexArray = Context->vertexArrayObject->vertexArray;
    }

    /* Get the pointers and sizes for each of the enabled attributes. */
    for (i = 0; i < Context->maxAttributes; ++i)
    {
        /* Short hand to vertex array. */
        const GLAttribute * a = &vertexArray[i];

        /* We don't support mixing dynamic buffers with VBO's. */
        if (a->buffer != 0)
        {
            return gcvSTATUS_INVALID_CONFIG;
        }

        /* Get attribute linkage. */
        linkage = Context->program->attributeLinkage[i];

        /* Ignore this attribute if it is not linked. */
        if (linkage == ~0U)
        {
            continue;
        }

        /* Get the linked attribute. */
        attribute = Context->program->attributeLocation[linkage].attribute;
        gcmASSERT(attribute != gcvNULL);

        /* Check whether the attribute is enabeld or not. */
        gcmERR_RETURN(
            gcATTRIBUTE_IsEnabled(attribute, &enabled));

        if (!enabled)
        {
            /* Skip attributes that are not enabled. */
            continue;
        }

        /* Get the type of the attribute. */
        gcmERR_RETURN(
            gcATTRIBUTE_GetType(attribute, &type, &length));

        /* Count the number of required components. */
        switch (type)
        {
        case gcSHADER_FLOAT_X1:
        case gcSHADER_INTEGER_X1:
        case gcSHADER_BOOLEAN_X1:
            components = 1;
            break;

        case gcSHADER_FLOAT_X2:
        case gcSHADER_INTEGER_X2:
        case gcSHADER_BOOLEAN_X2:
            components = 2;
            break;

        case gcSHADER_FLOAT_X3:
        case gcSHADER_INTEGER_X3:
        case gcSHADER_BOOLEAN_X3:
            components = 3;
            break;

        case gcSHADER_FLOAT_X4:
        case gcSHADER_INTEGER_X4:
        case gcSHADER_BOOLEAN_X4:
            components = 4;
            break;

        default:
            gcmFATAL("Invalid type %d for attribute 0x%x", type, attribute);
            return gcvSTATUS_INVALID_CONFIG;
        }

        /* Get the size of the attribute. */
        bytes = a->size * _gl2bytes(a->type);

        /* Get the stride. */
        stride = a->stride;
        if (stride == 0)
        {
            /* Use default stride. */
            stride = bytes;
        }

        if ((a->ptr == gcvNULL) || !a->enable)
        {
            /* Copy the generic vertex data for this attribute. */
            info[count].index      = linkage;
            info[count].format     = gcvVERTEX_FLOAT;
            info[count].normalized = a->normalized;
            info[count].components = components;
            info[count].size       = components * _gl2bytes(GL_FLOAT);
            info[count].data       = Context->genericVertex[i];
            info[count].stride     = 0;
        }
        else
        {
            /* Special check fi there is missing data for the attribute. */
            if (components > a->size)
            {
                switch (components)
                {
                case 1:
                    /* No components specified. */
                    if (Context->genericVertex[i][0] != 0.0f)
                    {
                        /* TODO: Add support for generics with x != 0.0f. */
                        return gcvSTATUS_INVALID_CONFIG;
                    }

                case 2:
                    /* Only 1 component specified. */
                    if (Context->genericVertex[i][1] != 0.0f)
                    {
                        /* TODO: Add support for generics with y != 0.0f. */
                        return gcvSTATUS_INVALID_CONFIG;
                    }

                case 3:
                    /* Only 2 components specified. */
                    if (Context->genericVertex[i][2] != 0.0f)
                    {
                        /* TODO: Add support for generics width z != 0.0f. */
                        return gcvSTATUS_INVALID_CONFIG;
                    }

                case 4:
                    /* Only 3 component specified. */
                    if (Context->genericVertex[i][3] != 1.0f)
                    {
                        /* TODO: Add support for generics with w != 1.0f. */
                        return gcvSTATUS_INVALID_CONFIG;
                    }
                }
            }

            gcmERR_RETURN(_gl2gcFormat(a->type, &vertexFormat));

            /* Fill in the info structure. */
            info[count].index      = linkage;
            info[count].format     = vertexFormat;
            info[count].normalized = a->normalized;
            info[count].components = a->size;
            info[count].size       = bytes;
            info[count].data       = a->ptr;
            info[count].stride     = stride;
        }

        ++ count;
    }

    /* Upload the data. */
    return gcoSTREAM_UploadDynamic(Context->dynamicArray,
                                   Count + First,
                                   count, info,
                                   Context->vertex);
}

#endif

/*******************************************************************************
**  glDrawElements
**
**  Render primitives from array data.
**
***** PARAMETERS
**
**  mode
**
**      Specifies what kind of primitives to render. Symbolic constants
**      GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP,
**      GL_TRIANGLE_FAN, and GL_TRIANGLES are accepted.
**
**  count
**
**      Specifies the number of elements to be rendered.
**
**  type
**
**      Specifies the type of the values in indices. Must be GL_UNSIGNED_BYTE or
**      GL_UNSIGNED_SHORT.
**
**  indices
**
**      Specifies a pointer to the location where the indices are stored.
**
***** DESCRIPTION
**
**  glDrawElements specifies multiple geometric primitives with very few
**  subroutine calls. Instead of calling a GL function to pass each vertex
**  attribute, you can use glVertexAttribPointer to prespecify separate arrays
**  of vertex attributes and use them to construct a sequence of primitives with
**  a single call to glDrawElements.
**
**  When glDrawElements is called, it uses count sequential elements from an
**  enabled array, starting at indices to construct a sequence of geometric
**  primitives. mode specifies what kind of primitives are constructed and how
**  the array elements construct these primitives. If more than one array is
**  enabled, each is used.
**
**  To enable and disable a generic vertex attribute array, call
**  glEnableVertexAttribArray and glDisableVertexAttribArray.
**
***** NOTES
**
**  If the current program object, as set by glUseProgram, is invalid, rendering
**  results are undefined. However, no error is generated for this case.
**
***** ERRORS
**
**  GL_INVALID_ENUM is generated if mode is not an accepted value.
**
**  GL_INVALID_ENUM is generated if type is not GL_UNSIGNED_BYTE or
**  GL_UNSIGNED_SHORT.
**
**  GL_INVALID_VALUE is generated if count is negative.
**
**  GL_INVALID_FRAMEBUFFER_OPERATION is generated if the currently bound
**  framebuffer is not framebuffer complete (i.e. the return value from
**  glCheckFramebufferStatus is not GL_FRAMEBUFFER_COMPLETE).
*/
GL_APICALL void GL_APIENTRY
glDrawElements(
    GLenum Mode,
    GLsizei Count,
    GLenum Type,
    const GLvoid * Indices
    )
{
#if gcdNULL_DRIVER < 3
    gcePRIMITIVE primitiveType;
    gctUINT primitiveCount;
    gceINDEX_TYPE indexType = gcvINDEX_8;
    gceSTATUS status;
    GLBuffer *elementArrayBuffer;
#if GL_USE_VERTEX_ARRAY
    gctUINT enable, i;
    gcsVERTEXARRAY *vertexArray;
#else
    gcoINDEX index = gcvNULL;
    GLBuffer buffer;
    gcoSTREAM streams[gldVERTEX_ELEMENT_COUNT];
    gctUINT bytesPerIndex;
    gctINT streamIndex = 0;
    gctUINT32 minIndex = 0, maxIndex = 0;
    GLAttribute *vertexArray;
#endif
	gctBOOL goToEnd = gcvFALSE;
    gcoINDEX indexBuffer = gcvNULL;

    GLboolean indexPatched = GL_FALSE;
    gctBOOL   newIndices   = GL_FALSE;

	glmENTER4(glmARGENUM, Mode, glmARGINT, Count, glmARGENUM, Type,
              glmARGPTR, Indices)
    {

    if(context->vertexArrayObject == gcvNULL)
    {
        vertexArray = context->vertexArray;
        elementArrayBuffer = &context->elementArrayBuffer;
    }
    else
    {
        vertexArray = context->vertexArrayObject->vertexArray;
        elementArrayBuffer = &context->vertexArrayObject->elementArrayBuffer;
    }

#if gcdDUMP_API
    gcmDUMP_API("${ES20 glDrawElements 0x%08X 0x%08X 0x%08X (0x%08X)",
                Mode, Count, Type, Indices);
    {
        GLint min = -1, max = -1, element;
        GLsizei i;

        if (*elementArrayBuffer == gcvNULL)
        {
            switch (Type)
            {
            case GL_UNSIGNED_BYTE:
                gcmDUMP_API_DATA(Indices, Count);
                for (i = 0; i < Count; ++i)
                {
                    element = ((GLubyte *) Indices)[i];
                    if ((min < 0) || (element < min))
                    {
                        min = element;
                    }
                    if ((max < 0) || (element > max))
                    {
                        max = element;
                    }
                }
                break;

            case GL_UNSIGNED_SHORT:
                gcmDUMP_API_DATA(Indices, Count * 2);
                for (i = 0; i < Count; ++i)
                {
                    element = ((GLushort *) Indices)[i];
                    if ((min < 0) || (element < min))
                    {
                        min = element;
                    }
                    if ((max < 0) || (element > max))
                    {
                        max = element;
                    }
                }
                break;

            case GL_UNSIGNED_INT:
                gcmDUMP_API_DATA(Indices, Count * 4);
                for (i = 0; i < Count; ++i)
                {
                    element = ((GLuint *) Indices)[i];
                    if ((min < 0) || (element < min))
                    {
                        min = element;
                    }
                    if ((max < 0) || (element > max))
                    {
                        max = element;
                    }
                }
                break;

            default:
                break;
            }
        }
        else
        {
            switch (Type)
            {
            case GL_UNSIGNED_BYTE:
                gcoINDEX_GetIndexRange((*elementArrayBuffer)->index,
                                       gcvINDEX_8,
                                       (gctUINT32) Indices,
                                       Count,
                                       (gctUINT32_PTR) &min,
                                       (gctUINT32_PTR) &max);
                break;

            case GL_UNSIGNED_SHORT:
                gcoINDEX_GetIndexRange((*elementArrayBuffer)->index,
                                       gcvINDEX_16,
                                       (gctUINT32) Indices,
                                       Count,
                                       (gctUINT32_PTR) &min,
                                       (gctUINT32_PTR) &max);
                break;

            case GL_UNSIGNED_INT:
                gcoINDEX_GetIndexRange((*elementArrayBuffer)->index,
                                       gcvINDEX_32,
                                       (gctUINT32) Indices,
                                       Count,
                                       (gctUINT32_PTR) &min,
                                       (gctUINT32_PTR) &max);
                break;

            default:
                break;
            }
        }

#if gcdDUMP_API
        if (min >= 0)
        {
            _DumpVertices(context, min, max - min + 1);
        }
    }
#endif
    gcmDUMP_API("$}");
#endif

    /* Count the call. */
    glmPROFILE(context, GLES2_DRAWELEMENTS, 0);

#if gldFBO_DATABASE
    /* Check if we have a current framebuffer and we are not playing
     * back. */
    if (context->dbEnable
        &&
        (context->framebuffer != gcvNULL)
        &&
        !context->playing
        )
    {
        /* Add the draw elements call to the database. */
        if (gcmIS_SUCCESS(glshAddDrawElements(context,
                                              Mode,
                                              Count,
                                              Type,
                                              Indices)))
        {
            /* Done. */
            break;
        }

        /* Some error - play back the database. */
        if (gcmIS_ERROR(glshPlayDatabase(context,
                                         context->framebuffer->currentDB)))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            glmERROR_BREAK();
        }
    }

    /* Check dependencies for playback. */
    gcmVERIFY_OK(glshCheckDependency(context));
#endif

#if gldBATCH_OPTIMIZATION
    /* Batch optimization. */
    if (
        /* Nothing has changed in between batches. */
        !context->batchDirty

        /* Alpha blending and stencil shoud be OFF. */
        && (context->blendEnable   == GL_FALSE)
        && (context->stencilEnable == GL_FALSE)

        /* None of the enabled streams can be an array. */
        && ((context->batchEnable & context->batchArray) == 0)

        /* The index has to come from a buffer. */
        && (context->elementArrayBuffer != gcvNULL)

        /* This is the same primitive as before. */
        &&  (context->batchMode      == Mode)
        &&  (context->batchCount     == Count)
        &&  (context->batchIndexType == Type)
        &&  (context->batchPointer   == Indices)

#if gcdUSE_TRIANGLE_STRIP_PATCH
        &&  (Mode != GL_TRIANGLE_STRIP)
#endif

        /* Make sure we don't skip too much. */
        && (++context->batchSkip < gldBATCH_OPTIMIZATION)
    )
    {
        /* Drop the primitive. */
		break;
    }
#endif

#if gcdUSE_TRIANGLE_STRIP_PATCH
    if (context->patchStrip)
    {
        if (Mode == GL_TRIANGLE_STRIP)
        {
            gcmTRACE(gcvLEVEL_WARNING, "Convert triangle strip.");

            indexPatched = _PatchIndex(context,
                                   Indices,
                                   Mode,
                                   Type,
                                   Count,
                                   &Indices,
                                   &Mode,
                                   &Count,
                                   &indexBuffer,
                                   &newIndices);
        }
        else
        {
            indexBuffer = (*elementArrayBuffer != gcvNULL)
                        ? (*elementArrayBuffer)->index
                        : gcvNULL;
        }
    }
    else
#endif
    {
        if (*elementArrayBuffer != gcvNULL)
        {
            indexBuffer = (*elementArrayBuffer)->index;
        }
    }

    /* Convert <Mode> and <Count> to primitive type and count. */
    if (!_glshMode2Type(Mode, Count, &primitiveType, &primitiveCount))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Mode=0x%04x is invalid",
                      Mode);

        gl2mERROR(GL_INVALID_ENUM);
		glmERROR_BREAK();
    }

    /* Test for valid <Count>. */
    if (Count < 0)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Negative Count=%lu is invalid",
                      Count);

        gl2mERROR(GL_INVALID_VALUE);
        glmERROR_BREAK();
    }

    /* Bail out if there is nothing to render. */
    if (primitiveCount < 1)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "No primitives to render");
		break;
    }

    /* Bail out if there are no indices specified. */
    if ((Indices == gcvNULL) && (*elementArrayBuffer == gcvNULL))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "No indices specified");

        gl2mERROR(GL_INVALID_OPERATION);
        glmERROR_BREAK();
    }

    /* Bail out if there is no program loaded. */
    if (context->program == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "No program loaded");

        break;
    }

    /* Invalid operation if the program could not be linked. */
    if (context->program->statesSize == 0)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Program could not be linked");

        break;
    }

    /* Update frame buffer. */
    if (!_glshFrameBuffer(context))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Framebuffer is incomplete");

        break;
    }

    /* Triangles won't be rendered when both front and back faces are culled. */
    if (context->cullEnable
    &&  (context->cullMode == GL_FRONT_AND_BACK)
    &&  (  (Mode == GL_TRIANGLES)
        || (Mode == GL_TRIANGLE_STRIP)
        || (Mode == GL_TRIANGLE_FAN)
        )
    )
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Triangles with both faces culled");

        break;
    }

    /* Map textures. */
    if (!_MapTextures(context))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Could not map all textures");

        gl2mERROR(GL_INVALID_OPERATION);
        glmERROR_BREAK();
    }

#if GC_ENABLE_LOADTIME_OPT
    /* Flush the uniforms. */
    gcmVERIFY(_FlushUniforms(context));
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Program the shader. */
    if (gcmIS_ERROR(_SetShaders(context, primitiveType)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Could not set the shaders.");

        gl2mERROR(GL_INVALID_OPERATION);
        glmERROR_BREAK();
    }

#if !GC_ENABLE_LOADTIME_OPT
    /* Flush the uniforms. */
    gcmVERIFY(_FlushUniforms(context));
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Convert <Type> into a HAL type. */
    switch (Type)
    {
    case GL_UNSIGNED_BYTE:
        indexType     = gcvINDEX_8;
#if !GL_USE_VERTEX_ARRAY
        bytesPerIndex = 1;
#endif
        break;

    case GL_UNSIGNED_SHORT:
        indexType     = gcvINDEX_16;
#if !GL_USE_VERTEX_ARRAY
        bytesPerIndex = 2;
#endif
        break;

    case GL_UNSIGNED_INT:
        indexType     = gcvINDEX_32;
#if !GL_USE_VERTEX_ARRAY
        bytesPerIndex = 4;
#endif
        break;

    default:
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Type=0x%04x is invalid");

        gl2mERROR(GL_INVALID_ENUM);
		goToEnd = gcvTRUE;
		glmERROR_BREAK();
    }

	if (goToEnd)
		break;

    /* Count the primitives. */
    glmPROFILE(context, GL2_PROFILER_PRIMITIVE_TYPE, primitiveType);
    glmPROFILE(context, GL2_PROFILER_PRIMITIVE_COUNT, primitiveCount);

#if GL_USE_VERTEX_ARRAY
   /* Walk all attributes. */
    for (i = enable = 0; i < context->maxAttributes; ++i)
    {
        gctUINT link;

        /* Get linkage of vertex attribute. */
        link = context->program->attributeLinkage[i];
        if (link == (GLuint) -1)
            continue;

        vertexArray[i].linkage = context->program->attributeMap[link];

        if ((link < context->maxAttributes)
        &&  ((context->program->attributeEnable >> link) & 1)
        )
        {
            enable |= (1 << i);
        }
    }

    /* Bind vertices and indices to hardware. */
    gcmONERROR(gcoVERTEXARRAY_Bind(context->vertex,
                                   enable,
                                   vertexArray,
                                   0, Count,
                                   indexType,
                                   indexBuffer,
                                   (gctPOINTER) Indices,
                                   &primitiveType,
                                   &primitiveCount));

    /* Draw primitives. */
    gcmONERROR(_DrawPrimitives(context,
                               primitiveType,
                               0,
                               primitiveCount,
                               gcvTRUE));

    /* Update batch. */
    context->batchDirty     = GL_FALSE;
    context->batchMode      = Mode;
    context->batchCount     = Count;
    context->batchIndexType = Type;
    context->batchPointer   = Indices;
    context->batchSkip      = 0;

	}

    if (indexPatched && newIndices)
    {
        gcmVERIFY_OK(gcoOS_Free(context->os, (gctPOINTER)Indices));
    }

	glmLEAVE();
    return;

OnError:

    if (indexPatched && newIndices)
    {
        gcmVERIFY_OK(gcoOS_Free(context->os, (gctPOINTER)Indices));
    }

    /* Convert status into GL error code. */
    switch (status)
    {
    case gcvSTATUS_OUT_OF_MEMORY:
    case gcvSTATUS_OUT_OF_RESOURCES:
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_ARG("0x%04x", GL_OUT_OF_MEMORY);
		glmGetApiEndTime(context);
        return;

    default:
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_OPERATION);
		glmGetApiEndTime(context);
        return;
    }
#else
    do
    {
        if (*elementArrayBuffer == gcvNULL)
        {
            GLsizei i;

#ifndef gcdENABLE_DYNAMIC_STREAM

            /* Construct gcoINDEX object. */
            gcmERR_BREAK(gcoINDEX_Construct(context->hal, &index));

            /* Upload indices. */
            gcmERR_BREAK(gcoINDEX_Load(index,
                                       indexType,
                                       Count,
                                       (gctPOINTER) Indices));
#else
            /* Upload indices. */
            gcmERR_BREAK(gcoINDEX_LoadDynamic(context->dynamicElement,
                                              indexType,
                                              Count,
                                              (gctPOINTER) Indices));
#endif

            /* Find maximum index. */
            switch (Type)
            {
            case GL_UNSIGNED_BYTE:
                minIndex = ~0U;
                maxIndex =  0;
                for (i = 0; i < Count; ++i)
                {
                    GLubyte j = ((GLubyte *) Indices)[i];
                    if (j < minIndex) minIndex = j;
                    if (j > maxIndex) maxIndex = j;
                }
                break;

            case GL_UNSIGNED_SHORT:
                minIndex = ~0U;
                maxIndex =  0;
                for (i = 0; i < Count; ++i)
                {
                    GLushort j = ((GLushort *) Indices)[i];
                    if (j < minIndex) minIndex = j;
                    if (j > maxIndex) maxIndex = j;
                }
                break;

            case GL_UNSIGNED_INT:
                minIndex = ~0U;
                maxIndex =  0;
                for (i = 0; i < Count; ++i)
                {
                    GLuint j = ((GLuint *) Indices)[i];
                    if (j < minIndex) minIndex = j;
                    if (j > maxIndex) maxIndex = j;
                }
                break;

            default:
                gcmFATAL("Huh?");
            }

            /* Special case line loop. */
            if (Mode == GL_LINE_LOOP)
            {
                gcmERR_BREAK(gcoINDEX_UploadOffset(index,
                                                   Count * bytesPerIndex,
                                                   Indices,
                                                   bytesPerIndex));

                primitiveType = gcvPRIMITIVE_LINE_STRIP;
            }
        }
        else
        {
            /* Line loop not yet implemented. */
            gcmASSERT(Mode != GL_LINE_LOOP);

            /* Get current buffer bound to element array. */
            buffer = *elementArrayBuffer;

            /* Bind buffer to the hardware. */
            gcmASSERT(buffer->index != gcvNULL);
            gcmERR_BREAK(gcoINDEX_BindOffset(buffer->index,
                                             indexType,
                                             gcmPTR2INT(Indices)));

            gcmERR_BREAK(gcoINDEX_GetIndexRange(buffer->index,
                                                indexType,
                                                gcmPTR2INT(Indices),
                                                Count,
                                                &minIndex,
                                                &maxIndex));
        }

#ifndef gcdENABLE_DYNAMIC_STREAM
        /* Create gcoVERTEX object. */
        gcmERR_BREAK(gcoVERTEX_Reset(context->vertex));

        gcmERR_BREAK(_ConstructVertex(context,
                                      minIndex, maxIndex - minIndex + 1,
                                      context->vertex,
                                      streams, &streamIndex));
#else
        gcmERR_BREAK(_ConstructDynamicVertex(context,
                                             minIndex,
                                             maxIndex - minIndex + 1));

        if (!gcmIS_SUCCESS(status))
        {
            gcmERR_BREAK(
                gcoVERTEX_Reset(context->vertex));

            gcmERR_BREAK(
                _ConstructVertex(context,
                                 minIndex, maxIndex - minIndex + 1,
                                 context->vertex,
                                 streams, &streamIndex));
        }
#endif

        /* Bind vertices to hardware. */
        gcmERR_BREAK(gcoVERTEX_Bind(context->vertex));

        /* Draw primitives. */
        gcmERR_BREAK(
            _DrawPrimitives(context,
                            primitiveType,
                            0,
                            primitiveCount,
                            gcvTRUE));
    }
    while (gcvFALSE);

    glmPROFILE(context, GL2_PROFILER_PRIMITIVE_END, primitiveType);

    if (indexPatched)
    {
        gcmVERIFY_OK(gcoOS_Free(context->os, (gctPOINTER)Indices));
    }

    for (; streamIndex > 0; --streamIndex)
    {
        /* Destroy temporary gcoSTREAM object. */
        gcmVERIFY_OK(gcoSTREAM_Destroy(streams[streamIndex - 1]));
    }

    if (index != gcvNULL)
    {
        /* Destroy temporary gcoINDEX object. */
        gcmVERIFY_OK(gcoINDEX_Destroy(index));
    }

    if (gcmIS_ERROR(status))
    {
        gcmFATAL("%s(%d): HAL returned status=%d(%s)", __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
        gl2mERROR(GL_INVALID_OPERATION);
    }

    gcmFOOTER_NO();
    glmGetApiEndTime(context);

	}
	glmLEAVE();
    return;
#endif
#endif
}

/*******************************************************************************
**  glDrawArrays
**
**  Render primitives from array data.
**
***** PARAMETERS
**
**  Mode
**
**      Specifies what kind of primitives to render. Symbolic constants
**      GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP,
**      GL_TRIANGLE_FAN, and GL_TRIANGLES are accepted.
**
**  first
**
**      Specifies the starting index in the enabled arrays.
**
**  count
**
**      Specifies the number of indices to be rendered.
**
***** DESCRIPTION
**
**  glDrawArrays specifies multiple geometric primitives with very few
**  subroutine calls. Instead of calling a GL procedure to pass each individual
**  vertex attribute, you can use glVertexAttribPointer to prespecify separate
**  arrays of vertices, normals, and colors and use them to construct a sequence
**  of primitives with a single call to glDrawArrays.
**
**  When glDrawArrays is called, it uses <count> sequential elements from each
**  enabled array to construct a sequence of geometric primitives, beginning
**  with element <first>. <mode> specifies what kind of primitives are
**  constructed and how the array elements construct those primitives.
**
**  To enable and disable a generic vertex attribute array, call
**  glEnableVertexAttribArray and glDisableVertexAttribArray.
**
***** NOTES
**
**  If the current program object, as set by glUseProgram, is invalid, rendering
**  results are undefined. However, no error is generated for this case.
**
***** ERRORS
**
**  GL_INVALID_ENUM is generated if <mode> is not an accepted value.
**
**  GL_INVALID_VALUE is generated if <count> is negative.
**
**  GL_INVALID_FRAMEBUFFER_OPERATION is generated if the currently bound
**  framebuffer is not framebuffer complete (i.e. the return value from
**  glCheckFramebufferStatus is not GL_FRAMEBUFFER_COMPLETE).
*/
GL_APICALL void GL_APIENTRY
glDrawArrays(
    GLenum mode,
    GLint first,
    GLsizei count
    )
{
#if gcdNULL_DRIVER < 3
    gcePRIMITIVE primitiveType;
    gctUINT primitiveCount;
#if GL_USE_VERTEX_ARRAY
    gceSTATUS status;
    gctINT16_PTR indices = gcvNULL;
    gcoINDEX index = gcvNULL;
    gctINT i;
    gctUINT enable;
    gctBOOL lineLoopPatch;
    gcsVERTEXARRAY *vertexArray;
#else
    gceSTATUS status = gcvSTATUS_OK;
    gctINT streamIndex = 0;
    gcoSTREAM streams[gldVERTEX_ELEMENT_COUNT];
    GLAttribute *vertexArray;
#endif

    glmENTER3(glmARGENUM, mode, glmARGINT, first, glmARGINT, count)
    {
#if gcdDUMP_API
    gcmDUMP_API("${ES20 glDrawArrays 0x%08X 0x%08X 0x%08X",
                mode, first, count);
    _DumpVertices(context, first, count);
    gcmDUMP_API("$}");
#endif

    /* Count the call. */
    glmPROFILE(context, GLES2_DRAWARRAYS, 0);

#if gldFBO_DATABASE
    /* Check if we have a current framebuffer and we are not playing
     * back. */
    if (context->dbEnable
        &&
        (context->framebuffer != gcvNULL)
        &&
        !context->playing
        )
    {
        /* Add the draw elements call to the database. */
        if (gcmIS_SUCCESS(glshAddDrawArrays(context, mode, first, count)))
        {
            /* Done. */
            break;
        }

        /* Some error - play back the database. */
        if (gcmIS_ERROR(glshPlayDatabase(context,
                                         context->framebuffer->currentDB)))
        {
            gl2mERROR(GL_INVALID_OPERATION);
            glmERROR_BREAK();
        }
    }

    /* Check dependencies for playback. */
    gcmVERIFY_OK(glshCheckDependency(context));
#endif

    if (context->vertexArrayObject == gcvNULL)
    {
        vertexArray = context->vertexArray;
    }
    else
    {
        vertexArray = context->vertexArrayObject->vertexArray;
    }

#if gldBATCH_OPTIMIZATION
    /* Batch optimization. */
    if (
        /* Nothing has changed in between batches. */
        !context->batchDirty

        /* Alpha blending and stencil shoud be OFF. */
        && (context->blendEnable   == GL_FALSE)
        && (context->stencilEnable == GL_FALSE)

        /* None of the enabled streams can be an array. */
        && ((context->batchEnable & context->batchArray) == 0)

        /* This is the same primitive as before. */
        &&  (context->batchMode      == mode)
        &&  (context->batchFirst     == first)
        &&  (context->batchCount     == count)
        &&  (context->batchIndexType == GL_NONE)

        /* Make sure we don't skip too much. */
        && (++context->batchSkip < gldBATCH_OPTIMIZATION)
    )
    {
        /* Drop the primitive. */
		break;
    }
#endif

    /* Convert <mode> and <count> to primitive type and count. */
    if (!_glshMode2Type(mode, count, &primitiveType, &primitiveCount))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "mode=0x%04x is invalid",
                      mode);

        gl2mERROR(GL_INVALID_ENUM);
		glmERROR_BREAK();
   }

    /* Test for valid <count>. */
    if (count < 0)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Negative count=%lu is invalid",
                      count);

        gl2mERROR(GL_INVALID_VALUE);
        glmERROR_BREAK();
    }

    /* Bail out if there is nothing to render. */
    if (primitiveCount < 1)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "No primitives to render");

        break;
    }

    /* Bail out if there is no program loaded. */
    if (context->program == gcvNULL)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "No program loaded");

        break;
    }

    /* Invalid operation if the program could not be linked. */
    if (context->program->statesSize == 0)
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Program could not be linked");

        break;
    }

    /* Update frame buffer. */
    if (!_glshFrameBuffer(context))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Framebuffer is incomplete");

        break;
    }

    /* Triangles won't be rendered when both front and back faces are culled. */
    if (context->cullEnable
    &&  (context->cullMode == GL_FRONT_AND_BACK)
    &&  (  (mode == GL_TRIANGLES)
        || (mode == GL_TRIANGLE_STRIP)
        || (mode == GL_TRIANGLE_FAN)
        )
    )
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Triangles with both faces culled");

		break;
    }

    /* Map textures. */
    if (!_MapTextures(context))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Could not map all textures");

        gl2mERROR(GL_INVALID_OPERATION);
        glmERROR_BREAK();
    }

#if GC_ENABLE_LOADTIME_OPT
    /* Flush the uniforms. */
    gcmVERIFY(_FlushUniforms(context));
#endif /* GC_ENABLE_LOADTIME_OPT */

    /* Program the shader. */
    if (gcmIS_ERROR(_SetShaders(context, primitiveType)))
    {
        gcmTRACE_ZONE(gcvLEVEL_WARNING, gldZONE_DRAW,
                      "Could not set the shaders.");

        gl2mERROR(GL_INVALID_OPERATION);
        glmERROR_BREAK();
    }

    /* Count the primitives. */
    glmPROFILE(context, GL2_PROFILER_PRIMITIVE_TYPE,  primitiveType);
    glmPROFILE(context, GL2_PROFILER_PRIMITIVE_COUNT, primitiveCount);

#if !GC_ENABLE_LOADTIME_OPT
    /* Flush the uniforms. */
    gcmVERIFY(_FlushUniforms(context));
#endif /* GC_ENABLE_LOADTIME_OPT */

#if GL_USE_VERTEX_ARRAY
    /* Walk all attributes. */
    for (i = enable = 0; i < (gctINT) context->maxAttributes; ++i)
    {
        gctUINT link;

        /* Get linkage of vertex attribute. */
        link = context->program->attributeLinkage[i];
        if (link == (GLuint) -1)
            continue;

        vertexArray[i].linkage = context->program->attributeMap[link];

        if ((link < context->maxAttributes)
        &&  ((context->program->attributeEnable >> link) & 1)
        )
        {
            enable |= (1 << i);
        }
    }

    /* Bind vertices to hardware. */
    gcmONERROR(gcoVERTEXARRAY_Bind(context->vertex,
                                   enable,
                                   vertexArray,
                                   first, count,
                                   gcvINDEX_8, gcvNULL, gcvNULL,
                                   &primitiveType,
                                   &primitiveCount));

    /* Test for line loop patch. */
    lineLoopPatch = (mode == GL_LINE_LOOP)
                 && (primitiveType != gcvPRIMITIVE_LINE_LOOP);

    /* Draw primitives. */
    gcmONERROR(_DrawPrimitives(context,
                               primitiveType,
                               lineLoopPatch ? 0 : first,
                               primitiveCount,
                               lineLoopPatch));

    /* Update batch. */
    context->batchDirty     = GL_FALSE;
    context->batchMode      = mode;
    context->batchFirst     = first;
    context->batchCount     = count;
    context->batchIndexType = GL_NONE;
    context->batchSkip      = 0;

	}
	glmLEAVE();
	return;

OnError:
    if (indices != gcvNULL)
    {
        /* Free the memory. */
        gcmVERIFY_OK(gcmOS_SAFE_FREE(context->os, indices));
    }

    if (index != gcvNULL)
    {
        /* Destroy the gcoINDEX object. */
        gcmVERIFY_OK(gcoINDEX_Destroy(index));
    }

    /* Convert status into GL error code. */
    switch (status)
    {
    case gcvSTATUS_OUT_OF_MEMORY:
    case gcvSTATUS_OUT_OF_RESOURCES:
        gl2mERROR(GL_OUT_OF_MEMORY);
        gcmFOOTER_ARG("0x%04x", GL_OUT_OF_MEMORY);
		glmGetApiEndTime(context);
        return;

    default:
        gl2mERROR(GL_INVALID_OPERATION);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_OPERATION);
		glmGetApiEndTime(context);
        return;
    }
#else
    do
    {
        status = _ConstructDynamicVertex(context, first, count);

        if (!gcmIS_SUCCESS(status))
        {
            gcmERR_BREAK(
                gcoVERTEX_Reset(context->vertex));

            gcmERR_BREAK(
                _ConstructVertex(context,
                                 first, count,
                                 context->vertex,
                                 streams, &streamIndex));
        }

        /* Bind vertices to hardware. */
        gcmERR_BREAK(gcoVERTEX_Bind(context->vertex));

        /* Enable point size for points. */
        gcmERR_BREAK(gco3D_SetPointSizeEnable(context->engine,
                                              mode == GL_POINTS));

        /* Enable point sprites for points. */
        gcmERR_BREAK(gco3D_SetPointSprite(context->engine,
                                          mode == GL_POINTS));

        if (mode == GL_LINE_LOOP)
        {
            gctINT16_PTR indices = gcvNULL;
            gcoINDEX index = gcvNULL;
            gctINT i;

            do
            {
                /* Create indices. */
                gcmERR_BREAK(gcoOS_Allocate(gcvNULL,
                                            (count + 1) * gcmSIZEOF(gctINT16),
                                            (gctPOINTER *) &indices));

                for (i = 0; i < count; ++i)
                {
                    indices[i] = (gctINT16) (first + i);
                }
                indices[i] = (gctINT16) first;

                /* Construct gcoINDEX object. */
                gcmERR_BREAK(gcoINDEX_Construct(context->hal, &index));

                /* Upload indices. */
                gcmERR_BREAK(gcoINDEX_Load(index,
                                           gcvINDEX_16,
                                           count + 1,
                                           (gctPOINTER) indices));

                /* Draw indexed primitives. */
                gcmERR_BREAK(
                    _DrawPrimitives(context,
                                    gcvPRIMITIVE_LINE_STRIP,
                                    0,
                                    primitiveCount,
                                    gcvTRUE));
            }
            while (gcvFALSE);

            if (index != gcvNULL)
            {
                gcmVERIFY_OK(gcoINDEX_Destroy(index));
            }

            if (indices != gcvNULL)
            {
                gcmVERIFY_OK(gcmOS_SAFE_FREE(gcvNULL, indices));
            }
        }
        else
        {
            /* Draw primitives. */
            gcmERR_BREAK(
                _DrawPrimitives(context,
                                primitiveType,
                                first,
                                primitiveCount,
                                gcvFALSE));
        }
    }
    while (gcvFALSE);

    for (; streamIndex > 0; --streamIndex)
    {
        /* Destroy temporary gcoSTREAM object. */
        gcmVERIFY_OK(gcoSTREAM_Destroy(streams[streamIndex - 1]));
    }

    if (gcmIS_ERROR(status))
    {
        gcmFATAL("%s(%d): HAL returned status=%d(%s)", __FUNCTION__, __LINE__, status, gcoOS_DebugStatus2Name(status));
        gl2mERROR(GL_INVALID_OPERATION);
    }

    gcmFOOTER_NO();
	glmGetApiEndTime(context);

	}
	glmLEAVE();
	return;
#endif
#endif
}

/*******************************************************************************
**
**  glMultiDrawArraysEXT
**
**  glMultiDrawArraysEXT Behaves identically to DrawArrays except that a list
**  of arrays is specified instead. The number of lists is specified in the
**  primcount parameter. It has the same effect as:
**
**  for(i=0; i<primcount; i++) {
**      if (*(count+i)>0) DrawArrays(mode, *(first+i), *(count+i));
**  }
**
**  INPUT:
**
**      Mode
**          Specifies what kind of primitives to render. Symbolic constants
**          GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP,
**          GL_TRIANGLE_FAN and GL_TRIANGLES are accepted.
**
**      First
**          Points to an array of starting indices in the enabled arrays.
**
**      Count
**          Points to an array of the number of indices
**          to be rendered.
**
**      Primcount
**          Specifies the size of first and count.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_APICALL void GL_APIENTRY
glMultiDrawArraysEXT(
    GLenum Mode,
    GLint* First,
    GLsizei* Count,
    GLsizei Primcount)
{
#if gcdNULL_DRIVER < 3
    GLsizei i;
    GLContext context;

    gcmHEADER_ARG("mode=0x%04x first=%d count=%ld primcount=%ld", Mode, First, Count, Primcount);

    /* Get current context. */
    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    /* Check Primcount. */
    if (Primcount < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    /* Use the unoptimized way for now. */
    for (i = 0; i < Primcount; ++i)
    {
        if (*(Count + i) > 0)
        {
            glDrawArrays(Mode, *(First + i), *(Count + i));
        }
    }

    gcmFOOTER_NO();
#endif
}

/*******************************************************************************
**
**  glMultiDrawElementsEXT
**
**  glMultiDrawArraysEXT behaves identically to DrawElements except that a list
**  of arrays is specified instead. The number of lists is specified in the
**  primcount parameter. It has the same effect as:
**
**  for(i=0; i<primcount; i++) {
**      if (*(count+i)>0) DrawElements(mode, *(count+i), type,
**                                     *(indices+i));
**  }
**
**  INPUT:
**
**      Mode
**          Specifies what kind of primitives to render. Symbolic constants
**          GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP,
**          GL_TRIANGLE_FAN and GL_TRIANGLES are accepted.
**
**      Count
**          Points to and array of the element counts.
**
**      Type
**          Specifies the type of the values in indices.
**          Must be GL_UNSIGNED_BYTE or GL_UNSIGNED_SHORT.
**
**      Indices
**          Specifies a pointer to the location where the indices are stored.
**
**      Primcount
**          Specifies the size of of the count array.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_APICALL void GL_APIENTRY
glMultiDrawElementsEXT(
    GLenum Mode,
    const GLsizei* Count,
    GLenum Type,
    const GLvoid** Indices,
    GLsizei Primcount
    )
{
#if gcdNULL_DRIVER < 3
    GLsizei i;
    GLContext context;

    gcmHEADER_ARG("Mode=0x%04x Count=%d Type=0x%04x Indices=0x%x Primcount=%ld",
                  Mode, Count, Type, Indices, Primcount);

    /* Get current context. */
    context = _glshGetCurrentContext();
    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    /* Check Primcount. */
    if (Primcount < 0)
    {
        gl2mERROR(GL_INVALID_VALUE);
        gcmFOOTER_ARG("0x%04x", GL_INVALID_VALUE);
        return;
    }

    /* Use the unoptimized way for now. */
    for (i = 0; i < Primcount; ++i)
    {
        if (*(Count + i) > 0)
        {
            glDrawElements(Mode, *(Count + i), Type, *(Indices + i));
        }
    }

    gcmFOOTER_NO();
#endif
}
