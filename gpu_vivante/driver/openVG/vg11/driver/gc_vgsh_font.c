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

void _VGFontCtor(gcoOS os, _VGFont *font)
{
	gcmHEADER_ARG("os=0x%x font=0x%x",
				  os, font);

	gcoOS_ZeroMemory(&font->object, sizeof(_VGObject));
	ARRAY_CTOR(font->glyphs, os);

	gcmFOOTER_NO();
}

void _VGFontDtor(gcoOS os, _VGFont *font)
{
	gctINT32 i;

	gcmHEADER_ARG("os=0x%x font=0x%x",
				  os, font);
	for (i = 0; i < font->glyphs.size; i++)
	{
		if (font->glyphs.items[i].type == GLYPH_PATH)
		{
			if (font->glyphs.items[i].path != VG_INVALID_HANDLE)
				VGObject_Release(os, &font->glyphs.items[i].path->object);
		}
		else
		if (font->glyphs.items[i].type == GLYPH_IMAGE)
		{
			if (font->glyphs.items[i].image != VG_INVALID_HANDLE)
			    VGObject_Release(os, &font->glyphs.items[i].image->object);
		}
	}
	ARRAY_DTOR(font->glyphs);

	gcmFOOTER_NO();
}

void _VGGlyphCtor(gcoOS os, _VGGlyph *glyph)
{
	gcmHEADER_ARG("os=0x%x glyph=0x%x",
				  os, glyph);

	glyph->index	= (_VGuint32) 0xffffffff;
	glyph->hinted	= gcvFALSE;
	glyph->origin.x = glyph->origin.y = 0;
	glyph->escapement.x = glyph->escapement.y = 0;
	glyph->type		= GLYPH_UNINITIALIZED;
	glyph->path		= gcvNULL;
	glyph->image	= gcvNULL;

	gcmFOOTER_NO();
}

void _VGGlyphDtor(gcoOS os, _VGGlyph *glyph)
{
	gcmHEADER_ARG("os=0x%x glyph=0x%x",
				  os, glyph);

	if (glyph->path != VG_INVALID_HANDLE)
	{
		VGObject_Release(os, &glyph->path->object);
	}
	else
	if (glyph->image != VG_INVALID_HANDLE)
	{
		VGObject_Release(os, &glyph->image->object);
	}

	gcmFOOTER_NO();
}

VGFont vgCreateFont(VGint		glyphCapacityHint)
{
	_VGFont *font = gcvNULL;
	_VGContext* context;
	gcmHEADER_ARG("glyphCapacityHint=%d", glyphCapacityHint);

	OVG_GET_CONTEXT(VG_INVALID_HANDLE);

	vgmPROFILE(context, VGCREATEFONT, 0);

	/* Argument check. */
	OVG_IF_ERROR(glyphCapacityHint < 0, VG_ILLEGAL_ARGUMENT_ERROR, VG_INVALID_HANDLE);

	/* Create. */
	NEWOBJ(_VGFont, context->os, font);

	if (!font)
	{
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
		gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
	}

	if (!vgshInsertObject(context, &font->object, VGObject_Font))
	{
        DELETEOBJ(_VGFont, context->os, font);
        SetError(context, VG_OUT_OF_MEMORY_ERROR);
		gcmFOOTER_ARG("return=%d", VG_INVALID_HANDLE);
        return VG_INVALID_HANDLE;
	}

	VGObject_AddRef(context->os, &font->object);
	ARRAY_ALLOCATE(font->glyphs, glyphCapacityHint > 0 ? glyphCapacityHint : 26);

	gcmFOOTER_ARG("return=%d", font->object.name);
	return font->object.name;
}

void vgDestroyFont(VGFont		font)
{
	_VGFont *f = gcvNULL;
	_VGContext* context;
	gcmHEADER_ARG("font=%d", font);

	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGDESTROYFONT, 0);

	f = _VGFONT(context, font);
	OVG_IF_ERROR(!f, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);

	vgshRemoveObject(context, &f->object);
	VGObject_Release(context->os, &f->object);

	gcmFOOTER_NO();
}

_VGGlyph *findGlyph(_VGFont		*font,
					_VGuint32	index)
{
	gctINT32 i;
	_VGGlyph *item = gcvNULL;

	gcmHEADER_ARG("font=0x%x index=%d",
				  font, index);

	for (i = 0; i < font->glyphs.size; ++i)
	{
		item = &font->glyphs.items[i];
		if (item->index == index)
		{
			gcmFOOTER_ARG("return=0x%x", item);
			return item;
		}
	}

	gcmFOOTER_ARG("return=0x%x", gcvNULL);
	return gcvNULL;
}

