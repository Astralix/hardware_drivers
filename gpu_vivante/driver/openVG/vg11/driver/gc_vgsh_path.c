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

#if USE_SCAN_LINE
#include "gc_vgsh_scanline.h"
#endif

#ifdef OBJECT_STATISTIC
static  gctINT32 points_flatten = 0;
#endif

void _VGPathCtor(gcoOS os, _VGPath *path)
{
	gcmHEADER_ARG("os=0x%x path=0x%x",
				   os, path);

    path->bias         = 0.0f;
    path->scale        = 1.0f;
    path->capabilities = 0;
    path->datatype     = VG_PATH_DATATYPE_F;
    path->format       = VG_PATH_FORMAT_STANDARD;

	path->fillRule             = VG_EVEN_ODD;
	path->strokeLineWidth      = 1.0f;
    path->strokeCapStyle       = VG_CAP_BUTT;
    path->strokeJoinStyle      = VG_JOIN_MITER;
    path->strokeMiterLimit     = 4.0f;

#if USE_SCAN_LINE
	path->transformRS[0] = path->transformRS[1] = path->transformRS[2] = path->transformRS[3] = 0.0f;
#else
	path->transformScale[0] = path->transformScale[1] = 0.0f;
#endif

    path->tessPhaseDirty    = VGTessPhase_ALL;
    path->tessPhaseFailed   = VGTessPhase_None;

    ARRAY_CTOR(path->segments, os);
    ARRAY_CTOR(path->data, os);
    ARRAY_CTOR(path->segmentsInfo, os);

	gcoOS_ZeroMemory(&path->object, sizeof(_VGObject));

    _VGTessellateBufferCtor(os, &path->tessellateResult);

#if USE_SCAN_LINE
	_VGVertexBufferCtor(os, &path->scanLineFillBuffer);
	_VGVertexBufferCtor(os, &path->scanLineStrokeBuffer);
#if SCAN_LINE_HAS_3D_SUPPORT
    _VGIndexBufferCtor(os, &path->fillIndexBuffer);
    _VGVertexBufferCtor(os, &path->fillVertexBuffer);
    _VGIndexBufferCtor(os, &path->strokeIndexBuffer);
    _VGVertexBufferCtor(os, &path->strokeVertexBuffer);
#endif
#endif

	gcmFOOTER_NO();
}

void _VGPathDtor(gcoOS os, _VGPath *path)
{
	gcmHEADER_ARG("os=0x%x path=0x%x",
				   os, path);

    ARRAY_DTOR(path->segments);
    ARRAY_DTOR(path->data);

    _VGTessellateBufferDtor(os, &path->tessellateResult);
#if USE_SCAN_LINE
    _VGVertexBufferDtor(&path->scanLineFillBuffer);
    _VGVertexBufferDtor(&path->scanLineStrokeBuffer);
#if SCAN_LINE_HAS_3D_SUPPORT
    _VGIndexBufferDtor(&path->fillIndexBuffer);
    _VGVertexBufferDtor(&path->fillVertexBuffer);
    _VGIndexBufferDtor(&path->strokeIndexBuffer);
    _VGVertexBufferDtor(&path->strokeVertexBuffer);
#endif
#endif
    ARRAY_DTOR(path->segmentsInfo);

	gcmFOOTER_NO();
}


void _VGVertexBufferCtor(gcoOS os, _VGVertexBuffer *vertexBuffer)
{
	gcmHEADER_ARG("os=0x%x vertexBuffer=0x%x",
				   os, vertexBuffer);

    ARRAY_CTOR(vertexBuffer->data, os);
    vertexBuffer->size   = 0;
    vertexBuffer->type   = gcvVERTEX_FLOAT;
    vertexBuffer->stream = gcvNULL;

    vertexBuffer->stride     = 0;
    vertexBuffer->normalized = gcvFALSE;
    vertexBuffer->streamSize = 0;

	gcmFOOTER_NO();
}

void _VGVertexBufferDtor(gcoOS os, _VGVertexBuffer *vertexBuffer)
{
	gcmHEADER_ARG("os=0x%x vertexBuffer=0x%x",
				   os, vertexBuffer);

    ARRAY_DTOR(vertexBuffer->data);
    if (vertexBuffer->stream)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(vertexBuffer->stream));
    }

	gcmFOOTER_NO();
}

void _VGIndexBufferCtor(gcoOS os, _VGIndexBuffer *indexBuffer)
{
	gcmHEADER_ARG("os=0x%x indexBuffer=0x%x",
				   os, indexBuffer);

    ARRAY_CTOR(indexBuffer->data, os);
    indexBuffer->indexType  = gcvINDEX_8;
    indexBuffer->index      = gcvNULL;

	gcmFOOTER_NO();
}

void _VGIndexBufferDtor(gcoOS os, _VGIndexBuffer *indexBuffer)
{
	gcmHEADER_ARG("os=0x%x indexBuffer=0x%x",
				   os, indexBuffer);

    ARRAY_DTOR(indexBuffer->data);
    if (indexBuffer->index)
    {
        gcmVERIFY_OK(gcoINDEX_Destroy(indexBuffer->index));
    }

	gcmFOOTER_NO();
}

void _VGFlattenBufferCtor(_VGFlattenBuffer *buffer)
{
	gcmHEADER_ARG("buffer=0x%x", buffer);

    buffer->numPoints = 0;
    buffer->numPointsInSubPath = gcvNULL;
    buffer->numSubPaths = 0;
    buffer->points = gcvNULL;
    buffer->subPathClosed = gcvNULL;

	gcmFOOTER_NO();
}

void _VGFlattenBufferDtor(gcoOS os, _VGFlattenBuffer *buffer)
{
	gcmHEADER_ARG("os=0x%x buffer=0x%x",
				   os, buffer);

    buffer->numPoints = 0;
    buffer->numSubPaths = 0;
    OVG_SAFE_FREE(os, buffer->points);
    OVG_SAFE_FREE(os, buffer->numPointsInSubPath);
    OVG_SAFE_FREE(os, buffer->subPathClosed);

	gcmFOOTER_NO();
}

void _VGTessellateBufferCtor(gcoOS os, _VGTessellateBuffer *tessellateResult)
{
	gcmHEADER_ARG("os=0x%x tessellateResult=0x%x",
				   os, tessellateResult);

    _VGVertexBufferCtor(os, &tessellateResult->vertexBuffer);
    _VGVertexBufferCtor(os ,&tessellateResult->strokeBuffer);
    _VGIndexBufferCtor(os, &tessellateResult->indexBuffer);

	_VGIndexBufferCtor(os, &tessellateResult->strokeIndexBuffer);
    tessellateResult->strokeDirty = gcvTRUE;
    tessellateResult->fillDirty = gcvTRUE;
    tessellateResult->flattenDirty = gcvTRUE;
    tessellateResult->boundStream = gcvNULL;
    gcoOS_ZeroMemory(&tessellateResult->bound, sizeof(_VGRectangle));

    _VGFlattenBufferCtor(&tessellateResult->flattenedFillPath);
    _VGFlattenBufferCtor(&tessellateResult->flattenedStrokePath);

	gcmFOOTER_NO();
}


void _VGTessellateBufferDtor(gcoOS os, _VGTessellateBuffer *tessellateResult)
{
	gcmHEADER_ARG("os=0x%x tessellateResult=0x%x",
				   os, tessellateResult);

	_VGIndexBufferDtor(os, &tessellateResult->strokeIndexBuffer);
    _VGIndexBufferDtor(os, &tessellateResult->indexBuffer);
    _VGVertexBufferDtor(os, &tessellateResult->vertexBuffer);
    _VGVertexBufferDtor(os, &tessellateResult->strokeBuffer);

    if (tessellateResult->boundStream)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(tessellateResult->boundStream));
    }

    _VGFlattenBufferDtor(os, &tessellateResult->flattenedFillPath);
    _VGFlattenBufferDtor(os, &tessellateResult->flattenedStrokePath);

	gcmFOOTER_NO();
}

void PathDirty(_VGPath* path,  _VGTessPhase tessPhase)
{
	gcmHEADER_ARG("path=0x%x tessPhase=0x%x", path, tessPhase);

    path->tessPhaseDirty |= tessPhase;

	gcmFOOTER_NO();
}

void PathClean(_VGPath* path,  _VGTessPhase tessPhase)
{
	gcmHEADER_ARG("path=0x%x tessPhase=0x%x", path, tessPhase);

	path->tessPhaseDirty &= ~tessPhase;

	gcmFOOTER_NO();
}


gctBOOL IsPathDirty(_VGPath* path,  _VGTessPhase tessPhase)
{
#ifdef PATH_NO_CACHE
	return gcvTRUE;
#else
    return (path->tessPhaseDirty & tessPhase);
#endif

}

gctBOOL IsTessPhaseFailed(_VGPath* path, _VGTessPhase tessPhase)
{
    return path->tessPhaseFailed & tessPhase;
}

void TessPhaseFailed(_VGPath* path, _VGTessPhase tessPhase)
{
    path->tessPhaseFailed |= tessPhase;
}

void TessPhaseClean(_VGPath* path, _VGTessPhase tessPhase)
{
    path->tessPhaseFailed &= ~tessPhase;
}


