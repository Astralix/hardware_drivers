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

/*******************************************************************************
***** Version Signature *******************************************************/

#define _gcmTXT2STR(t) #t
#define gcmTXT2STR(t) _gcmTXT2STR(t)
const char * _OPENVG_VERSION = "\n\0$VERSION$"
                               gcmTXT2STR(gcvVERSION_MAJOR) "."
                               gcmTXT2STR(gcvVERSION_MINOR) "."
                               gcmTXT2STR(gcvVERSION_PATCH) ":"
                               gcmTXT2STR(gcvVERSION_BUILD) "$\n";

static gctFLOAT getFloatMax()
{
    _VGfloatInt fi;

    fi.i = (((1<<(EXPONENT_BITS-1))-1+127) << 23) | (((1<<MANTISSA_BITS)-1) << (23-MANTISSA_BITS));

    return fi.f;
}

#define FLOAT_MAX  getFloatMax()


gctBOOL    IsNaN(gctFLOAT a)
{
    _VGuint32   exponent;
    _VGuint32   mantissa;
    _VGfloatInt p;

    p.f = a;
    exponent = (p.i>>23) & 0xff;
    mantissa = p.i & 0x7fffff;
    if(exponent == 255 && mantissa)
    {
        return gcvTRUE;
    }

    return gcvFALSE;
}

gctFLOAT    Mod(gctFLOAT a, gctFLOAT b)
{
    gctFLOAT f;

    gcmHEADER_ARG("a=%f b=%f", a, b);

    if(IsNaN(a) || IsNaN(b))
    {
        gcmFOOTER_ARG("return=%f", 0.0f);
        return 0.0f;
    }
    gcmASSERT(b >= 0.0f);
    if(b == 0.0f)
    {
        gcmFOOTER_ARG("return=%f", 0.0f);
        return 0.0f;
    }
    f = gcoMATH_Modulo(a, b);

    if(f < 0.0f) f = gcoMATH_Add(f, b);
    gcmASSERT(f >= 0.0f && f <= b);

    gcmFOOTER_ARG("return=%f", f);
    return f;
}


gctFLOAT    CLAMP(gctFLOAT a, gctFLOAT l, gctFLOAT h)
{
    if(IsNaN(a))
    {
        return l;
    }
    gcmASSERT(l <= h);

    return (a < l) ? l : (a > h) ? h : a;
}


void        SWAP(gctFLOAT *a, gctFLOAT *b)
{
    gctFLOAT tmp;

    tmp = *a;
    *a = *b;
    *b = tmp;
}

gctFLOAT    DEG_TO_RAD(gctFLOAT a)                  { return gcoMATH_Multiply(a, PI_DIV_180); }
gctFLOAT    RAD_TO_DEG(gctFLOAT a)                  { return gcoMATH_Divide(a, PI_DIV_180); }

gctINT32 ADDSATURATE_INT(gctINT32 a, gctINT32 b)
{
    gctINT32 r;

    r = a + b;
    gcmASSERT(b >= 0);

    return (r >= a) ? r : INT32_MAX;
}


gctBOOL isAligned(const void* ptr, gctINT32 alignment)
{
    gcmASSERT(alignment == 1 || alignment == 2 || alignment == 4);
    if(( *(_VGuint32*)(void**)&ptr) & (alignment-1))
        return gcvFALSE;
    return gcvTRUE;
}

void VGObject_AddRef(gcoOS os, _VGObject *object)
{
    object->reference++;
    gcmASSERT(object->reference >= 1);
}

void VGObject_Release(gcoOS os, _VGObject *object)
{
    gcmHEADER_ARG("os=0x%x object=0x%x", os, object);

    object->reference--;
    gcmASSERT(object->reference >= 0);
    if (object->reference == 0)
    {
        switch (object->type)
        {
        case VGObject_Path:

            DELETEOBJ(_VGPath, os, (_VGPath*)object);
            break;
        case VGObject_Image:
            DELETEOBJ(_VGImage, os, (_VGImage*)object);
            break;
        case VGObject_Paint:
            DELETEOBJ(_VGPaint, os, (_VGPaint*)object);
            break;
        case VGObject_Font:
            DELETEOBJ(_VGFont, os, (_VGFont*)object);
            break;
        case VGObject_MaskLayer:
            DELETEOBJ(_VGMaskLayer, os, (_VGMaskLayer*)object);
            break;
        default:
            gcmFATAL("Release: invalid type for object=0x%x", object);
        }
    }

    gcmFOOTER_NO();
}


#define FRAME_PATHS 200.0f  /* Assume that 200 paths at most in the next frame. */
void flush(_VGContext *context)
{
    gcmHEADER_ARG("context=0x%x", context);

    gcmVERIFY_OK(gcoSURF_Flush(context->targetImage.surface));
    gcmVERIFY_OK(gcoHAL_Commit(context->hal, gcvFALSE));

    if (context->hardware.zValue >= -POSITION_Z_INTERVAL * FRAME_PATHS)
    {
        gcmVERIFY_OK(gcoSURF_Clear(context->depth, gcvCLEAR_DEPTH));

        context->postionZ = POSITION_Z_BIAS;
        context->hardware.zValue            = POSITION_Z_BIAS;
    }

#if vgvUSE_PSC
    /* PSC. */
        /* Dec cache counter for all paths. */
    _PSCManagerShuffle(&context->pscm, -1);
        /* Kick out paths cache counter below than -10. (not used for more than 10 frames)*/
    _PSCManagerDismiss(context, -10);
#endif

    gcmFOOTER_NO();
}

void finish(_VGContext *context)
{
    gcmHEADER_ARG("context=0x%x", context);

    gcmVERIFY_OK(gcoSURF_Flush(context->targetImage.surface));
    gcmVERIFY_OK(gcoHAL_Commit(context->hal, gcvTRUE));

    if (context->hardware.zValue >= -POSITION_Z_INTERVAL * FRAME_PATHS)
    {
        gcmVERIFY_OK(gcoSURF_Clear(context->depth, gcvCLEAR_DEPTH));

        context->postionZ = POSITION_Z_BIAS;
        context->hardware.zValue            = POSITION_Z_BIAS;
    }

    gcmFOOTER_NO();
}

void  vgFlush(void)
{
    _VGContext* context;

    gcmHEADER();
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGFLUSH, 0);
    flush(context);

    gcmFOOTER_NO();
}


void  vgFinish(void)
{
    _VGContext* context;
    gcmHEADER();

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGFINISH, 0);
    finish(context);

    gcmFOOTER_NO();
}



void SetError(_VGContext *c, VGErrorCode e)
{
    gcmHEADER_ARG("c=0x%x e=0x%x", c, e);

    if (c->error == VG_NO_ERROR)
    {
        c->error = e;
    }

    gcmFOOTER_NO();
}

VGErrorCode  vgGetError(void)
{
    VGErrorCode error;
    _VGContext* context;
    gcmHEADER();

    OVG_GET_CONTEXT(VG_NO_CONTEXT_ERROR);

    vgmPROFILE(context, VGGETERROR, 0);

    error = context->error;
    context->error = VG_NO_ERROR;

    gcmFOOTER_ARG("return=0x%x", error);
    return error;
}


gctFLOAT inputFloat(VGfloat f)
{
    gctFLOAT r;

    /*this function is used for all floating point input values*/
    if(IsNaN(f))
    {
        return 0.0f;   /*convert NaN to zero*/
    }

    r = CLAMP(f, -FLOAT_MAX, FLOAT_MAX); /*clamp +-inf to +-gctFLOAT max*/

    return r;
}

static gctINT32 inputFloatToInt(VGfloat value)
{
    double v;

    v = (double)gcoMATH_Floor(inputFloat(value));

    return (v < (double)INT32_MIN) ? INT32_MIN
         : (v > (double)INT32_MAX) ? INT32_MAX
         : (gctINT32) v;
}

static gctINT32 paramToInt(const void* values, gctBOOL floats, gctINT32 count, gctINT32 i)
{
    gcmHEADER_ARG("values=0x%x floats=%s count=%d i=%d",
                   values, floats ? "TRUE" : "FALSE", count, i);

    gcmASSERT(i >= 0);

    if(i >= count || !values)
    {
        gcmFOOTER_ARG("return=%d", 0);
        return 0;
    }
    if(floats)
    {
        gctINT32 r = inputFloatToInt(((const VGfloat*)values)[i]);

        gcmFOOTER_ARG("return=%d", r);

        return r;
    }

    gcmFOOTER_ARG("return=%d", (gctINT32)((const gctINT32*)values)[i]);

    return (gctINT32)((const gctINT32*)values)[i];
}

static gctFLOAT paramToFloat(const void* values, gctBOOL floats, gctINT32 count, gctINT32 i)
{
    gcmHEADER_ARG("values=0x%x floats=%s count=%d i=%d",
                   values, floats ? "TRUE" : "FALSE", count, i);

    gcmASSERT(i >= 0);
    if(i >= count || !values)
    {
        gcmFOOTER_ARG("return=%f", 0.0f);

        return 0.0f;
    }

    if(floats)
    {
        gcmFOOTER_ARG("return=%f", ((const gctFLOAT*)values)[i]);

        return ((const gctFLOAT*)values)[i];
    }

    gcmFOOTER_ARG("return=%f", (gctFLOAT)((const gctINT32*)values)[i]);

    return (gctFLOAT)((const gctINT32*)values)[i];
}

static void floatToParam(void* output, gctBOOL outputFloats, gctINT32 count, gctINT32 i, VGfloat value)
{
    gcmHEADER_ARG("output=0x%x outputFloats=%s count=%d i=%d value=%f",
                   output, outputFloats ? "TRUE" : "FALSE", count, i, value);

    gcmASSERT(i >= 0);
    gcmASSERT(output);
    if(i >= count)
    {
        gcmFOOTER_NO();
        return;
    }

    if(outputFloats)
        ((VGfloat*)output)[i] = value;
    else
        ((VGint*)output)[i] = (VGint)inputFloatToInt(value);

    gcmFOOTER_NO();
}

static void intToParam(void* output, gctBOOL outputFloats, gctINT32 count, gctINT32 i, VGint value)
{
    gcmHEADER_ARG("output=0x%x outputFloats=%s count=%d i=%d value=%f",
                   output, outputFloats ? "TRUE" : "FALSE", count, i, value);

    gcmASSERT(i >= 0);
    gcmASSERT(output);
    if(i >= count)
    {
        gcmFOOTER_NO();
        return;
    }

    if(outputFloats)
        ((VGfloat*)output)[i] = (VGfloat)value;
    else
        ((VGint*)output)[i] = value;

    gcmFOOTER_NO();
}


_VGObject *GetVGObject(_VGContext *context, _VGObjectType type, VGHandle handle)
{
    _VGObject *object;

    gcmHEADER_ARG("context=0x%x type=%d handle=%u",
                   context, type, handle);

    object = vgshFindObject(context, handle);

    if ((gcvNULL == object) || (object->type != type))
    {
        object = gcvNULL;
    }

    gcmFOOTER_ARG("return=0x%x", object);

    return object;
}



void AllPathDirty(_VGContext *context, _VGTessPhase tessPhase)
{
    VGuint    index;
    _VGObject* object;

    gcmHEADER_ARG("context=0x%x tessPhase=0x%x", context, tessPhase);

    for (index = 0; index < NAMED_OBJECTS_HASH; index++)
    {
        object = context->sharedData->namedObjects[index];
        while(object != gcvNULL)
        {
            if (object->type == VGObject_Path)
            {
                PathDirty((_VGPath*)object, tessPhase);
            }
            object = object->next;
        }
    }

    gcmFOOTER_NO();
}

