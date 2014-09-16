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

#define _GC_OBJ_ZONE    glvZONE_DRAW

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/*******************************************************************************
**
**  glfInitializeDraw/glfDeinitializeDraw
**
**  Draw system start up and shut down routines.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing
*/

gceSTATUS
glfInitializeDraw(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS status;
    gctSIZE_T i;
    gcmHEADER_ARG("Context=0x%x", Context);

    /* Initialize the generic values (not used in OpenGL ES 1.1). */
    for (i = 0; i < gcmCOUNTOF(Context->attributeArray); ++i)
    {
        Context->attributeArray[i].genericValue[0] = 0.0f;
        Context->attributeArray[i].genericValue[1] = 0.0f;
        Context->attributeArray[i].genericValue[2] = 0.0f;
        Context->attributeArray[i].genericValue[3] = 1.0f;

        Context->attributeArray[i].enable = gcvTRUE;
    }

    for (i = 0; i < gldSTREAM_SIGNAL_NUM; i++)
    {
        gcmONERROR(gcoOS_CreateSignal(
            gcvNULL, gcvFALSE, &(Context->streamSignals[i])
            ));

        gcmONERROR(gcoOS_Signal(
            gcvNULL, Context->streamSignals[i], gcvTRUE
            ));
    }

    /* Construct the vertex array. */
    status = gcoVERTEXARRAY_Construct(Context->hal, &Context->vertexArray);

    gcmFOOTER();
    /* Return status. */
    return status;

OnError:
    /* Roll back. */
    gcmVERIFY_OK(glfDeinitializeDraw(Context));

    /* Set error. */
    glmERROR(GL_OUT_OF_MEMORY);

    /* Return status. */
    gcmFOOTER();
    return status;
}

gceSTATUS
glfDeinitializeDraw(
    glsCONTEXT_PTR Context
    )
{
    gceSTATUS   status;
    gctSIZE_T   i;

    gcmHEADER_ARG("Context=0x%x", Context);

    status = gcoVERTEXARRAY_Destroy(Context->vertexArray);

    for (i = 0; i < gldSTREAM_POOL_SIZE; i += 1)
    {
        if (Context->streams[i] != gcvNULL)
        {
            gcmONERROR(gcoSTREAM_Destroy(Context->streams[i]));
            Context->streams[i] = gcvNULL;
        }
    }

    /* before we destroy the signal, we need do stall to make sure
     * the event with signal has been send to hardware, we need this
     * sync, or the destroy maybe before the event signal.
     * _FreeStream will produce this kind of event*/
    gcmONERROR(gcoHAL_Commit(Context->hal, gcvTRUE));

    for (i = 0; i < gldSTREAM_SIGNAL_NUM; i += 1)
    {
        if (Context->streamSignals[i] != gcvNULL)
        {
            gcmONERROR(gcoOS_Signal(
                gcvNULL, Context->streamSignals[i], gcvTRUE
                ));

            gcmONERROR(gcoOS_DestroySignal(
                gcvNULL, Context->streamSignals[i]
                ));

            Context->streamSignals[i] = gcvNULL;
        }
    }

    gcmFOOTER();
    return status;

OnError:
    /* Return status. */
    gcmFOOTER();
    return status;
}

/*******************************************************************************
**
**  _LogicOpPreProcess/_LogicOpPostProcess
**
**  Logic operation emulation.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**  OUTPUT:
**
**      Nothing
*/

#define glvCOLOR_KEY 0x4C3D2E1F

static gceSTATUS _LogicOpPreProcess(
    glsCONTEXT_PTR Context
    )
{
    gctUINT width;
    gctUINT height;
    gctUINT samples;
    gceSURF_FORMAT format;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        /* Get the frame buffer characteristics. */
        gcmERR_BREAK(gcoSURF_GetSize(Context->draw, &width, &height, gcvNULL));
        gcmERR_BREAK(gcoSURF_GetFormat(Context->draw, gcvNULL, &format));
        gcmERR_BREAK(gcoSURF_GetSamples(Context->draw, &samples));

        /* Disable tile status on the render target. */
        gcmERR_BREAK(
            gcoSURF_DisableTileStatus(Context->draw, gcvTRUE));

        /* Create a linear buffer of the same size and format as frame buffer. */
        gcmERR_BREAK(gcoSURF_Construct(
            Context->hal,
            width, height, 1,
            gcvSURF_BITMAP, format,
            gcvPOOL_DEFAULT,
            &Context->fbLinear
            ));

        /* Set the samples. */
        gcmERR_BREAK(gcoSURF_SetSamples(
            Context->fbLinear,
            samples
            ));

        /* Set fbLinear buffer Orientation. */
        gcmERR_BREAK(gcoSURF_SetOrientation(
            Context->fbLinear,
            gcvORIENTATION_BOTTOM_TOP
            ));

        /* Resolve the frame buffer to the linear destination buffer. */
        gcmERR_BREAK(gcoSURF_Resolve(
            Context->draw,
            Context->fbLinear
            ));

        /* Create a temporary frame buffer. */
        gcmERR_BREAK(gcoSURF_Construct(
            Context->hal,
            width, height, 1,
            gcvSURF_RENDER_TARGET_NO_TILE_STATUS,
            gcvSURF_A8R8G8B8,
            gcvPOOL_DEFAULT,
            &Context->tempDraw
            ));

        /* Set the samples. */
        gcmERR_BREAK(gcoSURF_SetSamples(
            Context->tempDraw,
            samples
            ));

        /* Set tempDraw buffer Orientation. */
        gcmERR_BREAK(gcoSURF_SetOrientation(
            Context->tempDraw,
            gcvORIENTATION_BOTTOM_TOP
            ));

        /* Program the PE with the temporary frame buffer. */
        gcmERR_BREAK(gco3D_SetTarget(Context->hw, Context->tempDraw));

        /* Clear the temporary frame buffer with an ugly background color. */
        gcmERR_BREAK(gco3D_SetClearColor(
            Context->hw,
            (gctUINT8) ((glvCOLOR_KEY >> 16) & 0xFF),
            (gctUINT8) ((glvCOLOR_KEY >>  8) & 0xFF),
            (gctUINT8) ((glvCOLOR_KEY >>  0) & 0xFF),
            (gctUINT8) ((glvCOLOR_KEY >> 24) & 0xFF)
            ));

        gcmERR_BREAK(gcoSURF_Clear(Context->tempDraw, gcvCLEAR_COLOR));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the status. */
    return status;
}