VGPathSegment getPathSegment(_VGuint8 data)
{
    return (VGPathSegment)(data & 0x1e);
}


gctINT32 segmentToNumCoordinates(VGPathSegment segment)
{
    static const gctINT32 coords[13] = {0,2,2,1,1,4,6,2,4,5,5,5,5};

	gcmASSERT(((gctINT32)segment >> 1) >= (gctINT32)(VG_CLOSE_PATH >> 1) && ((gctINT32)segment >> 1) <= (gctINT32)(VG_LCWARC_TO >> 1));

	return coords[(gctINT32)segment >> 1];
}

gctINT32 countNumCoordinates(const _VGuint8* segments, gctINT32 numSegments)
{
    gctINT32 coordinates = 0;
    gctINT32 i;

	gcmASSERT(segments);
	gcmASSERT(numSegments >= 0);


	for(i=0;i<numSegments;i++)
		coordinates += segmentToNumCoordinates(getPathSegment(segments[i]));

	return coordinates;
}

gctINT32 getBytesPerCoordinate(VGPathDatatype datatype)
{
	gcmHEADER_ARG("datatype=%d", datatype);

	if(datatype == VG_PATH_DATATYPE_S_8)
	{
		gcmFOOTER_ARG("return=%d", 1);
		return 1;
	}
	if(datatype == VG_PATH_DATATYPE_S_16)
	{
		gcmFOOTER_ARG("return=%d", 2);
		return 2;
	}
	gcmASSERT(datatype == VG_PATH_DATATYPE_S_32 || datatype == VG_PATH_DATATYPE_F);

	gcmFOOTER_ARG("return=%d", 4);
	return 4;
}

gctINT32 getNumCoordinates(_VGPath *path)
{
	_VGint32 result;
	gcmHEADER_ARG("path=0x%x", path);
	result = path->data.size/getBytesPerCoordinate(path->datatype);
	gcmFOOTER_ARG("return=%d", result);

	return result;
}

gctFLOAT getCoordinate(_VGPath *p, gctINT32 i)
{
    const _VGuint8* ptr;

	gcmHEADER_ARG("p=0x%x i=%d", p, i);

	gcmASSERT(i >= 0 && i < p->data.size / getBytesPerCoordinate(p->datatype));
	gcmASSERT(p->scale != 0.0f);

    ptr = &p->data.items[0];
	switch(p->datatype)
	{
	case VG_PATH_DATATYPE_S_8:
		gcmFOOTER_ARG("return=%f", (gctFLOAT)(((const _VGint8*)ptr)[i]) * p->scale + p->bias);
		return (gctFLOAT)(((const _VGint8*)ptr)[i]) * p->scale + p->bias;

	case VG_PATH_DATATYPE_S_16:
		gcmFOOTER_ARG("return=%f", (gctFLOAT)(((const _VGint16*)ptr)[i]) * p->scale + p->bias);
		return (gctFLOAT)(((const _VGint16*)ptr)[i]) * p->scale + p->bias;

	case VG_PATH_DATATYPE_S_32:
		gcmFOOTER_ARG("return=%f", (gctFLOAT)(((const gctINT32*)ptr)[i]) * p->scale + p->bias);
		return (gctFLOAT)(((const gctINT32*)ptr)[i]) * p->scale + p->bias;

	default:
		gcmASSERT(p->datatype == VG_PATH_DATATYPE_F);
		gcmFOOTER_ARG("return=%f", (gctFLOAT)(((const gctFLOAT*)ptr)[i]) * p->scale + p->bias);
		return (gctFLOAT)(((const gctFLOAT*)ptr)[i]) * p->scale + p->bias;
	}
}

void setCoordinate(_VGubyteArray* data, VGPathDatatype datatype, gctFLOAT scale, gctFLOAT bias, gctINT32 i, gctFLOAT c)
{
    _VGuint8* ptr;

	gcmHEADER_ARG("data=0x%x datatype=%d scale=%f bias=%f i=%d c=%f", data, datatype, scale, bias, i, c);

	gcmASSERT(i >= 0 && i < data->size/getBytesPerCoordinate(datatype));
	gcmASSERT(scale != 0.0f);

	c -= bias;
	c /= scale;

    ptr = &data->items[0];
	switch(datatype)
	{
	case VG_PATH_DATATYPE_S_8:
		((_VGint8*)ptr)[i] = (_VGint8)gcoMATH_Floor(c + 0.5f);	/*add 0.5 for correct rounding*/
		break;

	case VG_PATH_DATATYPE_S_16:
		((_VGint16*)ptr)[i] = (_VGint16)gcoMATH_Floor(c + 0.5f);	/*add 0.5 for correct rounding*/
		break;

	case VG_PATH_DATATYPE_S_32:
		((gctINT32*)ptr)[i] = (gctINT32)gcoMATH_Floor(c + 0.5f);	/*add 0.5 for correct rounding*/
		break;

	default:
		gcmASSERT(datatype == VG_PATH_DATATYPE_F);
		((gctFLOAT*)ptr)[i] = (gctFLOAT)c;
		break;
	}

	gcmFOOTER_NO();
}


VGPathAbsRel	getPathAbsRel(_VGuint8 data)
{
	gcmHEADER_ARG("data=%u", data);
	gcmFOOTER_ARG("return=%d", (VGPathAbsRel)(data & 0x1));

    return (VGPathAbsRel)(data & 0x1);
}


VGPath  vgCreatePath(VGint pathFormat, VGPathDatatype datatype, VGfloat scale, VGfloat bias,
					 VGint segmentCapacityHint, VGint coordCapacityHint, VGbitfield capabilities)
{
    _VGPath* path = gcvNULL;
	gctFLOAT s;
	gctFLOAT b;
	_VGContext* context;

	gcmHEADER_ARG("pathFormat=%d datatype=%d scale=%f biase=%f "
				  "segmentCapacityHint=%d coordCapacityHint=%d capabilities=0x%x",
				  pathFormat, datatype, scale, bias, segmentCapacityHint,
				  coordCapacityHint, capabilities);
	OVG_GET_CONTEXT(VG_INVALID_HANDLE);

	vgmPROFILE(context, VGCREATEPATH, 0);

	OVG_IF_ERROR(pathFormat != VG_PATH_FORMAT_STANDARD, VG_UNSUPPORTED_PATH_FORMAT_ERROR, VG_INVALID_HANDLE);
	OVG_IF_ERROR(datatype < VG_PATH_DATATYPE_S_8 || datatype > VG_PATH_DATATYPE_F, VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);
	s = inputFloat(scale);
	b = inputFloat(bias);
	OVG_IF_ERROR(s == 0.0f, VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);
	capabilities &= VG_PATH_CAPABILITY_ALL;

    NEWOBJ(_VGPath, context->os, path);

    if (!path)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
		gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
    }

    if (!vgshInsertObject(context, &path->object, VGObject_Path)) {
        DELETEOBJ(_VGPath, context->os, path);
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
		gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
    }

	VGObject_AddRef(context->os, &path->object);

    path->bias         = b;
    path->scale        = s;
    path->capabilities = capabilities;
    path->datatype     = datatype;
    path->format       = pathFormat;

    ARRAY_CTOR(path->segments, context->os);
    ARRAY_CTOR(path->data, context->os);

	_VGTessellateBufferCtor(context->os, &path->tessellateResult);

    if(segmentCapacityHint > 0)
	{
		_VGint32 pathSegmentsSize = gcmMIN(segmentCapacityHint, 65536);
		ARRAY_ALLOCATE(path->segments,  pathSegmentsSize);
	}
	if(coordCapacityHint > 0)
	{
		_VGint32 pathDataSize = gcmMIN(coordCapacityHint, 65536) * getBytesPerCoordinate(datatype);
		ARRAY_ALLOCATE(path->data,  pathDataSize);
	}

	gcmFOOTER_ARG("return=%d", path->object.name);
    return (VGPath)path->object.name;
}



void  vgClearPath(VGPath path, VGbitfield capabilities)
{
    _VGPath *p = gcvNULL;
	_VGContext* context;

	gcmHEADER_ARG("path=%d capabilities=0x%x", path, capabilities);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGCLEARPATH, 0);

    p  = _VGPATH(context, path);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

	capabilities &= VG_PATH_CAPABILITY_ALL;

    p->capabilities = capabilities;

    PathDirty(p, VGTessPhase_ALL);

    ARRAY_CLEAR(p->segments);
    ARRAY_CLEAR(p->data);

	gcmFOOTER_NO();
}

void  vgDestroyPath(VGPath path)
{
    _VGPath *p;
	_VGContext* context;
	gcmHEADER_ARG("path=%d", path);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGDESTROYPATH, 0);

    p = _VGPATH(context, path);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    vgshRemoveObject( context, &p->object);

	VGObject_Release(context->os, &p->object);

	/* PSC. */
#if vgvUSE_PSC
	_PSCManagerRelease(&context->pscm, path);
#endif

	gcmFOOTER_NO();
}



void  vgRemovePathCapabilities(VGPath path, VGbitfield capabilities)
{
    _VGPath *p;
	_VGContext* context;

	gcmHEADER_ARG("path=%d capabilities=0x%x", path, capabilities);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGREMOVEPATHCAPABILITIES, 0);

    p = _VGPATH(context, path);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	capabilities &= VG_PATH_CAPABILITY_ALL;

	p->capabilities &= ~capabilities;

	gcmFOOTER_NO();
}