gceSTATUS _CreateSharedData(_VGContext *context, _VGContext *sharedContext)
{
    gceSTATUS status = gcvSTATUS_OK;
    gcmHEADER_ARG("context=0x%x sharedContext=0x%x", context, sharedContext);

    if (sharedContext == gcvNULL)
    {
        OVG_MALLOC(context->os, context->sharedData, sizeof(_vgSHAREDDATA));
        if (context->sharedData == gcvNULL)
        {
            status = gcvSTATUS_OUT_OF_MEMORY;
            gcmFOOTER();
            return status;
        }

        gcoOS_ZeroMemory(context->sharedData, sizeof(_vgSHAREDDATA));

        context->sharedData->reference = 1;
    }
    else
    {
        context->sharedData = ((_VGContext*)sharedContext)->sharedData;
        /* add the reference */
        context->sharedData->reference++;
    }

    gcmFOOTER();
    return status;
}

gceSTATUS _DestroySharedData(_VGContext *context)
{
    gctINT32 i;
    _VGObject   *object;
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("context=0x%x", context);

    if ((context == gcvNULL) || (context->sharedData == gcvNULL))
    {
        status = gcvSTATUS_INVALID_ARGUMENT;
        gcmFOOTER();
        return status;
    }

    context->sharedData->reference--;

    gcmASSERT(context->sharedData->reference >= 0);

    if (context->sharedData->reference == 0)
    {
        /* Free all attached named objects. */
        for (i = 0; i < NAMED_OBJECTS_HASH; i++)
        {
            /* Loop until this hash entry is empty. */
            while (context->sharedData->namedObjects[i] != gcvNULL)
            {
                /* Get named object. */
                object = context->sharedData->namedObjects[i];
                vgshRemoveObject(context, object);
                gcmASSERT((object->reference == 0) || (object->reference == 1));
                /* Dispatch in object type. */
                switch (object->type)
                {
                case VGObject_Path:

                    DELETEOBJ(_VGPath, context->os, (_VGPath*)object);
                    break;
                case VGObject_Image:
                    DELETEOBJ(_VGImage, context->os, (_VGImage*)object);
                    break;
                case VGObject_Paint:
                    DELETEOBJ(_VGPaint, context->os, (_VGPaint*)object);
                    break;
                case VGObject_Font:
                    DELETEOBJ(_VGFont, context->os, (_VGFont*)object);
                    break;
                case VGObject_MaskLayer:
                    DELETEOBJ(_VGMaskLayer, context->os, (_VGMaskLayer*)object);
                    break;
                default:
                    gcmFATAL(" invalid type for object=0x%x", object);
                }
            }
        }

        OVG_FREE(context->os, context->sharedData);

        context->sharedData = gcvNULL;
    }

    gcmFOOTER();
    return status;
}

#if vgvUSE_PSC
/* PSC: Init/Deinit. */
void _PSCManagerCtor(gcoOS os, vgsPSCManager *pscm)
{
    gcmHEADER_ARG("os=0x%x pscm=0x%x", os, pscm);

    pscm->capacity  = 100;
    pscm->count     = 0;
    pscm->streamSize= 0;
    OVG_MALLOC(os, (pscm->records), sizeof(vgsPSCRecord) * pscm->capacity);
    if (pscm->records != gcvNULL)
    {
        OVG_MEMSET((pscm->records), 0, sizeof(vgsPSCRecord) * pscm->capacity);
    }
    else
    {
        gcmASSERT(gcvFALSE);
    }

    gcmFOOTER_NO();
}

void _PSCManagerDtor(gcoOS os, vgsPSCManager *pscm)
{
    gcmHEADER_ARG("os=0x%x pscm=0x%x", os, pscm);

    pscm->capacity  = 0;
    pscm->count     = 0;
    pscm->streamSize= 0;
    OVG_SAFE_FREE(os, (pscm->records));

    gcmFOOTER_NO();
}

    /* Update the record when a path is choosen to draw. */
void _PSCManagerHit(VGint pid, vgsPSCManager *pscm, gcoOS os)
{
    int i;
    gcmHEADER_ARG("pid=%d pscm=0x%x os=0x%x", pid, pscm, os);

    if (pscm->count == pscm->capacity )
    {
        vgsPSCRecord *temp = gcvNULL;
        pscm->capacity += 50;
        OVG_MALLOC(os, temp, sizeof(vgsPSCRecord) * pscm->capacity);
        OVG_MEMCOPY(temp, pscm->records, sizeof(vgsPSCRecord) * (pscm->capacity - 50));
        OVG_FREE(os, pscm->records);
        pscm->records = temp;
    }

    for (i = 0; i < pscm->count; i++)
    {
        if (pscm->records[i].pid == pid)
        {
            pscm->records[i].weight++;
            break;
        }
    }

    /* A new item? */
    if (i == pscm->count)
    {
        pscm->count++;
        pscm->records[i].pid = pid;
        pscm->records[i].weight = 1;
    }

    gcmFOOTER_NO();
}

    /* Decrease cache counter for all records. */
void _PSCManagerShuffle(vgsPSCManager *pscm, VGint exception)
{
    int i;

    gcmHEADER_ARG("pscm=0x%x exception=%d", pscm, exception);

    if (exception > -1)
    {
        for (i = 0; i < pscm->count; i++)
        {
            if (pscm->records[i].pid != exception)
            {
                pscm->records[i].weight--;
            }
        }
    }
    else
    {
        for (i = 0; i < pscm->count; i++)
        {
            pscm->records[i].weight--;
        }
    }

    gcmFOOTER_NO();
}

    /* Flush cache objects that the score is lower than qual (optional). */
void _PSCManagerDismiss(_VGContext *context, gctINT32 qual)
{
    int i;
    vgsPSCManager   *pscm = &context->pscm;
    _VGPath *path;

    gcmHEADER_ARG("context=0x%x qual=%d", context, qual);

    for (i = 0; i < pscm->count; i++)
    {
        while (pscm->records[i].weight < qual)
        {
            /* TODO: Find the path object and dirty it. */
            path = (_VGPath*)vgshFindObject(context, pscm->records[i].pid);
            gcmASSERT((path != gcvNULL) && (path->object.type == VGObject_Path));

            if (path != gcvNULL)
            {
                if (path->tessellateResult.vertexBuffer.stream)
                {
                    gcoSTREAM_Destroy(path->tessellateResult.vertexBuffer.stream);
                    path->tessellateResult.vertexBuffer.stream = gcvNULL;
                }
                if (path->tessellateResult.indexBuffer.index)
                {
                    gcoINDEX_Destroy(path->tessellateResult.indexBuffer.index);
                    path->tessellateResult.indexBuffer.index = gcvNULL;
                }
                if (path->tessellateResult.strokeBuffer.stream)
                {
                    gcoSTREAM_Destroy(path->tessellateResult.strokeBuffer.stream);
                    path->tessellateResult.strokeBuffer.stream = gcvNULL;
                }
                if (path->tessellateResult.strokeIndexBuffer.index)
                {
                    gcoINDEX_Destroy(path->tessellateResult.strokeIndexBuffer.index);
                    path->tessellateResult.strokeIndexBuffer.index = gcvNULL;
                }
                PathDirty(path, VGTessPhase_ALL);
            }

            pscm->records[i] = pscm->records[pscm->count - 1];
            pscm->count--;
            if (pscm->count == 0)
                break;
        }
    }

    gcmFOOTER_NO();
}

    /* Delete a record cause the original does not exis any more. */
void _PSCManagerRelease(vgsPSCManager *pscm, VGint pid)
{
    int i;

    gcmHEADER_ARG("pscm=0x%x pid=%d", pscm, pid);

    for (i = 0; i < pscm->count; i++)
    {
        if (pscm->records[i].pid == pid)
        {
            pscm->count--;
            pscm->records[i] = pscm->records[pscm->count];
            break;
        }
    }

    gcmFOOTER_NO();
}
#endif

void _VGContextCtor(gcoOS os, _VGContext *context)
{
    gcmHEADER_ARG("os=0x%x context=0x%x",
                   os, context);

    context->matrixMode       = VG_MATRIX_PATH_USER_TO_SURFACE;
    context->fillRule         = VG_EVEN_ODD;

    context->imageQuality     = VG_IMAGE_QUALITY_FASTER;
    context->renderingQuality = VG_RENDERING_QUALITY_BETTER;
    context->blendMode        = VG_BLEND_SRC_OVER;
    context->imageMode        = VG_DRAW_IMAGE_NORMAL;

    context->scissoring       = VG_FALSE;
    context->scissorDirty     = gcvTRUE;
	context->scissorFS		  = gcvFALSE;
    context->masking          = VG_FALSE;
    context->pixelLayout      = VG_PIXEL_LAYOUT_UNKNOWN;

    context->filterFormatLinear        = VG_FALSE;
    context->filterFormatPremultiplied = VG_FALSE;
    context->filterChannelMask         = VG_RED|VG_GREEN|VG_BLUE|VG_ALPHA;

    /* Stroke parameters */
    context->strokeLineWidth      = 1.0f;
    context->strokeScale          = 1.41421f;
    context->strokeCapStyle       = VG_CAP_BUTT;
    context->strokeJoinStyle      = VG_JOIN_MITER;
    context->strokeMiterLimit     = 4.0f;
    context->strokeDashPhase      = 0.0f;
    context->strokeDashPhaseReset = VG_FALSE;
    context->error                = VG_NO_ERROR;

    COLOR_SET(context->tileFillColor, 0, 0, 0, 0, sRGBA);
    COLOR_SET(context->clearColor, 0, 0, 0, 0, sRGBA);
    COLOR_SET(context->inputTileFillColor, 0, 0, 0, 0, sRGBA);
    COLOR_SET(context->inputClearColor, 0, 0, 0, 0, sRGBA);

    _VGMatrixCtor(&context->pathUserToSurface);
    _VGMatrixCtor(&context->fillPaintToUser);
    _VGMatrixCtor(&context->imageUserToSurface);
    _VGMatrixCtor(&context->strokePaintToUser);
    _VGMatrixCtor(&context->glyphUserToSurface);
    ARRAY_CTOR(context->strokeDashPattern, os);
    ARRAY_CTOR(context->inputStrokeDashPattern, os);
    ARRAY_CTOR(context->scissor, os);


    context->postionZ             = POSITION_Z_BIAS;

    context->fillPaint            = gcvNULL;
    context->strokePaint          = gcvNULL;
    _VGPaintCtor(os, &context->defaultPaint);

    context->glyphOrigin.x        = 0.0f;
    context->glyphOrigin.y        = 0.0f;

    context->colorTransform       = VG_FALSE;
    context->inputColorTransformValues[0] = 1.0f;
    context->inputColorTransformValues[1] = 1.0f;
    context->inputColorTransformValues[2] = 1.0f;
    context->inputColorTransformValues[3] = 1.0f;
    context->inputColorTransformValues[4] = 0.0f;
    context->inputColorTransformValues[5] = 0.0f;
    context->inputColorTransformValues[6] = 0.0f;
    context->inputColorTransformValues[7] = 0.0f;
    context->colorTransformValues[0] = 1.0f;
    context->colorTransformValues[1] = 1.0f;
    context->colorTransformValues[2] = 1.0f;
    context->colorTransformValues[3] = 1.0f;
    context->colorTransformValues[4] = 0.0f;
    context->colorTransformValues[5] = 0.0f;
    context->colorTransformValues[6] = 0.0f;
    context->colorTransformValues[7] = 0.0f;
    context->vertex               = gcvNULL;

    context->os                   = gcvNULL;
    context->hal                  = gcvNULL;
    context->engine               = gcvNULL;
    context->depth                = gcvNULL;

    context->sharedData             = gcvNULL;

    /* Initialize tessellation states. */
    _VGTessellationContextCtor(os, &context->tessContext);

    _VGImageCtor(os, &context->targetImage);
    _VGImageCtor(os, &context->maskImage);

#if USE_FTA
    _VGFlattenContextCtor(os, &context->flatContext);
#endif

    _vgHARDWARECtor(&context->hardware);

    /* PSC. */
#if vgvUSE_PSC
    _PSCManagerCtor(os, &context->pscm);
#endif

    gcmFOOTER_NO();
}


