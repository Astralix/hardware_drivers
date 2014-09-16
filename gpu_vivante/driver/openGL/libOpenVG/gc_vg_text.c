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




#include "gc_vg_precomp.h"

/******************************************************************************\
*********************** Support Functions and Definitions **********************
\******************************************************************************/

/* Hash function to determine the glyph hash table index based on the index
   of the glyph. */
#define vgmHASHINDEX(GlyphIndex, EntryCount) \
	(((((gctUINT8_PTR) & (GlyphIndex)) [0] * 31) + \
	  (((gctUINT8_PTR) & (GlyphIndex)) [1] * 31) + \
	  (((gctUINT8_PTR) & (GlyphIndex)) [2] * 31) + \
	  (((gctUINT8_PTR) & (GlyphIndex)) [3] * 31)) % (EntryCount))


/*******************************************************************************
**
** _AllocateGlyph
**
** A primitive function to allocate and insert a new glyph into the collision
** chain.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Font
**       Pointer to the font object.
**
**    GlyphIndex
**       Index of the glyph to create.
**
**    Head
**       Pointer to the head of the collision chain.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _AllocateGlyph(
	vgsCONTEXT_PTR Context,
	vgsFONT_PTR Font,
	VGint GlyphIndex,
	vgsGLYPH_PTR * Head
	)
{
	gceSTATUS status;

	do
	{
		vgsGLYPH_PTR glyph;

		/* Allocate the object structure. */
		gcmERR_BREAK(gcoOS_Allocate(
			Context->os,
			gcmSIZEOF(vgsGLYPH),
			(gctPOINTER *) &glyph
			));

		/* Link in the new glyph. */
		glyph->next = *Head;
		*Head = glyph;

		/* Set the index. */
		glyph->index = GlyphIndex;

		/* Increment the glyph count. */
		Font->numGlyphs++;
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _ResetGlyph
**
** A primitive function to release resources held up by the specified glyph.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Glyph
**       Pointer to the glyph to be reset.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _ResetGlyph(
	vgsCONTEXT_PTR Context,
	vgsGLYPH_PTR Glyph
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		/* Dereference glyph image. */
		if (Glyph->image != gcvNULL)
		{
			gcmERR_BREAK(vgfUseImageAsGlyph(
				Context, Glyph->image, gcvFALSE
				));

			gcmERR_BREAK(vgfDereferenceObject(
				Context, (vgsOBJECT_PTR *) &Glyph->image
				));

			/* Reset the pointer. */
			Glyph->image = gcvNULL;
		}

		/* Dereference glyph path. */
		if (Glyph->path != gcvNULL)
		{
			gcmERR_BREAK(vgfDereferenceObject(
				Context, (vgsOBJECT_PTR *) &Glyph->path
				));

			/* Reset the pointer. */
			Glyph->path = gcvNULL;
		}
	}
	while (gcvFALSE);

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** _FindGlyph
**
** Find the glyph with the specified index.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    GlyphIndex
**       Index of the glyph to create.
**
**    Head
**       Pointer to the head of the collision chain.
**
** OUTPUT:
**
**    Previous
**       Pointer to the previous glyph.
**
**    Glyph
**       Pointer to the glyph.
*/

static void _FindGlyph(
	vgsCONTEXT_PTR Context,
	VGint GlyphIndex,
	vgsGLYPH_PTR * Head,
	vgsGLYPH_PTR * Previous,
	vgsGLYPH_PTR * Glyph
	)
{
	vgsGLYPH_PTR previous;
	vgsGLYPH_PTR glyph;

	/* Keep the history. */
	previous = gcvNULL;

	/* Get a pointer to the head of the collision chain. */
	glyph = *Head;

	/* Scan through the collision chain. */
	while (glyph != gcvNULL)
	{
		/* Found it? */
		if (glyph->index == GlyphIndex)
		{
			break;
		}

		/* Advance to the next glyph. */
		previous = glyph;
		glyph = glyph->next;
	}

	/* Set the result. */
	*Previous = previous;
	*Glyph = glyph;
}


/*******************************************************************************
**
** _AddGlyph
**
** Create a new glyph structure and assign it to the specified index.
** If another glyph already exists at the same index, the old glyph
** is replaced with the new one.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Font
**       Pointer to the font object.
**
**    GlyphIndex
**       Index of the glyph to create.
**
** OUTPUT:
**
**    Nothing.
*/

