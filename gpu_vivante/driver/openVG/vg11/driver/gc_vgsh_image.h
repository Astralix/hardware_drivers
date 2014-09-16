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


#ifndef __gc_vgsh_image_h_
#define __gc_vgsh_image_h_

#define RIGHT_LEFT    (1 << 0)
#define LEFT_RIGHT    (1 << 1)
#define BOTTOM_TOP    (1 << 2)
#define TOP_BOTTOM    (1 << 3)


#define vgvBLIT_COLORMASK_ALL   0xf
#define vgvBLIT_FROMDATA        (1 << 4)
#define vgvBLIT_TODATA          (1 << 5)
#define vgvBLIT_SCISSORING      (1 << 6)

struct _VGImage
{
	_VGObject           object;
	gctINT32		    width;
	gctINT32		    height;
	gctINT32            stride;
	void                *data;
	VGbitfield	        allowedQuality;
    gctBOOL 			hasChildren;
	_VGImage*			parent;
	gctINT32            parentOffsetX;
	gctINT32            parentOffsetY;

    gctINT32            rootWidth;
    gctINT32            rootHeight;
    gctINT32            rootOffsetX;
    gctINT32            rootOffsetY;

    gctBOOL             dirty;
    gctBOOL             *dirtyPtr;
    gctBOOL             wrapped;

    gctUINT32           samples;

    gcoTEXTURE          texture;
    gcoSURF             texSurface;
    gcoSTREAM			stream;
    gcoSURF             surface;
    gcsTEXTURE          texStates;

	gceORIENTATION		orient;

	gctBOOL             eglUsed;
    _VGColorDesc        internalColorDesc;
};


void UpdateTexture(_VGContext *context, _VGImage* image);

void BitBlit(_VGContext *context, gcoSURF srcSurface, gcsRECT *srcRect, gcoSURF dstSurface, gcsRECT *dstRect);
gctFLOAT gammaTrans(gctFLOAT c);
gctFLOAT invgammaTrans(gctFLOAT c);
gctBOOL eglIsInUse(_VGImage* image);

gctBOOL isImageAligned(const void* ptr, VGImageFormat format);
gctBOOL isValidImageFormat(gctINT32 format);
void _VGImageCtor(gcoOS os, _VGImage *image);
void _VGImageDtor(gcoOS os, _VGImage *image);
gctBOOL vgshGetIntersectArea(VGint *pdx, VGint *pdy,  VGint *psx, VGint *psy, VGint *pw, VGint *ph, VGint dstW, VGint dstH, VGint srcW, VGint srcH);
void UnPackColor(_VGuint32 inputData,  _VGColorDesc * inputDesc, _VGColor *outputColor);
void GetAncestorOffset(_VGImage *image, gctINT32 *px, gctINT32 *py);
_VGImage* GetAncestorImage(_VGImage* image);
gcoTEXTURE GetAncestorTexture(_VGImage* image);
void GetAncestorSize(_VGImage *image, gctINT32 *width, gctINT32 *height);
void intersect(_VGRectangle *rect1, _VGRectangle *rect2, _VGRectangle *out);
gctINT32 next_p2(gctINT32 a);
void ConvertColor(_VGColor *srcColor, _VGColorFormat dstFormat);
void SyncTextureBitmap(_VGContext *context, _VGImage* image);
gctINT32 getColorConvertValue(_VGColorFormat srcColorFormat, _VGColorFormat dstColorFormat);
void GetTexBound(_VGImage *image, VGfloat *texBound);

void DoReadData(_VGContext *context,
			   gcoSURF    srcSuface,
			   _VGColorDesc *srcColorDesc,
			   void* data,
			   VGint dataStride,
			   VGImageFormat dataFormat,
			   VGint dx,
			   VGint dy,
			   VGint sx,
			   VGint sy,
			   VGint width,
			   VGint height,
			   gctBOOL fromDraw);
void DoReadDataPixels(_VGContext *context,
			   gcoSURF    srcSuface,
			   _VGColorDesc *srcColorDesc,
			   void* data,
			   VGint dataStride,
			   VGImageFormat dataFormat,
			   VGint dx,
			   VGint dy,
			   VGint sx,
			   VGint sy,
			   VGint width,
			   VGint height,
			   gctBOOL fromDraw);

void DoWriteData(_VGContext *context,
			  gcoSURF      dstSurface,
			  _VGColorDesc *dstColorDesc,
			  const void   * data,
			  VGint			dataStride,
			  VGImageFormat dataFormat,
			  VGint dx, VGint dy, VGint sx, VGint sy, VGint width, VGint height);

gceSTATUS vgshIMAGE_Blit(
                    _VGContext      *context,
                    _VGImage        *dstImage,
                    _VGImage        *srcImage,
                    gctINT32        dx,
                    gctINT32        dy,
                    gctINT32        sx,
                    gctINT32        sy,
                    gctINT32        width,
                    gctINT32        height,
                    gctUINT32        flag);

void drawImage(_VGContext	*context,
			   _VGImage		*image,
			   _VGMatrix3x3	*matrix,
               gctINT32     samples);

gctINT32 getColorConvertValue(_VGColorFormat srcColorFormat, _VGColorFormat dstColorFormat);
gctINT32 getColorConvertAlphaValue(_VGColorFormat srcColorFormat, _VGColorFormat dstColorFormat);
gceSTATUS vgshIMAGE_WrapFromSurface(_VGContext *context, _VGImage *image, gcoSURF surface);

gceSTATUS vgshUploadImageData(_VGContext *context, _VGImage * image, const void * data,
                              VGint dataStride, VGImageFormat dataFormat, VGint dx, VGint dy, VGint width, VGint height);

gceSTATUS vgshIMAGE_Initialize(
    _VGContext      *context,
    _VGImage        *image,
    _VGColorDesc    *desc,
    gctINT32        width,
    gctINT32        height,
    gceORIENTATION  orientation);

gceSTATUS vgshIMAGE_SetSamples(
    _VGContext      *context,
    _VGImage        *image,
    gctUINT32       samples);

void vgshGetFormatColorDesc(
    VGImageFormat format,
    _VGColorDesc *colorDesc);

gctBOOL  vgshGetBlitRect(
    _VGImage    *dstImage,
    _VGImage    *srcImage,
    gctINT32    *dx,
    gctINT32    *dy,
    gctINT32    *sx,
    gctINT32    *sy,
    gctINT32    *width,
    gctINT32    *height);

void DumpSurface(_VGContext *context, gcoSURF srcSurface, const char * idStr);
#endif /* __gc_vgsh_image_h_ */