void _VGContextDtor(gcoOS os, _VGContext *context)
{
    gcmHEADER_ARG("os=0x%x context=0x%x",
                   os, context);

    ARRAY_DTOR(context->strokeDashPattern);
    ARRAY_DTOR(context->inputStrokeDashPattern);
    ARRAY_DTOR(context->scissor);

    _VGPaintDtor(os, &context->defaultPaint);

    _VGImageDtor(os, &context->targetImage);
    _VGImageDtor(os, &context->maskImage);

    if (context->vertex)
    {
        gcmVERIFY_OK(gcoVERTEX_Destroy(context->vertex));
    }

    gcmVERIFY_OK(_DestroySharedData(context));

#if VIVANTE_PROFILER
    DestroyVGProfiler(context);
#endif
    /* Uninitialize tessellation states. */
    _VGTessellationContextDtor(context);

#if USE_FTA
    _VGFlattenContextDtor(os, &context->flatContext);
#endif

    _vgHARDWAREDtor(&context->hardware);

    /* PSC. */
#if vgvUSE_PSC
    _PSCManagerDtor(os, &context->pscm);
#endif

    gcmFOOTER_NO();
}


gctBOOL vgshIsScissoringEnabled(_VGContext *context)
{
    return context->scissoring && !context->scissorFS;
}

gceSTATUS vgshUpdateScissor(_VGContext *context)
{
    gceSTATUS status = gcvSTATUS_OK;

    gcmHEADER_ARG("context=0x%x", context);

    do
    {
        gctINT32 i;
        _VGRectangle s;
        _vgHARDWARE *hardware = &context->hardware;

        /* Do we need to update ?*/
		if (!context->scissorDirty || !vgshIsScissoringEnabled(context))
        {
            status = gcvSTATUS_OK;
            break;
        }

        /* use the shader to do the clear */

        hardware->dstImage          = &context->targetImage;
        hardware->masking           = gcvFALSE;
        hardware->scissoring        = gcvFALSE;
        hardware->colorTransform    = gcvFALSE;
        hardware->drawPipe          = vgvDRAWPIPE_CLEAR;
        hardware->depthCompare      = gcvCOMPARE_ALWAYS;
        hardware->depthWrite        = gcvTRUE;

        hardware->blending          = gcvFALSE;
        hardware->flush             = gcvFALSE;
        hardware->colorWrite        = 0xf;

        {
            /* do a stencil clear */
            hardware->stencilMode           = gcvSTENCIL_SINGLE_SIDED;
            hardware->stencilMask           = 0xff;
            hardware->stencilRef            = 0x0;
            hardware->stencilCompare        = gcvCOMPARE_NEVER;
            hardware->stencilFail           = gcvSTENCIL_REPLACE;

            hardware->depthMode             = gcvDEPTH_Z;

            hardware->dx                     = 0;
            hardware->dy                     = 0;
            hardware->width                 = context->targetImage.width;
            hardware->height                = context->targetImage.height;

#if NO_STENCIL_VG
            {
                hardware->stencilMode       = gcvSTENCIL_NONE;
                hardware->stencilCompare    = gcvCOMPARE_ALWAYS;
                hardware->zValue            = POSITION_Z_BIAS;
                hardware->colorWrite        = 0x0; /*No color write, should set depthOnly = true */

            }
#endif  /* #ifdef NO_STENCIL_VG */

            gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));
        }

        /* enable the stencil test */
        hardware->stencilMode       = gcvSTENCIL_SINGLE_SIDED;
        hardware->stencilMask       = 0xff;
        hardware->stencilRef        = 0xff;
        hardware->stencilCompare    = gcvCOMPARE_NEVER;
        hardware->stencilFail       = gcvSTENCIL_REPLACE;

        hardware->depthMode         = gcvDEPTH_Z;

#if NO_STENCIL_VG
        {
            hardware->stencilMode       = gcvSTENCIL_NONE;
            hardware->stencilCompare    = gcvCOMPARE_ALWAYS;
            hardware->zValue            +=  2*POSITION_Z_INTERVAL;
            context->scissorZ            = hardware->zValue;

        }
#endif  /* #ifdef NO_STENCIL_VG */

        for (i = 0; i < context->scissor.size; ++i)
        {
            s = context->scissor.items[i];
            if ((s.width > 0) && (s.height > 0))
            {
                hardware->dx                 = s.x;
                hardware->dy                 = s.y;
                hardware->width             = s.width;
                hardware->height            = s.height;

                gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));
            }
        }

        gcmERR_BREAK(gcoSURF_Flush(context->depth));

        gcmERR_BREAK(gco3D_Semaphore(context->engine,
                     gcvWHERE_RASTER,
                     gcvWHERE_PIXEL,
                     gcvHOW_SEMAPHORE_STALL));

        context->scissorDirty = gcvFALSE;
    }while(gcvFALSE);

    gcmFOOTER();
    return status;
}


