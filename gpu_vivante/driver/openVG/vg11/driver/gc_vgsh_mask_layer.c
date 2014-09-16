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

void _VGMaskLayerCtor(gcoOS os, _VGMaskLayer *maskLayer)
{
	gcmHEADER_ARG("os=0x%x maskLayer=0x%x",
				   os, maskLayer);

    _VGImageCtor(os, &maskLayer->image);

    gcmFOOTER_NO();
}

void _VGMaskLayerDtor(gcoOS os, _VGMaskLayer *maskLayer)
{
	gcmHEADER_ARG("os=0x%x maskLayer=0x%x",
				   os, maskLayer);

    _VGImageDtor(os, &maskLayer->image);

    gcmFOOTER_NO();
}


void  vgMask(VGHandle mask,
			 VGMaskOperation operation,
			 VGint x, VGint y,
			 VGint width, VGint height)
{
    _VGImage		*imask = gcvNULL;
	_VGObject	    *object = gcvNULL;
    gctINT32 dx = x, dy = y, sx = 0, sy = 0;
	_VGContext* context;

	gcmHEADER_ARG("mask=%d operation=%d x=%d y=%d width=%d height=%d",
				  mask, operation, x, y, width, height);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGMASK, 0);

	object = vgshFindObject(context, mask);


    if (object == gcvNULL)
    {
        imask = gcvNULL;
    }
	else if (object->type == VGObject_Image)
	{
		imask = (_VGImage*)object;
	}
	else if (object->type == VGObject_MaskLayer)
	{
        imask = &((_VGMaskLayer*)object)->image;

	}

	OVG_IF_ERROR(operation != VG_CLEAR_MASK && operation != VG_FILL_MASK && !imask, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(operation != VG_CLEAR_MASK && operation != VG_FILL_MASK && imask && eglIsInUse(imask), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(operation < VG_CLEAR_MASK || operation > VG_SUBTRACT_MASK, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    gcmVERIFY_OK(vgshCreateMaskBuffer(context));

    if (operation != VG_CLEAR_MASK && operation != VG_FILL_MASK)
    {
        if (!vgshGetIntersectArea(
            &dx, &dy, &sx, &sy, &width, &height,
            context->maskImage.width, context->maskImage.height,
            imask->width, imask->height))
        {
            gcmFOOTER_NO();
            return;
        }

    }
    else
    {
        if (!vgshGetIntersectArea(
                &dx, &dy, &sx, &sy, &width, &height,
                context->maskImage.width, context->maskImage.height,
                width, height))
        {
            gcmFOOTER_NO();
            return;
        }
    }


	gcmVERIFY_OK(vgshMask(context, imask, operation, dx, dy, sx, sy, width, height));

	gcmFOOTER_NO();
}

/*
	The vgRenderToMask function modifies the current surface mask by applying
	the given operation to the set of coverage values associated with the rendering
	of the given path.
*/
void vgRenderToMask(VGPath path,
                    VGbitfield paintModes,
                    VGMaskOperation operation)
{
	/*
	 1. Set the masklayer as target, render the path into it;
	 2. Perform the mask operation on the mask surface with the masklayer.
	*/
	_VGPath		*pathObj = gcvNULL;
	_VGContext* context;

	gcmHEADER_ARG("path=%d paintModes=0x%x operation=%d", path, paintModes, operation);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGRENDERTOMASK, 0);

	pathObj = _VGPATH(context, path);
	OVG_IF_ERROR(!pathObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(!paintModes || (paintModes & ~(VG_FILL_PATH | VG_STROKE_PATH)), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(operation < VG_CLEAR_MASK || operation > VG_SUBTRACT_MASK, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	do
	{
		gceSTATUS status;

        gcmERR_BREAK(vgshCreateMaskBuffer(context));

		if (operation == VG_CLEAR_MASK || operation == VG_FILL_MASK)
		{
			gcmERR_BREAK(vgshMask(
                context, gcvNULL, operation,
                0, 0, 0, 0, context->maskImage.width, context->maskImage.height));

		}
		else
		{
			_VGImage        maskLayer;
            _VGColorDesc    colorDesc;

			_VGImageCtor(context->os, &maskLayer);

            vgshGetFormatColorDesc(VG_sRGBA_8888, &colorDesc);

            gcmERR_BREAK(vgshIMAGE_Initialize(
                context,
                &maskLayer,
                &colorDesc,
                context->maskImage.width,
                context->maskImage.height,
                context->maskImage.orient));

            gcmERR_BREAK(vgshIMAGE_SetSamples(
                context, &maskLayer,
                context->targetImage.samples));

			CheckContextParam(
                context, pathObj,
                &context->pathUserToSurface, paintModes);

			gcmERR_BREAK(vgshUpdateScissor(context));

			/*gcmVERIFY_OK(gco3D_SetClearColor(context->engine, 0, 0, 0, 0));*/

			if (paintModes & VG_FILL_PATH)
			{
				/* clear the masklayer surface */
				/*gcmVERIFY_OK(gcoSURF_Clear(maskLayer.surface, gcvCLEAR_COLOR));*/
				_VGColor zeroColor;
				COLOR_SET(zeroColor, 0.0, 0.0, 0.0, 0.0, sRGBA);

                 gcmERR_BREAK(vgshClear(
                    context, &maskLayer, 0, 0,
                    maskLayer.width, maskLayer.height,
			        &zeroColor,  gcvFALSE, gcvTRUE));

				/* fill the path to the masklayer*/
				gcmERR_BREAK(vgshDrawMaskPath(
                    context, &maskLayer,
                    pathObj, VG_FILL_PATH));

                /* maskLayer to  maskbuffer */
                gcmERR_BREAK(vgshMask(
                    context,
                    &maskLayer,
                    operation,
                    0, 0, 0, 0,
                    context->targetImage.width,
                    context->targetImage.height));
			}

			if (paintModes & VG_STROKE_PATH)
			{
				/* clear the masklayer surface */
				/*gcmVERIFY_OK(gcoSURF_Clear(maskLayer.surface, gcvCLEAR_COLOR));*/
				_VGColor zeroColor;
				COLOR_SET(zeroColor, 0.0, 0.0, 0.0, 0.0, sRGBA);

                gcmERR_BREAK(vgshClear(
                    context,
                    &maskLayer,
                    0, 0,
                    maskLayer.width, maskLayer.height,
					&zeroColor,  gcvFALSE, gcvTRUE));

				/* stroke the path to the masklayer*/
				gcmERR_BREAK(vgshDrawMaskPath(
                    context, &maskLayer,
                    pathObj, VG_STROKE_PATH));

                /* maskLayer to  maskbuffer */
                gcmERR_BREAK(vgshMask(
                    context, &maskLayer, operation,
                    0, 0, 0, 0,
                    context->targetImage.width,
                    context->targetImage.height));
            }

			_VGImageDtor(context->os, &maskLayer);

		}
	}
	while(gcvFALSE);

	gcmFOOTER_NO();
}

VGMaskLayer vgCreateMaskLayer(VGint width, VGint height)
{

	_VGMaskLayer *maskLayerObj = gcvNULL;
    _VGContext* context;
    _VGColor    color;

	gcmHEADER_ARG("width=%d height=%d", width, height);
	OVG_GET_CONTEXT(VG_INVALID_HANDLE);

	vgmPROFILE(context, VGCREATEMASKLAYER, 0);

	OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);
	OVG_IF_ERROR(width > MAX_IMAGE_WIDTH || height > MAX_IMAGE_HEIGHT || width * height > MAX_IMAGE_PIXELS, VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

	NEWOBJ(_VGMaskLayer, context->os, maskLayerObj);

	if (!maskLayerObj)
	{
		SetError(context, VG_OUT_OF_MEMORY_ERROR);
		gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
		return VG_INVALID_HANDLE;
	}

	if (!vgshInsertObject(context, &maskLayerObj->object, VGObject_MaskLayer))
	{
        DELETEOBJ(_VGMaskLayer, context->os, maskLayerObj);
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
		gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
	}

    gcmVERIFY_OK(vgshCreateMaskBuffer(context));

    gcmVERIFY_OK(vgshIMAGE_Initialize(
        context, &maskLayerObj->image,
        &context->maskImage.internalColorDesc,
        width, height,
        context->maskImage.orient));

	color.a = color.g = color.b = color.r = 1.0;
	color.format = sRGBA;

    gcmVERIFY_OK(vgshClear(
        context,
        &maskLayerObj->image,
        0, 0, width, height,
        &color, gcvFALSE, gcvTRUE));

	gcmFOOTER_ARG("return=%d", maskLayerObj->object.name);
	return maskLayerObj->object.name;

}

void vgDestroyMaskLayer(VGMaskLayer maskLayer)
{
	_VGMaskLayer *maskLayerObj = gcvNULL;
	_VGContext* context;
	gcmHEADER_ARG("maskLayer=%d", maskLayer);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGDESTROYMASKLAYER, 0);

	maskLayerObj = _VGMASKLAYER(context, maskLayer);
	OVG_IF_ERROR(!maskLayerObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

	vgshRemoveObject(context, &maskLayerObj->object);
	DELETEOBJ(_VGMaskLayer, context->os, maskLayerObj);

	gcmFOOTER_NO();
}

/*
	The vgFillMaskLayer function sets the values of a given maskLayer within a
	given rectangular region to a given value.
*/
void vgFillMaskLayer(VGMaskLayer maskLayer,
                     VGint x, VGint y,
                     VGint width, VGint height,
                     VGfloat value)
{
	_VGMaskLayer    *layerObj = gcvNULL;
	_VGColor	    color;
	_VGContext      *context;

    gctINT32 dx = x, dy = y, sx = 0, sy = 0;

    gcmHEADER_ARG("maskLayer=%d x=%d y=%d width=%d height=%d value=%f",
				  maskLayer, x, y, width, height, value);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGFILLMASKLAYER, 0);

	layerObj = _VGMASKLAYER(context, maskLayer);
	OVG_IF_ERROR(!layerObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(value < 0.0f || value > 1.0f, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(width <= 0 || height <= 0 || x < 0 || y < 0 || x > layerObj->image.width - width || y > layerObj->image.height - height, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	color.a = color.g = color.b = color.r = value;
	color.format = sRGBA;

    if (!vgshGetIntersectArea(
        &dx, &dy, &sx, &sy, &width, &height,
        layerObj->image.width, layerObj->image.height,
        width, height))
    {
        gcmFOOTER_NO();
        return;
    }

    gcmVERIFY_OK(vgshClear(
        context,
        &layerObj->image,
        dx, dy, width, height,
        &color, gcvFALSE, gcvTRUE));

	gcmFOOTER_NO();
}

/*
	vgCopyMask copies a portion of the current surface mask into a VGMaskLayer
	object.
*/
void vgCopyMask(VGMaskLayer maskLayer,
                VGint dx, VGint dy,
                VGint sx, VGint sy,
                VGint width, VGint height)
{
	_VGMaskLayer *layerObj = gcvNULL;
	_VGContext* context;

	gcmHEADER_ARG("maskLayer=%d dx=%d dy=%d sx=%d sy=%d width=%d height=%d",
				  maskLayer, dx, dy, sx, sy, width, height);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);
	vgmPROFILE(context, VGCOPYMASK, 0);

	layerObj = _VGMASKLAYER(context, maskLayer);
	OVG_IF_ERROR(!layerObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);


	OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	gcmVERIFY_OK(vgshCreateMaskBuffer(context));

    gcmVERIFY_OK(vgshIMAGE_Blit(context,
                        &layerObj->image,
                        &context->maskImage,
                        dx, dy, sx, sy, width, height,
                        vgvBLIT_COLORMASK_ALL));

	gcmFOOTER_NO();
}
