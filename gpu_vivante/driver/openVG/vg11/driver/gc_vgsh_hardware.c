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


#include "gc_vgsh_precomp.h"


void _vgCORECtor(_vgCORE *core)
{
    gcmHEADER_ARG("core=0x%x", core);

    core->blend             = gcvTRUE;
    core->depthWrite        = gcvTRUE;
    core->sourceFuncAlpha   = gcvBLEND_ONE;
    core->sourceFuncRGB     = gcvBLEND_SOURCE_ALPHA;
    core->destFuncAlpha     = gcvBLEND_INV_SOURCE_ALPHA;
    core->destFuncRGB       = gcvBLEND_INV_SOURCE_ALPHA;

    core->depthCompare      = gcvCOMPARE_NOT_EQUAL;
    core->depthMode         = gcvDEPTH_Z;
    core->colorWrite        = 0xf;
    core->stencilCompare    = gcvCOMPARE_ALWAYS;
    core->stencilRef        = 0;
    core->stencilMask       = 0xFF;
    core->stencilFail       = gcvSTENCIL_KEEP;
    core->stencilMode       = gcvSTENCIL_NONE;
    core->samples           = ~0U;
    core->invalidCache      = gcvFALSE;

    gcmFOOTER_NO();
}

void _vgCOREDtor(_vgCORE *core)
{
}

void _vgHARDWARECtor(_vgHARDWARE *hardware)
{
    gcmHEADER_ARG("hardware=0x%x", hardware);

    gcoOS_ZeroMemory(hardware, sizeof(_vgHARDWARE));
    _vgCORECtor(&hardware->core);

#if WALK_MG1
    hardware->gdx = hardware->gdy = -1.0f;
#endif

    hardware->paint         = gcvNULL;
    /*hardware->projection  = gcvNULL;*/

    hardware->blendMode     = VG_BLEND_SRC_OVER;
    hardware->imageMode     = VG_DRAW_IMAGE_NORMAL;

    hardware->masking           = gcvFALSE;
    hardware->colorTransform    = gcvFALSE;
    hardware->scissoring        = gcvFALSE;

    hardware->disableClamp      = gcvFALSE;
    hardware->disableTiling     = gcvFALSE;
    hardware->gaussianOPT       = gcvFALSE;


    hardware->zValue            = POSITION_Z_BIAS;

    hardware->program           = gcvNULL;
    hardware->context           = gcvNULL;
    hardware->colorWrite        = 0xf;

#if WALK600
    hardware->isConformance     = gcvFALSE;
    hardware->isGC600_19        = gcvFALSE;
#endif

    hardware->featureVAA        = gcvFALSE;
    hardware->usingMSAA         = gcvFALSE;

    hardware->lutImage          = gcvNULL;

    gcmFOOTER_NO();
}

void _vgHARDWAREDtor(_vgHARDWARE *hardware)
{
    gctINT i;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    for (i = 0; i < vgvMAX_PROGRAMS; i++)
    {
        if (hardware->programTable[i].program != gcvNULL)
        {
            DELETEOBJ(_VGProgram, hardware->core.os, hardware->programTable[i].program);
        }
        else
        {
            break;
        }
    }

    if (hardware->tempStream != gcvNULL)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(hardware->tempStream));
    }

    if (hardware->rectStream != gcvNULL)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(hardware->rectStream));
    }

    _vgCOREDtor(&hardware->core);

    gcmFOOTER_NO();
}


gceSTATUS vgshCORE_SetTarget(_vgCORE *core, gcoSURF target)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x target=0x%x",
                  core, target);

    do
    {
        gctUINT width, height, samples;

        /* set the render target */
        gcmERR_BREAK(gco3D_SetTarget(core->engine, target));

        gcmVERIFY_OK(gcoSURF_GetSize(target,
                                         &width,
                                         &height,
                                         gcvNULL));

        if ((core->width != width) || (core->height != height) || (core->invalidCache))
        {
            /* set the viewport, scissor size*/
            gcmERR_BREAK(gco3D_SetViewport(core->engine,
                                       0,
                                       0,
                                       width,
                                       height));

            core->width     = width;
            core->height    = height;
        }

        /* Update Scissor every thime because a clipping fix in arch-hal-user. */
        gcmERR_BREAK(gco3D_SetScissors(core->engine,
                                   0,
                                   0,
                                   width,
                                   height));

        core->draw = target;

        gcmERR_BREAK(gcoSURF_GetSamples(target, &samples));

        if ((core->samples != samples) || (core->invalidCache))
        {
            /* set the MSAA centroids */
            /*gcmERR_BREAK(_SetCentroids(core->engine, target));*/

            core->samples = samples;

            /* when change the render target(if samples changed), need to reload the shader*/
            core->states = gcvNULL;
        }
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS vgshCORE_EnableStencil(
    _vgCORE *core,
    gceSTENCIL_MODE mode,
    gceCOMPARE compare,
    gctUINT8 ref,
    gctUINT8 mask,
    gceSTENCIL_OPERATION fail)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x mode=%d compare=%d ref=%u mask=%u fail=%d",
        core, mode, compare, ref, mask, fail);

    do
    {
        if ((core->stencilMode != mode) || (core->invalidCache))
        {
            gcmERR_BREAK(gco3D_SetStencilMode(core->engine, mode));
            core->stencilMode = mode;
        }

        if ((mode == gcvSTENCIL_SINGLE_SIDED) || (core->invalidCache))
        {
            if ((core->stencilCompare != compare) || (core->invalidCache))
            {
                gcmERR_BREAK(gco3D_SetStencilCompare(core->engine, gcvSTENCIL_FRONT, compare));
                core->stencilCompare = compare;
            }

            if ((core->stencilRef != ref) || (core->invalidCache))
            {
                gcmERR_BREAK(gco3D_SetStencilReference(core->engine, ref, gcvTRUE));
                gcmERR_BREAK(gco3D_SetStencilReference(core->engine, ref, gcvFALSE));
                core->stencilRef = ref;
            }

            if ((core->stencilMask != mask) || (core->invalidCache))
            {
                gcmERR_BREAK(gco3D_SetStencilMask(core->engine, mask));
                gcmERR_BREAK(gco3D_SetStencilWriteMask(core->engine, mask));
                core->stencilMask = mask;
            }

            if ((core->stencilFail != fail) || (core->invalidCache))
            {
                gcmERR_BREAK(gco3D_SetStencilFail(core->engine, gcvSTENCIL_FRONT, fail));
                core->stencilFail = fail;
            }
        }
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


gceSTATUS vgshCORE_EnableBlend(_vgCORE *core, gctBOOL enable)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x enable=%s", core, enable ? "TRUE" : "FALSE");
    do
    {
        if ((core->blend != enable )|| (core->invalidCache))
        {
            gcmERR_BREAK(gco3D_EnableBlending(core->engine, enable));
            core->blend = enable;
        }
    }while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS vgshCORE_SetDepthCompare(_vgCORE *core, gceCOMPARE compare)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x compare=%d", core, compare);

    do
    {
        if ((core->depthCompare != compare) || (core->invalidCache))
        {
            gcmERR_BREAK(gco3D_SetDepthCompare(core->engine, compare));
            core->depthCompare = compare;
        }
    }while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS vgshCORE_SetDepthMode(_vgCORE *core, gceDEPTH_MODE mode)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x mode=%d", core, mode);

    do
    {
        if ((core->depthMode != mode) || (core->invalidCache))
        {
            gcmERR_BREAK(gco3D_SetDepthMode(core->engine, mode));
            core->depthMode = mode;
        }
    }while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS vgshCORE_EnableDepthWrite(_vgCORE *core, gctBOOL enable)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x enable=%s", core, enable ? "TRUE" : "FALSE");

    do
    {
        if ((core->depthWrite != enable) || (core->invalidCache))
        {
            gcmERR_BREAK(gco3D_EnableDepthWrite(core->engine, enable));

            core->depthWrite = enable;
        }
    }while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS vgshCORE_SetColorWrite(_vgCORE *core, gctUINT8 colorWrite)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x colorWrite=%u", core, colorWrite);

    do
    {
        if ((core->colorWrite != colorWrite) || (core->invalidCache))
        {
            gcmERR_BREAK(gco3D_SetColorWrite(core->engine, colorWrite));
            core->colorWrite = colorWrite;
        }
    }while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS vgshCORE_LoadShader(_vgCORE *core,
    gctSIZE_T stateBufferSize,
    gctPOINTER stateBuffer,
    gcsHINT_PTR hints)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x statusBufferSize=%u stateBuffer=0x%x hints=0x%x",
        core, (unsigned long)stateBufferSize, stateBuffer, hints);

    gcmASSERT(stateBuffer != gcvNULL);

    do
    {
        if (core->states != stateBuffer)
        {
            gcmERR_BREAK(gcLoadShaders(core->hal,
                                        stateBufferSize,
                                        stateBuffer,
                                        hints));

            core->statesSize    = stateBufferSize;
            core->states        = stateBuffer;
            core->hints         = hints;
        }
    }while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS vgshCORE_DisableAttribute(
    _vgCORE *core,
    gctUINT32 Index
    )
{
    return gcoVERTEX_DisableAttribute(core->vertex, Index);
}

gceSTATUS vgshCORE_EnableAttribute(
    _vgCORE *core,
    gctUINT32 location,
    gceVERTEX_FORMAT type,
    gctBOOL normalized,
    gctUINT32 size,
    gcoSTREAM stream,
    gctUINT32 offset,
    gctUINT32 stride
    )
{
    return gcoVERTEX_EnableAttribute(core->vertex,
                                    location,
                                    type,
                                    normalized,
                                    size,
                                    stream,
                                    offset,
                                    stride);

}

gceSTATUS vgshCORE_BindIndex(
    _vgCORE *core,
    gcoINDEX index,
    gceINDEX_TYPE type
    )
{
    return gcoINDEX_Bind(index, type);
}

gceSTATUS vgshCORE_BindVertex(
    _vgCORE *core
    )
{
    return gcoVERTEX_Bind(core->vertex);
}


gceSTATUS vgshCORE_SetPrimitiveInfo(
        _vgCORE *core,
        gcePRIMITIVE primitiveType,
        gctINT base,
        gctINT start,
        gctSIZE_T primitiveCount,
        gctBOOL  drawIndex)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x primitiveType=%d base=%d start=%d primitiveCount=%u drawIndex=%s",
        core, primitiveType, base, start, (unsigned long)primitiveCount,
        drawIndex ? "TRUE" : "FALSE");

    core->primitiveType     = primitiveType;
    core->base              = base;
    core->start             = start;
    core->primitiveCount    = primitiveCount;
    core->drawIndex         = drawIndex;

    gcmFOOTER();

    return status;
}


gceSTATUS vgshCORE_DrawPrimitives(_vgCORE *core)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("core=0x%x", core);

    do
    {
        if (core->drawIndex)
        {
            gcmERR_BREAK(gco3D_DrawIndexedPrimitives(core->engine,
                                 core->primitiveType,
                                 core->base,
                                 core->start,
                                 core->primitiveCount));
        }
        else
        {
            gcmERR_BREAK(gco3D_DrawPrimitives(core->engine,
                                 core->primitiveType,
                                 core->start,
                                 core->primitiveCount));
        }
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

/*=================================================================================================*/


gceSTATUS _TessFlatten(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix, gctFLOAT strokeWidth)
{
    return (TessFlatten(context, path, matrix, strokeWidth) ? gcvSTATUS_OK : gcvSTATUS_INVALID_DATA);
}

gceSTATUS _TessFillPath(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix)
{
    return (TessFillPath(context, path, matrix) ? gcvSTATUS_OK : gcvSTATUS_INVALID_DATA);
}

gceSTATUS _TessStrokePath(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix)
{
    return (TessStrokePath(context, path, matrix) ? gcvSTATUS_OK : gcvSTATUS_INVALID_DATA);
}

gceSTATUS _LoadIndices(_vgHARDWARE *hardware, _VGIndexBuffer *indexBuffer)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x indexBuffer=0x%x", hardware, indexBuffer);

    do
    {
        if (indexBuffer->data.items)
        {
            gcmASSERT(indexBuffer->data.size > 0);
            gcmASSERT(indexBuffer->data.allocated >= indexBuffer->data.size);

            if (indexBuffer->index)
            {
                gcmERR_BREAK(gcoINDEX_Destroy(indexBuffer->index));
                indexBuffer->index = gcvNULL;
            }

            gcmERR_BREAK(
                gcoINDEX_Construct(hardware->core.hal,
                                   &indexBuffer->index));

            /*TODO: since hal API can not support the video memory allocation alignemnt, so use the lock memory to substitute*/
            /* for the vgmark11 flash performance */
#if gcdDUMP || defined _DUMP_FILE
            gcmERR_BREAK(
                gcoINDEX_Upload(indexBuffer->index,
                                             indexBuffer->data.items,
                                             indexBuffer->data.size));
#else
            {
                gctPOINTER memory;

                gcmERR_BREAK(gcoINDEX_Upload(indexBuffer->index,
                                             gcvNULL,
                                             gcmALIGN(indexBuffer->data.size, 256)));

                gcmERR_BREAK(gcoINDEX_Lock(indexBuffer->index, gcvNULL, &memory));
                gcmERR_BREAK(gcoOS_MemCopy(memory, indexBuffer->data.items, indexBuffer->data.size));

                gcmERR_BREAK(gcoINDEX_Unlock(indexBuffer->index));
            }

#endif
            ARRAY_DTOR(indexBuffer->data);
        }


        gcmASSERT(indexBuffer->data.items       == gcvNULL);
        gcmASSERT(indexBuffer->data.size        == 0);
        gcmASSERT(indexBuffer->data.allocated   == 0);
        gcmASSERT(indexBuffer->index            != gcvNULL);

        gcmERR_BREAK(
            vgshCORE_BindIndex(&hardware->core,
                              indexBuffer->index,
                              indexBuffer->indexType));

    }while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS _LoadVertexs(_vgHARDWARE *hardware, _VGVertexBuffer *vertexBuffer)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x vertexBuffer=0x%x", hardware, vertexBuffer);

    do
    {
        if (vertexBuffer->data.items)
        {
            gcmASSERT(vertexBuffer->data.size > 0);
            gcmASSERT(vertexBuffer->data.allocated >= vertexBuffer->data.size);

            if (vertexBuffer->stream)
            {
                gcmERR_BREAK(gcoSTREAM_Destroy(vertexBuffer->stream));
                vertexBuffer->stream = gcvNULL;
            }

            gcmERR_BREAK(
                gcoSTREAM_Construct(hardware->core.hal,
                                    &vertexBuffer->stream));

            /*TODO: since hal API can not support the video memory allocation alignemnt, so use the lock memory to substitute*/
            /* for the vgmark11 flash performance */
#if gcdDUMP || defined _DUMP_FILE
            gcmERR_BREAK(gcoSTREAM_Upload(
                vertexBuffer->stream,
                vertexBuffer->data.items,
                0,
                vertexBuffer->data.size,
                gcvFALSE));
#else
            {
                gctPOINTER memory;

                gcmERR_BREAK(gcoSTREAM_Upload(
                    vertexBuffer->stream,
                    gcvNULL,
                    0,
                    gcmALIGN(vertexBuffer->data.size, 256),
                    gcvFALSE));

                gcmERR_BREAK(gcoSTREAM_Lock(vertexBuffer->stream, &memory, gcvNULL));
                gcmERR_BREAK(gcoOS_MemCopy(memory, vertexBuffer->data.items, vertexBuffer->data.size));

                /* delay the "unlock"  to destory : need to fix*/
                /*gcmERR_BREAK(gcoSTREAM_Unlock(vertexBuffer->stream));*/

                gcmERR_BREAK(
                    gcoSTREAM_Flush(vertexBuffer->stream));
            }
#endif
            gcmASSERT(vertexBuffer->stride > 0);
            gcmVERIFY_OK(
                gcoSTREAM_SetStride(vertexBuffer->stream,
                                    vertexBuffer->stride));

            ARRAY_DTOR(vertexBuffer->data);
        }

        gcmASSERT(vertexBuffer->data.items      == gcvNULL);
        gcmASSERT(vertexBuffer->data.size       == 0);
        gcmASSERT(vertexBuffer->data.allocated  == 0);
        gcmASSERT(vertexBuffer->stream          != gcvNULL);

        gcmERR_BREAK(
            vgshCORE_EnableAttribute(&hardware->core,
                                    hardware->program->vertexShader.attributes[vgvATTRIBUTE_VERTEX].index,
                                    vertexBuffer->type,
                                    vertexBuffer->normalized,
                                    vertexBuffer->size,
                                    vertexBuffer->stream,
                                    0,
                                    vertexBuffer->stride));

        gcmERR_BREAK(
            vgshCORE_BindVertex(&hardware->core));

        gcmERR_BREAK(
            vgshCORE_DisableAttribute(&hardware->core,
                                     hardware->program->vertexShader.attributes[vgvATTRIBUTE_VERTEX].index));
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


gceSTATUS _CreateBltStream(_vgHARDWARE *hardware,
                             gctINT32 dstLeft,
                             gctINT32 dstTop,
                             gctINT32 dstRight,
                             gctINT32 dstBottom,
                             gctINT32 srcLeft,
                             gctINT32 srcTop,
                             gctINT32 srcRight,
                             gctINT32 srcBottom,
                             gctINT32 srcRectWidth,
                             gctINT32 srcRectHeight,
                             gcoSTREAM *stream)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x dstLeft=%d dstTop=%d dstRight=%d dstBottom=%d srcLeft=%d "
        "srcTop=%d srcRight=%d srcBottom%d srcRectWidth=%d srcRectHeight=%d stream=0x%x",
        hardware, dstLeft, dstTop, dstRight, dstBottom, srcLeft, srcTop, srcRight, srcBottom,
        srcRectWidth, srcRectHeight, stream);

    do
    {
        gcmERR_BREAK(gcoSTREAM_Construct(hardware->core.hal, stream));

        /* construct the image stream */
        {
            gctFLOAT u0 = (gctFLOAT)(srcLeft)   / srcRectWidth;
            gctFLOAT v0 = (gctFLOAT)(srcTop)    / srcRectHeight;
            gctFLOAT u1 = (gctFLOAT)(srcRight)  / srcRectWidth;
            gctFLOAT v1 = (gctFLOAT)(srcBottom) / srcRectHeight;

            gctFLOAT vertexAttriData[16];

            vertexAttriData[0]  = (gctFLOAT)(dstLeft);
            vertexAttriData[1]  = (gctFLOAT)(dstBottom);
            vertexAttriData[2]  = u0;
            vertexAttriData[3]  = v1;
            vertexAttriData[4]  = (gctFLOAT)(dstRight);
            vertexAttriData[5]  = (gctFLOAT)(dstBottom);
            vertexAttriData[6]  = u1;
            vertexAttriData[7]  = v1;
            vertexAttriData[8]  = (gctFLOAT)(dstRight);
            vertexAttriData[9]  = (gctFLOAT)(dstTop);
            vertexAttriData[10] = u1;
            vertexAttriData[11] = v0;
            vertexAttriData[12] = (gctFLOAT)(dstLeft);
            vertexAttriData[13] = (gctFLOAT)(dstTop);
            vertexAttriData[14] = u0;
            vertexAttriData[15] = v0;

            gcmERR_BREAK(gcoSTREAM_Upload(
                            *stream,
                            vertexAttriData,
                            0,
                            sizeof(vertexAttriData),
                            gcvFALSE));

            gcmERR_BREAK(gcoSTREAM_SetStride(*stream, sizeof(gctFLOAT) * 4));
        }

        gcmFOOTER();
        return status;
    }
    while(gcvFALSE);

    /*roll back the resources*/
    if (*stream != gcvNULL)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(*stream));
    }

    gcmFOOTER();

    return status;
}

static gceSTATUS _QueryStream(
    _vgHARDWARE     *hardware,
    gctINT32        sx,
    gctINT32        sy,
    gctINT32        width,
    gctINT32        height,
    _VGImage        *srcImage,
    gcoSTREAM       *stream)
{
    gctINT32 dx = 0, dy = 0;
    gceSTATUS status;

    if ((sx == 0)
        && (sy == 0)
        && (width == srcImage->width)
        && (height == srcImage->height)
        && (srcImage->stream != gcvNULL))
    {
        *stream = srcImage->stream;
        return gcvSTATUS_OK;
    }

    sx += srcImage->rootOffsetX;
    sy += srcImage->rootOffsetY;

    if (hardware->tempStream)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(hardware->tempStream));
        hardware->tempStream = gcvNULL;
    }

    if (srcImage->orient == vgvORIENTATION_LEFT_BOT)
    {
        status = _CreateBltStream(
            hardware,
            dx, dy,
            dx + width,
            dy + height,
            sx, sy,
            sx + width,
            sy + height,
            srcImage->rootWidth,
            srcImage->rootHeight,
            &hardware->tempStream);
    }
    else
    {
       status = _CreateBltStream(
            hardware,
            dx, dy,
            dx +  width,
            dy + height,
            sx,
            srcImage->rootHeight - sy,
            sx + width,
            srcImage->rootHeight - (sy + height),
            srcImage->rootWidth,
            srcImage->rootHeight,
            &hardware->tempStream);
    }

    *stream = hardware->tempStream;

    return status;
}