VGbitfield  vgGetPathCapabilities(VGPath path)
{
    VGbitfield ret;
    _VGPath *p;
	_VGContext* context;

	gcmHEADER_ARG("path=%d", path);
	OVG_GET_CONTEXT(0);

	vgmPROFILE(context, VGGETPATHCAPABILITIES, 0);

    p  = _VGPATH(context, path);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, 0);
	ret = p->capabilities;

	gcmFOOTER_ARG("return=0x%x", ret);
	return ret;
}

void  vgAppendPathData(VGPath dstPath, VGint numSegments, const VGubyte * pathSegments, const void * pathData)
{
    gctINT32 oldSegmentsSize, newSegmentsSize;
    gctINT32 newCoords, bytesPerCoordinate, OldDataSize, newDataSize;
    gctINT32 i;
    _VGPath* p = gcvNULL;
    const gctFLOAT* s = (const gctFLOAT*)pathData;
    gctFLOAT* d;
    VGPathSegment c;
	_VGContext* context;

	gcmHEADER_ARG("dstPath=%d numSegments=%d pathSegments=0x%x pathData=0x%x",
				  dstPath, numSegments, pathSegments, pathData);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGAPPENDPATHDATA, 0);

    p = _VGPATH(context, dstPath);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

	OVG_IF_ERROR(!(p->capabilities & VG_PATH_CAPABILITY_APPEND_TO), VG_PATH_CAPABILITY_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(numSegments <= 0 || !pathSegments || !pathData, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR((p->datatype == VG_PATH_DATATYPE_S_16 && !isAligned(pathData,2)) ||
				((p->datatype == VG_PATH_DATATYPE_S_32 || p->datatype == VG_PATH_DATATYPE_F) && !isAligned(pathData,4)),
				VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);


	for(i=0;i<numSegments;i++)
	{
		c = (VGPathSegment)(pathSegments[i] & 0x1e);
		OVG_IF_ERROR(c < VG_CLOSE_PATH || c > VG_LCWARC_TO, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
		OVG_IF_ERROR(c & ~0x1f, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	}


    oldSegmentsSize = p->segments.size;
	newSegmentsSize = oldSegmentsSize + numSegments;

    ARRAY_RESIZE_RESERVE(p->segments, newSegmentsSize);

	newCoords = countNumCoordinates(pathSegments, numSegments);
	bytesPerCoordinate = getBytesPerCoordinate(p->datatype);
    OldDataSize = p->data.size;
	newDataSize = OldDataSize + newCoords * bytesPerCoordinate;

    ARRAY_RESIZE_RESERVE(p->data, newDataSize);

    gcoOS_MemCopy(&p->segments.items[0] + oldSegmentsSize, pathSegments, numSegments);

	if(p->data.size)
	{
		if(p->datatype == VG_PATH_DATATYPE_F)
		{
            d = (gctFLOAT*)(&p->data.items[0] + OldDataSize);
			for(i=0;i<newCoords;i++)
				*d++ = (gctFLOAT)inputFloat(*s++);
		}
		else
		{
			gcoOS_MemCopy(&p->data.items[0] + OldDataSize, pathData, newCoords * bytesPerCoordinate);
		}
	}

	gcmASSERT(p->data.size == countNumCoordinates(&p->segments.items[0], p->segments.size) * getBytesPerCoordinate(p->datatype));

    PathDirty(p, VGTessPhase_ALL);

	gcmFOOTER_NO();
}

void  vgModifyPathCoords(VGPath dstPath, VGint startIndex, VGint numSegments, const void * pathData)
{
    _VGPath* p = gcvNULL;
    gctINT32 startCoord;
    gctINT32 numCoords;
    gctINT32 bytesPerCoordinate;
    gctFLOAT* d;
    const gctFLOAT* s;
    gctINT32 i = 0;
    _VGuint8* dst;
	_VGContext* context;

	gcmHEADER_ARG("dstPath=%d startIndex=%d numSegments=%d pathData=0x%x",
				  dstPath, startIndex, numSegments, pathData);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGMODIFYPATHCOORDS, 0);

    p  = _VGPATH(context, dstPath);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    OVG_IF_ERROR(!(p->capabilities & VG_PATH_CAPABILITY_MODIFY), VG_PATH_CAPABILITY_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!pathData || startIndex < 0 || numSegments <= 0 || (startIndex + numSegments) > p->segments.size, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR((p->datatype == VG_PATH_DATATYPE_S_16 && !isAligned(pathData,2)) ||
				((p->datatype == VG_PATH_DATATYPE_S_32 || p->datatype == VG_PATH_DATATYPE_F) && !isAligned(pathData,4)),
				VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    startCoord = countNumCoordinates(&p->segments.items[0], startIndex);
	numCoords = countNumCoordinates(&p->segments.items[startIndex], numSegments);

	if(!numCoords)
	{
		gcmFOOTER_NO();
		return;
	}

	bytesPerCoordinate = getBytesPerCoordinate(p->datatype);
    dst = &p->data.items[startCoord * bytesPerCoordinate];
	if(p->datatype == VG_PATH_DATATYPE_F)
	{
		d = (gctFLOAT*)dst;
		s = (const gctFLOAT*)pathData;
		for(i=0; i<numCoords; i++)
			*d++ = (gctFLOAT)inputFloat(*s++);
	}
	else
	{
		gcoOS_MemCopy(dst, pathData, numCoords*bytesPerCoordinate);
	}

	PathDirty(p, VGTessPhase_ALL);

	gcmFOOTER_NO();
}



void  vgAppendPath(VGPath dstPath, VGPath srcPath)
{
    _VGPath  *dstP;
    _VGPath  *srcP;
	_VGContext* context;

	gcmHEADER_ARG("dstPath=%d srcPath=%d", dstPath, srcPath);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGAPPENDPATH, 0);

    dstP  = _VGPATH(context, dstPath);
    srcP  = _VGPATH(context, srcPath);

	OVG_IF_ERROR(!dstP || !srcP, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);


    OVG_IF_ERROR(!( dstP->capabilities & VG_PATH_CAPABILITY_APPEND_TO) ||
				!( srcP->capabilities & VG_PATH_CAPABILITY_APPEND_FROM), VG_PATH_CAPABILITY_ERROR, OVG_NO_RETVAL);

	if(srcP->segments.size)
	{
        _VGubyteArray newSegments;
        _VGubyteArray newData;
        gctINT32           newSegmentsSize;
        gctINT32           newDataSize;
        gctINT32           i = 0;
		gctINT32		   numCoord;

        ARRAY_CTOR(newSegments, context->os);
        ARRAY_CTOR(newData, context->os);

		newSegmentsSize = dstP->segments.size + srcP->segments.size;
        ARRAY_RESIZE(newSegments, newSegmentsSize);

        newDataSize = dstP->data.size + getNumCoordinates(srcP) * getBytesPerCoordinate(dstP->datatype);
        ARRAY_RESIZE(newData, newDataSize);
		if (newData.items == gcvNULL)
		{
			gcmFOOTER_NO();
			return;
		}

        if (dstP->segments.size)
            gcoOS_MemCopy(&newSegments.items[0], &dstP->segments.items[0], dstP->segments.size);
		if(srcP->segments.size)
            gcoOS_MemCopy(&newSegments.items[0] + dstP->segments.size, &srcP->segments.items[0], srcP->segments.size);

        if (dstP->data.size)
            gcoOS_MemCopy(&newData.items[0], &dstP->data.items[0], dstP->data.size);

		numCoord = getNumCoordinates(srcP);
		for(i = 0; i < numCoord; i++)
            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, i + getNumCoordinates(dstP), getCoordinate(srcP, i));

        gcmASSERT(newData.size == countNumCoordinates(&newSegments.items[0], newSegments.size) * getBytesPerCoordinate(dstP->datatype));

        ARRAY_DTOR(dstP->segments);
        ARRAY_DTOR(dstP->data);
        dstP->segments = newSegments;
        dstP->data = newData;

	    PathDirty(dstP, VGTessPhase_ALL);
	}

	gcmFOOTER_NO();
}


void  vgTransformPath(VGPath dstPath, VGPath srcPath)
{
    _VGPath  *dstP;
    _VGPath  *srcP;
    _VGMatrix3x3 matrix;
    gctINT32 srcCoord = 0;
    gctINT32 dstCoord;
#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
    gctINT32 oldDstCoord;
#endif
    gctINT32 oldSegmentSize, newSegmentSize;
    gctINT32 newDataSize;
    gctINT32 i;
    _VGVector2 s, o;
    VGPathSegment segment;
    VGPathAbsRel absRel;
    gctINT32 coords;
    _VGVector2 c, c0, c1, c2;
    _VGVector2 tc, tc0, tc1, tc2;
    _VGubyteArray newSegments;
    _VGubyteArray newData;

	gctINT32 numSrcCoords = 0;
	gctINT32 numDstCoords = 0;
	_VGContext* context;

	gcmHEADER_ARG("dstPath=%d srcPath=%d", dstPath, srcPath);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGTRANSFORMPATH, 0);

    dstP  = _VGPATH(context, dstPath);
    srcP  = _VGPATH(context, srcPath);

	OVG_IF_ERROR(!dstP || !srcP, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);


    OVG_IF_ERROR(!(dstP->capabilities & VG_PATH_CAPABILITY_TRANSFORM_TO) ||
				!( srcP->capabilities & VG_PATH_CAPABILITY_TRANSFORM_FROM), VG_PATH_CAPABILITY_ERROR, OVG_NO_RETVAL);

    matrix = context->pathUserToSurface;
    /*assert(isAffine(matrix));*/


	if(!srcP->segments.size)
	{
		gcmFOOTER_NO();
		return;
	}


	for(i=0;i<srcP->segments.size;i++)
	{
        segment = getPathSegment(srcP->segments.items[i]);
		coords = segmentToNumCoordinates(segment);
		numSrcCoords += coords;
		if(segment == VG_HLINE_TO || segment == VG_VLINE_TO)
			coords = 2;
		numDstCoords += coords;
	}

#if gcmIS_DEBUG(gcdDEBUG_ASSERT)
	oldDstCoord =
#endif
	dstCoord = getNumCoordinates(dstP);

    ARRAY_CTOR(newSegments, context->os);
    ARRAY_CTOR(newData, context->os);

    oldSegmentSize = dstP->segments.size;
    newSegmentSize = oldSegmentSize + srcP->segments.size;
    ARRAY_RESIZE(newSegments, newSegmentSize);
	if (newSegments.items == gcvNULL)
	{
		gcmFOOTER_NO();
		return;
	}

    newDataSize = dstP->data.size + numDstCoords * getBytesPerCoordinate(dstP->datatype);
    ARRAY_RESIZE(newData, newDataSize);
	if (newData.items == gcvNULL)
	{
		gcmFOOTER_NO();
		return;
	}

	/*copy old segments*/
	if(dstP->segments.size > 0)
		gcoOS_MemCopy(&newSegments.items[0], &dstP->segments.items[0], dstP->segments.size);

	/*copy old data*/
	if(dstP->data.size > 0)
        gcoOS_MemCopy(&newData.items[0], &dstP->data.items[0], dstP->data.size);


    V2_SET(s, 0.0f, 0.0f);
    V2_SET(o, 0.0f, 0.0f);

	for(i=0;i<srcP->segments.size;i++)
	{
        segment = getPathSegment(srcP->segments.items[i]);
        absRel = getPathAbsRel(srcP->segments.items[i]);
		coords = segmentToNumCoordinates(segment);

		switch(segment)
		{
		case VG_CLOSE_PATH:
		{
			gcmASSERT(coords == 0);
            V2_SETV(o, s);
			break;
		}

		case VG_MOVE_TO:
		{
			gcmASSERT(coords == 2);

            V2_SET(c, getCoordinate(srcP, srcCoord+0), getCoordinate(srcP, srcCoord+1));

			if (absRel == VG_ABSOLUTE)
			{
				AFFINE_TRANSFORM(tc, matrix, c);
			}
			else
			{
				AFFINE_TANGENT_TRANSFORM(tc, matrix, c);
				V2_ADDV(c, o);
			}

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.y);

            V2_SETV(s, c);
            V2_SETV(o, c);

			break;
		}

		case VG_LINE_TO:
		{
			gcmASSERT(coords == 2);

            V2_SET(c, getCoordinate(srcP, srcCoord+0), getCoordinate(srcP, srcCoord+1));

			if (absRel == VG_ABSOLUTE)
			{
				AFFINE_TRANSFORM(tc, matrix, c);
			}
			else
			{
				AFFINE_TANGENT_TRANSFORM(tc, matrix, c);
				V2_ADDV(c, o);
			}


            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.y);

            V2_SETV(o, c);
			break;
		}

		case VG_HLINE_TO:
		{
			gcmASSERT(coords == 1);

            V2_SET(c, getCoordinate(srcP, srcCoord+0), o.y);

			if (absRel == VG_ABSOLUTE)
			{
				c.y = o.y;
				AFFINE_TRANSFORM(tc, matrix, c);
			}
			else
			{
				c.y = 0;
				AFFINE_TANGENT_TRANSFORM(tc, matrix, c);
				V2_ADDV(c, o);
			}


            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.y);
            V2_SETV(o, c);
			segment = VG_LINE_TO;
			break;
		}

		case VG_VLINE_TO:
		{
			gcmASSERT(coords == 1);

            V2_SET(c, o.x, getCoordinate(srcP, srcCoord+0));

			if (absRel == VG_ABSOLUTE)
			{
				c.x = o.x;
				AFFINE_TRANSFORM(tc, matrix, c);
			}
			else
			{
				c.x = 0;
				AFFINE_TANGENT_TRANSFORM(tc, matrix, c);
				V2_ADDV(c, o);
			}


            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.y);
            V2_SETV(o, c);

			segment = VG_LINE_TO;
			break;
		}

		case VG_QUAD_TO:
		{
			gcmASSERT(coords == 4);

            V2_SET(c0, getCoordinate(srcP, srcCoord+0), getCoordinate(srcP, srcCoord+1));
            V2_SET(c1, getCoordinate(srcP, srcCoord+2), getCoordinate(srcP, srcCoord+3));


			if (absRel == VG_ABSOLUTE)
			{
				AFFINE_TRANSFORM(tc0, matrix, c0);
				AFFINE_TRANSFORM(tc1, matrix, c1);
			}
			else
			{
				AFFINE_TANGENT_TRANSFORM(tc0, matrix, c0);
				AFFINE_TANGENT_TRANSFORM(tc1, matrix, c1);
				V2_ADDV(c1, o);
			}

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc0.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc0.y);

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc1.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc1.y);

            V2_SETV(o, c1);
			break;
		}

		case VG_CUBIC_TO:
		{
			gcmASSERT(coords == 6);

            V2_SET(c0, getCoordinate(srcP, srcCoord+0), getCoordinate(srcP, srcCoord+1));
            V2_SET(c1, getCoordinate(srcP, srcCoord+2), getCoordinate(srcP, srcCoord+3));
            V2_SET(c2, getCoordinate(srcP, srcCoord+4), getCoordinate(srcP, srcCoord+5));

			if (absRel == VG_ABSOLUTE)
			{
				AFFINE_TRANSFORM(tc0, matrix, c0);
				AFFINE_TRANSFORM(tc1, matrix, c1);
				AFFINE_TRANSFORM(tc2, matrix, c2);
			}
			else
			{
				AFFINE_TANGENT_TRANSFORM(tc0, matrix, c0);
				AFFINE_TANGENT_TRANSFORM(tc1, matrix, c1);
				AFFINE_TANGENT_TRANSFORM(tc2, matrix, c2);
				V2_ADDV(c2, o);
			}

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc0.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc0.y);

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc1.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc1.y);

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc2.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc2.y);

            V2_SETV(o, c2);
			break;
		}

		case VG_SQUAD_TO:
		{
			gcmASSERT(coords == 2);

            V2_SET(c1, getCoordinate(srcP, srcCoord+0), getCoordinate(srcP, srcCoord+1));

			if (absRel == VG_ABSOLUTE)
			{
				AFFINE_TRANSFORM(tc1, matrix, c1);
			}
			else
			{
				AFFINE_TANGENT_TRANSFORM(tc1, matrix, c1);
				V2_ADDV(c1, o);
			}

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc1.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc1.y);

		    V2_SETV(o, c1);
			break;
		}

		case VG_SCUBIC_TO:
		{
			gcmASSERT(coords == 4);

            V2_SET(c1, getCoordinate(srcP, srcCoord+0), getCoordinate(srcP, srcCoord+1));
            V2_SET(c2, getCoordinate(srcP, srcCoord+2), getCoordinate(srcP, srcCoord+3));


			if (absRel == VG_ABSOLUTE)
			{
				AFFINE_TRANSFORM(tc1, matrix, c1);
				AFFINE_TRANSFORM(tc2, matrix, c2);
			}
			else
			{
				AFFINE_TANGENT_TRANSFORM(tc1, matrix, c1);
				AFFINE_TANGENT_TRANSFORM(tc2, matrix, c2);
				V2_ADDV(c2, o);
			}

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc1.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc1.y);

            setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc2.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc2.y);

            V2_SETV(o, c2);
			break;
		}

		default:
		{
            gctFLOAT rh, rv, rot;
            _VGVector2 p, q;
            _VGVector2 h, hp;
            gctBOOL       swapped = gcvFALSE;
            gctFLOAT hlen, hplen;
            _VGMatrix3x3 u, matrixOut;

			gcmASSERT(segment == VG_SCCWARC_TO || segment == VG_SCWARC_TO ||
					  segment == VG_LCCWARC_TO || segment == VG_LCWARC_TO);
			gcmASSERT(coords == 5);
			rh = getCoordinate(srcP, srcCoord+0);
			rv = getCoordinate(srcP, srcCoord+1);
			rot = getCoordinate(srcP, srcCoord+2);

            V2_SET(c, getCoordinate(srcP, srcCoord+3), getCoordinate(srcP, srcCoord+4));

			rot = DEG_TO_RAD(rot);

            _vgSetMatrix(&u,   (gctFLOAT)gcoMATH_Cosine(rot)*rh,   -(gctFLOAT)gcoMATH_Sine(rot)*rv,  0,
						   (gctFLOAT)gcoMATH_Sine(rot)*rh,  (gctFLOAT)gcoMATH_Cosine(rot)*rv,    0,
						   0,                        0,                          1);

            MultMatrix(&matrix, &u, &matrixOut);
            gcoOS_MemCopy(&u, &matrixOut, sizeof(u));

            FORCE_AFFINE(u.m);

            V2_SET(p, u.m[0][0], u.m[1][0]);
            V2_SET(q, u.m[1][1], -u.m[0][1]);

			if((V2_DOT(p,p)) < (V2_DOT(q,q)))
			{
				SWAP(&p.x, &q.x);
				SWAP(&p.y, &q.y);
				swapped = gcvTRUE;
			}


            V2_SETV(h, p);
            V2_ADDV(h, q);
            V2_MUL(h, 0.5f);

            V2_SETV(hp, p);
            V2_SUBV(hp, q);
            V2_MUL(hp, 0.5f);


            hlen = V2_NORM(h);
			hplen = V2_NORM(hp);

			rh = hlen + hplen;
			rv = hlen - hplen;

            V2_MUL(h, hplen);
            V2_MUL(hp, hlen);
            V2_ADDV(h, hp);


            hlen = V2_DOT(h,h);
			if(hlen == 0.0f)
				rot = 0.0f;
			else
			{
                V2_NORMALIZE(h);
				rot = (gctFLOAT)gcoMATH_ArcCosine(h.x);
				if(h.y < 0.0f)
					rot = 2.0f*gcdPI - rot;
			}
			if(swapped)
				rot += gcdPI*0.5f;

			if (absRel == VG_ABSOLUTE)
			{
				AFFINE_TRANSFORM(tc, matrix, c);
			}
			else
			{
				AFFINE_TANGENT_TRANSFORM(tc, matrix, c);
				V2_ADDV(c, o);
			}

			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, rh);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, rv);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, RAD_TO_DEG(rot));
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.x);
			setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, dstCoord++, tc.y);

            V2_SETV(o, c);


            if (MATRIX_DET(matrix) < 0)
            {
                switch (segment)
                {
                case VG_SCCWARC_TO: segment = VG_SCWARC_TO;     break;
                case VG_SCWARC_TO:  segment = VG_SCCWARC_TO;    break;
                case VG_LCCWARC_TO: segment = VG_LCWARC_TO;     break;
                case VG_LCWARC_TO:  segment = VG_LCCWARC_TO;    break;
                default:                                        break;
                }
            }
			break;
		}
		}


        newSegments.items[oldSegmentSize + i] = (_VGuint8)(segment | absRel);

		srcCoord += coords;
	}
	gcmASSERT(srcCoord == numSrcCoords);
	gcmASSERT(dstCoord == oldDstCoord + numDstCoords);

    gcmASSERT(newData.size == countNumCoordinates(&newSegments.items[0], newSegments.size) * getBytesPerCoordinate(dstP->datatype));

    ARRAY_DTOR(dstP->segments);
    ARRAY_DTOR(dstP->data);
    dstP->segments = newSegments;
    dstP->data = newData;

	PathDirty(dstP, VGTessPhase_ALL);

	gcmFOOTER_NO();
}