static gceSTATUS _LogicOpPostProcess(
    glsCONTEXT_PTR Context
    )
{
    gctUINT width;
    gctUINT height;
    gcoSURF tempLinear = gcvNULL;
    gctUINT samples;
    gceSTATUS status;

    gcmHEADER_ARG("Context=0x%x", Context);

    do
    {
        /* Get the frame buffer characteristics. */
        gcmERR_BREAK(gcoSURF_GetSize(Context->draw, &width, &height, gcvNULL));
        gcmERR_BREAK(gcoSURF_GetSamples(Context->draw, &samples));

        /* Create temporary linear frame buffers. */
        gcmONERROR(gcoSURF_Construct(
            Context->hal,
            width, height, 1,
            gcvSURF_BITMAP, gcvSURF_A8R8G8B8,
            gcvPOOL_DEFAULT,
            &tempLinear
            ));

        /* Set the samples. */
        gcmONERROR(gcoSURF_SetSamples(
            tempLinear,
            samples
            ));

        /* Set tempLinear buffer Orientation. */
        gcmONERROR(gcoSURF_SetOrientation(
            tempLinear,
            gcvORIENTATION_BOTTOM_TOP
            ));

        /* Resolve the temporary frame buffer to the linear source buffer. */
        gcmONERROR(gcoSURF_Resolve(
            Context->tempDraw,
            tempLinear
            ));

        /* Delete the temporary frame buffer. */
        gcmONERROR(gcoSURF_Destroy(Context->tempDraw));
        Context->tempDraw = gcvNULL;

        /* Set the clipping rectangle. */
        gcmONERROR(gcoSURF_SetClipping(Context->fbLinear));

        /* Run the 2D operation using color keying. */
        gcmONERROR(gcoSURF_Blit(
            tempLinear,
            Context->fbLinear,
            1, gcvNULL, gcvNULL,
            gcvNULL,
            Context->logicOp.rop,
            0xAA,
            gcvSURF_SOURCE_MATCH, glvCOLOR_KEY,
            gcvNULL, gcvSURF_UNPACKED
            ));

        /* Flush the frame buffer. */
        gcmONERROR(gcoSURF_Flush(Context->fbLinear));

        /* Tile the linear destination buffer back to the frame buffer. */
        gcmONERROR(gcoSURF_Resolve(Context->fbLinear, Context->draw));

        /* Delete the linear frame buffers. */
        gcmONERROR(gcoSURF_Destroy(tempLinear));
        tempLinear = gcvNULL;
        gcmONERROR(gcoSURF_Destroy(Context->fbLinear));
        Context->fbLinear = gcvNULL;

        /* Reprogram the clear color. */
        switch (Context->clearColor.type)
        {
        case glvFLOAT:
            gcmVERIFY_OK(gco3D_SetClearColorF(
                Context->hw,
                Context->clearColor.value[0].f,
                Context->clearColor.value[1].f,
                Context->clearColor.value[2].f,
                Context->clearColor.value[3].f
                ));
            break;

        case glvFIXED:
            gcmVERIFY_OK(gco3D_SetClearColorX(Context->hw,
                Context->clearColor.value[0].x,
                Context->clearColor.value[1].x,
                Context->clearColor.value[2].x,
                Context->clearColor.value[3].x
                ));
            break;

        default:
            gcmFATAL("Invalid type %d", Context->clearColor.type);
        }

        /* Reprogram the PE. */
        gcmONERROR(gco3D_SetTarget(Context->hw, Context->draw));

        /* Disable tile status on the render target. */
        gcmONERROR(
            gcoSURF_DisableTileStatus(Context->draw, gcvFALSE));
    }
    while (gcvFALSE);

    gcmFOOTER();
    /* Return the staus. */
    return status;

OnError:
	if (tempLinear != gcvNULL)
	{
		gcoSURF_Destroy(tempLinear);
		tempLinear = gcvNULL;
	}
    gcmFOOTER();
	return status;
}


/*******************************************************************************
**
**  _GetPrimitiveCount
**
**  Determines the number of primitives to render given the mode and vertex
**  count.
**
**  INPUT:
**
**      PrimitiveMode
**          Primitive mode.
**
**      VertexCount
**          Vertex count.
**
**  OUTPUT:
**
**      PrimitiveCount
**          Number of primitives.
**
**      HalPrimitive
**          Primitive type for HAL.
*/

static GLboolean _GetPrimitiveCount(
    IN GLenum PrimitiveMode,
    IN GLsizei VertexCount,
    OUT GLsizei * PrimitiveCount,
    OUT gcePRIMITIVE * HalPrimitive
    )
{
    GLboolean result = GL_TRUE;

    gcmHEADER_ARG("PrimitiveMode=0x%04x VertexCount=%d PrimitiveCount=0x%x HalPrimitive=0x%x",
                    PrimitiveMode, VertexCount, PrimitiveCount, HalPrimitive);

    /* Translate primitive count. */
    switch (PrimitiveMode)
    {
    case GL_TRIANGLE_STRIP:
        *PrimitiveCount = VertexCount - 2;
        *HalPrimitive   = gcvPRIMITIVE_TRIANGLE_STRIP;
        break;

    case GL_TRIANGLE_FAN:
        *PrimitiveCount = VertexCount - 2;
        *HalPrimitive   = gcvPRIMITIVE_TRIANGLE_FAN;
        break;

    case GL_TRIANGLES:
        *PrimitiveCount = VertexCount / 3;
        *HalPrimitive   = gcvPRIMITIVE_TRIANGLE_LIST;
        break;

    case GL_POINTS:
        *PrimitiveCount = VertexCount;
        *HalPrimitive   = gcvPRIMITIVE_POINT_LIST;
        break;

    case GL_LINES:
        *PrimitiveCount = VertexCount / 2;
        *HalPrimitive   = gcvPRIMITIVE_LINE_LIST;
        break;

    case GL_LINE_LOOP:
        *PrimitiveCount = VertexCount;
        *HalPrimitive   = gcvPRIMITIVE_LINE_LOOP;
        break;

    case GL_LINE_STRIP:
        *PrimitiveCount = VertexCount - 1;
        *HalPrimitive   = gcvPRIMITIVE_LINE_STRIP;
        break;

    default:
        result = GL_FALSE;
    }

    gcmFOOTER_ARG("result=%d", result);
    /* Return result. */
    return result;
}


/*******************************************************************************
**
**  _IsFullCulled
**
**  Check if need to discard this draw, since HW doesn't support
**  FRONT_AND_BACK cull mode.
**
**  INPUT:
**
**      Context
**          Current drawing context.
**
**      PrimitiveMode
**          Specifies the Primitive Mode.
**
**  OUTPUT:
**
**      GL_TRUE if the all primitive will be culled.
*/

static GLboolean _IsFullCulled(
    IN glsCONTEXT_PTR Context,
    IN GLenum PrimitiveMode
    )
{
    gcmHEADER_ARG("Context=0x%04x PrimitiveMode=%d", Context, PrimitiveMode);

    if (Context->cullStates.enabled
    &&  !Context->drawTexOESEnabled
    &&  (Context->cullStates.cullFace == GL_FRONT_AND_BACK)
    )
    {
        switch (PrimitiveMode)
        {
        case GL_TRIANGLES:
        case GL_TRIANGLE_FAN:
        case GL_TRIANGLE_STRIP:
            gcmFOOTER_ARG("result=%d", GL_TRUE);
            return GL_TRUE;
        }
    }

    gcmFOOTER_ARG("result=%d", GL_FALSE);
    return GL_FALSE;
}


/*******************************************************************************
**
**  _InvalidPalette
**
**  Verify whether proper matrix palette has been specified.
**
**  INPUT:
**
**      Context
**          Current drawing context.
**
**  OUTPUT:
**
**      GL_TRUE if the specified matrix palette is invalid.
*/

static GLboolean _InvalidPalette(
    IN glsCONTEXT_PTR Context
    )
{
    gcmHEADER_ARG("Context=0x%x", Context);
    if
    (
        /* Matrix palette enabled? */
        Context->matrixPaletteEnabled &&
        (
            /* Both streams have to be enabled. */
               !Context->aMatrixIndexInfo.streamEnabled
            || !Context->aWeightInfo.streamEnabled

            /* Number of components has to be within range. */
            || (Context->aMatrixIndexInfo.components < 1)
            || (Context->aMatrixIndexInfo.components > glvMAX_VERTEX_UNITS)

            || (Context->aWeightInfo.components < 1)
            || (Context->aWeightInfo.components > glvMAX_VERTEX_UNITS)
        )
    )
    {
        gcmFOOTER_ARG("result=%d", GL_TRUE);
        return GL_TRUE;
    }

    gcmFOOTER_ARG("result=%d", GL_FALSE);
    return GL_FALSE;
}

