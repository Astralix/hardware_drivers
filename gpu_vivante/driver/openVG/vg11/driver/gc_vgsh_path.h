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


#ifndef __gc_vgsh_path_h_
#define __gc_vgsh_path_h_

typedef enum _VGTessPhase
{
    VGTessPhase_None    = 0x00,
    VGTessPhase_FlattenFill = 0x01,
    VGTessPhase_FlattenStroke = 0x02,
    VGTessPhase_Stroke  = 0x04,
    VGTessPhase_Fill    = 0x08,
	VGTessPhase_ALL		= 0x0f,
}_VGTessPhase;

typedef struct _VGVertexBufferDesc
{
    VGint               primitiveCount;
    gcePRIMITIVE        primitiveType;
    VGint               start;
    VGint               start2;         /*if index buffer is used, then 2 starts are needed, 1 for vertex buffer, and one for index buffer.*/
}_VGVertexBufferDesc;

ARRAY_DEFINE(_VGVertexBufferDesc);

typedef struct _VGIndexBuffer
{
    gceINDEX_TYPE       indexType;

    gcoINDEX            index;
    _VGubyteArray       data;
    gctINT32            count;

}_VGIndexBuffer;


typedef struct _VGVertexBuffer
{

	gcoSTREAM			stream;


    VGint			    size;
	gceVERTEX_FORMAT	type;
	gctBOOL		        normalized;
	VGint			    stride;

    _VGubyteArray       data;

    gctINT32            streamSize;

    gctINT32            elementCount;

}_VGVertexBuffer;

/* Path Segment Data Object that holds the preprocessed data from Path Commands and Data. */
typedef struct _VGPathSegObject
{
	VGPathSegment	type;
	union _PathSegObject
	{
		_VGVector2	point;
		_VGVector2	points[2];
		_VGBezier	bezier;
		struct
		{
			_VGEllipse		ellipse;
			gctFLOAT		angle[2];
			_VGVector2		points[2];
		} arc;
	}	object;
}	_VGPathSegObject;
ARRAY_DEFINE(_VGPathSegObject);

typedef struct _VGTessellateBuffer
{
    /*for fill*/
    _VGIndexBuffer           indexBuffer;
    _VGVertexBuffer          vertexBuffer;
     /*for fill description
    _VGVertexBufferDescArray fillDescArray; */
	_VGVertexBufferDesc		 fillDesc;
    gctBOOL                  fillDirty;

    _VGFRectangle             bound;
    gcoSTREAM			     boundStream;

    /*for stroke*/
    _VGVertexBuffer          strokeBuffer;
	_VGIndexBuffer			 strokeIndexBuffer;

	/*maybe need
    _VGVertexBufferDescArray strokeDescArray; */
	_VGVertexBufferDesc		 strokeDesc;
    gctBOOL                  strokeDirty;

    /*for filling-path tessellation. (Also applied to path query)*/
    _VGFlattenBuffer         flattenedFillPath;
    gctBOOL                  flattenDirty;

    /*special for stroking-path.*/
    _VGFlattenBuffer         flattenedStrokePath;
    gctBOOL                  flattenStrokeDirty;
}_VGTessellateBuffer;

#if USE_SCAN_LINE
/*
    USE_FIXED_POINT_FOR_COORDINATES

    This define enables using 16.16 fixed point to represent coordinates instead of float.
*/
#define USE_FIXED_POINT_FOR_COORDINATES		1

/*
    USE_NEW_SCAN_LINE_FORMAT

    This define enables the new scan line format for VG core.
*/
#define USE_NEW_SCAN_LINE_FORMAT			0

/*
    SCAN_LINE_HAS_3D_SUPPORT

    This define enables the triangulaton feature in the scan line module.
*/
#define SCAN_LINE_HAS_3D_SUPPORT			0

/*
    USE_SCAN_LINE_ON_RI

    This define enables the scan line module on RI for C-Model.
*/
#define USE_SCAN_LINE_ON_RI					0

/*
    USE_TEST_LOG

    This define enables the scan line module to output flattened path to log file.
*/
#define USE_TEST_LOG						0

/*
    FOR_CONFORMANCE_TEST

    This define enables special flattening and small epsillon for conformance test.
*/
#define FOR_CONFORMANCE_TEST				0

#if FOR_CONFORMANCE_TEST
#define FLATTEN_CURVE_SAMPLES				256

/* Always use floating point for conformance test. */
#if USE_FIXED_POINT_FOR_COORDINATES
#undef USE_FIXED_POINT_FOR_COORDINATES
#define USE_FIXED_POINT_FOR_COORDINATES		0
#endif
#endif

#if USE_FIXED_POINT_FOR_COORDINATES
typedef _VGfixed			_VGcoord;
#else
typedef VGfloat				_VGcoord;
#endif

#define FRACTION_BITS		16
#define INTEGER_BITS		16

/* Line structure for scan line fill. */
#if USE_NEW_SCAN_LINE_FORMAT
#define SCANLINE_FRACTION_BITS	8
#define SCANLINE_INTEGER_BITS	16

