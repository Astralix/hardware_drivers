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




/*
 * Graphics API specific functions.
 */

#ifndef __gc_egl_graphics_h_
#define __gc_egl_graphics_h_

#include "gc_egl_os.h"
#include "gc_vdk_types.h"

#ifdef __cplusplus
extern "C" {
#endif

NativeDisplayType
veglGetDefaultDisplay(
	void
	);

void
veglReleaseDefaultDisplay(
	IN NativeDisplayType Display
	);

gctBOOL
veglIsValidDisplay(
    IN NativeDisplayType Display
    );

gctBOOL
veglInitLocalDisplayInfo(
    VEGLDisplay Display
    );

gctBOOL
veglDeinitLocalDisplayInfo(
    VEGLDisplay Display
    );

gctBOOL
veglGetDisplayInfo(
    IN VEGLDisplay Display,
	IN NativeWindowType Window,
	OUT VEGLDisplayInfo Info
	);

gctBOOL
veglGetDisplayBackBuffer(
	IN VEGLDisplay Display,
	IN NativeWindowType Window,
    OUT VEGLBackBuffer BackBuffer,
    OUT gctBOOL_PTR Flip
	);

gctBOOL
veglSetDisplayFlip(
	IN NativeDisplayType Display,
	IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer
	);

gctBOOL
veglSetDisplayFlipRegions(
    IN NativeDisplayType Display,
    IN NativeWindowType Window,
    IN VEGLBackBuffer BackBuffer,
    IN gctINT NumRects,
    IN gctINT_PTR Rects
    );

gctBOOL
veglGetWindowInfo(
	IN VEGLDisplay Display,
	IN NativeWindowType Window,
	OUT gctINT_PTR X,
	OUT gctINT_PTR Y,
	OUT gctUINT_PTR Width,
	OUT gctUINT_PTR Height,
	OUT gctUINT_PTR BitsPerPixel,
	OUT gceSURF_FORMAT * Format
	);

gctBOOL
veglGetWindowBits(
    IN NativeDisplayType Display,
#if defined(ANDROID) && !USE_VDK
	IN gctPOINTER Buffer,
#else
    IN NativeWindowType Window,
#endif
    OUT gctPOINTER * Logical,
    OUT gctUINT32_PTR Physical,
    OUT gctINT32_PTR Stride
    );

gctBOOL
veglDrawImage(
	IN VEGLDisplay Display,
	IN VEGLSurface Surface,
	IN gctPOINTER Bits,
	IN gctINT Left,
	IN gctINT Top,
	IN gctINT Width,
	IN gctINT Height
	);

gctBOOL
veglDrawPixmap(
	IN VEGLDisplay Display,
	IN EGLNativePixmapType Pixmap,
	IN gctINT Left,
	IN gctINT Top,
	IN gctINT Right,
	IN gctINT Bottom,
	IN gctINT Width,
	IN gctINT Height,
	IN gctINT BitsPerPixel,
	IN gctPOINTER Bits
	);

EGLint
veglGetNativeVisualId(
	IN VEGLDisplay Display,
    IN VEGLConfig Config
	);

gctBOOL
veglGetPixmapInfo(
	IN NativeDisplayType Display,
	IN NativePixmapType Pixmap,
	OUT gctUINT_PTR Width,
	OUT gctUINT_PTR Height,
	OUT gctUINT_PTR BitsPerPixel,
	OUT gceSURF_FORMAT * Format
	);

gctBOOL
veglGetPixmapBits(
	IN NativeDisplayType Display,
	IN NativePixmapType Pixmap,
	OUT gctPOINTER * Bits,
	OUT gctINT_PTR Stride,
	OUT gctUINT32_PTR Physical
	);

gctBOOL
veglCopyPixmapBits(
    IN VEGLDisplay Display,
    IN NativePixmapType Pixmap,
    IN gctUINT DstWidth,
    IN gctUINT DstHeight,
    IN gctINT DstStride,
    IN gceSURF_FORMAT DstFormat,
    OUT gctPOINTER DstBits
    );

gctBOOL
veglIsColorFormatSupport(
    IN NativeDisplayType Display,
    IN VEGLConfig        Config
    );

#ifdef __cplusplus
}
#endif

#endif /* __gc_egl_graphics_h_ */