static gceSTATUS _VertexArray(
    IN glsCONTEXT_PTR Context,
    IN GLint First,
    IN GLsizei Count,
    IN gceINDEX_TYPE IndexType,
    IN gcoINDEX IndexBuffer,
    IN const void * Indices,
    IN OUT gcePRIMITIVE * PrimitiveType,
    IN OUT gctUINT * PrimitiveCount
    )
{
    gctSIZE_T i, count;
    gctUINT enableBits = 0;
    gcoINDEX index;
    gceSTATUS status;
    gctUINT attr = 0, j;

    gcmHEADER();

    index = IndexBuffer;

    /* Get number of attributes for the vertex shader. */
    gcmONERROR(gcSHADER_GetAttributeCount(Context->currProgram->vs.shader,
                                          &count));

    /* Walk all vertex shader attributes. */
    for (i = 0, j = 0; i < count; ++i)
    {
        gctBOOL attributeEnabled;

        /* Get the attribute linkage. */
        attr = Context->currProgram->vs.attributes[i].binding;

        gcmERR_BREAK(gcATTRIBUTE_IsEnabled(
            Context->currProgram->vs.attributes[i].attribute,
            &attributeEnabled
            ));

        if (attributeEnabled)
        {
            glsATTRIBUTEINFO_PTR attribute = Context->currProgram->vs.attributes[i].info;
            glsNAMEDOBJECT_PTR   buffer = attribute->buffer;

            /* Link the attribute to the vertex shader input. */
            Context->attributeArray[attr].linkage = j++;

            /* Enable the attribute. */
            enableBits |= 1 << attr;

            /* Update stream information. */
            Context->attributeArray[attr].stream   =
                ((buffer != gcvNULL) && (buffer->object != gcvNULL))
                ? ((glsBUFFER_PTR) buffer->object)->stream
                : gcvNULL;
        }
    }

    /* Bind the vertex array to the hardware. */
    gcmONERROR(gcoVERTEXARRAY_Bind(Context->vertexArray,
                                   enableBits, Context->attributeArray,
                                   First, Count,
                                   IndexType, index, (gctPOINTER) Indices,
                                   PrimitiveType,
				                   PrimitiveCount));

    /* Success. */
    gcmFOOTER_NO();
    return gcvSTATUS_OK;

OnError:
    gcmFOOTER();
    return status;
}

#if !defined(COMMON_LITE)
/*
**            glTranslate(0, 1, 0);
**            glRotate(180, 1, 0, 0);
*/
void _applyFlip(glsMATRIX_PTR matrix)
{
    if (matrix->identity)
    {
        if (matrix->type == glvFLOAT)
        {
            glmMATFLOAT(matrix, 3, 0) = glvFRACZERO;
            glmMATFLOAT(matrix, 3, 1) = glvFRACONE;
            glmMATFLOAT(matrix, 3, 2) = glvFRACZERO;
        }
        else
        {
            glmMATFIXED(matrix, 3, 0) = glmFLOAT2FIXED(glvFRACZERO);
            glmMATFIXED(matrix, 3, 1) = glmFLOAT2FIXED(glvFRACONE);
            glmMATFIXED(matrix, 3, 2) = glmFLOAT2FIXED(glvFRACZERO);
        }
    }
    else
    {
        if (matrix->type == glvFLOAT)
        {
            glmMATFLOAT(matrix, 3, 0) = glmMATFLOAT(matrix, 3, 0) + glmMATFLOAT(matrix, 1, 0);
            glmMATFLOAT(matrix, 3, 1) = glmMATFLOAT(matrix, 3, 1) + glmMATFLOAT(matrix, 1, 1);
            glmMATFLOAT(matrix, 3, 2) = glmMATFLOAT(matrix, 3, 2) + glmMATFLOAT(matrix, 1, 2);
            glmMATFLOAT(matrix, 3, 3) = glmMATFLOAT(matrix, 3, 3) + glmMATFLOAT(matrix, 1, 3);
        }
        else
        {
            glmMATFIXED(matrix, 3, 0) = glmMATFIXED(matrix, 3, 0) + glmMATFIXED(matrix, 1, 0);
            glmMATFIXED(matrix, 3, 1) = glmMATFIXED(matrix, 3, 1) + glmMATFIXED(matrix, 1, 1);
            glmMATFIXED(matrix, 3, 2) = glmMATFIXED(matrix, 3, 2) + glmMATFIXED(matrix, 1, 2);
            glmMATFIXED(matrix, 3, 3) = glmMATFIXED(matrix, 3, 3) + glmMATFIXED(matrix, 1, 3);
        }
    }

    if (matrix->type == glvFLOAT)
    {
        glmMATFLOAT(matrix, 1, 0) = glvFRACZERO - (glmMATFLOAT(matrix, 1, 0));
        glmMATFLOAT(matrix, 1, 1) = glvFRACZERO - (glmMATFLOAT(matrix, 1, 1));
        glmMATFLOAT(matrix, 1, 2) = glvFRACZERO - (glmMATFLOAT(matrix, 1, 2));
        glmMATFLOAT(matrix, 1, 3) = glvFRACZERO - (glmMATFLOAT(matrix, 1, 3));
        glmMATFLOAT(matrix, 2, 0) = glvFRACZERO - (glmMATFLOAT(matrix, 2, 0));
        glmMATFLOAT(matrix, 2, 1) = glvFRACZERO - (glmMATFLOAT(matrix, 2, 1));
        glmMATFLOAT(matrix, 2, 2) = glvFRACZERO - (glmMATFLOAT(matrix, 2, 2));
        glmMATFLOAT(matrix, 2, 3) = glvFRACZERO - (glmMATFLOAT(matrix, 2, 3));
    }
    else
    {
        glmMATFIXED(matrix, 1, 0) = gcvZERO_X - (glmMATFIXED(matrix, 1, 0));
        glmMATFIXED(matrix, 1, 1) = gcvZERO_X - (glmMATFIXED(matrix, 1, 1));
        glmMATFIXED(matrix, 1, 2) = gcvZERO_X - (glmMATFIXED(matrix, 1, 2));
        glmMATFIXED(matrix, 1, 3) = gcvZERO_X - (glmMATFIXED(matrix, 1, 3));
        glmMATFIXED(matrix, 2, 0) = gcvZERO_X - (glmMATFIXED(matrix, 2, 0));
        glmMATFIXED(matrix, 2, 1) = gcvZERO_X - (glmMATFIXED(matrix, 2, 1));
        glmMATFIXED(matrix, 2, 2) = gcvZERO_X - (glmMATFIXED(matrix, 2, 2));
        glmMATFIXED(matrix, 2, 3) = gcvZERO_X - (glmMATFIXED(matrix, 2, 3));
    }

    matrix->identity = GL_FALSE;
}

/*******************************************************************************
**
**  _FlipBottomTopTextures
**
**  Apply the following transformation to flip Y (i.e. 180 degree rotation).
**
**            glTranslatef(1, 1, 0);
**            glRotatef(-180, 0, 0, 1);
**
**  INPUT:
**
**      Context
**          Current drawing context.
**
*/
static gceSTATUS _FlipBottomTopTextures(
    IN glsCONTEXT_PTR Context,
    OUT GLboolean * Flipped
    )
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER();

    *Flipped = gcvFALSE;

    do
    {
        gctUINT i;

        /* Iterate though the attributes. */
        for (i = 0; i < (gctUINT) Context->texture.pixelSamplers; i++)
        {
            glsTEXTURESAMPLER_PTR sampler;
            glsTEXTUREWRAPPER_PTR texture;
            gcoSURF surface = gcvNULL;
            gceORIENTATION orientation = gcvORIENTATION_TOP_BOTTOM;

            /* Get a shortcut to the current sampler. */
            sampler = &Context->texture.sampler[i];

            /* Skip the stage if disabled. */
            if (!sampler->stageEnabled)
            {
                continue;
            }

            gcmASSERT(sampler->binding != gcvNULL);
            gcmASSERT(sampler->binding->object != gcvNULL);

            /* Get a shortcut to the current sampler's bound texture. */
            texture = sampler->binding;

            if (gcmIS_ERROR(gcoTEXTURE_GetMipMap(texture->object, 0, &surface)))
            {
                gcmTRACE(gcvLEVEL_WARNING, "Failed to retrieve surface from texture. Ignored.");
                continue;
            }
            gcmASSERT(surface != gcvNULL);

            if (gcmIS_ERROR(gcoSURF_QueryOrientation(surface, &orientation)))
            {
                gcmTRACE(gcvLEVEL_WARNING, "Failed to retrieve orientation from surface. Ignored.");
                continue;
            }

            if (orientation == gcvORIENTATION_BOTTOM_TOP)
            {
                glsMATRIX_PTR topMatrix;
                gcmTRACE(gcvLEVEL_VERBOSE, "texture %d is %s", i, (orientation ? "bottom-top" : "top-bottom"));

                topMatrix = Context->matrixStackArray[glvTEXTURE_MATRIX_0 + i].topMatrix;

                _applyFlip(topMatrix);

                (*Context->matrixStackArray[glvTEXTURE_MATRIX_0 + i].dataChanged)(Context);

                *Flipped = gcvTRUE;
            }
        }
    }
    while (gcvFALSE);

    gcmFOOTER_NO();
    return status;
}
#endif