void StencilFunc(
    _VGContext *context,
	gceCOMPARE compare,
	VGint ref,
	VGuint mask
	)
{
	gcmHEADER_ARG("context=0x%x compare=%d ref=%d mask=%u", context, compare, ref, mask);

	gcmVERIFY_OK(gco3D_SetStencilCompare(context->engine, gcvSTENCIL_FRONT, compare));
	gcmVERIFY_OK(gco3D_SetStencilReference(context->engine, (gctUINT8) ref, gcvTRUE));
	gcmVERIFY_OK(gco3D_SetStencilReference(context->engine, (gctUINT8) ref, gcvFALSE));
	gcmVERIFY_OK(gco3D_SetStencilMask(context->engine, (gctUINT8) mask));
	gcmVERIFY_OK(gco3D_SetStencilWriteMask(context->engine, (gctUINT8) mask));

	gcmFOOTER_NO();
}

void StencilOp(
          _VGContext *context,
          gceSTENCIL_OPERATION fail,
          gceSTENCIL_OPERATION zfail,
          gceSTENCIL_OPERATION zpass
          )
{
	gcmHEADER_ARG("context=0x%x fail=%d zfail=%d zpass=%d", context, fail, zfail, zpass);

	gcmVERIFY_OK(gco3D_SetStencilFail(context->engine, gcvSTENCIL_FRONT, fail));
	gcmVERIFY_OK(gco3D_SetStencilDepthFail(context->engine, gcvSTENCIL_FRONT, zfail));
	gcmVERIFY_OK(gco3D_SetStencilPass(context->engine, gcvSTENCIL_FRONT, zpass));

	gcmFOOTER_NO();
}