static void setifv(_VGContext* context, VGParamType type, VGint count, const void* values, gctBOOL floats)
{
    gctINT32 ivalue;
    gctFLOAT fvalue;

    gctINT32 i = 0;

    gcmHEADER_ARG("context=0x%x type=0x%x count=%d values=0x%x floats=%s",
            context,
            type,
            count,
            values,
            floats ? "VG_TRUE" : "VG_FALSE");

    gcmASSERT(context);
    gcmASSERT(!count || (count && values));

    if (context == gcvNULL)
    {
        gcmFOOTER_NO();
        return;
    }

    ivalue = paramToInt(values, floats, count, 0);
    fvalue = paramToFloat(values, floats, count, 0);

    switch(type)
    {
    case VG_MATRIX_MODE:
        {
            if(count != 1 || ivalue < VG_MATRIX_PATH_USER_TO_SURFACE || ivalue > VG_MATRIX_GLYPH_USER_TO_SURFACE)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->matrixMode = (VGMatrixMode)ivalue;
        }
        break;

    case VG_FILL_RULE:
        {
            if(count != 1 || ivalue < VG_EVEN_ODD || ivalue > VG_NON_ZERO)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            if ((VGFillRule)ivalue != context->fillRule)
            {
                context->fillRule = (VGFillRule)ivalue;
            }
        }
        break;

    case VG_IMAGE_QUALITY:
        {
            if(count != 1 || ivalue < VG_IMAGE_QUALITY_NONANTIALIASED || ivalue > VG_IMAGE_QUALITY_BETTER)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->imageQuality = (VGImageQuality)ivalue;

        }
        break;

    case VG_RENDERING_QUALITY:
        {
            if(count != 1 || ivalue < VG_RENDERING_QUALITY_NONANTIALIASED || ivalue > VG_RENDERING_QUALITY_BETTER)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            if (ivalue != context->renderingQuality)
            {
                gctBOOL preAA = ((context->renderingQuality == VG_RENDERING_QUALITY_NONANTIALIASED) ? gcvFALSE : gcvTRUE);
                gctBOOL setAA = ((ivalue == VG_RENDERING_QUALITY_NONANTIALIASED)? gcvFALSE : gcvTRUE);

                context->renderingQuality = (VGRenderingQuality)ivalue;

                if (preAA != setAA)
                {
                    gcmVERIFY_OK(gco3D_SetAntiAlias(context->engine, setAA));
                    context->hardware.core.states = gcvNULL;
                }
            }
        }
        break;

    case VG_BLEND_MODE:
        {
            if(count != 1 || ivalue < VG_BLEND_SRC || ivalue > VG_BLEND_ADDITIVE)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->blendMode = (VGBlendMode)ivalue;

            if (context->targetImage.internalColorDesc.colorFormat & PREMULTIPLIED)
            {
                switch (context->blendMode)
                {
                case VG_BLEND_SRC:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_ONE, gcvBLEND_ONE));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_ZERO, gcvBLEND_ZERO));
                    }
                    break;
                case VG_BLEND_SRC_OVER:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_ONE, gcvBLEND_ONE));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_INV_SOURCE_ALPHA, gcvBLEND_INV_SOURCE_ALPHA));
                    }
                    break;
                case VG_BLEND_DST_OVER:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_INV_TARGET_ALPHA, gcvBLEND_INV_TARGET_ALPHA));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_ONE, gcvBLEND_ONE));
                    }
                    break;
                case VG_BLEND_SRC_IN:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_TARGET_ALPHA, gcvBLEND_TARGET_ALPHA));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_ZERO, gcvBLEND_ZERO));
                    }
                    break;
                case VG_BLEND_DST_IN:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_ZERO, gcvBLEND_ZERO));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_SOURCE_ALPHA, gcvBLEND_SOURCE_ALPHA));
                    }
                    break;

                case VG_BLEND_MULTIPLY:
                    break;
                case VG_BLEND_SCREEN:
                    gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                    gcvBLEND_SOURCE,
                                                    gcvBLEND_ONE, gcvBLEND_ONE));

                    gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                    gcvBLEND_TARGET,
                                                    gcvBLEND_INV_SOURCE_COLOR, gcvBLEND_INV_SOURCE_ALPHA));
                    break;
                case VG_BLEND_DARKEN:
                    break;
                case VG_BLEND_LIGHTEN:
                    break;
                case VG_BLEND_ADDITIVE:
                    gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                    gcvBLEND_SOURCE,
                                                    gcvBLEND_ONE, gcvBLEND_ONE));

                    gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                    gcvBLEND_TARGET,
                                                    gcvBLEND_ONE, gcvBLEND_ONE));
                    break;

                default:
                    gcmTRACE(gcvLEVEL_WARNING, " Current VG_BLEND_MODE 0x%x do not support", ivalue);
                    break;
                }
            }
            else
            {
                switch (context->blendMode)
                {
                case VG_BLEND_SRC:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_ONE, gcvBLEND_ONE));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_ZERO, gcvBLEND_ZERO));
                    }
                    break;
                case VG_BLEND_SRC_OVER:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_SOURCE_ALPHA, gcvBLEND_ONE));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_INV_SOURCE_ALPHA, gcvBLEND_INV_SOURCE_ALPHA));
                    }
                    break;
                case VG_BLEND_DST_OVER:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_INV_TARGET_ALPHA, gcvBLEND_INV_TARGET_ALPHA));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_TARGET_ALPHA, gcvBLEND_ONE));
                    }
                    break;
                case VG_BLEND_SRC_IN:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_ONE, gcvBLEND_ONE));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_ZERO, gcvBLEND_ZERO));
                    }
                    break;
                case VG_BLEND_DST_IN:
                    {
                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_SOURCE,
                                                        gcvBLEND_ZERO, gcvBLEND_ZERO));

                        gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                        gcvBLEND_TARGET,
                                                        gcvBLEND_ONE, gcvBLEND_SOURCE_ALPHA));
                    }
                    break;

                case VG_BLEND_MULTIPLY:
                    break;
                case VG_BLEND_SCREEN:
                    break;
                case VG_BLEND_DARKEN:
                    break;
                case VG_BLEND_LIGHTEN:
                    break;
                case VG_BLEND_ADDITIVE:
                    gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                    gcvBLEND_SOURCE,
                                                    gcvBLEND_SOURCE_ALPHA, gcvBLEND_ONE));

                    gcmVERIFY_OK(gco3D_SetBlendFunction(context->engine,
                                                    gcvBLEND_TARGET,
                                                    gcvBLEND_ONE, gcvBLEND_ONE));
                    break;

                default:
                    break;
                }
            }
        }
        break;

    case VG_IMAGE_MODE:
        {
            if(count != 1 || ivalue < VG_DRAW_IMAGE_NORMAL || ivalue > VG_DRAW_IMAGE_STENCIL)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->imageMode = (VGImageMode)ivalue;

        }
        break;

    case VG_SCISSOR_RECTS:
        {
            _VGRectangle s;

            if(count & 3)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            context->scissorDirty = gcvTRUE;

            if (0 == count)
            {
                ARRAY_CLEAR(context->scissor);
                gcmFOOTER_NO();
                return;
            }

            ARRAY_ALLOCATE(context->scissor, gcmMIN(count, MAX_SCISSOR_RECTANGLES*4));

			context->scissorFS = gcvFALSE;

            for(i = 0; i < gcmMIN(count, MAX_SCISSOR_RECTANGLES * 4); i += 4)
            {
                s.x = paramToInt(values, floats, count, i + 0);
                s.y = paramToInt(values, floats, count, i + 1);
                s.width = paramToInt(values, floats, count, i + 2);
                s.height = paramToInt(values, floats, count, i + 3);
                ARRAY_PUSHBACK(context->scissor, s);

				if ((s.x <= 0) && (s.x + s.width >= context->targetImage.width) &&
					(s.y <= 0) && (s.y + s.height >= context->targetImage.height))
				{
					context->scissorFS = gcvTRUE;
				}
            }
        }
        break;

    case VG_COLOR_TRANSFORM:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->colorTransform = ivalue ? VG_TRUE : VG_FALSE;
        }
        break;

    case VG_COLOR_TRANSFORM_VALUES:
        {
            if(count != 8 || !values)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            for(i = 0; i < 8; i++)
            {
                context->inputColorTransformValues[i] = paramToFloat(values, floats, count, i);
                context->colorTransformValues[i] = inputFloat(context->inputColorTransformValues[i]);
                context->colorTransformValues[i] = (i < 4 ? CLAMP(context->colorTransformValues[i], -127.0f, 127.0f) :
                                                            CLAMP(context->colorTransformValues[i], -1.0f, 1.0f));
            }
        }
        break;

    case VG_STROKE_LINE_WIDTH:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            if (context->strokeLineWidth != fvalue)
            {
                context->strokeLineWidth = fvalue;
            }
        }
        break;

    case VG_STROKE_CAP_STYLE:
        {
            if(count != 1 || ivalue < VG_CAP_BUTT || ivalue > VG_CAP_SQUARE)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            if ((VGCapStyle)ivalue != context->strokeCapStyle)
            {
                context->strokeCapStyle = (VGCapStyle)ivalue;
            }
        }
        break;

    case VG_STROKE_JOIN_STYLE:
        {
            if(count != 1 || ivalue < VG_JOIN_MITER || ivalue > VG_JOIN_BEVEL)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            if ((VGJoinStyle)ivalue != context->strokeJoinStyle)
            {
                context->strokeJoinStyle = (VGJoinStyle)ivalue;
            }
        }
        break;

    case VG_STROKE_MITER_LIMIT:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            if (fvalue != context->strokeMiterLimit)
            {
                context->strokeMiterLimit = fvalue;
            }
        }
        break;

    case VG_STROKE_DASH_PATTERN:
        {
            gctFLOAT v;

            count = gcmMIN(count, MAX_DASH_COUNT);
            if (count == context->strokeDashPattern.size)
            {
                gctBOOL dirty = gcvFALSE;
                for(i=0; i < count;i++)
                {
                    v = paramToFloat(values, floats, count, i);
                    if (v != context->inputStrokeDashPattern.items[i])
                    {
                        context->inputStrokeDashPattern.items[i] = v;
                        context->strokeDashPattern.items[i] = inputFloat(v);
                        dirty = gcvTRUE;
                    }
                }
                if (dirty)
                    AllPathDirty(context, (_VGTessPhase) (VGTessPhase_FlattenStroke | VGTessPhase_Stroke));
            }
            else
            {
                ARRAY_ALLOCATE(context->strokeDashPattern, count);
                ARRAY_ALLOCATE(context->inputStrokeDashPattern, count);

                if ((context->strokeDashPattern.items == gcvNULL) ||
                    (context->inputStrokeDashPattern.items == gcvNULL))
                {
                    SetError(context, VG_OUT_OF_MEMORY_ERROR);
                    gcmFOOTER_NO();
                    return;
                }

                for(i=0; i < count;i++)
                {
                    v = paramToFloat(values, floats, count, i);
                    ARRAY_PUSHBACK(context->inputStrokeDashPattern, v);
                    ARRAY_PUSHBACK(context->strokeDashPattern, inputFloat(v));
                }

                AllPathDirty(context, (_VGTessPhase) (VGTessPhase_FlattenStroke | VGTessPhase_Stroke));
            }
        }
        break;

    case VG_STROKE_DASH_PHASE:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            if (fvalue != context->strokeDashPhase)
            {
                context->strokeDashPhase = fvalue;
            }
        }
        break;

    case VG_STROKE_DASH_PHASE_RESET:
        {
            VGboolean bvalue;
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            bvalue = ivalue ? VG_TRUE : VG_FALSE;
            if (bvalue != context->strokeDashPhaseReset)
            {
                context->strokeDashPhaseReset = bvalue;
            }
        }
        break;

    case VG_TILE_FILL_COLOR:
        {
            if(count != 4 || !values)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            context->inputTileFillColor.r      = paramToFloat(values, floats, count, 0);
            context->inputTileFillColor.g      = paramToFloat(values, floats, count, 1);
            context->inputTileFillColor.b      = paramToFloat(values, floats, count, 2);
            context->inputTileFillColor.a      = paramToFloat(values, floats, count, 3);
            context->inputTileFillColor.format = sRGBA;

            COLOR_SETC(context->tileFillColor, context->inputTileFillColor);
            context->tileFillColor.r      = CLAMP(context->inputTileFillColor.r, 0.0f, 1.0f);
            context->tileFillColor.g      = CLAMP(context->inputTileFillColor.g, 0.0f, 1.0f);
            context->tileFillColor.b      = CLAMP(context->inputTileFillColor.b, 0.0f, 1.0f);
            context->tileFillColor.a      = CLAMP(context->inputTileFillColor.a, 0.0f, 1.0f);
        }
        break;

    case VG_GLYPH_ORIGIN:
        {
            if(count != 2 || !values)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->inputGlyphOrigin.x = paramToFloat(values, floats, count, 0);
            context->inputGlyphOrigin.y = paramToFloat(values, floats, count, 1);
            context->glyphOrigin = context->inputGlyphOrigin;
            break;
        }

    case VG_CLEAR_COLOR:
        {
            if(count != 4 || !values)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            context->inputClearColor.r      = paramToFloat(values, floats, count, 0);
            context->inputClearColor.g      = paramToFloat(values, floats, count, 1);
            context->inputClearColor.b      = paramToFloat(values, floats, count, 2);
            context->inputClearColor.a      = paramToFloat(values, floats, count, 3);
            context->inputClearColor.format = sRGBA;

            COLOR_SETC(context->clearColor, context->inputClearColor);

            /* - If color values outside the [0, 1] range are interpreted as the nearest endpoint of the range. */
            COLOR_CLAMP(context->clearColor);

            if (!(context->targetImage.internalColorDesc.colorFormat & NONLINEAR))
            {
                ConvertColor(&context->clearColor, lRGBA);
            }


            if (context->targetImage.internalColorDesc.colorFormat & PREMULTIPLIED)
            {
                COLOR_PREMULTIPLY(context->clearColor);
            }

            if (context->targetImage.internalColorDesc.surFormat == gcvSURF_R5G6B5)
            {
                context->clearColor.r = (VGfloat)((int)(context->clearColor.r * 31.0f + 0.5f)) / 31.0f;
                context->clearColor.g = (VGfloat)((int)(context->clearColor.g * 63.0f + 0.5f)) / 63.0f;
                context->clearColor.b = (VGfloat)((int)(context->clearColor.b * 31.0f + 0.5f)) / 31.0f;
            }

            gcmVERIFY_OK(gco3D_SetClearColorF(context->engine,
                                              context->clearColor.r,
                                              context->clearColor.g,
                                              context->clearColor.b,
                                              context->clearColor.a));
        }
        break;

    case VG_MASKING:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->masking = ivalue ? VG_TRUE : VG_FALSE;
        }
        break;

    case VG_SCISSORING:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->scissoring = ivalue ? VG_TRUE : VG_FALSE;
        }
        break;

    case VG_PIXEL_LAYOUT:
        {
            if(count != 1 || ivalue < VG_PIXEL_LAYOUT_UNKNOWN || ivalue > VG_PIXEL_LAYOUT_BGR_HORIZONTAL)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->pixelLayout = (VGPixelLayout)ivalue;
            gcmTRACE(gcvLEVEL_WARNING, " VG_PIXEL_LAYOUT do not support");
        }
        break;

    case VG_SCREEN_LAYOUT:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            gcmTRACE(gcvLEVEL_WARNING, " VG_SCREEN_LAYOUT do not support");
        }
        break;

    case VG_FILTER_FORMAT_LINEAR:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->filterFormatLinear = ivalue ? VG_TRUE : VG_FALSE;
            gcmTRACE(gcvLEVEL_WARNING, " VG_FILTER_FORMAT_LINEAR do not support");
        }
        break;

    case VG_FILTER_FORMAT_PREMULTIPLIED:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->filterFormatPremultiplied = ivalue ? VG_TRUE : VG_FALSE;

            gcmTRACE(gcvLEVEL_WARNING, " VG_FILTER_FORMAT_PREMULTIPLIED do not support");
        }
        break;

    case VG_FILTER_CHANNEL_MASK:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            context->filterChannelMask = (VGbitfield)ivalue;
        }
        break;

    case VG_MAX_SCISSOR_RECTS:
    case VG_MAX_DASH_COUNT:
    case VG_MAX_KERNEL_SIZE:
    case VG_MAX_SEPARABLE_KERNEL_SIZE:
    case VG_MAX_COLOR_RAMP_STOPS:
    case VG_MAX_IMAGE_WIDTH:
    case VG_MAX_IMAGE_HEIGHT:
    case VG_MAX_IMAGE_PIXELS:
    case VG_MAX_IMAGE_BYTES:
    case VG_MAX_FLOAT:
    case VG_MAX_GAUSSIAN_STD_DEVIATION:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
        }
        break;

    default:
        {
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
        }
        break;
    }

    gcmFOOTER_NO();
}

void  vgSetf(VGParamType type, VGfloat value)
{
    _VGContext* context;
    gcmHEADER_ARG("vgSetf type = %x, value = %f", type, value);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETF, 0);

    OVG_IF_ERROR(type == VG_SCISSOR_RECTS || type == VG_STROKE_DASH_PATTERN || type == VG_TILE_FILL_COLOR ||
                type == VG_CLEAR_COLOR, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    setifv(context, type, 1, &value, gcvTRUE);

    gcmFOOTER_NO();
}

void  vgSeti(VGParamType type, VGint value)
{
    _VGContext* context;
    gcmHEADER_ARG("type=0x%x value=0x%x", type, value);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETI, 0);

    OVG_IF_ERROR(type == VG_SCISSOR_RECTS || type == VG_STROKE_DASH_PATTERN || type == VG_TILE_FILL_COLOR ||
                type == VG_CLEAR_COLOR, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    setifv(context, type, 1, &value, gcvFALSE);

    gcmFOOTER_NO();
}

