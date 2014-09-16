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

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <KHR/khrvivante.h>
#include <VG/vgext.h>

gctFLOAT gammaTrans(gctFLOAT c)
{
    gcmHEADER_ARG("c=%f", c);

    if( c <= 0.00304f )
        c *= 12.92f;
    else
        c = 1.0556f * gcoMATH_Power(c, 1.0f/2.4f) - 0.0556f;

    gcmFOOTER_ARG("return=%f", c);

    return c;
}

gctFLOAT invgammaTrans(gctFLOAT c)
{
    gcmHEADER_ARG("c=%f", c);

    if( c <= 0.03928f )
        c /= 12.92f;
    else
        c = gcoMATH_Power((c + 0.0556f)/1.0556f, 2.4f);

    gcmFOOTER_ARG("return=%f", c);

    return c;
}

static gctFLOAT lRGBtoL(gctFLOAT r, gctFLOAT g, gctFLOAT b)
{
    gcmHEADER_ARG("r=%f g=%f b=%f", r, g, b);
    gcmFOOTER_ARG("return=%f", 0.2126f*r + 0.7152f*g + 0.0722f*b);

    return 0.2126f*r + 0.7152f*g + 0.0722f*b;
}


gctINT32 next_p2(gctINT32 a)
{
    gctINT32 rval = 1;

    gcmHEADER_ARG("a=%d", a);

    while(rval < a) rval <<= 1;

    gcmFOOTER_ARG("return=%d", rval);

    return rval;
}

gctBOOL eglIsInUse(_VGImage* image)
{
    return gcvFALSE;
}

gctBOOL IsOverlap(_VGImage* im1, _VGImage *im2)
{
    const _VGImage* im1ancestor = im1;
    const _VGImage* im2ancestor = im2;
    gctINT32 im1x = 0;
    gctINT32 im1y = 0;
    gctINT32 im2x = 0;
    gctINT32 im2y = 0;
    _VGRectangle rect1;
    _VGRectangle rect2;
    _VGRectangle out;

    gcmHEADER_ARG("im1=0x%x im2=0x%x", im1, im2);

    im1ancestor = GetAncestorImage(im1);
    im2ancestor = GetAncestorImage(im2);

    if(im1ancestor != im2ancestor)
    {
        gcmFOOTER_ARG("return=%s", "FALSE");

        return gcvFALSE;   /*no common ancestor, images don't overlap*/
    }

    GetAncestorOffset(im1, &im1x, &im1y);
    GetAncestorOffset(im2, &im2x, &im2y);

    rect1.x = im1x;
    rect1.y = im1y;
    rect1.width = im1->width;
    rect1.height = im1->height;
    rect2.x = im2x;
    rect2.y = im2y;
    rect2.width = im2->width;
    rect2.height = im2->height;

    intersect(&rect1, &rect2, &out);

    if (!out.width || !out.height)
    {
        gcmFOOTER_ARG("return=%s", "FALSE");

        return gcvFALSE;
    }

    gcmFOOTER_ARG("return=%s", "TRUE");

    return gcvTRUE;
}

void GetAncestorOffset(_VGImage *image, gctINT32 *px, gctINT32 *py)
{
    gctINT32 x = 0, y = 0;

    gcmHEADER_ARG("image=0x%x px=0x%x py=0x%x", image, px, py);

    while(image)
    {
        x += image->parentOffsetX;
        y += image->parentOffsetY;
        image = image->parent;
    }

    if (px) *px = x;
    if (py) *py = y;

    gcmFOOTER_NO();
}

_VGImage* GetAncestorImage(_VGImage* image)
{
    gcmHEADER_ARG("image=0x%x", image);

    while(image->parent)
    {
        image = image->parent;
    }

    gcmFOOTER_ARG("return=0x%x", image);

    return image;
}

gcoTEXTURE GetAncestorTexture(_VGImage* image)
{
    gcmHEADER_ARG("image=0x%x", image);

    gcmASSERT(image);
	if (image == gcvNULL)
	{
		gcmFOOTER_ARG("return=0x%x", gcvNULL);
		return gcvNULL;
	}

    while(image->parent)
    {
        image = image->parent;
    }

    gcmFOOTER_ARG("return=0x%x", image->texture);

    return image->texture;
}

gcoSURF GetAncestorSurface(_VGImage* image)
{
    gcmHEADER_ARG("image=0x%x", image);

   gcmASSERT(image);
   if (image == gcvNULL)
   {
	   gcmFOOTER_ARG("return=0x%x", gcvNULL);
	   return gcvNULL;
   }

   while(image->parent)
    {
        image = image->parent;
    }

    gcmFOOTER_ARG("return=0x%x", image->surface);

   return image->surface;
}


void GetAncestorSize(_VGImage *image, gctINT32 *width, gctINT32 *height)
{
    gctINT32 w = 0, h = 0;

    gcmHEADER_ARG("image=0x%x width=0x%x height=0x%x", image, width, height);

    while(image)
    {
        w = image->width;
        h = image->height;
        image = image->parent;
    }

    if (width)  *width = w;
    if (height) *height = h;

    gcmFOOTER_NO();
}


gctINT32 ImageFormat2Bpp(VGImageFormat format)
{
    static const gctINT32 bpps[15] = {32, 32, 32, 16, 16, 16, 8, 32, 32, 32, 8, 8, 1, 1, 4};
    gctINT32  imageFormat = format & 0xf;

    gcmHEADER_ARG("format=%d", format);

    gcmASSERT(imageFormat >= (gctINT32)VG_sRGBX_8888 && imageFormat <= (gctINT32)VG_A_4);
    gcmASSERT(isValidImageFormat(format));

    gcmFOOTER_ARG("return=%d", bpps[imageFormat]);

    return bpps[imageFormat];
}

_VGColorFormat ImageFormat2ColorFormat(VGImageFormat format)
{

    static const _VGColorFormat formats[15] = {sRGBA, sRGBA, sRGBA_PRE, sRGBA, sRGBA, sRGBA, sLA, lRGBA, lRGBA, lRGBA_PRE, lLA, lRGBA, lLA, lRGBA, lRGBA};
    gctINT32  imageFormat = format & 0xf;

    gcmHEADER_ARG("format=%d", format);

    gcmASSERT(imageFormat >= (gctINT32)VG_sRGBX_8888 && imageFormat <= (gctINT32)VG_A_4);

    gcmFOOTER_ARG("return=%d", formats[imageFormat]);

    return formats[imageFormat];
}


gctBOOL isImageAligned(const void* ptr, VGImageFormat format)
{
    gctINT32 alignment = 0;
    gctBOOL ret;

    gcmHEADER_ARG("ptr=0x%x format=%d", ptr, format);

    alignment = ImageFormat2Bpp(format) >> 3;
    if(alignment <= 1)
    {
        gcmFOOTER_ARG("return=%s", "TRUE");

        return gcvTRUE;
    }

    ret = isAligned(ptr, alignment);

    gcmFOOTER_ARG("return=%s", ret ? "TRUE" : "FALSE");

    return ret;
}

gctBOOL isValidImageFormat(gctINT32 f)
{
    gcmHEADER_ARG("f=%d", f);

    if (f < VG_sRGBX_8888 || f > VG_lABGR_8888_PRE)
    {
        gcmFOOTER_ARG("return=%s", "FALSE");

        return gcvFALSE;
    }
    else
    {
        gcmFOOTER_ARG("return=%s", "TRUE");

        return gcvTRUE;
    }
}

static _VGuint32 colorToInt(gctFLOAT c, gctINT32 maxc)
{
    _VGuint32 result;
	_VGint32 _floor;
    gcmHEADER_ARG("c=%f maxc=%d", c, maxc);
	_floor = (_VGint32)gcoMATH_Floor(c * (_VGfloat)maxc + 0.5f);
	result = gcmMIN(gcmMAX(_floor, 0), maxc);
    gcmFOOTER_ARG("return=%u", result);

    return result;
}

static  gctFLOAT intToColor(_VGuint32 i, _VGuint32 maxi)
{
    gcmHEADER_ARG("i=%u maxi=%u", i, maxi);
    gcmFOOTER_ARG("return=%f", (gctFLOAT)(i & maxi) / (gctFLOAT)maxi);

    return (gctFLOAT)(i & maxi) / (gctFLOAT)maxi;
}

static _VGColorFormat getProcessingFormat(_VGColorFormat srcFormat, gctBOOL filterFormatLinear,
                                          gctBOOL filterFormatPremultiplied)
{
    /*process in RGB, not luminance*/
    _VGColorFormat procFormat = (_VGColorFormat)(srcFormat & ~LUMINANCE);

    gcmHEADER_ARG("srcFormat=%d filterFormatLinear=%s filterFormatPremultiplied=%s",
        srcFormat,
        filterFormatLinear ? "TRUE" : "FALSE",
        filterFormatPremultiplied ? "TRUE" : "FALSE");

    if(filterFormatLinear)
        procFormat = (_VGColorFormat)(procFormat & ~NONLINEAR);
    else
        procFormat = (_VGColorFormat)(procFormat | NONLINEAR);

    if(filterFormatPremultiplied)
        procFormat = (_VGColorFormat)(procFormat | PREMULTIPLIED);
    else
        procFormat = (_VGColorFormat)(procFormat & ~PREMULTIPLIED);

    gcmFOOTER_ARG("return=%d", procFormat);

    return procFormat;
}

gctINT32 getColorConvertValue(_VGColorFormat srcColorFormat, _VGColorFormat dstColorFormat)
{
    gcmHEADER_ARG("srcColorFormat=%d dstColorFormat=%d", srcColorFormat, dstColorFormat);
    gcmFOOTER_ARG("return=%d",
        (srcColorFormat & (NONLINEAR | LUMINANCE)) | ((dstColorFormat & (NONLINEAR | LUMINANCE)) << SHIFT));

    return (srcColorFormat & (NONLINEAR | LUMINANCE)) | ((dstColorFormat & (NONLINEAR | LUMINANCE)) << SHIFT);
}

gctINT32 getColorConvertAlphaValue(_VGColorFormat srcColorFormat, _VGColorFormat dstColorFormat)
{
    gcmHEADER_ARG("srcColorFormat=%d dstColorFormat=%d", srcColorFormat, dstColorFormat);
    gcmFOOTER_ARG("return=%d", ((srcColorFormat & PREMULTIPLIED) >> 1) | (dstColorFormat & PREMULTIPLIED));

    return (((srcColorFormat & PREMULTIPLIED) >> 1) | (dstColorFormat & PREMULTIPLIED));
}

gctFLOAT Channel8toX(gctFLOAT a, gctINT32 max)
{
    VGint data;

    gcmHEADER_ARG("a=%f max=%d", a, max);

    data = (VGint)(a * (gctFLOAT)max + 0.5f);
    a = ((VGfloat)data)/(gctFLOAT)max;

    gcmFOOTER_ARG("return=%f", a);

    return a;
}



void ConvertColor(_VGColor *srcColor, _VGColorFormat dstFormat)
{
    gctFLOAT     r = srcColor->r, g = srcColor->g, b = srcColor->b, a = srcColor->a;
    _VGColorFormat format = srcColor->format;

    _VGuint32 conversion = (format & (NONLINEAR | LUMINANCE)) | ((dstFormat & (NONLINEAR | LUMINANCE)) << SHIFT);

    gcmHEADER_ARG("srcColor=0x%x dstFormat=%d", srcColor, dstFormat);

    gcmASSERT(r >= 0.0f && r <= 1.0f);
    gcmASSERT(g >= 0.0f && g <= 1.0f);
    gcmASSERT(b >= 0.0f && b <= 1.0f);
    gcmASSERT(a >= 0.0f && a <= 1.0f);

    gcmASSERT((format & LUMINANCE && r == g && r == b) || !(format & LUMINANCE));

    if(format == dstFormat )
    {
        gcmFOOTER_NO();

        return;
    }

    if(format & PREMULTIPLIED)
    {
        gctFLOAT ooa;
        gcmASSERT(r <= a);
        gcmASSERT(g <= a);
        gcmASSERT(b <= a);
        ooa = (a != 0.0f) ? 1.0f / a : (gctFLOAT)0.0f;
        r *= ooa;
        g *= ooa;
        b *= ooa;
    }

    /*From Section 3.4.2 of OpenVG 1.0.1 spec
    1: sRGB = gamma(lRGB)
    2: lRGB = invgamma(sRGB)
    3: lL = 0.2126 lR + 0.7152 lG + 0.0722 lB
    4: lRGB = lL
    5: sL = gamma(lL)
    6: lL = invgamma(sL)
    7: sRGB = sL

    Source/Dest lRGB sRGB   lL   sL
    lRGB          ?   1    3    3,5
    sRGB          2    ?   2,3  2,3,5
    lL            4    4,1  ?   5
    sL            7,2  7    6    ?
    */

    switch(conversion)
    {
    case lRGBA | (sRGBA << SHIFT): r = gammaTrans(r); g = gammaTrans(g); b = gammaTrans(b); break;                          /* 1    */
    case lRGBA | (lLA << SHIFT)  : a = 1.0f; r = g = b = lRGBtoL(r, g, b); break;                                           /* 3    */
    case lRGBA | (sLA << SHIFT)  : a = 1.0f; r = g = b = gammaTrans(lRGBtoL(r, g, b)); break;                               /* 3,5  */
    case sRGBA | (lRGBA << SHIFT): r = invgammaTrans(r); g = invgammaTrans(g); b = invgammaTrans(b); break;                 /* 2    */
    case sRGBA | (lLA << SHIFT)  : a = 1.0f; r = g = b = lRGBtoL(invgammaTrans(r), invgammaTrans(g), invgammaTrans(b)); break;      /*  2,3 */
    case sRGBA | (sLA << SHIFT)  : a = 1.0f; r = g = b = gammaTrans(lRGBtoL(invgammaTrans(r), invgammaTrans(g), invgammaTrans(b))); break; /*2,3,5*/
    case lLA   | (lRGBA << SHIFT): break;                                                                   /* 4 */
    case lLA   | (sRGBA << SHIFT): r = g = b = gammaTrans(r); break;                                                /*4,1*/
    case lLA   | (sLA << SHIFT)  : a = 1.0f; r = g = b = gammaTrans(r); break;                                              /*5*/
    case sLA   | (lRGBA << SHIFT): r = g = b = invgammaTrans(r); break;                                         /*7,2*/
    case sLA   | (sRGBA << SHIFT): break;                                                                   /*7*/
    case sLA   | (lLA << SHIFT)  : a = 1.0f; r = g = b = invgammaTrans(r); break;                                           /* 6 */
    default: gcmASSERT((format & (LUMINANCE | NONLINEAR)) == (dstFormat & (LUMINANCE | NONLINEAR))); break; /*nop*/
    }

    /*special processing*/
    if (dstFormat & BITWISE)
    {
        gcmASSERT((dstFormat & ~BITWISE) == lLA);
        r = ((r >= 0.5f) ? 1.0f : 0.0f);
        g = b = r;
    }
    else if (dstFormat & ALPHA_8)
    {
        gcmASSERT((dstFormat & ~ALPHA_8) == lRGBA);
        r = g = b = 1.0f;
    }
    else if (dstFormat & ALPHA_4)
    {
        gcmASSERT((dstFormat & ~ALPHA_4) == lRGBA);
        r = g = b = 1.0f;
        a = Channel8toX(a, 15);
    }
    else if (dstFormat & ALPHA_1)
    {
        gcmASSERT((dstFormat & ~ALPHA_1) == lRGBA);
        r = g = b = 1.0f;
        a = ((a >= 0.5f) ? 1.0f : 0.0f);
    }
    else if (dstFormat & ARGB_5650)
    {
        r = Channel8toX(r, 31);
        g = Channel8toX(g, 63);
        b = Channel8toX(b, 31);
    }
    else if (dstFormat & ARGB_5551)
    {
        a = ((a >= 0.5f) ? 1.0f : 0.0f);
        r = Channel8toX(r, 31);
        g = Channel8toX(g, 31);
        b = Channel8toX(b, 31);
    }
    else if (dstFormat & ARGB_4444)
    {
        a = Channel8toX(a, 15);
        r = Channel8toX(r, 15);
        g = Channel8toX(g, 15);
        b = Channel8toX(b, 15);
    }

    if(dstFormat & PREMULTIPLIED)
    {   /*premultiply*/
        r *= a;
        g *= a;
        b *= a;
        gcmASSERT(r <= a);
        gcmASSERT(g <= a);
        gcmASSERT(b <= a);
    }


    srcColor->a      = a;
    srcColor->r      = r;
    srcColor->g      = g;
    srcColor->b      = b;

    srcColor->format = dstFormat;

    gcmFOOTER_NO();
}