void ColorMask(
    _VGContext *context,
    VGboolean red,
	VGboolean green,
	VGboolean blue,
	VGboolean alpha)
{
	gctUINT8 enable = (red   ? 0x4 : 0x0) |
		     (green ? 0x2 : 0x0) |
		     (blue  ? 0x1 : 0x0) |
		     (alpha ? 0x8 : 0x0);

	gcmHEADER_ARG("context=0x%x red=%s green=%s blue=%s alpha=%s", context,
		red ? "TRUE" : "FALSE", green ? "TRUE" : "FALSE", blue ? "TRUE" : "FALSE", alpha ? "TRUE" : "FALSE");

	gcmVERIFY_OK(gco3D_SetColorWrite(context->engine, enable));

	gcmFOOTER_NO();
}

void EnableStencil(_VGContext *context, VGboolean enable)
{
	gcmHEADER_ARG("context=0x%x enable=%s", context, enable ? "TRUE" : "FALSE");

    if (!enable)
    {
        gcmVERIFY_OK(gco3D_SetStencilMode(context->engine, gcvSTENCIL_NONE));
    }
    else
    {
        gcmVERIFY_OK(gco3D_SetStencilMode(context->engine, gcvSTENCIL_SINGLE_SIDED));
    }

	gcmFOOTER_NO();
}

gctBOOL TessFlatten(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix, gctFLOAT strokeWidth)
{
    _VGTessPhase        phase;
    _VGFlattenBuffer    *flattened;

	gcmHEADER_ARG("context=0x%x path=0x%x matrix=0x%x strokeWidth=%f",
				   context, path, matrix, strokeWidth);

    if (strokeWidth == 0)
    {
        phase = VGTessPhase_FlattenFill;
        flattened = &path->tessellateResult.flattenedFillPath;
    }
	else
    {
        phase = VGTessPhase_FlattenStroke;
        flattened = &path->tessellateResult.flattenedStrokePath;
    }

#ifdef FLATTEN_PROCESS_OPTIM
	if (IsPathDirty(path, VGTessPhase_FlattenPre))
	{
		PathClean(path, VGTessPhase_FlattenPre);
		PreprocessPath(path);
	}
#endif

	if (IsPathDirty(path, phase))
    {
        PathClean(path, phase);

		if (_FlattenPath(context, path, TESSTYPE(strokeWidth), flattened) <= 0)
        {
            TessPhaseFailed(path, phase);
			gcmFOOTER_ARG("return=%s", "FALSE");
            return gcvFALSE;
        }
        else
        {
            TessPhaseClean(path, phase);
			gcmFOOTER_ARG("return=%s", "TRUE");
            return gcvTRUE;
        }
    }

    if (IsTessPhaseFailed(path, phase))
    {
		gcmFOOTER_ARG("return=%s", "FALSE");
        return gcvFALSE;
    }

	gcmFOOTER_ARG("return=%s", "TRUE");
    return gcvTRUE;
}

gctBOOL TessFillPath(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix)
{
	gcmHEADER_ARG("context=0x%x path=0x%x matrix=0x%x",
				   context, path, matrix);

    if (IsPathDirty(path, VGTessPhase_Fill))
    {
        PathClean(path, VGTessPhase_Fill);

        if (!Tessellate(context, path, matrix))
        {
            TessPhaseFailed(path, VGTessPhase_Fill);
			gcmFOOTER_ARG("return=%s", "FALSE");
            return gcvFALSE;
        }
        else
        {
#ifdef CHECK_FILL_PRIMITIVE
			OVGCheckFillPrimitives(&path->tessellateResult);
#endif
            TessPhaseClean(path, VGTessPhase_Fill);
			gcmFOOTER_ARG("return=%s", "TRUE");
            return gcvTRUE;
        }
    }


    if (IsTessPhaseFailed(path, VGTessPhase_Fill))
    {
		gcmFOOTER_ARG("return=%s", "FALSE");
        return gcvFALSE;
    }

	gcmFOOTER_ARG("return=%s", "TRUE");
    return gcvTRUE;
}