void  vgSetiv(VGParamType type, VGint count, const VGint * values)
{
    _VGContext* context;
    gcmHEADER_ARG("type=0x%x count=0x%x values=0x%x", type, count, values);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETIV, 0);

    OVG_IF_ERROR(count < 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR((!values && count > 0) || (values && !isAligned(values,4)), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    setifv(context, type, count, values, gcvFALSE);

    gcmFOOTER_NO();
}

void  vgSetfv(VGParamType type, VGint count, const VGfloat * values)
{
    _VGContext* context;
    gcmHEADER_ARG("type=0x%x count=%d values=0x%x", type, count, values);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETFV, 0);

    OVG_IF_ERROR(count < 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR((!values && count > 0) || (values && !isAligned(values,4)), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    setifv(context, type, count, values, gcvTRUE);

    gcmFOOTER_NO();
}


static void getifv(_VGContext* context, VGParamType type, VGint count, void* values, gctBOOL floats)
{
    gctINT32 i;

    gcmHEADER_ARG("context=0x%x type=0x%x count=%d values=0x%x floats=%s",
                  context, type, count, values,
                  floats ? "TRUE" : "FALSE");

    switch(type)
    {
    case VG_MATRIX_MODE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->matrixMode);
        }
        break;

    case VG_FILL_RULE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->fillRule);
        }
        break;

    case VG_IMAGE_QUALITY:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->imageQuality);
        }
        break;

    case VG_RENDERING_QUALITY:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->renderingQuality);
        }
        break;

    case VG_BLEND_MODE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->blendMode);
        }
        break;

    case VG_IMAGE_MODE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->imageMode);
        }
        break;

    case VG_SCISSOR_RECTS:
        {
            if(count > context->scissor.size * 4)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            for(i = 0; i< context->scissor.size; i++)
            {
                intToParam(values, floats, count, i*4+0, (VGint)context->scissor.items[i].x);
                intToParam(values, floats, count, i*4+1, (VGint)context->scissor.items[i].y);
                intToParam(values, floats, count, i*4+2, (VGint)context->scissor.items[i].width);
                intToParam(values, floats, count, i*4+3, (VGint)context->scissor.items[i].height);
            }
        }
        break;

    case VG_COLOR_TRANSFORM:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->colorTransform);
        }
        break;

    case VG_COLOR_TRANSFORM_VALUES:
        {
            if(count > 8)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            for (i = 0; i < count; i++)
            {
                floatToParam(values, floats, count, i, context->inputColorTransformValues[i]);
            }
        }
        break;

    case VG_STROKE_LINE_WIDTH:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, context->strokeLineWidth);
        }
        break;

    case VG_STROKE_CAP_STYLE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->strokeCapStyle);
        }
        break;

    case VG_STROKE_JOIN_STYLE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->strokeJoinStyle);
        }
        break;

    case VG_STROKE_MITER_LIMIT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, context->strokeMiterLimit);
        }
        break;

    case VG_STROKE_DASH_PATTERN:
        {
            if(count > context->inputStrokeDashPattern.size)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            for(i=0; i<context->inputStrokeDashPattern.size; i++)
                floatToParam(values, floats, count, i, context->inputStrokeDashPattern.items[i]);
        }
        break;

    case VG_STROKE_DASH_PHASE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, context->strokeDashPhase);
        }
        break;

    case VG_STROKE_DASH_PHASE_RESET:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->strokeDashPhaseReset);
        }
        break;

    case VG_TILE_FILL_COLOR:
        {
            if(count > 4)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, context->inputTileFillColor.r);
            floatToParam(values, floats, count, 1, context->inputTileFillColor.g);
            floatToParam(values, floats, count, 2, context->inputTileFillColor.b);
            floatToParam(values, floats, count, 3, context->inputTileFillColor.a);
        }
        break;

    case VG_CLEAR_COLOR:
        {
            if(count > 4)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, context->inputClearColor.r);
            floatToParam(values, floats, count, 1, context->inputClearColor.g);
            floatToParam(values, floats, count, 2, context->inputClearColor.b);
            floatToParam(values, floats, count, 3, context->inputClearColor.a);
        }
        break;

    case VG_MASKING:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->masking);
        }
        break;

    case VG_SCISSORING:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->scissoring);
        }
        break;

    case VG_PIXEL_LAYOUT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->pixelLayout);
        }
        break;

    case VG_SCREEN_LAYOUT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, VG_PIXEL_LAYOUT_UNKNOWN);
        }
        break;

    case VG_FILTER_FORMAT_LINEAR:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->filterFormatLinear);
        }
        break;

    case VG_FILTER_FORMAT_PREMULTIPLIED:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->filterFormatPremultiplied);
        }
        break;

    case VG_FILTER_CHANNEL_MASK:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, context->filterChannelMask);
        }
        break;

    case VG_MAX_SCISSOR_RECTS:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_SCISSOR_RECTANGLES);
        }
        break;

    case VG_MAX_DASH_COUNT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_DASH_COUNT);
        }
        break;

    case VG_MAX_KERNEL_SIZE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_KERNEL_SIZE);
        }
        break;

    case VG_MAX_SEPARABLE_KERNEL_SIZE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_SEPARABLE_KERNEL_SIZE);
        }
        break;

    case VG_MAX_COLOR_RAMP_STOPS:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_COLOR_RAMP_STOPS);
        }
        break;

    case VG_MAX_IMAGE_WIDTH:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_IMAGE_WIDTH);
        }
        break;

    case VG_MAX_IMAGE_HEIGHT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_IMAGE_HEIGHT);
        }
        break;

    case VG_MAX_IMAGE_PIXELS:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_IMAGE_PIXELS);
        }
        break;

    case VG_MAX_IMAGE_BYTES:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, MAX_IMAGE_BYTES);
        }
        break;

    case VG_MAX_FLOAT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, FLOAT_MAX);
        }
        break;

    case VG_MAX_GAUSSIAN_STD_DEVIATION:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, MAX_GAUSSIAN_STD_DEVIATION);
        }
        break;

    case VG_GLYPH_ORIGIN:
        {
            if(count > 2)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, context->glyphOrigin.x);
            floatToParam(values, floats, count, 1, context->glyphOrigin.y);
        }
        break;

    default:
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
        break;
    }

    gcmFOOTER_NO();
}

VGfloat  vgGetf(VGParamType type)
{
    gctFLOAT ret = 0.0f;
    _VGContext* context;
    gcmHEADER_ARG("type=0x%x", type);
    OVG_GET_CONTEXT(0.0f);

    vgmPROFILE(context, VGGETF, 0);

    OVG_IF_ERROR(type == VG_SCISSOR_RECTS || type == VG_STROKE_DASH_PATTERN || type == VG_TILE_FILL_COLOR ||
                type == VG_CLEAR_COLOR, VG_ILLEGAL_ARGUMENT_ERROR, 0.0f);

    getifv(context, type, 1, &ret, gcvTRUE);

    gcmFOOTER_ARG("return=%f", ret);
    return ret;
}

VGint  vgGeti(VGParamType type)
{
    VGint ret = 0;
    _VGContext* context;
    gcmHEADER_ARG("type=0x%x", type);
    OVG_GET_CONTEXT(0);

    vgmPROFILE(context, VGGETI, 0);

    OVG_IF_ERROR(type == VG_SCISSOR_RECTS || type == VG_STROKE_DASH_PATTERN || type == VG_TILE_FILL_COLOR ||
                type == VG_CLEAR_COLOR, VG_ILLEGAL_ARGUMENT_ERROR, 0);

    getifv(context, type, 1, &ret, gcvFALSE);

    gcmFOOTER_ARG("return=%d", ret);
    return ret;
}


