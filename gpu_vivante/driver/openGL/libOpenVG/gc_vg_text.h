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




#ifndef __gc_vg_font_h__
#define __gc_vg_font_h__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*----------------------- Glyph structure definition. -----------------------*/

typedef struct _vgsGLYPH * vgsGLYPH_PTR;
typedef struct _vgsGLYPH
{
	/* Glyph index. */
	VGint					index;

	/* Glyph states. */
	VGfloat					originX;
	VGfloat					originY;
	VGfloat					escapementX;
	VGfloat					escapementY;
	vgsIMAGE_PTR			image;
	vgsPATH_PTR				path;
	VGboolean				hinted;

	/* Next glyph in the collision chain. */
	vgsGLYPH_PTR			next;
}
vgsGLYPH;

/*---------------------------------------------------------------------------*/
/*----------------------- Object structure definition. ----------------------*/

typedef struct _vgsFONT * vgsFONT_PTR;
typedef struct _vgsFONT
{
	/* Embedded object. */
	vgsOBJECT				object;

	/* Object states. */
	VGint					numGlyphs;			/* VG_FONT_NUM_GLYPHS */

	/* Glyph hash table. */
	vgsGLYPH_PTR *			table;
	VGint					tableEntries;
}
vgsFONT;

/*----------------------------------------------------------------------------*/
/*------------------ Object management function declarations. ----------------*/

gceSTATUS vgfReferenceFont(
	vgsCONTEXT_PTR Context,
	vgsFONT_PTR * Font
	);

void vgfSetFontObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	);

#ifdef __cplusplus
}
#endif

#endif