void newGlyph(_VGFont	*font,
			  _VGGlyph	**glyph)
{
	gctINT32 i;

	gcmHEADER_ARG("font=0x%x glyph=0x%x",
				  font, glyph);

	*glyph = gcvNULL;
	for (i = 0; i < font->glyphs.size; ++i)
	{
		if (font->glyphs.items[i].type == GLYPH_UNINITIALIZED)
		{
			*glyph = &font->glyphs.items[i];
			break;
		}
	}

	if (*glyph == gcvNULL)
	{
		gctINT32 size = font->glyphs.size + 1;
		ARRAY_RESIZE_RESERVE(font->glyphs, size);
		*glyph = &font->glyphs.items[font->glyphs.size - 1];
	}

	gcmFOOTER_NO();
}

void clearGlyph(gcoOS os, _VGGlyph *glyph)
{
	gcmHEADER_ARG("os=0x%x glyph=0x%x",
				  os, glyph);

	_VGGlyphDtor(os, glyph);
	_VGGlyphCtor(os, glyph);

	gcmFOOTER_NO();
}

void setGlyphToPath(gcoOS           os,
					_VGFont	        *font,
					_VGuint32       index,
					_VGPath	        *path,
					gctBOOL         hinted,
					const gctFLOAT  origin[2],
					const gctFLOAT  escapement[2])
{
	_VGGlyph	*glyph = gcvNULL;

	gcmHEADER_ARG("os=0x%x font=0x%x index=%d path=0x%x hinted=%s origin=0x%x escapement=0x%x",
				  os, font, index, path,
				  hinted ? "TRUE" : "FALSE",
				  origin, escapement);

	glyph = findGlyph(font, index);
	if (glyph == gcvNULL)
	{
		newGlyph(font, &glyph);
	}
	else
	{
		clearGlyph(os, glyph);
	}

	/* Set the data. */
	glyph->escapement.x	= escapement[0];
	glyph->escapement.y	= escapement[1];
	glyph->origin.x		= origin[0];
	glyph->origin.y		= origin[1];
	glyph->index		= index;
	glyph->hinted		= hinted;
	glyph->type			= GLYPH_PATH;
	glyph->image		= gcvNULL;
	glyph->path			= path;

	gcmFOOTER_NO();
}

void setGlyphToImage(gcoOS          os,
					 _VGFont	    *font,
					 _VGuint32	    index,
					 _VGImage	    *image,
					 const gctFLOAT	origin[2],
					 const gctFLOAT	escapement[2])
{
	_VGGlyph	*glyph = gcvNULL;

	gcmHEADER_ARG("os=0x%x font=0x%x index=%d image=0x%x origin=0x%x escapement=0x%x",
				  os, font, index, image, origin, escapement);

	glyph = findGlyph(font, index);
	if (glyph == gcvNULL)
	{
		newGlyph(font, &glyph);
	}
	else
	{
		clearGlyph(os, glyph);
	}

	/* Set the data. */
	glyph->escapement.x	= escapement[0];
	glyph->escapement.y	= escapement[1];
	glyph->origin.x		= origin[0];
	glyph->origin.y		= origin[1];
	glyph->index		= index;
	glyph->type			= GLYPH_IMAGE;
	glyph->image		= image;
	glyph->path			= gcvNULL;

	gcmFOOTER_NO();
}