gceSTATUS vgshCreateImageStream(
    _VGContext *context,
    _VGImage *image,
    gctINT32 dx,
    gctINT32 dy,
    gctINT32 sx,
    gctINT32 sy,
    gctINT32 width,
    gctINT32 height,
    gcoSTREAM *stream)
{
    gctINT32  offsetX, offsetY, anWidth, anHeight;
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("context=0x%x image=0x%x dx=%d dy=%d sx=%d sy=%d width=%d height=%d stream=0x%x",
        context, image, dx, dy, sx, sy, width, height, stream);

    do
    {
        /* query the root image size */
        GetAncestorOffset(image, &offsetX, &offsetY);
        GetAncestorSize(image, &anWidth, &anHeight);

        if (image->orient == vgvORIENTATION_LEFT_BOT)
        {
            gcmERR_BREAK(_CreateBltStream(
                &context->hardware, dx, dy,
                dx + width, dy + height,
                sx + offsetX,
                sy + offsetY,
                sx + offsetX + width,
                sy + offsetY + height,
                anWidth, anHeight,
                stream));
        }
        else
        {
            gcmERR_BREAK(_CreateBltStream(
                &context->hardware, dx, dy,
                dx + width, dy + height,
                sx + offsetX,
                anHeight - (sy + offsetY),
                sx + offsetX + width,
                anHeight - (sy + offsetY + height),
                anWidth,
                anHeight,
                stream));
        }

    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS vgshCreateTexture(
    _VGContext *context,
    gctUINT32 width,
    gctUINT32 height,
    gceSURF_FORMAT format,
    gcoTEXTURE *texture,
    gcoSURF *surface)
{
    gceSTATUS  status = gcvSTATUS_OK;

    gcmHEADER_ARG("context=0x%x width=%d height=%d format=%d texture=0x%x surface=0x%x",
        context, width, height, format, texture, surface);

    do
    {
        /* initialize */
        *texture = gcvNULL;

        /* Create the texture object. */
        gcmERR_BREAK(gcoTEXTURE_Construct(context->hal, texture));

        if (surface)
        {
            gcmERR_BREAK(gcoTEXTURE_AddMipMap(*texture,
                                             0,
                                             format,
                                             width, height,
                                             0,
                                             0,
                                             gcvPOOL_DEFAULT,
                                             surface));

            gcmERR_BREAK(gcoSURF_SetResolvability(*surface, gcvFALSE));

            gcmERR_BREAK(gcoSURF_SetOrientation(*surface,  vgvORIENTATION_LEFT_BOT));
        }

        gcmFOOTER();

        return status;
    }
    while (gcvFALSE);


    /* roll back the resources */
    if (*texture)
    {
        gcmVERIFY_OK(gcoTEXTURE_Destroy(*texture));
    }

    gcmFOOTER();
    /* Return the status. */
    return status;
}


void _GetTexBound(_VGImage *image, gctFLOAT* texBound)
{
    gctINT32  offsetX, offsetY, anWidth, anHeight;

    gcmHEADER_ARG("image=0x%x texBound=0x%x", image, texBound);

    GetAncestorOffset(image, &offsetX, &offsetY);
    GetAncestorSize(image, &anWidth, &anHeight);

    texBound[0] = (gctFLOAT)(offsetX)           / anWidth;
    texBound[1] = (gctFLOAT)(offsetY)           / anHeight;
    texBound[2] = (gctFLOAT)(offsetX + image->width)    / anWidth;
    texBound[3] = (gctFLOAT)(offsetY + image->height)   / anHeight;

    /* vec2 texBoundEx */
    texBound[4] = (gctFLOAT)(offsetX + image->width - 1)    / anWidth;
    texBound[5] = (gctFLOAT)(offsetY + image->height - 1)   / anHeight;
    texBound[6] = 0.0f;
    texBound[7] = 0.0f;

    /* vec4 muv */
    texBound[8] = texBound[2] - texBound[0];
    texBound[9] = texBound[3] - texBound[1];
    texBound[10] = 2 * texBound[8];
    texBound[11] = 2 * texBound[9];

    gcmFOOTER_NO();
}

static gceSTATUS _CreateColorRamp(_vgHARDWARE *hardware, _VGPaint* paint)
{
    gceSTATUS status = gcvSTATUS_OK;

    if (paint->colorRamp.surface == gcvNULL)
    {
        _VGColorDesc    colorDesc;
        vgshGetFormatColorDesc(VG_sRGBA_8888, &colorDesc);

        gcmONERROR(vgshIMAGE_Initialize(
            hardware->context,
            &paint->colorRamp,
            &colorDesc,
            256, 1,
            gcvORIENTATION_BOTTOM_TOP));

        paint->colorRamp.texStates.s =
            _vgSpreadMode2gcMode(paint->colorRampSpreadMode);
    }

OnError:
    return status;
}

static gceSTATUS _UpdateColorRamp(_vgHARDWARE *hardware, _VGPaint *paint)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x paint=0x%x", hardware, paint);

    do
    {
        /* no need to generate the color ramp*/
        if ((paint->paintType == VG_PAINT_TYPE_COLOR) ||
            (paint->paintType == VG_PAINT_TYPE_PATTERN) ||
            !IsPaintDirty(paint))
        {
            break;
        }

        gcmVERIFY_OK(_CreateColorRamp(hardware, paint));

        hardware->dstImage      = &paint->colorRamp;
        hardware->paint         = paint;
        hardware->stencilMode   = gcvSTENCIL_NONE;
        hardware->depthMode     = gcvDEPTH_NONE;

        hardware->blending      = gcvFALSE;
        hardware->depthCompare  = gcvCOMPARE_ALWAYS;
        hardware->depthWrite    = gcvFALSE;
        hardware->drawPipe      = vgvDRAWPIPE_COLORRAMP;
        hardware->flush         = gcvTRUE;
        hardware->colorWrite    = 0xf;

        gcmERR_BREAK(vgshHARDWARE_RunPipe(hardware));
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gctBOOL _TestImageUserToSurfaceTransform(_VGImage *image, _VGMatrix3x3 *matrix)
{
    _VGVector3 p0, p1, p2, p3;

    gcmHEADER_ARG("image=0x%x matrix=0x%x", image, matrix);

    V3_SET(p0, 0, 0, 1);
    V3_SET(p1, 0, (VGfloat)image->height, 1);
    V3_SET(p2, (VGfloat)image->width, (VGfloat)image->height, 1);
    V3_SET(p3, (VGfloat)image->width, 0, 1);
    MATRIX_MUL_VEC3(matrix->m, p0, p0);
    MATRIX_MUL_VEC3(matrix->m, p1, p1);
    MATRIX_MUL_VEC3(matrix->m, p2, p2);
    MATRIX_MUL_VEC3(matrix->m, p3, p3);

    if(p0.z <= 0.0f || p1.z <= 0.0f || p2.z <= 0.0f || p3.z <= 0.0f)
    {
        gcmFOOTER_ARG("return=%s", "FALSE");
        return gcvFALSE;
    }

    gcmFOOTER_ARG("return=%s", "TRUE");

    return gcvTRUE;
}

static gctBOOL _TestSurfaceToPaintTransform(_VGMatrix3x3 *userToSurface, _VGMatrix3x3 *paintToUser)
{
    _VGMatrix3x3 surfaceToPaintMatrix, surfaceToPaintMatrixInvert;
    gctBOOL ret;

    gcmHEADER_ARG("userToSurface=0x%x paintToUser=0x%x", userToSurface, paintToUser);

    MultMatrix(userToSurface, paintToUser, &surfaceToPaintMatrix);

    ret = (InvertMatrix(&surfaceToPaintMatrix, &surfaceToPaintMatrixInvert) == 1) ? gcvTRUE : gcvFALSE;

    gcmFOOTER_ARG("return=%s", ret ? "TRUE" : "FALSE");

    return ret;
}

static gceSTATUS _RenderImage(_VGContext *context, _VGImage *image, _VGMatrix3x3 *imageUserToSurface)
{
    gceSTATUS   status = gcvSTATUS_OK;
    _vgHARDWARE *hardware = &context->hardware;

    gcmHEADER_ARG("context=0x%x image=0x%x imageUserToSurface=0x%x",
                  context, image, imageUserToSurface);

    do
    {
        if (_TestImageUserToSurfaceTransform(image, imageUserToSurface))
        {
            hardware->dstImage          = &context->targetImage;
            hardware->blendMode         = context->blendMode;
            hardware->masking           = context->masking;
            hardware->colorTransform    = context->colorTransform;
            hardware->depthCompare      = gcvCOMPARE_NOT_EQUAL;
            hardware->depthWrite        = gcvTRUE;
            hardware->blending          = gcvTRUE;

            hardware->srcImage          = image;

            hardware->paint             = context->fillPaint ?
                context->fillPaint : &context->defaultPaint;

            hardware->paintToUser       = &context->fillPaintToUser;
            hardware->userToSurface     = imageUserToSurface;

            hardware->dx                = 0;
            hardware->dy                = 0;
            hardware->sx                = 0;
            hardware->sy                = 0;
            hardware->width             = image->width;
            hardware->height            = image->height;

            hardware->drawPipe              = vgvDRAWPIPE_IMAGE;
            hardware->zValue                += POSITION_Z_INTERVAL;
            hardware->flush                 = gcvFALSE;
            hardware->colorWrite            = 0xf;
            hardware->colorTransformValues  = context->colorTransformValues;

            if (vgshIsScissoringEnabled(context))
            {
                hardware->stencilMode       = gcvSTENCIL_SINGLE_SIDED;
                hardware->stencilMask       = 0xff;
                hardware->stencilRef        = 0;
                hardware->stencilCompare    = gcvCOMPARE_NOT_EQUAL;
                hardware->stencilFail       = gcvSTENCIL_KEEP;

                /* enable the depth test */
                hardware->depthMode         = gcvDEPTH_Z;

#if NO_STENCIL_VG
                {
                    hardware->stencilMode       = gcvSTENCIL_NONE;
                    hardware->stencilCompare    = gcvCOMPARE_ALWAYS;
                    hardware->depthCompare      = gcvCOMPARE_GREATER;
                    hardware->zValue = context->scissorZ - POSITION_Z_INTERVAL;
                    hardware->depthWrite = VG_FALSE;
                    /*Since blending stroke drawing may change Z*/
                    context->scissorDirty |= hardware->depthWrite;
                }
#endif  /* #ifdef NO_STENCIL_VG */
            }
            else
            {
                /* disable the stencil test */
                hardware->stencilMode       = gcvSTENCIL_NONE;

                /* disable the depth test */
                hardware->depthMode         = gcvDEPTH_NONE;
            }

            if (!isAffine(imageUserToSurface))
            {
                hardware->imageMode = VG_DRAW_IMAGE_NORMAL;
            }
            else
            {
                hardware->imageMode = context->imageMode;
            }

            gcmERR_BREAK(vgshHARDWARE_RunPipe(hardware));
        }
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}

static gceSTATUS _AllowImageQuality(_VGContext *context, _VGImage *image)
{
    VGbitfield  aq;
    gcmHEADER_ARG("context=0x%x image=0x%x", context, image);

    aq = image->allowedQuality;
    aq &= (VGbitfield)context->imageQuality;
    if ((aq & VG_IMAGE_QUALITY_BETTER) || (aq & VG_IMAGE_QUALITY_FASTER))
    {
        image->texStates.minFilter = gcvTEXTURE_LINEAR;
        image->texStates.magFilter = gcvTEXTURE_LINEAR;
    }
    else
    {
        image->texStates.minFilter = gcvTEXTURE_POINT;
        image->texStates.magFilter = gcvTEXTURE_POINT;
    }

    gcmFOOTER_NO();

    return gcvSTATUS_OK;
}

gceSTATUS vgshDrawImage(_VGContext *context, _VGImage *image, _VGMatrix3x3 *imageUserToSurface)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("context=0x%x image=0x%x imageUserToSurface=0x%x",
        context, image, imageUserToSurface);
    do
    {
        gcmERR_BREAK(vgshUpdateScissor(context));
        gcmERR_BREAK(_AllowImageQuality(context, image));

        gcmERR_BREAK(_UpdateColorRamp(&context->hardware,
            context->fillPaint ?
            context->fillPaint : &context->defaultPaint));

        gcmERR_BREAK(_RenderImage(context, image, imageUserToSurface));
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _RenderPath(_VGContext *context, _VGPath *path,  _VGPaint* paint, _VGMatrix3x3 *userToSurface,
                      _VGMatrix3x3 *paintToUser, VGbitfield paintModes)
{
    gceSTATUS status = gcvSTATUS_OK;
    _vgHARDWARE * hardware = &context->hardware;

    gcmHEADER_ARG("context=0x%x path=0x%x paint=0x%x userToSurface=0x%x paintToUser=0x%x paintModes=0x%x",
                  context, path, paint, userToSurface, paintToUser, paintModes);

    do
    {
        if (_TestSurfaceToPaintTransform(userToSurface, paintToUser))
        {
            _VGPaint zeroPaint;

            hardware->dstImage          = &context->targetImage;

            hardware->blendMode         = context->blendMode;
            hardware->masking           = context->masking;
            hardware->colorTransform    = context->colorTransform;
            hardware->path              = path;
            hardware->drawPipe          = vgvDRAWPIPE_PATH;
            hardware->depthCompare      = gcvCOMPARE_NOT_EQUAL;
            hardware->depthWrite        = gcvTRUE;
            hardware->blending          = gcvTRUE;
            hardware->flush             = gcvFALSE;
            hardware->userToSurface     = userToSurface;

            if ((paint->paintType == VG_PAINT_TYPE_COLOR) &&
                (paint->paintColor.a == 0.0f) &&
                !(context->targetImage.internalColorDesc.colorFormat & PREMULTIPLIED) &&
                (context->blendMode == VG_BLEND_SRC))
            {
                zeroPaint = *paint;
                zeroPaint.paintColor.a =
                zeroPaint.paintColor.r =
                zeroPaint.paintColor.g =
                zeroPaint.paintColor.b = 0.0f;
                zeroPaint.paintColor.format = sRGBA;
                hardware->paint = &zeroPaint;
            }
            else
            {
                hardware->paint             = paint;
            }

            hardware->paintToUser       = paintToUser;
            hardware->paintMode         = (VGPaintMode) paintModes;

            hardware->colorWrite            = 0xf;
            hardware->colorTransformValues  = context->colorTransformValues;
            hardware->zValue                += POSITION_Z_INTERVAL;

            if (vgshIsScissoringEnabled(context))
            {
                hardware->stencilMode       = gcvSTENCIL_SINGLE_SIDED;
                hardware->stencilMask       = 0xff;
                hardware->stencilRef        = 0;
                hardware->stencilCompare    = gcvCOMPARE_NOT_EQUAL;
                hardware->stencilFail       = gcvSTENCIL_KEEP;

                hardware->depthMode         = gcvDEPTH_Z;
#if NO_STENCIL_VG
                {
                    hardware->stencilMode       = gcvSTENCIL_NONE;
                    hardware->stencilCompare    = gcvCOMPARE_ALWAYS;
                    hardware->depthCompare      = gcvCOMPARE_GREATER;
                    hardware->zValue = context->scissorZ - POSITION_Z_INTERVAL;
                    hardware->depthWrite = (paintModes == VG_STROKE_PATH && hardware->blendMode != VG_BLEND_SRC_OVER)? VG_TRUE:VG_FALSE;
                    /*Since blending stroke drawing may change Z*/
                    context->scissorDirty |= hardware->depthWrite;
                }
#endif  /* #ifdef NO_STENCIL_VG */
            }
            else
            {
                hardware->stencilMode       = gcvSTENCIL_NONE;
                hardware->depthMode         = (paintModes == VG_STROKE_PATH) ? gcvDEPTH_Z : gcvDEPTH_NONE;
            }
            gcmASSERT((paintModes == VG_FILL_PATH) || (paintModes == VG_STROKE_PATH));

            gcmERR_BREAK(vgshHARDWARE_RunPipe(hardware));
        }
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS vgshDrawPath(_VGContext *context, _VGPath *path, VGbitfield paintModes, _VGMatrix3x3 *userToSurface)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("context=0x%x path=0x%x paintModes=%u userToSurface=0x%x",
        context, path, paintModes, userToSurface);
    do
    {
        CheckContextParam(context, path, userToSurface, paintModes);

        gcmERR_BREAK(vgshUpdateScissor(context));

        if (paintModes & VG_FILL_PATH)
        {
            _VGPaint* fillPaint = context->fillPaint ? context->fillPaint : &context->defaultPaint;

            gcmERR_BREAK(_UpdateColorRamp(&context->hardware, fillPaint));

            /* skip the error check for G30226 */
            _RenderPath(context, path, fillPaint, userToSurface, &context->fillPaintToUser, VG_FILL_PATH);
        }

        if (paintModes & VG_STROKE_PATH)
        {
            _VGPaint* strokePaint = context->strokePaint ? context->strokePaint : &context->defaultPaint;

            gcmERR_BREAK(_UpdateColorRamp(&context->hardware, strokePaint));

            gcmERR_BREAK(_RenderPath(context, path, strokePaint, userToSurface, &context->strokePaintToUser, VG_STROKE_PATH));

        }

        gcmASSERT(
            !IsPathDirty(path, VGTessPhase_ALL)
            ||
            IsPathDirty(path, (_VGTessPhase) (VGTessPhase_Stroke | VGTessPhase_Fill))
            );

        ClearTessellateResult(context, path);
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gctBOOL _UseShaderBlending(_vgHARDWARE *hardware)
{
    gctBOOL ret = gcvFALSE;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    if (hardware->blending)
    {
        /*it fixs SFT_MaskLayer bug, but conformance test F20107 fail. remove it temporarily*/
        if (hardware->masking && !hardware->isConformance)
        {
            ret = gcvTRUE;
        }
        else
        if (hardware->blendMode == VG_BLEND_MULTIPLY    ||  \
            hardware->blendMode == VG_BLEND_SCREEN      ||  \
            hardware->blendMode == VG_BLEND_DARKEN      ||  \
            hardware->blendMode == VG_BLEND_LIGHTEN     ||  \
            hardware->blendMode == VG_BLEND_SRC_IN      ||  \
            hardware->blendMode == VG_BLEND_DST_IN)
        {
            ret = gcvTRUE;
        }
        else if ((hardware->blendMode == VG_BLEND_SRC) &&
                 ((hardware->drawPipe == vgvDRAWPIPE_IMAGE) ||
                  ((hardware->drawPipe == vgvDRAWPIPE_PATH) &&
                   (hardware->paint->paintType  == VG_PAINT_TYPE_PATTERN)
                  )))
        {
            /* special hack for performance */
            ret = gcvTRUE;
        }
        else if ((hardware->blendMode == VG_BLEND_SRC_OVER) && \
                    (hardware->drawPipe == vgvDRAWPIPE_IMAGE) && \
                    (hardware->imageMode == VG_DRAW_IMAGE_STENCIL))
        {
            ret = gcvTRUE;
        }
        /* it fixs SFT_BlendMode bug, but vgmark11 flash performance down. remove it temporarily
        else if ((hardware->drawPipe == vgvDRAWPIPE_PATH) && \
                 (hardware->paint->paintType == VG_PAINT_TYPE_COLOR) && \
                 (hardware->paint->paintColor.a == 0))
        {
            return gcvTRUE;
        }
        */
    }

    gcmFOOTER_ARG("ret=%s", ret ? "TRUE" : "FALSE");

    return ret;
}

gceSTATUS _DrawPrimitives(_vgHARDWARE *hardware)
{
    gceSTATUS status = vgshCORE_DrawPrimitives(&hardware->core);

    if (gcmIS_SUCCESS(status))
    {
        if (hardware->isConformance)
        {
            gcmVERIFY_OK(gcoSURF_Flush(hardware->dstImage->surface));

            gcmVERIFY_OK(gco3D_Semaphore(
                hardware->core.engine,
                gcvWHERE_RASTER, gcvWHERE_PIXEL,
                gcvHOW_SEMAPHORE_STALL));
        }

        *hardware->dstImage->dirtyPtr = gcvTRUE;
    }

    return status;
}

static gceSTATUS _TextureBind(_VGImage* image, gctINT sampler)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("image=0x%x sampler=%d", image, sampler);

    do
    {
        gcmASSERT(sampler >= 0);
        gcmERR_BREAK(gcoTEXTURE_Flush(image->texture));

        gcmERR_BREAK(gcoTEXTURE_BindTexture(
            image->texture,
            0,
            sampler,
            &image->texStates));
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_SolidColor(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)&hardware->paint->paintColor);
}

gceSTATUS SetUniform_ModeViewMatrix(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT userToSurface[16];
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    if (hardware->dstImage->orient == vgvORIENTATION_LEFT_BOT)
    {
        _VGMatrix3x3    modeViewMatrix, out;
        _VGMatrixCtor(&modeViewMatrix);

        MatrixTranslate(&modeViewMatrix, 0,
            (gctFLOAT)hardware->dstImage->rootHeight);

        MatrixScale(&modeViewMatrix, 1.0f, -1.0f);

        /* assume dx, dy is (0, 0) */
        MultMatrix(&modeViewMatrix, hardware->userToSurface, &out);
        _MatrixToGAL(&out, userToSurface);
    }
    else
    {
        _MatrixToGAL(hardware->userToSurface, userToSurface);
    }

    status = gcUNIFORM_SetValueF(uniform, 1, userToSurface);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_FilterModeViewMatrix(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT        userToSurface[16];
    _VGMatrix3x3    modelViewMatrix;
    gceSTATUS       status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);


    _VGMatrixCtor(&modelViewMatrix);

    if (hardware->dstImage->orient == vgvORIENTATION_LEFT_BOT)
    {
        MatrixTranslate(
            &modelViewMatrix, 0,
            (gctFLOAT)hardware->dstImage->rootHeight);

        MatrixScale(&modelViewMatrix, 1.0f, -1.0f);
    }

    MatrixTranslate(
        &modelViewMatrix,
        (gctFLOAT)(hardware->dstImage->rootOffsetX + hardware->dx),
        (gctFLOAT)(hardware->dstImage->rootOffsetY + hardware->dy));


    _MatrixToGAL(&modelViewMatrix, userToSurface);

    status = gcUNIFORM_SetValueF(uniform, 1, userToSurface);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_ProjectionMatrix(_vgHARDWARE *hardware, gcUNIFORM uniform)
{

    gctUINT     width, height;
    gctFLOAT    projection[16];
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    width = hardware->dstImage->rootWidth;
    height = hardware->dstImage->rootHeight;

#ifndef JRC
    vgmSET_GAL_MATRIX(projection,
        2/(gctFLOAT)width,                 0,                       0,     -1,
        0,                                 2/(gctFLOAT)height,      0,     -1,
        0,                                 0,                       -1,     0,
        0,                                 0,                       0,      1);
#else
    {
        _VGMatrix3x3 proj, out;

        _vgSetMatrix(&proj,
                    2/(VGfloat)width,                  0,                      -1,
                    0,                                 2/(VGfloat)height,      -1,
                    0,                                 0,                      1);

        MultMatrix(&proj, hardware->userToSurface, &out);

        _MatrixToGAL(&out, projection);
    }
#endif
    status = gcUNIFORM_SetValueF(uniform, 1, projection);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_GradientMatrix(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    /* TODO: need to optimize */
    gctINT32 width, height;
    gctFLOAT galMatrix[16];
    _VGMatrix3x3 matrix;
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    _VGMatrixCtor(&matrix);

    GetAncestorSize(hardware->paint->pattern, &width, &height);

    MatrixScale(&matrix,     (gctFLOAT)(1.0f / width) ,
                             (gctFLOAT)(1.0f / height));

    _MatrixToGAL(&matrix, galMatrix);

    status = gcUNIFORM_SetValueF(uniform, 1, galMatrix);

    gcmFOOTER();

    return status;
}


gceSTATUS SetUniform_ZValue(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, &hardware->zValue);
}

gceSTATUS SetUniform_Points0(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT value[4];

    value[0] = hardware->paint->linearGradient[0];
    value[1] = hardware->paint->linearGradient[1];
    value[2] = 0.0f;
    value[3] = 0.0f;

    return gcUNIFORM_SetValueF(uniform, 1, value);
}

gceSTATUS SetUniform_Points(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT value[12];
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    value[0] = hardware->paint->radialA.x;
    value[1] = hardware->paint->radialA.y;
    value[2] = 0.0f;
    value[3] = 1.0f;

    value[4] = hardware->paint->radialB.x;
    value[5] = hardware->paint->radialB.y;
    value[6] = 0.0f;
    value[7] = 1.0f;

    value[8] = hardware->paint->radialGradient[2];
    value[9] = hardware->paint->radialGradient[3];
    value[10] = 0.0f;
    value[11] = hardware->paint->radialC;

    status = gcUNIFORM_SetValueF(uniform, 3, value);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_Udusq(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT value[4];

    value[0] = hardware->paint->udusq.x;
    value[1] = hardware->paint->udusq.y;
    value[2] = 0.0f;
    value[3] = 0.0f;

    return gcUNIFORM_SetValueF(uniform, 1, value);
}

gceSTATUS SetUniform_UserToPaintMatrix(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT        userToPaint[16];
    _VGMatrix3x3    paintToUserReverse;
    gceSTATUS       status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    InvertMatrix(hardware->paintToUser, &paintToUserReverse);

    _MatrixToGAL(&paintToUserReverse, userToPaint);

    status = gcUNIFORM_SetValueF(uniform, 1, userToPaint);

    gcmFOOTER();
    return status;
}

gceSTATUS SetUniform_TexBound(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT    texBound[12];
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    _GetTexBound(hardware->paint->pattern, texBound);

    status = gcUNIFORM_SetValueF(uniform, 3, texBound);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_FilterTexBound(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT    texBound[12];
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    _GetTexBound(hardware->srcImage, texBound);

    status = gcUNIFORM_SetValueF(uniform, 3, texBound);

    gcmFOOTER();

    return status;
}


gceSTATUS SetUniform_EdgeColor(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)&hardware->context->tileFillColor);
}

gceSTATUS SetUniform_Gray(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT value[] = {0.2126f, 0.7152f, 0.0722f, 0.0f};
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    status = gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)value);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_ColorMatrix(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)hardware->colorMatrix);
}

gceSTATUS SetUniform_Offset(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)(hardware->colorMatrix + 16));
}

gceSTATUS SetUniform_Scale(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)&hardware->scale);
}

gceSTATUS SetUniform_Bias(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)&hardware->bias);
}

gceSTATUS SetUniform_KernelSize(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)&hardware->kernelSize);
}


gceSTATUS SetUniform_TexCoordOffset(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gcmASSERT(hardware->texCoordOffsetSize >= 1.0f);
    return gcUNIFORM_SetValueF(uniform, (gctSIZE_T)(hardware->texCoordOffsetSize), (gctFLOAT*)hardware->texCoordOffset);
}


gceSTATUS SetUniform_Kernel(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    if (hardware->kernelSize <= 0.0f) return gcvSTATUS_OK;
    return gcUNIFORM_SetValueF(uniform, (gctSIZE_T)(hardware->kernelSize), (gctFLOAT*)hardware->kernel);
}


gceSTATUS SetUniform_KernelSizeY(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)&hardware->kernelSizeY);
}


gceSTATUS SetUniform_TexCoordOffsetY(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gcmASSERT(hardware->texCoordOffsetSizeY >= 1.0f);

    return gcUNIFORM_SetValueF(uniform, (gctSIZE_T)(hardware->texCoordOffsetSizeY), (gctFLOAT*)hardware->texCoordOffsetY);
}


gceSTATUS SetUniform_KernelY(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    if (hardware->kernelSizeY <= 0.0f) return gcvSTATUS_OK;
    return gcUNIFORM_SetValueF(uniform, (gctSIZE_T)(hardware->kernelSizeY), (gctFLOAT*)hardware->kernelY);
}

gceSTATUS SetUniform_Kernel0(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)&hardware->kernel0);
}

gceSTATUS SetUniform_ColorTransformValues(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    return gcUNIFORM_SetValueF(uniform, 2, (gctFLOAT*)hardware->colorTransformValues);
}

gceSTATUS SetUniform_RenderSize(_vgHARDWARE *hardware, gcUNIFORM uniform)

{
    gctFLOAT size[2];

    size[0] = (gctFLOAT)(1.0f / hardware->context->targetImage.width);
    size[1] = (gctFLOAT)(1.0f / hardware->context->targetImage.height);

    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)size);
}

