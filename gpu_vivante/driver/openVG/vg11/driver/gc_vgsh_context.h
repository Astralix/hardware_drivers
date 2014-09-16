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


#ifndef __gc_vgsh_context_h_
#define __gc_vgsh_context_h_

typedef struct _vgSHAREDDATA *vgSHAREDDATA;
typedef struct _vgSHAREDDATA
{
    gctINT32    reference;
    gctINT32    objectCount;
    _VGObject   *namedObjects[NAMED_OBJECTS_HASH];
}_vgSHAREDDATA;

#if vgvUSE_PSC
/* PSC: Path Stream Cache Management. */
    /* Path Stream Cache Record for a Path. */
typedef struct _vgsPSCRecord
{
    VGint       pid;
    gctUINT32   streamSize;
    gctINT32    weight;
}   vgsPSCRecord;

    /* Path Stream Cache Manager. */
typedef struct _vgsPSCManager
{
    vgsPSCRecord    *records;
    gctINT32        count;      /* Actual record count. */
    gctINT32        capacity;   /* Record items allocated. */
    gctUINT32       streamSize; /* All stream size in bytes. */
}   vgsPSCManager;
#endif

struct _VGContext
{
        /* HAL objects. */
    gcoOS           os;
    gcoHAL          hal;
    gco3D           engine;

    gcoSURF         depth;


    _VGImage        targetImage;
    _VGImage        maskImage;

    gceCHIPMODEL    model;
    gctUINT32       revision;

    VGMatrixMode            matrixMode;
    VGFillRule              fillRule;
    VGImageQuality          imageQuality;
    VGRenderingQuality      renderingQuality;
    VGBlendMode             blendMode;
    VGImageMode             imageMode;
    /* Scissor rectangles */
    _VGRectangleArray       scissor;
    /* Stroke parameters */
    VGfloat                 strokeLineWidth;
    VGfloat                 strokeScale;
    VGCapStyle              strokeCapStyle;
    VGJoinStyle             strokeJoinStyle;
    VGfloat                 strokeMiterLimit;

    _VGfloatArray           strokeDashPattern;
    _VGfloatArray           inputStrokeDashPattern;

    VGfloat                 strokeDashPhase;
    VGboolean               strokeDashPhaseReset;
    /* Edge fill color for vgConvolve and pattern paint */
    _VGColor                tileFillColor;
    /* Color for vgClear */
    _VGColor                clearColor;

    _VGColor                inputTileFillColor;
    _VGColor                inputClearColor;

    VGboolean               masking;
    VGboolean               scissoring;
    gctBOOL                 scissorDirty;	/* Scissor rect data changed or not. */
	gctBOOL				    scissorFS;	/* Is Scissor rect larger than or equal to the full screen. */
    VGPixelLayout           pixelLayout;
    VGboolean               filterFormatLinear;
    VGboolean               filterFormatPremultiplied;
    VGbitfield              filterChannelMask;


    _VGMatrix3x3            pathUserToSurface;
    _VGMatrix3x3            imageUserToSurface;
    _VGMatrix3x3            fillPaintToUser;
    _VGMatrix3x3            strokePaintToUser;
    _VGMatrix3x3            glyphUserToSurface;
    _VGPaint                *fillPaint;
    _VGPaint                *strokePaint;
    _VGPaint                defaultPaint;
    VGErrorCode             error;

    VGboolean               colorTransform;
    VGfloat                 colorTransformValues[8];
    VGfloat                 inputColorTransformValues[8];

    /* context shared data */
    _vgSHAREDDATA           *sharedData;

    /*store the vertex attribure*/
    gcoVERTEX               vertex;
    gctFLOAT                postionZ;

    /* Glyph controls. */
    _VGVector2              glyphOrigin;
    _VGVector2              inputGlyphOrigin;

    /* Tessellation Context. */
    _VGTessellationContext  tessContext;

#if USE_FTA
    _VGFlattenContext       flatContext;
#endif

    gctFLOAT                projection[16];

    _vgHARDWARE             hardware;

#if NO_STENCIL_VG
    VGfloat                 scissorZ;
#endif

    /* Driver Profiler */
#if VIVANTE_PROFILER
    _VGProfiler profiler;
    char                        chipName[23];
#endif

#if vgvUSE_PSC
    /* PSC. */
    vgsPSCManager   pscm;
#endif
};