#if gcdDUMP_API
static void
_DumpVertices(
    IN glsCONTEXT_PTR Context,
    IN GLint First,
    IN GLsizei Count
    )
{
	gctINT i;

	if(Context->arrayBuffer == gcvNULL)		/* Not using vbo. */
    {
		if (Context->aPositionInfo.streamEnabled &&
			Context->aPositionInfo.pointer != gcvNULL)
		{
			gcmDUMP_API_DATA((gctUINT8_PTR)Context->aPositionInfo.pointer +
				First * Context->aPositionInfo.stride,
				Count * Context->aPositionInfo.stride);
		}
		if (Context->aColorInfo.streamEnabled)
		{
			gcmDUMP_API_DATA((gctUINT8_PTR)Context->aColorInfo.pointer +
				First * Context->aColorInfo.stride,
				Count * Context->aColorInfo.stride);
		}
		if (Context->aNormalInfo.streamEnabled)
		{
			gcmDUMP_API_DATA((gctUINT8_PTR)Context->aNormalInfo.pointer +
				First * Context->aNormalInfo.stride,
				Count * Context->aNormalInfo.stride);
		}

		/* Check each texture unit. */
		for (i = 0; i < Context->texture.pixelSamplers; i++)
		{
			glsTEXTURESAMPLER_PTR sampler = Context->texture.sampler + i;
			if (sampler->aTexCoordInfo.streamEnabled &&
				sampler->aTexCoordInfo.pointer != gcvNULL)
			{
				gcmDUMP_API_DATA((gctUINT8_PTR)sampler->aTexCoordInfo.pointer +
					First * sampler->aTexCoordInfo.stride,
					Count * sampler->aTexCoordInfo.stride);
			}
		}
    }
    else									/* Using a vbo. */
    {
    }
}
#endif

/******************************************************************************\
************************* OpenGL Primitive Drawing Code ************************
\******************************************************************************/

/*******************************************************************************
**
**  glDrawArrays
**
**  glDrawArrays specifies multiple geometric primitives with very few
**  subroutine calls. Instead of calling a GL procedure to pass each individual
**  vertex, normal, texture coordinate, edge flag, or color, you can prespecify
**  separate arrays of vertices, normals, and colors and use them to construct
**  a sequence of primitives with a single call to glDrawArrays.
**
**  When glDrawArrays is called, it uses count sequential elements from each
**  enabled array to construct a sequence of geometric primitives, beginning
**  with element first. mode specifies what kind of primitives are constructed
**  and how the array elements construct those primitives. If GL_VERTEX_ARRAY
**  is not enabled, no geometric primitives are generated.
**
**  Vertex attributes that are modified by glDrawArrays have an unspecified
**  value after glDrawArrays returns. For example, if GL_COLOR_ARRAY is enabled,
**  the value of the current color is undefined after glDrawArrays executes.
**  Attributes that aren't modified remain well defined.
**
**  INPUT:
**
**      Mode
**          Specifies what kind of primitives to render. Symbolic constants
**          GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP,
**          GL_TRIANGLE_FAN and GL_TRIANGLES are accepted.
**
**      First
**          Specifies the starting index in the enabled arrays.
**
**      Count
**          Specifies the number of indices to be rendered.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glDrawArrays(
    GLenum Mode,
    GLint First,
    GLsizei Count
    )
{
    gceSTATUS status = gcvSTATUS_OK;
    GLsizei count = Count;
    GLboolean lineLoopPatch;
#if !defined(COMMON_LITE)
	GLboolean flippedTextures = GL_FALSE;
#endif

    glmENTER3(glmARGENUM, Mode, glmARGINT, First, glmARGINT, Count)
    {
		glmPROFILE(context, GLES1_DRAWARRAYS, 0);
        do
        {
            GLsizei i, primitiveCount;
            gcePRIMITIVE halPrimitive;

#if gcdDUMP_API
			gcmDUMP_API("${ES11 glDrawArrays 0x%08X 0x%08X 0x%08X",
						Mode, First, Count);
			_DumpVertices(context, First, Count);
			gcmDUMP_API("$}");
#endif

            /* Check the count and first. */
            if ((Count < 0) || (First < 0) || (First + Count < 0))
            {
                glmERROR(GL_INVALID_VALUE);
                break;
            }

            /* Validate mode and determine the number of primitives. */
            if (!_GetPrimitiveCount(Mode,
                                    Count,
                                    &primitiveCount,
                                    &halPrimitive))
            {
                glmERROR(GL_INVALID_ENUM);
                break;
            }

            /* Don't do anything if primitive count is invalid. */
            if ((primitiveCount <= 0)
                /* Or primitive is fully culled. */
            ||  _IsFullCulled(context, Mode)
                /* Or matrix palette is invalid. */
            ||  _InvalidPalette(context)
            )
            {
                break;
            }

		    /* Count the primitives. */
		    glmPROFILE(context, GL1_PROFILER_PRIMITIVE_TYPE,  halPrimitive);
		    glmPROFILE(context, GL1_PROFILER_PRIMITIVE_COUNT, primitiveCount);

            /* Update frame buffer. */
            gcmERR_BREAK(glfUpdateFrameBuffer(context));

            /* Update clipping box. */
            gcmERR_BREAK(glfUpdateClpping(context));

            /* Completely clipped out? */
            if (context->viewportStates.clippedOut)
            {
                break;
            }

            /* Update stencil states. */
            gcmERR_BREAK(glfUpdateStencil(context));

            /* Update polygon offset states. */
            gcmERR_BREAK(glfUpdatePolygonOffset(context));

            /* Update primitive info. */
            gcmERR_BREAK(glfUpdatePrimitveType(context, Mode));

            /* Update texture states. */
            gcmERR_BREAK(glfUpdateTextureStates(context));

#if !defined(COMMON_LITE)
            /* Flip bottom-top textures. */
            gcmERR_BREAK(_FlipBottomTopTextures(context, &flippedTextures));
#endif
            if (context->useFragmentProcessor)
            {
                /* Program the fragment processor. */
                gcmERR_BREAK(glfUpdateFragmentProcessor(context));
            }
            else
            {
                /* Load shader program. */
                gcmERR_BREAK(glfLoadShader(context));
            }

            /* Load texture states. */
            gcmERR_BREAK(glfLoadTexture(context));

            /* Setup the vertex array. */
            gcmERR_BREAK(_VertexArray(context,
                                      First, count,
                                      gcvINDEX_8, gcvNULL, gcvNULL,
                                      &halPrimitive,
                                      (gctUINT *)(&primitiveCount)));

            /* Test for line loop patch. */
            lineLoopPatch = (Mode == GL_LINE_LOOP)
                         && (halPrimitive != gcvPRIMITIVE_LINE_LOOP);

            /* LOGICOP enabled? */
            if (context->logicOp.perform)
            {
                /* In LOGICOP we have to do one primitive at a time. */
                for (i = 0; i < primitiveCount; i++)
                {
                    /* Create temporary target as destination. */
                    gcmERR_BREAK(_LogicOpPreProcess(context));

                    if (lineLoopPatch)
                    {
                        /* Draw the primitives. */
                        gcmERR_BREAK(gco3D_DrawIndexedPrimitives(
                            context->hw,
                            gcvPRIMITIVE_LINE_STRIP,
                            0, i, 1
                            ));
                    }
                    else
                    {
                        /* Draw the primitives. */
                        gcmERR_BREAK(gco3D_DrawPrimitives(
                            context->hw,
                            halPrimitive,
                            First + i, 1
                            ));
                    }

                    /* Run the post processing pass. */
                    gcmERR_BREAK(_LogicOpPostProcess(context));
                }
            }
            else
            {
                if (lineLoopPatch)
                {
                    /* Draw the primitives. */
                    gcmERR_BREAK(gco3D_DrawIndexedPrimitives(
                        context->hw,
                        gcvPRIMITIVE_LINE_STRIP,
                        0, 0, primitiveCount
                        ));
                }
                else
                {
                    /* Draw the primitives. */
                    gcmERR_BREAK(gco3D_DrawPrimitives(
                        context->hw,
                        halPrimitive,
                        First, primitiveCount
                        ));
                }
            }