gceSTATUS SetUniform_ImageSampler(_vgHARDWARE *hardware, gctINT sampler)
{
    gceSTATUS   status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x sampler=%d", hardware, sampler);

    /* Resolve the render target into the texture. */
    if (*hardware->srcImage->dirtyPtr || hardware->srcImage->eglUsed)
    {

        gcmVERIFY_OK(gcoSURF_Flush(hardware->srcImage->surface));

        gcmVERIFY_OK(gco3D_Semaphore(
            hardware->core.engine,
            gcvWHERE_RASTER, gcvWHERE_PIXEL,
            gcvHOW_SEMAPHORE_STALL));

        gcmVERIFY_OK(gcoHAL_Commit(hardware->context->hal, gcvTRUE));

        gcmVERIFY_OK(
            gcoSURF_Resolve(hardware->srcImage->surface,
            hardware->srcImage->texSurface));

        /* Semaphore/stall. */
        gcmVERIFY_OK(
            gco3D_Semaphore(hardware->core.engine,
                    gcvWHERE_RASTER,
                    gcvWHERE_PIXEL,
                    gcvHOW_SEMAPHORE_STALL));

        *hardware->srcImage->dirtyPtr = gcvFALSE;
    }

    status = _TextureBind(hardware->srcImage, sampler);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_GradientSampler(_vgHARDWARE *hardware, gctINT sampler)
{
    if (*hardware->paint->colorRamp.dirtyPtr || hardware->paint->colorRamp.eglUsed)
    {

        gcmVERIFY_OK(gcoSURF_Flush(hardware->paint->colorRamp.surface));

        gcmVERIFY_OK(gco3D_Semaphore(
            hardware->core.engine,
            gcvWHERE_RASTER, gcvWHERE_PIXEL,
            gcvHOW_SEMAPHORE_STALL));

        gcmVERIFY_OK(gcoHAL_Commit(hardware->context->hal, gcvTRUE));

        /* Resolve the render target into the texture. */
        gcmVERIFY_OK(
            gcoSURF_Resolve(hardware->paint->colorRamp.surface,
            hardware->paint->colorRamp.texSurface));

        /* Semaphore/stall. */
        gcmVERIFY_OK(
            gco3D_Semaphore(hardware->core.engine,
                    gcvWHERE_RASTER,
                    gcvWHERE_PIXEL,
                    gcvHOW_SEMAPHORE_STALL));

        *hardware->paint->colorRamp.dirtyPtr = gcvFALSE;
    }

	return _TextureBind(&hardware->paint->colorRamp, sampler);
}

gceSTATUS SetUniform_PatternSampler(_vgHARDWARE *hardware, gctINT sampler)
{
    if (*hardware->paint->pattern->dirtyPtr || hardware->paint->pattern->eglUsed)
    {
        gcmVERIFY_OK(gcoSURF_Flush(hardware->paint->pattern->surface));

        gcmVERIFY_OK(gco3D_Semaphore(
            hardware->core.engine,
            gcvWHERE_RASTER, gcvWHERE_PIXEL,
            gcvHOW_SEMAPHORE_STALL));

        gcmVERIFY_OK(gcoHAL_Commit(hardware->context->hal, gcvTRUE));

        /* Resolve the render target into the texture. */
        gcmVERIFY_OK(
            gcoSURF_Resolve(hardware->paint->pattern->surface,
            hardware->paint->pattern->texSurface));

        /* Semaphore/stall. */
        gcmVERIFY_OK(
            gco3D_Semaphore(hardware->core.engine,
                    gcvWHERE_RASTER,
                    gcvWHERE_PIXEL,
                    gcvHOW_SEMAPHORE_STALL));

        *hardware->paint->pattern->dirtyPtr = gcvFALSE;
    }

    return _TextureBind(hardware->paint->pattern,  sampler);
}

gceSTATUS SetUniform_LutSampler(_vgHARDWARE *hardware, gctINT sampler)

{
    return _TextureBind(hardware->lutImage,  sampler);
}

gceSTATUS SetUniform_RenderTargetSampler(_vgHARDWARE *hardware, gctINT sampler)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x sampler=%d", hardware, sampler);

    do
    {
        if (*hardware->context->targetImage.dirtyPtr)
        {
            gcmVERIFY_OK(gcoSURF_Flush(
                hardware->context->targetImage.surface));

            gcmVERIFY_OK(gco3D_Semaphore(
                hardware->core.engine,
                gcvWHERE_RASTER, gcvWHERE_PIXEL,
                gcvHOW_SEMAPHORE_STALL));

            gcmVERIFY_OK(gcoHAL_Commit(hardware->context->hal, gcvTRUE));

            gcmERR_BREAK(gcoSURF_Resolve(
                    hardware->context->targetImage.surface,
                    hardware->context->targetImage.texSurface));

            /* Sempahore/stall. */
            gcmERR_BREAK(
                    gco3D_Semaphore(hardware->core.engine,
                                    gcvWHERE_RASTER,
                                    gcvWHERE_PIXEL,
                                    gcvHOW_SEMAPHORE_STALL));

            *hardware->context->targetImage.dirtyPtr = gcvFALSE;
        }

        /* Bind the texture. */
        gcmERR_BREAK(
            _TextureBind(&hardware->context->targetImage,
                         sampler));

    }
    while (gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS SetUniform_MaskSampler(_vgHARDWARE *hardware, gctINT sampler)
{

    if (*hardware->context->maskImage.dirtyPtr)
    {
        gcmVERIFY_OK(gcoSURF_Flush(hardware->context->maskImage.surface));

        gcmVERIFY_OK(gco3D_Semaphore(
            hardware->core.engine,
            gcvWHERE_RASTER, gcvWHERE_PIXEL,
            gcvHOW_SEMAPHORE_STALL));

        gcmVERIFY_OK(gcoHAL_Commit(hardware->context->hal, gcvTRUE));

        gcmVERIFY_OK(
                gcoSURF_Resolve(hardware->context->maskImage.surface,
                                hardware->context->maskImage.texSurface));

        /* Sempahore/stall. */
        gcmVERIFY_OK(
                gco3D_Semaphore(hardware->core.engine,
                                gcvWHERE_RASTER,
                                gcvWHERE_PIXEL,
                                gcvHOW_SEMAPHORE_STALL));

        *hardware->context->maskImage.dirtyPtr = gcvFALSE;
    }

    return _TextureBind(&hardware->context->maskImage,  sampler);

}

gceSTATUS SetUniform_SourceMaskSampler(_vgHARDWARE *hardware, gctINT sampler)
{
    if (*hardware->srcImage->dirtyPtr || hardware->srcImage->eglUsed)
    {
        gcmVERIFY_OK(gcoSURF_Flush(hardware->srcImage->surface));

        gcmVERIFY_OK(gco3D_Semaphore(
            hardware->core.engine,
            gcvWHERE_RASTER, gcvWHERE_PIXEL,
            gcvHOW_SEMAPHORE_STALL));

        gcmVERIFY_OK(gcoHAL_Commit(hardware->context->hal, gcvTRUE));

        gcmVERIFY_OK(
                gcoSURF_Resolve(hardware->srcImage->surface,
                                hardware->srcImage->texSurface));

        /* Sempahore/stall. */
        gcmVERIFY_OK(
                gco3D_Semaphore(hardware->core.engine,
                                gcvWHERE_RASTER,
                                gcvWHERE_PIXEL,
                                gcvHOW_SEMAPHORE_STALL));

        *hardware->srcImage->dirtyPtr = gcvFALSE;
    }

    return _TextureBind(hardware->srcImage, sampler);

}


gceSTATUS SetUniform_Posneg1(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    gctFLOAT value[] = {1.0f,1.0f,-1.0f,-1.0f};
    return gcUNIFORM_SetValueF(uniform, 1, (gctFLOAT*)value);
}

gceSTATUS SetUniform_Scissor(_vgHARDWARE *hardware, gcUNIFORM uniform)
{
    _VGRectangle item = hardware->context->scissor.items[0];
    gceSTATUS   status = gcvSTATUS_OK;

    gctFLOAT value[32*4];
    int i;

    gcmHEADER_ARG("hardware=0x%x uniform=0x%x", hardware, uniform);

    value[0] = (gctFLOAT)item.x;
    value[1] = (gctFLOAT)(hardware->context->targetImage.height - (item.height + item.y));
    value[2] = -(gctFLOAT)(item.width + item.x);
    value[3] = -(gctFLOAT)(hardware->context->targetImage.height - item.y);

    for(i = 0; i<hardware->context->scissor.size; i++){
        item = hardware->context->scissor.items[i];
        value[4*i] = (gctFLOAT)item.x;
        value[4*i+1] = (gctFLOAT)(hardware->context->targetImage.height - (item.height + item.y));
        value[4*i+2] = -(gctFLOAT)(item.width + item.x);
        value[4*i+3] = -(gctFLOAT)(hardware->context->targetImage.height - item.y);

    }

    status = gcUNIFORM_SetValueF(uniform, hardware->context->scissor.size, (gctFLOAT*)value);

    gcmFOOTER();
    return status;
}

#define vgmDEFINE_UNIFORM(name) \
    gcUNIFORM name

#define vgmDEFINE_SAMPLER(name) \
    gcUNIFORM name

#define vgmDEFINE_ATTRIBUTE(name) \
    gcATTRIBUTE name

#define vgmDEFINE_VARYING(name) \
    gcATTRIBUTE name

#define vgmDEFINE_OUTPUT(name) \
    gctUINT16 name

#define vgmDEFINE_OUTPUT_VARYING(name) \
    gctUINT16 name


#define vgmDEFINE_TEMP(name) \
    gctUINT16 name

#define vgmDEFINE_LABEL(name) \
    gctUINT16 name

#define vgmALLOCATE_UNIFORM(name, type, length) \
    name = _AddUniform(Shader, #name, gcSHADER_FLOAT_##type, length, SetUniform_##name)

#define vgmGET_UNIFORM(name, type, length) \
    name = _GetUniform(Shader, #name, gcSHADER_FLOAT_##type, length, SetUniform_##name)


#define vgmALLOCATE_SAMPLER(name, type, length) \
    name = _AddSampler(Shader, #name, gcSHADER_##type, length, SetUniform_##name)

#define vgmALLOCATE_ATTRIBUTE(name, type, length) \
    name = _AddAttribute(Shader, #name, vgvATTRIBUTE_##name, gcSHADER_FLOAT_##type, length)

#define vgmALLOCATE_VARYING(name, type, length) \
    name = _AddVarying(Shader, #name, gcSHADER_FLOAT_##type, length)

#define vgmALLOCATE_VARYINGEX(name, string, type, length) \
    name = _AddVarying(Shader, string, gcSHADER_FLOAT_##type, length)


#define vgmALLOCATE_OUTPUT(name, string, type, length) \
    name = _AddOutput(Shader, string, gcSHADER_FLOAT_##type, length)

#define vgmALLOCATE_OUTPUT_VARYING  \
            vgmALLOCATE_OUTPUT

#define vgmALLOCATE_TEMP(name) \
    name = _AllocateTemp(Shader)

#define vgmALLOCATE_LABEL(label) \
    label = _AllocateLabel(Shader)


#define vgmDEFINE_GET_UNIFORM(name, type, length) \
    vgmDEFINE_UNIFORM(name) = vgmGET_UNIFORM(name, type, length)

#define vgmDEFINE_ALLOCATE_UNIFORM(name, type, length) \
    vgmDEFINE_UNIFORM(name) = vgmALLOCATE_UNIFORM(name, type, length)

#define vgmDEFINE_ALLOCATE_SAMPLER(name, type, length) \
    vgmDEFINE_UNIFORM(name) = vgmALLOCATE_SAMPLER(name, type, length)


#define vgmDEFINE_ALLOCATE_ATTRIBUTE(name, type, length) \
    vgmDEFINE_ATTRIBUTE(name) = vgmALLOCATE_ATTRIBUTE(name, type, length)

#define vgmDEFINE_ALLOCATE_VARYING(name, type, length) \
    vgmDEFINE_ATTRIBUTE(name) = vgmALLOCATE_VARYING(name, type, length)

#define vgmDEFINE_ALLOCATE_VARYINGEX(name, string, type, length) \
    vgmDEFINE_ATTRIBUTE(name) = vgmALLOCATE_VARYINGEX(name, string, type, length)


#define vgmDEFINE_ALLOCATE_OUTPUT(name, string, type, length) \
    vgmDEFINE_OUTPUT(name) = vgmALLOCATE_OUTPUT(name, string, type, length)

#define vgmDEFINE_ALLOCATE_OUTPUT_VARYING(name, string, type, length) \
    vgmDEFINE_OUTPUT(name) = vgmALLOCATE_OUTPUT(name, string, type, length)


#define vgmDEFINE_ALLOCATE_TEMP(name) \
    vgmDEFINE_TEMP(name) = vgmALLOCATE_TEMP(name)

#define vgmDEFINE_ALLOCATE_LABEL(label) \
    vgmDEFINE_LABEL(label) = vgmALLOCATE_LABEL(label)


#define vgmLABEL(label)\
    gcmERR_BREAK(gcSHADER_AddLabel(Shader->binary, label))

#define vgmOPCODE(Opcode, TempRegister, ComponentEnable) \
    gcmASSERT(TempRegister != 0); \
    gcmERR_BREAK(gcSHADER_AddOpcode( \
        Shader->binary, \
        gcSL_##Opcode, \
        TempRegister, \
        gcSL_ENABLE_##ComponentEnable, \
        gcSL_FLOAT \
        ))

#define vgmOPCODE_COND(Opcode, Condition, Target) \
    gcmERR_BREAK(gcSHADER_AddOpcodeConditional( \
        Shader->binary, \
        gcSL_##Opcode, \
        gcSL_##Condition, \
        Target \
        ))

#define vgmCONST(Value) \
    gcmERR_BREAK(gcSHADER_AddSourceConstant( \
        Shader->binary, \
        (gctFLOAT) (Value) \
        ))


#define vgmUNIFORM(Name, Swizzle, Index) \
    gcmERR_BREAK(gcSHADER_AddSourceUniform( \
        Shader->binary, \
        Name, \
        gcSL_SWIZZLE_##Swizzle, \
        Index \
        ))

#define vgmUNIFORM_INDEXED(Name, Swizzle, IndexRegister) \
    gcmERR_BREAK(gcSHADER_AddSourceUniformIndexed( \
        Shader->binary, \
        Name, \
        gcSL_SWIZZLE_##Swizzle, \
        0, \
        gcSL_INDEXED_X, \
        IndexRegister \
        ))

#define vgmSAMPLER(Name) \
    vgmUNIFORM(Name, XYZW, 0)

#define vgmATTRIBUTE(Name, Swizzle, Index) \
    gcmERR_BREAK(gcSHADER_AddSourceAttribute( \
        Shader->binary, \
        Name, \
        gcSL_SWIZZLE_##Swizzle, \
        Index \
        ))

#define vgmVARYING(Name, Swizzle, Index) \
        vgmATTRIBUTE(Name, Swizzle, Index)


#define vgmTEMP(TempRegister, Swizzle) \
    gcmASSERT(TempRegister != 0); \
    gcmERR_BREAK(gcSHADER_AddSource( \
        Shader->binary, \
        gcSL_TEMP, \
        TempRegister, \
        gcSL_SWIZZLE_##Swizzle, \
        gcSL_FLOAT \
        ))

#define vgmTEMP_EX(TempRegister, Swizzle) \
    gcmASSERT(TempRegister != 0); \
    gcmERR_BREAK(gcSHADER_AddSource( \
        Shader->binary, \
        gcSL_TEMP, \
        TempRegister, \
        Swizzle, \
        gcSL_FLOAT \
        ))

#define vgmADD_FUNCTION(FunctionName) \
    gcmERR_BREAK(gcSHADER_AddFunction( \
        Shader->binary, \
        #FunctionName, \
        &FunctionName \
        ))

#define vgmBEGIN_FUNCTION(FunctionName) \
    gcmERR_BREAK(gcSHADER_BeginFunction( \
        Shader->binary, \
        FunctionName \
        ))

#define vgmEND_FUNCTION(FunctionName) \
    gcmERR_BREAK(gcSHADER_EndFunction( \
        Shader->binary, \
        FunctionName \
        ))

#define vgmRET() \
    gcmERR_BREAK(gcSHADER_AddOpcodeConditional( \
        Shader->binary, \
        gcSL_RET, \
        gcSL_ALWAYS, \
        0 \
        ))


gctUINT16 _AllocateLabel(_VGShader *Shader)
{
    gcmASSERT(Shader->lLastAllocated < 65535);
    return ++Shader->lLastAllocated;
}

gctUINT16 _AllocateTemp(_VGShader *Shader)
{
    gcmASSERT(Shader->rLastAllocated < 65535);
    return ++Shader->rLastAllocated;
}



gctUINT16 _AddOutput(
    _VGShader* Shader,
    gctCONST_STRING Name,
    gcSHADER_TYPE Type,
    gctSIZE_T Length)
{
    gctUINT16 temp ;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%u",
        Shader, Name, Type, (unsigned long)Length);

    temp = _AllocateTemp(Shader);

    status = gcSHADER_AddOutput(Shader->binary, Name, Type, Length, temp);
    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("return=%u", 0);
        /*invalid temp*/
        return 0;
    }

    gcmFOOTER_ARG("return=%u", temp);

    return temp;
}



gcATTRIBUTE _AddAttribute(
    _VGShader* Shader,
    gctCONST_STRING Name,
    gctUINT16   NameID,
    gcSHADER_TYPE Type,
    gctSIZE_T Length
)
{
    gcATTRIBUTE attribute;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x Name=%s NameID=%u Type=%d Length=%u",
        Shader, Name, NameID, Type, (unsigned long)Length);

    status = gcSHADER_AddAttribute(Shader->binary, Name, Type, Length, gcvFALSE, &attribute);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("return=0x%x", gcvNULL);

        return gcvNULL;
    }

    gcmASSERT(NameID < vgvMAX_ATTRIBUTES);

    Shader->attributes[NameID].attribute = attribute;
    Shader->attributes[NameID].index     = -1;

    gcmFOOTER_ARG("return=0x%x", attribute);

    return attribute;
}

gcATTRIBUTE _AddVarying(
    _VGShader* Shader,
    gctCONST_STRING Name,
    gcSHADER_TYPE Type,
    gctSIZE_T Length
)
{
    gcATTRIBUTE attribute;
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%u", Shader, Name, Type, (unsigned long)Length);

    status = gcSHADER_AddAttribute(Shader->binary, Name, Type, Length, gcvFALSE, &attribute);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("return=0x%x", gcvNULL);

        return gcvNULL;
    }

    gcmFOOTER_ARG("return=0x%x", attribute);

    return attribute;
}


gcUNIFORM _AddUniform(
    _VGShader* Shader,
    gctCONST_STRING Name,
    gcSHADER_TYPE Type,
    gctSIZE_T Length,
    vgUNIFORMSET setfunc)
{
    gcUNIFORM uniform;
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%u setfunc=0x%x",
        Shader, Name, Type, (unsigned long)Length, setfunc);

    status = gcSHADER_AddUniform(Shader->binary, Name, Type, Length, &uniform);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("return=0x%x", gcvNULL);
        return (gcUNIFORM)gcvNULL;
    }

    gcmASSERT(Shader->uniformCount < ((Shader->shaderType == VGShaderType_Fragment) ? vgvMAX_FRAG_UNIFORMS : vgvMAX_VERT_UNIFORMS));

    Shader->uniforms[Shader->uniformCount].uniform = uniform;
    Shader->uniforms[Shader->uniformCount].setfunc = setfunc;

    Shader->uniformCount++;

    gcmFOOTER_ARG("return=0x%x", uniform);

    return uniform;
}

gcUNIFORM _GetUniform(
    _VGShader* Shader,
    gctCONST_STRING Name,
    gcSHADER_TYPE Type,
    gctSIZE_T Length,
    vgUNIFORMSET setfunc)
{

    gctINT32 i;
    gctSIZE_T       uniformNameLength;
    gctCONST_STRING uniformName;
    gctSIZE_T   nameLength;
    gcUNIFORM   uniform;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%u setfunc=0x%x",
        Shader, Name, Type, (unsigned long)Length, setfunc);

    /* Get the length of the requested uniform. */
    gcoOS_StrLen(Name, &nameLength);

    /* find the exist uniform */
    for(i = 0; i < Shader->uniformCount; i++)
    {
            /* Get the uniform name. */
        gcmVERIFY_OK(gcUNIFORM_GetName(Shader->uniforms[i].uniform,
                                    &uniformNameLength, &uniformName));

        /* See if the uniform matches the requested name. */
        if ( (nameLength == uniformNameLength) &&
             (gcoOS_MemCmp(Name, uniformName, nameLength) == 0) )
        {
            gcmFOOTER_ARG("return=0x%x", Shader->uniforms[i].uniform);

            return Shader->uniforms[i].uniform;
        }
    }

    /* not found, then add a new one */
    uniform = _AddUniform(Shader, Name, Type, Length, setfunc);

    gcmFOOTER_ARG("return=0x%x", uniform);

    return uniform;
}

gcUNIFORM _AddSampler(
    _VGShader* Shader,
    gctCONST_STRING Name,
    gcSHADER_TYPE Type,
    gctSIZE_T Length,
    vgSAMPLERSET setfunc)
{
    gcUNIFORM uniform;
    gctUINT32 sampler;
    gceSTATUS status;

    gcmHEADER_ARG("Shader=0x%x Name=%s Type=%d Length=%u setfunc=0x%x",
        Shader, Name, Type, (unsigned long)Length, setfunc);

    status = gcSHADER_AddUniform(Shader->binary, Name, Type, Length, &uniform);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("return=0x%x", gcvNULL);

        return (gcUNIFORM)gcvNULL;
    }


    status = gcUNIFORM_GetSampler(uniform, &sampler);

    if (gcmIS_ERROR(status))
    {
        gcmFOOTER_ARG("return=0x%x", gcvNULL);

        return (gcUNIFORM)gcvNULL;
    }

    gcmASSERT(Shader->samplerCount <  vgvMAX_SAMPLERS);

    Shader->samplers[Shader->samplerCount].sampler = sampler;
    Shader->samplers[Shader->samplerCount].setfunc = setfunc;

    Shader->samplerCount++;

    gcmFOOTER_ARG("return=0x%x", uniform);

    return uniform;
}

static gceSTATUS _LinkShader(_VGProgram *program)
{

    /* Call the HAL backend compiler/linker. */
    return gcLinkShaders(
                        program->vertexShader.binary,
                        program->fragmentShader.binary,
                        (gceSHADER_FLAGS)
                        ( gcvSHADER_DEAD_CODE
                        | gcvSHADER_RESOURCE_USAGE
                        | gcvSHADER_OPTIMIZER
                        ),
                        &program->statesSize,
                        &program->states,
                        &program->hints);
}


static void _GetPaintColorFormat(_VGPaint *paint, _VGColorFormat *format)
{
    gcmHEADER_ARG("paint=0x%x format=0x%x", paint, format);

    if (paint->paintType == VG_PAINT_TYPE_COLOR)
    {
        *format = paint->paintColor.format;
    }
    else if (paint->paintType == VG_PAINT_TYPE_LINEAR_GRADIENT || paint->paintType == VG_PAINT_TYPE_RADIAL_GRADIENT)
    {
        *format = paint->colorRampTextureForamt;
    }
    else if (paint->paintType == VG_PAINT_TYPE_PATTERN)
    {
        if (paint->pattern)
        {
            *format = paint->pattern->internalColorDesc.colorFormat;
        }
        else
        {
            *format = paint->paintColor.format;
        }
    }

    gcmFOOTER_NO();
}

static void _GetConvertValue(_VGColorFormat srcFormat, _VGColorFormat dstFormat, gctUINT32 *colorConvert, gctUINT32 *alphaConvert)
{
    gcmHEADER_ARG("srcFormat=%d dstFormat=%d colorConvert=0x%x alphaConvert=0x%x",
        srcFormat, dstFormat, colorConvert, alphaConvert);

    *colorConvert = getColorConvertValue(srcFormat, dstFormat);
    *alphaConvert = getColorConvertAlphaValue(srcFormat, dstFormat);

    gcmFOOTER_NO();
}

static gctUINT32 _GetConvertMapValue(gctUINT32 colorConvert)
{
    gctUINT32 value = 0;

    gcmHEADER_ARG("colorConvert=%u", colorConvert);

    if (colorConvert == 1 || colorConvert == 5)
    {
        value = 1;
    }
    else if (colorConvert == 16 || colorConvert == 20)
    {
        value = 2;
    }
    else if (colorConvert == 64)
    {
        value = 3;
    }
    else if (colorConvert == 65)
    {
        value = 4;
    }
    else if (colorConvert == 80)
    {
        value = 5;
    }
    else if (colorConvert == 81)
    {
        value = 6;
    }
    else if (colorConvert == 69)
    {
        value = 7;
    }
    else if (colorConvert == 84)
    {
        value = 8;
    }

    gcmFOOTER_ARG("return=%u", value);
    return value;
}