gctBOOL TessStrokePath(_VGContext * context, _VGPath* path, _VGMatrix3x3 *matrix)
{
	gcmHEADER_ARG("context=0x%x path=0x%x matrix=0x%x",
				   context, path, matrix);

    if (IsPathDirty(path, VGTessPhase_Stroke))
    {
        PathClean(path, VGTessPhase_Stroke);

        if (TessellateStroke(context, path) <= 0)
        {
            TessPhaseFailed(path, VGTessPhase_Stroke);
			gcmFOOTER_ARG("return=%s", "FALSE");
            return gcvFALSE;
        }
        else
        {
#ifdef CHECK_STROKE_PRIMITIVE
			OVGCheckStrokePrimitives(&path->tessellateResult);
#endif
            TessPhaseClean(path, VGTessPhase_Stroke);
			gcmFOOTER_ARG("return=%s", "TRUE");
            return gcvTRUE;
        }
    }

    if (IsTessPhaseFailed(path, VGTessPhase_Stroke))
    {
		gcmFOOTER_ARG("return=%s", "FALSE");
        return gcvFALSE;
    }

	gcmFOOTER_ARG("return=%s", "TRUE");
    return gcvTRUE;
}



void CheckContextParam(_VGContext *context, _VGPath* path, _VGMatrix3x3 *matrix, VGbitfield	paintModes)
{
#if !USE_SCAN_LINE
	VGfloat scale[2];
#endif

	gcmHEADER_ARG("context=0x%x path=0x%x matrix=0x%x paintModes=%d",
		context, path, matrix, paintModes);

	scale[0] = matrix->m[0][0] * matrix->m[0][0] + matrix->m[1][0] * matrix->m[1][0];
	scale[1] = matrix->m[0][1] * matrix->m[0][1] + matrix->m[1][1] * matrix->m[1][1];

	context->tessContext.strokeScale = TESS_SQRT(gcmMAX(scale[0], scale[1]));

	if ((paintModes & VG_FILL_PATH) &&
		path->fillRule != context->fillRule)
	{
		path->fillRule = context->fillRule;
		PathDirty(path, (_VGTessPhase) (VGTessPhase_FlattenFill | VGTessPhase_Fill));
	}

	if (paintModes & VG_STROKE_PATH)
	{
		if (TESS_ABS(path->strokeLineWidth - context->strokeLineWidth) * context->tessContext.strokeScale > 0.2f) /*0.45f)	In AA mode, we have to use a smaller value such as <= 0.2f.*/
		{
			path->strokeLineWidth = context->strokeLineWidth;
			PathDirty(path, (_VGTessPhase) (VGTessPhase_FlattenStroke | VGTessPhase_Stroke));
		}

		if (path->strokeCapStyle != context->strokeCapStyle)
		{
			path->strokeCapStyle = context->strokeCapStyle;
			PathDirty(path, (_VGTessPhase) (VGTessPhase_FlattenStroke | VGTessPhase_Stroke));
		}

		if (path->strokeJoinStyle != context->strokeJoinStyle)
		{
			path->strokeJoinStyle = context->strokeJoinStyle;
			PathDirty(path, (_VGTessPhase) (VGTessPhase_FlattenStroke | VGTessPhase_Stroke));
		}

		if (path->strokeMiterLimit != context->strokeMiterLimit)
		{
			path->strokeMiterLimit = context->strokeMiterLimit;
			PathDirty(path, (_VGTessPhase) (VGTessPhase_FlattenStroke | VGTessPhase_Stroke));
		}

		if (path->dashPhase != context->strokeDashPhase)
		{
			path->dashPhase = context->strokeDashPhase;
			PathDirty(path, (_VGTessPhase) (VGTessPhase_FlattenStroke | VGTessPhase_Stroke));
		}

		if (path->dashPhaseReset != context->strokeDashPhaseReset)
		{
			path->dashPhaseReset = context->strokeDashPhaseReset;
			PathDirty(path, (_VGTessPhase) (VGTessPhase_FlattenStroke | VGTessPhase_Stroke));
		}
	}

#if USE_SCAN_LINE
	if (path->transformRS[0] != matrix->m[0][0] ||
		path->transformRS[1] != matrix->m[0][1] ||
		path->transformRS[2] != matrix->m[1][0] ||
		path->transformRS[3] != matrix->m[1][1])
	{
		path->transformRS[0] = matrix->m[0][0];
		path->transformRS[1] = matrix->m[0][1];
		path->transformRS[2] = matrix->m[1][0];
		path->transformRS[3] = matrix->m[1][1];
#else
	if (scale[0] > path->transformScale[0] * 3.0f||
		scale[1] > path->transformScale[1] * 3.0f)
	{
		path->transformScale[0] = scale[0];
		path->transformScale[1] = scale[1];
#endif
		PathDirty(path, VGTessPhase_ALL);
	}

	gcmFOOTER_NO();
}

void ClearTessellateResult(_VGContext * context, _VGPath* path)
{
	gcmHEADER_ARG("context=0x%x path=0x%x", context, path);


    if(!IsPathDirty(path, VGTessPhase_ALL))
    {
        /*free the segment info and flatten buffer*/
        ARRAY_DTOR(path->segmentsInfo);
        _VGFlattenBufferDtor(context->os, &path->tessellateResult.flattenedFillPath);
        _VGFlattenBufferDtor(context->os, &path->tessellateResult.flattenedStrokePath);
    }

	gcmFOOTER_NO();
}

void  vgDrawPath(VGPath path, VGbitfield paintModes)
{
    _VGPath   *p = gcvNULL;
	_VGContext* context;

	gcmHEADER_ARG("path=%d paintModes=0x%x", path, paintModes);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGDRAWPATH, 0);
	vgmPROFILE(context, VG_PROFILER_PRIMITIVE_TYPE, VG_PATH);
	vgmPROFILE(context, VG_PROFILER_PRIMITIVE_COUNT, 1);
	if (paintModes & VG_STROKE_PATH)
		vgmPROFILE(context, VG_PROFILER_STROKE, 1);
	if (paintModes & VG_FILL_PATH)
		vgmPROFILE(context, VG_PROFILER_FILL, 1);

    p = _VGPATH(context, path);

    OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

	OVG_IF_ERROR(!paintModes || (paintModes & ~(VG_FILL_PATH | VG_STROKE_PATH)),
                 VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	/* need to add error check(G20602) */
	vgshDrawPath(context, p, paintModes, &context->pathUserToSurface);

	/* PSC. */
#if vgvUSE_PSC
	_PSCManagerHit(path, &context->pscm, context->os);
#endif
	gcmFOOTER_NO();
}

void normalizeForInterpolation(_VGContext *context, _VGPath* dstPath, _VGPath* srcPath)
{
	gctINT32 numSrcCoords = 0;
	gctINT32 numDstCoords = 0;
    VGPathSegment segment;
    VGPathAbsRel absRel;
    gctINT32 coords;

    gctINT32 srcCoord = 0;
	gctINT32 dstCoord = 0;
    gctINT32 i;
    _VGVector2 s, o, p;

    _VGVector2 d0, d1, c0, c1, c2, c;

    gctFLOAT rh, rv, rot;
	VGint temp;

	gcmHEADER_ARG("context=0x%x dstPath=0x%x srcPath=0x%x", context, dstPath, srcPath);

	gcmASSERT(srcPath);
	gcmASSERT(srcPath != dstPath);



	for(i=0; i<srcPath->segments.size;i++)
	{
        segment = getPathSegment(srcPath->segments.items[i]);
		coords = segmentToNumCoordinates(segment);
		numSrcCoords += coords;
		switch(segment)
		{
		case VG_CLOSE_PATH:
		case VG_MOVE_TO:
		case VG_LINE_TO:
			break;

		case VG_HLINE_TO:
		case VG_VLINE_TO:
			coords = 2;
			break;

		case VG_QUAD_TO:
		case VG_CUBIC_TO:
		case VG_SQUAD_TO:
		case VG_SCUBIC_TO:
			coords = 6;
			break;

		default:
			gcmASSERT(segment == VG_SCCWARC_TO || segment == VG_SCWARC_TO ||
					  segment == VG_LCCWARC_TO || segment == VG_LCWARC_TO);
			break;
		}
		numDstCoords += coords;
	}

    ARRAY_RESIZE(dstPath->segments, srcPath->segments.size);

    temp = numDstCoords * getBytesPerCoordinate(VG_PATH_DATATYPE_F);
    ARRAY_RESIZE(dstPath->data, temp);

	if ((dstPath->segments.items == gcvNULL) || (dstPath->data.items == gcvNULL))
	{
		gcmFOOTER_NO();
		return;
	}

    V2_SET(s, 0, 0);
    V2_SET(o, 0, 0);
    V2_SET(p, 0, 0);

    for(i=0;i<srcPath->segments.size;i++)
	{
		segment = getPathSegment(srcPath->segments.items[i]);
        absRel = getPathAbsRel(srcPath->segments.items[i]);
		coords = segmentToNumCoordinates(segment);

		switch(segment)
		{
		case VG_CLOSE_PATH:
		{
			gcmASSERT(coords == 0);
            V2_SETV(p, s);
            V2_SETV(o, s);

			break;
		}

		case VG_MOVE_TO:
		{
			gcmASSERT(coords == 2);
			V2_SET(c, getCoordinate(srcPath, srcCoord+0), getCoordinate(srcPath, srcCoord+1));

            if(absRel == VG_RELATIVE)
				V2_ADDV(c, o);

            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.y);

            V2_SETV(s, c);
            V2_SETV(p, c);
            V2_SETV(o, c);
			break;
		}

		case VG_LINE_TO:
		{
			gcmASSERT(coords == 2);

            V2_SET(c, getCoordinate(srcPath, srcCoord+0), getCoordinate(srcPath, srcCoord+1));

			if(absRel == VG_RELATIVE)
				V2_ADDV(c, o);

            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.y);

            V2_SETV(p, c);
            V2_SETV(o, c);

			break;
		}

		case VG_HLINE_TO:
		{
			gcmASSERT(coords == 1);

            V2_SET(c, getCoordinate(srcPath, srcCoord+0), o.y);

			if(absRel == VG_RELATIVE)
				c.x += o.x;
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.y);

            V2_SETV(p, c);
            V2_SETV(o, c);

			segment = VG_LINE_TO;
			break;
		}

		case VG_VLINE_TO:
		{
			gcmASSERT(coords == 1);
            V2_SET(c, o.x, getCoordinate(srcPath, srcCoord+0));
			if(absRel == VG_RELATIVE)
				c.y += o.y;

            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.y);

            V2_SETV(p, c);
            V2_SETV(o, c);

			segment = VG_LINE_TO;
			break;
		}

		case VG_QUAD_TO:
		{
			gcmASSERT(coords == 4);

            V2_SET(c0, getCoordinate(srcPath, srcCoord+0), getCoordinate(srcPath, srcCoord+1));
            V2_SET(c1, getCoordinate(srcPath, srcCoord+2), getCoordinate(srcPath, srcCoord+3));
			if(absRel == VG_RELATIVE)
			{
                V2_ADDV(c0, o);
                V2_ADDV(c1, o);
			}

            V2_SETV(d0, c0);
            V2_SETV(d1, c0);

            V2_MUL(d0, 2.0f);
            V2_ADDV(d0, o);
            V2_MUL(d0, 1.0f/3.0f);


            V2_MUL(d1, 2.0f);
            V2_ADDV(d1, c1);
            V2_MUL(d1, 1.0f/3.0f);

            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, d0.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, d0.y);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, d1.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, d1.y);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c1.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c1.y);


            V2_SETV(p, c0);
            V2_SETV(o, c1);

			segment = VG_CUBIC_TO;
			break;
		}

		case VG_CUBIC_TO:
		{
			gcmASSERT(coords == 6);

            V2_SET(c0, getCoordinate(srcPath, srcCoord+0), getCoordinate(srcPath, srcCoord+1));
            V2_SET(c1, getCoordinate(srcPath, srcCoord+2), getCoordinate(srcPath, srcCoord+3));
            V2_SET(c2, getCoordinate(srcPath, srcCoord+4), getCoordinate(srcPath, srcCoord+5));


            if(absRel == VG_RELATIVE)
			{
                V2_ADDV(c0, o);
                V2_ADDV(c1, o);
                V2_ADDV(c2, o);

			}
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c0.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c0.y);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c1.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c1.y);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c2.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c2.y);

            V2_SETV(p, c1);
            V2_SETV(o, c2);

			break;
		}

		case VG_SQUAD_TO:
		{
			gcmASSERT(coords == 2);

            V2_SETV(c0, o);
            V2_MUL(c0, 2.0f);
            V2_SUBV(c0, p);

            V2_SET(c1, getCoordinate(srcPath, srcCoord+0), getCoordinate(srcPath, srcCoord+1));
			if(absRel == VG_RELATIVE)
			    V2_ADDV(c1, o);

            V2_SETV(d0, c0);
            V2_MUL(d0, 2.0f);
            V2_ADDV(d0, o);
            V2_MUL(d0, 1.0f/3.0f);

            V2_SETV(d1, c0);
            V2_MUL(d1, 2.0f);
            V2_ADDV(d1, c1);
            V2_MUL(d1, 1.0f/3.0f);

            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, d0.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, d0.y);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, d1.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, d1.y);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c1.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c1.y);


            V2_SETV(p, c0);
            V2_SETV(o, c1);

            segment = VG_CUBIC_TO;
			break;
		}

		case VG_SCUBIC_TO:
		{
			gcmASSERT(coords == 4);

            V2_SETV(c0, o);
            V2_MUL(c0, 2.0f);
            V2_SUBV(c0, p);
            V2_SET(c1, getCoordinate(srcPath, srcCoord+0), getCoordinate(srcPath, srcCoord+1));
            V2_SET(c2, getCoordinate(srcPath, srcCoord+2), getCoordinate(srcPath, srcCoord+3));

            if(absRel == VG_RELATIVE)
			{
                V2_ADDV(c1, o);
                V2_ADDV(c2, o);

			}
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c0.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c0.y);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c1.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c1.y);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c2.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c2.y);

            V2_SETV(p, c1);
            V2_SETV(o, c2);

			segment = VG_CUBIC_TO;
			break;
		}

		default:
		{
			gcmASSERT(segment == VG_SCCWARC_TO || segment == VG_SCWARC_TO ||
					  segment == VG_LCCWARC_TO || segment == VG_LCWARC_TO);
			gcmASSERT(coords == 5);

			rh = getCoordinate(srcPath, srcCoord+0);
			rv = getCoordinate(srcPath, srcCoord+1);
			rot = getCoordinate(srcPath, srcCoord+2);

            V2_SET(c, getCoordinate(srcPath, srcCoord+3), getCoordinate(srcPath, srcCoord+4));

            if(absRel == VG_RELATIVE)
				V2_ADDV(c, o);

            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, rh);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, rv);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, rot);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.x);
            setCoordinate(&dstPath->data, dstPath->datatype, dstPath->scale, dstPath->bias, dstCoord++, c.y);


            V2_SETV(p, c);
            V2_SETV(o, c);

			break;
		}
		}

		dstPath->segments.items[i] = (_VGuint8)(segment | VG_ABSOLUTE);
		srcCoord += coords;
	}

	gcmASSERT(srcCoord == numSrcCoords);
	gcmASSERT(dstCoord == numDstCoords);

	gcmFOOTER_NO();
}