static vgsGLYPH_PTR _AddGlyph(
	vgsCONTEXT_PTR Context,
	vgsFONT_PTR Font,
	VGint GlyphIndex
	)
{
	gceSTATUS status = gcvSTATUS_OK;
	vgsGLYPH_PTR glyph = gcvNULL;

	do
	{
		VGint index;
		vgsGLYPH_PTR * head;

		/* Create the glyph hash table. */
		if (Font->table == gcvNULL)
		{
			/* Determine the size of the table in bytes. */
			gctSIZE_T tableSize = gcmSIZEOF(vgsGLYPH_PTR) * Font->tableEntries;

			/* Allocate the object structure. */
			gcmERR_BREAK(gcoOS_Allocate(
				Context->os,
				tableSize,
				(gctPOINTER *) &Font->table
				));

			/* Reset all entries. */
			gcmERR_BREAK(gcoOS_ZeroMemory(
				Font->table, tableSize
				));
		}

		/* Determine the hash table index. */
		index = vgmHASHINDEX(GlyphIndex, Font->tableEntries);

		/* Get a pointer to the head of the collision chain. */
		head = &Font->table[index];

		/* First item at this index? */
		if (*head == gcvNULL)
		{
			/* Allocate a new glyph. */
			gcmERR_BREAK(_AllocateGlyph(Context, Font, GlyphIndex, head));

			/* Set the return value. */
			glyph = *head;
		}

		/* First item has the same index? */
		else if ((*head)->index == GlyphIndex)
		{
			/* Release glyph resources. */
			gcmERR_BREAK(_ResetGlyph(Context, *head));

			/* Set the return value. */
			glyph = *head;
		}

		/* Search the collision chain for a glyph with the same index. */
		else
		{
			/* Find the glyph. */
			vgsGLYPH_PTR previous;
			_FindGlyph(Context, GlyphIndex, head, &previous, &glyph);

			/* Create a new glyph if nothing was found. */
			if (glyph == gcvNULL)
			{
				/* Allocate a new glyph. */
				gcmERR_BREAK(_AllocateGlyph(Context, Font, GlyphIndex, head));

				/* Set the return value. */
				glyph = *head;
			}

			/* Reuse the existing glyph. */
			else
			{
				/* Move the glyph to the head of the collision chain. */
				previous->next = glyph->next;
				glyph->next = *head;
				*head = glyph;

				/* Release glyph resources. */
				gcmERR_BREAK(_ResetGlyph(Context, glyph));
			}
		}
	}
	while (gcvFALSE);

	/* Return the pointer to the new glyph. */
	return gcmIS_SUCCESS(status)
		? glyph
		: gcvNULL;
}


/*******************************************************************************
**
** _FontDestructor
**
** Font object destructor. Called when the object is being destroyed.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
**    Object
**       Pointer to the object.
**
** OUTPUT:
**
**    Nothing.
*/

static gceSTATUS _FontDestructor(
	vgsCONTEXT_PTR Context,
	vgsOBJECT_PTR Object
	)
{
	gceSTATUS status = gcvSTATUS_OK;

	do
	{
		VGint i;

		/* Cast the object. */
		vgsFONT_PTR font = (vgsFONT_PTR) Object;

		/* Free font glyphs. */
		if (font->table != gcvNULL)
		{
			for (i = 0; i < font->tableEntries; i++)
			{
				/* Get the first glyph. */
				vgsGLYPH_PTR * head = &font->table[i];

				/* Scan through the current collision chain. */
				while (*head)
				{
					/* Get the next glyph. */
					vgsGLYPH_PTR next = (*head)->next;

					/* Reset the head glyph. */
					gcmERR_BREAK(_ResetGlyph(Context, *head));

					/* Delete the current glyph. */
					gcmERR_BREAK(gcoOS_Free(Context->os, *head));

					/* Set the new head. */
					*head = next;
				}

				/* Break if failed. */
				if (gcmIS_ERROR(status))
				{
					break;
				}
			}

			/* Delete the table. */
			gcmERR_BREAK(gcoOS_Free(Context->os, font->table));
			font->table = gcvNULL;
		}
	}
	while (gcvFALSE);

	return status;
}


/******************************************************************************\
************************** Individual State Functions **************************
\******************************************************************************/

/*----------------------------------------------------------------------------*/
/*---------------------------- VG_FONT_NUM_GLYPHS ----------------------------*/

