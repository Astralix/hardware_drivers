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


#ifndef __gc_vgsh_font_h_
#define __gc_vgsh_font_h_

typedef enum GlyphType
{
    GLYPH_UNINITIALIZED     = 0,
    GLYPH_PATH              = 1,
    GLYPH_IMAGE             = 2
}GlyphType;

struct _VGGlyph
{
    _VGuint32	index;
    GlyphType	type;
	_VGPath		*path;
	_VGImage	*image;
	gctBOOL		hinted;
	_VGVector2	origin;
	_VGVector2	escapement;
};

struct _VGFont
{
	_VGObject object;
	_VGGlyphArray glyphs;
};

void _VGFontCtor(gcoOS os, _VGFont *font);
void _VGFontDtor(gcoOS os, _VGFont *font);
void _VGGlyphCtor(gcoOS os, _VGGlyph *glyph);
void _VGGlyphDtor(gcoOS os, _VGGlyph *glyph);

#endif /* __gc_vgsh_font_h_ */
