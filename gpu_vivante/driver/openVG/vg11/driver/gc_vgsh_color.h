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


#ifndef __gc_vgsh_color_h_
#define __gc_vgsh_color_h_

typedef enum _VGColorFormat
{
    lRGBA,
	sRGBA,
	lRGBA_PRE,
	sRGBA_PRE,
	lLA,
	sLA,
	lLA_PRE,
	sLA_PRE,
}_VGColorFormat;

typedef struct _VGColorDesc
{
	gctINT32		rbits;
	gctINT32		rshift;
	gctINT32		gbits;
	gctINT32		gshift;
	gctINT32		bbits;
	gctINT32		bshift;
	gctINT32		abits;
	gctINT32		ashift;
	gctINT32		lbits;
	gctINT32		lshift;
    gctINT32        bpp;
    _VGColorFormat  colorFormat;
    gceSURF_FORMAT  surFormat;
    VGImageFormat   format;
}_VGColorDesc;

typedef struct _VGColor
{
	gctFLOAT r;
	gctFLOAT g;
	gctFLOAT b;
	gctFLOAT a;

	_VGColorFormat format;

}_VGColor;

#define NONLINEAR		 (1<<0)
#define PREMULTIPLIED	 (1<<1)
#define LUMINANCE		 (1<<2)
#define SHIFT             4


#define BITWISE			(1<<16)
#define ALPHA_8			(1<<17)
#define ALPHA_4			(1<<18)
#define ALPHA_1			(1<<19)
#define ARGB_5650			(1<<20)
#define ARGB_5551			(1<<21)
#define ARGB_4444			(1<<22)
#define ALPHAONLY		(ALPHA_1 | ALPHA_8 | ALPHA_4)
#define ARGBONLY		(ARGB_5650 | ARGB_5551 | ARGB_4444)

#define COLOR_BITS_ALL \
	(BITWISE | ALPHA_1 | ALPHA_8 | ALPHA_4 | ARGB_5650 | ARGB_5551 | ARGB_4444)

#define COLOR_SET(c, rr,gg,bb,aa, ff) \
    { c.r = rr; c.g = gg; c.b = bb; c.a = aa; c.format = ff; }

#define COLOR_SETC(c1, c2) \
    { c1.r = c2.r; c1.g = c2.g; c1.b = c2.b; c1.a = c2.a; c1.format = c2.format; }

#define COLOR_CLAMP(c) \
    do {\
        gctFLOAT u;\
        c.a = CLAMP(c.a, 0.0f, 1.0f);\
          u = (c.format & PREMULTIPLIED) ? c.a : (gctFLOAT)1.0f;\
        c.r = CLAMP(c.r, 0.0f, u);\
        c.g = CLAMP(c.g, 0.0f, u);\
        c.b = CLAMP(c.b, 0.0f, u);\
    }while(0)

#define COLOR_MULTIPLY(c, s) \
    c.r *= s; c.g *= s; c.b *= s; c.a *= s;

#define COLOR_ADD(c1, c2) \
    c1.r = c1.r + c2.r; c1.g = c1.g + c2.g; c1.b = c1.b + c2.b; c1.a = c1.a + c2.a;


#define COLOR_TO_A8R8G8B8(c) \
    ((VGuint) ((((VGint)(c.a * 255.0f)) << 24) | (((VGint)(c.r * 255.0f)) << 16) | (((VGint)(c.g * 255.0f)) << 8) | ((VGint)(c.b * 255.0f))))

#define COLOR_TO_R8G8B8A8(c) \
    ((VGuint) ((((VGint)(c.a * 255.0f))) | (((VGint)(c.r * 255.0f)) << 24) | (((VGint)(c.g * 255.0f)) << 16) | (((VGint)(c.b * 255.0f)) << 8)))


#define COLOR_FLOAT_TO_UINT(c)  \
    (_VGuint32)(c * 255.0f)


#define R8G8B8A8_TO_COLOR(rgba, c) \
    do \
    {\
        c.r = (gctFLOAT)(((rgba & 0xff000000) >> 24) / 255.0f); \
        c.g = (gctFLOAT)(((rgba & 0x00ff0000) >> 16) / 255.0f); \
        c.b = (gctFLOAT)(((rgba & 0x0000ff00) >> 8)  / 255.0f); \
        c.a = (gctFLOAT)((rgba & 0xff) / 255.0f); \
		c.format = sRGBA;\
    }while(0)

#define COLOR_UNPREMULTIPLY(c) \
    do \
    {\
		if (c.format & PREMULTIPLIED)\
		{\
			gctFLOAT aa = (c.a != 0) ? 1.0f / c.a : (gctFLOAT)0.0f;\
			c.r *= aa; c.g *= aa; c.b *= aa;\
			c.format &= ~PREMULTIPLIED;\
		}\
    }while(0)

#define COLOR_PREMULTIPLY(c) \
    do \
    {\
		if (!(c.format & PREMULTIPLIED)) \
		{\
			c.r *= c.a; c.g *= c.a; c.b *= c.a;\
			c.format |= PREMULTIPLIED;\
		}\
    }while(0)


#define R8G8B8A8_TO_A8R8G8B8(color) \
    ((color >> 8 & 0xffffff)  | ((color << 24) & 0xff000000))


#define A8R8G8B8_TO_R8G8B8A8(color) \
    color = ((color << 8 & 0xffffff00) | ((color >> 24) & 0x000000ff))

#define ARGB(a, r, g, b)   ((VGuint) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

#endif /* __gc_vgsh_color_h_ */