#define NEWOBJ(type, os, objPtr) \
    do \
    {\
        if (gcmIS_ERROR(gcoOS_Allocate(os,\
                                     sizeof(type),\
                                     (gctPOINTER *) &objPtr)))\
        {\
            gcmTRACE(gcvLEVEL_ERROR, "Out of memory, %s, %d", __FUNCTION__, __LINE__);\
            objPtr = gcvNULL;\
        } \
        else type##Ctor(os, objPtr); \
    }while(0)



#define DELETEOBJ(type, os, objPtr) \
    do { \
        if (objPtr) \
        { \
            type##Dtor(os, objPtr); \
            gcoOS_Free(os, objPtr); \
        } \
    }while(0)


#define OVG_MALLOC(os, mem, size) \
    do { \
        mem = gcvNULL; \
        gcmVERIFY_OK(gcoOS_Allocate(os, size, (gctPOINTER *) &(mem))); \
    } while (0)

#define OVG_FREE(os, mem) \
    gcmVERIFY_OK(gcmOS_SAFE_FREE(os, mem))

#define OVG_MEMCOPY(dst, src, size)         \
    do \
    { \
        gcmASSERT((dst) != gcvNULL); \
        gcmASSERT((src) != gcvNULL); \
        gcmASSERT((size) > 0); \
        gcmVERIFY_OK(gcoOS_MemCopy(dst, src, size)); \
    } \
    while (gcvFALSE)

#define OVG_MEMSET(mem, value, size) \
    gcmVERIFY_OK(gcoOS_MemFill(mem, value, size))
#define OVG_SAFE_FREE(os, mem)  \
    do \
    { \
        if (mem != gcvNULL) \
        { \
            OVG_FREE(os, mem); \
            mem = gcvNULL; \
        } \
    } \
    while (gcvFALSE)


#define OVG_NO_RETVAL

#ifdef OVG_TIME

#define OVG_RETURN(RETVAL) \
    do \
    { \
        OVG_TRACE_ZONE(OVG_LEVEL_VERBOSE, OVG_ZONE_APICALL, "%s(%d) return", __FUNCTION__, __LINE__);\
        gcmTRACE(gcvLEVEL_ERROR, "    the time elapse %d us", GetTime() - startTime);\
        return RETVAL;\
    } while(0)

#define OVG_GET_CONTEXT(RETVAL) \
    gctINT32  startTime;\
    _VGContext* context = vgshGetCurrentContext(); \
    if(!context) \
    { \
        gcmTRACE(gcvLEVEL_ERROR, "Get Context error, %s, %d", __FUNCTION__, __LINE__); \
        return RETVAL;\
    }\
    startTime = GetTime();


#define OVG_START_TIME \
    gctINT32  startTime = GetTime();

#define OVG_END_TIME \
    OVG_TRACE_ZONE(OVG_LEVEL_VERBOSE, OVG_ZONE_CALLTIME, "    the time elapse %d us", GetTime() - startTime);\

#else

#define OVG_START_TIME
#define OVG_END_TIME

#define OVG_RETURN(RETVAL) \
    do \
    { \
        OVG_TRACE_ZONE(OVG_LEVEL_VERBOSE, OVG_ZONE_APICALL, "%s(%d) return", __FUNCTION__, __LINE__);\
        return RETVAL;\
    } while(0)

#define OVG_GET_CONTEXT(RETVAL) \
    context = vgshGetCurrentContext(); \
    do { \
        if(!context) \
        { \
            return RETVAL; \
        }\
    }while(0)
#endif


#define OVG_IF_ERROR(COND, ERRORCODE, RETVAL) \
    do { \
        if(COND) \
        { \
            OVG_TRACE_ZONE(OVG_LEVEL_VERBOSE, OVG_ZONE_APICALL, "%s(%d) return :  error code = 0x%x ", __FUNCTION__, __LINE__, ERRORCODE); \
            SetError(context, ERRORCODE);  \
            return RETVAL; \
        }\
    }while(0)



#define OVG_SHADOW_BEGIN            if (!context->shadow) {
#define OVG_SHADOW_END              }