VGboolean  vgInterpolatePath(VGPath dstPath, VGPath startPath, VGPath endPath, VGfloat amount)
{
    _VGPath *dstP, *startP, *endP;
    _VGPath start, end;
    gctINT32 oldSegmentSize;
    gctINT32 i;
    gctINT32 oldNumCoords;
	gctINT32 numCoords;
    _VGubyteArray newSegments;
    _VGubyteArray newData;
	_VGContext* context;
	VGint temp;

	gcmHEADER_ARG("dstPath=%d startPath=%d endPath=%d amount=%f",
				  dstPath, startPath, endPath, amount);
	OVG_GET_CONTEXT(VG_FALSE);

	vgmPROFILE(context, VGINTERPOLATEPATH, 0);

    dstP  = _VGPATH(context, dstPath);
    startP  = _VGPATH(context, startPath);
    endP  = _VGPATH(context, endPath);

	OVG_IF_ERROR(!dstP || !startP || !endP, VG_BAD_HANDLE_ERROR, VG_FALSE);

    OVG_IF_ERROR(!(dstP->capabilities & VG_PATH_CAPABILITY_INTERPOLATE_TO) ||
				!(startP->capabilities & VG_PATH_CAPABILITY_INTERPOLATE_FROM) ||
				!(endP->capabilities & VG_PATH_CAPABILITY_INTERPOLATE_FROM), VG_PATH_CAPABILITY_ERROR, VG_FALSE);


	if(!startP->segments.size || startP->segments.size != endP->segments.size)
    {
		gcmFOOTER_ARG("return=%s", "VG_FALSE");
		return VG_FALSE;
    }


    _VGPathCtor(context->os, &start);
    normalizeForInterpolation(context, &start, startP);

    _VGPathCtor(context->os, &end);
    normalizeForInterpolation(context, &end, endP);

	if(start.data.size != end.data.size || start.segments.size != end.segments.size ||
	   start.segments.items == gcvNULL || start.segments.items == gcvNULL)
    {
        _VGPathDtor(context->os, &start);
        _VGPathDtor(context->os, &end);
		gcmFOOTER_ARG("return=%s", "VG_FALSE");
		return VG_FALSE;
    }

    oldNumCoords = getNumCoordinates(dstP);
    oldSegmentSize = dstP->segments.size;

    ARRAY_CTOR(newSegments, context->os);
    ARRAY_CTOR(newData, context->os);

    ARRAY_RESIZE(newSegments, oldSegmentSize + start.segments.size);
    temp = dstP->data.size + start.data.size * getBytesPerCoordinate(dstP->datatype) / getBytesPerCoordinate(start.datatype);
    ARRAY_RESIZE(newData, temp);

	if ((newSegments.items == gcvNULL) || (newData.items == gcvNULL))
	{
		gcmFOOTER_ARG("return=%s", "VG_FALSE");
		return VG_FALSE;
	}

	if(dstP->segments.size > 0)
		gcoOS_MemCopy(&newSegments.items[0], &dstP->segments.items[0], dstP->segments.size);


	if(dstP->data.size > 0)
        gcoOS_MemCopy(&newData.items[0], &dstP->data.items[0], dstP->data.size);

	for(i=0;i<start.segments.size; i++)
	{
		VGPathSegment s = getPathSegment(start.segments.items[i]);
		VGPathSegment e = getPathSegment(end.segments.items[i]);

		if(s == VG_SCCWARC_TO || s == VG_SCWARC_TO || s == VG_LCCWARC_TO || s == VG_LCWARC_TO)
		{
			if(e != VG_SCCWARC_TO && e != VG_SCWARC_TO && e != VG_LCCWARC_TO && e != VG_LCWARC_TO)
				OVG_RETURN(VG_FALSE);
			if(amount < 0.5f)
                newSegments.items[oldSegmentSize + i] = start.segments.items[i];
			else
                newSegments.items[oldSegmentSize + i] = end.segments.items[i];
		}
		else
		{
			if(s != e)
				OVG_RETURN(VG_FALSE);
            newSegments.items[oldSegmentSize + i] = start.segments.items[i];
		}
	}

	numCoords = getNumCoordinates(&start);
	for(i = 0; i < numCoords; i++)
        setCoordinate(&newData, dstP->datatype, dstP->scale, dstP->bias, oldNumCoords + i, getCoordinate(&start, i) * (1.0f - amount) + getCoordinate(&end, i) * amount);

	gcmASSERT(newData.size == countNumCoordinates(&newSegments.items[0], newSegments.size) * getBytesPerCoordinate(dstP->datatype));


    _VGPathDtor(context->os, &start);
    _VGPathDtor(context->os, &end);

    ARRAY_DTOR(dstP->segments);
    ARRAY_DTOR(dstP->data);
    dstP->segments = newSegments;
    dstP->data = newData;

	PathDirty(dstP, VGTessPhase_ALL);

	gcmFOOTER_ARG("return=%s", "VG_TRUE");
	return VG_TRUE;
}