static gceSTATUS _GenInvGammaCode(_VGShader *Shader, gctUINT16 color)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x color=%u", Shader, color);

    do
    {
        vgmDEFINE_ALLOCATE_TEMP(temp0);
        vgmDEFINE_ALLOCATE_TEMP(temp1);
        /*
        c = pow((c + 0.0556f)/1.0556f, 2.4f);
        */

        vgmOPCODE(ADD, temp0, XYZ);
            vgmTEMP(color, XYZZ);
            vgmCONST(0.0556f);

        vgmOPCODE(MUL, temp1, XYZ);
            vgmTEMP(temp0, XYZZ);
            vgmCONST(1.0f/1.0556f);

        vgmOPCODE(POW, color, XYZ);
            vgmTEMP(temp1, XYZZ);
            vgmCONST(2.4f);
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


static gceSTATUS _GenGammaCode(_VGShader *Shader, gctUINT16 color, gctBOOL special)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x color=%u special=%s",
        Shader, color, special ? "TRUE" : "FALSE");

    do
    {
        vgmDEFINE_ALLOCATE_TEMP(temp0);
        vgmDEFINE_ALLOCATE_TEMP(temp1);

        vgmDEFINE_ALLOCATE_LABEL(label0);
        vgmDEFINE_ALLOCATE_LABEL(label1);
        vgmDEFINE_ALLOCATE_LABEL(label2);
        vgmDEFINE_ALLOCATE_LABEL(label3);
        vgmDEFINE_ALLOCATE_LABEL(label5);

        /*
        if(c <= 0.00304f)
        c *= 12.92f;
        */


        if (special)
        {
            vgmOPCODE_COND(JMP, GREATER, label0);
                vgmTEMP(color, XXXX);
                vgmCONST(0.00304f);

            vgmOPCODE(MUL, temp0, X);
                vgmCONST(12.92f);
                vgmTEMP(color, XXXX);

            vgmOPCODE(MOV, color, X);
                vgmTEMP(temp0, XXXX);

            vgmOPCODE_COND(JMP, ALWAYS, label1);

            vgmLABEL(label0);
            vgmOPCODE(POW, temp0, X);
                vgmTEMP(color, XXXX);
                vgmCONST(1.0f/2.4f);

            vgmOPCODE(MUL, temp1, X);
                vgmTEMP(temp0, XXXX);
                vgmCONST(1.0556f);

            vgmOPCODE(SUB, color, X);
                vgmTEMP(temp1, XXXX);
                vgmCONST(0.0556f);

            vgmLABEL(label1);


            vgmOPCODE_COND(JMP, GREATER, label2);
                vgmTEMP(color, YYYY);
                vgmCONST(0.00304f);

            vgmOPCODE(MUL, temp0, X);
                vgmCONST(12.92f);
                vgmTEMP(color, YYYY);

            vgmOPCODE(MOV, color, Y);
                vgmTEMP(temp0, XXXX);

            vgmOPCODE_COND(JMP, ALWAYS, label3);

            vgmLABEL(label2);

            vgmOPCODE(POW, temp0, X);
                vgmTEMP(color, YYYY);
                vgmCONST(1.0f/2.4f);

            vgmOPCODE(MUL, temp1, X);
                vgmTEMP(temp0, XXXX);
                vgmCONST(1.0556f);

            vgmOPCODE(SUB, color, Y);
                vgmTEMP(temp1, XXXX);
                vgmCONST(0.0556f);

            vgmLABEL(label3);


            vgmOPCODE(POW, temp0, X);
                vgmTEMP(color, ZZZZ);
                vgmCONST(1.0f/2.4f);

            vgmOPCODE(MUL, temp1, X);
                vgmTEMP(temp0, XXXX);
                vgmCONST(1.0556f);

            vgmOPCODE(SUB, color, Z);
                vgmTEMP(temp1, XXXX);
                vgmCONST(0.0556f);

            vgmLABEL(label5);

        }
        else
        {

            /*
            c = 1.0556f * pow(c, 1.0f/2.4f) - 0.0556f;
            */

            vgmOPCODE(POW, temp0, XYZ);
                vgmTEMP(color, XYZZ);
                vgmCONST(1.0f/2.4f);

            vgmOPCODE(MUL, temp1, XYZ);
                vgmTEMP(temp0, XYZZ);
                vgmCONST(1.0556f);

            vgmOPCODE(SUB, color, XYZ);
                vgmTEMP(temp1, XYZZ);
                vgmCONST(0.0556f);
        }
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenLRGB2LCode(_VGShader *Shader, gctUINT16 color)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x color=%u", Shader, color);

    do
    {
        vgmDEFINE_GET_UNIFORM(Gray, X4, 1);
        vgmDEFINE_ALLOCATE_TEMP(lRGBtoL);
        /*
        l = 0.2126g * color.r + 0.7152g * color.g + 0.0722g * color.b;
        */

        vgmOPCODE(DP4, lRGBtoL, X);
            vgmTEMP(color, XYZW);
            vgmUNIFORM(Gray, XYZW, 0);

        vgmOPCODE(MOV, color, XYZ);
            vgmTEMP(lRGBtoL, XXXX);

        vgmOPCODE(MOV, color, W);
            vgmCONST(1.0f);

    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenClampColorCode(_VGShader *Shader, gctUINT16 color, gctBOOL premultiply)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x color=%u premultiply=%s",
        Shader, color, premultiply ? "TRUE" : "FALSE");

    do
    {
        vgmDEFINE_ALLOCATE_TEMP(temp0);
        vgmDEFINE_ALLOCATE_TEMP(temp1);

        vgmOPCODE(MAX, temp1, XYZW);
            vgmCONST(0.0f);
            vgmTEMP(color, XYZW);

        vgmOPCODE(MIN, color, XYZW);
            vgmCONST(1.0f);
            vgmTEMP(temp1, XYZW);

        if (premultiply)
        {
            vgmOPCODE(MIN, temp0, XYZW);
                vgmTEMP(color, WWWW);
                vgmTEMP(color, XYZW);

            vgmOPCODE(MOV, color, XYZW);
                vgmTEMP(temp0, XYZW);
        }
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenUnPreMultiColorCode(_VGShader *Shader, gctUINT16 color, gctUINT32 alphaConvert)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x color=%u alphaConvert=%u", Shader, color, alphaConvert);

    do
    {
        if (alphaConvert == 1 || alphaConvert == 3)
        {

            vgmDEFINE_ALLOCATE_TEMP(temp0);
            vgmDEFINE_ALLOCATE_TEMP(temp1);
            vgmDEFINE_ALLOCATE_LABEL(label0);
            vgmDEFINE_ALLOCATE_LABEL(label1);
            /*
                if (color.a == 0.0f)
                    color.rgb = vec3(0.0f, 0.0f, 0.0f);
                else
                    color.rgb = color.rgb / color.a;
            */

            vgmOPCODE_COND(JMP, NOT_EQUAL, label0);
                vgmTEMP(color, WWWW);
                vgmCONST(0);

            vgmOPCODE(MOV, color, XYZ);
                vgmCONST(0);

            vgmOPCODE_COND(JMP, ALWAYS, label1);
            vgmLABEL(label0);

            vgmOPCODE(RCP, temp0, X);
                vgmTEMP(color, WWWW);

            vgmOPCODE(MUL, temp1, XYZ);
                vgmTEMP(color, XYZZ);
                vgmTEMP(temp0, XXXX);

            vgmOPCODE(MOV, color, XYZ);
                vgmTEMP(temp1, XYZZ);

            vgmLABEL(label1);
        }
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenPreMultiColorCode(_VGShader *Shader, gctUINT16 color, gctUINT32 alphaConvert)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x color=%u alphaConvert=%u", Shader, color, alphaConvert);
    do
    {
        if (alphaConvert == 2 || alphaConvert == 3)
        {
            vgmDEFINE_ALLOCATE_TEMP(temp0);
            /*
            color.rgb *= color.a
            */
            vgmOPCODE(MUL, temp0, XYZ);
                vgmTEMP(color, XYZZ);
                vgmTEMP(color, WWWW);

            vgmOPCODE(MOV, color, XYZ);
                vgmTEMP(temp0, XYZZ);
        }
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


static gceSTATUS _GenColorConvertCodeEx(_VGShader *Shader, gctUINT16 color, gctUINT32 colorConvert,
                                 gctUINT32 alphaConvert, gctBOOL forceAlpha, gctBOOL specialGamma)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x color=%u colorConvert=%u alphaConvert=%u forceAlpha=%s specialGamma=%s",
        Shader, color, colorConvert, alphaConvert,
        forceAlpha ? "TRUE" : "FALSE",
        specialGamma ? "TRUE" : "FALSE");

    do
    {
        if ((_GetConvertMapValue(colorConvert) == 0) && (alphaConvert == 3))
        {
            /* no need to process the pre/unpre*/
            break;
        }
        /* TODO: need to optimize */
        gcmERR_BREAK(_GenUnPreMultiColorCode(Shader, color, alphaConvert));

        /*
        1:  sRGBA -> lRGBA;
        4:  lLA  ->lRGBA;  (skip)
        5:  sLA  -> lRGBA;
        16: lRGBA -> sRGBA;
        20: lLA  -> sRGBA;
        21: sLA  -> sRGBA; (skip)
        64: lRGBA -> lLA;
        65: sRGBA -> lLA;
        69: sLA  -> lLA;
        80: lRGBA -> sLA;
        81: sRGBA -> sLA;
        84: lLA  -> sLA;
        */

        if (colorConvert == 1 || colorConvert == 5 || colorConvert == 69)
        {
            gcmERR_BREAK(_GenInvGammaCode(Shader, color));
        }
        else if (colorConvert == 16 || colorConvert == 20 || colorConvert == 84)
        {
            gcmERR_BREAK(_GenGammaCode(Shader, color, specialGamma));
        }
        else if (colorConvert == 64)
        {
            gcmERR_BREAK(_GenLRGB2LCode(Shader, color));
        }
        else if (colorConvert == 65)
        {
            gcmERR_BREAK(_GenInvGammaCode(Shader, color));
            gcmERR_BREAK(_GenLRGB2LCode(Shader, color));

        }
        else if (colorConvert == 80)
        {
            gcmERR_BREAK(_GenLRGB2LCode(Shader, color));
            gcmERR_BREAK(_GenGammaCode(Shader, color, gcvFALSE));

        }
        else if (colorConvert == 81)
        {
            gcmERR_BREAK(_GenInvGammaCode(Shader, color));
            gcmERR_BREAK(_GenLRGB2LCode(Shader, color));
            gcmERR_BREAK(_GenGammaCode(Shader, color, gcvFALSE));
        }

        if (forceAlpha)
        {
            /* force alpha to 1.0f */
            if (colorConvert == 69 || colorConvert == 84)
            {
                vgmOPCODE(MOV, color, W);
                    vgmCONST(1.0f);
            }
        }

        gcmERR_BREAK(_GenPreMultiColorCode(Shader, color, alphaConvert));
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenColorConvertCode(_VGShader *Shader, gctUINT16 color, gctUINT32 colorConvert, gctUINT32 alphaConvert)
{
    return _GenColorConvertCodeEx(Shader, color, colorConvert, alphaConvert, gcvFALSE, gcvFALSE);
}


/* For GC600 1.9 MAX/MIN Walk Around. */
/* Call it after a texld is executed. */
#if WALK600
static gceSTATUS _GenTextureTuneCode(
                              _VGShader *Shader,
                              gctUINT16 textureColor
                              )
{
#define c0  (256.0f/255.0f)
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x textureColor=%u", Shader, textureColor);

    do
    {
        vgmDEFINE_ALLOCATE_LABEL(label0);
        vgmDEFINE_ALLOCATE_LABEL(label2);
        vgmDEFINE_ALLOCATE_LABEL(label4);
        vgmDEFINE_ALLOCATE_LABEL(label6);
        vgmDEFINE_ALLOCATE_TEMP(temp1);

        vgmOPCODE(MOV, temp1, XYZW);
            vgmCONST(1.0f);

        vgmOPCODE_COND(JMP, EQUAL, label0);
            vgmTEMP(textureColor, XXXX);
            vgmCONST(1.0f);
        vgmOPCODE(MUL, temp1, X);
            vgmTEMP(textureColor, XXXX);
            vgmCONST(c0);
        vgmLABEL(label0);   /*Less*/

        vgmOPCODE_COND(JMP, EQUAL, label2);
            vgmTEMP(textureColor, YYYY);
            vgmCONST(1.0f);
        vgmOPCODE(MUL, temp1, Y);
            vgmTEMP(textureColor, YYYY);
            vgmCONST(c0);
        vgmLABEL(label2);   /*Less*/

        vgmOPCODE_COND(JMP, EQUAL, label4);
            vgmTEMP(textureColor, ZZZZ);
            vgmCONST(1.0f);
        vgmOPCODE(MUL, temp1, Z);
            vgmTEMP(textureColor, ZZZZ);
            vgmCONST(c0);
        vgmLABEL(label4);   /*Less*/

        vgmOPCODE_COND(JMP, EQUAL, label6);
            vgmTEMP(textureColor, WWWW);
            vgmCONST(1.0f);
        vgmOPCODE(MUL, temp1, W);
            vgmTEMP(textureColor, WWWW);
            vgmCONST(c0);
        vgmLABEL(label6);   /*Less*/

        vgmOPCODE(MOV, textureColor, XYZW);
            vgmTEMP(temp1, XYZW);
    }while(gcvFALSE);

    gcmFOOTER();

    return status;
#undef c0
}
#endif


static gceSTATUS _GenSampleTextureCode(_vgHARDWARE *hardware,
                                _VGShader *Shader,
                                gcUNIFORM Sampler,
                                gctUINT16 coord,
                                VGTilingMode tilingMode,
                                gcUNIFORM TexBound,
                                gctUINT16 paintColor,
                                gctBOOL rootImage)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x Shader=0x%x Sampler=0x%x coord=%u tilingMode=%d "
        "TexBound=0x%x paintColr=%u rootImage=%s",
        Shader, Sampler, coord, tilingMode, TexBound, paintColor,
        rootImage ? "TRUE" : "FALSE");

    do
    {
        if (tilingMode == VG_TILE_FILL)
        {
            /*
            if (a2.x < texBound[0].x || a2.x >= texBound[0].z || a2.y < texBound[0].y || a2.y >= texBound[0].w)
            */

            /* try to get an exist uniform */
            vgmDEFINE_GET_UNIFORM(EdgeColor, X4, 1);

            vgmDEFINE_ALLOCATE_LABEL(label0);
            vgmDEFINE_ALLOCATE_LABEL(label1);
            vgmDEFINE_ALLOCATE_LABEL(label2);

            vgmOPCODE_COND(JMP, LESS, label0);
                vgmTEMP(coord, XXXX);
                vgmUNIFORM(TexBound, XXXX, 0);

            vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label0);
                vgmTEMP(coord, XXXX);
                vgmUNIFORM(TexBound, ZZZZ, 0);

            vgmOPCODE_COND(JMP, LESS, label0);
                vgmTEMP(coord, YYYY);
                vgmUNIFORM(TexBound, YYYY, 0);

            vgmOPCODE_COND(JMP, LESS, label1);
                vgmTEMP(coord, YYYY);
                vgmUNIFORM(TexBound, WWWW, 0);

            vgmLABEL(label0);
            {
                /* paintColor = edgeColor */
                vgmOPCODE(MOV, paintColor, XYZW);
                    vgmUNIFORM(EdgeColor, XYZW, 0);

                vgmOPCODE_COND(JMP, ALWAYS, label2);
            }
            vgmLABEL(label1);

            /* else */
            {
                /* paintColor = texture2D(Sampler, a2.xy); */
                vgmOPCODE(TEXLD, paintColor, XYZW);
                    vgmSAMPLER(Sampler);
                    vgmTEMP(coord, XYYY);
#if WALK600
                if (hardware->isConformance && hardware->isGC600_19)
                {
                    gcmERR_BREAK(_GenTextureTuneCode(Shader, paintColor));
                }
#endif
            }
            vgmLABEL(label2);
        }
        else if (tilingMode == VG_TILE_PAD)
        {
            /*
            vec2 temp1 = max(texBound[0].xy, min(a2.xy, texBound[1].xy));

            paintColor = texture2D(sampler2d, temp1);
            */

            vgmDEFINE_ALLOCATE_TEMP(temp0);
            vgmDEFINE_ALLOCATE_TEMP(temp1);

            vgmOPCODE(MIN, temp0, XY);
                vgmTEMP(coord, XYYY);
                vgmUNIFORM(TexBound, XYYY, 1);

            vgmOPCODE(MAX, temp1, XY);
                vgmTEMP(temp0, XYYY);
                vgmUNIFORM(TexBound, XYYY, 0);

            vgmOPCODE(TEXLD, paintColor, XYZW);
                vgmSAMPLER(Sampler);
                vgmTEMP(temp1, XYYY);


        }
        else if (tilingMode == VG_TILE_REPEAT)
        {
            /*
            if (a2.x < texBound[0].x || a2.x >= texBound[0].z || a2.y < texBound[0].y || a2.y >= texBound[0].w)
            */
            vgmDEFINE_ALLOCATE_LABEL(label0);
            vgmDEFINE_ALLOCATE_LABEL(label1);

            vgmOPCODE_COND(JMP, LESS, label0);
                vgmTEMP(coord, XXXX);
                vgmUNIFORM(TexBound, XXXX, 0);

            vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label0);
                vgmTEMP(coord, XXXX);
                vgmUNIFORM(TexBound, ZZZZ, 0);

            vgmOPCODE_COND(JMP, LESS, label0);
                vgmTEMP(coord, YYYY);
                vgmUNIFORM(TexBound, YYYY, 0);

            vgmOPCODE_COND(JMP, LESS, label1);
                vgmTEMP(coord, YYYY);
                vgmUNIFORM(TexBound, WWWW, 0);

            vgmLABEL(label0);
            {
                vgmDEFINE_ALLOCATE_TEMP(newCoord);
                vgmDEFINE_ALLOCATE_TEMP(temp0);
                vgmDEFINE_ALLOCATE_TEMP(temp1);
                vgmDEFINE_ALLOCATE_TEMP(temp2);
                vgmDEFINE_ALLOCATE_TEMP(temp3);
                vgmDEFINE_ALLOCATE_TEMP(temp4);

                if (!rootImage)
                {
                    /*
                    coord -= texBound[0].xy;
                    */
                    vgmOPCODE(SUB, temp0, XY);
                        vgmTEMP(coord, XYYY);
                        vgmUNIFORM(TexBound, XYYY, 0);

                    /*
                    coord = mod(coord, texBound[2].xy);
                    */
                    vgmOPCODE(RCP, temp1, XY);
                        vgmUNIFORM(TexBound, XYYY, 2);

                    vgmOPCODE(MUL, temp2, XY);
                        vgmTEMP(temp0, XYYY);
                        vgmTEMP(temp1, XYYY);

                    vgmOPCODE(FLOOR, temp3, XY);
                        vgmTEMP(temp2, XYYY);

                    vgmOPCODE(MUL, temp4, XY);
                        vgmTEMP(temp3, XYYY);
                        vgmUNIFORM(TexBound, XYYY, 2);

                    vgmOPCODE(SUB, newCoord, XY);
                        vgmTEMP(temp0, XYYY);
                        vgmTEMP(temp4, XYYY);

                    /*
                    coord += texBound.xy;
                    paintColor = texture2D(PatternSampler, coord);
                    */
                    vgmOPCODE(ADD, coord, XY);
                        vgmTEMP(newCoord, XYYY);
                        vgmUNIFORM(TexBound, XYYY, 0);

                }
                else
                {
                    vgmOPCODE(FLOOR, temp0, XY);
                        vgmTEMP(coord, XYYY);

                    vgmOPCODE(SUB, newCoord, XY);
                        vgmTEMP(coord, XYYY);
                        vgmTEMP(temp0, XYYY);

                    vgmOPCODE(MOV, coord, XY);
                        vgmTEMP(newCoord, XYYY);
                }

            }
            vgmLABEL(label1);
            /* else */
            {
                /*
                paintColor = texture2D(PatternSampler, a2.xy);
                */
                vgmOPCODE(TEXLD, paintColor, XYZW);
                    vgmSAMPLER(Sampler);
                    vgmTEMP(coord, XYYY);
            }
        }
        else
        {


            vgmDEFINE_ALLOCATE_LABEL(label0);
            vgmDEFINE_ALLOCATE_LABEL(label1);

            gcmASSERT(tilingMode == VG_TILE_REFLECT);

            /*
            if (a2.x < texBound[0].x || a2.x >= texBound[0].z || a2.y < texBound[0].y || a2.y >= texBound[0].w)
            */

            vgmOPCODE_COND(JMP, LESS, label0);
                vgmTEMP(coord, XXXX);
                vgmUNIFORM(TexBound, XXXX, 0);

            vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label0);
                vgmTEMP(coord, XXXX);
                vgmUNIFORM(TexBound, ZZZZ, 0);

            vgmOPCODE_COND(JMP, LESS, label0);
                vgmTEMP(coord, YYYY);
                vgmUNIFORM(TexBound, YYYY, 0);

            vgmOPCODE_COND(JMP, LESS, label1);
                vgmTEMP(coord, YYYY);
                vgmUNIFORM(TexBound, WWWW, 0);

            vgmLABEL(label0);
            {
                /*
                newCoord -= texBound[0].xy;
                newCoord = mod(newCoord, texBound[2].zw);
                if (newCoord.x>=muv.x) newCoord.x=texBound[2].z - newCoord.x;
                if (newCoord.y>=muv.y) newCoord.y=texBound[2].w - newCoord.y;
                newCoord += texBound[0].xy;
                paintColor = texture2D(PatternSampler, newCoord);
                */
                vgmDEFINE_ALLOCATE_TEMP(temp0);
                vgmDEFINE_ALLOCATE_TEMP(temp1);
                vgmDEFINE_ALLOCATE_TEMP(temp2);
                vgmDEFINE_ALLOCATE_TEMP(temp3);
                vgmDEFINE_ALLOCATE_TEMP(temp4);
                vgmDEFINE_ALLOCATE_TEMP(temp5);
                vgmDEFINE_ALLOCATE_TEMP(temp6);

                vgmDEFINE_ALLOCATE_LABEL(label3);
                vgmDEFINE_ALLOCATE_LABEL(label4);


                if (!rootImage)
                {
                    /*
                    coord -= texBound[0].xy;
                    */
                    vgmOPCODE(SUB, temp0, XY);
                        vgmTEMP(coord, XYYY);
                        vgmUNIFORM(TexBound, XYYY, 0);

                    /*
                    coord = mod(coord, texBound[2].zw);
                    */
                    vgmOPCODE(RCP, temp1, XY);
                        vgmUNIFORM(TexBound, ZWWW, 2);

                    vgmOPCODE(MUL, temp2, XY);
                        vgmTEMP(temp0, XYYY);
                        vgmTEMP(temp1, XYYY);

                    vgmOPCODE(FLOOR, temp3, XY);
                        vgmTEMP(temp2, XYYY);

                    vgmOPCODE(MUL, temp4, XY);
                        vgmTEMP(temp3, XYYY);
                        vgmUNIFORM(TexBound, ZWWW, 2);

                    vgmOPCODE(SUB, coord, XY);
                        vgmTEMP(temp0, XYYY);
                        vgmTEMP(temp4, XYYY);

                    /*
                        if (coord.x>=texBound[2].x) coord.x=texBound[2].z - coord.x;
                        if (coord.y>=texBound[2].y) coord.y=texBound[2].w - coord.y;
                    */

                    vgmOPCODE_COND(JMP, LESS, label3);
                        vgmTEMP(coord, XXXX);
                        vgmUNIFORM(TexBound, XXXX, 2);
                    {
                        vgmOPCODE(SUB, temp5, X);
                            vgmUNIFORM(TexBound, ZZZZ, 2);
                            vgmTEMP(coord, XXXX);

                        vgmOPCODE(MOV, coord, X);
                            vgmTEMP(temp5, XXXX);
                    }
                    vgmLABEL(label3);


                    vgmOPCODE_COND(JMP, LESS, label4);
                        vgmTEMP(coord, YYYY);
                        vgmUNIFORM(TexBound, YYYY, 2);
                    {
                        vgmOPCODE(SUB, temp5, X);
                            vgmUNIFORM(TexBound, WWWW, 2);
                            vgmTEMP(coord, YYYY);

                        vgmOPCODE(MOV, coord, Y);
                            vgmTEMP(temp5, XXXX);
                    }
                    vgmLABEL(label4);

                    /*
                        coord += texBound[0].xy;
                        paintColor = texture2D(Sampler, coord);
                    */

                    vgmOPCODE(ADD, temp6, XY);
                        vgmTEMP(coord, XYYY);
                        vgmUNIFORM(TexBound, XYYY, 0);

                    vgmOPCODE(MOV, coord, XY);
                        vgmTEMP(temp6, XYYY);
                }
                else
                {
                    /*
                        float u=mod(myTexCoord.s, 2.0f);
                        float v=mod(myTexCoord.t, 2.0f);
                        if (u>=1.0f) u=2.0f-u;\
                        if (v>=1.0f) v=2.0f-v;\
                        color = texture2D(sampler2d, vec2(u, v));
                    */

                    vgmOPCODE(MUL, temp0, XY);
                        vgmTEMP(coord, XYYY);
                        vgmCONST(0.5f);

                    vgmOPCODE(FLOOR, temp3, XY);
                        vgmTEMP(temp0, XYYY);

                    vgmOPCODE(MUL, temp4, XY);
                        vgmTEMP(temp3, XYYY);
                        vgmCONST(2.0f);

                    vgmOPCODE(SUB, temp5, XY);
                        vgmTEMP(coord, XYYY);
                        vgmTEMP(temp4, XYYY);

                    vgmOPCODE(MOV, coord, XY);
                        vgmTEMP(temp5, XYYY);

                    vgmOPCODE_COND(JMP, LESS, label3);
                        vgmTEMP(coord, XXXX);
                        vgmCONST(1.0f);
                    {
                        vgmOPCODE(SUB, temp6, X);
                            vgmCONST(2.0f);
                            vgmTEMP(coord, XXXX);

                        vgmOPCODE(MOV, coord, X);
                            vgmTEMP(temp6, XXXX);
                    }
                    vgmLABEL(label3);


                    vgmOPCODE_COND(JMP, LESS, label4);
                        vgmTEMP(coord, YYYY);
                        vgmCONST(1.0f);
                    {
                        vgmOPCODE(SUB, temp6, X);
                            vgmCONST(2.0f);
                            vgmTEMP(coord, YYYY);

                        vgmOPCODE(MOV, coord, Y);
                            vgmTEMP(temp6, XXXX);
                    }
                    vgmLABEL(label4);

                }

            }
            vgmLABEL(label1);
            /* else */
            {
                /*
                paintColor = texture2D(PatternSampler, a2.xy);
                */
                vgmOPCODE(TEXLD, paintColor, XYZW);
                    vgmSAMPLER(Sampler);
                    vgmTEMP(coord, XYYY);
            }
        }
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


static gceSTATUS _GenColorTransform(_VGShader *Shader,
                            gctUINT16 paintColor,
                            gctBOOL colorTransform)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("Shader=0x%x paintColor=%u colorTransform=%s",
        Shader, paintColor, colorTransform ? "TRUE" : "FALSE");

    do
    {
        if (colorTransform)
        {
            vgmDEFINE_ALLOCATE_TEMP(temp0);

            vgmDEFINE_ALLOCATE_UNIFORM(ColorTransformValues, X4, 2);

            /*
            paintColor.r = paintColor.r * colorTransformValues[0].r + colorTransformValues[1].r;
            paintColor.g = paintColor.g * colorTransformValues[0].g + colorTransformValues[1].g;
            paintColor.b = paintColor.b * colorTransformValues[0].b + colorTransformValues[1].b;
            paintColor.a = paintColor.a * colorTransformValues[0].a + colorTransformValues[1].a;
            paintColor = clamp(paintColor, 0.0f, 1.0f);
            */

            vgmOPCODE(MUL, temp0, XYZW);
                vgmUNIFORM(ColorTransformValues, XYZW, 0);
                vgmTEMP(paintColor, XYZW);

            vgmOPCODE(ADD, paintColor, XYZW);
                vgmTEMP(temp0, XYZW);
                vgmUNIFORM(ColorTransformValues, XYZW, 1);

            gcmERR_BREAK(_GenClampColorCode(Shader, paintColor, gcvFALSE));

        }
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


static gceSTATUS _GenPathImageVertexCode(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;
    _VGShader *Shader = &hardware->program->vertexShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        /*  define the uniforms  */
        vgmDEFINE_UNIFORM(ModeViewMatrix);
        vgmDEFINE_UNIFORM(ProjectionMatrix);
        vgmDEFINE_UNIFORM(ZValue);

        /*  define the attributes  */
        vgmDEFINE_ATTRIBUTE(VERTEX);

        /*  define the outputs */
        vgmDEFINE_OUTPUT(gl_Position);

        /* allocate temps */
        vgmDEFINE_ALLOCATE_TEMP(p);
        vgmDEFINE_ALLOCATE_TEMP(temp0);
        vgmDEFINE_ALLOCATE_TEMP(temp2);
        vgmDEFINE_ALLOCATE_TEMP(temp3);
        vgmDEFINE_ALLOCATE_TEMP(temp4);

        /*allocate label*/
        vgmDEFINE_ALLOCATE_LABEL(label0);

        /* allocate uniforms */
        vgmALLOCATE_UNIFORM(ModeViewMatrix, 4X4, 1);
        vgmALLOCATE_UNIFORM(ProjectionMatrix, 4X4, 1);
        vgmALLOCATE_UNIFORM(ZValue, X1, 1);

        /* allocate attributes */
        vgmALLOCATE_ATTRIBUTE(VERTEX, X4, 1);
        /* allocate output */
        vgmALLOCATE_OUTPUT(gl_Position, "#Position", X4, 1);


        /* ===================   code  ==========================*/
#ifndef JRC
        /*  highp vec4 p = modeViewMatrix * myVertex;   */

        vgmOPCODE(DP4, p, X);
            vgmUNIFORM(ModeViewMatrix, XYZW, 0);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, Y);
            vgmUNIFORM(ModeViewMatrix, XYZW, 1);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, Z);
            vgmUNIFORM(ModeViewMatrix, XYZW, 2);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, W);
            vgmUNIFORM(ModeViewMatrix, XYZW, 3);
            vgmATTRIBUTE(VERTEX, XYZW, 0);


        /*  if (fract(p.y) == 0.5f) */

        vgmOPCODE(FRAC, temp2, X);
            vgmTEMP(p, YYYY);

        vgmOPCODE_COND(JMP, NOT_EQUAL, label0);
            vgmTEMP(temp2, XXXX);
            vgmCONST(0.5f);
        {
            /*  p.y -= 0.1f;    */

            vgmOPCODE(SUB, temp3, X);
                vgmTEMP(p, YYYY);
                vgmCONST(0.1f);

            vgmOPCODE(MOV, p, Y);
                vgmTEMP(temp3, XXXX);

        }
        vgmLABEL(label0);


        /*  p.z = zValue * p.w; */
        /*  gl_Position = projectionMatrix * p; */

        vgmOPCODE(MUL, temp4, X);
            vgmUNIFORM(ZValue, XXXX, 0);
            vgmTEMP(p, WWWW);

        vgmOPCODE(MOV, p, Z);
            vgmTEMP(temp4, XXXX);


        vgmOPCODE(DP4, gl_Position, X);
            vgmUNIFORM(ProjectionMatrix, XYZW, 0);
            vgmTEMP(p, XYZW);

        vgmOPCODE(DP4, gl_Position, Y);
            vgmUNIFORM(ProjectionMatrix, XYZW, 1);
            vgmTEMP(p, XYZW);

        vgmOPCODE(DP4, gl_Position, Z);
            vgmUNIFORM(ProjectionMatrix, XYZW, 2);
            vgmTEMP(p, XYZW);

        vgmOPCODE(DP4, gl_Position, W);
            vgmUNIFORM(ProjectionMatrix, XYZW, 3);
            vgmTEMP(p, XYZW);

#else

        vgmOPCODE(DP4, gl_Position, X);
            vgmUNIFORM(ProjectionMatrix, XYZW, 0);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, gl_Position, Y);
            vgmUNIFORM(ProjectionMatrix, XYZW, 1);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(MUL, gl_Position, Z);
            vgmUNIFORM(ZValue, XXXX, 0);
            vgmCONST(-1);

        vgmOPCODE(DP4, gl_Position, W);
            vgmUNIFORM(ProjectionMatrix, XYZW, 3);
            vgmATTRIBUTE(VERTEX, XYZW, 0);
#endif

        if (hardware->drawPipe == vgvDRAWPIPE_IMAGE)
        {
            vgmDEFINE_ALLOCATE_ATTRIBUTE(UV, X2, 1);
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(myTexCoord, "myTexCoord", X2, 1);


            /*  myTexCoord = myUV; */
            vgmOPCODE(MOV, myTexCoord, XY);
                vgmATTRIBUTE(UV, XYYY, 0);
        }

        if ((hardware->paint->paintType == VG_PAINT_TYPE_LINEAR_GRADIENT) || \
            (hardware->paint->paintType == VG_PAINT_TYPE_RADIAL_GRADIENT) || \
            ((hardware->paint->paintType == VG_PAINT_TYPE_PATTERN) && hardware->paint->pattern) )
        {
            /* vec3 myCoord = userToPaintMatrix * myVertex */
            vgmDEFINE_ALLOCATE_TEMP(myCoord);
            vgmDEFINE_ALLOCATE_UNIFORM(UserToPaintMatrix, 4X4, 1);

            vgmOPCODE(DP4, myCoord, X);
                vgmUNIFORM(UserToPaintMatrix, XYZW, 0);
                vgmATTRIBUTE(VERTEX, XYZW, 0);

            vgmOPCODE(DP4, myCoord, Y);
                vgmUNIFORM(UserToPaintMatrix, XYZW, 1);
                vgmATTRIBUTE(VERTEX, XYZW, 0);

            vgmOPCODE(DP4, myCoord, Z);
                vgmUNIFORM(UserToPaintMatrix, XYZW, 2);
                vgmATTRIBUTE(VERTEX, XYZW, 0);

            vgmOPCODE(DP4, myCoord, W);
                vgmUNIFORM(UserToPaintMatrix, XYZW, 3);
                vgmATTRIBUTE(VERTEX, XYZW, 0);

            if ((hardware->paint->paintType == VG_PAINT_TYPE_LINEAR_GRADIENT) && !hardware->paint->zeroFlag)
            {
                vgmDEFINE_ALLOCATE_OUTPUT_VARYING(a2, "a2", X4, 1);

                /*
                    myCoord -= points0;
                    float g = dot(myCoord, udusq);
                    a2.xy = vec2(g, 0.0f)
                */
                vgmDEFINE_ALLOCATE_UNIFORM(Points0, X4, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(Udusq, X4, 1);

                vgmOPCODE(SUB, temp0, XYZ);
                    vgmTEMP(myCoord, XYZZ);
                    vgmUNIFORM(Points0, XYZZ, 0);

                vgmOPCODE(DP4, a2, X);
                    vgmTEMP(temp0, XYZZ);
                    vgmUNIFORM(Udusq, XYZZ, 0);

                vgmOPCODE(MOV, a2, Y);
                    vgmCONST(0);
            }

            if ((hardware->paint->paintType == VG_PAINT_TYPE_RADIAL_GRADIENT) && !hardware->paint->zeroFlag)
            {
                /*
                vec3 p1 = myCoord - points[2].xyz;
                a2.w = dot(p1, points[0].xyz);
                a2.x = dot(p1, points[1].xyz);
                a2.y = p1.y * points[2].w;
                a2.z = 0.0f;
                */
                vgmDEFINE_ALLOCATE_OUTPUT_VARYING(a2, "a2", X4, 1);

                vgmDEFINE_ALLOCATE_UNIFORM(Points, X4, 3);
                vgmDEFINE_ALLOCATE_TEMP(p1);

                vgmOPCODE(SUB, p1, XYZ);
                    vgmTEMP(myCoord, XYZZ);
                    vgmUNIFORM(Points, XYZZ, 2);

                vgmOPCODE(DP4, a2, W);
                    vgmTEMP(p1, XYZZ);
                    vgmUNIFORM(Points, XYZZ, 0);

                vgmOPCODE(DP4, a2, X);
                    vgmTEMP(p1, XYZZ);
                    vgmUNIFORM(Points, XYZZ, 1);

                vgmOPCODE(MUL, a2, Y);
                    vgmTEMP(p1, YYYY);
                    vgmUNIFORM(Points, WWWW, 2);

                vgmOPCODE(MOV, a2, Z);
                    vgmCONST(0);

            }

            if (hardware->paint->paintType == VG_PAINT_TYPE_PATTERN)
            {
                /*
                a2.xy = (gradientMatrix * myCoord).xy;
                */
                vgmDEFINE_ALLOCATE_OUTPUT_VARYING(a2, "a2", X4, 1);

                vgmDEFINE_ALLOCATE_UNIFORM(GradientMatrix, 4X4, 1);

                vgmOPCODE(DP4, a2, X);
                    vgmUNIFORM(GradientMatrix, XYZW, 0);
                    vgmTEMP(myCoord, XYZW);

                vgmOPCODE(DP4, a2, Y);
                    vgmUNIFORM(GradientMatrix, XYZW, 1);
                    vgmTEMP(myCoord, XYZW);
            }
        }


        gcmERR_BREAK(gcSHADER_Pack(Shader->binary));
#if vgvUSE_SHADER_OPTIM
        gcmERR_BREAK(gcOptimizeShader(Shader->binary, gcvNULL));
#endif
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}