vgmGETSTATE_FUNCTION(VG_FONT_NUM_GLYPHS)
{
	ValueConverter(
		Values, &((vgsFONT_PTR) Object)->numGlyphs, 1, VG_FALSE, VG_TRUE
		);
}


/******************************************************************************\
***************************** Context State Array ******************************
\******************************************************************************/

static vgsSTATEENTRY _stateArray[] =
{
	vgmDEFINE_SCALAR_READONLY_ENTRY(VG_FONT_NUM_GLYPHS, INT)
};


/******************************************************************************\
************************* Internal Text Management API *************************
\******************************************************************************/

/*******************************************************************************
**
** vgfReferenceFont
**
** vgfReferenceFont increments the reference count of the given VGFont object.
** If the object does not exist yet, the object will be created and initialized
** with the default values.
**
** INPUT:
**
**    Context
**       Pointer to the context.
**
** OUTPUT:
**
**    vgsFONT_PTR * Font
**       Handle to the new VGFont object.
*/

gceSTATUS vgfReferenceFont(
	vgsCONTEXT_PTR Context,
	vgsFONT_PTR * Font
	)
{
	gceSTATUS status, last;
	vgsFONT_PTR font = gcvNULL;

	do
	{
		/* Create the object if does not exist yet. */
		if (*Font == gcvNULL)
		{
			/* Allocate the object structure. */
			gcmERR_BREAK(gcoOS_Allocate(
				Context->os,
				gcmSIZEOF(vgsFONT),
				(gctPOINTER *) &font
				));

			/* Initialize the object structure. */
			font->object.type           = vgvOBJECTTYPE_FONT;
			font->object.prev           = gcvNULL;
			font->object.next           = gcvNULL;
			font->object.referenceCount = 0;
			font->object.userValid      = VG_TRUE;

			/* Insert the object into the cache. */
			gcmERR_BREAK(vgfObjectCacheInsert(
				Context, (vgsOBJECT_PTR) font
				));

			/* Set default object states. */
			font->numGlyphs = 0;
			font->table     = gcvNULL;

			/* Set the result pointer. */
			*Font = font;
		}

		/* Increment the reference count. */
		(*Font)->object.referenceCount++;

		/* Success. */
		return gcvSTATUS_OK;
	}
	while (gcvFALSE);

	/* Roll back. */
	if (font != gcvNULL)
	{
		gcmCHECK_STATUS(gcoOS_Free(Context->os, font));
	}

	/* Return status. */
	return status;
}


/*******************************************************************************
**
** vgfSetFontObjectList
**
** Initialize object cache list information.
**
** INPUT:
**
**    ListEntry
**       Entry to initialize.
**
** OUTPUT:
**
**    Nothing.
*/

void vgfSetFontObjectList(
	vgsOBJECT_LIST_PTR ListEntry
	)
{
	ListEntry->stateArray     = _stateArray;
	ListEntry->stateArraySize = gcmCOUNTOF(_stateArray);
	ListEntry->destructor     = _FontDestructor;
}


/******************************************************************************\
************************** OpenVG Font Management API **************************
\******************************************************************************/

/*******************************************************************************
**
** vgCreateFont
**
** vgCreateFont creates a new font object and returns a VGFont handle to it.
** The GlyphCapacityHint argument provides a hint as to the capacity of a
** VGFont, i.e., the total number of glyphs that this VGFont object will be
** required to accept.
**
** A value of 0 indicates that the value is unknown. If an error occurs during
** execution, VG_INVALID_HANDLE is returned.
**
** INPUT:
**
**    GlyphCapacityHint
**       Memory allocation hint.
**
** OUTPUT:
**
**    VGFont
**       Handle to the new VGFont object.
*/

VG_API_CALL VGFont VG_API_ENTRY vgCreateFont(
	VGint GlyphCapacityHint
	)
{
	vgsFONT_PTR font = gcvNULL;

	vgmENTERAPI(vgCreateFont)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(%d);\n",
			__FUNCTION__,
			GlyphCapacityHint
			);

		/* Validate the capacity hint argument. */
		if (GlyphCapacityHint < 0)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Allocate a new object. */
		if (gcmIS_ERROR(vgfReferenceFont(Context, &font)))
		{
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Determine the hash table allocation size. */
		if (GlyphCapacityHint > 0)
		{
			font->tableEntries = GlyphCapacityHint;
		}
		else
		{
			/* Use 256 characters by default as in 256-character ASCII table. */
			font->tableEntries = 256;
		}

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s() = 0x%08X;\n",
			__FUNCTION__,
			font
			);
	}
	vgmLEAVEAPI(vgCreateFont);

	return (VGFont) font;
}