#define ARRAY_CTOR(a, oss) \
    do \
    { \
        a.items      = gcvNULL; \
        a.allocated  = 0;    \
        a.size       = 0;    \
        a.os         = oss; \
    }while(0)

#define ARRAY_CLEAR(a) \
    do \
    { \
        a.size     = 0;\
    }while(0)


#define ARRAY_DTOR(a) \
    do \
    { \
        if (a.items != gcvNULL) \
        {\
            OVG_FREE(a.os, a.items);\
        }\
        a.items      = gcvNULL; \
        a.allocated  = 0;    \
        a.size       = 0;    \
    }while(0)



#define ARRAY_ALLOCATE(a, count) \
    do \
    { \
        if ((count) > a.allocated) \
        { \
            if (a.items != gcvNULL) \
                OVG_FREE(a.os, a.items); \
            OVG_MALLOC(a.os, a.items, (count) * sizeof(*(a.items)));\
            if (a.items)\
            {\
               a.allocated = (count); \
            } \
            else \
            {  \
                gcmTRACE(gcvLEVEL_ERROR, "Out of memory"); \
                a.items = gcvNULL;\
                a.allocated = 0;\
            }\
        } \
        a.size = 0; \
    }while(0)



#define ARRAY_RESIZE(a, count) \
    do \
    { \
        ARRAY_ALLOCATE(a, count); \
        a.size = count; \
        gcmASSERT(a.size <= a.allocated); \
    }while(0)


#define ARRAY_ALLOCATE_RESERVE(a, count) \
    do \
    { \
        if ((count) > a.allocated) \
        { \
            gctPOINTER newMem;\
            OVG_MALLOC(a.os,  newMem, (count) * sizeof(*(a.items)));\
            if (newMem)\
            {\
                a.allocated = (count); \
                if (a.items != gcvNULL) \
                {\
                    if (a.size > 0) \
                        OVG_MEMCOPY(newMem, a.items, a.size * sizeof(*(a.items)));\
                    OVG_FREE(a.os, a.items);    \
                }\
                a.items = newMem;\
            } \
            else\
            {\
                gcmTRACE(gcvLEVEL_ERROR, "ARRAY_ALLOCATE_RESERVE: Out of memory"); \
                 if (a.items != gcvNULL) \
                 {\
                    OVG_FREE(a.os, a.items);\
                    a.items = gcvNULL;\
                    a.allocated = 0;\
                 }\
            } \
        } \
        a.size = 0; \
    }while(0)


#define ARRAY_RESIZE_RESERVE(a, count) \
    do \
    { \
        ARRAY_ALLOCATE_RESERVE(a, count); \
        a.size = (count); \
        gcmASSERT(a.size <= a.allocated); \
    }while(0)

#define ARRAY_PUSHBACK(a, value)  \
    do \
    { \
        gcmASSERT(a.items); \
        gcmASSERT(a.size < a.allocated); \
        a.items[a.size++] = value; \
    }while(0)

#define ARRAY_GETBACK(a, out)  \
    do \
    { \
        gcmASSERT(a.items); \
        gcmASSERT(a.size < a.allocated); \
        out = a.items[a.size]; \
    }while(0)

#define ARRAY_SIZE(a) \
    (a.size)

/* Note that there is no range check for i. */
#define ARRAY_ITEM(a, i) \
    (a.items[i])

#define _VGPATH(context, handle)   (_VGPath*)GetVGObject( context, VGObject_Path, handle)
#define _VGIMAGE(context, handle)  (_VGImage*)GetVGObject( context, VGObject_Image, handle)
#define _VGPAINT(context, handle)  (_VGPaint*)GetVGObject( context, VGObject_Paint, handle)
#define _VGFONT(context, handle)   (_VGFont*)GetVGObject( context, VGObject_Font, handle)
#define _VGMASKLAYER(context, handle)   (_VGMaskLayer*)GetVGObject( context, VGObject_MaskLayer, handle)

void _VGContextCtor(gcoOS os, _VGContext *context);
void _VGContextDtor(gcoOS os, _VGContext *context);
gctBOOL isAligned(const void* ptr, gctINT32 alignment);

gctFLOAT inputFloat(VGfloat f);

void SetError (_VGContext *c, VGErrorCode e);

gctBOOL vgshInsertObject(
    _VGContext    *Context,
    _VGObject     *Object,
    _VGObjectType Type);