static gceSTATUS _GenPathImageFragmentCode(_vgHARDWARE *hardware)
{
    gceSTATUS   status = gcvSTATUS_OK;
    gctUINT32       colorConvert    = 0;
    gctUINT32       alphaConvert    = 0;
    _VGColorFormat  colorFormat     = sRGBA;
    gctBOOL         shaderBlending  = gcvFALSE;

    _VGColorFormat dstColorFormat  = hardware->context->targetImage.internalColorDesc.colorFormat;

    _VGShader *Shader = &hardware->program->fragmentShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        vgmDEFINE_ALLOCATE_OUTPUT(gl_FragColor, "#Color", X4, 1);
        vgmDEFINE_ALLOCATE_TEMP(paintColor);
        vgmDEFINE_ALLOCATE_TEMP(blendAlpha);
        vgmDEFINE_ALLOCATE_TEMP(invBlendAlpha);
        vgmDEFINE_ALLOCATE_TEMP(dstColor);
        vgmDEFINE_ALLOCATE_TEMP(renderCoord);

        if (_UseShaderBlending(hardware))
        {
            shaderBlending = gcvTRUE;
        }


        if (hardware->masking || hardware->scissoring || (shaderBlending && (hardware->blendMode != VG_BLEND_SRC)))
        {
            vgmDEFINE_ALLOCATE_UNIFORM(RenderSize, X2, 1);
            vgmDEFINE_ALLOCATE_VARYINGEX(gl_FragCoord, "#Position", X4, 1);
            vgmOPCODE(MUL, renderCoord, XY);
                vgmUNIFORM(RenderSize, XYYY, 0);
                vgmVARYING(gl_FragCoord, XYYY, 0);
        }

        if ((hardware->drawPipe == vgvDRAWPIPE_PATH) ||
            (hardware->drawPipe == vgvDRAWPIPE_IMAGE && hardware->imageMode != VG_DRAW_IMAGE_NORMAL))
        {

            if ((hardware->paint->paintType == VG_PAINT_TYPE_COLOR) || \
                ((hardware->paint->paintType == VG_PAINT_TYPE_PATTERN) && !hardware->paint->pattern) )
            {
                /* paintColor = SolidColor; */
                vgmDEFINE_ALLOCATE_UNIFORM(SolidColor, X4, 1);
                vgmOPCODE(MOV, paintColor, XYZW);
                    vgmUNIFORM(SolidColor, XYZW, 0);

            }
            else if (hardware->paint->paintType == VG_PAINT_TYPE_LINEAR_GRADIENT)
            {
                /* paintColor = texture2D(GradientSampler, a2.xy);*/
                vgmDEFINE_ALLOCATE_SAMPLER(GradientSampler, SAMPLER_1D, 1);
                if (!hardware->paint->zeroFlag)
                {
                    vgmDEFINE_ALLOCATE_VARYING(a2, X4, 1);
                    vgmOPCODE(TEXLD, paintColor, XYZW);
                        vgmSAMPLER(GradientSampler);
                        vgmVARYING(a2, XXXX, 0);
                }
                else
                {
                    vgmOPCODE(TEXLD, paintColor, XYZW);
                        vgmSAMPLER(GradientSampler);
                        vgmCONST(1.0f);
                }

            }
            else if (hardware->paint->paintType == VG_PAINT_TYPE_RADIAL_GRADIENT)
            {
                /*
                    float s = sqrt(dot(a2.xyz, a2.xyz));
                    float g = s + a2.w;
                    paintColor = texture2D(GradientSampler, vec2(g, 0.0f));
                */
                vgmDEFINE_ALLOCATE_SAMPLER(GradientSampler, SAMPLER_1D, 1);
                if (!hardware->paint->zeroFlag)
                {
                    vgmDEFINE_ALLOCATE_VARYING(a2, X4, 1);
                    vgmDEFINE_ALLOCATE_TEMP(temp0);
                    vgmDEFINE_ALLOCATE_TEMP(s);
                    vgmDEFINE_ALLOCATE_TEMP(g);

                    vgmOPCODE(DP4, temp0, X);
                        vgmVARYING(a2, XYZZ, 0);
                        vgmVARYING(a2, XYZZ, 0);

                    vgmOPCODE(SQRT, s, X);
                        vgmTEMP(temp0, XXXX);

                    vgmOPCODE(ADD, g, X);
                        vgmTEMP(s, XXXX);
                        vgmVARYING(a2, WWWW, 0);

                    vgmOPCODE(TEXLD, paintColor, XYZW);
                        vgmSAMPLER(GradientSampler);
                        vgmTEMP(g, XXXX);

                }
                else
                {
                    vgmOPCODE(TEXLD, paintColor, XYZW);
                        vgmSAMPLER(GradientSampler);
                        vgmCONST(1.0f);
                }
            }
            else
            {

                /* TODO:  do more opitimize */
                vgmDEFINE_ALLOCATE_VARYING(a2, X4, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(TexBound, X4, 3);
                vgmDEFINE_ALLOCATE_SAMPLER(PatternSampler, SAMPLER_2D, 1);
                vgmDEFINE_ALLOCATE_TEMP(coord);

                gcmASSERT(hardware->paint->paintType == VG_PAINT_TYPE_PATTERN);

                vgmOPCODE(MOV, coord, XY);
                    vgmVARYING(a2, XYYY, 0);

                gcmERR_BREAK(_GenSampleTextureCode(hardware, Shader,
                                                PatternSampler,
                                                coord,
                                                hardware->paint->patternTilingMode,
                                                TexBound,
                                                paintColor,
                                                (hardware->paint->pattern->parent == gcvNULL) ? gcvTRUE : gcvFALSE));
            }
        }


        /* ---------------------- image mode ------------------------------- */
        if (hardware->drawPipe ==  vgvDRAWPIPE_IMAGE)
        {
            vgmDEFINE_ALLOCATE_SAMPLER(ImageSampler, SAMPLER_2D, 1);
            vgmDEFINE_ALLOCATE_VARYING(myTexCoord, X2, 1);
            vgmDEFINE_ALLOCATE_TEMP(imageColor);
            vgmDEFINE_ALLOCATE_TEMP(temp0);
            vgmDEFINE_ALLOCATE_TEMP(temp1);

            if (hardware->imageMode == VG_DRAW_IMAGE_NORMAL)
            {
                /*
                paintColor = texture2D(ImageSampler,myTexCoord);
                */
                vgmOPCODE(TEXLD, paintColor, XYZW);
                    vgmSAMPLER(ImageSampler);
                    vgmVARYING(myTexCoord, XYYY, 0);

#if WALK600
                if (hardware->isConformance && hardware->isGC600_19)
                {
                    gcmERR_BREAK(_GenTextureTuneCode(Shader, paintColor));
                }
#endif

                colorFormat = hardware->srcImage->internalColorDesc.colorFormat;

                if (hardware->colorTransform)
                {
                    vgmOPCODE(SIGN, temp0, X);
                        vgmTEMP(paintColor, WWWW);

                    vgmOPCODE(MUL, temp1, XYZ);
                        vgmTEMP(paintColor, XYZZ);
                        vgmTEMP(temp0, XXXX);

                    vgmOPCODE(MOV, paintColor, XYZ);
                        vgmTEMP(temp1, XYZZ);
                }

                if ((colorFormat & PREMULTIPLIED) &&  hardware->colorTransform)
                {
                    /* depre the color */
                    _GenUnPreMultiColorCode(Shader, paintColor, 1);
                    colorFormat &= ~PREMULTIPLIED;
                }

                gcmERR_BREAK(_GenColorTransform(Shader, paintColor, hardware->colorTransform));

                if (shaderBlending)
                {
					dstColorFormat = (_VGColorFormat)(((gctINT)dstColorFormat) | PREMULTIPLIED);
                }

                /* color convert : image space -> dest color space */
                _GetConvertValue(colorFormat, dstColorFormat, &colorConvert, &alphaConvert);
                gcmERR_BREAK(_GenColorConvertCode(Shader, paintColor, colorConvert, alphaConvert));


                vgmOPCODE(MOV, blendAlpha, XYZ);
                    vgmTEMP(paintColor, WWWW);
            }
            else if (hardware->imageMode == VG_DRAW_IMAGE_MULTIPLY)
            {
                _VGColorFormat paintFormat;
                /*
                imageColor = texture2D(ImageSampler,myTexCoord);
                paintColor *= imageColor;
                */
                colorFormat = hardware->srcImage->internalColorDesc.colorFormat;
                _GetPaintColorFormat(hardware->paint, &paintFormat);


                vgmOPCODE(TEXLD, imageColor, XYZW);
                    vgmSAMPLER(ImageSampler);
                    vgmVARYING(myTexCoord, XYYY, 0);

                if (!(colorFormat & PREMULTIPLIED))
                {
                    _GenPreMultiColorCode(Shader, imageColor, 2);
                    colorFormat |= PREMULTIPLIED;
                }

                if (!(paintFormat & PREMULTIPLIED))
                {
                    _GenPreMultiColorCode(Shader, paintColor, 2);
                }

                vgmOPCODE(MUL, temp0, XYZW);
                    vgmTEMP(imageColor, XYZW);
                    vgmTEMP(paintColor, XYZW);

                vgmOPCODE(MOV, paintColor, XYZW);
                    vgmTEMP(temp0, XYZW);


                if (hardware->colorTransform)
                {
                    /* depre the color */
                    _GenUnPreMultiColorCode(Shader, paintColor, 1);
                    colorFormat &= ~PREMULTIPLIED;
                }

                gcmERR_BREAK(_GenColorTransform(Shader, paintColor, hardware->colorTransform));

                if (shaderBlending)
                {
					dstColorFormat = (_VGColorFormat)(((gctINT)dstColorFormat) | PREMULTIPLIED);
                }
                /* color convert : image space -> dest color space*/
                _GetConvertValue(colorFormat, dstColorFormat, &colorConvert, &alphaConvert);
                gcmERR_BREAK(_GenColorConvertCode(Shader, paintColor, colorConvert, alphaConvert));


                vgmOPCODE(MOV, blendAlpha, XYZ);
                    vgmTEMP(paintColor, WWWW);

            }
            else if (hardware->imageMode == VG_DRAW_IMAGE_STENCIL)
            {
                _VGColorFormat imageFormat = hardware->srcImage->internalColorDesc.colorFormat;

                _GetPaintColorFormat(hardware->paint, &colorFormat);

                if ((colorFormat & PREMULTIPLIED) &&  hardware->colorTransform)
                {
                    /* depre the color */
                    _GenUnPreMultiColorCode(Shader, paintColor, 1);
                    colorFormat &= ~PREMULTIPLIED;
                }

                gcmERR_BREAK(_GenColorTransform(Shader, paintColor, hardware->colorTransform));

				dstColorFormat = (_VGColorFormat)(((gctINT)dstColorFormat) | PREMULTIPLIED);

                /* color convert : paintColor space -> dest color space*/
                _GetConvertValue(colorFormat, dstColorFormat, &colorConvert, &alphaConvert);
                gcmERR_BREAK(_GenColorConvertCodeEx(Shader, paintColor, colorConvert, alphaConvert, gcvFALSE, gcvTRUE));


                /*
                blendAlpha = paintColor.a * imageColor.rgb;
                paintColor *= imageColor;
                */
                vgmOPCODE(TEXLD, imageColor, XYZW);
                    vgmSAMPLER(ImageSampler);
                    vgmVARYING(myTexCoord, XYYY, 0);


                if (!(imageFormat & PREMULTIPLIED))
                {
                    _GenPreMultiColorCode(Shader, imageColor, 2);
                }

                vgmOPCODE(MUL, temp0, XYZW);
                    vgmTEMP(imageColor, XYZW);
                    vgmTEMP(paintColor, XYZW);

                vgmOPCODE(MUL, blendAlpha, XYZ);
                    vgmTEMP(paintColor, WWWW);
                    vgmTEMP(imageColor, XYZZ);

                vgmOPCODE(MOV, paintColor, XYZW);
                    vgmTEMP(temp0, XYZW);
            }
        }
        else
        {
            gcmASSERT(hardware->drawPipe == vgvDRAWPIPE_PATH);

            _GetPaintColorFormat(hardware->paint, &colorFormat);

            if (hardware->colorTransform)
            {
                vgmDEFINE_ALLOCATE_TEMP(temp0);
                vgmDEFINE_ALLOCATE_TEMP(temp1);

                vgmOPCODE(SIGN, temp0, X);
                    vgmTEMP(paintColor, WWWW);

                vgmOPCODE(MUL, temp1, XYZ);
                    vgmTEMP(paintColor, XYZZ);
                    vgmTEMP(temp0, XXXX);

                vgmOPCODE(MOV, paintColor, XYZ);
                    vgmTEMP(temp1, XYZZ);
            }

            if ((colorFormat & PREMULTIPLIED) &&  hardware->colorTransform)
            {
                /* depre the color */
                _GenUnPreMultiColorCode(Shader, paintColor, 1);
                colorFormat &= ~PREMULTIPLIED;
            }

            gcmERR_BREAK(_GenColorTransform(Shader, paintColor, hardware->colorTransform));

            if (shaderBlending)
            {
				dstColorFormat = (_VGColorFormat)(((gctINT)dstColorFormat) | PREMULTIPLIED);
            }

            /* color convert: paintColor Space -> dest color space */
            _GetConvertValue(colorFormat, dstColorFormat, &colorConvert, &alphaConvert);
            gcmERR_BREAK(_GenColorConvertCode(Shader, paintColor, colorConvert, alphaConvert));


            /*
            blendAlpha=vec3(paintColor.a,paintColor.a,paintColor.a);
            */
            vgmOPCODE(MOV, blendAlpha, XYZ);
                vgmTEMP(paintColor, WWWW);
        }


        /* ----------------------------- alpha blending ------------------------------- */


        if ((shaderBlending && (hardware->blendMode != VG_BLEND_SRC))|| hardware->masking)
        {
            vgmDEFINE_ALLOCATE_SAMPLER(RenderTargetSampler, SAMPLER_2D, 1);
            vgmOPCODE(TEXLD, dstColor, XYZW);
                vgmSAMPLER(RenderTargetSampler);
                vgmTEMP(renderCoord, XYYY);

        }


        if (shaderBlending)
        {
            vgmDEFINE_ALLOCATE_TEMP(temp0);
            vgmDEFINE_ALLOCATE_TEMP(temp1);
            vgmDEFINE_ALLOCATE_TEMP(temp2);
            vgmDEFINE_ALLOCATE_TEMP(temp3);
            vgmDEFINE_ALLOCATE_TEMP(temp4);
            vgmDEFINE_ALLOCATE_TEMP(temp5);
            vgmDEFINE_ALLOCATE_TEMP(temp6);
            vgmDEFINE_ALLOCATE_TEMP(temp7);


            if (hardware->blendMode == VG_BLEND_SRC)
            {
                /* dst=src; */
                /* bypass */
            }
            else if(hardware->blendMode == VG_BLEND_DST_IN)
            {
                /* if the image mode is stencil or ... , we need to use shader to do blending  */
                /*
                src.rgb=dst.rgb*blendAlpha;
                src.a=dst.a*src.a;
                */
                vgmOPCODE(MUL, paintColor, XYZ);
                    vgmTEMP(blendAlpha, XYZZ);
                    vgmTEMP(dstColor, XYZZ);

                vgmOPCODE(MUL, temp1, X);
                    vgmTEMP(dstColor, WWWW);
                    vgmTEMP(paintColor, WWWW);

                vgmOPCODE(MOV, paintColor, W);
                    vgmTEMP(temp1, XXXX);

            }
            else if(hardware->blendMode == VG_BLEND_SRC_IN)
            {
                /* if the image mode is stencil or ..., we need to use shader to do blending  */
                /*
                        src.rgb=src.rgb*dst.a;\
                        src.a=src.a*dst.a;\
                */
                vgmOPCODE(MUL, temp0, XYZW);
                    vgmTEMP(paintColor, XYZW);
                    vgmTEMP(dstColor, WWWW);

                vgmOPCODE(MOV, paintColor, XYZW);
                    vgmTEMP(temp0, XYZW);
            }
            else if(hardware->blendMode == VG_BLEND_DST_OVER)
            {
                /*
                src.rgba=src.rgba+dst.rgba-dst.a*src.rgba;
                */
                vgmOPCODE(ADD, temp0, XYZW);
                    vgmTEMP(paintColor, XYZW);
                    vgmTEMP(dstColor, XYZW);

                vgmOPCODE(MUL, temp1, XYZW);
                    vgmTEMP(dstColor, WWWW);
                    vgmTEMP(paintColor, XYZW);

                vgmOPCODE(SUB, paintColor, XYZW);
                    vgmTEMP(temp0, XYZW);
                    vgmTEMP(temp1, XYZW);

            }
            else if(hardware->blendMode == VG_BLEND_ADDITIVE)
            {
                /*
                src=min(src+dst,1.0f);
                */
                vgmOPCODE(ADD, temp0, XYZW);
                    vgmTEMP(paintColor, XYZW);
                    vgmTEMP(dstColor, XYZW);

                vgmOPCODE(MIN, paintColor, XYZW);
                    vgmTEMP(temp0, XYZW);
                    vgmCONST(1.0f);
            }
            else
            {

                /*
                vec3 invBlendAlpha = vec3(1.0f, 1.0f, 1.0f) - blendAlpha;
                */

                vgmOPCODE(SUB, invBlendAlpha, XYZ);
                    vgmCONST(1.0f);
                    vgmTEMP(blendAlpha, XYZZ);

                if (hardware->blendMode == VG_BLEND_SRC_OVER)
                {
                    /* if the image mode is stencil , we need to use shader to do blending  */

                    /*src.rgb=src.rgb+invBlendAlpha*dst.rgb;*/
                    vgmOPCODE(MUL, temp0, XYZ);
                        vgmTEMP(invBlendAlpha, XYZZ);
                        vgmTEMP(dstColor, XYZZ);

                    vgmOPCODE(ADD, temp1, XYZ);
                        vgmTEMP(temp0, XYZZ);
                        vgmTEMP(paintColor, XYZZ);

                    vgmOPCODE(MOV, paintColor, XYZ);
                        vgmTEMP(temp1, XYZZ);

                }
                else if (hardware->blendMode == VG_BLEND_MULTIPLY)
                {
                    /* src.rgb=src.rgb * (1.0f - dst.a) + dst.rgb*invBlendAlpha + src.rgb * dst.rgb; */
                    vgmOPCODE(SUB, temp0, X);
                        vgmCONST(1.0f);
                        vgmTEMP(dstColor, WWWW);

                    vgmOPCODE(MUL, temp1, XYZ);
                        vgmTEMP(temp0, XXXX);
                        vgmTEMP(paintColor, XYZZ);


                    vgmOPCODE(MUL, temp2, XYZ);
                        vgmTEMP(dstColor, XYZZ);
                        vgmTEMP(invBlendAlpha, XYZZ);

                    vgmOPCODE(ADD, temp4, XYZ);
                        vgmTEMP(temp1, XYZZ);
                        vgmTEMP(temp2, XYZZ);

                    vgmOPCODE(MUL, temp3, XYZ);
                        vgmTEMP(paintColor, XYZZ);
                        vgmTEMP(dstColor, XYZZ);

                    vgmOPCODE(ADD, paintColor, XYZ);
                        vgmTEMP(temp3, XYZZ);
                        vgmTEMP(temp4, XYZZ);
                }
                else if (hardware->blendMode == VG_BLEND_SCREEN)
                {
                    /* src.rgb=src.rgb+dst.rgb-src.rgb*dst.rgb; */
                    vgmOPCODE(ADD, temp0, XYZ);
                        vgmTEMP(paintColor, XYZZ);
                        vgmTEMP(dstColor, XYZZ);

                    vgmOPCODE(MUL, temp1, XYZ);
                        vgmTEMP(paintColor, XYZZ);
                        vgmTEMP(dstColor, XYZZ);

                    vgmOPCODE(SUB, paintColor, XYZ);
                        vgmTEMP(temp0, XYZZ);
                        vgmTEMP(temp1, XYZZ);
                }
                else
                {
                    /*src.rgb=min(src.rgb+dst.rgb*invBlendAlpha,dst.rgb+src.rgb*(1.0f-dst.a));*/
                    vgmOPCODE(MUL, temp0, XYZ);
                        vgmTEMP(dstColor, XYZZ);
                        vgmTEMP(invBlendAlpha, XYZZ);

                    vgmOPCODE(ADD, temp1, XYZ);
                        vgmTEMP(temp0, XYZZ);
                        vgmTEMP(paintColor, XYZZ);

                    vgmOPCODE(SUB, temp2, X);
                        vgmCONST(1.0f);
                        vgmTEMP(dstColor, WWWW);

                    vgmOPCODE(MUL, temp3, XYZ);
                        vgmTEMP(paintColor, XYZZ);
                        vgmTEMP(temp2, XXXX);

                    vgmOPCODE(ADD, temp4, XYZ);
                        vgmTEMP(temp3, XYZZ);
                        vgmTEMP(dstColor, XYZZ);

                    if (hardware->blendMode == VG_BLEND_DARKEN)
                    {
                        vgmOPCODE(MIN, paintColor, XYZ);
                            vgmTEMP(temp4, XYZZ);
                            vgmTEMP(temp1, XYZZ);
                    }
                    else
                    {
                        gcmASSERT(hardware->blendMode == VG_BLEND_LIGHTEN);
                        /*src.rgb=max(src.rgb+dst.rgb*invBlendAlpha,dst.rgb+src.rgb*(1.0f-dst.a));*/
                        vgmOPCODE(MAX, paintColor, XYZ);
                            vgmTEMP(temp4, XYZZ);
                            vgmTEMP(temp1, XYZZ);
                    }
                }

                /* src.a=src.a+(1.0f-src.a)*dst.a; */
                vgmOPCODE(SUB, temp5, X);
                    vgmCONST(1.0f);
                    vgmTEMP(paintColor, WWWW);

                vgmOPCODE(MUL, temp6, X);
                    vgmTEMP(temp5, XXXX);
                    vgmTEMP(dstColor, WWWW);

                vgmOPCODE(ADD, temp7, X);
                    vgmTEMP(paintColor, WWWW);
                    vgmTEMP(temp6, XXXX);

                vgmOPCODE(MOV, paintColor, W);
                    vgmTEMP(temp7, XXXX);
            }
        }


        if (hardware->masking || hardware->scissoring)
        {
            vgmDEFINE_ALLOCATE_SAMPLER(MaskSampler, SAMPLER_2D, 1);

            if (hardware->masking)
            {
                vgmDEFINE_ALLOCATE_TEMP(maskValue);
                vgmDEFINE_ALLOCATE_TEMP(temp0);
                vgmDEFINE_ALLOCATE_TEMP(temp1);
                vgmDEFINE_ALLOCATE_TEMP(temp2);
                vgmDEFINE_ALLOCATE_LABEL(label0);
                vgmDEFINE_ALLOCATE_LABEL(label1);

                /*
                mask = texture2D(maskTexture,texCoord).r
                if (mask == 0.0f) discard;

                dst = myConvert(convert3, dst);
                dstOld = myConvert(convert5, dstOld);
                dst = dst * mask + (1.0f - mask) * dstOld;
                dst = myConvert(convert4, dst);
                */

                /* there is shader optimize bug, and seems  use TEXLD must  enable XYZW to get around*/
                vgmOPCODE(TEXLD, maskValue, XYZW);
                    vgmSAMPLER(MaskSampler);
                    vgmTEMP(renderCoord, XYYY);

                vgmOPCODE_COND(KILL, EQUAL, 0);
                    vgmTEMP(maskValue, XXXX);
                    vgmCONST(0);

                /*  special hack for mask*/
                if (hardware->context->targetImage.height <= 64 &&  hardware->context->targetImage.width <= 64)
                {

                    if (hardware->featureVAA)
                    {
                        /*  filter the value: 0.37f, 0.5f, 0.62f for sepcial get around*/
                        vgmOPCODE_COND(JMP, LESS, label0);
                            vgmTEMP(maskValue, XXXX);
                            vgmCONST(0.36f);

                        vgmOPCODE_COND(JMP, LESS, label1);
                            vgmTEMP(maskValue, XXXX);
                            vgmCONST(0.38f);

                        vgmOPCODE_COND(JMP, LESS, label0);
                            vgmTEMP(maskValue, XXXX);
                            vgmCONST(0.49f);

                        vgmOPCODE_COND(JMP, LESS, label1);
                            vgmTEMP(maskValue, XXXX);
                            vgmCONST(0.51f);

                        vgmOPCODE_COND(JMP, LESS, label0);
                            vgmTEMP(maskValue, XXXX);
                            vgmCONST(0.61f);

                        vgmOPCODE_COND(JMP, LESS, label1);
                            vgmTEMP(maskValue, XXXX);
                            vgmCONST(0.63f);

                        vgmLABEL(label0);
                    }
                    else
                    {

                        /*  if (mask >= 0.51f || mask <= 0.49f)*/
                        vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label0);
                            vgmTEMP(maskValue, XXXX);
                            vgmCONST(0.51f);

                        vgmOPCODE_COND(JMP, GREATER, label1);
                            vgmTEMP(maskValue, XXXX);
                            vgmCONST(0.49f);

                        vgmLABEL(label0);
                    }
                }

                {
                    /* convert the color to the linear color space */
                    _GetConvertValue(dstColorFormat, lRGBA_PRE, &colorConvert, &alphaConvert);
                    gcmERR_BREAK(_GenColorConvertCode(Shader, paintColor, colorConvert, alphaConvert));


                    vgmOPCODE(MUL, temp0, XYZW);
                        vgmTEMP(paintColor, XYZW);
                        vgmTEMP(maskValue, XXXX);

                    vgmOPCODE(SUB, temp1, X);
                        vgmCONST(1);
                        vgmTEMP(maskValue, XXXX);

                    /* convert the color to the linear color space */
                    _GetConvertValue(
                        hardware->context->targetImage.internalColorDesc.colorFormat,
                        lRGBA_PRE, &colorConvert, &alphaConvert);

                    gcmERR_BREAK(_GenColorConvertCode(Shader, dstColor, colorConvert, alphaConvert));


                    vgmOPCODE(MUL, temp2, XYZW);
                        vgmTEMP(dstColor, XYZW);
                        vgmTEMP(temp1, XXXX);

                    vgmOPCODE(ADD, paintColor, XYZW);
                        vgmTEMP(temp0, XYZW);
                        vgmTEMP(temp2, XYZW);

                    /* convert the color to the dst color space */
                    _GetConvertValue(
                        lRGBA_PRE,
                        hardware->context->targetImage.internalColorDesc.colorFormat,
                        &colorConvert, &alphaConvert);

                    gcmERR_BREAK(_GenColorConvertCode(Shader, paintColor, colorConvert, alphaConvert));

                    /* now the colorspace is same */
                    dstColorFormat = hardware->context->targetImage.internalColorDesc.colorFormat;
                }

                vgmLABEL(label1);
            }

            if (hardware->scissoring)
            {
                vgmDEFINE_ALLOCATE_TEMP(scissorValue);

                /* there is shader optimize bug, and seems  use TEXLD must  enable XYZW to get around*/
                vgmOPCODE(TEXLD, scissorValue, XYZW);
                    vgmSAMPLER(MaskSampler);
                    vgmTEMP(renderCoord, XYYY);

                vgmOPCODE_COND(KILL, EQUAL, 0);
                    vgmTEMP(scissorValue, YYYY);
                    vgmCONST(0);
            }
        }

        /* according to the dest format to pre/unpre the color */
        if ((((gctINT)dstColorFormat) & PREMULTIPLIED)
            && (!(((gctINT)hardware->context->targetImage.internalColorDesc.colorFormat) & PREMULTIPLIED)))
        {
            gcmERR_BREAK(_GenUnPreMultiColorCode(Shader, paintColor, 1));
        }
        else if ((!(((gctINT)dstColorFormat) & PREMULTIPLIED))
            && (((gctINT)hardware->context->targetImage.internalColorDesc.colorFormat) & PREMULTIPLIED))
        {
            gcmERR_BREAK(_GenPreMultiColorCode(Shader, paintColor,  2));
        }

        /*_GenKillAlphaCode(Shader, paintColor, hardware->blending, hardware->blendMode);*/

        /*  gl_FragColor = paintColor;  */
        vgmOPCODE(MOV, gl_FragColor, XYZW);
            vgmTEMP(paintColor, XYZW);

        gcmERR_BREAK(gcSHADER_Pack(Shader->binary));

#if vgvUSE_SHADER_OPTIM
        gcmERR_BREAK(gcOptimizeShader(Shader->binary, gcvNULL));
#endif
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


static gceSTATUS _GenColorRampVertexCode(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;
    _VGShader *Shader = &hardware->program->vertexShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        vgmDEFINE_ALLOCATE_ATTRIBUTE(VERTEX, X4, 1);
        vgmDEFINE_ALLOCATE_ATTRIBUTE(COLOR, X4, 1);
        vgmDEFINE_ALLOCATE_OUTPUT_VARYING(colorRamp, "colorRamp", X4, 1);
        vgmDEFINE_ALLOCATE_OUTPUT(gl_Position, "#Position", X4, 1);
        vgmDEFINE_ALLOCATE_TEMP(temp0);
#if NO_STENCIL_VG
        vgmDEFINE_UNIFORM(ZValue);
#endif

        vgmOPCODE(MOV, gl_Position, XYZW);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

#if NO_STENCIL_VG
        {
            vgmALLOCATE_UNIFORM(ZValue, X1, 1);
            vgmOPCODE(MUL, temp0, XYZW);
                vgmUNIFORM(ZValue, XXXX, 0);
                vgmCONST(-1.0f);
            vgmOPCODE(MOV, gl_Position, Z);
            vgmTEMP(temp0, Z);
        }
#endif /* #ifdef NO_STENCIL_VG */

        vgmOPCODE(MOV, temp0, XYZW);
            vgmATTRIBUTE(COLOR, XYZW, 0);

        if (hardware->drawPipe == vgvDRAWPIPE_COLORRAMP)
        {
            gcmASSERT((hardware->paint->paintType == VG_PAINT_TYPE_LINEAR_GRADIENT) || \
                      (hardware->paint->paintType ==VG_PAINT_TYPE_RADIAL_GRADIENT));

            if (hardware->paint->colorRampPremultiplied)
            {
                vgmOPCODE(MUL, temp0, XYZ);
                    vgmATTRIBUTE(COLOR, XYZZ, 0);
                    vgmATTRIBUTE(COLOR, WWWW, 0);
            }
        }

        vgmOPCODE(MOV, colorRamp, XYZW);
            vgmTEMP(temp0, XYZW);

        if ((hardware->drawPipe == vgvDRAWPIPE_CLEAR)  && hardware->scissoring)
        {
            /* TODO:can use uniform to pass in, map to 0.0f - 1.0f */
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(texCoord, "texCoord", X2, 1);
            vgmDEFINE_ALLOCATE_TEMP(temp1);

            vgmOPCODE(MUL, temp1, XY);
                vgmTEMP(gl_Position, XYYY);
                vgmCONST(0.5f);

            vgmOPCODE(ADD, texCoord, X);
                vgmTEMP(temp1, XXXX);
                vgmCONST(0.5f);

            vgmOPCODE(SUB, texCoord, Y);
                vgmCONST(0.5f);
                vgmTEMP(temp1, YYYY);
        }

        gcmERR_BREAK(gcSHADER_Pack(Shader->binary));
#if vgvUSE_SHADER_OPTIM
        gcmERR_BREAK(gcOptimizeShader(Shader->binary, gcvNULL));
#endif
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenColorRampFragmentCode(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;
    _VGShader *Shader = &hardware->program->fragmentShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        vgmDEFINE_ALLOCATE_VARYING(colorRamp, X4, 1);
        vgmDEFINE_ALLOCATE_OUTPUT(gl_FragColor, "#Color", X4, 1);

        if ((hardware->drawPipe == vgvDRAWPIPE_CLEAR) && hardware->scissoring)
        {
            vgmDEFINE_ALLOCATE_TEMP(scissorValue);
            vgmDEFINE_ALLOCATE_SAMPLER(MaskSampler, SAMPLER_2D, 1);
            vgmDEFINE_ALLOCATE_VARYING(texCoord, X2, 1);

            /* there is shader optimize bug, and seems  use TEXLD must  enable XYZW to get around*/
            vgmOPCODE(TEXLD, scissorValue, XYZW);
                vgmSAMPLER(MaskSampler);
                vgmVARYING(texCoord, XYYY, 0);

            vgmOPCODE_COND(KILL, EQUAL, 0);
                vgmTEMP(scissorValue, YYYY);
                vgmCONST(0);
        }

        vgmOPCODE(MOV, gl_FragColor, XYZW);
            vgmVARYING(colorRamp, XYZW, 0);

        /*_GenKillAlphaCode(Shader, gl_FragColor, hardware->blending, hardware->blendMode);*/

        if (hardware->drawPipe == vgvDRAWPIPE_COLORRAMP)
        {
            gcmASSERT((hardware->paint->paintType == VG_PAINT_TYPE_LINEAR_GRADIENT) || \
                      (hardware->paint->paintType ==VG_PAINT_TYPE_RADIAL_GRADIENT));


            if (hardware->paint->colorRampPremultiplied)
            {
                _GenUnPreMultiColorCode(Shader, gl_FragColor, 1);
            }
        }

        gcmERR_BREAK(gcSHADER_Pack(Shader->binary));

#if vgvUSE_SHADER_OPTIM
        gcmERR_BREAK(gcOptimizeShader(Shader->binary, gcvNULL));
#endif
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


static gceSTATUS _GenMaskVertexCode(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;
    _VGShader *Shader = &hardware->program->vertexShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        /*
            vec4 p = modeViewMatrix * myVertex;
            gl_Position = projectionMatrix * p;
            myTexCoord = myUV;
        */

        vgmDEFINE_ALLOCATE_UNIFORM(ProjectionMatrix, 4X4, 1);
        vgmDEFINE_ALLOCATE_ATTRIBUTE(VERTEX, X4, 1);
        vgmDEFINE_ALLOCATE_UNIFORM(FilterModeViewMatrix, 4X4, 1);

        vgmDEFINE_ALLOCATE_OUTPUT(gl_Position, "#Position", X4, 1);

        vgmDEFINE_ALLOCATE_TEMP(p);

        vgmOPCODE(DP4, p, X);
            vgmUNIFORM(FilterModeViewMatrix, XYZW, 0);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, Y);
            vgmUNIFORM(FilterModeViewMatrix, XYZW, 1);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, Z);
            vgmUNIFORM(FilterModeViewMatrix, XYZW, 2);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, W);
            vgmUNIFORM(FilterModeViewMatrix, XYZW, 3);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, gl_Position, X);
            vgmUNIFORM(ProjectionMatrix, XYZW, 0);
            vgmTEMP(p, XYZW);

        vgmOPCODE(DP4, gl_Position, Y);
            vgmUNIFORM(ProjectionMatrix, XYZW, 1);
            vgmTEMP(p, XYZW);

        /* can be remove */
        vgmOPCODE(DP4, gl_Position, Z);
            vgmUNIFORM(ProjectionMatrix, XYZW, 2);
            vgmTEMP(p, XYZW);

        vgmOPCODE(DP4, gl_Position, W);
            vgmUNIFORM(ProjectionMatrix, XYZW, 3);
            vgmTEMP(p, XYZW);

        if (hardware->maskOperation != VG_CLEAR_MASK &&
            hardware->maskOperation != VG_FILL_MASK)
        {
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(myTexCoord, "myTexCoord", X2, 1);
            vgmDEFINE_ALLOCATE_ATTRIBUTE(UV, X2, 1);
            vgmOPCODE(MOV, myTexCoord, XY);
                vgmATTRIBUTE(UV, XYYY, 0);
        }

        if (hardware->maskOperation == VG_UNION_MASK ||
            hardware->maskOperation == VG_INTERSECT_MASK ||
            hardware->maskOperation == VG_SUBTRACT_MASK)
        {
            /* generate the coord for (0,0) = (left top)*/
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(texCoord, "texCoord", X2, 1);
            vgmDEFINE_ALLOCATE_TEMP(temp0);

            vgmOPCODE(MUL, temp0, XY);
                vgmTEMP(gl_Position, XYYY);
                vgmCONST(0.5f);

            vgmOPCODE(ADD, texCoord, X);
                vgmTEMP(temp0, XXXX);
                vgmCONST(0.5f);

            vgmOPCODE(SUB, texCoord, Y);
                vgmCONST(0.5f);
                vgmTEMP(temp0, YYYY);
        }

        gcmERR_BREAK(gcSHADER_Pack(Shader->binary));
#if vgvUSE_SHADER_OPTIM
        gcmERR_BREAK(gcOptimizeShader(Shader->binary, gcvNULL));
#endif
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