/*******************************************************************************
**
** vgDestroyFont
**
** vgDestroyFont destroys the VGFont object pointed to by the Font argument.
** Note that vgDestroyFont will not destroy underlying objects that were used
** to define glyphs in the font. It is the responsibility of an application to
** destroy all VGPath or VGImage objects that were used in a VGFont, if they
** are no longer in use.
**
** INPUT:
**
**    Font
**       Handle to a valid VGFont object.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgDestroyFont(
	VGFont Font
	)
{
	vgmENTERAPI(vgDestroyFont)
	{
		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X);\n",
			__FUNCTION__,
			Font
			);

		if (!vgfVerifyUserObject(Context, Font, vgvOBJECTTYPE_FONT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Invalidate the object. */
		((vgsOBJECT_PTR) Font)->userValid = VG_FALSE;

		/* Decrement the reference count. */
		gcmVERIFY_OK(vgfDereferenceObject(
			Context,
			(vgsOBJECT_PTR *) &Font
			));
	}
	vgmLEAVEAPI(vgDestroyFont);
}


/*******************************************************************************
**
** vgSetGlyphToPath
**
** vgSetGlyphToPath creates a new glyph and assigns the given path to a glyph
** associated with the GlyphIndex in a font object. The GlyphOrigin argument
** defines the coordinates of the glyph origin within the path, and the
** Escapement parameter determines the advance width for this glyph. Both
** GlyphOrigin and Escapement coordinates are defined in the same coordinate
** system as the path. For glyphs that have no visual representation (e.g., the
** <space> character), a value of VG_INVALID_HANDLE is used for path. The
** reference count for the path is incremented.
**
** The path object may define either an original glyph outline, or an outline
** that has been scaled and hinted to a particular size (in surface coordinate
** units); this is defined by the IsHinted parameter, which can be used by
** implementation for text-specific optimizations (e.g., heuristic auto-hinting
** of unhinted outlines).
**
** When IsHinted is equal to VG_TRUE, the implementation will never apply
** auto-hinting; otherwise, auto hinting will be applied at the implementation's
** discretion.
**
** INPUT:
**
**    Font
**       Handle to a valid VGFont object.
**
**    GlyphIndex
**       Index of the glyph within the font to be created.
**
**    Path
**       Handle to a valid VGPath object that defines the glyph.
**
**    IsHinted
**       Optimization hint.
**
**    GlyphOrigin
**       The coordinates of the glyph origin within the path.
**
**    Escapement
**       The advance width for this glyph.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgSetGlyphToPath(
	VGFont Font,
	VGuint GlyphIndex,
	VGPath Path,
	VGboolean IsHinted,
	const VGfloat GlyphOrigin[2],
	const VGfloat Escapement[2]
	)
{
	vgmENTERAPI(vgSetGlyphToPath)
	{
		vgsFONT_PTR font;
		vgsGLYPH_PTR glyph;
		vgsPATH_PTR path;

		vgmDUMP_FLOAT_ARRAY(
			GlyphOrigin, 2
			);

		vgmDUMP_FLOAT_ARRAY(
			Escapement, 2
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d, 0x%08X, %d, GlyphOrigin, Escapement);\n",
			__FUNCTION__,
			Font, GlyphIndex, Path, IsHinted
			);

		if (!vgfVerifyUserObject(Context, Font, vgvOBJECTTYPE_FONT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Verify pointers. */
		if (vgmIS_INVALID_PTR(GlyphOrigin, 4) ||
			vgmIS_INVALID_PTR(Escapement,  4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Verify the path object. */
		if (Path == VG_INVALID_HANDLE)
		{
			path = gcvNULL;
		}
		else
		{
			if (!vgfVerifyUserObject(Context, Path, vgvOBJECTTYPE_PATH))
			{
				vgmERROR(VG_BAD_HANDLE_ERROR);
				break;
			}

			/* Cast the object. */
			path = (vgsPATH_PTR) Path;

			/* Increment reference. */
			if (gcmIS_ERROR(vgfReferencePath(Context, &path)))
			{
				gcmFATAL("Failed to reference VGPath object.");
				break;
			}
		}

		/* Cast the object. */
		font = (vgsFONT_PTR) Font;

		/* Add a new glyph; if a previously created glyph exists
		   at the same index, it will be replaced with the new one. */
		glyph = _AddGlyph(Context, font, GlyphIndex);

		/* Verify the result. */
		if (glyph == gcvNULL)
		{
			/* Dereference the path. */
			if (path != gcvNULL)
			{
				gcmVERIFY_OK(vgfDereferenceObject(
					Context,
					(vgsOBJECT_PTR *) &path
					));
			}

			/* Signal out of memory error. */
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Initialize the new glyph. */
		glyph->originX     = GlyphOrigin[0];
		glyph->originY     = GlyphOrigin[1];
		glyph->escapementX = Escapement[0];
		glyph->escapementY = Escapement[1];
		glyph->image       = gcvNULL;
		glyph->path        = path;
		glyph->hinted      = IsHinted;
	}
	vgmLEAVEAPI(vgSetGlyphToPath);
}


/*******************************************************************************
**
** vgSetGlyphToImage
**
** vgSetGlyphToImage creates a new glyph and assigns the given image into a
** glyph associated with the GlyphIndex in a font object. The GlyphOrigin
** argument defines the coordinates of the glyph origin within the image, and
** the Escapement parameter determines the advance width for this glyph. Both
** GlyphOrigin and Escapement coordinates are defined in the image coordinate
** system. Applying transformations to an image (other than translations mapped
** to pixel grid in surface coordinate system) should be avoided as much as
** possible. For glyphs that have no visual representation (e.g., the <space>
** character), a value of VG_INVALID_HANDLE is used for image. The reference
** count for the image is incremented.
**
** INPUT:
**
**    Font
**       Handle to a valid VGFont object.
**
**    GlyphIndex
**       Index of the glyph within the font to be created.
**
**    Image
**       Handle to a valid VGImage object that defines the glyph.
**
**    GlyphOrigin
**       The coordinates of the glyph origin within the image.
**
**    Escapement
**       The advance width for this glyph.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgSetGlyphToImage(
	VGFont Font,
	VGuint GlyphIndex,
	VGImage Image,
	const VGfloat GlyphOrigin[2],
	const VGfloat Escapement[2]
	)
{
	vgmENTERAPI(vgSetGlyphToImage)
	{
		vgsFONT_PTR font;
		vgsGLYPH_PTR glyph;
		vgsIMAGE_PTR image;

		vgmDUMP_FLOAT_ARRAY(
			GlyphOrigin, 2
			);

		vgmDUMP_FLOAT_ARRAY(
			Escapement, 2
			);

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d, 0x%08X, GlyphOrigin, Escapement);\n",
			__FUNCTION__,
			Font, GlyphIndex, Image
			);

		if (!vgfVerifyUserObject(Context, Font, vgvOBJECTTYPE_FONT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Verify pointers. */
		if (vgmIS_INVALID_PTR(GlyphOrigin, 4) ||
			vgmIS_INVALID_PTR(Escapement,  4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Verify the path object. */
		if (Image == VG_INVALID_HANDLE)
		{
			image = gcvNULL;
		}
		else
		{
			if (!vgfVerifyUserObject(Context, Image, vgvOBJECTTYPE_IMAGE))
			{
				vgmERROR(VG_BAD_HANDLE_ERROR);
				break;
			}

			/* Cast the object. */
			image = (vgsIMAGE_PTR) Image;

			/* Set usage as the glyph image. */
			if (gcmIS_ERROR(vgfUseImageAsGlyph(Context, image, gcvTRUE)))
			{
				/* Signal out of memory error. */
				vgmERROR(VG_IMAGE_IN_USE_ERROR);
				break;
			}

			/* Increment image reference. */
			gcmVERIFY_OK(vgfReferenceImage(Context, &image));
		}

		/* Cast the object. */
		font = (vgsFONT_PTR) Font;

		/* Add a new glyph; if a previously created glyph exists
		   at the same index, it will be replaced with the new one. */
		glyph = _AddGlyph(Context, font, GlyphIndex);

		/* Verify the result. */
		if (glyph == gcvNULL)
		{
			/* Roll image changes back. */
			if (image != gcvNULL)
			{
				/* Set usage as the glyph image. */
				gcmVERIFY_OK(vgfUseImageAsGlyph(
					Context, image, gcvFALSE
					));

				/* Increment image reference. */
				gcmVERIFY_OK(vgfDereferenceObject(
					Context, (vgsOBJECT_PTR *) &image
					));
			}

			/* Signal out of memory error. */
			vgmERROR(VG_OUT_OF_MEMORY_ERROR);
			break;
		}

		/* Initialize the new glyph. */
		glyph->originX     = GlyphOrigin[0];
		glyph->originY     = GlyphOrigin[1];
		glyph->escapementX = Escapement[0];
		glyph->escapementY = Escapement[1];
		glyph->image       = image;
		glyph->path        = gcvNULL;
		glyph->hinted      = gcvFALSE;
	}
	vgmLEAVEAPI(vgSetGlyphToImage);
}


/*******************************************************************************
**
** vgClearGlyph
**
** vgClearGlyph deletes the glyph defined by a GlyphIndex parameter from a font.
** The reference count for the VGPath or VGImage object to which the glyph was
** previously set is decremented, and the object's resources are released if the
** count has fallen to 0.
**
** INPUT:
**
**    Font
**       Handle to a valid VGFont object.
**
**    GlyphIndex
**       Index of the glyph within the font to be deleted.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgClearGlyph(
	VGFont Font,
	VGuint GlyphIndex
	)
{
	vgmENTERAPI(vgClearGlyph)
	{
		vgsFONT_PTR font;
		vgsGLYPH_PTR * head;
		vgsGLYPH_PTR previous;
		vgsGLYPH_PTR glyph;
		VGint index;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d);\n",
			__FUNCTION__,
			Font, GlyphIndex
			);

		if (!vgfVerifyUserObject(Context, Font, vgvOBJECTTYPE_FONT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		font = (vgsFONT_PTR) Font;

		/* See if the font table has been created. */
		if (font->table == gcvNULL)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Determine the hash table index. */
		index = vgmHASHINDEX(GlyphIndex, font->tableEntries);

		/* Get a pointer to the head of the collision chain. */
		head = &font->table[index];

		/* Find the glyph. */
		_FindGlyph(Context, GlyphIndex, head, &previous, &glyph);

		/* Bad index? */
		if (glyph == gcvNULL)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Remove the glyph from the chain. */
		if (previous == gcvNULL)
		{
			/* Set the new head. */
			*head = glyph->next;
		}
		else
		{
			previous->next = glyph->next;
		}

		/* Release glyph resources. */
		gcmVERIFY_OK(_ResetGlyph(Context, glyph));

		/* Delete the current glyph. */
		gcmVERIFY_OK(gcoOS_Free(Context->os, glyph));

		/* Decrement the glyph count. */
		font->numGlyphs--;
	}
	vgmLEAVEAPI(vgClearGlyph);
}