void  vgGetiv(VGParamType type, VGint count, VGint * values)
{
    _VGContext* context;
    gcmHEADER_ARG("type=0x%x count=%d values=0x%x", type, count, values);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGGETIV, 0);

    OVG_IF_ERROR(count <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!values || !isAligned(values,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    getifv(context, type, count, values, gcvFALSE);

    gcmFOOTER_NO();
}

void  vgGetfv(VGParamType type, VGint count, VGfloat * values)
{
    _VGContext* context;
    gcmHEADER_ARG("type=0x%x count=%d values=0x%x", type, count, values);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGGETFV, 0);

    OVG_IF_ERROR(count <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!values || !isAligned(values,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    getifv(context, type, count, values, gcvTRUE);

    gcmFOOTER_NO();
}


VGint  vgGetVectorSize(VGParamType type)
{
    VGint ret = 0;
    _VGContext* context;
    gcmHEADER_ARG("type=0x%x", type);

    OVG_GET_CONTEXT(0);

    vgmPROFILE(context, VGGETVECTORSIZE, 0);

    switch(type)
    {
    case VG_MATRIX_MODE:
    case VG_FILL_RULE:
    case VG_IMAGE_QUALITY:
    case VG_RENDERING_QUALITY:
    case VG_BLEND_MODE:
    case VG_IMAGE_MODE:
        ret = 1;
        break;

    case VG_SCISSOR_RECTS:
        ret = 4*context->scissor.size;
        break;

    case VG_COLOR_TRANSFORM:
        ret = 1;
        break;

    case VG_COLOR_TRANSFORM_VALUES:
        ret = 8;
        break;

    case VG_STROKE_LINE_WIDTH:
    case VG_STROKE_CAP_STYLE:
    case VG_STROKE_JOIN_STYLE:
    case VG_STROKE_MITER_LIMIT:
        ret = 1;
        break;

    case VG_STROKE_DASH_PATTERN:
        ret = context->strokeDashPattern.size;
        break;

    case VG_STROKE_DASH_PHASE:
    case VG_STROKE_DASH_PHASE_RESET:
        ret = 1;
        break;

    case VG_TILE_FILL_COLOR:
    case VG_CLEAR_COLOR:
        ret = 4;
        break;

    case VG_GLYPH_ORIGIN:
        ret = 2;
        break;

    case VG_MASKING:
    case VG_SCISSORING:
    case VG_PIXEL_LAYOUT:
    case VG_SCREEN_LAYOUT:
    case VG_FILTER_FORMAT_LINEAR:
    case VG_FILTER_FORMAT_PREMULTIPLIED:
    case VG_FILTER_CHANNEL_MASK:
    case VG_MAX_SCISSOR_RECTS:
    case VG_MAX_DASH_COUNT:
    case VG_MAX_KERNEL_SIZE:
    case VG_MAX_SEPARABLE_KERNEL_SIZE:
    case VG_MAX_COLOR_RAMP_STOPS:
    case VG_MAX_IMAGE_WIDTH:
    case VG_MAX_IMAGE_HEIGHT:
    case VG_MAX_IMAGE_PIXELS:
    case VG_MAX_IMAGE_BYTES:
    case VG_MAX_FLOAT:
    case VG_MAX_GAUSSIAN_STD_DEVIATION:
        ret = 1;
        break;

    default:
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
        break;
    }

    gcmFOOTER_ARG("return=%d", ret);
    return ret;
}

gceTEXTURE_ADDRESSING
_vgSpreadMode2gcMode(
    VGColorRampSpreadMode Mode
    )
{
    gcmHEADER_ARG("Mode=%x", Mode);

    switch (Mode)
    {
    case VG_COLOR_RAMP_SPREAD_REPEAT:
        gcmFOOTER_ARG("return=%d", gcvTEXTURE_WRAP);
        return gcvTEXTURE_WRAP;

    case VG_COLOR_RAMP_SPREAD_PAD:
        gcmFOOTER_ARG("return=%d", gcvTEXTURE_CLAMP);
        return gcvTEXTURE_CLAMP;

    case VG_COLOR_RAMP_SPREAD_REFLECT:
        gcmFOOTER_ARG("return=%d", gcvTEXTURE_MIRROR);
        return gcvTEXTURE_MIRROR;
    default:
        break;
    }

    gcmBREAK();
    gcmFOOTER_ARG("return=%d", -1);

    return gcvTEXTURE_WRAP;     /* Return a default mode. */
}

gceTEXTURE_ADDRESSING
_vgTillingMode2gcMode(
    VGTilingMode Mode
    )
{
    gcmHEADER_ARG("Mode=0x%x", Mode);

    switch (Mode)
    {
    case VG_TILE_REPEAT:
        gcmFOOTER_ARG("return=%d", gcvTEXTURE_WRAP);
        return gcvTEXTURE_WRAP;

    case VG_TILE_PAD:
        gcmFOOTER_ARG("return=%d", gcvTEXTURE_CLAMP);
        return gcvTEXTURE_CLAMP;

    case VG_TILE_REFLECT:
        gcmFOOTER_ARG("return=%d", gcvTEXTURE_MIRROR);
        return gcvTEXTURE_MIRROR;

    case VG_TILE_FILL:
        gcmTRACE(gcvLEVEL_WARNING, "VG_TILE_FILL mode do not support");
        gcmFOOTER_ARG("return=%d", gcvTEXTURE_CLAMP);
        return gcvTEXTURE_CLAMP;
        /*return gcvTEXTURE_BORDER;*/
    default:
        break;
    }

    gcmBREAK();
    gcmFOOTER_ARG("return=%d", -1);

    return gcvTEXTURE_WRAP;     /* Return a default value. */
}

_VGColor integrateColorRamp(_VGPaint *paint, gctFLOAT gmin, gctFLOAT gmax)
{
    _VGColor c;
    gctINT32 i = 0;

    gcmHEADER_ARG("paint=0x%x gmin=%f gmax=%f", paint, gmin, gmax);

    gcmASSERT(gmin <= gmax);
    gcmASSERT(gmin >= 0.0f && gmin <= 1.0f);
    gcmASSERT(gmax >= 0.0f && gmax <= 1.0f);
    gcmASSERT(paint->colorRampStops.size >= 2);

    COLOR_SET(c, 0, 0, 0, 0, paint->colorRampPremultiplied ? sRGBA_PRE : sRGBA)
    if(gmin == 1.0f || gmax == 0.0f)
    {
        gcmFOOTER_ARG("return={%f, %f, %f, %f, %d}", c.r, c.g, c.b, c.a, c.format);
        return c;
    }

    for(;i<paint->colorRampStops.size-1;i++)
    {
        if(gmin >= paint->colorRampStops.items[i].offset && gmin < paint->colorRampStops.items[i+1].offset)
        {
            gctFLOAT s, e, g;
            _VGColor sc, ec, rc;
            s = paint->colorRampStops.items[i].offset;
            e = paint->colorRampStops.items[i+1].offset;
            gcmASSERT(s < e);
            g = (gmin - s) / (e - s);

            sc = paint->colorRampStops.items[i].color;
            ec = paint->colorRampStops.items[i+1].color;
            rc.r = (1.0f-g) * sc.r + g * ec.r;
            rc.g = (1.0f-g) * sc.g + g * ec.g;
            rc.b = (1.0f-g) * sc.b + g * ec.b;
            rc.a = (1.0f-g) * sc.a + g * ec.a;
            rc.format = sc.format;

            /*subtract the average color from the start of the stop to gmin*/
            c.r -= 0.5f*(gmin - s) * (sc.r + rc.r);
            c.g -= 0.5f*(gmin - s) * (sc.g + rc.g);
            c.b -= 0.5f*(gmin - s) * (sc.b + rc.b);
            c.a -= 0.5f*(gmin - s) * (sc.a + rc.a);
            break;
        }
    }

    for(;i<paint->colorRampStops.size-1;i++)
    {
        gctFLOAT s, e;
        _VGColor sc, ec;
        s = paint->colorRampStops.items[i].offset;
        e = paint->colorRampStops.items[i+1].offset;
        gcmASSERT(s <= e);

        sc = paint->colorRampStops.items[i].color;
        ec = paint->colorRampStops.items[i+1].color;

        /*average of the stop*/
        c.r += 0.5f*(e-s)*(sc.r + ec.r);
        c.g += 0.5f*(e-s)*(sc.g + ec.g);
        c.b += 0.5f*(e-s)*(sc.b + ec.b);
        c.a += 0.5f*(e-s)*(sc.a + ec.a);

        if(gmax >= paint->colorRampStops.items[i].offset && gmax < paint->colorRampStops.items[i+1].offset)
        {
            gctFLOAT g = (gmax - s) / (e - s);
            _VGColor rc;
            rc.r = (1.0f-g) * sc.r + g * ec.r;
            rc.g = (1.0f-g) * sc.g + g * ec.g;
            rc.b = (1.0f-g) * sc.b + g * ec.b;
            rc.a = (1.0f-g) * sc.a + g * ec.a;
            rc.format = sc.format;

            /*subtract the average color from gmax to the end of the stop*/
            c.r -= 0.5f*(e - gmax)*(rc.r + ec.r);
            c.g -= 0.5f*(e - gmax)*(rc.g + ec.g);
            c.b -= 0.5f*(e - gmax)*(rc.b + ec.b);
            c.a -= 0.5f*(e - gmax)*(rc.a + ec.a);
            break;
        }
    }

    gcmFOOTER_ARG("return={%f, %f, %f, %f, %d}", c.r, c.g, c.b, c.a, c.format);
    return c;
}

static void setPaintParameterifv(_VGContext* context, _VGPaint* paint, VGPaintParamType paramType,
                                 VGint count, const void* values, gctBOOL floats)
{
    gctINT32            ivalue;
    _VGGradientStop     gs;
    gctINT32            numStops = 0;
    gctFLOAT            prevOffset  = -FLOAT_MAX;
    gctBOOL             valid = gcvTRUE;
    gctINT32            i = 0;

    gcmHEADER_ARG("context=0x%x paint=0x%x paramType=0x%x count=%d values=0x%x floats=%s",
                  context, paint, paramType, count, values,
                  floats ? "TRUE" : "FALSE");

    gcmASSERT(context);
    gcmASSERT(paint);

    ivalue = paramToInt(values, floats, count, 0);

    switch(paramType)
    {
    case VG_PAINT_TYPE:
        {
            if(count != 1 || ivalue < VG_PAINT_TYPE_COLOR || ivalue > VG_PAINT_TYPE_PATTERN)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            if ((VGPaintType)ivalue == paint->paintType)
                break;
            paint->paintType = (VGPaintType)ivalue;
            /*PaintDirty(paint);*/
        }
        break;

    case VG_PAINT_COLOR:
        {
            if(count != 4)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            paint->inputPaintColor.r      = paramToFloat(values, floats, count, 0);
            paint->inputPaintColor.g      = paramToFloat(values, floats, count, 1);
            paint->inputPaintColor.b      = paramToFloat(values, floats, count, 2);
            paint->inputPaintColor.a      = paramToFloat(values, floats, count, 3);
            paint->inputPaintColor.format = sRGBA;

            paint->paintColor = paint->inputPaintColor;

            COLOR_CLAMP(paint->paintColor);
        }
        break;

    case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
        {
            if(count != 1 || ivalue < VG_COLOR_RAMP_SPREAD_PAD || ivalue > VG_COLOR_RAMP_SPREAD_REFLECT)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            paint->colorRampSpreadMode = (VGColorRampSpreadMode)ivalue;

            if (paint->colorRamp.texture)
            {
                paint->colorRamp.texStates.s =
                    _vgSpreadMode2gcMode(paint->colorRampSpreadMode);
            }
        }
        break;

    case VG_PAINT_COLOR_RAMP_STOPS:
        {
            numStops = count/5;
            if(count != numStops*5)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }

            ARRAY_ALLOCATE(paint->inputColorRampStops,  gcmMIN(numStops, MAX_COLOR_RAMP_STOPS));
            ARRAY_ALLOCATE(paint->colorRampStops,  gcmMIN(numStops + 2, MAX_COLOR_RAMP_STOPS));

            if ((numStops > 0) &&
                ((paint->inputColorRampStops.items == gcvNULL) ||
                (paint->colorRampStops.items == gcvNULL)))
            {
                SetError(context, VG_OUT_OF_MEMORY_ERROR);
                gcmFOOTER_NO();
                return;
            }

            for(i=0;i<gcmMIN(numStops, MAX_COLOR_RAMP_STOPS);i++)
            {
                gs.offset = paramToFloat(values, floats, count, i*5);
                COLOR_SET(gs.color,
                        paramToFloat(values, floats, count, i*5+1),
                        paramToFloat(values, floats, count, i*5+2),
                        paramToFloat(values, floats, count, i*5+3),
                        paramToFloat(values, floats, count, i*5+4),
                        sRGBA);

                ARRAY_PUSHBACK(paint->inputColorRampStops, gs);

                if(gs.offset < prevOffset)
                        valid = gcvFALSE;

                if(gs.offset >= 0.0f && gs.offset <= 1.0f)
                {
                    COLOR_CLAMP(gs.color);

                    if(!paint->colorRampStops.size && gs.offset > 0.0f)
                    {
                        gctFLOAT tmp = gs.offset;
                        gs.offset = 0.0f;
                        ARRAY_PUSHBACK(paint->colorRampStops, gs);
                        gs.offset = tmp;
                    }
                    ARRAY_PUSHBACK(paint->colorRampStops, gs);
                }
                prevOffset = gs.offset;
            }
            if(valid && paint->colorRampStops.size && paint->colorRampStops.items[paint->colorRampStops.size - 1].offset < 1.0f)
            {
                gs = paint->colorRampStops.items[paint->colorRampStops.size - 1];
                gs.offset = 1.0f;
                ARRAY_PUSHBACK(paint->colorRampStops, gs);
            }
            if(!valid || !paint->colorRampStops.size)
            {
                ARRAY_CLEAR(paint->colorRampStops);

                gs.offset = 0.0f;
                COLOR_SET(gs.color, 0, 0, 0, 1, sRGBA);
                ARRAY_PUSHBACK(paint->colorRampStops, gs);
                gs.offset = 1.0f;
                COLOR_SET(gs.color, 1, 1, 1, 1, sRGBA);
                ARRAY_PUSHBACK(paint->colorRampStops, gs);
            }
            gcmASSERT(paint->colorRampStops.size >= 2 && paint->colorRampStops.size <= MAX_COLOR_RAMP_STOPS);

            PaintDirty(paint, OVGPaintDirty_ColorStop);
        }
        break;

    case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
        {
            if(count != 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            paint->colorRampPremultiplied = ivalue ? VG_TRUE : VG_FALSE;

            PaintDirty(paint, OVGPaintDirty_ColorRampPremultiplied);
        }
        break;

    case VG_PAINT_LINEAR_GRADIENT:
        {
            gctFLOAT u;
            _VGVector2 v2;

            if(count != 4)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            paint->linearGradient[0] = paramToFloat(values, floats, count, 0);
            paint->linearGradient[1] = paramToFloat(values, floats, count, 1);
            paint->linearGradient[2] = paramToFloat(values, floats, count, 2);
            paint->linearGradient[3] = paramToFloat(values, floats, count, 3);

            V2_SET(v2, paint->linearGradient[2] - paint->linearGradient[0],
                       paint->linearGradient[3] - paint->linearGradient[1]);
            u = V2_DOT(v2, v2);
            if (u == 0.0f)
            {
                paint->zeroFlag = gcvTRUE;
                break;
            }
            else
            {
                paint->zeroFlag = gcvFALSE;
            }

            V2_DIV(v2, u);
            V2_SETV(paint->udusq, v2);
        }
        break;

    case VG_PAINT_RADIAL_GRADIENT:
        {
            if(count != 5)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            paint->inputRadialGradient[0] = paramToFloat(values, floats, count, 0);
            paint->inputRadialGradient[1] = paramToFloat(values, floats, count, 1);
            paint->inputRadialGradient[2] = paramToFloat(values, floats, count, 2);
            paint->inputRadialGradient[3] = paramToFloat(values, floats, count, 3);
            paint->inputRadialGradient[4] = paramToFloat(values, floats, count, 4);

            OVG_MEMCOPY(paint->radialGradient, paint->inputRadialGradient, 5* sizeof(paint->radialGradient[0]));

            {
                _VGVector2 fp;
                gctFLOAT   C, rsq, D, sqrtD, fpxsq, fpysq;
                gctFLOAT len;

                rsq =  paint->radialGradient[4] * paint->radialGradient[4];

                V2_SET(fp, paint->radialGradient[2] - paint->radialGradient[0],
                           paint->radialGradient[3] - paint->radialGradient[1]);

                len = gcoMATH_SquareRoot(fp.x * fp.x + fp.y * fp.y);
                if (len > 0.999f * paint->radialGradient[4])
                {
                    fp.x *= 0.999f * paint->radialGradient[4] / len;
                    fp.y *= 0.999f * paint->radialGradient[4] / len;
                    paint->radialGradient[2] = fp.x + paint->radialGradient[0];
                    paint->radialGradient[3] = fp.y + paint->radialGradient[1];
                }

                fpxsq = fp.x * fp.x;
                fpysq = fp.y * fp.y;

                C = rsq - (fpxsq + fpysq);

                if (C == 0.0f)
                {
                    paint->zeroFlag = gcvTRUE;
                    break;
                }
                else
                {
                    paint->zeroFlag = gcvFALSE;
                }

                V2_SET(paint->radialA, fp.x / C, fp.y / C);

                D = rsq - fpysq;
                sqrtD = gcoMATH_SquareRoot(D);

                V2_SET(paint->radialB, sqrtD/C, (fp.x * fp.y) / (sqrtD*C));
                paint->radialC= gcoMATH_SquareRoot(rsq - fpxsq - fpxsq * fpysq /D) / C;
            }
        }
        break;

    case VG_PAINT_PATTERN_TILING_MODE:
        {
            if(count != 1 || ivalue < VG_TILE_FILL || ivalue > VG_TILE_REFLECT)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            paint->patternTilingMode = (VGTilingMode)ivalue;
        }
        break;

    default:
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
        break;
    }

    gcmFOOTER_NO();
}


void  vgSetParameterf(VGHandle object, VGint paramType, VGfloat value)
{
    _VGImage        *image;
    _VGPath         *path;
    _VGPaint        *paint;
    _VGFont         *font;
    _VGMaskLayer    *maskLayer;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x value=%f", object, paramType, value);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETPARAMETERF, 0);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);
    maskLayer = _VGMASKLAYER(context, object);
    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !maskLayer && !font, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(paramType == VG_PAINT_COLOR || paramType == VG_PAINT_COLOR_RAMP_STOPS || paramType == VG_PAINT_LINEAR_GRADIENT ||
                paramType == VG_PAINT_RADIAL_GRADIENT, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    if(image)
    {
        gcmASSERT(!path && !paint && !maskLayer && !font);
        if(paramType < VG_IMAGE_FORMAT || paramType > VG_IMAGE_HEIGHT)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if(path)
    {
        gcmASSERT(!image && !paint && !maskLayer && !font);
        if(paramType < VG_PATH_FORMAT || paramType > VG_PATH_NUM_COORDS)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if (font)
    {
        gcmASSERT(!image && !path && !paint && !maskLayer);
        if (paramType != VG_FONT_NUM_GLYPHS)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if (maskLayer)
    {
        gcmASSERT(!image && !path && !paint && !font);
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else
    {
        gcmASSERT(!image && !path && paint && !maskLayer && !font);
        setPaintParameterifv(context, paint, (VGPaintParamType)paramType, 1, &value, gcvTRUE);
    }

    gcmFOOTER_NO();
}


void  vgSetParameteri(VGHandle object, VGint paramType, VGint value)
{
    _VGImage  *image;
    _VGPath   *path;
    _VGPaint  *paint;
    _VGFont         *font;
    _VGMaskLayer    *maskLayer;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x value=%d", object, paramType, value);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETPARAMETERI, 0);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);

    maskLayer = _VGMASKLAYER(context, object);
    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !maskLayer && !font, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    OVG_IF_ERROR(paramType == VG_PAINT_COLOR || paramType == VG_PAINT_COLOR_RAMP_STOPS || paramType == VG_PAINT_LINEAR_GRADIENT ||
                paramType == VG_PAINT_RADIAL_GRADIENT, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    if(image)
    {
        gcmASSERT(!path && !paint &&!font && !maskLayer);
        if(paramType < VG_IMAGE_FORMAT || paramType > VG_IMAGE_HEIGHT)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if(path)
    {

        gcmASSERT(!image && !paint &&!font && !maskLayer);
        if(paramType < VG_PATH_FORMAT || paramType > VG_PATH_NUM_COORDS)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if (font)
    {
        gcmASSERT(!image && !path && !paint && !maskLayer);
        if (paramType != VG_FONT_NUM_GLYPHS)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if (maskLayer)
    {
        gcmASSERT(!image && !path && !paint && !font);
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }

    else
    {
        gcmASSERT(!image && !path &&!font && !maskLayer && paint);
        setPaintParameterifv(context, paint, (VGPaintParamType)paramType, 1, &value, gcvFALSE);
    }

    gcmFOOTER_NO();
}


void  vgSetParameterfv(VGHandle object, VGint paramType, VGint count, const VGfloat * values)
{
    _VGImage  *image;
    _VGPath   *path;
    _VGPaint  *paint;
    _VGFont         *font;
    _VGMaskLayer    *maskLayer;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x count=%d values=0x%x", object, paramType, count, values);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETPARAMETERFV, 0);
    OVG_IF_ERROR(count < 0 || (!values && count > 0) || (values && !isAligned(values,4)), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);
    maskLayer = _VGMASKLAYER(context, object);
    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !maskLayer && !font, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    if(image)
    {
        gcmASSERT(!paint && !path &&!font && !maskLayer);
        if(paramType < VG_IMAGE_FORMAT || paramType > VG_IMAGE_HEIGHT)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if(path)
    {
        gcmASSERT(!paint && !image &&!font && !maskLayer);
        if(paramType < VG_PATH_FORMAT || paramType > VG_PATH_NUM_COORDS)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if (font)
    {
        gcmASSERT(!image && !path && !paint && !maskLayer);
        if (paramType != VG_FONT_NUM_GLYPHS)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if (maskLayer)
    {
        gcmASSERT(!image && !path && !paint && !font);
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else
    {
        gcmASSERT(!font && !image && !path && !maskLayer && paint);
        setPaintParameterifv(context, paint, (VGPaintParamType)paramType, count, values, gcvTRUE);
    }

    gcmFOOTER_NO();
}

void  vgSetParameteriv(VGHandle object, VGint paramType, VGint count, const VGint * values)
{
    _VGImage        *image;
    _VGPath         *path;
    _VGPaint        *paint;
    _VGFont         *font;
    _VGMaskLayer    *maskLayer;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x count=%d values=0x%x", object, paramType, count, values);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETPARAMETERIV, 0);
    OVG_IF_ERROR(count < 0 || (!values && count > 0) || (values && !isAligned(values,4)), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);

    maskLayer = _VGMASKLAYER(context, object);
    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !maskLayer && !font, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    if(image)
    {
        gcmASSERT(!font && !path && !paint && !maskLayer);
        if(paramType < VG_IMAGE_FORMAT || paramType > VG_IMAGE_HEIGHT)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if(path)
    {
        gcmASSERT(!font && !image && !paint && !maskLayer);
        if(paramType < VG_PATH_FORMAT || paramType > VG_PATH_NUM_COORDS)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if (font)
    {
        gcmASSERT(!image && !path && !paint && !maskLayer);
        if (paramType != VG_FONT_NUM_GLYPHS)
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else if (maskLayer)
    {
        gcmASSERT(!image && !path && !paint && !font);
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
    }
    else
    {
        gcmASSERT(!font && !image && !path && !maskLayer && paint);
        setPaintParameterifv(context, paint, (VGPaintParamType)paramType, count, values, gcvFALSE);
    }

    gcmFOOTER_NO();
}

static void getPaintParameterifv(_VGContext* context, _VGPaint* paint, VGPaintParamType type,
                                 VGint count, void* values, gctBOOL floats)
{
    gctINT32 i, j;

    gcmHEADER_ARG("context=0x%x paint=0x%x type=0x%x count=%d values=0x%x values=%s",
                  context, paint, type, count, values,
                  floats ? "TRUE" : "FALSE");
    switch(type)
    {
    case VG_PAINT_TYPE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, paint->paintType);
        }
        break;

    case VG_PAINT_COLOR:
        {
            if(count > 4)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, paint->inputPaintColor.r);
            floatToParam(values, floats, count, 1, paint->inputPaintColor.g);
            floatToParam(values, floats, count, 2, paint->inputPaintColor.b);
            floatToParam(values, floats, count, 3, paint->inputPaintColor.a);
        }
        break;

    case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, paint->colorRampSpreadMode);
        }
        break;

    case VG_PAINT_COLOR_RAMP_STOPS:
        {
            if(count > paint->inputColorRampStops.size*5)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            j = 0;
            for(i=0;i<paint->inputColorRampStops.size;i++)
            {
                floatToParam(values, floats, count, j++, paint->inputColorRampStops.items[i].offset);
                floatToParam(values, floats, count, j++, paint->inputColorRampStops.items[i].color.r);
                floatToParam(values, floats, count, j++, paint->inputColorRampStops.items[i].color.g);
                floatToParam(values, floats, count, j++, paint->inputColorRampStops.items[i].color.b);
                floatToParam(values, floats, count, j++, paint->inputColorRampStops.items[i].color.a);
            }
        }
        break;

    case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, paint->colorRampPremultiplied);
        }
        break;

    case VG_PAINT_LINEAR_GRADIENT:
        {
            if(count > 4)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, paint->linearGradient[0]);
            floatToParam(values, floats, count, 1, paint->linearGradient[1]);
            floatToParam(values, floats, count, 2, paint->linearGradient[2]);
            floatToParam(values, floats, count, 3, paint->linearGradient[3]);
        }
        break;

    case VG_PAINT_RADIAL_GRADIENT:
        {
            if(count > 5)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, paint->inputRadialGradient[0]);
            floatToParam(values, floats, count, 1, paint->inputRadialGradient[1]);
            floatToParam(values, floats, count, 2, paint->inputRadialGradient[2]);
            floatToParam(values, floats, count, 3, paint->inputRadialGradient[3]);
            floatToParam(values, floats, count, 4, paint->inputRadialGradient[4]);
        }
        break;

    case VG_PAINT_PATTERN_TILING_MODE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, paint->patternTilingMode);
        }
        break;

    default:
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
        break;
    }

    gcmFOOTER_NO();
}

static void getPathParameterifv(_VGContext* context, _VGPath* path, VGPathParamType type, VGint count, void* values, gctBOOL floats)
{
    gcmHEADER_ARG("context=0x%x path=0x%x type=0x%x count=%d values=0x%x floats=%s",
                  context, path, type, values,
                  floats ? "TRUE" : "FALSE");
    switch(type)
    {
    case VG_PATH_FORMAT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, path->format);
        }
        break;

    case VG_PATH_DATATYPE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, path->datatype);
        }
        break;

    case VG_PATH_SCALE:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, path->scale);
        }
        break;

    case VG_PATH_BIAS:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            floatToParam(values, floats, count, 0, path->bias);
        }
        break;

    case VG_PATH_NUM_SEGMENTS:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, path->segments.size);
        }
        break;

    case VG_PATH_NUM_COORDS:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, getNumCoordinates(path));
        }
        break;

    default:
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
        break;
    }

    gcmFOOTER_NO();
}