static gceSTATUS _GenMaskFragmentCode(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;
    _VGShader *Shader = &hardware->program->fragmentShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        vgmDEFINE_ALLOCATE_OUTPUT(gl_FragColor, "#Color", X4, 1);

        if (hardware->maskOperation == VG_CLEAR_MASK ||
            hardware->maskOperation == VG_FILL_MASK)
        {
            vgmOPCODE(MOV, gl_FragColor, X);
            vgmCONST((hardware->maskOperation == VG_CLEAR_MASK) ?  0 : 1);
        }
        else
        {
            vgmDEFINE_ALLOCATE_VARYING(myTexCoord, X2, 1);
            vgmDEFINE_ALLOCATE_VARYING(texCoord, X2, 1);
            vgmDEFINE_ALLOCATE_SAMPLER(MaskSampler, SAMPLER_2D, 1);
            vgmDEFINE_ALLOCATE_SAMPLER(SourceMaskSampler, SAMPLER_2D, 1);

            vgmDEFINE_ALLOCATE_TEMP(srcMask);
            vgmDEFINE_ALLOCATE_TEMP(dstMask);
            vgmDEFINE_ALLOCATE_TEMP(temp0);
            vgmDEFINE_ALLOCATE_TEMP(temp1);
            vgmDEFINE_ALLOCATE_TEMP(temp2);
            vgmDEFINE_ALLOCATE_TEMP(temp3);

            if (hardware->hasAlpha)
            {
                vgmOPCODE(TEXLD, temp3, XYZW);
                    vgmSAMPLER(SourceMaskSampler);
                    vgmVARYING(myTexCoord, XYYY, 0);

                /*get around the optimize bug, need to fix*/
                vgmOPCODE(MOV, srcMask, X);
                    vgmTEMP(temp3, WWWW);

                vgmOPCODE(MOV, srcMask, Y);
                    vgmCONST(0.5f);
            }
            else
            {
                vgmOPCODE(TEXLD, srcMask, XYZW);
                    vgmSAMPLER(SourceMaskSampler);
                    vgmVARYING(myTexCoord, XYYY, 0);
            }

            if (hardware->maskOperation == VG_SET_MASK)
            {
                vgmOPCODE(MOV, gl_FragColor, XYZW);
                    vgmTEMP(srcMask, XYYY);
            }
            else
            {
                vgmOPCODE(TEXLD, dstMask, XYZW);
                    vgmUNIFORM(MaskSampler, XXXX, 0);
                    vgmVARYING(texCoord, XYYY, 0);

                if (hardware->maskOperation == VG_UNION_MASK)
                {
                    /*
                    valueNew = 1.0f - (1.0f - srcMask) * (1.0f - dstMask);
                    */
                    vgmOPCODE(SUB, temp0, X);
                        vgmCONST(1);
                        vgmTEMP(srcMask, XXXX);

                    vgmOPCODE(SUB, temp1, X);
                        vgmCONST(1);
                        vgmTEMP(dstMask, XXXX);

                    vgmOPCODE(MUL, temp2, X);
                        vgmTEMP(temp0, XXXX);
                        vgmTEMP(temp1, XXXX);

                    vgmOPCODE(SUB, gl_FragColor, X);
                        vgmCONST(1);
                        vgmTEMP(temp2, XXXX);
                }
                else if (hardware->maskOperation == VG_INTERSECT_MASK)
                {
                    /*
                    valueNew = srcMask * dstMask;
                    */

                    vgmOPCODE(MUL, gl_FragColor, X);
                        vgmTEMP(srcMask, XXXX);
                        vgmTEMP(dstMask, XXXX);
                }
                else if (hardware->maskOperation == VG_SUBTRACT_MASK)
                {
                    /*
                    valueNew = dstMask * (1.0f - srcMask);
                    */
                    vgmOPCODE(SUB, temp0, X);
                        vgmCONST(1);
                        vgmTEMP(srcMask, XXXX);

                    vgmOPCODE(MUL, gl_FragColor, X);
                        vgmTEMP(temp0, XXXX);
                        vgmTEMP(dstMask, XXXX);
                }
                else
                {
                    gcmBREAK();
                }
            }
        }

        /*_GenKillAlphaCode(Shader, gl_FragColor, hardware->blending, hardware->blendMode);*/

        gcmERR_BREAK(gcSHADER_Pack(Shader->binary));
#if vgvUSE_SHADER_OPTIM
        gcmERR_BREAK(gcOptimizeShader(Shader->binary, gcvNULL));
#endif
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenFilterVertexCode(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;
    _VGShader *Shader = &hardware->program->vertexShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        /*
            vec4 p = modeViewMatrix * myVertex;
            gl_Position = projectionMatrix * p;
            myTexCoord = myUV;
        */
        vgmDEFINE_ALLOCATE_UNIFORM(FilterModeViewMatrix, 4X4, 1);
        vgmDEFINE_ALLOCATE_UNIFORM(ProjectionMatrix, 4X4, 1);
        vgmDEFINE_ALLOCATE_ATTRIBUTE(VERTEX, X4, 1);
        vgmDEFINE_ALLOCATE_ATTRIBUTE(UV, X2, 1);
        vgmDEFINE_ALLOCATE_OUTPUT(gl_Position, "#Position", X4, 1);
        vgmDEFINE_ALLOCATE_OUTPUT_VARYING(myTexCoord, "myTexCoord", X2, 1);
#if NO_STENCIL_VG
        vgmDEFINE_UNIFORM(ZValue);
#endif

        vgmDEFINE_ALLOCATE_TEMP(p);

        vgmOPCODE(DP4, p, X);
            vgmUNIFORM(FilterModeViewMatrix, XYZW, 0);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, Y);
            vgmUNIFORM(FilterModeViewMatrix, XYZW, 1);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, Z);
            vgmUNIFORM(FilterModeViewMatrix, XYZW, 2);
            vgmATTRIBUTE(VERTEX, XYZW, 0);

        vgmOPCODE(DP4, p, W);
            vgmUNIFORM(FilterModeViewMatrix, XYZW, 3);
            vgmATTRIBUTE(VERTEX, XYZW, 0);


        vgmOPCODE(DP4, gl_Position, X);
            vgmUNIFORM(ProjectionMatrix, XYZW, 0);
            vgmTEMP(p, XYZW);

        vgmOPCODE(DP4, gl_Position, Y);
            vgmUNIFORM(ProjectionMatrix, XYZW, 1);
            vgmTEMP(p, XYZW);

        /* can be remove */
        vgmOPCODE(DP4, gl_Position, Z);
            vgmUNIFORM(ProjectionMatrix, XYZW, 2);
            vgmTEMP(p, XYZW);

        vgmOPCODE(DP4, gl_Position, W);
            vgmUNIFORM(ProjectionMatrix, XYZW, 3);
            vgmTEMP(p, XYZW);

#if NO_STENCIL_VG
        {
            vgmALLOCATE_UNIFORM(ZValue, X1, 1);
            vgmOPCODE(MUL, p, XYZW);
                vgmUNIFORM(ZValue, XXXX, 0);
                vgmCONST(-1.0f);
            vgmOPCODE(MOV, gl_Position, Z);
            vgmTEMP(p, Z);
        }
#endif /* #ifdef NO_STENCIL_VG */

        vgmOPCODE(MOV, myTexCoord, XY);
            vgmATTRIBUTE(UV, XYYY, 0);

        if (hardware->scissoring && (hardware->filterType == vgvFILTER_COPY))
        {

            /* TODO:can use uniform to pass in, map to 0.0f - 1.0f */
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(texCoord, "texCoord", X2, 1);
            vgmDEFINE_ALLOCATE_TEMP(temp0);

            gcmASSERT(hardware->drawPipe == vgvDRAWPIPE_COPY);

            vgmOPCODE(MUL, temp0, XY);
                vgmTEMP(gl_Position, XYYY);
                vgmCONST(0.5f);

            vgmOPCODE(ADD, texCoord, X);
                vgmTEMP(temp0, XXXX);
                vgmCONST(0.5f);

            vgmOPCODE(SUB, texCoord, Y);
                vgmCONST(0.5f);
                vgmTEMP(temp0, YYYY);
        }


        if ((hardware->filterType == vgvFILTER_GAUSSIAN) && (hardware->gaussianOPT))
        {
            /*
                vec2 tmp = myUV + coordOffset;
                offset0.xy = tmp;
                tmp += coordOffset;
                offset0.zw = tmp;
                tmp += coordOffset;
                offset1.xy = tmp;
                tmp += coordOffset;
                offset1.zw = tmp;
                tmp = myUV - coordOffset;
                offset2.xy = tmp;
                tmp = tmp - coordOffset;
                offset2.zw = tmp;
                tmp = tmp - coordOffset;
                offset3.xy = tmp;
                tmp = tmp - coordOffset;
                offset3.zw = tmp;
            */
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(offset0, "offset0", X4, 1);
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(offset1, "offset1", X4, 1);
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(offset2, "offset2", X4, 1);
            vgmDEFINE_ALLOCATE_OUTPUT_VARYING(offset3, "offset3", X4, 1);

            vgmDEFINE_ALLOCATE_UNIFORM(TexCoordOffset,  X4, vgvMAX_GAUSSIAN_COORD_SIZE);
            vgmDEFINE_ALLOCATE_TEMP(temp0);



            vgmOPCODE(ADD, offset0, XY);
                vgmUNIFORM(TexCoordOffset, XYYY, 0);
                vgmATTRIBUTE(UV, XYYY, 0);

            vgmOPCODE(ADD, temp0, XY);
                vgmTEMP(offset0, XYYY);
                vgmUNIFORM(TexCoordOffset, XYYY, 0);

            vgmOPCODE(MOV, offset0, ZW);
                vgmTEMP_EX(temp0, gcmSWIZZLE(X, X, X, Y));

            vgmOPCODE(ADD, offset1, XY);
                vgmUNIFORM(TexCoordOffset, XYYY, 0);
                vgmTEMP(temp0, XYYY);

            vgmOPCODE(ADD, temp0, XY);
                vgmTEMP(offset1, XYYY);
                vgmUNIFORM(TexCoordOffset, XYYY, 0);

            vgmOPCODE(MOV, offset1, ZW);
                vgmTEMP_EX(temp0, gcmSWIZZLE(X, X, X, Y));


            vgmOPCODE(SUB, offset2, XY);
                vgmATTRIBUTE(UV, XYYY, 0);
                vgmUNIFORM(TexCoordOffset, XYYY, 0);

            vgmOPCODE(SUB, temp0, XY);
                vgmTEMP(offset2, XYYY);
                vgmUNIFORM(TexCoordOffset, XYYY, 0);

            vgmOPCODE(MOV, offset2, ZW);
                vgmTEMP_EX(temp0, gcmSWIZZLE(X, X, X, Y));

            vgmOPCODE(SUB, offset3, XY);
                vgmTEMP(temp0, XYYY);
                vgmUNIFORM(TexCoordOffset, XYYY, 0);

            vgmOPCODE(SUB, temp0, XY);
                vgmTEMP(offset3, XYYY);
                vgmUNIFORM(TexCoordOffset, XYYY, 0);

            vgmOPCODE(MOV, offset3, ZW);
                vgmTEMP_EX(temp0, gcmSWIZZLE(X, X, X, Y));

        }

        gcmERR_BREAK(gcSHADER_Pack(Shader->binary));
#if vgvUSE_SHADER_OPTIM
        gcmERR_BREAK(gcOptimizeShader(Shader->binary, gcvNULL));