static const gctINT32 bitsDesc[15][5] =
{
 /*( R  G  B  A  L )  */
    {8, 8, 8, -1, 0},   /* VG_sRGBX_8888       =  0,*/
    {8, 8, 8,  8, 0},   /* VG_sRGBA_8888       =  1,*/
    {8, 8, 8,  8, 0},   /* VG_sRGBA_8888_PRE   =  2,*/
    {5, 6, 5,  0, 0},   /* VG_sRGB_565         =  3,*/
    {5, 5, 5,  1, 0},   /* VG_sRGBA_5551       =  4,*/
    {4, 4, 4,  4, 0},   /* VG_sRGBA_4444       =  5,*/
    {0, 0, 0,  0, 8},   /* VG_sL_8             =  6,*/
    {8, 8, 8, -1, 0},   /* VG_lRGBX_8888       =  7,*/
    {8, 8, 8,  8, 0},   /* VG_lRGBA_8888       =  8,*/
    {8, 8, 8,  8, 0},   /* VG_lRGBA_8888_PRE   =  9,*/
    {0, 0, 0,  0, 8},   /* VG_lL_8             = 10,*/
    {0, 0, 0,  8, 0},   /* VG_A_8              = 11,*/
    {0, 0, 0,  0, 1},   /* VG_BW_1             = 12,*/
    {0, 0, 0,  1, 0},   /* VG_A_1             = 12,*/
    {0, 0, 0,  4, 0},   /* VG_A_4             = 12,*/
};


void GetChannelBits(VGImageFormat format, gctINT32 *rbits, gctINT32 *gbits, gctINT32 *bbits, gctINT32 *abits, gctINT32 *lbits)
{
    gcmHEADER_ARG("format=%d rbits=0x%x gbits=0x%x bbits=0x%x abits=0x%x lbits=0x%x",
        format, rbits, gbits, bbits, abits, lbits);

    format &=  0xf;

    gcmASSERT(format <= VG_A_4);

    *rbits = bitsDesc[format][0];
    *gbits = bitsDesc[format][1];
    *bbits = bitsDesc[format][2];

    *abits = (bitsDesc[format][3] < 0 ? 0 : bitsDesc[format][3]);

    *lbits = bitsDesc[format][4];

    gcmFOOTER_NO();
}

void GetChannelCount(VGImageFormat format, gctINT32 *rCount, gctINT32 *gCount, gctINT32 *bCount, gctINT32 *aCount)
{
    gcmHEADER_ARG("format=%d rCount=0x%x gCount=0x%x bCount=0x%x aCount=0x%x",
        format, rCount, gCount, bCount, aCount);

    format &=  0xf;
    gcmASSERT(format <= VG_A_4);


    *rCount = bitsDesc[format][0];
    *gCount = bitsDesc[format][1];
    *bCount = bitsDesc[format][2];
    *aCount = (bitsDesc[format][3] < 0 ? 8 : bitsDesc[format][3]);

    gcmFOOTER_NO();
}

void GetShift(VGImageFormat format, gctINT32 *rshift, gctINT32 *gshift, gctINT32 *bshift, gctINT32 *ashift, gctINT32 *lshift)
{
    gctINT32 rCount, gCount, bCount, aCount;
    gctINT32 ordering = ((gctINT32)format >> 6) & 3;

    gcmHEADER_ARG("format=%d rshift=0x%x gshift=0x%x bshift=0x%x ashift=0x%x lshift=0x%x",
        format, rshift, gshift, bshift, ashift, lshift);

    GetChannelCount(format, &rCount, &gCount, &bCount, &aCount);

    /*RGBA*/
    if (0 == ordering)
    {
        *rshift = gCount + bCount + aCount;
        *gshift = bCount + aCount;
        *bshift = aCount;
        *ashift = 0;
    }

    /*ARGB*/
    if (1 == ordering)
    {
        *rshift = gCount + bCount;
        *gshift = bCount;
        *bshift = 0;
        *ashift = rCount + gCount + bCount;
    }

    /*BGRA*/
    if (2 == ordering)
    {
        *rshift = aCount;
        *gshift = rCount + aCount;
        *bshift = gCount + rCount + aCount;
        *ashift = 0;
    }

    /*ABGR*/
    if (3 == ordering)
    {
        *rshift = 0;
        *gshift = rCount;
        *bshift = gCount + rCount;
        *ashift = gCount + rCount + bCount;
    }

    *lshift = 0;

    gcmFOOTER_NO();
}


void _GetRawFormatColorDesc(VGImageFormat format, _VGColorDesc *colorDesc)
{
    gcmHEADER_ARG("format=%d colorDesc=0x%x", format, colorDesc);

    GetChannelBits(format, &colorDesc->rbits, &colorDesc->gbits, &colorDesc->bbits, &colorDesc->abits, &colorDesc->lbits);
    GetShift(format, &colorDesc->rshift, &colorDesc->gshift, &colorDesc->bshift, &colorDesc->ashift, &colorDesc->lshift);

    colorDesc->bpp          = ImageFormat2Bpp(format);
    colorDesc->colorFormat  = ImageFormat2ColorFormat(format);
    colorDesc->format       = format;
    colorDesc->surFormat    = gcvSURF_UNKNOWN;

    gcmFOOTER_NO();
}


void UnPackColor(_VGuint32 inputData,  _VGColorDesc * inputDesc, _VGColor *outputColor)
{
    gctINT32 rb = inputDesc->rbits;
    gctINT32 gb = inputDesc->gbits;
    gctINT32 bb = inputDesc->bbits;
    gctINT32 ab = inputDesc->abits;
    gctINT32 lb = inputDesc->lbits;
    gctINT32 rs = inputDesc->rshift;
    gctINT32 gs = inputDesc->gshift;
    gctINT32 bs = inputDesc->bshift;
    gctINT32 as = inputDesc->ashift;
    gctINT32 ls = inputDesc->lshift;

    gctFLOAT r, g, b, a;
    _VGColorFormat format = inputDesc->colorFormat;

    gcmHEADER_ARG("inputData=%u inputDesc=0x%x outputColor=0x%x", inputData, inputDesc, outputColor);

    if(lb)
    {
        r = g = b = intToColor(inputData >> ls, (1<<lb)-1);
        a = 1.0f;
    }
    else
    {
        r = rb ? intToColor(inputData >> rs, (1<<rb)-1) : (gctFLOAT)1.0f;
        g = gb ? intToColor(inputData >> gs, (1<<gb)-1) : (gctFLOAT)1.0f;
        b = bb ? intToColor(inputData >> bs, (1<<bb)-1) : (gctFLOAT)1.0f;
        a = ab ? intToColor(inputData >> as, (1<<ab)-1) : (gctFLOAT)1.0f;

        if(format & PREMULTIPLIED)
        {
            r = gcmMIN(r, a);
            g = gcmMIN(g, a);
            b = gcmMIN(b, a);
        }
    }

    outputColor->r      = r;
    outputColor->g      = g;
    outputColor->b      = b;
    outputColor->a      = a;
    outputColor->format = format;

    gcmFOOTER_NO();
}


_VGuint32 PackColor(_VGColor * srcColor, _VGColorDesc  *outputDesc)
{
    gctFLOAT     r = srcColor->r, g = srcColor->g, b = srcColor->b, a = srcColor->a;
    gctINT32 rb = outputDesc->rbits;
    gctINT32 gb = outputDesc->gbits;
    gctINT32 bb = outputDesc->bbits;
    gctINT32 ab = outputDesc->abits;
    gctINT32 lb = outputDesc->lbits;
    gctINT32 rs = outputDesc->rshift;
    gctINT32 gs = outputDesc->gshift;
    gctINT32 bs = outputDesc->bshift;
    gctINT32 as = outputDesc->ashift;
    gctINT32 ls = outputDesc->lshift;
    _VGuint32 ret;

    gcmHEADER_ARG("srcColor=0x%x outputDesc=0x%x", srcColor, outputDesc);

    gcmASSERT(r >= 0.0f && r <= 1.0f);
    gcmASSERT(g >= 0.0f && g <= 1.0f);
    gcmASSERT(b >= 0.0f && b <= 1.0f);
    gcmASSERT(a >= 0.0f && a <= 1.0f);
    gcmASSERT(!(srcColor->format & PREMULTIPLIED) || (r <= a && g <= a && b <= a));
    gcmASSERT((srcColor->format & LUMINANCE && r == g && r == b) || !(srcColor->format & LUMINANCE));



    if (lb)
    {
        gcmASSERT(srcColor->format & LUMINANCE);
        ret = colorToInt(r, (1<<lb)-1) << ls;
    }
    else
    {

        _VGuint32 cr = rb ? colorToInt(r, (1<<rb)-1) : 0;
        _VGuint32 cg = gb ? colorToInt(g, (1<<gb)-1) : 0;
        _VGuint32 cb = bb ? colorToInt(b, (1<<bb)-1) : 0;
        _VGuint32 ca = ab ? colorToInt(a, (1<<ab)-1) : 0;

        ret = (cr << rs) | (cg << gs) | (cb << bs) | (ca << as);
    }

    gcmFOOTER_ARG("return=%u", ret);

    return ret;
}


#define CONVERT_INT_COLOR(inputData, inputDesc, outputData, outputDesc) \
    do {\
        _VGColor color;\
        UnPackColor(inputData, inputDesc, &color);\
        ConvertColor(&color, (outputDesc)->colorFormat);\
        outputData = PackColor(&color, outputDesc);\
    }while(0)


#define READ_DATA_PIXEL(data, x, y, stride, bpp, p) \
    do {\
        switch(bpp)\
        {\
            case 32:\
            {\
                _VGuint32* s = ((_VGuint32*)((_VGuint8*)data + (y) * stride)) + (x);\
                p = (_VGuint32)*s;\
                break;\
            }\
            case 16:\
            {\
                _VGuint16* s = ((_VGuint16*)((_VGuint8*)data + (y) * stride)) + (x);\
                p = (_VGuint32)*s;\
                break;\
            }\
            case 8:\
            {\
                _VGuint8* s = ((_VGuint8*)((_VGuint8*)data + (y) * stride)) + (x);\
                p = (_VGuint32)*s;\
                break;\
            }\
            case 4:\
            {\
                _VGuint8* s = ((_VGuint8*)data + (y) * stride) + ((x)>>1);\
                p = (_VGuint32)(*s >> (((x)&1)<<2)) & 0xf;\
                break;\
            }\
            case 2:\
            {\
                _VGuint8* s = ((_VGuint8*)data +(y) * stride)+ ((x)>>2);\
                p = (_VGuint32)(*s >> (((x)&3)<<1)) & 0x3;\
                break;\
            }\
            default:\
            {\
                _VGuint8* s;\
                gcmASSERT(bpp == 1);\
                s = ((_VGuint8*)data + (y) * stride) + ((x)>>3);\
                p = (((_VGuint32)*s) >> ((x)&7)) & 1u;\
                break;\
            }\
        }\
    }while(0)


#define WRITE_DATA_PIXEL(data, x, y, stride, bpp, p) \
    do {\
        switch(bpp)\
        {\
            case 32:\
            {\
                _VGuint32* s = ((_VGuint32*)((_VGuint8*)data + (y) * stride)) + (x);\
                *s = (_VGuint32)p;\
                break;\
            }\
            case 16:\
            {\
                _VGuint16* s = ((_VGuint16*)((_VGuint8*)data + (y) * stride)) + (x);\
                *s = (_VGuint16)p;\
                break;\
            }\
            case 8:\
            {\
                _VGuint8* s = ((_VGuint8*)((_VGuint8*)data + (y) * stride)) + (x);\
                *s = (_VGuint8)p;\
                break;\
            }\
            case 4:\
            {\
                _VGuint8* s = ((_VGuint8*)data + (y) * stride) + ((x)>>1);\
                *s = (_VGuint8)((p << (((x)&1)<<2)) | ((_VGuint32)*s & ~(0xf << (((x)&1)<<2))));\
                break;\
            }\
            case 2:\
            {\
                _VGuint8* s = ((_VGuint8*)data + (y) * stride) + ((x)>>2);\
                *s = (_VGuint8)((p << (((x)&3)<<1)) | ((_VGuint32)*s & ~(0x3 << (((x)&3)<<1))));\
                break;\
            }\
            default:\
            {\
                _VGuint8* s, d;\
                gcmASSERT(bpp == 1);\
                s = ((_VGuint8*)data + (y) * stride) + ((x)>>3);\
                d = *s;\
                d &= ~(1<<(x&7));\
                d |= (_VGuint8)(p<<((x)&7));\
                *s = d;\
                break;\
            }\
        }\
    }while(0)


#define WRITE_SURFACE_PIXEL(Surface, Memory, X, Y, Format, PixelValue) \
        gcmVERIFY_OK(gcoSURF_WritePixel(Surface, Memory, X, Y,  Format, PixelValue));

#define READ_SURFACE_PIXEL(Surface, Memory, X, Y, Format, PixelValue) \
        gcmVERIFY_OK(gcoSURF_ReadPixel(Surface, Memory, X, Y,  Format, PixelValue));



void _VGImageCtor(gcoOS os, _VGImage *image)
{
    gcmHEADER_ARG("os=0x%x image=0x%x",
                  os, image);

    image->parent        = gcvNULL;
    image->hasChildren   = VG_FALSE;
    image->parentOffsetX = 0;
    image->parentOffsetY = 0;
    image->stride        = 0;
    image->data         = gcvNULL;
    image->surface      = gcvNULL;
    image->texture      = gcvNULL;
    image->texSurface   = gcvNULL;
    image->stream       = gcvNULL;
    image->eglUsed       = gcvFALSE;

    image->rootWidth    = 0;
    image->rootHeight   = 0;
    image->rootOffsetX  = 0;
    image->rootOffsetY  = 0;

    image->dirty        = gcvFALSE;
    image->dirtyPtr     = &image->dirty;
    image->wrapped      = gcvFALSE;
    image->orient       = vgvORIENTATION_LEFT_BOT;
    image->samples      = 1;

    gcoOS_ZeroMemory(&image->texStates, sizeof(gcsTEXTURE));

    image->texStates.s         = gcvTEXTURE_CLAMP;
    image->texStates.t         = gcvTEXTURE_CLAMP;
    image->texStates.r         = gcvTEXTURE_CLAMP;
    image->texStates.minFilter = gcvTEXTURE_POINT;
    image->texStates.magFilter = gcvTEXTURE_POINT;
    image->texStates.mipFilter = gcvTEXTURE_NONE;
    image->texStates.anisoFilter = gcvTEXTURE_NONE;

    image->texStates.border[0] = 0;
    image->texStates.border[1] = 0;
    image->texStates.border[2] = 0;
    image->texStates.border[3] = 0;
    image->texStates.lodBias   = 0;
    image->texStates.lodMin    = 0;
    image->texStates.lodMax    = -1;

    gcoOS_ZeroMemory(&image->internalColorDesc, sizeof(_VGColorDesc));
    gcoOS_ZeroMemory(&image->object, sizeof(_VGObject));

    gcmFOOTER_NO();
}

void _VGImageDtor(gcoOS os, _VGImage *image)
{
    gcmHEADER_ARG("os=0x%x image=0x%x",
                  os, image);
    if (image->stream)
    {
        gcmVERIFY_OK(gcoSTREAM_Destroy(image->stream));
    }



    if (image->parent == gcvNULL)
    {
        if (image->texture != gcvNULL)
        {
            gcmVERIFY_OK(gcoTEXTURE_Destroy(image->texture));
        }

        if (image->surface && !image->wrapped)
        {
            gcmVERIFY_OK(gcoSURF_Destroy(image->surface));
        }
    }

    image = image->parent;

    while(image != gcvNULL)
    {
        image->object.reference--;
        gcmASSERT(image->object.reference >= 0);

        if (image->object.reference == 0)
        {
            _VGImage    *temp;

            if (image->stream)
            {
                gcmVERIFY_OK(gcoSTREAM_Destroy(image->stream));
            }

            if (image->parent == gcvNULL)
            {
                if (image->texture != gcvNULL)
                {
                    gcmVERIFY_OK(gcoTEXTURE_Destroy(image->texture));
                }

                if (image->surface && !image->wrapped)
                {
                    gcmVERIFY_OK(gcoSURF_Destroy(image->surface));
                }
            }

            temp = image->parent;

            gcmVERIFY_OK(gcmOS_SAFE_FREE(os, image));

            image = temp;
        }
        else break;
    }

    gcmFOOTER_NO();
}