VGfloat  vgPathLength(VGPath path, VGint startSegment, VGint numSegments)
{
    _VGPath *p = gcvNULL;
    VGfloat result = -1.0f;
	_VGContext* context;

	gcmHEADER_ARG("path=%d startSegment=%d numSegments=%d", path, startSegment, numSegments);
	OVG_GET_CONTEXT(result);

	vgmPROFILE(context, VGPATHLENGTH, 0);

    p  = _VGPATH(context, path);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, result);
    OVG_IF_ERROR(!(p->capabilities & VG_PATH_CAPABILITY_PATH_LENGTH), VG_PATH_CAPABILITY_ERROR, result);
    OVG_IF_ERROR(((startSegment < 0) ||
				  (startSegment > p->segments.size - 1) ||
				  (numSegments <= 0) ||
				  (startSegment + numSegments > p->segments.size)),
                  VG_ILLEGAL_ARGUMENT_ERROR, result);

    /*force to do the flatten again and no flatten phase flag is cleaned.*/
    PathDirty(p, VGTessPhase_FlattenStroke);
	TessFlatten(context, p, &context->pathUserToSurface, -10.0f);
	if (p->tessellateResult.flattenedStrokePath.numPoints < 1)
	{
		gcmFOOTER_ARG("return=%f", 0.0f);
		return 0.0f;
	}
    result = GetPathLength(p, startSegment, numSegments);

	gcmFOOTER_ARG("return=%f", result);
    return result;
}

void  vgPointAlongPath(VGPath path, VGint startSegment, VGint numSegments, VGfloat distance,
					   VGfloat * x, VGfloat * y, VGfloat * tangentX, VGfloat * tangentY)
{
    _VGPath *p = gcvNULL;
    _VGTesstype tx, ty, ttx, tty;
	gctBOOL	inValid = gcvTRUE;
	gctINT32 i;
	_VGContext* context;

	gcmHEADER_ARG("path=%d startSegment=%d numSegments=%d distance=%f "
				  "x=0x%x y=0x%x tangentX=0x%x tangentY=0x%x",
				  path, startSegment, numSegments, distance, x, y, tangentX, tangentY);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGPOINTALONGPATH, 0);

    p  = _VGPATH(context, path);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!(p->capabilities & VG_PATH_CAPABILITY_POINT_ALONG_PATH) && (x != gcvNULL) && (y != gcvNULL), VG_PATH_CAPABILITY_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!(p->capabilities & VG_PATH_CAPABILITY_TANGENT_ALONG_PATH) && (tangentX != gcvNULL) && (tangentY != gcvNULL), VG_PATH_CAPABILITY_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(((startSegment < 0) ||
				  (startSegment > p->segments.size - 1) ||
				  (numSegments <= 0) ||
				  (startSegment + numSegments > p->segments.size) ||
                  !isAligned(x, 4) ||
				  !isAligned(y, 4) ||
				  !isAligned(tangentX, 4) ||
				  !isAligned(tangentY, 4)),
                  VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    /* _FlattenPath does not handle this special case, so it's done here: but may not very precise. */
	for (i = startSegment; i < startSegment + numSegments; i++)
	{
		if (p->segments.items[i] != VG_MOVE_TO)
		{
			inValid = gcvFALSE;
			break;
		}
	}

	if (inValid)
    {
		if (x != gcvNULL)
			*x = 0;
		if (y != gcvNULL)
			*y = 0;
		if (tangentX != gcvNULL)
			*tangentX = TESS2TEMPLATE(TESS_CONST_ONE);
		if (tangentY != gcvNULL)
			*tangentY = 0;
    }
    else
    {
        /*force to do the flatten again and no flatten phase flag is cleaned*/
        PathDirty(p, VGTessPhase_FlattenStroke);
		TessFlatten(context, p, &context->pathUserToSurface, -10.0f);
		if (p->tessellateResult.flattenedStrokePath.numPoints < 1)
		{
			if (x != gcvNULL)
				*x = 0;
			if (y != gcvNULL)
				*y = 0;
			if (tangentX != gcvNULL)
				*tangentX = TESS2TEMPLATE(TESS_CONST_ONE);
			if (tangentY != gcvNULL)
				*tangentY = 0;
		}
		else
		{
			GetPointAlongPath(p, startSegment, numSegments, TESSTYPE(distance), &tx, &ty, &ttx, &tty);

			if (x != gcvNULL)
				*x = tx;
			if (y != gcvNULL)
				*y = ty;
			if (tangentX != gcvNULL)
				*tangentX = ttx;
			if (tangentY != gcvNULL)
				*tangentY = tty;
		}
    }

	gcmFOOTER_NO();
}

void  vgPathBounds(VGPath path, VGfloat * minx, VGfloat * miny, VGfloat * width, VGfloat * height)
{
    _VGPath *p = gcvNULL;
	_VGContext* context;

	gcmHEADER_ARG("path=%d minx=0x%x miny=0x%x width=0x%x height=0x%x",
				  path, minx, miny, width, height);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGPATHBOUNDS, 0);

    p  = _VGPATH(context, path);

	OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!(p->capabilities & VG_PATH_CAPABILITY_PATH_BOUNDS), VG_PATH_CAPABILITY_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(!minx || !miny || !width || !height, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(!isAligned(minx,4) || !isAligned(miny,4) || !isAligned(width,4) || !isAligned(height,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    /*Bounds data does not rely on tessellated points, so forced dirty is not necessary.*/
    GetPathBounds(context, p, minx, miny, width, height);

	gcmFOOTER_NO();
}



void  vgPathTransformedBounds(VGPath path, VGfloat * minx, VGfloat * miny, VGfloat * width, VGfloat * height)
{
    _VGPath         *p          = gcvNULL;
    _VGMatrix3x3    *matrix     = gcvNULL;
    VGfloat         maxx, maxy;
	_VGContext* context;

	gcmHEADER_ARG("path=%d minx=0x%x miny=0x%x width=0x%x height=0x%x", path, minx, miny, width, height);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGPATHTRANSFORMEDBOUNDS, 0);

	p  = _VGPATH(context, path);

    OVG_IF_ERROR(!p, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR((minx == gcvNULL) || (miny == gcvNULL) || (width == gcvNULL) || (height == gcvNULL), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!isAligned(minx, 4) || !isAligned(miny, 4) || !isAligned(width, 4) || !isAligned(height, 4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!(p->capabilities & VG_PATH_CAPABILITY_PATH_TRANSFORMED_BOUNDS), VG_PATH_CAPABILITY_ERROR, OVG_NO_RETVAL);

    matrix = &context->pathUserToSurface;

	GetPathBounds(context, p, minx, miny, width, height);
    /* Do Flattening if necessary. */

    /*Transform Flattened points and Get the bounds.*/
    if (*width > -1)
    {
        gctINT32 i;
		_VGVector2 buff[5];
		buff[0].x = *minx;
		buff[0].y = *miny;
		buff[1].x = buff[0].x + *width;
		buff[1].y = buff[0].y;
		buff[2].x = buff[1].x;
		buff[2].y = buff[1].y + *height;
		buff[3].x = buff[0].x;
		buff[3].y = buff[2].y;

		AFFINE_TRANSFORM(buff[4], (*matrix), buff[0]);
		*minx = maxx = buff[4].x;
		*miny = maxy = buff[4].y;
		for (i = 1; i < 4; i++)
		{
			AFFINE_TRANSFORM(buff[4], (*matrix), buff[i]);
			*minx = gcmMIN(*minx, buff[4].x);
			*miny = gcmMIN(*miny, buff[4].y);
			maxx = gcmMAX(maxx, buff[4].x);
			maxy = gcmMAX(maxy, buff[4].y);
		}

		*width = maxx - *minx;
		*height = maxy - *miny;
    }

	gcmFOOTER_NO();
}