/*******************************************************************************
**
** vgDrawGlyph
**
** vgDrawGlyph renders a glyph defined by the GlyphIndex using the given font
** object. The user space position of the glyph (the point where the glyph
** origin will be placed) is determined by value of VG_GLYPH_ORIGIN.
**
** vgDrawGlyph calculates the new text origin by translating the glyph origin
** by the escapement vector of the glyph defined by GlyphIndex. Following the
** call, the VG_GLYPH_ORIGIN parameter will be updated with the new origin.
**
** The PaintModes parameter controls how glyphs are rendered.
**
** If PaintModes is 0, neither VGImage-based nor VGPath-based glyphs are drawn.
** This mode is useful for determining the metrics of the glyph sequence.
**
** If PaintModes is equal to one of VG_FILL_PATH, VG_STROKE_PATH, or
** (VG_FILL_PATH | VG_STROKE_PATH), path-based glyphs are filled, stroked
** (outlined), or both, respectively, and image-based glyphs are drawn.
**
** When the AllowAutoHinting flag is set to VG_FALSE, rendering occurs without
** hinting. If AllowAutoHinting is equal to VG_TRUE, autohinting may be
** optionally applied to alter the glyph outlines slightly for better rendering
** quality. In this case, the escapement values will be adjusted to match the
** effects of hinting. Autohinting is not applied to image-based glyphs or
** path-based glyphs marked as IsHinted in vgSetGlyphToPath.
**
** INPUT:
**
**    Font
**       Handle to a valid VGFont object.
**
**    GlyphIndex
**       Index of the glyph within the font to be drawn.
**
**    PaintModes
**       Bitwise OR of VG_FILL_PATH and/or VG_STROKE_PATH (VGPaintMode).
**
**    AllowAutoHinting
**       Rendering quality hint.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgDrawGlyph(
	VGFont Font,
	VGuint GlyphIndex,
	VGbitfield PaintModes,
	VGboolean AllowAutoHinting
	)
{
	vgDrawGlyphs(
		Font,
		1,
		&GlyphIndex,
		gcvNULL,
		gcvNULL,
		PaintModes,
		AllowAutoHinting
		);
}


/*******************************************************************************
**
** vgDrawGlyphs
**
** vgDrawGlyphs renders a sequence of glyphs defined by the array pointed to
** by GlyphIndices using the given font object. The values in the AdjustmentsX
** and AdjustmentsY arrays define positional adjustment values for each pair of
** glyphs defined by the GlyphIndices array. The GlyphCount parameter defines
** the number of elements in the GlyphIndices and AdjustmentsX and AdjustmentsY
** arrays. The adjustment values defined in these arrays may represent kerning
** or other positional adjustments required for each pair of glyphs. If no
** adjustments for glyph positioning in a particular axis are required
** (all horizontal and/or vertical adjustments are zero), gcvNULL pointers may be
** passed for either or both of AdjustmentX and AdjustmentY. The adjustments
** values should be defined in the same coordinate system as the font glyphs;
** if the glyphs are defined by path objects with path data scaled (e.g., by
** a factor of 1/units-per-EM), the values in the AdjustmentX and AdjustmentY
** arrays are scaled using the same scale factor.
**
** The user space position of the first glyph (the point where the glyph origin
** will be placed) is determined by the value of VG_GLYPH_ORIGIN.
**
** vgDrawGlyphs calculates a new glyph origin for every glyph in the
** GlyphIndices array by translating the glyph origin by the escapement vector
** of the current glyph, and applying the necessary positional adjustments,
** taking into account both the escapement values associated with the glyphs
** as well as the AdjustmentsX and AdjustmentsY parameters. Following the call,
** the VG_GLYPH_ORIGIN parameter will be updated with the new origin.
**
** The PaintModes parameter controls how glyphs are rendered.
**
** If paintModes is 0, neither VGImage-based nor VGPath-based glyphs are drawn.
** This mode is useful for determining the metrics of the glyph sequence.
**
** If PaintModes equals to one of VG_FILL_PATH, VG_STROKE_PATH, or
** (VG_FILL_PATH | VG_STROKE_PATH), path-based glyphs are filled, stroked
** (outlined), or both, respectively, and image-based glyphs are drawn.
**
** When the AllowAutoHinting flag is set to VG_FALSE, rendering occurs without
** hinting. If AllowAutoHinting is equal to VG_TRUE, autohinting may be
** optionally applied to alter the glyph outlines slightly for better rendering
** quality. In this case, the escapement values will be adjusted to match the
** effects of hinting.
**
** INPUT:
**
**    Font
**       Handle to a valid VGFont object.
**
**    GlyphCount
**       The total number of glyphs to draw.
**
**    GlyphIndices
**       Pointer to the glyph index array.
**
**    AdjustmentsX, AdjustmentsY
**       Pointer to the glyph position adjustment array for each pair of glyphs.
**
**    PaintModes
**       Bitwise OR of VG_FILL_PATH and/or VG_STROKE_PATH (VGPaintMode).
**
**    AllowAutoHinting
**       Rendering quality hint.
**
** OUTPUT:
**
**    Nothing.
*/