VGImage CreateChildImage(_VGContext *context, _VGImage *parent, VGint x, VGint y, VGint width, VGint height)
{
    _VGImage*   image = gcvNULL;
    VGImage     ret;

    gcmHEADER_ARG("context=0x%x parent=0x%x x=%d y=%d width=%d height=%d",
                  context, parent, x, y, width, height);

    NEWOBJ(_VGImage, context->os, image);

    if (!image)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        ret = VG_INVALID_HANDLE;

        gcmFOOTER_ARG("return=%d", ret);
        return ret;
    }

    if (!vgshInsertObject(context, &image->object, VGObject_Image)) {
        DELETEOBJ(_VGImage, context->os, image);
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        ret = VG_INVALID_HANDLE;

        gcmFOOTER_ARG("return=%d", ret);
        return ret;
    }

    VGObject_AddRef(context->os, &image->object);

    image->width            = width;
    image->height           = height;
    image->allowedQuality   = parent->allowedQuality;

    image->parentOffsetX    = x;
    image->parentOffsetY    = y;

    image->internalColorDesc = parent->internalColorDesc;
    image->texStates         = parent->texStates;


    image->parent       = parent;
    parent->hasChildren = VG_TRUE;

    VGObject_AddRef(context->os, &parent->object);

    image->texture          = parent->texture;
    image->surface          = parent->surface;
    image->texSurface       = parent->texSurface;
    image->orient           = parent->orient;
    image->dirtyPtr         = parent->dirtyPtr;

    image->rootWidth        = parent->rootWidth;
    image->rootHeight       = parent->rootHeight;

    image->rootOffsetX      = x + parent->rootOffsetX;
    image->rootOffsetY      = y + parent->rootOffsetY;

    gcmVERIFY_OK(vgshCreateImageStream(
        context, parent,
        0, 0, x, y, width, height,
        &image->stream));

    if (!image->stream)
    {
        DELETEOBJ(_VGImage, context->os, image);
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        ret = VG_INVALID_HANDLE;

        gcmFOOTER_ARG("return=%d", ret);
        return ret;
    }

    gcmFOOTER_ARG("return=%d", image->object.name);
    return (VGImage)image->object.name;

}




void SetColorDesc(_VGColorDesc *desc,
                         gctINT32 r,
                         gctINT32 g,
                         gctINT32 b,
                         gctINT32 a,
                         gctINT32 l,
                         gctINT32 bpp,
                         _VGColorFormat format)
{
    gcmHEADER_ARG("desc=0x%x r=%d g=%d b=%d a=%d l=%d bpp=%d format=%d",
        desc, r, g, b, a, l, bpp, format);

    desc->abits = a;
    desc->rbits = r;
    desc->gbits = g;
    desc->bbits = b;
    desc->lbits = l;

    desc->ashift = a ? (b + g + r) : 0;
    desc->rshift = b + g;
    desc->gshift = b;
    desc->bshift = 0;
    desc->lshift = 0;

    desc->bpp           = bpp;
    desc->colorFormat   = format;

    gcmFOOTER_NO();
}

void _GetSurfaceColorDesc(
    gcoSURF         surface,
    _VGColorDesc    *colorDesc)
{
    _VGColorFormat      colorFormat = lRGBA;
    gceSURF_COLOR_TYPE  colorType;

    static const VGImageFormat formats_8888[] =
    {
        VG_sRGBA_8888,
        VG_lRGBA_8888,
        VG_sRGBA_8888_PRE,
        VG_lRGBA_8888_PRE
    };

    static const VGImageFormat formats_8880[] =
    {
        VG_sRGBX_8888,
        VG_lRGBX_8888,
        VG_sRGBX_8888,
        VG_lRGBX_8888
    };

    gcmHEADER_ARG("surface =%d colorDesc=0x%x", surface, colorDesc);

    gcmVERIFY_OK(gcoSURF_GetColorType(surface, &colorType));

    gcmVERIFY_OK(gcoSURF_GetFormat(
        surface, gcvNULL, &colorDesc->surFormat));

    if (!(colorType & gcvSURF_COLOR_LINEAR))
    {
        colorFormat |= NONLINEAR;
    }

    if (colorType & gcvSURF_COLOR_ALPHA_PRE)
    {
        colorFormat |= PREMULTIPLIED;
    }

    switch(colorDesc->surFormat)
    {
    case gcvSURF_X8R8G8B8 :  /*R  G  B  A  L  BPP*/
        SetColorDesc(colorDesc, 8, 8, 8, 0, 0, 32, colorFormat);
        colorDesc->format = formats_8880[colorType];
        break;
    case gcvSURF_A8R8G8B8 :
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, colorFormat);
        colorDesc->format = formats_8888[colorType];
        break;
    case gcvSURF_R5G6B5 :
        SetColorDesc(colorDesc, 5, 6, 5, 0, 0, 16, colorFormat);
        colorDesc->format = VG_sRGB_565;
        break;
    case gcvSURF_A1R5G5B5 :
        SetColorDesc(colorDesc, 5, 5, 5, 1, 0, 16, colorFormat);
        colorDesc->format = VG_sRGBA_5551;
        break;
    case gcvSURF_A4R4G4B4 :
        SetColorDesc(colorDesc, 4, 4, 4, 4, 0, 16, colorFormat);
        colorDesc->format = VG_sRGBA_4444;
        break;
    default:
        break;
    }

    gcmFOOTER_NO();
}

void VGFormat2SurFormat(VGImageFormat format, gceSURF_COLOR_TYPE *colorType, gceSURF_FORMAT *surFormat)
{
    static const gceSURF_FORMAT formats[] =
    {
        gcvSURF_X8R8G8B8,    /* VG_sRGBX_8888     */
        gcvSURF_A8R8G8B8,    /* VG_sRGBA_8888     */
        gcvSURF_A8R8G8B8,    /* VG_sRGBA_8888_PRE */
        gcvSURF_R5G6B5,      /* VG_sRGB_565       */
        gcvSURF_A1R5G5B5,    /* VG_sRGBA_5551     */
        gcvSURF_A4R4G4B4,    /* VG_sRGBA_4444     */
        gcvSURF_X8R8G8B8,    /* VG_sL_8           */
        gcvSURF_X8R8G8B8,    /* VG_lRGBX_8888     */
        gcvSURF_A8R8G8B8,    /* VG_lRGBA_8888     */
        gcvSURF_A8R8G8B8,    /* VG_lRGBA_8888_PRE */
        gcvSURF_X8R8G8B8,    /* VG_lL_8           */
        gcvSURF_A8R8G8B8,    /* VG_A_8            */
        gcvSURF_X8R8G8B8,    /* VG_BW_1           */
        gcvSURF_A8R8G8B8,    /* VG_A_1            */
        gcvSURF_A8R8G8B8     /* VG_A_4            */
    };

    static const gceSURF_COLOR_TYPE types[] =
    {
        gcvSURF_COLOR_UNKNOWN,      /* VG_sRGBX_8888     */
        gcvSURF_COLOR_UNKNOWN,      /* VG_sRGBA_8888     */
        gcvSURF_COLOR_ALPHA_PRE,    /* VG_sRGBA_8888_PRE */
        gcvSURF_COLOR_UNKNOWN,      /* VG_sRGB_565       */
        gcvSURF_COLOR_UNKNOWN,      /* VG_sRGBA_5551     */
        gcvSURF_COLOR_UNKNOWN,      /* VG_sRGBA_4444     */
        gcvSURF_COLOR_UNKNOWN,      /* VG_sL_8           */
        gcvSURF_COLOR_LINEAR,       /* VG_lRGBX_8888     */
        gcvSURF_COLOR_LINEAR,       /* VG_lRGBA_8888     */

        gcvSURF_COLOR_LINEAR | gcvSURF_COLOR_ALPHA_PRE,  /* VG_lRGBA_8888_PRE */

        gcvSURF_COLOR_LINEAR,   /* VG_lL_8           */
        gcvSURF_COLOR_LINEAR,   /* VG_A_8            */
        gcvSURF_COLOR_LINEAR,   /* VG_BW_1           */
        gcvSURF_COLOR_LINEAR,   /* VG_A_1            */
        gcvSURF_COLOR_LINEAR    /* VG_A_4            */
    };

    gctINT32  imageFormat = format & 0xf;

    gcmHEADER_ARG("format=%d colorType=0x%x surfFormat=0x%x",
        format, colorType, surFormat);

    gcmASSERT(imageFormat >= (gctINT32)VG_sRGBX_8888 && imageFormat <= (gctINT32)VG_A_4);

    *colorType = types[imageFormat];
    *surFormat = formats[imageFormat];

    gcmFOOTER_NO();
}

void vgshGetFormatColorDesc(VGImageFormat format, _VGColorDesc *colorDesc)
{
    static const gceSURF_FORMAT formats[] =
    {
       gcvSURF_X8R8G8B8,    /* VG_sRGBX_8888     */
       gcvSURF_A8R8G8B8,    /* VG_sRGBA_8888     */
       gcvSURF_A8R8G8B8,    /* VG_sRGBA_8888_PRE */
       gcvSURF_X8R8G8B8,    /* VG_sRGB_565       */
       gcvSURF_A8R8G8B8,    /* VG_sRGBA_5551     */
       gcvSURF_A8R8G8B8,    /* VG_sRGBA_4444     */
       gcvSURF_X8R8G8B8,    /* VG_sL_8           */
       gcvSURF_X8R8G8B8,    /* VG_lRGBX_8888     */
       gcvSURF_A8R8G8B8,    /* VG_lRGBA_8888     */
       gcvSURF_A8R8G8B8,    /* VG_lRGBA_8888_PRE */
       gcvSURF_X8R8G8B8,    /* VG_lL_8           */
       gcvSURF_A8R8G8B8,    /* VG_A_8            */
       gcvSURF_X8R8G8B8,    /* VG_BW_1           */
       gcvSURF_A8R8G8B8,    /* VG_A_1            */
       gcvSURF_A8R8G8B8     /* VG_A_4            */
    };

    gctINT32  imageFormat = format & 0xf;

    gcmHEADER_ARG("format=%d  colorDesc=0x%x", format, colorDesc);

    gcmASSERT(imageFormat >= (gctINT32)VG_sRGBX_8888 && imageFormat <= (gctINT32)VG_A_4);

    colorDesc->surFormat = formats[imageFormat];
    colorDesc->format    = format;

    switch(imageFormat)
    {
    case VG_sRGBX_8888 :      /*R  G  B  A  L  BPP*/
        SetColorDesc(colorDesc, 8, 8, 8, 0, 0, 32, sRGBA);
        break;
    case VG_sRGBA_8888 :
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, sRGBA);
        break;
    case VG_sRGBA_8888_PRE :
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, sRGBA_PRE);
        break;
    case VG_sRGB_565 :
        SetColorDesc(colorDesc, 8, 8, 8, 0, 0, 32, (_VGColorFormat) (sRGBA|ARGB_5650));
        break;
    case VG_sRGBA_5551 :
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, (_VGColorFormat) (sRGBA|ARGB_5551));
        break;
    case VG_sRGBA_4444 :
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, (_VGColorFormat) (sRGBA|ARGB_4444));
        break;
    case VG_sL_8:
        SetColorDesc(colorDesc, 8, 8, 8, 0, 0, 32, sLA);
        break;
    case VG_lRGBX_8888 :
        SetColorDesc(colorDesc, 8, 8, 8, 0, 0, 32, lRGBA);
        break;
    case VG_lRGBA_8888 :
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, lRGBA);
        break;
    case VG_lRGBA_8888_PRE:
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, lRGBA_PRE);
        break;
    case VG_lL_8:
        SetColorDesc(colorDesc, 8, 8, 8, 0, 0, 32, lLA);
        break;
    case VG_A_8:
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, (_VGColorFormat) (lRGBA|ALPHA_8));
        break;
    case VG_BW_1:
        SetColorDesc(colorDesc, 8, 8, 8, 0, 0, 32, (_VGColorFormat) (lLA|BITWISE));
        break;
    case VG_A_1:
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, (_VGColorFormat) (lRGBA|ALPHA_1));
        break;
    case VG_A_4:
        SetColorDesc(colorDesc, 8, 8, 8, 8, 0, 32, (_VGColorFormat) (lRGBA|ALPHA_4));
        break;
    default: break;
    }

    gcmFOOTER_NO();
}

VGImage  vgCreateImage(VGImageFormat format, VGint width, VGint height, VGbitfield allowedQuality)
{
    _VGImage*       image = gcvNULL;
    VGImage         ret;
    _VGContext      *context;
    _VGColorDesc    colorDesc;

    gcmHEADER_ARG("format=%d width=%d height=%d allowQuality=0x%x",
                  format, width, height, allowedQuality);
    OVG_GET_CONTEXT(VG_INVALID_HANDLE);

    vgmPROFILE(context, VGCREATEIMAGE, 0);
    OVG_IF_ERROR(!isValidImageFormat(format), VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, VG_INVALID_HANDLE);

    OVG_IF_ERROR(width <= 0 || height <= 0 || !allowedQuality ||
                (allowedQuality & ~(VG_IMAGE_QUALITY_NONANTIALIASED | VG_IMAGE_QUALITY_FASTER | VG_IMAGE_QUALITY_BETTER)), VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

    OVG_IF_ERROR(width > MAX_IMAGE_WIDTH || height > MAX_IMAGE_HEIGHT || width*height > MAX_IMAGE_PIXELS ||
                ((width * ImageFormat2Bpp(format) + 7 ) / 8) * height > MAX_IMAGE_BYTES, VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

    NEWOBJ(_VGImage, context->os, image);

    if (!image)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        ret = VG_INVALID_HANDLE;

        gcmFOOTER_ARG("return=%d", ret);
        return ret;
    }

    if (!vgshInsertObject(context, &image->object, VGObject_Image)) {
        DELETEOBJ(_VGImage, context->os, image);
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        ret = VG_INVALID_HANDLE;

        gcmFOOTER_ARG("return=%d", ret);
        return ret;
    }

    VGObject_AddRef(context->os, &image->object);


    vgshGetFormatColorDesc(format, &colorDesc);

    gcmVERIFY_OK(vgshIMAGE_Initialize(
        context, image, &colorDesc,
        width, height,
        gcvORIENTATION_BOTTOM_TOP));

    image->allowedQuality = allowedQuality;

    {
		_VGColor zeroColor;
		COLOR_SET(zeroColor, 0.0, 0.0, 0.0, 0.0, sRGBA);

        gcmVERIFY_OK(vgshClear(context,
                            image,
                            0, 0, width, height,
                            &zeroColor,
                            gcvFALSE,
                            gcvTRUE));
    }

    gcmFOOTER_ARG("return=%d", image->object.name);
    return (VGImage)image->object.name;
}


gceSTATUS vgshIMAGE_Initialize(
    _VGContext      *context,
    _VGImage        *image,
    _VGColorDesc    *desc,
    gctINT32        width,
    gctINT32        height,
    gceORIENTATION  orientation)
{
    image->width            = width;
    image->height           = height;

    image->rootWidth        = width;
    image->rootHeight       = height;
    image->rootOffsetX      = 0;
    image->rootOffsetY      = 0;
    image->orient           = orientation;

    image->allowedQuality =
        VG_IMAGE_QUALITY_NONANTIALIASED
        | VG_IMAGE_QUALITY_FASTER
        | VG_IMAGE_QUALITY_BETTER;

    image->internalColorDesc = *desc;

    gcmVERIFY_OK(vgshCreateImageStream(
        context, image,
        0, 0, 0, 0, width, height,
        &image->stream));

    if (!image->stream)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        OVG_RETURN(VG_INVALID_HANDLE);
    }

    gcmVERIFY_OK(vgshCreateTexture(
        context,
        image->width,
        image->height,
        image->internalColorDesc.surFormat,
        &image->texture,
        &image->texSurface));

    gcmVERIFY_OK(gcoSURF_Construct(
        context->hal,
        image->width,
        image->height, 1,
        gcvSURF_RENDER_TARGET_NO_TILE_STATUS,
        image->internalColorDesc.surFormat,
        gcvPOOL_DEFAULT,
        &image->surface));

    gcmVERIFY_OK(gcoSURF_SetOrientation(
        image->surface,
        orientation));

    gcmVERIFY_OK(gcoSURF_SetOrientation(
        image->texSurface,
        orientation));

    return gcvSTATUS_OK;
}