void vgSetGlyphToPath(VGFont	    font,
                      VGuint	    glyphIndex,
                      VGPath	    path,
                      VGboolean     isHinted,
                      const VGfloat	glyphOrigin[2],
                      const VGfloat	escapement[2])
{
	_VGFont *fontObj = gcvNULL;
	_VGPath *pathObj = gcvNULL;
	_VGContext* context;

	gcmHEADER_ARG("font=%d glyphIndex=%d path=%d isHinted=%s glyphOrigin=0x%x escapement=0x%x",
			font, glyphIndex, path,
			isHinted ? "VG_TRUE" : "VG_FALSE",
			glyphOrigin, escapement);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGSETGLYPHTOPATH, 0);

	fontObj = _VGFONT(context, font);
	pathObj = _VGPATH(context, path);

	OVG_IF_ERROR(!fontObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(!pathObj && (path != VG_INVALID_HANDLE), VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!glyphOrigin || !escapement ||
				 !isAligned(glyphOrigin, sizeof(VGfloat)) ||
				 !isAligned(escapement,  sizeof(VGfloat)), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	setGlyphToPath(context->os, fontObj, glyphIndex, pathObj, isHinted, glyphOrigin, escapement);
	if (pathObj)	/*pathObj == gcvNULL is allowed, when it's an invisual character like <space>*/
		VGObject_AddRef(context->os, &pathObj->object);

	gcmFOOTER_NO();
}

void vgSetGlyphToImage(VGFont	        font,
                       VGuint	        glyphIndex,
                       VGImage	        image,
                       const VGfloat	glyphOrigin [2],
                       const VGfloat	escapement[2])
{
	_VGFont  *fontObj = gcvNULL;
	_VGImage *imgObj  = gcvNULL;
	_VGContext* context;

	gcmHEADER_ARG("font=%d glyphIndex=%d image=%d glyphOrigin=0x%x escapement=0x%x",
			font, glyphIndex, image, glyphOrigin, escapement);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGSETGLYPHTOIMAGE, 0);

	fontObj = _VGFONT(context, font);
	imgObj  = _VGIMAGE(context, image);

	OVG_IF_ERROR(!fontObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(!imgObj && (image != VG_INVALID_HANDLE), VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
    OVG_IF_ERROR(!glyphOrigin || !escapement ||
				 !isAligned(glyphOrigin, sizeof(VGfloat)) ||
				 !isAligned(escapement,  sizeof(VGfloat)), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	setGlyphToImage(context->os, fontObj, glyphIndex, imgObj, glyphOrigin, escapement);
	if (imgObj)		/*imgObj == gcvNULL is allowed, when it's an invisual character like <space>*/
		VGObject_AddRef(context->os, &imgObj->object);

	gcmFOOTER_NO();
}

void vgClearGlyph(VGFont		font,
				  VGuint		glyphIndex)
{
	_VGFont	*fontObj = gcvNULL;
	_VGGlyph *glyphObj = gcvNULL;
	_VGContext* context;

	gcmHEADER_ARG("font=%d glyphIndex=%d", font, glyphIndex);
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGCLEARGLYPH, 0);

	fontObj = _VGFONT(context, font);
	OVG_IF_ERROR(!fontObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	glyphObj = findGlyph(fontObj, glyphIndex);
	OVG_IF_ERROR(!glyphObj, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	clearGlyph(context->os, glyphObj);

	gcmFOOTER_NO();
}

void vgDrawGlyph(VGFont			font,
                 VGuint			glyphIndex,
                 VGbitfield		paintModes,
                 VGboolean		allowAutoHinting)
{
	_VGFont* fontObj;
    _VGGlyph* glyphObj;
	_VGContext* context;

	gcmHEADER_ARG("font=%d glyphIndex=%d paintModes=0x%x allowAutoHinting=%s",
			font, glyphIndex, paintModes,
			allowAutoHinting ? "VG_TRUE" : "VG_FALSE");
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGDRAWGLYPH, 0);
	vgmPROFILE(context, VG_PROFILER_PRIMITIVE_TYPE, VG_GLYPH);
	vgmPROFILE(context, VG_PROFILER_PRIMITIVE_COUNT, 1);
	if (paintModes & VG_STROKE_PATH)
		vgmPROFILE(context, VG_PROFILER_STROKE, 1);
	if (paintModes & VG_FILL_PATH)
		vgmPROFILE(context, VG_PROFILER_FILL, 1);

	fontObj = _VGFONT(context, font);
	OVG_IF_ERROR(!fontObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(paintModes & ~(VG_FILL_PATH | VG_STROKE_PATH), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	glyphObj = findGlyph(fontObj, glyphIndex);
    OVG_IF_ERROR(!glyphObj, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

    if(paintModes)
    {
		_VGMatrix3x3 n, tempM3;
        _VGMatrix3x3 matrix = context->glyphUserToSurface;
        _VGVector2 t = context->glyphOrigin;
		V2_SUB(t, glyphObj->origin.x, glyphObj->origin.y);

		_vgSetMatrix(&n, 1, 0, t.x,
					  0, 1, t.y,
					  0, 0, 1);

		tempM3 = matrix;
		MultMatrix(&tempM3, &n, &matrix);
		FORCE_AFFINE(matrix.m);

        if (glyphObj->image != gcvNULL)
		{
			gcmVERIFY_OK(vgshDrawImage(context, glyphObj->image, &matrix));
		}
        else if(glyphObj->path != gcvNULL)
		{
			gcmVERIFY_OK(vgshDrawPath(context, glyphObj->path, paintModes, &matrix));
		}
    }

	V2_ADD(context->glyphOrigin, glyphObj->escapement.x, glyphObj->escapement.y);
    context->inputGlyphOrigin = context->glyphOrigin;

	gcmFOOTER_NO();
}

void vgDrawGlyphs(VGFont		    font,
                  VGint			    glyphCount,
                  const VGuint		*glyphIndices,
                  const VGfloat		*adjustments_x,
                  const VGfloat		*adjustments_y,
                  VGbitfield	    paintModes,
                  VGboolean		    allowAutoHinting)
{
	gctINT32 i;
	_VGFont	 *fontObj;
	_VGGlyph *glyphObj;
	_VGContext* context;

	gcmHEADER_ARG("font=%d glyphCount=%d glyphIndices=0x%x adjustments_x=0x%x "
				  "adjustments_y=0x%x paintModes=0x%x allowAutoHinting=%s",
				  font, glyphCount, glyphIndices, adjustments_x, adjustments_y, paintModes,
				  allowAutoHinting ? "VG_TRUE" : "VG_FALSE");
	OVG_GET_CONTEXT(OVG_NO_RETVAL);

	vgmPROFILE(context, VGDRAWGLYPHS, 0);
	vgmPROFILE(context, VG_PROFILER_PRIMITIVE_TYPE, VG_GLYPH);
	vgmPROFILE(context, VG_PROFILER_PRIMITIVE_COUNT, glyphCount);
	if (paintModes & VG_STROKE_PATH)
		vgmPROFILE(context, VG_PROFILER_STROKE, glyphCount);
	if (paintModes & VG_FILL_PATH)
		vgmPROFILE(context, VG_PROFILER_FILL, glyphCount);

	fontObj = _VGFONT(context, font);

	OVG_IF_ERROR(!fontObj, VG_BAD_HANDLE_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(!glyphIndices || !isAligned(glyphIndices, sizeof(VGuint)) || glyphCount <= 0, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR((adjustments_x && !isAligned(adjustments_x, sizeof(VGfloat))) || (adjustments_y && !isAligned(adjustments_y, sizeof(VGfloat))), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
	OVG_IF_ERROR(paintModes & ~(VG_FILL_PATH | VG_STROKE_PATH), VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);

	for(i=0; i < glyphCount; i++)
	{
        glyphObj = findGlyph(fontObj, glyphIndices[i]);
		OVG_IF_ERROR(!glyphObj || glyphObj->type == GLYPH_UNINITIALIZED, VG_ILLEGAL_ARGUMENT_ERROR, OVG_NO_RETVAL);
    }

	for(i = 0; i < glyphCount; i++)
	{
        glyphObj = findGlyph(fontObj, glyphIndices[i]);
		if (glyphObj == gcvNULL)
			break;

        if(paintModes)
        {
            _VGMatrix3x3 matrix, n, tempM3;
			_VGVector2 t;
			tempM3 = context->glyphUserToSurface;
            t = context->glyphOrigin;
			V2_SUB(t, glyphObj->origin.x, glyphObj->origin.y);
			_vgSetMatrix(&n, 1, 0, t.x,
						  0, 1, t.y,
						  0, 0, 1);
			MultMatrix(&tempM3, &n, &matrix);
			FORCE_AFFINE(matrix.m);

            if(glyphObj->image != gcvNULL)
			{
				gcmVERIFY_OK(vgshDrawImage(context, glyphObj->image, &matrix));
			}
            else if(glyphObj->path != gcvNULL)
			{
				gcmVERIFY_OK(vgshDrawPath(context, glyphObj->path, paintModes, &matrix));
			}
        }

		V2_ADD(context->glyphOrigin, glyphObj->escapement.x, glyphObj->escapement.y);
        if(adjustments_x)
            context->glyphOrigin.x += inputFloat(adjustments_x[i]);
        if(adjustments_y)
            context->glyphOrigin.y += inputFloat(adjustments_y[i]);
        context->inputGlyphOrigin = context->glyphOrigin;
	}

	gcmFOOTER_NO();
}
