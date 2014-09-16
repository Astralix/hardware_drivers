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

void _VGPaintCtor(gcoOS os, _VGPaint *paint)
{
	_VGGradientStop      gs;

	gcmHEADER_ARG("os=0x%x paint=0x%x",
				   os, paint);

    paint->paintType =  VG_PAINT_TYPE_COLOR;

    COLOR_SET(paint->paintColor, 0, 0, 0, 1, sRGBA);
	COLOR_SET(paint->inputPaintColor, 0, 0, 0, 1, sRGBA_PRE);
	COLOR_SET(paint->avgRampColor, 0.5, 0.5, 0.5, 1, sRGBA_PRE);
    paint->colorRampSpreadMode = VG_COLOR_RAMP_SPREAD_PAD;
	paint->colorRampTextureForamt = sRGBA;

    paint->linearGradient[0] = 0.0;
    paint->linearGradient[1] = 0.0;
    paint->linearGradient[2] = 1.0;
    paint->linearGradient[3] = 0.0;

	V2_SET(paint->udusq, 1.0, 0.0);

    paint->radialGradient[0] = 0.0;
    paint->radialGradient[1] = 0.0;
    paint->radialGradient[2] = 0.0;
    paint->radialGradient[3] = 0.0;
    paint->radialGradient[4] = 1.0;

    paint->inputRadialGradient[0] = 0.0;
    paint->inputRadialGradient[1] = 0.0;
    paint->inputRadialGradient[2] = 0.0;
    paint->inputRadialGradient[3] = 0.0;
    paint->inputRadialGradient[4] = 1.0;

	paint->radialC = 1.0;
	V2_SET(paint->radialA, 0.0, 0.0);
	V2_SET(paint->radialB, 1.0, 0.0);


    ARRAY_CTOR(paint->colorRampStops, os);
	ARRAY_ALLOCATE(paint->colorRampStops,  gcmMIN(2, MAX_COLOR_RAMP_STOPS));
	if (paint->colorRampStops.items == gcvNULL)
	{
		gcmFOOTER_NO();
		return;
	}

	gs.offset = 0.0f;
	COLOR_SET(gs.color, 0, 0, 0, 1, sRGBA);
	ARRAY_PUSHBACK(paint->colorRampStops, gs);
	gs.offset = 1.0f;
	COLOR_SET(gs.color, 1, 1, 1, 1, sRGBA);
	ARRAY_PUSHBACK(paint->colorRampStops, gs);

    ARRAY_CTOR(paint->inputColorRampStops, os);

	ARRAY_CTOR(paint->colorStops, os);
	ARRAY_ALLOCATE(paint->colorStops,  gcmMIN(2, MAX_COLOR_RAMP_STOPS) * 8);
	/*fixme BUG, initialize the colorstops*/

    _VGMatrixCtor(&paint->paintToUser);
    _VGMatrixCtor(&paint->linearGraMatrix);

    _VGMatrixCtor(&paint->radialGraMatrix);

    paint->patternTilingMode      = VG_TILE_FILL;
    paint->pattern                = gcvNULL;
    paint->colorRampPremultiplied = VG_TRUE;
    paint->dirty                  = OVGPaintDirty_ColorStop;
    paint->texSurface             = gcvNULL;
	paint->zeroFlag               = gcvFALSE;
	paint->alphaValue             = 0.0;
	paint->alphaChanged           = gcvFALSE;

	paint->lineStream			  = gcvNULL;

    _VGImageCtor(os, &paint->colorRamp);

	gcoOS_ZeroMemory(&paint->object, sizeof(_VGObject));

	gcmFOOTER_NO();
}

void _VGPaintDtor(gcoOS os, _VGPaint *paint)
{
	gcmHEADER_ARG("os=0x%x paint=0x%x",
				   os, paint);

    ARRAY_DTOR(paint->colorRampStops);
    ARRAY_DTOR(paint->inputColorRampStops);
	ARRAY_DTOR(paint->colorStops);
	if (paint->lineStream)
	{
		gcmVERIFY_OK(gcoSTREAM_Destroy(paint->lineStream));
	}
	if (paint->pattern)
	{
		VGObject_Release(os, &paint->pattern->object);
	}

    _VGImageDtor(os, &paint->colorRamp);

	gcmFOOTER_NO();
}


void PaintDirty(_VGPaint *paint, OVGPaintDirty dirty)
{
	gcmHEADER_ARG("paint=0x%x dirty=0x%x", paint, dirty);

    paint->dirty |=  dirty;

	gcmFOOTER_NO();
}

void PaintClean(_VGPaint *paint)
{
	gcmHEADER_ARG("paint=0x%x", paint);

	paint->dirty = OVGPaintDirty_None;

	gcmFOOTER_NO();
}

gctBOOL IsPaintDirty(_VGPaint *paint)
{
	gcmHEADER_ARG("paint=0x%x", paint);

#ifdef PAINT_NO_CACHE
	gcmFOOTER_ARG("return=%s", "TRUE");
    return gcvTRUE;
#else
	gcmFOOTER_ARG("return=%s", !(paint->dirty == OVGPaintDirty_None) ? "TRUE" : "FALSE");
    return !(paint->dirty == OVGPaintDirty_None);
#endif
}