gceSTATUS vgshIMAGE_WrapFromSurface(
    _VGContext  *context,
    _VGImage    *image,
    gcoSURF     surface)
{
    gceORIENTATION orient;

    gcmHEADER_ARG("context=0x%x image=0x%x", context, image);


    gcmVERIFY_OK(gcoSURF_GetSize(surface,
                           (gctUINT*) &image->width,
                           (gctUINT*) &image->height,
			    gcvNULL));

    gcmVERIFY_OK(gcoSURF_GetSamples(surface,
                            &image->samples));

    gcmVERIFY_OK(gcoSURF_QueryOrientation(surface,  &orient));

    _GetSurfaceColorDesc(surface, &image->internalColorDesc);

    image->rootWidth    = image->width;
    image->rootHeight   = image->height;
    image->rootOffsetX  = 0;
    image->rootOffsetY  = 0;

    image->orient       = orient;
    image->surface      = surface;
    image->wrapped      = gcvTRUE;

    gcmVERIFY_OK(vgshCreateTexture(
        context,
        image->width, image->height,
        image->internalColorDesc.surFormat,
        &image->texture, &image->texSurface));

    gcmVERIFY_OK(gcoSURF_SetOrientation(image->texSurface,  orient));

    gcmFOOTER_NO();
    return gcvSTATUS_OK;
}

gceSTATUS vgshIMAGE_UploadData(
    _VGContext  *context,
    _VGImage    *image,
    const void  *data,
    VGint           dataStride,
    VGImageFormat   dataFormat,
    gctINT32    dx,
    gctINT32    dy,
    gctINT32    sx,
    gctINT32    sy,
    gctINT32    width,
    gctINT32    height)
{
    dx += image->rootOffsetX;
    dy += image->rootOffsetY;

    DoWriteData(context,
        image->texSurface,
        &image->internalColorDesc,
        data, dataStride, dataFormat,
        dx, dy, sx, sy, width, height);

    return gcvSTATUS_OK;
}

gceSTATUS vgshIMAGE_GetData(
    _VGContext      *context,
    _VGImage        *image,
    void            *data,
    gctINT32        dataStride,
    VGImageFormat   dataFormat,
    gctINT32        dx,
    gctINT32        dy,
    gctINT32        sx,
    gctINT32        sy,
    gctINT32        width,
    gctINT32        height)
{
    gcmVERIFY_OK(gcoSURF_Flush(image->surface));

    gcmVERIFY_OK(gco3D_Semaphore(
        context->engine,
        gcvWHERE_RASTER, gcvWHERE_PIXEL,
        gcvHOW_SEMAPHORE_STALL));

    gcmVERIFY_OK(gcoHAL_Commit(context->hal, gcvTRUE));

    sx += image->rootOffsetX;
    sy += image->rootOffsetY;

    DoReadDataPixels(
        context,
        image->surface,
        &image->internalColorDesc,
        data, dataStride, dataFormat,
        dx, dy, sx, sy, width, height,
        image->orient == vgvORIENTATION_LEFT_TOP);

    return gcvSTATUS_OK;
}

gceSTATUS vgshIMAGE_WrapFromData(
    _VGContext      *context,
    _VGImage        *image,
    const void      *data,
    gctINT32        dataStride,
    VGImageFormat   dataFormat,
    gctINT32        width,
    gctINT32        height)
{
    image->data       = (void*)data;
    image->stride     = dataStride;
    image->width      = width;
    image->height     = height;

    image->wrapped      = gcvTRUE;

    image->rootWidth    = image->width;
    image->rootHeight   = image->height;
    image->rootOffsetX  = 0;
    image->rootOffsetY  = 0;

    _GetRawFormatColorDesc(
        dataFormat,
        &image->internalColorDesc);

    return gcvSTATUS_OK;
}

gceSTATUS vgshIMAGE_SetSamples(
    _VGContext      *context,
    _VGImage        *image,
    gctUINT32        samples)
{
    gcmASSERT(image->surface != gcvNULL);

    return gcoSURF_SetSamples(image->surface, samples);
}

void  vgDestroyImage(VGImage image)
{
    _VGImage *imageObjPtr;
    _VGContext* context;

    gcmHEADER_ARG("image=%d", image);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGDESTROYIMAGE, 0);

    imageObjPtr = _VGIMAGE(context, image);

    OVG_IF_ERROR(!imageObjPtr, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    vgshRemoveObject(context, &imageObjPtr->object);

    VGObject_Release(context->os, &imageObjPtr->object);

    gcmFOOTER_NO();
}



#if gcdDUMP
static void
_DumpTexture(
    gcoOS      Os,
	gcoSURF Surface
	)
{
	gctUINT32 address;
	gctPOINTER memory;
	gctUINT width, height;
	gctINT stride;

	if (Surface != gcvNULL)
	{
		gcmVERIFY_OK(gcoSURF_Lock(Surface, &address, &memory));

		gcmVERIFY_OK(gcoSURF_GetAlignedSize(Surface, &width, &height, &stride));

		gcmDUMP_BUFFER(Os, "texture", address, memory, 0, height * stride);

		gcmVERIFY_OK(gcoSURF_Unlock(Surface, memory));
	}

}
#endif


void  vgImageSubData(VGImage image, const void * data, VGint dataStride, VGImageFormat dataFormat, VGint x, VGint y, VGint width, VGint height)
{
    _VGImage        *imageObjPtr, tempImage;
    _VGContext      *context;

    gcmHEADER_ARG("image=%d data=0x%x dataStride=%d dataFormat=%d "
                  "x=%d y=%d width=%d height=%d",
                  image, data, dataStride, dataFormat,
                  x, y, width, height);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGIMAGESUBDATA, 0);

    imageObjPtr = _VGIMAGE(context, image);

    OVG_IF_ERROR(!imageObjPtr, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageObjPtr), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!isValidImageFormat(dataFormat), VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!data || !isImageAligned(data, dataFormat) || width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);


    _VGImageCtor(context->os, &tempImage);

    gcmVERIFY_OK(vgshIMAGE_WrapFromData(
        context,
        &tempImage, data, dataStride, dataFormat,
        width, height));

    gcmVERIFY_OK(vgshIMAGE_Blit(
        context, imageObjPtr, &tempImage,
        x, y, 0, 0, width, height,
        vgvBLIT_COLORMASK_ALL | vgvBLIT_FROMDATA));

    _VGImageDtor(context->os, &tempImage);



#if gcdDUMP
    _DumpTexture(context->os, imageObjPtr->surface);
#endif
    gcmFOOTER_NO();
}

void  vgDrawImage(VGImage image)
{
    _VGImage        *imageObjPtr;
    _VGContext* context;

    gcmHEADER_ARG("image=%d", image);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGDRAWIMAGE, 0);
    vgmPROFILE(context, VG_PROFILER_PRIMITIVE_TYPE, VG_IMAGE);
    vgmPROFILE(context, VG_PROFILER_PRIMITIVE_COUNT, 1);

    imageObjPtr = _VGIMAGE(context, image);

    OVG_IF_ERROR(!imageObjPtr, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageObjPtr), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);

	/*
    if (context->renderingQuality != VG_RENDERING_QUALITY_NONANTIALIASED)
	{
		gcmVERIFY_OK(gco3D_SetAntiAlias(context->engine, gcvFALSE));
	}*/
    gcmVERIFY_OK(vgshDrawImage(context, imageObjPtr, &context->imageUserToSurface));
	/*
    if (context->renderingQuality != VG_RENDERING_QUALITY_NONANTIALIASED)
	{
		gcmVERIFY_OK(gco3D_SetAntiAlias(context->engine, gcvTRUE));
	}*/

    gcmFOOTER_NO();
}

void intersect(_VGRectangle *rect1, _VGRectangle *rect2, _VGRectangle *out)
{
    gcmHEADER_ARG("rect1=0x%x rect2=0x%x out=0x%x", rect1, rect2, out);

    if(rect1->width >= 0 && rect2->width >= 0 && rect1->height >= 0 && rect2->height >= 0)
    {
        gctINT32 x1, y1;
		_VGint32 temp1, temp2;
		temp1 = ADDSATURATE_INT(rect1->x, rect1->width);
		temp2 = ADDSATURATE_INT(rect2->x, rect2->width);
        x1 = gcmMIN(temp1, temp2);
        out->x = gcmMAX(rect1->x, rect2->x);
        out->width = gcmMAX(x1 - out->x, 0);

        temp1 = ADDSATURATE_INT(rect1->y, rect1->height);
		temp2 = ADDSATURATE_INT(rect2->y, rect2->height);
        y1 = gcmMIN(temp1, temp2);
        out->y = gcmMAX(rect1->y, rect2->y);
        out->height = gcmMAX(y1 - out->y, 0);
    }
    else
    {
        out->x = 0;
        out->y = 0;
        out->width = 0;
        out->height = 0;
    }

    gcmFOOTER_NO();
}


void  vgClearImage(VGImage image, VGint x, VGint y, VGint width, VGint height)
{
    _VGImage    *imageObjPtr;
    _VGuint32   colorData;

    _VGColor clearColor, outColor;
    _VGContext* context;
    gctINT32 dx = x, dy = y, sx = 0, sy = 0;

    gcmHEADER_ARG("image=%d x=%d y=%d width=%d height=%d", image, x, y, width, height);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGCLEARIMAGE, 0);

    imageObjPtr = _VGIMAGE(context, image);

    OVG_IF_ERROR(!imageObjPtr, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageObjPtr), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    if (!vgshGetIntersectArea(
        &dx, &dy, &sx, &sy, &width, &height,
        imageObjPtr->width, imageObjPtr->height,
        width, height))
    {
        gcmFOOTER_NO();
        return;
    }

    clearColor = context->inputClearColor;
    COLOR_CLAMP(clearColor);
    ConvertColor(&clearColor, imageObjPtr->internalColorDesc.colorFormat);
    colorData = PackColor(&clearColor, &imageObjPtr->internalColorDesc);
    UnPackColor(colorData, &imageObjPtr->internalColorDesc, &outColor);

    gcmVERIFY_OK(vgshClear(context,
                        imageObjPtr,
                        dx, dy, width, height,
                        &outColor,
                        gcvFALSE,
                        gcvTRUE));

    gcmFOOTER_NO();
}


void  vgGetImageSubData(VGImage image, void * data, VGint dataStride, VGImageFormat dataFormat,
                        VGint x, VGint y, VGint width, VGint height)
{
    _VGImage        *imageObjPtr, tempImage;
    _VGContext      *context;

    gcmHEADER_ARG("image=%d data=0x%x dataStride=%d dataFormat=%d x=%d y=%d width=%d height=%d",
                  image, data, dataStride, dataFormat, x, y, width, height);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGGETIMAGESUBDATA, 0);

    imageObjPtr = _VGIMAGE(context, image);

    OVG_IF_ERROR(!image || !imageObjPtr, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageObjPtr), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!isValidImageFormat(dataFormat), VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!data || !isImageAligned(data, dataFormat) || width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    _VGImageCtor(context->os, &tempImage);

    gcmVERIFY_OK(vgshIMAGE_WrapFromData(
        context,
        &tempImage,
        data, dataStride, dataFormat,
        width, height));

    gcmVERIFY_OK(vgshIMAGE_Blit(
        context, &tempImage, imageObjPtr,
        0, 0, x, y, width, height, vgvBLIT_COLORMASK_ALL| vgvBLIT_TODATA));

    _VGImageDtor(context->os, &tempImage);

    gcmFOOTER_NO();
}



VGImage  vgChildImage(VGImage parent, VGint x, VGint y, VGint width, VGint height)
{
    _VGImage        *imageObjPtr;
    VGImage         ret;
    _VGContext* context;

    gcmHEADER_ARG("parent=%d x=%d y=%d width=%d height=%d",
                  parent, x, y, width, height);
    OVG_GET_CONTEXT(VG_INVALID_HANDLE);

    vgmPROFILE(context, VGCHILDIMAGE, 0);

    imageObjPtr = _VGIMAGE(context, parent);

    OVG_IF_ERROR(!imageObjPtr, VG_BAD_HANDLE_ERROR, VG_INVALID_HANDLE);
    OVG_IF_ERROR(eglIsInUse(imageObjPtr), VG_IMAGE_IN_USE_ERROR, VG_INVALID_HANDLE);
    OVG_IF_ERROR(x < 0 || x >= imageObjPtr->width || y < 0 || y >= imageObjPtr->height ||
                width <= 0 || height <= 0 || ADDSATURATE_INT(x, width) > imageObjPtr->width ||
                ADDSATURATE_INT(y, height) > imageObjPtr->height, VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

    ret = CreateChildImage(context, imageObjPtr, x, y, width, height);

    gcmFOOTER_ARG("return=%d", ret);
    return ret;
}


VGImage  vgGetParent(VGImage image)
{
    _VGImage        *imageObjPtr, *parent;
    _VGContext* context;

    gcmHEADER_ARG("image=%d", image);
    OVG_GET_CONTEXT(VG_INVALID_HANDLE);

    vgmPROFILE(context, VGGETPARENT, 0);

    imageObjPtr = _VGIMAGE(context, image);
    OVG_IF_ERROR(!imageObjPtr, VG_BAD_HANDLE_ERROR, VG_INVALID_HANDLE);

    parent = imageObjPtr->parent;
    while(parent)
    {
        if (_VGIMAGE(context, (VGImage)parent->object.name))
        {
            imageObjPtr = parent;
            break;
        }
        parent = parent->parent;
    }

    gcmFOOTER_ARG("return=%d", imageObjPtr->object.name);
    return (VGImage)imageObjPtr->object.name;
}

gctBOOL vgshGetIntersectArea(
    gctINT32 *pdx, gctINT32 *pdy,
    gctINT32 *psx, gctINT32 *psy,
    gctINT32 *pw,  gctINT32 *ph,
    gctINT32 dstW, gctINT32 dstH,
    gctINT32 srcW, gctINT32 srcH)
{
    gctINT32 srcsx, srcex, dstsx, dstex;
    gctINT32 srcsy, srcey, dstsy, dstey;

    gctINT32 dx = *pdx, dy = *pdy, sx = *psx, sy = *psy, w = *pw, h = *ph;

    gcmHEADER_ARG("pdx=0x%x pdy=0x%x psx=0x%x psy=0x%x pw=0x%x ph=0x%x dstW=%d dstH=%d srcW=%d srcH=%d",
        pdx, pdy, psx, psy, pw, ph, dstW, dstH, srcW, srcH);

    sx = gcmMIN(gcmMAX(sx, (gctINT32)(INT32_MIN>>2)), (gctINT32)(INT32_MAX>>2));
    sy = gcmMIN(gcmMAX(sy, (gctINT32)(INT32_MIN>>2)), (gctINT32)(INT32_MAX>>2));
    dx = gcmMIN(gcmMAX(dx, (gctINT32)(INT32_MIN>>2)), (gctINT32)(INT32_MAX>>2));
    dy = gcmMIN(gcmMAX(dy, (gctINT32)(INT32_MIN>>2)), (gctINT32)(INT32_MAX>>2));
    w = gcmMIN(w, (gctINT32)(INT32_MAX>>2));
    h = gcmMIN(h, (gctINT32)(INT32_MAX>>2));

    srcsx = sx;
    srcex = sx + w;
    dstsx = dx;
    dstex = dx + w;

    if(srcsx < 0)
    {
        dstsx -= srcsx;
        srcsx = 0;
    }
    if(srcex > srcW)
    {
        dstex -= srcex - srcW;
        srcex = srcW;
    }
    if(dstsx < 0)
    {
        srcsx -= dstsx;
        dstsx = 0;
    }
    if(dstex > dstW)
    {
        srcex -= dstex - dstW;
        dstex = dstW;
    }

    gcmASSERT(srcsx >= 0 && dstsx >= 0 && srcex <= srcW && dstex <= dstW);
    w = srcex - srcsx;
    gcmASSERT(w == dstex - dstsx);

    if(w <= 0)
    {
        gcmFOOTER_ARG("return=%s", "FALSE");

        return gcvFALSE;
    }

    srcsy = sy;
    srcey = sy + h;
    dstsy = dy;
    dstey = dy + h;

    if(srcsy < 0)
    {
        dstsy -= srcsy;
        srcsy = 0;
    }
    if(srcey > srcH)
    {
        dstey -= srcey - srcH;
        srcey = srcH;
    }
    if(dstsy < 0)
    {
        srcsy -= dstsy;
        dstsy = 0;
    }
    if(dstey > dstH)
    {
        srcey -= dstey - dstH;
        dstey = dstH;
    }
    gcmASSERT(srcsy >= 0 && dstsy >= 0 && srcey <= srcH && dstey <= dstH);
    h = srcey - srcsy;
    gcmASSERT(h == dstey - dstsy);

    if(h <= 0)
    {
        gcmFOOTER_ARG("return=%s", "FALSE");

        return gcvFALSE;
    }

    *pdx = dstsx;
    *pdy = dstsy;
    *psx = srcsx;
    *psy = srcsy;
    *pw  = w;
    *ph  = h;

    gcmFOOTER_ARG("return=%s", "TRUE");

    return gcvTRUE;
}


void  vgCopyImage(VGImage dst, VGint dx, VGint dy, VGImage src, VGint sx, VGint sy, VGint width, VGint height, VGboolean dither)
{
    _VGImage        *imageSrc;
    _VGImage        *imageDst;

    _VGContext* context;

    gcmHEADER_ARG("dst=%d dx=%d dy=%d src=%d sx=%d sy=%d width=%d height=%d dither=%s",
                  dst, dx, dy, src, sx, sy, width, height,
                  dither ? "VG_TRUE" : "VG_FALSE");
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGCOPYIMAGE, 0);

    imageSrc = _VGIMAGE(context, src);
    imageDst = _VGIMAGE(context, dst);

    OVG_IF_ERROR(!imageSrc || !imageDst, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageDst) || eglIsInUse(imageSrc), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);


    gcmVERIFY_OK(vgshIMAGE_Blit(context, imageDst, imageSrc,
        dx, dy, sx, sy, width, height, vgvBLIT_COLORMASK_ALL));


    gcmFOOTER_NO();
}