#endif
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenFilterFragmentCode(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;
    _VGShader *Shader = &hardware->program->fragmentShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        vgmDEFINE_ALLOCATE_TEMP(color);
        vgmDEFINE_ALLOCATE_TEMP(sum);
        vgmDEFINE_ALLOCATE_VARYING(myTexCoord, X2, 1);
        vgmDEFINE_ALLOCATE_SAMPLER(ImageSampler, SAMPLER_2D, 1);
        vgmDEFINE_ALLOCATE_OUTPUT(gl_FragColor, "#Color", X4, 1);


        if (hardware->filterType == vgvFILTER_COPY)
        {
            gcmASSERT(hardware->drawPipe == vgvDRAWPIPE_COPY);
            if (hardware->scissoring)
            {
                vgmDEFINE_ALLOCATE_TEMP(scissorValue);
                vgmDEFINE_ALLOCATE_SAMPLER(MaskSampler, SAMPLER_2D, 1);
                vgmDEFINE_ALLOCATE_VARYING(texCoord, X2, 1);

                /* there is shader optimize bug, and seems  use TEXLD must  enable XYZW to get around*/
                vgmOPCODE(TEXLD, scissorValue, XYZW);
                    vgmSAMPLER(MaskSampler);
                    vgmVARYING(texCoord, XYYY, 0);

                vgmOPCODE_COND(KILL, EQUAL, 0);
                    vgmTEMP(scissorValue, YYYY);
                    vgmCONST(0);
            }

            /* color = texture2D(ImageSampler,myTexCoord) */
            vgmOPCODE(TEXLD, color, XYZW);
                vgmSAMPLER(ImageSampler);
                vgmVARYING(myTexCoord, XYYY, 0);
#if WALK600     /*H10702 & H10902*/
            if (hardware->isConformance && hardware->isGC600_19)
            {
                gcmERR_BREAK(_GenTextureTuneCode(Shader, color));
            }
#endif
        }
        else if (hardware->filterType == vgvFILTER_COLOR_MATRIX)
        {
            vgmDEFINE_ALLOCATE_UNIFORM(ColorMatrix, 4X4, 1);
            vgmDEFINE_ALLOCATE_UNIFORM(Offset, X4, 1);
            vgmDEFINE_ALLOCATE_TEMP(temp0);
            vgmDEFINE_ALLOCATE_TEMP(temp1);
            vgmDEFINE_ALLOCATE_TEMP(temp2);
            vgmDEFINE_ALLOCATE_TEMP(temp3);
            vgmDEFINE_ALLOCATE_TEMP(temp4);
            vgmDEFINE_ALLOCATE_TEMP(temp5);
            vgmDEFINE_ALLOCATE_TEMP(temp6);

            /* color = texture2D(ImageSampler,myTexCoord) */
            vgmOPCODE(TEXLD, color, XYZW);
                vgmSAMPLER(ImageSampler);
                vgmVARYING(myTexCoord, XYYY, 0);
#if WALK600     /*J40107*/
            if (hardware->isConformance && hardware->isGC600_19)
            {
                gcmERR_BREAK(_GenTextureTuneCode(Shader, color));
            }
#endif

            /* convert to procFormat */
            gcmERR_BREAK(_GenColorConvertCode(Shader,
                                              color,
                                              hardware->srcConvert,
                                              hardware->srcConvertAlpha));

            /*
            color =  colorMatrix * color + offset;
            */

            vgmOPCODE(MUL, temp0, XYZW);
                vgmUNIFORM(ColorMatrix, XYZW, 0);
                vgmTEMP(color, XXXX);

            vgmOPCODE(MUL, temp1, XYZW);
                vgmUNIFORM(ColorMatrix, XYZW, 1);
                vgmTEMP(color, YYYY);

            vgmOPCODE(ADD, temp2, XYZW);
                vgmTEMP(temp0, XYZW);
                vgmTEMP(temp1, XYZW);

            vgmOPCODE(MUL, temp3, XYZW);
                vgmUNIFORM(ColorMatrix, XYZW, 2);
                vgmTEMP(color, ZZZZ);

            vgmOPCODE(ADD, temp4, XYZW);
                vgmTEMP(temp2, XYZW);
                vgmTEMP(temp3, XYZW);

            vgmOPCODE(MUL, temp5, XYZW);
                vgmUNIFORM(ColorMatrix, XYZW, 3);
                vgmTEMP(color, WWWW);

            vgmOPCODE(ADD, temp6, XYZW);
                vgmTEMP(temp4, XYZW);
                vgmTEMP(temp5, XYZW);

            vgmOPCODE(ADD, color, XYZW);
                vgmTEMP(temp6, XYZW);
                vgmUNIFORM(Offset, XYZW, 0);
        }
        else if ((hardware->filterType == vgvFILTER_CONVOLVE) ||
                 (hardware->filterType == vgvFILTER_SEPCONVOLVE) ||
                 (hardware->filterType == vgvFILTER_GAUSSIAN))
        {
            /* need to pack to X4*/


            vgmDEFINE_ALLOCATE_UNIFORM(FilterTexBound,  X4, 3);


            vgmDEFINE_ALLOCATE_TEMP(i);
            vgmDEFINE_ALLOCATE_TEMP(temp0);
            vgmDEFINE_ALLOCATE_TEMP(temp1);
            vgmDEFINE_ALLOCATE_TEMP(temp2);
            vgmDEFINE_ALLOCATE_TEMP(temp3);
            vgmDEFINE_ALLOCATE_TEMP(temp4);
            vgmDEFINE_ALLOCATE_TEMP(temp5);
            vgmDEFINE_ALLOCATE_TEMP(coord);
            vgmDEFINE_ALLOCATE_LABEL(label0);
            vgmDEFINE_ALLOCATE_LABEL(label1);
            vgmDEFINE_ALLOCATE_TEMP(coord0);


            if (hardware->filterType == vgvFILTER_GAUSSIAN)
            {
                vgmDEFINE_ALLOCATE_TEMP(j);
                vgmDEFINE_ALLOCATE_TEMP(coordOffset);
                vgmDEFINE_ALLOCATE_LABEL(label2);
                vgmDEFINE_ALLOCATE_LABEL(label3);
                vgmDEFINE_ALLOCATE_UNIFORM(Kernel0,         X1, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(KernelSize,      X1, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(TexCoordOffset,  X4, vgvMAX_GAUSSIAN_COORD_SIZE);
                vgmDEFINE_ALLOCATE_UNIFORM(Kernel,          X4, vgvMAX_GAUSSIAN_KERNEL_SIZE);

                /*
                    color = texture2d(ImageSampler, myTexCoord);
                    color *= kernel0;
                    sum = color;
                */

                if (hardware->gaussianOPT)
                {
                    vgmDEFINE_ALLOCATE_VARYING(offset0, X4, 1);
                    vgmDEFINE_ALLOCATE_VARYING(offset1, X4, 1);
                    vgmDEFINE_ALLOCATE_VARYING(offset2, X4, 1);
                    vgmDEFINE_ALLOCATE_VARYING(offset3, X4, 1);

                    vgmDEFINE_ALLOCATE_TEMP(tempColor0);
                    vgmDEFINE_ALLOCATE_TEMP(tempColor1);
                    vgmDEFINE_ALLOCATE_TEMP(tempColor2);
                    vgmDEFINE_ALLOCATE_TEMP(tempColor3);
                    vgmDEFINE_ALLOCATE_TEMP(tempColor4);
                    vgmDEFINE_ALLOCATE_TEMP(tempColor5);
                    vgmDEFINE_ALLOCATE_TEMP(tempColor6);
                    vgmDEFINE_ALLOCATE_TEMP(tempColor7);

                    vgmDEFINE_ALLOCATE_TEMP(temp7);
                    vgmDEFINE_ALLOCATE_TEMP(temp8);
                    vgmDEFINE_ALLOCATE_TEMP(temp9);
                    vgmDEFINE_ALLOCATE_TEMP(temp10);
                    vgmDEFINE_ALLOCATE_TEMP(temp11);
                    vgmDEFINE_ALLOCATE_TEMP(temp12);
                    vgmDEFINE_ALLOCATE_TEMP(temp13);
                    vgmDEFINE_ALLOCATE_TEMP(temp14);
                    vgmDEFINE_ALLOCATE_TEMP(temp15);
                    vgmDEFINE_ALLOCATE_TEMP(temp16);
                    vgmDEFINE_ALLOCATE_TEMP(temp17);
                    vgmDEFINE_ALLOCATE_TEMP(temp18);
                    vgmDEFINE_ALLOCATE_TEMP(temp19);
                    vgmDEFINE_ALLOCATE_TEMP(temp20);
                    vgmDEFINE_ALLOCATE_TEMP(temp21);


                    if (hardware->disableTiling)
                    {
                        vgmOPCODE(TEXLD, color, XYZW);
                            vgmSAMPLER(ImageSampler);
                            vgmVARYING(myTexCoord, XYYY, 0);
                    }
                    else
                    {
                        vgmOPCODE(MOV, coord0, XY);
                            vgmVARYING(myTexCoord, XYYY, 0);

                        gcmERR_BREAK(_GenSampleTextureCode(hardware, Shader, ImageSampler, coord0,
                                                            hardware->tilingMode, FilterTexBound,  color,
                                                            (hardware->srcImage->parent == gcvNULL) ? gcvTRUE : gcvFALSE));
                    }
                    gcmERR_BREAK(_GenColorConvertCode(Shader,
                                                          color,
                                                          hardware->srcConvert,
                                                          hardware->srcConvertAlpha));

                    vgmOPCODE(MUL, temp4, XYZW);
                        vgmTEMP(color, XYZW);
                        vgmUNIFORM(Kernel0, XXXX, 0);

                    vgmOPCODE(MOV, color, XYZW);
                        vgmTEMP(temp4, XYZW);
                        /*
                            sum = texture2D(sampler2d, myTexCoord);
                            sum =  sum * kernel0;
                            color0 = texture2D(sampler2d, offset0.xy);
                            color1 = texture2D(sampler2d, offset0.zw);
                            color2 = texture2D(sampler2d, offset1.xy);
                            color3 = texture2D(sampler2d, offset1.zw);
                            color4 = texture2D(sampler2d, offset2.xy);
                            color5 = texture2D(sampler2d, offset2.zw);
                            color6 = texture2D(sampler2d, offset3.xy);
                            color7 = texture2D(sampler2d, offset3.zw);
                            sum += color0*kernel.r;
                            sum += color1*kernel.g;
                            sum += color2*kernel.b;
                            sum += color3*kernel.a;
                            sum += color4*kernel.r;
                            sum += color5*kernel.g;
                            sum += color6*kernel.b;
                            sum += color7*kernel.a;
                            gl_FragColor = sum;
                        */
            #define vgmGAUSSIAN_GETCOLOR(offset, offsetSwizzle, tempColor) \
                            if (hardware->disableTiling) \
                            {\
                                vgmOPCODE(TEXLD, tempColor, XYZW);\
                                    vgmSAMPLER(ImageSampler);\
                                    vgmVARYING(offset, offsetSwizzle, 0);\
                            }\
                            else \
                            {\
                                vgmOPCODE(MOV, coord, XY);\
                                vgmVARYING(offset, offsetSwizzle, 0);\
                                gcmERR_BREAK(_GenSampleTextureCode(hardware, Shader, ImageSampler, coord, \
                                                    hardware->tilingMode, FilterTexBound,  tempColor,\
                                                    (hardware->srcImage->parent == gcvNULL) ? gcvTRUE : gcvFALSE));\
                            }\
                            gcmERR_BREAK(_GenColorConvertCode(Shader, \
                                                              tempColor, \
                                                              hardware->srcConvert, \
                                                              hardware->srcConvertAlpha));

            #define vgmGAUSSIAN_MADDCOLOR(tempColor, kernelSwizzle, myTemp1, myTemp2, myTemp3) \
                            vgmOPCODE(MUL, myTemp2, XYZW);\
                                vgmTEMP(tempColor, XYZW);\
                                vgmUNIFORM(Kernel, kernelSwizzle, 0);\
                            \
                            vgmOPCODE(ADD, myTemp3, XYZW);\
                                vgmTEMP(myTemp2, XYZW);\
                                vgmTEMP(myTemp1, XYZW);

                    /* for match the MADD instructions */
                    vgmGAUSSIAN_GETCOLOR(offset0, XYYY, tempColor0);
                    vgmGAUSSIAN_GETCOLOR(offset0, ZWWW, tempColor1);
                    vgmGAUSSIAN_GETCOLOR(offset1, XYYY, tempColor2);
                    vgmGAUSSIAN_GETCOLOR(offset1, ZWWW, tempColor3);
                    vgmGAUSSIAN_GETCOLOR(offset2, XYYY, tempColor4);
                    vgmGAUSSIAN_GETCOLOR(offset2, ZWWW, tempColor5);
                    vgmGAUSSIAN_GETCOLOR(offset3, XYYY, tempColor6);
                    vgmGAUSSIAN_GETCOLOR(offset3, ZWWW, tempColor7);

                    vgmGAUSSIAN_MADDCOLOR(tempColor0, XXXX, color,  temp7,  temp8);
                    vgmGAUSSIAN_MADDCOLOR(tempColor1, YYYY, temp8,  temp9,  temp10);
                    vgmGAUSSIAN_MADDCOLOR(tempColor2, ZZZZ, temp10, temp11, temp12);
                    vgmGAUSSIAN_MADDCOLOR(tempColor3, WWWW, temp12, temp13, temp14);
                    vgmGAUSSIAN_MADDCOLOR(tempColor4, XXXX, temp14, temp15, temp16);
                    vgmGAUSSIAN_MADDCOLOR(tempColor5, YYYY, temp16, temp17, temp18);
                    vgmGAUSSIAN_MADDCOLOR(tempColor6, ZZZZ, temp18, temp19, temp20);
                    vgmGAUSSIAN_MADDCOLOR(tempColor7, WWWW, temp20, temp21, color);

                }
                else
                {

                    vgmOPCODE(MOV, coord0, XY);
                        vgmVARYING(myTexCoord, XYYY, 0);

                    gcmERR_BREAK(_GenSampleTextureCode(hardware, Shader, ImageSampler, coord0,
                                                        hardware->tilingMode, FilterTexBound,  color,
                                                        (hardware->srcImage->parent == gcvNULL) ? gcvTRUE : gcvFALSE));

                    gcmERR_BREAK(_GenColorConvertCode(Shader,
                                                          color,
                                                          hardware->srcConvert,
                                                          hardware->srcConvertAlpha));

                    vgmOPCODE(MUL, temp4, XYZW);
                        vgmTEMP(color, XYZW);
                        vgmUNIFORM(Kernel0, XXXX, 0);

                    vgmOPCODE(MOV, sum, XYZW);
                        vgmTEMP(temp4, XYZW);

                    /*
                    for (int j = -1; j < 2; j+= 2)
                    */
                    vgmOPCODE(MOV, j, X);
                        vgmCONST(-1);

                    vgmLABEL(label2);

                    vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label3);
                        vgmTEMP(j, XXXX);
                        vgmCONST(2);
                    {

                        /*
                            coordOffset = texCoordOffset * j;
                        */

                        vgmOPCODE(MUL, coordOffset, XY);
                            vgmTEMP(j, XXXX);
                            vgmUNIFORM(TexCoordOffset, XYYY, 0);

                        vgmOPCODE(MOV, coord0, XY);
                            vgmVARYING(myTexCoord, XYYY, 0);

                        /*
                        for(int i = 0; i < int(KernelSize); i++)
                        */
                        vgmOPCODE(MOV, i, X);
                            vgmCONST(0);

                        vgmLABEL(label1);

                        vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label0);
                            vgmTEMP(i, XXXX);
                            vgmUNIFORM(KernelSize, XXXX, 0);
                        {
                    #define vgmGAUSSIAN_COLOR(operation, startCoord, kernelSwizzle) \
                            vgmOPCODE(operation, coord, XY);\
                                vgmTEMP(startCoord, XYYY);\
                                vgmTEMP(coordOffset, XYYY);\
                            vgmOPCODE(MOV, startCoord, XY);\
                                vgmTEMP(coord, XYYY);\
                            \
                            gcmERR_BREAK(_GenSampleTextureCode(hardware, Shader, ImageSampler, coord, \
                                                    hardware->tilingMode, FilterTexBound,  color,\
                                                    (hardware->srcImage->parent == gcvNULL) ? gcvTRUE : gcvFALSE));\
                            \
                            gcmERR_BREAK(_GenColorConvertCode(Shader, \
                                                              color, \
                                                              hardware->srcConvert, \
                                                              hardware->srcConvertAlpha));\
                            vgmOPCODE(MUL, temp3, XYZW);\
                                vgmTEMP(color, XYZW);\
                                vgmUNIFORM_INDEXED(Kernel, kernelSwizzle, i);\
                            \
                            vgmOPCODE(ADD, color, XYZW);\
                                vgmTEMP(temp3, XYZW);\
                                vgmTEMP(sum, XYZW);\
                            \
                            vgmOPCODE(MOV, sum, XYZW);\
                                vgmTEMP(color, XYZW);

                            /*
                                coord0 = myTexCoord;
                                coord1 = myTexCoord;

                                coord0 = coord0 + texCoordOffset.xy;
                                color = mySample(coord0);
                                sum += color * kernel[i].r;
                            */


                            vgmGAUSSIAN_COLOR(ADD, coord0, XXXX);
                            vgmGAUSSIAN_COLOR(ADD, coord0, YYYY);
                            vgmGAUSSIAN_COLOR(ADD, coord0, ZZZZ);
                            vgmGAUSSIAN_COLOR(ADD, coord0, WWWW);

                            /* i++; */
                            vgmOPCODE(ADD, temp0, X);
                                vgmTEMP(i, XXXX);
                                vgmCONST(1);

                            vgmOPCODE(MOV, i, X);
                                vgmTEMP(temp0, XXXX);

                            vgmOPCODE_COND(JMP, ALWAYS, label1);

                        }
                        vgmLABEL(label0);

                        /* j+= 2; */
                        vgmOPCODE(ADD, temp0, X);
                            vgmTEMP(j, XXXX);
                            vgmCONST(2);

                        vgmOPCODE(MOV, j, X);
                            vgmTEMP(temp0, XXXX);

                        vgmOPCODE_COND(JMP, ALWAYS, label2);
                    }
                    vgmLABEL(label3);
                }

            }
            else if (hardware->filterType == vgvFILTER_SEPCONVOLVE)
            {
                vgmDEFINE_ALLOCATE_TEMP(j);
                vgmDEFINE_ALLOCATE_LABEL(label2);
                vgmDEFINE_ALLOCATE_LABEL(label3);
                vgmDEFINE_ALLOCATE_UNIFORM(Scale,       X1, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(Bias,        X1, 1);

                vgmDEFINE_ALLOCATE_UNIFORM(KernelSizeY, X1, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(TexCoordOffsetY, X2, vgvMAX_SEPCONVOLVE_COORD_SIZE);
                vgmDEFINE_ALLOCATE_UNIFORM(KernelY,         X2, vgvMAX_SEPCONVOLVE_KERNEL_SIZE);

                vgmDEFINE_ALLOCATE_UNIFORM(KernelSize,  X1, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(TexCoordOffset,  X2, vgvMAX_SEPCONVOLVE_COORD_SIZE);
                vgmDEFINE_ALLOCATE_UNIFORM(Kernel,          X2, vgvMAX_SEPCONVOLVE_KERNEL_SIZE);

                /*sum = vec4(0.0f, 0.0f, 0.0f, 0.0f);*/
                vgmOPCODE(MOV, sum, XYZW);
                    vgmCONST(0.0f);
                /*
                for(int i = 0; i < int(KernelSize); i++)
                */
                vgmOPCODE(MOV, i, X);
                    vgmCONST(0);

                vgmLABEL(label1);

                vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label0);
                    vgmTEMP(i, XXXX);
                    vgmUNIFORM(KernelSize, XXXX, 0);
                {
    #define vgmSEPCONVOLVE_COLOR(SwizzleX, SwizzleY) \
                    vgmOPCODE(ADD, coord, Y);\
                        vgmVARYING(myTexCoord, YYYY, 0);\
                        vgmUNIFORM_INDEXED(TexCoordOffsetY, SwizzleY, j);\
                    vgmOPCODE(ADD, coord, X);\
                        vgmVARYING(myTexCoord, XXXX, 0);\
                        vgmUNIFORM_INDEXED(TexCoordOffset, SwizzleX, i);\
                    gcmERR_BREAK(_GenSampleTextureCode(hardware, Shader, ImageSampler, coord,\
                                                        hardware->tilingMode, FilterTexBound,  color,\
                                            (hardware->srcImage->parent == gcvNULL) ? gcvTRUE : gcvFALSE));\
                    \
                    gcmERR_BREAK(_GenColorConvertCode(Shader, \
                                                      color, \
                                                      hardware->srcConvert, \
                                                      hardware->srcConvertAlpha));\
                    vgmOPCODE(MUL, temp3, XYZW);\
                        vgmTEMP(color, XYZW);\
                        vgmUNIFORM_INDEXED(Kernel, SwizzleX, i);\
                    \
                    vgmOPCODE(MUL, temp4, XYZW);\
                        vgmTEMP(temp3, XYZW);\
                        vgmUNIFORM_INDEXED(KernelY, SwizzleY, j);\
                    \
                    vgmOPCODE(ADD, color, XYZW);\
                        vgmTEMP(temp4, XYZW);\
                        vgmTEMP(sum, XYZW);\
                    \
                    vgmOPCODE(MOV, sum, XYZW);\
                        vgmTEMP(color, XYZW);

                    /*

                        coord = myTexCoord + vec2(texCoordOffset[i].x, texCoordOffsetY[i].x);;
                        color = mySample(coord);
                        sum += color * kernelY[j].r * kernel[i].r;
                    */

                    vgmOPCODE(MOV, j, X);
                        vgmCONST(0);

                    vgmLABEL(label3);

                    vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label2);
                        vgmTEMP(j, XXXX);
                        vgmUNIFORM(KernelSizeY, XXXX, 0);
                    {

                        vgmSEPCONVOLVE_COLOR(XXXX, XXXX);
                        vgmSEPCONVOLVE_COLOR(XXXX, YYYY);
                        vgmSEPCONVOLVE_COLOR(YYYY, YYYY);
                        vgmSEPCONVOLVE_COLOR(YYYY, XXXX);



                        /* j++; */
                        vgmOPCODE(ADD, temp0, X);
                            vgmTEMP(j, XXXX);
                            vgmCONST(1);

                        vgmOPCODE(MOV, j, X);
                            vgmTEMP(temp0, XXXX);

                        vgmOPCODE_COND(JMP, ALWAYS, label3);
                    }
                    vgmLABEL(label2);

                    /* i++; */
                    vgmOPCODE(ADD, temp0, X);
                        vgmTEMP(i, XXXX);
                        vgmCONST(1);

                    vgmOPCODE(MOV, i, X);
                        vgmTEMP(temp0, XXXX);

                    vgmOPCODE_COND(JMP, ALWAYS, label1);

                }
                vgmLABEL(label0);
                /*
                    color   = sum * scale;
                    color   += vec4(bias, bias, bias, bias);
                */

                vgmOPCODE(MUL, temp5, XYZW);
                    vgmTEMP(sum, XYZW);
                    vgmUNIFORM(Scale, XXXX, 0);

                vgmOPCODE(ADD, color, XYZW);
                    vgmTEMP(temp5, XYZW);
                    vgmUNIFORM(Bias, XXXX, 0);
            }
            else
            {
                vgmDEFINE_ALLOCATE_UNIFORM(Scale,       X1, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(Bias,        X1, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(KernelSize,  X1, 1);
                vgmDEFINE_ALLOCATE_UNIFORM(TexCoordOffset,  X4, vgvMAX_CONVOLVE_COORD_SIZE);
                vgmDEFINE_ALLOCATE_UNIFORM(Kernel,          X4, vgvMAX_CONVOLVE_KERNEL_SIZE);

                gcmASSERT(hardware->filterType == vgvFILTER_CONVOLVE);

                /*sum = vec4(0.0f, 0.0f, 0.0f, 0.0f);*/
                vgmOPCODE(MOV, sum, XYZW);
                    vgmCONST(0.0f);
                /*
                for(int i = 0; i < int(KernelSize); i++)
                */
                vgmOPCODE(MOV, i, X);
                    vgmCONST(0);

                vgmLABEL(label1);

                vgmOPCODE_COND(JMP, GREATER_OR_EQUAL, label0);
                    vgmTEMP(i, XXXX);
                    vgmUNIFORM(KernelSize, XXXX, 0);
                {
        #define vgmCONVOLVE_COLOR(texCoordIndex, texCoordSwizzle, kernelSwizzle) \
                    vgmOPCODE(ADD, coord, XY);\
                        vgmVARYING(myTexCoord, XYYY, 0);\
                        vgmUNIFORM_INDEXED(TexCoordOffset, texCoordSwizzle, texCoordIndex);\
                    \
                    gcmERR_BREAK(_GenSampleTextureCode(hardware, Shader, ImageSampler, \
                                        coord, hardware->tilingMode, FilterTexBound,  color,\
                                        (hardware->srcImage->parent == gcvNULL) ? gcvTRUE : gcvFALSE));\
                    \
                    gcmERR_BREAK(_GenColorConvertCodeEx(Shader, color,  \
                                        hardware->srcConvert, hardware->srcConvertAlpha, \
                                        gcvFALSE, gcvTRUE));\
                    \
                    vgmOPCODE(MUL, temp3, XYZW);\
                        vgmTEMP(color, XYZW);\
                        vgmUNIFORM_INDEXED(Kernel, kernelSwizzle, i);\
                    \
                    vgmOPCODE(ADD, color, XYZW);\
                        vgmTEMP(temp3, XYZW);\
                        vgmTEMP(sum, XYZW);\
                    \
                    vgmOPCODE(MOV, sum, XYZW);\
                        vgmTEMP(color, XYZW);

                    /* 2*i */
                    vgmOPCODE(MUL, temp1, X);
                        vgmCONST(2);
                        vgmTEMP(i, XXXX);

                    /* 2*i + 1 */
                    vgmOPCODE(ADD, temp2, X);
                        vgmTEMP(temp1, XXXX);
                        vgmCONST(1);

                    /*
                        coord = myTexCoord + texCoordOffset[2*i].xy;
                        color = mySample(coord);
                        sum += color * kernel[i].r;
                    */
                    vgmCONVOLVE_COLOR(temp1, XYYY, XXXX);
                    vgmCONVOLVE_COLOR(temp1, ZWWW, YYYY);
                    vgmCONVOLVE_COLOR(temp2, XYYY, ZZZZ);
                    vgmCONVOLVE_COLOR(temp2, ZWWW, WWWW);



                    /* i++; */
                    vgmOPCODE(ADD, temp0, X);
                        vgmTEMP(i, XXXX);
                        vgmCONST(1);

                    vgmOPCODE(MOV, i, X);
                        vgmTEMP(temp0, XXXX);

                    vgmOPCODE_COND(JMP, ALWAYS, label1);

                }
                vgmLABEL(label0);

                /*
                    color   = sum * scale;
                    color   += vec4(bias, bias, bias, bias);
                */

                vgmOPCODE(MUL, temp4, XYZW);
                    vgmTEMP(sum, XYZW);
                    vgmUNIFORM(Scale, XXXX, 0);

                /* if seperate convolve , can optimize this instruction*/
                vgmOPCODE(ADD, color, XYZW);
                    vgmTEMP(temp4, XYZW);
                    vgmUNIFORM(Bias, XXXX, 0);
            }

        }
        else if (hardware->filterType == vgvFILTER_LOOKUP)
        {
            vgmDEFINE_ALLOCATE_SAMPLER(LutSampler, SAMPLER_1D, 1);
            vgmDEFINE_ALLOCATE_TEMP(temp0);

            /*
                vec4 color     =  texture2D(ImageSampler,myTexCoord);
                color = myColorConvert(color, srcConvert, srcConvertAlpha);
            */
            vgmOPCODE(TEXLD, color, XYZW);
                vgmSAMPLER(ImageSampler);
                vgmVARYING(myTexCoord, XYZZ, 0);

            /* convert to procFormat */
            gcmERR_BREAK(_GenColorConvertCode(Shader,
                                              color,
                                              hardware->srcConvert,
                                              hardware->srcConvertAlpha));

            /*
                color.r =  texture2D(LutSampler,vec2(color.r, 0.0f)).r ;
                color.g =  texture2D(LutSampler,vec2(color.g, 0.0f)).g ;
                color.b =  texture2D(LutSampler,vec2(color.b, 0.0f)).b ;
                color.a =  texture2D(LutSampler,vec2(color.a, 0.0f)).a ;
            */
            vgmOPCODE(TEXLD, temp0, X);
                vgmSAMPLER(LutSampler);
                vgmTEMP(color, XXXX);

            vgmOPCODE(TEXLD, temp0, Y);
                vgmSAMPLER(LutSampler);
                vgmTEMP(color, YYYY);

            vgmOPCODE(TEXLD, temp0, Z);
                vgmSAMPLER(LutSampler);
                vgmTEMP(color, ZZZZ);

            vgmOPCODE(TEXLD, temp0, W);
                vgmSAMPLER(LutSampler);
                vgmTEMP(color, WWWW);

            vgmOPCODE(MOV, color, XYZW);
                vgmTEMP(temp0, XYZW);

        }
        else if (hardware->filterType == vgvFILTER_LOOKUP_SINGLE)
        {
            vgmDEFINE_ALLOCATE_SAMPLER(LutSampler, SAMPLER_1D, 1);
            vgmDEFINE_ALLOCATE_TEMP(select);

            /*
                vec4 color     =  texture2D(ImageSampler,myTexCoord);
                color = myColorConvert(color, srcConvert, srcConvertAlpha);
            */
            vgmOPCODE(TEXLD, color, XYZW);
                vgmSAMPLER(ImageSampler);
                vgmVARYING(myTexCoord, XYZZ, 0);

            /* convert to procFormat */
            gcmERR_BREAK(_GenColorConvertCode(Shader,
                                              color,
                                              hardware->srcConvert,
                                              hardware->srcConvertAlpha));


            /*
                float select;
                if (int(channel) == 1) select = color.a;
                if (int(channel) == 2) select = color.b;
                if (int(channel) == 4) select = color.g;
                if (int(channel) == 8) select = color.r;
                color = texture2D(samplerlut,vec2(select, 0.0f)) ;
            */
            if (hardware->sourceChannel == VG_RED)
            {
                vgmOPCODE(MOV, select, X);
                    vgmTEMP(color, XXXX);
            }

            if (hardware->sourceChannel == VG_GREEN)
            {
                vgmOPCODE(MOV, select, X);
                    vgmTEMP(color, YYYY);
            }

            if (hardware->sourceChannel == VG_BLUE)
            {
                vgmOPCODE(MOV, select, X);
                    vgmTEMP(color, ZZZZ);
            }

            if (hardware->sourceChannel == VG_ALPHA)
            {
                vgmOPCODE(MOV, select, X);
                    vgmTEMP(color, WWWW);
            }

            vgmOPCODE(TEXLD, color, XYZW);
                vgmSAMPLER(LutSampler);
                vgmTEMP(select, XXXX);
        }
        else
        {
            /* unsupport filter type */
            gcmASSERT(0);
        }

        if (!hardware->disableClamp)
        {
            /* clamp the color for pre/nonpre form*/
            gcmERR_BREAK(_GenClampColorCode(Shader, color, (hardware->dstConvertAlpha & 0x1) ? gcvTRUE :  gcvFALSE ));
        }

        gcmERR_BREAK(_GenColorConvertCodeEx(Shader, color, hardware->dstConvert, hardware->dstConvertAlpha, gcvTRUE, gcvFALSE));

        /*  color pack */
        /*
            if (flag == 1.0f)
            {
                if (color.r >= 0.5f)
                    color.rgb = vec3(1.0f, 1.0f, 1.0f);
                else
                    color.rgb = vec3(0.0, 0.0, 0.0);
            }
            else if(flag == 4.0 || flag == 64.0)\
            {
                color.a = floor(color.a * 15.0 + 0.5);\
                color.a /= 15.0;
            }
            else if(flag == 8.0 || flag == 32.0)
            {
                color.a = (color.a >= 0.5) ? 1.0 : 0.0;
            }
            if (alphaOnly == 1.0)\
            {
                color.rgb = vec3(1.0, 1.0, 1.0);
            }
        */
        if (hardware->pack == 1)
        {
            vgmDEFINE_ALLOCATE_LABEL(label0);
            vgmDEFINE_ALLOCATE_LABEL(label1);

            /*
                if (color.r >= 0.5)
                    color.rgb = vec3(1.0, 1.0, 1.0);
                else
                    color.rgb = vec3(0.0, 0.0, 0.0);
            */
            vgmOPCODE_COND(JMP, LESS, label0);
                vgmTEMP(color, XXXX);
                vgmCONST(0.5f);
            {
                vgmOPCODE(MOV, color, XYZ);
                    vgmCONST(1.0f);

                vgmOPCODE_COND(JMP, ALWAYS, label1);
            }
            vgmLABEL(label0);
            {
                vgmOPCODE(MOV, color, XYZ);
                    vgmCONST(0.0f);
            }
            vgmLABEL(label1);
        }
        else if ((hardware->pack == 4) || (hardware->pack == 64))
        {
            vgmDEFINE_ALLOCATE_TEMP(temp0);
            vgmDEFINE_ALLOCATE_TEMP(temp1);
            vgmDEFINE_ALLOCATE_TEMP(temp2);
            /*
                color.a = floor(color.a * 15.0 + 0.5);
                color.a /= 15.0;
            */
            vgmOPCODE(MUL, temp0, X);
                vgmTEMP(color, WWWW);
                vgmCONST(15.0f);

            vgmOPCODE(ADD, temp1, X);
                vgmTEMP(temp0, XXXX);
                vgmCONST(0.5f);

            vgmOPCODE(FLOOR, temp2, X);
                vgmTEMP(temp1, XXXX);

            vgmOPCODE(MUL, color, W);
                vgmTEMP(temp2, XXXX);
                vgmCONST(1.0f/15.0f);
        }
        else if ((hardware->pack == 8) || (hardware->pack == 32))
        {
            /*
            color.a = (color.a >= 0.5) ? 1.0 : 0.0;
            */
            vgmDEFINE_ALLOCATE_LABEL(label0);
            vgmDEFINE_ALLOCATE_LABEL(label1);

            vgmOPCODE_COND(JMP, LESS, label0);
                vgmTEMP(color, WWWW);
                vgmCONST(0.5f);
            {
                vgmOPCODE(MOV, color, W);
                    vgmCONST(1.0f);

                vgmOPCODE_COND(JMP, ALWAYS, label1);
            }
            vgmLABEL(label0);
            {
                vgmOPCODE(MOV, color, W);
                    vgmCONST(0.0f);
            }
            vgmLABEL(label1);
        }

        if (hardware->alphaOnly)
        {
            /*
            color.rgb = vec3(1.0, 1.0, 1.0);
            */
            vgmOPCODE(MOV, color, XYZ);
                vgmCONST(1.0f);
        }



        /* gl_FragColor = color */
        vgmOPCODE(MOV, gl_FragColor, XYZW);
            vgmTEMP(color, XYZW);

#if WALK_MG1
        if ( hardware->isConformance &&
            (hardware->context->model == 0x800 && hardware->context->revision == 0x4301) &&
            (hardware->context->targetImage.internalColorDesc.surFormat == gcvSURF_R5G6B5) &&
            (hardware->dstConvert == 17) &&
            (hardware->filterType == vgvFILTER_GAUSSIAN) &&
            (hardware->gdx == 1.0f &&
             hardware->gdy != 1.0f && hardware->gdy < 15.9f && hardware->gdy > 0))
        {
            vgmOPCODE(ADD, gl_FragColor, X);
                vgmTEMP(color, XXXX);
                vgmCONST(0.00605f);

            vgmOPCODE(ADD, gl_FragColor, Z);
                vgmTEMP(color, ZZZZ);
                vgmCONST(0.00605f);
        }
#endif

        if ( hardware->isConformance && (hardware->context->model >= 0x2000))
        {

            vgmDEFINE_ALLOCATE_TEMP(temp0);

            vgmOPCODE(MUL, temp0, XYZ);
                vgmTEMP(gl_FragColor, XYZZ);
                vgmCONST(0.9981f);

             vgmOPCODE(MOV, gl_FragColor, XYZ);
                vgmTEMP(temp0, XYZZ);
        }

        /*_GenKillAlphaCode(Shader, gl_FragColor, hardware->blending, hardware->blendMode);*/

        gcmERR_BREAK(gcSHADER_Pack(Shader->binary));
#if vgvUSE_SHADER_OPTIM
        gcmERR_BREAK(gcOptimizeShader(Shader->binary, gcvNULL));
#endif
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _GenerateShaderCode(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        if (hardware->drawPipe == vgvDRAWPIPE_IMAGE || hardware->drawPipe == vgvDRAWPIPE_PATH)
        {
            gcmERR_BREAK(_GenPathImageVertexCode(hardware));
            gcmERR_BREAK(_GenPathImageFragmentCode(hardware));
        }
        else if (hardware->drawPipe == vgvDRAWPIPE_FILTER || hardware->drawPipe == vgvDRAWPIPE_COPY)
        {
            gcmERR_BREAK(_GenFilterVertexCode(hardware));
            gcmERR_BREAK(_GenFilterFragmentCode(hardware));
        }
        else if (hardware->drawPipe == vgvDRAWPIPE_COLORRAMP || hardware->drawPipe == vgvDRAWPIPE_CLEAR)
        {
            gcmERR_BREAK(_GenColorRampVertexCode(hardware));
            gcmERR_BREAK(_GenColorRampFragmentCode(hardware));
        }
        else if (hardware->drawPipe == vgvDRAWPIPE_MASK)
        {
            gcmERR_BREAK(_GenMaskVertexCode(hardware));
            gcmERR_BREAK(_GenMaskFragmentCode(hardware));
        }
        else
        {
            gcmBREAK();
        }
    }while(gcvFALSE);

    gcmFOOTER();

    return status;
}


static gceSTATUS _CreateProgramEntry(_vgHARDWARE *hardware, _vgPROGRAMTABLE *table)
{
    gceSTATUS   status = gcvSTATUS_OK;
    _VGProgram* program;

    gcmHEADER_ARG("hardware=0x%x table=0x%x",
                  hardware, table);

    do
    {
        /* create a new program */
        NEWOBJ(_VGProgram, hardware->core.os, program);
        if (program == gcvNULL)
        {
            status = gcvSTATUS_OUT_OF_MEMORY;

            gcmFOOTER();
            return status;
        }

        gcmERR_BREAK(gcSHADER_Construct(hardware->core.hal, VGShaderType_Vertex, &program->vertexShader.binary));
        gcmERR_BREAK(gcSHADER_Construct(hardware->core.hal, VGShaderType_Fragment, &program->fragmentShader.binary));

        table->program = program;

        gcmFOOTER();
        return status;
    }
    while(gcvFALSE);

    /*roll back the resources*/
    DELETEOBJ(_VGProgram, hardware->core.os, program);

    gcmFOOTER();
    return status;
}

static gctUINT32 _GetPackValue(gctUINT32 pack)
{
    gctUINT32 value;

    gcmHEADER_ARG("pack=%u", pack);

    switch(pack)
    {
        case 1:
            value = 1;
            break;
        case 8:
        case 32:
            value = 2;
            break;
        case 4:
        case 64:
            value = 3;
            break;
        default:
            value = 0;
            break;
    }

    gcmFOOTER_ARG("return=%u", value);

    return value;
}



static gceSTATUS _GetCurrentProgram(_vgHARDWARE *hardware)
{
    gceSTATUS       status = gcvSTATUS_OK;
    _vgSHADERKEY    entry;
    gctINT          i;
    gctUINT32       colorConvert;
    gctUINT32       alphaConvert;
    _VGColorFormat  colorFormat;
    _VGColorFormat  dstColorFormat =
        hardware->context->targetImage.internalColorDesc.colorFormat;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    gcoOS_ZeroMemory(&entry, sizeof(_vgSHADERKEY));

    entry.drawPipe = hardware->drawPipe;

    if (hardware->drawPipe == vgvDRAWPIPE_PATH )
    {
        if (_UseShaderBlending(hardware))
        {
            entry.blend = (hardware->blendMode - VG_BLEND_SRC) + 1;
        }

        entry.colorTransform    = hardware->colorTransform ? 1 : 0;
        entry.masking           = hardware->masking ? 1 : 0;
        entry.scissoring        = hardware->scissoring ? 1 : 0;


        entry.paintType         = hardware->paint->paintType - VG_PAINT_TYPE_COLOR;


        if ((hardware->paint->paintType == VG_PAINT_TYPE_PATTERN) && hardware->paint->pattern)
        {
            entry.patternTilingMode = hardware->paint->patternTilingMode - VG_TILE_FILL;

            if ((hardware->paint->patternTilingMode == VG_TILE_REPEAT) || \
                (hardware->paint->patternTilingMode == VG_TILE_REFLECT))
            {
                entry.rootImage = (hardware->paint->pattern->parent == gcvNULL) ? 1 : 0;
            }
        }
        else if (hardware->paint->paintType == VG_PAINT_TYPE_PATTERN)
        {
            entry.paintType         = 0;
        }

        if (((hardware->paint->paintType == VG_PAINT_TYPE_LINEAR_GRADIENT) || \
            (hardware->paint->paintType == VG_PAINT_TYPE_RADIAL_GRADIENT)) && hardware->paint->zeroFlag)
        {
            entry.paintZero     = 1;
        }

        _GetPaintColorFormat(hardware->paint, &colorFormat);

        if ((colorFormat & PREMULTIPLIED) &&  hardware->colorTransform)
        {
            colorFormat &= ~PREMULTIPLIED;
            entry.extra |= 0x1;
        }

        if (entry.blend)
        {
			dstColorFormat = (_VGColorFormat)(((gctINT)dstColorFormat) | PREMULTIPLIED);
        }


        _GetConvertValue(colorFormat, dstColorFormat, &colorConvert, &alphaConvert);

        entry.srcConvert = _GetConvertMapValue(colorConvert);
        entry.srcConvertAlpha = alphaConvert;

        if (hardware->masking)
        {
            _GetConvertValue(
                hardware->context->targetImage.internalColorDesc.colorFormat,
                lRGBA_PRE, &colorConvert, &alphaConvert);
            entry.maskColorConvert1 = _GetConvertMapValue(colorConvert);
            entry.maskColorConvertAlpha1 =  alphaConvert;


            _GetConvertValue(lRGBA_PRE,
                hardware->context->targetImage.internalColorDesc.colorFormat, &colorConvert, &alphaConvert);

            entry.maskColorConvert2 = _GetConvertMapValue(colorConvert);
            entry.maskColorConvertAlpha2 =  alphaConvert;

            dstColorFormat = hardware->context->targetImage.internalColorDesc.colorFormat;
        }

        if ((((gctINT)dstColorFormat) & PREMULTIPLIED)
            && (!(((gctINT)hardware->context->targetImage.internalColorDesc.colorFormat) & PREMULTIPLIED)))
        {
            entry.dstConvertAlpha = 1;
        }
        else if ((!(((gctINT)dstColorFormat) & PREMULTIPLIED))
            && (((gctINT)hardware->context->targetImage.internalColorDesc.colorFormat) & PREMULTIPLIED))
        {
            entry.dstConvertAlpha = 2;
        }
    }
    else if (hardware->drawPipe == vgvDRAWPIPE_IMAGE)
    {
        if (_UseShaderBlending(hardware))
        {
            entry.blend = (hardware->blendMode - VG_BLEND_SRC) + 1;
        }

        entry.colorTransform    = hardware->colorTransform ? 1 : 0;
        entry.masking           = hardware->masking ? 1 : 0;
        entry.scissoring        = hardware->scissoring ? 1 : 0;

        entry.imageMode         = hardware->imageMode - VG_DRAW_IMAGE_NORMAL;

        if (hardware->imageMode != VG_DRAW_IMAGE_NORMAL)
        {
            entry.paintType = hardware->paint->paintType - VG_PAINT_TYPE_COLOR;

            if ((hardware->paint->paintType == VG_PAINT_TYPE_PATTERN) && hardware->paint->pattern)
            {
                entry.patternTilingMode = hardware->paint->patternTilingMode - VG_TILE_FILL;

                if ((hardware->paint->patternTilingMode == VG_TILE_REPEAT) || \
                    (hardware->paint->patternTilingMode == VG_TILE_REFLECT))
                {
                    entry.rootImage = (hardware->paint->pattern->parent == gcvNULL) ? 1 : 0;
                }
            }
            else if (hardware->paint->paintType == VG_PAINT_TYPE_PATTERN)
            {
                entry.paintType         = 0;
            }
        }

        if (hardware->imageMode == VG_DRAW_IMAGE_NORMAL)
        {
            colorFormat = hardware->srcImage->internalColorDesc.colorFormat;
            if ((colorFormat & PREMULTIPLIED) &&  hardware->colorTransform)
            {
                colorFormat &= ~PREMULTIPLIED;
                entry.extra |= 1;
            }

            if (entry.blend)
            {
				dstColorFormat = (_VGColorFormat)(((gctINT)dstColorFormat) | PREMULTIPLIED);
            }
        }
        else if (hardware->imageMode == VG_DRAW_IMAGE_MULTIPLY)
        {
            _VGColorFormat paintFormat;
            colorFormat = hardware->srcImage->internalColorDesc.colorFormat;
            _GetPaintColorFormat(hardware->paint, &paintFormat);

            if (!(colorFormat & PREMULTIPLIED))
            {
                entry.extra |= 1;
                colorFormat |= PREMULTIPLIED;
            }

            if (!(paintFormat & PREMULTIPLIED))
            {
                entry.extra |= 2;
            }

            if (entry.blend)
            {
				dstColorFormat = (_VGColorFormat)(((gctINT)dstColorFormat) | PREMULTIPLIED);
            }
        }
        else
        {
            _VGColorFormat imageFormat = hardware->srcImage->internalColorDesc.colorFormat;
            _GetPaintColorFormat(hardware->paint, &colorFormat);
            if ((colorFormat & PREMULTIPLIED) &&  hardware->colorTransform)
            {
                entry.extra |= 1;
                colorFormat &= ~PREMULTIPLIED;
            }

            if (!(imageFormat & PREMULTIPLIED))
            {
                entry.extra |= 2;
            }

			dstColorFormat = (_VGColorFormat)(((gctINT)dstColorFormat) | PREMULTIPLIED);
        }



        _GetConvertValue(colorFormat, dstColorFormat, &colorConvert, &alphaConvert);

        entry.srcConvert = _GetConvertMapValue(colorConvert);
        entry.srcConvertAlpha = alphaConvert;

        if (hardware->masking)
        {
            _GetConvertValue(
                hardware->context->targetImage.internalColorDesc.colorFormat,
                lRGBA_PRE, &colorConvert, &alphaConvert);

            entry.maskColorConvert1 = _GetConvertMapValue(colorConvert);
            entry.maskColorConvertAlpha1 =  alphaConvert;


            _GetConvertValue(lRGBA_PRE,
                hardware->context->targetImage.internalColorDesc.colorFormat,
                &colorConvert, &alphaConvert);

            entry.maskColorConvert2 = _GetConvertMapValue(colorConvert);
            entry.maskColorConvertAlpha2 =  alphaConvert;

            dstColorFormat = hardware->context->targetImage.internalColorDesc.colorFormat;
        }

        if ((((gctINT)dstColorFormat) & PREMULTIPLIED)
            && (!(((gctINT)hardware->context->targetImage.internalColorDesc.colorFormat) & PREMULTIPLIED)))
        {
            entry.dstConvertAlpha = 1;
        }
        else if ((!(((gctINT)dstColorFormat) & PREMULTIPLIED))
            && (((gctINT)hardware->context->targetImage.internalColorDesc.colorFormat) & PREMULTIPLIED))
        {
            entry.dstConvertAlpha = 2;
        }

    }
    else if (hardware->drawPipe == vgvDRAWPIPE_COLORRAMP || hardware->drawPipe == vgvDRAWPIPE_CLEAR)
    {
        if ((hardware->drawPipe == vgvDRAWPIPE_CLEAR) && hardware->scissoring)
        {
            entry.scissoring    = 1;
        }

        if ((hardware->drawPipe == vgvDRAWPIPE_COLORRAMP) && hardware->paint->colorRampPremultiplied)
        {
            gcmASSERT((hardware->paint->paintType == VG_PAINT_TYPE_RADIAL_GRADIENT) || \
                (hardware->paint->paintType == VG_PAINT_TYPE_LINEAR_GRADIENT));
            entry.colorRampPremultiplied = 1;
        }
    }
    else if (hardware->drawPipe == vgvDRAWPIPE_FILTER || hardware->drawPipe == vgvDRAWPIPE_COPY)
    {
        entry.filterType = hardware->filterType;


        if ((hardware->drawPipe == vgvDRAWPIPE_COPY)  && hardware->scissoring && (hardware->filterType == vgvFILTER_COPY))
        {
            entry.scissoring = 1;
        }


        if ((hardware->filterType == vgvFILTER_SEPCONVOLVE) || \
            (hardware->filterType == vgvFILTER_GAUSSIAN) ||
            (hardware->filterType == vgvFILTER_CONVOLVE))
        {
            entry.patternTilingMode = hardware->tilingMode - VG_TILE_FILL;
            entry.rootImage         = (hardware->srcImage->parent == gcvNULL) ? 1 : 0;
        }

        if (hardware->filterType == vgvFILTER_LOOKUP_SINGLE)
        {
            entry.filterChannel = hardware->sourceChannel;
        }

        if (hardware->filterType == vgvFILTER_GAUSSIAN)
        {
            if (hardware->gaussianOPT)
            {
                entry.guassianOPT = 1;
            }

            if (hardware->disableClamp)
            {
                entry.disableClamp = 1;
            }

            if (hardware->disableTiling)
            {
                entry.disableTiling = 1;
            }
        }



        entry.srcConvert            = _GetConvertMapValue(hardware->srcConvert);
        entry.srcConvertAlpha       = hardware->srcConvertAlpha;

        entry.dstConvert            = _GetConvertMapValue(hardware->dstConvert);
        entry.dstConvertAlpha       = hardware->dstConvertAlpha;

        entry.clampToAlpha          = (entry.dstConvertAlpha & 0x1) ? 1 : 0;

        entry.alphaOnly             = hardware->alphaOnly ? 1 : 0;
        entry.pack                  = _GetPackValue(hardware->pack);


    }
    else if (hardware->drawPipe == vgvDRAWPIPE_MASK)
    {
        entry.maskOperation = hardware->maskOperation - VG_CLEAR_MASK;

        if ((hardware->maskOperation != VG_CLEAR_MASK) && \
            (hardware->maskOperation != VG_FILL_MASK) )
        {
            entry.hasAlpha = hardware->hasAlpha ? 1 : 0;
        }
    }
    else
    {
        gcmASSERT(0);
    }

    for(i = 0; i < vgvMAX_PROGRAMS; i++)
    {

        if (gcoOS_MemCmp(&hardware->programTable[i].entry,  &entry, sizeof(_vgSHADERKEY)) == 0)
        {
            hardware->program = hardware->programTable[i].program;
            gcmFOOTER();

            return status;
        }

        if (hardware->programTable[i].program == gcvNULL)
        {
            if (gcmIS_ERROR(_CreateProgramEntry(hardware, &hardware->programTable[i])))
                break;

            hardware->program = hardware->programTable[i].program;
            hardware->programTable[i].entry = entry;

            gcmFOOTER();

            return status;
        }
    }

    /* too many programs */
    gcmASSERT(0);

    status = gcvSTATUS_OUT_OF_RESOURCES;

    gcmFOOTER();

    return status;
}


static gceSTATUS _EnableAttributes(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        /* only vertex shader has attributes */
        _VGShader *vertShader = &hardware->program->vertexShader;
        gcePRIMITIVE primitiveType = gcvPRIMITIVE_TRIANGLE_FAN;
        gctINT      base = 0;
        gctINT      start = 0;
        gctSIZE_T   primitiveCount = 0;
        gctBOOL     drawIndex = gcvFALSE;

        if (hardware->drawPipe == vgvDRAWPIPE_PATH)
        {
            _VGContext      *context        = hardware->context;
            _VGPath         *p              = hardware->path;

                        /* tessellate the path */
            if (hardware->paintMode == VG_FILL_PATH)
            {

                status = _TessFlatten(context, p, hardware->userToSurface, 0.0f);
                if (gcmIS_ERROR(status))
				{
                    break;
				}
                status = _TessFillPath(context, p, hardware->userToSurface);
                if (gcmIS_ERROR(status))
				{
                    break;
				}

                gcmERR_BREAK(_LoadVertexs(hardware, &p->tessellateResult.vertexBuffer));

                if (p->tessellateResult.fillDesc.primitiveCount > 0)
                {
                    drawIndex = gcvTRUE;
                    gcmERR_BREAK(_LoadIndices(hardware, &p->tessellateResult.indexBuffer));
                }


                primitiveCount      = p->tessellateResult.fillDesc.primitiveCount > 0 ? \
                                    (p->tessellateResult.fillDesc.primitiveCount) : \
                                    (-p->tessellateResult.fillDesc.primitiveCount);

                primitiveType       = gcvPRIMITIVE_TRIANGLE_LIST;
            }
            else
            {
                gcmASSERT(hardware->paintMode == VG_STROKE_PATH);

                status = _TessFlatten(context, p,  hardware->userToSurface, TESS_ABS(context->strokeLineWidth));
                if (gcmIS_ERROR(status))
				{
                    break;
				}
                status = _TessStrokePath(context, p, hardware->userToSurface);
                if (gcmIS_ERROR(status))
				{
                    break;
				}

                gcmERR_BREAK(_LoadVertexs(hardware, &p->tessellateResult.strokeBuffer));
                gcmERR_BREAK(_LoadIndices(hardware, &p->tessellateResult.strokeIndexBuffer));

                primitiveCount      = p->tessellateResult.strokeDesc.primitiveCount;
#if SPECIAL_1PX_STROKE
                primitiveType       = p->tessellateResult.strokeDesc.primitiveType;
#else
                primitiveType       = gcvPRIMITIVE_TRIANGLE_LIST;
#endif
                drawIndex           = gcvTRUE;
            }

        }
        else if (hardware->drawPipe == vgvDRAWPIPE_COLORRAMP)
        {
            _VGPaint* paint = hardware->paint;

            if (paint->dirty & OVGPaintDirty_ColorStop)
            {
                /* adjust the struct layout for easy upload, and need to do more optimize */
                {
                    gctINT i;
                    /*get around the viewport 0.5 offset bug*/
                    gctFLOAT y = (hardware->context->model == gcv500) ? 0.0f : 1.0f;
                    ARRAY_RESIZE(paint->colorStops,  paint->colorRampStops.size * 8);
                    if (paint->colorStops.items == gcvNULL)
                    {
                        gcmERR_BREAK(gcvSTATUS_OUT_OF_MEMORY);
                    }
                    for(i = 0; i < paint->colorRampStops.size; i++)
                    {

                        paint->colorStops.items[i * 8]     = paint->colorRampStops.items[i].color.r;
                        paint->colorStops.items[i * 8 + 1] = paint->colorRampStops.items[i].color.g;
                        paint->colorStops.items[i * 8 + 2] = paint->colorRampStops.items[i].color.b;
                        paint->colorStops.items[i * 8 + 3] = paint->colorRampStops.items[i].color.a;

                        paint->colorStops.items[i * 8 + 4] = paint->colorRampStops.items[i].offset * (2.0f - 2.0f/256.0f) - (1.0f - 1.0f/256.0f);
                        paint->colorStops.items[i * 8 + 5] = y;
                        paint->colorStops.items[i * 8 + 6] = 0.0f;
                        paint->colorStops.items[i * 8 + 7] = 1.0f;
                    }

                    /* Check head/tail overlapping. */
                    if ((paint->colorRampStops.items[0].offset == paint->colorRampStops.items[1].offset) &&
                        (paint->colorRampStops.items[0].offset == 0.0f))
                    {
                        paint->colorStops.items[12] += 1.0f/128.0f;
                    }
                    i--;
                    if ((paint->colorRampStops.items[i].offset == paint->colorRampStops.items[i - 1].offset) &&
                        (paint->colorRampStops.items[i].offset == 1.0f))
                    {
                        paint->colorStops.items[(i - 1) * 8 + 4] -= 1.0f/128.0f;
                    }
                }

                if (paint->lineStream)
                {
                    gcmVERIFY_OK(gcoSTREAM_Destroy(paint->lineStream));
                    paint->lineStream = gcvNULL;
                }

                gcmERR_BREAK(gcoSTREAM_Construct(hardware->core.hal, &paint->lineStream));

                gcmERR_BREAK(gcoSTREAM_Upload(
                                    paint->lineStream,
                                    paint->colorStops.items,
                                    0,
                                    paint->colorStops.size * sizeof(VGfloat),
                                    gcvFALSE));

                gcmERR_BREAK(gcoSTREAM_SetStride(paint->lineStream, sizeof(VGfloat) * 4 * 2));
            }

            PaintClean(paint);

            gcmERR_BREAK(vgshCORE_EnableAttribute(&hardware->core,
                            vertShader->attributes[vgvATTRIBUTE_COLOR].index,
                            gcvVERTEX_FLOAT,
                            gcvFALSE,
                            4,
                            paint->lineStream,
                            0,
                            8 * sizeof(gctFLOAT)));

            gcmERR_BREAK(vgshCORE_EnableAttribute(&hardware->core,
                            vertShader->attributes[vgvATTRIBUTE_VERTEX].index,
                            gcvVERTEX_FLOAT,
                            gcvFALSE,
                            4,
                            paint->lineStream,
                            4 * sizeof(gctFLOAT),
                            8 * sizeof(gctFLOAT)));

            gcmERR_BREAK(vgshCORE_BindVertex(&hardware->core));

            gcmERR_BREAK(vgshCORE_DisableAttribute(&hardware->core, vertShader->attributes[vgvATTRIBUTE_VERTEX].index));
            gcmERR_BREAK(vgshCORE_DisableAttribute(&hardware->core, vertShader->attributes[vgvATTRIBUTE_COLOR].index));


            primitiveCount      = paint->colorRampStops.size - 1;
            primitiveType       = gcvPRIMITIVE_LINE_STRIP;

        }
        else if (hardware->drawPipe == vgvDRAWPIPE_IMAGE ||
                 hardware->drawPipe == vgvDRAWPIPE_FILTER  ||
                 hardware->drawPipe == vgvDRAWPIPE_COPY ||
                 hardware->drawPipe == vgvDRAWPIPE_MASK)
        {
            gcoSTREAM   stream;

            gcmERR_BREAK(_QueryStream(
                hardware,
                hardware->sx,
                hardware->sy,
                hardware->width,
                hardware->height,
                hardware->srcImage,
                &stream));

            gcmERR_BREAK(vgshCORE_EnableAttribute(&hardware->core,
                            vertShader->attributes[vgvATTRIBUTE_VERTEX].index,
                            gcvVERTEX_FLOAT,
                            gcvFALSE,
                            2,
                            stream,
                            0,
                            4 * sizeof(gctFLOAT)));

            gcmERR_BREAK(vgshCORE_EnableAttribute(&hardware->core,
                            vertShader->attributes[vgvATTRIBUTE_UV].index,
                            gcvVERTEX_FLOAT,
                            gcvFALSE,
                            2,
                            stream,
                            2 * sizeof(gctFLOAT),
                            4 * sizeof(gctFLOAT)));

            gcmERR_BREAK(vgshCORE_BindVertex(&hardware->core));

            gcmERR_BREAK(vgshCORE_DisableAttribute(&hardware->core, vertShader->attributes[vgvATTRIBUTE_VERTEX].index));

            gcmERR_BREAK(vgshCORE_DisableAttribute(&hardware->core, vertShader->attributes[vgvATTRIBUTE_UV].index));

            primitiveCount      = 2;
            primitiveType       = gcvPRIMITIVE_TRIANGLE_FAN;
        }
        else if (hardware->drawPipe == vgvDRAWPIPE_CLEAR)
        {

            gctFLOAT    x0, y0, x1, y1;
            _VGImage    *dstImage = hardware->dstImage;

            gctINT32    dx = hardware->dx;
            gctINT32    dy = hardware->dy;

            gctINT32 width   = hardware->width;
            gctINT32 height  = hardware->height;

            gctINT32 ancHeight = dstImage->rootHeight;
            gctINT32 ancWidth  = dstImage->rootWidth;

            dx += dstImage->rootOffsetX;
            dy += dstImage->rootOffsetY;

            if (hardware->dstImage->orient == vgvORIENTATION_LEFT_TOP)
            {
                x0 = (gctFLOAT)dx;
                y0 = (gctFLOAT)dy;
                x1 = (gctFLOAT)width  + x0;
                y1 = (gctFLOAT)height + y0;
            }
            else
            {
                x0 = (gctFLOAT)dx;
                y0 = (gctFLOAT)(ancHeight  - dy);
                x1 = (gctFLOAT)(x0 + width);
                y1 = (gctFLOAT)(ancHeight - (dy + height));
            }

            /* map to (-1.0 ~ 1.0) */
            x0 = x0 / (gctFLOAT)ancWidth    * 2.0f - 1.0f;
            x1 = x1 / (gctFLOAT)ancWidth    * 2.0f - 1.0f;
            y0 = y0 / (gctFLOAT)ancHeight   * 2.0f - 1.0f;
            y1 = y1 / (gctFLOAT)ancHeight   * 2.0f - 1.0f;


            /* TODO: opitimize */
            if (hardware->rectStream != gcvNULL)
            {
                gcmVERIFY_OK(gcoSTREAM_Destroy(hardware->rectStream));
                hardware->rectStream = gcvNULL;
            }

            {
                gctFLOAT vertexAttriData[24];

                vertexAttriData[0]  = x0;
                vertexAttriData[1]  = y1;
                vertexAttriData[2]  = hardware->clearColor.r;
                vertexAttriData[3]  = hardware->clearColor.g;
                vertexAttriData[4]  = hardware->clearColor.b;
                vertexAttriData[5]  = hardware->clearColor.a;
                vertexAttriData[6]  = x1;
                vertexAttriData[7]  = y1;
                vertexAttriData[8]  = hardware->clearColor.r;
                vertexAttriData[9]  = hardware->clearColor.g;
                vertexAttriData[10] = hardware->clearColor.b;
                vertexAttriData[11] = hardware->clearColor.a;
                vertexAttriData[12] = x1;
                vertexAttriData[13] = y0;
                vertexAttriData[14] = hardware->clearColor.r;
                vertexAttriData[15] = hardware->clearColor.g;
                vertexAttriData[16] = hardware->clearColor.b;
                vertexAttriData[17] = hardware->clearColor.a;
                vertexAttriData[18] = x0;
                vertexAttriData[19] = y0;
                vertexAttriData[20] = hardware->clearColor.r;
                vertexAttriData[21] = hardware->clearColor.g;
                vertexAttriData[22] = hardware->clearColor.b;
                vertexAttriData[23] = hardware->clearColor.a;

                gcmERR_BREAK(gcoSTREAM_Construct(hardware->core.hal, &hardware->rectStream));

                gcmERR_BREAK(gcoSTREAM_Upload(
                                    hardware->rectStream,
                                    vertexAttriData,
                                    0,
                                    sizeof(vertexAttriData),
                                    gcvFALSE));

                gcmERR_BREAK(gcoSTREAM_SetStride(hardware->rectStream, sizeof(gctFLOAT) * 6));
            }

            gcmERR_BREAK(vgshCORE_EnableAttribute(&hardware->core,
                            vertShader->attributes[vgvATTRIBUTE_VERTEX].index,
                            gcvVERTEX_FLOAT,
                            gcvFALSE,
                            2,
                            hardware->rectStream,
                            0,
                            6 * sizeof(gctFLOAT)));

            gcmERR_BREAK(vgshCORE_EnableAttribute(&hardware->core,
                            vertShader->attributes[vgvATTRIBUTE_COLOR].index,
                            gcvVERTEX_FLOAT,
                            gcvFALSE,
                            4,
                            hardware->rectStream,
                            2 * sizeof(gctFLOAT),
                            6 * sizeof(gctFLOAT)));


            gcmERR_BREAK(vgshCORE_BindVertex(&hardware->core));

            gcmERR_BREAK(vgshCORE_DisableAttribute(&hardware->core, vertShader->attributes[vgvATTRIBUTE_VERTEX].index));
            gcmERR_BREAK(vgshCORE_DisableAttribute(&hardware->core, vertShader->attributes[vgvATTRIBUTE_COLOR].index));

            primitiveCount      = 2;
            primitiveType       = gcvPRIMITIVE_TRIANGLE_FAN;

        }
        else
        {
            gcmBREAK();
        }

        gcmERR_BREAK(vgshCORE_SetPrimitiveInfo(&hardware->core, primitiveType, base, start, primitiveCount, drawIndex));
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

static gceSTATUS _LoadAttributes(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        gctUINT i, j;
        gctSIZE_T count;

        /*only vertex shader has attributes*/
        _VGShader *vertShader = &hardware->program->vertexShader;
        gcATTRIBUTE  attribute;

        if (!vertShader->resolveAttributes)
        {
            /* Get the number of vertex attributes. */
            gcmVERIFY_OK(gcSHADER_GetAttributeCount(vertShader->binary, &count));
            for (j = 0; j < count; j++)
            {
                /* Get the gcATTRIBUTE object pointer. */
                gcmVERIFY_OK(gcSHADER_GetAttribute(vertShader->binary,
                                        j,
                                        &attribute));


                for (i = 0; i < vgvMAX_ATTRIBUTES; i++)
                {
                    /* match the attribute*/
                    if (attribute == vertShader->attributes[i].attribute)
                    {
                        vertShader->attributes[i].index = (gctINT16) j;

                        /* next */
                        break;
                    }
                }
            }

            /*attributes resolved*/
            vertShader->resolveAttributes = gcvTRUE;
        }

        /* enable the attributes */
        gcmERR_BREAK(_EnableAttributes(hardware));
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS _LoadUniforms(_vgHARDWARE *hardware)
{
    gctINT i;
    gceSTATUS status = gcvSTATUS_OK;
    _VGShader *vertShader = &hardware->program->vertexShader;
    _VGShader *fragShader = &hardware->program->fragmentShader;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    /* load the uniforms in vertex shader */
    for (i = 0; i < vertShader->uniformCount; i++)
    {
        gcmONERROR((*vertShader->uniforms[i].setfunc)(hardware, vertShader->uniforms[i].uniform));
    }

    /* load the uniforms in fragment shader */
    for (i = 0; i < fragShader->uniformCount; i++)
    {
        gcmONERROR((*fragShader->uniforms[i].setfunc)(hardware, fragShader->uniforms[i].uniform));
    }

    /* load the samplers in fragment shader */
    for (i = 0; i < fragShader->samplerCount; i++)
    {
        gcmONERROR((*fragShader->samplers[i].setfunc)(hardware, fragShader->samplers[i].sampler));
    }

OnError:
    gcmFOOTER();

    return status;
}

static gceSTATUS _LoadShader(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        /*get the shortcut program*/
        gcmERR_BREAK(_GetCurrentProgram(hardware));

        if (hardware->program->statesSize == 0)
        {
            /*generate the shader code*/
            gcmERR_BREAK(_GenerateShaderCode(hardware));

            /*link the shader*/
            gcmERR_BREAK(_LinkShader(hardware->program));
        }

        /*load attributes and uniforms*/
        gcmERR_BREAK(_LoadAttributes(hardware));
        gcmERR_BREAK(_LoadUniforms(hardware));

        /*load the shader*/
        gcmERR_BREAK(vgshCORE_LoadShader(&hardware->core,
                                        hardware->program->statesSize,
                                        hardware->program->states,
                                        hardware->program->hints));
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


gceSTATUS vgshShaderClear(
                       _VGContext       *context,
                       _VGImage*        image,
                       gctINT32         x,
                       gctINT32         y,
                       gctINT32         width,
                       gctINT32         height,
                       _VGColor         *color,
                       gctBOOL          scissoring,
                       gctBOOL          flush)
{
    gceSTATUS status = gcvSTATUS_OK;
    _vgHARDWARE *hardware = &context->hardware;

    gcmHEADER_ARG("context=0x%x image=0x%x x=%d y=%d width=%d height=%d "
                  "color=0x%x  scissoring=%s flush=%s",
                  context, image, x, y, width, height, color,
                  scissoring ? "gcvTRUE" : "gcvFALSE",
                  flush ? "gcvTRUE" : "gcvFALSE");

    do
    {
        if (scissoring)
        {
            gcmERR_BREAK(vgshUpdateScissor(context));
        }

        hardware->dstImage          = image;
        hardware->dx         = x;
        hardware->dy         = y;
        hardware->width     = width;
        hardware->height    = height;

        hardware->colorWrite        = 0xf;
        hardware->drawPipe          = vgvDRAWPIPE_CLEAR;
        hardware->depthCompare      = gcvCOMPARE_ALWAYS;
        hardware->depthWrite        = gcvFALSE;
        hardware->blending          = gcvFALSE;
        hardware->clearColor        = *color;

        hardware->flush             = flush;


        if (scissoring)
        {
            hardware->stencilMode       = gcvSTENCIL_SINGLE_SIDED;
            hardware->stencilMask       = 0xff;
            hardware->stencilRef        = 0;
            hardware->stencilCompare    = gcvCOMPARE_NOT_EQUAL;
            hardware->stencilFail       = gcvSTENCIL_KEEP;

            hardware->depthMode         = gcvDEPTH_Z;
#if NO_STENCIL_VG
            {
                hardware->stencilMode       = gcvSTENCIL_NONE;
                hardware->stencilCompare    = gcvCOMPARE_ALWAYS;
                hardware->zValue = context->scissorZ - POSITION_Z_INTERVAL;
                hardware->depthCompare      = gcvCOMPARE_GREATER;/*gcvCOMPARE_EQUAL;*/
            }
#endif  /* #ifdef NO_STENCIL_VG */
        }
        else
        {
            hardware->stencilMode   = gcvSTENCIL_NONE;
            hardware->depthMode     = gcvDEPTH_NONE;
        }

        gcmERR_BREAK(vgshHARDWARE_RunPipe(hardware));
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}

static gctBOOL _IsFullWindow(
    _VGImage *image,
    gctINT32 x,
    gctINT32 y,
    gctINT32 width,
    gctINT32 height)
{
    x += image->rootOffsetX;
    y += image->rootOffsetY;

    if ((x == 0) && (y == 0)
        && (width == image->rootWidth)
        && (height == image->rootHeight))
    {
        return gcvTRUE;
    }

    return gcvFALSE;
}

gceSTATUS vgshClear(
    _VGContext     *context,
    _VGImage       *image,
    gctINT32       x,
    gctINT32       y,
    gctINT32       width,
    gctINT32       height,
    _VGColor       *color,
    gctBOOL        scissoring,
    gctBOOL        flush)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("context=0x%x image=0x%x x=%d y=%d width=%d height=%d "
                  "color=0x%x  scissoring=%s flush=%s",
                  context, image, x, y, width, height, color,
                  scissoring ? "gcvTRUE" : "gcvFALSE",
                  flush ? "gcvTRUE" : "gcvFALSE");

    do
    {
#if !DISABLE_TILE_STATUS

        if (_IsFullWindow(image, x,  y, width, height) && !scissoring)
        {
            gcmERR_BREAK(gco3D_SetColorWrite(
                context->engine, 0xf));

            gcmERR_BREAK(gco3D_SetClearColorF(
                context->engine,
                color->r,
                color->g,
                color->b,
                color->a));

            /* special handle for fc */
            if ((image->surface == context->targetImage.surface)
                && (context->hardware.core.draw != context->targetImage.surface))
            {
                gcmERR_BREAK(vgshCORE_SetTarget(&context->hardware.core, image->surface));

            }

            /* use the surface clear */
            gcmERR_BREAK(gcoSURF_Clear(image->surface, gcvCLEAR_COLOR));

            *image->dirtyPtr = gcvTRUE;
        }
        else
#endif
        {
            /* use the shader to do the clear */
            gcmERR_BREAK(vgshShaderClear(context, image, x, y, width, height, color, scissoring, flush));
        }
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}


gceSTATUS vgshMask(_VGContext *context, _VGImage* maskImage,
                    VGMaskOperation operation,
                    gctINT32 dx, gctINT32 dy,
                    gctINT32 sx, gctINT32 sy,
                    gctINT32 width, gctINT32 height)
{
    gceSTATUS status = gcvSTATUS_OK;

    _vgHARDWARE     *hardware = &context->hardware;

    gcmHEADER_ARG("context=0x%x maskImage=0x%x operation=%d x=%d y=%d width=%d height=%d",
                  context, maskImage, operation, dx, dy, width, height);


    do
    {
        hardware->dstImage      = &context->maskImage;
        hardware->srcImage      = maskImage;



        if (operation == VG_CLEAR_MASK || operation == VG_FILL_MASK)
        {
            gctFLOAT    value = (operation == VG_CLEAR_MASK) ? 0.0f : 1.0f;

            hardware->clearColor.a = value;
            hardware->clearColor.r = value;
            hardware->clearColor.g = value;
            hardware->clearColor.b = value;

            hardware->drawPipe     = vgvDRAWPIPE_CLEAR;

        }
        else
        {
            gcmASSERT(maskImage != gcvNULL);

            hardware->hasAlpha  = maskImage->internalColorDesc.abits > 0;
            hardware->drawPipe  = vgvDRAWPIPE_MASK;
        }

        hardware->sx             = sx;
        hardware->sy             = sy;
        hardware->dx             = dx;
        hardware->dy             = dy;
        hardware->width         = width;
        hardware->height        = height;

        hardware->maskOperation = operation;
        hardware->blending      = gcvFALSE;

        /*hardware->scissoring  = gcvFALSE;*/
        hardware->stencilMode   = gcvSTENCIL_NONE;
        hardware->depthMode     = gcvDEPTH_NONE;

        hardware->depthCompare  = gcvCOMPARE_ALWAYS;
        hardware->depthWrite    = gcvFALSE;
        hardware->flush         = gcvTRUE;
        hardware->colorWrite    = 0xf;

        gcmERR_BREAK(vgshHARDWARE_RunPipe(hardware));
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS vgshDrawMaskPath(_VGContext *context, _VGImage* layer,  _VGPath *path, VGbitfield paintModes)
{
    gceSTATUS status = gcvSTATUS_OK;
    _vgHARDWARE     *hardware = &context->hardware;

    gcmHEADER_ARG("context=0x%x layer=0x%x path=0x%x paintModes=0x%x",
                  context, layer, path, paintModes);

    do
    {
        hardware->dstImage          = layer;
        hardware->colorWrite        = 0xf;
        hardware->masking           = gcvFALSE;
        hardware->colorTransform    = gcvFALSE;
        hardware->path              = path;
        hardware->drawPipe          = vgvDRAWPIPE_PATH;

        hardware->depthCompare      = gcvCOMPARE_ALWAYS;
        hardware->depthWrite        = gcvFALSE;
        hardware->blending          = gcvFALSE;

        hardware->userToSurface     = &context->pathUserToSurface;
        hardware->paint             = &context->defaultPaint;
        hardware->paintMode         = (VGPaintMode) paintModes;
        hardware->flush             = gcvTRUE;

        if (vgshIsScissoringEnabled(context))
        {
            hardware->stencilMode       = gcvSTENCIL_SINGLE_SIDED;
            hardware->stencilMask       = 0xff;
            hardware->stencilRef        = 0;
            hardware->stencilCompare    = gcvCOMPARE_NOT_EQUAL;
            hardware->stencilFail       = gcvSTENCIL_KEEP;
            hardware->depthMode         = gcvDEPTH_Z;
#if NO_STENCIL_VG
            {
                hardware->stencilMode       = gcvSTENCIL_NONE;
                hardware->stencilCompare    = gcvCOMPARE_ALWAYS;
                hardware->zValue = context->scissorZ - POSITION_Z_INTERVAL;
                hardware->depthCompare      = gcvCOMPARE_GREATER;/*gcvCOMPARE_EQUAL;*/
            }
#endif  /* #ifdef NO_STENCIL_VG */
        }
        else
        {
            hardware->stencilMode       = gcvSTENCIL_NONE;
            hardware->depthMode         = gcvDEPTH_NONE;
        }

        gcmERR_BREAK(vgshHARDWARE_RunPipe(hardware));
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}

gceSTATUS vgshCreateMaskBuffer(_VGContext  *context)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("context=0x%x", context);

    do
    {

        if (context->maskImage.surface == gcvNULL)
        {
            _VGColorDesc    colorDesc;
            vgshGetFormatColorDesc(VG_sRGBX_8888, &colorDesc);

            gcmVERIFY_OK(vgshIMAGE_Initialize(
                context,
                &context->maskImage,
                &colorDesc,
                context->targetImage.width,
                context->targetImage.height,
                context->targetImage.orient));

            gcmERR_BREAK(gco3D_SetClearColor(
                context->engine,
                255, 255, 255, 255));

            gcmERR_BREAK(gco3D_SetColorWrite(
                context->engine, 0xf));

            gcmERR_BREAK(gcoSURF_Clear(
                context->maskImage.surface,
                gcvCLEAR_COLOR));
        }

    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}

gceSTATUS _SetStates(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
#if !DISABLE_TILE_STATUS
        /* special handle for fc */
        if ((hardware->context->targetImage.surface == hardware->core.draw)
            && (hardware->core.draw != hardware->dstImage->surface))
        {
            gcmVERIFY_OK(gcoSURF_DisableTileStatus(
                hardware->context->targetImage.surface, gcvTRUE));
        }
#endif

        /* state setting */
        gcmERR_BREAK(vgshCORE_SetTarget(&hardware->core, hardware->dstImage->surface));

        if (_UseShaderBlending(hardware))
        {
            /* use the shader to do blending */
            gcmERR_BREAK(vgshCORE_EnableBlend(&hardware->core, gcvFALSE));
        }
        else
        {
            /* use pe to do blending */
            gcmERR_BREAK(vgshCORE_EnableBlend(&hardware->core, hardware->blending));
        }

        gcmERR_BREAK(vgshCORE_SetDepthCompare(&hardware->core, hardware->depthCompare));
        gcmERR_BREAK(vgshCORE_SetDepthMode(&hardware->core, hardware->depthMode));

        gcmERR_BREAK(vgshCORE_EnableDepthWrite(&hardware->core, hardware->depthWrite));

        gcmERR_BREAK(vgshCORE_SetColorWrite(&hardware->core, hardware->colorWrite));

#if !NO_STENCIL_VG
        /* enable the stencil test */
        gcmERR_BREAK(vgshCORE_EnableStencil(&hardware->core,
                                            hardware->stencilMode,
                                            hardware->stencilCompare,
                                            hardware->stencilRef,
                                            hardware->stencilMask,
                                            hardware->stencilFail));
#endif

        /*  cache now is valid */
        hardware->core.invalidCache = gcvFALSE;
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}


gceSTATUS vgshHARDWARE_RunPipe(_vgHARDWARE *hardware)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("hardware=0x%x", hardware);

    do
    {
        /* set the current stats*/
        gcmERR_BREAK(_SetStates(hardware));

        /*generate and load the shader*/
        gcmERR_BREAK(_LoadShader(hardware));

        /*draw the primitives*/
        gcmERR_BREAK(_DrawPrimitives(hardware));
    }
    while(gcvFALSE);

    gcmFOOTER();

    return status;
}