static void getFontParameterifv(_VGContext* context, _VGFont* font, VGFontParamType type, VGint count,
                                void* values, gctBOOL floats)
{
    gcmHEADER_ARG("context=0x%x font=0x%x type=0x%x count=%d values=0x%x floats=%s",
                  context, font, type, count, values,
                  floats ? "TRUE" : "FALSE");
    switch (type)
    {
        case VG_FONT_NUM_GLYPHS:
        {
            gctINT32 i, glyphCount = 0;
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            for (i = 0; i < font->glyphs.size; i++)
            {
                if (font->glyphs.items[i].type != GLYPH_UNINITIALIZED)
                {
                    glyphCount++;
                }
            }

            intToParam(values, floats, count, 0, glyphCount);
            break;
        }
        default:
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
            break;
    }

    gcmFOOTER_NO();
}

static void getImageParameterifv(_VGContext* context, _VGImage* image, VGImageParamType type,
                                 VGint count, void* values, gctBOOL floats)
{
    gcmHEADER_ARG("context=0x%x image=0x%x type=0x%x count=%d values=0x%x floats=%s",
                  context, image, type, count, values,
                  floats ? "TRUE" : "FALSE");
    switch(type)
    {
    case VG_IMAGE_FORMAT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            gcmASSERT(isValidImageFormat(image->internalColorDesc.format));
            intToParam(values, floats, count, 0, image->internalColorDesc.format);
        }
        break;

    case VG_IMAGE_WIDTH:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, image->width);
        }
        break;

    case VG_IMAGE_HEIGHT:
        {
            if(count > 1)
            {
                SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
                gcmFOOTER_NO();
                return;
            }
            intToParam(values, floats, count, 0, image->height);
        }
        break;

    default:
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
        break;
    }

    gcmFOOTER_NO();
}