void  vgSetPixels(VGint dx, VGint dy, VGImage src, VGint sx, VGint sy, VGint width, VGint height)
{

    _VGImage        *imageObjPtr;

    gctUINT32        flag;
    _VGContext      * context;
    gcmHEADER_ARG("dx=%d dy=%d src=%d sx=%d sy=%d width=%d height=%d",
                  dx, dy, src, sx, sy, width, height);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSETPIXELS, 0);

    imageObjPtr = _VGIMAGE(context, src);
    OVG_IF_ERROR(!imageObjPtr, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageObjPtr), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);


    flag = vgshIsScissoringEnabled(context) ? vgvBLIT_SCISSORING : 0;
    flag |= vgvBLIT_COLORMASK_ALL;

    gcmVERIFY_OK(vgshIMAGE_Blit(context,
        &context->targetImage,
        imageObjPtr,
        dx, dy, sx, sy, width, height,
        flag));

    gcmFOOTER_NO();
}

void DoReadDataPixels(_VGContext *context,
               gcoSURF    srcSurface,
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
               gctBOOL fromDraw)
{
    gctUINT32  address[3];
    gctPOINTER memory[3];
    gctUINT32  pixel;
    gctINT32   x, y;
    gctUINT    sw, sh;
    gctINT    stride;
    gceSURF_FORMAT format;

    _VGColorDesc outputDesc;

    gcoSURF     tempSurf;
/*	gcsPOINT src, dst, rect;*/

    gcmHEADER_ARG("context=0x%x srcSurface=0x%x data=0x%x dataStride=0x%x dataFormat=%d "
                  "dx=%d dy=%d sx=%d sy=%d width=%d height=%d fromtDraw=%s",
                   context, srcSurface, data, dataStride, dataFormat,
                   dx, dy, sx, sy, width, height,
                   fromDraw ? "gcvTRUE" : "gcvFALSE");

    _GetRawFormatColorDesc(dataFormat, &outputDesc);

    gcmVERIFY_OK(gcoSURF_GetSize(srcSurface, &sw, &sh, gcvNULL));
    gcmVERIFY_OK(gcoSURF_GetFormat(srcSurface, gcvNULL, &format));


    gcmVERIFY_OK(gcoSURF_Construct(context->hal,
                                   width, height, 1,
                                   gcvSURF_BITMAP,
                                   format,
                                   gcvPOOL_DEFAULT,
                                   &tempSurf));

    gcmVERIFY_OK(gcoSURF_Lock(tempSurf, address, memory));
    gcmVERIFY_OK(gcoSURF_GetAlignedSize(tempSurf, gcvNULL, gcvNULL, &stride));

    if (fromDraw)
    {
        gcmVERIFY_OK(gcoSURF_CopyPixels(srcSurface, tempSurf, sx, sh - height - sy, 0, 0, width, -height));
    }
    else
    {
        gcmVERIFY_OK(gcoSURF_CopyPixels(srcSurface, tempSurf, sx, sy, 0, 0, width, height));
    }

    /* Walk all lines. */
    for (y = 0; y < height; y++)
    {
        /* Walk all pixels. */
        for (x = 0; x < width; x++)
        {
            /* Read frame buffer pixel. */
            READ_DATA_PIXEL(memory[0], x, y, stride, srcColorDesc->bpp, pixel);
            CONVERT_INT_COLOR(pixel, srcColorDesc, pixel, &outputDesc);
            WRITE_DATA_PIXEL(data, (x + dx), (y + dy), dataStride, outputDesc.bpp, pixel);
        }
    }

    gcmVERIFY_OK(gcoSURF_Unlock(tempSurf, memory[0]));

    gcmVERIFY_OK(gcoSURF_Destroy(tempSurf));

    gcmFOOTER_NO();
}

void DoWriteData(_VGContext *context,
              gcoSURF      dstSurface,
              _VGColorDesc *dstColorDesc,
              const void   * data,
              VGint         dataStride,
              VGImageFormat dataFormat,
              VGint dx, VGint dy, VGint sx, VGint sy, VGint width, VGint height)
{
    gctUINT32  address[3];
    gctPOINTER memory[3];
    gctUINT32  pixel;
    gctINT32   i, j;
    gctUINT    dh, dw;
    gceSURF_FORMAT format;
    gcoSURF     tempSurf;
    gctINT    stride;
    _VGColorDesc inputDesc;
/*	gcsPOINT src, dst, rect;*/

    gcmHEADER_ARG("context=0x%x dstSurface=0x%x dstColorDesc=0x%x data=0x%x dataStride=%d "
                  "dataFormat=%d dx=%d dy=%d sx=%d sy=%d width=%d height=%d",
                   context, dstSurface, dstColorDesc, data, dataStride, dataFormat,
                   dx, dy, sx, sy, width, height);

    _GetRawFormatColorDesc(dataFormat, &inputDesc);

    gcmVERIFY_OK(gcoSURF_GetSize(dstSurface, &dw, &dh, gcvNULL));

    gcmVERIFY_OK(gcoSURF_GetFormat(dstSurface, gcvNULL, &format));

    gcmVERIFY_OK(gcoSURF_Construct(context->hal,
                                   width, height, 1,
                                   gcvSURF_BITMAP,
                                   format,
                                   gcvPOOL_DEFAULT,
                                   &tempSurf));

    gcmVERIFY_OK(gcoSURF_Lock(tempSurf, address, memory));

    gcmVERIFY_OK(gcoSURF_GetAlignedSize(tempSurf, gcvNULL, gcvNULL, &stride));


    for (j = 0; j < height; j++)
    {
        for (i = 0; i < width; i++)
        {
            READ_DATA_PIXEL(data, sx + i, sy + j, dataStride, inputDesc.bpp, pixel);

            CONVERT_INT_COLOR(pixel, &inputDesc, pixel, dstColorDesc);

            WRITE_DATA_PIXEL(memory[0], i, j, stride, dstColorDesc->bpp, pixel);
        }
    }

    gcmVERIFY_OK(gcoSURF_CopyPixels(tempSurf, dstSurface, 0, 0, dx, dy, width, height));

    gcmVERIFY_OK(gcoSURF_Unlock(tempSurf, memory[0]));

    gcmVERIFY_OK(gcoSURF_Destroy(tempSurf));

    gcmFOOTER_NO();
}




gceSTATUS _Blit(
    _VGContext      *context,
    _VGImage        *dstImage,
    _VGImage        *srcImage,
    gctINT32        dx,
    gctINT32        dy,
    gctINT32        sx,
    gctINT32        sy,
    gctINT32        width,
    gctINT32        height,
    gctBOOL         scissoring,
    gctUINT8        colorWrite)
{
    _vgHARDWARE *hardware = &context->hardware;
    gceSTATUS status = gcvSTATUS_OK;
    _VGColorDesc *srcColorDesc = &srcImage->internalColorDesc;
    _VGColorDesc *dstColorDesc = &dstImage->internalColorDesc;

    gcmHEADER_ARG("context=0x%x dstImage=0x%x srcImage=0x%x dx=%d dy=%d "
                  " sx=%d sy=%d width=%d height=%d "
                  "scissoring=%s  colorWrite=%d",
                  context, dstImage, srcImage, dx, dy,  sx, sy, width, height,
                  scissoring ? "gcvTRUE" : "gcvFALSE",
                  colorWrite);

    do
    {
		hardware->dstImage      = dstImage;
        hardware->srcImage      = srcImage;

        hardware->blending      = gcvFALSE;
        hardware->masking       = gcvFALSE;
        hardware->colorTransform = gcvFALSE;
        hardware->depthCompare  = gcvCOMPARE_ALWAYS;
        hardware->depthWrite    = gcvFALSE;
        hardware->flush         = gcvFALSE;
        hardware->colorWrite    = colorWrite;

        hardware->drawPipe      = vgvDRAWPIPE_COPY;
        hardware->filterType    = vgvFILTER_COPY;

        hardware->sx    = sx;
        hardware->sy    = sy;
        hardware->dx    = dx;
        hardware->dy    = dy;
        hardware->width  = width;
        hardware->height = height;


        hardware->srcConvert        = getColorConvertValue(srcColorDesc->colorFormat, srcColorDesc->colorFormat);
        hardware->srcConvertAlpha   = getColorConvertAlphaValue(srcColorDesc->colorFormat, srcColorDesc->colorFormat);

        hardware->dstConvert        = getColorConvertValue(srcColorDesc->colorFormat, dstColorDesc->colorFormat);
        hardware->dstConvertAlpha   = getColorConvertAlphaValue(srcColorDesc->colorFormat, dstColorDesc->colorFormat);

        hardware->pack              = (gctUINT32)(dstColorDesc->colorFormat >> 16);
        hardware->alphaOnly         = (dstColorDesc->colorFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;

        if (scissoring)
        {
            hardware->stencilMode       = gcvSTENCIL_SINGLE_SIDED;
            hardware->stencilMask       = 0xff;
            hardware->stencilRef        = 0;
            hardware->stencilCompare    = gcvCOMPARE_NOT_EQUAL;
            hardware->stencilFail       = gcvSTENCIL_KEEP;
            hardware->depthMode         = gcvDEPTH_Z;
#if NO_STENCIL_VG
            {
                hardware->stencilMode       = gcvSTENCIL_NONE;
                hardware->stencilCompare    = gcvCOMPARE_ALWAYS;
                hardware->depthCompare      = gcvCOMPARE_GREATER;/*gcvCOMPARE_EQUAL;*/
                hardware->zValue = context->scissorZ - POSITION_Z_INTERVAL;
            }
#endif
        }
        else
        {
            hardware->stencilMode       = gcvSTENCIL_NONE;
            hardware->depthMode         = gcvDEPTH_NONE;
        }

        gcmERR_BREAK(vgshHARDWARE_RunPipe(hardware));
    }
    while(gcvFALSE);

    gcmFOOTER();
    return status;
}

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
    gctUINT32       flag)
{
    gceSTATUS status = gcvSTATUS_OK;

    gctUINT8    mask        = flag & vgvBLIT_COLORMASK_ALL;
    gctBOOL     scissoring  = flag & vgvBLIT_SCISSORING;
    _VGImage    tempImage;

    _VGImageCtor(context->os, &tempImage);

    if (scissoring)
    {
        gcmONERROR(vgshUpdateScissor(context));
    }

    if (!vgshGetIntersectArea(
        &dx, &dy, &sx, &sy, &width, &height,
        dstImage->width, dstImage->height,
        srcImage->width, srcImage->height))
    {
        status= gcvSTATUS_FALSE;
        goto OnError;
    }

    if (flag & vgvBLIT_FROMDATA)
    {
        _VGColorDesc    colorDesc;
        gcmASSERT(mask == vgvBLIT_COLORMASK_ALL);

        vgshGetFormatColorDesc(
            srcImage->internalColorDesc.format, &colorDesc);

        gcmONERROR(vgshIMAGE_Initialize(
            context, &tempImage,
            &colorDesc, width, height,
            gcvORIENTATION_BOTTOM_TOP));

        gcmONERROR(vgshIMAGE_UploadData(
            context, &tempImage,
            srcImage->data,
            srcImage->stride,
            srcImage->internalColorDesc.format,
            0, 0, sx, sy, width, height));

        gcmONERROR(_Blit(context,
            dstImage, &tempImage, dx, dy, 0, 0, width, height,
            scissoring, mask));

    }
    else if (flag & vgvBLIT_TODATA)
    {
        _VGColorDesc    colorDesc;

        gcmASSERT(mask == vgvBLIT_COLORMASK_ALL);
        gcmASSERT(!scissoring);

        vgshGetFormatColorDesc(
            dstImage->internalColorDesc.format, &colorDesc);

        gcmONERROR(vgshIMAGE_Initialize(
            context, &tempImage,
            &colorDesc, width, height,
            gcvORIENTATION_BOTTOM_TOP));

        gcmONERROR(_Blit(context,
            &tempImage, srcImage, 0, 0, sx, sy, width, height,
            scissoring, mask));

        gcmONERROR(vgshIMAGE_GetData(
            context, &tempImage,
            dstImage->data, dstImage->stride,
            dstImage->internalColorDesc.format,
            dx, dy, 0, 0, width, height));
    }
    else
    {
        if (IsOverlap(dstImage, srcImage))
        {
            gcmONERROR(vgshIMAGE_Initialize(
                context, &tempImage,
                &srcImage->internalColorDesc, width, height,
                gcvORIENTATION_BOTTOM_TOP));

            gcmONERROR(_Blit(
                context, &tempImage, srcImage,
                0, 0, sx, sy, width, height,
                gcvFALSE, vgvBLIT_COLORMASK_ALL));


            gcmONERROR(_Blit(
                context, dstImage, &tempImage,
                dx, dy, 0, 0, width, height,
                scissoring, mask));

        }
        else
        {
            gcmONERROR(_Blit(
                context, dstImage, srcImage,
                dx, dy, sx, sy, width, height,
                scissoring, mask));
        }
    }

OnError:
    _VGImageDtor(context->os, &tempImage);

    return status;
}

void  vgWritePixels(const void * data, VGint dataStride, VGImageFormat dataFormat, VGint dx, VGint dy, VGint width, VGint height)
{

    _VGContext* context;
    _VGImage    tempImage;
    gctUINT32   flag;
    gcmHEADER_ARG("data=0x%x dataStride=%d dataFormat=%d dx=%d dy=%d width=%d height=%d",
                  data, dataStride, dataFormat, dx, dy, width, height);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGWRITEPIXELS, 0);

    OVG_IF_ERROR(!isValidImageFormat(dataFormat), VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!data || !isImageAligned(data, dataFormat) || width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);


    _VGImageCtor(context->os, &tempImage);

    gcmVERIFY_OK(vgshIMAGE_WrapFromData(
        context,
        &tempImage, data, dataStride, dataFormat,
        width, height));

    flag = vgshIsScissoringEnabled(context) ? vgvBLIT_SCISSORING : 0;
    flag |= vgvBLIT_COLORMASK_ALL | vgvBLIT_FROMDATA;

    gcmVERIFY_OK(vgshIMAGE_Blit(
        context, &context->targetImage, &tempImage,
        dx, dy, 0, 0, width, height,
        flag));

    _VGImageDtor(context->os, &tempImage);


    gcmFOOTER_NO();
}


void  vgGetPixels(VGImage dst, VGint dx, VGint dy, VGint sx, VGint sy, VGint width, VGint height)
{
    _VGImage        *imageDst;
    _VGContext      *context;

    gcmHEADER_ARG("dst=%d dx=%d dy=%d sx=%d sy=%d width=%d height=%d",
                  dst, dx, dy, sx, sy, width, height);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGGETPIXELS, 0);

    imageDst = _VGIMAGE(context, dst);

    OVG_IF_ERROR(!imageDst, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageDst), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    gcmVERIFY_OK(vgshIMAGE_Blit(
        context, imageDst,
        &context->targetImage,
        dx, dy, sx, sy, width, height,
        vgvBLIT_COLORMASK_ALL));

    gcmFOOTER_NO();
}


void  vgReadPixels(void* data, VGint dataStride, VGImageFormat dataFormat, VGint sx, VGint sy, VGint width, VGint height)
{
    _VGContext      * context;
    _VGImage        tempImage;
    gcmHEADER_ARG("data=0x%x dataStride=%d dataFormat=%d sx=%d sy=%d width=%d height=%d",
                  data, dataStride, dataFormat, sx, sy, width, height);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGREADPIXELS, 0);

    OVG_IF_ERROR(!isValidImageFormat(dataFormat), VG_UNSUPPORTED_IMAGE_FORMAT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!data || !isImageAligned(data, dataFormat) || width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);


    _VGImageCtor(context->os, &tempImage);

    gcmVERIFY_OK(vgshIMAGE_WrapFromData(
        context,
        &tempImage,
        data, dataStride, dataFormat,
        width, height));

    gcmVERIFY_OK(vgshIMAGE_Blit(
        context, &tempImage, &context->targetImage,
        0, 0, sx, sy, width, height, vgvBLIT_COLORMASK_ALL| vgvBLIT_TODATA));

    _VGImageDtor(context->os, &tempImage);

    gcmFOOTER_NO();
}