VG_API_CALL void VG_API_ENTRY vgDrawGlyphs(
	VGFont Font,
	VGint GlyphCount,
	const VGuint * GlyphIndices,
	const VGfloat * AdjustmentsX,
	const VGfloat * AdjustmentsY,
	VGbitfield PaintModes,
	VGboolean AllowAutoHinting
	)
{
	vgmENTERAPI(vgDrawGlyphs)
	{
		vgsFONT_PTR font;
		VGint index;
		vgsGLYPH_PTR * head;
		vgsGLYPH_PTR previous;
		vgsGLYPH_PTR glyph;
		VGfloat offsetX;
		VGfloat offsetY;
		VGint i;

		gcmTRACE_ZONE(
			gcvLEVEL_INFO, gcvZONE_PARAMETERS,
			"%s(0x%08X, %d, GlyphIndices, AdjustmentsX, AdjustmentsY, 0x%04X, %d);\n",
			__FUNCTION__,
			Font, GlyphCount, PaintModes, AllowAutoHinting
			);

		/* Validate the glyph count. */
		if (GlyphCount <= 0)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the glyph indices pointer. */
		if (vgmIS_INVALID_PTR(GlyphIndices, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate pointer alignment. */
		if (vgmIS_MISSALIGNED(AdjustmentsX, 4) ||
			vgmIS_MISSALIGNED(AdjustmentsY, 4))
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the paint modes. */
		if ((PaintModes & ~(VG_STROKE_PATH | VG_FILL_PATH)) != 0)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Validate the font object. */
		if (!vgfVerifyUserObject(Context, Font, vgvOBJECTTYPE_FONT))
		{
			vgmERROR(VG_BAD_HANDLE_ERROR);
			break;
		}

		/* Cast the object. */
		font = (vgsFONT_PTR) Font;

		/* See if the font table has been created. */
		if (font->table == gcvNULL)
		{
			vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
			break;
		}

		/* Test to see if all characters are present. */
		for (i = 0; i < GlyphCount; i++)
		{
			/* Determine the hash table index. */
			index = vgmHASHINDEX(GlyphIndices[i], font->tableEntries);

			/* Get a pointer to the head of the collision chain. */
			head = &font->table[index];

			/* Find the glyph. */
			_FindGlyph(Context, GlyphIndices[i], head, &previous, &glyph);

			/* Bad index? */
			if (glyph == gcvNULL)
			{
				vgmERROR(VG_ILLEGAL_ARGUMENT_ERROR);
				break;
			}
		}

		/* All characters are present? */
		if (i != GlyphCount)
		{
			/* No, ignore the call. */
			break;
		}

		/* Set the proper matrices. */
		Context->drawImageSurfaceToPaint
			= &Context->glyphFillSurfaceToPaint;

		Context->drawImageSurfaceToImage
			= &Context->glyphSurfaceToImage;

		Context->drawPathFillSurfaceToPaint
			= &Context->glyphFillSurfaceToPaint;

		Context->drawPathStrokeSurfaceToPaint
			= &Context->glyphStrokeSurfaceToPaint;

		for (i = 0; i < GlyphCount; i++)
		{
			/* Determine the hash table index. */
			index = vgmHASHINDEX(GlyphIndices[i], font->tableEntries);

			/* Get a pointer to the head of the collision chain. */
			head = &font->table[index];

			/* Find the glyph. */
			_FindGlyph(Context, GlyphIndices[i], head, &previous, &glyph);
			gcmASSERT(glyph != gcvNULL);

			/* Set the offset of the current glyph. */
			offsetX = Context->glyphOrigin[0] - glyph->originX;
			offsetY = Context->glyphOrigin[1] - glyph->originY;

			/* Different offsets? */
			if ((offsetX != Context->glyphOffsetX) ||
				(offsetY != Context->glyphOffsetY))
			{
				/* Update the offset in the context. */
				Context->glyphOffsetX = offsetX;
				Context->glyphOffsetY = offsetY;

				/* Invalidate the translated matrix. */
				vgfInvalidateContainer(
					Context, &Context->translatedGlyphUserToSurface
					);
			}

			/* Image-based glyph? */
			if (glyph->image != gcvNULL)
			{
				/* Draw the image. */
				vgfTesselateImage(
					Context,
					&Context->targetImage,
					glyph->image
					);
			}

			/* Path-based glyph? */
			else if (glyph->path != gcvNULL)
			{
				/* Draw the path. */
				vgfDrawPath(
					Context,
					&Context->targetImage,
					glyph->path,
					PaintModes,
					Context->fillPaint,
					Context->strokePaint,
					Context->colTransform,
					Context->maskingEnable
					);
			}

			Context->glyphOrigin[0] += glyph->escapementX;
			Context->glyphOrigin[1] += glyph->escapementY;

			if (AdjustmentsX != gcvNULL)
			{
				Context->glyphOrigin[0] += AdjustmentsX[i];
			}

			if (AdjustmentsY != gcvNULL)
			{
				Context->glyphOrigin[1] += AdjustmentsY[i];
			}
		}
	}
	vgmLEAVEAPI(vgDrawGlyphs);
}