VGfloat  vgGetParameterf(VGHandle object, VGint paramType)
{
    _VGImage  *image;
    _VGPath   *path;
    _VGPaint  *paint;
    _VGFont   *font;

    VGfloat ret = 0.0f;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x", object, paramType);

    OVG_GET_CONTEXT(0.0f);

    vgmPROFILE(context, VGGETPARAMETERF, 0);
    OVG_IF_ERROR(paramType == VG_PAINT_COLOR || paramType == VG_PAINT_COLOR_RAMP_STOPS || paramType == VG_PAINT_LINEAR_GRADIENT ||
                paramType == VG_PAINT_RADIAL_GRADIENT, VG_ILLEGAL_ARGUMENT_ERROR, 0.0f);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);

    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !font, VG_BAD_HANDLE_ERROR, 0.0f);

    if(image)
    {
        gcmASSERT(!path && !paint && !font);
        getImageParameterifv(context, image, (VGImageParamType)paramType, 1, &ret, gcvTRUE);
    }
    else if(path)
    {
        gcmASSERT(!image && !paint && !font);
        getPathParameterifv(context, path, (VGPathParamType)paramType, 1, &ret, gcvTRUE);
    }
    else if (font)
    {
        gcmASSERT(!image && !paint && !path);
        getFontParameterifv(context, font, (VGFontParamType)paramType, 1, &ret, gcvTRUE);
    }
    else
    {
        gcmASSERT(!image && !path && !font && paint);
        getPaintParameterifv(context, paint, (VGPaintParamType)paramType, 1, &ret, gcvTRUE);
    }

    gcmFOOTER_ARG("return=%f", ret);
    return ret;
}


VGint  vgGetParameteri(VGHandle object, VGint paramType)
{
    _VGImage  *image;
    _VGPath   *path;
    _VGPaint  *paint;
    VGint ret = 0;
    _VGFont   *font;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x", object, paramType);

    OVG_GET_CONTEXT(0);

    vgmPROFILE(context, VGGETPARAMETERI, 0);
    OVG_IF_ERROR(paramType == VG_PAINT_COLOR || paramType == VG_PAINT_COLOR_RAMP_STOPS || paramType == VG_PAINT_LINEAR_GRADIENT ||
                paramType == VG_PAINT_RADIAL_GRADIENT, VG_ILLEGAL_ARGUMENT_ERROR, 0);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);

    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !font, VG_BAD_HANDLE_ERROR, 0);

    if(image)
    {
        gcmASSERT(!path && !paint && !font);
        getImageParameterifv(context, image, (VGImageParamType)paramType, 1, &ret, gcvFALSE);
    }
    else if(path)
    {
        gcmASSERT(!image && !paint && !font);
        getPathParameterifv(context, path, (VGPathParamType)paramType, 1, &ret, gcvFALSE);
    }
    else if (font)
    {
        gcmASSERT(!image && !paint && !path);
        getFontParameterifv(context, font, (VGFontParamType)paramType, 1, &ret, gcvFALSE);
    }
    else
    {
        gcmASSERT(!image && !path && !font && paint);
        getPaintParameterifv(context, paint, (VGPaintParamType)paramType, 1, &ret, gcvFALSE);
    }

    gcmFOOTER_ARG("return=%d", ret);
    return ret;
}

void  vgGetParameterfv(VGHandle object, VGint paramType, VGint count, VGfloat * values)
{
    _VGImage  *image;
    _VGPath   *path;
    _VGPaint  *paint;
    _VGFont   *font;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x count=%d values=0x%x", object, paramType, count, values);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGGETPARAMETERFV, 0);
    OVG_IF_ERROR(count <= 0 || !values || !isAligned(values,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);

    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !font, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    if(image)
    {
        gcmASSERT(!path && !paint && !font);
        getImageParameterifv(context, image, (VGImageParamType)paramType, count, values, gcvTRUE);
    }
    else if(path)
    {
        gcmASSERT(!image && !paint && !font);
        getPathParameterifv(context, path, (VGPathParamType)paramType, count, values, gcvTRUE);
    }
    else if (font)
    {
        gcmASSERT(!image && !paint && !path);
        getFontParameterifv(context, font, (VGFontParamType)paramType, 1, values, gcvTRUE);
    }
    else
    {
        gcmASSERT(!image && !path && !font && paint);
        getPaintParameterifv(context, paint, (VGPaintParamType)paramType, count, values, gcvTRUE);
    }

    gcmFOOTER_NO();
}


void  vgGetParameteriv(VGHandle object, VGint paramType, VGint count, VGint * values)
{
    _VGImage  *image;
    _VGPath   *path;
    _VGPaint  *paint;
    _VGFont   *font;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x count=%d values=0x%x", object, paramType, count, values);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGGETPARAMETERIV, 0);
    OVG_IF_ERROR(count <= 0 || !values || !isAligned(values,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);

    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !font, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    if(image)
    {
        gcmASSERT(!path && !paint && !font);
        getImageParameterifv(context, image, (VGImageParamType)paramType, count, values, gcvFALSE);
    }
    else if(path)
    {
        gcmASSERT(!image && !paint && !font);
        getPathParameterifv(context, path, (VGPathParamType)paramType, count, values, gcvFALSE);
    }
    else if (font)
    {
        gcmASSERT(!image && !paint && !path);
        getFontParameterifv(context, font, (VGFontParamType)paramType, 1, values, gcvFALSE);
    }
    else
    {
        gcmASSERT(!image && !path && !font && paint);
        getPaintParameterifv(context, paint, (VGPaintParamType)paramType, count, values, gcvFALSE);
    }

    gcmFOOTER_NO();
}

VGint  vgGetParameterVectorSize(VGHandle object, VGint paramType)
{
    _VGImage  *image;
    _VGPath   *path;
    _VGPaint  *paint;
    _VGFont    *font;
    gctINT32       ret = 0;
    _VGContext* context;
    gcmHEADER_ARG("object=%d paramType=0x%x", object, paramType);

    OVG_GET_CONTEXT(0);

    vgmPROFILE(context, VGGETPARAMETERVECTORSIZE, 0);

    image = _VGIMAGE(context, object);
    path = _VGPATH(context, object);
    paint = _VGPAINT(context, object);
    font = _VGFONT(context, object);
    OVG_IF_ERROR(!image && !path && !paint && !font, VG_BAD_HANDLE_ERROR, 0);

    if(image)
    {
        gcmASSERT(!path && !paint && !font);
        switch(paramType)
        {
        case VG_IMAGE_FORMAT:
        case VG_IMAGE_WIDTH:
        case VG_IMAGE_HEIGHT:
            ret = 1;
            break;

        default:
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
            break;
        }
    }
    else if(path)
    {
        gcmASSERT(!image && !paint && !font);

        switch(paramType)
        {
        case VG_PATH_FORMAT:
        case VG_PATH_DATATYPE:
        case VG_PATH_SCALE:
        case VG_PATH_BIAS:
        case VG_PATH_NUM_SEGMENTS:
        case VG_PATH_NUM_COORDS:
            ret = 1;
            break;

        default:
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
            break;
        }
    }

    else if(font)
    {
        gcmASSERT(!image && !path && !paint);
        switch(paramType)
        {
        case VG_FONT_NUM_GLYPHS:
            ret = 1;
            break;

        default:
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
            break;
        }
    }
    else
    {
        gcmASSERT(!path && !image && !font && paint);
        switch(paramType)
        {
        case VG_PAINT_TYPE:
        case VG_PAINT_COLOR_RAMP_SPREAD_MODE:
        case VG_PAINT_PATTERN_TILING_MODE:
            ret = 1;
            break;

        case VG_PAINT_COLOR:
        case VG_PAINT_LINEAR_GRADIENT:
            ret = 4;
            break;

        case VG_PAINT_COLOR_RAMP_STOPS:
            ret = (paint)->inputColorRampStops.size * 5;
            break;

        case VG_PAINT_COLOR_RAMP_PREMULTIPLIED:
            ret = 1;
            break;

        case VG_PAINT_RADIAL_GRADIENT:
            ret = 5;
            break;

        default:
            SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
            break;
        }
    }

    gcmFOOTER_ARG("return=%d", ret);
    return ret;
}


void  vgClear(VGint x, VGint y, VGint width, VGint height)
{
    _VGContext      *context;
    gctINT32 dx = x, dy = y, sx = 0, sy = 0;

    gcmHEADER_ARG("x=%d y=%d width=%d height=%d", x, y, width, height);

    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGCLEAR, 0);

    OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    if (!vgshGetIntersectArea(
        &dx, &dy, &sx, &sy, &width, &height,
        context->targetImage.width, context->targetImage.height,
        width, height))
    {
        gcmFOOTER_NO();
        return;
    }

    gcmVERIFY_OK(vgshClear(context,
            &context->targetImage,
            dx, dy, width, height,
            &context->clearColor,
            vgshIsScissoringEnabled(context),
            gcvFALSE));

    gcmFOOTER_NO();
}



VGHardwareQueryResult  vgHardwareQuery(VGHardwareQueryType key, VGint setting)
{
    _VGContext* context;
    gcmHEADER_ARG("key=0x%x setting=0x%x", key, setting);
    OVG_GET_CONTEXT(VG_HARDWARE_UNACCELERATED);

    vgmPROFILE(context, VGHARDWAREQUERY, 0);

    OVG_IF_ERROR(key != VG_IMAGE_FORMAT_QUERY && key != VG_PATH_DATATYPE_QUERY, VG_ILLEGAL_ARGUMENT_ERROR, VG_HARDWARE_UNACCELERATED);
    OVG_IF_ERROR(key == VG_IMAGE_FORMAT_QUERY && !isValidImageFormat(setting), VG_ILLEGAL_ARGUMENT_ERROR, VG_HARDWARE_UNACCELERATED);
    OVG_IF_ERROR(key == VG_PATH_DATATYPE_QUERY && (setting < VG_PATH_DATATYPE_S_8 || setting > VG_PATH_DATATYPE_F), VG_ILLEGAL_ARGUMENT_ERROR, VG_HARDWARE_UNACCELERATED);

    gcmFOOTER_ARG("return=0x%x", VG_HARDWARE_UNACCELERATED);
    return VG_HARDWARE_UNACCELERATED;
}


#define STR_LEN 16

const VGubyte *  vgGetString(VGStringID name)
{
    static const VGubyte vendor[]       = "Vivante Corporation";
    static const VGubyte version[]      = "1.1";
    static const VGubyte extensions[]   = "VG_KHR_EGL_image";
    const VGubyte   *r                  = gcvNULL;
    static VGubyte  renderer[STR_LEN + 1];

    gcmHEADER_ARG("name=0x%x", name);

    switch(name)
    {
    case VG_VENDOR:
        r = vendor;
        break;
    case VG_RENDERER:
        {
            gctUINT         offset;
            _VGContext      *context = vgshGetCurrentContext();

            renderer[STR_LEN] = 0;
            offset            = 0;

            if(context != gcvNULL)
            {
                gcmVERIFY_OK(gcoOS_PrintStrSafe((gctSTRING)renderer,
                                        gcmSIZEOF(renderer),
                                        &offset,
                                        "GC%x core",
                                        context->model));
            }

            r = renderer;
        }
        break;

    case VG_VERSION:
        r = version;
        break;
    case VG_EXTENSIONS:
        r = extensions;
        break;
    default:
        break;
    }

    gcmFOOTER_ARG("return=%s", r);
    return r;
}