void  vgCopyPixels(VGint dx, VGint dy, VGint sx, VGint sy, VGint width, VGint height)
{
    _VGContext* context;
    gctUINT32   flag;
    gcmHEADER_ARG("dx=%d dy=%d sx=%d sy=%d width=%d height=%d",
                  dx, dy, sx, sy, width, height);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGCOPYPIXELS, 0);

    OVG_IF_ERROR(width <= 0 || height <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    flag = vgshIsScissoringEnabled(context) ? vgvBLIT_SCISSORING : 0;
    flag |= vgvBLIT_COLORMASK_ALL;

    gcmVERIFY_OK(vgshIMAGE_Blit(
        context, &context->targetImage, &context->targetImage,
        dx, dy, sx, sy, width, height,
        flag));


    gcmFOOTER_NO();
}

void GetTexBound(_VGImage *image, VGfloat *texBound)
{
    gctINT32  offsetX, offsetY, anWidth, anHeight;

    gcmHEADER_ARG("image=0x%x texBound=0x%x", image, texBound);

    GetAncestorOffset(image, &offsetX, &offsetY);
    GetAncestorSize(image, &anWidth, &anHeight);

    texBound[0] = (gctFLOAT)(offsetX)           / anWidth;
    texBound[1] = (gctFLOAT)(offsetY)           / anHeight;
    texBound[2] = (gctFLOAT)(offsetX + image->width)    / anWidth;
    texBound[3] = (gctFLOAT)(offsetY + image->height)   / anHeight;

    /*vec2 texBoundEx*/
    texBound[4] = (gctFLOAT)(offsetX + image->width - 1)    / anWidth;
    texBound[5] = (gctFLOAT)(offsetY + image->height - 1)   / anHeight;


    /*vec4 muv*/
    texBound[6] = texBound[2] - texBound[0];
    texBound[7] = texBound[3] - texBound[1];
    texBound[8] = 2 * texBound[6];
    texBound[9] = 2 * texBound[7];

    gcmFOOTER_NO();
}


void ConvertImage(_VGContext *context, _VGImage *image, _VGColorFormat format)
{
    _VGImage dstImage;

    gcmHEADER_ARG("context=0x%x image=0x%x format=%d",
                   context, image, format);

    /*TODO:now only for the filter function*/
    if (context->filterChannelMask == (VG_ALPHA  |VG_RED | VG_GREEN | VG_BLUE))
    {
        gcmFOOTER_NO();
        return;
    }

    if (image->internalColorDesc.colorFormat == format)
    {
        gcmFOOTER_NO();
        return;
    }


    dstImage = *image;

    dstImage.internalColorDesc.colorFormat = format;

    gcmVERIFY_OK(vgshIMAGE_Blit(context,
                        &dstImage,
                        image,
                        0, 0, 0, 0, image->width, image->height,
                        vgvBLIT_COLORMASK_ALL));

    image->internalColorDesc.colorFormat = format;

    gcmFOOTER_NO();
}

gctUINT8 _GetFilterChannel(_VGContext *context, _VGImage *image)
{
    gcmHEADER_ARG("context=0x%x image=0x%x", context, image);

    if (!(image->internalColorDesc.colorFormat & LUMINANCE))
    {
        gctUINT8 mask = 0;

        if (context->filterChannelMask & VG_ALPHA)
            mask |= 0x8;
        if (context->filterChannelMask & VG_BLUE)
            mask |= 0x4;
        if (context->filterChannelMask & VG_GREEN)
            mask |= 0x2;
        if (context->filterChannelMask & VG_RED)
            mask |= 0x1;

        gcmFOOTER_ARG("return=0x%x", mask);
        return mask;
    }
    else
    {
        gcmFOOTER_ARG("return=0x%x", 0xf);
        return 0xf;
    }
}

void  vgColorMatrix(VGImage dst, VGImage src, const VGfloat * matrix)
{
    _VGImage        *imageSrc;
    _VGImage        *imageDst;
    _VGColorFormat  procFormat, srcFormat, dstFormat, oldDstFormat;
    _VGContext* context;

    gcmHEADER_ARG("dst=%d src=%d matrix=0x%x", dst, src, matrix);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGCOLORMATRIX, 0);

    imageSrc = _VGIMAGE(context, src);
    imageDst = _VGIMAGE(context, dst);

    OVG_IF_ERROR(!imageSrc || !imageDst, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageDst) || eglIsInUse(imageSrc), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!matrix || !isAligned(matrix,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(IsOverlap(imageSrc, imageDst), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    /*now convert image to non-pre format to support the channelmask*/
    oldDstFormat = imageDst->internalColorDesc.colorFormat;
    ConvertImage(context, imageDst,  (_VGColorFormat) (oldDstFormat & ~PREMULTIPLIED));

    srcFormat = (_VGColorFormat) (imageSrc->internalColorDesc.colorFormat & ~COLOR_BITS_ALL);
    dstFormat = imageDst->internalColorDesc.colorFormat;
    procFormat = getProcessingFormat(srcFormat, context->filterFormatLinear, context->filterFormatPremultiplied);


    {
        _vgHARDWARE *hardware   = &context->hardware;

        hardware->srcImage      = imageSrc;
        hardware->dstImage      = imageDst;

        hardware->dx                = 0;
        hardware->dy                = 0;
        hardware->sx                = 0;
        hardware->sy                = 0;
        hardware->width             = gcmMIN(imageSrc->width, imageDst->width);
        hardware->height            = gcmMIN(imageSrc->height, imageDst->height);

        hardware->colorMatrix   = (gctFLOAT*)matrix;
        hardware->blending      = gcvFALSE;
        hardware->masking       = gcvFALSE;

        /*hardware->scissoring  = gcvFALSE;*/
        hardware->stencilMode   = gcvSTENCIL_NONE;
        hardware->depthMode     = gcvDEPTH_NONE;

        hardware->colorTransform    = gcvFALSE;
        hardware->depthCompare      = gcvCOMPARE_ALWAYS;
        hardware->depthWrite        = gcvFALSE;
        hardware->flush             = gcvTRUE;

        hardware->drawPipe          = vgvDRAWPIPE_FILTER;
        hardware->filterType        = vgvFILTER_COLOR_MATRIX;

        hardware->srcConvert        = getColorConvertValue(srcFormat, procFormat);
        hardware->dstConvert        = getColorConvertValue(procFormat, dstFormat);
        hardware->srcConvertAlpha   = getColorConvertAlphaValue(srcFormat, procFormat);
        hardware->dstConvertAlpha   = getColorConvertAlphaValue(procFormat, dstFormat);
        hardware->pack              = (gctUINT32)(dstFormat >> 16);
        hardware->alphaOnly         = (dstFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;
        hardware->colorWrite        = _GetFilterChannel(context, imageDst);

        gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));
    }

    ConvertImage(context, imageDst,  oldDstFormat);

    gcmFOOTER_NO();
}



void  vgConvolve(VGImage dst, VGImage src,
                 VGint kernelWidth, VGint kernelHeight,
                 VGint shiftX, VGint shiftY, const VGshort * kernel,
                 VGfloat scale, VGfloat bias, VGTilingMode tilingMode)
{
    _VGImage        *imageSrc;
    _VGImage        *imageDst;
    gctFLOAT      fKernel[((MAX_KERNEL_SIZE * MAX_KERNEL_SIZE + 3) / 4) * 4];
    gctFLOAT      texCoordOffset[(((MAX_KERNEL_SIZE * MAX_KERNEL_SIZE + 3) / 4) * 4) * 2];
    gctINT32             i, j, k;
    _VGColorFormat  procFormat, srcFormat, dstFormat, oldDstFormat;
    _VGContext* context;

    gcmHEADER_ARG("dst=%d src=%d kernelWidth=%d kernelHeight=%d shiftX=%d shiftY=%d kernel=0x%x"
                  "scale=%f bias=%f tillingMode=%d",
                  dst, src, kernelWidth, kernelHeight,
                  shiftX, shiftY, kernel, scale, bias, tilingMode);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGCONVOLVE, 0);

    imageSrc = _VGIMAGE(context, src);
    imageDst = _VGIMAGE(context, dst);

    OVG_IF_ERROR(!imageSrc || !imageDst, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageSrc) || eglIsInUse(imageDst), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(IsOverlap(imageSrc, imageDst), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!kernel || !isAligned(kernel,2) || kernelWidth <= 0 || kernelHeight <= 0 || kernelWidth > MAX_KERNEL_SIZE || kernelHeight > MAX_KERNEL_SIZE, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(tilingMode < VG_TILE_FILL || tilingMode > VG_TILE_REFLECT, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    gcoOS_ZeroMemory(fKernel, sizeof(fKernel));
    gcoOS_ZeroMemory(texCoordOffset, sizeof(texCoordOffset));

    k = 0;
    for (j = 0; j < kernelHeight ; j++)
    {
        for (i = 0; i < kernelWidth ; i++)
        {
            fKernel[k] = (gctFLOAT)(kernel[(kernelWidth - 1 - i) * kernelHeight + kernelHeight - 1 - j]);
            texCoordOffset[2 * k] = (i - shiftX) / (gctFLOAT)GetAncestorImage(imageSrc)->width;
            texCoordOffset[2 * k + 1] = (j - shiftY) / (gctFLOAT)GetAncestorImage(imageSrc)->height;
            k++;
        }
    }

    oldDstFormat = imageDst->internalColorDesc.colorFormat;
    ConvertImage(context, imageDst,  (_VGColorFormat) (oldDstFormat & ~PREMULTIPLIED));

    srcFormat = (_VGColorFormat) (imageSrc->internalColorDesc.colorFormat & ~COLOR_BITS_ALL);
    dstFormat = imageDst->internalColorDesc.colorFormat;

    procFormat = getProcessingFormat(srcFormat, context->filterFormatLinear, context->filterFormatPremultiplied);

    {
        _vgHARDWARE *hardware   = &context->hardware;

        hardware->drawPipe      = vgvDRAWPIPE_FILTER;
        hardware->filterType    = vgvFILTER_CONVOLVE;

        hardware->srcImage      = imageSrc;
        hardware->dstImage      = imageDst;

        hardware->dx                = 0;
        hardware->dy                = 0;
        hardware->sx                = 0;
        hardware->sy                = 0;
        hardware->width             = gcmMIN(imageSrc->width, imageDst->width);
        hardware->height            = gcmMIN(imageSrc->height, imageDst->height);

        hardware->blending      = gcvFALSE;
        hardware->masking       = gcvFALSE;

        /*hardware->scissoring  = gcvFALSE;*/
        hardware->stencilMode   = gcvSTENCIL_NONE;
        hardware->depthMode     = gcvDEPTH_NONE;

        hardware->colorTransform = gcvFALSE;
        hardware->depthCompare  = gcvCOMPARE_ALWAYS;
        hardware->depthWrite    = gcvFALSE;
        hardware->flush         = gcvTRUE;

        hardware->kernel                = fKernel;
        hardware->kernelSize            = (gctFLOAT)((kernelWidth * kernelHeight + 3) / 4);
        hardware->texCoordOffset        = texCoordOffset;
        hardware->texCoordOffsetSize    = 2 * context->hardware.kernelSize;

        gcmASSERT(hardware->kernelSize <= vgvMAX_CONVOLVE_KERNEL_SIZE);
        gcmASSERT(hardware->texCoordOffsetSize <= vgvMAX_CONVOLVE_COORD_SIZE);

        hardware->scale         = scale;
        hardware->bias          = bias;
        hardware->tilingMode    = tilingMode;

        hardware->srcConvert        = getColorConvertValue(srcFormat, procFormat);
        hardware->dstConvert        = getColorConvertValue(procFormat, dstFormat);
        hardware->srcConvertAlpha   = getColorConvertAlphaValue(srcFormat, procFormat);
        hardware->dstConvertAlpha   = getColorConvertAlphaValue(procFormat, dstFormat);
        hardware->pack              = (gctUINT32)(dstFormat >> 16);
        hardware->alphaOnly         = (dstFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;
        hardware->colorWrite        = _GetFilterChannel(context, imageDst);

        gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));

    }

    ConvertImage(context, imageDst,  oldDstFormat);

    gcmFOOTER_NO();
}



void  vgSeparableConvolve(VGImage dst, VGImage src, VGint kernelWidth, VGint kernelHeight,
                          VGint shiftX, VGint shiftY, const VGshort * kernelX, const VGshort * kernelY,
                          VGfloat scale, VGfloat bias, VGTilingMode tilingMode)
{
    _VGImage        *imageSrc;
    _VGImage        *imageDst;
    gctFLOAT      fKernelX[((MAX_SEPARABLE_KERNEL_SIZE + 1) / 2) * 2];
    gctFLOAT      fKernelY[((MAX_SEPARABLE_KERNEL_SIZE + 1) / 2) * 2];
    gctFLOAT      texCoordOffsetX[((MAX_SEPARABLE_KERNEL_SIZE + 1) / 2) * 2];
    gctFLOAT      texCoordOffsetY[((MAX_SEPARABLE_KERNEL_SIZE + 1) / 2) * 2];
    gctINT32             i;

    _VGColorFormat  procFormat, srcFormat, dstFormat, oldDstFormat;
    _VGContext* context;

    gcmHEADER_ARG("dst=%d src=%d kernelWidth=%d kernelHeight=%d shiftX=%d shiftY=%d "
                  "kernelX=0x%x kernelY=0x%x scale=%f bias=%f tillingMode=%d",
                  dst, src, kernelWidth, kernelHeight, shiftX, shiftY,
                  kernelX, kernelY, scale, bias, tilingMode);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGSEPARABLECONVOLVE, 0);

    imageSrc = _VGIMAGE(context, src);
    imageDst = _VGIMAGE(context, dst);

    OVG_IF_ERROR(!imageSrc || !imageDst, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageSrc) || eglIsInUse(imageDst), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(IsOverlap(imageSrc, imageDst), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!kernelX || !kernelY || !isAligned(kernelX,2) || !isAligned(kernelY,2) || kernelWidth <= 0 || kernelHeight <= 0 || kernelWidth > MAX_SEPARABLE_KERNEL_SIZE || kernelHeight > MAX_SEPARABLE_KERNEL_SIZE, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(tilingMode < VG_TILE_FILL || tilingMode > VG_TILE_REFLECT, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    gcoOS_ZeroMemory(fKernelX, sizeof(fKernelX));
    gcoOS_ZeroMemory(texCoordOffsetX, sizeof(texCoordOffsetX));
    gcoOS_ZeroMemory(fKernelY, sizeof(fKernelY));
    gcoOS_ZeroMemory(texCoordOffsetY, sizeof(texCoordOffsetY));

    for (i = 0; i < kernelWidth ; i++)
    {

        fKernelX[i] = (gctFLOAT)(kernelX[kernelWidth - 1 - i]);
        texCoordOffsetX[i] = (i - shiftX) / (gctFLOAT)GetAncestorImage(imageSrc)->width;
    }


    for (i = 0; i < kernelHeight ; i++)
    {
        fKernelY[i] = (gctFLOAT)(kernelY[ kernelHeight - 1 - i]);
        texCoordOffsetY[i] = (i - shiftY) / (gctFLOAT)GetAncestorImage(imageSrc)->height;
    }


    oldDstFormat = imageDst->internalColorDesc.colorFormat;
    ConvertImage(context, imageDst,  (_VGColorFormat) (oldDstFormat & ~PREMULTIPLIED));


    srcFormat = (_VGColorFormat) (imageSrc->internalColorDesc.colorFormat & ~COLOR_BITS_ALL);
    dstFormat = imageDst->internalColorDesc.colorFormat;
    procFormat = getProcessingFormat(srcFormat, context->filterFormatLinear, context->filterFormatPremultiplied);

    {
        _vgHARDWARE *hardware   = &context->hardware;

        /* horizontal pass */
        hardware->drawPipe      = vgvDRAWPIPE_FILTER;
        hardware->filterType    = vgvFILTER_SEPCONVOLVE;

        hardware->srcImage      = imageSrc;
        hardware->dstImage      = imageDst;
        hardware->dx                = 0;
        hardware->dy                = 0;
        hardware->sx                = 0;
        hardware->sy                = 0;
        hardware->width             = gcmMIN(imageSrc->width, imageDst->width);
        hardware->height            = gcmMIN(imageSrc->height, imageDst->height);


        hardware->blending      = gcvFALSE;
        hardware->masking       = gcvFALSE;
        /*hardware.scissoring   = gcvFALSE;*/
        hardware->stencilMode   = gcvSTENCIL_NONE;
        hardware->depthMode     = gcvDEPTH_NONE;

        hardware->colorTransform    = gcvFALSE;
        hardware->depthCompare      = gcvCOMPARE_ALWAYS;
        hardware->depthWrite        = gcvFALSE;
        hardware->flush             = gcvTRUE;

        hardware->kernel                = fKernelX;
        hardware->kernelSize            = (gctFLOAT)((kernelWidth + 1) / 2);
        hardware->texCoordOffset        = texCoordOffsetX;
        hardware->texCoordOffsetSize    = context->hardware.kernelSize;

        hardware->kernelY               = fKernelY;
        hardware->kernelSizeY           = (gctFLOAT)((kernelHeight + 1) / 2);
        hardware->texCoordOffsetY       = texCoordOffsetY;
        hardware->texCoordOffsetSizeY   = context->hardware.kernelSizeY;

        gcmASSERT(hardware->kernelSize          <= vgvMAX_SEPCONVOLVE_KERNEL_SIZE);
        gcmASSERT(hardware->texCoordOffsetSize  <= vgvMAX_SEPCONVOLVE_COORD_SIZE);
        gcmASSERT(hardware->kernelSizeY         <= vgvMAX_SEPCONVOLVE_KERNEL_SIZE);
        gcmASSERT(hardware->texCoordOffsetSizeY <= vgvMAX_SEPCONVOLVE_COORD_SIZE);


        hardware->scale             = scale;
        hardware->bias              = bias;
        hardware->tilingMode        = tilingMode;
        hardware->srcConvert        = getColorConvertValue(srcFormat, procFormat);
        hardware->dstConvert        = getColorConvertValue(procFormat, dstFormat);
        hardware->srcConvertAlpha   = getColorConvertAlphaValue(srcFormat, procFormat);
        hardware->dstConvertAlpha   = getColorConvertAlphaValue(procFormat, dstFormat);
        hardware->pack              = (gctUINT32)(dstFormat >> 16);
        hardware->alphaOnly         = (dstFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;
        hardware->colorWrite        = _GetFilterChannel(context, imageDst);

        gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));
    }

    ConvertImage(context, imageDst,  oldDstFormat);

    gcmFOOTER_NO();
}