typedef struct _VGScanLine
{
	/* X coordinate. */
	gctINT64				x1:24;
	gctINT64				x2:24;

	/* Y coordinate. */
	gctINT64				y:16;
} _VGScanLine;
#else
typedef struct _VGScanLine
{
	/* Point 1. */
	VGfloat					x1;
	VGfloat					y1;

	/* Point 2. */
	VGfloat					x2;
	VGfloat					y2;
} _VGScanLine;
#endif
#endif

struct _VGPath
{

	_VGObject        object;
	VGint            format;
	VGPathDatatype   datatype;
	VGfloat          scale;
	VGfloat          bias;
	VGbitfield       capabilities;
	_VGubyteArray    segments;
	_VGubyteArray    data;


	/*tessellate result*/
    _VGTessPhase            tessPhaseFailed;
    _VGTessPhase            tessPhaseDirty;
    _VGTessellateBuffer     tessellateResult;
    _VGPathSegInfoArray     segmentsInfo;

#if USE_SCAN_LINE
	/* ScanLine buffer. */
    _VGVertexBuffer			scanLineFillBuffer;
    _VGVertexBuffer			scanLineStrokeBuffer;

#if SCAN_LINE_HAS_3D_SUPPORT
	/* Index buffer. */
	_VGIndexBuffer			fillIndexBuffer;
	/* Triangle buffer. */
    _VGVertexBuffer			fillVertexBuffer;
	/* Index buffer. */
	_VGIndexBuffer			strokeIndexBuffer;
	/* Triangle buffer. */
    _VGVertexBuffer			strokeVertexBuffer;
#endif
#endif

	/* Cashed states. */
	/* Fill params.*/
	VGFillRule				fillRule;
	/* Stroke paramters.*/
	gctFLOAT				strokeLineWidth;
	VGCapStyle				strokeCapStyle;
	VGJoinStyle				strokeJoinStyle;
	gctFLOAT				strokeMiterLimit;
	gctFLOAT				dashPhase;
	gctBOOL					dashPhaseReset;
#if USE_SCAN_LINE
	gctFLOAT				transformRS[4];
#else
	/* Transform Scale Value*/
	gctFLOAT				transformScale[2];
#endif

};

gctINT32 getNumCoordinates(_VGPath *path);

void _VGPathCtor(gcoOS os, _VGPath *path);
void _VGPathDtor(gcoOS os, _VGPath *path);

void _VGTessellateBufferCtor(gcoOS os, _VGTessellateBuffer *tessellateResult);
void _VGTessellateBufferDtor(gcoOS os, _VGTessellateBuffer *tessellateResult);
void _VGVertexBufferCtor(gcoOS os, _VGVertexBuffer *vertexBuffer);
void _VGVertexBufferDtor(gcoOS os, _VGVertexBuffer *vertexBuffer);
void _VGIndexBufferCtor(gcoOS os, _VGIndexBuffer *indexBuffer);
void _VGIndexBufferDtor(gcoOS os, _VGIndexBuffer *indexBuffer);

void DrawRect(_VGContext *context, _VGProgram *program, _VGRectangle *rect);

void _VGFlattenBufferCtor(_VGFlattenBuffer *buffer);
void _VGFlattenBufferDtor(gcoOS os, _VGFlattenBuffer *buffer);
void PathDirty(_VGPath* path,  _VGTessPhase tessPhase);
void PathClean(_VGPath* path,  _VGTessPhase tessPhase);
void AllPathDirty(_VGContext *context, _VGTessPhase tessPhase);
gctBOOL IsPathDirty(_VGPath* path,  _VGTessPhase tessPhase);

void CheckContextParam(_VGContext *context, _VGPath* path, _VGMatrix3x3 *mat, VGbitfield	paintModes);

/* Triangulation. 0610*/
_VGint16    *_Triangulation(
	_VGContext				*context,
    _VGVector2/*_VGTessPoint*/    **points,
    gctINT32        *pointsLength,
    gctINT32        loopLength[],
    gctINT32        loopLengthLength,
    gctBOOL            nonZero,
    gctINT32        *triangleCounts,
    gctINT32        *arrayLength
    );

void    GenRTTexture(
    _VGContext *context,
    _VGProgram *program
    );

gctBOOL		TessFlatten(_VGContext * context, _VGPath* path, _VGMatrix3x3* matrix, gctFLOAT strokeWidth);

void drawPath(_VGContext	*context,
			  _VGPath		*path,
			  _VGPaint		*fillPaint,
			  _VGPaint		*strokePaint,
			  _VGMatrix3x3  *matrix,
			  VGbitfield	paintModes,
			  VGboolean		masking,
			  VGboolean     invert);

void GenColorRamp(_VGContext *context, _VGPaint *paint, _VGColorFormat dstFormat);

gctBOOL TessFlatten(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix, gctFLOAT strokeScale);
gctBOOL TessFillPath(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix);
gctBOOL TessStrokePath(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix);
void LoadVertexs2(_VGContext *context, _VGVertexBuffer *vertexBuffer);
void LoadIndices2(_VGContext *context, _VGIndexBuffer *indexBuffer);
void ClearTessellateResult(_VGContext * context, _VGPath* path);

#endif /* __gc_vgsh_path_h_ */