VGPaint  vgCreatePaint(void)
{
	_VGPaint* paint = gcvNULL;
	_VGContext* context;

	gcmHEADER();
	OVG_GET_CONTEXT(VG_INVALID_HANDLE);

	vgmPROFILE(context, VGCREATEPAINT, 0);
    NEWOBJ(_VGPaint, context->os, paint);
    if (!paint)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
		gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
    }

    if (!vgshInsertObject(context, &paint->object, VGObject_Paint)) {

        DELETEOBJ(_VGPaint, context->os, paint);
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
		gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
    }

	VGObject_AddRef(context->os, &paint->object);

	gcmFOOTER_ARG("return=%d", paint->object.name);
    return (VGPath)paint->object.name;
}

void  vgDestroyPaint(VGPaint paint)
{
    _VGPaint *paintObjPtr;
	_VGContext* context;

	gcmHEADER_ARG("paint=%d", paint);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGDESTROYPAINT, 0);
    paintObjPtr = _VGPAINT(context, paint);

	OVG_IF_ERROR(!paintObjPtr, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

	vgshRemoveObject( context, &paintObjPtr->object);

	VGObject_Release(context->os, &paintObjPtr->object);

	gcmFOOTER_NO();
}

void  vgSetPaint(VGPaint paint, VGbitfield paintModes)
{
    _VGPaint *paintObjPtr;
	_VGContext* context;
	gcmHEADER_ARG("paint=%d paintModes=0x%x", paint, paintModes);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGSETPAINT, 0);
    paintObjPtr = _VGPAINT(context, paint);

    /*if paint == VG_INVALID_HANDLE then should reset the paint object to initial*/
	OVG_IF_ERROR((!paintObjPtr && paint != VG_INVALID_HANDLE), VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

	OVG_IF_ERROR(!paintModes || paintModes & ~(VG_FILL_PATH | VG_STROKE_PATH), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	if(paintModes & VG_FILL_PATH)
	{
		context->fillPaint = paintObjPtr;
	}

	if(paintModes & VG_STROKE_PATH)
	{
		context->strokePaint = paintObjPtr;
	}

	gcmFOOTER_NO();
}

void  vgPaintPattern(VGPaint paint, VGImage image)
{
    _VGImage  *inImage;
    _VGPaint  *inPaint;
	_VGContext* context;

	gcmHEADER_ARG("paint=%d image=%d", paint, image);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGPAINTPATTERN, 0);
    inImage = _VGIMAGE(context, image);
    inPaint = _VGPAINT(context, paint);

	OVG_IF_ERROR(!inPaint || (image != VG_INVALID_HANDLE && !inImage), VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(image != VG_INVALID_HANDLE && eglIsInUse(inImage), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);

	if (inPaint->pattern)
	{
		VGObject_Release(context->os, &inPaint->pattern->object);
	}

	inPaint->pattern = inImage;
	if (inImage != gcvNULL)
		VGObject_AddRef(context->os, &inImage->object);

	gcmFOOTER_NO();
}


void  vgSetColor(VGPaint paint, VGuint rgba)
{
    _VGPaint *paintObjPtr;
	_VGContext* context;
	gcmHEADER_ARG("paint=%d rgba=0x%x", paint, rgba);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGSETCOLOR, 0);
    paintObjPtr = _VGPAINT(context, paint);

    OVG_IF_ERROR(!paintObjPtr, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    R8G8B8A8_TO_COLOR(rgba, paintObjPtr->paintColor);

	COLOR_CLAMP(paintObjPtr->paintColor);

	COLOR_SETC(paintObjPtr->inputPaintColor, paintObjPtr->paintColor);

	gcmFOOTER_NO();
}

VGuint  vgGetColor(VGPaint paint)
{
    _VGPaint *paintObjPtr;
	VGuint		ret;
	_VGContext* context;

	gcmHEADER_ARG("paint=%d", paint);
    OVG_GET_CONTEXT(0);

	vgmPROFILE(context, VGGETCOLOR, 0);
    paintObjPtr = _VGPAINT(context, paint);
    OVG_IF_ERROR(!paintObjPtr, VG_BAD_HANDLE_ERROR, 0);

    ret = COLOR_TO_R8G8B8A8(paintObjPtr->inputPaintColor);

	gcmFOOTER_ARG("return=0x%x", ret);
	return ret;
}


VGPaint  vgGetPaint(VGPaintMode paintMode)
{
	_VGContext* context;
	gcmHEADER_ARG("paintMode=0x%x", paintMode);
	OVG_GET_CONTEXT(VG_INVALID_HANDLE);

	vgmPROFILE(context, VGGETPAINT, 0);

	OVG_IF_ERROR(paintMode != VG_FILL_PATH && paintMode != VG_STROKE_PATH, VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

	if(paintMode == VG_FILL_PATH)
	{
        if (context->fillPaint)
		{
			gcmFOOTER_ARG("return=%d", context->fillPaint->object.name);
            return (VGPaint)context->fillPaint->object.name;
		}

	}
    else
    {
        if (context->strokePaint)
		{
			gcmFOOTER_ARG("return=%d", context->strokePaint->object.name);
            return (VGPaint)context->strokePaint->object.name;
		}
    }

	gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
    return VG_INVALID_HANDLE;
}