#define MAX_GAS_KERNEL_SIZE 64

void  vgGaussianBlur(VGImage dst, VGImage src, VGfloat stdDeviationX, VGfloat stdDeviationY, VGTilingMode tilingMode)
{
    _VGImage        *imageSrc;
    _VGImage        *imageDst;
    gctFLOAT        sx, sy;
    _VGContext* context;

    gcmHEADER_ARG("dst=%d src=%d stdDeviationX=%f stdDeviationY=%f tillingMode=%d",
                  dst, src, stdDeviationX, stdDeviationY, tilingMode);
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGGAUSSIANBLUR, 0);

    imageSrc = _VGIMAGE(context, src);
    imageDst = _VGIMAGE(context, dst);

    OVG_IF_ERROR(!imageSrc || !imageDst, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(eglIsInUse(imageDst) || eglIsInUse(imageSrc), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(IsOverlap(imageSrc, imageDst), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    sx = inputFloat(stdDeviationX);
    sy = inputFloat(stdDeviationY);
    OVG_IF_ERROR(sx <= 0.0f || sy <= 0.0f || sx > (gctFLOAT)MAX_GAUSSIAN_STD_DEVIATION || sy > (gctFLOAT)MAX_GAUSSIAN_STD_DEVIATION, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(tilingMode < VG_TILE_FILL || tilingMode > VG_TILE_REFLECT, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    {

        gctFLOAT      expScaleX, expScaleY;

        gctFLOAT      fKernelX[MAX_GAS_KERNEL_SIZE];
        gctFLOAT      fKernelY[MAX_GAS_KERNEL_SIZE];
        gctFLOAT      texCoordOffsetX[4];
        gctFLOAT      texCoordOffsetY[4];

        gctINT32        i;
        gctFLOAT        scaleX = 0.0f, scaleY = 0.0f;
        gctINT32        shiftX, shiftY;
        _VGImage        imageTemp;

        _VGColorFormat procFormat;
        _VGColorFormat srcFormat, dstFormat, oldDstFormat;

        gctINT32    pass = -1;
        gctBOOL     optimization    = gcvFALSE;
        _vgHARDWARE *hardware       = &context->hardware;

        gcoOS_ZeroMemory(fKernelX, sizeof(fKernelX));
        gcoOS_ZeroMemory(fKernelY, sizeof(fKernelY));

#if WALK_MG1
        hardware->gdx = stdDeviationX;
        hardware->gdy = stdDeviationY;
#endif

        expScaleX = -1.0f / (2.0f * sx * sx);
        expScaleY = -1.0f / (2.0f * sy * sy);


        /*TODO:do better*/
        if (context->targetImage.height > 64 || context->targetImage.width > 64)
        {
            if (sx < 6.0f && sx > 2.0f)
            {
                shiftX = 4;
            }
            else
            {
                shiftX = (gctINT32)(sx * 2.0f);
            }


            if (sy < 6.0f && sy > 2.0f)
            {
                shiftY = 4;
            }
            else
            {
                shiftY = (gctINT32)(sy * 2.0f);
            }
        }
        else
        {
            shiftX = (gctINT32)(sx * 4.0f);
            shiftY = (gctINT32)(sy * 4.0f);
        }

        gcmASSERT(shiftX <= MAX_GAS_KERNEL_SIZE);
        gcmASSERT(shiftY <= MAX_GAS_KERNEL_SIZE);


        for(i = 0;i < shiftY; i++)
        {
            gctINT32 y = i + 1;
            fKernelY[i] = (gctFLOAT)gcoMATH_Exp((gctFLOAT)y*(gctFLOAT)y * expScaleY);
            scaleY      += fKernelY[i];
        }

        for (i = 0; i < shiftY; i++)
        {
            fKernelY[i] /= (gctFLOAT)(2.0f * scaleY + 1.0f);
        }


        for(i = 0; i < shiftX; i++)
        {
            gctINT32 x = i + 1;
            fKernelX[i] = (gctFLOAT)gcoMATH_Exp((gctFLOAT)x*(gctFLOAT)x * expScaleX);
            scaleX      += fKernelX[i];
        }

        for (i = 0; i <  shiftX; i++)
        {
            fKernelX[i] /= (gctFLOAT)(2.0f * scaleX + 1.0f);
        }

        texCoordOffsetX[0] = (gctFLOAT) 1.0f / GetAncestorImage(imageSrc)->width;
        texCoordOffsetX[1] = 0.0f;


        oldDstFormat = imageDst->internalColorDesc.colorFormat;
        ConvertImage(context, imageDst,  (_VGColorFormat) (oldDstFormat & ~PREMULTIPLIED));


        srcFormat = (_VGColorFormat) (imageSrc->internalColorDesc.colorFormat & ~COLOR_BITS_ALL);
        dstFormat = imageDst->internalColorDesc.colorFormat;
        procFormat = getProcessingFormat(srcFormat, context->filterFormatLinear, context->filterFormatPremultiplied);


        if (context->targetImage.height <= 64 && context->targetImage.width <= 64)
        {
            pass = 2;
        }
        else
        {
            /* special hack */
            if ((tilingMode == VG_TILE_FILL) &&
                (context->tileFillColor.a == 0.0f) &&
                (context->tileFillColor.r == 0.0f) &&
                (context->tileFillColor.g == 0.0f) &&
                (context->tileFillColor.b == 0.0f) && (shiftX <= 4) && (shiftY <= 4))
            {
                optimization = gcvTRUE;
            }

            if (shiftX == 0 && shiftY == 0)
            {
                pass = 0;
            }
            else if (shiftX == 0 || shiftY == 0)
            {
                pass = 1;
            }
            else
            {
                pass = 2;
            }
        }


        if (optimization)
        {
            /*turn on the special optimization */
            hardware->disableClamp      = gcvTRUE;
            hardware->disableTiling     = gcvTRUE;
            hardware->gaussianOPT       = gcvTRUE;
        }

        if (pass == 2)
        {
            _VGColor tileColor;
            _VGColorDesc colorDesc;

            _VGImageCtor(context->os, &imageTemp);

            vgshGetFormatColorDesc(VG_sRGBA_8888, &colorDesc);

            gcmVERIFY_OK(vgshIMAGE_Initialize(
                context,
                &imageTemp,
                &colorDesc,
                imageSrc->width, imageSrc->height,
                gcvORIENTATION_BOTTOM_TOP));

            texCoordOffsetX[0] = (gctFLOAT) 1.0f / GetAncestorImage(imageSrc)->width;
            texCoordOffsetX[1] = (gctFLOAT) 0.0f;

            texCoordOffsetY[0] = (gctFLOAT)0.0f;
            texCoordOffsetY[1] = (gctFLOAT) 1.0f / imageSrc->height;

            /* horizontal pass */
            hardware->drawPipe      = vgvDRAWPIPE_FILTER;
            hardware->filterType    = vgvFILTER_GAUSSIAN;

            hardware->srcImage      = imageSrc;
            hardware->dstImage      = &imageTemp;
            hardware->dx                = 0;
            hardware->dy                = 0;
            hardware->sx                = 0;
            hardware->sy                = 0;
            hardware->width             = imageSrc->width;
            hardware->height            = imageSrc->height;

            hardware->blending      = gcvFALSE;
            hardware->masking       = gcvFALSE;
            /*hardware.scissoring   = gcvFALSE;*/
            hardware->stencilMode   = gcvSTENCIL_NONE;
            hardware->depthMode             = gcvDEPTH_NONE;

            hardware->colorTransform = gcvFALSE;
            hardware->depthCompare  = gcvCOMPARE_ALWAYS;
            hardware->depthWrite    = gcvFALSE;
            hardware->flush         = gcvTRUE;

            hardware->kernel                = fKernelX;
            hardware->kernelSize            = (gctFLOAT)((shiftX + 3)/4);
            hardware->texCoordOffset        = texCoordOffsetX;
            hardware->texCoordOffsetSize    = 1;

            gcmASSERT(hardware->kernelSize          <= vgvMAX_GAUSSIAN_KERNEL_SIZE);
            gcmASSERT(hardware->texCoordOffsetSize  <= vgvMAX_GAUSSIAN_COORD_SIZE);

            hardware->tilingMode    = tilingMode;
            hardware->kernel0       = (gctFLOAT)(1.0f/(2.0f * scaleX + 1.0f));

            hardware->srcConvert        = getColorConvertValue(srcFormat, procFormat);
            hardware->dstConvert        = getColorConvertValue(procFormat, procFormat);
            hardware->srcConvertAlpha   = getColorConvertAlphaValue(srcFormat, procFormat);
            hardware->dstConvertAlpha   = getColorConvertAlphaValue(procFormat, procFormat);
            hardware->pack              = (gctUINT32)(procFormat >> 16);
            hardware->alphaOnly         = (procFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;
            hardware->colorWrite        = 0xf;

            gcmVERIFY_OK(vgshHARDWARE_RunPipe(&context->hardware));

            /* vertical pass */
            tileColor = context->tileFillColor;
            /*between the 2 passes, the tile color needs to be converted.*/
            ConvertColor(&context->tileFillColor, procFormat);

            hardware->srcImage      = &imageTemp;
            hardware->dstImage      = imageDst;

            hardware->width             = gcmMIN(imageSrc->width, imageDst->width);
            hardware->height            = gcmMIN(imageSrc->height, imageDst->height);

            hardware->kernel                = fKernelY;
            hardware->kernelSize            = (gctFLOAT)((shiftY + 3) / 4);
            hardware->texCoordOffset        = texCoordOffsetY;
            hardware->texCoordOffsetSize    = 1;

            gcmASSERT(hardware->kernelSize          <= vgvMAX_GAUSSIAN_KERNEL_SIZE);
            gcmASSERT(hardware->texCoordOffsetSize  <= vgvMAX_GAUSSIAN_COORD_SIZE);

            hardware->kernel0       = (gctFLOAT)(1.0f/(2.0f * scaleY + 1.0f));

            hardware->srcConvert        = getColorConvertValue(procFormat, procFormat);
            hardware->dstConvert        = getColorConvertValue(procFormat, dstFormat);
            hardware->srcConvertAlpha   = getColorConvertAlphaValue(procFormat, procFormat);
            hardware->dstConvertAlpha   = getColorConvertAlphaValue(procFormat, dstFormat);
            hardware->pack              = (gctUINT32)(dstFormat >> 16);
            hardware->alphaOnly         = (dstFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;
            hardware->colorWrite        = _GetFilterChannel(context, imageDst);

            gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));

            context->tileFillColor = tileColor;
            /* free the temp image */
            _VGImageDtor(context->os, &imageTemp);
        }
        else if (pass == 1)
        {
            texCoordOffsetX[0] = (gctFLOAT) 1.0f / GetAncestorImage(imageSrc)->width;
            texCoordOffsetX[1] = (gctFLOAT) 0.0f;

            texCoordOffsetY[0] = (gctFLOAT)0.0f;
            texCoordOffsetY[1] = (gctFLOAT) 1.0f / GetAncestorImage(imageSrc)->height;


            hardware->drawPipe      = vgvDRAWPIPE_FILTER;
            hardware->filterType    = vgvFILTER_GAUSSIAN;

            hardware->srcImage      = imageSrc;
            hardware->dstImage      = imageDst;
            hardware->dx                = 0;
            hardware->dy                = 0;
            hardware->sx                = 0;
            hardware->sy                = 0;
            hardware->width             = gcmMIN(imageSrc->width, imageDst->width);
            hardware->height            = gcmMIN(imageSrc->height, imageDst->height);


            hardware->blending      = gcvFALSE;
            hardware->masking       = gcvFALSE;
            /*hardware->scissoring  = gcvFALSE;*/
            hardware->stencilMode   = gcvSTENCIL_NONE;

            hardware->colorTransform = gcvFALSE;
            hardware->depthCompare  = gcvCOMPARE_ALWAYS;
            hardware->depthWrite    = gcvFALSE;
            hardware->flush         = gcvTRUE;
            hardware->tilingMode    = tilingMode;

            if (shiftY == 0)
            {
                gcmASSERT(shiftX != 0);

                hardware->kernel                = fKernelX;
                hardware->kernelSize            = (gctFLOAT)((shiftX + 3)/4);
                hardware->texCoordOffset        = texCoordOffsetX;
                hardware->texCoordOffsetSize    = 1;
                hardware->kernel0               = (gctFLOAT)(1.0f/(2.0f * scaleX + 1.0f));

                gcmASSERT(hardware->kernelSize          <= vgvMAX_GAUSSIAN_KERNEL_SIZE);
                gcmASSERT(hardware->texCoordOffsetSize  <= vgvMAX_GAUSSIAN_COORD_SIZE);

            }
            else
            {
                gcmASSERT(shiftX == 0);

                hardware->kernel                = fKernelY;
                hardware->kernelSize            = (gctFLOAT)((shiftY + 3) / 4);
                hardware->texCoordOffset        = texCoordOffsetY;
                hardware->texCoordOffsetSize    = 1;
                hardware->kernel0               = (gctFLOAT)(1.0f/(2.0f * scaleY + 1.0f));


                gcmASSERT(hardware->kernelSize          <= vgvMAX_GAUSSIAN_KERNEL_SIZE);
                gcmASSERT(hardware->texCoordOffsetSize  <= vgvMAX_GAUSSIAN_COORD_SIZE);

            }

            hardware->srcConvert        = getColorConvertValue(srcFormat, dstFormat);
            hardware->dstConvert        = getColorConvertValue(srcFormat, dstFormat);
            hardware->srcConvertAlpha   = getColorConvertAlphaValue(srcFormat, dstFormat);
            hardware->dstConvertAlpha   = getColorConvertAlphaValue(srcFormat, dstFormat);
            hardware->pack              = (gctUINT32)(dstFormat >> 16);
            hardware->alphaOnly         = (dstFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;
            hardware->colorWrite        = _GetFilterChannel(context, imageDst);

            gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));
        }
        else if (pass == 0)
        {

            gcmVERIFY_OK(vgshIMAGE_Blit(context,
                                imageDst,
                                imageSrc,
                                0, 0, 0, 0,
                                imageSrc->width, imageSrc->height,
                                _GetFilterChannel(context, imageDst)));
        }
        else
        {
            gcmASSERT(0);
        }

        /* restore the default */
        if (optimization)
        {
            hardware->disableClamp      = gcvFALSE;
            hardware->disableTiling     = gcvFALSE;
            hardware->gaussianOPT       = gcvFALSE;
        }

        ConvertImage(context, imageDst,  oldDstFormat);

    }

    gcmFOOTER_NO();
}

/*Returns lookup table format for lookup filters*/
static _VGColorFormat getLUTFormat(VGboolean outputLinear, VGboolean outputPremultiplied)
{
    _VGColorFormat lutFormat = lRGBA;

    gcmHEADER_ARG("outputLinear=%s outputPremultiplied=%s",
        outputLinear ? "TRUE" : "FALSE",
        outputPremultiplied ? "TRUE" : "FALSE");

	if (outputLinear)
	{
		if (outputPremultiplied)
		{
			lutFormat = lRGBA_PRE;
		}
	}
	else
	{
		if (outputPremultiplied)
		{
	        lutFormat = sRGBA_PRE;
		}
		else
		{
	        lutFormat = sRGBA;
		}
	}

    gcmFOOTER_ARG("return=%d", lutFormat);

    return lutFormat;
}

void  vgLookup(VGImage dst, VGImage src, const VGubyte * redLUT, const VGubyte * greenLUT, const VGubyte * blueLUT,
               const VGubyte * alphaLUT, VGboolean outputLinear, VGboolean outputPremultiplied)
{
    _VGImage    *imageSrc, *imageDst;
    gcoTEXTURE  texture;
    _VGImage    lutImage;
    VGuint     *lutTex;
    gctINT32    i;

    _VGColorFormat  procFormat, srcFormat, lutFormat, dstFormat, oldDstFormat;

    gcoSURF         surface= gcvNULL;
    _VGContext      *context;

    gcmHEADER_ARG("dst=%d src=%d redLUT=0x%x greenLUT=0x%x blueLUT=0x%x alphaLUT=0x%x "
                  "outputLinear=%s outputPremultiplied=%s",
                  dst, src, redLUT, greenLUT, blueLUT, alphaLUT,
                  outputLinear ? "VG_TRUE" : "VG_FALSE",
                  outputPremultiplied ? "VG_TRUE" : "VG_FALSE");
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGLOOKUP, 0);

    imageSrc = _VGIMAGE(context, src);
    imageDst = _VGIMAGE(context, dst);

    OVG_IF_ERROR(!imageSrc || !imageDst, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

    OVG_IF_ERROR(eglIsInUse(imageDst) || eglIsInUse(imageSrc), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(IsOverlap(imageSrc, imageDst), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!redLUT || !greenLUT || !blueLUT || !alphaLUT, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    OVG_MALLOC(context->os, lutTex, 256 * sizeof(VGuint));

    if (!lutTex)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        gcmFOOTER_NO();
        return;
    }

    gcmVERIFY_OK(vgshCreateTexture(context, 256, 1, gcvSURF_A8R8G8B8, &texture, &surface));
	if (surface == gcvNULL)
	{
		OVG_FREE(context->os, lutTex);
        gcmFOOTER_NO();
        return;
	}

    _VGImageCtor(context->os, &lutImage);

    lutImage.texture = texture;

    for(i = 0; i < 256; ++i)
    {
        lutTex[i] = ARGB((alphaLUT[i]), (redLUT[i]) , (greenLUT[i]) , (blueLUT[i]) );
    }

    gcmVERIFY_OK(gcoTEXTURE_UploadSub(texture,
                                        0,
                                        gcvFACE_NONE,
                                        0, 0, 256, 1,
                                        0,
                                        lutTex,
                                        256 * sizeof(VGuint),
                                        gcvSURF_A8R8G8B8));

    oldDstFormat = imageDst->internalColorDesc.colorFormat;
    ConvertImage(context, imageDst,  (_VGColorFormat) (oldDstFormat & ~PREMULTIPLIED));


    srcFormat = (_VGColorFormat) (imageSrc->internalColorDesc.colorFormat & ~COLOR_BITS_ALL);
    dstFormat = imageDst->internalColorDesc.colorFormat;

    procFormat = getProcessingFormat(srcFormat, context->filterFormatLinear, context->filterFormatPremultiplied);
    lutFormat = getLUTFormat(outputLinear, outputPremultiplied);

    {
        _vgHARDWARE *hardware   = &context->hardware;

        hardware->drawPipe      = vgvDRAWPIPE_FILTER;
        hardware->filterType    = vgvFILTER_LOOKUP;

        hardware->srcImage      = imageSrc;
        hardware->dstImage      = imageDst;
        hardware->dx                = 0;
        hardware->dy                = 0;
        hardware->sx                = 0;
        hardware->sy                = 0;
        hardware->width             = gcmMIN(imageSrc->width, imageDst->width);
        hardware->height            = gcmMIN(imageSrc->height, imageDst->height);


        hardware->blending      = gcvFALSE;
        hardware->masking       = gcvFALSE;

        /*hardware->scissoring  = gcvFALSE;*/
        hardware->stencilMode   = gcvSTENCIL_NONE;
        hardware->depthMode     = gcvDEPTH_NONE;

        hardware->colorTransform = gcvFALSE;
        hardware->depthCompare  = gcvCOMPARE_ALWAYS;
        hardware->depthWrite    = gcvFALSE;
        hardware->flush         = gcvTRUE;

        hardware->lutImage      = &lutImage;

        hardware->srcConvert        = getColorConvertValue(srcFormat, procFormat);
        hardware->dstConvert        = getColorConvertValue(lutFormat, dstFormat);
        hardware->srcConvertAlpha   = getColorConvertAlphaValue(srcFormat, procFormat);
        hardware->dstConvertAlpha   = getColorConvertAlphaValue(lutFormat, dstFormat);
        hardware->pack              = (gctUINT32)(dstFormat >> 16);
        hardware->alphaOnly         = (dstFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;
        hardware->colorWrite        = _GetFilterChannel(context, imageDst);

        gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));
    }

    ConvertImage(context, imageDst,  oldDstFormat);

    OVG_FREE(context->os, lutTex);
    gcmVERIFY_OK(gcoTEXTURE_Destroy(texture));

    gcmFOOTER_NO();
}

void  vgLookupSingle(VGImage dst, VGImage src, const VGuint * lookupTable, VGImageChannel sourceChannel,
                     VGboolean outputLinear, VGboolean outputPremultiplied)
{
    _VGImage     *imageSrc, *imageDst;
    _VGColorDesc colorDesc;
    gcoTEXTURE   texture;
    _VGImage     lutImage;
    VGuint       *lutTex;
    gctINT32     i;

    _VGColorFormat  procFormat, srcFormat, lutFormat, dstFormat, oldDstFormat;

    gcoSURF     surface = gcvNULL;

    _VGContext  *context;

    gcmHEADER_ARG("dst=%d src=%d lookupTable=0x%x sourceChannel=%d outputLinear=%s outputPremultiplied=%s",
                  dst, src, lookupTable, sourceChannel,
                  outputLinear ? "VG_TRUE" : "VG_FALSE",
                  outputPremultiplied ? "VG_TRUE" : "VG_FALSE");
    OVG_GET_CONTEXT(OVG_NO_RETVAL);

    vgmPROFILE(context, VGLOOKUPSINGLE, 0);

    imageSrc = _VGIMAGE(context, src);
    imageDst = _VGIMAGE(context, dst);

    OVG_IF_ERROR(!imageSrc || !imageDst, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);


    OVG_IF_ERROR(eglIsInUse(imageDst) || eglIsInUse(imageSrc), VG_IMAGE_IN_USE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(IsOverlap(imageSrc, imageDst), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!lookupTable || !isAligned(lookupTable,4), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    _GetRawFormatColorDesc(imageSrc->internalColorDesc.format, &colorDesc);

    OVG_IF_ERROR((!colorDesc.lbits && colorDesc.rbits + colorDesc.gbits + colorDesc.bbits) &&
                (sourceChannel != VG_RED && sourceChannel != VG_GREEN && sourceChannel != VG_BLUE && sourceChannel != VG_ALPHA),
                VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    if(colorDesc.lbits)
    {
        sourceChannel = VG_RED;
    }
    else if(colorDesc.rbits + colorDesc.gbits + colorDesc.bbits == 0)
    {
        gcmASSERT(colorDesc.abits);
        sourceChannel = VG_ALPHA;
    }

    OVG_MALLOC(context->os, lutTex, 256 * sizeof(VGuint));

    if (!lutTex)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        gcmFOOTER_NO();
        return;
    }

    gcmVERIFY_OK(vgshCreateTexture(context, 256, 1, gcvSURF_A8R8G8B8, &texture, &surface));
	if (surface == gcvNULL)
	{
		OVG_FREE(context->os, lutTex);
		gcmFOOTER_NO();
		return;
	}

    _VGImageCtor(context->os, &lutImage);

    lutImage.texture = texture;

    for(i = 0; i < 256; ++i)
    {
        lutTex[i] = R8G8B8A8_TO_A8R8G8B8(lookupTable[i]) ;
    }

    gcmVERIFY_OK(gcoTEXTURE_UploadSub(texture,
                                        0,
                                        gcvFACE_NONE,
                                        0, 0, 256, 1,
                                        0,
                                        lutTex,
                                        256 * sizeof(VGuint),
                                        gcvSURF_A8R8G8B8));

    oldDstFormat = imageDst->internalColorDesc.colorFormat;
    ConvertImage(context, imageDst,  (_VGColorFormat) (oldDstFormat & ~PREMULTIPLIED));


    srcFormat = (_VGColorFormat) (imageSrc->internalColorDesc.colorFormat & ~COLOR_BITS_ALL);
    dstFormat = imageDst->internalColorDesc.colorFormat;

    procFormat = getProcessingFormat(srcFormat, context->filterFormatLinear, context->filterFormatPremultiplied);
    lutFormat = getLUTFormat(outputLinear, outputPremultiplied);

    {
        _vgHARDWARE *hardware = &context->hardware;

        hardware->drawPipe      = vgvDRAWPIPE_FILTER;
        hardware->filterType    = vgvFILTER_LOOKUP_SINGLE;

        hardware->srcImage      = imageSrc;
        hardware->dstImage      = imageDst;
        hardware->dx                = 0;
        hardware->dy                = 0;
        hardware->sx                = 0;
        hardware->sy                = 0;
        hardware->width             = gcmMIN(imageSrc->width, imageDst->width);
        hardware->height            = gcmMIN(imageSrc->height, imageDst->height);


        hardware->blending      = gcvFALSE;
        hardware->masking       = gcvFALSE;
        /*hardware.scissoring   = gcvFALSE;*/
        hardware->stencilMode   = gcvSTENCIL_NONE;
        hardware->depthMode     = gcvDEPTH_NONE;

        hardware->colorTransform = gcvFALSE;
        hardware->depthCompare  = gcvCOMPARE_ALWAYS;
        hardware->depthWrite    = gcvFALSE;
        hardware->flush         = gcvTRUE;

        hardware->lutImage      = &lutImage;
        hardware->sourceChannel = sourceChannel;

        hardware->srcConvert        = getColorConvertValue(srcFormat, procFormat);
        hardware->dstConvert        = getColorConvertValue(lutFormat, dstFormat);
        hardware->srcConvertAlpha   = getColorConvertAlphaValue(srcFormat, procFormat);
        hardware->dstConvertAlpha   = getColorConvertAlphaValue(lutFormat, dstFormat);
        hardware->pack              = (gctUINT32)(dstFormat >> 16);
        hardware->alphaOnly         = (dstFormat & ALPHAONLY) ? gcvTRUE : gcvFALSE;
        hardware->colorWrite        = _GetFilterChannel(context, imageDst);

        gcmVERIFY_OK(vgshHARDWARE_RunPipe(hardware));
    }
    ConvertImage(context, imageDst,  oldDstFormat);

    OVG_FREE(context->os, lutTex);
    gcmVERIFY_OK(gcoTEXTURE_Destroy(texture));

    gcmFOOTER_NO();
}

gcoTEXTURE
_SetupTexture(_VGContext *context, gctINT32 width, gctINT32 height, gceSURF_FORMAT format, gcoSURF surface)
{
    gceSTATUS  status =  gcvSTATUS_OK;
    gcoTEXTURE texture;

    gcmHEADER_ARG("context=0x%x width=%d height=%d format=%d surface=0x%x",
                   context, width, height, format, surface);

    do
    {
        /* Create the texture object. */
        status = gcoTEXTURE_Construct(context->hal, &texture);
        if (gcmIS_ERROR(status))
        {
            texture = gcvNULL;
            break;
        }

        status = gcoTEXTURE_AddMipMapFromSurface(texture, 0, surface);

        if (gcmIS_ERROR(status))
        {
            gcmVERIFY_OK(gcoTEXTURE_Destroy(texture));
            texture = gcvNULL;
            break;
        }
        gcmVERIFY_OK(gcoSURF_SetResolvability(surface, gcvFALSE));
    }
    while (gcvFALSE);

    gcmFOOTER_ARG("return=0x%x", texture);

    /* Return the status. */
    return texture;
}


VGImage
vgCreateEGLImageTargetKHR(VGeglImageKHR inImage)
{
    _VGImage            *image = gcvNULL;
    khrEGL_IMAGE_PTR    kImage = gcvNULL;
    _VGContext* context;

    gcmHEADER_ARG("inImage=0x%x", inImage);
    OVG_GET_CONTEXT(VG_INVALID_HANDLE);

    kImage = (khrEGL_IMAGE_PTR)inImage;
    /* Validate the input. */
    if (kImage == gcvNULL ||
        kImage->magic != KHR_EGL_IMAGE_MAGIC_NUM
        )
    {
        SetError(context, VG_ILLEGAL_ARGUMENT_ERROR);
        gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
    }
    /*TODO: Check the format.*/

    /*Create the vgimage object.*/
    NEWOBJ(_VGImage, context->os, image);

    if (!image)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        OVG_RETURN(VG_INVALID_HANDLE);
    }

    if (!vgshInsertObject(context, &image->object, VGObject_Image)) {
        DELETEOBJ(_VGImage, context->os, image);
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
    }

    VGObject_AddRef(context->os, &image->object);
    switch (kImage->type)
    {
    case KHR_IMAGE_PIXMAP:
    case KHR_IMAGE_TEXTURE_2D:
    case KHR_IMAGE_RENDER_BUFFER:
        /*Just give other item useless value*/
        SetError(context, VG_UNSUPPORTED_IMAGE_FORMAT_ERROR);
        gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
        break;

    default:
        image->width            = kImage->u.vgimage.width;
        image->height           = kImage->u.vgimage.height;
        image->allowedQuality   = kImage->u.vgimage.allowedQuality;

        image->dirty            = *kImage->u.vgimage.dirtyPtr;
        image->dirtyPtr         = kImage->u.vgimage.dirtyPtr;
        image->rootOffsetX      = kImage->u.vgimage.rootOffsetX;
        image->rootOffsetY      = kImage->u.vgimage.rootOffsetY;
        image->rootWidth        = kImage->u.vgimage.rootWidth;
        image->rootHeight       = kImage->u.vgimage.rootHeight;
        image->surface          = kImage->surface;
        image->texSurface       = kImage->u.vgimage.texSurface;
        break;
    }
    gcmVERIFY_OK(gcoSURF_ReferenceSurface(image->surface));
    gcmVERIFY_OK(gcoSURF_ReferenceSurface(image->texSurface));

    vgshGetFormatColorDesc(kImage->u.vgimage.format, &image->internalColorDesc);

    gcmVERIFY_OK(vgshCreateImageStream(
        context, image,
        0, 0, 0, 0, image->width, image->height,
        &image->stream));

    if (!image->stream)
    {
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
        gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return  VG_INVALID_HANDLE;
    }

    image->texture = _SetupTexture(context, image->width, image->height,
        image->internalColorDesc.surFormat, image->texSurface);
    gcmASSERT(image->texture);

    /*can not use the gcoSURF_Clear here!!, because the surface size may not be aligned, will make the clear funciton slow*/
    {
        gctPOINTER memory;
        gctINT     stride;
        gctUINT    alignedHeight;
        gcmVERIFY_OK(gcoSURF_Lock(image->texSurface, gcvNULL, &memory));
        gcmVERIFY_OK(gcoSURF_GetAlignedSize(image->texSurface, gcvNULL, &alignedHeight, &stride));

        gcoOS_ZeroMemory(memory, stride * alignedHeight);
        gcmVERIFY_OK(gcoSURF_Unlock(image->texSurface, memory));
    }

    gcmFOOTER_ARG("return=%d", image->object.name);
    return (VGImage)image->object.name;
}


