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


#ifndef __gc_vgsh_defs_h_
#define __gc_vgsh_defs_h_

/*
    USE_SCAN_LINE

    This define enables the new scan line module instead of tessellation.
*/
#define USE_SCAN_LINE	0

typedef gctINT64		_VGint64;
typedef gctINT32		_VGint32;
typedef gctUINT32		_VGuint32;
typedef gctINT16		_VGint16;
typedef gctUINT16		_VGuint16;
typedef gctINT8			_VGint8;
typedef gctUINT8		_VGuint8;

typedef VGubyte         _VGubyte;
typedef gctFLOAT        _VGfloat;

typedef gctFIXED_POINT		_VGfixed;
typedef gctCONST_POINTER	_VGPointer;

#ifndef INT32_MAX
#define INT32_MAX       (0x7fffffff)
#endif
#ifndef INT32_MIN
#define INT32_MIN       (-0x7fffffff-1)
#endif

/* maximum mantissa is 23 */
#define MANTISSA_BITS 23

/* maximum exponent is 8 */
#define EXPONENT_BITS 8


#define MAX_IMAGE_WIDTH				1280
#define MAX_IMAGE_HEIGHT			1280
#define MAX_IMAGE_PIXELS			(MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT)
#define MAX_IMAGE_BYTES				(4*MAX_IMAGE_WIDTH*MAX_IMAGE_HEIGHT)
#define MAX_DASH_COUNT				256
#define MAX_COLOR_RAMP_STOPS		256
#define MAX_KERNEL_SIZE				7
#define MAX_SEPARABLE_KERNEL_SIZE	15
#define MAX_GAUSSIAN_STD_DEVIATION	16.0f
#define MAX_SCISSOR_RECTANGLES		256
#define MAX_EDGES					262144

#define NAMED_OBJECTS_HASH		    1024

#if NO_STENCIL_VG
#define POSITION_Z_INTERVAL         (1.0f/(1<<15))
#else
#define POSITION_Z_INTERVAL         0.000013f
#endif
#define POSITION_Z_BIAS             -1.0f


#define OVG_LEVEL_ERROR             0
#define OVG_LEVEL_WARNING           1
#define OVG_LEVEL_INFO			    2
#define OVG_LEVEL_VERBOSE		    3


/*caution:gcHal.h define the last zone value is (1 << 12), lets start at (1<<16)*/
#ifdef OVG_DBGOUT
#	define OVG_TRACE_ZONE				OVG_DebugTraceZone
#elif defined(UNDER_CE)
#	define OVG_TRACE_ZONE				__noop
#else
#	define OVG_TRACE_ZONE(...)
#endif

#define OVG_ZONE_APICALL            (1 << 0)
#define OVG_ZONE_TESSELLATE         (1 << 1)
#define OVG_ZONE_CALLTIME           (1 << 2)
#define OVG_ZONE_HALLCALL           (1 << 3)

/* DEBUG Log Definitions (Unified to Other Drivers). */
#define	vgdZONE_TRACE				(gcvZONE_API_VG11 | (1 << 0))
#define	vgdZONE_STATE				(gcvZONE_API_VG11 | (1 << 1))
#define	vgdZONE_FONT				(gcvZONE_API_VG11 | (1 << 2))
#define	vgdZONE_HARDWARE			(gcvZONE_API_VG11 | (1 << 3))
#define	vgdZONE_IMAGE				(gcvZONE_API_VG11 | (1 << 4))
#define	vgdZONE_MASK				(gcvZONE_API_VG11 | (1 << 5))
#define	vgdZONE_MATRIX				(gcvZONE_API_VG11 | (1 << 7))
#define	vgdZONE_PATH				(gcvZONE_API_VG11 | (1 << 8))
#define	vgdZONE_PAINT				(gcvZONE_API_VG11 | (1 << 9))
#define	vgdZONE_TESSELLATOR			(gcvZONE_API_VG11 | (1 << 10))
#define	vgdZONE_VGU					(gcvZONE_API_VG11 | (1 << 11))

#define	vgdZONE_ALL					(gcvZONE_API_VG11 | 0xffff)

#ifndef _GC_OBJ_ZONE
#define _GC_OBJ_ZONE				vgdZONE_TRACE
#endif


typedef struct _VGObject     _VGObject;
typedef struct _VGContext    _VGContext;
typedef struct _VGImage      _VGImage;
typedef struct _VGPaint      _VGPaint;
typedef struct _VGFont       _VGFont;

typedef struct _VGMaskLayer  _VGMaskLayer;

typedef struct _VGPath       _VGPath;
typedef struct _VGGlyph		 _VGGlyph;
typedef struct _VGProgram	_VGProgram;

typedef union
{
	gctFLOAT	f;
	_VGuint32	i;
} _VGfloatInt;


#define ARRAY_DEFINE(type) \
    typedef struct type##Array \
    { \
        type     *items; \
	    VGint     size;  \
	    VGint     allocated; \
        gcoOS	  os;  \
    }type##Array

typedef enum _VGObjectType
{
	VGObject_Path,
	VGObject_Image,
	VGObject_Paint,
	VGObject_Font,
	VGObject_MaskLayer
}_VGObjectType;


struct _VGObject
{
	_VGObject     * next;
	_VGObject     * prev;
	VGint         name;
    _VGObjectType type;
	VGint         reference;
};

typedef struct _VGRectangle
{
	gctINT32 x;
	gctINT32 y;
	gctINT32 width;
	gctINT32 height;
}_VGRectangle;

typedef struct _VGFRectangle
{
	gctFLOAT x;
	gctFLOAT y;
	gctFLOAT width;
	gctFLOAT height;
}_VGFRectangle;

ARRAY_DEFINE(_VGfloat);
ARRAY_DEFINE(_VGubyte);
ARRAY_DEFINE(_VGRectangle);
ARRAY_DEFINE(_VGGlyph);

/* Tessellation Section. */
typedef struct _VGTessellationContext	_VGTessellationContext;

#if USE_FTA
typedef	struct _VGFlattenContext	_VGFlattenContext;
#endif

#if VIVANTE_PROFILER
typedef struct _VGProfile	_VGProfiler;
struct _VGContext;
#endif

#endif  /* __gc_vgsh_defs_h_ */