void vgshRemoveObject(
    _VGContext   *Context,
    _VGObject    *Object
    );

_VGObject * vgshFindObject(
    _VGContext  *Context,
    VGuint      Name
    );

gctFLOAT    RAD_TO_DEG(gctFLOAT a);
void        SWAP(gctFLOAT *a, gctFLOAT *b);
gctFLOAT    DEG_TO_RAD(gctFLOAT a);
gctFLOAT    CLAMP(gctFLOAT a, gctFLOAT l, gctFLOAT h);
gctFLOAT    Mod(gctFLOAT a, gctFLOAT b);
gctINT32    ADDSATURATE_INT(gctINT32 a, gctINT32 b);
void StencilFunc(
    _VGContext *context,
    gceCOMPARE compare,
    VGint ref,
    VGuint mask);

void StencilOp(
          _VGContext *context,
          gceSTENCIL_OPERATION fail,
          gceSTENCIL_OPERATION zfail,
          gceSTENCIL_OPERATION zpass
          );

void ColorMask(
    _VGContext *context,
    VGboolean red,
    VGboolean green,
    VGboolean blue,
    VGboolean alpha);

void EnableStencil(_VGContext *context, VGboolean enable);

_VGObject       *GetVGObject(_VGContext *context, _VGObjectType type, VGHandle handle);
_VGContext*     vgshGetCurrentContext(void);

gceSTATUS SetTarget(_VGContext * context, gcoSURF Draw, gcoSURF Read, gcoSURF Depth);

gceTEXTURE_ADDRESSING _vgSpreadMode2gcMode( VGColorRampSpreadMode Mode);
gceTEXTURE_ADDRESSING _vgTillingMode2gcMode( VGTilingMode Mode);

void finish(_VGContext *context);
void flush(_VGContext *context);
gceSTATUS BindTexture(_VGContext *context, gcoTEXTURE texture, gctINT sampler);

void ClearSurface(_VGContext *context,
                  gcoSURF surface,
                  gctINT32 x, gctINT32 y, gctINT32 width, gctINT32 height, _VGColor *color);

void VGObject_AddRef(gcoOS os, _VGObject *object);
void VGObject_Release(gcoOS os, _VGObject *object);


void updateScissor(_VGContext *context);
gceSTATUS vgshUpdateScissor(_VGContext *context);
gctBOOL vgshIsScissoringEnabled(_VGContext *context);

gceSTATUS _CreateSharedData(_VGContext *context, _VGContext *sharedContext);
gceSTATUS _DestroySharedData(_VGContext *context);

/* Tessellation Section. */
void    _VGTessellationContextCtor(gcoOS os, _VGTessellationContext *tContext);
void    _VGTessellationContextDtor( _VGContext  *context );
void    _VGResetTessellationContext(    _VGContext  *context);
void    _VGResetTessellationContextFlatten(_VGTessellationContext *tContext);
void    _VGResetTessellationContextTriangulate( _VGContext  *context);
#if USE_FTA
void    _VGFlattenContextCtor(gcoOS os, _VGFlattenContext *fContext);
void    _VGFlattenContextDtor(gcoOS os, _VGFlattenContext *fContext);
#endif

gctINT32 GetTime(void);

void OVG_DebugTrace(
    IN gctUINT32 Level,
    IN char* Message,
    ...
    );

void OVG_DebugTraceZone(
    IN gctUINT32 Level,
    IN gctUINT32 Zone,
    IN char * Message,
    ...
    );

void OVG_SetDebugLevel(
    IN gctUINT32 Level);

void OVG_SetDebugZone(
    IN gctUINT32 Zone);

#if vgvUSE_PSC
/* PSC: Init/Deinit. */
void _PSCManagerCtor(gcoOS os, vgsPSCManager *pscm);
void _PSCManagerDtor(gcoOS os, vgsPSCManager *pscm);
void _PSCManagerHit(VGint pid, vgsPSCManager *pscm, gcoOS os);
void _PSCManagerShuffle(vgsPSCManager *pscm, VGint exception);
void _PSCManagerDismiss(_VGContext *context, gctINT32 qual);
void _PSCManagerRelease(vgsPSCManager *pscm, VGint pid);
#endif

#endif /* __gc_vgsh_context_h_ */