#if !defined(COMMON_LITE)
            /* Restore flipped bottom-top textures. */
            if (flippedTextures)
            {
                gcmERR_BREAK(_FlipBottomTopTextures(context, &flippedTextures));
            }
#endif

#if defined(ANDROID) && gldENABLE_MEMORY_REDUCTION
            /* Memory reduction. */
            for (i = 0; i < context->texture.pixelSamplers; i++)
            {
                glsTEXTURESAMPLER_PTR sampler = &context->texture.sampler[i];

                if ((sampler->enableTexturing
                  || sampler->enableExternalTexturing)
                &&  (sampler->binding->source != gcvNULL
                  || sampler->binding->direct.source != gcvNULL)
                &&  (sampler->binding->object != gcvNULL)
                )
                {
                    /* Dynamically free mipmap level 0 for EGLImage case. */
                    /* Get shortcuts. */
                    glsTEXTUREWRAPPER_PTR texture = sampler->binding;

                    /* Destroy texture to destroy mipmap surfaces.
                     * And then allocate an empty texture object. */
                    gcmERR_BREAK(gcoTEXTURE_Destroy(texture->object));

                    texture->object   = gcvNULL;
                    texture->uploaded = gcvFALSE;
                }
            }
#endif

		    glmPROFILE(context, GL1_PROFILER_PRIMITIVE_END, halPrimitive);
        }
        while (gcvFALSE);
    }
    glmLEAVE();
}

#if gcdNULL_DRIVER < 3
#if gcdUSE_TRIANGLE_STRIP_PATCH
static GLboolean
_PatchIndex(
    IN glsCONTEXT_PTR Context,
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
    GLubyte *  base        = gcvNULL;
    GLint      elementSize = 0;
    GLubyte *  start       = gcvNULL;
    GLint      triangles   = 0;
    gceSTATUS  status      = gcvSTATUS_OK;
    GLint      sub         = 0;
    GLint      i = 0, j = 0;

    glsNAMEDOBJECT_PTR wrapper;
    glsBUFFER_PTR  boundIndex = gcvNULL;

    gcmASSERT(patchedIndices != gcvNULL);
    gcmASSERT(patchedMode    != gcvNULL);
    gcmASSERT(patchedCount   != gcvNULL);
    gcmASSERT(patchedIndex   != gcvNULL);

    *patchedIndices = Indices;
    *patchedMode    = Mode;
    *patchedCount   = Count;

    wrapper = Context->elementArrayBuffer;
    if((wrapper != gcvNULL) && (wrapper->object != gcvNULL))
    {
         boundIndex    = (glsBUFFER_PTR)Context->elementArrayBuffer->object;
         *patchedIndex = boundIndex->index;
    }
    else
    {
         *patchedIndex = gcvNULL;
         boundIndex    = gcvNULL;
    }
    *newIndices = gcvFALSE;

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

/*******************************************************************************
**
**  glDrawElements
**
**  glDrawElements specifies multiple geometric primitives with very few
**  subroutine calls. Instead of calling a GL function to pass each individual
**  vertex, normal, texture coordinate, edge flag, or color, you can prespecify
**  separate arrays of vertices, normals, and so on, and use them to construct
**  a sequence of primitives with a single call to glDrawElements.
**
**  When glDrawElements is called, it uses count sequential elements from an
**  enabled array, starting at indices to construct a sequence of geometric
**  primitives. mode specifies what kind of primitives are constructed and how
**  the array elements construct these primitives. If more than one array is
**  enabled, each is used. If GL_VERTEX_ARRAY is not enabled, no geometric
**  primitives are constructed.
**
**  Vertex attributes that are modified by glDrawElements have an unspecified
**  value after glDrawElements returns. For example, if GL_COLOR_ARRAY is
**  enabled, the value of the current color is undefined after glDrawElements
**  executes. Attributes that aren't modified maintain their previous values.
**
**  INPUT:
**
**      Mode
**          Specifies what kind of primitives to render. Symbolic constants
**          GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP,
**          GL_TRIANGLE_FAN and GL_TRIANGLES are accepted.
**
**      Count
**          Specifies the number of elements to be rendered.
**
**      Type
**          Specifies the type of the values in indices.
**          Must be GL_UNSIGNED_BYTE or GL_UNSIGNED_SHORT.
**
**      Indices
**          Specifies a pointer to the location where the indices are stored.
**
**  OUTPUT:
**
**      Nothing.
*/

GL_API void GL_APIENTRY glDrawElements(
    GLenum Mode,
    GLsizei Count,
    GLenum Type,
    const GLvoid* Indices
    )
{
    gceSTATUS       status          = gcvSTATUS_OK;
	GLboolean		indexPatched = GL_FALSE;
    GLsizei         primitiveCount  = 0;
    gcePRIMITIVE    halPrimitive;
    gceINDEX_TYPE   indexType;
    glsNAMEDOBJECT_PTR    elementBuffer;
    gcoINDEX              indexBuffer = gcvNULL;
    gctBOOL         newIndices = gcvFALSE;

    glmENTER4(glmARGENUM, Mode, glmARGINT, Count, glmARGENUM, Type,
              glmARGPTR, Indices)
    {
		glmPROFILE(context, GLES1_DRAWELEMENTS, 0);
        do
        {
            /* Check the count. */
            if (Count < 0)
            {
                glmERROR(GL_INVALID_VALUE);
                break;
            }

            elementBuffer = context->elementArrayBuffer;

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
                    indexBuffer = ((elementBuffer != gcvNULL) && (elementBuffer->object != gcvNULL))
                                ? ((glsBUFFER_PTR)elementBuffer->object)->index
                                : gcvNULL;
                }
            }
            else
#endif
            {
                indexBuffer = ((elementBuffer != gcvNULL) && (elementBuffer->object != gcvNULL))
                            ? ((glsBUFFER_PTR)elementBuffer->object)->index
                            : gcvNULL;
            }

            /* Validate mode and determine the number of primitives. */
            if (!_GetPrimitiveCount(Mode, Count, &primitiveCount, &halPrimitive))
            {
                glmERROR(GL_INVALID_ENUM);
                break;
            }

            /* Validate and translate the index type. */
            if (Type == GL_UNSIGNED_BYTE)
            {
                indexType = gcvINDEX_8;
            }
            else if (Type == GL_UNSIGNED_SHORT)
            {
                indexType = gcvINDEX_16;
            }
            else if (Type == GL_UNSIGNED_INT)
            {
                indexType = gcvINDEX_32;
            }
            else
            {
                glmERROR(GL_INVALID_ENUM);
                break;
            }

            if
            (
                   (primitiveCount <= 0)        /* ...primitive count is invalid. */
                || _IsFullCulled(context, Mode) /* ...primitive is fully culled. */
                || _InvalidPalette(context)     /* ...matrix palette is invalid. */
            )
            {
                break;
            }

            /* Update frame buffer. */
            gcmERR_BREAK(glfUpdateFrameBuffer(context));

            /* Update clipping box. */
            gcmERR_BREAK(glfUpdateClpping(context));

            /* Completely clipped out? */
            if (context->viewportStates.clippedOut)
            {
                break;
            }

		    /* Count the primitives. */
		    glmPROFILE(context, GL1_PROFILER_PRIMITIVE_TYPE,  halPrimitive);
		    glmPROFILE(context, GL1_PROFILER_PRIMITIVE_COUNT, primitiveCount);

            /* Update stencil states. */
            gcmERR_BREAK(glfUpdateStencil(context));

            /* Update polygon offset states. */
            gcmERR_BREAK(glfUpdatePolygonOffset(context));

            /* Update primitive info. */
            gcmERR_BREAK(glfUpdatePrimitveType(context, Mode));

            /* Update texture states. */
            gcmERR_BREAK(glfUpdateTextureStates(context));

            if (context->useFragmentProcessor)
            {
                /* Program the fragment processor. */
                gcmERR_BREAK(glfUpdateFragmentProcessor(context));
            }
            else
            {
                /* Load shader program. */
                gcmERR_BREAK(glfLoadShader(context));
            }

            /* Load texture states. */
            gcmERR_BREAK(glfLoadTexture(context));


#if gcdDUMP_API
			gcmDUMP_API("${ES11 glDrawElements 0x%08X 0x%08X 0x%08X (0x%08X)",
						Mode, Count, Type, Indices);
			{
				GLint min = -1, max = -1, element;
				GLsizei i;
				glsNAMEDOBJECT_PTR elementArrayBuffer = context->elementArrayBuffer;

				if (elementArrayBuffer == gcvNULL)		/* No Buffer Object. */
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
					glsBUFFER_PTR object = (glsBUFFER_PTR)elementArrayBuffer->object;
					switch (Type)
					{
					case GL_UNSIGNED_BYTE:
						gcoINDEX_GetIndexRange(object->index,
											   gcvINDEX_8,
											   (gctUINT32) Indices,
											   Count,
											   (gctUINT32_PTR) &min,
											   (gctUINT32_PTR) &max);
						break;

					case GL_UNSIGNED_SHORT:
						gcoINDEX_GetIndexRange(object->index,
											   gcvINDEX_16,
											   (gctUINT32) Indices,
											   Count,
											   (gctUINT32_PTR) &min,
											   (gctUINT32_PTR) &max);
						break;

					case GL_UNSIGNED_INT:
						gcoINDEX_GetIndexRange(object->index,
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

				if (min >= 0)
				{
					_DumpVertices(context, 0, max + 1);
				}
			}
			gcmDUMP_API("$}");
#endif

            /* Setup the vertex array. */
            gcmERR_BREAK(_VertexArray(context,
                                      0, Count,
                                      indexType,
                                      indexBuffer,
                                      Indices,
                                      &halPrimitive,
                                      (gctUINT *)(&primitiveCount)));

            /* Draw the primitives. */
            gcmERR_BREAK(gco3D_DrawIndexedPrimitives(
                context->hw,
                halPrimitive,
                0,
                0,
                primitiveCount
                ));

            glmPROFILE(context, GL1_PROFILER_PRIMITIVE_END, halPrimitive);

        }
        while (gcvFALSE);

        /*******************************************************************************
        ** Bypass for stream cache limitation bug2631.
        ** If our vertex data is bigger than 1M which is stream cache size,
        ** _VertexArray just returns gcvSTATUS_INVALID_REQUEST and doesn't continue to
        ** render.
        ** We have to use _BuildStream to create a new stream and allocate enough memroy
        ** to store vertex data. This stream doesn't use cache.
        ** After using this stream, we'll release it in glfDeinitializeDraw.
        *******************************************************************************/
        if (status == gcvSTATUS_INVALID_REQUEST)
        {
            GLint       first = 0;
            glsSTREAM   stream;

            do
            {
                gcmERR_BREAK(_BuildStream(context,
                                          0,
                                          0,
                                          Count,
                                          indexType,
                                          Indices,
                                          &stream,
                                          &first));

                gcmERR_BREAK(gco3D_DrawIndexedPrimitives(
                    context->hw,
                    halPrimitive,
                    0,
                    first,
                    primitiveCount
                    ));
            }
            while (gcvFALSE);

            /* Destroy the stream. */
            _FreeStream(context, &stream);
        }

        if (indexPatched && newIndices)
        {
            gcmVERIFY_OK(gcoOS_Free(context->os, (gctPOINTER)Indices));
        }
    }
    glmLEAVE();
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

GL_API void GL_APIENTRY glMultiDrawArraysEXT(
    GLenum Mode,
    GLint* First,
    GLsizei* Count,
    GLsizei Primcount)
{
    gceSTATUS status = gcvSTATUS_OK;

    glmENTER4(glmARGENUM, Mode, glmARGPTR, First, glmARGPTR, Count, glmARGINT, Primcount)
    {
        do
        {
            GLsizei i, j, primitiveCount;
            gcePRIMITIVE halPrimitive;
            GLboolean lineLoopPatch;

            /* Check Primcount. */
            if (Primcount < 0)
            {
                glmERROR(GL_INVALID_VALUE);
                break;
            }

            /* Check all the count and first for error before proceeding. */
            for (j = 0; j < Primcount; j++)
            {
                if ((Count[j] < 0)
                ||  (First[j] < 0)
                )
                {
					glmERROR(GL_INVALID_VALUE);
                    break;
                }
            }

            if (j != Primcount)
            {
                /* Some error occurred in above loop. */
                break;
            }

            if
            (
                _IsFullCulled(context, Mode) /* ...primitive is fully culled. */
                || _InvalidPalette(context)     /* ...matrix palette is invalid. */
            )
            {
                break;
            }

            /* Initialize common states for all the primitives. */

            /* Update frame buffer. */
            gcmERR_BREAK(glfUpdateFrameBuffer(context));

            /* Update clipping box. */
            gcmERR_BREAK(glfUpdateClpping(context));

            /* Completely clipped out? */
            if (context->viewportStates.clippedOut)
            {
                break;
            }

            /* Update stencil states. */
            gcmERR_BREAK(glfUpdateStencil(context));

            /* Update polygon offset states. */
            gcmERR_BREAK(glfUpdatePolygonOffset(context));

            /* Update primitive info. */
            gcmERR_BREAK(glfUpdatePrimitveType(context, Mode));

            /* Update texture states. */
            gcmERR_BREAK(glfUpdateTextureStates(context));

            if (context->useFragmentProcessor)
            {
                /* Program the fragment processor. */
                gcmERR_BREAK(glfUpdateFragmentProcessor(context));
            }
            else
            {
                /* Load shader program. */
                gcmERR_BREAK(glfLoadShader(context));
            }

            /* Load texture states. */
            gcmERR_BREAK(glfLoadTexture(context));

			if (Mode != GL_LINE_LOOP)
			{
				GLint first = First[0];
				GLsizei last = first + Count[0];

                /* Union all the First/Count pairs. */
				for (j = 1; j < Primcount; j++)
				{
					if (first > First[j])
					{
						first = First[j];
					}
					if (First[j] + Count[j] > last)
					{
						last = First[j] + Count[j];
					}
				}

				if(_GetPrimitiveCount(Mode, Count[0], &primitiveCount, &halPrimitive)
					!= GL_TRUE)
				{
					glmERROR(GL_INVALID_VALUE);
					break;
				}


                /* Setup the vertex array. */
                gcmERR_BREAK(_VertexArray(context,
                                          first, last - first,
                                          gcvINDEX_8, gcvNULL, gcvNULL,
                                          &halPrimitive,
                                          (gctUINT *)(&primitiveCount)));

                /* Draw all the primitives at once. */
                gcmERR_BREAK(gco3D_DrawPrimitivesCount(
                    context->hw,
                    halPrimitive,
                    First, (gctSIZE_T*)Count,
					Primcount
                    ));
			}
			else
			{
                for (j = 0; j < Primcount; j++)
                {
                    if(_GetPrimitiveCount(Mode, Count[j], &primitiveCount, &halPrimitive)
					    != GL_TRUE)
				    {
					    glmERROR(GL_INVALID_VALUE);
                        break;
				    }

                    if (primitiveCount <= 0)
                    {
                        continue;
                    }

                    /* Setup the vertex array. */
                    gcmERR_BREAK(_VertexArray(context,
                                              First[j], Count[j],
                                              gcvINDEX_8, gcvNULL, gcvNULL,
                                              &halPrimitive,
                                              (gctUINT *)(&primitiveCount)));

                    /* Test for line loop patch. */
                    lineLoopPatch = (Mode == GL_LINE_LOOP)
                                 && (halPrimitive != gcvPRIMITIVE_LINE_LOOP);

                    /* LOGICOP enabled? */
                    if (context->logicOp.perform)
                    {
                        /* In LOGICOP we have to do one primitive at a time. */
                        for (i = 0; i < primitiveCount; i++)
                        {
                            /* Create temporary target as destination. */
                            gcmERR_BREAK(_LogicOpPreProcess(context));

                            /* Draw the primitives. */
                            if (lineLoopPatch)
                            {
                                gcmERR_BREAK(gco3D_DrawIndexedPrimitives(
                                    context->hw,
                                    gcvPRIMITIVE_LINE_STRIP,
                                    0, i, 1
                                    ));
                            }
                            else
                            {
                                gcmERR_BREAK(gco3D_DrawPrimitives(
                                    context->hw,
                                    halPrimitive,
                                    First[j] + i, 1
                                    ));
                            }

                            /* Run the post processing pass. */
                            gcmERR_BREAK(_LogicOpPostProcess(context));
                        }
                    }
                    else
                    {
                        /* Draw the primitives. */
                        if (lineLoopPatch)
                        {
                            gcmERR_BREAK(gco3D_DrawIndexedPrimitives(
                                context->hw,
                                gcvPRIMITIVE_LINE_STRIP,
                                0, 0, primitiveCount
                                ));
                        }
                        else
                        {
                            gcmERR_BREAK(gco3D_DrawPrimitives(
                                context->hw,
                                halPrimitive,
                                First[j], primitiveCount
                                ));
                        }
                    }
                }
			}

        }
        while (gcvFALSE);
    }
    glmLEAVE();
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

GL_API void GL_APIENTRY glMultiDrawElementsEXT(
    GLenum Mode,
    const GLsizei* Count,
    GLenum Type,
    const GLvoid** Indices,
    GLsizei Primcount
    )
{
    glmENTER5(glmARGENUM, Mode, glmARGPTR, Count, glmARGENUM, Type,
              glmARGPTR, Indices, glmARGINT, Primcount)
    {
        do
        {
            GLsizei i;

            /* Check Primcount. */
            if (Primcount < 0)
            {
                glmERROR(GL_INVALID_VALUE);
                break;
            }

			/* Use the unoptimized way for now. */
			for (i = 0; i < Primcount; ++i)
			{
				if (*(Count + i) > 0)
				{
					glDrawElements(Mode, *(Count + i), Type, *(Indices + i));
				}
			}


        }
        while (gcvFALSE);
    }
    glmLEAVE();
}

/*******************************************************************************
**
**  _GetArrayRange
**
**  Scan the index buffer to determine the active range of enabled arrays.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      IndexCount
**          Number of indices in the index buffer.
**
**      IndexType
**          Index type.
**
**      IndexBuffer
**          Pointer to the index buffer.
**
**  OUTPUT:
**
**      Length
**          Length of enabled arrays.
**
**      Length
**          Length of enabled arrays.
*/
static gceSTATUS _GetArrayRange(
    IN glsCONTEXT_PTR Context,
    IN GLsizei IndexCount,
    IN gceINDEX_TYPE IndexType,
    IN const GLvoid* IndexBuffer,
    OUT GLint * First,
    OUT GLsizei * Count
    )
{
    gceSTATUS status;
    gctUINT32 minIndex;
    gctUINT32 maxIndex;

    /* Is index buffer buffered? */
    if (Context->elementArrayBuffer)
    {
        /* Cast the buffer object. */
        glsBUFFER_PTR object =
            (glsBUFFER_PTR) Context->elementArrayBuffer->object;

        /* Get the index range. */
        status = gcoINDEX_GetIndexRange(
            object->index,
            IndexType,
            gcmPTR2INT(IndexBuffer),
            IndexCount,
            &minIndex,
            &maxIndex
            );
    }
    else
    {
        GLsizei i;
        minIndex = ~0U;
        maxIndex =  0;

        /* Assume success. */
        status = gcvSTATUS_OK;

        switch (IndexType)
        {
        case gcvINDEX_8:
            {
                gctUINT8_PTR indexBuffer = (gctUINT8_PTR) IndexBuffer;

                for (i = 0; i < IndexCount; i++)
                {
                    gctUINT32 curIndex = *indexBuffer++;

                    if (curIndex < minIndex)
                    {
                        minIndex = curIndex;
                    }

                    if (curIndex > maxIndex)
                    {
                        maxIndex = curIndex;
                    }
                }
            }
            break;

        case gcvINDEX_16:
            {
                gctUINT16_PTR indexBuffer = (gctUINT16_PTR) IndexBuffer;

                for (i = 0; i < IndexCount; i++)
                {
                    gctUINT32 curIndex = *indexBuffer++;

                    if (curIndex < minIndex)
                    {
                        minIndex = curIndex;
                    }

                    if (curIndex > maxIndex)
                    {
                        maxIndex = curIndex;
                    }
                }
            }
            break;

        case gcvINDEX_32:
            {
                gcmFATAL("_GetArrayRange: 32-bit indeces are not supported.");
                status = gcvSTATUS_NOT_SUPPORTED;
            }
            break;
        }
    }

    /* Set output. */
    *First = minIndex;
    *Count = maxIndex - minIndex + 1;

    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  _BuildStream
**
**  Build the input stream.
**
**  INPUT:
**
**      Context
**          Pointer to the current context.
**
**      First
**          Specifies the starting index in the enabled arrays.
**
**      Count
**          Specifies the number of items in the enabled arrays to be rendered.
**
**      IndexCount
**          Number of indices in the index buffer.
**
**      IndexType
**          Index type.
**
**      IndexBuffer
**          Pointer to the index buffer.
**
**  OUTPUT:
**
**      Stream
**          Pointer to the list of created streams.
**
**      Start
**          Pointer to the starting vertex number.
*/
gceSTATUS _BuildStream(
    IN glsCONTEXT_PTR   Context,
    IN GLint            First,
    IN GLsizei          Count,
    IN GLsizei          IndexCount,
    IN gceINDEX_TYPE    IndexType,
    IN const GLvoid     *IndexBuffer,
    OUT glsSTREAM_PTR   Stream,
    OUT GLint           *Start
    )
{
    gceSTATUS status;

    do
    {
        gctSIZE_T           i;
        gctSIZE_T           attributeCount;
        gctUINT32           activeAttributeCount = 0;
        glsSTREAMINFO       info[glvATTRIBUTE_VS_COUNT];
        glsSTREAMINFO_PTR   infoList;
        gctUINT32           totalSize   = 0;
        gctUINT32           totalStride = 0;
        GLint               first       = First;
        gctBOOL             rebuild     = gcvFALSE;
        gcoSTREAM           stream      = gcvNULL;

        /* Initialize the attribute wrap list pointer. */
        glsATTRIBUTEWRAP_PTR attribubteWrap = Context->currProgram->vs.attributes;

        /* Construct a vertex object. */
        gcmERR_BREAK(gcoVERTEX_Construct(
            Context->hal,
            &Stream->vertex
            ));

        /* Query the number of attributes. */
        gcmERR_BREAK(gcSHADER_GetAttributeCount(
            Context->currProgram->vs.shader,
            &attributeCount
            ));

        /* Initialize the info list pointer. */
        infoList = (glsSTREAMINFO_PTR) &info;

        for (i = 0; i < attributeCount; i++)
        {
            gctBOOL attributeEnabled;
            gcmERR_BREAK(gcATTRIBUTE_IsEnabled(
                attribubteWrap->attribute,
                &attributeEnabled
                ));

            if (attributeEnabled)
            {
                glsATTRIBUTEINFO_PTR attribute = attribubteWrap->info;

                infoList->attribute = attribute;

                activeAttributeCount++;

                if (attribute->buffer == gcvNULL)
                {
                    if (attribute->pointer == gcvNULL)
                    {
                        status = gcvSTATUS_INVALID_OBJECT;
                        break;
                    }

                    rebuild = gcvTRUE;
                }
                else
                {
                    /* Cast the object. */
                    glsBUFFER_PTR object =
                        (glsBUFFER_PTR) attribute->buffer->object;

                    /* Is there a stream attached? */
                    if (object->stream == gcvNULL)
                    {
                        status = gcvSTATUS_INVALID_OBJECT;
                        break;
                    }

                    if (stream != gcvNULL && stream != object->stream)
                    {
                        rebuild = gcvTRUE;
                    }

                    stream = object->stream;

                    /* Set the stream object. */
                    infoList->stream = stream;
                }

                totalStride += infoList->attribute->attributeSize;

                infoList++;
            }

            attribubteWrap++;
        }

        /* Failed? */
        if (gcmIS_ERROR(status))
        {
            break;
        }

        infoList        = (glsSTREAMINFO_PTR) &info;
        attribubteWrap  = Context->currProgram->vs.attributes;

        if (rebuild)
        {
            gctUINT8_PTR src, dest, destAddress;
            gctUINT32 offset = 0;
            gctBOOL dirty = gcvFALSE;
            gcoVERTEX vertex = Stream->vertex;

            /* Determine array range. */
            if (Count == 0)
            {
                gcmERR_BREAK(_GetArrayRange(
                    Context,
                    IndexCount,
                    IndexType,
                    IndexBuffer,
                    &first,
                    &Count
                    ));
            }

            /* If index count is 0, do not change First.     */
            /* Otherwise, the First is 0 for DrawElements to */
            /* locate vertex correctly.                      */
            *Start = (IndexCount == 0)
                    ? first
                    : 0;

#if gldUSE_VERTEX_CACHE
            /* Check if all attribute is cached. */
            if (_IsVertexCacheHit(
                Context, activeAttributeCount, infoList, first, Count,
                IndexCount, IndexType, IndexBuffer,
                &streamCache
                ))
            {
                return gcvSTATUS_OK;
            }
#endif

            totalSize = totalStride * first + totalStride * Count;

#if gldUSE_VERTEX_CACHE
            /* Cache not hit, but get a cache slot. */
            if (streamCache)
            {
                /* Save stream info to the cache. */
                vertex = streamCache->stream.vertex;
                streamCache->stride         = totalStride;
                streamCache->attributeCount = activeAttributeCount;
                streamCache->start          = first;
                streamCache->end            = first + Count -1;

                /* The vertice will be upload to cache. */
                stream = streamCache->stream.stream[0];
            }
            else
#endif
            {
                if ((Context->streamIndex % gldSTREAM_GROUP_SIZE) == 0)
                {
                    while (gcvTRUE)
                    {
                        status = gcoOS_WaitSignal(
                            gcvNULL,
                            Context->streamSignals
                                [Context->streamIndex / gldSTREAM_GROUP_SIZE],
                            10
                            );

                        if (gcmIS_SUCCESS(status))
                        {
                            break;
                        }

                        gcmERR_BREAK(gcoHAL_Commit(Context->hal, gcvFALSE));
                    }
                }

                if (!Context->streams[Context->streamIndex])
                {
                    /* Construct stream wrapper. */
                    gcmERR_BREAK(gcoSTREAM_Construct(Context->hal, &stream));
                    Context->streams[Context->streamIndex] = stream;
                }
                else
                {
                    stream = Context->streams[Context->streamIndex];
                }

                Context->streamIndex   = (Context->streamIndex + 1) % gldSTREAM_POOL_SIZE;
                Context->streamPending = gcvTRUE;
            }

            gcmERR_BREAK(gcoSTREAM_Reserve(
                stream, totalSize
                ));

            gcmERR_BREAK(gcoSTREAM_SetStride(
                stream, totalStride
                ));

            gcmERR_BREAK(gcoSTREAM_Lock(
                stream, (gctPOINTER) &destAddress, gcvNULL
                ));

#if gldUSE_VERTEX_CACHE
            /* These info should not be put into the temp Stream. */
            if (!streamCache)
#endif
            {
                Stream->attributeCount++;
            }

            for (i = 0; i < activeAttributeCount; ++i)
            {
                GLsizei j;
                gctUINT32 srcOffset = 0, destOffset = 0;
                gctUINT32 srcStride = 0;
                gctUINT32 size;

                dest = (gctUINT8_PTR)destAddress + totalStride * first + offset;
                size = infoList->attribute->attributeSize;

                if (infoList->attribute->buffer == gcvNULL)
                {
                    src = (gctUINT8_PTR) infoList->attribute->pointer
                        + first * infoList->attribute->stride;

                    srcStride = infoList->attribute->stride;
                }
                else
                {
                    glsBUFFER_PTR object = (glsBUFFER_PTR) infoList->attribute->buffer->object;

                    gcmERR_BREAK(gcoSTREAM_Lock(
                        object->stream, (gctPOINTER) &src, gcvNULL
                        ));

                    src = src + (gctUINT)(infoList->attribute->pointer) + first * infoList->attribute->stride;
                    srcStride = infoList->attribute->stride;
                }

                for (j = 0; j < Count; j += 1)
                {
                    gcmERR_BREAK(gcoOS_MemCopy(
                        dest + destOffset, src + srcOffset, size
                        ));

                    destOffset += totalStride;
                    srcOffset  += srcStride;
                }

                /* Add the stream to the vertex. */
                gcmERR_BREAK(gcoVERTEX_EnableAttribute(
                    vertex, i,
                    infoList->attribute->format,
                    infoList->attribute->normalize,
                    infoList->attribute->components,
                    stream,
                    offset,
                    totalStride
                    ));

                dirty = gcvTRUE;

#if gldUSE_VERTEX_CACHE
                /* Save the attribute info into the cache. */
                if (streamCache)
                {
                    gcmERR_BREAK(gcoOS_MemCopy(
                        &streamCache->attributes[i],
                        infoList->attribute,
                        sizeof(glsATTRIBUTEINFO)
                        ));

                    streamCache->offset[i] = offset;
                }
#endif

                offset += infoList->attribute->attributeSize;
                infoList++;
            }

            if (dirty)
            {
                gcmERR_BREAK(gcoSTREAM_Flush(stream));
            }

            gcmERR_BREAK(gcoVERTEX_Bind(vertex));
        }
        else
        {
            /* Determine active attributes. */
            for (i = 0; i < activeAttributeCount; i++)
            {
                gctUINT32 offset = gcmPTR2INT(infoList->attribute->pointer);

                /* Set the stream stride. */
                gcmERR_BREAK(gcoSTREAM_SetStride(
                    infoList->stream,
                    infoList->attribute->stride
                    ));

                /* Add the stream to the vertex. */
                gcmERR_BREAK(gcoVERTEX_EnableAttribute(
                    Stream->vertex, i,
                    infoList->attribute->format,
                    infoList->attribute->normalize,
                    infoList->attribute->components,
                    infoList->stream,
                    offset,
                    infoList->attribute->stride
                    ));

                /* Advance to the next attribute information structure. */
                infoList++;
            }

            /* If draw with index, start at zero vertex. Or if all attributes
               come from client pointer, start at zero because we have enabled
               attributes at proper offset. Else, start at the given offset. */
            *Start = (IndexCount == 0)
                    ? First
                    : 0;

            /* Bind the vertex. */
            gcmERR_BREAK(gcoVERTEX_Bind(Stream->vertex));
        }
    }
    while (GL_FALSE);

    /* Return status. */
    return status;
}

/*******************************************************************************
**
**  _FreeStream
**
**  Free the input stream.
**
**  INPUT:
**
**      Stream
**          Pointer to the stream to be deleted.
**
**  OUTPUT:
**
**      Nothing.
*/

void _FreeStream(
    IN glsCONTEXT_PTR Context,
    IN glsSTREAM_PTR Stream
    )
{
    if (Stream->vertex != gcvNULL)
    {
        gcmVERIFY_OK(gcoVERTEX_Destroy(Stream->vertex));
        Stream->vertex = gcvNULL;
    }

    if (Context->streamPending)
    {
        if ((Context->streamIndex % gldSTREAM_GROUP_SIZE) == 0)
        {
            gcsHAL_INTERFACE    halInterface;
            gctUINT32           signalIndex;

            signalIndex = (Context->streamIndex == 0)
                ? ((gldSTREAM_POOL_SIZE  - 1) / gldSTREAM_GROUP_SIZE)
                :  (Context->streamIndex - 1) / gldSTREAM_GROUP_SIZE;

            halInterface.command            = gcvHAL_SIGNAL;
            halInterface.u.Signal.signal    = Context->streamSignals[signalIndex];
            halInterface.u.Signal.auxSignal = gcvNULL;
            halInterface.u.Signal.process   = gcoOS_GetCurrentProcessID();
            halInterface.u.Signal.fromWhere = gcvKERNEL_COMMAND;

            /* Schedule the event. */
            gcmVERIFY_OK(gcoHAL_ScheduleEvent(
                Context->hal, &halInterface
                ));
        }

        Context->streamPending = gcvFALSE;
    }
}
